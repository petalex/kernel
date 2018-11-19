/*
 * pcb.cpp
 *
 *  Created on: May 9, 2017
 *      Author: OS1
 */

#include "pcb.h"

ID PCB::nextID = 0;
Time PCB::timeCounter = 0;
volatile PCB *PCB::running = 0;
volatile PCBElement *PCB::pcbHead = 0;
volatile PCBElement *PCB::pcbTail = 0;
volatile unsigned PCB::lockFlag = 1;
volatile unsigned PCB::switchContextFlag = 0;
volatile unsigned PCB::blockedThreadsCounter = 0;
volatile unsigned  PCB::infBlockedThreadsCounter = 0;
volatile unsigned PCB::delayedSwitchFlag = 0;

volatile unsigned PCB::blockedSignals = 0;
volatile unsigned PCB::maskedSignals = 0;

volatile PCB *PCB::idlePCB = (new IdleThread())->myPCB;
volatile PCB *PCB::mainPCB = new PCB(0);

// Global variables
unsigned oldTimerSegment = 0, oldTimerOffset = 0;
unsigned currentStackPointer, currentStackSegment, currentBasePointer;
unsigned lockCounter = 0, handleSignalsFlag = 0;

PCB::PCB (Thread *myThread, StackSize stackSize, Time timeSlice) {
	this->id = (PCB::nextID)++;
	this->myThread = myThread;
	this->timeSlice = timeSlice;
	this->stackPointer = this->stackSegment = this->basePointer = 0; // Warnings
	this->stack = 0; // Warnings
	this->state = created;
	this->signalFlag = 1;
	this->waitingHead = this->waitingTail = 0;

	this->signalHandlers = new SignalHandler[numOfSignals];
	this->signalHandlers[0] = PCB::finish; // For explicit thread finish
	// Copy parent signal info if it exists
	if(PCB::running != 0) {
		for(int i = 1; i < numOfSignals; i++) this->signalHandlers[i] = PCB::running->signalHandlers[i];
		this->myMaskedSignals = PCB::running->myMaskedSignals;
		this->myBlockedSignals = PCB::running->myBlockedSignals;
		this->myParent = (PCB *)PCB::running;
	}
	else {
		for(int i = 1; i < numOfSignals; i++) this->signalHandlers[i] = 0; // Initialise signal handlers
		this->myMaskedSignals = this->myBlockedSignals = 0;
		this->myParent = 0;
	}
	this->signalHead = this->signalTail = 0;

	this->numOfChildren = 0;
	this->mySemaphore = 0;
	if(this->myParent != 0)
		this->myParent->numOfChildren++;

	// Creating thread context
	if(stackSize > maxStackSize) stackSize = maxStackSize; // Limit maxStackSize = 64KB
	stackSize /= sizeof(REG);
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	this->stack = new REG[stackSize];
	int_unlock();
#endif
	this->stack[stackSize - 1] = 0x200;
#ifndef BCC_BLOCK_IGNORE
	stack[stackSize - 2] = FP_SEG(PCB::run);
	stack[stackSize - 3] = FP_OFF(PCB::run);
	// Moving stack pointer for all registers
	this->stackPointer = this->basePointer = FP_OFF(stack + stackSize - 12);
	this->stackSegment = FP_SEG(stack + stackSize - 12);
#endif
	PCBElement *element = 0;
#ifndef BCC_BLOCK_IGNORE
	// Putting thread in list
	int_lock();
	element = new PCBElement(this);
	int_unlock();
#endif
	PCB::pcbTail = (PCB::pcbHead == 0 ? PCB::pcbHead : PCB::pcbTail->next) = element;
}
PCB::~PCB() {
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	// Deallocate thread stack
	delete this->stack;
	int_unlock();
#endif
	// Remove from thread list
	PCBElement *prev = 0, *cur;
	for(cur = (PCBElement *)PCB::pcbHead; cur != 0; prev = cur, cur = cur->next) {
		if(cur->pcb == this) {
			if(prev == 0) PCB::pcbHead = PCB::pcbHead->next;
			else prev->next = cur->next;
#ifndef BCC_BLOCK_IGNORE
			int_lock();
			delete cur;
			int_unlock();
#endif
			break;
		}
	}
	// Unblock all blocked threads if there are some left!
	prev = 0;
	for(cur = this->waitingHead; cur != 0;) {
		cur->pcb->state = ready;
		Scheduler::put(cur->pcb);
		lock();
		// Update blocked threads counter
		--PCB::blockedThreadsCounter;
		--PCB::infBlockedThreadsCounter;
		unlock();
		prev = cur;
		cur = cur->next;
#ifndef BCC_BLOCK_IGNORE
		int_lock();
		delete prev;
		int_unlock();
#endif
	}
	this->waitingHead = this->waitingTail = 0;

	/*
	// Explicit destructor call on ready/blocked thread
	if(this->state == ready || this->state == blocked) {
		if(this->myParent != 0) {
			this->myParent->numOfChildren--;
			this->myParent = 0;
		}
		if(this->state == blocked) {
			if(this->mySemaphore != 0) {
				this->mySemaphore->remove(); // Have to define
				this->mySemaphore = 0;
			}
			else {
				// Remove from list on myBlockedThread (have to define)
			}
		}
	}*/
}
ID PCB::getId() {
	return this->id;
}
PCB *PCB::getRunning() {
	lock();
	PCB *r = (PCB *)PCB::running;
	unlock();
	return r;
}
Thread *PCB::getThreadById(ID id) {
	lock();
	for(PCBElement *cur = (PCBElement *)PCB::pcbHead; cur != 0; cur = cur->next) {
		if(cur->pcb->id == id) {
			unlock();
			return cur->pcb->myThread;
		}
	}
	unlock();
	return 0;
}
void PCB::start() {
	if(this->state != created) return;
	this->state = ready;
	Scheduler::put(this);
	// Call dispatch if this is first started thread (userMain thread)
	if(PCB::running == 0) {
		dispatch();
	}
}
void PCB::run () {
	PCB::running->myThread->run();
	// Unblock all blocked threads
	PCBElement *prev = 0;
	for(PCBElement *cur = PCB::running->waitingHead; cur != 0;) {
		cur->pcb->state = ready;
		Scheduler::put(cur->pcb);
		// Update blocked threads counter
		lock();
		--PCB::blockedThreadsCounter;
		--PCB::infBlockedThreadsCounter;
		unlock();
		prev = cur;
		cur = cur->next;
#ifndef BCC_BLOCK_IGNORE
		int_lock();
		delete prev;
		int_unlock();
#endif
	}
	PCB::running->waitingHead = PCB::running->waitingTail = 0;

	if(PCB::running->myParent != 0)
		PCB::running->myParent->numOfChildren--;
	PCB::running->myParent = 0;

	// Signal parent and self before finishing
	if(PCB::running->myParent)
		PCB::running->myParent->signal(1);
	((PCB *)PCB::running)->signal(2);
	signalHandler();
	// Enable interrupts after signal handler
#ifndef BCC_BLOCK_IGNORE
		asm sti;
#endif

	// Finish current thread and switch context
	PCB::running->state = finished;
	dispatch();
}
void PCB::waitToComplete(){
	// Only wait for active thread!
	if(this->state == finished || this->state == created) return;
	// Putting thread in waiting list
	PCBElement *element = 0;
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	element =  new PCBElement((PCB *)PCB::running);
	int_unlock();
#endif
	this->waitingTail = (this->waitingHead == 0 ? this->waitingHead : this->waitingTail->next) = element;
	// Update blocked threads counter
	lock();
	++PCB::blockedThreadsCounter;
	++PCB::infBlockedThreadsCounter;
	// Block current thread and switch context
	PCB::running->state = blocked;
	unlock();
	dispatch();
}
void waitAllToComplete() {
	for(PCBElement *cur = (PCBElement *)PCB::pcbHead; cur != 0; cur = cur->next) {
		if(cur->pcb == PCB::running || cur->pcb == PCB::mainPCB ||cur->pcb == PCB::idlePCB) continue;
		cur->pcb->waitToComplete();
	}
}
void interrupt timer() {
	handleSignalsFlag = 0; // Initially, don't request signal handling
	// Regular timer interrupt
	if (!PCB::switchContextFlag) --PCB::timeCounter;
	// Time slice finished / dispatch
	if (PCB::timeCounter == 0 || PCB::switchContextFlag || PCB::delayedSwitchFlag) {
		if (PCB::lockFlag){
			if(PCB::switchContextFlag)
				PCB::switchContextFlag = 2; // In case dispatch was called, mark flag for not entering the tick
			PCB::delayedSwitchFlag = 0;
#ifndef BCC_BLOCK_IGNORE
			asm {
			mov currentStackPointer, sp
			mov currentStackSegment, ss
			mov currentBasePointer, bp
			}
#endif
			// Save stack pointer
			if(PCB::running != 0) {
				PCB::running->stackPointer = currentStackPointer;
				PCB::running->stackSegment = currentStackSegment;
				PCB::running->basePointer = currentBasePointer;

				// Put running thread to scheduler only if it is not finished/blocked/pasused
				if(PCB::running->state == ready) Scheduler::put((PCB *)PCB::running);
			}
			else {
				PCB::mainPCB->stackPointer = currentStackPointer;
				PCB::mainPCB->stackSegment = currentStackSegment;
				PCB::mainPCB->basePointer = currentBasePointer;
			}

			// Get new running thread from scheduler
			if((PCB::running = Scheduler::get()) != 0) {
				currentStackPointer = PCB::running->stackPointer;
				currentStackSegment = PCB::running->stackSegment;
				currentBasePointer = PCB::running->basePointer;
				PCB::timeCounter = PCB::running->timeSlice;
				handleSignalsFlag = 1; // Successful dispatch
			}
			else {
				// Check if there are blocked threads, but no ready thread to choose
				if(PCB::blockedThreadsCounter > 0) {
					PCB::running = PCB::idlePCB;
					currentStackPointer = PCB::running->stackPointer;
					currentStackSegment = PCB::running->stackSegment;
					currentBasePointer = PCB::running->basePointer;
					PCB::timeCounter = PCB::running->timeSlice;
				}
				else {
					currentStackPointer = PCB::mainPCB->stackPointer;
					currentStackSegment = PCB::mainPCB->stackSegment;
					currentBasePointer = PCB::mainPCB->basePointer;
				}
			}
#ifndef BCC_BLOCK_IGNORE
			asm {
			mov sp, currentStackPointer
			mov ss, currentStackSegment
			mov bp, currentBasePointer
			}
#endif
		}
		else PCB::delayedSwitchFlag = 1;
	}

	// Regular timer interrupt
	if(PCB::switchContextFlag == 0) {
		// Waiting handler for semaphores
		waitingHandler();
		// Tick
		tick();
#ifndef BCC_BLOCK_IGNORE
		asm int 60h;
#endif
	}
	else PCB::switchContextFlag = 0; // Resetting flag after successful dispatch (!Never call dispatch in locked segment!)

	// Signal handlers
	if(handleSignalsFlag) signalHandler();
}
void dispatch() {
#ifndef BCC_BLOCK_IGNORE
	asm cli
#endif
	PCB::switchContextFlag = 1;
	timer();
#ifndef BCC_BLOCK_IGNORE
	asm sti
#endif
}
void initialise() {
#ifndef BCC_BLOCK_IGNORE
	asm{
		cli
		push es
		push ax

		mov ax,0
		mov es,ax

		mov ax, word ptr es:0022h
		mov word ptr oldTimerSegment, ax
		mov ax, word ptr es:0020h
		mov word ptr oldTimerOffset, ax

		mov word ptr es:0022h, seg timer
		mov word ptr es:0020h, offset timer

		mov ax, oldTimerSegment
		mov word ptr es:0182h, ax
		mov ax, oldTimerOffset
		mov word ptr es:0180h, ax

		pop ax
		pop es
		sti
	}
#endif
}
void restore() {
#ifndef BCC_BLOCK_IGNORE
	asm {
		cli
		push es
		push ax

		mov ax,0
		mov es,ax

		mov ax, word ptr oldTimerSegment
		mov word ptr es:0022h, ax
		mov ax, word ptr oldTimerOffset
		mov word ptr es:0020h, ax

		pop ax
		pop es
		sti
	}
#endif
}

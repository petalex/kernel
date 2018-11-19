/*
 * kernelsem.cpp
 *
 *  Created on: May 18, 2017
 *      Author: OS1
 */

#include "kersem.h"


#include "pcb.h"

volatile SemElement *KernelSem::semHead = 0;
volatile SemElement *KernelSem::semTail = 0;

KernelSem::KernelSem (int init): value(init), waitingHead(0), waitingTail(0) {
	// Putting semaphore in list
	SemElement *element = 0;
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	element = new SemElement(this);
	int_unlock();
#endif
	lock();
	KernelSem::semTail = (KernelSem::semHead == 0 ? KernelSem::semHead : KernelSem::semTail->next) = element;
	unlock();
}
KernelSem::~KernelSem() {
	// Remove semaphore from list
	lock();
	SemElement *p = 0;
	for(SemElement *cur = (SemElement *)KernelSem::semHead; cur != 0; p = cur, cur = cur->next) {
		if(cur->sem == this) {
			if(p == 0) KernelSem::semHead = KernelSem::semHead->next;
			else p->next = cur->next;
#ifndef BCC_BLOCK_IGNORE
			int_lock();
			delete cur;
			int_unlock();
#endif
			break;
		}
	}
	unlock();
	// Unblock all blocked threads if there are some left!
	WaitElement *prev = 0;
	while(this->waitingHead) {
		lock();
		deblock();
		unlock();
	}
	this->waitingHead = 0;
}
void KernelSem::block(int maxTimeToWait) {
	WaitElement *element = 0;
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	element = new WaitElement((PCB *)PCB::running, maxTimeToWait);
	int_unlock();
#endif
	// Putting thread in waiting list in sorted order
	if(this->waitingHead == 0) {
		this->waitingHead = this->waitingTail = element;
	}
	else {
		if(element->isWaiting) {
			WaitElement *cur = this->waitingHead, *prev = 0;
			while(cur != 0 && cur->isWaiting && element->time >= cur->time) {
				element->time -= cur->time;
				prev = cur;
				cur = cur->next;
			}
			// Insert element between prev and cur and update times
			if(cur == 0) this->waitingTail = element;
			element->next = cur;
			(prev == 0) ? this->waitingHead : prev->next = element;
			if(cur != 0 && cur->isWaiting) cur->time -= element->time;
		}
		else {
			// Put not waiting threads in the end
			this->waitingTail->next = element;
			this->waitingTail = element;
		}
	}
	// Update blocked threads counter
	++PCB::blockedThreadsCounter;
	if(element->isWaiting == 0)
		++PCB::infBlockedThreadsCounter;
	PCB::running->mySemaphore = this;
	// Block current thread and switch context
	PCB::running->state = blocked;
	unlock();
	dispatch();
	lock();
}
void KernelSem::deblock(int signalFlag) {
	WaitElement * cur = this->waitingHead;
	this->waitingHead = this->waitingHead->next;
	if(this->waitingHead == 0) this->waitingTail = 0;
	// Update waitingHead time
	if(this->waitingHead && this->waitingHead->isWaiting) this->waitingHead->time += cur->time;
	// Deblock thread
	cur->pcb->state = ready;
	Scheduler::put(cur->pcb);
	// Update blocked threads counter
	--PCB::blockedThreadsCounter;
	if(cur->isWaiting == 0)
		--PCB::infBlockedThreadsCounter;
	PCB::running->mySemaphore = 0;
	// Update signal flag (==0 if thread's time is up)
	cur->pcb->signalFlag = signalFlag;
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	delete cur;
	int_unlock();
#endif
}
int KernelSem::wait (Time maxTimeToWait) {
	lock();
	// Set signal flag in the begining of each wait!
	PCB::running->signalFlag = 1;
	if(--this->value < 0)
		this->block(maxTimeToWait);
	unsigned r = PCB::running->signalFlag;
	unlock();
	return r;
}
void KernelSem::signal(int signalFlag) {
	lock();
	if(this->value++ < 0)
		this->deblock(signalFlag);
	unlock();
}
int KernelSem::val () const {
	return value;
}
void waitingHandler() {
	for(SemElement *cur = (SemElement *)KernelSem::semHead; cur != 0; cur = cur->next) {
		if(cur->sem->waitingHead != 0) {
			if(cur->sem->waitingHead->time > 0)
				cur->sem->waitingHead->time--;
			// Do not change semaphore list inside locked section(for block/deblock)
			if(PCB::lockFlag) {
				// Deblock threads that finished waiting (and is waiting thread!)
				while(cur->sem->waitingHead != 0 && cur->sem->waitingHead->time == 0 && cur->sem->waitingHead->isWaiting)
					cur->sem->signal(0);
			}
		}
	}
}

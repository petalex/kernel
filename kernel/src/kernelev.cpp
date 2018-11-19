/*
 * kernelev.cpp
 *
 *  Created on: May 23, 2017
 *      Author: OS1
 */

#include "kernelev.h"
#include "ivtentry.h"

extern IVTEntry *ivtEntries[];

KernelEv::KernelEv (PCB *pcb, IVTNo i): myPCB(pcb), ivtNo(i), value(0), blockedPCB(0) {
	if (this->ivtNo >= 0 && this->ivtNo < numOfInterruptEntries && ivtEntries[this->ivtNo] && ivtEntries[this->ivtNo]->myKernelEv == 0) {
		lock();
		ivtEntries[this->ivtNo]->myKernelEv = this; // Set kernel event for IVT entry
		unlock();
	}
}
KernelEv::~KernelEv () {
	// Restore IVT entry's kernel event
	if(ivtEntries[this->ivtNo] != 0) {
		lock();
		ivtEntries[this->ivtNo]->myKernelEv = 0;
		unlock();
	}
}
void KernelEv::block() {
	// Set blocked PCB
	this->blockedPCB = (PCB *)PCB::running;
	// Update blocked threads counter
	lock();
	++PCB::blockedThreadsCounter;
	++PCB::infBlockedThreadsCounter;
	unlock();
	// Block current thread and switch context
	PCB::running->state = blocked;
	dispatch();
}
void KernelEv::deblock() {
	this->blockedPCB->state = ready;
	Scheduler::put(this->blockedPCB);
	// Update blocked threads counter
	lock();
	--PCB::blockedThreadsCounter;
	--PCB::infBlockedThreadsCounter;
	unlock();
	// Set blocked PCB
	this->blockedPCB = 0;
}
void KernelEv::wait () {
	if(PCB::running == this->myPCB) {
		if(this->value == 1)
			this->value = 0;
		else
			this->block();
	}
}
void KernelEv::signal() {
	if(this->blockedPCB == 0)
		this->value = 1; // If my thread is not blocked
	else
		this->deblock();
}

/*
 * ivtentry.cpp
 *
 *  Created on: May 23, 2017
 *      Author: OS1
 */

#include <dos.h>
#include "ivtentry.h"

// Global array of ivt entries
IVTEntry *ivtEntries[numOfInterruptEntries];

IVTEntry::IVTEntry(IVTNo i, pInterrupt routine): ivtNo(i), myKernelEv(0), newRoutine(routine), oldRoutine(0) {
#ifndef BCC_BLOCK_IGNORE
	// Save old routine
	asm cli;
	this->oldRoutine = getvect(this->ivtNo);
	// Initialise new routine
	setvect(this->ivtNo, newRoutine);
	asm sti
#endif
	// Put this IVT Entry in global array
	if(ivtEntries[this->ivtNo] == 0) {
		lock();
		ivtEntries[this->ivtNo] = this;
		unlock();
	}
}
IVTEntry::~IVTEntry() {
	this->newRoutine(); // Keyboard bug in public test
#ifndef BCC_BLOCK_IGNORE
	// Restore old routine
	asm cli;
	setvect(this->ivtNo, this->oldRoutine);
	asm sti;
#endif
	lock();
	ivtEntries[this->ivtNo] = 0;
	unlock();
}
void IVTEntry::signal(IVTNo ivtNo) {
	if(ivtNo >= 0 && ivtNo < numOfInterruptEntries && ivtEntries[ivtNo] != 0 && ivtEntries[ivtNo]->myKernelEv)
		ivtEntries[ivtNo]->myKernelEv->signal();
}
void IVTEntry::callOldRoutine(IVTNo ivtNo) {
	if(ivtNo >= 0 && ivtNo < numOfInterruptEntries && ivtEntries[ivtNo] != 0)
		ivtEntries[ivtNo]->oldRoutine();
}



/*
 * event.cpp
 *
 *  Created on: May 23, 2017
 *      Author: OS1
 */

#include "event.h"
#include "kernelev.h"
#include "pcb.h"

Event::Event (IVTNo ivtNo): myImpl(0) {
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	this->myImpl = new KernelEv((PCB *)PCB::running, ivtNo);
	int_unlock();
#endif
}
Event::~Event () {
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	delete this->myImpl;
	int_unlock();
#endif
}

void Event::wait () {
	this->myImpl->wait();
}
void Event::signal() {
	this->myImpl->signal();
}




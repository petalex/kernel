/*
 * kernelev.h
 *
 *  Created on: May 23, 2017
 *      Author: OS1
 */

#ifndef _kernelev_h_
#define _kernelev_h_

#include "event.h"
#include "pcb.h"

typedef unsigned char IVTNo;

class KernelEv {
public:
	KernelEv (PCB *myPCB, IVTNo ivtNo);
	~KernelEv ();

	void wait ();
	void signal();
private:
	void block();
	void deblock();

	IVTNo ivtNo;
	PCB *myPCB, *blockedPCB;
	int value;
};

#endif

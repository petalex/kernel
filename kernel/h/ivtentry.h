/*
 * ivtentry.h
 *
 *  Created on: May 23, 2017
 *      Author: OS1
 */

#ifndef _ivtentry_h_
#define _ivtentry_h_

#include "event.h"
#include "kernelev.h"

#define numOfInterruptEntries 256

typedef void interrupt (*pInterrupt)(...);
typedef unsigned char IVTNo;

class IVTEntry {
public:
	IVTEntry(IVTNo ivtNo, pInterrupt routine);
	~IVTEntry();
	static void signal(IVTNo ivtNo);
	static void callOldRoutine(IVTNo ivtNo);
private:
	friend class KernelEv;

	IVTNo ivtNo;
	KernelEv *myKernelEv;
	pInterrupt newRoutine, oldRoutine;
};

#endif

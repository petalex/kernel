/*
 * event.h
 *
 *  Created on: May 23, 2017
 *      Author: OS1
 */

#ifndef _event_h_
#define _event_h_

#include "ivtentry.h"

typedef unsigned char IVTNo;
class KernelEv;

#define PREPAREENTRY(ivtNo, callOld)\
	void interrupt newInterruptRoutine##ivtNo(...) {\
		IVTEntry::signal(ivtNo);\
		if (callOld == 1)\
			IVTEntry::callOldRoutine(ivtNo);\
	}\
	IVTEntry ivtEntry##ivtNo(ivtNo, newInterruptRoutine##ivtNo);

class Event {
public:
	Event (IVTNo ivtNo);
	~Event ();

	void wait ();

protected:
	friend class KernelEv;
	void signal(); // can call KernelEv

private:
	KernelEv* myImpl;
};

#endif

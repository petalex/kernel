/*
 * kersem.h
 *
 *  Created on: May 18, 2017
 *      Author: OS1
 */

#ifndef _kersem_h_
#define _kersem_h_

#include "pcb.h"

class KernelSem;

typedef struct WElement {
	PCB *pcb;
	Time time;
	unsigned isWaiting;
	WElement *next;
	WElement(PCB *p, Time t, WElement *n = 0): pcb(p), time(t), next(n) {
		this->isWaiting = this->time > 0 ? 1 : 0;
	}
} WaitElement;
typedef struct SElement {
	KernelSem *sem;
	SElement *next;
	SElement(KernelSem *s, SElement *n = 0): sem(s), next(n) {}
} SemElement;

class KernelSem {
public:
	KernelSem (int init = 1);
	~KernelSem ();
	int wait (Time maxTimeToWait);
	void signal(int signalFlag = 1);
	int val () const; // Returns the current value of the semaphore

	friend void waitingHandler();
private:
	void block(int maxTimeToWait);
	void deblock(int signalFlag = 1);

	volatile static SemElement *semHead, *semTail;

	volatile int value;
	WaitElement *waitingHead, *waitingTail;
};

#endif

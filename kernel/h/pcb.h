/*
 * pcb.h
 *
 *  Created on: May 9, 2017
 *      Author: OS1
 */

#ifndef _pcb_h_
#define _pcb_h_

extern unsigned lockCounter;
#define lock() PCB::lockFlag = 0;\
			   lockCounter++;
#define unlock() if (--lockCounter == 0) {\
					 PCB::lockFlag = 1;\
			   	 }\
				 if (PCB::lockFlag && PCB::delayedSwitchFlag) {\
					 dispatch();\
				 }
#define int_lock()\
			asm pushf;\
			asm cli;
#define int_unlock()\
			asm popf;
#define maxStackSize 65536
#define numOfSignals 16

#include <dos.h>
#include "thread.h"
#include "semaphor.h"
#include "schedule.h"

typedef unsigned REG;
typedef struct Element {
	PCB *pcb;
	Element *next;
	Element(PCB *p, Element *n = 0): pcb(p), next(n) {}
} PCBElement;
enum State {created, ready, blocked, finished, paused};
typedef struct SigElement {
	SignalId id;
	SigElement *next;
	SigElement(SignalId i, SigElement *n = 0): id(i), next(n) {}
} SignalElement;

class PCB {
public:
	PCB (Thread *myThread, StackSize stackSize = defaultStackSize, Time timeSlice = defaultTimeSlice);
	~PCB();
	static void run();
	ID getId();
	static PCB *getRunning();
	static Thread * getThreadById(ID id);
	void start();
	void waitToComplete();

	friend void initialise();
	friend void restore();
	friend void interrupt timer();
	friend void dispatch();
	friend void waitAllToComplete();
	friend void signalHandler();

	void signal(SignalId signal);
	void registerHandler(SignalId signal, SignalHandler handler);
	SignalHandler getHandler(SignalId signal);
	void maskSignal(SignalId signal);
	static void maskSignalGlobally(SignalId signal);
	void unmaskSignal(SignalId signal);
	static void unmaskSignalGlobally(SignalId signal);
	void blockSignal(SignalId signal);
	static void blockSignalGlobally(SignalId signal);
	void unblockSignal(SignalId signal);
	static void unblockSignalGlobally(SignalId signal);
	static void pause();
	static void finish();

	static ID nextID;
	static Time timeCounter;
	volatile static PCB *running;
	volatile static PCB *mainPCB;
	volatile static PCB *idlePCB;
	volatile static PCBElement *pcbHead, *pcbTail;
	volatile static unsigned lockFlag, switchContextFlag, blockedThreadsCounter, infBlockedThreadsCounter, delayedSwitchFlag;

	volatile static unsigned blockedSignals, maskedSignals;

	ID id;
	Thread *myThread;
	Time timeSlice;
	REG stackPointer, stackSegment, basePointer, *stack;
	State state;
	unsigned signalFlag;
	PCBElement *waitingHead, *waitingTail;
	unsigned numOfChildren;
	KernelSem *mySemaphore;

	SignalHandler *signalHandlers;
	unsigned myBlockedSignals, myMaskedSignals;
	PCB *myParent;
	SignalElement *signalHead, *signalTail;

};

// Idle thread
class IdleThread: public Thread {
public:
	IdleThread(): Thread() {}
	~IdleThread() {}
	void run() {
		while(PCB::blockedThreadsCounter > 0) {}
	}
};

void tick();
void waitingHandler();

#endif

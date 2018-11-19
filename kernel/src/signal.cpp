/*
 * signal.cpp
 *
 *  Created on: May 28, 2017
 *      Author: OS1
 */

#include "pcb.h"

void PCB::signal(SignalId signal) {
	if(this->state == finished) return; // Don't signal finished thread
	if((this->myMaskedSignals & (1U << signal)) || (PCB::maskedSignals & (1U << signal))) return; // Locally or globally masked
	SignalElement *element = 0;
#ifndef BCC_BLOCK_IGNORE
	// Putting thread in list
	int_lock();
	element = new SignalElement(signal);
	int_unlock();
#endif
	this->signalHead = (this->signalHead == 0 ? this->signalHead : this->signalTail->next) = element;
	// Unpause if paused
	if(this->state == paused) {
		this->state = ready;
		Scheduler::put(this);
		// Update blocked threads counter
		lock();
		--PCB::blockedThreadsCounter;
		--PCB::infBlockedThreadsCounter;
		unlock();
	}
}
void PCB::registerHandler(SignalId signal, SignalHandler handler) {
	if(signal >= 0 && signal < 16)
		this->signalHandlers[signal] = handler;
}
SignalHandler PCB::getHandler(SignalId signal) {
	return this->signalHandlers[signal];
}
void PCB::maskSignal(SignalId signal) {
	this->myMaskedSignals |= (1U << signal);
}
void PCB::maskSignalGlobally(SignalId signal) {
	PCB::maskedSignals |= (1U << signal);
}
void PCB::unmaskSignal(SignalId signal) {
	this->myMaskedSignals &= ~(1U << signal);
}
void PCB::unmaskSignalGlobally(SignalId signal) {
	PCB::maskedSignals &= ~(1U << signal);
}
void PCB::blockSignal(SignalId signal) {
	this->myBlockedSignals |= (1U << signal);
}
void PCB::blockSignalGlobally(SignalId signal) {
	PCB::blockedSignals |= (1U << signal);
}
void PCB::unblockSignal(SignalId signal) {
	unsigned wasBlocked = this->myBlockedSignals & (1U << signal);
	this->myBlockedSignals &= ~(1U << signal);
	/*// Check if thread had blocked signal that's unblocked now and unpause if paused
	if(wasBlocked && this->state == paused) {
		for(SignalElement *cur = this->signalHead; cur != 0; cur = cur->next) {
			if(cur->id == signal) {
				// Unpause
				this->state = ready;
				Scheduler::put(this);
				// Update blocked threads counter
				lock();
				--PCB::blockedThreadsCounter;
				unlock();
				break;
			}
		}
	}*/
}
void PCB::unblockSignalGlobally(SignalId signal) {
	unsigned wasBlockedGlobally = PCB::blockedSignals & (1U << signal);
	PCB::blockedSignals &= ~(1U << signal);
	/*// Check if any thread had blocked signal that's globally unblocked now and unpause if paused
	if(wasBlockedGlobally) {
		for(PCBElement *elem = (PCBElement *)PCB::pcbHead; elem != 0; elem = elem->next) {
			if(elem->pcb->state == paused) {
				for(SignalElement *cur = elem->pcb->signalHead; cur != 0; cur = cur->next) {
					if(cur->id == signal) {
						// Unpause
						elem->pcb->state = ready;
						Scheduler::put(elem->pcb);
						// Update blocked threads counter
						lock();
						--PCB::blockedThreadsCounter;
						unlock();
						break;
					}
				}
			}
		}
	}*/
}
void PCB::pause() {
	// Update blocked threads counter
	lock();
	++PCB::blockedThreadsCounter;
	++PCB::infBlockedThreadsCounter;
	// Pause current thread and switch context
	PCB::running->state = paused;
	unlock();
	dispatch();
}
void signalHandler() {
#ifndef BCC_BLOCK_IGNORE
	asm sti;
#endif
	lock();
	SignalElement * prev = 0;
	for(SignalElement *cur = PCB::running->signalHead; cur != 0;) {
		// Locally or globally blocked
		if((PCB::running->myBlockedSignals & (1U << cur->id)) || (PCB::blockedSignals & (1U << cur->id))) {
			// Skip blocked signal
			prev = cur;
			cur = cur->next;
		}
		else {
			if(PCB::running->signalHandlers[cur->id] != 0) {
				PCB::running->signalHandlers[cur->id]();
			}
			// Removing handled signal
			if(prev == 0) PCB::running->signalHead = PCB::running->signalHead->next;
			else prev->next = cur->next;
#ifndef BCC_BLOCK_IGNORE
			int_lock();
			delete cur;
			int_unlock();
#endif
			cur = (prev == 0) ? PCB::running->signalHead : prev->next;
		}
	}
	unlock();
#ifndef BCC_BLOCK_IGNORE
	asm cli;
#endif
}
void PCB::finish() {
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

	// Signal parent before finishing
	if(PCB::running->myParent) {
		PCB::running->myParent->signal(1);
	}

	if(PCB::running->myParent != 0) {
		PCB::running->myParent->numOfChildren--;
		PCB::running->myParent = 0;
	}

	// Finish current thread and switch context
	PCB::running->state = finished;
	unlock();
	dispatch();
}

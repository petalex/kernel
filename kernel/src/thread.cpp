/*
 * thread.cpp
 *
 *  Created on: May 12, 2017
 *      Author: OS1
 */

#include "thread.h"
#include "pcb.h"

Thread::Thread (StackSize stackSize, Time timeSlice): myPCB(0) {
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	this->myPCB = new PCB(this, stackSize, timeSlice);
	int_unlock();
#endif
}
Thread::~Thread() {
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	delete this->myPCB;
	int_unlock();
#endif
}
ID Thread::getId() {
	if(this) return this->myPCB->getId();
	return -1;
}
ID Thread::getRunningId() {
	return PCB::getRunning()->getId();
}
Thread * Thread::getThreadById(ID id) {
	return PCB::getThreadById(id);
}
void Thread::start() {
	if(this) this->myPCB->start();
}
void Thread::waitToComplete() {
	if(this) this->myPCB->waitToComplete();
}
void Thread::signal(SignalId signal) {
	if(this) this->myPCB->signal(signal);
}
void Thread::registerHandler(SignalId signal, SignalHandler handler) {
	if(this) this->myPCB->registerHandler(signal, handler);
}
SignalHandler Thread::getHandler(SignalId signal) {
	if(this) this->myPCB->getHandler(signal);
	return 0;
}
void Thread::maskSignal(SignalId signal) {
	if(this) this->myPCB->maskSignal(signal);
}
void Thread::maskSignalGlobally(SignalId signal) {
	PCB::maskSignalGlobally(signal);
}
void Thread::unmaskSignal(SignalId signal) {
	if(this) this->myPCB->unmaskSignal(signal);
}
void Thread::unmaskSignalGlobally(SignalId signal) {
	PCB::unmaskSignalGlobally(signal);
}
void Thread::blockSignal(SignalId signal) {
	if(this) this->myPCB->blockSignal(signal);
}
void Thread::blockSignalGlobally(SignalId signal) {
	PCB::blockSignalGlobally(signal);
}
void Thread::unblockSignal(SignalId signal) {
	if(this) this->myPCB->unblockSignal(signal);
}
void Thread::unblockSignalGlobally(SignalId signal) {
	PCB::unblockSignalGlobally(signal);
}
void Thread::pause() {
	PCB::pause();
}

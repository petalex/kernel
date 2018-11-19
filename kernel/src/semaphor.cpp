/*
 * semaphor.cpp
 *
 *  Created on: May 18, 2017
 *      Author: OS1
 */

#include "semaphor.h"
#include "kersem.h"

Semaphore::Semaphore (int init): myImpl(0) {
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	this->myImpl = new KernelSem(init);
	int_unlock();
#endif

}
Semaphore::~Semaphore () {
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	delete this->myImpl;
	int_unlock();
#endif
}
int Semaphore::wait (Time maxTimeToWait) {
	return this->myImpl->wait(maxTimeToWait);
}
void Semaphore::signal() {
	this->myImpl->signal();
}
int Semaphore::val () const {
	return this->myImpl->val();
}



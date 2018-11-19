/*
 * main.cpp
 *
 *  Created on: May 12, 2017
 *      Author: OS1
 */

#include <iostream.h>
#include "pcb.h"

int userMain(int argc, char** argv);

// User main thread
class UserMainThread: public Thread {
public:
	UserMainThread(int c, char** v): argc(c), argv(v), result(0) {}
	~UserMainThread() {}
	void run() {
		this->result = userMain(argc, argv);
	}
	int getResult() { return this->result; }
private:
	int argc, result;
	char** argv;
};

int main(int argc, char *argv[]) {
	initialise();
	UserMainThread *userMain = 0;
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	userMain = new UserMainThread(argc, argv);
	cout << "User main started" << endl;
	int_unlock();
#endif
	userMain->start();
	waitAllToComplete();
#ifndef BCC_BLOCK_IGNORE
	int_lock();
	cout << "User main returned " << userMain->getResult() << endl;
	int_unlock();
#endif
	restore();
	return userMain->getResult();
}



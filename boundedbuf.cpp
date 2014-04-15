#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "semaphore.h"

//g++ semaphore.cpp boundedbuff.cpp
//./a.out	default output for all compilations

//***define constants***
const int MAXCHAR = 10;
const int BUFFSIZE = 3;
enum {PUT_ITEM, TAKE_ITEM}; // set up names of my 2 semaphores

//***function prototyping***
void producer_proc(SEMAPHORE &, char *);
void parent_cleanup(SEMAPHORE &, int);
void consumer_proc(SEMAPHORE &, char *);

//***translation of in-class pseudocode of the producer and consumer process
int main(){
	//***used for shared buffer
	int shmid;
	char *shmBUF;

	//***instantiate two semaphores
	//***TAKE_ITEM semaphore is set to 0 in the next line
	//***the two semaphores are actually in an array [0] and [1]
	SEMAPHORE sem(2);//***one semaphore object called sem that will house two semaphores
	sem.V(PUT_ITEM);//***increment the PUT_ITEM variable to an initial value of 3
	sem.V(PUT_ITEM);//***this is the V process, also resumes a process that has been halted
	sem.V(PUT_ITEM);//***as of now no process has been halted yet

	//***set up the shared buffer / shared memory segment in Linux lingo
	//***PERMS = 0600		user who's executing this program and read and write memory
	//***BUFFSIZE*sizeof(char)	specifies bytes of memory that you will need. Memory enough to store 3 characters.
	//***IPC_PRIVATE		gives you a random key
	//***shmat			makes newly allocated memory available to the program by the pointer shmBUF

	//***0, SHM_RND			keep these the same in your program. has to do with how memory is organized
	shmid = shmget(IPC_PRIVATE, BUFFSIZE*sizeof(char), PERMS);//***allocate memory enough to store 3 characters***
	shmBUF = (char *)shmat(shmid, 0, SHM_RND);//***now shmBUF pointer points to the new buffer memory

	//***both parent and child have the same semaphores and shmBUF / shared memory buffer
	//***two semaphores and shared memory buffer are shared by both parent and child
	if(fork()){ /* parent process */

		producer_proc(sem, shmBUF);//***parent
		parent_cleanup(sem, shmid);

	} else { // child process
		consumer_proc(sem, shmBUF);//***child
	}

	exit(0);
} // main

//***function definition***
void consumer_proc(SEMAPHORE &sem, char *shmBUF) {
	char tmp;

	for(int k=0; k<MAXCHAR; k++){
		sem.P(TAKE_ITEM);//***decrement TAKE_ITEM
		tmp = *(shmBUF+k%BUFFSIZE);//***takes an item from the buffer
		sem.V(PUT_ITEM);//***increment PUT_ITEM
		cout << "(" << getpid() << ")  " //***consumes the item
				<< "buf[" << k%BUFFSIZE << "] "
				<< tmp << endl;
	}
} // child_proc

void producer_proc(SEMAPHORE &sem, char *shmBUF) {

	//***characters to be copied to the buffer
	char data[128];
	cout << "(" << getpid() << ")  Please enter a string --> ";
	cin.getline(data, 127);

	
	char input;
	//***this is the same loop in the producer loop pseudocode
	for(int k=0; k<MAXCHAR; k++){//***this is the "increment pointer" pseudocode
		input = data[k];//***produce an item. takes one character from data
		sem.P(PUT_ITEM);//***P(PUT_ITEM), PUT_ITEM is decremented
		*(shmBUF+(k%BUFFSIZE)) = input;//***copy character into buffer. this is pointer arithmetic
		sem.V(TAKE_ITEM);//***V(TAKE_ITEM), TAKE_ITEM is incremented
	}

	//***k%BUFFSIZE		k takes on the values from [0, 10). remember that BUFFSIZE is 3
	//k = 0			0%3 = 0
	//k = 1			1%3 = 1
	//k = 2			2%3 = 2
	//k = 3			3%3 = 0
	//k = 4			4%3 = 1
	//k = 5			5%3 = 2
	//k = 6			6%3 = 0...
} // parent_proc

void parent_cleanup (SEMAPHORE &sem, int shmid) {

	int status;			/* child status */
	wait(0);	/* wait for child to exit */
	//***deallocates the shared memory
	shmctl(shmid, IPC_RMID, NULL);	/* cleaning up */
	sem.remove();//***deallocates the semaphores
} // parent_cleanup


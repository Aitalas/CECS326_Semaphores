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
//./a.out	

const int MAXCHAR = 10;
const int BUFFSIZE = 3;
enum {SAVE, CHECK, VACA}; 

//Function Prototype
void parent_cleanup(SEMAPHORE &, int);


int main(){
	//Initialization
	int DEPOSIT = 50;
	int WITHDRAW = 100;
	int SAVINGS_TO_VACATION = 150;
	int SAVEVACA_TO_CHECKING = 200;

	//Initialize shared buffer of 3 integers
	int shmid;
	char *shmBUF;

	shmid = shmget(IPC_PRIVATE, BUFFSIZE*sizeof(int), PERMS);
	shmBUF = (int *)shmat(shmid, 0, SHM_RND);//shmBUF points to new buffer memory

	if(fork()){ /* parent process */

		producer_proc(sem, shmBUF);//***parent
		parent_cleanup(sem, shmid);

	} else { // child process
		consumer_proc(sem, shmBUF);//***child
	}

	exit(0);
} // main

void parent_cleanup (SEMAPHORE &sem, int shmid) {

	int status;			/* child status */
	wait(0);			/* wait for child to exit */
					//deallocates the shared memory
	shmctl(shmid, IPC_RMID, NULL);	/* cleaning up */
	sem.remove();			//deallocates the semaphores
} // parent_cleanup



/*

Start.

Initialization:
	int DEPOSIT = 50;
	int WITHDRAW = 100;
	int SAVINGS_TO_VACATION = 150;
	int SAVEVACA_TO_CHECK = 200;

	Initialize shared buffer with 3 slots for each balance variable:
		int savings = 500;
		int checking = 300;
		int vacation = 800;

	Initialize 3 semaphores.
		SAVE	= 
		CHECK	=
		VACA	=

Operations:
	1. Deposit fund or interest.				[3001]
	2. Withdraw or clear check.				[503]
	3. Transfer from savings to vacation account.		[10007]
	4. Transfer from vacation and savings to checking.	[4001]

fork();
fork();
fork();

Loop 100 times.

	Generate random number.
	If random number is divisible by [3001],

		P(SAVE)

		Add DEPOSIT to savings.
		Save checking.
		Print [savings account][deposit][DEPOSIT][new balance].

		V(SAVE)

	Else if random number is divisible by [503],

	If WITHDRAW < checking,

		P(CHECK)

		Subtract WITHDRAW from checking.
		Save checking.
		Print [checking account][withdraw][WITHDRAW][new balance].

		V(CHECK)

	Else if random number is divisible by [10007],

	If SAVINGS_TO_VACATION < savings,

		P(SAVE)			
		P(VACA)

		Subtract SAVINGS_TO_VACATION from savings.
		ADD SAVINGS_TO_VACATION to vacation.
		Save savings.
		Save vacation.

		Print [savings account][withdraw][SAVINGS_TO_VACATION][new balance].
		Print [vacation account][deposit][SAVINGS_TO_VACATION][new balance].

		V(VACA)
		P(SAVE)


	Else if random number is divisible by [4001],

	If SAVEVACA_TO_CHECK < savings &&
		SAVEVACA_TO_CHECK < vacation,

		P(SAVE)
		P(VACA)

		Subtract SAVEVACA_TO_CHECK from savings.
		Subtract SAVEVACA_TO_CHECK from vacation.
		Add SAVEVACA_TO_CHECK to checking.
		Save savings.
		Save vacation.
		Save checking.

		Print [savings account][withdraw][SAVEVACA_TO_CHECK][new balance].
		Print [vacation account][withdraw][SAVEVACA_TO_CHECK][new balance].
		Print [checking account][deposit][2*SAVEVACA_TO_CHECK][new balance].
	
		V(SAVE)
		V(VACA)

Endloop.

End.

*/

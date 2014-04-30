#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "semaphore.h"

//g++ semaphore.cpp bankaccounts.cpp
//./a.out	

using namespace std;

const int BUFFSIZE = 3;
enum {SAVE, CHECK, VACA};//Semaphore variables

//Initialize constant values
int DEPOSIT = 50;
int WITHDRAW = 100;
int SAVINGS_TO_VACATION = 150;
int SAVEVACA_TO_CHECKING = 200;

//Function Prototype
void depositeSavings(SEMAPHORE &, int *);
void withdrawChecking(SEMAPHORE &, int *);
void transferSavingsToVacation(SEMAPHORE &, int *);
void transferSaveVacaToChecking(SEMAPHORE &, int *);
void operateAccount(SEMAPHORE &, int *);
void parent_cleanup(SEMAPHORE &, int);

int main(){
	//Initialize balance variables
	int savings = 500;
	int checking = 500;
	int vacation = 500;
	long childPID;

	//Initialize shared buffer of 3 integers
	int shmid;
	int *shmBUF;

	shmid = shmget(IPC_PRIVATE, BUFFSIZE*sizeof(int), PERMS);
	shmBUF = (int *)shmat(shmid, 0, SHM_RND);//shmBUF points to new buffer memory

	*(shmBUF + 0) = savings;//load the shared balance variables into buffer
	*(shmBUF + 1) = checking;
	*(shmBUF + 2) = vacation;

	//Initialize 3 semaphores
	SEMAPHORE sem(3);
	sem.V(SAVE);	//init values are all 1
	sem.V(CHECK);
	sem.V(VACA);

	cout << "[" << getpid() << "]: Initial values of all accounts: 500\n";

	for (int i = 0; i < 3; i++) {
		childPID = fork();

		if (childPID == 0) {
			//Child process
			cout << "[" << getpid() << "]: Operating.\n";
			operateAccount(sem, shmBUF);
			break;
		} else {
			//Parent process
			wait(0);
		}
	}

	parent_cleanup(sem, shmid);
	cout << "\n[" << getpid() << "]: Cleaning up and ending.\n";
	exit(0);
} // main

void depositSavings(SEMAPHORE &sem, int *shmBUF) {
	int savings;

	sem.P(SAVE);

	savings = *(shmBUF + 0);
	savings = savings + DEPOSIT;
	*(shmBUF + 0) = savings;
	cout << "\n[" << getpid() << "]:===========================================\n";
	cout << "\t[savings][deposit]\t[+ " << DEPOSIT << "][" << *(shmBUF + 0) << "]\n";

	sem.V(SAVE);
}

void withdrawChecking(SEMAPHORE &sem, int *shmBUF) {
	int checking;

	sem.P(CHECK);

	checking = *(shmBUF + 1);
	checking = checking - WITHDRAW;
	*(shmBUF + 1) = checking;
	cout << "\n[" << getpid() << "]:===========================================\n";
	cout << "\t[checking][withdraw]\t[- " << WITHDRAW << "][" << *(shmBUF + 1) << "]\n";

	sem.V(CHECK);
}

void transferSavingsToVacation(SEMAPHORE &sem, int *shmBUF) {
	sem.P(SAVE);
	sem.P(VACA);

	*(shmBUF + 0) = *(shmBUF + 0) - SAVINGS_TO_VACATION;
	*(shmBUF + 2) = *(shmBUF + 2) + SAVINGS_TO_VACATION;	

	cout << "\n[" << getpid() << "]:===========================================\n";
	cout << "\t[savings][withdraw]\t[- " << SAVINGS_TO_VACATION << "][" << *(shmBUF + 0) 			<< "]\n";
	cout << "\t[vacation][deposit]\t[+ " << SAVINGS_TO_VACATION << "][" << *(shmBUF + 2) 			<< "]\n";

	sem.V(VACA);
	sem.P(SAVE);
}

void transferSaveVacaToChecking(SEMAPHORE &sem, int *shmBUF) {
	cout << "\n[" << getpid() << "]:===========================================\n";
}

void operateAccount(SEMAPHORE &sem, int *shmBUF) {
	int randomNum;
	srand (time(NULL));
	randomNum = rand() % 30000 + 503;//number in range 503 to 30000

	//depositSavings(sem, shmBUF);

	for (int i = 0; i < 100; i++) {
				

		if (randomNum % 3001 == 0) {
			depositSavings(sem, shmBUF);
		} else if (randomNum % 503 == 0) {
			withdrawChecking(sem, shmBUF);
		} else if (randomNum % 10007 == 0) {
			transferSavingsToVacation(sem, shmBUF);
		} else if (randomNum % 4001 == 0) {
			transferSaveVacaToChecking(sem, shmBUF);
		}
	}
}

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
		SAVE	= ?
		CHECK	= ?
		VACA	= ?

Operations:
	1. Deposit fund or interest.				[3001]
	2. Withdraw or clear check.				[503]
	3. Transfer from savings to vacation account.		[10007]
	4. Transfer from vacation and savings to checking.	[4001]

fork();
fork();
fork();

Loop 100 times.

	Generate random number between 503 and onwards.
	If random number is divisible by [3001],

		P(SAVE)

		Add DEPOSIT to savings.
		Save checking.
		Print [childpid]: ========================================
		Print [savings account][deposit][DEPOSIT][new balance].

		V(SAVE)

	Else if random number is divisible by [503],

	If WITHDRAW < checking,

		P(CHECK)

		Subtract WITHDRAW from checking.
		Save checking.
	
		Print [childpid]: ========================================
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

		Print [childpid]: ========================================
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

		Print [childpid]: ========================================
		Print [savings account][withdraw][SAVEVACA_TO_CHECK][new balance].
		Print [vacation account][withdraw][SAVEVACA_TO_CHECK][new balance].
		Print [checking account][deposit][2*SAVEVACA_TO_CHECK][new balance].
	
		V(SAVE)
		V(VACA)

Endloop.

End.

*/

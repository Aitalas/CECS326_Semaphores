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

//Semaphore variables
enum {SAVE, CHECK, VACA};

//Initialize constant values
int DEPOSIT = 50;
int WITHDRAW = 100;
int SAVINGS_TO_VACATION = 150;
int SAVEVACA_TO_CHECK = 200;

//Function Prototype
void depositeSavings(SEMAPHORE &, int *, int &);
void withdrawChecking(SEMAPHORE &, int *, int &);
void transferSavingsToVacation(SEMAPHORE &, int *, int &);
void transferSaveVacaToChecking(SEMAPHORE &, int *, int &);
void operateAccount(SEMAPHORE &, int *, int &);
void parent_cleanup(SEMAPHORE &, int);

int main(){

	//Each child will have their own copy of the operations variable
	//to count their number of operations done.
	//The address of int operations is passed as a function argument 
	//in order to keep their operations consistent.

	//Initialize variables
	int savings = 1000;
	int checking = 1000;
	int vacation = 1000;
	int operations = 1;
	long childPID;

	//Initialize shared buffer of 3 integers
	int shmid;
	int *shmBUF;

	shmid = shmget(IPC_PRIVATE, BUFFSIZE*sizeof(int &), PERMS);
	shmBUF = (int *)shmat(shmid, 0, SHM_RND);//shmBUF points to new buffer memory

	*(shmBUF + 0) = savings;//load the shared variables into buffer
	*(shmBUF + 1) = checking;
	*(shmBUF + 2) = vacation;

	//Initialize 3 semaphores
	SEMAPHORE sem(3);
	sem.V(SAVE);	
	sem.V(CHECK);
	sem.V(VACA);

	//Fork()
	for (int i = 0; i < 3; i++) {//parent begins to fork() three children
		childPID = fork();
		
		if (childPID == -1) {//error handling
			cout << "Child process failed to spawn." << endl;
			exit(1);
		} else if (childPID == 0) {//child					
			operateAccount(sem, shmBUF, operations);
			exit(0);
		} else {//parent
			cout << "Child created = " << i+1 << endl;			
		}
	}

	//Parent operates accounts
	operateAccount(sem, shmBUF, operations);

	//Wait for all children to end
	for (int i = 0; i < 3; i++) {
		wait(NULL);
	}

	//Parent cleans up
	parent_cleanup(sem, shmid);
	cout << "Ending." << endl;

	return 0;
} 


//Deposit a set amount into the savings account and increment operations
//Then print the pid, operation, amount added/subtracted, and new balance
void depositSavings(SEMAPHORE &sem, int *shmBUF, int & operations) {
	int* savings;

	sem.P(SAVE);

	savings = (shmBUF + 0);
	*savings = *savings + DEPOSIT;
	cout << getpid() << "\t" << operations << "\t";
	cout << "[savings]\t[+ " << DEPOSIT << "]\t[" << *savings << "]\n";
	operations++;

	sem.V(SAVE);
}

//Withdraw a set amount from the checking account and increment operations
//Then print the pid, operation, amount added/subtracted, and new balance
void withdrawChecking(SEMAPHORE &sem, int *shmBUF, int & operations) {
	int* checking;

	sem.P(CHECK);
	checking = (shmBUF + 1);

	if (WITHDRAW < *checking) {
		*checking = *checking - WITHDRAW;
		cout << getpid() << "\t" << operations << "\t";
		cout << "[checking]\t[- " << WITHDRAW << "]\t[" << *checking << "]\n";
		operations++;
	}

	sem.V(CHECK);
}

//Transfer a set amount from savings to vacation and increment operations.
//Then print the pid, operation, amount added/subtracted, and new balance
void transferSavingsToVacation(SEMAPHORE &sem, int *shmBUF, int & operations) {
	int* savings;
	int* vacation;

	sem.P(SAVE);
	sem.P(VACA);

	savings = (shmBUF + 0);
	vacation = (shmBUF + 2);

	if (SAVINGS_TO_VACATION < *savings) {
		*savings = *savings - SAVINGS_TO_VACATION;//take from savings
		*vacation = *vacation + SAVINGS_TO_VACATION;//put into vacation

		cout << getpid() << "\t" << operations << "\t";
		cout << "[savings]\t[- " << SAVINGS_TO_VACATION << "]\t[" << *savings 				<< "]\n";
		cout << "\t\t[vacation]\t[+ "<< SAVINGS_TO_VACATION << "]\t[" << *vacation 				<< "]\n";
		operations++;
	}

	sem.V(VACA);
	sem.V(SAVE);
}

//Transfer a set amount from savings and vacation into checking, then 
//increment operations.
//Then print the pid, operation, amount added/subtracted, and new balance
void transferSaveVacaToChecking(SEMAPHORE &sem, int *shmBUF, int & operations) {
	int* savings;
	int* checking;
	int* vacation;
	
	sem.P(SAVE);
	sem.P(CHECK);
	sem.P(VACA);

	savings = shmBUF + 0;
	checking = shmBUF + 1;
	vacation = shmBUF + 2;

	if (SAVEVACA_TO_CHECK < *savings && SAVEVACA_TO_CHECK < *vacation) {
		*savings = *savings - SAVEVACA_TO_CHECK;
		*vacation = *vacation - SAVEVACA_TO_CHECK;
		*checking = *checking + 2*SAVEVACA_TO_CHECK;

		cout << getpid() << "\t" << operations << "\t";
		cout << "[savings]\t[- " << SAVEVACA_TO_CHECK << "]\t[" << *savings 				<< "]\n";
		cout << "\t\t[vacation]\t[- " << SAVEVACA_TO_CHECK << "]\t[" << *vacation 				<< "]\n";
		cout << "\t\t[checking]\t[+ "<< 2*SAVEVACA_TO_CHECK << "]\t[" << *checking 				<< "]\n";
		operations++;
	}
	
	sem.V(VACA);
	sem.V(CHECK);
	sem.V(SAVE);
}

//Generates a random number to determine what action to take.
//Random number range is from 0 to a minimum of 32627.
//If the random number is divisible by the action number,
//the process will take that action.
void operateAccount(SEMAPHORE &sem, int *shmBUF, int & operations) {
	int randomNum;
	srand (time(NULL));


	while (operations != 11) {//do 10 operations
		randomNum = rand();//range 0 - 32627 

		if (randomNum % 3001 == 0) {			
			depositSavings(sem, shmBUF, operations);
		} else if (randomNum % 503 == 0) {
			withdrawChecking(sem, shmBUF, operations);
		} else if (randomNum % 10007 == 0) {
			transferSavingsToVacation(sem, shmBUF, operations);
		} else if (randomNum % 4001 == 0) {
			transferSaveVacaToChecking(sem, shmBUF, operations);
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
		Print [savings account][new balance].

		V(SAVE)

	Else if random number is divisible by [503],

	If WITHDRAW < checking,

		P(CHECK)

		Subtract WITHDRAW from checking.
		Save checking.
	
		Print [childpid]: ========================================
		Print [checking account][new balance].

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
		Print [savings account][SAVINGS_TO_VACATION][new balance].
		Print [vacation account][SAVINGS_TO_VACATION][new balance].

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
		Print [savings account][SAVEVACA_TO_CHECK][new balance].
		Print [vacation account][SAVEVACA_TO_CHECK][new balance].
		Print [checking account][2*SAVEVACA_TO_CHECK][new balance].
	
		V(SAVE)
		V(VACA)

Endloop.

End.

*/

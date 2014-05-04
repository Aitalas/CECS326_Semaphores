//Cynthia Nguyen
//CECS326
//Semaphore Program
//This program has three shared accounts: savings, checking, and vacation.
//Four processes (parent and 3 children) will act on them, using semaphores
//to block any process from corrupting the balances as needed.
//All four processes will execute 100 account operations before ending, 
//where the parent will then clean up the shared buffer and end.

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


//Deposit a set amount into the savings account and increment operations.
//Then print the pid, operation, amount added/subtracted, and new balance.
//This also blocks any other process from accessing the savings account
//while this is operating.
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
//Then print the pid, operation, amount added/subtracted, and new balance.
//Since both savings and vacation accounts are acted on, processes will
//be blocked from accessing them until this action is done.
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
//Then print the pid, operation, amount added/subtracted, and new balance.
//This action operation acts on all three accounts, so processes 
//will be blocked from all accounts until the action is done.
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

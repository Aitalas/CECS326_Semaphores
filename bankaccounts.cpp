/*	Questions
	
	Parent process and 3 child processes.
	How exactly do I do this? and what account's balance do I act on first?
*/


/*

Start.

Initialization:
	int DEPOSIT = 110;
	int WITHDRAW = 150;
	int SAVA_TRANSFER = 270;
	int SAVACH_TRANSFER = 230;

	int balance = 500;//if i go with 3 separate processes for this
	int savings = 500;
	int checking = 300;
	int vacation = 800;

Operations:
	1. Deposit fund or interest.						[3001]
	2. Withdraw or clear check.						[503]
	3. Transfer from savings to vacation account.		[10007]
	4. Transfer from vacation and savings to checking.	[4001]

Loop 100 times.

	Generate random number.
	If random number is divisible by [3001],
		Add DEPOSIT to balance.
		Save balance.
		Print action, amount added, and new balance.

	Else if random number is divisible by [503],
		If WITHDRAW < balance,
			Subtract WITHDRAW from balance.

	Else if random number is divisible by [10007],
		If SAVA_TRANSFER < savings,
			Subtract SAVA_TRANSFER from savings
			ADD SAVA_TRANSFER to vacation

	Else if random number is divisible by [4001],
		

Endloop.

End.

*/

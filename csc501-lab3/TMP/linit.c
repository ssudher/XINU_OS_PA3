#include<kernel.h>
#include<proc.h>
#include "lock.h"
#include<q.h>

struct lockstruct locks[NLOCKS];
void linit()
{
	//kprintf("\n--------Lock initialization----------\n");
	struct lockstruct *lock;
	int i=NLOCKS-1;
	nextlock = NLOCKS - 1;
	while(i>=0)
	{
		lock = &locks[i];
		lock->lstate = LFREE;
		lock->ltype = READ;
		lock->lqtail = 1+ (lock->lqhead = newqueue());
		int j;
		//here the index will represent the PID and '0/1' represents if that process holds this lock or not.
		for(j=0;j<NPROC;j++)
		{
			lock->holders[j] = 0;
		}
		i--;
	}
}

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include "lock.h"

int lcreate()
{
	STATWORD PS;
	disable(PS);
	
	//new_lock contains the index of the next lock that is to be used.
	int new_lock = create_newlock();
	//kprintf("lcreate -- the new lock that was acquired was : %d\n",new_lock);
	if(new_lock == SYSERR)
	{
		restore(PS);
		return SYSERR;
	}
	
	restore(PS);
	return new_lock;
}

int create_newlock()
{
	int l,i;
	for(i=0;i<NLOCKS;i++)
	{
		//this 'nextlock' just like 'nextsem' will contain the index of the next lock that is to
		//be returned.
		l = nextlock;
		nextlock--;
		if(nextlock < 0) 
		{
			//kprintf("nextlock before = %d\n",nextlock);
			nextlock = NLOCKS -1;
			//kprintf("nextlock after = %d\n",nextlock);
		}
		//kprintf("the nextlock is : %d\n",nextlock);
		if(locks[l].lstate == LFREE)
		{
			locks[l].lstate = LUSED;
			locks[l].created_time = ctr1000;
			locks[l].my_creator = currpid;
			return l;
		}
	}
	return SYSERR;
}

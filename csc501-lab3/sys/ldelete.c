#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include "lock.h"
#include <stdio.h>

SYSCALL ldelete(int l)
{

	STATWORD PS;
	disable(PS);
	//kprintf("inside ldelete for lock: %d\n",l);
	if(l<0 || l>=50){
		restore(PS);
		return SYSERR;
	}
	struct lockstruct *lptr;
	lptr = &locks[l];
	lptr->lstate = LFREE;
	lptr->ltype = READ;
	//printsq(lptr->lqhead, lptr->lqtail);
	
	int i;
	for(i=0;i<NPROC;i++)
	{
		lptr->holders[i] = 0;
		//kprintf("%d",lptr->holders[i]);
	}
	//kprintf("check this!!! %d\n",lptr->holders[46]);
	//nextlock++;

	//Implemented: if this lock has some processes in the wait list
	//then we need to make sure those processes know that these processes needs to know that the lock is deleted.
	//run through the waitlist of this lock and make the proctab->locksheld value for this lock as DELETED.
	struct pentry *pptr;
	int temp = lptr->lqhead;
	int pidd;
	if(nonempty(temp))
	{
		while(temp!=lptr->lqtail)
		{
			pidd = getlast(lptr->lqtail);		
			pptr = &proctab[pidd];
			//kprintf("!!!Marking the locksheld[%d] as DELETED for process %d!!!\n",l,pidd);
			pptr->locksheld[l] = DELETED;
			pptr->waittime = 0;
			pptr->pstate = PRREADY;
			insert(pidd, rdyhead, pptr->pprio);
			temp = q[temp].qnext;
		}
	resched();
	}
	restore(PS);
	return OK;
}

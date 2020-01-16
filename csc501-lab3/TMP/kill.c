#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  * kill  --  kill a process and remove it from the system
 *   *------------------------------------------------------------------------
 *    */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr,*ppt;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	//if(pid == 49) kprintf("THE MAIN IS GETTING KILLED!!!\n");
	
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();
	
	//if this process is holding a lock, then call release function from here
	//to put the next guy in PRREADY state
	//if this process is waiting for a lock, then dequeue this lock from that list
	
	//you are waiting for this lock!
	//kprintf("this process %d is getting killed, so need to dequeue it from the lock %d's waiting queue\n",pid,pptr->waiting_lock);
	int pp;
	int waiting_lid;
	if(pptr->waiting_lock != 0)
	{
		int lid = pptr->waiting_lock;
		//kprintf("kill-- the waiting lock for process %d is %d\n",pid,pptr->waiting_lock);
		dequeue(pid);
		update_priority(lid);				
	}

	//comment this if it doesn't work!!
	//this is the simplified recurrsive logic for priority updation
	//check the one-note for the algorithm.
	int stop_flag = 0;
	int ll;
	while(stop_flag == 0)
	{
		for(ll=0;ll<NLOCKS;ll++)
		{
			if(prio_inv_locks[ll] == 1)
			{
				update_priority(ll);
				prio_inv_locks[ll] = 0;
			}
		}
		for(ll=0;ll<NLOCKS;ll++)
		{
			if(prio_inv_locks[ll] == 1)
			{
				stop_flag = 0;
				break;
			}
			stop_flag = 1;
		}
	}

	int l;
	struct lockstruct *lptr;
	for(l=0;l<NLOCKS;l++)
	{
		lptr = &locks[l];
		//you are holding this lock!
		if(lptr->holders[pid] != 0)
		{
			resched_waiters(l);	
		}
	}
	
	//and also do the recalculation of priority for the holder.
		

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}


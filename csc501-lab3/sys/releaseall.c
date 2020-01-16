#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <sleep.h>
#include "lock.h"

int my_lock_type = -1;
int releaseall(int numlocks, int args, ...) {
	////kprintf("here\n");
	STATWORD PS;
	disable(PS);
	int i;
	int ret = OK;
	int present;
	struct pentry *pptr;
	struct lockstruct *lptr; 
	pptr = &proctab[currpid];
	unsigned long *a = (unsigned long)&args;
	////kprintf("the value of args is :%d\n",args);
	//run through all the arguments(lock descriptors), and release them 
	for(i=0; i<numlocks; i++)
	{
		//check if the process holds this lock before releasing the lock
		int lid = *a++;
		////kprintf("the lockid : %d, that is to be release for pid : %d\n",lid, currpid);
		lptr = &locks[lid];
		if(lid < 0 || lid >= 50)
		{
			////kprintf("failed the sanity check\n");
			restore(PS);
			//return SYSERR;
		}
		//check this
		////kprintf("the locksheld[%d] = %d\n",lid,pptr->locksheld[lid]);
		if(pptr->locksheld[lid] != -1)
		{
			my_lock_type = pptr->locksheld[lid];
			lptr->holders[currpid] = 0;
			pptr->locksheld[lid] = -1;
			present = checklist(lid);
			////kprintf("the present has returned: %d\n",present);
			//if present is 1, then it means one or more processes is waiting in locklist for this lock, so release them.
			if(present == 0)
			{
				ret = resched_waiters(lid);
				////kprintf("outside resched_waiters\n");
			}
		}
		else
		{
			ret = SYSERR;
		}
	}
	////kprintf("exiting the releaseall function with ret:%d\n",ret);
	//if (ret != SYSERR)
	update_priority(currpid);
	resched();
	restore(PS);
	////kprintf("restored PS\n");
	return ret;
}


int checklist(int lid)
{
	struct lockstruct *lptr;
	lptr = &locks[lid];
	int j;
	int present = 0;
	for(j=0;j<NLOCKS;j++)
	{
		if(lptr->holders[j] == 1)
		{
			////kprintf("the pid %d is also holding this lock\n",j);
			present = 1;
			break;
		}
	}
	return present;
}

void printsq(int head, int tail)
{
	int temp;
	temp = head;
	kprintf("\n\n--------------printing the elements of the queue-------------------------------\n");
	while(temp!=tail)
	{
		kprintf("%d,",q[temp].qnext);
		temp = q[temp].qnext;
	}
	kprintf("\n");
}

void printredq()
{
	int temp;
	temp = rdyhead;
	kprintf("\n\n--------------printing the elements of the ready queue-------------------------------\n");
	while(temp!=rdytail)
	{
		kprintf("%d,",q[temp].qnext);
		temp = q[temp].qnext;
	}
	kprintf("\n");
}


int check_multiple(int lid)
{
		
	struct pentry *pptr,*ppt;
	struct lockstruct *lptr; 
	int hpid;
	lptr = &locks[lid];
	int head = lptr->lqhead;
	int tail = lptr->lqtail;
	int read_cnt = 0;
	int write_cnt = 0;
	//printsq(head,tail);
	int highest_pid = checklast(tail);
	int highest_prio = q[highest_pid].qkey;
	////kprintf("the process : %d with priority %d has the highest prio\n",highest_pid, highest_prio);
	pptr = &proctab[highest_pid];
	int highest_wait = pptr->waittime;
	int flag = 0;
	int temp = head;
	//the logic here is
	//Only when the current lock type is WRITE, we will have this situation where a WRITE is waiting behind a read
	//so in that case, if that READ and WRITE have a delta_time diff of less than 1,then preference should be given to
	//the WRITE and not READ(but the READ is in the top of the queue(tail)), so I am just dequeueing the WRITE and putting him
	//in the top(tail end) of the queue, so the WRITE is picked up.
	if(my_lock_type == WRITE)
	{
		flag = 1;
		if(proctab[highest_pid].locksheld[lid] == READ)
		{
			temp = head;
			while(temp!=tail)
			{
				if(temp!=highest_pid && q[temp].qkey == highest_prio)
				{
					ppt = &proctab[temp];
					if(ppt->locksheld[lid] == WRITE)
					{
						if(((ctr1000 - highest_wait) - (ctr1000 - ppt->waittime)) < 1)
						{
							////kprintf("here\n");
							dequeue(temp);
							enqueue(temp,tail);
						}
					}
				}
			temp = q[temp].qnext;
			}
		}
							
	}
	//while(temp!=tail)
	//{
	//	if(q[temp].qkey == highest_prio)
	//	{
	//		////kprintf("the process %d with lock %d has the same priority\n",temp,lid);
	//		ppt = &proctab[temp];
	//		if(ppt->locksheld[lid] == 1)
	//		{
	//			write_cnt++;
	//		}
	//		else if(ppt->locksheld[lid] == 0)
	//		{
	//			read_cnt++;
	//		}
	//	}
	//	temp = q[temp].qnext;
	//}
	////kprintf("the number of reader locks with same prio is:%d\n",read_cnt++);
	////kprintf("the number of writer locks with same prio is:%d\n",write_cnt++);
	
	
}

int resched_waiters(int lid)
{ 
	//STATWORD PS;
	//disable(PS);
	////kprintf("Inside resched waiters lock ID:%d\n",lid);
	check_multiple(lid);
	struct pentry *pptr;
	struct lockstruct *lptr; 
	int hpid;
	lptr = &locks[lid];
	int head = lptr->lqhead;
	int tail = lptr->lqtail;
	//printsq(head,tail);
	//sanity check
	if(isempty(head))
	{
		////kprintf("the list seems to be empty!\n");
		//printredq();
		//restore(PS);
		return OK;
	}
	
	//now get the last guy from the list and check if he actually has the lock. if he does, then check for reader and writer conditions.
	hpid = getlast(tail);
	pptr = &proctab[hpid];
	if(pptr->locksheld[lid] == -1)
	{
		////kprintf("something is wrong!, this process should have held this lock\n");
		//restore(PS);
		return SYSERR;
	}

	//if i find that a lock has been deleted, then the process that was waiting for that lock must be woken up
	//the logic of DELETED is handled in ldelete and lock, so here we can just wake it up, so it will be able to
	//acquire a new lock or something if it wants.	
	if(pptr->locksheld[lid] == DELETED)
	{
		////kprintf("this process is waking up a waited process for a lock that has been deleted\n");
		pptr->pstate = PRREADY;
		insert(hpid,rdyhead,pptr->pprio);
		return DELETED;	
	}
	
	//if the locksheld has a value 1, then it means it is a WRITE lock.	
	if(pptr->locksheld[lid] == WRITE)
	{
		////kprintf("the WRITE process:%d was waiting for the lock, so putting it in PRREADY and pushing it in the readylist\n",hpid);
		pptr->pstate = PRREADY;
		lptr->holders[hpid] = 1;
		insert(hpid,rdyhead,pptr->pprio);
		//resched();
		//restore(PS);
		return OK;
	}
	//if the locksheld has a value 0, it means it is a READ lock
	//so run through the list and release all the READ locks 
	//until you hit the tail or until you hit another WRITE lock requester.
	else if(pptr->locksheld[lid] == READ)
	{
		////kprintf("the READ process:%d was waiting for the lock, so putting it in PRREADY and pushing it in the readylist\n",hpid);
		//pptr->pstate = PRREADY;
		//lptr->holders[hpid] = 1;
		//insert(hpid,rdyhead,pptr->pprio);
		//while(hpid = getlast(tail) != -1)
		while(hpid != -1)
		{
			////kprintf("we are checking for the process :%d\n",hpid);
			pptr = &proctab[hpid];
			if(pptr->locksheld[lid] == -1)
			{
				////kprintf("!!!something is wrong!, this process should have held this lock\n");
			}
			if(pptr->locksheld[lid] == 1)
			{
				////kprintf("DEBUG: lptr->lqhead: %d\n",head);
				insert(hpid,head,pptr->lock_prio);
				////kprintf("the lock waitlist has a WRITE requester %d, prio: %d, so stopping this and pushing him back into the locklist queue\n",hpid,pptr->lock_prio);
				//printsq(head,tail);
				break;
			}
			else if(pptr->locksheld[lid] == 0)
			{
				//by doing a getlast(tail)-> we remove that process from the locklist, since it is going to be the readylist now
				//hpid = getlast(tail);
				////kprintf("the READ process:%d was waiting for the lock, so putting it in PRREADY and pushing it in the readylist\n",hpid);
				pptr->pstate = PRREADY;
				lptr->holders[hpid] = 1;
				insert(hpid,rdyhead,pptr->pprio);
			}
			hpid = getlast(tail);
		}
		//resched();
		//restore(PS);
		return OK;	
	}
}


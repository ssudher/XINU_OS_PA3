#include <conf.h>
#include <q.h>
#include <kernel.h>
#include <stdio.h>
#include <proc.h>
#include "lock.h"

int checkhigherprio(int , int, int, int);
int checklast(int);
int prio_inv_locks[50];
int prio_inv_procs[50]; 
int lock(int lid,int type,int prio)
{
	STATWORD PS;
	disable(PS);
	struct lockstruct *lptr;
	struct pentry *pptr;
	pptr = &proctab[currpid];
	pptr->lock_prio = prio;
	lptr = &locks[lid];
	////kprintf("the lock %d type is: %d with status: %d\n",lid,lptr->ltype,lptr->lstate);
	if(lid<0 || lid>= 50 || lptr->ltype == DELETED)
	{
		////kprintf("returning SYSERR for the lock request!\n");
		restore(PS);
		return SYSERR;
	}
	
	if(lptr->created_time > pptr->reference_time && currpid!=lptr->my_creator)
	{
		////kprintf("The lock was created after the process was created, so this is not the same lock it was supposed to acquire\n");
		restore(PS);
		return SYSERR;
	}

	////kprintf("currpid: %d inside Lock with lid: %d with lstate: %d\n",currpid, lid,lptr->lstate);
	
	//printsq(lptr->lqhead, lptr->lqtail);
	int pp,free = 1;
	for(pp=0;pp<NPROC;pp++)
	{
		if(lptr->holders[pp] == 1)
		{
			////kprintf("!!!!!!!This is the list of procs waiting for the lock when freeness is checked %d!!!!!!!\n",pp);
			free = 0;
			break;
		}
	}
	
	if(free == 1)
	{
		////kprintf("the lock is free\n");
		lptr->lstate = LUSED;
		lptr->ltype = type;
		lptr->holders[currpid] = 1;
		pptr->locksheld[lid] = type;
		pptr->waittime = 0;
		restore(PS);
		return OK;
	}
	else if(lptr->lstate != LFREE)
	{
		if(lptr->ltype == READ)
		{
			int status = checkhigherprio(lptr->lqhead, lptr->lqtail,currpid, prio);
			//if status is 0, then there is no process in the lock list that has more priority than this current process's priority for the lock
			if(status == 0)
			{
				////kprintf("inside the READ lock type\n");
				if(type == READ)
				{
						
					////kprintf("inside the READ condition\n");
					lptr->holders[currpid] = 1;
					lptr->lstate = LUSED;
					lptr->ltype = type;
					pptr->locksheld[lid] = type;
					//check this.
					//pptr->pstate = PRWAIT;
					pptr->waittime = ctr1000;
					restore(PS);
					return OK;
				}
				else if(type == WRITE)
				{
					int ret = OK;
					////kprintf("inside the WRITE condition\n");
					lptr->lstate = LUSED;
					//lptr->ltype = type;
					pptr->locksheld[lid] = type;
					pptr->pstate = PRWAIT;
					////kprintf("putting this process: %d to PRWAIT state\n",currpid);
					pptr->waittime = ctr1000;
					pptr->waiting_lock = lid;
					//dequeue(currpid);
					insert(currpid, lptr->lqhead, prio);
					check_prior_inversion(pptr->pprio, currpid, lid);
					resched();
					//check if the proctab->locksheld has been set to DELETED. if so then return SYSERR.
					////kprintf("------------------woken up finally!!!%d-----------------------\n",currpid);
					if(pptr->locksheld[lid] == DELETED)
					{
						////kprintf("!!!Some other process deleted this lock before process %d got a chance to acquire it!!!\n",currpid);
						restore(PS);
						return DELETED;
					}
					lptr->ltype = type;
					lptr->holders[currpid] = 1;	
					pptr = &proctab[currpid];
					check_prior_inversion(pptr->pprio, currpid, lid);
					restore(PS);
					return ret;
				}	
			}	
			else
			{
				if(type == READ)
				{
					int ret = OK;
					////kprintf("inside the lesser priority condition for READ\n");
					//lptr->holders[currpid] = 1;
					pptr->pstate = PRWAIT;
					////kprintf("putting this process: %d to PRWAIT state\n",currpid);
					pptr->waittime = ctr1000;
					pptr->locksheld[lid] = type;
					pptr->waiting_lock = lid;
					insert(currpid, lptr->lqhead, prio);
					check_prior_inversion(pptr->pprio, currpid, lid);
					resched();
					////kprintf("------------------woken up finally!!!%d-----------------------\n",currpid);
					if(pptr->locksheld[lid] == DELETED)
					{
						////kprintf("!!!Some other process deleted this lock before process %d got a chance to acquire it!!!\n",currpid);
						restore(PS);
						return DELETED;
					}
					lptr->ltype = type;
					lptr->holders[currpid] = 1;	
					pptr = &proctab[currpid];
					check_prior_inversion(pptr->pprio, currpid, lid);
					restore(PS);
					return ret;
				}
				else if(type == WRITE)
				{
					int ret = OK;
					////kprintf("inside the lesser priority condition for WRITE\n");
					//lptr->holders[currpid] = 1;
					pptr->pstate = PRWAIT;
					////kprintf("putting this process: %d to PRWAIT state\n",currpid);
					pptr->waittime = ctr1000;
					pptr->locksheld[lid] = type;
					pptr->waiting_lock = lid;
					////kprintf("DEBUG: lptr->lqhead: %d\n",lptr->lqhead);
					insert(currpid, lptr->lqhead, prio);
					check_prior_inversion(pptr->pprio, currpid, lid);
					resched();
					////kprintf("------------------woken up finally!!!%d-----------------------\n",currpid);
					if(pptr->locksheld[lid] == DELETED)
					{
						////kprintf("!!!Some other process deleted this lock before process %d got a chance to acquire it!!!\n",currpid);
						restore(PS);
						return DELETED;
					}
					lptr->ltype = type;
					lptr->holders[currpid] = 1;	
					pptr = &proctab[currpid];
					check_prior_inversion(pptr->pprio, currpid, lid);
					restore(PS);
					return ret;	
				}	
			}
		}
		else if(lptr->ltype == WRITE)
		{
			int ret = OK;
			////kprintf("inside the WRITE type lock requested logic: %d\n",lid);
			pptr->pstate = PRWAIT;
			pptr->waittime = ctr1000;
			pptr->locksheld[lid] = type;
			pptr->waiting_lock = lid;
			insert(currpid, lptr->lqhead, prio);
			//kprintf("queuing a READ proc %d for WRITE lock %d\n",currpid,prio);	
			check_prior_inversion(pptr->pprio, currpid, lid);
			//printsq(lptr->lqhead,lptr->lqtail);
			resched();
			////kprintf("------------------woken up finally!!!%d-----------------------\n",currpid);
			if(pptr->locksheld[lid] == DELETED)
			{
				////kprintf("!!!Some other process deleted this lock before process %d got a chance to acquire it!!!\n",currpid);
				restore(PS);
				return DELETED;
			}
			lptr->ltype = type;
			lptr->holders[currpid] = 1;
			pptr = &proctab[currpid];
			check_prior_inversion(pptr->pprio, currpid, lid);
			restore(PS);
			return ret;
		}		
	}
}	


int checkhigherprio(int head,int tail, int pid, int currprio)
{
	int hpid;
	int hprio = q[hpid = checklast(tail)].qkey;
	////kprintf("the process with high prio: %d inside lock list is: %d\n",q[hpid].qkey,hpid);
	////kprintf("comparing my prio :%d, with list prio:%d\n",currprio,hprio);
	if(hprio > currprio)
	{
		////kprintf("returning 0 from checkhigherprio\n");
		return 1;
	}
	return 0;
}

int return_lock_holder(int lid)
{
	int p;
	struct lockstruct *lptr;
	lptr = &locks[lid];
	for(p=0;p<NPROC;p++)
	{
		if(lptr->holders[p] == 1)
		{
			////kprintf("the lock is held by process %d\n",p);
			return p;
		}
	}
}

int check_prior_inversion(int org_prio, int pid, int lid)
{
	struct pentry *new_pptr, *old_pptr;
	new_pptr = &proctab[pid];
	//kprintf("updating prio for lock %d\n",lid);
	update_priority(lid);
	//int owner = return_lock_holder(lid);
	//old_pptr = &proctab[owner];
	//if(new_pptr->pprio > old_pptr->pprio)
	//{
	//	if(old_pptr->pinh == 0)
	//		old_pptr->pinh = old_pptr->pprio;
	//	old_pptr->pprio = new_pptr->pprio;
	//	if(
	//	//chprio(pid,new_pptr->pprio);
	//}
}

int checklast(int tail)
{
	int proc;
	if ((proc=q[tail].qprev) < NPROC)
	{
		//kprintf("returning proc: %d with prio %d\n",proc,q[proc].qkey);
		return(proc);
	}
	else
		return(EMPTY);
}

int max_proc_prio(int lid)
{
	struct lockstruct *lptr;
	lptr = &locks[lid];
	int temp;
	int local_max = MININT;
	temp = 	lptr->lqhead;
	//kprintf("check tail %d and head %d\n",proctab[lptr->lqtail].pprio,proctab[lptr->lqhead].pprio);
	while(temp!=lptr->lqtail)
	{
		if(proctab[temp].pprio > local_max)
		{
			local_max = proctab[temp].pprio;
		}
		temp = q[temp].qnext;
	}
	return local_max;
}

//loop through all the locks for a single process and find the max
//priority for that process and update it
int find_max_prio(int pid)
{
	struct pentry *pptr;
	struct lockstruct *lptr;
	pptr = &proctab[pid];
	int l,temp_max;
	int max = MININT;
	for(l=0;l<NLOCKS;l++)
	{
		if((pptr->locksheld[l] != pptr->waiting_lock) && (pptr->locksheld[l] != -1 && pptr->locksheld[l] != DELETED))
		{
			////kprintf("Check %d :: %d\n",pptr->locksheld[l],pptr->waiting_lock);
			////kprintf("find_max_prio -- this lock is being held %d\n",l);
			lptr = &locks[l];
			//kprintf("printing the wait queue!!!\n");
			//printsq(lptr->lqhead, lptr->lqtail);
			temp_max = max_proc_prio(l);
			//kprintf("the temp_max prio is : %d\n",temp_max);
			if(temp_max != 0)
			{
				if(temp_max>max)
				{
					//kprintf("updating the max value to %d\n",temp_max);
					max = temp_max;
				}
			}
		}
	}
	return max;
}

//loop through all the processes that are holding a lock 
int update_priority(int lid)
{
	struct pentry *pptr;
	struct lockstruct *lptr;
	lptr = &locks[lid];
	int h;
	int max;
	
	//this for loop will update the priority of all the procs that was holding lid.
	for(h=0;h<NPROC;h++)
	{
		if(lptr->holders[h] == 1)
		{
			////kprintf("UP -- this process holds this lock: %d\n",h);
			if(proctab[h].waiting_lock!=-1)
			{
				prio_inv_locks[proctab[h].waiting_lock] = 1;
				prio_inv_procs[h] = 1;
			}
			pptr = &proctab[h];
			max = find_max_prio(h);
			if(max == MININT)
			{
				if(pptr->pinh != 0)
				{
					////kprintf("check this pinh %d\n",pptr->pinh);
					pptr->pprio = pptr->pinh;
					//chprio(h,pptr->pinh);
				}
				else
				{
					pptr->pinh = pptr->pprio;
				}
			}
			else
			{
				if(pptr->pinh != 0)
				{
					//kprintf("not my first time\n");
					pptr->pprio = max;
					//chprio(h,max);
				}
				else
				{
					//kprintf("firsttime da\n");
					pptr->pinh = pptr->pprio;
					pptr->pprio = max;
				}
			}
		}
	}
}

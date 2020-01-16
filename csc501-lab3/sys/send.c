/* send.c - send */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  send  --  send a message to another process
 *------------------------------------------------------------------------
 */
SYSCALL	send(int pid, WORD msg)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	//kprintf("inside SEND?\n");

	if (isbadpid(pid) || ( (pptr= &proctab[pid])->pstate == PRFREE)
	   || pptr->phasmsg != 0) {
		//kprintf("%d %d\n",pid,pptr->pstate);
		//if(pptr->phasmsg != 0)
		//	kprintf("err\n");
		restore(ps);
		return(SYSERR);
	}
	
	
	//to check if the issue is because of interleaved send()
	/*
	if(isbadpid(pid))
	{
		restore(ps);
		return SYSERR;
	}
	pptr = &proctab[pid];
	while(pptr->phasmsg)
	{
		//kprintf("spin\n");
		sleep1000(QUANTUM);
		//ready(currpid, RESCHYES);
	}
	*/

	pptr->pmsg = msg;
	pptr->phasmsg = TRUE;
	if (pptr->pstate == PRRECV)	/* if receiver waits, start it	*/
		ready(pid, RESCHYES);
	else if (pptr->pstate == PRTRECV) {
		unsleep(pid);
		ready(pid, RESCHYES);
	}
	restore(ps);
	return(OK);
}

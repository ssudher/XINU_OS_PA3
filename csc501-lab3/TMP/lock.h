#ifndef _LOCK_H_
#define _LOCK_H_

#define NLOCKS 50
#define LFREE 0
#define LUSED 1
#define READ 0
#define WRITE 1


struct lockstruct{

	int lstate; //FREE - 0, USED - 1
	int ltype;  //READ - 0, WRITE - 1
	int lqhead;
	int lqtail;
	int holders[NPROC];
	int my_creator;
	unsigned long created_time;
};

extern void linit();
extern int nextlock;
extern struct lockstruct locks[NLOCKS];
extern unsigned long ctr1000;
extern int checklast(int);
extern int resched_waiters(int);
extern void printsq(int, int);
extern void printredq();
extern int update_priority(int);
extern int prio_inv_locks[50];
extern int prio_inv_procs[50];
#endif

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <stdio.h>
#include "lock.h"


void semaphore(char *msg,int sema){
        kprintf("%s is about to acquire the semaphore\n",msg);
        int x = wait(sema);
        sleep(2);
        kprintf("%s is about to release the semaphore semaphore\n",msg);
        signal(sema);
	kprintf("%s released the semaphore\n",msg);
}

void semaphore2(char *msg,int sema){
	kprintf("%s is executing\n",msg);
	int iter;
	int sum = 0;
	for(iter=0;iter<20;iter++)
	{
		sum += (iter*(sum+iter));
	}
	sleep(2);
	kprintf("%s has calculated : %d\n",msg,sum);
	kprintf("%s is exiting its execution\n",msg);
}

void lock1(char *msg,int lck){
        kprintf("%s is about to acquire the lock\n",msg);
        int x = lock(lck,WRITE,25);
        kprintf("%s: Lock acquired.\n",msg);
        sleep(1);
        kprintf("%s is about to Release the lock.\n",msg);
        releaseall(1,lck);
	kprintf("%s released the lock\n",msg);
}

void lock2(char *msg,int lck){
	kprintf("%s is executing\n",msg);
	int iter;
	int sum = 0;
	for(iter=0;iter<20;iter++)
	{
		sum += (iter*(sum+iter));
	}
	sleep(4);
	kprintf("%s has calculated : %d\n",msg,sum);
	kprintf("%s is exiting its execution\n",msg);
}

int main(){

        kprintf("\n-----------------------------------------\n");
        kprintf("Output for priority inversion problem with Semaphores\n\n");
        int sem = screate(1);
        int sem_low = create(semaphore,2000,20,"A",1,"low_prio proc",sem_low);
        int sem_med = create(semaphore2,2000,22,"B",1,"med_prio proc",sem_med);
        int sem_high = create(semaphore,2000,28,"C",1,"high_prio proc",sem_high);

        resume(sem_low);
        resume(sem_med);
        sleep(2);
        resume(sem_high);
        sleep(10);

        kprintf("\n-----------------------------------------\n");
        kprintf("Priority inversion test case with LOCKS(priority_inheritance)\n\n");

        int lck = lcreate();
        int lock_low = create(lock1,2000,20,"A",1,"low_prio proc",lck);
        int lock_med = create(lock2,2000,22,"B",1,"med_prio proc",lck);
        int lock_high = create(lock1,2000,28,"C",1,"high_prio proc",lck);
        resume(lock_low);
        resume(lock_med);
        sleep(2);
        resume(lock_high);

        sleep(15);
        shutdown();
        return OK;
}

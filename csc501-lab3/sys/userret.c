/* userret.c - userret */

#include <conf.h>
#include <kernel.h>

/*------------------------------------------------------------------------
 * userret  --  entered when a process exits by return
 *------------------------------------------------------------------------
 */
int userret()
{
	//kprintf("user return\n");
	kill( getpid() );
        return(OK);
}

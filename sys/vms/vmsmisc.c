/*	SCCS Id: @(#)vmsmisc.c	3.4	1996/03/02	*/
/* NetHack may be freely redistributed.  See license for details. */

#include <ssdef.h>
#include <stsdef.h>

void vms_exit( /*_ int _*/ );
void vms_abort( /*_ void _*/ );

extern void exit( /*_ int _*/ );
extern void lib$signal( /*_ unsigned long,... _*/ );

void
vms_exit(status)
int status;
{
    exit(status ? (SS$_ABORT | STS$M_INHIB_MSG) : SS$_NORMAL);
}

void
vms_abort()
{
    lib$signal(SS$_DEBUG);
}

#ifdef VERYOLD_VMS
#include "oldcrtl.c"
#endif

/*vmsmisc.c*/

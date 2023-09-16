/* NetHack 3.7	vmsmisc.c	$NHDT-Date: 1685522049 2023/05/31 08:34:09 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.18 $ */
/*      Copyright (c) 2011 by Robert Patrick Rankin              */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#undef exit

#ifdef VMSVSI
#include <descrip.h>
#include <lib$routines.h>
#include <starlet.h>
#endif

#include <ssdef.h>
#include <stsdef.h>

int debuggable = 0; /* 1 if we can debug or show a call trace */

ATTRNORETURN void vms_exit(int);
ATTRNORETURN void vms_abort(void);

/* first arg should be unsigned long but <lib$routines.h> has unsigned int */

#ifndef VMSVSI
extern void lib$signal(unsigned, ...);
#endif

/* terminate, converting Unix-style exit code into VMS status code */
ATTRNORETURN void
vms_exit(int status)
{
    /* convert non-zero to failure, zero to success */
    exit(status ? (SS$_ABORT | STS$M_INHIB_MSG) : SS$_NORMAL);
    /* NOT REACHED */
}

/* put the user into the debugger; used for abort() when in wizard mode */
ATTRNORETURN void
vms_abort(void)
{
    if (debuggable)
        lib$signal(SS$_DEBUG);

    /* we'll get here if the debugger isn't available, or if the user
       uses GO to resume execution instead of EXIT to quit */
    vms_exit(2); /* don't return to caller (2==arbitrary non-zero) */
    /* NOT REACHED */
}

/*
 * Caveat: the VERYOLD_VMS configuration hasn't been tested in many years.
 */
#ifdef VERYOLD_VMS
#include "oldcrtl.c"
#endif

/*vmsmisc.c*/

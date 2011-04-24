/* NetHack 3.5	vmsmisc.c	$Date$  $Revision$ */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#undef exit
#include <ssdef.h>
#include <stsdef.h>

void FDECL(vms_exit, (int));
void NDECL(vms_abort);

extern int FDECL(vms_define, (const char *,const char *,int));

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

#ifdef PANICTRACE
void
vms_traceback(how)
int how;	/* 1: exit after traceback; 2: stay in debugger */
{
    /* signal handler expects first byte to hold length of the rest */
    char dbgcmd[1+255];

    dbgcmd[0] = dbgcmd[1] = '\0';
    if (how == 2) {
	/* limit output to 18 stack frames to avoid longer output causing
	   nethack's panic prolog from scrolling off conventional sized
	   screen; perhaps we should adapt to termcap LI here... */
	(void)strcpy(dbgcmd, "#set Module/Calls; show Calls 18");
    } else if (how == 1) {
	/*
	 * Suppress most of debugger's initial feedback to avoid scaring users.
	 */
	/* start up with output going to /dev/null instead of stdout */
	(void)vms_define("DBG$OUTPUT", "_NL:", 0);
	/* bypass any debugger initialization file the user might have */
	(void)vms_define("DBG$INIT", "_NL:", 0);
	/* force tty interface by suppressing DECwindows/Motif interface */
	(void)vms_define("DBG$DECW$DISPLAY", " ", 0);
	/* once started, send output to log file on stdout */
	(void)strcpy(dbgcmd, "#set Log SYS$OUTPUT:; set output Log,noTerminal");
	/* FIXME: the trailing exit command here is actually being ignored,
	   leaving us at the DBG> prompt contrary to our intent... */
	(void)strcat(dbgcmd, "; set Module/Calls; show Calls 18; exit");
    }
    
    if (dbgcmd[1]) {
	/* plug in command's length; debugger's signal handler expects ASCIC
	   counted string rather than C-style ASCIZ 0-terminated string */
	dbgcmd[0] = (char)strlen(&dbgcmd[1]);
	/*
	 * This won't work if we've been linked /noTraceback, and
	 * we have to link /noTraceback if nethack.exe is going
	 * to be installed with privileges, so this is of dubious
	 * value for a SECURE multi-user playground installation.
	 *
	 * TODO: What's worse, we need to add a condition handler
	 * to trap the resulting "improperly handled condition"
	 * and the annoying and/or frightening (and in this case,
	 * useless) register dump given when the debugger can't be
	 * activated for a noTraceback executable.
	 */
	(void)lib$signal(SS$_DEBUG, 1, dbgcmd);
    }

    vms_exit(2);	/* don't return to caller */
    /* NOT REACHED */
}
#endif

#ifdef VERYOLD_VMS
#include "oldcrtl.c"
#endif

/*vmsmisc.c*/

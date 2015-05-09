/* NetHack 3.6	sys.c	$NHDT-Date: 1431192764 2015/05/09 17:32:44 $  $NHDT-Branch: master $:$NHDT-Revision: 1.32 $ */
/* NetHack 3.6	sys.c	$Date: 2012/03/10 02:22:07 $  $Revision: 1.12 $ */
/* Copyright (c) Kenneth Lorber, Kensington, Maryland, 2008. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifndef SYSCF
/* !SYSCF configurations need '#define DEBUGFILES "foo.c bar.c"'
   to enable debugging feedback for source files foo.c and bar.c;
   to activate debugpline(), set an appropriate value and uncomment */
/* # define DEBUGFILES "*" */
/* note: DEBUGFILES value here or in sysconf.DEBUGFILES can be overridden
   at runtime by setting up a value for "DEBUGFILES" in the environment */
#endif

struct sysopt sysopt;

void
sys_early_init()
{
    sysopt.support = NULL;
    sysopt.recover = NULL;
#ifdef SYSCF
    sysopt.wizards = NULL;
#else
    sysopt.wizards = dupstr(WIZARD_NAME);
#endif
#if defined(SYSCF) || !defined(DEBUGFILES)
    sysopt.debugfiles = NULL;
#else
    sysopt.debugfiles = dupstr(DEBUGFILES);
#endif
    sysopt.env_dbgfl = 0; /* haven't checked getenv("DEBUGFILES") yet */
    sysopt.shellers = NULL;
    sysopt.explorers = NULL;
    sysopt.maxplayers = 0; /* XXX eventually replace MAX_NR_OF_PLAYERS */

    /* record file */
    sysopt.persmax = PERSMAX;
    sysopt.entrymax = ENTRYMAX;
    sysopt.pointsmin = POINTSMIN;
    sysopt.pers_is_uid = PERS_IS_UID;
    sysopt.tt_oname_maxrank = 10;

    /* sanity checks */
    if (PERSMAX < 1)
        sysopt.persmax = 1;
    if (ENTRYMAX < 10)
        sysopt.entrymax = 10;
    if (POINTSMIN < 1)
        sysopt.pointsmin = 1;
    if (PERS_IS_UID != 0 && PERS_IS_UID != 1)
        panic("config error: PERS_IS_UID must be either 0 or 1");

#ifdef PANICTRACE
    /* panic options */
    sysopt.gdbpath = dupstr(GDBPATH);
    sysopt.greppath = dupstr(GREPPATH);
#ifdef BETA
    sysopt.panictrace_gdb = 1;
#ifdef PANICTRACE_LIBC
    sysopt.panictrace_libc = 2;
#endif
#else
    sysopt.panictrace_gdb = 0;
#ifdef PANICTRACE_LIBC
    sysopt.panictrace_libc = 0;
#endif
#endif
#endif

    sysopt.check_save_uid = 1;
    sysopt.seduce = 1; /* if it's compiled in, default to on */
    sysopt_seduce_set(sysopt.seduce);
}

void
sysopt_release()
{
    if (sysopt.support)
        free(sysopt.support), sysopt.support = NULL;
    if (sysopt.recover)
        free(sysopt.recover), sysopt.recover = NULL;
    if (sysopt.wizards)
        free(sysopt.wizards), sysopt.wizards = NULL;
    if (sysopt.debugfiles)
        free(sysopt.debugfiles), sysopt.debugfiles = NULL;
#ifdef PANICTRACE
    if (sysopt.gdbpath)
        free(sysopt.gdbpath), sysopt.gdbpath = NULL;
    if (sysopt.greppath)
        free(sysopt.greppath), sysopt.greppath = NULL;
#endif
}

extern struct attack sa_yes[NATTK];
extern struct attack sa_no[NATTK];

void
sysopt_seduce_set(val)
int val;
{
    struct attack *setval = val ? sa_yes : sa_no;
    int x;
    for (x = 0; x < NATTK; x++) {
        mons[PM_INCUBUS].mattk[x] = setval[x];
        mons[PM_SUCCUBUS].mattk[x] = setval[x];
    }
}

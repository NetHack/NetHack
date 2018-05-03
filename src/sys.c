/* NetHack 3.6	sys.c	$NHDT-Date: 1448241785 2015/11/23 01:23:05 $  $NHDT-Branch: master $:$NHDT-Revision: 1.35 $ */
/* Copyright (c) Kenneth Lorber, Kensington, Maryland, 2008. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifndef SYSCF
/* !SYSCF configurations need '#define DEBUGFILES "foo.c bar.c"'
 * to enable debugging feedback for source files foo.c and bar.c;
 * to activate debugpline(), set an appropriate value and uncomment
 */
/* # define DEBUGFILES "*" */

/* note: DEBUGFILES value here or in sysconf.DEBUGFILES can be overridden
   at runtime by setting up a value for "DEBUGFILES" in the environment */
#endif

struct sysopt sysopt;

void
sys_early_init()
{
    sysopt.support = (char *) 0;
    sysopt.recover = (char *) 0;
#ifdef SYSCF
    sysopt.wizards = (char *) 0;
#else
    sysopt.wizards = dupstr(WIZARD_NAME);
#endif
#if defined(SYSCF) || !defined(DEBUGFILES)
    sysopt.debugfiles = (char *) 0;
#else
    sysopt.debugfiles = dupstr(DEBUGFILES);
#endif
#ifdef DUMPLOG
    sysopt.dumplogfile = (char *) 0;
#endif
    sysopt.env_dbgfl = 0; /* haven't checked getenv("DEBUGFILES") yet */
    sysopt.shellers = (char *) 0;
    sysopt.explorers = (char *) 0;
    sysopt.genericusers = (char *) 0;
    sysopt.maxplayers = 0; /* XXX eventually replace MAX_NR_OF_PLAYERS */
    sysopt.bones_pools = 0;

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
    sysopt.check_plname = 0;
    sysopt.seduce = 1; /* if it's compiled in, default to on */
    sysopt_seduce_set(sysopt.seduce);
    return;
}

void
sysopt_release()
{
    if (sysopt.support) {
        free((genericptr_t) sysopt.support); sysopt.support = (char *) 0;
    }
    if (sysopt.recover) {
        free((genericptr_t) sysopt.recover); sysopt.recover = (char *) 0;
    }
    if (sysopt.wizards) {
        free((genericptr_t) sysopt.wizards); sysopt.wizards = (char *) 0;
    }
    if (sysopt.explorers) {
        free((genericptr_t) sysopt.explorers); sysopt.explorers = (char *) 0;
    }
    if (sysopt.shellers) {
        free((genericptr_t) sysopt.shellers); sysopt.shellers = (char *) 0;
    }
    if (sysopt.debugfiles) {
        free((genericptr_t) sysopt.debugfiles);
        sysopt.debugfiles = (char *) 0;
    }
#ifdef DUMPLOG
    if (sysopt.dumplogfile) {
        free((genericptr_t)sysopt.dumplogfile); sysopt.dumplogfile=(char *)0;
    }
#endif
    if (sysopt.genericusers) {
        free((genericptr_t) sysopt.genericusers);
        sysopt.genericusers = (char *) 0;
    }
#ifdef PANICTRACE
    if (sysopt.gdbpath) {
        free((genericptr_t) sysopt.gdbpath); sysopt.gdbpath = (char *) 0;
    }
    if (sysopt.greppath) {
        free((genericptr_t) sysopt.greppath); sysopt.greppath = (char *) 0;
    }
#endif
    /* this one's last because it might be used in panic feedback, although
       none of the preceding ones are likely to trigger a controlled panic */
    if (sysopt.fmtd_wizard_list) {
        free((genericptr_t) sysopt.fmtd_wizard_list);
        sysopt.fmtd_wizard_list = (char *) 0;
    }
    return;
}

extern struct attack sa_yes[NATTK];
extern struct attack sa_no[NATTK];

void
sysopt_seduce_set(int val)
{
    struct attack *setval = val ? sa_yes : sa_no;
    int x;

    for (x = 0; x < NATTK; x++) {
        mons[PM_INCUBUS].mattk[x] = setval[x];
        mons[PM_SUCCUBUS].mattk[x] = setval[x];
    }
    return;
}

/*sys.c*/

/* NetHack 3.5	sys.c	$Date$  $Revision$ */
/* Copyright (c) Kenneth Lorber, Kensington, Maryland, 2008. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* for KR1ED config, WIZARD is 0 or 1 and WIZARD_NAME is a string;
   for usual config, WIZARD is the string; forcing WIZARD_NAME to match it
   eliminates conditional testing for which one to use in string ops */
#ifndef KR1ED
# undef WIZARD_NAME
# define WIZARD_NAME WIZARD
#endif

struct sysopt sysopt;

void
sys_early_init(){
	sysopt.support = NULL;
	sysopt.recover = NULL;
#ifdef SYSCF
	sysopt.wizards = NULL;
#else
	sysopt.wizards = WIZARD_NAME;
#endif
	sysopt.shellers = NULL;
	sysopt.maxplayers = 0;	/* XXX eventually replace MAX_NR_OF_PLAYERS */

		/* record file */
	sysopt.persmax = PERSMAX;
	sysopt.entrymax = ENTRYMAX;
	sysopt.pointsmin = POINTSMIN;
	sysopt.pers_is_uid = PERS_IS_UID;

		/* sanity checks */
	if(PERSMAX<1) sysopt.persmax = 1;
	if(ENTRYMAX<10) sysopt.entrymax = 10;
	if(POINTSMIN<1) sysopt.pointsmin = 1;
	if(PERS_IS_UID != 0 && PERS_IS_UID != 1)
		panic("config error: PERS_IS_UID must be either 0 or 1");

#ifdef PANICTRACE
		/* panic options */
	sysopt.gdbpath = dupstr(GDBPATH);
	sysopt.greppath = dupstr(GREPPATH);
# ifdef BETA
	sysopt.panictrace_gdb = 1;
#  ifdef PANICTRACE_LIBC
	sysopt.panictrace_libc = 2;
#  endif
# else
	sysopt.panictrace_gdb = 0;
#  ifdef PANICTRACE_LIBC
	sysopt.panictrace_libc = 0;
#  endif
# endif
#endif

#ifdef SEDUCE
	sysopt.seduce = 1;	/* if it's compiled in, default to on */
	sysopt_seduce_set(sysopt.seduce);
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
	for(x=0; x<NATTK; x++){
		mons[PM_INCUBUS].mattk[x] = setval[x];
		mons[PM_SUCCUBUS].mattk[x] = setval[x];
	}
}

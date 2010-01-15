/* NetHack 3.5	sys.c	$Date$  $Revision$ */
/*    SCCS Id: @(#)sys.c      3.5     2008/01/30      */
/* Copyright (c) Kenneth Lorber, Kensington, Maryland, 2008. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

struct sysopt sysopt;

void
sys_early_init(){
	sysopt.support = NULL;
	sysopt.recover = NULL;
#ifdef notyet
	/* replace use of WIZARD vs WIZARD_NAME vs KR1ED, by filling this in */
#endif
	sysopt.wizards = NULL;
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
	sysopt.gdbpath = NULL;
# ifdef BETA
	sysopt.panictrace_gdb = 1;
#  ifdef PANICTRACE_GLIBC
	sysopt.panictrace_glibc = 2;
#  endif
# else
	sysopt.panictrace_gdb = 0;
#  ifdef PANICTRACE_GLIBC
	sysopt.panictrace_glibc = 0;
#  endif
# endif
#endif
}


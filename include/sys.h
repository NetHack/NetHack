/* NetHack 3.5	sys.h	$NHDT-Date: 1426544796 2015/03/16 22:26:36 $  $NHDT-Branch: master $:$NHDT-Revision: 1.13 $ */
/* NetHack 3.5	sys.h	$Date: 2012/01/27 20:15:26 $  $Revision: 1.9 $ */
/* Copyright (c) Kenneth Lorber, Kensington, Maryland, 2008. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SYS_H
#define SYS_H

struct sysopt {
	char *support;	/* local support contact */
	char *recover;	/* how to run recover - may be overridden by win port */
	char *wizards;
	char *explorers;
	char *shellers;	/* like wizards, for ! command (-DSHELL) */
	char *debugfiles; /* files to show debugplines in. '*' is all. */
	int env_dbgfl;	/*  1: debugfiles comes from getenv("DEBUGFILES")
			 *     so sysconf's DEBUGFILES shouldn't override it;
			 *  0: getenv() hasn't been attempted yet;
			 * -1: getenv() didn't find a value for DEBUGFILES.
			 */    
	int   maxplayers;
		/* record file */
	int persmax;
	int pers_is_uid;
	int entrymax;
	int pointsmin;
	int tt_oname_maxrank;
#ifdef PANICTRACE
		/* panic options */
	char *gdbpath;
	char *greppath;
	int  panictrace_gdb;
# ifdef PANICTRACE_LIBC
	int panictrace_libc;
# endif
#endif
	int seduce;
};

extern  struct sysopt sysopt;

#define SYSOPT_SEDUCE sysopt.seduce

#endif /* SYS_H */


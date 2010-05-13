/* NetHack 3.5	sys.h	$Date$  $Revision$ */
/* Copyright (c) Kenneth Lorber, Kensington, Maryland, 2008. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SYS_H
#define SYS_H

#define E extern

E void NDECL(sys_early_init);

struct sysopt {
	char *support;	/* local support contact */
	char *recover;	/* how to run recover - may be overridden by win port */
	char *wizards;
	char *shellers;	/* like wizards, for ! command (-DSHELL) */
	int   maxplayers;
		/* record file */
	int persmax;
	int pers_is_uid;
	int entrymax;
	int pointsmin;
#ifdef PANICTRACE
		/* panic options */
	char *gdbpath;
	char *greppath;
	int  panictrace_gdb;
# ifdef PANICTRACE_GLIBC
	int panictrace_glibc;
# endif
#endif
	int seduce;
};
E struct sysopt sysopt;

#ifdef SEDUCE
# define SYSOPT_SEDUCE sysopt.seduce
#else
# define SYSOPT_SEDUCE 0
#endif

#endif /* SYS_H */


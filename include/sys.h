/* NetHack 3.5	sys.h	$Date$  $Revision$ */
/*    SCCS Id: @(#)sys.h      3.5     2008/01/30      */
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
};
E struct sysopt sysopt;

#endif /* SYS_H */


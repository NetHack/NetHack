/* NetHack 3.5	macunix.c	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	macunix.c	$Date: 2009/05/06 10:49:16 $  $Revision: 1.6 $ */
/*	SCCS Id: @(#)macunix.c	3.5	1994/11/07	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* This file collects some Unix dependencies */

#include "hack.h"

void
regularize(char *s)
{
	register char *lp;

	for (lp = s; *lp; lp++) {
		if (*lp == '.' || *lp == ':')
			*lp = '_';
	}
}

void
getlock(void)
{
	int fd;
	int pid = getpid(); /* Process ID */

	Sprintf(lock, "%d%s", getuid(), plname);
	set_levelfile_name (lock, 0);

	if ((fd = open (lock, O_RDWR | O_EXCL | O_CREAT, LEVL_TYPE)) == -1) {
		raw_printf ("Could not lock the game %s.", lock);
		panic ("Another game in progress?");
	}

	if (write (fd, (char *)&pid, sizeof (pid)) != sizeof (pid))  {
		raw_printf ("Could not lock the game %s.", lock);
		panic("Disk locked?");
	}
	close (fd);
}

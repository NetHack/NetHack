/* NetHack 3.6	macunix.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
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

    Sprintf(gl.lock, "%d%s", getuid(), gp.plname);
    set_levelfile_name(gl.lock, 0);

    if ((fd = open(gl.lock, O_RDWR | O_EXCL | O_CREAT, LEVL_TYPE)) == -1) {
        raw_printf("Could not lock the game %s.", gl.lock);
        panic("Another game in progress?");
    }

    if (write(fd, (char *) &pid, sizeof(pid)) != sizeof(pid)) {
        raw_printf("Could not lock the game %s.", gl.lock);
        panic("Disk locked?");
    }
    close(fd);
}

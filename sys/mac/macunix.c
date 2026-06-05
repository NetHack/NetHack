/* NetHack 5.0	macunix.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
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

/* Create the level-0 checkpoint file ("1<plname>.0").  Despite the name
   this is less a lock than the anchor for crash recovery: it records the
   current level and save-file name, and the Recover application reads it to
   rebuild a save after a crash.  If it already exists, a previous game
   crashed (only one copy of the app can run on classic Mac OS) -- refuse to
   start so a new game can't overwrite the still-recoverable level files. */
void
getlock(void)
{
    int fd;
    int pid = getpid(); /* always 1 on classic Mac OS */

    Sprintf(gl.lock, "%d%s", getuid(), svp.plname);
    set_levelfile_name(gl.lock, 0);

    if ((fd = open(gl.lock, O_RDWR | O_EXCL | O_CREAT, LEVL_TYPE)) == -1) {
        raw_printf("There are files from a crashed game (%s).", gl.lock);
        panic("Run the Recover application to salvage it, or delete the "
              "level files to abandon it.");
    }

    if (write(fd, (char *) &pid, sizeof(pid)) != sizeof(pid)) {
        raw_printf("Could not lock the game %s.", gl.lock);
        panic("Disk locked?");
    }
    close(fd);
}

unsigned long
sys_random_seed(void)
{
    unsigned long seed;

    seed = (unsigned long) getnow();
    seed ^= (unsigned long) getpid() << 16;
    return seed;
}

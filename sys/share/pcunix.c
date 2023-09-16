/* NetHack 3.7	pcunix.c	$NHDT-Date: 1596498285 2020/08/03 23:44:45 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.42 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

/* This file collects some Unix dependencies; pager.c contains some more */

#include "hack.h"
#include "wintty.h"

#include <sys/stat.h>
#if defined(MSDOS)
#include <errno.h>
#endif

#if defined(MSDOS)
extern char orgdir[];
#endif

#if defined(TTY_GRAPHICS)
extern void backsp(void);
extern void term_clear_screen(void);
#endif

#if 0
static struct stat buf;
#endif

#ifdef WANT_GETHDATE
static struct stat hbuf;
#endif

#if defined(PC_LOCKING) && !defined(SELF_RECOVER)
static int eraseoldlocks(void);
#endif

#if 0
int
uptodate(int fd)
{
#ifdef WANT_GETHDATE
    if(fstat(fd, &buf)) {
        pline("Cannot get status of saved level? ");
        return(0);
    }
    if(buf.st_mtime < hbuf.st_mtime) {
        pline("Saved level is out of date. ");
        return(0);
    }
#else
#if (defined(MICRO)) && !defined(NO_FSTAT)
    if(fstat(fd, &buf)) {
        if(gm.moves > 1) pline("Cannot get status of saved level? ");
        else pline("Cannot get status of saved game.");
        return(0);
    } 
    if(comp_times(buf.st_mtime)) { 
        if(gm.moves > 1) pline("Saved level is out of date.");
        else pline("Saved game is out of date. ");
        /* This problem occurs enough times we need to give the player
         * some more information about what causes it, and how to fix.
         */
#ifdef MSDOS
        pline("Make sure that your system's date and time are correct.");
        pline("They must be more current than NetHack.EXE's date/time stamp.");
#endif /* MSDOS */
        return(0);
    }
#endif /* MICRO */
#endif /* WANT_GETHDATE */
    return(1);
}
#endif

#if defined(PC_LOCKING)
#if !defined(SELF_RECOVER)
static int
eraseoldlocks(void)
{
    register int i;

    /* cannot use maxledgerno() here, because we need to find a lock name
     * before starting everything (including the dungeon initialization
     * that sets astral_level, needed for maxledgerno()) up
     */
    for (i = 1; i <= MAXDUNGEON * MAXLEVEL + 1; i++) {
        /* try to remove all */
        set_levelfile_name(gl.lock, i);
        (void) unlink(fqname(gl.lock, LEVELPREFIX, 0));
    }
    set_levelfile_name(gl.lock, 0);
#ifdef HOLD_LOCKFILE_OPEN
    really_close();
#endif
    if (unlink(fqname(gl.lock, LEVELPREFIX, 0)))
        return 0; /* cannot remove it */
    return (1);   /* success! */
}
#endif /* SELF_RECOVER */

void
getlock(void)
{
    register int fd, c, ci, ct;
    int fcmask = FCMASK;
    char tbuf[BUFSZ];
    const char *fq_lock;
#if defined(MSDOS) && defined(NO_TERMS)
    int grmode = iflags.grmode;
#endif
    /* we ignore QUIT and INT at this point */
    if (!lock_file(HLOCK, LOCKPREFIX, 10)) {
        wait_synch();
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
        chdirx(orgdir, 0);
#endif
        error("Quitting.");
    }

    /* regularize(lock); */ /* already done in pcmain */
    Sprintf(tbuf, "%s", fqname(gl.lock, LEVELPREFIX, 0));
    set_levelfile_name(gl.lock, 0);
    fq_lock = fqname(gl.lock, LEVELPREFIX, 1);
    if ((fd = open(fq_lock, 0)) == -1) {
        if (errno == ENOENT)
            goto gotlock; /* no such file */
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
        chdirx(orgdir, 0);
#endif
        perror(fq_lock);
        unlock_file(HLOCK);
        error("Cannot open %s", fq_lock);
    }

    (void) nhclose(fd);

    if (iflags.window_inited) {
#ifdef SELF_RECOVER
        c = y_n("There are files from a game in progress under your name. "
               "Recover?");
#else
        pline("There is already a game in progress under your name.");
        pline("You may be able to use \"recover %s\" to get it back.\n",
              tbuf);
        c = y_n("Do you want to destroy the old game?");
#endif
    } else {
#if defined(MSDOS) && defined(NO_TERMS)
        grmode = iflags.grmode;
        if (grmode)
            gr_finish();
#endif
        c = 'n';
        ct = 0;
#ifdef SELF_RECOVER
        msmsg("There are files from a game in progress under your name. "
              "Recover? [yn]");
#else
        msmsg("\nThere is already a game in progress under your name.\n");
        msmsg("If this is unexpected, you may be able to use \n");
        msmsg("\"recover %s\" to get it back.", tbuf);
        msmsg("\nDo you want to destroy the old game? [yn] ");
#endif
        while ((ci = nhgetch()) != '\n') {
            if (ct > 0) {
                msmsg("\b \b");
                ct = 0;
                c = 'n';
            }
            if (ci == 'y' || ci == 'n' || ci == 'Y' || ci == 'N') {
                ct = 1;
                c = ci;
                msmsg("%c", c);
            }
        }
    }
    if (c == 'y' || c == 'Y')
#ifndef SELF_RECOVER
        if (eraseoldlocks()) {
            goto gotlock;
        } else {
            unlock_file(HLOCK);
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
            chdirx(orgdir, 0);
#endif
            error("Couldn't destroy old game.");
        }
#else /*SELF_RECOVER*/
        if (recover_savefile()) {
#if defined(TTY_GRAPHICS)
            if (WINDOWPORT(tty))
                term_clear_screen(); /* display gets fouled up otherwise */
#endif
            goto gotlock;
        } else {
            unlock_file(HLOCK);
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
            chdirx(orgdir, 0);
#endif
            error("Couldn't recover old game.");
        }
#endif /*SELF_RECOVER*/
    else {
        unlock_file(HLOCK);
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
        chdirx(orgdir, 0);
#endif
        error("%s", "Cannot start a new game.");
    }

gotlock:
    fd = creat(fq_lock, fcmask);
    unlock_file(HLOCK);
    if (fd == -1) {
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
        chdirx(orgdir, 0);
#endif
        error("cannot creat file (%s.)", fq_lock);
    } else {
        if (write(fd, (char *) &gh.hackpid, sizeof(gh.hackpid))
            != sizeof(gh.hackpid)) {
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
            chdirx(orgdir, 0);
#endif
            error("cannot write lock (%s)", fq_lock);
        }
        if (nhclose(fd) == -1) {
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
            chdirx(orgdir, 0);
#endif
            error("cannot close lock (%s)", fq_lock);
        }
    }
#if defined(MSDOS) && defined(NO_TERMS)
    if (grmode)
        gr_init();
#endif
}
#endif /* PC_LOCKING */

void
regularize(register char *s)
/*
 * normalize file name - we don't like .'s, /'s, spaces, and
 * lots of other things
 */
{
    register char *lp;

    for (lp = s; *lp; lp++)
        if (*lp <= ' ' || *lp == '"' || (*lp >= '*' && *lp <= ',')
            || *lp == '.' || *lp == '/' || (*lp >= ':' && *lp <= '?') ||
#ifdef OS2
            *lp == '&' || *lp == '(' || *lp == ')' ||
#endif
            *lp == '|' || *lp >= 127 || (*lp >= '[' && *lp <= ']'))
            *lp = '_';
}

#ifdef __EMX__
void
seteuid(int i)
{
    ;
}
#endif

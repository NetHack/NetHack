/* NetHack 3.6	save.c	$NHDT-Date: 1489192905 2017/03/11 00:41:45 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.101 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"

#ifndef NO_SIGNAL
#include <signal.h>
#endif
#if !defined(LSC) && !defined(O_WRONLY) && !defined(AZTEC_C)
#include <fcntl.h>
#endif

#ifdef MFLOPPY
long bytes_counted;
static int count_only;
#endif

#ifdef MICRO
int dotcnt, dotrow; /* also used in restore */
#endif

STATIC_DCL void savelevchn(int, int);
STATIC_DCL void savedamage(int, int);
STATIC_DCL void saveobj(int, struct obj *);
STATIC_DCL void saveobjchn(int, struct obj *, int);
STATIC_DCL void savemon(int, struct monst *);
STATIC_DCL void savemonchn(int, struct monst *, int);
STATIC_DCL void savetrapchn(int, struct trap *, int);
STATIC_DCL void savegamestate(int, int);
STATIC_OVL void save_msghistory(int, int);
#ifdef MFLOPPY
STATIC_DCL void savelev0(int, xchar, int);
STATIC_DCL boolean swapout_oldest(void);
STATIC_DCL void copyfile(char *, char *);
#endif /* MFLOPPY */
STATIC_DCL void savelevl(int fd, boolean);
STATIC_DCL void def_bufon(int);
STATIC_DCL void def_bufoff(int);
STATIC_DCL void def_bflush(int);
STATIC_DCL void def_bwrite(int, genericptr_t, unsigned int);
#ifdef ZEROCOMP
STATIC_DCL void zerocomp_bufon(int);
STATIC_DCL void zerocomp_bufoff(int);
STATIC_DCL void zerocomp_bflush(int);
STATIC_DCL void zerocomp_bwrite(int, genericptr_t, unsigned int);
STATIC_DCL void zerocomp_bputc(int);
#endif

static struct save_procs {
    const char *name;
    void (*save_bufon)(int);
    void (*save_bufoff)(int);
    void (*save_bflush)(int);
    void (*save_bwrite)(int, genericptr_t, unsigned int);
    void (*save_bclose)(int);
} saveprocs = {
#if !defined(ZEROCOMP) || (defined(COMPRESS) || defined(ZLIB_COMP))
    "externalcomp", def_bufon, def_bufoff, def_bflush, def_bwrite, def_bclose,
#else
    "zerocomp",      zerocomp_bufon,  zerocomp_bufoff,
    zerocomp_bflush, zerocomp_bwrite, zerocomp_bclose,
#endif
};

#if defined(UNIX) || defined(VMS) || defined(__EMX__) || defined(WIN32)
#define HUP if (!program_state.done_hup)
#else
#define HUP
#endif

/* need to preserve these during save to avoid accessing freed memory */
static unsigned ustuck_id = 0, usteed_id = 0;

int
dosave()
{
    clear_nhwindow(WIN_MESSAGE);
    if (yn("Really save?") == 'n') {
        clear_nhwindow(WIN_MESSAGE);
        if (multi > 0)
            nomul(0);
    } else {
        clear_nhwindow(WIN_MESSAGE);
        pline("Saving...");
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
        program_state.done_hup = 0;
#endif
        if (dosave0()) {
            u.uhp = -1; /* universal game's over indicator */
            /* make sure they see the Saving message */
            display_nhwindow(WIN_MESSAGE, TRUE);
            exit_nhwindows("Be seeing you...");
            nh_terminate(EXIT_SUCCESS);
        } else
            (void) doredraw();
    }
    return 0;
}

/* returns 1 if save successful */
int
dosave0()
{
    const char *fq_save;
    register int fd, ofd;
    xchar ltmp;
    d_level uz_save;
    char whynot[BUFSZ];

    /* we may get here via hangup signal, in which case we want to fix up
       a few of things before saving so that they won't be restored in
       an improper state; these will be no-ops for normal save sequence */
    u.uinvulnerable = 0;
    if (iflags.save_uswallow) {
        u.uswallow = 1; iflags.save_uswallow = 0;
    }
    if (iflags.save_uinwater) {
        u.uinwater = 1; iflags.save_uinwater = 0;
    }
    if (iflags.save_uburied) {
        u.uburied = 1; iflags.save_uburied = 0;
    }

    if (!program_state.something_worth_saving || !SAVEF[0])
        return 0;
    fq_save = fqname(SAVEF, SAVEPREFIX, 1); /* level files take 0 */

#if defined(UNIX) || defined(VMS)
    sethanguphandler((void ((*)(int) )) SIG_IGN);
#endif
#ifndef NO_SIGNAL
    (void) signal(SIGINT, SIG_IGN);
#endif

#if defined(MICRO) && defined(MFLOPPY)
    if (!saveDiskPrompt(0))
        return 0;
#endif

    HUP if (iflags.window_inited)
    {
        nh_uncompress(fq_save);
        fd = open_savefile();
        if (fd > 0) {
            (void) nhclose(fd);
            clear_nhwindow(WIN_MESSAGE);
            There("seems to be an old save file.");
            if (yn("Overwrite the old file?") == 'n') {
                nh_compress(fq_save);
                return 0;
            }
        }
    }

    HUP mark_synch(); /* flush any buffered screen output */

    fd = create_savefile();
    if (fd < 0) {
        HUP pline("Cannot open save file.");
        (void) delete_savefile(); /* ab@unido */
        return 0;
    }

    vision_recalc(2); /* shut down vision to prevent problems
                         in the event of an impossible() call */

    /* undo date-dependent luck adjustments made at startup time */
    if (flags.moonphase == FULL_MOON) /* ut-sally!fletcher */
        change_luck(-1);              /* and unido!ab */
    if (flags.friday13)
        change_luck(1);
    if (iflags.window_inited)
        HUP clear_nhwindow(WIN_MESSAGE);

#ifdef MICRO
    dotcnt = 0;
    dotrow = 2;
    curs(WIN_MAP, 1, 1);
    if (strncmpi("X11", windowprocs.name, 3))
        putstr(WIN_MAP, 0, "Saving:");
#endif
#ifdef MFLOPPY
    /* make sure there is enough disk space */
    if (iflags.checkspace) {
        long fds, needed;

        savelev(fd, ledger_no(&u.uz), COUNT_SAVE);
        savegamestate(fd, COUNT_SAVE);
        needed = bytes_counted;

        for (ltmp = 1; ltmp <= maxledgerno(); ltmp++)
            if (ltmp != ledger_no(&u.uz) && level_info[ltmp].where)
                needed += level_info[ltmp].size + (sizeof ltmp);
        fds = freediskspace(fq_save);
        if (needed > fds) {
            HUP
            {
                There("is insufficient space on SAVE disk.");
                pline("Require %ld bytes but only have %ld.", needed, fds);
            }
            flushout();
            (void) nhclose(fd);
            (void) delete_savefile();
            return 0;
        }

        co_false();
    }
#endif /* MFLOPPY */

    store_version(fd);
    store_savefileinfo(fd);
    store_plname_in_file(fd);
    ustuck_id = (u.ustuck ? u.ustuck->m_id : 0);
    usteed_id = (u.usteed ? u.usteed->m_id : 0);
    savelev(fd, ledger_no(&u.uz), WRITE_SAVE | FREE_SAVE);
    savegamestate(fd, WRITE_SAVE | FREE_SAVE);

    /* While copying level files around, zero out u.uz to keep
     * parts of the restore code from completely initializing all
     * in-core data structures, since all we're doing is copying.
     * This also avoids at least one nasty core dump.
     */
    uz_save = u.uz;
    u.uz.dnum = u.uz.dlevel = 0;
    /* these pointers are no longer valid, and at least u.usteed
     * may mislead place_monster() on other levels
     */
    u.ustuck = (struct monst *) 0;
    u.usteed = (struct monst *) 0;

    for (ltmp = (xchar) 1; ltmp <= maxledgerno(); ltmp++) {
        if (ltmp == ledger_no(&uz_save))
            continue;
        if (!(level_info[ltmp].flags & LFILE_EXISTS))
            continue;
#ifdef MICRO
        curs(WIN_MAP, 1 + dotcnt++, dotrow);
        if (dotcnt >= (COLNO - 1)) {
            dotrow++;
            dotcnt = 0;
        }
        if (strncmpi("X11", windowprocs.name, 3)) {
            putstr(WIN_MAP, 0, ".");
        }
        mark_synch();
#endif
        ofd = open_levelfile(ltmp, whynot);
        if (ofd < 0) {
            HUP pline1(whynot);
            (void) nhclose(fd);
            (void) delete_savefile();
            HUP Strcpy(killer.name, whynot);
            HUP done(TRICKED);
            return 0;
        }
        minit(); /* ZEROCOMP */
        getlev(ofd, hackpid, ltmp, FALSE);
        (void) nhclose(ofd);
        bwrite(fd, (genericptr_t) &ltmp, sizeof ltmp); /* level number*/
        savelev(fd, ltmp, WRITE_SAVE | FREE_SAVE);     /* actual level*/
        delete_levelfile(ltmp);
    }
    bclose(fd);

    u.uz = uz_save;

    /* get rid of current level --jgm */
    delete_levelfile(ledger_no(&u.uz));
    delete_levelfile(0);
    nh_compress(fq_save);
    /* this should probably come sooner... */
    program_state.something_worth_saving = 0;
    return 1;
}

STATIC_OVL void
savegamestate(register int fd, register int mode)
{
    unsigned long uid;

#ifdef MFLOPPY
    count_only = (mode & COUNT_SAVE);
#endif
    uid = (unsigned long) getuid();
    bwrite(fd, (genericptr_t) &uid, sizeof uid);
    bwrite(fd, (genericptr_t) &context, sizeof(struct context_info));
    bwrite(fd, (genericptr_t) &flags, sizeof(struct flag));
#ifdef SYSFLAGS
    bwrite(fd, (genericptr_t) &sysflags, sizeof(struct sysflag));
#endif
    urealtime.finish_time = getnow();
    urealtime.realtime += (long) (urealtime.finish_time
                                  - urealtime.start_timing);
    bwrite(fd, (genericptr_t) &u, sizeof(struct you));
    bwrite(fd, yyyymmddhhmmss(ubirthday), 14);
    bwrite(fd, (genericptr_t) &urealtime.realtime, sizeof urealtime.realtime);
    bwrite(fd, yyyymmddhhmmss(urealtime.start_timing), 14);  /** Why? **/
    /* this is the value to use for the next update of urealtime.realtime */
    urealtime.start_timing = urealtime.finish_time;
    save_killers(fd, mode);

    /* must come before migrating_objs and migrating_mons are freed */
    save_timers(fd, mode, RANGE_GLOBAL);
    save_light_sources(fd, mode, RANGE_GLOBAL);

    saveobjchn(fd, invent, mode);
    if (BALL_IN_MON) {
        /* prevent loss of ball & chain when swallowed */
        uball->nobj = uchain;
        uchain->nobj = (struct obj *) 0;
        saveobjchn(fd, uball, mode);
    } else {
        saveobjchn(fd, (struct obj *) 0, mode);
    }

    saveobjchn(fd, migrating_objs, mode);
    savemonchn(fd, migrating_mons, mode);
    if (release_data(mode)) {
        invent = 0;
        migrating_objs = 0;
        migrating_mons = 0;
    }
    bwrite(fd, (genericptr_t) mvitals, sizeof(mvitals));

    save_dungeon(fd, (boolean) !!perform_bwrite(mode),
                 (boolean) !!release_data(mode));
    savelevchn(fd, mode);
    bwrite(fd, (genericptr_t) &moves, sizeof moves);
    bwrite(fd, (genericptr_t) &monstermoves, sizeof monstermoves);
    bwrite(fd, (genericptr_t) &quest_status, sizeof(struct q_score));
    bwrite(fd, (genericptr_t) spl_book,
           sizeof(struct spell) * (MAXSPELL + 1));
    save_artifacts(fd);
    save_oracles(fd, mode);
    if (ustuck_id)
        bwrite(fd, (genericptr_t) &ustuck_id, sizeof ustuck_id);
    if (usteed_id)
        bwrite(fd, (genericptr_t) &usteed_id, sizeof usteed_id);
    bwrite(fd, (genericptr_t) pl_character, sizeof pl_character);
    bwrite(fd, (genericptr_t) pl_fruit, sizeof pl_fruit);
    savefruitchn(fd, mode);
    savenames(fd, mode);
    save_waterlevel(fd, mode);
    save_msghistory(fd, mode);
    bflush(fd);
}

boolean
tricked_fileremoved(int fd, char *whynot)
{
    if (fd < 0) {
        pline1(whynot);
        pline("Probably someone removed it.");
        Strcpy(killer.name, whynot);
        done(TRICKED);
        return TRUE;
    }
    return FALSE;
}

#ifdef INSURANCE
void
savestateinlock()
{
    int fd, hpid;
    static boolean havestate = TRUE;
    char whynot[BUFSZ];

    /* When checkpointing is on, the full state needs to be written
     * on each checkpoint.  When checkpointing is off, only the pid
     * needs to be in the level.0 file, so it does not need to be
     * constantly rewritten.  When checkpointing is turned off during
     * a game, however, the file has to be rewritten once to truncate
     * it and avoid restoring from outdated information.
     *
     * Restricting havestate to this routine means that an additional
     * noop pid rewriting will take place on the first "checkpoint" after
     * the game is started or restored, if checkpointing is off.
     */
    if (flags.ins_chkpt || havestate) {
        /* save the rest of the current game state in the lock file,
         * following the original int pid, the current level number,
         * and the current savefile name, which should not be subject
         * to any internal compression schemes since they must be
         * readable by an external utility
         */
        fd = open_levelfile(0, whynot);
        if (tricked_fileremoved(fd, whynot))
            return;

        (void) read(fd, (genericptr_t) &hpid, sizeof(hpid));
        if (hackpid != hpid) {
            Sprintf(whynot, "Level #0 pid (%d) doesn't match ours (%d)!",
                    hpid, hackpid);
            pline1(whynot);
            Strcpy(killer.name, whynot);
            done(TRICKED);
        }
        (void) nhclose(fd);

        fd = create_levelfile(0, whynot);
        if (fd < 0) {
            pline1(whynot);
            Strcpy(killer.name, whynot);
            done(TRICKED);
            return;
        }
        (void) write(fd, (genericptr_t) &hackpid, sizeof(hackpid));
        if (flags.ins_chkpt) {
            int currlev = ledger_no(&u.uz);

            (void) write(fd, (genericptr_t) &currlev, sizeof(currlev));
            save_savefile_name(fd);
            store_version(fd);
            store_savefileinfo(fd);
            store_plname_in_file(fd);

            ustuck_id = (u.ustuck ? u.ustuck->m_id : 0);
            usteed_id = (u.usteed ? u.usteed->m_id : 0);
            savegamestate(fd, WRITE_SAVE);
        }
        bclose(fd);
    }
    havestate = flags.ins_chkpt;
}
#endif

#ifdef MFLOPPY
boolean
savelev(int fd, xchar lev, int mode)
{
    if (mode & COUNT_SAVE) {
        bytes_counted = 0;
        savelev0(fd, lev, COUNT_SAVE);
        /* probably bytes_counted will be filled in again by an
         * immediately following WRITE_SAVE anyway, but we'll
         * leave it out of checkspace just in case */
        if (iflags.checkspace) {
            while (bytes_counted > freediskspace(levels))
                if (!swapout_oldest())
                    return FALSE;
        }
    }
    if (mode & (WRITE_SAVE | FREE_SAVE)) {
        bytes_counted = 0;
        savelev0(fd, lev, mode);
    }
    if (mode != FREE_SAVE) {
        level_info[lev].where = ACTIVE;
        level_info[lev].time = moves;
        level_info[lev].size = bytes_counted;
    }
    return TRUE;
}

STATIC_OVL void
savelev0(int fd, xchar lev, int mode)
#else
void
savelev(int fd, xchar lev, int mode)
#endif
{
#ifdef TOS
    short tlev;
#endif

    /* if we're tearing down the current level without saving anything
       (which happens upon entrance to the endgame or after an aborted
       restore attempt) then we don't want to do any actual I/O */
    if (mode == FREE_SAVE)
        goto skip_lots;

    /* purge any dead monsters (necessary if we're starting
       a panic save rather than a normal one, or sometimes
       when changing levels without taking time -- e.g.
       create statue trap then immediately level teleport) */
    if (iflags.purge_monsters)
        dmonsfree();

    if (fd < 0)
        panic("Save on bad file!"); /* impossible */
#ifdef MFLOPPY
    count_only = (mode & COUNT_SAVE);
#endif
    if (lev >= 0 && lev <= maxledgerno())
        level_info[lev].flags |= VISITED;
    bwrite(fd, (genericptr_t) &hackpid, sizeof(hackpid));
#ifdef TOS
    tlev = lev;
    tlev &= 0x00ff;
    bwrite(fd, (genericptr_t) &tlev, sizeof(tlev));
#else
    bwrite(fd, (genericptr_t) &lev, sizeof(lev));
#endif
    savecemetery(fd, mode, &level.bonesinfo);
    savelevl(fd,
             (boolean) ((sfsaveinfo.sfi1 & SFI1_RLECOMP) == SFI1_RLECOMP));
    bwrite(fd, (genericptr_t) lastseentyp, sizeof(lastseentyp));
    bwrite(fd, (genericptr_t) &monstermoves, sizeof(monstermoves));
    bwrite(fd, (genericptr_t) &upstair, sizeof(stairway));
    bwrite(fd, (genericptr_t) &dnstair, sizeof(stairway));
    bwrite(fd, (genericptr_t) &upladder, sizeof(stairway));
    bwrite(fd, (genericptr_t) &dnladder, sizeof(stairway));
    bwrite(fd, (genericptr_t) &sstairs, sizeof(stairway));
    bwrite(fd, (genericptr_t) &updest, sizeof(dest_area));
    bwrite(fd, (genericptr_t) &dndest, sizeof(dest_area));
    bwrite(fd, (genericptr_t) &level.flags, sizeof(level.flags));
    bwrite(fd, (genericptr_t) doors, sizeof(doors));
    save_rooms(fd); /* no dynamic memory to reclaim */

/* from here on out, saving also involves allocated memory cleanup */
skip_lots:
    /* this comes before the map, so need cleanup here if we skipped */
    if (mode == FREE_SAVE)
        savecemetery(fd, mode, &level.bonesinfo);
    /* must be saved before mons, objs, and buried objs */
    save_timers(fd, mode, RANGE_LEVEL);
    save_light_sources(fd, mode, RANGE_LEVEL);

    savemonchn(fd, fmon, mode);
    save_worm(fd, mode); /* save worm information */
    savetrapchn(fd, ftrap, mode);
    saveobjchn(fd, fobj, mode);
    saveobjchn(fd, level.buriedobjlist, mode);
    saveobjchn(fd, billobjs, mode);
    if (release_data(mode)) {
        fmon = 0;
        ftrap = 0;
        fobj = 0;
        level.buriedobjlist = 0;
        billobjs = 0;
        /* level.bonesinfo = 0; -- handled by savecemetery() */
    }
    save_engravings(fd, mode);
    savedamage(fd, mode);
    save_regions(fd, mode);
    if (mode != FREE_SAVE)
        bflush(fd);
}

STATIC_OVL void
savelevl(int fd, boolean rlecomp)
{
#ifdef RLECOMP
    struct rm *prm, *rgrm;
    int x, y;
    uchar match;

    if (rlecomp) {
        /* perform run-length encoding of rm structs */

        rgrm = &levl[0][0]; /* start matching at first rm */
        match = 0;

        for (y = 0; y < ROWNO; y++) {
            for (x = 0; x < COLNO; x++) {
                prm = &levl[x][y];
                if (prm->glyph == rgrm->glyph && prm->typ == rgrm->typ
                    && prm->seenv == rgrm->seenv
                    && prm->horizontal == rgrm->horizontal
                    && prm->flags == rgrm->flags && prm->lit == rgrm->lit
                    && prm->waslit == rgrm->waslit
                    && prm->roomno == rgrm->roomno && prm->edge == rgrm->edge
                    && prm->candig == rgrm->candig) {
                    match++;
                    if (match > 254) {
                        match = 254; /* undo this match */
                        goto writeout;
                    }
                } else {
                /* the run has been broken,
                 * write out run-length encoding */
                writeout:
                    bwrite(fd, (genericptr_t) &match, sizeof(uchar));
                    bwrite(fd, (genericptr_t) rgrm, sizeof(struct rm));
                    /* start encoding again. we have at least 1 rm
                     * in the next run, viz. this one. */
                    match = 1;
                    rgrm = prm;
                }
            }
        }
        if (match > 0) {
            bwrite(fd, (genericptr_t) &match, sizeof(uchar));
            bwrite(fd, (genericptr_t) rgrm, sizeof(struct rm));
        }
        return;
    }
#else /* !RLECOMP */
    nhUse(rlecomp);
#endif /* ?RLECOMP */
    bwrite(fd, (genericptr_t) levl, sizeof levl);
}

/*ARGSUSED*/
void
bufon(int fd)
{
    (*saveprocs.save_bufon)(fd);
    return;
}

/*ARGSUSED*/
void
bufoff(int fd)
{
    (*saveprocs.save_bufoff)(fd);
    return;
}

/* flush run and buffer */
void
bflush(register int fd)
{
    (*saveprocs.save_bflush)(fd);
    return;
}

void
bwrite(int fd, genericptr_t loc, register unsigned num)
{
    (*saveprocs.save_bwrite)(fd, loc, num);
    return;
}

void
bclose(int fd)
{
    (*saveprocs.save_bclose)(fd);
    return;
}

static int bw_fd = -1;
static FILE *bw_FILE = 0;
static boolean buffering = FALSE;

STATIC_OVL void
def_bufon(int fd)
{
#ifdef UNIX
    if (bw_fd != fd) {
        if (bw_fd >= 0)
            panic("double buffering unexpected");
        bw_fd = fd;
        if ((bw_FILE = fdopen(fd, "w")) == 0)
            panic("buffering of file %d failed", fd);
    }
#endif
    buffering = TRUE;
}

STATIC_OVL void
def_bufoff(int fd)
{
    def_bflush(fd);
    buffering = FALSE;
}

STATIC_OVL void
def_bflush(int fd)
{
#ifdef UNIX
    if (fd == bw_fd) {
        if (fflush(bw_FILE) == EOF)
            panic("flush of savefile failed!");
    }
#endif
    return;
}

STATIC_OVL void
def_bwrite(register int fd, register genericptr_t loc, register unsigned num)
{
    boolean failed;

#ifdef MFLOPPY
    bytes_counted += num;
    if (count_only)
        return;
#endif

#ifdef UNIX
    if (buffering) {
        if (fd != bw_fd)
            panic("unbuffered write to fd %d (!= %d)", fd, bw_fd);

        failed = (fwrite(loc, (int) num, 1, bw_FILE) != 1);
    } else
#endif /* UNIX */
    {
        /* lint wants 3rd arg of write to be an int; lint -p an unsigned */
#if defined(BSD) || defined(ULTRIX) || defined(WIN32) || defined(_MSC_VER)
        failed = ((long) write(fd, loc, (int) num) != (long) num);
#else /* e.g. SYSV, __TURBOC__ */
        failed = ((long) write(fd, loc, num) != (long) num);
#endif
    }

    if (failed) {
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
        if (program_state.done_hup)
            nh_terminate(EXIT_FAILURE);
        else
#endif
            panic("cannot write %u bytes to file #%d", num, fd);
    }
}

void
def_bclose(int fd)
{
    bufoff(fd);
#ifdef UNIX
    if (fd == bw_fd) {
        (void) fclose(bw_FILE);
        bw_fd = -1;
        bw_FILE = 0;
    } else
#endif
        (void) nhclose(fd);
    return;
}

#ifdef ZEROCOMP
/* The runs of zero-run compression are flushed after the game state or a
 * level is written out.  This adds a couple bytes to a save file, where
 * the runs could be mashed together, but it allows gluing together game
 * state and level files to form a save file, and it means the flushing
 * does not need to be specifically called for every other time a level
 * file is written out.
 */

#define RLESC '\0' /* Leading character for run of LRESC's */
#define flushoutrun(ln) (zerocomp_bputc(RLESC), zerocomp_bputc(ln), ln = -1)

#ifndef ZEROCOMP_BUFSIZ
#define ZEROCOMP_BUFSIZ BUFSZ
#endif
static NEARDATA unsigned char outbuf[ZEROCOMP_BUFSIZ];
static NEARDATA unsigned short outbufp = 0;
static NEARDATA short outrunlength = -1;
static NEARDATA int bwritefd;
static NEARDATA boolean compressing = FALSE;

/*dbg()
{
    HUP printf("outbufp %d outrunlength %d\n", outbufp,outrunlength);
}*/

STATIC_OVL void
zerocomp_bputc(int c)
{
#ifdef MFLOPPY
    bytes_counted++;
    if (count_only)
        return;
#endif
    if (outbufp >= sizeof outbuf) {
        (void) write(bwritefd, outbuf, sizeof outbuf);
        outbufp = 0;
    }
    outbuf[outbufp++] = (unsigned char) c;
}

/*ARGSUSED*/
void STATIC_OVL
zerocomp_bufon(int fd)
{
    compressing = TRUE;
    return;
}

/*ARGSUSED*/
STATIC_OVL void
zerocomp_bufoff(int fd)
{
    if (outbufp) {
        outbufp = 0;
        panic("closing file with buffered data still unwritten");
    }
    outrunlength = -1;
    compressing = FALSE;
    return;
}

/* flush run and buffer */
STATIC_OVL void
zerocomp_bflush(register int fd)
{
    bwritefd = fd;
    if (outrunlength >= 0) { /* flush run */
        flushoutrun(outrunlength);
    }
#ifdef MFLOPPY
    if (count_only)
        outbufp = 0;
#endif

    if (outbufp) {
        if (write(fd, outbuf, outbufp) != outbufp) {
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
            if (program_state.done_hup)
                nh_terminate(EXIT_FAILURE);
            else
#endif
                zerocomp_bclose(fd); /* panic (outbufp != 0) */
        }
        outbufp = 0;
    }
}

STATIC_OVL void
zerocomp_bwrite(int fd, genericptr_t loc, register unsigned num)
{
    register unsigned char *bp = (unsigned char *) loc;

    if (!compressing) {
#ifdef MFLOPPY
        bytes_counted += num;
        if (count_only)
            return;
#endif
        if ((unsigned) write(fd, loc, num) != num) {
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
            if (program_state.done_hup)
                nh_terminate(EXIT_FAILURE);
            else
#endif
                panic("cannot write %u bytes to file #%d", num, fd);
        }
    } else {
        bwritefd = fd;
        for (; num; num--, bp++) {
            if (*bp == RLESC) { /* One more char in run */
                if (++outrunlength == 0xFF) {
                    flushoutrun(outrunlength);
                }
            } else {                     /* end of run */
                if (outrunlength >= 0) { /* flush run */
                    flushoutrun(outrunlength);
                }
                zerocomp_bputc(*bp);
            }
        }
    }
}

void
zerocomp_bclose(int fd)
{
    zerocomp_bufoff(fd);
    (void) nhclose(fd);
    return;
}
#endif /* ZEROCOMP */

STATIC_OVL void
savelevchn(register int fd, register int mode)
{
    s_level *tmplev, *tmplev2;
    int cnt = 0;

    for (tmplev = sp_levchn; tmplev; tmplev = tmplev->next)
        cnt++;
    if (perform_bwrite(mode))
        bwrite(fd, (genericptr_t) &cnt, sizeof(int));

    for (tmplev = sp_levchn; tmplev; tmplev = tmplev2) {
        tmplev2 = tmplev->next;
        if (perform_bwrite(mode))
            bwrite(fd, (genericptr_t) tmplev, sizeof(s_level));
        if (release_data(mode))
            free((genericptr_t) tmplev);
    }
    if (release_data(mode))
        sp_levchn = 0;
}

/* used when saving a level and also when saving dungeon overview data */
void
savecemetery(int fd, int mode, struct cemetery **cemeteryaddr)
{
    struct cemetery *thisbones, *nextbones;
    int flag;

    flag = *cemeteryaddr ? 0 : -1;
    if (perform_bwrite(mode))
        bwrite(fd, (genericptr_t) &flag, sizeof flag);
    nextbones = *cemeteryaddr;
    while ((thisbones = nextbones) != 0) {
        nextbones = thisbones->next;
        if (perform_bwrite(mode))
            bwrite(fd, (genericptr_t) thisbones, sizeof *thisbones);
        if (release_data(mode))
            free((genericptr_t) thisbones);
    }
    if (release_data(mode))
        *cemeteryaddr = 0;
}

STATIC_OVL void
savedamage(register int fd, register int mode)
{
    register struct damage *damageptr, *tmp_dam;
    unsigned int xl = 0;

    damageptr = level.damagelist;
    for (tmp_dam = damageptr; tmp_dam; tmp_dam = tmp_dam->next)
        xl++;
    if (perform_bwrite(mode))
        bwrite(fd, (genericptr_t) &xl, sizeof(xl));

    while (xl--) {
        if (perform_bwrite(mode))
            bwrite(fd, (genericptr_t) damageptr, sizeof(*damageptr));
        tmp_dam = damageptr;
        damageptr = damageptr->next;
        if (release_data(mode))
            free((genericptr_t) tmp_dam);
    }
    if (release_data(mode))
        level.damagelist = 0;
}

STATIC_OVL void
saveobj(int fd, struct obj *otmp)
{
    int buflen, zerobuf = 0;

    buflen = sizeof(struct obj);
    bwrite(fd, (genericptr_t) &buflen, sizeof(int));
    bwrite(fd, (genericptr_t) otmp, buflen);
    if (otmp->oextra) {
        if (ONAME(otmp))
            buflen = strlen(ONAME(otmp)) + 1;
        else
            buflen = 0;
        bwrite(fd, (genericptr_t) &buflen, sizeof buflen);
        if (buflen > 0)
            bwrite(fd, (genericptr_t) ONAME(otmp), buflen);

        /* defer to savemon() for this one */
        if (OMONST(otmp))
            savemon(fd, OMONST(otmp));
        else
            bwrite(fd, (genericptr_t) &zerobuf, sizeof zerobuf);

        if (OMID(otmp))
            buflen = sizeof(unsigned);
        else
            buflen = 0;
        bwrite(fd, (genericptr_t) &buflen, sizeof buflen);
        if (buflen > 0)
            bwrite(fd, (genericptr_t) OMID(otmp), buflen);

        if (OLONG(otmp))
            buflen = sizeof(long);
        else
            buflen = 0;
        bwrite(fd, (genericptr_t) &buflen, sizeof buflen);
        if (buflen > 0)
            bwrite(fd, (genericptr_t) OLONG(otmp), buflen);

        if (OMAILCMD(otmp))
            buflen = strlen(OMAILCMD(otmp)) + 1;
        else
            buflen = 0;
        bwrite(fd, (genericptr_t) &buflen, sizeof buflen);
        if (buflen > 0)
            bwrite(fd, (genericptr_t) OMAILCMD(otmp), buflen);
    }
}

STATIC_OVL void
saveobjchn(register int fd, register struct obj *otmp, register int mode)
{
    register struct obj *otmp2;
    int minusone = -1;

    while (otmp) {
        otmp2 = otmp->nobj;
        if (perform_bwrite(mode)) {
            saveobj(fd, otmp);
        }
        if (Has_contents(otmp))
            saveobjchn(fd, otmp->cobj, mode);
        if (release_data(mode)) {
            /*  if (otmp->oclass == FOOD_CLASS)
             *      food_disappears(otmp);
             */
            /*
             * If these are on the floor, the discarding could
             * be because of a game save, or we could just be changing levels.
             * Always invalidate the pointer, but ensure that we have
             * the o_id in order to restore the pointer on reload.
             */
            if (otmp == context.victual.piece) {
                /* Store the o_id of the victual if mismatched */
                if (context.victual.o_id != otmp->o_id)
                    context.victual.o_id = otmp->o_id;
                /* invalidate the pointer; on reload it will get restored */
                context.victual.piece = (struct obj *) 0;
            }
            if (otmp == context.tin.tin) {
                /* Store the o_id of your tin */
                if (context.tin.o_id != otmp->o_id)
                    context.tin.o_id = otmp->o_id;
                /* invalidate the pointer; on reload it will get restored */
                context.tin.tin = (struct obj *) 0;
            }
            /*  if (otmp->oclass == SPBOOK_CLASS)
             *      book_disappears(otmp);
             */
            if (otmp == context.spbook.book) {
                /* Store the o_id of your spellbook */
                if (context.spbook.o_id != otmp->o_id)
                    context.spbook.o_id = otmp->o_id;
                /* invalidate the pointer; on reload it will get restored */
                context.spbook.book = (struct obj *) 0;
            }
            otmp->where = OBJ_FREE; /* set to free so dealloc will work */
            otmp->nobj = NULL;      /* nobj saved into otmp2 */
            otmp->cobj = NULL;      /* contents handled above */
            otmp->timed = 0;        /* not timed any more */
            otmp->lamplit = 0;      /* caller handled lights */
            dealloc_obj(otmp);
        }
        otmp = otmp2;
    }
    if (perform_bwrite(mode))
        bwrite(fd, (genericptr_t) &minusone, sizeof(int));
}

STATIC_OVL void
savemon(int fd, struct monst *mtmp)
{
    int buflen;

    buflen = sizeof(struct monst);
    bwrite(fd, (genericptr_t) &buflen, sizeof(int));
    bwrite(fd, (genericptr_t) mtmp, buflen);
    if (mtmp->mextra) {
        if (MNAME(mtmp))
            buflen = strlen(MNAME(mtmp)) + 1;
        else
            buflen = 0;
        bwrite(fd, (genericptr_t) &buflen, sizeof buflen);
        if (buflen > 0)
            bwrite(fd, (genericptr_t) MNAME(mtmp), buflen);

        if (EGD(mtmp))
            buflen = sizeof(struct egd);
        else
            buflen = 0;
        bwrite(fd, (genericptr_t) &buflen, sizeof(int));
        if (buflen > 0)
            bwrite(fd, (genericptr_t) EGD(mtmp), buflen);

        if (EPRI(mtmp))
            buflen = sizeof(struct epri);
        else
            buflen = 0;
        bwrite(fd, (genericptr_t) &buflen, sizeof(int));
        if (buflen > 0)
            bwrite(fd, (genericptr_t) EPRI(mtmp), buflen);

        if (ESHK(mtmp))
            buflen = sizeof(struct eshk);
        else
            buflen = 0;
        bwrite(fd, (genericptr_t) &buflen, sizeof(int));
        if (buflen > 0)
            bwrite(fd, (genericptr_t) ESHK(mtmp), buflen);

        if (EMIN(mtmp))
            buflen = sizeof(struct emin);
        else
            buflen = 0;
        bwrite(fd, (genericptr_t) &buflen, sizeof(int));
        if (buflen > 0)
            bwrite(fd, (genericptr_t) EMIN(mtmp), buflen);

        if (EDOG(mtmp))
            buflen = sizeof(struct edog);
        else
            buflen = 0;
        bwrite(fd, (genericptr_t) &buflen, sizeof(int));
        if (buflen > 0)
            bwrite(fd, (genericptr_t) EDOG(mtmp), buflen);

        /* mcorpsenm is inline int rather than pointer to something,
           so doesn't need to be preceded by a length field */
        bwrite(fd, (genericptr_t) &MCORPSENM(mtmp), sizeof MCORPSENM(mtmp));
    }
}

STATIC_OVL void
savemonchn(register int fd, register struct monst *mtmp, register int mode)
{
    register struct monst *mtmp2;
    int minusone = -1;

    while (mtmp) {
        mtmp2 = mtmp->nmon;
        if (perform_bwrite(mode)) {
            mtmp->mnum = monsndx(mtmp->data);
            if (mtmp->ispriest)
                forget_temple_entry(mtmp); /* EPRI() */
            savemon(fd, mtmp);
        }
        if (mtmp->minvent)
            saveobjchn(fd, mtmp->minvent, mode);
        if (release_data(mode)) {
            if (mtmp == context.polearm.hitmon) {
                context.polearm.m_id = mtmp->m_id;
                context.polearm.hitmon = NULL;
            }
            mtmp->nmon = NULL;  /* nmon saved into mtmp2 */
            dealloc_monst(mtmp);
        }
        mtmp = mtmp2;
    }
    if (perform_bwrite(mode))
        bwrite(fd, (genericptr_t) &minusone, sizeof(int));
}

/* save traps; ftrap is the only trap chain so the 2nd arg is superfluous */
STATIC_OVL void
savetrapchn(int fd, register struct trap *trap, int mode)
{
    static struct trap zerotrap;
    register struct trap *trap2;

    while (trap) {
        trap2 = trap->ntrap;
        if (perform_bwrite(mode))
            bwrite(fd, (genericptr_t) trap, sizeof (struct trap));
        if (release_data(mode))
            dealloc_trap(trap);
        trap = trap2;
    }
    if (perform_bwrite(mode))
        bwrite(fd, (genericptr_t) &zerotrap, sizeof zerotrap);
}

/* save all the fruit names and ID's; this is used only in saving whole games
 * (not levels) and in saving bones levels.  When saving a bones level,
 * we only want to save the fruits which exist on the bones level; the bones
 * level routine marks nonexistent fruits by making the fid negative.
 */
void
savefruitchn(int fd, int mode)
{
    static struct fruit zerofruit;
    register struct fruit *f2, *f1;

    f1 = ffruit;
    while (f1) {
        f2 = f1->nextf;
        if (f1->fid >= 0 && perform_bwrite(mode))
            bwrite(fd, (genericptr_t) f1, sizeof (struct fruit));
        if (release_data(mode))
            dealloc_fruit(f1);
        f1 = f2;
    }
    if (perform_bwrite(mode))
        bwrite(fd, (genericptr_t) &zerofruit, sizeof zerofruit);
    if (release_data(mode))
        ffruit = 0;
}

void
store_plname_in_file(int fd)
{
    int plsiztmp = PL_NSIZ;

    bufoff(fd);
    /* bwrite() before bufon() uses plain write() */
    bwrite(fd, (genericptr_t) &plsiztmp, sizeof(plsiztmp));
    bwrite(fd, (genericptr_t) plname, plsiztmp);
    bufon(fd);
    return;
}

STATIC_OVL void
save_msghistory(int fd, int mode)
{
    char *msg;
    int msgcount = 0, msglen;
    int minusone = -1;
    boolean init = TRUE;

    if (perform_bwrite(mode)) {
        /* ask window port for each message in sequence */
        while ((msg = getmsghistory(init)) != 0) {
            init = FALSE;
            msglen = strlen(msg);
            /* sanity: truncate if necessary (shouldn't happen);
               no need to modify msg[] since terminator isn't written */
            if (msglen > BUFSZ - 1)
                msglen = BUFSZ - 1;
            bwrite(fd, (genericptr_t) &msglen, sizeof(msglen));
            bwrite(fd, (genericptr_t) msg, msglen);
            ++msgcount;
        }
        bwrite(fd, (genericptr_t) &minusone, sizeof(int));
    }
    debugpline1("Stored %d messages into savefile.", msgcount);
    /* note: we don't attempt to handle release_data() here */
}

void
store_savefileinfo(int fd)
{
    /* sfcap (decl.c) describes the savefile feature capabilities
     * that are supported by this port/platform build.
     *
     * sfsaveinfo (decl.c) describes the savefile info that actually
     * gets written into the savefile, and is used to determine the
     * save file being written.

     * sfrestinfo (decl.c) describes the savefile info that is
     * being used to read the information from an existing savefile.
     *
     */

    bufoff(fd);
    /* bwrite() before bufon() uses plain write() */
    bwrite(fd, (genericptr_t) &sfsaveinfo, (unsigned) (sizeof sfsaveinfo));
    bufon(fd);
    return;
}

void
set_savepref(const char *suitename)
{
    if (!strcmpi(suitename, "externalcomp")) {
        saveprocs.name = "externalcomp";
        saveprocs.save_bufon = def_bufon;
        saveprocs.save_bufoff = def_bufoff;
        saveprocs.save_bflush = def_bflush;
        saveprocs.save_bwrite = def_bwrite;
        saveprocs.save_bclose = def_bclose;
        sfsaveinfo.sfi1 |= SFI1_EXTERNALCOMP;
        sfsaveinfo.sfi1 &= ~SFI1_ZEROCOMP;
    }
    if (!strcmpi(suitename, "!rlecomp")) {
        sfsaveinfo.sfi1 &= ~SFI1_RLECOMP;
    }
#ifdef ZEROCOMP
    if (!strcmpi(suitename, "zerocomp")) {
        saveprocs.name = "zerocomp";
        saveprocs.save_bufon = zerocomp_bufon;
        saveprocs.save_bufoff = zerocomp_bufoff;
        saveprocs.save_bflush = zerocomp_bflush;
        saveprocs.save_bwrite = zerocomp_bwrite;
        saveprocs.save_bclose = zerocomp_bclose;
        sfsaveinfo.sfi1 |= SFI1_ZEROCOMP;
        sfsaveinfo.sfi1 &= ~SFI1_EXTERNALCOMP;
    }
#endif
#ifdef RLECOMP
    if (!strcmpi(suitename, "rlecomp")) {
        sfsaveinfo.sfi1 |= SFI1_RLECOMP;
    }
#endif
}

/* also called by prscore(); this probably belongs in dungeon.c... */
void
free_dungeons()
{
#ifdef FREE_ALL_MEMORY
    savelevchn(0, FREE_SAVE);
    save_dungeon(0, FALSE, TRUE);
#endif
    return;
}

void
freedynamicdata()
{
#if defined(UNIX) && defined(MAIL)
    free_maildata();
#endif
    unload_qtlist();
    free_menu_coloring();
    free_invbuf();           /* let_to_name (invent.c) */
    free_youbuf();           /* You_buf,&c (pline.c) */
    msgtype_free();
    tmp_at(DISP_FREEMEM, 0); /* temporary display effects */
#ifdef FREE_ALL_MEMORY
#define freeobjchn(X) (saveobjchn(0, X, FREE_SAVE), X = 0)
#define freemonchn(X) (savemonchn(0, X, FREE_SAVE), X = 0)
#define freetrapchn(X) (savetrapchn(0, X, FREE_SAVE), X = 0)
#define freefruitchn() savefruitchn(0, FREE_SAVE)
#define freenames() savenames(0, FREE_SAVE)
#define free_killers() save_killers(0, FREE_SAVE)
#define free_oracles() save_oracles(0, FREE_SAVE)
#define free_waterlevel() save_waterlevel(0, FREE_SAVE)
#define free_worm() save_worm(0, FREE_SAVE)
#define free_timers(R) save_timers(0, FREE_SAVE, R)
#define free_light_sources(R) save_light_sources(0, FREE_SAVE, R);
#define free_engravings() save_engravings(0, FREE_SAVE)
#define freedamage() savedamage(0, FREE_SAVE)
#define free_animals() mon_animal_list(FALSE)

    /* move-specific data */
    dmonsfree(); /* release dead monsters */

    /* level-specific data */
    free_timers(RANGE_LEVEL);
    free_light_sources(RANGE_LEVEL);
    clear_regions();
    freemonchn(fmon);
    free_worm(); /* release worm segment information */
    freetrapchn(ftrap);
    freeobjchn(fobj);
    freeobjchn(level.buriedobjlist);
    freeobjchn(billobjs);
    free_engravings();
    freedamage();

    /* game-state data */
    free_killers();
    free_timers(RANGE_GLOBAL);
    free_light_sources(RANGE_GLOBAL);
    freeobjchn(invent);
    freeobjchn(migrating_objs);
    freemonchn(migrating_mons);
    freemonchn(mydogs); /* ascension or dungeon escape */
    /* freelevchn();  --  [folded into free_dungeons()] */
    free_animals();
    free_oracles();
    freefruitchn();
    freenames();
    free_waterlevel();
    free_dungeons();

    /* some pointers in iflags */
    if (iflags.wc_font_map)
        free(iflags.wc_font_map);
    if (iflags.wc_font_message)
        free(iflags.wc_font_message);
    if (iflags.wc_font_text)
        free(iflags.wc_font_text);
    if (iflags.wc_font_menu)
        free(iflags.wc_font_menu);
    if (iflags.wc_font_status)
        free(iflags.wc_font_status);
    if (iflags.wc_tile_file)
        free(iflags.wc_tile_file);
    free_autopickup_exceptions();

    /* miscellaneous */
    /* free_pickinv_cache();  --  now done from really_done()... */
    free_symsets();
#endif /* FREE_ALL_MEMORY */
#ifdef STATUS_HILITES
    status_finish();
#endif
#ifdef DUMPLOG
    dumplogfreemessages();
#endif

    /* last, because it frees data that might be used by panic() to provide
       feedback to the user; conceivably other freeing might trigger panic */
    sysopt_release(); /* SYSCF strings */
    return;
}

#ifdef MFLOPPY
boolean
swapin_file(int lev)
{
    char to[PATHLEN], from[PATHLEN];

    Sprintf(from, "%s%s", permbones, alllevels);
    Sprintf(to, "%s%s", levels, alllevels);
    set_levelfile_name(from, lev);
    set_levelfile_name(to, lev);
    if (iflags.checkspace) {
        while (level_info[lev].size > freediskspace(to))
            if (!swapout_oldest())
                return FALSE;
    }
    if (wizard) {
        pline("Swapping in `%s'.", from);
        wait_synch();
    }
    copyfile(from, to);
    (void) unlink(from);
    level_info[lev].where = ACTIVE;
    return TRUE;
}

STATIC_OVL boolean
swapout_oldest()
{
    char to[PATHLEN], from[PATHLEN];
    int i, oldest;
    long oldtime;

    if (!ramdisk)
        return FALSE;
    for (i = 1, oldtime = 0, oldest = 0; i <= maxledgerno(); i++)
        if (level_info[i].where == ACTIVE
            && (!oldtime || level_info[i].time < oldtime)) {
            oldest = i;
            oldtime = level_info[i].time;
        }
    if (!oldest)
        return FALSE;
    Sprintf(from, "%s%s", levels, alllevels);
    Sprintf(to, "%s%s", permbones, alllevels);
    set_levelfile_name(from, oldest);
    set_levelfile_name(to, oldest);
    if (wizard) {
        pline("Swapping out `%s'.", from);
        wait_synch();
    }
    copyfile(from, to);
    (void) unlink(from);
    level_info[oldest].where = SWAPPED;
    return TRUE;
}

STATIC_OVL void
copyfile(char *from, char *to)
{
#ifdef TOS
    if (_copyfile(from, to))
        panic("Can't copy %s to %s", from, to);
#else
    char buf[BUFSIZ]; /* this is system interaction, therefore
                       * BUFSIZ instead of NetHack's BUFSZ */
    int nfrom, nto, fdfrom, fdto;

    if ((fdfrom = open(from, O_RDONLY | O_BINARY, FCMASK)) < 0)
        panic("Can't copy from %s !?", from);
    if ((fdto = open(to, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, FCMASK)) < 0)
        panic("Can't copy to %s", to);
    do {
        nfrom = read(fdfrom, buf, BUFSIZ);
        nto = write(fdto, buf, nfrom);
        if (nto != nfrom)
            panic("Copyfile failed!");
    } while (nfrom == BUFSIZ);
    (void) nhclose(fdfrom);
    (void) nhclose(fdto);
#endif /* TOS */
}

/* see comment in bones.c */
void
co_false()
{
    count_only = FALSE;
    return;
}

#endif /* MFLOPPY */

/*save.c*/

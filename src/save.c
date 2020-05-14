/* NetHack 3.6	save.c	$NHDT-Date: 1581886866 2020/02/16 21:01:06 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.153 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

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

#if defined(UNIX) || defined(WIN32)
#define USE_BUFFERING
#endif

#ifdef MICRO
int dotcnt, dotrow; /* also used in restore */
#endif

static void FDECL(savelevchn, (NHFILE *));
static void FDECL(savedamage, (NHFILE *));
/* static void FDECL(saveobj, (NHFILE *,struct obj *)); */
/* static void FDECL(savemon, (NHFILE *,struct monst *)); */
/* static void FDECL(savelevl, (NHFILE *, BOOLEAN_P)); */
static void FDECL(saveobj, (NHFILE *,struct obj *));
static void FDECL(savemon, (NHFILE *,struct monst *));
static void FDECL(savelevl, (NHFILE *,BOOLEAN_P));
static void FDECL(saveobjchn, (NHFILE *,struct obj *));
static void FDECL(savemonchn, (NHFILE *,struct monst *));
static void FDECL(savetrapchn, (NHFILE *,struct trap *));
static void FDECL(savegamestate, (NHFILE *));
static void FDECL(save_msghistory, (NHFILE *));

#ifdef MFLOPPY
static void FDECL(savelev0, (NHFILE *, XCHAR_P, int));
static boolean NDECL(swapout_oldest);
static void FDECL(copyfile, (char *, char *));
#endif /* MFLOPPY */

#ifdef ZEROCOMP
static void FDECL(zerocomp_bufon, (int));
static void FDECL(zerocomp_bufoff, (int));
static void FDECL(zerocomp_bflush, (int));
static void FDECL(zerocomp_bwrite, (int, genericptr_t, unsigned int));
static void FDECL(zerocomp_bputc, (int));
#endif

#if defined(UNIX) || defined(VMS) || defined(__EMX__) || defined(WIN32)
#define HUP if (!g.program_state.done_hup)
#else
#define HUP
#endif

int
dosave()
{
    if (iflags.debug_fuzzer)
        return 0;
    clear_nhwindow(WIN_MESSAGE);
    if (yn("Really save?") == 'n') {
        clear_nhwindow(WIN_MESSAGE);
        if (g.multi > 0)
            nomul(0);
    } else {
        clear_nhwindow(WIN_MESSAGE);
        pline("Saving...");
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
        g.program_state.done_hup = 0;
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
    xchar ltmp;
    d_level uz_save;
    char whynot[BUFSZ];
    NHFILE *nhfp, *onhfp;

    /* we may get here via hangup signal, in which case we want to fix up
       a few of things before saving so that they won't be restored in
       an improper state; these will be no-ops for normal save sequence */
    u.uinvulnerable = 0;
    if (iflags.save_uswallow)
        u.uswallow = 1, iflags.save_uswallow = 0;
    if (iflags.save_uinwater)
        u.uinwater = 1, iflags.save_uinwater = 0; /* bypass set_uinwater() */
    if (iflags.save_uburied)
        u.uburied = 1, iflags.save_uburied = 0;

    if (!g.program_state.something_worth_saving || !g.SAVEF[0])
        return 0;
    fq_save = fqname(g.SAVEF, SAVEPREFIX, 1); /* level files take 0 */

#if defined(UNIX) || defined(VMS)
    sethanguphandler((void FDECL((*), (int) )) SIG_IGN);
#endif
#ifndef NO_SIGNAL
    (void) signal(SIGINT, SIG_IGN);
#endif

#if defined(MICRO) && defined(MFLOPPY)
    if (!saveDiskPrompt(0))
        return 0;
#endif

    HUP if (iflags.window_inited) {
        nh_uncompress(fq_save);
        nhfp = open_savefile();
        if (nhfp) {
            close_nhfile(nhfp);
            clear_nhwindow(WIN_MESSAGE);
            There("seems to be an old save file.");
            if (yn("Overwrite the old file?") == 'n') {
                nh_compress(fq_save);
                return 0;
            }
        }
    }

    HUP mark_synch(); /* flush any buffered screen output */

    nhfp = create_savefile();
    if (!nhfp) {
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
    if (!WINDOWPORT("X11"))
        putstr(WIN_MAP, 0, "Saving:");
#endif
#ifdef MFLOPPY
    /* make sure there is enough disk space */
    if (iflags.checkspace) {
        long fds, needed;

        nhfp->mode = COUNTING;
        savelev(nhfp, ledger_no(&u.uz));
        savegamestate(nhfp);
        needed = bytes_counted;

        for (ltmp = 1; ltmp <= maxledgerno(); ltmp++)
            if (ltmp != ledger_no(&u.uz) && g.level_info[ltmp].where)
                needed += g.level_info[ltmp].size + (sizeof ltmp);
        fds = freediskspace(fq_save);
        if (needed > fds) {
            HUP
            {
                There("is insufficient space on SAVE disk.");
                pline("Require %ld bytes but only have %ld.", needed, fds);
            }
            flushout();
            close_nhfile(nhfp);
            (void) delete_savefile();
            return 0;
        }

        co_false();
    }
#endif /* MFLOPPY */

    nhfp->mode = WRITING | FREEING;
    store_version(nhfp);
    store_savefileinfo(nhfp);
    if (nhfp && nhfp->fplog)
        (void) fprintf(nhfp->fplog, "# post-validation\n");
    store_plname_in_file(nhfp);
    g.ustuck_id = (u.ustuck ? u.ustuck->m_id : 0);
    g.usteed_id = (u.usteed ? u.usteed->m_id : 0);
    savelev(nhfp, ledger_no(&u.uz));
    savegamestate(nhfp);

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
    set_ustuck((struct monst *) 0);
    u.usteed = (struct monst *) 0;

    for (ltmp = (xchar) 1; ltmp <= maxledgerno(); ltmp++) {
        if (ltmp == ledger_no(&uz_save))
            continue;
        if (!(g.level_info[ltmp].flags & LFILE_EXISTS))
            continue;
#ifdef MICRO
        curs(WIN_MAP, 1 + dotcnt++, dotrow);
        if (dotcnt >= (COLNO - 1)) {
            dotrow++;
            dotcnt = 0;
        }
        if (!WINDOWPORT("X11")) {
            putstr(WIN_MAP, 0, ".");
        }
        mark_synch();
#endif
        onhfp = open_levelfile(ltmp, whynot);
        if (!onhfp) {
            HUP pline1(whynot);
            close_nhfile(nhfp);
            (void) delete_savefile();
            HUP Strcpy(g.killer.name, whynot);
            HUP done(TRICKED);
            return 0;
        }
        minit(); /* ZEROCOMP */
        getlev(onhfp, g.hackpid, ltmp);
        close_nhfile(onhfp);
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &ltmp, sizeof ltmp); /* level number*/
        savelev(nhfp, ltmp);     /* actual level*/
        delete_levelfile(ltmp);
    }
    close_nhfile(nhfp);

    u.uz = uz_save;

    /* get rid of current level --jgm */
    delete_levelfile(ledger_no(&u.uz));
    delete_levelfile(0);
    nh_compress(fq_save);
    /* this should probably come sooner... */
    g.program_state.something_worth_saving = 0;
    return 1;
}

static void
savegamestate(nhfp)
NHFILE *nhfp;
{
    unsigned long uid;
    struct obj * bc_objs = (struct obj *)0;

#ifdef MFLOPPY
    count_only = (nhfp->mode & COUNTING);
#endif
    uid = (unsigned long) getuid();
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) &uid, sizeof uid);
        bwrite(nhfp->fd, (genericptr_t) &g.context, sizeof g.context);
        bwrite(nhfp->fd, (genericptr_t) &flags, sizeof flags);
#ifdef SYSFLAGS
        bwrite(nhfp->fd, (genericptr_t) &sysflags, sysflags);
#endif
    }
    urealtime.finish_time = getnow();
    urealtime.realtime += (long) (urealtime.finish_time
                                    - urealtime.start_timing);
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) &u, sizeof u);
        bwrite(nhfp->fd, yyyymmddhhmmss(ubirthday), 14);
        bwrite(nhfp->fd, (genericptr_t) &urealtime.realtime,
               sizeof urealtime.realtime);
        bwrite(nhfp->fd, yyyymmddhhmmss(urealtime.start_timing), 14);
    }
    /* this is the value to use for the next update of urealtime.realtime */
    urealtime.start_timing = urealtime.finish_time;
    save_killers(nhfp);

    /* must come before g.migrating_objs and g.migrating_mons are freed */
    save_timers(nhfp, RANGE_GLOBAL);
    save_light_sources(nhfp, RANGE_GLOBAL);

    saveobjchn(nhfp, g.invent);

    /* save ball and chain if they are currently dangling free (i.e. not on
       floor or in inventory) */
    if (CHAIN_IN_MON) {
        uchain->nobj = bc_objs;
        bc_objs = uchain;
    }
    if (BALL_IN_MON) {
        uball->nobj = bc_objs;
        bc_objs = uball;
    }
    saveobjchn(nhfp, bc_objs);

    saveobjchn(nhfp, g.migrating_objs);
    savemonchn(nhfp, g.migrating_mons);
    if (release_data(nhfp)) {
        g.invent = 0;
        g.migrating_objs = 0;
        g.migrating_mons = 0;
    }
    if (nhfp->structlevel)
        bwrite(nhfp->fd, (genericptr_t) g.mvitals, sizeof g.mvitals);
    save_dungeon(nhfp, (boolean) !!perform_bwrite(nhfp),
                 (boolean) !!release_data(nhfp));
    savelevchn(nhfp);
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) &g.moves, sizeof g.moves);
        bwrite(nhfp->fd, (genericptr_t) &g.monstermoves, sizeof g.monstermoves);
        bwrite(nhfp->fd, (genericptr_t) &g.quest_status, sizeof g.quest_status);
        bwrite(nhfp->fd, (genericptr_t) g.spl_book,
               sizeof (struct spell) * (MAXSPELL + 1));
    }
    save_artifacts(nhfp);
    save_oracles(nhfp);
    if (g.ustuck_id) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &g.ustuck_id, sizeof g.ustuck_id);
    }
    if (g.usteed_id) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &g.usteed_id, sizeof g.usteed_id);
    }
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) g.pl_character, sizeof g.pl_character);
        bwrite(nhfp->fd, (genericptr_t) g.pl_fruit, sizeof g.pl_fruit);
    }
    savefruitchn(nhfp);
    savenames(nhfp);
    save_waterlevel(nhfp);
    save_msghistory(nhfp);
    if (nhfp->structlevel)
        bflush(nhfp->fd);
}

boolean
tricked_fileremoved(nhfp, whynot)
NHFILE *nhfp;
char *whynot;
{
    if (!nhfp) {
        pline1(whynot);
        pline("Probably someone removed it.");
        Strcpy(g.killer.name, whynot);
        done(TRICKED);
        return TRUE;
    }
    return FALSE;
}

#ifdef INSURANCE
void
savestateinlock()
{
    int hpid = 0;
    char whynot[BUFSZ];
    NHFILE *nhfp;

    /* When checkpointing is on, the full state needs to be written
     * on each checkpoint.  When checkpointing is off, only the pid
     * needs to be in the level.0 file, so it does not need to be
     * constantly rewritten.  When checkpointing is turned off during
     * a game, however, the file has to be rewritten once to truncate
     * it and avoid restoring from outdated information.
     *
     * Restricting g.havestate to this routine means that an additional
     * noop pid rewriting will take place on the first "checkpoint" after
     * the game is started or restored, if checkpointing is off.
     */
    if (flags.ins_chkpt || g.havestate) {
        /* save the rest of the current game state in the lock file,
         * following the original int pid, the current level number,
         * and the current savefile name, which should not be subject
         * to any internal compression schemes since they must be
         * readable by an external utility
         */
        nhfp = open_levelfile(0, whynot);
        if (tricked_fileremoved(nhfp, whynot))
            return;

        if (nhfp->structlevel)
            (void) read(nhfp->fd, (genericptr_t) &hpid, sizeof hpid);
        if (g.hackpid != hpid) {
            Sprintf(whynot, "Level #0 pid (%d) doesn't match ours (%d)!",
                    hpid, g.hackpid);
            pline1(whynot);
            Strcpy(g.killer.name, whynot);
            done(TRICKED);
        }
        close_nhfile(nhfp);

        nhfp = create_levelfile(0, whynot);
        if (!nhfp) {
            pline1(whynot);
            Strcpy(g.killer.name, whynot);
            done(TRICKED);
            return;
        }
        nhfp->mode = WRITING;
        if (nhfp->structlevel)
            (void) write(nhfp->fd, (genericptr_t) &g.hackpid, sizeof g.hackpid);
        if (flags.ins_chkpt) {
            int currlev = ledger_no(&u.uz);

            if (nhfp->structlevel)
                (void) write(nhfp->fd, (genericptr_t) &currlev, sizeof currlev);
            save_savefile_name(nhfp);
            store_version(nhfp);
            store_savefileinfo(nhfp);
            store_plname_in_file(nhfp);

            g.ustuck_id = (u.ustuck ? u.ustuck->m_id : 0);
            g.usteed_id = (u.usteed ? u.usteed->m_id : 0);
            savegamestate(nhfp);
        }
        close_nhfile(nhfp);
    }
    g.havestate = flags.ins_chkpt;
}
#endif

#ifdef MFLOPPY
boolean
savelev(nhfp, lev)
NHFILE *nhfp;
xchar lev;
{
    if (nhfp->mode & COUNTING) {
        int savemode = nhfp->mode;

        bytes_counted = 0;
        savelev0(nhfp, lev);
        /* probably bytes_counted will be filled in again by an
         * immediately following WRITE_SAVE anyway, but we'll
         * leave it out of checkspace just in case */
        if (iflags.checkspace) {
            while (bytes_counted > freediskspace(levels))
                if (!swapout_oldest())
                    return FALSE;
        }
    }
    if (nhfp->mode & (WRITING | FREEING)) {
        bytes_counted = 0;
        savelev0(nhfp, lev);
    }
    if (nhfp->mode != FREEING) {
        g.level_info[lev].where = ACTIVE;
        g.level_info[lev].time = g.moves;
        g.level_info[lev].size = bytes_counted;
    }
    return TRUE;
}

static void
savelev0(nhfp, lev)
#else
void
savelev(nhfp, lev)
#endif
NHFILE *nhfp;
xchar lev;
{
#ifdef TOS
    short tlev;
#endif

    /*
     *  Level file contents:
     *    version info (handled by caller);
     *    save file info (compression type; also by caller);
     *    process ID;
     *    internal level number (ledger number);
     *    bones info;
     *    actual level data.
     *
     *  If we're tearing down the current level without saving anything
     *  (which happens at end of game or upon entrance to endgame or
     *  after an aborted restore attempt) then we don't want to do any
     *  actual I/O.  So when only freeing, we skip to the bones info
     *  portion (which has some freeing to do), then jump quite a bit
     *  further ahead to the middle of the 'actual level data' portion.
     */
    if (nhfp->mode != FREEING) {
        /* WRITING (probably ORed with FREEING), or COUNTING */

        /* purge any dead monsters (necessary if we're starting
           a panic save rather than a normal one, or sometimes
           when changing levels without taking time -- e.g.
           create statue trap then immediately level teleport) */
        if (iflags.purge_monsters)
            dmonsfree();

        if (!nhfp)
            panic("Save on bad file!"); /* impossible */
#ifdef MFLOPPY
        count_only = (nhfp->mode & COUNTING);
#endif
        if (lev >= 0 && lev <= maxledgerno())
            g.level_info[lev].flags |= VISITED;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &g.hackpid, sizeof g.hackpid);
#ifdef TOS
        tlev = lev;
        tlev &= 0x00ff;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &tlev, sizeof tlev);
#else
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &lev, sizeof lev);
#endif
    }

    /* bones info comes before level data; the intent is for an external
       program ('hearse') to be able to match a bones file with the
       corresponding log file entry--or perhaps just skip that?--without
       the guessing that was needed in 3.4.3 and without having to
       interpret level data to find where to start; unfortunately it
       still needs to handle all the data compression schemes */
    savecemetery(nhfp, &g.level.bonesinfo);
    if (nhfp->mode == FREEING) /* see above */
        goto skip_lots;

    savelevl(nhfp, (boolean) ((sfsaveinfo.sfi1 & SFI1_RLECOMP) == SFI1_RLECOMP));
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) g.lastseentyp, sizeof g.lastseentyp);
        bwrite(nhfp->fd, (genericptr_t) &g.monstermoves, sizeof g.monstermoves);
        bwrite(nhfp->fd, (genericptr_t) &g.upstair, sizeof (stairway));
        bwrite(nhfp->fd, (genericptr_t) &g.dnstair, sizeof (stairway));
        bwrite(nhfp->fd, (genericptr_t) &g.upladder, sizeof (stairway));
        bwrite(nhfp->fd, (genericptr_t) &g.dnladder, sizeof (stairway));
        bwrite(nhfp->fd, (genericptr_t) &g.sstairs, sizeof (stairway));
        bwrite(nhfp->fd, (genericptr_t) &g.updest, sizeof (dest_area));
        bwrite(nhfp->fd, (genericptr_t) &g.dndest, sizeof (dest_area));
        bwrite(nhfp->fd, (genericptr_t) &g.level.flags, sizeof g.level.flags);
        bwrite(nhfp->fd, (genericptr_t) g.doors, sizeof g.doors);
    }
    save_rooms(nhfp); /* no dynamic memory to reclaim */

    /* from here on out, saving also involves allocated memory cleanup */
 skip_lots:
    /* timers and lights must be saved before monsters and objects */
    save_timers(nhfp, RANGE_LEVEL);
    save_light_sources(nhfp, RANGE_LEVEL);

    savemonchn(nhfp, fmon);
    save_worm(nhfp); /* save worm information */
    savetrapchn(nhfp, g.ftrap);
    saveobjchn(nhfp, fobj);
    saveobjchn(nhfp, g.level.buriedobjlist);
    saveobjchn(nhfp, g.billobjs);
    if (release_data(nhfp)) {
        int x,y;
        /* TODO: maybe use clear_level_structures() */
        for (y = 0; y < ROWNO; y++)
            for (x = 0; x < COLNO; x++) {
                g.level.monsters[x][y] = 0;
                g.level.objects[x][y] = 0;
                levl[x][y].seenv = 0;
                levl[x][y].glyph = GLYPH_UNEXPLORED;
            }
        fmon = 0;
        g.ftrap = 0;
        fobj = 0;
        g.level.buriedobjlist = 0;
        g.billobjs = 0;
        /* level.bonesinfo = 0; -- handled by savecemetery() */
    }
    save_engravings(nhfp);
    savedamage(nhfp); /* pending shop wall and/or floor repair */
    save_regions(nhfp);
    if (nhfp->mode != FREEING) {
        if (nhfp->structlevel)
            bflush(nhfp->fd);
    }
}

static void
savelevl(nhfp, rlecomp)
NHFILE *nhfp;
boolean rlecomp;
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
                    /* run has been broken, write out run-length encoding */
 writeout:
                    if (nhfp->structlevel) {
                        bwrite(nhfp->fd, (genericptr_t) &match, sizeof match);
                        bwrite(nhfp->fd, (genericptr_t) rgrm, sizeof *rgrm);
                    }
                    /* start encoding again. we have at least 1 rm
                       in the next run, viz. this one. */
                    match = 1;
                    rgrm = prm;
                }
            }
        }
        if (match > 0) {
            if (nhfp->structlevel) {
                bwrite(nhfp->fd, (genericptr_t) &match, sizeof (uchar));
                bwrite(nhfp->fd, (genericptr_t) rgrm, sizeof (struct rm));
            }
        }
        return;
    }
#else /* !RLECOMP */
    nhUse(rlecomp);
#endif /* ?RLECOMP */
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) levl, sizeof levl);
    }
}

/* used when saving a level and also when saving dungeon overview data */
void
savecemetery(nhfp, cemeteryaddr)
NHFILE *nhfp;
struct cemetery **cemeteryaddr;
{
    struct cemetery *thisbones, *nextbones;
    int flag;

    flag = *cemeteryaddr ? 0 : -1;
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &flag, sizeof flag);
    }
    nextbones = *cemeteryaddr;
    while ((thisbones = nextbones) != 0) {
        nextbones = thisbones->next;
        if (perform_bwrite(nhfp)) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) thisbones, sizeof *thisbones);
        }
        if (release_data(nhfp))
            free((genericptr_t) thisbones);
    }
    if (release_data(nhfp))
        *cemeteryaddr = 0;
}

static void
savedamage(nhfp)
NHFILE *nhfp;
{
    register struct damage *damageptr, *tmp_dam;
    unsigned int xl = 0;

    damageptr = g.level.damagelist;
    for (tmp_dam = damageptr; tmp_dam; tmp_dam = tmp_dam->next)
        xl++;
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &xl, sizeof xl);
    }
    while (xl--) {
        if (perform_bwrite(nhfp)) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) damageptr, sizeof *damageptr);
        }
        tmp_dam = damageptr;
        damageptr = damageptr->next;
        if (release_data(nhfp))
            free((genericptr_t) tmp_dam);
    }
    if (release_data(nhfp))
        g.level.damagelist = 0;
}

static void
saveobj(nhfp, otmp)
NHFILE *nhfp;
struct obj *otmp;
{
    int buflen, zerobuf = 0;

    buflen = (int) sizeof (struct obj);
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        bwrite(nhfp->fd, (genericptr_t) otmp, buflen);
    }
    if (otmp->oextra) {
        buflen = ONAME(otmp) ? (int) strlen(ONAME(otmp)) + 1 : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);

        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) ONAME(otmp), buflen);
        }
        /* defer to savemon() for this one */
        if (OMONST(otmp)) {
            savemon(nhfp, OMONST(otmp));
        } else {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) &zerobuf, sizeof zerobuf);
        }
        /* extra info about scroll of mail */
        buflen = OMAILCMD(otmp) ? (int) strlen(OMAILCMD(otmp)) + 1 : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (buflen > 0) {
            if (nhfp->structlevel)
                  bwrite(nhfp->fd, (genericptr_t) OMAILCMD(otmp), buflen);
        }
        /* omid used to be indirect via a pointer in oextra but has
           become part of oextra itself; 0 means not applicable and
           gets saved/restored whenever any other oxtra components do */
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &OMID(otmp), sizeof OMID(otmp));
    }
}

static void
saveobjchn(nhfp, otmp)
NHFILE *nhfp;
register struct obj *otmp;
{
    register struct obj *otmp2;
    int minusone = -1;

    while (otmp) {
        otmp2 = otmp->nobj;
        if (perform_bwrite(nhfp)) {
            saveobj(nhfp, otmp);
        }
        if (Has_contents(otmp))
            saveobjchn(nhfp, otmp->cobj);
        if (release_data(nhfp)) {
            /*
             * If these are on the floor, the discarding could be
             * due to game save, or we could just be changing levels.
             * Always invalidate the pointer, but ensure that we have
             * the o_id in order to restore the pointer on reload.
             */
            if (otmp == g.context.victual.piece) {
                g.context.victual.o_id = otmp->o_id;
                g.context.victual.piece = (struct obj *) 0;
            }
            if (otmp == g.context.tin.tin) {
                g.context.tin.o_id = otmp->o_id;
                g.context.tin.tin = (struct obj *) 0;
            }
            if (otmp == g.context.spbook.book) {
                g.context.spbook.o_id = otmp->o_id;
                g.context.spbook.book = (struct obj *) 0;
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
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &minusone, sizeof (int));
    }
}

static void
savemon(nhfp, mtmp)
NHFILE *nhfp;
struct monst *mtmp;
{
    int buflen;

    mtmp->mtemplit = 0; /* normally clear; if set here then a panic save
                         * is being written while bhit() was executing */
    buflen = (int) sizeof (struct monst);
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        bwrite(nhfp->fd, (genericptr_t) mtmp, buflen);
    }
    if (mtmp->mextra) {
        buflen = MNAME(mtmp) ? (int) strlen(MNAME(mtmp)) + 1 : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) MNAME(mtmp), buflen);
        }
        buflen = EGD(mtmp) ? (int) sizeof (struct egd) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) EGD(mtmp), buflen);
        }
        buflen = EPRI(mtmp) ? (int) sizeof (struct epri) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) EPRI(mtmp), buflen);
        }
        buflen = ESHK(mtmp) ? (int) sizeof (struct eshk) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof (int));
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) ESHK(mtmp), buflen);
        }
        buflen = EMIN(mtmp) ? (int) sizeof (struct emin) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof (int));
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) EMIN(mtmp), buflen);
        }
        buflen = EDOG(mtmp) ? (int) sizeof (struct edog) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof (int));
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) EDOG(mtmp), buflen);
        }
        /* mcorpsenm is inline int rather than pointer to something,
           so doesn't need to be preceded by a length field */
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &MCORPSENM(mtmp),
                   sizeof MCORPSENM(mtmp));
    }
}

static void
savemonchn(nhfp, mtmp)
NHFILE *nhfp;
register struct monst *mtmp;
{
    register struct monst *mtmp2;
    int minusone = -1;

    while (mtmp) {
        mtmp2 = mtmp->nmon;
        if (perform_bwrite(nhfp)) {
            mtmp->mnum = monsndx(mtmp->data);
            if (mtmp->ispriest)
                forget_temple_entry(mtmp); /* EPRI() */
            savemon(nhfp, mtmp);
        }
        if (mtmp->minvent)
            saveobjchn(nhfp, mtmp->minvent);
        if (release_data(nhfp)) {
            if (mtmp == g.context.polearm.hitmon) {
                g.context.polearm.m_id = mtmp->m_id;
                g.context.polearm.hitmon = NULL;
            }
            mtmp->nmon = NULL;  /* nmon saved into mtmp2 */
            dealloc_monst(mtmp);
        }
        mtmp = mtmp2;
    }
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &minusone, sizeof (int));
    }
}

/* save traps; g.ftrap is the only trap chain so the 2nd arg is superfluous */
static void
savetrapchn(nhfp, trap)
NHFILE *nhfp;
register struct trap *trap;
{
    static struct trap zerotrap;
    register struct trap *trap2;

    while (trap) {
        trap2 = trap->ntrap;
        if (perform_bwrite(nhfp)) {
            if (nhfp->structlevel)  
                bwrite(nhfp->fd, (genericptr_t) trap, sizeof *trap);
        }
        if (release_data(nhfp))
            dealloc_trap(trap);
        trap = trap2;
    }
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &zerotrap, sizeof zerotrap);
    }
}

/* save all the fruit names and ID's; this is used only in saving whole games
 * (not levels) and in saving bones levels.  When saving a bones level,
 * we only want to save the fruits which exist on the bones level; the bones
 * level routine marks nonexistent fruits by making the fid negative.
 */
void
savefruitchn(nhfp)
NHFILE *nhfp;
{
    static struct fruit zerofruit;
    register struct fruit *f2, *f1;

    f1 = g.ffruit;
    while (f1) {
        f2 = f1->nextf;
        if (f1->fid >= 0 && perform_bwrite(nhfp)) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) f1, sizeof *f1);
        }
        if (release_data(nhfp))
            dealloc_fruit(f1);
        f1 = f2;
    }
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &zerofruit, sizeof zerofruit);
    }
    if (release_data(nhfp))
        g.ffruit = 0;
}



static void
savelevchn(nhfp)
NHFILE *nhfp;
{
    s_level *tmplev, *tmplev2;
    int cnt = 0;

    for (tmplev = g.sp_levchn; tmplev; tmplev = tmplev->next)
        cnt++;
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &cnt, sizeof cnt);
    }
    for (tmplev = g.sp_levchn; tmplev; tmplev = tmplev2) {
        tmplev2 = tmplev->next;
        if (perform_bwrite(nhfp)) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) tmplev, sizeof *tmplev);
        }
        if (release_data(nhfp))
            free((genericptr_t) tmplev);
    }
    if (release_data(nhfp))
        g.sp_levchn = 0;
}

void
store_plname_in_file(nhfp)
NHFILE *nhfp;
{
    int plsiztmp = PL_NSIZ;

    if (nhfp->structlevel) {
        bufoff(nhfp->fd);
        /* bwrite() before bufon() uses plain write() */
        bwrite(nhfp->fd, (genericptr_t) &plsiztmp, sizeof plsiztmp);
        bwrite(nhfp->fd, (genericptr_t) g.plname, plsiztmp);
        bufon(nhfp->fd);
    }
    return;
}

static void
save_msghistory(nhfp)
NHFILE *nhfp;
{
    char *msg;
    int msgcount = 0, msglen;
    int minusone = -1;
    boolean init = TRUE;

    if (perform_bwrite(nhfp)) {
        /* ask window port for each message in sequence */
        while ((msg = getmsghistory(init)) != 0) {
            init = FALSE;
            msglen = strlen(msg);
            if (msglen < 1)
                continue;
            /* sanity: truncate if necessary (shouldn't happen);
               no need to modify msg[] since terminator isn't written */
            if (msglen > BUFSZ - 1)
                msglen = BUFSZ - 1;
            if (nhfp->structlevel) {
                bwrite(nhfp->fd, (genericptr_t) &msglen, sizeof msglen);
                bwrite(nhfp->fd, (genericptr_t) msg, msglen);
            }
            ++msgcount;
        }
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &minusone, sizeof (int));
    }
    debugpline1("Stored %d messages into savefile.", msgcount);
    /* note: we don't attempt to handle release_data() here */
}

void
store_savefileinfo(nhfp)
NHFILE *nhfp;
{
    /* sfcap (decl.c) describes the savefile feature capabilities
     * that are supported by this port/platform build.
     *
     * sfsaveinfo (decl.c) describes the savefile info that actually
     * gets written into the savefile, and is used to determine the
     * save file being written.
     *
     * sfrestinfo (decl.c) describes the savefile info that is
     * being used to read the information from an existing savefile.
     */

    if (nhfp->structlevel) {
        bufoff(nhfp->fd);
        /* bwrite() before bufon() uses plain write() */
        bwrite(nhfp->fd, (genericptr_t) &sfsaveinfo, (unsigned) sizeof sfsaveinfo);
        bufon(nhfp->fd);
    }
    return;
}

/* also called by prscore(); this probably belongs in dungeon.c... */
void
free_dungeons()
{
#ifdef FREE_ALL_MEMORY
    NHFILE tnhfp;

    zero_nhfile(&tnhfp);    /* also sets fd to -1 */
    tnhfp.mode = FREEING;
    savelevchn(&tnhfp);
    save_dungeon(&tnhfp, FALSE, TRUE);
    free_luathemes(TRUE);
#endif
    return;
}

void
freedynamicdata()
{
    NHFILE tnhfp;

#if defined(UNIX) && defined(MAIL)
    free_maildata();
#endif
    zero_nhfile(&tnhfp);    /* also sets fd to -1 */
    tnhfp.mode = FREEING;
    free_menu_coloring();
    free_invbuf();           /* let_to_name (invent.c) */
    free_youbuf();           /* You_buf,&c (pline.c) */
    msgtype_free();
    tmp_at(DISP_FREEMEM, 0); /* temporary display effects */
#ifdef FREE_ALL_MEMORY
#define free_current_level() savelev(&tnhfp, -1)
#define freeobjchn(X) (saveobjchn(&tnhfp, X), X = 0)
#define freemonchn(X) (savemonchn(&tnhfp, X), X = 0)
#define freefruitchn() savefruitchn(&tnhfp)
#define freenames() savenames(&tnhfp)
#define free_killers() save_killers(&tnhfp)
#define free_oracles() save_oracles(&tnhfp)
#define free_waterlevel() save_waterlevel(&tnhfp)
#define free_timers(R) save_timers(&tnhfp, R)
#define free_light_sources(R) save_light_sources(&tnhfp, R)
#define free_animals() mon_animal_list(FALSE)

    /* move-specific data */
    dmonsfree(); /* release dead monsters */

    /* level-specific data */
    free_current_level();

    /* game-state data [ought to reorganize savegamestate() to handle this] */
    free_killers();
    free_timers(RANGE_GLOBAL);
    free_light_sources(RANGE_GLOBAL);
    freeobjchn(g.invent);
    freeobjchn(g.migrating_objs);
    freemonchn(g.migrating_mons);
    freemonchn(g.mydogs); /* ascension or dungeon escape */
    /* freelevchn();  --  [folded into free_dungeons()] */
    free_animals();
    free_oracles();
    freefruitchn();
    freenames();
    free_waterlevel();
    free_dungeons();

    /* some pointers in iflags */
    if (iflags.wc_font_map)
        free((genericptr_t) iflags.wc_font_map), iflags.wc_font_map = 0;
    if (iflags.wc_font_message)
        free((genericptr_t) iflags.wc_font_message),
        iflags.wc_font_message = 0;
    if (iflags.wc_font_text)
        free((genericptr_t) iflags.wc_font_text), iflags.wc_font_text = 0;
    if (iflags.wc_font_menu)
        free((genericptr_t) iflags.wc_font_menu), iflags.wc_font_menu = 0;
    if (iflags.wc_font_status)
        free((genericptr_t) iflags.wc_font_status), iflags.wc_font_status = 0;
    if (iflags.wc_tile_file)
        free((genericptr_t) iflags.wc_tile_file), iflags.wc_tile_file = 0;
    free_autopickup_exceptions();

    /* miscellaneous */
    /* free_pickinv_cache();  --  now done from really_done()... */
    free_symsets();
#endif /* FREE_ALL_MEMORY */
    if (VIA_WINDOWPORT())
        status_finish();
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
swapin_file(lev)
int lev;
{
    char to[PATHLEN], from[PATHLEN];

    Sprintf(from, "%s%s", g.permbones, g.alllevels);
    Sprintf(to, "%s%s", levels, g.alllevels);
    set_levelfile_name(from, lev);
    set_levelfile_name(to, lev);
    if (iflags.checkspace) {
        while (g.level_info[lev].size > freediskspace(to))
            if (!swapout_oldest())
                return FALSE;
    }
    if (wizard) {
        pline("Swapping in `%s'.", from);
        wait_synch();
    }
    copyfile(from, to);
    (void) unlink(from);
    g.level_info[lev].where = ACTIVE;
    return TRUE;
}

static boolean
swapout_oldest()
{
    char to[PATHLEN], from[PATHLEN];
    int i, oldest;
    long oldtime;

    if (!g.ramdisk)
        return FALSE;
    for (i = 1, oldtime = 0, oldest = 0; i <= maxledgerno(); i++)
        if (g.level_info[i].where == ACTIVE
            && (!oldtime || g.level_info[i].time < oldtime)) {
            oldest = i;
            oldtime = g.level_info[i].time;
        }
    if (!oldest)
        return FALSE;
    Sprintf(from, "%s%s", levels, g.alllevels);
    Sprintf(to, "%s%s", g.permbones, g.alllevels);
    set_levelfile_name(from, oldest);
    set_levelfile_name(to, oldest);
    if (wizard) {
        pline("Swapping out `%s'.", from);
        wait_synch();
    }
    copyfile(from, to);
    (void) unlink(from);
    g.level_info[oldest].where = SWAPPED;
    return TRUE;
}

static void
copyfile(from, to)
char *from, *to;
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

/* NetHack 3.6	save.c	$NHDT-Date: 1559994625 2019/06/08 11:50:25 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.121 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"
#include "sfproto.h"


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

/*SAVE2018*/
extern void FDECL(nhout, (int, const char *,
                    const char *, int, genericptr_t, int));
extern int NDECL(nhdatatypes_size);

#ifdef MICRO
int dotcnt, dotrow; /* also used in restore */
#endif

STATIC_DCL void FDECL(savelevchn, (NHFILE *));
STATIC_DCL void FDECL(savedamage, (NHFILE *));
/* STATIC_DCL void FDECL(saveobj, (NHFILE *,struct obj *)); */
/* STATIC_DCL void FDECL(savemon, (NHFILE *,struct monst *)); */
/* STATIC_DCL void FDECL(savelevl, (NHFILE *, BOOLEAN_P)); */
STATIC_DCL void FDECL(saveobj, (NHFILE *,struct obj *));
STATIC_DCL void FDECL(savemon, (NHFILE *,struct monst *));
STATIC_DCL void FDECL(savelevl, (NHFILE *,BOOLEAN_P));
STATIC_DCL void FDECL(saveobjchn, (NHFILE *,struct obj *));
STATIC_DCL void FDECL(savemonchn, (NHFILE *,struct monst *));
STATIC_DCL void FDECL(savetrapchn, (NHFILE *,struct trap *));
STATIC_DCL void FDECL(savegamestate, (NHFILE *));
STATIC_OVL void FDECL(save_msghistory, (NHFILE *));

#ifdef MFLOPPY
STATIC_DCL void FDECL(savelev0, (NHFILE *, XCHAR_P, int));
STATIC_DCL boolean NDECL(swapout_oldest);
STATIC_DCL void FDECL(copyfile, (char *, char *));
#endif /* MFLOPPY */

#ifdef ZEROCOMP
STATIC_DCL void FDECL(zerocomp_bufon, (int));
STATIC_DCL void FDECL(zerocomp_bufoff, (int));
STATIC_DCL void FDECL(zerocomp_bflush, (int));
STATIC_DCL void FDECL(zerocomp_bwrite, (int, genericptr_t, unsigned int));
STATIC_DCL void FDECL(zerocomp_bputc, (int));
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
        u.uinwater = 1, iflags.save_uinwater = 0;
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
#ifdef SAVEFILE_DEBUGGING
    if (nhfp && nhfp->fieldlevel && nhfp->fplog)
        (void) fprintf(nhfp->fplog, "# just opened\n");
#endif

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

    if (nhfp->fieldlevel && nhfp->addinfo && (nhfp->mode & WRITING))
        sfo_addinfo(nhfp, "NetHack", "start", "savefile", 0);

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
    u.ustuck = (struct monst *) 0;
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
        getlev(onhfp, g.hackpid, ltmp, FALSE);
        close_nhfile(onhfp);
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &ltmp, sizeof ltmp); /* level number*/
        if (nhfp->fieldlevel)
            sfo_xchar(nhfp, &ltmp, "gamestate", "level_number", 1);         /* xchar */
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

STATIC_OVL void
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
    if (nhfp->fieldlevel) {
        sfo_ulong(nhfp, &uid, "gamestate", "uid", 1);
        sfo_context_info(nhfp, &g.context, "gamestate", "g.context", 1);
        sfo_flag(nhfp, &flags, "gamestate" , "flags", 1);
#ifdef SYSFLAGS
        sfo_flag(nhfp, &sysflags, "gamestate" , "sysflags", 1);
#endif
    }
    urealtime.finish_time = getnow();
    urealtime.realtime += (long) (urealtime.finish_time
                                    - urealtime.start_timing);

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
    if (nhfp->fieldlevel) {
        sfo_you(nhfp, &u, "gamestate", "you", 1);
        sfo_str(nhfp, yyyymmddhhmmss(ubirthday), "gamestate", "ubirthday", 14);
        sfo_long(nhfp, &urealtime.realtime, "gamestate", "realtime", 1);
        sfo_str(nhfp, yyyymmddhhmmss(urealtime.start_timing), "gamestate", "start_timing", 14);
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
    if (nhfp->fieldlevel) {
        int i;

        for (i = 0; i < NUMMONS; ++i)
            sfo_mvitals(nhfp, &g.mvitals[i], "gamestate", "g.mvitals", 1);
    }            
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
    if (nhfp->fieldlevel) {
        int i;
        struct spell *sptmp;

        sfo_long(nhfp, &g.moves, "gamestate", "g.moves", 1);
        sfo_long(nhfp, &g.monstermoves, "gamestate", "g.monstermoves", 1);
        sfo_q_score(nhfp, &g.quest_status, "gamestate", "g.quest_status", 1);
        sptmp = g.spl_book;
        for (i = 0; i < (MAXSPELL + 1); ++i)
            sfo_spell(nhfp, sptmp++, "gamestate", "g.spl_book", 1);
    }
    save_artifacts(nhfp);
    save_oracles(nhfp);
    if (g.ustuck_id) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &g.ustuck_id, sizeof g.ustuck_id);
        if (nhfp->fieldlevel)
            sfo_unsigned(nhfp, &g.ustuck_id, "gamestate", "g.ustuck_id", 1);
    }
    if (g.usteed_id) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &g.usteed_id, sizeof g.usteed_id);
        if (nhfp->fieldlevel)
            sfo_unsigned(nhfp, &g.usteed_id, "gamestate", "g.usteed_id", 1);
    }
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) g.pl_character, sizeof g.pl_character);
        bwrite(nhfp->fd, (genericptr_t) g.pl_fruit, sizeof g.pl_fruit);
    }
    if (nhfp->fieldlevel) {
        sfo_char(nhfp, g.pl_character, "gamestate", "g.pl_character", sizeof g.pl_character);
        sfo_char(nhfp, g.pl_fruit, "gamestate", "g.pl_fruit", sizeof g.pl_fruit); 
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
    static boolean havestate = TRUE;
    char whynot[BUFSZ];
    NHFILE *nhfp;

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
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &g.hackpid, "gamestate", "g.hackpid", 1);
        if (flags.ins_chkpt) {
            int currlev = ledger_no(&u.uz);

            if (nhfp->structlevel)
                (void) write(nhfp->fd, (genericptr_t) &currlev, sizeof currlev);
            if (nhfp->fieldlevel)
                sfo_int(nhfp, &currlev, "gamestate", "savestateinlock", 1);
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

STATIC_OVL void
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
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &g.hackpid, "gamestate", "g.hackpid", 1);
#ifdef TOS
        tlev = lev;
        tlev &= 0x00ff;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &tlev, sizeof tlev);
        if (nhfp->fieldlevel)
            sfo_short(nhfp, &tlev, "gamestate", "tlev", 1);
#else
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &lev, sizeof lev);
        if (nhfp->fieldlevel)
            sfo_xchar(nhfp, &lev, "gamestate", "dlvl", 1);
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
    if (nhfp->fieldlevel) {
        int i, c, r;

        for (c = 0; c < COLNO; ++c)
            for (r = 0; r < ROWNO; ++r)
                sfo_schar(nhfp, &g.lastseentyp[c][r], "lev", "g.lastseentyp", 1);
        sfo_long(nhfp, &g.monstermoves, "lev", "timestmp", 1);
        sfo_stairway(nhfp, &g.upstair, "lev", "g.upstair", 1);
        sfo_stairway(nhfp, &g.dnstair, "lev", "g.dnstair", 1);
        sfo_stairway(nhfp, &g.upladder, "lev", "g.upladder", 1);
        sfo_stairway(nhfp, &g.dnladder, "lev", "g.dnladder", 1);
        sfo_stairway(nhfp, &g.sstairs, "lev", "g.sstairs", 1);
        sfo_dest_area(nhfp, &g.updest, "lev", "g.updest", 1);
        sfo_dest_area(nhfp, &g.dndest, "lev", "g.dndest", 1);
        sfo_levelflags(nhfp, &g.level.flags, "lev", "g.level.flags", 1);
        for (i = 0; i < DOORMAX; ++i)
            sfo_nhcoord(nhfp, &g.doors[i], "lev", "door", 1);
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

        for (y = 0; y < ROWNO; y++)
            for (x = 0; x < COLNO; x++)
                g.level.monsters[x][y] = 0;
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

STATIC_OVL void
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
                        bwrite(nhfp->fd, (genericptr_t) &match, sizeof (uchar));
                        bwrite(nhfp->fd, (genericptr_t) rgrm, sizeof (struct rm));
		    }
		    if (nhfp->fieldlevel) {
                        sfo_uchar(nhfp, &match, "levl", "match", 1);
                        sfo_rm(nhfp, rgrm, "levl", "rgrm", 1);
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
            if (nhfp->fieldlevel) {
                sfo_uchar(nhfp, &match, "levl", "match", 1);
                sfo_rm(nhfp, rgrm, "levl", "rgrm", 1);
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
    if (nhfp->fieldlevel) {
        int c, r;

        for (c = 0; c < COLNO; ++c)
            for (r = 0; r < ROWNO; ++r)
                sfo_rm(nhfp, &g.level.locations[c][r], "room", "levl", 1);
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
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &flag, "cemetery", "cemetery_flag", 1);
    }
    nextbones = *cemeteryaddr;
    while ((thisbones = nextbones) != 0) {
        nextbones = thisbones->next;
        if (perform_bwrite(nhfp)) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) thisbones, sizeof *thisbones);
            if (nhfp->fieldlevel)
                sfo_cemetery(nhfp, thisbones, "cemetery", "cemetery", 1);
	}
        if (release_data(nhfp))
            free((genericptr_t) thisbones);
    }
    if (release_data(nhfp))
        *cemeteryaddr = 0;
}

STATIC_OVL void
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
        if (nhfp->fieldlevel)
            sfo_unsigned(nhfp, &xl, "damage", "damage_count", 1);
    }
    while (xl--) {
        if (perform_bwrite(nhfp)) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) damageptr, sizeof *damageptr);
            if (nhfp->fieldlevel)
                sfo_damage(nhfp, damageptr, "damage", "damage", 1);
	}
        tmp_dam = damageptr;
        damageptr = damageptr->next;
        if (release_data(nhfp))
            free((genericptr_t) tmp_dam);
    }
    if (release_data(nhfp))
        g.level.damagelist = 0;
}

STATIC_OVL void
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
    if (nhfp->fieldlevel) {
        sfo_int(nhfp, &buflen, "obj", "obj_length", 1);
        sfo_obj(nhfp, otmp, "obj", "obj", 1);
    }
    if (otmp->oextra) {
        buflen = ONAME(otmp) ? (int) strlen(ONAME(otmp)) + 1 : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &buflen, "obj", "oname_length", 1);

        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) ONAME(otmp), buflen);
            if (nhfp->fieldlevel)
                sfo_str(nhfp, ONAME(otmp), "obj", "oname", buflen);
        }
        /* defer to savemon() for this one */
        if (OMONST(otmp)) {
            savemon(nhfp, OMONST(otmp));
        } else {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) &zerobuf, sizeof zerobuf);
            if (nhfp->fieldlevel)
                sfo_int(nhfp, &zerobuf, "obj", "omonst_length", 1);
	}
        buflen = OMID(otmp) ? (int) sizeof (unsigned) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &buflen, "obj", "omid_length", 1);            
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) OMID(otmp), buflen);
            if (nhfp->fieldlevel)
                sfo_int(nhfp, &buflen, "obj", "omid_length", 1);
	}
        /* TODO: post 3.6.x, get rid of this */
        buflen = OLONG(otmp) ? (int) sizeof (long) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &buflen, "obj", "olong_length", 1);
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) OLONG(otmp), buflen);
            if (nhfp->fieldlevel)
                sfo_long(nhfp, OLONG(otmp), "obj", "olong", 1);
	}

        buflen = OMAILCMD(otmp) ? (int) strlen(OMAILCMD(otmp)) + 1 : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &buflen, "obj", "omailcmd_length", 1);
        if (buflen > 0) {
            if (nhfp->structlevel)
                  bwrite(nhfp->fd, (genericptr_t) OMAILCMD(otmp), buflen);
            if (nhfp->fieldlevel)
                sfo_str(nhfp, OMAILCMD(otmp), "obj", "omailcmd", buflen);
        }
    }
}

STATIC_OVL void
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
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &minusone, "obj", "obj_length", 1);
    }
}

STATIC_OVL void
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
    if (nhfp->fieldlevel) {
        sfo_int(nhfp, &buflen, "mon", "monst_length", 1);
        sfo_monst(nhfp, mtmp, "mon", "monst", 1);
    }
    if (mtmp->mextra) {
        buflen = MNAME(mtmp) ? (int) strlen(MNAME(mtmp)) + 1 : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &buflen, "mon", "mname_length", 1);
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) MNAME(mtmp), buflen);
            if (nhfp->fieldlevel)
                sfo_str(nhfp, MNAME(mtmp), "mon", "mname", buflen);
        }
        buflen = EGD(mtmp) ? (int) sizeof (struct egd) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &buflen, "mon", "egd_length", 1);
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) EGD(mtmp), buflen);
            if (nhfp->fieldlevel)
                sfo_egd(nhfp, EGD(mtmp), "mon", "egd", 1);
        }
        buflen = EPRI(mtmp) ? (int) sizeof (struct epri) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &buflen, "mon", "epri_length", 1);
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) EPRI(mtmp), buflen);
            if (nhfp->fieldlevel)
                sfo_epri(nhfp, EPRI(mtmp), "mon", "epri", 1);
        }
        buflen = ESHK(mtmp) ? (int) sizeof (struct eshk) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof (int));
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &buflen, "mon", "eshk_length", 1);
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) ESHK(mtmp), buflen);
            if (nhfp->fieldlevel)
                sfo_eshk(nhfp, ESHK(mtmp), "mon", "eshk", 1);
        }
        buflen = EMIN(mtmp) ? (int) sizeof (struct emin) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof (int));
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &buflen, "mon", "emin_length", 1);
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) EMIN(mtmp), buflen);
            if (nhfp->fieldlevel)
                sfo_emin(nhfp, EMIN(mtmp), "mon", "emin", 1);
        }
        buflen = EDOG(mtmp) ? (int) sizeof (struct edog) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof (int));
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &buflen, "mon", "edog_length", 1);
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) EDOG(mtmp), buflen);
            if (nhfp->fieldlevel)
                sfo_edog(nhfp, EDOG(mtmp), "mon", "edog", 1);
	}
        /* mcorpsenm is inline int rather than pointer to something,
           so doesn't need to be preceded by a length field */
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &MCORPSENM(mtmp), sizeof MCORPSENM(mtmp));
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &MCORPSENM(mtmp), "mon", "mcorpsenm", 1);
    }
}

STATIC_OVL void
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
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &minusone, "mon", "monst_length", 1);
    }
}

/* save traps; g.ftrap is the only trap chain so the 2nd arg is superfluous */
STATIC_OVL void
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
            if (nhfp->fieldlevel)
                sfo_trap(nhfp, trap, "trap", "trap", 1);
	}
        if (release_data(nhfp))
            dealloc_trap(trap);
        trap = trap2;
    }
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &zerotrap, sizeof zerotrap);
        if (nhfp->fieldlevel)
            sfo_trap(nhfp, &zerotrap, "trap", "trap", 1);
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
            if (nhfp->fieldlevel)
                sfo_fruit(nhfp, f1, "fruit", "fruit", 1);
	}
        if (release_data(nhfp))
            dealloc_fruit(f1);
        f1 = f2;
    }
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &zerofruit, sizeof zerofruit);
        if (nhfp->fieldlevel)
            sfo_fruit(nhfp, &zerofruit, "fruit", "terminator", 1);
    }
    if (release_data(nhfp))
        g.ffruit = 0;
}



STATIC_OVL void
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
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &cnt, "levchn", "lev_count", 1);
    }
    for (tmplev = g.sp_levchn; tmplev; tmplev = tmplev2) {
        tmplev2 = tmplev->next;
        if (perform_bwrite(nhfp)) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) tmplev, sizeof *tmplev);
            if (nhfp->fieldlevel)
                sfo_s_level(nhfp, tmplev, "levchn", "s_level", 1);
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
    if (nhfp->fieldlevel) {
        sfo_int(nhfp, &plsiztmp, "plname", "plname_size", 1);
        sfo_str(nhfp, g.plname, "plname", "g.plname", plsiztmp);
    }
    return;
}

STATIC_OVL void
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
            if (nhfp->fieldlevel) {
                sfo_int(nhfp, &msglen, "msghistory", "msghistory_length", 1);
                sfo_str(nhfp, msg, "msghistory", "msg", msglen);
            }
            ++msgcount;
        }
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &minusone, sizeof (int));
        if (nhfp->fieldlevel)
            sfo_int(nhfp, &minusone, "msghistory", "msghistory_length", 1);
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
    if (nhfp->fieldlevel) {
        sfo_savefile_info(nhfp, &sfsaveinfo, "savefileinfo", "savefile_info", 1);
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
    unload_qtlist();
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

STATIC_OVL boolean
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

STATIC_OVL void
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

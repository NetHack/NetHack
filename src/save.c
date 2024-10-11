/* NetHack 3.7	save.c	$NHDT-Date: 1706079844 2024/01/24 07:04:04 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.214 $ */
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

#ifdef MICRO
int dotcnt, dotrow; /* also used in restore */
#endif

staticfn void savelevchn(NHFILE *);
staticfn void savelevl(NHFILE *,boolean);
staticfn void savedamage(NHFILE *);
staticfn void save_bubbles(NHFILE *, xint8);
staticfn void save_stairs(NHFILE *);
staticfn void save_bc(NHFILE *);
staticfn void saveobj(NHFILE *, struct obj *);
staticfn void saveobjchn(NHFILE *, struct obj **) NO_NNARGS;
staticfn void savemon(NHFILE *, struct monst *);
staticfn void savemonchn(NHFILE *, struct monst *) NO_NNARGS;
staticfn void savetrapchn(NHFILE *, struct trap *) NO_NNARGS;
staticfn void save_gamelog(NHFILE *);
staticfn void savegamestate(NHFILE *);
staticfn void savelev_core(NHFILE *, xint8);
staticfn void save_msghistory(NHFILE *);

#ifdef ZEROCOMP
staticfn void zerocomp_bufon(int);
staticfn void zerocomp_bufoff(int);
staticfn void zerocomp_bflush(int);
staticfn void zerocomp_bwrite(int, genericptr_t, unsigned int);
staticfn void zerocomp_bputc(int);
#endif

#if defined(HANGUPHANDLING)
#define HUP if (!program_state.done_hup)
#else
#define HUP
#endif

/* the #save command */
int
dosave(void)
{
    clear_nhwindow(WIN_MESSAGE);
    if (y_n("Really save?") == 'n') {
        clear_nhwindow(WIN_MESSAGE);
        if (gm.multi > 0)
            nomul(0);
    } else {
        clear_nhwindow(WIN_MESSAGE);
        pline("Saving...");
#if defined(HANGUPHANDLING)
        program_state.done_hup = 0;
#endif
        if (dosave0()) {
            u.uhp = -1; /* universal game's over indicator */
            if (soundprocs.sound_exit_nhsound)
                (*soundprocs.sound_exit_nhsound)("dosave");

            /* make sure they see the Saving message */
            display_nhwindow(WIN_MESSAGE, TRUE);
            exit_nhwindows("Be seeing you...");
            nh_terminate(EXIT_SUCCESS);
        } else
            docrt();
    }
    return ECMD_OK;
}

/* returns 1 if save successful */
int
dosave0(void)
{
    const char *fq_save;
    xint8 ltmp;
    char whynot[BUFSZ];
    NHFILE *nhfp, *onhfp;
    int res = 0;

    program_state.saving++; /* inhibit status and perm_invent updates */
    notice_mon_off();
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
    /* extra handling for hangup save or panic save; without this,
       a thrown light source might trigger an "obj_is_local" panic;
       if a thrown or kicked object is in transit, put it on the map;
       when punished, make sure ball and chain are placed too */
    done_object_cleanup(); /* maybe force some items onto map */

    if (!program_state.something_worth_saving || !gs.SAVEF[0])
        goto done;

    fq_save = fqname(gs.SAVEF, SAVEPREFIX, 1); /* level files take 0 */
#ifndef NO_SIGNAL
#if defined(UNIX) || defined(VMS)
    sethanguphandler((void (*)(int) ) SIG_IGN);
#endif
    (void) signal(SIGINT, SIG_IGN);
#endif

    HUP if (iflags.window_inited) {
        nh_uncompress(fq_save);
        nhfp = open_savefile();
        if (nhfp) {
            close_nhfile(nhfp);
            clear_nhwindow(WIN_MESSAGE);
            There("seems to be an old save file.");
            if (y_n("Overwrite the old file?") == 'n') {
                nh_compress(fq_save);
                goto done;
            }
        }
    }

    HUP mark_synch(); /* flush any buffered screen output */

    nhfp = create_savefile();
    if (!nhfp) {
        HUP pline("Cannot open save file.");
        (void) delete_savefile(); /* ab@unido */
        goto done;
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
    if (!WINDOWPORT(X11))
        putstr(WIN_MAP, 0, "Saving:");
#endif
    nhfp->mode = WRITING | FREEING;
    store_version(nhfp);
    store_savefileinfo(nhfp);
    if (nhfp && nhfp->fplog)
        (void) fprintf(nhfp->fplog, "# post-validation\n");
    store_plname_in_file(nhfp);
    /* savelev() might save uball and uchain, releasing their memory if
       FREEING, so we need to check their status now; if hero is swallowed,
       uball and uchain will persist beyond saving map floor and inventory
       so these copies of their pointers will be valid and savegamestate()
       will know to save them separately (from floor and invent); when not
       swallowed, uchain will be stale by then, and uball will be too if
       ball is on the floor rather than carried */
    gl.looseball = BALL_IN_MON ? uball : 0;
    gl.loosechain = CHAIN_IN_MON ? uchain : 0;
    savelev(nhfp, ledger_no(&u.uz));
    savegamestate(nhfp);

    /* While copying level files around, zero out u.uz to keep
     * parts of the restore code from completely initializing all
     * in-core data structures, since all we're doing is copying.
     * This also avoids at least one nasty core dump.
     * [gu.uz_save is used by save_bubbles() as well as to restore u.uz]
     */
    gu.uz_save = u.uz;
    u.uz.dnum = u.uz.dlevel = 0;
    /* these pointers are no longer valid, and at least u.usteed
     * may mislead place_monster() on other levels
     */
    set_ustuck((struct monst *) 0); /* also clears u.uswallow */
    u.usteed = (struct monst *) 0;

    for (ltmp = (xint8) 1; ltmp <= maxledgerno(); ltmp++) {
        if (ltmp == ledger_no(&gu.uz_save))
            continue;
        if (!(svl.level_info[ltmp].flags & LFILE_EXISTS))
            continue;
#ifdef MICRO
        curs(WIN_MAP, 1 + dotcnt++, dotrow);
        if (dotcnt >= (COLNO - 1)) {
            dotrow++;
            dotcnt = 0;
        }
        if (!WINDOWPORT(X11)) {
            putstr(WIN_MAP, 0, ".");
        }
        mark_synch();
#endif
        onhfp = open_levelfile(ltmp, whynot);
        if (!onhfp) {
            HUP pline1(whynot);
            close_nhfile(nhfp);
            (void) delete_savefile();
            HUP Strcpy(svk.killer.name, whynot);
            HUP done(TRICKED);
            goto done;
        }
        minit(); /* ZEROCOMP */
        getlev(onhfp, svh.hackpid, ltmp);
        close_nhfile(onhfp);
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &ltmp, sizeof ltmp); /* lvl no. */
        savelev(nhfp, ltmp);     /* actual level*/
        delete_levelfile(ltmp);
    }
    close_nhfile(nhfp);

    u.uz = gu.uz_save;
    gu.uz_save.dnum = gu.uz_save.dlevel = 0;

    /* get rid of current level --jgm */
    delete_levelfile(ledger_no(&u.uz));
    delete_levelfile(0);
    nh_compress(fq_save);
    /* this should probably come sooner... */
    program_state.something_worth_saving = 0;
    res = 1;

 done:
    notice_mon_on();
    program_state.saving--;
    return res;
}

staticfn void
save_gamelog(NHFILE *nhfp)
{
    struct gamelog_line *tmp = gg.gamelog, *tmp2;
    int slen;

    while (tmp) {
        tmp2 = tmp->next;
        if (perform_bwrite(nhfp)) {
            if (nhfp->structlevel) {
                slen = Strlen(tmp->text);
                bwrite(nhfp->fd, (genericptr_t) &slen, sizeof slen);
                bwrite(nhfp->fd, (genericptr_t) tmp->text, slen);
                bwrite(nhfp->fd, (genericptr_t) tmp,
                       sizeof (struct gamelog_line));
            }
        }
        if (release_data(nhfp)) {
            free((genericptr_t) tmp->text);
            free((genericptr_t) tmp);
        }
        tmp = tmp2;
    }
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel) {
            slen = -1;
            bwrite(nhfp->fd, (genericptr_t) &slen, sizeof slen);
        }
    }
    if (release_data(nhfp))
        gg.gamelog = NULL;
}

staticfn void
savegamestate(NHFILE *nhfp)
{
    unsigned long uid;

    program_state.saving++; /* caller should/did already set this... */
    uid = (unsigned long) getuid();
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) &uid, sizeof uid);
        bwrite(nhfp->fd, (genericptr_t) &svc.context, sizeof svc.context);
        bwrite(nhfp->fd, (genericptr_t) &flags, sizeof flags);
    }
    urealtime.finish_time = getnow();
    urealtime.realtime += timet_delta(urealtime.finish_time,
                                      urealtime.start_timing);
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

    /* must come before gm.migrating_objs and gm.migrating_mons are freed */
    save_timers(nhfp, RANGE_GLOBAL);
    save_light_sources(nhfp, RANGE_GLOBAL);

    /* when FREEING, deletes objects in invent and sets invent to Null;
       pointers into invent (uwep, uarmg, uamul, &c) are set to Null too */
    saveobjchn(nhfp, &gi.invent);

    /* save ball and chain if they happen to be in an unusual state */
    save_bc(nhfp);

    saveobjchn(nhfp, &gm.migrating_objs); /* frees objs and sets to Null */
    savemonchn(nhfp, gm.migrating_mons);
    if (release_data(nhfp))
        gm.migrating_mons = (struct monst *) 0;
    if (nhfp->structlevel)
        bwrite(nhfp->fd, (genericptr_t) svm.mvitals, sizeof svm.mvitals);
    save_dungeon(nhfp, (boolean) !!perform_bwrite(nhfp),
                 (boolean) !!release_data(nhfp));
    savelevchn(nhfp);
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) &svm.moves, sizeof svm.moves);
        bwrite(nhfp->fd, (genericptr_t) &svq.quest_status,
               sizeof svq.quest_status);
        bwrite(nhfp->fd, (genericptr_t) svs.spl_book,
               sizeof (struct spell) * (MAXSPELL + 1));
    }
    save_artifacts(nhfp);
    save_oracles(nhfp);
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) svp.pl_character,
               sizeof svp.pl_character);
        bwrite(nhfp->fd, (genericptr_t) svp.pl_fruit, sizeof svp.pl_fruit);
    }
    savefruitchn(nhfp);
    savenames(nhfp);
    save_msghistory(nhfp);
    save_gamelog(nhfp);
    save_luadata(nhfp);
    if (nhfp->structlevel)
        bflush(nhfp->fd);
    program_state.saving--;
    return;
}

/* potentially called from goto_level(do.c) as well as savestateinlock() */
boolean
tricked_fileremoved(NHFILE *nhfp, char *whynot)
{
    if (!nhfp) {
        pline1(whynot);
        pline("Probably someone removed it.");
        Strcpy(svk.killer.name, whynot);
        done(TRICKED);
        return TRUE;
    }
    return FALSE;
}

#ifdef INSURANCE
void
savestateinlock(void)
{
    int hpid = 0;
    char whynot[BUFSZ];
    NHFILE *nhfp;

    program_state.saving++; /* inhibit status and perm_invent updates */
    /* When checkpointing is on, the full state needs to be written
     * on each checkpoint.  When checkpointing is off, only the pid
     * needs to be in the level.0 file, so it does not need to be
     * constantly rewritten.  When checkpointing is turned off during
     * a game, however, the file has to be rewritten once to truncate
     * it and avoid restoring from outdated information.
     *
     * Restricting gh.havestate to this routine means that an additional
     * noop pid rewriting will take place on the first "checkpoint" after
     * the game is started or restored, if checkpointing is off.
     */
    if (flags.ins_chkpt || gh.havestate) {
        /* save the rest of the current game state in the lock file,
         * following the original int pid, the current level number,
         * and the current savefile name, which should not be subject
         * to any internal compression schemes since they must be
         * readable by an external utility
         */
        nhfp = open_levelfile(0, whynot);
        if (tricked_fileremoved(nhfp, whynot)) {
            program_state.saving--;
            return;
        }

        if (nhfp->structlevel)
            (void) read(nhfp->fd, (genericptr_t) &hpid, sizeof hpid);
        if (svh.hackpid != hpid) {
            Sprintf(whynot, "Level #0 pid (%d) doesn't match ours (%d)!",
                    hpid, svh.hackpid);
            goto giveup;
        }
        close_nhfile(nhfp);

        nhfp = create_levelfile(0, whynot);
        if (!nhfp) {
            pline1(whynot);
 giveup:
            Strcpy(svk.killer.name, whynot);
            /* done(TRICKED) will return when running in wizard mode;
               clear the display-update-suppression flag before rather
               than after so that screen updating behaves normally;
               game data shouldn't be inconsistent yet, unlike it would
               become midway through saving */
            program_state.saving--;
            done(TRICKED);
            return;
        }
        nhfp->mode = WRITING;
        if (nhfp->structlevel)
            (void) write(nhfp->fd, (genericptr_t) &svh.hackpid,
                         sizeof svh.hackpid);
        if (flags.ins_chkpt) {
            int currlev = ledger_no(&u.uz);

            if (nhfp->structlevel)
                (void) write(nhfp->fd, (genericptr_t) &currlev,
                             sizeof currlev);
            save_savefile_name(nhfp);
            store_version(nhfp);
            store_savefileinfo(nhfp);
            store_plname_in_file(nhfp);

            /* if ball and/or chain aren't on floor or in invent, keep a copy
               of their pointers; not valid when on floor or in invent */
            gl.looseball = BALL_IN_MON ? uball : 0;
            gl.loosechain = CHAIN_IN_MON ? uchain : 0;
            savegamestate(nhfp);
        }
        close_nhfile(nhfp);
    }
    program_state.saving--;
    gh.havestate = flags.ins_chkpt;
    return;
}
#endif

void
savelev(NHFILE *nhfp, xint8 lev)
{
    boolean set_uz_save = (gu.uz_save.dnum == 0 && gu.uz_save.dlevel == 0);

    /* caller might have already set up gu.uz_save and zeroed u.uz;
       if not, we need to set it for save_bubbles(); caveat: if the
       player quits during character selection, u.uz won't be set yet
       but we'll be called during run-down */
    if (set_uz_save && perform_bwrite(nhfp)) {
        if (u.uz.dnum == 0 && u.uz.dlevel == 0) {
            program_state.something_worth_saving = 0;
            panic("savelev: where are we?");
        }
        gu.uz_save = u.uz;
    }

    savelev_core(nhfp, lev);

    if (set_uz_save)
        gu.uz_save.dnum = gu.uz_save.dlevel = 0; /* unset */
}

staticfn void
savelev_core(NHFILE *nhfp, xint8 lev)
{
#ifdef TOS
    short tlev;
#endif

    program_state.saving++; /* even if current mode is FREEING */

    if (!nhfp)
        panic("Save on bad file!"); /* impossible */
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

        if (lev >= 0 && lev <= maxledgerno())
            svl.level_info[lev].flags |= VISITED;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &svh.hackpid, sizeof svh.hackpid);
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
    savecemetery(nhfp, &svl.level.bonesinfo);
    if (nhfp->mode == FREEING) /* see above */
        goto skip_lots;

    savelevl(nhfp, ((sfsaveinfo.sfi1 & SFI1_RLECOMP) == SFI1_RLECOMP));
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) svl.lastseentyp,
               sizeof svl.lastseentyp);
        bwrite(nhfp->fd, (genericptr_t) &svm.moves, sizeof svm.moves);
        save_stairs(nhfp);
        bwrite(nhfp->fd, (genericptr_t) &svu.updest, sizeof (dest_area));
        bwrite(nhfp->fd, (genericptr_t) &svd.dndest, sizeof (dest_area));
        bwrite(nhfp->fd, (genericptr_t) &svl.level.flags,
               sizeof svl.level.flags);
        bwrite(nhfp->fd, (genericptr_t) &svd.doors_alloc,
               sizeof svd.doors_alloc);
        /* don't rely on underlying write() behavior to write
         *  nothing if count arg is 0, just skip it */
        if (svd.doors_alloc)
            bwrite(nhfp->fd, (genericptr_t) svd.doors,
                   svd.doors_alloc * sizeof (coord));
    }
    save_rooms(nhfp); /* no dynamic memory to reclaim */

    /* from here on out, saving also involves allocated memory cleanup */
 skip_lots:
    /* timers and lights must be saved before monsters and objects */
    save_timers(nhfp, RANGE_LEVEL);
    save_light_sources(nhfp, RANGE_LEVEL);

    savemonchn(nhfp, fmon);
    save_worm(nhfp); /* save worm information */
    savetrapchn(nhfp, gf.ftrap);
    saveobjchn(nhfp, &fobj);
    saveobjchn(nhfp, &svl.level.buriedobjlist);
    saveobjchn(nhfp, &gb.billobjs);
    save_engravings(nhfp);
    savedamage(nhfp); /* pending shop wall and/or floor repair */
    save_regions(nhfp);
    save_bubbles(nhfp, lev); /* for water and air */
    save_exclusions(nhfp);
    save_track(nhfp);

    if (nhfp->mode != FREEING) {
        if (nhfp->structlevel)
            bflush(nhfp->fd);
    }
    program_state.saving--;
    if (release_data(nhfp)) {
        clear_level_structures();
        gf.ftrap = 0;
        gb.billobjs = 0;
        (void) memset(svr.rooms, 0, sizeof(svr.rooms));
    }
    return;
}

staticfn void
savelevl(NHFILE *nhfp, boolean rlecomp)
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
    return;
}

/* save Plane of Water's air bubbles and Plane of Air's clouds */
staticfn void
save_bubbles(NHFILE *nhfp, xint8 lev)
{
    xint8 bbubbly;

    /* air bubbles and clouds used to be saved as part of game state
       because restoring them needs dungeon data that isn't available
       during the first pass of their levels; now that they are part of
       the current level instead, we write a zero or non-zero marker
       so that restore can determine whether they are present even when
       u.uz and ledger_no() aren't available to it yet */
    bbubbly = 0;
    if (lev == ledger_no(&water_level) || lev == ledger_no(&air_level))
        bbubbly = lev; /* non-zero */
    if (nhfp->structlevel)
        bwrite(nhfp->fd, (genericptr_t) &bbubbly, sizeof bbubbly);

    if (bbubbly)
        save_waterlevel(nhfp); /* save air bubbles or clouds */
}

/* used when saving a level and also when saving dungeon overview data */
void
savecemetery(NHFILE *nhfp, struct cemetery **cemeteryaddr)
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

staticfn void
savedamage(NHFILE *nhfp)
{
    struct damage *damageptr, *tmp_dam;
    unsigned int xl = 0;

    damageptr = svl.level.damagelist;
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
        svl.level.damagelist = 0;
}

staticfn void
save_stairs(NHFILE *nhfp)
{
    stairway *stway = gs.stairs;
    int buflen = (int) sizeof *stway;

    while (stway) {
        if (perform_bwrite(nhfp)) {
            boolean use_relative = (program_state.restoring != REST_GSTATE
                                    && stway->tolev.dnum == u.uz.dnum);
            if (use_relative) {
                /* make dlevel relative to current level */
                stway->tolev.dlevel -= u.uz.dlevel;
            }
            if (nhfp->structlevel) {
                bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
                bwrite(nhfp->fd, (genericptr_t) stway, sizeof *stway);
            }
            if (use_relative) {
                /* reset stairway dlevel back to absolute */
                stway->tolev.dlevel += u.uz.dlevel;
            }
        }
        stway = stway->next;
    }
    if (perform_bwrite(nhfp)) {
        buflen = -1;
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        }
    }
}

/* if ball and/or chain are loose, make an object chain for it/them and
   save that separately from other objects */
staticfn void
save_bc(NHFILE *nhfp)
{
    struct obj *bc_objs = 0;

    /* save ball and chain if they are currently dangling free (i.e. not
       on floor or in inventory); 'looseball' and 'loosechain' have been
       set up in caller because ball and chain will be gone by now if on
       floor, or ball gone if carried */
    if (gl.loosechain) {
        gl.loosechain->nobj = bc_objs; /* uchain */
        bc_objs = gl.loosechain;
        if (nhfp->mode & FREEING) {
            setworn((struct obj *) 0, W_CHAIN); /* sets 'uchain' to Null */
            gl.loosechain = (struct obj *) 0;
        }
    }
    if (gl.looseball) {
        gl.looseball->nobj = bc_objs;
        bc_objs = gl.looseball;
        if (nhfp->mode & FREEING) {
            setworn((struct obj *) 0, W_BALL); /* sets 'uball' to Null */
            gl.looseball = (struct obj *) 0;
        }
    }
    saveobjchn(nhfp, &bc_objs); /* frees objs in list, sets bc_objs to Null */
}

/* save one object;
   caveat: this is only for perform_bwrite(); caller handles release_data() */
staticfn void
saveobj(NHFILE *nhfp, struct obj *otmp)
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
           gets saved/restored whenever any other oextra components do */
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &OMID(otmp), sizeof OMID(otmp));
    }
}

/* save an object chain; sets head of list to Null when done;
   handles release_data() for each object in the list */
staticfn void
saveobjchn(NHFILE *nhfp, struct obj **obj_p)
{
    struct obj *otmp = *obj_p;
    struct obj *otmp2;
    boolean is_invent = (otmp && otmp == gi.invent);
    int minusone = -1;

    while (otmp) {
        otmp2 = otmp->nobj;
        if (perform_bwrite(nhfp)) {
            saveobj(nhfp, otmp);
        }
        if (Has_contents(otmp))
            saveobjchn(nhfp, &otmp->cobj);
        if (release_data(nhfp)) {
            /*
             * If these are on the floor, the discarding could be
             * due to game save, or we could just be changing levels.
             * Always invalidate the pointer, but ensure that we have
             * the o_id in order to restore the pointer on reload.
             */
            if (otmp == svc.context.victual.piece) {
                svc.context.victual.o_id = otmp->o_id;
                svc.context.victual.piece = (struct obj *) 0;
            }
            if (otmp == svc.context.tin.tin) {
                svc.context.tin.o_id = otmp->o_id;
                svc.context.tin.tin = (struct obj *) 0;
            }
            if (otmp == svc.context.spbook.book) {
                svc.context.spbook.o_id = otmp->o_id;
                svc.context.spbook.book = (struct obj *) 0;
            }
            otmp->where = OBJ_FREE; /* set to free so dealloc will work */
            otmp->nobj = NULL;      /* nobj saved into otmp2 */
            otmp->cobj = NULL;      /* contents handled above */
            otmp->timed = 0;        /* not timed any more */
            otmp->lamplit = 0;      /* caller handled lights */
            otmp->leashmon = 0;     /* mon->mleashed could still be set;
                                     * unfortunately, we can't clear that
                                     * until after the monster is saved */
            /* clear 'uball' and 'uchain' pointers if resetting their mask;
               could also do same for other worn items but don't need to */
            if ((otmp->owornmask & (W_BALL | W_CHAIN)) != 0)
                setworn((struct obj *) 0,
                        otmp->owornmask & (W_BALL | W_CHAIN));
            otmp->owornmask = 0L;   /* no longer care */
            dealloc_obj(otmp);
        }
        otmp = otmp2;
    }
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &minusone, sizeof (int));
    }
    if (release_data(nhfp)) {
        if (is_invent)
            allunworn(); /* clear uwep, uarm, uball, &c pointers */
        *obj_p = (struct obj *) 0;
    }
}

staticfn void
savemon(NHFILE *nhfp, struct monst *mtmp)
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
        buflen = MGIVENNAME(mtmp) ? (int) strlen(MGIVENNAME(mtmp)) + 1 : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (buflen > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) MGIVENNAME(mtmp), buflen);
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

staticfn void
savemonchn(NHFILE *nhfp, struct monst *mtmp)
{
    struct monst *mtmp2;
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
            saveobjchn(nhfp, &mtmp->minvent);
        if (release_data(nhfp)) {
            if (mtmp == svc.context.polearm.hitmon) {
                svc.context.polearm.m_id = mtmp->m_id;
                svc.context.polearm.hitmon = NULL;
            }
            if (mtmp == u.ustuck)
                u.ustuck_mid = u.ustuck->m_id;
            if (mtmp == u.usteed)
                u.usteed_mid = u.usteed->m_id;
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

/* save traps; gf.ftrap is the only trap chain so 2nd arg is superfluous */
staticfn void
savetrapchn(NHFILE *nhfp, struct trap *trap)
{
    static struct trap zerotrap;
    struct trap *trap2;

    while (trap) {
        boolean use_relative = (program_state.restoring != REST_GSTATE
                                && trap->dst.dnum == u.uz.dnum);
        trap2 = trap->ntrap;
        if (use_relative)
            trap->dst.dlevel -= u.uz.dlevel; /* make it relative */
        if (perform_bwrite(nhfp)) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) trap, sizeof *trap);
        }
        if (use_relative)
            trap->dst.dlevel += u.uz.dlevel; /* reset back to absolute */
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
savefruitchn(NHFILE *nhfp)
{
    static struct fruit zerofruit;
    struct fruit *f2, *f1;

    f1 = gf.ffruit;
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
        gf.ffruit = 0;
}

staticfn void
savelevchn(NHFILE *nhfp)
{
    s_level *tmplev, *tmplev2;
    int cnt = 0;

    for (tmplev = svs.sp_levchn; tmplev; tmplev = tmplev->next)
        cnt++;
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &cnt, sizeof cnt);
    }
    for (tmplev = svs.sp_levchn; tmplev; tmplev = tmplev2) {
        tmplev2 = tmplev->next;
        if (perform_bwrite(nhfp)) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) tmplev, sizeof *tmplev);
        }
        if (release_data(nhfp))
            free((genericptr_t) tmplev);
    }
    if (release_data(nhfp))
        svs.sp_levchn = 0;
}

/* write "name-role-race-gend-algn" into save file for menu-based restore;
   the first dash is actually stored as '\0' instead of '-' */
void
store_plname_in_file(NHFILE *nhfp)
{
    char hero[PL_NSIZ_PLUS]; /* [PL_NSIZ + 4*4 + 1] */
    int plsiztmp = (int) sizeof hero;

    (void) memset((genericptr_t) hero, '\0', sizeof hero);
    /* augment svp.plname[]; the gender and alignment values reflect those
       in effect at time of saving rather than at start of game */
    Snprintf(hero, sizeof hero, "%s-%.3s-%.3s-%.3s-%.3s",
            svp.plname, gu.urole.filecode,
            gu.urace.filecode, genders[flags.female].filecode,
            aligns[1 - u.ualign.type].filecode);
    /* replace "-role-race..." with "\0role-race..." so that we can include
       or exclude the role-&c suffix easily, without worrying about whether
       plname contains any dashes; but don't rely on snprintf() for this */
    hero[strlen(svp.plname)] = '\0';
    /* insert playmode into final slot of hero[];
       'D','X','-' are the same characters as are used for paniclog entries */
    assert(hero[PL_NSIZ_PLUS - 1 - 1] == '\0');
    hero[PL_NSIZ_PLUS - 1] = wizard ? 'D' : discover ? 'X' : '-';

    if (nhfp->structlevel) {
        bufoff(nhfp->fd);
        /* bwrite() before bufon() uses plain write() */
        bwrite(nhfp->fd, (genericptr_t) &plsiztmp, sizeof plsiztmp);
        bwrite(nhfp->fd, (genericptr_t) hero, plsiztmp);
        bufon(nhfp->fd);
    }
    return;
}

staticfn void
save_msghistory(NHFILE *nhfp)
{
    char *msg;
    int msgcount = 0, msglen;
    int minusone = -1;
    boolean init = TRUE;

    if (perform_bwrite(nhfp)) {
        /* ask window port for each message in sequence */
        while ((msg = getmsghistory(init)) != 0) {
            init = FALSE;
            msglen = Strlen(msg);
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
store_savefileinfo(NHFILE *nhfp)
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
        bwrite(nhfp->fd, (genericptr_t) &sfsaveinfo,
               (unsigned) sizeof sfsaveinfo);
        bufon(nhfp->fd);
    }
    return;
}

/* also called by prscore(); this probably belongs in dungeon.c... */
void
free_dungeons(void)
{
#ifdef FREE_ALL_MEMORY
    NHFILE tnhfp;

    zero_nhfile(&tnhfp);    /* also sets fd to -1 */
    tnhfp.mode = FREEING;
    savelevchn(&tnhfp);
    save_dungeon(&tnhfp, FALSE, TRUE);
    free_luathemes(all_themes);
#endif
    return;
}

/* free a lot of allocated memory which is ordinarily freed during save */
void
freedynamicdata(void)
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
    savedsym_free();
    tmp_at(DISP_FREEMEM, 0); /* temporary display effects */
    purge_all_custom_entries();
#ifdef FREE_ALL_MEMORY
#define free_current_level() savelev(&tnhfp, -1)
#define freeobjchn(X) (saveobjchn(&tnhfp, &X), X = 0)
#define freemonchn(X) (savemonchn(&tnhfp, X), X = 0)
#define freefruitchn() savefruitchn(&tnhfp)
#define freenames() savenames(&tnhfp)
#define free_killers() save_killers(&tnhfp)
#define free_oracles() save_oracles(&tnhfp)
#define free_waterlevel() save_waterlevel(&tnhfp)
#define free_timers(R) save_timers(&tnhfp, R)
#define free_light_sources(R) save_light_sources(&tnhfp, R)
#define free_animals() mon_animal_list(FALSE)
#define discard_gamelog() save_gamelog(&tnhfp);

    /* move-specific data */
    dmonsfree(); /* release dead monsters */
    dobjsfree();
    alloc_itermonarr(0U); /* a request of 0 releases existing allocation */

    /* level-specific data */
    done_object_cleanup(); /* maybe force some OBJ_FREE items onto map */
    free_current_level();

    /* game-state data [ought to reorganize savegamestate() to handle this] */
    free_killers();
    free_timers(RANGE_GLOBAL);
    free_light_sources(RANGE_GLOBAL);
    freeobjchn(gi.invent);
    freeobjchn(gm.migrating_objs);
    freemonchn(gm.migrating_mons);
    freemonchn(gm.mydogs); /* ascension or dungeon escape */
    /* freelevchn();  --  [folded into free_dungeons()] */
    free_animals();
    free_oracles();
    freefruitchn();
    freenames();
    free_waterlevel();
    free_dungeons();
    free_CapMons();
    free_rect();
    freeroleoptvals(); /* saveoptvals(&tnhfp) */
    cmdq_clear(CQ_CANNED);
    cmdq_clear(CQ_REPEAT);
    free_tutorial(); /* (only needed if quitting while in tutorial) */

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
#ifdef USER_SOUNDS
    release_sound_mappings();
#endif
#ifdef DUMPLOG_CORE
    dumplogfreemessages();
#endif
    discard_gamelog();
    release_runtime_info(); /* build-time options and version stuff */
#endif /* FREE_ALL_MEMORY */

    if (VIA_WINDOWPORT())
        status_finish();

    /* last, because it frees data that might be used by panic() to provide
       feedback to the user; conceivably other freeing might trigger panic */
    sysopt_release(); /* SYSCF strings */
    return;
}

/*save.c*/

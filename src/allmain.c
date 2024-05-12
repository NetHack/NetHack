/* NetHack 3.7	allmain.c	$NHDT-Date: 1704225560 2024/01/02 19:59:20 $  $NHDT-Branch: keni-luabits2 $:$NHDT-Revision: 1.238 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

/* various code that was replicated in *main.c */

#include "hack.h"
#include <ctype.h>

#ifndef NO_SIGNAL
#include <signal.h>
#endif

staticfn void moveloop_preamble(boolean);
staticfn void u_calc_moveamt(int);
staticfn void maybe_do_tutorial(void);
#ifdef POSITIONBAR
staticfn void do_positionbar(void);
#endif
staticfn void regen_pw(int);
staticfn void regen_hp(int);
staticfn void interrupt_multi(const char *);
staticfn void debug_fields(const char *);
#ifndef NODUMPENUMS
staticfn void dump_enums(void);
#endif

#ifdef CRASHREPORT
#define USED_FOR_CRASHREPORT
#else
#define USED_FOR_CRASHREPORT UNUSED
#endif

/*ARGSUSED*/
void
early_init(int argc USED_FOR_CRASHREPORT, char *argv[] USED_FOR_CRASHREPORT)
{
#ifdef CRASHREPORT
    /* Do this as early as possible, but let ports do other things first. */
    crashreport_init(argc, argv);
#endif
    decl_globals_init();
    objects_globals_init();
    monst_globals_init();
    sys_early_init();
    runtime_info_init();
}

staticfn void
moveloop_preamble(boolean resuming)
{
    /* if a save file created in normal mode is now being restored in
       explore mode, treat it as normal restore followed by 'X' command
       to use up the save file and require confirmation for explore mode */
    if (resuming && iflags.deferred_X)
        (void) enter_explore_mode();

    /* side-effects from the real world */
    flags.moonphase = phase_of_the_moon();
    if (flags.moonphase == FULL_MOON) {
        You("are lucky!  Full moon tonight.");
        change_luck(1);
    } else if (flags.moonphase == NEW_MOON) {
        pline("Be careful!  New moon tonight.");
    }
    flags.friday13 = friday_13th();
    if (flags.friday13) {
        pline("Watch out!  Bad things can happen on Friday the 13th.");
        change_luck(-1);
    }

    if (!resuming) { /* new game */
        gp.program_state.beyond_savefile_load = 1; /* for TTY_PERM_INVENT */
        gc.context.rndencode = rnd(9000);
        set_wear((struct obj *) 0); /* for side-effects of starting gear */
        reset_justpicked(gi.invent);
        (void) pickup(1);      /* autopickup at initial location */
        /* only matters if someday a character is able to start with
           clairvoyance (wizard with cornuthaum perhaps?); without this,
           first "random" occurrence would always kick in on turn 1 */
        gc.context.seer_turn = (long) rnd(30);
        /* give hero initial movement points; new game only--for restore,
           pending movement points were included in the save file */
        u.umovement = NORMAL_SPEED;
        initrack();
    }
    disp.botlx = TRUE; /* for STATUS_HILITES */
    if (resuming) { /* restoring old game */
        read_engr_at(u.ux, u.uy); /* subset of pickup() */
        fix_shop_damage();
    }

    (void) encumber_msg(); /* in case they auto-picked up something */
    if (gd.defer_see_monsters) {
        gd.defer_see_monsters = FALSE;
        see_monsters();
    }

    u.uz0.dlevel = u.uz.dlevel;
    gc.context.move = 0;

    gp.program_state.in_moveloop = 1;
    /* for perm_invent preset at startup, display persistent inventory after
       invent is fully populated and the in_moveloop flag has been set */
    if (iflags.perm_invent)
        update_inventory();
}

staticfn void
u_calc_moveamt(int wtcap)
{
    int moveamt = 0;

    /* calculate how much time passed. */
    if (u.usteed && u.umoved) {
        /* your speed doesn't augment steed's speed */
        moveamt = mcalcmove(u.usteed, TRUE);
    } else {
        moveamt = gy.youmonst.data->mmove;

        if (Very_fast) { /* speed boots, potion, or spell */
            /* gain a free action on 2/3 of turns */
            if (rn2(3) != 0)
                moveamt += NORMAL_SPEED;
        } else if (Fast) { /* intrinsic */
            /* gain a free action on 1/3 of turns */
            if (rn2(3) == 0)
                moveamt += NORMAL_SPEED;
        }
    }

    switch (wtcap) {
    case UNENCUMBERED:
        break;
    case SLT_ENCUMBER:
        moveamt -= (moveamt / 4);
        break;
    case MOD_ENCUMBER:
        moveamt -= (moveamt / 2);
        break;
    case HVY_ENCUMBER:
        moveamt -= ((moveamt * 3) / 4);
        break;
    case EXT_ENCUMBER:
        moveamt -= ((moveamt * 7) / 8);
        break;
    default:
        break;
    }

    u.umovement += moveamt;
    if (u.umovement < 0)
        u.umovement = 0;
}

#if defined(MICRO) || defined(WIN32)
static int mvl_abort_lev;
#endif
static int mvl_wtcap = 0;
static int mvl_change = 0;

void
moveloop_core(void)
{
    boolean monscanmove = FALSE;

#ifdef SAFERHANGUP
    if (gp.program_state.done_hup)
        end_of_input();
#endif
    get_nh_event();
#ifdef POSITIONBAR
    do_positionbar();
#endif

    dobjsfree();

    if (gc.context.bypasses)
        clear_bypasses();

    if (iflags.sanity_check || iflags.debug_fuzzer)
        sanity_check();

    if (gc.context.move) {
        /* actual time passed */
        u.umovement -= NORMAL_SPEED;

        do { /* hero can't move this turn loop */
            mvl_wtcap = encumber_msg();

            gc.context.mon_moving = TRUE;
            do {
                monscanmove = movemon();
                if (u.umovement >= NORMAL_SPEED)
                    break; /* it's now your turn */
            } while (monscanmove);
            gc.context.mon_moving = FALSE;

            if (!monscanmove && u.umovement < NORMAL_SPEED) {
                /* both hero and monsters are out of steam this round */
                struct monst *mtmp;

                /* set up for a new turn */
                mcalcdistress(); /* adjust monsters' trap, blind, etc */

                /* reallocate movement rations to monsters; don't need
                   to skip dead monsters here because they will have
                   been purged at end of their previous round of moving */
                for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
                    mtmp->movement += mcalcmove(mtmp, TRUE);

                /* occasionally add another monster; since this takes
                   place after movement has been allotted, the new
                   monster effectively loses its first turn */
                if (!rn2(u.uevent.udemigod ? 25
                         : (depth(&u.uz) > depth(&stronghold_level)) ? 50
                         : 70))
                    (void) makemon((struct permonst *) 0, 0, 0,
                                   NO_MM_FLAGS);

                u_calc_moveamt(mvl_wtcap);
                settrack();

                gm.moves++;
                /*
                 * Never allow 'moves' to grow big enough to wrap.
                 * We don't care what the maximum possible 'long int'
                 * is for the current configuration, we want a value
                 * that is the same for all viable configurations.
                 * When imposing the limit, use a mystic decimal value
                 * instead of a magic binary one such as 0x7fffffffL.
                 */
                if (gm.moves >= 1000000000L) {
                    display_nhwindow(WIN_MESSAGE, TRUE);
                    urgent_pline("The dungeon capitulates.");
                    done(ESCAPED);
                }
                /* 'moves' is misnamed; it represents turns; hero_seq is
                   a value that is distinct every time the hero moves */
                gh.hero_seq = gm.moves << 3;

                if (flags.time && !gc.context.run)
                    disp.time_botl = TRUE; /* 'moves' just changed */

                /********************************/
                /* once-per-turn things go here */
                /********************************/

                l_nhcore_call(NHCORE_MOVELOOP_TURN);

                if (Glib)
                    glibr();
                nh_timeout();
                run_regions();

                if (u.ublesscnt)
                    u.ublesscnt--;

                /* One possible result of prayer is healing.  Whether or
                 * not you get healed depends on your current hit points.
                 * If you are allowed to regenerate during the prayer,
                 * the end-of-prayer calculation messes up on this.
                 * Another possible result is rehumanization, which
                 * requires that encumbrance and movement rate be
                 * recalculated.
                 */
                if (u.uinvulnerable) {
                    /* for the moment at least, you're in tiptop shape */
                    mvl_wtcap = UNENCUMBERED;
                } else if (!Upolyd ? (u.uhp < u.uhpmax)
                           : (u.mh < u.mhmax
                              || gy.youmonst.data->mlet == S_EEL)) {
                    /* maybe heal */
                    regen_hp(mvl_wtcap);
                }

                /* moving around while encumbered is hard work */
                if (mvl_wtcap > MOD_ENCUMBER && u.umoved) {
                    if (!(mvl_wtcap < EXT_ENCUMBER ? gm.moves % 30
                          : gm.moves % 10)) {
                        overexert_hp();
                    }
                }

                regen_pw(mvl_wtcap);

                if (!u.uinvulnerable) {
                    if (Teleportation && !rn2(85)) {
                        coordxy old_ux = u.ux, old_uy = u.uy;

                        tele();
                        if (u.ux != old_ux || u.uy != old_uy) {
                            if (!next_to_u()) {
                                check_leash(old_ux, old_uy);
                            }
                            /* clear doagain keystrokes */
                            cmdq_clear(CQ_CANNED);
                            cmdq_clear(CQ_REPEAT);
                        }
                    }
                    /* delayed change may not be valid anymore */
                    if ((mvl_change == 1 && !Polymorph)
                        || (mvl_change == 2 && u.ulycn == NON_PM))
                        mvl_change = 0;
                    if (Polymorph && !rn2(100))
                        mvl_change = 1;
                    else if (ismnum(u.ulycn) && !Upolyd
                             && !rn2(80 - (20 * night())))
                        mvl_change = 2;
                    if (mvl_change && !Unchanging) {
                        if (gm.multi >= 0) {
                            stop_occupation();
                            if (mvl_change == 1)
                                polyself(POLY_NOFLAGS);
                            else
                                you_were();
                            mvl_change = 0;
                        }
                    }
                }

                if (!gl.level.flags.noautosearch && Searching && gm.multi >= 0)
                    (void) dosearch0(1);
                if (Warning)
                    warnreveal();
                mkot_trap_warn();
                dosounds();
                do_storms();
                gethungry();
                age_spells();
                exerchk();
                invault();
                if (u.uhave.amulet)
                    amulet();
                if (!rn2(40 + (int) (ACURR(A_DEX) * 3)))
                    u_wipe_engr(rnd(3));
                if (u.uevent.udemigod && !u.uinvulnerable) {
                    if (u.udg_cnt)
                        u.udg_cnt--;
                    if (!u.udg_cnt) {
                        intervene();
                        u.udg_cnt = rn1(200, 50);
                    }
                }
/* XXX This should be recoded to use something like regions - a list of
 * things that are active and need to be handled that is dynamically
 * maintained and not a list of special cases. */
                /* vision will be updated as bubbles move */
                if (Is_waterlevel(&u.uz) || Is_airlevel(&u.uz))
                    movebubbles();
                else if (gl.level.flags.fumaroles)
                    fumaroles();

                /* when immobile, count is in turns */
                if (gm.multi < 0) {
                    runmode_delay_output();
                    if (++gm.multi == 0) { /* finished yet? */
                        unmul((char *) 0);
                        /* if unmul caused a level change, take it now */
                        if (u.utotype)
                            deferred_goto();
                    }
                }
            }
        } while (u.umovement < NORMAL_SPEED); /* hero can't move */

        /******************************************/
        /* once-per-hero-took-time things go here */
        /******************************************/

        gh.hero_seq++; /* moves*8 + n for n == 1..7 */

        /* although we checked for encumbrance above, we need to
           check again for message purposes, as the weight of
           inventory may have changed in, e.g., nh_timeout(); we do
           need two checks here so that the player gets feedback
           immediately if their own action encumbered them */
        (void) encumber_msg();

#ifdef STATUS_HILITES
        if (iflags.hilite_delta)
            status_eval_next_unhilite();
#endif
        if (gm.moves >= gc.context.seer_turn) {
            if ((u.uhave.amulet || Clairvoyant) && !In_endgame(&u.uz)
                && !BClairvoyant)
                do_vicinity_map((struct obj *) 0);
            /* we maintain this counter even when clairvoyance isn't
               taking place; on average, go again 30 turns from now */
            gc.context.seer_turn = gm.moves + (long) rn1(31, 15); /*15..45*/
            /* [it used to be that on every 15th turn, there was a 50%
               chance of farsight, so it could happen as often as every
               15 turns or theoretically never happen at all; but when
               a fast hero got multiple moves on that 15th turn, it
               could actually happen more than once on the same turn!] */
        }
        /* [fast hero who gets multiple moves per turn ends up sinking
           multiple times per turn; is that what we really want?] */
        if (u.utrap && u.utraptype == TT_LAVA)
            sink_into_lava();
        /* when/if hero escapes from lava, he can't just stay there */
        else if (!u.umoved)
            (void) pooleffects(FALSE);

        /* vision while buried or underwater is updated here */
        if (Underwater)
            under_water(0);
        else if (u.uburied)
            under_ground(0);

    } /* actual time passed */

    /****************************************/
    /* once-per-player-input things go here */
    /****************************************/

    clear_splitobjs();
    find_ac();
    if (!gc.context.mv || Blind) {
        /* redo monsters if hallu or wearing a helm of telepathy */
        if (Hallucination) { /* update screen randomly */
            see_monsters();
            see_objects();
            see_traps();
            if (u.uswallow)
                swallowed(0);
        } else if (Unblind_telepat) {
            see_monsters();
        } else if (Warning || Warn_of_mon)
            see_monsters();

        if (gv.vision_full_recalc)
            vision_recalc(0); /* vision! */
    }
    if (disp.botl || disp.botlx) {
        bot();
        curs_on_u();
    } else if (disp.time_botl) {
        timebot();
        curs_on_u();
    }

    m_everyturn_effect(&gy.youmonst);

    gc.context.move = 1;

    if (gm.multi >= 0 && go.occupation) {
#if defined(MICRO) || defined(WIN32CON)
        mvl_abort_lev = 0;
        if (kbhit()) {
            char ch;

            if ((ch = pgetchar()) == ABORT)
                mvl_abort_lev++;
            else
                cmdq_add_key(CQ_CANNED, ch);
        }
        if (!mvl_abort_lev && (*go.occupation)() == 0)
#else
            if ((*go.occupation)() == 0)
#endif
                go.occupation = 0;
        if (
#if defined(MICRO) || defined(WIN32)
            mvl_abort_lev ||
#endif
            monster_nearby()) {
            stop_occupation();
            reset_eat();
        }
        runmode_delay_output();
        return;
    }

#ifdef CLIPPING
    /* just before rhack */
    cliparound(u.ux, u.uy);
#endif

    u.umoved = FALSE;

    if (gm.multi > 0) {
        lookaround();
        runmode_delay_output();
        if (!gm.multi) {
            /* lookaround may clear multi */
            gc.context.move = 0;
            return;
        }
        if (gc.context.mv) {
            if (gm.multi < COLNO && !--gm.multi)
                end_running(TRUE);
            domove();
        } else {
            --gm.multi;
            nhassert(gc.command_count != 0);
            rhack(gc.cmd_key);
        }
    } else if (gm.multi == 0) {
#ifdef MAIL
        ckmailstatus();
#endif
        rhack(0);
    }
    if (u.utotype)       /* change dungeon level */
        deferred_goto(); /* after rhack() */

    if (gv.vision_full_recalc)
        vision_recalc(0); /* vision! */
    /* when running in non-tport mode, this gets done through domove() */
    if ((!gc.context.run || flags.runmode == RUN_TPORT)
        && (gm.multi && (!gc.context.travel ? !(gm.multi % 7)
                        : !(gm.moves % 7L)))) {
        if (flags.time && gc.context.run)
            disp.botl = TRUE;
        /* [should this be flush_screen() instead?] */
        display_nhwindow(WIN_MAP, FALSE);
    }

    if (gl.luacore && nhcb_counts[NHCB_END_TURN]) {
        lua_getglobal(gl.luacore, "nh_callback_run");
        lua_pushstring(gl.luacore, nhcb_name[NHCB_END_TURN]);
        nhl_pcall_handle(gl.luacore, 1, 0, "moveloop_core", NHLpa_panic);
        lua_settop(gl.luacore, 0);
    }
}

staticfn void
maybe_do_tutorial(void)
{
    s_level *sp = find_level("tut-1");

    if (!sp)
        return;

    if (ask_do_tutorial()) {
        assign_level(&u.ucamefrom, &u.uz);
        iflags.nofollowers = TRUE;
        schedule_goto(&sp->dlevel, UTOTYPE_NONE,
                      "Entering the tutorial.", (char *) 0);
        deferred_goto();
        vision_recalc(0);
        docrt();
        iflags.nofollowers = FALSE;
    }
}

void
moveloop(boolean resuming)
{
    moveloop_preamble(resuming);

    if (!resuming)
        maybe_do_tutorial();

    for (;;) {
        moveloop_core();
    }
}

staticfn void
regen_pw(int wtcap)
{
    if (u.uen < u.uenmax
        && ((wtcap < MOD_ENCUMBER
             && (!(gm.moves % ((MAXULEV + 8 - u.ulevel)
                              * (Role_if(PM_WIZARD) ? 3 : 4)
                              / 6)))) || Energy_regeneration)) {
        int upper = (int) (ACURR(A_WIS) + ACURR(A_INT)) / 15 + 1;

        u.uen += rn1(upper, 1);
        if (u.uen > u.uenmax)
            u.uen = u.uenmax;
        disp.botl = TRUE;
        if (u.uen == u.uenmax)
            interrupt_multi("You feel full of energy.");
    }
}

#define U_CAN_REGEN() (Regeneration || (Sleepy && u.usleep))

/* maybe recover some lost health (or lose some when an eel out of water) */
staticfn void
regen_hp(int wtcap)
{
    int heal = 0;
    boolean reached_full = FALSE,
            encumbrance_ok = (wtcap < MOD_ENCUMBER || !u.umoved);

    if (Upolyd) {
        if (u.mh < 1) { /* shouldn't happen... */
            rehumanize();
        } else if (gy.youmonst.data->mlet == S_EEL
                   && !is_pool(u.ux, u.uy) && !Is_waterlevel(&u.uz)
                   && !Breathless) {
            /* eel out of water loses hp, similar to monster eels;
               as hp gets lower, rate of further loss slows down */
            if (u.mh > 1 && !Regeneration && rn2(u.mh) > rn2(8)
                && (!Half_physical_damage || !(gm.moves % 2L)))
                heal = -1;
        } else if (u.mh < u.mhmax) {
            if (U_CAN_REGEN() || (encumbrance_ok && !(gm.moves % 20L)))
                heal = 1;
        }
        if (heal) {
            disp.botl = TRUE;
            u.mh += heal;
            reached_full = (u.mh == u.mhmax);
        }

    /* !Upolyd */
    } else {
        /* [when this code was in-line within moveloop(), there was
           no !Upolyd check here, so poly'd hero recovered lost u.uhp
           once u.mh reached u.mhmax; that may have been convenient
           for the player, but it didn't make sense for gameplay...] */
        if (u.uhp < u.uhpmax && (encumbrance_ok || U_CAN_REGEN())) {
            heal = (u.ulevel + (int)ACURR(A_CON)) > rn2(100);

            if (U_CAN_REGEN())
                heal += 1;
            if (Sleepy && u.usleep)
                heal++;

            if (heal) {
                disp.botl = TRUE;
                u.uhp += heal;
                if (u.uhp > u.uhpmax)
                    u.uhp = u.uhpmax;
                /* stop voluntary multi-turn activity if now fully healed */
                reached_full = (u.uhp == u.uhpmax);
            }
        }
    }

    if (reached_full)
        interrupt_multi("You are in full health.");
}

#undef U_CAN_REGEN

void
stop_occupation(void)
{
    if (go.occupation) {
        if (!maybe_finished_meal(TRUE))
            You("stop %s.", go.occtxt);
        go.occupation = (int (*)(void)) 0;
        disp.botl = TRUE; /* in case u.uhs changed */
        nomul(0);
    } else if (gm.multi >= 0) {
        nomul(0);
    }
    cmdq_clear(CQ_CANNED);
}

void
init_sound_disp_gamewindows(void)
{
    int menu_behavior = MENU_BEHAVE_STANDARD;

    activate_chosen_soundlib();

    if (iflags.wc_splash_screen && !flags.randomall) {
        SoundAchievement(0, sa2_splashscreen, 0);
        /* ToDo: new splash screen invocation will go here */
    } else {
        SoundAchievement(0, sa2_newgame_nosplash, 0);
    }

#ifdef CHANGE_COLOR
    /* init_nhwindows() has already been called, so before
       creating the windows, check to see if there are any
       palette entries to alter */
    change_palette();
#endif

    WIN_MESSAGE = create_nhwindow(NHW_MESSAGE);
    if (VIA_WINDOWPORT()) {
        status_initialize(0);
    } else {
        WIN_STATUS = create_nhwindow(NHW_STATUS);
    }
    WIN_MAP = create_nhwindow(NHW_MAP);
    WIN_INVEN = create_nhwindow(NHW_MENU);
    if (WIN_INVEN != WIN_ERR)
        adjust_menu_promptstyle(WIN_INVEN, &iflags.menu_headings);

#ifdef TTY_PERM_INVENT
    if (WINDOWPORT(tty) && WIN_INVEN != WIN_ERR) {
        menu_behavior = MENU_BEHAVE_PERMINV;
        prepare_perminvent(WIN_INVEN);
    }
#endif
    /* in case of early quit where WIN_INVEN could be destroyed before
       ever having been used, use it here to pacify the Qt interface */
    start_menu(WIN_INVEN, menu_behavior), end_menu(WIN_INVEN, (char *) 0);

#ifdef MAC
    /* This _is_ the right place for this - maybe we will
     * have to split init_sound_disp_gamewindows into
     * create_gamewindows and show_gamewindows to get rid of this ifdef...
     */
    if (!strcmp(windowprocs.name, "mac"))
        SanePositions();
#endif

    /*
     * The mac port is not DEPENDENT on the order of these
     * displays, but it looks a lot better this way...
     */
#ifndef STATUS_HILITES
    display_nhwindow(WIN_STATUS, FALSE);
#endif
    display_nhwindow(WIN_MESSAGE, FALSE);
    clear_glyph_buffer();
    display_nhwindow(WIN_MAP, FALSE);
 }

void
newgame(void)
{
    int i;

    /* make sure welcome messages are given before noticing monsters */
    notice_mon_off();
    disp.botlx = TRUE;
    gc.context.ident = 1;
    gc.context.warnlevel = 1;
    gc.context.next_attrib_check = 600L; /* arbitrary first setting */
    gc.context.tribute.enabled = TRUE;   /* turn on 3.6 tributes    */
    gc.context.tribute.tributesz = sizeof(struct tribute_info);

    for (i = LOW_PM; i < NUMMONS; i++)
        gm.mvitals[i].mvflags = mons[i].geno & G_NOCORPSE;

    init_objects(); /* must be before u_init() */

    flags.pantheon = -1; /* role_init() will reset this */
    role_init();         /* must be before init_dungeons(), u_init(),
                          * and init_artifacts() */

    init_dungeons();  /* must be before u_init() to avoid rndmonst()
                       * creating odd monsters for any tins and eggs
                       * in hero's initial inventory */
    init_artifacts(); /* before u_init() in case $WIZKIT specifies
                       * any artifacts */
    u_init();

    l_nhcore_init();  /* create a Lua state that lasts until the end of the game */
    reset_glyphmap(gm_newgame);
#ifndef NO_SIGNAL
    (void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
#ifdef NEWS
    if (iflags.news)
        display_file(NEWS, FALSE);
#endif
    /* quest_init();  --  Now part of role_init() */

    mklev();
    u_on_upstairs();
    if (wizard)
        obj_delivery(FALSE); /* finish wizkit */
    vision_reset();          /* set up internals for level (after mklev) */
    check_special_room(FALSE);

    if (MON_AT(u.ux, u.uy))
        mnexto(m_at(u.ux, u.uy), RLOC_NOMSG);
    (void) makedog();
    docrt();

    if (flags.legacy) {
        flush_screen(1);
        com_pager("legacy");
    }

    urealtime.realtime = 0L;
    urealtime.start_timing = getnow();
#ifdef INSURANCE
    save_currentstate();
#endif
    gp.program_state.something_worth_saving++; /* useful data now exists */

    /* Success! */
    welcome(TRUE);
    notice_mon_on(); /* now we can notice monsters */
    if (a11y.glyph_updates)
        (void) dolookaround();
    else
        notice_all_mons(TRUE);
    return;
}

/* show "welcome [back] to nethack" message at program startup */
void
welcome(boolean new_game) /* false => restoring an old game */
{
    char buf[BUFSZ];
    boolean currentgend = Upolyd ? u.mfemale : flags.female;

    l_nhcore_call(new_game ? NHCORE_START_NEW_GAME : NHCORE_RESTORE_OLD_GAME);

    /* skip "welcome back" if restoring a doomed character */
    if (!new_game && Upolyd && ugenocided()) {
        /* death via self-genocide is pending */
        pline("You're back, but you still feel %s inside.", udeadinside());
        return;
    }

    if (Hallucination)
        pline("NetHack is filmed in front of an undead studio audience.");

    /*
     * The "welcome back" message always describes your innate form
     * even when polymorphed or wearing a helm of opposite alignment.
     * Alignment is shown unconditionally for new games; for restores
     * it's only shown if it has changed from its original value.
     * Sex is shown for new games except when it is redundant; for
     * restores it's only shown if different from its original value.
     */
    *buf = '\0';
    if (new_game || u.ualignbase[A_ORIGINAL] != u.ualignbase[A_CURRENT])
        Sprintf(eos(buf), " %s", align_str(u.ualignbase[A_ORIGINAL]));
    if (!gu.urole.name.f
        && (new_game
                ? (gu.urole.allow & ROLE_GENDMASK) == (ROLE_MALE | ROLE_FEMALE)
                : currentgend != flags.initgend))
        Sprintf(eos(buf), " %s", genders[currentgend].adj);
    Sprintf(eos(buf), " %s %s", gu.urace.adj,
            (currentgend && gu.urole.name.f) ? gu.urole.name.f : gu.urole.name.m);

    pline(new_game ? "%s %s, welcome to NetHack!  You are a%s."
                   : "%s %s, the%s, welcome back to NetHack!",
          Hello((struct monst *) 0), gp.plname, buf);

    if (new_game) {
        /* guarantee that 'major' event category is never empty */
        livelog_printf(LL_ACHIEVE, "%s the%s entered the dungeon",
                       gp.plname, buf);
    } else {
        /* if restoring in Gehennom, give same hot/smoky message as when
           first entering it */
        hellish_smoke_mesg();
        /* remind player of the level annotation, like in goto_level() */
        print_level_annotation();
    }
}

#ifdef POSITIONBAR
staticfn void
do_positionbar(void)
{
    /* FIXME: this will break if any coordinate is too big for (char);
       the sys/msdos/vid*.c code uses (unsigned char) which is less
       vulnerable but not guaranteed to be able to hold coordxy values;
       also, there doesn't appear to be any need for this to be static,
       nor to contain pairs of (> or <) and x; it could just be a full
       line of spaces and > or < characters with update_positionbar()
       revised to reconstruct the x values for non-space characters */
    static char pbar[COLNO];
    char *p;
    stairway *stway;
    coordxy x, y;
    int glyph, symbol;

    p = pbar;
    /* TODO: use the same method as getpos() so objects don't cover stairs */
    /* FIXME: traversing 'stairs' list ignores mimics that pose as stairs */
    for (stway = gs.stairs; stway; stway = stway->next) {
        x = stway->sx;
        y = stway->sy;
        glyph = levl[x][y].glyph;
        symbol = glyph_to_cmap(glyph);

        if (is_cmap_stairs(symbol)) {
            *p++ = (stway->up ? '<' : '>');
            *p++ = (char) x;
        }
     }

    /* hero location */
    if (u.ux) {
        *p++ = '@';
        *p++ = u.ux;
    }
    /* fence post */
    *p = 0;

    update_positionbar(pbar);
}
#endif

staticfn void
interrupt_multi(const char *msg)
{
    if (gm.multi > 0 && !gc.context.travel && !gc.context.run) {
        nomul(0);
        if (flags.verbose && msg)
            Norep("%s", msg);
    }
}

/*
 * Argument processing helpers - for xxmain() to share
 * and call.
 *
 * These should return TRUE if the argument matched,
 * whether the processing of the argument was
 * successful or not.
 *
 * Most of these do their thing, then after returning
 * to xxmain(), the code exits without starting a game.
 *
 */

static const struct early_opt earlyopts[] = {
    { ARG_DEBUG, "debug", 5, TRUE },
    { ARG_VERSION, "version", 4, TRUE },
    { ARG_SHOWPATHS, "showpaths", 8, FALSE },
#ifndef NODUMPENUMS
    { ARG_DUMPENUMS, "dumpenums", 9, FALSE },
#endif
    { ARG_DUMPGLYPHIDS, "dumpglyphids", 12, FALSE },
#ifdef WIN32
    { ARG_WINDOWS, "windows", 4, TRUE },
#endif
#if defined(CRASHREPORT)
    { ARG_BIDSHOW, "bidshow", 7, FALSE },
#endif
};

#ifdef WIN32
extern int windows_early_options(const char *);
#endif

/*
 * Returns:
 *    0 = no match
 *    1 = found and skip past this argument
 *    2 = found and trigger immediate exit
 */
int
argcheck(int argc, char *argv[], enum earlyarg e_arg)
{
    int i, idx;
    boolean match = FALSE;
    char *userea = (char *) 0;
    const char *dashdash = "";

    for (idx = 0; idx < SIZE(earlyopts); idx++) {
        if (earlyopts[idx].e == e_arg){
            break;
	}
    }
    if (idx >= SIZE(earlyopts) || argc < 1)
        return 0;

    for (i = 0; i < argc; ++i) {
        if (argv[i][0] != '-')
            continue;
        if (argv[i][1] == '-') {
            userea = &argv[i][2];
            dashdash = "-";
        } else {
            userea = &argv[i][1];
        }
        match = match_optname(userea, earlyopts[idx].name,
                              earlyopts[idx].minlength,
                              earlyopts[idx].valallowed);
        if (match)
            break;
    }

    if (match) {
        const char *extended_opt = strchr(userea, ':');

        if (!extended_opt)
            extended_opt = strchr(userea, '=');
        switch(e_arg) {
        case ARG_DEBUG:
            if (extended_opt) {
                extended_opt++;
                debug_fields(extended_opt);
            }
            return 1;
        case ARG_VERSION: {
            boolean insert_into_pastebuf = FALSE;

            if (extended_opt) {
                extended_opt++;
                    /* Deprecated in favor of "copy" - remove no later
                       than  next major version */
                if (match_optname(extended_opt, "paste", 5, FALSE)) {
                    insert_into_pastebuf = TRUE;
                } else if (match_optname(extended_opt, "copy", 4, FALSE)) {
                    insert_into_pastebuf = TRUE;
                } else if (match_optname(extended_opt, "dump", 4, FALSE)) {
                    /* version number plus enabled features and sanity
                       values that the program compares against the same
                       thing recorded in save and bones files to check
                       whether they're being used compatibly */
                    dump_version_info();
                    return 2; /* done */
                } else if (!match_optname(extended_opt, "show", 4, FALSE)) {
                    raw_printf("-%sversion can only be extended with"
                               " -%sversion:copy or :dump or :show.\n",
                               dashdash, dashdash);
                    /* exit after we've reported bad command line argument */
                    return 2;
                }
            }
            early_version_info(insert_into_pastebuf);
            return 2;
        }
        case ARG_SHOWPATHS:
            return 2;
#ifndef NODUMPENUMS
        case ARG_DUMPENUMS:
            dump_enums();
            return 2;
#endif
        case ARG_DUMPGLYPHIDS:
            dump_glyphids();
            return 2;
#ifdef CRASHREPORT
        case ARG_BIDSHOW:
            crashreport_bidshow();
            return 2;
#endif
#ifdef WIN32
        case ARG_WINDOWS:
            if (extended_opt) {
                extended_opt++;
                return windows_early_options(extended_opt);
            }
        /*FALLTHRU*/
#endif
        default:
            break;
        }
    };
    return 0;
}

/*
 * These are internal controls to aid developers with
 * testing and debugging particular aspects of the code.
 * They are not player options and the only place they
 * are documented is right here. No gameplay is altered.
 *
 * test             - test whether this parser is working
 * ttystatus        - TTY:
 * immediateflips   - WIN32: turn off display performance
 *                    optimization so that display output
 *                    can be debugged without buffering.
 */
staticfn void
debug_fields(const char *opts)
{
    char *op;
    boolean negated = FALSE;

    while ((op = strchr(opts, ',')) != 0) {
        *op++ = 0;
        /* recurse */
        debug_fields(op);
    }
    if (strlen(opts) > BUFSZ / 2)
        return;


    /* strip leading and trailing white space */
    while (isspace((uchar) *opts))
        opts++;
    op = eos((char *) opts);
    while (--op >= opts && isspace((uchar) *op))
        *op = '\0';

    if (!*opts) {
        /* empty */
        return;
    }
    while ((*opts == '!') || !strncmpi(opts, "no", 2)) {
        if (*opts == '!')
            opts++;
        else
            opts += 2;
        negated = !negated;
    }
    if (match_optname(opts, "test", 4, FALSE))
        iflags.debug.test = negated ? FALSE : TRUE;
#ifdef TTY_GRAPHICS
    if (match_optname(opts, "ttystatus", 9, FALSE))
        iflags.debug.ttystatus = negated ? FALSE : TRUE;
#endif
#ifdef WIN32
    if (match_optname(opts, "immediateflips", 14, FALSE))
        iflags.debug.immediateflips = negated ? FALSE : TRUE;
#endif
    return;
}

/* convert from time_t to number of seconds */
long
timet_to_seconds(time_t ttim)
{
    /* for Unix-based and Posix-compliant systems, a cast to 'long' would
       suffice but the C Standard doesn't require time_t to be that simple */
    return timet_delta(ttim, (time_t) 0);
}

/* calculate the difference in seconds between two time_t values */
long
timet_delta(time_t etim, time_t stim) /* end and start times */
{
    /* difftime() is a STDC routine which returns the number of seconds
       between two time_t values as a 'double' */
    return (long) difftime(etim, stim);
}

#if !defined(NODUMPENUMS)
/* monsdump[] and objdump[] are also used in utf8map.c */

#define DUMP_ENUMS
struct enum_dump monsdump[] = {
#include "monsters.h"
    { NUMMONS, "NUMMONS" },
    { NON_PM, "NON_PM" },
    { LOW_PM, "LOW_PM" },
    { HIGH_PM, "HIGH_PM" },
    { SPECIAL_PM, "SPECIAL_PM" }
};
struct enum_dump objdump[] = {
#include "objects.h"
    { NUM_OBJECTS, "NUM_OBJECTS" },
};

#define DUMP_ENUMS_PCHAR
static struct enum_dump defsym_cmap_dump[] = {
#include "defsym.h"
    { MAXPCHARS, "MAXPCHARS" },
};
#undef DUMP_ENUMS_PCHAR

#define DUMP_ENUMS_MONSYMS
static struct enum_dump defsym_mon_syms_dump[] = {
#include "defsym.h"
    { MAXMCLASSES, "MAXMCLASSES" },
};
#undef DUMP_ENUMS_MONSYMS

#define DUMP_ENUMS_MONSYMS_DEFCHAR
static struct enum_dump defsym_mon_defchars_dump[] = {
#include "defsym.h"
};
#undef DUMP_ENUMS_MONSYMS_DEFCHAR

#define DUMP_ENUMS_OBJCLASS_DEFCHARS
static struct enum_dump objclass_defchars_dump[] = {
#include "defsym.h"
};
#undef DUMP_ENUMS_OBJCLASS_DEFCHARS

#define DUMP_ENUMS_OBJCLASS_CLASSES
static struct enum_dump objclass_classes_dump[] = {
#include "defsym.h"
    { MAXOCLASSES, "MAXOCLASSES" },
};
#undef DUMP_ENUMS_OBJCLASS_CLASSES

#define DUMP_ENUMS_OBJCLASS_SYMS
static struct enum_dump objclass_syms_dump[] = {
#include "defsym.h"
};
#undef DUMP_ENUMS_OBJCLASS_SYMS

#define DUMP_ARTI_ENUM
static struct enum_dump arti_enum_dump[] = {
#include "artilist.h"
    { AFTER_LAST_ARTIFACT, "AFTER_LAST_ARTIFACT" }
};
#undef DUMP_ARTI_ENUM

#undef DUMP_ENUMS


#ifndef NODUMPENUMS

staticfn void
dump_enums(void)
{
    enum enum_dumps {
        monsters_enum,
        objects_enum,
        objects_misc_enum,
        defsym_cmap_enum,
        defsym_mon_syms_enum,
        defsym_mon_defchars_enum,
        objclass_defchars_enum,
        objclass_classes_enum,
        objclass_syms_enum,
        arti_enum,
        NUM_ENUM_DUMPS
    };
    static const char *const titles[NUM_ENUM_DUMPS] = {
        "monnums", "objects_nums" , "misc_object_nums",
        "cmap_symbols", "mon_syms", "mon_defchars",
        "objclass_defchars", "objclass_classes",
        "objclass_syms", "artifacts_nums",
    };

#define dump_om(om) { om, #om }
    static const struct enum_dump omdump[] = {
        dump_om(LAST_GENERIC),
        dump_om(OBJCLASS_HACK),
        dump_om(FIRST_OBJECT),
        dump_om(FIRST_AMULET),
        dump_om(LAST_AMULET),
        dump_om(FIRST_SPELL),
        dump_om(LAST_SPELL),
        dump_om(MAXSPELL),
        dump_om(FIRST_REAL_GEM),
        dump_om(LAST_REAL_GEM),
        dump_om(FIRST_GLASS_GEM),
        dump_om(LAST_GLASS_GEM),
        dump_om(NUM_REAL_GEMS),
        dump_om(NUM_GLASS_GEMS),
        dump_om(MAX_GLYPH),
    };
#undef dump_om
    static const struct enum_dump *const ed[NUM_ENUM_DUMPS] = {
        monsdump, objdump, omdump,
        defsym_cmap_dump, defsym_mon_syms_dump,
        defsym_mon_defchars_dump,
        objclass_defchars_dump,
        objclass_classes_dump,
        objclass_syms_dump,
        arti_enum_dump,
    };
    static const char *const pfx[NUM_ENUM_DUMPS] = { "PM_", "", "",
                                                     "", "", "", "",
                                                     "", "", "" };
    /* 0 = dump numerically only, 1 = add 'char' comment */
    static const int dumpflgs[NUM_ENUM_DUMPS] = { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0 };
    static int szd[NUM_ENUM_DUMPS] = { SIZE(monsdump), SIZE(objdump),
                                       SIZE(omdump), SIZE(defsym_cmap_dump),
                                       SIZE(defsym_mon_syms_dump),
                                       SIZE(defsym_mon_defchars_dump),
                                       SIZE(objclass_defchars_dump),
                                       SIZE(objclass_classes_dump),
                                       SIZE(objclass_syms_dump),
                                       SIZE(arti_enum_dump),
    };
    const char *nmprefix;
    int i, j, nmwidth;
    char comment[BUFSZ];

    for (i = 0; i < NUM_ENUM_DUMPS; ++ i) {
        raw_printf("enum %s = {", titles[i]);
        for (j = 0; j < szd[i]; ++j) {
            int unprefixed_count = (i == monsters_enum) ? 4 : 1;
            nmprefix = (j >= szd[i] - unprefixed_count)
                           ? "" : pfx[i]; /* "" or "PM_" */
            nmwidth = 27 - (int) strlen(nmprefix); /* 27 or 24 */
            if (dumpflgs[i] > 0) {
                Snprintf(comment, sizeof comment,
                         "    /* '%c' */",
                         (ed[i][j].val >= 32 && ed[i][j].val <= 126)
                         ? ed[i][j].val : ' ');
            } else {
                comment[0] = '\0';
            }
            raw_printf("    %s%*s = %3d,%s",
                       nmprefix, -nmwidth,
                       ed[i][j].nm, ed[i][j].val,
                       comment);
       }
        raw_print("};");
        raw_print("");
    }
    raw_print("");
}
#endif /* NODUMPENUMS */

void
dump_glyphids(void)
{
    dump_all_glyphids(stdout);
}
#endif /* !NODUMPENUMS */

/*allmain.c*/

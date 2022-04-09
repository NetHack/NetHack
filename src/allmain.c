/* NetHack 3.7	allmain.c	$NHDT-Date: 1646136934 2022/03/01 12:15:34 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.178 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

/* various code that was replicated in *main.c */

#include "hack.h"
#include <ctype.h>

#ifndef NO_SIGNAL
#include <signal.h>
#endif

static void moveloop_preamble(boolean);
static void u_calc_moveamt(int);
#ifdef POSITIONBAR
static void do_positionbar(void);
#endif
static void regen_pw(int);
static void regen_hp(int);
static void interrupt_multi(const char *);
static void debug_fields(const char *);

void
early_init(void)
{
    decl_globals_init();
    objects_globals_init();
    monst_globals_init();
    sys_early_init();
    runtime_info_init();
}

static void
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
        g.context.rndencode = rnd(9000);
        set_wear((struct obj *) 0); /* for side-effects of starting gear */
        reset_justpicked(g.invent);
        (void) pickup(1);      /* autopickup at initial location */
        /* only matters if someday a character is able to start with
           clairvoyance (wizard with cornuthaum perhaps?); without this,
           first "random" occurrence would always kick in on turn 1 */
        g.context.seer_turn = (long) rnd(30);
    }
    g.context.botlx = TRUE; /* for STATUS_HILITES */
    if (resuming) { /* restoring old game */
        read_engr_at(u.ux, u.uy); /* subset of pickup() */
        fix_shop_damage();
    }

    (void) encumber_msg(); /* in case they auto-picked up something */
    if (g.defer_see_monsters) {
        g.defer_see_monsters = FALSE;
        see_monsters();
    }
    initrack();

    u.uz0.dlevel = u.uz.dlevel;
    g.youmonst.movement = NORMAL_SPEED; /* give hero some movement points */
    g.context.move = 0;

    g.program_state.in_moveloop = 1;
    /* for perm_invent preset at startup, display persistent inventory after
       invent is fully populated and the in_moveloop flag has been set */
    if (iflags.perm_invent)
        update_inventory();
}

static void
u_calc_moveamt(int wtcap)
{
    int moveamt = 0;

    /* calculate how much time passed. */
    if (u.usteed && u.umoved) {
        /* your speed doesn't augment steed's speed */
        moveamt = mcalcmove(u.usteed, TRUE);
    } else {
        moveamt = g.youmonst.data->mmove;

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

    g.youmonst.movement += moveamt;
    if (g.youmonst.movement < 0)
        g.youmonst.movement = 0;
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
    if (g.program_state.done_hup)
        end_of_input();
#endif
    get_nh_event();
#ifdef POSITIONBAR
    do_positionbar();
#endif

    if (g.context.bypasses)
        clear_bypasses();

    if (iflags.sanity_check || iflags.debug_fuzzer)
        sanity_check();

    if (g.context.move) {
        /* actual time passed */
        g.youmonst.movement -= NORMAL_SPEED;

        do { /* hero can't move this turn loop */
            mvl_wtcap = encumber_msg();

            g.context.mon_moving = TRUE;
            do {
                monscanmove = movemon();
                if (g.youmonst.movement >= NORMAL_SPEED)
                    break; /* it's now your turn */
            } while (monscanmove);
            g.context.mon_moving = FALSE;

            if (!monscanmove && g.youmonst.movement < NORMAL_SPEED) {
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

                g.moves++;
                /*
                 * Never allow 'moves' to grow big enough to wrap.
                 * We don't care what the maximum possible 'long int'
                 * is for the current configuration, we want a value
                 * that is the same for all viable configurations.
                 * When imposing the limit, use a mystic decimal value
                 * instead of a magic binary one such as 0x7fffffffL.
                 */
                if (g.moves >= 1000000000L) {
                    display_nhwindow(WIN_MESSAGE, TRUE);
                    urgent_pline("The dungeon capitulates.");
                    done(ESCAPED);
                }
                /* 'moves' is misnamed; it represents turns; hero_seq is
                   a value that is distinct every time the hero moves */
                g.hero_seq = g.moves << 3;

                if (flags.time && !g.context.run)
                    iflags.time_botl = TRUE; /* 'moves' just changed */

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
                              || g.youmonst.data->mlet == S_EEL)) {
                    /* maybe heal */
                    regen_hp(mvl_wtcap);
                }

                /* moving around while encumbered is hard work */
                if (mvl_wtcap > MOD_ENCUMBER && u.umoved) {
                    if (!(mvl_wtcap < EXT_ENCUMBER ? g.moves % 30
                          : g.moves % 10)) {
                        overexert_hp();
                    }
                }

                regen_pw(mvl_wtcap);

                if (!u.uinvulnerable) {
                    if (Teleportation && !rn2(85)) {
                        xchar old_ux = u.ux, old_uy = u.uy;

                        tele();
                        if (u.ux != old_ux || u.uy != old_uy) {
                            if (!next_to_u()) {
                                check_leash(old_ux, old_uy);
                            }
                            /* clear doagain keystrokes */
                            pushch(0);
                            savech(0);
                        }
                    }
                    /* delayed change may not be valid anymore */
                    if ((mvl_change == 1 && !Polymorph)
                        || (mvl_change == 2 && u.ulycn == NON_PM))
                        mvl_change = 0;
                    if (Polymorph && !rn2(100))
                        mvl_change = 1;
                    else if (u.ulycn >= LOW_PM && !Upolyd
                             && !rn2(80 - (20 * night())))
                        mvl_change = 2;
                    if (mvl_change && !Unchanging) {
                        if (g.multi >= 0) {
                            stop_occupation();
                            if (mvl_change == 1)
                                polyself(0);
                            else
                                you_were();
                            mvl_change = 0;
                        }
                    }
                }

                if (Searching && g.multi >= 0)
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
                /* underwater and waterlevel vision are done here */
                if (Is_waterlevel(&u.uz) || Is_airlevel(&u.uz))
                    movebubbles();
                else if (Is_firelevel(&u.uz))
                    fumaroles();
                else if (Underwater)
                    under_water(0);
                /* vision while buried done here */
                else if (u.uburied)
                    under_ground(0);

                /* when immobile, count is in turns */
                if (g.multi < 0) {
                    runmode_delay_output();
                    if (++g.multi == 0) { /* finished yet? */
                        unmul((char *) 0);
                        /* if unmul caused a level change, take it now */
                        if (u.utotype)
                            deferred_goto();
                    }
                }
            }
        } while (g.youmonst.movement < NORMAL_SPEED); /* hero can't move */

        /******************************************/
        /* once-per-hero-took-time things go here */
        /******************************************/

        g.hero_seq++; /* moves*8 + n for n == 1..7 */

        /* although we checked for encumberance above, we need to
           check again for message purposes, as the weight of
           inventory may have changed in, e.g., nh_timeout(); we do
           need two checks here so that the player gets feedback
           immediately if their own action encumbered them */
        (void) encumber_msg();

#ifdef STATUS_HILITES
        if (iflags.hilite_delta)
            status_eval_next_unhilite();
#endif
        if (g.moves >= g.context.seer_turn) {
            if ((u.uhave.amulet || Clairvoyant) && !In_endgame(&u.uz)
                && !BClairvoyant)
                do_vicinity_map((struct obj *) 0);
            /* we maintain this counter even when clairvoyance isn't
               taking place; on average, go again 30 turns from now */
            g.context.seer_turn = g.moves + (long) rn1(31, 15); /*15..45*/
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

    } /* actual time passed */

    /****************************************/
    /* once-per-player-input things go here */
    /****************************************/

    clear_splitobjs();
    find_ac();
    if (!g.context.mv || Blind) {
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

        if (g.vision_full_recalc)
            vision_recalc(0); /* vision! */
    }
    if (g.context.botl || g.context.botlx) {
        bot();
        curs_on_u();
    } else if (iflags.time_botl) {
        timebot();
        curs_on_u();
    }

    g.context.move = 1;

    if (g.multi >= 0 && g.occupation) {
#if defined(MICRO) || defined(WIN32)
        mvl_abort_lev = 0;
        if (kbhit()) {
            char ch;

            if ((ch = pgetchar()) == ABORT)
                mvl_abort_lev++;
            else
                pushch(ch);
        }
        if (!mvl_abort_lev && (*g.occupation)() == 0)
#else
            if ((*g.occupation)() == 0)
#endif
                g.occupation = 0;
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

    if (g.multi > 0) {
        lookaround();
        runmode_delay_output();
        if (!g.multi) {
            /* lookaround may clear multi */
            g.context.move = 0;
            return;
        }
        if (g.context.mv) {
            if (g.multi < COLNO && !--g.multi)
                end_running(TRUE);
            domove();
        } else {
            --g.multi;
            nhassert(g.command_count != 0);
            rhack(g.command_line);
        }
    } else if (g.multi == 0) {
#ifdef MAIL
        ckmailstatus();
#endif
        rhack((char *) 0);
    }
    if (u.utotype)       /* change dungeon level */
        deferred_goto(); /* after rhack() */

    if (g.vision_full_recalc)
        vision_recalc(0); /* vision! */
    /* when running in non-tport mode, this gets done through domove() */
    if ((!g.context.run || flags.runmode == RUN_TPORT)
        && (g.multi && (!g.context.travel ? !(g.multi % 7)
                        : !(g.moves % 7L)))) {
        if (flags.time && g.context.run)
            g.context.botl = TRUE;
        /* [should this be flush_screen() instead?] */
        display_nhwindow(WIN_MAP, FALSE);
    }
}

void
moveloop(boolean resuming)
{
    moveloop_preamble(resuming);
    for (;;) {
        moveloop_core();
    }
}

static void
regen_pw(int wtcap)
{
    if (u.uen < u.uenmax
        && ((wtcap < MOD_ENCUMBER
             && (!(g.moves % ((MAXULEV + 8 - u.ulevel)
                              * (Role_if(PM_WIZARD) ? 3 : 4)
                              / 6)))) || Energy_regeneration)) {
        int upper = (int) (ACURR(A_WIS) + ACURR(A_INT)) / 15 + 1;

        u.uen += rn1(upper, 1);
        if (u.uen > u.uenmax)
            u.uen = u.uenmax;
        g.context.botl = TRUE;
        if (u.uen == u.uenmax)
            interrupt_multi("You feel full of energy.");
    }
}

#define U_CAN_REGEN() (Regeneration || (Sleepy && u.usleep))

/* maybe recover some lost health (or lose some when an eel out of water) */
static void
regen_hp(int wtcap)
{
    int heal = 0;
    boolean reached_full = FALSE,
            encumbrance_ok = (wtcap < MOD_ENCUMBER || !u.umoved);

    if (Upolyd) {
        if (u.mh < 1) { /* shouldn't happen... */
            rehumanize();
        } else if (g.youmonst.data->mlet == S_EEL
                   && !is_pool(u.ux, u.uy) && !Is_waterlevel(&u.uz)
                   && !Breathless) {
            /* eel out of water loses hp, similar to monster eels;
               as hp gets lower, rate of further loss slows down */
            if (u.mh > 1 && !Regeneration && rn2(u.mh) > rn2(8)
                && (!Half_physical_damage || !(g.moves % 2L)))
                heal = -1;
        } else if (u.mh < u.mhmax) {
            if (U_CAN_REGEN() || (encumbrance_ok && !(g.moves % 20L)))
                heal = 1;
        }
        if (heal) {
            g.context.botl = TRUE;
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
            if (u.ulevel > 9) {
                if (!(g.moves % 3L)) {
                    int Con = (int) ACURR(A_CON);

                    if (Con <= 12) {
                        heal = 1;
                    } else {
                        heal = rnd(Con);
                        if (heal > u.ulevel - 9)
                            heal = u.ulevel - 9;
                    }
                }
            } else { /* u.ulevel <= 9 */
                if (!(g.moves % (long) ((MAXULEV + 12) / (u.ulevel + 2) + 1)))
                    heal = 1;
            }
            if (U_CAN_REGEN() && !heal)
                heal = 1;
            if (Sleepy && u.usleep)
                heal++;

            if (heal) {
                g.context.botl = TRUE;
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
    if (g.occupation) {
        if (!maybe_finished_meal(TRUE))
            You("stop %s.", g.occtxt);
        g.occupation = 0;
        g.context.botl = TRUE; /* in case u.uhs changed */
        nomul(0);
        pushch(0);
    } else if (g.multi >= 0) {
        nomul(0);
    }
    cmdq_clear();
}

void
display_gamewindows(void)
{
    WIN_MESSAGE = create_nhwindow(NHW_MESSAGE);
    if (VIA_WINDOWPORT()) {
        status_initialize(0);
    } else {
        WIN_STATUS = create_nhwindow(NHW_STATUS);
    }
    WIN_MAP = create_nhwindow(NHW_MAP);
    WIN_INVEN = create_nhwindow(NHW_MENU);
    /* in case of early quit where WIN_INVEN could be destroyed before
       ever having been used, use it here to pacify the Qt interface */
    start_menu(WIN_INVEN, 0U), end_menu(WIN_INVEN, (char *) 0);

#ifdef MAC
    /* This _is_ the right place for this - maybe we will
     * have to split display_gamewindows into create_gamewindows
     * and show_gamewindows to get rid of this ifdef...
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

    g.context.botlx = TRUE;
    g.context.ident = 1;
    g.context.warnlevel = 1;
    g.context.next_attrib_check = 600L; /* arbitrary first setting */
    g.context.tribute.enabled = TRUE;   /* turn on 3.6 tributes    */
    g.context.tribute.tributesz = sizeof(struct tribute_info);

    for (i = LOW_PM; i < NUMMONS; i++)
        g.mvitals[i].mvflags = mons[i].geno & G_NOCORPSE;

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

    l_nhcore_init();
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
    g.program_state.something_worth_saving++; /* useful data now exists */

    /* Success! */
    welcome(TRUE);
    return;
}

/* show "welcome [back] to nethack" message at program startup */
void
welcome(boolean new_game) /* false => restoring an old game */
{
    char buf[BUFSZ];
    boolean currentgend = Upolyd ? u.mfemale : flags.female;

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
    if (!g.urole.name.f
        && (new_game
                ? (g.urole.allow & ROLE_GENDMASK) == (ROLE_MALE | ROLE_FEMALE)
                : currentgend != flags.initgend))
        Sprintf(eos(buf), " %s", genders[currentgend].adj);
    Sprintf(eos(buf), " %s %s", g.urace.adj,
            (currentgend && g.urole.name.f) ? g.urole.name.f : g.urole.name.m);

    pline(new_game ? "%s %s, welcome to NetHack!  You are a%s."
                   : "%s %s, the%s, welcome back to NetHack!",
          Hello((struct monst *) 0), g.plname, buf);

    l_nhcore_call(new_game ? NHCORE_START_NEW_GAME : NHCORE_RESTORE_OLD_GAME);
    if (new_game) /* guarantee that 'major' event category is never empty */
        livelog_printf(LL_ACHIEVE, "%s the%s entered the dungeon",
                       g.plname, buf);
}

#ifdef POSITIONBAR
static void
do_positionbar(void)
{
    static char pbar[COLNO];
    char *p;
    stairway *stway = g.stairs;

    p = pbar;
    /* TODO: use the same method as getpos() so objects don't cover stairs */
    while (stway) {
        int x = stway->sx;
        int y = stway->sy;
        int glyph = glyph_to_cmap(g.level.locations[x][y].glyph);

        if (is_cmap_stairs(glyph)) {
            *p++ = (stway->up ? '<' : '>');
            *p++ = stway->sx;
        }
        stway = stway->next;
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

static void
interrupt_multi(const char *msg)
{
    if (g.multi > 0 && !g.context.travel && !g.context.run) {
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
    {ARG_DEBUG, "debug", 5, TRUE},
    {ARG_VERSION, "version", 4, TRUE},
    {ARG_SHOWPATHS, "showpaths", 9, FALSE},
#ifndef NODUMPENUMS
    {ARG_DUMPENUMS, "dumpenums", 9, FALSE},
#endif
#ifdef WIN32
    {ARG_WINDOWS, "windows", 4, TRUE},
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
        if (earlyopts[idx].e == e_arg)
            break;
    }
    if (idx >= SIZE(earlyopts) || argc < 1)
        return FALSE;

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
        const char *extended_opt = index(userea, ':');

        if (!extended_opt)
            extended_opt = index(userea, '=');
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
                if (match_optname(extended_opt, "paste", 5, FALSE)) {
                    insert_into_pastebuf = TRUE;
                } else {
                    raw_printf(
                   "-%sversion can only be extended with -%sversion:paste.\n",
                               dashdash, dashdash);
                    return TRUE;
                }
            }
            early_version_info(insert_into_pastebuf);
            return 2;
        }
        case ARG_SHOWPATHS: {
            return 2;
        }
#ifndef NODUMPENUMS
        case ARG_DUMPENUMS: {
            dump_enums();
            return 2;
        }
#endif
#ifdef WIN32
        case ARG_WINDOWS: {
            if (extended_opt) {
                extended_opt++;
                return windows_early_options(extended_opt);
            }
        }
#endif
        default:
            break;
        }
    };
    return FALSE;
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
static void
debug_fields(const char *opts)
{
    char *op;
    boolean negated = FALSE;

    while ((op = index(opts, ',')) != 0) {
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

#ifndef NODUMPENUMS
void
dump_enums(void)
{
    int i, j;
    enum enum_dumps {
        monsters_enum,
        objects_enum,
        objects_misc_enum,
        NUM_ENUM_DUMPS
    };
    static const char *const titles[NUM_ENUM_DUMPS] =
        { "monnums", "objects_nums" , "misc_object_nums" };
    struct enum_dump {
        int val;
        const char *nm;
    };

#define DUMP_ENUMS
    struct enum_dump monsdump[] = {
#include "monsters.h"
        { NUMMONS, "NUMMONS" },
    };
    struct enum_dump objdump[] = {
#include "objects.h"
        { NUM_OBJECTS, "NUM_OBJECTS" },
    };
#undef DUMP_ENUMS

    struct enum_dump omdump[] = {
            { LAST_GEM, "LAST_GEM" },
            { NUM_GLASS_GEMS, "NUM_GLASS_GEMS" },
            { MAXSPELL, "MAXSPELL" },
    };
    struct enum_dump *ed[NUM_ENUM_DUMPS] = { monsdump, objdump, omdump };
    static const char *const pfx[NUM_ENUM_DUMPS] = { "PM_", "", "" };
    int szd[NUM_ENUM_DUMPS] = { SIZE(monsdump), SIZE(objdump), SIZE(omdump) };

    for (i = 0; i < NUM_ENUM_DUMPS; ++ i) {
        raw_printf("enum %s = {", titles[i]);
        for (j = 0; j < szd[i]; ++j) {
            raw_printf("    %s%s = %i%s",
                       (j == szd[i] - 1) ? "" : pfx[i],
                       ed[i]->nm,
                       ed[i]->val,
                       (j == szd[i] - 1) ? "" : ",");
            ed[i]++;
       }
        raw_print("};");
        raw_print("");
    }
    raw_print("");
}
#endif /* NODUMPENUMS */

/*allmain.c*/

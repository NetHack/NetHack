/* NetHack 3.6	wintty.c	$NHDT-Date: 1526909614 2018/05/21 13:33:34 $  $NHDT-Branch: NetHack-3.6.2 $:$NHDT-Revision: 1.167 $ */
/* Copyright (c) David Cohrs, 1991                                */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Neither a standard out nor character-based control codes should be
 * part of the "tty look" windowing implementation.
 * h+ 930227
 */

/* It's still not clear I've caught all the cases for H2344.  #undef this
 * to back out the changes. */
#define H2344_BROKEN

#include "hack.h"

#ifdef TTY_GRAPHICS
#include "dlb.h"

#ifdef MAC
#define MICRO /* The Mac is a MICRO only for this file, not in general! */
#ifdef THINK_C
extern void msmsg(const char *, ...);
#endif
#endif

#ifndef NO_TERMS
#include "tcap.h"
#endif

#include "wintty.h"

#ifdef CLIPPING /* might want SIGWINCH */
#if defined(BSD) || defined(ULTRIX) || defined(AIX_31) || defined(_BULL_SOURCE)
#include <signal.h>
#endif
#endif

#ifdef TTY_TILES_ESCCODES
extern short glyph2tile[];
#define TILE_ANSI_COMMAND 'z'
#define AVTC_GLYPH_START   0
#define AVTC_GLYPH_END     1
#define AVTC_SELECT_WINDOW 2
#define AVTC_INLINE_SYNC   3
#endif

extern char mapped_menu_cmds[]; /* from options.c */

/* this is only needed until tty_status_* routines are written */
extern NEARDATA winid WIN_STATUS;

/* Interface definition, for windows.c */
struct window_procs tty_procs = {
    "tty",
    (0
#ifdef MSDOS
     | WC_TILED_MAP | WC_ASCII_MAP
#endif
#if defined(WIN32CON)
     | WC_MOUSE_SUPPORT
#endif
     | WC_COLOR | WC_HILITE_PET | WC_INVERSE | WC_EIGHT_BIT_IN),
    (0
#if defined(SELECTSAVED)
     | WC2_SELECTSAVED
#endif
#if defined(STATUS_HILITES)
     | WC2_HILITE_STATUS | WC2_HITPOINTBAR | WC2_FLUSH_STATUS
     | WC2_RESET_STATUS
#endif
     | WC2_DARKGRAY),
    tty_init_nhwindows, tty_player_selection, tty_askname, tty_get_nh_event,
    tty_exit_nhwindows, tty_suspend_nhwindows, tty_resume_nhwindows,
    tty_create_nhwindow, tty_clear_nhwindow, tty_display_nhwindow,
    tty_destroy_nhwindow, tty_curs, tty_putstr, genl_putmixed,
    tty_display_file, tty_start_menu, tty_add_menu, tty_end_menu,
    tty_select_menu, tty_message_menu, tty_update_inventory, tty_mark_synch,
    tty_wait_synch,
#ifdef CLIPPING
    tty_cliparound,
#endif
#ifdef POSITIONBAR
    tty_update_positionbar,
#endif
    tty_print_glyph, tty_raw_print, tty_raw_print_bold, tty_nhgetch,
    tty_nh_poskey, tty_nhbell, tty_doprev_message, tty_yn_function,
    tty_getlin, tty_get_ext_cmd, tty_number_pad, tty_delay_output,
#ifdef CHANGE_COLOR /* the Mac uses a palette device */
    tty_change_color,
#ifdef MAC
    tty_change_background, set_tty_font_name,
#endif
    tty_get_color_string,
#endif

    /* other defs that really should go away (they're tty specific) */
    tty_start_screen, tty_end_screen, genl_outrip,
#if defined(WIN32)
    nttty_preference_update,
#else
    genl_preference_update,
#endif
    tty_getmsghistory, tty_putmsghistory,
    tty_status_init,
    genl_status_finish, tty_status_enablefield,
#ifdef STATUS_HILITES
    tty_status_update,
#else
    genl_status_update,
#endif
    genl_can_suspend_yes,
};

static int maxwin = 0; /* number of windows in use */
winid BASE_WINDOW;
struct WinDesc *wins[MAXWIN];
struct DisplayDesc *ttyDisplay; /* the tty display descriptor */

extern void FDECL(cmov, (int, int));   /* from termcap.c */
extern void FDECL(nocmov, (int, int)); /* from termcap.c */
#if defined(UNIX) || defined(VMS)
static char obuf[BUFSIZ]; /* BUFSIZ is defined in stdio.h */
#endif

static char winpanicstr[] = "Bad window id %d";
char defmorestr[] = "--More--";

#ifdef CLIPPING
#if defined(USE_TILES) && defined(MSDOS)
boolean clipping = FALSE; /* clipping on? */
int clipx = 0, clipxmax = 0;
#else
static boolean clipping = FALSE; /* clipping on? */
static int clipx = 0, clipxmax = 0;
#endif
static int clipy = 0, clipymax = 0;
#endif /* CLIPPING */

#if defined(USE_TILES) && defined(MSDOS)
extern void FDECL(adjust_cursor_flags, (struct WinDesc *));
#endif

#if defined(ASCIIGRAPH) && !defined(NO_TERMS)
boolean GFlag = FALSE;
boolean HE_resets_AS; /* see termcap.c */
#endif

#if defined(MICRO) || defined(WIN32CON)
static const char to_continue[] = "to continue";
#define getret() getreturn(to_continue)
#else
STATIC_DCL void NDECL(getret);
#endif
STATIC_DCL void FDECL(erase_menu_or_text,
                      (winid, struct WinDesc *, BOOLEAN_P));
STATIC_DCL void FDECL(free_window_info, (struct WinDesc *, BOOLEAN_P));
STATIC_DCL void FDECL(dmore, (struct WinDesc *, const char *));
STATIC_DCL void FDECL(set_item_state, (winid, int, tty_menu_item *));
STATIC_DCL void FDECL(set_all_on_page, (winid, tty_menu_item *,
                                        tty_menu_item *));
STATIC_DCL void FDECL(unset_all_on_page, (winid, tty_menu_item *,
                                          tty_menu_item *));
STATIC_DCL void FDECL(invert_all_on_page, (winid, tty_menu_item *,
                                           tty_menu_item *, CHAR_P));
STATIC_DCL void FDECL(invert_all, (winid, tty_menu_item *,
                                   tty_menu_item *, CHAR_P));
STATIC_DCL void FDECL(toggle_menu_attr, (BOOLEAN_P, int, int));
STATIC_DCL void FDECL(process_menu_window, (winid, struct WinDesc *));
STATIC_DCL void FDECL(process_text_window, (winid, struct WinDesc *));
STATIC_DCL tty_menu_item *FDECL(reverse, (tty_menu_item *));
STATIC_DCL const char *FDECL(compress_str, (const char *));
STATIC_DCL void FDECL(tty_putsym, (winid, int, int, CHAR_P));
STATIC_DCL void FDECL(bail, (const char *)); /* __attribute__((noreturn)) */
STATIC_DCL void FDECL(setup_rolemenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(setup_racemenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(setup_gendmenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(setup_algnmenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL boolean NDECL(reset_role_filtering);
#ifdef STATUS_HILITES
STATIC_DCL boolean FDECL(check_fields, (BOOLEAN_P, int *, int *));
STATIC_DCL void NDECL(render_status);
STATIC_DCL void FDECL(tty_putstatusfield, (struct tty_status_fields *,
                                           const char *, int, int));
STATIC_DCL boolean NDECL(check_windowdata);
STATIC_DCL int NDECL(condition_size);
STATIC_DCL int FDECL(make_things_fit, (BOOLEAN_P));
STATIC_DCL void FDECL(shrink_enc, (int));
STATIC_DCL void FDECL(shrink_dlvl, (int));
STATIC_DCL void NDECL(do_setlast);
#endif

/*
 * A string containing all the default commands -- to add to a list
 * of acceptable inputs.
 */
static const char default_menu_cmds[] = {
    MENU_FIRST_PAGE,    MENU_LAST_PAGE,   MENU_NEXT_PAGE,
    MENU_PREVIOUS_PAGE, MENU_SELECT_ALL,  MENU_UNSELECT_ALL,
    MENU_INVERT_ALL,    MENU_SELECT_PAGE, MENU_UNSELECT_PAGE,
    MENU_INVERT_PAGE,   MENU_SEARCH,      0 /* null terminator */
};

#ifdef TTY_TILES_ESCCODES
static int vt_tile_current_window = -2;

void
print_vt_code(i, c, d)
int i, c, d;
{
    if (iflags.vt_tiledata) {
        if (c >= 0) {
            if (i == AVTC_SELECT_WINDOW) {
                if (c == vt_tile_current_window)
                    return;
                vt_tile_current_window = c;
            }
            if (d >= 0)
                printf("\033[1;%d;%d;%d%c", i, c, d, TILE_ANSI_COMMAND);
            else
                printf("\033[1;%d;%d%c", i, c, TILE_ANSI_COMMAND);
        } else {
            printf("\033[1;%d%c", i, TILE_ANSI_COMMAND);
        }
    }
}
#else
# define print_vt_code(i, c, d) ;
#endif /* !TTY_TILES_ESCCODES */
#define print_vt_code1(i)     print_vt_code((i), -1, -1)
#define print_vt_code2(i,c)   print_vt_code((i), (c), -1)
#define print_vt_code3(i,c,d) print_vt_code((i), (c), (d))


/* clean up and quit */
STATIC_OVL void
bail(mesg)
const char *mesg;
{
    clearlocks();
    tty_exit_nhwindows(mesg);
    nh_terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}

#if defined(SIGWINCH) && defined(CLIPPING)
STATIC_DCL void FDECL(winch_handler, (int));

    /*
     * This really ought to just set a flag like the hangup handler does,
     * then check the flag at "safe" times, in case the signal arrives
     * while something fragile is executing.  Better to have a brief period
     * where display updates don't fit the new size than for tty internals
     * to become corrupted.
     *
     * 'winch_seen' has been "notyet" for a long time....
     */
/*ARGUSED*/
STATIC_OVL void
winch_handler(sig_unused) /* signal handler is called with at least 1 arg */
int sig_unused UNUSED;
{
    int oldLI = LI, oldCO = CO, i;
    register struct WinDesc *cw;

#ifdef WINCHAIN
    {
#define WINCH_MESSAGE "(SIGWINCH)"
        if (wc_tracelogf)
            (void) write(fileno(wc_tracelogf), WINCH_MESSAGE,
                         strlen(WINCH_MESSAGE));
#undef WINCH_MESSAGE
    }
#endif
    getwindowsz();
    /* For long running events such as multi-page menus and
     * display_file(), we note the signal's occurance and
     * hope the code there decides to handle the situation
     * and reset the flag. There will be race conditions
     * when handling this - code handlers so it doesn't matter.
     */
#ifdef notyet
    winch_seen = TRUE;
#endif
    if ((oldLI != LI || oldCO != CO) && ttyDisplay) {
        ttyDisplay->rows = LI;
        ttyDisplay->cols = CO;

        cw = wins[BASE_WINDOW];
        cw->rows = ttyDisplay->rows;
        cw->cols = ttyDisplay->cols;

        if (iflags.window_inited) {
            cw = wins[WIN_MESSAGE];
            cw->curx = cw->cury = 0;

            tty_destroy_nhwindow(WIN_STATUS);
            WIN_STATUS = tty_create_nhwindow(NHW_STATUS);

            if (u.ux) {
#ifdef CLIPPING
                if (CO < COLNO || LI < ROWNO + 3) {
                    setclipped();
                    tty_cliparound(u.ux, u.uy);
                } else {
                    clipping = FALSE;
                    clipx = clipy = 0;
                }
#endif
                i = ttyDisplay->toplin;
                ttyDisplay->toplin = 0;
                docrt();
                bot();
                ttyDisplay->toplin = i;
                flush_screen(1);
                if (i) {
                    addtopl(toplines);
                } else
                    for (i = WIN_INVEN; i < MAXWIN; i++)
                        if (wins[i] && wins[i]->active) {
                            /* cop-out */
                            addtopl("Press Return to continue: ");
                            break;
                        }
                (void) fflush(stdout);
                if (i < 2)
                    flush_screen(1);
            }
        }
    }
}
#endif

/*ARGSUSED*/
void
tty_init_nhwindows(argcp, argv)
int *argcp UNUSED;
char **argv UNUSED;
{
    int wid, hgt, i;

/*
 *  Remember tty modes, to be restored on exit.
 *
 *  gettty() must be called before tty_startup()
 *    due to ordering of LI/CO settings
 *  tty_startup() must be called before initoptions()
 *    due to ordering of graphics settings
 */
#if defined(UNIX) || defined(VMS)
    setbuf(stdout, obuf);
#endif
    gettty();

    /* to port dependant tty setup */
    tty_startup(&wid, &hgt);
    setftty(); /* calls start_screen */

    /* set up tty descriptor */
    ttyDisplay = (struct DisplayDesc *) alloc(sizeof (struct DisplayDesc));
    ttyDisplay->toplin = 0;
    ttyDisplay->rows = hgt;
    ttyDisplay->cols = wid;
    ttyDisplay->curx = ttyDisplay->cury = 0;
    ttyDisplay->inmore = ttyDisplay->inread = ttyDisplay->intr = 0;
    ttyDisplay->dismiss_more = 0;
#ifdef TEXTCOLOR
    ttyDisplay->color = NO_COLOR;
#endif
    ttyDisplay->attrs = 0;

    /* set up the default windows */
    BASE_WINDOW = tty_create_nhwindow(NHW_BASE);
    wins[BASE_WINDOW]->active = 1;

    ttyDisplay->lastwin = WIN_ERR;

#if defined(SIGWINCH) && defined(CLIPPING) && !defined(NO_SIGNAL)
    (void) signal(SIGWINCH, (SIG_RET_TYPE) winch_handler);
#endif

    tty_clear_nhwindow(BASE_WINDOW);

    tty_putstr(BASE_WINDOW, 0, "");
    for (i = 1; i <= 4; ++i)
        tty_putstr(BASE_WINDOW, 0, copyright_banner_line(i));
    tty_putstr(BASE_WINDOW, 0, "");
    tty_display_nhwindow(BASE_WINDOW, FALSE);
}

/* try to reduce clutter in the code below... */
#define ROLE flags.initrole
#define RACE flags.initrace
#define GEND flags.initgend
#define ALGN flags.initalign

void
tty_player_selection()
{
    int i, k, n, choice, nextpick;
    boolean getconfirmation, picksomething;
    char pick4u = 'n';
    char pbuf[QBUFSZ], plbuf[QBUFSZ];
    winid win;
    anything any;
    menu_item *selected = 0;

    /* Used to avoid "Is this ok?" if player has already specified all
     * four facets of role.
     * Note that rigid_role_checks might force any unspecified facets to
     * have a specific value, but that will still require confirmation;
     * player can specify the forced ones if avoiding that is demanded.
     */
    picksomething = (ROLE == ROLE_NONE || RACE == ROLE_NONE
                     || GEND == ROLE_NONE || ALGN == ROLE_NONE);
    /* Used for '-@';
     * choose randomly without asking for all unspecified facets.
     */
    if (flags.randomall && picksomething) {
        if (ROLE == ROLE_NONE)
            ROLE = ROLE_RANDOM;
        if (RACE == ROLE_NONE)
            RACE = ROLE_RANDOM;
        if (GEND == ROLE_NONE)
            GEND = ROLE_RANDOM;
        if (ALGN == ROLE_NONE)
            ALGN = ROLE_RANDOM;
    }

    /* prevent unnecessary prompting if role forces race (samurai) or gender
       (valkyrie) or alignment (rogue), or race forces alignment (orc), &c */
    rigid_role_checks();

    /* Should we randomly pick for the player? */
    if (ROLE == ROLE_NONE || RACE == ROLE_NONE || GEND == ROLE_NONE
        || ALGN == ROLE_NONE) {
        int echoline;
        char *prompt = build_plselection_prompt(pbuf, QBUFSZ,
                                                ROLE, RACE, GEND, ALGN);

        /* this prompt string ends in "[ynaq]?":
           y - game picks role,&c then asks player to confirm;
           n - player manually chooses via menu selections;
           a - like 'y', but skips confirmation and starts game;
           q - quit
         */
        tty_putstr(BASE_WINDOW, 0, "");
        echoline = wins[BASE_WINDOW]->cury;
        tty_putstr(BASE_WINDOW, 0, prompt);
        do {
            pick4u = lowc(readchar());
            if (index(quitchars, pick4u))
                pick4u = 'y';
        } while (!index(ynaqchars, pick4u));
        if ((int) strlen(prompt) + 1 < CO) {
            /* Echo choice and move back down line */
            tty_putsym(BASE_WINDOW, (int) strlen(prompt) + 1, echoline,
                       pick4u);
            tty_putstr(BASE_WINDOW, 0, "");
        } else
            /* Otherwise it's hard to tell where to echo, and things are
             * wrapping a bit messily anyway, so (try to) make sure the next
             * question shows up well and doesn't get wrapped at the
             * bottom of the window.
             */
            tty_clear_nhwindow(BASE_WINDOW);

        if (pick4u != 'y' && pick4u != 'a' && pick4u != 'n')
            goto give_up;
    }

makepicks:
    nextpick = RS_ROLE;
    do {
        if (nextpick == RS_ROLE) {
            nextpick = RS_RACE;
            /* Select a role, if necessary;
               we'll try to be compatible with pre-selected
               race/gender/alignment, but may not succeed. */
            if (ROLE < 0) {
                /* Process the choice */
                if (pick4u == 'y' || pick4u == 'a' || ROLE == ROLE_RANDOM) {
                    /* Pick a random role */
                    k = pick_role(RACE, GEND, ALGN, PICK_RANDOM);
                    if (k < 0) {
                        tty_putstr(BASE_WINDOW, 0, "Incompatible role!");
                        k = randrole();
                    }
                } else {
                    /* Prompt for a role */
                    tty_clear_nhwindow(BASE_WINDOW);
                    role_selection_prolog(RS_ROLE, BASE_WINDOW);
                    win = create_nhwindow(NHW_MENU);
                    start_menu(win);
                    /* populate the menu with role choices */
                    setup_rolemenu(win, TRUE, RACE, GEND, ALGN);
                    /* add miscellaneous menu entries */
                    role_menu_extra(ROLE_RANDOM, win, TRUE);
                    any = zeroany; /* separator, not a choice */
                    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
                             MENU_UNSELECTED);
                    role_menu_extra(RS_RACE, win, FALSE);
                    role_menu_extra(RS_GENDER, win, FALSE);
                    role_menu_extra(RS_ALGNMNT, win, FALSE);
                    if (gotrolefilter())
                        role_menu_extra(RS_filter, win, FALSE);
                    role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                    Strcpy(pbuf, "Pick a role or profession");
                    end_menu(win, pbuf);
                    n = select_menu(win, PICK_ONE, &selected);
                    /*
                     * PICK_ONE with preselected choice behaves strangely:
                     *  n == -1 -- <escape>, so use quit choice;
                     *  n ==  0 -- explicitly chose preselected entry,
                     *             toggling it off, so use it;
                     *  n ==  1 -- implicitly chose preselected entry
                     *             with <space> or <return>;
                     *  n ==  2 -- explicitly chose a different entry, so
                     *             both it and preselected one are in list.
                     */
                    if (n > 0) {
                        choice = selected[0].item.a_int;
                        if (n > 1 && choice == ROLE_RANDOM)
                            choice = selected[1].item.a_int;
                    } else
                        choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                    if (selected)
                        free((genericptr_t) selected), selected = 0;
                    destroy_nhwindow(win);

                    if (choice == ROLE_NONE) {
                        goto give_up; /* Selected quit */
                    } else if (choice == RS_menu_arg(RS_ALGNMNT)) {
                        ALGN = k = ROLE_NONE;
                        nextpick = RS_ALGNMNT;
                    } else if (choice == RS_menu_arg(RS_GENDER)) {
                        GEND = k = ROLE_NONE;
                        nextpick = RS_GENDER;
                    } else if (choice == RS_menu_arg(RS_RACE)) {
                        RACE = k = ROLE_NONE;
                        nextpick = RS_RACE;
                    } else if (choice == RS_menu_arg(RS_filter)) {
                        ROLE = k = ROLE_NONE;
                        (void) reset_role_filtering();
                        nextpick = RS_ROLE;
                    } else if (choice == ROLE_RANDOM) {
                        k = pick_role(RACE, GEND, ALGN, PICK_RANDOM);
                        if (k < 0)
                            k = randrole();
                    } else {
                        k = choice - 1;
                    }
                }
                ROLE = k;
            } /* needed role */
        }     /* picking role */

        if (nextpick == RS_RACE) {
            nextpick = (ROLE < 0) ? RS_ROLE : RS_GENDER;
            /* Select a race, if necessary;
               force compatibility with role, try for compatibility
               with pre-selected gender/alignment. */
            if (RACE < 0 || !validrace(ROLE, RACE)) {
                /* no race yet, or pre-selected race not valid */
                if (pick4u == 'y' || pick4u == 'a' || RACE == ROLE_RANDOM) {
                    k = pick_race(ROLE, GEND, ALGN, PICK_RANDOM);
                    if (k < 0) {
                        tty_putstr(BASE_WINDOW, 0, "Incompatible race!");
                        k = randrace(ROLE);
                    }
                } else { /* pick4u == 'n' */
                    /* Count the number of valid races */
                    n = 0; /* number valid */
                    k = 0; /* valid race */
                    for (i = 0; races[i].noun; i++)
                        if (ok_race(ROLE, i, GEND, ALGN)) {
                            n++;
                            k = i;
                        }
                    if (n == 0) {
                        for (i = 0; races[i].noun; i++)
                            if (validrace(ROLE, i)) {
                                n++;
                                k = i;
                            }
                    }
                    /* Permit the user to pick, if there is more than one */
                    if (n > 1) {
                        tty_clear_nhwindow(BASE_WINDOW);
                        role_selection_prolog(RS_RACE, BASE_WINDOW);
                        win = create_nhwindow(NHW_MENU);
                        start_menu(win);
                        any = zeroany; /* zero out all bits */
                        /* populate the menu with role choices */
                        setup_racemenu(win, TRUE, ROLE, GEND, ALGN);
                        /* add miscellaneous menu entries */
                        role_menu_extra(ROLE_RANDOM, win, TRUE);
                        any.a_int = 0; /* separator, not a choice */
                        add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
                                 MENU_UNSELECTED);
                        role_menu_extra(RS_ROLE, win, FALSE);
                        role_menu_extra(RS_GENDER, win, FALSE);
                        role_menu_extra(RS_ALGNMNT, win, FALSE);
                        if (gotrolefilter())
                            role_menu_extra(RS_filter, win, FALSE);
                        role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                        Strcpy(pbuf, "Pick a race or species");
                        end_menu(win, pbuf);
                        n = select_menu(win, PICK_ONE, &selected);
                        if (n > 0) {
                            choice = selected[0].item.a_int;
                            if (n > 1 && choice == ROLE_RANDOM)
                                choice = selected[1].item.a_int;
                        } else
                            choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                        if (selected)
                            free((genericptr_t) selected), selected = 0;
                        destroy_nhwindow(win);

                        if (choice == ROLE_NONE) {
                            goto give_up; /* Selected quit */
                        } else if (choice == RS_menu_arg(RS_ALGNMNT)) {
                            ALGN = k = ROLE_NONE;
                            nextpick = RS_ALGNMNT;
                        } else if (choice == RS_menu_arg(RS_GENDER)) {
                            GEND = k = ROLE_NONE;
                            nextpick = RS_GENDER;
                        } else if (choice == RS_menu_arg(RS_ROLE)) {
                            ROLE = k = ROLE_NONE;
                            nextpick = RS_ROLE;
                        } else if (choice == RS_menu_arg(RS_filter)) {
                            RACE = k = ROLE_NONE;
                            if (reset_role_filtering())
                                nextpick = RS_ROLE;
                            else
                                nextpick = RS_RACE;
                        } else if (choice == ROLE_RANDOM) {
                            k = pick_race(ROLE, GEND, ALGN, PICK_RANDOM);
                            if (k < 0)
                                k = randrace(ROLE);
                        } else {
                            k = choice - 1;
                        }
                    }
                }
                RACE = k;
            } /* needed race */
        }     /* picking race */

        if (nextpick == RS_GENDER) {
            nextpick = (ROLE < 0) ? RS_ROLE : (RACE < 0) ? RS_RACE
                       : RS_ALGNMNT;
            /* Select a gender, if necessary;
               force compatibility with role/race, try for compatibility
               with pre-selected alignment. */
            if (GEND < 0 || !validgend(ROLE, RACE, GEND)) {
                /* no gender yet, or pre-selected gender not valid */
                if (pick4u == 'y' || pick4u == 'a' || GEND == ROLE_RANDOM) {
                    k = pick_gend(ROLE, RACE, ALGN, PICK_RANDOM);
                    if (k < 0) {
                        tty_putstr(BASE_WINDOW, 0, "Incompatible gender!");
                        k = randgend(ROLE, RACE);
                    }
                } else { /* pick4u == 'n' */
                    /* Count the number of valid genders */
                    n = 0; /* number valid */
                    k = 0; /* valid gender */
                    for (i = 0; i < ROLE_GENDERS; i++)
                        if (ok_gend(ROLE, RACE, i, ALGN)) {
                            n++;
                            k = i;
                        }
                    if (n == 0) {
                        for (i = 0; i < ROLE_GENDERS; i++)
                            if (validgend(ROLE, RACE, i)) {
                                n++;
                                k = i;
                            }
                    }
                    /* Permit the user to pick, if there is more than one */
                    if (n > 1) {
                        tty_clear_nhwindow(BASE_WINDOW);
                        role_selection_prolog(RS_GENDER, BASE_WINDOW);
                        win = create_nhwindow(NHW_MENU);
                        start_menu(win);
                        any = zeroany; /* zero out all bits */
                        /* populate the menu with gender choices */
                        setup_gendmenu(win, TRUE, ROLE, RACE, ALGN);
                        /* add miscellaneous menu entries */
                        role_menu_extra(ROLE_RANDOM, win, TRUE);
                        any.a_int = 0; /* separator, not a choice */
                        add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
                                 MENU_UNSELECTED);
                        role_menu_extra(RS_ROLE, win, FALSE);
                        role_menu_extra(RS_RACE, win, FALSE);
                        role_menu_extra(RS_ALGNMNT, win, FALSE);
                        if (gotrolefilter())
                            role_menu_extra(RS_filter, win, FALSE);
                        role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                        Strcpy(pbuf, "Pick a gender or sex");
                        end_menu(win, pbuf);
                        n = select_menu(win, PICK_ONE, &selected);
                        if (n > 0) {
                            choice = selected[0].item.a_int;
                            if (n > 1 && choice == ROLE_RANDOM)
                                choice = selected[1].item.a_int;
                        } else
                            choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                        if (selected)
                            free((genericptr_t) selected), selected = 0;
                        destroy_nhwindow(win);

                        if (choice == ROLE_NONE) {
                            goto give_up; /* Selected quit */
                        } else if (choice == RS_menu_arg(RS_ALGNMNT)) {
                            ALGN = k = ROLE_NONE;
                            nextpick = RS_ALGNMNT;
                        } else if (choice == RS_menu_arg(RS_RACE)) {
                            RACE = k = ROLE_NONE;
                            nextpick = RS_RACE;
                        } else if (choice == RS_menu_arg(RS_ROLE)) {
                            ROLE = k = ROLE_NONE;
                            nextpick = RS_ROLE;
                        } else if (choice == RS_menu_arg(RS_filter)) {
                            GEND = k = ROLE_NONE;
                            if (reset_role_filtering())
                                nextpick = RS_ROLE;
                            else
                                nextpick = RS_GENDER;
                        } else if (choice == ROLE_RANDOM) {
                            k = pick_gend(ROLE, RACE, ALGN, PICK_RANDOM);
                            if (k < 0)
                                k = randgend(ROLE, RACE);
                        } else {
                            k = choice - 1;
                        }
                    }
                }
                GEND = k;
            } /* needed gender */
        }     /* picking gender */

        if (nextpick == RS_ALGNMNT) {
            nextpick = (ROLE < 0) ? RS_ROLE : (RACE < 0) ? RS_RACE : RS_GENDER;
            /* Select an alignment, if necessary;
               force compatibility with role/race/gender. */
            if (ALGN < 0 || !validalign(ROLE, RACE, ALGN)) {
                /* no alignment yet, or pre-selected alignment not valid */
                if (pick4u == 'y' || pick4u == 'a' || ALGN == ROLE_RANDOM) {
                    k = pick_align(ROLE, RACE, GEND, PICK_RANDOM);
                    if (k < 0) {
                        tty_putstr(BASE_WINDOW, 0, "Incompatible alignment!");
                        k = randalign(ROLE, RACE);
                    }
                } else { /* pick4u == 'n' */
                    /* Count the number of valid alignments */
                    n = 0; /* number valid */
                    k = 0; /* valid alignment */
                    for (i = 0; i < ROLE_ALIGNS; i++)
                        if (ok_align(ROLE, RACE, GEND, i)) {
                            n++;
                            k = i;
                        }
                    if (n == 0) {
                        for (i = 0; i < ROLE_ALIGNS; i++)
                            if (validalign(ROLE, RACE, i)) {
                                n++;
                                k = i;
                            }
                    }
                    /* Permit the user to pick, if there is more than one */
                    if (n > 1) {
                        tty_clear_nhwindow(BASE_WINDOW);
                        role_selection_prolog(RS_ALGNMNT, BASE_WINDOW);
                        win = create_nhwindow(NHW_MENU);
                        start_menu(win);
                        any = zeroany; /* zero out all bits */
                        setup_algnmenu(win, TRUE, ROLE, RACE, GEND);
                        role_menu_extra(ROLE_RANDOM, win, TRUE);
                        any.a_int = 0; /* separator, not a choice */
                        add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
                                 MENU_UNSELECTED);
                        role_menu_extra(RS_ROLE, win, FALSE);
                        role_menu_extra(RS_RACE, win, FALSE);
                        role_menu_extra(RS_GENDER, win, FALSE);
                        if (gotrolefilter())
                            role_menu_extra(RS_filter, win, FALSE);
                        role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                        Strcpy(pbuf, "Pick an alignment or creed");
                        end_menu(win, pbuf);
                        n = select_menu(win, PICK_ONE, &selected);
                        if (n > 0) {
                            choice = selected[0].item.a_int;
                            if (n > 1 && choice == ROLE_RANDOM)
                                choice = selected[1].item.a_int;
                        } else
                            choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                        if (selected)
                            free((genericptr_t) selected), selected = 0;
                        destroy_nhwindow(win);

                        if (choice == ROLE_NONE) {
                            goto give_up; /* Selected quit */
                        } else if (choice == RS_menu_arg(RS_GENDER)) {
                            GEND = k = ROLE_NONE;
                            nextpick = RS_GENDER;
                        } else if (choice == RS_menu_arg(RS_RACE)) {
                            RACE = k = ROLE_NONE;
                            nextpick = RS_RACE;
                        } else if (choice == RS_menu_arg(RS_ROLE)) {
                            ROLE = k = ROLE_NONE;
                            nextpick = RS_ROLE;
                        } else if (choice == RS_menu_arg(RS_filter)) {
                            ALGN = k = ROLE_NONE;
                            if (reset_role_filtering())
                                nextpick = RS_ROLE;
                            else
                                nextpick = RS_ALGNMNT;
                        } else if (choice == ROLE_RANDOM) {
                            k = pick_align(ROLE, RACE, GEND, PICK_RANDOM);
                            if (k < 0)
                                k = randalign(ROLE, RACE);
                        } else {
                            k = choice - 1;
                        }
                    }
                }
                ALGN = k;
            } /* needed alignment */
        }     /* picking alignment */

    } while (ROLE < 0 || RACE < 0 || GEND < 0 || ALGN < 0);

    /*
     *  Role, race, &c have now been determined;
     *  ask for confirmation and maybe go back to choose all over again.
     *
     *  Uses ynaq for familiarity, although 'a' is usually a
     *  superset of 'y' but here is an alternate form of 'n'.
     *  Menu layout:
     *   title:  Is this ok? [ynaq]
     *   blank:
     *    text:  $name, $alignment $gender $race $role
     *   blank:
     *    menu:  y + yes; play
     *           n - no; pick again
     *   maybe:  a - no; rename hero
     *           q - quit
     *           (end)
     */
    getconfirmation = (picksomething && pick4u != 'a' && !flags.randomall);
    while (getconfirmation) {
        tty_clear_nhwindow(BASE_WINDOW);
        role_selection_prolog(ROLE_NONE, BASE_WINDOW);
        win = create_nhwindow(NHW_MENU);
        start_menu(win);
        any = zeroany; /* zero out all bits */
        any.a_int = 0;
        if (!roles[ROLE].name.f
            && (roles[ROLE].allow & ROLE_GENDMASK)
                   == (ROLE_MALE | ROLE_FEMALE))
            Sprintf(plbuf, " %s", genders[GEND].adj);
        else
            *plbuf = '\0'; /* omit redundant gender */
        Sprintf(pbuf, "%s, %s%s %s %s", plname, aligns[ALGN].adj, plbuf,
                races[RACE].adj,
                (GEND == 1 && roles[ROLE].name.f) ? roles[ROLE].name.f
                                                  : roles[ROLE].name.m);
        add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, pbuf,
                 MENU_UNSELECTED);
        /* blank separator */
        any.a_int = 0;
        add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
        /* [ynaq] menu choices */
        any.a_int = 1;
        add_menu(win, NO_GLYPH, &any, 'y', 0, ATR_NONE, "Yes; start game",
                 MENU_SELECTED);
        any.a_int = 2;
        add_menu(win, NO_GLYPH, &any, 'n', 0, ATR_NONE,
                 "No; choose role again", MENU_UNSELECTED);
        if (iflags.renameallowed) {
            any.a_int = 3;
            add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
                     "Not yet; choose another name", MENU_UNSELECTED);
        }
        any.a_int = -1;
        add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit",
                 MENU_UNSELECTED);
        Sprintf(pbuf, "Is this ok? [yn%sq]", iflags.renameallowed ? "a" : "");
        end_menu(win, pbuf);
        n = select_menu(win, PICK_ONE, &selected);
        /* [pick-one menus with a preselected entry behave oddly...] */
        choice = (n > 0) ? selected[n - 1].item.a_int : (n == 0) ? 1 : -1;
        if (selected)
            free((genericptr_t) selected), selected = 0;
        destroy_nhwindow(win);

        switch (choice) {
        default:          /* 'q' or ESC */
            goto give_up; /* quit */
            break;
        case 3: { /* 'a' */
            /*
             * TODO: what, if anything, should be done if the name is
             * changed to or from "wizard" after port-specific startup
             * code has set flags.debug based on the original name?
             */
            int saveROLE, saveRACE, saveGEND, saveALGN;

            iflags.renameinprogress = TRUE;
            /* plnamesuffix() can change any or all of ROLE, RACE,
               GEND, ALGN; we'll override that and honor only the name */
            saveROLE = ROLE, saveRACE = RACE, saveGEND = GEND,
                saveALGN = ALGN;
            *plname = '\0';
            plnamesuffix(); /* calls askname() when plname[] is empty */
            ROLE = saveROLE, RACE = saveRACE, GEND = saveGEND,
                ALGN = saveALGN;
            break; /* getconfirmation is still True */
        }
        case 2:    /* 'n' */
            /* start fresh, but bypass "shall I pick everything for you?"
               step; any partial role selection via config file, command
               line, or name suffix is discarded this time */
            pick4u = 'n';
            ROLE = RACE = GEND = ALGN = ROLE_NONE;
            goto makepicks;
            break;
        case 1: /* 'y' or Space or Return/Enter */
            /* success; drop out through end of function */
            getconfirmation = FALSE;
            break;
        }
    }

    /* Success! */
    tty_display_nhwindow(BASE_WINDOW, FALSE);
    return;

give_up:
    /* Quit */
    if (selected)
        free((genericptr_t) selected); /* [obsolete] */
    bail((char *) 0);
    /*NOTREACHED*/
    return;
}

STATIC_OVL boolean
reset_role_filtering()
{
    winid win;
    anything any;
    int i, n;
    menu_item *selected = 0;

    win = create_nhwindow(NHW_MENU);
    start_menu(win);
    any = zeroany;

    /* no extra blank line preceding this entry; end_menu supplies one */
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
             "Unacceptable roles", MENU_UNSELECTED);
    setup_rolemenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
             "Unacceptable races", MENU_UNSELECTED);
    setup_racemenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
             "Unacceptable genders", MENU_UNSELECTED);
    setup_gendmenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
             "Uncceptable alignments", MENU_UNSELECTED);
    setup_algnmenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    end_menu(win, "Pick all that apply");
    n = select_menu(win, PICK_ANY, &selected);

    if (n > 0) {
        clearrolefilter();
        for (i = 0; i < n; i++)
            setrolefilter(selected[i].item.a_string);

        ROLE = RACE = GEND = ALGN = ROLE_NONE;
    }
    if (selected)
        free((genericptr_t) selected), selected = 0;
    destroy_nhwindow(win);
    return (n > 0) ? TRUE : FALSE;
}

#undef ROLE
#undef RACE
#undef GEND
#undef ALGN

/* add entries a-Archeologist, b-Barbarian, &c to menu being built in 'win' */
STATIC_OVL void
setup_rolemenu(win, filtering, race, gend, algn)
winid win;
boolean filtering; /* True => exclude filtered roles; False => filter reset */
int race, gend, algn; /* all ROLE_NONE for !filtering case */
{
    anything any;
    int i;
    boolean role_ok;
    char thisch, lastch = '\0', rolenamebuf[50];

    any = zeroany; /* zero out all bits */
    for (i = 0; roles[i].name.m; i++) {
        role_ok = ok_role(i, race, gend, algn);
        if (filtering && !role_ok)
            continue;
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = roles[i].name.m;
        thisch = lowc(*roles[i].name.m);
        if (thisch == lastch)
            thisch = highc(thisch);
        Strcpy(rolenamebuf, roles[i].name.m);
        if (roles[i].name.f) {
            /* role has distinct name for female (C,P) */
            if (gend == 1) {
                /* female already chosen; replace male name */
                Strcpy(rolenamebuf, roles[i].name.f);
            } else if (gend < 0) {
                /* not chosen yet; append slash+female name */
                Strcat(rolenamebuf, "/");
                Strcat(rolenamebuf, roles[i].name.f);
            }
        }
        /* !filtering implies reset_role_filtering() where we want to
           mark this role as preseleted if current filter excludes it */
        add_menu(win, NO_GLYPH, &any, thisch, 0, ATR_NONE, an(rolenamebuf),
                 (!filtering && !role_ok) ? MENU_SELECTED : MENU_UNSELECTED);
        lastch = thisch;
    }
}

STATIC_OVL void
setup_racemenu(win, filtering, role, gend, algn)
winid win;
boolean filtering;
int role, gend, algn;
{
    anything any;
    boolean race_ok;
    int i;
    char this_ch;

    any = zeroany;
    for (i = 0; races[i].noun; i++) {
        race_ok = ok_race(role, i, gend, algn);
        if (filtering && !race_ok)
            continue;
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = races[i].noun;
        this_ch = *races[i].noun;
        /* filtering: picking race, so choose by first letter, with
           capital letter as unseen accelerator;
           !filtering: resetting filter rather than picking, choose by
           capital letter since lowercase role letters will be present */
        add_menu(win, NO_GLYPH, &any,
                 filtering ? this_ch : highc(this_ch),
                 filtering ? highc(this_ch) : 0,
                 ATR_NONE, races[i].noun,
                 (!filtering && !race_ok) ? MENU_SELECTED : MENU_UNSELECTED);
    }
}

STATIC_DCL void
setup_gendmenu(win, filtering, role, race, algn)
winid win;
boolean filtering;
int role, race, algn;
{
    anything any;
    boolean gend_ok;
    int i;
    char this_ch;

    any = zeroany;
    for (i = 0; i < ROLE_GENDERS; i++) {
        gend_ok = ok_gend(role, race, i, algn);
        if (filtering && !gend_ok)
            continue;
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = genders[i].adj;
        this_ch = *genders[i].adj;
        /* (see setup_racemenu for explanation of selector letters
           and setup_rolemenu for preselection) */
        add_menu(win, NO_GLYPH, &any,
                 filtering ? this_ch : highc(this_ch),
                 filtering ? highc(this_ch) : 0,
                 ATR_NONE, genders[i].adj,
                 (!filtering && !gend_ok) ? MENU_SELECTED : MENU_UNSELECTED);
    }
}

STATIC_DCL void
setup_algnmenu(win, filtering, role, race, gend)
winid win;
boolean filtering;
int role, race, gend;
{
    anything any;
    boolean algn_ok;
    int i;
    char this_ch;

    any = zeroany;
    for (i = 0; i < ROLE_ALIGNS; i++) {
        algn_ok = ok_align(role, race, gend, i);
        if (filtering && !algn_ok)
            continue;
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = aligns[i].adj;
        this_ch = *aligns[i].adj;
        /* (see setup_racemenu for explanation of selector letters
           and setup_rolemenu for preselection) */
        add_menu(win, NO_GLYPH, &any,
                 filtering ? this_ch : highc(this_ch),
                 filtering ? highc(this_ch) : 0,
                 ATR_NONE, aligns[i].adj,
                 (!filtering && !algn_ok) ? MENU_SELECTED : MENU_UNSELECTED);
    }
}

/*
 * plname is filled either by an option (-u Player  or  -uPlayer) or
 * explicitly (by being the wizard) or by askname.
 * It may still contain a suffix denoting the role, etc.
 * Always called after init_nhwindows() and before display_gamewindows().
 */
void
tty_askname()
{
    static const char who_are_you[] = "Who are you? ";
    register int c, ct, tryct = 0;

#ifdef SELECTSAVED
    if (iflags.wc2_selectsaved && !iflags.renameinprogress)
        switch (restore_menu(BASE_WINDOW)) {
        case -1:
            bail("Until next time then..."); /* quit */
            /*NOTREACHED*/
        case 0:
            break; /* no game chosen; start new game */
        case 1:
            return; /* plname[] has been set */
        }
#endif /* SELECTSAVED */

    tty_putstr(BASE_WINDOW, 0, "");
    do {
        if (++tryct > 1) {
            if (tryct > 10)
                bail("Giving up after 10 tries.\n");
            tty_curs(BASE_WINDOW, 1, wins[BASE_WINDOW]->cury - 1);
            tty_putstr(BASE_WINDOW, 0, "Enter a name for your character...");
            /* erase previous prompt (in case of ESC after partial response)
             */
            tty_curs(BASE_WINDOW, 1, wins[BASE_WINDOW]->cury), cl_end();
        }
        tty_putstr(BASE_WINDOW, 0, who_are_you);
        tty_curs(BASE_WINDOW, (int) (sizeof who_are_you),
                 wins[BASE_WINDOW]->cury - 1);
        ct = 0;
        while ((c = tty_nhgetch()) != '\n') {
            if (c == EOF)
                c = '\033';
            if (c == '\r')
                break;
            if (c == '\033') {
                ct = 0;
                break;
            } /* continue outer loop */
#if defined(WIN32CON)
            if (c == '\003')
                bail("^C abort.\n");
#endif
            /* some people get confused when their erase char is not ^H */
            if (c == '\b' || c == '\177') {
                if (ct) {
                    ct--;
#ifdef WIN32CON
                    ttyDisplay->curx--;
#endif
#if defined(MICRO) || defined(WIN32CON)
#if defined(WIN32CON) || defined(MSDOS)
                    backsp(); /* \b is visible on NT */
                    (void) putchar(' ');
                    backsp();
#else
                    msmsg("\b \b");
#endif
#else
                    (void) putchar('\b');
                    (void) putchar(' ');
                    (void) putchar('\b');
#endif
                }
                continue;
            }
#if defined(UNIX) || defined(VMS)
            if (c != '-' && c != '@')
                if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
                    /* reject leading digit but allow digits elsewhere
                       (avoids ambiguity when character name gets
                       appended to uid to construct save file name) */
                    !(c >= '0' && c <= '9' && ct > 0))
                    c = '_';
#endif
            if (ct < (int) (sizeof plname) - 1) {
#if defined(MICRO)
#if defined(MSDOS)
                if (iflags.grmode) {
                    (void) putchar(c);
                } else
#endif
                    msmsg("%c", c);
#else
                (void) putchar(c);
#endif
                plname[ct++] = c;
#ifdef WIN32CON
                ttyDisplay->curx++;
#endif
            }
        }
        plname[ct] = 0;
    } while (ct == 0);

    /* move to next line to simulate echo of user's <return> */
    tty_curs(BASE_WINDOW, 1, wins[BASE_WINDOW]->cury + 1);

    /* since we let user pick an arbitrary name now, he/she can pick
       another one during role selection */
    iflags.renameallowed = TRUE;
}

void
tty_get_nh_event()
{
    return;
}

#if !defined(MICRO) && !defined(WIN32CON)
STATIC_OVL void
getret()
{
    xputs("\n");
    if (flags.standout)
        standoutbeg();
    xputs("Hit ");
    xputs(iflags.cbreak ? "space" : "return");
    xputs(" to continue: ");
    if (flags.standout)
        standoutend();
    xwaitforspace(" ");
}
#endif

void
tty_suspend_nhwindows(str)
const char *str;
{
    settty(str); /* calls end_screen, perhaps raw_print */
    if (!str)
        tty_raw_print(""); /* calls fflush(stdout) */
}

void
tty_resume_nhwindows()
{
    gettty();
    setftty(); /* calls start_screen */
    docrt();
}

void
tty_exit_nhwindows(str)
const char *str;
{
    winid i;

    tty_suspend_nhwindows(str);
    /*
     * Disable windows to avoid calls to window routines.
     */
    free_pickinv_cache(); /* reset its state as well as tear down window */
    for (i = 0; i < MAXWIN; i++) {
        if (i == BASE_WINDOW)
            continue; /* handle wins[BASE_WINDOW] last */
        if (wins[i]) {
#ifdef FREE_ALL_MEMORY
            free_window_info(wins[i], TRUE);
            free((genericptr_t) wins[i]);
#endif
            wins[i] = (struct WinDesc *) 0;
        }
    }
    WIN_MAP = WIN_MESSAGE = WIN_INVEN = WIN_ERR; /* these are all gone now */
    WIN_STATUS = WIN_ERR;
#ifdef FREE_ALL_MEMORY
    if (BASE_WINDOW != WIN_ERR && wins[BASE_WINDOW]) {
        free_window_info(wins[BASE_WINDOW], TRUE);
        free((genericptr_t) wins[BASE_WINDOW]);
        wins[BASE_WINDOW] = (struct WinDesc *) 0;
        BASE_WINDOW = WIN_ERR;
    }
    free((genericptr_t) ttyDisplay);
    ttyDisplay = (struct DisplayDesc *) 0;
#endif

#ifndef NO_TERMS    /*(until this gets added to the window interface)*/
    tty_shutdown(); /* cleanup termcap/terminfo/whatever */
#endif
    iflags.window_inited = 0;
}

winid
tty_create_nhwindow(type)
int type;
{
    struct WinDesc *newwin;
    int i;
    int newid;

    if (maxwin == MAXWIN)
        return WIN_ERR;

    newwin = (struct WinDesc *) alloc(sizeof(struct WinDesc));
    newwin->type = type;
    newwin->flags = 0;
    newwin->active = FALSE;
    newwin->curx = newwin->cury = 0;
    newwin->morestr = 0;
    newwin->mlist = (tty_menu_item *) 0;
    newwin->plist = (tty_menu_item **) 0;
    newwin->npages = newwin->plist_size = newwin->nitems = newwin->how = 0;
    switch (type) {
    case NHW_BASE:
        /* base window, used for absolute movement on the screen */
        newwin->offx = newwin->offy = 0;
        newwin->rows = ttyDisplay->rows;
        newwin->cols = ttyDisplay->cols;
        newwin->maxrow = newwin->maxcol = 0;
        break;
    case NHW_MESSAGE:
        /* message window, 1 line long, very wide, top of screen */
        newwin->offx = newwin->offy = 0;
        /* sanity check */
        if (iflags.msg_history < 20)
            iflags.msg_history = 20;
        else if (iflags.msg_history > 60)
            iflags.msg_history = 60;
        newwin->maxrow = newwin->rows = iflags.msg_history;
        newwin->maxcol = newwin->cols = 0;
        break;
    case NHW_STATUS:
        /* status window, 2 lines long, full width, bottom of screen */
        newwin->offx = 0;
#if defined(USE_TILES) && defined(MSDOS)
        if (iflags.grmode) {
            newwin->offy = ttyDisplay->rows - 2;
        } else
#endif
            newwin->offy = min((int) ttyDisplay->rows - 2, ROWNO + 1);
        newwin->rows = newwin->maxrow = 2;
        newwin->cols = newwin->maxcol = ttyDisplay->cols;
        break;
    case NHW_MAP:
        /* map window, ROWNO lines long, full width, below message window */
        newwin->offx = 0;
        newwin->offy = 1;
        newwin->rows = ROWNO;
        newwin->cols = COLNO;
        newwin->maxrow = 0; /* no buffering done -- let gbuf do it */
        newwin->maxcol = 0;
        break;
    case NHW_MENU:
    case NHW_TEXT:
        /* inventory/menu window, variable length, full width, top of screen
         */
        /* help window, the same, different semantics for display, etc */
        newwin->offx = newwin->offy = 0;
        newwin->rows = 0;
        newwin->cols = ttyDisplay->cols;
        newwin->maxrow = newwin->maxcol = 0;
        break;
    default:
        panic("Tried to create window type %d\n", (int) type);
        return WIN_ERR;
    }

    for (newid = 0; newid < MAXWIN; newid++) {
        if (wins[newid] == 0) {
            wins[newid] = newwin;
            break;
        }
    }
    if (newid == MAXWIN) {
        panic("No window slots!");
        return WIN_ERR;
    }

    if (newwin->maxrow) {
        newwin->data =
            (char **) alloc(sizeof(char *) * (unsigned) newwin->maxrow);
        newwin->datlen =
            (short *) alloc(sizeof(short) * (unsigned) newwin->maxrow);
        if (newwin->maxcol) {
            /* WIN_STATUS */
            for (i = 0; i < newwin->maxrow; i++) {
                newwin->data[i] = (char *) alloc((unsigned) newwin->maxcol);
                newwin->datlen[i] = (short) newwin->maxcol;
            }
        } else {
            for (i = 0; i < newwin->maxrow; i++) {
                newwin->data[i] = (char *) 0;
                newwin->datlen[i] = 0;
            }
        }
        if (newwin->type == NHW_MESSAGE)
            newwin->maxrow = 0;
    } else {
        newwin->data = (char **) 0;
        newwin->datlen = (short *) 0;
    }

    return newid;
}

STATIC_OVL void
erase_menu_or_text(window, cw, clear)
winid window;
struct WinDesc *cw;
boolean clear;
{
    if (cw->offx == 0)
        if (cw->offy) {
            tty_curs(window, 1, 0);
            cl_eos();
        } else if (clear)
            clear_screen();
        else
            docrt();
    else
        docorner((int) cw->offx, cw->maxrow + 1);
}

STATIC_OVL void
free_window_info(cw, free_data)
struct WinDesc *cw;
boolean free_data;
{
    int i;

    if (cw->data) {
        if (cw == wins[WIN_MESSAGE] && cw->rows > cw->maxrow)
            cw->maxrow = cw->rows; /* topl data */
        for (i = 0; i < cw->maxrow; i++)
            if (cw->data[i]) {
                free((genericptr_t) cw->data[i]);
                cw->data[i] = (char *) 0;
                if (cw->datlen)
                    cw->datlen[i] = 0;
            }
        if (free_data) {
            free((genericptr_t) cw->data);
            cw->data = (char **) 0;
            if (cw->datlen)
                free((genericptr_t) cw->datlen);
            cw->datlen = (short *) 0;
            cw->rows = 0;
        }
    }
    cw->maxrow = cw->maxcol = 0;
    if (cw->mlist) {
        tty_menu_item *temp;

        while ((temp = cw->mlist) != 0) {
            cw->mlist = cw->mlist->next;
            if (temp->str)
                free((genericptr_t) temp->str);
            free((genericptr_t) temp);
        }
    }
    if (cw->plist) {
        free((genericptr_t) cw->plist);
        cw->plist = 0;
    }
    cw->plist_size = cw->npages = cw->nitems = cw->how = 0;
    if (cw->morestr) {
        free((genericptr_t) cw->morestr);
        cw->morestr = 0;
    }
}

void
tty_clear_nhwindow(window)
winid window;
{
    register struct WinDesc *cw = 0;

    if (window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);
    ttyDisplay->lastwin = window;

    print_vt_code2(AVTC_SELECT_WINDOW, window);

    switch (cw->type) {
    case NHW_MESSAGE:
        if (ttyDisplay->toplin) {
            home();
            cl_end();
            if (cw->cury)
                docorner(1, cw->cury + 1);
            ttyDisplay->toplin = 0;
        }
        break;
    case NHW_STATUS:
        tty_curs(window, 1, 0);
        cl_end();
        tty_curs(window, 1, 1);
        cl_end();
        break;
    case NHW_MAP:
        /* cheap -- clear the whole thing and tell nethack to redraw botl */
        context.botlx = 1;
        /*FALLTHRU*/
    case NHW_BASE:
        clear_screen();
        break;
    case NHW_MENU:
    case NHW_TEXT:
        if (cw->active)
            erase_menu_or_text(window, cw, TRUE);
        free_window_info(cw, FALSE);
        break;
    }
    cw->curx = cw->cury = 0;
}

boolean
toggle_menu_curr(window, curr, lineno, in_view, counting, count)
winid window;
tty_menu_item *curr;
int lineno;
boolean in_view, counting;
long count;
{
    if (curr->selected) {
        if (counting && count > 0) {
            curr->count = count;
            if (in_view)
                set_item_state(window, lineno, curr);
            return TRUE;
        } else { /* change state */
            curr->selected = FALSE;
            curr->count = -1L;
            if (in_view)
                set_item_state(window, lineno, curr);
            return TRUE;
        }
    } else { /* !selected */
        if (counting && count > 0) {
            curr->count = count;
            curr->selected = TRUE;
            if (in_view)
                set_item_state(window, lineno, curr);
            return TRUE;
        } else if (!counting) {
            curr->selected = TRUE;
            if (in_view)
                set_item_state(window, lineno, curr);
            return TRUE;
        }
        /* do nothing counting&&count==0 */
    }
    return FALSE;
}

STATIC_OVL void
dmore(cw, s)
register struct WinDesc *cw;
const char *s; /* valid responses */
{
    const char *prompt = cw->morestr ? cw->morestr : defmorestr;
    int offset = (cw->type == NHW_TEXT) ? 1 : 2;

    tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + offset,
             (int) ttyDisplay->cury);
    if (flags.standout)
        standoutbeg();
    xputs(prompt);
    ttyDisplay->curx += strlen(prompt);
    if (flags.standout)
        standoutend();

    xwaitforspace(s);
}

STATIC_OVL void
set_item_state(window, lineno, item)
winid window;
int lineno;
tty_menu_item *item;
{
    char ch = item->selected ? (item->count == -1L ? '+' : '#') : '-';

    tty_curs(window, 4, lineno);
    term_start_attr(item->attr);
    (void) putchar(ch);
    ttyDisplay->curx++;
    term_end_attr(item->attr);
}

STATIC_OVL void
set_all_on_page(window, page_start, page_end)
winid window;
tty_menu_item *page_start, *page_end;
{
    tty_menu_item *curr;
    int n;

    for (n = 0, curr = page_start; curr != page_end; n++, curr = curr->next)
        if (curr->identifier.a_void && !curr->selected) {
            curr->selected = TRUE;
            set_item_state(window, n, curr);
        }
}

STATIC_OVL void
unset_all_on_page(window, page_start, page_end)
winid window;
tty_menu_item *page_start, *page_end;
{
    tty_menu_item *curr;
    int n;

    for (n = 0, curr = page_start; curr != page_end; n++, curr = curr->next)
        if (curr->identifier.a_void && curr->selected) {
            curr->selected = FALSE;
            curr->count = -1L;
            set_item_state(window, n, curr);
        }
}

STATIC_OVL void
invert_all_on_page(window, page_start, page_end, acc)
winid window;
tty_menu_item *page_start, *page_end;
char acc; /* group accelerator, 0 => all */
{
    tty_menu_item *curr;
    int n;

    for (n = 0, curr = page_start; curr != page_end; n++, curr = curr->next)
        if (curr->identifier.a_void && (acc == 0 || curr->gselector == acc)) {
            if (curr->selected) {
                curr->selected = FALSE;
                curr->count = -1L;
            } else
                curr->selected = TRUE;
            set_item_state(window, n, curr);
        }
}

/*
 * Invert all entries that match the give group accelerator (or all if zero).
 */
STATIC_OVL void
invert_all(window, page_start, page_end, acc)
winid window;
tty_menu_item *page_start, *page_end;
char acc; /* group accelerator, 0 => all */
{
    tty_menu_item *curr;
    boolean on_curr_page;
    struct WinDesc *cw = wins[window];

    invert_all_on_page(window, page_start, page_end, acc);

    /* invert the rest */
    for (on_curr_page = FALSE, curr = cw->mlist; curr; curr = curr->next) {
        if (curr == page_start)
            on_curr_page = TRUE;
        else if (curr == page_end)
            on_curr_page = FALSE;

        if (!on_curr_page && curr->identifier.a_void
            && (acc == 0 || curr->gselector == acc)) {
            if (curr->selected) {
                curr->selected = FALSE;
                curr->count = -1;
            } else
                curr->selected = TRUE;
        }
    }
}

/* support menucolor in addition to caller-supplied attribute */
STATIC_OVL void
toggle_menu_attr(on, color, attr)
boolean on;
int color, attr;
{
    if (on) {
        term_start_attr(attr);
#ifdef TEXTCOLOR
        if (color != NO_COLOR)
            term_start_color(color);
#endif
    } else {
#ifdef TEXTCOLOR
        if (color != NO_COLOR)
            term_end_color();
#endif
        term_end_attr(attr);
    }

#ifndef TEXTCOLOR
    nhUse(color);
#endif
}

STATIC_OVL void
process_menu_window(window, cw)
winid window;
struct WinDesc *cw;
{
    tty_menu_item *page_start, *page_end, *curr;
    long count;
    int n, attr_n, curr_page, page_lines, resp_len;
    boolean finished, counting, reset_count;
    char *cp, *rp, resp[QBUFSZ], gacc[QBUFSZ], *msave, *morestr, really_morc;
#define MENU_EXPLICIT_CHOICE 0x7f /* pseudo menu manipulation char */

    curr_page = page_lines = 0;
    page_start = page_end = 0;
    msave = cw->morestr; /* save the morestr */
    cw->morestr = morestr = (char *) alloc((unsigned) QBUFSZ);
    counting = FALSE;
    count = 0L;
    reset_count = TRUE;
    finished = FALSE;

    /* collect group accelerators; for PICK_NONE, they're ignored;
       for PICK_ONE, only those which match exactly one entry will be
       accepted; for PICK_ANY, those which match any entry are okay */
    gacc[0] = '\0';
    if (cw->how != PICK_NONE) {
        int i, gcnt[128];
#define GSELIDX(c) (c & 127) /* guard against `signed char' */

        for (i = 0; i < SIZE(gcnt); i++)
            gcnt[i] = 0;
        for (n = 0, curr = cw->mlist; curr; curr = curr->next)
            if (curr->gselector && curr->gselector != curr->selector) {
                ++n;
                ++gcnt[GSELIDX(curr->gselector)];
            }

        if (n > 0) /* at least one group accelerator found */
            for (rp = gacc, curr = cw->mlist; curr; curr = curr->next)
                if (curr->gselector && curr->gselector != curr->selector
                    && !index(gacc, curr->gselector)
                    && (cw->how == PICK_ANY
                        || gcnt[GSELIDX(curr->gselector)] == 1)) {
                    *rp++ = curr->gselector;
                    *rp = '\0'; /* re-terminate for index() */
                }
    }
    resp_len = 0; /* lint suppression */

    /* loop until finished */
    while (!finished) {
        if (reset_count) {
            counting = FALSE;
            count = 0;
        } else
            reset_count = TRUE;

        if (!page_start) {
            /* new page to be displayed */
            if (curr_page < 0 || (cw->npages > 0 && curr_page >= cw->npages))
                panic("bad menu screen page #%d", curr_page);

            /* clear screen */
            if (!cw->offx) { /* if not corner, do clearscreen */
                if (cw->offy) {
                    tty_curs(window, 1, 0);
                    cl_eos();
                } else
                    clear_screen();
            }

            rp = resp;
            if (cw->npages > 0) {
                /* collect accelerators */
                page_start = cw->plist[curr_page];
                page_end = cw->plist[curr_page + 1];
                for (page_lines = 0, curr = page_start; curr != page_end;
                     page_lines++, curr = curr->next) {
                    int attr, color = NO_COLOR;

                    if (curr->selector)
                        *rp++ = curr->selector;

                    tty_curs(window, 1, page_lines);
                    if (cw->offx)
                        cl_end();

                    (void) putchar(' ');
                    ++ttyDisplay->curx;

                    if (!iflags.use_menu_color
                        || !get_menu_coloring(curr->str, &color, &attr))
                        attr = curr->attr;

                    /* which character to start attribute highlighting;
                       whole line for headers and such, after the selector
                       character and space and selection indicator for menu
                       lines (including fake ones that simulate grayed-out
                       entries, so we don't rely on curr->identifier here) */
                    attr_n = 0; /* whole line */
                    if (curr->str[0] && curr->str[1] == ' '
                        && curr->str[2] && index("-+#", curr->str[2])
                        && curr->str[3] == ' ')
                        /* [0]=letter, [1]==space, [2]=[-+#], [3]=space */
                        attr_n = 4; /* [4:N]=entry description */

                    /*
                     * Don't use xputs() because (1) under unix it calls
                     * tputstr() which will interpret a '*' as some kind
                     * of padding information and (2) it calls xputc to
                     * actually output the character.  We're faster doing
                     * this.
                     */
                    for (n = 0, cp = curr->str;
                         *cp &&
#ifndef WIN32CON
                            (int) ++ttyDisplay->curx < (int) ttyDisplay->cols;
#else
                            (int) ttyDisplay->curx < (int) ttyDisplay->cols;
                         ttyDisplay->curx++,
#endif
                         cp++, n++) {
                        if (n == attr_n && (color != NO_COLOR
                                            || attr != ATR_NONE))
                            toggle_menu_attr(TRUE, color, attr);
                        if (n == 2
                            && curr->identifier.a_void != 0
                            && curr->selected) {
                            if (curr->count == -1L)
                                (void) putchar('+'); /* all selected */
                            else
                                (void) putchar('#'); /* count selected */
                        } else
                            (void) putchar(*cp);
                    } /* for *cp */
                    if (n > attr_n && (color != NO_COLOR || attr != ATR_NONE))
                        toggle_menu_attr(FALSE, color, attr);
                } /* if npages > 0 */
            } else {
                page_start = 0;
                page_end = 0;
                page_lines = 0;
            }
            *rp = 0;
            /* remember how many explicit menu choices there are */
            resp_len = (int) strlen(resp);

            /* corner window - clear extra lines from last page */
            if (cw->offx) {
                for (n = page_lines + 1; n < cw->maxrow; n++) {
                    tty_curs(window, 1, n);
                    cl_end();
                }
            }

            /* set extra chars.. */
            Strcat(resp, default_menu_cmds);
            Strcat(resp, " ");                  /* next page or end */
            Strcat(resp, "0123456789\033\n\r"); /* counts, quit */
            Strcat(resp, gacc);                 /* group accelerators */
            Strcat(resp, mapped_menu_cmds);

            if (cw->npages > 1)
                Sprintf(cw->morestr, "(%d of %d)", curr_page + 1,
                        (int) cw->npages);
            else if (msave)
                Strcpy(cw->morestr, msave);
            else
                Strcpy(cw->morestr, defmorestr);

            tty_curs(window, 1, page_lines);
            cl_end();
            dmore(cw, resp);
        } else {
            /* just put the cursor back... */
            tty_curs(window, (int) strlen(cw->morestr) + 2, page_lines);
            xwaitforspace(resp);
        }

        really_morc = morc; /* (only used with MENU_EXPLICIT_CHOICE */
        if ((rp = index(resp, morc)) != 0 && rp < resp + resp_len)
            /* explicit menu selection; don't override it if it also
               happens to match a mapped menu command (such as ':' to
               look inside a container vs ':' to search) */
            morc = MENU_EXPLICIT_CHOICE;
        else
            morc = map_menu_cmd(morc);

        switch (morc) {
        case '0':
            /* special case: '0' is also the default ball class */
            if (!counting && index(gacc, morc))
                goto group_accel;
            /* fall through to count the zero */
            /*FALLTHRU*/
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            count = (count * 10L) + (long) (morc - '0');
            /*
             * It is debatable whether we should allow 0 to
             * start a count.  There is no difference if the
             * item is selected.  If not selected, then
             * "0b" could mean:
             *
             *  count starting zero:    "zero b's"
             *  ignore starting zero:   "select b"
             *
             * At present I don't know which is better.
             */
            if (count != 0L) { /* ignore leading zeros */
                counting = TRUE;
                reset_count = FALSE;
            }
            break;
        case '\033': /* cancel - from counting or loop */
            if (!counting) {
                /* deselect everything */
                for (curr = cw->mlist; curr; curr = curr->next) {
                    curr->selected = FALSE;
                    curr->count = -1L;
                }
                cw->flags |= WIN_CANCELLED;
                finished = TRUE;
            }
            /* else only stop count */
            break;
        case '\0': /* finished (commit) */
        case '\n':
        case '\r':
            /* only finished if we are actually picking something */
            if (cw->how != PICK_NONE) {
                finished = TRUE;
                break;
            }
        /* else fall through */
        case ' ':
        case MENU_NEXT_PAGE:
            if (cw->npages > 0 && curr_page != cw->npages - 1) {
                curr_page++;
                page_start = 0;
            } else if (morc == ' ') {
                /* ' ' finishes menus here, but stop '>' doing the same. */
                finished = TRUE;
            }
            break;
        case MENU_PREVIOUS_PAGE:
            if (cw->npages > 0 && curr_page != 0) {
                --curr_page;
                page_start = 0;
            }
            break;
        case MENU_FIRST_PAGE:
            if (cw->npages > 0 && curr_page != 0) {
                page_start = 0;
                curr_page = 0;
            }
            break;
        case MENU_LAST_PAGE:
            if (cw->npages > 0 && curr_page != cw->npages - 1) {
                page_start = 0;
                curr_page = cw->npages - 1;
            }
            break;
        case MENU_SELECT_PAGE:
            if (cw->how == PICK_ANY)
                set_all_on_page(window, page_start, page_end);
            break;
        case MENU_UNSELECT_PAGE:
            unset_all_on_page(window, page_start, page_end);
            break;
        case MENU_INVERT_PAGE:
            if (cw->how == PICK_ANY)
                invert_all_on_page(window, page_start, page_end, 0);
            break;
        case MENU_SELECT_ALL:
            if (cw->how == PICK_ANY) {
                set_all_on_page(window, page_start, page_end);
                /* set the rest */
                for (curr = cw->mlist; curr; curr = curr->next)
                    if (curr->identifier.a_void && !curr->selected)
                        curr->selected = TRUE;
            }
            break;
        case MENU_UNSELECT_ALL:
            unset_all_on_page(window, page_start, page_end);
            /* unset the rest */
            for (curr = cw->mlist; curr; curr = curr->next)
                if (curr->identifier.a_void && curr->selected) {
                    curr->selected = FALSE;
                    curr->count = -1;
                }
            break;
        case MENU_INVERT_ALL:
            if (cw->how == PICK_ANY)
                invert_all(window, page_start, page_end, 0);
            break;
        case MENU_SEARCH:
            if (cw->how == PICK_NONE) {
                tty_nhbell();
                break;
            } else {
                char searchbuf[BUFSZ + 2], tmpbuf[BUFSZ];
                boolean on_curr_page = FALSE;
                int lineno = 0;

                tty_getlin("Search for:", tmpbuf);
                if (!tmpbuf[0] || tmpbuf[0] == '\033')
                    break;
                Sprintf(searchbuf, "*%s*", tmpbuf);

                for (curr = cw->mlist; curr; curr = curr->next) {
                    if (on_curr_page)
                        lineno++;
                    if (curr == page_start)
                        on_curr_page = TRUE;
                    else if (curr == page_end)
                        on_curr_page = FALSE;
                    if (curr->identifier.a_void
                        && pmatchi(searchbuf, curr->str)) {
                        toggle_menu_curr(window, curr, lineno, on_curr_page,
                                         counting, count);
                        if (cw->how == PICK_ONE) {
                            finished = TRUE;
                            break;
                        }
                    }
                }
            }
            break;
        case MENU_EXPLICIT_CHOICE:
            morc = really_morc;
        /*FALLTHRU*/
        default:
            if (cw->how == PICK_NONE || !index(resp, morc)) {
                /* unacceptable input received */
                tty_nhbell();
                break;
            } else if (index(gacc, morc)) {
            group_accel:
                /* group accelerator; for the PICK_ONE case, we know that
                   it matches exactly one item in order to be in gacc[] */
                invert_all(window, page_start, page_end, morc);
                if (cw->how == PICK_ONE)
                    finished = TRUE;
                break;
            }
            /* find, toggle, and possibly update */
            for (n = 0, curr = page_start; curr != page_end;
                 n++, curr = curr->next)
                if (morc == curr->selector) {
                    toggle_menu_curr(window, curr, n, TRUE, counting, count);
                    if (cw->how == PICK_ONE)
                        finished = TRUE;
                    break; /* from `for' loop */
                }
            break;
        }

    } /* while */
    cw->morestr = msave;
    free((genericptr_t) morestr);
}

STATIC_OVL void
process_text_window(window, cw)
winid window;
struct WinDesc *cw;
{
    int i, n, attr;
    register char *cp;

    for (n = 0, i = 0; i < cw->maxrow; i++) {
        if (!cw->offx && (n + cw->offy == ttyDisplay->rows - 1)) {
            tty_curs(window, 1, n);
            cl_end();
            dmore(cw, quitchars);
            if (morc == '\033') {
                cw->flags |= WIN_CANCELLED;
                break;
            }
            if (cw->offy) {
                tty_curs(window, 1, 0);
                cl_eos();
            } else
                clear_screen();
            n = 0;
        }
        tty_curs(window, 1, n++);
#ifdef H2344_BROKEN
        cl_end();
#else
        if (cw->offx)
            cl_end();
#endif
        if (cw->data[i]) {
            attr = cw->data[i][0] - 1;
            if (cw->offx) {
                (void) putchar(' ');
                ++ttyDisplay->curx;
            }
            term_start_attr(attr);
            for (cp = &cw->data[i][1];
#ifndef WIN32CON
                 *cp && (int) ++ttyDisplay->curx < (int) ttyDisplay->cols;
                 cp++)
#else
                 *cp && (int) ttyDisplay->curx < (int) ttyDisplay->cols;
                 cp++, ttyDisplay->curx++)
#endif
                (void) putchar(*cp);
            term_end_attr(attr);
        }
    }
    if (i == cw->maxrow) {
#ifdef H2344_BROKEN
        if (cw->type == NHW_TEXT) {
            tty_curs(BASE_WINDOW, 1, (int) ttyDisplay->cury + 1);
            cl_eos();
        }
#endif
        tty_curs(BASE_WINDOW, (int) cw->offx + 1,
                 (cw->type == NHW_TEXT) ? (int) ttyDisplay->rows - 1 : n);
        cl_end();
        dmore(cw, quitchars);
        if (morc == '\033')
            cw->flags |= WIN_CANCELLED;
    }
}

/*ARGSUSED*/
void
tty_display_nhwindow(window, blocking)
winid window;
boolean blocking; /* with ttys, all windows are blocking */
{
    register struct WinDesc *cw = 0;
    short s_maxcol;

    if (window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);
    if (cw->flags & WIN_CANCELLED)
        return;
    ttyDisplay->lastwin = window;
    ttyDisplay->rawprint = 0;

    print_vt_code2(AVTC_SELECT_WINDOW, window);

    switch (cw->type) {
    case NHW_MESSAGE:
        if (ttyDisplay->toplin == 1) {
            more();
            ttyDisplay->toplin = 1; /* more resets this */
            tty_clear_nhwindow(window);
        } else
            ttyDisplay->toplin = 0;
        cw->curx = cw->cury = 0;
        if (!cw->active)
            iflags.window_inited = TRUE;
        break;
    case NHW_MAP:
        end_glyphout();
        if (blocking) {
            if (!ttyDisplay->toplin)
                ttyDisplay->toplin = 1;
            tty_display_nhwindow(WIN_MESSAGE, TRUE);
            return;
        }
        /*FALLTHRU*/
    case NHW_BASE:
        (void) fflush(stdout);
        break;
    case NHW_TEXT:
        cw->maxcol = ttyDisplay->cols; /* force full-screen mode */
        /*FALLTHRU*/
    case NHW_MENU:
        cw->active = 1;
        /* cw->maxcol is a long, but its value is constrained to
           be <= ttyDisplay->cols, so is sure to fit within a short */
        s_maxcol = (short) cw->maxcol;
#ifdef H2344_BROKEN
        cw->offx = (cw->type == NHW_TEXT)
                       ? 0
                       : min(min(82, ttyDisplay->cols / 2),
                             ttyDisplay->cols - s_maxcol - 1);
#else
        /* avoid converting to uchar before calculations are finished */
        cw->offx = (uchar) max((int) 10,
                               (int) (ttyDisplay->cols - s_maxcol - 1));
#endif
        if (cw->offx < 0)
            cw->offx = 0;
        if (cw->type == NHW_MENU)
            cw->offy = 0;
        if (ttyDisplay->toplin == 1)
            tty_display_nhwindow(WIN_MESSAGE, TRUE);
#ifdef H2344_BROKEN
        if (cw->maxrow >= (int) ttyDisplay->rows
            || !iflags.menu_overlay)
#else
        if (cw->offx == 10 || cw->maxrow >= (int) ttyDisplay->rows
            || !iflags.menu_overlay)
#endif
        {
            cw->offx = 0;
            if (cw->offy || iflags.menu_overlay) {
                tty_curs(window, 1, 0);
                cl_eos();
            } else
                clear_screen();
            ttyDisplay->toplin = 0;
        } else {
            if (WIN_MESSAGE != WIN_ERR)
                tty_clear_nhwindow(WIN_MESSAGE);
        }

        if (cw->data || !cw->maxrow)
            process_text_window(window, cw);
        else
            process_menu_window(window, cw);
        break;
    }
    cw->active = 1;
}

void
tty_dismiss_nhwindow(window)
winid window;
{
    register struct WinDesc *cw = 0;

    if (window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);

    print_vt_code2(AVTC_SELECT_WINDOW, window);

    switch (cw->type) {
    case NHW_MESSAGE:
        if (ttyDisplay->toplin)
            tty_display_nhwindow(WIN_MESSAGE, TRUE);
    /*FALLTHRU*/
    case NHW_STATUS:
    case NHW_BASE:
    case NHW_MAP:
        /*
         * these should only get dismissed when the game is going away
         * or suspending
         */
        tty_curs(BASE_WINDOW, 1, (int) ttyDisplay->rows - 1);
        cw->active = 0;
        break;
    case NHW_MENU:
    case NHW_TEXT:
        if (cw->active) {
            if (iflags.window_inited) {
                /* otherwise dismissing the text endwin after other windows
                 * are dismissed tries to redraw the map and panics.  since
                 * the whole reason for dismissing the other windows was to
                 * leave the ending window on the screen, we don't want to
                 * erase it anyway.
                 */
                erase_menu_or_text(window, cw, FALSE);
            }
            cw->active = 0;
        }
        break;
    }
    cw->flags = 0;
}

void
tty_destroy_nhwindow(window)
winid window;
{
    register struct WinDesc *cw = 0;

    if (window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);

    if (cw->active)
        tty_dismiss_nhwindow(window);
    if (cw->type == NHW_MESSAGE)
        iflags.window_inited = 0;
    if (cw->type == NHW_MAP)
        clear_screen();

    free_window_info(cw, TRUE);
    free((genericptr_t) cw);
    wins[window] = 0;
}

void
tty_curs(window, x, y)
winid window;
register int x, y; /* not xchar: perhaps xchar is unsigned and
                      curx-x would be unsigned as well */
{
    struct WinDesc *cw = 0;
    int cx = ttyDisplay->curx;
    int cy = ttyDisplay->cury;

    if (window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);
    ttyDisplay->lastwin = window;

    print_vt_code2(AVTC_SELECT_WINDOW, window);

#if defined(USE_TILES) && defined(MSDOS)
    adjust_cursor_flags(cw);
#endif
    cw->curx = --x; /* column 0 is never used */
    cw->cury = y;
#ifdef DEBUG
    if (x < 0 || y < 0 || y >= cw->rows || x > cw->cols) {
        const char *s = "[unknown type]";
        switch (cw->type) {
        case NHW_MESSAGE:
            s = "[topl window]";
            break;
        case NHW_STATUS:
            s = "[status window]";
            break;
        case NHW_MAP:
            s = "[map window]";
            break;
        case NHW_MENU:
            s = "[corner window]";
            break;
        case NHW_TEXT:
            s = "[text window]";
            break;
        case NHW_BASE:
            s = "[base window]";
            break;
        }
        debugpline4("bad curs positioning win %d %s (%d,%d)", window, s, x, y);
        /* This return statement caused a functional difference between
           DEBUG and non-DEBUG operation, so it is being commented
           out. It caused tty_curs() to fail to move the cursor to the
           location it needed to be if the x,y range checks failed,
           leaving the next piece of output to be displayed at whatever
           random location the cursor happened to be at prior. */

        /* return; */
    }
#endif
    x += cw->offx;
    y += cw->offy;

#ifdef CLIPPING
    if (clipping && window == WIN_MAP) {
        x -= clipx;
        y -= clipy;
    }
#endif

    if (y == cy && x == cx)
        return;

    if (cw->type == NHW_MAP)
        end_glyphout();

#ifndef NO_TERMS
    if (!nh_ND && (cx != x || x <= 3)) { /* Extremely primitive */
        cmov(x, y);                      /* bunker!wtm */
        return;
    }
#endif

    if ((cy -= y) < 0)
        cy = -cy;
    if ((cx -= x) < 0)
        cx = -cx;
    if (cy <= 3 && cx <= 3) {
        nocmov(x, y);
#ifndef NO_TERMS
    } else if ((x <= 3 && cy <= 3) || (!nh_CM && x < cx)) {
        (void) putchar('\r');
        ttyDisplay->curx = 0;
        nocmov(x, y);
    } else if (!nh_CM) {
        nocmov(x, y);
#endif
    } else
        cmov(x, y);

    ttyDisplay->curx = x;
    ttyDisplay->cury = y;
}

STATIC_OVL void
tty_putsym(window, x, y, ch)
winid window;
int x, y;
char ch;
{
    register struct WinDesc *cw = 0;

    if (window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);

    print_vt_code2(AVTC_SELECT_WINDOW, window);

    switch (cw->type) {
#ifndef STATUS_HILITES
    case NHW_STATUS:
#endif
    case NHW_MAP:
    case NHW_BASE:
        tty_curs(window, x, y);
        (void) putchar(ch);
        ttyDisplay->curx++;
        cw->curx++;
        break;
    case NHW_MESSAGE:
    case NHW_MENU:
    case NHW_TEXT:
        impossible("Can't putsym to window type %d", cw->type);
        break;
    }
}

STATIC_OVL const char *
compress_str(str)
const char *str;
{
    static char cbuf[BUFSZ];

    /* compress out consecutive spaces if line is too long;
       topline wrapping converts space at wrap point into newline,
       we reverse that here */
    if ((int) strlen(str) >= CO || index(str, '\n')) {
        const char *in_str = str;
        char c, *outstr = cbuf, *outend = &cbuf[sizeof cbuf - 1];
        boolean was_space = TRUE; /* True discards all leading spaces;
                                     False would retain one if present */

        while ((c = *in_str++) != '\0' && outstr < outend) {
            if (c == '\n')
                c = ' ';
            if (was_space && c == ' ')
                continue;
            *outstr++ = c;
            was_space = (c == ' ');
        }
        if ((was_space && outstr > cbuf) || outstr == outend)
            --outstr; /* remove trailing space or make room for terminator */
        *outstr = '\0';
        str = cbuf;
    }
    return str;
}

void
tty_putstr(window, attr, str)
winid window;
int attr;
const char *str;
{
    register struct WinDesc *cw = 0;
    register char *ob;
    register long i, n0;
#ifndef STATUS_HILITES
    register const char *nb;
    register long j;
#endif

    /* Assume there's a real problem if the window is missing --
     * probably a panic message
     */
    if (window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0) {
        tty_raw_print(str);
        return;
    }

    if (str == (const char *) 0
        || ((cw->flags & WIN_CANCELLED) && (cw->type != NHW_MESSAGE)))
        return;
    if (cw->type != NHW_MESSAGE)
        str = compress_str(str);

    ttyDisplay->lastwin = window;

    print_vt_code2(AVTC_SELECT_WINDOW, window);

    switch (cw->type) {
    case NHW_MESSAGE:
        /* really do this later */
#if defined(USER_SOUNDS) && defined(WIN32CON)
        play_sound_for_message(str);
#endif
        update_topl(str);
        break;
#ifndef STATUS_HILITES
    case NHW_STATUS:
        ob = &cw->data[cw->cury][j = cw->curx];
        if (context.botlx)
            *ob = 0;
        if (!cw->cury && (int) strlen(str) >= CO) {
            /* the characters before "St:" are unnecessary */
            nb = index(str, ':');
            if (nb && nb > str + 2)
                str = nb - 2;
        }
        nb = str;
        for (i = cw->curx + 1, n0 = cw->cols; i < n0; i++, nb++) {
            if (!*nb) {
                if (*ob || context.botlx) {
                    /* last char printed may be in middle of line */
                    tty_curs(WIN_STATUS, i, cw->cury);
                    cl_end();
                }
                break;
            }
            if (*ob != *nb)
                tty_putsym(WIN_STATUS, i, cw->cury, *nb);
            if (*ob)
                ob++;
        }

        (void) strncpy(&cw->data[cw->cury][j], str, cw->cols - j - 1);
        cw->data[cw->cury][cw->cols - 1] = '\0'; /* null terminate */
        cw->cury = (cw->cury + 1) % 2;
        cw->curx = 0;
        break;
#endif /* STATUS_HILITES */
    case NHW_MAP:
        tty_curs(window, cw->curx + 1, cw->cury);
        term_start_attr(attr);
        while (*str && (int) ttyDisplay->curx < (int) ttyDisplay->cols - 1) {
            (void) putchar(*str);
            str++;
            ttyDisplay->curx++;
        }
        cw->curx = 0;
        cw->cury++;
        term_end_attr(attr);
        break;
    case NHW_BASE:
        tty_curs(window, cw->curx + 1, cw->cury);
        term_start_attr(attr);
        while (*str) {
            if ((int) ttyDisplay->curx >= (int) ttyDisplay->cols - 1) {
                cw->curx = 0;
                cw->cury++;
                tty_curs(window, cw->curx + 1, cw->cury);
            }
            (void) putchar(*str);
            str++;
            ttyDisplay->curx++;
        }
        cw->curx = 0;
        cw->cury++;
        term_end_attr(attr);
        break;
    case NHW_MENU:
    case NHW_TEXT:
#ifdef H2344_BROKEN
        if (cw->type == NHW_TEXT
            && (cw->cury + cw->offy) == ttyDisplay->rows - 1)
#else
        if (cw->type == NHW_TEXT && cw->cury == ttyDisplay->rows - 1)
#endif
        {
            /* not a menu, so save memory and output 1 page at a time */
            cw->maxcol = ttyDisplay->cols; /* force full-screen mode */
            tty_display_nhwindow(window, TRUE);
            for (i = 0; i < cw->maxrow; i++)
                if (cw->data[i]) {
                    free((genericptr_t) cw->data[i]);
                    cw->data[i] = 0;
                }
            cw->maxrow = cw->cury = 0;
        }
        /* always grows one at a time, but alloc 12 at a time */
        if (cw->cury >= cw->rows) {
            char **tmp;

            cw->rows += 12;
            tmp = (char **) alloc(sizeof(char *) * (unsigned) cw->rows);
            for (i = 0; i < cw->maxrow; i++)
                tmp[i] = cw->data[i];
            if (cw->data)
                free((genericptr_t) cw->data);
            cw->data = tmp;

            for (i = cw->maxrow; i < cw->rows; i++)
                cw->data[i] = 0;
        }
        if (cw->data[cw->cury])
            free((genericptr_t) cw->data[cw->cury]);
        n0 = (long) strlen(str) + 1L;
        ob = cw->data[cw->cury] = (char *) alloc((unsigned) n0 + 1);
        *ob++ = (char) (attr + 1); /* avoid nuls, for convenience */
        Strcpy(ob, str);

        if (n0 > cw->maxcol)
            cw->maxcol = n0;
        if (++cw->cury > cw->maxrow)
            cw->maxrow = cw->cury;
        if (n0 > CO) {
            /* attempt to break the line */
            for (i = CO - 1; i && str[i] != ' ' && str[i] != '\n';)
                i--;
            if (i) {
                cw->data[cw->cury - 1][++i] = '\0';
                tty_putstr(window, attr, &str[i]);
            }
        }
        break;
    }
}

void
tty_display_file(fname, complain)
const char *fname;
boolean complain;
{
#ifdef DEF_PAGER /* this implies that UNIX is defined */
    {
        /* use external pager; this may give security problems */
        register int fd = open(fname, 0);

        if (fd < 0) {
            if (complain)
                pline("Cannot open %s.", fname);
            else
                docrt();
            return;
        }
        if (child(1)) {
            /* Now that child() does a setuid(getuid()) and a chdir(),
               we may not be able to open file fname anymore, so make
               it stdin. */
            (void) close(0);
            if (dup(fd)) {
                if (complain)
                    raw_printf("Cannot open %s as stdin.", fname);
            } else {
                (void) execlp(catmore, "page", (char *) 0);
                if (complain)
                    raw_printf("Cannot exec %s.", catmore);
            }
            if (complain)
                sleep(10); /* want to wait_synch() but stdin is gone */
            nh_terminate(EXIT_FAILURE);
        }
        (void) close(fd);
#ifdef notyet
        winch_seen = 0;
#endif
    }
#else /* DEF_PAGER */
    {
        dlb *f;
        char buf[BUFSZ];
        char *cr;

        tty_clear_nhwindow(WIN_MESSAGE);
        f = dlb_fopen(fname, "r");
        if (!f) {
            if (complain) {
                home();
                tty_mark_synch();
                tty_raw_print("");
                perror(fname);
                tty_wait_synch();
                pline("Cannot open \"%s\".", fname);
            } else if (u.ux)
                docrt();
        } else {
            winid datawin = tty_create_nhwindow(NHW_TEXT);
            boolean empty = TRUE;

            if (complain
#ifndef NO_TERMS
                && nh_CD
#endif
                ) {
                /* attempt to scroll text below map window if there's room */
                wins[datawin]->offy = wins[WIN_STATUS]->offy + 3;
                if ((int) wins[datawin]->offy + 12 > (int) ttyDisplay->rows)
                    wins[datawin]->offy = 0;
            }
            while (dlb_fgets(buf, BUFSZ, f)) {
                if ((cr = index(buf, '\n')) != 0)
                    *cr = 0;
#ifdef MSDOS
                if ((cr = index(buf, '\r')) != 0)
                    *cr = 0;
#endif
                if (index(buf, '\t') != 0)
                    (void) tabexpand(buf);
                empty = FALSE;
                tty_putstr(datawin, 0, buf);
                if (wins[datawin]->flags & WIN_CANCELLED)
                    break;
            }
            if (!empty)
                tty_display_nhwindow(datawin, FALSE);
            tty_destroy_nhwindow(datawin);
            (void) dlb_fclose(f);
        }
    }
#endif /* DEF_PAGER */
}

void
tty_start_menu(window)
winid window;
{
    tty_clear_nhwindow(window);
    return;
}

/*ARGSUSED*/
/*
 * Add a menu item to the beginning of the menu list.  This list is reversed
 * later.
 */
void
tty_add_menu(window, glyph, identifier, ch, gch, attr, str, preselected)
winid window;               /* window to use, must be of type NHW_MENU */
int glyph UNUSED;           /* glyph to display with item (not used) */
const anything *identifier; /* what to return if selected */
char ch;                    /* keyboard accelerator (0 = pick our own) */
char gch;                   /* group accelerator (0 = no group) */
int attr;                   /* attribute for string (like tty_putstr()) */
const char *str;            /* menu string */
boolean preselected;        /* item is marked as selected */
{
    register struct WinDesc *cw = 0;
    tty_menu_item *item;
    const char *newstr;
    char buf[4 + BUFSZ];

    if (str == (const char *) 0)
        return;

    if (window == WIN_ERR
        || (cw = wins[window]) == (struct WinDesc *) 0
        || cw->type != NHW_MENU)
        panic(winpanicstr, window);

    cw->nitems++;
    if (identifier->a_void) {
        int len = strlen(str);

        if (len >= BUFSZ) {
            /* We *think* everything's coming in off at most BUFSZ bufs... */
            impossible("Menu item too long (%d).", len);
            len = BUFSZ - 1;
        }
        Sprintf(buf, "%c - ", ch ? ch : '?');
        (void) strncpy(buf + 4, str, len);
        buf[4 + len] = '\0';
        newstr = buf;
    } else
        newstr = str;

    item = (tty_menu_item *) alloc(sizeof(tty_menu_item));
    item->identifier = *identifier;
    item->count = -1L;
    item->selected = preselected;
    item->selector = ch;
    item->gselector = gch;
    item->attr = attr;
    item->str = dupstr(newstr ? newstr : "");

    item->next = cw->mlist;
    cw->mlist = item;
}

/* Invert the given list, can handle NULL as an input. */
STATIC_OVL tty_menu_item *
reverse(curr)
tty_menu_item *curr;
{
    tty_menu_item *next, *head = 0;

    while (curr) {
        next = curr->next;
        curr->next = head;
        head = curr;
        curr = next;
    }
    return head;
}

/*
 * End a menu in this window, window must a type NHW_MENU.  This routine
 * processes the string list.  We calculate the # of pages, then assign
 * keyboard accelerators as needed.  Finally we decide on the width and
 * height of the window.
 */
void
tty_end_menu(window, prompt)
winid window;       /* menu to use */
const char *prompt; /* prompt to for menu */
{
    struct WinDesc *cw = 0;
    tty_menu_item *curr;
    short len;
    int lmax, n;
    char menu_ch;

    if (window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0
        || cw->type != NHW_MENU)
        panic(winpanicstr, window);

    /* Reverse the list so that items are in correct order. */
    cw->mlist = reverse(cw->mlist);

    /* Put the prompt at the beginning of the menu. */
    if (prompt) {
        anything any;

        any = zeroany; /* not selectable */
        tty_add_menu(window, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
                     MENU_UNSELECTED);
        tty_add_menu(window, NO_GLYPH, &any, 0, 0, ATR_NONE, prompt,
                     MENU_UNSELECTED);
    }

    /* XXX another magic number? 52 */
    lmax = min(52, (int) ttyDisplay->rows - 1);    /* # lines per page */
    cw->npages = (cw->nitems + (lmax - 1)) / lmax; /* # of pages */

    /* make sure page list is large enough */
    if (cw->plist_size < cw->npages + 1 /*need 1 slot beyond last*/) {
        if (cw->plist)
            free((genericptr_t) cw->plist);
        cw->plist_size = cw->npages + 1;
        cw->plist = (tty_menu_item **) alloc(cw->plist_size
                                             * sizeof(tty_menu_item *));
    }

    cw->cols = 0;  /* cols is set when the win is initialized... (why?) */
    menu_ch = '?'; /* lint suppression */
    for (n = 0, curr = cw->mlist; curr; n++, curr = curr->next) {
        /* set page boundaries and character accelerators */
        if ((n % lmax) == 0) {
            menu_ch = 'a';
            cw->plist[n / lmax] = curr;
        }
        if (curr->identifier.a_void && !curr->selector) {
            curr->str[0] = curr->selector = menu_ch;
            if (menu_ch++ == 'z')
                menu_ch = 'A';
        }

        /* cut off any lines that are too long */
        len = strlen(curr->str) + 2; /* extra space at beg & end */
        if (len > (int) ttyDisplay->cols) {
            curr->str[ttyDisplay->cols - 2] = 0;
            len = ttyDisplay->cols;
        }
        if (len > cw->cols)
            cw->cols = len;
    }
    cw->plist[cw->npages] = 0; /* plist terminator */

    /*
     * If greater than 1 page, morestr is "(x of y) " otherwise, "(end) "
     */
    if (cw->npages > 1) {
        char buf[QBUFSZ];
        /* produce the largest demo string */
        Sprintf(buf, "(%ld of %ld) ", cw->npages, cw->npages);
        len = strlen(buf);
        cw->morestr = dupstr("");
    } else {
        cw->morestr = dupstr("(end) ");
        len = strlen(cw->morestr);
    }

    if (len > (int) ttyDisplay->cols) {
        /* truncate the prompt if it's too long for the screen */
        if (cw->npages <= 1) /* only str in single page case */
            cw->morestr[ttyDisplay->cols] = 0;
        len = ttyDisplay->cols;
    }
    if (len > cw->cols)
        cw->cols = len;

    cw->maxcol = cw->cols;

    /*
     * The number of lines in the first page plus the morestr will be the
     * maximum size of the window.
     */
    if (cw->npages > 1)
        cw->maxrow = cw->rows = lmax + 1;
    else
        cw->maxrow = cw->rows = cw->nitems + 1;
}

int
tty_select_menu(window, how, menu_list)
winid window;
int how;
menu_item **menu_list;
{
    register struct WinDesc *cw = 0;
    tty_menu_item *curr;
    menu_item *mi;
    int n, cancelled;

    if (window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0
        || cw->type != NHW_MENU)
        panic(winpanicstr, window);

    *menu_list = (menu_item *) 0;
    cw->how = (short) how;
    morc = 0;
    tty_display_nhwindow(window, TRUE);
    cancelled = !!(cw->flags & WIN_CANCELLED);
    tty_dismiss_nhwindow(window); /* does not destroy window data */

    if (cancelled) {
        n = -1;
    } else {
        for (n = 0, curr = cw->mlist; curr; curr = curr->next)
            if (curr->selected)
                n++;
    }

    if (n > 0) {
        *menu_list = (menu_item *) alloc(n * sizeof(menu_item));
        for (mi = *menu_list, curr = cw->mlist; curr; curr = curr->next)
            if (curr->selected) {
                mi->item = curr->identifier;
                mi->count = curr->count;
                mi++;
            }
    }

    return n;
}

/* special hack for treating top line --More-- as a one item menu */
char
tty_message_menu(let, how, mesg)
char let;
int how;
const char *mesg;
{
    /* "menu" without selection; use ordinary pline, no more() */
    if (how == PICK_NONE) {
        pline("%s", mesg);
        return 0;
    }

    ttyDisplay->dismiss_more = let;
    morc = 0;
    /* barebones pline(); since we're only supposed to be called after
       response to a prompt, we'll assume that the display is up to date */
    tty_putstr(WIN_MESSAGE, 0, mesg);
    /* if `mesg' didn't wrap (triggering --More--), force --More-- now */
    if (ttyDisplay->toplin == 1) {
        more();
        ttyDisplay->toplin = 1; /* more resets this */
        tty_clear_nhwindow(WIN_MESSAGE);
    }
    /* normally <ESC> means skip further messages, but in this case
       it means cancel the current prompt; any other messages should
       continue to be output normally */
    wins[WIN_MESSAGE]->flags &= ~WIN_CANCELLED;
    ttyDisplay->dismiss_more = 0;

    return ((how == PICK_ONE && morc == let) || morc == '\033') ? morc : '\0';
}

void
tty_update_inventory()
{
    return;
}

void
tty_mark_synch()
{
    (void) fflush(stdout);
}

void
tty_wait_synch()
{
    /* we just need to make sure all windows are synch'd */
    if (!ttyDisplay || ttyDisplay->rawprint) {
        getret();
        if (ttyDisplay)
            ttyDisplay->rawprint = 0;
    } else {
        tty_display_nhwindow(WIN_MAP, FALSE);
        if (ttyDisplay->inmore) {
            addtopl("--More--");
            (void) fflush(stdout);
        } else if (ttyDisplay->inread > program_state.gameover) {
            /* this can only happen if we were reading and got interrupted */
            ttyDisplay->toplin = 3;
            /* do this twice; 1st time gets the Quit? message again */
            (void) tty_doprev_message();
            (void) tty_doprev_message();
            ttyDisplay->intr++;
            (void) fflush(stdout);
        }
    }
}

void
docorner(xmin, ymax)
register int xmin, ymax;
{
    register int y;
    register struct WinDesc *cw = wins[WIN_MAP];

#if 0   /* this optimization is not valuable enough to justify
           abusing core internals... */
    if (u.uswallow) { /* Can be done more efficiently */
        swallowed(1);
        /* without this flush, if we happen to follow --More-- displayed in
           leftmost column, the cursor gets left in the wrong place after
           <docorner<more<update_topl<tty_putstr calls unwind back to core */
        flush_screen(0);
        return;
    }
#endif /*0*/

#if defined(SIGWINCH) && defined(CLIPPING)
    if (ymax > LI)
        ymax = LI; /* can happen if window gets smaller */
#endif
    for (y = 0; y < ymax; y++) {
        tty_curs(BASE_WINDOW, xmin, y); /* move cursor */
        cl_end();                       /* clear to end of line */
#ifdef CLIPPING
        if (y < (int) cw->offy || y + clipy > ROWNO)
            continue; /* only refresh board */
#if defined(USE_TILES) && defined(MSDOS)
        if (iflags.tile_view)
            row_refresh((xmin / 2) + clipx - ((int) cw->offx / 2), COLNO - 1,
                        y + clipy - (int) cw->offy);
        else
#endif
            row_refresh(xmin + clipx - (int) cw->offx, COLNO - 1,
                        y + clipy - (int) cw->offy);
#else
        if (y < cw->offy || y > ROWNO)
            continue; /* only refresh board  */
        row_refresh(xmin - (int) cw->offx, COLNO - 1, y - (int) cw->offy);
#endif
    }

    end_glyphout();
    if (ymax >= (int) wins[WIN_STATUS]->offy) {
        /* we have wrecked the bottom line */
        context.botlx = 1;
        bot();
    }
}

void
end_glyphout()
{
#if defined(ASCIIGRAPH) && !defined(NO_TERMS)
    if (GFlag) {
        GFlag = FALSE;
        graph_off();
    }
#endif
#ifdef TEXTCOLOR
    if (ttyDisplay->color != NO_COLOR) {
        term_end_color();
        ttyDisplay->color = NO_COLOR;
    }
#endif
}

#ifndef WIN32
void
g_putch(in_ch)
int in_ch;
{
    register char ch = (char) in_ch;

#if defined(ASCIIGRAPH) && !defined(NO_TERMS)
    if (SYMHANDLING(H_IBM) || iflags.eight_bit_tty) {
        /* IBM-compatible displays don't need other stuff */
        (void) putchar(ch);
    } else if (ch & 0x80) {
        if (!GFlag || HE_resets_AS) {
            graph_on();
            GFlag = TRUE;
        }
        (void) putchar((ch ^ 0x80)); /* Strip 8th bit */
    } else {
        if (GFlag) {
            graph_off();
            GFlag = FALSE;
        }
        (void) putchar(ch);
    }

#else
    (void) putchar(ch);

#endif /* ASCIIGRAPH && !NO_TERMS */

    return;
}
#endif /* !WIN32 */

#ifdef CLIPPING
void
setclipped()
{
    clipping = TRUE;
    clipx = clipy = 0;
    clipxmax = CO;
    clipymax = LI - 3;
}

void
tty_cliparound(x, y)
int x, y;
{
    extern boolean restoring;
    int oldx = clipx, oldy = clipy;

    if (!clipping)
        return;
    if (x < clipx + 5) {
        clipx = max(0, x - 20);
        clipxmax = clipx + CO;
    } else if (x > clipxmax - 5) {
        clipxmax = min(COLNO, clipxmax + 20);
        clipx = clipxmax - CO;
    }
    if (y < clipy + 2) {
        clipy = max(0, y - (clipymax - clipy) / 2);
        clipymax = clipy + (LI - 3);
    } else if (y > clipymax - 2) {
        clipymax = min(ROWNO, clipymax + (clipymax - clipy) / 2);
        clipy = clipymax - (LI - 3);
    }
    if (clipx != oldx || clipy != oldy) {
        if (on_level(&u.uz0, &u.uz) && !restoring)
            (void) doredraw();
    }
}
#endif /* CLIPPING */

/*
 *  tty_print_glyph
 *
 *  Print the glyph to the output device.  Don't flush the output device.
 *
 *  Since this is only called from show_glyph(), it is assumed that the
 *  position and glyph are always correct (checked there)!
 */

void
tty_print_glyph(window, x, y, glyph, bkglyph)
winid window;
xchar x, y;
int glyph;
int bkglyph UNUSED;
{
    int ch;
    boolean reverse_on = FALSE;
    int color;
    unsigned special;

#ifdef CLIPPING
    if (clipping) {
        if (x <= clipx || y < clipy || x >= clipxmax || y >= clipymax)
            return;
    }
#endif
    /* map glyph to character and color */
    (void) mapglyph(glyph, &ch, &color, &special, x, y);

    print_vt_code2(AVTC_SELECT_WINDOW, window);

    /* Move the cursor. */
    tty_curs(window, x, y);

    print_vt_code3(AVTC_GLYPH_START, glyph2tile[glyph], special);

#ifndef NO_TERMS
    if (ul_hack && ch == '_') { /* non-destructive underscore */
        (void) putchar((char) ' ');
        backsp();
    }
#endif

#ifdef TEXTCOLOR
    if (color != ttyDisplay->color) {
        if (ttyDisplay->color != NO_COLOR)
            term_end_color();
        ttyDisplay->color = color;
        if (color != NO_COLOR)
            term_start_color(color);
    }
#endif /* TEXTCOLOR */

    /* must be after color check; term_end_color may turn off inverse too */
    if (((special & MG_PET) && iflags.hilite_pet)
        || ((special & MG_OBJPILE) && iflags.hilite_pile)
        || ((special & MG_DETECT) && iflags.use_inverse)
        || ((special & MG_BW_LAVA) && iflags.use_inverse)) {
        term_start_attr(ATR_INVERSE);
        reverse_on = TRUE;
    }

#if defined(USE_TILES) && defined(MSDOS)
    if (iflags.grmode && iflags.tile_view)
        xputg(glyph, ch, special);
    else
#endif
        g_putch(ch); /* print the character */

    if (reverse_on) {
        term_end_attr(ATR_INVERSE);
#ifdef TEXTCOLOR
        /* turn off color as well, ATR_INVERSE may have done this already */
        if (ttyDisplay->color != NO_COLOR) {
            term_end_color();
            ttyDisplay->color = NO_COLOR;
        }
#endif
    }

    print_vt_code1(AVTC_GLYPH_END);

    wins[window]->curx++; /* one character over */
    ttyDisplay->curx++;   /* the real cursor moved too */
}

void
tty_raw_print(str)
const char *str;
{
    if (ttyDisplay)
        ttyDisplay->rawprint++;
    print_vt_code2(AVTC_SELECT_WINDOW, NHW_BASE);
#if defined(MICRO) || defined(WIN32CON)
    msmsg("%s\n", str);
#else
    puts(str);
    (void) fflush(stdout);
#endif
}

void
tty_raw_print_bold(str)
const char *str;
{
    if (ttyDisplay)
        ttyDisplay->rawprint++;
    print_vt_code2(AVTC_SELECT_WINDOW, NHW_BASE);
    term_start_raw_bold();
#if defined(MICRO) || defined(WIN32CON)
    msmsg("%s", str);
#else
    (void) fputs(str, stdout);
#endif
    term_end_raw_bold();
#if defined(MICRO) || defined(WIN32CON)
    msmsg("\n");
#else
    puts("");
    (void) fflush(stdout);
#endif
}

int
tty_nhgetch()
{
    int i;
#ifdef UNIX
    /* kludge alert: Some Unix variants return funny values if getc()
     * is called, interrupted, and then called again.  There
     * is non-reentrant code in the internal _filbuf() routine, called by
     * getc().
     */
    static volatile int nesting = 0;
    char nestbuf;
#endif

    print_vt_code1(AVTC_INLINE_SYNC);
    (void) fflush(stdout);
    /* Note: if raw_print() and wait_synch() get called to report terminal
     * initialization problems, then wins[] and ttyDisplay might not be
     * available yet.  Such problems will probably be fatal before we get
     * here, but validate those pointers just in case...
     */
    if (WIN_MESSAGE != WIN_ERR && wins[WIN_MESSAGE])
        wins[WIN_MESSAGE]->flags &= ~WIN_STOP;
#ifdef UNIX
    i = (++nesting == 1) ? tgetch()
                         : (read(fileno(stdin), (genericptr_t) &nestbuf, 1)
                            == 1) ? (int) nestbuf : EOF;
    --nesting;
#else
    i = tgetch();
#endif
    if (!i)
        i = '\033'; /* map NUL to ESC since nethack doesn't expect NUL */
    else if (i == EOF)
        i = '\033'; /* same for EOF */
    if (ttyDisplay && ttyDisplay->toplin == 1)
        ttyDisplay->toplin = 2;
#ifdef TTY_TILES_ESCCODES
    {
        /* hack to force output of the window select code */
        int tmp = vt_tile_current_window;
        vt_tile_current_window++;
        print_vt_code2(AVTC_SELECT_WINDOW, tmp);
    }
#endif /* TTY_TILES_ESCCODES */
    return i;
}

/*
 * return a key, or 0, in which case a mouse button was pressed
 * mouse events should be returned as character postitions in the map window.
 * Since normal tty's don't have mice, just return a key.
 */
/*ARGSUSED*/
int
tty_nh_poskey(x, y, mod)
int *x, *y, *mod;
{
#if defined(WIN32CON)
    int i;
    (void) fflush(stdout);
    /* Note: if raw_print() and wait_synch() get called to report terminal
     * initialization problems, then wins[] and ttyDisplay might not be
     * available yet.  Such problems will probably be fatal before we get
     * here, but validate those pointers just in case...
     */
    if (WIN_MESSAGE != WIN_ERR && wins[WIN_MESSAGE])
        wins[WIN_MESSAGE]->flags &= ~WIN_STOP;
    i = ntposkey(x, y, mod);
    if (!i && mod && (*mod == 0 || *mod == EOF))
        i = '\033'; /* map NUL or EOF to ESC, nethack doesn't expect either */
    if (ttyDisplay && ttyDisplay->toplin == 1)
        ttyDisplay->toplin = 2;
    return i;
#else /* !WIN32CON */
    nhUse(x);
    nhUse(y);
    nhUse(mod);

    return tty_nhgetch();
#endif /* ?WIN32CON */
}

void
win_tty_init(dir)
int dir;
{
    if (dir != WININIT)
        return;
    return;
}

#ifdef POSITIONBAR
void
tty_update_positionbar(posbar)
char *posbar;
{
#ifdef MSDOS
    video_update_positionbar(posbar);
#endif
}
#endif /* POSITIONBAR */


/*
 * +------------------+
 * |  STATUS CODE     |
 * +------------------+
 */

/*
 * ----------------------------------------------------------------
 * tty_status_update
 *
 *      Called from the NetHack core and receives info hand-offs
 *      from the core, processes them, storing some information
 *      for rendering to the tty.
 *          -> make_things_fit()
 *          -> render_status()
 *
 * make_things_fit
 *
 *      Called from tty_status_update(). It calls check_fields()
 *      (described next) to get the required number of columns
 *      and tries a few different ways to squish a representation
 *      of the status window values onto the 80 column tty display.
 *          ->check_fields()
 *          ->condition_size()  - get the width of all conditions
 *          ->shrink_enc()      - shrink encumbrance message word
 *          ->shrink_dlvl()     - reduce the width of Dlvl:42
 *
 *  check_fields
 *
 *      Verifies that all the fields are ready for display, and
 *      returns FALSE if called too early in the startup
 *      processing sequence. It also figures out where everything
 *      needs to go, taking the current shrinking attempts into
 *      account. It returns number of columns needed back to
 *      make_things_fit(), so make_things_fit() can make attempt
 *      to make adjustments.
 *
 *  render_status
 *
 *      Goes through each of the two status row's fields and
 *      calls tty_putstatusfield() to place them on the display.
 *          ->tty_putstatusfield()
 *
 *  tty_putstatusfield()
 *
 *      Move the cursor to the target spot, and output the field
 *      asked for by render_status().
 *
 * ----------------------------------------------------------------
 */

/*
 * The following data structures come from the genl_ routines in
 * src/windows.c and as such are considered to be on the window-port
 * "side" of things, rather than the NetHack-core "side" of things.
 */
extern const char *status_fieldfmt[MAXBLSTATS];
extern char *status_vals[MAXBLSTATS];
extern boolean status_activefields[MAXBLSTATS];
extern winid WIN_STATUS;

const char *fieldnames[] = {
    "title",  "strength", "dexterity", "constitution", "intelligence",
    "wisdom",  "charisma", "alignment", "score", "carrying-capacity",
    "gold", "power",  "power-max", "experience-level", "armor-class",
    "HD",  "time",  "hunger",  "hitpoints",  "hitpoints-max",
    "dungeon-level",  "experience",  "condition",
};

#ifdef STATUS_HILITES
#ifdef TEXTCOLOR
STATIC_DCL int FDECL(condcolor, (long, unsigned long *));
#endif
STATIC_DCL int FDECL(condattr, (long, unsigned long *));
static unsigned long *tty_colormasks;
static long tty_condition_bits;
int cond_disp_width[2];                    /* 2: current and previous */
static struct tty_status_fields
                tty_status[2][MAXBLSTATS]; /* 2: first index is for current
                                                 and previous */
static int hpbar_percent, hpbar_color;
static struct condition_t {
    long mask;
    const char *text[3];  /* 3: potential display values, progressively
                           * smaller */
} conditions[] = {
    /* The sequence order of these matters */
   { BL_MASK_STONE,    {"Stone", "Ston", "Sto"}},
   { BL_MASK_SLIME,    {"Slime", "Slim", "Slm"}},
   { BL_MASK_STRNGL,   {"Strngl", "Stngl", "Str"}},
   { BL_MASK_FOODPOIS, {"FoodPois", "Fpois", "Poi"}},
   { BL_MASK_TERMILL,  {"TermIll" , "Ill", "Ill"}},
   { BL_MASK_BLIND,    {"Blind", "Blnd", "Bl"}},
   { BL_MASK_DEAF,     {"Deaf", "Def", "Df"}},
   { BL_MASK_STUN,     {"Stun", "Stun", "St"}},
   { BL_MASK_CONF,     {"Conf", "Cnf", "Cn"}},
   { BL_MASK_HALLU,    {"Hallu", "Hal", "Ha"}},
   { BL_MASK_LEV,      {"Lev", "Lev", "Lv"}},
   { BL_MASK_FLY,      {"Fly", "Fly", "Fl"}},
   { BL_MASK_RIDE,     {"Ride", "Rid", "Ri"}},
};
static const char *encvals[3][6] = {
    { "", "Burdened", "Stressed", "Strained", "Overtaxed", "Overloaded"},
    { "", "Burden", "Stress", "Strain", "Overtax", "Overload" },
    { "", "Brd", "Strs", "Strn", "Ovtx", "Ovld" }
};
#define MAX_PER_ROW 15
static enum statusfields fieldorder[2][MAX_PER_ROW] = { /* 2: two status lines */
    { BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_ALIGN,
      BL_SCORE, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH,
      BL_FLUSH },
    { BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX,
      BL_AC, BL_XP, BL_EXP, BL_HD, BL_TIME, BL_HUNGER,
      BL_CAP, BL_CONDITION, BL_FLUSH }
};

static int last_on_row[2];  /* [rows] */
static int finalx[2][2];    /* [rows][NOW or BEFORE] */
static boolean setlast = FALSE;
static boolean windowdata_init = FALSE;
static int cond_shrinklvl = 0, cond_width_at_shrink = 0;
static int enclev = 0, enc_shrinklvl = 0;
/* static int dl_shrinklvl = 0; */
static boolean truncation_expected = FALSE;
#define FORCE_RESET TRUE
#define NO_RESET FALSE

/* This controls whether to skip fields that aren't
 * flagged as requiring updating during the current
 * render_status().
 *
 * Hopefully that can be confirmed as working correctly
 * for all platforms eventually and the conditional
 * setting below can be removed.
 */
static int do_field_opt = 
#if defined(DISABLE_TTY_FIELD_OPT)
    0;
#else
    1;
#endif

#endif  /* STATUS_HILITES */

/*
 *  tty_status_init()
 *      -- initialize the tty-specific data structures.
 *      -- call genl_status_init() to initialize the general data.
 */
void
tty_status_init()
{
#ifdef STATUS_HILITES
    int i;

    for (i = 0; i < MAXBLSTATS; ++i) {
        tty_status[NOW][i].idx = BL_FLUSH;
        tty_status[NOW][i].color = NO_COLOR; /* no color */
        tty_status[NOW][i].attr = ATR_NONE;
        tty_status[NOW][i].x = 0;
        tty_status[NOW][i].y = 0;
        tty_status[NOW][i].valid  = FALSE;
        tty_status[NOW][i].dirty  = FALSE;
        tty_status[NOW][i].redraw = FALSE;
        tty_status[NOW][i].last_on_row = FALSE;
        tty_status[BEFORE][i] = tty_status[NOW][i];
    }
    tty_condition_bits = 0L;
    hpbar_percent = 0, hpbar_color = NO_COLOR;
#endif /* STATUS_HILITES */

    /* let genl_status_init do most of the initialization */
    genl_status_init();
}

void
tty_status_enablefield(fieldidx, nm, fmt, enable)
int fieldidx;
const char *nm;
const char *fmt;
boolean enable;
{
    genl_status_enablefield(fieldidx, nm, fmt, enable);
    /* force re-evaluation of last field on the row */
    setlast = FALSE;
}

void
do_setlast()
{
    int i, row, fld;

    setlast = TRUE;
    for (row = 0; row < 2; ++row)
        for (i = MAX_PER_ROW - 1; i > 0; --i) {
           fld = fieldorder[row][i];

           if (fld == BL_FLUSH || !status_activefields[fld])
                continue;

           last_on_row[row] = fld;
           break;
	}
}

#ifdef STATUS_HILITES

/*
 *  *_status_update()
 *      -- update the value of a status field.
 *      -- the fldindex identifies which field is changing and
 *         is an integer index value from botl.h
 *      -- fldindex could be any one of the following from botl.h:
 *         BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH,
 *         BL_ALIGN, BL_SCORE, BL_CAP, BL_GOLD, BL_ENE, BL_ENEMAX,
 *         BL_XP, BL_AC, BL_HD, BL_TIME, BL_HUNGER, BL_HP, BL_HPMAX,
 *         BL_LEVELDESC, BL_EXP, BL_CONDITION
 *      -- fldindex could also be BL_FLUSH (-1), which is not really
 *         a field index, but is a special trigger to tell the 
 *         windowport that it should output all changes received
 *         to this point. It marks the end of a bot() cycle.
 *      -- fldindex could also be BL_RESET (-3), which is not really
 *         a field index, but is a special advisory to to tell the 
 *         windowport that it should redisplay all its status fields,
 *         even if no changes have been presented to it.
 *      -- ptr is usually a "char *", unless fldindex is BL_CONDITION.
 *         If fldindex is BL_CONDITION, then ptr is a long value with
 *         any or none of the following bits set (from botl.h):
 *              BL_MASK_STONE           0x00000001L
 *              BL_MASK_SLIME           0x00000002L
 *              BL_MASK_STRNGL          0x00000004L
 *              BL_MASK_FOODPOIS        0x00000008L
 *              BL_MASK_TERMILL         0x00000010L
 *              BL_MASK_BLIND           0x00000020L
 *              BL_MASK_DEAF            0x00000040L
 *              BL_MASK_STUN            0x00000080L
 *              BL_MASK_CONF            0x00000100L
 *              BL_MASK_HALLU           0x00000200L
 *              BL_MASK_LEV             0x00000400L
 *              BL_MASK_FLY             0x00000800L
 *              BL_MASK_RIDE            0x00001000L
 *      -- The value passed for BL_GOLD includes an encoded leading
 *         symbol for GOLD "\GXXXXNNNN:nnn". If the window port needs to use
 *         the textual gold amount without the leading "$:" the port will
 *         have to skip past ':' in the passed "ptr" for the BL_GOLD case.
 *      -- color is an unsigned int.
 *               color_index = color & 0x00FF;         CLR_* value
 *               attribute   = (color >> 8) & 0x00FF;  HL_ATTCLR_* mask
 *         This holds the color and attribute that the field should
 *         be displayed in.
 *         This is relevant for everything except BL_CONDITION fldindex.
 *         If fldindex is BL_CONDITION, this parameter should be ignored,
 *         as condition highlighting is done via the next colormasks
 *         parameter instead.
 *
 *      -- colormasks - pointer to cond_hilites[] array of colormasks.
 *         Only relevant for BL_CONDITION fldindex. The window port
 *         should ignore this parameter for other fldindex values.
 *         Each condition bit must only ever appear in one of the
 *         CLR_ array members, but can appear in multiple HL_ATTCLR_
 *         offsets (because more than one attribute can co-exist).
 *         See doc/window.doc for more details.
 */

void
tty_status_update(fldidx, ptr, chg, percent, color, colormasks)
int fldidx, chg UNUSED, percent, color;
genericptr_t ptr;
unsigned long *colormasks;
{
    int i;
    long *condptr = (long *) ptr;
    char *text = (char *) ptr;
    char *lastchar = (char *) 0;
    char *fval = (char *) 0;
    boolean reset_state = NO_RESET;

    if ((fldidx < BL_RESET) || (fldidx >= MAXBLSTATS))
        return;

    if ((fldidx >= 0 && fldidx < MAXBLSTATS) && !status_activefields[fldidx])
        return;

    if (!setlast)
        do_setlast();

#ifndef TEXTCOLOR
    color = (color & ~0x00FF) | NO_COLOR;
#endif

    switch (fldidx) {
    case BL_RESET:
        reset_state = FORCE_RESET;
        /* FALLTHRU */
    case BL_FLUSH:
        if (make_things_fit(reset_state) || truncation_expected)
            render_status();
        return;
    case BL_CONDITION:
        tty_status[NOW][fldidx].idx = fldidx;
        tty_condition_bits = *condptr;
        tty_colormasks = colormasks;
        tty_status[NOW][fldidx].valid = TRUE;
        tty_status[NOW][fldidx].dirty = TRUE;
        truncation_expected = FALSE;
        break;
    default:
        tty_status[NOW][fldidx].idx = fldidx;
        Sprintf(status_vals[fldidx],
                status_fieldfmt[fldidx] ? status_fieldfmt[fldidx] : "%s",
                text);
        tty_status[NOW][fldidx].color = (color & 0x00FF);
        tty_status[NOW][fldidx].attr = ((color >> 8) & 0x00FF);
        tty_status[NOW][fldidx].lth = strlen(status_vals[fldidx]);
        tty_status[NOW][fldidx].valid = TRUE;
        tty_status[NOW][fldidx].dirty = TRUE;
        break;
    }

    /* The core botl engine sends a single blank to the window port
       for carrying-capacity when its unused. Let's suppress that */
    if (fldidx >= 0 && fldidx < MAXBLSTATS &&
            tty_status[NOW][fldidx].lth == 1 && status_vals[fldidx][0] == ' ') {
        status_vals[fldidx][0] = '\0';
        tty_status[NOW][fldidx].lth = 0;
    }

    /* default processing above was required before these */
    switch (fldidx) {
    case BL_HP:
        if (iflags.wc2_hitpointbar) {
            /* Special additional processing for hitpointbar */
            hpbar_percent = percent;
            hpbar_color = (color & 0x00FF);
        }
        if (iflags.wc2_hitpointbar && (tty_procs.wincap2 & WC2_FLUSH_STATUS) != 0L) {
            tty_status[NOW][BL_TITLE].color = hpbar_color;
            tty_status[NOW][BL_TITLE].dirty = TRUE;
        }
        break;
    case BL_LEVELDESC:
    case BL_HUNGER:
        /* The core sends trailing blanks for some fields.
           Let's suppress the trailing blanks */
        if (tty_status[NOW][fldidx].lth > 0) {
            lastchar = eos(status_vals[fldidx]);
            lastchar--;
            while (lastchar >= status_vals[fldidx] && *lastchar == ' ') {
                *lastchar-- = '\0';
                tty_status[NOW][fldidx].lth--;
            }
        }
        break;
    case BL_TITLE:
        if (iflags.wc2_hitpointbar)
            tty_status[NOW][fldidx].lth += 2; /* '[' and ']' */
        break;
    case BL_GOLD:
        tty_status[NOW][fldidx].lth -= (10 - 1); /* \GXXXXNNNN counts as 1 */
        break;
    case BL_CAP:
        fval = status_vals[fldidx];
        if (fval) {
            if (*fval == ' ')
                fval++;
            for (i = 0; i < SIZE(encvals); ++i) {
                if (!strcmp(encvals[enc_shrinklvl][i], fval)) {
                    enclev = i;
                    break;              /* for */
                }
            }
        }
        break;
    }
    /* 3.6.2 we only render on BL_FLUSH (or BL_RESET) */
    return;
}

STATIC_OVL int
make_things_fit(force_update)
boolean force_update;
{
    int trycnt, fitting = 0, condsz = 0, requirement = 0;
    int rowsz[2], otheroptions = 0;
    boolean check = FALSE;

    condsz = condition_size();
    for (trycnt = 0; trycnt < 6 && !fitting; ++trycnt) {
        check = check_fields(force_update, &rowsz[0], &rowsz[1]);
        if (!check)
            return 0;

        requirement = rowsz[1];
        if (requirement < wins[WIN_STATUS]->cols - 1) {
            fitting = requirement;
            break;  /* we're good */
        }
        if (trycnt < 2) {
            if (cond_shrinklvl < trycnt + 1) {
                cond_shrinklvl = trycnt + 1;
                condsz = condition_size();
                cond_width_at_shrink = cond_disp_width[NOW];
            }
            continue;
        }
        if (cond_shrinklvl >= 2) {
            /* We've exhausted the condition identifiers shrinkage,
             * so let's try something other things...
             */
            if (otheroptions == 0 || otheroptions == 1) {
                /* try shrinking the encumbrance word */
                shrink_enc(otheroptions + 1);
                otheroptions++;
            } else if (otheroptions == 2) {
                shrink_dlvl(1);
                otheroptions++;
            } else {
                /* Last resort - turn on trunction */
                truncation_expected = TRUE;
                otheroptions++;
            }
        }
    }
    return fitting;
}

/*
 * This is the routine where we figure out where each field
 * should be placed, and flag whether the on-screen details
 * must be updated because they need to change.
 * This is now done at an individual field case-by-case level.
 */
boolean
check_fields(forcefields, topsz, bottomsz)
boolean forcefields;
int *topsz, *bottomsz;
{
    int c, i, row, col, trackx, idx;
    boolean valid = TRUE, matchprev = FALSE, update_right, disregard;

    if (!windowdata_init && !check_windowdata())
        return FALSE;

    for (row = 0; row < 2; ++row) {
        col = 1;
        trackx = 1;
        update_right = FALSE;
        for (i = 0; fieldorder[row][i] != BL_FLUSH; ++i) {
            idx = fieldorder[row][i];
            if (!status_activefields[idx])
                continue;
            if (!tty_status[NOW][idx].valid)
                valid = FALSE;

            tty_status[NOW][idx].y = row;
            tty_status[NOW][idx].x = col;

            /* On a change to the field length, everything 
               further to the right must be updated as well */
            if (tty_status[NOW][idx].lth != tty_status[BEFORE][idx].lth)
                update_right = TRUE;

            if (idx == last_on_row[row])
                tty_status[NOW][idx].last_on_row = TRUE;

            if (!update_right && !forcefields) {
                /*
                 * Check values against those already on the dislay.
                 *  - Is the additional processing time for this worth it?
                 */
                if (do_field_opt) {
                    matchprev = FALSE;
                    disregard = (tty_status[NOW][idx].lth == 0);
                    if (do_field_opt && !disregard
                            && tty_status[NOW][idx].dirty) {
                        /* compare values */
                        const char *ob, *nb;     /* old byte, new byte */

                        c = col - 1;
                        ob = &wins[WIN_STATUS]->data[row][c];
                        nb = status_vals[idx];
                        while (*nb && c < wins[WIN_STATUS]->cols) {
                            if (*nb != *ob)
                                break;
                            nb++;
                            ob++;
                            c++;
                        }
                        if (!*nb && c > col - 1)
                            matchprev = TRUE;
                    }
                } else {
                    matchprev = FALSE;
                }
            }

            /*
             * With STATUS_HILITES, it is possible that the color
             * needs to change even if the text is the same, so
             * we flag that here by setting .redraw.
             * Then, render_status() will see that flag setting
             * and ensure that the tty cell content is updated.
             * After the field has been updated, render_status()
             * will also clear .redraw and .dirty.
             */
            if (forcefields || update_right
                || (!matchprev && tty_status[NOW][idx].dirty && !disregard)
                || tty_status[NOW][idx].color != tty_status[BEFORE][idx].color
                || tty_status[NOW][idx].attr  != tty_status[BEFORE][idx].attr)
                tty_status[NOW][idx].redraw = TRUE;
            col += tty_status[NOW][idx].lth;
        }
        if (row && bottomsz)
            *bottomsz = col + tty_status[NOW][idx].lth;
        else if (topsz)
            *topsz = col + tty_status[NOW][idx].lth;
    }
    return valid;
}

/*
 * This is what places a field on the tty display.
 * If val isn't null, it will be used rather than
 * fld (it takes precedence).
 */
STATIC_OVL void
tty_putstatusfield(fld, val, x, y)
struct tty_status_fields *fld;
const char *val;
int x, y;
{
    int i, n, ncols, lth;
    struct WinDesc *cw = 0;
    const char *text = (char *)0;

    if ((cw = wins[NHW_STATUS]) == (struct WinDesc *) 0)
        panic("Invalid WinDesc\n");

    ncols = cw->cols;
    if (val) {
        text = val;
        lth = strlen(text);
    } else if (fld) {
        text = status_vals[fld->idx];
        lth = fld->lth;
    }
    if (!text)
        return;

    print_vt_code2(AVTC_SELECT_WINDOW, NHW_STATUS);

    if (x < ncols && y < 2) {
        tty_curs(NHW_STATUS, x, y);
        for (i = 0; i < lth; ++i) {
            n = i + x;
            if (n < ncols && *text) {
                (void) putchar(*text);
                ttyDisplay->curx++;
                cw->curx++;
              cw->data[y][n-1] = *text;
                text++;
            }
        }
    } else {
        /* Now we're truncating */
        if (truncation_expected)
            ; /* but we knew in advance */
    }
}

STATIC_OVL int
condition_size()
{
    long mask = 0L;
    int c, x;

    x = 0;
    for (c = 0; c < SIZE(conditions); ++c) {
        mask = conditions[c].mask;
        if ((tty_condition_bits & mask) == mask) {
            x++;    /* for spacer */
            x += (int) strlen(conditions[c].text[cond_shrinklvl]);
        }
    }
    tty_status[NOW][BL_CONDITION].lth = x;
    cond_disp_width[NOW] = x;
    return x;
}

STATIC_OVL void
shrink_enc(lvl)
int lvl;
{
    /* shrink or restore the encumbrance word */
    if (lvl == 0 || lvl <= 2) {
        enc_shrinklvl = lvl;
        Strcpy(status_vals[BL_CAP], encvals[lvl][enclev]);
    }
    tty_status[NOW][BL_CAP].lth = strlen(status_vals[BL_CAP]);
}

STATIC_OVL void
shrink_dlvl(lvl)
int lvl;
{
    /* try changing Dlvl: to Dl: */
    char buf[BUFSZ];
    char *levval = index(status_vals[BL_LEVELDESC], ':');

    if (levval) {
        Strcpy(buf, (lvl == 0) ? "Dlvl" : "Dl");
        Strcat(buf, levval);
        Strcpy(status_vals[BL_LEVELDESC], buf);
        tty_status[NOW][BL_LEVELDESC].lth = strlen(status_vals[BL_LEVELDESC]);
    }
}

/*
 * Ensure the underlying status window data start out
 * blank and null-terminated.
 */
STATIC_OVL boolean
check_windowdata(VOID_ARGS)
{
    if (WIN_STATUS == WIN_ERR || wins[WIN_STATUS] == (struct WinDesc *) 0) {
        paniclog("check_windowdata", " null status window.");
        return FALSE;
    } else if (!windowdata_init) {
        int i, n = wins[WIN_STATUS]->cols;

        for (i = 0; i < wins[WIN_STATUS]->cols; ++i) {
            wins[WIN_STATUS]->data[0][i] = ' ';
            wins[WIN_STATUS]->data[1][i] = ' ';
        }
        wins[WIN_STATUS]->data[0][n - 1] = '\0';    /* null terminate */
        wins[WIN_STATUS]->data[1][n - 1] = '\0';    /* null terminate */
        windowdata_init = TRUE;
    }
    return TRUE;
}

#ifdef TEXTCOLOR
/*
 * Return what color this condition should
 * be displayed in based on user settings.
 */
STATIC_OVL int
condcolor(bm, bmarray)
long bm;
unsigned long *bmarray;
{
    int i;

    if (bm && bmarray)
        for (i = 0; i < CLR_MAX; ++i) {
            if ((bm & bmarray[i]) != 0)
                return i;
        }
    return NO_COLOR;
}
#else
/* might need something more elaborate if some compiler complains that
   the condition where this gets used always has the same value */
#define condcolor(bm,bmarray) NO_COLOR
#define term_start_color(color) /*empty*/
#define term_end_color(color) /*empty*/
#endif /* TEXTCOLOR */

STATIC_OVL int
condattr(bm, bmarray)
long bm;
unsigned long *bmarray;
{
    int attr = 0;
    int i;

    if (bm && bmarray) {
        for (i = HL_ATTCLR_DIM; i < BL_ATTCLR_MAX; ++i) {
            if ((bm & bmarray[i]) != 0) {
                switch (i) {
                case HL_ATTCLR_DIM:
                    attr |= HL_DIM;
                    break;
                case HL_ATTCLR_BLINK:
                    attr |= HL_BLINK;
                    break;
                case HL_ATTCLR_ULINE:
                    attr |= HL_ULINE;
                    break;
                case HL_ATTCLR_INVERSE:
                    attr |= HL_INVERSE;
                    break;
                case HL_ATTCLR_BOLD:
                    attr |= HL_BOLD;
                    break;
                }
            }
        }
    }
    return attr;
}

#define Begin_Attr(m) \
    do {                                                                \
        if (m) {                                                        \
            if ((m) & HL_BOLD)                                          \
                term_start_attr(ATR_BOLD);                              \
            if ((m) & HL_INVERSE)                                       \
                term_start_attr(ATR_INVERSE);                           \
            if ((m) & HL_ULINE)                                         \
                term_start_attr(ATR_ULINE);                             \
            if ((m) & HL_BLINK)                                         \
                term_start_attr(ATR_BLINK);                             \
            if ((m) & HL_DIM)                                           \
                term_start_attr(ATR_DIM);                               \
        }                                                               \
    } while (0)

#define End_Attr(m) \
    do {                                                                \
        if (m) {                                                        \
            if ((m) & HL_DIM)                                           \
                term_end_attr(ATR_DIM);                                 \
            if ((m) & HL_BLINK)                                         \
                term_end_attr(ATR_BLINK);                               \
            if ((m) & HL_ULINE)                                         \
                term_end_attr(ATR_ULINE);                               \
            if ((m) & HL_INVERSE)                                       \
                term_end_attr(ATR_INVERSE);                             \
            if ((m) & HL_BOLD)                                          \
                term_end_attr(ATR_BOLD);                                \
        }                                                               \
    } while (0)

STATIC_OVL void
render_status(VOID_ARGS)
{
    long mask = 0L;
    int i, c, row, attrmask = 0;
    struct WinDesc *cw = 0;
    struct tty_status_fields *nullfield = (struct tty_status_fields *) 0;

    if (WIN_STATUS == WIN_ERR
        || (cw = wins[WIN_STATUS]) == (struct WinDesc *) 0) {
        paniclog("render_status", "WIN_ERR on status window.");
        return;
    }

    for (row = 0; row < 2; ++row) {
        curs(WIN_STATUS, 1, row);
        for (i = 0; fieldorder[row][i] != BL_FLUSH; ++i) {
            int idx = fieldorder[row][i];

            if (!status_activefields[idx])
                continue;

            if ((tty_status[NOW][idx].lth || idx == BL_CONDITION)
                 && (tty_status[NOW][idx].redraw || !do_field_opt)) {
                int coloridx = tty_status[NOW][idx].color;
                int attridx = tty_status[NOW][idx].attr;
                int x = tty_status[NOW][idx].x;
                int y = row;
                char *text = status_vals[idx];
                boolean hitpointbar = (idx == BL_TITLE
                                       && iflags.wc2_hitpointbar);

                if (idx == BL_CONDITION) {
                    /*
                     * +-----------------+
                     * | Condition Codes |
                     * +-----------------+
                     */
                    for (c = 0; c < SIZE(conditions); ++c) {
                        mask = conditions[c].mask;
                        if ((tty_condition_bits & mask) == mask) {
                            const char *condtext;

                            tty_putstatusfield(nullfield, " ", x++, y);
                            if (iflags.hilite_delta) {
                                attrmask = condattr(mask, tty_colormasks);
                                Begin_Attr(attrmask);
                                coloridx = condcolor(mask, tty_colormasks);
                                if (coloridx != NO_COLOR
                                    && coloridx != CLR_MAX)
                                    term_start_color(coloridx);
                            }
                            if (x >= cw->cols && !truncation_expected)
                                impossible(
                                   "Unexpected condition placement overflow");
                            condtext = conditions[c].text[cond_shrinklvl];
                            tty_putstatusfield(nullfield, condtext, x, y);
                            x += (int) strlen(condtext);
                            if (iflags.hilite_delta) {
                                if (coloridx != NO_COLOR
                                    && coloridx != CLR_MAX)
                                    term_end_color();
                                End_Attr(attrmask);
                            }
                        }
                    }
                    if (x >= cw->cols) {
                        static unsigned once_only = 0;

                        if (!truncation_expected && !once_only++)
                            paniclog(
                                "render_status()", " unexpected truncation.");
                        x = cw->cols - 1;
                    }
                    tty_curs(WIN_STATUS, x, y);
                    cl_end();
                } else if (idx == BL_GOLD) {
                    char buf[BUFSZ];
                    /*
                     * +-----------+
                     * |   Gold    |
                     * +-----------+
                     */
                    if (iflags.hilite_delta) {
                        if (*text == ' ') {
                            tty_putstatusfield(nullfield, " ", x++, y);
                            text++;
                        }
                        /* multiple attributes can be in effect concurrently */
                        Begin_Attr(attridx);
                        if (coloridx != NO_COLOR && coloridx != CLR_MAX)
                            term_start_color(coloridx);
                    }
                    /* decode_mixed() due to GOLD glyph */
                    tty_putstatusfield(nullfield,
                                       decode_mixed(buf, text), x, y);
                    if (iflags.hilite_delta) {
                        if (coloridx != NO_COLOR && coloridx != CLR_MAX)
                            term_end_color();
                        End_Attr(attridx);
                    }
                } else if (hitpointbar) {
                    /*
                     * +-------------------------+
                     * | Title with Hitpoint Bar |
                     * +-------------------------+
                     */
                    /* hitpointbar using hp percent calculation */
                    int bar_pos, bar_len;
                    char *bar2 = (char *)0;
                    char bar[MAXCO], savedch;
                    boolean twoparts = FALSE;

                    bar_len = strlen(text);
                    if (bar_len < MAXCO-1) {
                        Strcpy(bar, text);
                        bar_pos = (bar_len * hpbar_percent) / 100;
                        if (bar_pos < 1 && hpbar_percent > 0)
                            bar_pos = 1;
                        if (bar_pos >= bar_len && hpbar_percent < 100)
                            bar_pos = bar_len - 1;
                        if (bar_pos > 0 && bar_pos < bar_len) {
                            twoparts = TRUE;
                            bar2 = &bar[bar_pos];
                            savedch = *bar2;
                            *bar2 = '\0';
                        }
                    }
                    if (iflags.hilite_delta) {
                        char *s = bar;
                        tty_putstatusfield(nullfield, "[", x++, y);
                        if (hpbar_percent > 0) {
                            if (hpbar_color != NO_COLOR && coloridx != CLR_MAX)
                                term_start_color(hpbar_color);
                            term_start_attr(ATR_INVERSE);
                        }
                        if (hpbar_percent == 0)
                            s = text;
                        tty_putstatusfield(nullfield, s, x, y);
                        x += (int) strlen(s);
                        if (hpbar_percent > 0) {
                            term_end_attr(ATR_INVERSE);
                            if (hpbar_color != NO_COLOR && coloridx != CLR_MAX)
                                term_end_color();
                        }
                        if (twoparts && hpbar_percent > 0) {
                            *bar2 = savedch;
                            tty_putstatusfield(nullfield, bar2, x, y);
                            x += (int) strlen(bar2);
                            tty_curs(WIN_STATUS, x, y);
                        }
                        tty_putstatusfield(nullfield, "]", x++, y);
                    } else {
                        tty_putstatusfield(&tty_status[NOW][idx],
                                           (char *) 0, x, y);
                    }
                } else {
                    /*
                     * +-----------------------------+
                     * | Everything else that is not |
                     * |   in a special case above   |
                     * +-----------------------------+
                     */
                    if (iflags.hilite_delta) {
                        if (*text == ' ') {
                            tty_putstatusfield(nullfield, " ", x++, y);
                            text++;
                        }
                        /* multiple attributes can be in effect concurrently */
                        Begin_Attr(attridx);
                        if (coloridx != NO_COLOR && coloridx != CLR_MAX)
                            term_start_color(coloridx);
                    }
                    tty_putstatusfield(&tty_status[NOW][idx],
                                       text, x, y);                    
                    if (iflags.hilite_delta) {
                        if (coloridx != NO_COLOR && coloridx != CLR_MAX)
                            term_end_color();
                        End_Attr(attridx);
                    }
                }
                if (tty_status[NOW][idx].last_on_row) {
                    int padright = 0;

                    x = tty_status[NOW][idx].x + tty_status[NOW][idx].lth;
                    finalx[row][NOW] = x - 1;
                    if (finalx[row][NOW] < finalx[row][BEFORE])
                        padright = finalx[row][BEFORE] - finalx[row][NOW];
                    while (padright-- > 0)
                        tty_putstatusfield(nullfield, " ", x++, y);
                }
            }
            /* reset .redraw, .dirty, .padright now that they've been rendered */
            tty_status[NOW][idx].dirty  = FALSE;
            tty_status[NOW][idx].redraw = FALSE;
            tty_status[NOW][idx].last_on_row = FALSE;

            /*
             * For comparison of current and previous:
             * - Copy the entire tty_status struct.
             */
             tty_status[BEFORE][idx] = tty_status[NOW][idx];
        }
        /*
         * For comparison of current and previous:
         * - Copy the last written column number on the row.
         */
        finalx[row][BEFORE] = finalx[row][NOW];
    }
    if (cond_disp_width[NOW] < cond_width_at_shrink) {
        cond_shrinklvl = 0;      /* reset */
        cond_width_at_shrink = condition_size();
        shrink_enc(0);
        shrink_dlvl(0);
    }
    return;
}

#endif /* STATUS_HILITES */

#endif /* TTY_GRAPHICS */

/*wintty.c*/


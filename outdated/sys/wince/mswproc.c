/* NetHack 3.6	mswproc.c	$NHDT-Date: 1433806606 2015/06/08 23:36:46 $  $NHDT-Branch: master $:$NHDT-Revision: 1.60 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file implements the interface between the window port specific
 * code in the mswin port and the rest of the nethack game engine.
*/

#include "hack.h"
#include "dlb.h"
#include "winMS.h"
#include "mhmap.h"
#include "mhstatus.h"
#include "mhtext.h"
#include "mhmsgwnd.h"
#include "mhmenu.h"
#include "mhmsg.h"
#include "mhcmd.h"
#include "mhinput.h"
#include "mhaskyn.h"
#include "mhdlg.h"
#include "mhrip.h"
#include "mhmain.h"
#include "mhfont.h"
#include "mhcolor.h"

#define LLEN 128

#ifdef _DEBUG
extern void logDebug(const char *fmt, ...);
#else
void
logDebug(const char *fmt, ...)
{
}
#endif

static void mswin_main_loop();
static BOOL initMapTiles(void);
static void prompt_for_player_selection(void);

/* Interface definition, for windows.c */
struct window_procs mswin_procs = {
    "MSWIN",
    WC_COLOR | WC_HILITE_PET | WC_ALIGN_MESSAGE | WC_ALIGN_STATUS | WC_INVERSE
        | WC_SCROLL_MARGIN | WC_MAP_MODE | WC_FONT_MESSAGE | WC_FONT_STATUS
        | WC_FONT_MENU | WC_FONT_TEXT | WC_FONT_MAP | WC_FONTSIZ_MESSAGE
        | WC_FONTSIZ_STATUS | WC_FONTSIZ_MENU | WC_FONTSIZ_TEXT
        | WC_TILE_WIDTH | WC_TILE_HEIGHT | WC_TILE_FILE | WC_VARY_MSGCOUNT
        | WC_WINDOWCOLORS | WC_PLAYER_SELECTION,
    WC2_FULLSCREEN | WC2_SOFTKEYBOARD | WC2_WRAPTEXT, mswin_init_nhwindows,
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   /* color availability */
    mswin_player_selection, mswin_askname, mswin_get_nh_event,
    mswin_exit_nhwindows, mswin_suspend_nhwindows, mswin_resume_nhwindows,
    mswin_create_nhwindow, mswin_clear_nhwindow, mswin_display_nhwindow,
    mswin_destroy_nhwindow, mswin_curs, mswin_putstr, genl_putmixed,
    mswin_display_file, mswin_start_menu, mswin_add_menu, mswin_end_menu,
    mswin_select_menu,
    genl_message_menu, /* no need for X-specific handling */
    mswin_update_inventory, mswin_mark_synch, mswin_wait_synch,
#ifdef CLIPPING
    mswin_cliparound,
#endif
#ifdef POSITIONBAR
    donull,
#endif
    mswin_print_glyph, mswin_raw_print, mswin_raw_print_bold, mswin_nhgetch,
    mswin_nh_poskey, mswin_nhbell, mswin_doprev_message, mswin_yn_function,
    mswin_getlin, mswin_get_ext_cmd, mswin_number_pad, mswin_delay_output,
#ifdef CHANGE_COLOR /* only a Mac option currently */
    mswin, mswin_change_background,
#endif
    /* other defs that really should go away (they're tty specific) */
    mswin_start_screen, mswin_end_screen, mswin_outrip,
    mswin_preference_update, genl_getmsghistory, genl_putmsghistory,
    genl_status_init, genl_status_finish, genl_status_enablefield,
    genl_status_update,
    genl_can_suspend_no,
};

/*
init_nhwindows(int* argcp, char** argv)
                -- Initialize the windows used by NetHack.  This can also
                   create the standard windows listed at the top, but does
                   not display them.
                -- Any commandline arguments relevant to the windowport
                   should be interpreted, and *argcp and *argv should
                   be changed to remove those arguments.
                -- When the message window is created, the variable
                   iflags.window_inited needs to be set to TRUE.  Otherwise
                   all plines() will be done via raw_print().
                ** Why not have init_nhwindows() create all of the "standard"
                ** windows?  Or at least all but WIN_INFO?      -dean
*/
void
mswin_init_nhwindows(int *argc, char **argv)
{
    HWND hWnd;
    logDebug("mswin_init_nhwindows()\n");

#ifdef _DEBUG
    {
        /* truncate trace file */
        FILE *dfp = fopen("nhtrace.log", "w");
        fclose(dfp);
    }
#endif

    /* intialize input subsystem */
    mswin_nh_input_init();

    /* read registry settings */
    mswin_read_reg();

    /* set it to WIN_ERR so we can detect attempts to
       use this ID before it is inialized */
    WIN_MAP = WIN_ERR;

    /* check default values */
    if (iflags.wc_fontsiz_status < NHFONT_SIZE_MIN
        || iflags.wc_fontsiz_status > NHFONT_SIZE_MAX)
        iflags.wc_fontsiz_status = NHFONT_STATUS_DEFAULT_SIZE;

    if (iflags.wc_fontsiz_message < NHFONT_SIZE_MIN
        || iflags.wc_fontsiz_message > NHFONT_SIZE_MAX)
        iflags.wc_fontsiz_message = NHFONT_DEFAULT_SIZE;

    if (iflags.wc_fontsiz_text < NHFONT_SIZE_MIN
        || iflags.wc_fontsiz_text > NHFONT_SIZE_MAX)
        iflags.wc_fontsiz_text = NHFONT_DEFAULT_SIZE;

    if (iflags.wc_fontsiz_menu < NHFONT_SIZE_MIN
        || iflags.wc_fontsiz_menu > NHFONT_SIZE_MAX)
        iflags.wc_fontsiz_menu = NHFONT_DEFAULT_SIZE;

    if (iflags.wc_align_message == 0)
        iflags.wc_align_message = ALIGN_BOTTOM;
    if (iflags.wc_align_status == 0)
        iflags.wc_align_status = ALIGN_TOP;
    if (iflags.wc_scroll_margin == 0)
        iflags.wc_scroll_margin = DEF_CLIPAROUND_MARGIN;
    if (iflags.wc_tile_width == 0)
        iflags.wc_tile_width = TILE_X;
    if (iflags.wc_tile_height == 0)
        iflags.wc_tile_height = TILE_Y;

    if (iflags.wc_vary_msgcount == 0)
        iflags.wc_vary_msgcount = 3;

    /* force tabs in menus */
    iflags.menu_tab_sep = 1;

    /* force toptenwin to be true.  toptenwin is the option that decides
     * whether to
     * write output to a window or stdout.  stdout doesn't make sense on
     * Windows
     * non-console applications
     */
    iflags.toptenwin = 1;
    set_option_mod_status("toptenwin", set_in_config);

    /* initialize map tiles bitmap */
    initMapTiles();

    /* set tile-related options to readonly */
    set_wc_option_mod_status(WC_TILE_WIDTH | WC_TILE_HEIGHT | WC_TILE_FILE,
                             set_gameview);

    /* init color table */
    mswin_init_color_table();

    /* set font-related options to change in the game */
    set_wc_option_mod_status(
        WC_HILITE_PET | WC_ALIGN_MESSAGE | WC_ALIGN_STATUS | WC_SCROLL_MARGIN
            | WC_MAP_MODE | WC_FONT_MESSAGE | WC_FONT_STATUS | WC_FONT_MENU
            | WC_FONT_TEXT | WC_FONTSIZ_MESSAGE | WC_FONTSIZ_STATUS
            | WC_FONTSIZ_MENU | WC_FONTSIZ_TEXT | WC_VARY_MSGCOUNT,
        set_in_game);

    /* WC2 options */
    set_wc2_option_mod_status(WC2_FULLSCREEN | WC2_SOFTKEYBOARD, set_in_config);
    GetNHApp()->bFullScreen = iflags.wc2_fullscreen;
    GetNHApp()->bUseSIP = iflags.wc2_softkeyboard;

    set_wc2_option_mod_status(WC2_WRAPTEXT, set_in_game);
    GetNHApp()->bWrapText = iflags.wc2_wraptext;

    /* create the main nethack window */
    hWnd = mswin_init_main_window();
    if (!hWnd)
        panic("Cannot create the main window.");
    ShowWindow(hWnd, GetNHApp()->nCmdShow);
    UpdateWindow(hWnd);
    GetNHApp()->hMainWnd = hWnd;

    /* set Full screen if requested */
    mswin_set_fullscreen(GetNHApp()->bFullScreen);

    /* let nethack code know that the window subsystem is ready */
    iflags.window_inited = TRUE;
}

/* Do a window-port specific player type selection. If player_selection()
   offers a Quit option, it is its responsibility to clean up and terminate
   the process. You need to fill in pl_character[0].
*/
void
mswin_player_selection(void)
{
    logDebug("mswin_player_selection()\n");

#if defined(WIN_CE_SMARTPHONE)
    /* SmartPhone does not supprt combo-boxes therefor we cannot
       use dialog for player selection */
    prompt_for_player_selection();
#else
    if (iflags.wc_player_selection == VIA_DIALOG) {
        int nRole;

        /* pick player type randomly (use pre-selected
         * role/race/gender/alignment) */
        if (flags.randomall) {
            if (flags.initrole < 0) {
                flags.initrole = pick_role(flags.initrace, flags.initgend,
                                           flags.initalign, PICK_RANDOM);
                if (flags.initrole < 0) {
                    raw_print("Incompatible role!");
                    flags.initrole = randrole(FALSE);
                }
            }

            if (flags.initrace < 0
                || !validrace(flags.initrole, flags.initrace)) {
                flags.initrace = pick_race(flags.initrole, flags.initgend,
                                           flags.initalign, PICK_RANDOM);
                if (flags.initrace < 0) {
                    raw_print("Incompatible race!");
                    flags.initrace = randrace(flags.initrole);
                }
            }

            if (flags.initgend < 0
                || !validgend(flags.initrole, flags.initrace,
                              flags.initgend)) {
                flags.initgend = pick_gend(flags.initrole, flags.initrace,
                                           flags.initalign, PICK_RANDOM);
                if (flags.initgend < 0) {
                    raw_print("Incompatible gender!");
                    flags.initgend = randgend(flags.initrole, flags.initrace);
                }
            }

            if (flags.initalign < 0
                || !validalign(flags.initrole, flags.initrace,
                               flags.initalign)) {
                flags.initalign = pick_align(flags.initrole, flags.initrace,
                                             flags.initgend, PICK_RANDOM);
                if (flags.initalign < 0) {
                    raw_print("Incompatible alignment!");
                    flags.initalign =
                        randalign(flags.initrole, flags.initrace);
                }
            }
        } else {
            /* select a role */
            if (mswin_player_selection_window(&nRole) == IDCANCEL) {
                bail(0);
            }
        }
    } else { /* iflags.wc_player_selection == VIA_PROMPTS */
        prompt_for_player_selection();
    }
#endif /* defined(WIN_CE_SMARTPHONE) */
}

void
prompt_for_player_selection(void)
{
    int i, k, n;
    char pick4u = 'n', thisch, lastch = 0;
    char pbuf[QBUFSZ], plbuf[QBUFSZ];
    winid win;
    anything any;
    menu_item *selected = 0;
    int box_result;
    TCHAR wbuf[BUFSZ];

    logDebug("prompt_for_player_selection()\n");

    /* prevent an unnecessary prompt */
    rigid_role_checks();

    /* Should we randomly pick for the player? */
    if (!flags.randomall
        && (flags.initrole == ROLE_NONE || flags.initrace == ROLE_NONE
            || flags.initgend == ROLE_NONE || flags.initalign == ROLE_NONE)) {
        /* int echoline; */
        char *prompt = build_plselection_prompt(
            pbuf, QBUFSZ, flags.initrole, flags.initrace, flags.initgend,
            flags.initalign);

        /* tty_putstr(BASE_WINDOW, 0, ""); */
        /* echoline = wins[BASE_WINDOW]->cury; */
        box_result = MessageBox(NULL, NH_A2W(prompt, wbuf, BUFSZ),
                                TEXT("NetHack for Windows"),
#if defined(WIN_CE_SMARTPHONE)
                                MB_YESNO | MB_DEFBUTTON1
#else
                                MB_YESNOCANCEL | MB_DEFBUTTON1
#endif
                                );

        pick4u =
            (box_result == IDYES) ? 'y' : (box_result == IDNO) ? 'n' : '\033';
        /* tty_putstr(BASE_WINDOW, 0, prompt); */
        do {
            /* pick4u = lowc(readchar()); */
            if (strchr(quitchars, pick4u))
                pick4u = 'y';
        } while (!strchr(ynqchars, pick4u));
        if ((int) strlen(prompt) + 1 < CO) {
            /* Echo choice and move back down line */
            /* tty_putsym(BASE_WINDOW, (int)strlen(prompt)+1, echoline,
             * pick4u); */
            /* tty_putstr(BASE_WINDOW, 0, ""); */
        } else
            /* Otherwise it's hard to tell where to echo, and things are
             * wrapping a bit messily anyway, so (try to) make sure the next
             * question shows up well and doesn't get wrapped at the
             * bottom of the window.
             */
            /* tty_clear_nhwindow(BASE_WINDOW) */;

        if (pick4u != 'y' && pick4u != 'n') {
        give_up: /* Quit */
            if (selected)
                free((genericptr_t) selected);
            bail((char *) 0);
            /*NOTREACHED*/
            return;
        }
    }

    (void) root_plselection_prompt(plbuf, QBUFSZ - 1, flags.initrole,
                                   flags.initrace, flags.initgend,
                                   flags.initalign);

    /* Select a role, if necessary */
    /* we'll try to be compatible with pre-selected race/gender/alignment,
     * but may not succeed */
    if (flags.initrole < 0) {
        char rolenamebuf[QBUFSZ];
        /* Process the choice */
        if (pick4u == 'y' || flags.initrole == ROLE_RANDOM
            || flags.randomall) {
            /* Pick a random role */
            flags.initrole = pick_role(flags.initrace, flags.initgend,
                                       flags.initalign, PICK_RANDOM);
            if (flags.initrole < 0) {
                /* tty_putstr(BASE_WINDOW, 0, "Incompatible role!"); */
                flags.initrole = randrole(FALSE);
            }
        } else {
            /* tty_clear_nhwindow(BASE_WINDOW); */
            /* tty_putstr(BASE_WINDOW, 0, "Choosing Character's Role"); */
            /* Prompt for a role */
            win = create_nhwindow(NHW_MENU);
            start_menu(win, MENU_BEHAVE_STANDARD);
            any.a_void = 0; /* zero out all bits */
            for (i = 0; roles[i].name.m; i++) {
                if (ok_role(i, flags.initrace, flags.initgend,
                            flags.initalign)) {
                    any.a_int = i + 1; /* must be non-zero */
                    thisch = lowc(roles[i].name.m[0]);
                    if (thisch == lastch)
                        thisch = highc(thisch);
                    if (flags.initgend != ROLE_NONE
                        && flags.initgend != ROLE_RANDOM) {
                        if (flags.initgend == 1 && roles[i].name.f)
                            Strcpy(rolenamebuf, roles[i].name.f);
                        else
                            Strcpy(rolenamebuf, roles[i].name.m);
                    } else {
                        if (roles[i].name.f) {
                            Strcpy(rolenamebuf, roles[i].name.m);
                            Strcat(rolenamebuf, "/");
                            Strcat(rolenamebuf, roles[i].name.f);
                        } else
                            Strcpy(rolenamebuf, roles[i].name.m);
                    }
                    add_menu(win, NO_GLYPH, &any, thisch, 0, ATR_NONE,
                             an(rolenamebuf), MENU_ITEMFLAGS_NONE);
                    lastch = thisch;
                }
            }
            any.a_int = pick_role(flags.initrace, flags.initgend,
                                  flags.initalign, PICK_RANDOM) + 1;
            if (any.a_int == 0) /* must be non-zero */
                any.a_int = randrole(FALSE) + 1;
            add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random",
                     MENU_ITEMFLAGS_NONE);
            any.a_int = i + 1; /* must be non-zero */
            add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit",
                     MENU_ITEMFLAGS_NONE);
            Sprintf(pbuf, "Pick a role for your %s", plbuf);
            end_menu(win, pbuf);
            n = select_menu(win, PICK_ONE, &selected);
            destroy_nhwindow(win);

            /* Process the choice */
            if (n != 1 || selected[0].item.a_int == any.a_int)
                goto give_up; /* Selected quit */

            flags.initrole = selected[0].item.a_int - 1;
            free((genericptr_t) selected), selected = 0;
        }
        (void) root_plselection_prompt(plbuf, QBUFSZ - 1, flags.initrole,
                                       flags.initrace, flags.initgend,
                                       flags.initalign);
    }

    /* Select a race, if necessary */
    /* force compatibility with role, try for compatibility with
     * pre-selected gender/alignment */
    if (flags.initrace < 0 || !validrace(flags.initrole, flags.initrace)) {
        /* pre-selected race not valid */
        if (pick4u == 'y' || flags.initrace == ROLE_RANDOM
            || flags.randomall) {
            flags.initrace = pick_race(flags.initrole, flags.initgend,
                                       flags.initalign, PICK_RANDOM);
            if (flags.initrace < 0) {
                /* tty_putstr(BASE_WINDOW, 0, "Incompatible race!"); */
                flags.initrace = randrace(flags.initrole);
            }
        } else { /* pick4u == 'n' */
            /* Count the number of valid races */
            n = 0; /* number valid */
            k = 0; /* valid race */
            for (i = 0; races[i].noun; i++) {
                if (ok_race(flags.initrole, i, flags.initgend,
                            flags.initalign)) {
                    n++;
                    k = i;
                }
            }
            if (n == 0) {
                for (i = 0; races[i].noun; i++) {
                    if (validrace(flags.initrole, i)) {
                        n++;
                        k = i;
                    }
                }
            }

            /* Permit the user to pick, if there is more than one */
            if (n > 1) {
                /* tty_clear_nhwindow(BASE_WINDOW); */
                /* tty_putstr(BASE_WINDOW, 0, "Choosing Race"); */
                win = create_nhwindow(NHW_MENU);
                start_menu(win, MENU_BEHAVE_STANDARD);
                any.a_void = 0; /* zero out all bits */
                for (i = 0; races[i].noun; i++)
                    if (ok_race(flags.initrole, i, flags.initgend,
                                flags.initalign)) {
                        any.a_int = i + 1; /* must be non-zero */
                        add_menu(win, NO_GLYPH, &any, races[i].noun[0], 0,
                                 ATR_NONE, races[i].noun, MENU_ITEMFLAGS_NONE);
                    }
                any.a_int = pick_race(flags.initrole, flags.initgend,
                                      flags.initalign, PICK_RANDOM) + 1;
                if (any.a_int == 0) /* must be non-zero */
                    any.a_int = randrace(flags.initrole) + 1;
                add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random",
                         MENU_ITEMFLAGS_NONE);
                any.a_int = i + 1; /* must be non-zero */
                add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit",
                         MENU_ITEMFLAGS_NONE);
                Sprintf(pbuf, "Pick the race of your %s", plbuf);
                end_menu(win, pbuf);
                n = select_menu(win, PICK_ONE, &selected);
                destroy_nhwindow(win);
                if (n != 1 || selected[0].item.a_int == any.a_int)
                    goto give_up; /* Selected quit */

                k = selected[0].item.a_int - 1;
                free((genericptr_t) selected), selected = 0;
            }
            flags.initrace = k;
        }
        (void) root_plselection_prompt(plbuf, QBUFSZ - 1, flags.initrole,
                                       flags.initrace, flags.initgend,
                                       flags.initalign);
    }

    /* Select a gender, if necessary */
    /* force compatibility with role/race, try for compatibility with
     * pre-selected alignment */
    if (flags.initgend < 0
        || !validgend(flags.initrole, flags.initrace, flags.initgend)) {
        /* pre-selected gender not valid */
        if (pick4u == 'y' || flags.initgend == ROLE_RANDOM
            || flags.randomall) {
            flags.initgend = pick_gend(flags.initrole, flags.initrace,
                                       flags.initalign, PICK_RANDOM);
            if (flags.initgend < 0) {
                /* tty_putstr(BASE_WINDOW, 0, "Incompatible gender!"); */
                flags.initgend = randgend(flags.initrole, flags.initrace);
            }
        } else { /* pick4u == 'n' */
            /* Count the number of valid genders */
            n = 0; /* number valid */
            k = 0; /* valid gender */
            for (i = 0; i < ROLE_GENDERS; i++) {
                if (ok_gend(flags.initrole, flags.initrace, i,
                            flags.initalign)) {
                    n++;
                    k = i;
                }
            }
            if (n == 0) {
                for (i = 0; i < ROLE_GENDERS; i++) {
                    if (validgend(flags.initrole, flags.initrace, i)) {
                        n++;
                        k = i;
                    }
                }
            }

            /* Permit the user to pick, if there is more than one */
            if (n > 1) {
                /* tty_clear_nhwindow(BASE_WINDOW); */
                /* tty_putstr(BASE_WINDOW, 0, "Choosing Gender"); */
                win = create_nhwindow(NHW_MENU);
                start_menu(win, MENU_BEHAVE_STANDARD);
                any.a_void = 0; /* zero out all bits */
                for (i = 0; i < ROLE_GENDERS; i++)
                    if (ok_gend(flags.initrole, flags.initrace, i,
                                flags.initalign)) {
                        any.a_int = i + 1;
                        add_menu(win, NO_GLYPH, &any, genders[i].adj[0], 0,
                                 ATR_NONE, genders[i].adj, MENU_ITEMFLAGS_NONE);
                    }
                any.a_int = pick_gend(flags.initrole, flags.initrace,
                                      flags.initalign, PICK_RANDOM) + 1;
                if (any.a_int == 0) /* must be non-zero */
                    any.a_int = randgend(flags.initrole, flags.initrace) + 1;
                add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random",
                         MENU_ITEMFLAGS_NONE);
                any.a_int = i + 1; /* must be non-zero */
                add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit",
                         MENU_ITEMFLAGS_NONE);
                Sprintf(pbuf, "Pick the gender of your %s", plbuf);
                end_menu(win, pbuf);
                n = select_menu(win, PICK_ONE, &selected);
                destroy_nhwindow(win);
                if (n != 1 || selected[0].item.a_int == any.a_int)
                    goto give_up; /* Selected quit */

                k = selected[0].item.a_int - 1;
                free((genericptr_t) selected), selected = 0;
            }
            flags.initgend = k;
        }
        (void) root_plselection_prompt(plbuf, QBUFSZ - 1, flags.initrole,
                                       flags.initrace, flags.initgend,
                                       flags.initalign);
    }

    /* Select an alignment, if necessary */
    /* force compatibility with role/race/gender */
    if (flags.initalign < 0
        || !validalign(flags.initrole, flags.initrace, flags.initalign)) {
        /* pre-selected alignment not valid */
        if (pick4u == 'y' || flags.initalign == ROLE_RANDOM
            || flags.randomall) {
            flags.initalign = pick_align(flags.initrole, flags.initrace,
                                         flags.initgend, PICK_RANDOM);
            if (flags.initalign < 0) {
                /* tty_putstr(BASE_WINDOW, 0, "Incompatible alignment!"); */
                flags.initalign = randalign(flags.initrole, flags.initrace);
            }
        } else { /* pick4u == 'n' */
            /* Count the number of valid alignments */
            n = 0; /* number valid */
            k = 0; /* valid alignment */
            for (i = 0; i < ROLE_ALIGNS; i++) {
                if (ok_align(flags.initrole, flags.initrace, flags.initgend,
                             i)) {
                    n++;
                    k = i;
                }
            }
            if (n == 0) {
                for (i = 0; i < ROLE_ALIGNS; i++) {
                    if (validalign(flags.initrole, flags.initrace, i)) {
                        n++;
                        k = i;
                    }
                }
            }

            /* Permit the user to pick, if there is more than one */
            if (n > 1) {
                /* tty_clear_nhwindow(BASE_WINDOW); */
                /* tty_putstr(BASE_WINDOW, 0, "Choosing Alignment"); */
                win = create_nhwindow(NHW_MENU);
                start_menu(win, MENU_BEHAVE_STANDARD);
                any.a_void = 0; /* zero out all bits */
                for (i = 0; i < ROLE_ALIGNS; i++)
                    if (ok_align(flags.initrole, flags.initrace,
                                 flags.initgend, i)) {
                        any.a_int = i + 1;
                        add_menu(win, NO_GLYPH, &any, aligns[i].adj[0], 0,
                                 ATR_NONE, aligns[i].adj, MENU_ITEMFLAGS_NONE);
                    }
                any.a_int = pick_align(flags.initrole, flags.initrace,
                                       flags.initgend, PICK_RANDOM) + 1;
                if (any.a_int == 0) /* must be non-zero */
                    any.a_int = randalign(flags.initrole, flags.initrace) + 1;
                add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random",
                         MENU_ITEMFLAGS_NONE);
                any.a_int = i + 1; /* must be non-zero */
                add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit",
                         MENU_ITEMFLAGS_NONE);
                Sprintf(pbuf, "Pick the alignment of your %s", plbuf);
                end_menu(win, pbuf);
                n = select_menu(win, PICK_ONE, &selected);
                destroy_nhwindow(win);
                if (n != 1 || selected[0].item.a_int == any.a_int)
                    goto give_up; /* Selected quit */

                k = selected[0].item.a_int - 1;
                free((genericptr_t) selected), selected = 0;
            }
            flags.initalign = k;
        }
    }
    /* Success! */
    /* tty_display_nhwindow(BASE_WINDOW, FALSE); */
}

/* Ask the user for a player name. */
void
mswin_askname(void)
{
    logDebug("mswin_askname()\n");

    if (mswin_getlin_window("who are you?", g.plname, PL_NSIZ) == IDCANCEL) {
        bail("bye-bye");
        /* not reached */
    }
}

/* Does window event processing (e.g. exposure events).
   A noop for the tty and X window-ports.
*/
void
mswin_get_nh_event(void)
{
    logDebug("mswin_get_nh_event()\n");
    return;
}

/* Exits the window system.  This should dismiss all windows,
   except the "window" used for raw_print().  str is printed if possible.
*/
void
mswin_exit_nhwindows(const char *str)
{
    logDebug("mswin_exit_nhwindows(%s)\n", str);

    /* Write Window settings to the registry */
    mswin_write_reg();

    // Don't do any of this (?) - exit_nhwindows does not terminate
    // the application
    // DestroyWindow(GetNHApp()->hMainWnd);
    // nh_terminate(EXIT_SUCCESS);
}

/* Prepare the window to be suspended. */
void
mswin_suspend_nhwindows(const char *str)
{
    logDebug("mswin_suspend_nhwindows(%s)\n", str);
    return;
}

/* Restore the windows after being suspended. */
void
mswin_resume_nhwindows()
{
    logDebug("mswin_resume_nhwindows()\n");
    return;
}

/*  Create a window of type "type" which can be
        NHW_MESSAGE     (top line)
        NHW_STATUS      (bottom lines)
        NHW_MAP         (main dungeon)
        NHW_MENU        (inventory or other "corner" windows)
        NHW_TEXT        (help/text, full screen paged window)
*/
winid
mswin_create_nhwindow(int type)
{
    winid i = 0;
    MSNHMsgAddWnd data;

    logDebug("mswin_create_nhwindow(%d)\n", type);

    /* Return the next available winid
     */

    for (i = 1; i < MAXWINDOWS; i++)
        if (GetNHApp()->windowlist[i].win == NULL
            && !GetNHApp()->windowlist[i].dead)
            break;
    if (i == MAXWINDOWS)
        panic("ERROR:  No windows available...\n");

    switch (type) {
    case NHW_MAP: {
        GetNHApp()->windowlist[i].win = mswin_init_map_window();
        GetNHApp()->windowlist[i].type = type;
        GetNHApp()->windowlist[i].dead = 0;
        break;
    }
    case NHW_MESSAGE: {
        GetNHApp()->windowlist[i].win = mswin_init_message_window();
        GetNHApp()->windowlist[i].type = type;
        GetNHApp()->windowlist[i].dead = 0;
        break;
    }
    case NHW_STATUS: {
        GetNHApp()->windowlist[i].win = mswin_init_status_window();
        GetNHApp()->windowlist[i].type = type;
        GetNHApp()->windowlist[i].dead = 0;
        break;
    }
    case NHW_MENU: {
        GetNHApp()->windowlist[i].win = NULL; // will create later
        GetNHApp()->windowlist[i].type = type;
        GetNHApp()->windowlist[i].dead = 1;
        break;
    }
    case NHW_TEXT: {
        GetNHApp()->windowlist[i].win = mswin_init_text_window();
        GetNHApp()->windowlist[i].type = type;
        GetNHApp()->windowlist[i].dead = 0;
        break;
    }
    }

    ZeroMemory(&data, sizeof(data));
    data.wid = i;
    SendMessage(GetNHApp()->hMainWnd, WM_MSNH_COMMAND,
                (WPARAM) MSNH_MSG_ADDWND, (LPARAM) &data);
    return i;
}

/* Clear the given window, when asked to. */
void
mswin_clear_nhwindow(winid wid)
{
    logDebug("mswin_clear_nhwindow(%d)\n", wid);

    if ((wid >= 0) && (wid < MAXWINDOWS)
        && (GetNHApp()->windowlist[wid].win != NULL)) {
        if (GetNHApp()->windowlist[wid].type == NHW_MAP) {
            if (Is_rogue_level(&u.uz))
                mswin_map_mode(mswin_hwnd_from_winid(WIN_MAP),
                               ROGUE_LEVEL_MAP_MODE);
            else
                mswin_map_mode(mswin_hwnd_from_winid(WIN_MAP),
                               iflags.wc_map_mode);
        }

        SendMessage(GetNHApp()->windowlist[wid].win, WM_MSNH_COMMAND,
                    (WPARAM) MSNH_MSG_CLEAR_WINDOW, (LPARAM) NULL);
    }
}

/* -- Display the window on the screen.  If there is data
                   pending for output in that window, it should be sent.
                   If blocking is TRUE, display_nhwindow() will not
                   return until the data has been displayed on the screen,
                   and acknowledged by the user where appropriate.
                -- All calls are blocking in the tty window-port.
                -- Calling display_nhwindow(WIN_MESSAGE,???) will do a
                   --more--, if necessary, in the tty window-port.
*/
void
mswin_display_nhwindow(winid wid, BOOLEAN_P block)
{
    logDebug("mswin_display_nhwindow(%d, %d)\n", wid, block);
    if (GetNHApp()->windowlist[wid].win != NULL) {
        if (GetNHApp()->windowlist[wid].type == NHW_MENU) {
            MENU_ITEM_P *p;
            mswin_menu_window_select_menu(GetNHApp()->windowlist[wid].win,
                                          PICK_NONE, &p);
        }
        if (GetNHApp()->windowlist[wid].type == NHW_TEXT) {
            mswin_display_text_window(GetNHApp()->windowlist[wid].win);
        }
        if (GetNHApp()->windowlist[wid].type == NHW_RIP) {
            mswin_display_RIP_window(GetNHApp()->windowlist[wid].win);
        } else {
            if (!block) {
                UpdateWindow(GetNHApp()->windowlist[wid].win);
            } else {
                if (GetNHApp()->windowlist[wid].type == NHW_MAP) {
                    (void) mswin_nhgetch();
                }
            }
        }
        SetFocus(GetNHApp()->hMainWnd);
    }
}

HWND
mswin_hwnd_from_winid(winid wid)
{
    if (wid >= 0 && wid < MAXWINDOWS) {
        return GetNHApp()->windowlist[wid].win;
    } else {
        return NULL;
    }
}

winid
mswin_winid_from_handle(HWND hWnd)
{
    winid i = 0;

    for (i = 1; i < MAXWINDOWS; i++)
        if (GetNHApp()->windowlist[i].win == hWnd)
            return i;
    return -1;
}

winid
mswin_winid_from_type(int type)
{
    winid i = 0;

    for (i = 1; i < MAXWINDOWS; i++)
        if (GetNHApp()->windowlist[i].type == type)
            return i;
    return -1;
}

void
mswin_window_mark_dead(winid wid)
{
    if (wid >= 0 && wid < MAXWINDOWS) {
        GetNHApp()->windowlist[wid].win = NULL;
        GetNHApp()->windowlist[wid].dead = 1;
    }
}

/* Destroy will dismiss the window if the window has not
 * already been dismissed.
*/
void
mswin_destroy_nhwindow(winid wid)
{
    logDebug("mswin_destroy_nhwindow(%d)\n", wid);

    if ((GetNHApp()->windowlist[wid].type == NHW_MAP)
        || (GetNHApp()->windowlist[wid].type == NHW_MESSAGE)
        || (GetNHApp()->windowlist[wid].type == NHW_STATUS)) {
        /* main windows is going to take care of those */
        return;
    }

    if (wid != -1) {
        if (!GetNHApp()->windowlist[wid].dead
            && GetNHApp()->windowlist[wid].win != NULL)
            DestroyWindow(GetNHApp()->windowlist[wid].win);
        GetNHApp()->windowlist[wid].win = NULL;
        GetNHApp()->windowlist[wid].type = 0;
        GetNHApp()->windowlist[wid].dead = 0;
    }
}

/* Next output to window will start at (x,y), also moves
 displayable cursor to (x,y).  For backward compatibility,
 1 <= x < cols, 0 <= y < rows, where cols and rows are
 the size of window.
*/
void
mswin_curs(winid wid, int x, int y)
{
    logDebug("mswin_curs(%d, %d, %d)\n", wid, x, y);

    if ((wid >= 0) && (wid < MAXWINDOWS)
        && (GetNHApp()->windowlist[wid].win != NULL)) {
        MSNHMsgCursor data;
        data.x = x;
        data.y = y;
        SendMessage(GetNHApp()->windowlist[wid].win, WM_MSNH_COMMAND,
                    (WPARAM) MSNH_MSG_CURSOR, (LPARAM) &data);
    }
}

/*
putstr(window, attr, str)
                -- Print str on the window with the given attribute.  Only
                   printable ASCII characters (040-0126) must be supported.
                   Multiple putstr()s are output on separate lines.
Attributes
                   can be one of
                        ATR_NONE (or 0)
                        ATR_ULINE
                        ATR_BOLD
                        ATR_BLINK
                        ATR_INVERSE
                   If a window-port does not support all of these, it may map
                   unsupported attributes to a supported one (e.g. map them
                   all to ATR_INVERSE).  putstr() may compress spaces out of
                   str, break str, or truncate str, if necessary for the
                   display.  Where putstr() breaks a line, it has to clear
                   to end-of-line.
                -- putstr should be implemented such that if two putstr()s
                   are done consecutively the user will see the first and
                   then the second.  In the tty port, pline() achieves this
                   by calling more() or displaying both on the same line.
*/
void
mswin_putstr(winid wid, int attr, const char *text)
{
    logDebug("mswin_putstr(%d, %d, %s)\n", wid, attr, text);

    mswin_putstr_ex(wid, attr, text, 0);
}

void
mswin_putstr_ex(winid wid, int attr, const char *text, boolean app)
{
    if ((wid >= 0) && (wid < MAXWINDOWS)) {
        if (GetNHApp()->windowlist[wid].win == NULL
            && GetNHApp()->windowlist[wid].type == NHW_MENU) {
            GetNHApp()->windowlist[wid].win =
                mswin_init_menu_window(MENU_TYPE_TEXT);
            GetNHApp()->windowlist[wid].dead = 0;
        }

        if (GetNHApp()->windowlist[wid].win != NULL) {
            MSNHMsgPutstr data;
            ZeroMemory(&data, sizeof(data));
            data.attr = attr;
            data.text = text;
            data.append = app;
            SendMessage(GetNHApp()->windowlist[wid].win, WM_MSNH_COMMAND,
                        (WPARAM) MSNH_MSG_PUTSTR, (LPARAM) &data);
        }
    }
}

/* Display the file named str.  Complain about missing files
                   iff complain is TRUE.
*/
void
mswin_display_file(const char *filename, BOOLEAN_P must_exist)
{
    dlb *f;
    TCHAR wbuf[BUFSZ];

    logDebug("mswin_display_file(%s, %d)\n", filename, must_exist);

    f = dlb_fopen(filename, RDTMODE);
    if (!f) {
        if (must_exist) {
            TCHAR message[90];
            _stprintf(message, TEXT("Warning! Could not find file: %s\n"),
                      NH_A2W(filename, wbuf, sizeof(wbuf)));
            MessageBox(GetNHApp()->hMainWnd, message, TEXT("ERROR"),
                       MB_OK | MB_ICONERROR);
        }
    } else {
        winid text;
        char line[LLEN];

        text = mswin_create_nhwindow(NHW_TEXT);

        while (dlb_fgets(line, LLEN, f)) {
            size_t len;
            len = strlen(line);
            if (line[len - 1] == '\n')
                line[len - 1] = '\x0';
            mswin_putstr(text, ATR_NONE, line);
        }
        (void) dlb_fclose(f);

        mswin_display_nhwindow(text, 1);
        mswin_destroy_nhwindow(text);
    }
}

/* Start using window as a menu.  You must call start_menu()
   before add_menu().  After calling start_menu() you may not
   putstr() to the window.  Only windows of type NHW_MENU may
   be used for menus.
*/
void
mswin_start_menu(winid wid, unsigned long mbehavior)
{
    logDebug("mswin_start_menu(%d, %lu)\n", wid, mbehavior);
    if ((wid >= 0) && (wid < MAXWINDOWS)) {
        if (GetNHApp()->windowlist[wid].win == NULL
            && GetNHApp()->windowlist[wid].type == NHW_MENU) {
            GetNHApp()->windowlist[wid].win =
                mswin_init_menu_window(MENU_TYPE_MENU);
            GetNHApp()->windowlist[wid].dead = 0;
        }

        if (GetNHApp()->windowlist[wid].win != NULL) {
            SendMessage(GetNHApp()->windowlist[wid].win, WM_MSNH_COMMAND,
                        (WPARAM) MSNH_MSG_STARTMENU, (LPARAM) NULL);
        }
    }
}

/*
add_menu(windid window, int glyph, const anything identifier,
                                char accelerator, char groupacc,
                                int attr, char *str, unsigned int itemflags)
                -- Add a text line str to the given menu window.  If
identifier
                   is 0, then the line cannot be selected (e.g. a title).
                   Otherwise, identifier is the value returned if the line is
                   selected.  Accelerator is a keyboard key that can be used
                   to select the line.  If the accelerator of a selectable
                   item is 0, the window system is free to select its own
                   accelerator.  It is up to the window-port to make the
                   accelerator visible to the user (e.g. put "a - " in front
                   of str).  The value attr is the same as in putstr().
                   Glyph is an optional glyph to accompany the line.  If
                   window port cannot or does not want to display it, this
                   is OK.  If there is no glyph applicable, then this
                   value will be NO_GLYPH.
                -- All accelerators should be in the range [A-Za-z].
                -- It is expected that callers do not mix accelerator
                   choices.  Either all selectable items have an accelerator
                   or let the window system pick them.  Don't do both.
                -- Groupacc is a group accelerator.  It may be any character
                   outside of the standard accelerator (see above) or a
                   number.  If 0, the item is unaffected by any group
                   accelerator.  If this accelerator conflicts with
                   the menu command (or their user defined aliases), it loses.
                   The menu commands and aliases take care not to interfere
                   with the default object class symbols.
                -- If you want this choice to be preselected when the
                   menu is displayed, set preselected to TRUE.
*/
void
mswin_add_menu(winid wid, int glyph, const ANY_P *identifier,
               CHAR_P accelerator, CHAR_P group_accel, int attr,
               const char *str, unsigned int itemflags)
{
    boolean presel = ((itemflags & MENU_ITEMFLAGS_SELECTED) != 0);

    logDebug("mswin_add_menu(%d, %d, %p, %c, %c, %d, %s, %u)\n", wid, glyph,
             identifier, (char) accelerator, (char) group_accel, attr, str,
             itemflags);
    if ((wid >= 0) && (wid < MAXWINDOWS)
        && (GetNHApp()->windowlist[wid].win != NULL)) {
        MSNHMsgAddMenu data;
        ZeroMemory(&data, sizeof(data));
        data.glyph = glyph;
        data.identifier = identifier;
        data.accelerator = accelerator;
        data.group_accel = group_accel;
        data.attr = attr;
        data.str = str;
        data.presel = presel;

        SendMessage(GetNHApp()->windowlist[wid].win, WM_MSNH_COMMAND,
                    (WPARAM) MSNH_MSG_ADDMENU, (LPARAM) &data);
    }
}

/*
end_menu(window, prompt)
                -- Stop adding entries to the menu and flushes the window
                   to the screen (brings to front?).  Prompt is a prompt
                   to give the user.  If prompt is NULL, no prompt will
                   be printed.
                ** This probably shouldn't flush the window any more (if
                ** it ever did).  That should be select_menu's job.  -dean
*/
void
mswin_end_menu(winid wid, const char *prompt)
{
    logDebug("mswin_end_menu(%d, %s)\n", wid, prompt);
    if ((wid >= 0) && (wid < MAXWINDOWS)
        && (GetNHApp()->windowlist[wid].win != NULL)) {
        MSNHMsgEndMenu data;
        ZeroMemory(&data, sizeof(data));
        data.text = prompt;

        SendMessage(GetNHApp()->windowlist[wid].win, WM_MSNH_COMMAND,
                    (WPARAM) MSNH_MSG_ENDMENU, (LPARAM) &data);
    }
}

/*
int select_menu(windid window, int how, menu_item **selected)
                -- Return the number of items selected; 0 if none were chosen,
                   -1 when explicitly cancelled.  If items were selected, then
                   selected is filled in with an allocated array of menu_item
                   structures, one for each selected line.  The caller must
                   free this array when done with it.  The "count" field
                   of selected is a user supplied count.  If the user did
                   not supply a count, then the count field is filled with
                   -1 (meaning all).  A count of zero is equivalent to not
                   being selected and should not be in the list.  If no items
                   were selected, then selected is NULL'ed out.  How is the
                   mode of the menu.  Three valid values are PICK_NONE,
                   PICK_ONE, and PICK_N, meaning: nothing is selectable,
                   only one thing is selectable, and any number valid items
                   may selected.  If how is PICK_NONE, this function should
                   never return anything but 0 or -1.
                -- You may call select_menu() on a window multiple times --
                   the menu is saved until start_menu() or destroy_nhwindow()
                   is called on the window.
                -- Note that NHW_MENU windows need not have select_menu()
                   called for them. There is no way of knowing whether
                   select_menu() will be called for the window at
                   create_nhwindow() time.
*/
int
mswin_select_menu(winid wid, int how, MENU_ITEM_P **selected)
{
    int nReturned = -1;

    logDebug("mswin_select_menu(%d, %d)\n", wid, how);

    if ((wid >= 0) && (wid < MAXWINDOWS)
        && (GetNHApp()->windowlist[wid].win != NULL)) {
        nReturned = mswin_menu_window_select_menu(
            GetNHApp()->windowlist[wid].win, how, selected);
    }
    return nReturned;
}

/*
    -- Indicate to the window port that the inventory has been changed.
    -- Merely calls display_inventory() for window-ports that leave the
        window up, otherwise empty.
*/
void
mswin_update_inventory()
{
    logDebug("mswin_update_inventory()\n");
}

/*
mark_synch()    -- Don't go beyond this point in I/O on any channel until
                   all channels are caught up to here.  Can be an empty call
                   for the moment
*/
void
mswin_mark_synch()
{
    logDebug("mswin_mark_synch()\n");
}

/*
wait_synch()    -- Wait until all pending output is complete (*flush*() for
                   streams goes here).
                -- May also deal with exposure events etc. so that the
                   display is OK when return from wait_synch().
*/
void
mswin_wait_synch()
{
    logDebug("mswin_wait_synch()\n");
}

/*
cliparound(x, y)-- Make sure that the user is more-or-less centered on the
                   screen if the playing area is larger than the screen.
                -- This function is only defined if CLIPPING is defined.
*/
void
mswin_cliparound(int x, int y)
{
    winid wid = WIN_MAP;

    logDebug("mswin_cliparound(%d, %d)\n", x, y);

    if ((wid >= 0) && (wid < MAXWINDOWS)
        && (GetNHApp()->windowlist[wid].win != NULL)) {
        MSNHMsgClipAround data;
        data.x = x;
        data.y = y;
        SendMessage(GetNHApp()->windowlist[wid].win, WM_MSNH_COMMAND,
                    (WPARAM) MSNH_MSG_CLIPAROUND, (LPARAM) &data);
    }
}

/*
print_glyph(window, x, y, glyph, bkglyph)
                -- Print the glyph at (x,y) on the given window.  Glyphs are
                   integers at the interface, mapped to whatever the window-
                   port wants (symbol, font, color, attributes, ...there's
                   a 1-1 map between glyphs and distinct things on the map).
*/
void
mswin_print_glyph(winid wid, XCHAR_P x, XCHAR_P y, int glyph, int bkglyph)
{
    logDebug("mswin_print_glyph(%d, %d, %d, %d, %d)\n", wid, x, y, glyph, bkglyph);

    if ((wid >= 0) && (wid < MAXWINDOWS)
        && (GetNHApp()->windowlist[wid].win != NULL)) {
        MSNHMsgPrintGlyph data;

        ZeroMemory(&data, sizeof(data));
        data.x = x;
        data.y = y;
        data.glyph = glyph;
        SendMessage(GetNHApp()->windowlist[wid].win, WM_MSNH_COMMAND,
                    (WPARAM) MSNH_MSG_PRINT_GLYPH, (LPARAM) &data);
    }
}

/*
raw_print(str)  -- Print directly to a screen, or otherwise guarantee that
                   the user sees str.  raw_print() appends a newline to str.
                   It need not recognize ASCII control characters.  This is
                   used during startup (before windowing system initialization
                   -- maybe this means only error startup messages are raw),
                   for error messages, and maybe other "msg" uses.  E.g.
                   updating status for micros (i.e, "saving").
*/
void
mswin_raw_print(const char *str)
{
    TCHAR wbuf[255];
    logDebug("mswin_raw_print(%s)\n", str);
    if (str && *str)
        MessageBox(GetNHApp()->hMainWnd, NH_A2W(str, wbuf, sizeof(wbuf)),
                   TEXT("NetHack"), MB_OK);
}

/*
raw_print_bold(str)
                -- Like raw_print(), but prints in bold/standout (if
possible).
*/
void
mswin_raw_print_bold(const char *str)
{
    TCHAR wbuf[255];
    logDebug("mswin_raw_print_bold(%s)\n", str);
    if (str && *str)
        MessageBox(GetNHApp()->hMainWnd, NH_A2W(str, wbuf, sizeof(wbuf)),
                   TEXT("NetHack"), MB_OK);
}

/*
int nhgetch()   -- Returns a single character input from the user.
                -- In the tty window-port, nhgetch() assumes that tgetch()
                   will be the routine the OS provides to read a character.
                   Returned character _must_ be non-zero.
*/
int
mswin_nhgetch()
{
    PMSNHEvent event;
    int key = 0;

    logDebug("mswin_nhgetch()\n");

    while ((event = mswin_input_pop()) == NULL || event->type != NHEVENT_CHAR)
        mswin_main_loop();

    key = event->kbd.ch;
    return (key);
}

/*
int nh_poskey(int *x, int *y, int *mod)
                -- Returns a single character input from the user or a
                   a positioning event (perhaps from a mouse).  If the
                   return value is non-zero, a character was typed, else,
                   a position in the MAP window is returned in x, y and mod.
                   mod may be one of

                        CLICK_1         -- mouse click type 1
                        CLICK_2         -- mouse click type 2

                   The different click types can map to whatever the
                   hardware supports.  If no mouse is supported, this
                   routine always returns a non-zero character.
*/
int
mswin_nh_poskey(int *x, int *y, int *mod)
{
    PMSNHEvent event;
    int key;

    logDebug("mswin_nh_poskey()\n");

    while ((event = mswin_input_pop()) == NULL)
        mswin_main_loop();

    if (event->type == NHEVENT_MOUSE) {
        *mod = event->ms.mod;
        *x = event->ms.x;
        *y = event->ms.y;
        key = 0;
    } else {
        key = event->kbd.ch;
    }
    return (key);
}

/*
nhbell()        -- Beep at user.  [This will exist at least until sounds are
                   redone, since sounds aren't attributable to windows
anyway.]
*/
void
mswin_nhbell()
{
    logDebug("mswin_nhbell()\n");
}

/*
doprev_message()
                -- Display previous messages.  Used by the ^P command.
                -- On the tty-port this scrolls WIN_MESSAGE back one line.
*/
int
mswin_doprev_message()
{
    logDebug("mswin_doprev_message()\n");
    SendMessage(mswin_hwnd_from_winid(WIN_MESSAGE), WM_VSCROLL,
                MAKEWPARAM(SB_LINEUP, 0), (LPARAM) NULL);
    return 0;
}

/*
char yn_function(const char *ques, const char *choices, char default)
                -- Print a prompt made up of ques, choices and default.
                   Read a single character response that is contained in
                   choices or default.  If choices is NULL, all possible
                   inputs are accepted and returned.  This overrides
                   everything else.  The choices are expected to be in
                   lower case.  Entering ESC always maps to 'q', or 'n',
                   in that order, if present in choices, otherwise it maps
                   to default.  Entering any other quit character (SPACE,
                   RETURN, NEWLINE) maps to default.
                -- If the choices string contains ESC, then anything after
                   it is an acceptable response, but the ESC and whatever
                   follows is not included in the prompt.
                -- If the choices string contains a '#' then accept a count.
                   Place this value in the global "yn_number" and return '#'.
                -- This uses the top line in the tty window-port, other
                   ports might use a popup.
*/
char
mswin_yn_function(const char *question, const char *choices, CHAR_P def)
{
    int result = -1;
    char ch;
    char yn_esc_map = '\033';
    char message[BUFSZ];
    char res_ch[2];

    logDebug("mswin_yn_function(%s, %s, %d)\n", question, choices, def);

    if (choices) {
        char *cb, choicebuf[QBUFSZ];
        Strcpy(choicebuf, choices);
        if ((cb = strchr(choicebuf, '\033')) != 0) {
            /* anything beyond <esc> is hidden */
            *cb = '\0';
        }
        (void) strncpy(message, question, QBUFSZ - 1);
        message[QBUFSZ - 1] = '\0';
        sprintf(eos(message), " [%s]", choicebuf);
        if (def)
            sprintf(eos(message), " (%c)", def);
        Strcat(message, " ");
        /* escape maps to 'q' or 'n' or default, in that order */
        yn_esc_map =
            (strchr(choices, 'q') ? 'q' : (strchr(choices, 'n') ? 'n' : def));
    } else {
        Strcpy(message, question);
    }

#if defined(WIN_CE_SMARTPHONE)
    {
        char buf[BUFSZ];
        ZeroMemory(buf, sizeof(buf));
        if (choices) {
            if (!strchr(choices, '\033'))
                buf[0] = '\033'; /* make sure ESC is always available */
            strncat(buf, choices, sizeof(buf) - 2);
            NHSPhoneSetKeypadFromString(buf);
        } else {
            /* sometimes choices are included in the message itself, e.g.
             * "what? [abcd]" */
            char *p1, *p2;
            p1 = strchr(question, '[');
            p2 = strrchr(question, ']');
            if (p1 && p2 && p1 < p2) {
                buf[0] = '\033'; /* make sure ESC is always available */
                strncat(buf, p1 + 1, p2 - p1 - 1);
                NHSPhoneSetKeypadFromString(buf);
            } else if (strstr(question, "direction")) {
                /* asking for direction here */
                NHSPhoneSetKeypadDirection();
            } else {
                /* anything goes */
                NHSPhoneSetKeypadFromString("\0330-9a-zA-Z");
            }
        }
    }
#endif /* defined(WIN_CE_SMARTPHONE) */

    mswin_putstr(WIN_MESSAGE, ATR_BOLD, message);

    /* Only here if main window is not present */
    while (result < 0) {
        ch = mswin_nhgetch();
        if (ch == '\033') {
            result = yn_esc_map;
        } else if (choices && !strchr(choices, ch)) {
            /* FYI: ch==-115 is for KP_ENTER */
            if (def
                && (ch == ' ' || ch == '\r' || ch == '\n' || ch == -115)) {
                result = def;
            } else {
                mswin_nhbell();
                /* and try again... */
            }
        } else {
            result = ch;
        }
    }

    /* display selection in the message window */
    if (isprint(ch)) {
        res_ch[0] = ch;
        res_ch[1] = '\x0';
        mswin_putstr_ex(WIN_MESSAGE, ATR_BOLD, res_ch, 1);
    }

    /* prevent "--more--" prompt from appearing when several
       questions being asked in the same loop (like selling
       something in the shop)
       It does not really clears the window - mhmsgwnd.c */
    mswin_clear_nhwindow(WIN_MESSAGE);

#if defined(WIN_CE_SMARTPHONE)
    NHSPhoneSetKeypadDefault();
#endif
    return result;
}

/*
getlin(const char *ques, char *input)
            -- Prints ques as a prompt and reads a single line of text,
               up to a newline.  The string entered is returned without the
               newline.  ESC is used to cancel, in which case the string
               "\033\000" is returned.
            -- getlin() must call flush_screen(1) before doing anything.
            -- This uses the top line in the tty window-port, other
               ports might use a popup.
*/
void
mswin_getlin(const char *question, char *input)
{
    logDebug("mswin_getlin(%s, %p)\n", question, input);
    if (mswin_getlin_window(question, input, BUFSZ) == IDCANCEL) {
        strcpy(input, "\033");
    }
}

/*
int get_ext_cmd(void)
            -- Get an extended command in a window-port specific way.
               An index into extcmdlist[] is returned on a successful
               selection, -1 otherwise.
*/
int
mswin_get_ext_cmd()
{
    int ret;
    logDebug("mswin_get_ext_cmd()\n");

    if (mswin_ext_cmd_window(&ret) == IDCANCEL)
        return -1;
    else
        return ret;
}

/*
number_pad(state)
            -- Initialize the number pad to the given state.
*/
void
mswin_number_pad(int state)
{
    /* Do Nothing */
    logDebug("mswin_number_pad(%d)\n", state);
}

/*
delay_output()  -- Causes a visible delay of 50ms in the output.
               Conceptually, this is similar to wait_synch() followed
               by a nap(50ms), but allows asynchronous operation.
*/
void
mswin_delay_output()
{
    logDebug("mswin_delay_output()\n");
    Sleep(50);
}

void
mswin_change_color()
{
    logDebug("mswin_change_color()\n");
}

char *
mswin_get_color_string()
{
    logDebug("mswin_get_color_string()\n");
    return ("");
}

/*
start_screen()  -- Only used on Unix tty ports, but must be declared for
               completeness.  Sets up the tty to work in full-screen
               graphics mode.  Look at win/tty/termcap.c for an
               example.  If your window-port does not need this function
               just declare an empty function.
*/
void
mswin_start_screen()
{
    /* Do Nothing */
    logDebug("mswin_start_screen()\n");
}

/*
end_screen()    -- Only used on Unix tty ports, but must be declared for
               completeness.  The complement of start_screen().
*/
void
mswin_end_screen()
{
    /* Do Nothing */
    logDebug("mswin_end_screen()\n");
}

/*
outrip(winid, int, when)
            -- The tombstone code.  If you want the traditional code use
               genl_outrip for the value and check the #if in rip.c.
*/
void
mswin_outrip(winid wid, int how, time_t when)
{
    logDebug("mswin_outrip(%d, %d, %ld)\n", wid, how, (long) when);
    if ((wid >= 0) && (wid < MAXWINDOWS)) {
        DestroyWindow(GetNHApp()->windowlist[wid].win);
        GetNHApp()->windowlist[wid].win = mswin_init_RIP_window();
        GetNHApp()->windowlist[wid].type = NHW_RIP;
        GetNHApp()->windowlist[wid].dead = 0;
    }
    genl_outrip(wid, how, when);
}

/* handle options updates here */
void
mswin_preference_update(const char *pref)
{
    HDC hdc;

    if (_stricmp(pref, "font_menu") == 0
        || _stricmp(pref, "font_size_menu") == 0) {
        if (iflags.wc_fontsiz_menu < NHFONT_SIZE_MIN
            || iflags.wc_fontsiz_menu > NHFONT_SIZE_MAX)
            iflags.wc_fontsiz_menu = NHFONT_DEFAULT_SIZE;

        hdc = GetDC(GetNHApp()->hMainWnd);
        mswin_get_font(NHW_MENU, ATR_NONE, hdc, TRUE);
        mswin_get_font(NHW_MENU, ATR_BOLD, hdc, TRUE);
        mswin_get_font(NHW_MENU, ATR_DIM, hdc, TRUE);
        mswin_get_font(NHW_MENU, ATR_ULINE, hdc, TRUE);
        mswin_get_font(NHW_MENU, ATR_BLINK, hdc, TRUE);
        mswin_get_font(NHW_MENU, ATR_INVERSE, hdc, TRUE);
        ReleaseDC(GetNHApp()->hMainWnd, hdc);

        mswin_layout_main_window(NULL);
        return;
    }

    if (_stricmp(pref, "font_status") == 0
        || _stricmp(pref, "font_size_status") == 0) {
        if (iflags.wc_fontsiz_status < NHFONT_SIZE_MIN
            || iflags.wc_fontsiz_status > NHFONT_SIZE_MAX)
            iflags.wc_fontsiz_status = NHFONT_DEFAULT_SIZE;

        hdc = GetDC(GetNHApp()->hMainWnd);
        mswin_get_font(NHW_STATUS, ATR_NONE, hdc, TRUE);
        mswin_get_font(NHW_STATUS, ATR_BOLD, hdc, TRUE);
        mswin_get_font(NHW_STATUS, ATR_DIM, hdc, TRUE);
        mswin_get_font(NHW_STATUS, ATR_ULINE, hdc, TRUE);
        mswin_get_font(NHW_STATUS, ATR_BLINK, hdc, TRUE);
        mswin_get_font(NHW_STATUS, ATR_INVERSE, hdc, TRUE);
        ReleaseDC(GetNHApp()->hMainWnd, hdc);

        mswin_layout_main_window(NULL);
        return;
    }

    if (_stricmp(pref, "font_message") == 0
        || _stricmp(pref, "font_size_message") == 0) {
        if (iflags.wc_fontsiz_message < NHFONT_SIZE_MIN
            || iflags.wc_fontsiz_message > NHFONT_SIZE_MAX)
            iflags.wc_fontsiz_message = NHFONT_DEFAULT_SIZE;

        hdc = GetDC(GetNHApp()->hMainWnd);
        mswin_get_font(NHW_MESSAGE, ATR_NONE, hdc, TRUE);
        mswin_get_font(NHW_MESSAGE, ATR_BOLD, hdc, TRUE);
        mswin_get_font(NHW_MESSAGE, ATR_DIM, hdc, TRUE);
        mswin_get_font(NHW_MESSAGE, ATR_ULINE, hdc, TRUE);
        mswin_get_font(NHW_MESSAGE, ATR_BLINK, hdc, TRUE);
        mswin_get_font(NHW_MESSAGE, ATR_INVERSE, hdc, TRUE);
        ReleaseDC(GetNHApp()->hMainWnd, hdc);

        mswin_layout_main_window(NULL);
        return;
    }

    if (_stricmp(pref, "font_text") == 0
        || _stricmp(pref, "font_size_text") == 0) {
        if (iflags.wc_fontsiz_text < NHFONT_SIZE_MIN
            || iflags.wc_fontsiz_text > NHFONT_SIZE_MAX)
            iflags.wc_fontsiz_text = NHFONT_DEFAULT_SIZE;

        hdc = GetDC(GetNHApp()->hMainWnd);
        mswin_get_font(NHW_TEXT, ATR_NONE, hdc, TRUE);
        mswin_get_font(NHW_TEXT, ATR_BOLD, hdc, TRUE);
        mswin_get_font(NHW_TEXT, ATR_DIM, hdc, TRUE);
        mswin_get_font(NHW_TEXT, ATR_ULINE, hdc, TRUE);
        mswin_get_font(NHW_TEXT, ATR_BLINK, hdc, TRUE);
        mswin_get_font(NHW_TEXT, ATR_INVERSE, hdc, TRUE);
        ReleaseDC(GetNHApp()->hMainWnd, hdc);

        mswin_layout_main_window(NULL);
        return;
    }

    if (_stricmp(pref, "scroll_margin") == 0) {
        mswin_cliparound(u.ux, u.uy);
        return;
    }

    if (_stricmp(pref, "map_mode") == 0) {
        mswin_select_map_mode(iflags.wc_map_mode);
        return;
    }

    if (_stricmp(pref, "hilite_pet") == 0) {
        InvalidateRect(mswin_hwnd_from_winid(WIN_MAP), NULL, TRUE);
        return;
    }

    if (_stricmp(pref, "align_message") == 0
        || _stricmp(pref, "align_status") == 0) {
        mswin_layout_main_window(NULL);
        return;
    }

    if (_stricmp(pref, "vary_msgcount") == 0) {
        InvalidateRect(mswin_hwnd_from_winid(WIN_MESSAGE), NULL, TRUE);
        mswin_layout_main_window(NULL);
        return;
    }

    if (_stricmp(pref, "fullscreen") == 0) {
        mswin_set_fullscreen(iflags.wc2_fullscreen);
        return;
    }

    if (_stricmp(pref, "softkeyboard") == 0) {
        GetNHApp()->bUseSIP = iflags.wc2_softkeyboard;
        return;
    }

    if (_stricmp(pref, "wraptext") == 0) {
        GetNHApp()->bWrapText = iflags.wc2_wraptext;
        return;
    }
}

void
mswin_main_loop()
{
    MSG msg;

    while (!mswin_have_input() && GetMessage(&msg, NULL, 0, 0) != 0) {
        if (!TranslateAccelerator(msg.hwnd, GetNHApp()->hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

/* clean up and quit */
void
bail(const char *mesg)
{
    clearlocks();
    mswin_exit_nhwindows(mesg);
    nh_terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}

BOOL
initMapTiles(void)
{
    HBITMAP hBmp;
    BITMAP bm;
    TCHAR wbuf[MAX_PATH];
    int tl_num;
    SIZE map_size;
    extern int total_tiles_used;

    /* no file - no tile */
    if (!(iflags.wc_tile_file && *iflags.wc_tile_file))
        return TRUE;

    /* load bitmap */
    hBmp = SHLoadDIBitmap(NH_A2W(iflags.wc_tile_file, wbuf, MAX_PATH));
    if (hBmp == NULL) {
        raw_print(
            "Cannot load tiles from the file. Reverting back to default.");
        return FALSE;
    }

    /* calculate tile dimensions */
    GetObject(hBmp, sizeof(BITMAP), (LPVOID) &bm);
    if (bm.bmWidth % iflags.wc_tile_width
        || bm.bmHeight % iflags.wc_tile_height) {
        DeleteObject(hBmp);
        raw_print("Tiles bitmap does not match tile_width and tile_height "
                  "options. Reverting back to default.");
        return FALSE;
    }

    tl_num = (bm.bmWidth / iflags.wc_tile_width)
             * (bm.bmHeight / iflags.wc_tile_height);
    if (tl_num < total_tiles_used) {
        DeleteObject(hBmp);
        raw_print("Number of tiles in the bitmap is less than required by "
                  "the game. Reverting back to default.");
        return FALSE;
    }

    /* set the tile information */
    if (GetNHApp()->bmpMapTiles != GetNHApp()->bmpTiles) {
        DeleteObject(GetNHApp()->bmpMapTiles);
    }

    GetNHApp()->bmpMapTiles = hBmp;
    GetNHApp()->mapTile_X = iflags.wc_tile_width;
    GetNHApp()->mapTile_Y = iflags.wc_tile_height;
    GetNHApp()->mapTilesPerLine = bm.bmWidth / iflags.wc_tile_width;

    map_size.cx = GetNHApp()->mapTile_X * COLNO;
    map_size.cy = GetNHApp()->mapTile_Y * ROWNO;
    mswin_map_stretch(mswin_hwnd_from_winid(WIN_MAP), &map_size, TRUE);
    return TRUE;
}

void
mswin_popup_display(HWND hWnd, int *done_indicator)
{
    MSG msg;
    HWND hChild;

    /* activate the menu window */
    GetNHApp()->hPopupWnd = hWnd;

    mswin_layout_main_window(hWnd);

    /* disable game windows */
    for (hChild = GetWindow(GetNHApp()->hMainWnd, GW_CHILD); hChild;
         hChild = GetWindow(hChild, GW_HWNDNEXT)) {
        if (hChild != hWnd)
            EnableWindow(hChild, FALSE);
    }
#if defined(WIN_CE_SMARTPHONE)
    ShowWindow(GetNHApp()->hMenuBar, SW_HIDE);
    ShowWindow(SHFindMenuBar(hWnd), SW_SHOW);
#else
    EnableWindow(GetNHApp()->hMenuBar, FALSE);
#endif

    /* bring menu window on top */
    SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    /* go into message loop */
    if (done_indicator)
        *done_indicator = 0;
    while (IsWindow(hWnd) && (done_indicator == NULL || !*done_indicator)
           && GetMessage(&msg, NULL, 0, 0) != 0) {
        if (!IsDialogMessage(hWnd, &msg)) {
            if (!TranslateAccelerator(msg.hwnd, GetNHApp()->hAccelTable,
                                      &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
}

void
mswin_popup_destroy(HWND hWnd)
{
    HWND hChild;

    /* enable game windows */
    for (hChild = GetWindow(GetNHApp()->hMainWnd, GW_CHILD); hChild;
         hChild = GetWindow(hChild, GW_HWNDNEXT)) {
        if (hChild != hWnd) {
            EnableWindow(hChild, TRUE);
        }
    }
#if defined(WIN_CE_SMARTPHONE)
    ShowWindow(SHFindMenuBar(hWnd), SW_HIDE);
    ShowWindow(GetNHApp()->hMenuBar, SW_SHOW);
#else
    EnableWindow(GetNHApp()->hMenuBar, TRUE);
#endif

    SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
    GetNHApp()->hPopupWnd = NULL;
    mswin_window_mark_dead(mswin_winid_from_handle(hWnd));
    DestroyWindow(hWnd);

    mswin_layout_main_window(hWnd);

    SetFocus(GetNHApp()->hMainWnd);
}

void
mswin_set_fullscreen(BOOL is_fullscreen)
{
#if defined(WIN_CE_POCKETPC) || defined(WIN_CE_SMARTPHONE)
    SetForegroundWindow(GetNHApp()->hMainWnd);
    if (is_fullscreen) {
        SHFullScreen(GetNHApp()->hMainWnd,
                     SHFS_HIDETASKBAR | SHFS_HIDESTARTICON);
        MoveWindow(GetNHApp()->hMainWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                   GetSystemMetrics(SM_CYSCREEN), FALSE);
    } else {
        RECT rc;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
        SHFullScreen(GetNHApp()->hMainWnd,
                     SHFS_SHOWTASKBAR | SHFS_SHOWSTARTICON);
        MoveWindow(GetNHApp()->hMainWnd, rc.left, rc.top, rc.right - rc.left,
                   rc.bottom - rc.top, FALSE);
    }
    GetNHApp()->bFullScreen = is_fullscreen;
#else
    GetNHApp()->bFullScreen = FALSE;
#endif
}

#if defined(WIN_CE_SMARTPHONE)
void
NHSPhoneDialogSetup(HWND hDlg, UINT nToolBarId, BOOL is_edit,
                    BOOL is_fullscreen)
{
    SHMENUBARINFO mbi;
    HWND hOK, hCancel;
    RECT rtOK, rtDlg;

    // Create our MenuBar
    ZeroMemory(&mbi, sizeof(SHMENUBARINFO));
    mbi.cbSize = sizeof(mbi);
    mbi.hwndParent = hDlg;
    mbi.nToolBarId = nToolBarId;
    mbi.hInstRes = GetNHApp()->hApp;
    if (!SHCreateMenuBar(&mbi)) {
        error("cannot create dialog menu");
    }

    if (is_fullscreen) {
        SHINITDLGINFO shidi;
        RECT main_wnd_rect;
        shidi.dwMask = SHIDIM_FLAGS;
        shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
        shidi.hDlg = hDlg;
        SHInitDialog(&shidi);

        GetWindowRect(GetNHApp()->hMainWnd, &main_wnd_rect);
        MoveWindow(hDlg, main_wnd_rect.left, main_wnd_rect.top,
                   main_wnd_rect.right - main_wnd_rect.left,
                   main_wnd_rect.bottom - main_wnd_rect.top, FALSE);
    }

    /* hide OK and CANCEL buttons */
    hOK = GetDlgItem(hDlg, IDOK);
    hCancel = GetDlgItem(hDlg, IDCANCEL);

    if (IsWindow(hCancel))
        ShowWindow(hCancel, SW_HIDE);
    if (IsWindow(hOK)) {
        GetWindowRect(hOK, &rtOK);
        GetWindowRect(hDlg, &rtDlg);

        rtDlg.bottom -= rtOK.bottom - rtOK.top;
        ShowWindow(hOK, SW_HIDE);
        SetWindowPos(hDlg, HWND_TOP, 0, 0, rtDlg.right - rtDlg.left,
                     rtDlg.bottom - rtDlg.top,
                     SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOZORDER);
    }

    /* override "Back" button for edit box dialogs */
    if (is_edit)
        SendMessage(mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
                    MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
                               SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
}
#endif /* defined(WIN_CE_SMARTPHONE) */

void
mswin_read_reg(void)
{
}

void
mswin_destroy_reg(void)
{
}

void
mswin_write_reg(void)
{
}

/* check HKCU\Software\\Microsoft\\Shell\HasKeyboard for keyboard presence,
  if the key is not there assume older device and no keyboard */
BOOL
mswin_has_keyboard(void)
{
    DWORD dwHasKB = 0;
#if defined(WIN_CE_POCKETPC) || defined(WIN_CE_SMARTPHONE)
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize = sizeof(dwHasKB);
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Shell"), 0,
                     0, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, _T("HasKeyboard"), NULL, &dwType,
                            (LPBYTE) &dwHasKB, &dwSize) != ERROR_SUCCESS) {
            dwHasKB = 0;
        }
        RegCloseKey(hKey);
    }
#endif
    return (dwHasKB == 1);
}

#ifdef _DEBUG
#include <stdarg.h>

void
logDebug(const char *fmt, ...)
{
    FILE *dfp = fopen("nhtrace.log", "a");

    if (dfp) {
        va_list args;

        va_start(args, fmt);
        vfprintf(dfp, fmt, args);
        va_end(args);
        fclose(dfp);
    }
}

#endif

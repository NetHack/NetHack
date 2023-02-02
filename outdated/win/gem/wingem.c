/* NetHack 3.6	wingem.c	$NHDT-Date: 1450453304 2015/12/18 15:41:44 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.26 $ */
/* Copyright (c) Christian Bressler, 1999 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "func_tab.h"
#include "dlb.h"
#include <ctype.h>
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif

#ifdef GEM_GRAPHICS
#include "wingem.h"

static char nullstr[] = "", winpanicstr[] = "Bad window id %d";
static int curr_status_line;

static char *copy_of(const char *);
static void bail(const char *); /* __attribute__((noreturn)) */

extern int mar_set_tile_mode(int);
extern void mar_set_font(int, const char *, int);
extern void mar_set_margin(int);
extern void mar_set_msg_visible(int);
extern void mar_set_status_align(int);
extern void mar_set_msg_align(int);
extern void mar_set_tilefile(char *);
extern void mar_set_tilex(int);
extern void mar_set_tiley(int);
extern short glyph2tile[MAX_GLYPH];      /* from tile.c */
extern void mar_display_nhwindow(winid); /* from wingem1.c */

void Gem_outrip(winid, int, time_t);
void Gem_preference_update(const char *);
/* Interface definition, for windows.c */
struct window_procs Gem_procs = {
    "Gem",
    WC_COLOR | WC_HILITE_PET | WC_ALIGN_MESSAGE | WC_ALIGN_STATUS | WC_INVERSE
        | WC_SCROLL_MARGIN | WC_FONT_MESSAGE | WC_FONT_STATUS | WC_FONT_MENU
        | WC_FONT_TEXT | WC_FONT_MAP | WC_FONTSIZ_MESSAGE | WC_FONTSIZ_STATUS
        | WC_FONTSIZ_MENU | WC_FONTSIZ_TEXT | WC_FONTSIZ_MAP | WC_TILE_WIDTH
        | WC_TILE_HEIGHT | WC_TILE_FILE | WC_VARY_MSGCOUNT | WC_ASCII_MAP,
    0L,
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   /* color availability */
    Gem_init_nhwindows, Gem_player_selection, Gem_askname,
    Gem_get_nh_event, Gem_exit_nhwindows, Gem_suspend_nhwindows,
    Gem_resume_nhwindows, Gem_create_nhwindow, Gem_clear_nhwindow,
    Gem_display_nhwindow, Gem_destroy_nhwindow, Gem_curs, Gem_putstr,
    genl_putmixed, Gem_display_file, Gem_start_menu, Gem_add_menu,
    Gem_end_menu, Gem_select_menu, genl_message_menu, Gem_update_inventory,
    Gem_mark_synch, Gem_wait_synch,
#ifdef CLIPPING
    Gem_cliparound,
#endif
#ifdef POSITIONBAR
    Gem_update_positionbar,
#endif
    Gem_print_glyph, Gem_raw_print, Gem_raw_print_bold, Gem_nhgetch,
    Gem_nh_poskey, Gem_nhbell, Gem_doprev_message, Gem_yn_function,
    Gem_getlin, Gem_get_ext_cmd, Gem_number_pad, Gem_delay_output,
#ifdef CHANGE_COLOR /* the Mac uses a palette device */
    Gem_change_color,
#ifdef MAC
    Gem_change_background, Gem_set_font_name,
#endif
    Gem_get_color_string,
#endif

    /* other defs that really should go away (they're tty specific) */
    Gem_start_screen, Gem_end_screen, Gem_outrip, Gem_preference_update,
    genl_getmsghistory, genl_putmsghistory
                            genl_status_init,
    genl_status_finish, genl_status_enablefield, genl_status_update,
    genl_can_suspend_no,
};

#ifdef MAC
void *
Gem_change_background(dummy)
int dummy;
{
}

short *
Gem_set_font_name(foo, bar)
winid foo;
char *bar;
{
}
#endif

/*************************** Proceduren *************************************/

int
mar_hp_query(void)
{
    if (Upolyd)
        return (u.mh ? u.mhmax / u.mh : -1);
    return (u.uhp ? u.uhpmax / u.uhp : -1);
}

int
mar_iflags_numpad()
{
    return (iflags.num_pad ? 1 : 0);
}

int
mar_get_msg_history()
{
    return (iflags.msg_history);
}

int
mar_get_msg_visible()
{
    return (iflags.wc_vary_msgcount);
}
/* clean up and quit */
static void
bail(mesg)
const char *mesg;
{
    clearlocks();
    Gem_exit_nhwindows(mesg);
    nh_terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}

/*$$$*/
#define DEF_CLIPAROUND_MARGIN -1
#ifndef TILE_X
#define TILE_X 16
#endif
#define TILE_Y 16
#define TILES_PER_LINE 20
#define NHFONT_DEFAULT_SIZE 10
#define NHFONT_SIZE_MIN 3
#define NHFONT_SIZE_MAX 20
/*$$$*/
/*ARGSUSED*/
void
Gem_init_nhwindows(argcp, argv)
int *argcp;
char **argv;
{
    argv = argv, argcp = argcp;
    colors_changed = TRUE;

    set_wc_option_mod_status(WC_ALIGN_MESSAGE | WC_ALIGN_STATUS
                                 | WC_TILE_WIDTH | WC_TILE_HEIGHT
                                 | WC_TILE_FILE,
                             set_gameview);
    set_wc_option_mod_status(
        WC_HILITE_PET | WC_SCROLL_MARGIN | WC_FONT_MESSAGE | WC_FONT_MAP
            | WC_FONT_STATUS | WC_FONT_MENU | WC_FONT_TEXT
            | WC_FONTSIZ_MESSAGE | WC_FONTSIZ_MAP | WC_FONTSIZ_STATUS
            | WC_FONTSIZ_MENU | WC_FONTSIZ_TEXT | WC_VARY_MSGCOUNT,
        set_in_game);
    if (iflags.wc_align_message == 0)
        iflags.wc_align_message = ALIGN_TOP;
    if (iflags.wc_align_status == 0)
        iflags.wc_align_status = ALIGN_BOTTOM;
    if (iflags.wc_scroll_margin == 0)
        iflags.wc_scroll_margin = DEF_CLIPAROUND_MARGIN;
    if (iflags.wc_tile_width == 0)
        iflags.wc_tile_width = TILE_X;
    if (iflags.wc_tile_height == 0)
        iflags.wc_tile_height = TILE_Y;
    if (iflags.wc_tile_file && *iflags.wc_tile_file)
        mar_set_tilefile(iflags.wc_tile_file);
    if (iflags.wc_vary_msgcount == 0)
        iflags.wc_vary_msgcount = 3;
    mar_set_tile_mode(
        !iflags.wc_ascii_map); /* MAR -- 17.Mar 2002 True is tiles */
    mar_set_tilex(iflags.wc_tile_width);
    mar_set_tiley(iflags.wc_tile_height);
    mar_set_msg_align(iflags.wc_align_message - ALIGN_BOTTOM);
    mar_set_status_align(iflags.wc_align_status - ALIGN_BOTTOM);
    if (mar_gem_init() == 0) {
        bail((char *) 0);
        /*NOTREACHED*/
    }
    iflags.window_inited = TRUE;

    CO = 80; /* MAR -- whatsoever */
    LI = 25;

    add_menu_cmd_alias(' ', MENU_NEXT_PAGE);
    mar_set_no_glyph(NO_GLYPH);
}

void
Gem_player_selection()
{
    int i, k, n;
    char pick4u = 'n', pbuf[QBUFSZ], lastch = 0, currch;
    winid win;
    anything any;
    menu_item *selected = NULL;

    /* avoid unnecessary prompts further down */
    rigid_role_checks();

    /* Should we randomly pick for the player? */
    if (!flags.randomall
        && (flags.initrole == ROLE_NONE || flags.initrace == ROLE_NONE
            || flags.initgend == ROLE_NONE || flags.initalign == ROLE_NONE)) {
        /*		pick4u = yn_function("Shall I pick a character for you?
         * [ynq]",ynqchars,'n');*/
        pick4u = yn_function(build_plselection_prompt(
                                 pbuf, QBUFSZ, flags.initrole, flags.initrace,
                                 flags.initgend, flags.initalign),
                             ynqchars, 'n');
        if (pick4u == 'q') {
        give_up: /* Just quit */
            if (selected)
                free((genericptr_t) selected);
            bail((char *) 0);
            /*NOTREACHED*/
            return;
        }
    }

    /* Select a role, if necessary */
    if (flags.initrole < 0) {
        /* Process the choice */
        if (pick4u == 'y' || flags.initrole == ROLE_RANDOM
            || flags.randomall) {
            /* Pick a random role */
            flags.initrole = pick_role(flags.initrace, flags.initgend,
                                       flags.initalign, PICK_RANDOM);
            if (flags.initrole < 0) {
                mar_add_message("Incompatible role!");
                mar_display_nhwindow(WIN_MESSAGE);
                flags.initrole = randrole(FALSE);
            }
        } else {
            /* Prompt for a role */
            win = create_nhwindow(NHW_MENU);
            start_menu(win, MENU_BEHAVE_STANDARD);
            any.a_void = 0; /* zero out all bits */
            for (i = 0; roles[i].name.m; i++) {
                if (ok_role(i, flags.initrace, flags.initgend,
                            flags.initalign)) {
                    any.a_int = i + 1; /* must be non-zero */
                    currch = lowc(roles[i].name.m[0]);
                    if (currch == lastch)
                        currch = highc(currch);
                    add_menu(win, roles[i].mnum, &any, currch, 0, ATR_NONE,
                             an(roles[i].name.m), MENU_ITEMFLAGS_NONE);
                    lastch = currch;
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
            end_menu(win, "Pick a role");
            n = select_menu(win, PICK_ONE, &selected);
            destroy_nhwindow(win);

            /* Process the choice */
            if (n != 1 || selected[0].item.a_int == any.a_int)
                goto give_up; /* Selected quit */

            flags.initrole = selected[0].item.a_int - 1;
            free((genericptr_t) selected), selected = 0;
        }
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
                mar_add_message("Incompatible race!");
                mar_display_nhwindow(WIN_MESSAGE);
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
                Sprintf(pbuf, "Pick the race of your %s",
                        roles[flags.initrole].name.m);
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
                mar_add_message("Incompatible gender!");
                mar_display_nhwindow(WIN_MESSAGE);
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
                Sprintf(pbuf, "Pick the gender of your %s %s",
                        races[flags.initrace].adj,
                        roles[flags.initrole].name.m);
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
                mar_add_message("Incompatible alignment!");
                mar_display_nhwindow(WIN_MESSAGE);
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
                Sprintf(pbuf, "Pick the alignment of your %s %s %s",
                        genders[flags.initgend].adj,
                        races[flags.initrace].adj,
                        (flags.initgend && roles[flags.initrole].name.f)
                            ? roles[flags.initrole].name.f
                            : roles[flags.initrole].name.m);
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
    return;
}

/*
 * plname is filled either by an option (-u Player  or  -uPlayer) or
 * explicitly (by being the wizard) or by askname.
 * It may still contain a suffix denoting pl_character.
 * Always called after init_nhwindows() and before init_sound_and_display_gamewindows().
 */

void
Gem_askname()
{
    strncpy(gp.plname, mar_ask_name(), PL_NSIZ);
}

void
Gem_get_nh_event()
{
}

void
Gem_suspend_nhwindows(str)
const char *str;
{
    const char *foo;

    foo = str; /* MAR -- And the compiler whines no more ... */
}

void
Gem_resume_nhwindows()
{
}

void
Gem_end_screen()
{
}

void
Gem_start_screen()
{
}

extern void mar_exit_nhwindows(void);
extern boolean run_from_desktop;

void
Gem_exit_nhwindows(str)
const char *str;
{
    if (str)
        Gem_raw_print(str);
    mar_exit_nhwindows();
    if (iflags.toptenwin)
        run_from_desktop = FALSE;
    iflags.window_inited = 0;
}

winid
Gem_create_nhwindow(type)
int type;
{
    winid newid;

    switch (type) {
    case NHW_MESSAGE:
        if (iflags.msg_history < 20)
            iflags.msg_history = 20;
        else if (iflags.msg_history > 60)
            iflags.msg_history = 60;
        break;
    case NHW_STATUS:
    case NHW_MAP:
    case NHW_MENU:
    case NHW_TEXT:
        break;
    default:
        panic("Tried to create window type %d\n", (int) type);
        return WIN_ERR;
    }

    newid = mar_create_window(type);

    if (newid == MAXWIN) {
        panic("No window slots!");
        /* NOTREACHED */
    }

    return newid;
}

void
Gem_nhbell()
{
    if (flags.silent)
        return;
    putchar('\007');
    fflush(stdout);
}

extern void mar_clear_map(void);

void
Gem_clear_nhwindow(window)
winid window;
{
    if (window == WIN_ERR)
        panic(winpanicstr, window);

    switch (mar_hol_win_type(window)) {
    case NHW_MESSAGE:
        mar_clear_messagewin();
        break;
    case NHW_MAP:
        mar_clear_map();
        break;
    case NHW_STATUS:
    case NHW_MENU:
    case NHW_TEXT:
        break;
    }
}

extern void mar_more(void);

/*ARGSUSED*/
void
Gem_display_nhwindow(window, blocking)
winid window;
boolean blocking;
{
    if (window == WIN_ERR)
        panic(winpanicstr, window);

    mar_display_nhwindow(window);

    switch (mar_hol_win_type(window)) {
    case NHW_MESSAGE:
        if (blocking)
            mar_more();
        break;
    case NHW_MAP:
        if (blocking)
            Gem_display_nhwindow(WIN_MESSAGE, TRUE);
        break;
    case NHW_STATUS:
    case NHW_TEXT:
    case NHW_MENU:
    default:
        break;
    }
}

void
Gem_destroy_nhwindow(window)
winid window;
{
    if (window == WIN_ERR) /* MAR -- test existence */
        panic(winpanicstr, window);

    mar_destroy_nhwindow(window);
}

extern void mar_curs(int, int); /* mar_curs is only for map */

void
Gem_curs(window, x, y)
winid window;
register int x, y;
{
    if (window == WIN_ERR) /* MAR -- test existence */
        panic(winpanicstr, window);

    if (window == WIN_MAP)
        mar_curs(x - 1, y); /*$$$*/
    else if (window == WIN_STATUS)
        curr_status_line = y;
}

extern void mar_add_status_str(const char *, int);
extern void mar_putstr_text(winid, int, const char *);

void
Gem_putstr(window, attr, str)
winid window;
int attr;
const char *str;
{
    int win_type;

    if (window == WIN_ERR) {
        Gem_raw_print(str);
        return;
    }

    if (str == (const char *) 0)
        return;

    switch ((win_type = mar_hol_win_type(window))) {
    case NHW_MESSAGE:
        mar_add_message(str);
        break;

    case NHW_STATUS:
        mar_status_dirty();
        mar_add_status_str(str, curr_status_line);
        if (curr_status_line)
            mar_display_nhwindow(WIN_STATUS);
        break;

    case NHW_MAP:
        if (strcmp(str, "."))
            Gem_putstr(WIN_MESSAGE, 0, str);
        else
            mar_map_curs_weiter();
        mar_display_nhwindow(WIN_MESSAGE);
        mar_display_nhwindow(WIN_STATUS);
        break;

    case NHW_MENU:
        mar_change_menu_2_text(window);
    /* Fallthru */
    case NHW_TEXT:
        mar_putstr_text(window, attr, str);
        break;
    } /* endswitch win_type */
}

void
Gem_display_file(fname, complain)
const char *fname;
boolean complain;
{
    dlb *f;
    char buf[BUFSZ];
    char *cr;

    f = dlb_fopen(fname, "r");
    if (!f) {
        if (complain)
            pline("Cannot open \"%s\".", fname);
    } else {
        winid datawin;

        datawin = Gem_create_nhwindow(NHW_TEXT);
        while (dlb_fgets(buf, BUFSZ, f)) {
            if ((cr = strchr(buf, '\n')) != 0)
                *cr = 0;
            if (strchr(buf, '\t') != 0)
                (void) tabexpand(buf);
            Gem_putstr(datawin, 0, buf);
        }
        (void) dlb_fclose(f);
        Gem_display_nhwindow(datawin, FALSE);
        Gem_destroy_nhwindow(datawin);
    }
}

/*ARGSUSED*/
/*
 * Add a menu item to the beginning of the menu list.  This list is reversed
 * later.
 */
void
Gem_add_menu(window, glyph, identifier, ch, gch, attr, str, itemflags)
winid window;               /* window to use, must be of type NHW_MENU */
int glyph;                  /* glyph to display with item (unused) */
const anything *identifier; /* what to return if selected */
char ch;                    /* keyboard accelerator (0 = pick our own) */
char gch;                   /* group accelerator (0 = no group) */
int attr;                   /* attribute for string (like Gem_putstr()) */
const char *str;            /* menu string */
unsigned int itemflags;     /* itemflags such as marked as selected */
{
    boolean preselected = ((itemflags & MENU_ITEMFLAGS_SELECTED) != 0);
    Gem_menu_item *G_item;
    const char *newstr;
    char buf[QBUFSZ];

    if (str == (const char *) 0)
        return;

    if (window == WIN_ERR) /* MAR -- test existence */
        panic(winpanicstr, window);

    if (identifier->a_void)
        Sprintf(buf, "%c - %s", ch ? ch : '?', str);
    else
        Sprintf(buf, "%s", str);
    newstr = buf;

    G_item = (Gem_menu_item *) alloc(sizeof(Gem_menu_item));
    G_item->Gmi_identifier = (long) identifier->a_void;
    G_item->Gmi_glyph = glyph != NO_GLYPH ? glyph2tile[glyph] : NO_GLYPH;
    G_item->Gmi_count = -1L;
    G_item->Gmi_selected = preselected ? 1 : 0;
    G_item->Gmi_accelerator = ch;
    G_item->Gmi_groupacc = gch;
    G_item->Gmi_itemflags = itemflags;
    G_item->Gmi_attr = attr;
    G_item->Gmi_str = copy_of(newstr);
    mar_add_menu(window, G_item);
}

/*
 * End a menu in this window, window must a type NHW_MENU.
 * We assign the keyboard accelerators as needed.
 */
void
Gem_end_menu(window, prompt)
winid window;       /* menu to use */
const char *prompt; /* prompt to for menu */
{
    if (window == WIN_ERR || mar_hol_win_type(window) != NHW_MENU)
        panic(winpanicstr, window);

    /* Reverse the list so that items are in correct order. */
    mar_reverse_menu();

    /* Put the prompt at the beginning of the menu. */
    mar_set_menu_title(prompt);

    mar_set_accelerators();
}

int
Gem_select_menu(window, how, menu_list)
winid window;
int how;
menu_item **menu_list;
{
    Gem_menu_item *Gmit;
    menu_item *mi;
    int n;

    if (window == WIN_ERR || mar_hol_win_type(window) != NHW_MENU)
        panic(winpanicstr, window);

    *menu_list = (menu_item *) 0;
    mar_set_menu_type(how);
    Gem_display_nhwindow(window, TRUE);

    for (n = 0, Gmit = mar_hol_inv(); Gmit; Gmit = Gmit->Gmi_next)
        if (Gmit->Gmi_selected)
            n++;

    if (n > 0) {
        *menu_list = (menu_item *) alloc(n * sizeof(menu_item));
        for (mi = *menu_list, Gmit = mar_hol_inv(); Gmit;
             Gmit = Gmit->Gmi_next)
            if (Gmit->Gmi_selected) {
                mi->item = (anything)(genericptr_t) Gmit->Gmi_identifier;
                mi->count = Gmit->Gmi_count;
                mi++;
            }
    }

    return n;
}

void
Gem_update_inventory()
{
}

void
Gem_mark_synch()
{
    mar_display_nhwindow(WIN_MESSAGE);
    mar_display_nhwindow(WIN_MAP);
    mar_display_nhwindow(WIN_STATUS);
}

void
Gem_wait_synch()
{
    mar_display_nhwindow(WIN_MESSAGE);
    mar_display_nhwindow(WIN_MAP);
    mar_display_nhwindow(WIN_STATUS);
}

#ifdef CLIPPING
extern void mar_cliparound(void);
void
Gem_cliparound(x, y)
int x, y;
{
    mar_curs(x - 1, y);
    mar_cliparound();
}
#endif /* CLIPPING */

/*
 *  Gem_print_glyph
 *
 *  Print the glyph to the output device.  Don't flush the output device.
 *
 *  Since this is only called from show_glyph(), it is assumed that the
 *  position and glyph are always correct (checked there)!
 */

void mar_print_gl_char(winid, xchar, xchar, int);

extern int mar_set_rogue(int);

extern void mar_add_pet_sign(winid, int, int);

void
Gem_print_glyph(window, x, y, glyph, bkglyph)
winid window;
xchar x, y;
int glyph, bkglyph;
{
    /* Move the cursor. */
    Gem_curs(window, x, y);

    mar_set_rogue(Is_rogue_level(&u.uz) ? TRUE : FALSE);

    x--; /* MAR -- because x ranges from 1 to COLNO */
    if (mar_set_tile_mode(-1)) {
        mar_print_glyph(window, x, y, glyph2tile[glyph], glyph2tile[bkglyph]);
        if (
#ifdef TEXTCOLOR
            iflags.hilite_pet &&
#endif
            glyph_is_pet(glyph))
            mar_add_pet_sign(window, x, y);
    } else
        mar_print_gl_char(window, x, y, glyph);
}

void mar_print_char(winid, xchar, xchar, char, int);

void
mar_print_gl_char(window, x, y, glyph)
winid window;
xchar x, y;
int glyph;
{
    int ch;
    int color;
    unsigned special;

    /* map glyph to character and color */
    (void) mapglyph(glyph, &ch, &color, &special, x, y, 0);

#ifdef TEXTCOLOR
    /* Turn off color if rogue level. */
    if (Is_rogue_level(&u.uz))
        color = NO_COLOR;
#endif /* TEXTCOLOR */

    mar_print_char(window, x, y, ch, color);
}

extern void mar_raw_print(const char *);
extern void mar_raw_print_bold(const char *);

void
Gem_raw_print(str)
const char *str;
{
    if (str && *str) {
        if (iflags.window_inited)
            mar_raw_print(str);
        else
            printf("%s\n", str);
    }
}

void
Gem_raw_print_bold(str)
const char *str;
{
    if (str && *str) {
        if (iflags.window_inited)
            mar_raw_print_bold(str);
        else
            printf("%s\n", str);
    }
}

extern void mar_update_value(void); /* wingem1.c */

int
Gem_nhgetch()
{
    int i;

    mar_update_value();
    i = tgetch();
    if (!i)
        i = '\033'; /* map NUL to ESC since nethack doesn't expect NUL */

    return i;
}

/* Get a extended command in windowport specific way.
        returns index of the ext_cmd or -1.
        called after '#'.
        It's a menu with all the possibilities. */
int
Gem_get_ext_cmd()
{
    winid wind;
    int i, count, what, too_much = FALSE;
    menu_item *selected = NULL;
    anything any;
    char accelerator = 0, tmp_acc = 0;
    const char *ptr;

    wind = Gem_create_nhwindow(NHW_MENU);
    Gem_start_menu(wind, MENU_BEHAVE_STANDARD);
    for (i = 0; (ptr = extcmdlist[i].ef_txt); i++) {
        any.a_int = i;
        accelerator = *ptr;
        if (tmp_acc == accelerator) {
            if (too_much)
                accelerator = '&'; /* MAR -- poor choice, anyone? */
            else
                accelerator += 'A' - 'a';
            too_much = TRUE;
        } else
            too_much = FALSE;
        tmp_acc = *ptr;
        Gem_add_menu(wind, NO_GLYPH, &any, accelerator, 0, ATR_NONE, ptr,
                     MENU_ITEMFLAGS_NONE);
    }
    Gem_end_menu(wind, "What extended command?");
    count = Gem_select_menu(wind, PICK_ONE, &selected);
    what = count ? selected->item.a_int : -1;
    if (selected)
        free(selected);
    Gem_destroy_nhwindow(wind);
    return (what);
}

void
Gem_number_pad(state)
int state;
{
    state = state;
}

void
win_Gem_init()
{
}

#ifdef POSITIONBAR
void
Gem_update_positionbar(posbar)
char *posbar;
{
}
#endif

/** Gem_outrip **/
void mar_set_text_to_rip(winid);
char **rip_line = 0;

void
Gem_outrip(w, how, when)
winid w;
int how;
time_t when;
{
/* Code from X11 windowport */
#define STONE_LINE_LEN 15 /* # chars that fit on one line */
#define NAME_LINE 0       /* line # for player name */
#define GOLD_LINE 1       /* line # for amount of gold */
#define DEATH_LINE 2      /* line # for death description */
#define YEAR_LINE 6       /* line # for year */
    char buf[BUFSZ];
    char *dpx;
    int line;
    long year;

    if (!rip_line) {
        int i;
        rip_line = (char **) malloc((YEAR_LINE + 1) * sizeof(char *));
        for (i = 0; i < YEAR_LINE + 1; i++) {
            rip_line[i] =
                (char *) malloc((STONE_LINE_LEN + 1) * sizeof(char));
        }
    }
    /* Follows same algorithm as genl_outrip() */
    /* Put name on stone */
    Sprintf(rip_line[NAME_LINE], "%s", gp.plname);
    /* Put $ on stone */
    Sprintf(rip_line[GOLD_LINE], "%ld Au", done_money);
    /* Put together death description */
    formatkiller(buf, sizeof buf, how, FALSE);

    /* Put death type on stone */
    for (line = DEATH_LINE, dpx = buf; line < YEAR_LINE; line++) {
        register int i, i0;
        char tmpchar;
        if ((i0 = strlen(dpx)) > STONE_LINE_LEN) {
            for (i = STONE_LINE_LEN; ((i0 > STONE_LINE_LEN) && i); i--)
                if (dpx[i] == ' ')
                    i0 = i;
            if (!i)
                i0 = STONE_LINE_LEN;
        }
        tmpchar = dpx[i0];
        dpx[i0] = 0;
        strcpy(rip_line[line], dpx);
        if (tmpchar != ' ') {
            dpx[i0] = tmpchar;
            dpx = &dpx[i0];
        } else
            dpx = &dpx[i0 + 1];
    }
    /* Put year on stone */
    year = yyyymmdd(when) / 10000L;
    Sprintf(rip_line[YEAR_LINE], "%4ld", year);

    mar_set_text_to_rip(w);
    for (line = 0; line < 13; line++)
        putstr(w, 0, "");
}
void
mar_get_font(type, p_fname, psize)
int type;
char **p_fname;
int *psize;
{
    switch (type) {
    case NHW_MESSAGE:
        *p_fname = iflags.wc_font_message;
        *psize = iflags.wc_fontsiz_message;
        break;
    case NHW_MAP:
        *p_fname = iflags.wc_font_map;
        *psize = iflags.wc_fontsiz_map;
        break;
    case NHW_STATUS:
        *p_fname = iflags.wc_font_status;
        *psize = iflags.wc_fontsiz_status;
        break;
    case NHW_MENU:
        *p_fname = iflags.wc_font_menu;
        *psize = iflags.wc_fontsiz_menu;
        break;
    case NHW_TEXT:
        *p_fname = iflags.wc_font_text;
        *psize = iflags.wc_fontsiz_text;
        break;
    default:
        break;
    }
}
void
Gem_preference_update(pref)
const char *pref;
{
    if (stricmp(pref, "font_message") == 0
        || stricmp(pref, "font_size_message") == 0) {
        if (iflags.wc_fontsiz_message < NHFONT_SIZE_MIN
            || iflags.wc_fontsiz_message > NHFONT_SIZE_MAX)
            iflags.wc_fontsiz_message = NHFONT_DEFAULT_SIZE;
        mar_set_font(NHW_MESSAGE, iflags.wc_font_message,
                     iflags.wc_fontsiz_message);
        return;
    }
    if (stricmp(pref, "font_map") == 0
        || stricmp(pref, "font_size_map") == 0) {
        if (iflags.wc_fontsiz_map < NHFONT_SIZE_MIN
            || iflags.wc_fontsiz_map > NHFONT_SIZE_MAX)
            iflags.wc_fontsiz_map = NHFONT_DEFAULT_SIZE;
        mar_set_font(NHW_MAP, iflags.wc_font_map, iflags.wc_fontsiz_map);
        return;
    }
    if (stricmp(pref, "font_status") == 0
        || stricmp(pref, "font_size_status") == 0) {
        if (iflags.wc_fontsiz_status < NHFONT_SIZE_MIN
            || iflags.wc_fontsiz_status > NHFONT_SIZE_MAX)
            iflags.wc_fontsiz_status = NHFONT_DEFAULT_SIZE;
        mar_set_font(NHW_STATUS, iflags.wc_font_status,
                     iflags.wc_fontsiz_status);
        return;
    }
    if (stricmp(pref, "font_menu") == 0
        || stricmp(pref, "font_size_menu") == 0) {
        if (iflags.wc_fontsiz_menu < NHFONT_SIZE_MIN
            || iflags.wc_fontsiz_menu > NHFONT_SIZE_MAX)
            iflags.wc_fontsiz_menu = NHFONT_DEFAULT_SIZE;
        mar_set_font(NHW_MENU, iflags.wc_font_menu, iflags.wc_fontsiz_menu);
        return;
    }
    if (stricmp(pref, "font_text") == 0
        || stricmp(pref, "font_size_text") == 0) {
        if (iflags.wc_fontsiz_text < NHFONT_SIZE_MIN
            || iflags.wc_fontsiz_text > NHFONT_SIZE_MAX)
            iflags.wc_fontsiz_text = NHFONT_DEFAULT_SIZE;
        mar_set_font(NHW_TEXT, iflags.wc_font_text, iflags.wc_fontsiz_text);
        return;
    }
    if (stricmp(pref, "scroll_margin") == 0) {
        mar_set_margin(iflags.wc_scroll_margin);
        Gem_cliparound(u.ux, u.uy);
        return;
    }
    if (stricmp(pref, "ascii_map") == 0) {
        mar_set_tile_mode(!iflags.wc_ascii_map);
        doredraw();
        return;
    }
    if (stricmp(pref, "hilite_pet") == 0) {
        /* MAR -- works without doing something here. */
        return;
    }
    if (stricmp(pref, "align_message") == 0) {
        mar_set_msg_align(iflags.wc_align_message - ALIGN_BOTTOM);
        return;
    }
    if (stricmp(pref, "align_status") == 0) {
        mar_set_status_align(iflags.wc_align_status - ALIGN_BOTTOM);
        return;
    }
    if (stricmp(pref, "vary_msgcount") == 0) {
        mar_set_msg_visible(iflags.wc_vary_msgcount);
        return;
    }
}
/*
 * Allocate a copy of the given string.  If null, return a string of
 * zero length.
 *
 * This is an exact duplicate of copy_of() in X11/winmenu.c.
 */
static char *
copy_of(s)
const char *s;
{
    if (!s)
        s = nullstr;
    return strcpy((char *) alloc((unsigned) (strlen(s) + 1)), s);
}

#endif /* GEM_GRAPHICS \
                       \
/*wingem.c*/

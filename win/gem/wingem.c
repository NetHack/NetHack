/* NetHack 5.0	wingem.c	$NHDT-Date: 1450453304 2015/12/18 15:41:44 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.26 $ */
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

static char winpanicstr[] = "Bad window id %d";
static short curr_status_line;

static void bail(const char *); /* __attribute__((noreturn)) */

extern short mar_set_tile_mode(short);
extern void mar_set_font(short, const char *, short);
extern void mar_set_margin(short);
extern void mar_set_msg_visible(short);
extern void mar_set_status_align(short);
extern void mar_set_msg_align(short);
extern void mar_set_tilefile(char *);
extern void mar_set_tilex(short);
extern void mar_set_tiley(short);
extern void mar_display_nhwindow(winid); /* from wingem1.c */

void Gem_outrip(winid, int, time_t);
void Gem_preference_update(const char *);
/* Interface definition, for windows.c */
struct window_procs Gem_procs = {
    WPID(Gem),
    WC_COLOR | WC_HILITE_PET | WC_ALIGN_MESSAGE | WC_ALIGN_STATUS | WC_INVERSE
        | WC_SCROLL_MARGIN | WC_FONT_MESSAGE | WC_FONT_STATUS | WC_FONT_MENU
        | WC_FONT_TEXT | WC_FONT_MAP | WC_FONTSIZ_MESSAGE | WC_FONTSIZ_STATUS
        | WC_FONTSIZ_MENU | WC_FONTSIZ_TEXT | WC_FONTSIZ_MAP | WC_TILE_WIDTH
        | WC_TILE_HEIGHT | WC_TILE_FILE | WC_VARY_MSGCOUNT
        | WC_ASCII_MAP | WC_TILED_MAP,
    WC2_HILITE_STATUS | WC2_FLUSH_STATUS | WC2_HITPOINTBAR,
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   /* color availability */
    Gem_init_nhwindows, Gem_player_selection, Gem_askname,
    Gem_get_nh_event, Gem_exit_nhwindows, Gem_suspend_nhwindows,
    Gem_resume_nhwindows, Gem_create_nhwindow, Gem_clear_nhwindow,
    Gem_display_nhwindow, Gem_destroy_nhwindow, Gem_curs, Gem_putstr,
    genl_putmixed, Gem_display_file, Gem_start_menu, Gem_add_menu,
    Gem_end_menu, Gem_select_menu, genl_message_menu, Gem_mark_synch,
    Gem_wait_synch,
#ifdef CLIPPING
    Gem_cliparound,
#endif
#ifdef POSITIONBAR
    Gem_update_positionbar,
#endif
    Gem_print_glyph, Gem_raw_print, Gem_raw_print_bold, Gem_nhgetch,
    Gem_nh_poskey, Gem_nhbell, Gem_doprev_message, Gem_yn_function,
    Gem_getlin, Gem_get_ext_cmd, Gem_number_pad, Gem_delay_output,
    /* no CHANGE_COLOR entries: never defined for GEM, so struct
       window_procs has no such fields here */
    Gem_outrip, Gem_preference_update,
    genl_getmsghistory, genl_putmsghistory,
    Gem_status_init,
    genl_status_finish, genl_status_enablefield, Gem_status_update,
    genl_can_suspend_no,
    Gem_update_inventory,
    Gem_ctrl_nhwindow,
};


/*************************** Procedures *************************************/

/* hpmax/hp ratio for the map cursor colour; 10 = the critical band */
short
mar_hp_query(void)
{
    int hp = Upolyd ? u.mh : u.uhp;
    int hpmax = Upolyd ? u.mhmax : u.uhpmax;

    return (hp <= 0) ? 10 : (short) (hpmax / hp);
}

short
mar_iflags_numpad(void)
{
    return (iflags.num_pad ? 1 : 0);
}

/* clamped here so msg_anz is sized consistently at init */
short
mar_get_msg_history(void)
{
    if (iflags.msg_history < 20)
        iflags.msg_history = 20;
    else if (iflags.msg_history > 60)
        iflags.msg_history = 60;
    return (iflags.msg_history);
}

short
mar_get_msg_visible(void)
{
    return (iflags.wc_vary_msgcount);
}
/* clean up and quit */
static void
bail(const char *mesg)
{
    clearlocks();
    Gem_exit_nhwindows(mesg);
    nh_terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}

#define DEF_CLIPAROUND_MARGIN -1
#ifndef TILE_X
#define TILE_X 16
#endif
#ifndef TILE_Y
#define TILE_Y 16
#endif
#define NHFONT_DEFAULT_SIZE 10
#define NHFONT_SIZE_MIN 3
#define NHFONT_SIZE_MAX 20
#include "crashlog.h"
/*ARGSUSED*/
void
Gem_init_nhwindows(int *argcp, char **argv)
{
    nhUse(argcp);
    nhUse(argv);
    crash_checkpoint("gemwin");
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
        !iflags.wc_ascii_map); /* True is tiles */
    mar_set_tilex(iflags.wc_tile_width);
    mar_set_tiley(iflags.wc_tile_height);
    mar_set_msg_align(iflags.wc_align_message - ALIGN_BOTTOM);
    mar_set_status_align(iflags.wc_align_status - ALIGN_BOTTOM);
    {
        extern void Gem_flush_preinit_raw(void);
        if (mar_gem_init() == 0) {
            bail((char *) 0);
            /*NOTREACHED*/
        }
        iflags.window_inited = TRUE;
        Gem_flush_preinit_raw();
    }
    crash_checkpoint("gemok");
    crash_selftest();

    CO = 80;
    LI = 25;

    add_menu_cmd_alias(' ', MENU_NEXT_PAGE);
    mar_set_no_glyph(NO_GLYPH);
}

void
Gem_player_selection(void)
{
    short i, k, n;
    char pick4u = 'n', pbuf[QBUFSZ], lastch = 0, currch;
    winid win;
    anything any;
    menu_item *selected = NULL;

    crash_checkpoint("plsel");
    /* avoid unnecessary prompts further down */
    rigid_role_checks();

    /* Should we randomly pick for the player? */
    if (!flags.randomall
        && (flags.initrole == ROLE_NONE || flags.initrace == ROLE_NONE
            || flags.initgend == ROLE_NONE || flags.initalign == ROLE_NONE)) {
        pick4u = yn_function(build_plselection_prompt(
                                 pbuf, QBUFSZ, flags.initrole, flags.initrace,
                                 flags.initgend, flags.initalign),
                             ynqchars, 'n', TRUE);
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
                    glyph_info gi;
                    int glyph = monnum_to_glyph(roles[i].mnum, MALE);
                    map_glyphinfo(0, 0, glyph, 0, &gi);
                    any.a_int = i + 1; /* must be non-zero */
                    currch = lowc(roles[i].name.m[0]);
                    if (currch == lastch)
                        currch = highc(currch);
                    add_menu(win, &gi, &any, currch, 0, ATR_NONE,
                             NO_COLOR, an(roles[i].name.m), MENU_ITEMFLAGS_NONE);
                    lastch = currch;
                }
            }
            any.a_int = pick_role(flags.initrace, flags.initgend,
                                  flags.initalign, PICK_RANDOM) + 1;
            if (any.a_int == 0) /* must be non-zero */
                any.a_int = randrole(FALSE) + 1;
            add_menu(win, &nul_glyphinfo, &any, '*', 0, ATR_NONE, NO_COLOR,
                     "Random", MENU_ITEMFLAGS_NONE);
            any.a_int = i + 1; /* must be non-zero */
            add_menu(win, &nul_glyphinfo, &any, 'q', 0, ATR_NONE, NO_COLOR,
                     "Quit", MENU_ITEMFLAGS_NONE);
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
                        glyph_info gi;
                        int glyph = monnum_to_glyph(races[i].mnum, MALE);
                        map_glyphinfo(0, 0, glyph, 0, &gi);
                        any.a_int = i + 1; /* must be non-zero */
                        add_menu(win, &gi, &any, races[i].noun[0], 0,
                                 ATR_NONE, NO_COLOR, races[i].noun, MENU_ITEMFLAGS_NONE);
                    }
                any.a_int = pick_race(flags.initrole, flags.initgend,
                                      flags.initalign, PICK_RANDOM) + 1;
                if (any.a_int == 0) /* must be non-zero */
                    any.a_int = randrace(flags.initrole) + 1;
                add_menu(win, &nul_glyphinfo, &any, '*', 0, ATR_NONE, NO_COLOR,
                         "Random", MENU_ITEMFLAGS_NONE);
                any.a_int = i + 1; /* must be non-zero */
                add_menu(win, &nul_glyphinfo, &any, 'q', 0, ATR_NONE, NO_COLOR,
                         "Quit", MENU_ITEMFLAGS_NONE);
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
                        glyph_info gi;
                        int glyph = monnum_to_glyph(
                            roles[flags.initrole].mnum,
                            i == 0 ? MALE : FEMALE);
                        map_glyphinfo(0, 0, glyph, 0, &gi);
                        any.a_int = i + 1;
                        add_menu(win, &gi, &any, genders[i].adj[0], 0,
                                 ATR_NONE, NO_COLOR, genders[i].adj, MENU_ITEMFLAGS_NONE);
                    }
                any.a_int = pick_gend(flags.initrole, flags.initrace,
                                      flags.initalign, PICK_RANDOM) + 1;
                if (any.a_int == 0) /* must be non-zero */
                    any.a_int = randgend(flags.initrole, flags.initrace) + 1;
                add_menu(win, &nul_glyphinfo, &any, '*', 0, ATR_NONE, NO_COLOR,
                         "Random", MENU_ITEMFLAGS_NONE);
                any.a_int = i + 1; /* must be non-zero */
                add_menu(win, &nul_glyphinfo, &any, 'q', 0, ATR_NONE, NO_COLOR,
                         "Quit", MENU_ITEMFLAGS_NONE);
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
                        glyph_info gi;
                        int glyph = monnum_to_glyph(
                            roles[flags.initrole].mnum,
                            flags.initgend > 0 ? FEMALE : MALE);
                        map_glyphinfo(0, 0, glyph, 0, &gi);
                        any.a_int = i + 1;
                        add_menu(win, &gi, &any, aligns[i].adj[0], 0,
                                 ATR_NONE, NO_COLOR, aligns[i].adj, MENU_ITEMFLAGS_NONE);
                    }
                any.a_int = pick_align(flags.initrole, flags.initrace,
                                       flags.initgend, PICK_RANDOM) + 1;
                if (any.a_int == 0) /* must be non-zero */
                    any.a_int = randalign(flags.initrole, flags.initrace) + 1;
                add_menu(win, &nul_glyphinfo, &any, '*', 0, ATR_NONE, NO_COLOR,
                         "Random", MENU_ITEMFLAGS_NONE);
                any.a_int = i + 1; /* must be non-zero */
                add_menu(win, &nul_glyphinfo, &any, 'q', 0, ATR_NONE, NO_COLOR,
                         "Quit", MENU_ITEMFLAGS_NONE);
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
 * Always called after init_nhwindows() and before display_gamewindows().
 */

void
Gem_askname(void)
{
    strncpy(svp.plname, mar_ask_name(), PL_NSIZ);
    svp.plname[PL_NSIZ - 1] = '\0';
}

void
Gem_get_nh_event(void)
{
}

void
Gem_suspend_nhwindows(const char *str)
{
    nhUse(str);
}

void
Gem_resume_nhwindows(void)
{
}


extern void mar_exit_nhwindows(void);
extern boolean run_from_desktop;
static boolean gem_windows_exited = FALSE;
static void preinit_raw_dump_console(void);

void
Gem_exit_nhwindows(const char *str)
{
    if (str)
        Gem_raw_print(str);
    mar_exit_nhwindows();
    if (iflags.toptenwin)
        run_from_desktop = FALSE;
    iflags.window_inited = 0;
    gem_windows_exited = TRUE;
    preinit_raw_dump_console();
}

winid
Gem_create_nhwindow(int type)
{
    winid newid;

    switch (type) {
    case NHW_MESSAGE:
    case NHW_STATUS:
    case NHW_MAP:
    case NHW_MENU:
    case NHW_TEXT:
        break;
    default:
        panic("Tried to create window type %d\n", (short) type);
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
Gem_nhbell(void)
{
    if (flags.silent)
        return;
    putchar('\007');
    fflush(stdout);
}

extern void mar_clear_map(void);

void
Gem_clear_nhwindow(winid window)
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
Gem_display_nhwindow(winid window, boolean blocking)
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
Gem_destroy_nhwindow(winid window)
{
    if (window == WIN_ERR) /* test existence */
        panic(winpanicstr, window);

    mar_destroy_nhwindow(window);
}

extern void mar_curs(short, short); /* mar_curs is only for map */

void
Gem_curs(winid window, int x, int y)
{
    if (window == WIN_ERR) /* test existence */
        panic(winpanicstr, window);

    if (window == WIN_MAP)
        mar_curs(x - 1, y);
    else if (window == WIN_STATUS)
        curr_status_line = y;
}

extern void mar_add_status_str(const char *, short);
extern void mar_putstr_text(winid, short, const char *, short);

void
Gem_putstr(winid window, int attr, const char *str)
{
    short win_type;

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
    case NHW_TEXT: {
        /* If the line begins with a <x,y> map coordinate (used by
           upstream look_all() for "nearby monsters/objects"), grab
           the glyph at that position so mar_putstr_text can render
           a tile next to the text. */
        short tile = (short) NO_GLYPH;
        int xc, yc;
        if (sscanf(str, " <%d,%d>", &xc, &yc) == 2
            && xc >= 1 && xc < COLNO && yc >= 0 && yc < ROWNO) {
            int glyph = glyph_at((coordxy) xc, (coordxy) yc);
            glyph_info gi;
            map_glyphinfo((coordxy) xc, (coordxy) yc, glyph, 0, &gi);
            tile = (short) gi.gm.tileidx;
        }
        mar_putstr_text(window, attr, str, tile);
        break;
    }
    } /* endswitch win_type */
}

void
Gem_display_file(const char *fname, boolean complain)
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
Gem_add_menu(winid window, const glyph_info *glyphinfo,
             const ANY_P *identifier, char ch, char gch,
             int attr, int clr, const char *str, unsigned int itemflags)
{
    int glyph = glyphinfo ? glyphinfo->glyph : NO_GLYPH;
    Gem_menu_item *G_item;
    const char *newstr;
    char buf[4 + BUFSZ];

    if (str == (const char *) 0)
        return;

    if (window == WIN_ERR) /* test existence */
        panic(winpanicstr, window);

    if (identifier->a_void)
        Sprintf(buf, "%c - %s", ch ? ch : '?', str);
    else
        Sprintf(buf, "%s", str);
    newstr = buf;

    G_item = (Gem_menu_item *) alloc(sizeof(Gem_menu_item));
    G_item->Gmi_identifier = (long) identifier->a_void;
    /* nul_glyphinfo has glyph == NO_GLYPH but tileidx == 0; without this
       guard those header / separator rows would blit tile 0 in the tile
       column. */
    G_item->Gmi_glyph = (glyphinfo && glyphinfo->glyph != NO_GLYPH)
                            ? glyphinfo->gm.tileidx : NO_GLYPH;
    G_item->Gmi_count = -1L;
    G_item->Gmi_selected = (itemflags & MENU_ITEMFLAGS_SELECTED) ? 1 : 0;
    G_item->Gmi_accelerator = ch;
    G_item->Gmi_groupacc = gch;
    G_item->Gmi_attr = (short) attr;
    G_item->Gmi_color = (short) clr;
    G_item->Gmi_itemflags =
        (unsigned short) (itemflags & ~MENU_ITEMFLAGS_SELECTED);
    G_item->Gmi_str = dupstr(newstr);
    mar_add_menu(window, G_item);
}

/*
 * End a menu in this window, window must a type NHW_MENU.
 * We assign the keyboard accelerators as needed.
 */
void
Gem_end_menu(winid window, const char *prompt)
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
Gem_select_menu(winid window, int how, menu_item **menu_list)
{
    Gem_menu_item *Gmit;
    menu_item *mi;
    short n;

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
    } else if (mar_menu_cancelled()) {
        return -1;
    }

    return n;
}

/* native status handler: per-field colors/attributes and the hitpointbar;
   text buffers stay owned by genl_status_init/enablefield/finish */
extern char *status_vals[MAXBLSTATS]; /* from src/windows.c */
extern boolean status_activefields[MAXBLSTATS];
extern const char *status_fieldfmt[MAXBLSTATS];
extern void mar_add_status_line(const char *, const char *, const char *,
                                short);

static short gem_field_colors[MAXBLSTATS];
static unsigned long gem_cond_bits = 0L;
static unsigned long *gem_cond_colormasks = (unsigned long *) 0;
static int gem_hpbar_percent = 0, gem_hpbar_color = NO_COLOR;

static const enum statusfields gem_fieldorder[2][15] = {
    { BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_ALIGN,
      BL_SCORE, BL_FLUSH },
    { BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX, BL_AC,
      BL_XP, BL_EXP, BL_HD, BL_TIME, BL_HUNGER, BL_CAP, BL_CONDITION,
      BL_FLUSH },
};

static const struct {
    unsigned long mask;
    const char *text;
} gem_conds[] = {
    { BL_MASK_STONE, "Stone" },     { BL_MASK_SLIME, "Slime" },
    { BL_MASK_STRNGL, "Strngl" },   { BL_MASK_FOODPOIS, "FoodPois" },
    { BL_MASK_TERMILL, "TermIll" }, { BL_MASK_BLIND, "Blind" },
    { BL_MASK_DEAF, "Deaf" },       { BL_MASK_STUN, "Stun" },
    { BL_MASK_CONF, "Conf" },       { BL_MASK_HALLU, "Hallu" },
    { BL_MASK_LEV, "Lev" },         { BL_MASK_FLY, "Fly" },
    { BL_MASK_RIDE, "Ride" },
};

void
Gem_status_init(void)
{
    int i;

    for (i = 0; i < MAXBLSTATS; i++)
        gem_field_colors[i] = NO_COLOR;
    gem_cond_bits = 0L;
    gem_cond_colormasks = (unsigned long *) 0;
    gem_hpbar_percent = 0;
    gem_hpbar_color = NO_COLOR;
    genl_status_init();
}

static int
gem_condcolor(unsigned long bm, unsigned long *bmarray)
{
    int i;

    if (bm && bmarray)
        for (i = 0; i < CLR_MAX; i++)
            if (bmarray[i] & bm)
                return i;
    return NO_COLOR;
}

static int
gem_condattr(unsigned long bm, unsigned long *bmarray)
{
    static const int attrmap[] = { 0,       HL_BOLD,  HL_DIM,    HL_ITALIC,
                                   HL_ULINE, HL_BLINK, HL_INVERSE };
    int i, attr = 0;

    if (bm && bmarray)
        for (i = HL_ATTCLR_NONE; i < BL_ATTCLR_MAX; i++)
            if (bmarray[i] & bm)
                attr |= attrmap[i - HL_ATTCLR_NONE];
    return attr;
}

static void
gem_status_put(char *line, char *colors, char *attrs, int *n,
               const char *val, int color)
{
    int i;

    for (i = 0; val[i] && *n < MAXCO - 1; i++, (*n)++) {
        line[*n] = val[i];
        colors[*n] = (char) (color & 0x00ff);
        attrs[*n] = (char) ((color >> 8) & 0x00ff);
    }
}

static void
gem_status_lineout(int lineno)
{
    char line[MAXCO], colors[MAXCO], attrs[MAXCO];
    const char *val;
    int i, k, fld, n = 0;

    for (i = 0; (fld = gem_fieldorder[lineno][i]) != BL_FLUSH; i++) {
        if (!status_activefields[fld])
            continue;
        if (fld == BL_CONDITION) {
            for (k = 0; k < SIZE(gem_conds); k++)
                if (gem_cond_bits & gem_conds[k].mask) {
                    int cc =
                        gem_condcolor(gem_conds[k].mask, gem_cond_colormasks)
                        | (gem_condattr(gem_conds[k].mask,
                                        gem_cond_colormasks) << 8);

                    gem_status_put(line, colors, attrs, &n, " ", NO_COLOR);
                    gem_status_put(line, colors, attrs, &n,
                                   gem_conds[k].text, cc);
                }
            continue;
        }
        val = status_vals[fld];
        switch (fld) {
        case BL_HP:
        case BL_XP:
        case BL_HD:
        case BL_TIME:
            gem_status_put(line, colors, attrs, &n, " ", NO_COLOR);
            break;
        case BL_HUNGER:
            if (strcmp(val, " "))
                gem_status_put(line, colors, attrs, &n, " ", NO_COLOR);
            break;
        case BL_CAP:
            if (!strcmp(val, " "))
                ++val;
            break;
        default:
            break;
        }
        if (fld == BL_TITLE && iflags.wc2_hitpointbar) {
            int start = n, len, bar_pos;

            gem_status_put(line, colors, attrs, &n, val,
                           gem_field_colors[fld]);
            len = n - start;
            bar_pos = (len * gem_hpbar_percent) / 100;
            if (bar_pos < 1 && gem_hpbar_percent > 0)
                bar_pos = 1;
            if (bar_pos >= len && gem_hpbar_percent < 100)
                bar_pos = len - 1;
            for (k = 0; k < bar_pos; k++) {
                colors[start + k] = (char) gem_hpbar_color;
                attrs[start + k] |= HL_INVERSE;
            }
        } else {
            gem_status_put(line, colors, attrs, &n, val,
                           gem_field_colors[fld]);
        }
    }
    line[n] = '\0';
    mar_add_status_line(line, colors, attrs, (short) lineno);
}

void
Gem_status_update(int fldidx, genericptr_t ptr, int chg UNUSED, int percent,
                  int color, unsigned long *colormasks)
{
    char goldbuf[BUFSZ], *text = (char *) ptr;

    if (fldidx >= 0 && fldidx < MAXBLSTATS) {
        if (!status_activefields[fldidx])
            return;
        if (fldidx == BL_CONDITION) {
            gem_cond_bits = ptr ? (unsigned long) *(long *) ptr : 0L;
            gem_cond_colormasks = colormasks;
            return;
        }
        if (fldidx == BL_GOLD && text)
            text = decode_mixed(goldbuf, text);
        Sprintf(status_vals[fldidx],
                status_fieldfmt[fldidx] ? status_fieldfmt[fldidx] : "%s",
                text ? text : "");
        gem_field_colors[fldidx] = (short) color;
        if (fldidx == BL_HP && iflags.wc2_hitpointbar) {
            gem_hpbar_percent = percent;
            gem_hpbar_color = color & 0x00ff;
        }
    } else if (fldidx == BL_FLUSH || fldidx == BL_RESET) {
        mar_status_dirty();
        gem_status_lineout(0);
        gem_status_lineout(1);
        mar_display_nhwindow(WIN_STATUS);
    }
}

void
Gem_update_inventory(int arg)
{
    nhUse(arg);
}

void
Gem_mark_synch(void)
{
    mar_display_nhwindow(WIN_MESSAGE);
    mar_display_nhwindow(WIN_MAP);
    mar_display_nhwindow(WIN_STATUS);
}

void
Gem_wait_synch(void)
{
    mar_display_nhwindow(WIN_MESSAGE);
    mar_display_nhwindow(WIN_MAP);
    mar_display_nhwindow(WIN_STATUS);
}

#ifdef CLIPPING
extern void mar_cliparound(void);
void
Gem_cliparound(int x, int y)
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

void mar_print_gl_char(winid, coordxy, coordxy, int, int);

extern short mar_set_rogue(short);

extern void mar_add_pet_sign(winid, short, short);

void
Gem_print_glyph(winid window, coordxy x, coordxy y,
                const glyph_info *glyphinfo, const glyph_info *bkglyphinfo)
{
    int glyph = glyphinfo->glyph;

    /* Move the cursor. */
    Gem_curs(window, x, y);

    mar_set_rogue(Is_rogue_level(&u.uz) ? TRUE : FALSE);

    x--; /* x ranges from 1 to COLNO */
    if (mar_set_tile_mode(-1)) {
        mar_print_glyph(window, x, y, glyphinfo->gm.tileidx,
                         bkglyphinfo->gm.tileidx);
        if (
#ifdef TEXTCOLOR
            iflags.hilite_pet &&
#endif
            glyph_is_pet(glyph))
            mar_add_pet_sign(window, x, y);
    } else
        mar_print_gl_char(window, x, y, glyphinfo->ttychar,
                           glyphinfo->gm.sym.color);
}

void mar_print_char(winid, coordxy, coordxy, char, short);

void
mar_print_gl_char(winid window, coordxy x, coordxy y, int ch, int color)
{
#ifdef TEXTCOLOR
    /* Turn off color if rogue level. */
    if (Is_rogue_level(&u.uz))
        color = NO_COLOR;
#endif /* TEXTCOLOR */

    /* NetHack 5.0 may set NH_BASIC_COLOR (0x1000000) or NH_ALTPALETTE
       (0x2000000) flags on the colour value, with the basic CLR_* index
       or a 24-bit RGB in the low bits.  We only handle the basic-colour
       form here: strip flags, keep the low 8 bits.  Out-of-range values
       become NO_COLOR and fall through to the default pen. */
    color = color & 0xFF;
    if (color >= 16)
        color = NO_COLOR;

    mar_print_char(window, x, y, ch, (short) color);
}

extern void mar_raw_print(const char *);
extern void mar_raw_print_bold(const char *);

/* Pre-init raw_print buffer.  Before iflags.window_inited becomes TRUE
   the only output channel is stdout, which under MiNT bounces through
   the kernel console device and the AES console host (TosWin2 /
   N.AES console) per character -- noticeably slow on startup.  Stash
   such messages here and surface them via xalert() once the AES is
   up; cap the buffer so a runaway pre-init raw_print storm can't pin
   the process. */
#define PREINIT_BUF_SIZE 1024
static char preinit_raw_buf[PREINIT_BUF_SIZE];
static int preinit_raw_len = 0;
static int preinit_raw_overflow = 0;

static void
preinit_raw_append(const char *str)
{
    int n, room;
    if (!str)
        return;
    n = (int) strlen(str);
    room = PREINIT_BUF_SIZE - 1 - preinit_raw_len;
    if (n + 1 > room) {
        preinit_raw_overflow = 1;
        if (room <= 1)
            return;
        n = room - 1;
    }
    memcpy(preinit_raw_buf + preinit_raw_len, str, n);
    preinit_raw_len += n;
    preinit_raw_buf[preinit_raw_len++] = '\n';
    preinit_raw_buf[preinit_raw_len] = '\0';
}

/* after the windows are gone (top-ten list, panic text) fall back to
   stdout; msexit()'s desktop pause keeps it readable */
void
Gem_raw_print(const char *str)
{
    if (str && *str) {
        if (iflags.window_inited)
            mar_raw_print(str);
        else if (gem_windows_exited)
            printf("%s\n", str);
        else
            preinit_raw_append(str);
    }
}

void
Gem_raw_print_bold(const char *str)
{
    if (str && *str) {
        if (iflags.window_inited)
            mar_raw_print_bold(str);
        else if (gem_windows_exited)
            printf("%s\n", str);
        else
            preinit_raw_append(str);
    }
}

/* surface pre-init lines that never reached the message window */
static void
preinit_raw_dump_console(void)
{
    if (preinit_raw_len > 0)
        fputs(preinit_raw_buf, stdout);
    preinit_raw_len = 0;
    preinit_raw_overflow = 0;
    preinit_raw_buf[0] = '\0';
}

/* Called from Gem_init_nhwindows right after iflags.window_inited
   becomes TRUE: drop the collected lines into the message window
   instead of a modal raw_print alert. */
extern void mar_add_message(const char *); /* wingem1.c */
extern void mar_display_nhwindow(winid);   /* wingem1.c */
void
Gem_flush_preinit_raw(void)
{
    char *p, *eol;

    if (preinit_raw_len <= 0)
        return;

    p = preinit_raw_buf;
    while (p < preinit_raw_buf + preinit_raw_len) {
        eol = strchr(p, '\n');
        if (eol)
            *eol = '\0';
        if (*p)
            mar_add_message(p);
        if (!eol)
            break;
        p = eol + 1;
    }
    if (preinit_raw_overflow)
        mar_add_message("[startup messages truncated]");
    if (WIN_MESSAGE != WIN_ERR)
        mar_display_nhwindow(WIN_MESSAGE);

    preinit_raw_len = 0;
    preinit_raw_overflow = 0;
    preinit_raw_buf[0] = '\0';
}

extern void mar_update_value(void); /* wingem1.c */
extern short mar_nh_poskey(short *x, short *y, short *mod); /* wingem1.c */

int
Gem_nhgetch()
{
    short i, x, y, mod;

    mar_update_value();
    i = mar_nh_poskey(&x, &y, &mod);
    if (!i)
        i = '\033'; /* mouse click during a getch prompt -> ESC */

    return i;
}

/* Curated set of extended commands shown in the GEM "#" menu, in
   alphabetical order with single-letter accelerators (case-sensitive).
   Based on NetHack 3.4.3, with new 3.7 commands added.  Each entry is
   resolved to its current extcmdlist index at runtime; commands that
   no longer exist (or that are gated off by build flags) are simply
   skipped. */
static const struct {
    const char *name;
    char accelerator;
} gem_ext_menu[] = {
    { "adjust",     'a' },
    { "annotate",   'A' },
    { "chat",       'c' },
    { "chronicle",  'h' },
    { "conduct",    'C' },
    { "dip",        'd' },
    { "enhance",    'e' },
    { "force",      'f' },
    { "genocided",  'g' },
    { "invoke",     'i' },
    { "jump",       'j' },
    { "loot",       'l' },
    { "monster",    'm' },
    { "name",       'n' },
    { "offer",      'o' },
    { "overview",   'O' },
    { "pray",       'p' },
    { "quit",       'q' },
    { "ride",       'r' },
    { "rub",        'R' },
    { "sit",        's' },
    { "terrain",    'L' },
    { "tip",        'P' },
    { "turn",       't' },
    { "twoweapon",  'T' },
    { "untrap",     'u' },
    { "vanquished", 'V' },
    { "version",    'v' },
    { "wipe",       'w' },
    { "?",          '?' },
};

/* extcmdlist index of the which-th prefix match, ordering AUTOCOMPLETE
   commands before the rest and wrapping modulo the match count (so a
   negative which cycles backward).  -1 if there is no match. */
static int
extcmds_match_cycle(const char *prefix, int which)
{
    int *all;
    int i, o, k, n_ac = 0;
    int n_all = extcmds_match(prefix, ECM_IGNOREAC, &all);

    if (n_all <= 0)
        return -1;
    for (i = 0; i < n_all; i++)
        if (extcmdlist[all[i]].flags & AUTOCOMPLETE)
            n_ac++;
    o = ((which % n_all) + n_all) % n_all;
    if (o < n_ac) {
        for (i = 0, k = 0; i < n_all; i++)
            if ((extcmdlist[all[i]].flags & AUTOCOMPLETE) && k++ == o)
                return all[i];
    } else {
        o -= n_ac;
        for (i = 0, k = 0; i < n_all; i++)
            if (!(extcmdlist[all[i]].flags & AUTOCOMPLETE) && k++ == o)
                return all[i];
    }
    return -1; /* not reached */
}

/* fill out[] with the which-th tab-completion of prefix (AUTOCOMPLETE
   commands first, wrapping); returns 0 on success, -1 if no match.
   bridges extcmd data to the GEM-side prompt in wingem1.c. */
int
gem_ext_complete_next(const char *prefix, int which, char *out, int outsz)
{
    int idx = extcmds_match_cycle(prefix, which);

    if (idx < 0)
        return -1;
    (void) strncpy(out, extcmdlist[idx].ef_txt, outsz - 1);
    out[outsz - 1] = '\0';
    return 0;
}

/* the curated menu picker, reached from Gem_get_ext_cmd when the player
   types '?' at the text prompt */
static int
gem_ext_cmd_menu(void)
{
    winid wind;
    int i, idx, count, what = -1;
    menu_item *selected = NULL;
    anything any;
    boolean show_all = FALSE;

    wind = Gem_create_nhwindow(NHW_MENU);
    Gem_start_menu(wind, MENU_BEHAVE_STANDARD);
    for (i = 0; i < (int) (sizeof gem_ext_menu / sizeof gem_ext_menu[0]);
         i++) {
        if (gem_ext_menu[i].name[0] == '?' && !gem_ext_menu[i].name[1]) {
            /* Sentinel: select to open the rest of extcmdlist as a
               selectable menu (commands not already shown above). */
            any.a_int = -1;
            Gem_add_menu(wind, &nul_glyphinfo, &any, '?', 0, ATR_NONE,
                         NO_COLOR, "(more commands...)",
                         MENU_ITEMFLAGS_NONE);
            continue;
        }
        {
            int *m;

            if (extcmds_match(gem_ext_menu[i].name,
                              ECM_IGNOREAC | ECM_EXACTMATCH, &m) != 1)
                continue; /* not present / not available in this build */
            idx = m[0];
        }
        any.a_int = idx + 1; /* +1 so identifier is non-zero */
        Gem_add_menu(wind, &nul_glyphinfo, &any,
                     gem_ext_menu[i].accelerator, 0, ATR_NONE, NO_COLOR,
                     extcmdlist[idx].ef_txt, MENU_ITEMFLAGS_NONE);
    }
    Gem_end_menu(wind, "What extended command?");
    count = Gem_select_menu(wind, PICK_ONE, &selected);
    if (count > 0) { /* -1 means cancelled, selected stays NULL */
        if (selected->item.a_int == -1)
            show_all = TRUE;
        else
            what = selected->item.a_int - 1;
        free((genericptr_t) selected);
        selected = NULL;
    }
    Gem_destroy_nhwindow(wind);

    if (show_all) {
        wind = Gem_create_nhwindow(NHW_MENU);
        Gem_start_menu(wind, MENU_BEHAVE_STANDARD);
        for (idx = 0; extcmdlist[idx].ef_txt; idx++) {
            boolean curated = FALSE;

            if ((extcmdlist[idx].flags
                 & (CMD_NOT_AVAILABLE | INTERNALCMD)) != 0)
                continue;
            if (!wizard && (extcmdlist[idx].flags & WIZMODECMD) != 0)
                continue;
            /* Skip anything that's already on the curated menu. */
            for (i = 0;
                 i < (int)(sizeof gem_ext_menu / sizeof gem_ext_menu[0]);
                 i++) {
                if (!strcmp(extcmdlist[idx].ef_txt, gem_ext_menu[i].name)) {
                    curated = TRUE;
                    break;
                }
            }
            if (curated)
                continue;
            /* Pass 0 as the accelerator and let mar_set_accelerators()
               auto-assign unique a-z,A-Z keys.  Passing extcmdlist[idx].key
               directly would either inject garbage (M(x), C(x)) or collide
               with the auto-assigned letters and cause the dialog handler
               to close itself on freed state. */
            any.a_int = idx + 1;
            Gem_add_menu(wind, &nul_glyphinfo, &any,
                         0, 0, ATR_NONE, NO_COLOR,
                         extcmdlist[idx].ef_txt, MENU_ITEMFLAGS_NONE);
        }
        Gem_end_menu(wind, "Pick a command:");
        count = Gem_select_menu(wind, PICK_ONE, &selected);
        if (count > 0) {
            what = selected->item.a_int - 1;
            free((genericptr_t) selected);
        }
        Gem_destroy_nhwindow(wind);
    }

    return what;
}

/* Get an extended command.  Runs a text prompt with TAB completion; the
   player can type '?' to fall back to the curated menu. */
int
Gem_get_ext_cmd(void)
{
    char buf[BUFSZ];
    int *m;

    switch (gem_ext_cmd_getlin(buf)) {
    case 1:
        return gem_ext_cmd_menu();
    case -1:
        return -1;
    default:
        break;
    }

    (void) mungspaces(buf);
    if (buf[0] == '\0')
        return -1;
    if (extcmds_match(buf, ECM_IGNOREAC | ECM_EXACTMATCH, &m) == 1)
        return m[0];
    if (extcmds_match(buf, ECM_NOFLAGS, &m) == 1)
        return m[0];
    pline("#%.60s: unknown extended command.", buf);
    return -1;
}

void
Gem_number_pad(int state)
{
    (void)state;
}

void
win_Gem_init(int dir)
{
    nhUse(dir);
}

win_request_info *
Gem_ctrl_nhwindow(winid window, int request, win_request_info *wri)
{
    return (win_request_info *) 0;
}

#ifdef POSITIONBAR
void
Gem_update_positionbar(char *posbar)
{
}
#endif

/** Gem_outrip **/
void mar_set_text_to_rip(winid);
char **rip_line = 0;

void
Gem_outrip(winid w, int how, time_t when)
{
/* Code from X11 windowport */
#define STONE_LINE_LEN 15 /* # chars that fit on one line */
#define NAME_LINE 0       /* line # for player name */
#define GOLD_LINE 1       /* line # for amount of gold */
#define DEATH_LINE 2      /* line # for death description */
#define YEAR_LINE 6       /* line # for year */
    char buf[BUFSZ];
    char *dpx;
    short line;
    long year;

    if (!rip_line) {
        short i;
        rip_line = (char **) alloc((YEAR_LINE + 1) * sizeof(char *));
        for (i = 0; i < YEAR_LINE + 1; i++) {
            rip_line[i] =
                (char *) alloc((STONE_LINE_LEN + 1) * sizeof(char));
        }
    }
    /* Follows same algorithm as genl_outrip() */
    /* Put name on stone */
    Sprintf(rip_line[NAME_LINE], "%.*s", STONE_LINE_LEN, svp.plname);
    /* Put $ on stone */
    Sprintf(rip_line[GOLD_LINE], "%ld Au", gd.done_money);
    /* Put together death description */
    formatkiller(buf, sizeof buf, how, FALSE);

    /* Put death type on stone */
    for (line = DEATH_LINE, dpx = buf; line < YEAR_LINE; line++) {
        register short i, i0;
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
mar_get_font(short type, char **p_fname, short *psize)
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
Gem_preference_update(const char *pref)
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

#endif /* GEM_GRAPHICS */

/*wingem.c*/

/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.7 cursinit.c */
/* Copyright (c) Karl Garrison, 2010. */
/* NetHack may be freely redistributed.  See license for details. */

#include "curses.h"
#include "hack.h"
#include "wincurs.h"
#include "cursinit.h"

#include <ctype.h>

/* Initialization and startup functions for curses interface */

/* Private declarations */

static void set_window_position(int *, int *, int *, int *, int,
                                int *, int *, int *, int *, int,
                                int, int);

#if 0   /* no longer used */
typedef struct nhrgb_type {
    short r;
    short g;
    short b;
} nhrgb;
#endif

/* Banners used for an optional ASCII splash screen */

#define NETHACK_SPLASH_A \
" _   _        _    _    _               _    "

#define NETHACK_SPLASH_B \
"| \\ | |      | |  | |  | |             | |   "

#define NETHACK_SPLASH_C \
"|  \\| |  ___ | |_ | |__| |  __ _   ___ | | __"

#define NETHACK_SPLASH_D \
"| . ` | / _ \\| __||  __  | / _` | / __|| |/ /"

#define NETHACK_SPLASH_E \
"| |\\  ||  __/| |_ | |  | || (_| || (__ |   < "

#define NETHACK_SPLASH_F \
"|_| \\_| \\___| \\__||_|  |_| \\__,_| \\___||_|\\_\\"


/* win* is size and placement of window to change, x/y/w/h is baseline
   which can decrease depending on alignment of win* in orientation.
   Negative minh/minw: as much as possible, but at least as much as
   specified. */
static void
set_window_position(int *winx, int *winy, int *winw, int *winh,
                    int orientation,
                    int *x, int *y, int *w, int *h, int border_space,
                    int minh, int minw)
{
    *winw = *w;
    *winh = *h;

    /* Set window height/width */
    if (orientation == ALIGN_TOP || orientation == ALIGN_BOTTOM) {
        if (minh < 0) {
            *winh = (*h - ROWNO - border_space);
            if (-minh > *winh)
                *winh = -minh;
        } else
            *winh = minh;
        *h -= (*winh + border_space);
    } else {
        if (minw < 0) {
            *winw = (*w - COLNO - border_space);
            if (-minw > *winw)
                *winw = -minw;
        } else
            *winw = minw;
        *w -= (*winw + border_space);
    }

    *winx = *w + border_space + *x;
    *winy = *h + border_space + *y;

    /* Set window position */
    if (orientation != ALIGN_RIGHT) {
        *winx = *x;
        if (orientation == ALIGN_LEFT)
            *x += *winw + border_space;
    }
    if (orientation != ALIGN_BOTTOM) {
        *winy = *y;
        if (orientation == ALIGN_TOP)
            *y += *winh + border_space;
    }
}

/* Create the "main" nonvolatile windows used by nethack */
void
curses_create_main_windows(void)
{
    int min_message_height UNUSED = 1;
    int message_orientation = 0;
    int status_orientation = 0;
    int border_space = 0;
    int hspace = term_cols - 80;
    boolean borders = FALSE, noperminv_borders = FALSE;

    switch (iflags.wc2_windowborders) {
    default:
    case 0:                     /* Off */
        borders = FALSE;
        break;

    case 3:
        noperminv_borders = TRUE;
        /*FALLTHRU*/
    case 1:                     /* On */
        borders = TRUE;
        break;

    case 4:
        noperminv_borders = TRUE;
        /*FALLTHRU*/
    case 2:                     /* Auto */
        borders = (term_cols >= 80 + 2 && term_rows >= 24 + 2);
        break;
    }

    if (borders) {
        border_space = 2;
        hspace -= border_space;
    }

    if ((term_cols - border_space) < COLNO) {
        min_message_height++;
    }

    /* Determine status window orientation */
    if (!iflags.wc_align_status)
        iflags.wc_align_status = ALIGN_BOTTOM;
    if (iflags.wc_align_status == ALIGN_TOP
        || iflags.wc_align_status == ALIGN_BOTTOM) {
        status_orientation = iflags.wc_align_status;
    } else { /* left or right alignment */
        /*
         * Max space for player name and title horizontally.
         * [Width of 26 gives enough room for a 24 character
         * hitpoint bar (horizontal layout uses 30 for that) and
         * can accommodate widest field ("Experience  : 30/123456789")
         * other than title without truncating anything.
         * Height originally required at least 24 lines, but 21
         * suffices and 20 can be made to work by suppressing score.]
         */
        if (hspace >= 26 && term_rows >= 20) {
            status_orientation = iflags.wc_align_status;
            hspace -= (26 + border_space);
        } else {
            /* orientation won't match option setting, making 'O' command's
               list of settings seem inaccurate; but leaving the requested
               setting in iflags might allow it to take effect if the main
               window gets resized */
            status_orientation = ALIGN_BOTTOM;
        }
    }

    /* Determine message window orientation */
    if (!iflags.wc_align_message)
        iflags.wc_align_message = ALIGN_TOP;
    if (iflags.wc_align_message == ALIGN_TOP
        || iflags.wc_align_message == ALIGN_BOTTOM) {
        message_orientation = iflags.wc_align_message;
    } else { /* left or right alignment */
        if ((hspace - border_space) >= 25) {    /* Arbitrary */
            message_orientation = iflags.wc_align_message;
        } else {
            /* orientation won't match option setting (see above) */
            message_orientation = ALIGN_TOP;
        }
    }

    /* Figure out window positions and placements. Status and message area
       can be aligned based on configuration. The priority alignment-wise
       is: status > msgarea > game.
       Define everything as taking as much space as possible and shrink/move
       based on alignment positions. */
    {
        int message_x = 0;
        int message_y = 0;
        int status_x = 0;
        int status_y = 0;
        int inv_x = 0;
        int inv_y = 0;
        int map_x = 0;
        int map_y = 0;
        int message_height = 0;
        int message_width = 0;
        int status_height = 0;
        int status_width = 0;
        int inv_height = 0;
        int inv_width = 0;
        int map_height = (term_rows - border_space);
        int map_width = (term_cols - border_space);
        int statusheight = (iflags.wc2_statuslines < 3) ? 2 : 3;
        boolean status_vertical = (status_orientation == ALIGN_LEFT
                                   || status_orientation == ALIGN_RIGHT);
        boolean msg_vertical = (message_orientation == ALIGN_LEFT
                                || message_orientation == ALIGN_RIGHT);

        /* Vertical windows have priority. Otherwise, priotity is:
           status > inv > msg */
        if (status_vertical)
            set_window_position(&status_x, &status_y,
                                &status_width, &status_height,
                                status_orientation,
                                &map_x, &map_y, &map_width, &map_height,
                                border_space, 20, 26);

        if (iflags.perm_invent) {
            /* Take up all width unless msgbar is also vertical. */
            int width = msg_vertical ? 25 : -25;

            set_window_position(&inv_x, &inv_y, &inv_width, &inv_height,
                                ALIGN_RIGHT, &map_x, &map_y,
                                &map_width, &map_height,
                                border_space, -1, width);
            /* suppress borders on perm_invent window, part I */
            if (noperminv_borders)
                inv_width += border_space, inv_height += border_space; /*+=2*/
        }

        if (msg_vertical)
            set_window_position(&message_x, &message_y,
                                &message_width, &message_height,
                                message_orientation,
                                &map_x, &map_y, &map_width, &map_height,
                                border_space, -1, -25);

        /* Now draw horizontal windows */
        if (!status_vertical)
            set_window_position(&status_x, &status_y,
                                &status_width, &status_height,
                                status_orientation,
                                &map_x, &map_y, &map_width, &map_height,
                                border_space, statusheight, 0);

        if (!msg_vertical)
            set_window_position(&message_x, &message_y,
                                &message_width, &message_height,
                                message_orientation,
                                &map_x, &map_y, &map_width, &map_height,
                                border_space, -1, -25);

        if (map_width > COLNO)
            map_width = COLNO;
        if (map_height > ROWNO)
            map_height = ROWNO;

        if (curses_get_nhwin(STATUS_WIN)) {
            curses_del_nhwin(STATUS_WIN);
            /* 'count window' overlays last line of mesg win;
               asking it to display a Null string removes it */
            curses_count_window((char *) 0);
            curses_del_nhwin(MESSAGE_WIN);
            curses_del_nhwin(MAP_WIN);
            curses_del_nhwin(INV_WIN);
            clear();
        }

        curses_add_nhwin(STATUS_WIN, status_height, status_width, status_y,
                         status_x, status_orientation, borders);

        curses_add_nhwin(MESSAGE_WIN, message_height, message_width, message_y,
                         message_x, message_orientation, borders);

        if (iflags.perm_invent)
            curses_add_nhwin(INV_WIN, inv_height, inv_width, inv_y, inv_x,
                             ALIGN_RIGHT,
                             /* suppress perm_invent borders, part II */
                             borders && !noperminv_borders);

        curses_add_nhwin(MAP_WIN, map_height, map_width,
                         map_y, map_x, 0, borders);

        refresh();

        curses_refresh_nethack_windows();
    /*
        if (iflags.window_inited) {
            curses_update_stats();
            if (iflags.perm_invent)
                curses_update_inventory();
        } else {
            iflags.window_inited = TRUE;
        }
    */
    }
}

/* Initialize curses colors to colors used by NetHack */
void
curses_init_nhcolors(void)
{
#ifdef TEXTCOLOR
    if (has_colors()) {
        use_default_colors();
        init_pair(1, COLOR_BLACK, -1);
        init_pair(2, COLOR_RED, -1);
        init_pair(3, COLOR_GREEN, -1);
        init_pair(4, COLOR_YELLOW, -1);
        init_pair(5, COLOR_BLUE, -1);
        init_pair(6, COLOR_MAGENTA, -1);
        init_pair(7, COLOR_CYAN, -1);
        init_pair(8, -1, -1);

        {
            int i;
            boolean hicolor = FALSE;

            static const int clr_remap[16] = {
                COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
                COLOR_BLUE,
                COLOR_MAGENTA, COLOR_CYAN, -1, COLOR_WHITE,
                COLOR_RED + 8, COLOR_GREEN + 8, COLOR_YELLOW + 8,
                COLOR_BLUE + 8,
                COLOR_MAGENTA + 8, COLOR_CYAN + 8, COLOR_WHITE + 8
            };

            for (i = 0; i < (COLORS >= 16 ? 16 : 8); i++) {
                init_pair(17 + (i * 2) + 0, clr_remap[i], COLOR_RED);
                init_pair(17 + (i * 2) + 1, clr_remap[i], COLOR_BLUE);
            }

            if (COLORS >= 16)
                hicolor = TRUE;

            /* Work around the crazy definitions above for more background
               colors... */
            for (i = 0; i < (COLORS >= 16 ? 16 : 8); i++) {
                init_pair((hicolor ? 49 : 9) + i, clr_remap[i], COLOR_GREEN);
                init_pair((hicolor ? 65 : 33) + i, clr_remap[i], COLOR_YELLOW);
                init_pair((hicolor ? 81 : 41) + i, clr_remap[i], COLOR_MAGENTA);
                init_pair((hicolor ? 97 : 49) + i, clr_remap[i], COLOR_CYAN);
                init_pair((hicolor ? 113 : 57) + i, clr_remap[i], COLOR_WHITE);
            }
        }


        if (COLORS >= 16) {
# ifdef USE_DARKGRAY
            if (iflags.wc2_darkgray) {
                init_pair(1, COLOR_BLACK + 8, -1);
            }
# endif
            init_pair(9, COLOR_WHITE, -1);
            init_pair(10, COLOR_RED + 8, -1);
            init_pair(11, COLOR_GREEN + 8, -1);
            init_pair(12, COLOR_YELLOW + 8, -1);
            init_pair(13, COLOR_BLUE + 8, -1);
            init_pair(14, COLOR_MAGENTA + 8, -1);
            init_pair(15, COLOR_CYAN + 8, -1);
            init_pair(16, COLOR_WHITE + 8, -1);
        }
    }
#endif
}

#if 0   /* curses_choose_character + curses_character_dialog no longer used */

/* Allow player to pick character's role, race, gender, and alignment.
   Borrowed from the Gnome window port. */
void
curses_choose_character(void)
{
    int n, i, sel, count_off, pick4u;
    int count = 0;
    int cur_character = 0;
    const char **choices;
    int *pickmap;
    char *prompt;
    char pbuf[QBUFSZ];
    char choice[QBUFSZ];
    char tmpchoice[QBUFSZ];

    prompt = build_plselection_prompt(pbuf, QBUFSZ, flags.initrole,
                                      flags.initrace, flags.initgend,
                                      flags.initalign);

    /* This part is irritating: we have to strip the choices off of
       the string and put them in a separate string in order to use
       curses_character_input_dialog for this prompt. */
    while (cur_character != '[') {
        cur_character = prompt[count];
        count++;
    }

    count_off = count;

    while (cur_character != ']') {
        tmpchoice[count - count_off] = prompt[count];
        count++;
        cur_character = prompt[count];
    }

    tmpchoice[count - count_off] = '\0';
    lcase(tmpchoice);

    while (!isspace(prompt[count_off])) {
        count_off--;
    }

    prompt[count_off] = '\0';
    Snprintf(choice, sizeof(choice), "%s%c", tmpchoice, '\033');
    if (strchr(tmpchoice, 't')) {       /* Tutorial mode */
        mvaddstr(0, 1, "New? Press t to enter a tutorial.");
    }

    /* Add capital letters as choices that aren't displayed */
    for (count = 0; tmpchoice[count]; count++) {
        tmpchoice[count] = toupper(tmpchoice[count]);
    }

    strcat(choice, tmpchoice);

    /* prevent an unnecessary prompt */
    rigid_role_checks();

    if (!flags.randomall &&
        (flags.initrole == ROLE_NONE || flags.initrace == ROLE_NONE ||
         flags.initgend == ROLE_NONE || flags.initalign == ROLE_NONE)) {
        pick4u = tolower(curses_character_input_dialog(prompt, choice, 'y'));
    } else {
        pick4u = 'y';
    }

    if (pick4u == 'q') {        /* Quit or cancelled */
        clearlocks();
        curses_bail(0);
    }

    if (pick4u == 'y') {
        flags.randomall = TRUE;
    }

    clear();
    refresh();

    if (!flags.randomall && flags.initrole < 0) {
        /* select a role */
        for (n = 0; roles[n].name.m; n++)
            continue;
        choices = (const char **) alloc(sizeof (char *) * (n + 1));
        pickmap = (int *) alloc(sizeof (int) * (n + 1));
        for (;;) {
            for (n = 0, i = 0; roles[i].name.m; i++) {
                if (ok_role(i, flags.initrace,
                            flags.initgend, flags.initalign)) {
                    if (flags.initgend >= 0 && flags.female && roles[i].name.f)
                        choices[n] = roles[i].name.f;
                    else
                        choices[n] = roles[i].name.m;
                    pickmap[n++] = i;
                }
            }
            if (n > 0)
                break;
            else if (flags.initalign >= 0)
                flags.initalign = -1;   /* reset */
            else if (flags.initgend >= 0)
                flags.initgend = -1;
            else if (flags.initrace >= 0)
                flags.initrace = -1;
            else
                panic("no available ROLE+race+gender+alignment combinations");
        }
        choices[n] = (const char *) 0;
        if (n > 1)
            sel = curses_character_dialog(choices,
                                        "Choose one of the following roles:");
        else
            sel = 0;
        if (sel >= 0)
            sel = pickmap[sel];
        else if (sel == ROLE_NONE) {    /* Quit */
            clearlocks();
            curses_bail(0);
        }
        free((genericptr_t) choices);
        free((genericptr_t) pickmap);
    } else if (flags.initrole < 0)
        sel = ROLE_RANDOM;
    else
        sel = flags.initrole;

    if (sel == ROLE_RANDOM) {   /* Random role */
        sel = pick_role(flags.initrace, flags.initgend,
                        flags.initalign, PICK_RANDOM);
        if (sel < 0)
            sel = randrole(FALSE);
    }

    flags.initrole = sel;

    /* Select a race, if necessary */
    /* force compatibility with role, try for compatibility with
     * pre-selected gender/alignment */
    if (flags.initrace < 0 || !validrace(flags.initrole, flags.initrace)) {
        if (flags.initrace == ROLE_RANDOM || flags.randomall) {
            flags.initrace = pick_race(flags.initrole, flags.initgend,
                                       flags.initalign, PICK_RANDOM);
            if (flags.initrace < 0)
                flags.initrace = randrace(flags.initrole);
        } else {
            /* Count the number of valid races */
            n = 0;              /* number valid */
            for (i = 0; races[i].noun; i++) {
                if (ok_race(flags.initrole, i,
                            flags.initgend, flags.initalign))
                    n++;
            }
            if (n == 0) {
                for (i = 0; races[i].noun; i++) {
                    if (validrace(flags.initrole, i))
                        n++;
                }
            }

            choices = (const char **) alloc(sizeof (char *) * (n + 1));
            pickmap = (int *) alloc(sizeof (int) * (n + 1));
            for (n = 0, i = 0; races[i].noun; i++) {
                if (ok_race(flags.initrole, i,
                            flags.initgend, flags.initalign)) {
                    choices[n] = races[i].noun;
                    pickmap[n++] = i;
                }
            }
            choices[n] = (const char *) 0;
            /* Permit the user to pick, if there is more than one */
            if (n > 1)
                sel = curses_character_dialog(choices,
                                        "Choose one of the following races:");
            else
                sel = 0;
            if (sel >= 0)
                sel = pickmap[sel];
            else if (sel == ROLE_NONE) {        /* Quit */
                clearlocks();
                curses_bail(0);
            }
            flags.initrace = sel;
            free((genericptr_t) choices);
            free((genericptr_t) pickmap);
        }
        if (flags.initrace == ROLE_RANDOM) {    /* Random role */
            sel = pick_race(flags.initrole, flags.initgend,
                            flags.initalign, PICK_RANDOM);
            if (sel < 0)
                sel = randrace(flags.initrole);
            flags.initrace = sel;
        }
    }

    /* Select a gender, if necessary */
    /* force compatibility with role/race, try for compatibility with
     * pre-selected alignment */
    if (flags.initgend < 0 ||
        !validgend(flags.initrole, flags.initrace, flags.initgend)) {
        if (flags.initgend == ROLE_RANDOM || flags.randomall) {
            flags.initgend = pick_gend(flags.initrole, flags.initrace,
                                       flags.initalign, PICK_RANDOM);
            if (flags.initgend < 0)
                flags.initgend = randgend(flags.initrole, flags.initrace);
        } else {
            /* Count the number of valid genders */
            n = 0;              /* number valid */
            for (i = 0; i < ROLE_GENDERS; i++) {
                if (ok_gend(flags.initrole, flags.initrace,
                            i, flags.initalign))
                    n++;
            }
            if (n == 0) {
                for (i = 0; i < ROLE_GENDERS; i++) {
                    if (validgend(flags.initrole, flags.initrace, i))
                        n++;
                }
            }

            choices = (const char **) alloc(sizeof (char *) * (n + 1));
            pickmap = (int *) alloc(sizeof (int) * (n + 1));
            for (n = 0, i = 0; i < ROLE_GENDERS; i++) {
                if (ok_gend(flags.initrole, flags.initrace,
                            i, flags.initalign)) {
                    choices[n] = genders[i].adj;
                    pickmap[n++] = i;
                }
            }
            choices[n] = (const char *) 0;
            /* Permit the user to pick, if there is more than one */
            if (n > 1)
                sel = curses_character_dialog(choices,
                                      "Choose one of the following genders:");
            else
                sel = 0;
            if (sel >= 0)
                sel = pickmap[sel];
            else if (sel == ROLE_NONE) {        /* Quit */
                clearlocks();
                curses_bail(0);
            }
            flags.initgend = sel;
            free((genericptr_t) choices);
            free((genericptr_t) pickmap);
        }
        if (flags.initgend == ROLE_RANDOM) {    /* Random gender */
            sel = pick_gend(flags.initrole, flags.initrace,
                            flags.initalign, PICK_RANDOM);
            if (sel < 0)
                sel = randgend(flags.initrole, flags.initrace);
            flags.initgend = sel;
        }
    }

    /* Select an alignment, if necessary */
    /* force compatibility with role/race/gender */
    if (flags.initalign < 0 ||
        !validalign(flags.initrole, flags.initrace, flags.initalign)) {
        if (flags.initalign == ROLE_RANDOM || flags.randomall) {
            flags.initalign = pick_align(flags.initrole, flags.initrace,
                                         flags.initgend, PICK_RANDOM);
            if (flags.initalign < 0)
                flags.initalign = randalign(flags.initrole, flags.initrace);
        } else {
            /* Count the number of valid alignments */
            n = 0;              /* number valid */
            for (i = 0; i < ROLE_ALIGNS; i++) {
                if (ok_align(flags.initrole, flags.initrace,
                             flags.initgend, i))
                    n++;
            }
            if (n == 0) {
                for (i = 0; i < ROLE_ALIGNS; i++)
                    if (validalign(flags.initrole, flags.initrace, i))
                        n++;
            }

            choices = (const char **) alloc(sizeof (char *) * (n + 1));
            pickmap = (int *) alloc(sizeof (int) * (n + 1));
            for (n = 0, i = 0; i < ROLE_ALIGNS; i++) {
                if (ok_align(flags.initrole, flags.initrace,
                             flags.initgend, i)) {
                    choices[n] = aligns[i].adj;
                    pickmap[n++] = i;
                }
            }
            choices[n] = (const char *) 0;
            /* Permit the user to pick, if there is more than one */
            if (n > 1)
                sel = curses_character_dialog(choices,
                                   "Choose one of the following alignments:");
            else
                sel = 0;
            if (sel >= 0)
                sel = pickmap[sel];
            else if (sel == ROLE_NONE) {        /* Quit */
                clearlocks();
                curses_bail(0);
            }
            flags.initalign = sel;
            free((genericptr_t) choices);
            free((genericptr_t) pickmap);
        }
        if (flags.initalign == ROLE_RANDOM) {
            sel = pick_align(flags.initrole, flags.initrace,
                             flags.initgend, PICK_RANDOM);
            if (sel < 0)
                sel = randalign(flags.initrole, flags.initrace);
            flags.initalign = sel;
        }
    }
    return;
}

/* Prompt user for character race, role, alignment, or gender */
int
curses_character_dialog(const char **choices, const char *prompt)
{
    int count, count2, ret, curletter;
    char used_letters[52];
    anything identifier;
    menu_item *selected = NULL;
    winid wid = curses_get_wid(NHW_MENU);
    int clr = 0;

    identifier.a_void = 0;
    curses_start_menu(wid, MENU_BEHAVE_STANDARD);

    for (count = 0; choices[count]; count++) {
        curletter = tolower(choices[count][0]);
        for (count2 = 0; count2 < count; count2++) {
            if (curletter == used_letters[count2]) {
                curletter = toupper(curletter);
            }
        }

        identifier.a_int = (count + 1); /* Must be non-zero */
        curses_add_menu(wid, &nul_glyphinfo, &identifier, curletter, 0,
                        A_NORMAL, clr, choices[count], MENU_ITEMFLAGS_NONE);
        used_letters[count] = curletter;
    }

    /* Random Selection */
    identifier.a_int = ROLE_RANDOM;
    curses_add_menu(wid, &nul_glyphinfo, &identifier, '*', 0,
                    A_NORMAL, clr, "Random", MENU_ITEMFLAGS_NONE);

    /* Quit prompt */
    identifier.a_int = ROLE_NONE;
    curses_add_menu(wid, &nul_glyphinfo, &identifier, 'q', 0,
                    A_NORMAL, clr, "Quit", MENU_ITEMFLAGS_NONE);
    curses_end_menu(wid, prompt);
    ret = curses_select_menu(wid, PICK_ONE, &selected);
    if (ret == 1) {
        ret = (selected->item.a_int);
    } else {                    /* Cancelled selection */

        ret = ROLE_NONE;
    }

    if (ret > 0) {
        ret--;
    }

    free((genericptr_t) selected);
    return ret;
}

#endif /* 0 */

/* Initialize and display options appropriately */
void
curses_init_options(void)
{
    /* change these from set_gameview to set_in_game */
    set_wc_option_mod_status(WC_ALIGN_MESSAGE | WC_ALIGN_STATUS, set_in_game);

    /* Remove a few options that are irrelevant to this windowport */
    set_option_mod_status("eight_bit_tty", set_in_config);

    /* If we don't have a symset defined, load the curses symset by default */
    if (!gs.symset[PRIMARYSET].explicitly)
        load_symset("curses", PRIMARYSET);
    if (!gs.symset[ROGUESET].explicitly)
        load_symset("default", ROGUESET);

#ifdef PDCURSES
    /* PDCurses for SDL, win32 and OS/2 has the ability to set the
       terminal size programatically.  If the user does not specify a
       size in the config file, we will set it to a nice big 32x110 to
       take advantage of some of the nice features of this windowport. */
    if (iflags.wc2_term_cols == 0)
        iflags.wc2_term_cols = 110;
    if (iflags.wc2_term_rows == 0)
        iflags.wc2_term_rows = 32;

    resize_term(iflags.wc2_term_rows, iflags.wc2_term_cols);
    getmaxyx(base_term, term_rows, term_cols);

    /* This is needed for an odd bug with PDCurses-SDL */
/* How to deal with this?
    switch_graphics(ASCII_GRAPHICS);
    if (iflags.IBMgraphics) {
        switch_graphics(IBM_GRAPHICS);
    } else if (iflags.cursesgraphics) {
        switch_graphics(CURS_GRAPHICS);
    } else {
        switch_graphics(ASCII_GRAPHICS);
    }
*/
#endif /* PDCURSES */

    /* FIXME: this overrides explicit OPTIONS=!use_inverse */
    iflags.wc_inverse = TRUE; /* aka iflags.use_inverse; default is False */

    /* fix up pet highlighting */
    if (iflags.wc2_petattr == -1) /* shouldn't happen */
        iflags.wc2_petattr = A_NORMAL;
    if (iflags.wc2_petattr != A_NORMAL) {
        /* Pet attribute specified, so hilite_pet should be true */
        iflags.hilite_pet = TRUE;
    } else if (iflags.hilite_pet) {
        /* pet highlighting specified, so don't leave petattr at A_NORMAL */
        iflags.wc2_petattr = A_REVERSE;
    }

    /* curses doesn't support 's' (single message at a time; successive
       ^P's go back to earlier messages) and 'c' (combination; single
       on first and second of consecutive ^P's, full on third) */
    if (iflags.prevmsg_window != 'f')
        iflags.prevmsg_window = 'r';

#ifdef NCURSES_MOUSE_VERSION
    if (iflags.wc_mouse_support) {
        curses_mouse_support(iflags.wc_mouse_support);
    }
#else
    iflags.wc_mouse_support = 0;
#endif
}

/* Display an ASCII splash screen if the splash_screen option is set */
void
curses_display_splash_window(void)
{
     int i, x_start, y_start;

    curses_get_window_xy(MAP_WIN, &x_start, &y_start);

    if ((term_cols < 70) || (term_rows < 20)) {
        iflags.wc_splash_screen = FALSE;        /* No room for s.s. */
    }

    if (iflags.wc_splash_screen) {
         if (iflags.wc2_guicolor)
              curses_toggle_color_attr(stdscr, CLR_WHITE, A_NORMAL, ON);
        mvaddstr(y_start, x_start, NETHACK_SPLASH_A);
        mvaddstr(y_start + 1, x_start, NETHACK_SPLASH_B);
        mvaddstr(y_start + 2, x_start, NETHACK_SPLASH_C);
        mvaddstr(y_start + 3, x_start, NETHACK_SPLASH_D);
        mvaddstr(y_start + 4, x_start, NETHACK_SPLASH_E);
        mvaddstr(y_start + 5, x_start, NETHACK_SPLASH_F);
        y_start += 7;
    }
    if (iflags.wc2_guicolor)
         curses_toggle_color_attr(stdscr, CLR_WHITE, A_NORMAL, OFF);

    for (i = 1; i <= 4; ++i) {
         mvaddstr(y_start, x_start, copyright_banner_line(i));
         y_start++;
    }

    refresh();
}

/* Restore colors and cursor state before exiting */
void
curses_cleanup(void)
{
}

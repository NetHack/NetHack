/* NetHack 3.6	gnbind.c	$NHDT-Date: 1450453305 2015/12/18 15:41:45 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.33 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file implements the interface between the window port specific
 * code in the Gnome port and the rest of the nethack game engine.
*/

#include "gnbind.h"
#include "gnmain.h"
#include "gnmenu.h"
#include "gnaskstr.h"
#include "gnyesno.h"

GNHWinData gnome_windowlist[MAXWINDOWS];
winid WIN_WORN = WIN_ERR;

extern void tty_raw_print(const char *);
extern void tty_raw_print_bold(const char *);

/* Interface definition, for windows.c */
struct window_procs Gnome_procs = {
    "Gnome", WC_COLOR | WC_HILITE_PET | WC_INVERSE, 0L,
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   /* color availability */
    gnome_init_nhwindows,
    gnome_player_selection, gnome_askname, gnome_get_nh_event,
    gnome_exit_nhwindows, gnome_suspend_nhwindows, gnome_resume_nhwindows,
    gnome_create_nhwindow, gnome_clear_nhwindow, gnome_display_nhwindow,
    gnome_destroy_nhwindow, gnome_curs, gnome_putstr, genl_putmixed,
    gnome_display_file, gnome_start_menu, gnome_add_menu, gnome_end_menu,
    gnome_select_menu,
    genl_message_menu, /* no need for X-specific handling */
    gnome_update_inventory, gnome_mark_synch, gnome_wait_synch,
#ifdef CLIPPING
    gnome_cliparound,
#endif
#ifdef POSITIONBAR
    donull,
#endif
    gnome_print_glyph, gnome_raw_print, gnome_raw_print_bold, gnome_nhgetch,
    gnome_nh_poskey, gnome_nhbell, gnome_doprev_message, gnome_yn_function,
    gnome_getlin, gnome_get_ext_cmd, gnome_number_pad, gnome_delay_output,
#ifdef CHANGE_COLOR /* only a Mac option currently */
    donull, donull,
#endif
    /* other defs that really should go away (they're tty specific) */
    gnome_start_screen, gnome_end_screen, gnome_outrip,
    genl_preference_update, genl_getmsghistory, genl_putmsghistory,
    genl_status_init, genl_status_finish, genl_status_enablefield,
    genl_status_update,
    genl_can_suspend_yes,
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
gnome_init_nhwindows(int *argc, char **argv)
{
    /* Main window */
    ghack_init_main_window(*argc, argv);
    ghack_init_signals();

#ifdef HACKDIR
    // if (ghack_init_glyphs(HACKDIR "/t32-1024.xpm"))
    if (ghack_init_glyphs(HACKDIR "/x11tiles"))
        g_error("ERROR:  Could not initialize glyphs.\n");
#else
#error HACKDIR is not defined!
#endif

    // gnome/gtk is not reentrant
    set_option_mod_status("ignintr", set_gameview);
    flags.ignintr = TRUE;

    iflags.window_inited = TRUE;

    /* gnome-specific window creation */
    WIN_WORN = gnome_create_nhwindow(NHW_WORN);
}

/* Do a window-port specific player type selection. If player_selection()
   offers a Quit option, it is its responsibility to clean up and terminate
   the process. You need to fill in pl_character[0].
*/
void
gnome_player_selection()
{
    int n, i, sel;
    const char **choices;
    int *pickmap;

    /* prevent an unnecessary prompt */
    rigid_role_checks();

    if (!flags.randomall && flags.initrole < 0) {
        /* select a role */
        for (n = 0; roles[n].name.m; n++)
            continue;
        choices = (const char **) alloc(sizeof(char *) * (n + 1));
        pickmap = (int *) alloc(sizeof(int) * (n + 1));
        for (;;) {
            for (n = 0, i = 0; roles[i].name.m; i++) {
                if (ok_role(i, flags.initrace, flags.initgend,
                            flags.initalign)) {
                    if (flags.initgend >= 0 && flags.female
                        && roles[i].name.f)
                        choices[n] = roles[i].name.f;
                    else
                        choices[n] = roles[i].name.m;
                    pickmap[n++] = i;
                }
            }
            if (n > 0)
                break;
            else if (flags.initalign >= 0)
                flags.initalign = -1; /* reset */
            else if (flags.initgend >= 0)
                flags.initgend = -1;
            else if (flags.initrace >= 0)
                flags.initrace = -1;
            else
                panic("no available ROLE+race+gender+alignment combinations");
        }
        choices[n] = (const char *) 0;
        if (n > 1)
            sel = ghack_player_sel_dialog(
                choices, _("Player selection"),
                _("Choose one of the following roles:"));
        else
            sel = 0;
        if (sel >= 0)
            sel = pickmap[sel];
        else if (sel == ROLE_NONE) { /* Quit */
            clearlocks();
            gtk_exit(0);
        }
        free(choices);
        free(pickmap);
    } else if (flags.initrole < 0)
        sel = ROLE_RANDOM;
    else
        sel = flags.initrole;

    if (sel == ROLE_RANDOM) { /* Random role */
        sel = pick_role(flags.initrace, flags.initgend, flags.initalign,
                        PICK_RANDOM);
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
            n = 0; /* number valid */
            for (i = 0; races[i].noun; i++) {
                if (ok_race(flags.initrole, i, flags.initgend,
                            flags.initalign))
                    n++;
            }
            if (n == 0) {
                for (i = 0; races[i].noun; i++) {
                    if (validrace(flags.initrole, i))
                        n++;
                }
            }

            choices = (const char **) alloc(sizeof(char *) * (n + 1));
            pickmap = (int *) alloc(sizeof(int) * (n + 1));
            for (n = 0, i = 0; races[i].noun; i++) {
                if (ok_race(flags.initrole, i, flags.initgend,
                            flags.initalign)) {
                    choices[n] = races[i].noun;
                    pickmap[n++] = i;
                }
            }
            choices[n] = (const char *) 0;
            /* Permit the user to pick, if there is more than one */
            if (n > 1)
                sel = ghack_player_sel_dialog(
                    choices, _("Race selection"),
                    _("Choose one of the following races:"));
            else
                sel = 0;
            if (sel >= 0)
                sel = pickmap[sel];
            else if (sel == ROLE_NONE) { /* Quit */
                clearlocks();
                gtk_exit(0);
            }
            flags.initrace = sel;
            free(choices);
            free(pickmap);
        }
        if (flags.initrace == ROLE_RANDOM) { /* Random role */
            sel = pick_race(flags.initrole, flags.initgend, flags.initalign,
                            PICK_RANDOM);
            if (sel < 0)
                sel = randrace(flags.initrole);
            flags.initrace = sel;
        }
    }

    /* Select a gender, if necessary */
    /* force compatibility with role/race, try for compatibility with
     * pre-selected alignment */
    if (flags.initgend < 0
        || !validgend(flags.initrole, flags.initrace, flags.initgend)) {
        if (flags.initgend == ROLE_RANDOM || flags.randomall) {
            flags.initgend = pick_gend(flags.initrole, flags.initrace,
                                       flags.initalign, PICK_RANDOM);
            if (flags.initgend < 0)
                flags.initgend = randgend(flags.initrole, flags.initrace);
        } else {
            /* Count the number of valid genders */
            n = 0; /* number valid */
            for (i = 0; i < ROLE_GENDERS; i++) {
                if (ok_gend(flags.initrole, flags.initrace, i,
                            flags.initalign))
                    n++;
            }
            if (n == 0) {
                for (i = 0; i < ROLE_GENDERS; i++) {
                    if (validgend(flags.initrole, flags.initrace, i))
                        n++;
                }
            }

            choices = (const char **) alloc(sizeof(char *) * (n + 1));
            pickmap = (int *) alloc(sizeof(int) * (n + 1));
            for (n = 0, i = 0; i < ROLE_GENDERS; i++) {
                if (ok_gend(flags.initrole, flags.initrace, i,
                            flags.initalign)) {
                    choices[n] = genders[i].adj;
                    pickmap[n++] = i;
                }
            }
            choices[n] = (const char *) 0;
            /* Permit the user to pick, if there is more than one */
            if (n > 1)
                sel = ghack_player_sel_dialog(
                    choices, _("Gender selection"),
                    _("Choose one of the following genders:"));
            else
                sel = 0;
            if (sel >= 0)
                sel = pickmap[sel];
            else if (sel == ROLE_NONE) { /* Quit */
                clearlocks();
                gtk_exit(0);
            }
            flags.initgend = sel;
            free(choices);
            free(pickmap);
        }
        if (flags.initgend == ROLE_RANDOM) { /* Random gender */
            sel = pick_gend(flags.initrole, flags.initrace, flags.initalign,
                            PICK_RANDOM);
            if (sel < 0)
                sel = randgend(flags.initrole, flags.initrace);
            flags.initgend = sel;
        }
    }

    /* Select an alignment, if necessary */
    /* force compatibility with role/race/gender */
    if (flags.initalign < 0
        || !validalign(flags.initrole, flags.initrace, flags.initalign)) {
        if (flags.initalign == ROLE_RANDOM || flags.randomall) {
            flags.initalign = pick_align(flags.initrole, flags.initrace,
                                         flags.initgend, PICK_RANDOM);
            if (flags.initalign < 0)
                flags.initalign = randalign(flags.initrole, flags.initrace);
        } else {
            /* Count the number of valid alignments */
            n = 0; /* number valid */
            for (i = 0; i < ROLE_ALIGNS; i++) {
                if (ok_align(flags.initrole, flags.initrace, flags.initgend,
                             i))
                    n++;
            }
            if (n == 0) {
                for (i = 0; i < ROLE_ALIGNS; i++)
                    if (validalign(flags.initrole, flags.initrace, i))
                        n++;
            }

            choices = (const char **) alloc(sizeof(char *) * (n + 1));
            pickmap = (int *) alloc(sizeof(int) * (n + 1));
            for (n = 0, i = 0; i < ROLE_ALIGNS; i++) {
                if (ok_align(flags.initrole, flags.initrace, flags.initgend,
                             i)) {
                    choices[n] = aligns[i].adj;
                    pickmap[n++] = i;
                }
            }
            choices[n] = (const char *) 0;
            /* Permit the user to pick, if there is more than one */
            if (n > 1)
                sel = ghack_player_sel_dialog(
                    choices, _("Alignment selection"),
                    _("Choose one of the following alignments:"));
            else
                sel = 0;
            if (sel >= 0)
                sel = pickmap[sel];
            else if (sel == ROLE_NONE) { /* Quit */
                clearlocks();
                gtk_exit(0);
            }
            flags.initalign = sel;
            free(choices);
            free(pickmap);
        }
        if (flags.initalign == ROLE_RANDOM) {
            sel = pick_align(flags.initrole, flags.initrace, flags.initgend,
                             PICK_RANDOM);
            if (sel < 0)
                sel = randalign(flags.initrole, flags.initrace);
            flags.initalign = sel;
        }
    }
}

/* Ask the user for a player name. */
void
gnome_askname()
{
    int ret;

    g_message("Asking name....");

    /* Ask for a name and stuff the response into plname, a nethack global */
    ret = ghack_ask_string_dialog("What is your name?", "gandalf",
                                  "GnomeHack", gp.plname);

    /* Quit if they want to quit... */
    if (ret == -1) {
        clearlocks();
        gtk_exit(0);
    }
}

/* Does window event processing (e.g. exposure events).
   A noop for the tty and X window-ports.
*/
void
gnome_get_nh_event()
{
    /* We handle our own events. */
    return;
}

/* Exits the window system.  This should dismiss all windows,
   except the "window" used for raw_print().  str is printed if possible.
*/
void
gnome_exit_nhwindows(const char *str)
{
    /* gtk cannot do this without exiting the program, do nothing */
}

/* Prepare the window to be suspended. */
void
gnome_suspend_nhwindows(const char *str)
{
    /* I don't think we need to do anything here... */
    return;
}

/* Restore the windows after being suspended. */
void
gnome_resume_nhwindows()
{
    /* Do Nothing.  Un-necessary since the GUI will refresh itself. */
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
gnome_create_nhwindow(int type)
{
    winid i = 0;

    /* Return the next available winid */

    for (i = 0; i < MAXWINDOWS; i++)
        if (gnome_windowlist[i].win == NULL)
            break;
    if (i == MAXWINDOWS)
        g_error("ERROR:  No windows available...\n");
    gnome_create_nhwindow_by_id(type, i);
    return i;
}

void
gnome_create_nhwindow_by_id(int type, winid i)
{
    switch (type) {
    case NHW_MAP: {
        gnome_windowlist[i].win = ghack_init_map_window();
        gnome_windowlist[i].type = NHW_MAP;
        ghack_main_window_add_map_window(gnome_windowlist[i].win);
        break;
    }
    case NHW_MESSAGE: {
        gnome_windowlist[i].win = ghack_init_message_window();
        gnome_windowlist[i].type = NHW_MESSAGE;
        ghack_main_window_add_message_window(gnome_windowlist[i].win);
        break;
    }
    case NHW_STATUS: {
        gnome_windowlist[i].win = ghack_init_status_window();
        gnome_windowlist[i].type = NHW_STATUS;
        ghack_main_window_add_status_window(gnome_windowlist[i].win);
        break;
    }
    case NHW_WORN: {
        gnome_windowlist[i].win = ghack_init_worn_window();
        gnome_windowlist[i].type = NHW_WORN;
        ghack_main_window_add_worn_window(gnome_windowlist[i].win);
        break;
    }
    case NHW_MENU: {
        gnome_windowlist[i].type = NHW_MENU;
        gnome_windowlist[i].win = ghack_init_menu_window();
        break;
    }
    case NHW_TEXT: {
        gnome_windowlist[i].win = ghack_init_text_window();
        gnome_windowlist[i].type = NHW_TEXT;
        break;
    }
    }
}

/* This widget is being destroyed before its time--
 * clear its entry from the windowlist.
*/
void
gnome_delete_nhwindow_by_reference(GtkWidget *menuWin)
{
    int i;

    for (i = 0; i < MAXWINDOWS; i++) {
        if (gnome_windowlist[i].win == menuWin) {
            gnome_windowlist[i].win = NULL;
            gnome_windowlist[i].type = 0;
            break;
        }
    }
}

/* Clear the given window, when asked to. */
void
gnome_clear_nhwindow(winid wid)
{
    if (gnome_windowlist[wid].win != NULL) {
        gtk_signal_emit(GTK_OBJECT(gnome_windowlist[wid].win),
                        ghack_signals[GHSIG_CLEAR]);
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
gnome_display_nhwindow(winid wid, BOOLEAN_P block)
{
    if (gnome_windowlist[wid].win != NULL) {
        gtk_signal_emit(GTK_OBJECT(gnome_windowlist[wid].win),
                        ghack_signals[GHSIG_DISPLAY], block);
        if (block && (gnome_windowlist[wid].type == NHW_MAP))
            (void) gnome_nhgetch();
    }
}

/* Destroy will dismiss the window if the window has not
 * already been dismissed.
*/
void
gnome_destroy_nhwindow(winid wid)
{
    if ((wid == WIN_MAP) || (wid == WIN_MESSAGE) || (wid == WIN_STATUS)) {
        /* no thanks, I'll do these myself */
        return;
    }
    if (wid != -1 && gnome_windowlist[wid].win != NULL) {
        gtk_widget_destroy(gnome_windowlist[wid].win);
        gnome_windowlist[wid].win = NULL;
        gnome_windowlist[wid].type = 0;
    }
}

/* Next output to window will start at (x,y), also moves
 displayable cursor to (x,y).  For backward compatibility,
 1 <= x < cols, 0 <= y < rows, where cols and rows are
 the size of window.
*/
void
gnome_curs(winid wid, int x, int y)
{
    if (wid != -1 && gnome_windowlist[wid].win != NULL) {
        gtk_signal_emit(GTK_OBJECT(gnome_windowlist[wid].win),
                        ghack_signals[GHSIG_CURS], x, y);
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
gnome_putstr(winid wid, int attr, const char *text)
{
    if ((wid >= 0) && (wid < MAXWINDOWS)
        && (gnome_windowlist[wid].win != NULL)) {
        gtk_signal_emit(GTK_OBJECT(gnome_windowlist[wid].win),
                        ghack_signals[GHSIG_PUTSTR], (guint) attr, text);
    }
}

/* Display the file named str.  Complain about missing files
                   iff complain is TRUE.
*/
void
gnome_display_file(const char *filename, BOOLEAN_P must_exist)
{
    /* Strange -- for some reason it makes us create a new text window
     * instead of reusing any existing ones -- perhaps we can work out
     * some way to reuse stuff -- but for now just make and destroy new
     * ones each time */

    dlb *f;

    f = dlb_fopen(filename, "r");
    if (!f) {
        if (must_exist) {
            GtkWidget *box;
            char message[90];
            sprintf(message, "Warning! Could not find file: %s\n", filename);

            box = gnome_message_box_new(_(message), GNOME_MESSAGE_BOX_ERROR,
                                        GNOME_STOCK_BUTTON_OK, NULL);
            gnome_dialog_set_default(GNOME_DIALOG(box), 0);
            gnome_dialog_set_parent(GNOME_DIALOG(box),
                                    GTK_WINDOW(ghack_get_main_window()));
            gtk_window_set_modal(GTK_WINDOW(box), TRUE);
            gtk_widget_show(box);
        }
    } else {
        GtkWidget *txtwin, *gless, *frametxt;
#define LLEN 128
        char line[LLEN], *textlines;
        int num_lines, charcount;

        txtwin = gnome_dialog_new("Text Window", GNOME_STOCK_BUTTON_OK, NULL);
        gtk_widget_set_usize(GTK_WIDGET(txtwin), 500, 400);
        gtk_window_set_policy(GTK_WINDOW(txtwin), TRUE, TRUE, FALSE);
        gtk_window_set_title(GTK_WINDOW(txtwin), "Text Window");
        gnome_dialog_set_default(GNOME_DIALOG(txtwin), 0);
        gtk_window_set_modal(GTK_WINDOW(txtwin), TRUE);
        frametxt = gtk_frame_new("");
        gtk_widget_show(frametxt);

        /*
         * Count the number of lines and characters in the file.
         */
        num_lines = 0;
        charcount = 1;
        while (dlb_fgets(line, LLEN, f)) {
            num_lines++;
            charcount += strlen(line);
        }
        (void) dlb_fclose(f);

        /* Ignore empty files */
        if (num_lines == 0)
            return;

        /*
         * Re-open the file and read the data into a buffer.
         */
        textlines = (char *) alloc((unsigned int) charcount);
        textlines[0] = '\0';
        f = dlb_fopen(filename, RDTMODE);

        while (dlb_fgets(line, LLEN, f)) {
            (void) strcat(textlines, line);
        }
        (void) dlb_fclose(f);

        gless = gnome_less_new();
        gnome_less_show_string(GNOME_LESS(gless), textlines);
        gtk_container_add(GTK_CONTAINER(frametxt), gless);
        gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(txtwin)->vbox), frametxt,
                           TRUE, TRUE, 0);
        gtk_widget_show_all(txtwin);
        gtk_window_set_modal(GTK_WINDOW(txtwin), TRUE);
        gnome_dialog_set_parent(GNOME_DIALOG(txtwin),
                                GTK_WINDOW(ghack_get_main_window()));
        gnome_dialog_run_and_close(GNOME_DIALOG(txtwin));
        free(textlines);
    }
}

/* Start using window as a menu.  You must call start_menu()
   before add_menu().  After calling start_menu() you may not
   putstr() to the window.  Only windows of type NHW_MENU may
   be used for menus.
*/
void
gnome_start_menu(winid wid, unsigned long mbehavior)
{
    if (wid != -1) {
        if (gnome_windowlist[wid].win == NULL
            && gnome_windowlist[wid].type != 0) {
            gnome_create_nhwindow_by_id(gnome_windowlist[wid].type, wid);
        }
        gtk_signal_emit(GTK_OBJECT(gnome_windowlist[wid].win),
                        ghack_signals[GHSIG_START_MENU]);
    }
}

/*
add_menu(windid window, int glyph, const anything identifier,
                                char accelerator, char groupacc,
                                int attr, char *str, unsigned int itemflags)
                -- Add a text line str to the given menu window.  If
                   identifier is 0, then the line cannot be selected (e.g. a title).
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
                   menu is displayed, set bit MENU_ITEMFLAGS_SELECTED.
*/
void
gnome_add_menu(winid wid, int glyph, const ANY_P *identifier,
               CHAR_P accelerator, CHAR_P group_accel, int attr,
               const char *str, unsigned int itemflags)
{
    boolean presel = ((itemflags & MENU_ITEMFLAGS_SELECTED) != 0);
    GHackMenuItem item;
    item.glyph = glyph;
    item.identifier = identifier;
    item.accelerator = accelerator;
    item.group_accel = group_accel;
    item.attr = attr;
    item.str = str;
    item.presel = presel;
    item.itemflags = itemflags;

    if (wid != -1 && gnome_windowlist[wid].win != NULL) {
        gtk_signal_emit(GTK_OBJECT(gnome_windowlist[wid].win),
                        ghack_signals[GHSIG_ADD_MENU], &item);
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
gnome_end_menu(winid wid, const char *prompt)
{
    if (wid != -1 && gnome_windowlist[wid].win != NULL) {
        gtk_signal_emit(GTK_OBJECT(gnome_windowlist[wid].win),
                        ghack_signals[GHSIG_END_MENU], prompt);
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
gnome_select_menu(winid wid, int how, MENU_ITEM_P **selected)
{
    int nReturned = -1;

    if (wid != -1 && gnome_windowlist[wid].win != NULL
        && gnome_windowlist[wid].type == NHW_MENU) {
        nReturned = ghack_menu_window_select_menu(gnome_windowlist[wid].win,
                                                  selected, how);
    }

    return nReturned;
}

/*
    -- Indicate to the window port that the inventory has been changed.
    -- Merely calls display_inventory() for window-ports that leave the
        window up, otherwise empty.
*/
void
gnome_update_inventory()
{
    ghack_main_window_update_inventory();
}

/*
mark_synch()    -- Don't go beyond this point in I/O on any channel until
                   all channels are caught up to here.  Can be an empty call
                   for the moment
*/
void
gnome_mark_synch()
{
    /* Do nothing */
}

/*
wait_synch()    -- Wait until all pending output is complete (*flush*() for
                   streams goes here).
                -- May also deal with exposure events etc. so that the
                   display is OK when return from wait_synch().
*/
void
gnome_wait_synch()
{
    /* Do nothing */
}

/*
cliparound(x, y)-- Make sure that the user is more-or-less centered on the
                   screen if the playing area is larger than the screen.
                -- This function is only defined if CLIPPING is defined.
*/
void
gnome_cliparound(int x, int y)
{
    /* FIXME!!!  winid should be a parameter!!!
     * Call a function that Does The Right Thing(tm).
    */
    gnome_cliparound_proper(WIN_MAP, x, y);
}

void
gnome_cliparound_proper(winid wid, int x, int y)
{
    if (wid != -1 && gnome_windowlist[wid].win != NULL) {
        gtk_signal_emit(GTK_OBJECT(gnome_windowlist[wid].win),
                        ghack_signals[GHSIG_CLIPAROUND], (guint) x,
                        (guint) y);
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
gnome_print_glyph(winid wid, XCHAR_P x, XCHAR_P y, int glyph, int bkglyph)
{
    if (wid != -1 && gnome_windowlist[wid].win != NULL) {
        GdkImlibImage *im;

        im = ghack_image_from_glyph(glyph, FALSE);

        gtk_signal_emit(GTK_OBJECT(gnome_windowlist[wid].win),
                        ghack_signals[GHSIG_PRINT_GLYPH], (guint) x,
                        (guint) y, im, NULL);
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
gnome_raw_print(const char *str)
{
    tty_raw_print(str);
}

/*
raw_print_bold(str)
                -- Like raw_print(), but prints in bold/standout (if
possible).
*/
void
gnome_raw_print_bold(const char *str)
{
    tty_raw_print_bold(str);
}

/*
int nhgetch()   -- Returns a single character input from the user.
                -- In the tty window-port, nhgetch() assumes that tgetch()
                   will be the routine the OS provides to read a character.
                   Returned character _must_ be non-zero.
*/
int
gnome_nhgetch()
{
    int key;
    GList *theFirst;
    gtk_signal_emit(GTK_OBJECT(gnome_windowlist[WIN_STATUS].win),
                    ghack_signals[GHSIG_FADE_HIGHLIGHT]);

    g_askingQuestion = 1;
    /* Process events until a key press event arrives. */
    while (g_numKeys == 0) {
        if (gp.program_state.done_hup)
            return '\033';
        gtk_main_iteration();
    }

    theFirst = g_list_first(g_keyBuffer);
    g_keyBuffer = g_list_remove_link(g_keyBuffer, theFirst);
    key = GPOINTER_TO_INT(theFirst->data);
    g_list_free_1(theFirst);
    g_numKeys--;
    g_askingQuestion = 0;
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
gnome_nh_poskey(int *x, int *y, int *mod)
{
    gtk_signal_emit(GTK_OBJECT(gnome_windowlist[WIN_STATUS].win),
                    ghack_signals[GHSIG_FADE_HIGHLIGHT]);

    g_askingQuestion = 0;
    /* Process events until a key or map-click arrives. */
    while (g_numKeys == 0 && g_numClicks == 0) {
        if (gp.program_state.done_hup)
            return '\033';
        gtk_main_iteration();
    }

    if (g_numKeys > 0) {
        int key;
        GList *theFirst;

        theFirst = g_list_first(g_keyBuffer);
        g_keyBuffer = g_list_remove_link(g_keyBuffer, theFirst);
        key = GPOINTER_TO_INT(theFirst->data);
        g_list_free_1(theFirst);
        g_numKeys--;
        return (key);
    } else {
        GHClick *click;
        GList *theFirst;

        theFirst = g_list_first(g_clickBuffer);
        g_clickBuffer = g_list_remove_link(g_clickBuffer, theFirst);
        click = (GHClick *) theFirst->data;
        *x = click->x;
        *y = click->y;
        *mod = click->mod;
        g_free(click);
        g_list_free_1(theFirst);
        g_numClicks--;
        return (0);
    }
}

/*
nhbell()        -- Beep at user.  [This will exist at least until sounds are
                   redone, since sounds aren't attributable to windows
anyway.]
*/
void
gnome_nhbell()
{
    /* FIXME!!! Play a cool GNOME sound instead */
    gdk_beep();
}

/*
doprev_message()
                -- Display previous messages.  Used by the ^P command.
                -- On the tty-port this scrolls WIN_MESSAGE back one line.
*/
int
gnome_doprev_message()
{
    /* Do Nothing.  They can read old messages using the scrollbar. */
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
gnome_yn_function(const char *question, const char *choices, CHAR_P def)
{
    int ch;
    int result = -1;
    char message[BUFSZ];
    char yn_esc_map = '\033';
    GtkWidget *mainWnd = ghack_get_main_window();

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

    gnome_putstr(WIN_MESSAGE, ATR_BOLD, message);
    if (mainWnd != NULL && choices && !strchr(choices, ch)) {
        return (ghack_yes_no_dialog(question, choices, def));
    }

    /* Only here if main window is not present */
    while (result < 0) {
        ch = gnome_nhgetch();
        if (ch == '\033') {
            result = yn_esc_map;
        } else if (choices && !strchr(choices, ch)) {
            /* FYI: ch==-115 is for KP_ENTER */
            if (def
                && (ch == ' ' || ch == '\r' || ch == '\n' || ch == -115)) {
                result = def;
            } else {
                gnome_nhbell();
                /* and try again... */
            }
        } else {
            result = ch;
        }
    }
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
gnome_getlin(const char *question, char *input)
{
    int ret;

    ret = ghack_ask_string_dialog(question, "", "nethack", input);

    if (ret == -1)
        input[0] = 0;
}

/*
int get_ext_cmd(void)
            -- Get an extended command in a window-port specific way.
               An index into extcmdlist[] is returned on a successful
               selection, -1 otherwise.
*/
int
gnome_get_ext_cmd()
{
    return ghack_menu_ext_cmd();
}

/*
number_pad(state)
            -- Initialize the number pad to the given state.
*/
void
gnome_number_pad(int state)
{
    /* Do Nothing */
}

/*
delay_output()  -- Causes a visible delay of 50ms in the output.
               Conceptually, this is similar to wait_synch() followed
               by a nap(50ms), but allows asynchronous operation.
*/
void
gnome_delay_output()
{
    if (gnome_windowlist[WIN_MESSAGE].win != NULL) {
        gtk_signal_emit(GTK_OBJECT(gnome_windowlist[WIN_MESSAGE].win),
                        ghack_signals[GHSIG_DELAY], (guint) 50);
    }
}

/*
start_screen()  -- Only used on Unix tty ports, but must be declared for
               completeness.  Sets up the tty to work in full-screen
               graphics mode.  Look at win/tty/termcap.c for an
               example.  If your window-port does not need this function
               just declare an empty function.
*/
void
gnome_start_screen()
{
    /* Do Nothing */
}

/*
end_screen()    -- Only used on Unix tty ports, but must be declared for
               completeness.  The complement of start_screen().
*/
void
gnome_end_screen()
{
    /* Do Nothing */
}

/*
outrip(winid, int, when)
            -- The tombstone code.  If you want the traditional code use
               genl_outrip for the value and check the #if in rip.c.
*/
void
gnome_outrip(winid wid, int how, time_t when)
{
    /* Follows roughly the same algorithm as genl_outrip() */
    char buf[BUFSZ];
    char ripString[BUFSZ] = "\0";
    long year;

    /* Put name on stone */
    Sprintf(buf, "%s\n", gp.plname);
    Strcat(ripString, buf);

    /* Put $ on stone */
    Sprintf(buf, "%ld Au\n", done_money);
    Strcat(ripString, buf);

    /* Put together death description */
    formatkiller(buf, sizeof buf, how, FALSE);

    /* Put death type on stone */
    Strcat(ripString, buf);
    Strcat(ripString, "\n");

    /* Put year on stone */
    year = yyyymmdd(when) / 10000L;
    Sprintf(buf, "%4ld\n", year);
    Strcat(ripString, buf);

    ghack_text_window_rip_string(ripString);
}

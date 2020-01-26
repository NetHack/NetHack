/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.6 cursmain.c */
/* Copyright (c) Karl Garrison, 2010. */
/* NetHack may be freely redistributed.  See license for details. */

#include "curses.h"
#include "hack.h"
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif
#include "color.h"
#include "wincurs.h"

/* define this if not linking with <foo>tty.o|.obj for some reason */
#ifdef CURSES_DEFINE_ERASE_CHAR
char erase_char, kill_char;
#else
/* defined in sys/<foo>/<foo>tty.o|.obj which gets linked into
   tty-only, tty+curses, and curses-only binaries */
extern char erase_char, kill_char;
#endif

extern long curs_mesg_suppress_turn; /* from cursmesg.c */

/* stubs for curses_procs{} */
#ifdef POSITIONBAR
static void dummy_update_position_bar(char *);
#endif
#ifdef CHANGE_COLOR
static void dummy_change_color(int, long, int);
static char *dummy_get_color_string(VOID_ARGS);
#endif

/* Public functions for curses NetHack interface */

/* Interface definition, for windows.c */
struct window_procs curses_procs = {
    "curses",
    (WC_ALIGN_MESSAGE | WC_ALIGN_STATUS | WC_COLOR | WC_INVERSE
     | WC_HILITE_PET
#ifdef NCURSES_MOUSE_VERSION /* (this macro name works for PDCURSES too) */
     | WC_MOUSE_SUPPORT
#endif
     | WC_PERM_INVENT | WC_POPUP_DIALOG | WC_SPLASH_SCREEN),
    (WC2_DARKGRAY | WC2_HITPOINTBAR
#if defined(STATUS_HILITES)
     | WC2_HILITE_STATUS
#endif
     | WC2_FLUSH_STATUS | WC2_TERM_SIZE
     | WC2_STATUSLINES | WC2_WINDOWBORDERS | WC2_PETATTR | WC2_GUICOLOR
     | WC2_SUPPRESS_HIST),
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   /* color availability */
    curses_init_nhwindows,
    curses_player_selection,
    curses_askname,
    curses_get_nh_event,
    curses_exit_nhwindows,
    curses_suspend_nhwindows,
    curses_resume_nhwindows,
    curses_create_nhwindow,
    curses_clear_nhwindow,
    curses_display_nhwindow,
    curses_destroy_nhwindow,
    curses_curs,
    curses_putstr,
    genl_putmixed,
    curses_display_file,
    curses_start_menu,
    curses_add_menu,
    curses_end_menu,
    curses_select_menu,
    genl_message_menu,
    curses_update_inventory,
    curses_mark_synch,
    curses_wait_synch,
#ifdef CLIPPING
    curses_cliparound,
#endif
#ifdef POSITIONBAR
    dummy_update_position_bar,
#endif
    curses_print_glyph,
    curses_raw_print,
    curses_raw_print_bold,
    curses_nhgetch,
    curses_nh_poskey,
    curses_nhbell,
    curses_doprev_message,
    curses_yn_function,
    curses_getlin,
    curses_get_ext_cmd,
    curses_number_pad,
    curses_delay_output,
#ifdef CHANGE_COLOR
    dummy_change_color,
#ifdef MAC /* old OS 9, not OSX */
    (void (*)(int)) 0,
    (short (*)(winid, char *)) 0,
#endif
    dummy_get_color_string,
#endif
    curses_start_screen,
    curses_end_screen,
    genl_outrip,
    curses_preference_update,
    curses_getmsghistory,
    curses_putmsghistory,
    curses_status_init,
    curses_status_finish,
    genl_status_enablefield,
    curses_status_update,
    genl_can_suspend_yes,
};

/*
 * Global variables for curses interface
 */

int term_rows, term_cols;   /* size of underlying terminal */
int orig_cursor;            /* Preserve initial cursor state */
WINDOW *base_term;          /* underlying terminal window */
boolean counting;           /* Count window is active */
WINDOW *mapwin, *statuswin, *messagewin;    /* Main windows */

/* Track if we're performing an update to the permanent window.
   Needed since we aren't using the normal menu functions to handle
   the inventory window. */
static int inv_update = 0;

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
curses_init_nhwindows(int *argcp UNUSED,
                      char **argv UNUSED)
{
#ifdef PDCURSES
    char window_title[BUFSZ];
#endif

#ifdef XCURSES
    base_term = Xinitscr(*argcp, argv);
#else
    base_term = initscr();
#endif
#ifdef TEXTCOLOR
    if (has_colors()) {
        start_color();
        curses_init_nhcolors();
    } else {
        iflags.use_color = FALSE;
        set_option_mod_status("color", SET_IN_FILE);
        iflags.wc2_guicolor = FALSE;
        set_wc2_option_mod_status(WC2_GUICOLOR, SET_IN_FILE);
    }
#else
    iflags.use_color = FALSE;
    set_option_mod_status("color", SET_IN_FILE);
    iflags.wc2_guicolor = FALSE;
    set_wc2_option_mod_status(WC2_GUICOLOR, SET_IN_FILE);
#endif
    noecho();
    raw();
    nonl(); /* don't force ^M into newline (^J); input accepts them both
             * but as a command, accidental <enter> won't run South */
    meta(stdscr, TRUE);
    orig_cursor = curs_set(0);
    keypad(stdscr, TRUE);
#ifdef NCURSES_VERSION
# ifdef __APPLE__
    ESCDELAY = 25;
# else
    set_escdelay(25);
# endif/* __APPLE__ */
#endif /* NCURSES_VERSION */
#ifdef PDCURSES
# ifdef DEF_GAME_NAME
#  ifdef VERSION_STRING
    sprintf(window_title, "%s %s", DEF_GAME_NAME, VERSION_STRING);
#  else
    sprintf(window_title, "%s", DEF_GAME_NAME);
#  endif
       /* VERSION_STRING */
# else
#  ifdef VERSION_STRING
    sprintf(window_title, "%s %s", "NetHack", VERSION_STRING);
#  else
    sprintf(window_title, "%s", "NetHack");
#  endif
       /* VERSION_STRING */
# endif/* DEF_GAME_NAME */
    PDC_set_title(window_title);
    PDC_set_blink(TRUE);        /* Only if the user asks for it! */
    timeout(1);
    (void) getch();
    timeout(-1);
#endif /* PDCURSES */
    getmaxyx(base_term, term_rows, term_cols);
    counting = FALSE;
    curses_init_options();
    if (term_rows < 15 || term_cols < 40) {
        panic("Terminal is too small; must have at least %s%s%s.",
              (term_rows < 15) ? "15 rows" : "",
              (term_rows < 15 && term_cols < 40) ? " and " : "",
              (term_cols < 40) ? "40 columns" : "");
    }
    /* during line input, deletes the most recently typed character */
    erase_char = erasechar(); /* <delete>/<rubout> or possibly <backspace> */
    /* during line input, deletes all typed characters */
    kill_char = killchar(); /* ^U (back in prehistoric times, '@') */

    curses_create_main_windows();
    curses_init_mesg_history();
    curses_display_splash_window();
}

/* Do a window-port specific player type selection. If player_selection()
   offers a Quit option, it is its responsibility to clean up and terminate
   the process. You need to fill in pl_character[0].
*/
void
curses_player_selection()
{
    curses_choose_character();
}


/* Ask the user for a player name. */
void
curses_askname()
{
    plname[0] = '\0';
    curses_line_input_dialog("Who are you?", plname, PL_NSIZ);
}


/* Does window event processing (e.g. exposure events).
   A noop for the tty and X window-ports.
*/
void
curses_get_nh_event()
{
    boolean do_reset = FALSE;

#ifdef PDCURSES
    if (is_termresized()) {
        resize_term(0, 0);
        do_reset = TRUE;
    }
#endif
#ifdef NCURSES_VERSION          /* Is there a better way to detect ncurses? */
    if (is_term_resized(term_rows, term_cols)) {
        if (!isendwin()) {
            endwin();
        }
        refresh();
        do_reset = TRUE;
    }
#endif

    if (do_reset) {
        getmaxyx(base_term, term_rows, term_cols);
        curses_got_input(); /* reset More>> */
        /* status_initialize, create_main_windows, last_messages, doredraw */
        curs_reset_windows(TRUE, TRUE);
    }
}

/* Exits the window system.  This should dismiss all windows,
   except the "window" used for raw_print().  str is printed if possible.
*/
void
curses_exit_nhwindows(const char *str)
{
    curses_destroy_nhwindow(INV_WIN);
    curses_destroy_nhwindow(MAP_WIN);
    curses_destroy_nhwindow(STATUS_WIN);
    curses_destroy_nhwindow(MESSAGE_WIN);
    curs_destroy_all_wins();

    curses_cleanup();
    curs_set(orig_cursor);
    endwin();
    iflags.window_inited = 0;
    if (str != NULL) {
        raw_print(str);
    }
}

/* Prepare the window to be suspended. */
void
curses_suspend_nhwindows(const char *str UNUSED)
{
    endwin();
}


/* Restore the windows after being suspended. */
void
curses_resume_nhwindows()
{
    curses_refresh_nethack_windows();
}

/*  Create a window of type "type" which can be
        NHW_MESSAGE     (top line)
        NHW_STATUS      (bottom lines)
        NHW_MAP         (main dungeon)
        NHW_MENU        (inventory or other "corner" windows)
        NHW_TEXT        (help/text, full screen paged window)
*/
winid
curses_create_nhwindow(int type)
{
    winid wid = curses_get_wid(type);

    if (curses_is_menu(wid) || curses_is_text(wid)) {
        curses_start_menu(wid);
        curses_add_wid(wid);
    }

    return wid;
}


/* Clear the given window, when asked to. */
void
curses_clear_nhwindow(winid wid)
{
    if (wid != NHW_MESSAGE) {
        curses_clear_nhwin(wid);
    } else {
        /* scroll the message window one line if it's full */
        curses_count_window("");
        /* remove 'countwin', leaving last message line blank */
        curses_count_window((char *) 0);
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
curses_display_nhwindow(winid wid, BOOLEAN_P block)
{
    menu_item *selected = NULL;

    if (curses_is_menu(wid) || curses_is_text(wid)) {
        curses_end_menu(wid, "");
        curses_select_menu(wid, PICK_NONE, &selected);
        return;
    }

    /* don't overwrite the splash screen first time through */
    if (!iflags.window_inited && wid == MAP_WIN) {
        iflags.window_inited = TRUE;
    } else {
        /* actually display the window */
        wnoutrefresh(curses_get_nhwin(wid));
        /* flush pending writes from other windows too */
        doupdate();
    }
    if ((wid == MAP_WIN) && block) {
        (void) curses_more();
    }

    if ((wid == MESSAGE_WIN) && block) {
        (void) curses_block(TRUE);
    }
}


/* Destroy will dismiss the window if the window has not
 * already been dismissed.
*/
void
curses_destroy_nhwindow(winid wid)
{
    switch (wid) {
    case MESSAGE_WIN:
        curses_teardown_messages(); /* discard ^P message history data */
        break;
    case STATUS_WIN:
        if (VIA_WINDOWPORT())
            curses_status_finish(); /* discard cached status data */
        break;
    case INV_WIN:
        iflags.perm_invent = 0; /* avoid unexpected update_inventory() */
        break;
    case MAP_WIN:
        break;
    default:
        break;
    }
    curses_del_nhwin(wid);
}

/* Next output to window will start at (x,y), also moves
 displayable cursor to (x,y).  For backward compatibility,
 1 <= x < cols, 0 <= y < rows, where cols and rows are
 the size of window.
*/
void
curses_curs(winid wid, int x, int y)
{
    curses_move_cursor(wid, x, y);
}

/*
putstr(window, attr, str)
                -- Print str on the window with the given attribute.  Only
                   printable ASCII characters (040-0126) must be supported.
                   Multiple putstr()s are output on separate lines.
                   Attributes can be one of
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
curses_putstr(winid wid, int attr, const char *text)
{
    int mesgflags, curses_attr;

    mesgflags = attr & (ATR_URGENT | ATR_NOHISTORY);
    attr &= ~mesgflags;

    if (wid == WIN_MESSAGE && (mesgflags & ATR_NOHISTORY) != 0) {
        /* display message without saving it in recall history */
        curses_count_window(text);
    } else {
        /* We need to convert NetHack attributes to curses attributes */
        curses_attr = curses_convert_attr(attr);
        curses_puts(wid, curses_attr, text);
    }
}

/* Display the file named str.  Complain about missing files
                   iff complain is TRUE.
*/
void
curses_display_file(const char *filename, BOOLEAN_P must_exist)
{
    curses_view_file(filename, must_exist);
}

/* Start using window as a menu.  You must call start_menu()
   before add_menu().  After calling start_menu() you may not
   putstr() to the window.  Only windows of type NHW_MENU may
   be used for menus.
*/
void
curses_start_menu(winid wid)
{
    if (inv_update)
        return;

    curses_create_nhmenu(wid);
}

/*
add_menu(winid wid, int glyph, const anything identifier,
                                char accelerator, char groupacc,
                                int attr, char *str, boolean preselected)
                -- Add a text line str to the given menu window.  If identifier
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
                   the menu command (or their user defined alises), it loses.
                   The menu commands and aliases take care not to interfere
                   with the default object class symbols.
                -- If you want this choice to be preselected when the
                   menu is displayed, set preselected to TRUE.
*/
void
curses_add_menu(winid wid, int glyph, const ANY_P * identifier,
                CHAR_P accelerator, CHAR_P group_accel, int attr,
                const char *str, BOOLEAN_P presel)
{
    int curses_attr;

    attr &= ~(ATR_URGENT | ATR_NOHISTORY);
    curses_attr = curses_convert_attr(attr);

    if (inv_update) {
        curses_add_inv(inv_update, glyph, accelerator, curses_attr, str);
        inv_update++;
        return;
    }

    curses_add_nhmenu_item(wid, glyph, identifier, accelerator, group_accel,
                           curses_attr, str, presel);
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
curses_end_menu(winid wid, const char *prompt)
{
    if (inv_update)
        return;

    curses_finalize_nhmenu(wid, prompt);
}

/*
int select_menu(winid window, int how, menu_item **selected)
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
curses_select_menu(winid wid, int how, MENU_ITEM_P ** selected)
{
    if (inv_update)
        return 0;

    return curses_display_nhmenu(wid, how, selected);
}

void
curses_update_inventory(void)
{
    /* Don't do anything if perm_invent is off unless we
       changed the option. */
    if (!iflags.perm_invent) {
        if (curses_get_nhwin(INV_WIN)) {
            curs_reset_windows(TRUE, FALSE);
        }
        return;
    }

    /* Update inventory sidebar. NetHack uses normal menu functions
       when drawing the inventory, and we don't want to change the
       underlying code. So instead, track if an inventory update is
       being performed with a static variable. */
    inv_update = 1;
    curses_update_inv();
    inv_update = 0;
}

/*
mark_synch()    -- Don't go beyond this point in I/O on any channel until
                   all channels are caught up to here.  Can be an empty call
                   for the moment
*/
void
curses_mark_synch()
{
}

/*
wait_synch()    -- Wait until all pending output is complete (*flush*() for
                   streams goes here).
                -- May also deal with exposure events etc. so that the
                   display is OK when return from wait_synch().
*/
void
curses_wait_synch()
{
    /* [do we need 'if (counting) curses_count_window((char *)0);' here?] */
}

/*
cliparound(x, y)-- Make sure that the user is more-or-less centered on the
                   screen if the playing area is larger than the screen.
                -- This function is only defined if CLIPPING is defined.
*/
void
curses_cliparound(int x, int y)
{
    int sx, sy, ex, ey;
    boolean redraw = curses_map_borders(&sx, &sy, &ex, &ey, x, y);

    if (redraw) {
        curses_draw_map(sx, sy, ex, ey);
    }
}

/*
print_glyph(window, x, y, glyph, bkglyph)
                -- Print the glyph at (x,y) on the given window.  Glyphs are
                   integers at the interface, mapped to whatever the window-
                   port wants (symbol, font, color, attributes, ...there's
                   a 1-1 map between glyphs and distinct things on the map).
                   bkglyph is to render the background behind the glyph.
                   It's not used here.
*/
void
curses_print_glyph(winid wid, XCHAR_P x, XCHAR_P y, int glyph,
                   int bkglyph UNUSED)
{
    int ch;
    int color;
    unsigned int special;
    int attr = -1;

    /* map glyph to character and color */
    mapglyph(glyph, &ch, &color, &special, x, y, 0);
    if ((special & MG_PET) && iflags.hilite_pet) {
        attr = iflags.wc2_petattr;
    }
    if ((special & MG_DETECT) && iflags.use_inverse) {
        attr = A_REVERSE;
    }
    if (SYMHANDLING(H_DEC))
        ch = curses_convert_glyph(ch, glyph);

    if (wid == NHW_MAP) {
/* hilite stairs not in 3.6, yet
        if ((special & MG_STAIRS) && iflags.hilite_hidden_stairs) {
            color = 16 + (color * 2);
        } else
*/
        if ((special & MG_OBJPILE) && iflags.hilite_pile) {
            if (iflags.wc_color)
                color = 16 + (color * 2) + 1;
            else
                attr = A_REVERSE;
        }
        /* water and lava look the same except for color; when color is off,
           render lava in inverse video so that they look different */
        if ((special & MG_BW_LAVA) && iflags.use_inverse) {
            attr = A_REVERSE; /* mapglyph() only sets this if color is off */
        }
    }

    curses_putch(wid, x, y, ch, color, attr);
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
curses_raw_print(const char *str)
{
#ifdef PDCURSES
    WINDOW *win = curses_get_nhwin(MESSAGE_WIN);

    curses_message_win_puts(str, FALSE);
#else
    puts(str);
#endif
}

/*
raw_print_bold(str)
            -- Like raw_print(), but prints in bold/standout (if possible).
*/
void
curses_raw_print_bold(const char *str)
{
    curses_raw_print(str);
}

/*
int nhgetch()   -- Returns a single character input from the user.
                -- In the tty window-port, nhgetch() assumes that tgetch()
                   will be the routine the OS provides to read a character.
                   Returned character _must_ be non-zero.
*/
int
curses_nhgetch()
{
    int ch;

    curses_prehousekeeping();
    ch = curses_read_char();
    curses_posthousekeeping();

    return ch;
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
curses_nh_poskey(int *x, int *y, int *mod)
{
    int key = curses_nhgetch();

#ifdef NCURSES_MOUSE_VERSION
    /* Mouse event if mouse_support is true */
    if (key == KEY_MOUSE) {
        key = curses_get_mouse(x, y, mod);
    }
#endif

    return key;
}

/*
nhbell()        -- Beep at user.  [This will exist at least until sounds are
                   redone, since sounds aren't attributable to windows anyway.]
*/
void
curses_nhbell()
{
    beep();
}

/*
doprev_message()
                -- Display previous messages.  Used by the ^P command.
                -- On the tty-port this scrolls WIN_MESSAGE back one line.
*/
int
curses_doprev_message()
{
    curses_prev_mesg();
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
curses_yn_function(const char *question, const char *choices, CHAR_P def)
{
    return (char) curses_character_input_dialog(question, choices, def);
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
curses_getlin(const char *question, char *input)
{
    curses_line_input_dialog(question, input, BUFSZ);
}

/*
int get_ext_cmd(void)
            -- Get an extended command in a window-port specific way.
               An index into extcmdlist[] is returned on a successful
               selection, -1 otherwise.
*/
int
curses_get_ext_cmd()
{
    return curses_ext_cmd();
}


/*
number_pad(state)
            -- Initialize the number pad to the given state.
*/
void
curses_number_pad(int state UNUSED)
{
    return;
}

/*
delay_output()  -- Causes a visible delay of 50ms in the output.
               Conceptually, this is similar to wait_synch() followed
               by a nap(50ms), but allows asynchronous operation.
*/
void
curses_delay_output()
{
    /* refreshing the whole display is a waste of time,
     * but that's why we're here */
    refresh();
    napms(50);
}

/*
start_screen()  -- Only used on Unix tty ports, but must be declared for
               completeness.  Sets up the tty to work in full-screen
               graphics mode.  Look at win/tty/termcap.c for an
               example.  If your window-port does not need this function
               just declare an empty function.
*/
void
curses_start_screen()
{
}

/*
end_screen()    -- Only used on Unix tty ports, but must be declared for
               completeness.  The complement of start_screen().
*/
void
curses_end_screen()
{
}

/*
outrip(winid, int)
            -- The tombstone code.  If you want the traditional code use
               genl_outrip for the value and check the #if in rip.c.
*/
void
curses_outrip(winid wid UNUSED,
              int how UNUSED)
{
     return;
}

/*
preference_update(preference)
                -- The player has just changed one of the wincap preference
                   settings, and the NetHack core is notifying your window
                   port of that change.  If your window-port is capable of
                   dynamically adjusting to the change then it should do so.
                   Your window-port will only be notified of a particular
                   change if it indicated that it wants to be by setting the
                   corresponding bit in the wincap mask.
*/
void
curses_preference_update(const char *pref)
{
    boolean redo_main = FALSE, redo_status = FALSE;

    if (!strcmp(pref, "align_status")
        || !strcmp(pref, "statuslines")
        || !strcmp(pref, "windowborders"))
        redo_main = redo_status = TRUE;
    else if (!strcmp(pref, "hilite_status"))
        redo_status = TRUE;
    else if (!strcmp(pref, "align_message"))
        redo_main = TRUE;
    else if (!strcmp(pref, "mouse_support"))
        curses_mouse_support(iflags.wc_mouse_support);

    if (redo_main || redo_status)
        curs_reset_windows(redo_main, redo_status);
}

void
curs_reset_windows(boolean redo_main, boolean redo_status)
{
    boolean need_redraw = FALSE;

    if (redo_status) {
        status_initialize(REASSESS_ONLY);
        need_redraw = TRUE;
    }
    if (redo_main) {
        curses_create_main_windows();
        need_redraw = TRUE;
    }
    if (need_redraw) {
        curses_last_messages();
        doredraw();
    }
}

/* stubs for curses_procs{} */

#ifdef POSITIONBAR
static void
dummy_update_position_bar(char *arg UNUSED)
{
    return;
}
#endif

#ifdef CHANGE_COLOR
static void
dummy_change_color(int a1 UNUSED, long a2 UNUSED, int a3 UNUSED)
{
    return;
}

static char *
dummy_get_color_string(VOID_ARGS)
{
    return (char *) 0;
}
#endif

/*cursmain.c*/

/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.6 cursinvt.c */
/* Copyright (c) Fredrik Ljungdahl, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#include "curses.h"
#include "hack.h"
#include "wincurs.h"
#include "cursinvt.h"

/* Permanent inventory for curses interface */

/* Runs when the game indicates that the inventory has been updated */
void
curses_update_inv(void)
{
    WINDOW *win = curses_get_nhwin(INV_WIN);
    boolean border;
    int x = 0, y = 0;

    /* Check if the inventory window is enabled in first place */
    if (!win) {
        /* It's not. Re-initialize the main windows if the
           option was enabled. */
        if (iflags.perm_invent) {
            /* [core_]status_initialize, curses_create_main_windows,
               curses_last_messages, [core_]doredraw; doredraw will
               call (*update_inventory) [curses_update_inventory] which
               will call us but 'win' should be defined that time */
            curs_reset_windows(TRUE, FALSE);
            /* TODO: guard against window creation failure [if that's
               possible] which would lead to uncontrolled recursion */
        }
        return;
    }

    border = curses_window_has_border(INV_WIN);

    /* Figure out drawing area */
    if (border) {
        x++;
        y++;
    }

    /* Clear the window as it is at the moment. */
    werase(win);

    display_inventory(NULL, FALSE);

    if (border)
        box(win, 0, 0);

    wnoutrefresh(win);
}

/* Adds an inventory item.  'y' is 1 rather than 0 for the first item. */
void
curses_add_inv(int y,
               int glyph UNUSED,
               CHAR_P accelerator, attr_t attr, const char *str)
{
    WINDOW *win = curses_get_nhwin(INV_WIN);
    int color = NO_COLOR;
    int x = 0, width, height, available_width,
        border = curses_window_has_border(INV_WIN) ? 1 : 0;

    /* Figure out where to draw the line */
    x += border; /* x starts at 0 and is incremented for border */
    y -= 1 - border; /* y starts at 1 and is decremented for non-border */

    curses_get_window_size(INV_WIN, &height, &width);
    /*
     * TODO:
     *  If border is On and 'y' is too big, turn border Off in order to
     *  get two more lines of perm_invent.
     *
     *  And/or implement a way to switch focus from map to inventory
     *  so that the latter can be scrolled.  Must not require use of a
     *  mouse.
     *
     *  Also, when entries are omitted due to lack of space, mark the
     *  last line to indicate "there's more that you can't see" (like
     *  horizontal status window does for excess status conditions).
     *  Normal menu does this via 'page M of N'.
     */
    if (y - border >= height) /* 'height' is already -2 for Top+Btm borders */
        return;
    available_width = width; /* 'width' also already -2 for Lft+Rgt borders */

    wmove(win, y, x);
    if (accelerator) {
#if 0
        attr_t bold = A_BOLD;

        wattron(win, bold);
        waddch(win, accelerator);
        wattroff(win, bold);
        wprintw(win, ") ");
#else
        /* despite being shown as a menu, nothing is selectable from the
           persistent inventory window so don't highlight inventory letters */
        wprintw(win, "%c) ", accelerator);
#endif
        available_width -= 3;

        /* narrow the entries to fit more of the interesting text; do so
           unconditionally rather than trying to figure whether it's needed;
           when 'sortpack' is enabled we could also strip out "<class> of"
           from "<prefix><class> of <item><suffix> but if that's to be done,
           core ought to do it */
        if (!strncmpi(str, "a ", 2))
            str += 2;
        else if (!strncmpi(str, "an ", 3))
            str += 3;
        else if (!strncmpi(str, "the ", 4))
            str +=4;
    }
#if 0 /* FIXME: MENU GLYPHS */
    if (accelerator && glyph != NO_GLYPH && iflags.use_menu_glyphs) {
        unsigned dummy = 0; /* Not used */
        int color = 0;
        int symbol = 0;
        attr_t glyphclr;

        mapglyph(glyph, &symbol, &color, &dummy, u.ux, u.uy);
        glyphclr = curses_color_attr(color, 0);
        wattron(win, glyphclr);
        wprintw(win, "%c ", symbol);
        wattroff(win, glyphclr);
        available_width -= 2;
    }
#endif
    if (accelerator /* Don't colorize categories */
        && iflags.use_menu_color) {
        attr = 0;
        get_menu_coloring(str, &color, (int *) &attr);
        attr = curses_convert_attr(attr);
    }
    if (color == NO_COLOR)
        color = NONE;
    curses_toggle_color_attr(win, color, attr, ON);
    /* wattron(win, attr); */
    wprintw(win, "%.*s", available_width, str);
    /* wattroff(win, attr); */
    curses_toggle_color_attr(win, color, attr, OFF);
    wclrtoeol(win);
}

/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.7 curswins.c */
/* Copyright (c) Karl Garrison, 2010. */
/* NetHack may be freely redistributed.  See license for details. */

#include "curses.h"
#include "hack.h"
#include "wincurs.h"
#include "curswins.h"

/* Window handling for curses interface */

/* Private declarations */

typedef struct nhw {
    winid nhwin;                /* NetHack window id */
    WINDOW *curwin;             /* Curses window pointer */
    int width;                  /* Usable width not counting border */
    int height;                 /* Usable height not counting border */
    int x;                      /* start of window on terminal (left) */
    int y;                      /* start of window on termial (top) */
    int orientation;            /* Placement of window relative to map */
    boolean border;             /* Whether window has a visible border */
} nethack_window;

typedef struct nhwd {
    winid nhwid;                /* NetHack window id */
    struct nhwd *prev_wid;      /* Pointer to previous entry */
    struct nhwd *next_wid;      /* Pointer to next entry */
} nethack_wid;

typedef struct nhchar {
    int ch;                     /* character */
    int color;                  /* color info for character */
    int attr;                   /* attributes of character */
} nethack_char;

static boolean map_clipped;     /* Map window smaller than 80x21 */
static nethack_window nhwins[NHWIN_MAX];        /* NetHack window array */
static nethack_char map[ROWNO][COLNO];  /* Map window contents */
static nethack_wid *nhwids = NULL;      /* NetHack wid array */

static boolean is_main_window(winid wid);
static void write_char(WINDOW * win, int x, int y, nethack_char ch);
static void clear_map(void);

/* Create a window with the specified size and orientation */

WINDOW *
curses_create_window(int width, int height, orient orientation)
{
    int mapx = 0, mapy = 0, maph = 0, mapw = 0;
    int startx = 0;
    int starty = 0;
    WINDOW *win;
    boolean map_border = FALSE;
    int mapb_offset = 0;

    if ((orientation == UP) || (orientation == DOWN) ||
        (orientation == LEFT) || (orientation == RIGHT)) {
        if (g.invent || (g.moves > 1)) {
            map_border = curses_window_has_border(MAP_WIN);
            curses_get_window_xy(MAP_WIN, &mapx, &mapy);
            curses_get_window_size(MAP_WIN, &maph, &mapw);
        } else {
            map_border = TRUE;
            mapx = 0;
            mapy = 0;
            maph = term_rows;
            mapw = term_cols;
        }
    }

    if (map_border) {
        mapb_offset = 1;
    }

    width += 2;                 /* leave room for bounding box */
    height += 2;

    if ((width > term_cols) || (height > term_rows)) {
        impossible(
                "curses_create_window: Terminal too small for dialog window");
        width = term_cols;
        height = term_rows;
    }
    switch (orientation) {
    default:
        impossible("curses_create_window: Bad orientation");
        /*FALLTHRU*/
    case CENTER:
        startx = (term_cols / 2) - (width / 2);
        starty = (term_rows / 2) - (height / 2);
        break;
    case UP:
        if (g.invent || (g.moves > 1)) {
            startx = (mapw / 2) - (width / 2) + mapx + mapb_offset;
        } else {
            startx = 0;
        }

        starty = mapy + mapb_offset;
        break;
    case DOWN:
        if (g.invent || (g.moves > 1)) {
            startx = (mapw / 2) - (width / 2) + mapx + mapb_offset;
        } else {
            startx = 0;
        }

        starty = height - mapy - 1 - mapb_offset;
        break;
    case LEFT:
        if (map_border && (width < term_cols))
            startx = 1;
        else
            startx = 0;
        starty = term_rows - height;
        break;
    case RIGHT:
        if (g.invent || (g.moves > 1)) {
            startx = (mapw + mapx + (mapb_offset * 2)) - width;
        } else {
            startx = term_cols - width;
        }

        starty = 0;
        break;
    }

    if (startx < 0) {
        startx = 0;
    }

    if (starty < 0) {
        starty = 0;
    }

    win = newwin(height, width, starty, startx);
    curses_toggle_color_attr(win, DIALOG_BORDER_COLOR, NONE, ON);
    box(win, 0, 0);
    curses_toggle_color_attr(win, DIALOG_BORDER_COLOR, NONE, OFF);
    return win;
}


/* Erase and delete curses window, and refresh standard windows */

void
curses_destroy_win(WINDOW *win)
{
    werase(win);
    wrefresh(win);
    delwin(win);
    if (win == activemenu)
        activemenu = NULL;
    curses_refresh_nethack_windows();
}


/* Refresh nethack windows if they exist, or base window if not */

void
curses_refresh_nethack_windows(void)
{
    WINDOW *status_window, *message_window, *map_window, *inv_window;

    status_window = curses_get_nhwin(STATUS_WIN);
    message_window = curses_get_nhwin(MESSAGE_WIN);
    map_window = curses_get_nhwin(MAP_WIN);
    inv_window = curses_get_nhwin(INV_WIN);

    if ((g.moves <= 1) && !g.invent) {
        /* Main windows not yet displayed; refresh base window instead */
        touchwin(stdscr);
        refresh();
    } else {
        touchwin(status_window);
        wnoutrefresh(status_window);
        touchwin(map_window);
        wnoutrefresh(map_window);
        touchwin(message_window);
        wnoutrefresh(message_window);
        if (inv_window) {
            touchwin(inv_window);
            wnoutrefresh(inv_window);
        }
        doupdate();
    }
}


/* Return curses window pointer for given NetHack winid */

WINDOW *
curses_get_nhwin(winid wid)
{
    if (!is_main_window(wid)) {
        impossible("curses_get_nhwin: wid %d out of range. Not a main window.",
                   wid);
        return NULL;
    }

    return nhwins[wid].curwin;
}


/* Add curses window pointer and window info to list for given NetHack winid */

void
curses_add_nhwin(winid wid, int height, int width, int y, int x,
                 orient orientation, boolean border)
{
    WINDOW *win;
    int real_width = width;
    int real_height = height;

    if (!is_main_window(wid)) {
        impossible("curses_add_nhwin: wid %d out of range. Not a main window.",
                   wid);
        return;
    }

    nhwins[wid].nhwin = wid;
    nhwins[wid].border = border;
    nhwins[wid].width = width;
    nhwins[wid].height = height;
    nhwins[wid].x = x;
    nhwins[wid].y = y;
    nhwins[wid].orientation = orientation;

    if (border) {
        real_width += 2;        /* leave room for bounding box */
        real_height += 2;
    }

    win = newwin(real_height, real_width, y, x);

    switch (wid) {
    case MESSAGE_WIN:
        messagewin = win;
        break;
    case STATUS_WIN:
        statuswin = win;
        break;
    case MAP_WIN:
        mapwin = win;

        if ((width < COLNO) || (height < ROWNO)) {
            map_clipped = TRUE;
        } else {
            map_clipped = FALSE;
        }

        break;
    }

    if (border) {
        box(win, 0, 0);
    }

    nhwins[wid].curwin = win;
}


/* Add wid to list of known window IDs */

void
curses_add_wid(winid wid)
{
    nethack_wid *new_wid;
    nethack_wid *widptr = nhwids;

    new_wid = (nethack_wid *) alloc((unsigned) sizeof (nethack_wid));
    new_wid->nhwid = wid;

    new_wid->next_wid = NULL;

    if (widptr == NULL) {
        new_wid->prev_wid = NULL;
        nhwids = new_wid;
    } else {
        while (widptr->next_wid != NULL) {
            widptr = widptr->next_wid;
        }
        new_wid->prev_wid = widptr;
        widptr->next_wid = new_wid;
    }
}


/* refresh a curses window via given nethack winid */

void
curses_refresh_nhwin(winid wid)
{
    wnoutrefresh(curses_get_nhwin(wid));
    doupdate();
}


/* Delete curses window via given NetHack winid and remove entry from list */

void
curses_del_nhwin(winid wid)
{
    if (curses_is_menu(wid) || curses_is_text(wid)) {
        curses_del_menu(wid, TRUE);
        return;
    } else if (wid == INV_WIN) {
        curses_del_menu(wid, TRUE);
        /* don't return yet */
    }

    if (!is_main_window(wid)) {
        impossible("curses_del_nhwin: wid %d out of range. Not a main window.",
                   wid);
        return;
    }
    nhwins[wid].curwin = NULL;
    nhwins[wid].nhwin = -1;
}


/* Delete wid from list of known window IDs */

void
curses_del_wid(winid wid)
{
    nethack_wid *tmpwid;
    nethack_wid *widptr;

    if (curses_is_menu(wid) || curses_is_text(wid)) {
        curses_del_menu(wid, FALSE);
    }

    for (widptr = nhwids; widptr; widptr = widptr->next_wid) {
        if (widptr->nhwid == wid) {
            if ((tmpwid = widptr->prev_wid) != NULL) {
                tmpwid->next_wid = widptr->next_wid;
            } else {
                nhwids = widptr->next_wid;      /* New head mode, or NULL */
            }
            if ((tmpwid = widptr->next_wid) != NULL) {
                tmpwid->prev_wid = widptr->prev_wid;
            }
            free(widptr);
            break;
        }
    }
}

/* called by destroy_nhwindows() prior to exit */
void
curs_destroy_all_wins(void)
{
    curses_count_window((char *) 0); /* clean up orphan */

    while (nhwids)
        curses_del_wid(nhwids->nhwid);
}

/* Print a single character in the given window at the given coordinates */

void
curses_putch(winid wid, int x, int y, int ch, int color, int attr)
{
    static boolean map_initted = FALSE;
    int sx, sy, ex, ey;
    boolean border = curses_window_has_border(wid);
    nethack_char nch;
/*
    if (wid == STATUS_WIN) {
        curses_update_stats();
    }
*/
    if (wid != MAP_WIN) {
        return;
    }

    if (!map_initted) {
        clear_map();
        map_initted = TRUE;
    }

    --x; /* map column [0] is not used; draw column [1] in first screen col */
    map[y][x].ch = ch;
    map[y][x].color = color;
    map[y][x].attr = attr;
    nch = map[y][x];

    (void) curses_map_borders(&sx, &sy, &ex, &ey, -1, -1);

    if ((x >= sx) && (x <= ex) && (y >= sy) && (y <= ey)) {
        if (border) {
            x++;
            y++;
        }

        write_char(mapwin, x - sx, y - sy, nch);
    }
    /* refresh after every character?
     * Fair go, mate! Some of us are playing from Australia! */
    /* wrefresh(mapwin); */
}


/* Get x, y coordinates of curses window on the physical terminal window */

void
curses_get_window_xy(winid wid, int *x, int *y)
{
    if (!is_main_window(wid)) {
        impossible(
              "curses_get_window_xy: wid %d out of range. Not a main window.",
                   wid);
        *x = 0;
        *y = 0;
        return;
    }

    *x = nhwins[wid].x;
    *y = nhwins[wid].y;
}


/* Get usable width and height curses window on the physical terminal window */

void
curses_get_window_size(winid wid, int *height, int *width)
{
    *height = nhwins[wid].height;
    *width = nhwins[wid].width;
}


/* Determine if given window has a visible border */

boolean
curses_window_has_border(winid wid)
{
    return nhwins[wid].border;
}


/* Determine if window for given winid exists */

boolean
curses_window_exists(winid wid)
{
    nethack_wid *widptr;

    for (widptr = nhwids; widptr; widptr = widptr->next_wid)
        if (widptr->nhwid == wid)
            return TRUE;

    return FALSE;
}


/* Return the orientation of the specified window */

int
curses_get_window_orientation(winid wid)
{
    if (!is_main_window(wid)) {
        impossible(
     "curses_get_window_orientation: wid %d out of range. Not a main window.",
                   wid);
        return CENTER;
    }

    return nhwins[wid].orientation;
}


/* Output a line of text to specified NetHack window with given coordinates
   and text attributes */

void
curses_puts(winid wid, int attr, const char *text)
{
    anything Id;
    WINDOW *win = NULL;

    if (is_main_window(wid)) {
        win = curses_get_nhwin(wid);
    }

    if (wid == MESSAGE_WIN) {
        /* if a no-history message is being shown, remove it */
        if (counting)
            curses_count_window((char *) 0);

        curses_message_win_puts(text, FALSE);
        return;
    }

#if 0
    if (wid == STATUS_WIN) {
        curses_update_stats();     /* We will do the write ourselves */
        return;
    }
#endif

    if (curses_is_menu(wid) || curses_is_text(wid)) {
        if (!curses_menu_exists(wid)) {
            impossible(
                     "curses_puts: Attempted write to nonexistent window %d!",
                       wid);
            return;
        }
        Id = cg.zeroany;
        curses_add_nhmenu_item(wid, &nul_glyphinfo, &Id, 0, 0,
                               attr, text, MENU_ITEMFLAGS_NONE);
    } else {
        waddstr(win, text);
        wnoutrefresh(win);
    }
}


/* Clear the contents of a window via the given NetHack winid */

void
curses_clear_nhwin(winid wid)
{
    WINDOW *win = curses_get_nhwin(wid);
    boolean border = curses_window_has_border(wid);

    if (wid == MAP_WIN) {
        clearok(win, TRUE);     /* Redraw entire screen when refreshed */
        clear_map();
    }

    werase(win);

    if (border) {
        box(win, 0, 0);
    }
}

/* Change colour of window border to alert player to something */
void
curses_alert_win_border(winid wid, boolean onoff)
{
    WINDOW *win = curses_get_nhwin(wid);

    if (!win || !curses_window_has_border(wid))
        return;
    if (onoff)
        curses_toggle_color_attr(win, ALERT_BORDER_COLOR, NONE, ON);
    box(win, 0, 0);
    if (onoff)
        curses_toggle_color_attr(win, ALERT_BORDER_COLOR, NONE, OFF);
    wnoutrefresh(win);
}


void
curses_alert_main_borders(boolean onoff)
{
    curses_alert_win_border(MAP_WIN, onoff);
    curses_alert_win_border(MESSAGE_WIN, onoff);
    curses_alert_win_border(STATUS_WIN, onoff);
    curses_alert_win_border(INV_WIN, onoff);
}

/* Return true if given wid is a main NetHack window */

static boolean
is_main_window(winid wid)
{
    if (wid == MESSAGE_WIN || wid == MAP_WIN
        || wid == STATUS_WIN || wid == INV_WIN)
        return TRUE;

    return FALSE;
}


/* Unconditionally write a single character to a window at the given
coordinates without a refresh.  Currently only used for the map. */

static void
write_char(WINDOW * win, int x, int y, nethack_char nch)
{
    curses_toggle_color_attr(win, nch.color, nch.attr, ON);
#ifdef PDCURSES
    mvwaddrawch(win, y, x, nch.ch);
#else
    mvwaddch(win, y, x, nch.ch);
#endif
    curses_toggle_color_attr(win, nch.color, nch.attr, OFF);
}


/* Draw the entire visible map onto the screen given the visible map
boundaries */

void
curses_draw_map(int sx, int sy, int ex, int ey)
{
    int curx, cury;
    int bspace = 0;

#ifdef MAP_SCROLLBARS
    int sbsx, sbsy, sbex, sbey, count;
    nethack_char hsb_back, hsb_bar, vsb_back, vsb_bar;
#endif

    if (curses_window_has_border(MAP_WIN)) {
        bspace++;
    }
#ifdef MAP_SCROLLBARS
    hsb_back.ch = '-';
    hsb_back.color = SCROLLBAR_BACK_COLOR;
    hsb_back.attr = A_NORMAL;
    hsb_bar.ch = '*';
    hsb_bar.color = SCROLLBAR_COLOR;
    hsb_bar.attr = A_NORMAL;
    vsb_back.ch = '|';
    vsb_back.color = SCROLLBAR_BACK_COLOR;
    vsb_back.attr = A_NORMAL;
    vsb_bar.ch = '*';
    vsb_bar.color = SCROLLBAR_COLOR;
    vsb_bar.attr = A_NORMAL;

    /* Horizontal scrollbar */
    if (sx > 0 || ex < (COLNO - 1)) {
         sbsx = (int) (((long) sx * (long) (ex - sx + 1)) / (long) COLNO);
         sbex = (int) (((long) ex * (long) (ex - sx + 1)) / (long) COLNO);

        if (sx > 0 && sbsx == 0)
            ++sbsx;
        if (ex < ROWNO - 1 && sbex == ROWNO - 1)
            --sbex;

        for (count = 0; count < sbsx; count++) {
            write_char(mapwin, count + bspace, ey - sy + 1 + bspace, hsb_back);
        }

        for (count = sbsx; count <= sbex; count++) {
            write_char(mapwin, count + bspace, ey - sy + 1 + bspace, hsb_bar);
        }

        for (count = sbex + 1; count <= (ex - sx); count++) {
            write_char(mapwin, count + bspace, ey - sy + 1 + bspace, hsb_back);
        }
    }

    /* Vertical scrollbar */
    if (sy > 0 || ey < (ROWNO - 1)) {
        sbsy = (int) (((long) sy * (long) (ey - sy + 1)) / (long) ROWNO);
        sbey = (int) (((long) ey * (long) (ey - sy + 1)) / (long) ROWNO);

        if (sy > 0 && sbsy == 0)
            ++sbsy;
        if (ey < ROWNO - 1 && sbey == ROWNO - 1)
            --sbey;

        for (count = 0; count < sbsy; count++) {
            write_char(mapwin, ex - sx + 1 + bspace, count + bspace, vsb_back);
        }

        for (count = sbsy; count <= sbey; count++) {
            write_char(mapwin, ex - sx + 1 + bspace, count + bspace, vsb_bar);
        }

        for (count = sbey + 1; count <= (ey - sy); count++) {
            write_char(mapwin, ex - sx + 1 + bspace, count + bspace, vsb_back);
        }
    }
#endif /* MAP_SCROLLBARS */

    for (curx = sx; curx <= ex; curx++) {
        for (cury = sy; cury <= ey; cury++) {
            write_char(mapwin, curx - sx + bspace, cury - sy + bspace,
                       map[cury][curx]);
        }
    }
}


/* Init map array to blanks */

static void
clear_map(void)
{
    int x, y;

    for (x = 0; x < COLNO; x++) {
        for (y = 0; y < ROWNO; y++) {
            map[y][x].ch = ' ';
            map[y][x].color = NO_COLOR;
            map[y][x].attr = A_NORMAL;
        }
    }
}


/* Determine visible boundaries of map, and determine if it needs to be
based on the location of the player. */

boolean
curses_map_borders(int *sx, int *sy, int *ex, int *ey, int ux, int uy)
{
    static int width = 0;
    static int height = 0;
    static int osx = 0;
    static int osy = 0;
    static int oex = 0;
    static int oey = 0;
    static int oux = -1;
    static int ouy = -1;

    if ((oux == -1) || (ouy == -1)) {
        oux = u.ux;
        ouy = u.uy;
    }

    if (ux == -1) {
        ux = oux;
    } else {
        oux = ux;
    }

    if (uy == -1) {
        uy = ouy;
    } else {
        ouy = uy;
    }

    curses_get_window_size(MAP_WIN, &height, &width);

#ifdef MAP_SCROLLBARS
    if (width < COLNO) {
        height--;               /* room for horizontal scrollbar */
    }

    if (height < ROWNO) {
        width--;                /* room for vertical scrollbar */

        if (width == COLNO) {
            height--;
        }
    }
#endif /* MAP_SCROLLBARS */

    if (width >= COLNO) {
        *sx = 0;
        *ex = COLNO - 1;
    } else {
        *ex = (width / 2) + ux;
        *sx = *ex - (width - 1);

        if (*ex >= COLNO) {
            *sx = COLNO - width;
            *ex = COLNO - 1;
        } else if (*sx < 0) {
            *sx = 0;
            *ex = width - 1;
        }
    }

    if (height >= ROWNO) {
        *sy = 0;
        *ey = ROWNO - 1;
    } else {
        *ey = (height / 2) + uy;
        *sy = *ey - (height - 1);

        if (*ey >= ROWNO) {
            *sy = ROWNO - height;
            *ey = ROWNO - 1;
        } else if (*sy < 0) {
            *sy = 0;
            *ey = height - 1;
        }
    }

    if ((*sx != osx) || (*sy != osy) || (*ex != oex) || (*ey != oey) ||
        map_clipped) {
        osx = *sx;
        osy = *sy;
        oex = *ex;
        oey = *ey;
        return TRUE;
    }

    return FALSE;
}

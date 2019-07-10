/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.6 cursmisc.c */
/* Copyright (c) Karl Garrison, 2010. */
/* NetHack may be freely redistributed.  See license for details. */

#include "curses.h"
#include "hack.h"
#include "wincurs.h"
#include "cursmisc.h"
#include "func_tab.h"
#include "dlb.h"

#include <ctype.h>

/* Misc. curses interface functions */

/* Private declarations */

static int curs_x = -1;
static int curs_y = -1;

static int parse_escape_sequence(void);

/* Macros for Control and Alt keys */

#ifndef M
# ifndef NHSTDC
#  define M(c)          (0x80 | (c))
# else
#  define M(c)          ((c) - 128)
# endif/* NHSTDC */
#endif
#ifndef C
# define C(c)           (0x1f & (c))
#endif


/* Read a character of input from the user */

int
curses_read_char()
{
    int ch, tmpch;

    /* cancel message suppression; all messages have had a chance to be read */
    curses_got_input();

    ch = getch();
    tmpch = ch;
    ch = curses_convert_keys(ch);

    if (ch == 0) {
        ch = '\033'; /* map NUL to ESC since nethack doesn't expect NUL */
    }
#if defined(ALT_0) && defined(ALT_9)    /* PDCurses, maybe others */
    if ((ch >= ALT_0) && (ch <= ALT_9)) {
        tmpch = (ch - ALT_0) + '0';
        ch = M(tmpch);
    }
#endif

#if defined(ALT_A) && defined(ALT_Z)    /* PDCurses, maybe others */
    if ((ch >= ALT_A) && (ch <= ALT_Z)) {
        tmpch = (ch - ALT_A) + 'a';
        ch = M(tmpch);
    }
#endif

#ifdef KEY_RESIZE
    /* Handle resize events via get_nh_event, not this code */
    if (ch == KEY_RESIZE) {
        ch = '\033'; /* NetHack doesn't know what to do with KEY_RESIZE */
    }
#endif

    if (counting && !isdigit(ch)) { /* Dismiss count window if necissary */
        curses_count_window(NULL);
        curses_refresh_nethack_windows();
    }

    return ch;
}

/* Turn on or off the specified color and / or attribute */

void
curses_toggle_color_attr(WINDOW *win, int color, int attr, int onoff)
{
#ifdef TEXTCOLOR
    int curses_color;

    /* if color is disabled, just show attribute */
    if ((win == mapwin) ? !iflags.wc_color
                        /* statuswin is for #if STATUS_HILITES
                           but doesn't need to be conditional */
                        : !(iflags.wc2_guicolor || win == statuswin)) {
#endif
        if (attr != NONE) {
            if (onoff == ON)
                wattron(win, attr);
            else
                wattroff(win, attr);
        }
        return;
#ifdef TEXTCOLOR
    }

    if (color == 0) {           /* make black fg visible */
# ifdef USE_DARKGRAY
        if (iflags.wc2_darkgray) {
            if (can_change_color() && (COLORS > 16)) {
                /* colorpair for black is already darkgray */
            } else {            /* Use bold for a bright black */
                wattron(win, A_BOLD);
            }
        } else
# endif/* USE_DARKGRAY */
            color = CLR_BLUE;
    }
    curses_color = color + 1;
    if (COLORS < 16) {
        if (curses_color > 8 && curses_color < 17)
            curses_color -= 8;
        else if (curses_color > (17 + 16))
            curses_color -= 16;
    }
    if (onoff == ON) {          /* Turn on color/attributes */
        if (color != NONE) {
            if ((((color > 7) && (color < 17)) ||
                 (color > 17 + 17)) && (COLORS < 16)) {
                wattron(win, A_BOLD);
            }
            wattron(win, COLOR_PAIR(curses_color));
        }

        if (attr != NONE) {
            wattron(win, attr);
        }
    } else {                    /* Turn off color/attributes */

        if (color != NONE) {
            if ((color > 7) && (COLORS < 16)) {
                wattroff(win, A_BOLD);
            }
# ifdef USE_DARKGRAY
            if ((color == 0) && (!can_change_color() || (COLORS <= 16))) {
                wattroff(win, A_BOLD);
            }
# else
            if (iflags.use_inverse) {
                wattroff(win, A_REVERSE);
            }
# endif/* DARKGRAY */
            wattroff(win, COLOR_PAIR(curses_color));
        }

        if (attr != NONE) {
            wattroff(win, attr);
        }
    }
#else
    nhUse(color);
#endif /* TEXTCOLOR */
}


/* clean up and quit - taken from tty port */

void
curses_bail(const char *mesg)
{
    clearlocks();
    curses_exit_nhwindows(mesg);
    nh_terminate(EXIT_SUCCESS);
}


/* Return a winid for a new window of the given type */

winid
curses_get_wid(int type)
{
    static winid menu_wid = 20; /* Always even */
    static winid text_wid = 21; /* Always odd */
    winid ret;

    switch (type) {
    case NHW_MESSAGE:
        return MESSAGE_WIN;
    case NHW_MAP:
        return MAP_WIN;
    case NHW_STATUS:
        return STATUS_WIN;
    case NHW_MENU:
        ret = menu_wid;
        break;
    case NHW_TEXT:
        ret = text_wid;
        break;
    default:
        impossible("curses_get_wid: unsupported window type");
        ret = -1;
    }

    while (curses_window_exists(ret)) {
        ret += 2;
        if ((ret + 2) > 10000) {        /* Avoid "wid2k" problem */
            ret -= 9900;
        }
    }

    if (type == NHW_MENU) {
        menu_wid += 2;
    } else {
        text_wid += 2;
    }

    return ret;
}


/*
 * Allocate a copy of the given string.  If null, return a string of
 * zero length.
 *
 * This is taken from copy_of() in tty/wintty.c.
 */

char *
curses_copy_of(const char *s)
{
    if (!s)
        s = "";
    return dupstr(s);
}


/* Determine the number of lines needed for a string for a dialog window
   of the given width */

int
curses_num_lines(const char *str, int width)
{
    int last_space, count;
    int curline = 1;
    char substr[BUFSZ];
    char tmpstr[BUFSZ];

    strncpy(substr, str, BUFSZ-1);
    substr[BUFSZ-1] = '\0';

    while (strlen(substr) > (size_t) width) {
        last_space = 0;

        for (count = 0; count <= width; count++) {
            if (substr[count] == ' ')
                last_space = count;

        }
        if (last_space == 0) {  /* No spaces found */
            last_space = count - 1;
        }
        for (count = (last_space + 1); count < (int) strlen(substr); count++) {
            tmpstr[count - (last_space + 1)] = substr[count];
        }
        tmpstr[count - (last_space + 1)] = '\0';
        strcpy(substr, tmpstr);
        curline++;
    }

    return curline;
}


/* Break string into smaller lines to fit into a dialog window of the
given width */

char *
curses_break_str(const char *str, int width, int line_num)
{
    int last_space, count;
    char *retstr;
    int curline = 0;
    int strsize = (int) strlen(str) + 1;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    char substr[strsize];
    char curstr[strsize];
    char tmpstr[strsize];

    strcpy(substr, str);
#else
#ifndef BUFSZ
#define BUFSZ 256
#endif
    char substr[BUFSZ * 2];
    char curstr[BUFSZ * 2];
    char tmpstr[BUFSZ * 2];

    if (strsize > (BUFSZ * 2) - 1) {
        paniclog("curses", "curses_break_str() string too long.");
        strncpy(substr, str, (BUFSZ * 2) - 2);
        substr[(BUFSZ * 2) - 1] = '\0';
    } else
        strcpy(substr, str);
#endif

    while (curline < line_num) {
        if (strlen(substr) == 0) {
            break;
        }
        curline++;
        last_space = 0;
        for (count = 0; count <= width; count++) {
            if (substr[count] == ' ') {
                last_space = count;
            } else if (substr[count] == '\0') {
                last_space = count;
                break;
            }
        }
        if (last_space == 0) {  /* No spaces found */
            last_space = count - 1;
        }
        for (count = 0; count < last_space; count++) {
            curstr[count] = substr[count];
        }
        curstr[count] = '\0';
        if (substr[count] == '\0') {
            break;
        }
        for (count = (last_space + 1); count < (int) strlen(substr); count++) {
            tmpstr[count - (last_space + 1)] = substr[count];
        }
        tmpstr[count - (last_space + 1)] = '\0';
        strcpy(substr, tmpstr);
    }

    if (curline < line_num) {
        return NULL;
    }

    retstr = curses_copy_of(curstr);

    return retstr;
}


/* Return the remaining portion of a string after hacking-off line_num lines */

char *
curses_str_remainder(const char *str, int width, int line_num)
{
    int last_space, count;
    char *retstr;
    int curline = 0;
    int strsize = strlen(str) + 1;
#if __STDC_VERSION__ >= 199901L
    char substr[strsize];
    char curstr[strsize];
    char tmpstr[strsize];

    strcpy(substr, str);
#else
#ifndef BUFSZ
#define BUFSZ 256
#endif
    char substr[BUFSZ * 2];
    char curstr[BUFSZ * 2];
    char tmpstr[BUFSZ * 2];

    if (strsize > (BUFSZ * 2) - 1) {
        paniclog("curses", "curses_str_remainder() string too long.");
        strncpy(substr, str, (BUFSZ * 2) - 2);
        substr[(BUFSZ * 2) - 1] = '\0';
    } else
        strcpy(substr, str);
#endif

    while (curline < line_num) {
        if (strlen(substr) == 0) {
            break;
        }
        curline++;
        last_space = 0;
        for (count = 0; count <= width; count++) {
            if (substr[count] == ' ') {
                last_space = count;
            } else if (substr[count] == '\0') {
                last_space = count;
                break;
            }
        }
        if (last_space == 0) {  /* No spaces found */
            last_space = count - 1;
        }
        for (count = 0; count < last_space; count++) {
            curstr[count] = substr[count];
        }
        curstr[count] = '\0';
        if (substr[count] == '\0') {
            break;
        }
        for (count = (last_space + 1); count < (int) strlen(substr); count++) {
            tmpstr[count - (last_space + 1)] = substr[count];
        }
        tmpstr[count - (last_space + 1)] = '\0';
        strcpy(substr, tmpstr);
    }

    if (curline < line_num) {
        return NULL;
    }

    retstr = curses_copy_of(substr);

    return retstr;
}


/* Determine if the given NetHack winid is a menu window */

boolean
curses_is_menu(winid wid)
{
    if ((wid > 19) && !(wid % 2)) {     /* Even number */
        return TRUE;
    } else {
        return FALSE;
    }
}


/* Determine if the given NetHack winid is a text window */

boolean
curses_is_text(winid wid)
{
    if ((wid > 19) && (wid % 2)) {      /* Odd number */
        return TRUE;
    } else {
        return FALSE;
    }
}


/* Replace certain characters with portable drawing characters if
cursesgraphics option is enabled */

int
curses_convert_glyph(int ch, int glyph)
{
    int symbol;

    if (Is_rogue_level(&u.uz)) {
        return ch;
    }

    /* Save some processing time by returning if the glyph represents
       an object that we don't have custom characters for */
    if (!glyph_is_cmap(glyph)) {
        return ch;
    }

    symbol = glyph_to_cmap(glyph);

    /* If user selected a custom character for this object, don't
       override this. */
    if (((glyph_is_cmap(glyph)) && (ch != showsyms[symbol]))) {
        return ch;
    }

    switch (symbol) {
    case S_vwall:
        return ACS_VLINE;
    case S_hwall:
        return ACS_HLINE;
    case S_tlcorn:
        return ACS_ULCORNER;
    case S_trcorn:
        return ACS_URCORNER;
    case S_blcorn:
        return ACS_LLCORNER;
    case S_brcorn:
        return ACS_LRCORNER;
    case S_crwall:
        return ACS_PLUS;
    case S_tuwall:
        return ACS_BTEE;
    case S_tdwall:
        return ACS_TTEE;
    case S_tlwall:
        return ACS_RTEE;
    case S_trwall:
        return ACS_LTEE;
    case S_tree:
        return ACS_PLMINUS;
    case S_corr:
        return ACS_CKBOARD;
    case S_litcorr:
        return ACS_CKBOARD;
    }

    return ch;
}


/* Move text cursor to specified coordinates in the given NetHack window */

void
curses_move_cursor(winid wid, int x, int y)
{
    int sx, sy, ex, ey;
    int xadj = 0;
    int yadj = 0;

#ifndef PDCURSES
    WINDOW *win = curses_get_nhwin(MAP_WIN);
#endif

    if (wid != MAP_WIN) {
        return;
    }
#ifdef PDCURSES
    /* PDCurses seems to not handle wmove correctly, so we use move and
       physical screen coordinates instead */
    curses_get_window_xy(wid, &xadj, &yadj);
#endif
    curs_x = x + xadj;
    curs_y = y + yadj;
    curses_map_borders(&sx, &sy, &ex, &ey, x, y);

    if (curses_window_has_border(wid)) {
        curs_x++;
        curs_y++;
    }

    if (x >= sx && x <= ex && y >= sy && y <= ey) {
        /* map column #0 isn't used; shift column #1 to first screen column */
        curs_x -= (sx + 1);
        curs_y -= sy;
#ifdef PDCURSES
        move(curs_y, curs_x);
#else
        wmove(win, curs_y, curs_x);
#endif
    }
}


/* Perform actions that should be done every turn before nhgetch() */

void
curses_prehousekeeping()
{
#ifndef PDCURSES
    WINDOW *win = curses_get_nhwin(MAP_WIN);
#endif /* PDCURSES */

    if ((curs_x > -1) && (curs_y > -1)) {
        curs_set(1);
#ifdef PDCURSES
        /* PDCurses seems to not handle wmove correctly, so we use move
           and physical screen coordinates instead */
        move(curs_y, curs_x);
#else
        wmove(win, curs_y, curs_x);
#endif /* PDCURSES */
        curses_refresh_nhwin(MAP_WIN);
    }
}


/* Perform actions that should be done every turn after nhgetch() */

void
curses_posthousekeeping()
{
    curs_set(0);
    /* curses_decrement_highlights(FALSE); */
    curses_clear_unhighlight_message_window();
}


void
curses_view_file(const char *filename, boolean must_exist)
{
    winid wid;
    anything Id;
    char buf[BUFSZ];
    menu_item *selected = NULL;
    dlb *fp = dlb_fopen(filename, "r");

    if (fp == NULL) {
        if (must_exist)
            pline("Cannot open \"%s\" for reading!", filename);
        return;
    }

    wid = curses_get_wid(NHW_MENU);
    curses_create_nhmenu(wid);
    Id = zeroany;

    while (dlb_fgets(buf, BUFSZ, fp) != NULL) {
        curses_add_menu(wid, NO_GLYPH, &Id, 0, 0, A_NORMAL, buf, FALSE);
    }

    dlb_fclose(fp);
    curses_end_menu(wid, "");
    curses_select_menu(wid, PICK_NONE, &selected);
    curses_del_wid(wid);
}


void
curses_rtrim(char *str)
{
    char *s;

    for (s = str; *s != '\0'; ++s);
    for (--s; isspace(*s) && s > str; --s);
    if (s == str)
        *s = '\0';
    else
        *(++s) = '\0';
}


/* Read numbers until non-digit is encountered, and return number
in int form. */

int
curses_get_count(int first_digit)
{
    long current_count = first_digit;
    int current_char;

    current_char = curses_read_char();

    while (isdigit(current_char)) {
        current_count = (current_count * 10) + (current_char - '0');
        if (current_count > LARGEST_INT) {
            current_count = LARGEST_INT;
        }

        custompline(SUPPRESS_HISTORY, "Count: %ld", current_count);
        current_char = curses_read_char();
    }

    ungetch(current_char);

    if (current_char == '\033') {     /* Cancelled with escape */
        current_count = -1;
    }

    return current_count;
}


/* Convert the given NetHack text attributes into the format curses
   understands, and return that format mask. */

int
curses_convert_attr(int attr)
{
    int curses_attr;

    /* first, strip off control flags masked onto the display attributes
       (caller should have already done this...) */
    attr &= ~(ATR_URGENT | ATR_NOHISTORY);

    switch (attr) {
    case ATR_NONE:
        curses_attr = A_NORMAL;
        break;
    case ATR_ULINE:
        curses_attr = A_UNDERLINE;
        break;
    case ATR_BOLD:
        curses_attr = A_BOLD;
        break;
    case ATR_DIM:
        curses_attr = A_DIM;
        break;
    case ATR_BLINK:
        curses_attr = A_BLINK;
        break;
    case ATR_INVERSE:
        curses_attr = A_REVERSE;
        break;
    default:
        curses_attr = A_NORMAL;
    }

    return curses_attr;
}


/* Map letter attributes from a string to bitmask.  Return mask on
   success (might be 0), or -1 if not found. */

int
curses_read_attrs(const char *attrs)
{
    int retattr = 0;

    if (!attrs || !*attrs)
        return A_NORMAL;

    if (strchr(attrs, 'b') || strchr(attrs, 'B'))
        retattr |= A_BOLD;
    if (strchr(attrs, 'i') || strchr(attrs, 'I')) /* inverse */
        retattr |= A_REVERSE;
    if (strchr(attrs, 'u') || strchr(attrs, 'U'))
        retattr |= A_UNDERLINE;
    if (strchr(attrs, 'k') || strchr(attrs, 'K'))
        retattr |= A_BLINK;
    if (strchr(attrs, 'd') || strchr(attrs, 'D'))
        retattr |= A_DIM;
#ifdef A_ITALIC
    if (strchr(attrs, 't') || strchr(attrs, 'T'))
        retattr |= A_ITALIC;
#endif
#ifdef A_LEFTLINE
    if (strchr(attrs, 'l') || strchr(attrs, 'L'))
        retattr |= A_LEFTLINE;
#endif
#ifdef A_RIGHTLINE
    if (strchr(attrs, 'r') || strchr(attrs, 'R'))
        retattr |= A_RIGHTLINE;
#endif
    if (retattr == 0) {
        /* still default; check for none/normal */
        if (strchr(attrs, 'n') || strchr(attrs, 'N'))
            retattr = A_NORMAL;
        else
            retattr = -1; /* error */
    }
    return retattr;
}

/* format iflags.wc2_petattr into "+a+b..." for set bits a, b, ...
   (used by core's 'O' command; return value points past leading '+') */
char *
curses_fmt_attrs(outbuf)
char *outbuf;
{
    int attr = iflags.wc2_petattr;

    outbuf[0] = '\0';
    if (attr == A_NORMAL) {
        Strcpy(outbuf, "+N(None)");
    } else {
        if (attr & A_BOLD)
            Strcat(outbuf, "+B(Bold)");
        if (attr & A_REVERSE)
            Strcat(outbuf, "+I(Inverse)");
        if (attr & A_UNDERLINE)
            Strcat(outbuf, "+U(Underline)");
        if (attr & A_BLINK)
            Strcat(outbuf, "+K(blinK)");
        if (attr & A_DIM)
            Strcat(outbuf, "+D(Dim)");
#ifdef A_ITALIC
        if (attr & A_ITALIC)
            Strcat(outbuf, "+T(iTalic)");
#endif
#ifdef A_LEFTLINE
        if (attr & A_LEFTLINE)
            Strcat(outbuf, "+L(Left line)");
#endif
#ifdef A_RIGHTLINE
        if (attr & A_RIGHTLINE)
            Strcat(outbuf, "+R(Right line)");
#endif
    }
    if (!*outbuf)
        Sprintf(outbuf, "+unknown [%d]", attr);
    return &outbuf[1];
}

/* Convert special keys into values that NetHack can understand.
Currently this is limited to arrow keys, but this may be expanded. */

int
curses_convert_keys(int key)
{
    int ret = key;

    if (ret == '\033') {
        ret = parse_escape_sequence();
    }

    /* Handle arrow keys */
    switch (key) {
    case KEY_BACKSPACE:
        /* we can't distinguish between a separate backspace key and
           explicit Ctrl+H intended to rush to the left; without this,
           a value for ^H greater than 255 is passed back to core's
           readchar() and stripping the value down to 0..255 yields ^G! */
        ret = C('H');
        break;
    case KEY_LEFT:
        if (iflags.num_pad) {
            ret = '4';
        } else {
            ret = 'h';
        }
        break;
    case KEY_RIGHT:
        if (iflags.num_pad) {
            ret = '6';
        } else {
            ret = 'l';
        }
        break;
    case KEY_UP:
        if (iflags.num_pad) {
            ret = '8';
        } else {
            ret = 'k';
        }
        break;
    case KEY_DOWN:
        if (iflags.num_pad) {
            ret = '2';
        } else {
            ret = 'j';
        }
        break;
#ifdef KEY_A1
    case KEY_A1:
        if (iflags.num_pad) {
            ret = '7';
        } else {
            ret = 'y';
        }
        break;
#endif /* KEY_A1 */
#ifdef KEY_A3
    case KEY_A3:
        if (iflags.num_pad) {
            ret = '9';
        } else {
            ret = 'u';
        }
        break;
#endif /* KEY_A3 */
#ifdef KEY_C1
    case KEY_C1:
        if (iflags.num_pad) {
            ret = '1';
        } else {
            ret = 'b';
        }
        break;
#endif /* KEY_C1 */
#ifdef KEY_C3
    case KEY_C3:
        if (iflags.num_pad) {
            ret = '3';
        } else {
            ret = 'n';
        }
        break;
#endif /* KEY_C3 */
#ifdef KEY_B2
    case KEY_B2:
        if (iflags.num_pad) {
            ret = '5';
        } else {
            ret = 'g';
        }
        break;
#endif /* KEY_B2 */
    }

    return ret;
}

/*
 * We treat buttons 2 and 3 as equivalent so that it doesn't matter which
 * one is for right-click and which for middle-click.  The core uses CLICK_2
 * for right-click ("not left"-click) even though 2 might be middle button.
 *
 * BUTTON_CTRL was enabled at one point but was not working as intended.
 * Ctrl+left_click was generating pairs of duplicated events with Ctrl and
 * Report_mouse_position bits set (even though Report_mouse_position wasn't
 * enabled) but no button click bit set.  (It sort of worked because Ctrl+
 * Report_mouse_position wasn't a left click so passed along CLICK_2, but
 * the duplication made that too annoying to use.  Attempting to immediately
 * drain the second one wasn't working as intended either.)
 */
#define MOUSEBUTTONS (BUTTON1_CLICKED | BUTTON2_CLICKED | BUTTON3_CLICKED)

/* Process mouse events.  Mouse movement is processed until no further
mouse movement events are available.  Returns 0 for a mouse click
event, or the first non-mouse key event in the case of mouse
movement. */

int
curses_get_mouse(int *mousex, int *mousey, int *mod)
{
    int key = '\033';

#ifdef NCURSES_MOUSE_VERSION
    MEVENT event;

    if (getmouse(&event) == OK) { /* True if user has clicked */
        if ((event.bstate & MOUSEBUTTONS) != 0) {
        /*
         * The ncurses man page documents wmouse_trafo() incorrectly.
         * It says that last argument 'TRUE' translates from screen
         * to window and 'FALSE' translates from window to screen,
         * but those are backwards.  The mouse_trafo() macro calls
         * last argument 'to_screen', suggesting that the backwards
         * implementation is the intended behavior and the man page
         * is describing it wrong.
         */
            /* See if coords are in map window & convert coords */
            if (wmouse_trafo(mapwin, &event.y, &event.x, FALSE)) {
                key = '\0'; /* core uses this to detect a mouse click */
                *mousex = event.x + 1; /* +1: screen 0..78 is map 1..79 */
                *mousey = event.y;

                if (curses_window_has_border(MAP_WIN)) {
                    (*mousex)--;
                    (*mousey)--;
                }

                *mod = ((event.bstate & (BUTTON1_CLICKED | BUTTON_CTRL))
                        == BUTTON1_CLICKED) ? CLICK_1 : CLICK_2;
            }
        }
    }
#endif /* NCURSES_MOUSE_VERSION */

    return key;
}

void
curses_mouse_support(mode)
int mode; /* 0: off, 1: on, 2: alternate on */
{
#ifdef NCURSES_MOUSE_VERSION
    mmask_t result, oldmask, newmask;

    if (!mode)
        newmask = 0;
    else
        newmask = MOUSEBUTTONS; /* buttons 1, 2, and 3 */

    result = mousemask(newmask, &oldmask);
    nhUse(result);
#else
    nhUse(mode);
#endif
}

static int
parse_escape_sequence(void)
{
#ifndef PDCURSES
    int ret;

    timeout(10);

    ret = getch();

    if (ret != ERR) {           /* Likely an escape sequence */
        if (((ret >= 'a') && (ret <= 'z')) || ((ret >= '0') && (ret <= '9'))) {
            ret |= 0x80;        /* Meta key support for most terminals */
        } else if (ret == 'O') {        /* Numeric keypad */
            ret = getch();
            if ((ret != ERR) && (ret >= 112) && (ret <= 121)) {
                ret = ret - 112 + '0';  /* Convert to number */
            } else {
                ret = '\033';   /* Escape */
            }
        }
    } else {
        ret = '\033';           /* Just an escape character */
    }

    timeout(-1);

    return ret;
#else
    return '\033';
#endif /* !PDCURSES */
}


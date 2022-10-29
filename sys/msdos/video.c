/* NetHack 3.7	video.c	$NHDT-Date: 1596498277 2020/08/03 23:44:37 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.16 $ */
/*   Copyright (c) NetHack PC Development Team 1993, 1994, 2001     */
/*   NetHack may be freely redistributed.  See license for details. */

/*
 * video.c - Hardware video support front-ends
 *
 *Edit History:
 *     Initial Creation              M. Allison      1993/04/04
 *     Add djgpp support             K. Smolkowski   1993/04/26
 *     Add txt/graphics mode support M. Allison      1993/10/30
 *     Add graphics mode cursor sim. M. Allison      1994/02/19
 *     Add hooks for decals on vga   M. Allison      2001/04/07
 */

#include "hack.h"

#ifndef STUBVIDEO
#include "pcvideo.h"
#include "pctiles.h"

#if defined(_MSC_VER)
#if _MSC_VER >= 700
#pragma warning(disable : 4018) /* signed/unsigned mismatch */
#pragma warning(disable : 4127) /* conditional expression is constant */
#pragma warning(disable : 4131) /* old style declarator */
#pragma warning(disable : 4305) /* prevents complaints with MK_FP */
#pragma warning(disable : 4309) /* initializing */
#pragma warning(disable : 4759) /* prevents complaints with MK_FP */
#endif
#endif
/*=========================================================================
 * General PC Video routines.
 *
 * The following routines are the video interfacing functions.
 * In general these make calls to more hardware specific
 * routines in other source files.
 *
 * Assumptions (94/04/23):
 *
 *   - Supported defaults.nh file video options:
 *
 *          If OPTIONS=video:autodetect is defined in defaults.nh then
 *          check for a VGA video adapter.  If one is detected, then
 *          use the VGA code, otherwise resort to using the 'standard'
 *          video BIOS routines.
 *
 *          If OPTIONS=video:vga is defined in defaults.nh, then use
 *          the VGA code.
 *
 *          If OPTIONS=video:default is defined in defaults.nh use the
 *          'standard' video BIOS routines (in the overlaid version),
 *          or DJGPPFAST routines (under djgpp). This is equivalent to
 *          having no OPTIONS=video:xxxx entry at all.
 *
 * Notes (94/04/23):
 *
 *   - The handler for defaults.nh file entry:
 *
 *           OPTIONS=video:xxxxx
 *
 *     has now been added.  The handler is in video.c and is called
 *     from options.c.
 *
 *   - Handling of videocolors and videoshades are now done with
 *     OPTIONS= statements.  The new syntax separates the colour
 *     values with dashes ('-') rather than spaces (' ').
 *
 * To Do (94/04/23):
 *
 *
 *=========================================================================
 */

void
get_scr_size(void)
{
#ifdef SCREEN_VGA
    if (iflags.usevga) {
        vga_get_scr_size();
    } else
#endif
#ifdef SCREEN_VESA
    if (iflags.usevesa) {
        vesa_get_scr_size();
    } else
#endif
        txt_get_scr_size();
}

/*
 * --------------------------------------------------------------
 * The rest of this file is only compiled if NO_TERMS is defined.
 * --------------------------------------------------------------
 */

#ifdef NO_TERMS

#include <ctype.h>
#include "wintty.h"

#ifdef __GO32__
#include <pc.h>
#include <unistd.h>
#if !(__DJGPP__ >= 2)
typedef long clock_t;
#endif
#endif

#ifdef __BORLANDC__
#include <dos.h> /* needed for delay() */
#endif

#ifdef SCREEN_DJGPPFAST /* parts of this block may be unecessary now */
#define get_cursor(x, y) ScreenGetCursor(y, x)
#endif

#ifdef SCREEN_BIOS
void get_cursor(int *, int *);
#endif

void adjust_cursor_flags(struct WinDesc *);
void cmov(int, int);
void nocmov(int, int);
static void init_ttycolor(void);

int savevmode;               /* store the original video mode in here */
int curcol, currow;          /* graphics mode current cursor locations */
int g_attribute;             /* Current attribute to use */
int monoflag;                /* 0 = not monochrome, else monochrome */
int attrib_text_normal;      /* text mode normal attribute */
int attrib_gr_normal;        /* graphics mode normal attribute */
int attrib_text_intense;     /* text mode intense attribute */
int attrib_gr_intense;       /* graphics mode intense attribute */
boolean traditional = FALSE; /* traditonal TTY character mode */
boolean inmap = FALSE;       /* in the map window */
#ifdef TEXTCOLOR
char ttycolors[CLR_MAX]; /* also used/set in options.c */
#endif                   /* TEXTCOLOR */

void
backsp(void)
{
    if (!iflags.grmode) {
        txt_backsp();
#ifdef SCREEN_VGA
    } else if (iflags.usevga) {
        vga_backsp();
#endif
#ifdef SCREEN_VESA
    } else if (iflags.usevesa) {
        vesa_backsp();
#endif
    }
}

void
clear_screen(void)
{
    if (!iflags.grmode) {
        txt_clear_screen();
#ifdef SCREEN_VGA
    } else if (iflags.usevga) {
        vga_clear_screen(BACKGROUND_VGA_COLOR);
#endif
#ifdef SCREEN_VESA
    } else if (iflags.usevesa) {
        vesa_clear_screen(BACKGROUND_VGA_COLOR);
#endif
    }
}

void cl_end(void) /* clear to end of line */
{
    int col, row;

    col = (int) ttyDisplay->curx;
    row = (int) ttyDisplay->cury;
    if (!iflags.grmode) {
        txt_cl_end(col, row);
#ifdef SCREEN_VGA
    } else if (iflags.usevga) {
        vga_cl_end(col, row);
#endif
#ifdef SCREEN_VESA
    } else if (iflags.usevesa) {
        vesa_cl_end(col, row);
#endif
    }
    tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1, (int) ttyDisplay->cury);
}

void cl_eos(void) /* clear to end of screen */
{
    int cy = (int) ttyDisplay->cury + 1;

    if (!iflags.grmode) {
        txt_cl_eos();
#ifdef SCREEN_VGA
    } else if (iflags.usevga) {
        vga_cl_eos(cy);
#endif
#ifdef SCREEN_VESA
    } else if (iflags.usevesa) {
        vesa_cl_eos(cy);
#endif
    }
    tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1, (int) ttyDisplay->cury);
}

void
cmov(int col, int row)
{
    ttyDisplay->cury = (uchar) row;
    ttyDisplay->curx = (uchar) col;
    if (!iflags.grmode) {
        txt_gotoxy(col, row);
#ifdef SCREEN_VGA
    } else if (iflags.usevga) {
        vga_gotoloc(col, row);
#endif
#ifdef SCREEN_VESA
    } else if (iflags.usevesa) {
        vesa_gotoloc(col, row);
#endif
    }
}

#if 0
int
has_color(int color)
{
    ++color; /* prevents compiler warning (unref. param) */
#ifdef TEXTCOLOR
    return (monoflag) ? 0 : 1;
#else
    return 0;
#endif
}
#endif

void
home(void)
{
    tty_curs(BASE_WINDOW, 1, 0);
    ttyDisplay->curx = ttyDisplay->cury = (uchar) 0;
    if (!iflags.grmode) {
        txt_gotoxy(0, 0);
#ifdef SCREEN_VGA
    } else if (iflags.usevga) {
        vga_gotoloc(0, 0);
#endif
#ifdef SCREEN_VESA
    } else if (iflags.usevesa) {
        vesa_gotoloc(0, 0);
#endif
    }
}

void
nocmov(int col, int row)
{
    if (!iflags.grmode) {
        txt_gotoxy(col, row);
#ifdef SCREEN_VGA
    } else if (iflags.usevga) {
        vga_gotoloc(col, row);
#endif
#ifdef SCREEN_VESA
    } else if (iflags.usevesa) {
        vesa_gotoloc(col, row);
#endif
    }
}

void
standoutbeg(void)
{
    g_attribute = iflags.grmode ? attrib_gr_intense : attrib_text_intense;
}

void
standoutend(void)
{
    g_attribute = iflags.grmode ? attrib_gr_normal : attrib_text_normal;
}

int
term_attr_fixup(int attrmask)
{
    return attrmask;
}

void
term_end_attr(int attr)
{
    switch (attr) {
    case ATR_ULINE:
    case ATR_BOLD:
    case ATR_BLINK:
    case ATR_INVERSE:
    default:
        g_attribute = iflags.grmode ? attrib_gr_normal : attrib_text_normal;
    }
}

void
term_end_color(void)
{
    g_attribute = iflags.grmode ? attrib_gr_normal : attrib_text_normal;
}

void
term_end_raw_bold(void)
{
    standoutend();
}

void
term_start_attr(int attr)
{
    switch (attr) {
    case ATR_ULINE:
        if (monoflag) {
            g_attribute = ATTRIB_MONO_UNDERLINE;
        } else {
            g_attribute =
                iflags.grmode ? attrib_gr_intense : attrib_text_intense;
        }
        break;
    case ATR_BOLD:
        g_attribute = iflags.grmode ? attrib_gr_intense : attrib_text_intense;
        break;
    case ATR_BLINK:
        if (monoflag) {
            g_attribute = ATTRIB_MONO_BLINK;
        } else {
            g_attribute =
                iflags.grmode ? attrib_gr_intense : attrib_text_intense;
        }
        break;
    case ATR_INVERSE:
        if (monoflag) {
            g_attribute = ATTRIB_MONO_REVERSE;
        } else {
            g_attribute =
                iflags.grmode ? attrib_gr_intense : attrib_text_intense;
        }
        break;
    default:
        g_attribute = iflags.grmode ? attrib_gr_normal : attrib_text_normal;
        break;
    }
}

void
term_start_color(int color)
{
#ifdef TEXTCOLOR
    if (monoflag) {
        g_attribute = attrib_text_normal;
    } else {
        if (color >= 0 && color < CLR_MAX) {
            if (iflags.grmode)
                g_attribute = color;
            else
                g_attribute = ttycolors[color];
        }
    }
#endif
}

void
term_start_raw_bold(void)
{
    standoutbeg();
}

void
tty_delay_output(void)
{
#ifdef TIMED_DELAY
    if (flags.nap) {
        (void) fflush(stdout);
        msleep(50); /* sleep for 50 milliseconds */
        return;
    }
#endif
}

void
tty_end_screen(void)
{
    if (!iflags.grmode) {
        txt_clear_screen();
#ifdef PC9800
        fputs("\033[>1l", stdout);
#endif
#ifdef SCREEN_VGA
    } else if (iflags.usevga) {
        vga_tty_end_screen();
#endif
#ifdef SCREEN_VESA
    } else if (iflags.usevesa) {
        vesa_tty_end_screen();
#endif
    }
}

void
tty_nhbell(void)
{
    txt_nhbell();
}

void
tty_number_pad(int state)
{
    ++state; /* prevents compiler warning (unref. param) */
}

void
tty_startup(int *wid, int *hgt)
{
    /* code to sense display adapter is required here - MJA */

    attrib_text_normal = ATTRIB_NORMAL;
    attrib_text_intense = ATTRIB_INTENSE;

    /* These are defaults and may get overridden */
    attrib_gr_normal = attrib_text_normal;
    attrib_gr_intense = attrib_text_intense;
    g_attribute = attrib_text_normal; /* Give it a starting value */

#ifdef SCREEN_VGA
    if (iflags.usevga) {
        vga_tty_startup(wid, hgt);
    } else
#endif
#ifdef SCREEN_VESA
    if (iflags.usevesa) {
        vesa_tty_startup(wid, hgt);
    } else
#endif
        txt_startup(wid, hgt);

    *wid = CO;
    *hgt = LI;

#ifdef CLIPPING
    if (CO < COLNO || LI < ROWNO + 3)
        setclipped();
#endif

#ifdef TEXTCOLOR
    init_ttycolor();
#endif

#ifdef MONO_CHECK
    monoflag = txt_monoadapt_check();
#else
    monoflag = 0;
#endif
}

void
tty_start_screen(void)
{
#ifdef PC9800
    fputs("\033[>1h", stdout);
#endif
    if (iflags.num_pad)
        tty_number_pad(1); /* make keypad send digits */
}

void
gr_init(void)
{
    windowprocs.wincap2 &= ~WC2_U_24BITCOLOR;
#ifdef SCREEN_VGA
    if (iflags.usevga) {
        vga_Init();
    } else
#endif
#ifdef SCREEN_VESA
    if (iflags.usevesa) {
        vesa_Init();
    } else
#endif
#ifdef SCREEN_8514
    if (iflags.use8514) {
        v8514_Init();
    } else
#endif
    {}
}

void
gr_finish(void)
{
    if (iflags.grmode) {
#ifdef SCREEN_VGA
        if (iflags.usevga) {
            vga_Finish();
        } else
#endif
#ifdef SCREEN_VESA
        if (iflags.usevesa) {
            vesa_Finish();
        } else
#endif
#ifdef SCREEN_8514
        if (iflags.use8514) {
            v8514_Finish();
        } else
#endif
        {}
    }
}

/*
 * Screen output routines (these are heavily used).
 *
 * These are the 3 routines used to place information on the screen
 * in the NO_TERMS PC tty port of NetHack.  These are the routines
 * that get called by routines in other NetHack source files (such
 * as those in win/tty).
 *
 * xputs - Writes a c null terminated string at the current location.
 *         Depending on compile options, this could just be a series
 *         of repeated calls to xputc() for each character.
 *
 * xputc - Writes a single character at the current location. Since
 *         various places in the code assume that control characters
 *         can be used to control, we are forced to interpret some of
 *         the more common ones, in order to keep things looking correct.
 *
 * xputg - If using a graphics mode display mechanism (such as VGA, this
 *         routine is used to display a graphical representation of a
 *         NetHack glyph at the current location.  For more information on
 *         NetHack glyphs refer to the comments in include/display.h.
 *
 * NOTES:
 *         wintty.h uses macros to redefine common output functions
 *         such as puts, putc, putchar, so that they get steered into
 *         either xputs (for strings) or xputc (for single characters).
 *         References to puts, putc, and putchar in other source files
 *         (that include wintty.h) are actually using these routines.
 */

void
xputs(const char *s)
{
    int col, row;

    col = (int) ttyDisplay->curx;
    row = (int) ttyDisplay->cury;

    if (!iflags.grmode) {
        txt_xputs(s, col, row);
#ifdef SCREEN_VGA
    } else if (iflags.usevga) {
        vga_xputs(s, col, row);
#endif
#ifdef SCREEN_VESA
    } else if (iflags.usevesa) {
        vesa_xputs(s, col, row);
#endif
    }
}

/* same signature as 'putchar()' with potential failure result ignored */
int
xputc(int ch) /* write out character (and attribute) */
{
    int i;
    char attribute;

    i = iflags.grmode ? attrib_gr_normal : attrib_text_normal;

    attribute = (char) ((g_attribute == 0) ? i : g_attribute);
    if (!iflags.grmode) {
        txt_xputc(ch, attribute);
#ifdef SCREEN_VGA
    } else if (iflags.usevga) {
        vga_xputc(ch, attribute);
#endif /*SCREEN_VGA*/
#ifdef SCREEN_VESA
    } else if (iflags.usevesa) {
        vesa_xputc(ch, attribute);
#endif /*SCREEN_VESA*/
    }
    return 0;
}

/* write out a glyph picture at current location */
void xputg(const glyph_info *glyphinfo)
{
    if (!iflags.grmode || !iflags.tile_view) {
        (void) xputc((char) glyphinfo->ttychar);
#ifdef SCREEN_VGA
    } else if (iflags.grmode && iflags.usevga) {
        vga_xputg(glyphinfo);
#endif
#ifdef SCREEN_VESA
    } else if (iflags.grmode && iflags.usevesa) {
        vesa_xputg(glyphinfo);
#endif
    }
}

#ifdef POSITIONBAR
void
video_update_positionbar(char *posbar)
{
    if (!iflags.grmode)
        return;
#ifdef SCREEN_VGA
    else if (iflags.usevga)
        vga_update_positionbar(posbar);
#endif
#ifdef SCREEN_VESA
    else if (iflags.usevesa)
        vesa_update_positionbar(posbar);
#endif
}
#endif

void
adjust_cursor_flags(struct WinDesc *cw)
{
#ifdef SIMULATE_CURSOR
#if 0
    if (cw->type == NHW_MAP) cursor_flag = 1;
    else cursor_flag = 0;
#else
    if (cw->type == NHW_MAP) {
        inmap = 1;
        cursor_flag = 1;
    } else {
        inmap = 0;
        cursor_flag = 1;
    }
#endif /* 0 */
#endif /* SIMULATE_CURSOR */
}

#ifdef SIMULATE_CURSOR

/* change the defaults in pcvideo.h, not here */
int cursor_type = CURSOR_DEFAULT_STYLE;
int cursor_color = CURSOR_DEFAULT_COLOR;
int cursor_flag;

/* The check for iflags.grmode is made BEFORE calling these. */
void
DrawCursor(void)
{
#ifdef SCREEN_VGA
    if (iflags.usevga)
        vga_DrawCursor();
#endif
#ifdef SCREEN_VESA
    if (iflags.usevesa)
        vesa_DrawCursor();
#endif
}

void
HideCursor(void)
{
#ifdef SCREEN_VGA
    if (iflags.usevga)
        vga_HideCursor();
#endif
#ifdef SCREEN_VESA
    if (iflags.usevesa)
        vesa_HideCursor();
#endif
}

#endif /* SIMULATE_CURSOR */

#ifdef TEXTCOLOR
/*
 * CLR_BLACK            0
 * CLR_RED              1
 * CLR_GREEN            2
 * CLR_BROWN            3       low-intensity yellow
 * CLR_BLUE             4
 * CLR_MAGENTA          5
 * CLR_CYAN             6
 * CLR_GRAY             7       low-intensity white
 * NO_COLOR             8
 * CLR_ORANGE           9
 * CLR_BRIGHT_GREEN     10
 * CLR_YELLOW           11
 * CLR_BRIGHT_BLUE      12
 * CLR_BRIGHT_MAGENTA   13
 * CLR_BRIGHT_CYAN      14
 * CLR_WHITE            15
 * CLR_MAX              16
 * BRIGHT               8
 */

#ifdef VIDEOSHADES
/* assign_videoshades() is prototyped in extern.h */
/* assign_videocolors() is prototyped in extern.h */
/* assign_video()       is prototyped in extern.h */

int shadeflag; /* shades are initialized */
int colorflag; /* colors are initialized */
const char *schoice[3] = { "dark", "normal", "light" };
const char *shade[3];
#endif /* VIDEOSHADES */

static void
init_ttycolor(void)
{
#ifdef VIDEOSHADES
    if (!shadeflag) {
        ttycolors[CLR_BLACK] = M_BLACK; /*  8 = dark gray */
        ttycolors[CLR_WHITE] = M_WHITE; /* 15 = bright white */
        ttycolors[CLR_GRAY] = M_GRAY;   /*  7 = normal white */
        shade[0] = schoice[0];
        shade[1] = schoice[1];
        shade[2] = schoice[2];
    }
#else
    ttycolors[CLR_BLACK] = M_GRAY; /*  mapped to white */
    ttycolors[CLR_WHITE] = M_GRAY; /*  mapped to white */
    ttycolors[CLR_GRAY] = M_GRAY;  /*  mapped to white */
#endif

#ifdef VIDEOSHADES
    if (!colorflag) {
#endif
        ttycolors[CLR_RED] = M_RED;
        ttycolors[CLR_GREEN] = M_GREEN;
        ttycolors[CLR_BROWN] = M_BROWN;
        ttycolors[CLR_BLUE] = M_BLUE;
        ttycolors[CLR_MAGENTA] = M_MAGENTA;
        ttycolors[CLR_CYAN] = M_CYAN;
        ttycolors[BRIGHT] = M_WHITE;
        ttycolors[CLR_ORANGE] = M_ORANGE;
        ttycolors[CLR_BRIGHT_GREEN] = M_BRIGHTGREEN;
        ttycolors[CLR_YELLOW] = M_YELLOW;
        ttycolors[CLR_BRIGHT_BLUE] = M_BRIGHTBLUE;
        ttycolors[CLR_BRIGHT_MAGENTA] = M_BRIGHTMAGENTA;
        ttycolors[CLR_BRIGHT_CYAN] = M_BRIGHTCYAN;
#ifdef VIDEOSHADES
    }
#endif
}

static int convert_uchars(char *, uchar *, int);
#ifdef VIDEOSHADES
int
assign_videoshades(char *choiceptr)
{
    char choices[120];
    char *cptr, *cvalue[3];
    int i, icolor = CLR_WHITE;

    strcpy(choices, choiceptr);
    cvalue[0] = choices;

    /* find the next ' ' or tab */
    cptr = strchr(cvalue[0], '-');
    if (!cptr)
        cptr = strchr(cvalue[0], ' ');
    if (!cptr)
        cptr = strchr(cvalue[0], '\t');
    if (!cptr)
        return 0;
    *cptr = '\0';
    /* skip  whitespace between '=' and value */
    do {
        ++cptr;
    } while (isspace(*cptr) || (*cptr == '-'));
    cvalue[1] = cptr;

    cptr = strchr(cvalue[1], '-');
    if (!cptr)
        cptr = strchr(cvalue[0], ' ');
    if (!cptr)
        cptr = strchr(cvalue[0], '\t');
    if (!cptr)
        return 0;
    *cptr = '\0';
    do {
        ++cptr;
    } while (isspace(*cptr) || (*cptr == '-'));
    cvalue[2] = cptr;

    for (i = 0; i < 3; ++i) {
        switch (i) {
        case 0:
            icolor = CLR_BLACK;
            break;
        case 1:
            icolor = CLR_GRAY;
            break;
        case 2:
            icolor = CLR_WHITE;
            break;
        }

        shadeflag = 1;
        if ((strncmpi(cvalue[i], "black", 5) == 0)
            || (strncmpi(cvalue[i], "dark", 4) == 0)) {
            shade[i] = schoice[0];
            ttycolors[icolor] = M_BLACK; /* dark gray */
        } else if ((strncmpi(cvalue[i], "gray", 4) == 0)
                   || (strncmpi(cvalue[i], "grey", 4) == 0)
                   || (strncmpi(cvalue[i], "medium", 6) == 0)
                   || (strncmpi(cvalue[i], "normal", 6) == 0)) {
            shade[i] = schoice[1];
            ttycolors[icolor] = M_GRAY; /* regular gray */
        } else if ((strncmpi(cvalue[i], "white", 5) == 0)
                   || (strncmpi(cvalue[i], "light", 5) == 0)) {
            shade[i] = schoice[2];
            ttycolors[icolor] = M_WHITE; /* bright white */
        } else {
            shadeflag = 0;
            return 0;
        }
    }
    return 1;
}

/*
 * Process defaults.nh OPTIONS=videocolors:xxx
 * Left to right assignments for:
 *      red green brown blue magenta cyan orange br.green yellow
 *      br.blue br.mag br.cyan
 *
 * Default Mapping (BIOS): 4-2-6-1-5-3-12-10-14-9-13-11
 */
int
assign_videocolors(char *colorvals)
{
    int i, icolor;
    uchar *tmpcolor;

    init_ttycolor(); /* in case defaults.nh entry wasn't complete */
    i = strlen(colorvals);
    tmpcolor = (uchar *) alloc(i);
    (void) convert_uchars(colorvals, tmpcolor, i);
    icolor = CLR_RED;
    for (i = 0; tmpcolor[i] != 0; ++i) {
        if (icolor < (CLR_WHITE)) {
            ttycolors[icolor++] = tmpcolor[i];
            if ((icolor > CLR_CYAN) && (icolor < CLR_ORANGE)) {
                icolor = CLR_ORANGE;
            }
        }
    }
    colorflag = 1;
    free((genericptr_t) tmpcolor);
    return 1;
}

static int
convert_uchars(char *bufp,  /* current pointer */
               uchar *list, /* return list */
               int size)
{
    unsigned int num = 0;
    int count = 0;

    while (1) {
        switch (*bufp) {
        case ' ':
        case '\0':
        case '\t':
        case '-':
        case '\n':
            if (num) {
                list[count++] = num;
                num = 0;
            }
            if ((count == size) || !*bufp)
                return count;
            bufp++;
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            num = num * 10 + (*bufp - '0');
            bufp++;
            break;
            return count;
        }
    }
    /*NOTREACHED*/
}

#endif /* VIDEOSHADES */
#endif /* TEXTCOLOR */

/*
 * Process defaults.nh OPTIONS=video:xxxx
 *
 *    where (current) legitimate values are:
 *
 *    autodetect (attempt to determine the adapter type)
 *    default    (force use of the default video method for environment)
 *    vga        (use vga adapter code)
 */
int
assign_video(char *sopt)
{
    /*
     * debug
     *
     *  printf("video is %s",sopt);
     *  getch();
     */
    iflags.grmode = 0;
    iflags.hasvesa = 0;
    iflags.hasvga = 0;
    iflags.usevesa = 0;
    iflags.usevga = 0;

    if (strncmpi(sopt, "def", 3) == 0) { /* default */
                                         /* do nothing - default */
#ifdef SCREEN_VGA
    } else if (strncmpi(sopt, "vga", 3) == 0) { /* vga */
        iflags.usevga = 1;
        iflags.hasvga = 1;
#endif
#ifdef SCREEN_VESA
    } else if (strncmpi(sopt, "vesa", 4) == 0) { /* vesa */
        iflags.hasvesa = 1;
        iflags.usevesa = 1;
#endif
#ifdef SCREEN_8514
    } else if (strncmpi(sopt, "8514", 4) == 0) { /* 8514/A */
        iflags.use8514 = 1;
        iflags.has8514 = 1;
#endif
    } else if (strncmpi(sopt, "auto", 4) == 0) { /* autodetect */
#ifdef SCREEN_VESA
        if (vesa_detect()) {
            iflags.hasvesa = 1;
        }
#endif
#ifdef SCREEN_8514
        if (v8514_detect()) {
            iflags.has8514 = 1;
        }
#endif
#ifdef SCREEN_VGA
        if (vga_detect()) {
            iflags.hasvga = 1;
        }
#endif
        /*
         * Auto-detect Priorities (arbitrary for now):
         *    VESA, VGA
         */
        if (iflags.hasvesa) iflags.usevesa = 1;
        else if (iflags.hasvga) {
            iflags.usevga = 1;
            /* VGA depends on BIOS to enable function keys*/
            iflags.BIOS = 1;
            iflags.rawio = 1;
        }
    } else {
        return 0;
    }
    return 1;
}

void
tileview(boolean enable)
{
#ifdef SCREEN_VGA
    if (iflags.grmode && iflags.usevga)
        vga_traditional(enable ? FALSE : TRUE);
#endif
#ifdef SCREEN_VESA
    if (iflags.grmode && iflags.usevesa)
        vesa_traditional(enable ? FALSE : TRUE);
#endif
}
#endif /* NO_TERMS  */

#else  /* STUBVIDEO */

void
tileview(boolean enable)
{
}
#endif /* STUBVIDEO */

/*video.c*/

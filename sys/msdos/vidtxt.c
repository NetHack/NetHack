/* NetHack 3.7	vidtxt.c	$NHDT-Date: 1596498278 2020/08/03 23:44:38 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.12 $ */
/*   Copyright (c) NetHack PC Development Team 1993                 */
/*   NetHack may be freely redistributed.  See license for details. */
/*                                                                  */
/*
 * vidtxt.c - Textmode video hardware support (BIOS and DJGPPFAST)
 *
 *Edit History:
 *     Initial Creation              M. Allison      93/04/04
 *     Add djgpp support             K. Smolkowski   93/04/26
 *     Add runtime monoadapter check M. Allison      93/05/09
 */

#define VIDEO_TEXT

#include "hack.h"
#include "pcvideo.h"
#include "wintty.h"

#include <dos.h>
#include <ctype.h>

#if defined(_MSC_VER)
#if _MSC_VER >= 700
#pragma warning(disable : 4018) /* signed/unsigned mismatch */
#pragma warning(disable : 4127) /* conditional expression is constant */
/* #pragma warning(disable : 4131) */ /* old style declarator */
#pragma warning(disable : 4305) /* prevents complaints with MK_FP */
#pragma warning(disable : 4309) /* initializing */
#pragma warning(disable : 4759) /* prevents complaints with MK_FP */
#endif
#endif

/* void txt_xputc(char, int);*/ /* write out character (and
                                            attribute) */

extern int attrib_text_normal;  /* text mode normal attribute */
extern int attrib_gr_normal;    /* graphics mode normal attribute */
extern int attrib_text_intense; /* text mode intense attribute */
extern int attrib_gr_intense;   /* graphics mode intense attribute */

void
txt_get_scr_size(void)
{
    union REGS regs;

    if (!iflags.BIOS) {
        CO = 80;
        LI = 24;
        return;
    }

#ifdef PC9800
    regs.h.ah = SENSEMODE;
    (void) int86(CRT_BIOS, &regs, &regs);

    CO = (regs.h.al & 0x02) ? 40 : 80;
    LI = (regs.h.al & 0x01) ? 20 : 25;
#else
    regs.x.ax = FONTINFO;
    regs.x.bx = 0;  /* current ROM BIOS font */
    regs.h.dl = 24; /* default row count */
    /* in case no EGA/MCGA/VGA */
    (void) int86(VIDEO_BIOS, &regs, &regs); /* Get Font Information */

    /* MDA/CGA/PCjr ignore INT 10h, Function 11h, but since we
     * cleverly loaded up DL with the default, everything's fine.
     *
     * Otherwise, DL now contains rows - 1.  Also, CX contains the
     * points (bytes per character) and ES:BP points to the font
     * table.  -3.
     */

    regs.h.ah = GETMODE;
    (void) int86(VIDEO_BIOS, &regs, &regs); /* Get Video Mode */

    /* This goes back all the way to the original PC.  Completely
     * safe.  AH contains # of columns, AL contains display mode,
     * and BH contains the active display page.
     */

    LI = regs.h.dl + 1;
    CO = regs.h.ah;
#endif /* PC9800 */
}

/*
 * --------------------------------------------------------------
 * The rest of this file is only compiled if NO_TERMS is defined.
 * --------------------------------------------------------------
 */

#ifdef NO_TERMS
/* #include "wintty.h" */

#ifdef SCREEN_DJGPPFAST
#include <pc.h>
#include <unistd.h>
#endif

void txt_gotoxy(int, int);

#if defined(SCREEN_BIOS) && !defined(PC9800)
void txt_get_cursor(int *, int *);
#endif

#ifdef SCREEN_DJGPPFAST
#define txt_get_cursor(x, y) ScreenGetCursor(y, x)
#endif

extern int g_attribute; /* Current attribute to use */
extern int monoflag;    /* 0 = not monochrome, else monochrome */

void
txt_backsp(void)
{
#ifdef PC9800
    union REGS regs;

    regs.h.dl = 0x01; /* one column */
    regs.h.ah = CURSOR_LEFT;
    regs.h.cl = DIRECT_CON_IO;

    int86(DOS_EXT_FUNC, &regs, &regs);

#else
    int col, row;

    txt_get_cursor(&col, &row);
    if (col > 0)
        col = col - 1;
    txt_gotoxy(col, row);
#endif
}

void
txt_nhbell(void)
{
    union REGS regs;

    if (flags.silent)
        return;
    regs.h.dl = 0x07; /* bell */
    regs.h.ah = 0x02; /* Character Output function */
    (void) int86(DOSCALL, &regs, &regs);
}

void
txt_clear_screen(void)
/* djgpp provides ScreenClear(), but in version 1.09 it is broken
 * so for now we just use the BIOS Routines
 */
{
    union REGS regs;
#ifdef PC9800
    regs.h.dl = attr98[attrib_text_normal];
    regs.h.ah = SETATT;
    regs.h.cl = DIRECT_CON_IO;

    (void) int86(DOS_EXT_FUNC, &regs, &regs);

    regs.h.dl = 0x02; /* clear whole screen */
    regs.h.ah = SCREEN_CLEAR;
    regs.h.cl = DIRECT_CON_IO;

    (void) int86(DOS_EXT_FUNC, &regs, &regs);
#else
    regs.h.dl = (char) (CO - 1); /* columns */
    regs.h.dh = (char) (LI - 1); /* rows */
    regs.x.cx = 0;               /* CL,CH = x,y of upper left */
    regs.x.ax = 0;
    regs.x.bx = 0;
    regs.h.bh = (char) attrib_text_normal;
    regs.h.ah = (char) SCROLL;
    /* DL,DH = x,y of lower rt */
    (void) int86(VIDEO_BIOS, &regs, &regs); /* Scroll or init window   */
    txt_gotoxy(0, 0);
#endif
}

/* clear to end of line */
void txt_cl_end(int col, int row)
{
    union REGS regs;
#ifndef PC9800
    int count;
#endif

#ifdef PC9800
    regs.h.dl = attr98[attrib_text_normal];
    regs.h.ah = SETATT;
    regs.h.cl = DIRECT_CON_IO;

    (void) int86(DOS_EXT_FUNC, &regs, &regs);

    regs.h.dl = 0x00; /* clear to end of line */
    regs.h.ah = LINE_CLEAR;
    regs.h.cl = DIRECT_CON_IO;

    (void) int86(DOS_EXT_FUNC, &regs, &regs);
#else
    count = CO - col;
    txt_gotoxy(col, row);
    regs.h.ah = PUTCHARATT; /* write attribute & character */
    regs.h.al = ' ';        /* character */
    regs.h.bh = 0;          /* display page */
                            /* BL = attribute */
    regs.h.bl = (char) attrib_text_normal;
    regs.x.cx = count;
    if (count != 0)
        (void) int86(VIDEO_BIOS, &regs, &regs); /* write attribute
                                                   & character */
#endif
}

void txt_cl_eos(void) /* clear to end of screen */
{
    union REGS regs;
#ifndef PC9800
    int col, row;
#endif

#ifdef PC9800
    regs.h.dl = attr98[attrib_text_normal];
    regs.h.ah = SETATT;
    regs.h.cl = DIRECT_CON_IO;

    (void) int86(DOS_EXT_FUNC, &regs, &regs);

    regs.h.dl = 0x00; /* clear to end of screen */
    regs.h.ah = SCREEN_CLEAR;
    regs.h.cl = DIRECT_CON_IO;

    (void) int86(DOS_EXT_FUNC, &regs, &regs);
#else
    txt_get_cursor(&col, &row);
    txt_cl_end(col, row); /* clear to end of line */
    if (row < LI - 1) {
        txt_gotoxy(0, (row < (LI - 1) ? row + 1 : (LI - 1)));
        regs.h.dl = (char) (CO - 1); /* X  of lower right */
        regs.h.dh = (char) (LI - 1); /* Y  of lower right */
        regs.h.cl = 0;               /* X  of upper left */
                                     /* Y (row)  of upper left */
        regs.h.ch = (char) (row < (LI - 1) ? row + 1 : (LI - 1));
        regs.x.ax = 0;
        regs.x.bx = 0;
        regs.h.bh = (char) attrib_text_normal;
        regs.h.ah = SCROLL;
        (void) int86(VIDEO_BIOS, &regs, &regs); /* Scroll or initialize window */
    }
#endif
}

void
txt_startup(int *wid, int *hgt)
{
    txt_get_scr_size();
    *wid = CO;
    *hgt = LI;

    attrib_gr_normal = attrib_text_normal;
    attrib_gr_intense = attrib_text_intense;
    g_attribute = attrib_text_normal; /* Give it a starting value */
}

/*
 * Screen output routines (these are heavily used).
 *
 * These are the 3 routines used to place information on the screen
 * in the NO_TERMS PC tty port of NetHack.  These are the routines
 * that get called by routines in other NetHack source files (such
 * as those in win/tty).
 *
 * txt_xputs - Writes a c null terminated string at the current location.
 *         Depending on compile options, this could just be a series
 *         of repeated calls to xputc() for each character.
 * txt_xputc - Writes a single character at the current location. Since
 *         various places in the code assume that control characters
 *         can be used to control, we are forced to interpret some of
 *         the more common ones, in order to keep things looking correct.
 *
 * NOTES:
 *         wintty.h uses macros to redefine common output functions
 *         such as puts, putc, putchar, so that they get steered into
 *         either xputs (for strings) or xputc (for single characters).
 *         References to puts, putc, and putchar in other source files
 *         (that include wintty.h) are actually using these routines.
 */

void
txt_xputs(const char *s, int col, int row)
{
    char c;

    if (s != (char *) 0) {
        while (*s != '\0') {
            txt_gotoxy(col, row);
            c = *s++;
            txt_xputc(c, g_attribute);
            if (col < (CO - 1))
                col++;
            txt_gotoxy(col, row);
        }
    }
}

/* write out character (and attribute) */
void txt_xputc(char ch, int attr)
{
#ifdef PC9800
    union REGS regs;

    regs.h.dl = attr98[attr];
    regs.h.ah = SETATT;
    regs.h.cl = DIRECT_CON_IO;

    (void) int86(DOS_EXT_FUNC, &regs, &regs);

    if (ch == '\n') {
        regs.h.dl = '\r';
        regs.h.ah = PUTCHAR;
        regs.h.cl = DIRECT_CON_IO;

        (void) int86(DOS_EXT_FUNC, &regs, &regs);
    }
    regs.h.dl = ch;
    regs.h.ah = PUTCHAR;
    regs.h.cl = DIRECT_CON_IO;

    (void) int86(DOS_EXT_FUNC, &regs, &regs);
#else
#ifdef SCREEN_BIOS
    union REGS regs;
#endif
    int col, row;

    txt_get_cursor(&col, &row);
    switch (ch) {
    case '\n':
#if 0
                col = 0;
                ++row;
#endif
        break;
    default:
#ifdef SCREEN_DJGPPFAST
        ScreenPutChar((int) ch, attr, col, row);
#endif
#ifdef SCREEN_BIOS
        regs.h.ah = PUTCHARATT;  /* write att & character */
        regs.h.al = ch;          /* character             */
        regs.h.bh = 0;           /* display page          */
        regs.h.bl = (char) attr; /* BL = attribute        */
        regs.x.cx = 1;           /* one character         */
        (void) int86(VIDEO_BIOS, &regs, &regs);
#endif
        if (col < (CO - 1))
            ++col;
        break;
    } /* end switch */
    txt_gotoxy(col, row);
#endif /* PC9800 */
}

/*
 * This marks the end of the general screen output routines that are
 * called from other places in NetHack.
 * ---------------------------------------------------------------------
 */

/*
 * Cursor location manipulation, and location information fetching
 * routines.
 * These include:
 *
 * txt_get_cursor(x,y)  - Returns the current location of the cursor.  In
 *                    some implementations this is implemented as a
 *                    function (BIOS), and in others it is a macro
 *                    (DJGPPFAST).
 *
 * txt_gotoxy(x,y)      - Moves the cursor on screen to the specified x and
 *                    y location.  This routine moves the location where
 *                    screen writes will occur next, it does not change
 *                    the location of the player on the NetHack level.
 */

#if defined(SCREEN_BIOS) && !defined(PC9800)
/*
 * get cursor position 
 *
 * This is implemented as a macro under DJGPPFAST.
 */
void txt_get_cursor(int *x, int *y)
{
    union REGS regs;

    regs.x.dx = 0;
    regs.h.ah = GETCURPOS; /* get cursor position */
    regs.x.cx = 0;
    regs.x.bx = 0;
    (void) int86(VIDEO_BIOS, &regs, &regs); /* Get Cursor Position */
    *x = regs.h.dl;
    *y = regs.h.dh;
}
#endif /* SCREEN_BIOS && !PC9800 */

void
txt_gotoxy(int x, int y)
{
#ifdef SCREEN_BIOS
    union REGS regs;

#ifdef PC9800
    regs.h.dh = (char) y; /* row */
    regs.h.dl = (char) x; /* column */
    regs.h.ah = SETCURPOS;
    regs.h.cl = DIRECT_CON_IO;
    (void) int86(DOS_EXT_FUNC, &regs, &regs); /* Set Cursor Position */
#else
    regs.h.ah = SETCURPOS;
    regs.h.bh = 0;                          /* display page */
    regs.h.dh = (char) y;                   /* row */
    regs.h.dl = (char) x;                   /* column */
    (void) int86(VIDEO_BIOS, &regs, &regs); /* Set Cursor Position */
#endif
#endif
#if defined(SCREEN_DJGPPFAST)
    ScreenSetCursor(y, x);
#endif
    /* The above, too, goes back all the way to the original PC.  If
     * we ever get so fancy as to swap display pages (i doubt it),
     * then we'll need to set BH appropriately.  This function
     * returns nothing.  -3.
     */
}

/*
 * This marks the end of the cursor manipulation/information routines.
 * -------------------------------------------------------------------
 */

#ifdef MONO_CHECK
int
txt_monoadapt_check(void)
{
    union REGS regs;

    regs.h.al = 0;
    regs.h.ah = GETMODE; /* get video mode */
    (void) int86(VIDEO_BIOS, &regs, &regs);
    return (regs.h.al == 7) ? 1 : 0; /* 7 means monochrome mode */
}
#endif /* MONO_CHECK */
#endif /* NO_TERMS  */

/* vidtxt.c */

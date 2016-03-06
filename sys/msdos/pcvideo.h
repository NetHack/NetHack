/* NetHack 3.6	pcvideo.h	$NHDT-Date: 1457207040 2016/03/05 19:44:00 $  $NHDT-Branch: chasonr $:$NHDT-Revision: 1.9 $ */
/*   Copyright (c) NetHack PC Development Team 1993, 1994           */
/*   NetHack may be freely redistributed.  See license for details. */
/*                                                                  */
/*
 * pcvideo.h - Hardware video support definitions and prototypes
 *
 *Edit History:
 *     Initial Creation              M. Allison      93/10/30
 *
 */

#ifndef PCVIDEO_H
#define PCVIDEO_H

#include "portio.h"

#ifdef SCREEN_BIOS
#if !defined(PC9800)
#define MONO_CHECK /* Video BIOS can do the check       */
#endif
#endif

#ifdef SCREEN_DJGPPFAST
/*# define MONO_CHECK 		/* djgpp should be able to do check  */
#endif

/*
 * PC interrupts
 */
#ifdef PC9800
#define CRT_BIOS 0x18
#define DOS_EXT_FUNC 0xdc
#define DIRECT_CON_IO 0x10
#else
#define VIDEO_BIOS 0x10
#endif
#define DOSCALL 0x21

/*
 * Video BIOS functions
 */
#if defined(PC9800)
#define SENSEMODE 0x0b /* Sense CRT Mode */

#define PUTCHAR 0x00      /* Put Character */
#define SETATT 0x02       /* Set Attribute */
#define SETCURPOS 0x03    /* Set Cursor Position */
#define CURSOR_RIGHT 0x08 /* Move Cursor Right */
#define CURSOR_LEFT 0x09  /* Move Cursor Left */
#define SCREEN_CLEAR 0x0a /* Clear Screen */
#define LINE_CLEAR 0x0b   /* Clear Line */
#else
#define SETCURPOS 0x02 /* Set Cursor Position */
#endif

#define GETCURPOS 0x03  /* Get Cursor Position */
#define GETMODE 0x0f    /* Get Video Mode */
#define SETMODE 0x00    /* Set Video Mode */
#define SETPAGE 0x05    /* Set Video Page */
#define FONTINFO 0x1130 /* Get Font Info */
#define SCROLL 0x06     /* Scroll or initialize window */
#define PUTCHARATT 0x09 /* Write attribute & char at cursor */

/*
 * VGA Specific Stuff
 */
#ifdef SCREEN_VGA
/* #define HW_PANNING		/* Hardware panning enabled */
#define USHORT unsigned short
#define MODE640x480 0x0012 /* Switch to VGA 640 x 480 Graphics mode */
#define MODETEXT 0x0003    /* Switch to Text mode 3 */

#ifdef HW_PANNING
#define PIXELINC 16 /* How much to increment by when panning */
/*#define PIXELINC 1	/* How much to increment by when panning */
#define SCREENBYTES 128
#define CharRows 30
#define VERT_RETRACE                          \
    {                                         \
        while (!(inportb(crt_status) & 0x08)) \
            ;                                 \
    }
#define VERT_RETRACE_END                     \
    {                                        \
        while ((inportb(crt_status) & 0x08)) \
            ;                                \
    }
#else
#define SCREENBYTES 80
#endif

#define CharacterWidth 8
#define SCREENHEIGHT 480
#define SCREENWIDTH (SCREENBYTES * CharacterWidth)
#define VIDEOSEG 0xa000
#define FONT_PTR_SEGMENT 0x0000
#define FONT_PTR_OFFSET 0x010C
#define SCREENPLANES 4
#define COLORDEPTH 16
#define egawriteplane(n)    \
    {                       \
        outportb(0x3c4, 2); \
        outportb(0x3c5, n); \
    }
#define egareadplane(n)     \
    {                       \
        outportb(0x3ce, 4); \
        outportb(0x3cf, n); \
    }
#define col2x8(c) ((c) *8)
#define col2x16(c) ((c) *16)
#define col2x(c) ((c) *2)
#define row2y(c) ((c) *16)
#define MAX_ROWS_PER_CELL 16
#define MAX_COLS_PER_CELL 16
#define MAX_BYTES_PER_CELL 2 /* MAX_COLS_PER_CELL/8 */
#define ROWS_PER_CELL MAX_ROWS_PER_CELL
#define COLS_PER_CELL MAX_COLS_PER_CELL
#define BYTES_PER_CELL MAX_BYTES_PER_CELL

struct cellplane {
    char image[MAX_ROWS_PER_CELL][MAX_BYTES_PER_CELL];
};

struct planar_cell_struct {
    struct cellplane plane[SCREENPLANES];
};

struct overview_cellplane {
    char image[MAX_ROWS_PER_CELL][1];
};

struct overview_planar_cell_struct {
    struct overview_cellplane plane[SCREENPLANES];
};

#endif /* SCREEN_VGA */

/*
 * Default color Indexes for hardware palettes
 *
 * Do not change the values below.
 * These are the color mappings defined by the particular video
 * hardware/mode.  You can rearrange the NetHack color mappings at
 * run-time via the defaults.nh "videocolors" and "videoshades"
 * settings.
 *
 */

#if defined(SCREEN_BIOS) || defined(SCREEN_DJGPPFAST)
#define M_BLACK 8
#define M_WHITE 15
#define M_GRAY 7 /* low-intensity white */
#define M_RED 4
#define M_GREEN 2
#define M_BROWN 6 /* low-intensity yellow */
#define M_BLUE 1
#define M_MAGENTA 5
#define M_CYAN 3
#define M_ORANGE 12
#define M_BRIGHTGREEN 10
#define M_YELLOW 14
#define M_BRIGHTBLUE 9
#define M_BRIGHTMAGENTA 13
#define M_BRIGHTCYAN 11

#define M_TEXT M_GRAY
#define BACKGROUND_COLOR 1
#define ATTRIB_NORMAL M_TEXT       /* Normal attribute */
#define ATTRIB_INTENSE M_WHITE     /* Intense White */
#define ATTRIB_MONO_NORMAL 0x01    /* Underlined,white */
#define ATTRIB_MONO_UNDERLINE 0x01 /* Underlined,white */
#define ATTRIB_MONO_BLINK 0x87     /* Flash bit, white */
#define ATTRIB_MONO_REVERSE 0x70   /* Black on white */
#endif                             /*SCREEN_BIOS || SCREEN_DJGPPFAST */

#if defined(SCREEN_VGA) || defined(SCREEN_8514)
#define BACKGROUND_VGA_COLOR 1
#define ATTRIB_VGA_NORMAL CLR_GRAY /* Normal attribute */
#define ATTRIB_VGA_INTENSE 14      /* Intense White 94/06/07 palette chg*/
#endif                             /*SCREEN_VGA || SCREEN_8514*/

#if defined(PC9800)
static unsigned char attr98[CLR_MAX] = {
    0xe1, /*  0 white            */
    0x21, /*  1 blue             */
    0x81, /*  2 green            */
    0xa1, /*  3 cyan             */
    0x41, /*  4 red              */
    0x61, /*  5 magenta          */
    0xc1, /*  6 yellow           */
    0xe1, /*  7 white            */
    0xe1, /*  8 white            */
    0x25, /*  9 reversed blue    */
    0x85, /* 10 reversed green   */
    0xa5, /* 11 reversed cyan    */
    0x45, /* 12 reversed red     */
    0x65, /* 13 reversed magenta */
    0xc5, /* 14 reversed yellow  */
    0xe5, /* 15 reversed white   */
};
#endif

#ifdef SIMULATE_CURSOR
#define CURSOR_HEIGHT 3 /* this should go - MJA */
/* cursor styles */
#define CURSOR_INVIS 0     /* cursor not visible at all            */
#define CURSOR_FRAME 1     /* block around the current tile        */
#define CURSOR_UNDERLINE 2 /* thin line at bottom of the tile      */
#define CURSOR_CORNER 3    /* cursor visible at the 4 tile corners */
#define NUM_CURSOR_TYPES 4 /* number of different cursor types     */
#define CURSOR_DEFAULT_STYLE CURSOR_CORNER
#define CURSOR_DEFAULT_COLOR M_GRAY
/* global variables for cursor */
extern int cursor_type;
extern int cursor_flag;
extern int cursor_color;
#endif

/*
 *   Function Prototypes
 *
 */

#define E extern

/* ### video.c ### */

#ifdef SIMULATE_CURSOR
E void NDECL(DrawCursor);
E void NDECL(HideCursor);
#endif

/* ### vidtxt.c ### */

#ifdef NO_TERMS
E void NDECL(txt_backsp);
E void NDECL(txt_clear_screen);
E void FDECL(txt_cl_end, (int, int));
E void NDECL(txt_cl_eos);
E void NDECL(txt_get_scr_size);
E void FDECL(txt_gotoxy, (int, int));
E int NDECL(txt_monoadapt_check);
E void NDECL(txt_nhbell);
E void FDECL(txt_startup, (int *, int *));
E void FDECL(txt_xputs, (const char *, int, int));
E void FDECL(txt_xputc, (CHAR_P, int));

/* ### vidvga.c ### */

#ifdef SCREEN_VGA
E void NDECL(vga_backsp);
E void FDECL(vga_clear_screen, (int));
E void FDECL(vga_cl_end, (int, int));
E void FDECL(vga_cl_eos, (int));
E int NDECL(vga_detect);
#ifdef SIMULATE_CURSOR
E void NDECL(vga_DrawCursor);
#endif
E void NDECL(vga_Finish);
E char __far *NDECL(vga_FontPtrs);
E void NDECL(vga_get_scr_size);
E void FDECL(vga_gotoloc, (int, int));
#ifdef POSITIONBAR
E void FDECL(vga_update_positionbar, (char *));
#endif
#ifdef SIMULATE_CURSOR
E void NDECL(vga_HideCursor);
#endif
E void NDECL(vga_Init);
E void NDECL(vga_tty_end_screen);
E void FDECL(vga_tty_startup, (int *, int *));
E void FDECL(vga_xputs, (const char *, int, int));
E void FDECL(vga_xputc, (CHAR_P, int));
E void FDECL(vga_xputg, (int, int, unsigned));
E void FDECL(vga_userpan, (BOOLEAN_P));
E void FDECL(vga_overview, (BOOLEAN_P));
E void FDECL(vga_traditional, (BOOLEAN_P));
E void NDECL(vga_refresh);
#endif /* SCREEN_VGA */
#ifdef SCREEN_VESA
E void NDECL(vesa_backsp);
E void FDECL(vesa_clear_screen, (int));
E void FDECL(vesa_cl_end, (int, int));
E void FDECL(vesa_cl_eos, (int));
E int NDECL(vesa_detect);
#ifdef SIMULATE_CURSOR
E void NDECL(vesa_DrawCursor);
#endif
E void NDECL(vesa_Finish);
E void NDECL(vesa_get_scr_size);
E void FDECL(vesa_gotoloc, (int, int));
#ifdef POSITIONBAR
E void FDECL(vesa_update_positionbar, (char *));
#endif
#ifdef SIMULATE_CURSOR
E void NDECL(vesa_HideCursor);
#endif
E void NDECL(vesa_Init);
E void NDECL(vesa_tty_end_screen);
E void FDECL(vesa_tty_startup, (int *, int *));
E void FDECL(vesa_xputs, (const char *, int, int));
E void FDECL(vesa_xputc, (CHAR_P, int));
E void FDECL(vesa_xputg, (int, int, unsigned));
E void FDECL(vesa_userpan, (BOOLEAN_P));
E void FDECL(vesa_overview, (BOOLEAN_P));
E void FDECL(vesa_traditional, (BOOLEAN_P));
E void NDECL(vesa_refresh);
#endif /* SCREEN_VESA */
#endif /* NO_TERMS   */

#undef E

#endif /* PCVIDEO_H  */
/* pcvideo.h */

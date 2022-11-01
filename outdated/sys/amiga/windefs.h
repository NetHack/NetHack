/* NetHack 3.6	windefs.h	$NHDT-Date: 1432512795 2015/05/25 00:13:15 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Gregg Wonderly, Naperville, Illinois,  1991,1992,1993. */
/* NetHack may be freely redistributed.  See license for details. */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#if !defined(_DCC) && !defined(__GNUC__)
#include <dos.h>
#endif
#include <exec/alerts.h>
#include <exec/devices.h>
#include <exec/execbase.h>
#include <devices/console.h>
#include <devices/conunit.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/gadtools.h>
#include <libraries/dosextens.h>
#include <libraries/asl.h>
/* stddef.h is included in the precompiled version of hack.h .  If we include
 * it here normally (through string.h) we'll get an "illegal typedef" later
 * on.  This is the easiest way I can think of to fix it without messing
 * around with the rest of the #includes.  --AMC
 */
#if defined(_DCC) && !defined(HACK_H)
#define ptrdiff_t ptrdiff_t_
#define size_t size_t_
#define wchar_t wchar_t_
#endif
#include <ctype.h>
#undef strcmpi
#include <string.h>
#include <errno.h>
#if defined(_DCC) && !defined(HACK_H)
#undef ptrdiff_t
#undef size_t
#undef wchar_T
#endif

#ifdef IDCMP_CLOSEWINDOW
#ifndef INTUI_NEW_LOOK
#define INTUI_NEW_LOOK
#endif
#endif

#ifndef HACK_H
#include "hack.h"
#endif
#include "wintype.h"
#include "winami.h"
#include "func_tab.h"

#ifndef CLIPPING
CLIPPING must be defined for the AMIGA version
#endif

#undef LI
#undef CO

/*#define   TOPL_GETLINE	/* Don't use a window for getlin() */
/*#define   WINDOW_YN		/* Use a window for y/n questions */

#ifdef AZTEC_C
#include <functions.h>
#else
#ifdef _DCC
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/console_protos.h>
#include <clib/layers_protos.h>
#include <clib/diskfont_protos.h>
#include <clib/gadtools_protos.h>
#else
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/console.h>
#include <proto/layers.h>
#include <proto/diskfont.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#endif

/* kludge - see amirip for why */
#undef red
#undef green
#undef blue
#ifdef _DCC
#include <clib/graphics_protos.h>
#else
#include <proto/graphics.h>
#endif

#ifdef _DCC
#define __asm /* DICE doesn't like __asm */
#endif

#ifdef _DCC
#include <clib/intuition_protos.h>
#else
#include <proto/intuition.h>
#endif
#endif

#ifdef SHAREDLIB
#include "NH:sys/amiga/lib/libmacs.h"
#endif

#ifdef INTUI_NEW_LOOK
#include <utility/tagitem.h>
#endif

#define WINVERS_AMII (strcmp("amii", windowprocs.name) == 0)
#define WINVERS_AMIV (strcmp("amitile", windowprocs.name) == 0)
#define WINVERS_AMIT (strcmp("amitty", windowprocs.name) == 0)

/* cw->data[x] contains 2 characters worth of special information.  These
 * characters are stored at the offsets as described here.
 */
#define VATTR 0    /* Video attribute is in this slot */
#define SEL_ITEM 1 /* If this is a select item, slot is 1 else 0 */
#define SOFF 2     /* The string starts here.  */

#undef NULL
#define NULL 0L

/*
 * Versions we need of various libraries.  We can't use LIBRARY_VERSION
 * as defined in <exec/types.h> because some of the libraries we need
 * don't have that version number in the 1.2 ROM.
 */

#define LIBRARY_FONT_VERSION 34L
#define LIBRARY_TILE_VERSION 37L

/* These values are just sorta suggestions in use, but are minimum
 * requirements
 * in reality...
 */
#define WINDOWHEIGHT 192
#define SCREENHEIGHT 200
#define WIDTH 640

/* This character is a solid block (cursor) in Hack.font */
#define CURSOR_CHAR 0x90

#define FONTHEIGHT 8
#define FONTWIDTH 8
#define FONTBASELINE 8

#define MAPFTWIDTH 8
#define MAPFTHEIGHT 8
#define MAPFTBASELN 6

/* If Compiling with the "New Look", redefine these now */
#ifdef INTUI_NEW_LOOK
#define NewWindow ExtNewWindow
#define NewScreen ExtNewScreen
#endif

#define SIZEOF_DISKNAME 8

#define CSI '\x9b'
#define NO_CHAR -1
#define RAWHELP 0x5F /* Rawkey code of the HELP key */

#define C_BLACK 0
#define C_WHITE 1
#define C_BROWN (WINVERS_AMIV ? 11 : 2)
#define C_CYAN (WINVERS_AMIV ? 2 : 3)
#define C_GREEN (WINVERS_AMIV ? 5 : 4)
#define C_MAGENTA (WINVERS_AMIV ? 10 : 5)
#define C_BLUE (WINVERS_AMIV ? 4 : 6)
#define C_RED 7
#define C_ORANGE 3
#define C_GREY 6
#define C_LTGREEN 8
#define C_YELLOW 9
#define C_GREYBLUE 12
#define C_LTBROWN 13
#define C_LTGREY 14
#define C_PEACH 15

/* Structure describing tile files */
struct PDAT
{
    long nplanes; /* Depth of images */
    long pbytes;  /* Bytes in a plane of data */
    long across;  /* Number of tiles across */
    long down;    /* Number of tiles down */
    long npics;   /* Number of pictures in this file */
    long xsize;   /* X-size of a tile */
    long ysize;   /* Y-size of a-tile */
};

#undef MAXCOLORS
#define MAXCOLORS 256

/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINMapWindow_h
#define MSWINMapWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"


HWND mswin_init_map_window ();
void mswin_map_stretch(HWND hWnd, LPSIZE lpsz, BOOL redraw);

#define NHMAP_VIEW_TILES			0 
#define NHMAP_VIEW_ASCII4x6			1
#define NHMAP_VIEW_ASCII6x8			2
#define NHMAP_VIEW_ASCII8x8			3
#define NHMAP_VIEW_ASCII16x8		4
#define NHMAP_VIEW_ASCII7x12		5
#define NHMAP_VIEW_ASCII8x12		6
#define NHMAP_VIEW_ASCII16x12		7
#define NHMAP_VIEW_ASCII12x16		8
#define NHMAP_VIEW_ASCII10x18		9
#define NHMAP_VIEW_FIT_TO_SCREEN	10

int mswin_map_mode(HWND hWnd, int mode);

#define ROGUE_LEVEL_MAP_MODE		NHMAP_VIEW_ASCII12x16	

#define DEF_CLIPAROUND_MARGIN  5

#endif /* MSWINMapWindow_h */

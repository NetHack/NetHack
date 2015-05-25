/* NetHack 3.6	mhmap.h	$NHDT-Date: 1432512810 2015/05/25 00:13:30 $  $NHDT-Branch: master $:$NHDT-Revision: 1.15 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINMapWindow_h
#define MSWINMapWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

COLORREF nhcolor_to_RGB(int c);
HWND mswin_init_map_window(void);
void mswin_map_stretch(HWND hWnd, LPSIZE lpsz, BOOL redraw);
int mswin_map_mode(HWND hWnd, int mode);

#define ROGUE_LEVEL_MAP_MODE MAP_MODE_ASCII12x16

#define DEF_CLIPAROUND_MARGIN 5
#define DEF_CLIPAROUND_AMOUNT 1

#endif /* MSWINMapWindow_h */

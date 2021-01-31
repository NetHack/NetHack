/* NetHack 3.7	mhmap.h	$NHDT-Date: 1596498354 2020/08/03 23:45:54 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.17 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINMapWindow_h
#define MSWINMapWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

COLORREF nhcolor_to_RGB(int c);
HWND mswin_init_map_window(void);
void mswin_map_layout(HWND hWnd, LPSIZE lpsz);
int mswin_map_mode(HWND hWnd, int mode);
void mswin_map_update(HWND hWnd);

#define ROGUE_LEVEL_MAP_MODE MAP_MODE_ASCII12x16
#define ROGUE_LEVEL_MAP_MODE_FIT_TO_SCREEN MAP_MODE_ASCII_FIT_TO_SCREEN

#define DEF_CLIPAROUND_MARGIN 5
#define DEF_CLIPAROUND_AMOUNT 1

#endif /* MSWINMapWindow_h */

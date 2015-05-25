/* NetHack 3.6	mhmap.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	mhmap.h	$Date: 2009/05/06 10:59:56 $  $Revision: 1.10 $ */
/*	SCCS Id: @(#)mhmap.h	3.5	2005/01/23	*/
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

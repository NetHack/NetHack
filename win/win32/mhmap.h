/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINMapWindow_h
#define MSWINMapWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"


HWND mswin_init_map_window (void);
void mswin_map_stretch(HWND hWnd, LPSIZE lpsz, BOOL redraw);
int mswin_map_mode(HWND hWnd, int mode);

#define ROGUE_LEVEL_MAP_MODE		MAP_MODE_ASCII12x16	

#define DEF_CLIPAROUND_MARGIN  5
#define DEF_CLIPAROUND_AMOUNT  1

#endif /* MSWINMapWindow_h */

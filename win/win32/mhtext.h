/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINTextWindow_h
#define MSWINTextWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

HWND mswin_init_text_window ();
void mswin_display_text_window (HWND hwnd);

#endif /* MSWINTextWindow_h */

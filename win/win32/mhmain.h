/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINMainWindow_h
#define MSWINMainWindow_h

/* this is a main appliation window */

#include "winMS.h"

HWND mswin_init_main_window (void);
void mswin_layout_main_window(HWND changed_child);

#endif /* MSWINMainWindow_h */

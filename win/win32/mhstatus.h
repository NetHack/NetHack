/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINStatusWindow_h
#define MSWINStatusWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

HWND mswin_init_status_window ();
void mswin_status_window_size (HWND hWnd, LPSIZE sz);

#endif /* MSWINStatusWindow_h */

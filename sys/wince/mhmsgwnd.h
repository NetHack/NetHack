/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINMessageWindow_h
#define MSWINMessageWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

HWND mswin_init_message_window ();
void mswin_message_window_size (HWND hWnd, LPSIZE sz);


#endif /* MSWINMessageWindow_h */

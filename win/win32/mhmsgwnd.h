/* $NHDT-Header: NetHack 5.0 mhmsgwnd.h 2026/06/20_22:26:43 NetHack-5.0:1.14 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINMessageWindow_h
#define MSWINMessageWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

HWND mswin_init_message_window(void);
void mswin_message_window_size(HWND hWnd, LPSIZE sz);

#endif /* MSWINMessageWindow_h */

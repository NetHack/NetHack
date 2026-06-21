/* NetHack 5.0	mhmsgwnd.h	$NHDT-Date: 1781973105 2026/06/20 16:31:45 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.12 $ */
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

/* NetHack 3.7	mhmsgwnd.h	$NHDT-Date: 1596498358 2020/08/03 23:45:58 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.10 $ */
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

/* NetHack 3.6	mhstatus.h	$NHDT-Date: 1432512812 2015/05/25 00:13:32 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINStatusWindow_h
#define MSWINStatusWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

HWND mswin_init_status_window(void);
void mswin_status_window_size(HWND hWnd, LPSIZE sz);

#endif /* MSWINStatusWindow_h */

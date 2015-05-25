/* NetHack 3.6	mhtext.h	$NHDT-Date: 1432512811 2015/05/25 00:13:31 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINTextWindow_h
#define MSWINTextWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

HWND mswin_init_text_window(void);
void mswin_display_text_window(HWND hwnd);

#endif /* MSWINTextWindow_h */

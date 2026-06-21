/* NetHack 5.0	mhtext.h	$NHDT-Date: 1781973106 2026/06/20 16:31:46 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.12 $ */
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

/* NetHack 3.7	mhtext.h	$NHDT-Date: 1596498363 2020/08/03 23:46:03 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.10 $ */
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

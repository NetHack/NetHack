/* NetHack 3.7	mhrip.h	$NHDT-Date: 1596498359 2020/08/03 23:45:59 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.11 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINRIPWindow_h
#define MSWINRIPWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

void mswin_finish_rip_text(winid wid);
HWND mswin_init_RIP_window(void);
void mswin_display_RIP_window(HWND hwnd);

#endif /* MSWINRIPWindow_h */

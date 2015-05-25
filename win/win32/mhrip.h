/* NetHack 3.6	mhrip.h	$NHDT-Date: 1432512813 2015/05/25 00:13:33 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
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

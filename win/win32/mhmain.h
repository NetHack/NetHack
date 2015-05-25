/* NetHack 3.6	mhmain.h	$NHDT-Date: 1432512811 2015/05/25 00:13:31 $  $NHDT-Branch: master $:$NHDT-Revision: 1.14 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINMainWindow_h
#define MSWINMainWindow_h

/* this is a main appliation window */

#include "winMS.h"

HWND mswin_init_main_window(void);
void mswin_layout_main_window(HWND changed_child);
void mswin_select_map_mode(int map_mode);
void mswin_menu_check_intf_mode(void);

#endif /* MSWINMainWindow_h */

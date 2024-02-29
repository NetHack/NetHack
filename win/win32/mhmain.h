/* NetHack 3.7	mhmain.h	$NHDT-Date: 1596498352 2020/08/03 23:45:52 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.15 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINMainWindow_h
#define MSWINMainWindow_h

/* this is a main application window */

#include "winMS.h"

HWND mswin_init_main_window(void);
void mswin_layout_main_window(HWND changed_child);
void mswin_select_map_mode(int map_mode);
void mswin_menu_check_intf_mode(void);

#endif /* MSWINMainWindow_h */

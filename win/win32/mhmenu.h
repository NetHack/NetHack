/* NetHack 3.7	mhmenu.h	$NHDT-Date: 1596498355 2020/08/03 23:45:55 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.10 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINMenuWindow_h
#define MSWINMenuWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

#define MENU_TYPE_TEXT 1
#define MENU_TYPE_MENU 2

extern COLORREF nhcolor_to_RGB(int c);
HWND mswin_init_menu_window(int type);
int mswin_menu_window_select_menu(HWND hwnd, int how, MENU_ITEM_P **selected,
                                  BOOL activate);
void mswin_menu_window_size(HWND hwnd, LPSIZE sz);

#endif /* MSWINTextWindow_h */

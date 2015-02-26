/* NetHack 3.5	mhmenu.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	mhmenu.h	$Date: 2009/05/06 11:00:00 $  $Revision: 1.4 $ */
/*	SCCS Id: @(#)mhmenu.h	3.5	2005/01/23	*/
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINMenuWindow_h
#define MSWINMenuWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

#define MENU_TYPE_TEXT	    1
#define MENU_TYPE_MENU	    2

HWND mswin_init_menu_window ( int type );
int mswin_menu_window_select_menu (HWND hwnd, int how, MENU_ITEM_P ** selected, BOOL activate);
void mswin_menu_window_size (HWND hwnd, LPSIZE sz);

#endif /* MSWINTextWindow_h */


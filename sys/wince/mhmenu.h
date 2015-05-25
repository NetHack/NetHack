/* NetHack may be freely redistributed.  See license for details. */
/*
 * $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$
 * $Date: 2002/07/22 03:19:13 $  $Revision: 1.1 $
 */

#ifndef MSWINMenuWindow_h
#define MSWINMenuWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

#define MENU_TYPE_TEXT 1
#define MENU_TYPE_MENU 2

HWND mswin_init_menu_window(int type);
int mswin_menu_window_select_menu(HWND hwnd, int how, MENU_ITEM_P **);
void mswin_menu_window_size(HWND hwnd, LPSIZE sz);

#endif /* MSWINTextWindow_h */

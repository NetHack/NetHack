/* NetHack 3.6  mhcmd.h       $NHDT-Date: 1524689383 2018/04/25 20:49:43 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.2 $ */
/*      Copyright (c) 2002 by Michael Allison              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINCMDWindow_h
#define MSWINCMDWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

HWND mswin_init_command_window();

/* if either sz->cx or sz->cy are already set this function will
   no modify it. It will adjust them to the minimum size
   required by the command window */
void mswin_command_window_size(HWND hwnd, LPSIZE sz);

#if defined(WIN_CE_SMARTPHONE)
/* special keypad input handling for SmartPhone */
BOOL NHSPhoneTranslateKbdMessage(WPARAM wParam, LPARAM lParam, BOOL keyDown);
void NHSPhoneSetKeypadFromString(const char *str);
void NHSPhoneSetKeypadDirection();
void NHSPhoneSetKeypadDefault();
#endif

#endif /* MSWINCMDWindow_h */

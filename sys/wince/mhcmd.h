/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINCMDWindow_h
#define MSWINCMDWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

HWND mswin_init_command_window ();
void mswin_command_window_size (HWND hwnd, LPSIZE sz);

#endif /* MSWINCMDWindow_h */

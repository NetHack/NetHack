/* NetHack 3.5	mhdlg.h	$Date$  $Revision$ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINDlgWindow_h
#define MSWINDlgWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

INT_PTR mswin_getlin_window (const char *question, char *result, size_t result_size);
INT_PTR mswin_ext_cmd_window (int* selection);
INT_PTR  mswin_player_selection_window(int* selection);

#endif /* MSWINDlgWindow_h */


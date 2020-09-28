/* NetHack 3.7	mhdlg.h	$NHDT-Date: 1596498348 2020/08/03 23:45:48 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.12 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINDlgWindow_h
#define MSWINDlgWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

int mswin_getlin_window(const char *question, char *result,
                        size_t result_size);
int mswin_ext_cmd_window(int *selection);
boolean mswin_player_selection_window();

#endif /* MSWINDlgWindow_h */

/* NetHack 3.6	mhdlg.h	$NHDT-Date: 1432512812 2015/05/25 00:13:32 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
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

/* NetHack 3.6	mhdlg.h	$NHDT-Date: 1432512802 2015/05/25 00:13:22 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
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
int mswin_player_selection_window(int *selection);

#endif /* MSWINDlgWindow_h */

/* NetHack 3.6	mhdlg.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	mhdlg.h	$Date: 2009/05/06 10:52:06 $  $Revision: 1.3 $ */
/*	SCCS Id: @(#)mhdlg.h	3.5	2005/01/23	*/
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINDlgWindow_h
#define MSWINDlgWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

int mswin_getlin_window (const char *question, char *result, size_t result_size);
int mswin_ext_cmd_window (int* selection);
int  mswin_player_selection_window(int* selection);

#endif /* MSWINDlgWindow_h */

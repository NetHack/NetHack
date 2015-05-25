/* NetHack 3.6	mhtext.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	mhtext.h	$Date: 2009/05/06 10:52:32 $  $Revision: 1.3 $ */
/*	SCCS Id: @(#)mhtext.h	3.5	2005/01/23	*/
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINTextWindow_h
#define MSWINTextWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

HWND mswin_init_text_window();
void mswin_display_text_window(HWND hwnd);

#endif /* MSWINTextWindow_h */

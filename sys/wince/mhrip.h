/* NetHack 3.6	mhrip.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	mhrip.h	$Date: 2009/05/06 10:52:28 $  $Revision: 1.4 $ */
/*	SCCS Id: @(#)mhrip.h	3.5	2005/01/23	*/
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINRIPWindow_h
#define MSWINRIPWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

HWND mswin_init_RIP_window();
void mswin_display_RIP_window(HWND hwnd);

#endif /* MSWINRIPWindow_h */

/*	SCCS Id: @(#)mhstatus.h	3.5	2005/01/23	*/
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINStatusWindow_h
#define MSWINStatusWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

HWND mswin_init_status_window (void);
void mswin_status_window_size (HWND hWnd, LPSIZE sz);

#endif /* MSWINStatusWindow_h */


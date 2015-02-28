/* NetHack 3.5	mhrip.c	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	mhrip.c	$Date: 2009/05/06 10:52:26 $  $Revision: 1.5 $ */
/*	SCCS Id: @(#)mhrip.c	3.5	2005/01/23	*/
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "mhrip.h"
#include "mhtext.h"

HWND mswin_init_RIP_window () 
{
	return mswin_init_text_window();
}

void mswin_display_RIP_window (HWND hWnd)
{
	mswin_display_text_window(hWnd);
}


/* NetHack 3.6	mhrip.c	$NHDT-Date: 1432512801 2015/05/25 00:13:21 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "mhrip.h"
#include "mhtext.h"

HWND
mswin_init_RIP_window()
{
    return mswin_init_text_window();
}

void
mswin_display_RIP_window(HWND hWnd)
{
    mswin_display_text_window(hWnd);
}

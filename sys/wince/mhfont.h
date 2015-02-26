/* NetHack 3.5	mhfont.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	mhfont.h	$Date: 2009/05/06 10:52:08 $  $Revision: 1.4 $ */
/*	SCCS Id: @(#)mhfont.h	3.5	2005/01/23	*/
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

/* font management functions */

#ifndef MSWINFont_h
#define MSWINFont_h

#include "winMS.h"

HGDIOBJ mswin_get_font(int win_type, int attr, HDC hdc, BOOL replace);
UINT mswin_charset();

#endif /* MSWINFont_h */

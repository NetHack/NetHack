/* NetHack 3.6	mhfont.h	$NHDT-Date: 1432512798 2015/05/25 00:13:18 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

/* font management functions */

#ifndef MSWINFont_h
#define MSWINFont_h

#include "winMS.h"

HGDIOBJ mswin_get_font(int win_type, int attr, HDC hdc, BOOL replace);
UINT mswin_charset();

#endif /* MSWINFont_h */

/* NetHack 3.6	mhfont.h	$NHDT-Date: 1432512810 2015/05/25 00:13:30 $  $NHDT-Branch: master $:$NHDT-Revision: 1.12 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

/* font management functions */

#ifndef MSWINFont_h
#define MSWINFont_h

#include "winMS.h"

BOOL mswin_font_supports_unicode(HFONT hFont);
HGDIOBJ mswin_get_font(int win_type, int attr, HDC hdc, BOOL replace);
void mswin_init_splashfonts(HWND hWnd);
void mswin_destroy_splashfonts(void);
UINT mswin_charset(void);

#endif /* MSWINFont_h */

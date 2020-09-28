/* NetHack 3.7	mhfont.h	$NHDT-Date: 1596498350 2020/08/03 23:45:50 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.17 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

/* font management functions */

#ifndef MSWINFont_h
#define MSWINFont_h

#include "winMS.h"

#define MIN_FONT_WIDTH 9
#define MIN_FONT_HEIGHT 12

typedef struct cached_font {
    int code;
    HFONT hFont;
    BOOL supportsUnicode;
    int width;
    int height;
} cached_font;

BOOL mswin_font_supports_unicode(HFONT hFont);
cached_font * mswin_get_font(int win_type, int attr, HDC hdc, BOOL replace);
HFONT mswin_create_splashfont(HWND hWnd);
UINT mswin_charset(void);

#endif /* MSWINFont_h */

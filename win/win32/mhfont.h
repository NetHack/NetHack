/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

/* font management functions */

#ifndef MSWINFont_h
#define MSWINFont_h

#include "winMS.h"

HGDIOBJ mswin_create_font(int win_type, int attr, HDC hdc);
void mswin_destroy_font( HGDIOBJ fnt );

#endif /* MSWINFont_h */

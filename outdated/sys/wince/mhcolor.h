/* NetHack 3.6	mhcolor.h	$NHDT-Date: 1432512799 2015/05/25 00:13:19 $  $NHDT-Branch: master $:$NHDT-Revision: 1.6 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

/* color management functions */

#ifndef MSWINColor_h
#define MSWINColor_h

#define MSWIN_COLOR_BG 0
#define MSWIN_COLOR_FG 1
#define SYSCLR_TO_BRUSH(x) ((HBRUSH)((x) + 1))

extern void mswin_init_color_table();
extern HBRUSH mswin_get_brush(int win_type, int color_index);
extern COLORREF mswin_get_color(int win_type, int color_index);

#endif /* MSWINColor_h */

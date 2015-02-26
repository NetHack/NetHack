/* NetHack 3.5	mhcolor.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	mhcolor.h	$Date: 2009/05/06 10:52:04 $  $Revision: 1.3 $ */
/*	SCCS Id: @(#)mhcolor.h	3.5	2005/01/23	*/
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

/* NetHack 3.6	mhtxtbuf.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	mhtxtbuf.h	$Date: 2009/10/13 01:55:12 $  $Revision: 1.5 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINTextBuffer_h
#define MSWINTextBuffer_h

#include "winMS.h"

typedef struct mswin_nethack_text_buffer *PNHTextBuffer;
PNHTextBuffer mswin_init_text_buffer(BOOL wrap_text);
void mswin_free_text_buffer(PNHTextBuffer pb);
void mswin_add_text(PNHTextBuffer pb, int attr, const char *text);
void mswin_set_text_wrap(PNHTextBuffer pb, BOOL wrap_text);
BOOL mswin_get_text_wrap(PNHTextBuffer pb);
void mswin_render_text(PNHTextBuffer pb, HWND edit_control);

#endif /* MSWINTextBuffer_h */

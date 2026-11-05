/* $NHDT-Header: NetHack 5.0 winos.h 2026/04/26_14:47:48 NetHack-5.0:1.7 $ */
/* Copyright (c) Michael Allison, 2021 */
/* Copyright (c) NetHack PC Development Team 2018 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINOS_H
#define WINOS_H

#include "win32api.h"

#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR '\\'
#endif

extern const WCHAR cp437[256];

WCHAR *
winos_ascii_to_wide_str(const unsigned char * src, WCHAR * dst, size_t dstLength);

void
winos_ascii_to_wide(WCHAR dst[3], UINT32 c);

BOOL winos_font_support_cp437(HFONT hFont);

#endif // WINOS_H

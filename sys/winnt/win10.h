/* NetHack 3.6	win10.h	$NHDT-Date: 1432512810 2015/05/25 00:13:30 $  $NHDT-Branch: master $:$NHDT-Revision: 1.15 $ */
/* Copyright (C) 2018 by Bart House 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WIN10_H
#define WIN10_H

#include "win32api.h"

typedef DPI_AWARENESS_CONTEXT(WINAPI *GetThreadDpiAwarenessContextProc)(VOID);
typedef BOOL(WINAPI *AreDpiAwarenessContextsEqualProc)(
    DPI_AWARENESS_CONTEXT dpiContextA, DPI_AWARENESS_CONTEXT dpiContextB);
typedef UINT(WINAPI *GetDpiForWindowProc)(HWND hwnd);

typedef struct {
    BOOL Valid;
    GetThreadDpiAwarenessContextProc GetThreadDpiAwarenessContext;
    AreDpiAwarenessContextsEqualProc AreDpiAwarenessContextsEqual;
    GetDpiForWindowProc GetDpiForWindow;
} Win10;

extern Win10 gWin10;

void win10_init();

#endif // WIN10_H
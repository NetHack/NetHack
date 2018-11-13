/* NetHack 3.6	win10.c	$NHDT-Date: 1432512810 2015/05/25 00:13:30 $  $NHDT-Branch: master $:$NHDT-Revision: 1.15 $ */
/* Copyright (C) 2018 by Bart House 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "win10.h"
#include <process.h>
#include <VersionHelpers.h>

#include "hack.h"

Win10 gWin10 = { 0 };

void win10_init()
{
    if (IsWindows10OrGreater())
    {
        HINSTANCE hUser32 = LoadLibraryA("user32.dll");

        if (hUser32 == NULL) 
            panic("Unable to load user32.dll");

        gWin10.GetThreadDpiAwarenessContext = (GetThreadDpiAwarenessContextProc) GetProcAddress(hUser32, "GetThreadDpiAwarenessContext");
        if (gWin10.GetThreadDpiAwarenessContext == NULL)
            panic("Unable to get address of GetThreadDpiAwarenessContext()");

        gWin10.AreDpiAwarenessContextsEqual = (AreDpiAwarenessContextsEqualProc) GetProcAddress(hUser32, "AreDpiAwarenessContextsEqual");
        if (gWin10.AreDpiAwarenessContextsEqual == NULL)
            panic("Unable to get address of AreDpiAwarenessContextsEqual");

        gWin10.GetDpiForWindow = (GetDpiForWindowProc) GetProcAddress(hUser32, "GetDpiForWindow");
        if (gWin10.GetDpiForWindow == NULL)
            panic("Unable to get address of GetDpiForWindow");

        FreeLibrary(hUser32);

        gWin10.Valid = TRUE;
    }

    if (gWin10.Valid) {
        if (!gWin10.AreDpiAwarenessContextsEqual(
                gWin10.GetThreadDpiAwarenessContext(),
                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
            panic("Unexpected DpiAwareness state");
    }

}

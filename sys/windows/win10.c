/* NetHack 3.7	win10.c	$NHDT-Date: 1596498318 2020/08/03 23:45:18 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.10 $ */
/* Copyright (C) 2018 by Bart House */
/* NetHack may be freely redistributed.  See license for details. */

#include "win10.h"
#include <process.h>
#include <VersionHelpers.h>

#include "hack.h"

typedef DPI_AWARENESS_CONTEXT(WINAPI *GetThreadDpiAwarenessContextProc)(VOID);
typedef DPI_AWARENESS_CONTEXT(WINAPI *SetThreadDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
typedef BOOL(WINAPI *AreDpiAwarenessContextsEqualProc)(
    DPI_AWARENESS_CONTEXT dpiContextA, DPI_AWARENESS_CONTEXT dpiContextB);
typedef UINT(WINAPI *GetDpiForWindowProc)(HWND hwnd);
typedef LONG (WINAPI *GetCurrentPackageFullNameProc)(UINT32 *packageFullNameLength,
    PWSTR  packageFullName);

typedef struct {
    BOOL Valid;
    SetThreadDpiAwarenessContextProc SetThreadDpiAwarenessContext;
    GetThreadDpiAwarenessContextProc GetThreadDpiAwarenessContext;
    AreDpiAwarenessContextsEqualProc AreDpiAwarenessContextsEqual;
    GetDpiForWindowProc GetDpiForWindow;
    GetCurrentPackageFullNameProc GetCurrentPackageFullName;
} Win10;

Win10 gWin10 = { 0 };

void win10_init(void)
{
    if (IsWindows10OrGreater())
    {
        HINSTANCE hUser32 = LoadLibraryA("user32.dll");

        if (hUser32 == NULL) 
            panic("Unable to load user32.dll");

        gWin10.SetThreadDpiAwarenessContext = (SetThreadDpiAwarenessContextProc) GetProcAddress(hUser32, "SetThreadDpiAwarenessContext");
        if (gWin10.SetThreadDpiAwarenessContext == NULL)
            panic("Unable to get address of SetThreadDpiAwarenessContext()");

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

        HINSTANCE hKernel32 = LoadLibraryA("kernel32.dll");

        if (hKernel32 == NULL) 
            panic("Unable to load kernel32.dll");

        gWin10.GetCurrentPackageFullName = (GetCurrentPackageFullNameProc) GetProcAddress(hKernel32, "GetCurrentPackageFullName");
        if (gWin10.GetCurrentPackageFullName == NULL)
            panic("Unable to get address of GetCurrentPackageFullName");

        FreeLibrary(hKernel32);

        gWin10.Valid = TRUE;
    }

    if (gWin10.Valid) {
        if (!gWin10.SetThreadDpiAwarenessContext(
                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
            panic("Unable to set DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2");

        if (!gWin10.AreDpiAwarenessContextsEqual(
                gWin10.GetThreadDpiAwarenessContext(),
                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
            panic("Unexpected DpiAwareness state");
    }

}

int win10_monitor_dpi(HWND hWnd)
{
    UINT monitorDpi = 96;

    if (gWin10.Valid) {
        monitorDpi = gWin10.GetDpiForWindow(hWnd);
        if (monitorDpi == 0)
            monitorDpi = 96;
    }

    monitorDpi = max(96, monitorDpi);

    return monitorDpi;
}

double win10_monitor_scale(HWND hWnd)
{
    return (double) win10_monitor_dpi(hWnd) / 96.0;
}

void win10_monitor_info(HWND hWnd, MonitorInfo * monitorInfo)
{
    monitorInfo->scale = win10_monitor_scale(hWnd);

    HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    BOOL success = GetMonitorInfo(monitor, &info);
    nhassert(success);
    monitorInfo->width = info.rcMonitor.right - info.rcMonitor.left;
    monitorInfo->height = info.rcMonitor.bottom - info.rcMonitor.top;
    monitorInfo->left = info.rcMonitor.left;
    monitorInfo->top = info.rcMonitor.top;
}

BOOL
win10_is_desktop_bridge_application(void)
{
    if (gWin10.Valid) {
        UINT32 length = 0;
        LONG rc = gWin10.GetCurrentPackageFullName(&length, NULL);

        return (rc == ERROR_INSUFFICIENT_BUFFER);
    }

    return FALSE;
}

/* NetHack 3.7	win10.h	$NHDT-Date: 1596498319 2020/08/03 23:45:19 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.8 $ */
/* Copyright (C) 2018 by Bart House 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WIN10_H
#define WIN10_H

#include "win32api.h"

#ifdef __MINGW32__
#ifndef DECLARE_HANDLE
#define DECLARE_HANDLE(name) struct name##__{int unused;}; typedef struct name##__ *name
#endif
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);

/* DPI awareness */
typedef enum DPI_AWARENESS
{
    DPI_AWARENESS_INVALID = -1,
    DPI_AWARENESS_UNAWARE = 0,
    DPI_AWARENESS_SYSTEM_AWARE,
    DPI_AWARENESS_PER_MONITOR_AWARE
} DPI_AWARENESS;

#define DPI_AWARENESS_CONTEXT_UNAWARE              ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((DPI_AWARENESS_CONTEXT)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#define DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED    ((DPI_AWARENESS_CONTEXT)-5)
#endif

typedef struct {
    double  scale;  // dpi of monitor / 96
    int     width;  // in pixels
    int     height; // in pixels
    int     top; // in desktop coordinate pixel space
    int     left; // in desktop coordinate pixel space
} MonitorInfo;

void win10_init();
int win10_monitor_dpi(HWND hWnd);
double win10_monitor_scale(HWND hWnd);
void win10_monitor_info(HWND hWnd, MonitorInfo * monitorInfo);
BOOL win10_is_desktop_bridge_application(void);

#endif // WIN10_H

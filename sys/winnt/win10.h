/* NetHack 3.6	win10.h	$NHDT-Date: 1432512810 2015/05/25 00:13:30 $  $NHDT-Branch: master $:$NHDT-Revision: 1.15 $ */
/* Copyright (C) 2018 by Bart House 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WIN10_H
#define WIN10_H

#include "win32api.h"

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

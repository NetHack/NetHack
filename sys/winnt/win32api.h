/* NetHack 3.6	win32api.h	$NHDT-Date: 1432516197 2015/05/25 01:09:57 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $ */
/* Copyright (c) NetHack PC Development Team 1996                 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This header file is used to clear up some discrepencies with Visual C
 * header files & NetHack before including windows.h, so all NetHack
 * files should include "win32api.h" rather than <windows.h>.
 */
#if defined(_MSC_VER)
#undef strcmpi
#undef min
#undef max
#pragma warning(disable : 4142) /* Warning, Benign redefinition of type */
#pragma pack(8)
#endif

#define WIN32_LEAN_AND_MEAN

#undef Protection /* We have a global name space collision.  No source file
                     using win32api.h should be using the Protection macro
                     from youprop.h.
                     A better fix would be to ensure we include all window
                     header files before we start clobbering the global name
                     space with NetHack specific macros. */

#include <windows.h>
#include <commctrl.h>

#if defined(_MSC_VER)
#pragma pack()
#endif

/*win32api.h*/

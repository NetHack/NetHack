/* NetHack 3.6	win32api.h	$NHDT-Date: 1432516197 2015/05/25 01:09:57 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $ */
/* Copyright (c) NetHack PC Development Team 1996                 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This header file is used to clear up some discrepencies with Visual C
 * header files & NetHack before including windows.h, so all NetHack
 * files should include "win32api.h" rather than <windows.h>.
 */
#ifndef WIN32API
#define WIN32API

#if defined(_MSC_VER)

#if defined(HACK_H)
#error win32api.h must be included before hack.h
#endif

#if defined(strcmpi)
#error win32api.h should be included first
#endif

#if defined(min)
#error win32api.h should be included first
#endif

#if defined(max)
#error win32api.h should be included first
#endif

#if defined(Protection)
#error win32api.h should be included first
#endif

#pragma warning(disable : 4142) /* Warning, Benign redefinition of type */
#pragma pack(8)
#endif // _MSC_VER

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>

#if defined(_MSC_VER)
#pragma pack()
#endif

#endif // WIN32API

/*win32api.h*/

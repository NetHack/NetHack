/*	SCCS Id: @(#)win32api.h	 3.4	 1996/02/15		  */
/* Copyright (c) NetHack PC Development Team 1996                 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This header file is used to clear up some discrepencies with Visual C
 * header files & NetHack before including windows.h, so all NetHack
 * files should include "win32api.h" rather than <windows.h>.
 */
# if defined(_MSC_VER)
# undef strcmpi
# undef min
# undef max
# pragma warning(disable:4142)  /* Warning, Benign redefinition of type */
# pragma pack(8)
# endif

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>

# if defined(_MSC_VER)
# pragma pack()
# endif

/*win32api.h*/

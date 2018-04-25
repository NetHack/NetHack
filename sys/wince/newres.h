/* NetHack 3.6  newres.h       $NHDT-Date: 1524689383 2018/04/25 20:49:43 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.3 $ */
/*      Copyright (c) 2003 by Michael Allison              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef __NEWRES_H__
#define __NEWRES_H__

#if !defined(UNDER_CE)
#define UNDER_CE _WIN32_WCE
#endif

#if defined(_WIN32_WCE)
#if !defined(WCEOLE_ENABLE_DIALOGEX)
#define DIALOGEX DIALOG DISCARDABLE
#endif
#include <commctrl.h>
#define SHMENUBAR RCDATA
#if (defined(WIN32_PLATFORM_PSPC) || defined(WIN32_PLATFORM_WFSP)) \
    && (_WIN32_WCE >= 300)
#include <aygshell.h>
#else
#define I_IMAGENONE (-2)
#define NOMENU 0xFFFF
#define IDS_SHNEW 1

#define IDM_SHAREDNEW 10
#define IDM_SHAREDNEWDEFAULT 11
#endif
#endif // _WIN32_WCE

#ifdef RC_INVOKED
#ifndef _INC_WINDOWS
#define _INC_WINDOWS
#include "winuser.h" // extract from windows header
#endif
#endif

#ifdef IDC_STATIC
#undef IDC_STATIC
#endif
#define IDC_STATIC (-1)

#endif //__NEWRES_H__

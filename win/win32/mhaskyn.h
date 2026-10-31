/* $NHDT-Header: NetHack 5.0 mhaskyn.h 2026/06/20_22:26:43 NetHack-5.0:1.13 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINAskYesNO_h
#define MSWINAskYesNO_h

#include "winMS.h"

int mswin_yes_no_dialog(const char *question, const char *choices, int def);

#endif /* MSWINAskYesNO_h */

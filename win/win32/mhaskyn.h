/* NetHack 5.0	mhaskyn.h	$NHDT-Date: 1781973101 2026/06/20 16:31:41 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.11 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINAskYesNO_h
#define MSWINAskYesNO_h

#include "winMS.h"

int mswin_yes_no_dialog(const char *question, const char *choices, int def);

#endif /* MSWINAskYesNO_h */

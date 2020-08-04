/* NetHack 3.7	mhaskyn.h	$NHDT-Date: 1596498347 2020/08/03 23:45:47 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.9 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINAskYesNO_h
#define MSWINAskYesNO_h

#include "winMS.h"

int mswin_yes_no_dialog(const char *question, const char *choices, int def);

#endif /* MSWINAskYesNO_h */

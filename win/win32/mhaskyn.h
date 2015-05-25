/* NetHack 3.6	mhaskyn.h	$NHDT-Date: 1432512811 2015/05/25 00:13:31 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINAskYesNO_h
#define MSWINAskYesNO_h

#include "winMS.h"

int mswin_yes_no_dialog(const char *question, const char *choices, int def);

#endif /* MSWINAskYesNO_h */

/* NetHack 3.6	mhaskyn.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	mhaskyn.h	$Date: 2009/05/06 10:52:03 $  $Revision: 1.3 $ */
/*	SCCS Id: @(#)mhaskyn.h	3.5	2005/01/23	*/
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINAskYesNO_h
#define MSWINAskYesNO_h

#include "winMS.h"

int mswin_yes_no_dialog( const char *question, const char *choices, int def);

#endif /* MSWINAskYesNO_h */

/* NetHack 3.6	gnyesno.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	gnyesno.h	$Date: 2009/05/06 10:58:08 $  $Revision: 1.4 $ */
/*	SCCS Id: @(#)gnyesno.h	3.5	2000/07/16	*/
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackYesNoDialog_h
#define GnomeHackYesNoDialog_h

int ghack_yes_no_dialog(const char *szQuestionStr, const char *szChoicesStr,
                        int nDefault);

#endif

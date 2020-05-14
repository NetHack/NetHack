/* NetHack 3.6	gnyesno.h	$NHDT-Date: 1432512807 2015/05/25 00:13:27 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackYesNoDialog_h
#define GnomeHackYesNoDialog_h

int ghack_yes_no_dialog(const char *szQuestionStr, const char *szChoicesStr,
                        int nDefault);

#endif

/* NetHack 3.6	gnaskstr.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	gnaskstr.h	$Date: 2009/05/06 10:57:24 $  $Revision: 1.4 $ */
/*	SCCS Id: @(#)gnaskstr.h	3.5	2000/07/16	*/
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackAskStringDialog_h
#define GnomeHackAskStringDialog_h


int ghack_ask_string_dialog(const char *szMessageStr,
        const char *szDefaultStr, const char *szTitleStr,
        char *buffer);

#endif /* GnomeHackAskStringDialog_h */


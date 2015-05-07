/* NetHack 3.6	winGnome.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	winGnome.h	$Date: 2009/05/06 10:45:16 $  $Revision: 1.4 $ */
/*	SCCS Id: @(#)winGnome.h	3.4.	2000/07/16	*/
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* Copyright (C) 1998 by Anthony Taylor <tonyt@ptialaska.net> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINGNOME_H
#define WINGNOME_H

#define E extern

E struct window_procs Gnome_procs;

#undef E

#define NHW_WORN	6
extern winid WIN_WORN;

#endif /* WINGNOME_H */

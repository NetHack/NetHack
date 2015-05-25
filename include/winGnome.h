/* NetHack 3.6	winGnome.h	$NHDT-Date: 1432512782 2015/05/25 00:13:02 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* Copyright (C) 1998 by Anthony Taylor <tonyt@ptialaska.net> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINGNOME_H
#define WINGNOME_H

#define E extern

E struct window_procs Gnome_procs;

#undef E

#define NHW_WORN 6
extern winid WIN_WORN;

#endif /* WINGNOME_H */

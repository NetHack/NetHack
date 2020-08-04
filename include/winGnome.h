/* NetHack 3.7	winGnome.h	$NHDT-Date: 1596498571 2020/08/03 23:49:31 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.9 $ */
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

/*	SCCS Id: @(#)gnmap.h	3.4	2000/07/16	*/
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackMapWindow_h
#define GnomeHackMapWindow_h

#include <gnome.h>
#include <gdk_imlib.h>
#include "config.h"
#include "global.h"

GtkWidget *ghack_init_map_window(void);
void ghack_reinit_map_window(void);

#endif /* GnomeHackMapWindow_h */

/* NetHack 3.6	gnmap.h	$NHDT-Date: 1432512807 2015/05/25 00:13:27 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
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

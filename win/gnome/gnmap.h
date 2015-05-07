/* NetHack 3.6	gnmap.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	gnmap.h	$Date: 2009/05/06 10:57:35 $  $Revision: 1.4 $ */
/*	SCCS Id: @(#)gnmap.h	3.5	2000/07/16	*/
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

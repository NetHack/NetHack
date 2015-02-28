/* NetHack 3.5	gntext.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	gntext.h	$Date: 2009/05/06 10:58:03 $  $Revision: 1.4 $ */
/*	SCCS Id: @(#)gntext.h	3.5	2000/07/16	*/
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackTextWindow_h
#define GnomeHackTextWindow_h

#include <gnome.h>
#include "config.h"
#include "global.h"

GtkWidget* ghack_init_text_window ( );
void ghack_text_window_clear(GtkWidget *widget, gpointer data);
void ghack_text_window_destroy();
void ghack_text_window_display(GtkWidget *widget, boolean block,
                              gpointer data);
void ghack_text_window_put_string(GtkWidget *widget, int attr,
                                  const char* text, gpointer data);
void ghack_text_window_rip_string( const char* ripString);


#endif /* GnomeHackTextWindow_h */

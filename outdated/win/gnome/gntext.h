/* NetHack 3.6	gntext.h	$NHDT-Date: 1432512805 2015/05/25 00:13:25 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackTextWindow_h
#define GnomeHackTextWindow_h

#include <gnome.h>
#include "config.h"
#include "global.h"

GtkWidget *ghack_init_text_window();
void ghack_text_window_clear(GtkWidget *widget, gpointer data);
void ghack_text_window_destroy();
void ghack_text_window_display(GtkWidget *widget, boolean block,
                               gpointer data);
void ghack_text_window_put_string(GtkWidget *widget, int attr,
                                  const char *text, gpointer data);
void ghack_text_window_rip_string(const char *ripString);

#endif /* GnomeHackTextWindow_h */

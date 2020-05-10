/* NetHack 3.6	gnmain.h	$NHDT-Date: 1432512805 2015/05/25 00:13:25 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackMainWindow_h
#define GnomeHackMainWindow_h

#include <gnome.h>
#include <gtk/gtk.h>

void ghack_init_main_window(int argc, char **argv);
void ghack_main_window_add_map_window(GtkWidget *win);
void ghack_main_window_add_message_window(GtkWidget *win);
void ghack_main_window_add_status_window(GtkWidget *win);
void ghack_main_window_add_text_window(GtkWidget *);
void ghack_main_window_add_worn_window(GtkWidget *win);
void ghack_main_window_remove_window(GtkWidget *);
void ghack_main_window_update_inventory();
void ghack_save_game_cb(GtkWidget *widget, gpointer data);
GtkWidget *ghack_get_main_window();

#endif /* GnomeHackMainWindow_h */

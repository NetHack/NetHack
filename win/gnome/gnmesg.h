/*	SCCS Id: @(#)gnmesg.h	3.4	2000/07/16	*/
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackMessageWindow_h
#define GnomeHackMessageWindow_h

#include <gnome.h>
#include "config.h"

GtkWidget* ghack_init_message_window ( /* GnomeHackKeyBuffer g_keybuffer,
					  GnomeHackClickBuffer g_clickbuffer */ );
void ghack_message_window_clear(GtkWidget *widget, gpointer data);
void ghack_message_window_destroy();
void ghack_message_window_display(GtkWidget *widget, boolean block,
                                  gpointer data);
void ghack_message_window_put_string(GtkWidget *widget, int attr,
                                     const char* text, gpointer data);
void ghack_message_window_use_RIP(int how);
void ghack_message_window_scroll(int dx, int dy);


#endif /* GnomeHackMessageWindow_h */

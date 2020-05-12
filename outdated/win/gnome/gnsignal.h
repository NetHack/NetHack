/* NetHack 3.6	gnsignal.h	$NHDT-Date: 1432512807 2015/05/25 00:13:27 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (C) 1998 by Anthony Taylor <tonyt@ptialaska.net> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackSignals_h
#define GnomeHackSignals_h

#include <gtk/gtk.h>
#include <gnome.h>
#include "gnomeprv.h"
#include "gnglyph.h"

/* The list of custom signals */

enum {
    GHSIG_CURS,
    GHSIG_PUTSTR,
    GHSIG_PRINT_GLYPH,
    GHSIG_CLEAR,
    GHSIG_DISPLAY,
    GHSIG_START_MENU,
    GHSIG_ADD_MENU,
    GHSIG_END_MENU,
    GHSIG_SELECT_MENU,
    GHSIG_CLIPAROUND,
    GHSIG_FADE_HIGHLIGHT,
    GHSIG_DELAY,
    GHSIG_LAST_SIG
};

guint ghack_signals[GHSIG_LAST_SIG];

extern void ghack_init_signals(void);

void ghack_handle_key_press(GtkWidget *widget, GdkEventKey *event,
                            gpointer data);
void ghack_handle_button_press(GtkWidget *widget, GdkEventButton *event,
                               gpointer data);

typedef struct {
    int x, y, mod;
} GHClick;

extern GList *g_keyBuffer;
extern GList *g_clickBuffer;
extern int g_numKeys;
extern int g_numClicks;

extern int g_askingQuestion;

void ghack_delay(GtkWidget *win, int numMillisecs, gpointer data);

#endif /* GnomeHackSignals_h */

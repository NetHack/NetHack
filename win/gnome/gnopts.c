/* NetHack 3.6	gnopts.c	$NHDT-Date: 1432512805 2015/05/25 00:13:25 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#include "gnopts.h"
#include "gnglyph.h"
#include "gnmain.h"
#include "gnmap.h"
#include <gnome.h>
#include <ctype.h>
#include "hack.h"

static gint tileset;
static GtkWidget *clist;
const char *tilesets[] = { "Traditional (16x16)", "Big (32x32)", 0 };

static void
opt_sel_key_hit(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    int i;
    for (i = 0; tilesets[i] != 0; ++i) {
        if (tilesets[i][0] == toupper(event->keyval)) {
            tileset = i;
            gtk_clist_select_row(GTK_CLIST(clist), i, 0);
        }
    }
}

static void
opt_sel_row_selected(GtkCList *cList, int row, int col, GdkEvent *event)
{
    tileset = row;
}

void
ghack_settings_dialog()
{
    int i;
    static GtkWidget *dialog;
    static GtkWidget *swin;
    static GtkWidget *frame1;

    dialog = gnome_dialog_new(_("GnomeHack Settings"), GNOME_STOCK_BUTTON_OK,
                              GNOME_STOCK_BUTTON_CANCEL, NULL);
    gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
    gtk_signal_connect(GTK_OBJECT(dialog), "key_press_event",
                       GTK_SIGNAL_FUNC(opt_sel_key_hit), tilesets);

    frame1 = gtk_frame_new(_("Choose one of the following tilesets:"));
    gtk_object_set_data(GTK_OBJECT(dialog), "frame1", frame1);
    gtk_widget_show(frame1);
    gtk_container_border_width(GTK_CONTAINER(frame1), 3);

    swin = gtk_scrolled_window_new(NULL, NULL);
    clist = gtk_clist_new(2);
    gtk_clist_column_titles_hide(GTK_CLIST(clist));
    gtk_widget_set_usize(GTK_WIDGET(clist), 100, 180);
    gtk_container_add(GTK_CONTAINER(swin), clist);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    gtk_signal_connect(GTK_OBJECT(clist), "select_row",
                       GTK_SIGNAL_FUNC(opt_sel_row_selected), NULL);

    gtk_container_add(GTK_CONTAINER(frame1), swin);
    gtk_box_pack_start_defaults(GTK_BOX(GNOME_DIALOG(dialog)->vbox), frame1);

    /* Add the tilesets into the list here... */
    for (i = 0; tilesets[i]; i++) {
        gchar accelBuf[BUFSZ];
        const char *text[3] = { accelBuf, tilesets[i], NULL };
        sprintf(accelBuf, "%c ", tolower(tilesets[i][0]));
        gtk_clist_insert(GTK_CLIST(clist), i, (char **) text);
    }

    gtk_clist_columns_autosize(GTK_CLIST(clist));
    gtk_widget_show_all(swin);

    /* Center the dialog over over parent */
    gnome_dialog_set_default(GNOME_DIALOG(dialog), 0);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gnome_dialog_set_parent(GNOME_DIALOG(dialog),
                            GTK_WINDOW(ghack_get_main_window()));

    /* Run the dialog -- returning whichever button was pressed */
    i = gnome_dialog_run(GNOME_DIALOG(dialog));
    gnome_dialog_close(GNOME_DIALOG(dialog));

    /* They hit Quit or error */
    if (i != 0) {
        return;
    }
    switch (tileset) {
    case 0:
        /* They selected traditional */
        ghack_free_glyphs();
        if (ghack_init_glyphs(HACKDIR "/x11tiles"))
            g_error("ERROR:  Could not initialize glyphs.\n");
        ghack_reinit_map_window();
        break;
    case 1:
        ghack_free_glyphs();
        if (ghack_init_glyphs(HACKDIR "/t32-1024.xpm"))
            g_error("ERROR:  Could not initialize glyphs.\n");
        ghack_reinit_map_window();

        /* They selected big */
        break;
    default:
        /* This shouldn't happen */
        g_warning("This shouldn't happen\n");
    }
}

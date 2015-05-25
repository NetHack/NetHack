/* NetHack 3.6	gntext.c	$NHDT-Date: 1432512804 2015/05/25 00:13:24 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#include "gntext.h"
#include "gnmain.h"
#include <gnome.h>

/* include the standard RIP window (win/X11/rip.xpm) */
#include "gn_rip.h"

/* dimensions of the pixmap */
#define RIP_IMAGE_WIDTH 400
#define RIP_IMAGE_HEIGHT 200

/* dimensions and location of area where we can draw text on the pixmap */
#define RIP_DRAW_WIDTH 84
#define RIP_DRAW_HEIGHT 89
#define RIP_DRAW_X 114
#define RIP_DRAW_Y 69

/* Text Window widgets */
GtkWidget *RIP = NULL;
GtkWidget *RIPlabel = NULL;
GtkWidget *TW_window = NULL;
GnomeLess *gless;

static int showRIP = 0;

void
ghack_text_window_clear(GtkWidget *widget, gpointer data)
{
    g_assert(gless != NULL);
    gtk_editable_delete_text(GTK_EDITABLE(gless->text), 0, 0);
}

void
ghack_text_window_destroy()
{
    TW_window = NULL;
}

void
ghack_text_window_display(GtkWidget *widget, boolean block, gpointer data)
{
    if (showRIP == 1) {
        gtk_widget_show(GTK_WIDGET(RIP));
        gtk_window_set_title(GTK_WINDOW(TW_window), "Rest In Peace");
    }

    gtk_signal_connect(GTK_OBJECT(TW_window), "destroy",
                       (GtkSignalFunc) ghack_text_window_destroy, NULL);
    if (block)
        gnome_dialog_run(GNOME_DIALOG(TW_window));
    else
        gnome_dialog_run_and_close(GNOME_DIALOG(TW_window));

    if (showRIP == 1) {
        showRIP = 0;
        gtk_widget_hide(GTK_WIDGET(RIP));
        gtk_window_set_title(GTK_WINDOW(TW_window), "Text Window");
    }
}

void
ghack_text_window_put_string(GtkWidget *widget, int attr, const char *text,
                             gpointer data)
{
    if (text == NULL)
        return;

    /* Don't bother with attributes yet */
    gtk_text_insert(GTK_TEXT(gless->text), NULL, NULL, NULL, text, -1);
    gtk_text_insert(GTK_TEXT(gless->text), NULL, NULL, NULL, "\n", -1);
}

GtkWidget *
ghack_init_text_window()
{
    GtkWidget *pixmap;
    if (TW_window)
        return (GTK_WIDGET(TW_window));

    TW_window = gnome_dialog_new("Text Window", GNOME_STOCK_BUTTON_OK, NULL);
    gtk_window_set_default_size(GTK_WINDOW(TW_window), 500, 400);
    gtk_window_set_policy(GTK_WINDOW(TW_window), TRUE, TRUE, FALSE);
    gtk_window_set_title(GTK_WINDOW(TW_window), "Text Window");

    /* create GNOME pixmap object */
    pixmap = gnome_pixmap_new_from_xpm_d(rip_xpm);
    g_assert(pixmap != NULL);
    gtk_widget_show(GTK_WIDGET(pixmap));

    /* create label with our "death message", sized to fit into the
    * tombstone */
    RIPlabel = gtk_label_new("RIP");
    g_assert(RIPlabel != NULL);
    /* gtk_label_set_justify is broken? */
    gtk_label_set_justify(GTK_LABEL(RIPlabel), GTK_JUSTIFY_CENTER);
    gtk_label_set_line_wrap(GTK_LABEL(RIPlabel), TRUE);
    gtk_widget_set_usize(RIPlabel, RIP_DRAW_WIDTH, RIP_DRAW_HEIGHT);
    gtk_widget_show(RIPlabel);

    /* create a fixed sized widget for the RIP pixmap */
    RIP = gtk_fixed_new();
    g_assert(RIP != NULL);
    gtk_widget_set_usize(RIP, RIP_IMAGE_WIDTH, RIP_IMAGE_HEIGHT);
    gtk_fixed_put(GTK_FIXED(RIP), pixmap, 0, 0);
    gtk_fixed_put(GTK_FIXED(RIP), RIPlabel, RIP_DRAW_X, RIP_DRAW_Y);
    gtk_widget_show(RIP);
    gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(TW_window)->vbox), RIP, TRUE,
                       TRUE, 0);

    /* create a gnome Less widget for the text stuff */
    gless = GNOME_LESS(gnome_less_new());
    g_assert(gless != NULL);
    gtk_widget_show(GTK_WIDGET(gless));
    gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(TW_window)->vbox),
                       GTK_WIDGET(gless), TRUE, TRUE, 0);

    /* Hook up some signals */
    gtk_signal_connect(GTK_OBJECT(TW_window), "ghack_putstr",
                       GTK_SIGNAL_FUNC(ghack_text_window_put_string), NULL);

    gtk_signal_connect(GTK_OBJECT(TW_window), "ghack_clear",
                       GTK_SIGNAL_FUNC(ghack_text_window_clear), NULL);

    gtk_signal_connect(GTK_OBJECT(TW_window), "ghack_display",
                       GTK_SIGNAL_FUNC(ghack_text_window_display), NULL);

    /* Center the dialog over over parent */
    gnome_dialog_set_parent(GNOME_DIALOG(TW_window),
                            GTK_WINDOW(ghack_get_main_window()));

    gtk_window_set_modal(GTK_WINDOW(TW_window), TRUE);
    gtk_widget_show_all(TW_window);
    gtk_widget_hide(GTK_WIDGET(RIP));
    gnome_dialog_close_hides(GNOME_DIALOG(TW_window), TRUE);

    return GTK_WIDGET(TW_window);
}

void
ghack_text_window_rip_string(const char *string)
{
    /* This is called to specify that the next message window will
     * be a RIP window, which will include this text */

    showRIP = 1;
    gtk_label_set(GTK_LABEL(RIPlabel), string);
}

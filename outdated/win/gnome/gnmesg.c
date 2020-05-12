/* NetHack 3.6	gnmesg.c	$NHDT-Date: 1432512805 2015/05/25 00:13:25 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#include "gnmesg.h"
#include "gnsignal.h"

/* Pick an arbitrary number of chars such as 80 col X 40 rows text = 3200
 * chars */
#define nCharsBeforeDeletingStuff 3200

/* Message Window widgets */
GtkWidget *MW_table;
GtkWidget *MW_text;
GtkWidget *MW_scrollbar;

void
ghack_message_window_clear(GtkWidget *widget, gpointer data)
{
    /* Seems nethack calls this after every move -- we don't want
     * to really clear the window at all though.  Ignore the request */
    gint len;

    len = gtk_text_get_length(GTK_TEXT(MW_text));

    if (len < nCharsBeforeDeletingStuff)
        return;

    gtk_text_freeze(GTK_TEXT(MW_text));
    gtk_text_set_point(GTK_TEXT(MW_text), 0);
    gtk_text_forward_delete(GTK_TEXT(MW_text),
                            len - ((guint)(nCharsBeforeDeletingStuff * 0.5)));
    gtk_text_set_point(GTK_TEXT(MW_text),
                       (guint)(nCharsBeforeDeletingStuff * 0.5));
    gtk_text_thaw(GTK_TEXT(MW_text));
}

void
ghack_message_window_destroy(GtkWidget *win, gpointer data)
{
}

void
ghack_message_window_display(GtkWidget *widget, boolean block, gpointer data)
{
}

void
ghack_message_window_put_string(GtkWidget *widget, int attr, const char *text,
                                gpointer data)
{
    if (text == NULL)
        return;

    /* Don't bother with attributes yet */
    gtk_text_insert(GTK_TEXT(MW_text), NULL, NULL, NULL, text, -1);
    gtk_text_insert(GTK_TEXT(MW_text), NULL, NULL, NULL, "\n", -1);
}

void
ghack_message_window_use_RIP(int how)
{
}

void
ghack_message_window_scroll(int dx, int dy)
{
}

GtkWidget *
ghack_init_message_window(void)
{
    MW_table = gtk_table_new(2, 1, FALSE);
    gtk_table_set_row_spacing(GTK_TABLE(MW_table), 0, 2);

    MW_text = gtk_text_new(NULL, NULL);
    gtk_text_set_editable(GTK_TEXT(MW_text), FALSE);
    gtk_text_set_word_wrap(GTK_TEXT(MW_text), TRUE);
    gtk_table_attach(GTK_TABLE(MW_table), MW_text, 0, 1, 0, 1,
                     (GTK_EXPAND | GTK_FILL), (GTK_EXPAND | GTK_FILL), 0, 0);

    MW_scrollbar = gtk_vscrollbar_new(GTK_TEXT(MW_text)->vadj);
    gtk_table_attach(GTK_TABLE(MW_table), MW_scrollbar, 1, 2, 0, 1, GTK_FILL,
                     (GTK_EXPAND | GTK_FILL), 0, 0);

    gtk_signal_connect(GTK_OBJECT(MW_table), "ghack_putstr",
                       GTK_SIGNAL_FUNC(ghack_message_window_put_string),
                       NULL);

    gtk_signal_connect(GTK_OBJECT(MW_table), "ghack_clear",
                       GTK_SIGNAL_FUNC(ghack_message_window_clear), NULL);

    gtk_signal_connect(GTK_OBJECT(MW_table), "gnome_delay_output",
                       GTK_SIGNAL_FUNC(ghack_delay), NULL);

    gtk_widget_show_all(MW_table);

    return GTK_WIDGET(MW_table);
}

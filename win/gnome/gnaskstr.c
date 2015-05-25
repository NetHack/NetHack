/* NetHack 3.6	gnaskstr.c	$NHDT-Date: 1432512806 2015/05/25 00:13:26 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#include "gnaskstr.h"
#include "gnmain.h"
#include <gnome.h>

static void
ghack_ask_string_callback(gchar *string, gpointer data)
{
    char **user_text = (char **) data;

    g_assert(user_text != NULL);

    *user_text = string; /* note - value must be g_free'd */
}

int
ghack_ask_string_dialog(const char *szMessageStr, const char *szDefaultStr,
                        const char *szTitleStr, char *buffer)
{
    int i;
    GtkWidget *dialog;
    gchar *user_text = NULL;

    dialog =
        gnome_request_dialog(FALSE, szMessageStr, szDefaultStr, 0,
                             ghack_ask_string_callback, &user_text, NULL);
    g_assert(dialog != NULL);

    gtk_window_set_title(GTK_WINDOW(dialog), szTitleStr);

    gnome_dialog_set_default(GNOME_DIALOG(dialog), 0);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gnome_dialog_set_parent(GNOME_DIALOG(dialog),
                            GTK_WINDOW(ghack_get_main_window()));

    i = gnome_dialog_run_and_close(GNOME_DIALOG(dialog));

    /* Quit */
    if (i != 0 || user_text == NULL) {
        if (user_text)
            g_free(user_text);
        return -1;
    }

    if (*user_text == 0) {
        g_free(user_text);
        return -1;
    }

    g_assert(strlen(user_text) > 0);
    strcpy(buffer, user_text);
    g_free(user_text);
    return 0;
}

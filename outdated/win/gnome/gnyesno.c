/* NetHack 3.6	gnyesno.c	$NHDT-Date: 1432512807 2015/05/25 00:13:27 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#include "gnbind.h"
#include "gnyesno.h"

int
ghack_yes_no_dialog(const char *question, const char *choices, int def)
{
    int i = 0, ret;
    gchar button_name[BUFSZ];
    GtkWidget *box;
    GtkWidget *mainWnd = NULL;

    box = gnome_message_box_new(question, GNOME_MESSAGE_BOX_QUESTION, NULL);
    /* add buttons for each choice */
    if (!strcmp(GNOME_STOCK_BUTTON_OK, choices)) {
        gnome_dialog_append_button(GNOME_DIALOG(box), GNOME_STOCK_BUTTON_OK);
        gnome_dialog_set_default(GNOME_DIALOG(box), 0);
        gnome_dialog_set_accelerator(GNOME_DIALOG(box), 0, 'o', 0);
#if 0
	g_print("Setting accelerator '%c' for button %d\n", 'o', 0);
#endif
    } else {
        for (; choices[i] != '\0'; i++) {
            if (choices[i] == 'y') {
                sprintf(button_name, GNOME_STOCK_BUTTON_YES);
            } else if (choices[i] == 'n') {
                sprintf(button_name, GNOME_STOCK_BUTTON_NO);
            } else if (choices[i] == 'q') {
                sprintf(button_name, "Quit");
            } else {
                sprintf(button_name, "%c", choices[i]);
            }
            if (def == choices[i])
                gnome_dialog_set_default(GNOME_DIALOG(box), i);
            gnome_dialog_append_button(GNOME_DIALOG(box), button_name);
            gnome_dialog_set_accelerator(GNOME_DIALOG(box), i, choices[i], 0);
#if 0
	    g_print("Setting accelerator '%c' for button %d\n", choices[i], i);
#endif
        }
    }
#if 0
    /* Perhaps add in a quit game button, like this... */
    gnome_dialog_append_button ( GNOME_DIALOG(box), GNOME_STOCK_BUTTON_CLOSE);
    gnome_dialog_set_accelerator( GNOME_DIALOG(box), i, choices[i], 0);
    g_print("Setting accelerator '%c' for button %d\n", 'Q', i);
#endif

    gnome_dialog_set_close(GNOME_DIALOG(box), TRUE);
    mainWnd = ghack_get_main_window();
    gtk_window_set_modal(GTK_WINDOW(box), TRUE);
    gtk_window_set_title(GTK_WINDOW(box), "GnomeHack");
    if (mainWnd != NULL) {
        gnome_dialog_set_parent(GNOME_DIALOG(box), GTK_WINDOW(mainWnd));
    }

    ret = gnome_dialog_run_and_close(GNOME_DIALOG(box));

#if 0
    g_print("You selected button %d\n", ret);
#endif

    if (ret == -1)
        return ('\033');
    else
        return (choices[ret]);
}

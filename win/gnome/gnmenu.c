/* NetHack 3.6	gnmenu.c	$NHDT-Date: 1432512805 2015/05/25 00:13:25 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#include <string.h>
#include <gtk/gtk.h>
#include <gnome.h>
#include "gnmenu.h"
#include "gnmain.h"
#include "gnbind.h"
#include "func_tab.h"

typedef enum { MenuUnknown = 0, MenuText, MenuMenu } MenuWinType;

typedef struct {
    ANY_P identifier;
    gchar accelerator[BUFSZ];
    int itemNumber;
    int selected;
} menuItem;

typedef struct {
    int curItem;
    int numRows;
    int charIdx;
    guint32 lastTime;
} extMenu;

static GdkColor color_blue = { 0, 0, 0, 0xffff };

static void
ghack_menu_window_key(GtkWidget *menuWin, GdkEventKey *event, gpointer data)
{
    int i, numRows;
    menuItem *item;
    MenuWinType isMenu;

    isMenu = (MenuWinType) GPOINTER_TO_INT(
        gtk_object_get_data(GTK_OBJECT(menuWin), "isMenu"));

    if (isMenu == MenuMenu) {
        GtkWidget *clist;
        gint selection_mode;

        clist = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(menuWin), "clist"));
        g_assert(clist != NULL);
        numRows = GPOINTER_TO_INT(
            gtk_object_get_data(GTK_OBJECT(clist), "numRows"));
        selection_mode = GPOINTER_TO_INT(
            gtk_object_get_data(GTK_OBJECT(clist), "selection_mode"));
        for (i = 0; i <= numRows; ++i) {
            item = (menuItem *) gtk_clist_get_row_data(GTK_CLIST(clist), i);
            if (item == NULL)
                continue;
            if (!strcmp(item->accelerator, ""))
                continue;

            if ((!strcmp(item->accelerator, event->string))
                || ((selection_mode == GTK_SELECTION_MULTIPLE)
                    && (event->keyval == ','))) {
                if (item->selected) {
                    gtk_clist_unselect_row(GTK_CLIST(clist), item->itemNumber,
                                           0);
                    item->selected = FALSE;
                } else {
                    gtk_clist_select_row(GTK_CLIST(clist), item->itemNumber,
                                         0);
                    if (gtk_clist_row_is_visible(GTK_CLIST(clist),
                                                 item->itemNumber)
                        != GTK_VISIBILITY_FULL)
                        gtk_clist_moveto(GTK_CLIST(clist), item->itemNumber,
                                         0, 0.5, 0);
                    item->selected = TRUE;
                }
            }
        }
    }
}

static void
ghack_menu_row_selected(GtkCList *clist, int row, int col, GdkEvent *event)
{
    /* FIXME: Do something */
}

void
ghack_menu_window_clear(GtkWidget *menuWin, gpointer data)
{
    MenuWinType isMenu;
    int i, numRows;
    menuItem *item;

    isMenu = (MenuWinType) GPOINTER_TO_INT(
        gtk_object_get_data(GTK_OBJECT(menuWin), "isMenu"));

    if (isMenu == MenuMenu) {
        GtkWidget *clist;

        clist = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(menuWin), "clist"));
        g_assert(clist != NULL);

        /* destroy existing menu data, if any */
        if (clist) {
            /* destroy all the row_data we stored in the clist */
            numRows = GPOINTER_TO_INT(
                gtk_object_get_data(GTK_OBJECT(clist), "numRows"));
            for (i = 0; i < numRows; i++) {
                item =
                    (menuItem *) gtk_clist_get_row_data(GTK_CLIST(clist), i);
                if (item != NULL) {
                    g_free(item);
                    gtk_clist_set_row_data(GTK_CLIST(clist), i,
                                           (gpointer) NULL);
                }
            }
            gtk_object_set_data(GTK_OBJECT(clist), "numItems",
                                GINT_TO_POINTER(-1));
            gtk_clist_clear(GTK_CLIST(clist));
        }
    }

    else if (isMenu == MenuText) {
        GnomeLess *gless;

        gless = GNOME_LESS(gtk_object_get_data(GTK_OBJECT(menuWin), "gless"));
        g_assert(gless != NULL);

        gtk_editable_delete_text(GTK_EDITABLE(gless->text), 0, 0);
    }
}

void
ghack_menu_window_display(GtkWidget *menuWin, gboolean blocking,
                          gpointer data)
{
    // if(blocking) {
    gnome_dialog_close_hides(GNOME_DIALOG(menuWin), TRUE);
    gnome_dialog_set_close(GNOME_DIALOG(menuWin), TRUE);
    gnome_dialog_run_and_close(GNOME_DIALOG(menuWin));
    //}
    // else {
    // gtk_widget_show(menuWin);
    //}
}

gint
ghack_menu_hide(GtkWidget *menuWin, GdkEvent *event, gpointer data)
{
    gtk_widget_hide(menuWin);
    return FALSE; /* FIXME: what is correct result here? */
}

void
ghack_menu_window_start_menu(GtkWidget *menuWin, gpointer data)
{
    GtkWidget *frame1, *swin, *clist;
    MenuWinType isMenu;

    g_assert(menuWin != NULL);
    g_assert(data == NULL);

    /* destroy existing menu data, if any */
    frame1 = gtk_object_get_data(GTK_OBJECT(menuWin), "frame1");
    if (frame1)
        gtk_widget_destroy(frame1);

    isMenu = MenuMenu;
    gtk_object_set_data(GTK_OBJECT(menuWin), "isMenu",
                        GINT_TO_POINTER(isMenu));

    gtk_widget_set_usize(GTK_WIDGET(menuWin), 500, 400);
    gtk_window_set_policy(GTK_WINDOW(menuWin), TRUE, TRUE, FALSE);

    frame1 = gtk_frame_new("Make your selection");
    g_assert(frame1 != NULL);
    gtk_object_set_data(GTK_OBJECT(menuWin), "frame1", frame1);
    gtk_widget_show(GTK_WIDGET(frame1));
    gtk_container_set_border_width(GTK_CONTAINER(frame1), 5);
    gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(menuWin)->vbox), frame1, TRUE,
                       TRUE, 0);

    swin = gtk_scrolled_window_new(NULL, NULL);
    g_assert(swin != NULL);
    gtk_object_set_data(GTK_OBJECT(menuWin), "swin", swin);
    gtk_widget_show(GTK_WIDGET(swin));
    gtk_container_add(GTK_CONTAINER(frame1), swin);

    clist = gtk_clist_new(4);
    g_assert(clist != NULL);
    gtk_object_set_data(GTK_OBJECT(menuWin), "clist", clist);
    gtk_widget_show(GTK_WIDGET(clist));
    gtk_container_add(GTK_CONTAINER(swin), clist);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    gtk_signal_connect(GTK_OBJECT(clist), "select_row",
                       GTK_SIGNAL_FUNC(ghack_menu_row_selected), NULL);
    gtk_object_set_data(GTK_OBJECT(clist), "numItems", GINT_TO_POINTER(-1));
}

int
ghack_menu_window_select_menu(GtkWidget *menuWin, MENU_ITEM_P **_selected,
                              gint how)
{
    gint rc;
    guint num_sel, i, idx;
    GtkWidget *clist;
    GList *cur;
    MENU_ITEM_P *selected = NULL;
    menuItem *item;

    g_assert(_selected != NULL);
    *_selected = NULL;

    if (how == PICK_NONE) {
        gnome_dialog_close_hides(GNOME_DIALOG(menuWin), TRUE);
        rc = gnome_dialog_run_and_close(GNOME_DIALOG(menuWin));
        return (rc == 1 ? -1 : 0);
    }

    clist = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(menuWin), "clist"));
    g_assert(clist != NULL);

    gtk_object_set_data(GTK_OBJECT(clist), "selection_mode",
                        GINT_TO_POINTER((how == PICK_ANY)
                                            ? GTK_SELECTION_MULTIPLE
                                            : GTK_SELECTION_SINGLE));
    gtk_clist_set_selection_mode(GTK_CLIST(clist),
                                 (how == PICK_ANY) ? GTK_SELECTION_MULTIPLE
                                                   : GTK_SELECTION_SINGLE);
    gnome_dialog_close_hides(GNOME_DIALOG(menuWin), TRUE);
    rc = gnome_dialog_run_and_close(GNOME_DIALOG(menuWin));
    if ((rc == 1) || (GTK_CLIST(clist)->selection == NULL)) {
        return (-1);
    }

    num_sel = g_list_length(GTK_CLIST(clist)->selection);
    if (num_sel < 1) {
        return (-1);
    }

    /* fill in array with selections from clist */
    selected = g_new0(MENU_ITEM_P, num_sel);
    g_assert(selected != NULL);
    cur = GTK_CLIST(clist)->selection;
    i = 0;
    while (cur) {
        g_assert(i < num_sel);

        /* grab row number from clist selection list */
        idx = GPOINTER_TO_INT(cur->data);

        item = (menuItem *) gtk_clist_get_row_data(GTK_CLIST(clist), idx);
        selected[i].item = item->identifier;
        selected[i].count = -1;
        cur = g_list_next(cur);
        i++;
    }

    *_selected = selected;

    return ((int) num_sel);
}

void
ghack_menu_window_add_menu(GtkWidget *menuWin, gpointer menu_item,
                           gpointer data)
{
    GHackMenuItem *item;
    GtkWidget *clist;
    gchar buf[BUFSZ] = "", accelBuf[BUFSZ] = "";
    gchar *pbuf;
    char *text[4] = { buf, NULL, NULL, NULL };
    gint nCurrentRow = -1, numItems = -1;
    MenuWinType isMenu;
    GtkStyle *bigStyle = NULL;
    gboolean item_selectable;
    GdkImlibImage *image;
    static gboolean special;

    g_assert(menu_item != NULL);
    item = (GHackMenuItem *) menu_item;
    item_selectable = (item->identifier->a_int == 0) ? FALSE : TRUE;
    isMenu = (MenuWinType) GPOINTER_TO_INT(
        gtk_object_get_data(GTK_OBJECT(menuWin), "isMenu"));

    clist = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(menuWin), "clist"));
    g_assert(clist != NULL);
    /* This is a special kludge to make the special hidden help menu item work
     * as designed */
    if (special == TRUE) {
        special = FALSE;
        item_selectable = TRUE;
    }
    if (!strcmp(item->str, "The NetHack license.")) {
        special = TRUE;
    }

    if (item->str) {
        /* First, make a new blank entry in the clist */
        nCurrentRow = gtk_clist_append(GTK_CLIST(clist), text);

        if (item->glyph != NO_GLYPH) {
            image = ghack_image_from_glyph(item->glyph, FALSE);
            if (image == NULL || image->pixmap == NULL) {
                g_warning("Bummer -- having to force rendering for glyph %d!",
                          item->glyph);
                /* wierd -- pixmap is NULL so retry rendering it */
                image = ghack_image_from_glyph(item->glyph, TRUE);
            }
            if (image == NULL || image->pixmap == NULL) {
                g_error("Aiiee! glyph is still NULL for item\n\"%s\"",
                        item->str);
            } else
                gtk_clist_set_pixmap(GTK_CLIST(clist), nCurrentRow, 1,
                                     gdk_imlib_move_image(image),
                                     gdk_imlib_move_mask(image));
        }
        if (item->accelerator) {
            /* FIXME: handle accelerator, */
            g_snprintf(accelBuf, sizeof(accelBuf), "%c ", item->accelerator);
            gtk_clist_set_text(GTK_CLIST(clist), nCurrentRow, 0, accelBuf);
            g_snprintf(buf, sizeof(buf), "%s", item->str);
            gtk_clist_set_text(GTK_CLIST(clist), nCurrentRow, 2, buf);
        } else {
            if (item->group_accel) {
                /* FIXME: maybe some day I should try to handle
                 * group accelerators... */
            }
            if (((item->attr == 0) && (item->identifier->a_int != 0))
                || (special == TRUE)) {
                numItems = GPOINTER_TO_INT(gtk_object_get_data(
                               GTK_OBJECT(clist), "numItems")) + 1;

                /* Ok, now invent a unique accelerator */
                if (('a' + numItems) <= 'z') {
                    g_snprintf(accelBuf, sizeof(accelBuf), "%c ",
                               'a' + numItems);
                    gtk_clist_set_text(GTK_CLIST(clist), nCurrentRow, 0,
                                       accelBuf);
                } else if (('A' + numItems - 26) <= 'Z') {
                    g_snprintf(accelBuf, sizeof(accelBuf), "%c ",
                               'A' + numItems - 26);
                    gtk_clist_set_text(GTK_CLIST(clist), nCurrentRow, 0,
                                       accelBuf);
                } else {
                    accelBuf[0] = buf[0] = 0;
                }
                g_snprintf(buf, sizeof(buf), "%s", item->str);
                gtk_clist_set_text(GTK_CLIST(clist), nCurrentRow, 2, buf);
                gtk_object_set_data(GTK_OBJECT(clist), "numItems",
                                    GINT_TO_POINTER(numItems));

                /* This junk is to specially handle the options menu */
                pbuf = strstr(buf, " [");
                if (pbuf == NULL) {
                    pbuf = strstr(buf, "\t[");
                }
                if (pbuf != NULL) {
                    *pbuf = 0;
                    pbuf++;
                    gtk_clist_set_text(GTK_CLIST(clist), nCurrentRow, 3,
                                       pbuf);
                }
            }
            /* FIXME: handle more than 26*2 accelerators (but how?
             * since I only have so many keys to work with???)
            else
            {
                foo();
            }
            */
            else {
                g_snprintf(buf, sizeof(buf), "%s", item->str);
                pbuf = strstr(buf, " [");
                if (pbuf == NULL) {
                    pbuf = strstr(buf, "\t[");
                }
                if (pbuf != NULL) {
                    *pbuf = 0;
                    pbuf++;
                    gtk_clist_set_text(GTK_CLIST(clist), nCurrentRow, 3,
                                       pbuf);
                }
                gtk_clist_set_text(GTK_CLIST(clist), nCurrentRow, 2, buf);
            }
        }

        if (item->attr) {
            switch (item->attr) {
            case ATR_ULINE:
            case ATR_BOLD:
            case ATR_BLINK:
            case ATR_INVERSE:
                bigStyle = gtk_style_copy(GTK_WIDGET(clist)->style);
                g_assert(bigStyle != NULL);
                gdk_font_unref(bigStyle->font);
                bigStyle->font =
                    gdk_font_load("-misc-fixed-*-*-*-*-20-*-*-*-*-*-*-*");
                bigStyle->fg[GTK_STATE_NORMAL] = color_blue;
                gtk_clist_set_cell_style(GTK_CLIST(clist), nCurrentRow, 2,
                                         bigStyle);
                item_selectable = FALSE;
            }
        }

        g_assert(nCurrentRow >= 0);
        gtk_clist_set_selectable(GTK_CLIST(clist), nCurrentRow,
                                 item_selectable);

        if (item_selectable == TRUE && item->presel == TRUE) {
            /* pre-select this item */
            gtk_clist_select_row(GTK_CLIST(clist), nCurrentRow, 0);
        }

        gtk_object_set_data(GTK_OBJECT(clist), "numRows",
                            GINT_TO_POINTER(nCurrentRow));

        /* We have to allocate memory here, since the menu_item currently
         * lives on the stack, and will otherwise go to the great bit bucket
         * in the sky as soon as this function exits, which would leave a
         * pointer to crap in the row_data.  Use g_memdup to make a private,
         * persistant copy of the item identifier.
         *
         * We need to arrange to blow away this memory somewhere (like
         * ghack_menu_destroy and ghack_menu_window_clear for example).
         *
         *  -Erik
         */
        {
            menuItem newItem;
            menuItem *pNewItem;

            newItem.identifier = *item->identifier;
            newItem.itemNumber = nCurrentRow;
            newItem.selected = FALSE;
            newItem.accelerator[0] = 0;
            /* only copy 1 char, since accel keys are by definition 1 char */
            if (accelBuf[0]) {
                strncpy(newItem.accelerator, accelBuf, 1);
            }
            newItem.accelerator[1] = 0;

            pNewItem = g_memdup(&newItem, sizeof(menuItem));
            gtk_clist_set_row_data(GTK_CLIST(clist), nCurrentRow,
                                   (gpointer) pNewItem);
        }
    }
    /* Now adjust the column widths to match the contents */
    gtk_clist_columns_autosize(GTK_CLIST(clist));
}

void
ghack_menu_window_end_menu(GtkWidget *menuWin, gpointer data)
{
    const char *p = (const char *) data;

    if ((p) && (*p)) {
        GtkWidget *frame1 =
            gtk_object_get_data(GTK_OBJECT(menuWin), "frame1");
        g_assert(frame1 != NULL);

        gtk_frame_set_label(GTK_FRAME(frame1), p);
    }
}

void
ghack_menu_window_put_string(GtkWidget *menuWin, int attr, const char *text,
                             gpointer data)
{
    GnomeLess *gless;
    MenuWinType isMenu;

    if (text == NULL)
        return;

    isMenu = (MenuWinType) GPOINTER_TO_INT(
        gtk_object_get_data(GTK_OBJECT(menuWin), "isMenu"));

    if (isMenu == MenuText) {
        gless = GNOME_LESS(gtk_object_get_data(GTK_OBJECT(menuWin), "gless"));
        g_assert(gless != NULL);
        g_assert(gless->text != NULL);
        g_assert(GTK_IS_TEXT(gless->text));

        /* Don't bother with attributes yet */
        gtk_text_insert(GTK_TEXT(gless->text), NULL, NULL, NULL, text, -1);
        gtk_text_insert(GTK_TEXT(gless->text), NULL, NULL, NULL, "\n", -1);

    }

    else if (isMenu == MenuUnknown) {
        isMenu = MenuText;
        gtk_object_set_data(GTK_OBJECT(menuWin), "isMenu",
                            GINT_TO_POINTER(isMenu));

        gtk_widget_set_usize(GTK_WIDGET(menuWin), 500, 400);
        gtk_window_set_policy(GTK_WINDOW(menuWin), TRUE, TRUE, FALSE);

        gless = GNOME_LESS(gnome_less_new());
        g_assert(gless != NULL);
        gtk_object_set_data(GTK_OBJECT(menuWin), "gless", gless);
        gtk_widget_show(GTK_WIDGET(gless));

        gnome_less_show_string(gless, text);
        gtk_text_insert(GTK_TEXT(gless->text), NULL, NULL, NULL, "\n", -1);

        gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(menuWin)->vbox),
                           GTK_WIDGET(gless), TRUE, TRUE, 0);
    }
}

void
ghack_menu_destroy(GtkWidget *menuWin, gpointer data)
{
    MenuWinType isMenu;

    isMenu = (MenuWinType) GPOINTER_TO_INT(
        gtk_object_get_data(GTK_OBJECT(menuWin), "isMenu"));

    if (isMenu == MenuText) {
        GnomeLess *gless;

        gless = GNOME_LESS(gtk_object_get_data(GTK_OBJECT(menuWin), "gless"));
        g_assert(gless != NULL);
        g_assert(gless->text != NULL);
        g_assert(GTK_IS_TEXT(gless->text));
        gtk_widget_destroy(GTK_WIDGET(gless));
    }

    else if (isMenu == MenuMenu) {
        GtkWidget *frame1, *swin, *clist;

        /* destroy existing menu data, if any */
        clist = gtk_object_get_data(GTK_OBJECT(menuWin), "clist");
        if (clist) {
            /* destroy all the row_data we stored in the clist */
            int i, numRows;
            menuItem *item;
            numRows = GPOINTER_TO_INT(
                gtk_object_get_data(GTK_OBJECT(clist), "numRows"));
            for (i = 0; i < numRows; i++) {
                item =
                    (menuItem *) gtk_clist_get_row_data(GTK_CLIST(clist), i);
                if (item != NULL) {
                    g_free(item);
                    gtk_clist_set_row_data(GTK_CLIST(clist), i,
                                           (gpointer) NULL);
                }
            }

            gtk_object_set_data(GTK_OBJECT(clist), "numItems",
                                GINT_TO_POINTER(-1));
            gtk_widget_destroy(clist);
        }
        swin = gtk_object_get_data(GTK_OBJECT(menuWin), "swin");
        if (swin) {
            gtk_widget_destroy(swin);
        }
        frame1 = gtk_object_get_data(GTK_OBJECT(menuWin), "frame1");
        if (frame1) {
            gtk_widget_destroy(frame1);
        }
    }
    gnome_delete_nhwindow_by_reference(menuWin);
}

GtkWidget *
ghack_init_menu_window(void)
{
    GtkWidget *menuWin = NULL;
    GtkWidget *parent = ghack_get_main_window();

    menuWin = gnome_dialog_new("GnomeHack", GNOME_STOCK_BUTTON_OK,
                               GNOME_STOCK_BUTTON_CANCEL, NULL);

    gnome_dialog_set_default(GNOME_DIALOG(menuWin), 0);
    gtk_signal_connect(GTK_OBJECT(menuWin), "destroy",
                       GTK_SIGNAL_FUNC(ghack_menu_destroy), NULL);

    gtk_signal_connect(GTK_OBJECT(menuWin), "delete_event",
                       GTK_SIGNAL_FUNC(ghack_menu_hide), NULL);

    gtk_signal_connect(GTK_OBJECT(menuWin), "ghack_clear",
                       GTK_SIGNAL_FUNC(ghack_menu_window_clear), NULL);

    gtk_signal_connect(GTK_OBJECT(menuWin), "ghack_display",
                       GTK_SIGNAL_FUNC(ghack_menu_window_display), NULL);

    gtk_signal_connect(GTK_OBJECT(menuWin), "ghack_start_menu",
                       GTK_SIGNAL_FUNC(ghack_menu_window_start_menu), NULL);

    gtk_signal_connect(GTK_OBJECT(menuWin), "ghack_add_menu",
                       GTK_SIGNAL_FUNC(ghack_menu_window_add_menu), NULL);

    gtk_signal_connect(GTK_OBJECT(menuWin), "ghack_end_menu",
                       GTK_SIGNAL_FUNC(ghack_menu_window_end_menu), NULL);

    gtk_signal_connect(GTK_OBJECT(menuWin), "ghack_select_menu",
                       GTK_SIGNAL_FUNC(ghack_menu_window_select_menu), NULL);

    gtk_signal_connect(GTK_OBJECT(menuWin), "ghack_putstr",
                       GTK_SIGNAL_FUNC(ghack_menu_window_put_string), NULL);

    gtk_signal_connect(GTK_OBJECT(menuWin), "key_press_event",
                       GTK_SIGNAL_FUNC(ghack_menu_window_key), NULL);

    /* Center the dialog over parent */
    g_assert(parent != NULL);
    g_assert(menuWin != NULL);
    g_assert(GTK_IS_WINDOW(parent));
    g_assert(GNOME_IS_DIALOG(menuWin));
    gnome_dialog_set_parent(GNOME_DIALOG(menuWin), GTK_WINDOW(parent));

    return menuWin;
}

static void
ghack_ext_key_hit(GtkWidget *menuWin, GdkEventKey *event, gpointer data)
{
    GtkWidget *clist;
    extMenu *info = (extMenu *) data;
    int i;
    char c = event->string[0];

    clist = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(menuWin), "clist"));
    g_assert(clist != NULL);

    /* if too long between keystrokes, reset to initial state */
    if (event->time - info->lastTime > 500)
        goto init_state;

    /* see if current item continue to match */
    if (info->charIdx > 0) {
        if (extcmdlist[info->curItem].ef_txt[info->charIdx] == c) {
            ++info->charIdx;
            goto found;
        }
    }

    /* see if the prefix matches a later command in the list */
    if (info->curItem >= 0) {
        for (i = info->curItem + 1; i < info->numRows; ++i) {
            if (!strncmp(extcmdlist[info->curItem].ef_txt,
                         extcmdlist[i].ef_txt, info->charIdx)) {
                if (extcmdlist[i].ef_txt[info->charIdx] == c) {
                    ++info->charIdx;
                    info->curItem = i;
                    goto found;
                }
            }
        }
    }

init_state:
    /* reset to initial state, look for matching 1st character */
    for (i = 0; i < info->numRows; ++i) {
        if (extcmdlist[i].ef_txt[0] == c) {
            info->charIdx = 1;
            info->curItem = i;
            goto found;
        }
    }

    /* no match: leave prior, if any selection in place */
    return;

found:
    info->lastTime = event->time;
    gtk_clist_select_row(GTK_CLIST(clist), info->curItem, 0);
    if (gtk_clist_row_is_visible(GTK_CLIST(clist), info->curItem)
        != GTK_VISIBILITY_FULL)
        gtk_clist_moveto(GTK_CLIST(clist), info->curItem, 0, 0.5, 0);
}

int
ghack_menu_ext_cmd(void)
{
    int n;
    GtkWidget *dialog;
    GtkWidget *swin;
    GtkWidget *frame1;
    GtkWidget *clist;
    extMenu info;

    dialog = gnome_dialog_new("Extended Commands", GNOME_STOCK_BUTTON_OK,
                              GNOME_STOCK_BUTTON_CANCEL, NULL);
    gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
    gtk_signal_connect(GTK_OBJECT(dialog), "key_press_event",
                       GTK_SIGNAL_FUNC(ghack_ext_key_hit), &info);

    frame1 = gtk_frame_new("Make your selection");
    gtk_object_set_data(GTK_OBJECT(dialog), "frame1", frame1);
    gtk_widget_show(frame1);
    gtk_container_border_width(GTK_CONTAINER(frame1), 3);

    swin = gtk_scrolled_window_new(NULL, NULL);
    clist = gtk_clist_new(2);
    gtk_object_set_data(GTK_OBJECT(dialog), "clist", clist);
    gtk_widget_set_usize(clist, 500, 400);
    gtk_container_add(GTK_CONTAINER(swin), clist);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    gtk_signal_connect(GTK_OBJECT(clist), "select_row",
                       GTK_SIGNAL_FUNC(ghack_menu_row_selected), NULL);

    gtk_container_add(GTK_CONTAINER(frame1), swin);
    gtk_box_pack_start_defaults(GTK_BOX(GNOME_DIALOG(dialog)->vbox), frame1);

    /* Add the extended commands into the list here... */
    for (n = 0; extcmdlist[n].ef_txt; ++n) {
        const char *text[3] = { extcmdlist[n].ef_txt, extcmdlist[n].ef_desc,
                                NULL };
        gtk_clist_insert(GTK_CLIST(clist), n, (char **) text);
    }

    /* fill in starting info fields */
    info.curItem = -1;
    info.numRows = n;
    info.charIdx = 0;
    info.lastTime = 0;

    gtk_clist_columns_autosize(GTK_CLIST(clist));
    gtk_widget_show_all(swin);

    /* Center the dialog over over parent */
    gnome_dialog_set_default(GNOME_DIALOG(dialog), 0);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gnome_dialog_set_parent(GNOME_DIALOG(dialog),
                            GTK_WINDOW(ghack_get_main_window()));

    /* Run the dialog -- returning whichever button was pressed */
    n = gnome_dialog_run_and_close(GNOME_DIALOG(dialog));

    /* Quit on button 2 or error */
    return (n != 0) ? -1 : info.curItem;
}

/* NetHack 3.6	gnworn.c	2009/05/06 10:58:06  1.3 */
/*
 * $NHDT-Date: 1432512804 2015/05/25 00:13:24 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $
 */
/* Copyright (C) 2002, Dylan Alex Simon			*/
/* NetHack may be freely redistributed.  See license for details. */

#include "gnworn.h"
#include "gnglyph.h"
#include "gnsignal.h"
#include "gnomeprv.h"

#define WORN_WIDTH 3
#define WORN_HEIGHT 6

#define WORN_OBJECT_LIST /* struct obj *[WORN_HEIGHT][WORN_WIDTH] = */ \
    {                                                                  \
        {                                                              \
            uquiver, uarmh, u.twoweap ? NULL : uswapwep                \
        }                                                              \
        , { u.twoweap ? uswapwep : NULL, ublindf, uwep },              \
            { uleft, uamul, uright }, { uarms, uarmc, uarmg },         \
            { uarmu, uarm, uskin },                                    \
        {                                                              \
            uball, uarmf, uchain                                       \
        }                                                              \
    }

static GtkWidget *worn_contents[WORN_HEIGHT][WORN_WIDTH];
static struct obj *last_worn_objects[WORN_HEIGHT][WORN_WIDTH];

GdkImlibImage *image_of_worn_object(struct obj *o);
void ghack_worn_display(GtkWidget *win, boolean block, gpointer data);

GtkWidget *
ghack_init_worn_window()
{
    GtkWidget *top;
    GtkWidget *table;
    GtkWidget *tablealign;
    GtkWidget *label;
    int i, j;

    top = gtk_vbox_new(FALSE, 2);

    table = gtk_table_new(WORN_HEIGHT, WORN_WIDTH, TRUE);
    for (i = 0; i < WORN_HEIGHT; i++) {
        for (j = 0; j < WORN_WIDTH; j++) {
            worn_contents[i][j] =
                gnome_pixmap_new_from_imlib(image_of_worn_object(NULL));
            last_worn_objects[i][j] = NULL; /* a pointer that will never be */
            gtk_table_attach(GTK_TABLE(table),
                             GTK_WIDGET(worn_contents[i][j]), j, j + 1, i,
                             i + 1, 0, 0, 0, 0);
        }
    }
    tablealign = gtk_alignment_new(0.5, 0.0, 0.0, 1.0);
    gtk_box_pack_start(GTK_BOX(top), tablealign, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(tablealign), table);

    label = gtk_label_new("Equipment");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(top), label, FALSE, FALSE, 0);

    gtk_signal_connect(GTK_OBJECT(top), "ghack_display",
                       GTK_SIGNAL_FUNC(ghack_worn_display), NULL);

    return top;
}

GdkImlibImage *
image_of_worn_object(struct obj *o)
{
    int glyph;
    GdkImlibImage *im;

    if (o)
        glyph = obj_to_glyph(o, rn2_on_display_rng);
    else
        glyph = cmap_to_glyph(S_stone);

    im = ghack_image_from_glyph(glyph, FALSE);

    return im;
}

void
ghack_worn_display(GtkWidget *win, boolean block, gpointer data)
{
    int i, j;
    struct obj *worn_objects[WORN_HEIGHT][WORN_WIDTH] = WORN_OBJECT_LIST;

    for (i = 0; i < WORN_HEIGHT; i++) {
        for (j = 0; j < WORN_WIDTH; j++) {
            if (worn_objects[i][j] != last_worn_objects[i][j]) {
                last_worn_objects[i][j] = worn_objects[i][j];
                gnome_pixmap_load_imlib(
                    GNOME_PIXMAP(worn_contents[i][j]),
                    image_of_worn_object(worn_objects[i][j]));
            }
        }
    }
}

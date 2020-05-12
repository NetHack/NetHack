/* NetHack 3.6	gnmap.c	$NHDT-Date: 1432512806 2015/05/25 00:13:26 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* Copyright (C) 1998 by Anthony Taylor <tonyt@ptialaska.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "gnmap.h"
#include "gnglyph.h"
#include "gnsignal.h"
#include "hack.h"

#ifndef ROWNO
#define ROWNO 21
#define COLNO 80
#endif

/* globals static to this file go here */
struct {
    GnomeCanvas *canvas;
    GnomeCanvasImage *map[(ROWNO + 1) * COLNO];
    GnomeCanvasImage *overlay[(ROWNO + 1) * COLNO];
    double zoom;
    GtkWidget *frame;
} ghack_map;

static GdkImlibImage *background;
static GdkImlibImage *petmark;
static GnomeCanvasGroup *myCanvasGroup;

/* static function declarations -- local to this file go here */
void ghack_map_cursor_to(GtkWidget *win, int x, int y, gpointer data);
void ghack_map_putstr(GtkWidget *win, int attr, const char *text,
                      gpointer data);
void ghack_map_print_glyph(GtkObject *win, guint x, guint y,
                           GdkImlibImage *im, gpointer data);
void ghack_map_clear(GtkWidget *win, gpointer data);
static void ghack_map_display(GtkWidget *win, boolean block, gpointer data);
static void ghack_map_cliparound(GtkWidget *win, int x, int y, gpointer data);
static void ghack_map_window_zoom(GtkAdjustment *adj, gpointer data);

/* The following XPM is the artwork of Warwick Allison
 * <warwick@troll.no>.  It has been borrowed from
 * the most excellent NetHackQt, until such time as
 * we can come up with something better.
 *
 * More information about NetHackQt can be had from:
 * http://www.troll.no/~warwick/nethack/
 */

/* XPM */
static char *pet_mark_xpm[] = {
    /* width height ncolors chars_per_pixel */
    "8 7 2 1",
    /* colors */
    ". c None", "  c #FF0000",
    /* pixels */
    "........", "..  .  .",    ".       ", ".       ",
    "..     .", "...   ..",    ".... ..."
};

/* NAME:
 *     ghack_init_map_window( )
 *
 * ARGUMENTS:
 *     NONE
 *
 * RETURNS:
 *     GtkWidget*
 *
 * PURPOSE:
 *     Create the basic map necessities.  Create a canvas;
 *     give it a background.  Attach all the right signals
 *     to all the right places.  Generally prepare the map
 *     to behave properly.
*/

GtkWidget *
ghack_init_map_window()
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *table;
    GtkWidget *frame;
    GtkWidget *w;
    GtkWidget *hSeparator;
    GtkAdjustment *adj;
    GnomeCanvasImage *bg;
    double width, height, x, y;
    int i;

    width = COLNO * ghack_glyph_width();
    height = ROWNO * ghack_glyph_height();

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);

    /* Add in a horiz seperator */
    hSeparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hSeparator, FALSE, FALSE, 2);
    gtk_widget_show(hSeparator);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* Create the Zoom spinbutton.
    */
    ghack_map.zoom = 1.0;
    w = gtk_label_new("Zoom:");
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);
    gtk_widget_show(w);
    adj =
        GTK_ADJUSTMENT(gtk_adjustment_new(1.00, 0.5, 3.00, 0.05, 0.50, 0.50));
    w = gtk_spin_button_new(adj, 0.5, 2);
    gtk_widget_set_usize(w, 50, 0);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);
    gtk_widget_show(w);

    /* Canvas and scrollbars
    */
    gtk_widget_push_visual(gdk_imlib_get_visual());
    gtk_widget_push_colormap(gdk_imlib_get_colormap());
    ghack_map.canvas = GNOME_CANVAS(gnome_canvas_new());
    // gtk_widget_push_visual(gdk_rgb_get_visual());
    // gtk_widget_push_colormap(gdk_rgb_get_cmap());
    // ghack_map.canvas = GNOME_CANVAS (gnome_canvas_new_aa());

    gtk_widget_pop_colormap();
    gtk_widget_pop_visual();
    gtk_widget_show(GTK_WIDGET(ghack_map.canvas));

    table = gtk_table_new(2, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table), 4);
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
    gtk_widget_show(table);

    frame = gtk_frame_new(NULL);
    ghack_map.frame = frame;
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_table_attach(GTK_TABLE(table), frame, 0, 1, 0, 1,
                     GTK_EXPAND | GTK_FILL | GTK_SHRINK,
                     GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0);
    gtk_widget_show(frame);

    gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(ghack_map.canvas));
    gnome_canvas_set_scroll_region(GNOME_CANVAS(ghack_map.canvas), 0, 0,
                                   width + 2 * ghack_glyph_width(),
                                   height + 2 * ghack_glyph_height());

    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(ghack_map.canvas), 1.0);

    w = gtk_hscrollbar_new(GTK_LAYOUT(ghack_map.canvas)->hadjustment);
    gtk_table_attach(GTK_TABLE(table), w, 0, 1, 1, 2,
                     GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL, 0, 0);
    gtk_widget_show(w);

    w = gtk_vscrollbar_new(GTK_LAYOUT(ghack_map.canvas)->vadjustment);
    gtk_table_attach(GTK_TABLE(table), w, 1, 2, 0, 1, GTK_FILL,
                     GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0);
    gtk_widget_show(w);

    myCanvasGroup = GNOME_CANVAS_GROUP(gnome_canvas_item_new(
        gnome_canvas_root(GNOME_CANVAS(ghack_map.canvas)),
        gnome_canvas_group_get_type(), "x", 0.0, "y", 0.0, NULL));

    /* Tile the map background with a pretty image */
    background = gdk_imlib_load_image((char *) "mapbg.xpm");
    if (background == NULL) {
        g_warning(
            "Bummer! Failed to load the map background image (mapbg.xpm)!");
    } else {
        gdk_imlib_render(background, background->rgb_width,
                         background->rgb_height);

        /* Tile the map background */
        for (y = 0; y < height + background->rgb_height;
             y += background->rgb_height) {
            for (x = 0; x < width + background->rgb_width;
                 x += background->rgb_width) {
                bg = GNOME_CANVAS_IMAGE(gnome_canvas_item_new(
                    myCanvasGroup, gnome_canvas_image_get_type(), "x",
                    (double) x, "y", (double) y, "width",
                    (double) background->rgb_width, "height",
                    (double) background->rgb_height, "image", background,
                    "anchor", (GtkAnchorType) GTK_ANCHOR_CENTER, NULL));
                gnome_canvas_item_lower_to_bottom(GNOME_CANVAS_ITEM(bg));
            }
        }
    }

    /* ghack_map.map is an array of canvas images.  Each cell of
     * the array will contain one tile.  Here, we create the
     * space for the cells and then create the cells for easy
     * access later.
    */
    for (i = 0, y = 0; y < height; y += ghack_glyph_height()) {
        for (x = 0; x < width; x += ghack_glyph_width()) {
            ghack_map.map[i++] = GNOME_CANVAS_IMAGE(gnome_canvas_item_new(
                myCanvasGroup, gnome_canvas_image_get_type(), "x", (double) x,
                "y", (double) y, "width", (double) ghack_glyph_width(),
                "height", (double) ghack_glyph_height(), "anchor",
                GTK_ANCHOR_NORTH_WEST, NULL));
        }
    }

    /* Set up the pet mark image */
    petmark = gdk_imlib_create_image_from_xpm_data(pet_mark_xpm);
    if (petmark == NULL) {
        g_warning("Bummer! Failed to load the pet_mark image!");
    } else {
        gdk_imlib_render(petmark, petmark->rgb_width, petmark->rgb_height);

        /* ghack_map.overlay is an array of canvas images used to
         * overlay tile images...
         */
        for (i = 0, y = 0; y < height; y += ghack_glyph_height()) {
            for (x = 0; x < width; x += ghack_glyph_width()) {
                ghack_map.overlay[i] =
                    GNOME_CANVAS_IMAGE(gnome_canvas_item_new(
                        myCanvasGroup, gnome_canvas_image_get_type(), "x",
                        (double) x, "y", (double) y, "width",
                        (double) petmark->rgb_width, "height",
                        (double) petmark->rgb_height, "image", petmark,
                        "anchor", GTK_ANCHOR_NORTH_WEST, NULL));
                gnome_canvas_item_lower_to_bottom(
                    GNOME_CANVAS_ITEM(ghack_map.overlay[i++]));
            }
        }
    }

    /* Resize the canvas when the spinbutton changes
    */
    gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
                       (GtkSignalFunc) ghack_map_window_zoom,
                       ghack_map.canvas);

    /* Game signals
    */
    gtk_signal_connect(GTK_OBJECT(vbox), "ghack_curs",
                       GTK_SIGNAL_FUNC(ghack_map_cursor_to), NULL);
    gtk_signal_connect(GTK_OBJECT(vbox), "ghack_putstr",
                       GTK_SIGNAL_FUNC(ghack_map_putstr), NULL);
    gtk_signal_connect(GTK_OBJECT(vbox), "ghack_print_glyph",
                       GTK_SIGNAL_FUNC(ghack_map_print_glyph), NULL);
    gtk_signal_connect(GTK_OBJECT(vbox), "ghack_clear",
                       GTK_SIGNAL_FUNC(ghack_map_clear), NULL);
    gtk_signal_connect(GTK_OBJECT(vbox), "ghack_display",
                       GTK_SIGNAL_FUNC(ghack_map_display), NULL);
    gtk_signal_connect(GTK_OBJECT(vbox), "ghack_cliparound",
                       GTK_SIGNAL_FUNC(ghack_map_cliparound), NULL);
    gtk_signal_connect(GTK_OBJECT(ghack_map.canvas), "button_press_event",
                       GTK_SIGNAL_FUNC(ghack_handle_button_press), NULL);
    gtk_signal_connect(GTK_OBJECT(ghack_map.canvas), "gnome_delay_output",
                       GTK_SIGNAL_FUNC(ghack_delay), NULL);

    return GTK_WIDGET(vbox);
}

/* NAME:
 *     ghack_map_window_zoom
 *
 * ARGUMENTS:
 *     double     zoom -- The zoom factor
 *
 * RETURNS:
 *     Nothing.
 *
 * PURPOSE:
 *     Zoom the map image in and out.  This should allow the user to
 *     dynamically scale the map.  Ideally, the background should
 *     *NOT* scale, but this may be impractical.
*/

static void
ghack_map_window_zoom(GtkAdjustment *adj, gpointer data)
{
    if (adj->value > 3.0)
        adj->value = 3.0;
    if (adj->value < 0.5)
        adj->value = 0.5;
    ghack_map.zoom = adj->value;
    gnome_canvas_set_pixels_per_unit(data, adj->value);
}

void
ghack_map_cursor_to(GtkWidget *win, int x, int y, gpointer data)
{
    GnomeCanvasGroup *group;
    static GnomeCanvasRE *cursor = NULL;

    double x1, y1, x2, y2;
    float hp;
    guint r, g, b;

    x1 = x * ghack_glyph_width() - 1;
    y1 = y * ghack_glyph_height() - 1;
    x2 = x1 + ghack_glyph_width() + 2;
    y2 = y1 + ghack_glyph_height() + 2;
    hp = u.mtimedone ? (u.mhmax ? (float) u.mh / u.mhmax : 1)
                     : (u.uhpmax ? (float) u.uhp / u.uhpmax : 1);

    r = 255;
    g = (hp >= 0.75) ? 255 : (hp >= 0.25 ? 255 * 2 * (hp - 0.25) : 0);
    b = (hp >= 0.75) ? 255 * 4 * (hp - 0.75)
                     : (hp >= 0.25 ? 0 : 255 * 4 * (0.25 - hp));

    group = gnome_canvas_root(GNOME_CANVAS(ghack_map.canvas));

    if (!cursor) {
        cursor = GNOME_CANVAS_RE(gnome_canvas_item_new(
            group, gnome_canvas_rect_get_type(), "width_units", 1.0, NULL));
    }
    gnome_canvas_item_set(GNOME_CANVAS_ITEM(cursor), "outline_color_rgba",
                          GNOME_CANVAS_COLOR(r, g, b), "x1", x1, "y1", y1,
                          "x2", x2, "y2", y2, NULL);

    gnome_canvas_item_raise_to_top(GNOME_CANVAS_ITEM(cursor));
    gnome_canvas_item_show(GNOME_CANVAS_ITEM(cursor));
}

void
ghack_map_putstr(GtkWidget *win, int attr, const char *text, gpointer data)
{
    g_warning("Fixme!!! ghack_map_putstr is not implemented");
}

/* NAME:
 *     ghack_map_print_glyph( )
 *
 * ARGUMENTS:
 *     XCHAR_P x, y  -- The coordinates where which to print the glyph
 *     GdkImlibImage*   glyph -- The glyph image to print
 *
 * RETURNS:
 *     Nothing.
 *
 * PURPOSE:
 *     Draw the glyph-tile at the specified coordinates.
*/

void
ghack_map_print_glyph(GtkObject *win, guint x, guint y, GdkImlibImage *im,
                      gpointer data)
{
    GnomeCanvasGroup *group;
    int i = y * COLNO + x;
    int glyph = glyph_at(x, y);
    GnomeCanvasImage *canvas_image = GNOME_CANVAS_IMAGE(ghack_map.map[i]);

    group = gnome_canvas_root(GNOME_CANVAS(ghack_map.canvas));

    gnome_canvas_item_set(GNOME_CANVAS_ITEM(canvas_image), "image", im, NULL);
    gnome_canvas_item_show(GNOME_CANVAS_ITEM(canvas_image));

    canvas_image = GNOME_CANVAS_IMAGE(ghack_map.overlay[i]);

    if (x == u.ux && y == u.uy)
        ghack_map_cliparound(NULL, x, y, NULL);

    if (glyph_is_pet(glyph)
#ifdef TEXTCOLOR
        && iflags.hilite_pet
#endif
        ) {
        gnome_canvas_item_raise_to_top(GNOME_CANVAS_ITEM(canvas_image));
        gnome_canvas_item_show(GNOME_CANVAS_ITEM(canvas_image));
    } else {
        gnome_canvas_item_hide(GNOME_CANVAS_ITEM(canvas_image));
    }
}

/* NAME:
 *     ghack_map_clear( )
 *
 * ARGUMENTS:
 *     NONE
 *
 * RETURNS:
 *     Nothing.
 *
 * PURPOSE:
 *     Clear the map by hiding all the map tiles.
*/

void
ghack_map_clear(GtkWidget *win, gpointer data)
{
    int i;

    for (i = 0; i < ROWNO * COLNO; i++) {
        if (GNOME_IS_CANVAS_IMAGE(ghack_map.map[i])) {
            gnome_canvas_item_hide(GNOME_CANVAS_ITEM(ghack_map.map[i]));
        }
        if (GNOME_IS_CANVAS_IMAGE(ghack_map.overlay[i])) {
            gnome_canvas_item_hide(GNOME_CANVAS_ITEM(ghack_map.overlay[i]));
        }
    }
    gnome_canvas_update_now(GNOME_CANVAS(ghack_map.canvas));
}

void
ghack_map_display(GtkWidget *win, boolean block, gpointer data)
{
    gtk_widget_show_all(GTK_WIDGET(win));
}

void
ghack_map_cliparound(GtkWidget *win, int x, int y, gpointer data)
{
    int map_width, map_height;
    int to_x, to_y;
    int cur_x, cur_y;
    int width, height, half_width, half_height;

    x *= ghack_glyph_width() * ghack_map.zoom;
    y *= ghack_glyph_height() * ghack_map.zoom;
    map_width = COLNO * ghack_glyph_width() * ghack_map.zoom;
    map_height = ROWNO * ghack_glyph_height() * ghack_map.zoom;

    gdk_window_get_size(GTK_LAYOUT(ghack_map.canvas)->bin_window, &width,
                        &height);
    gnome_canvas_get_scroll_offsets(ghack_map.canvas, &cur_x, &cur_y);

    half_width = width * 0.5;
    half_height = height * 0.5;

    if (((x - cur_x) < (width * 0.25)) || ((x - cur_x) > (width * 0.75))) {
        to_x = ((x - half_width) > 0) ? x - half_width : 0;
        to_x = ((x + half_width) > map_width) ? map_width - 2 * half_width
                                              : to_x;
    } else {
        to_x = cur_x;
    }

    if (((y - cur_y) < (height * 0.25)) || ((y - cur_y) > (height * 0.75))) {
        to_y = ((y - half_height) > 0) ? y - half_height : 0;
        to_y = ((y + half_height) > map_height) ? map_height - 2 * half_height
                                                : to_y;
    } else {
        to_y = cur_y;
    }

    if (to_x != cur_x || to_y != cur_y)
        gnome_canvas_scroll_to(ghack_map.canvas, to_x, to_y);
    // gnome_canvas_update_now ( ghack_map.canvas);
}

void
ghack_reinit_map_window()
{
    GnomeCanvasImage *bg;
    double width, height, x, y;
    int i;

    /* ghack_map_clear(NULL, NULL); */

    width = COLNO * ghack_glyph_width();
    height = ROWNO * ghack_glyph_height();

    gnome_canvas_set_scroll_region(GNOME_CANVAS(ghack_map.canvas), 0, 0,
                                   width + 2 * ghack_glyph_width(),
                                   height + 2 * ghack_glyph_height());

    /* remove everything currently in the canvas map */
    gtk_object_destroy(GTK_OBJECT(myCanvasGroup));

    /* Put some groups back */
    myCanvasGroup = GNOME_CANVAS_GROUP(gnome_canvas_item_new(
        gnome_canvas_root(GNOME_CANVAS(ghack_map.canvas)),
        gnome_canvas_group_get_type(), "x", 0.0, "y", 0.0, NULL));

    /* Tile the map background with a pretty image */
    if (background != NULL) {
        /* Tile the map background */
        for (y = 0; y < height + background->rgb_height;
             y += background->rgb_height) {
            for (x = 0; x < width + background->rgb_width;
                 x += background->rgb_width) {
                bg = GNOME_CANVAS_IMAGE(gnome_canvas_item_new(
                    myCanvasGroup, gnome_canvas_image_get_type(), "x",
                    (double) x, "y", (double) y, "width",
                    (double) background->rgb_width, "height",
                    (double) background->rgb_height, "image", background,
                    "anchor", (GtkAnchorType) GTK_ANCHOR_CENTER, NULL));
                gnome_canvas_item_lower_to_bottom(GNOME_CANVAS_ITEM(bg));
            }
        }
    }

    /* ghack_map.map is an array of canvas images.  Each cell of
     * the array will contain one tile.  Here, we create the
     * space for the cells and then create the cells for easy
     * access later.
    */
    for (i = 0, y = 0; y < height; y += ghack_glyph_height()) {
        for (x = 0; x < width; x += ghack_glyph_width()) {
            ghack_map.map[i++] = GNOME_CANVAS_IMAGE(gnome_canvas_item_new(
                myCanvasGroup, gnome_canvas_image_get_type(), "x", (double) x,
                "y", (double) y, "width", (double) ghack_glyph_width(),
                "height", (double) ghack_glyph_height(), "anchor",
                GTK_ANCHOR_NORTH_WEST, NULL));
        }
    }

    if (petmark != NULL) {
        /* ghack_map.overlay is an array of canvas images used to
         * overlay tile images...
        */
        for (i = 0, y = 0; y < height; y += ghack_glyph_height()) {
            for (x = 0; x < width; x += ghack_glyph_width()) {
                ghack_map.overlay[i] =
                    GNOME_CANVAS_IMAGE(gnome_canvas_item_new(
                        myCanvasGroup, gnome_canvas_image_get_type(), "x",
                        (double) x, "y", (double) y, "width",
                        (double) petmark->rgb_width, "height",
                        (double) petmark->rgb_height, "image", petmark,
                        "anchor", GTK_ANCHOR_NORTH_WEST, NULL));
                gnome_canvas_item_lower_to_bottom(
                    GNOME_CANVAS_ITEM(ghack_map.overlay[i++]));
            }
        }
    }

    ghack_map_cliparound(NULL, u.ux, u.uy, NULL);
    ghack_map_cursor_to(NULL, u.ux, u.uy, NULL);
    gnome_canvas_update_now(ghack_map.canvas);
    doredraw();
}

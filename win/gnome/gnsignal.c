/* NetHack 3.6	gnsignal.c	$NHDT-Date: 1432512805 2015/05/25 00:13:25 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $ */
/* Copyright (C) 1998 by Anthony Taylor <tonyt@ptialaska.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "gnsignal.h"
#include "gnmain.h"
#include <gdk/gdkkeysyms.h>

GList *g_keyBuffer;
GList *g_clickBuffer;
int g_numKeys = 0;
int g_numClicks = 0;
int g_askingQuestion = 0;
static int s_done = FALSE;

/*
 * ghack_init_signals
 *
 * Create some signals and attach them to the GtkWidget class.
 * These are the signals:
 *
 *	ghack_curs        : NONE:INT,INT
 *                            INT 1 = x
 *                            INT 2 = y
 *
 *	ghack_putstr      : NONE:INT,POINTER
 *                            INT     = attribute
 *                            POINTER = char* string to print
 *
 *	ghack_print_glyph : NONE:INT,INT,POINTER
 *                            INT 1 = x
 *                            INT 2 = y
 *                            INT 3 = GtkPixmap* to rendered glyph
 *
 *	ghack_clear       : NONE:NONE
 *
 *	ghack_display     : NONE:BOOL
 *                            BOOL  = blocking flag
 *
 *	ghack_start_menu  : NONE:NONE
 *
 *	ghack_add_menu    : NONE:POINTER
 *                            POINTER = GHackMenuItem*
 *
 *	ghack_end_menu    : NONE:POINTER
 *                            POINTER = char* to closing string
 *
 *	ghack_select_menu : NONE:POINTER,INT,POINTER
 *                            POINTER    = int pointer-- filled with number
 *                                         of selected items on return
 *                            INT        = number of items selected
 *                            POINTER    = structure to fill
 *
 *	ghack_cliparound  : NONE:INT,INT
 *                            INT 1 = x
 *                            INT 2 = y
*/

void
ghack_init_signals(void)
{
    ghack_signals[GHSIG_CURS] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "ghack_curs", GTK_RUN_FIRST,
        gtk_marshal_NONE__INT_INT, GTK_TYPE_NONE, 2, GTK_TYPE_INT,
        GTK_TYPE_INT);

    ghack_signals[GHSIG_PUTSTR] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "ghack_putstr", GTK_RUN_FIRST,
        gtk_marshal_NONE__INT_POINTER, GTK_TYPE_NONE, 2, GTK_TYPE_INT,
        GTK_TYPE_POINTER);

    ghack_signals[GHSIG_PRINT_GLYPH] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "ghack_print_glyph",
        GTK_RUN_FIRST, gtk_marshal_NONE__INT_INT_POINTER, GTK_TYPE_NONE, 3,
        GTK_TYPE_INT, GTK_TYPE_INT, GTK_TYPE_POINTER);

    ghack_signals[GHSIG_CLEAR] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "ghack_clear", GTK_RUN_FIRST,
        gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);

    ghack_signals[GHSIG_DISPLAY] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "ghack_display", GTK_RUN_FIRST,
        gtk_marshal_NONE__BOOL, GTK_TYPE_NONE, 1, GTK_TYPE_BOOL);

    ghack_signals[GHSIG_START_MENU] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "ghack_start_menu",
        GTK_RUN_FIRST, gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);

    ghack_signals[GHSIG_ADD_MENU] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "ghack_add_menu",
        GTK_RUN_FIRST, gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1,
        GTK_TYPE_POINTER);

    ghack_signals[GHSIG_END_MENU] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "ghack_end_menu",
        GTK_RUN_FIRST, gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1,
        GTK_TYPE_POINTER);

    ghack_signals[GHSIG_SELECT_MENU] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "ghack_select_menu",
        GTK_RUN_FIRST, gtk_marshal_NONE__POINTER_INT_POINTER, GTK_TYPE_NONE,
        3, GTK_TYPE_POINTER, GTK_TYPE_INT, GTK_TYPE_POINTER);

    ghack_signals[GHSIG_CLIPAROUND] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "ghack_cliparound",
        GTK_RUN_FIRST, gtk_marshal_NONE__INT_INT, GTK_TYPE_NONE, 2,
        GTK_TYPE_INT, GTK_TYPE_INT);

    ghack_signals[GHSIG_FADE_HIGHLIGHT] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "ghack_fade_highlight",
        GTK_RUN_FIRST, gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);

    ghack_signals[GHSIG_DELAY] = gtk_object_class_user_signal_new(
        gtk_type_class(gtk_widget_get_type()), "gnome_delay_output",
        GTK_RUN_FIRST, gtk_marshal_NONE__INT, GTK_TYPE_NONE, 1, GTK_TYPE_INT);
}

/* For want of a better place, I'm putting the delay output stuff here
 *   -Erik
 */
static gint
timeout_callback(gpointer data)
{
    s_done = TRUE;
    return FALSE;
}

void
ghack_delay(GtkWidget *win, int numMillisecs, gpointer data)
{
    s_done = FALSE;
    gtk_timeout_add((unsigned int) numMillisecs, timeout_callback, NULL);
    while (s_done == FALSE)
        gtk_main_iteration();
}

void
ghack_handle_button_press(GtkWidget *widget, GdkEventButton *event,
                          gpointer data)
{
    GHClick *click;
    double x1, y1;

    if (event->type != GDK_BUTTON_PRESS)
        return;

    gnome_canvas_window_to_world(GNOME_CANVAS(widget), event->x, event->y,
                                 &x1, &y1);
    /*
        g_message("I got a click at %f,%f with button %d \n",
                x1, y1, event->button);
    */

    /* We allocate storage here, so we need to remember if (g_numClicks>0)
     * to blow this away when closing the app using something like
     *  while (g_clickBuffer)
     *       {
     *		g_free((GHClick)g_clickBuffer->data);
     *          g_clickBuffer = g_clickBuffer->next;
     *       }
     *  g_list_free( g_clickBuffer );
     *
     */
    click = g_new(GHClick, 1);

    click->x = (int) x1 / ghack_glyph_width();
    click->y = (int) y1 / ghack_glyph_height();
    click->mod = (event->button == 1) ? CLICK_1 : CLICK_2;

    g_clickBuffer = g_list_prepend(g_clickBuffer, click);
    /* Could use g_list_length(), but it is stupid and just
     * traverses the list while counting, so we'll just do
     * the counting ourselves in advance. */
    g_numClicks++;
}

#ifndef M
#ifndef NHSTDC
#define M(c) (0x80 | (c))
#else
#define M(c) ((c) -128)
#endif /* NHSTDC */
#endif
#ifndef C
#define C(c) (0x1f & (c))
#endif

void
ghack_handle_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    static int was_pound = 0;
    int key = 0;
    int ctl = GDK_CONTROL_MASK;
    int alt = GDK_MOD1_MASK;

/* Turn this on to debug key events */
#if 0
    g_message("I got a \"%s\" key (%d) %s%s", 
	      gdk_keyval_name (event->keyval), event->keyval, 
	      (event->state&ctl)? "+CONTROL":"", (event->state&alt)? "+ALT":"");
#endif

    switch (event->keyval) {
    /* special keys to do stuff with */

    /* Set up the direction keys */

    /* First handle the arrow keys -- these always mean move */
    case GDK_Right:
    case GDK_rightarrow:
        key = Cmd.move_E;
        break;
    case GDK_Left:
    case GDK_leftarrow:
        key = Cmd.move_W;
        break;
    case GDK_Up:
    case GDK_uparrow:
        key = Cmd.move_N;
        break;
    case GDK_Down:
    case GDK_downarrow:
        key = Cmd.move_S;
        break;
    case GDK_Home:
        key = Cmd.move_NW;
        break;
    case GDK_End:
        key = Cmd.move_SW;
        break;
    case GDK_Page_Down:
        key = Cmd.move_SE;
        break;
    case GDK_Page_Up:
        key = Cmd.move_NE;
        break;
    case ' ':
        key = '.';
        break;

    /* Now, handle the numberpad (move or numbers) */
    case GDK_KP_Right:
    case GDK_KP_6:
        if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
            && iflags.num_pad)
            key = GDK_KP_6;
        else
            key = '6';
        break;

    case GDK_KP_Left:
    case GDK_KP_4:
        if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
            && iflags.num_pad)
            key = GDK_KP_4;
        else
            key = '4';
        break;

    case GDK_KP_Up:
    case GDK_KP_8:
        if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
            && iflags.num_pad)
            key = GDK_KP_8;
        else
            key = '8';
        break;

    case GDK_KP_Down:
    case GDK_KP_2:
        if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
            && iflags.num_pad)
            key = GDK_KP_2;
        else
            key = '2';
        break;

    /* Move Top-Left */
    case GDK_KP_Home:
    case GDK_KP_7:
        if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
            && iflags.num_pad)
            key = GDK_KP_7;
        else
            key = '7';
        break;

    case GDK_KP_Page_Up:
    case GDK_KP_9:
        if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
            && iflags.num_pad)
            key = GDK_KP_9;
        else
            key = '9';
        break;

    case GDK_KP_End:
    case GDK_KP_1:
        if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
            && iflags.num_pad)
            key = GDK_KP_1;
        else
            key = '1';
        break;

    case GDK_KP_Page_Down:
    case GDK_KP_3:
        if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
            && iflags.num_pad)
            key = GDK_KP_3;
        else
            key = '3';
        break;

    case GDK_KP_5:
        if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
            && iflags.num_pad)
            key = GDK_KP_5;
        else
            key = '5';
        break;

    case GDK_KP_Delete:
    case GDK_KP_Decimal:
        key = '.';
        break;

    /* can't just ignore "#", it's a core feature */
    case GDK_numbersign:
        key = '#';
        break;

    /* We will probably want to do something with these later... */
    case GDK_KP_Begin:
    case GDK_KP_F1:
    case GDK_F1:
    case GDK_KP_F2:
    case GDK_F2:
    case GDK_KP_F3:
    case GDK_F3:
    case GDK_KP_F4:
    case GDK_F4:
    case GDK_F5:
    case GDK_F6:
    case GDK_F7:
    case GDK_F8:
    case GDK_F9:
    case GDK_F10:
    case GDK_F11:
    case GDK_F12:
        break;
    /* various keys to ignore */
    case GDK_KP_Insert:
    case GDK_Insert:
    case GDK_Delete:
    case GDK_Print:
    case GDK_BackSpace:
    case GDK_Pause:
    case GDK_Scroll_Lock:
    case GDK_Shift_Lock:
    case GDK_Num_Lock:
    case GDK_Caps_Lock:
    case GDK_Control_L:
    case GDK_Control_R:
    case GDK_Shift_L:
    case GDK_Shift_R:
    case GDK_Alt_L:
    case GDK_Alt_R:
    case GDK_Meta_L:
    case GDK_Meta_R:
    case GDK_Mode_switch:
    case GDK_Multi_key:
        return;

    default:
        key = event->keyval;
        break;
    }

    if ((event->state & alt) || was_pound) {
        key = M(event->keyval);
    } else if (event->state & ctl) {
        key = C(event->keyval);
    }
    if (was_pound) {
        was_pound = 0;
    }

    /* Ok, here is where we do clever stuff to overide the default
     * game behavior */
    if (g_askingQuestion == 0) {
        if (key == 'S' || key == M('S') || key == C('S')) {
            ghack_save_game_cb(NULL, NULL);
            return;
        }
    }
    g_keyBuffer = g_list_prepend(g_keyBuffer, GINT_TO_POINTER(key));
    g_numKeys++;
}

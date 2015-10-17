/* NetHack 3.6	gnbind.h	$NHDT-Date: 1432512806 2015/05/25 00:13:26 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackBind_h
#define GnomeHackBind_h

/*
 * This header files defines the interface between the window port specific
 * code in the Gnome port and the rest of the nethack game engine.
*/

#include <gnome.h>
#include <gdk/gdkkeysyms.h>

#include "gnomeprv.h"
#include "gnmain.h"
#include "gnmap.h"
#include "gnmenu.h"
#include "gnplayer.h"
#include "gnsignal.h"
#include "gnstatus.h"
#include "gntext.h"
#include "gnmesg.h"
#include "gnyesno.h"
#include "gnglyph.h"
#include "gnworn.h"

/* Create an array to keep track of the various windows */

#ifndef MAXWINDOWS
#define MAXWINDOWS 15
#endif

typedef struct gnome_nhwindow_data {
    GtkWidget *win;
    int type;
} GNHWinData;

/* Some prototypes */
void gnome_init_nhwindows(int *argc, char **argv);
void gnome_player_selection(void);
void gnome_askname(void);
void gnome_get_nh_event(void);
void gnome_exit_nhwindows(const char *);
void gnome_suspend_nhwindows(const char *);
void gnome_resume_nhwindows(void);
winid gnome_create_nhwindow(int type);
void gnome_create_nhwindow_by_id(int type, winid i);
void gnome_clear_nhwindow(winid wid);
void gnome_display_nhwindow(winid wid, BOOLEAN_P block);
void gnome_destroy_nhwindow(winid wid);
void gnome_curs(winid wid, int x, int y);
void gnome_putstr(winid wid, int attr, const char *text);
void gnome_display_file(const char *filename, BOOLEAN_P must_exist);
void gnome_start_menu(winid wid);
void gnome_add_menu(winid wid, int glyph, const ANY_P *identifier,
                    CHAR_P accelerator, CHAR_P group_accel, int attr,
                    const char *str, BOOLEAN_P presel);
void gnome_end_menu(winid wid, const char *prompt);
int gnome_select_menu(winid wid, int how, MENU_ITEM_P **selected);
/* No need for message_menu -- we'll use genl_message_menu instead */
void gnome_update_inventory(void);
void gnome_mark_synch(void);
void gnome_wait_synch(void);
void gnome_cliparound(int x, int y);
/* The following function does the right thing.  The nethack
 * gnome_cliparound (which lacks the winid) simply calls this function.
*/
void gnome_cliparound_proper(winid wid, int x, int y);
void gnome_print_glyph(winid wid, XCHAR_P x, XCHAR_P y, int glyph);
void gnome_raw_print(const char *str);
void gnome_raw_print_bold(const char *str);
int gnome_nhgetch(void);
int gnome_nh_poskey(int *x, int *y, int *mod);
void gnome_nhbell(void);
int gnome_doprev_message(void);
char gnome_yn_function(const char *question, const char *choices, CHAR_P def);
void gnome_getlin(const char *question, char *input);
int gnome_get_ext_cmd(void);
void gnome_number_pad(int state);
void gnome_delay_output(void);
void gnome_start_screen(void);
void gnome_end_screen(void);
void gnome_outrip(winid wid, int how, time_t when);
void gnome_delete_nhwindow_by_reference(GtkWidget *menuWin);

#endif /* GnomeHackBind_h */

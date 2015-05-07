/* NetHack 3.6	gnmenu.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	gnmenu.h	$Date: 2009/05/06 10:57:39 $  $Revision: 1.5 $ */
/*	SCCS Id: @(#)gnmenu.h	3.5	2000/07/16	*/
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackMenuWindow_h
#define GnomeHackMenuWindow_h

#include <gnome.h>
#include "config.h"
#include "global.h"
#include "gnomeprv.h"

GtkWidget* ghack_init_menu_window( void );

struct _GHackMenuItem
{
  int		glyph; 
  const ANY_P *identifier;
  CHAR_P       accelerator;
  CHAR_P       group_accel;
  int          attr;
  const char*  str;
  BOOLEAN_P    presel;
};

typedef struct _GHackMenuItem GHackMenuItem;

int ghack_menu_window_select_menu (GtkWidget *menuWin, 
	MENU_ITEM_P **_selected, gint how);
int ghack_menu_ext_cmd(void);

#endif  /* GnomeHackMenuWindow_h */

/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.7 cursdial.h */
/* Copyright (c) Karl Garrison, 2010. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef CURSDIAL_H
# define CURSDIAL_H

/* Global declarations */

void curses_line_input_dialog(const char *prompt, char *answer, int buffer);
int curses_character_input_dialog(const char *prompt, const char *choices,
                                  char def);
int curses_ext_cmd(void);
void curses_create_nhmenu(winid wid, unsigned long);
void curses_add_nhmenu_item(winid wid, const glyph_info *glyphinfo,
                            const ANY_P *identifier, char accelerator,
                            char group_accel, int attr,
                            const char *str, unsigned itemflags);
void curs_menu_set_bottom_heavy(winid);
void curses_finalize_nhmenu(winid wid, const char *prompt);
int curses_display_nhmenu(winid wid, int how, MENU_ITEM_P **_selected);
boolean curses_menu_exists(winid wid);
void curses_del_menu(winid, boolean);
boolean curs_nonselect_menu_action(WINDOW *win, void *menu, int how,
                                   int curletter, int *curpage_p,
                                   char selectors[256], int *num_selected_p);

#endif /* CURSDIAL_H */

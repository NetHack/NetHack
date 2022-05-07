/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.7 curswins.h */
/* Copyright (c) Karl Garrison, 2010. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef CURSWIN_H
# define CURSWIN_H


/* Global declarations */

WINDOW *curses_create_window(int width, int height, orient orientation);

void curses_destroy_win(WINDOW * win);
void curses_refresh_nethack_windows(void);
WINDOW *curses_get_nhwin(winid wid);
void curses_add_nhwin(winid wid, int height, int width, int y, int x,
                      orient orientation, boolean border);
void curses_add_wid(winid wid);
void curses_refresh_nhwin(winid wid);
void curses_del_nhwin(winid wid);
void curses_del_wid(winid wid);
void curs_destroy_all_wins(void);
#ifdef ENHANCED_SYMBOLS
void curses_putch(winid wid, int x, int y, int ch,
                  struct unicode_representation *ur, int color, int attrs);
#else
void curses_putch(winid wid, int x, int y, int ch, int color, int attrs);
#endif
void curses_get_window_xy(winid wid, int *x, int *y);
boolean curses_window_has_border(winid wid);
boolean curses_window_exists(winid wid);
int curses_get_window_orientation(winid wid);
void curses_puts(winid wid, int attr, const char *text);
void curses_clear_nhwin(winid wid);
void curses_alert_win_border(winid wid, boolean onoff);
void curses_alert_main_borders(boolean onoff);
void curses_draw_map(int sx, int sy, int ex, int ey);
boolean curses_map_borders(int *sx, int *sy, int *ex, int *ey, int ux, int uy);


#endif /* CURSWIN_H */

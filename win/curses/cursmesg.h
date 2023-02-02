/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.7 cursmesg.h */
/* Copyright (c) Karl Garrison, 2010. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef CURSMESG_H
# define CURSMESG_H


/* Global declarations */

void curses_message_win_puts(const char *message, boolean recursed);
void curses_got_input(void);
int curses_got_output(void);
int curses_block(boolean require_tab);
int curses_more(void);
void curses_clear_unhighlight_message_window(void);
void curses_message_win_getline(const char *prompt, char *answer, int buffer);
void curses_last_messages(void);
void curses_init_mesg_history(void);
void curses_prev_mesg(void);
void curses_count_window(const char *count_text);
char *curses_getmsghistory(boolean);
void curses_putmsghistory(const char *, boolean);

#endif /* CURSMESG_H */

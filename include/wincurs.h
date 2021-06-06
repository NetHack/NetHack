/* NetHack 3.7 wincurs.h */
/* Copyright (c) Karl Garrison, 2010. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINCURS_H
#define WINCURS_H

/* Global declarations for curses interface */

extern int term_rows, term_cols;   /* size of underlying terminal   */
extern int orig_cursor;            /* Preserve initial cursor state */
extern WINDOW *base_term;          /* underlying terminal window    */
extern boolean counting;           /* Count window is active        */
extern WINDOW *mapwin, *statuswin, *messagewin;    /* Main windows  */
extern WINDOW *activemenu;         /* curses window for menu requesting a
                                    * count; affects count_window refresh */

#define TEXTCOLOR   /* Allow color */
#define NHW_END 19
#define OFF 0
#define ON 1
#define NONE -1
#define KEY_ESC 0x1b
#define DIALOG_BORDER_COLOR CLR_MAGENTA
#define ALERT_BORDER_COLOR CLR_RED
#define SCROLLBAR_COLOR CLR_MAGENTA
#define SCROLLBAR_BACK_COLOR CLR_BLACK
#define HIGHLIGHT_COLOR CLR_WHITE
#define MORECOLOR CLR_ORANGE
#define STAT_UP_COLOR CLR_GREEN
#define STAT_DOWN_COLOR CLR_RED
#define MESSAGE_WIN 1
#define STATUS_WIN  2
#define MAP_WIN     3
#define INV_WIN     4
#define NHWIN_MAX   5
#define MESG_HISTORY_MAX   200
#if !defined(__APPLE__) || !defined(NCURSES_VERSION)
# define USE_DARKGRAY /* Allow "bright" black; delete if not visible */
#endif  /* !__APPLE__ && !PDCURSES */
#define CURSES_DARK_GRAY    17
#define MAP_SCROLLBARS

#if !defined(A_LEFTLINE) && defined(A_LEFT)
#define A_LEFTLINE A_LEFT
#endif
#if !defined(A_RIGHTLINE) && defined(A_RIGHT)
#define A_RIGHTLINE A_RIGHT
#endif

typedef enum orient_type
{
    CENTER,
    UP,
    DOWN,
    RIGHT,
    LEFT,
    UNDEFINED
} orient;

#ifdef GCC_WARN
int wprintw(WINDOW *, const char *, ...) PRINTF_F(2, 3);
int mvwprintw(WINDOW *, int, int, const char *, ...) PRINTF_F(4, 5);
#endif

/* cursmain.c */

extern struct window_procs curses_procs;

extern void curses_init_nhwindows(int* argcp, char** argv);
extern void curses_player_selection(void);
extern void curses_askname(void);
extern void curses_get_nh_event(void);
extern void curses_uncurse_terminal(void);
extern void curses_exit_nhwindows(const char *str);
extern void curses_suspend_nhwindows(const char *str);
extern void curses_resume_nhwindows(void);
extern winid curses_create_nhwindow(int type);
extern void curses_clear_nhwindow(winid wid);
extern void curses_display_nhwindow(winid wid, boolean block);
extern void curses_destroy_nhwindow(winid wid);
extern void curses_curs(winid wid, int x, int y);
extern void curses_putstr(winid wid, int attr, const char *text);
extern void curses_display_file(const char *filename, boolean must_exist);
extern void curses_start_menu(winid wid, unsigned long);
extern void curses_add_menu(winid wid, const glyph_info *,
                            const ANY_P * identifier,
                            char accelerator, char group_accel, int attr,
                            const char *str, unsigned int itemflags);
extern void curses_end_menu(winid wid, const char *prompt);
extern int curses_select_menu(winid wid, int how, MENU_ITEM_P **selected);
extern void curses_update_inventory(int);
extern void curses_mark_synch(void);
extern void curses_wait_synch(void);
extern void curses_cliparound(int x, int y);
extern void curses_print_glyph(winid wid, xchar x, xchar y,
                                const glyph_info *, const glyph_info *);
extern void curses_raw_print(const char *str);
extern void curses_raw_print_bold(const char *str);
extern int curses_nhgetch(void);
extern int curses_nh_poskey(int *x, int *y, int *mod);
extern void curses_nhbell(void);
extern int curses_doprev_message(void);
extern char curses_yn_function(const char *question, const char *choices,
                               char def);
extern void curses_getlin(const char *question, char *input);
extern int curses_get_ext_cmd(void);
extern void curses_number_pad(int state);
extern void curses_delay_output(void);
extern void curses_start_screen(void);
extern void curses_end_screen(void);
extern void curses_outrip(winid wid, int how, time_t when);
extern void genl_outrip(winid tmpwin, int how, time_t when);
extern void curses_preference_update(const char *pref);
extern void curs_reset_windows(boolean, boolean);

/* curswins.c */

extern WINDOW *curses_create_window(int width, int height, orient orientation);
extern void curses_destroy_win(WINDOW *win);
extern WINDOW *curses_get_nhwin(winid wid);
extern void curses_add_nhwin(winid wid, int height, int width, int y,
                             int x, orient orientation, boolean border);
extern void curses_add_wid(winid wid);
extern void curses_refresh_nhwin(winid wid);
extern void curses_refresh_nethack_windows(void);
extern void curses_del_nhwin(winid wid);
extern void curses_del_wid(winid wid);
extern void curs_destroy_all_wins(void);
extern void curses_putch(winid wid, int x, int y, int ch,
                         int color, int attrs);
extern void curses_get_window_size(winid wid, int *height, int *width);
extern boolean curses_window_has_border(winid wid);
extern boolean curses_window_exists(winid wid);
extern int curses_get_window_orientation(winid wid);
extern void curses_get_window_xy(winid wid, int *x, int *y);
extern void curses_puts(winid wid, int attr, const char *text);
extern void curses_clear_nhwin(winid wid);
extern void curses_alert_win_border(winid wid, boolean onoff);
extern void curses_alert_main_borders(boolean onoff);
extern void curses_draw_map(int sx, int sy, int ex, int ey);
extern boolean curses_map_borders(int *sx, int *sy, int *ex, int *ey,
                                  int ux, int uy);

/* cursmisc.c */

extern int curses_read_char(void);
extern void curses_toggle_color_attr(WINDOW *win, int color, int attr,
                                     int onoff);
extern void curses_menu_color_attr(WINDOW *, int, int, int);
extern void curses_bail(const char *mesg);
extern winid curses_get_wid(int type);
extern char *curses_copy_of(const char *s);
extern int curses_num_lines(const char *str, int width);
extern char *curses_break_str(const char *str, int width, int line_num);
extern char *curses_str_remainder(const char *str, int width, int line_num);
extern boolean curses_is_menu(winid wid);
extern boolean curses_is_text(winid wid);
extern int curses_convert_glyph(int ch, int glyph);
extern void curses_move_cursor(winid wid, int x, int y);
extern void curses_prehousekeeping(void);
extern void curses_posthousekeeping(void);
extern void curses_view_file(const char *filename, boolean must_exist);
extern void curses_rtrim(char *str);
extern long curses_get_count(int first_digit);
extern int curses_convert_attr(int attr);
extern int curses_read_attrs(const char *attrs);
extern char *curses_fmt_attrs(char *);
extern int curses_convert_keys(int key);
extern int curses_get_mouse(int *mousex, int *mousey, int *mod);
extern void curses_mouse_support(int);

/* cursdial.c */

extern void curses_line_input_dialog(const char *prompt,
                                     char *answer, int buffer);
extern int curses_character_input_dialog(const char *prompt,
                                         const char *choices, char def);
extern int curses_ext_cmd(void);
extern void curses_create_nhmenu(winid wid, unsigned long);
extern void curses_add_nhmenu_item(winid wid, const glyph_info *,
                                   const ANY_P *identifier, char accelerator,
                                   char group_accel, int attr,
                                   const char *str, unsigned itemflags);
extern void curs_menu_set_bottom_heavy(winid);
extern void curses_finalize_nhmenu(winid wid, const char *prompt);
extern int curses_display_nhmenu(winid wid, int how, MENU_ITEM_P **_selected);
extern boolean curses_menu_exists(winid wid);
extern void curses_del_menu(winid, boolean);

/* cursstat.c */

extern void curses_status_init(void);
extern void curses_status_finish(void);
extern void curses_status_update(int, genericptr_t, int, int, int,
                                 unsigned long *);

/* cursinvt.c */

extern void curs_purge_perminv_data(boolean);
extern void curs_update_invt(int);
extern void curs_add_invt(int, char, attr_t, const char *);

/* cursinit.c */

extern void curses_create_main_windows(void);
extern void curses_init_nhcolors(void);
extern void curses_choose_character(void);
extern int curses_character_dialog(const char **choices, const char *prompt);
extern void curses_init_options(void);
extern void curses_display_splash_window(void);
extern void curses_cleanup(void);

/* cursmesg.c */

extern void curses_message_win_puts(const char *message, boolean recursed);
extern void curses_got_input(void);
extern int curses_block(boolean require_tab); /* for MSGTYPE=STOP */
extern int curses_more(void);
extern void curses_clear_unhighlight_message_window(void);
extern void curses_message_win_getline(const char *prompt,
                                       char *answer, int buffer);
extern void curses_last_messages(void);
extern void curses_init_mesg_history(void);
extern void curses_teardown_messages(void);
extern void curses_prev_mesg(void);
extern void curses_count_window(const char *count_text);
char *curses_getmsghistory(boolean);
void curses_putmsghistory(const char *, boolean);

#endif  /* WINCURS_H */


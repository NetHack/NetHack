/* NetHack 3.6	wingem.h	$NHDT-Date: 1433806582 2015/06/08 23:36:22 $  $NHDT-Branch: master $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Christian Bressler, 1999				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINGEM_H
#define WINGEM_H

#define E extern

/* menu structure */
typedef struct Gmi {
    struct Gmi *Gmi_next;
    int Gmi_glyph;
    long Gmi_identifier;
    char Gmi_accelerator, Gmi_groupacc;
    int Gmi_attr;
    char *Gmi_str;
    long Gmi_count;
    int Gmi_selected;
} Gem_menu_item;

#define MAXWIN 20 /* maximum number of windows, cop-out */

extern struct window_procs Gem_procs;

/* ### wingem1.c ### */
#ifdef CLIPPING
E void setclipped(void);
#endif
E void docorner(int, int);
E void end_glyphout(void);
E void g_putch(int);
E void win_Gem_init(int);
E int mar_gem_init(void);
E char mar_ask_class(void);
E char *mar_ask_name(void);
E int mar_create_window(int);
E void mar_destroy_nhwindow(int);
E void mar_print_glyph(int, int, int, int, int);
E void mar_print_line(int, int, int, char *);
E void mar_set_message(char *, char *, char *);
E Gem_menu_item *mar_hol_inv(void);
E void mar_set_menu_type(int);
E void mar_reverse_menu(void);
E void mar_set_menu_title(const char *);
E void mar_set_accelerators(void);
E void mar_add_menu(winid, Gem_menu_item *);
E void mar_change_menu_2_text(winid);
E void mar_add_message(const char *);
E void mar_status_dirty(void);
E int mar_hol_win_type(int);
E void mar_clear_messagewin(void);
E void mar_set_no_glyph(int);
E void mar_map_curs_weiter(void);

/* external declarations */
E void Gem_init_nhwindows(int *, char **);
E void Gem_player_selection(void);
E void Gem_askname(void);
E void Gem_get_nh_event(void);
E void Gem_exit_nhwindows(const char *);
E void Gem_suspend_nhwindows(const char *);
E void Gem_resume_nhwindows(void);
E winid Gem_create_nhwindow(int);
E void Gem_clear_nhwindow(winid);
E void Gem_display_nhwindow(winid, boolean);
E void Gem_dismiss_nhwindow(winid);
E void Gem_destroy_nhwindow(winid);
E void Gem_curs(winid, int, int);
E void Gem_putstr(winid, int, const char *);
E void Gem_display_file(const char *, boolean);
E void Gem_start_menu(winid);
E void Gem_add_menu(winid, int, const ANY_P *, char, char, int,
                            const char *, boolean);
E void Gem_end_menu(winid, const char *);
E int Gem_select_menu(winid, int, MENU_ITEM_P **);
E char Gem_message_menu(char, int, const char *);
E void Gem_update_inventory(void);
E void Gem_mark_synch(void);
E void Gem_wait_synch(void);
#ifdef CLIPPING
E void Gem_cliparound(int, int);
#endif
#ifdef POSITIONBAR
E void Gem_update_positionbar(char *);
#endif
E void Gem_print_glyph(winid, xchar, xchar, int, int);
E void Gem_raw_print(const char *);
E void Gem_raw_print_bold(const char *);
E int Gem_nhgetch(void);
E int Gem_nh_poskey(int *, int *, int *);
E void Gem_nhbell(void);
E int Gem_doprev_message(void);
E char Gem_yn_function(const char *, const char *, char);
E void Gem_getlin(const char *, char *);
E int Gem_get_ext_cmd(void);
E void Gem_number_pad(int);
E void Gem_delay_output(void);
#ifdef CHANGE_COLOR
E void Gem_change_color(int color, long rgb, int reverse);
E char *Gem_get_color_string(void);
#endif

/* other defs that really should go away (they're tty specific) */
E void Gem_start_screen(void);
E void Gem_end_screen(void);

E void genl_outrip(winid, int, time_t);

#undef E

#endif /* WINGEM_H */

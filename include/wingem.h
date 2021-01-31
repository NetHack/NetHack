/* NetHack 3.7	wingem.h	$NHDT-Date: 1596498570 2020/08/03 23:49:30 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.15 $ */
/* Copyright (c) Christian Bressler, 1999				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINGEM_H
#define WINGEM_H

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
extern void setclipped(void);
#endif
extern void docorner(int, int);
extern void end_glyphout(void);
extern void g_putch(int);
extern void win_Gem_init(int);
extern int mar_gem_init(void);
extern char mar_ask_class(void);
extern char *mar_ask_name(void);
extern int mar_create_window(int);
extern void mar_destroy_nhwindow(int);
extern void mar_print_glyph(int, int, int, const glyph_info *,
                       const glyph_info *);
extern void mar_print_line(int, int, int, char *);
extern void mar_set_message(char *, char *, char *);
extern Gem_menu_item *mar_hol_inv(void);
extern void mar_set_menu_type(int);
extern void mar_reverse_menu(void);
extern void mar_set_menu_title(const char *);
extern void mar_set_accelerators(void);
extern void mar_add_menu(winid, Gem_menu_item *);
extern void mar_change_menu_2_text(winid);
extern void mar_add_message(const char *);
extern void mar_status_dirty(void);
extern int mar_hol_win_type(int);
extern void mar_clear_messagewin(void);
extern void mar_set_no_glyph(int);
extern void mar_map_curs_weiter(void);

/* external declarations */
extern void Gem_init_nhwindows(int *, char **);
extern void Gem_player_selection(void);
extern void Gem_askname(void);
extern void Gem_get_nh_event(void);
extern void Gem_exit_nhwindows(const char *);
extern void Gem_suspend_nhwindows(const char *);
extern void Gem_resume_nhwindows(void);
extern winid Gem_create_nhwindow(int);
extern void Gem_clear_nhwindow(winid);
extern void Gem_display_nhwindow(winid, boolean);
extern void Gem_dismiss_nhwindow(winid);
extern void Gem_destroy_nhwindow(winid);
extern void Gem_curs(winid, int, int);
extern void Gem_putstr(winid, int, const char *);
extern void Gem_display_file(const char *, boolean);
extern void Gem_start_menu(winid, unsigned long);
extern void Gem_add_menu(winid, const glyph_info *, const ANY_P *, char, char,
                    int, const char *, unsigned int);
extern void Gem_end_menu(winid, const char *);
extern int Gem_select_menu(winid, int, MENU_ITEM_P **);
extern char Gem_message_menu(char, int, const char *);
extern void Gem_update_inventory(void);
extern void Gem_mark_synch(void);
extern void Gem_wait_synch(void);
#ifdef CLIPPING
extern void Gem_cliparound(int, int);
#endif
#ifdef POSITIONBAR
extern void Gem_update_positionbar(char *);
#endif
extern void Gem_print_glyph(winid, xchar, xchar, const glyph_info *,
                       const glyph_info *);
extern void Gem_raw_print(const char *);
extern void Gem_raw_print_bold(const char *);
extern int Gem_nhgetch(void);
extern int Gem_nh_poskey(int *, int *, int *);
extern void Gem_nhbell(void);
extern int Gem_doprev_message(void);
extern char Gem_yn_function(const char *, const char *, char);
extern void Gem_getlin(const char *, char *);
extern int Gem_get_ext_cmd(void);
extern void Gem_number_pad(int);
extern void Gem_delay_output(void);
#ifdef CHANGE_COLOR
extern void Gem_change_color(int color, long rgb, int reverse);
extern char *Gem_get_color_string(void);
#endif

/* other defs that really should go away (they're tty specific) */
extern void Gem_start_screen(void);
extern void Gem_end_screen(void);

extern void genl_outrip(winid, int, time_t);


#endif /* WINGEM_H */

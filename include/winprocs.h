/*	SCCS Id: @(#)winprocs.h 3.3	96/02/18	*/
/* Copyright (c) David Cohrs, 1992				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINPROCS_H
#define WINPROCS_H

struct window_procs {
    const char *name;
    void FDECL((*win_init_nhwindows), (int *, char **));
    void NDECL((*win_player_selection));
    void NDECL((*win_askname));
    void NDECL((*win_get_nh_event)) ;
    void FDECL((*win_exit_nhwindows), (const char *));
    void FDECL((*win_suspend_nhwindows), (const char *));
    void NDECL((*win_resume_nhwindows));
    winid FDECL((*win_create_nhwindow), (int));
    void FDECL((*win_clear_nhwindow), (winid));
    void FDECL((*win_display_nhwindow), (winid, BOOLEAN_P));
    void FDECL((*win_destroy_nhwindow), (winid));
    void FDECL((*win_curs), (winid,int,int));
    void FDECL((*win_putstr), (winid, int, const char *));
    void FDECL((*win_display_file), (const char *, BOOLEAN_P));
    void FDECL((*win_start_menu), (winid));
    void FDECL((*win_add_menu), (winid,int,const ANY_P *,
		CHAR_P,CHAR_P,int,const char *, BOOLEAN_P));
    void FDECL((*win_end_menu), (winid, const char *));
    int FDECL((*win_select_menu), (winid, int, MENU_ITEM_P **));
    char FDECL((*win_message_menu), (CHAR_P,int,const char *));
    void NDECL((*win_update_inventory));
    void NDECL((*win_mark_synch));
    void NDECL((*win_wait_synch));
#ifdef CLIPPING
    void FDECL((*win_cliparound), (int, int));
#endif
#ifdef POSITIONBAR
    void FDECL((*win_update_positionbar), (char *));
#endif
    void FDECL((*win_print_glyph), (winid,XCHAR_P,XCHAR_P,int));
    void FDECL((*win_raw_print), (const char *));
    void FDECL((*win_raw_print_bold), (const char *));
    int NDECL((*win_nhgetch));
    int FDECL((*win_nh_poskey), (int *, int *, int *));
    void NDECL((*win_nhbell));
    int NDECL((*win_doprev_message));
    char FDECL((*win_yn_function), (const char *, const char *, CHAR_P));
    void FDECL((*win_getlin), (const char *,char *));
    int NDECL((*win_get_ext_cmd));
    void FDECL((*win_number_pad), (int));
    void NDECL((*win_delay_output));
#ifdef CHANGE_COLOR
    void FDECL((*win_change_color), (int,long,int));
#ifdef MAC
    void FDECL((*win_change_background), (int));
    short FDECL((*win_set_font_name), (winid, char *));
#endif
    char * NDECL((*win_get_color_string));
#endif

    /* other defs that really should go away (they're tty specific) */
    void NDECL((*win_start_screen));
    void NDECL((*win_end_screen));

    void FDECL((*win_outrip), (winid,int));
};

extern NEARDATA struct window_procs windowprocs;

/*
 * If you wish to only support one window system and not use procedure
 * pointers, add the appropriate #ifdef below.
 */

#define init_nhwindows (*windowprocs.win_init_nhwindows)
#define player_selection (*windowprocs.win_player_selection)
#define askname (*windowprocs.win_askname)
#define get_nh_event (*windowprocs.win_get_nh_event)
#define exit_nhwindows (*windowprocs.win_exit_nhwindows)
#define suspend_nhwindows (*windowprocs.win_suspend_nhwindows)
#define resume_nhwindows (*windowprocs.win_resume_nhwindows)
#define create_nhwindow (*windowprocs.win_create_nhwindow)
#define clear_nhwindow (*windowprocs.win_clear_nhwindow)
#define display_nhwindow (*windowprocs.win_display_nhwindow)
#define destroy_nhwindow (*windowprocs.win_destroy_nhwindow)
#define curs (*windowprocs.win_curs)
#define putstr (*windowprocs.win_putstr)
#define display_file (*windowprocs.win_display_file)
#define start_menu (*windowprocs.win_start_menu)
#define add_menu (*windowprocs.win_add_menu)
#define end_menu (*windowprocs.win_end_menu)
#define select_menu (*windowprocs.win_select_menu)
#define message_menu (*windowprocs.win_message_menu)
#define update_inventory (*windowprocs.win_update_inventory)
#define mark_synch (*windowprocs.win_mark_synch)
#define wait_synch (*windowprocs.win_wait_synch)
#ifdef CLIPPING
#define cliparound (*windowprocs.win_cliparound)
#endif
#ifdef POSITIONBAR
#define update_positionbar (*windowprocs.win_update_positionbar)
#endif
#define print_glyph (*windowprocs.win_print_glyph)
#define raw_print (*windowprocs.win_raw_print)
#define raw_print_bold (*windowprocs.win_raw_print_bold)
#define nhgetch (*windowprocs.win_nhgetch)
#define nh_poskey (*windowprocs.win_nh_poskey)
#define nhbell (*windowprocs.win_nhbell)
#define nh_doprev_message (*windowprocs.win_doprev_message)
#define yn_function (*windowprocs.win_yn_function)
#define getlin (*windowprocs.win_getlin)
#define get_ext_cmd (*windowprocs.win_get_ext_cmd)
#define number_pad (*windowprocs.win_number_pad)
#define delay_output (*windowprocs.win_delay_output)
#ifdef CHANGE_COLOR
#define change_color (*windowprocs.win_change_color)
#ifdef MAC
#define change_background (*windowprocs.win_change_background)
#define set_font_name (*windowprocs.win_set_font_name)
#endif
#define get_color_string (*windowprocs.win_get_color_string)
#endif

/* other defs that really should go away (they're tty specific) */
#define start_screen (*windowprocs.win_start_screen)
#define end_screen (*windowprocs.win_end_screen)

#define outrip (*windowprocs.win_outrip)
#endif

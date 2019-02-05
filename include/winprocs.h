/* NetHack 3.6	winprocs.h	$NHDT-Date: 1549327479 2019/02/05 00:44:39 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.46 $ */
/* Copyright (c) David Cohrs, 1992				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINPROCS_H
#define WINPROCS_H

#include "botl.h"

/* NB: this MUST match chain_procs below */
struct window_procs {
    const char *name;     /* Names should start with [a-z].  Names must
                           * not start with '-'.  Names starting with
                           * '+' are reserved for processors. */
    unsigned long wincap; /* window port capability options supported */
    unsigned long wincap2; /* additional window port capability options */
    void FDECL((*win_init_nhwindows), (int *, char **));
    void NDECL((*win_player_selection));
    void NDECL((*win_askname));
    void NDECL((*win_get_nh_event));
    void FDECL((*win_exit_nhwindows), (const char *));
    void FDECL((*win_suspend_nhwindows), (const char *));
    void NDECL((*win_resume_nhwindows));
    winid FDECL((*win_create_nhwindow), (int));
    void FDECL((*win_clear_nhwindow), (winid));
    void FDECL((*win_display_nhwindow), (winid, BOOLEAN_P));
    void FDECL((*win_destroy_nhwindow), (winid));
    void FDECL((*win_curs), (winid, int, int));
    void FDECL((*win_putstr), (winid, int, const char *));
    void FDECL((*win_putmixed), (winid, int, const char *));
    void FDECL((*win_display_file), (const char *, BOOLEAN_P));
    void FDECL((*win_start_menu), (winid));
    void FDECL((*win_add_menu), (winid, int, const ANY_P *, CHAR_P, CHAR_P,
                                 int, const char *, BOOLEAN_P));
    void FDECL((*win_end_menu), (winid, const char *));
    int FDECL((*win_select_menu), (winid, int, MENU_ITEM_P **));
    char FDECL((*win_message_menu), (CHAR_P, int, const char *));
    void NDECL((*win_update_inventory));
    void NDECL((*win_mark_synch));
    void NDECL((*win_wait_synch));
#ifdef CLIPPING
    void FDECL((*win_cliparound), (int, int));
#endif
#ifdef POSITIONBAR
    void FDECL((*win_update_positionbar), (char *));
#endif
    void FDECL((*win_print_glyph), (winid, XCHAR_P, XCHAR_P, int, int));
    void FDECL((*win_raw_print), (const char *));
    void FDECL((*win_raw_print_bold), (const char *));
    int NDECL((*win_nhgetch));
    int FDECL((*win_nh_poskey), (int *, int *, int *));
    void NDECL((*win_nhbell));
    int NDECL((*win_doprev_message));
    char FDECL((*win_yn_function), (const char *, const char *, CHAR_P));
    void FDECL((*win_getlin), (const char *, char *));
    int NDECL((*win_get_ext_cmd));
    void FDECL((*win_number_pad), (int));
    void NDECL((*win_delay_output));
#ifdef CHANGE_COLOR
    void FDECL((*win_change_color), (int, long, int));
#ifdef MAC
    void FDECL((*win_change_background), (int));
    short FDECL((*win_set_font_name), (winid, char *));
#endif
    char *NDECL((*win_get_color_string));
#endif

    /* other defs that really should go away (they're tty specific) */
    void NDECL((*win_start_screen));
    void NDECL((*win_end_screen));

    void FDECL((*win_outrip), (winid, int, time_t));
    void FDECL((*win_preference_update), (const char *));
    char *FDECL((*win_getmsghistory), (BOOLEAN_P));
    void FDECL((*win_putmsghistory), (const char *, BOOLEAN_P));
    void NDECL((*win_status_init));
    void NDECL((*win_status_finish));
    void FDECL((*win_status_enablefield),
               (int, const char *, const char *, BOOLEAN_P));
    void FDECL((*win_status_update), (int, genericptr_t, int, int, int, unsigned long *));
    boolean NDECL((*win_can_suspend));
};

extern
#ifdef HANGUPHANDLING
    volatile
#endif
    NEARDATA struct window_procs windowprocs;

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
#define putmixed (*windowprocs.win_putmixed)
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

/* 3.4.2: There is a real yn_function() in the core now, which does
 *        some buffer length validation on the parameters prior to
 *        invoking the window port routine. yn_function() is in cmd.c
 */
/* #define yn_function (*windowprocs.win_yn_function) */

/* other defs that really should go away (they're tty specific) */
#define start_screen (*windowprocs.win_start_screen)
#define end_screen (*windowprocs.win_end_screen)

#define outrip (*windowprocs.win_outrip)
#define preference_update (*windowprocs.win_preference_update)
#define getmsghistory (*windowprocs.win_getmsghistory)
#define putmsghistory (*windowprocs.win_putmsghistory)
/* there is a status_initialize() in botl.c,
 * which calls win_status_init() directly; same with status_finish.
 */
#define status_enablefield (*windowprocs.win_status_enablefield)
#define status_update (*windowprocs.win_status_update)

/*
 * WINCAP
 * Window port preference capability bits.
 * Some day this might be better in its own wincap.h file.
 */
/* clang-format off */
#define WC_COLOR         0x00000001L /* 01 Port can display things in color  */
#define WC_HILITE_PET    0x00000002L /* 02 supports hilite pet               */
#define WC_ASCII_MAP     0x00000004L /* 03 supports an ascii map             */
#define WC_TILED_MAP     0x00000008L /* 04 supports a tiled map              */
#define WC_PRELOAD_TILES 0x00000010L /* 05 supports pre-loading tiles        */
#define WC_TILE_WIDTH    0x00000020L /* 06 prefer this width of tile         */
#define WC_TILE_HEIGHT   0x00000040L /* 07 prefer this height of tile        */
#define WC_TILE_FILE     0x00000080L /* 08 alternative tile file name        */
#define WC_INVERSE       0x00000100L /* 09 Port supports inverse video       */
#define WC_ALIGN_MESSAGE 0x00000200L /* 10 supports mesg alignment top|b|l|r */
#define WC_ALIGN_STATUS  0x00000400L /* 11 supports status alignmt top|b|l|r */
#define WC_VARY_MSGCOUNT 0x00000800L /* 12 supports varying message window   */
#define WC_FONT_MAP      0x00001000L /* 13 supports spec of map window font  */
#define WC_FONT_MESSAGE  0x00002000L /* 14 supports spec of message font     */
#define WC_FONT_STATUS   0x00004000L /* 15 supports spec of status font      */
#define WC_FONT_MENU     0x00008000L /* 16 supports spec of menu font        */
#define WC_FONT_TEXT     0x00010000L /* 17 supports spec of text window font */
#define WC_FONTSIZ_MAP   0x00020000L /* 18 supports spec of map font size    */
#define WC_FONTSIZ_MESSAGE 0x040000L /* 19 supports spec of mesg font size   */
#define WC_FONTSIZ_STATUS 0x0080000L /* 20 supports spec of status font size */
#define WC_FONTSIZ_MENU  0x00100000L /* 21 supports spec of menu font size   */
#define WC_FONTSIZ_TEXT  0x00200000L /* 22 supports spec of text font size   */
#define WC_SCROLL_MARGIN 0x00400000L /* 23 supports setting map scroll marg  */
#define WC_SPLASH_SCREEN 0x00800000L /* 24 supports display of splash screen */
#define WC_POPUP_DIALOG  0x01000000L /* 25 supports queries in popup dialogs */
#define WC_SCROLL_AMOUNT 0x02000000L /* 26 scroll this amount at scroll marg */
#define WC_EIGHT_BIT_IN  0x04000000L /* 27 8-bit character input             */
#define WC_PERM_INVENT   0x08000000L /* 28 supports persistent inventory win */
#define WC_MAP_MODE      0x10000000L /* 29 map_mode option                   */
#define WC_WINDOWCOLORS  0x20000000L /* 30 background color for mesg window  */
#define WC_PLAYER_SELECTION 0x40000000L /* 31 supports player selection      */
#ifdef NHSTDC
#define WC_MOUSE_SUPPORT 0x80000000UL /* 32 mouse support                    */
#else
#define WC_MOUSE_SUPPORT 0x80000000L /* 32 mouse support                     */
#endif
                                     /* no free bits */

#define WC2_FULLSCREEN    0x0001L /* 01 display full screen             */
#define WC2_SOFTKEYBOARD  0x0002L /* 02 software keyboard               */
#define WC2_WRAPTEXT      0x0004L /* 03 wrap long lines of text         */
#define WC2_HILITE_STATUS 0x0008L /* 04 hilite fields in status         */
#define WC2_SELECTSAVED   0x0010L /* 05 saved game selection menu       */
#define WC2_DARKGRAY      0x0020L /* 06 use bold black for black glyphs */
#define WC2_HITPOINTBAR   0x0040L /* 07 show bar representing hit points */
#define WC2_FLUSH_STATUS  0x0080L /* 08 call status_update(BL_FLUSH)
                                   *    after updating status window fields */
#define WC2_RESET_STATUS  0x0100L /* 09 call status_update(BL_RESET) to
                                   *    indicate 'draw everything'      */
#define WC2_TERM_SIZE     0x0200L /* 10 support setting terminal size   */
#define WC2_WINDOWBORDERS 0x0400L /* 11 display borders on nh windows   */
#define WC2_PETATTR       0x0800L /* 12 attributes for hilite_pet       */
#define WC2_GUICOLOR      0x1000L /* 13 display colours outside map win */
/* pline() can overload the display attributes argument passed to putstr()
   with one or more flags and at most one of bold/blink/inverse/&c */
#define WC2_URGENT_MESG   0x2000L /* 14 putstr(WIN_MESSAGE) supports urgency
                                   *    via non-display attribute flag  */
#define WC2_SUPPRESS_HIST 0x4000L /* 15 putstr(WIN_MESSAGE) supports history
                                   *    suppression via non-disp attr   */
                                  /* 17 free bits */

#define ALIGN_LEFT   1
#define ALIGN_RIGHT  2
#define ALIGN_TOP    3
#define ALIGN_BOTTOM 4

/* player_selection */
#define VIA_DIALOG  0
#define VIA_PROMPTS 1

/* map_mode settings - deprecated */
#define MAP_MODE_TILES      0
#define MAP_MODE_ASCII4x6   1
#define MAP_MODE_ASCII6x8   2
#define MAP_MODE_ASCII8x8   3
#define MAP_MODE_ASCII16x8  4
#define MAP_MODE_ASCII7x12  5
#define MAP_MODE_ASCII8x12  6
#define MAP_MODE_ASCII16x12 7
#define MAP_MODE_ASCII12x16 8
#define MAP_MODE_ASCII10x18 9
#define MAP_MODE_ASCII_FIT_TO_SCREEN 10
#define MAP_MODE_TILES_FIT_TO_SCREEN 11

#if 0
#define WC_SND_SOUND   0x0001L /* 01 Port has some sound capabilities     */
#define WC_SND_SPEAKER 0x0002L /* 02 Sound supported via built-in speaker */
#define WC_SND_STEREO  0x0004L /* 03 Stereo sound supported               */
#define WC_SND_RAW     0x0008L /* 04 Raw sound supported                  */
#define WC_SND_WAVE    0x0010L /* 05 Wave support                         */
#define WC_SND_MIDI    0x0020L /* 06 Midi support                         */
                               /* 26 free bits */
#endif

struct wc_Opt {
    const char *wc_name;
    unsigned long wc_bit;
};

/* Macro for the currently active Window Port whose function
   pointers have been loaded */
#define WINDOWPORT(wn) \
    (windowprocs.name && !strncmpi((wn), windowprocs.name, strlen((wn))))

/* role selection by player_selection(); this ought to be in the core... */
#define RS_NAME    0
#define RS_ROLE    1
#define RS_RACE    2
#define RS_GENDER  3
#define RS_ALGNMNT 4
#define RS_filter  5
#define RS_menu_arg(x) (ROLE_RANDOM - ((x) + 1)) /* 0..5 -> -3..-8 */

/* Choose_windows() may be called multiple times; these constants tell the
 * init function whether the window system is coming or going. */
#define WININIT      0
#define WININIT_UNDO 1

#ifdef WINCHAIN
/* Setup phases for window chain elements.
        void *  rv = X_procs_chain(int,	int, void *, void *, void *);
                Xprivate*	   ALLOC n	0	0	0
                -		   INIT	 n   self    next    nextdata
   where:
        Xprivate* is anything window chain entry type X wants back
        n is the link count (starting with 1)
        self is the Xprivate* returned earlier
        next is struct winprocs * or struct chainprocs * for the next link
        nextdata is the Xprivate* for the next link in the chain
*/
#define WINCHAIN_ALLOC 0
#define WINCHAIN_INIT  1

/* clang-format on */

#define CARGS void *

extern FILE *wc_tracelogf; /* Expose log file for additional debugging. */

struct chain_procs {
    const char *name;     /* Names should start with [a-z].  Names must
                           * not start with '-'.  Names starting with
                           * '+' are reserved for processors. */
    unsigned long wincap; /* window port capability options supported */
    unsigned long wincap2; /* additional window port capability options */
    void FDECL((*win_init_nhwindows), (CARGS, int *, char **));
    void FDECL((*win_player_selection), (CARGS));
    void FDECL((*win_askname), (CARGS));
    void FDECL((*win_get_nh_event), (CARGS));
    void FDECL((*win_exit_nhwindows), (CARGS, const char *));
    void FDECL((*win_suspend_nhwindows), (CARGS, const char *));
    void FDECL((*win_resume_nhwindows), (CARGS));
    winid FDECL((*win_create_nhwindow), (CARGS, int));
    void FDECL((*win_clear_nhwindow), (CARGS, winid));
    void FDECL((*win_display_nhwindow), (CARGS, winid, BOOLEAN_P));
    void FDECL((*win_destroy_nhwindow), (CARGS, winid));
    void FDECL((*win_curs), (CARGS, winid, int, int));
    void FDECL((*win_putstr), (CARGS, winid, int, const char *));
    void FDECL((*win_putmixed), (CARGS, winid, int, const char *));
    void FDECL((*win_display_file), (CARGS, const char *, BOOLEAN_P));
    void FDECL((*win_start_menu), (CARGS, winid));
    void FDECL((*win_add_menu), (CARGS, winid, int, const ANY_P *, CHAR_P,
                                 CHAR_P, int, const char *, BOOLEAN_P));
    void FDECL((*win_end_menu), (CARGS, winid, const char *));
    int FDECL((*win_select_menu), (CARGS, winid, int, MENU_ITEM_P **));
    char FDECL((*win_message_menu), (CARGS, CHAR_P, int, const char *));
    void FDECL((*win_update_inventory), (CARGS));
    void FDECL((*win_mark_synch), (CARGS));
    void FDECL((*win_wait_synch), (CARGS));
#ifdef CLIPPING
    void FDECL((*win_cliparound), (CARGS, int, int));
#endif
#ifdef POSITIONBAR
    void FDECL((*win_update_positionbar), (CARGS, char *));
#endif
    void FDECL((*win_print_glyph), (CARGS, winid, XCHAR_P, XCHAR_P, int, int));
    void FDECL((*win_raw_print), (CARGS, const char *));
    void FDECL((*win_raw_print_bold), (CARGS, const char *));
    int FDECL((*win_nhgetch), (CARGS));
    int FDECL((*win_nh_poskey), (CARGS, int *, int *, int *));
    void FDECL((*win_nhbell), (CARGS));
    int FDECL((*win_doprev_message), (CARGS));
    char FDECL((*win_yn_function),
               (CARGS, const char *, const char *, CHAR_P));
    void FDECL((*win_getlin), (CARGS, const char *, char *));
    int FDECL((*win_get_ext_cmd), (CARGS));
    void FDECL((*win_number_pad), (CARGS, int));
    void FDECL((*win_delay_output), (CARGS));
#ifdef CHANGE_COLOR
    void FDECL((*win_change_color), (CARGS, int, long, int));
#ifdef MAC
    void FDECL((*win_change_background), (CARGS, int));
    short FDECL((*win_set_font_name), (CARGS, winid, char *));
#endif
    char *FDECL((*win_get_color_string), (CARGS));
#endif

    /* other defs that really should go away (they're tty specific) */
    void FDECL((*win_start_screen), (CARGS));
    void FDECL((*win_end_screen), (CARGS));

    void FDECL((*win_outrip), (CARGS, winid, int, time_t));
    void FDECL((*win_preference_update), (CARGS, const char *));
    char *FDECL((*win_getmsghistory), (CARGS, BOOLEAN_P));
    void FDECL((*win_putmsghistory), (CARGS, const char *, BOOLEAN_P));
    void FDECL((*win_status_init), (CARGS));
    void FDECL((*win_status_finish), (CARGS));
    void FDECL((*win_status_enablefield),
               (CARGS, int, const char *, const char *, BOOLEAN_P));
    void FDECL((*win_status_update), (CARGS, int, genericptr_t, int, int, int, unsigned long *));
    boolean FDECL((*win_can_suspend), (CARGS));
};
#endif /* WINCHAIN */

#ifdef SAFEPROCS
/*
 * window port routines available in sys/share/safeproc.c
 */
extern struct window_procs *FDECL(get_safe_procs, (int));
extern void FDECL(safe_init_nhwindows, (int *, char **));
extern void NDECL(safe_player_selection);
extern void NDECL(safe_askname);
extern void NDECL(safe_get_nh_event);
extern void FDECL(safe_exit_nhwindows, (const char *));
extern void FDECL(safe_suspend_nhwindows, (const char *));
extern void NDECL(safe_resume_nhwindows);
extern winid FDECL(safe_create_nhwindow, (int));
extern void FDECL(safe_clear_nhwindow, (winid));
extern void FDECL(safe_display_nhwindow, (winid, BOOLEAN_P));
extern void FDECL(safe_destroy_nhwindow, (winid));
extern void FDECL(safe_curs, (winid, int, int));
extern void FDECL(safe_putstr, (winid, int, const char *));
extern void FDECL(safe_putmixed, (winid, int, const char *));
extern void FDECL(safe_display_file, (const char *, BOOLEAN_P));
extern void FDECL(safe_start_menu, (winid));
extern void FDECL(safe_add_menu, (winid, int, const ANY_P *, CHAR_P, CHAR_P,
                                  int, const char *, BOOLEAN_P));
extern void FDECL(safe_end_menu, (winid, const char *));
extern int FDECL(safe_select_menu, (winid, int, MENU_ITEM_P **));
extern char FDECL(safe_message_menu, (CHAR_P, int, const char *));
extern void NDECL(safe_update_inventory);
extern void NDECL(safe_mark_synch);
extern void NDECL(safe_wait_synch);
#ifdef CLIPPING
extern void FDECL(safe_cliparound, (int, int));
#endif
#ifdef POSITIONBAR
extern void FDECL(safe_update_positionbar, (char *));
#endif
extern void FDECL(safe_print_glyph, (winid, XCHAR_P, XCHAR_P, int, int));
extern void FDECL(safe_raw_print, (const char *));
extern void FDECL(safe_raw_print_bold, (const char *));
extern int NDECL(safe_nhgetch);
extern int FDECL(safe_nh_poskey, (int *, int *, int *));
extern void NDECL(safe_nhbell);
extern int NDECL(safe_doprev_message);
extern char FDECL(safe_yn_function, (const char *, const char *, CHAR_P));
extern void FDECL(safe_getlin, (const char *, char *));
extern int NDECL(safe_get_ext_cmd);
extern void FDECL(safe_number_pad, (int));
extern void NDECL(safe_delay_output);
#ifdef CHANGE_COLOR
extern void FDECL(safe_change_color, (int, long, int));
#ifdef MAC
extern void FDECL(safe_change_background, (int));
extern short FDECL(safe_set_font_name, (winid, char *));
#endif
extern char *NDECL(safe_get_color_string);
#endif
extern void NDECL(safe_start_screen);
extern void NDECL(safe_end_screen);
extern void FDECL(safe_outrip, (winid, int, time_t));
extern void FDECL(safe_preference_update, (const char *));
extern char *FDECL(safe_getmsghistory, (BOOLEAN_P));
extern void FDECL(safe_putmsghistory, (const char *, BOOLEAN_P));
extern void NDECL(safe_status_init);
extern void NDECL(safe_status_finish);
extern void FDECL(safe_status_enablefield,
                    (int, const char *, const char *, BOOLEAN_P));
extern void FDECL(safe_status_update, (int, genericptr_t, int, int, int, unsigned long *));
extern boolean NDECL(safe_can_suspend);
extern void FDECL(stdio_raw_print, (const char *));
extern void FDECL(stdio_raw_print_bold, (const char *));
extern void NDECL(stdio_wait_synch);
extern int NDECL(stdio_nhgetch);
#endif /* SAFEPROCS */
#endif /* WINPROCS_H */


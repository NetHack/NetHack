/* NetHack 3.7 winshim.c    $NHDT-Date: 1596498345 2020/08/03 23:45:45 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.259 $ */
/* Copyright (c) Adam Powers, 2020                                */
/* NetHack may be freely redistributed.  See license for details. */

/* not an actual windowing port, but a fake win port for libnethack */

#include "hack.h"

#ifdef SHIM_GRAPHICS

enum win_types {
    WINSTUB_MESSAGE = 1,
    WINSTUB_MAP,
    WINSTUB_MENU,
    WINSTUB_EXT
};

#define VSTUB(name, args) \
void name args { \
    printf ("Running " #name "...\n"); \
}

#define STUB(name, retval, args) \
name args { \
    printf ("Running " #name "...\n"); \
    return retval; \
}

#define DECL(name, args) \
void name args;

VSTUB(shim_init_nhwindows,(int *argcp, char **argv))
VSTUB(shim_player_selection,(void))
VSTUB(shim_askname,(void))
VSTUB(shim_get_nh_event,(void))
VSTUB(shim_exit_nhwindows,(const char *a))
VSTUB(shim_suspend_nhwindows,(const char *a))
VSTUB(shim_resume_nhwindows,(void))
winid STUB(shim_create_nhwindow, WINSTUB_MAP, (int a))
VSTUB(shim_clear_nhwindow,(winid a))
VSTUB(shim_display_nhwindow,(winid a, BOOLEAN_P b))
VSTUB(shim_destroy_nhwindow,(winid a))
VSTUB(shim_curs,(winid a, int x, int y))
DECL(shim_putstr,(winid w, int attr, const char *str))
VSTUB(shim_display_file,(const char *a, BOOLEAN_P b))
VSTUB(shim_start_menu,(winid w, unsigned long mbehavior))
VSTUB(shim_add_menu,(winid a, int b, const ANY_P *c, CHAR_P d, CHAR_P e, int f, const char *h, unsigned int k))
VSTUB(shim_end_menu,(winid a, const char *b))
int STUB(shim_select_menu,0,(winid a, int b, MENU_ITEM_P **c))
char STUB(shim_message_menu,'y',(CHAR_P a, int b, const char *c))
VSTUB(shim_update_inventory,(void))
VSTUB(shim_mark_synch,(void))
VSTUB(shim_wait_synch,(void))
VSTUB(shim_cliparound,(int a, int b))
VSTUB(shim_update_positionbar,(char *a))
DECL(shim_print_glyph,(winid w, XCHAR_P x, XCHAR_P y, int glyph, int bkglyph))
DECL(shim_raw_print,(const char *str))
VSTUB(shim_raw_print_bold,(const char *a))
int STUB(shim_nhgetch,0,(void))
int STUB(shim_nh_poskey,0,(int *a, int *b, int *c))
VSTUB(shim_nhbell,(void))
int STUB(shim_doprev_message,0,(void))
char STUB(shim_yn_function,'y',(const char *a, const char *b, CHAR_P c))
VSTUB(shim_getlin,(const char *a, char *b))
int STUB(shim_get_ext_cmd,0,(void))
VSTUB(shim_number_pad,(int a))
VSTUB(shim_delay_output,(void))
VSTUB(shim_change_color,(int a, long b, int c))
VSTUB(shim_change_background,(int a))
short STUB(set_shim_font_name,0,(winid a, char *b))
VSTUB(shim_get_color_string,(void))

/* other defs that really should go away (they're tty specific) */
VSTUB(shim_start_screen, (void))
VSTUB(shim_end_screen, (void))
VSTUB(shim_preference_update, (const char *a))
char *STUB(shim_getmsghistory, (char *)"", (BOOLEAN_P a))
VSTUB(shim_putmsghistory, (const char *a, BOOLEAN_P b))
VSTUB(shim_status_init, (void))
VSTUB(shim_status_enablefield, (int a, const char *b, const char *c, BOOLEAN_P d))
VSTUB(shim_status_update, (int a, genericptr_t b, int c, int d, int e, unsigned long *f))


/* old:       | WC_TILED_MAP */
/* Interface definition, for windows.c */
struct window_procs shim_procs = {
    "shim",
    (0
     | WC_ASCII_MAP
     | WC_COLOR | WC_HILITE_PET | WC_INVERSE | WC_EIGHT_BIT_IN),
    (0
#if defined(SELECTSAVED)
     | WC2_SELECTSAVED
#endif
#if defined(STATUS_HILITES)
     | WC2_HILITE_STATUS | WC2_HITPOINTBAR | WC2_FLUSH_STATUS
     | WC2_RESET_STATUS
#endif
     | WC2_DARKGRAY | WC2_SUPPRESS_HIST | WC2_STATUSLINES),
#ifdef TEXTCOLOR
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   /* color availability */
#else
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
#endif
    shim_init_nhwindows, shim_player_selection, shim_askname, shim_get_nh_event,
    shim_exit_nhwindows, shim_suspend_nhwindows, shim_resume_nhwindows,
    shim_create_nhwindow, shim_clear_nhwindow, shim_display_nhwindow,
    shim_destroy_nhwindow, shim_curs, shim_putstr, genl_putmixed,
    shim_display_file, shim_start_menu, shim_add_menu, shim_end_menu,
    shim_select_menu, shim_message_menu, shim_update_inventory, shim_mark_synch,
    shim_wait_synch,
#ifdef CLIPPING
    shim_cliparound,
#endif
#ifdef POSITIONBAR
    shim_update_positionbar,
#endif
    shim_print_glyph, shim_raw_print, shim_raw_print_bold, shim_nhgetch,
    shim_nh_poskey, shim_nhbell, shim_doprev_message, shim_yn_function,
    shim_getlin, shim_get_ext_cmd, shim_number_pad, shim_delay_output,
#ifdef CHANGE_COLOR /* the Mac uses a palette device */
    shim_change_color,
#ifdef MAC
    shim_change_background, set_shim_font_name,
#endif
    shim_get_color_string,
#endif

    /* other defs that really should go away (they're tty specific) */
    shim_start_screen, shim_end_screen, genl_outrip,
    shim_preference_update,
    shim_getmsghistory, shim_putmsghistory,
    shim_status_init,
    genl_status_finish, genl_status_enablefield,
#ifdef STATUS_HILITES
    shim_status_update,
#else
    genl_status_update,
#endif
    genl_can_suspend_yes,
};

void shim_print_glyph(winid w, XCHAR_P x, XCHAR_P y, int glyph, int bkglyph) {
    /* map glyph to character and color */
    // (void) mapglyph(glyph, &ch, &color, &special, x, y, 0);

    fprintf(stdout, "shim_print_glyph (%d,%d): %c\n", x,y,(char)glyph);
    fflush(stdout);
}

void shim_raw_print(const char *str) {
    fprintf(stdout, "shim_raw_print: %s\n", str);
    fflush(stdout);
}

void shim_putstr(winid w, int attr, const char *str) {
    fprintf(stdout, "shim_putstr (win %d): %s\n", w, str);
    fflush(stdout);
}

#endif /* SHIM_GRAPHICS */
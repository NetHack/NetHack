/* NetHack 3.7 winshim.c    $NHDT-Date: 1596498345 2020/08/03 23:45:45 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.259 $ */
/* Copyright (c) Adam Powers, 2020                                */
/* NetHack may be freely redistributed.  See license for details. */

/* not an actual windowing port, but a fake win port for libnethack */

#include "hack.h"

#ifdef SHIM_GRAPHICS
#include <stdarg.h>
/* for cross-compiling to WebAssembly (WASM) */
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#define SHIM_DEBUG

#ifndef __EMSCRIPTEN__
typedef void(*stub_callback_t)(const char *name, void *ret_ptr, const char *fmt, ...);
#else /* __EMSCRIPTEN__ */
/* WASM can't handle a variadic callback, so we pass back an array of pointers instead... */
typedef void(*stub_callback_t)(const char *name, void *ret_ptr, const char *fmt, void *args[]);
#endif /* !__EMSCRIPTEN__ */

/* this is the primary interface to shim graphics,
 * call this function with your declared callback function
 * and you will receive all the windowing calls
 */
static stub_callback_t shim_graphics_callback = NULL;
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_KEEPALIVE
#endif
void stub_graphics_set_callback(stub_callback_t cb) {
    shim_graphics_callback = cb;
}

#ifdef __EMSCRIPTEN__
/* A2P = Argument to Pointer */
#define A2P &
/* P2V = Pointer to Void */
#define P2V (void *)
#define DECLCB(ret_type, name, fn_args, fmt, ...) \
ret_type name fn_args { \
    void *args[] = { __VA_ARGS__ }; \
    ret_type ret; \
    debugf("SHIM GRAPHICS: " #name "\n"); \
    if (!shim_graphics_callback) return; \
    shim_graphics_callback(#name, (void *)&ret, fmt, args); \
    return ret; \
}

#define VDECLCB(name, fn_args, fmt, ...) \
void name fn_args { \
    void *args[] = { __VA_ARGS__ }; \
    debugf("SHIM GRAPHICS: " #name "\n"); \
    if (!shim_graphics_callback) return; \
    shim_graphics_callback(#name, NULL, fmt, args); \
}
#else /* !__EMSCRIPTEN__ */
#define A2P
#define P2V
#define DECLCB(ret_type, name, fn_args, fmt, ...) \
ret_type name fn_args { \
    ret_type ret; \
    debugf("SHIM GRAPHICS: " #name "\n"); \
    if (!shim_graphics_callback) return; \
    shim_graphics_callback(#name, (void *)&ret, fmt, __VA_ARGS__); \
    return ret; \
}

#define VDECLCB(name, fn_args, fmt, ...) \
void name fn_args { \
    debugf("SHIM GRAPHICS: " #name "\n"); \
    if (!shim_graphics_callback) return; \
    shim_graphics_callback(#name, NULL, fmt, __VA_ARGS__); \
}
#endif /* __EMSCRIPTEN__ */

#ifdef SHIM_DEBUG
#define debugf printf
#else /* !SHIM_DEBUG */
#define debugf(...)
#endif /* SHIM_DEBUG */


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
VDECLCB(shim_curs,(winid a, int x, int y), "viii", A2P a, A2P x, A2P y)
VDECLCB(shim_putstr,(winid w, int attr, const char *str), "viis", A2P w, A2P attr, P2V str)
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
VDECLCB(shim_print_glyph,(winid w, int x, int y, int glyph, int bkglyph), "viiiii", A2P w, A2P x, A2P y, A2P glyph, A2P bkglyph)
VDECLCB(shim_raw_print,(const char *str), "vs", P2V str)
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

#endif /* SHIM_GRAPHICS */
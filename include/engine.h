/* NetHack 3.6	engine.h	$NHDT-Date:  $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.76 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* Copyright (c) Bart House, 2017                                 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef ENGINE_H
#define ENGINE_H

#include "winprocs.h"

typedef struct engine_callbacks {
    void (*win_tty_init)(int);
    void (*nttty_open)(int);
    void (*xputc)(char);
    void(*xputs)(const char *);
    void(*mswin_destroy_reg)(void);
    void(*raw_clear_screen)(void);
    void(*clear_screen)(void);
    void(*backsp)(void);
    int(*has_color)(int);
#ifndef NO_MOUSE_ALLOWED
    void(*toggle_mouse_support)(void);
#endif
#ifdef PORT_DEBUG
    void(*win32con_debug_keystrokes)(void);
    void(*win32con_handler_info)(void);
#endif
    void(*map_subkeyvalue)(char *);
    void(*load_keyboard_handler)(void);
    void(*vmsmsg)(const char*, va_list);
    void(*ntty_error)(const char*, va_list);
    void(*synch_cursor)(void);
    void(*more)(void);
    struct window_procs * tty_procs;
    struct window_procs * mswin_procs;
} engine_callbacks;

#if defined(ENGINE_EXPORT) || defined(ENGINE_IMPORT)
ENGINE_FUNC int engine_main(int argc, char * argv[], engine_callbacks *);
ENGINE_FUNC void engine_free(void * ptr);
#endif /* ENGINE_EXPORT || ENGINE_IMPORT */

#endif /* ENGINE */

/* NetHack 3.6	conmain.c	$NHDT-Date:  $  $NHDT-Branch: chasonr $:$NHDT-Revision: 1.69 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* Copyright (c) Bart House, 2017                                 */
/* NetHack may be freely redistributed.  See license for details. */

/* conmain.c - Window Console NetHack */

#include "hack.h"
#include "extern.h"
#include "wintty.h"

/* TODO add extern.h */
extern void NDECL(win32con_debug_keystrokes);
extern void NDECL(win32con_handler_info);
extern void vmsmsg(const char * fmt, va_list the_args);

int main(int argc, char *argv[])
{
    engine_callbacks callbacks = { NULL };
    callbacks.win_tty_init = win_tty_init;
    callbacks.nttty_open = nttty_open;
    callbacks.xputc = xputc;
    callbacks.xputs = xputs;
    callbacks.raw_clear_screen = raw_clear_screen;
    callbacks.clear_screen = clear_screen;
    callbacks.backsp = backsp;
    callbacks.has_color = has_color;
#ifndef NO_MOUSE_ALLOWED
    callbacks.toggle_mouse_support = toggle_mouse_support;
#endif
#ifdef PORT_DEBUG
    callbacks.win32con_debug_keystrokes = win32con_debug_keystrokes;
    callbacks.win32con_handler_info = win32con_handler_info;
#endif
    callbacks.map_subkeyvalue = map_subkeyvalue;
    callbacks.load_keyboard_handler = load_keyboard_handler;
    callbacks.vmsmsg = vmsmsg;
    callbacks.synch_cursor = synch_cursor;
    callbacks.more = more;
    callbacks.tty_procs = &tty_procs;
	engine_setcallbacks(&callbacks);
    engine_main(argc, argv);
}

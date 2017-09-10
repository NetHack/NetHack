/* NetHack 3.6	conmain.c	$NHDT-Date:  $  $NHDT-Branch: chasonr $:$NHDT-Revision: 1.69 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* Copyright (c) Bart House, 2017                                 */
/* NetHack may be freely redistributed.  See license for details. */

/* engine.c - Supplemental code to support compiling NetHack engine as a standalone DLL
              and callable from the windowing interface (application shell) */

#include "hack.h"
#include "extern.h"
#include "win32api.h"

static engine_callbacks callbacks = { NULL };

HANDLE hConIn;
HANDLE hConOut;
int GUILaunched;

struct window_procs mswin_procs = { "guistubs" };
struct window_procs tty_procs = { "ttystubs" };

void win_tty_init(int dir)
{
    if (callbacks.win_tty_init != NULL)
        callbacks.win_tty_init(dir);
}

void nttty_open(int mode)
{
    if (callbacks.nttty_open != NULL)
        callbacks.nttty_open(mode);
}

void
xputc(char ch)
{
    if (callbacks.xputc != NULL)
        callbacks.xputc(ch);
}

void
xputs(const char *s)
{
    if (callbacks.xputs != NULL)
        callbacks.xputs(s);
}

void
mswin_destroy_reg()
{
    if (callbacks.mswin_destroy_reg != NULL)
        callbacks.mswin_destroy_reg();
}

void
raw_clear_screen()
{
    if (callbacks.raw_clear_screen != NULL)
        callbacks.raw_clear_screen();
}

void
clear_screen()
{
    if (callbacks.clear_screen != NULL)
        callbacks.clear_screen();
}

void
backsp()
{
    if (callbacks.backsp != NULL)
        callbacks.backsp();
}

int
has_color(int color)
{
    if (callbacks.has_color != NULL)
        return callbacks.has_color(color);
    else
        return 1;
}

#ifndef NO_MOUSE_ALLOWED
void
toggle_mouse_support()
{
    if (callbacks.toggle_mouse_support != NULL)
        callbacks.toggle_mouse_support();
}
#endif

#ifdef PORT_DEBUG
void
win32con_debug_keystrokes()
{
    if (callbacks.win32con_debug_keystrokes != NULL)
        callbacks.win32con_debug_keystrokes();
}
void
win32con_handler_info()
{
    if (callbacks.win32con_handler_info != NULL)
        callbacks.win32con_handler_info();
}
#endif

void
map_subkeyvalue(char * in)
{
    if (callbacks.map_subkeyvalue != NULL)
        callbacks.map_subkeyvalue(in);
}

void
load_keyboard_handler()
{
    if (callbacks.load_keyboard_handler != NULL)
        callbacks.load_keyboard_handler();
}

void msmsg
VA_DECL(const char *, fmt)
{
    VA_START(fmt);
    VA_INIT(fmt, const char *);
    if (callbacks.vmsmsg != NULL)
        callbacks.vmsmsg(fmt, the_args);
    VA_END();
    return;
}

void nttty_error
VA_DECL(const char *, s)
{
    VA_START(s);
    VA_INIT(s, const char *);
    if (callbacks.ntty_error != NULL)
        callbacks.ntty_error(s, the_args);
    VA_END();
    return;
}

void
synch_cursor()
{
    if (callbacks.synch_cursor != NULL)
        callbacks.synch_cursor();
}

void
more()
{
    if (callbacks.more != NULL)
        callbacks.more();
}

void engine_free(void *ptr)
{
    free(ptr);
}

int engine_setcallbacks(engine_callbacks * inCallbacks)
{
    callbacks = *inCallbacks;
    /* TODO we should instead set the pointer to the procs in the table */
    if (callbacks.tty_procs != NULL)
        tty_procs = *callbacks.tty_procs;
    if (callbacks.mswin_procs != NULL)
        mswin_procs = *callbacks.mswin_procs;
}

int engine_main(int argc, char *argv[])
{
	return main(argc, argv);
}

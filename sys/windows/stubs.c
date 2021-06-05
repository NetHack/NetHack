/* NetHack 3.7  stubs.c       $NHDT-Date: 1596498317 2020/08/03 23:45:17 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.31 $ */
/*      Copyright (c) 2015 by Michael Allison              */
/* NetHack may be freely redistributed.  See license for details. */

#include "win32api.h"
#include "hack.h"

#ifdef GUISTUB
#ifdef TTYSTUB
#error You cannot compile this with both GUISTUB and TTYSTUB defined.
#endif

int GUILaunched;
struct window_procs mswin_procs = { "-guistubs" };

#ifdef QT_GRAPHICS
struct window_procs Qt_procs = { "-guistubs" };
int qt_tilewidth, qt_tileheight, qt_fontsize, qt_compact_mode;
#endif
void
mswin_destroy_reg(void)
{
    return;
}
void
mswin_raw_print_flush(void)
{
}

void
mswin_raw_print(const char *str)
{
}

/* MINGW32 has trouble with both a main() and WinMain()
 * so we move main for the MINGW tty version into this stub
 * so that it is out of sight for the gui linkage.
 */
#ifdef __MINGW32__
extern char default_window_sys[];
extern int mingw_main(int argc, char **argv);

int
main(int argc, char *argv[])
{
    boolean resuming;

    resuming = mingw_main(argc, argv);
    nethack_exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}
#endif

#endif /* GUISTUB */

/* =============================================== */

#ifdef TTYSTUB

HANDLE hConIn;
HANDLE hConOut;
int GUILaunched;
struct window_procs tty_procs = { "-ttystubs" };
#ifdef CURSES_GRAPHICS
char erase_char, kill_char;
#endif

void
win_tty_init(int dir)
{
    return;
}

void
consoletty_open(int mode)
{
    return;
}

int
xputc(int ch)
{
    return 0;
}

void
xputs(const char *s)
{
    return;
}

void
raw_clear_screen(void)
{
    return;
}

void
clear_screen(void)
{
    return;
}

void
backsp(void)
{
    return;
}

#ifndef NO_MOUSE_ALLOWED
void
toggle_mouse_support(void)
{
    return;
}
#endif

#ifdef PORT_DEBUG
void
win32con_debug_keystrokes(void)
{
    return;
}
void
win32con_handler_info(void)
{
    return;
}
#endif

void
map_subkeyvalue(char *op)
{
    return;
}

/* this is used as a printf() replacement when the window
 * system isn't initialized yet
 */
void msmsg
VA_DECL(const char *, fmt)
{
    VA_START(fmt);
    VA_INIT(fmt, const char *);
    VA_END();
    return;
}

/*VARARGS1*/
void consoletty_error
VA_DECL(const char *, s)
{
    VA_START(s);
    VA_INIT(s, const char *);
    VA_END();
    return;
}

#ifdef TTY_GRAPHICS
void
synch_cursor(void)
{
    return;
}
#endif

void
more(void)
{
    return;
}

void
nethack_enter_consoletty(void)
{
    return;
}

void
set_altkeyhandler(const char *inName)
{
    return;
}

#if defined(USER_SOUNDS) && defined(TTY_SOUND_ESCCODES)
void
play_usersound_via_idx(int idx, int volume)
{
}
#endif /* USER_SOUNDS && TTY_SOUND_ESCCODES */

#endif /* TTYSTUBS */

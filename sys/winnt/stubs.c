/* NetHack 3.6  stubs.c       $NHDT-Date: 1524689357 2018/04/25 20:49:17 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.3 $ */
/*      Copyright (c) 2015 by Michael Allison              */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef GUISTUB
#ifdef TTYSTUB
#error You can't compile this with both GUISTUB and TTYSTUB defined.
#endif

int GUILaunched;
struct window_procs mswin_procs = { "guistubs" };

#ifdef QT_GRAPHICS
struct window_procs Qt_procs = { "guistubs" };
int qt_tilewidth, qt_tileheight, qt_fontsize, qt_compact_mode;
#endif
void
mswin_destroy_reg()
{
    return;
}

/* MINGW32 has trouble with both a main() and WinMain()
 * so we move main for the MINGW tty version into this stub
 * so that it is out of sight for the gui linkage.
 */
#ifdef __MINGW32__
extern char default_window_sys[];

int
main(argc, argv)
int argc;
char *argv[];
{
    boolean resuming;

    sys_early_init();
    Strcpy(default_window_sys, "tty");
    resuming = pcmain(argc, argv);
    moveloop(resuming);
    nethack_exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}
#endif

#endif /* GUISTUB */

/* =============================================== */

#ifdef TTYSTUB

#include "hack.h"
#include "win32api.h"

HANDLE hConIn;
HANDLE hConOut;
int GUILaunched;
struct window_procs tty_procs = { "ttystubs" };

void
win_tty_init(dir)
int dir;
{
    return;
}

void
nttty_open(mode)
int mode;
{
    return;
}

void
xputc(ch)
char ch;
{
    return;
}

void
xputs(s)
const char *s;
{
    return;
}

void
raw_clear_screen()
{
    return;
}

void
clear_screen()
{
    return;
}

void
backsp()
{
    return;
}

int
has_color(int color)
{
    return 1;
}

#ifndef NO_MOUSE_ALLOWED
void
toggle_mouse_support()
{
    return;
}
#endif

#ifdef PORT_DEBUG
void
win32con_debug_keystrokes()
{
    return;
}
void
win32con_handler_info()
{
    return;
}
#endif

void
map_subkeyvalue(op)
register char *op;
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
void nttty_error
VA_DECL(const char *, s)
{
    VA_START(s);
    VA_INIT(s, const char *);
    VA_END();
    return;
}

void
synch_cursor()
{
    return;
}

void
more()
{
    return;
}

void
nethack_enter_nttty()
{
    return;
}

void
set_altkeyhandler(const char *inName)
{
    return;
}
#endif /* TTYSTUBS */

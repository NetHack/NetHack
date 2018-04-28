/* NetHack 3.6	ioctl.c	$NHDT-Date: 1520099308 2018/03/03 17:48:28 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.13 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

/* This cannot be part of hack.tty.c (as it was earlier) since on some
   systems (e.g. MUNIX) the include files <termio.h> and <sgtty.h>
   define the same constants, and the C preprocessor complains. */

#include "hack.h"

#if defined(BSD_JOB_CONTROL) || defined(_BULL_SOURCE)
#ifdef HPUX
#include <bsdtty.h>
#else
#if defined(AIX_31) && !defined(_ALL_SOURCE)
#define _ALL_SOURCE /* causes struct winsize to be present */
#ifdef _AIX32
#include <sys/ioctl.h>
#endif
#endif
#if defined(_BULL_SOURCE)
#include <termios.h>
struct termios termio;
#undef TIMEOUT            /* defined in you.h and sys/tty.h */
#include <sys/tty.h>      /* define winsize */
#include <sys/ttold.h>    /* define struct ltchars */
#include <sys/bsdioctl.h> /* define TIOGWINSZ */
#else
#ifdef LINUX
#include <bsd/sgtty.h>
#else
#include <sgtty.h>
#endif
#endif
#endif
struct ltchars ltchars;
struct ltchars ltchars0 = { -1, -1, -1, -1, -1, -1 }; /* turn all off */
#else

#ifdef POSIX_TYPES
#include <termios.h>
struct termios termio;
#if defined(BSD) || defined(_AIX32) || defined(__linux__)
#if defined(_AIX32) && !defined(_ALL_SOURCE)
#define _ALL_SOURCE
#endif
#include <sys/ioctl.h>
#endif
#else
#include <termio.h> /* also includes part of <sgtty.h> */
#if defined(TCSETS) && !defined(AIX_31)
struct termios termio;
#else
struct termio termio;
#endif
#endif
#if defined(AMIX) || defined(__APPLE__)
#include <sys/ioctl.h>
#endif /* AMIX */
#endif

#ifdef SUSPEND /* BSD isn't alone anymore... */
#include <signal.h>
#endif

/* AVOID_WIN_IOCTL can be uncommented in unixconf.h
 * to force USE_WIN_IOTCL to remain undefined,
 * instead of the restricted explicit opt-in
 * logic that used to be here.
 */
#if defined(TIOCGWINSZ) && !defined(AVOID_WIN_IOCTL)
#define USE_WIN_IOCTL
#include "tcap.h" /* for LI and CO */
#endif

#ifdef _M_UNIX
extern void NDECL(sco_mapon);
extern void NDECL(sco_mapoff);
#endif
#ifdef __linux__
extern void NDECL(linux_mapon);
extern void NDECL(linux_mapoff);
#endif

#ifdef AUX
void
catch_stp()
{
    signal(SIGTSTP, SIG_DFL);
    dosuspend();
}
#endif /* AUX */

void
getwindowsz()
{
#ifdef USE_WIN_IOCTL
    /*
     * ttysize is found on Suns and BSD
     * winsize is found on Suns, BSD, and Ultrix
     */
    struct winsize ttsz;

    if (ioctl(fileno(stdin), (int) TIOCGWINSZ, (char *) &ttsz) != -1) {
        /*
         * Use the kernel's values for lines and columns if it has
         * any idea.
         */
        if (ttsz.ws_row)
            LI = ttsz.ws_row;
        if (ttsz.ws_col)
            CO = ttsz.ws_col;
    }
#endif
}

void
getioctls()
{
#ifdef BSD_JOB_CONTROL
    (void) ioctl(fileno(stdin), (int) TIOCGLTC, (char *) &ltchars);
    (void) ioctl(fileno(stdin), (int) TIOCSLTC, (char *) &ltchars0);
#else
#ifdef POSIX_TYPES
    (void) tcgetattr(fileno(stdin), &termio);
#else
#if defined(TCSETS) && !defined(AIX_31)
    (void) ioctl(fileno(stdin), (int) TCGETS, &termio);
#else
    (void) ioctl(fileno(stdin), (int) TCGETA, &termio);
#endif
#endif
#endif
    getwindowsz();
#ifdef AUX
    (void) signal(SIGTSTP, catch_stp);
#endif
}

void
setioctls()
{
#ifdef BSD_JOB_CONTROL
    (void) ioctl(fileno(stdin), (int) TIOCSLTC, (char *) &ltchars);
#else
#ifdef POSIX_TYPES
    (void) tcsetattr(fileno(stdin), TCSADRAIN, &termio);
#else
#if defined(TCSETS) && !defined(AIX_31)
    (void) ioctl(fileno(stdin), (int) TCSETSW, &termio);
#else
    (void) ioctl(fileno(stdin), (int) TCSETAW, &termio);
#endif
#endif
#endif
}

#ifdef SUSPEND /* No longer implies BSD */
int
dosuspend()
{
#ifdef SYSCF
    /* NB: check_user_string() is port-specific. */
    if (!sysopt.shellers || !sysopt.shellers[0]
        || !check_user_string(sysopt.shellers)) {
        Norep("Suspend command not available.");
        return 0;
    }
#endif
#if defined(SIGTSTP) && !defined(NO_SIGNAL)
    if (signal(SIGTSTP, SIG_IGN) == SIG_DFL) {
        suspend_nhwindows((char *) 0);
#ifdef _M_UNIX
        sco_mapon();
#endif
#ifdef __linux__
        linux_mapon();
#endif
        (void) signal(SIGTSTP, SIG_DFL);
#ifdef AUX
        (void) kill(0, SIGSTOP);
#else
        (void) kill(0, SIGTSTP);
#endif
#ifdef _M_UNIX
        sco_mapoff();
#endif
#ifdef __linux__
        linux_mapoff();
#endif
        resume_nhwindows();
    } else {
        pline("I don't think your shell has job control.");
    }
#else
    pline("Sorry, it seems we have no SIGTSTP here.  Try ! or S.");
#endif
    return (0);
}
#endif /* SUSPEND */

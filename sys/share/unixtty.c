/* NetHack 3.7	unixtty.c	$NHDT-Date: 1596498288 2020/08/03 23:44:48 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.27 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

/* tty.c - (Unix) version */

/* With thanks to the people who sent code for SYSV - hpscdi!jon,
 * arnold@ucsf-cgl, wcs@bo95b, cbcephus!pds and others.
 */

#define NEED_VARARGS
#include "hack.h"

/*
 * The distinctions here are not BSD - rest but rather USG - rest, as
 * BSD still has the old sgttyb structure, but SYSV has termio. Thus:
 */
#if (defined(BSD) || defined(ULTRIX)) && !defined(POSIX_TYPES)
#define V7
#else
#define USG
#endif

#ifdef USG

#ifdef POSIX_TYPES
#include <termios.h>
#include <unistd.h>
#define termstruct termios
#else
#include <termio.h>
#if defined(TCSETS) && !defined(AIX_31)
#define termstruct termios
#else
#define termstruct termio
#endif
#endif /* POSIX_TYPES */
#ifdef LINUX
#include <sys/ioctl.h>
#undef delay_output /* curses redefines this */
#include <curses.h>
#endif
#define kill_sym c_cc[VKILL]
#define erase_sym c_cc[VERASE]
#define intr_sym c_cc[VINTR]
#ifdef TAB3 /* not a POSIX flag, but some have it anyway */
#define EXTABS TAB3
#else
#define EXTABS 0
#endif
#define tabflgs c_oflag
#define echoflgs c_lflag
#define cbrkflgs c_lflag
#define CBRKMASK ICANON
#define CBRKON !/* reverse condition */
#ifdef POSIX_TYPES
#define OSPEED(x) (speednum(cfgetospeed(&x)))
#else
#ifndef CBAUD
#define CBAUD _CBAUD /* for POSIX nitpickers (like RS/6000 cc) */
#endif
#define OSPEED(x) ((x).c_cflag & CBAUD)
#endif
#define IS_7BIT(x) ((x).c_cflag & CS7)
#define inputflags c_iflag
#define STRIPHI ISTRIP
#ifdef POSIX_TYPES
#define GTTY(x) (tcgetattr(0, x))
#define STTY(x) (tcsetattr(0, TCSADRAIN, x))
#else
#if defined(TCSETS) && !defined(AIX_31)
#define GTTY(x) (ioctl(0, TCGETS, x))
#define STTY(x) (ioctl(0, TCSETSW, x))
#else
#define GTTY(x) (ioctl(0, TCGETA, x))
#define STTY(x) (ioctl(0, TCSETAW, x))
#endif
#endif /* POSIX_TYPES */
#define GTTY2(x) 1
#define STTY2(x) 1
#ifdef POSIX_TYPES
#if defined(BSD) && !defined(__DGUX__)
#define nonesuch _POSIX_VDISABLE
#else
#define nonesuch (fpathconf(0, _PC_VDISABLE))
#endif
#else
#define nonesuch 0
#endif
#define inittyb2 inittyb
#define curttyb2 curttyb

#else /* V7 */

#include <sgtty.h>
#define termstruct sgttyb
#define kill_sym sg_kill
#define erase_sym sg_erase
#define intr_sym t_intrc
#define EXTABS XTABS
#define tabflgs sg_flags
#define echoflgs sg_flags
#define cbrkflgs sg_flags
#define CBRKMASK CBREAK
#define CBRKON              /* empty */
#define inputflags sg_flags /* don't know how enabling meta bits */
#define IS_7BIT(x) (FALSE)
#define STRIPHI 0 /* should actually be done on BSD */
#define OSPEED(x) (x).sg_ospeed
#if defined(bsdi) || defined(__386BSD) || defined(SUNOS4)
#define GTTY(x) (ioctl(0, TIOCGETP, (char *) x))
#define STTY(x) (ioctl(0, TIOCSETP, (char *) x))
#else
#define GTTY(x) (gtty(0, x))
#define STTY(x) (stty(0, x))
#endif
#define GTTY2(x) (ioctl(0, TIOCGETC, (char *) x))
#define STTY2(x) (ioctl(0, TIOCSETC, (char *) x))
#define nonesuch -1
struct tchars inittyb2, curttyb2;

#endif /* V7 */

/*
 * Old curses.h relied on implicit declaration of has_colors().
 * Modern compilers tend to warn about implicit declarations and
 * modern curses.h declares has_colors() explicitly.  However, the
 * declaration it uses is not one we can simply clone (requires
 * <stdbool.h>).  This simpler declaration suffices but can't be
 * used unconditionally because it conflicts with the 'bool' one.
 */
#ifdef NEED_HAS_COLORS_DECL
int has_colors(void);
#endif

#if defined(TTY_GRAPHICS) && ((!defined(SYSV) && !defined(HPUX)) \
                              || defined(UNIXPC) || defined(SVR4))
extern /* it is defined in libtermlib (libtermcap) */
    short ospeed; /* terminal baudrate; set by gettty */
#else
short ospeed = 0; /* gets around "not defined" error message */
#endif

#if defined(POSIX_TYPES) && defined(BSD)
unsigned
#endif
    char erase_char,
    intr_char, kill_char;
static boolean settty_needed = FALSE;
struct termstruct inittyb, curttyb;

#ifdef POSIX_TYPES
static int
speednum(speed_t speed)
{
    switch (speed) {
    case B0:
        return 0;
    case B50:
        return 1;
    case B75:
        return 2;
    case B110:
        return 3;
    case B134:
        return 4;
    case B150:
        return 5;
    case B200:
        return 6;
    case B300:
        return 7;
    case B600:
        return 8;
    case B1200:
        return 9;
    case B1800:
        return 10;
    case B2400:
        return 11;
    case B4800:
        return 12;
    case B9600:
        return 13;
    case B19200:
        return 14;
    case B38400:
        return 15;
    }

    return 0;
}
#endif

static void
setctty(void)
{
    if (STTY(&curttyb) < 0 || STTY2(&curttyb2) < 0)
        perror("NetHack (setctty)");
}

/*
 * Get initial state of terminal, set ospeed (for termcap routines)
 * and switch off tab expansion if necessary.
 * Called by startup() in termcap.c and after returning from ! or ^Z
 */
void
gettty(void)
{
    if (GTTY(&inittyb) < 0 || GTTY2(&inittyb2) < 0)
        perror("NetHack (gettty)");
    curttyb = inittyb;
    curttyb2 = inittyb2;
    ospeed = OSPEED(inittyb);
    erase_char = inittyb.erase_sym;
    kill_char = inittyb.kill_sym;
    intr_char = inittyb2.intr_sym;
    getioctls();

    /* do not expand tabs - they might be needed inside a cm sequence */
    if (curttyb.tabflgs & EXTABS) {
        curttyb.tabflgs &= ~EXTABS;
        setctty();
    }
    settty_needed = TRUE;
}

/* reset terminal to original state */
void
settty(const char *s)
{
    end_screen();
    if (s)
        raw_print(s);
    if (STTY(&inittyb) < 0 || STTY2(&inittyb2) < 0)
        perror("NetHack (settty)");
    iflags.echo = (inittyb.echoflgs & ECHO) ? ON : OFF;
    iflags.cbreak = (CBRKON(inittyb.cbrkflgs & CBRKMASK)) ? ON : OFF;
    curttyb.inputflags |= STRIPHI;
    setioctls();
    settty_needed = FALSE;
}

void
setftty(void)
{
    unsigned ef, cf;
    int change = 0;

    ef = 0;                /* desired value of flags & ECHO */
    cf = CBRKON(CBRKMASK); /* desired value of flags & CBREAK */
    iflags.cbreak = ON;
    iflags.echo = OFF;
    /* Should use (ECHO|CRMOD) here instead of ECHO */
    if ((unsigned) (curttyb.echoflgs & ECHO) != ef) {
        curttyb.echoflgs &= ~ECHO;
        /* curttyb.echoflgs |= ef; */
        change++;
    }
    if ((unsigned) (curttyb.cbrkflgs & CBRKMASK) != cf) {
        curttyb.cbrkflgs &= ~CBRKMASK;
        curttyb.cbrkflgs |= cf;
#ifdef USG
        /* be satisfied with one character; no timeout */
        curttyb.c_cc[VMIN] = 1;  /* was VEOF */
        curttyb.c_cc[VTIME] = 0; /* was VEOL */
#ifdef POSIX_JOB_CONTROL
/* turn off system suspend character
 * due to differences in structure layout, this has to be
 * here instead of in ioctl.c:getioctls() with the BSD
 * equivalent
 */
#ifdef VSUSP /* real POSIX */
        curttyb.c_cc[VSUSP] = nonesuch;
#else /* other later SYSV */
        curttyb.c_cc[VSWTCH] = nonesuch;
#endif
#endif
#ifdef VDSUSP /* SunOS Posix extensions */
        curttyb.c_cc[VDSUSP] = nonesuch;
#endif
#ifdef VREPRINT
        curttyb.c_cc[VREPRINT] = nonesuch;
#endif
#ifdef VDISCARD
        curttyb.c_cc[VDISCARD] = nonesuch;
#endif
#ifdef VWERASE
        curttyb.c_cc[VWERASE] = nonesuch;
#endif
#ifdef VLNEXT
        curttyb.c_cc[VLNEXT] = nonesuch;
#endif
#endif
        change++;
    }
    if (!IS_7BIT(inittyb))
        curttyb.inputflags &= ~STRIPHI;
    /* If an interrupt character is used, it will be overridden and
     * set to ^C.
     */
    if (intr_char != nonesuch && curttyb2.intr_sym != '\003') {
        curttyb2.intr_sym = '\003';
        change++;
    }

    if (change)
        setctty();
    start_screen();
}

void intron(void) /* enable kbd interupts if enabled when game started */
{
#ifdef TTY_GRAPHICS
    /* Ugly hack to keep from changing tty modes for non-tty games -dlc */
    if (WINDOWPORT(tty) && intr_char != nonesuch
        && curttyb2.intr_sym != '\003') {
        curttyb2.intr_sym = '\003';
        setctty();
    }
#endif
}

void introff(void) /* disable kbd interrupts if required*/
{
#ifdef TTY_GRAPHICS
    /* Ugly hack to keep from changing tty modes for non-tty games -dlc */
    if (WINDOWPORT(tty) && curttyb2.intr_sym != nonesuch) {
        curttyb2.intr_sym = nonesuch;
        setctty();
    }
#endif
}

#ifdef _M_UNIX /* SCO UNIX (3.2.4), from Andreas Arens */
#include <sys/console.h>

#define BSIZE (E_TABSZ * 2)
#define LDIOC ('D' << 8) /* POSIX prevents definition */

#include <sys/emap.h>

int sco_flag_console = 0;
int sco_map_valid = -1;
unsigned char sco_chanmap_buf[BSIZE];

void sco_mapon(void);
void sco_mapoff(void);
void check_sco_console(void);
void init_sco_cons(void);

void
sco_mapon(void)
{
#ifdef TTY_GRAPHICS
    if (WINDOWPORT(tty) && sco_flag_console) {
        if (sco_map_valid != -1) {
            ioctl(0, LDSMAP, sco_chanmap_buf);
        }
        sco_map_valid = -1;
    }
#endif
}

void
sco_mapoff(void)
{
#ifdef TTY_GRAPHICS
    if (WINDOWPORT(tty) && sco_flag_console) {
        sco_map_valid = ioctl(0, LDGMAP, sco_chanmap_buf);
        if (sco_map_valid != -1) {
            ioctl(0, LDNMAP, (char *) 0);
        }
    }
#endif
}

void
check_sco_console(void)
{
    if (isatty(0) && ioctl(0, CONS_GET, 0) != -1) {
        sco_flag_console = 1;
    }
}

void
init_sco_cons(void)
{
#ifdef TTY_GRAPHICS
    if (WINDOWPORT(tty) && sco_flag_console) {
        atexit(sco_mapon);
        sco_mapoff();
        load_symset("IBMGraphics", PRIMARYSET);
        load_symset("RogueIBM", ROGUESET);
        switch_symbols(TRUE);
#ifdef TEXTCOLOR
        if (has_colors())
            iflags.use_color = TRUE;
#endif
    }
#endif
}
#endif /* _M_UNIX */

#ifdef __linux__ /* via Jesse Thilo and Ben Gertzfield */
#include <sys/vt.h>
#include <sys/ioctl.h>

int linux_flag_console = 0;

void linux_mapon(void);
void linux_mapoff(void);
void check_linux_console(void);
void init_linux_cons(void);

void
linux_mapon(void)
{
#ifdef TTY_GRAPHICS
    if (WINDOWPORT(tty) && linux_flag_console) {
        write(1, "\033(B", 3);
    }
#endif
}

void
linux_mapoff(void)
{
#ifdef TTY_GRAPHICS
    if (WINDOWPORT(tty) && linux_flag_console) {
        write(1, "\033(U", 3);
    }
#endif
}

void
check_linux_console(void)
{
    struct vt_mode vtm;

    if (isatty(0) && ioctl(0, VT_GETMODE, &vtm) >= 0) {
        linux_flag_console = 1;
    }
}

void
init_linux_cons(void)
{
#ifdef TTY_GRAPHICS
    if (WINDOWPORT(tty) && linux_flag_console) {
        atexit(linux_mapon);
        linux_mapoff();
#ifdef TEXTCOLOR
        if (has_colors())
            iflags.use_color = TRUE;
#endif
    }
#endif
}
#endif /* __linux__ */

DISABLE_WARNING_FORMAT_NONLITERAL

#ifndef __begui__ /* the Be GUI will define its own error proc */
/* fatal error */
void
error(const char *s, ...)
{
    va_list the_args;

    va_start(the_args, s);
    if (iflags.window_inited)
        exit_nhwindows((char *) 0); /* for tty, will call settty() */
    if (settty_needed)
        settty((char *) 0);
    Vprintf(s, the_args);
    (void) putchar('\n');
    va_end(the_args);
    exit(EXIT_FAILURE);
}
#endif /* !__begui__ */

RESTORE_WARNING_FORMAT_NONLITERAL

#ifdef ENHANCED_SYMBOLS
/*
 * set in tty_start_screen() and allows
 * OS-specific changes that may be
 * required for support of utf8.
 */
void
tty_utf8graphics_fixup(void)
{
}
#endif


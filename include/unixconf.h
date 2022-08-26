/* NetHack 3.7	unixconf.h	$NHDT-Date: 1607461111 2020/12/08 20:58:31 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.49 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef UNIX
#ifndef UNIXCONF_H
#define UNIXCONF_H

/*
 * Some include files are in a different place under SYSV
 *      BSD                SYSV
 * <sys/time.h>         <time.h>
 * <sgtty.h>            <termio.h>
 *
 * Some routines are called differently
 * index                strchr
 * rindex               strrchr
 *
 */

/* define exactly one of the following four choices */
/* #define BSD 1 */  /* define for 4.n/Free/Open/Net BSD  */
                     /* also for relatives like SunOS 4.x, DG/UX, and */
                     /* older versions of Linux */
/* #define ULTRIX */ /* define for Ultrix v3.0 or higher (but not lower) */
                     /* Use BSD for < v3.0 */
                     /* "ULTRIX" not to be confused with "ultrix" */
#define SYSV         /* define for System V, Solaris 2.x, newer versions */
                     /* of Linux */
/* #define HPUX */   /* Hewlett-Packard's Unix, version 6.5 or higher */
                     /* use SYSV for < v6.5 */

/* define any of the following that are appropriate */
#define SVR4           /* use in addition to SYSV for System V Release 4 */
                       /* including Solaris 2+ */
#define NETWORK        /* if running on a networked system */
                       /* e.g. Suns sharing a playground through NFS */
/* #define SUNOS4 */   /* SunOS 4.x */
#ifdef __linux__
#define LINUX    /* Another Unix clone */
#endif
/* #define CYGWIN32 */ /* Unix on Win32 -- use with case sensitive defines */
/* #define GENIX */    /* Yet Another Unix Clone */
/* #define HISX */     /* Bull Unix for XPS Machines */
/* #define BOS */      /* Bull Open Software - Unix for DPX/2 Machines */
/* #define UNIXPC */   /* use in addition to SYSV for AT&T 7300/3B1 */
/* #define AIX_31 */   /* In AIX 3.1 (IBM RS/6000) use BSD ioctl's to gain
                        * job control (note that AIX is SYSV otherwise)
                        * Also define this for AIX 3.2 */

#define TERMINFO       /* uses terminfo rather than termcap */
                       /* Should be defined for most SYSV, SVR4 (including
                        * Solaris 2+), HPUX, and Linux systems.  In
                        * particular, it should NOT be defined for the UNIXPC
                        * unless you remove the use of the shared library in
                        * the Makefile */
#define TEXTCOLOR      /* Use System V r3.2 terminfo color support
                        * and/or ANSI color support on termcap systems
                        * and/or X11 color.  Note:  if you get compiler
                        * warnings about 'has_colors()' being implicitly
                        * declared, uncomment NEED_HAS_COLORS_DECL below. */
#define POSIX_JOB_CONTROL /* use System V / Solaris 2.x / POSIX job control
                           * (e.g., VSUSP) */
#define POSIX_TYPES /* use POSIX types for system calls and termios */
                    /* Define for many recent OS releases, including
                     * those with specific defines (since types are
                     * changing toward the standard from earlier chaos).
                     * For example, platforms using the GNU libraries,
                     * Linux, Solaris 2.x
                     */

/* #define RANDOM */ /* if neither random/srandom nor lrand48/srand48
                        is available from your system */

/*
 * The next two defines are intended mainly for the Andrew File System,
 * which does not allow hard links.  If NO_FILE_LINKS is defined, lock files
 * will be created in LOCKDIR using open() instead of in the playground using
 * link().
 *              Ralf Brown, 7/26/89 (from v2.3 hack of 10/10/88)
 */

/* #define NO_FILE_LINKS */                       /* if no hard links */
/* #define LOCKDIR "/usr/games/lib/nethackdir" */ /* where to put locks */

/*
 * If you want the static parts of your playground on a read-only file
 * system, define VAR_PLAYGROUND to be where the variable parts are kept.
 */
/* #define VAR_PLAYGROUND "/var/lib/games/nethack" */

/*
 * Define DEF_PAGER as your default pager, e.g. "/bin/cat" or "/usr/ucb/more"
 * If defined, it can be overridden by the environment variable PAGER.
 * NetHack will use its internal pager if DEF_PAGER is not defined _or_
 * if DLB is defined since an external pager won't know how to access the
 * contents of the dlb container file.
 * (Note: leaving DEF_PAGER undefined is preferable for security reasons.)
 */
/* #define DEF_PAGER "/usr/bin/less" */

/*
 * Define PORT_HELP to be the name of the port-specfic help file.
 * This file is found in HACKDIR.
 * Normally, you shouldn't need to change this.
 * There is currently no port-specific help for Unix systems.
 */
/* #define PORT_HELP "Unixhelp" */

#ifdef TTY_GRAPHICS
/*
 * To enable the `timed_delay' option for using a timer rather than extra
 * screen output when pausing for display effect.  Requires that `msleep'
 * function be available (with time argument specified in milliseconds).
 * Various output devices can produce wildly varying delays when the
 * "extra output" method is used, but not all systems provide access to
 * a fine-grained timer.
 */
/* #define TIMED_DELAY */ /* usleep() */
#endif
#if defined(MACOS) && !defined(TIMED_DELAY)
#define TIMED_DELAY
#endif

/* #define AVOID_WIN_IOCTL */ /* ensure USE_WIN_IOCTL remains undefined */

/*
 * If you define MAIL, then the player will be notified of new mail
 * when it arrives.  If you also define DEF_MAILREADER then this will
 * be the default mail reader, and can be overridden by the environment
 * variable MAILREADER; otherwise an internal pager will be used.
 * A stat system call is done on the mailbox every MAILCKFREQ moves.
 */
#if !defined(NOMAIL)
#define MAIL /* Deliver mail during the game */
#endif

/* The Andrew Message System does mail a little differently from normal
 * UNIX.  Mail is deposited in the user's own directory in ~/Mailbox
 * (another directory).  MAILBOX is the element that will be added on to
 * the user's home directory path to generate the Mailbox path - just in
 * case other Andrew sites do it differently from CMU.
 *              dan lovinger
 *              dl2n+@andrew.cmu.edu (dec 19 1989)
 */

/* #define AMS */ /* use Andrew message system for mail */

/* NO_MAILREADER is for kerberos authenticating filesystems where it is
 * essentially impossible to securely exec child processes, like mail
 * readers, when the game is running under a special token.
 *              dan
 */

/* #define NO_MAILREADER */ /* have mail daemon just tell player of mail */

#ifdef MAIL
#if defined(BSD) || defined(ULTRIX)
#ifdef AMS
#define AMS_MAILBOX "/Mailbox"
#else
#if defined(__FreeBSD__) || defined(__OpenBSD__)
#define DEF_MAILREADER "/usr/bin/mail"
#else
#define DEF_MAILREADER "/usr/ucb/Mail"
#endif
#endif
#else
#if (defined(SYSV) || defined(DGUX) || defined(HPUX)) && !defined(LINUX)
#if defined(M_XENIX)
#define DEF_MAILREADER "/usr/bin/mail"
#else
#ifdef __sgi
#define DEF_MAILREADER "/usr/sbin/Mail"
#else
#define DEF_MAILREADER "/usr/bin/mailx"
#endif
#endif
#else
#define DEF_MAILREADER "/bin/mail"
#endif
#endif

/* If SIMPLE_MAIL is defined, the mail spool file format is
   "sender:message", one mail per line, and mails are
   read within game, from demon-delivered mail scrolls.
   The mail spool file will be deleted once the player
   has read the message. */
/* #define SIMPLE_MAIL */

#ifndef MAILCKFREQ
/* How often mail spool file is checked for new messages, in turns */
#define MAILCKFREQ 50
#endif

#endif /* MAIL */

/* If SERVER_ADMIN_MSG is defined and the file exists, players get
   a message from the user defined in the file.  The file format
   is "sender:message" all in one line. */
/* #define SERVER_ADMIN_MSG "adminmsg" */
#ifndef SERVER_ADMIN_MSG_CKFREQ
/* How often admin message file is checked for new messages, in turns */
#define SERVER_ADMIN_MSG_CKFREQ 25
#endif


/*
 * Some terminals or terminal emulators send two character sequence "ESC c"
 * when Alt+c is pressed.  The altmeta run-time option allows the user to
 * request that "ESC c" be treated as M-c.
 */
#define ALTMETA /* support altmeta run-time option */

#ifdef COMPRESS
/* Some implementations of compress need a 'quiet' option.
 * If you've got one of these versions, put -q here.
 * You can also include any other strange options your compress needs.
 * If you have a normal compress, just leave it commented out.
 */
/* #define COMPRESS_OPTIONS "-q" */
#endif

#ifndef FCMASK
#define FCMASK 0660 /* file creation mask */
#endif

/* fcntl(2) is a POSIX-portable call for manipulating file descriptors.
 * Comment out the USE_FCNTL if for some reason you have a strange
 * OS/filesystem combination for which fcntl(2) does not work. */
#ifdef POSIX_TYPES
#define USE_FCNTL
#endif

/*
 * The remainder of the file should not need to be changed.
 */

#ifdef _AUX_SOURCE
#ifdef AUX /* gcc ? */
#define _SYSV_SOURCE
#define _BSD_SOURCE
#else
#define AUX
#endif
#endif /* _AUX_SOURCE */

#if defined(LINUX) || defined(bsdi)
#ifndef POSIX_TYPES
#define POSIX_TYPES
#endif
#ifndef POSIX_JOB_CONTROL
#define POSIX_JOB_CONTROL
#endif
#endif

/*
 * BSD/ULTRIX systems are normally the only ones that can suspend processes.
 * Suspending NetHack processes cleanly should be easy to add to other systems
 * that have SIGTSTP in the Berkeley sense.  Currently the only such systems
 * known to work are HPUX and AIX 3.1; other systems will probably require
 * tweaks to unixtty.c and ioctl.c.
 *
 * POSIX defines a slightly different type of job control, which should be
 * equivalent for NetHack's purposes.  POSIX_JOB_CONTROL should work on
 * various recent SYSV versions (with possibly tweaks to unixtty.c again).
 */
#ifndef POSIX_JOB_CONTROL
#if defined(BSD) || defined(ULTRIX) || defined(HPUX) || defined(AIX_31)
#define BSD_JOB_CONTROL
#else
#if defined(SVR4)
#define POSIX_JOB_CONTROL
#endif
#endif
#endif

#ifndef NOSUSPEND
#if defined(BSD_JOB_CONTROL) || defined(POSIX_JOB_CONTROL) || defined(AUX)
#define SUSPEND /* let ^Z suspend the game (push to background) */
#endif
#endif

/*
 * Define SAFERHANGUP to delay hangup processing until the main command
 * loop. 'safer' because it avoids certain cheats and also avoids losing
 * objects being thrown when the hangup occurs.  All unix windowports
 * support SAFERHANGUP (couldn't define it here otherwise).
 */
#define SAFERHANGUP

#if defined(BSD) || defined(ULTRIX)
#include <sys/time.h>
#else
#include <time.h>
#endif

/* these might be needed for include/system.h;
   default comes from system's time.h */
/* #define NEED_TIME_DECL 1 */
/* #define NEED_LOCALTIME_DECL 1 */
/* #define NEED_CTIME_DECL 1 */
/* these might be needed for src/hacklib.c;
   default is 'time_t *' */
/* #define TIME_type long * */
/* #define LOCALTIME_type long * */

#define HLOCK "perm" /* an empty file used for locking purposes */

#define tgetch getchar

#ifndef NOSHELL
#define SHELL /* do not delete the '!' command */
#endif

#include "system.h"

#if defined(POSIX_TYPES) || defined(__GNUC__)
#include <stdlib.h>
#include <unistd.h>
#endif

#if defined(POSIX_TYPES) || defined(__GNUC__) || defined(BSD) \
    || defined(ULTRIX)
#include <sys/wait.h>
#endif

#if defined(BSD) || defined(ULTRIX)
#if !defined(DGUX) && !defined(SUNOS4)
#define memcpy(d, s, n) bcopy(s, d, n)
#define memcmp(s1, s2, n) bcmp(s2, s1, n)
#endif
#ifdef SUNOS4
#include <memory.h>
#endif
#else         /* therefore SYSV */
#ifndef index /* some systems seem to do this for you */
#define index strchr
#endif
#ifndef rindex
#define rindex strrchr
#endif
#endif

/* Use the high quality random number routines. */
/* the high quality random number routines */
#ifndef USE_ISAAC64
# if defined(BSD) || defined(LINUX) || defined(ULTRIX) || defined(CYGWIN32) \
    || defined(RANDOM) || defined(MACOS)
#  define Rand() random()
# else
#  define Rand() lrand48()
# endif
#endif

#ifdef TIMED_DELAY
#if defined(SUNOS4) || defined(LINUX) || (defined(BSD) && !defined(ULTRIX))
#define msleep(k) usleep((k) *1000)
#endif
#ifdef ULTRIX
#define msleep(k) napms(k)
#endif
#endif

/* Relevant for some systems which enable TEXTCOLOR:  some older versions
   of curses (the run-time library optionally used by nethack's tty
   interface in addition to its curses interface) supply 'has_colors()'
   but corresponding <curses.h> doesn't declare it.  has_colors() is used
   in unixtty.c by init_sco_cons() and init_linux_cons().  If the compiler
   complains about has_colors() not being declared, try uncommenting
   NEED_HAS_COLORS_DECL.  If the linker complains about has_colors not
   being found, switch to ncurses() or update to newer version of curses. */
/* #define NEED_HAS_COLORS_DECL */

#ifdef hc /* older versions of the MetaWare High-C compiler define this */
#ifdef __HC__
#undef __HC__
#endif
#define __HC__ hc
#undef hc
#endif

#if defined(GNOME_GRAPHICS)
#if defined(LINUX)
#include <linux/unistd.h>
#if defined(__NR_getresuid) && defined(__NR_getresgid) /* ie., >= v2.1.44 */
#define GETRES_SUPPORT
#endif
#else
#if defined(BSD) || defined(SVR4)
/*
 * [ALI] We assume that SVR4 means we can safely include syscall.h
 * (although it's really a BSDism). This is certainly true for Solaris 2.5,
 * Solaris 7, Solaris 8 and Compaq Tru64 5.1
 * Later BSD systems will have the getresid system calls.
 */
#include <sys/syscall.h>
#if (defined(SYS_getuid) || defined(SYS_getresuid)) \
    && (defined(SYS_getgid) || defined(SYS_getresgid))
#define GETRES_SUPPORT
#endif
#endif /* BSD || SVR4 */
#endif /* LINUX */
#endif /* GNOME_GRAPHICS */

#if defined(MACOS) && !defined(LIBNH)
# define RUNTIME_PASTEBUF_SUPPORT
#endif

/*
 * /dev/random is blocking on Linux, so there we default to /dev/urandom which
 * should still be good enough.
 * BSD systems usually have /dev/random that is supposed to be used.
 * OSX is based on NetBSD kernel and has both /dev/random and /dev/urandom.
 */
#ifdef LINUX
# define DEV_RANDOM "/dev/urandom"
#else
# if defined(BSD) || defined(MACOS)
#  define DEV_RANDOM "/dev/random"
# endif
#endif

#endif /* UNIXCONF_H */
#endif /* UNIX */

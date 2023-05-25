/* NetHack 3.7	vmsconf.h	$NHDT-Date: 1596498569 2020/08/03 23:49:29 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.33 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef VMS
#ifndef VMSCONF_H
#define VMSCONF_H

/*
 * Edit these to choose values appropriate for your site.
 * WIZARD is the username allowed to use the debug option of nethack; no harm
 *   is done by leaving it as a username that doesn't exist at your site.
 * HACKDIR can be overridden at run-time with the logical name HACKDIR, as in
 *   $ define hackdir disk$users:[games.nethack]
 * Trailing NULs are present in the default values in order to make some
 *   extra room for patching longer values into an existing executable.
 */
#define Local_WIZARD "NHWIZARD\0\0\0\0"
#define Local_HACKDIR "DISK$USERS:[GAMES.NETHACK.3_7_X.PLAY]\0\0\0\0\0\0\0\0"

/*
 * This section cleans up the stuff done in config.h so that it
 * shouldn't need to be modified.  It's conservative so that if
 * config.h is actually edited, the changes won't impact us.
 */
#ifdef UNIX
#undef UNIX
#endif
#ifdef HACKDIR
#undef HACKDIR
#endif
#ifdef WIZARD_NAME
#undef WIZARD_NAME
#endif
#define HACKDIR Local_HACKDIR
#define WIZARD_NAME Local_WIZARD
#ifndef SYSCF
#define SYSCF
#endif

/* filenames require punctuation to avoid redirection via logical names */
#undef RECORD
#define RECORD "record;1" /* scoreboard file (retains high scores) */
#undef LOGFILE
#define LOGFILE "logfile;0" /* optional file (records all games) */
#undef SYSCF_FILE
#define SYSCF_FILE "sysconf;0"

#define HLOCK "perm;1" /* an empty file used for locking purposes */

/* want compression--for level & save files--performed within NetHack itself
 */
#ifdef COMPRESS
#undef COMPRESS
#endif
#ifndef INTERNAL_COMP
#define INTERNAL_COMP
#endif

/*
 * If nethack.exe will be installed with privilege so that the playground
 * won't need to be left unprotected, define SECURE to suppress a couple
 * of file protection fixups (protection of bones files and ownership of
 * save files).
 */
/* #define SECURE */

/*
 * If you use SECURE you'll need to link /noTraceback, in which case
 * there's no point trying to get extra PANICTRACE info and this might
 * as well be commented out.  When enabled, the sysconf file controls
 * how to handle it (note that we're hijacking the Unix GDB setting):
PANICTRACE_GDB=0  #behave as if PANICTRACE was disabled
PANICTRACE_GDB=1  #at conclusion of panic, show a call traceback and exit
PANICTRACE_GDB=2  #at conclusion of panic, show a call traceback and then
 *                # remain in the debugger for more interactive debugging
 *                # (not as useful as it might sound since we're normally
 *                # linked /noDebug so there's no symbol table accessible)
 */
#define PANICTRACE

/*
 * Put the readonly data files into a single container rather than into
 * separate files in the playground directory.
 */
#define DLB /* use data librarian code */

/*
 * Provide menu of saved games to choose from at start.
 * [Player needs to use ``nethack "-ugames"'' for this to work.]
 */
#define SELECTSAVED

/*
 * You may define TEXTCOLOR if your system has any terminals that recognize
 * ANSI color sequences of the form ``<ESCAPE>[#;#m'', where the first # is
 * a number between 40 and 47 represented background color, and the second
 * # is a number between 30 and 37 representing the foreground color.
 * GIGI terminals and DECterm windows on color VAXstations support these
 * color escape sequences, as do some 3rd party terminals and many micro
 * computers.
 */
/* #define TEXTCOLOR */

/*
 * If you define USE_QIO_INPUT, then you'll get raw characters from the
 * keyboard, not unlike those of the unix version of Nethack.  This will
 * allow you to use the Escape key in normal gameplay, and the appropriate
 * control characters in Wizard mode.  It will work most like the unix
 * version.
 * It will also avoid "<interrupt>" being displayed when ^Y is pressed.
 *
 * Otherwise, the VMS SMG calls will be used.  These calls block use of
 * the escape key, as well as certain control keys, so gameplay is not
 * the same, although the differences are fairly negligible.  You must
 * then use a VTxxx function key or two <escape>s to give an ESC response.
 */
#define USE_QIO_INPUT /* use SYS$QIOW instead of SMG$READ_KEYSTROKE */

/*
 * Allow the user to decide whether to pause via timer or excess screen
 * output for various display effects like explosions and moving objects.
 */
#define TIMED_DELAY /* enable the `timed_delay' run-time option */

/*
 * If you define MAIL, then NetHack will capture incoming broadcast
 * messages such as "New mail from so-and-so" and "Print job completed,"
 * and then deliver them to the player.  For mail and phone broadcasts
 * a scroll of mail will be created, which when read will cause NetHack
 * to prompt the player for a command to spawn in order to respond.  The
 * latter capability will not be available if SHELL is disabled below.
 * If you undefine MAIL, broadcasts will go straight to the terminal,
 * resulting in disruption of the screen display; use <ctrl/R> to redraw.
 */
#define MAIL /* enable broadcast trapping */

/*
 * SHELL enables the player to 'escape' into a spawned subprocess via
 * the '!' command.  Logout or attach back to the parent to resume play.
 * If the player attaches back to NetHack, then a subsequent escape will
 * re-attach to the existing subprocess.  Any such subprocess left over
 * at game exit will be deleted by an exit handler.
 * SUSPEND enables someone running NetHack in a subprocess to reconnect
 * to the parent process with the <ctrl/Z> command; this is not very
 * close to Unix job control, but it's better than nothing.
 */
#define SHELL   /* do not delete the '!' command */
#define SUSPEND /* don't delete the ^Z command, such as it is */

/*
 * Some terminals or terminal emulators send two character sequence "ESC c"
 * when Alt+c is pressed.  The altmeta run-time option allows the user to
 * request that "ESC c" be treated as M-c, which means that if nethack sees
 * ESC when it is waiting for a command, it will wait for another character
 * (even if user intended that ESC to be standalone to cancel a count prefix).
 */
#define ALTMETA /* support altmeta run-time option */

#define RANDOM /* use sys/share/random.c instead of vaxcrtl rand */

/* config.h defines USE_ISAAC64; we'll use it on Alpha or IA64 but not VAX;
   it overrides RANDOM */
#if !defined(VMSVSI)
#if (defined(VAX) || defined(vax) || defined(__vax)) && defined(USE_ISAAC64)
#undef ISAAC64
#endif
#endif

#define FCMASK 0660 /* file creation mask */

/*
 * The remainder of the file should not need to be changed.
 */

/* This used to be force-defined for VMS in topten.c, but with
 * the global variable consolidation into g in 3.7, it has to be
 * defined here so that decl.h includes the field in g.
 */
#define UPDATE_RECORD_IN_PLACE

/* data librarian defs */
#ifdef DLB
#define DLBFILE "nh-data.dlb"
/*
 * Since we can do without case insensitive filename comparison,
 * avoid enabling it because that requires compiling and linking
 * src/hacklib into util/dlb_main.
 */
/* # define FILENAME_CMP strcmpi */ /* case insensitive */
#endif

#ifndef VMSVSI
#if defined(VAXC) && !defined(ANCIENT_VAXC)
#ifdef volatile
#undef volatile
#endif
#ifdef const
#undef const
#endif
#endif

#ifdef __DECC
#define STRICT_REF_DEF /* used in lev_main.c */
#endif
#ifdef STRICT_REF_DEF
#define DEFINE_OSPEED
#endif

#ifndef alloca
/* bison generated foo_yacc.c might try to use alloca() */
#ifdef __GNUC__
#define alloca __builtin_alloca
#else
#define ALLOCA_HACK /* used in util/panic.c */
#endif
#endif
#endif /* !VMSVSI */

#ifdef VMSVSI
#define NO_TERMCAP_HEADERS
/* C99 */
#include <types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <stat.h>
#include <errno.h>
#include <stsdef.h>
#endif

#ifdef _DECC_V4_SOURCE
/* <types.h> excludes some necessary typedefs when _DECC_V4_SOURCE is defined
 */
#include <types.h>
#ifndef __PID_T
#define __PID_T
typedef __pid_t pid_t;
#endif
#ifndef __UID_T
#define __UID_T
typedef __uid_t uid_t;
#endif
#ifndef __GID_T
#define __GID_T
typedef __gid_t gid_t;
#endif
#ifndef __MODE_T
#define __MODE_T
typedef __mode_t mode_t;
#endif
#ifndef __OFF_T
#define __OFF_T
typedef int32_t off_t;
#endif
#endif /* _DECC_V4_SOURCE */

#include <time.h>

#ifndef VMSVSI
#if 0 /* <file.h> is missing for old gcc versions; skip it to save time */
#include <file.h>
#else /* values needed from missing include file */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0x200
#define O_TRUNC 0x400
#endif
#endif

#define tgetch vms_getchar

#ifndef VMSVSI
#if defined(__DECC_VER) && (__DECC_VER >= 50000000)
 /* for cc/Standard=ANSI89, suppress notification that '$' in identifiers
    is an extension; sys/vms/*.c needs it regardless of strict ANSI mode */
# pragma message disable DOLLARID
#endif
#endif

/* #include "system.h" */

/* Use the high quality random number routines. */
#ifndef USE_ISAAC64
# if defined(RANDOM)
#  define Rand() random()
/* VMS V7 adds these entry points to DECC$SHR; stick with the nethack-supplied
   code to avoid having to deal with version-specific conditionalized builds */
#  define random nh_random
#  define srandom nh_srandom
#  define initstate nh_initstate
#  define setstate nh_setstate
# else
#  define Rand() rand()
# endif
#endif

#if !defined(VMSVSI)
#ifndef __GNUC__
#ifndef bcopy
#define bcopy(s, d, n) memcpy((d), (s), (n)) /* vaxcrtl */
#endif
#endif
#define abort() vms_abort()             /* vmsmisc.c */
#define creat(f, m) vms_creat(f, m)     /* vmsfiles.c */
#define exit(sts) vms_exit(sts)         /* vmsmisc.c */
#define getuid() vms_getuid()           /* vmsunix.c */
#define link(f1, f2) vms_link(f1, f2)   /* vmsfiles.c */
#define open(f, k, m) vms_open(f, k, m) /* vmsfiles.c */
#define fopen(f, m) vms_fopen(f, m)     /* vmsfiles.c */
/* #define unlink(f0) vms_unlink(f0)       /* vmsfiles.c */
#ifdef VERYOLD_VMS
#define unlink(f0) delete (f0) /* vaxcrtl */
#else
#define unlink(f0) remove(f0) /* vaxcrtl, decc$shr */
#endif
#endif /* VMSVSI */

#define C$$TRANSLATE(n) c__translate(n) /* vmsfiles.c */

#if !defined(VMSVSI)
/* VMS global names are case insensitive... */
#define An vms_an
#define The vms_the
#define Shk_Your vms_shk_your
#endif /* VMSVSI */

/* avoid global symbol in Alpha/VMS V1.5 STARLET library (link trouble) */
#define ospeed vms_ospeed

/* used in several files which don't #include "extern.h" */
extern void vms_exit(int);
extern int vms_open(const char *, int, unsigned);
extern FILE *vms_fopen(const char *, const char *);
char *vms_basename(const char *, boolean); /* vmsfiles.c */

#endif /* VMSCONF_H */
#endif /* VMS */

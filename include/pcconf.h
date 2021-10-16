/* NetHack 3.7	pcconf.h	$NHDT-Date: 1596498554 2020/08/03 23:49:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.28 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef PCCONF_H
#define PCCONF_H

#define MICRO /* always define this! */

#ifdef MSDOS /* some of this material is MS-DOS specific */

/*
 *  Automatic Defines:
 *
 *     __GO32__ is defined automatically by the djgpp port of gcc.
 *     __DJGPP__ is defined automatically by djgpp version 2 and above.
 *     _MSC_VER is defined automatically by Microsoft C.
 *     __BORLANDC__ is defined automatically by Borland C.
 *     __SC__ is defined automatically by Symantec C.
 *	Note: 3.6.x was not verified with Symantec C.
 */

#define CONFIG_FILE "defaults.nh"
#define GUIDEBOOK_FILE "Guidebook.txt"

/*
 *  The following options are somewhat configurable depending on
 *  your compiler.
 */

/*
 *  For pre-V7.0 Microsoft Compilers only, manually define OVERLAY here.
 */

/*#define OVERLAY */ /* Manual overlay definition (MSC 6.0ax only) */

#ifndef CROSS_TO_AMIGA
#define SHELL /* via exec of COMMAND.COM */
#endif

/*
 * Screen control options
 *
 * You may uncomment:
 *		       ANSI_DEFAULT
 *		  or   TERMLIB
 *		  or   ANSI_DEFAULT and TERMLIB
 *		  or   NO_TERMS
 */

/* # define TERMLIB */ /* enable use of termcap file /etc/termcap */
                       /* or ./termcap for MSDOS (SAC) */
                       /* compile and link in Fred Fish's termcap library, */
                       /* enclosed in TERMCAP.ARC, to use this */

/* # define ANSI_DEFAULT */ /* allows NetHack to run without a ./termcap */

#define NO_TERMS /* Allows Nethack to run without ansi.sys by linking */
                 /* screen routines into the .exe     */

#ifdef NO_TERMS     /* if NO_TERMS select one screen package below */
#define SCREEN_BIOS /* Use bios calls for all screen control */
/* #define SCREEN_DJGPPFAST */ /* Use djgpp fast screen routines	*/
#endif

/* # define PC9800 */ /* Allows NetHack to run on NEC PC-9800 machines */
/* Yamamoto Keizo */

/*
 * PC video hardware support options (for graphical tile support)
 *
 * You may uncomment any/all of the options below.
 *
 */
#ifndef SUPPRESS_GRAPHICS
#if (defined(SCREEN_BIOS) || defined(SCREEN_DJGPPFAST)) && !defined(PC9800)
#ifdef USE_TILES
#define SCREEN_VGA /* Include VGA	  graphics routines in the build */
#define SCREEN_VESA
#endif
#endif
#else
#undef NO_TERMS
#undef SCREEN_BIOS
#undef SCREEN_DJGPPFAST
#undef SCREEN_VGA
#undef SCREEN_VESA
#undef TERMLIB
#define ANSI_DEFAULT
#endif

#ifndef CROSS_TO_AMIGA
#define RANDOM /* have Berkeley random(3) */
#endif

#define MAIL /* Allows for fake mail daemon to deliver mail */
             /* in the MSDOS version.  (For AMIGA MAIL see  */
             /* amiconf.h).	In the future this will be the */
             /* hook for mail reader implementation.        */

/* The following is needed for prototypes of certain functions */

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__SC__)
#include <process.h> /* Provides prototypes of exit(), spawn()      */
#endif

#ifdef CROSS_TO_AMIGA
#include <spawn.h>
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 7)
#include <sys/types.h>
#include <stdlib.h>
#ifdef strcmpi
#undef strcmpi
#endif
#include <string.h> /* Provides prototypes of strncmpi(), etc.     */
#include <conio.h>
#include <io.h>
#include <direct.h>
#define SIG_RET_TYPE void(__cdecl *)(int)
#define M(c) ((char) (0x80 | (c)))
#define vprintf printf
#define vfprintf fprintf
#define vsprintf sprintf
#endif

#if defined(__BORLANDC__) && defined(STRNCMPI)
#include <string.h> /* Provides prototypes of strncmpi(), etc.     */
#endif

#if defined(__DJGPP__)
#define _NAIVE_DOS_REGS
#include <stdlib.h>
#include <string.h> /* Provides prototypes of strncmpi(), etc.     */
#ifndef M
#define M(c) ((char) (0x80 | (c)))
#endif
#endif

/*
 * On the VMS and unix, this option controls whether a delay is done by
 * the clock, or whether it is done by excess output.  On the PC, however,
 * there is always a clock to use for the delay.  The TIMED_DELAY option
 * on MSDOS (without the termcap routines) is used to determine whether to
 * include the delay routines in the code (and thus, provides a compile time
 * method to turn off napping for visual effect).  However, it is also used
 * in the music code to wait between different notes.  So it is needed in that
 * case as well.

 * Whereas on the VMS and unix, flags.nap is a run-time option controlling
 * whether there is a delay by clock or by excess output, on MSDOS it is
 * simply a flag to turn on or off napping for visual effects at run-time.
 */

#define TIMED_DELAY /* enable the `timed_delay' run-time option */

#define NOCWD_ASSUMPTIONS /* Allow paths to be specified for HACKDIR,      \
                             LEVELDIR, SAVEDIR, BONESDIR, DATADIR,         \
                             SCOREDIR, LOCKDIR, CONFIGDIR, and TROUBLEDIR. \
                             */

#endif /* MSDOS configuration stuff */

#ifndef PATHLEN
#define PATHLEN 64  /* maximum pathlength */
#endif
#define FILENAME 80 /* maximum filename length (conservative) */
#ifndef MICRO_H
#include "micro.h" /* contains necessary externs for [os_name].c */
#endif

/* ===================================================
 *  The remaining code shouldn't need modification.
 */

#ifndef SYSTEM_H
#if !defined(_MSC_VER)
#include "system.h"
#endif
#endif

#ifdef __DJGPP__
#include <unistd.h> /* close(), etc. */
/* lock() in io.h interferes with lock[] in decl.h */
#define lock djlock
#include <io.h>
#undef lock
#include <pc.h> /* kbhit() */
#define PC_LOCKING
#define SELF_RECOVER /* NetHack itself can recover games */
#endif

#ifdef MSDOS
#ifndef EXEPATH
#define EXEPATH /* HACKDIR is .exe location if not explicitly defined */
#endif
#endif

#if defined(_MSC_VER) && defined(MSDOS)
#if (_MSC_VER >= 700) && !defined(FUNCTION_LEVEL_LINKING)
#ifndef MOVERLAY
#define MOVERLAY /* Microsoft's MOVE overlay system (MSC >= 7.0) */
#endif
#endif
#define PC_LOCKING
#endif

/* Borland Stuff */
#if defined(__BORLANDC__)
#if defined(__OVERLAY__) && !defined(VROOMM)
/* __OVERLAY__ is automatically defined by Borland C if overlay option is on
 */
#define VROOMM /* Borland's VROOMM overlay system */
#endif
#if !defined(STKSIZ)
#define STKSIZ 5 * 1024 /* Use a default of 5K stack for Borland C	*/
                        /* This macro is used in any file that contains */
                        /* a main() function.				*/
#endif
#define PC_LOCKING
#endif

#ifdef PC_LOCKING
#define HLOCK "NHPERM"
#endif

#ifndef index
#define index strchr
#endif
#ifndef rindex
#define rindex strrchr
#endif

#ifndef AMIGA
#include <time.h>
#endif

/* the high quality random number routines */
#ifndef USE_ISAAC64
# ifdef RANDOM
#  define Rand() random()
# else
#  define Rand() rand()
# endif
#endif

#ifndef TOS
#define FCMASK 0660 /* file creation mask */
#endif

#include <fcntl.h>

#ifdef MSDOS
#define TEXTCOLOR                /* */
#define PORT_HELP "msdoshlp.txt" /* msdos port specific help file */
#endif

/* Sanity check, do not modify these blocks. */

#if defined(MSDOS) && defined(NO_TERMS)
#ifdef TERMLIB
#if defined(_MSC_VER) || defined(__SC__)
#pragma message("Warning -- TERMLIB defined with NO_TERMS in pcconf.h")
#pragma message("           Forcing undef of TERMLIB")
#endif
#undef TERMLIB
#endif
#ifdef ANSI_DEFAULT
#if defined(_MSC_VER) || defined(__SC__)
#pragma message("Warning -- ANSI_DEFAULT defined with NO_TERMS in pcconf.h")
#pragma message("           Forcing undef of ANSI_DEFAULT")
#endif
#undef ANSI_DEFAULT
#endif
/* only one screen package is allowed */
#if defined(SCREEN_BIOS) && defined(SCREEN_DJGPPFAST)
#if defined(_MSC_VER) || defined(__SC__)
#pragma message("Warning -- More than one screen package defined in pcconf.h")
#endif
#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__SC__)
#if defined(SCREEN_DJGPPFAST)
#if defined(_MSC_VER) || defined(__SC__)
#pragma message("           Forcing undef of SCREEN_DJGPPFAST")
#endif
#undef SCREEN_DJGPPFAST /* Can't use djgpp fast with other compilers anyway \
                           */
#endif
#else
/* djgpp C compiler	*/
#if defined(SCREEN_BIOS)
#undef SCREEN_BIOS
#endif
#endif
#endif
#define ASCIIGRAPH
#ifdef TEXTCOLOR
#define VIDEOSHADES
#endif
/* SCREEN_8514, SCREEN_VESA are only placeholders presently - sub VGA instead
 */
#if defined(SCREEN_8514)
#undef SCREEN_8514
#define SCREEN_VGA
#endif
/* Graphical tile sanity checks */
#ifdef SCREEN_VGA
#define SIMULATE_CURSOR
#define POSITIONBAR
/* Select appropriate tile file format, and map size */
#define PLANAR_FILE
#define SMALL_MAP
#endif
#endif /* End of sanity check block */

#if defined(MSDOS) && defined(DLB)
#define FILENAME_CMP stricmp /* case insensitive */
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 7)
#pragma warning(disable : 4131)
#pragma warning(disable : 4135)
#pragma warning(disable : 4309)
#pragma warning(disable : 4746)
#pragma warning(disable : 4761)
#endif

#ifdef TIMED_DELAY
#ifdef __DJGPP__
#define msleep(k) (void) usleep((k) *1000)
#endif
#ifdef __BORLANDC__
#define msleep(k) delay(k)
#endif
#ifdef __SC__
#define msleep(k) (void) usleep((long)((k) *1000))
#endif
#endif

#endif /* PCCONF_H */

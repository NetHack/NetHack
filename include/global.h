/* NetHack 3.7	global.h	$NHDT-Date: 1594032649 2020/07/06 10:50:49 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.104 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>

/*
 * Files expected to exist in the playground directory.
 */

#define RECORD "record"         /* file containing list of topscorers */
#define HELP "help"             /* file containing command descriptions */
#define SHELP "hh"              /* abbreviated form of the same */
#define KEYHELP "keyhelp"       /* explanatory text for 'whatdoes' command */
#define DEBUGHELP "wizhelp"     /* file containing debug mode cmds */
#define RUMORFILE "rumors"      /* file with fortune cookies */
#define ORACLEFILE "oracles"    /* file with oracular information */
#define DATAFILE "data"         /* file giving the meaning of symbols used */
#define CMDHELPFILE "cmdhelp"   /* file telling what commands do */
#define HISTORY "history"       /* file giving nethack's history */
#define LICENSE "license"       /* file with license information */
#define OPTIONFILE "opthelp"    /* file explaining runtime options */
#define OPTIONS_USED "options"  /* compile-time options, for #version */
#define SYMBOLS "symbols"       /* replacement symbol sets */
#define EPITAPHFILE "epitaph"   /* random epitaphs on graves */
#define ENGRAVEFILE "engrave"   /* random engravings on the floor */
#define BOGUSMONFILE "bogusmon" /* hallucinatory monsters */
#define TRIBUTEFILE "tribute"   /* 3.6 tribute to Terry Pratchett */
#define LEV_EXT ".lua"          /* extension for special level files */

/* Assorted definitions that may depend on selections in config.h. */

/*
 * for DUMB preprocessor and compiler, e.g., cpp and pcc supplied
 * with Microport SysV/AT, which have small symbol tables;
 * DUMB if needed is defined in CFLAGS
 */
#ifdef DUMB
#ifdef BITFIELDS
#undef BITFIELDS
#endif
#ifndef STUPID
#define STUPID
#endif
#endif /* DUMB */

/*
 * type xchar: small integers in the range 0 - 127, usually coordinates
 * although they are nonnegative they must not be declared unsigned
 * since otherwise comparisons with signed quantities are done incorrectly
 */
typedef schar xchar;

#ifdef __MINGW32__
/* Resolve conflict with Qt 5 and MinGW-w32 */
typedef unsigned char boolean; /* 0 or 1 */
#else
#ifndef SKIP_BOOLEAN
typedef xchar boolean; /* 0 or 1 */
#endif
#endif

#ifndef TRUE /* defined in some systems' native include files */
#define TRUE ((boolean) 1)
#define FALSE ((boolean) 0)
#endif

enum optchoice { opt_in, opt_out};

/*
 * type nhsym: loadable symbols go into this type
 */
typedef uchar nhsym;

#ifndef STRNCMPI
#ifndef __SASC_60 /* SAS/C already shifts to stricmp */
#define strcmpi(a, b) strncmpi((a), (b), -1)
#endif
#endif

#if 0
/* comment out to test effects of each #define -- these will probably
 * disappear eventually
 */
#ifdef INTERNAL_COMP
#define RLECOMP  /* run-length compression of levl array - JLee */
#define ZEROCOMP /* zero-run compression of everything - Olaf Seibert */
#endif
#endif

/* #define SPECIALIZATION */ /* do "specialized" version of new topology */

#ifdef BITFIELDS
#define Bitfield(x, n) unsigned x : n
#else
#define Bitfield(x, n) uchar x
#endif

#define SIZE(x) (int)(sizeof(x) / sizeof(x[0]))

/* A limit for some NetHack int variables.  It need not, and for comparable
 * scoring should not, depend on the actual limit on integers for a
 * particular machine, although it is set to the minimum required maximum
 * signed integer for C (2^15 -1).
 */
#define LARGEST_INT 32767

#include "coord.h"

#if defined(CROSSCOMPILE)
struct cross_target_s {
    const char *build_date;
    const char *copyright_banner_c;
    const char *git_sha;
    const char *git_branch;
    const char *version_string;
    const char *version_id;
    unsigned long version_number;
    unsigned long version_features;
    unsigned long ignored_features;
    unsigned long version_sanity1;
    unsigned long version_sanity2;
    unsigned long version_sanity3;
    unsigned long build_time;
};
extern struct cross_target_s cross_target;
#if defined(CROSSCOMPILE_TARGET) && !defined(MAKEDEFS_C)
#define BUILD_DATE cross_target.build_date        /* "Wed Apr 1 00:00:01 2020" */
#define COPYRIGHT_BANNER_C cross_target.copyright_banner_c
#define NETHACK_GIT_SHA cross_target.git_sha
#define NETHACK_GIT_BRANCH cross_target.git_branch
#define VERSION_ID cross_target.version_id
#define IGNORED_FEATURES cross_target.ignored_features
#define VERSION_FEATURES cross_target.version_features
#define VERSION_NUMBER cross_target.version_number
#define VERSION_SANITY1 cross_target.version_sanity1
#define VERSION_SANITY2 cross_target.version_sanity2
#define VERSION_SANITY3 cross_target.version_sanity3
#define VERSION_STRING cross_target.version_string
#define BUILD_TIME cross_target.build_time        /* (1574157640UL) */
#endif /* CROSSCOMPILE_TARGET && !MAKEDEFS_C */
#endif /* CROSSCOMPILE */

/*
 * Automatic inclusions for the subsidiary files.
 * Please don't change the order.  It does matter.
 */

#ifdef VMS
#include "vmsconf.h"
#endif

#ifdef UNIX
#include "unixconf.h"
#endif

#ifdef MSDOS
#include "pcconf.h"
#endif

#ifdef WIN32
#include "ntconf.h"
#endif

/* Displayable name of this port; don't redefine if defined in *conf.h */
#ifndef PORT_ID
#ifdef AMIGA
#define PORT_ID "Amiga"
#endif
#ifdef MAC
#define PORT_ID "Mac"
#endif
#ifdef __APPLE__
#define PORT_ID "MacOSX"
#endif
#ifdef MSDOS
#ifdef PC9800
#define PORT_ID "PC-9800"
#else
#define PORT_ID "PC"
#endif
#ifdef DJGPP
#define PORT_SUB_ID "djgpp"
#else
#ifdef OVERLAY
#define PORT_SUB_ID "overlaid"
#else
#define PORT_SUB_ID "non-overlaid"
#endif
#endif
#endif
#ifdef OS2
#define PORT_ID "OS/2"
#endif
#ifdef TOS
#define PORT_ID "ST"
#endif
/* Check again in case something more specific has been defined above. */
#ifndef PORT_ID
#ifdef UNIX
#define PORT_ID "Unix"
#endif
#endif
#ifdef VMS
#define PORT_ID "VMS"
#endif
#ifdef WIN32
#define PORT_ID "Windows"
#endif
#endif

#if defined(MICRO)
#if !defined(AMIGA) && !defined(TOS) && !defined(OS2_HPFS)
#define SHORT_FILENAMES /* filenames are 8.3 */
#endif
#endif

#ifdef VMS
/* vms_exit() (sys/vms/vmsmisc.c) expects the non-VMS EXIT_xxx values below.
 * these definitions allow all systems to be treated uniformly, provided
 * main() routines do not terminate with return(), whose value is not
 * so massaged.
 */
#ifdef EXIT_SUCCESS
#undef EXIT_SUCCESS
#endif
#ifdef EXIT_FAILURE
#undef EXIT_FAILURE
#endif
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#if defined(X11_GRAPHICS) || defined(QT_GRAPHICS) || defined(GNOME_GRAPHICS) \
    || defined(WIN32)
#ifndef USE_TILES
#define USE_TILES /* glyph2tile[] will be available */
#endif
#endif
#if defined(AMII_GRAPHICS) || defined(GEM_GRAPHICS)
#ifndef USE_TILES
#define USE_TILES
#endif
#endif

#if defined(UNIX) || defined(VMS) || defined(__EMX__) || defined(WIN32)
#define HANGUPHANDLING
#endif
#if defined(SAFERHANGUP) \
    && (defined(NOSAVEONHANGUP) || !defined(HANGUPHANDLING))
#undef SAFERHANGUP
#endif

#define Sprintf (void) sprintf
#define Strcat (void) strcat
#define Strcpy (void) strcpy
#ifdef NEED_VARARGS
#define Vprintf (void) vprintf
#define Vfprintf (void) vfprintf
#define Vsprintf (void) vsprintf
#endif

/*
 *  Memory allocation.  Functions are declared here rather than in
 *  extern.h so that source files which use config.h instead of hack.h
 *  will see the declarations.
 *
 *  NetHack does not use malloc() [except to implement alloc() in alloc.c]
 *  or realloc() or calloc().  They return Null if memory runs out and
 *  nethack's code relies on alloc() to intercept that so that a zillion
 *  callers don't need to test for Null result.  alloc() never returns
 *  Null; if memory runs out, it calls panic() and does not return at all.
 */

/* dupstr() is unconditional in alloc.c but not used when MONITOR_HEAP
   is enabled; some utility programs link with alloc.{o,obj} and need it
   if nethack is built with MONITOR_HEAP enabled and they aren't; this
   declaration has been moved out of the '#else' below to avoid getting
   a complaint from -Wmissing-prototypes when building with MONITOR_HEAP */
extern char *FDECL(dupstr, (const char *));

/*
 * MONITOR_HEAP is conditionally used for primitive memory leak debugging.
 * When enabled, NH_HEAPLOG (if defined in the environment) is used as the
 * name of a log file to create for capturing allocations and releases.
 * [The 'heaputil' program to analyze that file isn't included in releases.]
 *
 * See alloc.c.
 */
#ifdef MONITOR_HEAP
/* plain alloc() is not declared except in alloc.c */
extern long *FDECL(nhalloc, (unsigned int, const char *, int));
extern void FDECL(nhfree, (genericptr_t, const char *, int));
extern char *FDECL(nhdupstr, (const char *, const char *, int));
/* this predates C99's __func__; that is trickier to use conditionally
   because it is not implemented as a preprocessor macro; MONITOR_HEAP
   wouldn't gain much benefit from it anyway so continue to live without it;
   if func's caller were accessible, that would be a very different issue */
#ifndef __FILE__
#define __FILE__ ""
#endif
#ifndef __LINE__
#define __LINE__ 0
#endif
#define alloc(a) nhalloc(a, __FILE__, (int) __LINE__)
#define free(a) nhfree(a, __FILE__, (int) __LINE__)
#define dupstr(s) nhdupstr(s, __FILE__, (int) __LINE__)
#else /* !MONITOR_HEAP */
/* declare alloc.c's alloc(); allocations made with it use ordinary free() */
extern long *FDECL(alloc, (unsigned int));  /* alloc.c */
#endif /* ?MONITOR_HEAP */

/* Used for consistency checks of various data files; declare it here so
   that utility programs which include config.h but not hack.h can see it. */
struct version_info {
    unsigned long incarnation;   /* actual version number */
    unsigned long feature_set;   /* bitmask of config settings */
    unsigned long entity_count;  /* # of monsters and objects */
    unsigned long struct_sizes1; /* size of key structs */
    unsigned long struct_sizes2; /* size of more key structs */
};

struct savefile_info {
    unsigned long sfi1; /* compression etc. */
    unsigned long sfi2; /* miscellaneous */
    unsigned long sfi3; /* thirdparty */
};
#ifdef NHSTDC
#define SFI1_EXTERNALCOMP (1UL)
#define SFI1_RLECOMP (1UL << 1)
#define SFI1_ZEROCOMP (1UL << 2)
#else
#define SFI1_EXTERNALCOMP (1L)
#define SFI1_RLECOMP (1L << 1)
#define SFI1_ZEROCOMP (1L << 2)
#endif

/*
 * Configurable internal parameters.
 *
 * Please be very careful if you are going to change one of these.  Any
 * changes in these parameters, unless properly done, can render the
 * executable inoperative.
 */

/* size of terminal screen is (at least) (ROWNO+3) by COLNO */
#define COLNO 80
#define ROWNO 21

#define MAXNROFROOMS 40 /* max number of rooms per level */
#define MAX_SUBROOMS 24 /* max # of subrooms in a given room */
#define DOORMAX 120     /* max number of doors per level */

#define BUFSZ 256  /* for getlin buffers */
#define QBUFSZ 128 /* for building question text */
#define TBUFSZ 300 /* g.toplines[] buffer max msg: 3 81char names */
/* plus longest prefix plus a few extra words */

#define PL_NSIZ 32 /* name of player, ghost, shopkeeper */
#define PL_CSIZ 32 /* sizeof pl_character */
#define PL_FSIZ 32 /* fruit name */
#define PL_PSIZ 63 /* player-given names for pets, other monsters, objects */

#define MAXDUNGEON 16 /* current maximum number of dungeons */
#define MAXLEVEL 32   /* max number of levels in one dungeon */
#define MAXSTAIRS 1   /* max # of special stairways in a dungeon */
#define ALIGNWEIGHT 4 /* generation weight of alignment */

#define MAXULEV 30 /* max character experience level */

#define MAXMONNO 120 /* extinct monst after this number created */
#define MHPMAX 500   /* maximum monster hp */

/*
 * Version 3.7.x has aspirations of portable file formats. We
 * make a distinction between MAIL functionality and MAIL_STRUCTURES
 * so that the underlying structures are consistent, whether MAIL is
 * defined or not.
 */
#define MAIL_STRUCTURES

/* PANICTRACE: Always defined for NH_DEVEL_STATUS != NH_STATUS_RELEASED
   but only for supported platforms. */
#ifdef UNIX
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
/* see end.c */
#ifndef PANICTRACE
#define PANICTRACE
#endif
#endif
#endif
/* The following are meaningless if PANICTRACE is not defined: */
#if defined(__linux__) && defined(__GLIBC__) && (__GLIBC__ >= 2)
#define PANICTRACE_LIBC
#endif
#if defined(MACOSX)
#define PANICTRACE_LIBC
#endif
#ifdef UNIX
#define PANICTRACE_GDB
#endif

/* Supply nethack_enter macro if not supplied by port */
#ifndef nethack_enter
#define nethack_enter(argc, argv) ((void) 0)
#endif

/* Supply nhassert macro if not supplied by port */
#ifndef nhassert
#define nhassert(e) ((void)0)
#endif


#endif /* GLOBAL_H */

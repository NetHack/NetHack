/* NetHack 3.7	config1.h	$NHDT-Date: 1596498530 2020/08/03 23:48:50 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.23 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef CONFIG1_H
#define CONFIG1_H

/*
 * MS DOS - compilers
 *
 * Microsoft C auto-defines MSDOS,
 * Borland C   auto-defines __MSDOS__,
 * DJGPP       auto-defines MSDOS.
 */

/* #define MSDOS */ /* use if not defined by compiler or cases below */

#ifdef __MSDOS__ /* for Borland C */
#ifndef MSDOS
#define MSDOS
#endif
#endif

#ifdef __TURBOC__
#define __MSC /* increase Borland C compatibility in libraries */
#endif

#ifdef MSDOS
#undef UNIX
#ifndef CROSSCOMPILE
#define SHORT_FILENAMES
#endif
#endif

/*
 * Mac Stuff.
 */
#if defined(__APPLE__) && defined(__MACH__)
#define MACOS
#endif

#ifdef macintosh /* Auto-defined symbol for MPW compilers (sc and mrc) */
#define MAC
#endif

#ifdef THINK_C /* Think C auto-defined symbol */
#define MAC
#define NEED_VARARGS
#endif

#ifdef __MWERKS__ /* defined by Metrowerks' Codewarrior compiler */
#ifndef __BEOS__  /* BeOS */
#define MAC
#endif
#define NEED_VARARGS
#define USE_STDARG
#endif

#if defined(MAC) || defined(__BEOS__)
#define DLB
#undef UNIX
#endif

#ifdef __BEOS__
#define NEED_VARARGS
#endif

/*
 * Amiga setup.
 */
#ifdef AZTEC_C   /* Manx auto-defines this */
#ifdef MCH_AMIGA /* Manx auto-defines this for AMIGA */
#ifndef AMIGA
#define AMIGA    /* define for Commodore-Amiga */
#endif           /* (SAS/C auto-defines AMIGA) */
#define AZTEC_50 /* define for version 5.0 of manx */
#endif
#endif
#ifdef __SASC_60
#define NEARDATA __near /* put some data close */
#else
#ifdef _DCC
#define NEARDATA __near /* put some data close */
#else
#define NEARDATA
#endif
#endif
#ifdef AMIGA
#define NEED_VARARGS
#undef UNIX
#define DLB
#define HACKDIR "NetHack:"
#define NO_MACRO_CPATH
#endif

/*
 * Atari auto-detection
 */

#ifdef atarist
#undef UNIX
#ifndef TOS
#define TOS
#endif
#else
#ifdef __MINT__
#undef UNIX
#ifndef TOS
#define TOS
#endif
#endif
#endif

/*
 * Windows NT Autodetection
 */
#ifdef _WIN32_WCE
#define WIN_CE
#ifndef WIN32
#define WIN32
#endif
#endif

#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif

#ifdef WIN32
#undef UNIX
#undef MSDOS
#define NHSTDC
#define USE_STDARG
#define NEED_VARARGS

#ifndef WIN_CE
#define STRNCMPI
#define STRCMPI
#endif

#endif

#if defined(__linux__) && defined(__GNUC__) && !defined(_GNU_SOURCE)
/* ensure _GNU_SOURCE is defined before including any system headers */
#define _GNU_SOURCE
#endif

#ifdef __vms
#ifndef VMS
#define VMS
#endif
#endif

#ifdef VMS /* really old compilers need special handling, detected here */
#undef UNIX
#ifdef __DECC
#ifndef __DECC_VER /* buggy early versions want widened prototypes */
#define NOTSTDC    /* except when typedefs are involved            */
        /* [25 or so years later...  That was probably uchar widening to */
        /* 'unsigned int' rather than anything to do with typedefs.  pr] */
#define USE_VARARGS
#else              /* __DECC_VER not defined */
#if __DECC_VER >= 70000000
#define VMSVSI
#endif /* _DECC_VER >= 70000000 */
#ifndef VMSVSI
#define NHSTDC
#define USE_STDARG
#define POSIX_TYPES
#ifndef _DECC_V4_SOURCE /* only def here if not already def'd on comd line */
#define _DECC_V4_SOURCE /* avoid some incompatible V5.x (and later) changes */
#endif
#endif /* !VMSVSI */
#endif /*__DECC_VER*/
#undef __HIDE_FORBIDDEN_NAMES /* need non-ANSI library support functions */
#ifndef VMSVSI
#ifdef VAXC    /* DEC C in VAX C compatibility mode; 'signed' works   */
#define signed /* but causes diagnostic about VAX C not supporting it */
#endif
#else /*!__DECC*/
#ifdef VAXC /* must use CC/DEFINE=ANCIENT_VAXC for vaxc v2.2 or older */
#define signed
#ifdef ANCIENT_VAXC /* vaxc v2.2 and earlier [lots of warnings to come] */
#define KR1ED       /* simulate defined() */
#define USE_VARARGS
#else                       /* vaxc v2.3,2.4,or 3.x, or decc in vaxc mode */
#if defined(USE_PROTOTYPES) /* this breaks 2.2 (*forces* use of ANCIENT)*/
#define __STDC__ 0 /* vaxc is not yet ANSI compliant, but close enough */
#include <stddef.h>
#define UNWIDENED_PROTOTYPES
#endif
#define USE_STDARG
#endif
#endif              /*VAXC*/
#endif              /*__DECC*/
#ifdef VERYOLD_VMS  /* v4.5 or earlier; no longer available for testing */
#define USE_OLDARGS /* <varargs.h> is there, vprintf & vsprintf aren't */
#ifdef USE_VARARGS
#undef USE_VARARGS
#endif
#ifdef USE_STDARG
#undef USE_STDARG
#endif
#endif
#endif /* !VMSVSI */
#endif /*VMS*/

#ifdef vax
/* just in case someone thinks a DECstation is a vax. It's not, it's a mips */
#ifdef ULTRIX_PROTO
#undef ULTRIX_PROTO
#endif
#ifdef ULTRIX_CC20
#undef ULTRIX_CC20
#endif
#endif

#ifdef KR1ED /* For compilers which cannot handle defined() */
#define defined(x) (-x - 1 != -1)
/* Because:
 * #define FOO => FOO={} => defined( ) => (-1 != - - 1) => 1
 * #define FOO 1 or on command-line -DFOO
 *      => defined(1) => (-1 != - 1 - 1) => 1
 * if FOO isn't defined, FOO=0. But some compilers default to 0 instead of 1
 * for -DFOO, oh well.
 *      => defined(0) => (-1 != - 0 - 1) => 0
 *
 * But:
 * defined("") => (-1 != - "" - 1)
 *   [which is an unavoidable catastrophe.]
 */
#endif

#endif /* CONFIG1_H */

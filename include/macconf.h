/*	SCCS Id: @(#)macconf.h	3.3	99/10/25	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef MAC
# ifndef MACCONF_H
#  define MACCONF_H

/*
 * Compiler selection is based on the following symbols:
 *
 *	applec			MPW compiler
 *	THINK_C			Think C compiler
 *	__MWERKS__		Metrowerks compiler
 *
 * We use these early in config.h to define some needed symbols,
 * including MAC.
 #
 # The Metrowerks compiler defines __STDC__ (which sets NHSTC) and uses
 # WIDENED_PROTOTYPES (defined if UNWIDENED_PROTOTYPES is undefined and
 # NHSTDC is defined).
 */
#  ifdef applec
#   define MAC_MPW32		/* Headers, and for avoiding a bug */
#  endif

#  ifndef __powerc
#   define MAC68K		/* 68K mac (non-powerpc) */
#  endif

#  define RANDOM
#  define NO_SIGNAL		/* You wouldn't believe our signals ... */
#  define FILENAME 256
#  define NO_TERMS		/* For tty port (see wintty.h) */

#  define TEXTCOLOR		/* For Mac TTY interface */
#  define CHANGE_COLOR

/* Use these two includes instead of system.h. */
#include <string.h>
#include <stdlib.h>

/* Uncomment this line if your headers don't already define off_t */
/*typedef long off_t;*/

/*
 * Try and keep the number of files here to an ABSOLUTE minimum !
 * include the relevant files in the relevant .c files instead !
 */
#include <MacTypes.h>
/*
 * Turn off the Macsbug calls for the production version.
 */
#if 0
#  undef Debugger
#  undef DebugStr
#  define Debugger()
#  define DebugStr(aStr)
#endif

/*
 * We could use the PSN under sys 7 here ...
 */
#ifndef __CONDITIONALMACROS__	/* universal headers */
#  define getpid() 1
#  define getuid() 1
#endif
#  define index strchr
#  define rindex strrchr

#  define Rand random
extern void error(const char *,...);

# if !defined(O_WRONLY)
#  ifdef __MWERKS__
#include <unix.h>
#   ifndef O_EXCL
     /* MW 4.5 doesn't have this, so just use a bogus value */
#    define O_EXCL 0x80000000
#   endif
#  else
#include <fcntl.h>
#  endif
# endif

/*
 * Don't redefine these Unix IO functions when making LevComp or DgnComp for
 * MPW.  With MPW, we make them into MPW tools, which use unix IO.  SPEC_LEV
 * and DGN_COMP are defined when compiling for LevComp and DgnComp respectively.
 */
#if !(defined(applec) && (defined(SPEC_LEV) || defined(DGN_COMP)))
# define creat maccreat
# define open macopen
# define close macclose
# define read macread
# define write macwrite
# define lseek macseek
#endif

# define TEXT_TYPE 'TEXT'
# define LEVL_TYPE 'LEVL'
# define BONE_TYPE 'BONE'
# define SAVE_TYPE 'SAVE'
# define PREF_TYPE 'PREF'
# define DATA_TYPE 'DATA'
# define MAC_CREATOR 'nh31' /* Registered with DTS ! */
# define TEXT_CREATOR 'ttxt' /* Something the user can actually edit */

/*
 * Define PORT_HELP to be the name of the port-specfic help file.
 * This file is included into the resource fork of the application.
 */
#define PORT_HELP "MacHelp"

#define MAC_GRAPHICS_ENV

# endif /* ! MACCONF_H */
#endif /* MAC */

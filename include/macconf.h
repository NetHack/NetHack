/* NetHack 3.6	macconf.h	$NHDT-Date: 1432512782 2015/05/25 00:13:02 $  $NHDT-Branch: master $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kevin Hugo, 2004. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef MAC
#ifndef MACCONF_H
#define MACCONF_H

/*
 * Compiler selection is based on the following symbols:
 *
 *  __SC__			sc, a MPW 68k compiler
 *  __MRC__			mrc, a MPW PowerPC compiler
 *	THINK_C			Think C compiler
 *	__MWERKS__		Metrowerks' Codewarrior compiler
 *
 * We use these early in config.h to define some needed symbols,
 * including MAC.
 #
 # The Metrowerks compiler defines __STDC__ (which sets NHSTC) and uses
 # WIDENED_PROTOTYPES (defined if UNWIDENED_PROTOTYPES is undefined and
 # NHSTDC is defined).
 */

#ifndef __powerc
#define MAC68K /* 68K mac (non-powerpc) */
#endif
#ifndef TARGET_API_MAC_CARBON
#define TARGET_API_MAC_CARBON 0
#endif

#ifndef __MACH__
#define RANDOM
#endif
#define NO_SIGNAL /* You wouldn't believe our signals ... */
#define FILENAME 256
#define NO_TERMS /* For tty port (see wintty.h) */

#define TEXTCOLOR /* For Mac TTY interface */
#define CHANGE_COLOR

/* Use these two includes instead of system.h. */
#include <string.h>
#include <stdlib.h>

/* Uncomment this line if your headers don't already define off_t */
/*typedef long off_t;*/
#include <time.h> /* for time_t */

/*
 * Try and keep the number of files here to an ABSOLUTE minimum !
 * include the relevant files in the relevant .c files instead !
 */
#if TARGET_API_MAC_CARBON
#ifdef GNUC
/* Avoid including <CarbonCore/fp.h> -- it has a conflicting expl() */
#define __FP__
#include <Carbon/Carbon.h>
#else
/* Avoid including <fenv.h> -- it uses GENERATINGPOWERPC */
#define __FENV__
#include <machine/types.h>
#include <Carbon.h>
#endif
#else
#include <MacTypes.h>
#endif

/*
 * We could use the PSN under sys 7 here ...
 * ...but it wouldn't matter...
 */
#define getpid() 1
#define getuid() 1
#define index strchr
#define rindex strrchr

#define Rand random
extern void error(const char *, ...);

#if !defined(O_WRONLY)
#if defined(__MWERKS__) && !TARGET_API_MAC_CARBON
#include <unix.h>
#endif
#include <fcntl.h>
#endif

/*
 * Don't redefine these Unix IO functions when making LevComp or DgnComp for
 * MPW.  With MPW, we make them into MPW tools, which use unix IO.  SPEC_LEV
 * and DGN_COMP are defined when compiling for LevComp and DgnComp
 * respectively.
 */
#if !((defined(__SC__) || defined(__MRC__) || defined(__MACH__)) \
      && (defined(SPEC_LEV) || defined(DGN_COMP)))
#define creat maccreat
#define open macopen
#define close macclose
#define read macread
#define write macwrite
#define lseek macseek
#ifdef __MWERKS__
#define unlink _unlink
#endif
#endif

#define YY_NEVER_INTERACTIVE 1

#define TEXT_TYPE 'TEXT'
#define LEVL_TYPE 'LEVL'
#define BONE_TYPE 'BONE'
#define SAVE_TYPE 'SAVE'
#define PREF_TYPE 'PREF'
#define DATA_TYPE 'DATA'
#define MAC_CREATOR 'nh31'  /* Registered with DTS ! */
#define TEXT_CREATOR 'ttxt' /* Something the user can actually edit */

/*
 * Define PORT_HELP to be the name of the port-specfic help file.
 * This file is included into the resource fork of the application.
 */
#define PORT_HELP "MacHelp"

#define MAC_GRAPHICS_ENV

#endif /* ! MACCONF_H */
#endif /* MAC */

/* NetHack 3.6	os2conf.h	$NHDT-Date: 1432512775 2015/05/25 00:12:55 $  $NHDT-Branch: master $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* Copyright (c) Timo Hakulinen, 1990, 1991, 1992, 1993, 1996. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef OS2
#ifndef OS2CONF_H
#define OS2CONF_H

/*
 * Compiler configuration.  Compiler may be
 * selected either here or in Makefile.os2.
 */

/* #define OS2_MSC		/* Microsoft C 5.1 and 6.0 */
#define OS2_GCC /* GCC emx 0.8f */
                /* #define OS2_CSET2		/* IBM C Set/2 (courtesy Jeff Urlwin) */
                /* #define OS2_CSET2_VER_1	/* CSet/2 version selection */
                /* #define OS2_CSET2_VER_2	/* - " - */

/*
 * System configuration.
 */

#define OS2_USESYSHEADERS /* use compiler's own system headers */
/* #define OS2_HPFS		/* use OS/2 High Performance File System */

#if defined(OS2_GCC) || defined(OS2_CSET2)
#define OS2_32BITAPI /* enable for compilation in OS/2 2.0 */
#endif

/*
 * Other configurable options.  Generally no
 * reason to touch the defaults, I think.
 */

/*#define MFLOPPY			/* floppy and ramdisk support */
#define RANDOM /* Berkeley random(3) */
#define SHELL  /* shell escape */
/* #define TERMLIB		/* use termcap file */
#define ANSI_DEFAULT /* allows NetHack to run without termcap file */
#define TEXTCOLOR    /* allow color */

/*
 * The remaining code shouldn't need modification.
 */

#ifdef MSDOS
#undef MSDOS /* MSC autodefines this but we don't want it */
#endif

#ifndef MICRO
#define MICRO /* must be defined to allow some inclusions */
#endif

#if !defined(TERMLIB) && !defined(ANSI_DEFAULT)
#define ANSI_DEFAULT /* have to have one or the other */
#endif

#define PATHLEN 260  /* maximum pathlength (HPFS) */
#define FILENAME 260 /* maximum filename length (HPFS) */
#ifndef MICRO_H
#include "micro.h" /* necessary externs for [os_name].c */
#endif

#ifndef SYSTEM_H
/* #include "system.h" */
#endif

#include <time.h>

/* the high quality random number routines */
#ifndef USE_ISAAC64
# ifdef RANDOM
#  define Rand() random()
# else
#  define Rand() rand()
# endif
#endif

/* file creation mask */

#include <sys\types.h>
#include <sys\stat.h>

#define FCMASK (S_IREAD | S_IWRITE)

#include <fcntl.h>

#ifdef __EMX__
#include <unistd.h>
#define sethanguphandler(foo) (void) signal(SIGHUP, (SIG_RET_TYPE) foo)
#endif

void hangup(int i);
#endif /* OS2CONF_H */
#endif /* OS2 */

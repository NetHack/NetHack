/* NetHack 3.6	tosconf.h	$NHDT-Date: 1432512782 2015/05/25 00:13:02 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef TOS
#ifndef TOSCONF_H
#define TOSCONF_H

#define MICRO /* must be defined to allow some inclusions */

/*
   Adjust these options to suit your compiler. The default here is for
   GNU C with the MiNT library.
*/

/*#define NO_SIGNAL		/* library doesn't support signals	*/
/*#define NO_FSTAT		/* library doesn't have fstat() call	*/
#define MINT /* library supports MiNT extensions to TOS */

#ifdef __MINT__
#define MINT
#endif

#ifdef O_BINARY
#define FCMASK O_BINARY
#else
#define FCMASK 0660
#define O_BINARY 0
#endif

#ifdef UNIXDEBUG
#define remove(x) unlink(x)
#endif

/* configurable options */
#define MFLOPPY   /* floppy support		*/
#define RANDOM    /* improved random numbers	*/
#define SHELL     /* allow spawning of shell	*/
#define TERMLIB   /* use termcap			*/
#define TEXTCOLOR /* allow color			*/
#define MAIL      /* enable the fake maildemon */
#ifdef MINT
#define SUSPEND /* allow suspending the game	*/
#endif

#ifndef TERMLIB
#define ANSI_DEFAULT /* use vt52 by default		*/
#endif

#if defined(__GNUC__) || defined(__MINT__)
/* actually, only more recent GNU C libraries have strcmpi
 * on the other hand, they're free -- if yours is out of
 * date, grab the most recent from atari.archive.umich.edu
 */
#define STRNCMPI
#undef strcmpi
extern int FDECL(strcmpi, (const char *, const char *));
extern int FDECL(strncmpi, (const char *, const char *, size_t));
#endif

#include <termcap.h>
#include <unistd.h>
/* instead of including system.h from pcconf.h */
#include <string.h>
#include <stdlib.h>
#include <types.h>
#define SIG_RET_TYPE __Sigfunc
#define SYSTEM_H

#ifndef MICRO_H
#include "micro.h"
#endif
#ifndef PCCONF_H
#include "pcconf.h" /* remainder of stuff is same as the PC */
#endif

#ifdef TEXTCOLOR
extern boolean colors_changed; /* in tos.c */
#endif

#ifdef __GNUC__
#define GCC_BUG /* correct a gcc bug involving double for loops */
#endif

#endif /* TOSCONF_H */
#endif /* TOS */

/* NetHack 3.6	tosconf.h	$NHDT-Date: 1432512782 2015/05/25 00:13:02 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef TOS
#ifndef TOSCONF_H
#define TOSCONF_H

#define MICRO /* must be defined to allow some inclusions */
#define NOCWD_ASSUMPTIONS /* allow paths to be specified for HACKDIR, etc. */
#define DLB
#define DLBFILE "nhdat"
#undef DLBFILE2

/*
   Adjust these options to suit your compiler. The default here is for
   GNU C with the MiNT library.
*/

/*#define NO_SIGNAL		/* library doesn't support signals	*/
/*#define NO_FSTAT		/* library doesn't have fstat() call	*/
#ifdef __MINT__
#define MINT /* library supports MiNT extensions to TOS */
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
#ifdef MINT
#define SHELL   /* allow spawning of shell (requires system(3))	*/
#define SUSPEND /* allow suspending the game	*/
#endif
#ifndef NO_TERMS
#define TERMLIB   /* use termcap			*/
#endif
#define MAIL      /* enable the fake maildemon */

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
extern int strcmpi(const char *, const char *);
extern int strncmpi(const char *, const char *, size_t);
#endif

#ifdef TERMLIB
#include <termcap.h>
#endif
#include <unistd.h>
/* instead of including system.h from pcconf.h */
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
/* SIG_RET_TYPE: let hack.h fall back to "void (*)(int)" -- POSIX-correct
   and avoids depending on MiNTLib's evolving __sighandler_t / __Sigfunc. */
#define SYSTEM_H

#ifndef MICRO_H
#include "micro.h"
#endif
#ifndef PCCONF_H
#include "pcconf.h" /* remainder of stuff is same as the PC */
#endif

/* Atari TOS is single-user with no system-wide config directory.
   nethack.cnf in HACKDIR is the only config file. The cross-build's
   -USYSCF in TARGET_CFLAGS is the primary mechanism; this undef is the
   safety net for any native-TOS build scenario. */
#ifdef SYSCF
#undef SYSCF
#endif
#ifdef SYSCF_FILE
#undef SYSCF_FILE
#endif

/* The GEM windowport uses the generic genl_status_* fallbacks and
   doesn't implement field-level hilites.  Under STATUS_HILITES,
   init_sound_disp_gamewindows() skips the early
   display_nhwindow(WIN_STATUS); on TOS 4.04 that late open leaves
   title-bar / tile-column draw glitches behind the role-picker
   dialog.  Undef'ing restores the pre-3.5 ordering. */
#ifdef STATUS_HILITES
#undef STATUS_HILITES
#endif

extern boolean colors_changed; /* in tos.c */

#endif /* TOSCONF_H */
#endif /* TOS */

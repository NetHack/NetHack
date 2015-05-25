/* NetHack 3.6	amiconf.h	$NHDT-Date: 1432512775 2015/05/25 00:12:55 $  $NHDT-Branch: master $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Kenneth Lorber, Bethesda, Maryland, 1990, 1991, 1992, 1993.
 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef AMICONF_H
#define AMICONF_H

#undef abs /* avoid using macro form of abs */
#ifndef __SASC_60
#undef min /* this gets redefined */
#undef max /* this gets redefined */
#endif

#include <time.h> /* get time_t defined before use! */

#ifdef __SASC_60    /* since SAS can prevent re-inclusion */
#include <stdlib.h> /* general things, including builtins */
#include <string.h>
#endif

#ifdef AZTEC_50
#include <stdlib.h>
#define AZTEC_C_WORKAROUND /* Bug which turns up in sounds.c. Bummer... */
#define NO_SIGNAL          /* 5.0 signal handling doesn't like SIGINT... */
#endif

#ifdef _DCC
#include <stdlib.h>
#define _SIZE_T
#define DCC30_BUG /* A bitfield bug (from dog.c, others) in DICE 3.0. */
#endif

#ifndef __GNUC__
typedef long off_t;
#endif

#define MICRO /* must be defined to allow some inclusions */

#define NOCWD_ASSUMPTIONS /* Allow paths to be specified for HACKDIR, \
                             LEVELDIR, SAVEDIR, BONESDIR, DATADIR,    \
                             SCOREDIR, LOCKDIR, CONFIGDIR, and TROUBLEDIR */

/* data librarian defs */
#ifndef NOCWD_ASSUMPTIONS
#define DLBFILE "NetHack:nhdat"   /* main library */
#define DLBFILE2 "NetHack:nhsdat" /* sound library */
#else
#define DLBFILE "nhdat"   /* main library */
#define DLBFILE2 "nhsdat" /* sound library */
#endif

#define FILENAME_CMP stricmp /* case insensitive */

#ifndef __SASC_60
#define O_BINARY 0
#endif

/* Compile in New Intuition look for 2.0 */
#ifdef IDCMP_CLOSEWINDOW
#ifndef INTUI_NEW_LOOK
#define INTUI_NEW_LOOK 1
#endif
#endif

#define MFLOPPY /* You'll probably want this; provides assistance \
                 * for typical personal computer configurations   \
                 */
#define RANDOM

/* ### amidos.c ### */

extern void FDECL(nethack_exit, (int));

/* ### amiwbench.c ### */

extern void NDECL(ami_wbench_init);
extern void NDECL(ami_wbench_args);
extern int FDECL(ami_wbench_getsave, (int));
extern void FDECL(ami_wbench_unlink, (char *));
extern int FDECL(ami_wbench_iconsize, (char *));
extern void FDECL(ami_wbench_iconwrite, (char *));
extern int FDECL(ami_wbench_badopt, (const char *));
extern void NDECL(ami_wbench_cleanup);
extern void FDECL(getlind, (const char *, char *, const char *));

/* ### winreq.c ### */

extern void amii_setpens(int);

extern void FDECL(exit, (int));
extern void NDECL(CleanUp);
extern void FDECL(Abort, (long));
extern int NDECL(getpid);
extern char *FDECL(CopyFile, (const char *, const char *));
extern int NDECL(kbhit);
extern int NDECL(WindowGetchar);
extern void FDECL(ami_argset, (int *, char *[]));
extern void FDECL(ami_mkargline, (int *, char **[]));
extern void FDECL(ami_wininit_data, (int));

#define FromWBench 0 /* A hint for compiler ... */
/* extern boolean FromWBench;	/* how were we run? */
extern int ami_argc;
extern char **ami_argv;

#ifndef MICRO_H
#include "micro.h"
#endif

#ifndef PCCONF_H
#include "pcconf.h" /* remainder of stuff is almost same as the PC */
#endif

#define remove(x) unlink(x)

/* DICE wants rewind() to return void.	We want it to return int. */
#if defined(_DCC) || defined(__GNUC__)
#define rewind(f) fseek(f, 0, 0)
#endif

#ifdef AZTEC_C
extern FILE *FDECL(freopen, (const char *, const char *, FILE *));
extern char *FDECL(gets, (char *));
#endif

#define msmsg printf

/*
 * If AZTEC_C  we can't use the long cpath in vision.c....
 */
#ifdef AZTEC_C
#undef MACRO_CPATH
#endif

/*
 *  (Possibly) configurable Amiga options:
 */

#define TEXTCOLOR /* Use colored monsters and objects */
#define HACKFONT  /* Use special hack.font */
#define SHELL     /* Have a shell escape command (!) */
#define MAIL      /* Get mail at unexpected occasions */
#define DEFAULT_ICON "NetHack:default.icon" /* private icon */
#define AMIFLUSH /* toss typeahead (select flush in .cnf) */
/* #define OPT_DISPMAP		/* enable fast_map option */

/* new window system options */
/* WRONG - AMIGA_INTUITION should go away */
#ifdef AMII_GRAPHICS
#define AMIGA_INTUITION /* high power graphics interface (amii) */
#endif

#define CHANGE_COLOR 1

#ifdef TEXTCOLOR
#define DEPTH 6 /* Maximum depth of the screen allowed */
#else
#define DEPTH 2 /* Four colors...sigh... */
#endif

#define AMII_MAXCOLORS (1L << DEPTH)
typedef unsigned short AMII_COLOR_TYPE;

#define PORT_HELP "nethack:amii.hlp"

#undef TERMLIB

#define AMII_MUFFLED_VOLUME 40
#define AMII_SOFT_VOLUME 50
#define AMII_OKAY_VOLUME 60
#define AMII_LOUDER_VOLUME 80

#ifdef TTY_GRAPHICS
#define ANSI_DEFAULT
#endif

extern int amibbs; /* BBS mode? */

#ifdef AMII_GRAPHICS
extern int amii_numcolors;
void FDECL(amii_setpens, (int));
#endif

/* for cmd.c: override version in micro.h */
#ifdef __SASC_60
#undef M
#define M(c) ((c) -128)
#endif

#endif /* AMICONF_H */

/* NetHack 3.6	amiconf.h	$NHDT-Date: 1432512775 2015/05/25 00:12:55 $  $NHDT-Branch: master $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Kenneth Lorber, Bethesda, Maryland, 1990, 1991, 1992, 1993.
 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef AMICONF_H
#define AMICONF_H

#undef abs /* avoid using macro form of abs */
#undef min /* this gets redefined */
#undef max /* this gets redefined */

#include <time.h> /* get time_t defined before use! */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <proto/dos.h>

#define MICRO /* must be defined to allow some inclusions */

#define NOCWD_ASSUMPTIONS /* Allow paths to be specified for HACKDIR, \
                             LEVELDIR, SAVEDIR, BONESDIR, DATADIR,    \
                             SCOREDIR, LOCKDIR, CONFIGDIR, and TROUBLEDIR */

#define PATHLEN 130

/* data librarian defs */
#define DLBFILE "nhdat"   /* main library */
/* nhsdat sound library not used in 5.0 */
#undef DLBFILE2

#define FILENAME_CMP strcmpi /* case insensitive */
#define O_BINARY 0

#define MFLOPPY /* You'll probably want this; provides assistance \
                 * for typical personal computer configurations   \
                 */

/* ### amidos.c ### */

extern void nethack_exit(int);

/* ### winreq.c ### */

extern void amii_setpens(int);

extern void getlind(const char *, char *, const char *);
extern void CleanUp(void);
extern void Abort(long) NORETURN;
extern int getpid(void);
extern int kbhit(void);
extern int WindowGetchar(void);
extern void ami_wininit_data(int);

#ifndef MICRO_H
#include "micro.h"
#endif

#ifndef PCCONF_H
#include "pcconf.h" /* remainder of stuff is almost same as the PC */
#endif

#define remove(x) unlink(x)
#define rewind(f) fseek(f, 0, 0)

/*
 *  (Possibly) configurable Amiga options:
 */

#define HACKFONT  /* Use special hack.font */
#define MAIL      /* Get mail at unexpected occasions */
#define AMIFLUSH /* toss typeahead (select flush in .cnf) */

/* new window system options */
/* WRONG - AMIGA_INTUITION should go away */
#ifdef AMII_GRAPHICS
#define AMIGA_INTUITION /* high power graphics interface (amii) */
#endif

#define CHANGE_COLOR 1
#define DEPTH 6 /* Maximum depth of the screen allowed */
#define AMII_MAXCOLORS (1L << DEPTH)
/* Number of palette entries actually populated in amii_init_map[] (AMII text
 * mode) and amiv_init_map[] (AMIV tile mode).  Indices beyond these read 0. */
#define AMII_PALETTE_SIZE 8
#define AMIV_PALETTE_SIZE 32
typedef unsigned short AMII_COLOR_TYPE;

#define PORT_HELP "amii.hlp"

#undef TERMLIB

#ifdef AMII_GRAPHICS
extern int amii_numcolors;
void amii_setpens(int);
#endif

struct ami_sysflags {
    char sysflagsid[10];
#ifdef AMIFLUSH
    boolean altmeta;  /* use ALT keys as META */
    boolean amiflush; /* kill typeahead */
#endif
#ifdef AMII_GRAPHICS
    int numcols;
    unsigned short amii_dripens[20]; /* DrawInfo Pens currently there are 13 in v39 */
    AMII_COLOR_TYPE amii_curmap[AMII_MAXCOLORS]; /* colormap */
#endif
#ifdef MFLOPPY
    boolean asksavedisk;
#endif
};
extern struct ami_sysflags sysflags;

#undef SYSCF
#undef SYSCF_FILE

#endif /* AMICONF_H */

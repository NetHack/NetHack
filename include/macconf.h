/* NetHack 5.0	macconf.h	$NHDT-Date: 1432512782 2015/05/25 00:13:02 $  $NHDT-Branch: master $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kevin Hugo, 2004. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef MACOS9
#ifndef MACCONF_H
#define MACCONF_H

/* Built with the Retro68 GCC cross-toolchain (see sys/mac/BUILD.md).
 * The MPW/Think C/CodeWarrior compilers of the original port are no
 * longer supported. */

/* No system-wide config file on classic Mac OS */
#undef STATUS_HILITES  /* Mac port doesn't support terminal-based hilites;
                          with it defined, WIN_STATUS is never displayed */

/* Lua: use 32-bit integers and 32-bit floats.
   Default 64-bit types are emulated in software on 68k and extremely slow. */
#define LUA_32BITS
#ifndef TARGET_API_MAC_OS8
#define TARGET_API_MAC_OS8 1
#endif
/* Use classic (non-opaque) toolbox structs and direct field access */
#ifndef OPAQUE_TOOLBOX_STRUCTS
#define OPAQUE_TOOLBOX_STRUCTS 0
#endif
#ifndef ACCESSOR_CALLS_ARE_FUNCTIONS
#define ACCESSOR_CALLS_ARE_FUNCTIONS 0
#endif

/* newlib provides random(); RANDOM (sys/share/random.c) not needed */
#define NO_SIGNAL /* You wouldn't believe our signals ... */
#define FILENAME 256
#define NO_TERMS /* For tty port (see wintty.h) */
#ifndef NO_CHANGE_COLOR
#define CHANGE_COLOR
#endif

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
#include <MacTypes.h>

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
/* macwin.c; called from options.c under #ifdef MACOS9 */
extern short set_font_name(int, char *);
/* macmain.c; called from options.c (other ports declare these in their
   *conf.h as well) */
extern boolean authorize_wizard_mode(void);
extern boolean authorize_explore_mode(void);

#include <fcntl.h>

/* Route the Unix I/O calls through their HFS implementations. */
#ifndef O_BINARY
#define O_BINARY 0
#endif
/* implemented in sys/mac/macunix.c */
extern void regularize(char *);
extern void getlock(void);

/* implemented in sys/mac/macfile.c */
extern int maccreat(const char *, long);
extern int macopen(const char *, int, long);
extern int macclose(int);
extern int macread(int, void *, unsigned);
extern int macwrite(int, void *, unsigned);
extern long macseek(int, long, short);
extern int macunlink(const char *);
#define creat maccreat
#define open macopen
#define close macclose
#define read macread
#define write macwrite
#define lseek macseek

#define TEXT_TYPE 'TEXT'
#define LEVL_TYPE 'LEVL'
#define BONE_TYPE 'BONE'
#define SAVE_TYPE 'SAVE'
#define PREF_TYPE 'PREF'
#define DATA_TYPE 'DATA'
#define MAC_CREATOR 'nh50'  /* this port's creator code (files from older
                               builds keep working; only their Finder icon
                               binding goes stale) */
#define TEXT_CREATOR 'ttxt' /* Something the user can actually edit */

/*
 * Define PORT_HELP to be the name of the port-specfic help file.
 * This file is included into the resource fork of the application.
 */
#define PORT_HELP "MacHelp"

#define MAC_GRAPHICS_ENV

#endif /* ! MACCONF_H */
#endif /* MAC */

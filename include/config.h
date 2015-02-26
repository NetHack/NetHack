/* NetHack 3.5	config.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	config.h	$Date: 2012/01/27 20:15:26 $  $Revision: 1.37 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef CONFIG_H /* make sure the compiler does not see the typedefs twice */
#define CONFIG_H


/*
 * Section 1:	Operating and window systems selection.
 *		Select the version of the OS you are using.
 *		For "UNIX" select BSD, ULTRIX, SYSV, or HPUX in unixconf.h.
 *		A "VMS" option is not needed since the VMS C-compilers
 *		provide it (no need to change sec#1, vmsconf.h handles it).
 */

#define UNIX		/* delete if no fork(), exec() available */

/* #define MSDOS */	/* in case it's not auto-detected */

/* #define OS2 */	/* define for OS/2 */

/* #define TOS */	/* define for Atari ST/TT */

/* #define STUPID */	/* avoid some complicated expressions if
			   your C compiler chokes on them */
/* #define MINIMAL_TERM */
			/* if a terminal handles highlighting or tabs poorly,
			   try this define, used in pager.c and termcap.c */
/* #define ULTRIX_CC20 */
			/* define only if using cc v2.0 on a DECstation */
/* #define ULTRIX_PROTO */
			/* define for Ultrix 4.0 (or higher) on a DECstation;
			 * if you get compiler errors, don't define this. */
			/* Hint: if you're not developing code, don't define
			   ULTRIX_PROTO. */

#include "config1.h"	/* should auto-detect MSDOS, MAC, AMIGA, and WIN32 */


/* Windowing systems...
 * Define all of those you want supported in your binary.
 * Some combinations make no sense.  See the installation document.
 */
#if !defined(NOTTYGRAPHICS)
# define TTY_GRAPHICS	/* good old tty based graphics */
#endif
/* #define X11_GRAPHICS */	/* X11 interface */
/* #define QT_GRAPHICS */	/* Qt interface */
/* #define GNOME_GRAPHICS */	/* Gnome interface */
/* #define MSWIN_GRAPHICS */	/* Windows NT, CE, Graphics */

/*
 * Define the default window system.  This should be one that is compiled
 * into your system (see defines above).  Known window systems are:
 *
 *	tty, X11, mac, amii, BeOS, Qt, Gem, Gnome
 */

/* MAC also means MAC windows */
#ifdef MAC
# ifndef	AUX
#  define DEFAULT_WINDOW_SYS "mac"
# endif
#endif

/* Amiga supports AMII_GRAPHICS and/or TTY_GRAPHICS */
#ifdef AMIGA
# define AMII_GRAPHICS			/* (optional) */
# define DEFAULT_WINDOW_SYS "amii"	/* "amii", "amitile" or "tty" */
#endif

/* Atari supports GEM_GRAPHICS and/or TTY_GRAPHICS */
#ifdef TOS
# define GEM_GRAPHICS			/* Atari GEM interface (optional) */
# define DEFAULT_WINDOW_SYS "Gem"	/* "Gem" or "tty" */
#endif

#ifdef __BEOS__
#define BEOS_GRAPHICS /* (optional) */
#define DEFAULT_WINDOW_SYS "BeOS"  /* "tty" */
#ifndef HACKDIR	/* override the default hackdir below */
# define HACKDIR "/boot/apps/NetHack"
#endif
#endif

#ifdef QT_GRAPHICS
# ifndef DEFAULT_WC_TILED_MAP
#  define DEFAULT_WC_TILED_MAP	/* Default to tiles if users doesn't say wc_ascii_map */
# endif
#ifndef NOUSER_SOUNDS
# define USER_SOUNDS		/* Use sounds */
/* # define USER_SOUNDS_REGEX */    /* Use regexps for sound message matches */
#endif
# define USE_XPM		/* Use XPM format for images (required) */
# define GRAPHIC_TOMBSTONE	/* Use graphical tombstone (rip.ppm) */
# ifndef DEFAULT_WINDOW_SYS
#  define DEFAULT_WINDOW_SYS "Qt"
# endif
#endif

#ifdef GNOME_GRAPHICS
# define USE_XPM		/* Use XPM format for images (required) */
# define GRAPHIC_TOMBSTONE	/* Use graphical tombstone (rip.ppm) */
# ifndef DEFAULT_WINDOW_SYS
#  define DEFAULT_WINDOW_SYS "Gnome"
# endif
#endif

#ifdef MSWIN_GRAPHICS
# ifdef TTY_GRAPHICS
# undef TTY_GRAPHICS
# endif
# ifndef DEFAULT_WINDOW_SYS
#  define DEFAULT_WINDOW_SYS "mswin"
# endif
# define HACKDIR "\\nethack"
#endif

#ifndef DEFAULT_WINDOW_SYS
# define DEFAULT_WINDOW_SYS "tty"
#endif

#ifdef X11_GRAPHICS
/*
 * There are two ways that X11 tiles may be defined.  (1) using a custom
 * format loaded by NetHack code, or (2) using the XPM format loaded by
 * the free XPM library.  The second option allows you to then use other
 * programs to generate tiles files.  For example, the PBMPlus tools
 * would allow:
 *  xpmtoppm <x11tiles.xpm | pnmscale 1.25 | ppmquant 90 >x11tiles_big.xpm
 */
/* # define USE_XPM */		/* Disable if you do not have the XPM library */
# ifdef USE_XPM
#  define GRAPHIC_TOMBSTONE	/* Use graphical tombstone (rip.xpm) */
# endif
# ifndef DEFAULT_WC_TILED_MAP
#  define DEFAULT_WC_TILED_MAP	/* Default to tiles */
# endif
#endif


/*
 * Section 2:	Some global parameters and filenames.
 *		Commenting out WIZARD, LOGFILE, NEWS or PANICLOG removes that
 *		feature from the game; otherwise set the appropriate wizard
 *		name.  LOGFILE, NEWS and PANICLOG refer to files in the
 *		playground.
 *
 *		If SYSCF is defined, the following configuration info is
 *		available in a global config space, with the compiled-in
 *		entries as defaults:
 *		WIZARDS		(a value of * allows anyone to be wizard)
 *                              (this does NOT default to compiled-in value)
 *		MAXPLAYERS	(see MAX_NR_OF_PLAYERS above and nethack.sh)
 *		SUPPORT		(how to get local support)(no default)
 *		RECOVER		(how to recover a game at your site)(no default)
 *		SHELLERS	(who can use !, syntax as WIZARDS)
 *		  for the record file (see topten.c):
 *		PERSMAX		(max entries for one person)
 *		ENTRYMAX	(max entries in the record file)
 *		POINTSMIN	(min points to get an entry)
 *		PERS_IS_UID	(0 or 1 - person is name or (numeric) userid)
 *		SEDUCE		(0 or 1 - runtime disable/enable SEDUCE option)
 *
 *		The following options select how the config space is stored:
 *		SYSCF_FILE	in the named file
 *
 *              The following options pertain to crash reporting:
 *              GREPPATH	(the path to the system grep(1) utility)
 *              GDBPATH		(the path to the system gdb(1) program)
 */

#ifndef WIZARD		/* allow for compile-time or Makefile changes */
# ifndef KR1ED
#  define WIZARD  "wizard" /* the person allowed to use the -D option */
/* #define SYSCF */	/* use a global configuration */
/* #define SYSCF_FILE "sysconf" */ /* global configuration is in a file */
# else
#  define WIZARD
#  define WIZARD_NAME "wizard"
# endif
#endif

#ifndef GDBPATH
# define GDBPATH "/usr/bin/gdb"
#endif
#ifndef GREPPATH
# define GREPPATH "/bin/grep"
#endif

#define LOGFILE "logfile"	/* larger file for debugging purposes */
#define NEWS "news"		/* the file containing the latest hack news */
#define PANICLOG "paniclog"	/* log of panic and impossible events */

#ifndef PERSMAX
# define PERSMAX        3       /* entries per name/uid per char. allowed */
#endif
#ifndef POINTSMIN
# define POINTSMIN      1       /* must be > 0 */
#endif
#ifndef ENTRYMAX
# define ENTRYMAX       100     /* must be >= 10 */
#endif
#ifndef PERS_IS_UID
# if !defined(MICRO) && !defined(MAC) && !defined(WIN32)
#  define PERS_IS_UID   1         /* delete for PERSMAX per name; now per uid */
# else
#  define PERS_IS_UID   0
# endif
#endif


/*
 *	If COMPRESS is defined, it should contain the full path name of your
 *	'compress' program.
 *
 *	If you define COMPRESS, you must also define COMPRESS_EXTENSION
 *	as the extension your compressor appends to filenames after
 *	compression. Currently, only UNIX fully implements
 *	COMPRESS; other ports should be able to uncompress save files a
 *	la unixmain.c if so inclined.
 *
 *	Defining ZLIB_COMP builds in support for zlib compression. If you
 *	define ZLIB_COMP, you must link with a zlib library. Not all ports
 *	support ZLIB_COMP.
 *
 *	COMPRESS and ZLIB_COMP are mutually exclusive.
 *
 */
 
#if defined(UNIX) && !defined(ZLIB_COMP) && !defined(COMPRESS)
/* path and file name extension for compression program */
#define COMPRESS "/usr/bin/compress"	/* Lempel-Ziv compression */
#define COMPRESS_EXTENSION ".Z"		/* compress's extension */
/* An example of one alternative you might want to use: */
/* #define COMPRESS "/usr/local/bin/gzip" */	/* FSF gzip compression */
/* #define COMPRESS_EXTENSION ".gz" */		/* normal gzip extension */
#endif

#ifndef COMPRESS
/* # define ZLIB_COMP		*/	/* ZLIB for compression */
#endif

/*
 *	Internal Compression Options
 *
 *	Internal compression options RLECOMP and ZEROCOMP alter the data
 *	that gets written to the save file by NetHack, in contrast
 *	to COMPRESS or ZLIB_COMP which compress the entire file after
 *	the NetHack data is written out.
 *
 *	Defining RLECOMP builds in support for internal run-length
 *	compression of level structures. If RLECOMP support is included
 *	it can be toggled on/off at runtime via the config file option
 *	rlecomp.
 *
 *	Defining ZEROCOMP builds in support for internal zero-comp
 *	compression of data. If ZEROCOMP support is included it can still
 *	be toggled on/off at runtime via the config file option zerocomp.
 *
 *	RLECOMP and ZEROCOMP support can be included even if
 *	COMPRESS or ZLIB_COMP support is included. One reason for doing
 *	so would be to provide savefile read compatibility with a savefile
 *	where those options were in effect. With RLECOMP and/or ZEROCOMP
 *	defined, NetHack can read an rlecomp or zerocomp savefile in, yet
 *	re-save without them.
 *
 *	Using any compression option will create smaller bones/level/save
 *	files at the cost of additional code and time.
 */

/* # define INTERNAL_COMP	*/	/* defines both ZEROCOMP and RLECOMP */
/* # define ZEROCOMP		*/	/* Support ZEROCOMP compression */
/* # define RLECOMP		*/	/* Support RLECOMP compression  */

/*
 *	Data librarian.  Defining DLB places most of the support files into
 *	a tar-like file, thus making a neater installation.  See *conf.h
 *	for detailed configuration.
 */
/* #define DLB */	/* not supported on all platforms */

/*
 *	Defining INSURANCE slows down level changes, but allows games that
 *	died due to program or system crashes to be resumed from the point
 *	of the last level change, after running a utility program.
 */
#define INSURANCE	/* allow crashed game recovery */

#ifndef MAC
# define CHDIR		/* delete if no chdir() available */
#endif

#ifdef CHDIR
/*
 * If you define HACKDIR, then this will be the default playground;
 * otherwise it will be the current directory.
 */
# ifndef HACKDIR
#  define HACKDIR "/usr/games/lib/nethackdir"
# endif

/*
 * Some system administrators are stupid enough to make Hack suid root
 * or suid daemon, where daemon has other powers besides that of reading or
 * writing Hack files.	In such cases one should be careful with chdir's
 * since the user might create files in a directory of his choice.
 * Of course SECURE is meaningful only if HACKDIR is defined.
 */
/* #define SECURE */	/* do setuid(getuid()) after chdir() */

/*
 * If it is desirable to limit the number of people that can play Hack
 * simultaneously, define HACKDIR, SECURE and MAX_NR_OF_PLAYERS (or use
 * MAXPLAYERS under SYSCF).
 * #define MAX_NR_OF_PLAYERS 6
 */
#endif /* CHDIR */

/*
 * Enable some UNICODE support.
 */
/*#define UNICODE_SUPPORT */		/* master on/off for any unicode support    */


/*
 * Section 3:	Definitions that may vary with system type.
 *		For example, both schar and uchar should be short ints on
 *		the AT&T 3B2/3B5/etc. family.
 */

/*
 * Uncomment the following line if your compiler doesn't understand the
 * 'void' type (and thus would give all sorts of compile errors without
 * this definition).
 */
/* #define NOVOID */			/* define if no "void" data type. */

/*
 * Uncomment the following line if your compiler falsely claims to be
 * a standard C compiler (i.e., defines __STDC__ without cause).
 * Examples are Apollo's cc (in some versions) and possibly SCO UNIX's rcc.
 */
/* #define NOTSTDC */			/* define for lying compilers */

#include "tradstdc.h"

/*
 * type schar: small signed integers (8 bits suffice) (eg. TOS)
 *
 *	typedef char	schar;
 *
 *	will do when you have signed characters; otherwise use
 *
 *	typedef short int schar;
 */
#ifdef AZTEC
# define schar	char
#else
typedef signed char	schar;
#endif

/*
 * type uchar: small unsigned integers (8 bits suffice - but 7 bits do not)
 *
 *	typedef unsigned char	uchar;
 *
 *	will be satisfactory if you have an "unsigned char" type;
 *	otherwise use
 *
 *	typedef unsigned short int uchar;
 */
#ifndef _AIX32		/* identical typedef in system file causes trouble */
typedef unsigned char	uchar;
#endif

/*
 * Various structures have the option of using bitfields to save space.
 * If your C compiler handles bitfields well (e.g., it can initialize structs
 * containing bitfields), you can define BITFIELDS.  Otherwise, the game will
 * allocate a separate character for each bitfield.  (The bitfields used never
 * have more than 7 bits, and most are only 1 bit.)
 */
#define BITFIELDS	/* Good bitfield handling */

/* #define STRNCMPI */	/* compiler/library has the strncmpi function */

/*
 * There are various choices for the NetHack vision system.  There is a
 * choice of two algorithms with the same behavior.  Defining VISION_TABLES
 * creates huge (60K) tables at compile time, drastically increasing data
 * size, but runs slightly faster than the alternate algorithm.  (MSDOS in
 * particular cannot tolerate the increase in data size; other systems can
 * flip a coin weighted to local conditions.)
 *
 * If VISION_TABLES is not defined, things will be faster if you can use
 * MACRO_CPATH.  Some cpps, however, cannot deal with the size of the
 * functions that have been macroized.
 */

/* #define VISION_TABLES */ /* use vision tables generated at compile time */
#ifndef VISION_TABLES
# ifndef NO_MACRO_CPATH
#  define MACRO_CPATH	/* use clear_path macros instead of functions */
# endif
#endif

/*
 * Section 4:  THE FUN STUFF!!!
 *
 * Conditional compilation of special options are controlled here.
 * If you define the following flags, you will add not only to the
 * complexity of the game but also to the size of the load module.
 */

/* display features */
#define LOADSYMSETS	/* loadable symbol sets; only default symbols w/o this  */
/* dungeon features */
#define SINKS		/* Kitchen sinks - Janet Walz */
/* dungeon levels */
#define WALLIFIED_MAZE	/* Fancy mazes - Jean-Christophe Collet */
#define REINCARNATION	/* Special Rogue-like levels */
/* monsters & objects */
#define KOPS		/* Keystone Kops by Scott R. Turner */
#define SEDUCE		/* Succubi/incubi seduction, by KAA, suggested by IM */
#define STEED		/* Riding steeds */
#define TOURIST		/* Tourist players with cameras and Hawaiian shirts */
/* difficulty */
#define ELBERETH	/* Engraving the E-word repels monsters */
/* I/O */
#define REDO		/* support for redoing last command - DGK */
#if !defined(MAC)
# if !defined(NOCLIPPING)
# define CLIPPING	/* allow smaller screens -- ERS */
# endif
#endif
#define AUTOPICKUP_EXCEPTIONS	/* exceptions to autopickup */
#define BARGETHROUGH	/* allow some monsters to move others out of their way */

#ifdef REDO
# define DOAGAIN '\001' /* ^A, the "redo" key used in cmd.c and getline.c */
#endif

#define EXP_ON_BOTL	/* Show experience on bottom line */
/* #define SCORE_ON_BOTL */	/* added by Gary Erickson (erickson@ucivax) */

/*
 * Section 5:  EXPERIMENTAL STUFF
 *
 * Conditional compilation of new or experimental options are controlled here.
 * Enable any of these at your own risk -- there are almost certainly
 * bugs left here.
 */

/*#define GOLDOBJ */	/* Gold is kept on obj chains - Helge Hafting */
#define STATUS_VIA_WINDOWPORT	/* re-work of the status line updating process */
#define STATUS_HILITES		/* support hilites of status fields */
#define DUNGEON_OVERVIEW	/* dungeon overview by Hojita Discordia */
/* #define WINCHAIN*/		/* stacked window systems */
/* End of Section 5 */

#include "global.h"	/* Define everything else according to choices above */

#endif /* CONFIG_H */

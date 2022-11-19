/* NetHack 3.7	config.h	$NHDT-Date: 1610141601 2021/01/08 21:33:21 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.148 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef CONFIG_H /* make sure the compiler does not see the typedefs twice */
#define CONFIG_H

/*
 * Section 1:   Operating and window systems selection.
 *              Select the version of the OS you are using.
 *              For "UNIX" select BSD, ULTRIX, SYSV, or HPUX in unixconf.h.
 *              A "VMS" option is not needed since the VMS C-compilers
 *              provide it (no need to change sec#1, vmsconf.h handles it).
 *              MacOSX uses the UNIX configuration, not the old MAC one.
 */

#define UNIX /* delete if no fork(), exec() available */

/* #define MSDOS */ /* in case it's not auto-detected */

/* #define OS2 */ /* define for OS/2 */

/* #define TOS */ /* define for Atari ST/TT */

/* #define STUPID */ /* avoid some complicated expressions if
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

#include "config1.h" /* should auto-detect MSDOS, MAC, AMIGA, and WIN32 */

/*
 * Consolidated version, patchlevel, development status.
 */
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif


/* Windowing systems...
 * Define all of those you want supported in your binary.
 * Some combinations make no sense.  See the installation document.
 */
#if !defined(NOTTYGRAPHICS)
#define TTY_GRAPHICS /* good old tty-based graphics */
#endif
/* #define CURSES_GRAPHICS *//* Curses interface - Karl Garrison*/
/* #define X11_GRAPHICS */   /* X11 interface */
/* #define QT_GRAPHICS */    /* Qt interface */
/* #define MSWIN_GRAPHICS */ /* Windows NT, CE, Graphics */

/*
 * Define the default window system.  This should be one that is compiled
 * into your system (see defines above).  Known window systems are:
 *
 *      tty, X11, mac, amii, BeOS, Qt, Gem, Gnome, shim
 */

/* MAC also means MAC windows */
#ifdef MAC
#ifndef AUX
#define DEFAULT_WINDOW_SYS "mac"
#endif
#endif

/* Amiga supports AMII_GRAPHICS and/or TTY_GRAPHICS */
#ifdef AMIGA
#define AMII_GRAPHICS             /* (optional) */
#define DEFAULT_WINDOW_SYS "amii" /* "amii", "amitile" or "tty" */
#endif

/* Atari supports GEM_GRAPHICS and/or TTY_GRAPHICS */
#ifdef TOS
#define GEM_GRAPHICS             /* Atari GEM interface (optional) */
#define DEFAULT_WINDOW_SYS "Gem" /* "Gem" or "tty" */
#endif

#ifdef __BEOS__
#define BEOS_GRAPHICS             /* (optional) */
#define DEFAULT_WINDOW_SYS "BeOS" /* "tty" */
#ifndef HACKDIR                   /* override the default hackdir below */
#define HACKDIR "/boot/apps/NetHack"
#endif
#endif

#ifdef QT_GRAPHICS
#ifndef DEFAULT_WC_TILED_MAP
#define DEFAULT_WC_TILED_MAP /* Default to tiles if users doesn't say \
                                wc_ascii_map */
#endif
#ifndef NOUSER_SOUNDS
#define USER_SOUNDS /* Use sounds */
#endif
#ifndef USE_XPM
#define USE_XPM           /* Use XPM format for images (required) */
#endif
#ifndef GRAPHIC_TOMBSTONE
#define GRAPHIC_TOMBSTONE /* Use graphical tombstone (rip.ppm) */
#endif
#ifndef DEFAULT_WINDOW_SYS
#define DEFAULT_WINDOW_SYS "Qt"
#endif
#endif

#ifdef GNOME_GRAPHICS
#ifndef USE_XPM
#define USE_XPM           /* Use XPM format for images (required) */
#endif
#ifndef GRAPHIC_TOMBSTONE
#define GRAPHIC_TOMBSTONE /* Use graphical tombstone (rip.ppm) */
#endif
#ifndef DEFAULT_WINDOW_SYS
#define DEFAULT_WINDOW_SYS "Gnome"
#endif
#endif

#ifdef MSWIN_GRAPHICS
#ifndef DEFAULT_WINDOW_SYS
#define DEFAULT_WINDOW_SYS "mswin"
#endif
#define HACKDIR "\\nethack"
#endif

#ifdef TTY_GRAPHICS
#ifndef DEFAULT_WINDOW_SYS
#define DEFAULT_WINDOW_SYS "tty"
#endif
#endif

#ifdef CURSES_GRAPHICS
#ifndef DEFAULT_WINDOW_SYS
#define DEFAULT_WINDOW_SYS "curses"
#endif
#endif

#ifdef SHIM_GRAPHICS
#ifndef DEFAULT_WINDOW_SYS
#define DEFAULT_WINDOW_SYS "shim"
#endif
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
/* # define USE_XPM */ /* Disable if you do not have the XPM library */
#ifdef USE_XPM
#ifndef GRAPHIC_TOMBSTONE
#define GRAPHIC_TOMBSTONE /* Use graphical tombstone (rip.xpm) */
#endif
#endif
#ifndef DEFAULT_WC_TILED_MAP
#define DEFAULT_WC_TILED_MAP /* Default to tiles */
#endif
#endif

/*
 * Section 2:   Some global parameters and filenames.
 *
 *              LOGFILE, XLOGFILE, LIVELOGFILE, NEWS and PANICLOG refer to
 *              files in the playground directory.  Commenting out LOGFILE,
 *              XLOGFILE, NEWS or PANICLOG removes that feature from the game.
 *
 *              Building with debugging features enabled is now unconditional;
 *              the old WIZARD setting for that has been eliminated.
 *              If SYSCF is enabled, WIZARD_NAME will be overridden at
 *              runtime by the SYSCF WIZARDS value.
 *
 *              SYSCF:  (not supported by all ports)
 *            If SYSCF is defined, the following configuration info is
 *            available in a global config space, with the compiled-in
 *            entries as defaults:
 *              WIZARDS      (a space-separated list of usernames of users who
 *                           can run the game in debug mode, aka wizard mode;
 *                           a value of * allows anyone to debug;
 *                           this does NOT default to compiled-in value)
 *              EXPLORERS    (who can use explore mode, aka discover mode)
 *              SHELLERS     (who can use ! to execute a shell subprocess)
 *              MAXPLAYERS   (see MAX_NR_OF_PLAYERS below and nethack.sh)
 *              SUPPORT      (how to get local support) [no default]
 *              RECOVER      (how to recover a game at your site) [no default]
 *            For the record file (see topten.c):
 *              PERSMAX      (max entries for one person)
 *              ENTRYMAX     (max entries in the record file)
 *              POINTSMIN    (min points to get an entry)
 *              PERS_IS_UID  (0 or 1 - person is name or (numeric) userid)
 *            Can force incubi/succubi behavior to be toned down to nymph-like:
 *              SEDUCE       (0 or 1 - runtime disable/enable SEDUCE option)
 *            Can hide the entry for displaying command line usage from
 *            the help menu if players don't have access to command lines:
 *              HIDEUSAGE    (0 or 1 - runtime show/hide command line usage)
 *            The following options pertain to crash reporting:
 *              GREPPATH     (the path to the system grep(1) utility)
 *              GDBPATH      (the path to the system gdb(1) program)
 *            Regular nethack options can also be specified in order to
 *            provide system-wide default values local to your system:
 *              OPTIONS      (same as in users' .nethackrc or defaults.nh)
 *
 *              In the future there may be other ways to supply SYSCF
 *              information (Windows registry, Apple resource forks, etc)
 *              but at present the only supported method is via a text file.
 *              If the program is built with SYSCF enabled, the file *must*
 *              exist and be readable, otherwise the game will complain and
 *              refuse to start.
 *              SYSCF_FILE:  file containing the SYSCF options shown above;
 *              default is 'sysconf' in nethack's playground.
 */

#ifndef WIZARD_NAME /* allow for compile-time or Makefile changes */
#define WIZARD_NAME "wizard" /* value is ignored if SYSCF is enabled */
#endif

#ifndef SYSCF
#define SYSCF                /* use a global configuration */
#define SYSCF_FILE "sysconf" /* global configuration is in a file */
#endif

#ifndef GDBPATH
#define GDBPATH "/usr/bin/gdb"
#endif
#ifndef GREPPATH
#define GREPPATH "/bin/grep"
#endif

/* note: "larger" is in comparison with 'record', the high-scores file
   (whose name can be overridden via #define in global.h if desired) */
#define LOGFILE  "logfile"  /* larger file for debugging purposes */
#define XLOGFILE "xlogfile" /* even larger logfile */
#define NEWS     "news"     /* the file containing the latest hack news */
#define PANICLOG "paniclog" /* log of panic and impossible events */

/* alternative paniclog format, better suited for public servers with
   many players, as it saves the player name and the game start time */
/* #define PANICLOG_FMT2 */

/*
 *      PERSMAX, POINTSMIN, ENTRYMAX, PERS_IS_UID:
 *      These control the contents of 'record', the high-scores file.
 *      They used to be located in topten.c rather than config.h, and
 *      their values can be overridden at runtime (to increase ENTRYMAX, the
 *      maximum number of scores to keep, for example) if SYSCF is enabled.
 */
#ifndef PERSMAX
#define PERSMAX 3 /* entries per name/uid per char. allowed */
#endif
#ifndef POINTSMIN
#define POINTSMIN 1 /* must be > 0 */
#endif
#ifndef ENTRYMAX
#define ENTRYMAX 100 /* must be >= 10 */
#endif
#ifndef PERS_IS_UID
#if !defined(MICRO) && !defined(MAC) && !defined(WIN32)
#define PERS_IS_UID 1 /* delete for PERSMAX per name; now per uid */
#else
#define PERS_IS_UID 0
#endif
#endif

/*
 *      NODUMPENUMS
 *      If there are memory constraints and you don't want to store information
 *      about the internal enum values for monsters and objects, this can be
 *      uncommented to define NODUMPENUMS. Doing so will disable the
 *          nethack --dumpenums
 *      command line option.
 *      Note:  the extra memory is also used when ENHANCED_SYMBOLS is
 *      defined, so defining both ENHANCED_SYMBOLS and NODUMPENUMS will limit
 *      the amount of memory and code reduction offered by the latter.
 */
/* #define NODUMPENUMS */

/*
 *      ENHANCED_SYMBOLS
 *      Support the enhanced display of symbols by utilizing utf8 and 24-bit
 *      color sequences. Enabled by default, but it can be disabled by
 *      commenting it out.
 */

#define ENHANCED_SYMBOLS

/*
 *      If COMPRESS is defined, it should contain the full path name of your
 *      'compress' program.
 *
 *      If you define COMPRESS, you must also define COMPRESS_EXTENSION
 *      as the extension your compressor appends to filenames after
 *      compression. Currently, only UNIX fully implements
 *      COMPRESS; other ports should be able to uncompress save files a
 *      la unixmain.c if so inclined.
 *
 *      Defining ZLIB_COMP builds in support for zlib compression. If you
 *      define ZLIB_COMP, you must link with a zlib library. Not all ports
 *      support ZLIB_COMP.
 *
 *      COMPRESS and ZLIB_COMP are mutually exclusive.
 *
 */

#if defined(UNIX) && !defined(ZLIB_COMP) && !defined(COMPRESS)
/* path and file name extension for compression program */
#define COMPRESS "/usr/bin/compress" /* Lempel-Ziv compression */
#define COMPRESS_EXTENSION ".Z"      /* compress's extension */
/* An example of one alternative you might want to use: */
/* #define COMPRESS "/usr/local/bin/gzip" */ /* FSF gzip compression */
/* #define COMPRESS_EXTENSION ".gz" */       /* normal gzip extension */
#endif

#ifndef COMPRESS
/* # define ZLIB_COMP */            /* ZLIB for compression */
#endif

/*
 *      Internal Compression Options
 *
 *      Internal compression options RLECOMP and ZEROCOMP alter the data
 *      that gets written to the save file by NetHack, in contrast
 *      to COMPRESS or ZLIB_COMP which compress the entire file after
 *      the NetHack data is written out.
 *
 *      Defining RLECOMP builds in support for internal run-length
 *      compression of level structures. If RLECOMP support is included
 *      it can be toggled on/off at runtime via the config file option
 *      rlecomp.
 *
 *      Defining ZEROCOMP builds in support for internal zero-comp
 *      compression of data. If ZEROCOMP support is included it can still
 *      be toggled on/off at runtime via the config file option zerocomp.
 *
 *      RLECOMP and ZEROCOMP support can be included even if
 *      COMPRESS or ZLIB_COMP support is included. One reason for doing
 *      so would be to provide savefile read compatibility with a savefile
 *      where those options were in effect. With RLECOMP and/or ZEROCOMP
 *      defined, NetHack can read an rlecomp or zerocomp savefile in, yet
 *      re-save without them.
 *
 *      Using any compression option will create smaller bones/level/save
 *      files at the cost of additional code and time.
 */

/* # define INTERNAL_COMP */ /* defines both ZEROCOMP and RLECOMP */
/* # define ZEROCOMP      */ /* Support ZEROCOMP compression */
/* # define RLECOMP       */ /* Support RLECOMP compression  */

/*
 *      Data librarian.  Defining DLB places most of the support files into
 *      a tar-like file, thus making a neater installation.  See *conf.h
 *      for detailed configuration.
 */
/* #define DLB */ /* not supported on all platforms */

/*
 *      Defining REPRODUCIBLE_BUILD causes 'util/makedefs -v' to construct
 *      date+time in include/date.h (to be shown by nethack's 'v' command)
 *      from SOURCE_DATE_EPOCH in the build environment rather than use
 *      current date+time when makedefs is run.
 *
 *      [The version string will show "last revision <date><time>" instead
 *      of "last build <date><time>" if SOURCE_DATE_EPOCH has a value
 *      which seems valid at the time date.h is generated.  The person
 *      building the program is responsible for setting it correctly,
 *      and the value should be in UTC rather than local time.  NetHack
 *      normally uses local time and doesn't display timezone so toggling
 *      REPRODUCIBLE_BUILD on or off might yield a date+time that appears
 *      to be incorrect relative to what the other setting produced.]
 *
 *      Intent is to be able to rebuild the program with the same value
 *      and obtain an identical copy as was produced by a previous build.
 *      Not necessary for normal game play....
 */
/* #define REPRODUCIBLE_BUILD */ /* use getenv("SOURCE_DATE_EPOCH") instead
                                    of current time when creating date.h */

/*
 *      Defining INSURANCE slows down level changes, but allows games that
 *      died due to program or system crashes to be resumed from the point
 *      of the last level change, after running a utility program.
 */
#define INSURANCE /* allow crashed game recovery */

#ifndef MAC
#define CHDIR /* delete if no chdir() available */
#endif

#ifdef CHDIR
/*
 * If you define HACKDIR, then this will be the default playground;
 * otherwise it will be the current directory.
 */
#ifndef HACKDIR
#define HACKDIR "/usr/games/lib/nethackdir"
#endif

/*
 * Some system administrators are stupid enough to make Hack suid root
 * or suid daemon, where daemon has other powers besides that of reading or
 * writing Hack files.  In such cases one should be careful with chdir's
 * since the user might create files in a directory of his choice.
 * Of course SECURE is meaningful only if HACKDIR is defined.
 */
/* #define SECURE */ /* do setuid(getuid()) after chdir() */

/*
 * If it is desirable to limit the number of people that can play Hack
 * simultaneously, define HACKDIR, SECURE and MAX_NR_OF_PLAYERS (or use
 * MAXPLAYERS under SYSCF).
 * #define MAX_NR_OF_PLAYERS 6
 */
#endif /* CHDIR */


/*
 * Section 3:   Definitions that may vary with system type.
 *              For example, both schar and uchar should be short ints on
 *              the AT&T 3B2/3B5/etc. family.
 */

/*
 * Uncomment the following line if your compiler doesn't understand the
 * 'void' type (and thus would give all sorts of compile errors without
 * this definition).
 */
/* #define NOVOID */ /* define if no "void" data type. */

/*
 * Uncomment the following line if your compiler falsely claims to be
 * a standard C compiler (i.e., defines __STDC__ without cause).
 * Examples are Apollo's cc (in some versions) and possibly SCO UNIX's rcc.
 */
/* #define NOTSTDC */ /* define for lying compilers */

#include "tradstdc.h"

/*
 * type schar:
 * small signed integers (8 bits suffice) (eg. TOS)
 *      typedef char schar;
 * will do when you have signed characters; otherwise use
 *      typedef short int schar;
 */
#ifdef AZTEC
#define schar char
#else
typedef signed char schar;
#endif

/*
 * type uchar:
 * small unsigned integers (8 bits suffice - but 7 bits do not)
 *      typedef unsigned char uchar;
 * will be satisfactory if you have an "unsigned char" type; otherwise use
 *      typedef unsigned short int uchar;
 */
#ifndef _AIX32 /* identical typedef in system file causes trouble */
typedef unsigned char uchar;
#endif

/*
 * Various structures have the option of using bitfields to save space.
 * If your C compiler handles bitfields well (e.g., it can initialize structs
 * containing bitfields), you can define BITFIELDS.  Otherwise, the game will
 * allocate a separate character for each bitfield.  (The bitfields used never
 * have more than 7 bits, and most are only 1 bit.)
 */
#define BITFIELDS /* Good bitfield handling */

/* #define STRNCMPI */ /* compiler/library has the strncmpi function */

/*
 * Vision choices.
 *
 * Things will be faster if you can use MACRO_CPATH.  Some cpps, however,
 * cannot deal with the size of the functions that have been macroized.
 */

#ifndef NO_MACRO_CPATH
#define MACRO_CPATH /* use clear_path macros instead of functions */
#endif

#if !defined(MAC)
#if !defined(NOCLIPPING)
#define CLIPPING /* allow smaller screens -- ERS */
#endif
#endif

/* CONFIG_ERROR_SECURE: If user makes NETHACKOPTIONS point to a file ...
 *  TRUE: Show the first error, nothing else.
 *  FALSE: Show all errors as normal, with line numbers and context.
 */
#ifndef CONFIG_ERROR_SECURE
# define CONFIG_ERROR_SECURE TRUE
#endif

/*
 * Section 4:  EXPERIMENTAL STUFF
 *
 * Conditional compilation of new or experimental options are controlled here.
 * Enable any of these at your own risk -- there are almost certainly
 * bugs left here.
 */

/* SELECTSAVED: Enable the 'selectsaved' run-time option, allowing it
 * to be set in user's config file or NETHACKOPTIONS.  When set, if
 * player is about to be given the "who are you?" prompt, check for
 * save files and if any are found, put up a menu of them for choosing
 * one to restore (plus extra menu entries "new game" and "quit").
 *
 * Not useful if players are forced to use a specific character name
 * such as their user name.  However in some cases, players can set
 * their character name to one which is classified as generic in the
 * sysconf file (such as "player" or "games")
 *  nethack -u player
 * to force the "who are you?" prompt in which case 'selectsaved' will
 * be honored.
 *
 * Comment out if the wildcard file name lookup in files.c doesn't
 * compile or doesn't work as intended.
 */
#define SELECTSAVED /* support for restoring via menu */

/* TTY_TILES_ESCCODES: Enable output of special console escape codes
 * which act as hints for external programs such as EbonHack, or hterm.
 *
 * TTY_SOUND_ESCCODES: Enable output of special console escape codes
 * which act as hints for theoretical external programs to play sound effect.
 *
 * Only for TTY_GRAPHICS.
 *
 * All of the escape codes are in the format ESC [ N z, where N can be
 * one or more positive integer values, separated by semicolons.
 * For example ESC [ 1 ; 0 ; 120 z
 *
 * Possible TTY_TILES_ESCCODES codes are:
 *  ESC [ 1 ; 0 ; n ; m z   Start a glyph (aka a tile) number n, with flags m
 *  ESC [ 1 ; 1 z           End a glyph.
 *  ESC [ 1 ; 2 ; n z       Select a window n to output to.
 *  ESC [ 1 ; 3 z           End of data. NetHack has finished sending data,
 *                          and is waiting for input.
 * Possible TTY_SOUND_ESCCODES codes are:
 *  ESC [ 1 ; 4 ; n ; m z   Play specified sound n, volume m
 *
 * Whenever NetHack outputs anything, it will first output the "select window"
 * code. Whenever NetHack outputs a tile, it will first output the "start
 * glyph" code, then the escape codes for color and the glyph character
 * itself, and then the "end glyph" code.
 *
 * To compile NetHack with this, add tile.c to WINSRC and tile.o to WINOBJ
 * in the hints file or Makefile.
 * Set boolean option vt_xdata in your config file to turn either of these on.
 * Note that gnome-terminal at least doesn't work with this. */
/* #define TTY_TILES_ESCCODES */
/* #define TTY_SOUND_ESCCODES */

/* An experimental minimalist inventory list capability under tty if you have
 * at least 28 additional rows beneath the status window on your terminal  */
/* #define TTY_PERM_INVENT */

/* NetHack will execute an external program whenever a new message-window
 * message is shown.  The program to execute is given in environment variable
 * NETHACK_MSGHANDLER.  It will get the message as the only parameter.
 * Only available with POSIX_TYPES, GNU C, or WIN32 */
/* #define MSGHANDLER */

/* enable status highlighting via STATUS_HILITE directives in run-time
   config file and the 'statushilites' option */
#define STATUS_HILITES         /* support hilites of status fields */

/* #define WINCHAIN */              /* stacked window systems */

#if defined(DEBUG) && !defined(DEBUG_MIGRATING_MONS)
#define DEBUG_MIGRATING_MONS  /* add a wizard-mode command to help debug
                                 migrating monsters */
#endif

/* SCORE_ON_BOTL is neither experimental nor inadequately tested,
   but doesn't seem to fit in any other section... */
/* #define SCORE_ON_BOTL */         /* enable the 'showscore' option to
                                       show estimated score on status line */

/* FREE_ALL_MEMORY is neither experimental nor inadequately tested,
   but it isn't necessary for successful operation of the program */
#define FREE_ALL_MEMORY             /* free all memory at exit */

/* EXTRA_SANITY_CHECKS adds extra impossible calls,
 * probably not useful for normal play */
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
#define EXTRA_SANITY_CHECKS
#endif

/* BREADCRUMBS employs the use of predefined compiler macros
 * __FUNCTION__ and __LINE__ to store some caller breadcrumbs
 * for use during heavy debugging sessions. Only define if your
 * compiler supports those predefined macros and you are debugging */
/* #define BREADCRUMBS */

/* EDIT_GETLIN makes the string input in TTY, curses, Qt4, and X11
   for some prompts be pre-loaded with previously input text (from
   a previous instance of the same prompt) as the default response.
   In some cases, the previous instance can only be within the same
   session; in others, such as #annotate, the previous input can be
   from any session because the response is saved and restored with
   the map.  The 'edit' capability is just <delete> or <backspace>
   to strip off characters at the end, or <escape> to discard the
   whole thing, then type a new end for the text. */
/* #define EDIT_GETLIN */

#ifndef NO_CHRONICLE
/* CHRONICLE - enable #chronicle command, a log of major game events.
   The logged messages will also appear in DUMPLOG. */
#define CHRONICLE
#ifdef CHRONICLE
/* LIVELOG - log CHRONICLE events into LIVELOGFILE as they happen. */
/* #define LIVELOG */
#ifdef LIVELOG
#define LIVELOGFILE "livelog" /* in-game events recorded, live */
#endif /* LIVELOG */
#endif /* CHRONICLE */
#else
#undef LIVELOG
#endif /* NO_CHRONICLE */

/* #define DUMPLOG */  /* End-of-game dump logs */
#ifdef DUMPLOG

#ifndef DUMPLOG_MSG_COUNT
#define DUMPLOG_MSG_COUNT   50
#endif

#ifndef DUMPLOG_FILE
#define DUMPLOG_FILE        "/tmp/nethack.%n.%d.log"
/* DUMPLOG_FILE allows following placeholders:
   %% literal '%'
   %v version (eg. "3.6.3-0")
   %u game UID
   %t game start time, UNIX timestamp format
   %T current time, UNIX timestamp format
   %d game start time, YYYYMMDDhhmmss format
   %D current time, YYYYMMDDhhmmss format
   %n player name
   %N first character of player name
   DUMPLOG_FILE is not used if SYSCF is defined
*/
#endif

#endif

#define USE_ISAAC64 /* Use cross-plattform, bundled RNG */

/* TEMPORARY - MAKE UNCONDITIONAL BEFORE RELEASE */
/* undef this to check if sandbox breaks something */
#define NHL_SANDBOX

/* End of Section 4 */

#ifdef TTY_TILES_ESCCODES
# ifndef USE_TILES
#  define USE_TILES
# endif
#endif

#include "integer.h"
#include "global.h" /* Define everything else according to choices above */

#endif /* CONFIG_H */

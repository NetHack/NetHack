/* NetHack 3.7  mdlib.c  $NHDT-Date: 1575161954 2019/12/01 00:59:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.6 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* Copyright (c) M. Stephenson, 1990, 1991.                       */
/* Copyright (c) Dean Luick, 1990.                                */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This can be linked into a binary to provide the functionality
 * via the contained functions, or it can be #included directly
 * into util/makedefs.c to provide it there.
 */

#ifndef MAKEDEFS_C
#define MDLIB_C
#include "config.h"
#ifdef MONITOR_HEAP
#undef free /* makedefs, mdlib don't use the alloc and free in src/alloc.c */
#endif
#include "permonst.h"
#include "objclass.h"
#include "monsym.h"
#include "artilist.h"
#include "dungeon.h"
#include "obj.h"
#include "monst.h"
#include "you.h"
#include "context.h"
#include "flag.h"
#include "dlb.h"
#include <ctype.h>
/* version information */
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif
#define Fprintf (void) fprintf
#define Fclose (void) fclose
#define Unlink (void) unlink
#if !defined(AMIGA) || defined(AZTEC_C)
#define rewind(fp) fseek((fp), 0L, SEEK_SET) /* guarantee a return value */
#endif  /* AMIGA || AZTEC_C */
#else
#ifndef GLOBAL_H
#include "global.h"
#endif
#endif  /* !MAKEDEFS_C */

/* shorten up some lines */
#if defined(CROSSCOMPILE_TARGET) || defined(OPTIONS_AT_RUNTIME)
#define FOR_RUNTIME
#endif

#if defined(MAKEDEFS_C) || defined(FOR_RUNTIME)
/* REPRODUCIBLE_BUILD will change this to TRUE */
static boolean date_via_env = FALSE;

static char *FDECL(version_string, (char *, const char *));
static char *FDECL(version_id_string, (char *, const char *));
static char *FDECL(bannerc_string, (char *, const char *));

static int FDECL(case_insensitive_comp, (const char *, const char *));
static void NDECL(make_version);
static char *FDECL(eos, (char *));
static char *FDECL(mdlib_strsubst, (char *, const char *, const char *));
#endif /* MAKEDEFS_C || FOR_RUNTIME */
#if !defined(MAKEDEFS_C) && defined(WIN32)
extern int GUILaunched;
#endif

/* these two are in extern.h but we don't include hack.h */
void NDECL(runtime_info_init);
const char *FDECL(do_runtime_info, (int *));

void NDECL(build_options);
static int NDECL(count_and_validate_winopts);
static void FDECL(opt_out_words, (char *, int *));
static void NDECL(build_savebones_compat_string);
static int idxopttext, done_runtime_opt_init_once = 0;
#define MAXOPT 40
static char rttimebuf[MAXOPT];
static char *opttext[120] = { 0 };
char optbuf[BUFSZ];
static struct version_info version;
static const char opt_indent[] = "    ";

struct win_info {
    const char *id, /* DEFAULT_WINDOW_SYS string */
        *name;      /* description, often same as id */
    boolean valid;
};

static struct win_info window_opts[] = {
#ifdef TTY_GRAPHICS
    { "tty",
      /* testing 'USE_TILES' here would bring confusion because it could
         apply to another interface such as X11, so check MSDOS explicitly
         instead; even checking TTY_TILES_ESCCODES would probably be
         confusing to most users (and it will already be listed separately
         in the compiled options section so users aware of it can find it) */
#ifdef MSDOS
      "traditional text with optional 'tiles' graphics",
#else
      /* assume that one or more of IBMgraphics, DECgraphics, or MACgraphics
         can be enabled; we can't tell from here whether that is accurate */
      "traditional text with optional line-drawing",
#endif
      TRUE
    },
#endif /*TTY_GRAPHICS */
#ifdef CURSES_GRAPHICS
    { "curses", "terminal-based graphics", TRUE },
#endif
#ifdef X11_GRAPHICS
    { "X11", "X11", TRUE },
#endif
#ifdef QT_GRAPHICS /* too vague; there are multiple incompatible versions */
    { "Qt", "Qt", TRUE },
#endif
#ifdef GNOME_GRAPHICS /* unmaintained/defunct */
    { "Gnome", "Gnome", TRUE },
#endif
#ifdef MAC /* defunct OS 9 interface */
    { "mac", "Mac", TRUE },
#endif
#ifdef AMIGA_INTUITION /* unmaintained/defunct */
    { "amii", "Amiga Intuition", TRUE },
#endif
#ifdef GEM_GRAPHICS /* defunct Atari interface */
    { "Gem", "Gem", TRUE },
#endif
#ifdef MSWIN_GRAPHICS /* win32 */
    { "mswin", "mswin", TRUE },
#endif
#ifdef BEOS_GRAPHICS /* unmaintained/defunct */
    { "BeOS", "BeOS InterfaceKit", TRUE },
#endif
    { 0, 0, FALSE }
};

/*
 * Use this to explicitly mask out features during version checks.
 *
 * ZEROCOMP, RLECOMP, and ZLIB_COMP describe compression features
 * that the port/plaform which wrote the savefile was capable of
 * dealing with. Don't reject a savefile just because the port
 * reading the savefile doesn't match on all/some of them.
 * The actual compression features used to produce the savefile are
 * recorded in the savefile_info structure immediately following the
 * version_info, and that is what needs to be checked against the
 * feature set of the port that is reading the savefile back in.
 * That check is done in src/restore.c now.
 *
 */
#ifndef MD_IGNORED_FEATURES
#define MD_IGNORED_FEATURES              \
    (0L | (1L << 19) /* SCORE_ON_BOTL */ \
     | (1L << 27)    /* ZEROCOMP */      \
     | (1L << 28)    /* RLECOMP */       \
     )
#endif /* MD_IGNORED_FEATUES */

static void
make_version()
{
    register int i;

    /*
     * integer version number
     */
    version.incarnation = ((unsigned long) VERSION_MAJOR << 24)
                          | ((unsigned long) VERSION_MINOR << 16)
                          | ((unsigned long) PATCHLEVEL << 8)
                          | ((unsigned long) EDITLEVEL);
    /*
     * encoded feature list
     * Note:  if any of these magic numbers are changed or reassigned,
     * EDITLEVEL in patchlevel.h should be incremented at the same time.
     * The actual values have no special meaning, and the category
     * groupings are just for convenience.
     */
    version.feature_set = (unsigned long) (0L
/* levels and/or topology (0..4) */
/* monsters (5..9) */
#ifdef MAIL_STRUCTURES
                                           | (1L << 6)
#endif
/* objects (10..14) */
/* flag bits and/or other global variables (15..26) */
#ifdef TEXTCOLOR
                                           | (1L << 17)
#endif
#ifdef INSURANCE
                                           | (1L << 18)
#endif
#ifdef SCORE_ON_BOTL
                                           | (1L << 19)
#endif
/* data format (27..31)
 * External compression methods such as COMPRESS and ZLIB_COMP
 * do not affect the contents and are thus excluded from here */
#ifdef ZEROCOMP
                                           | (1L << 27)
#endif
#ifdef RLECOMP
                                           | (1L << 28)
#endif
                                               );
    /*
     * Value used for object & monster sanity check.
     *    (NROFARTIFACTS<<24) | (NUM_OBJECTS<<12) | (NUMMONS<<0)
     */
    for (i = 1; artifact_names[i]; i++)
        continue;
    version.entity_count = (unsigned long) (i - 1);
    for (i = 1; objects[i].oc_class != ILLOBJ_CLASS; i++)
        continue;
    version.entity_count = (version.entity_count << 12) | (unsigned long) i;
    for (i = 0; mons[i].mlet; i++)
        continue;
    version.entity_count = (version.entity_count << 12) | (unsigned long) i;
    /*
     * Value used for compiler (word size/field alignment/padding) check.
     */
    version.struct_sizes1 =
        (((unsigned long) sizeof(struct context_info) << 24)
         | ((unsigned long) sizeof(struct obj) << 17)
         | ((unsigned long) sizeof(struct monst) << 10)
         | ((unsigned long) sizeof(struct you)));
    version.struct_sizes2 = (((unsigned long) sizeof(struct flag) << 10) |
/* free bits in here */
#ifdef SYSFLAGS
                             ((unsigned long) sizeof(struct sysflag)));
#else
                             ((unsigned long) 0L));
#endif
    return;
}

#if defined(MAKEDEFS_C) || defined(FOR_RUNTIME)

static char *
version_string(outbuf, delim)
char *outbuf;
const char *delim;
{
    Sprintf(outbuf, "%d%s%d%s%d", VERSION_MAJOR, delim, VERSION_MINOR, delim,
            PATCHLEVEL);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
    Sprintf(eos(outbuf), "-%d", EDITLEVEL);
#endif
    return outbuf;
}

static char *
version_id_string(outbuf, build_date)
char *outbuf;
const char *build_date;
{
    char subbuf[64], versbuf[64];
    char statusbuf[64];

#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
#if (NH_DEVEL_STATUS == NH_STATUS_BETA)
    Strcpy(statusbuf, " Beta");
#else
#if (NH_DEVEL_STATUS == NH_STATUS_WIP)
    Strcpy(statusbuf, " Work-in-progress");
#else
    Strcpy(statusbuf, " post-release");
#endif
#endif
#else
    statusbuf[0] = '\0';
#endif
    subbuf[0] = '\0';
#ifdef PORT_SUB_ID
    subbuf[0] = ' ';
    Strcpy(&subbuf[1], PORT_SUB_ID);
#endif

    Sprintf(outbuf, "%s NetHack%s Version %s%s - last %s %s.", PORT_ID,
            subbuf, version_string(versbuf, "."), statusbuf,
            date_via_env ? "revision" : "build", build_date);
    return outbuf;
}

/* still within #if MAKDEFS_C || FOR_RUNTIME */

static char *
bannerc_string(outbuf, build_date)
char *outbuf;
const char *build_date;
{
    char subbuf[64], versbuf[64];

    subbuf[0] = '\0';
#ifdef PORT_SUB_ID
    subbuf[0] = ' ';
    Strcpy(&subbuf[1], PORT_SUB_ID);
#endif
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
#if (NH_DEVEL_STATUS == NH_STATUS_BETA)
    Strcat(subbuf, " Beta");
#else
    Strcat(subbuf, " Work-in-progress");
#endif
#endif

    Sprintf(outbuf, "         Version %s %s%s, %s %s.",
            version_string(versbuf, "."), PORT_ID, subbuf,
            date_via_env ? "revised" : "built", &build_date[4]);
#if 0
    Sprintf(outbuf, "%s NetHack%s %s Copyright 1985-%s (built %s)",
            PORT_ID, subbuf, version_string(versbuf,"."), RELEASE_YEAR,
            &build_date[4]);
#endif
    return outbuf;
}

#endif /* MAKEDEFS_C || FOR_RUNTIME */

static int
case_insensitive_comp(s1, s2)
const char *s1;
const char *s2;
{
    uchar u1, u2;

    for (;; s1++, s2++) {
        u1 = (uchar) *s1;
        if (isupper(u1))
            u1 = tolower(u1);
        u2 = (uchar) *s2;
        if (isupper(u2))
            u2 = tolower(u2);
        if (u1 == '\0' || u1 != u2)
            break;
    }
    return u1 - u2;
}

static char *
eos(str)
char *str;
{
    while (*str)
        str++;
    return str;
}

static char *
mdlib_strsubst(bp, orig, replacement)
char *bp;
const char *orig, *replacement;
{
    char *found, buf[BUFSZ];

    if (bp) {
        /* [this could be replaced by strNsubst(bp, orig, replacement, 1)] */
        found = strstr(bp, orig);
        if (found) {
            Strcpy(buf, found + strlen(orig));
            Strcpy(found, replacement);
            Strcat(bp, buf);
        }
    }
    return bp;
}

static char save_bones_compat_buf[BUFSZ];

static void
build_savebones_compat_string()
{
#ifdef VERSION_COMPATIBILITY
    unsigned long uver = VERSION_COMPATIBILITY,
                  cver  = (((unsigned long) VERSION_MAJOR << 24)
                         | ((unsigned long) VERSION_MINOR << 16)
                         | ((unsigned long) PATCHLEVEL    <<  8));
#endif

    Strcpy(save_bones_compat_buf,
           "save and bones files accepted from version");
#ifdef VERSION_COMPATIBILITY
    if (uver != cver)
        Sprintf(eos(save_bones_compat_buf), "s %lu.%lu.%lu through %d.%d.%d",
                ((uver >> 24) & 0x0ffUL),
                ((uver >> 16) & 0x0ffUL),
                ((uver >>  8) & 0x0ffUL),
                VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL);
    else
#endif
        Sprintf(eos(save_bones_compat_buf), " %d.%d.%d only",
                VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL);
}

static const char *build_opts[] = {
#ifdef AMIGA_WBENCH
    "Amiga WorkBench support",
#endif
#ifdef ANSI_DEFAULT
    "ANSI default terminal",
#endif
#ifdef TEXTCOLOR
    "color",
#endif
#ifdef TTY_GRAPHICS
#ifdef TTY_TILES_ESCCODES
    "console escape codes for tile hinting",
#endif
#endif
#ifdef COM_COMPL
    "command line completion",
#endif
#ifdef LIFE
    "Conway's Game of Life",
#endif
#ifdef COMPRESS
    "data file compression",
#endif
#ifdef ZLIB_COMP
    "ZLIB data file compression",
#endif
#ifdef DLB
#ifndef VERSION_IN_DLB_FILENAME
    "data librarian",
#else
    "data librarian with a version-dependent name",
#endif
#endif
#ifdef DUMPLOG
    "end-of-game dumplogs",
#endif
#ifdef HOLD_LOCKFILE_OPEN
    "exclusive lock on level 0 file",
#endif
#if defined(MSGHANDLER) && (defined(POSIX_TYPES) || defined(__GNUC__))
    "external program as a message handler",
#endif
#ifdef MFLOPPY
    "floppy drive support",
#endif
#ifdef INSURANCE
    "insurance files for recovering from crashes",
#endif
#ifdef LOGFILE
    "log file",
#endif
#ifdef XLOGFILE
    "extended log file",
#endif
#ifdef PANICLOG
    "errors and warnings log file",
#endif
#ifdef MAIL
    "mail daemon",
#endif
#if defined(GNUDOS) || defined(__DJGPP__)
    "MSDOS protected mode",
#endif
#ifdef NEWS
    "news file",
#endif
#ifdef OVERLAY
#ifdef MOVERLAY
    "MOVE overlays",
#else
#ifdef VROOMM
    "VROOMM overlays",
#else
    "overlays",
#endif
#endif
#endif
    /* pattern matching method will be substituted by nethack at run time */
    "pattern matching via :PATMATCH:",
#ifdef USE_ISAAC64
    "pseudo random numbers generated by ISAAC64",
#ifdef DEV_RANDOM
#ifdef NHSTDC
    /* include which specific one */
    "strong PRNG seed available from " DEV_RANDOM,
#else
    "strong PRNG seed available from DEV_RANDOM",
#endif
#else
#ifdef WIN32
    "strong PRNG seed available from CNG BCryptGenRandom()",
#endif
#endif  /* DEV_RANDOM */    
#else   /* ISAAC64 */
#ifdef RANDOM
    "pseudo random numbers generated by random()",
#else
    "pseudo random numbers generated by C rand()",
#endif
#endif /* ISAAC64 */
#ifdef SELECTSAVED
    "restore saved games via menu",
#endif
#ifdef SCORE_ON_BOTL
    "score on status line",
#endif
#ifdef CLIPPING
    "screen clipping",
#endif
#ifdef NO_TERMS
#ifdef MAC
    "screen control via mactty",
#endif
#ifdef SCREEN_BIOS
    "screen control via BIOS",
#endif
#ifdef SCREEN_DJGPPFAST
    "screen control via DJGPP fast",
#endif
#ifdef SCREEN_VGA
    "screen control via VGA graphics",
#endif
#ifdef WIN32CON
    "screen control via WIN32 console I/O",
#endif
#endif /* NO_TERMS */
#ifdef SHELL
    "shell command",
#endif
    "traditional status display",
#ifdef STATUS_HILITES
    "status via windowport with highlighting",
#else
    "status via windowport without highlighting",
#endif
#ifdef SUSPEND
    "suspend command",
#endif
#ifdef TTY_GRAPHICS
#ifdef TERMINFO
    "terminal info library",
#else
#if defined(TERMLIB) || (!defined(MICRO) && !defined(WIN32))
    "terminal capability library",
#endif
#endif
#endif /*TTY_GRAPHICS*/
#ifdef USE_XPM
    "tiles file in XPM format",
#endif
#ifdef GRAPHIC_TOMBSTONE
    "graphical RIP screen",
#endif
#ifdef TIMED_DELAY
    "timed wait for display effects",
#endif
#ifdef USER_SOUNDS
    "user sounds",
#endif
#ifdef PREFIXES_IN_USE
    "variable playground",
#endif
#ifdef VISION_TABLES
    "vision tables",
#endif
#ifdef ZEROCOMP
    "zero-compressed save files",
#endif
#ifdef RLECOMP
    "run-length compression of map in save files",
#endif
#ifdef SYSCF
    "system configuration at run-time",
#endif
    save_bones_compat_buf,
    "and basic NetHack features"
};

int
count_and_validate_winopts(VOID_ARGS)
{
    int i, cnt = 0;

    /* window_opts has a fencepost entry at the end */
    for (i = 0; i < SIZE(window_opts) - 1; i++) {
#if !defined(MAKEDEFS_C) && defined(FOR_RUNTIME)
#ifdef WIN32
        window_opts[i].valid = FALSE;
        if ((GUILaunched
             && case_insensitive_comp(window_opts[i].id, "mswin") != 0)
            || (!GUILaunched
                && case_insensitive_comp(window_opts[i].id, "mswin") == 0))
            continue;
#endif
#endif /* !MAKEDEFS_C && FOR_RUNTIME */
        ++cnt;
        window_opts[i].valid = TRUE;
    }
    return cnt;
}

static void
opt_out_words(str, length_p)
char *str; /* input, but modified during processing */
int *length_p; /* in/out */
{
    char *word;

    while (*str) {
        word = index(str, ' ');
#if 0
        /* treat " (" as unbreakable space */
        if (word && *(word + 1) == '(')
            word = index(word + 1,  ' ');
#endif
        if (word)
            *word = '\0';
        if (*length_p + (int) strlen(str) > COLNO - 5) {
            opttext[idxopttext] = strdup(optbuf);
            if (idxopttext < (MAXOPT - 1))
                idxopttext++;
            Sprintf(optbuf, "%s", opt_indent),
                    *length_p = (int) strlen(opt_indent);
        } else {
            Sprintf(eos(optbuf), " "), (*length_p)++;
        }
        Sprintf(eos(optbuf),
                "%s", str), *length_p += (int) strlen(str);
        str += strlen(str) + (word ? 1 : 0);
    }
}

void
build_options()
{
    char buf[BUFSZ];
    int i, length, winsyscnt, cnt = 0;
    const char *defwinsys = DEFAULT_WINDOW_SYS;

#if !defined (MAKEDEFS_C) && defined(FOR_RUNTIME)
#ifdef WIN32
    defwinsys = GUILaunched ? "mswin" : "tty";
#endif
#endif
    build_savebones_compat_string();
    opttext[idxopttext] = strdup(optbuf);
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
#if (NH_DEVEL_STATUS == NH_STATUS_BETA)
#define STATUS_ARG " [beta]"
#else
#define STATUS_ARG " [work-in-progress]"
#endif
#else
#define STATUS_ARG ""
#endif /* NH_DEVEL_STATUS == NH_STATUS_RELEASED */
    Sprintf(optbuf, "%sNetHack version %d.%d.%d%s\n",
            opt_indent, VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL, STATUS_ARG);
    opttext[idxopttext] = strdup(optbuf);
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    Sprintf(optbuf, "Options compiled into this edition:");
    opttext[idxopttext] = strdup(optbuf);
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    optbuf[0] = '\0';
    length = COLNO + 1; /* force 1st item onto new line */
    for (i = 0; i < SIZE(build_opts); i++) {
#if !defined(MAKEDEFS_C) && defined(FOR_RUNTIME)
#ifdef WIN32
        /* ignore the console entry if GUI version */
        if (GUILaunched
            && !strcmp("screen control via WIN32 console I/O", build_opts[i]))
            continue;
#endif
#endif /* !MAKEDEFS_C && FOR_RUNTIME */
        opt_out_words(strcat(strcpy(buf, build_opts[i]),
                             (i < SIZE(build_opts) - 1) ? "," : "."),
                      &length);
    }
    opttext[idxopttext] = strdup(optbuf);
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    optbuf[0] = '\0';
    winsyscnt = count_and_validate_winopts();
    opttext[idxopttext] = strdup(optbuf);
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    Sprintf(optbuf, "Supported windowing system%s:",
            (winsyscnt > 1) ? "s" : "");
    opttext[idxopttext] = strdup(optbuf);
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    optbuf[0] = '\0';
    length = COLNO + 1; /* force 1st item onto new line */

    for (i = 0; i < SIZE(window_opts) - 1; i++) {
        if (!window_opts[i].valid)
            continue;
        Sprintf(buf, "\"%s\"", window_opts[i].id);
        if (strcmp(window_opts[i].name, window_opts[i].id))
            Sprintf(eos(buf), " (%s)", window_opts[i].name);
        /*
         * 1 : foo.
         * 2 : foo and bar  (note no period; comes from 'with default' below)
         * 3+: for, bar, and quux
         */
        if (cnt != (winsyscnt - 1)) {
            Strcat(buf, (winsyscnt == 1) ? "." /* no 'default' */
                          : (winsyscnt == 2 && cnt == 0) ? " and"
                            : (cnt == winsyscnt - 2) ? ", and"
                              : ",");
        }
        opt_out_words(buf, &length);
        cnt++;
    }
    if (cnt > 1) {
        Sprintf(buf, ", with a default of \"%s\".", defwinsys);
        opt_out_words(buf, &length);
    }
    (void) mdlib_strsubst(optbuf, " , ", ", ");
    opttext[idxopttext] = strdup(optbuf);
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    optbuf[0] = '\0';

#if defined(MAKEDEFS_C) || defined(FOR_RUNTIME)
    {
        static const char *lua_info[] = {
 "", "NetHack 3.7.* uses the 'Lua' interpreter to process some data:", "",
 "    :LUACOPYRIGHT:", "",
 /*        1         2         3         4         5         6         7
  1234567890123456789012345678901234567890123456789012345678901234567890123456
  */
 "    \"Permission is hereby granted, free of charge, to any person obtaining",
 "     a copy of this software and associated documentation files (the ",
 "     \"Software\"), to deal in the Software without restriction including",
 "     without limitation the rights to use, copy, modify, merge, publish,",
 "     distribute, sublicense, and/or sell copies of the Software, and to ",
 "     permit persons to whom the Software is furnished to do so, subject to",
 "     the following conditions:",
 "     The above copyright notice and this permission notice shall be",
 "     included in all copies or substantial portions of the Software.\"",
            (const char *) 0
        };

        /* add lua copyright notice;
           ":TAG:" substitutions are deferred to caller */
        for (i = 0; lua_info[i]; ++i) {
            opttext[idxopttext] = strdup(lua_info[i]);
            if (idxopttext < (MAXOPT - 1))
                idxopttext++;
        }
    }
#endif /* MAKEDEFS_C || FOR_RUNTIME */

    /* end with a blank line */
    opttext[idxopttext] = strdup("");
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    return;
}

#if defined(__DATE__) && defined(__TIME__)
#define extract_field(t,s,n,z)    \
    do {                          \
        for (i = 0; i < n; ++i)   \
            t[i] = s[i + z];      \
        t[i] = '\0';              \
    } while (0)
#endif

void
runtime_info_init()
{
    int i;
    char tmpbuf[BUFSZ], *strp;
    const char *mth[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    struct tm t = {0};
    time_t timeresult;

    if (!done_runtime_opt_init_once) {
        done_runtime_opt_init_once = 1;
        build_savebones_compat_string();
        /* construct the current version number */
        make_version();
        /*
         * In a cross-compiled environment, you can't execute
         * the target binaries during the build, so we can't
         * use makedefs to write the values of the build
         * date and time to a file for retrieval. Not for
         * information meaningful to the target execution
         * environment.
         *
         * How can we capture the build date/time of the target
         * binaries in such a situation?  We need to rely on the
         * cross-compiler itself to do it for us during the
         * cross-compile.
         *
         * To that end, we are going to make use of the
         * following pre-defined preprocessor macros for this:
         *    gcc, msvc, clang   __DATE__  "Feb 12 1996"
         *    gcc, msvc, clang   __TIME__  "23:59:01"
         *
         */

#if defined(__DATE__) && defined(__TIME__)
        if (sizeof __DATE__ + sizeof __TIME__  + sizeof "123" <
            sizeof rttimebuf)
            Sprintf(rttimebuf, "%s %s", __DATE__, __TIME__);
        /* "Feb 12 1996 23:59:01"
            01234567890123456789  */
        if ((int) strlen(rttimebuf) == 20) {
            extract_field(tmpbuf, rttimebuf, 4, 7);   /* year */
            t.tm_year = atoi(tmpbuf) - 1900;
            extract_field(tmpbuf, rttimebuf, 3, 0);   /* mon */
            for (i = 0; i < SIZE(mth); ++i)
                if (!case_insensitive_comp(tmpbuf, mth[i])) {
                    t.tm_mon = i;
                    break;
                }
            extract_field(tmpbuf, rttimebuf, 2, 4);   /* mday */
            strp = tmpbuf;
            if (*strp == ' ')
                strp++;
            t.tm_mday = atoi(strp);
            extract_field(tmpbuf, rttimebuf, 2, 12);  /* hour */
            t.tm_hour = atoi(tmpbuf);
            extract_field(tmpbuf, rttimebuf, 2, 15);  /* min  */
            t.tm_min = atoi(tmpbuf);
            extract_field(tmpbuf, rttimebuf, 2, 18);  /* sec  */
            t.tm_sec = atoi(tmpbuf);
            timeresult = mktime(&t);
#if !defined(MAKEDEFS_C) && defined(CROSSCOMPILE_TARGET)
            BUILD_TIME = (unsigned long) timeresult;
            BUILD_DATE = rttimebuf;
#endif
#else  /* __DATE__ && __TIME__ */
            nhUse(strp);
#endif /* __DATE__ && __TIME__ */

#if !defined(MAKEDEFS_C) && defined(CROSSCOMPILE_TARGET)
            VERSION_NUMBER = version.incarnation;
            VERSION_FEATURES = version.feature_set;
#ifdef MD_IGNORED_FEATURES
            IGNORED_FEATURES = MD_IGNORED_FEATURES;
#endif
            VERSION_SANITY1 = version.entity_count;
            VERSION_SANITY2 = version.struct_sizes1;
            VERSION_SANITY3 = version.struct_sizes2;

            VERSION_STRING = strdup(version_string(tmpbuf, "."));
            VERSION_ID = strdup(version_id_string(tmpbuf, BUILD_DATE));
            COPYRIGHT_BANNER_C = strdup(bannerc_string(tmpbuf, BUILD_DATE));
#ifdef NETHACK_HOST_GIT_SHA
            NETHACK_GIT_SHA = strdup(NETHACK_HOST_GIT_SHA);
#endif
#ifdef NETHACK_HOST_GIT_BRANCH
            NETHACK_GIT_BRANCH = strdup(NETHACK_HOST_GIT_BRANCH);
#endif
#endif /* !MAKEDEFS_C  && CROSSCOMPILE_TARGET */
	}
        idxopttext = 0;
        build_options();
    }
}

const char *
do_runtime_info(rtcontext)
int *rtcontext;
{
    const char *retval = (const char *) 0;

    if (!done_runtime_opt_init_once)
        runtime_info_init();
    if (idxopttext && rtcontext)
        if (*rtcontext >= 0 && *rtcontext < (MAXOPT - 1)) {
            retval = opttext[*rtcontext];
            *rtcontext += 1;
	}
    return retval;
}

/*mdlib.c*/

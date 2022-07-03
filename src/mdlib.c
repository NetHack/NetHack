/* NetHack 3.7  mdlib.c  $NHDT-Date: 1655402414 2022/06/16 18:00:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.31 $ */
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
#include "permonst.h"
#include "objclass.h"
#include "wintype.h"
#include "sym.h"
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
#define FOR_RUNTIME

#if defined(MAKEDEFS_C) || defined(FOR_RUNTIME)
#include <stdarg.h>
/* REPRODUCIBLE_BUILD will change this to TRUE */
static boolean date_via_env = FALSE;

extern unsigned long md_ignored_features(void);
char *mdlib_version_string(char *, const char *);
char *version_id_string(char *, int, const char *);
char *bannerc_string(char *, int, const char *);
int case_insensitive_comp(const char *, const char *);

static void make_version(void);
static char *eos(char *);
#if 0
static char *mdlib_strsubst(char *, const char *, const char *);
#endif

#ifndef HAS_NO_MKSTEMP
#ifdef _MSC_VER
static int mkstemp(char *);
#endif
#endif

#endif /* MAKEDEFS_C || FOR_RUNTIME */

#if !defined(MAKEDEFS_C) && defined(WIN32)
extern int GUILaunched;
#endif

/* these are in extern.h but we don't include hack.h */
void runtime_info_init(void);
const char *do_runtime_info(int *);
void release_runtime_info(void);
void populate_nomakedefs(struct version_info *);
extern void free_nomakedefs(void); /* date.c */

void build_options(void);
static int count_and_validate_winopts(void);
static void opt_out_words(char *, int *);
static void build_savebones_compat_string(void);

static int idxopttext, done_runtime_opt_init_once = 0;
#define MAXOPT 40
static char *opttext[120] = { 0 };
char optbuf[COLBUFSZ];
static struct version_info version;
static const char opt_indent[] = "    ";

struct win_information {
    const char *id, /* DEFAULT_WINDOW_SYS string */
        *name;      /* description, often same as id */
    boolean valid;
};

static struct win_information window_opts[] = {
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
#ifdef MSWIN_GRAPHICS /* win32 */
    { "mswin", "Windows GUI", TRUE },
#endif
#ifdef SHIM_GRAPHICS
    { "shim", "NetHack Library Windowing Shim", TRUE },
#endif

#if 0  /* remainder have been retired */
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
#ifdef BEOS_GRAPHICS /* unmaintained/defunct */
    { "BeOS", "BeOS InterfaceKit", TRUE },
#endif
#endif  /* 0 => retired */
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
unsigned long
md_ignored_features(void)
{
    return (0UL
            | (1UL << 19) /* SCORE_ON_BOTL */
            | (1UL << 27) /* ZEROCOMP */
            | (1UL << 28) /* RLECOMP */
            );
}

static void
make_version(void)
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
    version.struct_sizes2 = (((unsigned long) sizeof(struct flag) << 10));
/* free bits in here */
    return;
}

#if defined(MAKEDEFS_C) || defined(FOR_RUNTIME)

char *
mdlib_version_string(char *outbuf, const char *delim)
{
    Sprintf(outbuf, "%d%s%d%s%d", VERSION_MAJOR, delim, VERSION_MINOR, delim,
            PATCHLEVEL);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
    Sprintf(eos(outbuf), "-%d", EDITLEVEL);
#endif
    return outbuf;
}

#define Snprintf(str, size, ...) \
    nh_snprintf(__func__, __LINE__, str, size, __VA_ARGS__)
extern void nh_snprintf(const char *func, int line, char *str, size_t size,
                        const char *fmt, ...);

#ifdef MAKEDEFS_C
DISABLE_WARNING_FORMAT_NONLITERAL

void
nh_snprintf(const char *func UNUSED, int line UNUSED, char *str, size_t size,
            const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
#ifdef NO_VSNPRINTF
    n = vsprintf(str, fmt, ap);
#else
    n = vsnprintf(str, size, fmt, ap);
#endif
    va_end(ap);

    if (n < 0 || (size_t)n >= size) { /* is there a problem? */
        str[size-1] = 0; /* make sure it is nul terminated */
    }

}

RESTORE_WARNING_FORMAT_NONLITERAL
#endif  /* MAKEDEFS_C */

char *
version_id_string(char *outbuf, int bufsz, const char *build_date)
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

    Snprintf(outbuf, bufsz, "%s NetHack%s Version %s%s - last %s %s.", PORT_ID,
            subbuf, mdlib_version_string(versbuf, "."), statusbuf,
            date_via_env ? "revision" : "build", build_date);
    return outbuf;
}

/* still within #if MAKDEFS_C || FOR_RUNTIME */

char *
bannerc_string(char *outbuf, int bufsz, const char *build_date)
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

    Snprintf(outbuf, bufsz, "         Version %s %s%s, %s %s.",
            mdlib_version_string(versbuf, "."), PORT_ID, subbuf,
            date_via_env ? "revised" : "built", build_date);
    return outbuf;
}

#ifndef HAS_NO_MKSTEMP
#ifdef _MSC_VER
int
mkstemp(char *template)
{
    int err;

    err = _mktemp_s(template, strlen(template) + 1);
    if( err != 0 )
        return -1;
    return _open(template,
                 _O_RDWR | _O_BINARY | _O_TEMPORARY | _O_CREAT,
                 _S_IREAD | _S_IWRITE);
}
#endif /* _MSC_VER */
#endif /* HAS_NO_MKSTEMP */
#endif /* MAKEDEFS_C || FOR_RUNTIME */

static char *
eos(char *str)
{
    while (*str)
        str++;
    return str;
}

#if 0
static char *
mdlib_strsubst(char *bp, const char *orig, const char *replacement)
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
#endif

static char save_bones_compat_buf[BUFSZ];

static void
build_savebones_compat_string(void)
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

static const char *const build_opts[] = {
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
#ifdef EDIT_GETLIN
    "edit getlin - some prompts remember previous response",
#endif
#ifdef DUMPLOG
    "end-of-game dumplogs",
#endif
#ifdef HOLD_LOCKFILE_OPEN
    "exclusive lock on level 0 file",
#endif
#ifdef MSGHANDLER
    "external program as a message handler",
#endif
#if defined(HANGUPHANDLING) && !defined(NO_SIGNAL)
#ifdef SAFERHANGUP
    "deferred handling of hangup signal",
#else
    "immediate handling of hangup signal",
#endif
#endif
#ifdef INSURANCE
    "insurance files for recovering from crashes",
#endif
#ifdef LIVELOG
    "live logging support",
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
#ifdef MONITOR_HEAP
    "monitor heap - record memory usage for later analysis",
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
#ifdef UNIX
#if defined(DEF_PAGER) && !defined(DLB)
    "external pager used for viewing help files",
#else
    "internal pager used for viewing help files",
#endif
#endif /* UNIX */
    /* pattern matching method will be substituted by nethack at run time */
    "pattern matching via :PATMATCH:",
#ifdef USE_ISAAC64
    "pseudo random numbers generated by ISAAC64",
#ifdef DEV_RANDOM
    /* include which specific one */
    "strong PRNG seed from " DEV_RANDOM,
#else
#ifdef WIN32
    "strong PRNG seed from CNG BCryptGenRandom()",
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

static int
count_and_validate_winopts(void)
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
opt_out_words(
    char *str,     /* input, but modified during processing */
    int *length_p) /* in/out */
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
            opttext[idxopttext] = dupstr(optbuf);
            if (idxopttext < (MAXOPT - 1))
                idxopttext++;
            Sprintf(optbuf, "%s", opt_indent),
                *length_p = (int) strlen(opt_indent);
        } else {
            Sprintf(eos(optbuf), " "), (*length_p)++;
        }
        Sprintf(eos(optbuf), "%s", str), *length_p += (int) strlen(str);
        str += strlen(str) + (word ? 1 : 0);
    }
}

void
build_options(void)
{
    char buf[COLBUFSZ];
    int i, length, winsyscnt, cnt = 0;
    const char *defwinsys = DEFAULT_WINDOW_SYS;

#if !defined (MAKEDEFS_C) && defined(FOR_RUNTIME)
#ifdef WIN32
    defwinsys = GUILaunched ? "mswin" : "tty";
#endif
#endif
    build_savebones_compat_string();
    opttext[idxopttext] = dupstr(optbuf);
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
    opttext[idxopttext] = dupstr(optbuf);
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    Sprintf(optbuf, "Options compiled into this edition:");
    opttext[idxopttext] = dupstr(optbuf);
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
        Strcat(strcpy(buf, build_opts[i]),
               (i < SIZE(build_opts) - 1) ? "," : ".");
        opt_out_words(buf, &length);
    }
    opttext[idxopttext] = dupstr(optbuf);
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    optbuf[0] = '\0';
    winsyscnt = count_and_validate_winopts();
    opttext[idxopttext] = dupstr(optbuf);
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    Sprintf(optbuf, "Supported windowing system%s:",
            (winsyscnt > 1) ? "s" : "");
    opttext[idxopttext] = dupstr(optbuf);
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
         * 2 : foo and bar,
         * 3+: for, bar, and quux,
         *
         * 2+ will be followed by " with a default of..."
         */
        Strcat(buf, (winsyscnt == 1) ? "." /* no 'default' */
                    : (winsyscnt == 2 && cnt == 0) ? " and"
                      : (cnt == winsyscnt - 2) ? ", and"
                        : ",");
        opt_out_words(buf, &length);
        cnt++;
    }
    if (cnt > 1) {
        /* loop ended with a comma; opt_out_words() will insert a space */
        Sprintf(buf, "with a default of \"%s\".", defwinsys);
        opt_out_words(buf, &length);
    }

    opttext[idxopttext] = dupstr(optbuf);
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    optbuf[0] = '\0';

#if defined(MAKEDEFS_C) || defined(FOR_RUNTIME)
    {
        static const char *const lua_info[] = {
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
            opttext[idxopttext] = dupstr(lua_info[i]);
            if (idxopttext < (MAXOPT - 1))
                idxopttext++;
        }
    }
#endif /* MAKEDEFS_C || FOR_RUNTIME */

    /* end with a blank line */
    opttext[idxopttext] = dupstr("");
    if (idxopttext < (MAXOPT - 1))
        idxopttext++;
    return;
}

int
case_insensitive_comp(const char *s1, const char *s2)
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

void
runtime_info_init(void)
{
    if (!done_runtime_opt_init_once) {
        done_runtime_opt_init_once = 1;
        build_savebones_compat_string();
        /* construct the current version number */
        make_version();
        populate_nomakedefs(&version);          /* date.c */
        idxopttext = 0;
        build_options();
    }
}

const char *
do_runtime_info(int *rtcontext)
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

void
release_runtime_info(void)
{
    while (idxopttext > 0) {
        --idxopttext;
        free((genericptr_t) opttext[idxopttext]), opttext[idxopttext] = 0;
    }
    done_runtime_opt_init_once = 0;
    free_nomakedefs();
}

/*mdlib.c*/

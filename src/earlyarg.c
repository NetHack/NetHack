/* NetHack 5.0	earlyarg.c	$NHDT-Date: 1771213100 2026/02/15 19:38:20 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.286 $ */
/* Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"

staticfn void debug_fields(char *);
#ifndef NODUMPENUMS
staticfn void dump_enums(void);
#endif
ATTRNORETURN staticfn void opt_terminate(void) NORETURN;
ATTRNORETURN staticfn void opt_usage(const char *) NORETURN;
ATTRNORETURN staticfn void scores_only(int, char **, const char *) NORETURN;
staticfn char *lopt(char *, int, const char *, const char *, int *, char ***);
staticfn void consume_arg(int, int *, char ***);
staticfn void consume_two_args(int, int *, char ***);

#ifdef UNIX
extern boolean whoami(void);
#endif

/*
 * Argument processing helpers - for xxmain() to share
 * and call.
 *
 * These should return TRUE if the argument matched,
 * whether the processing of the argument was
 * successful or not.
 *
 * Most of these do their thing, then after returning
 * to xxmain(), the code exits without starting a game.
 *
 */

static const struct early_opt earlyopts[] = {
    { ARG_DEBUG, "debug", 5, TRUE },
    { ARG_VERSION, "version", 4, TRUE },
    { ARG_SHOWPATHS, "showpaths", 8, FALSE },
#ifndef NODUMPENUMS
    { ARG_DUMPENUMS, "dumpenums", 9, FALSE },
#endif
    { ARG_DUMPGLYPHIDS, "dumpglyphids", 12, FALSE },
    { ARG_DUMPMONGEN, "dumpmongen", 10, FALSE },
    { ARG_DUMPWEIGHTS, "dumpweights", 11, FALSE },
#ifdef WIN32
    { ARG_WINDOWS, "windows", 4, TRUE },
#endif
#if defined(CRASHREPORT)
    { ARG_BIDSHOW, "bidshow", 7, FALSE },
#endif
};

static char ArgVal_novalue[] = "[nothing]"; /* note: not 'const' */

enum cmdlinearg {
    ArgValRequired = 0,
    ArgValOptional = 1,
    ArgValDisallowed = 2,
    ArgVal_mask = (1 | 2),
    ArgNamOneLetter = 4,
    ArgNam_mask = 4,
    ArgErrSilent = 0,
    ArgErrComplain = 8,
    ArgErr_mask = 8
};

/* approximate 'getopt_long()' for one option; all the comments refer to
   "-windowtype" but the code isn't specific to that  */
staticfn char *
lopt(char *arg,  /* command line token; beginning matches 'optname' */
     int lflags, /* cmdlinearg | errorhandling */
     const char *optname, /* option's name; "-windowtype" in examples below */
     const char *origarg, /* 'arg' might have had a dash prefix removed */
     int *argc_p,         /* argc that can have changes passed to caller */
     char ***argv_p)      /* argv[] ditto */
{
    int argc = *argc_p;
    char **argv = *argv_p;
    char *p, *nextarg = (argc > 1 && argv[1][0] != '-') ? argv[1] : 0;
    int l, opttype = (lflags & ArgVal_mask);
    boolean oneletterok = ((lflags & ArgNam_mask) == ArgNamOneLetter),
            complain = ((lflags & ArgErr_mask) == ArgErrComplain);

    /* first letter must match */
    if (arg[1] != optname[1]) {
    loptbail:
        if (complain)
            config_error_add("Unknown option: %.60s", origarg);
        return (char *) 0;
    loptnotallowed:
        if (complain)
            config_error_add("Value not allowed: %.60s", origarg);
        return (char *) 0;
    loptrequired:
        if (complain)
            config_error_add("Missing required value: %.60s", origarg);
        return (char *) 0;
    }

    if ((p = strchr(arg, '=')) == 0)
        p = strchr(arg, ':');
    if (p && opttype == ArgValDisallowed)
        goto loptnotallowed;

    l = (int) (p ? (long) (p - arg) : (long) strlen(arg));
    if ((l > 2 || oneletterok) && !strncmp(arg, optname, l)) {
        /* "-windowtype[=foo]" */
        if (p)
            ++p; /* past '=' or ':' */
        else if (opttype == ArgValRequired)
            p = eos(arg); /* we have "-w[indowtype]" w/o "=foo"
                           * so we'll take foo from next element */
        else
            return ArgVal_novalue;
    } else if (oneletterok) {
        /* "-w..." but not "-w[indowtype[=foo]]" */
        if (!p) {
            p = &arg[2]; /* past 'w' of "-wfoo" */
#if 0 /* -x:value could work but is not supported (callers don't expect it) \
       */
        } else if (p == arg + 2) {
            ++p; /* past ':' of "-w:foo" */
#endif
        } else {
            /* "-w...=foo" but not "-w[indowtype]=foo" */
            goto loptbail;
        }
    } else {
        goto loptbail;
    }
    if (!p || !*p) {
        /* "-w[indowtype]" w/o '='/':' if there is a next element, use
           it for "foo"; if not, supply a non-Null bogus value */
        if (nextarg
            && (opttype == ArgValRequired || opttype == ArgValOptional))
            p = nextarg, --(*argc_p), ++(*argv_p);
        else if (opttype == ArgValRequired)
            goto loptrequired;
        else
            p = ArgVal_novalue; /* there is no next element */
    }
    return p;
}
/* move argv[ndx] to end of argv[] array, then reduce argc to hide it;
   prevents process_options() from encountering it after early_options()
   has processed it; elements get reordered but all remain intact */
staticfn void
consume_arg(int ndx, int *ac_p, char ***av_p)
{
    char *gone, **av = *av_p;
    int i, ac = *ac_p;

    /* "-one -two -three -four" -> "-two -three -four -one" */
    if (ac > 2) {
        gone = av[ndx];
        for (i = ndx + 1; i < ac; ++i)
            av[i - 1] = av[i];
        av[ac - 1] = gone;
    }
    --(*ac_p);
}

/* consume two tokens for '-argname value' w/o '=' or ':' */
staticfn void
consume_two_args(int ndx, int *ac_p, char ***av_p)
{
    /* when consuming "-two arg" from "-two arg -three -four",
       the *ac_p manipulation results in "-three -four -two arg"
       rather than the "-three -four arg -two" that would happen
       with just two ordinary consume_arg() calls */
    consume_arg(ndx, ac_p, av_p);
    ++(*ac_p); /* bring the final slot back into view */
    consume_arg(ndx, ac_p, av_p);
    --(*ac_p); /* take away restored slot */
}

/* process some command line arguments before loading options */
void
early_options(int *argc_p, char ***argv_p, char **hackdir_p)
{
    char **argv, *arg, *origarg;
    int argc, oldargc, ndx = 0, consumed = 0;

#ifdef ENHANCED_SYMBOLS
    if (argcheck(*argc_p, *argv_p, ARG_DUMPGLYPHIDS) == 2)
        opt_terminate();
#endif

    config_error_init(FALSE, "command line", FALSE);

    /* treat "nethack ?" as a request for usage info; due to shell
       processing, player likely has to use "nethack \?" or "nethack '?'"
       [won't work if used as "nethack -dpath ?" or "nethack -d path ?"] */
    if (*argc_p > 1 && !strcmp((*argv_p)[1], "?"))
        opt_usage(*hackdir_p); /* doesn't return */

    /*
     * Both *argc_p and *argv_p account for the program name as (*argv_p)[0];
     * local argc and argv implicitly discard that (by starting 'ndx' at 1).
     * argcheck() doesn't mind, prscore() (via scores_only()) does (for the
     * number of args it gets passed, not for the value of argv[0]).
     */
    for (ndx = 1; ndx < *argc_p; ndx += (consumed ? 0 : 1)) {
        consumed = 0;
        argc = *argc_p - ndx;
        argv = *argv_p + ndx;

        arg = origarg = argv[0];
        /* skip any args intended for deferred options */
        if (*arg != '-')
            continue;
        /* allow second dash if arg name is longer than one character */
        if (arg[0] == '-' && arg[1] == '-' && arg[2] != '\0'
            && (arg[3] != '\0' && arg[3] != '=' && arg[3] != ':'))
            ++arg;

        switch (arg[1]) { /* char after leading dash */
        case 'b':
#ifdef CRASHREPORT
            // --bidshow
            if (argcheck(argc, argv, ARG_BIDSHOW) == 2) {
                opt_terminate();
                /*NOTREACHED*/
            }
#endif
            break;
        case 'd':
            if (argcheck(argc, argv, ARG_DEBUG) == 1) {
                consume_arg(ndx, argc_p, argv_p), consumed = 1;
#ifndef NODUMPENUMS
            } else if (argcheck(argc, argv, ARG_DUMPENUMS) == 2) {
                opt_terminate();
                /*NOTREACHED*/
#endif
            } else if (argcheck(argc, argv, ARG_DUMPMONGEN) == 2) {
                opt_terminate();
                /*NOTREACHED*/
            } else if (argcheck(argc, argv, ARG_DUMPWEIGHTS) == 2) {
                opt_terminate();
                /*NOTREACHED*/
            } else {
#ifdef CHDIR
                oldargc = argc;
                arg = lopt(arg,
                           (ArgValRequired | ArgNamOneLetter | ArgErrSilent),
                           "-directory", origarg, &argc, &argv);
                if (!arg)
                    error("Flag -d must be followed by a directory name.");
                if (*arg != 'e') { /* avoid matching -decgraphics or -debug */
                    *hackdir_p = arg;
                    if (oldargc == argc)
                        consume_arg(ndx, argc_p, argv_p), consumed = 1;
                    else
                        consume_two_args(ndx, argc_p, argv_p), consumed = 2;
                }
#endif /* CHDIR */
            }
            break;
        case 'h':
        case '?':
            if (lopt(arg, ArgValDisallowed, "-help", origarg, &argc, &argv)
                || lopt(arg, ArgValDisallowed | ArgNamOneLetter, "-?",
                        origarg, &argc, &argv))
                opt_usage(*hackdir_p); /* doesn't return */
            break;
        case 'n':
            oldargc = argc;
            if (!strcmp(arg, "-no-nethackrc")) /* no abbreviation allowed */
                arg = nhStr("/dev/null");
            else
                arg = lopt(arg, (ArgValRequired | ArgErrComplain),
                           "-nethackrc", origarg, &argc, &argv);
            if (arg) {
                gc.cmdline_rcfile = dupstr(arg);
                if (oldargc == argc)
                    consume_arg(ndx, argc_p, argv_p), consumed = 1;
                else
                    consume_two_args(ndx, argc_p, argv_p), consumed = 2;
            }
            break;
        case 's':
            if (argcheck(argc, argv, ARG_SHOWPATHS) == 2) {
                gd.deferred_showpaths = TRUE;
                gd.deferred_showpaths_dir = *hackdir_p;
                config_error_done();
                return;
            }
            /* check for "-s" request to show scores */
            if (lopt(arg,
                     ((ArgValDisallowed | ArgErrComplain)
                      /* only accept one-letter if there is just one
                         dash; reject "--s" because prscore() via
                         scores_only() doesn't understand it */
                      | ((origarg[1] != '-') ? ArgNamOneLetter : 0)),
                     /* [ought to omit val-disallowed and accept
                        --scores=foo since -s foo and -sfoo are
                        allowed, but -s form can take more than one
                        space-separated argument and --scores=foo
                        isn't suited for that] */
                     "-scores", origarg, &argc, &argv)) {
                /* at this point, argv[0] contains "-scores" or a leading
                   substring of it; prscore() (via scores_only()) expects
                   that to be in argv[1] so we adjust the pointer to make
                   that be the case; if there are any non-early args waiting
                   to be passed along to process_options(), the resulting
                   argv[0] will be one of those rather than the program
                   name but prscore() doesn't care */
                scores_only(argc + 1, argv - 1, *hackdir_p);
                /*NOTREACHED*/
            }
            break;
        case 'u':
#if defined(UNIX)
            if (lopt(arg, ArgValDisallowed, "-usage", origarg, &argc, &argv))
                opt_usage(*hackdir_p);
#elif defined(WIN32) || defined(MSDOS) || defined(AMIGA)
            if (arg[2]) {
                (void) strncpy(svp.plname, arg + 2, sizeof(svp.plname) - 1);
            } else if (ndx + 1 < *argc_p) {
                const char *nextarg = (*argv_p)[ndx + 1];

                if (nextarg[0] != '-') {
                    (void) strncpy(svp.plname, nextarg, sizeof(svp.plname) - 1);
                } else {
                    raw_print("Player name expected after -u\n");
                }
            }
#endif
            break;
        case 'v':
            if (argcheck(argc, argv, ARG_VERSION) == 2) {
                opt_terminate();
                /*NOTREACHED*/
            }
            break;
        case 'w': /* windowtype: "-wfoo" or "-w[indowtype]=foo"
                   * or "-w[indowtype]:foo" or "-w[indowtype] foo" */
            arg =
                lopt(arg, (ArgValRequired | ArgNamOneLetter | ArgErrComplain),
                     "-windowtype", origarg, &argc, &argv);
            if (gc.cmdline_windowsys)
                free((genericptr_t) gc.cmdline_windowsys);
            gc.cmdline_windowsys = arg ? dupstr(arg) : NULL;
            break;
 #if !defined(UNIX) && !defined(VMS)
        case 'D':
            wizard = TRUE, discover = FALSE;
            break;
        case 'X':
            discover = TRUE, wizard = FALSE;
            break;
#endif
        default:
            break;
        }
    }
    /* empty or "N errors on command line" */
    config_error_done();
    return;
}
/* for command-line options that perform some immediate action and then
   terminate the program without starting play, like 'nethack --version'
   or 'nethack -s Zelda'; do some cleanup before that termination */
ATTRNORETURN staticfn void
opt_terminate(void)
{
    program_state.early_options = 0;
    config_error_done(); /* free memory allocated by config_error_init() */

    nh_terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}

ATTRNORETURN staticfn void
opt_usage(const char *hackdir)
{
#ifdef CHDIR
    chdirx(hackdir, TRUE);
#else
    nhUse(hackdir);
#endif
    dlb_init();

    genl_display_file(USAGEHELP, TRUE);
    opt_terminate();
}
/* show the sysconf file name, playground directory, run-time configuration
   file name, dumplog file name if applicable, and some other things */
ATTRNORETURN void
after_opt_showpaths(const char *dir)
{
#ifdef CHDIR
    chdirx(dir, FALSE);
#else
    nhUse(dir);
#endif
    opt_terminate();
    /*NOTREACHED*/
}

/* handle "-s <score options> [character-names]" to show all the entries
   in the high scores file ('record') belonging to particular characters;
   nethack will end after doing so without starting play */
ATTRNORETURN staticfn void
scores_only(int argc, char **argv, const char *dir)
{
    /* do this now rather than waiting for final termination, in case there
       is an error summary coming */
    config_error_done();

#ifdef CHDIR
    chdirx(dir, FALSE);
#else
    nhUse(dir);
#endif
#ifdef SYSCF
    iflags.initoptions_noterminate = TRUE;
    initoptions(); /* sysconf options affect whether panictrace is enabled */
    iflags.initoptions_noterminate = FALSE;
#endif
#ifdef PANICTRACE
    ARGV0 = gh.hname; /* save for possible stack trace */
#ifndef NO_SIGNAL
    panictrace_setsignals(TRUE);
#endif
#endif
#ifdef UNIX
    (void) whoami(); /* set up default plname[] */
#endif
    prscore(argc, argv);
#ifdef MSWIN_GRAPHICS
    /* NetHackW can also support WINDOWPORT(curses) now, so check */
    if (WINDOWPORT(mswin)) {
        wait_synch();
    }
#endif

    nh_terminate(EXIT_SUCCESS); /* bypass opt_terminate() */
    /*NOTREACHED*/
}

/*
 * Returns:
 *    0 = no match
 *    1 = found and skip past this argument
 *    2 = found and trigger immediate exit
 */
int
argcheck(int argc, char *argv[], enum earlyarg e_arg)
{
    int i, idx;
    boolean match = FALSE;
    char *userea = (char *) 0;
    const char *dashdash = "";

    for (idx = 0; idx < SIZE(earlyopts); idx++) {
        if (earlyopts[idx].e == e_arg){
            break;
        }
    }
    if (idx >= SIZE(earlyopts) || argc < 1)
        return 0;

    for (i = 0; i < argc; ++i) {
        if (argv[i][0] != '-')
            continue;
        if (argv[i][1] == '-') {
            userea = &argv[i][2];
            dashdash = "-";
        } else {
            userea = &argv[i][1];
        }
        match = match_optname(userea, earlyopts[idx].name,
                              earlyopts[idx].minlength,
                              earlyopts[idx].valallowed);
        if (match)
            break;
    }

    if (match) {
        const char *extended_opt = strchr(userea, ':');

        if (!extended_opt)
            extended_opt = strchr(userea, '=');
        switch(e_arg) {
        case ARG_DEBUG:
            if (extended_opt) {
                char *cpy_extended_opt;

                cpy_extended_opt = dupstr(extended_opt);
                debug_fields(cpy_extended_opt + 1);
                free((genericptr_t) cpy_extended_opt);
            }
            return 1;
        case ARG_VERSION: {
            boolean insert_into_pastebuf = FALSE;

            if (extended_opt) {
                extended_opt++;
                    /* Deprecated in favor of "copy" - remove no later
                       than  next major version */
                if (match_optname(extended_opt, "paste", 5, FALSE)) {
                    insert_into_pastebuf = TRUE;
                } else if (match_optname(extended_opt, "copy", 4, FALSE)) {
                    insert_into_pastebuf = TRUE;
                } else if (match_optname(extended_opt, "dump", 4, FALSE)) {
                    /* version number plus enabled features and sanity
                       values that the program compares against the same
                       thing recorded in save and bones files to check
                       whether they're being used compatibly */
                    dump_version_info();
                    return 2; /* done */
                } else if (!match_optname(extended_opt, "show", 4, FALSE)) {
                    raw_printf("-%sversion can only be extended with"
                               " -%sversion:copy or :dump or :show.\n",
                               dashdash, dashdash);
                    /* exit after we've reported bad command line argument */
                    return 2;
                }
            }
            early_version_info(insert_into_pastebuf);
            return 2;
        }
        case ARG_SHOWPATHS:
            return 2;
#ifndef NODUMPENUMS
        case ARG_DUMPENUMS:
            dump_enums();
            return 2;
#endif
        case ARG_DUMPGLYPHIDS:
            dump_glyphids();
            return 2;
        case ARG_DUMPMONGEN:
            dump_mongen();
            return 2;
        case ARG_DUMPWEIGHTS:
            dump_weights();
            return 2;
#ifdef CRASHREPORT
        case ARG_BIDSHOW:
            crashreport_bidshow();
            return 2;
#endif
#ifdef WIN32
        case ARG_WINDOWS:
            if (extended_opt) {
                extended_opt++;
                return windows_early_options(extended_opt);
            }
        FALLTHROUGH;
        /*FALLTHRU*/
#endif
        default:
            break;
        }
    };
    return 0;
}

/*
 * These are internal controls to aid developers with
 * testing and debugging particular aspects of the code.
 * They are not player options and the only place they
 * are documented is right here. No gameplay is altered.
 *
 * test             - test whether this parser is working
 * ttystatus        - TTY:
 * immediateflips   - WIN32: turn off display performance
 *                    optimization so that display output
 *                    can be debugged without buffering.
 * fuzzer           - enable fuzzer without debugger intervention.
 */
staticfn void
debug_fields(char *opts)
{
    char *op;
    boolean negated = FALSE;

    while ((op = strchr(opts, ',')) != 0) {
        *op++ = 0;
        /* recurse */
        debug_fields(op);
    }
    if (strlen(opts) > BUFSZ / 2)
        return;


    /* strip leading and trailing white space */
    while (isspace((uchar) *opts))
        opts++;
    op = eos((char *) opts);
    while (--op >= opts && isspace((uchar) *op))
        *op = '\0';

    if (!*opts) {
        /* empty */
        return;
    }
    while ((*opts == '!') || !strncmpi(opts, "no", 2)) {
        if (*opts == '!')
            opts++;
        else
            opts += 2;
        negated = !negated;
    }
    if (match_optname(opts, "test", 4, FALSE))
        iflags.debug.test = negated ? FALSE : TRUE;
#ifdef TTY_GRAPHICS
    if (match_optname(opts, "ttystatus", 9, FALSE))
        iflags.debug.ttystatus = negated ? FALSE : TRUE;
#endif
#ifdef WIN32
    if (match_optname(opts, "immediateflips", 14, FALSE))
        iflags.debug.immediateflips = negated ? FALSE : TRUE;
#endif
    if (match_optname(opts, "fuzzer", 4, FALSE))
        iflags.fuzzerpending = TRUE;
    return;
}

#if !defined(NODUMPENUMS)
/* monsdump[] and objdump[] are also used in utf8map.c */

#define DUMP_ENUMS
#define UNPREFIXED_COUNT (5)
struct enum_dump monsdump[] = {
#include "monsters.h"
    { NUMMONS, "NUMMONS" },
    { NON_PM, "NON_PM" },
    { LOW_PM, "LOW_PM" },
    { HIGH_PM, "HIGH_PM" },
    { SPECIAL_PM, "SPECIAL_PM" }
};
struct enum_dump objdump[] = {
#include "objects.h"
    { NUM_OBJECTS, "NUM_OBJECTS" },
};

#define DUMP_ENUMS_PCHAR
static struct enum_dump defsym_cmap_dump[] = {
#include "defsym.h"
    { MAXPCHARS, "MAXPCHARS" },
};
#undef DUMP_ENUMS_PCHAR

#define DUMP_ENUMS_MONSYMS
static struct enum_dump defsym_mon_syms_dump[] = {
#include "defsym.h"
    { MAXMCLASSES, "MAXMCLASSES" },
};
#undef DUMP_ENUMS_MONSYMS

#define DUMP_ENUMS_MONSYMS_DEFCHAR
static struct enum_dump defsym_mon_defchars_dump[] = {
#include "defsym.h"
};
#undef DUMP_ENUMS_MONSYMS_DEFCHAR

#define DUMP_ENUMS_OBJCLASS_DEFCHARS
static struct enum_dump objclass_defchars_dump[] = {
#include "defsym.h"
};
#undef DUMP_ENUMS_OBJCLASS_DEFCHARS

#define DUMP_ENUMS_OBJCLASS_CLASSES
static struct enum_dump objclass_classes_dump[] = {
#include "defsym.h"
    { MAXOCLASSES, "MAXOCLASSES" },
};
#undef DUMP_ENUMS_OBJCLASS_CLASSES

#define DUMP_ENUMS_OBJCLASS_SYMS
static struct enum_dump objclass_syms_dump[] = {
#include "defsym.h"
};
#undef DUMP_ENUMS_OBJCLASS_SYMS

#define DUMP_ARTI_ENUM
static struct enum_dump arti_enum_dump[] = {
#include "artilist.h"
    { AFTER_LAST_ARTIFACT, "AFTER_LAST_ARTIFACT" }
};
#undef DUMP_ARTI_ENUM

/* the enums are not part of hack.h for this one */
#define DUMP_MCASTU_ENUM1
enum mcast_dumpenum_spells {
    #include "mcastu.h"
};
#undef DUMP_MCASTU_ENUM1

#define DUMP_MCASTU_ENUM2
static struct enum_dump mcastu_enum_dump[] = {
#include "mcastu.h"
};
#undef DUMP_MCASTU_ENUM2

#undef DUMP_ENUMS


#ifndef NODUMPENUMS

staticfn void
dump_enums(void)
{
    enum enum_dumps {
        monsters_enum,
        objects_enum,
        objects_misc_enum,
        defsym_cmap_enum,
        defsym_mon_syms_enum,
        defsym_mon_defchars_enum,
        objclass_defchars_enum,
        objclass_classes_enum,
        objclass_syms_enum,
        arti_enum,
        mcastu_enum,
        NUM_ENUM_DUMPS
    };

#define dump_om(om) { om, #om }
    static const struct enum_dump omdump[] = {
        dump_om(LAST_GENERIC),
        dump_om(OBJCLASS_HACK),
        dump_om(FIRST_OBJECT),
        dump_om(FIRST_AMULET),
        dump_om(LAST_AMULET),
        dump_om(FIRST_SPELL),
        dump_om(LAST_SPELL),
        dump_om(MAXSPELL),
        dump_om(FIRST_REAL_GEM),
        dump_om(LAST_REAL_GEM),
        dump_om(FIRST_GLASS_GEM),
        dump_om(LAST_GLASS_GEM),
        dump_om(NUM_REAL_GEMS),
        dump_om(NUM_GLASS_GEMS),
        dump_om(MAX_GLYPH),
    };
#undef dump_om

    static const struct enum_dump *const ed[NUM_ENUM_DUMPS] = {
        monsdump, objdump, omdump,
        defsym_cmap_dump, defsym_mon_syms_dump,
        defsym_mon_defchars_dump,
        objclass_defchars_dump,
        objclass_classes_dump,
        objclass_syms_dump,
        arti_enum_dump,
        mcastu_enum_dump,
    };

    static const struct de_params {
        const char *const title;
        const char *const pfx;
        int unprefixed_count;
        int dumpflgs;  /* 0 = dump numerically only, 1 = add 'char' comment */
        int szd;
    } edmp[NUM_ENUM_DUMPS] = {
        { "monnums", "PM_", UNPREFIXED_COUNT, 0, SIZE(monsdump) },
        { "objects_nums", "", 1, 0, SIZE(objdump) },
        { "misc_object_nums", "", 1, 0, SIZE(omdump) },
        { "cmap_symbols", "", 1, 0, SIZE(defsym_cmap_dump) },
        { "mon_syms", "", 1, 0, SIZE(defsym_mon_syms_dump) },
        { "mon_defchars", "", 1, 1, SIZE(defsym_mon_defchars_dump) },
        { "objclass_defchars", "", 1, 1, SIZE(objclass_defchars_dump) },
        { "objclass_classes", "", 1, 0, SIZE(objclass_classes_dump) },
        { "objclass_syms", "", 1, 0, SIZE(objclass_syms_dump) },
        { "artifacts_nums", "", 1, 0, SIZE(arti_enum_dump) },
        { "mcast_spells", "MCAST_", 0, 0, SIZE(mcastu_enum_dump) },
    };

    const char *nmprefix;
    int i, j, nmwidth;
    char comment[BUFSZ];

    for (i = 0; i < NUM_ENUM_DUMPS; ++ i) {
        raw_printf("enum %s = {", edmp[i].title);
        for (j = 0; j < edmp[i].szd; ++j) {
            nmprefix = (j >= edmp[i].szd - edmp[i].unprefixed_count)
                           ? "" : edmp[i].pfx; /* "" or "PM_" */
            nmwidth = 27 - (int) strlen(nmprefix); /* 27 or 24 */
            if (edmp[i].dumpflgs > 0) {
                Snprintf(comment, sizeof comment,
                         "    /* '%c' */",
                         (ed[i][j].val >= 32 && ed[i][j].val <= 126)
                         ? ed[i][j].val : ' ');
            } else {
                comment[0] = '\0';
            }
            raw_printf("    %s%*s = %3d,%s",
                       nmprefix, -nmwidth,
                       ed[i][j].nm, ed[i][j].val,
                       comment);
       }
        raw_print("};");
        raw_print("");
    }
    raw_print("");
}
#undef UNPREFIXED_COUNT
#endif /* NODUMPENUMS */

void
dump_glyphids(void)
{
    dump_all_glyphids(stdout);
}
#endif /* !NODUMPENUMS */

/*allmain.c*/

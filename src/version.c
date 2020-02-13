/* NetHack 3.6	version.c	$NHDT-Date: 1575161965 2019/12/01 00:59:25 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.69 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"
#include "date.h"

/*
 * All the references to the contents of patchlevel.h have been moved
 * into makedefs....
 */
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif

#if defined(CROSSCOMPILE)
struct cross_target_s cross_target = {
    /* https://groups.google.com/forum/#!original/
       comp.sources.games/91SfKYg_xzI/dGnR3JnspFkJ */
    "Tue, 28-Jul-87 13:18:57 EDT",
    "Version 1.0, built Jul 28 13:18:57 1987.",
    "0000000000000000000000000000000000000000",
    "master",
    "1.0.0-0",
    "NetHack Version 1.0.0-0 - last build Tue Jul 28 13:18:57 1987.",
    0x01010000UL,
    0x00000000UL,
    0x00000000UL,
    0x00000000UL,
    0x00000000UL,
    0x00000000UL,
    554476737UL,
};
#endif /* CROSSCOMPILE */

#if defined(NETHACK_GIT_SHA)
const char *NetHack_git_sha
#if !defined(CROSSCOMPILE) || (defined(CROSSCOMPILE) && defined(CROSSCOMPILE_HOST))
                = NETHACK_GIT_SHA
#else
#ifdef NETHACK_HOST_GIT_SHA
                = NETHACK_HOST_GIT_SHA
#endif
#endif
;
#endif

#if defined(NETHACK_GIT_BRANCH)
const char *NetHack_git_branch
#if !defined(CROSSCOMPILE) || (defined(CROSSCOMPILE) && defined(CROSSCOMPILE_HOST))
                = NETHACK_GIT_BRANCH
#else
#ifdef NETHACK_HOST_GIT_BRANCH
                = NETHACK_HOST_GIT_BRANCH
#endif
#endif
;
#endif

static void FDECL(insert_rtoption, (char *));

/* fill buffer with short version (so caller can avoid including date.h) */
char *
version_string(buf)
char *buf;
{
    return strcpy(buf, VERSION_STRING);
}

/* fill and return the given buffer with the long nethack version string */
char *
getversionstring(buf)
char *buf;
{
    Strcpy(buf, VERSION_ID);

#if defined(RUNTIME_PORT_ID) \
    || defined(NETHACK_GIT_SHA) || defined(NETHACK_GIT_BRANCH)
    {
        int c = 0;
#if defined(RUNTIME_PORT_ID)
        char tmpbuf[BUFSZ], *tmp;
#endif
        char *p = eos(buf);
        boolean dotoff = (p > buf && p[-1] == '.');

        if (dotoff)
            --p;
        Strcpy(p, " (");
#if defined(RUNTIME_PORT_ID)
        tmp = get_port_id(tmpbuf);
        if (tmp)
            Sprintf(eos(buf), "%s%s", c++ ? "," : "", tmp);
#endif
#if defined(NETHACK_GIT_SHA)
        if (NetHack_git_sha)
            Sprintf(eos(buf), "%s%s", c++ ? "," : "", NetHack_git_sha);
#endif
#if defined(NETHACK_GIT_BRANCH)
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
        if (NetHack_git_branch)
            Sprintf(eos(buf), "%sbranch:%s",
                    c++ ? "," : "", NetHack_git_branch);
#endif
#endif
        if (c)
            Strcat(buf, ")");
        else /* if nothing has been added, strip " (" back off */
            *p = '\0';
        if (dotoff)
            Strcat(buf, ".");
    }
#endif /* RUNTIME_PORT_ID || NETHACK_GIT_SHA || NETHACK_GIT_BRANCH */

    return buf;
}

/* the 'v' command */
int
doversion()
{
    char buf[BUFSZ];

    pline("%s", getversionstring(buf));
    return 0;
}

/* the '#version' command; also a choice for '?' */
int
doextversion()
{
    int rtcontext = 0;
    const char *rtbuf;
    dlb *f = (dlb *) 0;
    char buf[BUFSZ], *p = 0;
    winid win = create_nhwindow(NHW_TEXT);
    boolean use_dlb = TRUE,
            done_rt = FALSE,
            done_dlb = FALSE,
            prolog;
#if 0   /* moved to util/mdlib.c and rendered via do_runtime_info() */
    const char *lua_info[] = {
 "About Lua: Copyright (c) 1994-2017 Lua.org, PUC-Rio.",
 /*        1         2         3         4         5         6         7
  1234567890123456789012345678901234567890123456789012345678901234567890123456789
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
#endif /*0*/
#if defined(OPTIONS_AT_RUNTIME) || defined(CROSSCOMPILE_TARGET)
    use_dlb = FALSE;
#else
    done_rt = TRUE;
#endif

    /* instead of using ``display_file(OPTIONS_USED,TRUE)'' we handle
       the file manually so we can include dynamic version info */

    (void) getversionstring(buf);
    /* if extra text (git info) is present, put it on separate line
       but don't wrap on (x86) */
    if (strlen(buf) >= COLNO)
        p = rindex(buf, '(');
    if (p && p > buf && p[-1] == ' ' && p[1] != 'x')
        p[-1] = '\0';
    else
        p = 0;
    putstr(win, 0, buf);
    if (p) {
        *--p = ' ';
        putstr(win, 0, p);
    }

    if (use_dlb) {
        f = dlb_fopen(OPTIONS_USED, "r");
        if (!f) {
            putstr(win, 0, "");
            Sprintf(buf, "[Configuration '%s' not available?]", OPTIONS_USED);
            putstr(win, 0, buf);
            done_dlb = TRUE;
        }
    }
    /*
     * already inserted above:
     * + outdented program name and version plus build date and time
     * dat/options; display contents with lines prefixed by '-' deleted:
     * - blank-line
     * -     indented program name and version
     *   blank-line
     *   outdented feature header
     * - blank-line
     *       indented feature list
     *       spread over multiple lines
     *   blank-line
     *   outdented windowing header
     * - blank-line
     *       indented windowing choices with
     *       optional second line for default
     * - blank-line
     * - EOF
     */

    prolog = TRUE; /* to skip indented program name */
    for (;;) {
        if (use_dlb && !done_dlb) {
            if (!dlb_fgets(buf, BUFSZ, f)) {
                done_dlb = TRUE;
                continue;
            }
        } else if (!done_rt) {
            if (!(rtbuf = do_runtime_info(&rtcontext))) {
                done_rt = TRUE;
                continue;
            }
            (void) strncpy(buf, rtbuf, BUFSZ - 1);
            buf[BUFSZ - 1] = '\0';
        } else {
            break;
        }
        (void) strip_newline(buf);
        if (index(buf, '\t') != 0)
            (void) tabexpand(buf);

        if (*buf && *buf != ' ') {
            /* found outdented header; insert a separator since we'll
               have skipped corresponding blank line inside the file */
            putstr(win, 0, "");
            prolog = FALSE;
        }
        /* skip blank lines and prolog (progame name plus version) */
        if (prolog || !*buf)
            continue;

        if (index(buf, ':'))
            insert_rtoption(buf);

        if (*buf)
            putstr(win, 0, buf);
    }
    if (use_dlb)
        (void) dlb_fclose(f);
    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);
    return 0;
}

void early_version_info(pastebuf)
boolean pastebuf;
{
    char buf[BUFSZ], buf2[BUFSZ];
    char *tmp = getversionstring(buf);

    /* this is early enough that we have to do
       our own line-splitting */
    if (tmp) {
        tmp = strstri(buf," (");
        if (tmp) *tmp++ = '\0';
    }

    Sprintf(buf2, "%s\n", buf);
    if (tmp) Sprintf(eos(buf2), "%s\n", tmp);
    raw_printf("%s", buf2);

    if (pastebuf) {
#ifdef RUNTIME_PASTEBUF_SUPPORT
        /*
         * Call a platform/port-specific routine to insert the
         * version information into a paste buffer. Useful for
         * easy inclusion in bug reports.
         */
        port_insert_pastebuf(buf2);
#else
        raw_printf("%s", "Paste buffer copy is not available.\n");
#endif
    }
}

extern const char regex_id[];

/*
 * makedefs should put the first token into dat/options; we'll substitute
 * the second value for it.  The token must contain at least one colon
 * so that we can spot it, and should not contain spaces so that makedefs
 * won't split it across lines.  Ideally the length should be close to
 * that of the substituted value since we don't do phrase-splitting/line-
 * wrapping when displaying it.
 */
static struct rt_opt {
    const char *token, *value;
} rt_opts[] = {
    { ":PATMATCH:", regex_id },
    { ":LUAVERSION:", (const char *) g.lua_ver },
    { ":LUACOPYRIGHT:", (const char *) g.lua_copyright },
};

/*
 * 3.6.0
 * Some optional stuff is no longer available to makedefs because
 * it depends which of several object files got linked into the
 * game image, so we insert those options here.
 */
static void
insert_rtoption(buf)
char *buf;
{
    int i;

    if (!g.lua_ver[0])
        get_lua_version();

    for (i = 0; i < SIZE(rt_opts); ++i) {
        if (strstri(buf, rt_opts[i].token) && *rt_opts[i].value) {
            (void) strsubst(buf, rt_opts[i].token, rt_opts[i].value);
	}
        /* we don't break out of the loop after a match; there might be
           other matches on the same line */
    }
}

#ifdef MICRO
boolean
comp_times(filetime)
long filetime;
{
    /* BUILD_TIME is constant but might have L suffix rather than UL;
       'filetime' is historically signed but ought to have been unsigned */
    return (boolean) ((unsigned long) filetime < (unsigned long) BUILD_TIME);
}
#endif

boolean
check_version(version_data, filename, complain, utdflags)
struct version_info *version_data;
const char *filename;
boolean complain;
unsigned long utdflags;
{
    if (
#ifdef VERSION_COMPATIBILITY
        version_data->incarnation < VERSION_COMPATIBILITY
        || version_data->incarnation > VERSION_NUMBER
#else
        version_data->incarnation != VERSION_NUMBER
#endif
        ) {
        if (complain)
            pline("Version mismatch for file \"%s\".", filename);
        return FALSE;
    } else if (
#ifndef IGNORED_FEATURES
        version_data->feature_set != VERSION_FEATURES
#else
        (version_data->feature_set & ~IGNORED_FEATURES)
            != (VERSION_FEATURES & ~IGNORED_FEATURES)
#endif
        || ((utdflags & UTD_SKIP_SANITY1) == 0
             && version_data->entity_count != VERSION_SANITY1)
        || ((utdflags & UTD_CHECKSIZES) &&
            (version_data->struct_sizes1 != VERSION_SANITY2))
        || ((utdflags & UTD_CHECKSIZES) &&
            (version_data->struct_sizes2 != VERSION_SANITY3))) {
        if (complain)
            pline("Configuration incompatibility for file \"%s\".", filename);
        return FALSE;
    }
    return TRUE;
}

/* this used to be based on file date and somewhat OS-dependant,
   but now examines the initial part of the file's contents */
boolean
uptodate(nhfp, name, utdflags)
NHFILE *nhfp;
const char *name;
unsigned long utdflags;
{
    int rlen = 0, cmc = 0, filecmc = 0;
    struct version_info vers_info;
    boolean verbose = name ? TRUE : FALSE;
    char indicator;

    if (nhfp->structlevel) {
        rlen = read(nhfp->fd, (genericptr_t) &indicator, sizeof indicator);
        rlen = read(nhfp->fd, (genericptr_t) &filecmc, sizeof filecmc);
        if (rlen == 0)
            return FALSE;
    }
    if (cmc != filecmc)
        return FALSE;

    rlen = read(nhfp->fd, (genericptr_t) &vers_info, sizeof vers_info);
    minit();                /* ZEROCOMP */
    if (rlen == 0) {
        if (verbose) {
            pline("File \"%s\" is empty?", name);
            wait_synch();
        }
        return FALSE;
    }

    if (!check_version(&vers_info, name, verbose, utdflags)) {
        if (verbose)
            wait_synch();
        return FALSE;
    }
    return TRUE;
}

void
store_formatindicator(nhfp)
NHFILE *nhfp;
{
    char indicate = 'u';
    int cmc = 0;

    if (nhfp->mode & WRITING) {
        if (nhfp->structlevel) {
            indicate = 'h';     /* historical */
            bwrite(nhfp->fd, (genericptr_t) &indicate, sizeof indicate);
            bwrite(nhfp->fd, (genericptr_t) &cmc, sizeof cmc);
        }
    }
}

void
store_version(nhfp)
NHFILE *nhfp;
{
#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_HOST)
    static const struct version_info version_data = {
        VERSION_NUMBER, VERSION_FEATURES,
        VERSION_SANITY1, VERSION_SANITY2, VERSION_SANITY3
#else
    struct version_info version_data = {
        0UL,0UL,0UL,0UL,0Ul
#endif
    };

#if defined(CROSSCOMPILE) && !defined(CROSSCOMPILE_HOST)
    version_data.incarnation = VERSION_NUMBER;    /* actual version number */
    version_data.feature_set = VERSION_FEATURES;  /* bitmask of config settings */
    version_data.entity_count  = VERSION_SANITY1; /* # of monsters and objects */
    version_data.struct_sizes1 = VERSION_SANITY2; /* size of key structs */
    version_data.struct_sizes2 = VERSION_SANITY3; /* size of more key structs */
#endif
    if (nhfp->structlevel) {
        bufoff(nhfp->fd);
        /* bwrite() before bufon() uses plain write() */
        store_formatindicator(nhfp);
        bwrite(nhfp->fd,(genericptr_t) &version_data,
               (unsigned) (sizeof version_data));
        bufon(nhfp->fd);
    }
    return;
}

#ifdef AMIGA
const char amiga_version_string[] = AMIGA_VERSION_STRING;
#endif

unsigned long
get_feature_notice_ver(str)
char *str;
{
    char buf[BUFSZ];
    int ver_maj, ver_min, patch;
    char *istr[3];
    int j = 0;

    if (!str)
        return 0L;
    str = strcpy(buf, str);
    istr[j] = str;
    while (*str) {
        if (*str == '.') {
            *str++ = '\0';
            j++;
            istr[j] = str;
            if (j == 2)
                break;
        } else if (index("0123456789", *str) != 0) {
            str++;
        } else
            return 0L;
    }
    if (j != 2)
        return 0L;
    ver_maj = atoi(istr[0]);
    ver_min = atoi(istr[1]);
    patch = atoi(istr[2]);
    return FEATURE_NOTICE_VER(ver_maj, ver_min, patch);
    /* macro from hack.h */
}

unsigned long
get_current_feature_ver()
{
    return FEATURE_NOTICE_VER(VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL);
}

/*ARGUSED*/
const char *
copyright_banner_line(indx)
int indx;
{
#ifdef COPYRIGHT_BANNER_A
    if (indx == 1)
        return COPYRIGHT_BANNER_A;
#endif
#ifdef COPYRIGHT_BANNER_B
    if (indx == 2)
        return COPYRIGHT_BANNER_B;
#endif
#ifdef COPYRIGHT_BANNER_C
    if (indx == 3)
        return COPYRIGHT_BANNER_C;
#endif
#ifdef COPYRIGHT_BANNER_D
    if (indx == 4)
        return COPYRIGHT_BANNER_D;
#endif
    return "";
}

/*version.c*/

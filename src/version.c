/* NetHack 3.6	version.c	$NHDT-Date: 1524693365 2018/04/25 21:56:05 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.49 $ */
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

#if defined(NETHACK_GIT_SHA)
const char * NetHack_git_sha = NETHACK_GIT_SHA;
#endif
#if defined(NETHACK_GIT_BRANCH)
const char * NetHack_git_branch = NETHACK_GIT_BRANCH;
#endif

STATIC_DCL void FDECL(insert_rtoption, (char *));

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
    boolean details = FALSE;

    Strcpy(buf, VERSION_ID);
#if defined(RUNTIME_PORT_ID) || \
    defined(NETHACK_GIT_SHA) || defined(NETHACK_GIT_BRANCH)
    details = TRUE;
#endif

    if (details) {
#if defined(RUNTIME_PORT_ID) || defined(NETHACK_GIT_SHA) || defined(NETHACK_GIT_BRANCH)
        int c = 0;
#endif
#if defined(RUNTIME_PORT_ID)
        char tmpbuf[BUFSZ];
        char *tmp = (char *)0;
#endif

        Sprintf(eos(buf), " (");
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
#if defined(BETA)
        if (NetHack_git_branch)
            Sprintf(eos(buf), "%sbranch:%s", c++ ? "," : "", NetHack_git_branch);
#endif
#endif
        Sprintf(eos(buf), ")");
    }
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
    dlb *f;
    char buf[BUFSZ];
    winid win = create_nhwindow(NHW_TEXT);

    /* instead of using ``display_file(OPTIONS_USED,TRUE)'' we handle
       the file manually so we can include dynamic version info */
    putstr(win, 0, getversionstring(buf));

    f = dlb_fopen(OPTIONS_USED, "r");
    if (!f) {
        putstr(win, 0, "");
        Sprintf(buf, "[Configuration '%s' not available?]", OPTIONS_USED);
        putstr(win, 0, buf);
    } else {
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
        boolean prolog = TRUE; /* to skip indented program name */

        while (dlb_fgets(buf, BUFSZ, f)) {
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
        (void) dlb_fclose(f);
        display_nhwindow(win, FALSE);
        destroy_nhwindow(win);
    }
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
};

/*
 * 3.6.0
 * Some optional stuff is no longer available to makedefs because
 * it depends which of several object files got linked into the
 * game image, so we insert those options here.
 */
STATIC_OVL void
insert_rtoption(buf)
char *buf;
{
    int i;

    for (i = 0; i < SIZE(rt_opts); ++i) {
        if (strstri(buf, rt_opts[i].token))
            (void) strsubst(buf, rt_opts[i].token, rt_opts[i].value);
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
check_version(version_data, filename, complain)
struct version_info *version_data;
const char *filename;
boolean complain;
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
        || version_data->entity_count != VERSION_SANITY1
        || version_data->struct_sizes1 != VERSION_SANITY2
        || version_data->struct_sizes2 != VERSION_SANITY3) {
        if (complain)
            pline("Configuration incompatibility for file \"%s\".", filename);
        return FALSE;
    }
    return TRUE;
}

/* this used to be based on file date and somewhat OS-dependant,
   but now examines the initial part of the file's contents */
boolean
uptodate(fd, name)
int fd;
const char *name;
{
    int rlen;
    struct version_info vers_info;
    boolean verbose = name ? TRUE : FALSE;

    rlen = read(fd, (genericptr_t) &vers_info, sizeof vers_info);
    minit(); /* ZEROCOMP */
    if (rlen == 0) {
        if (verbose) {
            pline("File \"%s\" is empty?", name);
            wait_synch();
        }
        return FALSE;
    }
    if (!check_version(&vers_info, name, verbose)) {
        if (verbose)
            wait_synch();
        return FALSE;
    }
    return TRUE;
}

void
store_version(fd)
int fd;
{
    static const struct version_info version_data = {
        VERSION_NUMBER, VERSION_FEATURES,
        VERSION_SANITY1, VERSION_SANITY2, VERSION_SANITY3
    };

    bufoff(fd);
    /* bwrite() before bufon() uses plain write() */
    bwrite(fd, (genericptr_t) &version_data,
           (unsigned) (sizeof version_data));
    bufon(fd);
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

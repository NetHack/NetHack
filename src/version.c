/* NetHack 3.6	version.c	$NHDT-Date: 1431192762 2015/05/09 17:32:42 $  $NHDT-Branch: master $:$NHDT-Revision: 1.29 $ */
/* NetHack 3.6	version.c	$Date: 2012/01/04 18:52:36 $  $Revision: 1.26 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
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

/* #define BETA_INFO "" */

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
#if defined(BETA) && defined(BETA_INFO)
    Sprintf(eos(buf), " %s", BETA_INFO);
#endif
#if defined(RUNTIME_PORT_ID)
    append_port_id(buf);
#endif
    return buf;
}

/* the 'v' command */
int
doversion()
{
    char buf[BUFSZ];

    pline1(getversionstring(buf));
    return 0;
}

/* the '#version' command; also a choice for '?' */
int
doextversion()
{
    dlb *f;
    char *cr, buf[BUFSZ];
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
         * dat/options; display the contents with lines prefixed by '-'
         * deleted:
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
            if ((cr = index(buf, '\n')) != 0)
                *cr = 0;
            if ((cr = index(buf, '\r')) != 0)
                *cr = 0;
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

            putstr(win, 0, buf);
        }
        (void) dlb_fclose(f);
        display_nhwindow(win, FALSE);
        destroy_nhwindow(win);
    }
    return 0;
}

#ifdef MICRO
boolean
comp_times(filetime)
long filetime;
{
    return ((boolean)(filetime < BUILD_TIME));
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
        version_data->feature_set != VERSION_FEATURES ||
#else
        (version_data->feature_set & ~IGNORED_FEATURES)
            != (VERSION_FEATURES & ~IGNORED_FEATURES)
        ||
#endif
        version_data->entity_count != VERSION_SANITY1
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
        VERSION_NUMBER, VERSION_FEATURES, VERSION_SANITY1, VERSION_SANITY2,
        VERSION_SANITY3
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

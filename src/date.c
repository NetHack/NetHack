/* NetHack 3.7  date.c  $NHDT-Date: 1644524054 2022/02/10 20:14:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $ */
/* Copyright (c) Michael Allison, 2021.                           */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"

/* these are in extern.h but we don't include hack.h */
void populate_nomakedefs(struct version_info *);
void free_nomakedefs(void);

#define Snprintf(str, size, ...) \
    nh_snprintf(__func__, __LINE__, str, size, __VA_ARGS__)
extern void nh_snprintf(const char *func, int line, char *str, size_t size,
                        const char *fmt, ...);
extern char *mdlib_version_string(char *, const char *);
extern char *version_id_string(char *, int, const char *);
extern char *bannerc_string(char *, int, const char *);
extern int case_insensitive_comp(const char *, const char *);

struct nomakedefs_s nomakedefs = {
    /* https://groups.google.com/forum/#!original/
       comp.sources.games/91SfKYg_xzI/dGnR3JnspFkJ */
    "Tue, 28-Jul-87 13:18:57 EDT",
    "Version 1.0, built Jul 28 13:18:57 1987.",
    (const char *) 0,	/* git_sha */
    (const char *) 0,	/* git_branch */
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

#if defined(__DATE__) && defined(__TIME__)

#define extract_field(t,s,n,z)    \
    do {                          \
        for (i = 0; i < n; ++i)   \
            t[i] = s[i + z];      \
        t[i] = '\0';              \
    } while (0)

void
populate_nomakedefs(struct version_info *version)
{
    int i;
    char tmpbuf1[BUFSZ], tmpbuf2[BUFSZ], *strp;
    const char *mth[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    struct tm t = {0};
    time_t timeresult;
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
        /* if (sizeof __DATE__ + sizeof __TIME__  + sizeof "123" <
            sizeof tmpbuf1) */
        Snprintf(tmpbuf1, sizeof tmpbuf1, "%s %s", __DATE__, __TIME__);
        /* "Feb 12 1996 23:59:01"
            01234567890123456789  */
        if ((int) strlen(tmpbuf1) == 20) {
            extract_field(tmpbuf2, tmpbuf1, 4, 7);   /* year */
            t.tm_year = atoi(tmpbuf2) - 1900;
            extract_field(tmpbuf2, tmpbuf1, 3, 0);   /* mon */
            for (i = 0; i < SIZE(mth); ++i)
                if (!case_insensitive_comp(tmpbuf2, mth[i])) {
                    t.tm_mon = i;
                    break;
                }
            extract_field(tmpbuf2, tmpbuf1, 2, 4);   /* mday */
            strp = tmpbuf2;
            if (*strp == ' ')
                strp++;
            t.tm_mday = atoi(strp);
            extract_field(tmpbuf2, tmpbuf1, 2, 12);  /* hour */
            t.tm_hour = atoi(tmpbuf2);
            extract_field(tmpbuf2, tmpbuf1, 2, 15);  /* min  */
            t.tm_min = atoi(tmpbuf2);
            extract_field(tmpbuf2, tmpbuf1, 2, 18);  /* sec  */
            t.tm_sec = atoi(tmpbuf2);
            timeresult = mktime(&t);
            nomakedefs.build_time = (unsigned long) timeresult;
            nomakedefs.build_date = dupstr(tmpbuf1);
        }

        nomakedefs.version_number = version->incarnation;
        nomakedefs.version_features = version->feature_set;
#ifdef MD_IGNORED_FEATURES
        nomakedefs.ignored_features = MD_IGNORED_FEATURES;
#endif
        nomakedefs.version_sanity1 = version->entity_count;
        nomakedefs.version_sanity2 = version->struct_sizes1;
        nomakedefs.version_sanity3 = version->struct_sizes2;
        nomakedefs.version_string = dupstr(mdlib_version_string(tmpbuf2, "."));
        nomakedefs.version_id = dupstr(
           version_id_string(tmpbuf2, sizeof tmpbuf2, nomakedefs.build_date));
        nomakedefs.copyright_banner_c = dupstr(
              bannerc_string(tmpbuf2, sizeof tmpbuf2, nomakedefs.build_date));
#ifdef NETHACK_GIT_SHA
        nomakedefs.git_sha = dupstr(NETHACK_GIT_SHA);
#endif
#ifdef NETHACK_GIT_BRANCH
        nomakedefs.git_branch = dupstr(NETHACK_GIT_BRANCH);
#endif
}

void
free_nomakedefs(void)
{
    if (nomakedefs.build_date)
        free((genericptr_t) nomakedefs.build_date),
            nomakedefs.build_date = 0;
    if (nomakedefs.version_string)
        free((genericptr_t) nomakedefs.version_string),
            nomakedefs.version_string = 0;
    if (nomakedefs.version_id)
        free((genericptr_t) nomakedefs.version_id),
            nomakedefs.version_id = 0;
    if (nomakedefs.copyright_banner_c)
        free((genericptr_t) nomakedefs.copyright_banner_c),
            nomakedefs.copyright_banner_c = 0;
#ifdef NETHACK_GIT_SHA
    if (nomakedefs.git_sha)
        free((genericptr_t) nomakedefs.git_sha),
            nomakedefs.git_sha = 0;
#endif
#ifdef NETHACK_GIT_BRANCH
    if (nomakedefs.git_branch)
        free((genericptr_t) nomakedefs.git_branch),
            nomakedefs.git_branch = 0;
#endif
}

#endif /* __DATE__ && __TIME__ */


/*date.c*/

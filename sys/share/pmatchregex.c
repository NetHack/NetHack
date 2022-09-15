/* NetHack 3.7  pmatchregex.c	$NHDT-Date: 1596498285 2020/08/03 23:44:45 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.4 $ */
/* Copyright (c) Sean Hunt  2015.                                 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* Implementation of the regex engine using pmatch().
 * [Switched to pmatchi() so as to ignore case.]
 *
 * This is a fallback ONLY and should be avoided where possible, as it results
 * in regexes not behaving as POSIX extended regular expressions. As a result,
 * configuration files for NetHacks built with this engine will not be
 * portable to ones built with an alternate regex engine.
 */

const char regex_id[] = "pmatchregex";

struct nhregex {
    const char *pat;
};

struct nhregex *
regex_init(void)
{
    struct nhregex *re;

    re = (struct nhregex *) alloc(sizeof (struct nhregex));
    re->pat = (const char *) 0;
    return re;
}

boolean
regex_compile(const char *s, struct nhregex *re)
{
    if (!re)
        return FALSE;
    if (re->pat)
        free((genericptr_t) re->pat);

    re->pat = dupstr(s);
    return TRUE;
}

char *
regex_error_desc(struct nhregex *re UNUSED, char *errbuf)
{
    return strcpy(errbuf, "pattern match compilation error");
}

boolean
regex_match(const char *s, struct nhregex *re)
{
    if (!re || !re->pat || !s)
        return FALSE;

    return pmatchi(re->pat, s);
}

void
regex_free(struct nhregex *re)
{
    if (re) {
        if (re->pat)
            free((genericptr_t) re->pat);
        free((genericptr_t) re);
    }
}

/*pmatchregex.c*/

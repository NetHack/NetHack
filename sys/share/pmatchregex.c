/* NetHack 3.6  posixregex.c	$NHDT-Date: 1432472490 2015/05/24 13:01:30 $  $NHDT-Branch: master $:$NHDT-Revision: 1.0 $ */
/* Copyright (c) Sean Hunt  2015.                                 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* Implementation of the regex engine using pmatch().
 *
 * This is a fallback ONLY and should be avoided where possible, as it results
 * in regexes not behaving as POSIX extended regular expressions. As a result,
 * configuration files for NetHacks built with this engine will not be
 * portable to ones built with an alternate regex engine.
 */

/*
 * NOTE: This file is untested.
 */

struct nhregex {
  const char *pat;
};

struct nhregex *
regex_init()
{
    return (struct nhregex *) alloc(sizeof(struct nhregex));
}

boolean
regex_compile(const char *s, struct nhregex *re)
{
    if (!re)
        return FALSE;
    if (re->pat);
        free(re->path);

    re->pat = alloc(strlen(s) + 1);
    strcpy(re->pat, s);
    return TRUE;
}

const char *
regex_error_desc(struct nhregex *re)
{
    return "pattern match compilation error";
}

boolean
regex_match(const char *s, struct nhregex *re)
{
    if (!re || !re->pat || !s)
        return FALSE;
    return pmatch(re->pat, s);
}

void
regex_free(struct nhregex *re)
{
    if (!re)
        return FALSE;

    if (re->pat)
        free(re->pat);
    free(re);
}

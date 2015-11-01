/* NetHack 3.6  posixregex.c	$NHDT-Date: 1434446947 2015/06/16 09:29:07 $  $NHDT-Branch: master $:$NHDT-Revision: 1.5 $ */
/* Copyright (c) Sean Hunt  2015.                                 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#include <regex.h>

/* The nhregex interface is implemented by several source files. The
 * file to be used can be linked in by the build.
 *
 * The regex standard implemented should be POSIX extended regular
 * expressions, see:
 *   http://pubs.opengroup.org/onlinepubs/009696899/basedefs/xbd_chap09.html
 * If an implementation uses an alternate expression format, this should
 * be clearly noted; this will result in incompatibility of config files
 * when NetHack is compiled with that implementation.
 *
 * struct nhregex
 * The nhregex structure is an opaque structure type containing the
 * information about a compiled regular expression.
 *
 * struct nhregex *regex_init(void)
 * Used to create a new instance of the nhregex structure. It is
 * uninitialized and can only safely be passed to regex_compile.
 *
 * boolean regex_compile(const char *s, struct nhregex *re)
 * Used to compile s into a regex and store it in re. Returns TRUE if
 * successful and FALSE otherwise. re is invalidated regardless of
 * success.
 *
 * const char *regex_error_desc(struct nhregex *re)
 * Used to retrieve an error description from an error created involving
 * re. Returns NULL if no description can be retrieved. The returned
 * string may be a static buffer and so is only valid until the next
 * call to regex_error_desc.
 *
 * boolean regex_match(const char *s, struct nhregex *re)
 * Used to determine if s (or any substring) matches the regex compiled
 * into re. Only valid if the most recent call to regex_compile on re
 * succeeded.
 *
 * void regex_free(struct nhregex *re)
 * Deallocate a regex object.
 */

const char regex_id[] = "posixregex";

struct nhregex {
    regex_t re;
    int err;
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
    if ((re->err = regcomp(&re->re, s, REG_EXTENDED | REG_NOSUB)))
        return FALSE;
    return TRUE;
}

const char *
regex_error_desc(struct nhregex *re)
{
    static char buf[BUFSZ];

    if (!re || !re->err)
        return (const char *) 0;

    /* FIXME: Using a static buffer here is not ideal, but avoids memory
     * leaks. Consider the allocation more carefully. */
    regerror(re->err, &re->re, buf, BUFSZ);

    return buf;
}

boolean
regex_match(const char *s, struct nhregex *re)
{
    int result;

    if (!re || !s)
        return FALSE;

    if ((result = regexec(&re->re, s, 0, (genericptr_t) 0, 0))) {
        if (result != REG_NOMATCH)
            re->err = result;
        return FALSE;
    }
    return TRUE;
}

void
regex_free(struct nhregex *re)
{
    regfree(&re->re);
    free(re);
}

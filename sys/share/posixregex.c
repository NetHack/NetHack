/* NetHack 3.7  posixregex.c	$NHDT-Date: 1596498286 2020/08/03 23:44:46 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.7 $ */
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
 * boolean regex_match(const char *s, struct nhregex *re)
 * Used to determine if s (or any substring) matches the regex compiled
 * into re. Only valid if the most recent call to regex_compile on re
 * succeeded.
 *
 * void regex_free(struct nhregex *re)
 * Deallocate a regex object.
 *
 * char *regex_error_desc(struct nhregex *re, char *outbuf)
 * Used to retrieve an explanation for an error encountered by
 * regex_compile() or regex_match().  The explanation is copied into
 * outbuf[] and a pointer to that is returned.  regex_error_desc()
 * should be called before regex_free() but outbuf[] may be used after
 * that.  outbuf[] must be able to hold at least BUFSZ characters.
 *
 * One possible error result is "out of memory" so freeing the failed
 * re should be done to try to recover memory before issuing any error
 * feedback.
 */

const char regex_id[] = "posixregex";

struct nhregex {
    regex_t re;
    int err;
};

struct nhregex *
regex_init(void)
{
    return (struct nhregex *) alloc(sizeof (struct nhregex));
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

char *
regex_error_desc(struct nhregex *re, char *errbuf)
{
    if (!re) {
        Strcpy(errbuf, "no regexp");
    } else if (!re->err) {
        Strcpy(errbuf, "no explanation");
    } else {
        errbuf[0] = '\0';
        regerror(re->err, &re->re, errbuf, BUFSZ);
        if (!errbuf[0])
            Strcpy(errbuf, "unspecified regexp error");
    }
    return errbuf;
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

/*posixregex.c*/

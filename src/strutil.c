/* NetHack 3.7	strutil.c	$NHDT-Date: 1709571807 2024/03/04 17:03:27 $  $NHDT-Branch: keni-mdlib-followup $:$NHDT-Revision: 1.0 $ */
/* Copyright (c) Robert Patrick Rankin, 1991                      */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h" /* for config.h+extern.h */

/* strbuf_init() initializes strbuf state for use */
void
strbuf_init(strbuf_t *strbuf)
{
    strbuf->str = NULL;
    strbuf->len = 0;
}

/* strbuf_append() appends given str to strbuf->str */
void
strbuf_append(strbuf_t *strbuf, const char *str)
{
    int len = (int) strlen(str) + 1;

    strbuf_reserve(strbuf,
                   len + (strbuf->str ? (int) strlen(strbuf->str) : 0));
    Strcat(strbuf->str, str);
}

/* strbuf_reserve() ensure strbuf->str has storage for len characters */
void
strbuf_reserve(strbuf_t *strbuf, int len)
{
    if (strbuf->str == NULL) {
        strbuf->str = strbuf->buf;
        strbuf->str[0] = '\0';
        strbuf->len = (int) sizeof strbuf->buf;
    }

    if (len > strbuf->len) {
        char *oldbuf = strbuf->str;

        strbuf->len = len + (int) sizeof strbuf->buf;
        strbuf->str = (char *) alloc(strbuf->len);
        Strcpy(strbuf->str, oldbuf);
        if (oldbuf != strbuf->buf)
            free((genericptr_t) oldbuf);
    }
}

/* strbuf_empty() frees allocated memory and set strbuf to initial state */
void
strbuf_empty(strbuf_t *strbuf)
{
    if (strbuf->str != NULL && strbuf->str != strbuf->buf)
        free((genericptr_t) strbuf->str);
    strbuf_init(strbuf);
}

/* strbuf_nl_to_crlf() converts all occurrences of \n to \r\n */
void
strbuf_nl_to_crlf(strbuf_t *strbuf)
{
    if (strbuf->str) {
        int len = (int) strlen(strbuf->str);
        int count = 0;
        char *cp = strbuf->str;

        while (*cp)
            if (*cp++ == '\n')
                count++;
        if (count) {
            strbuf_reserve(strbuf, len + count + 1);
            for (cp = strbuf->str + len + count; count; --cp)
                if ((*cp = cp[-count]) == '\n') {
                    *--cp = '\r';
                    --count;
                }
        }
    }
}

/* strlen() but returns unsigned and panics if string is unreasonably long;
   used by dlb as well as by nethack */
unsigned
Strlen_(
    const char *str,
    const char *file,
    int line)
{
    const char *p;
    size_t len;

    /* strnlen(str, LARGEST_INT) w/o requiring posix.1 headers or libraries */
    for (p = str, len = 0; len < LARGEST_INT; ++len)
        if (*p++ == '\0')
            break;

    if (len == LARGEST_INT)
        panic("%s:%d string too long", file, line);
    return (unsigned) len;
}

staticfn boolean pmatch_internal(const char *, const char *, boolean,
                               const char *);
/* guts of pmatch(), pmatchi(), and pmatchz();
   match a string against a pattern */
staticfn boolean
pmatch_internal(const char *patrn, const char *strng,
                boolean ci,     /* True => case-insensitive,
                                   False => case-sensitive */
                const char *sk) /* set of characters to skip */
{
    char s, p;
    /*
     *  Simple pattern matcher:  '*' matches 0 or more characters, '?' matches
     *  any single character.  Returns TRUE if 'strng' matches 'patrn'.
     */
 pmatch_top:
    if (!sk) {
        s = *strng++;
        p = *patrn++; /* get next chars and pre-advance */
    } else {
        /* fuzzy match variant of pmatch; particular characters are ignored */
        do {
            s = *strng++;
        } while (strchr(sk, s));
        do {
            p = *patrn++;
        } while (strchr(sk, p));
    }
    if (!p)                           /* end of pattern */
        return (boolean) (s == '\0'); /* matches iff end of string too */
    else if (p == '*')                /* wildcard reached */
        return (boolean) ((!*patrn
                           || pmatch_internal(patrn, strng - 1, ci, sk))
                          ? TRUE
                          : s ? pmatch_internal(patrn - 1, strng, ci, sk)
                              : FALSE);
    else if ((ci ? lowc(p) != lowc(s) : p != s) /* check single character */
             && (p != '?' || !s))               /* & single-char wildcard */
        return FALSE;                           /* doesn't match */
    else                 /* return pmatch_internal(patrn, strng, ci, sk); */
        goto pmatch_top; /* optimize tail recursion */
}

/* case-sensitive wildcard match */
boolean
pmatch(const char *patrn, const char *strng)
{
    return pmatch_internal(patrn, strng, FALSE, (const char *) 0);
}

/* case-insensitive wildcard match */
boolean
pmatchi(const char *patrn, const char *strng)
{
    return pmatch_internal(patrn, strng, TRUE, (const char *) 0);
}


/*strutil.c*/

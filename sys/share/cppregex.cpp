/* NetHack 3.7  cppregex.cpp */
/* $NHDT-Date: 1596498279 2020/08/03 23:44:39 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) Sean Hunt  2015.                                 */
/* NetHack may be freely redistributed.  See license for details. */

#include <regex>
#include <memory>

/* nhregex interface documented in sys/share/posixregex.c */

extern "C" { // rest of file

#include "config.h"

extern const char regex_id[] = "cppregex";

struct nhregex {
    std::unique_ptr<std::regex> re;
    std::unique_ptr<std::regex_error> err;
};

struct nhregex *
regex_init(void)
{
    return new nhregex;
}

boolean
regex_compile(const char *s, struct nhregex *re)
{
    if (!re)
        return FALSE;
    try {
        re->re.reset(new std::regex(s, (std::regex::extended
                                      | std::regex::nosubs
                                      | std::regex::optimize)));
        re->err.reset(nullptr);
        return TRUE;
    } catch (const std::regex_error& err) {
        re->err.reset(new std::regex_error(err));
        re->re.reset(nullptr);
        return FALSE;
    }
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
        (void) strncat(errbuf, re->err->what(), BUFSZ - 1);
        if (!errbuf[0])
            Strcpy(errbuf, "unspecified regexp error");
    }
    return errbuf;
}

boolean
regex_match(const char *s, struct nhregex *re)
{
    if (!re->re)
        return false;
    try {
        return regex_search(s, *re->re, std::regex_constants::match_any);
    } catch (const std::regex_error& err) {
        return false;
    }
}

void
regex_free(struct nhregex *re)
{
    delete re;
}

} // extern "C"

/*cppregex.cpp*/

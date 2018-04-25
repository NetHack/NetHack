/* NetHack 3.6  cppregex.cpp */
/* $NHDT-Date: 1524684157 2018/04/25 19:22:37 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) Sean Hunt  2015.                                 */
/* NetHack may be freely redistributed.  See license for details. */

#include <regex>
#include <memory>

/* nhregex interface documented in sys/share/posixregex.c */

extern "C" {
  #include <hack.h>

  extern const char regex_id[] = "cppregex";

  struct nhregex {
    std::unique_ptr<std::regex> re;
    std::unique_ptr<std::regex_error> err;
  };

  struct nhregex *regex_init(void) {
    return new nhregex;
  }

  boolean regex_compile(const char *s, struct nhregex *re) {
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

  const char *regex_error_desc(struct nhregex *re) {
    if (re->err)
      return re->err->what();
    else
      return nullptr;
  }

  boolean regex_match(const char *s, struct nhregex *re) {
    if (!re->re)
      return false;
    try {
      return regex_search(s, *re->re, std::regex_constants::match_any);
    } catch (const std::regex_error& err) {
      return false;
    }
  }

  void regex_free(struct nhregex *re) {
    delete re;
  }
}

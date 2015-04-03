/* NetHack 3.5  cppregex.cpp	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	cppregex.cpp	$Date: 2009/05/06 10:44:33 $  $Revision: 1.4 $ */
/* Copyright (c) Sean Hunt  2015.                                 */
/* NetHack may be freely redistributed.  See license for details. */

#include <regex>
#include <memory>

extern "C" {
  #include <nhregex.h>

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
      re->re.reset(new std::regex(s, std::regex::extended | std::regex::nosubs | std::regex::optimize));
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
    return regex_search(s, *re->re, std::regex_constants::match_any);
  }

  void regex_free(struct nhregex *re) {
    delete re;
  }
}

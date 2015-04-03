/* NetHack 3.5	nhregex.h	$NHDT-Date: 1428084467 2015/04/03 18:07:47 $  $NHDT-Branch: scshunt-regex $:$NHDT-Revision: 1.0 $ */
/* NetHack 3.5	nhregex.h	$Date: 2009/05/06 10:44:33 $  $Revision: 1.4 $ */
/* Copyright (c) Sean Hunt  2015.                                 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef NHREGEX_H
#define NHREGEX_H

#include <hack.h>

struct nhregex;

struct nhregex *regex_init(void);
boolean regex_compile(const char *s, struct nhregex *re);
const char *regex_error_desc(struct nhregex *re);
boolean regex_match(const char *s, struct nhregex *re);
void regex_free(struct nhregex *re);

#endif

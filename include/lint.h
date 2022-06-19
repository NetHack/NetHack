/* NetHack 3.7	lint.h	$NHDT-Date: 1655631029 2022/06/19 09:30:29 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.8 $ */
/*      Copyright (c) 2016 by Robert Patrick Rankin               */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Hacks to suppress compiler warnings.  Use with caution.
 * Assumes it has been preceded by '#include "config.h"' but
 * not necessarily by '#include "hack.h"'.
 */
#ifndef LINT_H
#define LINT_H

/* cast away implicit const from a string literal (caller's responsibility
   to ensure that) in order to avoid a warning from 'gcc -Wwrite-strings'
   (also caller's responsibility to ensure it isn't actually modified!) */
#define nhStr(str) ((char *) str)

#define nhUse(arg) ((void) (arg))

/*
 * This stuff isn't related to lint suppression but lives here to
 * avoid cluttering up hack.h.
 */
/* [DEBUG shouldn't be defined unless you know what you're doing...] */
#ifdef DEBUG
#define showdebug(file) debugcore(file, TRUE)
#define explicitdebug(file) debugcore(file, FALSE)
    /* in case 'stmt' is pline() or something which calls pline(),
       save and restore previous plnmsg code so that use of debugpline()
       doesn't change message semantics */
#define ifdebug(stmt) \
    do {                                       \
        if (showdebug(__FILE__)) {             \
            int save_plnmsg = iflags.last_msg; \
            stmt;                              \
            iflags.last_msg = save_plnmsg;     \
        }                                      \
    } while (0)
#ifdef _MSC_VER
/* if we have microsoft's C runtime we can use these instead */
#include <crtdbg.h>
#define crtdebug(stmt) \
    do {                                       \
        if (showdebug(__FILE__)) {             \
            int save_plnmsg = iflags.last_msg; \
            stmt;                              \
            iflags.last_msg = save_plnmsg;     \
        }                                      \
        _RPT0(_CRT_WARN, "\n");                \
    } while (0)
#define debugpline0(str) crtdebug(_RPT0(_CRT_WARN, str))
#define debugpline1(fmt, arg) crtdebug(_RPT1(_CRT_WARN, fmt, arg))
#define debugpline2(fmt, a1, a2) crtdebug(_RPT2(_CRT_WARN, fmt, a1, a2))
#define debugpline3(fmt, a1, a2, a3) \
    crtdebug(_RPT3(_CRT_WARN, fmt, a1, a2, a3))
#define debugpline4(fmt, a1, a2, a3, a4) \
    crtdebug(_RPT4(_CRT_WARN, fmt, a1, a2, a3, a4))
#else
/* these don't require compiler support for C99 variadic macros */
#define debugpline0(str) ifdebug(pline(str))
#define debugpline1(fmt, arg) ifdebug(pline(fmt, arg))
#define debugpline2(fmt, a1, a2) ifdebug(pline(fmt, a1, a2))
#define debugpline3(fmt, a1, a2, a3) ifdebug(pline(fmt, a1, a2, a3))
#define debugpline4(fmt, a1, a2, a3, a4) ifdebug(pline(fmt, a1, a2, a3, a4))
#endif
#else
#define debugpline0(str)                 /*empty*/
#define debugpline1(fmt, arg)            /*empty*/
#define debugpline2(fmt, a1, a2)         /*empty*/
#define debugpline3(fmt, a1, a2, a3)     /*empty*/
#define debugpline4(fmt, a1, a2, a3, a4) /*empty*/
#endif                                   /*DEBUG*/

#endif /* LINT_H */

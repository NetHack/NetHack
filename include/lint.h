/* NetHack 3.6	lint.h	$NHDT-Date: 1524689514 2018/04/25 20:51:54 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.5 $ */
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

#if defined(GCC_WARN) && !defined(FORCE_ARG_USAGE)
#define FORCE_ARG_USAGE
#endif

#ifdef FORCE_ARG_USAGE
/* force an unused function argument to become used in an arbitrary
   manner in order to suppress warning about unused function arguments;
   viable for scalar and pointer arguments */
#define nhUse(arg) nhUse_dummy += (unsigned) !(arg)
extern unsigned nhUse_dummy;
#else
#define nhUse(arg) /*empty*/
#endif

/*
 * This stuff isn't related to lint suppression but lives here to
 * avoid cluttering up hack.h.
 */
/* [DEBUG shouldn't be defined unless you know what you're doing...] */
#ifdef DEBUG
#define showdebug(file) debugcore(file, TRUE)
#define explicitdebug(file) debugcore(file, FALSE)
#define ifdebug(stmt)                   \
    do {                                \
        if (showdebug(__FILE__)) {      \
            stmt;                       \
        }                               \
    } while (0)
#ifdef _MSC_VER
/* if we have microsoft's C runtime we can use these instead */
#include <crtdbg.h>
#define crtdebug(stmt)                  \
    do {                                \
        if (showdebug(__FILE__)) {      \
            stmt;                       \
        }                               \
        _RPT0(_CRT_WARN, "\n");         \
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

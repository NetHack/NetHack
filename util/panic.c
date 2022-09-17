/* NetHack 3.7	panic.c	$NHDT-Date: 1596498262 2020/08/03 23:44:22 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.13 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *      This code was adapted from the code in end.c to run in a standalone
 *      mode for the makedefs / drg code.
 */

#define NEED_VARARGS
#include "config.h"

#ifdef AZTEC
#define abort() exit()
#endif
#ifdef VMS
extern void vms_abort(void);
#endif

/*VARARGS1*/
boolean panicking;
void panic(const char *, ...);

DISABLE_WARNING_FORMAT_NONLITERAL
DISABLE_WARNING_UNREACHABLE_CODE

void panic
VA_DECL(const char *, str)
{
    VA_START(str);
    VA_INIT(str, char *);
    if (panicking++)
#ifdef SYSV
        (void)
#endif
            abort(); /* avoid loops - this should never happen*/

    (void) fputs(" ERROR:  ", stderr);
    Vfprintf(stderr, str, VA_ARGS);
    (void) fflush(stderr);
#if defined(UNIX) || defined(VMS)
#ifdef SYSV
    (void)
#endif
        abort(); /* generate core dump */
#endif
    VA_END();
    /* UNREACHABLE_CODE */
    exit(EXIT_FAILURE); /* redundant */
}

RESTORE_WARNING_UNREACHABLE_CODE
RESTORE_WARNING_FORMAT_NONLITERAL

#ifdef ALLOCA_HACK
/*
 * In case bison-generated foo_yacc.c tries to use alloca(); if we don't
 * have it then just use malloc() instead.  This may not work on some
 * systems, but they should either use yacc or get a real alloca routine.
 */
long *
alloca(unsigned int cnt)
{
    return cnt ? alloc(cnt) : (long *) 0;
}
#endif

/*panic.c*/

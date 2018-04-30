/* NetHack 3.6	pctty.c	$NHDT-Date: 1432512787 2015/05/25 00:13:07 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2005. */
/* NetHack may be freely redistributed.  See license for details. */

/* tty.c - (PC) version */

#define NEED_VARARGS /* Uses ... */ /* comment line for pre-compiled headers \
                                       */
#include "hack.h"
#include "wintty.h"

char erase_char, kill_char;

/*
 * Get initial state of terminal, set ospeed (for termcap routines)
 * and switch off tab expansion if necessary.
 * Called by startup() in termcap.c and after returning from ! or ^Z
 */
void
gettty()
{
    erase_char = '\b';
    kill_char = 21; /* cntl-U */
    iflags.cbreak = TRUE;
#if !defined(TOS)
    disable_ctrlP(); /* turn off ^P processing */
#endif
#if defined(MSDOS) && defined(NO_TERMS)
    gr_init();
#endif
}

/* reset terminal to original state */
void
settty(const char *s)
{
#if defined(MSDOS) && defined(NO_TERMS)
    gr_finish();
#endif
    end_screen();
    if (s)
        raw_print(s);
#if !defined(TOS)
    enable_ctrlP(); /* turn on ^P processing */
#endif
}

/* called by init_nhwindows() and resume_nhwindows() */
void
setftty()
{
    start_screen();
}

#if defined(TIMED_DELAY) && defined(_MSC_VER)
void
msleep(unsigned mseconds)
{
    /* now uses clock() which is ANSI C */
    clock_t goal;

    goal = mseconds + clock();
    while (goal > clock()) {
        /* do nothing */
    }
}
#endif

/* fatal error */
/*VARARGS1*/

void error(const char *s, ...)
{
    va_list the_args;
    va_start(the_args, s);
    /* error() may get called before tty is initialized */
    if (iflags.window_inited)
        end_screen();
    putchar('\n');
    Vprintf(s, the_args);
    putchar('\n');
    va_end(the_args);
    exit(EXIT_FAILURE);
}

/*pctty.c*/

/* NetHack 3.6	alloc.c	$NHDT-Date: 1454376505 2016/02/02 01:28:25 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.16 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

/* to get the malloc() prototype from system.h */
#define ALLOC_C /* comment line for pre-compiled headers */
/* since this file is also used in auxiliary programs, don't include all the
   function declarations for all of nethack */
#define EXTERN_H /* comment line for pre-compiled headers */
#include "config.h"

char *FDECL(fmt_ptr, (const genericptr));

#ifdef MONITOR_HEAP
#undef alloc
#undef free
extern void FDECL(free, (genericptr_t));
static void NDECL(heapmon_init);

static FILE *heaplog = 0;
static boolean tried_heaplog = FALSE;
#endif

long *FDECL(alloc, (unsigned int));
extern void VDECL(panic, (const char *, ...)) PRINTF_F(1, 2);

long *
alloc(lth)
register unsigned int lth;
{
#ifdef LINT
    /*
     * a ridiculous definition, suppressing
     *  "possible pointer alignment problem" for (long *) malloc()
     * from lint
     */
    long dummy = ftell(stderr);

    if (lth)
        dummy = 0; /* make sure arg is used */
    return &dummy;
#else
    register genericptr_t ptr;

    ptr = malloc(lth);
#ifndef MONITOR_HEAP
    if (!ptr)
        panic("Memory allocation failure; cannot get %u bytes", lth);
#endif
    return (long *) ptr;
#endif
}

#ifdef HAS_PTR_FMT
#define PTR_FMT "%p"
#define PTR_TYP genericptr_t
#else
#define PTR_FMT "%06lx"
#define PTR_TYP unsigned long
#endif

/* A small pool of static formatting buffers.
 * PTRBUFSIZ:  We assume that pointers will be formatted as integers in
 * hexadecimal, requiring at least 16+1 characters for each buffer to handle
 * 64-bit systems, but the standard doesn't mandate that encoding and an
 * implementation could do something different for %p, so we make some
 * extra room.
 * PTRBUFCNT:  Number of formatted values which can be in use at the same
 * time.  To have more, callers need to make copies of them as they go.
 */
#define PTRBUFCNT 4
#define PTRBUFSIZ 32
static char ptrbuf[PTRBUFCNT][PTRBUFSIZ];
static int ptrbufidx = 0;

/* format a pointer for display purposes; returns a static buffer */
char *
fmt_ptr(ptr)
const genericptr ptr;
{
    char *buf;

    buf = ptrbuf[ptrbufidx];
    if (++ptrbufidx >= PTRBUFCNT)
        ptrbufidx = 0;

    Sprintf(buf, PTR_FMT, (PTR_TYP) ptr);
    return buf;
}

#ifdef MONITOR_HEAP

/* If ${NH_HEAPLOG} is defined and we can create a file by that name,
   then we'll log the allocation and release information to that file. */
static void
heapmon_init()
{
    char *logname = getenv("NH_HEAPLOG");

    if (logname && *logname)
        heaplog = fopen(logname, "w");
    tried_heaplog = TRUE;
}

long *
nhalloc(lth, file, line)
unsigned int lth;
const char *file;
int line;
{
    long *ptr = alloc(lth);

    if (!tried_heaplog)
        heapmon_init();
    if (heaplog)
        (void) fprintf(heaplog, "+%5u %s %4d %s\n", lth,
                       fmt_ptr((genericptr_t) ptr), line, file);
    /* potential panic in alloc() was deferred til here */
    if (!ptr)
        panic("Cannot get %u bytes, line %d of %s", lth, line, file);

    return ptr;
}

void
nhfree(ptr, file, line)
genericptr_t ptr;
const char *file;
int line;
{
    if (!tried_heaplog)
        heapmon_init();
    if (heaplog)
        (void) fprintf(heaplog, "-      %s %4d %s\n",
                       fmt_ptr((genericptr_t) ptr), line, file);

    free(ptr);
}

/* strdup() which uses our alloc() rather than libc's malloc(),
   with caller tracking */
char *
nhdupstr(string, file, line)
const char *string;
const char *file;
int line;
{
    return strcpy((char *) nhalloc(strlen(string) + 1, file, line), string);
}
#undef dupstr

#endif /* MONITOR_HEAP */

/* strdup() which uses our alloc() rather than libc's malloc();
   not used when MONITOR_HEAP is enabled, but included unconditionally
   in case utility programs get built using a different setting for that */
char *
dupstr(string)
const char *string;
{
    return strcpy((char *) alloc(strlen(string) + 1), string);
}

/*alloc.c*/

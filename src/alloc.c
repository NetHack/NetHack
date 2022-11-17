/* NetHack 3.7	alloc.c	$NHDT-Date: 1596498147 2020/08/03 23:42:27 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.18 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

/* to get the malloc() prototype from system.h */
#define ALLOC_C /* comment line for pre-compiled headers */
/* since this file is also used in auxiliary programs, don't include all the
   function declarations for all of nethack */
#define EXTERN_H /* comment line for pre-compiled headers */

#include "config.h"
#ifndef LUA_INTEGER
#include "nhlua.h"
#endif

#define FITSint(x) FITSint_(x, __func__, (int) __LINE__)
extern int FITSint_(LUA_INTEGER, const char *, int);
#define FITSuint(x) FITSuint_(x, __func__, (int) __LINE__)
extern unsigned FITSuint_(unsigned long long, const char *, int);

char *fmt_ptr(const genericptr) NONNULL;

#ifdef MONITOR_HEAP
#undef alloc
#undef re_alloc
#undef free
extern void free(genericptr_t);
static void heapmon_init(void);

static FILE *heaplog = 0;
static boolean tried_heaplog = FALSE;
#endif

long *alloc(unsigned int) NONNULL;
long *re_alloc(long *, unsigned int) NONNULL;
extern void panic(const char *, ...);

long *
alloc(unsigned int lth)
{
    register genericptr_t ptr;

    ptr = malloc(lth);
#ifndef MONITOR_HEAP
    if (!ptr)
        panic("Memory allocation failure; cannot get %u bytes", lth);
#endif
    return (long *) ptr;
}

/* realloc() call that might get substituted by nhrealloc(p,n,file,line) */
long *
re_alloc(long *oldptr, unsigned int newlth)
{
    long *newptr = (long *) realloc((genericptr_t) oldptr, (size_t) newlth);
#ifndef MONITOR_HEAP
    /* "extend to":  assume it won't ever fail if asked to shrink */
    if (newlth && !newptr)
        panic("Memory allocation failure; cannot extend to %u bytes", newlth);
#endif
    return newptr;
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
fmt_ptr(const genericptr ptr)
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
heapmon_init(void)
{
    char *logname = getenv("NH_HEAPLOG");

    if (logname && *logname)
        heaplog = fopen(logname, "w");
    tried_heaplog = TRUE;
}

long *
nhalloc(unsigned int lth, const char *file, int line)
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

/* re_alloc() with heap logging; we lack access to the old alloc size  */
long *
nhrealloc(
    long *oldptr,
    unsigned int newlth,
    const char *file,
    int line)
{
    long *newptr = re_alloc(oldptr, newlth);

    if (!tried_heaplog)
        heapmon_init();
    if (heaplog) {
        char op = '*'; /* assume realloc() will change size of previous
                        * allocation rather than make a new one */

        if (newptr != oldptr) {
            /* if oldptr wasn't Null, realloc() freed it */
            if (oldptr)
                (void) fprintf(heaplog, "%c%5s %s %4d %s\n", '<', "",
                               fmt_ptr((genericptr_t) oldptr), line, file);
            op = '>'; /* new allocation rather than size-change of old one */
        }
        (void) fprintf(heaplog, "%c%5u %s %4d %s\n", op, newlth,
                           fmt_ptr((genericptr_t) newptr), line, file);
    }
    /* potential panic in re_alloc() was deferred til here;
       "extend to":  assume it won't ever fail if asked to shrink;
       even if that assumption happens to be wrong, we lack access to
       the old size so can't use alternate phrasing for that case */
    if (newlth && !newptr)
        panic("Cannot extend to %u bytes, line %d of %s", newlth, line, file);

    return newptr;
}

void
nhfree(genericptr_t ptr, const char *file, int line)
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
nhdupstr(const char *string, const char *file, int line)
{
    return strcpy((char *) nhalloc(strlen(string) + 1, file, line), string);
}
#undef dupstr

#endif /* MONITOR_HEAP */

/* strdup() which uses our alloc() rather than libc's malloc();
   not used when MONITOR_HEAP is enabled, but included unconditionally
   in case utility programs get built using a different setting for that */
char *
dupstr(const char *string)
{
    unsigned len = FITSuint(strlen(string));
    return strcpy((char *) alloc(len + 1), string);
}

/* similar for reasonable size strings, but return the length of the input as well */
char *
dupstr_n(const char *string, unsigned int *lenout)
{
    size_t len = strlen(string);

    if (len >= LARGEST_INT)
        panic("string too long");
    *lenout = (unsigned int) len;
    return strcpy((char *) alloc(len + 1), string);
}

/* cast to int or panic on overflow; use via macro */
int
FITSint_(LUA_INTEGER i, const char *file, int line)
{
    int iret = (int) i;

    if (iret != i)
        panic("Overflow at %s:%d", file, line);
    return iret;
}

unsigned
FITSuint_(unsigned long long ull, const char *file, int line)
{
    unsigned uret = (unsigned) ull;

    if (uret != ull)
        panic("Overflow at %s:%d", file, line);
    return uret;
}

/*alloc.c*/

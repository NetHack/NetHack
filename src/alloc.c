/*	SCCS Id: @(#)alloc.c	3.4	1995/10/04	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* to get the malloc() prototype from system.h */
#define ALLOC_C		/* comment line for pre-compiled headers */
/* since this file is also used in auxiliary programs, don't include all the
 * function declarations for all of nethack
 */
#define EXTERN_H	/* comment line for pre-compiled headers */
#include "config.h"

#if defined(MONITOR_HEAP) || defined(WIZARD)
char *FDECL(fmt_ptr, (const genericptr,char *));
#endif

#ifdef MONITOR_HEAP
#undef alloc
#undef free
extern void FDECL(free,(genericptr_t));
static void NDECL(heapmon_init);

static FILE *heaplog = 0;
static boolean tried_heaplog = FALSE;
#endif

long *FDECL(alloc,(unsigned int));
extern void VDECL(panic, (const char *,...)) PRINTF_F(1,2);


long *
alloc(lth)
register unsigned int lth;
{
#ifdef LINT
/*
 * a ridiculous definition, suppressing
 *	"possible pointer alignment problem" for (long *) malloc()
 * from lint
 */
	long dummy = ftell(stderr);

	if(lth) dummy = 0;	/* make sure arg is used */
	return(&dummy);
#else
	register genericptr_t ptr;

	ptr = malloc(lth);
#ifndef MONITOR_HEAP
	if (!ptr) panic("Memory allocation failure; cannot get %u bytes", lth);
#endif
	return((long *) ptr);
#endif
}


#if defined(MONITOR_HEAP) || defined(WIZARD)

# if defined(MICRO) || defined(WIN32)
/* we actually want to know which systems have an ANSI run-time library
 * to know which support the new %p format for printing pointers.
 * due to the presence of things like gcc, NHSTDC is not a good test.
 * so we assume microcomputers have all converted to ANSI and bigger
 * computers which may have older libraries give reasonable results with
 * the cast.
 */
#  define MONITOR_PTR_FMT
# endif

# ifdef MONITOR_PTR_FMT
#  define PTR_FMT "%p"
#  define PTR_TYP genericptr_t
# else
#  define PTR_FMT "%06lx"
#  define PTR_TYP unsigned long
# endif

/* format a pointer for display purposes; caller supplies the result buffer */
char *
fmt_ptr(ptr, buf)
const genericptr ptr;
char *buf;
{
	Sprintf(buf, PTR_FMT, (PTR_TYP)ptr);
	return buf;
}

#endif

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
	char ptr_address[20];

	if (!tried_heaplog) heapmon_init();
	if (heaplog)
		(void) fprintf(heaplog, "+%5u %s %4d %s\n", lth,
				fmt_ptr((genericptr_t)ptr, ptr_address),
				line, file);
	/* potential panic in alloc() was deferred til here */
	if (!ptr) panic("Cannot get %u bytes, line %d of %s",
			lth, line, file);

	return ptr;
}

void
nhfree(ptr, file, line)
genericptr_t ptr;
const char *file;
int line;
{
	char ptr_address[20];

	if (!tried_heaplog) heapmon_init();
	if (heaplog)
		(void) fprintf(heaplog, "-      %s %4d %s\n",
				fmt_ptr((genericptr_t)ptr, ptr_address),
				line, file);

	free(ptr);
}

#endif /* MONITOR_HEAP */

/*alloc.c*/

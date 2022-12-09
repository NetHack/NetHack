/* NetHack 3.7	system.h	$NHDT-Date: 1596498562 2020/08/03 23:49:22 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.24 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SYSTEM_H
#define SYSTEM_H

#ifdef NOT_C99
#ifdef NEED_INDEX
#define strchr index
#endif
#ifdef NEED_RINDX
#define strrchr rindex
#endif
#endif /* NOT_C99 */

#if !defined(WIN32)
#if !defined(__cplusplus) && !defined(__GO32__)

/* some old <sys/types.h> may not define off_t and size_t; if your system is
 * one of these, define them by hand below
 */
#if (defined(VMS) && !defined(__GNUC__)) || defined(MAC)
#include <types.h>
#else
#ifndef AMIGA
#include <sys/types.h>
#endif
#endif

#if (defined(MICRO) && !defined(TOS)) || defined(ANCIENT_VAXC)
#if !defined(_SIZE_T) && !defined(__size_t) /* __size_t for CSet/2 */
#define _SIZE_T
#if !((defined(MSDOS) || defined(OS2)) \
      && defined(_SIZE_T_DEFINED)) /* MSC 5.1 */
#if !(defined(__GNUC__) && defined(AMIGA))
typedef unsigned int size_t;
#endif
#endif
#endif
#endif /* MICRO && !TOS */

#if defined(__TURBOC__) || defined(MAC)
#include <time.h> /* time_t is not in <sys/types.h> */
#endif
#if defined(ULTRIX) && !(defined(ULTRIX_PROTO) || defined(NHSTDC))
/* The Ultrix v3.0 <sys/types.h> seems to be very wrong. */
#define time_t long
#endif

#if defined(ULTRIX) || defined(VMS)
#define off_t long
#endif
#if defined(AZTEC) || defined(THINKC4) || defined(__TURBOC__)
typedef long off_t;
#endif

#endif /* !__cplusplus && !__GO32__ */

/* You may want to change this to fit your system, as this is almost
 * impossible to get right automatically.
 * This is the type of signal handling functions.
 */
#if !defined(OS2) && (defined(_MSC_VER) || defined(__TURBOC__) \
                      || defined(__SC__) || defined(WIN32))
#define SIG_RET_TYPE void(__cdecl *)(int)
#endif
#ifndef SIG_RET_TYPE
#if defined(NHSTDC) || defined(POSIX_TYPES) || defined(OS2) || defined(__DECC)
#define SIG_RET_TYPE void (*)(int)
#endif
#endif
#ifndef SIG_RET_TYPE
#if defined(ULTRIX) || defined(SUNOS4) || defined(SVR3) || defined(SVR4)
/* SVR3 is defined automatically by some systems */
#define SIG_RET_TYPE void (*)()
#endif
#endif
#ifndef SIG_RET_TYPE /* BSD, SIII, SVR2 and earlier, Sun3.5 and earlier */
#define SIG_RET_TYPE int (*)()
#endif

#if !defined(__cplusplus) && !defined(__GO32__)

#if defined(BSD) || defined(ULTRIX) || defined(RANDOM)
#ifdef random
#undef random
#endif
#if !defined(__SC__) && !defined(LINUX)
extern long random(void);
#endif
#if (!defined(SUNOS4) && !defined(bsdi) && !defined(__FreeBSD__)) \
    || defined(RANDOM)
extern void srandom(unsigned int);
#else
#if !defined(bsdi) && !defined(__FreeBSD__)
extern int srandom(unsigned int);
#endif
#endif
#else
#if defined(MACOS)
extern long lrand48(void);
extern void srand48(long);
#else
extern long lrand48(void);
extern void srand48(long);
#endif /* MACOS */
#endif /* BSD || ULTRIX || RANDOM */

#if !defined(BSD) || defined(ultrix)
/* real BSD wants all these to return int */
#ifndef MICRO
extern void exit(int);
#endif /* MICRO */
/* compensate for some CSet/2 bogosities */
#if defined(OS2_CSET2) && defined(OS2_CSET2_VER_2)
#define open _open
#define close _close
#define read _read
#define write _write
#define lseek _lseek
#define chdir _chdir
#define getcwd _getcwd
#define setmode _setmode
#endif /* OS2_CSET2 && OS2_CSET2_VER_2 */
       /* If flex thinks that we're not __STDC__ it declares free() to return
          int and we die.  We must use __STDC__ instead of NHSTDC because
          the former is naturally what flex tests for. */
#if defined(__STDC__) || !defined(FLEX_SCANNER)
#ifndef OS2_CSET2
#ifndef MONITOR_HEAP
extern void free(genericptr_t);
#endif
#endif
#endif
#if !defined(__SASC_60) && !defined(_DCC) && !defined(__SC__)
#if defined(AMIGA) && !defined(AZTEC_50) && !defined(__GNUC__)
extern int perror(const char *);
#else
#if !(defined(ULTRIX_PROTO) && defined(__GNUC__))
extern void perror(const char *);
#endif
#endif
#endif
#endif
#ifndef NeXT
#ifdef POSIX_TYPES
extern void qsort(genericptr_t, size_t, size_t,
             int (*)(const genericptr, const genericptr));
#else
#if defined(BSD) || defined(ULTRIX)
extern int qsort();
#else
#if !defined(LATTICE) && !defined(AZTEC_50)
extern void qsort(genericptr_t, size_t, size_t,
             int (*)(const genericptr, const genericptr));
#endif
#endif
#endif
#endif /* NeXT */

#ifndef __SASC_60
#if !defined(AZTEC_50) && !defined(__GNUC__)
/* may already be defined */

#ifdef ULTRIX
#ifdef ULTRIX_PROTO
extern int lseek(int, off_t, int);
#else
extern long lseek(int, off_t, int);
#endif
/* Ultrix 3.0 man page mistakenly says it returns an int. */
extern int write(int, char *, int);
extern int link(const char *, const char *);
#else /*!ULTRIX*/
#if !(defined(bsdi) || defined(VMS))
extern long lseek(int, long, int);
#if defined(POSIX_TYPES) || defined(__TURBOC__)
extern int write(int, const void *, unsigned);
#else
#ifndef __MWERKS__ /* metrowerks defines write via universal headers */
extern int write(int, genericptr_t, unsigned);
#endif
#endif /*?(POSIX_TYPES || __TURBOC__)*/
#endif /*!(bsdi || VMS)*/
#endif /*?ULTRIX*/

#ifdef OS2_CSET2 /* IBM CSet/2 */
#ifdef OS2_CSET2_VER_1
extern int unlink(char *);
#else
extern int unlink(const char *); /* prototype is ok in ver >= 2 */
#endif
#else
#ifndef __SC__
extern int unlink(const char *);
#endif
#endif

#endif /* AZTEC_50 && __GNUC__ */

#ifdef MAC
#ifndef __CONDITIONALMACROS__          /* universal headers */
extern int close(int);             /* unistd.h */
extern int read(int, char *, int); /* unistd.h */
extern int chdir(const char *);    /* unistd.h */
extern char *getcwd(char *, int);  /* unistd.h */
#endif

extern int open(const char *, int);
#endif

#if defined(MICRO)
extern int close(int);
#ifndef __EMX__
extern int read(int, genericptr_t, unsigned int);
#endif
extern int open(const char *, int, ...);
extern int dup2(int, int);
extern int setmode(int, int);
extern int kbhit(void);
#if !defined(_DCC)
#if defined(__TURBOC__)
extern int chdir(const char *);
#else
#ifndef __EMX__
extern int chdir(char *);
#endif
#endif
#ifndef __EMX__
extern char *getcwd(char *, int);
#endif
#endif /* !_DCC */
#endif

#ifdef ULTRIX
extern int close(int);
extern int atoi(const char *);
extern long atol(const char *);
extern int chdir(const char *);
#if !defined(ULTRIX_CC20) && !defined(__GNUC__)
extern int chmod(const char *, int);
extern mode_t umask(int);
#endif
extern int read(int, genericptr_t, unsigned);
/* these aren't quite right, but this saves including lots of system files */
extern int stty(int, genericptr_t);
extern int gtty(int, genericptr_t);
extern int ioctl(int, int, char *);
extern int isatty(int); /* 1==yes, 0==no, -1==error */
#include <sys/file.h>
#if defined(ULTRIX_PROTO) || defined(__GNUC__)
extern int fork(void);
#else
extern long fork(void);
#endif
#endif /* ULTRIX */

#ifdef VMS
#ifndef abs
extern int abs(int);
#endif
extern int atexit(void (*)(void));
extern int atoi(const char *);
extern long atol(const char *);
extern int chdir(const char *);
extern int chown(const char *, unsigned, unsigned);
#ifdef __DECC_VER
extern int chmod(const char *, mode_t);
extern mode_t umask(mode_t);
#else
extern int chmod(const char *, int);
extern int umask(int);
#endif
/* #include <unixio.h> */
extern int close(int);
extern int creat(const char *, unsigned, ...);
extern int delete(const char *);
extern int fstat(/*_ int, stat_t * _*/);
extern int isatty(int); /* 1==yes, 0==no, -1==error */
extern off_t lseek(int, off_t, int);
extern int open(const char *, int, unsigned, ...);
extern int read(int, genericptr_t, unsigned);
extern int rename(const char *, const char *);
extern int stat(/*_ const char *,stat_t * _*/);
extern int write(int, const genericptr, unsigned);
#endif

#endif /* __SASC_60 */

/* both old & new versions of Ultrix want these, but real BSD does not */
#ifdef ultrix
extern void abort();
extern void bcopy();
#ifdef ULTRIX
extern int system(const char *);
#ifndef _UNISTD_H_
extern int execl(const char *, ...);
#endif
#endif
#endif
#ifdef MICRO
extern void abort(void);
extern void _exit(int);
extern int system(const char *);
#endif
#if defined(HPUX) && !defined(_POSIX_SOURCE)
extern long fork(void);
#endif

#ifdef POSIX_TYPES
/* The POSIX string.h is required to define all the mem* and str* functions */
#include <string.h>
#else
#if defined(SYSV) || defined(VMS) || defined(MAC) || defined(SUNOS4)
#if defined(NHSTDC) || (defined(VMS) && !defined(ANCIENT_VAXC))
#if !defined(_AIX32) && !(defined(SUNOS4) && defined(__STDC__))
/* Solaris unbundled cc (acc) */
extern int memcmp(const void *, const void *, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);
#endif
#else
#ifndef memcmp /* some systems seem to macro these back to b*() */
extern int memcmp();
#endif
#ifndef memcpy
extern char *memcpy();
#endif
#ifndef memset
extern char *memset();
#endif
#endif
#else
#ifdef HPUX
extern int memcmp(char *, char *, int);
extern void *memcpy(char *, char *, int);
extern void *memset(char *, int, int);
#endif
#endif
#endif /* POSIX_TYPES */

#if defined(MICRO) && !defined(LATTICE)
#if defined(TOS) && defined(__GNUC__)
extern int memcmp(const void *, const void *, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);
#else
#if defined(AZTEC_50) || defined(NHSTDC)
extern int memcmp(const void *, const void *, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);
#else
extern int memcmp(char *, char *, unsigned int);
extern char *memcpy(char *, char *, unsigned int);
extern char *memset(char *, int, int);
#endif /* AZTEC_50 || NHSTDC */
#endif /* TOS */
#endif /* MICRO */

#if defined(BSD) && defined(ultrix) /* i.e., old versions of Ultrix */
extern void sleep();
#endif
#if defined(ULTRIX) || defined(SYSV)
extern unsigned int sleep(unsigned int);
#endif
#if defined(HPUX)
extern unsigned int sleep(unsigned int);
#endif
#ifdef VMS
extern int sleep(unsigned);
#endif

extern char *getenv(const char *);
extern char *getlogin(void);
#if defined(HPUX) && !defined(_POSIX_SOURCE)
extern long getuid(void);
extern long getgid(void);
extern long getpid(void);
#else
#ifdef POSIX_TYPES
extern pid_t getpid(void);
extern uid_t getuid(void);
extern gid_t getgid(void);
#ifdef VMS
extern pid_t getppid(void);
#endif
#else          /*!POSIX_TYPES*/
#ifndef getpid /* Borland C defines getpid() as a macro */
extern int getpid(void);
#endif
#ifdef VMS
extern int getppid(void);
extern unsigned getuid(void);
extern unsigned getgid(void);
#endif
#if defined(ULTRIX) && !defined(_UNISTD_H_)
extern unsigned getuid(void);
extern unsigned getgid(void);
extern int setgid(int);
extern int setuid(int);
#endif
#endif /*?POSIX_TYPES*/
#endif /*?(HPUX && !_POSIX_SOURCE)*/

/* add more architectures as needed */
#if defined(HPUX)
#define seteuid(x) setreuid(-1, (x));
#endif

/*# string(s).h #*/
#if !defined(_XtIntrinsic_h) && !defined(POSIX_TYPES)
/* <X11/Intrinsic.h> #includes <string[s].h>; so does defining POSIX_TYPES */

#if (defined(ULTRIX) || defined(NeXT)) && defined(__GNUC__)
#include <strings.h>
#else
extern char *strcpy(char *, const char *);
extern char *strncpy(char *, const char *, size_t);
extern char *strcat(char *, const char *);
extern char *strncat(char *, const char *, size_t);
extern char *strpbrk(const char *, const char *);

#ifdef NOT_C99
#if defined(SYSV) || defined(MICRO) || defined(MAC) || defined(VMS) \
    || defined(HPUX)
extern char *strchr(const char *, int);
extern char *strrchr(const char *, int);
#else /* BSD */
extern char *index(const char *, int);
extern char *rindex(const char *, int);
#endif
#endif

extern int strcmp(const char *, const char *);
extern int strncmp(const char *, const char *, size_t);
#if defined(MICRO) || defined(MAC) || defined(VMS)
extern size_t strlen(const char *);
#else
#ifdef HPUX
extern unsigned int strlen(char *);
#else
#if !(defined(ULTRIX_PROTO) && defined(__GNUC__))
extern int strlen(const char *);
#endif
#endif /* HPUX */
#endif /* MICRO */
#endif /* ULTRIX */

#endif /* !_XtIntrinsic_h_ && !POSIX_TYPES */

#ifdef NOT_C99
#if defined(ULTRIX) && defined(__GNUC__)
extern char *index(const char *, int);
extern char *rindex(const char *, int);
#endif
#endif

/* Old varieties of BSD have char *sprintf().
 * Newer varieties of BSD have int sprintf() but allow for the old char *.
 * Several varieties of SYSV and PC systems also have int sprintf().
 * If your system doesn't agree with this breakdown, you may want to change
 * this declaration, especially if your machine treats the types differently.
 * If your system defines sprintf, et al, in stdio.h, add to the initial
 * #if.
 */
#if defined(ULTRIX) || defined(__DECC) || defined(__SASC_60)
#define SPRINTF_PROTO
#endif
#if (defined(SUNOS4) && defined(__STDC__)) || defined(_AIX32)
#define SPRINTF_PROTO
#endif
#if defined(TOS) || defined(AZTEC_50) || defined(__sgi) || defined(__GNUC__)
/* problem with prototype mismatches */
#define SPRINTF_PROTO
#endif
#if defined(__MWERKS__) || defined(__SC__)
/* Metrowerks already has a prototype for sprintf() */
#define SPRINTF_PROTO
#endif

#ifndef SPRINTF_PROTO
#if defined(POSIX_TYPES) || defined(DGUX) || defined(NeXT) || !defined(BSD)
extern int sprintf(char *, const char *, ...);
#else
#define OLD_SPRINTF
extern char *sprintf();
#endif
#endif
#ifdef SPRINTF_PROTO
#undef SPRINTF_PROTO
#endif

#ifndef __SASC_60
#ifdef NEED_VARARGS
#if defined(USE_STDARG) || defined(USE_VARARGS)
#if !defined(SVR4) && !defined(apollo)
#if !(defined(ULTRIX_PROTO) && defined(__GNUC__))
#if !(defined(SUNOS4) && defined(__STDC__)) /* Solaris unbundled cc (acc) */
extern int vsprintf(char *, const char *, va_list);
extern int vfprintf(FILE *, const char *, va_list);
extern int vprintf(const char *, va_list);
#endif
#endif
#endif
#else
#ifdef vprintf
#undef vprintf
#endif
#define vprintf printf
#ifdef vfprintf
#undef vfprintf
#endif
#define vfprintf fprintf
#ifdef vsprintf
#undef vsprintf
#endif
#define vsprintf sprintf
#endif
#endif /* NEED_VARARGS */
#endif

#ifdef MICRO
extern int tgetent(const char *, const char *);
extern void tputs(const char *, int, int (*)(void));
extern int tgetnum(const char *);
extern int tgetflag(const char *);
extern char *tgetstr(const char *, char **);
extern char *tgoto(const char *, int, int);
#else
#if !(defined(HPUX) && defined(_POSIX_SOURCE))
extern int tgetent(char *, const char *);
extern void tputs(const char *, int, int (*)(int));
#endif
extern int tgetnum(const char *);
extern int tgetflag(const char *);
extern char *tgetstr(const char *, char **);
extern char *tgoto(const char *, int, int);
#endif

#if defined(ALLOC_C) || defined(MAKEDEFS_C) || defined(MDLIB_C)
extern genericptr_t malloc(size_t);
extern genericptr_t realloc(genericptr_t, size_t);
#endif

/* time functions */

#ifdef NEED_TIME_DECL
extern time_t time(time_t *);
#endif
#ifdef NEED_LOCALTIME_DECL
extern struct tm *localtime(const time_t *);
#endif
#ifdef NEED_CTIME_DECL
extern char *ctime(const time_t *);
#endif

#ifdef MICRO
#ifdef abs
#undef abs
#endif
extern int abs(int);
#ifdef atoi
#undef atoi
#endif
extern int atoi(const char *);
#endif

#endif /*  !__cplusplus && !__GO32__ */
#endif /* WIN32 */

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
#include "nhlua.h"
#endif

#endif /* SYSTEM_H */

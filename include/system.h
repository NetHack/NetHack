/* NetHack 3.7	system.h	$NHDT-Date: 1596498562 2020/08/03 23:49:22 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.24 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SYSTEM_H
#define SYSTEM_H

#if !defined(WIN32)
#if !defined(__cplusplus) && !defined(__GO32__)
#define E extern

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
E long random(void);
#endif
#if (!defined(SUNOS4) && !defined(bsdi) && !defined(__FreeBSD__)) \
    || defined(RANDOM)
E void srandom(unsigned int);
#else
#if !defined(bsdi) && !defined(__FreeBSD__)
E int srandom(unsigned int);
#endif
#endif
#else
#if defined(MACOS)
E long lrand48(void);
E void srand48(long);
#else
extern long lrand48(void);
extern void srand48(long);
#endif /* MACOS */
#endif /* BSD || ULTRIX || RANDOM */

#if !defined(BSD) || defined(ultrix)
/* real BSD wants all these to return int */
#ifndef MICRO
E void exit(int);
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
E void free(genericptr_t);
#endif
#endif
#endif
#if !defined(__SASC_60) && !defined(_DCC) && !defined(__SC__)
#if defined(AMIGA) && !defined(AZTEC_50) && !defined(__GNUC__)
E int perror(const char *);
#else
#if !(defined(ULTRIX_PROTO) && defined(__GNUC__))
E void perror(const char *);
#endif
#endif
#endif
#endif
#ifndef NeXT
#ifdef POSIX_TYPES
E void qsort(genericptr_t, size_t, size_t,
             int (*)(const genericptr, const genericptr));
#else
#if defined(BSD) || defined(ULTRIX)
E int qsort();
#else
#if !defined(LATTICE) && !defined(AZTEC_50)
E void qsort(genericptr_t, size_t, size_t,
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
E int lseek(int, off_t, int);
#else
E long lseek(int, off_t, int);
#endif
/* Ultrix 3.0 man page mistakenly says it returns an int. */
E int write(int, char *, int);
E int link(const char *, const char *);
#else /*!ULTRIX*/
#if !(defined(bsdi) || defined(VMS))
E long lseek(int, long, int);
#if defined(POSIX_TYPES) || defined(__TURBOC__)
E int write(int, const void *, unsigned);
#else
#ifndef __MWERKS__ /* metrowerks defines write via universal headers */
E int write(int, genericptr_t, unsigned);
#endif
#endif /*?(POSIX_TYPES || __TURBOC__)*/
#endif /*!(bsdi || VMS)*/
#endif /*?ULTRIX*/

#ifdef OS2_CSET2 /* IBM CSet/2 */
#ifdef OS2_CSET2_VER_1
E int unlink(char *);
#else
E int unlink(const char *); /* prototype is ok in ver >= 2 */
#endif
#else
#ifndef __SC__
E int unlink(const char *);
#endif
#endif

#endif /* AZTEC_50 && __GNUC__ */

#ifdef MAC
#ifndef __CONDITIONALMACROS__          /* universal headers */
E int close(int);             /* unistd.h */
E int read(int, char *, int); /* unistd.h */
E int chdir(const char *);    /* unistd.h */
E char *getcwd(char *, int);  /* unistd.h */
#endif

E int open(const char *, int);
#endif

#if defined(MICRO)
E int close(int);
#ifndef __EMX__
E int read(int, genericptr_t, unsigned int);
#endif
E int open(const char *, int, ...);
E int dup2(int, int);
E int setmode(int, int);
E int kbhit(void);
#if !defined(_DCC)
#if defined(__TURBOC__)
E int chdir(const char *);
#else
#ifndef __EMX__
E int chdir(char *);
#endif
#endif
#ifndef __EMX__
E char *getcwd(char *, int);
#endif
#endif /* !_DCC */
#endif

#ifdef ULTRIX
E int close(int);
E int atoi(const char *);
E long atol(const char *);
E int chdir(const char *);
#if !defined(ULTRIX_CC20) && !defined(__GNUC__)
E int chmod(const char *, int);
E mode_t umask(int);
#endif
E int read(int, genericptr_t, unsigned);
/* these aren't quite right, but this saves including lots of system files */
E int stty(int, genericptr_t);
E int gtty(int, genericptr_t);
E int ioctl(int, int, char *);
E int isatty(int); /* 1==yes, 0==no, -1==error */
#include <sys/file.h>
#if defined(ULTRIX_PROTO) || defined(__GNUC__)
E int fork(void);
#else
E long fork(void);
#endif
#endif /* ULTRIX */

#ifdef VMS
#ifndef abs
E int abs(int);
#endif
E int atexit(void (*)(void));
E int atoi(const char *);
E long atol(const char *);
E int chdir(const char *);
E int chown(const char *, unsigned, unsigned);
#ifdef __DECC_VER
E int chmod(const char *, mode_t);
E mode_t umask(mode_t);
#else
E int chmod(const char *, int);
E int umask(int);
#endif
/* #include <unixio.h> */
E int close(int);
E int creat(const char *, unsigned, ...);
E int delete(const char *);
E int fstat(/*_ int, stat_t * _*/);
E int isatty(int); /* 1==yes, 0==no, -1==error */
E off_t lseek(int, off_t, int);
E int open(const char *, int, unsigned, ...);
E int read(int, genericptr_t, unsigned);
E int rename(const char *, const char *);
E int stat(/*_ const char *,stat_t * _*/);
E int write(int, const genericptr, unsigned);
#endif

#endif /* __SASC_60 */

/* both old & new versions of Ultrix want these, but real BSD does not */
#ifdef ultrix
E void abort();
E void bcopy();
#ifdef ULTRIX
E int system(const char *);
#ifndef _UNISTD_H_
E int execl(const char *, ...);
#endif
#endif
#endif
#ifdef MICRO
E void abort(void);
E void _exit(int);
E int system(const char *);
#endif
#if defined(HPUX) && !defined(_POSIX_SOURCE)
E long fork(void);
#endif

#ifdef POSIX_TYPES
/* The POSIX string.h is required to define all the mem* and str* functions */
#include <string.h>
#else
#if defined(SYSV) || defined(VMS) || defined(MAC) || defined(SUNOS4)
#if defined(NHSTDC) || (defined(VMS) && !defined(ANCIENT_VAXC))
#if !defined(_AIX32) && !(defined(SUNOS4) && defined(__STDC__))
/* Solaris unbundled cc (acc) */
E int memcmp(const void *, const void *, size_t);
E void *memcpy(void *, const void *, size_t);
E void *memset(void *, int, size_t);
#endif
#else
#ifndef memcmp /* some systems seem to macro these back to b*() */
E int memcmp();
#endif
#ifndef memcpy
E char *memcpy();
#endif
#ifndef memset
E char *memset();
#endif
#endif
#else
#ifdef HPUX
E int memcmp(char *, char *, int);
E void *memcpy(char *, char *, int);
E void *memset(char *, int, int);
#endif
#endif
#endif /* POSIX_TYPES */

#if defined(MICRO) && !defined(LATTICE)
#if defined(TOS) && defined(__GNUC__)
E int memcmp(const void *, const void *, size_t);
E void *memcpy(void *, const void *, size_t);
E void *memset(void *, int, size_t);
#else
#if defined(AZTEC_50) || defined(NHSTDC)
E int memcmp(const void *, const void *, size_t);
E void *memcpy(void *, const void *, size_t);
E void *memset(void *, int, size_t);
#else
E int memcmp(char *, char *, unsigned int);
E char *memcpy(char *, char *, unsigned int);
E char *memset(char *, int, int);
#endif /* AZTEC_50 || NHSTDC */
#endif /* TOS */
#endif /* MICRO */

#if defined(BSD) && defined(ultrix) /* i.e., old versions of Ultrix */
E void sleep();
#endif
#if defined(ULTRIX) || defined(SYSV)
extern unsigned int sleep(unsigned int);
#endif
#if defined(HPUX)
E unsigned int sleep(unsigned int);
#endif
#ifdef VMS
E int sleep(unsigned);
#endif

E char *getenv(const char *);
extern char *getlogin(void);
#if defined(HPUX) && !defined(_POSIX_SOURCE)
E long getuid(void);
E long getgid(void);
E long getpid(void);
#else
#ifdef POSIX_TYPES
E pid_t getpid(void);
E uid_t getuid(void);
E gid_t getgid(void);
#ifdef VMS
E pid_t getppid(void);
#endif
#else          /*!POSIX_TYPES*/
#ifndef getpid /* Borland C defines getpid() as a macro */
E int getpid(void);
#endif
#ifdef VMS
E int getppid(void);
E unsigned getuid(void);
E unsigned getgid(void);
#endif
#if defined(ULTRIX) && !defined(_UNISTD_H_)
E unsigned getuid(void);
E unsigned getgid(void);
E int setgid(int);
E int setuid(int);
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
E char *strcpy(char *, const char *);
E char *strncpy(char *, const char *, size_t);
E char *strcat(char *, const char *);
E char *strncat(char *, const char *, size_t);
E char *strpbrk(const char *, const char *);

#if defined(SYSV) || defined(MICRO) || defined(MAC) || defined(VMS) \
    || defined(HPUX)
E char *strchr(const char *, int);
E char *strrchr(const char *, int);
#else /* BSD */
E char *index(const char *, int);
E char *rindex(const char *, int);
#endif

E int strcmp(const char *, const char *);
E int strncmp(const char *, const char *, size_t);
#if defined(MICRO) || defined(MAC) || defined(VMS)
E size_t strlen(const char *);
#else
#ifdef HPUX
E unsigned int strlen(char *);
#else
#if !(defined(ULTRIX_PROTO) && defined(__GNUC__))
E int strlen(const char *);
#endif
#endif /* HPUX */
#endif /* MICRO */
#endif /* ULTRIX */

#endif /* !_XtIntrinsic_h_ && !POSIX_TYPES */

#if defined(ULTRIX) && defined(__GNUC__)
E char *index(const char *, int);
E char *rindex(const char *, int);
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
E int sprintf(char *, const char *, ...);
#else
#define OLD_SPRINTF
E char *sprintf();
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
E int vsprintf(char *, const char *, va_list);
E int vfprintf(FILE *, const char *, va_list);
E int vprintf(const char *, va_list);
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
E int tgetent(const char *, const char *);
E void tputs(const char *, int, int (*)(void));
E int tgetnum(const char *);
E int tgetflag(const char *);
E char *tgetstr(const char *, char **);
E char *tgoto(const char *, int, int);
#else
#if !(defined(HPUX) && defined(_POSIX_SOURCE))
E int tgetent(char *, const char *);
extern void tputs(const char *, int, int (*)(int));
#endif
E int tgetnum(const char *);
E int tgetflag(const char *);
E char *tgetstr(const char *, char **);
E char *tgoto(const char *, int, int);
#endif

#if defined(ALLOC_C) || defined(MAKEDEFS_C) || defined(MDLIB_C)
E genericptr_t malloc(size_t);
E genericptr_t realloc(genericptr_t, size_t);
#endif

/* time functions */

#ifdef NEED_TIME_DECL
E time_t time(time_t *);
#endif
#ifdef NEED_LOCALTIME_DECL
E struct tm *localtime(const time_t *);
#endif
#ifdef NEED_CTIME_DECL
E char *ctime(const time_t *);
#endif

#ifdef MICRO
#ifdef abs
#undef abs
#endif
E int abs(int);
#ifdef atoi
#undef atoi
#endif
E int atoi(const char *);
#endif

#undef E

#endif /*  !__cplusplus && !__GO32__ */
#endif /* WIN32 */

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
#include "nhlua.h"
#endif

#endif /* SYSTEM_H */

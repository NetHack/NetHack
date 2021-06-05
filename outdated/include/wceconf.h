/* NetHack 3.6	wceconf.h	$NHDT-Date: 1432512776 2015/05/25 00:12:56 $  $NHDT-Branch: master $:$NHDT-Revision: 1.22 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WCECONF_H
#define WCECONF_H

#pragma warning(disable : 4142) /* benign redefinition of type */

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

#include <windows.h>

/* Detect the target device */
#if defined(WIN32_PLATFORM_PSPC)
#if _WIN32_WCE >= 300
#define WIN_CE_POCKETPC
#else
#define WIN_CE_PS2xx
#endif
#elif defined(WIN32_PLATFORM_HPCPRO)
#define WIN_CE_HPCPRO
#elif defined(WIN32_PLATFORM_WFSP)
#define WIN_CE_SMARTPHONE
#else
#error "Unsupported Windows CE platform"
#endif

/* #define SHELL	/* nt use of pcsys routines caused a hang */

#define RANDOM    /* have Berkeley random(3) */
#define TEXTCOLOR /* Color text */

#define EXEPATH              /* Allow .exe location to be used as HACKDIR */
#define TRADITIONAL_GLYPHMAP /* Store glyph mappings at level change time */

#define PC_LOCKING /* Prevent overwrites of aborted or in-progress games */
/* without first receiving confirmation. */

#define SELF_RECOVER /* Allow the game itself to recover from an aborted \
                        game */

#define NOTSTDC /* no strerror() */

#define USER_SOUNDS

/*
 * -----------------------------------------------------------------
 *  The remaining code shouldn't need modification.
 * -----------------------------------------------------------------
 */
/* #define SHORT_FILENAMES	/* All NT filesystems support long names now
 */

#ifdef MICRO
#undef MICRO /* never define this! */
#endif

#define NOCWD_ASSUMPTIONS /* Always define this. There are assumptions that \
                             it is defined for WIN32.                       \
                             Allow paths to be specified for HACKDIR,       \
                             LEVELDIR, SAVEDIR, BONESDIR, DATADIR,          \
                             SCOREDIR, LOCKDIR, CONFIGDIR, and TROUBLEDIR */
#define NO_TERMS
#define ASCIIGRAPH

#ifdef OPTIONS_USED
#undef OPTIONS_USED
#endif
#ifdef MSWIN_GRAPHICS
#define OPTIONS_USED "guioptions"
#else
#define OPTIONS_USED "ttyoptions"
#endif
#define OPTIONS_FILE OPTIONS_USED

#define PORT_HELP "porthelp"

#define SAFERHANGUP /* Define SAFERHANGUP to delay hangup processing   \
                     * until the main command loop. 'safer' because it \
                     * avoids certain cheats and also avoids losing    \
                     * objects being thrown when the hangup occurs.    \
                     */

#if defined(WIN_CE_POCKETPC)
#define PORT_CE_PLATFORM "Pocket PC"
#elif defined(WIN_CE_PS2xx)
#define PORT_CE_PLATFORM "Palm-size PC 2.11"
#elif defined(WIN_CE_HPCPRO)
#define PORT_CE_PLATFORM "H/PC Pro 2.11"
#elif defined(WIN_CE_SMARTPHONE)
#define PORT_CE_PLATFORM "Smartphone 2002"
#endif

#if defined(ARM)
#define PORT_CE_CPU "ARM"
#elif defined(PPC)
#define PORT_CE_CPU "PPC"
#elif defined(ALPHA)
#define PORT_CE_CPU "ALPHA"
#elif defined(SH3)
#define PORT_CE_CPU "SH3"
#elif defined(SH4)
#define PORT_CE_CPU "SH4"
#elif defined(MIPS)
#define PORT_CE_CPU "MIPS"
#elif defined(X86) || defined(_X86_)
#define PORT_CE_CPU "X86"
#else
#error Only ARM, PPC, ALPHA, SH3, SH4, MIPS and X86 supported
#endif

#define RUNTIME_PORT_ID /* trigger run-time port identification since \
                           Makedefs is bootstrapped on a cross-platform. */

#include <string.h> /* Provides prototypes of strncmpi(), etc.     */
#ifdef STRNCMPI
#define strncmpi(a, b, c) _strnicmp(a, b, c)
#endif

#ifdef STRCMPI
#define strcmpi(a, b) _stricmp(a, b)
#define stricmp(a, b) _stricmp(a, b)
#endif

#include <stdlib.h>

#define PATHLEN BUFSZ  /* maximum pathlength */
#define FILENAME BUFSZ /* maximum filename length (conservative) */

#if defined(_MAX_PATH) && defined(_MAX_FNAME)
#if (_MAX_PATH < BUFSZ) && (_MAX_FNAME < BUFSZ)
#undef PATHLEN
#undef FILENAME
#define PATHLEN _MAX_PATH
#define FILENAME _MAX_FNAME
#endif
#endif

#define NO_SIGNAL
#define index strchr
#define rindex strrchr
#define USE_STDARG

/* Use the high quality random number routines. */
#ifndef USE_ISAAC64
# ifdef RANDOM
#  define Rand() random()
# else
#  define Rand() rand()
# endif
#endif

#define FCMASK 0660 /* file creation mask */
#define regularize nt_regularize
#define HLOCK "NHPERM"

#ifndef M
#define M(c) ((char) (0x80 | (c)))
/* #define M(c)		((c) - 128) */
#endif

#ifndef C
#define C(c) (0x1f & (c))
#endif

#if defined(DLB)
#define FILENAME_CMP _stricmp /* case insensitive */
#endif

#if 0
extern char levels[], bones[], permbones[],
#endif /* 0 */

/* this was part of the MICRO stuff in the past */
extern const char *alllevels, *allbones;
extern char hackdir[];
#define ABORT C('a')
#define getuid() 1
#define getlogin() ((char *) 0)
extern void win32_abort(void);
#ifdef WIN32CON
extern void consoletty_preference_update(const char *);
extern void toggle_mouse_support(void);
#endif

#ifndef alloca
#define ALLOCA_HACK /* used in util/panic.c */
#endif

#ifdef _MSC_VER
#if 0
#pragma warning(disable : 4018) /* signed/unsigned mismatch */
#pragma warning(disable : 4305) /* init, conv from 'const int' to 'char' */
#endif
#pragma warning(disable : 4761) /* integral size mismatch in arg; conv \
                                   supp*/
#ifdef YYPREFIX
#pragma warning(disable : 4102) /* unreferenced label */
#endif
#endif

/* UNICODE stuff */
#define NHSTR_BUFSIZE 255
#ifdef UNICODE
#define NH_W2A(w, a, cb) \
    (WideCharToMultiByte(CP_ACP, 0, (w), -1, (a), (cb), NULL, NULL), (a))

#define NH_A2W(a, w, cb) \
    (MultiByteToWideChar(CP_ACP, 0, (a), -1, (w), (cb)), (w))
#else
#define NH_W2A(w, a, cb) (strncpy((a), (w), (cb)))

#define NH_A2W(a, w, cb) (strncpy((w), (a), (cb)))
#endif

extern int set_win32_option(const char *, const char *);

/*
 * 3.4.3 addition - Stuff to help the user with some common, yet significant
 * errors
 * Let's make it NOP for now
 */
#define interject_assistance(_1, _2, _3, _4)
#define interject(_1)

/* Missing definitions */
extern int mswin_have_input();
#define kbhit mswin_have_input

#define getenv(a) ((char *) NULL)

/* __stdio.h__ */
#define perror(a)
#define freopen(a, b, c) fopen(a, b)
extern int isatty(int);

/* __time.h___ */
#ifndef _TIME_T_DEFINED
typedef __int64 time_t; /* time value */
#define _TIME_T_DEFINED /* avoid multiple def's of time_t */
#endif

#ifndef _TM_DEFINED
struct tm {
    int tm_sec;   /* seconds after the minute - [0,59] */
    int tm_min;   /* minutes after the hour - [0,59] */
    int tm_hour;  /* hours since midnight - [0,23] */
    int tm_mday;  /* day of the month - [1,31] */
    int tm_mon;   /* months since January - [0,11] */
    int tm_year;  /* years since 1900 */
    int tm_wday;  /* days since Sunday - [0,6] */
    int tm_yday;  /* days since January 1 - [0,365] */
    int tm_isdst; /* daylight savings time flag - - NOT IMPLEMENTED */
};
#define _TM_DEFINED
#endif

extern struct tm *__cdecl localtime(const time_t *);
extern time_t __cdecl time(time_t *);
extern time_t __cdecl mktime(struct tm *tb);

/* __stdio.h__ */
#ifndef BUFSIZ
#define BUFSIZ 255
#endif

#define rewind(stream) (void) fseek(stream, 0L, SEEK_SET)

/* __io.h__ */
typedef long off_t;

extern int __cdecl close(int);
extern int __cdecl creat(const char *, int);
extern int __cdecl eof(int);
extern long __cdecl lseek(int, long, int);
extern int __cdecl open(const char *, int, ...);
extern int __cdecl read(int, void *, unsigned int);
extern int __cdecl unlink(const char *);
extern int __cdecl write(int, const void *, unsigned int);
extern int __cdecl rename(const char *, const char *);
extern int __cdecl access(const char *, int);

#ifdef DeleteFile
#undef DeleteFile
#endif
#define DeleteFile(a) unlink(a)

int chdir(const char *dirname);
extern char *getcwd(char *buffer, int maxlen);

/* __stdlib.h__ */
#define abort() (void) TerminateProcess(GetCurrentProcess(), 0)
#ifndef strdup
#define strdup _strdup
#endif

/* sys/stat.h */
#define S_IWRITE GENERIC_WRITE
#define S_IREAD GENERIC_READ

/* CE 2.xx is missing even more stuff */
#if defined(WIN_CE_PS2xx) || defined(WIN32_PLATFORM_HPCPRO)
#define ZeroMemory(p, s) memset((p), 0, (s))

extern int __cdecl isupper(int c);
extern int __cdecl isdigit(int c);
extern int __cdecl isspace(int c);
extern int __cdecl isprint(int c);

extern char *__cdecl _strdup(const char *s);
extern char *__cdecl strrchr(const char *string, int c);
extern int __cdecl _stricmp(const char *a, const char *b);

extern FILE *__cdecl fopen(const char *filename, const char *mode);
extern int __cdecl fscanf(FILE *f, const char *format, ...);
extern int __cdecl fprintf(FILE *f, const char *format, ...);
extern int __cdecl vfprintf(FILE *f, const char *format, va_list args);
extern int __cdecl fgetc(FILE *f);
extern char *__cdecl fgets(char *s, int size, FILE *f);
extern int __cdecl printf(const char *format, ...);
extern int __cdecl vprintf(const char *format, va_list args);
extern int __cdecl puts(const char *s);
extern FILE *__cdecl _getstdfilex(int desc);
extern int __cdecl fclose(FILE *f);
extern size_t __cdecl fread(void *p, size_t size, size_t count, FILE *f);
extern size_t __cdecl fwrite(const void *p, size_t size, size_t count,
                             FILE *f);
extern int __cdecl fflush(FILE *f);
extern int __cdecl feof(FILE *f);
extern int __cdecl fseek(FILE *f, long offset, int from);
extern long __cdecl ftell(FILE *f);

#endif

/* ARM - the processor; avoids conflict with ARM in hack.h */
#ifdef ARM
#undef ARM
#endif

/* leave - Windows CE defines leave as part of exception handling (__leave)
   It conflicts with existing sources and since we don't use exceptions it is
   safe
   to undefine it */
#ifdef leave
#undef leave
#endif

#endif /* WCECONF_H */

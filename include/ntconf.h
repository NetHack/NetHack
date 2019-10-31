/* NetHack 3.6	ntconf.h	$NHDT-Date: 1447424077 2015/11/13 14:14:37 $  $NHDT-Branch: master $:$NHDT-Revision: 1.48 $ */
/* Copyright (c) NetHack PC Development Team 1993, 1994.  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef NTCONF_H
#define NTCONF_H

/* #define SHELL */	/* nt use of pcsys routines caused a hang */

#define TEXTCOLOR /* Color text */

#define EXEPATH              /* Allow .exe location to be used as HACKDIR */
#define TRADITIONAL_GLYPHMAP /* Store glyph mappings at level change time */

#define LAN_FEATURES /* Include code for lan-aware features. Untested in \
                        3.4.0*/

#define PC_LOCKING /* Prevent overwrites of aborted or in-progress games */
/* without first receiving confirmation. */

#define HOLD_LOCKFILE_OPEN /* Keep an exclusive lock on the .0 file */

#define SELF_RECOVER /* Allow the game itself to recover from an aborted \
                        game */

#define SYSCF                /* Use a global configuration */
#define SYSCF_FILE "sysconf" /* Use a file to hold the SYSCF configuration */

#define DUMPLOG      /* Enable dumplog files */
/*#define DUMPLOG_FILE "nethack-%n-%d.log"*/
#define DUMPLOG_MSG_COUNT 50

#define USER_SOUNDS

/*#define CHANGE_COLOR*/ /* allow palette changes */
#define SELECTSAVED /* Provide menu of saved games to choose from at start */
 
/*
 * -----------------------------------------------------------------
 *  The remaining code shouldn't need modification.
 * -----------------------------------------------------------------
 */
/* #define SHORT_FILENAMES */ /* All NT filesystems support long names now
 */

#define VERSION_IN_DLB_FILENAME     /* Append version digits to nhdat */

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
#define OPTIONS_USED "options"
#define OPTIONS_FILE OPTIONS_USED

#define PORT_HELP "porthelp"

#define PORT_DEBUG /* include ability to debug international keyboard issues \
                      */

#define RUNTIME_PORT_ID /* trigger run-time port identification for \
                         * identification of exe CPU architecture   \
                         */
#define RUNTIME_PASTEBUF_SUPPORT


#define SAFERHANGUP /* Define SAFERHANGUP to delay hangup processing   \
                     * until the main command loop. 'safer' because it \
                     * avoids certain cheats and also avoids losing    \
                     * objects being thrown when the hangup occurs.    \
                     */

#define CONFIG_FILE "defaults.nh"
#define CONFIG_TEMPLATE "defaults.template"
#define SYSCF_TEMPLATE "sysconf.template"
#define SYMBOLS_TEMPLATE "symbols.template"
#define GUIDEBOOK_FILE "Guidebook.txt"

/* Stuff to help the user with some common, yet significant errors */
#define INTERJECT_PANIC 0
#define INTERJECTION_TYPES (INTERJECT_PANIC + 1)
extern void FDECL(interject_assistance,
                  (int, int, genericptr_t, genericptr_t));
extern void FDECL(interject, (int));

/*
 *===============================================
 * Compiler-specific adjustments
 *===============================================
 */

#ifdef __MINGW32__
#ifdef strncasecmp
#undef strncasecmp
#endif
#ifdef strcasecmp
#undef strcasecmp
#endif
extern void NDECL(getlock);
#endif
 
#ifdef _MSC_VER
#if (_MSC_VER > 1000)
/* Visual C 8 warning elimination */
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#ifndef _SCL_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#pragma warning(disable : 4996) /* VC8 deprecation warnings */
#pragma warning(disable : 4142) /* benign redefinition */
#pragma warning(disable : 4267) /* conversion from 'size_t' to XX */
#if (_MSC_VER > 1600)
#pragma warning(disable : 4459) /* hide global declaration */
#endif                          /* _MSC_VER > 1600 */
#endif                          /* _MSC_VER > 1000 */
#pragma warning(disable : 4761) /* integral size mismatch in arg; conv \
                                   supp*/
#ifdef YYPREFIX
#pragma warning(disable : 4102) /* unreferenced label */
#endif
#ifdef __cplusplus
/* suppress a warning in cppregex.cpp */
#pragma warning(disable : 4101) /* unreferenced local variable */
#endif
#ifndef HAS_STDINT_H
#define HAS_STDINT_H    /* force include of stdint.h in integer.h */
#endif
#endif /* _MSC_VER */

/* The following is needed for prototypes of certain functions */
#if defined(_MSC_VER)
#include <process.h> /* Provides prototypes of exit(), spawn()      */
#endif

#include <string.h> /* Provides prototypes of strncmpi(), etc.     */
#ifdef STRNCMPI
#define strncmpi(a, b, c) strnicmp(a, b, c)
#endif

#ifdef _MSC_VER
/* Visual Studio defines this in their own headers, which we don't use */
#ifndef snprintf
#define snprintf _snprintf
#pragma warning( \
    disable : 4996) /* deprecation warning suggesting snprintf_s */
#endif
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef __BORLANDC__
#undef randomize
#undef random
#endif

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

/* Time stuff */
#include <time.h>

#define USE_STDARG

/* Use the high quality random number routines. */
#ifdef USE_ISAAC64
#undef RANDOM
#else
#define RANDOM
#define Rand() random()
#endif

/* Fall back to C's if nothing else, but this really isn't acceptable */
#if !defined(USE_ISAAC64) && !defined(RANDOM)
#define Rand() rand()
#endif

#include <sys/stat.h>
#define FCMASK (_S_IREAD | _S_IWRITE) /* file creation mask */
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
#define FILENAME_CMP stricmp /* case insensitive */
#endif

/* this was part of the MICRO stuff in the past */
extern const char *alllevels, *allbones;
extern char hackdir[];
#define ABORT C('a')
#define getuid() 1
#define getlogin() ((char *) 0)
extern void NDECL(win32_abort);
extern void FDECL(nttty_preference_update, (const char *));
extern void NDECL(toggle_mouse_support);
extern void FDECL(map_subkeyvalue, (char *));
#if defined(WIN32CON)
extern void FDECL(set_altkeyhandler, (const char *));
#endif
extern void NDECL(raw_clear_screen);

#include <fcntl.h>
#ifndef __BORLANDC__
#include <io.h>
#include <direct.h>
#else
int _RTLENTRY _EXPFUNC access(const char _FAR *__path, int __amode);
int _RTLENTRY _EXPFUNC _chdrive(int __drive);
int _RTLENTRYF _EXPFUNC32 chdir(const char _FAR *__path);
char _FAR *_RTLENTRY _EXPFUNC getcwd(char _FAR *__buf, int __buflen);
int _RTLENTRY _EXPFUNC
write(int __handle, const void _FAR *__buf, unsigned __len);
int _RTLENTRY _EXPFUNC creat(const char _FAR *__path, int __amode);
int _RTLENTRY _EXPFUNC close(int __handle);
int _RTLENTRY _EXPFUNC _close(int __handle);
int _RTLENTRY _EXPFUNC
open(const char _FAR *__path, int __access, ... /*unsigned mode*/);
long _RTLENTRY _EXPFUNC lseek(int __handle, long __offset, int __fromwhere);
int _RTLENTRY _EXPFUNC read(int __handle, void _FAR *__buf, unsigned __len);
#endif
#ifndef CURSES_GRAPHICS
#include <conio.h>      /* conflicting definitions with curses.h */
#endif
#undef kbhit /* Use our special NT kbhit */
#define kbhit (*nt_kbhit)

#ifdef LAN_FEATURES
#define MAX_LAN_USERNAME 20
#endif

#ifndef alloca
#define ALLOCA_HACK /* used in util/panic.c */
#endif

extern int FDECL(set_win32_option, (const char *, const char *));
#define LEFTBUTTON FROM_LEFT_1ST_BUTTON_PRESSED
#define RIGHTBUTTON RIGHTMOST_BUTTON_PRESSED
#define MIDBUTTON FROM_LEFT_2ND_BUTTON_PRESSED
#define MOUSEMASK (LEFTBUTTON | RIGHTBUTTON | MIDBUTTON)
#ifdef CHANGE_COLOR
extern int FDECL(alternative_palette, (char *));
#endif

#ifdef NDEBUG
#define nhassert(expression) ((void)0)
#else
extern void FDECL(nhassert_failed, (const char * exp, const char * file,
                                    int line));

#define nhassert(expression) (void)((!!(expression)) || \
        (nhassert_failed(#expression, __FILE__, __LINE__), 0))
#endif

#define nethack_enter(argc, argv) nethack_enter_winnt()
extern void FDECL(nethack_exit, (int)) NORETURN;
extern boolean FDECL(file_exists, (const char *));
extern boolean FDECL(file_newer, (const char *, const char *));
#endif /* NTCONF_H */

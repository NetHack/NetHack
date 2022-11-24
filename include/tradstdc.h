/* NetHack 3.7	tradstdc.h	$NHDT-Date: 1596498565 2020/08/03 23:49:25 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.37 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef TRADSTDC_H
#define TRADSTDC_H

#if defined(DUMB) && !defined(NOVOID)
#define NOVOID
#endif

#ifdef NOVOID
#define void int
#endif

/*
 * Borland C provides enough ANSI C compatibility in its Borland C++
 * mode to warrant this.  But it does not set __STDC__ unless it compiles
 * in its ANSI keywords only mode, which prevents use of <dos.h> and
 * far pointer use.
 */
#if (defined(__STDC__) || defined(__TURBOC__)) && !defined(NOTSTDC)
#define NHSTDC
#endif

#if defined(ultrix) && defined(__STDC__) && !defined(__LANGUAGE_C)
/* Ultrix seems to be in a constant state of flux.  This check attempts to
 * set up ansi compatibility if it wasn't set up correctly by the compiler.
 */
#ifdef mips
#define __mips mips
#endif
#ifdef LANGUAGE_C
#define __LANGUAGE_C LANGUAGE_C
#endif
#endif

/*
 * ANSI X3J11 detection.
 * Makes substitutes for compatibility with the old C standard.
 */

/* Decide how to handle variable parameter lists:
 * USE_STDARG means use the ANSI <stdarg.h> facilities (only ANSI compilers
 * should do this, and only if the library supports it).
 * USE_VARARGS means use the <varargs.h> facilities.  Again, this should only
 * be done if the library supports it.  ANSI is *not* required for this.
 * Otherwise, the kludgy old methods are used.
 */

/* #define USE_VARARGS */ /* use <varargs.h> instead of <stdarg.h> */
/* #define USE_OLDARGS */ /* don't use any variable argument facilities */

#if defined(apollo) /* Apollos have stdarg(3) but not stdarg.h */
#define USE_VARARGS
#endif

#if !defined(USE_STDARG) && !defined(USE_VARARGS) && !defined(USE_OLDARGS)
/* the old VARARGS and OLDARGS stuff is still here, but since we're
   requiring C99 these days it's unlikely to be useful */
#define USE_STDARG
#endif

#ifdef NEED_VARARGS /* only define these if necessary */
/*
 * These changed in 3.6.0.  VA_END() provides a hidden
 * closing brace to complement VA_DECL()'s hidden opening brace, so code
 * started with VA_DECL() needs an extra opening brace to complement
 * the explicit final closing brace.  This was done so that the source
 * would look less strange, where VA_DECL() appeared to introduce a
 * function whose opening brace was missing; there are now visible and
 * invisible braces at beginning and end.  Sample usage:
 void foo VA_DECL(int, arg)  --macro expansion has a hidden opening brace
 {  --explicit opening brace (actually introduces a nested block)
 VA_START(bar);
 ...code for foo...
 VA_END();  --expansion provides a closing brace for the nested block
 }  --closing brace, pairs with the hidden one in VA_DECL()
 * Reading the code--or using source browsing tools which match braces--
 * results in seeing a matched set of braces.  Usage of VA_END() is
 * potentially trickier, but nethack uses it in a straightforward manner.
 */

#ifdef USE_STDARG
#include <stdarg.h>
#define VA_DECL(typ1, var1) \
    (typ1 var1, ...)        \
    {                       \
        va_list the_args;
#define VA_DECL2(typ1, var1, typ2, var2) \
    (typ1 var1, typ2 var2, ...)          \
    {                                    \
        va_list the_args;
#define VA_INIT(var1, typ1)
#define VA_NEXT(var1, typ1) (var1 = va_arg(the_args, typ1))
#define VA_ARGS the_args
#define VA_START(x) va_start(the_args, x)
#define VA_END()      \
    va_end(the_args); \
    }
#define VA_PASS1(a1) a1
#if defined(ULTRIX_PROTO) && !defined(_VA_LIST_)
#define _VA_LIST_ /* prevents multiple def in stdio.h */
#endif
#else

#ifdef USE_VARARGS
#include <varargs.h>
#define VA_DECL(typ1, var1) \
    (va_alist) va_dcl       \
    {                       \
        va_list the_args;   \
        typ1 var1;
#define VA_DECL2(typ1, var1, typ2, var2) \
    (va_alist) va_dcl                    \
    {                                    \
        va_list the_args;                \
        typ1 var1;                       \
        typ2 var2;
#define VA_ARGS the_args
#define VA_START(x) va_start(the_args)
#define VA_INIT(var1, typ1) var1 = va_arg(the_args, typ1)
#define VA_NEXT(var1, typ1) (var1 = va_arg(the_args, typ1))
#define VA_END()      \
    va_end(the_args); \
    }
#define VA_PASS1(a1) a1
#else

/*USE_OLDARGS*/
/*
 * CAVEAT:  passing double (including float promoted to double) will
 * almost certainly break this, as would any integer type bigger than
 * sizeof (char *).
 * NetHack avoids floating point, and any configuration able to use
 * 'long long int' or I64P32 or the like should be using USE_STDARG.
 */
#ifndef VA_TYPE
typedef const char *vA;
#define VA_TYPE
#endif
#define VA_ARGS arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9
#define VA_DECL(typ1, var1)                                             \
    (var1, VA_ARGS) typ1 var1; vA VA_ARGS;                              \
    {
#define VA_DECL2(typ1, var1, typ2, var2)                                \
    (var1, var2, VA_ARGS) typ1 var1; typ2 var2; vA VA_ARGS;             \
    {
#define VA_START(x)
#define VA_INIT(var1, typ1)
/* This is inherently risky, and should only be attempted as a
   very last resort; manipulating arguments which haven't actually
   been passed may or may not cause severe trouble depending on
   the function-calling/argument-passing mechanism being used.

   [nethack's core doesn't use VA_NEXT() so doesn't use VA_SHIFT()
   either, and this definition is just retained for completeness.
   lev_comp does use VA_NEXT(), but it passes all 'argX' arguments.
   Note: as of 3.7.0, lev_comp doesn't exist anymore.]
 */
#define VA_SHIFT()                                                    \
    (arg1 = arg2, arg2 = arg3, arg3 = arg4, arg4 = arg5, arg5 = arg6, \
     arg6 = arg7, arg7 = arg8, arg8 = arg9, arg9 = 0)
#define VA_NEXT(var1, typ1) ((var1 = (typ1) arg1), VA_SHIFT(), var1)
#define VA_END() }
/* needed in pline.c, where full number of arguments is known and expected */
#define VA_PASS1(a1)                                                  \
    (vA) a1, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0
#endif
#endif

#endif /* NEED_VARARGS */

#if defined(NHSTDC) || defined(MSDOS) || defined(MAC) \
    || defined(ULTRIX_PROTO) || defined(__BEOS__)

/*
 * Used for robust ANSI parameter forward declarations:
 * int VDECL(sprintf, (char *, const char *, ...));
 *
 * VDECL() is used for functions with a variable number of arguments.
 * Separate macros are needed because ANSI will mix old-style declarations
 * with prototypes, except in the case of varargs
  */

#if defined(MSDOS) || defined(USE_STDARG)
#define VDECL(f, p) f p
#else
#define VDECL(f, p) f()
#endif

/* generic pointer, always a macro; genericptr_t is usually a typedef */
#define genericptr void *

#if (defined(ULTRIX_PROTO) && !defined(__GNUC__)) || defined(OS2_CSET2)
/* Cover for Ultrix on a DECstation with 2.0 compiler, which coredumps on
 *   typedef void * genericptr_t;
 *   extern void a(void(*)(int, genericptr_t));
 * Using the #define is OK for other compiler versions too.
 */
/* And IBM CSet/2.  The redeclaration of free hoses the compile. */
#define genericptr_t genericptr
#else
#if !defined(NHSTDC) && !defined(MAC)
#define const
#define signed
#define volatile
#endif
#endif

/*
 * Suppress `const' if necessary and not handled elsewhere.
 * Don't use `#if defined(xxx) && !defined(const)'
 * because some compilers choke on `defined(const)'.
 * This has been observed with Lattice, MPW, and High C.
 */
#if (defined(ULTRIX_PROTO) && !defined(NHSTDC)) || defined(apollo)
/* the system header files don't use `const' properly */
#ifndef const
#define const
#endif
#endif

#else /* NHSTDC */ /* a "traditional" C  compiler */

#define VDECL(f, p) f()

#if defined(AMIGA) || defined(HPUX) || defined(POSIX_TYPES) \
    || defined(__DECC) || defined(__BORLANDC__)
#define genericptr void *
#endif
#ifndef genericptr
#define genericptr char *
#endif

/*
 * Traditional C compilers don't have "signed", "const", or "volatile".
 */
#define signed
#define const
#define volatile

#endif /* NHSTDC */

#ifndef genericptr_t
typedef genericptr genericptr_t; /* (void *) or (char *) */
#endif

#if defined(MICRO) || defined(WIN32)
/* We actually want to know which systems have an ANSI run-time library
 * to know which support the %p format for printing pointers.
 * Due to the presence of things like gcc, NHSTDC is not a good test.
 * So we assume microcomputers have all converted to ANSI and bigger
 * computers which may have older libraries give reasonable results with
 * casting pointers to unsigned long int (fmt_ptr() in alloc.c).
 */
#define HAS_PTR_FMT
#endif

/*
 * According to ANSI C, prototypes for old-style function definitions like
 *   int func(arg) short arg; { ... }
 * must specify widened arguments (char and short to int, float to double),
 *   int func(int);
 * same as how narrow arguments get passed when there is no prototype info.
 * However, various compilers accept shorter arguments (char, short, etc.)
 * in prototypes and do typechecking with them.  Therefore this mess to
 * allow the better typechecking while also allowing some prototypes for
 * the ANSI compilers so people quit trying to fix the prototypes to match
 * the standard and thus lose the typechecking.
 */
#if defined(MSDOS) && !defined(__GO32__)
#define UNWIDENED_PROTOTYPES
#endif
#if defined(AMIGA) && !defined(AZTEC_50)
#define UNWIDENED_PROTOTYPES
#endif
#if defined(macintosh) && (defined(__SC__) || defined(__MRC__))
#define WIDENED_PROTOTYPES
#endif
#if defined(__MWERKS__) && defined(__BEOS__)
#define UNWIDENED_PROTOTYPES
#endif
#if defined(WIN32)
#define UNWIDENED_PROTOTYPES
#endif

#if defined(ULTRIX_PROTO) && defined(ULTRIX_CC20)
#define UNWIDENED_PROTOTYPES
#endif
#if defined(apollo)
#define UNWIDENED_PROTOTYPES
#endif

#ifndef UNWIDENED_PROTOTYPES
#if defined(NHSTDC) || defined(ULTRIX_PROTO) || defined(THINK_C)
#ifndef WIDENED_PROTOTYPES
#define WIDENED_PROTOTYPES
#endif
#endif
#endif

/* this applies to both VMS and Digital Unix/HP Tru64 */
#ifdef WIDENED_PROTOTYPES
/* ANSI C uses "value preserving rules", where 'unsigned char' and
   'unsigned short' promote to 'int' if signed int is big enough to hold
   all possible values, rather than traditional "sign preserving rules"
   where 'unsigned char' and 'unsigned short' promote to 'unsigned int'.
   However, the ANSI C rules aren't binding on non-ANSI compilers.
   When DEC C (aka Compaq C, then HP C) is in non-standard 'common' mode
   it supports prototypes that expect widened types, but it uses the old
   sign preserving rules for how to widen narrow unsigned types.  (In its
   default 'relaxed' mode, __STDC__ is 1 and uchar widens to 'int'.) */
#if defined(__DECC) && (!defined(__STDC__) || !__STDC__)
#define UCHAR_P unsigned int
#endif
#endif

/* These are used for arguments within VDECL prototype declarations.
 */
#ifdef UNWIDENED_PROTOTYPES
#define CHAR_P char
#define SCHAR_P schar
#define UCHAR_P uchar
#define XCHAR_P coordxy
#define SHORT_P short
#ifndef SKIP_BOOLEAN
#define BOOLEAN_P boolean
#endif
#define ALIGNTYP_P aligntyp
#else
#ifdef WIDENED_PROTOTYPES
#define CHAR_P int
#define SCHAR_P int
#ifndef UCHAR_P
#define UCHAR_P int
#endif
#define XCHAR_P int
#define SHORT_P int
#define BOOLEAN_P int
#define ALIGNTYP_P int
#else
/* Neither widened nor unwidened prototypes.  Argument list expansion
 * by VDECL always empty; all xxx_P vanish so defs aren't needed. */
#endif
#endif

/* OBJ_P and MONST_P should _only_ be used for declaring function pointers.
 */
#if defined(ULTRIX_PROTO) && !defined(__STDC__)
/* The ultrix 2.0 and 2.1 compilers (on Ultrix 4.0 and 4.2 respectively) can't
 * handle "struct obj *" constructs in prototypes.  Their bugs are different,
 * but both seem to work if we put "void*" in the prototype instead.  This
 * gives us minimal prototype checking but avoids the compiler bugs.
 */
#define OBJ_P void *
#define MONST_P void *
#else
#define OBJ_P struct obj *
#define MONST_P struct monst *
#endif

#if 0
/* The problem below is still the case through 4.0.5F, but the suggested
 * compiler flags in the Makefiles suppress the nasty messages, so we don't
 * need to be quite so drastic.
 */
#if defined(__sgi) && !defined(__GNUC__)
/*
 * As of IRIX 4.0.1, /bin/cc claims to be an ANSI compiler, but it thinks
 * it's impossible for a prototype to match an old-style definition with
 * unwidened argument types.  Thus, we have to turn off all NetHack
 * prototypes, and avoid declaring several system functions, since the system
 * include files have prototypes and the compiler also complains that
 * prototyped and unprototyped declarations don't match.
 */
#undef VDECL
#define VDECL(f, p) f()
#endif
#endif

/* MetaWare High-C defaults to unsigned chars */
/* AIX 3.2 needs this also */
#if defined(__HC__) || defined(_AIX32)
#undef signed
#endif

#ifdef __clang__
/* clang's gcc emulation is sufficient for nethack's usage */
#ifndef __GNUC__
#define __GNUC__ 5 /* high enough for returns_nonnull */
#endif
#endif

/*
 * Give first priority to standard
 */
#ifndef ATTRNORETURN
#if defined(__STDC_VERSION__) || defined(__cplusplus)
#if (__STDC_VERSION__ > 202300L) || defined(__cplusplus)
#define ATTRNORETURN [[noreturn]]
#endif
#endif
#endif

/*
 * Allow gcc2 to check parameters of printf-like calls with -Wformat;
 * append this to a prototype declaration (see pline() in extern.h).
 */
#ifdef __GNUC__
#if (__GNUC__ >= 2) && !defined(USE_OLDARGS)
#define PRINTF_F(f, v) __attribute__((format(printf, f, v)))
#endif
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define PRINTF_F_PTR(f, v) PRINTF_F(f, v)
#endif
#if __GNUC__ >= 3
#define UNUSED __attribute__((unused))
#ifndef ATTRNORETURN
#ifndef NORETURN
#define NORETURN __attribute__((noreturn))
#endif
#endif
#if (!defined(__linux__) && !defined(MACOS)) || defined(GCC_URWARN)
/* disable gcc's __attribute__((__warn_unused_result__)) since explicitly
   discarding the result by casting to (void) is not accepted as a 'use' */
#define __warn_unused_result__ /*empty*/
#define warn_unused_result /*empty*/
#endif
#endif
#if __GNUC__ >= 5
#define NONNULL __attribute__((returns_nonnull))
#endif
#endif

#ifdef _MSC_VER
#ifndef ATTRNORETURN
#define ATTRNORETURN __declspec(noreturn)
#endif
#endif

#ifndef PRINTF_F
#define PRINTF_F(f, v)
#endif
#ifndef PRINTF_F_PTR
#define PRINTF_F_PTR(f, v)
#endif
#ifndef UNUSED
#define UNUSED
#endif
#ifndef ATTRNORETURN
#define ATTRNORETURN
#endif
#ifndef NORETURN
#define NORETURN
#endif
#ifndef NONNULL
#define NONNULL
#endif


#endif /* TRADSTDC_H */

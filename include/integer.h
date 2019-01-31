/* NetHack 3.6	integer.h	$NHDT-Date: 1524689514 2018/04/25 20:51:54 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.0 $ */
/*      Copyright (c) 2016 by Michael Allison          */
/* NetHack may be freely redistributed.  See license for details. */

/* integer.h -- provide sized integer types
 *
 * We try to sort out a way to provide sized integer types
 * in here. The strong preference is to try and let a
 * compiler-supplied header file set up the types.
 *
 * If your compiler is C99 conforming and sets a value of
 * __STDC_VERSION__ >= 199901L, then <stdint.h> is supposed
 * to be available for inclusion.
 *
 * If your compiler doesn't set __STDC_VERSION__ to indicate
 * full conformance to C99, but does actually supply a suitable
 * <stdint.h>, you can pass a compiler flag -DHAS_STDINT_H
 * during build to cause the inclusion of <stdint.h> anyway.
 *
 * If <stdint.h> doesn't get included, then the code in the
 * STDINT_WORKAROUND section of code is not skipped and will
 * be used to set up the types.
 *
 * We acknowledge that some ongoing maintenance may be needed
 * over time if people send us code updates for making the
 * determination of whether <stdint.h> is available, or
 * require adjustments to the base type used for some
 * compiler/platform combinations.
 *
 */

#ifndef INTEGER_H
#define INTEGER_H

#if (defined(__STDC__) && __STDC_VERSION__ >= 199901L)
/* The compiler claims to conform to C99. Use stdint.h */
#include <stdint.h>
#define SKIP_STDINT_WORKAROUND
#else
# if defined(HAS_STDINT_H)
/* Some compilers have stdint.h but don't conform to all of C99. */
#include <stdint.h>
#define SKIP_STDINT_WORKAROUND
# endif
# if defined(__GNUC__) && defined(__INT64_MAX__)
#  include <stdint.h>
#  define SKIP_STDINT_WORKAROUND
# endif
#endif

#ifndef SKIP_STDINT_WORKAROUND /* !C99 */
/*
 * STDINT_WORKAROUND section begins here
 */
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;

#if defined(__WATCOMC__) && !defined(__386__)
/* Open Watcom providing a 16 bit build for MS-DOS or OS/2 */
/* int is 16 bits; use long for 32 bits */
typedef long int32_t;
typedef unsigned long uint32_t;
#else
/* Otherwise, assume either a 32- or 64-bit compiler */
/* long may be 64 bits; use int for 32 bits */
typedef int int32_t;
typedef unsigned int uint32_t;
#endif

typedef long long int64_t;
typedef unsigned long long uint64_t;

#endif /* !C99 */

/* Provide uint8, int16, uint16, int32, uint32, int64 and uint64 */
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

#endif /* INTEGER_H */

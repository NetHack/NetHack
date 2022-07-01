/* NetHack 3.7	integer.h	$NHDT-Date: 1596498539 2020/08/03 23:48:59 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.8 $ */
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

/* DEC C (aka Compaq C for a while, HP C these days) for VMS is
   classified as a freestanding implementation rather than a hosted one
   and even though it claims to be C99, it does not provide <stdint.h>. */
#if defined(__DECC) && defined(VMS) && !defined(HAS_STDINT_H)
#define HAS_INTTYPES_H
#else /*!__DECC*/

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
    && !defined(HAS_STDINT_H)
/* The compiler claims to conform to C99. Use stdint.h */
#define HAS_STDINT_H
#endif
#if defined(__GNUC__) && defined(__INT64_MAX__) && !defined(HAS_STDINT_H)
#define HAS_STDINT_H
#endif

#endif /*?__DECC*/

#ifdef HAS_STDINT_H
#include <stdint.h>
#define SKIP_STDINT_WORKAROUND
#else /*!stdint*/
#ifdef HAS_INTTYPES_H
#include <inttypes.h>
#define SKIP_STDINT_WORKAROUND
#endif
#endif /*?stdint*/

#ifndef SKIP_STDINT_WORKAROUND /* !C99 */
/*
 * STDINT_WORKAROUND section begins here
 */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;

#if defined(__WATCOMC__) && !defined(__386__)
/* Open Watcom providing a 16 bit build for MS-DOS or OS/2 */
/* int is 16 bits; use long for 32 bits */
typedef long int int32_t;
typedef unsigned long int uint32_t;
#else
/* Otherwise, assume either a 32- or 64-bit compiler */
/* long may be 64 bits; use int for 32 bits */
typedef int int32_t;
typedef unsigned int uint32_t;
#endif

/* The only place where nethack cares about 64-bit integers is in the
   Isaac64 random number generator.  If your environment can't support
   64-bit integers, you should comment out USE_ISAAC64 in config.h so
   that the previous RNG gets used instead.  Then this file will be
   inhibited and it won't matter what the int64_t and uint64_t lines are. */
typedef long long int int64_t;
typedef unsigned long long int uint64_t;

#endif /* !C99 */

/* Provide int8, uint8, int16, uint16, int32, uint32, int64 and uint64 */
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

#endif /* INTEGER_H */

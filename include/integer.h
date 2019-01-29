/* NetHack 3.6	integer.h	$NHDT-Date: 1524689514 2018/04/25 20:51:54 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.0 $ */
/*      Copyright (c) 2016 by Michael Allison          */
/* NetHack may be freely redistributed.  See license for details. */

/* integer.h -- provide sized integer types */

#ifndef INTEGER_H
#define INTEGER_H

#if defined(__STDC__) && __STDC_VERSION__ >= 199101L
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

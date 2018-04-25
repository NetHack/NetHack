/* NetHack 3.6	integer.h	$NHDT-Date: 1524689514 2018/04/25 20:51:54 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.0 $ */
/*      Copyright (c) 2016 by Michael Allison          */
/* NetHack may be freely redistributed.  See license for details. */

/* integer.h -- provide sized integer types */

#ifndef INTEGER_H
#define INTEGER_H

#if defined(__STDC__) && __STDC__ >= 199101L

/* The compiler claims to conform to C99. Use stdint.h */
#include <stdint.h>
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;

#else /* !C99 */

/* Provide uint8, int16, uint16, int32 and uint32 */
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;

#if defined(__WATCOMC__) && !defined(__386__)
/* Open Watcom providing a 16 bit build for MS-DOS or OS/2 */
/* int is 16 bits; use long for 32 bits */
typedef long int32;
typedef unsigned long uint32;
#else
/* Otherwise, assume either a 32- or 64-bit compiler */
/* long may be 64 bits; use int for 32 bits */
typedef int int32;
typedef unsigned int uint32;
#endif

#endif /* !C99 */

#endif /* INTEGER_H */

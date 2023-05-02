/* NetHack 3.7	cstd.h	$NHDT-Date: 1596498562 2020/08/03 23:49:22 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.24 $ */
/*-Copyright (c) Robert Patrick Rankin, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef CSTD_H
#define CSTD_H

/*
 * The list of standard (C99 unless noted otherwise) header files:
 *
 * <assert.h>	         Conditionally compiled macro that compares its argument
 *                       to zero
 * <complex.h> (C99)     Complex number arithmetic
 * <ctype.h>	         Functions to determine the type contained in character
 *                       data
 * <errno.h>	         Macros reporting error conditions
 * <fenv.h> (C99)        Floating-point environment
 * <float.h>             Limits of floating-point types
 * <inttypes.h> (C99)    Format conversion of integer types
 * <iso646.h> (C95)      Alternative operator spellings
 * <limits.h>            Ranges of integer types
 * <locale.h>            Localization utilities
 * <math.h>              Common mathematics functions
 * <setjmp.h>            Nonlocal jumps
 * <signal.h>            Signal handling
 * <stdarg.h>            Variable arguments
 * <stdbool.h> (C99)     Macros for boolean type
 * <stddef.h>            Common macro definitions
 * <stdint.h> (C99)      Fixed-width integer types
 * <stdio.h>             Input/output program utilities
 *
 * General utilities:    memory management, program utilities, string conversions,
 *                       random numbers, algorithms
 * <string.h>            String handling
 * <tgmath.h> (C99)      Type-generic math (macros wrapping math.h and complex.h)
 * <time.h>              Time/date utilities
 * <wchar.h> (C95)       Extended multibyte and wide character utilities
 * <wctype.h> (C95)      Functions to determine the type contained in wide character data

 * We watch these and try not to conflict with them, or make it tough to adopt
 * these in future:
 *
 * <stdalign.h> (C11)    alignas and alignof convenience macros
 * <stdatomic.h> (C11)   Atomic operations
 * <stdbit.h> (C23)      Macros to work with the byte and bit representations of types
 * <stdckdint.h> (C23)   Macros for performing checked integer arithmetic
 * <stdnoreturn.h> (C11) noreturn convenience macro
 * <threads.h> (C11)     Thread library
 * <uchar.h> (C11)       UTF-16 and UTF-32 character utilities
 *
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>

#endif /* CSTD_H */

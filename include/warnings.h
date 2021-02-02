/* NetHack 3.7	warnings.h	$NHDT-Date: 1596498562 2020/08/03 23:49:22 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.24 $ */
/* Copyright (c) Michael Allison, 2021. */

#ifndef WARNINGS_H
#define WARNINGS_H

/*
 * If ENABLE_WARNING_PRAGMAS is defined, the checks for various
 * compilers is activated.
 *
 * If a suitable compiler is found, STDC_Pragma_AVAILABLE will be defined.
 * When STDC_Pragma_AVAILABLE is not defined, these are defined as no-ops:
 *     DISABLE_WARNING_UNREACHABLE_CODE
 *     DISABLE_WARNING_CONDEXPR_IS_CONSTANT
 *       ...
 *     RESTORE_WARNINGS
 *
 */

#if !defined(DISABLE_WARNING_PRAGMAS)
#if defined(__STDC_VERSION__)
#if __STDC_VERSION__ >= 199901L
#define ACTIVATE_WARNING_PRAGMAS
#endif /* __STDC_VERSION >= 199901L */
#endif /* __STDC_VERSION */
#if defined(_MSC_VER)
#ifndef ACTIVATE_WARNING_PRAGMAS
#define ACTIVATE_WARNING_PRAGMAS
#endif
#endif

#ifdef ACTIVATE_WARNING_PRAGMAS
#if defined(__clang__)
#define DISABLE_WARNING_UNREACHABLE_CODE \
                           _Pragma("clang diagnostic push") \
                           _Pragma("clang diagnostic ignored \"-Wunreachable-code\"")
#define DISABLE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNINGS _Pragma("clang diagnostic pop")
#define STDC_Pragma_AVAILABLE

#elif defined(__GNUC__)
/* unlike in clang, -Wunreachable-code does not function in later versions of gcc */
#define DISABLE_WARNING_UNREACHABLE_CODE \
                           _Pragma("GCC diagnostic push") \
                           _Pragma("GCC diagnostic ignored \"-Wunreachable-code\"")
#define DISABLE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNINGS _Pragma("GCC diagnostic pop")
#define STDC_Pragma_AVAILABLE

#elif defined(_MSC_VER)
#define DISABLE_WARNING_UNREACHABLE_CODE \
                           _Pragma("warning( push )") \
                           _Pragma("warning( disable : 4702 )")
#define DISABLE_WARNING_CONDEXPR_IS_CONSTANT \
                           _Pragma("warning( push )") \
                           _Pragma("warning( disable : 4127 )")
#define RESTORE_WARNING_CONDEXPR_IS_CONSTANT _Pragma("warning( pop )")
#define RESTORE_WARNINGS _Pragma("warning( pop )")
#define STDC_Pragma_AVAILABLE

#endif /* various compiler detections */
#endif /* ACTIVATE_WARNING_PRAGMAS */
#else  /* DISABLE_WARNING_PRAGMAS */
#if defined(STDC_Pragma_AVAILABLE)
#undef STDC_Pragma_AVAILABLE
#endif
#endif /* DISABLE_WARNING_PRAGMAS */

#if !defined(STDC_Pragma_AVAILABLE)
#define DISABLE_WARNING_UNREACHABLE_CODE
#define DISABLE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNINGS
#endif

#endif /* WARNINGS_H */

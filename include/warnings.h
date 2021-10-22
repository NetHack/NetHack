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
 *     DISABLE_WARNING_FORMAT_NONLITERAL
 *       ...
 *     RESTORE_WARNINGS
 *     RESTORE_WARNING_CONDEXPR_IS_CONSTANT
 *     RESTORE_WARNING_FORMAT_NONLITERAL
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
#if defined(__GNUC__) || defined(__clang__)
#if defined(__cplusplus)
#ifndef ACTIVATE_WARNING_PRAGMAS
#define ACTIVATE_WARNING_PRAGMAS
#endif
#endif /* __cplusplus */
#endif /* __GNUC__ || __clang__ */

#ifdef ACTIVATE_WARNING_PRAGMAS
#if defined(__clang__)
#define DISABLE_WARNING_UNREACHABLE_CODE \
                           _Pragma("clang diagnostic push") \
                           _Pragma("clang diagnostic ignored \"-Wunreachable-code\"")
#define DISABLE_WARNING_FORMAT_NONLITERAL \
                           _Pragma("clang diagnostic push") \
                           _Pragma("clang diagnostic ignored \"-Wformat-nonliteral\"")
#define DISABLE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNING_FORMAT_NONLITERAL _Pragma("clang diagnostic pop")
#define RESTORE_WARNING_UNREACHABLE_CODE _Pragma("clang diagnostic pop")
#define RESTORE_WARNINGS _Pragma("clang diagnostic pop")
#define STDC_Pragma_AVAILABLE

#elif defined(__GNUC__)
/* unlike in clang, -Wunreachable-code does not function in later versions of gcc */
#define DISABLE_WARNING_UNREACHABLE_CODE \
                           _Pragma("GCC diagnostic push") \
                           _Pragma("GCC diagnostic ignored \"-Wunreachable-code\"")
#define DISABLE_WARNING_FORMAT_NONLITERAL \
                           _Pragma("GCC diagnostic push") \
                           _Pragma("GCC diagnostic ignored \"-Wformat-nonliteral\"")
#define DISABLE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNING_FORMAT_NONLITERAL _Pragma("GCC diagnostic pop")
#define RESTORE_WARNING_UNREACHABLE_CODE _Pragma("GCC diagnostic pop")
#define RESTORE_WARNINGS _Pragma("GCC diagnostic pop")
#define STDC_Pragma_AVAILABLE

#elif defined(_MSC_VER)
#if _MSC_VER > 1916
#define DISABLE_WARNING_UNREACHABLE_CODE \
                           _Pragma("warning( push )") \
                           _Pragma("warning( disable : 4702 )")
#define DISABLE_WARNING_FORMAT_NONLITERAL \
                           _Pragma("warning( push )") \
                           _Pragma("warning( disable : 4774 )")
#define DISABLE_WARNING_CONDEXPR_IS_CONSTANT \
                           _Pragma("warning( push )") \
                           _Pragma("warning( disable : 4127 )")
#define RESTORE_WARNING_CONDEXPR_IS_CONSTANT _Pragma("warning( pop )")
#define RESTORE_WARNING_FORMAT_NONLITERAL _Pragma("warning( pop )")
#define RESTORE_WARNING_UNREACHABLE_CODE _Pragma("warning( pop )")
#define RESTORE_WARNINGS _Pragma("warning( pop )")
#define STDC_Pragma_AVAILABLE
#else  /* Visual Studio prior to 2019 below */
#define DISABLE_WARNING_UNREACHABLE_CODE \
                           __pragma(warning(push)) \
                           __pragma(warning(disable:4702))
#define DISABLE_WARNING_FORMAT_NONLITERAL \
                           __pragma(warning(push)) \
                           __pragma(warning(disable:4774))
#define DISABLE_WARNING_CONDEXPR_IS_CONSTANT \
                           __pragma(warning(push)) \
                           __pragma(warning(disable:4127))
#define RESTORE_WARNING_CONDEXPR_IS_CONSTANT __pragma(warning(pop))
#define RESTORE_WARNING_FORMAT_NONLITERAL __pragma(warning(pop))
#define RESTORE_WARNING_UNREACHABLE_CODE __pragma(warning(pop))
#define RESTORE_WARNINGS  __pragma(warning(pop))
#define STDC_Pragma_AVAILABLE
#endif /* vs2019 or vs2017 */

#endif /* various compiler detections */
#endif /* ACTIVATE_WARNING_PRAGMAS */
#else  /* DISABLE_WARNING_PRAGMAS */
#if defined(STDC_Pragma_AVAILABLE)
#undef STDC_Pragma_AVAILABLE
#endif
#endif /* DISABLE_WARNING_PRAGMAS */

#if !defined(STDC_Pragma_AVAILABLE)
#define DISABLE_WARNING_UNREACHABLE_CODE
#define DISABLE_WARNING_FORMAT_NONLITERAL
#define DISABLE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNING_CONDEXPR_IS_CONSTANT
#define RESTORE_WARNING_FORMAT_NONLITERAL
#define RESTORE_WARNING_UNREACHABLE_CODE
#define RESTORE_WARNINGS
#endif

#endif /* WARNINGS_H */

/* NetHack 3.7	qt_pre.h	$NHDT-Date: 1597276835 2020/08/13 00:00:35 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.0 $ */

/*
 * qt_pre.h -- undefine some nethack macros which conflict with Qt headers.
 *
 * #include after "hack.h", before <Qt.../Qt...>.
 */

#undef C            // conflicts with Qt6 header
#undef Invisible
#undef Warning
#undef index
#undef msleep
#undef rindex
#undef wizard
#undef yn
#undef min
#undef max

#if defined(__cplusplus)

#ifdef __clang__
#pragma clang diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#endif
/* the diagnostic pop is in qt_post.h */

#ifdef __clang__
/* disable warnings for shadowed names; some of the Qt prototypes use
   placeholder argument names which conflict with nethack variables
   ('g', 'u', a couple of others) */
#pragma clang diagnostic ignored "-Wshadow"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <QtGlobal>

/* QFontMetrics::width was deprecated in Qt 5.11 */
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
#define QFM_WIDTH(foo) width(foo)
#else
#define QFM_WIDTH(foo) horizontalAdvance(foo)
#endif

#if __cplusplus >= 202002L
/* c++20 or newer */
#if QT_VERSION < 0x060000
/*
 * qt5/QtWidgets/qsizepolicy.h
 * Qt5 header file issue under c++ 20
 *
 * warning: bitwise operation between different enumeration types 
 * ‘QSizePolicy::Policy’ and ‘QSizePolicy::PolicyFlag’
 * is deprecated [-Wdeprecated-enum-enum-conversion]
 */
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
#endif  /* QT_VERSION < 0x060000 */
#endif  /* __cplusplus >= 202002L */
#endif /* __cplusplus */

/*qt_pre.h*/


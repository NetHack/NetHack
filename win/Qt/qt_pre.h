/* NetHack 3.7	qt_pre.h	$NHDT-Date: 1597276835 2020/08/13 00:00:35 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.0 $ */

/*
 * qt_pre.h -- undefine some nethack macros which conflict with Qt headers.
 *
 * #include after "hack.h", before <Qt.../Qt...>.
 */

#undef Invisible
#undef Warning
#undef index
#undef msleep
#undef rindex
#undef wizard
#undef yn
#undef min
#undef max

/* disable warnings for shadowed names; some of the Qt prototypes use
   placeholder argument names which conflict with nethack variables
   ('g', 'u', a couple of others) */
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#endif

/*qt_pre.h*/


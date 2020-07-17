#if defined(QT_GRAPHICS)
#if defined(MACOSX)
 
/* 
 * The following conflict with Qt header files so after
 * undefing them in qt_undef.h, we redefine them back to
 * the non-conflicting names again in here, presumably
 * right after the Qt header files with the conflicts have 
 * been included.
 */

#define u NETHACK_u
#define flags NETHACK_flags
#define g NETHACK_g
#define cg NETHACK_cg

#endif /* MACOSX */
#endif /* QT_GRAPHICS */


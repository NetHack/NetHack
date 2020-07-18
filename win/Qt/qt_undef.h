#if defined(QT_GRAPHICS)
#if defined(MACOSX)

/* 
 * The following conflict with Qt header files so we
 * undef them in here, then redef them using qt_redef.h
 * after the Qt includes.
 */

#undef u
#undef flags
#undef g
#undef cg

#endif /* MACOSX */
#endif /* QT_GRAPHICS */


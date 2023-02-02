/* NetHack 3.6	gnglyph.h	$NHDT-Date: 1432512806 2015/05/25 00:13:26 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GnomeHackGlyph_h
#define GnomeHackGlyph_h

#include "config.h"
#include "global.h"

/* the prototypes in system headers contain useless argument names
   that trigger spurious warnings if gcc's `-Wshadow' option is used */
#ifdef index
#undef index
#endif
#define index _hide_index_
#define time _hide_time_

#include <gdk_imlib.h>
#include <gdk/gdk.h>

#undef index
#undef time

extern short glyph2tile[]; /* From tile.c */

typedef struct {
    GdkImlibImage *im;
    int count;
    int width;
    int height;
} GHackGlyphs;

extern int ghack_init_glyphs(const char *);
extern void ghack_free_glyphs(void);
extern void ghack_dispose_glyphs(void);
extern int ghack_glyph_count(void);
extern GdkImlibImage *ghack_image_from_glyph(int, gboolean);
extern int ghack_glyph_height(void);
extern int ghack_glyph_width(void);

#endif /* GnomeHackGlyph_h */

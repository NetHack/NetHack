/* NetHack 3.7  tile.h       $NHDT-Date: 1596498339 2020/08/03 23:45:39 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.14 $ */
/*      Copyright (c) 2016 by Michael Allison             */
/* NetHack may be freely redistributed.  See license for details. */

typedef unsigned char pixval;

typedef struct pixel_s {
    pixval r, g, b;
} pixel;

#define MAXCOLORMAPSIZE 256

#define CM_RED 0
#define CM_GREEN 1
#define CM_BLUE 2

/* shared between reader and writer */
extern pixval ColorMap[3][MAXCOLORMAPSIZE];
extern int colorsinmap;
/* writer's accumulated colormap */
extern pixval MainColorMap[3][MAXCOLORMAPSIZE];
extern int colorsinmainmap;

#include "dlb.h" /* for MODEs */

/* size of tiles */
#ifndef TILE_X
#define TILE_X 16
#endif
#ifndef TILE_Y
#define TILE_Y 16
#endif

#define Fprintf (void) fprintf

extern boolean fopen_text_file(const char *, const char *);
extern boolean read_text_tile(pixel(*) [TILE_X]);
extern boolean write_text_tile(pixel(*) [TILE_X]);
extern int fclose_text_file(void);

extern void set_grayscale(int);
extern void init_colormap(void);
extern void merge_colormap(void);

#if defined(MICRO) || defined(WIN32)
#undef exit
#if !defined(MSDOS) && !defined(WIN32)
extern void exit(int);
#endif
#endif

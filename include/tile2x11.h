/* NetHack 3.7	tile2x11.h	$NHDT-Date: 1597274123 2020/08/12 23:15:23 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.12 $ */
/*      Copyright (c) 2002 by David Cohrs              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef TILE2X11_H
#define TILE2X11_H

/*
 * Header for the X11 tile map.
 *
 * dat/x11tiles is used by Qt for fallback if nhtiles.bmp is inaccessible,
 * so this header is used by Qt as well as by X11.
 */
typedef struct {
    unsigned long version;
    unsigned long ncolors;
    unsigned long tile_width;
    unsigned long tile_height;
    unsigned long ntiles;
    unsigned long per_row;
} x11_header;

/* how wide each row in the tile file is, in tiles */
#define TILES_PER_ROW (40)

#endif /* TILE2X11_H */

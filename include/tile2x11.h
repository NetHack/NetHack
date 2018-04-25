/* NetHack 3.6	tile2x11.h	$NHDT-Date: 1524689515 2018/04/25 20:51:55 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.10 $ */
/*      Copyright (c) 2002 by David Cohrs              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef TILE2X11_H
#define TILE2X11_H

/*
 * Header for the x11 tile map.
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

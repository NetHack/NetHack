/*	SCCS Id: @(#)tile2x11.h 3.3	95/01/25	*/
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
} x11_header;

#endif	/* TILE2X11_H */

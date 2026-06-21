/* NetHack 5.0    tileset.h    $NHDT-Date: 1781973089 2026/06/20 16:31:29 $ $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.6 $ */
/* Copyright (c) Ray Chason, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef TILESET_H
#define TILESET_H

struct Pixel {
    unsigned char r, g, b, a;
};

struct TileImage {
    /* Image data */
    unsigned width, height;
    struct Pixel *pixels; /* for direct color */
    unsigned char *indexes; /* for paletted images */
};

boolean read_tiles(const char *filename, boolean true_color);
const struct Pixel *get_palette(void);
void set_tile_type(boolean true_color);
void free_tiles(void);
const struct TileImage *get_tile(unsigned tile_index);

/* For resizing tiles */
struct TileImage *stretch_tile(const struct TileImage *, unsigned,
                               unsigned);
void free_tile(struct TileImage *);

/* Used internally by the tile set code */
struct TileSetImage {
    /* Image data */
    unsigned width, height;
    struct Pixel *pixels; /* for direct color */
    unsigned char *indexes; /* for paletted images */
    struct Pixel palette[256];

    /* Image description from the file */
    char *image_desc;

    /* Tile dimensions */
    unsigned tile_width, tile_height;
};

boolean read_bmp_tiles(const char *filename, struct TileSetImage *image);
boolean read_gif_tiles(const char *filename, struct TileSetImage *image);
boolean read_png_tiles(const char *filename, struct TileSetImage *image);

#endif

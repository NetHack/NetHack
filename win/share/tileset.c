/* NetHack 3.6    tileset.c    $NHDT-Date: 1501463811 2017/07/31 01:16:51 $ $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.0 $ */
/* Copyright (c) Ray Chason, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "tileset.h"

static void FDECL(get_tile_map, (const char *));
static void FDECL(split_tiles, (const struct TileSetImage *));
static void FDECL(free_image, (struct TileSetImage *));

static struct TileImage *tiles;
static unsigned num_tiles;
static struct TileImage blank_tile; /* for graceful undefined tile handling */

static boolean have_palette;
static struct Pixel palette[256];

boolean
read_tiles(filename, true_color)
const char *filename;
boolean true_color;
{
    static const unsigned char png_sig[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
    };
    struct TileSetImage image;
    FILE *fp;
    char header[16];
    boolean ok;

    /* Fill the image structure with known values */
    image.width = 0;
    image.height = 0;
    image.pixels = NULL;        /* custodial */
    image.indexes = NULL;       /* custodial */
    image.image_desc = NULL;    /* custodial */
    image.tile_width = 0;
    image.tile_height = 0;

    /* Identify the image type */
    fp = fopen(filename, "rb");
    if (!fp)
        goto error;
    memset(header, 0, sizeof(header));
    fread(header, 1, sizeof(header), fp);
    fclose(fp), fp = (FILE *) 0;

    /* Call the loader appropriate for the image */
    if (memcmp(header, "BM", 2) == 0) {
        ok = read_bmp_tiles(filename, &image);
    } else if (memcmp(header, "GIF87a", 6) == 0
           ||  memcmp(header, "GIF89a", 6) == 0) {
        ok = read_gif_tiles(filename, &image);
    } else if (memcmp(header, png_sig, sizeof(png_sig)) == 0) {
        ok = read_png_tiles(filename, &image);
    } else {
        ok = FALSE;
    }
    if (!ok)
        goto error;

    /* Reject if the interface cannot handle direct color and the image does
       not use a palette */
    if (!true_color && image.indexes == NULL)
        goto error;

    /* Save the palette if present */
    have_palette = image.indexes != NULL;
    memcpy(palette, image.palette, sizeof(palette));

    /* Set defaults for tile metadata */
    if (image.tile_width == 0) {
        image.tile_width = image.width / 40;
    }
    if (image.tile_height == 0) {
        image.tile_height = image.tile_width;
    }
    /* Set the tile dimensions if the user has not done so */
    if (iflags.wc_tile_width == 0) {
        iflags.wc_tile_width = image.tile_width;
    }
    if (iflags.wc_tile_height == 0) {
        iflags.wc_tile_height = image.tile_height;
    }

    /* Parse the tile map */
    get_tile_map(image.image_desc);

    /* Split the image into tiles */
    split_tiles(&image);

    free_image(&image);
    return TRUE;

error:
    if (fp)
        fclose(fp);
    free_image(&image);
    return FALSE;
}

const struct Pixel *
get_palette()
{
    return have_palette ? palette : NULL;
}

/* TODO: derive tile_map from image_desc */
static void
get_tile_map(image_desc)
const char *image_desc;
{
    return;
}

void
free_tiles()
{
    unsigned i;

    if (tiles) {
        for (i = 0; i < num_tiles; ++i) {
            free((genericptr_t) tiles[i].pixels);
            free((genericptr_t) tiles[i].indexes);
        }
        free((genericptr_t) tiles), tiles = NULL;
    }
    num_tiles = 0;

    if (blank_tile.pixels)
        free((genericptr_t) blank_tile.pixels), blank_tile.pixels = NULL;
    if (blank_tile.indexes)
        free((genericptr_t) blank_tile.indexes), blank_tile.indexes = NULL;
}

static void
free_image(image)
struct TileSetImage *image;
{
    if (image->pixels)
        free((genericptr_t) image->pixels), image->pixels = NULL;
    if (image->indexes)
        free((genericptr_t) image->indexes), image->indexes = NULL;
    if (image->image_desc)
        free((genericptr_t) image->image_desc), image->image_desc = NULL;
}

const struct TileImage *
get_tile(tile_index)
unsigned tile_index;
{
    if (tile_index >= num_tiles)
        return &blank_tile;
    return &tiles[tile_index];
}

static void
split_tiles(image)
const struct TileSetImage *image;
{
    unsigned tile_rows, tile_cols;
    size_t tile_size, i, j;
    unsigned x1, y1, x2, y2;

    /* Get the number of tiles */
    tile_rows = image->height / iflags.wc_tile_height;
    tile_cols = image->width / iflags.wc_tile_width;
    num_tiles = tile_rows * tile_cols;
    tile_size = (size_t) iflags.wc_tile_height * (size_t) iflags.wc_tile_width;

    /* Allocate the tile array */
    tiles = (struct TileImage *) alloc(num_tiles * sizeof(tiles[0]));
    memset(tiles, 0, num_tiles * sizeof(tiles[0]));

    /* Copy the pixels into the tile structures */
    for (y1 = 0; y1 < tile_rows; ++y1) {
        for (x1 = 0; x1 < tile_cols; ++x1) {
            struct TileImage *tile = &tiles[y1 * tile_cols + x1];

            tile->width = iflags.wc_tile_width;
            tile->height = iflags.wc_tile_height;
            tile->pixels = (struct Pixel *)
                    alloc(tile_size * sizeof (struct Pixel));
            if (image->indexes != NULL) {
                tile->indexes = (unsigned char *) alloc(tile_size);
            }
            for (y2 = 0; y2 < iflags.wc_tile_height; ++y2) {
                for (x2 = 0; x2 < iflags.wc_tile_width; ++x2) {
                    unsigned x = x1 * iflags.wc_tile_width + x2;
                    unsigned y = y1 * iflags.wc_tile_height + y2;

                    i = y * image->width + x;
                    j = y2 * tile->width + x2;
                    tile->pixels[j] = image->pixels[i];
                    if (image->indexes != NULL) {
                        tile->indexes[j] = image->indexes[i];
                    }
                }
            }
        }
    }

    /* Create a blank tile for use when the tile index is invalid */
    blank_tile.width = iflags.wc_tile_width;
    blank_tile.height = iflags.wc_tile_height;
    blank_tile.pixels = (struct Pixel *)
            alloc(tile_size * sizeof (struct Pixel));
    for (i = 0; i < tile_size; ++i) {
        blank_tile.pixels[i].r = 0;
        blank_tile.pixels[i].g = 0;
        blank_tile.pixels[i].b = 0;
        blank_tile.pixels[i].a = 255;
    }
    if (image->indexes) {
        blank_tile.indexes = (unsigned char *) alloc(tile_size);
        memset(blank_tile.indexes, 0, tile_size);
    }
}

boolean
read_png_tiles(filename, image)
const char *filename;
struct TileSetImage *image;
{
    /* stub */
    return FALSE;
}

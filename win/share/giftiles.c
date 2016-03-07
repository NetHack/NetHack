/* NetHack 3.6    giftiles.c    $NHDT-Date: 1457358406 2016/03/07 13:46:46 $ $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.0 $ */
/* Copyright (c) Ray Chason, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

/* giftiles.c -- read a tile map in GIF format */
/* Reference: GIF specification,, retrieved from
 * http://www.w3.org/Graphics/GIF/spec-gif89a.txt */

#include "config.h"
#include "tileset.h"

#define MIN_LZW_BITS 3
#define MAX_LZW_BITS 12
#define END_OF_DATA 0x7FFF

/* For use when reading the GIF as a bitstream */
struct Bitstream {
    /* The file */
    FILE *fp;

    /* For unpacking LZW codes */
    unsigned long bits;
    unsigned char num_bits;
    unsigned char bit_width;
    unsigned char initial_bit_width;
    unsigned char block_size;

    /* The dictionary */
    struct {
        unsigned char byte;
        unsigned short next;
    } dictionary[1 << MAX_LZW_BITS];
    unsigned short dict_size;

    /* The string currently being decoded */
    unsigned char string[1 << MAX_LZW_BITS];
    unsigned short str_size;

    unsigned short last_code;
};

struct DataBlock {
    size_t size;
    size_t index;
    unsigned char *data;
};

static boolean FDECL(read_data_block, (struct Bitstream *gif, struct DataBlock *block));
static void FDECL(free_data_block, (struct DataBlock *block));
static unsigned short FDECL(read_u16, (const unsigned char buf[2]));
static void FDECL(init_decoder, (struct Bitstream *gif, unsigned bit_width));
static void FDECL(reset_decoder, (struct Bitstream *gif));
static int FDECL(decode, (struct Bitstream *gif, struct DataBlock *block));
static int FDECL(get_code, (struct Bitstream *gif, struct DataBlock *block));
static unsigned FDECL(interlace_incr, (unsigned y, unsigned height));

/*
 * GIF specifies a canvas, which may have a palette (the "global color table")
 * of up to 256 colors, of which one is a background color, and zero or more
 * images, each of which may have its own palette (a "local color table")
 * independently of the canvas. We will join all palettes found into a single
 * palette, and indicate that the image uses a palette if, and only if, the
 * various palettes do not total more than 256 colors.
 */

boolean
read_gif_tiles(filename, image)
const char *filename;
struct TileSetImage *image;
{
    struct Bitstream gif;
    struct DataBlock block;

    unsigned char buf[1024];
    size_t size, num_pixels, i;

    /* Image data not returned to the caller */
    boolean have_gct;   /* global color table is present */
    unsigned gct_size;  /* global color table size */
    unsigned back_color; /* index for background color */
    unsigned trans_color = 0xFFFF; /* index for transparent palette entry */

    block.data = NULL;      /* custodial */
    gif.fp = NULL;          /* custodial */

    image->width = 0;
    image->height = 0;
    image->pixels = NULL;       /* custodial, returned */
    image->indexes = NULL;      /* custodial, returned */
    image->image_desc = NULL;   /* custodial, returned */
    image->tile_width = 0;
    image->tile_height = 0;

    gif.fp = fopen(filename, "rb");
    if (gif.fp == NULL) goto error;

    /* 17. Header */
    size = fread(buf, 1, 6, gif.fp);
    if (size < 6) goto error;
    if (memcmp(buf, "GIF87a", 6) != 0 && memcmp(buf, "GIF89a", 6) != 0)
        goto error;

    /* 18. Logical screen descriptor */
    size = fread(buf, 1, 7, gif.fp);
    if (size < 7) goto error;
    image->width = read_u16(buf + 0);
    image->height = read_u16(buf + 2);
    have_gct = (buf[4] & 0x80) != 0;
    gct_size = 1 << ((buf[4] & 0x07) + 1);
    back_color = buf[5];
    if (image->width == 0 || image->height == 0) goto error;

    /* 19. Global Color Table */
    for (i = 0; i < SIZE(image->palette); ++i) {
        image->palette[i].r = 0;
        image->palette[i].g = 0;
        image->palette[i].b = 0;
        image->palette[i].a = 255;
    }
    if (have_gct) {
        size = fread(buf, 3, gct_size, gif.fp);
        if (size < gct_size) goto error;
        for (i = 0; i < gct_size; ++i) {
            image->palette[i].r = buf[i * 3 + 0];
            image->palette[i].g = buf[i * 3 + 1];
            image->palette[i].b = buf[i * 3 + 2];
            image->palette[i].a = 255;
        }
    }

    /* Allocate pixel area; watch out for overflow */
    num_pixels = (size_t) image->width * (size_t) image->height;
    if (num_pixels / image->width != image->height) goto error; /* overflow */
    size = num_pixels * sizeof(image->pixels[0]);
    if (size / sizeof(image->pixels[0]) != num_pixels) goto error; /* overflow */
    image->pixels = (struct Pixel *) alloc(size);
    image->indexes = (unsigned char *) alloc(num_pixels);

    /* Fill with the background color */
    for (i = 0; i < num_pixels; ++i) {
        image->pixels[i] = image->palette[back_color];
        image->indexes[i] = back_color;
    }

    /* Read the image data */
    while (TRUE) {
        int b = fgetc(gif.fp);
        if (b == EOF) goto error;

        /* 27. Trailer (0x3B) */
        if (b == 0x3B) break;

        switch (b) {
        case 0x2C:
            /* 20. Image descriptor (0x2C) */
            {
                unsigned img_left, img_top, img_width, img_height;
                boolean have_lct, interlace;
                unsigned lct_start, lct_size;
                struct Pixel lct[256];
                int b;
                unsigned x, y, x2, y2;

                size = fread(buf, 1, 9, gif.fp);
                if (size < 9) goto error;
                img_left = read_u16(buf + 0);
                img_top = read_u16(buf + 2);
                img_width = read_u16(buf + 4);
                img_height = read_u16(buf + 6);
                have_lct = (buf[8] & 0x80) != 0;
                interlace = (buf[8] & 0x40) != 0;
                lct_size = 1 << ((buf[8] & 0x07) + 1);

                /* 21. Local color table */
                lct_start = 0;
                memcpy(lct, image->palette, sizeof(lct));
                if (have_lct) {
                    size = fread(buf, 3, lct_size, gif.fp);
                    if (size < lct_size) goto error;
                    for (i = 0; i < lct_size; ++i) {
                        lct[i].r = buf[i * 3 + 0];
                        lct[i].g = buf[i * 3 + 1];
                        lct[i].b = buf[i * 3 + 2];
                        lct[i].a = 255;
                    }
                    /*
                     * The combined palette may exceed 256 colors, in which
                     * case the indexes array will be discarded, indicating a
                     * full-color image.
                     */
                    if (lct_size + gct_size <= 256) {
                        memcpy(image->palette, lct, sizeof(lct[0]) * lct_size);
                    }
                    lct_start = gct_size;
                    gct_size += lct_start;
                }
                if (trans_color != 0xFFFF) {
                    lct[trans_color].a = 0;
                    if (!have_lct) {
                        /* FIXME: this will affect all images using the global
                         * color table, not just the current one */
                        image->palette[trans_color].a = 0;
                    }
                }
                /* 22. Table based image data */
                b = fgetc(gif.fp);
                if (b == EOF) goto error;
                if (b < MIN_LZW_BITS - 1 || MAX_LZW_BITS - 1 < b) goto error;
                init_decoder(&gif, b);
                x = 0;
                y = 0;
                if (!read_data_block(&gif, &block)) goto error;
                while (TRUE) {
                    b = decode(&gif, &block);
                    if (b == EOF) goto error;
                    if (b == END_OF_DATA) break;
                    if (y >= img_height) goto error;
                    x2 = img_left + x;
                    y2 = img_top + y;
                    if (x2 < image->width && y2 < image->height) {
                        image->pixels[y2 * image->width + x2] = lct[b];
                        image->indexes[y2 * image->width + x2] = b + lct_start;
                    }
                    ++x;
                    if (x >= img_width) {
                        x = 0;
                        if (interlace) {
                            y = interlace_incr(y, img_height);
                        } else {
                            ++y;
                        }
                    }
                }
                free_data_block(&block);
                trans_color = 0xFFFF;
            }
            break;

        case 0x21:
            /* Extension blocks */
            {
                int label;

                label = fgetc(gif.fp);
                if (label == EOF) goto error;
                if (!read_data_block(&gif, &block)) goto error;
                switch (label) {
                case 0xF9:
                    /* 23. Graphic control extension (0xF9) */
                    if (block.size >= 4 && (block.data[0] & 0x01) != 0) {
                        /* image has a transparent index */
                        trans_color = block.data[3];
                    }
                    break;

#if 0
                case 0xFE:
                    /* 24. Comment extension (0xFE) */
                    break;

                case 0x01:
                    /* 25. Plain text extension (0x01) */
                    break;
#endif

                case 0xFF:
                    /* 26. Application extension (0xFF) */
                    if (block.size > 11
                    &&  memcmp(block.data, "NETHACK3GIF", 11) == 0
                    &&  image->image_desc == NULL) {
                        memmove(block.data, block.data + 11, block.size - 11);
                        block.data[block.size - 11] = '\0';
                        image->image_desc = (char *) block.data;
                        block.data = NULL;
                    }
                    break;

                default:
                    /* Unknown extension type */
                    break;
                }
                free_data_block(&block);
            }
            break;

        default:
            goto error;
        }
    }

    fclose(gif.fp);
    free_data_block(&block);
    if (gct_size > 256) {
        /* Max palette size exceeded; indexes array is not meaningful */
        free(image->indexes);
        image->indexes = NULL;
    }
    return TRUE;

error:
    if (gif.fp) fclose(gif.fp);
    free_data_block(&block);
    free(image->pixels);
    image->pixels = NULL;
    free(image->indexes);
    image->indexes = NULL;
    free(image->image_desc);
    image->image_desc = NULL;
    return FALSE;
}

static void
init_decoder(gif, bit_width)
struct Bitstream *gif;
unsigned bit_width;
{
    unsigned i;
    unsigned clear;

    gif->bits = 0;
    gif->num_bits = 0;
    gif->initial_bit_width = bit_width;
    gif->block_size = 0;

    clear = 1 << bit_width;
    gif->dict_size = clear + 2;
    for (i = 0; i < clear; ++i) {
        gif->dictionary[i].byte = i;
        gif->dictionary[i].next = 0xFFFF;
    }

    gif->str_size = 0;

    reset_decoder(gif);
}

static void
reset_decoder(gif)
struct Bitstream *gif;
{
    /* Set the bit width */
    gif->bit_width = gif->initial_bit_width + 1;

    /* Reset the dictionary */
    gif->dict_size = (1 << gif->initial_bit_width) + 2;

    /* No last code */
    gif->last_code = 0xFFFF;
}

static int
decode(gif, block)
struct Bitstream *gif;
struct DataBlock *block;
{
    int code;
    unsigned clear = 1 << gif->initial_bit_width;

    /* If a string is being decoded, return the next byte */
    if (gif->str_size != 0) {
        return gif->string[--gif->str_size];
    }

    /* Get the next code, until code other than clear */
    while (TRUE) {
        code = get_code(gif, block);
        if (code != clear) break;
        reset_decoder(gif);
    }

    if (code == EOF) return EOF;
    if (code == clear + 1) return END_OF_DATA;
    if (code > gif->dict_size) return EOF;

    /* Add a new string to the dictionary */
    if (gif->last_code != 0xFFFF && gif->dict_size < SIZE(gif->dictionary)) {
        unsigned next_code;
        if (code < gif->dict_size) {
            next_code = code;
        } else {
            next_code = gif->last_code;
        }
        while (next_code >= clear) {
            next_code = gif->dictionary[next_code].next;
        }
        gif->dictionary[gif->dict_size].next = gif->last_code;
        gif->dictionary[gif->dict_size].byte = next_code;
        ++gif->dict_size;
        if (gif->dict_size >= 1 << gif->bit_width
        &&  gif->bit_width < MAX_LZW_BITS) {
            ++gif->bit_width;
        }
    }
    gif->last_code = code;

    /* code is less than gif->dict_size and not equal to clear or clear + 1 */
    /* Prepare the decoded string for return; note that it is stored in
     * reverse order */
    while (code >= clear) {
        gif->string[gif->str_size++] = gif->dictionary[code].byte;
        code = gif->dictionary[code].next;
    }

    return code;
}

static int
get_code(gif, block)
struct Bitstream *gif;
struct DataBlock *block;
{
    int code;

    while (gif->num_bits < gif->bit_width) {
        unsigned char b;
        if (block->index >= block->size) return EOF;
        b = block->data[block->index++];
        gif->bits |= (unsigned long)b << gif->num_bits;
        gif->num_bits += 8;
    }

    code = (int) (gif->bits & ((1UL << gif->bit_width) - 1));
    gif->bits >>= gif->bit_width;
    gif->num_bits -= gif->bit_width;
    return code;
}

static unsigned
interlace_incr(y, height)
unsigned y;
unsigned height;
{
    static const unsigned char incr[] = { 8, 2, 4, 2 };

    /* The lower three bits indicate the current pass */

    /* Advance to the next row of the current pass */
    y += incr[y & 0x3];

    /* Go to the next pass if y exceeds height */
    /* Might not be the immediately following pass if height is small */
    if (y >= height && (y & 0x7) == 0) {
        /* Pass 1 -> Pass 2 */
        y = 4;
    }
    if (y >= height && (y & 0x7) == 4) {
        /* Pass 2 -> Pass 3 */
        y = 2;
    }
    if (y >= height && (y & 0x3) == 2) {
        /* Pass 3 -> Pass 4 */
        y = 1;
    }

    return y;
}

/* Decode an unsigned 16 bit quantity */
static unsigned short
read_u16(buf)
const unsigned char buf[2];
{
    return ((unsigned short)buf[0] << 0)
         | ((unsigned short)buf[1] << 8);
}

static boolean
read_data_block(gif, block)
struct Bitstream *gif;
struct DataBlock *block;
{
    long pos = ftell(gif->fp);
    int b;
    size_t i;

    free_data_block(block);

    /* Get the length of the data block */
    while (TRUE) {
        b = fgetc(gif->fp);
        if (b == EOF) return FALSE;
        if (b == 0) break;
        block->size += b;
        fseek(gif->fp, b, SEEK_CUR);
    }
    fseek(gif->fp, pos, SEEK_SET);

    /* Allocate memory */
    block->data = (unsigned char *) alloc(block->size);

    /* Read the data from the file */
    i = 0;
    while (TRUE) {
        b = fgetc(gif->fp);
        if (b == EOF) return FALSE;
        if (b == 0) break;
        if (fread(block->data + i, 1, b, gif->fp) != b) return FALSE;
        i += b;
    }

    block->index = 0;
    return TRUE;
}

static void
free_data_block(block)
struct DataBlock *block;
{
    free(block->data);
    block->size = 0;
    block->data = NULL;
}

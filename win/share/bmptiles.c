/* NetHack 3.7    bmptiles.c    $NHDT-Date: 1596498334 2020/08/03 23:45:34 $ $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $ */
/* Copyright (c) Ray Chason, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#include "tileset.h"

/* First BMP file header */
struct BitmapHeader {
    char magic[2];
    uint32 bmp_size;
    uint32 img_offset;
};

/* Color model information for larger BMP headers */
struct CIE_XYZ {
    uint32 ciexyzX;
    uint32 ciexyzY;
    uint32 ciexyzZ;
};

struct CIE_XYZTriple {
    struct CIE_XYZ ciexyzRed;
    struct CIE_XYZ ciexyzGreen;
    struct CIE_XYZ ciexyzBlue;
};

/* Second BMP file header */
/* This one can vary in size; contents can vary according to the size */
struct BitmapInfoHeader {
    uint32 Size;                    /* 12 40 52 56 108 124 64 */
    int32  Width;                   /* 12 40 52 56 108 124 64 */
    int32  Height;                  /* 12 40 52 56 108 124 64 */
    uint16 NumPlanes;               /* 12 40 52 56 108 124 64 */
    uint16 BitsPerPixel;            /* 12 40 52 56 108 124 64 */
    uint32 Compression;             /*    40 52 56 108 124 64 */
    uint32 ImageDataSize;           /*    40 52 56 108 124 64 */
    int32  XResolution;             /*    40 52 56 108 124 64 */
    int32  YResolution;             /*    40 52 56 108 124 64 */
    uint32 ColorsUsed;              /*    40 52 56 108 124 64 */
    uint32 ColorsImportant;         /*    40 52 56 108 124 64 */
    uint32 RedMask;                 /*       52 56 108 124 */
    uint32 GreenMask;               /*       52 56 108 124 */
    uint32 BlueMask;                /*       52 56 108 124 */
    uint32 AlphaMask;               /*          56 108 124 */
    uint32 CSType;                  /*             108 124 */
    struct CIE_XYZTriple Endpoints; /*             108 124 */
    uint32 GammaRed;                /*             108 124 */
    uint32 GammaGreen;              /*             108 124 */
    uint32 GammaBlue;               /*             108 124 */
    uint32 Intent;                  /*                 124 */
    uint32 ProfileData;             /*                 124 */
    uint32 ProfileSize;             /*                 124 */
};
/* Compression */
#define BI_RGB           0
#define BI_RLE8          1
#define BI_RLE4          2
#define BI_BITFIELDS     3
#define BI_JPEG          4
#define BI_PNG           5

static uint16 read_u16(const unsigned char buf[2]);
static uint32 read_u32(const unsigned char buf[4]);
static int32 read_s32(const unsigned char buf[4]);
static struct Pixel build_pixel(const struct BitmapInfoHeader *, uint32);
static unsigned char pixel_element(uint32, uint32);
static boolean read_header(FILE *, struct BitmapHeader *);
static boolean read_info_header(FILE *, struct BitmapInfoHeader *);
static boolean check_info_header(const struct BitmapInfoHeader *);
static unsigned get_palette_size(const struct BitmapInfoHeader *);
static boolean read_palette(FILE *, struct Pixel *, unsigned);

/* Read a .BMP file into the image structure */
/* Return TRUE if successful, FALSE on any error */
boolean
read_bmp_tiles(const char *filename, struct TileSetImage *image)
{
    struct BitmapHeader header1;
    struct BitmapInfoHeader header2;
    unsigned palette_size;
    size_t num_pixels, size;
    unsigned x, y, y_start, y_end, y_inc;
    unsigned bytes_per_row;

    FILE *fp = NULL; /* custodial */
    unsigned char *row_bytes = NULL; /* custodial */

    image->width = 0;
    image->height = 0;
    image->pixels = NULL;       /* custodial, returned */
    image->indexes = NULL;      /* custodial, returned */
    image->image_desc = NULL;   /* custodial, returned */
    image->tile_width = 0;
    image->tile_height = 0;

    fp = fopen(filename, "rb");
    if (fp == NULL) goto error;

    /* Read the headers */
    if (!read_header(fp, &header1)) goto error;
    if (memcmp(header1.magic, "BM", 2) != 0) goto error;

    if (!read_info_header(fp, &header2)) goto error;
    if (!check_info_header(&header2)) goto error;

    /* header2.Height < 0 means the Y coordinate is reversed; the origin is
     * top left rather than bottom left */
    image->width = header2.Width;
    image->height = labs(header2.Height);

    /* Allocate pixel area; watch out for overflow */
    num_pixels = (size_t) image->width * (size_t) image->height;
    if (num_pixels / image->width != image->height) goto error; /* overflow */
    size = num_pixels * sizeof(image->pixels[0]);
    if (size / sizeof(image->pixels[0]) != num_pixels) goto error; /* overflow */
    image->pixels = (struct Pixel *) alloc(size);
    if (header2.BitsPerPixel <= 8) {
        image->indexes = (unsigned char *) alloc(num_pixels);
    }

    /* Read the palette */
    palette_size = get_palette_size(&header2);
    if (!read_palette(fp, image->palette, palette_size)) goto error;

    /* Read the pixels */
    fseek(fp, header1.img_offset, SEEK_SET);
    if (header2.Height < 0) {
        y_start = 0;
        y_end = image->height;
        y_inc = 1;
    } else {
        y_start = image->height - 1;
        y_end = (unsigned) -1;
        y_inc = -1;
    }
    if (header2.Compression == BI_RLE4 || header2.Compression == BI_RLE8) {
        unsigned char *p;
        p = image->indexes;
        memset(p, 0, num_pixels);
        x = 0;
        y = image->height - 1;
        while (TRUE) {
            int b1, b2;
            b1 = fgetc(fp);
            if (b1 == EOF) goto error;
            b2 = fgetc(fp);
            if (b2 == EOF) goto error;
            /*
             * b1  b2
             *  0   0   end of line
             *  0   1   end of bitmap
             *  0   2   next two bytes are x and y offset
             *  0  >2   b2 is a count of bytes
             * >0  any  repeat b2, b1 times
             */
            if (b1 == 0) {
                if (b2 == 0) {
                    /* end of line */
                    --y;
                    x = 0;
                } else if (b2 == 1) {
                    /* end of bitmap */
                    break;
                } else if (b2 == 2) {
                    /* next two bytes are x and y offset */
                    b1 = fgetc(fp);
                    if (b1 == EOF) break;
                    b2 = fgetc(fp);
                    if (b2 == EOF) break;
                    x += b1;
                    y += b2;
                } else {
                    /* get bytes */
                    int i;
                    if (y < image->height) {
                        p = image->indexes + y * image->width;
                        for (i = 0; i < b2; ++i) {
                            b1 = fgetc(fp);
                            if (b1 == EOF) break;
                            if (header2.BitsPerPixel == 8) {
                                if (x < image->width) {
                                    p[x] = b1;
                                }
                                ++x;
                            } else {
                                if (x < image->width) {
                                    p[x] = b1 >> 4;
                                }
                                ++x;
                                if (x < image->width) {
                                    p[x] = b1 & 0xF;
                                }
                                ++x;
                            }
                        }
                        if (b2 & 1) {
                            b1 = fgetc(fp);
                            if (b1 == EOF) break;
                        }
                    }
                }
            } else {
                /* repeat b2, b1 times */
                int i;
                if (y < image->height) {
                    p = image->indexes + y * image->width;
                    for (i = 0; i < b1; ++i) {
                        if (header2.BitsPerPixel == 8) {
                            if (x < image->width) {
                                p[x] = b2;
                            }
                            ++x;
                        } else {
                            if (x < image->width) {
                                p[x] = b2 >> 4;
                            }
                            ++x;
                            if (x < image->width) {
                                p[x] = b2 & 0xF;
                            }
                            ++x;
                        }
                    }
                }
            }
        }
    } else {
        bytes_per_row = (image->width * header2.BitsPerPixel + 31) / 32 * 4;
        row_bytes = (unsigned char *) alloc(bytes_per_row);
        if (header2.Compression == BI_RGB) {
            switch (header2.BitsPerPixel) {
            case 16:
                header2.RedMask   = 0x001F;
                header2.GreenMask = 0x07E0;
                header2.BlueMask  = 0xF800;
                header2.AlphaMask = 0x0000;
                break;

            case 32:
                header2.RedMask   = 0x000000FF;
                header2.GreenMask = 0x0000FF00;
                header2.BlueMask  = 0x00FF0000;
                header2.AlphaMask = 0xFF000000;
                break;
            }
        }
        for (y = y_start; y != y_end; y += y_inc) {
            struct Pixel *row = image->pixels + y * image->width;
            unsigned char *ind = image->indexes + y * image->width;
            size = fread(row_bytes, 1, bytes_per_row, fp);
            if (size < bytes_per_row) goto error;
            switch (header2.BitsPerPixel) {
            case 1:
                for (x = 0; x < image->width; ++x) {
                    unsigned byte = x / 8;
                    unsigned shift = x % 8;
                    unsigned color = (row_bytes[byte] >> shift) & 1;
                    ind[x] = color;
                }
                break;
            case 4:
                for (x = 0; x < image->width; ++x) {
                    unsigned byte = x / 2;
                    unsigned shift = (x % 2) * 4;
                    unsigned color = (row_bytes[byte] >> shift) & 1;
                    ind[x] = color;
                }
                break;
            case 8:
                for (x = 0; x < image->width; ++x) {
                    ind[x] = row_bytes[x];
                }
                break;
            case 16:
                for (x = 0; x < image->width; ++x) {
                    uint16 color = read_u16(row_bytes + x * 2);
                    row[x] = build_pixel(&header2, color);
                }
                break;
            case 24:
                for (x = 0; x < image->width; ++x) {
                    row[x].r = row_bytes[x * 3 + 2];
                    row[x].g = row_bytes[x * 3 + 1];
                    row[x].b = row_bytes[x * 3 + 0];
                    row[x].a = 255;
                }
                break;
            case 32:
                for (x = 0; x < image->width; ++x) {
                    uint32 color = read_u32(row_bytes + x * 4);
                    row[x] = build_pixel(&header2, color);
                }
                break;
            }
        }
        free(row_bytes);
        row_bytes = NULL;
    }

    if (image->indexes != NULL) {
        size_t i;
        for (i = 0; i < num_pixels; ++i) {
            image->pixels[i] = image->palette[image->indexes[i]];
        }
    }

    fclose(fp);
    return TRUE;

error:
    if (fp) fclose(fp);
    free(row_bytes);
    free(image->pixels);
    image->pixels = NULL;
    free(image->indexes);
    image->indexes = NULL;
    free(image->image_desc);
    image->image_desc = NULL;
    return FALSE;
}

/* Read and decode the first header */
static boolean
read_header(FILE *fp, struct BitmapHeader *header)
{
    unsigned char buf[14];
    size_t size;

    size = fread(buf, 1, sizeof(buf), fp);
    if (size < sizeof(buf)) return FALSE;

    memcpy(header->magic, buf + 0, 2);
    header->bmp_size = read_u32(buf + 2);
    /* 6 and 8 are 16 bit integers giving the hotspot of a cursor */
    header->img_offset = read_u32(buf + 10);
    return TRUE;
}

/* Read and decode the second header */
static boolean
read_info_header(FILE *fp, struct BitmapInfoHeader *header)
{
    unsigned char buf[124]; /* maximum size */
    size_t size;
    boolean have_color_mask;

    memset(header, 0, sizeof(*header));

    /* Get the header size */
    size = fread(buf, 1, 4, fp);
    if (size < 4) return FALSE;
    header->Size = read_u32(buf + 0);
    if (header->Size > sizeof(buf)) return FALSE;

    /* Get the rest of the header */
    size = fread(buf + 4, 1, header->Size - 4, fp);
    if (size < header->Size - 4) return FALSE;

    have_color_mask = FALSE;
    switch (header->Size) {
    case 124: /* BITMAPV5INFOHEADER */
        /* 120 is reserved */
        header->ProfileSize = read_u32(buf + 116);
        header->ProfileData = read_u32(buf + 112);
        header->Intent = read_u32(buf + 108);
        /* fall through */

    case 108: /* BITMAPV4INFOHEADER */
        header->GammaBlue = read_u32(buf + 104);
        header->GammaGreen = read_u32(buf + 100);
        header->GammaRed = read_u32(buf + 96);
        header->Endpoints.ciexyzBlue.ciexyzZ = read_u32(buf + 92);
        header->Endpoints.ciexyzBlue.ciexyzY = read_u32(buf + 88);
        header->Endpoints.ciexyzBlue.ciexyzX = read_u32(buf + 84);
        header->Endpoints.ciexyzGreen.ciexyzZ = read_u32(buf + 80);
        header->Endpoints.ciexyzGreen.ciexyzY = read_u32(buf + 76);
        header->Endpoints.ciexyzGreen.ciexyzX = read_u32(buf + 72);
        header->Endpoints.ciexyzRed.ciexyzZ = read_u32(buf + 68);
        header->Endpoints.ciexyzRed.ciexyzY = read_u32(buf + 64);
        header->Endpoints.ciexyzRed.ciexyzX = read_u32(buf + 60);
        header->CSType = read_u32(buf + 56);
        /* fall through */

    case 56: /* BITMAPV3INFOHEADER */
        header->AlphaMask = read_u32(buf + 52);
        /* fall through */

    case 52: /* BITMAPV2INFOHEADER */
        header->BlueMask = read_u32(buf + 48);
        header->GreenMask = read_u32(buf + 44);
        header->RedMask = read_u32(buf + 40);
        have_color_mask = TRUE;
        /* fall through */

    case 40: /* BITMAPINFOHEADER */
    case 64: /* OS22XBITMAPHEADER */
        /* The last 24 bytes in OS22XBITMAPHEADER are incompatible with the
         * later Microsoft versions of the header */
        header->ColorsImportant = read_u32(buf + 36);
        header->ColorsUsed = read_u32(buf + 32);
        header->YResolution = read_s32(buf + 28);
        header->XResolution = read_s32(buf + 24);
        header->ImageDataSize = read_u32(buf + 20);
        header->Compression = read_u32(buf + 16);
        header->BitsPerPixel = read_u16(buf + 14);
        header->NumPlanes = read_u16(buf + 12);
        header->Height = read_s32(buf + 8);
        header->Width = read_s32(buf + 4);
        break;

    case 12: /* BITMAPCOREHEADER */
        header->BitsPerPixel = read_u16(buf + 10);
        header->NumPlanes = read_u16(buf + 8);
        header->Height = read_u16(buf + 6);
        header->Width = read_u16(buf + 4);
        break;

    default:
        return FALSE;
    }

    /* For BI_BITFIELDS, the next three 32 bit words are the color masks */
    if (header->Compression == BI_BITFIELDS && !have_color_mask) {
        size = fread(buf, 1, 12, fp);
        if (size < 12) return FALSE;
        header->RedMask = read_u32(buf + 0);
        header->GreenMask = read_u32(buf + 4);
        header->BlueMask = read_u32(buf + 8);
    }

    return TRUE;
}

/* Check the second header for consistency and unsupported features */
static boolean
check_info_header(const struct BitmapInfoHeader *header)
{
    if (header->NumPlanes != 1) return FALSE;
    switch (header->BitsPerPixel) {
#if 0 /* TODO */
    case 0:
        if (header->Compression != BI_PNG) return FALSE;
        /* JPEG not supported */
        break;
#endif

    case 1:
    case 24:
        if (header->Compression != BI_RGB) return FALSE;
        break;

    case 4:
        if (header->Compression != BI_RGB
        &&  header->Compression != BI_RLE4) return FALSE;
        break;

    case 8:
        if (header->Compression != BI_RGB
        &&  header->Compression != BI_RLE8) return FALSE;
        break;

    case 16:
    case 32:
        if (header->Compression != BI_RGB
        &&  header->Compression != BI_BITFIELDS) return FALSE;
        /* Any of the color masks could conceivably be zero; the bitmap, though
         * limited, would still be meaningful */
        if (header->Compression == BI_BITFIELDS
        &&  header->RedMask == 0
        &&  header->GreenMask == 0
        &&  header->BlueMask == 0) return FALSE;
        break;

    default:
        return FALSE;
    }

    if (header->Height < 0 && header->Compression != BI_RGB
            && header->Compression != BI_BITFIELDS) return FALSE;

    return TRUE;
}

/* Return the number of palette entries to read from the file */
static unsigned
get_palette_size(const struct BitmapInfoHeader *header)
{
    switch (header->BitsPerPixel) {
    case 1:
        return 2;

    case 4:
        return header->ColorsUsed ? header->ColorsUsed : 16;

    case 8:
        return header->ColorsUsed ? header->ColorsUsed : 256;

    default:
        return 0;
    }
}

/*
 * Read the palette from the file
 * palette_size is the number of entries to read, but no more than 256 will
 * be written into the palette array
 * Return TRUE if successful, FALSE on any error
 */
static boolean
read_palette(FILE *fp, struct Pixel *palette, unsigned palette_size)
{
    unsigned i;
    unsigned char buf[4];
    unsigned read_size;

    read_size = (palette_size < 256) ? palette_size : 256;
    for (i = 0; i < read_size; ++i) {
        size_t size = fread(buf, 1, sizeof(buf), fp);
        if (size < sizeof(buf)) return FALSE;
        palette[i].b = buf[0];
        palette[i].g = buf[1];
        palette[i].r = buf[2];
        palette[i].a = 255;
    }
    for (; i < 256; ++i) {
        palette[i].b = 0;
        palette[i].g = 0;
        palette[i].r = 0;
        palette[i].a = 255;
    }
    fseek(fp, 4 * (palette_size - read_size), SEEK_CUR);
    return TRUE;
}

/* Decode an unsigned 16 bit quantity */
static uint16
read_u16(const unsigned char buf[2])
{
    return ((uint16)buf[0] << 0)
         | ((uint16)buf[1] << 8);
}

/* Decode an unsigned 32 bit quantity */
static uint32
read_u32(const unsigned char buf[4])
{
    return ((uint32)buf[0] <<  0)
         | ((uint32)buf[1] <<  8)
         | ((uint32)buf[2] << 16)
         | ((uint32)buf[3] << 24);
}

/* Decode a signed 32 bit quantity */
static int32
read_s32(const unsigned char buf[4])
{
    return (int32)((read_u32(buf) ^ 0x80000000) - 0x80000000);
}

/* Build a pixel structure, given the mask words in the second header and
 * a packed 16 or 32 bit pixel */
static struct Pixel
build_pixel(const struct BitmapInfoHeader *header, uint32 color)
{
    struct Pixel pixel;

    pixel.r = pixel_element(header->RedMask, color);
    pixel.g = pixel_element(header->GreenMask, color);
    pixel.b = pixel_element(header->BlueMask, color);
    pixel.a = header->AlphaMask ? pixel_element(header->AlphaMask, color) : 255;
    return pixel;
}

/* Extract one element (red, green, blue or alpha) from a pixel */
static unsigned char
pixel_element(uint32 mask, uint32 color)
{
    uint32 bits, shift;

    if (mask == 0) return 0;
    bits = 0xFFFF; /* 0xFF, 0xF, 0x3, 0x1 */
    shift = 16;    /*    8,   4,   2,   1 */
    while (shift != 0) {
        if ((mask & bits) == 0) {
            mask >>= shift;
            color >>= shift;
        }
        shift /= 2;
        bits >>= shift;
    }
    color &= mask;
    return color * 255 / mask;
}

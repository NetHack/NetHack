/*
 * bmp2img_host.c - Convert a BMP tileset to Atari XIMG.
 * Copyright (c) 2026 by Ingo Paschke.
 * NetHack may be freely redistributed.  See license for details.
 *
 * Uses the same colour-reduction algorithm as the Amiga bmp2iff_host.c:
 *   - First 16 palette slots are reserved base colours.
 *   - Remaining slots are filled by most-frequent tile colours.
 *   - Unused BMP entries are mapped to nearest colour in the palette.
 *
 * Usage: bmp2img_host -planes N input.bmp output.img
 *
 * This is a HOST tool -- runs on the build machine (Linux/Mac/etc.).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define TILE_X 16
#define TILE_Y 16

#pragma pack(push,1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1, bfReserved2;
    uint32_t bfOffBits;
} BMPFILEHEADER;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth, biHeight;
    uint16_t biPlanes, biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter, biYPelsPerMeter;
    uint32_t biClrUsed, biClrImportant;
} BMPINFOHEADER;
#pragma pack(pop)

typedef struct {
    uint8_t r, g, b;
} RGB;

/* --------------------------------------------------------- */
/*  Colour helpers (same as Amiga bmp2iff_host.c)            */
/* --------------------------------------------------------- */

static int
coldist(const RGB *a, const RGB *b)
{
    int dr = a->r - b->r;
    int dg = a->g - b->g;
    int db = a->b - b->b;
    return dr*dr + dg*dg + db*db;
}

static int
nearest(const RGB *c, const RGB *pal, int n)
{
    int best = 0, bestd = 0x7fffffff, i;
    for (i = 0; i < n; i++) {
        int d = coldist(c, &pal[i]);
        if (d < bestd) { bestd = d; best = i; }
    }
    return best;
}

/* --------------------------------------------------------- */
/*  XIMG output                                              */
/* --------------------------------------------------------- */

/* Write big-endian 16-bit word */
static void
wr16(FILE *f, uint16_t v)
{
    uint8_t b[2] = { v >> 8, v & 0xff };
    fwrite(b, 1, 2, f);
}

/* Read little-endian 16-bit word (BMP byte order) */
static unsigned short
rd_le16(FILE *fp)
{
    int b0 = fgetc(fp);
    int b1 = fgetc(fp);
    return (unsigned short) ((b1 << 8) | b0);
}

/* Read little-endian 32-bit word (BMP byte order) */
static unsigned int
rd_le32(FILE *fp)
{
    int b0 = fgetc(fp);
    int b1 = fgetc(fp);
    int b2 = fgetc(fp);
    int b3 = fgetc(fp);
    return ((unsigned int) b3 << 24) | ((unsigned int) b2 << 16)
         | ((unsigned int) b1 <<  8) |  (unsigned int) b0;
}

/*
 * Write an XIMG file.
 * Pixel data is stored uncompressed (literal byte strings per plane per line).
 * The XIMG palette uses VDI RGB values in the range 0-1000.
 */
static void
ximg_write(FILE *fp, int nplanes, int ncolors,
           const RGB *pal, int w, int h,
           const uint8_t *pixdata)
{
    int rb = (w + 7) / 8;   /* bytes per plane per line (before word-align) */
    int rbw = (rb + 1) & ~1; /* word-aligned row bytes */
    int headlen;
    int x, y, p, i;

    /* XIMG header length in words:
       8 words standard IMG header + 2 words XIMG magic
       + 1 word palette type + ncolors * 3 words RGB data */
    headlen = 8 + 3 + ncolors * 3;

    /* Standard IMG header */
    wr16(fp, 1);           /* version */
    wr16(fp, headlen);     /* header length in words */
    wr16(fp, nplanes);     /* number of planes */
    wr16(fp, 2);           /* pattern length (bytes) */
    wr16(fp, 372);         /* pixel width in microns (approx 72 dpi) */
    wr16(fp, 372);         /* pixel height in microns */
    wr16(fp, w);           /* image width in pixels */
    wr16(fp, h);           /* image height in pixels */

    /* XIMG extension */
    wr16(fp, 0x5849);      /* 'XI' - XIMG magic high */
    wr16(fp, 0x4D47);      /* 'MG' - XIMG magic low */
    wr16(fp, 0x0000);      /* palette type: 0 = RGB */

    /* Palette in VDI format: RGB 0-1000 per entry */
    for (i = 0; i < ncolors; i++) {
        wr16(fp, (uint16_t)(pal[i].r * 1000 / 255));
        wr16(fp, (uint16_t)(pal[i].g * 1000 / 255));
        wr16(fp, (uint16_t)(pal[i].b * 1000 / 255));
    }

    /* Bitmap data: for each scan line, for each plane, write a
       "literal byte string" record (opcode 0x80, byte count, raw
       bytes), chunked into runs of <= 255 bytes. */
    {
        uint8_t *planebuf = malloc(rbw);
        if (!planebuf) {
            fprintf(stderr, "ximg_write: out of memory (rbw=%d)\n", rbw);
            exit(1);
        }
        for (y = 0; y < h; y++) {
            for (p = 0; p < nplanes; p++) {
                memset(planebuf, 0, rbw);

                /* extract plane p for this scan line */
                for (x = 0; x < w; x++) {
                    uint8_t v = pixdata[y * w + x];
                    if (v & (1 << p))
                        planebuf[x / 8] |= (0x80 >> (x & 7));
                }

                {
                    int left = rbw;
                    uint8_t *src = planebuf;
                    while (left > 0) {
                        int chunk = left > 255 ? 255 : left;
                        fputc(0x80, fp);        /* literal opcode */
                        fputc(chunk, fp);       /* byte count */
                        fwrite(src, 1, chunk, fp);
                        src += chunk;
                        left -= chunk;
                    }
                }
            }
        }
        free(planebuf);
    }
}

/* --------------------------------------------------------- */
/*  Palette building (adapted from Amiga bmp2iff_host.c)     */
/* --------------------------------------------------------- */

/*
 * Build an output palette and pixel remap table.
 *
 * Algorithm (matches the 3.6.7 runtime reorder_tile_palette,
 * but done on the host so the Atari doesn't burn 30 seconds):
 *
 *  1. Collect the 'maxcol' most-used BMP colours.
 *  2. For each VDI pen, greedily pick the tile colour closest
 *     to that pen's default ST colour.  This keeps GEM UI
 *     elements (which use fixed VDI pen numbers) looking right.
 *  3. Write the XIMG palette in DEVICE order (the format that
 *     load_img.c / img_set_colors expects).
 *  4. Map BMP colours to DEVICE indices for the bitplane data.
 *
 * out[0..maxcol-1]:     XIMG palette in DEVICE order (RGB 0-255)
 * remap[0..nsrc-1]:     BMP palette index -> device pixel index
 */
static void
build_palette(const RGB *src, int nsrc,
              const uint8_t *pix, int npix,
              int maxcol,
              RGB *out, int *remap)
{
    int freq[256] = {0};
    int order[256];
    int i, j;

    /* count pixel frequency per BMP palette entry */
    for (i = 0; i < npix; i++)
        freq[pix[i]]++;

    /* sort BMP colours by frequency (descending) */
    for (i = 0; i < nsrc; i++) order[i] = i;
    for (i = 1; i < nsrc; i++) {
        int k = order[i], kf = freq[k];
        j = i - 1;
        while (j >= 0 && freq[order[j]] < kf) {
            order[j+1] = order[j];
            j--;
        }
        order[j+1] = k;
    }

    /*
     * Colour selection:
     *   For <= 16 colours:
     *     Slot 0 = white, slot maxcol-1 = black,
     *     slots 1..maxcol-2 = most-used tile colours.
     *   For > 16 colours (256-colour mode):
     *     Slots 0-15 = reserved for system (zeroed, runtime keeps them),
     *     slots 16..maxcol-2 = tile colours, slot maxcol-1 = black.
     *
     * The Atari runtime (reorder_tile_palette + img_set_colors)
     * handles VDI/device remapping for the actual hardware.
     */
    int num_out;
    int first_tile_slot;

    memset(out, 0, maxcol * sizeof(RGB));
    if (maxcol > 16) {
        /* 256-colour: reserve system slots 0-15 */
        first_tile_slot = 16;
        out[maxcol - 1].r = 0; out[maxcol - 1].g = 0; out[maxcol - 1].b = 0;
    } else {
        /* 16 or fewer: slot 0 = white, last = black */
        first_tile_slot = 1;
        out[0].r = 255; out[0].g = 255; out[0].b = 255;
        out[maxcol - 1].r = 0; out[maxcol - 1].g = 0; out[maxcol - 1].b = 0;
    }
    num_out = first_tile_slot;

    for (i = 0; i < nsrc && num_out < maxcol - 1; i++) {
        int idx = order[i];
        int r = src[idx].r, g = src[idx].g, b = src[idx].b;
        if (freq[idx] == 0) continue;
        if (r >= 250 && g >= 250 && b >= 250) continue;
        if (r <= 5 && g <= 5 && b <= 5) continue;
        {
            int dup = 0;
            for (j = first_tile_slot; j < num_out; j++)
                if (out[j].r == r && out[j].g == g && out[j].b == b)
                    { dup = 1; break; }
            if (!dup && out[maxcol-1].r == r && out[maxcol-1].g == g
                && out[maxcol-1].b == b)
                dup = 1;
            if (dup) continue;
        }
        out[num_out] = src[idx];
        num_out++;
    }
    for (i = num_out; i < maxcol - 1; i++)
        out[i] = out[maxcol - 1];

    for (i = 0; i < nsrc; i++)
        remap[i] = nearest(&src[i], out + first_tile_slot,
                           maxcol - first_tile_slot) + first_tile_slot;
}

/* --------------------------------------------------------- */
/*  Floyd-Steinberg dithering (tile-aware)                   */
/* --------------------------------------------------------- */

static int
clamp(int v)
{
    return v < 0 ? 0 : v > 255 ? 255 : v;
}

/*
 * Apply Floyd-Steinberg error-diffusion dithering.
 * Operates on full RGB pixel data, tile-by-tile so that
 * quantisation error does not bleed across tile boundaries.
 *
 * rgb[y * w + x] = input/output RGB pixel (modified in place).
 * pal/npal       = output palette to dither against.
 * result[y*w+x]  = output palette index per pixel.
 */
static void
dither_fs(RGB *rgb, int w, int h,
          const RGB *pal, int npal,
          uint8_t *result)
{
    int tw = TILE_X, th = TILE_Y;
    int tx, ty, x, y;

    memset(result, 0, w * h);

    for (ty = 0; ty < h; ty += th) {
        int bh = (ty + th <= h) ? th : h - ty;
        for (tx = 0; tx < w; tx += tw) {
            int bw = (tx + tw <= w) ? tw : w - tx;

            for (y = 0; y < bh; y++) {
                for (x = 0; x < bw; x++) {
                    int px = tx + x, py = ty + y;
                    int idx = py * w + px;
                    RGB old = rgb[idx];
                    int ci = nearest(&old, pal, npal);
                    RGB new = pal[ci];
                    int er = old.r - new.r;
                    int eg = old.g - new.g;
                    int eb = old.b - new.b;

                    result[idx] = (uint8_t)ci;

                    /* distribute error to neighbours within tile */
                    #define DIFFUSE(dx, dy, frac) do { \
                        int nx = x+(dx), ny = y+(dy); \
                        if (nx >= 0 && nx < bw && ny >= 0 && ny < bh) { \
                            int ni = (ty+ny)*w + (tx+nx); \
                            rgb[ni].r = clamp(rgb[ni].r + er*(frac)/16); \
                            rgb[ni].g = clamp(rgb[ni].g + eg*(frac)/16); \
                            rgb[ni].b = clamp(rgb[ni].b + eb*(frac)/16); \
                        } \
                    } while(0)

                    DIFFUSE( 1, 0, 7);
                    DIFFUSE(-1, 1, 3);
                    DIFFUSE( 0, 1, 5);
                    DIFFUSE( 1, 1, 1);
                    #undef DIFFUSE
                }
            }
        }
    }
}

/* --------------------------------------------------------- */
/*  Ordered (Bayer) dithering                                */
/* --------------------------------------------------------- */

/* 4x4 Bayer threshold matrix, scaled 0-255.
   Produces 17 distinct grey levels — much cleaner than
   Floyd-Steinberg at small tile sizes. */
static const int bayer4[4][4] = {
    {  15, 143,  47, 175 },
    { 207,  79, 239, 111 },
    {  63, 191,  31, 159 },
    { 255, 127, 223,  95 }
};

/*
 * Apply ordered (Bayer) dithering.
 * For each pixel, compare its luminance against the Bayer
 * threshold at that position (modulo 4x4) to decide which
 * palette entry to use.
 */
static void
dither_ordered(const RGB *src_pal,
               const uint8_t *indices, int w, int h,
               const RGB *pal, int npal,
               uint8_t *result)
{
    int x, y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int idx = y * w + x;
            RGB c = src_pal[indices[idx]];
            int threshold = bayer4[y & 3][x & 3];

            /* Bias the colour by the Bayer threshold before
               finding the nearest palette entry.  This shifts
               mid-tones towards black or white depending on
               position, creating a regular dither pattern. */
            RGB biased;
            biased.r = clamp(c.r + (threshold - 128) * 2 / 3);
            biased.g = clamp(c.g + (threshold - 128) * 2 / 3);
            biased.b = clamp(c.b + (threshold - 128) * 2 / 3);
            result[idx] = (uint8_t)nearest(&biased, pal, npal);
        }
    }
}

/* --------------------------------------------------------- */
/*  Atkinson dithering (tile-aware)                          */
/* --------------------------------------------------------- */

/*
 * Atkinson dithering — designed for the original Macintosh.
 * Only 3/4 of the quantisation error is diffused (1/4 is
 * discarded), so it preserves more contrast than Floyd-Steinberg.
 * Light areas stay light, dark stays dark.  Very readable at
 * small tile sizes.
 *
 * Error distribution pattern (each 1/8 of error):
 *         *  1  1
 *      1  1  1
 *         1
 */
static void
dither_atkinson(RGB *rgb, int w, int h,
                const RGB *pal, int npal,
                uint8_t *result)
{
    int tw = TILE_X, th = TILE_Y;
    int tx, ty, x, y;

    memset(result, 0, w * h);

    for (ty = 0; ty < h; ty += th) {
        int bh = (ty + th <= h) ? th : h - ty;
        for (tx = 0; tx < w; tx += tw) {
            int bw = (tx + tw <= w) ? tw : w - tx;

            for (y = 0; y < bh; y++) {
                for (x = 0; x < bw; x++) {
                    int px = tx + x, py = ty + y;
                    int idx = py * w + px;
                    RGB old = rgb[idx];
                    int ci = nearest(&old, pal, npal);
                    RGB new = pal[ci];
                    /* only 6/8 = 3/4 of error is distributed */
                    int er = (old.r - new.r) / 8;
                    int eg = (old.g - new.g) / 8;
                    int eb = (old.b - new.b) / 8;

                    result[idx] = (uint8_t)ci;

                    #define ATK(dx, dy) do { \
                        int nx = x+(dx), ny = y+(dy); \
                        if (nx >= 0 && nx < bw && ny >= 0 && ny < bh) { \
                            int ni = (ty+ny)*w + (tx+nx); \
                            rgb[ni].r = clamp(rgb[ni].r + er); \
                            rgb[ni].g = clamp(rgb[ni].g + eg); \
                            rgb[ni].b = clamp(rgb[ni].b + eb); \
                        } \
                    } while(0)

                    ATK( 1, 0);
                    ATK( 2, 0);
                    ATK(-1, 1);
                    ATK( 0, 1);
                    ATK( 1, 1);
                    ATK( 0, 2);
                    #undef ATK
                }
            }
        }
    }
}

/* --------------------------------------------------------- */
/*  Main                                                     */
/* --------------------------------------------------------- */

int
main(int argc, char **argv)
{
    FILE *bmpfp, *outfp;
    BMPFILEHEADER fhdr;
    BMPINFOHEADER ihdr;
    RGB palette[256];
    int ncolors, img_w, img_h, rowstride;
    uint8_t *bmpdata, *pixels;
    int nplanes, maxcol;
    int i, y;
    RGB outpal[256];
    int remap[256];
    uint8_t *remapped;
    int use_dither = 0; /* 0=none, 1=Floyd-Steinberg, 2=ordered/Bayer */
    int argi = 1;

    /* parse args */
    if (argi < argc && strcmp(argv[argi], "-dither") == 0) {
        use_dither = 1; /* default: Floyd-Steinberg */
        argi++;
    } else if (argi < argc && strcmp(argv[argi], "-ordered") == 0) {
        use_dither = 2; /* ordered/Bayer dithering */
        argi++;
    } else if (argi < argc && strcmp(argv[argi], "-atkinson") == 0) {
        use_dither = 3; /* Atkinson dithering */
        argi++;
    }
    if (argc - argi != 4
        || strcmp(argv[argi], "-planes") != 0) {
        fprintf(stderr,
                "Usage: %s [-dither|-ordered|-atkinson] -planes N input.bmp output.img\n",
                argv[0]);
        return 1;
    }
    nplanes = atoi(argv[argi + 1]);
    if (nplanes < 1 || nplanes > 8) {
        fprintf(stderr, "planes must be 1-8\n");
        return 1;
    }
    maxcol = 1 << nplanes;

    /* read BMP */
    bmpfp = fopen(argv[argi + 2], "rb");
    if (!bmpfp) { perror(argv[argi + 2]); return 1; }

    /* BMP files are little-endian; read field-by-field for portability. */
    fhdr.bfType      = rd_le16(bmpfp);
    fhdr.bfSize      = rd_le32(bmpfp);
    fhdr.bfReserved1 = rd_le16(bmpfp);
    fhdr.bfReserved2 = rd_le16(bmpfp);
    fhdr.bfOffBits   = rd_le32(bmpfp);
    ihdr.biSize          = rd_le32(bmpfp);
    ihdr.biWidth         = (int32_t) rd_le32(bmpfp);
    ihdr.biHeight        = (int32_t) rd_le32(bmpfp);
    ihdr.biPlanes        = rd_le16(bmpfp);
    ihdr.biBitCount      = rd_le16(bmpfp);
    ihdr.biCompression   = rd_le32(bmpfp);
    ihdr.biSizeImage     = rd_le32(bmpfp);
    ihdr.biXPelsPerMeter = (int32_t) rd_le32(bmpfp);
    ihdr.biYPelsPerMeter = (int32_t) rd_le32(bmpfp);
    ihdr.biClrUsed       = rd_le32(bmpfp);
    ihdr.biClrImportant  = rd_le32(bmpfp);
    if (feof(bmpfp) || ferror(bmpfp)) {
        fprintf(stderr, "Failed to read BMP header\n");
        return 1;
    }
    if (fhdr.bfType != 0x4D42) {
        fprintf(stderr, "Not a BMP file\n");
        return 1;
    }
    if (ihdr.biBitCount != 8) {
        fprintf(stderr,
                "Expected 8-bit BMP, got %d-bit\n",
                ihdr.biBitCount);
        return 1;
    }

    img_w = ihdr.biWidth;
    img_h = abs(ihdr.biHeight);
    ncolors = ihdr.biClrUsed ? ihdr.biClrUsed : 256;
    if (ncolors > 256) ncolors = 256;

    /* read palette (BMP stores BGRx) */
    {
        uint8_t raw[256][4];
        if (fread(raw, 4, ncolors, bmpfp)
            != (size_t)ncolors) {
            fprintf(stderr, "Failed to read palette\n");
            return 1;
        }
        for (i = 0; i < ncolors; i++) {
            palette[i].r = raw[i][2];
            palette[i].g = raw[i][1];
            palette[i].b = raw[i][0];
        }
    }

    /* read pixel data */
    rowstride = (img_w + 3) & ~3;
    bmpdata = malloc(rowstride * img_h);
    fseek(bmpfp, fhdr.bfOffBits, SEEK_SET);
    if (fread(bmpdata, 1, rowstride * img_h, bmpfp)
        != (size_t)(rowstride * img_h)) {
        fprintf(stderr, "Failed to read pixel data\n");
        return 1;
    }
    fclose(bmpfp);

    /* flip bottom-up to top-down */
    pixels = malloc(img_w * img_h);
    if (ihdr.biHeight > 0) {
        for (y = 0; y < img_h; y++)
            memcpy(pixels + y * img_w,
                   bmpdata + (img_h-1-y) * rowstride,
                   img_w);
    } else {
        for (y = 0; y < img_h; y++)
            memcpy(pixels + y * img_w,
                   bmpdata + y * rowstride, img_w);
    }
    free(bmpdata);

    /* Pad image dimensions to tile multiples */
    {
        int pad_w = (img_w + TILE_X - 1) / TILE_X * TILE_X;
        int pad_h = (img_h + TILE_Y - 1) / TILE_Y * TILE_Y;
        if (pad_w != img_w || pad_h != img_h) {
            uint8_t *padded = calloc(pad_w * pad_h, 1);
            for (y = 0; y < img_h; y++)
                memcpy(padded + y * pad_w, pixels + y * img_w, img_w);
            free(pixels);
            pixels = padded;
            img_w = pad_w;
            img_h = pad_h;
        }
    }

    /* build palette and remap pixels */
    build_palette(palette, ncolors,
                  pixels, img_w * img_h,
                  maxcol, outpal, remap);

    remapped = malloc(img_w * img_h);
    if (use_dither == 1) {
        /* Floyd-Steinberg error diffusion */
        RGB *rgbpix = malloc(img_w * img_h * sizeof(RGB));
        for (i = 0; i < img_w * img_h; i++)
            rgbpix[i] = palette[pixels[i]];
        dither_fs(rgbpix, img_w, img_h,
                  outpal, maxcol, remapped);
        free(rgbpix);
        free(pixels);
    } else if (use_dither == 2) {
        /* Ordered (Bayer) dithering */
        dither_ordered(palette, pixels, img_w, img_h,
                       outpal, maxcol, remapped);
        free(pixels);
    } else if (use_dither == 3) {
        /* Atkinson dithering */
        RGB *rgbpix = malloc(img_w * img_h * sizeof(RGB));
        for (i = 0; i < img_w * img_h; i++)
            rgbpix[i] = palette[pixels[i]];
        dither_atkinson(rgbpix, img_w, img_h,
                        outpal, maxcol, remapped);
        free(rgbpix);
        free(pixels);
    } else {
        for (i = 0; i < img_w * img_h; i++)
            remapped[i] = (uint8_t)remap[pixels[i]];
        free(pixels);
    }

    /* write XIMG */
    outfp = fopen(argv[argi + 3], "wb");
    if (!outfp) { perror(argv[argi + 3]); return 1; }

    ximg_write(outfp, nplanes, maxcol,
               outpal, img_w, img_h, remapped);
    fclose(outfp);

    printf("%s: %dx%d, %d colours (%d planes)%s\n",
           argv[argi + 3], img_w, img_h, maxcol, nplanes,
           use_dither == 1 ? " [Floyd-Steinberg]" :
           use_dither == 2 ? " [ordered]" :
           use_dither == 3 ? " [Atkinson]" : "");

    free(remapped);
    return 0;
}

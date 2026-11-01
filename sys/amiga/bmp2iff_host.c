/*
 * bmp2iff_host.c - Convert a BMP to Amiga BMAP IFF.
 * Copyright (c) 2026 by Ingo Paschke.
 * NetHack may be freely redistributed.  See license for details.
 *
 * IFF BMAP format matches sys/amiga/xpm2iff.c by Gregg Wonderly.
 *
 * Usage: bmp2iff_host -planes N input.bmp output.iff
 *
 * This is a HOST tool -- runs on the build machine.
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
/*  Fixed 16-color AMIV UI palette                          */
/*  Must match amiv_init_map[] in winami.c                   */
/* --------------------------------------------------------- */

static const RGB amiv_pal[16] = {
    {0x00,0x00,0x00}, /*  0 black    */
    {0xFF,0xFF,0xFF}, /*  1 white    */
    {0x00,0xBB,0xFF}, /*  2 cyan     */
    {0xFF,0x66,0x00}, /*  3 orange   */
    {0x00,0x00,0xFF}, /*  4 blue     */
    {0x00,0x99,0x00}, /*  5 green    */
    {0x66,0x99,0xBB}, /*  6 grey     */
    {0xFF,0x00,0x00}, /*  7 red      */
    {0x66,0xFF,0x00}, /*  8 ltgreen  */
    {0xFF,0xFF,0x00}, /*  9 yellow   */
    {0xFF,0x00,0xFF}, /* 10 magenta  */
    {0x99,0x44,0x00}, /* 11 brown    */
    {0x44,0x66,0x66}, /* 12 greyblue */
    {0xCC,0x44,0x00}, /* 13 ltbrown  */
    {0xDD,0xDD,0xBB}, /* 14 ltgrey   */
    {0xFF,0xBB,0x99}, /* 15 peach    */
};

/* --------------------------------------------------------- */
/*  Colour helpers                                           */
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
/*  IFF output                                               */
/* --------------------------------------------------------- */

static FILE *iff_fp;

static void
wr32(uint32_t v)
{
    uint8_t b[4] = { v>>24, v>>16, v>>8, v };
    fwrite(b, 1, 4, iff_fp);
}

static void
wr_chunk(const char *id, const void *d, uint32_t len)
{
    fwrite(id, 1, 4, iff_fp);
    wr32(len);
    fwrite(d, 1, len, iff_fp);
    if (len & 1) fputc(0, iff_fp);
}

static void
iff_write(int nplanes, int ncolors, uint8_t *cmap,
          int w, int h, uint8_t **planes,
          int ntiles, int across, int down)
{
    long spos;
    uint32_t plsz = (uint32_t)(w / 8) * h;
    int i;

    fwrite("FORM", 1, 4, iff_fp);
    spos = ftell(iff_fp);
    wr32(0);
    fwrite("BMAP", 1, 4, iff_fp);

    /* BMHD */
    {
        uint8_t bm[20];
        memset(bm, 0, 20);
        bm[0] = w >> 8;  bm[1] = w;
        bm[2] = h >> 8;  bm[3] = h;
        bm[8] = (uint8_t)nplanes;
        bm[14] = 100; bm[15] = 100;
        wr_chunk("BMHD", bm, 20);
    }

    /* CAMG */
    {
        uint8_t c[4] = {0,0,0x80,0x04};
        wr_chunk("CAMG", c, 4);
    }

    /* CMAP */
    wr_chunk("CMAP", cmap, (uint32_t)ncolors * 3);

    /* PDAT */
    {
        uint8_t pd[28], *p = pd;
        uint32_t v[7];
        v[0] = nplanes;
        v[1] = plsz;
        v[2] = across;
        v[3] = down;
        v[4] = ntiles;
        v[5] = TILE_X;
        v[6] = TILE_Y;
        for (i = 0; i < 7; i++) {
            p[0] = (v[i]>>24); p[1] = (v[i]>>16);
            p[2] = (v[i]>> 8); p[3] =  v[i];
            p += 4;
        }
        wr_chunk("PDAT", pd, 28);
    }

    /* PLNE */
    fwrite("PLNE", 1, 4, iff_fp);
    wr32(plsz * nplanes);
    for (i = 0; i < nplanes; i++)
        fwrite(planes[i], 1, plsz, iff_fp);

    /* fix FORM size */
    {
        long end = ftell(iff_fp);
        uint32_t sz = (uint32_t)(end - spos - 4);
        fseek(iff_fp, spos, SEEK_SET);
        wr32(sz);
        fseek(iff_fp, 0, SEEK_END);
    }
}

/* --------------------------------------------------------- */
/*  Pixel-to-bitplane conversion                             */
/* --------------------------------------------------------- */

static void
to_planes(uint8_t *pix, int w, int h,
          int np, uint8_t **pl)
{
    int rb = w / 8;
    int x, y, p;

    for (p = 0; p < np; p++)
        memset(pl[p], 0, rb * h);

    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            uint8_t v = pix[y * w + x];
            int off = y * rb + x / 8;
            int bit = 7 - (x & 7);
            for (p = 0; p < np; p++)
                if (v & (1 << p))
                    pl[p][off] |= (1 << bit);
        }
}

/* --------------------------------------------------------- */
/*  Palette building                                         */
/* --------------------------------------------------------- */

/*
 * Build an output palette of 'maxcol' entries:
 *   - slots 0-15:  fixed AMIV UI colors
 *   - slots 16+:   tile colors from the BMP
 *
 * Returns remap[0..nsrc-1] mapping BMP palette index
 * to output palette index.
 */
static void
build_palette(const RGB *src, int nsrc,
              const uint8_t *pix, int npix,
              int maxcol,
              RGB *out, int *remap)
{
    int freq[256] = {0};
    int order[256];
    int i, j, nfree, nuniq;

    /* count pixel frequency per BMP palette entry */
    for (i = 0; i < npix; i++)
        freq[pix[i]]++;

    /* sort BMP colors by frequency (descending) */
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

    /* count unique colors actually used */
    nuniq = 0;
    for (i = 0; i < nsrc; i++)
        if (freq[i] > 0) nuniq++;

    /* first 16 slots are AMIV UI */
    for (i = 0; i < 16 && i < maxcol; i++)
        out[i] = amiv_pal[i];
    for (i = 16; i < maxcol; i++)
        memset(&out[i], 0, sizeof(RGB));

    nfree = maxcol - 16;
    if (nfree < 0) nfree = 0;

    /*
     * Case 1: enough free slots for all unique colors.
     * Assign each unique BMP color its own pen, exact
     * AMIV matches share the UI pen.
     */
    if (nuniq <= nfree) {
        int next = 16;
        for (i = 0; i < nsrc; i++) {
            if (freq[i] == 0) {
                remap[i] = 0;
                continue;
            }
            /* exact match to an AMIV pen? */
            int best = nearest(&src[i], amiv_pal, 16);
            if (coldist(&src[i], &amiv_pal[best]) == 0) {
                remap[i] = best;
            } else {
                remap[i] = next;
                out[next] = src[i];
                next++;
            }
        }
        return;
    }

    /*
     * Case 2: more unique colors than free slots.
     * - Direct/near AMIV matches use UI pens.
     * - Remaining slots filled by most-frequent colors.
     * - Leftovers mapped to nearest in final palette.
     */
    {
        int next = 16;
        int assigned[256];
        memset(assigned, 0, sizeof(assigned));

        /* pass 1: exact/near AMIV matches */
        for (i = 0; i < nsrc; i++) {
            if (freq[i] == 0) {
                remap[i] = 0;
                assigned[i] = 1;
                continue;
            }
            int best = nearest(&src[i], amiv_pal, 16);
            int d = coldist(&src[i], &amiv_pal[best]);
            if (d < 200) {  /* near match threshold */
                remap[i] = best;
                assigned[i] = 1;
            }
        }

        /* pass 2: fill free slots with most-frequent
         * unassigned colors (order[] is freq-sorted) */
        for (i = 0; i < nsrc && next < maxcol; i++) {
            int idx = order[i];
            if (assigned[idx] || freq[idx] == 0)
                continue;
            remap[idx] = next;
            out[next] = src[idx];
            assigned[idx] = 1;
            next++;
        }

        /* pass 3: map remaining to nearest in palette */
        for (i = 0; i < nsrc; i++) {
            if (!assigned[i])
                remap[i] = nearest(&src[i], out, next);
        }
    }
}

/* --------------------------------------------------------- */
/*  Main                                                     */
/* --------------------------------------------------------- */

int
main(int argc, char **argv)
{
    FILE *bmpfp;
    BMPFILEHEADER fhdr;
    BMPINFOHEADER ihdr;
    RGB palette[256];
    int ncolors, img_w, img_h, rowstride;
    uint8_t *bmpdata, *pixels;
    int ntiles, across, down;
    int nplanes, maxcol;
    int i, y;
    RGB outpal[256];
    int remap[256];
    uint8_t *remapped;
    uint8_t *plane_data[8];
    uint8_t cmap_rgb[256 * 3];
    int planesize;

    /* parse args */
    if (argc != 5
        || strcmp(argv[1], "-planes") != 0) {
        fprintf(stderr,
                "Usage: %s -planes N input.bmp output.iff\n",
                argv[0]);
        return 1;
    }
    nplanes = atoi(argv[2]);
    if (nplanes < 1 || nplanes > 8) {
        fprintf(stderr, "planes must be 1-8\n");
        return 1;
    }
    maxcol = 1 << nplanes;

    /* read BMP */
    bmpfp = fopen(argv[3], "rb");
    if (!bmpfp) { perror(argv[3]); return 1; }

    if (fread(&fhdr, sizeof(fhdr), 1, bmpfp) != 1
        || fread(&ihdr, sizeof(ihdr), 1, bmpfp) != 1) {
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

    across = img_w / TILE_X;
    down   = img_h / TILE_Y;
    ntiles = across * down;

    /* build palette and remap pixels */
    build_palette(palette, ncolors,
                  pixels, img_w * img_h,
                  maxcol, outpal, remap);

    remapped = malloc(img_w * img_h);
    for (i = 0; i < img_w * img_h; i++)
        remapped[i] = (uint8_t)remap[pixels[i]];

    /* convert to bitplanes */
    planesize = (img_w / 8) * img_h;
    for (i = 0; i < nplanes; i++)
        plane_data[i] = calloc(1, planesize);

    to_planes(remapped, img_w, img_h,
              nplanes, plane_data);

    /* build CMAP */
    for (i = 0; i < maxcol; i++) {
        cmap_rgb[i*3+0] = outpal[i].r;
        cmap_rgb[i*3+1] = outpal[i].g;
        cmap_rgb[i*3+2] = outpal[i].b;
    }

    /* write IFF */
    iff_fp = fopen(argv[4], "wb");
    if (!iff_fp) { perror(argv[4]); return 1; }

    iff_write(nplanes, maxcol, cmap_rgb,
              img_w, img_h, plane_data,
              ntiles, across, down);
    fclose(iff_fp);

    printf("%s: %dx%d, %d colors (%d planes), "
           "%d tiles\n",
           argv[4], img_w, img_h,
           maxcol, nplanes, ntiles);

    for (i = 0; i < nplanes; i++) free(plane_data[i]);
    free(remapped);
    free(pixels);
    return 0;
}

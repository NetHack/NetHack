/* NetHack 3.6	txt2iff.c	$NHDT-Date: 1432512795 2015/05/25 00:13:15 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* 	Copyright (c) 1995 by Gregg Wonderly, Naperville, Illinois */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>

#include "config.h"
#include "tile.h"

#include <dos/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <libraries/iffparse.h>
#include <libraries/dos.h>
#include <clib/dos_protos.h>
#include <clib/iffparse_protos.h>
#ifndef _DCC
#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/dos.h>
#endif

void panic(const char *);
void map_colors(void);
int BestMatch(int, int, int);

extern pixval ColorMap[3][MAXCOLORMAPSIZE];
extern int colorsinmap;

/*
 * WARNING:
 * This program carries forth the assumption that the colormaps in all
 * of the .txt files are the same.  This is a bug.
 */

struct {
    int Height;
    int Width;
} IFFScreen;

/*
 * We are using a hybrid form of our own design which we call a BMAP (for
 * bitmap) form.  It is an ILBM with the bitmaps already deinterleaved,
 * completely uncompressed.
 * This speeds the loading of the images from the games point of view because
 * it
 * does not have to deinterleave and uncompress them.
 */
#define ID_BMAP MAKE_ID('B', 'M', 'A', 'P') /* instead of ILBM */
#define ID_BMHD MAKE_ID('B', 'M', 'H', 'D') /* Same as ILBM */
#define ID_CAMG MAKE_ID('C', 'A', 'M', 'G') /* Same as ILBM */
#define ID_CMAP MAKE_ID('C', 'M', 'A', 'P') /* Same as ILBM */
#define ID_PDAT                                                             \
    MAKE_ID('P', 'D', 'A', 'T')             /* Extra data describing plane  \
                                             * size due to graphics.library \
                                             * rounding requirements.       \
                                             */
#define ID_PLNE MAKE_ID('P', 'L', 'N', 'E') /* The planes of the image */

#ifndef _DCC
extern
#endif
    struct Library *IFFParseBase;

int nplanes;

/* BMHD from IFF documentation */
typedef struct {
    UWORD w, h;
    WORD x, y;
    UBYTE nPlanes;
    UBYTE masking;
    UBYTE compression;
    UBYTE reserved1;
    UWORD transparentColor;
    UBYTE xAspect, yAspect;
    WORD pageWidth, pageHeight;
} BitMapHeader;

typedef struct {
    UBYTE r, g, b;
} AmiColorMap;

pixel pixels[TILE_Y][TILE_X];
AmiColorMap *cmap;

int findcolor(register pixel *pix);
void packwritebody(pixel (*tile)[TILE_X], char **planes, int tileno);

void
error(char *str)
{
    fprintf(stderr, "ERROR: %s\n", str);
}

/*
 * This array maps the image colors to the amiga's first 16 colors.  The
 * colors
 * are reordered to help with maintaining dripen settings.
 */
int colrmap[] = { 0, 6, 9, 15, 4, 10, 2, 3, 5, 11, 7, 13, 8, 1, 14, 12 };

/* How many tiles fit across and down. */

#define COLS 20
#define ROWS ((tiles + COLS - 1) / COLS)

main(int argc, char **argv)
{
    int colors;
    struct {
        long nplanes;
        long pbytes;
        long across;
        long down;
        long npics;
        long xsize;
        long ysize;
    } pdat;
    long pbytes; /* Bytes of data in a plane */
    int i, cnt;
    BitMapHeader bmhd;
    struct IFFHandle *iff;
    long camg = HIRES | LACE;
    int tiles = 0;
    char **planes;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s source destination\n", argv[0]);
        exit(1);
    }

#if defined(_DCC) || defined(__GNUC__)
    IFFParseBase = OpenLibrary("iffparse.library", 0);
    if (!IFFParseBase) {
        error("unable to open iffparse.library");
        exit(1);
    }
#endif

    /* First, count the files in the file */
    if (fopen_text_file(argv[1], "r") != TRUE) {
        perror(argv[1]);
        return (1);
    }

    nplanes = 0;
    i = colorsinmap - 1; /*IFFScreen.Colors - 1; */
    while (i != 0) {
        nplanes++;
        i >>= 1;
    }

    planes = malloc(nplanes * sizeof(char *));
    if (planes == 0) {
        error("can not allocate planes pointer");
        exit(1);
    }

    while (read_text_tile(pixels) == TRUE)
        ++tiles;
    fclose_text_file();

    IFFScreen.Width = COLS * TILE_X;
    IFFScreen.Height = ROWS * TILE_Y;

    pbytes = (COLS * ROWS * TILE_X + 15) / 16 * 2 * TILE_Y;

    for (i = 0; i < nplanes; ++i) {
        planes[i] = calloc(1, pbytes);
        if (planes[i] == 0) {
            error("can not allocate planes pointer");
            exit(1);
        }
    }

    /* Now, process it */
    if (fopen_text_file(argv[1], "r") != TRUE) {
        perror(argv[1]);
        return (1);
    }

    iff = AllocIFF();
    if (!iff) {
        error("Can not allocate IFFHandle");
        return (1);
    }

    iff->iff_Stream = Open(argv[2], MODE_NEWFILE);
    if (!iff->iff_Stream) {
        error("Can not open output file");
        return (1);
    }

    InitIFFasDOS(iff);
    OpenIFF(iff, IFFF_WRITE);

    PushChunk(iff, ID_BMAP, ID_FORM, IFFSIZE_UNKNOWN);

    bmhd.w = IFFScreen.Width;
    bmhd.h = IFFScreen.Height;
    bmhd.x = 0;
    bmhd.y = 0;
    bmhd.nPlanes = nplanes;
    bmhd.masking = 0;
    bmhd.compression = 0;
    bmhd.reserved1 = 0;
    bmhd.transparentColor = 0;
    bmhd.xAspect = 100;
    bmhd.yAspect = 100;
    bmhd.pageWidth = TILE_X;
    bmhd.pageHeight = TILE_Y;

    PushChunk(iff, ID_BMAP, ID_BMHD, sizeof(bmhd));
    WriteChunkBytes(iff, &bmhd, sizeof(bmhd));
    PopChunk(iff);

    PushChunk(iff, ID_BMAP, ID_CAMG, sizeof(camg));
    WriteChunkBytes(iff, &camg, sizeof(camg));
    PopChunk(iff);

    /* We need to reorder the colors to get reasonable default pens but
     * we also need to know where some of the colors are - so go find out.
     */
    map_colors();

    cmap = malloc((colors = (1L << nplanes)) * sizeof(AmiColorMap));
    for (i = 0; i < colors; ++i) {
        cmap[colrmap[i]].r = ColorMap[CM_RED][i];
        cmap[colrmap[i]].g = ColorMap[CM_GREEN][i];
        cmap[colrmap[i]].b = ColorMap[CM_BLUE][i];
    }

    PushChunk(iff, ID_BMAP, ID_CMAP, IFFSIZE_UNKNOWN);
    for (i = 0; i < colors; ++i)
        WriteChunkBytes(iff, &cmap[i], 3);
    PopChunk(iff);

    cnt = 0;
    while (read_text_tile(pixels) == TRUE) {
        packwritebody(pixels, planes, cnt);
        if (cnt % 20 == 0)
            printf("%d..", cnt);
        ++cnt;
        fflush(stdout);
    }

    pdat.nplanes = nplanes;
    pdat.pbytes = pbytes;
    pdat.xsize = TILE_X;
    pdat.ysize = TILE_Y;
    pdat.across = COLS;
    pdat.down = ROWS;
    pdat.npics = cnt;

    PushChunk(iff, ID_BMAP, ID_PDAT, IFFSIZE_UNKNOWN);
    WriteChunkBytes(iff, &pdat, sizeof(pdat));
    PopChunk(iff);

    PushChunk(iff, ID_BMAP, ID_PLNE, IFFSIZE_UNKNOWN);
    for (i = 0; i < nplanes; ++i)
        WriteChunkBytes(iff, planes[i], pbytes);
    PopChunk(iff);

    CloseIFF(iff);
    Close(iff->iff_Stream);
    FreeIFF(iff);

    printf("\n%d tiles converted\n", cnt);

#if defined(_DCC) || defined(__GNUC__)
    CloseLibrary(IFFParseBase);
#endif
    exit(0);
}

findcolor(register pixel *pix)
{
    register int i;

    for (i = 0; i < MAXCOLORMAPSIZE; ++i) {
        if ((pix->r == ColorMap[CM_RED][i])
            && (pix->g == ColorMap[CM_GREEN][i])
            && (pix->b == ColorMap[CM_BLUE][i])) {
            return (i);
        }
    }
    return (-1);
}

void
packwritebody(pixel (*tile)[TILE_X], char **planes, int tileno)
{
    register int i, j, k, col;
    register char *buf;
    register int across, rowbytes, xoff, yoff;

    /* how many tiles fit across? */
    across = COLS;

    /* How many bytes per pixel row */
    rowbytes = ((IFFScreen.Width + 15) / 16) * 2;

    /* How many bytes to account for y distance in planes */
    yoff = ((tileno / across) * TILE_Y) * rowbytes;

    /* How many bytes to account for x distance in planes */
    xoff = (tileno % across) * (TILE_X / 8);

    /* For each row... */
    for (i = 0; i < TILE_Y; ++i) {
        /* For each bitplane... */
        for (k = 0; k < nplanes; ++k) {
            const int mask = 1l << k;

            /* Go across the row */
            for (j = 0; j < TILE_X; j++) {
                col = findcolor(&tile[i][j]);
                if (col == -1) {
                    error("can not convert pixel color to colormap index");
                    return;
                }
                /* Shift the colors around to have good complements and to
                 * know the dripen values.
                 */
                col = colrmap[col];

                /* To top left corner of tile */
                buf = planes[k] + yoff + xoff;

                /*To i'th row of tile and the correct byte for the j'th
                 * pixel*/
                buf += (i * rowbytes) + (j / 8);

                /* Or in the bit for this color */
                *buf |= (((col & mask) != 0) << (7 - (j % 8)));
            }
        }
    }
}

/* #define DBG */

/* map_colors
 * The incoming colormap is in arbitrary order and has arbitrary colors in
 * it, but we need (some) specific colors in specific places.  Find the
 * colors we need and fix the mapping table to match.
 */
/* What we are aiming for: */
/* XXX was 0-7 */
#define CX_BLACK 0
#define CX_WHITE 1
#define CX_BROWN 11
#define CX_CYAN 2
#define CX_GREEN 5
#define CX_MAGENTA 10
#define CX_BLUE 4
#define CX_RED 7
/* we don't care about the rest, at least now */
/* should get: black white blue red grey greyblue ltgrey */
void
map_colors()
{
    int x;
#if 1
    int tmpmap[] = { 0, 2, 3, 7, 4, 5, 8, 9, 10, 11, 13, 15, 12, 1, 14, 6 };
/* still not right: gray green yellow lost somewhere? */
#else
    int tmpmap[16];
    int x, y;
    for (x = 0; x < 16; x++)
        tmpmap[x] = -1; /* set not assigned yet */

    tmpmap[BestMatch(0, 0, 0)] = CX_BLACK;
    tmpmap[BestMatch(255, 255, 255)] = CX_WHITE;
    tmpmap[BestMatch(255, 0, 0)] = CX_RED;
    tmpmap[BestMatch(0, 255, 0)] = CX_GREEN;
    tmpmap[BestMatch(0, 0, 255)] = CX_BLUE;

    /* clean up the rest */
    for (x = 0; x < 16; x++) {
        for (y = 0; y < 16; y++)
            if (tmpmap[y] == x)
                goto outer_cont;
        for (y = 0; y < 16; y++)
            if (tmpmap[y] == -1) {
                tmpmap[y] = x;
                break;
            }
        if (y == 16)
            panic("too many colors?");
    outer_cont:
        ;
    }
    for (x = 0; x < 16; x++)
        if (tmpmap[y] == -1)
            panic("lost color?");
#endif
    for (x = 0; x < 16; x++) {
#ifdef DBG
        printf("final: c[%d]=%d (target: %d)\n", x, tmpmap[x], colrmap[x]);
#endif
        colrmap[x] = tmpmap[x];
    }
}
BestMatch(r, g, b) int r, g, b;
{
    int x;
    int bestslot;
    int bestrate = 99999999L;
    for (x = 0; x < 16; x++) {
        int rr = r - ColorMap[CM_RED][x];
        int gg = g - ColorMap[CM_GREEN][x];
        int bb = b - ColorMap[CM_BLUE][x];
        int rate = rr * rr + gg * gg + bb * bb;
        if (bestrate > rate) {
            bestrate = rate;
            bestslot = x;
        }
    }
#ifdef DBG
    printf("map (%d,%d,%d) -> %d (error=%d)\n", r, g, b, bestslot, bestrate);
#endif
    return bestslot;
}

long *
alloc(unsigned int n)
{
    long *ret = malloc(n);
    if (!ret) {
        error("Can't allocate memory");
        exit(1);
    }
    return (ret);
}

void
panic(const char *msg)
{
    fprintf(stderr, "PANIC: %s\n", msg);
    exit(1);
}

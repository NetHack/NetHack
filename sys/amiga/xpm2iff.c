/* NetHack 3.6	xpm2iff.c	$NHDT-Date: 1432512795 2015/05/25 00:13:15 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
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
#ifndef _DCC
#include <proto/iffparse.h>
#include <proto/dos.h>
#include <proto/exec.h>
#endif

struct xpmscreen {
    int Width;
    int Height;
    int Colors;
    int ColorResolution;
    int Background;
    int AspectRatio;
    int Interlace;
    int BytesPerRow;
} XpmScreen;

/* translation table from xpm characters to RGB and colormap slots */
struct Ttable {
    char flag;
    char r, g, b;
    int slot; /* output colortable index */
} ttable[256];

pixval ColorMap[3][MAXCOLORMAPSIZE];
int colorsinmap;

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

void
error(char *str)
{
    fprintf(stderr, "ERROR: %s\n", str);
}

char **planes;

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
    int index;

#if defined(_DCC) || defined(__GNUC__)
    IFFParseBase = OpenLibrary("iffparse.library", 0);
    if (!IFFParseBase) {
        error("unable to open iffparse.library");
        exit(1);
    }
#endif

    if (fopen_xpm_file(argv[1], "r") != TRUE) {
        perror(argv[1]);
        return (1);
    }

    nplanes = 0;
    i = XpmScreen.Colors - 1;
    while (i != 0) {
        nplanes++;
        i >>= 1;
    }

    planes = malloc(nplanes * sizeof(char *));
    if (planes == 0) {
        error("can not allocate planes pointer");
        exit(1);
    }

    XpmScreen.BytesPerRow = ((XpmScreen.Width + 15) / 16) * 2;
    pbytes = XpmScreen.BytesPerRow * XpmScreen.Height;
    for (i = 0; i < nplanes; ++i) {
        planes[i] = malloc(pbytes);
        if (planes[i] == 0) {
            error("can not allocate planes pointer");
            exit(1);
        }
        memset(planes[i], 0, pbytes);
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

    bmhd.w = XpmScreen.Width;
    bmhd.h = XpmScreen.Height;
    bmhd.x = 0;
    bmhd.y = 0;
    bmhd.nPlanes = nplanes;
    bmhd.masking = 0;
    bmhd.compression = 0;
    bmhd.reserved1 = 0;
    bmhd.transparentColor = 0;
    bmhd.xAspect = 100;
    bmhd.yAspect = 100;
    bmhd.pageWidth = 0;  /* not needed for this program */
    bmhd.pageHeight = 0; /* not needed for this program */

    PushChunk(iff, ID_BMAP, ID_BMHD, sizeof(bmhd));
    WriteChunkBytes(iff, &bmhd, sizeof(bmhd));
    PopChunk(iff);

    PushChunk(iff, ID_BMAP, ID_CAMG, sizeof(camg));
    WriteChunkBytes(iff, &camg, sizeof(camg));
    PopChunk(iff);

#define SCALE(x) (x)
    cmap = malloc((colors = (1L << nplanes)) * sizeof(AmiColorMap));
    if (cmap == 0) {
        error("Can't allocate color map");
        exit(1);
    }
    for (index = 0; index < 256; index++) {
        if (ttable[index].flag) {
            cmap[ttable[index].slot].r = SCALE(ttable[index].r);
            cmap[ttable[index].slot].g = SCALE(ttable[index].g);
            cmap[ttable[index].slot].b = SCALE(ttable[index].b);
        }
    }
#undef SCALE

    PushChunk(iff, ID_BMAP, ID_CMAP, IFFSIZE_UNKNOWN);
    WriteChunkBytes(iff, cmap, colors * sizeof(*cmap));
    PopChunk(iff);

    conv_image();

    pdat.nplanes = nplanes;
    pdat.pbytes = pbytes;
    pdat.xsize = XpmScreen.Width;
    pdat.ysize = XpmScreen.Height;
    pdat.across = 0;
    pdat.down = 0;
    pdat.npics = 1;

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

#if defined(_DCC) || defined(__GNUC__)
    CloseLibrary(IFFParseBase);
#endif
    exit(0);
}

#define SETBIT(Plane, Plane_offset, Col, Value)                          \
    if (Value) {                                                         \
        planes[Plane][Plane_offset + (Col / 8)] |= 1 << (7 - (Col & 7)); \
    }

conv_image()
{
    int row, col, planeno;

    for (row = 0; row < XpmScreen.Height; row++) {
        char *xb = xpmgetline();
        int plane_offset;
        if (xb == 0)
            return;
        plane_offset = row * XpmScreen.BytesPerRow;
        for (col = 0; col < XpmScreen.Width; col++) {
            int slot;
            int color = xb[col];
            if (!ttable[color].flag) {
                fprintf(stderr, "Bad image data\n");
            }
            slot = ttable[color].slot;
            for (planeno = 0; planeno < nplanes; planeno++) {
                SETBIT(planeno, plane_offset, col, slot & (1 << planeno));
            }
        }
    }
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

FILE *xpmfh = 0;
char initbuf[200];
char *xpmbuf = initbuf;

/* version 1.  Reads the raw xpm file, NOT the compiled version.  This is
 * not a particularly good idea but I don't have time to do the right thing
 * at this point, even if I was absolutely sure what that was. */
fopen_xpm_file(const char *fn, const char *mode)
{
    int temp;
    char *xb;
    if (strcmp(mode, "r"))
        return FALSE; /* no choice now */
    if (xpmfh)
        return FALSE; /* one file at a time */
    xpmfh = fopen(fn, mode);
    if (!xpmfh)
        return FALSE; /* I'm hard to please */

    /* read the header */
    xb = xpmgetline();
    if (xb == 0)
        return FALSE;
    if (4 != sscanf(xb, "%d %d %d %d", &XpmScreen.Width, &XpmScreen.Height,
                    &XpmScreen.Colors, &temp))
        return FALSE; /* bad header */
                      /* replace the original buffer with one big enough for
                       * the real data
                       */
    /* XXX */
    xpmbuf = malloc(XpmScreen.Width * 2);
    if (!xpmbuf) {
        error("Can't allocate line buffer");
        exit(1);
    }
    if (temp != 1)
        return FALSE; /* limitation of this code */

    {
        /* read the colormap and translation table */
        int ccount = -1;
        while (ccount++ < (XpmScreen.Colors - 1)) {
            char index;
            int r, g, b;
            xb = xpmgetline();
            if (xb == 0)
                return FALSE;
            if (4 != sscanf(xb, "%c c #%2x%2x%2x", &index, &r, &g, &b)) {
                fprintf(stderr, "Bad color entry: %s\n", xb);
                return FALSE;
            }
            ttable[index].flag = 1; /* this color is valid */
            ttable[index].r = r;
            ttable[index].g = g;
            ttable[index].b = b;
            ttable[index].slot = ccount;
        }
    }
    return TRUE;
}

/* This deserves better.  Don't read it too closely - you'll get ill. */
#define bufsz 2048
char buf[bufsz];
xpmgetline()
{
    char *bp;
    do {
        if (fgets(buf, bufsz, xpmfh) == 0)
            return 0;
    } while (buf[0] != '"');
    /* strip off the trailing <",> if any */
    for (bp = buf; *bp; bp++)
        ;
    bp--;
    while (isspace(*bp))
        bp--;
    if (*bp == ',')
        bp--;
    if (*bp == '"')
        bp--;
    bp++;
    *bp = '\0';

    return &buf[1];
}

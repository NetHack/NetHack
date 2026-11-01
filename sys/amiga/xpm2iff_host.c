/* xpm2iff_host.c - host-side .xpm -> Amiga BMAP IFF converter.
 * Copyright (c) 2026 by Ingo Paschke.
 * NetHack may be freely redistributed.  See license for details.
 *
 * Adapted from sys/amiga/xpm2iff.c, Copyright (c) 1995 by Gregg Wonderly.
 * Rewritten for host-side cross-compilation using POSIX file I/O with
 * explicit big-endian output instead of AmigaOS IFFParse library calls.
 *
 * Input:  an XPM2 file, 1 char per pixel.
 * Output: a BMAP IFF file readable by sys/amiga/winchar.c (tomb.iff).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

/* ------------------------------------------------------------------
 * XPM screen descriptor and color translation table
 * ------------------------------------------------------------------ */

static struct {
    int Width;
    int Height;
    int Colors;
    int BytesPerRow;
} XpmScreen;

/* Translation table: indexed by the single XPM character code.
 * slot  = output palette index (0-based)
 * flag  = 1 if this entry is valid
 * r,g,b = RGB components (0-255) */
static struct {
    unsigned char flag;
    unsigned char r, g, b;
    int slot;
} ttable[256];

/* ------------------------------------------------------------------
 * XPM parsing
 * Adapted directly from sys/amiga/xpm2iff.c (already POSIX).
 * ------------------------------------------------------------------ */

static FILE *xpmfh;

#define XBUFSZ 2048
static char xbuf[XBUFSZ];

/* Read the next quoted line from the XPM file.
 * Returns a pointer to the content between the first pair of double-quotes,
 * or NULL on EOF.  Trailing ",  and whitespace are stripped. */
static char *
xpmgetline(void)
{
    char *bp;
    do {
        if (fgets(xbuf, XBUFSZ, xpmfh) == NULL)
            return NULL;
    } while (xbuf[0] != '"');

    /* strip trailing <",> and whitespace */
    for (bp = xbuf; *bp; bp++)
        ;
    bp--;
    while (isspace((unsigned char)*bp))
        bp--;
    if (*bp == ',')
        bp--;
    if (*bp == '"')
        bp--;
    bp++;
    *bp = '\0';

    return &xbuf[1];
}

/* Open an XPM file and parse its header + color table.
 * Populates XpmScreen and ttable[].
 * Returns 1 on success, 0 on failure. */
static int
fopen_xpm_file(const char *fn)
{
    int temp;
    char *xb;

    xpmfh = fopen(fn, "r");
    if (!xpmfh)
        return 0;

    /* read dimensions header: "W H Colors 1" */
    xb = xpmgetline();
    if (!xb)
        return 0;
    if (sscanf(xb, "%d %d %d %d",
               &XpmScreen.Width, &XpmScreen.Height,
               &XpmScreen.Colors, &temp) != 4)
        return 0;
    if (temp != 1) {
        fprintf(stderr, "xpm2iff_host: only 1 char/pixel XPM files supported\n");
        return 0;
    }

    /* read color map: "%c c #rrggbb" */
    {
        int ccount = 0;
        while (ccount < XpmScreen.Colors) {
            char idx;
            int r, g, b;
            xb = xpmgetline();
            if (!xb)
                return 0;
            if (sscanf(xb, "%c c #%2x%2x%2x", &idx, &r, &g, &b) != 4) {
                fprintf(stderr, "xpm2iff_host: bad color entry: %s\n", xb);
                return 0;
            }
            ttable[(unsigned char)idx].flag = 1;
            ttable[(unsigned char)idx].r    = (unsigned char)r;
            ttable[(unsigned char)idx].g    = (unsigned char)g;
            ttable[(unsigned char)idx].b    = (unsigned char)b;
            ttable[(unsigned char)idx].slot = ccount;
            ccount++;
        }
    }
    return 1;
}

/* ------------------------------------------------------------------
 * Bitplane packing
 * ------------------------------------------------------------------ */

static char **planes;

#define SETBIT(plane, plane_offset, col, value)         \
    do {                                                \
        if (value)                                      \
            planes[plane][plane_offset + ((col) / 8)]   \
                |= (char)(1 << (7 - ((col) & 7)));      \
    } while (0)

static void
conv_image(int nplanes)
{
    int row, col, planeno;

    for (row = 0; row < XpmScreen.Height; row++) {
        char *xb = xpmgetline();
        int plane_offset;
        if (!xb)
            return;
        plane_offset = row * XpmScreen.BytesPerRow;
        for (col = 0; col < XpmScreen.Width; col++) {
            int color = (unsigned char)xb[col];
            int slot;
            if (!ttable[color].flag) {
                fprintf(stderr, "xpm2iff_host: bad image data at row %d col %d\n",
                        row, col);
                continue;
            }
            slot = ttable[color].slot;
            for (planeno = 0; planeno < nplanes; planeno++)
                SETBIT(planeno, plane_offset, col, slot & (1 << planeno));
        }
    }
}

/* ------------------------------------------------------------------
 * Big-endian IFF output helpers
 * ------------------------------------------------------------------ */

static FILE *iff_out;

static void
wr32(uint32_t v)
{
    fputc((v >> 24) & 0xff, iff_out);
    fputc((v >> 16) & 0xff, iff_out);
    fputc((v >>  8) & 0xff, iff_out);
    fputc( v        & 0xff, iff_out);
}

static void
write_chunk(const char *id, const void *data, uint32_t size)
{
    fwrite(id,   1, 4,    iff_out);
    wr32(size);
    fwrite(data, 1, size, iff_out);
    if (size & 1)
        fputc(0, iff_out);
}

/* ------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------ */

int
main(int argc, char **argv)
{
    int      i, nplanes, colors;
    uint32_t pbytes, plne_size, form_size;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s source.xpm destination.iff\n", argv[0]);
        return 1;
    }

    if (!fopen_xpm_file(argv[1])) {
        fprintf(stderr, "%s: failed to open or parse XPM file\n", argv[1]);
        return 1;
    }

    /* nplanes = ceil(log2(Colors)) */
    nplanes = 0;
    i = XpmScreen.Colors - 1;
    while (i > 0) { nplanes++; i >>= 1; }

    colors = 1 << nplanes;

    XpmScreen.BytesPerRow = ((XpmScreen.Width + 15) / 16) * 2;
    pbytes = (uint32_t)XpmScreen.BytesPerRow * (uint32_t)XpmScreen.Height;

    /* Allocate zero-initialised bitplane buffers */
    planes = malloc(nplanes * sizeof(char *));
    if (!planes) { perror("malloc"); return 1; }
    for (i = 0; i < nplanes; ++i) {
        planes[i] = calloc(1, pbytes);
        if (!planes[i]) { perror("calloc"); return 1; }
    }

    /* Pack pixel data into bitplanes */
    conv_image(nplanes);
    fclose(xpmfh);

    /* Open output IFF file */
    iff_out = fopen(argv[2], "wb");
    if (!iff_out) { perror(argv[2]); return 1; }

    /* Pre-compute FORM size:
     *   4  (BMAP type tag)
     *   8 + 20  (BMHD)
     *   8 +  4  (CAMG)
     *   8 + colors*3  (CMAP; colors is a power of 2, so colors*3 is even)
     *   8 + 28  (PDAT: 7 x uint32_t)
     *   8 + nplanes*pbytes  (PLNE; pbytes is always even)
     */
    plne_size = (uint32_t)nplanes * pbytes;
    form_size = 4
              + (8 + 20)
              + (8 +  4)
              + (8 + (uint32_t)colors * 3)
              + (8 + 28)
              + (8 + plne_size);

    /* FORM header */
    fwrite("FORM", 1, 4, iff_out);
    wr32(form_size);
    fwrite("BMAP", 1, 4, iff_out);

    /* BMHD chunk */
    {
        uint8_t bmhd[20];
        uint8_t *p = bmhd;
        uint16_t w = (uint16_t)XpmScreen.Width;
        uint16_t h = (uint16_t)XpmScreen.Height;

        p[0] = w >> 8;  p[1] = w & 0xff;  p += 2;  /* w */
        p[0] = h >> 8;  p[1] = h & 0xff;  p += 2;  /* h */
        memset(p, 0, 4);                   p += 4; /* x=0, y=0 */
        *p++ = (uint8_t)nplanes;                   /* nPlanes */
        *p++ = 0;                                  /* masking: none */
        *p++ = 0;                                  /* compression: none */
        *p++ = 0;                                  /* reserved1 */
        p[0] = 0; p[1] = 0;               p += 2;  /* transparentColor */
        *p++ = 100;                                /* xAspect */
        *p++ = 100;                                /* yAspect */
        p[0] = 0; p[1] = 0;               p += 2;  /* pageWidth (not used) */
        p[0] = 0; p[1] = 0;               p += 2;  /* pageHeight (not used) */

        write_chunk("BMHD", bmhd, 20);
    }

    /* CAMG chunk: HIRES | LACE = 0x00008004 */
    {
        uint8_t camg[4] = { 0x00, 0x00, 0x80, 0x04 };
        write_chunk("CAMG", camg, 4);
    }

    /* CMAP chunk: built from ttable (no color reordering for XPM images) */
    {
        uint8_t *cmap = calloc(colors, 3);
        if (!cmap) { perror("calloc"); return 1; }
        for (i = 0; i < 256; i++) {
            if (ttable[i].flag) {
                int s = ttable[i].slot;
                cmap[s * 3 + 0] = ttable[i].r;
                cmap[s * 3 + 1] = ttable[i].g;
                cmap[s * 3 + 2] = ttable[i].b;
            }
        }
        write_chunk("CMAP", cmap, (uint32_t)colors * 3);
        free(cmap);
    }

    /* PDAT chunk: 7 x uint32_t big-endian
     * nplanes, pbytes, across=0, down=0, npics=1, xsize=Width, ysize=Height */
    {
        uint32_t vals[7];
        uint8_t  pdat[28];
        uint8_t *p = pdat;

        vals[0] = (uint32_t)nplanes;
        vals[1] = pbytes;
        vals[2] = 0;  /* across: not a tile sheet */
        vals[3] = 0;  /* down */
        vals[4] = 1;  /* npics */
        vals[5] = (uint32_t)XpmScreen.Width;
        vals[6] = (uint32_t)XpmScreen.Height;

        for (i = 0; i < 7; i++) {
            p[0] = (vals[i] >> 24) & 0xff;
            p[1] = (vals[i] >> 16) & 0xff;
            p[2] = (vals[i] >>  8) & 0xff;
            p[3] =  vals[i]        & 0xff;
            p += 4;
        }
        write_chunk("PDAT", pdat, 28);
    }

    /* PLNE chunk: concatenated bitplane data */
    fwrite("PLNE", 1, 4, iff_out);
    wr32(plne_size);
    for (i = 0; i < nplanes; ++i)
        fwrite(planes[i], 1, pbytes, iff_out);
    if (plne_size & 1)
        fputc(0, iff_out);

    fclose(iff_out);

    for (i = 0; i < nplanes; ++i) free(planes[i]);
    free(planes);

    printf("tomb.iff: %dx%d, %d colors (%d planes), %u bytes/plane\n",
           XpmScreen.Width, XpmScreen.Height, colors, nplanes,
           (unsigned)pbytes);
    return 0;
}

/* NetHack 5.0	tile2pict.c	*/
/* Copyright (c) Ingo Paschke, 2026. */
/* NetHack may be freely redistributed.  See license for details. */
/*
 * Host tool: emit a Rez source file containing PICT v2 resources for the
 * Mac 68k port. Sibling of tile2bmp.c, sharing tiletext.c.
 * Emits PICT 1001 (8bpp/256-color) and PICT 1000 (4bpp/16-color).
 */

#include "config.h"
#include "hacklib.h"
#include "tile.h"

#include <stdlib.h>
#include <string.h>

#include "tile2pict_quant.h"

extern void monst_globals_init(void);
extern void objects_globals_init(void);

static const char *const relative_tiledir = "../win/share/";
static const char *const tilefilenames[3] = {
    "monsters.txt", "objects.txt", "other.txt"
};

#define TILES_PER_ROW 30   /* 30 * 16 = 480 px wide sheet */

static pixel tilepixels[TILE_Y][TILE_X];

static int sheet_w, sheet_h;
static unsigned char *sheet;     /* 8bpp pixels, row-major top-down */

static void
paste_tile(int tile_index)
{
    int sx = (tile_index % TILES_PER_ROW) * TILE_X;
    int sy = (tile_index / TILES_PER_ROW) * TILE_Y;
    int x, y, c;
    for (y = 0; y < TILE_Y; ++y) {
        for (x = 0; x < TILE_X; ++x) {
            for (c = 0; c < colorsinmap; ++c) {
                if (ColorMap[CM_RED][c] == tilepixels[y][x].r
                    && ColorMap[CM_GREEN][c] == tilepixels[y][x].g
                    && ColorMap[CM_BLUE][c] == tilepixels[y][x].b)
                    break;
            }
            if (c >= colorsinmap) c = 0; /* fallback */
            sheet[(sy + y) * sheet_w + (sx + x)] = (unsigned char) c;
        }
    }
}

/* -----------------------------------------------------------------------
 * Step 1: Big-endian helpers + growable byte buffer
 * ----------------------------------------------------------------------- */

static unsigned char *gPicBuf = NULL;
static size_t gPicLen = 0, gPicCap = 0;

static void put_byte(unsigned v) {
    if (gPicLen + 1 > gPicCap) {
        size_t newcap = gPicCap ? gPicCap * 2 : 4096;
        unsigned char *tmp = realloc(gPicBuf, newcap);
        if (!tmp) {
            fprintf(stderr, "tile2pict: out of memory growing pic buffer\n");
            exit(1);
        }
        gPicBuf = tmp;
        gPicCap = newcap;
    }
    gPicBuf[gPicLen++] = (unsigned char) v;
}
static void put_be16(unsigned v) { put_byte(v >> 8); put_byte(v); }
static void put_be32(unsigned long v) {
    put_byte(v >> 24); put_byte(v >> 16); put_byte(v >> 8); put_byte(v);
}
static void put_rect(int t, int l, int b, int r) {
    put_be16(t); put_be16(l); put_be16(b); put_be16(r);
}

/* -----------------------------------------------------------------------
 * Step 2: PackBits row encoder
 * ----------------------------------------------------------------------- */

static void
packbits_row(const unsigned char *row, int n, unsigned char *out, int *out_n)
{
    int i = 0, o = 0;
    while (i < n) {
        int run = 1;
        while (i + run < n && run < 128 && row[i + run] == row[i]) run++;
        if (run >= 3) {
            out[o++] = (unsigned char) (256 - (run - 1));   /* -(run-1) as int8 */
            out[o++] = row[i];
            i += run;
        } else {
            int lit_start = i;
            int lit = 0;
            while (i < n && lit < 128) {
                int look = 1;
                while (i + look < n && look < 4 && row[i + look] == row[i]) look++;
                if (look >= 3) break;       /* upcoming replicate run; cut literal */
                ++i; ++lit;
            }
            out[o++] = (unsigned char) (lit - 1);          /* literal count */
            memcpy(out + o, row + lit_start, lit);
            o += lit;
        }
    }
    *out_n = o;
}

/* -----------------------------------------------------------------------
 * Step 3: PICT v2 emitter for the 8bpp sheet
 * ----------------------------------------------------------------------- */

static void
build_pict_8bpp(unsigned char **out_buf, size_t *out_len)
{
    gPicBuf = NULL; gPicLen = 0; gPicCap = 0;

    /* picSize placeholder. QuickDraw reads PICTs from a Resource Manager
       handle by handle size, not by picSize, so this 16-bit field is
       informational. We patch it at the end and saturate to 0 when the
       PICT exceeds 32767 bytes — which it always does for our data. */
    size_t size_off = gPicLen;
    put_be16(0);

    /* picFrame: (top, left, bottom, right). */
    put_rect(0, 0, sheet_h, sheet_w);

    /* version-2 picture preamble. */
    put_be16(0x0011);            /* versionOp */
    put_be16(0x02FF);            /* version 2 */
    put_be16(0x0C00);            /* HeaderOp */
    /* Header (24 bytes): version=-2, reserved=0, hRes/vRes=72.0 fixed,
       srcRect, reserved=0. */
    put_be16(0xFFFE);            /* version */
    put_be16(0);                 /* reserved */
    put_be32(0x00480000);        /* hRes 72.0 */
    put_be32(0x00480000);        /* vRes 72.0 */
    put_rect(0, 0, sheet_h, sheet_w);
    put_be32(0);                 /* reserved */

    /* DefHilite + Clip rgn (rgnSize=10, bbox covers whole pic). */
    put_be16(0x001E);            /* DefHilite */
    put_be16(0x0001);            /* Clip */
    put_be16(0x000A);            /* rgnSize */
    put_rect(0, 0, sheet_h, sheet_w);

    /* PackBitsRect opcode for an 8bpp PixMap. */
    put_be16(0x0098);            /* PackBitsRect */

    /* PixMap (50 bytes including baseAddr=0 absent in PICT — see IM:QD). */
    put_be16(sheet_w | 0x8000);  /* rowBytes high bit = "this is a PixMap" */
    put_rect(0, 0, sheet_h, sheet_w);  /* bounds */
    put_be16(0);                 /* pmVersion */
    put_be16(0);                 /* packType (default) */
    put_be32(0);                 /* packSize */
    put_be32(0x00480000);        /* hRes */
    put_be32(0x00480000);        /* vRes */
    put_be16(0);                 /* pixelType (chunky) */
    put_be16(8);                 /* pixelSize */
    put_be16(1);                 /* cmpCount */
    put_be16(8);                 /* cmpSize */
    put_be32(0);                 /* planeBytes */
    put_be32(0);                 /* pmTable */
    put_be32(0);                 /* pmReserved */

    /* ColorTable: 8-byte header + 8 bytes per ColorSpec, full 256 entries. */
    put_be32(0);                 /* ctSeed */
    put_be16(0);                 /* ctFlags */
    put_be16(255);               /* ctSize = entries - 1 */
    int c;
    for (c = 0; c < 256; ++c) {
        put_be16(c);             /* value (palette index) */
        if (c < colorsinmap) {
            put_be16(ColorMap[CM_RED][c]   * 257);  /* R: 8-bit -> 16-bit */
            put_be16(ColorMap[CM_GREEN][c] * 257);
            put_be16(ColorMap[CM_BLUE][c]  * 257);
        } else {
            put_be16(0); put_be16(0); put_be16(0);   /* unused entry */
        }
    }

    /* srcRect, dstRect, mode. */
    put_rect(0, 0, sheet_h, sheet_w);
    put_rect(0, 0, sheet_h, sheet_w);
    put_be16(0);                 /* srcCopy */

    /* PackBits-compressed pixel rows. rowBytes >= 250 -> 2-byte length prefix. */
    {
        /* PackBits worst case: ceil(n/128) groups of 1 header + 128 bytes
           (same formula as the 4bpp path) */
        unsigned char *rowtmp =
            malloc((size_t) (((sheet_w + 127) / 128) * 129 + 1));
        if (!rowtmp) {
            fprintf(stderr, "tile2pict: out of memory for row buffer\n");
            exit(1);
        }
        int y;
        for (y = 0; y < sheet_h; ++y) {
            int packed_n = 0;
            packbits_row(sheet + (size_t) y * sheet_w, sheet_w,
                         rowtmp, &packed_n);
            put_be16(packed_n);
            int k;
            for (k = 0; k < packed_n; ++k) put_byte(rowtmp[k]);
        }
        free(rowtmp);
    }

    put_be16(0x00FF);            /* endPic */
    if (gPicLen & 1) put_byte(0); /* word-align */

    /* Patch picSize at offset 0. QuickDraw uses the resource handle's
       size, not picSize, so this is informational; saturate to 0 when
       the PICT exceeds 32767 bytes. */
    unsigned short shortsize = (gPicLen <= 0xFFFF) ? (unsigned short) gPicLen : 0;
    gPicBuf[size_off]     = (unsigned char) (shortsize >> 8);
    gPicBuf[size_off + 1] = (unsigned char) (shortsize & 0xFF);

    *out_buf = gPicBuf;
    *out_len = gPicLen;
}

/* -----------------------------------------------------------------------
 * Step 3b: PICT v2 emitter for the 4bpp sheet
 *
 * For sheet_w=480: packed rowBytes = 480/2 = 240, which is <= 250, so we use
 * a 1-byte PackBits row-length prefix (IM:QD rule: rowBytes <= 250 -> 1 byte).
 * This differs from the 8bpp emitter where raw rowBytes=480 > 250 -> 2 bytes.
 * ----------------------------------------------------------------------- */

static void
build_pict_4bpp(const unsigned char *sheet4,
                const unsigned char *dst_pal, int dst_n,
                unsigned char **out_buf, size_t *out_len)
{
    int packed_row_bytes = (sheet_w + 1) / 2;  /* raw rowBytes, no high bit */
    int c;

    gPicBuf = NULL; gPicLen = 0; gPicCap = 0;

    /* picSize placeholder. */
    size_t size_off = gPicLen;
    put_be16(0);

    /* picFrame. */
    put_rect(0, 0, sheet_h, sheet_w);

    /* version-2 preamble. */
    put_be16(0x0011);
    put_be16(0x02FF);
    put_be16(0x0C00);
    put_be16(0xFFFE);
    put_be16(0);
    put_be32(0x00480000);
    put_be32(0x00480000);
    put_rect(0, 0, sheet_h, sheet_w);
    put_be32(0);

    put_be16(0x001E);   /* DefHilite */
    put_be16(0x0001);   /* Clip */
    put_be16(0x000A);
    put_rect(0, 0, sheet_h, sheet_w);

    /* PackBitsRect for a 4bpp PixMap. */
    put_be16(0x0098);

    /* PixMap: rowBytes has high bit set to flag PixMap. */
    put_be16((unsigned) packed_row_bytes | 0x8000);
    put_rect(0, 0, sheet_h, sheet_w);
    put_be16(0);                /* pmVersion */
    put_be16(0);                /* packType */
    put_be32(0);                /* packSize */
    put_be32(0x00480000);       /* hRes */
    put_be32(0x00480000);       /* vRes */
    put_be16(0);                /* pixelType (chunky) */
    put_be16(4);                /* pixelSize = 4 */
    put_be16(1);                /* cmpCount */
    put_be16(4);                /* cmpSize = 4 */
    put_be32(0);                /* planeBytes */
    put_be32(0);                /* pmTable */
    put_be32(0);                /* pmReserved */

    /* ColorTable: 16 entries (ctSize = 15). */
    put_be32(0);                /* ctSeed */
    put_be16(0);                /* ctFlags */
    put_be16(15);               /* ctSize = entries - 1 */
    for (c = 0; c < 16; ++c) {
        put_be16(c);
        if (c < dst_n) {
            put_be16(dst_pal[3*c+0] * 257);
            put_be16(dst_pal[3*c+1] * 257);
            put_be16(dst_pal[3*c+2] * 257);
        } else {
            put_be16(0); put_be16(0); put_be16(0);
        }
    }

    /* srcRect, dstRect, mode. */
    put_rect(0, 0, sheet_h, sheet_w);
    put_rect(0, 0, sheet_h, sheet_w);
    put_be16(0);    /* srcCopy */

    /* PackBits-compressed pixel rows.
     * raw rowBytes = 240 <= 250 -> 1-byte length prefix per row. */
    {
        /* packed_row_bytes bytes per row (2 pixels/byte, high nibble first).
         * Worst-case PackBits: ceil(n/128)*129 control+literal bytes. */
        int pack_buf_size = ((packed_row_bytes + 127) / 128) * 129 + 1;
        unsigned char *rowpacked = malloc((size_t) packed_row_bytes);
        unsigned char *rowcomp   = malloc((size_t) pack_buf_size);
        if (!rowpacked || !rowcomp) {
            fprintf(stderr, "tile2pict: out of memory for 4bpp row buffers\n");
            exit(1);
        }

        int y;
        for (y = 0; y < sheet_h; ++y) {
            const unsigned char *src_row = sheet4 + (size_t) y * sheet_w;
            int x;
            /* Pack two 4-bit indices per byte, high nibble first. */
            for (x = 0; x < packed_row_bytes; ++x) {
                int p0 = src_row[x * 2];
                int p1 = (x * 2 + 1 < sheet_w) ? src_row[x * 2 + 1] : 0;
                rowpacked[x] = (unsigned char) ((p0 << 4) | (p1 & 0x0F));
            }
            int comp_n = 0;
            packbits_row(rowpacked, packed_row_bytes, rowcomp, &comp_n);
            /* 1-byte prefix (packed_row_bytes = 240 <= 250). */
            put_byte((unsigned) comp_n);
            int k;
            for (k = 0; k < comp_n; ++k) put_byte(rowcomp[k]);
        }
        free(rowpacked);
        free(rowcomp);
    }

    put_be16(0x00FF);           /* endPic */
    if (gPicLen & 1) put_byte(0);

    unsigned short shortsize = (gPicLen <= 0xFFFF) ? (unsigned short) gPicLen : 0;
    gPicBuf[size_off]     = (unsigned char) (shortsize >> 8);
    gPicBuf[size_off + 1] = (unsigned char) (shortsize & 0xFF);

    *out_buf = gPicBuf;
    *out_len = gPicLen;
}

/* -----------------------------------------------------------------------
 * Step 4: Rez emitter
 * ----------------------------------------------------------------------- */

static void
emit_rez(FILE *out, int rsrc_id, const unsigned char *buf, size_t n)
{
    size_t i;
    fprintf(out, "data 'PICT' (%d) {\n", rsrc_id);
    for (i = 0; i < n; ++i) {
        if ((i % 16) == 0) fprintf(out, "    $\"");
        fprintf(out, "%02X", buf[i]);
        if ((i % 16) == 15 || i + 1 == n) fprintf(out, "\"\n");
        else if ((i % 2) == 1) fprintf(out, " ");
    }
    fprintf(out, "};\n\n");
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */

int
main(int argc, char *argv[])
{
    int total_tiles = 0, i;
    char path[256];

    if (argc != 2) {
        fprintf(stderr, "usage: %s outfile.r\n", argv[0]);
        return 1;
    }
    objects_globals_init();
    monst_globals_init();

    /* Pass 1: count tiles. */
    for (i = 0; i < 3; ++i) {
        snprintf(path, sizeof path, "%s%s", relative_tiledir, tilefilenames[i]);
        if (!fopen_text_file(path, RDTMODE)) {
            fprintf(stderr, "cannot open %s\n", path);
            return 1;
        }
        while (read_text_tile(tilepixels))
            ++total_tiles;
        fclose_text_file();
    }
    fprintf(stderr, "tile2pict: %d tiles total\n", total_tiles);

    /* Pass 2: allocate sheet and paste tiles. */
    sheet_w = TILES_PER_ROW * TILE_X;
    sheet_h = ((total_tiles + TILES_PER_ROW - 1) / TILES_PER_ROW) * TILE_Y;
    sheet = calloc((size_t) sheet_w * (size_t) sheet_h, 1);
    if (!sheet) { fprintf(stderr, "out of memory\n"); return 1; }

    {
        int placed = 0;
        for (i = 0; i < 3; ++i) {
            snprintf(path, sizeof path, "%s%s", relative_tiledir, tilefilenames[i]);
            if (!fopen_text_file(path, RDTMODE)) {
                fprintf(stderr, "cannot open %s on second pass\n", path);
                return 1;
            }
            while (read_text_tile(tilepixels))
                paste_tile(placed++);
            fclose_text_file();
        }
    }
    fprintf(stderr, "tile2pict: built %dx%d 8bpp sheet (%d colors)\n",
            sheet_w, sheet_h, colorsinmap);

    /* Open output file. */
    FILE *out = fopen(argv[1], "w");
    if (!out) {
        fprintf(stderr, "cannot open %s for writing\n", argv[1]);
        return 1;
    }
    fprintf(out, "/* Auto-generated by tile2pict. Do not edit. */\n\n");

    /* Step 5a: Build and emit PICT 1001 (8bpp, 256-color palette). */
    {
        unsigned char *picbuf = NULL;
        size_t piclen = 0;
        build_pict_8bpp(&picbuf, &piclen);
        /* Note: picSize (the 16-bit field at byte 0) is saturated to 0 when the
           PICT binary exceeds 32767 bytes.  QuickDraw ignores picSize when reading
           a PICT from a resource handle, so a large resource is fine.  A 480x816
           8bpp sheet compresses to ~190KB — well within Mac resource limits. */
        fprintf(stderr, "tile2pict: PICT 1001 (8bpp) %zu bytes (%d colors)\n",
                piclen, colorsinmap);
        emit_rez(out, 1001, picbuf, piclen);
        free(picbuf);
    }

    /* Step 5b: Build and emit PICT 1000 (4bpp, 16-color quantized + dithered). */
    {
        unsigned long hist[256];
        unsigned char src_pal[3 * 256];
        unsigned char dst_pal[3 * 16];
        int dst_n;
        size_t px;
        int j;

        memset(hist, 0, sizeof hist);
        for (px = 0; px < (size_t) sheet_w * (size_t) sheet_h; ++px)
            hist[sheet[px]]++;

        for (j = 0; j < colorsinmap; ++j) {
            src_pal[3*j+0] = ColorMap[CM_RED][j];
            src_pal[3*j+1] = ColorMap[CM_GREEN][j];
            src_pal[3*j+2] = ColorMap[CM_BLUE][j];
        }

        dst_n = median_cut(src_pal, hist, colorsinmap, 16, dst_pal);

        unsigned char *sheet4 = malloc((size_t) sheet_w * (size_t) sheet_h);
        if (!sheet4) {
            fprintf(stderr, "tile2pict: out of memory for sheet4\n");
            fclose(out);
            return 1;
        }
        dither_fs(sheet, sheet_w, sheet_h,
                  src_pal, colorsinmap,
                  dst_pal, dst_n, sheet4);

        unsigned char *picbuf4 = NULL;
        size_t piclen4 = 0;
        build_pict_4bpp(sheet4, dst_pal, dst_n, &picbuf4, &piclen4);
        fprintf(stderr, "tile2pict: PICT 1000 (4bpp) %zu bytes (%d colors)\n",
                piclen4, dst_n);
        emit_rez(out, 1000, picbuf4, piclen4);

        free(sheet4);
        free(picbuf4);
    }

    fclose(out);
    return 0;
}

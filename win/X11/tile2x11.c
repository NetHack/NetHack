/* $NHDT-Date: 1546081295 2018/12/29 11:01:35 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.12 $ */
/*      Copyright (c) 2017 by Pasi Kallinen                       */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Convert the given input files into an output file that is expected
 * by nethack.
 *
 * Assumptions:
 * 	+ Two dimensional byte arrays are in row order and are not padded
 *	  between rows (x11_colormap[][]).
 */
#include "hack.h" /* for MAX_GLYPH */
#include "tile.h"
#include "tile2x11.h" /* x11 output file header structure */

#define OUTNAME "x11tiles"   /* output file name */
/* #define PRINT_COLORMAP */ /* define to print the colormap */

x11_header header;
unsigned char tile_bytes[TILE_X * TILE_Y * (MAX_GLYPH + TILES_PER_ROW)];
unsigned char *curr_tb = tile_bytes;
unsigned char x11_colormap[MAXCOLORMAPSIZE][3];

extern void monst_globals_init(void);
extern void objects_globals_init(void);

/* Look up the given pixel and return its colormap index. */
static unsigned char
pix_to_colormap(pixel pix)
{
    unsigned long i;

    for (i = 0; i < header.ncolors; i++) {
        if (pix.r == ColorMap[CM_RED][i]
            && pix.g == ColorMap[CM_GREEN][i]
            && pix.b == ColorMap[CM_BLUE][i])
            break;
    }

    if (i == header.ncolors) {
        Fprintf(stderr, "can't find color: [%u,%u,%u]\n", pix.r, pix.g,
                pix.b);
        exit(1);
    }
    return (unsigned char) (i & 0xFF);
}

/* Convert the tiles in the file to our format of bytes. */
static unsigned long
convert_tiles(unsigned char **tb_ptr, /* pointer to a tile byte pointer */
              unsigned long total)    /* total tiles so far */
{
    unsigned char *tb = *tb_ptr;
    unsigned long count = 0;
    pixel tile[TILE_Y][TILE_X];
    int x, y;

    while (read_text_tile(tile)) {
        count++;
        total++;
        for (y = 0; y < TILE_Y; y++) {
            for (x = 0; x < TILE_X; x++)
                tb[x] = pix_to_colormap(tile[y][x]);
            tb += TILE_X * header.per_row;
        }

        /* repoint at the upper-left corner of the next tile */
        *tb_ptr += TILE_X;
        if (header.per_row == 1 || (total % header.per_row) == 0)
            *tb_ptr += TILE_X * (TILE_Y - 1) * header.per_row;
        tb = *tb_ptr;
    }

    return count;
}

/* Merge the current text colormap (ColorMap) with ours (x11_colormap). */
static void
merge_text_colormap(void)
{
    unsigned i, j;

    for (i = 0; i < (unsigned) colorsinmap; i++) {
        for (j = 0; j < header.ncolors; j++)
            if (x11_colormap[j][CM_RED] == ColorMap[CM_RED][i]
                && x11_colormap[j][CM_GREEN] == ColorMap[CM_GREEN][i]
                && x11_colormap[j][CM_BLUE] == ColorMap[CM_BLUE][i])
                break;

        if (j >= MAXCOLORMAPSIZE) {
            Fprintf(stderr, "colormap overflow\n");
            exit(1);
        }

        if (j == header.ncolors) { /* couldn't find it */
#ifdef PRINT_COLORMAP
            Fprintf(stdout, "color %2d: %3d %3d %3d\n", header.ncolors,
                   ColorMap[CM_RED][i], ColorMap[CM_GREEN][i],
                   ColorMap[CM_BLUE][i]);
#endif

            x11_colormap[j][CM_RED] = ColorMap[CM_RED][i];
            x11_colormap[j][CM_GREEN] = ColorMap[CM_GREEN][i];
            x11_colormap[j][CM_BLUE] = ColorMap[CM_BLUE][i];
            header.ncolors++;
        }
    }
}

/* Open the given file, read & merge the colormap, convert the tiles. */
static void
process_file(char *fname)
{
    unsigned long count;

    if (!fopen_text_file(fname, RDTMODE)) {
        Fprintf(stderr, "can't open file \"%s\"\n", fname);
        exit(1);
    }
    merge_text_colormap();
    count = convert_tiles(&curr_tb, header.ntiles);
    Fprintf(stdout, "%s: %lu tiles\n", fname, count);
    header.ntiles += count;
    fclose_text_file();
}

#ifdef USE_XPM
static int
xpm_write(FILE *fp)
{
    unsigned long i, j;
    unsigned n;

    if (header.ncolors > 64) {
        Fprintf(stderr, "Sorry, only configured for up to 64 colors\n");
        exit(1);
        /* All you need to do is add more char per color - below */
    }

    Fprintf(fp, "/* XPM */\n");
    Fprintf(fp, "static char* nhtiles[] = {\n");
    Fprintf(fp, "\"%lu %lu %lu %d\",\n", header.tile_width * header.per_row,
            (header.tile_height * header.ntiles) / header.per_row,
            header.ncolors, 1 /* char per color */);
    for (i = 0; i < header.ncolors; i++)
        Fprintf(fp, "\"%c  c #%02x%02x%02x\",\n",
                (char) (i + '0'), /* just one char per color */
                x11_colormap[i][0], x11_colormap[i][1], x11_colormap[i][2]);

    n = 0;
    for (i = 0; i < (header.tile_height * header.ntiles) / header.per_row;
         i++) {
        Fprintf(fp, "\"");
        for (j = 0; j < header.tile_width * header.per_row; j++) {
            /* just one char per color */
            fputc(tile_bytes[n++] + '0', fp);
        }

        Fprintf(fp, "\",\n");
    }

    return fprintf(fp, "};\n") >= 0;
}
#endif /* USE_XPM */

int
main(int argc, char *argv[])
{
    FILE *fp;
    int i;

    header.version = 2; /* version 1 had no per_row field */
    header.ncolors = 0;
    header.tile_width = TILE_X;
    header.tile_height = TILE_Y;
    header.ntiles = 0; /* updated as we read in files */
    header.per_row = TILES_PER_ROW;

    if (argc == 1) {
        Fprintf(stderr,
                "usage: %s txt_file1 [txt_file2 ...] [-grayscale txt_fileN]\n",
                argv[0]);
        exit(1);
    }

    /* without this, the comparisons check uninitialized data and won't pass */
    objects_globals_init();
    monst_globals_init();

    fp = fopen(OUTNAME, "w");
    if (!fp) {
        Fprintf(stderr, "can't open output file\n");
        exit(1);
    }

    /* don't leave garbage at end of partial row */
    (void) memset((genericptr_t) tile_bytes, 0, sizeof(tile_bytes));

    for (i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "-grayscale", 10)) {
            set_grayscale(TRUE);
            if (i < (argc - 1)) i++;
        } else {
            set_grayscale(FALSE);
        }
        process_file(argv[i]);
    }
    Fprintf(stdout, "Total tiles: %ld\n", header.ntiles);

    /* round size up to the end of the row */
    if ((header.ntiles % header.per_row) != 0) {
        header.ntiles += header.per_row - (header.ntiles % header.per_row);
    }

#ifdef USE_XPM
    if (xpm_write(fp) == 0) {
        Fprintf(stderr, "can't write XPM file\n");
        exit(1);
    }
#else
    if (fwrite((char *) &header, sizeof(x11_header), 1, fp) == 0) {
        Fprintf(stderr, "can't open output header\n");
        exit(1);
    }

    if (fwrite((char *) x11_colormap, 1, header.ncolors * 3, fp) == 0) {
        Fprintf(stderr, "can't write output colormap\n");
        exit(1);
    }

    if (fwrite((char *) tile_bytes, 1,
               (int) header.ntiles * header.tile_width * header.tile_height,
               fp) == 0) {
        Fprintf(stderr, "can't write tile bytes\n");
        exit(1);
    }
#endif

    fclose(fp);
    return 0;
}

/*tile2X11.c*/


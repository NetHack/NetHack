/* NetHack 3.6	xpm2img.c	$NHDT-Date: 1432512809 2015/05/25 00:13:29 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/*   Copyright (c) Christian Bressler 2002                 */
/*   NetHack may be freely redistributed.  See license for details. */
/* This is mainly a reworked tile2bmp.c + xpm2iff.c -- Marvin */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "bitmfile.h"
#define TRUE 1
#define FALSE 0
void get_color(unsigned int colind, struct RGB *rgb);
void get_pixel(int x, int y, unsigned int *colind);
char *xpmgetline();
unsigned int **Bild_daten;
/* translation table from xpm characters to RGB and colormap slots */
struct Ttable {
    char flag;
    struct RGB col;
    int slot; /* output colortable index */
} ttable[256];
struct RGB *ColorMap;
int num_colors = 0;
int width = 0, height = 0;
int initflag;
FILE *fp;
int
main(argc, argv)
int argc;
char *argv[];
{
    int i;
    int row, col, planeno;
    int farben, planes;
    if (argc != 3) {
        fprintf(stderr, "usage: tile2img infile.xpm outfile.img\n");
        exit(EXIT_FAILURE);
    }
    initflag = 0;
    fp = fopen(argv[2], "wb");
    if (!fp) {
        printf("Error creating IMG-file %s, aborting.\n", argv[2]);
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    if (fopen_xpm_file(argv[1], "r") != TRUE) {
        printf("Error reading xpm-file %s, aborting.\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    Bild_daten =
        (unsigned int **) malloc((long) height * sizeof(unsigned int *));
    for (i = 0; i < height; i++)
        Bild_daten[i] =
            (unsigned int *) malloc((long) width * sizeof(unsigned int));
    for (row = 0; row < height; row++) {
        char *xb = xpmgetline();
        int plane_offset;
        if (xb == 0) {
            printf("Error to few lines in xpm-file %s, aborting.\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        for (col = 0; col < width; col++) {
            int color = xb[col];
            if (!ttable[color].flag)
                fprintf(stderr, "Bad image data\n");
            Bild_daten[row][col] = ttable[color].slot;
        }
    }
    if (num_colors > 256) {
        fprintf(stderr, "ERROR: zuviele Farben\n");
        exit(EXIT_FAILURE);
    } else if (num_colors > 16) {
        farben = 256;
        planes = 8;
    } else if (num_colors > 2) {
        farben = 16;
        planes = 4;
    } else {
        farben = 2;
        planes = 1;
    }
    bitmap_to_file(XIMG, width, height, 372, 372, planes, farben, argv[2],
                   get_color, get_pixel);
    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}
void
get_color(unsigned int colind, struct RGB *rgb)
{
    rgb->r = (1000L * (long) ColorMap[colind].r) / 0xFF;
    rgb->g = (1000L * (long) ColorMap[colind].g) / 0xFF;
    rgb->b = (1000L * (long) ColorMap[colind].b) / 0xFF;
}
void
get_pixel(int x, int y, unsigned int *colind)
{
    *colind = Bild_daten[y][x];
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
    if (4 != sscanf(xb, "%d %d %d %d", &width, &height, &num_colors, &temp))
        return FALSE; /* bad header */
                      /* replace the original buffer with one big enough for
                       * the real data
                       */
    /* XXX */
    xpmbuf = malloc(width * 2);
    if (!xpmbuf) {
        fprintf(stderr, "ERROR: Can't allocate line buffer\n");
        exit(1);
    }
    if (temp != 1)
        return FALSE; /* limitation of this code */
    {
        /* read the colormap and translation table */
        int ccount = -1;
        ColorMap =
            (struct RGB *) malloc((long) num_colors * sizeof(struct RGB));
        while (ccount++ < (num_colors - 1)) {
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
            ttable[index].col.r = r;
            ttable[index].col.g = g;
            ttable[index].col.b = b;
            ttable[index].slot = ccount;
            ColorMap[ccount].r = r;
            ColorMap[ccount].g = g;
            ColorMap[ccount].b = b;
        }
    }
    return TRUE;
}
/* This deserves better.  Don't read it too closely - you'll get ill. */
#define bufsz 2048
char buf[bufsz];
char *
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

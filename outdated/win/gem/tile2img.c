/* NetHack 3.6	tile2img.c	$NHDT-Date: 1432512809 2015/05/25 00:13:29 $  $NHDT-Branch: master $:$NHDT-Revision: 1.6 $ */
/*   Copyright (c) NetHack PC Development Team 1995                 */
/*   NetHack may be freely redistributed.  See license for details. */

/*
 * Edit History:
 *
 *	Initial Creation			M.Allison	94/01/11
 * Marvin was here 			Marvin		97/01/11
 *
 */

/* #include <stdlib.h>	*/
#include "hack.h"
#include "tile.h"
#include "bitmfile.h"

/* #define COLORS_IN_USE MAXCOLORMAPSIZE       /* 256 colors */
#define COLORS_IN_USE 16 /* 16 colors */

extern char *FDECL(tilename, (int, int));
static void FDECL(build_ximgtile, (pixel(*) [TILE_X]));
void get_color(unsigned int colind, struct RGB *rgb);
void get_pixel(int x, int y, unsigned int *colind);

#if COLORS_IN_USE == 16
#define MAX_X 320 /* 2 per byte, 4 bits per pixel */
#else
#define MAX_X 640
#endif
#define MAX_Y 1200

FILE *tibfile2;

pixel tilepixels[TILE_Y][TILE_X];

char *tilefiles[] = { "..\\win\\share\\monsters.txt",
                      "..\\win\\share\\objects.txt",
                      "..\\win\\share\\other.txt" };

unsigned int **Bild_daten;
int num_colors = 0;
int tilecount;
int max_tiles_in_row = 40;
int tiles_in_row;
int filenum;
int initflag;
int yoffset, xoffset;
char bmpname[128];
FILE *fp;

int
main(argc, argv)
int argc;
char *argv[];
{
    int i;

    if (argc != 2) {
        Fprintf(stderr, "usage: tile2img outfile.img\n");
        exit(EXIT_FAILURE);
    } else
        strcpy(bmpname, argv[1]);

#ifdef OBSOLETE
    bmpfile2 = fopen(NETHACK_PACKED_TILEFILE, WRBMODE);
    if (bmpfile2 == (FILE *) 0) {
        Fprintf(stderr, "Unable to open output file %s\n",
                NETHACK_PACKED_TILEFILE);
        exit(EXIT_FAILURE);
    }
#endif

    tilecount = 0;
    xoffset = yoffset = 0;
    initflag = 0;
    filenum = 0;
    fp = fopen(bmpname, "wb");
    if (!fp) {
        printf("Error creating tile file %s, aborting.\n", bmpname);
        exit(1);
    }
    fclose(fp);

    Bild_daten = (unsigned int **) malloc(MAX_Y * sizeof(unsigned int *));
    for (i = 0; i < MAX_Y; i++)
        Bild_daten[i] = (unsigned int *) malloc(MAX_X * sizeof(unsigned int));

    while (filenum < 3) {
        if (!fopen_text_file(tilefiles[filenum], RDTMODE)) {
            Fprintf(stderr, "usage: tile2img (from the util directory)\n");
            exit(EXIT_FAILURE);
        }
        num_colors = colorsinmap;
        if (num_colors > 62) {
            Fprintf(stderr, "too many colors (%d)\n", num_colors);
            exit(EXIT_FAILURE);
        }
        while (read_text_tile(tilepixels)) {
            build_ximgtile(tilepixels);
            tilecount++;
            xoffset += TILE_X;
            if (xoffset >= MAX_X) {
                yoffset += TILE_Y;
                xoffset = 0;
            }
        }
        (void) fclose_text_file();
        ++filenum;
    }
    Fprintf(stderr, "Total of %d tiles in memory.\n", tilecount);

    bitmap_to_file(XIMG, MAX_X, (tilecount / 20 + 1) * 16, 372, 372, 4, 16,
                   bmpname, get_color, get_pixel);

    Fprintf(stderr, "Total of %d tiles written to %s.\n", tilecount, bmpname);

    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

void
get_color(unsigned int colind, struct RGB *rgb)
{
    rgb->r = (1000L * (long) ColorMap[CM_RED][colind]) / 0xFF;
    rgb->g = (1000L * (long) ColorMap[CM_GREEN][colind]) / 0xFF;
    rgb->b = (1000L * (long) ColorMap[CM_BLUE][colind]) / 0xFF;
}

void
get_pixel(int x, int y, unsigned int *colind)
{
    *colind = Bild_daten[y][x];
}

static void
build_ximgtile(pixels)
pixel (*pixels)[TILE_X];
{
    int cur_x, cur_y, cur_color;
    int x, y;

    for (cur_y = 0; cur_y < TILE_Y; cur_y++) {
        for (cur_x = 0; cur_x < TILE_X; cur_x++) {
            for (cur_color = 0; cur_color < num_colors; cur_color++) {
                if (ColorMap[CM_RED][cur_color] == pixels[cur_y][cur_x].r
                    && ColorMap[CM_GREEN][cur_color] == pixels[cur_y][cur_x].g
                    && ColorMap[CM_BLUE][cur_color] == pixels[cur_y][cur_x].b)
                    break;
            }
            if (cur_color >= num_colors)
                Fprintf(stderr, "color not in colormap!\n");
            y = cur_y + yoffset;
            x = cur_x + xoffset;
            Bild_daten[y][x] = cur_color;
        }
    }
}

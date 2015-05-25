/* NetHack 3.6  ppmwrite.c	$NHDT-Date: 1432512803 2015/05/25 00:13:23 $  $NHDT-Branch: master $:$NHDT-Revision: 1.6 $ */
/* this produces a raw ppm file, with a 15-character header of
 * "P6 3-digit-width 3-digit-height 255\n"
 */

#include "config.h"
#include "tile.h"

#ifndef MONITOR_HEAP
extern long *FDECL(alloc, (unsigned int));
#endif

FILE *ppm_file;

struct ppmscreen {
    int Width;
    int Height;
} PpmScreen;

static int tiles_across, tiles_down, curr_tiles_across;
static pixel **image;

static void NDECL(write_header);
static void NDECL(WriteTileStrip);

static void
write_header()
{
    (void) fprintf(ppm_file, "P6 %03d %03d 255\n", PpmScreen.Width,
                   PpmScreen.Height);
}

static void
WriteTileStrip()
{
    int i, j;

    for (j = 0; j < TILE_Y; j++) {
        for (i = 0; i < PpmScreen.Width; i++) {
            (void) fputc((char) image[j][i].r, ppm_file);
            (void) fputc((char) image[j][i].g, ppm_file);
            (void) fputc((char) image[j][i].b, ppm_file);
        }
    }
}

boolean
fopen_ppm_file(filename, type)
const char *filename;
const char *type;
{
    int i;

    if (strcmp(type, WRBMODE)) {
        Fprintf(stderr, "using writing routine for non-writing?\n");
        return FALSE;
    }
    ppm_file = fopen(filename, type);
    if (ppm_file == (FILE *) 0) {
        Fprintf(stderr, "cannot open ppm file %s\n", filename);
        return FALSE;
    }

    if (!colorsinmainmap) {
        Fprintf(stderr, "no colormap set yet\n");
        return FALSE;
    }

    tiles_across = 20;
    curr_tiles_across = 0;
    PpmScreen.Width = 20 * TILE_X;

    tiles_down = 0;
    PpmScreen.Height = 0; /* will be rewritten later */

    write_header();

    image = (pixel **) alloc(TILE_Y * sizeof(pixel *));
    for (i = 0; i < TILE_Y; i++) {
        image[i] = (pixel *) alloc(PpmScreen.Width * sizeof(pixel));
    }

    return TRUE;
}

boolean
write_ppm_tile(pixels)
pixel (*pixels)[TILE_X];
{
    int i, j;

    for (j = 0; j < TILE_Y; j++) {
        for (i = 0; i < TILE_X; i++) {
            image[j][curr_tiles_across * TILE_X + i] = pixels[j][i];
        }
    }
    curr_tiles_across++;
    if (curr_tiles_across == tiles_across) {
        WriteTileStrip();
        curr_tiles_across = 0;
        tiles_down++;
    }
    return TRUE;
}

int
fclose_ppm_file()
{
    int i, j;

    if (curr_tiles_across) { /* partial row */
        /* fill with checkerboard, for lack of a better idea */
        for (j = 0; j < TILE_Y; j++) {
            for (i = curr_tiles_across * TILE_X; i < PpmScreen.Width;
                 i += 2) {
                image[j][i].r = MainColorMap[CM_RED][0];
                image[j][i].g = MainColorMap[CM_GREEN][0];
                image[j][i].b = MainColorMap[CM_BLUE][0];
                image[j][i + 1].r = MainColorMap[CM_RED][1];
                image[j][i + 1].g = MainColorMap[CM_GREEN][1];
                image[j][i + 1].b = MainColorMap[CM_BLUE][1];
            }
        }
        WriteTileStrip();
        curr_tiles_across = 0;
        tiles_down++;
    }

    for (i = 0; i < TILE_Y; i++) {
        free((genericptr_t) image[i]);
    }
    free((genericptr_t) image);

    PpmScreen.Height = tiles_down * TILE_Y;
    rewind(ppm_file);
    write_header(); /* update size */

    return (fclose(ppm_file));
}

int
main(argc, argv)
int argc;
char *argv[];
{
    pixel pixels[TILE_Y][TILE_X];

    if (argc != 3) {
        Fprintf(stderr, "usage: txt2ppm txtfile ppmfile\n");
        exit(EXIT_FAILURE);
    }

    if (!fopen_text_file(argv[1], RDTMODE))
        exit(EXIT_FAILURE);

    init_colormap();

    if (!fopen_ppm_file(argv[2], WRBMODE)) {
        (void) fclose_text_file();
        exit(EXIT_FAILURE);
    }

    while (read_text_tile(pixels))
        (void) write_ppm_tile(pixels);

    (void) fclose_text_file();
    (void) fclose_ppm_file();
    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

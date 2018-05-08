/* NetHack 3.6	tiletext.c	$NHDT-Date: 1524689272 2018/04/25 20:47:52 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.16 $ */
/*      Copyright (c) 2016 by Pasi Kallinen                       */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#include "tile.h"

pixval ColorMap[3][MAXCOLORMAPSIZE];
int colorsinmap;
pixval MainColorMap[3][MAXCOLORMAPSIZE];
int colorsinmainmap;

static short color_index[MAXCOLORMAPSIZE];
static int num_colors;
static char charcolors[MAXCOLORMAPSIZE];

static int placeholder_init = 0;
static pixel placeholder[TILE_Y][TILE_X];
static FILE *tile_file;
static int tile_set, tile_set_indx;
#if (TILE_X == 8)
static const char *text_sets[] = { "monthin.txt", "objthin.txt",
                                   "oththin.txt" };
#else
static const char *text_sets[] = { "monsters.txt", "objects.txt",
                                   "other.txt" };
#endif

extern const char *FDECL(tilename, (int, int));
extern boolean FDECL(acceptable_tilename, (int, const char *, const char *));
static void FDECL(read_text_colormap, (FILE *));
static boolean FDECL(write_text_colormap, (FILE *));
static boolean FDECL(read_txttile, (FILE *, pixel (*)[TILE_X]));
static void FDECL(write_txttile, (FILE *, pixel (*)[TILE_X]));

/* Ugh.  DICE doesn't like %[A-Z], so we have to spell it out... */
#define FORMAT_STRING                                                       \
    "%[ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.] = " \
    "(%d, %d, %d) "

static int grayscale = 0;
/* grayscale color mapping */
static const int graymappings[] = {
 /* .  A  B   C   D   E   F   G   H   I   J   K   L   M   N   O   P */
    0, 1, 17, 18, 19, 20, 27, 22, 23, 24, 25, 26, 21, 15, 13, 14, 14
};

void
set_grayscale(g)
int g;
{
    grayscale = g;
}

static void
read_text_colormap(txtfile)
FILE *txtfile;
{
    int i, r, g, b;
    char c[2];

    for (i = 0; i < MAXCOLORMAPSIZE; i++)
        color_index[i] = -1;

    num_colors = 0;
    while (fscanf(txtfile, FORMAT_STRING, c, &r, &g, &b) == 4) {
        color_index[(int) c[0]] = num_colors;
        ColorMap[CM_RED][num_colors] = r;
        ColorMap[CM_GREEN][num_colors] = g;
        ColorMap[CM_BLUE][num_colors] = b;
        num_colors++;
    }
    colorsinmap = num_colors;
}

#undef FORMAT_STRING

static boolean
write_text_colormap(txtfile)
FILE *txtfile;
{
    int i;
    char c;

    num_colors = colorsinmainmap;
    if (num_colors > 62) {
        Fprintf(stderr, "too many colors (%d)\n", num_colors);
        return FALSE;
    }
    for (i = 0; i < num_colors; i++) {
        if (i < 26)
            c = 'A' + i;
        else if (i < 52)
            c = 'a' + i - 26;
        else
            c = '0' + i - 52;

        charcolors[i] = c;
        Fprintf(
            txtfile, "%c = (%d, %d, %d)\n", c, (int) MainColorMap[CM_RED][i],
            (int) MainColorMap[CM_GREEN][i], (int) MainColorMap[CM_BLUE][i]);
    }
    return TRUE;
}

static boolean
read_txttile(txtfile, pixels)
FILE *txtfile;
pixel (*pixels)[TILE_X];
{
    int ph, i, j, k;
    char buf[BUFSZ], ttype[BUFSZ];
    const char *p;
    char c[2];

    if (fscanf(txtfile, "# %s %d (%[^)])", ttype, &i, buf) <= 0)
        return FALSE;

    ph = strcmp(ttype, "placeholder") == 0;

    if (!ph && strcmp(ttype, "tile") != 0)
        Fprintf(stderr, "Keyword \"%s\" unexpected for entry %d\n", ttype, i);

    if (tile_set != 0) {
        /* check tile name, but not relative number, which will
         * change when tiles are added
         */
        p = tilename(tile_set, tile_set_indx);
        if (p && strcmp(p, buf) && !acceptable_tilename(tile_set_indx,buf,p)) {
            Fprintf(stderr, "warning: for tile %d (numbered %d) of %s,\n",
                    tile_set_indx, i, text_sets[tile_set - 1]);
            Fprintf(stderr, "\tfound '%s' while expecting '%s'\n", buf, p);
        }
    }
    tile_set_indx++;

    /* look for non-whitespace at each stage */
    if (fscanf(txtfile, "%1s", c) < 0) {
        Fprintf(stderr, "unexpected EOF\n");
        return FALSE;
    }
    if (c[0] != '{') {
        Fprintf(stderr, "didn't find expected '{'\n");
        return FALSE;
    }
    for (j = 0; j < TILE_Y; j++) {
        for (i = 0; i < TILE_X; i++) {
            if (fscanf(txtfile, "%1s", c) < 0) {
                Fprintf(stderr, "unexpected EOF\n");
                return FALSE;
            }
            k = color_index[(int) c[0]];
            if (grayscale) {
                if (k > (SIZE(graymappings) - 1))
                    Fprintf(stderr, "Gray mapping issue %d > %d.\n", k,
                            SIZE(graymappings) - 1);
                else
                    k = graymappings[k];
            }
            if (k == -1)
                Fprintf(stderr, "color %c not in colormap!\n", c[0]);
            else {
                pixels[j][i].r = ColorMap[CM_RED][k];
                pixels[j][i].g = ColorMap[CM_GREEN][k];
                pixels[j][i].b = ColorMap[CM_BLUE][k];
            }
        }
    }
    if (ph) {
        /* remember it for later */
        memcpy(placeholder, pixels, sizeof(placeholder));
    }
    if (fscanf(txtfile, "%1s ", c) < 0) {
        Fprintf(stderr, "unexpected EOF\n");
        return FALSE;
    }
    if (c[0] != '}') {
        Fprintf(stderr, "didn't find expected '}'\n");
        return FALSE;
    }
#ifdef _DCC
    /* DICE again... it doesn't seem to eat whitespace after the } like
     * it should, so we have to do so manually.
     */
    while ((*c = fgetc(txtfile)) != EOF && isspace((uchar) *c))
        ;
    ungetc(*c, txtfile);
#endif
    return TRUE;
}

static void
write_txttile(txtfile, pixels)
FILE *txtfile;
pixel (*pixels)[TILE_X];
{
    const char *p;
    const char *type;
    int i, j, k;

    if (memcmp(placeholder, pixels, sizeof(placeholder)) == 0)
        type = "placeholder";
    else
        type = "tile";

    if (tile_set == 0)
        Fprintf(txtfile, "# %s %d (unknown)\n", type, tile_set_indx);
    else {
        p = tilename(tile_set, tile_set_indx);
        if (p)
            Fprintf(txtfile, "# %s %d (%s)\n", type, tile_set_indx, p);
        else
            Fprintf(txtfile, "# %s %d (null)\n", type, tile_set_indx);
    }
    tile_set_indx++;

    Fprintf(txtfile, "{\n");
    for (j = 0; j < TILE_Y; j++) {
        Fprintf(txtfile, "  ");
        for (i = 0; i < TILE_X; i++) {
            for (k = 0; k < num_colors; k++) {
                if (ColorMap[CM_RED][k] == pixels[j][i].r
                    && ColorMap[CM_GREEN][k] == pixels[j][i].g
                    && ColorMap[CM_BLUE][k] == pixels[j][i].b)
                    break;
            }
            if (k >= num_colors)
                Fprintf(stderr, "color not in colormap!\n");
            (void) fputc(charcolors[k], txtfile);
        }
        Fprintf(txtfile, "\n");
    }
    Fprintf(txtfile, "}\n");
}

/* initialize main colormap from globally accessed ColorMap */
void
init_colormap()
{
    int i;

    colorsinmainmap = colorsinmap;
    for (i = 0; i < colorsinmap; i++) {
        MainColorMap[CM_RED][i] = ColorMap[CM_RED][i];
        MainColorMap[CM_GREEN][i] = ColorMap[CM_GREEN][i];
        MainColorMap[CM_BLUE][i] = ColorMap[CM_BLUE][i];
    }
}

/* merge new colors from ColorMap into MainColorMap */
void
merge_colormap()
{
    int i, j;

    for (i = 0; i < colorsinmap; i++) {
        for (j = 0; j < colorsinmainmap; j++) {
            if (MainColorMap[CM_RED][j] == ColorMap[CM_RED][i]
                && MainColorMap[CM_GREEN][j] == ColorMap[CM_GREEN][i]
                && MainColorMap[CM_BLUE][j] == ColorMap[CM_BLUE][i])
                break;
        }
        if (j >= colorsinmainmap) { /* new color */
            if (colorsinmainmap >= MAXCOLORMAPSIZE) {
                Fprintf(stderr,
                        "Too many colors to merge -- excess ignored.\n");
            }
            j = colorsinmainmap;
            MainColorMap[CM_RED][j] = ColorMap[CM_RED][i];
            MainColorMap[CM_GREEN][j] = ColorMap[CM_GREEN][i];
            MainColorMap[CM_BLUE][j] = ColorMap[CM_BLUE][i];
            colorsinmainmap++;
        }
    }
}

boolean
fopen_text_file(filename, type)
const char *filename;
const char *type;
{
    const char *p;
    int i;

    if (tile_file != (FILE *) 0) {
        Fprintf(stderr, "can only open one text file at at time\n");
        return FALSE;
    }

    tile_file = fopen(filename, type);
    if (tile_file == (FILE *) 0) {
        Fprintf(stderr, "cannot open text file %s\n", filename);
        return FALSE;
    }

    p = rindex(filename, '/');
    if (p)
        p++;
    else
        p = filename;

    tile_set = 0;
    for (i = 0; i < SIZE(text_sets); i++) {
        if (!strcmp(p, text_sets[i]))
            tile_set = i + 1;
    }
    tile_set_indx = 0;

    if (!strcmp(type, RDTMODE)) {
        /* Fill placeholder with noise */
        if (!placeholder_init) {
            placeholder_init++;
            for (i = 0; i < (int) sizeof placeholder; i++)
                ((char *) placeholder)[i] = i % 256;
        }

        read_text_colormap(tile_file);
        if (!colorsinmainmap)
            init_colormap();
        else
            merge_colormap();
        return TRUE;
    } else if (!strcmp(type, WRTMODE)) {
        if (!colorsinmainmap) {
            Fprintf(stderr, "no colormap set yet\n");
            return FALSE;
        }
        return (write_text_colormap(tile_file));
    } else {
        Fprintf(stderr, "bad mode (%s) for fopen_text_file\n", type);
        return FALSE;
    }
}

boolean
read_text_tile(pixels)
pixel (*pixels)[TILE_X];
{
    return (read_txttile(tile_file, pixels));
}

boolean
write_text_tile(pixels)
pixel (*pixels)[TILE_X];
{
    write_txttile(tile_file, pixels);
    return TRUE;
}

int
fclose_text_file()
{
    int ret;

    ret = fclose(tile_file);
    tile_file = (FILE *) 0;
    return ret;
}

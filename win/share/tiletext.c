/* NetHack 3.7	tiletext.c	$NHDT-Date: 1596498342 2020/08/03 23:45:42 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.17 $ */
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
static const char *const text_sets[] = {
#if (TILE_X == 8)
    "monthin.txt", "objthin.txt", "oththin.txt"
#else
    "monsters.txt", "objects.txt", "other.txt"
#endif
};

static char inbuf[BUFSZ];

extern const char *tilename(int, int, int);
extern boolean acceptable_tilename(int, int, const char *, const char *);
static int get_next_line(FILE *, boolean);
static void read_text_colormap(FILE *);
static boolean write_text_colormap(FILE *);
static boolean read_txttile(FILE *, pixel (*)[TILE_X]);
static void write_txttile(FILE *, pixel (*)[TILE_X]);

enum { MONSTER_SET, OBJECT_SET, OTHER_SET};

/* Ugh.  DICE doesn't like %[A-Z], so we have to spell it out... */
#define FORMAT_STRING \
    "%1[ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.] = " \
    "(%d, %d, %d) "

/*
-----------------------------------------------------------------------------
    Tile palette Feb 9, 2022

    [0]  . = (71, 108, 108)
    [1]  A = (0, 0, 0)        grayshade   maps to [1] itself
    [2]  B = (0, 182, 255)                maps to [17] or Q for shade of gray
    [3]  C = (255, 108, 0)                maps to [18] or R for shade of gray
    [4]  D = (255, 0, 0)                  maps to [19] or S for shade of gray
    [5]  E = (0, 0, 255)                  maps to [20] or T for shade of gray
    [6]  F = (0, 145, 0)                  maps to [27] or 0 for shade of gray
    [7]  G = (108, 255, 0)                maps to [22] or V for shade of gray
    [8]  H = (255, 255, 0)                maps to [23] or W for shade of gray
    [9]  I = (255, 0, 255)                maps to [24] or X for shade of gray
    [10] J = (145, 71, 0)                 maps to [25] or Y for shade of gray
    [11] K = (204, 79, 0)                 maps to [26] or Z for shade of gray
    [12] L = (255, 182, 145)              maps to [21] or U for shade of gray
    [13] M = (237, 237, 237)  grayshade   maps to [15] or O for shade of gray
    [14] N = (255, 255, 255)  grayshade   maps to [13] or M for shade of gray
    [15] O = (215, 215, 215)  grayshade   maps to [14] or N for shade of gray
    [16] P = (108, 145, 182)              maps to [14] or N for shade of gray
    [17] Q = (18, 18, 18)     grayshade   maps to [1]  or A for shade of gray
    [18] R = (54, 54, 54)     grayshade   maps to [17] or Q for shade of gray
    [19] S = (73, 73, 73)     grayshade   maps to [18] or R for shade of gray
    [20] T = (82, 82, 82)     grayshade   maps to [19] or S for shade of gray
    [21] U = (205,205,205)    grayshade   maps to [20] or T for shade of gray
    [22] V = (104, 104, 104)  grayshade   maps to [27] or 0 for shade of gray
    [23] W = (131, 131, 131)  grayshade   maps to [22] or V for shade of gray
    [24] X = (140, 140, 140)  grayshade   not mapped in graymappings
    [25] Y = (149, 149, 149)  grayshade   not mapped in graymappings
    [26] Z = (195, 195, 195)  grayshade   not mapped in graymappings
    [27] 0 = (100, 100, 100)  grayshade   not mapped in graymappings
    [28] 1 = (72, 108, 108)               not mapped in graymappings
-----------------------------------------------------------------------------
*/

static int grayscale = 0;
/* grayscale color mapping */
static const int graymappings[] = {
 /* .  A  B   C   D   E   F   G   H   I   J   K   L   M   N   O   P */
    0, 1, 17, 18, 19, 20, 27, 22, 23, 24, 25, 26, 21, 15, 13, 14, 14,
 /* Q  R   S   T   U   V   W */
    1, 17, 18, 19, 20, 27, 22
};

void
set_grayscale(int gs)
{
    grayscale = gs;
}

/* read next line; repeat until it's a non-comment; populates 'inbuf[]';
   the buffer might already have data (first line after colormap)  */
static int
get_next_line(FILE *txtfile, boolean force)
{
    int ch;

    for (;;) {
        if (force || !inbuf[0]) {
            /* skip leading whitespace */
            do {
                ch = fgetc(txtfile);
            } while (ch == ' ');
            ungetc(ch, txtfile);
            /* get rest of line */
            if (!fgets(inbuf, BUFSZ, txtfile))
                break;
            force = TRUE;
            /* ignore blank lines;
               the old fscanf() processing did that, possibly by accident */
            if (!inbuf[0] || (inbuf[0] == '\n' && !inbuf[1]))
                continue;
        }
        if (inbuf[0] != '#' || !strncmp(inbuf, "# tile ", 7))
            return 1;
    }
    inbuf[0] = '\0';
    return 0;
}

static void
read_text_colormap(FILE *txtfile)
{
    int i, r, g, b;
    char c[2];

    for (i = 0; i < MAXCOLORMAPSIZE; i++)
        color_index[i] = -1;

    num_colors = 0;
    while (get_next_line(txtfile, TRUE)) {
        if (sscanf(inbuf, FORMAT_STRING, c, &r, &g, &b) != 4)
            break;
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
write_text_colormap(FILE *txtfile)
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

/* read one tile from win/share/{monsters,objects,other}.txt */
static boolean
read_txttile(FILE *txtfile, pixel (*pixels)[TILE_X])
{
    static int gidx = 0;
    int ph, i = 0, j, k, reslt;
    char buf[BUFSZ], ttype[BUFSZ], gend[BUFSZ];
    const char *p;
    char *q;
    boolean res = FALSE;

    gend[0] = '\0';
    reslt = 0;
    if (get_next_line(txtfile, FALSE)) {
        if (tile_set == MONSTER_SET)
            reslt = sscanf(inbuf, "# %255s %d (%255[^,],%255[^)])",
                           ttype, &i, buf, gend);
        else
            reslt = sscanf(inbuf, "# %255s %d (%255[^)])",
                           ttype, &i, buf);
    }
    if (reslt <= 0)
        goto done;

    if (tile_set == MONSTER_SET && gend[0] == 'f')
        gidx = 1;

    ph = strcmp(ttype, "placeholder") == 0;

    if (!ph && strcmp(ttype, "tile") != 0)
        Fprintf(stderr, "Keyword \"%s\" unexpected for entry %d\n", ttype, i);

    /* check tile name, but not relative number, which will
     * change when tiles are added
     */
    p = tilename(tile_set, tile_set_indx, gidx);
    if (p && (q = strstr(p, " {")) != 0) {
        *q = '\0';
    }
    if (p && strcmp(p, buf)) {
        boolean other_mismatch =
            (tile_set == OTHER_SET
             && !acceptable_tilename(tile_set, tile_set_indx, buf, p));

        if (tile_set != OTHER_SET || other_mismatch) {
            Fprintf(stderr, "warning: for tile %d (numbered %d) of %s,\n",
                    tile_set_indx, i, text_sets[tile_set]);
            Fprintf(stderr, "\tfound '%s' while expecting '%s'\n", buf, p);
        }
    }
    tile_set_indx++;

    /* look for non-whitespace at each stage */
    if (!get_next_line(txtfile, TRUE)) {
        Fprintf(stderr, "unexpected EOF\n");
        goto done;
    }
    if (inbuf[0] != '{') {
        Fprintf(stderr, "didn't find expected '{'\n");
        goto done;
    }

    for (j = 0; j < TILE_Y; j++) {
        /* read next line of color indices */
        if (!get_next_line(txtfile, TRUE)) {
            Fprintf(stderr, "unexpected EOF\n");
            goto done;
        }
        if ((q = strchr(inbuf, '\n')) == 0)
            q = &inbuf[strlen(inbuf)]; /* eos(inbuf) */
        while (q < &inbuf[TILE_X])
            *q++ = '.';
        *q = '\0';

        for (i = 0; i < TILE_X; i++) {
            k = color_index[(unsigned char) inbuf[i]];
            if (grayscale && k >= 0) {
                if (k > (SIZE(graymappings) - 1))
                    Fprintf(stderr, "Gray mapping issue %d > %d.\n", k,
                            SIZE(graymappings) - 1);
                else
                    k = graymappings[k];
            }
            if (k == -1) {
                Fprintf(stderr, "color %c not in colormap!\n", inbuf[i]);
            } else {
                pixels[j][i].r = ColorMap[CM_RED][k];
                pixels[j][i].g = ColorMap[CM_GREEN][k];
                pixels[j][i].b = ColorMap[CM_BLUE][k];
            }
        }
    }
    if (ph) {
        /* remember it for later */
        memcpy(placeholder, pixels, sizeof placeholder);
    }
    if (!get_next_line(txtfile, TRUE)) {
        Fprintf(stderr, "unexpected EOF\n");
        goto done;
    }
    if (inbuf[0] != '}') {
        Fprintf(stderr, "didn't find expected '}'\n");
        goto done;
    }
    res = TRUE;
 done:
    inbuf[0] = '\0';
    return res;
}

static void
write_txttile(FILE *txtfile, pixel (*pixels)[TILE_X])
{
    const char *p;
    const char *type;
    int i = 0, j, k;

    if (memcmp(placeholder, pixels, sizeof(placeholder)) == 0)
        type = "placeholder";
    else
        type = "tile";

    if (tile_set == MONSTER_SET) {
        for (i = 0; i < 2; ++i) {
            Fprintf(txtfile, "# %s %d (unknown,%s)\n", type, tile_set_indx,
                    i ? "female" : "male");
            if (i == 0)
                tile_set_indx++;
        }
    } else {
            p = tilename(tile_set, tile_set_indx, i);
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
init_colormap(void)
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
merge_colormap(void)
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
fopen_text_file(const char *filename, const char *type)
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

    p = strrchr(filename, '/');
    if (p)
        p++;
    else
        p = filename;

    tile_set = 0;
    for (i = 0; i < SIZE(text_sets); i++) {
        if (!strcmp(p, text_sets[i]))
            tile_set = i;
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
read_text_tile(pixel (*pixels)[TILE_X])
{
    return read_txttile(tile_file, pixels);
}

boolean
write_text_tile(pixel (*pixels)[TILE_X])
{
    write_txttile(tile_file, pixels);
    return TRUE;
}

int
fclose_text_file(void)
{
    int ret;

    ret = fclose(tile_file);
    tile_file = (FILE *) 0;
    return ret;
}

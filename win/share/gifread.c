/* GIF reading routines based on those in pbmplus:ppm/giftoppm.c, bearing
 * following copyright notice:
 */

/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, David Koblas.                                     | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */

/*
 * $NHDT-Date: 1432512803 2015/05/25 00:13:23 $  $NHDT-Branch: master $:$NHDT-Revision: 1.5 $
 */

#include "config.h"
#include "tile.h"

#ifndef MONITOR_HEAP
extern long *alloc(unsigned int);
#endif

#define PPM_ASSIGN(p, red, grn, blu) \
    do {                             \
        (p).r = (red);               \
        (p).g = (grn);               \
        (p).b = (blu);               \
    } while (0)

#define MAX_LWZ_BITS 12

#define INTERLACE 0x40
#define LOCALCOLORMAP 0x80
#define BitSet(byte, bit) (((byte) & (bit)) == (bit))

#define ReadOK(file, buffer, len) \
    (fread((genericptr_t) buffer, (int) len, 1, file) != 0)

#define LM_to_uint(a, b) (((b) << 8) | (a))

struct gifscreen {
    int Width;
    int Height;
    int Colors;
    int ColorResolution;
    int Background;
    int AspectRatio;
    int Interlace;
} GifScreen;

struct {
    int transparent;
    int delayTime;
    int inputFlag;
    int disposal;
} Gif89 = { -1, -1, -1, 0 };

int ZeroDataBlock = FALSE;

static FILE *gif_file;
static int tiles_across, tiles_down, curr_tiles_across, curr_tiles_down;
static pixel **image;
static unsigned char input_code_size;

static int GetDataBlock(FILE * fd, unsigned char *buf);
static void DoExtension(FILE * fd, int label);
static boolean ReadColorMap(FILE * fd, int number);
static void read_header(FILE * fd);
static int GetCode(FILE * fd, int code_size, int flag);
static int LWZReadByte(FILE * fd, int flag); /*, int input_code_size);*/
static void ReadInterleavedImage(FILE * fd, int len, int height);
static void ReadTileStrip(FILE * fd, int len);

/* These should be in gif.h, but there isn't one. */
boolean fopen_gif_file(const char *, const char *);
boolean read_gif_tile(pixel(*) [TILE_X]);
int fclose_gif_file(void);

static int
GetDataBlock(FILE *fd, unsigned char *buf)
{
    unsigned char count;

    if (!ReadOK(fd, &count, 1)) {
        Fprintf(stderr, "error in getting DataBlock size\n");
        return -1;
    }

    ZeroDataBlock = (count == 0);

    if ((count != 0) && (!ReadOK(fd, buf, count))) {
        Fprintf(stderr, "error in reading DataBlock\n");
        return -1;
    }

    return count;
}

static void
DoExtension(FILE *fd, int label)
{
    static char buf[256];
    const char *str;

    switch (label) {
    case 0x01: /* Plain Text Extension */
        str = "Plain Text Extension";
#ifdef notdef
        if (GetDataBlock(fd, (unsigned char *) buf) == 0)
            ;

        lpos = LM_to_uint(buf[0], buf[1]);
        tpos = LM_to_uint(buf[2], buf[3]);
        width = LM_to_uint(buf[4], buf[5]);
        height = LM_to_uint(buf[6], buf[7]);
        cellw = buf[8];
        cellh = buf[9];
        foreground = buf[10];
        background = buf[11];

        while (GetDataBlock(fd, (unsigned char *) buf) != 0) {
            PPM_ASSIGN(image[ypos][xpos], cmap[CM_RED][v], cmap[CM_GREEN][v],
                       cmap[CM_BLUE][v]);
            ++index;
        }

        return;
#else
        break;
#endif
    case 0xff: /* Application Extension */
        str = "Application Extension";
        break;
    case 0xfe: /* Comment Extension */
        str = "Comment Extension";
        while (GetDataBlock(fd, (unsigned char *) buf) != 0) {
            Fprintf(stderr, "gif comment: %s\n", buf);
        }
        return;
    case 0xf9: /* Graphic Control Extension */
        str = "Graphic Control Extension";
        (void) GetDataBlock(fd, (unsigned char *) buf);
        Gif89.disposal = (buf[0] >> 2) & 0x7;
        Gif89.inputFlag = (buf[0] >> 1) & 0x1;
        Gif89.delayTime = LM_to_uint(buf[1], buf[2]);
        if ((buf[0] & 0x1) != 0)
            Gif89.transparent = buf[3];

        while (GetDataBlock(fd, (unsigned char *) buf) != 0)
            ;
        return;
    default:
        str = buf;
        Sprintf(buf, "UNKNOWN (0x%02x)", label);
        break;
    }

    Fprintf(stderr, "got a '%s' extension\n", str);

    while (GetDataBlock(fd, (unsigned char *) buf) != 0)
        ;
}

static boolean
ReadColorMap(FILE *fd, int number)
{
    int i;
    unsigned char rgb[3];

    for (i = 0; i < number; ++i) {
        if (!ReadOK(fd, rgb, sizeof(rgb))) {
            return (FALSE);
        }

        ColorMap[CM_RED][i] = rgb[0];
        ColorMap[CM_GREEN][i] = rgb[1];
        ColorMap[CM_BLUE][i] = rgb[2];
    }
    colorsinmap = number;
    return TRUE;
}

/*
 * Read gif header, including colormaps.  We expect only one image per
 * file, so if that image has a local colormap, overwrite the global one.
 */
static void
read_header(FILE *fd)
{
    unsigned char buf[16];
    unsigned char c;
    char version[4];

    if (!ReadOK(fd, buf, 6)) {
        Fprintf(stderr, "error reading magic number\n");
        exit(EXIT_FAILURE);
    }

    if (strncmp((genericptr_t) buf, "GIF", 3) != 0) {
        Fprintf(stderr, "not a GIF file\n");
        exit(EXIT_FAILURE);
    }

    (void) strncpy(version, (char *) buf + 3, 3);
    version[3] = '\0';

    if ((strcmp(version, "87a") != 0) && (strcmp(version, "89a") != 0)) {
        Fprintf(stderr, "bad version number, not '87a' or '89a'\n");
        exit(EXIT_FAILURE);
    }

    if (!ReadOK(fd, buf, 7)) {
        Fprintf(stderr, "failed to read screen descriptor\n");
        exit(EXIT_FAILURE);
    }

    GifScreen.Width = LM_to_uint(buf[0], buf[1]);
    GifScreen.Height = LM_to_uint(buf[2], buf[3]);
    GifScreen.Colors = 2 << (buf[4] & 0x07);
    GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
    GifScreen.Background = buf[5];
    GifScreen.AspectRatio = buf[6];

    if (BitSet(buf[4], LOCALCOLORMAP)) { /* Global Colormap */
        if (!ReadColorMap(fd, GifScreen.Colors)) {
            Fprintf(stderr, "error reading global colormap\n");
            exit(EXIT_FAILURE);
        }
    }

    if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49) {
        Fprintf(stderr, "warning - non-square pixels\n");
    }

    for (;;) {
        if (!ReadOK(fd, &c, 1)) {
            Fprintf(stderr, "EOF / read error on image data\n");
            exit(EXIT_FAILURE);
        }

        if (c == ';') { /* GIF terminator */
            return;
        }

        if (c == '!') { /* Extension */
            if (!ReadOK(fd, &c, 1)) {
                Fprintf(stderr,
                        "EOF / read error on extension function code\n");
                exit(EXIT_FAILURE);
            }
            DoExtension(fd, (int) c);
            continue;
        }

        if (c != ',') { /* Not a valid start character */
            Fprintf(stderr, "bogus character 0x%02x, ignoring\n", (int) c);
            continue;
        }

        if (!ReadOK(fd, buf, 9)) {
            Fprintf(stderr, "couldn't read left/top/width/height\n");
            exit(EXIT_FAILURE);
        }

        if (BitSet(buf[8], LOCALCOLORMAP)) {
            /* replace global color map with local */
            GifScreen.Colors = 1 << ((buf[8] & 0x07) + 1);
            if (!ReadColorMap(fd, GifScreen.Colors)) {
                Fprintf(stderr, "error reading local colormap\n");
                exit(EXIT_FAILURE);
            }
        }
        if (GifScreen.Width != LM_to_uint(buf[4], buf[5])) {
            Fprintf(stderr, "warning: widths don't match\n");
            GifScreen.Width = LM_to_uint(buf[4], buf[5]);
        }
        if (GifScreen.Height != LM_to_uint(buf[6], buf[7])) {
            Fprintf(stderr, "warning: heights don't match\n");
            GifScreen.Height = LM_to_uint(buf[6], buf[7]);
        }
        GifScreen.Interlace = BitSet(buf[8], INTERLACE);
        return;
    }
}

static int
GetCode(FILE *fd, int code_size, int flag)
{
    static unsigned char buf[280];
    static int curbit, lastbit, done, last_byte;
    int i, j, ret;
    unsigned char count;

    if (flag) {
        curbit = 0;
        lastbit = 0;
        done = FALSE;
        return 0;
    }

    if ((curbit + code_size) >= lastbit) {
        if (done) {
            if (curbit >= lastbit)
                Fprintf(stderr, "ran off the end of my bits\n");
            return -1;
        }
        buf[0] = buf[last_byte - 2];
        buf[1] = buf[last_byte - 1];

        if ((count = GetDataBlock(fd, &buf[2])) == 0)
            done = TRUE;

        last_byte = 2 + count;
        curbit = (curbit - lastbit) + 16;
        lastbit = (2 + count) * 8;
    }

    ret = 0;
    for (i = curbit, j = 0; j < code_size; ++i, ++j)
        ret |= ((buf[i / 8] & (1 << (i % 8))) != 0) << j;

    curbit += code_size;

    return ret;
}

static int
LWZReadByte(FILE *fd, int flag) /*, int input_code_size)*/
{
    static int fresh = FALSE;
    int code, incode;
    static int code_size, set_code_size;
    static int max_code, max_code_size;
    static int firstcode, oldcode;
    static int clear_code, end_code;
    static int table[2][(1 << MAX_LWZ_BITS)];
    static int stack[(1 << (MAX_LWZ_BITS)) * 2], *sp;
    register int i;

    if (flag) {
        set_code_size = input_code_size;
        code_size = set_code_size + 1;
        clear_code = 1 << set_code_size;
        end_code = clear_code + 1;
        max_code_size = 2 * clear_code;
        max_code = clear_code + 2;

        (void) GetCode(fd, 0, TRUE);

        fresh = TRUE;

        for (i = 0; i < clear_code; ++i) {
            table[0][i] = 0;
            table[1][i] = i;
        }
        for (; i < (1 << MAX_LWZ_BITS); ++i)
            table[0][i] = table[1][0] = 0;

        sp = stack;

        return 0;
    } else if (fresh) {
        fresh = FALSE;
        do {
            firstcode = oldcode = GetCode(fd, code_size, FALSE);
        } while (firstcode == clear_code);
        return firstcode;
    }

    if (sp > stack)
        return *--sp;

    while ((code = GetCode(fd, code_size, FALSE)) >= 0) {
        if (code == clear_code) {
            for (i = 0; i < clear_code; ++i) {
                table[0][i] = 0;
                table[1][i] = i;
            }
            for (; i < (1 << MAX_LWZ_BITS); ++i)
                table[0][i] = table[1][i] = 0;
            code_size = set_code_size + 1;
            max_code_size = 2 * clear_code;
            max_code = clear_code + 2;
            sp = stack;
            firstcode = oldcode = GetCode(fd, code_size, FALSE);
            return firstcode;
        } else if (code == end_code) {
            int count;
            unsigned char buf[260];

            if (ZeroDataBlock)
                return -2;

            while ((count = GetDataBlock(fd, buf)) > 0)
                ;

            if (count != 0)
                Fprintf(stderr,
                        "missing EOD in data stream (common occurrence)\n");
            return -2;
        }

        incode = code;

        if (code >= max_code) {
            *sp++ = firstcode;
            code = oldcode;
        }

        while (code >= clear_code) {
            *sp++ = table[1][code];
            if (code == table[0][code]) {
                Fprintf(stderr, "circular table entry BIG ERROR\n");
                exit(EXIT_FAILURE);
            }
            code = table[0][code];
        }

        *sp++ = firstcode = table[1][code];

        if ((code = max_code) < (1 << MAX_LWZ_BITS)) {
            table[0][code] = oldcode;
            table[1][code] = firstcode;
            ++max_code;
            if ((max_code >= max_code_size)
                && (max_code_size < (1 << MAX_LWZ_BITS))) {
                max_code_size *= 2;
                ++code_size;
            }
        }

        oldcode = incode;

        if (sp > stack)
            return *--sp;
    }
    return code;
}

static void
ReadInterleavedImage(FILE *fd, int len, int height)
{
    int v;
    int xpos = 0, ypos = 0, pass = 0;

    while ((v = LWZReadByte(fd, FALSE /*, (int) input_code_size*/ )) >= 0) {
        PPM_ASSIGN(image[ypos][xpos], ColorMap[CM_RED][v],
                   ColorMap[CM_GREEN][v], ColorMap[CM_BLUE][v]);

        ++xpos;
        if (xpos == len) {
            xpos = 0;
            switch (pass) {
            case 0:
            case 1:
                ypos += 8;
                break;
            case 2:
                ypos += 4;
                break;
            case 3:
                ypos += 2;
                break;
            }

            if (ypos >= height) {
                ++pass;
                switch (pass) {
                case 1:
                    ypos = 4;
                    break;
                case 2:
                    ypos = 2;
                    break;
                case 3:
                    ypos = 1;
                    break;
                default:
                    goto fini;
                }
            }
        }
        if (ypos >= height)
            break;
    }

 fini:
    if (LWZReadByte(fd, FALSE /*, (int) input_code_size*/ ) >= 0)
        Fprintf(stderr, "too much input data, ignoring extra...\n");
}

static void
ReadTileStrip(FILE *fd, int len)
{
    int v;
    int xpos = 0, ypos = 0;

    while ((v = LWZReadByte(fd, FALSE /*, (int) input_code_size*/ )) >= 0) {
        PPM_ASSIGN(image[ypos][xpos], ColorMap[CM_RED][v],
                   ColorMap[CM_GREEN][v], ColorMap[CM_BLUE][v]);

        ++xpos;
        if (xpos == len) {
            xpos = 0;
            ++ypos;
        }
        if (ypos >= TILE_Y)
            break;
    }
}

boolean
fopen_gif_file(const char *filename, const char *type)
{
    int i;

    if (strcmp(type, RDBMODE)) {
        Fprintf(stderr, "using reading routine for non-reading?\n");
        return FALSE;
    }
    gif_file = fopen(filename, type);
    if (gif_file == (FILE *) 0) {
        Fprintf(stderr, "cannot open gif file %s\n", filename);
        return FALSE;
    }

    read_header(gif_file);
    if (GifScreen.Width % TILE_X) {
        Fprintf(stderr, "error: width %d not divisible by %d\n",
                GifScreen.Width, TILE_X);
        exit(EXIT_FAILURE);
    }
    tiles_across = GifScreen.Width / TILE_X;
    curr_tiles_across = 0;
    if (GifScreen.Height % TILE_Y) {
        Fprintf(stderr, "error: height %d not divisible by %d\n",
                GifScreen.Height, TILE_Y);
        /* exit(EXIT_FAILURE) */;
    }
    tiles_down = GifScreen.Height / TILE_Y;
    curr_tiles_down = 0;

    if (GifScreen.Interlace) {
        /* sigh -- hope this doesn't happen on micros */
        image = (pixel **) alloc(GifScreen.Height * sizeof(pixel *));
        for (i = 0; i < GifScreen.Height; i++) {
            image[i] = (pixel *) alloc(GifScreen.Width * sizeof(pixel));
        }
    } else {
        image = (pixel **) alloc(TILE_Y * sizeof(pixel *));
        for (i = 0; i < TILE_Y; i++) {
            image[i] = (pixel *) alloc(GifScreen.Width * sizeof(pixel));
        }
    }

    /*
    **  Initialize the Compression routines
    */
    if (!ReadOK(gif_file, &input_code_size, 1)) {
        Fprintf(stderr, "EOF / read error on image data\n");
        exit(EXIT_FAILURE);
    }

    if (LWZReadByte(gif_file, TRUE /*, (int) input_code_size*/ ) < 0) {
        Fprintf(stderr, "error reading image\n");
        exit(EXIT_FAILURE);
    }

    /* read first section */
    if (GifScreen.Interlace) {
        ReadInterleavedImage(gif_file, GifScreen.Width, GifScreen.Height);
    } else {
        ReadTileStrip(gif_file, GifScreen.Width);
    }
    return TRUE;
}

/* Read a tile.  Returns FALSE when there are no more tiles */
boolean
read_gif_tile(pixel (*pixels)[TILE_X])
{
    int i, j;

    if (curr_tiles_down >= tiles_down)
        return FALSE;
    if (curr_tiles_across == tiles_across) {
        curr_tiles_across = 0;
        curr_tiles_down++;
        if (curr_tiles_down >= tiles_down)
            return FALSE;
        if (!GifScreen.Interlace)
            ReadTileStrip(gif_file, GifScreen.Width);
    }
    if (GifScreen.Interlace) {
        for (j = 0; j < TILE_Y; j++) {
            for (i = 0; i < TILE_X; i++) {
                pixels[j][i] = image[curr_tiles_down * TILE_Y
                                     + j][curr_tiles_across * TILE_X + i];
            }
        }
    } else {
        for (j = 0; j < TILE_Y; j++) {
            for (i = 0; i < TILE_X; i++) {
                pixels[j][i] = image[j][curr_tiles_across * TILE_X + i];
            }
        }
    }
    curr_tiles_across++;

    /* check for "filler" tile */
    for (j = 0; j < TILE_Y; j++) {
        for (i = 0; i < TILE_X && i < 4; i += 2) {
            if (pixels[j][i].r != ColorMap[CM_RED][0]
                || pixels[j][i].g != ColorMap[CM_GREEN][0]
                || pixels[j][i].b != ColorMap[CM_BLUE][0]
                || pixels[j][i + 1].r != ColorMap[CM_RED][1]
                || pixels[j][i + 1].g != ColorMap[CM_GREEN][1]
                || pixels[j][i + 1].b != ColorMap[CM_BLUE][1])
                return TRUE;
        }
    }
    return FALSE;
}

int
fclose_gif_file(void)
{
    int i;

    if (GifScreen.Interlace) {
        for (i = 0; i < GifScreen.Height; i++) {
            free((genericptr_t) image[i]);
        }
        free((genericptr_t) image);
    } else {
        for (i = 0; i < TILE_Y; i++) {
            free((genericptr_t) image[i]);
        }
        free((genericptr_t) image);
    }
    return (fclose(gif_file));
}

#ifndef AMIGA
static const char *const std_args[] = {
    "tilemap", /* dummy argv[0] */
    "monsters.gif", "monsters.txt",
    "objects.gif",  "objects.txt",
    "other.gif",    "other.txt",
};

int
main(int argc, char *argv[])
{
    pixel pixels[TILE_Y][TILE_X];

    if (argc == 1) {
        argc = SIZE(std_args);
        argv = (char **) std_args;
    } else if (argc != 3) {
        Fprintf(stderr, "usage: gif2txt giffile txtfile\n");
        exit(EXIT_FAILURE);
    }

    while (argc > 1) {
        if (!fopen_gif_file(argv[1], RDBMODE))
            exit(EXIT_FAILURE);

        init_colormap();

        if (!fopen_text_file(argv[2], WRTMODE)) {
            (void) fclose_gif_file();
            exit(EXIT_FAILURE);
        }

        while (read_gif_tile(pixels))
            (void) write_text_tile(pixels);

        (void) fclose_gif_file();
        (void) fclose_text_file();

        argc -= 2;
        argv += 2;
    }
    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}
#endif

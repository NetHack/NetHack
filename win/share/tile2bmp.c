/* NetHack 3.7	tile2bmp.c	$NHDT-Date: 1596498340 2020/08/03 23:45:40 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.32 $ */
/*   Copyright (c) NetHack PC Development Team 1995                 */
/*   NetHack may be freely redistributed.  See license for details. */

/*
 * Edit History:
 *
 *      Initial Creation                        M.Allison   1994/01/11
 *      256 colour bmp and statue support       M.Allison   2015/04/19
 *
 */

#ifndef __GNUC__
#include "win32api.h"
#endif

#include "config.h"
#include "tile.h"
extern void monst_globals_init(void);
extern void objects_globals_init(void);
static int examine_tilefiles(void);
static int set_tilefile_path(const char *, const char *, char *, size_t);

#if defined(_MSC_VER) && defined(_WIN64)
#define UNALIGNED_POINTER __unaligned
#else
#define UNALIGNED_POINTER
#endif

#define COLORS_IN_USE 256
#define BITCOUNT 8

/* GCC fix by Paolo Bonzini 1999/03/28 */
#ifdef __GNUC__
#define PACK __attribute__((packed))
#else
#define PACK
#endif

static short
leshort(short x)
{
#ifdef __BIG_ENDIAN__
    return ((x & 0xff) << 8) | ((x >> 8) & 0xff);
#else
    return x;
#endif
}

static int32_t
lelong(int32_t x)
{
#ifdef __BIG_ENDIAN__
    return ((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00)
           | ((x >> 24) & 0xff);
#else
    return x;
#endif
}

unsigned FITSuint_(unsigned long long, const char *, int);

#ifdef __GNUC__
typedef struct tagBMIH {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} PACK BITMAPINFOHEADER;

typedef struct tagBMFH {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} PACK BITMAPFILEHEADER;

typedef struct tagRGBQ {
    uint8_t rgbBlue;
    uint8_t rgbGreen;
    uint8_t rgbRed;
    uint8_t rgbReserved;
} PACK RGBQUAD;
#define BI_RGB       0L
#define BI_RLE8      1L
#define BI_RLE4      2L
#define BI_BITFIELDS 3L
#endif /* __GNUC__ */
#define RGBQUAD_COUNT 256

#pragma pack(1)
struct tagBMP {
    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;
    RGBQUAD bmaColors[RGBQUAD_COUNT];
    uchar packtile; /* start */
} PACK bmp, *newbmp;
#pragma pack()

FILE *tibfile2;
pixel tilepixels[TILE_Y][TILE_X];
static void build_bmfh(BITMAPFILEHEADER *);
static void build_bmih(UNALIGNED_POINTER BITMAPINFOHEADER *);
static void build_bmptile(pixel(*) [TILE_X]);

#define TILESETS 1
static const char *const relative_tiledir = "../win/share/";
/* monsters must be first because main uses it twice */
const char *const tilefilenames[TILESETS][3] = {
    {"monsters.txt", "objects.txt", "other.txt"},
#if 0
    {"mon32.txt", "obj32.txt", "oth32.txt"},
    {"mon64.txt", "obj64.txt", "oth64.txt"},
#endif
};

int tilecnt[SIZE(tilefilenames[0])];
int tilefileset = 0;
int tiles_counted;
int max_x, max_y;
int magictileno = 0, bmpsize;
int num_colors = 0;
int max_tiles_in_row = 40;
int tiles_in_row;
int filenum;
int initflag;
int pass;
int yoffset, xoffset;
char bmpname[128];
FILE *fp;

DISABLE_WARNING_UNREACHABLE_CODE

int
main(int argc, char *argv[])
{
    int i, j;
    uchar *c;
    char tilefile_full_path[256] = { 0 };

    if (argc != 2) {
        Fprintf(stderr, "usage: %s outfile.bmp\n", argv[0]);
        exit(EXIT_FAILURE);
    } else if (argv[1] == 0 || strlen(argv[1]) >= sizeof bmpname - 1) {
        Fprintf(stderr, "invalid output bmp file name %s, aborting.\n",
                argv[0]);
        exit(EXIT_FAILURE);
    } else {
        strcpy(bmpname, argv[1]);
    }

    objects_globals_init();
    monst_globals_init();

/*    tilefileset = (TILE_X == 64) ? 2 : (TILE_X == 32) ? 1 : 0; */
    tilefileset = 0;
    if (!examine_tilefiles()) {
        Fprintf(stderr, "unable to open all of the tile files, aborting.\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < SIZE(tilecnt); ++i)
        magictileno += tilecnt[i];
    /* count monsters twice for grayscale variation */
    magictileno += tilecnt[0];

    max_x = TILE_X * max_tiles_in_row;
    max_y = ((TILE_Y * magictileno) / max_tiles_in_row) + TILE_Y;
    bmpsize = (sizeof bmp - sizeof bmp.packtile)
                  + (max_y * (max_x * sizeof(uchar)));
    newbmp = malloc(bmpsize);
    if (!newbmp) {
        printf("memory allocation failure, %d %d, aborting.\n",
                bmpsize, magictileno);
        exit(EXIT_FAILURE);
    }
    tiles_counted = 0;
    xoffset = yoffset = 0;
    initflag = 0;
    filenum = 0;
    pass = 0;
    fp = fopen(bmpname, "wb");
    if (!fp) {
        printf("Error creating tile file %s, aborting.\n", bmpname);
        exit(EXIT_FAILURE);
    }
    while (pass < 4) {
        filenum = pass % SIZE(tilefilenames[tilefileset]);
        if (!set_tilefile_path(relative_tiledir,
                               tilefilenames[tilefileset][filenum],
                               tilefile_full_path, sizeof tilefile_full_path)) {
            Fprintf(stderr, "tile2bmp path issue %s %s %d\n",
                    relative_tiledir, tilefilenames[tilefileset][filenum],
                    (int) sizeof tilefile_full_path);
            exit(EXIT_FAILURE);
        }
        if (!fopen_text_file(tilefile_full_path, RDTMODE)) {
            Fprintf(stderr, "usage: tile2bmp (from the util directory)\n");
            exit(EXIT_FAILURE);
        }
        num_colors = colorsinmap;
        if (num_colors > 62) {
            Fprintf(stderr, "too many colors (%d), aborting.\n", num_colors);
            exit(EXIT_FAILURE);
        }
        if (!initflag) {
            UNALIGNED_POINTER BITMAPINFOHEADER *bmih;

            build_bmfh(&bmp.bmfh);
            bmih = &bmp.bmih;
            build_bmih(bmih);
            for (i = 0; i < num_colors; i++) {
                bmp.bmaColors[i].rgbRed = ColorMap[CM_RED][i];
                bmp.bmaColors[i].rgbGreen = ColorMap[CM_GREEN][i];
                bmp.bmaColors[i].rgbBlue = ColorMap[CM_BLUE][i];
                bmp.bmaColors[i].rgbReserved = 0;
            }
            *newbmp = bmp;
            for (i = 0; i < max_y; ++i)
                for (j = 0; j < max_x; ++j) {
                    c = &newbmp->packtile + ((i * max_x) + j);
                    *c = (uchar) 0;
                }
            initflag = 1;
        }
        set_grayscale(pass == 3);
        /* printf("Colormap initialized\n"); */
        while (read_text_tile(tilepixels)) {
            if (tiles_counted >= magictileno) {
                Fprintf(stderr, "tile2bmp: more than %d tiles!\n",
                        magictileno);
                exit(EXIT_FAILURE);
            }
            build_bmptile(tilepixels);
            tiles_counted++;
            xoffset += TILE_X;
            if (xoffset >= max_x) {
                yoffset += TILE_Y;
                xoffset = 0;
            }
        }
        (void) fclose_text_file();
        ++pass;
    }
    fwrite(newbmp, bmpsize, 1, fp);
    fclose(fp);
    Fprintf(stderr, "Total of %d tiles written to %s.\n",
            tiles_counted, bmpname);
    free((genericptr_t) newbmp);

    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

RESTORE_WARNINGS

static void
build_bmfh(BITMAPFILEHEADER* pbmfh)
{
    pbmfh->bfType = leshort(0x4D42);
    pbmfh->bfSize = lelong(bmpsize);
    pbmfh->bfReserved1 = (uint32_t) 0;
    pbmfh->bfReserved2 = (uint32_t) 0;
    pbmfh->bfOffBits = lelong(sizeof bmp.bmfh + sizeof bmp.bmih
                              + (RGBQUAD_COUNT * sizeof(RGBQUAD)));
}

static void
build_bmih(UNALIGNED_POINTER BITMAPINFOHEADER* pbmih)
{
    uint16_t cClrBits;
    int w, h;
    pbmih->biSize = lelong(sizeof(bmp.bmih));
    pbmih->biWidth = lelong(w = max_x);
    pbmih->biHeight = lelong(h = max_y);
    pbmih->biPlanes = leshort(1);
    pbmih->biBitCount = leshort(8);
    cClrBits = 8;
    if (cClrBits == 1)
        cClrBits = 1;
    else if (cClrBits <= 4)
        cClrBits = 4;
    else if (cClrBits <= 8)
        cClrBits = 8;
    else if (cClrBits <= 16)
        cClrBits = 16;
    else if (cClrBits <= 24)
        cClrBits = 24;
    else
        cClrBits = 32;
    pbmih->biCompression = lelong(BI_RGB);
    pbmih->biXPelsPerMeter = lelong(0);
    pbmih->biYPelsPerMeter = lelong(0);
#if (TILE_X == 32)
    if (cClrBits < 24)
        pbmih->biClrUsed = lelong(1 << cClrBits);
#else
    pbmih->biClrUsed = lelong(RGBQUAD_COUNT);
#endif
    pbmih->biSizeImage = lelong(((w * cClrBits + 31) & ~31) / 8 * h);
    pbmih->biClrImportant = (uint32_t) 0;
}

static void
build_bmptile(pixel(*pixels)[TILE_X])
{
    int cur_x, cur_y, cur_color, apply_color;
    int x, y;
    uchar *c;

    for (cur_y = 0; cur_y < TILE_Y; cur_y++) {
        for (cur_x = 0; cur_x < TILE_X; cur_x++) {
            for (cur_color = 0; cur_color < num_colors; cur_color++) {
                if (ColorMap[CM_RED][cur_color] == pixels[cur_y][cur_x].r
                    && ColorMap[CM_GREEN][cur_color] == pixels[cur_y][cur_x].g
                    && ColorMap[CM_BLUE][cur_color] == pixels[cur_y][cur_x].b)
                    break;
            }
            if (cur_color >= num_colors)
                Fprintf(stderr, "color not in colormap! (tile #%d)\n",
                        tiles_counted);
            y = (max_y - 1) - (cur_y + yoffset);
            apply_color = cur_color;
            x = cur_x + xoffset;
            c = &newbmp->packtile + ((y * max_x) + x);
            *c = (uchar) apply_color;
        }
    }
}

static int
set_tilefile_path(const char *fdir, const char *fname, char *buf, size_t sz)
{
    size_t consuming = 0;

    *buf = '\0';
    consuming = strlen(fdir) + strlen(fname) + 1;
    if (consuming > sz)
        return 0;
    Strcpy(buf, fdir);
    Strcat(buf, fname);
    return 1;
}

static int
examine_tilefiles(void)
{
    FILE *fp2;
    int i, tiles_in_file;
    char tilefile_full_path[256];

    for (i = 0; i < SIZE(tilefilenames[0]); ++i) {
        tiles_in_file = 0;
        if (!set_tilefile_path(relative_tiledir,
                               tilefilenames[tilefileset][i],
                               tilefile_full_path,
                               sizeof tilefile_full_path)) {
            Fprintf(stderr, "tile2bmp path issue %s/%s %d\n",
                    relative_tiledir, tilefilenames[tilefileset][i],
                    (int) sizeof tilefile_full_path);
            return 0;
        }
        fp2 = fopen(tilefile_full_path, "r");
        if (fp2) {
            char line[256];

            while (fgets(line, sizeof line, fp2)) {
                if (!strncmp(line, "# tile ", 7))
                    tiles_in_file++;
            }
            (void) fclose(fp2);
            tilecnt[i] = tiles_in_file;
        }
    }
    return 1;
}

/*tile2bmp.c*/

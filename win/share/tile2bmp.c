/* NetHack 3.7	tile2bmp.c	$NHDT-Date: 1596498340 2020/08/03 23:45:40 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.32 $ */
/*   Copyright (c) NetHack PC Development Team 1995                 */
/*   NetHack may be freely redistributed.  See license for details. */

/*
 * Edit History:
 *
 *	Initial Creation			M.Allison   1994/01/11
 *	256 colour bmp and statue support	M.Allison   2015/04/19
 *
 */

/* #pragma warning(4103:disable) */

#ifndef __GNUC__
#include "win32api.h"
#endif

#include "config.h"
#include "tile.h"
extern void monst_globals_init(void);
extern void objects_globals_init(void);

#include <stdint.h>
#if defined(UINT32_MAX) && defined(INT32_MAX) && defined(UINT16_MAX)
#define UINT8 uint8_t
#define UINT16 uint16_t
#define UINT32 uint32_t
#define INT32 int32_t
#else
# ifdef _MSC_VER
#define UINT8 unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned long
#define INT32 long
# endif
#endif

#if defined(_MSC_VER) && defined(_WIN64)
#define UNALIGNED_POINTER __unaligned
#else
#define UNALIGNED_POINTER
#endif

#if (TILE_X == 32)
#define COLORS_IN_USE 256
#else
/*#define COLORS_IN_USE 16 */ /* 16 colors */
#define COLORS_IN_USE 256 /* 256 colors */
#endif

#define BITCOUNT 8

extern char *tilename(int, int);

/* The numbers in the following calculation are the
   count of tiles present in:
   monsters.txt objects.txt other.txt monsters.txt */
#define MAGICTILENO (788 + 459 + 237 + 788)

#if BITCOUNT == 4
#define MAX_X 320 /* 2 per byte, 4 bits per pixel */
#define MAX_Y 480
#else
#if (TILE_X == 32)
#define MAX_X (32 * 40)
#define MAX_Y ((MAGICTILENO * 32) / 40) * 2
#else
#define MAX_X (16 * 40)
#define MAX_Y (((16 * MAGICTILENO) / 40) + 16)
#endif
#endif

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

static INT32
lelong(INT32 x)
{
#ifdef __BIG_ENDIAN__
    return ((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00)
           | ((x >> 24) & 0xff);
#else
    return x;
#endif
}

#ifdef __GNUC__
typedef struct tagBMIH {
    UINT32 biSize;
    INT32 biWidth;
    INT32 biHeight;
    UINT16 biPlanes;
    UINT16 biBitCount;
    UINT32 biCompression;
    UINT32 biSizeImage;
    INT32 biXPelsPerMeter;
    INT32 biYPelsPerMeter;
    UINT32 biClrUsed;
    UINT32 biClrImportant;
} PACK BITMAPINFOHEADER;

typedef struct tagBMFH {
    UINT16 bfType;
    UINT32 bfSize;
    UINT16 bfReserved1;
    UINT16 bfReserved2;
    UINT32 bfOffBits;
} PACK BITMAPFILEHEADER;

typedef struct tagRGBQ {
    UINT8 rgbBlue;
    UINT8 rgbGreen;
    UINT8 rgbRed;
    UINT8 rgbReserved;
} PACK RGBQUAD;
#define DWORD UINT32
#define WORD UINT16
#define BI_RGB 0L
#define BI_RLE8 1L
#define BI_RLE4 2L
#define BI_BITFIELDS 3L
#endif /* __GNUC__ */

#pragma pack(1)
struct tagBMP {
    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;
#if BITCOUNT == 4
#define RGBQUAD_COUNT 16
    RGBQUAD bmaColors[RGBQUAD_COUNT];
#else
#if (TILE_X == 32)
#define RGBQUAD_COUNT 256
#else
/*#define RGBQUAD_COUNT 16 */
#define RGBQUAD_COUNT 256
#endif
    RGBQUAD bmaColors[RGBQUAD_COUNT];
#endif
#if (COLORS_IN_USE == 16)
    uchar packtile[MAX_Y][MAX_X];
#else
    uchar packtile[MAX_Y][MAX_X];
/*    uchar            packtile[TILE_Y][TILE_X]; */
#endif
} PACK bmp;
#pragma pack()

#define BMPFILESIZE (sizeof(struct tagBMP))

FILE *tibfile2;

pixel tilepixels[TILE_Y][TILE_X];

static void build_bmfh(BITMAPFILEHEADER *);
static void build_bmih(UNALIGNED_POINTER BITMAPINFOHEADER *);
static void build_bmptile(pixel(*) [TILE_X]);

const char *tilefiles[] = {
#if (TILE_X == 32)
    "../win/share/mon32.txt", "../win/share/obj32.txt",
    "../win/share/oth32.txt",
#else
    "../win/share/monsters.txt", "../win/share/objects.txt",
    "../win/share/other.txt",
#endif
};

int num_colors = 0;
int tilecount;
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

    if (argc != 2) {
        Fprintf(stderr, "usage: %s outfile.bmp\n", argv[0]);
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

    objects_globals_init();
    monst_globals_init();

    tilecount = 0;
    xoffset = yoffset = 0;
    initflag = 0;
    filenum = 0;
    pass = 0;
    fp = fopen(bmpname, "wb");
    if (!fp) {
        printf("Error creating tile file %s, aborting.\n", bmpname);
        exit(1);
    }
    while (pass < 4) {
        filenum = pass % (sizeof(tilefiles) / sizeof(char *));
        if (!fopen_text_file(tilefiles[filenum], RDTMODE)) {
            Fprintf(stderr, "usage: tile2bmp (from the util directory)\n");
            exit(EXIT_FAILURE);
        }
        num_colors = colorsinmap;
        if (num_colors > 62) {
            Fprintf(stderr, "too many colors (%d)\n", num_colors);
            exit(EXIT_FAILURE);
        }
        if (!initflag) {
            UNALIGNED_POINTER BITMAPINFOHEADER *bmih;

            build_bmfh(&bmp.bmfh);
            bmih = &bmp.bmih;
            build_bmih(bmih);
            for (i = 0; i < MAX_Y; ++i)
                for (j = 0; j < MAX_X; ++j)
                    bmp.packtile[i][j] = (uchar) 0;
            for (i = 0; i < num_colors; i++) {
                bmp.bmaColors[i].rgbRed = ColorMap[CM_RED][i];
                bmp.bmaColors[i].rgbGreen = ColorMap[CM_GREEN][i];
                bmp.bmaColors[i].rgbBlue = ColorMap[CM_BLUE][i];
                bmp.bmaColors[i].rgbReserved = 0;
            }
            initflag = 1;
        }
        set_grayscale(pass == 3);
        /*		printf("Colormap initialized\n"); */
        while (read_text_tile(tilepixels)) {
            build_bmptile(tilepixels);
            tilecount++;
#if BITCOUNT == 4
            xoffset += (TILE_X / 2);
#else
            xoffset += TILE_X;
#endif
            if (xoffset >= MAX_X) {
                yoffset += TILE_Y;
                xoffset = 0;
            }
        }
        (void) fclose_text_file();
        ++pass;
    }
    fwrite(&bmp, sizeof(bmp), 1, fp);
    fclose(fp);
    Fprintf(stderr, "Total of %d tiles written to %s.\n", tilecount, bmpname);

    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

RESTORE_WARNINGS

static void
build_bmfh(BITMAPFILEHEADER* pbmfh)
{
    pbmfh->bfType = leshort(0x4D42);
    pbmfh->bfSize = lelong(BMPFILESIZE);
    pbmfh->bfReserved1 = (UINT32) 0;
    pbmfh->bfReserved2 = (UINT32) 0;
    pbmfh->bfOffBits = lelong(sizeof(bmp.bmfh) + sizeof(bmp.bmih)
                              + (RGBQUAD_COUNT * sizeof(RGBQUAD)));
}

static void
build_bmih(UNALIGNED_POINTER BITMAPINFOHEADER* pbmih)
{
    WORD cClrBits;
    int w, h;
    pbmih->biSize = lelong(sizeof(bmp.bmih));
#if BITCOUNT == 4
    pbmih->biWidth = lelong(w = MAX_X * 2);
#else
    pbmih->biWidth = lelong(w = MAX_X);
#endif
    pbmih->biHeight = lelong(h = MAX_Y);
    pbmih->biPlanes = leshort(1);
#if BITCOUNT == 4
    pbmih->biBitCount = leshort(4);
    cClrBits = 4;
#else
    pbmih->biBitCount = leshort(8);
    cClrBits = 8;
#endif
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

#if (TILE_X == 16)
    /* pbmih->biSizeImage = lelong(0); */
    pbmih->biSizeImage = lelong(((w * cClrBits + 31) & ~31) / 8 * h);
#else
    pbmih->biSizeImage = lelong(((w * cClrBits + 31) & ~31) / 8 * h);
#endif
    pbmih->biClrImportant = (DWORD) 0;
}

static void
build_bmptile(pixel(*pixels)[TILE_X])
{
    int cur_x, cur_y, cur_color, apply_color;
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
            y = (MAX_Y - 1) - (cur_y + yoffset);
            apply_color = cur_color;
#if BITCOUNT == 4
            x = (cur_x / 2) + xoffset;
            bmp.packtile[y][x] = cur_x % 2
                                     ? (uchar)(bmp.packtile[y][x] | cur_color)
                                     : (uchar)(cur_color << 4);
#else
            x = cur_x + xoffset;
            bmp.packtile[y][x] = (uchar) apply_color;
#endif
        }
    }
}

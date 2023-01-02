/* NetHack 3.7	pctiles.h	$NHDT-Date: 1596498272 2020/08/03 23:44:32 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.10 $ */
/*   Copyright (c) NetHack PC Development Team 1993, 1994             */
/*   NetHack may be freely redistributed.  See license for details.   */
/*                                                                    */
/*
 * pctiles.h - Definitions for PC graphical tile support
 *
 *Edit History:
 *     Initial Creation              M. Allison      93/10/30
 *
 */

#ifdef TILES_IN_GLYPHMAP
#ifndef TILE_X
#define TILE_X 16
#endif

#define NETHACK_PLANAR_TILEFILE "NetHack1.tib"   /* Planar style tiles */
#define NETHACK_PACKED_TILEFILE "NetHack2.tib"   /* Packed style tiles */
#define NETHACK_OVERVIEW_TILEFILE "NetHacko.tib" /* thin overview tiles */

#define ROWS_PER_TILE TILE_Y
#define COLS_PER_TILE TILE_X
#define EMPTY_TILE -1
#define TIBHEADER_SIZE 1024 /* Use this for size, allows expansion */
#define PLANAR_STYLE 0
#define PACKED_STYLE 1
#define DJGPP_COMP 0
#define MSC_COMP 1
#define BC_COMP 2
#define OTHER_COMP 10

struct tibhdr_struct {
    char ident[80];        /* Identifying string           */
    char timestamp[26];    /* Ascii timestamp              */
    char tilestyle;        /* 0 = planar, 1 = pixel        */
    char compiler;         /* 0 = DJGPP, 1 = MSC, 2= BC etc. see above */
    short tilecount;       /* number of tiles in file      */
    short numcolors;       /* number of colors in palette  */
    char palette[256 * 3]; /* palette                      */
};

/* Note on packed style tile file:
 * Each record consists of one of the following arrays:
 *	char packtile[TILE_Y][TILE_X];
 */

extern void CloseTileFile(boolean);
extern int OpenTileFile(char *, boolean);
extern int ReadTileFileHeader(struct tibhdr_struct *, boolean);

#ifdef PLANAR_FILE
#ifdef SCREEN_VGA
extern int ReadPlanarTileFile(int, struct planar_cell_struct **);
extern int ReadPlanarTileFile_O(int, struct overview_planar_cell_struct **);
#endif
#endif

#ifdef PACKED_FILE
extern int ReadPackedTileFile(int, char (*)[TILE_X]);
#endif
#endif /* TILES_IN_GLYPHMAP */

/* pctiles.h */

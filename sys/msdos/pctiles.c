/* NetHack 3.7	pctiles.c	$NHDT-Date: 1596498271 2020/08/03 23:44:31 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.13 $ */
/*   Copyright (c) NetHack PC Development Team 1993, 1994           */
/*   NetHack may be freely redistributed.  See license for details. */
/*                                                                  */
/*
 * pctiles.c - PC Graphical Tile Support Routines
 *
 *Edit History:
 *     Initial Creation              M. Allison      93/10/30
 *
 */

#include "hack.h"

#ifdef TILES_IN_GLYPHMAP

#if defined(__GO32__) || defined(__DJGPP__)
#include <unistd.h>
#define TILES_IN_RAM /* allow tiles to be read into ram */
#endif

#if defined(_MSC_VER)
#if _MSC_VER >= 700
#pragma warning(disable : 4018) /* signed/unsigned mismatch */
#pragma warning(disable : 4127) /* conditional expression is constant */
#pragma warning(disable : 4131) /* old style declarator */
#pragma warning(disable : 4309) /* initializing */
#endif
#include <conio.h>
#endif

#include "pcvideo.h"
#include "tile.h"
#include "pctiles.h"

static FILE *tilefile;
static FILE *tilefile_O;
extern short glyph2tile[]; /* in tile.c (made from tilemap.c) */

#ifdef TILES_IN_RAM
struct planar_cell_struct *ramtiles;
struct overview_planar_cell_struct *oramtiles;
boolean tiles_in_ram = FALSE;
boolean otiles_in_ram = FALSE;
extern int total_tiles_used; /* tile.c */
#endif

/*
 * Read the header/palette information at the start of the
 * NetHack.tib file.
 *
 * There is 1024 bytes (1K) of header information
 * at the start of the file, including a palette.
 *
 */
int
ReadTileFileHeader(struct tibhdr_struct *tibhdr, boolean filestyle)
{
    FILE *x;
    x = filestyle ? tilefile_O : tilefile;
    if (fseek(x, 0L, SEEK_SET)) {
        return 1;
    } else {
        fread(tibhdr, sizeof(struct tibhdr_struct), 1, x);
    }
    return 0;
}

/*
 * Open the requested tile file.
 *
 * NetHack1.tib file is a series of
 * 'struct planar_tile_struct' structures, one for each
 * glyph tile.
 *
 * NetHack2.tib file is a series of
 * char arrays [TILE_Y][TILE_X] in dimensions, one for each
 * glyph tile.
 *
 * There is 1024 bytes (1K) of header information
 * at the start of each .tib file. The first glyph tile starts at
 * location 1024.
 *
 */
int
OpenTileFile(char *tilefilename, boolean filestyle)
{
#ifdef TILES_IN_RAM
    int k;
#endif
    if (filestyle) {
        tilefile_O = fopen(tilefilename, "rb");
        if (tilefile_O == (FILE *) 0)
            return 1;
    } else {
        tilefile = fopen(tilefilename, "rb");
        if (tilefile == (FILE *) 0)
            return 1;
    }
#ifdef TILES_IN_RAM
    if (iflags.preload_tiles) {
        if (filestyle) {
            struct overview_planar_cell_struct *gp;
            long ram_needed =
                sizeof(struct overview_planar_cell_struct) * total_tiles_used;
            if (fseek(tilefile_O, (long) TIBHEADER_SIZE,
                      SEEK_SET)) { /*failure*/
            }
            oramtiles =
                (struct overview_planar_cell_struct *) alloc(ram_needed);
            /* Todo: fall back to file method here if alloc failed */
            gp = oramtiles;
            for (k = 0; k < total_tiles_used; ++k) {
                fread(gp, sizeof(struct overview_planar_cell_struct), 1,
                      tilefile_O);
                ++gp;
            }
#ifdef DEBUG_RAMTILES
            pline("%d overview tiles read into ram.", k);
            mark_synch();
#endif
            otiles_in_ram = TRUE;
        } else {
            struct planar_cell_struct *gp;
            long ram_needed =
                sizeof(struct planar_cell_struct) * total_tiles_used;
            if (fseek(tilefile, (long) TIBHEADER_SIZE,
                      SEEK_SET)) { /*failure*/
            }
            ramtiles = (struct planar_cell_struct *) alloc(ram_needed);
            /* Todo: fall back to file method here if alloc failed */
            gp = ramtiles;
            for (k = 0; k < total_tiles_used; ++k) {
                fread(gp, sizeof(struct planar_cell_struct), 1, tilefile);
                ++gp;
            }
#ifdef DEBUG_RAMTILES
            pline("%d tiles read into ram.", k);
            mark_synch();
#endif
            tiles_in_ram = TRUE;
        }
    }
#endif
    return 0;
}

void
CloseTileFile(boolean filestyle)
{
    fclose(filestyle ? tilefile_O : tilefile);
#ifdef TILES_IN_RAM
    if (!filestyle && tiles_in_ram) {
        if (ramtiles)
            free((genericptr_t) ramtiles);
        tiles_in_ram = FALSE;
    } else if (filestyle && otiles_in_ram) {
        if (oramtiles)
            free((genericptr_t) oramtiles);
        otiles_in_ram = FALSE;
    }
#endif
}

struct planar_cell_struct plancell;
struct overview_planar_cell_struct oplancell;

/* This routine retrieves the requested NetHack glyph tile
 * from the planar style binary .tib file.
 * This is currently done 'on demand', so if the player
 * is running without a disk cache (ie. smartdrv) operating,
 * things can really be slowed down.  We don't have any
 * base memory under MSDOS, in which to store the pictures.
 *
 * Todo: Investigate the possibility of loading the glyph
 *       tiles into extended or expanded memory using
 *       the MSC virtual memory routines.
 *
 * Under an environment like djgpp, it should be possible to
 * read the entire set of glyph tiles into a large
 * array of 'struct planar_cell_struct' structures at
 * game initialization time, and recall them from the array
 * as needed.  That should speed things up (at the cost of
 * increasing the memory requirement - can't have everything).
 *
 */
#ifdef PLANAR_FILE
int
ReadPlanarTileFile(int tilenum, struct planar_cell_struct **gp)
{
    long fpos;

#ifdef TILES_IN_RAM
    if (tiles_in_ram) {
        *gp = ramtiles + tilenum;
        return 0;
    }
#endif
    fpos = ((long) (tilenum) * (long) sizeof(struct planar_cell_struct))
           + (long) TIBHEADER_SIZE;
    if (fseek(tilefile, fpos, SEEK_SET)) {
        return 1;
    } else {
        fread(&plancell, sizeof(struct planar_cell_struct), 1, tilefile);
    }
    *gp = &plancell;
    return 0;
}
int
ReadPlanarTileFile_O(int tilenum, struct overview_planar_cell_struct **gp)
{
    long fpos;

#ifdef TILES_IN_RAM
    if (otiles_in_ram) {
        *gp = oramtiles + tilenum;
        return 0;
    }
#endif
    fpos =
        ((long) (tilenum) * (long) sizeof(struct overview_planar_cell_struct))
        + (long) TIBHEADER_SIZE;
    if (fseek(tilefile_O, fpos, SEEK_SET)) {
        return 1;
    } else {
        fread(&oplancell, sizeof(struct overview_planar_cell_struct), 1,
              tilefile_O);
    }
    *gp = &oplancell;
    return 0;
}
#endif

#ifdef PACKED_FILE
int
ReadPackedTileFile(int tilenum, char (*pta)[TILE_X])
{
    long fpos;

    fpos =
        ((long) (tilenum) * (long) (TILE_Y * TILE_X) + (long) TIBHEADER_SIZE);
    if (fseek(tilefile, fpos, SEEK_SET)) {
        return 1;
    } else {
        fread(pta, (TILE_Y * TILE_X), 1, tilefile);
    }
    return 0;
}
#endif
#endif /* TILES_IN_GLYPHMAP */

/* pctiles.c */

/* NetHack 3.6	winchar.c	$NHDT-Date: 1432512795 2015/05/25 00:13:15 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/*    Copyright (c) Olaf Seibert (KosmoSoft), 1989, 1992	  */
/*    Copyright (c) Kenneth Lorber, Bethesda, Maryland 1993	  */
/*    Copyright (c) Gregg Wonderly, Naperville Illinois, 1994.		*/
/*    NetHack may be freely redistributed.  See license for details.	*/

#include <exec/types.h>
#include <libraries/iffparse.h>
#include <graphics/scale.h>
#include <proto/iffparse.h>

#include "../src/tile.c"

#include "tile.h"
#include "windefs.h"
#include "winext.h"
#include "winproto.h"

/* NH:sys/amiga/winvchar.c */
int main(int, char **);
struct BitMap *MyAllocBitMap(int, int, int, long);
void MyFreeBitMap(struct BitMap *);
BitMapHeader ReadImageFile(const char *, struct BitMap **);
void FreeImageFile(struct BitMap **);
void amiv_flush_glyph_buffer(struct Window *);
void amiv_lprint_glyph(winid, int, int);
void amii_lprint_glyph(winid, int, int);
void amiv_start_glyphout(winid);
void amii_end_glyphout(winid);
void SetMazeType(MazeType);
int GlyphToIcon(int);
void amii_start_glyphout(winid);
void amii_end_glyphout(winid);
void amii_flush_glyph_buffer(struct Window *);

int amii_extraplanes = 0;
extern int reclip;

struct BitMap *MyAllocBitMap(int xsize, int ysize, int depth, long mflags);
void MyFreeBitMap(struct BitMap *bmp);

#ifdef TILES_IN_GLYPHMAP
extern int maxmontile, maxobjtile, maxothtile; /* from tile.c */
#define MAXMONTILE maxmontile
#define MAXOBJTILE maxobjtile
#define MAXOTHTILE maxothtile
#endif

/*
 *  These values will be available from tile.c source
 *
 * #define MAXMONTILE 335
 * #define MAXOBJTILE 722
 * #define MAXOTHTILE 841
 */

#define IMGROWS 12
#define IMGCOLUMNS 20
#define IMGPAGESIZE (IMGROWS * IMGCOLUMNS)

#define ID_BMAP MAKE_ID('B', 'M', 'A', 'P') /* The type of form we use */
#define ID_BMHD MAKE_ID('B', 'M', 'H', 'D') /* The ILBM bitmap header */
#define ID_CAMG MAKE_ID('C', 'A', 'M', 'G') /* The ILBM camg (ignored) */
#define ID_CMAP MAKE_ID('C', 'M', 'A', 'P') /* Standard ILBM color map */
#define ID_PLNE MAKE_ID('P', 'L', 'N', 'E') /* The plane data */
#define ID_PDAT MAKE_ID('P', 'D', 'A', 'T') /* The PDAT structure below */

struct PDAT pictdata;

/* Single tile image file, set at runtime by amii_init_nhwindows */
char *tilefile;
struct BitMap *tileimg, *tile;

/*
 * Read a single BMAP IFF file into a BitMap.
 * Returns the BitMapHeader; *bmp receives the bitmap.
 * Caller frees via FreeImageFile().
 */
BitMapHeader
ReadImageFile(const char *filename, struct BitMap **bmp)
{
    BitMapHeader *bmhd, bmhds = { 0 };
    int j, np;
    long err;
    struct IFFHandle *iff = NULL;
    struct StoredProperty *prop;
    int iff_opened = 0;
    const char *errfmt = NULL;
    long errcode = 0;

    IFFParseBase = OpenLibrary("iffparse.library", 0L);
    if (!IFFParseBase)
        panic("No iffparse.library");

    iff = AllocIFF();
    if (!iff) {
        errfmt = "can't start IFF processing";
        goto cleanup;
    }

    iff->iff_Stream = Open(filename, MODE_OLDFILE);
    if (iff->iff_Stream == 0) {
        errfmt = "Can't open %s";
        goto cleanup;
    }

    InitIFFasDOS(iff);
    if ((err = OpenIFF(iff, IFFF_READ)) != 0) {
        errfmt = "OpenIFF failed on %s, code %ld";
        errcode = err;
        goto cleanup;
    }
    iff_opened = 1;

    PropChunk(iff, ID_BMAP, ID_BMHD);
    PropChunk(iff, ID_BMAP, ID_CMAP);
    PropChunk(iff, ID_BMAP, ID_PDAT);
    StopChunk(iff, ID_BMAP, ID_PLNE);
    if ((err = ParseIFF(iff, IFFPARSE_SCAN)) != 0) {
        errfmt = "ParseIFF failed on %s, code %ld";
        errcode = err;
        goto cleanup;
    }

    prop = FindProp(iff, ID_BMAP, ID_BMHD);
    if (!prop) {
        errfmt = "No BMHD chunk in %s";
        goto cleanup;
    }
    bmhd = (BitMapHeader *) prop->sp_Data;
    np = bmhd->nPlanes;

    if (np > DEPTH) {
        errfmt = "%s: too many bitplanes (code %ld)";
        errcode = np;
        goto cleanup;
    }

    /* Load CMAP into palette arrays if present */
    prop = FindProp(iff, ID_BMAP, ID_CMAP);
    if (prop) {
        unsigned char *cmap = prop->sp_Data;
        for (j = 0; j < (1UL << np) * 3; j += 3) {
            amii_initmap[j / 3] =
                amiv_init_map[j / 3] =
                    ((cmap[j+0] >> 4) << 8)
                  | ((cmap[j+1] >> 4) << 4)
                  |  (cmap[j+2] >> 4);
        }
    }

    /* Load PDAT if present */
    prop = FindProp(iff, ID_BMAP, ID_PDAT);
    if (prop)
        pictdata = *(struct PDAT *) prop->sp_Data;

    *bmp = MyAllocBitMap(bmhd->w, bmhd->h,
                np, MEMF_CHIP | MEMF_CLEAR);
    if (!*bmp) {
        errfmt = "Can't allocate bitmap for %s";
        goto cleanup;
    }

    for (j = 0; j < np; j++)
        ReadChunkBytes(iff, (*bmp)->Planes[j],
                       RASSIZE(bmhd->w, bmhd->h));

    bmhds = *bmhd;

cleanup:
    if (iff_opened)
        CloseIFF(iff);
    if (iff && iff->iff_Stream)
        Close(iff->iff_Stream);
    if (iff)
        FreeIFF(iff);
    CloseLibrary(IFFParseBase);
    IFFParseBase = NULL;

    if (errfmt)
        panic(errfmt, filename, errcode);

    return bmhds;
}

void
FreeImageFile(struct BitMap **bmp)
{
    if (*bmp) {
        MyFreeBitMap(*bmp);
        *bmp = NULL;
    }
}

BitMapHeader
ReadTileImageFiles(void)
{
    BitMapHeader bmhds;

    bmhds = ReadImageFile(tilefile, &tileimg);

    tile = MyAllocBitMap(pictdata.xsize, pictdata.ysize,
                pictdata.nplanes + amii_extraplanes,
                MEMF_CHIP | MEMF_CLEAR);
    if (!tile)
        panic("Can't allocate tile temp bitmap");

    return bmhds;
}

struct MyBitMap {
    struct BitMap bm;
    long mflags;
    USHORT xsize, ysize;
};

struct BitMap *
MyAllocBitMap(int xsize, int ysize, int depth, long mflags)
{
    int j;
    struct MyBitMap *bm;

    bm = (struct MyBitMap *) alloc(sizeof(*bm));
    if (!bm)
        return (NULL);

    bm->mflags = mflags;
    bm->xsize = xsize;
    bm->ysize = ysize;
    InitBitMap(&bm->bm, depth, xsize, ysize);
    /* InitBitMap does not zero Planes[]; if a later AllocRaster fails
     * and MyFreeBitMap unwinds, the uninitialized entries above the
     * failure would be passed to FreeRaster as garbage pointers. */
    for (j = 0; j < (int) (sizeof bm->bm.Planes / sizeof bm->bm.Planes[0]); ++j)
        bm->bm.Planes[j] = NULL;
    for (j = 0; j < depth; ++j) {
        if (mflags & MEMF_CHIP)
            bm->bm.Planes[j] = AllocRaster(xsize, ysize);
        else
            bm->bm.Planes[j] = AllocMem(RASSIZE(xsize, ysize), mflags);

        if (bm->bm.Planes[j] == 0) {
            MyFreeBitMap(&bm->bm);
            return (NULL);
        }
        if (mflags & MEMF_CLEAR)
            memset(bm->bm.Planes[j], 0, RASSIZE(xsize, ysize));
    }
    return (&bm->bm);
}

void
MyFreeBitMap(struct BitMap *bmp)
{
    int j;
    struct MyBitMap *bm = (struct MyBitMap *) bmp;

    for (j = 0; j < bm->bm.Depth; ++j) {
        if (bm->bm.Planes[j]) {
            if (bm->mflags & MEMF_CHIP)
                FreeRaster(bm->bm.Planes[j], bm->xsize, bm->ysize);
            else
                FreeMem(bm->bm.Planes[j], RASSIZE(bm->xsize, bm->ysize));
        }
    }
    free(bm);
}

void
FreeTileImageFiles(void)
{
    FreeImageFile(&tileimg);
    FreeImageFile(&tile);
}

/*
 * Define some stuff for our special glyph drawing routines
 */
unsigned short glyph_node_index, glyph_buffer_index;
#define NUMBER_GLYPH_NODES 80
#define GLYPH_BUFFER_SIZE 512
struct amiv_glyph_node {
    short odstx, odsty;
    short srcx, srcy, dstx, dsty;
    struct BitMap *bitmap;
};
struct amiv_glyph_node amiv_g_nodes[NUMBER_GLYPH_NODES];
static char amiv_glyph_buffer[GLYPH_BUFFER_SIZE];

void
flush_glyph_buffer(struct Window *vw)
{
    if (WINVERS_AMIV)
        amiv_flush_glyph_buffer(vw);
    else
        amii_flush_glyph_buffer(vw);
}

/*
 * Routine to flush whatever is buffered
 */
void
amiv_flush_glyph_buffer(struct Window *vw)
{
    int xsize, ysize, x, y;
    struct BitScaleArgs bsa;
    struct BitScaleArgs bsm;
    struct RastPort rast;
    struct Window *w = NULL;
    struct BitMap *imgbm = 0, *bm = 0;
    int i, k;
    int scaling_needed;
    struct RastPort *rp = vw->RPort;

    /* If nothing is buffered, return before we do anything */
    if (glyph_node_index == 0)
        return;

    cursor_off(WIN_MAP);
    amiv_start_glyphout(WIN_MAP);

    /* This is a dynamic value based on this relationship. */
    scaling_needed = (pictdata.xsize != mxsize || pictdata.ysize != mysize);

        /* If overview window is up, set up to render the correct scale there
         */
        if (WIN_OVER != WIN_ERR && (w = amii_wins[WIN_OVER]->win) != NULL) {
            InitRastPort(&rast);

            /* Calculate the x and y size of each tile for a ROWNO by COLNO
             * map */
            xsize = (w->Width - w->BorderLeft - w->BorderRight) / COLNO;
            ysize = (w->Height - w->BorderTop - w->BorderBottom) / ROWNO;

            /* Get a chip memory bitmap to blit out of */
            bm = MyAllocBitMap(pictdata.xsize, pictdata.ysize,
                               pictdata.nplanes + amii_extraplanes,
                               MEMF_CLEAR | MEMF_CHIP);
            if (bm == NULL) {
                amii_putstr(
                    WIN_MESSAGE, 0,
                    "Can't allocate bitmap for scaling overview window");
            }

            rast.BitMap = bm;

            memset(&bsa, 0, sizeof(bsa));
            bsa.bsa_SrcX = bsa.bsa_SrcY = 0;
            bsa.bsa_SrcBitMap = tile;
            bsa.bsa_SrcWidth = pictdata.xsize;
            bsa.bsa_SrcHeight = pictdata.ysize;
            bsa.bsa_XSrcFactor = pictdata.xsize;
            bsa.bsa_YSrcFactor = pictdata.ysize;
            bsa.bsa_DestX = 0;
            bsa.bsa_DestY = 0;
            bsa.bsa_DestWidth = xsize;
            bsa.bsa_DestHeight = ysize;
            bsa.bsa_XDestFactor = xsize;
            bsa.bsa_YDestFactor = ysize;
            bsa.bsa_DestBitMap = bm;
        }

        if (scaling_needed) {
            /* Fill in scaling data for map rendering */
            memset(&bsm, 0, sizeof(bsm));
            bsm.bsa_SrcX = bsm.bsa_SrcY = 0;
            bsm.bsa_SrcBitMap = tile;

            bsm.bsa_SrcWidth = pictdata.xsize;
            bsm.bsa_SrcHeight = pictdata.ysize;

            bsm.bsa_XSrcFactor = pictdata.xsize;
            bsm.bsa_YSrcFactor = pictdata.ysize;

            bsm.bsa_DestWidth = mxsize;
            bsm.bsa_DestHeight = mysize;

            bsm.bsa_XDestFactor = mxsize;
            bsm.bsa_YDestFactor = mysize;
            bsm.bsa_DestBitMap = rp->BitMap;
            bsm.bsa_DestY = bsm.bsa_DestX = 0;

            imgbm = MyAllocBitMap(mxsize, mysize,
                                  pictdata.nplanes + amii_extraplanes,
                                  MEMF_CLEAR | MEMF_CHIP);
            if (imgbm == NULL) {
                amii_putstr(WIN_MESSAGE, 0,
                            "Can't allocate scaling bitmap for map window");
            } else
                bsm.bsa_DestBitMap = imgbm;
        }

        /* Go ahead and start dumping the stuff */
        for (i = 0; i < glyph_node_index; ++i) {
            /* Do it */
            int offx, offy, j;
            struct BitMap *nodebm = amiv_g_nodes[i].bitmap;

            /* Get the unclipped coordinates */
            x = amiv_g_nodes[i].odstx;
            y = amiv_g_nodes[i].odsty;

            /* If image is not in CHIP. copy each plane into tile line by line
             */

            offx = amiv_g_nodes[i].srcx / 8; /* 8 is bits per byte */
            offy = amiv_g_nodes[i].srcy * nodebm->BytesPerRow;
            for (j = 0; j < pictdata.nplanes + amii_extraplanes; ++j) {
                for (k = 0; k < pictdata.ysize; ++k) {
                    /* For a 16x16 tile, this could just be short assignments,
                     * but
                     * this code is generalized to handle any size tile
                     * image...
                     */
                    memcpy(tile->Planes[j] + k * tile->BytesPerRow,
                           nodebm->Planes[j] + offx + offy
                               + (nodebm->BytesPerRow * k),
                           pictdata.xsize / 8);
                }
            }

            if (!clipping || (x >= clipx && y >= clipy && x < clipxmax
                              && y < clipymax)) {
                /* scaling is needed, do it */
                if (scaling_needed) {
                    BitMapScale(&bsm);
                    BltBitMapRastPort(imgbm, 0, 0, rp, amiv_g_nodes[i].dstx,
                                      amiv_g_nodes[i].dsty, mxsize, mysize,
                                      0xc0);
                } else {
                    BltBitMapRastPort(tile, 0, 0, rp, amiv_g_nodes[i].dstx,
                                      amiv_g_nodes[i].dsty, pictdata.xsize,
                                      pictdata.ysize, 0xc0);
                }
            }
            /* Draw the overview window unless we are scrolling the map raster
             * around */
            if (bm && w && reclip != 2) {
                BitMapScale(&bsa);
                BltBitMapRastPort(
                    rast.BitMap, 0, 0, w->RPort,
                    w->BorderLeft + amiv_g_nodes[i].odstx * xsize,
                    w->BorderTop + amiv_g_nodes[i].odsty * ysize, xsize,
                    ysize, 0xc0);
            }
        }

        if (imgbm)
            MyFreeBitMap(imgbm);
        if (bm)
            MyFreeBitMap(bm);

    amii_end_glyphout(WIN_MAP);

    /* Clean up */
    glyph_node_index = glyph_buffer_index = 0;
}

/*
 * Glyph buffering routine.  Called instead of WindowPuts().
 */
void
amiv_lprint_glyph(winid window, int color_index, int glyph)
{
    int base = 0;
    struct amii_WinDesc *cw;
    struct Window *w;
    int curx;
    int cury;
    int icon;
    int xoff, yoff;

    /* Skip NO_GLYPH — nothing to draw */
    if (glyph == NO_GLYPH)
        return;

    icon = GlyphToIcon(glyph);

    if ((cw = amii_wins[window]) == (struct amii_WinDesc *) NULL)
        panic("bad winid in amiv_lprint_glyph: %d", window);

    w = cw->win;

    if (glyph < 10000) {
        if (icon >= pictdata.npics)
            icon = 0;  /* fallback for out-of-range */

        /* Get the relative offset in the page */

        /* How many pixels to account for y distance down */
        yoff = ((icon - base) / pictdata.across) * pictdata.ysize;

        /* How many pixels to account for x distance across */
        xoff = ((icon - base) % pictdata.across) * pictdata.xsize;
    }

    if (glyph >= 10000) {
        /* Run a single ASCII character out to the rastport right now */
        char c = glyph - 10000;
        int xxx, xxy;
        struct RastPort *rp = w->RPort;

        Move(rp, xxx = (((cw->curx - clipx) * rp->TxWidth) + w->BorderLeft),
             xxy = (w->BorderTop + (((cw->cury - clipy) + 1) * rp->TxHeight)
                    + 1));
        Text(rp, &c, 1);
        /* XXX this shouldn't be necessary: */
        if (cw->cursx == xxx && cw->cursy == xxy) {
            cw->wflags &= ~FLMAP_CURSUP;
        }
        cw->curx += rp->TxWidth; /* keep things in sync */
        return;
    }

    if (cw->type == NHW_MAP) {
        curx = cw->curx - clipx;
        cury = cw->cury - clipy;

        /* See if we're out of glyph nodes */
        if (glyph_node_index >= NUMBER_GLYPH_NODES)
            amiv_flush_glyph_buffer(w);

        /* Fill in the node. */
        amiv_g_nodes[glyph_node_index].dsty =
            min(w->BorderTop + (cury * mysize), w->Height - 1);

        amiv_g_nodes[glyph_node_index].dstx =
            min(w->BorderLeft + (curx * mxsize), w->Width - 1);
        amiv_g_nodes[glyph_node_index].odsty = cw->cury;
        amiv_g_nodes[glyph_node_index].odstx = cw->curx;
        amiv_g_nodes[glyph_node_index].srcx = xoff;
        amiv_g_nodes[glyph_node_index].srcy = yoff;
        amiv_g_nodes[glyph_node_index].bitmap = tileimg;
        ++glyph_node_index;
    } else {
        /* Do it */
        int j, k, x, y, apen;
        struct RastPort *rp = w->RPort;
        x = rp->cp_x - pictdata.xsize - 3;

        y = rp->cp_y - pictdata.ysize + 1;

        if (glyph != NO_GLYPH) {
            struct BitMap *bm = tileimg;

            /* 8 bits per byte */
            xoff /= 8;
            yoff *= bm->BytesPerRow;
            for (j = 0; j < pictdata.nplanes; ++j) {
                for (k = 0; k < pictdata.ysize; ++k) {
                    memcpy(tile->Planes[j] + k * tile->BytesPerRow,
                           bm->Planes[j] + xoff + yoff
                               + (bm->BytesPerRow * k),
                           pictdata.xsize / 8);
                }
            }

            BltBitMapRastPort(tile, 0, 0, rp, x, y, pictdata.xsize,
                              pictdata.ysize, 0xc0);

            apen = rp->FgPen;
            SetAPen(rp, sysflags.amii_dripens[SHINEPEN]);
            Move(rp, x - 1, y + pictdata.ysize);
            Draw(rp, x - 1, y - 1);
            Draw(rp, x + pictdata.xsize, y - 1);
            SetAPen(rp, sysflags.amii_dripens[SHADOWPEN]);
            Move(rp, x + pictdata.xsize, y);
            Draw(rp, x + pictdata.xsize, y + pictdata.ysize);
            Draw(rp, x, y + pictdata.ysize);
            SetAPen(rp, apen);
        } else if (x > w->BorderLeft) {
            int apen, bpen;
            apen = rp->FgPen;
            bpen = rp->BgPen;
            SetAPen(rp, amii_menuBPen);
            SetBPen(rp, amii_menuBPen);
            RectFill(rp, x - 1, y - 1, x + pictdata.xsize,
                     y + pictdata.ysize);
            SetAPen(rp, apen);
            SetBPen(rp, bpen);
        }
    }
}

/*
 * Define some variables which will be used to save context when toggling
 * back and forth between low level text and console I/O.
 */
static long xsave, ysave, modesave, apensave, bpensave;
static int usecolor;

/*
 * The function is called before any glyphs are driven to the screen.  It
 * removes the cursor, saves internal state of the window, then returns.
 */

void
amiv_start_glyphout(winid window)
{
    struct amii_WinDesc *cw;
    struct Window *w;

    if ((cw = amii_wins[window]) == (struct amii_WinDesc *) NULL)
        panic("bad winid %d in start_glyphout()", window);

    if (cw->wflags & FLMAP_INGLYPH)
        return;

    if (!(w = cw->win))
        panic("bad winid %d, no window ptr set", window);

    /*
     * Save the context of the window
     */
    xsave = w->RPort->cp_x;
    ysave = w->RPort->cp_y;
    modesave = w->RPort->DrawMode;
    apensave = w->RPort->FgPen;
    bpensave = w->RPort->BgPen;

    /*
     * Set the mode, and be done with it
     */
    usecolor = iflags.use_color;
    iflags.use_color = FALSE;
    cw->wflags |= FLMAP_INGLYPH;
}

/*
 * General cleanup routine -- flushes and restores cursor
 */
void
amii_end_glyphout(winid window)
{
    struct amii_WinDesc *cw;
    struct Window *w;

    if ((cw = amii_wins[window]) == (struct amii_WinDesc *) NULL)
        panic("bad window id %d in amii_end_glyphout()", window);

    if ((cw->wflags & FLMAP_INGLYPH) == 0)
        return;
    cw->wflags &= ~(FLMAP_INGLYPH);

    if (!(w = cw->win))
        panic("bad winid %d, no window ptr set", window);

    /*
     * Clean up whatever is left in the buffer
     */
    iflags.use_color = usecolor;

    /*
     * Reset internal data structs
     */
    SetAPen(w->RPort, apensave);
    SetBPen(w->RPort, bpensave);
    SetDrMd(w->RPort, modesave);

    Move(w->RPort, xsave, ysave);
}

static int maze_type = COL_MAZE_BRICK;

void
SetMazeType(MazeType t)
{
    maze_type = t;
}

int
GlyphToIcon(int glyph)
{
    glyph_info gi;

    map_glyphinfo(0, 0, glyph, 0, &gi);
    if (glyph >= 10000)
        return glyph;
    return (gi.gm.tileidx);
}

#ifdef AMII_GRAPHICS

struct amii_glyph_node {
    short x;
    short y;
    short len;
    unsigned char bg_color;
    unsigned char fg_color;
    char *buffer;
};
static struct amii_glyph_node amii_g_nodes[NUMBER_GLYPH_NODES];
static char amii_glyph_buffer[GLYPH_BUFFER_SIZE];

/*
 * Map our amiga-specific colormap into the colormap specified in color.h.
 * See winami.c for the amiga specific colormap.
 */

/* CLR_BLACK (slot 0) renders as the dim blue pen on black background,
   CLR_WHITE (slot 15) uses white on black.  Slots 9-14 use the inverse-
   video trick (fg=black, bg=color) to fit 16 logical colors into 8 pens. */
int foreg[AMII_MAXCOLORS] = {
    6, 7, 4, 2, 6, 5, 3, 1, 1, 0, 0, 0, 0, 0, 0, 1
};
int backg[AMII_MAXCOLORS] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 4, 1, 6, 5, 3, 0
};
/*
 * Begin Revamped Text display routines
 *
 * Up until version 3.1, the only method for displaying text on the playing
 * field was by using the console.device.  This was nice for a number of
 * reasons, the most signifigant of which was a lot of the nuts and bolts was
 * done for you via escape sequences interpreted by said device.  This did
 * not come without a price however.  And that price was speed. It has now
 * come to a point where the speed has now been deemed unacceptable.
 *
 * The following series of routines are designed to drop into the current
 * nethack display code, using hooks provided for such a measure. It works
 * on similar principals as the WindowPuts(), buffering I/O internally
 * until either an explicit flush or internal buffering is exceeded, thereby
 * forcing the flush.  The output (or glyphs) does not go to the
 * console.device, however.  It is driven directly to the rasterport of the
 * nethack window via the low-level Text() calls, increasing the speed by
 * a very signifigant factor.
 */
/*
 * Routine to simply flush whatever is buffered
 */
void
amii_flush_glyph_buffer(struct Window *w)
{
    short i, x, y;
    struct RastPort *rp = w->RPort;

    /* If nothing is buffered, return before we do anything */
    if (glyph_node_index == 0)
        return;

    cursor_off(WIN_MAP);
    amii_start_glyphout(WIN_MAP);

    /* Set up the drawing mode */
    SetDrMd(rp, JAM2);

    /* Go ahead and start dumping the stuff */
    for (i = 0; i < glyph_node_index; ++i) {
        /* These coordinate calculations must be synced with the
         * code in amii_curs() in winfuncs.c.  curs_on_u() calls amii_curs()
         * to draw the cursor on top of the player
         */
        y = w->BorderTop + (amii_g_nodes[i].y - 2) * rp->TxHeight
            + rp->TxBaseline + 1;
        x = amii_g_nodes[i].x * rp->TxWidth + w->BorderLeft;

        /* Skip if pixel coordinates are outside window */
        if (x < 0 || y < 0 || y >= w->Height || x >= w->Width)
            continue;

        /* Move pens to correct location */
        Move(rp, (long) x, (long) y);

        /* Setup the colors */
        SetAPen(rp, (long) amii_g_nodes[i].fg_color);
        SetBPen(rp, (long) amii_g_nodes[i].bg_color);

        /* Do it */
        Text(rp, amii_g_nodes[i].buffer, amii_g_nodes[i].len);
    }

    amii_end_glyphout(WIN_MAP);
    /* Clean up */
    glyph_node_index = glyph_buffer_index = 0;
}
void
amiga_print_glyph(winid window, int color_index, int glyph)
{
    if (WINVERS_AMIV)
        amiv_lprint_glyph(window, color_index, glyph);
    else
        amii_lprint_glyph(window, color_index, glyph);
}

/*
 * Glyph buffering routine.  Called instead of WindowPuts().
 */
void
amii_lprint_glyph(winid window, int color_index, int glyph)
{
    int fg_color, bg_color;
    struct amii_WinDesc *cw;
    struct Window *w;
    int curx;
    int cury;

    if ((cw = amii_wins[window]) == (struct amii_WinDesc *) NULL)
        panic("bad winid in amii_lprint_glyph: %d", window);

    w = cw->win;
    curx = cw->curx;
    cury = cw->cury;

    fg_color = foreg[color_index];
    bg_color = backg[color_index];

    /* See if we have enough character buffer space... */
    if (glyph_buffer_index >= GLYPH_BUFFER_SIZE)
        amii_flush_glyph_buffer(w);

    /*
     * See if we can append it to the current active node of glyph buffer. It
     * must satisfy the following conditions:
     *
     *    * background colors are the same, AND
     *    * foreground colors are the same, AND
     *    * they are precisely side by side
     */
    if ((glyph_buffer_index != 0)
        && (fg_color == amii_g_nodes[glyph_node_index - 1].fg_color)
        && (bg_color == amii_g_nodes[glyph_node_index - 1].bg_color)
        && (amii_g_nodes[glyph_node_index - 1].x
                + amii_g_nodes[glyph_node_index - 1].len
            == curx) && (amii_g_nodes[glyph_node_index - 1].y == cury)) {
        /*
         * Add it to the end of the buffer
         */
        amii_glyph_buffer[glyph_buffer_index++] = (char) glyph;
        amii_g_nodes[glyph_node_index - 1].len++;
    } else {
        /* See if we're out of glyph nodes */
        if (glyph_node_index >= NUMBER_GLYPH_NODES)
            amii_flush_glyph_buffer(w);
        amii_g_nodes[glyph_node_index].len = 1;
        amii_g_nodes[glyph_node_index].x = curx;
        amii_g_nodes[glyph_node_index].y = cury;
        amii_g_nodes[glyph_node_index].fg_color = fg_color;
        amii_g_nodes[glyph_node_index].bg_color = bg_color;
        amii_g_nodes[glyph_node_index].buffer =
            &amii_glyph_buffer[glyph_buffer_index];
        amii_glyph_buffer[glyph_buffer_index] = glyph;
        ++glyph_buffer_index;
        ++glyph_node_index;
    }
}

/*
 * Variables used to save context when toggling between low level text
 * and console I/O.
 */
static long xsave, ysave, modesave, apensave, bpensave;
static int usecolor;

/*
 * The function is called before any glyphs are driven to the screen.  It
 * removes the cursor, saves internal state of the window, then returns.
 */

void
amii_start_glyphout(winid window)
{
    struct amii_WinDesc *cw;
    struct Window *w;

    if ((cw = amii_wins[window]) == (struct amii_WinDesc *) NULL)
        panic("bad winid %d in start_glyphout()", window);

    if (cw->wflags & FLMAP_INGLYPH)
        return;

    if (!(w = cw->win))
        panic("bad winid %d, no window ptr set", window);

    /*
     * Save the context of the window
     */
    xsave = w->RPort->cp_x;
    ysave = w->RPort->cp_y;
    modesave = w->RPort->DrawMode;
    apensave = w->RPort->FgPen;
    bpensave = w->RPort->BgPen;

    /*
     * Set the mode, and be done with it
     */
    usecolor = iflags.use_color;
    iflags.use_color = FALSE;
    cw->wflags |= FLMAP_INGLYPH;
}

#endif

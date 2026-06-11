/* NetHack 5.0	mactile.c	*/
/* Copyright (c) Ingo Paschke, 2026. */
/* NetHack may be freely redistributed.  See license for details. */
/* mactile.c — tile sheet asset + per-tile blit. See mactile.h. */
#include "hack.h"
#include "macwin.h"
#include "mactile.h"
#include <Gestalt.h>
#include <QDOffscreen.h>
#include <Palettes.h>
#include <Resources.h>

/* --- module state --- */
static GWorldPtr     gTileSheet     = NULL;
static short         gSheetDepth    = 0;
static short         gSheetCols     = 0;   /* tiles across in the sheet */
static short         gSheetRows     = 0;   /* tiles down in the sheet */
static short         gCursorClutIdx = -2;  /* cached farlook-cursor color index;
                                              -2 = unset, -1 = none, >=0 = index */

/* --- helper: load a PICT resource into an offscreen GWorld --- */
static Boolean
load_tile_pict(short pict_id, short depth)
{
    PicHandle ph = GetPicture(pict_id);
    if (!ph) {
        mac_dprintf("mactile: GetPicture(%d) returned NULL\n", (int) pict_id);
        return false;
    }
    HLock((Handle) ph);
    Rect frame = (**ph).picFrame;
    HUnlock((Handle) ph);
    OffsetRect(&frame, -frame.left, -frame.top);

    QDErr err = NewGWorld(&gTileSheet, depth, &frame, NULL, NULL, 0);
    if (err != noErr || !gTileSheet) {
        mac_dprintf("mactile: NewGWorld failed err=%d\n", (int) err);
        ReleaseResource((Handle) ph);
        return false;
    }
    PixMapHandle pm = GetGWorldPixMap(gTileSheet);
    NoPurgePixels(pm);   /* tile sheet stays resident */
    LockPixels(pm);      /* kept locked for the sheet's lifetime (per-blit locking
                            this resident pixmap thousands of times per redraw is
                            pure overhead); released by DisposeGWorld in shutdown */
    GWorldPtr saveW; GDHandle saveD;
    GetGWorld(&saveW, &saveD);
    SetGWorld(gTileSheet, NULL);
    EraseRect(&frame);
    DrawPicture(ph, &frame);
    QDErr draw_err = QDError();           /* capture while gTileSheet is current */
    SetGWorld(saveW, saveD);
    ReleaseResource((Handle) ph);

    if (draw_err != noErr) {
        mac_dprintf("mactile: DrawPicture err=%d\n", (int) draw_err);
        DisposeGWorld(gTileSheet);
        gTileSheet = NULL;
        return false;
    }
    gSheetDepth = depth;
    gSheetCols  = (frame.right - frame.left) / MACTILE_DIM;
    gSheetRows  = (frame.bottom - frame.top) / MACTILE_DIM;
    return true;
}

Boolean
mactile_available(void)
{
    GDHandle gd = GetMainDevice();
    if (!gd) return false;
    short depth = (*(*gd)->gdPMap)->pixelSize;
    return depth >= 4;
}

Boolean
mactile_init(void)
{
    if (gTileSheet) return true;        /* idempotent */
    if (!mactile_available()) return false;

    GDHandle gd = GetMainDevice();
    short screen_depth = (*(*gd)->gdPMap)->pixelSize;
    short pict_id = (screen_depth >= 8) ? 1001 : 1000;
    short depth   = (screen_depth >= 8) ? 8    : 4;

    if (!load_tile_pict(pict_id, depth)) return false;

    return true;
}

short
mactile_sheet_depth(void)
{
    return gSheetDepth;
}

CTabHandle
mactile_sheet_ctable(void)
{
    if (!gTileSheet) return NULL;
    return (**GetGWorldPixMap(gTileSheet)).pmTable;
}

/* CLUT index of a bright, non-white tile color (scored R+G-B, favors yellow)
   for the farlook cursor; near-white skipped as the white slot triggers a CLUT
   reorg.  Cached.  Returns -1 if no sheet/ctable or no usable color. */
short
mactile_cursor_clut_index(void)
{
    if (gCursorClutIdx == -2) {
        gCursorClutIdx = -1;
        CTabHandle ct = mactile_sheet_ctable();
        if (ct && *ct) {
            short n = (**ct).ctSize;      /* ctSize is (count - 1) */
            long  bestscore = -0x7FFFFFFFL;
            short i;
            /* the result feeds PmForeColor against the map window's
               TILE_PALETTE_ENTRIES-entry palette; an index beyond it
               would read past the palette's pmInfo array */
            if (n > TILE_PALETTE_ENTRIES - 1)
                n = TILE_PALETTE_ENTRIES - 1;
            for (i = 0; i <= n; i++) {
                RGBColor c = (**ct).ctTable[i].rgb;
                if (c.red > 0xC000 && c.green > 0xC000 && c.blue > 0xC000)
                    continue;             /* skip near-white */
                long score = (long) c.red + (long) c.green - (long) c.blue;
                if (score > bestscore) { bestscore = score; gCursorClutIdx = i; }
            }
        }
    }
    return gCursorClutIdx;
}

void
mactile_blit_to(GWorldPtr dst, int tile_idx, short dst_x, short dst_y)
{
    if (!gTileSheet || !dst) return;
    /* Bounds-check tile_idx: an out-of-sheet source rect would read stray
       PixMap memory. */
    if (tile_idx < 0 || tile_idx >= (int) gSheetCols * (int) gSheetRows) {
        mac_dprintf("mactile: tile_idx %d out of sheet (max %d)\n",
                    tile_idx, (int) gSheetCols * (int) gSheetRows - 1);
        return;
    }
    short sx = (tile_idx % gSheetCols) * MACTILE_DIM;
    short sy = (tile_idx / gSheetCols) * MACTILE_DIM;
    Rect src = { sy, sx, sy + MACTILE_DIM, sx + MACTILE_DIM };
    Rect dr  = { dst_y, dst_x, dst_y + MACTILE_DIM, dst_x + MACTILE_DIM };

    PixMapHandle spm = GetGWorldPixMap(gTileSheet);   /* sheet stays locked */
    PixMapHandle dpm = GetGWorldPixMap(dst);
    if (!LockPixels(dpm)) {
        mac_dprintf("mactile: LockPixels(dst) failed (purged?)\n");
        return;
    }
    GWorldPtr saveW; GDHandle saveD;
    GetGWorld(&saveW, &saveD);
    SetGWorld(dst, NULL);
    CopyBits((BitMap *) *spm, (BitMap *) *dpm,
             &src, &dr, srcCopy, NULL);
    SetGWorld(saveW, saveD);
    UnlockPixels(dpm);
}

void
mactile_blit_to_window(WindowPtr dst, int tile_idx, short dst_x, short dst_y)
{
    if (!gTileSheet || !dst) return;
    if (tile_idx < 0 || tile_idx >= (int) gSheetCols * (int) gSheetRows) {
        mac_dprintf("mactile: tile_idx %d out of sheet (max %d)\n",
                    tile_idx, (int) gSheetCols * (int) gSheetRows - 1);
        return;
    }
    short sx = (tile_idx % gSheetCols) * MACTILE_DIM;
    short sy = (tile_idx / gSheetCols) * MACTILE_DIM;
    Rect src = { sy, sx, sy + MACTILE_DIM, sx + MACTILE_DIM };
    Rect dr  = { dst_y, dst_x, dst_y + MACTILE_DIM, dst_x + MACTILE_DIM };

    PixMapHandle spm = GetGWorldPixMap(gTileSheet);   /* sheet stays locked */
    GrafPtr saveP; GetPort(&saveP);
    SetPort(dst);
    CopyBits((BitMap *) *spm,
             GetPortBitMapForCopyBits(GetWindowPort(dst)),
             &src, &dr, srcCopy, NULL);
    SetPort(saveP);
}

/* NetHack 5.0	mactile.h	*/
/* Copyright (c) Ingo Paschke, 2026. */
/* NetHack may be freely redistributed.  See license for details. */
/* mactile.h: tile sheet asset + per-tile blit. See macmap.h for the
   map window owner. */
#ifndef MACTILE_H
#define MACTILE_H

#include "macwin.h"
#include <Palettes.h>
#include <QDOffscreen.h>

/* Tile edge length in pixels.  Must match the sheet emitted by
   util/tile2pict (TILE_X/TILE_Y in win/share/tile.h, currently 16). */
#define MACTILE_DIM 16

/* Size of the pmTolerant palette macmap attaches to the map window at
   8bpp. NewPalette copies the first n sheet-CLUT entries, so anything
   handed to PmForeColor must stay below this. */
#define TILE_PALETTE_ENTRIES 32

extern Boolean mactile_available(void);    /* depth >= 4bpp */
extern Boolean mactile_init(void);         /* load PICT 1000/1001 into GWorld */

/* Where the tilesheet was loaded; needed by macmap to attach a Palette. */
extern short      mactile_sheet_depth(void);
extern CTabHandle mactile_sheet_ctable(void);

/* Blit a single tile into a destination GWorld at (dst_x, dst_y).
   The function manages SetGWorld save/restore internally. */
extern void    mactile_blit_to(GWorldPtr dst,
                                int tile_idx,
                                short dst_x, short dst_y);

/* Batch variant: blit into the CURRENT GWorld, already locked by the
   caller (macmap's cell batch). No SetGWorld/LockPixels -- a nested
   unlock would clear the caller's lock. */
extern void    mactile_blit_in_place(int tile_idx,
                                      short dst_x, short dst_y);

/* Blit a single tile into a destination Window at (dst_x, dst_y).
   The function manages SetPort save/restore internally. */
extern void    mactile_blit_to_window(WindowPtr dst,
                                       int tile_idx,
                                       short dst_x, short dst_y);

/* CLUT index of a bright, non-white tile color for the farlook cursor.
   Use with PmForeColor(index): sets the foreground straight to that slot, no
   render, so it can't recolor the map.  Returns -1 if no sheet/ctable. */
extern short   mactile_cursor_clut_index(void);

#endif /* MACTILE_H */

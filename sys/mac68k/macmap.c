/* NetHack 5.0	macmap.c	*/
/* Copyright (c) Ingo Paschke, 2026. */
/* NetHack may be freely redistributed.  See license for details. */
/* macmap.c — separate map window for the Mac 68k port. See macmap.h. */
#include "hack.h"
#include "macwin.h"
#include "mactty.h"
#include "macmap.h"
#include "mactile.h"
#include <Resources.h>
#include <QDOffscreen.h>
#include <Palettes.h>
#include <Controls.h>

/* src/tile.c reserves a tile slot per statue-monster, but our PICT sheet has
   only the one generic statue tile; remap any out-of-sheet index to it. */
extern int maxothtile;
extern glyph_map glyphmap[MAX_GLYPH];

static short
remap_tile_idx(int idx)
{
    if (idx <= maxothtile) return (short) idx;
    static short statue_tile = -1;
    if (statue_tile < 0)
        statue_tile = glyphmap[GLYPH_OBJ_OFF + STATUE].tileidx;
    return statue_tile;
}

typedef struct {
    NhWindow      *owner;
    Boolean        tile_mode;
    GWorldPtr      backing;
    PaletteHandle  palette;
    short          cell_w, cell_h;
    short          vis_cols, vis_rows;
    short          scroll_col, scroll_row;
    short          tile_cache[ROWNO][COLNO];
    unsigned char  text_cache[ROWNO][COLNO];
    unsigned char  text_color[ROWNO][COLNO];
    /* software cursor for getpos/farlook: track the framed cell to un-frame it */
    Boolean        cursor_on;
    short          cursor_x, cursor_y;
    ControlHandle  vscroll, hscroll;   /* functional scrollbars (decorated only) */
    Boolean        decorated;          /* documentProc with chrome/strips */
    short          inset_r, inset_b;   /* reserved strip widths (0 = borderless) */
    /* Per-cell draws paint the backing and union this dirty rect; macmap_flush
       blits it to the window once per frame (at display_nhwindow / curs) instead
       of one CopyBits per cell. Coords are backing/window-local. */
    Rect           dirty;
    Boolean        has_dirty;
} MacMapState;

static MacMapState gMap = {0};

/* action proc for live scrollbar tracking; created lazily in macmap_click */
static ControlActionUPP gMapScrollUPP = NULL;

static void repaint_full_viewport(void);
static void scroll_viewport_to(short new_col, short new_row);
static void draw_cursor_border(int col, int row);

/* Union a backing-local cell rect into the pending dirty region. */
static void
mark_dirty(const Rect *cell)
{
    if (!gMap.has_dirty) { gMap.dirty = *cell; gMap.has_dirty = true; }
    else UnionRect(cell, &gMap.dirty, &gMap.dirty);
}

/* Mark the whole viewport dirty (a scroll/full repaint changed every pixel). */
static void
mark_dirty_all(void)
{
    SetRect(&gMap.dirty, 0, 0,
            gMap.vis_cols * gMap.cell_w, gMap.vis_rows * gMap.cell_h);
    gMap.has_dirty = true;
}

/* Drawable map area = port bounds minus the scrollbar strips (0 when borderless). */
static void
map_content_bounds(Rect *out)
{
    if (!gMap.owner || !gMap.owner->its_window) { SetRect(out, 0, 0, 0, 0); return; }
    GetWindowPortBounds(gMap.owner->its_window, out);
    out->right  -= gMap.inset_r;
    out->bottom -= gMap.inset_b;
}

/* Position the scrollbar controls in the right/bottom strips; call after any
   SizeWindow (no-op when borderless). The bars stop short of the grow-box corner. */
static void
layout_scroll_controls(void)
{
    Rect b;
    if (!gMap.decorated || !gMap.vscroll || !gMap.hscroll || !gMap.owner
        || !gMap.owner->its_window)
        return;
    GetWindowPortBounds(gMap.owner->its_window, &b);
    HideControl(gMap.vscroll); HideControl(gMap.hscroll);
    MoveControl(gMap.vscroll, b.right - 15, b.top - 1);
    SizeControl(gMap.vscroll, 16, (b.bottom - 14) - (b.top - 1));
    MoveControl(gMap.hscroll, b.left - 1, b.bottom - 15);
    SizeControl(gMap.hscroll, (b.right - 14) - (b.left - 1), 16);
    ShowControl(gMap.vscroll); ShowControl(gMap.hscroll);
}

/* Reflect the viewport position in the scrollbar thumbs (no-op when borderless).
   Vertical = rows; horizontal = cols 1..COLNO-1 (col 0 unused). */
static void
update_scroll_controls(void)
{
    short vmax, vval, hmax, hval;
    if (!gMap.decorated || !gMap.vscroll || !gMap.hscroll) return;
    vmax = ROWNO - gMap.vis_rows;
    if (vmax < 0) vmax = 0;
    vval = gMap.scroll_row;
    if (vval > vmax) vval = vmax;
    if (vval < 0) vval = 0;
    SetControlMaximum(gMap.vscroll, vmax);
    SetControlValue(gMap.vscroll, vval);
    hmax = (COLNO - 1) - gMap.vis_cols;
    if (hmax < 0) hmax = 0;
    hval = gMap.scroll_col - 1;   /* scroll_col is 1-based (col 0 unused) */
    if (hval > hmax) hval = hmax;
    if (hval < 0) hval = 0;
    SetControlMaximum(gMap.hscroll, hmax);
    SetControlValue(gMap.hscroll, hval);
}

/* NetHack color indices to RGB. Values are 16-bit per channel (Mac
   QuickDraw convention; 8-bit values multiplied by 257 for full range). */
#define R16(v) ((unsigned short)((v) * 257))
static const RGBColor gNhColorRGB[16] = {
    {R16(0x00), R16(0x00), R16(0x00)},   /* 0  CLR_BLACK   */
    {R16(0xC0), R16(0x00), R16(0x00)},   /* 1  CLR_RED     */
    {R16(0x00), R16(0x80), R16(0x00)},   /* 2  CLR_GREEN   */
    {R16(0x80), R16(0x80), R16(0x00)},   /* 3  CLR_BROWN   */
    {R16(0x00), R16(0x00), R16(0xC0)},   /* 4  CLR_BLUE    */
    {R16(0x80), R16(0x00), R16(0x80)},   /* 5  CLR_MAGENTA */
    {R16(0x00), R16(0x80), R16(0x80)},   /* 6  CLR_CYAN    */
    {R16(0xC0), R16(0xC0), R16(0xC0)},   /* 7  CLR_GRAY    */
    {R16(0x80), R16(0x80), R16(0x80)},   /* 8  NO_COLOR    */
    {R16(0xFF), R16(0x80), R16(0x00)},   /* 9  CLR_ORANGE  */
    {R16(0x00), R16(0xFF), R16(0x00)},   /* 10 CLR_BRIGHT_GREEN */
    {R16(0xFF), R16(0xFF), R16(0x00)},   /* 11 CLR_YELLOW  */
    {R16(0x00), R16(0x80), R16(0xFF)},   /* 12 CLR_BRIGHT_BLUE */
    {R16(0xFF), R16(0x00), R16(0xFF)},   /* 13 CLR_BRIGHT_MAGENTA */
    {R16(0x00), R16(0xFF), R16(0xFF)},   /* 14 CLR_BRIGHT_CYAN */
    {R16(0xFF), R16(0xFF), R16(0xFF)}    /* 15 CLR_WHITE   */
};

static void
set_nh_color(int color)
{
    if (color < 0 || color >= 16) color = 8;   /* NO_COLOR */
    RGBColor c = gNhColorRGB[color];
    /* color 0 (black) would be invisible on the black map bg; show it as dark gray */
    if (color == 0) { c.red = c.green = c.blue = R16(0x55); }
    RGBForeColor(&c);
}

static Boolean
allocate_backing(void)
{
    if (gMap.backing) {
        DisposeGWorld(gMap.backing);
        gMap.backing = NULL;
    }
    Rect r;
    SetRect(&r, 0, 0,
            gMap.vis_cols * gMap.cell_w,
            gMap.vis_rows * gMap.cell_h);

    short depth;
    if (gMap.tile_mode) {
        depth = mactile_sheet_depth();
        if (depth < 4) depth = 8;
    } else {
        GDHandle gd = GetMainDevice();
        depth = (*(*gd)->gdPMap)->pixelSize;
        if (depth > 8) depth = 8;
        if (depth < 1) depth = 1;
    }

    QDErr err = NewGWorld(&gMap.backing, depth, &r, NULL, NULL, 0);
    if (err != noErr || !gMap.backing) {
        mac_dprintf("macmap: NewGWorld failed err=%d (depth=%d)\n",
                    (int) err, (int) depth);
        gMap.backing = NULL;
        return false;
    }

    /* pin the backing so the Memory Manager can't purge it under CopyBits */
    {
        PixMapHandle pm = GetGWorldPixMap(gMap.backing);
        NoPurgePixels(pm);
    }
    /* fg=black/bg=white is REQUIRED so the tile CopyBits isn't tinted; the map
       font lets DrawChar render real glyphs. (text path flips bg per cell.) */
    GWorldPtr saveW; GDHandle saveD;
    GetGWorld(&saveW, &saveD);
    SetGWorld(gMap.backing, NULL);
    PixMapHandle pm = GetGWorldPixMap(gMap.backing);
    if (!LockPixels(pm)) {
        /* pixels purged — tear down and fail rather than draw into bad memory */
        mac_dprintf("macmap: LockPixels failed in allocate_backing\n");
        SetGWorld(saveW, saveD);
        DisposeGWorld(gMap.backing);
        gMap.backing = NULL;
        return false;
    }
    {
        RGBColor white = {0xFFFF, 0xFFFF, 0xFFFF};
        RGBColor black = {0, 0, 0};
        RGBBackColor(&white);
        RGBForeColor(&black);
        {
            short fn = (gMap.owner && gMap.owner->font_number > 0)
                       ? gMap.owner->font_number : kFontIDMonaco;
            short fs = (gMap.owner && gMap.owner->font_size > 0)
                       ? gMap.owner->font_size : 9;
            TextFont(fn);
            TextSize(fs);
            TextFace(0);
            TextMode(srcCopy);
        }
    }
    EraseRect(&r);
    UnlockPixels(pm);
    SetGWorld(saveW, saveD);
    return true;
}

/* Copy a rect from the backing GWorld to the map window.  The two share a
   coordinate space -- both origin at the content top-left, one cell per
   gMap.cell_w/cell_h -- which is why callers can pass the SAME rect for
   src and dst. */
static void
blit_backing_to_window(const Rect *src_rect, const Rect *dst_rect)
{
    if (!gMap.backing || !gMap.owner || !gMap.owner->its_window) return;
    PixMapHandle pm = GetGWorldPixMap(gMap.backing);
    if (!LockPixels(pm)) {
        /* pixels purged — skip the blit */
        mac_dprintf("macmap: blit_backing_to_window: LockPixels failed\n");
        return;
    }
    GrafPtr saveP; GetPort(&saveP);
    SetPort(gMap.owner->its_window);
    CopyBits((BitMap *) *pm,
             GetPortBitMapForCopyBits(GetWindowPort(gMap.owner->its_window)),
             src_rect, dst_rect, srcCopy, NULL);
    SetPort(saveP);
    UnlockPixels(pm);
}

Boolean
macmap_create(NhWindow *map)
{
    if (!map) return false;
    if (gMap.owner) return true;   /* idempotent */

    short wind_id = small_screen ? kWindMapBorderless : kWindMapDocument;

    WindowPtr w = (WindowPtr) GetNewCWindow(wind_id, NULL, (WindowPtr) -1L);
    if (!w) {
        mac_dprintf("macmap: GetNewCWindow(%d) returned NULL\n", (int) wind_id);
        return false;
    }
    SetWRefCon(w, MACMAP_REFCON);
    SetWindowKind(w, WIN_BASE_KIND + NHW_MAP);
    map->its_window = w;
    ShowWindow(w);

    gMap.owner     = map;   /* set early so map_content_bounds is usable */
    gMap.decorated = !small_screen;
    gMap.inset_r   = gMap.decorated ? 15 : 0;
    gMap.inset_b   = gMap.decorated ? 15 : 0;
    if (gMap.decorated) {
        Rect b, vr, hr;
        GetWindowPortBounds(w, &b);
        SetRect(&vr, b.right - 15, b.top - 1,  b.right + 1, b.bottom - 14);
        SetRect(&hr, b.left - 1,  b.bottom - 15, b.right - 14, b.bottom + 1);
        /* nominal range until update_scroll_controls sets the real one.
           procID 16 == scrollBarProc */
        gMap.vscroll = NewControl(w, &vr, P_EMPTY_STRING, true, 0, 0, 1, 16, 0);
        gMap.hscroll = NewControl(w, &hr, P_EMPTY_STRING, true, 0, 0, 1, 16, 0);
    } else {
        gMap.vscroll = gMap.hscroll = NULL;
    }

    /* apply the saved text-mode size from the prefs file; position is
       deferred to SanePositions() */
    {
        Rect b; short sw, sh;
        GetWindowPortBounds(w, &b);
        if (RetrieveSize(kMapWindow, b.top, b.left, &sh, &sw))
            SizeWindow(w, sw, sh, false);
    }
    layout_scroll_controls();

    gMap.tile_mode   = false;
    gMap.backing     = NULL;
    gMap.palette     = NULL;
    /* safe defaults; macmap_finalize re-derives them from the NhWindow */
    gMap.cell_w      = 6;
    gMap.cell_h      = 14;
    gMap.vis_cols    = 80;
    gMap.vis_rows    = 21;
    gMap.scroll_col  = 1;   /* col 0 is unused */
    gMap.scroll_row  = 0;
    /* seed cache to -1 (empty): tile 0 is a real tile (giant ant), so zero-init
       would paint ants before the first print_glyph */
    {
        int rr, cc;
        for (rr = 0; rr < ROWNO; ++rr)
            for (cc = 0; cc < COLNO; ++cc)
                gMap.tile_cache[rr][cc] = -1;
    }
    /* Backing + tile-mode init deferred to macmap_finalize. */

    return true;
}

/* Called from mac_create_nhwindow AFTER get_tty_metrics has populated
   aWin->char_width / row_height / font_number. Re-derive cell metrics,
   recompute the viewport from actual window size, allocate the backing
   GWorld, and apply tile mode if NHDeflts asks. */
void
macmap_finalize(NhWindow *map)
{
    if (!map || gMap.owner != map || !map->its_window) return;
    if (map->char_width  > 0) gMap.cell_w = map->char_width;
    if (map->row_height  > 0) gMap.cell_h = map->row_height;
    {
        Rect cr; map_content_bounds(&cr);
        gMap.vis_cols = (cr.right - cr.left) / gMap.cell_w;
        gMap.vis_rows = (cr.bottom - cr.top) / gMap.cell_h;
        if (gMap.vis_cols < 1) gMap.vis_cols = 1;
        if (gMap.vis_rows < 1) gMap.vis_rows = 1;
    }
    if (!allocate_backing()) {
        mac_dprintf("macmap: backing alloc failed at finalize\n");
    }
    if (iflags.wc_tiled_map && mactile_available()) {
        macmap_set_mode(map, true);
    }
    /* centering happens lazily in macmap_print_glyph; u.ux/u.uy may be unset here */
}

void
macmap_destroy(NhWindow *map)
{
    if (!map || gMap.owner != map) return;
    if (gMap.backing) { DisposeGWorld(gMap.backing); gMap.backing = NULL; }
    if (gMap.palette) { DisposePalette(gMap.palette); gMap.palette = NULL; }
    if (map->its_window) {
        /* DisposeWindow disposes attached controls; just drop our handles */
        gMap.vscroll = gMap.hscroll = NULL;
        DisposeWindow(map->its_window);
        map->its_window = NULL;
    }
    if (gMapScrollUPP) {   /* lazily recreated in macmap_click if needed */
        DisposeControlActionUPP(gMapScrollUPP);
        gMapScrollUPP = NULL;
    }
    gMap.owner = NULL;
}

Boolean
macmap_set_mode(NhWindow *map, Boolean tile_mode)
{
    if (!map || gMap.owner != map) return false;

    if (tile_mode && !mactile_init()) return false;
    gMap.tile_mode = tile_mode;
    map->tile_mode = tile_mode;   /* keep NhWindow field in sync for macwin/mactty */
    if (tile_mode) {
        gMap.cell_w = MACTILE_DIM;
        gMap.cell_h = MACTILE_DIM;
        if (map->its_window) {
            Rect b; short sw, sh;
            GetWindowPortBounds(map->its_window, &b);
            if (RetrieveSize(kMapTileWindow, b.top, b.left, &sh, &sw))
                SizeWindow(map->its_window, sw, sh, false);
        }
        layout_scroll_controls();
        /* derive the viewport dims from the actual window size */
        if (map->its_window) {
            Rect cr; map_content_bounds(&cr);
            gMap.vis_cols = (cr.right - cr.left) / gMap.cell_w;
            gMap.vis_rows = (cr.bottom - cr.top) / gMap.cell_h;
        }
        if (gMap.vis_cols < 1) gMap.vis_cols = 1;
        if (gMap.vis_rows < 1) gMap.vis_rows = 1;
        if (!allocate_backing()) {
            mac_dprintf("macmap: backing alloc failed in tile mode; using fallback\n");
        }
        /* 8bpp: anchor the sheet's dominant colors with a pmTolerant palette
           (tolerance 0x1000 of 0xFFFF, ~6%); colors beyond the first entries
           map to their nearest match in the default CLUT.  32 entries -- not
           256 -- so reserved system slots and other windows' colors survive. */
#define TILE_PALETTE_ENTRIES 32
#define TILE_PALETTE_TOLERANCE 0x1000
        if (mactile_sheet_depth() == 8) {
            if (!gMap.palette) {
                CTabHandle ct = mactile_sheet_ctable();
                if (ct)
                    gMap.palette = NewPalette(TILE_PALETTE_ENTRIES, ct,
                                              pmTolerant,
                                              TILE_PALETTE_TOLERANCE);
            }
            if (gMap.palette) {
                SetPalette(map->its_window, gMap.palette, true);
                ActivatePalette(map->its_window);
            }
        }
    } else {
        if (gMap.owner) {
            gMap.cell_w = gMap.owner->char_width;
            gMap.cell_h = gMap.owner->row_height;
            if (gMap.cell_w < 1) gMap.cell_w = 6;
            if (gMap.cell_h < 1) gMap.cell_h = 14;
        }
        if (map->its_window) {
            Rect b; short sw, sh;
            GetWindowPortBounds(map->its_window, &b);
            if (RetrieveSize(kMapWindow, b.top, b.left, &sh, &sw))
                SizeWindow(map->its_window, sw, sh, false);
        }
        layout_scroll_controls();
        if (map->its_window) {
            Rect cr; map_content_bounds(&cr);
            gMap.vis_cols = (cr.right - cr.left) / gMap.cell_w;
            gMap.vis_rows = (cr.bottom - cr.top) / gMap.cell_h;
        }
        if (gMap.vis_cols < 1) gMap.vis_cols = 1;
        if (gMap.vis_rows < 1) gMap.vis_rows = 1;
        /* dispose the palette; a re-enable rebuilds it */
        if (gMap.palette) {
            SetPalette(map->its_window, NULL, false);
            DisposePalette(gMap.palette);
            gMap.palette = NULL;
        }
        if (!allocate_backing()) {
            mac_dprintf("macmap: backing alloc failed in text mode; using fallback\n");
        }
    }
    /* repopulate the backing and invalidate; the update event does the blit
       (a synchronous blit here gets clipped — port not settled in menu context) */
    repaint_full_viewport();
    if (map->its_window) {
        Rect b; GetWindowPortBounds(map->its_window, &b);
        InvalWindowRect(map->its_window, &b);
    }
    return true;
}

Boolean
macmap_get_mode(NhWindow *map)
{
    return (map && gMap.owner == map) ? gMap.tile_mode : false;
}

static void
draw_cell_text(int col, int row, char ch, int color)
{
    if (!gMap.owner || !gMap.owner->its_window) return;
    if (col < 0 || col >= COLNO || row < 0 || row >= ROWNO) return;

    short dx = (col - gMap.scroll_col) * gMap.cell_w;
    short dy = (row - gMap.scroll_row) * gMap.cell_h;
    if (dx < 0 || dy < 0
        || dx >= gMap.vis_cols * gMap.cell_w
        || dy >= gMap.vis_rows * gMap.cell_h) {
        return;   /* off-viewport, cache only */
    }

    RGBColor black = {0, 0, 0};
    RGBColor white = {0xFFFF, 0xFFFF, 0xFFFF};

    if (gMap.backing) {
        /* paint into backing; macmap_flush blits the dirty region once */
        PixMapHandle pm = GetGWorldPixMap(gMap.backing);
        if (!LockPixels(pm)) {
            mac_dprintf("macmap: draw_cell_text: LockPixels failed\n");
            return;
        }
        GWorldPtr saveW; GDHandle saveD;
        GetGWorld(&saveW, &saveD);
        SetGWorld(gMap.backing, NULL);

        FontInfo fi; GetFontInfo(&fi);
        Rect cell = { dy, dx, dy + gMap.cell_h, dx + gMap.cell_w };
        RGBBackColor(&black);          /* NetHack's colors want a dark bg */
        EraseRect(&cell);
        set_nh_color(color);
        MoveTo(dx, dy + fi.ascent);    /* baseline = top + ascent (no top clip) */
        DrawChar(ch);
        RGBForeColor(&black);          /* restore fg=black/bg=white so a */
        RGBBackColor(&white);          /* later tile blit isn't tinted */

        SetGWorld(saveW, saveD);
        UnlockPixels(pm);
        mark_dirty(&cell);
    } else {
        /* fallback: direct to window */
        GrafPtr saveP; GetPort(&saveP);
        SetPort(gMap.owner->its_window);
        FontInfo fi; GetFontInfo(&fi);
        Rect cell = { dy, dx, dy + gMap.cell_h, dx + gMap.cell_w };
        RGBBackColor(&black);
        EraseRect(&cell);
        set_nh_color(color);
        MoveTo(dx, dy + fi.ascent);
        DrawChar(ch);
        RGBForeColor(&black);
        RGBBackColor(&white);
        SetPort(saveP);
    }
}

static void
draw_cell_tile(int col, int row, int tile_idx)
{
    if (!gMap.owner || !gMap.owner->its_window) return;
    if (col < 0 || col >= COLNO || row < 0 || row >= ROWNO) return;

    short dx = (col - gMap.scroll_col) * gMap.cell_w;
    short dy = (row - gMap.scroll_row) * gMap.cell_h;
    if (dx < 0 || dy < 0
        || dx >= gMap.vis_cols * gMap.cell_w
        || dy >= gMap.vis_rows * gMap.cell_h) {
        return;   /* off-viewport, cached only */
    }

    if (gMap.backing) {
        mactile_blit_to(gMap.backing, tile_idx, dx, dy);
        Rect cell = { dy, dx, dy + gMap.cell_h, dx + gMap.cell_w };
        mark_dirty(&cell);
    } else {
        mactile_blit_to_window(gMap.owner->its_window, tile_idx, dx, dy);
    }
}

/* Cell cursor for getpos/farlook. TILE mode: a bright inner ring via PmForeColor
   by INDEX (index-direct — avoids the Palette Manager recoloring the map) plus a
   black outer ring for light tiles. TEXT mode: a white frame (the map bg is black). */
static void
draw_cursor_border(int col, int row)
{
    if (!gMap.owner || !gMap.owner->its_window) return;
    if (col < 1 || col >= COLNO || row < 0 || row >= ROWNO) return;
    short dx = (col - gMap.scroll_col) * gMap.cell_w;
    short dy = (row - gMap.scroll_row) * gMap.cell_h;
    if (dx < 0 || dy < 0
        || dx >= gMap.vis_cols * gMap.cell_w
        || dy >= gMap.vis_rows * gMap.cell_h) return;
    Rect cell = { dy, dx, dy + gMap.cell_h, dx + gMap.cell_w };
    GrafPtr saveP; GetPort(&saveP);
    SetPortWindowPort(gMap.owner->its_window);
    PenState savePen; GetPenState(&savePen);
    PenSize(1, 1);
    PenMode(srcCopy);
    RGBColor black = {0, 0, 0};
    if (gMap.palette) {
        short cidx = mactile_cursor_clut_index();
        if (cidx >= 0) {
            Rect inner = cell;
            InsetRect(&inner, 1, 1);
            if (inner.right > inner.left && inner.bottom > inner.top) {
                PmForeColor(cidx);
                FrameRect(&inner);
            }
        }
        RGBForeColor(&black);
        FrameRect(&cell);
    } else {
        RGBColor white = {0xFFFF, 0xFFFF, 0xFFFF};
        RGBForeColor(&white);
        FrameRect(&cell);
        RGBForeColor(&black);
    }
    SetPenState(&savePen);
    SetPort(saveP);
}

/* Repaint one cell from cache (also erases the cursor border). */
static void
redraw_cell_from_cache(int col, int row)
{
    if (col < 1 || col >= COLNO || row < 0 || row >= ROWNO) return;
    if (gMap.tile_mode) {
        short idx = gMap.tile_cache[row][col];
        if (idx >= 0)                  /* -1 = no glyph yet; 0 is a real tile */
            draw_cell_tile(col, row, (int) idx);
    } else {
        char ch  = (char) gMap.text_cache[row][col];
        int  color = (int) gMap.text_color[row][col];
        if (ch == 0) ch = ' ';
        draw_cell_text(col, row, ch, color);
    }
}

/* Blit the accumulated dirty region to the window in one CopyBits, then draw
   the hero/cursor highlight on top (it's a window-only overlay, never in the
   backing, so the blit would otherwise erase it). Called at the per-frame flush
   boundary (display_nhwindow / curs); a no-op blit when nothing is dirty. */
void
macmap_flush(void)
{
    if (!gMap.owner || !gMap.owner->its_window) return;
    if (gMap.has_dirty && gMap.backing) {
        Rect r = gMap.dirty, b;
        SetRect(&b, 0, 0, gMap.vis_cols * gMap.cell_w, gMap.vis_rows * gMap.cell_h);
        if (SectRect(&r, &b, &r))
            blit_backing_to_window(&r, &r);
    }
    gMap.has_dirty = false;
    if (gMap.cursor_on)
        draw_cursor_border(gMap.cursor_x, gMap.cursor_y);
}

void
macmap_curs(NhWindow *map, int x, int y)
{
    if (!map || gMap.owner != map) return;
    /* repaint the old cursor cell from cache so the flush erases its border */
    if (gMap.cursor_on
        && (gMap.cursor_x != x || gMap.cursor_y != y)) {
        redraw_cell_from_cache(gMap.cursor_x, gMap.cursor_y);
    }
    gMap.cursor_x  = (short) x;
    gMap.cursor_y  = (short) y;
    gMap.cursor_on = true;
    /* blit any pending cells and draw the new border on top (immediate so
       interactive cursor moves in getpos/farlook show without a frame flush) */
    macmap_flush();
}

void
macmap_print_glyph(NhWindow *map, int x, int y,
                    const glyph_info *gi)
{
    if (!map || gMap.owner != map) return;
    if (!gi) return;
    if (x < 0 || x >= COLNO || y < 0 || y >= ROWNO) return;

    /* Auto-center the viewport on the hero. NetHack core only calls
       cliparound() between turns from moveloop; the very first frame
       (and the frame after a resize) doesn't get one, so without this
       the viewport stays at (0,0) and the visible area is unexplored
       stone tiles — looks like a black window. */
    if ((int) x == (int) u.ux && (int) y == (int) u.uy) {
        macmap_cliparound(map, x, y);
    }

    char ch = gi->ttychar;
    int  color = gi->gm.sym.color;
    int  idx = remap_tile_idx(gi->gm.tileidx);

    /* update both caches so a mode toggle can repaint */
    gMap.text_cache[y][x] = (unsigned char) ch;
    gMap.text_color[y][x] = (unsigned char) color;
    gMap.tile_cache[y][x] = (short) idx;

    if (gMap.tile_mode)
        draw_cell_tile(x, y, idx);
    else
        draw_cell_text(x, y, ch, color);
}

void
macmap_update_event(NhWindow *map)
{
    /* Called inside HandleUpdate's BeginUpdate/EndUpdate; don't call BeginUpdate
       here or the second call empties the visRgn and clips out every draw. */
    if (!map || gMap.owner != map || !map->its_window) return;

    gMap.cursor_on = false;   /* the repaint wipes the software cursor */

    GrafPtr saveP; GetPort(&saveP);
    SetPort(map->its_window);

    if (gMap.backing) {
        Rect bbox; bbox = ((CGrafPtr) gMap.backing)->portRect;
        Rect dst = bbox;
        blit_backing_to_window(&bbox, &dst);
    } else {
        /* fallback: cache-replay redraw */
        Rect content; map_content_bounds(&content);
        EraseRect(&content);
        int r, c;
        for (r = gMap.scroll_row; r < gMap.scroll_row + gMap.vis_rows && r < ROWNO; ++r)
            for (c = gMap.scroll_col; c < gMap.scroll_col + gMap.vis_cols && c < COLNO; ++c) {
                if (gMap.tile_mode) {
                    short idx = gMap.tile_cache[r][c];
                    if (idx >= 0) draw_cell_tile(c, r, (int) idx);
                } else {
                    char ch  = (char) gMap.text_cache[r][c];
                    int  col = (int)  gMap.text_color[r][c];
                    if (ch != 0) draw_cell_text(c, r, ch, col);
                }
            }
    }

    gMap.has_dirty = false;   /* the full-backing blit subsumes any pending dirty */

    /* draw the scrollbars and grow box on top of the blit */
    if (gMap.decorated) {
        update_scroll_controls();
        DrawControls(map->its_window);
        DrawGrowIcon(map->its_window);
    }

    SetPort(saveP);
}

void
macmap_clear(NhWindow *map)
{
    if (!map || gMap.owner != map) return;
    int r, c;
    for (r = 0; r < ROWNO; ++r)
        for (c = 0; c < COLNO; ++c) {
            gMap.text_cache[r][c] = ' ';
            gMap.text_color[r][c] = 8; /* NO_COLOR */
            gMap.tile_cache[r][c] = -1;  /* -1 = no glyph (tile 0 is the ant) */
        }
    /* reset scroll so the next hero print_glyph recenters (col 0 unused) */
    gMap.scroll_col = 1;
    gMap.scroll_row = 0;
    gMap.cursor_on  = false;
    gMap.has_dirty  = false;
    if (map->its_window) {
        GrafPtr saveP; GetPort(&saveP);
        SetPort(map->its_window);
        Rect content; map_content_bounds(&content);
        EraseRect(&content);
        SetPort(saveP);
    }
    /* clear the backing too, or the next damage event blits stale pixels */
    if (gMap.backing) {
        PixMapHandle pm = GetGWorldPixMap(gMap.backing);
        if (LockPixels(pm)) {
            GWorldPtr saveW; GDHandle saveD;
            GetGWorld(&saveW, &saveD);
            SetGWorld(gMap.backing, NULL);
            Rect bb; bb = ((CGrafPtr) gMap.backing)->portRect;
            EraseRect(&bb);
            SetGWorld(saveW, saveD);
            UnlockPixels(pm);
        } else {
            mac_dprintf("macmap: macmap_clear: LockPixels failed\n");
        }
    }
}

#define MT_EDGE_MARGIN 3

static void
recompute_scroll_for_center(int x, int y, short *new_col, short *new_row)
{
    short c = (short) x - gMap.vis_cols / 2;
    short r = (short) y - gMap.vis_rows / 2;
    /* clamp so col 0 never enters the viewport (cols [1,COLNO-1], rows [0,ROWNO)) */
    if (c < 1) c = 1;
    if (r < 0) r = 0;
    if (c + gMap.vis_cols > COLNO) c = COLNO - gMap.vis_cols;
    if (r + gMap.vis_rows > ROWNO) r = ROWNO - gMap.vis_rows;
    if (c < 1) c = 1;
    if (r < 0) r = 0;
    *new_col = c;
    *new_row = r;
}

static void
repaint_full_viewport(void)
{
    if (!gMap.owner) return;
    gMap.cursor_on = false;   /* full repaint wipes the inverted-cell cursor */
    if (gMap.backing) {
        PixMapHandle pm = GetGWorldPixMap(gMap.backing);
        if (LockPixels(pm)) {
            Rect bbox; bbox = ((CGrafPtr) gMap.backing)->portRect;
            GWorldPtr saveW; GDHandle saveD;
            GetGWorld(&saveW, &saveD);
            SetGWorld(gMap.backing, NULL);
            EraseRect(&bbox);
            SetGWorld(saveW, saveD);
            UnlockPixels(pm);
        }
    }
    int r, c;
    /* skip col 0 (unused); its stale cache would paint as tile 0, a real glyph */
    int c_first = gMap.scroll_col < 1 ? 1 : gMap.scroll_col;
    int c_last  = gMap.scroll_col + gMap.vis_cols;
    if (c_last > COLNO) c_last = COLNO;
    for (r = gMap.scroll_row; r < gMap.scroll_row + gMap.vis_rows && r < ROWNO; ++r)
        for (c = c_first; c < c_last; ++c)
            redraw_cell_from_cache(c, r);
    /* Mark the whole viewport dirty (incl. erased empty cells); macmap_flush
       blits it. Callers not followed by a core flush call macmap_flush themselves. */
    if (gMap.backing)
        mark_dirty_all();
}

static void
backing_self_scroll(int dx_cells, int dy_cells)
{
    if (!gMap.backing) return;
    PixMapHandle pm = GetGWorldPixMap(gMap.backing);
    if (!LockPixels(pm)) {
        /* pixels purged — skip; CopyBits here reads and writes the backing */
        mac_dprintf("macmap: backing_self_scroll: LockPixels failed\n");
        return;
    }
    GWorldPtr saveW; GDHandle saveD;
    GetGWorld(&saveW, &saveD);
    SetGWorld(gMap.backing, NULL);

    Rect bbox; bbox = ((CGrafPtr) gMap.backing)->portRect;
    Rect src = bbox, dst = bbox;
    OffsetRect(&dst, (short)(-dx_cells * gMap.cell_w), (short)(-dy_cells * gMap.cell_h));
    CopyBits((BitMap *) *pm, (BitMap *) *pm, &src, &dst, srcCopy, NULL);

    SetGWorld(saveW, saveD);
    UnlockPixels(pm);
}

static void
repaint_strip(int col_start, int row_start, int col_end, int row_end)
{
    int r, c;
    if (col_start < 0) col_start = 0;
    if (row_start < 0) row_start = 0;
    if (col_end > COLNO) col_end = COLNO;
    if (row_end > ROWNO) row_end = ROWNO;
    if (col_start < 1) col_start = 1;   /* col 0 unused */
    for (r = row_start; r < row_end; ++r)
        for (c = col_start; c < col_end; ++c)
            redraw_cell_from_cache(c, r);
}

/* Move the viewport to (new_col,new_row), clamped, repainting via soft-scroll
   (small move) or full redraw (big jump / no backing). Updates the thumbs.
   Shared by macmap_cliparound and the scrollbar handlers. */
static void
scroll_viewport_to(short new_col, short new_row)
{
    short old_col = gMap.scroll_col, old_row = gMap.scroll_row;
    short dx, dy;

    /* clamp so col 0 never enters the viewport and the last row/col isn't passed */
    if (new_col + gMap.vis_cols > COLNO) new_col = COLNO - gMap.vis_cols;
    if (new_row + gMap.vis_rows > ROWNO) new_row = ROWNO - gMap.vis_rows;
    if (new_col < 1) new_col = 1;
    if (new_row < 0) new_row = 0;
    if (new_col == old_col && new_row == old_row) return;

    dx = new_col - old_col;
    dy = new_row - old_row;

    gMap.cursor_on = false;   /* the repaint wipes the software cursor */

    gMap.scroll_col = new_col;
    gMap.scroll_row = new_row;
    update_scroll_controls();

    /* big jump or no backing: full redraw */
    if (!gMap.backing
        || abs(dx) > gMap.vis_cols / 2 || abs(dy) > gMap.vis_rows / 2) {
        repaint_full_viewport();
        return;
    }

    /* soft-scroll: shift backing pixels, re-render the exposed strip */
    backing_self_scroll(dx, dy);

    if (dx > 0)
        repaint_strip(new_col + gMap.vis_cols - dx, new_row,
                      new_col + gMap.vis_cols, new_row + gMap.vis_rows);
    else if (dx < 0)
        repaint_strip(new_col, new_row,
                      old_col, new_row + gMap.vis_rows);

    if (dy > 0)
        repaint_strip(new_col, new_row + gMap.vis_rows - dy,
                      new_col + gMap.vis_cols, new_row + gMap.vis_rows);
    else if (dy < 0)
        repaint_strip(new_col, new_row,
                      new_col + gMap.vis_cols, old_row);

    /* the self-scroll shifted every pixel, so the whole viewport is dirty */
    mark_dirty_all();
}

void
macmap_cliparound(NhWindow *map, int x, int y)
{
    if (!map || gMap.owner != map)
        return;

    /* don't scroll while the hero stays inside the edge margin */
    short hero_in_view_x = (short) x - gMap.scroll_col;
    short hero_in_view_y = (short) y - gMap.scroll_row;
    if (hero_in_view_x >= MT_EDGE_MARGIN
        && hero_in_view_x <  gMap.vis_cols - MT_EDGE_MARGIN
        && hero_in_view_y >= MT_EDGE_MARGIN
        && hero_in_view_y <  gMap.vis_rows - MT_EDGE_MARGIN) {
        return;
    }

    short new_col, new_row;
    recompute_scroll_for_center(x, y, &new_col, &new_row);
    scroll_viewport_to(new_col, new_row);
}

void
macmap_grow_event(NhWindow *map, long newSize)
{
    if (!map || gMap.owner != map || !map->its_window) return;
    SizeWindow(map->its_window, (short)(newSize & 0xffff), (short)(newSize >> 16), true);
    layout_scroll_controls();
    Rect full; GetWindowPortBounds(map->its_window, &full);
    Rect cr; map_content_bounds(&cr);
    gMap.vis_cols = (cr.right - cr.left) / gMap.cell_w;
    gMap.vis_rows = (cr.bottom - cr.top) / gMap.cell_h;
    if (gMap.vis_cols < 1) gMap.vis_cols = 1;
    if (gMap.vis_rows < 1) gMap.vis_rows = 1;
    /* persist per display mode in the prefs file (text and tile sizes
       are independent) */
    SaveSizeForKind(gMap.tile_mode ? kMapTileWindow : kMapWindow,
                    (short)(full.bottom - full.top),
                    (short)(full.right - full.left));
    if (!allocate_backing()) {
        mac_dprintf("macmap: backing realloc failed on grow\n");
    }
    /* recenter on the hero, then repaint from cache; allocate_backing
       just wiped the backing, so the repaint must be full and
       unconditional (cliparound/scroll_viewport_to skip an unchanged
       scroll) */
    if (u.ux > 0 || u.uy > 0) {
        recompute_scroll_for_center((int) u.ux, (int) u.uy,
                                    &gMap.scroll_col, &gMap.scroll_row);
    }
    repaint_full_viewport();
    update_scroll_controls();
    macmap_flush();   /* a grow isn't followed by a core frame flush */
}

/* Size the map window to fit as much map as fits in avail_w x avail_h, snapped
   to whole cells and never larger than the full map. Placement stays with
   SanePositions(). */
void
macmap_fit(short avail_w, short avail_h)
{
    long full_w, full_h;
    short w, h, cols, rows;
    short map_cols = COLNO - 1;   /* col 0 unused */
    if (!gMap.owner || !gMap.owner->its_window) return;
    if (gMap.cell_w < 1 || gMap.cell_h < 1) return;
    if (avail_w < gMap.cell_w + gMap.inset_r) avail_w = gMap.cell_w + gMap.inset_r;
    if (avail_h < gMap.cell_h + gMap.inset_b) avail_h = gMap.cell_h + gMap.inset_b;
    full_w = (long) map_cols * gMap.cell_w + gMap.inset_r;
    full_h = (long) ROWNO * gMap.cell_h + gMap.inset_b;
    w = (full_w < (long) avail_w) ? (short) full_w : avail_w;
    h = (full_h < (long) avail_h) ? (short) full_h : avail_h;
    cols = (short) ((w - gMap.inset_r) / gMap.cell_w);
    rows = (short) ((h - gMap.inset_b) / gMap.cell_h);
    if (cols < 1) cols = 1;
    if (cols > map_cols) cols = map_cols;
    if (rows < 1) rows = 1;
    if (rows > ROWNO) rows = ROWNO;
    w = (short) (cols * gMap.cell_w + gMap.inset_r);
    h = (short) (rows * gMap.cell_h + gMap.inset_b);
    macmap_grow_event(gMap.owner, ((long) h << 16) | ((long) w & 0xffffL));
}

/* Set the viewport from the scrollbar values (vscroll=row, hscroll=col; scroll_col
   is 1-based so +1). */
static void
apply_scroll_from_controls(void)
{
    short new_row, new_col;
    if (!gMap.vscroll || !gMap.hscroll) return;
    new_row = GetControlValue(gMap.vscroll);
    new_col = GetControlValue(gMap.hscroll) + 1;
    scroll_viewport_to(new_col, new_row);
    macmap_flush();   /* live scrollbar feedback; not followed by a core flush */
}

/* TrackControl action proc: 1 cell per arrow, one page-minus-one per page click. */
static pascal void
macmap_scroll_action(ControlHandle ctl, short part)
{
    short now, max, page, amt, val;
    Boolean vert;
    if (!part || !ctl) return;
    vert = (ctl == gMap.vscroll);
    now  = GetControlValue(ctl);
    max  = GetControlMaximum(ctl);
    page = vert ? gMap.vis_rows : gMap.vis_cols;
    if (page > 1) page -= 1;   /* keep a row/col of context across a page jump */
    switch (part) {
    case kControlUpButtonPart:   amt = -1;     break;
    case kControlDownButtonPart: amt =  1;     break;
    case kControlPageUpPart:     amt = -page;  break;
    case kControlPageDownPart:   amt =  page;  break;
    default: return;
    }
    val = now + amt;
    if (val < 0)   val = 0;
    if (val > max) val = max;
    if (val == now) return;
    SetControlValue(ctl, val);
    apply_scroll_from_controls();
}

/* Handle a click on the map chrome: track the scrollbars and swallow strip/grow
   clicks. Returns true if handled, so click-to-move is suppressed; false for the
   real map area. Called from BaseClick. */
Boolean
macmap_click(NhWindow *map, Point pt, UInt32 mod UNUSED)
{
    if (!gMap.decorated || !map || !map->its_window)
        return false;
    {
        ControlHandle c; short part;
        part = FindControl(pt, map->its_window, &c);
        if (part && (c == gMap.vscroll || c == gMap.hscroll)) {
            if (GetControlMaximum(c) > 0) {   /* only if there's a hidden range */
                if (part == kControlIndicatorPart) {
                    /* thumb: apply the landing value on release */
                    if (TrackControl(c, pt, NULL) == kControlIndicatorPart)
                        apply_scroll_from_controls();
                } else {
                    /* arrows / page gutters: scroll live via the action proc */
                    if (!gMapScrollUPP)
                        gMapScrollUPP = NewControlActionUPP(macmap_scroll_action);
                    (void) TrackControl(c, pt, gMapScrollUPP);
                }
            }
            return true;
        }
    }
    {   /* the reserved strips (incl. the grow-box corner) */
        Rect b;
        GetWindowPortBounds(map->its_window, &b);
        if (pt.h >= b.right - gMap.inset_r || pt.v >= b.bottom - gMap.inset_b)
            return true;
    }
    return false;
}

void
macmap_pixel_to_cell(NhWindow *map, Point pt, int *col, int *row)
{
    if (col) *col = (pt.h / gMap.cell_w) + gMap.scroll_col;
    if (row) *row = (pt.v / gMap.cell_h) + gMap.scroll_row;
    (void) map;
}

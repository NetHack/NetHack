/*   Copyright (c) NetHack PC Development Team 1995                 */
/*   VESA BIOS functions copyright (c) Ray Chason 2016              */
/*   NetHack may be freely redistributed.  See license for details. */
/*
 * vidvesa.c - VGA Hardware video support with VESA BIOS Extensions
 */

#include "hack.h"

#ifdef SCREEN_VESA /* this file is for SCREEN_VESA only    */
#include <dpmi.h>

#include "pcvideo.h"
#include "tile.h"
#include "pctiles.h"
#include "vesa.h"
#include "wintty.h"
#include "tileset.h"

#define BACKGROUND_VESA_COLOR 1
#define FIRST_TEXT_COLOR 240

static unsigned long FDECL(vesa_SetWindow, (int window, unsigned long offset));
static unsigned long FDECL(vesa_ReadPixel32, (unsigned x, unsigned y));
static void FDECL(vesa_WritePixel32, (unsigned x, unsigned y,
        unsigned long color));
static void FDECL(vesa_WritePixel, (unsigned x, unsigned y, unsigned color));
static unsigned long FDECL(vesa_MakeColor, (unsigned r, unsigned g, unsigned b));
static void FDECL(vesa_GetRGB, (
        unsigned long color,
        unsigned char *rp, unsigned char *gp, unsigned char *bp));
static void FDECL(vesa_FillRect, (
        unsigned left, unsigned top,
        unsigned width, unsigned height,
        unsigned color));

static void NDECL(vesa_redrawmap);
static void FDECL(vesa_cliparound, (int, int));
static void FDECL(decal_packed, (const struct TileImage *tile, unsigned special));
static void FDECL(vesa_SwitchMode, (unsigned mode));
static boolean FDECL(vesa_SetPalette, (const struct Pixel *));
static boolean FDECL(vesa_SetHardPalette, (const struct Pixel *));
static boolean FDECL(vesa_SetSoftPalette, (const struct Pixel *));
static void FDECL(vesa_DisplayCell, (const struct TileImage *tile, int, int));
static void FDECL(vesa_DisplayCellInMemory, (const struct TileImage *tile,
        int, char buf[TILE_Y][640*2]));
static unsigned FDECL(vesa_FindMode, (unsigned long mode_addr, unsigned bits));
static void FDECL(vesa_WriteChar, (int, int, int, int, BOOLEAN_P));
static void FDECL(vesa_WriteCharInMemory, (int, int, char buf[TILE_Y][640*2],
        int));
static void FDECL(vesa_WriteStr, (const char *, int, int, int, int));
static char __far *NDECL(vesa_FontPtrs);

#ifdef POSITIONBAR
static void NDECL(positionbar);
#endif

extern int clipx, clipxmax; /* current clipping column from wintty.c */
extern int curcol, currow;  /* current column and row        */
extern int g_attribute;
extern int attrib_text_normal;  /* text mode normal attribute */
extern int attrib_gr_normal;    /* graphics mode normal attribute */
extern int attrib_gr_intense;   /* graphics mode intense attribute */
extern boolean inmap;           /* in the map window */
extern boolean restoring;

/*
 * Global Variables
 */

static unsigned char __far *font;

static struct map_struct {
    int glyph;
    int ch;
    int attr;
    unsigned special;
} map[ROWNO][COLNO]; /* track the glyphs */

#define vesa_clearmap()                                   \
    {                                                     \
        int x, y;                                         \
        for (y = 0; y < ROWNO; ++y)                       \
            for (x = 0; x < COLNO; ++x) {                 \
                map[y][x].glyph = cmap_to_glyph(S_stone); \
                map[y][x].ch = S_stone;                   \
                map[y][x].attr = 0;                       \
                map[y][x].special = 0;                    \
            }                                             \
    }
#define TOP_MAP_ROW 1

static int viewport_size = 40;

static const struct Pixel defpalette[] = {    /* Colors for text and the position bar */
	{ 0x18, 0x18, 0x18, 0xff }, /* CLR_BLACK */
	{ 0xaa, 0x00, 0x00, 0xff }, /* CLR_RED */
	{ 0x00, 0xaa, 0x00, 0xff }, /* CLR_GREEN */
	{ 0x99, 0x40, 0x00, 0xff }, /* CLR_BROWN */
	{ 0x00, 0x00, 0xaa, 0xff }, /* CLR_BLUE */
	{ 0xaa, 0x00, 0xaa, 0xff }, /* CLR_MAGENTA */
	{ 0x00, 0xaa, 0xaa, 0xff }, /* CLR_CYAN */
	{ 0xaa, 0xaa, 0xaa, 0xff }, /* CLR_GRAY */
	{ 0x55, 0x55, 0x55, 0xff }, /* NO_COLOR */
	{ 0xff, 0x90, 0x00, 0xff }, /* CLR_ORANGE */
	{ 0x00, 0xff, 0x00, 0xff }, /* CLR_BRIGHT_GREEN */
	{ 0xff, 0xff, 0x00, 0xff }, /* CLR_YELLOW */
	{ 0x00, 0x00, 0xff, 0xff }, /* CLR_BRIGHT_BLUE */
	{ 0xff, 0x00, 0xff, 0xff }, /* CLR_BRIGHT_MAGENTA */
	{ 0x00, 0xff, 0xff, 0xff }, /* CLR_BRIGHT_CYAN */
	{ 0xff, 0xff, 0xff, 0xff }  /* CLR_WHITE */
};

/* Information about the selected VESA mode */
static unsigned short vesa_mode = 0xFFFF; /* Mode number */
static unsigned short vesa_x_res; /* X resolution */
static unsigned short vesa_y_res; /* Y resolution */
static unsigned short vesa_x_center; /* X centering offset */
static unsigned short vesa_y_center; /* Y centering offset */
static unsigned short vesa_scan_line; /* Bytes per scan line */
static int vesa_read_win;  /* Select the read window */
static int vesa_write_win; /* Select the write window */
static unsigned long vesa_win_pos[2]; /* Window position */
static unsigned long vesa_win_addr[2]; /* Window physical address */
static unsigned long vesa_win_size; /* Window size */
static unsigned long vesa_win_gran; /* Window granularity */
static unsigned char vesa_pixel_size;
static unsigned char vesa_pixel_bytes;
static unsigned char vesa_red_pos;
static unsigned char vesa_red_size;
static unsigned char vesa_green_pos;
static unsigned char vesa_green_size;
static unsigned char vesa_blue_pos;
static unsigned char vesa_blue_size;
static unsigned long vesa_palette[256];

struct OldModeInfo {
    unsigned mode;

    unsigned short XResolution;    /* horizontal resolution in pixels or characters */
    unsigned short YResolution;    /* vertical resolution in pixels or characters */
    unsigned char  BitsPerPixel;   /* bits per pixel */
    unsigned char  MemoryModel;    /* memory model type */
};

static const struct OldModeInfo old_mode_table[] = {
    { 0x0101,  640,  480,  8, 4 },
    { 0x0103,  800,  600,  8, 4 },
    { 0x0105, 1024,  768,  8, 4 },
    { 0x0107, 1280, 1024,  8, 4 },
    { 0x0110,  640,  480, 15, 6 },
    { 0x0111,  640,  480, 16, 6 },
    { 0x0112,  640,  480, 24, 6 },
    { 0x0113,  800,  600, 15, 6 },
    { 0x0114,  800,  600, 16, 6 },
    { 0x0115,  800,  600, 24, 6 },
    { 0x0116, 1024,  768, 15, 6 },
    { 0x0117, 1024,  768, 16, 6 },
    { 0x0118, 1024,  768, 24, 6 },
    { 0x0119, 1280, 1024, 15, 6 },
    { 0x011A, 1280, 1024, 16, 6 },
    { 0x011B, 1280, 1024, 24, 6 },
};

/* Retrieve the mode info block */
static boolean
vesa_GetModeInfo(mode, info)
unsigned mode;
struct ModeInfoBlock *info;
{
    int mode_info_sel = -1; /* custodial */
    int mode_info_seg;
    __dpmi_regs regs;

    mode_info_seg = __dpmi_allocate_dos_memory(
            (sizeof(*info) + 15) / 16,
            &mode_info_sel);
    if (mode_info_seg < 0) goto error;

    memset(info, 0, sizeof(*info));
    dosmemput(info, sizeof(*info), mode_info_seg * 16L);

    memset(&regs, 0, sizeof(regs));
    regs.x.ax = 0x4F01;
    regs.x.cx = mode;
    regs.x.di = 0;
    regs.x.es = mode_info_seg;
    (void) __dpmi_int(VIDEO_BIOS, &regs);

    if (regs.x.ax != 0x004F) goto error;
    dosmemget(mode_info_seg * 16L, sizeof(*info), info);
    if (!(info->ModeAttributes & 0x0001)) goto error;

    if (!(info->ModeAttributes & 0x0002)) {
        /* Older VESA BIOS that did not return certain mode properties, but
           that has fixed mode numbers; search the table to find the right
           mode properties */

        unsigned i;

        for (i = 0; i < SIZE(old_mode_table); ++i) {
            if (mode == old_mode_table[i].mode) {
                break;
            }
        }
        if (i >= SIZE(old_mode_table)) goto error;

        info->XResolution = old_mode_table[i].XResolution;
        info->YResolution = old_mode_table[i].YResolution;
        info->NumberOfPlanes = 1;
        info->BitsPerPixel = old_mode_table[i].BitsPerPixel;
        info->NumberOfBanks = 1;
        info->MemoryModel = old_mode_table[i].MemoryModel;
    }

    __dpmi_free_dos_memory(mode_info_sel);
    return TRUE;

error:
    if (mode_info_sel != -1) __dpmi_free_dos_memory(mode_info_sel);
    return FALSE;
}

/* Set the memory window and return the offset */
static unsigned long
vesa_SetWindow(window, offset)
int window;
unsigned long offset;
{
    /* If the desired offset is already within the window, leave the window
       as it is and return the address based on the current window position.
       This minimizes the use of the window switch function.

       On the first call to the function, vesa_win_pos[window] == 0xFFFFFFFF,
       the offset will always be less than this, and the BIOS will always be
       called. */

    unsigned long pos = vesa_win_pos[window];
    if (offset < pos || pos + vesa_win_size <= offset) {
        __dpmi_regs regs;

        memset(&regs, 0, sizeof(regs));
        regs.x.ax = 0x4F05;
        regs.h.bh = 0x00;
        regs.h.bl = window;
        regs.x.dx = offset / vesa_win_gran;
        pos = regs.x.dx * vesa_win_gran;
        (void) __dpmi_int(VIDEO_BIOS, &regs);
        vesa_win_pos[window] = pos;
    }

    offset = offset - vesa_win_pos[window] + vesa_win_addr[window];
    /* Keep from crashing the system if some malfunction gives us a bad
       offset */
    if (offset < 0xA0000 || offset > 0xBFFFF) {
        vesa_SwitchMode(MODETEXT);
        fprintf(stderr, "Abort: offset=%08lX\n", offset);
        exit(1);
    }
    return offset;
}

static unsigned long
vesa_ReadPixel32(x, y)
unsigned x, y;
{
    unsigned long offset = y * vesa_scan_line + x * vesa_pixel_bytes;
    unsigned long addr, color;
    unsigned i;

    switch (vesa_pixel_size) {
    case 8:
        addr = vesa_SetWindow(vesa_read_win, offset);
        color = _farpeekb(_dos_ds, addr);
        break;

    case 15:
    case 16:
        addr = vesa_SetWindow(vesa_read_win, offset);
        color = _farpeekw(_dos_ds, addr);
        break;

    case 24:
        /* Pixel may cross a window boundary */
        color = 0;
        for (i = 0; i < 3; ++i) {
            addr = vesa_SetWindow(vesa_read_win, offset + i);
            color |= (unsigned long) _farpeekb(_dos_ds, addr) << (i * 8);
        }
        break;

    case 32:
        addr = vesa_SetWindow(vesa_read_win, offset);
        color = _farpeekl(_dos_ds, addr);
        break;
    }
    return color;
}

static void
vesa_WritePixel32(x, y, color)
unsigned x, y;
unsigned long color;
{
    unsigned long offset = y * vesa_scan_line + x * vesa_pixel_bytes;
    unsigned long addr;
    unsigned i;

    switch (vesa_pixel_size) {
    case 8:
        addr = vesa_SetWindow(vesa_write_win, offset);
        _farpokeb(_dos_ds, addr, color);
        break;

    case 15:
    case 16:
        addr = vesa_SetWindow(vesa_write_win, offset);
        _farpokew(_dos_ds, addr, color);
        break;

    case 24:
        /* Pixel may cross a window boundary */
        for (i = 0; i < 3; ++i) {
            addr = vesa_SetWindow(vesa_read_win, offset + i);
            _farpokeb(_dos_ds, addr, (unsigned char) (color >> (i * 8)));
        }
        break;

    case 32:
        addr = vesa_SetWindow(vesa_write_win, offset);
        _farpokel(_dos_ds, addr, color);
        break;
    }
}

static void
vesa_WritePixel(x, y, color)
unsigned x, y;
unsigned color;
{
    if (vesa_pixel_size == 8) {
        vesa_WritePixel32(x, y, color);
    } else {
        vesa_WritePixel32(x, y, vesa_palette[color & 0xFF]);
    }
}

static unsigned long
vesa_MakeColor(r, g, b)
unsigned r, g, b;
{
    r = (r & 0xFF) >> (8 - vesa_red_size);
    g = (g & 0xFF) >> (8 - vesa_green_size);
    b = (b & 0xFF) >> (8 - vesa_blue_size);
    return ((unsigned long) r << vesa_red_pos)
         | ((unsigned long) g << vesa_green_pos)
         | ((unsigned long) b << vesa_blue_pos);
}

static void
vesa_GetRGB(color, rp, gp, bp)
unsigned long color;
unsigned char *rp, *gp, *bp;
{
    unsigned r, g, b;

    r = color >> vesa_red_pos;
    g = color >> vesa_green_pos;
    b = color >> vesa_blue_pos;
    r <<= 8 - vesa_red_size;
    g <<= 8 - vesa_green_size;
    b <<= 8 - vesa_blue_size;
    *rp = (unsigned char) r;
    *gp = (unsigned char) g;
    *bp = (unsigned char) b;
}

static void
vesa_FillRect(left, top, width, height, color)
unsigned left, top, width, height, color;
{
    unsigned x, y;

    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            vesa_WritePixel(left + x, top + y, color);
        }
    }
}

void
vesa_get_scr_size()
{
    CO = 80;
    LI = 29;
}

void
vesa_backsp()
{
    int col, row;

    col = curcol; /* Character cell row and column */
    row = currow;

    if (col > 0)
        col = col - 1;
    vesa_gotoloc(col, row);
}

void
vesa_clear_screen(colour)
int colour;
{
    vesa_FillRect(0, 0, vesa_x_res, vesa_y_res, colour);
    if (iflags.tile_view)
        vesa_clearmap();
    vesa_gotoloc(0, 0); /* is this needed? */
}

/* clear to end of line */
void
vesa_cl_end(col, row)
int col, row;
{
    unsigned left = vesa_x_center + col * 8;
    unsigned top  = vesa_y_center + row * 16;
    unsigned width = (CO - 1 - col) * 8;
    unsigned height = 16;

    vesa_FillRect(left, top, width, height, BACKGROUND_VESA_COLOR);
}

/* clear to end of screen */
void
vesa_cl_eos(cy)
int cy;
{
    int count;

    cl_end();
    if (cy < LI - 1) {
        unsigned left = vesa_x_center;
        unsigned top  = vesa_y_center + cy * 16;
        unsigned width = 640;
        unsigned height = (LI - 1 - cy) * 16;

        vesa_FillRect(left, top, width, height, BACKGROUND_VESA_COLOR);
    }
}

void
vesa_tty_end_screen()
{
    vesa_clear_screen(BACKGROUND_VESA_COLOR);
    vesa_SwitchMode(MODETEXT);
}

void
vesa_tty_startup(wid, hgt)
int *wid, *hgt;
{
    /* code to sense display adapter is required here - MJA */

    vesa_get_scr_size();
    if (CO && LI) {
        *wid = CO;
        *hgt = LI;
    }

    attrib_gr_normal = ATTRIB_VGA_NORMAL;
    attrib_gr_intense = ATTRIB_VGA_INTENSE;
    g_attribute = attrib_gr_normal; /* Give it a starting value */
}

/*
 * Screen output routines (these are heavily used).
 *
 * These are the 3 routines used to place information on the screen
 * in the VGA PC tty port of NetHack.  These are the routines
 * that get called by the general interface routines in video.c.
 *
 * vesa_xputs -Writes a c null terminated string at the current location.
 *
 * vesa_xputc -Writes a single character at the current location. Since
 *             various places in the code assume that control characters
 *             can be used to control, we are forced to interpret some of
 *             the more common ones, in order to keep things looking correct.
 *
 * vesa_xputg -This routine is used to display a graphical representation of a
 *             NetHack glyph (a tile) at the current location.  For more
 *             information on NetHack glyphs refer to the comments in
 *             include/display.h.
 *
 */

void
vesa_xputs(s, col, row)
const char *s;
int col, row;
{
    if (s != NULL) {
        vesa_WriteStr(s, strlen(s), col, row, g_attribute);
    }
}

/* write out character (and attribute) */
void
vesa_xputc(ch, attr)
char ch;
int attr;
{
    int col, row;

    col = curcol;
    row = currow;

    switch (ch) {
    case '\n':
        col = 0;
        ++row;
        break;
    default:
        vesa_WriteChar((unsigned char) ch, col, row, attr, FALSE);
        if (col < (CO - 1))
            ++col;
        break;
    } /* end switch */
    vesa_gotoloc(col, row);
}

#if defined(USE_TILES)
/* Place tile represent. a glyph at current location */
void
vesa_xputg(glyphnum, ch,
          special)
int glyphnum;
int ch;
unsigned special; /* special feature: corpse, invis, detected, pet, ridden -
                     hack.h */
{
    int col, row;
    int attr;
    int ry;
    const struct TileImage *packcell;

    row = currow;
    col = curcol;
    if ((col < 0 || col >= COLNO)
        || (row < TOP_MAP_ROW || row >= (ROWNO + TOP_MAP_ROW)))
        return;
    ry = row - TOP_MAP_ROW;
    map[ry][col].glyph = glyphnum;
    map[ry][col].ch = ch;
    map[ry][col].special = special;
    attr = (g_attribute == 0) ? attrib_gr_normal : g_attribute;
    map[ry][col].attr = attr;
    if (iflags.traditional_view) {
        vesa_WriteChar((unsigned char) ch, col, row, attr, FALSE);
    } else {
        if ((col >= clipx) && (col <= clipxmax)) {
            packcell = get_tile(glyph2tile[glyphnum]);
            if (!iflags.over_view && map[ry][col].special)
                decal_packed(packcell, special);
            vesa_DisplayCell(packcell, col - clipx, row);
        }
    }
    if (col < (CO - 1))
        ++col;
    vesa_gotoloc(col, row);
}
#endif /* USE_TILES */

/*
 * Cursor location manipulation, and location information fetching
 * routines.
 * These include:
 *
 * vesa_gotoloc(x,y)    - Moves the "cursor" on screen to the specified x
 *			 and y character cell location.  This routine
 *                       determines the location where screen writes
 *                       will occur next, it does not change the location
 *                       of the player on the NetHack level.
 */

void
vesa_gotoloc(col, row)
int col, row;
{
    curcol = min(col, CO - 1); /* protection from callers */
    currow = min(row, LI - 1);
}

#if defined(USE_TILES) && defined(CLIPPING)
static void
vesa_cliparound(x, y)
int x, y;
{
    int oldx = clipx;

    if (!iflags.tile_view || iflags.over_view || iflags.traditional_view)
        return;

    if (x < clipx + 5) {
        clipx = max(0, x - (viewport_size / 2));
        clipxmax = clipx + (viewport_size - 1);
    } else if (x > clipxmax - 5) {
        clipxmax = min(COLNO - 1, x + (viewport_size / 2));
        clipx = clipxmax - (viewport_size - 1);
    }
    if (clipx != oldx) {
        if (on_level(&u.uz0, &u.uz) && !restoring)
            /* (void) doredraw(); */
            vesa_redrawmap();
    }
}

static void
vesa_redrawmap()
{
    int x, y, t;
    const struct TileImage *packcell;

    /* y here is in screen rows*/
    /* Build each row in local memory, then write, to minimize use of the
       window switch function */
    for (y = 0; y < ROWNO; ++y) {
        char buf[TILE_Y][640*2];

        for (x = clipx; x <= clipxmax; ++x) {
            if (iflags.traditional_view) {
                vesa_WriteCharInMemory((unsigned char) map[y][x].ch, x,
                              buf, map[y][x].attr);
            } else {
                t = map[y][x].glyph;
                packcell = get_tile(glyph2tile[t]);
                if (!iflags.over_view && map[y][x].special)
                    decal_packed(packcell, map[y][x].special);
                vesa_DisplayCellInMemory(packcell, x - clipx, buf);
            }
        }
        if (iflags.over_view && vesa_pixel_size != 8) {
            for (t = 0; t < TILE_Y; ++t) {
                for (x = 0; x < 640; ++x) {
                    unsigned long c1 = vesa_palette[buf[t][x * 2 + 0]];
                    unsigned long c2 = vesa_palette[buf[t][x * 2 + 1]];
                    unsigned char r1, r2, g1, g2, b1, b2;

                    vesa_GetRGB(c1, &r1, &g1, &b1);
                    vesa_GetRGB(c2, &r2, &g2, &b2);
                    r1 = (r1 + r2) / 2;
                    g1 = (g1 + g2) / 2;
                    b1 = (b1 + b2) / 2;
                    vesa_WritePixel32(x, (y + TOP_MAP_ROW) * TILE_Y + t,
                            vesa_MakeColor(r1, g1, b1));
                }
            }
        } else {
            for (t = 0; t < TILE_Y; ++t) {
                for (x = 0; x < 640; ++x) {
                    vesa_WritePixel(x, (y + TOP_MAP_ROW) * TILE_Y + t, buf[t][x]);
                }
            }
        }
    }
}
#endif /* USE_TILES && CLIPPING */

void
vesa_userpan(left)
boolean left;
{
    int x;

    /*	pline("Into userpan"); */
    if (iflags.over_view || iflags.traditional_view)
        return;
    if (left)
        x = min(COLNO - 1, clipxmax + 10);
    else
        x = max(0, clipx - 10);
    vesa_cliparound(x, 10); /* y value is irrelevant on VGA clipping */
    positionbar();
    vesa_DrawCursor();
}

void
vesa_overview(on)
boolean on;
{
    /*	vesa_HideCursor(); */
    if (on) {
        iflags.over_view = TRUE;
        clipx = 0;
        clipxmax = CO - 1;
    } else {
        iflags.over_view = FALSE;
        clipx = max(0, (curcol - viewport_size / 2));
        if (clipx > ((CO - 1) - viewport_size))
            clipx = (CO - 1) - viewport_size;
        clipxmax = clipx + (viewport_size - 1);
    }
}

void
vesa_traditional(on)
boolean on;
{
    /*	vesa_HideCursor(); */
    if (on) {
        /*		switch_symbols(FALSE); */
        iflags.traditional_view = TRUE;
        clipx = 0;
        clipxmax = CO - 1;
    } else {
        iflags.traditional_view = FALSE;
        if (!iflags.over_view) {
            clipx = max(0, (curcol - viewport_size / 2));
            if (clipx > ((CO - 1) - viewport_size))
                clipx = (CO - 1) - viewport_size;
            clipxmax = clipx + (viewport_size - 1);
        }
    }
}

void
vesa_refresh()
{
    positionbar();
    vesa_redrawmap();
    vesa_DrawCursor();
}

static void
decal_packed(gp, special)
const struct TileImage *gp;
unsigned special;
{
    /* FIXME: the tile array is fixed in memory and should not be changed;
       if we ever implement this, we'll have to copy the pixels */
    if (special & MG_CORPSE) {
    } else if (special & MG_INVIS) {
    } else if (special & MG_DETECT) {
    } else if (special & MG_PET) {
    } else if (special & MG_RIDDEN) {
    }
}

/*
 * Open tile files,
 * initialize the SCREEN, switch it to graphics mode,
 * initialize the pointers to the fonts, clear
 * the screen.
 *
 */
void
vesa_Init(void)
{
    const struct Pixel *paletteptr;
#ifdef USE_TILES
    const char *tile_file;
    int tilefailure = 0;
    /*
     * Attempt to open the required tile files. If we can't
     * don't perform the video mode switch, use TTY code instead.
     *
     */
    tile_file = iflags.wc_tile_file;
    if (tile_file == NULL || *tile_file == '\0') {
        tile_file = "nhtiles.bmp";
    }
    if (!read_tiles(tile_file, FALSE))
        tilefailure |= 1;
    if (get_palette() == NULL)
        tilefailure |= 4;

    if (tilefailure) {
        raw_printf("Reverting to TTY mode, tile initialization failure (%d).",
                   tilefailure);
        wait_synch();
        iflags.usevga = 0;
        iflags.tile_view = FALSE;
        iflags.over_view = FALSE;
        CO = 80;
        LI = 25;
        /*	clear_screen()	*/ /* not vesa_clear_screen() */
        return;
    }
#endif

    if (vesa_mode == 0xFFFF) {
        vesa_detect();
    }
    vesa_SwitchMode(vesa_mode);
    windowprocs.win_cliparound = vesa_cliparound;
#ifdef USE_TILES
    paletteptr = get_palette();
    iflags.tile_view = TRUE;
    iflags.over_view = FALSE;
#else
    paletteptr = defpalette;
#endif
    vesa_SetPalette(paletteptr);
    g_attribute = attrib_gr_normal;
    font = vesa_FontPtrs();
    clear_screen();
    clipx = 0;
    clipxmax = clipx + (viewport_size - 1);
}

/*
 * Switches modes of the video card.
 *
 * If mode == MODETEXT (0x03), then the card is placed into text
 * mode.  Otherwise, the card is placed in the mode selected by
 * vesa_detect.  Supported modes are those with packed 8 bit pixels.
 *
 */
static void
vesa_SwitchMode(mode)
unsigned mode;
{
    __dpmi_regs regs;

    if (mode == MODETEXT) {
        iflags.grmode = 0;
        regs.x.ax = mode;
        (void) __dpmi_int(VIDEO_BIOS, &regs);
    } else if (mode >= 0x100) {
        iflags.grmode = 1;
        regs.x.ax = 0x4F02;
        regs.x.bx = mode & 0x81FF;
        (void) __dpmi_int(VIDEO_BIOS, &regs);
        /* Record that the window position is unknown */
        vesa_win_pos[0] = 0xFFFFFFFF;
        vesa_win_pos[1] = 0xFFFFFFFF;
    } else {
        iflags.grmode = 0; /* force text mode for error msg */
        regs.x.ax = MODETEXT;
        (void) __dpmi_int(VIDEO_BIOS, &regs);
        g_attribute = attrib_text_normal;
        impossible("vesa_SwitchMode: Bad video mode requested 0x%X", mode);
    }
}

/*
 * This allows grouping of several tasks to be done when
 * switching back to text mode. This is a public (extern) function.
 *
 */
void
vesa_Finish(void)
{
    free_tiles();
    vesa_SwitchMode(MODETEXT);
    windowprocs.win_cliparound = tty_cliparound;
    g_attribute = attrib_text_normal;
    iflags.tile_view = FALSE;
}

/*
 *
 * Returns a far pointer (or flat 32 bit pointer under djgpp) to the
 * location of the appropriate ROM font for the _current_ video mode
 * (so you must place the card into the desired video mode before
 * calling this function).
 *
 * This function takes advantage of the video BIOS loading the
 * address of the appropriate character definition table for
 * the current graphics mode into interrupt vector 0x43 (0000:010C).
 */
static char __far *
vesa_FontPtrs(void)
{
    USHORT __far *tmp;
    char __far *retval;
    USHORT fseg, foff;
    tmp = (USHORT __far *) MK_PTR(((USHORT) FONT_PTR_SEGMENT),
                                  ((USHORT) FONT_PTR_OFFSET));
    foff = READ_ABSOLUTE_WORD(tmp);
    ++tmp;
    fseg = READ_ABSOLUTE_WORD(tmp);
    retval = (char __far *) MK_PTR(fseg, foff);
    return retval;
}

/*
 * This will verify the existance of a VGA adapter on the machine.
 * Video function call 0x4F00 returns 0x004F in AX if successful, and
 * returns a VbeInfoBlock describing the features of the VESA BIOS.
 */
int
vesa_detect()
{
    int vbe_info_sel = -1; /* custodial */
    int vbe_info_seg;
    struct VbeInfoBlock vbe_info;
    __dpmi_regs regs;
    unsigned long mode_addr;
    struct ModeInfoBlock mode_info;

    vbe_info_seg = __dpmi_allocate_dos_memory(
            (sizeof(vbe_info) + 15) / 16,
            &vbe_info_sel);
    if (vbe_info_seg < 0) goto error;

    /* Request VBE 2.0 information if it is available */
    memset(&vbe_info, 0, sizeof(vbe_info));
    memcpy(vbe_info.VbeSignature, "VBE2", 4);
    dosmemput(&vbe_info, sizeof(vbe_info), vbe_info_seg * 16L);

    /* Request VESA BIOS information */
    regs.x.ax = 0x4F00;
    regs.x.di = 0;
    regs.x.es = vbe_info_seg;
    (void) __dpmi_int(VIDEO_BIOS, &regs);

    /* Check for successful completion of function: is VESA BIOS present? */
    if (regs.x.ax != 0x004F) goto error;
    dosmemget(vbe_info_seg * 16L, sizeof(vbe_info), &vbe_info);
    if (memcmp(vbe_info.VbeSignature, "VESA", 4) != 0) goto error;

    /* Get the address of the mode list */
    /* The mode list may be within the DOS memory area allocated above.
       That area must remain allocated and must not be rewritten until
       we're done here. */
    mode_addr = (vbe_info.VideoModePtr >> 16) * 16L
              + (vbe_info.VideoModePtr & 0xFFFF);

    /* Scan the mode list for an acceptable mode */
    vesa_mode = vesa_FindMode(mode_addr, 32);
    if (vesa_mode == 0xFFFF)
        vesa_mode = vesa_FindMode(mode_addr, 24);
    if (vesa_mode == 0xFFFF)
        vesa_mode = vesa_FindMode(mode_addr, 16);
    if (vesa_mode == 0xFFFF)
        vesa_mode = vesa_FindMode(mode_addr, 15);
    if (vesa_mode == 0xFFFF)
        vesa_mode = vesa_FindMode(mode_addr,  8);
    if (vesa_mode == 0xFFFF)
        goto error;

    /* Set up the variables for the pixel functions */
    vesa_GetModeInfo(vesa_mode, &mode_info);
    vesa_x_res = mode_info.XResolution;
    vesa_y_res = mode_info.YResolution;
    vesa_x_center = (vesa_x_res - 640) / 2;
    vesa_y_center = (vesa_y_res - 480) / 2;
    vesa_scan_line = mode_info.BytesPerScanLine;
    vesa_win_size = mode_info.WinSize * 1024L;
    vesa_win_gran = mode_info.WinGranularity * 1024L;
    vesa_pixel_size = mode_info.BitsPerPixel;
    vesa_pixel_bytes = (vesa_pixel_size + 7) / 8;
    if (vbe_info.VbeVersion >= 0x0300) {
        vesa_red_pos = mode_info.RedFieldPosition;
        vesa_red_size = mode_info.RedMaskSize;
        vesa_green_pos = mode_info.GreenFieldPosition;
        vesa_green_size = mode_info.GreenMaskSize;
        vesa_blue_pos = mode_info.BlueFieldPosition;
        vesa_blue_size = mode_info.BlueMaskSize;
    } else {
        switch (vesa_pixel_size) {
        case 15:
            vesa_blue_pos = 0;
            vesa_blue_size = 5;
            vesa_green_pos = 5;
            vesa_green_size = 5;
            vesa_red_pos = 10;
            vesa_red_size = 5;
            break;

        case 16:
            vesa_blue_pos = 0;
            vesa_blue_size = 5;
            vesa_green_pos = 5;
            vesa_green_size = 6;
            vesa_red_pos = 11;
            vesa_red_size = 5;
            break;

        case 24:
        case 32:
            vesa_blue_pos = 0;
            vesa_blue_size = 8;
            vesa_green_pos = 8;
            vesa_green_size = 8;
            vesa_red_pos = 16;
            vesa_red_size = 8;
            break;
        }
    }
    vesa_win_addr[0] = mode_info.WinASegment * 16L;
    vesa_win_addr[1] = mode_info.WinBSegment * 16L;
    vesa_win_pos[0] = 0xFFFFFFFF; /* position unknown */
    vesa_win_pos[1] = 0xFFFFFFFF;
    /* Read window */
    if (mode_info.WinAAttributes & 0x2) {
        vesa_read_win = 0;
    } else if (mode_info.WinBAttributes & 0x2) {
        vesa_read_win = 1;
    } else {
        goto error; /* Shouldn't happen */
    }
    /* Write window */
    if (mode_info.WinAAttributes & 0x4) {
        vesa_write_win = 0;
    } else if (mode_info.WinBAttributes & 0x4) {
        vesa_write_win = 1;
    } else {
        goto error; /* Shouldn't happen */
    }

    __dpmi_free_dos_memory(vbe_info_sel);
    return TRUE;

error:
    if (vbe_info_sel != -1) __dpmi_free_dos_memory(vbe_info_sel);
    return FALSE;
}

static unsigned
vesa_FindMode(mode_addr, bits)
unsigned long mode_addr;
unsigned bits;
{
    unsigned selected_mode;
    struct ModeInfoBlock mode_info0, mode_info;
    unsigned model = (bits == 8) ? 4 : 6;

    memset(&mode_info, 0, sizeof(mode_info));
    selected_mode = 0xFFFF;
    while (1) {
        unsigned mode = _farpeekw(_dos_ds, mode_addr);
        if (mode == 0xFFFF) break;
        mode_addr += 2;

        /* Query the mode info; skip to next if not in fact supported */
        if (!vesa_GetModeInfo(mode, &mode_info0)) continue;

        /* Check that the mode is acceptable */
        if (mode_info0.XResolution < 640) continue;
        if (mode_info0.YResolution < 480) continue;
        if (mode_info0.NumberOfPlanes != 1) continue;
        if (mode_info0.BitsPerPixel != bits) continue;
        if (mode_info0.NumberOfBanks != 1) continue;
        if (mode_info0.MemoryModel != model) continue;
        if (mode_info0.ModeAttributes & 0x40) continue;

        /* The mode is OK. Accept it if it is smaller than any previous mode
           or if no previous mode is accepted. */
        if (selected_mode == 0xFFFF
        ||  mode_info0.XResolution * mode_info0.YResolution
          < mode_info.XResolution * mode_info.YResolution) {
            selected_mode = mode;
            mode_info = mode_info0;
        }
    }

    return selected_mode;
}

/*
 * Write character 'ch', at (x,y) and
 * do it using the colour 'colour'.
 *
 */
static void
vesa_WriteChar(chr, col, row, colour, transparent)
int chr, col, row, colour;
boolean transparent;
{
    int i, j;
    int pixx, pixy;

    unsigned char __far *fp = font;
    unsigned char fnt;

    pixx = min(col, (CO - 1)) * 8;  /* min() protects from callers */
    pixy = min(row, (LI - 1)) * 16; /* assumes 8 x 16 char set */
    pixx += vesa_x_center;
    pixy += vesa_y_center;

    for (i = 0; i < MAX_ROWS_PER_CELL; ++i) {
        fnt = READ_ABSOLUTE((fp + chr * 16 + i));
        for (j = 0; j < 8; ++j) {
            if (fnt & (0x80 >> j)) {
                vesa_WritePixel(pixx + j, pixy + i, colour + FIRST_TEXT_COLOR);
            } else if (!transparent) {
                vesa_WritePixel(pixx + j, pixy + i, BACKGROUND_VESA_COLOR);
            }
        }
    }
}

/*
 * Like vesa_WriteChar, but draw the character in local memory rather than in
 * the VGA frame buffer.
 *
 * vesa_redrawmap uses this to gather a row of cells in local memory and then
 * draw them in strict row-major order, minimizing the use of the VESA
 * windowing function.
 *
 */
static void
vesa_WriteCharInMemory(chr, col, buf, colour)
int chr, col;
char buf[TILE_Y][640*2];
int colour;
{
    int i, j;
    int pixx;

    unsigned char __far *fp = font;
    unsigned char fnt;

    pixx = min(col, (CO - 1)) * 8;  /* min() protects from callers */

    for (i = 0; i < MAX_ROWS_PER_CELL; ++i) {
        fnt = READ_ABSOLUTE((fp + chr * 16 + i));
        for (j = 0; j < 8; ++j) {
            if (fnt & (0x80 >> j)) {
                buf[i][pixx + j] = colour + FIRST_TEXT_COLOR;
            } else {
                buf[i][pixx + j] = BACKGROUND_VESA_COLOR;
            }
        }
    }
}

/*
 * This is the routine that displays a high-res "cell" pointed to by 'gp'
 * at the desired location (col,row).
 *
 * Note: (col,row) in this case refer to the coordinate location in
 * NetHack character grid terms, (ie. the 40 x 25 character grid),
 * not the x,y pixel location.
 *
 */
static void
vesa_DisplayCell(tile, col, row)
const struct TileImage *tile;
int col, row;
{
    int i, j, pixx, pixy;

    pixx = col * TILE_X;
    pixy = row * TILE_Y;
    if (iflags.over_view) {
        pixx /= 2;
        pixx += vesa_x_center;
        pixy += vesa_y_center;
        if (vesa_pixel_size != 8) {
            for (i = 0; i < TILE_Y; ++i) {
                for (j = 0; j < TILE_X; j += 2) {
                    unsigned index = i * tile->width + j;
                    unsigned long c1 = vesa_palette[tile->indexes[index + 0]];
                    unsigned long c2 = vesa_palette[tile->indexes[index + 1]];
                    unsigned char r1, r2, g1, g2, b1, b2;

                    vesa_GetRGB(c1, &r1, &g1, &b1);
                    vesa_GetRGB(c2, &r2, &g2, &b2);
                    r1 = (r1 + r2) / 2;
                    g1 = (g1 + g2) / 2;
                    b1 = (b1 + b2) / 2;
                    vesa_WritePixel32(pixx + j / 2, pixy + i,
                            vesa_MakeColor(r1, g1, b1));
                }
            }
        } else {
            for (i = 0; i < TILE_Y; ++i) {
                for (j = 0; j < TILE_X; j += 2) {
                    unsigned index = i * tile->width + j;
                    vesa_WritePixel(pixx + j / 2, pixy + i, tile->indexes[index]);
                }
            }
        }
    } else {
        pixx += vesa_x_center;
        pixy += vesa_y_center;
        for (i = 0; i < TILE_Y; ++i) {
            for (j = 0; j < TILE_X; ++j) {
                unsigned index = i * tile->width + j;
                vesa_WritePixel(pixx + j, pixy + i, tile->indexes[index]);
            }
        }
    }
}

/*
 * Like vesa_DisplayCell, but draw the tile in local memory rather than in
 * the VGA frame buffer.
 *
 * vesa_redrawmap uses this to gather a row of cells in local memory and then
 * draw them in strict row-major order, minimizing the use of the VESA
 * windowing function.
 *
 */
static void
vesa_DisplayCellInMemory(tile, col, buf)
const struct TileImage *tile;
int col;
char buf[TILE_Y][640*2];
{
    int i, j, pixx;

    pixx = col * TILE_X;
    if (iflags.over_view && vesa_pixel_size == 8) {
        pixx /= 2;
        for (i = 0; i < TILE_Y; ++i) {
            for (j = 0; j < TILE_X; j += 2) {
                unsigned index = i * tile->width + j;
                buf[i][pixx + j / 2] = tile->indexes[index];
            }
        }
    } else {
        for (i = 0; i < TILE_Y; ++i) {
            for (j = 0; j < TILE_X; ++j) {
                unsigned index = i * tile->width + j;
                buf[i][pixx + j] = tile->indexes[index];
            }
        }
    }
}

/*
 * Write the character string pointed to by 's', whose maximum length
 * is 'len' at location (x,y) using the 'colour' colour.
 *
 */
static void
vesa_WriteStr(s, len, col, row, colour)
const char *s;
int len, col, row, colour;
{
    const unsigned char *us;
    int i = 0;

    /* protection from callers */
    if (row > (LI - 1))
        return;

    i = 0;
    us = (const unsigned char *) s;
    while ((*us != 0) && (i < len) && (col < (CO - 1))) {
        vesa_WriteChar(*us, col, row, colour, FALSE);
        ++us;
        ++i;
        ++col;
    }
}

/*
 * Initialize the VGA palette with the desired colours. This
 * must be a series of 720 bytes for use with a card in 256
 * colour mode at 640 x 480. The first 240 palette entries are
 * used by the tile set; the last 16 are reserved for text.
 *
 */
static boolean
vesa_SetPalette(palette)
const struct Pixel *palette;
{
    if (vesa_pixel_size == 8) {
        vesa_SetHardPalette(palette);
    } else {
        vesa_SetSoftPalette(palette);
    }
}

static boolean
vesa_SetHardPalette(palette)
const struct Pixel *palette;
{
    const struct Pixel *p = palette;
    int palette_sel = -1; /* custodial */
    int palette_seg;
    unsigned long palette_ptr;
    unsigned i, shift;
    unsigned char r, g, b;
    unsigned long color;
    __dpmi_regs regs;

    palette_seg = __dpmi_allocate_dos_memory( 1024 / 16, &palette_sel);
    if (palette_seg < 0) goto error;

    /* Use 8 bit DACs if we have them */
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = 0x4F08;
    regs.h.bl = 0;
    regs.h.bh = 8;
    (void) __dpmi_int(VIDEO_BIOS, &regs);
    if (regs.x.ax != 0x004F) {
        shift = 2;
    } else if (regs.h.bh > 8) {
        shift = 0;
    } else {
        shift = 8 - regs.h.bh;
    }

    /* Set the tile set and text colors */
    palette_ptr = palette_seg * 16L;
#ifdef USE_TILES
    for (i = 0; i < FIRST_TEXT_COLOR; ++i) {
        r = p->r >> shift;
        g = p->g >> shift;
        b = p->b >> shift;
        color =   ((unsigned long) r << 16)
                | ((unsigned long) g <<  8)
                | ((unsigned long) b <<  0);
        _farpokel(_dos_ds, palette_ptr, color);
        palette_ptr += 4;
        ++p;
    }
#else
    palette_ptr += FIRST_TEXT_COLOR * 4;
#endif
    p = defpalette;
    for (i = FIRST_TEXT_COLOR; i < 256; ++i) {
        r = p->r >> shift;
        g = p->g >> shift;
        b = p->b >> shift;
        color =   ((unsigned long) r << 16)
                | ((unsigned long) g <<  8)
                | ((unsigned long) b <<  0);
        _farpokel(_dos_ds, palette_ptr, color);
        palette_ptr += 4;
        ++p;
    }

    memset(&regs, 0, sizeof(regs));
    regs.x.ax = 0x4F09;
    regs.h.bl = 0;
    regs.x.cx = 256;
    regs.x.dx = 0;
    regs.x.di = 0;
    regs.x.es = palette_seg;
    (void) __dpmi_int(VIDEO_BIOS, &regs);

    __dpmi_free_dos_memory(palette_sel);
    return TRUE;

error:
    if (palette_sel != -1) __dpmi_free_dos_memory(palette_sel);
    return FALSE;
}

static boolean
vesa_SetSoftPalette(palette)
const struct Pixel *palette;
{
    const struct Pixel *p;
    unsigned i;
    unsigned char r, g, b;

    /* Set the tile set and text colors */
#ifdef USE_TILES
    p = palette;
    for (i = 0; i < FIRST_TEXT_COLOR; ++i) {
        r = p->r;
        g = p->g;
        b = p->b;
        vesa_palette[i] = vesa_MakeColor(r, g, b);
        ++p;
    }
#endif
    p = defpalette;
    for (i = FIRST_TEXT_COLOR; i < 256; ++i) {
        r = p->r;
        g = p->g;
        b = p->b;
        vesa_palette[i] = vesa_MakeColor(r, g, b);
        ++p;
    }
}

#ifdef POSITIONBAR

#define PBAR_ROW (LI - 4)
#define PBAR_COLOR_ON 16    /* slate grey background colour of tiles */
#define PBAR_COLOR_OFF 0    /* bluish grey, used in old style only */
#define PBAR_COLOR_STAIRS CLR_BROWN /* brown */
#define PBAR_COLOR_HERO CLR_WHITE  /* creamy white */

static unsigned char pbar[COLNO];

void
vesa_update_positionbar(posbar)
char *posbar;
{
    char *p = pbar;
    if (posbar)
        while (*posbar)
            *p++ = *posbar++;
    *p = 0;
}

static void
positionbar()
{
    char *posbar = pbar;
    int feature, ucol;
    int k, x, y, colour, row;

    int startk, stopk;
    boolean nowhere = FALSE;
    int pixy = (PBAR_ROW * MAX_ROWS_PER_CELL);
    int tmp;

    if (!iflags.grmode || !iflags.tile_view)
        return;
    if ((clipx < 0) || (clipxmax <= 0) || (clipx >= clipxmax))
        nowhere = TRUE;
    if (nowhere) {
#ifdef DEBUG
        pline("Would have put bar using %d - %d.", clipx, clipxmax);
#endif
        return;
    }
#ifdef OLD_STYLE
    for (y = pixy; y < (pixy + MAX_ROWS_PER_CELL); ++y) {
        for (x = 0; x < 640; ++x) {
            k = x / 8;
            if ((k < clipx) || (k > clipxmax)) {
                colour = PBAR_COLOR_OFF;
            } else
                colour = PBAR_COLOR_ON;
            vesa_WritePixel(x + vesa_x_center, y + vesa_y_center, colour);
        }
    }
#else
    for (y = pixy, row = 0; y < (pixy + MAX_ROWS_PER_CELL); ++y, ++row) {
        if ((!row) || (row == (ROWS_PER_CELL - 1))) {
            startk = 0;
            stopk = SCREENBYTES;
        } else {
            startk = clipx;
            stopk = clipxmax;
        }
        for (x = 0; x < 640; ++x) {
            k = x / 8;
            if ((k < startk) || (k > stopk))
                colour = BACKGROUND_VGA_COLOR;
            else
                colour = PBAR_COLOR_ON;
            vesa_WritePixel(x + vesa_x_center, y + vesa_y_center, colour);
        }
    }
#endif
    ucol = 0;
    if (posbar) {
        while (*posbar != 0) {
            feature = *posbar++;
            switch (feature) {
            case '>':
                vesa_WriteChar(feature, (int) *posbar++, PBAR_ROW, PBAR_COLOR_STAIRS, TRUE);
                break;
            case '<':
                vesa_WriteChar(feature, (int) *posbar++, PBAR_ROW, PBAR_COLOR_STAIRS, TRUE);
                break;
            case '@':
                ucol = (int) *posbar++;
                vesa_WriteChar(feature, ucol, PBAR_ROW, PBAR_COLOR_HERO, TRUE);
                break;
            default: /* unanticipated symbols */
                vesa_WriteChar(feature, (int) *posbar++, PBAR_ROW, PBAR_COLOR_STAIRS, TRUE);
                break;
            }
        }
    }
#ifdef SIMULATE_CURSOR
    if (inmap) {
        tmp = curcol + 1;
        if ((tmp != ucol) && (curcol >= 0))
            vesa_WriteChar('_', tmp, PBAR_ROW, PBAR_COLOR_HERO, TRUE);
    }
#endif
}

#endif /*POSITIONBAR*/

#ifdef SIMULATE_CURSOR

static unsigned long undercursor[TILE_Y][TILE_X];

void
vesa_DrawCursor()
{
    unsigned x, y, left, top, right, bottom, width;
    boolean isrogue = Is_rogue_level(&u.uz);
    boolean halfwidth =
        (isrogue || iflags.over_view || iflags.traditional_view || !inmap);
    int curtyp;

    if (!cursor_type && inmap)
        return; /* CURSOR_INVIS - nothing to do */

    x = min(curcol, (CO - 1)); /* protection from callers */
    y = min(currow, (LI - 1)); /* protection from callers */
    if (!halfwidth && ((x < clipx) || (x > clipxmax)))
        return;
    if (inmap)
        x -= clipx;
    left = x * TILE_X; /* convert to pixels */
    top  = y * TILE_Y;
    if (halfwidth) {
        left /= 2;
        width = TILE_X / 2;
    } else {
        width = TILE_X;
    }
    left += vesa_x_center;
    top  += vesa_y_center;
    right = left + width - 1;
    bottom = top + TILE_Y - 1;

    for (y = 0; y < ROWS_PER_CELL; ++y) {
        for (x = 0; x < width; ++x) {
            undercursor[y][x] = vesa_ReadPixel32(left + x, top + y);
        }
    }

    /*
     * Now we have a snapshot of the current cell.
     * Write the cursor on top of the display.
     */

    if (inmap)
        curtyp = cursor_type;
    else
        curtyp = CURSOR_UNDERLINE;

    switch (curtyp) {
    case CURSOR_CORNER:
        vesa_WritePixel(left     , top       , FIRST_TEXT_COLOR + 15);
        vesa_WritePixel(left  + 1, top       , FIRST_TEXT_COLOR + 15);
        vesa_WritePixel(right - 1, top       , FIRST_TEXT_COLOR + 15);
        vesa_WritePixel(right    , top       , FIRST_TEXT_COLOR + 15);
        vesa_WritePixel(left     , top    + 1, FIRST_TEXT_COLOR + 15);
        vesa_WritePixel(right    , top    + 1, FIRST_TEXT_COLOR + 15);
        vesa_WritePixel(left     , bottom - 1, FIRST_TEXT_COLOR + 15);
        vesa_WritePixel(right    , bottom - 1, FIRST_TEXT_COLOR + 15);
        vesa_WritePixel(left     , bottom    , FIRST_TEXT_COLOR + 15);
        vesa_WritePixel(left  + 1, bottom    , FIRST_TEXT_COLOR + 15);
        vesa_WritePixel(right - 1, bottom    , FIRST_TEXT_COLOR + 15);
        vesa_WritePixel(right    , bottom    , FIRST_TEXT_COLOR + 15);
        break;

    case CURSOR_UNDERLINE:
        for (x = left; x <= right; ++x) {
            vesa_WritePixel(x, bottom, FIRST_TEXT_COLOR + 15);
        }
        break;

    case CURSOR_FRAME:

    /* fall through */

    default:
        for (x = left; x <= right; ++x) {
            vesa_WritePixel(x, top, FIRST_TEXT_COLOR + 15);
        }
        for (y = top + 1; y <= bottom - 1; ++y) {
            vesa_WritePixel(left , y, FIRST_TEXT_COLOR + 15);
            vesa_WritePixel(right, y, FIRST_TEXT_COLOR + 15);
        }
        for (x = left; x <= right; ++x) {
            vesa_WritePixel(x, bottom, FIRST_TEXT_COLOR + 15);
        }
        break;
    }
#ifdef POSITIONBAR
    if (inmap)
        positionbar();
#endif
}

void
vesa_HideCursor()
{
    unsigned x, y, left, top, width;
    boolean isrogue = Is_rogue_level(&u.uz);
    boolean halfwidth =
        (isrogue || iflags.over_view || iflags.traditional_view || !inmap);
    int curtyp;

    if (!cursor_type && inmap)
        return; /* CURSOR_INVIS - nothing to do */

    x = min(curcol, (CO - 1)); /* protection from callers */
    y = min(currow, (LI - 1)); /* protection from callers */
    if (!halfwidth && ((x < clipx) || (x > clipxmax)))
        return;
    if (inmap)
        x -= clipx;
    left = x * TILE_X; /* convert to pixels */
    top  = y * TILE_Y;
    if (halfwidth) {
        left /= 2;
        width = TILE_X / 2;
    } else {
        width = TILE_X;
    }
    left += vesa_x_center;
    top  += vesa_y_center;

    for (y = 0; y < ROWS_PER_CELL; ++y) {
        for (x = 0; x < width; ++x) {
            vesa_WritePixel32(left + x, top + y, undercursor[y][x]);
        }
    }
}
#endif /* SIMULATE_CURSOR */
#endif /* SCREEN_VESA */

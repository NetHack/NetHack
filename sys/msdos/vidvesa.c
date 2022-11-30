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
#include "font.h"

#define FIRST_TEXT_COLOR 240

extern int total_tiles_used, Tile_corr, Tile_unexplored;  /* from tile.c */
struct VesaCharacter {
    uint32 colour;
    uint32 chr;
};

static unsigned long vesa_SetWindow(int window, unsigned long offset);
static unsigned long vesa_ReadPixel32(unsigned x, unsigned y);
static void vesa_WritePixel32(unsigned x, unsigned y, unsigned long color);
static void vesa_WritePixel(unsigned x, unsigned y, unsigned color);
static void vesa_WritePixelRow(unsigned long offset,
                               unsigned char const *p_row, unsigned p_row_size);
static unsigned long vesa_MakeColor(struct Pixel);
static void vesa_FillRect(unsigned left, unsigned top, unsigned width,
                          unsigned height, unsigned color);

static void vesa_redrawmap(void);
static void vesa_cliparound(int, int);
#if 0
static void decal_packed(const struct TileImage *tile, unsigned special);
#endif
static void vesa_SwitchMode(unsigned mode);
static void vesa_SetViewPort(void);
static boolean vesa_SetPalette(const struct Pixel *);
static boolean vesa_SetHardPalette(const struct Pixel *);
static boolean vesa_SetSoftPalette(const struct Pixel *);
static void vesa_DisplayCell(int, int, int);
static unsigned vesa_FindMode(unsigned long mode_addr, unsigned bits);
static void vesa_WriteChar(uint32, int, int, uint32);
static void vesa_WriteCharXY(uint32, int, int, uint32);
static void vesa_WriteCharTransparent(int, int, int, int);
static void vesa_WriteTextRow(int pixx, int pixy,
                              struct VesaCharacter const *t_row, unsigned t_row_width);
static boolean vesa_GetCharPixel(int, unsigned, unsigned);
static unsigned char vesa_GetCharPixelRow(uint32, unsigned, unsigned);
static unsigned long vesa_DoublePixels(unsigned long);
static unsigned long vesa_TriplePixels(unsigned long);
static void vesa_WriteStr(const char *, int, int, int, int);
static unsigned char __far *vesa_FontPtrs(void);
/* static void vesa_process_tile(struct TileImage *tile); */

#ifdef POSITIONBAR
static void positionbar(void);
#endif

extern int clipx, clipxmax; /* current clipping column from wintty.c */
extern int clipy, clipymax; /* current clipping row from wintty.c */
extern int curcol, currow;  /* current column and row        */
extern int g_attribute;
extern int attrib_text_normal;  /* text mode normal attribute */
extern int attrib_gr_normal;    /* graphics mode normal attribute */
extern int attrib_gr_intense;   /* graphics mode intense attribute */
extern boolean inmap;           /* in the map window */

/*
 * Global Variables
 */

static unsigned char __far *font;

static struct map_struct {
    int glyph;
    uint32 ch;
    uint32 attr;
    unsigned special;
    short int tileidx;
} map[ROWNO][COLNO]; /* track the glyphs */

#define vesa_clearmap()                                         \
    {                                                           \
        int x, y;                                               \
        for (y = 0; y < ROWNO; ++y)                             \
            for (x = 0; x < COLNO; ++x) {                       \
                map[y][x].glyph = GLYPH_UNEXPLORED;             \
                map[y][x].ch = glyph2ttychar(GLYPH_UNEXPLORED); \
                map[y][x].attr = 0;                             \
                map[y][x].special = 0;                          \
                map[y][x].tileidx = Tile_unexplored;            \
            }                                                   \
    }
#define TOP_MAP_ROW 1

static int viewport_cols = 40;
static int viewport_rows = ROWNO;

static const struct Pixel defpalette[] = {    /* Colors for text and the position bar */
        { 0x00, 0x00, 0x00, 0xff }, /* CLR_BLACK */
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
static unsigned long vesa_win_func;
static unsigned long vesa_win_pos[2]; /* Window position */
static unsigned long vesa_win_addr[2]; /* Window physical address */
static unsigned long vesa_segment;  /* Selector of linear frame buffer */
static unsigned long vesa_win_size; /* Window size */
static unsigned long vesa_win_gran; /* Window granularity */
static unsigned char vesa_pixel_size;
static unsigned char vesa_pixel_bytes;
static unsigned char vesa_red_pos;
static unsigned char vesa_red_shift;
static unsigned char vesa_green_pos;
static unsigned char vesa_green_shift;
static unsigned char vesa_blue_pos;
static unsigned char vesa_blue_shift;
static unsigned long vesa_palette[256];
static unsigned vesa_char_width = 8, vesa_char_height = 16;
static unsigned vesa_oview_width, vesa_oview_height;
static unsigned char **vesa_tiles;
static unsigned char **vesa_oview_tiles;
static struct BitmapFont *vesa_font;

#ifdef SIMULATE_CURSOR
static unsigned long *undercursor;
#endif

/* Used to cache character writes for speed */
static struct VesaCharacter chr_cache[100];
static unsigned chr_cache_size;
static unsigned chr_cache_pixx, chr_cache_pixy, chr_cache_lastx;

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
vesa_GetModeInfo(unsigned mode, struct ModeInfoBlock *info)
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

static unsigned
vesa_map_frame_buffer(unsigned phys_addr, unsigned size)
{
    __dpmi_meminfo info;
    int rc;

    info.handle = 0;
    info.address = phys_addr;
    info.size = size;
    rc = __dpmi_physical_address_mapping(&info);

    return rc == 0 ? info.address : 0;
}

/* Set the memory window and return the offset */
static unsigned long
vesa_SetWindow(int window, unsigned long offset)
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

        if (vesa_win_func != 0) {
            regs.x.cs = vesa_win_func >> 16;
            regs.x.ip = vesa_win_func & 0xFFFF;
            (void) __dpmi_simulate_real_mode_procedure_retf(&regs);
        } else {
            (void) __dpmi_int(VIDEO_BIOS, &regs);
        }
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
vesa_ReadPixel32(unsigned x, unsigned y)
{
    unsigned long offset = y * vesa_scan_line + x * vesa_pixel_bytes;
    unsigned long addr, color;
    unsigned i;

    if (vesa_segment != 0) {
        /* Linear frame buffer in use */
        switch (vesa_pixel_size) {
        case 8:
            color = _farpeekb(vesa_segment, offset);
            break;

        case 15:
        case 16:
            color = _farpeekw(vesa_segment, offset);
            break;

        case 24:
            /* Don't cross a 4 byte boundary if it can be avoided */
            if (offset & 1) {
                color = _farpeekl(vesa_segment, offset - 1) >> 8;
            } else {
                color = _farpeekl(vesa_segment, offset) & 0xFFFFFFFF;
            }
            break;

        case 32:
            color = _farpeekl(vesa_segment, offset);
            break;

        default: /* Shouldn't happen */
            color = 0;
            break;
        }
    } else {
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

        default: /* Shouldn't happen */
            color = 0;
            break;
        }
    }
    return color;
}

static void
vesa_WritePixel32(unsigned x, unsigned y, unsigned long color)
{
    unsigned long offset = y * vesa_scan_line + x * vesa_pixel_bytes;
    unsigned long addr;
    unsigned i;

    if (vesa_segment != 0) {
        /* Linear frame buffer in use */
        switch (vesa_pixel_size) {
        case 8:
            _farpokeb(vesa_segment, offset, color);
            break;

        case 15:
        case 16:
            _farpokew(vesa_segment, offset, color);
            break;

        case 24:
            if (offset & 1) {
                _farpokeb(vesa_segment, offset + 0, color & 0xFF);
                _farpokew(vesa_segment, offset + 1, color >> 8);
            } else {
                _farpokew(vesa_segment, offset + 0, color & 0xFFFF);
                _farpokeb(vesa_segment, offset + 2, color >> 16);
            }
            break;

        case 32:
            _farpokel(vesa_segment, offset, color);
            break;

        default: /* Shouldn't happen */
            color = 0;
            break;
        }
    } else {
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
                addr = vesa_SetWindow(vesa_write_win, offset + i);
                _farpokeb(_dos_ds, addr, (unsigned char) (color >> (i * 8)));
            }
            break;

        case 32:
            addr = vesa_SetWindow(vesa_write_win, offset);
            _farpokel(_dos_ds, addr, color);
            break;
        }
    }
}

static void
vesa_WritePixel(unsigned x, unsigned y, unsigned color)
{
    if (vesa_pixel_size == 8) {
        vesa_WritePixel32(x, y, color);
    } else {
        vesa_WritePixel32(x, y, vesa_palette[color & 0xFF]);
    }
}

static void
vesa_WritePixelRow(unsigned long offset,
                   unsigned char const *p_row, unsigned p_row_size)
{
    if (vesa_segment != 0) {
        /* Linear frame buffer in use */
        movedata(_go32_my_ds(), (unsigned)p_row, vesa_segment, offset, p_row_size);
    } else {
        /* Windowed frame buffer in use */
        unsigned long addr = vesa_SetWindow(vesa_write_win, offset);
        unsigned i = 0;
        while (offset + p_row_size > vesa_win_pos[vesa_write_win] + vesa_win_size) {
            /* The row passes the end of the current window. Write what we can,
             * and advance the window */
            unsigned w = (vesa_win_pos[vesa_write_win] + vesa_win_size)
                       - (offset + i);
            dosmemput(p_row + i, w, addr);
            i += w;
            addr = vesa_SetWindow(vesa_write_win, offset + i);
        }
        dosmemput(p_row + i, p_row_size - i, addr);
    }
}

static unsigned long
vesa_MakeColor(struct Pixel p)
{
    unsigned long r = p.r >> vesa_red_shift;
    unsigned long g = p.g >> vesa_green_shift;
    unsigned long b = p.b >> vesa_blue_shift;
    return (r << vesa_red_pos)
         | (g << vesa_green_pos)
         | (b << vesa_blue_pos);
}

static void
vesa_FillRect(unsigned left, unsigned top, unsigned width,
              unsigned height, unsigned color)
{
    unsigned p_row_size = width * vesa_pixel_bytes;
    unsigned char *p_row = (unsigned char *) alloc(p_row_size);
    unsigned long c32 = vesa_palette[color & 0xFF];
    unsigned long offset = top * (unsigned long)vesa_scan_line + left * vesa_pixel_bytes;
    unsigned x, y;

    switch (vesa_pixel_bytes) {
    case 1:
        memset(p_row, color, p_row_size);
        break;

    case 2:
        for (x = 0; x < width; ++x) {
            ((uint16_t *)p_row)[x] = c32;
        }
        break;

    case 3:
        for (x = 0; x < width; ++x) {
            p_row[3*x + 0] =  c32        & 0xFF;
            p_row[3*x + 1] = (c32 >>  8) & 0xFF;
            p_row[3*x + 2] =  c32 >> 16        ;
        }
        break;

    case 4:
        for (x = 0; x < width; ++x) {
            ((uint32_t *)p_row)[x] = c32;
        }
        break;
    }

    for (y = 0; y < height; ++y) {
        vesa_WritePixelRow(offset, p_row, p_row_size);
        offset += vesa_scan_line;
    }

    free(p_row);
}

void
vesa_get_scr_size(void)
{
    CO = vesa_x_res / vesa_char_width;
    LI = vesa_y_res / vesa_char_height - 1;
}

void
vesa_backsp(void)
{
    int col, row;

    col = curcol; /* Character cell row and column */
    row = currow;

    if (col > 0)
        col = col - 1;
    vesa_gotoloc(col, row);
}

void
vesa_clear_screen(int colour)
{
    vesa_FillRect(0, 0, vesa_x_res, vesa_y_res, colour);
    if (iflags.tile_view)
        vesa_clearmap();
    vesa_gotoloc(0, 0); /* is this needed? */
}

/* clear to end of line */
void
vesa_cl_end(int col, int row)
{
    unsigned left = vesa_x_center + col * vesa_char_width;
    unsigned top  = vesa_y_center + row * vesa_char_height;
    unsigned width = (CO - 1 - col) * vesa_char_width;
    unsigned height = vesa_char_height;

    vesa_flush_text();
    vesa_FillRect(left, top, width, height, BACKGROUND_VESA_COLOR);
}

/* clear to end of screen */
void
vesa_cl_eos(int cy)
{
    cl_end();
    if (cy < LI - 1) {
        unsigned left = 0;
        unsigned top  = vesa_y_center + cy * vesa_char_height;
        unsigned width = vesa_x_res;
        unsigned height = (LI - 1 - cy) * vesa_char_height;

        vesa_FillRect(left, top, width, height, BACKGROUND_VESA_COLOR);
    }
}

void
vesa_tty_end_screen(void)
{
    vesa_clear_screen(BACKGROUND_VESA_COLOR);
    vesa_SwitchMode(MODETEXT);
}

void
vesa_tty_startup(int *wid, int *hgt)
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
vesa_xputs(const char *s, int col, int row)
{
    if (s != NULL) {
        vesa_WriteStr(s, strlen(s), col, row, g_attribute);
    }
}

/* write out character (and attribute) */
void
vesa_xputc(char ch, int attr)
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
        vesa_WriteChar((unsigned char) ch, col, row, attr);
        if (ch == '.') {
            vesa_flush_text();
        }
        if (col < (CO - 1))
            ++col;
        break;
    } /* end switch */
    vesa_gotoloc(col, row);
}

#if defined(USE_TILES)
/* Place tile represent. a glyph at current location */
void
vesa_xputg(const glyph_info *glyphinfo)
{
    int glyphnum = glyphinfo->glyph;
    uint32 ch = (uchar) glyphinfo->ttychar;
    unsigned special = glyphinfo->gm.glyphflags;
    int col, row;
    uint32_t attr = (g_attribute == 0) ? attrib_gr_normal : g_attribute;
    int ry;

#ifdef ENHANCED_SYMBOLS
    if (SYMHANDLING(H_UTF8) && glyphinfo->gm.u && glyphinfo->gm.u->utf8str) {
        ch = glyphinfo->gm.u->utf32ch;
        if (vesa_pixel_size > 8 && glyphinfo->gm.u->ucolor != 0) {
            /* FIXME: won't display black (0,0,0) correctly, but the background
               is usually black anyway */
            attr = glyphinfo->gm.u->ucolor | 0x80000000;
        }
    }
#endif

    row = currow;
    col = curcol;
    if ((col < 0 || col >= COLNO)
        || (row < TOP_MAP_ROW || row >= (ROWNO + TOP_MAP_ROW)))
        return;
    ry = row - TOP_MAP_ROW;
    map[ry][col].glyph = glyphnum;
    map[ry][col].ch = ch;
    map[ry][col].special = special;
    map[ry][col].tileidx = glyphinfo->gm.tileidx;
    map[ry][col].attr = attr;
    if (iflags.traditional_view) {
        vesa_WriteChar(ch, col, row, attr);
    } else {
        if ((col >= clipx) && (col <= clipxmax)
        &&  (ry >= clipy) && (ry <= clipymax)) {
#if 0
            if (!iflags.over_view && map[ry][col].special)
                decal_packed(packcell, special);
#endif
            vesa_DisplayCell(glyphinfo->gm.tileidx, col - clipx, ry - clipy);
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
 *                       and y character cell location.  This routine
 *                       determines the location where screen writes
 *                       will occur next, it does not change the location
 *                       of the player on the NetHack level.
 */

void
vesa_gotoloc(int col, int row)
{
    curcol = min(col, CO - 1); /* protection from callers */
    currow = min(row, LI - 1);
}

#if defined(USE_TILES) && defined(CLIPPING)
static void
vesa_cliparound(int x, int y)
{
    int oldx = clipx, oldy = clipy;

    if (!iflags.tile_view || iflags.over_view || iflags.traditional_view)
        return;

    if (viewport_cols < COLNO) {
        if (x < clipx + 5) {
            clipx = max(0, x - (viewport_cols / 2));
            clipxmax = clipx + (viewport_cols - 1);
        } else if (x > clipxmax - 5) {
            clipxmax = min(COLNO - 1, x + (viewport_cols / 2));
            clipx = clipxmax - (viewport_cols - 1);
        }
    } else {
        clipx = 0;
        clipxmax = COLNO - 1;
    }
    if (viewport_rows < ROWNO) {
        if (y < clipy + 5) {
            clipy = max(0, y - (viewport_rows / 2));
            clipymax = clipy + (viewport_rows - 1);
        } else if (y > clipymax - 5) {
            clipymax = min(ROWNO, y + (viewport_rows / 2));
            clipy = clipymax - (viewport_rows - 1);
        }
    } else {
        clipy = 0;
        clipymax = ROWNO - 1;
    }
    if (clipx != oldx || clipy != oldy) {
        if (on_level(&u.uz0, &u.uz) && !gp.program_state.restoring)
            /* (void) doredraw(); */
            vesa_redrawmap();
    }
}

static void
vesa_redrawmap(void)
{
    unsigned y_top = TOP_MAP_ROW * vesa_char_height;
    unsigned y_bottom = vesa_y_res - 5 * vesa_char_height;
    unsigned x, y, cx, cy, py /*, px */ ;
    /* unsigned long color; */
    unsigned long offset = y_top * (unsigned long) vesa_scan_line;
    unsigned char *p_row = NULL;
    unsigned p_row_width;

    /*
     * The map is drawn in pixel-row order (pixel row 0, then row 1, etc.),
     * rather than cell by cell, to minimize calls to vesa_SetWindow.
     */
    if (iflags.traditional_view) {
        /* Text mode */
        y = y_top;
        for (cy = clipy; cy <= (unsigned) clipymax && cy < ROWNO; ++cy) {
            struct VesaCharacter t_row[COLNO];
            for (cx = clipx; cx <= (unsigned) clipxmax && cx < COLNO; ++cx) {
                t_row[cx].chr = map[cy][cx].ch;
                t_row[cx].colour = map[cy][cx].attr;
            }
            vesa_WriteTextRow(0, y, t_row + clipx, cx - clipx);
            x = (cx - clipx) * vesa_char_width;
            if (x < vesa_x_res) {
                vesa_FillRect(x, y, vesa_x_res - x, vesa_char_height, BACKGROUND_VESA_COLOR);
            }
            y += vesa_char_height;
        }
    } else if (iflags.over_view) {
        /* Overview mode */
        const unsigned char *tile;

        p_row_width = vesa_oview_width * vesa_pixel_bytes;
        y = y_top;
        for (cy = 0; cy < ROWNO; ++cy) {
            for (py = 0; py < vesa_oview_height; ++py) {
                for (cx = 0; cx < COLNO; ++cx) {
                    tile = vesa_oview_tiles[map[cy][cx].tileidx];
                    vesa_WritePixelRow(offset + p_row_width * cx, tile + p_row_width * py, p_row_width);
                }
                x = COLNO * vesa_oview_width;
                if (x < vesa_x_res) {
                    vesa_FillRect(x, y, vesa_x_res - x, 1, BACKGROUND_VESA_COLOR);
                }
                offset += vesa_scan_line;
                ++y;
            }
        }
    } else {
        /* Normal tiled mode */
        const unsigned char *tile;

        p_row_width = iflags.wc_tile_width * vesa_pixel_bytes;
        y = y_top;
        for (cy = clipy; cy <= (unsigned) clipymax && cy < ROWNO; ++cy) {
            for (py = 0; py < (unsigned) iflags.wc_tile_height; ++py) {
                for (cx = clipx; cx <= (unsigned) clipxmax && cx < COLNO; ++cx) {
                    tile = vesa_tiles[map[cy][cx].tileidx];
                    vesa_WritePixelRow(offset + p_row_width * (cx - clipx), tile + p_row_width * py, p_row_width);
                }
                x = (cx - clipx) * iflags.wc_tile_width;
                if (x < vesa_x_res) {
                    vesa_FillRect(x, y, vesa_x_res - x, 1, BACKGROUND_VESA_COLOR);
                }
                offset += vesa_scan_line;
                ++y;
            }
        }
    }
    /* Loops leave y as the start of any remaining unfilled space */
    if (y < y_bottom) {
        vesa_FillRect(0, y, vesa_x_res, y_bottom - y, BACKGROUND_VESA_COLOR);
    }

    free(p_row);
}
#endif /* USE_TILES && CLIPPING */

void
vesa_userpan(enum vga_pan_direction pan)
{
    /* pline("Into userpan"); */
    if (iflags.over_view || iflags.traditional_view)
        return;

    switch (pan) {
    case pan_left:
        if (viewport_cols < COLNO) {
            clipxmax = clipx - 1;
            clipx = clipxmax - (viewport_cols - 1);
            if (clipx < 0) {
                clipx = 0;
                clipxmax = viewport_cols - 1;
            }
        }
        break;

    case pan_right:
        if (viewport_cols < COLNO) {
            clipx = clipxmax + 1;
            clipxmax = clipx + (viewport_cols - 1);
            if (clipxmax > COLNO - 1) {
                clipxmax = COLNO - 1;
                clipx = clipxmax - (viewport_cols - 1);
            }
        }
        break;

    case pan_up:
        if (viewport_rows < ROWNO) {
            clipymax = clipy - 1;
            clipy = clipymax - (viewport_rows - 1);
            if (clipy < 0) {
                clipy = 0;
                clipymax = viewport_rows - 1;
            }
        }
        break;

    case pan_down:
        if (viewport_rows < ROWNO) {
            clipy = clipymax + 1;
            clipymax = clipy + (viewport_rows - 1);
            if (clipymax > ROWNO - 1) {
                clipymax = ROWNO - 1;
                clipy = clipymax - (viewport_rows - 1);
            }
        }
        break;
    }

    vesa_refresh();
}

void
vesa_overview(boolean on)
{
    /* vesa_HideCursor(); */
    if (on) {
        iflags.over_view = TRUE;
        clipx = 0;
        clipxmax = COLNO - 1;
        clipy = 0;
        clipymax = ROWNO - 1;
    } else {
        iflags.over_view = FALSE;
        if (viewport_cols < COLNO) {
            clipx = max(0, (curcol - viewport_cols / 2));
            if (clipx > ((COLNO - 1) - viewport_cols))
                clipx = (COLNO - 1) - viewport_cols;
            clipxmax = clipx + (viewport_cols - 1);
        } else {
            clipx = 0;
            clipxmax = COLNO - 1;
        }
        if (viewport_rows < ROWNO) {
            clipy = max(0, (currow - viewport_rows / 2));
            if (clipy > ((ROWNO - 1) - viewport_rows))
                clipy = (ROWNO - 1) - viewport_rows;
            clipymax = clipy + (viewport_rows - 1);
        } else {
            clipy = 0;
            clipymax = ROWNO - 1;
        }
    }
}

void
vesa_traditional(boolean on)
{
    /* vesa_HideCursor(); */
    if (on) {
        /* switch_symbols(FALSE); */
        iflags.traditional_view = TRUE;
        clipx = 0;
        clipxmax = COLNO - 1;
        clipy = 0;
        clipymax = ROWNO - 1;
    } else {
        iflags.traditional_view = FALSE;
        if (!iflags.over_view) {
            if (viewport_cols < COLNO) {
                clipx = max(0, (curcol - viewport_cols / 2));
                if (clipx > ((COLNO - 1) - viewport_cols))
                    clipx = (COLNO - 1) - viewport_cols;
                clipxmax = clipx + (viewport_cols - 1);
            } else {
                clipx = 0;
                clipxmax = COLNO - 1;
            }
            if (viewport_rows < ROWNO) {
                clipy = max(0, (currow - viewport_rows / 2));
                if (clipy > ((ROWNO - 1) - viewport_rows))
                    clipy = (ROWNO - 1) - viewport_rows;
                clipymax = clipy + (viewport_rows - 1);
            } else {
                clipy = 0;
                clipymax = ROWNO - 1;
            }
        }
    }
}

void
vesa_refresh(void)
{
    positionbar();
    vesa_redrawmap();
    vesa_DrawCursor();
}

#if 0
static void
decal_packed(const struct TileImage *gp, unsigned special)
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
#endif

DISABLE_WARNING_FORMAT_NONLITERAL

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
    static boolean inited = FALSE;
    const struct Pixel *paletteptr;
    unsigned i;
    unsigned num_pixels, num_oview_pixels;
    const char *tile_file;
    const char *font_name;
    int tilefailure = 0;

    if (inited) return;
    inited = TRUE;

#ifdef USE_TILES
    /*
     * Attempt to open the required tile files. If we can't
     * don't perform the video mode switch, use TTY code instead.
     *
     */
    tile_file = iflags.wc_tile_file;
    if (tile_file == NULL || *tile_file == '\0') {
        tile_file = "nhtiles.bmp";
    }
    if (!read_tiles(tile_file, vesa_pixel_size > 8))
        tilefailure |= 1;
    if (vesa_pixel_size == 8 && get_palette() == NULL)
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
        /* clear_screen() */ /* not vesa_clear_screen() */
        return;
    }
#endif

    vesa_mode = 0xFFFF; /* might want an 8 bit mode after loading tiles */
    vesa_detect();
    if (vesa_mode == 0xFFFF || (vesa_pixel_size == 8 && get_palette() == NULL)) {
        raw_printf("%s (%d)", "Reverting to TTY mode, no VESA mode available.",
                   tilefailure);
        wait_synch();
        iflags.usevga = 0;
        iflags.tile_view = FALSE;
        iflags.over_view = FALSE;
        CO = 80;
        LI = 25;
        /* clear_screen() */ /* not vesa_clear_screen() */
        return;
    }

    vesa_SwitchMode(vesa_mode);
    vesa_SetViewPort();
    windowprocs.win_cliparound = vesa_cliparound;
#ifdef ENHANCED_SYMBOLS
    if (vesa_pixel_size > 8) {
        windowprocs.wincap2 |= WC2_U_24BITCOLOR;
    }
#endif
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
    clipxmax = clipx + (viewport_cols - 1);
    clipy = 0;
    clipymax = clipy + (viewport_rows - 1);

    /* Set the size of the tiles for the overview mode */
    vesa_oview_width = vesa_x_res / COLNO;
    if (vesa_oview_width > (unsigned) iflags.wc_tile_width) {
        vesa_oview_width = (unsigned) iflags.wc_tile_width;
    }
    vesa_oview_height = (vesa_y_res - (TOP_MAP_ROW + 4) * vesa_char_height)
                      / ROWNO;
    if (vesa_oview_height > (unsigned) iflags.wc_tile_height) {
        vesa_oview_height = (unsigned) iflags.wc_tile_height;
    }

    /* Load a font of size appropriate to the screen size */
    if (vesa_x_res >= 1280 && vesa_y_res >= 960)
        font_name = "ter-u32b.psf";
    else if (vesa_x_res >= 1120 && vesa_y_res >= 840)
        font_name = "ter-u28b.psf";
    else if (vesa_x_res >= 960 && vesa_y_res >= 720)
        font_name = "ter-u24b.psf";
    else if (vesa_x_res >= 880 && vesa_y_res >= 660)
        font_name = "ter-u22b.psf";
    else if (vesa_x_res >= 800 && vesa_y_res >= 600)
        font_name = "ter-u20b.psf";
    else if (vesa_x_res >= 720 && vesa_y_res >= 540)
        font_name = "ter-u18b.psf";
    else
        font_name = "ter-u16v.psf";
    if (iflags.wc_font_map != NULL && iflags.wc_font_map[0] != '\0')
        font_name = iflags.wc_font_map;
    free_font(vesa_font);
    vesa_font = load_font(font_name);
    /* if load_font fails, vesa_font is NULL and we'll fall back to the font
       defined in ROM */
    if (vesa_font != NULL) {
        vesa_char_width = vesa_font->width;
        vesa_char_height = vesa_font->height;
    } else {
        /* Use the map font size to set the font size */
        /* Supported sizes are 8x16, 16x32, 24x48 and 32x64 */
        vesa_char_height = iflags.wc_fontsiz_map;
        if (vesa_char_height <= 0 || vesa_char_height > vesa_y_res / 30) {
            vesa_char_height = vesa_y_res / 30;
        }
        if (vesa_char_height < 32) {
            vesa_char_height = 16;
        } else if (vesa_char_height < 48) {
            vesa_char_height = 32;
        } else if (vesa_char_height < 64) {
            vesa_char_height = 48;
        } else {
            vesa_char_height = 64;
        }
        vesa_char_width = vesa_char_height / 2;
    }

    /* Process tiles for the current video mode */
    vesa_tiles = (unsigned char **) alloc(total_tiles_used * sizeof(void *));
    vesa_oview_tiles = (unsigned char **) alloc(total_tiles_used * sizeof(void *));
    num_pixels = iflags.wc_tile_width * iflags.wc_tile_height;
    num_oview_pixels = vesa_oview_width * vesa_oview_height;
    set_tile_type(vesa_pixel_size > 8);
    for (i = 0; i < (unsigned) total_tiles_used; ++i) {
        const struct TileImage *tile = get_tile(i);
        struct TileImage *ov_tile = stretch_tile(tile, vesa_oview_width, vesa_oview_height);
        unsigned j;
        unsigned char *t_img = (unsigned char *) alloc(num_pixels * vesa_pixel_bytes);
        unsigned char *ot_img = (unsigned char *) alloc(num_oview_pixels * vesa_pixel_bytes);
        vesa_tiles[i] = t_img;
        vesa_oview_tiles[i] = ot_img;
        switch (vesa_pixel_bytes) {
        case 1:
            memcpy(t_img, tile->indexes, num_pixels);
            memcpy(ot_img, ov_tile->indexes, num_oview_pixels);
            break;

        case 2:
            for (j = 0; j < num_pixels; ++j) {
                ((uint16_t *)t_img)[j] = vesa_MakeColor(tile->pixels[j]);
            }
            for (j = 0; j < num_oview_pixels; ++j) {
                ((uint16_t *)ot_img)[j] = vesa_MakeColor(ov_tile->pixels[j]);
            }
            break;

        case 3:
            for (j = 0; j < num_pixels; ++j) {
                unsigned long color = vesa_MakeColor(tile->pixels[j]);
                t_img[3*j + 0] =  color        & 0xFF;
                t_img[3*j + 1] = (color >>  8) & 0xFF;
                t_img[3*j + 2] = (color >> 16) & 0xFF;
            }
            for (j = 0; j < num_oview_pixels; ++j) {
                unsigned long color = vesa_MakeColor(ov_tile->pixels[j]);
                ot_img[3*j + 0] =  color        & 0xFF;
                ot_img[3*j + 1] = (color >>  8) & 0xFF;
                ot_img[3*j + 2] = (color >> 16) & 0xFF;
            }
            break;

        case 4:
            for (j = 0; j < num_pixels; ++j) {
                ((uint32_t *)t_img)[j] = vesa_MakeColor(tile->pixels[j]);
            }
            for (j = 0; j < num_oview_pixels; ++j) {
                ((uint32_t *)ot_img)[j] = vesa_MakeColor(ov_tile->pixels[j]);
            }
            break;
        }
        free_tile(ov_tile);
    }
    free_tiles();
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* Set the size of the map viewport */
static void
vesa_SetViewPort(void)
{
    unsigned y_reserved = (TOP_MAP_ROW + 5) * vesa_char_height;
    unsigned y_map = vesa_y_res - y_reserved;

    viewport_cols = vesa_x_res / iflags.wc_tile_width;
    viewport_rows = y_map / iflags.wc_tile_height;
    if (viewport_cols > COLNO) viewport_cols = COLNO;
    if (viewport_rows > ROWNO) viewport_rows = ROWNO;
}

/*
 * Switches modes of the video card.
 *
 * If mode == MODETEXT (0x03), then the card is placed into text
 * mode.  Otherwise, the card is placed in the mode selected by
 * vesa_detect.
 *
 */
static void
vesa_SwitchMode(unsigned mode)
{
    __dpmi_regs regs;

    if (mode == MODETEXT) {
        iflags.grmode = 0;
        memset(&regs, 0, sizeof(regs));
        regs.x.ax = mode;
        (void) __dpmi_int(VIDEO_BIOS, &regs);
#ifdef SIMULATE_CURSOR
        free(undercursor);
        undercursor = NULL;
#endif
    } else if (mode >= 0x100) {
        iflags.grmode = 1;
        memset(&regs, 0, sizeof(regs));
        regs.x.ax = 0x4F02;
        regs.x.bx = mode;
        (void) __dpmi_int(VIDEO_BIOS, &regs);
        /* Record that the window position is unknown */
        vesa_win_pos[0] = 0xFFFFFFFF;
        vesa_win_pos[1] = 0xFFFFFFFF;
    } else {
        iflags.grmode = 0; /* force text mode for error msg */
        memset(&regs, 0, sizeof(regs));
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
    int i;

    for (i = 0; i < total_tiles_used; ++i) {
        free(vesa_tiles[i]);
        free(vesa_oview_tiles[i]);
    }
    free(vesa_tiles);
    free(vesa_oview_tiles);
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
static unsigned char __far *
vesa_FontPtrs(void)
{
    USHORT __far *tmp;
    unsigned char __far *retval;
    USHORT fseg, foff;
    tmp = (USHORT __far *) MK_PTR(((USHORT) FONT_PTR_SEGMENT),
                                  ((USHORT) FONT_PTR_OFFSET));
    foff = READ_ABSOLUTE_WORD(tmp);
    ++tmp;
    fseg = READ_ABSOLUTE_WORD(tmp);
    retval = (unsigned char __far *) MK_PTR(fseg, foff);
    return retval;
}

/*
 * This will verify the existance of a VGA adapter on the machine.
 * Video function call 0x4F00 returns 0x004F in AX if successful, and
 * returns a VbeInfoBlock describing the features of the VESA BIOS.
 */
int
vesa_detect(void)
{
    int vbe_info_sel = -1; /* custodial */
    int vbe_info_seg;
    struct VbeInfoBlock vbe_info;
    __dpmi_regs regs;
    unsigned long mode_addr;
    struct ModeInfoBlock mode_info;
    const char *mode_str;

    vbe_info_seg = __dpmi_allocate_dos_memory(
            (sizeof(vbe_info) + 15) / 16,
            &vbe_info_sel);
    if (vbe_info_seg < 0) goto error;

    /* Request VBE 2.0 information if it is available */
    memset(&vbe_info, 0, sizeof(vbe_info));
    memcpy(vbe_info.VbeSignature, "VBE2", 4);
    dosmemput(&vbe_info, sizeof(vbe_info), vbe_info_seg * 16L);

    /* Request VESA BIOS information */
    memset(&regs, 0, sizeof(regs));
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

    /* Allow the user to select a specific mode */
    mode_str = nh_getenv("NH_DISPLAY_MODE");
    if (mode_str != NULL) {
        char *end;
        unsigned long num = strtoul(mode_str, &end, 16);
        if (*end == '\0') {
            /* Can we select this mode? */
            if (vesa_GetModeInfo(num, &mode_info)
            &&  mode_info.XResolution >= 640
            &&  mode_info.YResolution >= 480
            &&  mode_info.BitsPerPixel >= 8) {
                vesa_mode = num & 0x47FF;
            }
        }
        if (vesa_mode == 0xFFFF)
            mode_str = NULL;
    }

    /* Scan the mode list for an acceptable mode */
    /* Choose the widest bit-width, even if the tile set can handle 8 bits,
       so that Unicode symbols can display in their colors */
#ifndef ENHANCED_SYMBOLS
    if (get_palette() != NULL && vesa_mode == 0xFFFF)
        vesa_mode = vesa_FindMode(mode_addr,  8);
#endif
    if (vesa_mode == 0xFFFF)
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
    vesa_x_center = 0;
    vesa_y_center = 0;
    vesa_scan_line = mode_info.BytesPerScanLine;
    vesa_win_size = mode_info.WinSize * 1024L;
    vesa_win_gran = mode_info.WinGranularity * 1024L;
    vesa_pixel_size = mode_info.BitsPerPixel;
    vesa_pixel_bytes = (vesa_pixel_size + 7) / 8;
    if (vbe_info.VbeVersion >= 0x0300) {
        if (mode_info.ModeAttributes & 0x80) {
            vesa_red_pos = mode_info.LinRedFieldPosition;
            vesa_red_shift = 8 - mode_info.LinRedMaskSize;
            vesa_green_pos = mode_info.LinGreenFieldPosition;
            vesa_green_shift = 8 - mode_info.LinGreenMaskSize;
            vesa_blue_pos = mode_info.LinBlueFieldPosition;
            vesa_blue_shift = 8 - mode_info.LinBlueMaskSize;
        } else {
            vesa_red_pos = mode_info.RedFieldPosition;
            vesa_red_shift = 8 - mode_info.RedMaskSize;
            vesa_green_pos = mode_info.GreenFieldPosition;
            vesa_green_shift = 8 - mode_info.GreenMaskSize;
            vesa_blue_pos = mode_info.BlueFieldPosition;
            vesa_blue_shift = 8 - mode_info.BlueMaskSize;
        }
    } else {
        switch (vesa_pixel_size) {
        case 15:
            vesa_blue_pos = 0;
            vesa_blue_shift = 3;
            vesa_green_pos = 5;
            vesa_green_shift = 3;
            vesa_red_pos = 10;
            vesa_red_shift = 3;
            break;

        case 16:
            vesa_blue_pos = 0;
            vesa_blue_shift = 3;
            vesa_green_pos = 5;
            vesa_green_shift = 2;
            vesa_red_pos = 11;
            vesa_red_shift = 3;
            break;

        case 24:
        case 32:
            vesa_blue_pos = 0;
            vesa_blue_shift = 0;
            vesa_green_pos = 8;
            vesa_green_shift = 0;
            vesa_red_pos = 16;
            vesa_red_shift = 0;
            break;
        }
    }
    vesa_win_func = mode_info.WinFuncPtr;
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

    /* Configure a linear frame buffer if we have it */
    if ((mode_info.ModeAttributes & 0x80) != 0
        && (mode_str == NULL || (vesa_mode & 0x4000) != 0)) {
        unsigned sel = vesa_segment;
        unsigned win_size = mode_info.BytesPerScanLine * mode_info.YResolution;
        unsigned addr = vesa_map_frame_buffer(mode_info.PhysBasePtr, win_size);
        if (sel == 0) {
            sel = __dpmi_allocate_ldt_descriptors(1);
        }
        if (addr != 0) {
            vesa_mode |= 0x4000;
            vesa_segment = sel;
            __dpmi_set_segment_base_address(sel, addr);
            __dpmi_set_segment_limit(sel, (win_size - 1) | 0xFFF);
        } else {
            __dpmi_free_ldt_descriptor(sel);
            vesa_segment = 0;
        }
    }

    __dpmi_free_dos_memory(vbe_info_sel);
    return TRUE;

error:
    if (vbe_info_sel != -1) __dpmi_free_dos_memory(vbe_info_sel);
    return FALSE;
}

static unsigned
vesa_FindMode(unsigned long mode_addr, unsigned bits)
{
    unsigned selected_mode;
    struct ModeInfoBlock mode_info0, mode_info;
    unsigned model = (bits == 8) ? 4 : 6;

    if (iflags.wc_video_width < 640) {
        iflags.wc_video_width = 640;
    }
    if (iflags.wc_video_height < 480) {
        iflags.wc_video_height = 480;
    }

    memset(&mode_info, 0, sizeof(mode_info));
    selected_mode = 0xFFFF;
    while (1) {
        unsigned mode = _farpeekw(_dos_ds, mode_addr);
        if (mode == 0xFFFF) break;
        mode_addr += 2;

        /* Query the mode info; skip to next if not in fact supported */
        if (!vesa_GetModeInfo(mode, &mode_info0)) continue;

        /* Check that the mode is acceptable */
        if (mode_info0.XResolution < iflags.wc_video_width) continue;
        if (mode_info0.YResolution < iflags.wc_video_height) continue;
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
vesa_WriteChar(uint32 chr, int col, int row, uint32 colour)
{
    int pixx, pixy;

    /* min() protects from callers */
    pixx = min(col, (CO - 1)) * vesa_char_width;
    pixy = min(row, (LI - 1)) * vesa_char_height;

    pixx += vesa_x_center;
    pixy += vesa_y_center;

    vesa_WriteCharXY(chr, pixx, pixy, colour);
}

/*
 * As vesa_WriteChar, but specify coordinates by pixel and allow
 * transparency
 */
static void
vesa_WriteCharXY(uint32 chr, int pixx, int pixy, uint32 colour)
{
    /* Flush if cache is full or if not contiguous to the last character */
    if (chr_cache_size >= SIZE(chr_cache)) {
        vesa_flush_text();
    }
    if (chr_cache_size != 0 && chr_cache_lastx + vesa_char_width != (unsigned) pixx) {
        vesa_flush_text();
    }
    if (chr_cache_size != 0 && chr_cache_pixy != (unsigned) pixy) {
        vesa_flush_text();
    }
    /* Add to cache and write later */
    if (chr_cache_size == 0) {
        chr_cache_pixx = pixx;
        chr_cache_pixy = pixy;
    }
    chr_cache_lastx = pixx;
    chr_cache[chr_cache_size].chr = chr;
    chr_cache[chr_cache_size].colour = colour;
    ++chr_cache_size;
}

/*
 * Draw a character with a transparent background
 * Don't bother cacheing; only the position bar and the cursor use this
 */
static void
vesa_WriteCharTransparent(int chr, int pixx, int pixy, int colour)
{
    int px, py;

    for (py = 0; py < (int) vesa_char_height; ++py) {
        for (px = 0; px < (int) vesa_char_width; ++px) {
            if (vesa_GetCharPixel(chr, px, py)) {
                vesa_WritePixel(pixx + px, pixy + py, colour + FIRST_TEXT_COLOR);
            }
        }
    }
}

void
vesa_flush_text(void)
{
    if (chr_cache_size == 0) return;

    vesa_WriteTextRow(chr_cache_pixx, chr_cache_pixy, chr_cache, chr_cache_size);
    chr_cache_size = 0;
}

static void
vesa_WriteTextRow(int pixx, int pixy, struct VesaCharacter const *t_row,
                  unsigned t_row_width)
{
    int x, px, py;
    unsigned i;
    unsigned p_row_width = t_row_width * vesa_char_width * vesa_pixel_bytes;
    unsigned char *p_row = (unsigned char *) alloc(p_row_width);
    unsigned long offset = pixy * (unsigned long)vesa_scan_line + pixx * vesa_pixel_bytes;
    unsigned char fg[4], bg[4];

    /* Preprocess the background color */
    if (vesa_pixel_bytes == 1) {
        bg[0] = BACKGROUND_VESA_COLOR;
    } else {
        unsigned long pix = vesa_palette[BACKGROUND_VESA_COLOR];
        bg[0] =  pix        & 0xFF;
        bg[1] = (pix >>  8) & 0xFF;
        bg[2] = (pix >> 16) & 0xFF;
        bg[3] = (pix >> 24) & 0xFF;
    }

    /* First loop: draw one raster line of all row entries */
    for (py = 0; py < (int) vesa_char_height; ++py) {
        /* Second loop: draw one raster line of one character */
        x = 0;
        for (i = 0; i < t_row_width; ++i) {
            uint32 chr = t_row[i].chr;
            uint32 colour = t_row[i].colour;
            /* Preprocess the foreground color */
            if (colour & 0x80000000) {
                fg[0] =  colour        & 0xFF;
                fg[1] = (colour >>  8) & 0xFF;
                fg[2] = (colour >> 16) & 0xFF;
                fg[3] = 0;
            } else if (vesa_pixel_bytes == 1) {
                fg[0] = colour + FIRST_TEXT_COLOR;
            } else {
                unsigned long pix = vesa_palette[colour + FIRST_TEXT_COLOR];
                fg[0] =  pix        & 0xFF;
                fg[1] = (pix >>  8) & 0xFF;
                fg[2] = (pix >> 16) & 0xFF;
                fg[3] = (pix >> 24) & 0xFF;
            }
            /* Third loop: draw eight pixels */
            for (px = 0; px < (int) vesa_char_width; px += 8) {
                /* Fourth loop: draw one pixel */
                int px2;
                unsigned char fnt = vesa_GetCharPixelRow(chr, px, py);
                int l = vesa_char_width - px;
                if (l > 8) {
                    l = 8;
                }
                for (px2 = 0; px2 < l; ++px2) {
                    if (fnt & 0x80) {
                        memcpy(p_row + x, fg, vesa_pixel_bytes);
                    } else {
                        memcpy(p_row + x, bg, vesa_pixel_bytes);
                    }
                    x += vesa_pixel_bytes;
                    fnt <<= 1;
                }
            }
        }
        vesa_WritePixelRow(offset, p_row, p_row_width);
        offset += vesa_scan_line;
    }
    free(p_row);
}

static boolean
vesa_GetCharPixel(int ch, unsigned x, unsigned y)
{
    unsigned x2;
    unsigned char fnt;

    x2 = x % 8;

    fnt = vesa_GetCharPixelRow(ch, x, y);
    return (fnt & (0x80 >> x2)) != 0;
}

static unsigned char
vesa_GetCharPixelRow(uint32 ch, unsigned x, unsigned y)
{
    unsigned fnt_width;
    unsigned x1;
    unsigned char fnt;
    size_t offset;

    if (x >= vesa_char_width) return 0;
    if (y >= vesa_char_height) return 0;

    fnt_width = (vesa_char_width + 7) / 8;
    x1 = x / 8;

    if (vesa_font != NULL) {
        const unsigned char *fp;

        offset = y * fnt_width + x1;
        fp = get_font_glyph(vesa_font, ch, SYMHANDLING(H_UTF8));
        fnt = fp[offset];
    } else {
        const unsigned char __far *fp;

        if (255 < ch) return 0;
        offset = (ch * vesa_char_height + y) * fnt_width + x1;
        fp = font;
        fnt = READ_ABSOLUTE((fp + offset));

        if (vesa_char_width != 8) {
            unsigned long fnt2 = fnt;
            unsigned width = vesa_char_width;
            if (width % 3 == 0) {
                fnt2 = vesa_TriplePixels(fnt2);
                width /= 3;
            }
            while (width > 8) {
                fnt2 = vesa_DoublePixels(fnt2);
                width /= 2;
            }
            fnt2 <<= 32 - vesa_char_width;
            fnt = (unsigned char)(fnt2 >> (24 - 8 * x1));
        }
    }
    return fnt;
}

/* Scale font pixels horizontally */
static unsigned long
vesa_DoublePixels(unsigned long fnt)
{
    static const unsigned char double_bits[] = {
        0x00, 0x03, 0x0C, 0x0F,
        0x30, 0x33, 0x3C, 0x3F,
        0xC0, 0xC3, 0xCC, 0xCF,
        0xF0, 0xF3, 0xFC, 0xFF
    };
    unsigned i;
    unsigned long fnt2;

    fnt2 = 0;
    for (i = 0; i < 16; i += 4) {
        unsigned long b4 = (fnt >> i) & 0xF;
        fnt2 |= (unsigned long)double_bits[b4] << (i * 2);
    }
    return fnt2;
}

static unsigned long
vesa_TriplePixels(unsigned long fnt)
{
    static const unsigned short triple_bits[] = {
        00000, 00007, 00070, 00077,
        00700, 00707, 00770, 00777,
        07000, 07007, 07070, 07077,
        07700, 07707, 07770, 07777
    };
    unsigned i;
    unsigned long fnt2;

    fnt2 = 0;
    for (i = 0; i < 11; i += 4) {
        unsigned long b4 = (fnt >> i) & 0xF;
        fnt2 |= (unsigned long)triple_bits[b4] << (i * 3);
    }
    return fnt2;
}

/*
 * This is the routine that displays a high-res "cell" pointed to by 'gp'
 * at the desired location (col,row).
 *
 * Note: (col,row) in this case refer to the coordinate location in
 * NetHack character grid terms, relative to the map viewport,
 * not the x,y pixel location.
 *
 */
static void
vesa_DisplayCell(int tilenum, int col, int row)
{
    unsigned char const *tile;
    unsigned t_width, t_height;
    unsigned char const *tptr;
    int /* px, */ py, pixx, pixy;
    unsigned long offset;
    unsigned p_row_width;

    if (iflags.over_view) {
        tile = vesa_oview_tiles[tilenum];
        t_width = vesa_oview_width;
        t_height = vesa_oview_height;
    } else {
        tile = vesa_tiles[tilenum];
        t_width = iflags.wc_tile_width;
        t_height = iflags.wc_tile_height;
    }

    p_row_width = t_width * vesa_pixel_bytes;

    pixx = col * t_width;
    pixy = row * t_height + TOP_MAP_ROW * vesa_char_height;
    pixx += vesa_x_center;
    pixy += vesa_y_center;
    offset = pixy * (unsigned long)vesa_scan_line + pixx * vesa_pixel_bytes;
    tptr = tile;

    for (py = 0; py < (int) t_height; ++py) {
        vesa_WritePixelRow(offset, tptr, p_row_width);
        offset += vesa_scan_line;
        tptr += p_row_width;
    }
}

/*
 * Write the character string pointed to by 's', whose maximum length
 * is 'len' at location (x,y) using the 'colour' colour.
 *
 */
static void
vesa_WriteStr(const char *s, int len, int col, int row, int colour)
{
    const unsigned char *us;
    int i = 0;

    /* protection from callers */
    if (row > (LI - 1))
        return;

    i = 0;
    us = (const unsigned char *) s;
    while ((*us != 0) && (i < len) && (col < (CO - 1))) {
        vesa_WriteChar(*us, col, row, colour);
        ++us;
        ++i;
        ++col;
    }
}

/*
 * Initialize the VGA palette with the desired colours. This
 * must be a series of 720 bytes for use with a card in 256
 * colour mode. The first 240 palette entries are used by the
 * tile set; the last 16 are reserved for text.
 *
 */
static boolean
vesa_SetPalette(const struct Pixel *palette)
{
    if (vesa_pixel_size == 8) {
        return vesa_SetHardPalette(palette);
    } else {
        return vesa_SetSoftPalette(palette);
    }
}

static boolean
vesa_SetHardPalette(const struct Pixel *palette)
{
    int palette_sel = -1; /* custodial */
    int palette_seg;
    unsigned char p2[1024];
    unsigned i, shift;
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

    /* First, try the VESA palette function */
    /* Set the tile set and text colors */
#ifdef USE_TILES
    for (i = 0; i < FIRST_TEXT_COLOR; ++i) {
        p2[i*4 + 0] = palette[i].b >> shift;
        p2[i*4 + 1] = palette[i].g >> shift;
        p2[i*4 + 2] = palette[i].r >> shift;
    }
#endif
    for (i = FIRST_TEXT_COLOR; i < 256; ++i) {
        p2[i*4 + 0] = defpalette[i-FIRST_TEXT_COLOR].b >> shift;
        p2[i*4 + 1] = defpalette[i-FIRST_TEXT_COLOR].g >> shift;
        p2[i*4 + 2] = defpalette[i-FIRST_TEXT_COLOR].r >> shift;
    }

    /* Call the BIOS */
    dosmemput(p2, 256*4, palette_seg*16L);
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = 0x4F09;
    regs.h.bl = 0;
    regs.x.cx = 256;
    regs.x.dx = 0;
    regs.x.di = 0;
    regs.x.es = palette_seg;
    (void) __dpmi_int(VIDEO_BIOS, &regs);

    /* If that didn't work, use the original BIOS function */
    if (regs.x.ax != 0x004F) {
        /* Set the tile set and text colors */
#ifdef USE_TILES
        for (i = 0; i < FIRST_TEXT_COLOR; ++i) {
            p2[i*3 + 0] = palette[i].r >> shift;
            p2[i*3 + 1] = palette[i].g >> shift;
            p2[i*3 + 2] = palette[i].b >> shift;
        }
#endif
        for (i = FIRST_TEXT_COLOR; i < 256; ++i) {
            p2[i*3 + 0] = defpalette[i-FIRST_TEXT_COLOR].r >> shift;
            p2[i*3 + 1] = defpalette[i-FIRST_TEXT_COLOR].g >> shift;
            p2[i*3 + 2] = defpalette[i-FIRST_TEXT_COLOR].b >> shift;
        }

        /* Call the BIOS */
        dosmemput(p2, 256*3, palette_seg*16L);
        memset(&regs, 0, sizeof(regs));
        regs.x.ax = 0x1012;
        regs.x.cx = 256;
        regs.x.bx = 0;
        regs.x.dx = 0;
        regs.x.es = palette_seg;
        (void) __dpmi_int(VIDEO_BIOS, &regs);
    }

    __dpmi_free_dos_memory(palette_sel);
    return TRUE;

error:
    if (palette_sel != -1) __dpmi_free_dos_memory(palette_sel);
    return FALSE;
}

static boolean
vesa_SetSoftPalette(const struct Pixel *palette)
{
    const struct Pixel *p;
    unsigned i;

    /* Set the tile set and text colors */
#ifdef USE_TILES
    if (palette != NULL) {
        p = palette;
        for (i = 0; i < FIRST_TEXT_COLOR; ++i) {
            vesa_palette[i] = vesa_MakeColor(*p);
            ++p;
        }
    }
#endif
    p = defpalette;
    for (i = FIRST_TEXT_COLOR; i < 256; ++i) {
        vesa_palette[i] = vesa_MakeColor(*p);
        ++p;
    }
    return TRUE;
}

#ifdef POSITIONBAR

#define PBAR_ROW (LI - 4)
#define PBAR_COLOR_ON 6     /* slate grey background colour of tiles */
#define PBAR_COLOR_OFF 0    /* bluish grey, used in old style only */
#define PBAR_COLOR_STAIRS CLR_BROWN /* brown */
#define PBAR_COLOR_HERO CLR_WHITE  /* creamy white */

static unsigned char pbar[COLNO];

void
vesa_update_positionbar(char *posbar)
{
    unsigned char *p = pbar;
    if (posbar)
        while (*posbar)
            *p++ = *posbar++;
    *p = 0;
}

static void
positionbar(void)
{
    unsigned char *posbar = pbar;
    int feature, ucol;

    int startk, stopk;
    boolean nowhere = FALSE;
    int pixx, col;
    int pixy = (PBAR_ROW * vesa_char_height);
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
    startk = clipx * vesa_x_res / COLNO;
    stopk = (clipxmax + 1) * vesa_x_res / COLNO;
#ifdef OLD_STYLE
    vesa_FillRect(0, pixy, startk, vesa_char_height,
                  PBAR_COLOR_OFF + FIRST_TEXT_COLOR);
    vesa_FillRect(startk, pixy, stopk - startk, vesa_char_height,
                  PBAR_COLOR_ON + FIRST_TEXT_COLOR);
    vesa_FillRect(stopk, pixy, vesa_x_res - stopk, vesa_char_height,
                  PBAR_COLOR_OFF + FIRST_TEXT_COLOR);
#else
    vesa_FillRect(0, pixy, vesa_x_res, 1, PBAR_COLOR_ON + FIRST_TEXT_COLOR);
    vesa_FillRect(0, pixy + 1, startk, vesa_char_height - 2,
                  BACKGROUND_VESA_COLOR);
    vesa_FillRect(startk, pixy + 1, stopk - startk, vesa_char_height - 2,
                  PBAR_COLOR_ON + FIRST_TEXT_COLOR);
    vesa_FillRect(stopk, pixy + 1, vesa_x_res - stopk, vesa_char_height - 2,
                  BACKGROUND_VESA_COLOR);
    vesa_FillRect(0, pixy + vesa_char_height - 1, vesa_x_res, 1,
                  PBAR_COLOR_ON + FIRST_TEXT_COLOR);
#endif
    ucol = 0;
    if (posbar) {
        while (*posbar != 0) {
            feature = *posbar++;
            col = *posbar++;
            pixx = col * vesa_x_res / COLNO;
            switch (feature) {
            case '>':
                vesa_WriteCharTransparent(feature, pixx, pixy, PBAR_COLOR_STAIRS);
                break;
            case '<':
                vesa_WriteCharTransparent(feature, pixx, pixy, PBAR_COLOR_STAIRS);
                break;
            case '@':
                ucol = col;
                vesa_WriteCharTransparent(feature, pixx, pixy, PBAR_COLOR_HERO);
                break;
            default: /* unanticipated symbols */
                vesa_WriteCharTransparent(feature, pixx, pixy, PBAR_COLOR_STAIRS);
                break;
            }
        }
    }
#ifdef SIMULATE_CURSOR
    if (inmap) {
        tmp = curcol + 1;
        if ((tmp != ucol) && (curcol >= 0))
            vesa_WriteCharTransparent('_', tmp * vesa_x_res / COLNO, pixy,
                                      PBAR_COLOR_HERO);
    }
#endif
    vesa_flush_text();
}

#endif /*POSITIONBAR*/

#ifdef SIMULATE_CURSOR

void
vesa_DrawCursor(void)
{
    static boolean last_inmap = FALSE;
    unsigned x, y, left, top, right, bottom, width, height;
    boolean isrogue = Is_rogue_level(&u.uz);
    boolean halfwidth =
        (isrogue || iflags.over_view || iflags.traditional_view || !inmap);
    int curtyp;

    if (inmap && !last_inmap) {
        vesa_redrawmap();
    }
    last_inmap = inmap;

    if (!cursor_type && inmap)
        return; /* CURSOR_INVIS - nothing to do */
    if (undercursor == NULL) {
        /* size for the greater of one tile or one character */
        unsigned size1 = vesa_char_width * vesa_char_height;
        unsigned size2 = iflags.wc_tile_width * iflags.wc_tile_height;
        undercursor = (unsigned long *) alloc(
                sizeof(undercursor[0]) * max(size1, size2));
    }

    x = min(curcol, (CO - 1)); /* protection from callers */
    y = min(currow, (LI - 1)); /* protection from callers */
    if (!halfwidth
    &&  ((x < (unsigned) clipx) || (x > (unsigned) clipxmax)
      || (y < (unsigned) clipy) || (y > (unsigned) clipymax)))
        return;
    if (inmap) {
        x -= clipx;
        y -= clipy;
    }
    /* convert to pixels */
    if (!inmap || iflags.traditional_view) {
        width = vesa_char_width;
        height = vesa_char_height;
    } else if (iflags.over_view) {
        width = vesa_oview_width;
        height = vesa_oview_height;
    } else {
        width = iflags.wc_tile_width;
        height = iflags.wc_tile_height;
    }
    left = x * width  + vesa_x_center;
    top  = y * height + vesa_y_center;
    if (y >= TOP_MAP_ROW) {
        top -= (height - vesa_char_height) * TOP_MAP_ROW;
    }
    right = left + width - 1;
    bottom = top + height - 1;

    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            undercursor[y * width + x] = vesa_ReadPixel32(left + x, top + y);
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
vesa_HideCursor(void)
{
    unsigned x, y, left, top, width, height;
    boolean isrogue = Is_rogue_level(&u.uz);
    boolean halfwidth =
        (isrogue || iflags.over_view || iflags.traditional_view || !inmap);

    if (!cursor_type && inmap)
        return; /* CURSOR_INVIS - nothing to do */
    if (undercursor == NULL)
        return;

    x = min(curcol, (CO - 1)); /* protection from callers */
    y = min(currow, (LI - 1)); /* protection from callers */
    if (!halfwidth
    &&  ((x < (unsigned) clipx) || (x > (unsigned) clipxmax)
                                || (y < (unsigned) clipy)
                                || (y > (unsigned) clipymax)))
        return;
    if (inmap) {
        x -= clipx;
        y -= clipy;
    }
    /* convert to pixels */
    if (!inmap || iflags.traditional_view) {
        width = vesa_char_width;
        height = vesa_char_height;
    } else if (iflags.over_view) {
        width = vesa_oview_width;
        height = vesa_oview_height;
    } else {
        width = iflags.wc_tile_width;
        height = iflags.wc_tile_height;
    }
    left = x * width  + vesa_x_center;
    top  = y * height + vesa_y_center;
    if (y >= TOP_MAP_ROW) {
        top -= (height - vesa_char_height) * TOP_MAP_ROW;
    }

    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            vesa_WritePixel32(left + x, top + y, undercursor[y * width + x]);
        }
    }
}
#endif /* SIMULATE_CURSOR */
#endif /* SCREEN_VESA */

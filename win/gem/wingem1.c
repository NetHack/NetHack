/* NetHack 5.0	wingem1.c	$NHDT-Date: 1433806613 2015/06/08 23:36:53 $  $NHDT-Branch: master $:$NHDT-Revision: 1.13 $ */
/* Copyright (c) Christian Bressler 1999 	  */
/* NetHack may be freely redistributed.  See license for details. */

#define __TCC_COMPAT__

/* USERDEF callbacks (draw_status, draw_msgline, draw_titel, ...) run on
   the AES per-process supervisor stack u_super[], which is ~1.9KB on
   stock EmuTOS 1.4.  gemlib's regular v_gtext allocates a 2104-byte
   scratch frame (intin[1024] etc.) -- larger than the entire supervisor
   stack -- and overflows into adjacent BSS (corrupting gl_rfull and
   EmuTOS's contrl[]).  FORCE_GEMLIB_UDEF routes v_gtext to
   udef_v_gtext, which uses a 20-byte stack frame plus a static global
   intin buffer. */
#define FORCE_GEMLIB_UDEF

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <e_gem.h>
#include <string.h>

#include "gem_rsc.h"
#include "load_img.h"
#include "gr_rect.h"

/* Provide types needed by wintype.h without pulling in all of hack.h,
   which would conflict with e_gem.h definitions. */
#include <stdint.h>
#define genericptr_t void *
typedef signed char schar;
typedef unsigned char uchar;
typedef int8_t xint8;
typedef int16_t xint16;
typedef int16_t coordxy;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef struct { int color; int attr; } color_attr;
#include "wintype.h"
#undef genericptr_t

#include "wingem.h"

static char nullstr[] = "", md[] = "NetHack", strCancel[] = "Cancel",
            strOk[] = "Ok", strMap[] = "Dungeon", strText[] = "Info";

extern winid WIN_MESSAGE, WIN_MAP, WIN_STATUS, WIN_INVEN;

#define MAXWIN 20
#define ROWNO 21
#define COLNO 80
#define MSGLEN 100

#define MAP_GADGETS                                                        \
    NAME | MOVER | CLOSER | FULLER | LFARROW | RTARROW | UPARROW | DNARROW \
        | VSLIDE | HSLIDE | SIZER
#define DIALOG_MODE AUTO_DIAL | MODAL | NO_ICONIFY

/*
 *  Keyboard translation tables.
 */
#define C(c) (0x1f & (c))
#define M(c) (0x80 | (c))

#define KEYPADLO 0x61
#define KEYPADHI 0x71

#define PADKEYS (KEYPADHI - KEYPADLO + 1)
#define iskeypad(x) (KEYPADLO <= (x) && (x) <= KEYPADHI)

/*
 * Keypad keys are translated to the normal values below.
 * When iflags.BIOS is active, shifted keypad keys are translated to the
 *    shift values below.
 */
static const struct pad {
    char normal, shift, cntrl;
} keypad[PADKEYS] =
    {
      { C('['), 'Q', C('[') }, /* UNDO */
      { '?', '/', '?' },       /* HELP */
      { '(', 'a', '(' },       /* ( */
      { ')', 'w', ')' },       /* ) */
      { '/', '/', '/' },       /* / */
      { C('p'), '$', C('p') }, /* * */
      { 'y', 'Y', C('y') },    /* 7 */
      { 'k', 'K', C('k') },    /* 8 */
      { 'u', 'U', C('u') },    /* 9 */
      { 'h', 'H', C('h') },    /* 4 */
      { '.', '.', '.' },
      { 'l', 'L', C('l') }, /* 6 */
      { 'b', 'B', C('b') }, /* 1 */
      { 'j', 'J', C('j') }, /* 2 */
      { 'n', 'N', C('n') }, /* 3 */
      { 'i', 'I', C('i') }, /* Ins */
      { '.', ':', ':' }     /* Del */
    },
  numpad[PADKEYS] = {
      { C('['), 'Q', C('[') }, /* UNDO */
      { '?', '/', '?' },       /* HELP */
      { '(', 'a', '(' },       /* ( */
      { ')', 'w', ')' },       /* ) */
      { '/', '/', '/' },       /* / */
      { C('p'), '$', C('p') }, /* * */
      { '7', M('7'), '7' },    /* 7 */
      { '8', M('8'), '8' },    /* 8 */
      { '9', M('9'), '9' },    /* 9 */
      { '4', M('4'), '4' },    /* 4 */
      { '.', '.', '.' },       /* 5 */
      { '6', M('6'), '6' },    /* 6 */
      { '1', M('1'), '1' },    /* 1 */
      { '2', M('2'), '2' },    /* 2 */
      { '3', M('3'), '3' },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
  };

#define TBUFSZ 300
#define BUFSZ 256
short mar_set_tile_mode(short);                   /* forward decl */
extern long yn_number;                           /* from decl.c (long in 3.7) */
extern char mapped_menu_cmds[];                /* from options.c */
extern short mar_iflags_numpad(void);            /* from wingem.c */
extern void Gem_raw_print(const char *);       /* from wingem.c */
extern short mar_hp_query(void);                 /* from wingem.c */
extern short mar_get_msg_history(void);          /* from wingem.c */
extern short mar_get_msg_visible(void);          /* from wingem.c */
extern void mar_get_font(short, char **, short *); /* from wingem.c */

/* Find the VDI pen whose current color is closest to the given RGB (0-1000).
   Useful because the tile palette remaps standard VDI color indices.
   When skip_black is set, pens that are too close to pure black are
   excluded -- prevents NetHack text colors that don't have a close
   palette match from rendering as black-on-black on the map. */
static short
nearest_pen_ex(short want_r, short want_g, short want_b, short skip_black)
{
    short i, best = 1;
    long best_dist = 0x7FFFFFFFL, d;
    short rgb[3];

    for (i = 0; i < colors && i < 16; i++) {
        vq_color(x_handle, i, 0, rgb);
        /* Exclude pure-black-ish pens from the candidate set. */
        if (skip_black && rgb[0] + rgb[1] + rgb[2] < 300)
            continue;
        d = (long)(rgb[0]-want_r)*(rgb[0]-want_r)
          + (long)(rgb[1]-want_g)*(rgb[1]-want_g)
          + (long)(rgb[2]-want_b)*(rgb[2]-want_b);
        if (d < best_dist) {
            best_dist = d;
            best = i;
        }
    }
    return best;
}

static short
nearest_pen(short want_r, short want_g, short want_b)
{
    return nearest_pen_ex(want_r, want_g, want_b, 0);
}

/* Cached pen lookups - recomputed after tile palette is set.
   Defaults match standard VDI palette (before tile remap). */
static short pen_black = 1, pen_white = 0, pen_darkgray = 1;

/* Per-NetHack-CLR_* VDI pen, computed at runtime via nearest_pen
   against the currently-installed workstation palette.  Computed
   rather than fixed because the tile palette install in palettized
   modes can remap pens 0..15 away from the standard ST palette, so a
   fixed lookup would miss the closest available colour. */
static short nhclr_to_pen[16] = {
    /* sensible defaults until cache_nhclr_pens() runs */
    1, 2, 3, 6, 4, 7, 5, 9, 1, 10, 11, 14, 12, 15, 13, 0
};

static void
cache_pens(void)
{
    /* Target RGB (VDI 0..1000) for each NetHack CLR_*.  CLR_BLACK
       and CLR_WHITE keep the corners of the cube; the rest use the
       standard 8-colour wheel with a darkened "bright" variant. */
    static const short nhclr_rgb[16][3] = {
        {   0,   0,   0}, /* CLR_BLACK         */
        {1000,   0,   0}, /* CLR_RED           */
        {   0, 800,   0}, /* CLR_GREEN         */
        { 700, 400,   0}, /* CLR_BROWN         */
        {   0,   0,1000}, /* CLR_BLUE          */
        {1000,   0,1000}, /* CLR_MAGENTA       */
        {   0, 800,1000}, /* CLR_CYAN          */
        { 733, 733, 733}, /* CLR_GRAY          */
        { 500, 500, 500}, /* NO_COLOR (unused) */
        {1000, 600,   0}, /* CLR_ORANGE        */
        { 400,1000, 400}, /* CLR_BRIGHT_GREEN  */
        {1000,1000,   0}, /* CLR_YELLOW        */
        { 533, 533,1000}, /* CLR_BRIGHT_BLUE   */
        {1000, 533,1000}, /* CLR_BRIGHT_MAGENTA*/
        { 533,1000,1000}, /* CLR_BRIGHT_CYAN   */
        {1000,1000,1000}, /* CLR_WHITE         */
    };
    short i;
    pen_black    = nearest_pen(0, 0, 0);
    pen_white    = nearest_pen(1000, 1000, 1000);
    pen_darkgray = nearest_pen(400, 400, 400);
    /* Skip pure-black-ish pens for NetHack text colours so map glyphs
       always render visibly on the black map background, even when
       the palette has no close match for a particular CLR_*. */
    for (i = 0; i < 16; i++)
        nhclr_to_pen[i] = nearest_pen_ex(nhclr_rgb[i][0],
                                         nhclr_rgb[i][1],
                                         nhclr_rgb[i][2], 1);
}

/* Set window scrollbar elements to dark grey */
static void
set_slider_colors(short whandle)
{
    /* Color word (AES OBJECT format):
       bits 11-8 = border color, bits 6-4 = fill pattern (0=hollow,7=solid),
       bits 3-0 = fill color */
    short col = (pen_black << 8) | (7 << 4) | pen_darkgray;
    wind_set6(whandle, WF_COLOR, W_VBAR, col, col, 0);
    wind_set6(whandle, WF_COLOR, W_VSLIDE, col, col, 0);
    wind_set6(whandle, WF_COLOR, W_VELEV, col, col, 0);
    wind_set6(whandle, WF_COLOR, W_HBAR, col, col, 0);
    wind_set6(whandle, WF_COLOR, W_HSLIDE, col, col, 0);
    wind_set6(whandle, WF_COLOR, W_HELEV, col, col, 0);
    wind_set6(whandle, WF_COLOR, W_UPARROW, col, col, 0);
    wind_set6(whandle, WF_COLOR, W_DNARROW, col, col, 0);
}

/* Default STE VDI palette (VDI 0-1000 scale).
   Used to reorder the tile palette so GEM UI elements look correct. */
static const short default_st_vdi[16][3] = {
    {1000,1000,1000}, /* VDI  0: white   */
    {   0,   0,   0}, /* VDI  1: black   */
    {1000,   0,   0}, /* VDI  2: red     */
    {   0,1000,   0}, /* VDI  3: green   */
    {   0,   0,1000}, /* VDI  4: blue    */
    {   0,1000,1000}, /* VDI  5: cyan    */
    {1000,1000,   0}, /* VDI  6: yellow  */
    {1000,   0,1000}, /* VDI  7: magenta */
    { 733, 733, 733}, /* VDI  8: lt grey */
    { 533, 533, 533}, /* VDI  9: dk grey */
    {1000, 533, 533}, /* VDI 10: lt red  */
    { 533,1000, 533}, /* VDI 11: lt green*/
    { 533, 533,1000}, /* VDI 12: lt blue */
    { 533,1000,1000}, /* VDI 13: lt cyan */
    {1000,1000, 533}, /* VDI 14: lt yel  */
    {1000, 533,1000}, /* VDI 15: lt mag  */
};

/* Reorder the tile palette so each VDI pen holds the tile color closest
   to the default ST color for that pen.  Remap the tile bitmap pixels
   (in standard bitplane format) to match.
   palette: XIMG palette (device-ordered, VDI 0-1000 RGB triples)
   addr:    bitplane data in standard format (before vr_trnfm)
   nplanes: number of bitplanes
   words_per_line: fd_wdwidth * fd_h (total 16-pixel word groups) */
static void
reorder_tile_palette(short *palette, char *addr, int nplanes,
                     int img_w, int img_h)
{
    /* dev2vdi mapping (inverse of vdi2dev4) */
    static const short dev2vdi[16] = {
        0, 2, 3, 6, 4, 7, 5, 8, 9, 10, 11, 14, 12, 15, 13, 1
    };
    static const short vdi2dev[16] = {
        0, 15, 1, 2, 4, 6, 3, 5, 7, 8, 9, 10, 12, 14, 11, 13
    };
    int ncolors = 1 << nplanes;
    int i, j, best, vi;
    long best_dist, d;
    short vdi_r[16], vdi_g[16], vdi_b[16];
    short new_r[16], new_g[16], new_b[16];
    short vdi_remap[16]; /* old VDI index -> new VDI index */
    short dev_remap[16];
    int used[16];

    if (nplanes != 4)
        return; /* only 4-plane STE palette reordering is supported */

    /* Read XIMG palette into VDI-ordered arrays */
    for (i = 0; i < ncolors; i++) {
        /* palette is in device order; convert to VDI order */
        vi = (nplanes == 4) ? dev2vdi[i] : i;
        vdi_r[vi] = palette[i * 3 + 0];
        vdi_g[vi] = palette[i * 3 + 1];
        vdi_b[vi] = palette[i * 3 + 2];
    }

    /* For each default ST VDI slot, find the closest tile VDI color */
    for (i = 0; i < 16; i++)
        used[i] = 0;

    for (i = 0; i < ncolors; i++) {
        /* Find unused tile VDI color closest to default_st_vdi[i] */
        best = -1;
        best_dist = 0x7FFFFFFFL;
        for (j = 0; j < ncolors; j++) {
            long dr, dg, db;

            if (used[j])
                continue;
            dr = vdi_r[j] - default_st_vdi[i][0];
            dg = vdi_g[j] - default_st_vdi[i][1];
            db = vdi_b[j] - default_st_vdi[i][2];
            d = dr * dr + dg * dg + db * db;
            if (d < best_dist) {
                best_dist = d;
                best = j;
            }
        }
        used[best] = 1;
        new_r[i] = vdi_r[best];
        new_g[i] = vdi_g[best];
        new_b[i] = vdi_b[best];
        vdi_remap[best] = i; /* old VDI best -> new VDI i */
    }

    /* Build device-to-device remap (for pixel remapping).
       Pixel values in standard bitplane format are device indices.
       old_dev -> old_vdi -> new_vdi -> new_dev */
    for (i = 0; i < ncolors; i++) {
        int old_vdi = (nplanes == 4) ? dev2vdi[i] : i;
        int new_vdi = vdi_remap[old_vdi];
        int new_dev = (nplanes == 4) ? vdi2dev[new_vdi] : new_vdi;

        dev_remap[i] = new_dev;
    }

    /* Remap tile bitmap pixels in standard sequential bitplane format.
       depack_img stores: all lines of plane 0, then plane 1, etc.
       Each plane has wdwidth words per line, img_h lines.
       word_aligned = (img_w + 15) / 16 * 2 bytes per line. */
    {
        int word_aligned = ((img_w + 15) / 16) * 2; /* bytes per line */
        int wdwidth = word_aligned / 2; /* 16-bit words per line */
        long plane_size = (long)word_aligned * img_h; /* bytes per plane */
        int y, x, bit, p;

        for (y = 0; y < img_h; y++) {
            for (x = 0; x < wdwidth; x++) {
                /* Read this word from each plane */
                unsigned short plane_bits[8], new_bits[8];
                for (p = 0; p < nplanes; p++) {
                    unsigned short *wp = (unsigned short *)
                        (addr + p * plane_size + y * word_aligned);
                    plane_bits[p] = wp[x];
                    new_bits[p] = 0;
                }

                /* Remap each of the 16 pixels */
                for (bit = 15; bit >= 0; bit--) {
                    int old_dev = 0, new_dev;
                    for (p = 0; p < nplanes; p++)
                        old_dev |= ((plane_bits[p] >> bit) & 1) << p;
                    new_dev = dev_remap[old_dev];
                    for (p = 0; p < nplanes; p++)
                        new_bits[p] |= ((new_dev >> p) & 1) << bit;
                }

                /* Write back */
                for (p = 0; p < nplanes; p++) {
                    unsigned short *wp = (unsigned short *)
                        (addr + p * plane_size + y * word_aligned);
                    wp[x] = new_bits[p];
                }
            }
        }
    }

    /* Write back reordered palette in device order */
    for (i = 0; i < ncolors; i++) {
        int vi = (nplanes == 4) ? dev2vdi[i] : i;
        palette[i * 3 + 0] = new_r[vi];
        palette[i * 3 + 1] = new_g[vi];
        palette[i * 3 + 2] = new_b[vi];
    }
}

/* Read a pixel value from an N-plane standard-format (plane-major)
   bitmap.  Each plane contributes one bit; LSB = plane 0. */
static unsigned int
get_pixel_n(const unsigned char *raw, int x, int y, int w, int h, int planes)
{
    int lw = (w + 15) >> 4;
    int word = x >> 4;
    int bit_pos = 15 - (x & 15);
    long offset = (long) y * lw * 2 + word * 2;
    long plane_size = (long) lw * 2 * h;
    unsigned int pixel = 0;
    int p;
    for (p = 0; p < planes; p++) {
        const unsigned short *pl = (const unsigned short *)
            (raw + p * plane_size + offset);
        if (*pl & (1 << bit_pos))
            pixel |= (1U << p);
    }
    return pixel;
}

/* Truecolor pixel-format state, populated by probe_truecolor_format()
   from vq_scrninfo.  Defaults work for 24/32bpp Motorola-order
   workstations (Falcon native, MagiC on m68k, MagiCOnLinux 24bpp). */
static struct {
    short have_probe;     /* nonzero once probed successfully */
    short swap_bytes;     /* 1 = Intel byte order within pixel words */
    short r_bits, g_bits, b_bits;
    short r_pos[16], g_pos[16], b_pos[16]; /* bit positions LSB->MSB */
} tc_fmt;

/* Encode an 8-bit-per-channel RGB triple into a packed pixel using
   either the layout-agnostic bit-position tables from vq_scrninfo
   (preferred) or hardcoded fallbacks (24/32-plane Motorola RGB, or
   16-plane standard RGB565).  Writes 'bytes_per_pixel' bytes to *p,
   in big-endian unless swap_bytes is set. */
static void
encode_truecolor_pixel(unsigned char *p, int r, int g, int b, int planes)
{
    unsigned long val = 0;
    int bytes = (planes + 7) >> 3;
    int i;

    if (tc_fmt.have_probe) {
        int rs = r >> (8 - tc_fmt.r_bits);
        int gs = g >> (8 - tc_fmt.g_bits);
        int bs = b >> (8 - tc_fmt.b_bits);
        for (i = 0; i < tc_fmt.r_bits; i++)
            if (rs & (1 << i)) val |= 1UL << tc_fmt.r_pos[i];
        for (i = 0; i < tc_fmt.g_bits; i++)
            if (gs & (1 << i)) val |= 1UL << tc_fmt.g_pos[i];
        for (i = 0; i < tc_fmt.b_bits; i++)
            if (bs & (1 << i)) val |= 1UL << tc_fmt.b_pos[i];
    } else if (planes == 16) {
        val = ((unsigned long) ((r >> 3) & 0x1F) << 11)
            | ((unsigned long) ((g >> 2) & 0x3F) << 5)
            |  (unsigned long) ((b >> 3) & 0x1F);
    } else {
        val = ((unsigned long) (r & 0xFF) << 16)
            | ((unsigned long) (g & 0xFF) << 8)
            |  (unsigned long) (b & 0xFF);
    }

    if (tc_fmt.swap_bytes) {
        for (i = 0; i < bytes; i++)
            p[i] = (unsigned char) (val >> (i * 8));
    } else {
        for (i = 0; i < bytes; i++)
            p[i] = (unsigned char) (val >> ((bytes - 1 - i) * 8));
    }
}

/* Build a truecolor device-format MFDB from a palettized standard-
   format source image (1..8 planes).  Output plane count matches the
   screen (16, 24, or 32); bytes per pixel = (planes+7)/8.  fd_stand=0,
   fd_wdwidth = rounded_w * bytes_per_pixel / 2 / planes.

   This is the canonical NVDI / MagiC / fVDI pattern from Behne's
   PRINT_TC.C reference: vro_cpyfm in mode S_ONLY copies these
   device-format pixels straight to the screen with no palette
   involvement.  Works on any direct-color workstation.

   Note on fd_wdwidth: this code uses (bytes_per_line / 2 / scr_planes),
   not the VDI-spec "total 16-bit words per scanline".  The non-standard
   form is what NVDI/MagiC/fVDI actually expect for chunky direct-color
   MFDBs in this port; "fixing" it to match the spec produces scrambled
   16/24/32 bpp output. */
static int
build_truecolor_mfdb(IMG_header *img, MFDB *out, int scr_planes)
{
    int w = img->img_w;
    int h = img->img_h;
    int src_planes = img->planes;
    int rounded_w = (w + 15) & ~15;
    int bytes_per_pixel = (scr_planes + 7) >> 3;
    long bytes_per_line = (long) rounded_w * bytes_per_pixel;
    long total = bytes_per_line * (long) h;
    unsigned char *buf;
    int x, y;

    if (src_planes < 1 || src_planes > 8 || !img->addr || !img->palette)
        return FALSE;
    if (scr_planes != 16 && scr_planes != 24 && scr_planes != 32)
        return FALSE;
    buf = (unsigned char *) calloc(1, (size_t) total);
    if (!buf) return FALSE;

    for (y = 0; y < h; y++) {
        unsigned char *row = buf + (long) y * bytes_per_line;
        for (x = 0; x < w; x++) {
            unsigned int idx = get_pixel_n((unsigned char *) img->addr,
                                           x, y, w, h, src_planes);
            int r = (img->palette[idx * 3 + 0] * 255) / 1000;
            int g = (img->palette[idx * 3 + 1] * 255) / 1000;
            int b = (img->palette[idx * 3 + 2] * 255) / 1000;
            encode_truecolor_pixel(row + x * bytes_per_pixel,
                                   r, g, b, scr_planes);
        }
    }

    out->fd_addr = (short *) buf;
    out->fd_w = rounded_w;
    out->fd_h = h;
    out->fd_wdwidth = (short) ((bytes_per_line / 2) / scr_planes);
    out->fd_stand = 0;
    out->fd_nplanes = scr_planes;
    out->fd_r1 = out->fd_r2 = out->fd_r3 = 0;
    return TRUE;
}

/* Query screen workstation pixel format via vq_scrninfo (NVDI EdDI
   1.0+, opcode 102, subfunction 1).  Populates tc_fmt with bytes-
   per-pixel, byte-order, and per-channel bit-position tables so
   encode_truecolor_pixel can use the workstation's exact pixel
   layout (Falcon RGB555+overlay, RGB565, packed RGB, xRGB, etc.).
   Silently leaves have_probe=0 if vq_scrninfo isn't supported --
   encode_truecolor_pixel then falls back to hardcoded layouts. */
static void
probe_truecolor_format(void)
{
    short contrl[15], intin[2], ptsin[2];
    short intout[273], ptsout[2];
    VDIPB pb;
    int i;

    for (i = 0; i < 15; i++) contrl[i] = 0;
    for (i = 0; i < 273; i++) intout[i] = 0;
    contrl[0] = 102;
    contrl[1] = 0;
    contrl[3] = 1;
    contrl[5] = 1;
    contrl[6] = x_handle;
    intin[0] = 2;

    pb.control = contrl;
    pb.intin = intin;
    pb.ptsin = ptsin;
    pb.intout = intout;
    pb.ptsout = ptsout;
    vdi(&pb);

    /* contrl[4] = number of shorts written to intout.  Need at
       least 64 shorts for the full bit-position tables to be
       populated. */
    if (contrl[4] < 64)
        return;
    tc_fmt.swap_bytes = (intout[14] & 0x80) ? TRUE : FALSE;
    tc_fmt.r_bits = intout[8];
    tc_fmt.g_bits = intout[9];
    tc_fmt.b_bits = intout[10];
    if (tc_fmt.r_bits <= 0 || tc_fmt.r_bits > 16
        || tc_fmt.g_bits <= 0 || tc_fmt.g_bits > 16
        || tc_fmt.b_bits <= 0 || tc_fmt.b_bits > 16)
        return;
    for (i = 0; i < tc_fmt.r_bits; i++) tc_fmt.r_pos[i] = intout[16 + i];
    for (i = 0; i < tc_fmt.g_bits; i++) tc_fmt.g_pos[i] = intout[32 + i];
    for (i = 0; i < tc_fmt.b_bits; i++) tc_fmt.b_pos[i] = intout[48 + i];
    tc_fmt.have_probe = TRUE;
}

void recalc_msg_win(GRECT *);
void recalc_status_win(GRECT *);
void calc_std_winplace(short, GRECT *);
void (*v_mtext)(short, short, short, const char *);

/* v_gtext takes char* (non-const); shim it so v_mtext's prototype matches
   without requiring a cast that some toolchains miscompile. */
static void
vgtext_wrapper(short h, short x, short y, const char *s)
{
    v_gtext(h, x, y, (char *) s);
}

static void
set_normal_dial_colors(void)
{
    if (planes < 4)
        dial_colors(4, BLACK, WHITE, RED, RED, WHITE, BLACK, BLACK, BLACK,
                    WHITE, WHITE, WHITE, WHITE, FALSE, FALSE);
    else
        dial_colors(7, LWHITE, BLACK, RED, RED, BLACK, BLACK, BLACK, BLACK,
                    LWHITE, LWHITE, LWHITE, LWHITE, FALSE, FALSE);
}

static short no_glyph; /* the short indicating there is no glyph */
IMG_header tile_image, titel_image, rip_image;
MFDB Tile_bilder, Map_bild, Titel_bild, Rip_bild, Pet_Mark;
static short Tile_width = 16, Tile_height = 16, Tiles_per_line = 20;
char *Tilefile = NULL;
/* pet_mark Design by Warwick Allison warwick@troll.no */
static short pet_mark_data[] = { 0x0000, 0x3600, 0x7F00, 0x7F00,
                               0x3E00, 0x1C00, 0x0800 };
static short *normal_palette = NULL;
static void restore_normal_palette(void);

static struct gw {
    WIN *gw_window;
    short gw_type, gw_dirty;
    GRECT gw_place;
} Gem_nhwindow[MAXWIN];

typedef struct {
    short id;
    short size;
    short cw, ch;
    short prop;
} NHGEM_FONT;

GRECT dirty_map_area = { COLNO - 1, ROWNO, 0, 0 };
short map_cursx = 0, map_cursy = 0, curs_col = WHITE;
short draw_cursor = TRUE, scroll_margin = -1;
NHGEM_FONT map_font;
SCROLL scroll_map;
/* Set when the user has dragged the map window via MOVER, resized
   via SIZER, or toggled FULLER.  Rearrange_windows then keeps the
   user's geometry across font/tile-mode changes instead of snapping
   back to calc_std_winplace. */
static short map_user_placed = FALSE;
char **map_glyphs = NULL;
/* Per-cell VDI pen for ASCII map rendering.  Populated by
   mar_print_char; consumed by win_draw_map's ASCII branch which
   draws each same-colour run as one v_mtext call.  Replaces the
   old "white text + OR colored cells" trick that only worked on
   ST 4-plane palette ordering and breaks in truecolor. */
short **map_colors = NULL;

char **status_line;
short num_status_lines, status_w, status_align = FALSE;
NHGEM_FONT status_font;
dirty_rect *dr_stat;

short mar_message_pause = TRUE;
short mar_esc_pressed = FALSE;
short messages_per_move = 0;
char **message_line;
short *message_age;
short msg_pos = 0, msg_max = 0, msg_anz = 0, msg_width = 0, msg_vis = 3,
    msg_align = TRUE;
NHGEM_FONT msg_font;

SCROLL scroll_menu;
Gem_menu_item *invent_list;
short num_inv_lines = 0, Inv_width = 16;
NHGEM_FONT menu_font;
short Inv_how;

char **text_lines;
short *text_line_glyph;     /* parallel array: tile-idx per line, or no_glyph */
short num_text_lines = 0, text_width;
/* scratch output buffer for v_set_text; NVDI/EmuTOS dereferences this,
   stock TOS tolerates NULL but other VDI implementations bus-error. */
static short vst_out[4];
NHGEM_FONT text_font;
short use_rip = FALSE;
extern char **rip_line;

static OBJECT *zz_oblist[NHICON + 1];

MITEM scroll_keys[] = {
    /* menu, scan, state, mode, msg */
    { FAIL, key(CTRLLEFT, 0), K_CTRL, PAGE_LEFT, FAIL },
    { FAIL, key(CTRLRIGHT, 0), K_CTRL, PAGE_RIGHT, FAIL },
    { FAIL, key(SCANUP, 0), K_SHIFT, PAGE_UP, FAIL },
    { FAIL, key(SCANDOWN, 0), K_SHIFT, PAGE_DOWN, FAIL },
    { FAIL, key(SCANLEFT, 0), 0, LINE_LEFT, FAIL },
    { FAIL, key(SCANRIGHT, 0), 0, LINE_RIGHT, FAIL },
    { FAIL, key(SCANUP, 0), 0, LINE_UP, FAIL },
    { FAIL, key(SCANDOWN, 0), 0, LINE_DOWN, FAIL },
    { FAIL, key(SCANLEFT, 0), K_SHIFT, LINE_START, FAIL },
    { FAIL, key(SCANRIGHT, 0), K_SHIFT, LINE_END, FAIL },
    { FAIL, key(SCANUP, 0), K_CTRL, WIN_START, FAIL },
    { FAIL, key(SCANDOWN, 0), K_CTRL, WIN_END, FAIL },
    { FAIL, key(SCANHOME, 0), K_SHIFT, WIN_END, FAIL },
    { FAIL, key(SCANHOME, 0), 0, WIN_START, FAIL }
};
#define SCROLL_KEYS 14

static DIAINFO *Inv_dialog;

#define null_free(ptr) free(ptr), (ptr) = NULL
#define test_free(ptr) \
    if (ptr)           \
    null_free(ptr)

static char *Menu_title = NULL;

void mar_display_nhwindow(winid);
void
mar_check_hilight_status(void)
{
} /* to be filled :-) */
static char *mar_copy_of(const char *);

extern void panic(const char *, ...);
extern int done2(void);
/* NetHack core defines boolean as schar (signed char, 1 byte); E_GEM's
   boolean is enum int (4 bytes).  Declare with signed char so the d0
   return value is read as the 1 byte the callee actually wrote, not
   with garbage in the upper bytes. */
extern signed char menuitem_invert_test(int, unsigned, signed char);
void *
m_alloc(size_t amt)
{
    void *ptr;

    ptr = malloc(amt);
    if (!ptr)
        panic("Memory allocation failure; cannot get %lu bytes", amt);
    return (ptr);
}

void
mar_clear_messagewin(void)
{
    short i, *ptr = message_age;

    if (WIN_MESSAGE == WIN_ERR)
        return;
    for (i = msg_anz; --i >= 0; ptr++) {
        if (*ptr)
            Gem_nhwindow[WIN_MESSAGE].gw_dirty = TRUE;
        *ptr = FALSE;
    }
    mar_message_pause = FALSE;

    mar_display_nhwindow(WIN_MESSAGE);
}

void
clipbrd_save(void *data, short cnt, boolean append, boolean is_inv)
{
    char path[MAX_PATH], *text, *crlf = "\r\n";
    long handle;
    short i;

    if (data && cnt > 0 && scrp_path(path, "scrap.txt")
        && (handle = append ? Fopen(path, 1) : Fcreate(path, 0)) > 0) {
        if (append)
            Fseek(0L, (short) handle, SEEK_END);
        if (is_inv) {
            Gem_menu_item *it = (Gem_menu_item *) data;

            for (; it; it = it->Gmi_next) {
                text = it->Gmi_str;
                Fwrite((short) handle, strlen(text), text);
                Fwrite((short) handle, 2L, crlf);
            }
        } else {
            for (i = 0; i < cnt; i++) {
                text = ((char **) data)[i] + 1;
                Fwrite((short) handle, strlen(text), text);
                Fwrite((short) handle, 2L, crlf);
            }
        }
        Fclose((short) handle);

        scrp_changed(SCF_TEXT, 0x2e545854l); /* .TXT */
    }
}

void
move_win(WIN *z_win)
{
    GRECT frame = desk;
    short drag_x, drag_y;

    v_set_mode(MD_XOR);
    v_set_line(BLACK, 1, 0, 0, 0);
    frame.g_w <<= 1, frame.g_h <<= 1;
    if (graf_rt_dragbox(FALSE, &z_win->curr, &frame, &drag_x,
                        &drag_y, NULL)) {
        z_win->curr.g_x = drag_x;
        z_win->curr.g_y = drag_y;
        window_size(z_win, &z_win->curr);
    } else
        window_top(z_win);
}

void
message_handler(short x, short y)
{
    switch (objc_find(zz_oblist[MSGWIN], ROOT, MAX_DEPTH, x, y)) {
    case UPMSG:
        if (msg_pos > msg_vis - 1) {
            msg_pos--;
            Gem_nhwindow[WIN_MESSAGE].gw_dirty = TRUE;
            mar_display_nhwindow(WIN_MESSAGE);
        }
        Event_Timer(50, 0, TRUE);
        break;
    case DNMSG:
        if (msg_pos < msg_max) {
            msg_pos++;
            Gem_nhwindow[WIN_MESSAGE].gw_dirty = TRUE;
            mar_display_nhwindow(WIN_MESSAGE);
        }
        Event_Timer(50, 0, TRUE);
        break;
    case GRABMSGWIN:
    default:
        move_win(Gem_nhwindow[WIN_MESSAGE].gw_window);
        break;
    case -1:
        break;
    }
}

short
mar_ob_mapcenter(OBJECT *p_obj)
{
    WIN *p_w = WIN_MAP != WIN_ERR ? Gem_nhwindow[WIN_MAP].gw_window : NULL;

    if (p_obj && p_w) {
        p_obj->ob_x = p_w->work.g_x + p_w->work.g_w / 2 - p_obj->ob_width / 2;
        p_obj->ob_y =
            p_w->work.g_y + p_w->work.g_h / 2 - p_obj->ob_height / 2;
        return (DIA_LASTPOS);
    }
    return (DIA_CENTERED);
}

/****************************** set_no_glyph
 * *************************************/

void
mar_set_no_glyph(short ng)
{
    no_glyph = ng;
}

void
mar_set_tilefile(char *name)
{
    Tilefile = name;
}
void
mar_set_tilex(short value)
{
    Min(&value, 32);
    Max(&value, 1);
    Tile_width = value;
}
void
mar_set_tiley(short value)
{
    Min(&value, 32);
    Max(&value, 1);
    Tile_height = value;
}
/****************************** userdef_draw
 * *************************************/

void rearrange_windows(void);
void
mar_set_status_align(short sa)
{
    if (status_align != sa) {
        status_align = sa;
        rearrange_windows();
    }
}
void
mar_set_msg_align(short ma)
{
    if (msg_align != ma) {
        msg_align = ma;
        rearrange_windows();
    }
}
void
mar_set_msg_visible(short mv)
{
    if (mv != msg_vis) {
        Max(&mv, 1);
        Min(&mv, min(msg_anz, 20));
        Min(&mv, desk.g_h / msg_font.ch / 2);
        msg_vis = mv;
        rearrange_windows();
    }
}
/* size<0 cellheight; size>0 points */
void
mar_set_fontbyid(short type, short id, short size)
{
    short chardim[4];
    if (id <= 0)
        id = ibm_font_id;
    if ((size > -3 && size < 3) || size < -20 || size > 20)
        size = -ibm_font;
    /* For now allow FNT_PROP only with NHW_TEXT */
    if (type != NHW_TEXT && (FontInfo(id)->type & (FNT_PROP | FNT_ASCII)))
        id = ibm_font_id;
    switch (type) {
    case NHW_MESSAGE:
        if (msg_font.size == -size && msg_font.id == id)
            break;
        msg_font.size = -size;
        msg_font.id = id;
        msg_font.prop = FontInfo(id)->type & (FNT_PROP | FNT_ASCII);
        v_set_text(msg_font.id, msg_font.size, BLACK, 0, 0, chardim);
        msg_font.ch = chardim[3] ? chardim[3] : 1;
        msg_font.cw = chardim[2] ? chardim[2] : 1;
        msg_width = min(max_w / msg_font.cw - 3, MSGLEN);
        rearrange_windows();
        break;
    case NHW_MAP:
        if (map_font.size != -size || map_font.id != id) {
            map_font.size = -size;
            map_font.id = id;
            map_font.prop = FontInfo(id)->type & (FNT_PROP | FNT_ASCII);
            v_set_text(map_font.id, map_font.size, BLACK, 0, 0, chardim);
            map_font.ch = chardim[3] ? chardim[3] : 1;
            map_font.cw = chardim[2] ? chardim[2] : 1;
            rearrange_windows();
        }
        break;
    case NHW_STATUS:
        if (status_font.size == -size && status_font.id == id)
            break;
        status_font.size = -size;
        status_font.id = id;
        status_font.prop = FontInfo(id)->type & (FNT_PROP | FNT_ASCII);
        v_set_text(status_font.id, status_font.size, BLACK, 0, 0, chardim);
        status_font.ch = chardim[3] ? chardim[3] : 1;
        status_font.cw = chardim[2] ? chardim[2] : 1;
        rearrange_windows();
        break;
    case NHW_MENU:
        if (menu_font.size == -size && menu_font.id == id)
            break;
        menu_font.size = -size;
        menu_font.id = id;
        menu_font.prop = FontInfo(id)->type & (FNT_PROP | FNT_ASCII);
        v_set_text(menu_font.id, menu_font.size, BLACK, 0, 0, chardim);
        menu_font.ch = chardim[3] ? chardim[3] : 1;
        menu_font.cw = chardim[2] ? chardim[2] : 1;
        break;
    case NHW_TEXT:
        if (text_font.size == -size && text_font.id == id)
            break;
        text_font.size = -size;
        text_font.id = id;
        text_font.prop = FontInfo(id)->type & (FNT_PROP | FNT_ASCII);
        v_set_text(text_font.id, text_font.size, BLACK, 0, 0, chardim);
        text_font.ch = chardim[3] ? chardim[3] : 1;
        text_font.cw = chardim[2] ? chardim[2] : 1;
        break;
    default:
        break;
    }
}
void
mar_set_font(short type, const char *font_name, short size)
{
    short id = 0;
    /* usual Gem behavior, use the Font-ID */
    if (font_name && *font_name) {
        id = atoi(font_name);
        if (id <= 0) {
            short i, tid;
            char name[33]; /* vqt_name stores 32 chars plus NUL */
            for (i = fonts_loaded; i >= 1; i--) {
                tid = vqt_name(x_handle, i, name);
                if (!stricmp(name, font_name)) {
                    id = tid;
                    break;
                }
            }
        }
    }
    mar_set_fontbyid(type, id, size);
}
/* Apply a user-driven move/resize/full to the map window: commit the
   new geometry via window_size (which clamps to max/min, rebuilds the
   SCROLL state, calls wind_set WF_CURRXYWH, and triggers a redraw),
   then remember that the user owns the placement. */
static void
mar_map_resized(GRECT *new_curr)
{
    WIN *w;
    if (WIN_MAP == WIN_ERR || (w = Gem_nhwindow[WIN_MAP].gw_window) == NULL)
        return;
    window_size(w, new_curr);
    Gem_nhwindow[WIN_MAP].gw_place = w->curr;
    map_user_placed = TRUE;
}

/* Translate WIN_MESSAGE and WIN_STATUS by (dx, dy) pixels so they
   track a map move.  GEM has no parent/child window relationship,
   this is a manual lockstep.  Called from the WM_MOVED handler with
   the actual delta map->curr changed by (post-window_size, so any
   clamping the map underwent is reflected here). */
static void
mar_shift_chrome_windows(short dx, short dy)
{
    short which;
    if (dx == 0 && dy == 0)
        return;
    for (which = 0; which < 2; which++) {
        winid w_id = (which == 0) ? WIN_MESSAGE : WIN_STATUS;
        WIN *w;
        GRECT nc;
        if (w_id == WIN_ERR) continue;
        w = Gem_nhwindow[w_id].gw_window;
        if (!w) continue;
        nc = w->curr;
        nc.g_x = (short) (nc.g_x + dx);
        nc.g_y = (short) (nc.g_y + dy);
        window_size(w, &nc);
        Gem_nhwindow[w_id].gw_place = w->curr;
    }
}

void
rearrange_windows(void)
{
    GRECT area;
    short todo = TRUE;
    if (WIN_MAP != WIN_ERR && Gem_nhwindow[WIN_MAP].gw_window) {
        WIN *map_win = Gem_nhwindow[WIN_MAP].gw_window;
        scroll_map.px_hline =
            mar_set_tile_mode(FAIL) ? Tile_width : map_font.cw;
        scroll_map.px_vline =
            mar_set_tile_mode(FAIL) ? Tile_height : map_font.ch;
        if (todo) {
            calc_std_winplace(FAIL, &area);
            todo = FALSE;
        }
        calc_std_winplace(NHW_MAP, &area);
        map_win->max.g_w = area.g_w;
        map_win->max.g_h = area.g_h;
        if (map_user_placed)
            /* User has moved/resized the window; preserve the geometry
               but rerun window_size so SCROLL hpage/vpage/hmax/vmax
               reflect the new line size from the font/tile change. */
            window_size(map_win, &map_win->curr);
        else
            window_reinit(map_win, md, md, NULL, FALSE, 0);
        {
            short buf[8] = {0};
            buf[3] = K_CTRL;
            buf[4] = C('L');
            AvSendMsg(ap_id, AV_SENDKEY, buf);
        }
    }
    if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window) {
        if (todo) {
            calc_std_winplace(FAIL, &area);
            todo = FALSE;
        }
        calc_std_winplace(NHW_MESSAGE, &area);
        Gem_nhwindow[WIN_MESSAGE].gw_window->min_h = area.g_h;
        window_size(Gem_nhwindow[WIN_MESSAGE].gw_window, &area);
        redraw_window(Gem_nhwindow[WIN_MESSAGE].gw_window, NULL);
    }
    if (WIN_STATUS != WIN_ERR && Gem_nhwindow[WIN_STATUS].gw_window) {
        if (todo) {
            calc_std_winplace(FAIL, &area);
            todo = FALSE;
        }
        calc_std_winplace(NHW_STATUS, &area);
        Gem_nhwindow[WIN_STATUS].gw_window->min_h = area.g_h;
        window_size(Gem_nhwindow[WIN_STATUS].gw_window, &area);
        redraw_window(Gem_nhwindow[WIN_STATUS].gw_window, NULL);
    }
}
void
my_color_area(GRECT *area, short col)
{
    short pxy[4];

    v_set_fill(col, 1, IP_SOLID, 0);
    rc_grect_to_array(area, pxy);
    v_bar(x_handle, pxy);
}

void
my_clear_area(GRECT *area)
{
    my_color_area(area, pen_white);
}

static void
win_draw_map(short msg, WIN *win, GRECT *area)
{
    short pla[8], w = area->g_w - 1, h = area->g_h - 1;
    short i, x, y;
    GRECT back = *area;

    if (!mar_set_tile_mode(FAIL)) {
        short start, stop, starty, stopy;
        char tmp;

        start = (area->g_x - win->work.g_x) / map_font.cw
                + scroll_map.hpos;
        stop = (area->g_x + area->g_w + map_font.cw - 1
                - win->work.g_x) / map_font.cw
               + scroll_map.hpos;
        if (stop >= COLNO)
            stop = COLNO - 1;
        starty = (area->g_y - win->work.g_y) / map_font.ch
                 + scroll_map.vpos;
        stopy = min((area->g_y + area->g_h + map_font.ch - 1
                     - win->work.g_y) / map_font.ch
                    + scroll_map.vpos,
                    ROWNO);
        v_set_mode(MD_TRANS);

        x = win->work.g_x - scroll_map.px_hpos + start * map_font.cw;
        y = win->work.g_y - scroll_map.px_vpos + starty * map_font.ch;
        back.g_h = map_font.ch;
        /* Render each row as a sequence of same-colour runs, drawing
           each run as one v_mtext.  Replaces the old "white text +
           OR colored cells" trick (which only worked on the ST
           4-plane palette by accident, and gives white text on
           colored backgrounds in truecolor). */
        for (i = starty; i < stopy; i++, y += map_font.ch) {
            short j = start;
            back.g_y = y;
            my_color_area(&back, BLACK);
            while (j < stop) {
                short run_color =
                    map_colors ? map_colors[i][j] : WHITE;
                short k = j + 1;
                while (k < stop && map_colors
                       && map_colors[i][k] == run_color)
                    k++;
                v_set_text(map_font.id, map_font.size, run_color,
                           0, 0, vst_out);
                tmp = map_glyphs[i][k];
                map_glyphs[i][k] = 0;
                (*v_mtext)(x_handle,
                           x + (short) (j - start) * map_font.cw, y,
                           &map_glyphs[i][j]);
                map_glyphs[i][k] = tmp;
                j = k;
            }
        }
    } else {
        v_set_mode(MD_REPLACE);
        pla[2] = pla[0] = scroll_map.px_hpos + area->g_x - win->work.g_x;
        pla[3] = pla[1] = scroll_map.px_vpos + area->g_y - win->work.g_y;
        pla[2] += w;
        pla[3] += h;
        pla[6] = pla[4] = area->g_x; /* x_wert to */
        pla[7] = pla[5] = area->g_y; /* y_wert to */
        pla[6] += w;
        pla[7] += h;
        if (planes == 1) {
            short colindex[2] = { 1, 0 }; /* fg=black, bg=white */
            vrt_cpyfm(x_handle, MD_REPLACE, pla, &Map_bild, screen,
                      colindex);
        } else {
            vro_cpyfm(x_handle, S_ONLY, pla, &Map_bild, screen);
        }
    }

    if (draw_cursor) {
        v_set_line(curs_col, 1, 0, 0, 0);
        pla[0] = pla[2] =
            win->work.g_x
            + scroll_map.px_hline * (map_cursx - scroll_map.hpos);
        pla[1] = pla[3] =
            win->work.g_y
            + scroll_map.px_vline * (map_cursy - scroll_map.vpos);
        pla[2] += scroll_map.px_hline - 1;
        pla[3] += scroll_map.px_vline - 1;
        v_rect(pla[0], pla[1], pla[2], pla[3]);
    }
}

static short
draw_titel(PARMBLK *pb)
{
    static short pla[8];
    GRECT work = *(GRECT *) &pb->pb_x;

    if (rc_intersect((GRECT *) &pb->pb_xc, &work)) {
        pla[0] = pla[1] = 0;
        pla[2] = pb->pb_w - 1;
        pla[3] = pb->pb_h - 1;
        pla[6] = pla[4] = pb->pb_x; /* x_wert to */
        pla[7] = pla[5] = pb->pb_y; /* y_wert to */
        pla[6] += pb->pb_w - 1;
        pla[7] += pb->pb_h - 1;

        if (planes == 1) {
            short colindex[2] = { 1, 0 };
            vrt_cpyfm(x_handle, MD_REPLACE, pla, &Titel_bild, screen,
                      colindex);
        } else {
            vro_cpyfm(x_handle, S_ONLY, pla, &Titel_bild, screen);
        }
    }

    return (0);
}

static short
draw_lines(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;

    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        char **ptr;
        short *gptr;
        short x = pb->pb_x, y = pb->pb_y, start_line = (area.g_y - y);
        short use_tiles = mar_set_tile_mode(FAIL);
        short text_x_off = use_tiles ? Tile_width + 4 : 0;

        v_set_mode((text_font.cw & 7) == 0 && text_font.prop == 0 ? MD_REPLACE
                                                                  : MD_TRANS);

        /* void v_set_text(short font,short height,short color,short effect,short mode,short out[4]) */
        v_set_text(text_font.id, text_font.size, BLACK, 0, 0, vst_out);
        start_line /= text_font.ch;
        y += start_line * text_font.ch;
        x -= (short) scroll_menu.px_hpos;
        ptr = &text_lines[start_line += scroll_menu.vpos];
        gptr = &text_line_glyph[start_line];
        start_line =
            min((area.g_y - y + area.g_h + text_font.ch - 1) / text_font.ch,
                num_text_lines - start_line);
        area.g_h = text_font.ch;
        Vsync();
        for (; --start_line >= 0; y += text_font.ch) {
            short gl = *gptr++;
            short line_off = (use_tiles && gl != no_glyph) ? text_x_off : 0;
            area.g_y = y;
            my_clear_area(&area);
            if (use_tiles && gl != no_glyph) {
                short pla[8], h = min(text_font.ch, Tile_height) - 1;
                pla[0] = pla[2] = (gl % Tiles_per_line) * Tile_width;
                pla[1] = pla[3] = (gl / Tiles_per_line) * Tile_height;
                pla[4] = pla[6] = x;
                pla[5] = pla[7] = y;
                pla[2] += Tile_width - 1;
                pla[3] += h;
                pla[6] += Tile_width - 1;
                pla[7] += h;
                if (planes == 1) {
                    short colindex[2] = { 1, 0 };
                    vrt_cpyfm(x_handle, MD_REPLACE, pla, &Tile_bilder,
                              screen, colindex);
                } else {
                    vro_cpyfm(x_handle, S_ONLY, pla, &Tile_bilder, screen);
                }
            }
            if (**ptr - 1) {
                v_set_text(FAIL, 0, BLUE, 0, 0, vst_out);
                (*v_mtext)(x_handle, x + line_off, y, (*ptr++) + 1);
                v_set_text(FAIL, 0, BLACK, 0, 0, vst_out);
            } else
                (*v_mtext)(x_handle, x + line_off, y, (*ptr++) + 1);
        }
    }
    return (0);
}

static short
draw_rip(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;
    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        char **ptr;
        short x = pb->pb_x, y = pb->pb_y, start_line = (area.g_y - y),
            chardim[4], i;
        short pla[8], sa_dummy;
        v_set_mode(MD_REPLACE);
        /* void v_set_text(short font,short height,short color,short effect,short mode,short out[4]) */
        v_set_text(text_font.id, text_font.size, BLACK, 0, 0, chardim);
        start_line /= text_font.ch;
        y += start_line * text_font.ch;
        x -= scroll_menu.px_hpos;
        ptr = &text_lines[start_line += scroll_menu.vpos];
        start_line =
            min((area.g_y - y + area.g_h + text_font.ch - 1) / text_font.ch,
                num_text_lines - start_line);
        area.g_h = text_font.ch;
        Vsync();
        x = (x + 7) & ~7;
        for (; --start_line >= 0; y += text_font.ch) {
            area.g_y = y;
            my_clear_area(&area);
            if (**ptr - 1) {
                v_set_text(FAIL, 0, BLUE, 0, 0, vst_out);
                (*v_mtext)(x_handle, x, y, (*ptr++) + 1);
                v_set_text(FAIL, 0, BLACK, 0, 0, vst_out);
            } else
                (*v_mtext)(x_handle, x, y, (*ptr++) + 1);
        }
        /* no tombstone image loaded: keep the plain text screen */
        if (Rip_bild.fd_addr) {
            pla[0] = pla[1] = 0;
            pla[2] = min(pb->pb_w - 1, Rip_bild.fd_w - 1);
            pla[3] = min(pb->pb_h - 1, Rip_bild.fd_h - 1);
            pla[6] = pla[4] =
                pb->pb_x + (pb->pb_w - Rip_bild.fd_w) / 2; /* x_wert to */
            pla[7] = pla[5] = pb->pb_y;                    /* y_wert to */
            pla[6] += pla[2];
            pla[7] += pla[3];
            if (planes == 1) {
                short colindex[2] = { 1, 0 };
                vrt_cpyfm(x_handle, MD_REPLACE, pla, &Rip_bild, screen,
                          colindex);
            } else {
                vro_cpyfm(x_handle, S_ONLY, pla, &Rip_bild, screen);
            }
            v_set_mode(MD_TRANS);
            vst_alignment(x_handle, 1, 5, &sa_dummy, &sa_dummy);
            pla[5] += 64;
            for (i = 0; i < 7; i++, pla[5] += chardim[3]) {
                v_set_text(text_font.id,
                           (i == 0 || i == 6) ? text_font.size : 12,
                           pen_white, 0, 0, chardim);
                (*v_mtext)(x_handle, pla[4] + 157, pla[5], rip_line[i]);
                v_set_text(text_font.id,
                           (i == 0 || i == 6) ? text_font.size : 12,
                           pen_black, 0, 0, chardim);
                (*v_mtext)(x_handle, pla[4] + 157, pla[5], rip_line[i]);
            }
            vst_alignment(x_handle, 0, 5, &sa_dummy, &sa_dummy);
        }
    }
    return (0);
}

static short
draw_msgline(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;

    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        short x = pb->pb_x, y = pb->pb_y + (msg_vis - 1) * msg_font.ch, i;
        short sa_foo;
        char **ptr = &message_line[msg_pos], tmp;
        short startx, stopx, starty, stopy;

        x = (x + 7) & ~7; /* Byte alignment speeds output up */

        v_set_mode(MD_REPLACE);

        /* void v_set_text(short font,short height,short color,short effect,short mode,short out[4]) */
        v_set_text(msg_font.id, msg_font.size, FAIL, 0, 0, vst_out);
        vst_alignment(x_handle, 0, 5, &sa_foo, &sa_foo);
        stopy = min(msg_pos, msg_vis);
        /*		Vsync();*/
        startx =
            (area.g_x - x) / msg_font.cw
            - 1; /* italic covers the next char */
        Max(&startx, 0);
        stopx = (area.g_x + area.g_w + msg_font.cw - x - 1) / msg_font.cw;
        Min(&stopx, MSGLEN);
        x += startx * msg_font.cw;
        for (i = 0; i < stopy; i++, y -= msg_font.ch, ptr--) {
            short len = (short) strlen(*ptr), ex;
            if (message_age[msg_pos - i])
                v_set_text(FAIL, 0, pen_black, 0, 0, vst_out);
            else
                v_set_text(FAIL, 0, pen_darkgray, 0, 0, vst_out);
            if (startx >= len) /* nothing of this line is exposed */
                continue;
            ex = min(stopx, len); /* don't read past the message text */
            tmp = (*ptr)[ex];
            (*ptr)[ex] = 0;
            (*v_mtext)(x_handle, x, y, &(*ptr)[startx]);
            (*ptr)[ex] = tmp;
        }
    }
    return (0);
}

static short
draw_status(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;

    area.g_x += 2 * status_font.cw - 2;
    area.g_w -= 2 * status_font.cw - 2;
    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        short x = pb->pb_x, y = pb->pb_y, startx, stopx, starty, stopy, i;
        char tmp;

        /* void v_set_text(short font,short height,short color,short effect,short mode,short out[4]) */
        v_set_mode(MD_REPLACE);
        v_set_text(status_font.id, status_font.size, BLACK, 0, 0, vst_out);
        x = (x + 2 * status_font.cw + 6) & ~7;

        startx = (area.g_x - x) / status_font.cw;
        starty = (area.g_y - y) / status_font.ch;
        stopx =
            (area.g_x + area.g_w + status_font.cw - 1 - x) / status_font.cw;
        stopy =
            (area.g_y + area.g_h + status_font.ch - 1 - y) / status_font.ch;
        Max(&startx, 0); /* area.g_x could end up 1 below x */
        Max(&stopx, 0);
        Min(&stopx, (short)(status_w - 1));
        x += startx * status_font.cw;
        y += starty * status_font.ch;
        /*		Vsync();*/
        area.g_h = status_font.ch;
        for (i = starty; i < min(2, stopy);
             i++, area.g_y += status_font.ch, y += status_font.ch) {
            my_clear_area(&area);
            tmp = status_line[i][stopx];
            status_line[i][stopx] = 0;
            (*v_mtext)(x_handle, x, y, &status_line[i][startx]);
            status_line[i][stopx] = tmp;
        }
    }
    return (0);
}

static short
draw_inventory(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;

    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        short gl, i, x = pb->pb_x, y = pb->pb_y, start_line = area.g_y - y;
        Gem_menu_item *it;

        v_set_mode(MD_REPLACE);
        v_set_text(menu_font.id, menu_font.size, BLACK, 0, 0, vst_out);

        start_line /= menu_font.ch;
        y += start_line * menu_font.ch;
        x -= scroll_menu.px_hpos;
        start_line += scroll_menu.vpos;

        for (it = invent_list, i = start_line; --i >= 0 && it;
             it = it->Gmi_next)
            ;

        i = min((area.g_y - y + area.g_h + menu_font.ch - 1) / menu_font.ch,
                num_inv_lines - start_line);

        Vsync();
        area.g_h = menu_font.ch;

        for (; (--i >= 0) && it; it = it->Gmi_next, y += menu_font.ch) {
            short pen;

            /* Gmi_color == 8 is NO_COLOR -- fall back to attr-based
               BLUE/BLACK so uncoloured items keep the historical look.
               CLR_WHITE on the white dialog background would be
               invisible; substitute BLACK so it stays readable. */
            if (it->Gmi_color >= 0 && it->Gmi_color < 16
                && it->Gmi_color != 8)
                pen = (it->Gmi_color == 15)
                    ? BLACK : nhclr_to_pen[it->Gmi_color];
            else if (it->Gmi_attr)
                pen = BLUE;
            else
                pen = BLACK;
            v_set_text(FAIL, FALSE, pen, 0, 0, vst_out);

            area.g_y = y;
            my_clear_area(&area);
            if ((gl = it->Gmi_glyph) != no_glyph) {
                short pla[8], h = min(menu_font.ch, Tile_height) - 1;

                pla[0] = pla[2] =
                    (gl % Tiles_per_line) * Tile_width; /* x_wert from */
                pla[1] = pla[3] =
                    (gl / Tiles_per_line) * Tile_height; /* y_wert from */
                pla[4] = pla[6] = x;                     /* x_wert to */
                pla[5] = pla[7] = y;                     /* y_wert to */
                pla[2] += Tile_width - 1;
                pla[3] += h;
                pla[6] += Tile_width - 1;
                pla[7] += h;

                if (planes == 1) {
                    short colindex[2] = { 1, 0 };
                    vrt_cpyfm(x_handle, MD_REPLACE, pla, &Tile_bilder,
                              screen, colindex);
                } else {
                    vro_cpyfm(x_handle, S_ONLY, pla, &Tile_bilder, screen);
                }
            }
            if (it->Gmi_identifier)
                it->Gmi_str[2] = it->Gmi_selected
                                     ? (it->Gmi_count == -1L ? '+' : '#')
                                     : '-';
            (*v_mtext)(x_handle, (x + 23) & ~7, y, it->Gmi_str);
        }
    }
    return (0);
}

static short
draw_prompt(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;

    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        char **ptr = (char **) pb->pb_parm;
        short x = pb->pb_x, y = pb->pb_y, chardim[4];

        /* void v_set_text(short font,short height,short color,short effect,short mode,short out[4]) */
        v_set_mode(MD_TRANS);
        v_set_text(ibm_font_id, ibm_font, WHITE, 0, 0, chardim);
        Vsync();
        if (planes < 4) {
            short pxy[4];
            v_set_fill(BLACK, 2, 4, 0);
            rc_grect_to_array(&area, pxy);
            v_bar(x_handle, pxy);
        } else
            my_color_area(&area, LWHITE);
        (*v_mtext)(x_handle, x, y, *(ptr++));
        if (*ptr)
            (*v_mtext)(x_handle, x, y + chardim[3], *ptr);
    }
    return (0);
}

static USERBLK ub_lines = { draw_lines, 0L }, ub_msg = { draw_msgline, 0L },
               ub_inventory = { draw_inventory, 0L },
               ub_titel = { draw_titel, 0L }, ub_status = { draw_status, 0L },
               ub_prompt = { draw_prompt, 0L };

/**************************** rsc_funktionen *****************************/

void
my_close_dialog(DIAINFO *dialog, boolean shrink_box)
{
    close_dialog(dialog, shrink_box);
    Event_Timer(0, 0, TRUE);
}

void
mar_get_rsc_tree(short obj_number, OBJECT **z_ob_obj)
{
    rsrc_gaddr(R_TREE, obj_number, z_ob_obj);
    fix_objects(*z_ob_obj, SCALING, 0, 0);
}

void mar_clear_map(void);

void
img_error(short errnumber)
{
    char buf[BUFSZ];

    switch (errnumber) {
    case ERR_HEADER:
        strcpy(buf, "[1][ Image Header | corrupt. ][ Oops ]");
        break;
    case ERR_ALLOC:
        strcpy(buf, "[1][ Not enough | memory for | an image. ][ Oops ]");
        break;
    case ERR_FILE:
        strcpy(buf, "[1][ The Image-file | is not available ][ Oops ]");
        break;
    case ERR_DEPACK:
        strcpy(buf, "[1][ The Image-file | is corrupt ][ Oops ]");
        break;
    case ERR_COLOR:
        strcpy(buf, "[1][ Number of colors | not supported ][ Oops ]");
        break;
    default:
        sprintf(buf, "[1][ img_error | strange error | number: %i ][ Hmm ]",
                errnumber);
        break;
    }
    form_alert(1, buf);
}

void
mar_change_button_char(OBJECT *z_ob, short nr, char ch)
{
    *ob_get_text(z_ob, nr, 0) = ch;
    ob_set_hotkey(z_ob, nr, ch);
}

void
mar_set_dir_keys(void)
{
    static short mi_numpad = FAIL;
    char mcmd[] = "bjnh.lyku", npcmd[] = "123456789", *p_cmd;

    if (mi_numpad != mar_iflags_numpad()) {
        OBJECT *z_ob = zz_oblist[DIRECTION];
        short i;
        mi_numpad = mar_iflags_numpad();
        ob_set_hotkey(z_ob, DIRDOWN, '>');
        ob_set_hotkey(z_ob, DIRUP, '<');
        p_cmd = mi_numpad ? npcmd : mcmd;
        for (i = 0; i < 9; i++)
            mar_change_button_char(z_ob, DIR1 + 2 * i, p_cmd[i]);
    }
}

extern int total_tiles_used; /* tile.c */

/* load and prepare the tile sheet; 0 on success, IMG error code else */
static short
load_tile_image(void)
{
    short img_err, tried_default = FALSE;

    if (tile_image.addr)
        return (0);

loadimg:
    img_err = depack_img(Tilefile ? Tilefile : (planes >= 5) ? "NH32.IMG"
                                                  : (planes >= 4) ? "NH16.IMG"
                                                                   : "NH2.IMG",
                             &tile_image);
    if (img_err)
        return (img_err);
    if ((tile_image.img_w % Tile_width || tile_image.img_h % Tile_height)
        && !tried_default) {
        Tilefile = NULL;
        Tile_width = Tile_height = 16;
        tried_default = TRUE;
        img_error(ERR_HEADER);
        goto loadimg;
    }
    if ((tile_image.img_w / Tile_width) * (tile_image.img_h / Tile_height)
            < total_tiles_used
        && !tried_default) {
        Tilefile = NULL;
        Tile_width = Tile_height = 16;
        tried_default = TRUE;
        img_error(ERR_HEADER);
        goto loadimg;
    }
    Tiles_per_line = tile_image.img_w / Tile_width;

    /* Reorder tile palette to match default ST VDI palette ordering.
       Must happen before transform_img (which converts to device format).
       Skipped in truecolor mode -- there's no workstation palette to
       align with. */
    if (planes <= 8 && tile_image.planes >= 4 && tile_image.palette)
        reorder_tile_palette(tile_image.palette, tile_image.addr,
                             tile_image.planes,
                             tile_image.img_w, tile_image.img_h);

    if (planes >= 16 && tile_image.palette) {
        /* Truecolor path (Behne PRINT_TC.C convention): pre-render
           the palettized tile sheet into a chunky device-format
           buffer at the screen's native depth.  vro_cpyfm then
           copies device-format pixels straight to screen with no
           palette involvement. */
        MFDB new_mfdb;
        if (build_truecolor_mfdb(&tile_image, &new_mfdb, planes)) {
            free(tile_image.addr);
            tile_image.addr = (char *) new_mfdb.fd_addr;
            tile_image.planes = planes;
            Tile_bilder = new_mfdb;
        }
    } else {
        mfdb(&Tile_bilder, (short *) tile_image.addr, tile_image.img_w,
             tile_image.img_h, 1, tile_image.planes);
        transform_img(&Tile_bilder);
        /* Set workstation palette so vro_cpyfm of palettized device
           data displays the right colors.  Only meaningful at <=8
           planes; on truecolor we've already baked RGB into pixels. */
        if (tile_image.planes > 1 && tile_image.palette)
            img_set_colors(x_handle, tile_image.palette, tile_image.planes);
    }
    return (0);
}

int
mar_gem_init(void)
{
    short i, img_err = FALSE, fsize;
    char *fname;
    static MITEM wish_workaround = { FAIL, key(0, 'J'), K_CTRL, W_CYCLE,
                                     FAIL };
    OBJECT *z_ob;

    if (!open_rsc("gem_rsc.rsc", md, md, md, md, 0, 0, 0)) {
        graf_mouse(M_OFF, NULL);
        form_alert(1, "[3][| Fatal Error | File: GEM_RSC.RSC | not "
                      "found or | GEM init failed. ][ grumble ]");
        return (0);
    }
    if (planes < 1
        || (planes > 8 && planes != 16 && planes != 24 && planes != 32)) {
        form_alert(
            1,
            "[3][ Color-depth | not supported. | Try 2-256 colors | or 16/24-bit. ][ Ok ]");
        return (0);
    }
    if (planes >= 16) {
        short i;
        probe_truecolor_format();
        /* Install the standard ST palette at pens 0..15 so text and
           chrome rendering (which uses pen indices via nhclr_to_pen
           and vst_color) gets the expected colors. */
        for (i = 0; i < 16; i++)
            vs_color(x_handle, i, (short *) default_st_vdi[i]);
    }
    MouseBee();

    /* NVDI 3.0 or better used v_ftext; not available in modern gemlib,
       so always wrap v_gtext through a const-correct shim. */
    v_mtext = vgtext_wrapper;
    for (i = 0; i < NHICON; i++)
        mar_get_rsc_tree(i, &zz_oblist[i]);

    /* Force the YN prompt to render in black; the RSC ships a textc that
       maps to a grey shade in MagiC's truecolor AES rendering, which is
       hard to read on the dialog body.  Color word layout: bits 11-8 hold
       text color, with G_BLACK == 1. */
    if (zz_oblist[YNCHOICE]) {
        TEDINFO *te = zz_oblist[YNCHOICE][YNPROMPT].ob_spec.tedinfo;
        if (te)
            te->te_color = (te->te_color & ~0x0F00) | 0x0100;
    }

    z_ob = zz_oblist[ABOUT];
    ob_hide(z_ob, OKABOUT, TRUE);
    beg_update(FALSE, FALSE);
    ob_draw_dialog(z_ob, 0, 0, 0, 0);
    end_update(FALSE);

    mar_get_font(NHW_MESSAGE, &fname, &fsize);
    mar_set_font(NHW_MESSAGE, fname, fsize);
    mar_get_font(NHW_MAP, &fname, &fsize);
    mar_set_font(NHW_MAP, fname, fsize);
    mar_get_font(NHW_STATUS, &fname, &fsize);
    mar_set_font(NHW_STATUS, fname, fsize);
    mar_get_font(NHW_MENU, &fname, &fsize);
    mar_set_font(NHW_MENU, fname, fsize);
    mar_get_font(NHW_TEXT, &fname, &fsize);
    mar_set_font(NHW_TEXT, fname, fsize);
    msg_anz = mar_get_msg_history();
    mar_set_msg_visible(mar_get_msg_visible());
    msg_width = min(max_w / msg_font.cw - 3, MSGLEN);

    if (max_w / status_font.cw < COLNO - 1)
        mar_set_fontbyid(NHW_STATUS, small_font_id, -small_font);
    status_w = min(max_w / status_font.cw - 3, MSGLEN);

    if (planes > 0 && colors > 0 && colors <= 256) {
        normal_palette = (short *) m_alloc(3 * colors * sizeof(short));
        get_colors(x_handle, normal_palette, colors);
        atexit(restore_normal_palette);
    }

    if (mar_set_tile_mode(FAIL)) {
        img_err = load_tile_image();
        if (img_err) {
            z_ob = zz_oblist[ABOUT];
            ob_undraw_dialog(z_ob, 0, 0, 0, 0);
            ob_hide(z_ob, OKABOUT, FALSE);
            img_error(img_err);
            return (0);
        }
    }
    cache_pens();

    mfdb(&Map_bild, NULL, (COLNO - 1) * Tile_width, ROWNO * Tile_height, 0,
         planes);
    Map_bild.fd_addr = (short *) m_alloc(mfdb_size(&Map_bild));

    mfdb(&Pet_Mark, pet_mark_data, 8, 7, 1, 1);
    vr_trnfm(x_handle, &Pet_Mark, &Pet_Mark);

    for (i = 0; i < MAXWIN; i++) {
        Gem_nhwindow[i].gw_window = NULL;
        Gem_nhwindow[i].gw_type = 0;
        Gem_nhwindow[i].gw_dirty = TRUE;
    }

    memset(&scroll_menu, 0, sizeof(scroll_menu));
    scroll_menu.scroll = AUTO_SCROLL;
    scroll_menu.obj = LINESLIST;
    scroll_menu.px_hline = menu_font.cw;
    scroll_menu.px_vline = menu_font.ch;
    scroll_menu.hscroll = scroll_menu.vscroll = 1;
    scroll_menu.tbar_d = 2 * gr_ch - 2;

    mar_set_dir_keys();

    memset(&scroll_map, 0, sizeof(scroll_map));
    scroll_map.scroll = AUTO_SCROLL;
    scroll_map.obj = ROOT;
    scroll_map.px_hline = mar_set_tile_mode(FAIL) ? Tile_width : map_font.cw;
    scroll_map.px_vline = mar_set_tile_mode(FAIL) ? Tile_height : map_font.ch;
    scroll_map.hsize = COLNO - 1;
    scroll_map.vsize = ROWNO;
    scroll_map.hpage = 8;
    scroll_map.vpage = 8;
    scroll_map.hscroll = 1;
    scroll_map.vscroll = 1;

    /* dial_options( round, niceline, standard, return_default, background,
       nonselectable,
            always_keys, toMouse, clipboard, hz);	*/
    dial_options(TRUE, TRUE, FALSE, TRUE, TRUE, TRUE,
                 TRUE, FALSE, TRUE, 0);
    set_normal_dial_colors();

    /* void MenuItems(MITEM *close,MITEM *closeall,MITEM *cycle,MITEM
       *invcycle,
            MITEM *globcycle,MITEM *full,MITEM *bottom,MITEM *iconify,MITEM
       *iconify_all,
            MITEM *menu,short menu_cnt) */
    /* Ctrl-W ist normaly bound to cycle */
    MenuItems(NULL, NULL, &wish_workaround, NULL, NULL, NULL, NULL, NULL,
              NULL, NULL, 0);

    menu_install(zz_oblist[MENU], TRUE);

    z_ob = zz_oblist[ABOUT];
    ob_undraw_dialog(z_ob, 0, 0, 0, 0);
    ob_hide(z_ob, OKABOUT, FALSE);

    return (1);
}

/* Restore the original VDI palette and forget our saved copy.
   Idempotent: subsequent calls are no-ops because null_free clears
   normal_palette.  Registered with atexit() at startup so even
   non-windowport exit paths (panic via nh_terminate, dialog quit)
   leave the GEM desktop in a sane state.

   Uses preserve_sys=0 (unlike img_set_colors which defaults to 1):
   the title-image setup overwrites pens 0-15 directly with vs_color,
   so a preserve_sys=1 restore would leave the desktop stuck with
   title-image colors after quitting. */
static void
restore_normal_palette(void)
{
    if (normal_palette) {
        img_set_colors_ex(x_handle, normal_palette, planes, 0);
        null_free(normal_palette);
    }
}

/************************* mar_exit_nhwindows *******************************/

void
mar_exit_nhwindows(void)
{
    short i;

    /* Restore the original VDI palette before tearing anything down, so a
       GEM desktop survives even if a later step bails out. */
    restore_normal_palette();

    for (i = MAXWIN; --i >= 0;)
        if (Gem_nhwindow[i].gw_type)
            mar_destroy_nhwindow(i);

    test_free(tile_image.palette);
    test_free(tile_image.addr);
    test_free(titel_image.palette);
    test_free(titel_image.addr);

    close_rsc(TRUE, 0);
}

/************************* mar_curs *******************************/

void
mar_curs(short x, short y)
{
    short tmp;
    tmp = dirty_map_area.g_x; Min(&tmp, x); dirty_map_area.g_x = tmp;
    tmp = dirty_map_area.g_y; Min(&tmp, y); dirty_map_area.g_y = tmp;
    tmp = dirty_map_area.g_w; Max(&tmp, x); dirty_map_area.g_w = tmp;
    tmp = dirty_map_area.g_h; Max(&tmp, y); dirty_map_area.g_h = tmp;
    tmp = dirty_map_area.g_x; Min(&tmp, map_cursx); dirty_map_area.g_x = tmp;
    tmp = dirty_map_area.g_y; Min(&tmp, map_cursy); dirty_map_area.g_y = tmp;
    tmp = dirty_map_area.g_w; Max(&tmp, map_cursx); dirty_map_area.g_w = tmp;
    tmp = dirty_map_area.g_h; Max(&tmp, map_cursy); dirty_map_area.g_h = tmp;

    map_cursx = x;
    map_cursy = y;

    if (WIN_MAP != WIN_ERR)
        Gem_nhwindow[WIN_MAP].gw_dirty = TRUE;
}

void mar_cliparound(void);
void
mar_map_curs_weiter(void)
{
    static short once = TRUE;

    if (once) {
        if (WIN_STATUS != WIN_ERR && Gem_nhwindow[WIN_STATUS].gw_window)
            redraw_window(Gem_nhwindow[WIN_STATUS].gw_window, NULL);
        if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window)
            redraw_window(Gem_nhwindow[WIN_MESSAGE].gw_window, NULL);
        once = FALSE;
    }
    mar_curs(map_cursx + 1, map_cursy);
    mar_cliparound();
}

/************************* about *******************************/

void
mar_about(void)
{
    xdialog(zz_oblist[ABOUT], md, NULL, NULL, DIA_CENTERED, FALSE,
            DIALOG_MODE);
    Event_Timer(0, 0, TRUE);
}

/************************* ask_name *******************************/

char *
mar_ask_name(void)
{
    OBJECT *z_ob = zz_oblist[NAMEGET];
    short img_err;
    char who_are_you[] = "Who are you? ";

    img_err =
        depack_img(planes < 4 ? "TITLE2.IMG" : "TITLE.IMG", &titel_image);
    if (img_err) { /* not fatal */
        ob_set_text(z_ob, NETHACKPICTURE, "missing title.img.");
    } else if (planes >= 16 && titel_image.palette) {
        /* Truecolor: pre-render via the same chunky-buffer pattern
           we use for tiles.  transform_img would zero the buffer on
           a >8-plane workstation. */
        MFDB new_mfdb;
        if (build_truecolor_mfdb(&titel_image, &new_mfdb, planes)) {
            free(titel_image.addr);
            titel_image.addr = NULL;
            Titel_bild = new_mfdb;
            z_ob[NETHACKPICTURE].ob_type = G_USERDEF;
            z_ob[NETHACKPICTURE].ob_spec.userblk = &ub_titel;
        } else {
            ob_set_text(z_ob, NETHACKPICTURE, "transform failed.");
            img_err = 1;
        }
    } else {
        mfdb(&Titel_bild, (short *) titel_image.addr, titel_image.img_w,
             titel_image.img_h, 1, titel_image.planes);
        if (!transform_img(&Titel_bild)) {
            /* convert() freed titel_image.addr (aliased via Titel_bild)
               before its second alloc failed; avoid double-free */
            titel_image.addr = NULL;
            ob_set_text(z_ob, NETHACKPICTURE, "transform failed.");
            img_err = 1;
        } else {
            /* convert() freed the original addr via the MFDB; avoid
               double-free in cleanup */
            titel_image.addr = NULL;
            z_ob[NETHACKPICTURE].ob_type = G_USERDEF;
            z_ob[NETHACKPICTURE].ob_spec.userblk = &ub_titel;
        }
    }

    /* Close the About splash dialog before opening the name dialog */
    {
        OBJECT *about_ob = zz_oblist[ABOUT];
        ob_undraw_dialog(about_ob, 0, 0, 0, 0);
        ob_hide(about_ob, OKABOUT, FALSE);
    }

    ob_clear_edit(z_ob);
    /* In palettized modes, install the title-image palette via
       vs_color so the title MFDB's device-format pixel indices render
       with the right colors.  Skipped in truecolor mode -- the title
       chunky buffer already encodes RGB directly. */
    if (planes <= 8 && !img_err && titel_image.palette
        && titel_image.planes > 1) {
        static const short dev2vdi[] =
            { 0, 2, 3, 6, 4, 7, 5, 8, 9, 10, 11, 14, 12, 15, 13, 1 };
        short i, nimg = min(1 << titel_image.planes, 16);
        for (i = 0; i < nimg; i++) {
            short vdi_pen = dev2vdi[i];
            if (planes > 4 && i == 15)
                vdi_pen = colors - 1;
            vs_color(x_handle, vdi_pen, titel_image.palette + i * 3);
        }
    }
    xdialog(z_ob, who_are_you, NULL, NULL, DIA_CENTERED, FALSE, DIALOG_MODE);
    Event_Timer(0, 0, TRUE);
    /* Restore system palette after the title dialog closes (no-op
       in truecolor: title left the workstation palette alone). */
    if (planes <= 8 && normal_palette)
        img_set_colors(x_handle, normal_palette, planes);

    test_free(titel_image.palette);
    test_free(titel_image.addr);
    test_free(Titel_bild.fd_addr);

    /* Re-install the tile palette after the title-restore step
       above clobbered it (palettized only; tiles in truecolor mode
       have their colors baked into the pixel data). */
    if (planes <= 8 && tile_image.planes > 1 && tile_image.palette)
        img_set_colors(x_handle, tile_image.palette, tile_image.planes);

    /* Cache nearest-pen lookups now that the tile palette is active */
    cache_pens();

    return (ob_get_text(z_ob, PLNAME, 0));
}

/************************* more *******************************/

void
send_key(short key)
{
    short buf[8] = {0};

    buf[3] = 0; /* No Shift/Ctrl/Alt */
    buf[4] = key;
    AvSendMsg(ap_id, AV_SENDKEY, buf);
}

void
send_return(void)
{
    send_key(key(SCANRET, 0));
}

/* Forward declarations for Event_Handler callbacks */
static short K_Init(XEVENT *, short);
static short KM_Init(XEVENT *, short);
static short M_Init(XEVENT *, short);
static short More_Handler(XEVENT *);
static short Text_Handler(XEVENT *);
static short Inv_Handler(XEVENT *);
static short Main_Init(XEVENT *, short);
static short Dia_Handler(XEVENT *);
static short single_handler(XEVENT *);
static short any_handler(XEVENT *);

short
K_Init(XEVENT *xev, short availiable)
{
    (void)xev;
    return (MU_KEYBD & availiable);
}

short
KM_Init(XEVENT *xev, short availiable)
{
    (void)xev;
    return ((MU_KEYBD | MU_MESAG) & availiable);
}

short
M_Init(XEVENT *xev, short availiable)
{
    (void)xev;
    return (MU_MESAG & availiable);
}

#define More_Init K_Init

short
More_Handler(XEVENT *xev)
{
    short ev = xev->ev_mwich;

    if (ev & MU_KEYBD) {
        char ch = (char) (xev->ev_mkreturn & 0x00FF);
        DIAINFO *dinf;
        WIN *w;

        switch (ch) {
        case '\033': /* no more more more */
        case ' ':
            if ((w = get_top_window()) && (dinf = (DIAINFO *) w->dialog)
                && dinf->di_tree == zz_oblist[PAGER]) {
                if (ch == '\033')
                    mar_esc_pressed = TRUE;
                send_return();
                break;
            }
        /* Fall thru */
        default:
            ev &= ~MU_KEYBD; /* unknown key */
            break;
        }
    }
    return (ev);
}

void
mar_more(void)
{
    if (!mar_esc_pressed) {
        OBJECT *z_ob = zz_oblist[PAGER];
        WIN *p_w;

        Event_Handler(More_Init, More_Handler);
        dial_colors(7, RED, BLACK, RED, RED, BLACK, BLACK, BLACK, BLACK, RED, RED, RED, RED, FALSE, FALSE);
        if (WIN_MESSAGE != WIN_ERR
            && (p_w = Gem_nhwindow[WIN_MESSAGE].gw_window)) {
            z_ob->ob_x = p_w->work.g_x;
            z_ob->ob_y = p_w->curr.g_y + p_w->curr.g_h + gr_ch;
        }
        xdialog(z_ob, NULL, NULL, NULL, DIA_LASTPOS, FALSE, DIALOG_MODE);
        Event_Timer(0, 0, TRUE);
        Event_Handler(NULL, NULL);

        set_normal_dial_colors();
    }
}

/************************* Gem_start_menu *******************************/
void
Gem_start_menu(winid win, unsigned long mbehavior)
{
    (void) win;
    (void) mbehavior;
    if (invent_list) {
        Gem_menu_item *curr, *next;

        for (curr = invent_list; curr; curr = next) {
            next = curr->Gmi_next;
            free(curr->Gmi_str);
            free(curr);
        }
    }
    invent_list = NULL;
    num_inv_lines = 0;
    Inv_width = 16;
}

/************************* mar_add_menu *******************************/

void
mar_add_menu(winid win, Gem_menu_item *item)
{
    (void)win;
    item->Gmi_next = invent_list;
    invent_list = item;
    num_inv_lines++;
}

void
mar_reverse_menu(void)
{
    Gem_menu_item *next, *head = 0, *curr = invent_list;

    while (curr) {
        next = curr->Gmi_next;
        curr->Gmi_next = head;
        head = curr;
        curr = next;
    }
    invent_list = head;
}

void
mar_set_accelerators(void)
{
    char ch = 'a';
    Gem_menu_item *curr;

    for (curr = invent_list; curr; curr = curr->Gmi_next) {
        short extent[8];
        v_set_text(menu_font.id, menu_font.size, BLACK, 0, 0, vst_out);
        vqt_extent(x_handle, curr->Gmi_str, extent);
        Max(&Inv_width, (short) (extent[4] + Tile_width + menu_font.cw));
        if (ch && curr->Gmi_accelerator == 0 && curr->Gmi_identifier) {
            curr->Gmi_accelerator = ch;
            /* Gem_add_menu writes a '?' placeholder at str[0] for items
               with no pre-assigned accelerator; only overwrite that. */
            if (curr->Gmi_str[0] == '?')
                curr->Gmi_str[0] = ch;
            if (ch == 'z')
                ch = 'A';
            else if (ch == 'Z')
                ch = 0;
            else
                ch++;
        }
    }
}

Gem_menu_item *
mar_hol_inv(void)
{
    return (invent_list);
}

/************************* mar_putstr_text *********************/

void mar_raw_print(const char *);

void
mar_set_text_to_rip(winid w)
{
    use_rip = TRUE;
}
void
mar_putstr_text(winid window, short attr, const char *str, short glyph)
{
    static short lines_free = 0;
    short width = 0;
    char *ptr, *nl;

    (void)window;
    if (!text_lines) {
        text_lines = (char **) m_alloc(12 * sizeof(char *));
        text_line_glyph = (short *) m_alloc(12 * sizeof(short));
        lines_free = 12;
    }
    if (!lines_free) {
        char **tmp = (char **) realloc(text_lines, (num_text_lines + 12)
                                                       * sizeof(char *));
        short *tmp2;
        if (!tmp) {
            mar_raw_print("No room for Text");
            return;
        }
        text_lines = tmp;
        tmp2 = (short *) realloc(text_line_glyph,
                                 (num_text_lines + 12) * sizeof(short));
        if (!tmp2) {
            mar_raw_print("No room for Text");
            return;
        }
        text_line_glyph = tmp2;
        lines_free = 12;
    }

    if (str)
        width = strlen(str);
    Min(&width, 80);
    ptr = text_lines[num_text_lines] =
        (char *) m_alloc(width * sizeof(char) + 2);
    *ptr = (char) (attr + 1); /* avoid 0 */
    strncpy(ptr + 1, str ? str : "", width);
    ptr[width + 1] = 0;
    /* strip trailing newline/CR */
    for (nl = ptr + width; nl > ptr && (nl[0] == '\n' || nl[0] == '\r'); nl--)
        *nl = 0;
    text_line_glyph[num_text_lines] = glyph;
    num_text_lines++;
    lines_free--;
}

short
mar_set_inv_win(short Anzahl, short Breite)
{
    OBJECT *z_ob = zz_oblist[LINES];
    short retval = DIALOG_MODE;

    scroll_menu.hsize = 0;
    scroll_menu.vpage = (desk.g_h - 3 * gr_ch) / scroll_menu.px_vline;
    if (Anzahl > scroll_menu.vpage) {
        retval |= WD_VSLIDER;
        if (Breite > max_w - 3 * scroll_menu.px_hline) {
            retval |= WD_HSLIDER;
            scroll_menu.hpage =
                (max_w - 3 * scroll_menu.px_hline) / scroll_menu.px_hline;
            scroll_menu.hpos = 0;
            scroll_menu.hsize = Breite / scroll_menu.px_hline;
            scroll_menu.vpage =
                (desk.g_h - 4 * gr_ch - 1) / scroll_menu.px_vline;
        }
        Anzahl = scroll_menu.vpage;
    } else {
        if (Breite > max_w - scroll_menu.px_hline) {
            retval |= WD_HSLIDER;
            scroll_menu.hpage =
                (max_w - scroll_menu.px_hline) / scroll_menu.px_hline;
            scroll_menu.hpos = 0;
            scroll_menu.hsize = Breite / scroll_menu.px_hline;
            scroll_menu.vpage =
                (desk.g_h - 4 * gr_ch - 1) / scroll_menu.px_vline;
            if (Anzahl > scroll_menu.vpage) {
                retval |= WD_VSLIDER;
                Anzahl = scroll_menu.vpage;
            }
        }
        scroll_menu.vpage = Anzahl;
    }
    {
        short hmax_tmp = scroll_menu.hsize - scroll_menu.hpage;
        short vmax_tmp = scroll_menu.vsize - scroll_menu.vpage;
        if (hmax_tmp < 0) hmax_tmp = 0;
        if (vmax_tmp < 0) vmax_tmp = 0;
        scroll_menu.hmax = hmax_tmp;
        scroll_menu.vmax = vmax_tmp;
    }

    /* left/right/up 2 pixel border down 2gr_ch toolbar */
    z_ob[ROOT].ob_width = z_ob[LINESLIST].ob_width = Breite;
    z_ob[ROOT].ob_height = z_ob[QLINE].ob_y = z_ob[LINESLIST].ob_height =
        scroll_menu.px_vline * Anzahl;
    z_ob[QLINE].ob_y += gr_ch / 2;
    z_ob[ROOT].ob_width += 4;
    z_ob[ROOT].ob_height += 2 * gr_ch + 2;

    return (retval);
}

/************************* mar_status_dirty *******************************/

void
mar_status_dirty(void)
{
    short ccol;

    ccol = mar_hp_query();

    if (ccol < 2)
        curs_col = pen_white; /* 50-100% : 0 */
    else if (ccol < 3)
        curs_col = YELLOW; /* 33-50% : 6 */
    else if (ccol < 5)
        curs_col = LYELLOW; /* 20-33% : 14*/
    else if (ccol < 10)
        curs_col = RED; /* 10-20% : 2 */
    else
        curs_col = MAGENTA; /* <10% : 7*/
}

/************************* mar_add_message *******************************/

void
mar_add_message(const char *str)
{
    short i, mesg_hist = mar_get_msg_history();
    char *tmp, *rest, buf[TBUFSZ], toplines[TBUFSZ];

    if (WIN_MESSAGE == WIN_ERR)
        return;

    if (!mar_message_pause) {
        mar_message_pause = TRUE;
        messages_per_move = 0;
        msg_pos = msg_max;
    }

    if (msg_max > mesg_hist - 2) {
        msg_max = mesg_hist - 2;
        msg_pos--;
        if (msg_pos < 0)
            msg_pos = 0;
        tmp = message_line[0];
        for (i = 0; i < mesg_hist - 1; i++) {
            message_line[i] = message_line[i + 1];
            message_age[i] = message_age[i + 1];
        }
        message_line[mesg_hist - 1] = tmp;
    }
    strncpy(toplines, str, TBUFSZ - 1);
    toplines[TBUFSZ - 1] = '\0';
    messages_per_move++;
    msg_max++;
    if (msg_max >= msg_anz)
        msg_max = msg_anz - 1;

    if ((short) strlen(toplines) >= msg_width) {
        short pos = msg_width;
        tmp = toplines + msg_width;
        while (pos >= 0 && *tmp != ' ') {
            tmp--;
            pos--;
        }
        if (pos <= 0)
            pos = msg_width; /* Mar -- Oops, what a word :-) */
        message_age[msg_max] = TRUE;
        strncpy(message_line[msg_max], toplines, pos);
        message_line[msg_max][pos] = 0;
        rest = strcpy(buf, toplines + pos);
    } else {
        message_age[msg_max] = TRUE;
        strncpy(message_line[msg_max], toplines, msg_width);
        rest = 0;
    }

    Gem_nhwindow[WIN_MESSAGE].gw_dirty = TRUE;
    if (messages_per_move
        >= mesg_hist) { /* greater than should never happen */
        messages_per_move = mesg_hist;
        mar_display_nhwindow(WIN_MESSAGE);
    }

    if (rest)
        mar_add_message(rest);
}

/************************* mar_add_status_str *******************************/

void
mar_add_status_str(const char *str, short line)
{
    short i, last_diff = -1;
    GRECT area = { 0, line * status_font.ch, status_font.cw, status_font.ch };
    for (i = 0; (i < status_w - 2) && str[i]; i++)
        if (str[i] != status_line[line][i]) {
            if (last_diff == -1)
                area.g_x = i * status_font.cw;
            else
                area.g_w += status_font.cw;
            last_diff = i;
            status_line[line][i] = str[i];
        } else if (last_diff >= 0) {
            add_dirty_rect(dr_stat, &area);
            last_diff = -1;
            area.g_w = status_font.cw;
        }
    for (; i < status_w - 1; i++) {
        if (status_line[line][i]) {
            if (last_diff == -1)
                area.g_x = i * status_font.cw;
            else
                area.g_w += status_font.cw;
            last_diff = i;
        }
        status_line[line][i] = 0;
    }
    if (last_diff >= 0)
        add_dirty_rect(dr_stat, &area);
}

/************************* mar_set_menu_title *******************************/

void
mar_set_menu_title(const char *str)
{
    test_free(Menu_title); /* just in case */
    Menu_title = mar_copy_of(str ? str : nullstr);
}

/************************* mar_set_menu_type *******************************/

static short menu_cancelled = FALSE;

void
mar_set_menu_type(short how)
{
    Inv_how = how;
    menu_cancelled = FALSE;
}

short
mar_menu_cancelled(void)
{
    return menu_cancelled;
}

/************************* Inventory Utils *******************************/

void
set_all_on_page(short start, short page)
{
    Gem_menu_item *curr;

    if (start < 0 || page < 0)
        return;

    for (curr = invent_list; start-- && curr; curr = curr->Gmi_next)
        ;
    for (; page-- && curr; curr = curr->Gmi_next)
        if (curr->Gmi_identifier && !curr->Gmi_selected)
            curr->Gmi_selected = TRUE;
}

void
unset_all_on_page(short start, short page)
{
    Gem_menu_item *curr;

    if (start < 0 || page < 0)
        return;

    for (curr = invent_list; start-- && curr; curr = curr->Gmi_next)
        ;
    for (; page-- && curr; curr = curr->Gmi_next)
        if (curr->Gmi_identifier && curr->Gmi_selected) {
            curr->Gmi_selected = FALSE;
            curr->Gmi_count = -1L;
        }
}

void
invert_all_on_page(short start, short page, char acc)
{
    Gem_menu_item *curr;

    if (start < 0 || page < 0)
        return;

    for (curr = invent_list; start-- && curr; curr = curr->Gmi_next)
        ;
    for (; page-- && curr; curr = curr->Gmi_next)
        if (curr->Gmi_identifier && (acc == 0 || curr->Gmi_groupacc == acc)) {
            if (!menuitem_invert_test(0, curr->Gmi_itemflags,
                                      (signed char) curr->Gmi_selected))
                continue;
            if (curr->Gmi_selected) {
                curr->Gmi_selected = FALSE;
                curr->Gmi_count = -1L;
            } else
                curr->Gmi_selected = TRUE;
        }
}

/************************* Inv_Handler and Inv_Init
 * *******************************/

short
scroll_top_dialog(char ch)
{
    WIN *w;
    DIAINFO *dinf;

    if ((w = get_top_window()) && (dinf = (DIAINFO *) w->dialog)
        && dinf->di_tree == zz_oblist[LINES]) {
        switch (ch) {
        case ' ':
            if (scroll_menu.vpos == scroll_menu.vmax) {
                send_return();
                break;
            }
        /* Fall thru */
        case MENU_NEXT_PAGE:
            scroll_window(w, PAGE_DOWN, NULL);
            break;
        case MENU_PREVIOUS_PAGE:
            scroll_window(w, PAGE_UP, NULL);
            break;
        case MENU_FIRST_PAGE:
            scroll_window(w, WIN_START, NULL);
            break;
        case MENU_LAST_PAGE:
            scroll_window(w, WIN_END, NULL);
            break;
        default:
            return (FALSE);
        }
        return (TRUE);
    }
    return (FALSE);
}

#define Text_Init KM_Init

short
Text_Handler(XEVENT *xev)
{
    short ev = xev->ev_mwich;

    if (ev & MU_MESAG) {
        short *buf = xev->ev_mmgpbuf, y_wo, i;
        if (*buf == FNT_CHANGED) {
            if (buf[3] >= 0) {
                mar_set_fontbyid(NHW_TEXT, buf[4], buf[5]);
                FontAck(buf[1], 1);
            }
        }
    }
    if (ev & MU_KEYBD) {
        char ch = (char) (xev->ev_mkreturn & 0x00FF);

        if (!scroll_top_dialog(ch))
            switch (ch) {
            case '\033':
                send_return(); /* just closes the textwin */
                break;
            case C('c'):
                clipbrd_save(text_lines, num_text_lines,
                             xev->ev_mmokstate & K_SHIFT, FALSE);
                break;
            default:
                ev &= ~MU_KEYBD; /* unknown key */
                break;
            }
    }
    return (ev);
}

#define Inv_Init KM_Init

static long count = 0;
short
Inv_Handler(XEVENT *xev)
{
    short ev = xev->ev_mwich;
    Gem_menu_item *it;
    GRECT area;
    OBJECT *z_ob = zz_oblist[LINES];

    ob_pos(z_ob, LINESLIST, &area);
    if (ev & MU_MESAG) {
        short *buf = xev->ev_mmgpbuf;
        short y_wo, i;

        if (*buf == FNT_CHANGED) {
            if (buf[3] >= 0) {
                mar_set_fontbyid(NHW_MENU, buf[4], buf[5]);
                FontAck(buf[1], 1);
            }
        } else if (*buf == OBJC_CHANGED && buf[3] == LINESLIST) {
            ob_undostate(z_ob, LINESLIST, SELECTED);
            mouse(NULL, &y_wo);
            y_wo = (y_wo - area.g_y) / menu_font.ch + scroll_menu.vpos;
            for (it = invent_list, i = 0; i < y_wo && it;
                 it = it->Gmi_next, i++)
                ;
            if (it && it->Gmi_identifier) {
                it->Gmi_selected = !it->Gmi_selected;
                it->Gmi_count = count == 0L ? -1L : count;
                count = 0L;
                if (Inv_how != PICK_ANY) {
                    /*my_close_dialog(Inv_dialog,TRUE);*/
                    send_return();
                } else {
                    area.g_x = (area.g_x + 23 + 2 * menu_font.cw) & ~7;
                    area.g_w = menu_font.cw;
                    area.g_h = menu_font.ch;
                    area.g_y += (y_wo - scroll_menu.vpos) * menu_font.ch;
                    ob_draw_chg(Inv_dialog, LINESLIST, &area, FAIL);
                }            /* how != PICK_ANY */
            }                /* identifier */
        } else               /* LINESLIST changed */
            ev &= ~MU_MESAG; /* unknown message not used */
    }                        /* MU_MESAG */

    if (ev & MU_KEYBD) {
        char ch = (char) (xev->ev_mkreturn & 0x00FF);

        if (!scroll_top_dialog(ch)) {
            switch (ch) {
            case '0': /* special 0 is also groupaccelerator for balls */
                if (count <= 0)
                    goto find_acc;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (Inv_how == PICK_NONE)
                    goto find_acc;
                count = (count * 10L) + (long) (ch - '0');
                break;
            case '\033': /* cancel - from counting or loop */
                if (count > 0L)
                    count = 0L;
                else {
                    unset_all_on_page(0, (short) scroll_menu.vsize);
                    menu_cancelled = TRUE;
                    my_close_dialog(Inv_dialog, TRUE);
                    return (ev);
                }
                break;
            case '\0': /* finished (commit) */
            case '\n':
            case '\r':
                break;
            case MENU_SELECT_PAGE:
                if (Inv_how == PICK_NONE)
                    goto find_acc;
                if (Inv_how == PICK_ANY)
                    set_all_on_page((short) scroll_menu.vpos,
                                    scroll_menu.vpage);
                break;
            case MENU_SELECT_ALL:
                if (Inv_how == PICK_NONE)
                    goto find_acc;
                if (Inv_how == PICK_ANY)
                    set_all_on_page(0, (short) scroll_menu.vsize);
                break;
            case MENU_UNSELECT_PAGE:
                unset_all_on_page((short) scroll_menu.vpos, scroll_menu.vpage);
                break;
            case MENU_UNSELECT_ALL:
                unset_all_on_page(0, (short) scroll_menu.vsize);
                break;
            case MENU_INVERT_PAGE:
                if (Inv_how == PICK_NONE)
                    goto find_acc;
                if (Inv_how == PICK_ANY)
                    invert_all_on_page((short) scroll_menu.vpos,
                                       scroll_menu.vpage, 0);
                break;
            case MENU_INVERT_ALL:
                if (Inv_how == PICK_NONE)
                    goto find_acc;
                if (Inv_how == PICK_ANY)
                    invert_all_on_page(0, (short) scroll_menu.vsize, 0);
                break;
            case MENU_SEARCH:
                if (Inv_how != PICK_NONE) {
                    char buf[BUFSZ];
                    Gem_getlin("Search for:", buf);
                    if (!*buf || buf[0] == '\033')
                        break;
                    for (it = invent_list; it; it = it->Gmi_next) {
                        if (it->Gmi_identifier && strstr(it->Gmi_str, buf)) {
                            it->Gmi_selected = TRUE;
                            if (Inv_how != PICK_ANY) {
                                my_close_dialog(Inv_dialog, FALSE);
                                break;
                            }
                        }
                    }
                }
                break;
            case C('c'):
                clipbrd_save(invent_list, num_inv_lines,
                             xev->ev_mmokstate & K_SHIFT, TRUE);
                break;
            default:
            find_acc:
                if (Inv_how == PICK_NONE)
                    my_close_dialog(Inv_dialog, TRUE);
                else
                    for (it = invent_list; it; it = it->Gmi_next) {
                        if (it->Gmi_identifier
                            && (it->Gmi_accelerator == ch
                                || it->Gmi_groupacc == ch)) {
                            it->Gmi_selected = !it->Gmi_selected;
                            it->Gmi_count = count == 0L ? -1L : count;
                            count = 0L;
                            if (Inv_how != PICK_ANY) {
                                my_close_dialog(Inv_dialog, TRUE);
                                break;
                            }
                        }
                    }
                break;
            } /* end switch(ch) */
            if (Inv_how == PICK_ANY) {
                area.g_x = (area.g_x + 23 + 2 * menu_font.cw) & ~7;
                area.g_w = menu_font.cw;
                ob_draw_chg(Inv_dialog, LINESLIST, &area, FAIL);
            }
        } /* !scroll_Inv_dialog */
    }     /* MU_KEYBD */

    if (Inv_how == PICK_ANY) {
        ob_set_text(Inv_dialog->di_tree, QLINE, strCancel);
        for (it = invent_list; it; it = it->Gmi_next)
            if (it->Gmi_identifier && it->Gmi_selected) {
                ob_set_text(Inv_dialog->di_tree, QLINE, strOk);
                break;
            }
        ob_draw_chg(Inv_dialog, QLINE, NULL, FAIL);
    }
    return (ev);
}

/************************* draw_window *******************************/

/* Helper: find the OBJECT associated with a window's redraw callback.
   In the old E_GEM API this was stored in WIN.para; we look it up instead. */
static OBJECT *
mar_win_get_obj(WIN *win)
{
    if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window == win)
        return zz_oblist[MSGWIN];
    if (WIN_STATUS != WIN_ERR && Gem_nhwindow[WIN_STATUS].gw_window == win)
        return zz_oblist[STATUSLINE];
    return NULL;
}

static void
mar_draw_window(short msg, WIN *win, GRECT *area)
{
    OBJECT *obj = mar_win_get_obj(win);

    if (obj) {
        obj->ob_x = win->work.g_x;
        obj->ob_y = win->work.g_y;
        if (area == NULL)
            area = &(win->work);
        objc_draw(obj, ROOT, MAX_DEPTH, area->g_x, area->g_y, area->g_w,
                  area->g_h);
    }
}

/************************* mar_display_nhwindow
 * *******************************/

void
redraw_winwork(WIN *w, GRECT *area)
{
    area->g_x += w->work.g_x;
    area->g_y += w->work.g_y;
    redraw_window(w, area);
}
void
mar_menu_set_slider(WIN *p_win)
{
    if (p_win) {
        SCROLL *sc = p_win->scroll;

        if (!sc)
            return;

        if (p_win->gadgets & HSLIDE) {
            long hsize = 1000l;

            if (sc->hsize > 0 && sc->hpage > 0) {
                hsize *= sc->hpage;
                hsize /= sc->hsize;
            }
            window_slider(p_win, HOR_SLIDER, 0, (short) hsize);
        }
        if (p_win->gadgets & VSLIDE) {
            long vsize = 1000l;

            if (sc->vsize > 0 && sc->vpage > 0) {
                vsize *= sc->vpage;
                vsize /= sc->vsize;
            }
            window_slider(p_win, VERT_SLIDER, 0, (short) vsize);
        }
    }
}

void
recalc_msg_win(GRECT *area)
{
    OBJECT *z_ob;
    z_ob = zz_oblist[MSGWIN];
    z_ob[MSGLINES].ob_spec.userblk = &ub_msg;
    z_ob[MSGLINES].ob_width = z_ob[ROOT].ob_width =
        (msg_width + 3) * msg_font.cw;
    z_ob[MSGLINES].ob_width -= z_ob[UPMSG].ob_width;
    z_ob[ROOT].ob_height = z_ob[GRABMSGWIN].ob_height =
        z_ob[MSGLINES].ob_height = msg_vis * msg_font.ch;
    z_ob[DNMSG].ob_y = z_ob[GRABMSGWIN].ob_height - z_ob[DNMSG].ob_height;
    window_border(0, 0, 0, z_ob->ob_width, z_ob->ob_height, area);
}
void
recalc_status_win(GRECT *area)
{
    OBJECT *z_ob;
    z_ob = zz_oblist[STATUSLINE];
    z_ob[ROOT].ob_type = G_USERDEF;
    z_ob[ROOT].ob_spec.userblk = &ub_status;
    z_ob[ROOT].ob_width = (status_w + 2) * status_font.cw;
    z_ob[ROOT].ob_height = z_ob[GRABSTATUS].ob_height = 2 * status_font.ch;
    z_ob[GRABSTATUS].ob_width = 2 * status_font.cw - 2;
    window_border(0, 0, 0, z_ob->ob_width, z_ob->ob_height, area);
}
void
calc_std_winplace(short which, GRECT *place)
{
    static short todo = TRUE;
    static GRECT me, ma, st;

    if (todo || which < 0) {
        OBJECT *z_ob;
        short map_h_off;
        short wc_x, wc_y, wc_w, wc_h;

        /* First the messagewin */
        recalc_msg_win(&me);

        /* Now the map */
        wind_calc(WC_BORDER, MAP_GADGETS, 0, 0,
                  scroll_map.px_hline * (COLNO - 1),
                  scroll_map.px_vline * ROWNO, &wc_x, &wc_y, &wc_w, &wc_h);
        map_h_off = (short)wc_h - scroll_map.px_vline * ROWNO;
        window_border(MAP_GADGETS, 0, 0, scroll_map.px_hline * (COLNO - 1),
                      scroll_map.px_vline * ROWNO, &ma);

        /* Next the statuswin */
        recalc_status_win(&st);

        /* And last but not least a final test */
        ma.g_h = map_h_off + scroll_map.px_vline * ROWNO;
        while (me.g_h + ma.g_h + st.g_h >= desk.g_h)
            ma.g_h -= scroll_map.px_vline;
        /* stack the windows */
        ma.g_y = me.g_y = st.g_y = desk.g_y;
        if (status_align) {
            ma.g_y += st.g_h;
            if (msg_align) {
                st.g_y += me.g_h;
                ma.g_y += me.g_h;
            } else {
                me.g_y += st.g_h + ma.g_h;
            }
        } else {
            if (msg_align) {
                ma.g_y += me.g_h;
            } else {
                me.g_y += ma.g_h;
            }
            st.g_y += me.g_h + ma.g_h;
        }

        if (which)
            todo = FALSE;
    }
    switch (which) {
    case NHW_MESSAGE:
        *place = me;
        break;
    case NHW_MAP:
        *place = ma;
        break;
    case NHW_STATUS:
        *place = st;
        break;
    default:
        break;
    }
}

void
mar_display_nhwindow(winid wind)
{
    DIAINFO *dlg_info;
    OBJECT *z_ob;
    short d_exit = W_ABANDON, i, width, mar_di_mode, tmp_magx = magx;
    GRECT g_mapmax, area;
    char *tmp_button;
    struct gw *p_Gw;

    if (wind == WIN_ERR)
        return;

    p_Gw = &Gem_nhwindow[wind];
    switch (p_Gw->gw_type) {
    case NHW_TEXT:
        if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window)
            mar_display_nhwindow(WIN_MESSAGE);
        z_ob = zz_oblist[LINES];
        scroll_menu.vsize = num_text_lines;
        scroll_menu.vpos = 0;
        if (use_rip) {
            if (!depack_img(planes < 4 ? "RIP2.IMG" : "RIP.IMG",
                            &rip_image)) {
                if (planes >= 16 && rip_image.palette) {
                    MFDB new_mfdb;
                    if (build_truecolor_mfdb(&rip_image, &new_mfdb, planes)) {
                        free(rip_image.addr);
                        rip_image.addr = NULL;
                        Rip_bild = new_mfdb;
                    }
                } else {
                    mfdb(&Rip_bild, (short *) rip_image.addr,
                         rip_image.img_w, rip_image.img_h, 1,
                         rip_image.planes);
                    transform_img(&Rip_bild);
                    if (rip_image.planes > 1 && rip_image.palette)
                        img_set_colors_ex(x_handle, rip_image.palette,
                                          rip_image.planes, 0);
                }
            }
            ub_lines.ub_code = draw_rip;
        } else
            ub_lines.ub_code = draw_lines;
        z_ob[LINESLIST].ob_spec.userblk = &ub_lines;
        width = 16;
        v_set_text(text_font.id, text_font.size, BLACK, 0, 0, vst_out);
        for (i = 0; i < num_text_lines; i++) {
            short eout[8];
            vqt_extent(x_handle, text_lines[i], eout);
            Max(&width, (short)eout[4]);
        }
        scroll_menu.px_vline = text_font.ch;
        scroll_menu.px_hline = text_font.cw;
        mar_di_mode = mar_set_inv_win(num_text_lines, width);
        tmp_button = ob_get_text(z_ob, QLINE, 0);
        ob_set_text(z_ob, QLINE, strOk);
        ob_undoflag(z_ob, LINESLIST, TOUCHEXIT);
        Event_Handler(Text_Init, Text_Handler);
        if ((dlg_info = open_dialog(z_ob, strText, NULL, NULL,
                        mar_ob_mapcenter(z_ob), FALSE,
                        mar_di_mode, FAIL, NULL, NULL)) != NULL) {
            WIN *ptr_win = dlg_info->di_win;

            ptr_win->scroll = &scroll_menu;
            set_slider_colors(ptr_win->handle);
            mar_menu_set_slider(ptr_win);
            WindowItems(ptr_win, SCROLL_KEYS, scroll_keys);
            if ((d_exit = X_Form_Do(NULL)) != W_ABANDON) {
                my_close_dialog(dlg_info, FALSE);
                if (d_exit != W_CLOSED)
                    ob_undostate(z_ob, d_exit & NO_CLICK, SELECTED);
            }
        }
        Event_Handler(NULL, NULL);
        /* RIP.IMG installed a custom palette via preserve_sys=0; restore
           the system palette so any follow-up dialog renders normally.
           No-op in truecolor mode -- RIP didn't touch the workstation
           palette there. */
        if (planes <= 8 && use_rip && normal_palette)
            img_set_colors(x_handle, normal_palette, planes);
        if (use_rip) {
            if (Rip_bild.fd_addr
                && Rip_bild.fd_addr != (short *) rip_image.addr)
                free(Rip_bild.fd_addr);
            Rip_bild.fd_addr = NULL;
            test_free(rip_image.palette);
            rip_image.palette = NULL;
            test_free(rip_image.addr);
            rip_image.addr = NULL;
        }
        ob_set_text(z_ob, QLINE, tmp_button);
        break;
    case NHW_MENU:
        if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window)
            mar_display_nhwindow(WIN_MESSAGE);
        z_ob = zz_oblist[LINES];
        scroll_menu.vsize = num_inv_lines;
        scroll_menu.vpos = 0;
        z_ob[LINESLIST].ob_spec.userblk = &ub_inventory;
        if ((Menu_title)
            && (wind != WIN_INVEN)) /* because I sets no Menu_title */
            Max(&Inv_width, (short) (gr_cw * strlen(Menu_title) + 16));
        scroll_menu.px_vline = menu_font.ch;
        scroll_menu.px_hline = menu_font.cw;
        mar_di_mode = mar_set_inv_win(num_inv_lines, Inv_width);
        tmp_button = ob_get_text(z_ob, QLINE, 0);
        ob_set_text(z_ob, QLINE, Inv_how != PICK_NONE ? strCancel : strOk);
        ob_doflag(z_ob, LINESLIST, TOUCHEXIT);
        count = 0L; /* no count prefix carried over from an earlier menu */
        Event_Handler(Inv_Init, Inv_Handler);
        if ((Inv_dialog = open_dialog(z_ob,
                        (wind == WIN_INVEN)
                            ? "Inventory"
                            : (Menu_title ? Menu_title : "NetHack"),
                        NULL, NULL, mar_ob_mapcenter(z_ob), FALSE,
                        mar_di_mode, FAIL, NULL, NULL)) != NULL) {
            WIN *ptr_win = Inv_dialog->di_win;

            ptr_win->scroll = &scroll_menu;
            set_slider_colors(ptr_win->handle);
            mar_menu_set_slider(ptr_win);
            WindowItems(ptr_win, SCROLL_KEYS, scroll_keys);
            do {
                short y_wo, x_wo, ru_w = 1, ru_h = 1;
                GRECT oarea;
                Gem_menu_item *it;
                d_exit = X_Form_Do(NULL);
                if ((d_exit & NO_CLICK) == LINESLIST) {
                    ob_pos(z_ob, LINESLIST, &oarea);
                    if (mouse(&x_wo, &y_wo) && Inv_how == PICK_ANY) {
                        graf_rt_rubberbox(x_wo, y_wo, 1, 1, 0,
                                          &oarea, &ru_w, &ru_h, NULL);
                        invert_all_on_page(
                            (short) ((y_wo - oarea.g_y) / menu_font.ch
                                   + scroll_menu.vpos),
                            (ru_h + menu_font.ch - 1) / menu_font.ch, 0);
                    } else {
                        for (it = invent_list, i = 0;
                             i < ((y_wo - oarea.g_y) / menu_font.ch
                                  + scroll_menu.vpos)
                             && it;
                             it = it->Gmi_next, i++)
                            ;
                        if (it && it->Gmi_identifier) {
                            it->Gmi_selected = !it->Gmi_selected;
                            it->Gmi_count = count == 0L ? -1L : count;
                            count = 0L;
                            if (Inv_how != PICK_ANY)
                                break;
                        } /* identifier */
                    }
                    oarea.g_x = (oarea.g_x + 23 + 2 * menu_font.cw) & ~7;
                    oarea.g_y = y_wo - (y_wo - oarea.g_y) % menu_font.ch;
                    oarea.g_w = menu_font.cw;
                    oarea.g_h = ((ru_h + menu_font.ch - 1) / menu_font.ch)
                                * menu_font.ch;
                    ob_draw_chg(Inv_dialog, LINESLIST, &oarea, FAIL);
                }
                if (Inv_how == PICK_ANY) {
                    ob_set_text(Inv_dialog->di_tree, QLINE, strCancel);
                    for (it = invent_list; it; it = it->Gmi_next)
                        if (it->Gmi_identifier && it->Gmi_selected) {
                            ob_set_text(Inv_dialog->di_tree, QLINE, strOk);
                            break;
                        }
                    ob_draw_chg(Inv_dialog, QLINE, NULL, FAIL);
                }
            } while ((d_exit & NO_CLICK) == LINESLIST);
            if (d_exit != W_ABANDON) {
                my_close_dialog(Inv_dialog, FALSE);
                if (d_exit != W_CLOSED)
                    ob_undostate(z_ob, d_exit & NO_CLICK, SELECTED);
            }
        }
        Event_Handler(NULL, NULL);
        ob_set_text(z_ob, QLINE, tmp_button);
        break;
    case NHW_MAP:
        if (p_Gw->gw_window == NULL) {
            calc_std_winplace(NHW_MAP, &p_Gw->gw_place);
            window_border(MAP_GADGETS, 0, 0, Tile_width * (COLNO - 1),
                          Tile_height * ROWNO, &g_mapmax);
            p_Gw->gw_window = open_window(
                strMap, strMap, NULL, zz_oblist[NHICON], MAP_GADGETS, TRUE, 128, 128,
                &g_mapmax, &p_Gw->gw_place, &scroll_map, win_draw_map,
                NULL, 0);
            if (p_Gw->gw_window == NULL)
                break;
            set_slider_colors(p_Gw->gw_window->handle);
            WindowItems(p_Gw->gw_window, SCROLL_KEYS - 1,
                        scroll_keys); /* ClrHome centers on u */
            mar_clear_map();
        }
        if (p_Gw->gw_dirty) {
            area.g_x = p_Gw->gw_window->work.g_x
                       + scroll_map.px_hline
                             * (dirty_map_area.g_x - scroll_map.hpos);
            area.g_y = p_Gw->gw_window->work.g_y
                       + scroll_map.px_vline
                             * (dirty_map_area.g_y - scroll_map.vpos);
            area.g_w = (dirty_map_area.g_w - dirty_map_area.g_x + 1)
                       * scroll_map.px_hline;
            area.g_h = (dirty_map_area.g_h - dirty_map_area.g_y + 1)
                       * scroll_map.px_vline;

            redraw_window(p_Gw->gw_window, &area);

            dirty_map_area.g_x = COLNO - 1;
            dirty_map_area.g_y = ROWNO;
            dirty_map_area.g_w = dirty_map_area.g_h = 0;
        }
        break;
    case NHW_MESSAGE:
        if (p_Gw->gw_window == NULL) {
            calc_std_winplace(NHW_MESSAGE, &p_Gw->gw_place);
            z_ob = zz_oblist[MSGWIN];
            magx = 0; /* fake E_GEM to remove Backdropper */
            p_Gw->gw_window = open_window(
                NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL, &p_Gw->gw_place,
                NULL, mar_draw_window, NULL, 0);
            magx = tmp_magx;
            window_size(p_Gw->gw_window, &p_Gw->gw_window->curr);
            p_Gw->gw_dirty = TRUE;
        }

        if (p_Gw->gw_dirty) {
            ob_pos(zz_oblist[MSGWIN], MSGLINES, &area);
            while (messages_per_move > msg_vis) {
                messages_per_move -= msg_vis;
                msg_pos += msg_vis;
                redraw_window(p_Gw->gw_window, &area);
                mar_more();
            }
            msg_pos += messages_per_move;
            messages_per_move = 0;
            if (msg_pos > msg_max)
                msg_pos = msg_max;
            redraw_window(p_Gw->gw_window, &area);
            mar_message_pause = FALSE;
        }
        break;
    case NHW_STATUS:
        if (p_Gw->gw_window == NULL) {
            z_ob = zz_oblist[STATUSLINE];
            calc_std_winplace(NHW_STATUS, &p_Gw->gw_place);
            magx = 0; /* fake E_GEM to remove Backdropper */
            p_Gw->gw_window = open_window(
                NULL, NULL, NULL, NULL, 0, FALSE, 0, 0, NULL, &p_Gw->gw_place,
                NULL, mar_draw_window, NULL, 0);
            magx = tmp_magx;
            /* Because 2*status_font.ch is smaller then e_gem expects the
             * minimum win_height */
            p_Gw->gw_window->min_h = z_ob[ROOT].ob_height;
            window_size(p_Gw->gw_window, &p_Gw->gw_place);
            p_Gw->gw_dirty = TRUE;
            add_dirty_rect(dr_stat, &p_Gw->gw_place);
        }
        while (get_dirty_rect(dr_stat, &area)) {
            area.g_x = (area.g_x + p_Gw->gw_window->work.g_x
                        + 2 * status_font.cw + 6) & ~7;
            area.g_y += p_Gw->gw_window->work.g_y;
            redraw_window(p_Gw->gw_window, &area);
        }
        break;
    default:
        if (p_Gw->gw_dirty)
            redraw_window(p_Gw->gw_window, NULL);
    }
    p_Gw->gw_dirty = FALSE;
}

/************************* create_window *******************************/

int
mar_hol_win_type(int window)
{
    return (Gem_nhwindow[window].gw_type);
}

winid
mar_create_window(short type)
{
    winid newid;
    static char name[] = "Gem";
    short i;
    struct gw *p_Gw = &Gem_nhwindow[0];

    for (newid = 0; newid < MAXWIN && p_Gw->gw_type; newid++, p_Gw++)
        ;

    if (newid == MAXWIN) /* table full; let the caller panic */
        return (newid);

    switch (type) {
    case NHW_MESSAGE:
        message_line = (char **) m_alloc(msg_anz * sizeof(char *));
        message_age = (short *) m_alloc(msg_anz * sizeof(short));
        for (i = 0; i < msg_anz; i++) {
            message_age[i] = FALSE;
            message_line[i] = (char *) m_alloc((MSGLEN + 1) * sizeof(char));
            *message_line[i] = 0;
        }
        break;
    case NHW_STATUS:
        status_line = (char **) m_alloc(2 * sizeof(char *));
        for (i = 0; i < 2; i++) {
            status_line[i] = (char *) m_alloc(status_w * sizeof(char));
            memset(status_line[i], 0, status_w);
        }
        dr_stat = new_dirty_rect(10);
        if (!dr_stat)
            panic("Memory allocation failure (dr_stat)");
        break;
    case NHW_MAP:
        map_glyphs = (char **) m_alloc((long) ROWNO * sizeof(char *));
        map_colors = (short **) m_alloc((long) ROWNO * sizeof(short *));
        for (i = 0; i < ROWNO; i++) {
            map_glyphs[i] = (char *) m_alloc((long) COLNO * sizeof(char));
            map_colors[i] = (short *) m_alloc((long) COLNO * sizeof(short));
            *map_glyphs[i] = map_glyphs[i][COLNO - 1] = 0;
            {
                int xc;
                for (xc = 0; xc < COLNO; xc++)
                    map_colors[i][xc] = WHITE; /* default: visible on black */
            }
        }
        mar_clear_map();
        break;
    case NHW_MENU:
    case NHW_TEXT: /* They are no more treated as dialog */
        break;
    default:
        p_Gw->gw_window = open_window(
            "Misc", name, NULL, NULL, NAME | MOVER | CLOSER, 0, 0, 0, NULL,
            &p_Gw->gw_place, NULL, NULL, NULL, 0);
        break;
    }

    p_Gw->gw_type = type;

    return (newid);
}

void
mar_change_menu_2_text(winid win)
{
    Gem_nhwindow[win].gw_type = NHW_TEXT;
}

/************************* mar_clear_map *******************************/

void
mar_clear_map(void)
{
    short pla[8];
    short x, y;

    pla[0] = pla[1] = pla[4] = pla[5] = 0;
    pla[2] = pla[6] = scroll_map.px_hline * (COLNO - 1) - 1;
    pla[3] = pla[7] = scroll_map.px_vline * ROWNO - 1;
    for (y = 0; y < ROWNO; y++)
        for (x = 0; x < COLNO - 1; x++)
            map_glyphs[y][x] = ' ';
    vro_cpyfm(x_handle, ALL_BLACK, pla, &Tile_bilder, &Map_bild);
    if (WIN_MAP != WIN_ERR && Gem_nhwindow[WIN_MAP].gw_window)
        redraw_window(Gem_nhwindow[WIN_MAP].gw_window, NULL);
}

/************************* destroy_window *******************************/

void
mar_destroy_nhwindow(int window)
{
    short i;

    switch (Gem_nhwindow[window].gw_type) {
    case NHW_TEXT:
        for (i = 0; i < num_text_lines; i++)
            free(text_lines[i]);
        null_free(text_lines);
        null_free(text_line_glyph);
        num_text_lines = 0;
        use_rip = FALSE;
        break;
    case NHW_MENU:
        Gem_start_menu(window, 0UL); /* delete invent_list */
        test_free(Menu_title);
        break;
    case 0: /* No window available, probably an error message? */
        break;
    default:
        close_window(Gem_nhwindow[window].gw_window, 0);
        break;
    }
    Gem_nhwindow[window].gw_window = NULL;
    Gem_nhwindow[window].gw_type = 0;
    Gem_nhwindow[window].gw_dirty = FALSE;

    if (window == WIN_MAP) {
        for (i = 0; i < ROWNO; i++) {
            free(map_glyphs[i]);
            if (map_colors) free(map_colors[i]);
        }
        null_free(map_glyphs);
        if (map_colors) null_free(map_colors);
        WIN_MAP = WIN_ERR;
    }
    if (window == WIN_STATUS) {
        for (i = 0; i < 2; i++)
            free(status_line[i]);
        null_free(status_line);
        WIN_STATUS = WIN_ERR;
    }
    if (window == WIN_MESSAGE) {
        for (i = 0; i < msg_anz; i++)
            free(message_line[i]);
        null_free(message_line);
        null_free(message_age);
        WIN_MESSAGE = WIN_ERR;
    }
    if (window == WIN_INVEN)
        WIN_INVEN = WIN_ERR;
}

/************************* nh_poskey *******************************/

/* Non-blocking key probe.  AES events don't peek; once an event is
   returned by evnt_multi it is dequeued.  So mar_kbhit() consumes any
   pending key event into mar_kbhit_buf_*, and mar_nh_poskey() replays
   that buffered key on its next call instead of polling the AES queue.
   This lets src/allmain.c kbhit()-then-pgetchar() interrupt long
   occupations (eat / dig / travel) without the BIOS Cconis() syscall
   round-trip per turn under MiNT. */
static short mar_kbhit_buf_kreturn = 0;
static short mar_kbhit_buf_kstate = 0;
static short mar_kbhit_buf_valid = 0;

short
mar_kbhit(void)
{
    XEVENT probe;
    short ev;

    if (mar_kbhit_buf_valid)
        return 1;

    memset(&probe, 0, sizeof(probe));
    probe.ev_mflags = MU_TIMER | MU_KEYBD;
    probe.ev_mt1locount = 0;
    probe.ev_mt1hicount = 0;
    ev = Event_Multi(&probe);

    if (ev & MU_KEYBD) {
        mar_kbhit_buf_kreturn = probe.ev_mkreturn;
        mar_kbhit_buf_kstate = probe.ev_mmokstate;
        mar_kbhit_buf_valid = 1;
        return 1;
    }
    return 0;
}

void
mar_set_margin(short m)
{
    Max(&m, 0);
    Min(&m,
        min(ROWNO, COLNO)); /* the larger the less sense */
    scroll_margin = m;
}
void
mar_cliparound(void)
{
    if (WIN_MAP != WIN_ERR && Gem_nhwindow[WIN_MAP].gw_window) {
        short width = scroll_margin > 0 ? scroll_margin
                                       : max(scroll_map.hpage / 4, 1),
            height = scroll_margin > 0 ? scroll_margin
                                      : max(scroll_map.vpage / 4, 1),
            adjust_needed;
        adjust_needed = FALSE;
        if ((map_cursx < scroll_map.hpos + width)
            || (map_cursx >= scroll_map.hpos + scroll_map.hpage - width)) {
            scroll_map.hpos = map_cursx - scroll_map.hpage / 2;
            adjust_needed = TRUE;
        }
        if ((map_cursy < scroll_map.vpos + height)
            || (map_cursy >= scroll_map.vpos + scroll_map.vpage - height)) {
            scroll_map.vpos = map_cursy - scroll_map.vpage / 2;
            adjust_needed = TRUE;
        }
        if (adjust_needed)
            scroll_window(Gem_nhwindow[WIN_MAP].gw_window, WIN_SCROLL, NULL);
    }
}

void
mar_update_value(void)
{
    if (WIN_MESSAGE != WIN_ERR) {
        mar_message_pause = FALSE;
        mar_esc_pressed = FALSE;
        mar_display_nhwindow(WIN_MESSAGE);
    }

    if (WIN_MAP != WIN_ERR)
        mar_cliparound();

    if (WIN_STATUS != WIN_ERR) {
        mar_check_hilight_status();
        mar_display_nhwindow(WIN_STATUS);
    }
}

short
Main_Init(XEVENT *xev, short availiable)
{
    xev->ev_mb1mask = xev->ev_mb1state = 1;
    xev->ev_mb1clicks = xev->ev_mb2clicks = xev->ev_mb2mask =
        xev->ev_mb2state = 2;
    return ((MU_KEYBD | MU_BUTTON1 | MU_BUTTON2 | MU_MESAG) & availiable);
}

/*
 * return a key, or 0, in which case a mouse button was pressed
 * mouse events should be returned as character postitions in the map window.
 */
/*ARGSUSED*/
short
mar_nh_poskey(short *x, short *y, short *mod)
{
    static XEVENT xev;
    short retval, ev;

    do {
    if (mar_kbhit_buf_valid) {
        /* Replay key consumed by a prior mar_kbhit() probe. */
        xev.ev_mkreturn = mar_kbhit_buf_kreturn;
        xev.ev_mmokstate = mar_kbhit_buf_kstate;
        mar_kbhit_buf_valid = 0;
        ev = MU_KEYBD;
    } else {
        xev.ev_mflags = Main_Init(&xev, 0xFFFF);
        ev = Event_Multi(&xev);
    }

    retval = FAIL;

    if (ev & MU_KEYBD) {
        char ch = xev.ev_mkreturn & 0x00FF;
        char scan = (xev.ev_mkreturn & 0xff00) >> 8;
        short shift = xev.ev_mmokstate;
        const struct pad *kpad;

        /* Translate keypad keys */
        if (iskeypad(scan)) {
            kpad = mar_iflags_numpad() == 1 ? numpad : keypad;
            if (shift & K_SHIFT)
                ch = kpad[scan - KEYPADLO].shift;
            else if (shift & K_CTRL) {
                if (scan >= 0x67 && scan <= 0x6f && scan != 0x6b) {
                    send_key(kpad[scan - KEYPADLO].normal);
                    ch = 'g';
                } else {
                    ch = kpad[scan - KEYPADLO].cntrl;
                }
            } else
                ch = kpad[scan - KEYPADLO].normal;
        }
        if (scan == SCANHOME)
            mar_cliparound();
        else if (scan == SCANF1)
            retval = 'h';
        else if (scan == SCANF2) {
            mar_set_tile_mode(!mar_set_tile_mode(FAIL));
            /* Wipe the map work area before the redraw so leftover
               pixels at the old cell size do not show through until
               doredraw() repaints. */
            mar_clear_map();
            retval = C('r'); /* trigger full-redraw via doredraw() */
        } else if (scan == SCANF3) {
            draw_cursor = !draw_cursor;
            mar_curs(map_cursx, map_cursy);
            mar_display_nhwindow(WIN_MAP);
        } else if (scan == SCANF4) { /* Font-Selector */
            if (!CallFontSelector(0, FAIL, FAIL, FAIL, FAIL)) {
                xalert(1, 1, X_ICN_ALERT, NULL, SYS_MODAL, BUTTONS_RIGHT,
                       TRUE, "Hello", "Fontselector not available!", NULL);
            }
        } else if (!ch && shift & K_CTRL && scan == -57) {
            /* ignore Ctrl-Alt-Clr/Home == MagiC's restore
             * screen */
        } else {
            if (!ch)
                ch = (char) M(tolower(scan_2_ascii(xev.ev_mkreturn, shift)));
            if (((short) ch) == -128)
                ch = '\033';
            retval = ch;
        }
    }

    if (ev & MU_BUTTON1 || ev & MU_BUTTON2) {
        short ex = xev.ev_mmox, ey = xev.ev_mmoy;
        WIN *akt_win = window_find(ex, ey);

        if (WIN_MAP != WIN_ERR
            && akt_win == Gem_nhwindow[WIN_MAP].gw_window
            && rc_inside(ex, ey, &akt_win->work)) {
            /* rc_inside guard: window_find/wind_find can return the
               map window for clicks just outside its current work
               rect (title bar, scroll bars, or stale post-move
               hit-test).  Without the guard, the clamp below maps
               those clicks to map cell (0,0) and the player walks
               toward the top-left corner. */
            *x = max(min((ex - akt_win->work.g_x) / scroll_map.px_hline
                             + scroll_map.hpos,
                         COLNO - 1),
                     0) + 1;
            *y = max(min((ey - akt_win->work.g_y) / scroll_map.px_vline
                             + scroll_map.vpos,
                         ROWNO),
                     0);
            *mod = xev.ev_mmobutton;
            retval = 0;
        } else if (WIN_STATUS != WIN_ERR
                   && akt_win == Gem_nhwindow[WIN_STATUS].gw_window) {
            move_win(akt_win);
        } else if (WIN_MESSAGE != WIN_ERR
                   && akt_win == Gem_nhwindow[WIN_MESSAGE].gw_window) {
            message_handler(ex, ey);
        }
    }

    if (ev & MU_MESAG) {
        short *buf = xev.ev_mmgpbuf;
        char *str;
        OBJECT *z_ob = zz_oblist[MENU];

        switch (*buf) {
        case MN_SELECTED:
            menu_tnormal(z_ob, buf[3], TRUE); /* unselect menu header */
            if (buf[4] == DOQUIT) {
                done2(); /* Quit without saving */
                break;
            }
            str = ob_get_text(z_ob, buf[4], 0);
            str += strlen(str) - 2;
            switch (*str) {
            case ' ': /* just that command */
                retval = str[1];
                break;
            case '\005': /* Alt command */
            case '\007':
                retval = M(str[1]);
                break;
            case '^': /* Ctrl command */
                retval = C(str[1]);
                break;
            case 'f': /* Func Key */
                switch (str[1]) {
                case '1':
                    retval = 'h';
                    break;
                case '2':
                    mar_set_tile_mode(!mar_set_tile_mode(FAIL));
                    retval = C('r'); /* trigger full-redraw via doredraw() */
                    break;
                case '3':
                    draw_cursor = !draw_cursor;
                    mar_curs(map_cursx, map_cursy);
                    mar_display_nhwindow(WIN_MAP);
                    break;
                default:
                }
                break;
            default:
                mar_about();
                break;
            }
            break; /* MN_SELECTED */
        case WM_TOPPED:
        case WM_ONTOP:
            /* In palettized screen modes, another app (MagiC desktop,
               accessories, other GEM programs) installs its own VDI
               palette when active.  Re-install ours so the tile colors
               are correct when the user returns to NetHack. */
            if (tile_image.planes > 1 && tile_image.palette)
                img_set_colors(x_handle, tile_image.palette,
                               tile_image.planes);
            break;
        case WM_CLOSED:
            WindowHandler(W_ICONIFYALL, NULL, NULL);
            break;
        case WM_MOVED:
            /* Route the move through window_size so EGEM also
               recalculates win->work (the inner work-area rect that
               win_draw_map uses to place tiles on screen).  Updating
               win->curr alone leaves win->work stale and the map
               keeps drawing to the previous screen position.

               Pre-clamp the requested delta so the message/status
               chrome stays on screen; the map then stops at the same
               boundary instead of sliding under the chrome.  After
               committing, translate the chrome by the actual (post-
               clamp) delta -- GEM has no parent/child windows; this
               is manual lockstep. */
            if (WIN_MAP != WIN_ERR
                && Gem_nhwindow[WIN_MAP].gw_window
                && buf[3] == Gem_nhwindow[WIN_MAP].gw_window->handle) {
                WIN *w = Gem_nhwindow[WIN_MAP].gw_window;
                short old_x = w->curr.g_x, old_y = w->curr.g_y;
                short dx = (short) (buf[4] - w->curr.g_x);
                short dy = (short) (buf[5] - w->curr.g_y);
                GRECT nc;
                if (WIN_MESSAGE != WIN_ERR
                    && Gem_nhwindow[WIN_MESSAGE].gw_window) {
                    WIN *mw = Gem_nhwindow[WIN_MESSAGE].gw_window;
                    short min_dy = (short) (desk.g_y - mw->curr.g_y);
                    if (dy < min_dy) dy = min_dy;
                }
                if (WIN_STATUS != WIN_ERR
                    && Gem_nhwindow[WIN_STATUS].gw_window) {
                    WIN *sw = Gem_nhwindow[WIN_STATUS].gw_window;
                    short max_dy = (short) (desk.g_y + desk.g_h
                                            - sw->curr.g_y - sw->curr.g_h);
                    if (dy > max_dy) dy = max_dy;
                }
                nc.g_x = (short) (w->curr.g_x + dx);
                nc.g_y = (short) (w->curr.g_y + dy);
                nc.g_w = w->curr.g_w; nc.g_h = w->curr.g_h;
                mar_map_resized(&nc);
                mar_shift_chrome_windows((short) (w->curr.g_x - old_x),
                                         (short) (w->curr.g_y - old_y));
            }
            break;
        case WM_SIZED:
            if (WIN_MAP != WIN_ERR
                && Gem_nhwindow[WIN_MAP].gw_window
                && buf[3] == Gem_nhwindow[WIN_MAP].gw_window->handle) {
                GRECT nc;
                nc.g_x = buf[4]; nc.g_y = buf[5];
                nc.g_w = buf[6]; nc.g_h = buf[7];
                mar_map_resized(&nc);
            }
            break;
        case WM_FULLED:
            /* Toggle between max and the previous user size.  EGEM
               tracks win->prev (set by window_size on every resize). */
            if (WIN_MAP != WIN_ERR
                && Gem_nhwindow[WIN_MAP].gw_window
                && buf[3] == Gem_nhwindow[WIN_MAP].gw_window->handle) {
                WIN *w = Gem_nhwindow[WIN_MAP].gw_window;
                GRECT nc =
                    rc_equal(&w->curr, &w->max) ? w->prev : w->max;
                mar_map_resized(&nc);
            }
            break;
        case AP_TERM:
            retval = 'S';
            break;
        case FNT_CHANGED:
            if (buf[3] >= 0) {
                if (WIN_MESSAGE != WIN_ERR
                    && Gem_nhwindow[WIN_MESSAGE].gw_window
                    && buf[3] == Gem_nhwindow[WIN_MESSAGE].gw_window->handle) {
                    mar_set_fontbyid(NHW_MESSAGE, buf[4], buf[5]);
                    mar_display_nhwindow(WIN_MESSAGE);
                } else if (WIN_MAP != WIN_ERR
                           && Gem_nhwindow[WIN_MAP].gw_window
                           && buf[3]
                                  == Gem_nhwindow[WIN_MAP].gw_window->handle) {
                    mar_set_fontbyid(NHW_MAP, buf[4], buf[5]);
                    mar_display_nhwindow(WIN_MAP);
                } else if (WIN_STATUS != WIN_ERR
                           && Gem_nhwindow[WIN_STATUS].gw_window
                           && buf[3]
                                  == Gem_nhwindow[WIN_STATUS].gw_window
                                         ->handle) {
                    mar_set_fontbyid(NHW_STATUS, buf[4], buf[5]);
                    mar_display_nhwindow(WIN_STATUS);
                }
                FontAck(buf[1], 1);
            }
            break;
        default:
            break;
        }
    } /* MU_MESAG */

    } while (retval == FAIL);

    return (retval);
}

int
Gem_nh_poskey(coordxy *x, coordxy *y, int *mod)
{
    short sx, sy, smod;
    int ret;
    mar_update_value();
    ret = mar_nh_poskey(&sx, &sy, &smod);
    *x = sx;
    *y = sy;
    *mod = smod;
    return ret;
}

void
Gem_delay_output(void)
{
    Event_Timer(50, 0, TRUE); /* wait 50ms */
}

int
Gem_doprev_message(void)
{
    if (msg_pos > 2) {
        msg_pos--;
        if (WIN_MESSAGE != WIN_ERR)
            Gem_nhwindow[WIN_MESSAGE].gw_dirty = TRUE;
        mar_display_nhwindow(WIN_MESSAGE);
    }
    return (0);
}

/************************* print_glyph *******************************/

short mar_set_rogue(short);

short
mar_set_tile_mode(short tiles)
{
    static short tile_mode = TRUE;
    static GRECT prev;
    short err;
    WIN *z_w = WIN_MAP != WIN_ERR ? Gem_nhwindow[WIN_MAP].gw_window : NULL;

    if (tiles < 0)
        return (tile_mode);
    else if (!z_w)
        tile_mode = tiles;
    else if (tile_mode == tiles || (mar_set_rogue(FAIL) && tiles))
        return (FAIL);
    else if (tiles && (err = load_tile_image()) != 0) {
        /* keep the ascii map if the tile sheet will not load */
        img_error(err);
        return (FAIL);
    } else {
        GRECT tmp;

        tile_mode = tiles;
        scroll_map.px_hline = tiles ? Tile_width : map_font.cw;
        scroll_map.px_vline = tiles ? Tile_height : map_font.ch;
        window_border(MAP_GADGETS, 0, 0, scroll_map.px_hline * (COLNO - 1),
                      scroll_map.px_vline * ROWNO, &tmp);
        z_w->max.g_w = tmp.g_w;
        z_w->max.g_h = tmp.g_h;
        if (tiles)
            z_w->curr = prev;
        else
            prev = z_w->curr;

        window_reinit(z_w, strMap, strMap, NULL, FALSE, 0);
    }
    return (FAIL);
}

short
mar_set_rogue(short what)
{
    static short rogue = FALSE, prev_mode = TRUE;

    if (what < 0)
        return (rogue);
    if (what != rogue) {
        rogue = what;
        if (rogue) {
            prev_mode = mar_set_tile_mode(FAIL);
            mar_set_tile_mode(FALSE);
        } else
            mar_set_tile_mode(prev_mode);
    }
    return (FAIL);
}

void
mar_add_pet_sign(winid window, short x, short y)
{
    if (window != WIN_ERR && window == WIN_MAP) {
        static short pla[8] = { 0, 0, 7, 7, 0, 0, 0, 0 },
                     colindex[2] = { RED, WHITE };

        pla[4] = pla[6] = scroll_map.px_hline * x;
        pla[5] = pla[7] = scroll_map.px_vline * y;
        pla[6] += 7;
        pla[7] += 6;
        vrt_cpyfm(x_handle, MD_TRANS, pla, &Pet_Mark, &Map_bild, colindex);
    }
}

void
mar_print_glyph(winid window, short x, short y, short gl, short bkgl)
{
    if (window != WIN_ERR && window == WIN_MAP) {
        static short pla[8];

        pla[2] = pla[0] = (gl % Tiles_per_line) * Tile_width;
        pla[3] = pla[1] = (gl / Tiles_per_line) * Tile_height;
        pla[2] += Tile_width - 1;
        pla[3] += Tile_height - 1;
        pla[6] = pla[4] = Tile_width * x;  /* x_wert to */
        pla[7] = pla[5] = Tile_height * y; /* y_wert to */
        pla[6] += Tile_width - 1;
        pla[7] += Tile_height - 1;

        vro_cpyfm(x_handle, gl != -1 ? S_ONLY : ALL_BLACK, pla, &Tile_bilder,
                  &Map_bild);
    }
}

void
mar_print_char(winid window, coordxy x, coordxy y, char ch, short col)
{
    if (window != WIN_ERR && window == WIN_MAP) {
        map_glyphs[y][x] = ch;
        if (map_colors)
            map_colors[y][x] = (col >= 0 && col < 16)
                ? nhclr_to_pen[col] : pen_white;
    }
}

/************************* getlin *******************************/

void
Gem_getlin(const char *ques, char *input)
{
    OBJECT *z_ob = zz_oblist[LINEGET];
    short d_exit, length;
    char *pr[2], *tmp;
    char ques_buf[128];

    if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window)
        mar_display_nhwindow(WIN_MESSAGE);

    z_ob[LGPROMPT].ob_type = G_USERDEF;
    z_ob[LGPROMPT].ob_spec.userblk = &ub_prompt;
    z_ob[LGPROMPT].ob_height = 2 * gr_ch;

    (void) strncpy(ques_buf, ques, sizeof(ques_buf) - 1);
    ques_buf[sizeof(ques_buf) - 1] = '\0';

    length = z_ob[LGPROMPT].ob_width / gr_cw;
    if ((short) strlen(ques_buf) > length) {
        tmp = ques_buf + length;
        while (tmp >= ques_buf && *tmp != ' ') {
            tmp--;
        }
        if (tmp <= ques_buf)
            tmp = ques_buf + length; /* Mar -- Oops, what a word :-) */
        pr[0] = ques_buf;
        *tmp = 0;
        pr[1] = ++tmp;
    } else {
        pr[0] = ques_buf;
        pr[1] = NULL;
    }
    ub_prompt.ub_parm = (long) pr;

    ob_clear_edit(z_ob);
    d_exit = xdialog(z_ob, nullstr, NULL, NULL, mar_ob_mapcenter(z_ob), FALSE,
                     DIALOG_MODE);
    Event_Timer(0, 0, TRUE);

    if (d_exit == W_CLOSED || d_exit == W_ABANDON
        || (d_exit & NO_CLICK) == QLG) {
        *input = '\033';
        input[1] = 0;
    } else {
        strncpy(input, ob_get_text(z_ob, LGREPLY, 0), BUFSZ - 1);
        input[BUFSZ - 1] = '\0';
    }
}

/************************* ask_direction *******************************/

#define Dia_Init K_Init

short
Dia_Handler(XEVENT *xev)
{
    short ev = xev->ev_mwich;
    char ch = (char) (xev->ev_mkreturn & 0x00FF);

    if (ev & MU_KEYBD) {
        WIN *w;
        DIAINFO *dinf;

        switch (ch) {
        case 's':
            send_key((short) (mar_iflags_numpad() ? '5' : '.'));
            break;
        case '.':
            send_key('5'); /*'.' is a button if numpad isn't set */
            break;
        case '\033': /*ESC*/
            if ((w = get_top_window()) && (dinf = (DIAINFO *) w->dialog)
                && dinf->di_tree == zz_oblist[DIRECTION]) {
                my_close_dialog(dinf, FALSE);
                break;
            }
        /* Fall thru */
        default:
            ev &= ~MU_KEYBD; /* let the dialog handle it */
            break;
        }
    }
    return (ev);
}

short
mar_ask_direction(void)
{
    short d_exit;
    OBJECT *z_ob = zz_oblist[DIRECTION];

    Event_Handler(Dia_Init, Dia_Handler);
    mar_set_dir_keys();
    d_exit = xdialog(z_ob, nullstr, NULL, NULL, mar_ob_mapcenter(z_ob), FALSE,
                     DIALOG_MODE);
    Event_Timer(0, 0, TRUE);
    Event_Handler(NULL, NULL);

    if (d_exit == W_CLOSED || d_exit == W_ABANDON)
        return ('\033');
    if ((d_exit & NO_CLICK) == DIRDOWN)
        return ('>');
    if ((d_exit & NO_CLICK) == DIRUP)
        return ('<');
    if ((d_exit & NO_CLICK) == (DIR1 + 8)) /* 5 or . */
        return ('.');
    return (*ob_get_text(z_ob, d_exit & NO_CLICK, 0));
}

/************************* yn_function *******************************/

#define any_init M_Init

static short
any_handler(XEVENT *xev)
{
    short ev = xev->ev_mwich;

    if (ev & MU_MESAG) {
        short *buf = xev->ev_mmgpbuf;

        if (*buf == OBJC_EDITED)
            my_close_dialog(*(DIAINFO **) &buf[4], FALSE);
        else
            ev &= ~MU_MESAG;
    }
    return (ev);
}

short
send_yn_esc(char ch)
{
    static char esc_char = 0;

    if (ch < 0) {
        if (esc_char) {
            send_key((short) esc_char);
            return (TRUE);
        }
        return (FALSE);
    } else
        esc_char = ch;
    return (TRUE);
}

#define single_init K_Init

static short
single_handler(XEVENT *xev)
{
    short ev = xev->ev_mwich;

    if (ev & MU_KEYBD) {
        char ch = (char) (xev->ev_mkreturn & 0x00FF);
        WIN *w;
        DIAINFO *dinf;

        switch (ch) {
        case ' ':
            send_return();
            break;
        case '\033':
            if ((w = get_top_window()) && (dinf = (DIAINFO *) w->dialog)
                && dinf->di_tree == zz_oblist[YNCHOICE]) {
                if (!send_yn_esc(FAIL))
                    my_close_dialog(dinf, FALSE);
                break;
            }
        /* Fall thru */
        default:
            ev &= ~MU_KEYBD;
        }
    }
    return (ev);
}

char
Gem_yn_function(const char *query, const char *resp, char def)
{
    OBJECT *z_ob = zz_oblist[YNCHOICE];
    short d_exit, i, len;
    long anzahl;
    char *tmp;
    const char *ptr;

    if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window)
        mar_display_nhwindow(WIN_MESSAGE);

    /* if query for direction the special dialog */
    if (strstr(query, "irect"))
        return (mar_ask_direction());

    len = min(strlen(query), (max_w - 8 * gr_cw) / gr_cw);
    z_ob[ROOT].ob_width = (len + 8) * gr_cw;
    z_ob[YNPROMPT].ob_width = gr_cw * len + 8;
    tmp = ob_get_text(z_ob, YNPROMPT, 0);
    ob_set_text(z_ob, YNPROMPT, mar_copy_of(query));

    if (resp) { /* single inputs */
        ob_hide(z_ob, SOMECHARS, FALSE);
        ob_hide(z_ob, ANYCHAR, TRUE);

        if (strchr(resp, 'q'))
            send_yn_esc('q');
        else if (strchr(resp, 'n'))
            send_yn_esc('n');
        else
            send_yn_esc(
                def); /* strictly def should be returned, but in trad. I it's
                         0 */

        if (strchr(resp, '#')) { /* count possible */
            ob_hide(z_ob, YNOK, FALSE);
            ob_hide(z_ob, COUNT, FALSE);
        } else { /* no count */
            ob_hide(z_ob, YNOK, TRUE);
            ob_hide(z_ob, COUNT, TRUE);
        }

        {
            const char *esc = strchr(resp, '\033');
            if (esc)
                anzahl = esc - resp;
            else
                anzahl = strlen(resp);
        }
        for (i = 0, ptr = resp; i < 2 * anzahl; i += 2, ptr++) {
            ob_hide(z_ob, YN1 + i, FALSE);
            mar_change_button_char(z_ob, YN1 + i, *ptr);
            ob_undoflag(z_ob, YN1 + i, DEFAULT);
            if (*ptr == def)
                ob_doflag(z_ob, YN1 + i, DEFAULT);
        }

        z_ob[SOMECHARS].ob_width = z_ob[YN1 + i].ob_x + 8;
        z_ob[SOMECHARS].ob_height = z_ob[YN1 + i].ob_y + gr_ch + gr_ch / 2;
        {
            int proposed = z_ob[SOMECHARS].ob_width + 4 * gr_cw;
            if (proposed > z_ob[ROOT].ob_width)
                z_ob[ROOT].ob_width = proposed;
        }
        z_ob[ROOT].ob_height = z_ob[SOMECHARS].ob_height + 4 * gr_ch;
        if (strchr(resp, '#'))
            z_ob[ROOT].ob_height = z_ob[YNOK].ob_y + 2 * gr_ch;

        for (i += YN1; i < (YNN + 1); i += 2) {
            ob_hide(z_ob, i, TRUE);
        }
        Event_Handler(single_init, single_handler);
    } else { /* any input */
        ob_hide(z_ob, SOMECHARS, TRUE);
        ob_hide(z_ob, ANYCHAR, FALSE);
        ob_hide(z_ob, YNOK, TRUE);
        ob_hide(z_ob, COUNT, TRUE);
        z_ob[ANYCHAR].ob_height = 2 * gr_ch;
        z_ob[CHOSENCH].ob_y = z_ob[CHOSENCH + 1].ob_y = gr_ch / 2;
        z_ob[ROOT].ob_width =
            max(z_ob[YNPROMPT].ob_width + z_ob[YNPROMPT].ob_x,
                z_ob[ANYCHAR].ob_width + z_ob[ANYCHAR].ob_x) + 2 * gr_cw;
        z_ob[ROOT].ob_height =
            z_ob[ANYCHAR].ob_height + z_ob[ANYCHAR].ob_y + gr_ch / 2;
        *ob_get_text(z_ob, CHOSENCH, 0) = '?';
        Event_Handler(any_init, any_handler);
    }

    d_exit = xdialog(z_ob, nullstr, NULL, NULL, mar_ob_mapcenter(z_ob), FALSE,
                     DIALOG_MODE);
    Event_Timer(0, 0, TRUE);
    Event_Handler(NULL, NULL);
    /* display of count is missing (through the core too) */

    free(ob_get_text(z_ob, YNPROMPT, 0));
    ob_set_text(z_ob, YNPROMPT, tmp);

    if (resp && (d_exit == W_CLOSED || d_exit == W_ABANDON))
        return ('\033');
    if ((d_exit & NO_CLICK) == YNOK) {
        yn_number = atol(ob_get_text(z_ob, COUNT, 0));
        return ('#');
    }
    if (!resp)
        return (*ob_get_text(z_ob, CHOSENCH, 0));
    return (*ob_get_text(z_ob, d_exit & NO_CLICK, 0));
}

/*
 * Allocate a copy of the given string.  If null, return a string of
 * zero length.
 *
 * This is an exact duplicate of copy_of() in X11/winmenu.c.
 */
static char *
mar_copy_of(const char *s)
{
    if (!s)
        s = nullstr;
    return strcpy((char *) m_alloc((unsigned) (strlen(s) + 1)), s);
}

const char *strRP = "raw_print", *strRPB = "raw_print_bold";

void
mar_raw_print(const char *str)
{
    xalert(1, FAIL, X_ICN_INFO, NULL, APPL_MODAL, BUTTONS_CENTERED, TRUE,
           strRP, str, NULL);
}

void
mar_raw_print_bold(const char *str)
{
    char buf[BUFSZ];

    snprintf(buf, sizeof buf, "!%s", str);
    xalert(1, FAIL, X_ICN_INFO, NULL, APPL_MODAL, BUTTONS_CENTERED, TRUE,
           strRPB, buf, NULL);
}

/*wingem1.c*/

/* NetHack 3.7	winmap.c	$NHDT-Date: 1643328598 2022/01/28 00:09:58 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.49 $ */
/* Copyright (c) Dean Luick, 1992                                 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains:
 *      + global functions print_glyph() and cliparound()
 *      + the map window routines
 *      + the char and pointer input routines
 *
 * Notes:
 *      + We don't really have a good way to get the compiled ROWNO and
 *        COLNO as defaults.  They are hardwired to the current "correct"
 *        values in the Window widget.  I am _not_ in favor of including
 *        some nethack include file for Window.c.
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV /* X11 include files may define SYSV */
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xatom.h>

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#include "xwindow.h" /* map widget declarations */

#include "hack.h"
#include "dlb.h"
#include "winX.h"

#ifdef USE_XPM
#include <X11/xpm.h>
#endif

/* from tile.c */
extern int total_tiles_used, Tile_corr;

/* Define these if you really want a lot of junk on your screen. */
/* #define VERBOSE */        /* print various info & events as they happen */
/* #define VERBOSE_UPDATE */ /* print screen update bounds */
/* #define VERBOSE_INPUT */  /* print input events */

#define USE_WHITE /* almost always use white as a tile cursor border */

#define COL0_OFFSET 1 /* change to 0 to revert to displaying unused column 0 */

static boolean init_tiles(struct xwindow *);
static void set_button_values(Widget, int, int, unsigned);
static void map_check_size_change(struct xwindow *);
static void map_update(struct xwindow *, int, int, int, int, boolean);
static void init_text(struct xwindow *);
static void map_exposed(Widget, XtPointer, XtPointer);
static void set_gc(Widget, Font, const char *, Pixel, GC *, GC *);
static void get_text_gc(struct xwindow *, Font);
static void map_all_unexplored(struct map_info_t *);
static void get_char_info(struct xwindow *);
static void display_cursor(struct xwindow *);

/* Global functions ======================================================= */

void
X11_print_glyph(
    winid window,
    coordxy x, coordxy y,
    const glyph_info *glyphinfo,
    const glyph_info *bkglyphinfo UNUSED)
{
    struct map_info_t *map_info;
    boolean update_bbox = FALSE;

    check_winid(window);
    if (window_list[window].type != NHW_MAP) {
        impossible("print_glyph: can (currently) only print to map windows");
        return;
    }
    map_info = window_list[window].map_information;

    /* update both the tile and text backing stores */
    {
        unsigned short *g_ptr = &map_info->tile_map.glyphs[y][x].glyph,
                       *t_ptr = &map_info->tile_map.glyphs[y][x].tileidx;

        if (*g_ptr != glyphinfo->glyph) {
            *g_ptr = glyphinfo->glyph;
            if (map_info->is_tile)
                update_bbox = TRUE;
        }
        if (*t_ptr != glyphinfo->gm.tileidx) {
            *t_ptr = glyphinfo->gm.tileidx;
            if (map_info->is_tile)
                update_bbox = TRUE;
        }
    }
    {
        uchar ch;
        register unsigned char *ch_ptr;
        int color, och;
        unsigned special;
#ifdef TEXTCOLOR
        int colordif;
        register unsigned char *co_ptr;
#endif

        color = glyphinfo->gm.sym.color;
        special = glyphinfo->gm.glyphflags;
        och = glyphinfo->ttychar;
        ch = (uchar) och;

        if (special != map_info->tile_map.glyphs[y][x].glyphflags) {
            map_info->tile_map.glyphs[y][x].glyphflags = special;
            update_bbox = TRUE;
        }

        /* Only update if we need to. */
        ch_ptr = &map_info->text_map.text[y][x];
        if (*ch_ptr != ch) {
            *ch_ptr = ch;
            if (!map_info->is_tile)
                update_bbox = TRUE;
        }
#ifdef TEXTCOLOR
        co_ptr = &map_info->text_map.colors[y][x];
        colordif = (((special & MG_PET) != 0 && iflags.hilite_pet)
                    || ((special & MG_OBJPILE) != 0 && iflags.hilite_pile)
                    || ((special & (MG_DETECT | MG_BW_LAVA | MG_BW_ICE)) != 0
                        && iflags.use_inverse))
                      ? CLR_MAX : 0;
        if (*co_ptr != (uchar) (color + colordif)) {
            *co_ptr = (uchar) (color + colordif);
            if (!map_info->is_tile)
                update_bbox = TRUE;
        }
#endif
    }

    if (update_bbox) { /* update row bbox */
        if (x < map_info->t_start[y])
            map_info->t_start[y] = x;
        if (x > map_info->t_stop[y])
            map_info->t_stop[y] = x;
    }
}

#ifdef CLIPPING
/*
 * The is the tty clip call.  Since X can resize at any time, we can't depend
 * on this being defined.
 */
/*ARGSUSED*/
void
X11_cliparound(int x UNUSED, int y UNUSED)
{
    return;
}
#endif /* CLIPPING */

/* End global functions =================================================== */

#include "tile2x11.h"

/*
 * We're expecting to never read more than one tile file per session.
 * If this is false, then we can make an array of this information,
 * or just keep it on a per-window basis.
 */
Pixmap tile_pixmap = None;
static int tile_width;
static int tile_height;
static int tile_count;
static XImage *tile_image = 0;

/*
 * This structure is used for small bitmaps that are used for annotating
 * tiles.  For example, a "heart" annotates pets.
 */
struct tile_annotation {
    Pixmap bitmap;
    Pixel foreground;
    unsigned int width, height;
    int hotx, hoty; /* not currently used */
};

static struct tile_annotation pet_annotation;
static struct tile_annotation pile_annotation;

static void
init_annotation(
    struct tile_annotation *annotation,
    char *filename,
    Pixel colorpixel)
{
    Display *dpy = XtDisplay(toplevel);

    if (0 != XReadBitmapFile(dpy, XtWindow(toplevel), filename,
                             &annotation->width, &annotation->height,
                             &annotation->bitmap, &annotation->hotx,
                             &annotation->hoty)) {
        char buf[BUFSZ];

        Sprintf(buf, "Failed to load %s", filename);
        X11_raw_print(buf);
    }

    annotation->foreground = colorpixel;
}

/*
 * Put the tile image on the server.
 *
 * We can't send the image to the server until the top level
 * is realized.  When the tile file is first processed, the top
 * level is not realized.  This routine is called after we
 * realize the top level, but before we start resizing the
 * map viewport.
 */
void
post_process_tiles(void)
{
    Display *dpy = XtDisplay(toplevel);
    unsigned int width, height;

    if (tile_image == 0)
        return; /* no tiles */

    height = tile_image->height;
    width = tile_image->width;

    tile_pixmap = XCreatePixmap(dpy, XtWindow(toplevel), width, height,
                                DefaultDepth(dpy, DefaultScreen(dpy)));

    XPutImage(dpy, tile_pixmap, DefaultGC(dpy, DefaultScreen(dpy)),
              tile_image, 0, 0, 0, 0, /* src, dest top left */
              width, height);

#ifdef MONITOR_HEAP
    /* if we let XDestroyImage() handle it, our tracking will be off */
    if (tile_image->data)
        free((genericptr_t) tile_image->data), tile_image->data = 0;
#endif
    XDestroyImage(tile_image); /* data bytes free'd also */
    tile_image = 0;

    init_annotation(&pet_annotation, appResources.pet_mark_bitmap,
                    appResources.pet_mark_color);
    init_annotation(&pile_annotation, appResources.pilemark_bitmap,
                    appResources.pilemark_color);
}

/*
 * Open and read the tile file.  Return TRUE if there were no problems.
 * Return FALSE otherwise.
 */
static boolean
init_tiles(struct xwindow *wp)
{
#ifdef USE_XPM
    XpmAttributes attributes;
    int errorcode;
#else
    FILE *fp = (FILE *) 0;
    x11_header header;
    unsigned char *cp, *colormap = (unsigned char *) 0;
    unsigned char *tb, *tile_bytes = (unsigned char *) 0;
    int size;
    XColor *colors = (XColor *) 0;
    unsigned i;
    int x, y;
    int bitmap_pad;
    int ddepth;
#endif
    char buf[BUFSZ];
    Display *dpy = XtDisplay(toplevel);
    Screen *screen = DefaultScreenOfDisplay(dpy);
    struct map_info_t *map_info = (struct map_info_t *) 0;
    struct tile_map_info_t *tile_info = (struct tile_map_info_t *) 0;
    unsigned int image_height = 0, image_width = 0;
    boolean result = TRUE;
    XGCValues values;
    XtGCMask mask;

    /* already have tile information */
    if (tile_pixmap != None)
        goto tiledone;

    map_info = wp->map_information;
    tile_info = &map_info->tile_map;
    (void) memset((genericptr_t) tile_info, 0,
                  sizeof (struct tile_map_info_t));

    /* no tile file name, no tile information */
    if (!appResources.tile_file[0]) {
        result = FALSE;
        goto tiledone;
    }

#ifdef USE_XPM
    attributes.valuemask = XpmCloseness;
    attributes.closeness = 25000;

    errorcode = XpmReadFileToImage(dpy, appResources.tile_file, &tile_image,
                                   0, &attributes);

    if (errorcode == XpmColorFailed) {
        Sprintf(buf, "Insufficient colors available to load %s.",
                appResources.tile_file);
        X11_raw_print(buf);
        X11_raw_print("Try closing other colorful applications and restart.");
        X11_raw_print("Attempting to load with inferior colors.");
        attributes.closeness = 50000;
        errorcode = XpmReadFileToImage(dpy, appResources.tile_file,
                                       &tile_image, 0, &attributes);
    }

    if (errorcode != XpmSuccess) {
        if (errorcode == XpmColorFailed) {
            Sprintf(buf, "Insufficient colors available to load %s.",
                    appResources.tile_file);
            X11_raw_print(buf);
        } else {
            Sprintf(buf, "Failed to load %s: %s", appResources.tile_file,
                    XpmGetErrorString(errorcode));
            X11_raw_print(buf);
        }
        result = FALSE;
        X11_raw_print("Switching to text-based mode.");
        goto tiledone;
    }

    /* assume a fixed number of tiles per row */
    if (tile_image->width % TILES_PER_ROW != 0
        || tile_image->width <= TILES_PER_ROW) {
        Sprintf(buf,
               "%s is not a multiple of %d (number of tiles/row) pixels wide",
                appResources.tile_file, TILES_PER_ROW);
        X11_raw_print(buf);
        XDestroyImage(tile_image);
        tile_image = 0;
        result = FALSE;
        goto tiledone;
    }

    /* infer tile dimensions from image size and TILES_PER_ROW */
    image_width = tile_image->width;
    image_height = tile_image->height;

    tile_count = total_tiles_used;
    if ((tile_count % TILES_PER_ROW) != 0) {
        tile_count += TILES_PER_ROW - (tile_count % TILES_PER_ROW);
    }
    tile_width = image_width / TILES_PER_ROW;
    tile_height = image_height / (tile_count / TILES_PER_ROW);
#else /* !USE_XPM */
    /* any less than 16 colours makes tiles useless */
    ddepth = DefaultDepthOfScreen(screen);
    if (ddepth < 4) {
        X11_raw_print("need a screen depth of at least 4");
        result = FALSE;
        goto tiledone;
    }

    fp = fopen_datafile(appResources.tile_file, RDBMODE, FALSE);
    if (!fp) {
        X11_raw_print("can't open tile file");
        result = FALSE;
        goto tiledone;
    }

    if ((int) fread((char *) &header, sizeof(header), 1, fp) != 1) {
        X11_raw_print("read of header failed");
        result = FALSE;
        goto tiledone;
    }

    if (header.version != 2) {
        Sprintf(buf, "Wrong tile file version, expected 2, got %lu",
                header.version);
        X11_raw_print(buf);
        result = FALSE;
        goto tiledone;
    }
#ifdef VERBOSE
    fprintf(stderr, "\
X11 tile file:\n    version %ld\n    ncolors %ld\n    \
tile width %ld\n    tile height %ld\n    per row %ld\n    \
ntiles %ld\n",
            header.version, header.ncolors, header.tile_width,
            header.tile_height, header.per_row, header.ntiles);
#endif

    size = 3 * header.ncolors;
    colormap = (unsigned char *) alloc((unsigned) size);
    if ((int) fread((char *) colormap, 1, size, fp) != size) {
        X11_raw_print("read of colormap failed");
        result = FALSE;
        goto tiledone;
    }

    colors = (XColor *) alloc(sizeof (XColor) * (unsigned) header.ncolors);
    for (i = 0; i < header.ncolors; i++) {
        cp = colormap + (3 * i);
        colors[i].red = cp[0] * 256;
        colors[i].green = cp[1] * 256;
        colors[i].blue = cp[2] * 256;
        colors[i].flags = 0;
        colors[i].pixel = 0;

        if (!XAllocColor(dpy, DefaultColormapOfScreen(screen), &colors[i])
            && !nhApproxColor(screen, DefaultColormapOfScreen(screen),
                              (char *) 0, &colors[i])) {
            Sprintf(buf, "%dth out of %ld color allocation failed", i,
                    header.ncolors);
            X11_raw_print(buf);
            result = FALSE;
            goto tiledone;
        }
    }

    size = header.tile_height * header.tile_width;
    /*
     * This alloc() and the one below require 32-bit ints, since tile_bytes
     * is currently ~200k and alloc() takes an int
     */
    tile_count = header.ntiles;
    if ((tile_count % header.per_row) != 0) {
        tile_count += header.per_row - (tile_count % header.per_row);
    }
    tile_bytes = (unsigned char *) alloc((unsigned) tile_count * size);
    if ((int) fread((char *) tile_bytes, size, tile_count, fp)
        != tile_count) {
        X11_raw_print("read of tile bytes failed");
        result = FALSE;
        goto tiledone;
    }

    if (header.ntiles < (unsigned) total_tiles_used) {
        Sprintf(buf, "tile file incomplete, expecting %d tiles, found %lu",
                total_tiles_used, header.ntiles);
        X11_raw_print(buf);
        result = FALSE;
        goto tiledone;
    }

    if (appResources.double_tile_size) {
        tile_width = 2 * header.tile_width;
        tile_height = 2 * header.tile_height;
    } else {
        tile_width = header.tile_width;
        tile_height = header.tile_height;
    }

    image_height = tile_height * tile_count / header.per_row;
    image_width = tile_width * header.per_row;

    /* calculate bitmap_pad */
    if (ddepth > 16)
        bitmap_pad = 32;
    else if (ddepth > 8)
        bitmap_pad = 16;
    else
        bitmap_pad = 8;

    tile_image = XCreateImage(dpy, DefaultVisualOfScreen(screen),
                              ddepth,           /* depth */
                              ZPixmap,          /* format */
                              0,                /* offset */
                              (char *) 0,       /* data */
                              image_width,      /* width */
                              image_height,     /* height */
                              bitmap_pad,       /* bit pad */
                              0);               /* bytes_per_line */

    if (!tile_image) {
        impossible("init_tiles: insufficient memory to create image");
        X11_raw_print("Resorting to text map.");
        result = FALSE;
        goto tiledone;
    }

    /* now we know the physical memory requirements, we can allocate space */
    tile_image->data =
        (char *) alloc((unsigned) tile_image->bytes_per_line * image_height);

    if (appResources.double_tile_size) {
        unsigned long *expanded_row =
            (unsigned long *) alloc(sizeof (unsigned long) * image_width);

        tb = tile_bytes;
        for (y = 0; y < (int) image_height; y++) {
            for (x = 0; x < (int) image_width / 2; x++)
                expanded_row[2 * x] = expanded_row[(2 * x) + 1] =
                    colors[*tb++].pixel;

            for (x = 0; x < (int) image_width; x++)
                XPutPixel(tile_image, x, y, expanded_row[x]);

            y++; /* duplicate row */
            for (x = 0; x < (int) image_width; x++)
                XPutPixel(tile_image, x, y, expanded_row[x]);
        }
        free((genericptr_t) expanded_row);

    } else {
        for (tb = tile_bytes, y = 0; y < (int) image_height; y++)
            for (x = 0; x < (int) image_width; x++, tb++)
                XPutPixel(tile_image, x, y, colors[*tb].pixel);
    }
#endif /* ?USE_XPM */

    /* fake an inverted tile by drawing a border around the edges */
#ifdef USE_WHITE
    /* use white or black as the border */
    mask = GCFunction | GCForeground | GCGraphicsExposures;
    values.graphics_exposures = False;
    values.foreground = WhitePixelOfScreen(screen);
    values.function = GXcopy;
    tile_info->white_gc = XtGetGC(wp->w, mask, &values);
    values.graphics_exposures = False;
    values.foreground = BlackPixelOfScreen(screen);
    values.function = GXcopy;
    tile_info->black_gc = XtGetGC(wp->w, mask, &values);
#else
    /*
     * Use xor so we don't have to check for special colors.  Xor white
     * against the upper left pixel of the corridor so that we have a
     * white rectangle when in a corridor.
     */
    mask = GCFunction | GCForeground | GCGraphicsExposures;
    values.graphics_exposures = False;
    values.foreground =
        WhitePixelOfScreen(screen)
        ^ XGetPixel(tile_image, 0,
#if 0
                    tile_height * glyph2tile[cmap_to_glyph(S_corr)]);
#else
                    tile_height * T_corr);
#endif
    values.function = GXxor;
    tile_info->white_gc = XtGetGC(wp->w, mask, &values);

    mask = GCFunction | GCGraphicsExposures;
    values.function = GXCopy;
    values.graphics_exposures = False;
    tile_info->black_gc = XtGetGC(wp->w, mask, &values);
#endif /* USE_WHITE */

 tiledone:
#ifndef USE_XPM
    if (fp)
        (void) fclose(fp);
    if (colormap)
        free((genericptr_t) colormap);
    if (tile_bytes)
        free((genericptr_t) tile_bytes);
    if (colors)
        free((genericptr_t) colors);
#endif

    if (result) { /* succeeded */
        tile_info->square_height = tile_height;
        tile_info->square_width = tile_width;
        tile_info->square_ascent = 0;
        tile_info->square_lbearing = 0;
        tile_info->image_width = image_width;
        tile_info->image_height = image_height;
    }

    return result;
}

/*
 * Make sure the map's cursor is always visible.
 */
void
check_cursor_visibility(struct xwindow *wp)
{
    Arg arg[2];
    Widget viewport, horiz_sb, vert_sb;
    float top, shown, cursor_middle;
    Boolean do_call, adjusted = False;
#ifdef VERBOSE
    char *s;
#endif

    viewport = XtParent(wp->w);
    horiz_sb = XtNameToWidget(viewport, "horizontal");
    vert_sb = XtNameToWidget(viewport, "vertical");

/* All values are relative to currently visible area */

#define V_BORDER 0.25 /* if this far from vert edge, shift */
#define H_BORDER 0.25 /* if this from from horiz edge, shift */

#define H_DELTA 0.25 /* distance of horiz shift */
#define V_DELTA 0.25 /* distance of vert shift */

    if (horiz_sb) {
        XtSetArg(arg[0], XtNshown, &shown);
        XtSetArg(arg[1], nhStr(XtNtopOfThumb), &top);
        XtGetValues(horiz_sb, arg, TWO);

        /* [ALI] Don't assume map widget is the same size as actual map */
        if (wp->map_information->is_tile)
            cursor_middle = wp->map_information->tile_map.square_width;
        else
            cursor_middle = wp->map_information->text_map.square_width;
        cursor_middle = (wp->cursx + 0.5) * cursor_middle / wp->pixel_width;
        do_call = True;

#ifdef VERBOSE
        if (cursor_middle < top) {
            s = " outside left";
        } else if (cursor_middle < top + shown * H_BORDER) {
            s = " close to left";
        } else if (cursor_middle > (top + shown)) {
            s = " outside right";
        } else if (cursor_middle > (top + shown - shown * H_BORDER)) {
            s = " close to right";
        } else {
            s = "";
        }
        printf("Horiz: shown = %3.2f, top = %3.2f%s", shown, top, s);
#endif

        if (cursor_middle < top) {
            top = cursor_middle - shown * H_DELTA;
            if (top < 0.0)
                top = 0.0;
        } else if (cursor_middle < top + shown * H_BORDER) {
            top -= shown * H_DELTA;
            if (top < 0.0)
                top = 0.0;
        } else if (cursor_middle > (top + shown)) {
            top = cursor_middle - shown * H_DELTA;
            if (top < 0.0)
                top = 0.0;
            if (top + shown > 1.0)
                top = 1.0 - shown;
        } else if (cursor_middle > (top + shown - shown * H_BORDER)) {
            top += shown * H_DELTA;
            if (top + shown > 1.0)
                top = 1.0 - shown;
        } else {
            do_call = False;
        }

        if (do_call) {
            XtCallCallbacks(horiz_sb, XtNjumpProc, &top);
            adjusted = True;
        }
    }

    if (vert_sb) {
        XtSetArg(arg[0], XtNshown, &shown);
        XtSetArg(arg[1], nhStr(XtNtopOfThumb), &top);
        XtGetValues(vert_sb, arg, TWO);

        if (wp->map_information->is_tile)
            cursor_middle = wp->map_information->tile_map.square_height;
        else
            cursor_middle = wp->map_information->text_map.square_height;
        cursor_middle = (wp->cursy + 0.5) * cursor_middle / wp->pixel_height;
        do_call = True;

#ifdef VERBOSE
        if (cursor_middle < top) {
            s = " above top";
        } else if (cursor_middle < top + shown * V_BORDER) {
            s = " close to top";
        } else if (cursor_middle > (top + shown)) {
            s = " below bottom";
        } else if (cursor_middle > (top + shown - shown * V_BORDER)) {
            s = " close to bottom";
        } else {
            s = "";
        }
        printf("%sVert: shown = %3.2f, top = %3.2f%s", horiz_sb ? ";  " : "",
               shown, top, s);
#endif

        if (cursor_middle < top) {
            top = cursor_middle - shown * V_DELTA;
            if (top < 0.0)
                top = 0.0;
        } else if (cursor_middle < top + shown * V_BORDER) {
            top -= shown * V_DELTA;
            if (top < 0.0)
                top = 0.0;
        } else if (cursor_middle > (top + shown)) {
            top = cursor_middle - shown * V_DELTA;
            if (top < 0.0)
                top = 0.0;
            if (top + shown > 1.0)
                top = 1.0 - shown;
        } else if (cursor_middle > (top + shown - shown * V_BORDER)) {
            top += shown * V_DELTA;
            if (top + shown > 1.0)
                top = 1.0 - shown;
        } else {
            do_call = False;
        }

        if (do_call) {
            XtCallCallbacks(vert_sb, XtNjumpProc, &top);
            adjusted = True;
        }
    }

    /* make sure cursor is displayed during dowhatis.. */
    if (adjusted)
        display_cursor(wp);

#ifdef VERBOSE
    if (horiz_sb || vert_sb)
        printf("\n");
#endif
}

/*
 * Check to see if the viewport has grown smaller.  If so, then we want to
 * make
 * sure that the cursor is still on the screen.  We do this to keep the cursor
 * on the screen when the user resizes the nethack window.
 */
static void
map_check_size_change(struct xwindow *wp)
{
    struct map_info_t *map_info = wp->map_information;
    Arg arg[2];
    Dimension new_width, new_height;
    Widget viewport;

    viewport = XtParent(wp->w);

    XtSetArg(arg[0], XtNwidth, &new_width);
    XtSetArg(arg[1], XtNheight, &new_height);
    XtGetValues(viewport, arg, TWO);

    /* Only do cursor check if new size is smaller. */
    if (new_width < map_info->viewport_width
        || new_height < map_info->viewport_height) {
        /* [ALI] If the viewport was larger than the map (and so the map
         * widget was contrained to be larger than the actual map) then we
         * may be able to shrink the map widget as the viewport shrinks.
         */
        if (map_info->is_tile) {
            wp->pixel_width = map_info->tile_map.square_width * COLNO;
            wp->pixel_height = map_info->tile_map.square_height * ROWNO;
        } else {
            wp->pixel_width = map_info->text_map.square_width * COLNO;
            wp->pixel_height = map_info->text_map.square_height * ROWNO;
        }

        if (wp->pixel_width < new_width)
            wp->pixel_width = new_width;
        if (wp->pixel_height < new_height)
            wp->pixel_height = new_height;
        XtSetArg(arg[0], XtNwidth, wp->pixel_width);
        XtSetArg(arg[1], XtNheight, wp->pixel_height);
        XtSetValues(wp->w, arg, TWO);

        check_cursor_visibility(wp);
    }

    map_info->viewport_width = new_width;
    map_info->viewport_height = new_height;

    /* [ALI] These may have changed if the user has re-sized the viewport */
    XtSetArg(arg[0], XtNwidth, &wp->pixel_width);
    XtSetArg(arg[1], XtNheight, &wp->pixel_height);
    XtGetValues(wp->w, arg, TWO);
}

/*
 * Fill in parameters "regular" and "inverse" with newly created GCs.
 * Using the given background pixel and the foreground pixel optained
 * by querying the widget with the resource name.
 */
static void
set_gc(Widget w, Font font, const char *resource_name, Pixel bgpixel,
       GC *regular, GC *inverse)
{
    XGCValues values;
    XtGCMask mask = GCFunction | GCForeground | GCBackground | GCFont;
    Pixel curpixel;
    Arg arg[1];

    XtSetArg(arg[0], (char *) resource_name, &curpixel);
    XtGetValues(w, arg, ONE);

    values.foreground = curpixel;
    values.background = bgpixel;
    values.function = GXcopy;
    values.font = font;
    *regular = XtGetGC(w, mask, &values);
    values.foreground = bgpixel;
    values.background = curpixel;
    values.function = GXcopy;
    values.font = font;
    *inverse = XtGetGC(w, mask, &values);
}

/*
 * Create the GC's for each color.
 *
 * I'm not sure if it is a good idea to have a GC for each color (and
 * inverse). It might be faster to just modify the foreground and
 * background colors on the current GC as needed.
 */
static void
get_text_gc(struct xwindow *wp, Font font)
{
    struct map_info_t *map_info = wp->map_information;
    Pixel bgpixel;
    Arg arg[1];

    /* Get background pixel. */
    XtSetArg(arg[0], XtNbackground, &bgpixel);
    XtGetValues(wp->w, arg, ONE);

#ifdef TEXTCOLOR
#define set_color_gc(nh_color, resource_name)       \
    set_gc(wp->w, font, resource_name, bgpixel,     \
           &map_info->text_map.color_gcs[nh_color], \
           &map_info->text_map.inv_color_gcs[nh_color]);

    set_color_gc(CLR_BLACK, XtNblack);
    set_color_gc(CLR_RED, XtNred);
    set_color_gc(CLR_GREEN, XtNgreen);
    set_color_gc(CLR_BROWN, XtNbrown);
    set_color_gc(CLR_BLUE, XtNblue);
    set_color_gc(CLR_MAGENTA, XtNmagenta);
    set_color_gc(CLR_CYAN, XtNcyan);
    set_color_gc(CLR_GRAY, XtNgray);
    set_color_gc(NO_COLOR, XtNforeground);
    set_color_gc(CLR_ORANGE, XtNorange);
    set_color_gc(CLR_BRIGHT_GREEN, XtNbright_green);
    set_color_gc(CLR_YELLOW, XtNyellow);
    set_color_gc(CLR_BRIGHT_BLUE, XtNbright_blue);
    set_color_gc(CLR_BRIGHT_MAGENTA, XtNbright_magenta);
    set_color_gc(CLR_BRIGHT_CYAN, XtNbright_cyan);
    set_color_gc(CLR_WHITE, XtNwhite);
#else
    set_gc(wp->w, font, XtNforeground, bgpixel,
           &map_info->text_map.copy_gc,
           &map_info->text_map.inv_copy_gc);
#endif
}

/*
 * Display the cursor on the map window.
 */
static void
display_cursor(struct xwindow *wp)
{
    /* Redisplay the cursor location inverted. */
    map_update(wp, wp->cursy, wp->cursy, wp->cursx, wp->cursx, TRUE);
}

/*
 * Check if there are any changed characters.  If so, then plaster them on
 * the screen.
 */
void
display_map_window(struct xwindow *wp)
{
    register int row;
    struct map_info_t *map_info = wp->map_information;

    if ((Is_rogue_level(&u.uz) ? map_info->is_tile
                               : (map_info->is_tile != iflags.wc_tiled_map))
        && map_info->tile_map.image_width) {
        int i;

        /* changed map display mode, re-display the full map */
        for (i = 0; i < ROWNO; i++) {
            map_info->t_start[i] = 0;
            map_info->t_stop[i] = COLNO-1;
        }
        map_info->is_tile = iflags.wc_tiled_map && !Is_rogue_level(&u.uz);
        XClearWindow(XtDisplay(wp->w), XtWindow(wp->w));
        set_map_size(wp, COLNO, ROWNO);
        check_cursor_visibility(wp);
        highlight_yn(TRUE); /* change fg/bg to match map */
    } else if (wp->prevx != wp->cursx || wp->prevy != wp->cursy) {
        register coordxy x = wp->prevx, y = wp->prevy;

        /*
         * Previous cursor position is not the same as the current
         * cursor position, update the old cursor position.
         */
        if (x < map_info->t_start[y])
            map_info->t_start[y] = x;
        if (x > map_info->t_stop[y])
            map_info->t_stop[y] = x;
    }

    for (row = 0; row < ROWNO; row++) {
        if (map_info->t_start[row] <= map_info->t_stop[row]) {
            map_update(wp, row, row, (int) map_info->t_start[row],
                       (int) map_info->t_stop[row], FALSE);
            map_info->t_start[row] = COLNO - 1;
            map_info->t_stop[row] = 0;
        }
    }
    display_cursor(wp);
    wp->prevx = wp->cursx; /* adjust old cursor position */
    wp->prevy = wp->cursy;
}

/*
 * Set all map tiles and characters to S_unexplored (was S_stone).
 * (Actually, column 0 is set to S_nothing and 1..COLNO-1 to S_unexplored.)
 */
static void
map_all_unexplored(struct map_info_t *map_info) /* [was map_all_stone()] */
{
    int x, y;
    glyph_info gi;
    short unexp_idx, nothg_idx;
 /* unsigned short g_stone = cmap_to_glyph(S_stone); */
    unsigned short g_unexp = GLYPH_UNEXPLORED, g_nothg = GLYPH_NOTHING;
    int mgunexp = ' ', mgnothg = ' ';
    struct tile_map_info_t *tile_map = &map_info->tile_map;
    struct text_map_info_t *text_map = &map_info->text_map;

    mgunexp = glyph2ttychar(GLYPH_UNEXPLORED);
    mgnothg = glyph2ttychar(GLYPH_NOTHING);

    map_glyphinfo(0, 0, g_unexp, 0, &gi);
    unexp_idx = gi.gm.tileidx;
    map_glyphinfo(0, 0, g_nothg, 0, &gi);
    nothg_idx = gi.gm.tileidx;

    /*
     * Tiles map tracks glyphs.
     * Text map tracks characters derived from glyphs.
     */
    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            tile_map->glyphs[y][x].glyph = !x ? g_nothg : g_unexp;
            tile_map->glyphs[y][x].glyphflags = 0;
            tile_map->glyphs[y][x].tileidx = !x ? nothg_idx : unexp_idx;

            text_map->text[y][x] = (uchar) (!x ? mgnothg : mgunexp);
#ifdef TEXTCOLOR
            text_map->colors[y][x] = NO_COLOR;
#endif
        }
}

/*
 * Fill the saved screen characters with the "clear" tile or character.
 *
 * Flush out everything by resetting the "new" bounds and calling
 * display_map_window().
 */
void
clear_map_window(struct xwindow *wp)
{
    struct map_info_t *map_info = wp->map_information;
    int i;

    /* update both tile and text backing store, then update */
    map_all_unexplored(map_info);

    /* force a full update */
    for (i = 0; i < ROWNO; i++) {
        map_info->t_start[i] = 0;
        map_info->t_stop[i] = COLNO-1;
    }

    display_map_window(wp);
}

/*
 * Retreive the font associated with the map window and save attributes
 * that are used when updating it.
 */
static void
get_char_info(struct xwindow *wp)
{
    XFontStruct *fs;
    struct map_info_t *map_info = wp->map_information;
    struct text_map_info_t *text_map = &map_info->text_map;

    fs = WindowFontStruct(wp->w);
    text_map->square_width = fs->max_bounds.width;
    text_map->square_height = fs->max_bounds.ascent + fs->max_bounds.descent;
    text_map->square_ascent = fs->max_bounds.ascent;
    text_map->square_lbearing = -fs->min_bounds.lbearing;

#ifdef VERBOSE
    printf("Font information:\n");
    printf("fid = %ld, direction = %d\n", fs->fid, fs->direction);
    printf("first = %d, last = %d\n",
           fs->min_char_or_byte2, fs->max_char_or_byte2);
    printf("all chars exist? %s\n", fs->all_chars_exist ? "yes" : "no");
    printf("min_bounds:lb=%d rb=%d width=%d asc=%d des=%d attr=%d\n",
           fs->min_bounds.lbearing, fs->min_bounds.rbearing,
           fs->min_bounds.width, fs->min_bounds.ascent,
           fs->min_bounds.descent, fs->min_bounds.attributes);
    printf("max_bounds:lb=%d rb=%d width=%d asc=%d des=%d attr=%d\n",
           fs->max_bounds.lbearing, fs->max_bounds.rbearing,
           fs->max_bounds.width, fs->max_bounds.ascent,
           fs->max_bounds.descent, fs->max_bounds.attributes);
    printf("per_char = 0x%lx\n", (unsigned long) fs->per_char);
    printf("Text: (max) width = %d, height = %d\n",
           text_map->square_width, text_map->square_height);
#endif

    if (fs->min_bounds.width != fs->max_bounds.width)
        X11_raw_print("Warning:  map font is not monospaced!");
}

/*
 * keyhit buffer
 */
#define INBUF_SIZE 64
static int inbuf[INBUF_SIZE];
static int incount = 0;
static int inptr = 0; /* points to valid data */

/*
 * Keyboard and button event handler for map window.
 */
void
map_input(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    XKeyEvent *key;
    XButtonEvent *button;
    boolean meta = FALSE;
    int i, nbytes;
    Cardinal in_nparams = (num_params ? *num_params : 0);
    char c;
    char keystring[MAX_KEY_STRING];

    switch (event->type) {
    case ButtonPress:
        if (!iflags.wc_mouse_support)
            return;

        button = (XButtonEvent *) event;
#ifdef VERBOSE_INPUT
        printf("button press\n");
#endif
        if (in_nparams > 0 && (nbytes = strlen(params[0])) < MAX_KEY_STRING) {
            Strcpy(keystring, params[0]);
            key = (XKeyEvent *) event; /* just in case */
            goto key_events;
        }
        if (w != window_list[WIN_MAP].w) {
#ifdef VERBOSE_INPUT
            printf("map_input called from wrong window\n");
#endif
            X11_nhbell();
            return;
        }
        set_button_values(w, button->x, button->y, button->button);
        break;
    case KeyPress:
#ifdef VERBOSE_INPUT
        printf("key: ");
#endif
        if (appResources.slow && input_func) {
            (*input_func)(w, event, params, num_params);
            break;
        }

        /*
         * Don't use key_event_to_char() because we want to be able
         * to allow keys mapped to multiple characters.
         */
        key = (XKeyEvent *) event;
        if (in_nparams > 0 && (nbytes = strlen(params[0])) < MAX_KEY_STRING) {
            Strcpy(keystring, params[0]);
        } else {
            /*
             * Assume that mod1 is really the meta key.
             */
            meta = !!(key->state & Mod1Mask);
            nbytes = XLookupString(key, keystring, MAX_KEY_STRING,
                                   (KeySym *) 0, (XComposeStatus *) 0);
        }
 key_events:
        /* Modifier keys return a zero length string when pressed. */
        if (nbytes) {
#ifdef VERBOSE_INPUT
            printf("\"");
#endif
            for (i = 0; i < nbytes; i++) {
                c = keystring[i];

                if (incount < INBUF_SIZE) {
                    inbuf[(inptr + incount) % INBUF_SIZE] =
                        ((int) c) + (meta ? 0x80 : 0);
                    incount++;
                } else {
                    X11_nhbell();
                }
#ifdef VERBOSE_INPUT
                if (meta) /* meta will print as M<c> */
                    (void) putchar('M');
                if (c < ' ') { /* ctrl will print as ^<c> */
                    (void) putchar('^');
                    c += '@';
                }
                (void) putchar(c);
#endif
            }
#ifdef VERBOSE_INPUT
            printf("\" [%d bytes]\n", nbytes);
#endif
        }
        break;

    default:
        impossible("unexpected X event, type = %d\n", (int) event->type);
        break;
    }
}

static void
set_button_values(Widget w, int x, int y, unsigned int button)
{
    struct xwindow *wp;
    struct map_info_t *map_info;

    wp = find_widget(w);
    map_info = wp->map_information;

    if (map_info->is_tile) {
        click_x = x / map_info->tile_map.square_width;
        click_y = y / map_info->tile_map.square_height;
    } else {
        click_x = x / map_info->text_map.square_width;
        click_y = y / map_info->text_map.square_height;
    }
    click_x += COL0_OFFSET; /* note: reverse of usual adjustment */

    /* The values can be out of range if the map window has been resized
       to be larger than the max size. */
    if (click_x >= COLNO)
        click_x = COLNO - 1;
    if (click_y >= ROWNO)
        click_y = ROWNO - 1;

    /* Map all buttons but the first to the second click */
    click_button = (button == Button1) ? CLICK_1 : CLICK_2;
}

/*
 * Map window expose callback.
 */
/*ARGSUSED*/
static void
map_exposed(Widget w, XtPointer client_data, /* unused */
            XtPointer widget_data) /* expose event from Window widget */
{
    int x, y;
    struct xwindow *wp;
    struct map_info_t *map_info;
    unsigned width, height;
    int start_row, stop_row, start_col, stop_col;
    XExposeEvent *event = (XExposeEvent *) widget_data;
    int t_height, t_width; /* tile/text height & width */

    nhUse(client_data);

    if (!XtIsRealized(w) || event->count > 0)
        return;

    wp = find_widget(w);
    map_info = wp->map_information;
    if (wp->keep_window && !map_info)
        return;
    /*
     * The map is sent an expose event when the viewport resizes.  Make sure
     * that the cursor is still in the viewport after the resize.
     */
    map_check_size_change(wp);

    if (event) { /* called from button-event */
        x = event->x;
        y = event->y;
        width = event->width;
        height = event->height;
    } else {
        x = 0;
        y = 0;
        width = wp->pixel_width;
        height = wp->pixel_height;
    }
    /*
     * Convert pixels into INCLUSIVE text rows and columns.
     */
    if (map_info->is_tile) {
        t_height = map_info->tile_map.square_height;
        t_width = map_info->tile_map.square_width;
    } else {
        t_height = map_info->text_map.square_height;
        t_width = map_info->text_map.square_width;
    }
    start_row = y / t_height;
    stop_row = ((y + height) / t_height) + 1;

    start_col = x / t_width;
    stop_col = ((x + width) / t_width) + 1;

#ifdef VERBOSE
    printf("map_exposed: x = %d, y = %d, width = %d, height = %d\n", x, y,
           width, height);
    printf("chars %d x %d, rows %d to %d, columns %d to %d\n", t_height,
           t_width, start_row, stop_row, start_col, stop_col);
#endif

    /* Out of range values are possible if the map window is resized to be
       bigger than the largest expected value. */
    if (stop_row >= ROWNO)
        stop_row = ROWNO - 1;
    if (stop_col >= COLNO)
        stop_col = COLNO - 1;

    map_update(wp, start_row, stop_row, start_col, stop_col, FALSE);
    display_cursor(wp); /* make sure cursor shows up */
}

/*
 * Do the actual work of the putting characters onto our X window.  This
 * is called from the expose event routine, the display window (flush)
 * routine, and the display cursor routine.  The last involves inverting
 * the foreground and background colors, which are also inverted when the
 * position's color is above CLR_MAX.
 *
 * This works for rectangular regions (this includes one line rectangles).
 * The start and stop columns are *inclusive*.
 */
static void
map_update(struct xwindow *wp, int start_row, int stop_row, int start_col, int stop_col, boolean inverted)
{
    struct map_info_t *map_info = wp->map_information;
    int row;
    register int count;

    if (start_row < 0 || stop_row >= ROWNO) {
        impossible("map_update:  bad row range %d-%d\n", start_row, stop_row);
        return;
    }
    if (start_col < 0 || stop_col >= COLNO) {
        impossible("map_update:  bad col range %d-%d\n", start_col, stop_col);
        return;
    }

#ifdef VERBOSE_UPDATE
    printf("update: [0x%x] %d %d %d %d\n",
           (int) wp->w, start_row, stop_row, start_col, stop_col);
#endif

    if (map_info->is_tile) {
        struct tile_map_info_t *tile_map = &map_info->tile_map;
        int cur_col;
        Display *dpy = XtDisplay(wp->w);
        Screen *screen = DefaultScreenOfDisplay(dpy);

        for (row = start_row; row <= stop_row; row++) {
            for (cur_col = start_col; cur_col <= stop_col; cur_col++) {
#if 0
                int glyph = tile_map->glyphs[row][cur_col].glyph;
                int tile = glyph2tile[glyph];
#else
                int tile = tile_map->glyphs[row][cur_col].tileidx;
#endif
                int src_x, src_y;
                int dest_x = (cur_col - COL0_OFFSET) * tile_map->square_width;
                int dest_y = row * tile_map->square_height;

#if 0
                /* not required with the new glyph representations */
                if ((tile_map->glyphs[row][cur_col].glyphflags & MG_FEMALE))
                    tile++; /* advance to the female tile variation */
#endif
                src_x = (tile % TILES_PER_ROW) * tile_width;
                src_y = (tile / TILES_PER_ROW) * tile_height;
                XCopyArea(dpy, tile_pixmap, XtWindow(wp->w),
                          tile_map->black_gc, /* no grapics_expose */
                          src_x, src_y, tile_width, tile_height,
                          dest_x, dest_y);

                if ((tile_map->glyphs[row][cur_col].glyphflags & MG_PET) && iflags.hilite_pet) {
                    /* draw pet annotation (a heart) */
                    XSetForeground(dpy, tile_map->black_gc,
                                   pet_annotation.foreground);
                    XSetClipOrigin(dpy, tile_map->black_gc, dest_x, dest_y);
                    XSetClipMask(dpy, tile_map->black_gc,
                                 pet_annotation.bitmap);
                    XCopyPlane(dpy, pet_annotation.bitmap, XtWindow(wp->w),
                               tile_map->black_gc, 0, 0, pet_annotation.width,
                               pet_annotation.height, dest_x, dest_y, 1);
                    XSetClipOrigin(dpy, tile_map->black_gc, 0, 0);
                    XSetClipMask(dpy, tile_map->black_gc, None);
                    XSetForeground(dpy, tile_map->black_gc,
                                   BlackPixelOfScreen(screen));
                }
                if ((tile_map->glyphs[row][cur_col].glyphflags & MG_OBJPILE)) {
                    /* draw object pile annotation (a plus sign) */
                    XSetForeground(dpy, tile_map->black_gc,
                                   pile_annotation.foreground);
                    XSetClipOrigin(dpy, tile_map->black_gc, dest_x, dest_y);
                    XSetClipMask(dpy, tile_map->black_gc,
                                 pile_annotation.bitmap);
                    XCopyPlane(dpy, pile_annotation.bitmap, XtWindow(wp->w),
                               tile_map->black_gc, 0, 0,
                               pile_annotation.width, pile_annotation.height,
                               dest_x, dest_y, 1);
                    XSetClipOrigin(dpy, tile_map->black_gc, 0, 0);
                    XSetClipMask(dpy, tile_map->black_gc, None);
                    XSetForeground(dpy, tile_map->black_gc,
                                   BlackPixelOfScreen(screen));
                }
            }
        }

        if (inverted) {
            XDrawRectangle(XtDisplay(wp->w), XtWindow(wp->w),
#ifdef USE_WHITE
                           /* kludge for white square... */
                           (tile_map->glyphs[start_row][start_col].glyph
                            == cmap_to_glyph(S_ice))
                                 ? tile_map->black_gc
                                 : tile_map->white_gc,
#else
                           tile_map->white_gc,
#endif
                           (start_col - COL0_OFFSET) * tile_map->square_width,
                           start_row * tile_map->square_height,
                           tile_map->square_width - 1,
                           tile_map->square_height - 1);
        }
    } else {
        struct text_map_info_t *text_map = &map_info->text_map;

#ifdef TEXTCOLOR
        {
            register char *c_ptr;
            char *t_ptr;
            int cur_col, color, win_ystart;
            boolean cur_inv;

            for (row = start_row; row <= stop_row; row++) {
                win_ystart =
                    text_map->square_ascent + (row * text_map->square_height);

                t_ptr = (char *) &(text_map->text[row][start_col]);
                c_ptr = (char *) &(text_map->colors[row][start_col]);
                cur_col = start_col;
                while (cur_col <= stop_col) {
                    color = *c_ptr++;
                    cur_inv = inverted;
                    count = 1;
                    while ((cur_col + count) <= stop_col && *c_ptr == color) {
                        count++;
                        c_ptr++;
                    }
                    if (color >= CLR_MAX) {
                        color -= CLR_MAX;
                        cur_inv = !cur_inv;
                    }

                    XDrawImageString(XtDisplay(wp->w), XtWindow(wp->w),
                                     iflags.use_color
                                        ? (cur_inv
                                           ? text_map->inv_color_gcs[color]
                                           : text_map->color_gcs[color])
                                        : (cur_inv
                                           ? text_map->inv_copy_gc
                                           : text_map->copy_gc),
                                     text_map->square_lbearing
                                         + (text_map->square_width
                                            * (cur_col - COL0_OFFSET)),
                                     win_ystart, t_ptr, count);

                    /* move text pointer and column count */
                    t_ptr += count;
                    cur_col += count;
                } /* col loop */
            }     /* row loop */
        }
#else   /* !TEXTCOLOR */
        {
            int win_row, win_xstart;
            int win_start_row, win_start_col;

            win_start_row = start_row;
            win_start_col = start_col;

            /* We always start at the same x window position and have
               the same character count. */
            win_xstart = text_map->square_lbearing
                         + ((win_start_col - COL0_OFFSET)
                            * text_map->square_width);
            count = stop_col - start_col + 1;

            for (row = start_row, win_row = win_start_row; row <= stop_row;
                 row++, win_row++) {
                XDrawImageString(XtDisplay(wp->w), XtWindow(wp->w),
                                 inverted ? text_map->inv_copy_gc
                                          : text_map->copy_gc,
                                 win_xstart,
                                 text_map->square_ascent
                                    + (win_row * text_map->square_height),
                                 (char *) &(text_map->text[row][start_col]),
                                 count);
            }
        }
#endif  /* ?TEXTCOLOR */
    }
}

/* Adjust the number of rows and columns on the given map window */
void
set_map_size(struct xwindow *wp, Dimension cols, Dimension rows)
{
    Arg args[4];
    Cardinal num_args;

    cols -= COL0_OFFSET;
    if (wp->map_information->is_tile) {
        wp->pixel_width = wp->map_information->tile_map.square_width * cols;
        wp->pixel_height = wp->map_information->tile_map.square_height * rows;
    } else {
        wp->pixel_width = wp->map_information->text_map.square_width * cols;
        wp->pixel_height = wp->map_information->text_map.square_height * rows;
    }

    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, wp->pixel_width); num_args++;
    XtSetArg(args[num_args], XtNheight, wp->pixel_height); num_args++;
    XtSetValues(wp->w, args, num_args);
}

static void
init_text(struct xwindow *wp)
{
    struct map_info_t *map_info = wp->map_information;

    /* set up map_info->text_map->text */
    map_all_unexplored(map_info);

    get_char_info(wp);
    get_text_gc(wp, WindowFont(wp->w));
}

static char map_translations[] = "#override\n\
 <Key>Left: scroll(4)\n\
 <Key>Right: scroll(6)\n\
 <Key>Up: scroll(8)\n\
 <Key>Down: scroll(2)\n\
 <Key>: input() \
";

/*
 * The map window creation routine.
 */
void
create_map_window(
    struct xwindow *wp,
    boolean create_popup, /* True: parent is a popup shell that we create */
    Widget parent)
{
    struct map_info_t *map_info; /* map info pointer */
    Widget map, viewport;
    Arg args[16];
    Cardinal num_args;
    Dimension rows, columns;
#if 0
    int screen_width, screen_height;
#endif
    int i;

    wp->type = NHW_MAP;

    if (create_popup) {
        /*
         * Create a popup that accepts key and button events.
         */
        num_args = 0;
        XtSetArg(args[num_args], XtNinput, False);
        num_args++;

        wp->popup = parent = XtCreatePopupShell("nethack",
                                                topLevelShellWidgetClass,
                                                toplevel, args, num_args);
        /*
         * If we're here, then this is an auxiliary map window.  If we're
         * cancelled via a delete window message, we should just pop down.
         */
    }

    num_args = 0;
    XtSetArg(args[num_args], XtNallowHoriz, True);
    num_args++;
    XtSetArg(args[num_args], XtNallowVert, True);
    num_args++;
#if 0
    XtSetArg(args[num_args], XtNforceBars,  True);
    num_args++;
#endif
    XtSetArg(args[num_args], XtNuseBottom, True);
    num_args++;
    XtSetArg(args[num_args], XtNtranslations,
             XtParseTranslationTable(map_translations));
    num_args++;
    viewport = XtCreateManagedWidget("map_viewport",                /* name */
                                     viewportWidgetClass,  /* from Window.h */
                                     parent,               /* parent widget */
                                     args,               /* set some values */
                                     num_args);  /* number of values to set */

    /*
     * Create a map window.  We need to set the width and height to some
     * value when we create it.  We will change it to the value we want
     * later
     */
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, 100);
    num_args++;
    XtSetArg(args[num_args], XtNheight, 100);
    num_args++;
    XtSetArg(args[num_args], XtNtranslations,
             XtParseTranslationTable(map_translations));
    num_args++;

    wp->w = map = XtCreateManagedWidget(
        "map",             /* name */
        windowWidgetClass, /* widget class from Window.h */
        viewport,          /* parent widget */
        args,              /* set some values */
        num_args);         /* number of values to set */

    XtAddCallback(map, XtNexposeCallback, map_exposed, (XtPointer) 0);

    map_info = wp->map_information =
        (struct map_info_t *) alloc(sizeof (struct map_info_t));

    map_info->viewport_width = map_info->viewport_height = 0;

    /* reset the "new entry" indicators */
    for (i = 0; i < ROWNO; i++) {
        map_info->t_start[i] = COLNO;
        map_info->t_stop[i] = 0;
    }

    /* we probably want to restrict this to the 1st map window only */
    map_info->is_tile = (init_tiles(wp) && iflags.wc_tiled_map);
    init_text(wp);

    /*
     * Initially, set the map widget to be the size specified by the
     * widget rows and columns resources.  We need to do this to
     * correctly set the viewport window size.  After the viewport is
     * realized, then the map can resize to its normal size.
     */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNrows), &rows);
    num_args++;
    XtSetArg(args[num_args], nhStr(XtNcolumns), &columns);
    num_args++;
    XtGetValues(wp->w, args, num_args);

    /* Don't bother with windows larger than ROWNOxCOLNO. */
    if (columns > COLNO)
        columns = COLNO;
    if (rows > ROWNO)
        rows = ROWNO;

    set_map_size(wp, columns, rows);

    /*
     * If we have created our own popup, then realize it so that the
     * viewport is also realized.  Then resize the map window.
     */
    if (create_popup) {
        XtRealizeWidget(wp->popup);
        XSetWMProtocols(XtDisplay(wp->popup), XtWindow(wp->popup),
                        &wm_delete_window, 1);
        set_map_size(wp, COLNO, ROWNO);
    }

    map_all_unexplored(map_info);
}

/*
 * Destroy this map window.
 */
void
destroy_map_window(struct xwindow *wp)
{
    struct map_info_t *map_info = wp->map_information;

    if (wp->popup)
        nh_XtPopdown(wp->popup);

    if (map_info) {
        struct text_map_info_t *text_map = &map_info->text_map;

/* Free allocated GCs. */
#ifdef TEXTCOLOR
        int i;

        for (i = 0; i < CLR_MAX; i++) {
            XtReleaseGC(wp->w, text_map->color_gcs[i]);
            XtReleaseGC(wp->w, text_map->inv_color_gcs[i]);
        }
#else
        XtReleaseGC(wp->w, text_map->copy_gc);
        XtReleaseGC(wp->w, text_map->inv_copy_gc);
#endif

        /* Free malloc'ed space. */
        free((genericptr_t) map_info);
        wp->map_information = 0;
    }

    /* Destroy map widget. */
    if (wp->popup && !wp->keep_window)
        XtDestroyWidget(wp->popup), wp->popup = (Widget) 0;

    if (wp->keep_window)
        XtRemoveCallback(wp->w, XtNexposeCallback, map_exposed,
                         (XtPointer) 0);
    else
        wp->type = NHW_NONE; /* allow re-use */

    /* when map goes away, presumably we're exiting, so get rid of the
       cached extended commands menu (if we aren't actually exiting, it
       will get recreated if needed again) */
    release_extended_cmds();
}

boolean exit_x_event; /* exit condition for the event loop */

#if 0   /*******/
void
pkey(int k)
{
    printf("key = '%s%c'\n", (k < 32) ? "^" : "", (k < 32) ? '@' + k : k);
}
#endif  /***0***/

/*
 * Main X event loop.  Here we accept and dispatch X events.  We only exit
 * under certain circumstances.
 */
int
x_event(int exit_condition)
{
    XEvent event;
    int retval = 0;
    boolean keep_going = TRUE;

    /* Hold globals so function is re-entrant */
    boolean hold_exit_x_event = exit_x_event;

    click_button = NO_CLICK; /* reset click exit condition */
    exit_x_event = FALSE;    /* reset callback exit condition */

    /*
     * Loop until we get a sent event, callback exit, or are accepting key
     * press and button press events and we receive one.
     */
    if ((exit_condition == EXIT_ON_KEY_PRESS
         || exit_condition == EXIT_ON_KEY_OR_BUTTON_PRESS) && incount)
        goto try_test;

    do {
        XtAppNextEvent(app_context, &event);
        XtDispatchEvent(&event);

    /* See if we can exit. */
 try_test:
        switch (exit_condition) {
        case EXIT_ON_SENT_EVENT: {
            XAnyEvent *any = (XAnyEvent *) &event;

            if (any->send_event) {
                retval = 0;
                keep_going = FALSE;
            }
            break;
        }
        case EXIT_ON_EXIT:
            if (exit_x_event) {
                incount = 0;
                retval = 0;
                keep_going = FALSE;
            }
            break;
        case EXIT_ON_KEY_PRESS:
            if (incount != 0) {
                /* get first pressed key */
                --incount;
                retval = inbuf[inptr];
                inptr = (inptr + 1) % INBUF_SIZE;
                /* pkey(retval); */
                keep_going = FALSE;
            } else if (g.program_state.done_hup) {
                retval = '\033';
                inptr = (inptr + 1) % INBUF_SIZE;
                keep_going = FALSE;
            }
            break;
        case EXIT_ON_KEY_OR_BUTTON_PRESS:
            if (incount != 0 || click_button != NO_CLICK) {
                if (click_button != NO_CLICK) { /* button press */
                    /* click values are already set */
                    retval = 0;
                } else { /* key press */
                    /* get first pressed key */
                    --incount;
                    retval = inbuf[inptr];
                    inptr = (inptr + 1) % INBUF_SIZE;
                    /* pkey(retval); */
                }
                keep_going = FALSE;
            } else if (g.program_state.done_hup) {
                retval = '\033';
                inptr = (inptr + 1) % INBUF_SIZE;
                keep_going = FALSE;
            }
            break;
        default:
            panic("x_event: unknown exit condition %d", exit_condition);
            break;
        }
    } while (keep_going);

    /* Restore globals */
    exit_x_event = hold_exit_x_event;

    return retval;
}

/*winmap.c*/

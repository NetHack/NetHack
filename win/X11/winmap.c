/*	SCCS Id: @(#)winmap.c	3.4	1996/04/05	*/
/* Copyright (c) Dean Luick, 1992				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains:
 *	+ global functions print_glyph() and cliparound()
 *	+ the map window routines
 *	+ the char and pointer input routines
 *
 * Notes:
 *	+ We don't really have a good way to get the compiled ROWNO and
 *	  COLNO as defaults.  They are hardwired to the current "correct"
 *	  values in the Window widget.  I am _not_ in favor of including
 *	  some nethack include file for Window.c.
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV	/* X11 include files may define SYSV */
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xatom.h>

#ifdef PRESERVE_NO_SYSV
# ifdef SYSV
#  undef SYSV
# endif
# undef PRESERVE_NO_SYSV
#endif

#include "xwindow.h"	/* map widget declarations */

#include "hack.h"
#include "dlb.h"
#include "winX.h"

#ifdef USE_XPM
#include <X11/xpm.h>
#endif


/* from tile.c */
extern short glyph2tile[];
extern int total_tiles_used;

/* Define these if you really want a lot of junk on your screen. */
/* #define VERBOSE */		/* print various info & events as they happen */
/* #define VERBOSE_UPDATE */	/* print screen update bounds */
/* #define VERBOSE_INPUT */	/* print input events */


#define USE_WHITE	/* almost always use white as a tile cursor border */


static boolean FDECL(init_tiles, (struct xwindow *));
static void FDECL(set_button_values, (Widget,int,int,unsigned));
static void FDECL(map_check_size_change, (struct xwindow *));
static void FDECL(map_update, (struct xwindow *,int,int,int,int,BOOLEAN_P));
static void FDECL(init_text, (struct xwindow *));
static void FDECL(map_exposed, (Widget,XtPointer,XtPointer));
static void FDECL(set_gc, (Widget,Font,char *,Pixel,GC *,GC *));
static void FDECL(get_text_gc, (struct xwindow *,Font));
static void FDECL(get_char_info, (struct xwindow *));
static void FDECL(display_cursor, (struct xwindow *));

/* Global functions ======================================================== */

void
X11_print_glyph(window, x, y, glyph)
    winid window;
    xchar x, y;
    int glyph;
{
    struct map_info_t *map_info;
    boolean update_bbox;

    check_winid(window);
    if (window_list[window].type != NHW_MAP) {
	impossible("print_glyph: can (currently) only print to map windows");
	return;
    }
    map_info = window_list[window].map_information;

    if (map_info->is_tile) {
	unsigned short *t_ptr;

	t_ptr = &map_info->mtype.tile_map->glyphs[y][x];

	if (*t_ptr != glyph) {
	    *t_ptr = glyph;
	    update_bbox = TRUE;
	} else
	    update_bbox = FALSE;

    } else {
	uchar			ch;
	register unsigned char *ch_ptr;
	int			color,och;
	unsigned		special;
#ifdef TEXTCOLOR
	register unsigned char *co_ptr;
#endif
	/* map glyph to character and color */
        mapglyph(glyph, &och, &color, &special, x, y);
	ch = (uchar)och;
	
	/* Only update if we need to. */
	ch_ptr = &map_info->mtype.text_map->text[y][x];

#ifdef TEXTCOLOR
	co_ptr = &map_info->mtype.text_map->colors[y][x];
	if (*ch_ptr != ch || *co_ptr != color)
#else
	if (*ch_ptr != ch)
#endif
	{
	    *ch_ptr = ch;
#ifdef TEXTCOLOR
	    *co_ptr = color;
#endif
	    update_bbox = TRUE;
	} else
	    update_bbox = FALSE;
    }

    if (update_bbox) {		/* update row bbox */
	if ((uchar) x < map_info->t_start[y]) map_info->t_start[y] = x;
	if ((uchar) x > map_info->t_stop[y])  map_info->t_stop[y]  = x;
    }
}

#ifdef CLIPPING
/*
 * The is the tty clip call.  Since X can resize at any time, we can't depend
 * on this being defined.
 */
/*ARGSUSED*/
void X11_cliparound(x, y) int x, y; { }
#endif /* CLIPPING */

/* End global functions ==================================================== */

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

static void
init_annotation(annotation, filename, colorpixel)
struct tile_annotation *annotation;
char *filename;
Pixel colorpixel;
{
    Display *dpy = XtDisplay(toplevel);

    if (0!=XReadBitmapFile(dpy, XtWindow(toplevel), filename,
	    &annotation->width, &annotation->height, &annotation->bitmap,
	    &annotation->hotx, &annotation->hoty)) {
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
post_process_tiles()
{
    Display *dpy = XtDisplay(toplevel);
    unsigned int width, height;

    if (tile_image == 0) return;	/* no tiles */

    height = tile_image->height;
    width  = tile_image->width;

    tile_pixmap = XCreatePixmap(dpy, XtWindow(toplevel),
			width,
    			height,
			DefaultDepth(dpy, DefaultScreen(dpy)));

    XPutImage(dpy, tile_pixmap,
	DefaultGC(dpy, DefaultScreen(dpy)),
	tile_image,
	0,0, 0,0,		/* src, dest top left */
	width,
	height);

    XDestroyImage(tile_image);	/* data bytes free'd also */
    tile_image = 0;

    init_annotation(&pet_annotation,
	appResources.pet_mark_bitmap, appResources.pet_mark_color);
}


/*
 * Open and read the tile file.  Return TRUE if there were no problems.
 * Return FALSE otherwise.
 */
static boolean
init_tiles(wp)
    struct xwindow *wp;
{
#ifdef USE_XPM
    XpmAttributes attributes;
    int errorcode;
#else
    FILE *fp = (FILE *)0;
    x11_header header;
    unsigned char *cp, *colormap = (unsigned char *)0;
    unsigned char *tb, *tile_bytes = (unsigned char *)0;
    int size;
    XColor *colors = (XColor *)0;
    int i, x, y;
    int bitmap_pad;
    int ddepth;
#endif
    char buf[BUFSZ];
    Display *dpy = XtDisplay(toplevel);
    Screen *screen = DefaultScreenOfDisplay(dpy);
    struct map_info_t *map_info = (struct map_info_t *)0;
    struct tile_map_info_t *tile_info = (struct tile_map_info_t *)0;
    unsigned int image_height = 0, image_width = 0;
    boolean result = TRUE;
    XGCValues values;
    XtGCMask mask;

    /* already have tile information */
    if (tile_pixmap != None) goto tiledone;

    map_info = wp->map_information;
    tile_info = map_info->mtype.tile_map =
	    (struct tile_map_info_t *) alloc(sizeof(struct tile_map_info_t));
    (void) memset((genericptr_t) tile_info, 0,
				sizeof(struct tile_map_info_t));

#ifdef USE_XPM
    attributes.valuemask = XpmCloseness;
    attributes.closeness = 25000;

    errorcode = XpmReadFileToImage(dpy, appResources.tile_file,
				   &tile_image, 0, &attributes);

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
    if (tile_image->width % TILES_PER_ROW != 0) {
	Sprintf(buf,
		"%s is not a multiple of %d (number of tiles/row) pixels wide",
		appResources.tile_file, TILES_PER_ROW);
	X11_raw_print(buf);
	XDestroyImage(tile_image);
	tile_image = 0;
	result = FALSE;
	goto tiledone;
    }

    /* infer tile dimensions from image size, assume square tiles */
    image_width = tile_image->width;
    image_height = tile_image->height;
    tile_width = image_width / TILES_PER_ROW;
    tile_height = tile_width;
    tile_count = (image_width * image_height) / (tile_width * tile_height);

    if (tile_count < total_tiles_used) {
	Sprintf(buf, "%s incomplete, expecting %d tiles, found %d",
		appResources.tile_file, total_tiles_used, tile_count);
	X11_raw_print(buf);
	result = FALSE;
	goto tiledone;
    }
#else
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

    if (fread((char *) &header, sizeof(header), 1, fp) != 1) {
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

# ifdef VERBOSE
    fprintf(stderr, "X11 tile file:\n    version %ld\n    ncolors %ld\n    tile width %ld\n    tile height %ld\n    per row %ld\n    ntiles %ld\n",
	header.version,
	header.ncolors,
	header.tile_width,
	header.tile_height,
	header.per_row,
	header.ntiles);
# endif

    size = 3*header.ncolors;
    colormap = (unsigned char *) alloc((unsigned)size);
    if (fread((char *) colormap, 1, size, fp) != size) {
	X11_raw_print("read of colormap failed");
	result = FALSE;
	goto tiledone;
    }

/* defined in decl.h - these are _not_ good defines to have */
#undef red
#undef green
#undef blue

    colors = (XColor *) alloc(sizeof(XColor) * (unsigned)header.ncolors);
    for (i = 0; i < header.ncolors; i++) {
	cp = colormap + (3 * i);
	colors[i].red   = cp[0] * 256;
	colors[i].green = cp[1] * 256;
	colors[i].blue  = cp[2] * 256;
	colors[i].flags = 0;
	colors[i].pixel = 0;

	if (!XAllocColor(dpy, DefaultColormapOfScreen(screen), &colors[i]) &&
	    !nhApproxColor(screen, DefaultColormapOfScreen(screen),
			   (char *)0, &colors[i])) {
	    Sprintf(buf, "%dth out of %ld color allocation failed",
		    i, header.ncolors);
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
    tile_bytes = (unsigned char *) alloc((unsigned)tile_count*size);
    if (fread((char *) tile_bytes, size, tile_count, fp) != tile_count) {
	X11_raw_print("read of tile bytes failed");
	result = FALSE;
	goto tiledone;
    }

    if (header.ntiles < total_tiles_used) {
	Sprintf(buf, "tile file incomplete, expecting %d tiles, found %lu",
		total_tiles_used, header.ntiles);
	X11_raw_print(buf);
	result = FALSE;
	goto tiledone;
    }


    if (appResources.double_tile_size) {
	tile_width  = 2*header.tile_width;
	tile_height = 2*header.tile_height;
    } else {
	tile_width  = header.tile_width;
	tile_height = header.tile_height;
    }

    image_height = tile_height * tile_count / header.per_row;
    image_width  = tile_width * header.per_row;

    /* calculate bitmap_pad */
    if (ddepth > 16)
	bitmap_pad = 32;
    else if (ddepth > 8)
	bitmap_pad = 16;
    else
	bitmap_pad = 8;

    tile_image = XCreateImage(dpy, DefaultVisualOfScreen(screen),
		ddepth,			/* depth */
		ZPixmap,		/* format */
		0,			/* offset */
		0,			/* data */
		image_width,		/* width */
		image_height,		/* height */
		bitmap_pad,		/* bit pad */
		0);			/* bytes_per_line */

    if (!tile_image)
	impossible("init_tiles: insufficient memory to create image");

    /* now we know the physical memory requirements, we can allocate space */
    tile_image->data =
	(char *) alloc((unsigned)tile_image->bytes_per_line * image_height);

    if (appResources.double_tile_size) {
	unsigned long *expanded_row =
	    (unsigned long *)alloc(sizeof(unsigned long)*(unsigned)image_width);

	tb = tile_bytes;
	for (y = 0; y < image_height; y++) {
	    for (x = 0; x < image_width/2; x++)
		expanded_row[2*x] =
			    expanded_row[(2*x)+1] = colors[*tb++].pixel;

	    for (x = 0; x < image_width; x++)
		XPutPixel(tile_image, x, y, expanded_row[x]);

	    y++;	/* duplicate row */
	    for (x = 0; x < image_width; x++)
		XPutPixel(tile_image, x, y, expanded_row[x]);
	}
	free((genericptr_t)expanded_row);

    } else {

	for (tb = tile_bytes, y = 0; y < image_height; y++)
	    for (x = 0; x < image_width; x++, tb++)
		XPutPixel(tile_image, x, y, colors[*tb].pixel);
    }
#endif /* USE_XPM */

    /* fake an inverted tile by drawing a border around the edges */
#ifdef USE_WHITE
    /* use white or black as the border */
    mask = GCFunction | GCForeground | GCGraphicsExposures;
    values.graphics_exposures = False;
    values.foreground = WhitePixelOfScreen(screen);
    values.function   = GXcopy;
    tile_info->white_gc = XtGetGC(wp->w, mask, &values);
    values.graphics_exposures = False;
    values.foreground = BlackPixelOfScreen(screen);
    values.function   = GXcopy;
    tile_info->black_gc = XtGetGC(wp->w, mask, &values);
#else
    /*
     * Use xor so we don't have to check for special colors.  Xor white
     * against the upper left pixel of the corridor so that we have a
     * white rectangle when in a corridor.
     */
    mask = GCFunction | GCForeground | GCGraphicsExposures;
    values.graphics_exposures = False;
    values.foreground = WhitePixelOfScreen(screen) ^
	XGetPixel(tile_image, 0, tile_height*glyph2tile[cmap_to_glyph(S_corr)]);
    values.function = GXxor;
    tile_info->white_gc = XtGetGC(wp->w, mask, &values);

    mask = GCFunction | GCGraphicsExposures;
    values.function = GXCopy;
    values.graphics_exposures = False;
    tile_info->black_gc = XtGetGC(wp->w, mask, &values);
#endif /* USE_WHITE */

tiledone:
#ifndef USE_XPM
    if (fp) (void) fclose(fp);
    if (colormap) free((genericptr_t)colormap);
    if (tile_bytes) free((genericptr_t)tile_bytes);
    if (colors) free((genericptr_t)colors);
#endif

    if (result) {				/* succeeded */
	map_info->square_height = tile_height;
	map_info->square_width = tile_width;
	map_info->square_ascent = 0;
	map_info->square_lbearing = 0;
	tile_info->image_width = image_width;
	tile_info->image_height = image_height;
    } else {
	if (tile_info) free((genericptr_t)tile_info);
	tile_info = 0;
    }

    return result;
}


/*
 * Make sure the map's cursor is always visible.
 */
void
check_cursor_visibility(wp)
    struct xwindow *wp;
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
    vert_sb  = XtNameToWidget(viewport, "vertical");

/* All values are relative to currently visible area */

#define V_BORDER 0.3	/* if this far from vert edge, shift */
#define H_BORDER 0.3	/* if this from from horiz edge, shift */

#define H_DELTA 0.4	/* distance of horiz shift */
#define V_DELTA 0.4	/* distance of vert shift */

    if (horiz_sb) {
	XtSetArg(arg[0], XtNshown,	&shown);
	XtSetArg(arg[1], XtNtopOfThumb, &top);
	XtGetValues(horiz_sb, arg, TWO);

	/* [ALI] Don't assume map widget is the same size as actual map */
	cursor_middle = (wp->cursx + 0.5) * wp->map_information->square_width /
	  wp->pixel_width;
	do_call = True;

#ifdef VERBOSE
	if (cursor_middle < top) {
	    s = " outside left";
	} else if (cursor_middle < top + shown*H_BORDER) {
	    s = " close to left";
	} else if (cursor_middle > (top + shown)) {
	    s = " outside right";
	} else if (cursor_middle > (top + shown - shown*H_BORDER)) {
	    s = " close to right";
	} else {
	    s = "";
	}
	printf("Horiz: shown = %3.2f, top = %3.2f%s", shown, top, s);
#endif

	if (cursor_middle < top) {
	    top = cursor_middle - shown*H_DELTA;
	    if (top < 0.0) top = 0.0;
	} else if (cursor_middle < top + shown*H_BORDER) {
	    top -= shown*H_DELTA;
	    if (top < 0.0) top = 0.0;
	} else if (cursor_middle > (top + shown)) {
	    top = cursor_middle - shown*H_DELTA;
	    if (top < 0.0) top = 0.0;
	    if (top + shown > 1.0) top = 1.0 - shown;
	} else if (cursor_middle > (top + shown - shown*H_BORDER)) {
	    top += shown*H_DELTA;
	    if (top + shown > 1.0) top = 1.0 - shown;
	} else {
	    do_call = False;
	}

	if (do_call) {
	    XtCallCallbacks(horiz_sb, XtNjumpProc, &top);
	    adjusted = True;
	}
    }

    if (vert_sb) {
	XtSetArg(arg[0], XtNshown,      &shown);
	XtSetArg(arg[1], XtNtopOfThumb, &top);
	XtGetValues(vert_sb, arg, TWO);

	cursor_middle = (wp->cursy + 0.5) * wp->map_information->square_height /
	  wp->pixel_height;
	do_call = True;

#ifdef VERBOSE
	if (cursor_middle < top) {
	    s = " above top";
	} else if (cursor_middle < top + shown*V_BORDER) {
	    s = " close to top";
	} else if (cursor_middle > (top + shown)) {
	    s = " below bottom";
	} else if (cursor_middle > (top + shown - shown*V_BORDER)) {
	    s = " close to bottom";
	} else {
	    s = "";
	}
	printf("%sVert: shown = %3.2f, top = %3.2f%s",
				    horiz_sb ? ";  " : "", shown, top, s);
#endif

	if (cursor_middle < top) {
	    top = cursor_middle - shown*V_DELTA;
	    if (top < 0.0) top = 0.0;
	} else if (cursor_middle < top + shown*V_BORDER) {
	    top -= shown*V_DELTA;
	    if (top < 0.0) top = 0.0;
	} else if (cursor_middle > (top + shown)) {
	    top = cursor_middle - shown*V_DELTA;
	    if (top < 0.0) top = 0.0;
	    if (top + shown > 1.0) top = 1.0 - shown;
	} else if (cursor_middle > (top + shown - shown*V_BORDER)) {
	    top += shown*V_DELTA;
	    if (top + shown > 1.0) top = 1.0 - shown;
	} else {
	    do_call = False;
	}

	if (do_call) {
	    XtCallCallbacks(vert_sb, XtNjumpProc, &top);
	    adjusted = True;
	}
    }

    /* make sure cursor is displayed during dowhatis.. */
    if (adjusted) display_cursor(wp);

#ifdef VERBOSE
    if (horiz_sb || vert_sb) printf("\n");
#endif
}


/*
 * Check to see if the viewport has grown smaller.  If so, then we want to make
 * sure that the cursor is still on the screen.  We do this to keep the cursor
 * on the screen when the user resizes the nethack window.
 */
static void
map_check_size_change(wp)
    struct xwindow *wp;
{
    struct map_info_t *map_info = wp->map_information;
    Arg arg[2];
    Dimension new_width, new_height;
    Widget viewport;

    viewport = XtParent(wp->w);

    XtSetArg(arg[0], XtNwidth,  &new_width);
    XtSetArg(arg[1], XtNheight, &new_height);
    XtGetValues(viewport, arg, TWO);

    /* Only do cursor check if new size is smaller. */
    if (new_width < map_info->viewport_width
		    || new_height < map_info->viewport_height) {
	/* [ALI] If the viewport was larger than the map (and so the map
	 * widget was contrained to be larger than the actual map) then we
	 * may be able to shrink the map widget as the viewport shrinks.
	 */
	wp->pixel_width = map_info->square_width * COLNO;
	if (wp->pixel_width < new_width)
	    wp->pixel_width = new_width;
	wp->pixel_height = map_info->square_height * ROWNO;
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
set_gc(w, font, resource_name, bgpixel, regular, inverse)
    Widget w;
    Font font;
    char *resource_name;
    Pixel bgpixel;
    GC   *regular, *inverse;
{
    XGCValues values;
    XtGCMask mask = GCFunction | GCForeground | GCBackground | GCFont;
    Pixel curpixel;
    Arg arg[1];

    XtSetArg(arg[0], resource_name, &curpixel);
    XtGetValues(w, arg, ONE);

    values.foreground = curpixel;
    values.background = bgpixel;
    values.function   = GXcopy;
    values.font	      = font;
    *regular = XtGetGC(w, mask, &values);
    values.foreground = bgpixel;
    values.background = curpixel;
    values.function   = GXcopy;
    values.font	      = font;
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
get_text_gc(wp, font)
    struct xwindow *wp;
    Font font;
{
    struct map_info_t *map_info = wp->map_information;
    Pixel bgpixel;
    Arg arg[1];

    /* Get background pixel. */
    XtSetArg(arg[0], XtNbackground, &bgpixel);
    XtGetValues(wp->w, arg, ONE);

#ifdef TEXTCOLOR
#define set_color_gc(nh_color, resource_name)			\
	    set_gc(wp->w, font, resource_name, bgpixel,		\
		&map_info->mtype.text_map->color_gcs[nh_color],	\
		    &map_info->mtype.text_map->inv_color_gcs[nh_color]);

    set_color_gc(CLR_BLACK,	XtNblack);
    set_color_gc(CLR_RED,	XtNred);
    set_color_gc(CLR_GREEN,	XtNgreen);
    set_color_gc(CLR_BROWN,	XtNbrown);
    set_color_gc(CLR_BLUE,	XtNblue);
    set_color_gc(CLR_MAGENTA,	XtNmagenta);
    set_color_gc(CLR_CYAN,	XtNcyan);
    set_color_gc(CLR_GRAY,	XtNgray);
    set_color_gc(NO_COLOR,	XtNforeground);
    set_color_gc(CLR_ORANGE,	XtNorange);
    set_color_gc(CLR_BRIGHT_GREEN, XtNbright_green);
    set_color_gc(CLR_YELLOW,	XtNyellow);
    set_color_gc(CLR_BRIGHT_BLUE, XtNbright_blue);
    set_color_gc(CLR_BRIGHT_MAGENTA, XtNbright_magenta);
    set_color_gc(CLR_BRIGHT_CYAN, XtNbright_cyan);
    set_color_gc(CLR_WHITE,	XtNwhite);
#else
    set_gc(wp->w, font, XtNforeground, bgpixel,
		&map_info->mtype.text_map->copy_gc,
		&map_info->mtype.text_map->inv_copy_gc);
#endif
}


/*
 * Display the cursor on the map window.
 */
static void
display_cursor(wp)
    struct xwindow *wp;
{
    /* Redisplay the cursor location inverted. */
    map_update(wp, wp->cursy, wp->cursy, wp->cursx, wp->cursx, TRUE);
}


/*
 * Check if there are any changed characters.  If so, then plaster them on
 * the screen.
 */
void
display_map_window(wp)
    struct xwindow *wp;
{
    register int row;
    struct map_info_t *map_info = wp->map_information;

    /*
     * If the previous cursor position is not the same as the current
     * cursor position, then update the old cursor position.
     */
    if (wp->prevx != wp->cursx || wp->prevy != wp->cursy) {
	register unsigned int x = wp->prevx, y = wp->prevy;
	if (x < map_info->t_start[y]) map_info->t_start[y] = x;
	if (x > map_info->t_stop[y])  map_info->t_stop[y]  = x;
    }

    for (row = 0; row < ROWNO; row++) {
	if (map_info->t_start[row] <= map_info->t_stop[row]) {
	    map_update(wp, row, row,
			(int) map_info->t_start[row],
			(int) map_info->t_stop[row], FALSE);
	    map_info->t_start[row] = COLNO-1;
	    map_info->t_stop[row] = 0;
	}
    }
    display_cursor(wp);
    wp->prevx = wp->cursx;	/* adjust old cursor position */
    wp->prevy = wp->cursy;
}

/*
 * Set all map tiles to S_stone
 */
static void
map_all_stone(map_info)
struct map_info_t *map_info;
{
    int i;
    unsigned short *sp, stone;
    stone = cmap_to_glyph(S_stone);

    for (sp = (unsigned short *) map_info->mtype.tile_map->glyphs, i = 0;
	i < ROWNO*COLNO; sp++, i++)

    *sp = stone;
}

/*
 * Fill the saved screen characters with the "clear" tile or character.
 *
 * Flush out everything by resetting the "new" bounds and calling
 * display_map_window().
 */
void
clear_map_window(wp)
    struct xwindow *wp;
{
    struct map_info_t *map_info = wp->map_information;

    if (map_info->is_tile) {
	map_all_stone(map_info);
    } else {
	/* Fill text with spaces, and update */
	(void) memset((genericptr_t) map_info->mtype.text_map->text, ' ',
			sizeof(map_info->mtype.text_map->text));
#ifdef TEXTCOLOR
	(void) memset((genericptr_t) map_info->mtype.text_map->colors, NO_COLOR,
			sizeof(map_info->mtype.text_map->colors));
#endif
    }

    /* force a full update */
    (void) memset((genericptr_t) map_info->t_start, (char) 0,
			sizeof(map_info->t_start));
    (void) memset((genericptr_t) map_info->t_stop, (char) COLNO-1,
			sizeof(map_info->t_stop));
    display_map_window(wp);
}

/*
 * Retreive the font associated with the map window and save attributes
 * that are used when updating it.
 */
static void
get_char_info(wp)
    struct xwindow *wp;
{
    XFontStruct *fs;
    struct map_info_t *map_info = wp->map_information;

    fs = WindowFontStruct(wp->w);
    map_info->square_width    = fs->max_bounds.width;
    map_info->square_height   = fs->max_bounds.ascent + fs->max_bounds.descent;
    map_info->square_ascent   = fs->max_bounds.ascent;
    map_info->square_lbearing = -fs->min_bounds.lbearing;

#ifdef VERBOSE
    printf("Font information:\n");
    printf("fid = %ld, direction = %d\n", fs->fid, fs->direction);
    printf("first = %d, last = %d\n",
			fs->min_char_or_byte2, fs->max_char_or_byte2);
    printf("all chars exist? %s\n", fs->all_chars_exist?"yes":"no");
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
	    map_info->square_width, map_info->square_height);
#endif

    if (fs->min_bounds.width != fs->max_bounds.width)
	X11_raw_print("Warning:  map font is not monospaced!");
}

/*
 * keyhit buffer
 */
#define INBUF_SIZE 64
int inbuf[INBUF_SIZE];
int incount = 0;
int inptr = 0;	/* points to valid data */


/*
 * Keyboard and button event handler for map window.
 */
void
map_input(w, event, params, num_params)
    Widget   w;
    XEvent   *event;
    String   *params;
    Cardinal *num_params;
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
	    button = (XButtonEvent *) event;
#ifdef VERBOSE_INPUT
	    printf("button press\n");
#endif
	    if (in_nparams > 0 &&
		(nbytes = strlen(params[0])) < MAX_KEY_STRING) {
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
	    if(appResources.slow && input_func) {
		(*input_func)(w, event, params, num_params);
		break;
	    }

	    /*
	     * Don't use key_event_to_char() because we want to be able
	     * to allow keys mapped to multiple characters.
	     */
	    key = (XKeyEvent *) event;
	    if (in_nparams > 0 &&
		(nbytes = strlen(params[0])) < MAX_KEY_STRING) {
		Strcpy(keystring, params[0]);
	    } else {
		/*
		 * Assume that mod1 is really the meta key.
		 */
		meta = !!(key->state & Mod1Mask);
		nbytes =
		    XLookupString(key, keystring, MAX_KEY_STRING,
				  (KeySym *)0, (XComposeStatus *)0);
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
			inbuf[(inptr+incount)%INBUF_SIZE] =
			    ((int) c) + (meta ? 0x80 : 0);
			incount++;
		    } else {
			X11_nhbell();
		    }
#ifdef VERBOSE_INPUT
		    if (meta)			/* meta will print as M<c> */
			(void) putchar('M');
		    if (c < ' ') {		/* ctrl will print as ^<c> */
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
set_button_values(w, x, y, button)
    Widget w;
    int x;
    int y;
    unsigned int button;
{
    struct xwindow *wp;
    struct map_info_t *map_info;

    wp = find_widget(w);
    map_info = wp->map_information;

    click_x = x / map_info->square_width;
    click_y = y / map_info->square_height;

    /* The values can be out of range if the map window has been resized */
    /* to be larger than the max size.					 */
    if (click_x >= COLNO) click_x = COLNO-1;
    if (click_y >= ROWNO) click_x = ROWNO-1;

    /* Map all buttons but the first to the second click */
    click_button = (button == Button1) ? CLICK_1 : CLICK_2;
}

/*
 * Map window expose callback.
 */
/*ARGSUSED*/
static void
map_exposed(w, client_data, widget_data)
    Widget w;
    XtPointer client_data;	/* unused */
    XtPointer widget_data;	/* expose event from Window widget */
{
    int x, y;
    struct xwindow *wp;
    struct map_info_t *map_info;
    unsigned width, height;
    int start_row, stop_row, start_col, stop_col;
    XExposeEvent *event = (XExposeEvent *) widget_data;
    int t_height, t_width;	/* tile/text height & width */

    if (!XtIsRealized(w) || event->count > 0) return;

    wp = find_widget(w);
    map_info = wp->map_information;
    if (wp->keep_window && !map_info) return;
    /*
     * The map is sent an expose event when the viewport resizes.  Make sure
     * that the cursor is still in the viewport after the resize.
     */
    map_check_size_change(wp);

    if (event) {		/* called from button-event */
	x      = event->x;
	y      = event->y;
	width  = event->width;
	height = event->height;
    } else {
	x     = 0;
	y     = 0;
	width = wp->pixel_width;
	height= wp->pixel_height;
    }
    /*
     * Convert pixels into INCLUSIVE text rows and columns.
     */
    t_height = map_info->square_height;
    t_width = map_info->square_width;
    start_row = y / t_height;
    stop_row = ((y + height) / t_height) +
		((((y + height) % t_height) == 0) ? 0 : 1) - 1;

    start_col = x / t_width;
    stop_col = ((x + width) / t_width) +
		((((x + width) % t_width) == 0) ? 0 : 1) - 1;

#ifdef VERBOSE
    printf("map_exposed: x = %d, y = %d, width = %d, height = %d\n",
						    x, y, width, height);
    printf("chars %d x %d, rows %d to %d, columns %d to %d\n",
			map_info->square_height, map_info->square_width,
			start_row, stop_row, start_col, stop_col);
#endif

    /* Out of range values are possible if the map window is resized to be */
    /* bigger than the largest expected value.				   */
    if (stop_row >= ROWNO) stop_row = ROWNO-1;
    if (stop_col >= COLNO) stop_col = COLNO-1;

    map_update(wp, start_row, stop_row, start_col, stop_col, FALSE);
    display_cursor(wp);		/* make sure cursor shows up */
}

/*
 * Do the actual work of the putting characters onto our X window.  This
 * is called from the expose event routine, the display window (flush)
 * routine, and the display cursor routine.  The later is a kludge that
 * involves the inverted parameter of this function.  A better solution
 * would be to double the color count, with any color above CLR_MAX
 * being inverted.
 *
 * This works for rectangular regions (this includes one line rectangles).
 * The start and stop columns are *inclusive*.
 */
static void
map_update(wp, start_row, stop_row, start_col, stop_col, inverted)
    struct xwindow *wp;
    int start_row, stop_row, start_col, stop_col;
    boolean inverted;
{
    int win_start_row, win_start_col;
    struct map_info_t *map_info = wp->map_information;
    int row;
    register int count;

    if (start_row < 0 || stop_row >= ROWNO) {
	impossible("map_update:  bad row range %d-%d\n", start_row, stop_row);
	return;
    }
    if (start_col < 0 || stop_col >=COLNO) {
	impossible("map_update:  bad col range %d-%d\n", start_col, stop_col);
	return;
    }

#ifdef VERBOSE_UPDATE
    printf("update: [0x%x] %d %d %d %d\n",
		(int) wp->w, start_row, stop_row, start_col, stop_col);
#endif
    win_start_row = start_row;
    win_start_col = start_col;

    if (map_info->is_tile) {
	struct tile_map_info_t *tile_map = map_info->mtype.tile_map;
	int cur_col;
	Display* dpy = XtDisplay(wp->w);
	Screen* screen = DefaultScreenOfDisplay(dpy);

	for (row = start_row; row <= stop_row; row++) {
	    for (cur_col = start_col; cur_col <= stop_col; cur_col++) {
		int glyph = tile_map->glyphs[row][cur_col];
		int tile = glyph2tile[glyph];
		int src_x, src_y;
		int dest_x = cur_col * map_info->square_width;
		int dest_y = row * map_info->square_height;

		src_x = (tile % TILES_PER_ROW) * tile_width;
		src_y = (tile / TILES_PER_ROW) * tile_height;
		XCopyArea(dpy, tile_pixmap, XtWindow(wp->w),
			  tile_map->black_gc,	/* no grapics_expose */
			  src_x, src_y,
			  tile_width, tile_height,
			  dest_x, dest_y);

		if (glyph_is_pet(glyph) && iflags.hilite_pet) {
		    /* draw pet annotation (a heart) */
		    XSetForeground(dpy, tile_map->black_gc, pet_annotation.foreground);
		    XSetClipOrigin(dpy, tile_map->black_gc, dest_x, dest_y);
		    XSetClipMask(dpy, tile_map->black_gc, pet_annotation.bitmap);
		    XCopyPlane(
			dpy,
			pet_annotation.bitmap,
			XtWindow(wp->w),
			tile_map->black_gc,
			0,0,
			pet_annotation.width,pet_annotation.height,
			dest_x,dest_y,
			1
		    );
		    XSetClipOrigin(dpy, tile_map->black_gc, 0, 0);
		    XSetClipMask(dpy, tile_map->black_gc, None);
		    XSetForeground(dpy, tile_map->black_gc, BlackPixelOfScreen(screen));
		}
	    }
	}

	if (inverted) {
	    XDrawRectangle(XtDisplay(wp->w), XtWindow(wp->w),
#ifdef USE_WHITE
		/* kludge for white square... */
		tile_map->glyphs[start_row][start_col] ==
		    cmap_to_glyph(S_ice) ?
			tile_map->black_gc : tile_map->white_gc,
#else
		tile_map->white_gc,
#endif
		start_col * map_info->square_width,
		start_row * map_info->square_height,
		map_info->square_width-1,
		map_info->square_height-1);
	}
    } else {
	struct text_map_info_t *text_map = map_info->mtype.text_map;

#ifdef TEXTCOLOR
	if (iflags.use_color) {
	    register char *c_ptr;
	    char *t_ptr;
	    int cur_col, color, win_ystart;

	    for (row = start_row; row <= stop_row; row++) {
		win_ystart = map_info->square_ascent +
					(row * map_info->square_height);

		t_ptr = (char *) &(text_map->text[row][start_col]);
		c_ptr = (char *) &(text_map->colors[row][start_col]);
		cur_col = start_col;
		while (cur_col <= stop_col) {
		    color = *c_ptr++;
		    count = 1;
		    while ((cur_col + count) <= stop_col && *c_ptr == color) {
			count++;
			c_ptr++;
		    }

		    XDrawImageString(XtDisplay(wp->w), XtWindow(wp->w),
			inverted ? text_map->inv_color_gcs[color] :
				   text_map->color_gcs[color],
			map_info->square_lbearing + (map_info->square_width * cur_col),
			win_ystart,
			t_ptr, count);

		    /* move text pointer and column count */
		    t_ptr += count;
		    cur_col += count;
		} /* col loop */
	    } /* row loop */
	} else
#endif /* TEXTCOLOR */
	{
	    int win_row, win_xstart;

	    /* We always start at the same x window position and have	*/
	    /* the same character count.				*/
	    win_xstart = map_info->square_lbearing +
				    (win_start_col * map_info->square_width);
	    count = stop_col - start_col + 1;

	    for (row = start_row, win_row = win_start_row;
					row <= stop_row; row++, win_row++) {

		XDrawImageString(XtDisplay(wp->w), XtWindow(wp->w),
		    inverted ? text_map->inv_copy_gc : text_map->copy_gc,
		    win_xstart,
		    map_info->square_ascent + (win_row * map_info->square_height),
		    (char *) &(text_map->text[row][start_col]), count);
	    }
	}
    }
}

/* Adjust the number of rows and columns on the given map window */
void
set_map_size(wp, cols, rows)
    struct xwindow *wp;
    Dimension cols, rows;
{
    Arg args[4];
    Cardinal num_args;

    wp->pixel_width  = wp->map_information->square_width  * cols;
    wp->pixel_height = wp->map_information->square_height * rows;

    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, wp->pixel_width);   num_args++;
    XtSetArg(args[num_args], XtNheight, wp->pixel_height); num_args++;
    XtSetValues(wp->w, args, num_args);
}


static void
init_text(wp)
    struct xwindow *wp;
{

    struct map_info_t *map_info = wp->map_information;
    struct text_map_info_t *text_map;

    map_info->is_tile = FALSE;
    text_map = map_info->mtype.text_map =
	(struct text_map_info_t *) alloc(sizeof(struct text_map_info_t));

    (void) memset((genericptr_t) text_map->text, ' ', sizeof(text_map->text));
#ifdef TEXTCOLOR
    (void) memset((genericptr_t) text_map->colors, NO_COLOR,
			sizeof(text_map->colors));
#endif

    get_char_info(wp);
    get_text_gc(wp, WindowFont(wp->w));
}

static char map_translations[] =
"#override\n\
 <Key>Left: scroll(4)\n\
 <Key>Right: scroll(6)\n\
 <Key>Up: scroll(8)\n\
 <Key>Down: scroll(2)\n\
 <Key>:		input()	\
";

/*
 * The map window creation routine.
 */
void
create_map_window(wp, create_popup, parent)
    struct xwindow *wp;
    boolean create_popup;	/* parent is a popup shell that we create */
    Widget parent;
{
    struct map_info_t *map_info;	/* map info pointer */
    Widget map, viewport;
    Arg args[16];
    Cardinal num_args;
    Dimension rows, columns;
#if 0
    int screen_width, screen_height;
#endif

    wp->type = NHW_MAP;

    if (create_popup) {
	/*
	 * Create a popup that accepts key and button events.
	 */
	num_args = 0;
	XtSetArg(args[num_args], XtNinput, False);            num_args++;

	wp->popup = parent = XtCreatePopupShell("nethack",
					topLevelShellWidgetClass,
				       toplevel, args, num_args);
	/*
	 * If we're here, then this is an auxiliary map window.  If we're
	 * cancelled via a delete window message, we should just pop down.
	 */
    }

    num_args = 0;
    XtSetArg(args[num_args], XtNallowHoriz, True);	num_args++;
    XtSetArg(args[num_args], XtNallowVert,  True);	num_args++;
    /* XtSetArg(args[num_args], XtNforceBars,  True);	num_args++; */
    XtSetArg(args[num_args], XtNuseBottom,  True);	num_args++;
    XtSetArg(args[num_args], XtNtranslations,
		XtParseTranslationTable(map_translations));	num_args++;
    viewport = XtCreateManagedWidget(
			"map_viewport",		/* name */
			viewportWidgetClass,	/* widget class from Window.h */
			parent,			/* parent widget */
			args,			/* set some values */
			num_args);		/* number of values to set */

    /*
     * Create a map window.  We need to set the width and height to some
     * value when we create it.  We will change it to the value we want
     * later
     */
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth,  100); num_args++;
    XtSetArg(args[num_args], XtNheight, 100); num_args++;
    XtSetArg(args[num_args], XtNtranslations,
		XtParseTranslationTable(map_translations));	num_args++;

    wp->w = map = XtCreateManagedWidget(
		"map",			/* name */
		windowWidgetClass,	/* widget class from Window.h */
		viewport,		/* parent widget */
		args,			/* set some values */
		num_args);		/* number of values to set */

    XtAddCallback(map, XtNexposeCallback, map_exposed, (XtPointer) 0);

    map_info = wp->map_information =
			(struct map_info_t *) alloc(sizeof(struct map_info_t));

    map_info->viewport_width = map_info->viewport_height = 0;

    /* reset the "new entry" indicators */
    (void) memset((genericptr_t) map_info->t_start, (char) COLNO,
			sizeof(map_info->t_start));
    (void) memset((genericptr_t) map_info->t_stop, (char) 0,
			sizeof(map_info->t_stop));

    /* we probably want to restrict this to the 1st map window only */
    if (appResources.tile_file[0] && init_tiles(wp)) {
	map_info->is_tile = TRUE;
    } else {
	init_text(wp);
	map_info->is_tile = FALSE;
    }


    /*
     * Initially, set the map widget to be the size specified by the
     * widget rows and columns resources.  We need to do this to
     * correctly set the viewport window size.  After the viewport is
     * realized, then the map can resize to its normal size.
     */
    num_args = 0;
    XtSetArg(args[num_args], XtNrows,    &rows);	num_args++;
    XtSetArg(args[num_args], XtNcolumns, &columns);	num_args++;
    XtGetValues(wp->w, args, num_args);

    /* Don't bother with windows larger than ROWNOxCOLNO. */
    if (columns > COLNO) columns = COLNO;
    if (rows    > ROWNO) rows = ROWNO;

#if 0 /* This is insufficient.  We now resize final window in winX.c */
    /*
     * Check for overrunning the size of the screen.  This does an ad hoc
     * job.
     *
     * Width:	We expect that there is nothing but borders on either side
     *		of the map window.  Use some arbitrary width to decide
     *		when to shrink.
     *
     * Height:	if the map takes up more than 1/2 of the screen height, start
     *		reducing its size.
     */
    screen_height = HeightOfScreen(XtScreen(wp->w));
    screen_width  = WidthOfScreen(XtScreen(wp->w));

#define WOFF 50
    if ((int)(columns*map_info->square_width) > screen_width-WOFF) {
	columns = (screen_width-WOFF) / map_info->square_width;
	if (columns == 0) columns = 1;
    }

    if ((int)(rows*map_info->square_height) > screen_height/2) {
	rows = screen_height / (2*map_info->square_height);
	if (rows == 0) rows = 1;
    }
#endif

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

    if (map_info->is_tile) {
	map_all_stone(map_info);
    }
}

/*
 * Destroy this map window.
 */
void
destroy_map_window(wp)
    struct xwindow *wp;
{
    struct map_info_t *map_info = wp->map_information;

    if (wp->popup)
	nh_XtPopdown(wp->popup);

    if (map_info) {
	struct text_map_info_t *text_map = map_info->mtype.text_map;

	/* Free allocated GCs. */
	if (!map_info->is_tile) {
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
	}
	/* free alloc'ed text information */
	free((genericptr_t)text_map),   map_info->mtype.text_map = 0;

	/* Free malloc'ed space. */
	free((genericptr_t)map_info),  wp->map_information = 0;
    }

	/* Destroy map widget. */
    if (wp->popup && !wp->keep_window)
	XtDestroyWidget(wp->popup),  wp->popup = (Widget)0;

    if (wp->keep_window)
	XtRemoveCallback(wp->w, XtNexposeCallback, map_exposed, (XtPointer)0);
    else
	wp->type = NHW_NONE;	/* allow re-use */
}



boolean exit_x_event;	/* exit condition for the event loop */
/*******
pkey(k)
    int k;
{
    printf("key = '%s%c'\n", (k<32) ? "^":"", (k<32) ? '@'+k : k);
}
******/

/*
 * Main X event loop.  Here we accept and dispatch X events.  We only exit
 * under certain circumstances.
 */
int
x_event(exit_condition)
    int exit_condition;
{
    XEvent  event;
    int     retval = 0;
    boolean keep_going = TRUE;

    /* Hold globals so function is re-entrant */
    boolean hold_exit_x_event = exit_x_event;

    click_button = NO_CLICK;	/* reset click exit condition */
    exit_x_event = FALSE;	/* reset callback exit condition */

    /*
     * Loop until we get a sent event, callback exit, or are accepting key
     * press and button press events and we receive one.
     */
    if((exit_condition == EXIT_ON_KEY_PRESS ||
	exit_condition == EXIT_ON_KEY_OR_BUTTON_PRESS) && incount)
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
		    inptr = (inptr+1) % INBUF_SIZE;
		    /* pkey(retval); */
		    keep_going = FALSE;
		}
		break;
	    case EXIT_ON_KEY_OR_BUTTON_PRESS:
		if (incount != 0 || click_button != NO_CLICK) {
		    if (click_button != NO_CLICK) {	/* button press */
			/* click values are already set */
			retval = 0;
		    } else {				/* key press */
			/* get first pressed key */
			--incount;
			retval = inbuf[inptr];
			inptr = (inptr+1) % INBUF_SIZE;
			/* pkey(retval); */
		    }
		    keep_going = FALSE;
		}
		break;
	    default:
		panic("x_event: unknown exit condition %d\n", exit_condition);
		break;
	}
    } while (keep_going);

    /* Restore globals */
    exit_x_event = hold_exit_x_event;

    return retval;
}

/*winmap.c*/

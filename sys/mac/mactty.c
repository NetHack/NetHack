/* NetHack 3.6	mactty.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) Jon W{tte 1993.					*/
/* NetHack may be freely redistributed.  See license for details.	*/

/*
 * mactty.c
 *
 * This file contains the actual code for the tty library. For a
 * description, see the file mactty.h, which contains all you
 * need to know to use the library.
 */

#include "hack.h" /* to get flags */
#include "mttypriv.h"
#if !TARGET_API_MAC_CARBON
#include <Resources.h>
#endif

char game_active = 0; /* flag to window rendering routines not to use ppat */

/* these declarations are here because I can't include macwin.h without
 * including the world */
extern void dprintf(char *, ...); /* dprintf.c */

/*
 * Borrowed from the Mac tty port
 */
extern WindowPtr _mt_window;

static void select_onscreen_window(tty_record *record);
static void select_offscreen_port(tty_record *record);

#define MEMORY_MARGIN 30000

/*
 * Convenience macro for most functions - put last in declaration
 */
#define RECORD_EXISTS(record)                                     \
    tty_record *record;                                           \
    if (!window || !(record = (tty_record *) GetWRefCon(window))) \
        return general_failure;

/*
 * Simple macro for deciding whether we draw at once or delay
 */
#define DRAW_DIRECT (TA_ALWAYS_REFRESH & record->attribute[TTY_ATTRIB_FLAGS])

/*
 * Table of special characters. Zero is ALWAYS special; it means
 * end of string and would be MISSED if it was not included here.
 */
#define COOKED_CONTROLS 0X00002581
#define RAW_CONTROLS 1
static unsigned long s_control = COOKED_CONTROLS;

/*
 * Memory-related error
 */
static short
mem_err(void)
{
    short ret_val = MemError();
    if (!ret_val) {
        ret_val = general_failure;
    }
    return ret_val;
}

/*
 * Make a rectangle empty
 */
static void
empty_rect(Rect *r)
{
    r->right = -20000;
    r->left = 20000;
    r->top = 20000;
    r->bottom = -20000;
}

/*
 * Union twp rect together
 */
static void
union_rect(Rect *r1, Rect *r2, Rect *dest)
{
    dest->left = min(r1->left, r2->left);
    dest->top = min(r1->top, r2->top);
    dest->bottom = max(r1->bottom, r2->bottom);
    dest->right = max(r1->right, r2->right);
}

/*
 * Dispose a pointer using the set memory-allocator
 */
static short
dispose_ptr(void *ptr)
{
    if (!ptr) {
        return noErr; /* Silently accept disposing nulls */
    }
    DisposePtr(ptr);
    return MemError();
}

#if 0 /* Use alloc.c instead */
/*
 * Allocate a pointer using the set memory-allocator
 */
static short
alloc_ptr (void **ptr, long size) {
	*ptr = NewPtr (size);
	return MemError ();
}
#endif

/*
 * Set up a GWorld in the record
 */
static short
allocate_offscreen_world(tty_record *record)
{
    GWorldPtr gw = (GWorldPtr) 0;
    GWorldFlags world_flags = 0;
    long mem_here, mem_there, other, required_mem;
    Point p = { 0, 0 };
    Rect r_screen;
    GDHandle gdh;
    short s_err;

    select_onscreen_window(record);
    LocalToGlobal(&p);
    r_screen = record->its_bits.bounds;
    OffsetRect(&r_screen, p.h, p.v);

    gdh = GetMaxDevice(&r_screen);
    required_mem = (long) (*((*gdh)->gdPMap))->pixelSize
                       * ((long) record->its_bits.bounds.right
                          * record->its_bits.bounds.bottom)
                   >> 3;

    PurgeSpace(&other, &mem_here);
    if (other < mem_here + MEMORY_MARGIN) {
        mem_here = other - MEMORY_MARGIN;
    }
    dprintf("Heap %ld Required %ld", mem_here, required_mem);
    if (required_mem > mem_here) {
        mem_there = required_mem;
        if (required_mem > TempMaxMem(&mem_there)) {
            dprintf("No memory");
            return memFullErr;
        }
        world_flags |= useTempMem;
    }
    s_err = NewGWorld(&gw, 0, &r_screen, (CTabHandle) 0, (GDHandle) 0,
                      world_flags);
    if (!s_err) {
        record->offscreen_world = gw;
        select_offscreen_port(record);
        SetOrigin(0, 0);
        select_onscreen_window(record);
        dprintf("New GWorld @ %lx;dm", gw);
    }
    return s_err;
}

/*
 * Done with GWorld, release data
 */
static short
deallocate_gworld(tty_record *record)
{
    if (record->offscreen_world) {
        DisposeGWorld(record->offscreen_world);
        record->offscreen_world = (GWorldPtr) 0;
    }
    return noErr;
}

/*
 * Get rid of offscreen bitmap
 */
static short
free_bits(tty_record *record)
{
    short s_err;

    if (record->uses_gworld) {
        s_err = deallocate_gworld(record);
#if !TARGET_API_MAC_CARBON
    } else {
        s_err = dispose_ptr(record->its_bits.baseAddr);
        if (!s_err) {
            record->its_bits.baseAddr = (char *) 0;
            if (record->offscreen_port) {
                ClosePort(record->offscreen_port);
                s_err = dispose_ptr(record->offscreen_port);
                if (!s_err) {
                    record->offscreen_port = (GrafPtr) 0;
                }
            }
        }
#endif
    }
    return s_err;
}

/*
 * Snatch a window from the resource fork. Create the record.
 * Otherwise, do nothing.
 */

short
create_tty(WindowRef *window, short resource_id, Boolean in_color)
{
    tty_record *record;
    Boolean was_allocated = !!*window;

    if (in_color) {
        *window = GetNewCWindow(resource_id, (Ptr) *window, (WindowRef) -1L);
    } else {
        *window = GetNewWindow(resource_id, (Ptr) *window, (WindowRef) -1L);
    }
    if (!*window) {
        return mem_err();
    }

    record = (tty_record *) NewPtrClear(sizeof(tty_record));
    if (!record) {
#if !TARGET_API_MAC_CARBON
        if (was_allocated) {
            CloseWindow(*window);
        } else {
#endif
            DisposeWindow(*window);
#if !TARGET_API_MAC_CARBON
        }
#endif
        return mem_err();
    }
    record->its_window = *window;
    SetWRefCon(*window, (long) record);
    record->was_allocated = was_allocated;
    record->its_bits.baseAddr = (char *) 0;
    record->curs_state = TRUE;

    /*
     * We need to keep the window world around if we switch worlds
     */
    record->offscreen_world = (GWorldPtr) 0;
    record->uses_gworld = in_color;
    if (in_color) {
        GDHandle gh;

        SetPortWindowPort(*window);
        GetGWorld(&(record->its_window_world), &gh);
    } else {
        record->its_window_world = (GWorldPtr) 0;
    }

#if CLIP_RECT_ONLY
    empty_rect(&(record->invalid_rect));
#else
    record->invalid_part = NewRgn();
    if (!record->invalid_part) {
        return destroy_tty(*window);
    }
#endif

    return noErr;
}

short
init_tty_number(WindowPtr window, short font_number, short font_size,
                short x_size, short y_size)
{
    RECORD_EXISTS(record);

    record->font_number = font_number;
    record->font_size = font_size;
    record->x_size = x_size;
    record->y_size = y_size;

    return force_tty_coordinate_system_recalc(window);
}

/*
 * Done with a window - destroy it. Release the memory only if
 * it wasn't allocated when we got it!
 */
short
destroy_tty(WindowPtr window)
{
    short s_err;
    RECORD_EXISTS(record);

    s_err = free_bits(record);
    if (!s_err) {
#if !TARGET_API_MAC_CARBON
        if (record->was_allocated) {
            CloseWindow(window);
        } else {
#endif
            DisposeWindow(window);
#if !TARGET_API_MAC_CARBON
        }
#endif
        s_err = dispose_ptr(record);
    }

    return s_err;
}

static void
do_set_port_font(tty_record *record)
{
    PenNormal();
    TextFont(record->font_number);
    TextSize(record->font_size);
    if (0L != (record->attribute[TTY_ATTRIB_FLAGS] & TA_OVERSTRIKE)) {
        TextMode(srcOr);
    } else {
        TextMode(srcCopy);
    }
}

/*
 * Fill in some fields from some other fields that may have changed
 */
static void
calc_font_sizes(tty_record *record)
{
    FontInfo font_info;

    do_set_port_font(record);

    GetFontInfo(&font_info);
    record->char_width = font_info.widMax;
    record->ascent_height = font_info.ascent + font_info.leading;
    record->row_height = record->ascent_height + font_info.descent;
}

/*
 * Allocate memory for the bitmap holding the tty window
 */
static short
alloc_bits(tty_record *record)
{
    short s_err;

    SetRect(&record->its_bits.bounds, 0, 0,
            record->char_width * record->x_size,
            record->row_height * record->y_size);

    /*
     * Clear two highest and lowest bit - not a color pixMap, and even in size
     */
    record->its_bits.rowBytes =
        ((record->its_bits.bounds.right + 15) >> 3) & 0x1ffe;

    if (record->uses_gworld) {
        s_err = allocate_offscreen_world(record);
#if !TARGET_API_MAC_CARBON
    } else {
        s_err = alloc_ptr((void **) &(record->its_bits.baseAddr),
                          record->its_bits.rowBytes
                              * record->its_bits.bounds.bottom);
        if (!s_err) {
            s_err = alloc_ptr((void **) &(record->offscreen_port),
                              sizeof(GrafPort));
        }
        if (!s_err) {
            OpenPort(record->offscreen_port);
            SetPort(record->offscreen_port);
            ClipRect(&(record->its_bits.bounds));
            SetPortBits(&(record->its_bits));
        }
#endif
    }
    return s_err;
}

/*
 * Save the current port/world in a safe place for later retrieval
 */
static void
save_port(tty_record *record, void *save)
{
    GWorldPtr gw;
    GDHandle gh;
    GrafPtr gp;

    if (record->uses_gworld) {
        GetGWorld(&gw, &gh);
        *(GWorldPtr *) save = gw;
    } else {
        GetPort(&gp);
        *(GrafPtr *) save = gp;
    }
}

/*
 * Restore current port/world after a save
 */
static void
use_port(tty_record *record, void *port)
{
    if (record->uses_gworld) {
        PixMapHandle pix_map;

        SetGWorld((GWorldPtr) port, (GDHandle) 0);
        pix_map = GetGWorldPixMap(record->offscreen_world);
        if (pix_map) {
            if (port == record->offscreen_world)
                LockPixels(pix_map);
            else
                UnlockPixels(pix_map);
        }
    } else {
        SetPort((GrafPtr) port);
    }
}

/*
 * Use offscreen drawing - lock the pixels through use_port
 */
static void
select_offscreen_port(tty_record *record)
{
    if (record->uses_gworld) {
        use_port(record, record->offscreen_world);
    } else {
        use_port(record, record->offscreen_port);
    }
}

/*
 * Use the window - unlock pixels
 */
static void
select_onscreen_window(tty_record *record)
{
    if (record->uses_gworld) {
        use_port(record, record->its_window_world);
        SetPortWindowPort(record->its_window);
    } else {
        use_port(record, record->its_window);
    }
}

/*
 * Do bits copy depending on if we're using color or not
 */
static void
copy_bits(tty_record *record, Rect *bounds, short xfer_mode,
          RgnHandle mask_rgn)
{
    GWorldFlags pix_state;
    BitMap *source;

    if (record->uses_gworld) {
        pix_state = GetPixelsState(GetGWorldPixMap(record->offscreen_world));
        LockPixels(GetGWorldPixMap(record->offscreen_world));
        source = (BitMapPtr) *GetGWorldPixMap(record->offscreen_world);
    } else
        source = &record->its_bits;

    SetPortWindowPort(record->its_window);
    CopyBits(source,
             GetPortBitMapForCopyBits(GetWindowPort(record->its_window)),
             bounds, bounds, xfer_mode, mask_rgn);

    if (record->uses_gworld) {
        SetPixelsState(GetGWorldPixMap(record->offscreen_world), pix_state);
    }
}

/*
 * Fill an area with the background color
 */
static void
erase_rect(tty_record *record, Rect *area)
{
    if (game_active && u.uhp > 0 && iflags.use_stone
        && record->its_window == _mt_window) {
        PixPatHandle ppat;

        ppat = GetPixPat(iflags.use_stone + 127); /* find which pat to get */
        if (ppat) { /* in game window, using backgroung pattern, and have
                       pattern */
            FillCRect(area, ppat);
            DisposePixPat(ppat);
            return;
        }
    }
    EraseRect(area);
}

/*
 * Recalculate the window based on new size, font, extent values,
 * and re-allocate the bitmap.
 */
short
force_tty_coordinate_system_recalc(WindowPtr window)
{
    short s_err;
    RECORD_EXISTS(record);

    s_err = free_bits(record);
    if (s_err) {
        return s_err;
    }
    calc_font_sizes(record);

    s_err = alloc_bits(record);
    if (s_err) {
        /*
         * Catastrophe! We could not allocate memory for the bitmap! Things
         * may go very
         * much downhill from here!
         */
        dprintf("alloc_bits returned null in "
                "force_tty_coordinate_system_recalc!");
        return s_err;
    }
    select_offscreen_port(record);
    do_set_port_font(record);
    return clear_tty(window);
}

#if 0
/*
 * Update TTY according to new color environment for the window
 */
static short
tty_environment_changed (tty_record *record) {
Point p = {0, 0};
Rect r_screen;

	if (record->uses_gworld) {
		r_screen = record->its_bits.bounds;
		LocalToGlobal (&p);
		OffsetRect (&r_screen, p.h, p.v);
		UpdateGWorld (&(record->offscreen_world), 0, &r_screen,
			(CTabHandle) 0, (GDHandle) 0, stretchPix);
		select_offscreen_port (record);
		SetOrigin (0, 0);
		select_onscreen_window (record);
	}
	return 0;
}
#endif

/*
 * Read a lot of interesting and useful information from the current tty
 */
short
get_tty_metrics(WindowPtr window, short *x_size, short *y_size,
                short *x_size_pixels, short *y_size_pixels,
                short *font_number, short *font_size, short *char_width,
                short *row_height)
{
    RECORD_EXISTS(record);

    /*
     * First, test that we actually have something to draw to...
     */
    if ((((char *) 0 == record->its_bits.baseAddr) && !record->uses_gworld)
        || (((GWorldPtr) 0 == record->offscreen_world)
            && record->uses_gworld)) {
        return general_failure;
    }

    *x_size = record->x_size;
    *y_size = record->y_size;
    *x_size_pixels = record->its_bits.bounds.right;
    *y_size_pixels = record->its_bits.bounds.bottom;
    *font_number = record->font_number;
    *font_size = record->font_size;
    *char_width = record->char_width;
    *row_height = record->row_height;

    return noErr;
}

/*
 * Map a position on the map to screen coordinates
 */
static void
pos_rect(tty_record *record, Rect *r, short x_pos, short y_pos, short x_end,
         short y_end)
{
    SetRect(r, x_pos * (record->char_width), y_pos * (record->row_height),
            (1 + x_end) * (record->char_width),
            (1 + y_end) * (record->row_height));
}

static void
accumulate_rect(tty_record *record, Rect *rect)
{
#if CLIP_RECT_ONLY
    union_rect(rect, &(record->invalid_rect), &(record->invalid_rect));
#else
    RgnHandle rh = NewRgn();

    RectRgn(rh, rect);
    UnionRgn(record->invalid_part, rh, record->invalid_part);
    DisposeRgn(rh);
#endif
}

/*
 * get and set window invalid region.  exposed for HandleUpdateEvent in
 * macwin.c
 * to correct display problem
 */

short
get_invalid_region(WindowPtr window, Rect *inval_rect)
{
    RECORD_EXISTS(record);
#if CLIP_RECT_ONLY
    if (record->invalid_rect.right <= record->invalid_rect.left
        || record->invalid_rect.bottom <= record->invalid_rect.top) {
        return general_failure;
    }
    *inval_rect = record->invalid_rect;
#else
    if (EmptyRgn(record->invalid_part)) {
        return general_failure;
    }
    *inval_rect = (*(record->invalid_part))->rgnBBox;
#endif
    return noErr;
}

short
set_invalid_region(WindowPtr window, Rect *inval_rect)
{
    RECORD_EXISTS(record);
    accumulate_rect(record, inval_rect);
    return noErr;
}

/*
 * Invert the specified position
 */
static void
curs_pos(tty_record *record, short x_pos, short y_pos, short to_state)
{
    Rect r;

    if (record->curs_state == to_state) {
        return;
    }
    record->curs_state = to_state;
    pos_rect(record, &r, x_pos, y_pos, x_pos, y_pos);

    if (DRAW_DIRECT) {
        void *old_port;

        save_port(record, &old_port);
        select_onscreen_window(record);
        InvertRect(&r);
        use_port(record, old_port);
    } else {
        accumulate_rect(record, &r);
    }
}

/*
 * Move the cursor (both as displayed and where drawing goes)
 * HOWEVER: The cursor is NOT stored in the bitmap!
 */
short
move_tty_cursor(WindowPtr window, short x_pos, short y_pos)
{
    RECORD_EXISTS(record);

    if (record->x_curs == x_pos && record->y_curs == y_pos) {
        return noErr;
    }
    if (record->x_size <= x_pos || x_pos < 0 || record->y_size <= y_pos
        || y_pos < 0) {
        return general_failure;
    }
    curs_pos(record, record->x_curs, record->y_curs, 0);
    record->x_curs = x_pos;
    record->y_curs = y_pos;
    curs_pos(record, x_pos, y_pos, 1);

    return noErr;
}

/*
 * Update the screen to match the current bitmap, after adding stuff
 * with add_tty_char etc.
 */
short
update_tty(WindowPtr window)
{
    Rect r;
    RECORD_EXISTS(record);

#if CLIP_RECT_ONLY
    if (record->invalid_rect.right <= record->invalid_rect.left
        || record->invalid_rect.bottom <= record->invalid_rect.top) {
        return noErr;
    }
    r = record->invalid_rect;
#else
    if (EmptyRgn(record->invalid_part)) {
        return noErr;
    }
    r = (*(record->invalid_part))->rgnBBox;
#endif
    select_onscreen_window(record);
    copy_bits(record, &r, srcCopy, (RgnHandle) 0);
#if CLIP_RECT_ONLY
    empty_rect(&(record->invalid_rect));
#else
    SetEmptyRgn(record->invalid_part);
#endif
    if (record->curs_state) {
        pos_rect(record, &r, record->x_curs, record->y_curs, record->x_curs,
                 record->y_curs);
        InvertRect(&r);
    }

    return noErr;
}

/*
 * Low level add to screen
 */
static void
do_add_string(tty_record *record, char *str, short len)
{
    Rect r;

    if (len < 1) {
        return;
    }
    select_offscreen_port(record);

    MoveTo(record->x_curs * record->char_width,
           record->y_curs * record->row_height + record->ascent_height);
    DrawText(str, 0, len);

    pos_rect(record, &r, record->x_curs, record->y_curs,
             record->x_curs + len - 1, record->y_curs);
    select_onscreen_window(record);
    if (DRAW_DIRECT) {
        copy_bits(record, &r, srcCopy, (RgnHandle) 0);
    } else {
        accumulate_rect(record, &r);
    }
}

/*
 * Low-level cursor handling routine
 */
static void
do_add_cursor(tty_record *record, short x_pos)
{
    record->x_curs = x_pos;
    if (record->x_curs >= record->x_size) {
        if (0L != (record->attribute[TTY_ATTRIB_FLAGS] & TA_WRAP_AROUND)) {
            record->y_curs++;
            record->x_curs = 0;
            if (record->y_curs >= record->y_size) {
                if (0L != (record->attribute[TTY_ATTRIB_FLAGS]
                           & TA_INHIBIT_VERT_SCROLL)) {
                    record->y_curs = record->y_size;
                } else {
                    scroll_tty(record->its_window, 0,
                               1 + record->y_curs - record->y_size);
                }
            }
        } else {
            record->x_curs = record->x_size;
        }
    }
}

/*
 * Do control character
 */
static void
do_control(tty_record *record, short character)
{
    int recurse = 0;

    /*
     * Check recursion because nl_add_cr and cr_add_nl may both be set and
     * invoke each other
     */
    do {
        switch (character) {
        case CHAR_CR:
            record->x_curs = 0;
            if (!recurse
                && (record->attribute[TTY_ATTRIB_CURSOR] & TA_CR_ADD_NL)) {
                recurse = 1;
            } else {
                recurse = 0;
                break;
            } /* FALL-THROUGH: if CR-LF, don't bother with loop */
        case CHAR_LF:
            record->y_curs++;
            if (record->y_curs >= record->y_size) {
                scroll_tty(record->its_window, 0,
                           1 + record->y_curs - record->y_size);
            }
            if (!recurse
                && (record->attribute[TTY_ATTRIB_CURSOR] & TA_NL_ADD_CR)) {
                character = CHAR_CR;
                recurse = 1;
            } else
                recurse = 0;
            break;
        case CHAR_BELL:
            tty_nhbell();
            break;
        case CHAR_BS:
            if (record->x_curs > 0)
                record->x_curs--;
        default:
            break;
        }
    } while (recurse);
}

/*
 * Add a single character. It is drawn directly if the correct flag is set,
 * else deferred to the next update event or call of update_tty()
 */
short
add_tty_char(WindowPtr window, short character)
{
    register char is_control;
    char ch;
    RECORD_EXISTS(record);

    if (!(record->attribute[TTY_ATTRIB_FLAGS] & TA_WRAP_AROUND)
        && record->x_curs >= record->x_size)
        return noErr; /* Optimize away drawing across border without wrap */

    if (record->curs_state != 0)
        curs_pos(record, record->x_curs, record->y_curs, 0);

    ch = character;
    is_control = (ch < sizeof(long) * 8) && ((s_control & (1 << ch)) != 0L);
    if (is_control)
        do_control(record, ch);
    else {
        do_add_string(record, (char *) &ch, 1);
        do_add_cursor(record, record->x_curs + 1);
    }

    return noErr;
}

/*
 * Add a null-terminated string of characters
 */
short
add_tty_string(WindowPtr window, const char *string)
{
    register const unsigned char *start_c;
    register const unsigned char *the_c;
    register unsigned char ch, is_control = 0, tty_wrap;
    register short max_x, pos_x;
    RECORD_EXISTS(record);

    if (record->curs_state != 0)
        curs_pos(record, record->x_curs, record->y_curs, 0);

    the_c = (const unsigned char *) string;
    max_x = record->x_size;
    tty_wrap = (record->attribute[TTY_ATTRIB_FLAGS] & TA_WRAP_AROUND);
    for (;;) {
        pos_x = record->x_curs;
        if (!tty_wrap && pos_x >= max_x)
            break; /* Optimize away drawing across border without wrap */

        start_c = the_c;
        ch = *the_c;
        while (pos_x < max_x) {
            is_control =
                (ch < sizeof(long) * 8) && ((s_control & (1 << ch)) != 0L);
            if (is_control)
                break;
            the_c++;
            ch = *the_c;
            pos_x++;
        }
        do_add_string(record, (char *) start_c, the_c - start_c);
        do_add_cursor(record, pos_x);
        if (!ch)
            break;

        if (is_control) {
            do_control(record, ch);
            the_c++;
        }
    }

    return noErr;
}

/*
 * Read or change attributes for the tty. Note that some attribs may
 * very well clear and reallocate the bitmap when changed, whereas
 * others (color, highlight, ...) are guaranteed not to.
 */
short
get_tty_attrib(WindowPtr window, tty_attrib attrib, long *value)
{
    RECORD_EXISTS(record);

    if (attrib < 0 || attrib >= TTY_NUMBER_ATTRIBUTES) {
        return general_failure;
    }
    *value = record->attribute[attrib];

    return noErr;
}

short
set_tty_attrib(WindowPtr window, tty_attrib attrib, long value)
{
    RGBColor rgb_color;
    RECORD_EXISTS(record);

    if (attrib < 0 || attrib >= TTY_NUMBER_ATTRIBUTES) {
        return general_failure;
    }
    record->attribute[attrib] = value;
    /*
     * Presently, no attributes generate a new bitmap.
     */
    switch (attrib) {
    case TTY_ATTRIB_CURSOR:
        /*
         * Check if we should change tables
         */
        if (0L != (value & TA_RAW_OUTPUT)) {
            s_control = RAW_CONTROLS;
        } else {
            s_control = COOKED_CONTROLS;
        }
        break;
    case TTY_ATTRIB_FLAGS:
        /*
         * Check if we should flush the output going from cached to
         * draw-direct
         */
        if (0L != (value & TA_ALWAYS_REFRESH)) {
            update_tty(window);
        }
        break;
    case TTY_ATTRIB_FOREGROUND:
        /*
         * Set foreground color
         */
        TA_TO_RGB(value, rgb_color);
        select_offscreen_port(record);
        RGBForeColor(&rgb_color);
        select_onscreen_window(record);
        break;
    case TTY_ATTRIB_BACKGROUND:
        /*
         * Set background color
         */
        TA_TO_RGB(value, rgb_color);
        select_offscreen_port(record);
        RGBBackColor(&rgb_color);
        select_onscreen_window(record);
        break;
    default:
        break;
    }
    return noErr;
}

/*
 * Scroll the window. Positive is up/left. scroll_tty ( window, 0, 1 ) is a
 * line feed.
 * Scroll flushes the accumulated update area by calling update_tty().
 */

short
scroll_tty(WindowPtr window, short delta_x, short delta_y)
{
    RgnHandle rgn;
    short s_err;
    RECORD_EXISTS(record);

    s_err = update_tty(window);

    rgn = NewRgn();

    select_offscreen_port(record);
    ScrollRect(&(record->its_bits.bounds), -delta_x * record->char_width,
               -delta_y * record->row_height, rgn);
    EraseRgn(rgn);
    SetEmptyRgn(rgn);

    select_onscreen_window(record);
    ScrollRect(&(record->its_bits.bounds), -delta_x * record->char_width,
               -delta_y * record->row_height, rgn);
    EraseRgn(rgn);
    DisposeRgn(rgn);

    record->y_curs -= delta_y;
    record->x_curs -= delta_x;

    return noErr;
}

/*
 * Clear the screen. Immediate.
 */
short
clear_tty(WindowPtr window)
{
    RECORD_EXISTS(record);

    record->curs_state = 0;
    select_offscreen_port(record);
    erase_rect(record, &(record->its_bits.bounds));
    accumulate_rect(record, &(record->its_bits.bounds));
    update_tty(window);

    return noErr;
}

/*
 * Blink cursor on window if necessary
 */
short
blink_cursor(WindowPtr window, long when)
{
    RECORD_EXISTS(record);

    if ((record->attribute[TTY_ATTRIB_CURSOR] & TA_BLINKING_CURSOR)) {
        if (when > record->last_cursor + GetCaretTime()) {
            curs_pos(record, record->x_curs, record->y_curs,
                     !record->curs_state);
            record->last_cursor = when;
            update_tty(window);
        }
    }
    return 0;
}

/*
 * Draw an image of the tty - used for update events and can be called
 * for screen dumps.
 */
short
image_tty(EventRecord *theEvent, WindowPtr window)
{
#if defined(__SC__) || defined(__MRC__)
#pragma unused(theEvent)
#endif
    RECORD_EXISTS(record);

#if CLIP_RECT_ONLY
    record->invalid_rect = record->its_bits.bounds;
#else
    RgnHandle rh = NewRgn();

    RectRgn(rh, record->its_bits.bounds);
    UnionRgn(record->invalid_part, rh, record->invalid_part);
    DisposeRgn(rh);
#endif
    return update_tty(window);
}

/*
 * Clear an area
 */
short
clear_tty_window(WindowPtr window, short from_x, short from_y, short to_x,
                 short to_y)
{
    Rect r;
    RECORD_EXISTS(record);

    if (from_x > to_x || from_y > to_y) {
        return general_failure;
    }
    pos_rect(record, &r, from_x, from_y, to_x, to_y);
    select_offscreen_port(record);
    erase_rect(record, &r);
    accumulate_rect(record, &r);
    if (DRAW_DIRECT) {
        update_tty(window);
    } else
        select_onscreen_window(record);
    return noErr;
}

#if EXTENDED_SUPPORT
/*
 * Delete or insert operations used by many terminals can bottleneck through
 * here. Note that the order of executin for row/colum insertions is NOT
 * specified. Negative values for num_ mean delete, zero means no effect.
 */
short
mangle_tty_rows_columns(WindowPtr window, short from_row, short num_rows,
                        short from_column, short num_columns)
{
    Rect r;
    RgnHandle rh = NewRgn();
    RECORD_EXISTS(record);

    update_tty(window); /* Always make sure screen is OK */
    curs_pos(record, record->x_curs, record->y_curs, 0);

    if (num_rows) {
        pos_rect(record, &r, 0, from_row, record->x_size - 1,
                 record->y_size - 1);
        select_offscreen_port(record);
        ScrollRect(&r, 0, num_rows * record->row_height, rh);
        EraseRgn(rh);
        SetEmptyRgn(rh);
        select_onscreen_window(record);
        ScrollRect(&r, 0, num_rows * record->row_height, rh);
        EraseRgn(rh);
        SetEmptyRgn(rh);
    }
    if (num_columns) {
        pos_rect(record, &r, from_column, 0, record->x_size - 1,
                 record->y_size - 1);
        select_offscreen_port(record);
        ScrollRect(&r, num_columns * record->char_width, 0, rh);
        EraseRgn(rh);
        SetEmptyRgn(rh);
        select_onscreen_window(record);
        ScrollRect(&r, num_columns * record->char_width, 0, rh);
        EraseRgn(rh);
        SetEmptyRgn(rh);
    }
    DisposeRgn(rh);
    if (record->x_curs >= from_column) {
        record->x_curs += num_columns;
    }
    if (record->y_curs >= from_row) {
        record->y_curs += num_rows;
    }
    curs_pos(record, record->x_curs, record->y_curs, 1);

    return noErr;
}

/*
 * Frame an area in an aesthetically pleasing way.
 */
short
frame_tty_window(WindowPtr window, short from_x, short from_y, short to_x,
                 short to_y, short frame_fatness)
{
    Rect r;
    RECORD_EXISTS(record);

    if (from_x > to_x || from_y > to_y) {
        return general_failure;
    }
    pos_rect(record, &r, from_x, from_y, to_x, to_y);
    select_offscreen_port(record);
    PenSize(frame_fatness, frame_fatness);
    FrameRect(&r);
    PenNormal();
    accumulate_rect(record, &r);
    if (DRAW_DIRECT) {
        update_tty(window);
    } else
        select_onscreen_window(record);
}

/*
 * Highlighting a specific part of the tty window
 */
short
invert_tty_window(WindowPtr window, short from_x, short from_y, short to_x,
                  short to_y)
{
    Rect r;
    RECORD_EXISTS(record);

    if (from_x > to_x || from_y > to_y) {
        return general_failure;
    }
    pos_rect(record, &r, from_x, from_y, to_x, to_y);
    select_offscreen_port(record);
    InvertRect(&r);
    accumulate_rect(record, &r);
    if (DRAW_DIRECT) {
        update_tty(window);
    } else
        select_onscreen_window(record);
}

static void
canonical_rect(Rect *r, short x1, short y1, short x2, short y2)
{
    if (x1 < x2) {
        if (y1 < y2) {
            SetRect(r, x1, x2, y1, y2);
        } else {
            SetRect(r, x1, x2, y2, y1);
        }
    } else {
        if (y1 < y2) {
            SetRect(r, x2, x1, y1, y2);
        } else {
            SetRect(r, x2, x1, y2, y1);
        }
    }
}

/*
 * Line drawing - very device dependent
 */
short
draw_tty_line(WindowPtr window, short from_x, short from_y, short to_x,
              short to_y)
{
    Rect r;
    RECORD_EXISTS(record);

    select_offscreen_port(record);
    MoveTo(from_x, from_y);
    LineTo(to_x, to_y);
    canonical_rect(&r, from_x, from_y, to_x, to_y);
    accumulate_rect(record, &r);
    if (DRAW_DIRECT) {
        update_tty(window);
    } else
        select_onscreen_window(record);
}

#endif /* EXTENDED_SUPPORT */

/* NetHack 5.0	mactty.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) Jon W{tte 1993.					*/
/* NetHack may be freely redistributed.  See license for details.	*/

/*
 * mactty.c
 *
 * Implementation of the tty library; see mactty.h for the interface.
 */

#include "hack.h" /* to get flags */
#include "mttypriv.h"
#include "maccompat.h" /* P_STRING_CONV */
#include <Sound.h>
#include <Resources.h>

/* declared here because macwin.h cannot be included without pulling in the world */
extern void mac_dprintf(char *, ...); /* dprintf.c */

extern WindowPtr _mt_window;

static void select_onscreen_window(tty_record *record);
static short force_tty_coordinate_system_recalc(WindowPtr window);
static short scroll_tty(WindowPtr window, short delta_x, short delta_y);
static void select_offscreen_port(tty_record *record);

#define MEMORY_MARGIN 30000

/* fetch the tty_record from the window; put last in declaration */
#define RECORD_EXISTS(record)                                     \
    tty_record *record;                                           \
    if (!window || !(record = (tty_record *) GetWRefCon(window))) \
        return general_failure;

/* true when we draw immediately rather than deferring to update_tty() */
#define DRAW_DIRECT (TA_ALWAYS_REFRESH & record->attribute[TTY_ATTRIB_FLAGS])

/* control-char bitmask; bit 0 (NUL) must be set so it terminates strings */
#define COOKED_CONTROLS 0X00002581
static const unsigned long s_control = COOKED_CONTROLS;

static short
mem_err(void)
{
    short ret_val = MemError();
    if (!ret_val) {
        ret_val = general_failure;
    }
    return ret_val;
}

/* make a rectangle empty (inverted bounds) */
static void
empty_rect(Rect *r)
{
    r->right = -20000;
    r->left = 20000;
    r->top = 20000;
    r->bottom = -20000;
}

static void
union_rect(Rect *r1, Rect *r2, Rect *dest)
{
    dest->left = min(r1->left, r2->left);
    dest->top = min(r1->top, r2->top);
    dest->bottom = max(r1->bottom, r2->bottom);
    dest->right = max(r1->right, r2->right);
}

static short
dispose_ptr(void *ptr)
{
    if (!ptr) {
        return noErr; /* silently accept disposing nulls */
    }
    DisposePtr(ptr);
    return MemError();
}

static short
alloc_ptr(void **ptr, long size)
{
    *ptr = NewPtr(size);
    return MemError();
}

/* set up the offscreen GWorld in the record */
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
    if (required_mem > mem_here) {
        mem_there = required_mem;
        if (required_mem > TempMaxMem(&mem_there)) {
            mac_dprintf("No memory");
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
    }
    return s_err;
}

static short
deallocate_gworld(tty_record *record)
{
    if (record->offscreen_world) {
        DisposeGWorld(record->offscreen_world);
        record->offscreen_world = (GWorldPtr) 0;
    }
    return noErr;
}

/* release the offscreen bitmap or GWorld */
static short
free_bits(tty_record *record)
{
    short s_err;

    if (record->uses_gworld) {
        s_err = deallocate_gworld(record);
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
    }
    return s_err;
}

/* load a window from the resource fork and create its tty_record */
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
        if (was_allocated) {
            CloseWindow(*window);
        } else {
            DisposeWindow(*window);
            /* don't leave *window dangling: the caller's B&W retry
               would treat it as pre-allocated window storage and
               build a WindowRecord inside the freed block */
            *window = (WindowRef) 0;
        }
        return mem_err();
    }
    record->its_window = *window;
    SetWRefCon(*window, (long) record);
    record->its_bits.baseAddr = (char *) 0;
    record->curs_state = TRUE;

    /* keep the window's world around so we can switch back to it */
    record->offscreen_world = (GWorldPtr) 0;
    record->uses_gworld = in_color;
    if (in_color) {
        GDHandle gh;

        SetPortWindowPort(*window);
        GetGWorld(&(record->its_window_world), &gh);
    } else {
        record->its_window_world = (GWorldPtr) 0;
    }

    empty_rect(&(record->invalid_rect));

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

static void
do_set_port_font(tty_record *record)
{
    PenNormal();
    TextFont(record->font_number);
    TextSize(record->font_size);
    TextMode(srcCopy);
}

void
tty_nhbell(void)
{
    Handle h = GetNamedResource('snd ', P_STRING_CONV("Bell"));

    if (h) {
        HLock(h);
        SndPlay((SndChannelPtr) 0, (SndListHandle) h, 0); /* async=0: synchronous */
        HUnlock(h);
        ReleaseResource(h);
    } else
        SysBeep(30);
}

/* recompute char_width/row_height from the current font */
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

/* allocate the offscreen bitmap holding the tty window */
static short
alloc_bits(tty_record *record)
{
    short s_err;

    SetRect(&record->its_bits.bounds, 0, 0,
            record->char_width * record->x_size,
            record->row_height * record->y_size);

    /* mask off high bits (flag a non-color pixMap) and force even rowBytes */
    record->its_bits.rowBytes =
        ((record->its_bits.bounds.right + 15) >> 3) & 0x1ffe;

    if (record->uses_gworld) {
        s_err = allocate_offscreen_world(record);
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
    }
    return s_err;
}

/* save the current port/world for later restore via use_port() */
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

/* switch to a port/world, locking pixels when entering the offscreen world */
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

/* switch drawing to the offscreen port */
static void
select_offscreen_port(tty_record *record)
{
    if (record->uses_gworld) {
        use_port(record, record->offscreen_world);
    } else {
        use_port(record, record->offscreen_port);
    }
}

/* switch drawing to the onscreen window */
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

/* copy the offscreen bits to the window (handles color vs B&W) */
static void
copy_bits(tty_record *record, Rect *bounds, short xfer_mode,
          RgnHandle mask_rgn)
{
    GWorldFlags pix_state = 0; /* set+used only under uses_gworld; init quiets gcc */
    BitMap *source;

    if (record->uses_gworld) {
        pix_state = GetPixelsState(GetGWorldPixMap(record->offscreen_world));
        LockPixels(GetGWorldPixMap(record->offscreen_world));
        source = (BitMap *) *GetGWorldPixMap(record->offscreen_world);
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

/* fill an area with the background color */
static void
erase_rect(tty_record *record UNUSED, Rect *area)
{
    EraseRect(area);
}

/* recalc window metrics for new size/font and re-allocate the bitmap */
static short
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
        /* could not allocate the bitmap; the game cannot recover from here */
        mac_dprintf("alloc_bits returned null in "
                "force_tty_coordinate_system_recalc!");
        return s_err;
    }
    select_offscreen_port(record);
    do_set_port_font(record);
    return clear_tty(window);
}

/* read metrics (size, font, char dimensions) from the current tty */
short
get_tty_metrics(WindowPtr window, short *x_size, short *y_size,
                short *x_size_pixels, short *y_size_pixels,
                short *font_number, short *font_size, short *char_width,
                short *row_height)
{
    RECORD_EXISTS(record);

    /* fail if there is nothing to draw to yet */
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

/* map a character cell range to a pixel rectangle */
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
    union_rect(rect, &(record->invalid_rect), &(record->invalid_rect));
}

/* get/set the window's invalid region; used by HandleUpdateEvent in macwin.c */
short
get_invalid_region(WindowPtr window, Rect *inval_rect)
{
    RECORD_EXISTS(record);
    if (record->invalid_rect.right <= record->invalid_rect.left
        || record->invalid_rect.bottom <= record->invalid_rect.top) {
        return general_failure;
    }
    *inval_rect = record->invalid_rect;
    return noErr;
}

short
set_invalid_region(WindowPtr window, Rect *inval_rect)
{
    RECORD_EXISTS(record);
    accumulate_rect(record, inval_rect);
    return noErr;
}

/* Invert the cell at (x_pos, y_pos) to show/hide the cursor.

   Invariant: the cursor is an ONSCREEN-ONLY overlay; it is never drawn into
   the offscreen bitmap.  Every full blit (update_tty/image_tty) paints the
   un-inverted cell from the bitmap and then re-inverts at the cursor
   position, so the two stay consistent.  Keep it that way -- inverting the
   offscreen copy as well would double-invert on the next blit. */
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

/* move the cursor; note the cursor is not stored in the bitmap */
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

/* copy the accumulated invalid region from the bitmap to the screen */
short
update_tty(WindowPtr window)
{
    Rect r;
    RECORD_EXISTS(record);

    if (record->invalid_rect.right <= record->invalid_rect.left
        || record->invalid_rect.bottom <= record->invalid_rect.top) {
        return noErr;
    }
    r = record->invalid_rect;
    select_onscreen_window(record);
    copy_bits(record, &r, srcCopy, (RgnHandle) 0);
    empty_rect(&(record->invalid_rect));
    if (record->curs_state) {
        pos_rect(record, &r, record->x_curs, record->y_curs, record->x_curs,
                 record->y_curs);
        InvertRect(&r);
    }

    return noErr;
}

/* draw a run of characters at the cursor (no control-char handling) */
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

/* advance the cursor, wrapping/scrolling per the active flags */
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
                    /* pin to the last valid row (y_size itself would be one
                       row past the bitmap and draw out of bounds) */
                    record->y_curs = record->y_size - 1;
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

/* handle a control character (CR, LF, BEL, BS) */
static void
do_control(tty_record *record, short character)
{
    switch (character) {
    case CHAR_LF:
        record->y_curs++;
        if (record->y_curs >= record->y_size) {
            scroll_tty(record->its_window, 0,
                       1 + record->y_curs - record->y_size);
        }
        if (!(record->attribute[TTY_ATTRIB_CURSOR] & TA_NL_ADD_CR))
            break;
        /* FALL-THROUGH: NL-add-CR returns the cursor to column 0 */
    case CHAR_CR:
        record->x_curs = 0;
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
}

/* add a single character; drawn directly or deferred per DRAW_DIRECT */
short
add_tty_char(WindowPtr window, short character)
{
    int is_control;
    unsigned char ch;
    RECORD_EXISTS(record);

    if (!(record->attribute[TTY_ATTRIB_FLAGS] & TA_WRAP_AROUND)
        && record->x_curs >= record->x_size)
        return noErr; /* nothing to draw past the right edge without wrap */

    if (record->curs_state != 0)
        curs_pos(record, record->x_curs, record->y_curs, 0);

    ch = (unsigned char) character;
    is_control = (ch < sizeof(long) * 8) && ((s_control & (1UL << ch)) != 0L);
    if (is_control)
        do_control(record, ch);
    else {
        do_add_string(record, (char *) &ch, 1);
        do_add_cursor(record, record->x_curs + 1);
    }

    return noErr;
}

/* add a null-terminated string */
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
            break; /* nothing to draw past the right edge without wrap */

        start_c = the_c;
        ch = *the_c;
        while (pos_x < max_x) {
            is_control =
                (ch < sizeof(long) * 8) && ((s_control & (1UL << ch)) != 0L);
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

/* read a tty attribute */
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
    switch (attrib) {
    case TTY_ATTRIB_FLAGS:
        /* flush pending output when switching to draw-direct */
        if (0L != (value & TA_ALWAYS_REFRESH)) {
            update_tty(window);
        }
        break;
    case TTY_ATTRIB_FOREGROUND:
        TA_TO_RGB(value, rgb_color);
        select_offscreen_port(record);
        RGBForeColor(&rgb_color);
        select_onscreen_window(record);
        break;
    case TTY_ATTRIB_BACKGROUND:
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

/* scroll the window (positive = up/left); flushes pending output first */
static short
scroll_tty(WindowPtr window, short delta_x, short delta_y)
{
    RgnHandle rgn;
    RECORD_EXISTS(record);

    (void) update_tty(window); /* flush pending output first */

    rgn = NewRgn();
    if (!rgn)
        return general_failure;

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

/* clear the screen immediately */
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

/* toggle the cursor if the blink interval has elapsed */
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

/* redraw the whole tty; used for update events and screen dumps */
short
image_tty(EventRecord *theEvent, WindowPtr window)
{
    RECORD_EXISTS(record);

    record->invalid_rect = record->its_bits.bounds;
    return update_tty(window);
}

/* clear a rectangular area of cells */
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

/* NetHack 3.6	mactty.h	$NHDT-Date: 1447755970 2015/11/17 10:26:10 $  $NHDT-Branch: master $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Jon W{tte 1993.                                        */
/* NetHack may be freely redistributed.  See license for details.       */

/*
 * This header is the supported external interface for the "tty" window
 * package. This package sports care-free handling of "dumb" tty windows
 * (preferably using monospaced fonts) - it does NOT remember the strings
 * sent to it; rather, it uses an offscreen bitmap.
 *
 * For best performance, make sure it is aligned on a 32-pixel boundary
 * (or at least a 16-pixel one) in black & white. For 24bit color,
 * alignment doesn't matter, and for 8-bit color, alignment to every
 * fourth pixel is most efficient.
 *
 * (c) Copyright 1993 Jon W{tte
 */

/*
 * You should really not poke in the structures used by the tty window.
 * However, since it uses the wRefCon of windows (by calling GetWRefCon
 * and SetWRefCon) you lose that possibility. If you still want to store
 * information about a window, the FIRST location _pointed to_ by the
 * wRefCon will be a void * that you can use for whatever reasons. Don't
 * take the address of this variable and expect it to stay the same
 * across calls to the tty window.
 *
 * void * my_config_ptr = * ( void * * ) GetWRefCon ( tty_window ) ;
 */

/*
 * The library uses the window's port temporarily through SetPortBits;
 * that means you shouldn't do any funky things to the clipping region
 * etc. Actually, you shouldn't even resize the window, as that will clip
 * new drawing.
 *
 * Also, if you use this library under Pascal, remember that the string
 * passed to add_tty_string() is a "C" style string with NO length byte,
 * and a terminating zero byte at the end instead.
 */

#ifndef _H_tty_public
#define _H_tty_public
#undef red /* undef internal color const strings from decl */
#undef green
#undef blue
#if 1 /*!TARGET_API_MAC_CARBON*/
#include <windows.h>
#endif

/*
 * Error code returned when it's probably our fault, or
 * bad parameters.
 */
#define general_failure 1

/*
 * Base resource id's for window types
 */
#define WIN_BASE_RES 128
#define WIN_BASE_KIND 128

/*
 * Commonly used characters
 */
#define CHAR_ENTER ((char) 3)
#define CHAR_BELL ((char) 7)
#define CHAR_BS ((char) 8)
#define CHAR_LF ((char) 10)
#define CHAR_CR ((char) 13)
#define CHAR_ESC ((char) 27)
#define CHAR_BLANK ((char) 32)
#define CHAR_DELETE ((char) 127)

extern char game_active; /* flag to window rendering routines
                            not to use ppat */
/*
 * If you want some fancy operations that not a normal TTY device normally
 * supports, use EXTENDED_SUPPORT. For frames, area erases and area scrolls,
 * plus bitmap graphics - RESOLUTION DEPENDENT, be sure to call
 * get_tty_metrics and use those limits.
 */
#define EXTENDED_SUPPORT 0
/*
 * if you print a lot of single characters, accumulating each one in a
 * clipping region will take too much time. Instead, define this, which
 * will clip in rects.
 */
#define CLIP_RECT_ONLY 1

typedef enum tty_attrib {

    /*
     * Flags relating to the general functioning of the window.
     * These flags are passed at create_tty time, and changing them
     * later will clear the screen.
     */
    TTY_ATTRIB_FLAGS,
/*
 * When using proportional fonts, this will place each character
 * separately, ensuring aligned columns (but looking ugly and taking
 * time)
 */
#define TA_MOVE_EACH_CHAR 1L
/*
 * This means draw each change as it occurs instead of collecting the area
 * and draw it all at once at update_tty() - slower, but more reliable.
 */
#define TA_ALWAYS_REFRESH 2L
/*
 * When reaching the right end, we either just stop drawing, or wrap to the
 * next line.
 */
#define TA_WRAP_AROUND 4L
/*
 * Overstrike means that characters are added on top of each other; i e don't
 * clear the letter beneath. This is faster, using srcOr under QuickDraw
 */
#define TA_OVERSTRIKE 8L
/*
 * We may want the window not to scroll when we reach the end line,
 * but stop drawing instead.
 */
#define TA_INHIBIT_VERT_SCROLL 16L

    /*
     * Foreground and background colors. These only affect characters
     * drawn by subsequent calls; not what's already there (but it does
     * affect clears)
     * On b/w screens these do nothing.
     */
    TTY_ATTRIB_FOREGROUND,
    TTY_ATTRIB_BACKGROUND,
#define TA_RGB_TO_TTY(r)                       \
    ((((long) ((r).red >> 8) & 0xff) << 16)    \
     + (((long) ((r).green >> 8) & 0xff) << 8) \
     + ((long) ((r).blue >> 8) & 0xff))

    /*
     * Attributes relating to the cursor, and character set mappings
     */
    TTY_ATTRIB_CURSOR,
/*
 * Blinking cursor is more noticeable when it's idle
 */
#define TA_BLINKING_CURSOR 1L
/*
 * When handling input, do we echo characters as they are typed?
 */
#define TA_ECHO_INPUT 2L
/*
 * Do we return each character code separately, or map delete etc? Note
 * that non-raw input means getchar won't return anything until the user
 * has typed a return.
 */
#define TA_RAW_INPUT 4L
/*
 * Do we print every character as it is (including BS, NL and CR!) or do
 * do we interpret characters such as NL, BS and CR?
 */
#define TA_RAW_OUTPUT 8L
/*
 * When getting a NL, do we also move to the left?
 */
#define TA_NL_ADD_CR 16L
/*
 * When getting a CR, do we also move down?
 */
#define TA_CR_ADD_NL 32L
/*
 * Wait for input or return what we've got?
 */
#define TA_NONBLOCKING_IO 64L

/*
 * Use this macro to cast a function pointer to a tty attribute; this will
 * help portability to systems where a function pointer doesn't fit in a long
 */
#define TA_ATTRIB_FUNC(x) ((long) (x))

    /*
     * This symbolic constant is used to check the number of attributes
     */
    TTY_NUMBER_ATTRIBUTES

} tty_attrib;

/*
 * Character returned by end-of-file condition
 */
#define TTY_EOF -1

/*
 * Create the window according to a resource WIND template.
 * The window pointer pointed to by window should be NULL to
 * allocate the window record dynamically, or point to a
 * WindowRecord structure already allocated.
 *
 * Passing in_color means you have to be sure there's color support;
 * on the Mac, this means 32bit QuickDraw present, else it will
 * crash. Not passing in_color means everything's rendered in
 * black & white.
 */
extern short create_tty(WindowPtr *window, short resource_id,
                        Boolean in_color);

/*
 * Use init_tty_name or init_tty_number to initialize a window
 * once allocated by create_tty. Size parameters are in characters.
 */

extern short init_tty_number(WindowPtr window, short font_number,
                             short font_size, short x_size, short y_size);

/*
 * Close and deallocate a window and its data
 */
extern short destroy_tty(WindowPtr window);

/*
 * Change the font and font size used in the window for drawing after
 * the calls are made. To change the coordinate system, be sure to call
 * force_tty_coordinate_system_recalc() - else it may look strange if
 * the new font doesn't match the old one.
 */
extern short set_tty_font_name(winid window_type, char *name);
extern short force_tty_coordinate_system_recalc(WindowPtr window);

/*
 * Getting some metrics about the tty and its drawing.
 */
extern short get_tty_metrics(WindowPtr window, short *x_size, short *y_size,
                             short *x_size_pixels, short *y_size_pixels,
                             short *font_number, short *font_size,
                             short *char_width, short *row_height);

/*
 * The basic move cursor function. 0,0 is topleft.
 */
extern short move_tty_cursor(WindowPtr window, short x_pos, short y_pos);

/*
 * Flush all changes done to a tty to the screen (see TA_ALWAYS_UPDATE above)
 */
extern short update_tty(WindowPtr window);

/*
 * Add a character to the tty and update the cursor position
 */
extern short add_tty_char(WindowPtr window, short character);

/*
 * Add a string of characters to the tty and update the cursor
 * position. The string is 0-terminated!
 */
extern short add_tty_string(WindowPtr window, const char *string);

/*
 * Change or read an attribute of the tty. Note that some attribute changes
 * may clear the screen. See the above enum and defines for values.
 * Attributes can be both function pointers and special flag values.
 */
extern short get_tty_attrib(WindowPtr window, tty_attrib attrib, long *value);
extern short set_tty_attrib(WindowPtr window, tty_attrib attrib, long value);

/*
 * Scroll the actual TTY image, in characters, positive means up/left
 * scroll_tty ( my_tty , 0 , 1 ) means a linefeed. Is always carried out
 * directly, regardless of the wait-update setting. Does updates before
 * scrolling.
 */
extern short scroll_tty(WindowPtr window, short delta_x, short delta_y);

/*
 * Erase the offscreen bitmap and move the cursor to 0,0. Re-init some window
 * values (font etc) Is always carried out directly on-screen, regardless of
 * the wait-for-update setting. Clears update area.
 */
extern short clear_tty(WindowPtr window);

/*
 * Call this routine with a window (always _mt_window) and a time (usually
 * from most recent event) to determine if cursor in window should be blinked
 */
extern short blink_cursor(WindowPtr window, long when);

/*
 * For screen dumps, open the printer port and call this function. Can be used
 * for clipboard as well (only for a PICT, though; this library doesn't
 * concern
 * itself with characters, just bitmaps)
 */
extern short image_tty(EventRecord *theEvent, WindowPtr window);

/*
 * For erasing just an area of characters
 */
extern short clear_tty_window(WindowPtr window, short from_row,
                              short from_col, short to_row, short to_col);

/*
 * get and set the invalid region of the main window
 */
extern short get_invalid_region(WindowPtr window, Rect *inval_rect);
extern short set_invalid_region(WindowPtr window, Rect *inval_rect);

/*
 * Now in macsnd.c, which seemed like a good place
 */
extern void tty_nhbell();

#if EXTENDED_SUPPORT

/*
 * Various versions of delete character/s, insert line/s etc can be handled by
 * this general-purpose function. Negative num_ means delete, positive means
 * insert, and you can never be sure which of row and col operations come
 * first
 * if you specify both...
 */
extern short mangle_tty_rows_columns(WindowPtr window, short from_row,
                                     short num_rows, short from_col,
                                     short num_cols);

/*
 * For framing an area without using grahpics characters.
 * Note that the given limits are those used for framing, you should not
 * draw in them. frame_fatness should typically be 1-5, and may be clipped
 * if it is too large.
 */
extern short frame_tty_window(WindowPtr window, short from_row,
                              short from_col, short to_row, short to_col,
                              short frame_fatness);

/*
 * For inverting specific characters after the fact. May look funny in color.
 */
extern short invert_tty_window(WindowPtr window, short from_row,
                               short from_col, short to_row, short to_col);

/*
 * For drawing lines on the tty - VERY DEVICE DEPENDENT. Use get_tty_metrics.
 */
extern short draw_tty_line(WindowPtr window, short from_x, short from_y,
                           short to_x, short to_y);

#endif /* EXTENDED_SUPPORT */

#endif /* _H_tty_public */

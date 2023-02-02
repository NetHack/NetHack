/* NetHack 3.6	mhcolor.c	$NHDT-Date: 1432512802 2015/05/25 00:13:22 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

/* color management and such */

#include "winMS.h"
#include "mhcolor.h"

#define TOTAL_BRUSHES 10
#define NHBRUSH_CODE(win, type) ((((win) &0xFF) << 8) | ((type) &0xFF))

struct t_brush_table {
    int code;
    HBRUSH brush;
    COLORREF color;
};
static struct t_brush_table brush_table[TOTAL_BRUSHES];
static int max_brush = 0;

static struct t_brush_table default_brush_table[] = {
    { NHBRUSH_CODE(NHW_STATUS, MSWIN_COLOR_FG), NULL, RGB(0, 0, 0) },
    { NHBRUSH_CODE(NHW_MESSAGE, MSWIN_COLOR_FG), NULL, RGB(0, 0, 0) },
    { NHBRUSH_CODE(NHW_STATUS, MSWIN_COLOR_FG), NULL, RGB(0, 0, 0) },
    { NHBRUSH_CODE(NHW_TEXT, MSWIN_COLOR_FG), NULL, RGB(0, 0, 0) },
    { NHBRUSH_CODE(NHW_KEYPAD, MSWIN_COLOR_FG), NULL, RGB(0, 0, 0) },
    { NHBRUSH_CODE(NHW_MAP, MSWIN_COLOR_FG), NULL, RGB(96, 96, 96) },

    { NHBRUSH_CODE(NHW_MENU, MSWIN_COLOR_BG), NULL, RGB(255, 255, 255) },
    { NHBRUSH_CODE(NHW_MESSAGE, MSWIN_COLOR_BG), NULL, RGB(192, 192, 192) },
    { NHBRUSH_CODE(NHW_STATUS, MSWIN_COLOR_BG), NULL, RGB(192, 192, 192) },
    { NHBRUSH_CODE(NHW_TEXT, MSWIN_COLOR_BG), NULL, RGB(255, 255, 255) },
    { NHBRUSH_CODE(NHW_KEYPAD, MSWIN_COLOR_BG), NULL, RGB(255, 255, 255) },
    { NHBRUSH_CODE(NHW_MAP, MSWIN_COLOR_BG), NULL, RGB(192, 192, 192) },
    { -1, NULL, RGB(0, 0, 0) }
};

static void mswin_color_from_string(char *colorstring, HBRUSH *brushptr,
                                    COLORREF *colorptr);

typedef struct ctv {
    const char *colorstring;
    COLORREF colorvalue;
} color_table_value;

/*
 * The color list here is a combination of:
 * NetHack colors.  (See mhmap.c)
 * HTML colors. (See http://www.w3.org/TR/REC-html40/types.html#h-6.5 )
 */

static color_table_value color_table[] = {
    /* NetHack colors */
    { "black", RGB(0x55, 0x55, 0x55) },
    { "red", RGB(0xFF, 0x00, 0x00) },
    { "green", RGB(0x00, 0x80, 0x00) },
    { "brown", RGB(0xA5, 0x2A, 0x2A) },
    { "blue", RGB(0x00, 0x00, 0xFF) },
    { "magenta", RGB(0xFF, 0x00, 0xFF) },
    { "cyan", RGB(0x00, 0xFF, 0xFF) },
    { "orange", RGB(0xFF, 0xA5, 0x00) },
    { "brightgreen", RGB(0x00, 0xFF, 0x00) },
    { "yellow", RGB(0xFF, 0xFF, 0x00) },
    { "brightblue", RGB(0x00, 0xC0, 0xFF) },
    { "brightmagenta", RGB(0xFF, 0x80, 0xFF) },
    { "brightcyan", RGB(0x80, 0xFF, 0xFF) },
    { "white", RGB(0xFF, 0xFF, 0xFF) },
    /* Remaining HTML colors */
    { "trueblack", RGB(0x00, 0x00, 0x00) },
    { "gray", RGB(0x80, 0x80, 0x80) },
    { "grey", RGB(0x80, 0x80, 0x80) },
    { "purple", RGB(0x80, 0x00, 0x80) },
    { "silver", RGB(0xC0, 0xC0, 0xC0) },
    { "maroon", RGB(0x80, 0x00, 0x00) },
    { "fuchsia", RGB(0xFF, 0x00, 0xFF) }, /* = NetHack magenta */
    { "lime", RGB(0x00, 0xFF, 0x00) },    /* = NetHack bright green */
    { "olive", RGB(0x80, 0x80, 0x00) },
    { "navy", RGB(0x00, 0x00, 0x80) },
    { "teal", RGB(0x00, 0x80, 0x80) },
    { "aqua", RGB(0x00, 0xFF, 0xFF) }, /* = NetHack cyan */
    { "", RGB(0x00, 0x00, 0x00) },
};

typedef struct ctbv {
    char *colorstring;
    int syscolorvalue;
} color_table_brush_value;

static color_table_brush_value color_table_brush[] = {
    { "activeborder", COLOR_ACTIVEBORDER },
    { "activecaption", COLOR_ACTIVECAPTION },
    { "appworkspace", COLOR_APPWORKSPACE },
    { "background", COLOR_BACKGROUND },
    { "btnface", COLOR_BTNFACE },
    { "btnshadow", COLOR_BTNSHADOW },
    { "btntext", COLOR_BTNTEXT },
    { "captiontext", COLOR_CAPTIONTEXT },
    { "graytext", COLOR_GRAYTEXT },
    { "greytext", COLOR_GRAYTEXT },
    { "highlight", COLOR_HIGHLIGHT },
    { "highlighttext", COLOR_HIGHLIGHTTEXT },
    { "inactiveborder", COLOR_INACTIVEBORDER },
    { "inactivecaption", COLOR_INACTIVECAPTION },
    { "menu", COLOR_MENU },
    { "menutext", COLOR_MENUTEXT },
    { "scrollbar", COLOR_SCROLLBAR },
    { "window", COLOR_WINDOW },
    { "windowframe", COLOR_WINDOWFRAME },
    { "windowtext", COLOR_WINDOWTEXT },
    { "", -1 },
};

void
mswin_init_color_table()
{
    int i;
    struct t_brush_table *p;

    /* cleanup */
    for (i = 0; i < max_brush; i++)
        DeleteObject(brush_table[i].brush);
    max_brush = 0;

/* initialize brush table */
#define BRUSHTABLE_ENTRY(opt, win, type)                          \
    brush_table[max_brush].code = NHBRUSH_CODE((win), (type));    \
    mswin_color_from_string((opt), &brush_table[max_brush].brush, \
                            &brush_table[max_brush].color);       \
    max_brush++;

    BRUSHTABLE_ENTRY(iflags.wc_foregrnd_menu, NHW_MENU, MSWIN_COLOR_FG);
    BRUSHTABLE_ENTRY(iflags.wc_foregrnd_message, NHW_MESSAGE, MSWIN_COLOR_FG);
    BRUSHTABLE_ENTRY(iflags.wc_foregrnd_status, NHW_STATUS, MSWIN_COLOR_FG);
    BRUSHTABLE_ENTRY(iflags.wc_foregrnd_text, NHW_TEXT, MSWIN_COLOR_FG);
    BRUSHTABLE_ENTRY(iflags.wc_foregrnd_message, NHW_KEYPAD, MSWIN_COLOR_FG);

    BRUSHTABLE_ENTRY(iflags.wc_backgrnd_menu, NHW_MENU, MSWIN_COLOR_BG);
    BRUSHTABLE_ENTRY(iflags.wc_backgrnd_message, NHW_MESSAGE, MSWIN_COLOR_BG);
    BRUSHTABLE_ENTRY(iflags.wc_backgrnd_status, NHW_STATUS, MSWIN_COLOR_BG);
    BRUSHTABLE_ENTRY(iflags.wc_backgrnd_text, NHW_TEXT, MSWIN_COLOR_BG);
    BRUSHTABLE_ENTRY(iflags.wc_backgrnd_message, NHW_KEYPAD, MSWIN_COLOR_BG);
#undef BRUSHTABLE_ENTRY

    /* go through the values and fill in "blanks" (use default values) */
    for (i = 0; i < max_brush; i++) {
        if (!brush_table[i].brush) {
            for (p = default_brush_table; p->code != -1; p++) {
                if (p->code == brush_table[i].code) {
                    brush_table[i].brush = CreateSolidBrush(p->color);
                    brush_table[i].color = p->color;
                }
            }
        }
    }
}

HBRUSH
mswin_get_brush(int win_type, int color_index)
{
    int i;
    for (i = 0; i < max_brush; i++)
        if (brush_table[i].code == NHBRUSH_CODE(win_type, color_index))
            return brush_table[i].brush;
    return NULL;
}

COLORREF
mswin_get_color(int win_type, int color_index)
{
    int i;
    for (i = 0; i < max_brush; i++)
        if (brush_table[i].code == NHBRUSH_CODE(win_type, color_index))
            return brush_table[i].color;
    return RGB(0, 0, 0);
}

static void
mswin_color_from_string(char *colorstring, HBRUSH *brushptr,
                        COLORREF *colorptr)
{
    color_table_value *ctv_ptr = color_table;
    color_table_brush_value *ctbv_ptr = color_table_brush;
    int red_value, blue_value, green_value;
    static char *hexadecimals = "0123456789abcdef";

    *brushptr = NULL;
    *colorptr = RGB(0, 0, 0);

    if (colorstring == NULL)
        return;
    if (*colorstring == '#') {
        if (strlen(++colorstring) != 6)
            return;

        red_value =
            strchr(hexadecimals, tolower(*colorstring++)) - hexadecimals;
        red_value *= 16;
        red_value +=
            strchr(hexadecimals, tolower(*colorstring++)) - hexadecimals;

        green_value =
            strchr(hexadecimals, tolower(*colorstring++)) - hexadecimals;
        green_value *= 16;
        green_value +=
            strchr(hexadecimals, tolower(*colorstring++)) - hexadecimals;

        blue_value =
            strchr(hexadecimals, tolower(*colorstring++)) - hexadecimals;
        blue_value *= 16;
        blue_value +=
            strchr(hexadecimals, tolower(*colorstring++)) - hexadecimals;

        *colorptr = RGB(red_value, blue_value, green_value);
    } else {
        while (*ctv_ptr->colorstring
               && _stricmp(ctv_ptr->colorstring, colorstring))
            ++ctv_ptr;
        if (*ctv_ptr->colorstring) {
            *colorptr = ctv_ptr->colorvalue;
        } else {
            while (*ctbv_ptr->colorstring
                   && _stricmp(ctbv_ptr->colorstring, colorstring))
                ++ctbv_ptr;
            if (*ctbv_ptr->colorstring) {
                *brushptr = SYSCLR_TO_BRUSH(ctbv_ptr->syscolorvalue);
                *colorptr = GetSysColor(ctbv_ptr->syscolorvalue);
            }
        }
    }
    if (max_brush > TOTAL_BRUSHES)
        panic("Too many colors!");
    *brushptr = CreateSolidBrush(*colorptr);
}

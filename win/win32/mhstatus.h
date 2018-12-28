/* NetHack 3.6	mhstatus.h	$NHDT-Date: 1432512812 2015/05/25 00:13:32 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINStatusWindow_h
#define MSWINStatusWindow_h

#include "winMS.h"
#include "config.h"
#include "global.h"

#define NHSW_LINES 2

static const int fieldorder1[] = { BL_TITLE, BL_STR, BL_DX,    BL_CO,    BL_IN,
                             BL_WI,    BL_CH,  BL_ALIGN, BL_SCORE, -1 };
static const int fieldorder2[] = { BL_LEVELDESC, BL_GOLD,      BL_HP,   BL_HPMAX,
                             BL_ENE,       BL_ENEMAX,    BL_AC,   BL_XP,
                             BL_EXP,       BL_HD,        BL_TIME, BL_HUNGER,
                             BL_CAP,       BL_CONDITION, -1 };
static const int *fieldorders[] = { fieldorder1, fieldorder2, NULL };
static const int fieldcounts[NHSW_LINES] = { SIZE(fieldorder1) - 1, SIZE(fieldorder2) - 1};

#define MSWIN_MAX_LINE1_STRINGS (SIZE(fieldorder1) - 1)
#define MSWIN_MAX_LINE2_STRINGS (SIZE(fieldorder2) - 1 + BL_MASK_BITS)
#define MSWIN_MAX_LINE_STRINGS (MSWIN_MAX_LINE1_STRINGS > MSWIN_MAX_LINE2_STRINGS ? \
                                MSWIN_MAX_LINE1_STRINGS : MSWIN_MAX_LINE2_STRINGS)

#define MSWIN_LINE1_FIELDS (SIZE(fieldorder1) - 1)
#define MSWIN_LINE2_FIELDS (SIZE(fieldorder2) - 1)
#define MSWIN_MAX_LINE_FIELDS (MSWIN_LINE1_FIELDS > MSWIN_LINE2_FIELDS ? \
                               MSWIN_LINE1_FIELDS : MSWIN_LINE2_FIELDS)

/* when status hilites are enabled, we use an array of mswin_status_strings
 * to represent what needs to be rendered. */
typedef struct mswin_status_string  {
    const char * str; /* ascii string to be displayed */
    boolean space_in_front; /* render with a space in front of string */
    int color; /* string text color index */
    int attribute; /* string text attributes */
    boolean draw_bar; /* draw a percentage bar  */
    int bar_percent; /* a percentage to indicate */
    int bar_color; /* color index of percentage bar */
    int bar_attribute; /* attributes of percentage bar */
} mswin_status_string;

typedef struct mswin_status_strings
{
    int count;
    mswin_status_string * status_strings[MSWIN_MAX_LINE_STRINGS];
} mswin_status_strings;

typedef struct mswin_status_field {
    int field_index; // field index
    boolean enabled; // whether the field is enabled
    const char * name; // name of status field
    const char * format; // format of field
    boolean space_in_front; // add a space in front of the field

    int percent;
    int color;
    int attribute;
    char string[BUFSZ];

} mswin_status_field;

typedef struct mswin_condition_field {
    int mask;
    const char * name;
    int bit_position;
} mswin_condition_field;

typedef struct mswin_status_fields {
    int count;
    mswin_status_field * status_fields[MSWIN_MAX_LINE_FIELDS];
} mswin_status_fields;

typedef struct mswin_status_line {
    mswin_status_strings status_strings;
    mswin_status_fields status_fields;
} mswin_status_line;

typedef struct mswin_status_lines {
    mswin_status_line lines[NHSW_LINES]; /* number of strings to be rendered on each line */
} mswin_status_lines;

HWND mswin_init_status_window(void);
void mswin_status_window_size(HWND hWnd, LPSIZE sz);

#endif /* MSWINStatusWindow_h */

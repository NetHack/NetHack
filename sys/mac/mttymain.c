/* NetHack 3.6	mttymain.c	$NHDT-Date: 1554215928 2019/04/02 14:38:48 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.13 $ */
/* Copyright (c) Jon W{tte, 1993					*/
/* NetHack may be freely redistributed.  See license for details.	*/

#include "hack.h"
#include "macwin.h"
#include "mttypriv.h"
#include "mactty.h"
#include "wintty.h"

#if !TARGET_API_MAC_CARBON
#include <Palettes.h>
#endif
#include <Gestalt.h>

#define MT_WINDOW 135
#define MT_WIDTH 80
#define MT_HEIGHT 24

/*
 * Names:
 *
 * Statics are prefixed _
 * Mac-tty becomes mt_
 */

static long _mt_attrs[5][2] = {
    { 0x000000, 0xffffff }, /* Normal */
    { 0xff8080, 0xffffff }, /* Underline */
    { 0x40c020, 0xe0e0e0 }, /* Bold */
    { 0x003030, 0xff0060 }, /* Blink */
    { 0xff8888, 0x000000 }, /* Inverse */
};

static char _attrs_inverse[5] = {
    0, 0, 0, 0, 0,
};

/* see color.h */

static long _mt_colors[CLR_MAX][2] = {
    { 0x000000, 0x808080 }, /* Black */
    { 0x880000, 0xffffff }, /* Red */
    { 0x008800, 0xffffff }, /* Green */
    { 0x553300, 0xffffff }, /* Brown */
    { 0x000088, 0xffffff }, /* Blue */
    { 0x880088, 0xffffff }, /* Magenta */
    { 0x008888, 0xffffff }, /* Cyan */
    { 0x888888, 0xffffff }, /* Gray */
    { 0x000000, 0xffffff }, /* No Color */
    { 0xff4400, 0xffffff }, /* Orange */
    { 0x00ff00, 0xffffff }, /* Bright Green */
    { 0xffff00, 0x606060 }, /* Yellow */
    { 0x0033ff, 0xffffff }, /* Bright Blue */
    { 0xff00ff, 0xffffff }, /* Bright Magenta */
    { 0x00ffff, 0xffffff }, /* Bright Cyan */
    { 0xffffff, 0x505050 }, /* White */
};

static char _colors_inverse[CLR_MAX] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

#ifdef CHANGE_COLOR

#define POWER_LIMIT 22
#define SECONDARY_POWER_LIMIT 16
#define CHANNEL_LIMIT 14
#define SECONDARY_CHANNEL_LIMIT 12

void
tty_change_color(int color, long rgb, int reverse)
{
    long inverse, working_rgb = rgb;
    int total_power = 0, max_channel = 0;
    int cnt = 3;

    working_rgb >>= 4;
    while (cnt-- > 0) {
        total_power += working_rgb & 0xf;
        max_channel = max(max_channel, working_rgb & 0xf);
        working_rgb >>= 8;
    }

    if (total_power >= POWER_LIMIT
        || (total_power >= SECONDARY_POWER_LIMIT
            && max_channel >= SECONDARY_CHANNEL_LIMIT)
        || max_channel >= CHANNEL_LIMIT)
        inverse = 0x000000;
    else
        inverse = 0xffffff;

    if (reverse) {
        working_rgb = rgb;
        rgb = inverse;
        inverse = working_rgb;
    }

    if (color >= CLR_MAX) {
        if (color - CLR_MAX >= 5)
            impossible("Changing too many colors");
        else {
            _mt_attrs[color - CLR_MAX][0] = rgb;
            _mt_attrs[color - CLR_MAX][1] = inverse;
            _attrs_inverse[color - CLR_MAX] = reverse;
        }
    } else if (color >= 0) {
        _mt_colors[color][0] = rgb;
        _mt_colors[color][1] = inverse;
        _colors_inverse[color] = reverse;
    } else
        impossible("Changing negative color");
}

void
tty_change_background(int white_or_black)
{
    register int i;

    for (i = 0; i < CLR_MAX; i++) {
        if (white_or_black)
            _mt_colors[i][1] = 0xffffff; /* white */
        else
            _mt_colors[i][1] = 0x000000; /* black */
    }

    /* special cases */
    if (white_or_black) {
        _mt_colors[CLR_BLACK][1] =
            0x808080; /* differentiate black from no color */
        _mt_colors[CLR_WHITE][1] =
            0x505050; /* highlight white with grey background */
        _mt_colors[CLR_YELLOW][1] =
            0x606060; /* highlight yellow with grey background */
        _mt_colors[CLR_BLUE][0] = 0x000088; /* make pure blue */
        _mt_colors[NO_COLOR][0] = 0x000000; /* make no_color black on white */
        _mt_attrs[0][0] = 0x000000;         /* "normal" is black on white */
        _mt_attrs[0][1] = 0xffffff;
    } else {
        _mt_colors[NO_COLOR][0] = 0xffffff; /* make no_color white on black */
        _mt_colors[CLR_BLACK][1] =
            0x808080; /* differentiate black from no color */
        _mt_colors[CLR_BLUE][0] =
            0x222288; /* lighten blue - it's too dark on black */
        _mt_attrs[0][0] = 0xffffff; /* "normal" is white on black */
        _mt_attrs[0][1] = 0x000000;
    }
}

char *
tty_get_color_string(void)
{
    char *ptr;
    int count;
    static char color_buf[5 * (CLR_MAX + 5) + 1];

    color_buf[0] = 0;
    ptr = color_buf;

    for (count = 0; count < CLR_MAX; count++) {
        int flag = _colors_inverse[count] ? 1 : 0;

        sprintf(ptr, "%s%s%x%x%x", count ? "/" : "", flag ? "-" : "",
                (int) (_mt_colors[count][flag] >> 20) & 0xf,
                (int) (_mt_colors[count][flag] >> 12) & 0xf,
                (int) (_mt_colors[count][flag] >> 4) & 0xf);
        ptr += strlen(ptr);
    }
    for (count = 0; count < 5; count++) {
        int flag = _attrs_inverse[count] ? 1 : 0;

        sprintf(ptr, "/%s%x%x%x", flag ? "-" : "",
                (int) (_mt_attrs[count][flag] >> 20) & 0xf,
                (int) (_mt_attrs[count][flag] >> 12) & 0xf,
                (int) (_mt_attrs[count][flag] >> 4) & 0xf);
        ptr += strlen(ptr);
    }

    return color_buf;
}
#endif

extern struct DisplayDesc *ttyDisplay; /* the tty display descriptor */

char kill_char = CHAR_ESC;
char erase_char = CHAR_BS;

WindowRef _mt_window = (WindowRef) 0;
static Boolean _mt_in_color = 0;
extern short win_fonts[NHW_TEXT + 1];

static void
_mt_init_stuff(void)
{
    long resp, flag;
    short num_cols, num_rows, win_width, win_height, font_num, font_size;
    short char_width, row_height;
    short hor, vert;

    LI = MT_HEIGHT;
    CO = MT_WIDTH;

    if (!strcmp(windowprocs.name, "mac")) {
        dprintf("Mac Windows");
        LI -= 1;
    } else {
        dprintf("TTY Windows");
    }

    /*
     * If there is at least one screen CAPABLE of color, and if
     * 32-bit QD is there, we use color. 32-bit QD is needed for the
     * offscreen GWorld
     */
    if (!Gestalt(gestaltQuickdrawVersion, &resp) && resp > 0x1ff) {
        GDHandle gdh = GetDeviceList();
        while (gdh) {
            if (TestDeviceAttribute(gdh, screenDevice)) {
                if (HasDepth(gdh, 4, 1, 1) || HasDepth(gdh, 8, 1, 1)
                    || HasDepth(gdh, 16, 1, 1) || HasDepth(gdh, 32, 1, 1)) {
                    _mt_in_color = 1;
                    break;
                }
            }
            gdh = GetNextDevice(gdh);
        }
    }

    if (create_tty(&_mt_window, WIN_BASE_KIND + NHW_MAP, _mt_in_color)
        != noErr)
        error("_mt_init_stuff: Couldn't create tty.");
    SetWindowKind(_mt_window, WIN_BASE_KIND + NHW_MAP);
    SelectWindow(_mt_window);
    SetPortWindowPort(_mt_window);
    SetOrigin(-1, -1);

    font_size = iflags.wc_fontsiz_map
                    ? iflags.wc_fontsiz_map
                    : (iflags.large_font && !small_screen) ? 12 : 9;
    if (init_tty_number(_mt_window, win_fonts[NHW_MAP], font_size, CO, LI)
        != noErr)
        error("_mt_init_stuff: Couldn't init tty.");

    if (get_tty_metrics(_mt_window, &num_cols, &num_rows, &win_width,
                        &win_height, &font_num, &font_size, &char_width,
                        &row_height))
        error("_mt_init_stuff: Couldn't get tty metrics.");

    SizeWindow(_mt_window, win_width + 2, win_height + 2, 1);
    if (RetrievePosition(kMapWindow, &vert, &hor)) {
        dprintf("Moving window to (%d,%d)", hor, vert);
        MoveWindow(_mt_window, hor, vert, 1);
    }
    ShowWindow(_mt_window);

    /* Start in raw, always flushing mode */
    get_tty_attrib(_mt_window, TTY_ATTRIB_FLAGS, &flag);
    flag |= TA_ALWAYS_REFRESH | TA_WRAP_AROUND;
    set_tty_attrib(_mt_window, TTY_ATTRIB_FLAGS, flag);

    get_tty_attrib(_mt_window, TTY_ATTRIB_CURSOR, &flag);
    flag |= (TA_BLINKING_CURSOR | TA_NL_ADD_CR);
    set_tty_attrib(_mt_window, TTY_ATTRIB_CURSOR, flag);

    set_tty_attrib(_mt_window, TTY_ATTRIB_FOREGROUND,
                   _mt_colors[NO_COLOR][0]);
    set_tty_attrib(_mt_window, TTY_ATTRIB_BACKGROUND,
                   _mt_colors[NO_COLOR][1]);
    clear_tty(_mt_window);

    InitMenuRes();
}

int
tgetch(void)
{
    EventRecord event;
    long sleepTime = 0;
    int ret = 0;

    for (; !ret;) {
        WaitNextEvent(-1, &event, sleepTime, 0);
        HandleEvent(&event);
        blink_cursor(_mt_window, event.when);
        if (event.what == nullEvent) {
            sleepTime = GetCaretTime();
        } else {
            sleepTime = 0;
        }
        ret = GetFromKeyQueue();
        if (ret == CHAR_CR)
            ret = CHAR_LF;
    }
    return ret;
}

void
getreturn(char *str)
{
    FlushEvents(-1, 0);
    msmsg("Press space %s", str);
    (void) tgetch();
}

int
has_color(int color)
{
#if defined(__SC__) || defined(__MRC__)
#pragma unused(color)
#endif
    Rect r;
    //	Point p = {0, 0};
    GDHandle gh;

    if (!_mt_in_color)
        return 0;

    GetWindowBounds(_mt_window, kWindowContentRgn, &r);
    //	SetPortWindowPort(_mt_window);
    //	LocalToGlobal (&p);
    //	OffsetRect (&r, p.h, p.v);

    gh = GetMaxDevice(&r);
    if (!gh) {
        return 0;
    }

    return (*((*gh)->gdPMap))->pixelSize > 4; /* > 4 bpp */
}

void
tty_delay_output(void)
{
    EventRecord event;
    long toWhen = TickCount() + 3;

    while (TickCount() < toWhen) {
        WaitNextEvent(updateMask, &event, 3L, 0);
        if (event.what == updateEvt) {
            HandleEvent(&event);
            blink_cursor(_mt_window, event.when);
        }
    }
}

void
cmov(int x, int y)
{
    move_tty_cursor(_mt_window, x, y);
    ttyDisplay->cury = y;
    ttyDisplay->curx = x;
}

void
nocmov(int x, int y)
{
    cmov(x, y);
}

static void
_mt_set_colors(long *colors)
{
    short err;

    if (!_mt_in_color) {
        return;
    }
    err = set_tty_attrib(_mt_window, TTY_ATTRIB_FOREGROUND, colors[0]);
    err = set_tty_attrib(_mt_window, TTY_ATTRIB_BACKGROUND, colors[1]);
}

int
term_attr_fixup(int attrmask)
{
    attrmask &= ~ATR_DIM;
    return attrmask;
}

void
term_end_attr(int attr)
{
#if defined(__SC__) || defined(__MRC__)
#pragma unused(attr)
#endif
    _mt_set_colors(_mt_attrs[0]);
}

void
term_start_attr(int attr)
{
    switch (attr) {
    case ATR_ULINE:
        _mt_set_colors(_mt_attrs[1]);
        break;
    case ATR_BOLD:
        _mt_set_colors(_mt_attrs[2]);
        break;
    case ATR_BLINK:
        _mt_set_colors(_mt_attrs[3]);
        break;
    case ATR_INVERSE:
        _mt_set_colors(_mt_attrs[4]);
        break;
    default:
        _mt_set_colors(_mt_attrs[0]);
        break;
    }
}

void
standoutend(void)
{
    term_end_attr(ATR_INVERSE);
}

void
standoutbeg(void)
{
    term_start_attr(ATR_INVERSE);
}

void
term_end_color(void)
{
    _mt_set_colors(_mt_colors[NO_COLOR]);
}

void
cl_end(void)
{
    _mt_set_colors(_mt_attrs[0]);
    clear_tty_window(_mt_window, ttyDisplay->curx, ttyDisplay->cury, CO - 1,
                     ttyDisplay->cury);
}

void
clear_screen(void)
{
    _mt_set_colors(_mt_attrs[0]);
    clear_tty(_mt_window);
}

void
cl_eos(void)
{
    _mt_set_colors(_mt_attrs[0]);
    clear_tty_window(_mt_window, ttyDisplay->curx, ttyDisplay->cury, CO - 1,
                     LI - 1);
}

void
home(void)
{
    cmov(0, 0);
}

void
backsp(void)
{
    char eraser[] = { CHAR_BS, CHAR_BLANK, CHAR_BS, 0 };
    short err;

    err = add_tty_string(_mt_window, eraser);
    err = update_tty(_mt_window);
}

void
msmsg(const char *str, ...)
{
    va_list args;
    char buf[1000];

    va_start(args, str);
    vsprintf(buf, str, args);
    va_end(args);

    xputs(buf);
}

void
term_end_raw_bold(void)
{
    term_end_attr(ATR_INVERSE);
}

void
term_start_raw_bold(void)
{
    term_start_attr(ATR_INVERSE);
}

void
term_start_color(int color)
{
    if (color >= 0 && color < CLR_MAX) {
        _mt_set_colors(_mt_colors[color]);
    }
}

void
setftty(void)
{
    long flag;

    /* Buffered output for the game */
    get_tty_attrib(_mt_window, TTY_ATTRIB_FLAGS, &flag);
    flag &= ~TA_ALWAYS_REFRESH;
    flag |= TA_INHIBIT_VERT_SCROLL; /* don't scroll */
    set_tty_attrib(_mt_window, TTY_ATTRIB_FLAGS, flag);
    iflags.cbreak = 1;
}

void
tty_startup(int *width, int *height)
{
    _mt_init_stuff();
    *width = CO;
    *height = LI;
}

void
gettty(void)
{
}

void
settty(const char *str)
{
    long flag;

    update_tty(_mt_window);

    /* Buffered output for the game, raw in "raw" mode */
    get_tty_attrib(_mt_window, TTY_ATTRIB_FLAGS, &flag);
    flag &= ~TA_INHIBIT_VERT_SCROLL; /* scroll */
    flag |= TA_ALWAYS_REFRESH;
    set_tty_attrib(_mt_window, TTY_ATTRIB_FLAGS, flag);

    tty_raw_print("\n");
    if (str) {
        tty_raw_print(str);
    }
}

void
tty_number_pad(int arg)
{
#if defined(__SC__) || defined(__MRC__)
#pragma unused(arg)
#endif
}

void
tty_start_screen(void)
{
    iflags.cbreak = 1;
}

void
tty_end_screen(void)
{
}

void
xputs(const char *str)
{
    add_tty_string(_mt_window, str);
}

int
term_puts(const char *str)
{
    xputs(str);
    return strlen(str);
}

int
term_putc(int c)
{
    short err;

    err = add_tty_char(_mt_window, c);
    return err ? EOF : c;
}

int
term_flush(void *desc)
{
    if (desc == stdout || desc == stderr) {
        update_tty(_mt_window);
    } else {
        impossible("Substituted flush for file");
        return fflush(desc);
    }
    return 0;
}

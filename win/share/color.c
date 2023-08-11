/* NetHack 3.7  colors.c */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef WIN32
#include "winos.h"
#ifdef CURSES_GRAPHICS
#include "curses.h"
#endif
#endif

#include "hack.h"
#include "wintty.h"

RGB *tpalette[CLR_MAX];
static RGB black = { 64, 60, 64 };

static void
default_palette(void)
{
#ifndef MICRO
    tpalette[NO_COLOR] = stdclrval("darkgray");
    tpalette[CLR_RED]     = stdclrval("firebrick");
    tpalette[CLR_GREEN]   = stdclrval("forestgreen");
    tpalette[CLR_BROWN]   = stdclrval("sienna");
    tpalette[CLR_BLUE]    = stdclrval("royalblue");
    tpalette[CLR_MAGENTA] = stdclrval("mediumpurple");
    tpalette[CLR_CYAN]    = stdclrval("darkcyan");
    tpalette[CLR_GRAY]    = stdclrval("lightsteelblue");
    tpalette[CLR_ORANGE]         = stdclrval("darkorange");
    tpalette[CLR_BRIGHT_GREEN]   = stdclrval("olivedrab");
    tpalette[CLR_YELLOW]         = stdclrval("gold");
    tpalette[CLR_BRIGHT_BLUE]    = stdclrval("dodgerblue");
    tpalette[CLR_BRIGHT_MAGENTA] = stdclrval("orchid");
    tpalette[CLR_BRIGHT_CYAN]    = stdclrval("seagreen");
    tpalette[CLR_WHITE]          = stdclrval("ivory");
    tpalette[CLR_BLACK] = &black;
#endif
}

void
set_black(unsigned char x)
{
    if (x == 1) {
#ifdef MICRO
        iflags.wc2_black = 2;
    }
#else
        if (iflags.colorcount && iflags.colorcount < 16)
            iflags.wc2_black = 0;
        else
            iflags.wc2_black = black.b;
    }
    if (x >= 4) {
        black.r = x;
        black.g = x - 4;
        black.b = x;
    }
    if (x < 90)
        tpalette[CLR_DARKGRAY] = stdclrval("dimgray");
    else
        tpalette[CLR_DARKGRAY] = stdclrval("gray");

#if defined(TTY_GRAPHICS) && defined(TEXTCOLOR)
    if (iflags.default_palette) {
        char buf[32];

        Sprintf(buf, "\033]4;0;rgb:%02x/%02x/%02x\033\\",
            black.r, black.g, black.b);
        fwrite(buf, 1, strlen(buf), stdout);

        Sprintf(buf, "\033]4;8;rgb:%02x/%02x/%02x\033\\",
            tpalette[CLR_DARKGRAY]->r,
            tpalette[CLR_DARKGRAY]->g,
            tpalette[CLR_DARKGRAY]->b);
        fwrite(buf, 1, strlen(buf), stdout);
    }
#endif
#endif
}

#if defined(TTY_GRAPHICS) && defined(TEXTCOLOR)
static const NEARDATA char *efg[CLR_MAX];
static const char *ebg[CLR_MAX];

static unsigned char
black_switch(unsigned char c)
{
    switch (c) {
        case CLR_DARKGRAY:
            switch (iflags.wc2_black) {
                case 0:
                case 2: return NO_COLOR;
            } break;
        case CLR_BLACK:
            switch (iflags.wc2_black) {
                case 0: return CLR_DARKGRAY;
                case 2: return CLR_BLUE;
            } break;
        case CLR_BLUE:
            switch (iflags.wc2_black)
                case 2: return CLR_BRIGHT_BLUE;
    }
    return c;
}

const char
*fg_hilite(unsigned char c)
{
    c = black_switch(c);
    return efg[c];
}

const char
*bg_hilite(unsigned char c)
{
    c = black_switch(c);
    return ebg[c];
}

#ifdef CURSES_GRAPHICS
/*
 *  colorpair() returns attr_t including A_BOLD for bright on systems with
 *  less than 16 colors.  Using A_REVERSE much lesser pairs are needed for
 *  the whole palette.
 */
unsigned int
colorpair(unsigned char f, unsigned char b)
{
    attr_t c, a = 0;
    unsigned char m = 16;
    register char z;

    if (b)
        b = black_switch(b);
    if (COLORS < m) {
        m = 8;
        b %= m;
    }
    if (b && f == NO_COLOR)
        f = b;
    else {
        f = black_switch(f);
        if (m == BRIGHT) {
            if (f >= m && (!b || f < CLR_BLACK))
                a |= A_BOLD;

            if (f == CLR_BLACK)
                f = m;
            if (f > m || (b && f == m))
                f %= m;
        }
        if (f < b) {
            c = b; b = f; f = c;
            a |= A_REVERSE;
        }
    }
    c = f + b * m;

    z = (b > 1) ? b : 0;
    while (z--)
        c -= z;
    if (m == BRIGHT && b)
        c -= b - 1;

    return COLOR_PAIR(c) | a;
}
#endif

static void
no_color(void)
{
    iflags.colorcount = FALSE;
    iflags.wc_color = FALSE;
    iflags.wc2_guicolor = FALSE;
    iflags.use_menu_color = FALSE;
    iflags.wc2_black = FALSE;
    set_wc_option_mod_status(WC_COLOR, set_in_config);
    set_wc2_option_mod_status(WC2_GUICOLOR, set_in_config);
    set_wc2_option_mod_status(WC2_BLACK, set_in_config);
}

#if defined(UNIX) && defined(TERMINFO) && !defined(ANSI_DEFAULT)
/*
 * Sets up color highlighting, using terminfo(4) escape sequences.
 *
 * Having never seen a terminfo system without curses, we assume this
 * inclusion is safe.  On systems with color terminfo, it should define
 * the 8 COLOR_FOOs, and avoid us having to guess whether this particular
 * terminfo uses BGR or RGB for its indexes.
 *
 * If we don't get the definitions, then guess.  Original color terminfos
 * used BGR for the original Sf (setf, Standard foreground) codes, but
 * there was a near-total lack of user documentation, so some subsequent
 * terminfos, such as early Linux ncurses and SCO UNIX, used RGB.  Possibly
 * as a result of the confusion, AF (setaf, ANSI Foreground) codes were
 * introduced, but this caused yet more confusion.  Later Linux ncurses
 * have BGR Sf, RGB AF, and RGB COLOR_FOO, which appears to be the SVR4
 * standard.  We could switch the colors around when using Sf with ncurses,
 * which would help things on later ncurses and hurt things on early ncurses.
 * We'll try just preferring AF and hoping it always agrees with COLOR_FOO,
 * and falling back to Sf if AF isn't defined.
 *
 * In any case, treat black specially so we don't try to display black
 * characters on the assumed black background.
 */

/* `curses' is aptly named; various versions don't like these
    macros used elsewhere within nethack; fortunately they're
    not needed beyond this point, so we don't need to worry
    about reconstructing them after the header file inclusion. */
#undef TRUE
#undef FALSE
#define m_move curses_m_move /* Some curses.h decl m_move(), not used here */

#include <curses.h>

#if !defined(LINUX) && !defined(__FreeBSD__) && !defined(NOTPARMDECL)
extern char *tparm();
#endif

#ifndef COLOR_BLACK /* trust include file */
#ifndef _M_UNIX     /* guess BGR */
#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_MAGENTA 5
#define COLOR_YELLOW 6
#define COLOR_WHITE 7
#else /* guess RGB */
#define COLOR_BLACK 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_YELLOW 3
#define COLOR_BLUE 4
#define COLOR_MAGENTA 5
#define COLOR_CYAN 6
#define COLOR_WHITE 7
#endif
#endif

/* Mapping data for the terminfo colors that resolve to pairs of nethack
 * colors.  Black is handled specially.
 */
const struct {
    int ti_color, nh_color, nh_bright_color;
} ti_map[8] = { { COLOR_BLACK, CLR_BLACK, CLR_DARKGRAY },
                { COLOR_RED, CLR_RED, CLR_ORANGE },
                { COLOR_GREEN, CLR_GREEN, CLR_BRIGHT_GREEN },
                { COLOR_YELLOW, CLR_BROWN, CLR_YELLOW },
                { COLOR_BLUE, CLR_BLUE, CLR_BRIGHT_BLUE },
                { COLOR_MAGENTA, CLR_MAGENTA, CLR_BRIGHT_MAGENTA },
                { COLOR_CYAN, CLR_CYAN, CLR_BRIGHT_CYAN },
                { COLOR_WHITE, CLR_GRAY, CLR_WHITE } };
#endif // TERMINFO

void
init_hilite(void)
{
    register int c;

    for (c = NO_COLOR; c < CLR_MAX; c++)
        efg[c] = ebg[c] = (char *) 0;

    if (iflags.default_palette)
        set_palette();

#ifdef CURSES_GRAPHICS
    if (WINDOWPORT(curses)) {
        if (has_colors()) {
            unsigned char n, m;
            register char f, b;

            start_color();
            use_default_colors();
            iflags.colorcount = COLORS;
            if (COLORS < 16) {
                iflags.wc2_black = 0;
                m = 8;
            } else
                m = 16;

            for (b = 0; b < CLR_MAX; b++) {
                if (COLORS < 16 && b >= BRIGHT)
                    continue;

                for (f = 1; f < CLR_MAX; f++) {
                    if (COLORS < 16 && (f > BRIGHT || (f == BRIGHT && b)))
                        continue;

                    if (f >= b) {
                        n = f + b * m;
                        c = (b > 1) ? b : 0;
                        while (c--)
                            n -= c;
                        if (COLORS < 16 && b)
                            n -= b - 1;

                        if (f == b)
                            init_pair(n, -1, b % m);
                        else
                            if (!b)
                                init_pair(n, f % m, -1);
                            else
                                init_pair(n, f % m, b % m);
                    }
                }
            }
        } else no_color();

        return;
    }
#endif
#if defined(TOS) && !defined(ANSI_DEFAULT)
//  iflags.colorcount = 1 << (((unsigned char *) _a_line)[1]);
    char foo[8];

    efg[CLR_RED] = dupstr("\033b1");
    efg[CLR_GREEN] = dupstr("\033b2");

    if (iflags.colorcount == 4) {
        efg[CLR_BRIGHT_RED] = efg[CLR_RED];
        efg[CLR_BRIGHT_GREEN] = efg[CLR_GREEN];

        TI = dupstr("\033b0\033c3\033E\033e");
        TE = dupstr("\033b3\033c0\033J");
    } else {
        for(c = 3; c < CLR_MAX; ++c) {
            Sprintf(foo, "\033b%d", c % 16);
            efg[c] = dupstr(foo);
        }
        TI = dupstr("\033b0\033c\017\033E\033e");
        TE = dupstr("\033b\017\033c0\033J");
    }
    nh_HE = dupstr("\033q\033b0");

#elif defined(UNIX) && defined(TERMINFO) && !defined(ANSI_DEFAULT)
    extern const char *setf, *setb;
    extern char *MD;
    char *foo;

    if (iflags.colorcount < 8 || (MD == NULL || strlen(MD) == 0) || !setf) {
        no_color();
        return;
    }

    for (c = 0; c < SIZE(ti_map); c++) {
        foo = tparm(setf, ti_map[c].ti_color);
        efg[ti_map[c].nh_color] = dupstr(foo);

        foo = tparm(setb, ti_map[c].ti_color);
        ebg[ti_map[c].nh_color] = dupstr(foo);
    }
iflags.colorcount = 8;
    if (iflags.colorcount >= 16)
        for (c = 0; c < SIZE(ti_map); c++) {
            foo = tparm(setf, ti_map[c].ti_color | BRIGHT);
            efg[ti_map[c].nh_bright_color] = dupstr(foo);

            foo = tparm(setb, ti_map[c].ti_color | BRIGHT);
            ebg[ti_map[c].nh_bright_color] = dupstr(foo);
        }
    else {
        /* On many terminals, esp. those using classic PC CGA/EGA/VGA
         * textmode, specifying "hilight" and "black" simultaneously
         * produces a dark shade of gray that is visible against a
         * black background.  We can use it to represent black objects.
         */
        char *bar;
        for (c = 0; c < SIZE(ti_map); c++) {
            bar = tparm(setf, ti_map[c].ti_color);
            Strcpy(foo, bar);
            Strcat(foo, MD);
            efg[ti_map[c].nh_bright_color] = dupstr(foo);

            bar = tparm(setb, ti_map[c].ti_color);
            ebg[ti_map[c].nh_bright_color] = dupstr(bar);
        }
    }
#else // ANSI_DEFAULT
    char foo[16];

    for (c = 1; c < BRIGHT; c++) {
        Sprintf(foo, "\033[3%dm", c);
        efg[c] = dupstr(foo);

        Sprintf(foo, "\033[4%dm", c);
        ebg[c] = dupstr(foo);

        Sprintf(foo, "\033[3%d;9%dm", c, c);
        efg[c | BRIGHT] = dupstr(foo);

        Sprintf(foo, "\033[4%d;10%dm", c, c);
        ebg[c | BRIGHT] = dupstr(foo);
    }
    efg[CLR_BLACK] = dupstr("\033[30m");
    ebg[CLR_BLACK] = dupstr("\033[40m");

    efg[CLR_DARKGRAY] = dupstr("\033[30;90m");
    ebg[CLR_DARKGRAY] = dupstr("\033[40;100m");

    efg[NO_COLOR] = dupstr("\033[39m");
    ebg[NO_COLOR] = dupstr("\033[49m");
#endif

    if (iflags.colorcount && iflags.colorcount < 16)
        iflags.wc2_black = 0;
}

#ifndef MICRO
void
set_palette(void)
{
// !PDCurses
    register int c;
    char buf[32];
    RGB *cursor;

    if (!tpalette[0])
        default_palette();

    for (c = 1; c < CLR_MAX; c++) {
        Sprintf(buf, "\033]4;%d;rgb:%02x/%02x/%02x\033\\", c % 16,
            tpalette[c]->r, tpalette[c]->g, tpalette[c]->b);
        fwrite(buf, 1, strlen(buf), stdout);
    }
    Sprintf(buf, "\033]10;rgb:%02x/%02x/%02x\033\\",
        tpalette[0]->r, tpalette[0]->g, tpalette[0]->b);
    fwrite(buf, 1, strlen(buf), stdout);

    Strcpy(buf, "\033]11;rgb:0/0/0\033\\");
    fwrite(buf, 1, strlen(buf), stdout);

#ifndef WIN32
    cursor = stdclrval("lightgray");
    Sprintf(buf, "\033]12;rgb:%02x/%02x/%02x\033\\",
        cursor->r, cursor->g, cursor->b);
    fwrite(buf, 1, strlen(buf), stdout);
#endif
    Strcpy(buf, "\033[2 q");
    fwrite(buf, 1, strlen(buf), stdout);
}

#ifndef WIN32
void
reset_palette(void)
{
    puts("\033]104\033\\");
    puts("\033]110\033\\");
    puts("\033]111\033\\");
    puts("\033]112\033\\");
    puts("\033[0 q");
}
#endif
#endif
#endif // TTY_GRAPHICS && TEXTCOLOR

#ifndef MICRO
struct named_color {
    const char *name;
    RGB val;
} stdclr[] = {
    { "maroon", { 0, 0, 128 } },
    { "darkred", { 0, 0, 139 } },
    { "brown", { 42, 42, 165 } },
    { "firebrick", { 34, 34, 178 } },
    { "crimson", { 60, 20, 220 } },
    { "red", { 0, 0, 255 } },
    { "tomato", { 71, 99, 255 } },
    { "coral", { 80, 127, 255 } },
    { "indianred", { 92, 92, 205 } },
    { "lightcoral", { 128, 128, 240 } },
    { "darksalmon", { 122, 150, 233 } },
    { "salmon", { 114, 128, 250 } },
    { "lightsalmon", { 122, 160, 255 } },
    { "orangered", { 0, 69, 255 } },
    { "darkorange", { 0, 140, 255 } },
    { "orange", { 0, 165, 255 } },
    { "gold", { 0, 215, 255 } },
    { "darkgoldenrod", { 11, 134, 184 } },
    { "goldenrod", { 32, 165, 218 } },
    { "palegoldenrod", { 170, 232, 238 } },
    { "darkkhaki", { 107, 183, 189 } },
    { "khaki", { 140, 230, 240 } },
    { "olive", { 0, 128, 128 } },
    { "yellow", { 0, 255, 255 } },
    { "yellowgreen", { 50, 205, 154 } },
    { "darkolivegreen", { 47, 107, 85 } },
    { "olivedrab", { 35, 142, 107 } },
    { "lawngreen", { 0, 252, 124 } },
    { "chartreuse", { 0, 255, 127 } },
    { "greenyellow", { 47, 255, 173 } },
    { "darkgreen", { 0, 100, 0 } },
    { "green", { 0, 128, 0 } },
    { "forestgreen", { 34, 139, 34 } },
    { "lime", { 0, 255, 0 } },
    { "limegreen", { 50, 205, 50 } },
    { "lightgreen", { 144, 238, 144 } },
    { "palegreen", { 152, 251, 152 } },
    { "darkseagreen", { 143, 188, 143 } },
    { "mediumspringgreen", { 154, 250, 0 } },
    { "springgreen", { 127, 255, 0 } },
    { "seagreen", { 87, 139, 46 } },
    { "mediumaquamarine", { 170, 205, 102 } },
    { "mediumseagreen", { 113, 179, 60 } },
    { "lightseagreen", { 170, 178, 32 } },
    { "darkslategray", { 79, 79, 47 } },
    { "teal", { 128, 128, 0 } },
    { "darkcyan", { 139, 139, 0 } },
    { "aqua", { 255, 255, 0 } },
    { "cyan", { 255, 255, 0 } },
    { "lightcyan", { 255, 255, 224 } },
    { "darkturquoise", { 209, 206, 0 } },
    { "turquoise", { 208, 224, 64 } },
    { "mediumturquoise", { 204, 209, 72 } },
    { "paleturquoise", { 238, 238, 175 } },
    { "aquamarine", { 212, 255, 127 } },
    { "powderblue", { 230, 224, 176 } },
    { "cadetblue", { 160, 158, 95 } },
    { "steelblue", { 180, 130, 70 } },
    { "cornflowerblue", { 237, 149, 100 } },
    { "deepskyblue", { 255, 191, 0 } },
    { "dodgerblue", { 255, 144, 30 } },
    { "lightblue", { 230, 216, 173 } },
    { "skyblue", { 235, 206, 135 } },
    { "lightskyblue", { 250, 206, 135 } },
    { "midnightblue", { 112, 25, 25 } },
    { "navy", { 128, 0, 0 } },
    { "darkblue", { 139, 0, 0 } },
    { "mediumblue", { 205, 0, 0 } },
    { "blue", { 255, 0, 0 } },
    { "royalblue", { 225, 105, 65 } },
    { "blueviolet", { 226, 43, 138 } },
    { "indigo", { 130, 0, 75 } },
    { "darkslateblue", { 139, 61, 72 } },
    { "slateblue", { 205, 90, 106 } },
    { "mediumslateblue", { 238, 104, 123 } },
    { "mediumpurple", { 219, 112, 147 } },
    { "darkmagenta", { 139, 0, 139 } },
    { "darkviolet", { 211, 0, 148 } },
    { "darkorchid", { 204, 50, 153 } },
    { "mediumorchid", { 211, 85, 186 } },
    { "purple", { 128, 0, 128 } },
    { "thistle", { 216, 191, 216 } },
    { "plum", { 221, 160, 221 } },
    { "violet", { 238, 130, 238 } },
    { "magenta", { 255, 0, 255 } },
    { "fuchsia", { 255, 0, 255 } },
    { "orchid", { 214, 112, 218 } },
    { "mediumvioletred", { 133, 21, 199 } },
    { "palevioletred", { 147, 112, 219 } },
    { "deeppink", { 147, 20, 255 } },
    { "hotpink", { 180, 105, 255 } },
    { "lightpink", { 193, 182, 255 } },
    { "pink", { 203, 192, 255 } },
    { "antiquewhite", { 215, 235, 250 } },
    { "beige", { 220, 245, 245 } },
    { "bisque", { 196, 228, 255 } },
    { "blanchedalmond", { 205, 235, 255 } },
    { "wheat", { 179, 222, 245 } },
    { "cornsilk", { 220, 248, 255 } },
    { "lemonchiffon", { 205, 250, 255 } },
    { "lightgoldenrodyellow", { 210, 250, 250 } },
    { "lightyellow", { 224, 255, 255 } },
    { "saddlebrown", { 19, 69, 139 } },
    { "sienna", { 45, 82, 160 } },
    { "chocolate", { 30, 105, 210 } },
    { "peru", { 63, 133, 205 } },
    { "sandybrown", { 96, 164, 244 } },
    { "burlywood", { 135, 184, 222 } },
    { "tan", { 140, 180, 210 } },
    { "rosybrown", { 143, 143, 188 } },
    { "moccasin", { 181, 228, 255 } },
    { "navajowhite", { 173, 222, 255 } },
    { "peachpuff", { 185, 218, 255 } },
    { "mistyrose", { 225, 228, 255 } },
    { "lavenderblush", { 245, 240, 255 } },
    { "linen", { 230, 240, 250 } },
    { "oldlace", { 230, 245, 253 } },
    { "papayawhip", { 213, 239, 255 } },
    { "seashell", { 238, 245, 255 } },
    { "mintcream", { 250, 255, 245 } },
    { "slategray", { 144, 128, 112 } },
    { "lightslategray", { 153, 136, 119 } },
    { "lightsteelblue", { 222, 196, 176 } },
    { "lavender", { 250, 230, 230 } },
    { "floralwhite", { 240, 250, 255 } },
    { "aliceblue", { 255, 248, 240 } },
    { "ghostwhite", { 255, 248, 248 } },
    { "honeydew", { 240, 255, 240 } },
    { "ivory", { 240, 255, 255 } },
    { "azure", { 255, 255, 240 } },
    { "snow", { 250, 250, 255 } },
    { "black", { 0, 0, 0 } },
    { "dimgray", { 105, 105, 105 } },
    { "dimgrey", { 105, 105, 105 } },
    { "gray", { 128, 128, 128 } },
    { "grey", { 128, 128, 128 } },
    { "darkgray", { 169, 169, 169 } },
    { "darkgrey", { 169, 169, 169 } },
    { "silver", { 192, 192, 192 } },
    { "lightgray", { 211, 211, 211 } },
    { "lightgrey", { 211, 211, 211 } },
    { "gainsboro", { 220, 220, 220 } },
    { "whitesmoke", { 245, 245, 245 } },
    { "white", { 255, 255, 255 } },
};

RGB
*stdclrval(const char *name)
{
    register int i;

    for (i = 0; i < SIZE(stdclr); ++i)
    {
        if (strcmp(name, stdclr[i].name) == 0)
            return &stdclr[i].val;
    }
    if (tpalette[NO_COLOR])
        return tpalette[NO_COLOR];
    else
        return stdclrval("gray");
}
#endif

//eof colors.c

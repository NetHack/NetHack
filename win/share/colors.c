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
void default_palette(void);
void set_black(unsigned char);
void use_darkgray(void);

void
default_palette(void)
{
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
}

void
set_black(unsigned char b)
{
    if (b == 0) b = 64;
    iflags.wc2_black = b;
    RGB varblack = { b, b - 4, b };
    tpalette[CLR_BLACK] = &varblack;

    if (iflags.wc2_setpalette) {
        if (b < 90)
            tpalette[CLR_DARKGRAY] = stdclrval("dimgray");
        else
            tpalette[CLR_DARKGRAY] = stdclrval("gray");

#if defined(TTY_GRAPHICS) && !defined(MICRO)
#ifndef TOS
        printf("\033]4;0;#%02x%02x%02x\033\\",
            tpalette[CLR_BLACK]->r,
            tpalette[CLR_BLACK]->g,
            tpalette[CLR_BLACK]->b);
        printf("\033]4;8;#%02x%02x%02x\033\\",
            tpalette[CLR_DARKGRAY]->r,
            tpalette[CLR_DARKGRAY]->g,
            tpalette[CLR_DARKGRAY]->b);
#endif

        if (WINDOWPORT(curses)) {
            init_pair(CLR_BLACK, COLOR_BLACK, -1);
            init_pair(CLR_DARKGRAY, CLR_DARKGRAY, -1);
        } else {
#if defined(TERMLIB) && !defined(ANSI_DEFAULT)
            char foo[16];
            hilites[CLR_BLACK] = dupstr(tparm(setf, 0));
            bghilites[CLR_BLACK] = dupstr(tparm(setb, 0));
            foo = tparm(setf, COLOR_BLACK|BRIGHT);
            hilites[CLR_DARKGRAY] = dupstr(foo);
            foo = tparm(setb, COLOR_BLACK|BRIGHT);
            bghilites[CLR_DARKGRAY] = dupstr(foo);
#else
/* FIXME WINDOWPORT(mswin) && !VIRTUAL_TERMINAL_SEQUENCES */
            hilites[CLR_BLACK] = dupstr("\033[30m");
            bghilites[CLR_BLACK] = dupstr("\033[40m");
            hilites[CLR_DARKGRAY] = dupstr("\033[90m");
            bghilites[CLR_DARKGRAY] = dupstr("\033[100m");
#endif
        }
#endif
    }
}

void
use_darkgray(void)
{
    iflags.wc2_black = 0;
#if defined(TTY_GRAPHICS) && !defined(MICRO)
    if (WINDOWPORT(curses)) {
        init_pair(CLR_BLACK, CLR_DARKGRAY, -1);
        init_pair(CLR_DARKGRAY, -1, -1);
    } else {
        hilites[CLR_BLACK] = hilites[CLR_DARKGRAY];
        hilites[CLR_DARKGRAY] = hilites[NO_COLOR];
        bghilites[CLR_BLACK] = bghilites[CLR_DARKGRAY];
        bghilites[CLR_DARKGRAY] = bghilites[NO_COLOR];
    }
#endif
}

#ifdef TTY_GRAPHICS
void set_palette(void);
void reset_palette(void);
void init_hilite(void);
const NEARDATA char *hilites[CLR_MAX];
const char *bghilites[CLR_MAX];

#if defined(TERMLIB) && !defined(ANSI_DEFAULT)
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

#ifdef TERMINFO
/* `curses' is aptly named; various versions don't like these
    macros used elsewhere within nethack; fortunately they're
    not needed beyond this point, so we don't need to worry
    about reconstructing them after the header file inclusion. */
#undef TRUE
#undef FALSE
#define m_move curses_m_move /* Some curses.h decl m_move(), not used here */

#include <curses.h>
#endif

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
} ti_map[7] = { { COLOR_RED, CLR_RED, CLR_ORANGE },
                { COLOR_GREEN, CLR_GREEN, CLR_BRIGHT_GREEN },
                { COLOR_YELLOW, CLR_BROWN, CLR_YELLOW },
                { COLOR_BLUE, CLR_BLUE, CLR_BRIGHT_BLUE },
                { COLOR_MAGENTA, CLR_MAGENTA, CLR_BRIGHT_MAGENTA },
                { COLOR_CYAN, CLR_CYAN, CLR_BRIGHT_CYAN },
                { COLOR_WHITE, CLR_GRAY, CLR_WHITE } };

const char *setf, *setb;
(setf = dupstr(Tgetstr("AF")))
    ?(setb = dupstr(Tgetstr("AB")))
    :(setf = dupstr(Tgetstr("Sf")));
#endif

void
init_hilite(void)
{
    register int c;
    char foo[16];
    static char nullstr[] = "";
    hilites[NO_COLOR] = nullstr;

#if defined(TERMLIB) && !defined(ANSI_DEFAULT)
    static char tbuf[512];
    char *tbufptr;
    tbufptr = tbuf;
    char *md = dupstr(tgetstr("md"), &tbufptr);

    extern *MD;
    int md_len = strlen(MD);
    iflags.colorcount = tgetnum("Co");

    if (iflags.colorcount < 8 || (MD == NULL || md_len == 0)
        || setf == (char*) 0)
            return;

    c = SIZE(ti_map);

    if (iflags.colorcount >= 16) {
        while (c--) {
            foo = tparm(setf, ti_map[c].nh_color);
            hilites[ti_map[c].nh_color] = dupstr(foo);
            foo = tparm(setf, ti_map[c].nh_bright_color);
            hilites[ti_map[c].nh_bright_color] = dupstr(foo);
            foo = tparm(setb, ti_map[c].nh_color);
            bghilites[ti_map[c].nh_color] = dupstr(foo);
            foo = tparm(setb, ti_map[c].nh_bright_color);
            bghilites[ti_map[c].nh_bright_color] = dupstr(foo);
        }
    } else {
        while (c--) {
            foo = tparm(setf, ti_map[c].ti_color);
            hilites[ti_map[c].nh_color] = dupstr(foo);
            hilites[ti_map[c].nh_bright_color] = (char *) alloc(strlen(foo) + md_len + 1);
            Strcpy(hilites[ti_map[c].nh_bright_color], md);
            Strcat(hilites[ti_map[c].nh_bright_color], foo);
            foo = tparm(setb, ti_map[c].ti_color);
            bghilites[ti_map[c].nh_color] = dupstr(foo);
            bghilites[ti_map[c].nh_bright_color] = bghilites[ti_map[c].nh_color];
        }
        foo = tparm(setf, COLOR_BLACK);
        hilites[CLR_DARKGRAY] = (char *) alloc(strlen(foo) + md_len + 1);
        Strcpy(hilites[CLR_DARKGRAY], md);
        Strcat(hilites[CLR_DARKGRAY], foo);
        foo = tparm(setb, COLOR_BLACK);
        bghilites[CLR_DARKGRAY] = dupstr(foo);
        use_darkgray();
        set_wc2_option_mod_status(WC2_BLACK, set_in_config);
    }
#elif defined(TOS) && !defined(ANSI_DEFAULT)
    extern char *TI, *TE, *nh_HE;
    iflags.colorcount = 1 << (((unsigned char *) _a_line)[1]);
    set_wc2_option_mod_status(WC2_BLACK, set_in_config);

    /* Under TOS, the "bright" and "dim" colors had been reversed.
     * Moreover, on the Falcon the dim colors were *really* dim.
     */
    if (iflags.colorcount < 3)
        iflags.wc_color = FALSE;
        iflags.wc2_guicolor = FALSE;
        set_wc_option_mod_status(WC_COLOR, set_in_config);
        set_wc2_option_mod_status(WC2_GUICOLOR, set_in_config);
    else {
        hilites[CLR_RED] = dupstr("\033b1");
        hilites[CLR_GREEN] = dupstr("\033b2");

            if (iflags.colorcount > 4)
            for(c = 3; c < 16; ++c) {
                Sprintf(foo, "\033b%d", c);
                hilites[c] = dupstr(foo);
            }
            if (iflags.colorcount > 16)
                hilites[CLR_BLACK] = dupstr("\033b0");
            else
                hilites[CLR_BLACK] = hilites[CLR_DARKGRAY];
        else
            for(c = 3; c < CLR_MAX; ++c)
                hilites[c] = hilites[NO_COLOR];
    }

    if (iflags.colorcount == 4) {
        hilites[CLR_BRIGHT_RED] = hilites[CLR_RED];
        hilites[CLR_BRIGHT_GREEN] = hilites[CLR_GREEN];

        TI = dupstr("\033b0\033c3\033E\033e");
        TE = dupstr("\033b3\033c0\033J");
    } else {
        TI = dupstr("\033b0\033c\017\033E\033e");
        TE = dupstr("\033b\017\033c0\033J");
    }
    nh_HE = dupstr("\033q\033b0");
#else
    for(c = 1; c < 8; ++c) {
        Sprintf(foo, "\033[3%dm", c);
        hilites[c] = dupstr(foo);
        Sprintf(foo, "\033[3%d;9%dm", c, c);
        hilites[c|BRIGHT] = dupstr(foo);
        Sprintf(foo, "\033[4%dm", c);
        bghilites[c] = dupstr(foo);
        Sprintf(foo, "\033[4%d;10%dm", c, c);
        bghilites[c|BRIGHT] = dupstr(foo);
    }
#endif

#ifdef MICRO
    hilites[CLR_BLACK] = hilites[CLR_BLUE];
    hilites[CLR_BLUE] = hilites[CLR_BRIGHT_BLUE];
    bghilites[CLR_BLACK] = bghilites[CLR_BLUE];
    bghilites[CLR_BLUE] = bghilites[CLR_BRIGHT_BLUE];
    set_wc2_option_mod_status(WC2_BLACK, set_in_config);
#endif

    if (iflags.wc2_setpalette)
        set_palette();
}

#ifndef TOS
void
set_palette(void)
{
    register int c;
    RGB *foo;

    if (!tpalette[0])
        default_palette();

    for(c = 1; c < 8; ++c)
        printf("\033]4;%d;#%02x%02x%02x\033\\", c,
            tpalette[c]->r, tpalette[c]->g, tpalette[c]->b);
    for(c = 9; c < 16; ++c)
        printf("\033]4;%d;#%02x%02x%02x\033\\", c,
            tpalette[c]->r, tpalette[c]->g, tpalette[c]->b);
    printf("\033]10;#%02x%02x%02x\033\\",
        tpalette[NO_COLOR]->r, tpalette[NO_COLOR]->g, tpalette[NO_COLOR]->b);
    foo = stdclrval("black");
    printf("\033]11;#%02x%02x%02x\033\\", foo->r, foo->g, foo->b);
    foo = stdclrval("lightgray");
    printf("\033]12;#%02x%02x%02x\033\\", foo->r, foo->g, foo->b);
    printf("\033[2 q");
}

void
reset_palette(void)
{
// !WIN32
    printf("\033]104\033\\");
    printf("\033]110\033\\");
    printf("\033]111\033\\");
    printf("\033]112\033\\");
    printf("\033[0 q");
}
#endif
#endif // TTY_GRAPHICS

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

int num_named_colors = SIZE(stdclr);

RGB
*stdclrval(const char *name)
{
    register int i;

    for(i = 0; i < num_named_colors; ++i)
    {
        if(strcmp(name, stdclr[i].name) == 0)
            return &stdclr[i].val;
    }
    if (tpalette[0])
        return tpalette[0];
    else
        return stdclrval("gray");
}

//eof colors.c

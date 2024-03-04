/* NetHack 3.7	coloratt.c	$NHDT-Date: 1709559438 2024/03/04 13:37:18 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $ */
/* Copyright (c) Pasi Kallinen, 2024 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include <ctype.h>

struct color_names {
    const char *name;
    int color;
};

static const struct color_names colornames[] = {
    { "black", CLR_BLACK },
    { "red", CLR_RED },
    { "green", CLR_GREEN },
    { "brown", CLR_BROWN },
    { "blue", CLR_BLUE },
    { "magenta", CLR_MAGENTA },
    { "cyan", CLR_CYAN },
    { "gray", CLR_GRAY },
    { "orange", CLR_ORANGE },
    { "light green", CLR_BRIGHT_GREEN },
    { "yellow", CLR_YELLOW },
    { "light blue", CLR_BRIGHT_BLUE },
    { "light magenta", CLR_BRIGHT_MAGENTA },
    { "light cyan", CLR_BRIGHT_CYAN },
    { "white", CLR_WHITE },
    { "no color", NO_COLOR },
    { (const char *) 0, CLR_BLACK }, /* everything after this is an alias */
    { "transparent", NO_COLOR },
    { "purple", CLR_MAGENTA },
    { "light purple", CLR_BRIGHT_MAGENTA },
    { "bright purple", CLR_BRIGHT_MAGENTA },
    { "grey", CLR_GRAY },
    { "bright red", CLR_ORANGE },
    { "bright green", CLR_BRIGHT_GREEN },
    { "bright blue", CLR_BRIGHT_BLUE },
    { "bright magenta", CLR_BRIGHT_MAGENTA },
    { "bright cyan", CLR_BRIGHT_CYAN }
};

struct attr_names {
    const char *name;
    int attr;
};

static const struct attr_names attrnames[] = {
    { "none", ATR_NONE },
    { "bold", ATR_BOLD },
    { "dim", ATR_DIM },
    { "italic", ATR_ITALIC },
    { "underline", ATR_ULINE },
    { "blink", ATR_BLINK },
    { "inverse", ATR_INVERSE },
    { (const char *) 0, ATR_NONE }, /* everything after this is an alias */
    { "normal", ATR_NONE },
    { "uline", ATR_ULINE },
    { "reverse", ATR_INVERSE },
};

/* { colortyp, tableindex, rgbindx, name, hexval, r, g, b }, */

static struct nethack_color colortable[] = {
  { nh_color,    0,   0, "black",                   "",   0,   0,   0 },
  { nh_color,    1,   0, "red",                     "", 255,   0,   0 },
  { nh_color,    2,   0, "green",                   "",  34, 139,  34 },
  { nh_color,    3,   0, "brown",                   "", 165,  42,  42 },
  { nh_color,    4,   0, "blue",                    "",   0,   0, 255 },
  { nh_color,    5,   0, "magenta",                 "", 255,   0, 255 },
  { nh_color,    6,   0, "cyan",                    "",   0, 255, 255 },
  { nh_color,    7,   0, "gray",                    "", 128, 128, 128 },
  { no_color,    8,   0, "nocolor",                 "",   0,   0,   0 },
  { nh_color,    9,   0, "orange",                  "", 255, 165,   0 },
  { nh_color,   10,   0, "bright-green",            "",   0, 128,   0 },
  { nh_color,   11,   0, "yellow",                  "", 255, 255,   0 },
  { nh_color,   12,   0, "bright-blue",             "", 173, 216, 230 },
  { nh_color,   13,   0, "bright-magenta",          "", 147, 112, 219 },
  { nh_color,   14,   0, "light-cyan",              "", 224, 255, 255 },
  { nh_color,   15,   0, "white",                   "", 255, 255, 255 },
  { rgb_color,  16,   0, "maroon",                  "#800000", 128,   0,   0 },
  { rgb_color,  17,   1, "dark-red",                "#8B0000", 139,   0,   0 },
  { rgb_color,  18,   2, "brown",                   "#A52A2A", 165,  42,  42 },
  { rgb_color,  19,   3, "firebrick",               "#B22222", 178,  34,  34 },
  { rgb_color,  20,   4, "crimson",                 "#DC143C", 220,  20,  60 },
  { rgb_color,  21,   5, "red",                     "#FF0000", 255,   0,   0 },
  { rgb_color,  22,   6, "tomato",                  "#FF6347", 255,  99,  71 },
  { rgb_color,  23,   7, "coral",                   "#FF7F50", 255, 127,  80 },
  { rgb_color,  24,   8, "indian-red",              "#CD5C5C", 205,  92,  92 },
  { rgb_color,  25,   9, "light-coral",             "#F08080", 240, 128, 128 },
  { rgb_color,  26,  10, "dark-salmon",             "#E9967A", 233, 150, 122 },
  { rgb_color,  27,  11, "salmon",                  "#FA8072", 250, 128, 114 },
  { rgb_color,  28,  12, "light-salmon",            "#FFA07A", 255, 160, 122 },
  { rgb_color,  29,  13, "orange-red",              "#FF4500", 255,  69,   0 },
  { rgb_color,  30,  14, "dark-orange",             "#FF8C00", 255, 140,   0 },
  { rgb_color,  31,  15, "orange",                  "#FFA500", 255, 165,   0 },
  { rgb_color,  32,  16, "gold",                    "#FFD700", 255, 215,   0 },
  { rgb_color,  33,  17, "dark-golden-rod",         "#B8860B", 184, 134,  11 },
  { rgb_color,  34,  18, "golden-rod",              "#DAA520", 218, 165,  32 },
  { rgb_color,  35,  19, "pale-golden-rod",         "#EEE8AA", 238, 232, 170 },
  { rgb_color,  36,  20, "dark-khaki",              "#BDB76B", 189, 183, 107 },
  { rgb_color,  37,  21, "khaki",                   "#F0E68C", 240, 230, 140 },
  { rgb_color,  38,  22, "olive",                   "#808000", 128, 128,   0 },
  { rgb_color,  39,  23, "yellow",                  "#FFFF00", 255, 255,   0 },
  { rgb_color,  40,  24, "yellow-green",            "#9ACD32", 154, 205,  50 },
  { rgb_color,  41,  25, "dark-olive-green",        "#556B2F",  85, 107,  47 },
  { rgb_color,  42,  26, "olive-drab",              "#6B8E23", 107, 142,  35 },
  { rgb_color,  43,  27, "lawn-green",              "#7CFC00", 124, 252,   0 },
  { rgb_color,  44,  28, "chart-reuse",             "#7FFF00", 127, 255,   0 },
  { rgb_color,  45,  29, "green-yellow",            "#ADFF2F", 173, 255,  47 },
  { rgb_color,  46,  30, "dark-green",              "#006400",   0, 100,   0 },
  { rgb_color,  47,  31, "green",                   "#008000",   0, 128,   0 },
  { rgb_color,  48,  32, "forest-green",            "#228B22",  34, 139,  34 },
  { rgb_color,  49,  33, "lime",                    "#00FF00",   0, 255,   0 },
  { rgb_color,  50,  34, "lime-green",              "#32CD32",  50, 205,  50 },
  { rgb_color,  51,  35, "light-green",             "#90EE90", 144, 238, 144 },
  { rgb_color,  52,  36, "pale-green",              "#98FB98", 152, 251, 152 },
  { rgb_color,  53,  37, "dark-sea-green",          "#8FBC8F", 143, 188, 143 },
  { rgb_color,  54,  38, "medium-spring-green",     "#00FA9A",   0, 250, 154 },
  { rgb_color,  55,  39, "spring-green",            "#00FF7F",   0, 255, 127 },
  { rgb_color,  56,  40, "sea-green",               "#2E8B57",  46, 139,  87 },
  { rgb_color,  57,  41, "medium-aqua-marine",      "#66CDAA", 102, 205, 170 },
  { rgb_color,  58,  42, "medium-sea-green",        "#3CB371",  60, 179, 113 },
  { rgb_color,  59,  43, "light-sea-green",         "#20B2AA",  32, 178, 170 },
  { rgb_color,  60,  44, "dark-slate-gray",         "#2F4F4F",  47,  79,  79 },
  { rgb_color,  61,  45, "teal",                    "#008080",   0, 128, 128 },
  { rgb_color,  62,  46, "dark-cyan",               "#008B8B",   0, 139, 139 },
  { rgb_color,  63,  47, "aqua",                    "#00FFFF",   0, 255, 255 },
  { rgb_color,  64,  48, "cyan",                    "#00FFFF",   0, 255, 255 },
  { rgb_color,  65,  49, "light-cyan",              "#E0FFFF", 224, 255, 255 },
  { rgb_color,  66,  50, "dark-turquoise",          "#00CED1",   0, 206, 209 },
  { rgb_color,  67,  51, "turquoise",               "#40E0D0",  64, 224, 208 },
  { rgb_color,  68,  52, "medium-turquoise",        "#48D1CC",  72, 209, 204 },
  { rgb_color,  69,  53, "pale-turquoise",          "#AFEEEE", 175, 238, 238 },
  { rgb_color,  70,  54, "aqua-marine",             "#7FFFD4", 127, 255, 212 },
  { rgb_color,  71,  55, "powder-blue",             "#B0E0E6", 176, 224, 230 },
  { rgb_color,  72,  56, "cadet-blue",              "#5F9EA0",  95, 158, 160 },
  { rgb_color,  73,  57, "steel-blue",              "#4682B4",  70, 130, 180 },
  { rgb_color,  74,  58, "corn-flower-blue",        "#6495ED", 100, 149, 237 },
  { rgb_color,  75,  59, "deep-sky-blue",           "#00BFFF",   0, 191, 255 },
  { rgb_color,  76,  60, "dodger-blue",             "#1E90FF",  30, 144, 255 },
  { rgb_color,  77,  61, "light-blue",              "#ADD8E6", 173, 216, 230 },
  { rgb_color,  78,  62, "sky-blue",                "#87CEEB", 135, 206, 235 },
  { rgb_color,  79,  63, "light-sky-blue",          "#87CEFA", 135, 206, 250 },
  { rgb_color,  80,  64, "midnight-blue",           "#191970",  25,  25, 112 },
  { rgb_color,  81,  65, "navy",                    "#000080",   0,   0, 128 },
  { rgb_color,  82,  66, "dark-blue",               "#00008B",   0,   0, 139 },
  { rgb_color,  83,  67, "medium-blue",             "#0000CD",   0,   0, 205 },
  { rgb_color,  84,  68, "blue",                    "#0000FF",   0,   0, 255 },
  { rgb_color,  85,  69, "royal-blue",              "#4169E1",  65, 105, 225 },
  { rgb_color,  86,  70, "blue-violet",             "#8A2BE2", 138,  43, 226 },
  { rgb_color,  87,  71, "indigo",                  "#4B0082",  75,   0, 130 },
  { rgb_color,  88,  72, "dark-slate-blue",         "#483D8B",  72,  61, 139 },
  { rgb_color,  89,  73, "slate-blue",              "#6A5ACD", 106,  90, 205 },
  { rgb_color,  90,  74, "medium-slate-blue",       "#7B68EE", 123, 104, 238 },
  { rgb_color,  91,  75, "medium-purple",           "#9370DB", 147, 112, 219 },
  { rgb_color,  92,  76, "dark-magenta",            "#8B008B", 139,   0, 139 },
  { rgb_color,  93,  77, "dark-violet",             "#9400D3", 148,   0, 211 },
  { rgb_color,  94,  78, "dark-orchid",             "#9932CC", 153,  50, 204 },
  { rgb_color,  95,  79, "medium-orchid",           "#BA55D3", 186,  85, 211 },
  { rgb_color,  96,  80, "purple",                  "#800080", 128,   0, 128 },
  { rgb_color,  97,  81, "thistle",                 "#D8BFD8", 216, 191, 216 },
  { rgb_color,  98,  82, "plum",                    "#DDA0DD", 221, 160, 221 },
  { rgb_color,  99,  83, "violet",                  "#EE82EE", 238, 130, 238 },
  { rgb_color, 100,  84, "magenta",                 "#FF00FF", 255,   0, 255 },
  { rgb_color, 101,  85, "orchid",                  "#DA70D6", 218, 112, 214 },
  { rgb_color, 102,  86, "medium-violet-red",       "#C71585", 199,  21, 133 },
  { rgb_color, 103,  87, "pale-violet-red",         "#DB7093", 219, 112, 147 },
  { rgb_color, 104,  88, "deep-pink",               "#FF1493", 255,  20, 147 },
  { rgb_color, 105,  89, "hot-pink",                "#FF69B4", 255, 105, 180 },
  { rgb_color, 106,  90, "light-pink",              "#FFB6C1", 255, 182, 193 },
  { rgb_color, 107,  91, "pink",                    "#FFC0CB", 255, 192, 203 },
  { rgb_color, 108,  92, "antique-white",           "#FAEBD7", 250, 235, 215 },
  { rgb_color, 109,  93, "beige",                   "#F5F5DC", 245, 245, 220 },
  { rgb_color, 110,  94, "bisque",                  "#FFE4C4", 255, 228, 196 },
  { rgb_color, 111,  95, "blanched-almond",         "#FFEBCD", 255, 235, 205 },
  { rgb_color, 112,  96, "wheat",                   "#F5DEB3", 245, 222, 179 },
  { rgb_color, 113,  97, "corn-silk",               "#FFF8DC", 255, 248, 220 },
  { rgb_color, 114,  98, "lemon-chiffon",           "#FFFACD", 255, 250, 205 },
  { rgb_color, 115,  99, "light-golden-rod-yellow", "#FAFAD2", 250, 250, 210 },
  { rgb_color, 116, 100, "light-yellow",            "#FFFFE0", 255, 255, 224 },
  { rgb_color, 117, 101, "saddle-brown",            "#8B4513", 139,  69,  19 },
  { rgb_color, 118, 102, "sienna",                  "#A0522D", 160,  82,  45 },
  { rgb_color, 119, 103, "chocolate",               "#D2691E", 210, 105,  30 },
  { rgb_color, 120, 104, "peru",                    "#CD853F", 205, 133,  63 },
  { rgb_color, 121, 105, "sandy-brown",             "#F4A460", 244, 164,  96 },
  { rgb_color, 122, 106, "burly-wood",              "#DEB887", 222, 184, 135 },
  { rgb_color, 123, 107, "tan",                     "#D2B48C", 210, 180, 140 },
  { rgb_color, 124, 108, "rosy-brown",              "#BC8F8F", 188, 143, 143 },
  { rgb_color, 125, 109, "moccasin",                "#FFE4B5", 255, 228, 181 },
  { rgb_color, 126, 110, "navajo-white",            "#FFDEAD", 255, 222, 173 },
  { rgb_color, 127, 111, "peach-puff",              "#FFDAB9", 255, 218, 185 },
  { rgb_color, 128, 112, "misty-rose",              "#FFE4E1", 255, 228, 225 },
  { rgb_color, 129, 113, "lavender-blush",          "#FFF0F5", 255, 240, 245 },
  { rgb_color, 130, 114, "linen",                   "#FAF0E6", 250, 240, 230 },
  { rgb_color, 131, 115, "old-lace",                "#FDF5E6", 253, 245, 230 },
  { rgb_color, 132, 116, "papaya-whip",             "#FFEFD5", 255, 239, 213 },
  { rgb_color, 133, 117, "sea-shell",               "#FFF5EE", 255, 245, 238 },
  { rgb_color, 134, 118, "mint-cream",              "#F5FFFA", 245, 255, 250 },
  { rgb_color, 135, 119, "slate-gray",              "#708090", 112, 128, 144 },
  { rgb_color, 136, 120, "light-slate-gray",        "#778899", 119, 136, 153 },
  { rgb_color, 137, 121, "light-steel-blue",        "#B0C4DE", 176, 196, 222 },
  { rgb_color, 138, 122, "lavender",                "#E6E6FA", 230, 230, 250 },
  { rgb_color, 139, 123, "floral-white",            "#FFFAF0", 255, 250, 240 },
  { rgb_color, 140, 124, "alice-blue",              "#F0F8FF", 240, 248, 255 },
  { rgb_color, 141, 125, "ghost-white",             "#F8F8FF", 248, 248, 255 },
  { rgb_color, 142, 126, "honeydew",                "#F0FFF0", 240, 255, 240 },
  { rgb_color, 143, 127, "ivory",                   "#FFFFF0", 255, 255, 240 },
  { rgb_color, 144, 128, "azure",                   "#F0FFFF", 240, 255, 255 },
  { rgb_color, 145, 129, "snow",                    "#FFFAFA", 255, 250, 250 },
  { rgb_color, 146, 130, "black",                   "#000000",   0,   0,   0 },
  { rgb_color, 147, 131, "dim-gray",                "#696969", 105, 105, 105 },
  { rgb_color, 148, 132, "gray",                    "#808080", 128, 128, 128 },
  { rgb_color, 149, 133, "dark-gray",               "#A9A9A9", 169, 169, 169 },
  { rgb_color, 150, 134, "silver",                  "#C0C0C0", 192, 192, 192 },
  { rgb_color, 151, 135, "light-gray",              "#D3D3D3", 211, 211, 211 },
  { rgb_color, 152, 136, "gainsboro",               "#DCDCDC", 220, 220, 220 },
  { rgb_color, 153, 137, "white-smoke",             "#F5F5F5", 245, 245, 245 },
  { rgb_color, 154, 138, "white",                   "#FFFFFF", 255, 255, 255 },
};

int32
colortable_to_int32(struct nethack_color *cte)
{
    int32 clr = NO_COLOR | NH_BASIC_COLOR;

    if (cte->colortyp == rgb_color)
        clr = (cte->r << 16) | (cte->g << 8) | cte->b;
    else if (cte->colortyp == nh_color)
        clr = cte->tableindex | NH_BASIC_COLOR;
    return clr;
}

char *
color_attr_to_str(color_attr *ca)
{
    static char buf[BUFSZ];

    Sprintf(buf, "%s&%s",
            clr2colorname(ca->color),
            attr2attrname(ca->attr));
    return buf;
}

/* parse string like "color&attr" into color_attr */
boolean
color_attr_parse_str(color_attr *ca, char *str)
{
    char buf[BUFSZ];
    char *amp = NULL;
    int tmp, c = NO_COLOR, a = ATR_NONE;

    (void) strncpy(buf, str, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';

    if ((amp = strchr(buf, '&')) != 0)
        *amp = '\0';

    if (amp) {
        amp++;
        c = match_str2clr(buf, FALSE);
        a = match_str2attr(amp, TRUE);
        /* FIXME: match_str2clr & match_str2attr give config_error_add(),
           so this is useless */
        if (c >= CLR_MAX && a == -1) {
            /* try other way around */
            c = match_str2clr(amp, FALSE);
            a = match_str2attr(buf, TRUE);
        }
        if (c >= CLR_MAX || a == -1)
            return FALSE;
    } else {
        /* one param only */
        tmp = match_str2attr(buf, FALSE);
        if (tmp == -1) {
            tmp = match_str2clr(buf, FALSE);
            if (tmp >= CLR_MAX)
                return FALSE;
            c = tmp;
        } else {
            a = tmp;
        }
    }
    ca->attr = a;
    ca->color = c;
    return TRUE;
}

boolean
query_color_attr(color_attr *ca, const char *prompt)
{
    int c, a;

    c = query_color(prompt, ca->color);
    if (c == -1)
        return FALSE;
    a = query_attr(prompt, ca->attr);
    if (a == -1)
        return FALSE;
    ca->color = c;
    ca->attr = a;
    return TRUE;
}

const char *
attr2attrname(int attr)
{
    int i;

    for (i = 0; i < SIZE(attrnames); i++)
        if (attrnames[i].attr == attr)
            return attrnames[i].name;
    return (char *) 0;
}

/*
 * Color support functions and data for "color"
 *
 * Used by: optfn_()
 *
 */

const char *
clr2colorname(int clr)
{
    int i;

    for (i = 0; i < SIZE(colornames); i++)
        if (colornames[i].name && colornames[i].color == clr)
            return colornames[i].name;
    return (char *) 0;
}

int
match_str2clr(char *str, boolean suppress_msg)
{
    int i, c = CLR_MAX;

    /* allow "lightblue", "light blue", and "light-blue" to match "light blue"
       (also junk like "_l i-gh_t---b l u e" but we won't worry about that);
       also copes with trailing space; caller has removed any leading space */
    for (i = 0; i < SIZE(colornames); i++)
        if (colornames[i].name
            && fuzzymatch(str, colornames[i].name, " -_", TRUE)) {
            c = colornames[i].color;
            break;
        }
    if (i == SIZE(colornames) && digit(*str))
        c = atoi(str);

    if (c < 0 || c >= CLR_MAX) {
        if (!suppress_msg)
            config_error_add("Unknown color '%.60s'", str);
        c = CLR_MAX; /* "none of the above" */
    }
    return c;
}

int
match_str2attr(const char *str, boolean complain)
{
    int i, a = -1;

    for (i = 0; i < SIZE(attrnames); i++)
        if (attrnames[i].name
            && fuzzymatch(str, attrnames[i].name, " -_", TRUE)) {
            a = attrnames[i].attr;
            break;
        }

    if (a == -1 && complain)
        config_error_add("Unknown text attribute '%.50s'", str);

    return a;
}

/* ask about highlighting attribute; for menu headers and menu
   coloring patterns, only one attribute at a time is allowed;
   for status highlighting, multiple attributes are allowed [overkill;
   life would be much simpler if that were restricted to one also...] */
int
query_attr(const char *prompt, int dflt_attr)
{
    winid tmpwin;
    anything any;
    int i, pick_cnt;
    menu_item *picks = (menu_item *) 0;
    boolean allow_many = (prompt && !strncmpi(prompt, "Choose", 6));
    int clr = NO_COLOR;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(attrnames); i++) {
        if (!attrnames[i].name)
            break;
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                 attrnames[i].attr, clr, attrnames[i].name,
                 (attrnames[i].attr == dflt_attr) ? MENU_ITEMFLAGS_SELECTED
                                                  : MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, (prompt && *prompt) ? prompt : "Pick an attribute");
    pick_cnt = select_menu(tmpwin, allow_many ? PICK_ANY : PICK_ONE, &picks);
    destroy_nhwindow(tmpwin);
    if (pick_cnt > 0) {
        int j, k = 0;

        if (allow_many) {
            /* PICK_ANY, with one preselected entry (ATR_NONE) which
               should be excluded if any other choices were picked */
            for (i = 0; i < pick_cnt; ++i) {
                j = picks[i].item.a_int - 1;
                if (attrnames[j].attr != ATR_NONE || pick_cnt == 1) {
                    switch (attrnames[j].attr) {
                    case ATR_NONE:
                        k = HL_NONE;
                        break;
                    case ATR_BOLD:
                        k |= HL_BOLD;
                        break;
                    case ATR_DIM:
                        k |= HL_DIM;
                        break;
                    case ATR_ITALIC:
                        k |= HL_ITALIC;
                        break;
                    case ATR_ULINE:
                        k |= HL_ULINE;
                        break;
                    case ATR_BLINK:
                        k |= HL_BLINK;
                        break;
                    case ATR_INVERSE:
                        k |= HL_INVERSE;
                        break;
                    }
                }
            }
        } else {
            /* PICK_ONE, but might get 0 or 2 due to preselected entry */
            j = picks[0].item.a_int - 1;
            /* pick_cnt==2: explicitly picked something other than the
               preselected entry */
            if (pick_cnt == 2 && attrnames[j].attr == dflt_attr)
                j = picks[1].item.a_int - 1;
            k = attrnames[j].attr;
        }
        free((genericptr_t) picks);
        return k;
    } else if (pick_cnt == 0 && !allow_many) {
        /* PICK_ONE, preselected entry explicitly chosen */
        return dflt_attr;
    }
    /* either ESC to explicitly cancel (pick_cnt==-1) or
       PICK_ANY with preselected entry toggled off and nothing chosen */
    return -1;
}

int
query_color(const char *prompt, int dflt_color)
{
    winid tmpwin;
    anything any;
    int i, pick_cnt;
    menu_item *picks = (menu_item *) 0;

    /* replace user patterns with color name ones and force 'menucolors' On */
    basic_menu_colors(TRUE);

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(colornames); i++) {
        if (!colornames[i].name)
            break;
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                 ATR_NONE, NO_COLOR, colornames[i].name,
                 (colornames[i].color == dflt_color) ? MENU_ITEMFLAGS_SELECTED
                                                     : MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, (prompt && *prompt) ? prompt : "Pick a color");
    pick_cnt = select_menu(tmpwin, PICK_ONE, &picks);
    destroy_nhwindow(tmpwin);

    /* remove temporary color name patterns and restore user-specified ones;
       reset 'menucolors' option to its previous value */
    basic_menu_colors(FALSE);

    if (pick_cnt > 0) {
        i = colornames[picks[0].item.a_int - 1].color;
        /* pick_cnt==2: explicitly picked something other than the
           preselected entry */
        if (pick_cnt == 2 && i == NO_COLOR)
            i = colornames[picks[1].item.a_int - 1].color;
        free((genericptr_t) picks);
        return i;
    } else if (pick_cnt == 0) {
        /* pick_cnt==0: explicitly picking preselected entry toggled it off */
        return dflt_color;
    }
    return -1;
}

DISABLE_WARNING_FORMAT_NONLITERAL

extern const char regex_id[]; /* from sys/share/<various>regex.{c,cpp} */

/* True: temporarily replace menu color entries with a fake set of menu
   colors, { "light blue"=light_blue, "blue"=blue, "red"=red, &c }, that
   illustrates most colors for use when the pick-a-color menu is rendered;
   suppresses black and white because one of those will likely be invisible
   due to matching the background; False: restore user-specified colorings */
void
basic_menu_colors(boolean load_colors)
{
    if (load_colors) {
        /* replace normal menu colors with a set specifically for colors */
        gs.save_menucolors = iflags.use_menu_color;
        gs.save_colorings = gm.menu_colorings;

        iflags.use_menu_color = TRUE;
        if (gc.color_colorings) {
            /* use the alternate colorings which were set up previously */
            gm.menu_colorings = gc.color_colorings;
        } else {
            /* create the alternate colorings once */
            char cnm[QBUFSZ];
            int i, c;
            boolean pmatchregex = !strcmpi(regex_id, "pmatchregex");
            const char *patternfmt = pmatchregex ? "*%s" : "%s";

            /* menu_colorings pointer has been saved; clear it in order
               to add the alternate entries as if from scratch */
            gm.menu_colorings = (struct menucoloring *) 0;

            /* this orders the patterns last-in/first-out; that means
               that the "light <foo>" variations come before the basic
               "<foo>" ones, which is exactly what we want */
            for (i = 0; i < SIZE(colornames); ++i) {
                if (!colornames[i].name) /* first alias entry has no name */
                    break;
                c = colornames[i].color;
                if (c == CLR_BLACK || c == CLR_WHITE || c == NO_COLOR)
                    continue; /* skip these */
                Sprintf(cnm, patternfmt, colornames[i].name);
                add_menu_coloring_parsed(cnm, c, ATR_NONE);
            }

            /* right now, menu_colorings contains the alternate color list;
               remember that list for future pick-a-color instances and
               also keep it as is for this instance */
            gc.color_colorings = gm.menu_colorings;
        }
    } else {
        /* restore normal user-specified menu colors */
        iflags.use_menu_color = gs.save_menucolors;
        gm.menu_colorings = gs.save_colorings;
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

boolean
add_menu_coloring_parsed(const char *str, int c, int a)
{
    static const char re_error[] = "Menucolor regex error";
    struct menucoloring *tmp;

    if (!str)
        return FALSE;
    tmp = (struct menucoloring *) alloc(sizeof *tmp);
    tmp->match = regex_init();
    /* test_regex_pattern() has already validated this regexp but parsing
       it again could conceivably run out of memory */
    if (!regex_compile(str, tmp->match)) {
        char errbuf[BUFSZ];
        char *re_error_desc = regex_error_desc(tmp->match, errbuf);

        /* free first in case reason for regcomp failure was out-of-memory */
        regex_free(tmp->match);
        free((genericptr_t) tmp);
        config_error_add("%s: %s", re_error, re_error_desc);
        return FALSE;
    }
    tmp->next = gm.menu_colorings;
    tmp->origstr = dupstr(str);
    tmp->color = c;
    tmp->attr = a;
    gm.menu_colorings = tmp;
    iflags.use_menu_color = TRUE;
    return TRUE;
}

/* parse '"regex_string"=color&attr' and add it to menucoloring */
boolean
add_menu_coloring(char *tmpstr) /* never Null but could be empty */
{
    int c = NO_COLOR, a = ATR_NONE;
    char *tmps, *cs, *amp;
    char str[BUFSZ];

    (void) strncpy(str, tmpstr, sizeof str - 1);
    str[sizeof str - 1] = '\0';

    if ((cs = strchr(str, '=')) == 0) {
        config_error_add("Malformed MENUCOLOR");
        return FALSE;
    }

    tmps = cs + 1; /* advance past '=' */
    mungspaces(tmps);
    if ((amp = strchr(tmps, '&')) != 0)
        *amp = '\0';

    c = match_str2clr(tmps, FALSE);
    if (c >= CLR_MAX)
        return FALSE;

    if (amp) {
        tmps = amp + 1; /* advance past '&' */
        a = match_str2attr(tmps, TRUE);
        if (a == -1)
            return FALSE;
    }

    /* the regexp portion here has not been condensed by mungspaces() */
    *cs = '\0';
    tmps = str;
    if (*tmps == '"' || *tmps == '\'') {
        cs--;
        while (isspace((uchar) *cs))
            cs--;
        if (*cs == *tmps) {
            *cs = '\0';
            tmps++;
        }
    }
    return add_menu_coloring_parsed(tmps, c, a);
}

/* release all menu color patterns */
void
free_menu_coloring(void)
{
    /* either menu_colorings or color_colorings or both might need to
       be freed or already be Null; do-loop will iterate at most twice */
    do {
        struct menucoloring *tmp, *tmp2;

        for (tmp = gm.menu_colorings; tmp; tmp = tmp2) {
            tmp2 = tmp->next;
            regex_free(tmp->match);
            free((genericptr_t) tmp->origstr);
            free((genericptr_t) tmp);
        }
        gm.menu_colorings = gc.color_colorings;
        gc.color_colorings = (struct menucoloring *) 0;
    } while (gm.menu_colorings);
}

/* release a specific menu color pattern; not used for color_colorings */
void
free_one_menu_coloring(int idx) /* 0 .. */
{
    struct menucoloring *tmp = gm.menu_colorings;
    struct menucoloring *prev = NULL;

    while (tmp) {
        if (idx == 0) {
            struct menucoloring *next = tmp->next;

            regex_free(tmp->match);
            free((genericptr_t) tmp->origstr);
            free((genericptr_t) tmp);
            if (prev)
                prev->next = next;
            else
                gm.menu_colorings = next;
            return;
        }
        idx--;
        prev = tmp;
        tmp = tmp->next;
    }
}

int
count_menucolors(void)
{
    struct menucoloring *tmp;
    int count = 0;

    for (tmp = gm.menu_colorings; tmp; tmp = tmp->next)
        count++;
    return count;
}

/* returns -1 on no-match.
 * buf is NONNULLARG1
 */
int32
check_enhanced_colors(char *buf)
{
    int32 retcolor = -1, color;

    if ((color = match_str2clr(buf, TRUE)) != CLR_MAX)  {
        retcolor = color | NH_BASIC_COLOR;
    } else {
        for (color = 0; color < SIZE(colortable); ++color) {
            if (!strcmpi(buf, colortable[color].name))
                retcolor = colortable_to_int32(&colortable[color]);
        }
    }
    return retcolor;
}



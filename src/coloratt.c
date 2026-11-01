/* NetHack 5.0	coloratt.c	$NHDT-Date: 1737286550 2025/01/19 03:35:50 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.14 $ */
/* Copyright (c) Pasi Kallinen, 2024 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

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

/* { colortyp, tableindex, rgbindx, name, r, g, b }, */

#define NHC nh_color
#define NOC no_color
#define RGBC rgb_color

const struct nethack_color colortable[] = {
    { NHC,    0,   0, "black", 0x00, 0x00, 0x00 },      /*        CLR_BLACK */
    { NHC,    1,   0, "red", 0xFF, 0x00, 0x00 },        /*          CLR_RED */
    { NHC,    2,   0, "green", 0x22, 0x8B, 0x22 },      /*        CLR_GREEN */
    { NHC,    3,   0, "brown", 0xA5, 0x2A, 0x2A },      /*        CLR_BROWN */
    { NHC,    4,   0, "blue", 0x00, 0x00, 0xFF },       /*         CLR_BLUE */
    { NHC,    5,   0, "magenta", 0xFF, 0x00, 0xFF },    /*      CLR_MAGENTA */
    { NHC,    6,   0, "cyan", 0x00, 0xFF, 0xFF },       /*         CLR_CYAN */
    { NHC,    7,   0, "gray", 0x80, 0x80, 0x80 },       /*         CLR_GRAY */
    { NOC,    8,   0, "nocolor", 0x00, 0x00, 0x00 },    /*         NO_COLOR */
    { NHC,    9,   0, "orange", 0xFF, 0xA5, 0x00 },     /*       CLR_ORANGE */
    { NHC,   10,   0, "bright-green",
                      0x00, 0x80, 0x00 },               /* CLR_BRIGHT_GREEN */
    { NHC,   11,   0, "yellow", 0xFF, 0xFF, 0x00 },     /*       CLR_YELLOW */
    { NHC,   12,   0, "bright-blue", 0xAD, 0xD8, 0xE6 }, /* CLR_BRIGHT_BLUE */
    { NHC,   13,   0, "bright-magenta",
                      0x93, 0x70, 0xDB },             /* CLR_BRIGHT_MAGENTA */
    { NHC,   14,   0, "light-cyan", 0xE0, 0xFF, 0xFF }, /*  CLR_BRIGHT_CYAN */
    { NHC,   15,   0, "white", 0xFF, 0xFF, 0xFF },      /*        CLR_WHITE */
    { RGBC,  16,   0, "maroon", 0x80, 0x00, 0x00 },              /* #800000 */
    { RGBC,  17,   1, "dark-red", 0x8B, 0x00, 0x00 },            /* #8B0000 */
    { RGBC,  18,   2, "brown", 0xA5, 0x2A, 0x2A },               /* #A52A2A */
    { RGBC,  19,   3, "firebrick", 0xB2, 0x22, 0x22 },           /* #B22222 */
    { RGBC,  20,   4, "crimson", 0xDC, 0x14, 0x3C },             /* #DC143C */
    { RGBC,  21,   5, "red", 0xFF, 0x00, 0x00 },                 /* #FF0000 */
    { RGBC,  22,   6, "tomato", 0xFF, 0x63, 0x47 },              /* #FF6347 */
    { RGBC,  23,   7, "coral", 0xFF, 0x7F, 0x50 },               /* #FF7F50 */
    { RGBC,  24,   8, "indian-red", 0xCD, 0x5C, 0x5C },          /* #CD5C5C */
    { RGBC,  25,   9, "light-coral", 0xF0, 0x80, 0x80 },         /* #F08080 */
    { RGBC,  26,  10, "dark-salmon", 0xE9, 0x96, 0x7A },         /* #E9967A */
    { RGBC,  27,  11, "salmon", 0xFA, 0x80, 0x72 },              /* #FA8072 */
    { RGBC,  28,  12, "light-salmon", 0xFF, 0xA0, 0x7A },        /* #FFA07A */
    { RGBC,  29,  13, "orange-red", 0xFF, 0x45, 0x00 },          /* #FF4500 */
    { RGBC,  30,  14, "dark-orange", 0xFF, 0x8C, 0x00 },         /* #FF8C00 */
    { RGBC,  31,  15, "orange", 0xFF, 0xA5, 0x00 },              /* #FFA500 */
    { RGBC,  32,  16, "gold", 0xFF, 0xD7, 0x00 },                /* #FFD700 */
    { RGBC,  33,  17, "dark-golden-rod", 0xB8, 0x86, 0x0B },     /* #B8860B */
    { RGBC,  34,  18, "golden-rod", 0xDA, 0xA5, 0x20 },          /* #DAA520 */
    { RGBC,  35,  19, "pale-golden-rod", 0xEE, 0xE8, 0xAA },     /* #EEE8AA */
    { RGBC,  36,  20, "dark-khaki", 0xBD, 0xB7, 0x6B },          /* #BDB76B */
    { RGBC,  37,  21, "khaki", 0xF0, 0xE6, 0x8C },               /* #F0E68C */
    { RGBC,  38,  22, "olive", 0x80, 0x80, 0x00 },               /* #808000 */
    { RGBC,  39,  23, "yellow", 0xFF, 0xFF, 0x00 },              /* #FFFF00 */
    { RGBC,  40,  24, "yellow-green", 0x9A, 0xCD, 0x32 },        /* #9ACD32 */
    { RGBC,  41,  25, "dark-olive-green", 0x55, 0x6B, 0x2F },    /* #556B2F */
    { RGBC,  42,  26, "olive-drab", 0x6B, 0x8E, 0x23 },          /* #6B8E23 */
    { RGBC,  43,  27, "lawn-green", 0x7C, 0xFC, 0x00 },          /* #7CFC00 */
    { RGBC,  44,  28, "chart-reuse", 0x7F, 0xFF, 0x00 },         /* #7FFF00 */
    { RGBC,  45,  29, "green-yellow", 0xAD, 0xFF, 0x2F },        /* #ADFF2F */
    { RGBC,  46,  30, "dark-green", 0x00, 0x64, 0x00 },          /* #006400 */
    { RGBC,  47,  31, "green", 0x00, 0x80, 0x00 },               /* #008000 */
    { RGBC,  48,  32, "forest-green", 0x22, 0x8B, 0x22 },        /* #228B22 */
    { RGBC,  49,  33, "lime", 0x00, 0xFF, 0x00 },                /* #00FF00 */
    { RGBC,  50,  34, "lime-green", 0x32, 0xCD, 0x32 },          /* #32CD32 */
    { RGBC,  51,  35, "light-green", 0x90, 0xEE, 0x90 },         /* #90EE90 */
    { RGBC,  52,  36, "pale-green", 0x98, 0xFB, 0x98 },          /* #98FB98 */
    { RGBC,  53,  37, "dark-sea-green", 0x8F, 0xBC, 0x8F },      /* #8FBC8F */
    { RGBC,  54,  38, "medium-spring-green", 0x00, 0xFA, 0x9A }, /* #00FA9A */
    { RGBC,  55,  39, "spring-green", 0x00, 0xFF, 0x7F },        /* #00FF7F */
    { RGBC,  56,  40, "sea-green", 0x2E, 0x8B, 0x57 },           /* #2E8B57 */
    { RGBC,  57,  41, "medium-aqua-marine", 0x66, 0xCD, 0xAA },  /* #66CDAA */
    { RGBC,  58,  42, "medium-sea-green", 0x3C, 0xB3, 0x71 },    /* #3CB371 */
    { RGBC,  59,  43, "light-sea-green", 0x20, 0xB2, 0xAA },     /* #20B2AA */
    { RGBC,  60,  44, "dark-slate-gray", 0x2F, 0x4F, 0x4F },     /* #2F4F4F */
    { RGBC,  61,  45, "teal", 0x00, 0x80, 0x80 },                /* #008080 */
    { RGBC,  62,  46, "dark-cyan", 0x00, 0x8B, 0x8B },           /* #008B8B */
    { RGBC,  63,  47, "aqua", 0x00, 0xFF, 0xFF },                /* #00FFFF */
    { RGBC,  64,  48, "cyan", 0x00, 0xFF, 0xFF },                /* #00FFFF */
    { RGBC,  65,  49, "light-cyan", 0xE0, 0xFF, 0xFF },          /* #E0FFFF */
    { RGBC,  66,  50, "dark-turquoise", 0x00, 0xCE, 0xD1 },      /* #00CED1 */
    { RGBC,  67,  51, "turquoise", 0x40, 0xE0, 0xD0 },           /* #40E0D0 */
    { RGBC,  68,  52, "medium-turquoise", 0x48, 0xD1, 0xCC },    /* #48D1CC */
    { RGBC,  69,  53, "pale-turquoise", 0xAF, 0xEE, 0xEE },      /* #AFEEEE */
    { RGBC,  70,  54, "aqua-marine", 0x7F, 0xFF, 0xD4 },         /* #7FFFD4 */
    { RGBC,  71,  55, "powder-blue", 0xB0, 0xE0, 0xE6 },         /* #B0E0E6 */
    { RGBC,  72,  56, "cadet-blue", 0x5F, 0x9E, 0xA0 },          /* #5F9EA0 */
    { RGBC,  73,  57, "steel-blue", 0x46, 0x82, 0xB4 },          /* #4682B4 */
    { RGBC,  74,  58, "corn-flower-blue", 0x64, 0x95, 0xED },    /* #6495ED */
    { RGBC,  75,  59, "deep-sky-blue", 0x00, 0xBF, 0xFF },       /* #00BFFF */
    { RGBC,  76,  60, "dodger-blue", 0x1E, 0x90, 0xFF },         /* #1E90FF */
    { RGBC,  77,  61, "light-blue", 0xAD, 0xD8, 0xE6 },          /* #ADD8E6 */
    { RGBC,  78,  62, "sky-blue", 0x87, 0xCE, 0xEB },            /* #87CEEB */
    { RGBC,  79,  63, "light-sky-blue", 0x87, 0xCE, 0xFA },      /* #87CEFA */
    { RGBC,  80,  64, "midnight-blue", 0x19, 0x19, 0x70 },       /* #191970 */
    { RGBC,  81,  65, "navy", 0x00, 0x00, 0x80 },                /* #000080 */
    { RGBC,  82,  66, "dark-blue", 0x00, 0x00, 0x8B },           /* #00008B */
    { RGBC,  83,  67, "medium-blue", 0x00, 0x00, 0xCD },         /* #0000CD */
    { RGBC,  84,  68, "blue", 0x00, 0x00, 0xFF },                /* #0000FF */
    { RGBC,  85,  69, "royal-blue", 0x41, 0x69, 0xE1 },          /* #4169E1 */
    { RGBC,  86,  70, "blue-violet", 0x8A, 0x2B, 0xE2 },         /* #8A2BE2 */
    { RGBC,  87,  71, "indigo", 0x4B, 0x00, 0x82 },              /* #4B0082 */
    { RGBC,  88,  72, "dark-slate-blue", 0x48, 0x3D, 0x8B },     /* #483D8B */
    { RGBC,  89,  73, "slate-blue", 0x6A, 0x5A, 0xCD },          /* #6A5ACD */
    { RGBC,  90,  74, "medium-slate-blue", 0x7B, 0x68, 0xEE },   /* #7B68EE */
    { RGBC,  91,  75, "medium-purple", 0x93, 0x70, 0xDB },       /* #9370DB */
    { RGBC,  92,  76, "dark-magenta", 0x8B, 0x00, 0x8B },        /* #8B008B */
    { RGBC,  93,  77, "dark-violet", 0x94, 0x00, 0xD3 },         /* #9400D3 */
    { RGBC,  94,  78, "dark-orchid", 0x99, 0x32, 0xCC },         /* #9932CC */
    { RGBC,  95,  79, "medium-orchid", 0xBA, 0x55, 0xD3 },       /* #BA55D3 */
    { RGBC,  96,  80, "purple", 0x80, 0x00, 0x80 },              /* #800080 */
    { RGBC,  97,  81, "thistle", 0xD8, 0xBF, 0xD8 },             /* #D8BFD8 */
    { RGBC,  98,  82, "plum", 0xDD, 0xA0, 0xDD },                /* #DDA0DD */
    { RGBC,  99,  83, "violet", 0xEE, 0x82, 0xEE },              /* #EE82EE */
    { RGBC, 100,  84, "magenta", 0xFF, 0x00, 0xFF },             /* #FF00FF */
    { RGBC, 101,  85, "orchid", 0xDA, 0x70, 0xD6 },              /* #DA70D6 */
    { RGBC, 102,  86, "medium-violet-red", 0xC7, 0x15, 0x85 },   /* #C71585 */
    { RGBC, 103,  87, "pale-violet-red", 0xDB, 0x70, 0x93 },     /* #DB7093 */
    { RGBC, 104,  88, "deep-pink", 0xFF, 0x14, 0x93 },           /* #FF1493 */
    { RGBC, 105,  89, "hot-pink", 0xFF, 0x69, 0xB4 },            /* #FF69B4 */
    { RGBC, 106,  90, "light-pink", 0xFF, 0xB6, 0xC1 },          /* #FFB6C1 */
    { RGBC, 107,  91, "pink", 0xFF, 0xC0, 0xCB },                /* #FFC0CB */
    { RGBC, 108,  92, "antique-white", 0xFA, 0xEB, 0xD7 },       /* #FAEBD7 */
    { RGBC, 109,  93, "beige", 0xF5, 0xF5, 0xDC },               /* #F5F5DC */
    { RGBC, 110,  94, "bisque", 0xFF, 0xE4, 0xC4 },              /* #FFE4C4 */
    { RGBC, 111,  95, "blanched-almond", 0xFF, 0xEB, 0xCD },     /* #FFEBCD */
    { RGBC, 112,  96, "wheat", 0xF5, 0xDE, 0xB3 },               /* #F5DEB3 */
    { RGBC, 113,  97, "corn-silk", 0xFF, 0xF8, 0xDC },           /* #FFF8DC */
    { RGBC, 114,  98, "lemon-chiffon", 0xFF, 0xFA, 0xCD },       /* #FFFACD */
    { RGBC, 115,  99, "light-golden-rod-yellow",
                      0xFA, 0xFA, 0xD2 },                        /* #FAFAD2 */
    { RGBC, 116, 100, "light-yellow", 0xFF, 0xFF, 0xE0 },        /* #FFFFE0 */
    { RGBC, 117, 101, "saddle-brown", 0x8B, 0x45, 0x13 },        /* #8B4513 */
    { RGBC, 118, 102, "sienna", 0xA0, 0x52, 0x2D },              /* #A0522D */
    { RGBC, 119, 103, "chocolate", 0xD2, 0x69, 0x1E },           /* #D2691E */
    { RGBC, 120, 104, "peru", 0xCD, 0x85, 0x3F },                /* #CD853F */
    { RGBC, 121, 105, "sandy-brown", 0xF4, 0xA4, 0x60 },         /* #F4A460 */
    { RGBC, 122, 106, "burly-wood", 0xDE, 0xB8, 0x87 },          /* #DEB887 */
    { RGBC, 123, 107, "tan", 0xD2, 0xB4, 0x8C },                 /* #D2B48C */
    { RGBC, 124, 108, "rosy-brown", 0xBC, 0x8F, 0x8F },          /* #BC8F8F */
    { RGBC, 125, 109, "moccasin", 0xFF, 0xE4, 0xB5 },            /* #FFE4B5 */
    { RGBC, 126, 110, "navajo-white", 0xFF, 0xDE, 0xAD },        /* #FFDEAD */
    { RGBC, 127, 111, "peach-puff", 0xFF, 0xDA, 0xB9 },          /* #FFDAB9 */
    { RGBC, 128, 112, "misty-rose", 0xFF, 0xE4, 0xE1 },          /* #FFE4E1 */
    { RGBC, 129, 113, "lavender-blush", 0xFF, 0xF0, 0xF5 },      /* #FFF0F5 */
    { RGBC, 130, 114, "linen", 0xFA, 0xF0, 0xE6 },               /* #FAF0E6 */
    { RGBC, 131, 115, "old-lace", 0xFD, 0xF5, 0xE6 },            /* #FDF5E6 */
    { RGBC, 132, 116, "papaya-whip", 0xFF, 0xEF, 0xD5 },         /* #FFEFD5 */
    { RGBC, 133, 117, "sea-shell", 0xFF, 0xF5, 0xEE },           /* #FFF5EE */
    { RGBC, 134, 118, "mint-cream", 0xF5, 0xFF, 0xFA },          /* #F5FFFA */
    { RGBC, 135, 119, "slate-gray", 0x70, 0x80, 0x90 },          /* #708090 */
    { RGBC, 136, 120, "light-slate-gray", 0x77, 0x88, 0x99 },    /* #778899 */
    { RGBC, 137, 121, "light-steel-blue", 0xB0, 0xC4, 0xDE },    /* #B0C4DE */
    { RGBC, 138, 122, "lavender", 0xE6, 0xE6, 0xFA },            /* #E6E6FA */
    { RGBC, 139, 123, "floral-white", 0xFF, 0xFA, 0xF0 },        /* #FFFAF0 */
    { RGBC, 140, 124, "alice-blue", 0xF0, 0xF8, 0xFF },          /* #F0F8FF */
    { RGBC, 141, 125, "ghost-white", 0xF8, 0xF8, 0xFF },         /* #F8F8FF */
    { RGBC, 142, 126, "honeydew", 0xF0, 0xFF, 0xF0 },            /* #F0FFF0 */
    { RGBC, 143, 127, "ivory", 0xFF, 0xFF, 0xF0 },               /* #FFFFF0 */
    { RGBC, 144, 128, "azure", 0xF0, 0xFF, 0xFF },               /* #F0FFFF */
    { RGBC, 145, 129, "snow", 0xFF, 0xFA, 0xFA },                /* #FFFAFA */
    { RGBC, 146, 130, "black", 0x00, 0x00, 0x00 },               /* #000000 */
    { RGBC, 147, 131, "dim-gray", 0x69, 0x69, 0x69 },            /* #696969 */
    { RGBC, 148, 132, "gray", 0x80, 0x80, 0x80 },                /* #808080 */
    { RGBC, 149, 133, "dark-gray", 0xA9, 0xA9, 0xA9 },           /* #A9A9A9 */
    { RGBC, 150, 134, "silver", 0xC0, 0xC0, 0xC0 },              /* #C0C0C0 */
    { RGBC, 151, 135, "light-gray", 0xD3, 0xD3, 0xD3 },          /* #D3D3D3 */
    { RGBC, 152, 136, "gainsboro", 0xDC, 0xDC, 0xDC },           /* #DCDCDC */
    { RGBC, 153, 137, "white-smoke", 0xF5, 0xF5, 0xF5 },         /* #F5F5F5 */
    { RGBC, 154, 138, "white", 0xFF, 0xFF, 0xFF },               /* #FFFFFF */
};

#undef NHC
#undef NOC
#undef RGBC

#ifdef CHANGE_COLOR
staticfn int32 alt_color_spec(const char *cp);
#endif

int32
colortable_to_int32(const struct nethack_color *cte)
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

/* set up a menu for picking a color, one that shows each name in its color;
   overrides player's MENUCOLORS with a set of "blue"=blue, "red"=red, and
   so forth; suppresses color for black and white because one of those will
   likely be invisible due to matching the background; the alternate set of
   MENUCOLORS is kept around for potential re-use */
void
basic_menu_colors(
    boolean load_colors) /* True: temporarily replace menu color entries with
                          * a fake set of menu colors which match their names;
                          * False: restore user-specified colorings */
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
               "<foo>" ones, which is exactly what we want (so that the
               shorter basic names won't get false matches as substrings
               of the longer ones) */
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
    char xtra = '\0'; /* used to catch trailing junk after "#rrggbb" */
    unsigned r, g, b;
    int32 retcolor = -1, color;

    if ((color = match_str2clr(buf, TRUE)) != CLR_MAX)  {
        retcolor = color | NH_BASIC_COLOR;
    } else if (sscanf(buf, "#%02x%02x%02x%c", &r, &g, &b, &xtra) >= 3) {
        retcolor = !xtra ? (int32) ((r << 16) | (g << 8) | b) : -1;
    } else {
        /* altbuf: allow user's "grey" to match colortable[]'s "gray";
         * fuzzymatch(): ignore spaces, hyphens, and underscores so that
         * space or underscore in user-supplied name will match hyphen
         * [note: caller splits text at spaces so we won't see any here]
         */
        char *altbuf = NULL, *grey = strstri(buf, "grey");
        ptrdiff_t greyoffset = grey ? (grey - buf) : -1;

        if (greyoffset >= 0) {
            altbuf = dupstr(buf);
            /* use direct copy because strsubst() is case-sensitive */
            /*(void) strncpy(&altbuf[greyoffset], "gray", 4);*/
            (void) memcpy(altbuf + greyoffset, "gray", 4);
        }
        for (color = 0; color < SIZE(colortable); ++color) {
            if (fuzzymatch(buf, colortable[color].name, " -_", TRUE)
                || (altbuf && fuzzymatch(altbuf, colortable[color].name,
                                         " -_", TRUE))) {
                retcolor = colortable_to_int32(&colortable[color]);
                break;
            }
        }
        if (altbuf)
            free(altbuf);
    }
    return retcolor;
}

/* return the canonical name of a particular color */
const char *
wc_color_name(int32 colorindx)
{
    static char hexcolor[sizeof "#rrggbb"]; /* includes room for '\0' */
    const char *result = "no-color";

    if (colorindx >= 0) {
        int32 basicindx = colorindx & ~NH_BASIC_COLOR;

        /* if colorindx has NH_BASIC_COLOR bit set, basicindx won't,
           so differing implies a basic color */
        if (basicindx != colorindx) {
            assert(basicindx < 16);
            result = colortable[basicindx].name;
        } else {
            int indx;
            long r = (colorindx >> 16) & 0x0000ff, /* shift rrXXXX to rr */
                 g = (colorindx >> 8) & 0x0000ff,  /* shift XXggXX to gg */
                 b = colorindx & 0x0000ff;         /* mask  XXXXbb to bb */

            Snprintf(hexcolor, sizeof hexcolor, "#%02x%02x%02x",
                     (uint8) r, (uint8) g, (uint8) b);
            result = hexcolor;
            /* override hex value if this is a named color */
            for (indx = 16; indx < SIZE(colortable); ++indx)
                if (colortable[indx].r == r
                    && colortable[indx].g == g
                    && colortable[indx].b == b) {
                    result = colortable[indx].name;
                    break;
                }
        }
    }
    return result;
}

/* hexdd[] is defined in decl.c */
boolean
onlyhexdigits(const char *buf)
{
    const char *dp = buf;

    for (dp = buf; *dp; ++dp) {
        if (!(strchr(hexdd, *dp) || *dp == '-'))
            return FALSE;
    }
    return TRUE;
}

int32_t
rgbstr_to_int32(const char *rgbstr)
{
    int r, g, b, milestone = 0;
    char *cp, *c_r, *c_g, *c_b;
    int32_t rgb = 0;
    char buf[BUFSZ];
    boolean dash = FALSE;


    Snprintf(buf, sizeof buf, "%s",
             rgbstr ? rgbstr : "");

    if (*buf && onlyhexdigits(buf)) {
        c_g = c_b = (char *) 0;
        c_r = cp = buf;
        while (*cp) {
            if (digit(*cp) || *cp == '-') {
                if (*cp == '-') {
                    *cp = '\0';
                    milestone++;
                    dash = TRUE;
                }
                cp++;
                if (dash) {
                    if (milestone < 2)
                        c_g = cp;
                    else
                        c_b = cp;
                    dash = FALSE;
                }
            } else {
                return -1L;
            }
        }
        /* sanity checks */
        if (c_r && c_g && c_b
            && (strlen(c_r) > 0 && strlen(c_r) < 4)
            && (strlen(c_g) > 0 && strlen(c_g) < 4)
            && (strlen(c_b) > 0 && strlen(c_b) < 4)) {
            r = atoi(c_r);
            g = atoi(c_g);
            b = atoi(c_b);
            rgb = (r << 16) | (g << 8) | (b << 0);
            return rgb;
        }
    } else if (*buf) {
        /* perhaps an enhanced color name was used instead of rgb value? */
        if ((rgb = check_enhanced_colors(buf)) != -1) {
            return rgb;
        }
    }
    return -1;
}

int
set_map_customcolor(glyph_map *gmap, uint32 nhcolor)
{
    glyph_map *tmpgm = gmap;
    uint32 closecolor = 0;
    uint16 clridx = 0;

    if (!tmpgm)
        return 0;

    gmap->customcolor = nhcolor;
    if (closest_color(nhcolor, &closecolor, &clridx))
        gmap->color256idx = clridx;
    else
        gmap->color256idx = 0;
    return 1;
}

static struct {
    int index;
    uint32 value;
} color_256_definitions[] = {
    /* color values are from unnethack */
    {  16, 0x000000 }, {  17, 0x00005f }, {  18, 0x000087 },
    {  19, 0x0000af }, {  20, 0x0000d7 }, {  21, 0x0000ff },
    {  22, 0x005f00 }, {  23, 0x005f5f }, {  24, 0x005f87 },
    {  25, 0x005faf }, {  26, 0x005fd7 }, {  27, 0x005fff },
    {  28, 0x008700 }, {  29, 0x00875f }, {  30, 0x008787 },
    {  31, 0x0087af }, {  32, 0x0087d7 }, {  33, 0x0087ff },
    {  34, 0x00af00 }, {  35, 0x00af5f }, {  36, 0x00af87 },
    {  37, 0x00afaf }, {  38, 0x00afd7 }, {  39, 0x00afff },
    {  40, 0x00d700 }, {  41, 0x00d75f }, {  42, 0x00d787 },
    {  43, 0x00d7af }, {  44, 0x00d7d7 }, {  45, 0x00d7ff },
    {  46, 0x00ff00 }, {  47, 0x00ff5f }, {  48, 0x00ff87 },
    {  49, 0x00ffaf }, {  50, 0x00ffd7 }, {  51, 0x00ffff },
    {  52, 0x5f0000 }, {  53, 0x5f005f }, {  54, 0x5f0087 },
    {  55, 0x5f00af }, {  56, 0x5f00d7 }, {  57, 0x5f00ff },
    {  58, 0x5f5f00 }, {  59, 0x5f5f5f }, {  60, 0x5f5f87 },
    {  61, 0x5f5faf }, {  62, 0x5f5fd7 }, {  63, 0x5f5fff },
    {  64, 0x5f8700 }, {  65, 0x5f875f }, {  66, 0x5f8787 },
    {  67, 0x5f87af }, {  68, 0x5f87d7 }, {  69, 0x5f87ff },
    {  70, 0x5faf00 }, {  71, 0x5faf5f }, {  72, 0x5faf87 },
    {  73, 0x5fafaf }, {  74, 0x5fafd7 }, {  75, 0x5fafff },
    {  76, 0x5fd700 }, {  77, 0x5fd75f }, {  78, 0x5fd787 },
    {  79, 0x5fd7af }, {  80, 0x5fd7d7 }, {  81, 0x5fd7ff },
    {  82, 0x5fff00 }, {  83, 0x5fff5f }, {  84, 0x5fff87 },
    {  85, 0x5fffaf }, {  86, 0x5fffd7 }, {  87, 0x5fffff },
    {  88, 0x870000 }, {  89, 0x87005f }, {  90, 0x870087 },
    {  91, 0x8700af }, {  92, 0x8700d7 }, {  93, 0x8700ff },
    {  94, 0x875f00 }, {  95, 0x875f5f }, {  96, 0x875f87 },
    {  97, 0x875faf }, {  98, 0x875fd7 }, {  99, 0x875fff },
    { 100, 0x878700 }, { 101, 0x87875f }, { 102, 0x878787 },
    { 103, 0x8787af }, { 104, 0x8787d7 }, { 105, 0x8787ff },
    { 106, 0x87af00 }, { 107, 0x87af5f }, { 108, 0x87af87 },
    { 109, 0x87afaf }, { 110, 0x87afd7 }, { 111, 0x87afff },
    { 112, 0x87d700 }, { 113, 0x87d75f }, { 114, 0x87d787 },
    { 115, 0x87d7af }, { 116, 0x87d7d7 }, { 117, 0x87d7ff },
    { 118, 0x87ff00 }, { 119, 0x87ff5f }, { 120, 0x87ff87 },
    { 121, 0x87ffaf }, { 122, 0x87ffd7 }, { 123, 0x87ffff },
    { 124, 0xaf0000 }, { 125, 0xaf005f }, { 126, 0xaf0087 },
    { 127, 0xaf00af }, { 128, 0xaf00d7 }, { 129, 0xaf00ff },
    { 130, 0xaf5f00 }, { 131, 0xaf5f5f }, { 132, 0xaf5f87 },
    { 133, 0xaf5faf }, { 134, 0xaf5fd7 }, { 135, 0xaf5fff },
    { 136, 0xaf8700 }, { 137, 0xaf875f }, { 138, 0xaf8787 },
    { 139, 0xaf87af }, { 140, 0xaf87d7 }, { 141, 0xaf87ff },
    { 142, 0xafaf00 }, { 143, 0xafaf5f }, { 144, 0xafaf87 },
    { 145, 0xafafaf }, { 146, 0xafafd7 }, { 147, 0xafafff },
    { 148, 0xafd700 }, { 149, 0xafd75f }, { 150, 0xafd787 },
    { 151, 0xafd7af }, { 152, 0xafd7d7 }, { 153, 0xafd7ff },
    { 154, 0xafff00 }, { 155, 0xafff5f }, { 156, 0xafff87 },
    { 157, 0xafffaf }, { 158, 0xafffd7 }, { 159, 0xafffff },
    { 160, 0xd70000 }, { 161, 0xd7005f }, { 162, 0xd70087 },
    { 163, 0xd700af }, { 164, 0xd700d7 }, { 165, 0xd700ff },
    { 166, 0xd75f00 }, { 167, 0xd75f5f }, { 168, 0xd75f87 },
    { 169, 0xd75faf }, { 170, 0xd75fd7 }, { 171, 0xd75fff },
    { 172, 0xd78700 }, { 173, 0xd7875f }, { 174, 0xd78787 },
    { 175, 0xd787af }, { 176, 0xd787d7 }, { 177, 0xd787ff },
    { 178, 0xd7af00 }, { 179, 0xd7af5f }, { 180, 0xd7af87 },
    { 181, 0xd7afaf }, { 182, 0xd7afd7 }, { 183, 0xd7afff },
    { 184, 0xd7d700 }, { 185, 0xd7d75f }, { 186, 0xd7d787 },
    { 187, 0xd7d7af }, { 188, 0xd7d7d7 }, { 189, 0xd7d7ff },
    { 190, 0xd7ff00 }, { 191, 0xd7ff5f }, { 192, 0xd7ff87 },
    { 193, 0xd7ffaf }, { 194, 0xd7ffd7 }, { 195, 0xd7ffff },
    { 196, 0xff0000 }, { 197, 0xff005f }, { 198, 0xff0087 },
    { 199, 0xff00af }, { 200, 0xff00d7 }, { 201, 0xff00ff },
    { 202, 0xff5f00 }, { 203, 0xff5f5f }, { 204, 0xff5f87 },
    { 205, 0xff5faf }, { 206, 0xff5fd7 }, { 207, 0xff5fff },
    { 208, 0xff8700 }, { 209, 0xff875f }, { 210, 0xff8787 },
    { 211, 0xff87af }, { 212, 0xff87d7 }, { 213, 0xff87ff },
    { 214, 0xffaf00 }, { 215, 0xffaf5f }, { 216, 0xffaf87 },
    { 217, 0xffafaf }, { 218, 0xffafd7 }, { 219, 0xffafff },
    { 220, 0xffd700 }, { 221, 0xffd75f }, { 222, 0xffd787 },
    { 223, 0xffd7af }, { 224, 0xffd7d7 }, { 225, 0xffd7ff },
    { 226, 0xffff00 }, { 227, 0xffff5f }, { 228, 0xffff87 },
    { 229, 0xffffaf }, { 230, 0xffffd7 }, { 231, 0xffffff },
    { 232, 0x080808 }, { 233, 0x121212 }, { 234, 0x1c1c1c },
    { 235, 0x262626 }, { 236, 0x303030 }, { 237, 0x3a3a3a },
    { 238, 0x444444 }, { 239, 0x4e4e4e }, { 240, 0x585858 },
    { 241, 0x626262 }, { 242, 0x6c6c6c }, { 243, 0x767676 },
    { 244, 0x808080 }, { 245, 0x8a8a8a }, { 246, 0x949494 },
    { 247, 0x9e9e9e }, { 248, 0xa8a8a8 }, { 249, 0xb2b2b2 },
    { 250, 0xbcbcbc }, { 251, 0xc6c6c6 }, { 252, 0xd0d0d0 },
    { 253, 0xdadada }, { 254, 0xe4e4e4 }, { 255, 0xeeeeee },
};

/** Calculate the color distance between two colors.
 *
 * Algorithm taken from UnNetHack which took it from
 * https://www.compuphase.com/cmetric.htm
 **/

int
color_distance(uint32_t rgb1, uint32_t rgb2)
{
    int r1 = (rgb1 >> 16) & 0xFF;
    int g1 = (rgb1 >> 8) & 0xFF;
    int b1 = (rgb1) & 0xFF;
    int r2 = (rgb2 >> 16) & 0xFF;
    int g2 = (rgb2 >> 8) & 0xFF;
    int b2 = (rgb2) & 0xFF;

    int rmean = (r1 + r2) / 2;
    int r = r1 - r2;
    int g = g1 - g2;
    int b = b1 - b2;
    return ((((512 + rmean) * r * r) >> 8) + 4 * g * g
            + (((767 - rmean) * b * b) >> 8));
}

boolean
closest_color(uint32 lcolor, uint32 *closecolor, uint16 *clridx)
{
    int i, color_index = -1, similar = INT_MAX, current;
    boolean retbool = FALSE;

    for (i = 0; i < SIZE(color_256_definitions); i++) {
        /* look for an exact match */
        if (lcolor == color_256_definitions[i].value) {
            color_index = i;
            break;
        }
        /* find a close color match */
        current = color_distance(lcolor, color_256_definitions[i].value);
        if (current < similar) {
            color_index = i;
            similar = current;
        }
    }
    if (closecolor && clridx && color_index >= 0) {
        *closecolor = color_256_definitions[color_index].value;
        *clridx = color_256_definitions[color_index].index;
        retbool = TRUE;
    }
    return retbool;
}

uint32
get_nhcolor_from_256_index(int idx)
{
    uint32 retcolor = NO_COLOR | NH_BASIC_COLOR;

    if (IndexOk(idx, color_256_definitions))
        retcolor = color_256_definitions[idx].value;
    return retcolor;
}

#ifdef CHANGE_COLOR

int
count_alt_palette(void)
{
    int clr, clrcount = 0;

    for (clr = 0; clr < CLR_MAX; ++clr) {
        if (ga.altpalette[clr] != 0U)
            clrcount++;
    }
    return clrcount;
}

int
alternative_palette(char *op)
{
    char buf[BUFSZ], *c_colorid, *c_colorval, *cp;
    int reslt = 0, coloridx = CLR_MAX;
    long rgb = 0L;
    boolean slash = FALSE;

    if (!op)
        return 0;

    Snprintf(buf, sizeof buf, "%s", op);
    c_colorval = (char *) 0;
    c_colorid = cp = buf;
    while (*cp) {
        if (*cp == ':' || *cp == '/') {
            if (*cp == '/') {
                slash = TRUE;
                *cp = '\0';
            }
        }
        cp++;
        if (slash) {
            c_colorval = cp;
            slash = FALSE;
        }
    }
    /* some sanity checks */
    if (c_colorid && *c_colorid == ' ')
        c_colorid++;
    if (c_colorval && *c_colorval == ' ')
        c_colorval++;
    if (c_colorid)
        coloridx = match_str2clr(c_colorid, TRUE);

    if (c_colorval && coloridx >= 0 && coloridx < CLR_MAX) {
        rgb = rgbstr_to_int32(c_colorval);
        if (rgb == -1) {
            rgb = alt_color_spec(c_colorval);
        }
        if (rgb != -1) {
            ga.altpalette[coloridx] = (uint32) rgb | NH_ALTPALETTE;
            /* use COLORVAL(ga.altpalette[coloridx]) to get
               the actual rgb value out of ga.altpalette[] */
            reslt = 1;
        }
    }
    return reslt;
}

void
change_palette(void)
{
    int clridx;

    for (clridx = 0; clridx < CLR_MAX; ++clridx) {
        if (ga.altpalette[clridx] != 0) {
            long rgb = (long) COLORVAL(ga.altpalette[clridx]);
            (*windowprocs.win_change_color)(clridx, rgb, 0);
        }
    }
}

staticfn int32
alt_color_spec(const char *str)
{
    static NEARDATA const char oct[] = "01234567", dec[] = "0123456789";
    /* hexdd[] is defined in decl.c */

    const char *dp, *cp = str;
    int32 cval = -1;
    int dcount, dlimit = 6;
    boolean hexescape = FALSE, octescape = FALSE;

    dcount = 0; /* for decimal, octal, hexadecimal cases */
    hexescape =
        (*cp == '\\' && cp[1] && (cp[1] == 'x' || cp[1] == 'X') && cp[2]);
    if (!hexescape) {
        octescape =
            (*cp == '\\' && cp[1] && (cp[1] == 'o' || cp[1] == 'O') && cp[2]);
    }

    if (hexescape || octescape) {
        cval = 0;
        cp += 2;
        if (octescape)
            dlimit = 8;
    } else if (*cp == '#' && cp[1]) {
        hexescape = TRUE;
        cval = 0;
        cp += 1;
    } else if (cp[1]) {
        cval = 0;
        dlimit = 8;
    } else if (!cp[1]) {
        if (strchr(dec, *cp) != 0) {
            /* simple val, or nothing left for \ to escape */
            cval = (*cp - '0');
        }
        dlimit = 1;
        cp++;
    }

    while (*cp) {
        if (!hexescape && !octescape && strchr(dec, *cp)) {
            cval = (cval * 10) + (*cp - '0');
        } else if (octescape && strchr(oct, *cp)) {
            cval = (cval * 8) + (*cp - '0');
        } else if (hexescape && (dp = strchr(hexdd, *cp)) != 0) {
            cval = (cval * 16) + ((int) (dp - hexdd) / 2);
        }
        ++cp;
        if (++dcount > dlimit) {
            cval = -1;
            break;
        }
    }
    return cval;
}
#endif /* CHANGE_COLOR */

/*coloratt.c*/

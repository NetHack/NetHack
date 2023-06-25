/* NetHack 3.7  colors.c */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef WIN32
#include "winos.h"
#endif
#include "color.h"
#include "hack.h"
#include "wintty.h"
#include <string.h>

RGB DefPalette[CLR_MAX+1];

struct named_color {
    const char* name;
    RGB val;
} stdclr[] = {
// name, b, g, r
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

int num_named_colors = sizeof(stdclr)/sizeof(stdclr[0]);

RGB
*stdclrval(const char* name)
{
    for(int i = 0; i < num_named_colors; i++)
    {
        if(strcmp(name, stdclr[i].name) == 0)
            return &stdclr[i].val;
    }
    if (DefPalette[0].g)
        return &DefPalette[0];
    else
        return stdclrval("gray");
}

#ifdef TTY_GRAPHICS
void
set_palette(void)
{
    // foreground
    printf("\033]10;rgb:%0x/%0x/%0x\033\\",
        DefPalette[0].r, DefPalette[0].g, DefPalette[0].b);

    // background
    printf("\033]11;rgb:%0x/%0x/%0x\033\\",
        DefPalette[17].r, DefPalette[17].g, DefPalette[17].b);

    // black
    printf("\033]4;0;rgb:%0x/%0x/%0x\033\\",
        DefPalette[16].r, DefPalette[16].g, DefPalette[16].b);

    for(int i = 1; i < 16; i++)
    {
        printf("\033]4;%d;rgb:%0x/%0x/%0x\033\\", 
            i, DefPalette[i].r, DefPalette[i].g, DefPalette[i].b);
    }
}

void
reset_palette(void)
{
    printf("\033]104\033\\");
    printf("\033]110\033\\");
    printf("\033]111\033\\");
}
#endif // TTY_GRAPHICS

void
init_default_palette(void)
{
    iflags.wc2_black = 1;
    RGB black = { 0x44, 0x40, 0x44 };

    DefPalette[0] = *stdclrval("darkgray");
    DefPalette[1] = *stdclrval("firebrick");
    DefPalette[2] = *stdclrval("forestgreen");
    DefPalette[3] = *stdclrval("sienna");
    DefPalette[4] = *stdclrval("royalblue");
    DefPalette[5] = *stdclrval("mediumpurple");
    DefPalette[6] = *stdclrval("darkcyan");
    DefPalette[7] = *stdclrval("lightsteelblue");
    DefPalette[8] = *stdclrval("dimgray");
    DefPalette[9] = *stdclrval("darkorange");
    DefPalette[10] = *stdclrval("olivedrab");
    DefPalette[11] = *stdclrval("gold");
    DefPalette[12] = *stdclrval("dodgerblue");
    DefPalette[13] = *stdclrval("orchid");
    DefPalette[14] = *stdclrval("seagreen");
    DefPalette[15] = *stdclrval("ivory");
    DefPalette[16] = black;
    DefPalette[17] = *stdclrval("black");

    set_palette();
}

//eof colors.c

/* NetHack 5.0	macstat.c	*/
/* NetHack may be freely redistributed.  See license for details. */

/* Native status renderer: draws the two styled status lines into the
 * shared _mt_window, per-field color/attrs from the core's STATUS_HILITES
 * rules, colors via mac_set_text_color (palette-safe).
 */

#include "hack.h"
#include "macwin.h"

/* genl field tables (src/windows.c); allocated by genl_status_init() */
extern const char *status_fieldfmt[MAXBLSTATS];
extern const char *status_fieldnm[MAXBLSTATS];
extern char *status_vals[MAXBLSTATS];
extern boolean status_activefields[MAXBLSTATS];

extern WindowPtr _mt_window; /* mttymain.c */

static Boolean stat_inited = false;
static int stat_colors[MAXBLSTATS];     /* CLR_* | (HL_* << 8) per field */
static unsigned long stat_cond_bits = 0UL;
static unsigned long *stat_colormasks = (unsigned long *) 0;

static void stat_draw_str(const char *, int, short, short);
static void stat_draw_conds(short, short);
static int stat_condcolor(unsigned long, unsigned long *);
static int stat_condattr(unsigned long, unsigned long *);

/* same two-line layout as genl_status_update's default fieldorder */
static const enum statusfields fieldorder[2][15] = {
    { BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_ALIGN,
      BL_SCORE, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH,
      BL_FLUSH },
    { BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX, BL_AC,
      BL_XP, BL_EXP, BL_HD, BL_TIME, BL_HUNGER, BL_CAP, BL_CONDITION,
      BL_FLUSH },
};

/* same conditions, same order, as genl_status_update */
static const struct {
    unsigned long mask;
    const char *text;
} cond_list[] = {
    { BL_MASK_STONE,    " Stone" },     { BL_MASK_SLIME,    " Slime" },
    { BL_MASK_STRNGL,   " Strngl" },    { BL_MASK_FOODPOIS, " FoodPois" },
    { BL_MASK_TERMILL,  " TermIll" },   { BL_MASK_BLIND,    " Blind" },
    { BL_MASK_DEAF,     " Deaf" },      { BL_MASK_STUN,     " Stun" },
    { BL_MASK_CONF,     " Conf" },      { BL_MASK_HALLU,    " Hallu" },
    { BL_MASK_LEV,      " Lev" },       { BL_MASK_FLY,      " Fly" },
    { BL_MASK_RIDE,     " Ride" },
};

/* TRUE once mac_status_init has run and WIN_STATUS is bound to _mt_window;
   gates the update-event/flush rerouting in macwin.c */
Boolean
macstat_active(void)
{
    return stat_inited && WIN_STATUS >= 0 && WIN_STATUS < NUM_MACWINDOWS
           && theWindows
           && theWindows[WIN_STATUS].its_window == _mt_window;
}

void
mac_status_init(void)
{
    int i;

    for (i = 0; i < MAXBLSTATS; ++i)
        stat_colors[i] = NO_COLOR;
    stat_cond_bits = 0UL;
    stat_colormasks = (unsigned long *) 0;
    /* genl_status_init allocates the field tables and creates+displays
       WIN_STATUS (the mac create path binds it to _mt_window) */
    genl_status_init();
    stat_inited = true;
}

void
mac_status_finish(void)
{
    stat_inited = false;
    genl_status_finish(); /* frees status_vals[]; WIN_STATUS stays set but
                             stat_inited=false blocks macstat_active() */
}

void
mac_status_enablefield(
    int fieldidx, const char *nm, const char *fmt, boolean enable)
{
    genl_status_enablefield(fieldidx, nm, fmt, enable);
}

/* cache one field (or render on BL_FLUSH/BL_RESET); color packs
   CLR_* | (HL_* attrmask << 8), per the status_update contract */
void
mac_status_update(
    int idx, genericptr_t ptr, int chg UNUSED, int percent UNUSED,
    int color, unsigned long *colormasks)
{
    char *text = (char *) ptr;

    if (idx == BL_FLUSH || idx == BL_RESET) {
        macstat_redraw();
        return;
    }
    if (idx < 0 || idx >= MAXBLSTATS || !status_activefields[idx]
        || !status_vals[idx])
        return;
    stat_colors[idx] = color;
    if (idx == BL_CONDITION) {
        stat_cond_bits = ptr ? *(unsigned long *) ptr : 0UL;
        stat_colormasks = colormasks;
    } else if (idx == BL_GOLD) {
        /* gold arrives glyph-encoded ("\GXXXXNNNN:nnn"); decode to the
           symset's gold symbol once, like the curses port does */
        status_vals[BL_GOLD][0] = ' ';
        (void) decode_mixed(&status_vals[BL_GOLD][1], text ? text : "");
    } else {
        Snprintf(status_vals[idx], MAXCO,
                 status_fieldfmt[idx] ? status_fieldfmt[idx] : "%s",
                 text ? text : "");
    }
}

/* full redraw of the status rows; called on BL_FLUSH and from the
   update-event/clear paths in macwin.c */
void
macstat_redraw(void)
{
    NhWindow *aWin;
    GrafPtr savePort;
    FontInfo fi;
    Rect content;
    short line, ascent, row_h;

    if (!macstat_active() || !_mt_window)
        return;
    aWin = &theWindows[WIN_STATUS];

    GetPort(&savePort);
    SetPortWindowPort(_mt_window);
    TextFont(aWin->font_number);
    TextSize(aWin->font_size);
    TextFace(normal);
    TextMode(srcOr); /* mactty leaves the port in srcCopy, which would
                        whiteout glyphs in the HL_INVERSE box */
    GetFontInfo(&fi);
    ascent = fi.ascent;
    row_h = aWin->row_height;
    if (row_h <= 0)
        row_h = fi.ascent + fi.descent + fi.leading;

    /* SetOrigin(-1,-1) puts content rows at local (0,0); content.right
       then spans the full width plus a harmless 1px inset */
    GetWindowPortBounds(_mt_window, &content);
    SetRect(&content, 0, 0, content.right, 2 * row_h);
    EraseRect(&content);

    for (line = 0; line < 2; line++) {
        short top = line * row_h;
        int i;

        MoveTo(0, top + ascent);
        for (i = 0; fieldorder[line][i] != BL_FLUSH; i++) {
            enum statusfields f = fieldorder[line][i];

            if (!status_activefields[f])
                continue;
            if (f == BL_CONDITION)
                stat_draw_conds(top, ascent);
            else if (status_vals[f] && *status_vals[f])
                stat_draw_str(status_vals[f], stat_colors[f], top, ascent);
        }
    }
    TextFace(normal);
    ForeColor(blackColor);
    SetPort(savePort);
}

/* draw one field at the pen, styled from the packed color/attr.
   HL_DIM/HL_BLINK render normal; caller sets up the port (font, srcOr,
   pen on baseline). */
static void
stat_draw_str(const char *str, int packed, short top, short ascent)
{
    int color = packed & 0x00FF;
    int attr = (packed >> 8) & 0x00FF;
    Style face = normal;
    short len = (short) strlen(str);
    Point pen;

    if (attr & HL_BOLD)
        face |= bold;
    if (attr & HL_ULINE)
        face |= underline;
    if (attr & HL_ITALIC)
        face |= italic;
    TextFace(face);

    if (attr & HL_INVERSE) {
        /* box in the field color (black if none), white text; classic
           ForeColor (RGBForeColor is a no-op at 1bpp; black/white don't
           disturb the tile CLUT) */
        NhWindow *aWin = &theWindows[WIN_STATUS];
        Rect box;
        short w;

        GetPen(&pen);
        w = TextWidth(str, 0, len);
        SetRect(&box, pen.h, top, pen.h + w, top + aWin->row_height);
        mac_set_text_color(color); /* NO_COLOR -> black */
        PaintRect(&box);
        ForeColor(whiteColor);
        /* on 1-bit screens white-via-srcOr is a no-op (OR can only set
           bits); srcBic cuts the glyphs out of the black box instead */
        if (mac_main_depth() < 2)
            TextMode(srcBic);
        MoveTo(pen.h, top + ascent);
        DrawText(str, 0, len);
        if (mac_main_depth() < 2)
            TextMode(srcOr);
        ForeColor(blackColor);
    } else {
        mac_set_text_color(color);
        DrawText(str, 0, len);
        ForeColor(blackColor);
    }
    TextFace(normal);
}

/* conditions get per-condition color/attr from the colormasks array */
static void
stat_draw_conds(short top, short ascent)
{
    int i, color, attr, packed;

    for (i = 0; i < (int) SIZE(cond_list); i++) {
        if (!(stat_cond_bits & cond_list[i].mask))
            continue;
        color = stat_condcolor(cond_list[i].mask, stat_colormasks);
        attr = stat_condattr(cond_list[i].mask, stat_colormasks);
        packed = (color & 0x00FF) | (attr << 8);
        stat_draw_str(cond_list[i].text, packed, top, ascent);
    }
}

/* cond_hilites[0..CLR_MAX-1] hold per-color condition masks
   (pattern from win/curses/cursstat.c condcolor) */
static int
stat_condcolor(unsigned long bm, unsigned long *bmarray)
{
    int i;

    if (bm && bmarray)
        for (i = 0; i < CLR_MAX; ++i)
            if ((bmarray[i] & (unsigned long) bm) != 0)
                return i;
    return NO_COLOR;
}

/* cond_hilites[HL_ATTCLR_*] hold per-attribute condition masks
   (pattern from win/curses/cursstat.c condattr) */
static int
stat_condattr(unsigned long bm, unsigned long *bmarray)
{
    int i, attr = 0;

    if (bm && bmarray)
        for (i = HL_ATTCLR_BOLD; i < BL_ATTCLR_MAX; ++i)
            if ((bmarray[i] & (unsigned long) bm) != 0)
                switch (i) {
                case HL_ATTCLR_BOLD:
                    attr |= HL_BOLD;
                    break;
                case HL_ATTCLR_DIM:
                    attr |= HL_DIM;
                    break;
                case HL_ATTCLR_ITALIC:
                    attr |= HL_ITALIC;
                    break;
                case HL_ATTCLR_ULINE:
                    attr |= HL_ULINE;
                    break;
                case HL_ATTCLR_BLINK:
                    attr |= HL_BLINK;
                    break;
                case HL_ATTCLR_INVERSE:
                    attr |= HL_INVERSE;
                    break;
                }
    return attr;
}

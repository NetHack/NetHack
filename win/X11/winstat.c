/* NetHack 3.7	winstat.c	$NHDT-Date: 1649269127 2022/04/06 18:18:47 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.37 $ */
/* Copyright (c) Dean Luick, 1992                                 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Status window routines.  This file supports both the "traditional"
 * tty status display and a "fancy" status display.  A tty status is
 * made if a popup window is requested, otherwise a fancy status is
 * made.  This code assumes that only one fancy status will ever be made.
 * Currently, only one status window (of any type) is _ever_ made.
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV /* X11 include files may define SYSV */
#endif

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h> /* for XtResizeWidget() and XtConfigureWidget() */
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Cardinals.h> /* just for ONE, TWO */
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Viewport.h>
/*#include <X11/Xatom.h>*/

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#include "hack.h"
#include "winX.h"
#include "xwindow.h"

/*
 * Fancy status form entry storage indices.
 */
#define F_DUMMY     0
#define F_STR       1
#define F_DEX       2
#define F_CON       3
#define F_INT       4
#define F_WIS       5
#define F_CHA       6

#define F_NAME      7 /* title: "Name the Rank" where rank is role-specific */
#define F_DLEVEL    8 /* location: dungeon branch and level */
#define F_GOLD      9
#define F_HP       10
#define F_MAXHP    11
#define F_POWER    12
#define F_MAXPOWER 13
#define F_AC       14
#define F_XP_LEVL  15
/*#define F_HD F_XP_LEVL*/
#define F_EXP_PTS  16
#define F_ALIGN    17
#define F_TIME     18
#define F_SCORE    19

/* status conditions grouped by columns; tty orders these differently;
   hunger/encumbrance/movement used to be in the middle with fatal
   conditions on the left but those columns have been swapped and
   renumbered to match new order (forcing shown_stats[] to be reordered);
   some mutually exclusive conditions are overloaded during display--
   they're separate within shown_stats[] but share the same widget */
#define F_HUNGER   20
#define F_ENCUMBER 21
#define F_TRAPPED  22
#define F_TETHERED 23 /* overloads trapped rather than having its own slot */
#define F_LEV      24
#define F_FLY      25
#define F_RIDE     26

#define F_GRABBED  27
#define F_STONE    28
#define F_SLIME    29
#define F_STRNGL   30
#define F_FOODPOIS 31
#define F_TERMILL  32
#define F_IN_LAVA  33 /* could overload trapped but severity differs a lot */

#define F_HELD     34 /* could overload grabbed but severity differs a lot */
#define F_HOLDING  35 /* overloads held */
#define F_BLIND    36
#define F_DEAF     37
#define F_STUN     38
#define F_CONF     39
#define F_HALLU    40

#define NUM_STATS  41

static int condcolor(long, unsigned long *);
static int condattr(long, unsigned long *);
static void HiliteField(Widget, int, int, int, XFontStruct **);
static void PrepStatusField(int, Widget, const char *);
static void DisplayCond(int, unsigned long *);
static int render_conditions(int, int);
#ifdef STATUS_HILITES
static void tt_reset_color(int, int, unsigned long *);
#endif
static void tt_status_fixup(void);
static Widget create_tty_status_field(int, int, Widget, Widget);
static Widget create_tty_status(Widget, Widget);
static void update_fancy_status_field(int, int, int);
static void update_fancy_status(boolean);
static Widget create_fancy_status(Widget, Widget);
static void destroy_fancy_status(struct xwindow *);
static void create_status_window_fancy(struct xwindow *, boolean, Widget);
static void create_status_window_tty(struct xwindow *, boolean, Widget);
static void destroy_status_window_fancy(struct xwindow *);
static void destroy_status_window_tty(struct xwindow *);
static void adjust_status_fancy(struct xwindow *, const char *);
static void adjust_status_tty(struct xwindow *, const char *);

extern const char *status_fieldfmt[MAXBLSTATS];
extern char *status_vals[MAXBLSTATS];
extern boolean status_activefields[MAXBLSTATS];

static unsigned long X11_condition_bits, old_condition_bits;
static int X11_status_colors[MAXBLSTATS],
           old_field_colors[MAXBLSTATS],
           old_cond_colors[32];
static int hpbar_percent, hpbar_color;
/* Number of conditions displayed during this update and last update.
   When the last update had more, the excess need to be erased. */
static int next_cond_indx = 0, prev_cond_indx = 0;

/* TODO: support statuslines:3 in addition to 2 for the tty-style status */
#define X11_NUM_STATUS_LINES 2
#define X11_NUM_STATUS_FIELD 15

static enum statusfields X11_fieldorder[][X11_NUM_STATUS_FIELD] = {
    { BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_ALIGN,
      BL_SCORE, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH,
      BL_FLUSH },
    { BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX,
      BL_AC, BL_XP, BL_EXP, BL_HD, BL_TIME, BL_HUNGER,
      BL_CAP, BL_CONDITION, BL_FLUSH }
};

/* condition list for tty-style display, roughly in order of importance */
static struct tt_condinfo {
    unsigned long mask;
    const char *text;
} tt_condorder[] = {
    { BL_MASK_GRAB, "Grabbed!" },
    { BL_MASK_STONE, "Stone" },
    { BL_MASK_SLIME, "Slime" },
    { BL_MASK_STRNGL, "Strngl" },
    { BL_MASK_FOODPOIS, "FoodPois" },
    { BL_MASK_TERMILL, "TermIll" },
    { BL_MASK_INLAVA, "InLava" },
    { BL_MASK_HELD, "Held" },
    { BL_MASK_HELD, "Holding" },
    { BL_MASK_BLIND, "Blind" },
    { BL_MASK_DEAF, "Deaf" },
    { BL_MASK_STUN, "Stun" },
    { BL_MASK_CONF, "Conf" },
    { BL_MASK_HALLU, "Hallu" },
    { BL_MASK_TRAPPED, "Trapped" },
    { BL_MASK_TETHERED, "Tethered", },
    { BL_MASK_LEV, "Lev" },
    { BL_MASK_FLY, "Fly" },
    { BL_MASK_RIDE, "Ride" },
};

static const char *fancy_status_hilite_colors[] = {
    "grey15",
    "red3",
    "dark green",
    "saddle brown",
    "blue",
    "magenta3",
    "dark cyan",
    "web gray",
    "",          /* NO_COLOR */
    "orange",
    "green3",
    "goldenrod",
    "royal blue",
    "magenta",
    "cyan",
    "white",
};

static Widget X11_status_widget;
static Widget X11_status_labels[MAXBLSTATS];
static Widget X11_cond_labels[32]; /* Ugh */
static XFontStruct *X11_status_font;
static Pixel X11_status_fg, X11_status_bg;

struct xwindow *xw_status_win;

static int
condcolor(long bm, unsigned long *bmarray)
{
    int i;

    if (bm && bmarray)
        for (i = 0; i < CLR_MAX; ++i) {
            if (bmarray[i] && (bm & bmarray[i]))
                return i;
        }
    return NO_COLOR;
}

static int
condattr(long bm, unsigned long *bmarray)
{
    int attr = 0;
    int i;

    if (bm && bmarray) {
        for (i = HL_ATTCLR_DIM; i < BL_ATTCLR_MAX; ++i) {
            if (bmarray[i] && (bm & bmarray[i])) {
                switch(i) {
                case HL_ATTCLR_DIM:
                    attr |= HL_DIM;
                    break;
                case HL_ATTCLR_BLINK:
                    attr |= HL_BLINK;
                    break;
                case HL_ATTCLR_ULINE:
                    attr |= HL_ULINE;
                    break;
                case HL_ATTCLR_INVERSE:
                    attr |= HL_INVERSE;
                    break;
                case HL_ATTCLR_BOLD:
                    attr |= HL_BOLD;
                    break;
                }
            }
        }
    }
    return attr;
}

void
X11_status_init(void)
{
    int i;

    /* no color and no attributes */
    for (i = 0; i < MAXBLSTATS; ++i)
        X11_status_colors[i] = old_field_colors[i] = NO_COLOR;
    for (i = 0; i < SIZE(old_cond_colors); ++i)
        old_cond_colors[i] = NO_COLOR;
    hpbar_percent = 0, hpbar_color = NO_COLOR;
    X11_condition_bits = old_condition_bits = 0L;
    /* let genl_status_init do most of the initialization */
    genl_status_init();
}

void
X11_status_finish(void)
{
    /* nothing */
    return;
}

void
X11_status_enablefield(int fieldidx, const char *nm,
                       const char *fmt, boolean enable)
{
    genl_status_enablefield(fieldidx, nm, fmt, enable);
}

#if 0
int
cond_bm2idx(unsigned long bm)
{
    int i;

    for (i = 0; i < 32; i++)
        if ((1 << i) == bm)
            return i;
    return -1;
}
#endif

/* highlight a tty-style status field (or condition) */
static void
HiliteField(Widget label,
            int fld, int cond, int colrattr,
            XFontStruct **font_p)
{
#ifdef STATUS_HILITES
    static Pixel grayPxl, blackPxl, whitePxl;
    Arg args[6];
    Cardinal num_args;
    XFontStruct *font = X11_status_font;
    Pixel px, fg = X11_status_fg, bg = X11_status_bg;
    struct xwindow *xw = xw_status_win;
    int colr, attr;

#ifdef TEXTCOLOR
    if ((colrattr & 0x00ff) >= CLR_MAX)
    /* for !TEXTCOLOR, the following line is unconditional */
#endif
        colrattr = (colrattr & ~0x00ff) | NO_COLOR;
    colr = colrattr & 0x00ff; /* guaranteed to be >= 0 and < CLR_MAX */
    attr = (colrattr >> 8) & 0x00ff;

    /* potentially used even for !TEXTCOLOR configuration */
    if (!grayPxl) {/* one-time init */
        grayPxl = get_nhcolor(xw, CLR_GRAY).pixel;
        blackPxl = get_nhcolor(xw, CLR_BLACK).pixel;
        whitePxl = get_nhcolor(xw, CLR_WHITE).pixel;
    }
    /* [shouldn't be necessary; setting up gray will set up all colors] */
    if (colr != NO_COLOR && !xw->nh_colors[colr].pixel)
        (void) get_nhcolor(xw, colr);

    /* handle highlighting if caller has specified that; set foreground,
       background, and font even if not specified this time in case they
       used modified values last time (which would stick if not reset) */
    (void) memset((genericptr_t) args, 0, sizeof args);
    num_args = 0;
    if (colr != NO_COLOR)
        fg = xw->nh_colors[colr].pixel;
    if ((attr & HL_INVERSE) != 0) {
        px = fg;
        fg = bg;
        bg = px;
    }
    /* foreground and background might both default to black, so we
       need to force one to be different if/when they're the same
       (actually, tt_status_fixup() takes care of that nowadays);
       using gray to implement 'dim' only works for black and white
       (or color+'inverse' when former background was black or white) */
    if (fg == bg
        || ((attr & HL_DIM) != 0 && (fg == whitePxl || fg == blackPxl)))
        fg = (fg != grayPxl) ? grayPxl
             : (fg != blackPxl) ? blackPxl
               : whitePxl;
    XtSetArg(args[num_args], nhStr(XtNforeground), fg); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbackground), bg); num_args++;
    if (attr & HL_BOLD) {
        load_boldfont(xw_status_win, label);
        if (xw_status_win->boldfs)
            font = xw_status_win->boldfs;
    }
    XtSetArg(args[num_args], nhStr(XtNfont), font); num_args++;
    XtSetValues(label, args, num_args);

    /* return possibly modified font to caller so that text width
       measurement can use it */
    if (font_p)
        *font_p = font;
#else /*!STATUS_HILITES*/
    nhUse(label);
    nhUse(font_p);
#endif /*?STATUS_HILITES*/
    if (fld != BL_CONDITION)
        old_field_colors[fld] = colrattr;
    else
        old_cond_colors[cond] = colrattr;
}

/* set up a specific field other than 'condition'; its general location
   was specified during widget creation but it might need adjusting */
static void
PrepStatusField(int fld, Widget label, const char *text)
{
    Arg args[6];
    Cardinal num_args;
    Dimension lbl_wid;
    XFontStruct *font = X11_status_font;
    int colrattr = X11_status_colors[fld];
    struct status_info_t *si = xw_status_win->Win_info.Status_info;

    /* highlight if color and/or attribute(s) are different from last time */
    if (colrattr != old_field_colors[fld])
        HiliteField(label, fld, 0, colrattr, &font);

    num_args = 0;
    (void) memset((genericptr_t) args, 0, sizeof args);
    /* set up the current text to be displayed */
    if (text && *text) {
        lbl_wid = 2 * si->in_wd + XTextWidth(font, text, (int) strlen(text));
    } else {
        text = "";
        lbl_wid = 1;
    }
    XtSetArg(args[num_args], nhStr(XtNlabel), text); num_args++;
    /*XtSetArg(args[num_args], nhStr(XtNwidth), lbl_wid); num_args++;*/
    XtSetValues(label, args, num_args);
    XtResizeWidget(label, lbl_wid, si->ht, si->brd);
}

/* set up one status condition for tty-style status display */
static void
DisplayCond(int c_idx, /* index into tt_condorder[] */
            unsigned long *colormasks)
{
    Widget label;
    Arg args[6];
    Cardinal num_args;
    Dimension lbl_wid;
    XFontStruct *font = X11_status_font;
    int coloridx, attrmask, colrattr, idx;
    unsigned long bm = tt_condorder[c_idx].mask;
    const char *text = tt_condorder[c_idx].text;
    struct status_info_t *si = xw_status_win->Win_info.Status_info;

    if ((X11_condition_bits & bm) == 0)
        return;

    /* widgets have been created for every condition; we allocate them
       from left to right rather than keeping their original assignments */
    idx = next_cond_indx;
    label = X11_cond_labels[idx];

    /* handle highlighting if caller requests it */
    coloridx = condcolor(bm, colormasks);
    attrmask = condattr(bm, colormasks);
    colrattr = (attrmask << 8) | coloridx;
    if (colrattr != old_cond_colors[c_idx])
        HiliteField(label, BL_CONDITION, c_idx, colrattr, &font);

    (void) memset((genericptr_t) args, 0, sizeof args);
    num_args = 0;
    /* set the condition text and its width; this widget might have
       been displaying a different condition last time around */
    XtSetArg(args[num_args], nhStr(XtNlabel), text); num_args++;
    /* measure width after maybe changing font [HiliteField()] */
    lbl_wid = 2 * si->in_wd + XTextWidth(font, text, (int) strlen(text));
    /*XtSetArg(args[num_args], nhStr(XtNwidth), lbl_wid); num_args++;*/

    /* make this condition widget be ready for display */
    XtSetValues(label, args, num_args);
    XtResizeWidget(label, lbl_wid, si->ht, si->brd);

    ++next_cond_indx;
}

/* display the tty-style status conditions; the number shown varies and
   we might be showing more, same, or fewer than during previous status */
static int
render_conditions(int row, int dx)
{
    Widget label;
    Arg args[6];
    Cardinal num_args;
    Position lbl_x;
    int i, gap = 5;
    struct status_info_t *si = xw_status_win->Win_info.Status_info;
    Dimension lbl_wid, brd_wid = si->brd;

    for (i = 0; i < next_cond_indx; i++) {
        label = X11_cond_labels[i];

        /* width of this widget was set in DisplayCond(); fetch it */
        (void) memset((genericptr_t) args, 0, sizeof args);
        num_args = 0;
        XtSetArg(args[num_args], nhStr(XtNwidth), &lbl_wid); num_args++;
        XtGetValues(label, args, num_args);

        /* figure out where to draw this widget and place it there */
        lbl_x = (Position) (dx + 1 + gap);

        XtConfigureWidget(label, lbl_x, si->y[row], lbl_wid, si->ht, brd_wid);

        /* keep track of where the end of our text appears */
        dx = (int) lbl_x + (int) (lbl_wid + 2 * brd_wid) - 1;
    }

    /* if we have fewer conditions shown now than last time, set the
       excess ones to blank; unlike the set drawn above, these haven't
       been prepared in advance by DisplayCond because they aren't
       being displayed; they might have been highlighted last time so
       we need to specify more than just an empty text string */
    if (next_cond_indx < prev_cond_indx) {
        XFontStruct *font = X11_status_font;
        Pixel fg = X11_status_fg, bg = X11_status_bg;

        lbl_x = dx + 1;
        lbl_wid = 1 + 2 * brd_wid;
        for (i = next_cond_indx; i < prev_cond_indx; ++i) {
            label = X11_cond_labels[i];

            (void) memset((genericptr_t) args, 0, sizeof args);
            num_args = 0;
            XtSetArg(args[num_args], nhStr(XtNlabel), ""); num_args++;
            XtSetArg(args[num_args], nhStr(XtNfont), font); num_args++;
            XtSetArg(args[num_args], nhStr(XtNforeground), fg); num_args++;
            XtSetArg(args[num_args], nhStr(XtNbackground), bg); num_args++;
            /*XtSetArg(args[num_args], nhStr(XtNwidth), lbl_wid); num_args++;*/
            /*XtSetArg(args[num_args], nhStr(XtNx), lbl_x); num_args++;*/
            XtSetValues(label, args, num_args);
            old_cond_colors[i] = NO_COLOR; /* fg, bg, font were just reset */
            XtConfigureWidget(label, lbl_x, si->y[row], lbl_wid, si->ht, 0);
            /* don't advance 'dx' here */
        }
    }

    return dx;
}

#ifdef STATUS_HILITES
/* reset status_hilite for BL_RESET; if highlighting has been disabled or
   this field is disabled, clear highlighting for this field or condition */
static void
tt_reset_color(int fld, int cond, unsigned long *colormasks)
{
    Widget label;
    int colrattr = NO_COLOR;

    if (fld != BL_CONDITION) {
        if (iflags.hilite_delta != 0L && status_activefields[fld])
            colrattr = X11_status_colors[fld];
        cond = 0;
        label = X11_status_labels[fld];
    } else {
        unsigned long bm = tt_condorder[cond].mask;

        if (iflags.hilite_delta != 0L && (X11_condition_bits & bm) != 0) {
            /* BL_RESET before first BL_CONDITION will have colormasks==Null
               but condcolor() and condattr() can cope with that */
#ifdef TEXTCOLOR
            colrattr = condcolor(bm, colormasks);
#endif
            colrattr |= (condattr(bm, colormasks) << 8);
        }
        label = X11_cond_labels[cond];
    }
    HiliteField(label, fld, cond, colrattr, (XFontStruct **) 0);
}
#endif

/* make sure foreground, background, and font have reasonable values,
   then explicitly set them for all the status widgets;
   also cache some geometry settings in (*xw_status_win).Status_info */
static void
tt_status_fixup(void)
{
    Arg args[6];
    Cardinal num_args;
    Widget w;
    Position lbl_y;
    int x, y, ci, fld;
    XFontStruct *font = 0;
    Pixel fg = 0, bg = 0;
    struct status_info_t *si = xw_status_win->Win_info.Status_info;

    (void) memset((genericptr_t) args, 0, sizeof args);
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfont), &font); num_args++;
    XtSetArg(args[num_args], nhStr(XtNforeground), &fg); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbackground), &bg); num_args++;
    XtGetValues(X11_status_widget, args, num_args);
    if (fg == bg) {
        XColor black = get_nhcolor(xw_status_win, CLR_BLACK),
               white = get_nhcolor(xw_status_win, CLR_WHITE);

        fg = (bg == white.pixel) ? black.pixel : white.pixel;
    }
    X11_status_fg = si->fg = fg, X11_status_bg = si->bg = bg;

    if (!font) {
        w = X11_status_widget;
        XtSetArg(args[0], nhStr(XtNfont), &font);
        do {
            XtGetValues(w, args, ONE);
        } while (!font && (w = XtParent(w)) != 0);

        if (!font) {
            /* trial and error time -- this is where we've actually
               been obtaining the font even though we aren't setting
               it for any of the field widgets (until for(y,x) below) */
            XtGetValues(X11_status_labels[0], args, ONE);

            if (!font) { /* this bit is untested... */
                /* write some text and hope Xaw sets up font for us */
                XtSetArg(args[0], nhStr(XtNlabel), "NetHack");
                XtSetValues(X11_status_labels[0], args, ONE);
                (void) XFlush(XtDisplay(X11_status_labels[0]));

                XtSetArg(args[0], nhStr(XtNfont), &font);
                XtGetValues(X11_status_labels[0], args, ONE);

                /* if still Null, XTextWidth() would crash so bail out */
                if (!font)
                    panic("X11 status can't determine font.");
            }
        }
    }
    X11_status_font = si->fs = font;

    /* amount of space to advance a widget's location by one space;
       increase width a tiny bit beyond the actual space */
    si->spacew = XTextWidth(X11_status_font, " ", 1) + (Dimension) 1;

    (void) memset((genericptr_t) args, 0, sizeof args);
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfont), font); num_args++;
    XtSetArg(args[num_args], nhStr(XtNforeground), fg); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbackground), bg); num_args++;
    XtSetValues(X11_status_widget, args, num_args);

    for (y = 0; y < X11_NUM_STATUS_LINES; y++) {
        for (x = 0; x < X11_NUM_STATUS_FIELD; x++) {
            fld = X11_fieldorder[y][x]; /* next field to handle */
            if (fld <= BL_FLUSH)
                continue; /* skip fieldorder[][] padding */

            XtSetValues(X11_status_labels[fld], args, num_args);
            old_field_colors[fld] = NO_COLOR;

            if (fld == BL_CONDITION) {
                for (ci = 0; ci < SIZE(X11_cond_labels); ++ci) { /* 0..31 */
                    XtSetValues(X11_cond_labels[ci], args, num_args);
                    old_cond_colors[ci] = NO_COLOR;
                }
            }
        }
    }

    /* cache the y coordinate of each row for XtConfigureWidget() */
    (void) memset((genericptr_t) args, 0, sizeof args);
    num_args = 0;
    XtSetArg(args[0], nhStr(XtNy), &lbl_y); num_args++;
    for (y = 0; y < X11_NUM_STATUS_LINES; y++) {
        fld = X11_fieldorder[y][0];
        XtGetValues(X11_status_labels[fld], args, num_args);
        si->y[y] = lbl_y;
    }

    /* cache height, borderWidth, and internalWidth for XtResizeWidget() */
    (void) memset((genericptr_t) args, 0, sizeof args);
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNheight), &si->ht); num_args++;
    XtSetArg(args[num_args], nhStr(XtNborderWidth), &si->brd); num_args++;
    XtSetArg(args[num_args], nhStr(XtNinternalWidth), &si->in_wd); num_args++;
    XtGetValues(X11_status_labels[0], args, num_args);
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* core requests updating one status field (or is indicating that it's time
   to flush all updated fields); tty-style handling */
static void
X11_status_update_tty(int fld, genericptr_t ptr, int chg UNUSED, int percent,
                      int color,
                      unsigned long *colormasks) /* bitmask of highlights
                                                    for conditions */
{
    static int xtra_space[MAXBLSTATS];
    static unsigned long *cond_colormasks = (unsigned long *) 0;

    Arg args[6];
    Cardinal num_args;
    Position lbl_x;
    Dimension lbl_wid, brd_wid;
    Widget label;

    struct status_info_t *si;
    char goldbuf[40];
    const char *fmt;
    const char *text;
    unsigned long *condptr;
    int f, x, y, dx;

    if (X11_status_fg == X11_status_bg || !X11_status_font)
        tt_status_fixup();

    if (fld == BL_RESET) {
#ifdef STATUS_HILITES
        for (y = 0; y < X11_NUM_STATUS_LINES; y++) {
            for (x = 0; x < X11_NUM_STATUS_FIELD; x++) {
                f = X11_fieldorder[y][x];
                if (f <= BL_FLUSH)
                    continue; /* skip padding in the fieldorder[][] layout */
                if (f != BL_CONDITION) {
                    tt_reset_color(f, 0, (unsigned long *) 0);
                } else {
                    int c_i;

                    for (c_i = 0; c_i < SIZE(tt_condorder); ++c_i)
                        tt_reset_color(f, c_i, cond_colormasks);
                }
            }
        }
#endif
        fld = BL_FLUSH;
    }

    if (fld == BL_CONDITION) {
        condptr = (unsigned long *) ptr;
        X11_condition_bits = *condptr;
        cond_colormasks = colormasks; /* expected to be non-Null */

        prev_cond_indx = next_cond_indx;
        next_cond_indx = 0;
        /* if any conditions are active, set up their widgets */
        if (X11_condition_bits)
            for (f = 0; f < SIZE(tt_condorder); ++f)
                DisplayCond(f, cond_colormasks);
        return;

    } else if (fld != BL_FLUSH) {
        /* set up a specific field other than 'condition' */
        text = (char *) ptr;
        if (fld == BL_GOLD)
            text = decode_mixed(goldbuf, text);
        xtra_space[fld] = 0;
        if (status_activefields[fld]) {
            fmt = (fld == BL_TITLE && iflags.wc2_hitpointbar) ? "%-30s"
                  : status_fieldfmt[fld] ? status_fieldfmt[fld] : "%s";
            if (*fmt == ' ') {
                ++xtra_space[fld];
                ++fmt;
            }
            Sprintf(status_vals[fld], fmt, text);
        } else {
            /* don't expect this since core won't call status_update()
               for a field which isn't active */
            *status_vals[fld] = '\0';
        }
#ifndef TEXTCOLOR
        /* even without color, attribute(s) bits still apply */
        color = (color & ~0x00ff) | NO_COLOR;
#endif
#ifdef STATUS_HILITES
        if (!iflags.hilite_delta)
            color = NO_COLOR;
#endif
        X11_status_colors[fld] = color;
        if (iflags.wc2_hitpointbar && fld == BL_HP) {
            hpbar_percent = percent;
            hpbar_color = color;
        }

        label = X11_status_labels[fld];
        text = status_vals[fld];
        PrepStatusField(fld, label, text);
        return;
    }

    /*
     * BL_FLUSH:  draw all the status fields.
     */
    si = xw_status_win->Win_info.Status_info; /* for cached geometry */

    for (y = 0; y < X11_NUM_STATUS_LINES; y++) { /* row */
        dx = 0; /* no pixels written to this row yet */

        for (x = 0; x < X11_NUM_STATUS_FIELD; x++) { /* 'column' */
            f = X11_fieldorder[y][x];
            if (f <= BL_FLUSH)
                continue; /* skip padding in the fieldorder[][] layout */

            if (f == BL_CONDITION) {
                if (next_cond_indx > 0 || prev_cond_indx > 0)
                    dx = render_conditions(y, dx);
                continue;
            }

            label = X11_status_labels[f];
            text = status_vals[f];

            (void) memset((genericptr_t) args, 0, sizeof args);
            num_args = 0;
            XtSetArg(args[num_args], nhStr(XtNwidth), &lbl_wid); num_args++;
            XtGetValues(label, args, num_args);
            brd_wid = si->brd;

            /* for a field which shouldn't be shown, we can't just skip
               it because we might need to remove its previous content
               if it has just been toggled off */
            if (!status_activefields[f]) {
                if (lbl_wid <= 1)
                    continue;
                text = "";
                lbl_wid = (Dimension) 1;
                brd_wid = (Dimension) 0;
            } else if (xtra_space[f]) {
                /* if this field was to be formatted with a leading space
                   to separate it from the preceding field, we suppressed
                   that space during formatting; insert separation between
                   fields here; this prevents inverse video highlighting
                   from inverting the separating space along with the text */
                dx += xtra_space[f] * si->spacew;
            }

            /* where to display the current widget */
            lbl_x = (Position) (dx + 1);

            (void) memset((genericptr_t) args, 0, sizeof args);
            num_args = 0;
            XtSetArg(args[num_args], nhStr(XtNlabel), text); num_args++;
            /*XtSetArg(args[num_args], nhStr(XtNwidth), lbl_wid); num_args++;*/
            /*XtSetArg(args[num_args], nhStr(XtNx), lbl_x); num_args++;*/
            XtSetValues(label, args, num_args);
            XtConfigureWidget(label, lbl_x, si->y[y],
                              lbl_wid, si->ht, brd_wid);

            /* track the right-most pixel written so far */
            dx = (int) lbl_x + (int) (lbl_wid + 2 * brd_wid) - 1;
        } /* [x] fields/columns in current row */
    } /* [y] rows */

    /* this probably doesn't buy us anything but it isn't a sure thing
       that nethack will immediately ask for input (triggering auto-flush) */
    (void) XFlush(XtDisplay(X11_status_labels[0]));
}

RESTORE_WARNING_FORMAT_NONLITERAL

/*ARGSUSED*/
static void
X11_status_update_fancy(int fld, genericptr_t ptr, int chg UNUSED,
                        int percent UNUSED, int colrattr,
                        unsigned long *colormasks UNUSED)
{
    static const struct bl_to_ff {
        int bl, ff;
    } bl_to_fancyfield[] = {
        { BL_TITLE, F_NAME },
        { BL_STR, F_STR },
        { BL_DX, F_DEX },
        { BL_CO, F_CON },
        { BL_IN, F_INT },
        { BL_WI, F_WIS },
        { BL_CH, F_CHA },
        { BL_ALIGN, F_ALIGN },
        { BL_SCORE, F_SCORE },
        { BL_CAP, F_ENCUMBER },
        { BL_GOLD, F_GOLD },
        { BL_ENE, F_POWER },
        { BL_ENEMAX, F_MAXPOWER },
        { BL_XP, F_XP_LEVL }, /* shares with BL_HD, depending upon Upolyd */
        { BL_AC, F_AC },
        { BL_TIME, F_TIME },
        { BL_HUNGER, F_HUNGER },
        { BL_HP, F_HP },
        { BL_HPMAX, F_MAXHP },
        { BL_LEVELDESC, F_DLEVEL },
        { BL_EXP, F_EXP_PTS }
    };
    static const struct mask_to_ff {
        unsigned long mask;
        int ff;
    } mask_to_fancyfield[] = {
        { BL_MASK_GRAB, F_GRABBED },
        { BL_MASK_STONE, F_STONE },
        { BL_MASK_SLIME, F_SLIME },
        { BL_MASK_STRNGL, F_STRNGL },
        { BL_MASK_FOODPOIS, F_FOODPOIS },
        { BL_MASK_TERMILL, F_TERMILL },
        { BL_MASK_INLAVA, F_IN_LAVA },
        { BL_MASK_HELD, F_HELD },
        { BL_MASK_HOLDING, F_HOLDING },
        { BL_MASK_BLIND, F_BLIND },
        { BL_MASK_DEAF, F_DEAF },
        { BL_MASK_STUN, F_STUN },
        { BL_MASK_CONF, F_CONF },
        { BL_MASK_HALLU, F_HALLU },
        { BL_MASK_TRAPPED, F_TRAPPED },
        { BL_MASK_TETHERED, F_TETHERED },
        { BL_MASK_LEV, F_LEV },
        { BL_MASK_FLY, F_FLY },
        { BL_MASK_RIDE, F_RIDE }
    };
    int i;

    if (fld == BL_RESET || fld == BL_FLUSH) {
        if (WIN_STATUS != WIN_ERR) {
            update_fancy_status(FALSE);
        }
        return;
    }

    if (fld == BL_CONDITION) {
        unsigned long changed_bits, *condptr = (unsigned long *) ptr;

        X11_condition_bits = *condptr;
        /* process the bits that are different from last time */
        changed_bits = (X11_condition_bits ^ old_condition_bits);
        if (changed_bits) {
            for (i = 0; i < SIZE(mask_to_fancyfield); i++)
                if ((changed_bits & mask_to_fancyfield[i].mask) != 0L)
                    update_fancy_status_field(mask_to_fancyfield[i].ff,
                            condcolor(mask_to_fancyfield[i].mask, colormasks),
                            condattr(mask_to_fancyfield[i].mask, colormasks));
            old_condition_bits = X11_condition_bits; /* remember 'On' bits */
        }
    } else {
        int colr, attr;

#ifdef TEXTCOLOR
        if ((colrattr & 0x00ff) >= CLR_MAX)
            /* for !TEXTCOLOR, the following line is unconditional */
#endif
            colrattr = (colrattr & ~0x00ff) | NO_COLOR;
        colr = colrattr & 0x00ff; /* guaranteed to be >= 0 and < CLR_MAX */
        attr = (colrattr >> 8) & 0x00ff;

        for (i = 0; i < SIZE(bl_to_fancyfield); i++)
            if (bl_to_fancyfield[i].bl == fld) {
                update_fancy_status_field(bl_to_fancyfield[i].ff, colr, attr);
                break;
            }
    }
}

void
X11_status_update(int fld, genericptr_t ptr, int chg,
                  int percent, int color,
                  unsigned long *colormasks)
{
    if (fld < BL_RESET || fld >= MAXBLSTATS)
        panic("X11_status_update(%d) -- invalid field", fld);

    if (appResources.fancy_status)
        X11_status_update_fancy(fld, ptr, chg, percent, color, colormasks);
    else
        X11_status_update_tty(fld, ptr, chg, percent, color, colormasks);
}

/* create a widget for a particular status field or potential condition */
static Widget
create_tty_status_field(int fld, int condindx, Widget above, Widget left)
{
    Arg args[16];
    Cardinal num_args;
    char labelname[40];
    int gap = condindx ? 5 : 0;

    if (!condindx)
        Sprintf(labelname, "label_%s", bl_idx_to_fldname(fld));
    else
        Sprintf(labelname, "cond_%02d", condindx);

    /* set up widget attributes which (mostly) aren't going to be changing */
    (void) memset((genericptr_t) args, 0, sizeof args);
    num_args = 0;
    if (above) {
        XtSetArg(args[num_args], nhStr(XtNfromVert), above); num_args++;
    }
    if (left) {
        XtSetArg(args[num_args], nhStr(XtNfromHoriz), left); num_args++;
    }

    XtSetArg(args[num_args], nhStr(XtNhorizDistance), gap); num_args++;
    XtSetArg(args[num_args], nhStr(XtNvertDistance), 0); num_args++;

    XtSetArg(args[num_args], nhStr(XtNtopMargin), 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottomMargin), 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleftMargin), 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNrightMargin), 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNjustify), XtJustifyLeft); num_args++;
    /* internalWidth:  default is 4; cut that it half and adjust regular
       width to have it on both left and right instead of just on the left */
    XtSetArg(args[num_args], nhStr(XtNinternalWidth), 2); num_args++;
    XtSetArg(args[num_args], nhStr(XtNborderWidth), 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNlabel), ""); num_args++;
    return XtCreateManagedWidget(labelname, labelWidgetClass,
                                 X11_status_widget, args, num_args);
}

/* create an overall status widget (X11_status_widget) and also
   separate widgets for all status fields and potential conditions */
static Widget
create_tty_status(Widget parent, Widget top)
{
    Widget form; /* viewport that holds the form that surrounds everything */
    Widget w, over_w, prev_w;
    Arg args[6];
    Cardinal num_args;
    int x, y, i, fld;

    (void) memset((genericptr_t) args, 0, sizeof args);
    num_args = 0;
    if (top) {
        XtSetArg(args[num_args], nhStr(XtNfromVert), top); num_args++;
    }
    XtSetArg(args[num_args], nhStr(XtNdefaultDistance), 0); num_args++;
    XtSetArg(args[num_args], XtNborderWidth, 0); num_args++;
    XtSetArg(args[num_args], XtNwidth, 400); num_args++;
    XtSetArg(args[num_args], XtNheight, 100); num_args++;
    form = XtCreateManagedWidget("status_viewport", viewportWidgetClass,
                                 parent, args, num_args);

    (void) memset((genericptr_t) args, 0, sizeof args);
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, 400); num_args++;
    XtSetArg(args[num_args], XtNheight, 100); num_args++;
    X11_status_widget = XtCreateManagedWidget("status_form", formWidgetClass,
                                              form, args, num_args);

    for (y = 0; y < X11_NUM_STATUS_LINES; y++) {
        fld = (y > 0) ? X11_fieldorder[y - 1][0] : 0; /* temp, for over_w */
        /* for row(s) beyond the first, pick a widget in the previous
           row to put this one underneath (in 'y' terms; 'x' is fluid) */
        over_w = (y > 0) ? X11_status_labels[fld] : (Widget) 0;
        /* widget on current row to put the next one to the right of ('x') */
        prev_w = (Widget) 0;
        for (x = 0; x < X11_NUM_STATUS_FIELD; x++) {
            fld = X11_fieldorder[y][x]; /* next field to handle */
            if (fld <= BL_FLUSH)
                continue; /* skip fieldorder[][] padding */

            w = create_tty_status_field(fld, 0, over_w, prev_w);
            X11_status_labels[fld] = prev_w = w;

            if (fld == BL_CONDITION) {
                for (i = 1; i <= SIZE(X11_cond_labels); ++i) { /* 1..32 */
                    w = create_tty_status_field(fld, i, over_w, prev_w);
                    X11_cond_labels[i - 1] = prev_w = w;
                }
            }
        }
    }
    return X11_status_widget;
}

/*ARGSUSED*/
void
create_status_window_tty(struct xwindow *wp, /* window pointer */
                         boolean create_popup UNUSED, Widget parent)
{
    wp->type = NHW_STATUS;
    wp->w = create_tty_status(parent, (Widget) 0);
}

void
destroy_status_window_tty(struct xwindow *wp)
{
    /* If status_information is defined, then it a "text" status window. */
    if (wp->status_information) {
        if (wp->popup) {
            nh_XtPopdown(wp->popup);
            if (!wp->keep_window)
                XtDestroyWidget(wp->popup), wp->popup = (Widget) 0;
        }
        free((genericptr_t) wp->status_information);
        wp->status_information = 0;
    } else {
        ;
    }
    if (!wp->keep_window)
        wp->type = NHW_NONE;
}

/*ARGSUSED*/
void
adjust_status_tty(struct xwindow *wp UNUSED, const char *str UNUSED)
{
    /* nothing */
    return;
}

void
create_status_window(struct xwindow *wp, /* window pointer */
                     boolean create_popup, Widget parent)
{
    struct status_info_t *si = (struct status_info_t *) alloc(sizeof *si);

    xw_status_win = wp;
    if (wp->Win_info.Status_info)
        free((genericptr_t) wp->Win_info.Status_info);
    wp->Win_info.Status_info = si;
    (void) memset((genericptr_t) si, 0, sizeof *si);

    if (!appResources.fancy_status)
        create_status_window_tty(wp, create_popup, parent);
    else
        create_status_window_fancy(wp, create_popup, parent);
}

void
destroy_status_window(struct xwindow *wp)
{
    if (appResources.fancy_status)
        destroy_status_window_fancy(wp);
    else
        destroy_status_window_tty(wp);
}

void
adjust_status(struct xwindow *wp, const char *str)
{
    if (appResources.fancy_status)
        adjust_status_fancy(wp, str);
    else
        adjust_status_tty(wp, str);
}

void
create_status_window_fancy(struct xwindow *wp, /* window pointer */
                           boolean create_popup, Widget parent)
{
    XFontStruct *fs;
    Arg args[8];
    Cardinal num_args;
    Position top_margin, bottom_margin, left_margin, right_margin;

    wp->type = NHW_STATUS;

    if (!create_popup) {
        /*
         * If we are not creating a popup, then we must be the "main" status
         * window.
         */
        if (!parent)
            panic("create_status_window_fancy: no parent for fancy status");
        wp->status_information = 0;
        wp->w = create_fancy_status(parent, (Widget) 0);
        return;
    }

    wp->status_information =
        (struct status_info_t *) alloc(sizeof (struct status_info_t));

    init_text_buffer(&wp->status_information->text);

    num_args = 0;
    XtSetArg(args[num_args], XtNallowShellResize, False); num_args++;
    XtSetArg(args[num_args], XtNinput, False); num_args++;

    wp->popup = parent = XtCreatePopupShell("status_popup",
                                            topLevelShellWidgetClass,
                                            toplevel, args, num_args);
    /*
     * If we're here, then this is an auxiliary status window.  If we're
     * cancelled via a delete window message, we should just pop down.
     */

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNdisplayCaret), False); num_args++;
    XtSetArg(args[num_args], nhStr(XtNscrollHorizontal),
             XawtextScrollWhenNeeded); num_args++;
    XtSetArg(args[num_args], nhStr(XtNscrollVertical),
             XawtextScrollWhenNeeded); num_args++;

    wp->w = XtCreateManagedWidget("status", /* name */
                                  asciiTextWidgetClass,
                                  parent,    /* parent widget */
                                  args,      /* set some values */
                                  num_args); /* number of values to set */

    /*
     * Adjust the height and width of the message window so that it
     * is two lines high and COLNO of the widest characters wide.
     */

    /* Get the font and margin information. */
    num_args = 0;
    XtSetArg(args[num_args], XtNfont, &fs); num_args++;
    XtSetArg(args[num_args], nhStr(XtNtopMargin), &top_margin); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottomMargin),
             &bottom_margin); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleftMargin), &left_margin); num_args++;
    XtSetArg(args[num_args], nhStr(XtNrightMargin),
             &right_margin); num_args++;
    XtGetValues(wp->w, args, num_args);

    wp->pixel_height = 2 * nhFontHeight(wp->w) + top_margin + bottom_margin;
    wp->pixel_width = COLNO * fs->max_bounds.width
                    + left_margin + right_margin;

    /* Set the new width and height. */
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, wp->pixel_width); num_args++;
    XtSetArg(args[num_args], XtNheight, wp->pixel_height); num_args++;
    XtSetValues(wp->w, args, num_args);
}

void
destroy_status_window_fancy(struct xwindow *wp)
{
    /* If status_information is defined, then it a "text" status window. */
    if (wp->status_information) {
        if (wp->popup) {
            nh_XtPopdown(wp->popup);
            if (!wp->keep_window)
                XtDestroyWidget(wp->popup), wp->popup = (Widget) 0;
        }
        free((genericptr_t) wp->status_information);
        wp->status_information = 0;
    } else {
        destroy_fancy_status(wp);
    }
    if (!wp->keep_window)
        wp->type = NHW_NONE;
}

/*
 * This assumes several things:
 *      + Status has only 2 lines
 *      + That both lines are updated in succession in line order.
 *      + We didn't set stringInPlace on the widget.
 */
void
adjust_status_fancy(struct xwindow *wp, const char *str)
{
    Arg args[2];
    Cardinal num_args;

    if (!wp->status_information) {
        update_fancy_status(TRUE);
        return;
    }

    if (wp->cursy == 0) {
        clear_text_buffer(&wp->status_information->text);
        append_text_buffer(&wp->status_information->text, str, FALSE);
        return;
    }
    append_text_buffer(&wp->status_information->text, str, FALSE);

    /* Set new buffer as text. */
    num_args = 0;
    XtSetArg(args[num_args], XtNstring,
             wp->status_information->text.text); num_args++;
    XtSetValues(wp->w, args, num_args);
}

/* Fancy ================================================================== */
extern const char *const hu_stat[];  /* from eat.c */
extern const char *const enc_stat[]; /* from botl.c */

struct X_status_value {
    /* we have to cast away 'const' when assigning new names */
    const char *name;   /* text name */
    int type;           /* status type */
    Widget w;           /* widget of name/value pair */
    long last_value;    /* value displayed */
    int turn_count;     /* last time the value changed */
    boolean set;        /* if highlighted */
    boolean after_init; /* don't highlight on first change (init) */
    boolean inverted_hilite; /* if highlit due to hilite_status inverse rule */
    Pixel default_fg;   /* what FG color it initialized with */
    int colr, attr;     /* color and attribute */
};

/* valid type values */
#define SV_VALUE 0 /* displays a label:value pair */
#define SV_LABEL 1 /* displays a changable label */
#define SV_NAME  2 /* displays an unchangeable name */

/* for overloaded conditions */
struct ovld_item {
    unsigned long ovl_mask;
    int ff;
};
#define NUM_OVLD 4 /* peak number of overloads for a single field */
struct f_overload {
    unsigned long all_mask;
    struct ovld_item conds[NUM_OVLD];
};

static const struct f_overload *ff_ovld_from_mask(unsigned long);
static const struct f_overload *ff_ovld_from_indx(int);
static void hilight_label(Widget);
static void update_val(struct X_status_value *, long);
static void skip_cond_val(struct X_status_value *);
static void update_color(struct X_status_value *, int);
static boolean name_widget_has_label(struct X_status_value *);
static void apply_hilite_attributes(struct X_status_value *, int);
static const char *width_string(int);
static void create_widget(Widget, struct X_status_value *, int);
static void get_widths(struct X_status_value *, int *, int *);
static void set_widths(struct X_status_value *, int, int);
static Widget init_column(const char *, Widget, Widget, Widget, int *, int);
static void fixup_cond_widths(void);
static Widget init_info_form(Widget, Widget, Widget);

/*
 * Notes:
 * + Alignment needs a different init value, because -1 is an alignment.
 * + Armor Class is an schar, so 256 is out of range.
 * + Blank value is 0 and should never change.
 *
 * - These must be in the same order as the F_foo numbers.
 */
static struct X_status_value shown_stats[NUM_STATS] = {
    /* 0 */
    { "",             SV_NAME,  (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    /* 1 */
    { "Strength",     SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    { "Dexterity",    SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    { "Constitution", SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    { "Intelligence", SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    /* 5 */
    { "Wisdom",       SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    { "Charisma",     SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    /* F_NAME: 7 */
    { "",             SV_LABEL, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    /* F_DLEVEL: 8 */
    { "",             SV_LABEL, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    { "Gold",         SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    /* F_HP: 10 */
    { "Hit Points",   SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    { "Max HP",       SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    { "Power",        SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    { "Max Power",    SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    { "Armor Class",  SV_VALUE, (Widget) 0, 256L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    /* F_XP_LEVL: 15 */
    { "Xp Level",     SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    /* also 15 (overloaded field) */
    /*{ "Hit Dice",   SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },*/
    /* F_EXP_PTS: 16 (optionally displayed) */
    { "Exp Points",   SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    { "Alignment",    SV_VALUE, (Widget) 0,  -2L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    /* 18, optionally displayed */
    { "Time",         SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    /* 19, condtionally present, optionally displayed when present */
    { "Score",        SV_VALUE, (Widget) 0,  -1L, 0, FALSE, FALSE, FALSE, 0, 0, 0 },
    /* F_HUNGER: 20 (blank if 'normal') */
    { "",             SV_NAME,  (Widget) 0,  -1L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    /* F_ENCUMBER: 21 (blank if unencumbered) */
    { "",             SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Trapped",      SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Tethered",     SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Levitating",   SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    /* 25 */
    { "Flying",       SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Riding",       SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Grabbed!",     SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    /* F_STONE: 28 */
    { "Petrifying",   SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Slimed",       SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    /* 30 */
    { "Strangled",    SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Food Pois",    SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Term Ill",     SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    /* F_IN_LAVA: 33 */
    { "Sinking",      SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Held",         SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    /* 35 */
    { "Holding",      SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Blind",        SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Deaf",         SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Stunned",      SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    { "Confused",     SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
    /* F_HALLU: 40 (full spelling truncated due to space limitations) */
    { "Hallucinat",   SV_NAME,  (Widget) 0,   0L, 0, FALSE, TRUE, FALSE, 0, 0, 0 },
};
/*
 * The following are supported by the core but not yet handled here:
 *  bareh      'bare handed' (no weapon and no gloves)
 *  busy       involved in some multi-turn activity, possibly involuntarily
 *  elf_iron   elf being harmed by contact with iron (not implemented)
 *  glowhands  'glowing hands' (inflict confuse monster for next N melee hits)
 *  icy        on or above ice terrain (temporary fumbling; might melt)
 *  parlyz     paralyzed (can't move)
 *  sleeping   asleep (can't move; might wake if attacked)
 *  slippery   'slippery hands' or gloves (will drop non-cursed weapons)
 *  submerged  underwater (severely restricted vision, hampered movement)
 *  unconsc    unconscious (can't move; includes fainted)
 *  woundedl   'wounded legs' (can't kick; temporary dex loss)
 */

/* overloaded condition fields */
static const struct f_overload cond_ovl[] = {
    { (BL_MASK_TRAPPED | BL_MASK_TETHERED),
      { { BL_MASK_TRAPPED, F_TRAPPED },
        { BL_MASK_TETHERED, F_TETHERED } },
    },
    { (BL_MASK_HELD | BL_MASK_HOLDING),
      { { BL_MASK_HELD, F_HELD },
        { BL_MASK_HOLDING, F_HOLDING } },
    },
#if 0   /* not yet implemented */
    { (BL_MASK_BUSY | BL_MASK_PARALYZ | BL_MASK_SLEEPING | BL_MASK_UNCONSC),
      { { BL_MASK_BUSY, F_BUSY }, /* can't move but none of the below... */
        { BL_MASK_PARALYZ, F_PARALYZED }
        { BL_MASK_SLEEPING, F_SLEEPING }
        { BL_MASK_UNCONSC, F_UNCONSCIOUS } },
    },
#endif
};

static const struct f_overload *
ff_ovld_from_mask(unsigned long mask)
{
    const struct f_overload *fo;

    for (fo = cond_ovl; fo < cond_ovl + SIZE(cond_ovl); ++fo) {
        if ((fo->all_mask & mask) != 0L)
            return fo;
    }
    return (struct f_overload *) 0;
}

static const struct f_overload *
ff_ovld_from_indx(int indx) /* F_foo number, index into shown_stats[] */
{
    const struct f_overload *fo;
    int i, ff;

    if (indx > 0) { /* skip 0 (F_DUMMY) */
        for (fo = cond_ovl; fo < cond_ovl + SIZE(cond_ovl); ++fo) {
            for (i = 0; i < NUM_OVLD && (ff = fo->conds[i].ff) > 0; ++i)
                if (ff == indx)
                    return fo;
        }
    }
    return (struct f_overload *) 0;
}

/*
 * Set all widget values to a null string.  This is used after all spacings
 * have been calculated so that when the window is popped up we don't get all
 * kinds of funny values being displayed.
 */
void
null_out_status(void)
{
    int i;
    struct X_status_value *sv;
    Arg args[1];

    for (i = 0, sv = shown_stats; i < NUM_STATS; i++, sv++) {
        switch (sv->type) {
        case SV_VALUE:
            set_value(sv->w, "");
            break;

        case SV_LABEL:
        case SV_NAME:
            XtSetArg(args[0], XtNlabel, "");
            XtSetValues(sv->w, args, ONE);
            break;

        default:
            impossible("null_out_status: unknown type %d\n", sv->type);
            break;
        }
    }
}

/* this is almost an exact duplicate of hilight_value() */
static void
hilight_label(Widget w) /* label widget */
{
    /*
     * This predates STATUS_HILITES.
     * It is used to show any changed item in inverse and gets
     * reset on the next turn.
     */
    swap_fg_bg(w);
}

DISABLE_WARNING_FORMAT_NONLITERAL

static void
update_val(struct X_status_value *attr_rec, long new_value)
{
    static boolean Exp_shown = TRUE, time_shown = TRUE, score_shown = TRUE,
                   Xp_was_HD = FALSE;
    char buf[BUFSZ];
    Arg args[4];

    if (attr_rec->type == SV_LABEL) {
        if (attr_rec == &shown_stats[F_NAME]) {
            Strcpy(buf, g.plname);
            buf[0] = highc(buf[0]);
            Strcat(buf, " the ");
            if (Upolyd) {
                char mnam[BUFSZ];
                int k;

                Strcpy(mnam, pmname(&mons[u.umonnum], Ugender));
                for (k = 0; mnam[k] != '\0'; k++) {
                    if (k == 0 || mnam[k - 1] == ' ')
                        mnam[k] = highc(mnam[k]);
                }
                Strcat(buf, mnam);
            } else
                Strcat(buf,
                       rank_of(u.ulevel, g.pl_character[0], flags.female));

        } else if (attr_rec == &shown_stats[F_DLEVEL]) {
            if (!describe_level(buf, 0)) {
                Strcpy(buf, g.dungeons[u.uz.dnum].dname);
                Sprintf(eos(buf), ", level %d", depth(&u.uz));
            }
        } else {
            impossible("update_val: unknown label type \"%s\"",
                       attr_rec->name);
            return;
        }

        if (strcmp(buf, attr_rec->name) == 0)
            return; /* same */

        /* Set the label.  'name' field is const for most entries;
           we need to cast away that const for this assignment */
        Strcpy((char *) attr_rec->name, buf);
        XtSetArg(args[0], XtNlabel, buf);
        XtSetValues(attr_rec->w, args, ONE);

    } else if (attr_rec->type == SV_NAME) {
        if (attr_rec->last_value == new_value)
            return; /* no change */

        attr_rec->last_value = new_value;

        /* special cases: hunger and encumbrance */
        if (attr_rec == &shown_stats[F_HUNGER]) {
            Strcpy(buf, hu_stat[new_value]);
            (void) mungspaces(buf);
        } else if (attr_rec == &shown_stats[F_ENCUMBER]) {
            Strcpy(buf, enc_stat[new_value]);
        } else if (new_value) {
            Strcpy(buf, attr_rec->name); /* condition name On */
        } else {
            *buf = '\0'; /* condition name Off */
        }

        XtSetArg(args[0], XtNlabel, buf);
        XtSetValues(attr_rec->w, args, ONE);

    } else { /* a value pair */
        boolean force_update = FALSE;

        /* special case: time can be enabled & disabled */
        if (attr_rec == &shown_stats[F_TIME]) {
            if (flags.time && !time_shown) {
                set_name(attr_rec->w, shown_stats[F_TIME].name);
                force_update = TRUE;
                time_shown = TRUE;
            } else if (!flags.time && time_shown) {
                set_name(attr_rec->w, "");
                set_value(attr_rec->w, "");
                time_shown = FALSE;
            }
            if (!time_shown)
                return;

        /* special case: experience points can be enabled & disabled */
        } else if (attr_rec == &shown_stats[F_EXP_PTS]) {
            boolean showexp = flags.showexp && !Upolyd;

            if (showexp && !Exp_shown) {
                set_name(attr_rec->w, shown_stats[F_EXP_PTS].name);
                force_update = TRUE;
                Exp_shown = TRUE;
            } else if (!showexp && Exp_shown) {
                set_name(attr_rec->w, "");
                set_value(attr_rec->w, "");
                Exp_shown = FALSE;
            }
            if (!Exp_shown)
                return;

        /* special case: when available, score can be enabled & disabled */
        } else if (attr_rec == &shown_stats[F_SCORE]) {
#ifdef SCORE_ON_BOTL
            if (flags.showscore && !score_shown) {
                set_name(attr_rec->w, shown_stats[F_SCORE].name);
                force_update = TRUE;
                score_shown = TRUE;
            } else
#endif
            if (!flags.showscore && score_shown) {
                set_name(attr_rec->w, "");
                set_value(attr_rec->w, "");
                score_shown = FALSE;
            }
            if (!score_shown)
                return;

        /* special case: when polymorphed, show "Hit Dice" and disable Exp */
        } else if (attr_rec == &shown_stats[F_XP_LEVL]) {
            if (Upolyd && !Xp_was_HD) {
                force_update = TRUE;
                set_name(attr_rec->w, "Hit Dice");
                Xp_was_HD = TRUE;
            } else if (!Upolyd && Xp_was_HD) {
                force_update = TRUE;
                set_name(attr_rec->w, shown_stats[F_XP_LEVL].name);
                Xp_was_HD = FALSE;
            }
            /* core won't call status_update() for Exp when it hasn't changed
               so do so ourselves (to get Exp_shown flag to match display) */
            if (force_update)
                update_fancy_status_field(F_EXP_PTS, NO_COLOR, HL_UNDEF);
        }

        if (attr_rec->last_value == new_value && !force_update) /* same */
            return;

        attr_rec->last_value = new_value;

        /* Special cases: strength and other characteristics, alignment
           and "clear". */
        if (attr_rec >= &shown_stats[F_STR]
            && attr_rec <= &shown_stats[F_CHA]) {
            static const char fmt1[] = "%ld%s", fmt2[] = "%2ld%s";
            struct xwindow *wp;
            const char *fmt = fmt1, *padding = "";

            /* for full-fledged fancy status, force two digits for all
               six characteristics, followed by three spaces of padding
               to match "/xx" exceptional strength */
            wp = (WIN_STATUS != WIN_ERR) ? &window_list[WIN_STATUS] : 0;
            if (wp && !wp->status_information)
                fmt = fmt2, padding = "   ";

            if (new_value > 18L && attr_rec == &shown_stats[F_STR]) {
                if (new_value > 118L) /* 19..25 encoded as 119..125 */
                    Sprintf(buf, fmt, new_value - 100L, padding);
                else if (new_value < 118L) /* 18/01..18/99 as 19..117*/
                    Sprintf(buf, "18/%02ld", new_value - 18L);
                else
                    Strcpy(buf, "18/**"); /* 18/100 encoded as 118 */
            } else { /* non-strength or less than 18/01 strength (3..18) */
                Sprintf(buf, fmt, new_value, padding); /* 3..25 */
            }
        } else if (attr_rec == &shown_stats[F_ALIGN]) {
            Strcpy(buf, (new_value == A_CHAOTIC) ? "Chaotic"
                        : (new_value == A_NEUTRAL) ? "Neutral" : "Lawful");
        } else {
            Sprintf(buf, "%ld", new_value);
        }
        set_value(attr_rec->w, buf);
    }

    /*
     * Now highlight the changed information.  Don't highlight Time because
     * it's continually changing.  For others, don't highlight if this is
     * the first update.  If already highlighted, don't change it unless
     * it's being set to blank (where that item should be reset now instead
     * of showing highlighted blank until the next expiration check).
     *
     * 3.7:  highlight non-labelled 'name' items (conditions plus hunger
     * and encumbrance) when they come On.  For all conditions going Off,
     * or changing to not-hungry or not-encumbered, there's nothing to
     * highlight because the field becomes blank.
     */
    if (attr_rec->after_init) {
        /* toggle if not highlighted and just set to nonblank or if
           already highlighted and just set to blank */
        if (attr_rec != &shown_stats[F_TIME] && !attr_rec->set ^ !*buf) {
            /* But don't hilite if inverted from status_hilite since
               it will already be hilited by apply_hilite_attributes(). */
            if (!attr_rec->inverted_hilite) {
                if (attr_rec->type == SV_VALUE)
                    hilight_value(attr_rec->w);
                else
                    hilight_label(attr_rec->w);
            }
            attr_rec->set = !attr_rec->set;
        }
        attr_rec->turn_count = 0;
    } else {
        XtSetArg(args[0], XtNforeground, &attr_rec->default_fg);
        XtGetValues(attr_rec->w, args, ONE);
        attr_rec->after_init = TRUE;
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* overloaded condition is being cleared without going through update_val()
   so that an alternate can be shown; put this one back to default settings */
static void
skip_cond_val(struct X_status_value *sv)
{
    sv->last_value = 0L; /* Off */
    if (sv->set) {
        /* if condition was highlighted and the alternate value has
           also requested to be highlighted, it used its own copy of
           'set' but the same widget so the highlighing got toggled
           off; this will turn in back on in that exceptional case */
        hilight_label(sv->w);
        sv->set = FALSE;
    }
}

static void
update_color(struct X_status_value *sv, int color)
{
    Pixel pixel = 0;
    Arg args[1];
    XrmValue source;
    XrmValue dest;
    Widget w = (sv->type == SV_LABEL || sv->type == SV_NAME) ? sv->w
               : get_value_widget(sv->w);

    if (color == NO_COLOR) {
        if (sv->after_init)
            pixel = sv->default_fg;
        sv->colr = NO_COLOR;
    } else {
        source.addr = (XPointer) fancy_status_hilite_colors[color];
        source.size = (unsigned int) strlen((const char *) source.addr) + 1;
        dest.size = (unsigned int) sizeof (Pixel);
        dest.addr = (XPointer) &pixel;
        if (XtConvertAndStore(w, XtRString, &source, XtRPixel, &dest))
            sv->colr = color;
    }
    if (pixel != 0) {
        char *arg_name = (sv->set || sv->inverted_hilite) ? XtNbackground
                         : XtNforeground;

        XtSetArg(args[0], arg_name, pixel);
        XtSetValues(w, args, ONE);
    }
}

static boolean
name_widget_has_label(struct X_status_value *sv)
{
    Arg args[1];
    const char *label;

    XtSetArg(args[0], XtNlabel, &label);
    XtGetValues(sv->w, args, ONE);
    return strlen(label) > 0;
}

static void
apply_hilite_attributes(struct X_status_value *sv, int attributes)
{
    boolean attr_inversion = ((HL_INVERSE & attributes)
                              && (sv->type != SV_NAME
                                  || name_widget_has_label(sv)));

    if (sv->inverted_hilite != attr_inversion) {
        sv->inverted_hilite = attr_inversion;
        if (!sv->set) {
            if (sv->type == SV_VALUE)
                hilight_value(sv->w);
            else
                hilight_label(sv->w);
        }
    }
    sv->attr = attributes;
    /* Could possibly add more attributes here: HL_ATTCLR_DIM,
       HL_ATTCLR_BLINK, HL_ATTCLR_ULINE, and HL_ATTCLR_BOLD. If so,
       extract the above into its own function apply_hilite_inverse()
       and each other attribute into its own to keep the code clean. */
}

/*
 * Update the displayed status.  The current code in botl.c updates
 * two lines of information.  Both lines are always updated one after
 * the other.  So only do our update when we update the second line.
 *
 * Information on the first line:
 *      name, characteristics, alignment, score
 *
 * Information on the second line:
 *      dlvl, gold, hp, power, ac, {level & exp or HD **}, time,
 *      status * (stone, slime, strngl, foodpois, termill,
 *                hunger, encumbrance, lev, fly, ride,
 *                blind, deaf, stun, conf, hallu)
 *
 *  [*] order of status fields is different on tty.
 * [**] HD is shown instead of level and exp if Upolyd.
 */
static void
update_fancy_status_field(int i, int color, int attributes)
{
    struct X_status_value *sv = &shown_stats[i];
    unsigned long condmask = 0L;
    long val = 0L;

    switch (i) {
        case F_DUMMY:
            val = 0L;
            break;
        case F_STR:
            val = (long) ACURR(A_STR);
            break;
        case F_DEX:
            val = (long) ACURR(A_DEX);
            break;
        case F_CON:
            val = (long) ACURR(A_CON);
            break;
        case F_INT:
            val = (long) ACURR(A_INT);
            break;
        case F_WIS:
            val = (long) ACURR(A_WIS);
            break;
        case F_CHA:
            val = (long) ACURR(A_CHA);
            break;
        /*
         * Label stats.  With the exceptions of hunger and encumbrance
         * these are either on or off.  Pleae leave the ternary operators
         * the way they are.  I want to specify 0 or 1, not a boolean.
         */
        case F_HUNGER:
            val = (long) u.uhs;
            break;
        case F_ENCUMBER:
            val = (long) near_capacity();
            break;

        case F_TRAPPED: /* belongs with non-fatal but fits with 'other' */
            condmask = BL_MASK_TRAPPED;
            break;
        case F_TETHERED: /* overloaded with 'trapped' */
            condmask = BL_MASK_TETHERED;
            break;
        /* 'other' status conditions */
        case F_LEV:
            condmask = BL_MASK_LEV;
            break;
        case F_FLY:
            condmask = BL_MASK_FLY;
            break;
        case F_RIDE:
            condmask = BL_MASK_RIDE;
            break;
        /* fatal status conditions */
        case F_GRABBED:
            condmask = BL_MASK_GRAB;
            break;
        case F_STONE:
            condmask = BL_MASK_STONE;
            break;
        case F_SLIME:
            condmask = BL_MASK_SLIME;
            break;
        case F_STRNGL:
            condmask = BL_MASK_STRNGL;
            break;
        case F_FOODPOIS:
            condmask = BL_MASK_FOODPOIS;
            break;
        case F_TERMILL:
            condmask = BL_MASK_TERMILL;
            break;
        case F_IN_LAVA: /* could overload with 'trapped' but is more severe */
            condmask = BL_MASK_INLAVA;
            break;
        /* non-fatal status conditions */
        case F_HELD:
            condmask = BL_MASK_HELD;
            break;
        case F_HOLDING: /* belongs with 'other' but overloads 'held' */
            condmask = BL_MASK_HOLDING;
            break;
        case F_BLIND:
            condmask = BL_MASK_BLIND;
            break;
        case F_DEAF:
            condmask = BL_MASK_DEAF;
            break;
        case F_STUN:
            condmask = BL_MASK_STUN;
            break;
        case F_CONF:
            condmask = BL_MASK_CONF;
            break;
        case F_HALLU:
            condmask = BL_MASK_HALLU;
            break;

        case F_NAME:
        case F_DLEVEL:
            val = (long) 0L;
            break; /* special */

        case F_GOLD:
            val = money_cnt(g.invent);
            if (val < 0L)
                val = 0L; /* ought to issue impossible() and discard gold */
            break;
        case F_HP:
            val = (long) (Upolyd ? (u.mh > 0 ? u.mh : 0)
                                 : (u.uhp > 0 ? u.uhp : 0));
            break;
        case F_MAXHP:
            val = (long) (Upolyd ? u.mhmax : u.uhpmax);
            break;
        case F_POWER:
            val = (long) u.uen;
            break;
        case F_MAXPOWER:
            val = (long) u.uenmax;
            break;
        case F_AC:
            val = (long) u.uac;
            break;
        case F_XP_LEVL:
            val = (long) (Upolyd ? mons[u.umonnum].mlevel : u.ulevel);
            break;
        case F_EXP_PTS:
            val = flags.showexp ? u.uexp : 0L;
            break;
        case F_ALIGN:
            val = (long) u.ualign.type;
            break;
        case F_TIME:
            val = flags.time ? (long) g.moves : 0L;
            break;
        case F_SCORE:
#ifdef SCORE_ON_BOTL
            val = flags.showscore ? botl_score() : 0L;
#else
            val = 0L;
#endif
            break;
        default: {
            /*
             * There is a possible infinite loop that occurs with:
             *
             *  impossible->pline->flush_screen->bot->bot{1,2}->
             *  putstr->adjust_status->update_other->impossible
             *
             * Break out with this.
             */
            static boolean isactive = FALSE;

            if (!isactive) {
                isactive = TRUE;
                impossible("update_other: unknown shown value");
                isactive = FALSE;
            }
            val = 0L;
            break;
        } /* default */
    } /* switch */

    if (condmask) {
        const struct f_overload *fo = ff_ovld_from_mask(condmask);

        val = ((X11_condition_bits & condmask) != 0L);
        /* if we're turning an overloaded field Off, don't do it if any
           of the other alternatives are being set On because we would
           clobber that if the other one happens to be drawn first */
        if (!val && fo && (X11_condition_bits & fo->all_mask) != 0L) {
            skip_cond_val(sv);
            return;
        }
    }
    update_val(sv, val);
    if (color != sv->colr)
        update_color(sv, color);
    if (attributes != sv->attr)
        apply_hilite_attributes(sv, attributes);
}

/* fully update status after bl_flush or window resize */
static void
update_fancy_status(boolean force_update)
{
    static boolean old_showtime, old_showexp, old_showscore;
    static int old_upolyd = -1; /* -1: force first time update */
    int i;

    if (force_update
        || Upolyd != old_upolyd /* Xp vs HD */
        || flags.time != old_showtime
        || flags.showexp != old_showexp
        || flags.showscore != old_showscore) {
        /* update everything; usually only need this on the very first
           time, then later if the window gets resized or if poly/unpoly
           triggers Xp <-> HD switch or if an optional field gets
           toggled off since there won't be a status_update() call for
           the no longer displayed field; we're a bit more conservative
           than that and do this when toggling on as well as off */
        for (i = 0; i < NUM_STATS; i++)
            update_fancy_status_field(i, NO_COLOR, HL_UNDEF);
        old_condition_bits = X11_condition_bits;

        old_upolyd = Upolyd;
        old_showtime = flags.time;
        old_showexp = flags.showexp;
        old_showscore = flags.showscore;
    }
}

/*
 * Turn off hilighted status values after a certain amount of turns.
 */
void
check_turn_events(void)
{
    int i;
    struct X_status_value *sv;
    int hilight_time = 1;

#ifdef STATUS_HILITES
    if (iflags.hilite_delta)
        hilight_time = (int) iflags.hilite_delta;
#endif
    for (sv = shown_stats, i = 0; i < NUM_STATS; i++, sv++) {
        if (!sv->set)
            continue;

        if (sv->turn_count++ >= hilight_time) {
            /* unhighlights by toggling a highlighted item back off again,
               unless forced inverted by a status_hilite rule */
            if (!sv->inverted_hilite) {
                if (sv->type == SV_VALUE)
                    hilight_value(sv->w);
                else
                    hilight_label(sv->w);
            }
            sv->set = FALSE;
        }
    }
}

/* Initialize alternate status ============================================ */

/* Return a string for the initial width, so use longest possible value. */
static const char *
width_string(int sv_index)
{
    switch (sv_index) {
    case F_DUMMY:
        return " ";

    case F_STR:
        return "018/**";
    case F_DEX:
    case F_CON:
    case F_INT:
    case F_WIS:
    case F_CHA:
        return "088"; /* all but str never get bigger */

    case F_HUNGER:
        return "Satiated";
    case F_ENCUMBER:
        return "Overloaded";

    case F_LEV:
    case F_FLY:
    case F_RIDE:
    case F_TRAPPED:
    case F_TETHERED:
    case F_GRABBED:
    case F_STONE:
    case F_SLIME:
    case F_STRNGL:
    case F_FOODPOIS:
    case F_TERMILL:
    case F_IN_LAVA:
    case F_HELD:
    case F_HOLDING:
    case F_BLIND:
    case F_DEAF:
    case F_STUN:
    case F_CONF:
    case F_HALLU:
        return shown_stats[sv_index].name;

    case F_NAME:
    case F_DLEVEL:
        return ""; /* longest possible value not needed for these */

    case F_HP:
    case F_MAXHP:
        return "9999";
    case F_POWER:
    case F_MAXPOWER:
        return "9999";
    case F_AC:
        return "-127";
    case F_XP_LEVL:
        return "99";
    case F_GOLD:
        /* strongest hero can pick up roughly 30% of this much */
        return "999999"; /* same limit as tty */
    case F_EXP_PTS:
    case F_TIME:
    case F_SCORE:
        return "123456789"; /* a tenth digit will still fit legibly */
    case F_ALIGN:
        return "Neutral";
    }
    impossible("width_string: unknown index %d\n", sv_index);
    return "";
}

static void
create_widget(Widget parent, struct X_status_value *sv, int sv_index)
{
    Arg args[4];
    Cardinal num_args;

    switch (sv->type) {
    case SV_VALUE:
        sv->w = create_value(parent, sv->name);
        set_value(sv->w, width_string(sv_index));
        break;
    case SV_LABEL:
        /* Labels get their own buffer. */
        sv->name = (char *) alloc(BUFSZ);
        /* we need to cast away 'const' when assigning a value */
        *(char *) (sv->name) = '\0';

        num_args = 0;
        XtSetArg(args[num_args], XtNborderWidth, 0); num_args++;
        XtSetArg(args[num_args], XtNinternalHeight, 0); num_args++;
        sv->w = XtCreateManagedWidget((sv_index == F_NAME)
                                         ? "name"
                                         : "dlevel",
                                      labelWidgetClass, parent,
                                      args, num_args);
        break;
    case SV_NAME: {
        char buf[BUFSZ];
        const char *txt;
        const struct f_overload *fo = ff_ovld_from_indx(sv_index);
        int baseindx = fo ? fo->conds[0].ff : sv_index;

        if (sv_index != baseindx) {
            /* this code isn't actually executed; only the base condition
               is in one of the fancy status columns and only the fields
               in those columns are passed to this routine; the real
               initialization--this same assignment--for overloaded
               conditions takes place at the end of create_fancy_status() */
            sv->w = shown_stats[baseindx].w;
            break;
        }
        txt = width_string(sv_index); /* for conditions, it's just sv->name */
        if (fo) {
            int i, ff, altln, ln = (int) strlen(txt);

            /* make the initial value have the width of the longest of
               these overloaded conditions; used for widget sizing, not for
               display, and ultimately only matters if one of the overloads
               happens to be the longest string in its whole column */
            for (i = 1; i < NUM_OVLD && (ff = fo->conds[i].ff) > 0; ++i)
                if ((altln = (int) strlen(width_string(ff))) > ln)
                    ln = altln;
            Sprintf(buf, "%*s", ln, txt);
            txt = buf;
        }
        num_args = 0;
        XtSetArg(args[0], XtNlabel, txt); num_args++;
        XtSetArg(args[num_args], XtNborderWidth, 0); num_args++;
        XtSetArg(args[num_args], XtNinternalHeight, 0); num_args++;
        sv->w = XtCreateManagedWidget(sv->name, labelWidgetClass, parent,
                                      args, num_args);
        break;
    }
    default:
        panic("create_widget: unknown type %d", sv->type);
    }
}

/*
 * Get current width of value.  width2p is only valid for SV_VALUE types.
 */
static void
get_widths(struct X_status_value *sv, int *width1p, int *width2p)
{
    Arg args[1];
    Dimension width;

    switch (sv->type) {
    case SV_VALUE:
        *width1p = get_name_width(sv->w);
        *width2p = get_value_width(sv->w);
        break;
    case SV_LABEL:
    case SV_NAME:
        XtSetArg(args[0], XtNwidth, &width);
        XtGetValues(sv->w, args, ONE);
        *width1p = width;
        *width2p = 0;
        break;
    default:
        panic("get_widths: unknown type %d", sv->type);
    }
}

static void
set_widths(struct X_status_value *sv, int width1, int width2)
{
    Arg args[1];

    switch (sv->type) {
    case SV_VALUE:
        set_name_width(sv->w, width1);
        set_value_width(sv->w, width2);
        break;
    case SV_LABEL:
    case SV_NAME:
        XtSetArg(args[0], XtNwidth, (width1 + width2));
        XtSetValues(sv->w, args, ONE);
        break;
    default:
        panic("set_widths: unknown type %d", sv->type);
    }
}

static Widget
init_column(const char *name, Widget parent, Widget top, Widget left,
            int *col_indices, int xtrawidth)
{
    Widget form;
    Arg args[4];
    Cardinal num_args;
    int max_width1, width1, max_width2, width2;
    int *ip;
    struct X_status_value *sv;

    num_args = 0;
    if (top != (Widget) 0) {
        XtSetArg(args[num_args], nhStr(XtNfromVert), top); num_args++;
    }
    if (left != (Widget) 0) {
        XtSetArg(args[num_args], nhStr(XtNfromHoriz), left); num_args++;
    }
    /* this was 0 but that resulted in the text being crammed together */
    XtSetArg(args[num_args], nhStr(XtNdefaultDistance), 2); num_args++;
    form = XtCreateManagedWidget(name, formWidgetClass, parent,
                                 args, num_args);

    max_width1 = max_width2 = 0;
    for (ip = col_indices; *ip >= 0; ip++) {
        sv = &shown_stats[*ip];
        create_widget(form, sv, *ip); /* will set init width */
        if (ip != col_indices) {      /* not first */
            num_args = 0;
            XtSetArg(args[num_args], nhStr(XtNfromVert),
                     shown_stats[*(ip - 1)].w); num_args++;
            XtSetValues(sv->w, args, num_args);
        }
        get_widths(sv, &width1, &width2);
        if (width1 > max_width1)
            max_width1 = width1;
        if (width2 > max_width2)
            max_width2 = width2;
    }

    /* insert some extra spacing between columns */
    max_width1 += xtrawidth;

    for (ip = col_indices; *ip >= 0; ip++) {
        set_widths(&shown_stats[*ip], max_width1, max_width2);
    }

    /* There is room behind the end marker for the two widths. */
    *++ip = max_width1;
    *++ip = max_width2;

    return form;
}

/*
 * These are the orders of the displayed columns.  Change to suit.  The -1
 * indicates the end of the column.  The two numbers after that are used
 * to store widths that are calculated at run-time.
 *
 * 3.7:  changed so that all 6 columns have 8 rows, but a few entries
 * are left blank <>.  Exp-points, Score, and Time are optional depending
 * on run-time settings; Xp-level is replaced by Hit-Dice (and Exp-points
 * suppressed) when the hero is polymorphed.  Title and Dungeon-Level span
 * two columns and might expand to more if 'hitpointbar' is implemented.
 *
 Title ("Plname the Rank")   <>            <>           <>          <>
 Dungeon-Branch-and-Level    <>           Hunger       Grabbed     Held
 Hit-points    Max-HP       Strength      Encumbrance  Petrifying  Blind
 Power-points  Max-Power    Dexterity     Trapped      Slimed      Deaf
 Armor-class   Alignment    Constitution  Levitation   Strangled   Stunned
 Xp-level     [Exp-points]  Intelligence  Flying       Food-Pois   Confused
 Gold         [Score]       Wisdom        Riding       Term-Ill    Hallucinatg
  <>          [Time]        Charisma       <>          Sinking      <>
 *
 * A seventh column is going to be needed to fit in more conditions.
 *
 * Possible enhancement:  if Exp-points and Score are both disabled, move
 * Gold to the Exp-points slot.
 */

/* including F_DUMMY makes the three status condition columns evenly
   spaced with regard to the adjacent characteristics (Str,Dex,&c) column;
   we lose track of the Widget pointer for F_DUMMY, each use clobbering the
   one before, leaving the one from leftover_indices[]; since they're never
   updated, that shouldn't matter */
static int status_indices[3][11] = {
    { F_DUMMY, F_HUNGER, F_ENCUMBER, F_TRAPPED,
      F_LEV, F_FLY, F_RIDE, F_DUMMY, -1, 0, 0 },
    { F_DUMMY, F_GRABBED, F_STONE, F_SLIME, F_STRNGL,
      F_FOODPOIS, F_TERMILL, F_IN_LAVA, -1, 0, 0 },
    { F_DUMMY, F_HELD, F_BLIND, F_DEAF, F_STUN,
      F_CONF, F_HALLU, F_DUMMY, -1, 0, 0 },
};
/* used to fill up the empty space to right of 3rd status condition column */
static int leftover_indices[] = { F_DUMMY, -1, 0, 0 };
/* -2: top two rows of these columns are reserved for title and location */
static int col1_indices[11 - 2] = {
    F_HP,    F_POWER,    F_AC,    F_XP_LEVL, F_GOLD,  F_DUMMY,  -1, 0, 0
};
static int col2_indices[11 - 2] = {
    F_MAXHP, F_MAXPOWER, F_ALIGN, F_EXP_PTS, F_SCORE, F_TIME,   -1, 0, 0
};
static int characteristics_indices[11 - 2] = {
    F_STR, F_DEX, F_CON, F_INT, F_WIS, F_CHA, -1, 0, 0
};

/*
 * Produce a form that looks like the following:
 *
 *                title
 *               location
 * col1_indices[0]      col2_indices[0]      col3_indices[0]
 * col1_indices[1]      col2_indices[1]      col3_indices[1]
 *    ...                  ...                  ...
 * col1_indices[5]      col2_indices[5]      col3_indices[5]
 *
 * The status conditions are managed separately and appear to the right
 * of this form.
 *
 * TODO:  widen title field and implement hitpoint bar on it.
 */
static Widget
init_info_form(Widget parent, Widget top, Widget left)
{
    Widget form, col1, col2;
    struct X_status_value *sv_name, *sv_dlevel;
    Arg args[6];
    Cardinal num_args;
    int total_width, *ip;

    num_args = 0;
    if (top != (Widget) 0) {
        XtSetArg(args[num_args], nhStr(XtNfromVert), top); num_args++;
    }
    if (left != (Widget) 0) {
        XtSetArg(args[num_args], nhStr(XtNfromHoriz), left); num_args++;
    }
    XtSetArg(args[num_args], nhStr(XtNdefaultDistance), 2); num_args++;
    form = XtCreateManagedWidget("status_info", formWidgetClass, parent,
                                 args, num_args);

    /* top line/row of form */
    sv_name = &shown_stats[F_NAME]; /* title */
    create_widget(form, sv_name, F_NAME);

    /* second line/row */
    sv_dlevel = &shown_stats[F_DLEVEL]; /* location */
    create_widget(form, sv_dlevel, F_DLEVEL);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), sv_name->w); num_args++;
    XtSetValues(sv_dlevel->w, args, num_args);

    /* there are 3 columns beneath but top 2 rows are centered over first 2 */
    col1 = init_column("name_col1", form, sv_dlevel->w, (Widget) 0,
                       col1_indices, 0);
    col2 = init_column("name_col2", form, sv_dlevel->w, col1,
                       col2_indices, 5);
    (void) init_column("status_characteristics", form, sv_dlevel->w, col2,
                       characteristics_indices, 15);

    /* Add calculated widths. */
    for (ip = col1_indices; *ip >= 0; ip++)
        ; /* skip to end */
    total_width = *++ip;
    total_width += *++ip;
    for (ip = col2_indices; *ip >= 0; ip++)
        ; /* skip to end */
    total_width += *++ip;
    total_width += *++ip;

    XtSetArg(args[0], XtNwidth, total_width);
    XtSetValues(sv_name->w, args, ONE);
    XtSetArg(args[0], XtNwidth, total_width);
    XtSetValues(sv_dlevel->w, args, ONE);

    return form;
}

/* give the three status condition columns the same width */
static void
fixup_cond_widths(void)
{
    int pass, i, *ip, w1, w2;

    w1 = w2 = 0;
    for (pass = 1; pass <= 2; ++pass) { /* two passes... */
        for (i = 0; i < 3; i++) { /* three columns */
            for (ip = status_indices[i]; *ip != -1; ++ip) { /* X fields */
                /* pass 1: find -1;  pass 2: update field widths, find -1 */
                if (pass == 2)
                    set_widths(&shown_stats[*ip], w1, w2);
            }
            /* found -1; the two slots beyond it contain column widths */
            if (pass == 1) { /* pass 1: collect maxima */
                if (ip[1] > w1)
                    w1 = ip[1];
                if (ip[2] > w2)
                    w2 = ip[2];
            } else { /* pass 2: update column widths with maxima */
                ip[1] = w1;
                ip[2] = w2;
            }
        }
        /* ascetics:  expand the maximum width to make cond columns wider */
        if (pass == 1) {
            w1 += 15;
            if (w2 > 0)
                w2 += 15;
        }
    }
}

/*
 * Create the layout for the fancy status.  Return a form widget that
 * contains everything.
 */
static Widget
create_fancy_status(Widget parent, Widget top)
{
    Widget form; /* The form that surrounds everything. */
    Widget w;
    Arg args[8];
    Cardinal num_args;
    char buf[32];
    const struct f_overload *fo;
    int i, ff;

    num_args = 0;
    if (top != (Widget) 0) {
        XtSetArg(args[num_args], nhStr(XtNfromVert), top); num_args++;
    }
    XtSetArg(args[num_args], nhStr(XtNdefaultDistance), 2); num_args++;
    XtSetArg(args[num_args], XtNborderWidth, 0); num_args++;
    XtSetArg(args[num_args], XtNorientation, XtorientHorizontal); num_args++;
    form = XtCreateManagedWidget("fancy_status", panedWidgetClass, parent,
                                 args, num_args);

    w = init_info_form(form, (Widget) 0, (Widget) 0);
#if 0   /* moved to init_info_form() */
    w = init_column("status_characteristics", form, (Widget) 0, w,
                    characteristics_indices, 15);
#endif
    for (i = 0; i < 3; i++) {
        Sprintf(buf, "status_condition%d", i + 1);
        w = init_column(buf, form, (Widget) 0, w, status_indices[i], 0);
    }
    fixup_cond_widths(); /* make all 3 status_conditionN columns same width */
    /* extra dummy 'column' to allocate any remaining space below the map */
    (void) init_column("status_leftover", form, (Widget) 0, w,
                       leftover_indices, 0);

    /* handle overloading; extra conditions don't start out in any column
       so need to be initialized separately; the only initialization they
       need is to share the widget of the base condition which is present
       in one of the columns [could be deferred until first use] */
    for (fo = cond_ovl; fo < cond_ovl + SIZE(cond_ovl); ++fo)
        for (i = 1; i < NUM_OVLD && (ff = fo->conds[i].ff) > 0; ++i)
            if (!shown_stats[ff].w)
                shown_stats[ff].w = shown_stats[fo->conds[0].ff].w;

    return form;
}

static void
destroy_fancy_status(struct xwindow *wp)
{
    int i;
    struct X_status_value *sv;

    if (!wp->keep_window)
        XtDestroyWidget(wp->w), wp->w = (Widget) 0;

    for (i = 0, sv = shown_stats; i < NUM_STATS; i++, sv++)
        if (sv->type == SV_LABEL) {
            free((genericptr_t) sv->name);
            sv->name = 0;
        }
}

/*winstat.c*/

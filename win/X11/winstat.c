/* NetHack 5.0	winstat.c	$NHDT-Date: 1781973110 2026/06/20 16:31:50 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.50 $ */
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

#define F_VERS     41 /* version info */

#define F_WEAPON   42
#define F_ARMOR    43
#define F_TERRAIN  44
#define F_BAREH    45
#define F_GLOWHANDS 46
#define F_ICY      47
#define F_BUSY     48
#define F_PARALYZED 49
#define F_SLEEPING 50
#define F_UNCONSCIOUS 51
#define F_IRON     52
#define F_SLIPPERY 53
#define F_SUBMERGED 54
#define F_WOUNDEDL 55
#define NUM_STATS  56

static int condcolor(long, unsigned long *);
static int condattr(long, unsigned long *);
static Widget create_tty_status(Widget, Widget);
static void stat_resized(Widget, XtPointer, XtPointer);
static void update_fancy_status_field(int, const char *, int, int);
static void update_fancy_status(boolean);
static Widget create_fancy_status(Widget, Widget);
static void destroy_fancy_status(struct xwindow *);
static void create_status_window_fancy(struct xwindow *, boolean, Widget);
static void create_status_window_tty(struct xwindow *, boolean, Widget);
static void destroy_status_window_fancy(struct xwindow *);
static void destroy_status_window_tty(struct xwindow *);
#ifndef STATUS_HILITES
static void adjust_status_fancy(struct xwindow *, const char *);
static void adjust_status_tty(struct xwindow *, const char *);
#endif
static void set_percent(int, int, int);
static void tty_status_exposed(Widget, XtPointer, XtPointer);
static void tty_status_redraw(Widget);
static int tty_render_field(Widget, int, int, int, enum statusfields);
static void tty_status_colors(Widget, int, int, Pixel *, Pixel *);
static int tty_render_text(Widget, const XRectangle *, int, int, const char *,
                           int, int, enum statusfields);

static unsigned long X11_condition_bits, old_condition_bits;
static int X11_status_colors[MAXBLSTATS],
           old_field_colors[MAXBLSTATS],
           old_cond_colors[32];
static int hpbar_percent, hpbar_color;

static enum statusfields X11_fieldorder_2[][18] = {
    { BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_ALIGN,
      BL_SCORE, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH,
      BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH },
    { BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX,
      BL_AC, BL_XP, BL_EXP, BL_HD, BL_TIME, BL_HUNGER, BL_CAP,
      BL_CONDITION, BL_WEAPON, BL_ARMOR, BL_TERRAIN, BL_VERS }
};

static enum statusfields X11_fieldorder_3[][13] = {
    { BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_SCORE, BL_FLUSH },
    { BL_ALIGN, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX,
      BL_AC, BL_XP, BL_EXP, BL_HD, BL_HUNGER, BL_CAP,
      BL_FLUSH },
    { BL_LEVELDESC, BL_TIME, BL_CONDITION, BL_WEAPON, BL_ARMOR, BL_TERRAIN,
      BL_VERS, BL_FLUSH }
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
    { BL_MASK_HOLDING, "Holding" },
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

static const char *const fancy_status_hilite_colors[] = {
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

struct tty_status_field {
    char *text;
    int color;
    unsigned attrs;
    int chg;
    int percent;
};

struct tty_cond_field {
    const char *text;
    int color;
    unsigned attrs;
};

static Widget X11_status_widget;
static struct tty_status_field X11_status_labels[MAXBLSTATS];
static struct tty_cond_field X11_cond_labels[32]; /* Ugh */

static struct xwindow *xw_status_win;

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
        for (i = HL_ATTCLR_BOLD; i < BL_ATTCLR_MAX; ++i) {
            if (bmarray[i] && (bm & bmarray[i])) {
                switch(i) {
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
    if (X11_status_widget && !enable && 0 <= fieldidx && fieldidx < MAXBLSTATS) {
        free(X11_status_labels[fieldidx].text);
        X11_status_labels[fieldidx].text = NULL;
        tty_status_redraw(X11_status_widget);
    }
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

DISABLE_WARNING_FORMAT_NONLITERAL

/* core requests updating one status field (or is indicating that it's time
   to flush all updated fields); tty-style handling */
static void
X11_status_update_tty(
    int fld,
    genericptr_t ptr,
    int chg,
    int percent,
    int color,
    unsigned long *colormasks) /* bitmask of highlights for conditions */
{
#ifndef STATUS_HILITES
    nhUse(colormasks);
#endif

    switch (fld) {
    case BL_RESET:
    case BL_FLUSH:
        tty_status_redraw(X11_status_widget);
        break;

    case BL_CONDITION:
        {
            unsigned long cond = *(unsigned long const *)ptr;
            for (unsigned j = 0; j < SIZE(tt_condorder); ++j) {
                struct tty_cond_field *fldp = &X11_cond_labels[j];
                fldp->color = NO_COLOR;
                fldp->attrs = 0;
                if ((cond & tt_condorder[j].mask) != 0) {
                    fldp->text = tt_condorder[j].text;
                } else {
                    fldp->text = NULL;
                }
            }
#ifdef STATUS_HILITES
            for (unsigned i = 0; i < CLR_MAX; ++i) {
                unsigned long mask = colormasks[i];
                for (unsigned j = 0; j < SIZE(tt_condorder); ++j) {
                    if ((mask & tt_condorder[j].mask) != 0) {
                        struct tty_cond_field *fldp = &X11_cond_labels[j];
                        fldp->color = i;
                    }
                }
            }
            for (unsigned i = CLR_MAX; i < BL_ATTCLR_MAX; ++i) {
                unsigned long mask = colormasks[i];
                for (unsigned j = 0; j < SIZE(tt_condorder); ++j) {
                    if ((mask & tt_condorder[j].mask) != 0) {
                        struct tty_cond_field *fldp = &X11_cond_labels[j];
                        fldp->attrs |= 0x1 << (i - HL_ATTCLR_NONE);
                    }
                }
            }
#endif
        }
        break;

    default:
        if (0 <= fld && fld < SIZE(X11_status_labels)) {
            struct tty_status_field *fldp = &X11_status_labels[fld];
            const char *str = (const char *) ptr;
            free(fldp->text);
            fldp->text = dupstr(str);
            fldp->chg = chg;
            fldp->percent = percent;
            fldp->color = color & 0xFF;
            fldp->attrs = color >> 8;
        }
        break;
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

/*ARGSUSED*/
static void
X11_status_update_fancy(
    int fld,
    genericptr_t ptr,
    int chg UNUSED, int percent,
    int colrattr,
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
        { BL_VERS, F_VERS },
        { BL_WEAPON, F_WEAPON },
        { BL_ARMOR, F_ARMOR },
        { BL_TERRAIN, F_TERRAIN },
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
        { BL_MASK_BAREH, F_BAREH },
        { BL_MASK_GLOWHANDS, F_GLOWHANDS },
        { BL_MASK_ICY, F_ICY },
        { BL_MASK_BUSY, F_BUSY },
        { BL_MASK_PARLYZ, F_PARALYZED },
        { BL_MASK_SLEEPING, F_SLEEPING },
        { BL_MASK_UNCONSC, F_UNCONSCIOUS },
        { BL_MASK_ELF_IRON, F_IRON },
        { BL_MASK_SLIPPERY, F_SLIPPERY },
        { BL_MASK_SUBMERGED, F_SUBMERGED },
        { BL_MASK_WOUNDEDL, F_WOUNDEDL },
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
                            NULL,
                            condcolor(mask_to_fancyfield[i].mask, colormasks),
                            condattr(mask_to_fancyfield[i].mask, colormasks));
            old_condition_bits = X11_condition_bits; /* remember 'On' bits */
        }
    } else {
        int colr, attr;

        if ((colrattr & 0x00ff) >= CLR_MAX)
            colrattr = (colrattr & ~0x00ff) | NO_COLOR;
        colr = colrattr & 0x00ff; /* guaranteed to be >= 0 and < CLR_MAX */
        attr = (colrattr >> 8) & 0x00ff;

        for (i = 0; i < SIZE(bl_to_fancyfield); i++)
            if (bl_to_fancyfield[i].bl == fld) {
                update_fancy_status_field(bl_to_fancyfield[i].ff,
                                          (const char *) ptr, colr, attr);
                break;
            }

        /* Hit point bar */
        if (fld == BL_HP) {
            set_percent(F_NAME, iflags.wc2_hitpointbar ? percent : 0, colr);
        }
    }
}

void
X11_status_update(
    int fld,
    genericptr_t ptr,
    int chg, int percent,
    int color,
    unsigned long *colormasks)
{
    if (fld < BL_RESET || fld >= MAXBLSTATS)
        panic("X11_status_update(%d) -- invalid field", fld);

    if (appResources.fancy_status)
        X11_status_update_fancy(fld, ptr, chg, percent, color, colormasks);
    else
        X11_status_update_tty(fld, ptr, chg, percent, color, colormasks);
}

/* create an overall status widget (X11_status_widget) and also
   separate widgets for all status fields and potential conditions */
static Widget
create_tty_status(Widget parent, Widget top)
{
    Widget form; /* viewport that holds the form that surrounds everything */
    Arg args[6];
    Cardinal num_args;

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
    X11_status_widget = XtCreateManagedWidget("status_form", windowWidgetClass,
                                              form, args, num_args);

    int height = nhFontHeight(X11_status_widget, NHW_STATUS) * 3;
    num_args = 0;
    XtSetArg(args[num_args], XtNheight, height); num_args++;
    XtSetValues(form, args, num_args);
    XtSetValues(X11_status_widget, args, num_args);

    XtAddCallback(X11_status_widget, XtNexposeCallback, tty_status_exposed,
                  (XtPointer) 0);

    return X11_status_widget;
}

/*
 * TTY status window expose callback.
 */
/*ARGSUSED*/
static void
tty_status_exposed(Widget w, XtPointer client_data, /* unused */
                   XtPointer widget_data) /* expose event from Window widget */
{
    tty_status_redraw(w);
    nhUse(client_data);
    nhUse(widget_data);
}

#ifdef STATUS_HILITES
void
X11_tty_status_blink(void)
{
    /* Do we have any active blink attributes? */
    boolean have_blink = FALSE;
    for (unsigned i = 0; i < SIZE(X11_status_labels) && !have_blink; ++i) {
        have_blink = (X11_status_labels[i].attrs & HL_BLINK) != 0;
    }
    for (unsigned i = 0; i < SIZE(X11_cond_labels) && !have_blink; ++i) {
        have_blink = (X11_cond_labels[i].attrs & HL_BLINK) != 0;
    }

    if (have_blink) {
        tty_status_redraw(X11_status_widget);
    }
}
#endif /* STATUS_HILITES */

static void
tty_status_redraw(Widget w)
{
    Arg args[5];
    int num_args;
    Dimension width;

    /* Name is placed here */
    int name_x = 0;
    int name_y = 0;
    int name_width = 0;

    /* Get the height of the font and the width of the widget */
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, &width); num_args++;
    XtGetValues(w, args, num_args);
    int height = nhFontHeight(w, NHW_STATUS);

    /* Space between fields. For now, this is equal to one half the height. */
    int spacing = height/2;

    int x = 0;
    int y = 0;
    XClearWindow(XtDisplay(w), XtWindow(w));
    if (iflags.wc2_statuslines <= 2) {
        for (unsigned row = 0; row < SIZE(X11_fieldorder_2); ++row) {
            for (unsigned i = 0; i < SIZE(X11_fieldorder_2[0]); ++i) {
                enum statusfields fld = X11_fieldorder_2[row][i];
                if (fld == BL_FLUSH) {
                    break;
                }
                int wid = tty_render_field(w, x, y, spacing, fld);
                if (fld == BL_TITLE) {
                    name_x = x;
                    name_y = y;
                    name_width = wid;
                }
                x += wid;
            }
            x = 0;
            y += height;
        }
    } else {
        for (unsigned row = 0; row < SIZE(X11_fieldorder_3); ++row) {
            for (unsigned i = 0; i < SIZE(X11_fieldorder_3[0]); ++i) {
                enum statusfields fld = X11_fieldorder_3[row][i];
                if (fld == BL_FLUSH) {
                    break;
                }
                int wid = tty_render_field(w, x, y, spacing, fld);
                if (fld == BL_TITLE) {
                    name_x = x;
                    name_y = y;
                    name_width = wid;
                }
                x += wid;
            }
            x = 0;
            y += height;
        }
    }

    /*
     * Draw the hit point bar using:
     * * the colors for hit points
     * * the opposite of the HL_INVERSE bit for the title
     * * the font for the title
     */
    if (name_width != 0 && iflags.wc2_hitpointbar) {
        XRectangle clip = {
            .x = name_x,
            .y = name_y,
            .width = name_width * X11_status_labels[BL_HP].percent / 100,
            .height = height
        };
        struct tty_status_field const *title = &X11_status_labels[BL_TITLE];
        struct tty_status_field const *hitp = &X11_status_labels[BL_HP];
        int attrs = ((title->attrs & ~HL_DIM) ^ HL_INVERSE)
                  | (hitp->attrs & HL_DIM);
        tty_render_text(w, &clip, name_x, name_y,
                        title->text, hitp->color, attrs,
                        BL_TITLE);
    }
}

/* Render one field of the TTY status */
static int
tty_render_field(Widget w, int x, int y, int spacing, enum statusfields fld)
{
    static struct stat_fmt {
        const char *pre;
        const char *post;
    } formats[MAXBLSTATS] = {
        [BL_TITLE] = { NULL, NULL },
        [BL_STR] = { "St:", NULL },
        [BL_DX] = { "Dx:", NULL },
        [BL_CO] = { "Co:", NULL },
        [BL_IN] = { "In:", NULL },
        [BL_WI] = { "Wi:", NULL },
        [BL_CH] = { "Ch:", NULL },
        [BL_ALIGN] = { NULL, NULL },
        [BL_SCORE] = { "S:", NULL },
        [BL_CAP] = { NULL, NULL },
        [BL_GOLD] = { NULL, NULL },
        [BL_ENE] = { "Pw:", NULL },
        [BL_ENEMAX] = { "(", ")" },
        [BL_XP] = { "Xp:", NULL },
        [BL_AC] = { "AC:", NULL },
        [BL_HD] = { "HD:", NULL },
        [BL_TIME] = { "T:", NULL },
        [BL_HUNGER] = { NULL, NULL },
        [BL_HP] = { "HP:", NULL },
        [BL_HPMAX] = { "(", ")" },
        [BL_LEVELDESC] = { NULL, NULL },
        [BL_EXP] = { "/", NULL },
        [BL_CONDITION] = { NULL, NULL },
        [BL_WEAPON] = { NULL, NULL },
        [BL_ARMOR] = { NULL, NULL },
        [BL_TERRAIN] = { NULL, NULL },
        [BL_VERS] = { NULL, NULL },
    };

    int width = 0;

    switch (fld) {
    case BL_FLUSH:
        break;

    case BL_CONDITION:
        for (unsigned j = 0; j < SIZE(X11_cond_labels); ++j) {
            struct tty_cond_field const *fldp = &X11_cond_labels[j];
            if (fldp->text != NULL) {
                width += spacing;
                width += tty_render_text(w, NULL, x + width, y, fldp->text, fldp->color, fldp->attrs, BL_FLUSH);
            }
        }
        break;

    default:
        {
            struct tty_status_field const *fldp = &X11_status_labels[fld];
            if (fldp->text != NULL && fldp->text[0] != '\0') {
                if (x != 0 && (formats[fld].pre == NULL || strchr("(/", formats[fld].pre[0]) == NULL)) {
                    width += spacing;
                }
                if (formats[fld].pre != NULL) {
                    width += tty_render_text(w, NULL, x + width, y, formats[fld].pre, NO_COLOR, 0, BL_FLUSH);
                }
                width += tty_render_text(w, NULL, x + width, y, fldp->text, fldp->color,
                                     fldp->attrs, fld);
                if (formats[fld].post != NULL) {
                    width += tty_render_text(w, NULL, x + width, y, formats[fld].post, NO_COLOR, 0, BL_FLUSH);
                }
            }
        }
        break;
    }

    return width;
}

/* Render text as part of the TTY status */
static int
tty_render_text(Widget w, const XRectangle *clip, int x, int y,
                const char *text, int color, int attr, enum statusfields fld)
{
#ifndef STATUS_HILITES
    nhUse(color);
    nhUse(attr);
#endif

    int goldwidth = 0;

    /* Get the colors */
    Pixel fgpixel, bgpixel;
    tty_status_colors(w, color, attr, &fgpixel, &bgpixel);

#ifdef USE_XFT

    /* Get the font */
    XftFont *font = X11_new_font(w, attr, NHW_STATUS);
    int height = X11_font_height(font);

    /* Get the drawing resources */
    Display *display = XtDisplay(w);
    Screen *screen = DefaultScreenOfDisplay(display);
    Visual *visual = DefaultVisualOfScreen(screen);
    Colormap cmap = DefaultColormapOfScreen(screen);
    XftDraw *draw = XftDrawCreate(display, XtWindow(w), visual, cmap);

    /* Convert the colors to Xft form */
    XftColor fgcolor, bgcolor;
    X11_new_color(w, fgpixel, &fgcolor);
    X11_new_color(w, bgpixel, &bgcolor);

    /* If drawing a hitpoint bar, we'll need a clipping rectangle */
    if (clip != NULL) {
        XftDrawSetClipRectangles(draw, 0, 0, clip, 1);
    }

    /* Gold will begin with a glyph string. Convert this to the proper
       character, which might possibly be Unicode */
    if (fld == BL_GOLD && memcmp(text, "\\G", 2) == 0) {
        char *end;
        unsigned long glyphcode = strtoul(text+2, &end, 16);
        if ((glyphcode >> 16) == (unsigned) svc.context.rndencode && *end == ':') {
            /* We have a proper glyph code */
            glyph_info glyphinfo;
            glyphcode &= 0xFFFF;
            map_glyphinfo(0, 0, glyphcode, 0, &glyphinfo);
            X11_map_symbol goldsym = X11_glyph_char(&glyphinfo);

            /* Get the width of the gold symbol */
            XGlyphInfo extents;
#ifdef ENHANCED_SYMBOLS
            FcChar32 goldch = goldsym;
            XftTextExtents32(display, font, &goldch, 1, &extents);
#else /* !ENHANCED_SYMBOLS */
            FcChar8 goldch = goldsym;
            XftTextExtents8(display, font, &goldch, 1, &extents);
#endif /* ?ENHANCED_SYMBOLS */
            goldwidth = extents.width - extents.x;

            /* Render the gold symbol */
            XftDrawRect(draw, &bgcolor, x, y, goldwidth, height);
#ifdef ENHANCED_SYMBOLS
            XftDrawString32(draw, &fgcolor, font, x, y + font->ascent, &goldch, 1);
#else /* !ENHANCED_SYMBOLS */
            XftDrawString8(draw, &fgcolor, font, x, y + font->ascent, &goldch, 1);
#endif /* ?ENHANCED_SYMBOLS */

            text = end;
        }
    }

    /* Get the width of the rendered string, not including goldwidth */
    XGlyphInfo extents;
    XftTextExtents8(display, font, (const FcChar8 *) text, strlen(text), &extents);
    int width = extents.width - extents.x;

    /* Place version on the right */
    if (fld == BL_VERS) {
        Cardinal num_args = 0;
        Arg args[1];
        Dimension wwidth;
        XtSetArg(args[num_args], XtNwidth, &wwidth); num_args++;
        XtGetValues(w, args, num_args);
        int x2 = wwidth - width;
        if (x2 > x) {
            x = x2;
        }
    }

    /* Render the string */
    XftDrawRect(draw, &bgcolor, x + goldwidth, y, width, height);
    XftDrawString8(draw, &fgcolor, font,
                   x + goldwidth, y + font->ascent,
                     (const FcChar8 *) text, strlen(text));
    width += goldwidth;

#ifdef STATUS_HILITES
    /* Implement underline */
    if (attr & HL_ULINE) {
        XftDrawRect(draw, &fgcolor, x, y + font->ascent, width, 1);
    }
#endif /* STATUS_HILITES */

    /* Release resources */

    XftColorFree(display, visual, cmap, &fgcolor);
    XftColorFree(display, visual, cmap, &bgcolor);
    XftDrawDestroy(draw);
    X11_release_font(w, font);

#else /* !USE_XFT */

    Arg args[5];
    Cardinal num_args;
    XGCValues values;
    XFontStruct *font;
    XFontStruct *font_italic = NULL; /* custodial */

    values.foreground = fgpixel;
    values.background = bgpixel;

    /* Get the font */
    num_args = 0;
    XtSetArg(args[num_args], XtNfont, &font); num_args++;
    XtGetValues(w, args, num_args);

    /* Implement bold font */
    if (attr & HL_BOLD) {
        struct xwindow *wp = find_widget(w);
        load_boldfont(wp, w);
        font = wp->boldfs;
    }

    /* Implement italic font */
    if (attr & HL_ITALIC) {
        /* font may also be bold */
        font_italic = X11_italic_font(XtDisplay(w), font);
        if (font_italic != NULL) {
            font = font_italic;
        }
    }

    /* Get a graphics context */
    values.font = font->fid;
    values.function = GXcopy;
    GC ggc = XtGetGC(w,
                     GCFunction | GCForeground | GCBackground | GCFont,
                     &values);

    /* If drawing a hitpoint bar, we'll need a clipping rectangle */
    if (clip != NULL) {
        /* @#$% non-const-correct Xlib functions */
        XRectangle clip2 = *clip;
        XSetClipRectangles(XtDisplay(w), ggc, 0, 0, &clip2, 1, Unsorted);
    }

    /* Gold will begin with a glyph string. Convert this to the proper
       character, which might possibly be Unicode */
    if (fld == BL_GOLD && memcmp(text, "\\G", 2) == 0) {
        char *end;
        unsigned long glyphcode = strtoul(text+2, &end, 16);
        if ((glyphcode >> 16) == (unsigned) svc.context.rndencode && *end == ':') {
            /* We have a proper glyph code */
            glyph_info glyphinfo;
            glyphcode &= 0xFFFF;
            map_glyphinfo(0, 0, glyphcode, 0, &glyphinfo);
            X11_map_symbol goldsym = X11_glyph_char(&glyphinfo);
#ifdef ENHANCED_SYMBOLS
            if (goldsym > 0xFFFF) {
                /* No support for supplementary planes */
                goldsym = GOLD_SYM;
            }
            if (goldsym > 0x7F) {
                XFontStruct *unifont = X11_unicode_font(XtDisplay(w), font);

                if (unifont != NULL) {
                    XChar2b goldstr[1];
                    values.font = unifont->fid;
                    GC ggc2 = XtGetGC(w,
                                      GCFunction | GCForeground | GCBackground | GCFont,
                                      &values);

                    goldstr[0].byte1 = goldsym >> 8;
                    goldstr[0].byte2 = goldsym & 0xFF;
                    goldwidth = XTextWidth16(font, goldstr, 1);

                    /* Render the string */
                    XDrawImageString16(XtDisplay(w), XtWindow(w), ggc,
                                       x, y + font->max_bounds.ascent,
                                       goldstr, 1);

                    XFreeFont(XtDisplay(w), unifont);
                    XtReleaseGC(w, ggc2);
                }
            }
#endif
            if (goldwidth == 0) {
                char goldstr[1];
                goldstr[0] = (char) goldsym;
                goldwidth = XTextWidth(font, goldstr, 1);

                /* Render the string */
                XDrawImageString(XtDisplay(w), XtWindow(w), ggc,
                                 x, y + font->max_bounds.ascent,
                                 goldstr, 1);
            }

            text = end;
        }
    }

    /* Get the width of the rendered string */
    int width = XTextWidth(font, text, strlen(text)) + goldwidth;

    /* Place version on the right */
    if (fld == BL_VERS) {
        num_args = 0;
        Dimension wwidth;
        XtSetArg(args[num_args], XtNwidth, &wwidth); num_args++;
        XtGetValues(w, args, num_args);
        int x2 = wwidth - width;
        if (x2 > x) {
            x = x2;
        }
    }

    /* Render the string */
    XDrawImageString(XtDisplay(w), XtWindow(w), ggc,
                     x + goldwidth, y + font->max_bounds.ascent,
                     text, strlen(text));

#ifdef STATUS_HILITES
    /* Implement underline */
    if (attr & HL_ULINE) {
        XDrawLine(XtDisplay(w), XtWindow(w), ggc,
                  x, y + font->max_bounds.ascent,
                  x + width - 1, y + font->max_bounds.ascent);
    }
#endif /* STATUS_HILITES */

    /* Release resources */
    XtReleaseGC(w, ggc);
    if (font_italic != NULL) {
        XFreeFont(XtDisplay(w), font_italic);
    }
#endif /* ?USE_XFT */

    /* Caller will advance x by the width */
    return width;
}

static void
tty_status_colors(Widget w, int color, int attr, Pixel *fgpixel, Pixel *bgpixel)
{
    Cardinal num_args;
    Arg args[5];

    /* Get the default colors */
    num_args = 0;
    XtSetArg(args[num_args], XtNforeground, fgpixel); num_args++;
    XtSetArg(args[num_args], XtNbackground, bgpixel); num_args++;
    XtGetValues(w, args, num_args);

    /* Implement color if requested */
#ifdef STATUS_HILITES
    if (color != NO_COLOR) {
        XrmValue source;
        XrmValue dest;
        Pixel pixel;
        const char *cname = fancy_status_hilite_colors[color];
        source.addr = (XPointer) cname;
        source.size = (unsigned int) strlen(cname) + 1;
        dest.size = (unsigned int) sizeof (Pixel);
        dest.addr = (XPointer) &pixel;
        if (XtConvertAndStore(w, XtRString, &source, XtRPixel, &dest))
            *fgpixel = pixel;
    }

    /* Implement dim text */
    if (attr & HL_DIM) {
        *fgpixel = (*fgpixel & 0xFEFEFE) >> 1;
        *bgpixel = (*bgpixel & 0xFEFEFE) >> 1;
    }

    /* Implement blink */
    if ((attr & HL_BLINK) && X11_blink) {
        *fgpixel = *bgpixel;
    }

    /* Get a graphics context */
    if (attr & HL_INVERSE) {
        Pixel swap;
        swap = *fgpixel;
        *fgpixel = *bgpixel;
        *bgpixel = swap;
    }
#else
    nhUse(color);
    nhUse(attr);
#endif
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
    /* if status_information is defined, then it is a "text" status window */
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

#ifndef STATUS_HILITES
/*ARGSUSED*/
void
adjust_status_tty(struct xwindow *wp UNUSED, const char *str UNUSED)
{
    /* nothing */
    return;
}
#endif

void
create_status_window(
    struct xwindow *wp, /* window pointer */
    boolean create_popup,
    Widget parent)
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

#if 0   /*
         * this does not work as intended; it triggers
         * "Warning: Cannot find callback list in XtAddCallback"
         */
    XtAddCallback(wp->w, XtNresizeCallback, stat_resized, (XtPointer) 0);
#else
    nhUse(stat_resized);
#endif
}

/* callback to deal with the game window being resized */
static void
stat_resized(Widget w, XtPointer call_data, XtPointer client_data)
{
    Arg args[4];
    Cardinal num_args;
    struct xwindow *wp = xw_status_win;

    nhUse(call_data);
    nhUse(client_data);

    if (w == wp->w) {
        num_args = 0;
        XtSetArg(args[num_args], XtNwidth, &wp->pixel_width); num_args++;
        XtSetArg(args[num_args], XtNwidth, &wp->pixel_height); num_args++;
        XtGetValues(w, args, num_args);
    } else {
        impossible("Status Window resized, but of what widget?");
    }

    /* tell core to call us back for a full status update */
    disp.botlx = TRUE;
}

void
destroy_status_window(struct xwindow *wp)
{
    if (appResources.fancy_status)
        destroy_status_window_fancy(wp);
    else
        destroy_status_window_tty(wp);
}

#ifndef STATUS_HILITES
void
adjust_status(struct xwindow *wp, const char *str)
{
    if (appResources.fancy_status)
        adjust_status_fancy(wp, str);
    else
        adjust_status_tty(wp, str);
}
#endif

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

    wp->pixel_height = 2 * nhFontHeight(wp->w, NHW_STATUS) + top_margin + bottom_margin;
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

#ifndef STATUS_HILITES
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
#endif

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
#define SV_LABEL 1 /* displays a changeable label */
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
static void update_val(struct X_status_value *, long, const char *);
static void skip_cond_val(struct X_status_value *);
static void update_color(struct X_status_value *, int);
static Pixel color_to_pixel(Widget, int);
static void apply_hilite_attributes(struct X_status_value *, int);
static const char *width_string(int);
static void create_widget(Widget, struct X_status_value *, int);
static void get_widths(struct X_status_value *, int *, int *);
static void set_widths(struct X_status_value *, int, int);
static Widget init_column(const char *, Widget, Widget, Widget, int *, int);
static void fixup_cond_widths(void);
static Widget init_info_form(Widget, Widget, Widget);

/* narrower values for the array initializer */
#define W0 (Widget) 0
#define P0 (Pixel) 0
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
    { "",             SV_NAME,  W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    /* 1 */
    { "Strength",     SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Dexterity",    SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Constitution", SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Intelligence", SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    /* 5 */
    { "Wisdom",       SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Charisma",     SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    /* F_NAME: 7 */
    { "",             SV_LABEL, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    /* F_DLEVEL: 8 */
    { "",             SV_LABEL, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Gold",         SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    /* F_HP: 10 */
    { "Hit Points",   SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Max HP",       SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Power",        SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Max Power",    SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Armor Class",  SV_VALUE, W0, 256L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    /* F_XP_LEVL: 15 */
    { "Xp Level",     SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    /* also 15 (overloaded field) */
    /*{ "Hit Dice",   SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },*/
    /* F_EXP_PTS: 16 (optionally displayed) */
    { "Exp Points",   SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Alignment",    SV_VALUE, W0,  -2L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    /* 18, optionally displayed */
    { "Time",         SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    /* 19, conditionally present, optionally displayed when present */
    { "Score",        SV_VALUE, W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    /* F_HUNGER: 20 (blank if 'normal') */
    { "",             SV_NAME,  W0,  -1L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    /* F_ENCUMBER: 21 (blank if unencumbered) */
    { "",             SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Trapped",      SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Tethered",     SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Levitating",   SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    /* 25 */
    { "Flying",       SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Riding",       SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Grabbed!",     SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    /* F_STONE: 28 */
    { "Petrifying",   SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Slimed",       SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    /* 30 */
    { "Strangled",    SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Food Pois",    SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Term Ill",     SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    /* F_IN_LAVA: 33 */
    { "Sinking",      SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Held",         SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    /* 35 */
    { "Holding",      SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Blind",        SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Deaf",         SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Stunned",      SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Confused",     SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    /* F_HALLU: 40 (full spelling truncated due to space limitations) */
    { "Hallucinat",   SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    /* F_VERS; optionally shown, generally treated as a pseudo-condition */
    { "Version 1.2.3", SV_LABEL, W0,  0L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Weapon",       SV_NAME,  W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Armor",        SV_NAME,  W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "Terrain",      SV_NAME,  W0,  -1L, 0, FALSE, FALSE, FALSE, P0, 0, 0 },
    { "BareHands",    SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "GlowHands",    SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Icy",          SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Busy",         SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Paralyzed",    SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Sleeping",     SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Unconsc",      SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "ElfIron",      SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Slippery",     SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "Submerged",    SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
    { "HurtLegs",     SV_NAME,  W0,   0L, 0, FALSE, TRUE,  FALSE, P0, 0, 0 },
};
#undef W0
#undef P0
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

/* some conditions are mutually exclusive so we overload their fields in
   order to share same display slot */
static const struct f_overload cond_ovl[] = {
    { (BL_MASK_TRAPPED | BL_MASK_TETHERED),
      { { BL_MASK_TRAPPED, F_TRAPPED },
        { BL_MASK_TETHERED, F_TETHERED } },
    },
    {
      /* BL_GRABBED is mutually exclusive with these but is more severe so
         is shown separately rather than being overloaded with them */
      (BL_MASK_HELD | BL_MASK_HOLDING),
      { { BL_MASK_HELD, F_HELD },
        { BL_MASK_HOLDING, F_HOLDING } },
    },
    { (BL_MASK_BUSY | BL_MASK_PARLYZ | BL_MASK_SLEEPING | BL_MASK_UNCONSC),
      { { BL_MASK_BUSY, F_BUSY }, /* can't move but none of the below... */
        { BL_MASK_PARLYZ, F_PARALYZED },
        { BL_MASK_SLEEPING, F_SLEEPING },
        { BL_MASK_UNCONSC, F_UNCONSCIOUS } },
    },
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
            X11_update_label(sv->w);
            break;

        default:
            impossible("null_out_status: unknown type %d\n", sv->type);
            break;
        }
    }
}

DISABLE_WARNING_FORMAT_NONLITERAL

static void
update_val(struct X_status_value *attr_rec, long new_value, const char *new_valuestr)
{
    static boolean Exp_shown = TRUE, time_shown = TRUE, score_shown = TRUE,
                   Xp_was_HD = FALSE;
    char buf[BUFSZ];
    Arg args[4];

    if (attr_rec->type == SV_LABEL) {
        if (attr_rec == &shown_stats[F_NAME]) {
            Strcpy(buf, svp.plname);
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
            } else {
                Strcat(buf,
                       rank_of(u.ulevel, svp.pl_character[0], flags.female));
            }

        } else if (attr_rec == &shown_stats[F_DLEVEL]) {
            if (!describe_level(buf, 0)) {
                Strcpy(buf, svd.dungeons[u.uz.dnum].dname);
                Sprintf(eos(buf), ", level %d", depth(&u.uz));
            }
        } else if (attr_rec == &shown_stats[F_VERS]) {
            if (flags.showvers)
                (void) status_version(buf, sizeof buf, FALSE);
            else
                buf[0] = '\0';
        } else {
            impossible("update_val: unknown label type \"%s\"",
                       attr_rec->name);
            return;
        }

        if (!strcmp(buf, attr_rec->name))
            return; /* same */

        /* Set the label.  'name' field is const for most entries;
           we need to cast away that const for this assignment */
        Strcpy((char *) attr_rec->name, buf);
        XtSetArg(args[0], XtNlabel, buf);
        XtSetValues(attr_rec->w, args, ONE);
        X11_update_label(attr_rec->w);

    } else if (attr_rec->type == SV_NAME) {
        boolean direct = attr_rec == &shown_stats[F_ARMOR]
                      || attr_rec == &shown_stats[F_WEAPON]
                      || attr_rec == &shown_stats[F_TERRAIN];
        if (!direct && attr_rec->last_value == new_value)
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
        /* Special cases: weapon, armor and terrain */
        } else if (direct) {
            if (new_valuestr == NULL) {
                return;
            }
            Strcpy(buf, new_valuestr);
        } else {
            *buf = '\0'; /* condition name Off */
        }

        XtSetArg(args[0], XtNlabel, buf);
        XtSetValues(attr_rec->w, args, ONE);
        X11_update_label(attr_rec->w);

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
                update_fancy_status_field(F_EXP_PTS, NULL, NO_COLOR, HL_UNDEF);
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
     * it's continually changing.  Don't highlight version because once set
     * it only changes if player modifies 'versinfo' option.  For others,
     * don't highlight if this is the first update.
     * If already highlighted, don't change it unless
     * it's being set to blank (where that item should be reset now instead
     * of showing highlighted blank until the next expiration check).
     *
     * 5.0:  highlight non-labelled 'name' items (conditions plus hunger
     * and encumbrance) when they come On.  For all conditions going Off,
     * or changing to not-hungry or not-encumbered, there's nothing to
     * highlight because the field becomes blank.
     */
    if (attr_rec->after_init) {
        /* toggle if not highlighted and being set to nonblank or if
           already highlighted and being set to blank */
        if (attr_rec != &shown_stats[F_TIME]
            && attr_rec != &shown_stats[F_VERS]
            && attr_rec != &shown_stats[F_TERRAIN]
            && !attr_rec->set ^ !*buf) {
            if (attr_rec->type == SV_VALUE)
                X11_set_highlight(get_value_widget(attr_rec->w), TRUE);
            else
                X11_set_highlight(attr_rec->w, TRUE);
            attr_rec->set = !attr_rec->set;
        }
        attr_rec->turn_count = 0;
    } else {
        Widget w = (attr_rec->type == SV_LABEL || attr_rec->type == SV_NAME) ? attr_rec->w
                   : get_value_widget(attr_rec->w);
        XtSetArg(args[0], XtNforeground, &attr_rec->default_fg);
        XtGetValues(w, args, ONE);
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
           'set' but the same widget so the highlighting got toggled
           off; this will turn in back on in that exceptional case */
        X11_set_highlight(sv->w, FALSE);
        sv->set = FALSE;
    }
}

static void
update_color(struct X_status_value *sv, int color)
{
    Pixel pixel = 0;
    Arg args[1];
    Widget w = (sv->type == SV_LABEL || sv->type == SV_NAME) ? sv->w
               : get_value_widget(sv->w);

    if (color == NO_COLOR) {
        if (sv->after_init)
            pixel = sv->default_fg;
        sv->colr = NO_COLOR;
    } else {
        pixel = color_to_pixel(w, color);
        sv->colr = color;
    }
    if (pixel != 0) {
        XtSetArg(args[0], XtNforeground, pixel);
        XtSetValues(w, args, ONE);
        X11_update_label(w);
    }
}

static Pixel
color_to_pixel(Widget w, int color)
{
    Pixel pixel;

    if (fancy_status_hilite_colors[color][0] != '\0') {
        XrmValue source;
        XrmValue dest;
        source.addr = (XPointer) fancy_status_hilite_colors[color];
        source.size = (unsigned int) strlen((const char *) source.addr) + 1;
        dest.size = (unsigned int) sizeof (Pixel);
        dest.addr = (XPointer) &pixel;
        if (XtConvertAndStore(w, XtRString, &source, XtRPixel, &dest)) {
            return pixel;
        }
    }

    pixel = 0xFFFFFF;
    Arg args[1];
    XtSetArg(args[0], XtNforeground, &pixel);
    XtGetValues(toplevel, args, ONE);
    return pixel;
}

static void
apply_hilite_attributes(struct X_status_value *sv, int attributes)
{
    Widget w = sv->w;
    if (sv->type == SV_VALUE) {
        w = get_value_widget(w);
    }
    X11_set_attrs(w, attributes);
}

static void
set_percent(int index, int percent, int color)
{
    Widget w = shown_stats[index].w;
    X11_set_percent(w, percent, color_to_pixel(w, color));
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
 *                blind, deaf, stun, conf, hallu, version ***)
 *
 *   [*] order of status fields is different on tty.
 *  [**] HD is shown instead of level and exp if Upolyd.
 * [***] version is optional, right-justified after conditions
 */
static void
update_fancy_status_field(int i, const char *valstr, int color, int attributes)
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
         * these are either on or off.  Please leave the ternary operators
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
        case F_BAREH:
            condmask = BL_MASK_BAREH;
            break;
        case F_GLOWHANDS:
            condmask = BL_MASK_GLOWHANDS;
            break;
        case F_ICY:
            condmask = BL_MASK_ICY;
            break;
        case F_BUSY:
            condmask = BL_MASK_BUSY;
            break;
        case F_PARALYZED:
            condmask = BL_MASK_PARLYZ;
            break;
        case F_SLEEPING:
            condmask = BL_MASK_SLEEPING;
            break;
        case F_UNCONSCIOUS:
            condmask = BL_MASK_UNCONSC;
            break;
        case F_IRON:
            condmask = BL_MASK_ELF_IRON;
            break;
        case F_SLIPPERY:
            condmask = BL_MASK_SLIPPERY;
            break;
        case F_SUBMERGED:
            condmask = BL_MASK_SUBMERGED;
            break;
        case F_WOUNDEDL:
            condmask = BL_MASK_WOUNDEDL;
            break;

        /* pseudo-condition */
        case F_VERS:
            val = (long) flags.versinfo; /* 1..7 */
            break;

        case F_NAME:
        case F_DLEVEL:
            val = (long) 0L;
            break; /* special */

        case F_GOLD:
            val = money_cnt(gi.invent);
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
            val = flags.time ? (long) svm.moves : 0L;
            break;
        case F_SCORE:
#ifdef SCORE_ON_BOTL
            val = flags.showscore ? botl_score() : 0L;
#else
            val = 0L;
#endif
            break;
        case F_WEAPON:
        case F_ARMOR:
        case F_TERRAIN:
            /* valstr is the value */
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
    update_val(sv, val, valstr);
    if (color != sv->colr)
        update_color(sv, color);
    if (attributes != sv->attr)
        apply_hilite_attributes(sv, attributes);
}

/* fully update status after bl_flush or window resize */
static void
update_fancy_status(boolean force_update)
{
    static boolean old_showtime, old_showexp, old_showscore, old_showvers;
    static int old_upolyd = -1; /* -1: force first time update */
    int i;

    if (force_update
        || Upolyd != old_upolyd /* Xp vs HD */
        || flags.time != old_showtime
        || flags.showexp != old_showexp
        || flags.showscore != old_showscore
        || flags.showvers != old_showvers) {
        /* update everything; usually only need this on the very first
           time, then later if the window gets resized or if poly/unpoly
           triggers Xp <-> HD switch or if an optional field gets
           toggled off since there won't be a status_update() call for
           the no longer displayed field; we're a bit more conservative
           than that and do this when toggling on as well as off */
        for (i = 0; i < NUM_STATS; i++)
            update_fancy_status_field(i, NULL, NO_COLOR, HL_UNDEF);
        old_condition_bits = X11_condition_bits;

        old_upolyd = Upolyd;
        old_showtime = flags.time;
        old_showexp = flags.showexp;
        old_showscore = flags.showscore;
        old_showvers = flags.showvers;
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
            if (sv->type == SV_VALUE)
                X11_set_highlight(get_value_widget(sv->w), FALSE);
            else
                X11_set_highlight(sv->w, FALSE);
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
    case F_BAREH:
    case F_GLOWHANDS:
    case F_BUSY:
    case F_PARALYZED:
    case F_SLEEPING:
    case F_UNCONSCIOUS:
    case F_IRON:
    case F_SLIPPERY:
    case F_SUBMERGED:
    case F_WOUNDEDL:
    case F_ICY:
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
    case F_WEAPON:
        return "Dual+joust";
    case F_ARMOR:
        return "GCAUHBS";
    case F_TERRAIN:
        return "Portcullis";
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
        X11_wrap_widget(sv->w, NHW_STATUS);
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
        X11_wrap_widget(sv->w, NHW_STATUS);
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
init_column(
    const char *name,
    Widget parent, Widget top, Widget left,
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
 * 5.0:  changed so that all 6 columns have 8 rows, but a few entries
 * are left blank <>.  Exp-points, Score, and Time are optional depending
 * on run-time settings; Xp-level is replaced by Hit-Dice (and Exp-points
 * suppressed) when the hero is polymorphed.  Title and Dungeon-Level span
 * two columns and might expand to more if 'hitpointbar' is implemented.
 * Version is optional, right justified, and much wider than the others.
 *
 Title ("Plname the Rank")   <>            <>           <>          <>
 Dungeon-Branch-and-Level    <>            <>           <>          <>
 Hit-points    Max-HP       Strength      Hunger       Grabbed     Held
 Power-points  Max-Power    Dexterity     Encumbrance  Petrifying  Blind
 Armor-class   Alignment    Constitution  Trapped      Slimed      Deaf
 Xp-level     [Exp-points]  Intelligence  Levitation   Strangled   Stunned
 Gold         [Score]       Wisdom        Flying       Food-Pois   Confused
  <>          [Time]        Charisma      Riding       Term-Ill    Hallucinat
 BareHands     <>            <>           HurtLegs     Sinking     GlowHands
 Weapon       Armor         Terrain       Elf-Iron     Busy        Icy
  <>           <>            <>           Slippery     Submerged       Version
 *
 * A seventh column is going to be needed to fit in more conditions.
 */

/* including F_DUMMY makes the status condition columns evenly
   spaced with regard to the adjacent characteristics (Str,Dex,&c) column;
   we lose track of the Widget pointer for F_DUMMY, each use clobbering the
   one before, leaving the one from leftover_indices[]; since they're never
   updated, that shouldn't matter */
static int status_indices[][13] = {
    { F_DUMMY, F_HUNGER, F_ENCUMBER, F_TRAPPED,
      F_LEV, F_FLY, F_RIDE, F_WOUNDEDL, F_IRON, F_SLIPPERY,
      -1, 0, 0 },
    { F_DUMMY, F_GRABBED, F_STONE, F_SLIME, F_STRNGL,
      F_FOODPOIS, F_TERMILL, F_IN_LAVA, F_BUSY, F_SUBMERGED,
      -1, 0, 0 },
    { F_DUMMY, F_HELD, F_BLIND, F_DEAF, F_STUN,
      F_CONF, F_HALLU, F_GLOWHANDS, F_ICY, F_VERS,
      -1, 0, 0 },
};
/* used to fill up the empty space to right of last status condition column */
static int leftover_indices[] = { F_DUMMY, -1, 0, 0 };
/* -2: top two rows of these columns are reserved for title and location */
static int col1_indices[13 - 2] = {
    F_HP,    F_POWER,    F_AC,    F_XP_LEVL, F_GOLD,  F_DUMMY,
    F_BAREH, F_WEAPON, F_DUMMY,
    -1
};
static int col2_indices[13 - 2] = {
    F_MAXHP, F_MAXPOWER, F_ALIGN, F_EXP_PTS, F_SCORE, F_TIME,
    F_DUMMY, F_ARMOR, F_DUMMY,
    -1
};
static int characteristics_indices[13 - 2] = {
    F_STR, F_DEX, F_CON, F_INT, F_WIS, F_CHA,
    F_DUMMY, F_TERRAIN, F_DUMMY,
    -1
};

/*
 * Produce a form that looks like the following:
 *
 *                title
 *               location
 * col1_indices[0]      col2_indices[0]      col3_indices[0]      col4_indices[0]
 * col1_indices[1]      col2_indices[1]      col3_indices[1]      col4_indices[1]
 *    ...                  ...                  ...                  ...
 * col1_indices[5]      col2_indices[5]      col3_indices[5]      col4_indices[5]
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

    /* there are 4 columns beneath but top 2 rows are centered over first 2 */
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

/* give the status condition columns the same width */
static void
fixup_cond_widths(void)
{
    int pass, i, *ip, w1, w2;

    w1 = w2 = 0;
    for (pass = 1; pass <= 2; ++pass) { /* two passes... */
        for (i = 0; i < SIZE(status_indices); i++) {
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

    {
        Arg args[3];
        Dimension vers_width = 0;
        struct X_status_value *sv = &shown_stats[F_VERS];

        if (sv->w) {
            XtSetArg(args[0], XtNwidth, &vers_width);
            XtGetValues(sv->w, args, ONE);
            if (vers_width) {
                vers_width *= 3;
                XtSetArg(args[0], XtNwidth, vers_width);
                XtSetArg(args[1], nhStr(XtNjustify), XtJustifyRight);
                XtSetValues(sv->w, args, TWO);
            }
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
    for (i = 0; i < SIZE(status_indices); i++) {
        Sprintf(buf, "status_condition%d", i + 1);
        w = init_column(buf, form, (Widget) 0, w, status_indices[i], 0);
    }
    fixup_cond_widths(); /* make all status_conditionN columns same width
                          * (actually, the slot for F_VERS is much wider) */
    /* TODO:
     * Calculate and set the width of the F_VERS widjet to be from the
     * start of the last condition column through the right edge and
     * get rid of the dummy column.
     */

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

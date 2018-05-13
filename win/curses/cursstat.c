/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/

#include <ctype.h> /* toupper() */
#include "curses.h"
#include "hack.h"
#include "wincurs.h"
#include "cursstat.h"

/* Status window functions for curses interface */

/*
 * The following data structures come from the genl_ routines in
 * src/windows.c and as such are considered to be on the window-port
 * "side" of things, rather than the NetHack-core "side" of things.
 */

extern const char *status_fieldfmt[MAXBLSTATS];
extern const char *status_fieldnm[MAXBLSTATS];
extern char *status_vals[MAXBLSTATS];
extern boolean status_activefields[MAXBLSTATS];

/* Long format fields for vertical window */
static char *status_vals_long[MAXBLSTATS];

#ifdef STATUS_HILITES
static long curses_condition_bits;
static int curses_status_colors[MAXBLSTATS];
int hpbar_percent, hpbar_color;

static int FDECL(condcolor, (long, unsigned long *));
static int FDECL(condattr, (long, unsigned long *));
#endif /* STATUS_HILITES */
static void FDECL(draw_status, (unsigned long *));
static void FDECL(draw_classic, (boolean, unsigned long *));
static void FDECL(draw_vertical, (boolean, unsigned long *));
static void FDECL(draw_horizontal, (boolean, unsigned long *));

void
curses_status_init()
{
#ifdef STATUS_HILITES
    int i;

    for (i = 0; i < MAXBLSTATS; ++i) {
	curses_status_colors[i] = NO_COLOR; /* no color */
	status_vals_long[i] = (char *) alloc(MAXCO);
        *status_vals_long[i] = '\0';
    }
    curses_condition_bits = 0L;
    hpbar_percent = 0, hpbar_color = NO_COLOR;
#endif /* STATUS_HILITES */

    /* let genl_status_init do most of the initialization */
    genl_status_init();
}

/*
 *  *_status_update()
 *      -- update the value of a status field.
 *      -- the fldindex identifies which field is changing and
 *         is an integer index value from botl.h
 *      -- fldindex could be any one of the following from botl.h:
 *         BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH,
 *         BL_ALIGN, BL_SCORE, BL_CAP, BL_GOLD, BL_ENE, BL_ENEMAX,
 *         BL_XP, BL_AC, BL_HD, BL_TIME, BL_HUNGER, BL_HP, BL_HPMAX,
 *         BL_LEVELDESC, BL_EXP, BL_CONDITION
 *      -- fldindex could also be BL_FLUSH (-1), which is not really
 *         a field index, but is a special trigger to tell the
 *         windowport that it should redisplay all its status fields,
 *         even if no changes have been presented to it.
 *      -- ptr is usually a "char *", unless fldindex is BL_CONDITION.
 *         If fldindex is BL_CONDITION, then ptr is a long value with
 *         any or none of the following bits set (from botl.h):
 *              BL_MASK_STONE           0x00000001L
 *              BL_MASK_SLIME           0x00000002L
 *              BL_MASK_STRNGL          0x00000004L
 *              BL_MASK_FOODPOIS        0x00000008L
 *              BL_MASK_TERMILL         0x00000010L
 *              BL_MASK_BLIND           0x00000020L
 *              BL_MASK_DEAF            0x00000040L
 *              BL_MASK_STUN            0x00000080L
 *              BL_MASK_CONF            0x00000100L
 *              BL_MASK_HALLU           0x00000200L
 *              BL_MASK_LEV             0x00000400L
 *              BL_MASK_FLY             0x00000800L
 *              BL_MASK_RIDE            0x00001000L
 *      -- The value passed for BL_GOLD includes an encoded leading
 *         symbol for GOLD "\GXXXXNNNN:nnn". If the window port needs to use
 *         the textual gold amount without the leading "$:" the port will
 *         have to skip past ':' in the passed "ptr" for the BL_GOLD case.
 *      -- color is an unsigned int.
 *               color_index = color & 0x00FF;       CLR_* value
 *               attribute   = color & 0xFF00 >> 8;  BL_* values
 *         This holds the color and attribute that the field should
 *         be displayed in.
 *         This is relevant for everything except BL_CONDITION fldindex.
 *         If fldindex is BL_CONDITION, this parameter should be ignored,
 *         as condition hilighting is done via the next colormasks
 *         parameter instead.
 *
 *      -- colormasks - pointer to cond_hilites[] array of colormasks.
 *         Only relevant for BL_CONDITION fldindex. The window port
 *         should ignore this parameter for other fldindex values.
 *         Each condition bit must only ever appear in one of the
 *         CLR_ array members, but can appear in multiple HL_ATTCLR_
 *         offsets (because more than one attribute can co-exist).
 *         See doc/window.doc for more details.
 */

/* new approach through status_update() only */
#define Begin_Attr(m) \
            if (m) {                                                          \
                if ((m) & HL_BOLD)                                            \
                    wattron(win, A_BOLD);                                     \
                if ((m) & HL_INVERSE)                                         \
                    wattron(win,A_REVERSE);                                   \
                if ((m) & HL_ULINE)                                           \
                    wattron(win,A_UNDERLINE);                                 \
                if ((m) & HL_BLINK)                                           \
                    wattron(win,A_BLINK);                                     \
                if ((m) & HL_DIM)                                             \
                    wattron(win,A_DIM);                                       \
            }

#define End_Attr(m) \
            if (m) {                                                          \
                if ((m) & HL_DIM)                                             \
                    wattroff(win,A_DIM);                                      \
                if ((m) & HL_BLINK)                                           \
                    wattroff(win,A_BLINK);                                    \
                if ((m) & HL_ULINE)                                           \
                    wattroff(win,A_UNDERLINE);                                \
                if ((m) & HL_INVERSE)                                         \
                    wattroff(win,A_REVERSE);                                  \
                if ((m) & HL_BOLD)                                            \
                    wattroff(win,A_BOLD);                                     \
            }

#ifdef STATUS_HILITES

#ifdef TEXTCOLOR
#define MaybeDisplayCond(bm,txt) \
            if (curses_condition_bits & (bm)) {                               \
                putstr(STATUS_WIN, 0, " ");                                   \
                if (iflags.hilite_delta) {                                    \
                    attrmask = condattr((bm), colormasks);                    \
                    Begin_Attr(attrmask);                                 \
                    if ((coloridx = condcolor((bm), colormasks)) != NO_COLOR) \
                        curses_toggle_color_attr(win, coloridx, NONE, ON);    \
                }                                                             \
                putstr(STATUS_WIN, 0, (txt));                                 \
                if (iflags.hilite_delta) {                                    \
                    if (coloridx != NO_COLOR)                                 \
                        curses_toggle_color_attr(win, coloridx, NONE, OFF);   \
                    End_Attr(attrmask);                                       \
                }                                                             \
            }
#else
#define MaybeDisplayCond(bm,txt) \
            if (curses_condition_bits & (bm)) {                               \
                putstr(STATUS_WIN, 0, " ");                                   \
                if (iflags.hilite_delta) {                                    \
                    attrmask = condattr((bm), colormasks);                    \
                    Begin_Attr(attrmask);                                 \
                }                                                             \
                putstr(STATUS_WIN, 0, (txt));                                 \
                if (iflags.hilite_delta) {                                    \
                    End_Attr(attrmask);                                   \
                }                                                             \
            }
#endif

void
curses_status_update(fldidx, ptr, chg, percent, color, colormasks)
int fldidx, chg UNUSED, percent, color;
genericptr_t ptr;
unsigned long *colormasks;
{
    long *condptr = (long *) ptr;
    char *text = (char *) ptr;
    char *goldnum = NULL;
    static boolean oncearound = FALSE; /* prevent premature partial display */
    boolean use_name = TRUE;

    if (fldidx != BL_FLUSH) {
        if (!status_activefields[fldidx])
            return;
        if (fldidx == BL_GOLD)
           goldnum = index(text,':') + 1;
        switch (fldidx) {
        case BL_CONDITION:
            curses_condition_bits = *condptr;
            oncearound = TRUE;
            break;
        case BL_TITLE:
        case BL_HPMAX:
        case BL_ENEMAX:
        case BL_HUNGER:
        case BL_CAP:
        case BL_EXP:
            use_name = FALSE;
            /* FALLTHROUGH */
        default:
            Sprintf(status_vals[fldidx],
                    (fldidx == BL_TITLE && iflags.wc2_hitpointbar) ? "%-30s" :
                    status_fieldfmt[fldidx] ? status_fieldfmt[fldidx] : "%s",
                    text);
            if (use_name) {
                Sprintf(status_vals_long[fldidx], "%-16s: %s",
                        status_fieldnm[fldidx], goldnum ? goldnum : text);
                *(status_vals_long[fldidx]) = toupper((*status_vals_long[fldidx]));
            } else
                Strcpy(status_vals_long[fldidx], status_vals[fldidx]);
#ifdef TEXTCOLOR
            curses_status_colors[fldidx] = color;
#else
            curses_status_colors[fldidx] = NO_COLOR;
#endif
            if (iflags.wc2_hitpointbar && fldidx == BL_HP) {
			hpbar_percent = percent;
#ifdef TEXTCOLOR
                hpbar_color = color;
#else
                hpbar_color = NO_COLOR;
#endif
            }
            break;
        }
    }

    if (!oncearound) return;
    draw_status(colormasks);
}

void
draw_status(colormasks)
unsigned long *colormasks;
{
    boolean horiz = FALSE;
    WINDOW *win = curses_get_nhwin(STATUS_WIN);
    int orient = curses_get_window_orientation(STATUS_WIN);
    boolean border = curses_window_has_border(STATUS_WIN);

    if ((orient != ALIGN_RIGHT) && (orient != ALIGN_LEFT))
        horiz = TRUE;

    /* Figure out if we have proper window dimensions for horizontal statusbar. */
    if (horiz) {
        /* correct y */
        int cy = 3;
        if (iflags.classic_status)
            cy = 2;

        /* actual y (and x) */
        int ax = 0;
        int ay = 0;
        getmaxyx(win, ay, ax);
        if (border)
            ay -= 2;

        if (cy != ay) { /* something changed. Redo everything */
            curses_create_main_windows();
            curses_last_messages();
            doredraw();
            win = curses_get_nhwin(STATUS_WIN);
        }
    }

    werase(win);
    if (horiz) {
        if (iflags.classic_status)
            draw_classic(border, colormasks);
        else
            draw_horizontal(border, colormasks);
    } else
        draw_vertical(border, colormasks);

    if (border)
        box(win, 0, 0);
    wnoutrefresh(win);
}

/* The 'classic' NetHack 3.x status layout */
void
draw_classic(border, colormasks)
boolean border;
unsigned long *colormasks;
{
    enum statusfields fieldorder[2][15] = {
        { BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_ALIGN,
          BL_SCORE, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH,
          BL_FLUSH },
        { BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX,
          BL_AC, BL_XP, BL_EXP, BL_HD, BL_TIME, BL_HUNGER,
          BL_CAP, BL_CONDITION, BL_FLUSH }
    };
#ifdef TEXTCOLOR
    int coloridx = NO_COLOR;
#endif
    int i, attrmask = 0;
    char *text;
    int attridx = 0;

    WINDOW *win = curses_get_nhwin(STATUS_WIN);
    if (border) wmove(win, 1, 1);
    else wmove(win, 0, 0);
    for (i = 0; fieldorder[0][i] != BL_FLUSH; ++i) {
        int fldidx1 = fieldorder[0][i];
        if (status_activefields[fldidx1]) {
            if (fldidx1 != BL_TITLE || !iflags.wc2_hitpointbar) {
#ifdef TEXTCOLOR
                coloridx = curses_status_colors[fldidx1] & 0x00FF;
#endif
                attridx = (curses_status_colors[fldidx1] & 0xFF00) >> 8;
                text = status_vals[fldidx1];
                if (iflags.hilite_delta) {
                    if (*text == ' ') {
                        putstr(STATUS_WIN, 0, " ");
                        text++;
                    }
                    /* multiple attributes can be in effect concurrently */
                    Begin_Attr(attridx);
#ifdef TEXTCOLOR
                    if (coloridx != NO_COLOR && coloridx != CLR_MAX)
                        curses_toggle_color_attr(win, coloridx, NONE, ON);
#endif
                }

                putstr(STATUS_WIN, 0, text);

                if (iflags.hilite_delta) {
#ifdef TEXTCOLOR
                    if (coloridx != NO_COLOR)
                        curses_toggle_color_attr(win, coloridx,NONE,OFF);
#endif
                    End_Attr(attridx);
                }
            } else {
                /* hitpointbar using hp percent calculation */
                int bar_pos, bar_len;
                char *bar2 = (char *)0;
                char bar[MAXCO], savedch;
                boolean twoparts = FALSE;

                text = status_vals[fldidx1];
                bar_len = strlen(text);
                if (bar_len < MAXCO-1) {
                    Strcpy(bar, text);
                    bar_pos = (bar_len * hpbar_percent) / 100;
                    if (bar_pos < 1 && hpbar_percent > 0)
                        bar_pos = 1;
                    if (bar_pos >= bar_len && hpbar_percent < 100)
                        bar_pos = bar_len - 1;
                    if (bar_pos > 0 && bar_pos < bar_len) {
                        twoparts = TRUE;
                        bar2 = &bar[bar_pos];
                        savedch = *bar2;
                        *bar2 = '\0';
                    }
                }
                if (iflags.hilite_delta && iflags.wc2_hitpointbar) {
                    putstr(STATUS_WIN, 0, "[");
#ifdef TEXTCOLOR
                    coloridx = hpbar_color & 0x00FF;
                    /* attridx = (hpbar_color & 0xFF00) >> 8; */
                    if (coloridx != NO_COLOR)
                        curses_toggle_color_attr(win,coloridx,NONE,ON);
#endif
                    wattron(win,A_REVERSE);
                    putstr(STATUS_WIN, 0, bar);
                    wattroff(win,A_REVERSE);
#ifdef TEXTCOLOR
                    if (coloridx != NO_COLOR)
                        curses_toggle_color_attr(win,coloridx,NONE,OFF);
#endif
                    if (twoparts) {
                        *bar2 = savedch;
                        putstr(STATUS_WIN, 0, bar2);
                    }
                    putstr(STATUS_WIN, 0, "]");
                } else
                    putstr(STATUS_WIN, 0, text);
            }
        }
    }
    wclrtoeol(win);
    if (border) wmove(win, 2, 1);
    else wmove (win, 1, 0);
    for (i = 0; fieldorder[1][i] != BL_FLUSH; ++i) {
        int fldidx2 = fieldorder[1][i];

        if (status_activefields[fldidx2]) {
            if (fldidx2 != BL_CONDITION) {
#ifdef TEXTCOLOR
                coloridx = curses_status_colors[fldidx2] & 0x00FF;
#endif
                attridx = (curses_status_colors[fldidx2] & 0xFF00) >> 8;
                text = status_vals[fldidx2];
                if (iflags.hilite_delta) {
                    if (*text == ' ') {
                        putstr(STATUS_WIN, 0, " ");
                        text++;
                    }
                    /* multiple attributes can be in effect concurrently */
                    Begin_Attr(attridx);
#ifdef TEXTCOLOR
                    if (coloridx != NO_COLOR && coloridx != CLR_MAX)
                        curses_toggle_color_attr(win,coloridx,NONE,ON);
#endif
                }

                if (fldidx2 == BL_GOLD) {
                    /* putmixed() due to GOLD glyph */
                    putmixed(STATUS_WIN, 0, text);
                } else {
                    putstr(STATUS_WIN, 0, text);
                }

                if (iflags.hilite_delta) {
#ifdef TEXTCOLOR
                    if (coloridx != NO_COLOR)
                        curses_toggle_color_attr(win,coloridx,NONE,OFF);
#endif
                    End_Attr(attridx);
                }
            } else {
                MaybeDisplayCond(BL_MASK_STONE, "Stone");
                MaybeDisplayCond(BL_MASK_SLIME, "Slime");
                MaybeDisplayCond(BL_MASK_STRNGL, "Strngl");
                MaybeDisplayCond(BL_MASK_FOODPOIS, "FoodPois");
                MaybeDisplayCond(BL_MASK_TERMILL, "TermIll");
                MaybeDisplayCond(BL_MASK_BLIND, "Blind");
                MaybeDisplayCond(BL_MASK_DEAF, "Deaf");
                MaybeDisplayCond(BL_MASK_STUN, "Stun");
                MaybeDisplayCond(BL_MASK_CONF, "Conf");
                MaybeDisplayCond(BL_MASK_HALLU, "Hallu");
                MaybeDisplayCond(BL_MASK_LEV, "Lev");
                MaybeDisplayCond(BL_MASK_FLY, "Fly");
                MaybeDisplayCond(BL_MASK_RIDE, "Ride");
            }
        }
    }
    wclrtoeol(win);
    return;
}

/* The new NH4-style horizontal layout on 3 lines */
void
draw_horizontal(border, colormasks)
boolean border;
unsigned long *colormasks;
{
    /* TODO: implement this */
    /* for now, just draw classic */
    draw_classic(border, colormasks);
}

/* The vertical layout from the original curses implementation */
void
draw_vertical(border, colormasks)
boolean border;
unsigned long *colormasks;
{
    enum statusfields fieldorder[24] = {
         BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_ALIGN,
         BL_SCORE, BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX,
         BL_AC, BL_XP, BL_EXP, BL_HD, BL_TIME, BL_HUNGER,
         BL_CAP, BL_CONDITION, BL_FLUSH
    };
#ifdef TEXTCOLOR
    int coloridx = NO_COLOR;
#endif
    int i, attrmask = 0;
    char *text;
    int attridx = 0;
    int x = 0, y = 0;

    WINDOW *win = curses_get_nhwin(STATUS_WIN);
    if (border) x++,y++;
    for (i = 0; fieldorder[i] != BL_FLUSH; ++i) {
        int fldidx1 = fieldorder[i];
        if (status_activefields[fldidx1]) {
            if (fldidx1 != BL_TITLE || !iflags.wc2_hitpointbar) {

                if (fldidx1 != BL_CONDITION) {
#ifdef TEXTCOLOR
                    coloridx = curses_status_colors[fldidx1] & 0x00FF;
#endif
                    attridx = (curses_status_colors[fldidx1] & 0xFF00) >> 8;
                    text = status_vals_long[fldidx1];
                    if (iflags.hilite_delta) {
                        if (*text == ' ') {
                            putstr(STATUS_WIN, 0, " ");
                            text++;
                        }
                        /* multiple attributes can be in effect concurrently */
                        Begin_Attr(attridx);
#ifdef TEXTCOLOR
                        if (coloridx != NO_COLOR && coloridx != CLR_MAX)
                            curses_toggle_color_attr(win, coloridx, NONE, ON);
#endif
                    }

                    if (fldidx1 != BL_HPMAX
                        && fldidx1 != BL_ENEMAX
                        && fldidx1 != BL_EXP)
                        wmove(win, y++, x); /* everything on a new line except the above */

                    putstr(STATUS_WIN, 0, text);

                    if (fldidx1 == BL_TITLE) y++;

                    if (iflags.hilite_delta) {
#ifdef TEXTCOLOR
                        if (coloridx != NO_COLOR)
                            curses_toggle_color_attr(win, coloridx,NONE,OFF);
#endif
                        End_Attr(attridx);
                    }
                } else { /* condition */
                    MaybeDisplayCond(BL_MASK_STONE, "Stone");
                    MaybeDisplayCond(BL_MASK_SLIME, "Slime");
                    MaybeDisplayCond(BL_MASK_STRNGL, "Strngl");
                    MaybeDisplayCond(BL_MASK_FOODPOIS, "FoodPois");
                    MaybeDisplayCond(BL_MASK_TERMILL, "TermIll");
                    MaybeDisplayCond(BL_MASK_BLIND, "Blind");
                    MaybeDisplayCond(BL_MASK_DEAF, "Deaf");
                    MaybeDisplayCond(BL_MASK_STUN, "Stun");
                    MaybeDisplayCond(BL_MASK_CONF, "Conf");
                    MaybeDisplayCond(BL_MASK_HALLU, "Hallu");
                    MaybeDisplayCond(BL_MASK_LEV, "Lev");
                    MaybeDisplayCond(BL_MASK_FLY, "Fly");
                    MaybeDisplayCond(BL_MASK_RIDE, "Ride");
                }
            } else {
                /* hitpointbar using hp percent calculation */
                int bar_pos, bar_len;
                char *bar2 = (char *)0;
                char bar[MAXCO], savedch;
                boolean twoparts = FALSE;
                int height,width;

                text = status_vals[fldidx1];
                getmaxyx(win, height, width);
                bar_len = min(strlen(text), width - (border ? 4 : 2));
                text[bar_len] = '\0';
                if (bar_len < MAXCO-1) {
                    Strcpy(bar, text);
                    bar_pos = (bar_len * hpbar_percent) / 100;
                    if (bar_pos < 1 && hpbar_percent > 0)
                        bar_pos = 1;
                    if (bar_pos >= bar_len && hpbar_percent < 100)
                        bar_pos = bar_len - 1;
                    if (bar_pos > 0 && bar_pos < bar_len) {
                        twoparts = TRUE;
                        bar2 = &bar[bar_pos];
                        savedch = *bar2;
                        *bar2 = '\0';
                    }
                }
                wmove(win, y++, x);
                if (iflags.hilite_delta && iflags.wc2_hitpointbar) {
                    putstr(STATUS_WIN, 0, "[");
#ifdef TEXTCOLOR
                    coloridx = hpbar_color & 0x00FF;
                    /* attridx = (hpbar_color & 0xFF00) >> 8; */
                    if (coloridx != NO_COLOR)
                        curses_toggle_color_attr(win,coloridx,NONE,ON);
#endif
                    wattron(win,A_REVERSE);
                    putstr(STATUS_WIN, 0, bar);
                    wattroff(win,A_REVERSE);
#ifdef TEXTCOLOR
                    if (coloridx != NO_COLOR)
                        curses_toggle_color_attr(win,coloridx,NONE,OFF);
#endif
                    if (twoparts) {
                        *bar2 = savedch;
                        putstr(STATUS_WIN, 0, bar2);
                    }
                    putstr(STATUS_WIN, 0, "]");
                } else
                    putstr(STATUS_WIN, 0, text);
                y++; /* blank line after title */
            }
        }
    }
    return;
}

#ifdef TEXTCOLOR
/*
 * Return what color this condition should
 * be displayed in based on user settings.
 */
int condcolor(bm, bmarray)
long bm;
unsigned long *bmarray;
{
    int i;

    if (bm && bmarray)
        for (i = 0; i < CLR_MAX; ++i) {
            if (bmarray[i] && (bm & bmarray[i]))
                return i;
        }
    return NO_COLOR;
}
#endif /* TEXTCOLOR */

int condattr(bm, bmarray)
long bm;
unsigned long *bmarray;
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
#endif /* STATUS_HILITES */

#if 0 //old stuff to be re-incorporated
/* Private declarations */

/* Used to track previous value of things, to highlight changes. */
typedef struct nhs {
    long value;
    int highlight_turns;
    int highlight_color;
} nhstat;

static attr_t get_trouble_color(const char *);
static void draw_trouble_str(const char *);
static void print_statdiff(const char *append, nhstat *, int, int);
static void get_playerrank(char *);
static int hpen_color(boolean, int, int);
static void draw_bar(boolean, int, int, const char *);
static void draw_horizontal(int, int, int, int);
static void draw_horizontal_new(int, int, int, int);
static void draw_vertical(int, int, int, int);
static void curses_add_statuses(WINDOW *, boolean, boolean, int *, int *);
static void curses_add_status(WINDOW *, boolean, boolean, int *, int *,
                              const char *, int);
static int decrement_highlight(nhstat *, boolean);

#ifdef STATUS_COLORS
static attr_t hpen_color_attr(boolean, int, int);
extern struct color_option text_color_of(const char *text,
                                         const struct text_color_option *color_options);
struct color_option percentage_color_of(int value, int max,
                                        const struct percent_color_option *color_options);

extern const struct text_color_option *text_colors;
extern const struct percent_color_option *hp_colors;
extern const struct percent_color_option *pw_colors;
#endif

/* Whether or not we have printed status window content at least once.
   Used to ensure that prev* doesn't end up highlighted on game start. */
static boolean first = TRUE;
static nhstat prevdepth;
static nhstat prevstr;
static nhstat prevint;
static nhstat prevwis;
static nhstat prevdex;
static nhstat prevcon;
static nhstat prevcha;
static nhstat prevau;
static nhstat prevlevel;
static nhstat prevac;
static nhstat prevexp;
static nhstat prevtime;

#ifdef SCORE_ON_BOTL
static nhstat prevscore;
#endif

extern const char *hu_stat[];   /* from eat.c */
extern const char *enc_stat[];  /* from botl.c */

/* If the statuscolors patch isn't enabled, have some default colors for status problems
   anyway */

struct statcolor {
    const char *txt; /* For status problems */
    int color; /* Default color assuming STATUS_COLORS isn't enabled */
};

static const struct statcolor default_colors[] = {
    {"Satiated", CLR_YELLOW},
    {"Hungry", CLR_YELLOW},
    {"Weak", CLR_ORANGE},
    {"Fainted", CLR_BRIGHT_MAGENTA},
    {"Fainting", CLR_BRIGHT_MAGENTA},
    {"Burdened", CLR_RED},
    {"Stressed", CLR_RED},
    {"Strained", CLR_ORANGE},
    {"Overtaxed", CLR_ORANGE},
    {"Overloaded", CLR_BRIGHT_MAGENTA},
    {"Conf", CLR_BRIGHT_BLUE},
    {"Blind", CLR_BRIGHT_BLUE},
    {"Stun", CLR_BRIGHT_BLUE},
    {"Hallu", CLR_BRIGHT_BLUE},
    {"Ill", CLR_BRIGHT_MAGENTA},
    {"FoodPois", CLR_BRIGHT_MAGENTA},
    {"Slime", CLR_BRIGHT_MAGENTA},
    {NULL, NULL, NO_COLOR},
};

static attr_t
get_trouble_color(const char *stat)
{
    attr_t res = curses_color_attr(CLR_GRAY, 0);
    const struct statcolor *clr;
    for (clr = default_colors; clr->txt; clr++) {
        if (stat && !strcmp(clr->txt, stat)) {
#ifdef STATUS_COLORS
            /* Check if we have a color enabled with statuscolors */
            if (!iflags.use_status_colors)
                return curses_color_attr(CLR_GRAY, 0); /* no color configured */

            struct color_option stat_color;

            stat_color = text_color_of(clr->txt, text_colors);
            if (stat_color.color == NO_COLOR && !stat_color.attr_bits)
                return curses_color_attr(CLR_GRAY, 0);

            if (stat_color.color != NO_COLOR)
                res = curses_color_attr(stat_color.color, 0);

            res = curses_color_attr(stat_color.color, 0);
            int count;
            for (count = 0; (1 << count) <= stat_color.attr_bits; count++) {
                if (count != ATR_NONE &&
                    (stat_color.attr_bits & (1 << count)))
                    res |= curses_convert_attr(count);
            }

            return res;
#else
            return curses_color_attr(clr->color, 0);
#endif
        }
    }

    return res;
}

/* TODO: This is in the wrong place. */
void
get_playerrank(char *rank)
{
    char buf[BUFSZ];
    if (Upolyd) {
        int k = 0;

        Strcpy(buf, mons[u.umonnum].mname);
        while(buf[k] != 0) {
            if ((k == 0 || (k > 0 && buf[k-1] == ' ')) &&
                'a' <= buf[k] && buf[k] <= 'z')
                buf[k] += 'A' - 'a';
            k++;
        }
        Strcpy(rank, buf);
    } else
        Strcpy(rank, rank_of(u.ulevel, Role_switch, flags.female));
}

/* Handles numerical stat changes of various kinds.
   type is generally STAT_OTHER (generic "do nothing special"),
   but is used if the stat needs to be handled in a special way. */
static void
print_statdiff(const char *append, nhstat *stat, int new, int type)
{
    char buf[BUFSZ];
    WINDOW *win = curses_get_nhwin(STATUS_WIN);

    int color = CLR_GRAY;

    /* Turncount isn't highlighted, or it would be highlighted constantly. */
    if (type != STAT_TIME && new != stat->value) {
        /* Less AC is better */
        if ((type == STAT_AC && new < stat->value) ||
            (type != STAT_AC && new > stat->value)) {
            color = STAT_UP_COLOR;
            if (type == STAT_GOLD)
                color = HI_GOLD;
        } else
            color = STAT_DOWN_COLOR;

        stat->value = new;
        stat->highlight_color = color;
        stat->highlight_turns = 5;
    } else if (stat->highlight_turns)
        color = stat->highlight_color;

    attr_t attr = curses_color_attr(color, 0);
    wattron(win, attr);
    wprintw(win, "%s", append);
    if (type == STAT_STR && new > 18) {
        if (new > 118)
            wprintw(win, "%d", new - 100);
        else if (new == 118)
            wprintw(win, "18/**");
        else
            wprintw(win, "18/%02d", new - 18);
    } else
        wprintw(win, "%d", new);

    wattroff(win, attr);
}

static void
draw_trouble_str(const char *str)
{
    WINDOW *win = curses_get_nhwin(STATUS_WIN);

    attr_t attr = get_trouble_color(str);
    wattron(win, attr);
    wprintw(win, "%s", str);
    wattroff(win, attr);
}

/* Returns a ncurses attribute for foreground and background.
   This should probably be in cursinit.c or something. */
attr_t
curses_color_attr(int nh_color, int bg_color)
{
    int color = nh_color + 1;
    attr_t cattr = A_NORMAL;

    if (!nh_color) {
#ifdef USE_DARKGRAY
        if (iflags.wc2_darkgray) {
            if (!can_change_color() || COLORS <= 16)
                cattr |= A_BOLD;
        } else
#endif
            color = COLOR_BLUE;
    }

    if (COLORS < 16 && color > 8) {
        color -= 8;
        cattr = A_BOLD;
    }

    /* Can we do background colors? We can if we have more than
       16*7 colors (more than 8*7 for terminals with bold) */
    if (COLOR_PAIRS > (COLORS >= 16 ? 16 : 8) * 7) {
        /* NH3 has a rather overcomplicated way of defining
           its colors past the first 16:
           Pair    Foreground  Background
           17      Black       Red
           18      Black       Blue
           19      Red         Red
           20      Red         Blue
           21      Green       Red
           ...
           (Foreground order: Black, Red, Green, Yellow, Blue,
           Magenta, Cyan, Gray/White)

           To work around these oddities, we define backgrounds
           by the following pairs:

           16 COLORS
           49-64: Green
           65-80: Yellow
           81-96: Magenta
           97-112: Cyan
           113-128: Gray/White

           8 COLORS
           9-16: Green
           33-40: Yellow
           41-48: Magenta
           49-56: Cyan
           57-64: Gray/White */

        if (bg_color == nh_color)
            color = 1; /* Make foreground black if fg==bg */

        if (bg_color == CLR_RED || bg_color == CLR_BLUE) {
            /* already defined before extension */
            color *= 2;
            color += 16;
            if (bg_color == CLR_RED)
                color--;
        } else {
            boolean hicolor = FALSE;
            if (COLORS >= 16)
                hicolor = TRUE;

            switch (bg_color) {
            case CLR_GREEN:
                color = (hicolor ? 48 : 8) + color;
                break;
            case CLR_BROWN:
                color = (hicolor ? 64 : 32) + color;
                break;
            case CLR_MAGENTA:
                color = (hicolor ? 80 : 40) + color;
                break;
            case CLR_CYAN:
                color = (hicolor ? 96 : 48) + color;
                break;
            case CLR_GRAY:
                color = (hicolor ? 112 : 56) + color;
                break;
            default:
                break;
            }
        }
    }
    cattr |= COLOR_PAIR(color);

    return cattr;
}

/* Returns a complete curses attribute. Used to possibly bold/underline/etc HP/Pw. */
#ifdef STATUS_COLORS
static attr_t
hpen_color_attr(boolean is_hp, int cur, int max)
{
    struct color_option stat_color;
    int count;
    attr_t attr = 0;
    if (!iflags.use_status_colors)
        return curses_color_attr(CLR_GRAY, 0);

    stat_color = percentage_color_of(cur, max, is_hp ? hp_colors : pw_colors);

    if (stat_color.color != NO_COLOR)
        attr |= curses_color_attr(stat_color.color, 0);

    for (count = 0; (1 << count) <= stat_color.attr_bits; count++) {
        if (count != ATR_NONE && (stat_color.attr_bits & (1 << count)))
            attr |= curses_convert_attr(count);
    }

    return attr;
}
#endif

/* Return color for the HP bar.
   With status colors ON, this respect its configuration (defaulting to gray), but
   only obeys the color (no weird attributes for the HP bar).
   With status colors OFF, this returns reasonable defaults which are also used
   for the HP/Pw text itself. */
static int
hpen_color(boolean is_hp, int cur, int max)
{
#ifdef STATUS_COLORS
    if (iflags.use_status_colors) {
        struct color_option stat_color;
        stat_color = percentage_color_of(cur, max, is_hp ? hp_colors : pw_colors);

        if (stat_color.color == NO_COLOR)
            return CLR_GRAY;
        else
            return stat_color.color;
    } else
        return CLR_GRAY;
#endif

    int color = CLR_GRAY;
    if (cur == max)
        color = CLR_GRAY;
    else if (cur * 3 > max * 2) /* >2/3 */
        color = is_hp ? CLR_GREEN : CLR_CYAN;
    else if (cur * 3 > max) /* >1/3 */
        color = is_hp ? CLR_YELLOW : CLR_BLUE;
    else if (cur * 7 > max) /* >1/7 */
        color = is_hp ? CLR_RED : CLR_MAGENTA;
    else
        color = is_hp ? CLR_ORANGE : CLR_BRIGHT_MAGENTA;

    return color;
}

/* Draws a bar
   is_hp: TRUE if we're drawing HP, Pw otherwise (determines colors)
   cur/max: Current/max HP/Pw
   title: Not NULL if we are drawing as part of an existing title.
   Otherwise, the format is as follows: [   11 / 11   ] */
static void
draw_bar(boolean is_hp, int cur, int max, const char *title)
{
    WINDOW *win = curses_get_nhwin(STATUS_WIN);

#ifdef STATUS_COLORS
    if (!iflags.hitpointbar) {
        wprintw(win, "%s", !title ? "---" : title);
        return;
    }
#endif

    char buf[BUFSZ];
    if (title)
        Strcpy(buf, title);
    else {
        int len = 5;
        sprintf(buf, "%*d / %-*d", len, cur, len, max);
    }

    /* Colors */
    attr_t fillattr, attr;
    int color = hpen_color(is_hp, cur, max);
    int invcolor = color & 7;

    fillattr = curses_color_attr(color, invcolor);
    attr = curses_color_attr(color, 0);

    /* Figure out how much of the bar to fill */
    int fill = 0;
    int len = strlen(buf);
    if (cur > 0 && max > 0)
        fill = len * cur / max;
    if (fill > len)
        fill = len;

    waddch(win, '[');
    wattron(win, fillattr);
    wprintw(win, "%.*s", fill, buf);
    wattroff(win, fillattr);
    wattron(win, attr);
    wprintw(win, "%.*s", len - fill, &buf[fill]);
    wattroff(win, attr);
    waddch(win, ']');
}

/* Update the status win - this is called when NetHack would normally
   write to the status window, so we know somwthing has changed.  We
   override the write and update what needs to be updated ourselves. */
void
curses_update_stats(void)
{
    WINDOW *win = curses_get_nhwin(STATUS_WIN);

    /* Clear the window */
    werase(win);

    int orient = curses_get_window_orientation(STATUS_WIN);

    boolean horiz = FALSE;
    if ((orient != ALIGN_RIGHT) && (orient != ALIGN_LEFT))
        horiz = TRUE;
    boolean border = curses_window_has_border(STATUS_WIN);

    /* Figure out if we have proper window dimensions for horizontal statusbar. */
    if (horiz) {
        /* correct y */
        int cy = 3;
        if (iflags.classic_status)
            cy = 2;

        /* actual y (and x) */
        int ax = 0;
        int ay = 0;
        getmaxyx(win, ay, ax);
        if (border)
            ay -= 2;

        if (cy != ay) {
            curses_create_main_windows();
            curses_last_messages();
            doredraw();

            /* Reset XP highlight (since classic_status and new show different numbers) */
            prevexp.highlight_turns = 0;
            curses_update_stats();
            return;
        }
    }

    /* Starting x/y. Passed to draw_horizontal/draw_vertical to keep track of
       window positioning. */
    int x = 0;
    int y = 0;

    /* Don't start at border position if applicable */
    if (border) {
        x++;
        y++;
    }

    /* Get HP values. */
    int hp = u.uhp;
    int hpmax = u.uhpmax;
    if (Upolyd) {
        hp = u.mh;
        hpmax = u.mhmax;
    }

    if (orient != ALIGN_RIGHT && orient != ALIGN_LEFT)
        draw_horizontal(x, y, hp, hpmax);
    else
        draw_vertical(x, y, hp, hpmax);

    if (border)
        box(win, 0, 0);

    wnoutrefresh(win);

    if (first) {
        first = FALSE;

        /* Zero highlight timers. This will call curses_update_status again if needed */
        curses_decrement_highlights(TRUE);
    }
}

static void
draw_horizontal(int x, int y, int hp, int hpmax)
{
    if (!iflags.classic_status) {
        /* Draw new-style statusbar */
        draw_horizontal_new(x, y, hp, hpmax);
        return;
    }
    char buf[BUFSZ];
    char rank[BUFSZ];
    WINDOW *win = curses_get_nhwin(STATUS_WIN);

    /* Line 1 */
    wmove(win, y, x);

    get_playerrank(rank);
    sprintf(buf, "%s the %s", plname, rank);

    /* Use the title as HP bar (similar to hitpointbar) */
    draw_bar(TRUE, hp, hpmax, buf);

    /* Attributes */
    print_statdiff(" St:", &prevstr, ACURR(A_STR), STAT_STR);
    print_statdiff(" Dx:", &prevdex, ACURR(A_DEX), STAT_OTHER);
    print_statdiff(" Co:", &prevcon, ACURR(A_CON), STAT_OTHER);
    print_statdiff(" In:", &prevint, ACURR(A_INT), STAT_OTHER);
    print_statdiff(" Wi:", &prevwis, ACURR(A_WIS), STAT_OTHER);
    print_statdiff(" Ch:", &prevcha, ACURR(A_CHA), STAT_OTHER);

    wprintw(win, (u.ualign.type == A_CHAOTIC ? " Chaotic" :
                  u.ualign.type == A_NEUTRAL ? " Neutral" : " Lawful"));

#ifdef SCORE_ON_BOTL
    if (flags.showscore)
        print_statdiff(" S:", &prevscore, botl_score(), STAT_OTHER);
#endif /* SCORE_ON_BOTL */


    /* Line 2 */
    y++;
    wmove(win, y, x);

    describe_level(buf);

    wprintw(win, "%s", buf);

#ifndef GOLDOBJ
    print_statdiff("$", &prevau, u.ugold, STAT_GOLD);
#else
    print_statdiff("$", &prevau, money_cnt(invent), STAT_GOLD);
#endif

    /* HP/Pw use special coloring rules */
    attr_t hpattr, pwattr;
#ifdef STATUS_COLORS
    hpattr = hpen_color_attr(TRUE, hp, hpmax);
    pwattr = hpen_color_attr(FALSE, u.uen, u.uenmax);
#else
    int hpcolor, pwcolor;
    hpcolor = hpen_color(TRUE, hp, hpmax);
    pwcolor = hpen_color(FALSE, u.uen, u.uenmax);
    hpattr = curses_color_attr(hpcolor, 0);
    pwattr = curses_color_attr(pwcolor, 0);
#endif
    wprintw(win, " HP:");
    wattron(win, hpattr);
    wprintw(win, "%d(%d)", (hp < 0) ? 0 : hp, hpmax);
    wattroff(win, hpattr);

    wprintw(win, " Pw:");
    wattron(win, pwattr);
    wprintw(win, "%d(%d)", u.uen, u.uenmax);
    wattroff(win, pwattr);

    print_statdiff(" AC:", &prevac, u.uac, STAT_AC);

    if (Upolyd)
        print_statdiff(" HD:", &prevlevel, mons[u.umonnum].mlevel, STAT_OTHER);
#ifdef EXP_ON_BOTL
    else if (flags.showexp) {
        print_statdiff(" Xp:", &prevlevel, u.ulevel, STAT_OTHER);
        /* use waddch, we don't want to highlight the '/' */
        waddch(win, '/');
        print_statdiff("", &prevexp, u.uexp, STAT_OTHER);
    }
#endif
    else
        print_statdiff(" Exp:", &prevlevel, u.ulevel, STAT_OTHER);

    if (flags.time)
        print_statdiff(" T:", &prevtime, moves, STAT_TIME);

    curses_add_statuses(win, FALSE, FALSE, NULL, NULL);
}

static void
draw_horizontal_new(int x, int y, int hp, int hpmax)
{
    char buf[BUFSZ];
    char rank[BUFSZ];
    WINDOW *win = curses_get_nhwin(STATUS_WIN);

    /* Line 1 */
    wmove(win, y, x);

    get_playerrank(rank);
    char race[BUFSZ];
    Strcpy(race, urace.adj);
    race[0] = highc(race[0]);
    wprintw(win, "%s the %s %s%s%s", plname,
            (u.ualign.type == A_CHAOTIC ? "Chaotic" :
             u.ualign.type == A_NEUTRAL ? "Neutral" : "Lawful"),
            Upolyd ? "" : race, Upolyd ? "" : " ",
            rank);

    /* Line 2 */
    y++;
    wmove(win, y, x);
    wprintw(win, "HP:");
    draw_bar(TRUE, hp, hpmax, NULL);
    print_statdiff(" AC:", &prevac, u.uac, STAT_AC);
    if (Upolyd)
        print_statdiff(" HD:", &prevlevel, mons[u.umonnum].mlevel, STAT_OTHER);
#ifdef EXP_ON_BOTL
    else if (flags.showexp) {
        /* Ensure that Xp have proper highlight on level change. */
        int levelchange = 0;
        if (prevlevel.value != u.ulevel) {
            if (prevlevel.value < u.ulevel)
                levelchange = 1;
            else
                levelchange = 2;
        }
        print_statdiff(" Xp:", &prevlevel, u.ulevel, STAT_OTHER);
        /* use waddch, we don't want to highlight the '/' */
        waddch(win, '(');

        /* Figure out amount of Xp needed to next level */
        int xp_left = 0;
        if (u.ulevel < 30)
            xp_left = (newuexp(u.ulevel) - u.uexp);

        if (levelchange) {
            prevexp.value = (xp_left + 1);
            if (levelchange == 2)
                prevexp.value = (xp_left - 1);
        }
        print_statdiff("", &prevexp, xp_left, STAT_AC);
        waddch(win, ')');
    }
#endif
    else
        print_statdiff(" Exp:", &prevlevel, u.ulevel, STAT_OTHER);

    waddch(win, ' ');
    describe_level(buf);

    wprintw(win, "%s", buf);

    /* Line 3 */
    y++;
    wmove(win, y, x);
    wprintw(win, "Pw:");
    draw_bar(FALSE, u.uen, u.uenmax, NULL);

#ifndef GOLDOBJ
    print_statdiff(" $", &prevau, u.ugold, STAT_GOLD);
#else
    print_statdiff(" $", &prevau, money_cnt(invent), STAT_GOLD);
#endif

#ifdef SCORE_ON_BOTL
    if (flags.showscore)
        print_statdiff(" S:", &prevscore, botl_score(), STAT_OTHER);
#endif /* SCORE_ON_BOTL */

    if (flags.time)
        print_statdiff(" T:", &prevtime, moves, STAT_TIME);

    curses_add_statuses(win, TRUE, FALSE, &x, &y);

    /* Right-aligned attributes */
    int stat_length = 6; /* " Dx:xx" */
    int str_length = 6;
    if (ACURR(A_STR) > 18 && ACURR(A_STR) < 119)
        str_length = 9;

    getmaxyx(win, y, x);

    /* We want to deal with top line of y. getmaxx would do what we want, but it only
       exist for compatibility reasons and might not exist at all in some versions. */
    y = 0;
    if (curses_window_has_border(STATUS_WIN)) {
        x--;
        y++;
    }

    x -= stat_length;
    int orig_x = x;
    wmove(win, y, x);
    print_statdiff(" Co:", &prevcon, ACURR(A_CON), STAT_OTHER);
    x -= stat_length;
    wmove(win, y, x);
    print_statdiff(" Dx:", &prevdex, ACURR(A_DEX), STAT_OTHER);
    x -= str_length;
    wmove(win, y, x);
    print_statdiff(" St:", &prevstr, ACURR(A_STR), STAT_STR);

    x = orig_x;
    y++;
    wmove(win, y, x);
    print_statdiff(" Ch:", &prevcha, ACURR(A_CHA), STAT_OTHER);
    x -= stat_length;
    wmove(win, y, x);
    print_statdiff(" Wi:", &prevwis, ACURR(A_WIS), STAT_OTHER);
    x -= str_length;
    wmove(win, y, x);
    print_statdiff(" In:", &prevint, ACURR(A_INT), STAT_OTHER);
}

/* Personally I never understood the point of a vertical status bar. But removing the
   option would be silly, so keep the functionality. */
static void
draw_vertical(int x, int y, int hp, int hpmax)
{
    char buf[BUFSZ];
    char rank[BUFSZ];
    WINDOW *win = curses_get_nhwin(STATUS_WIN);

    /* Print title and dungeon branch */
    wmove(win, y++, x);

    get_playerrank(rank);
    int ranklen = strlen(rank);
    int namelen = strlen(plname);
    int maxlen = 19;
#ifdef STATUS_COLORS
    if (!iflags.hitpointbar)
        maxlen += 2; /* With no hitpointbar, we can fit more since there's no "[]" */
#endif

    if ((ranklen + namelen) > maxlen) {
        /* The result doesn't fit. Strip name if >10 characters, then strip title */
        if (namelen > 10) {
            while (namelen > 10 && (ranklen + namelen) > maxlen)
                namelen--;
        }

        while ((ranklen + namelen) > maxlen)
            ranklen--; /* Still doesn't fit, strip rank */
    }
    sprintf(buf, "%-*s the %-*s", namelen, plname, ranklen, rank);
    draw_bar(TRUE, hp, hpmax, buf);
    wmove(win, y++, x);
    wprintw(win, "%s", dungeons[u.uz.dnum].dname);

    y++; /* Blank line inbetween */
    wmove(win, y++, x);

    /* Attributes. Old  vertical order is preserved */
    print_statdiff("Strength:      ", &prevstr, ACURR(A_STR), STAT_STR);
    wmove(win, y++, x);
    print_statdiff("Intelligence:  ", &prevint, ACURR(A_INT), STAT_OTHER);
    wmove(win, y++, x);
    print_statdiff("Wisdom:        ", &prevwis, ACURR(A_WIS), STAT_OTHER);
    wmove(win, y++, x);
    print_statdiff("Dexterity:     ", &prevdex, ACURR(A_DEX), STAT_OTHER);
    wmove(win, y++, x);
    print_statdiff("Constitution:  ", &prevcon, ACURR(A_CON), STAT_OTHER);
    wmove(win, y++, x);
    print_statdiff("Charisma:      ", &prevcha, ACURR(A_CHA), STAT_OTHER);
    wmove(win, y++, x);
    wprintw(win,   "Alignment:     ");
    wprintw(win, (u.ualign.type == A_CHAOTIC ? "Chaotic" :
                  u.ualign.type == A_NEUTRAL ? "Neutral" : "Lawful"));
    wmove(win, y++, x);
    wprintw(win,   "Dungeon Level: ");

    /* Astral Plane doesn't fit */
    if (In_endgame(&u.uz))
        wprintw(win, "%s", Is_astralevel(&u.uz) ? "Astral" : "End Game");
    else
        wprintw(win, "%d", depth(&u.uz));
    wmove(win, y++, x);

#ifndef GOLDOBJ
    print_statdiff("Gold:          ", &prevau, u.ugold, STAT_GOLD);
#else
    print_statdiff("Gold:          ", &prevau, money_cnt(invent), STAT_GOLD);
#endif
    wmove(win, y++, x);

    /* HP/Pw use special coloring rules */
    attr_t hpattr, pwattr;
#ifdef STATUS_COLORS
    hpattr = hpen_color_attr(TRUE, hp, hpmax);
    pwattr = hpen_color_attr(FALSE, u.uen, u.uenmax);
#else
    int hpcolor, pwcolor;
    hpcolor = hpen_color(TRUE, hp, hpmax);
    pwcolor = hpen_color(FALSE, u.uen, u.uenmax);
    hpattr = curses_color_attr(hpcolor, 0);
    pwattr = curses_color_attr(pwcolor, 0);
#endif

    wprintw(win,   "Hit Points:    ");
    wattron(win, hpattr);
    wprintw(win, "%d/%d", (hp < 0) ? 0 : hp, hpmax);
    wattroff(win, hpattr);
    wmove(win, y++, x);

    wprintw(win,   "Magic Power:   ");
    wattron(win, pwattr);
    wprintw(win, "%d/%d", u.uen, u.uenmax);
    wattroff(win, pwattr);
    wmove(win, y++, x);

    print_statdiff("Armor Class:   ", &prevac, u.uac, STAT_AC);
    wmove(win, y++, x);

    if (Upolyd)
        print_statdiff("Hit Dice:      ", &prevlevel, mons[u.umonnum].mlevel, STAT_OTHER);
#ifdef EXP_ON_BOTL
    else if (flags.showexp) {
        print_statdiff("Experience:    ", &prevlevel, u.ulevel, STAT_OTHER);
        /* use waddch, we don't want to highlight the '/' */
        waddch(win, '/');
        print_statdiff("", &prevexp, u.uexp, STAT_OTHER);
    }
#endif
    else
        print_statdiff("Level:         ", &prevlevel, u.ulevel, STAT_OTHER);
    wmove(win, y++, x);

    if (flags.time) {
        print_statdiff("Time:          ", &prevtime, moves, STAT_TIME);
        wmove(win, y++, x);
    }

#ifdef SCORE_ON_BOTL
    if (flags.showscore) {
        print_statdiff("Score:         ", &prevscore, botl_score(), STAT_OTHER);
        wmove(win, y++, x);
    }
#endif /* SCORE_ON_BOTL */

    curses_add_statuses(win, FALSE, TRUE, &x, &y);
}

static void
curses_add_statuses(WINDOW *win, boolean align_right,
                    boolean vertical, int *x, int *y)
{
    if (align_right) {
        /* Right-aligned statuses. Since add_status decrease one x more
           (to separate them with spaces), add 1 to x unless we have borders
           (which would offset what add_status does) */
        int mx = *x;
        int my = *y;
        getmaxyx(win, my, mx);
        if (!curses_window_has_border(STATUS_WIN))
            mx++;

        *x = mx;
    }

#define statprob(str, trouble)                                  \
    curses_add_status(win, align_right, vertical, x, y, str, trouble)

    /* Hunger */
    statprob(hu_stat[u.uhs], u.uhs != 1); /* 1 is NOT_HUNGRY (not defined here) */

    /* General troubles */
    statprob("Conf",     Confusion);
    statprob("Blind",    Blind);
    statprob("Stun",     Stunned);
    statprob("Hallu",    Hallucination);
    statprob("Ill",      (u.usick_type & SICK_NONVOMITABLE));
    statprob("FoodPois", (u.usick_type & SICK_VOMITABLE));
    statprob("Slime",    Slimed);

    /* Encumbrance */
    int enc = near_capacity();
    statprob(enc_stat[enc], enc > UNENCUMBERED);
#undef statprob
}

static void
curses_add_status(WINDOW *win, boolean align_right, boolean vertical,
                  int *x, int *y, const char *str, int trouble)
{
    /* If vertical is TRUE here with no x/y, that's an error. But handle
       it gracefully since NH3 doesn't recover well in crashes. */
    if (!x || !y)
        vertical = FALSE;

    if (!trouble)
        return;

    if (!vertical && !align_right)
        waddch(win, ' ');

    /* For whatever reason, hunger states have trailing spaces. Get rid of them. */
    char buf[BUFSZ];
    Strcpy(buf, str);
    int i;
    for (i = 0; (buf[i] != ' ' && buf[i] != '\0'); i++) ;

    buf[i] = '\0';
    if (align_right) {
        *x -= (strlen(buf) + 1); /* add spacing */
        wmove(win, *y, *x);
    }

    draw_trouble_str(buf);

    if (vertical) {
        wmove(win, *y, *x);
        *y += 1; /* ++ advances the pointer addr */
    }
}

/* Decrement a single highlight, return 1 if decremented to zero. zero is TRUE if we're
   zeroing the highlight. */
static int
decrement_highlight(nhstat *stat, boolean zero)
{
    if (stat->highlight_turns > 0) {
        if (zero) {
            stat->highlight_turns = 0;
            return 1;
        }

        stat->highlight_turns--;
        if (stat->highlight_turns == 0)
            return 1;
    }
    return 0;
}

/* Decrement the highlight_turns for all stats.  Call curses_update_stats
   if needed to unhighlight a stat */
void
curses_decrement_highlights(boolean zero)
{
    int unhighlight = 0;

    unhighlight |= decrement_highlight(&prevdepth, zero);
    unhighlight |= decrement_highlight(&prevstr, zero);
    unhighlight |= decrement_highlight(&prevdex, zero);
    unhighlight |= decrement_highlight(&prevcon, zero);
    unhighlight |= decrement_highlight(&prevint, zero);
    unhighlight |= decrement_highlight(&prevwis, zero);
    unhighlight |= decrement_highlight(&prevcha, zero);
    unhighlight |= decrement_highlight(&prevau, zero);
    unhighlight |= decrement_highlight(&prevlevel, zero);
    unhighlight |= decrement_highlight(&prevac, zero);
#ifdef EXP_ON_BOTL
    unhighlight |= decrement_highlight(&prevexp, zero);
#endif
    unhighlight |= decrement_highlight(&prevtime, zero);
#ifdef SCORE_ON_BOTL
    unhighlight |= decrement_highlight(&prevscore, zero);
#endif

    if (unhighlight)
        curses_update_stats();
}
#endif

/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.7 cursstat.c */
/* Copyright (c) Andy Thomson, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

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

static unsigned long *curses_colormasks;
static long curses_condition_bits;
static int curses_status_colors[MAXBLSTATS];
static int hpbar_percent, hpbar_color;
static int vert_status_dirty;

static void draw_status(void);
static void draw_vertical(boolean);
static void draw_horizontal(boolean);
static void curs_HPbar(char *, int);
static void curs_stat_conds(int, int, int *, int *, char *, boolean *);
static void curs_vert_status_vals(int);
#ifdef STATUS_HILITES
#ifdef TEXTCOLOR
static int condcolor(long, unsigned long *);
#endif
static int condattr(long, unsigned long *);
static int nhattr2curses(int);
#endif /* STATUS_HILITES */

/* width of a single line in vertical status orientation (one field per line;
   everything but title fits within 30 even with prefix and longest value) */
#define STATVAL_WIDTH 60 /* overkill; was MAXCO (200), massive overkill */

void
curses_status_init(void)
{
    int i;

    for (i = 0; i < MAXBLSTATS; ++i) {
        curses_status_colors[i] = NO_COLOR; /* no color and no attributes */
        status_vals_long[i] = (char *) alloc(STATVAL_WIDTH);
        *status_vals_long[i] = '\0';
    }
    curses_condition_bits = 0L;
    hpbar_percent = 0, hpbar_color = NO_COLOR;
    vert_status_dirty = 1;

    /* let genl_status_init do most of the initialization */
    genl_status_init();
    return;
}

void
curses_status_finish(void)
{
    int i;

    for (i = 0; i < MAXBLSTATS; ++i) {
        if (status_vals_long[i])
            free(status_vals_long[i]), status_vals_long[i] = (char *) 0;
    }

    genl_status_finish();
    return;
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

static int changed_fields = 0;

DISABLE_WARNING_FORMAT_NONLITERAL

void
curses_status_update(int fldidx, genericptr_t ptr, int chg UNUSED, int percent,
                     int color_and_attr, unsigned long *colormasks)
{
    long *condptr = (long *) ptr;
    char *text = (char *) ptr;

    if (fldidx != BL_FLUSH) {
        if (fldidx < 0 || fldidx >= MAXBLSTATS) {
            g.context.botlx = g.context.botl = FALSE; /* avoid another bot() */
            panic("curses_status_update(%d)", fldidx);
        }
        changed_fields |= (1 << fldidx);
        *status_vals[fldidx] = '\0';
        if (!status_activefields[fldidx])
            return;
        if (fldidx == BL_CONDITION) {
            curses_condition_bits = *condptr;
            curses_colormasks = colormasks;
        } else {
#ifndef TEXTCOLOR
            color_and_attr = (color_and_attr & ~0x00FF) | NO_COLOR;
#endif
            /*
             * status_vals[] are used for horizontal orientation
             *  (wide lines of multiple short values).
             * status_vals_long[] are used for vertical orientation
             *  (narrow-ish lines of one long value each [mostly]
             *  and get constructed at need from status_vals[] rather
             *  than the original values passed to status_update()).
             */
            if (fldidx == BL_GOLD) {
                /* decode once instead of every time it's displayed */
                status_vals[BL_GOLD][0] = ' ';
                text = decode_mixed(&status_vals[BL_GOLD][1], text);
            } else if ((fldidx == BL_HUNGER || fldidx == BL_CAP)
                       && (!*text || !strcmp(text, " "))) {
                /* fieldfmt[] is " %s"; avoid lone space when empty */
                *status_vals[fldidx] = '\0';
            } else {
                Sprintf(status_vals[fldidx],
                        (fldidx == BL_TITLE && iflags.wc2_hitpointbar)
                        ? "%-30s" : status_fieldfmt[fldidx]
                                    ? status_fieldfmt[fldidx] : "%s",
                        text);
                /* strip trailing spaces; core ought to do this for us */
                if (fldidx == BL_HUNGER || fldidx == BL_LEVELDESC)
                    (void) trimspaces(status_vals[fldidx]);
            }

            /* status_vals_long[] used to be set up here even when not
               in use; it has been moved to curs_vert_status_vals() */
            vert_status_dirty = 1;

            curses_status_colors[fldidx] = color_and_attr;
            if (iflags.wc2_hitpointbar && fldidx == BL_HP) {
                hpbar_percent = percent;
                hpbar_color = color_and_attr;
            }
        }
    } else { /* BL_FLUSH */
        draw_status();
        changed_fields = 0;
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

static void
draw_status(void)
{
    WINDOW *win = curses_get_nhwin(STATUS_WIN);
    orient statorient = (orient) curses_get_window_orientation(STATUS_WIN);
    boolean horiz = (statorient != ALIGN_RIGHT && statorient != ALIGN_LEFT);
    boolean border = curses_window_has_border(STATUS_WIN);

    /* Figure out if we have proper window dimensions for horizontal
       statusbar. */
    if (horiz) {
        int ax = 0, ay = 0;
        int cy = (iflags.wc2_statuslines < 3) ? 2 : 3;

        /* actual y (and x) */
        getmaxyx(win, ay, ax);
        if (border)
            ay -= 2;

        if (cy != ay) { /* something changed. Redo everything */
            curs_reset_windows(TRUE, TRUE);
            win = curses_get_nhwin(STATUS_WIN);
        }
        nhUse(ax);  /* getmaxyx macro isn't sufficient */
    }

    werase(win);
    if (horiz)
        draw_horizontal(border);
    else
        draw_vertical(border);

    if (border)
        box(win, 0, 0);
    wnoutrefresh(win);
}

/* horizontal layout on 2 or 3 lines */
static void
draw_horizontal(boolean border)
{
#define blPAD BL_FLUSH
    /* almost all fields already come with a leading space;
       "xspace" indicates places where we'll generate an extra one */
    static const enum statusfields
    twolineorder[3][15] = {
        { BL_TITLE,
          /*xspace*/ BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH,
          /*xspace*/ BL_ALIGN,
          /*xspace*/ BL_SCORE,
          BL_FLUSH, blPAD, blPAD, blPAD, blPAD, blPAD },
        { BL_LEVELDESC,
          /*xspace*/ BL_GOLD,
          /*xspace*/ BL_HP, BL_HPMAX,
          /*xspace*/ BL_ENE, BL_ENEMAX,
          /*xspace*/ BL_AC,
          /*xspace*/ BL_XP, BL_EXP, BL_HD,
          /*xspace*/ BL_TIME,
          /*xspace*/ BL_HUNGER, BL_CAP, BL_CONDITION,
          BL_FLUSH },
        { BL_FLUSH, blPAD, blPAD, blPAD, blPAD, blPAD, blPAD, blPAD,
          blPAD, blPAD, blPAD, blPAD, blPAD, blPAD, blPAD }
    },
    threelineorder[3][15] = { /* moves align to line 2, leveldesc+ to 3 */
        { BL_TITLE,
          /*xspace*/ BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH,
          /*xspace*/ BL_SCORE,
          BL_FLUSH, blPAD, blPAD, blPAD, blPAD, blPAD, blPAD },
        { BL_ALIGN,
          /*xspace*/ BL_GOLD,
          /*xspace*/ BL_HP, BL_HPMAX,
          /*xspace*/ BL_ENE, BL_ENEMAX,
          /*xspace*/ BL_AC,
          /*xspace*/ BL_XP, BL_EXP, BL_HD,
          /*xspace*/ BL_HUNGER, BL_CAP,
          BL_FLUSH, blPAD, blPAD },
        { BL_LEVELDESC,
          /*xspace*/ BL_TIME,
          /*xspecial*/ BL_CONDITION,
          BL_FLUSH, blPAD, blPAD, blPAD, blPAD, blPAD, blPAD, blPAD,
          blPAD, blPAD, blPAD, blPAD }
    };
    const enum statusfields (*fieldorder)[15];
    xchar spacing[MAXBLSTATS], valline[MAXBLSTATS];
    enum statusfields fld, prev_fld;
    char *text, *p, cbuf[BUFSZ], ebuf[STATVAL_WIDTH];
#ifdef SCORE_ON_BOTL
    char *colon;
    char sbuf[STATVAL_WIDTH];
#endif
    int i, j, number_of_lines,
        cap_and_hunger, exp_points, sho_score,
        height, width, w, xtra, clen, x, y, t, ex, ey,
        condstart = 0, conddummy = 0;
#ifdef STATUS_HILITES
    int coloridx = NO_COLOR, attrmask = 0;
#endif /* STATUS_HILITES */
    boolean asis = FALSE;
    WINDOW *win = curses_get_nhwin(STATUS_WIN);

    /* note: getmaxyx() is a macro which assigns values to height and width */
    getmaxyx(win, height, width);
    if (border)
        height -= 2, width -= 2;

    /*
     * Potential revisions:
     *  If line with gold is too long, strip "$:" prefix like is done
     *  for score's "S:", and/or convert "$:n" to n /= 1000 and "$:nK"
     *  if amount takes 4+ digits.  (No realistic chance for 7+ digits
     *  for gold so would recover only 2 columns.  n >>= 10 might have
     *  greater geek appeal but could lead to bug reports and couldn't
     *  be accomplished via simple string truncation.)
     *
     *  For experience point and score suppression, might try that first
     *  (better chance for column recovery, with "/nM" freeing 5 out of
     *  7+ digits; in rare instances, "/nG" could free 8 out of 10+ digits)
     *  before deciding to remove them altogether.
     *  tty's shorter condition designations combined with comparable
     *  trimming of hunger and encumbrance would be better overall.
     *
     *  For first line when hitpointbar is off, treat trailing spaces
     *  on Title as discardable leading spaces on Str.  (Enabling
     *  perm_invent without having a wide terminal size results in status
     *  being narrower than usual and possibly truncating by omitting
     *  right hand fields, emphasizing the wasted space devoted to
     *  title's trailing spaces.  Same issue without perm_invent if main
     *  window gets clipped to fit a narrow terminal.)
     */

    number_of_lines = (iflags.wc2_statuslines < 3) ? 2 : 3;
    fieldorder = (number_of_lines != 3) ? twolineorder : threelineorder;

    cbuf[0] = '\0';
    x = y = border ? 1 : 0; /* origin; ignored by curs_stat_conds(0) */
    /* collect active conditions in cbuf[], space separated, suitable
       for direct output if no highlighting is requested ('asis') but
       primarily used to measure the length */
    curs_stat_conds(0, 0, &x, &y, cbuf, &asis);
    clen = (int) strlen(cbuf);

    cap_and_hunger = 0;
    if (*status_vals[BL_HUNGER])
        cap_and_hunger |= 1;
    if (*status_vals[BL_CAP])
        cap_and_hunger |= 2;
    exp_points = (flags.showexp ? 1 : 0);
    /* don't bother conditionalizing this; always 0 for !SCORE_ON_BOTL */
    sho_score = (status_activefields[BL_SCORE] != 0);

    /* simplify testing which fields reside on which lines; assume line #0 */
    (void) memset((genericptr_t) valline, 0, sizeof valline);
    for (j = 1; j < number_of_lines; ++j)
        for (i = 0; (fld = fieldorder[j][i]) != BL_FLUSH; ++i)
            valline[fld] = j;

    /* iterate 0 and 1 and maybe 2 for status lines 1 and 2 and maybe 3 */
    for (j = 0; j < number_of_lines; ++j) {

 startover:
        /* first pass for line #j -- figure out spacing */
        (void) memset((genericptr_t) spacing, 0, sizeof spacing);
        w = xtra = 0; /* w: width so far; xtra: number of extra spaces */
        prev_fld = BL_FLUSH;
        for (i = 0; (fld = fieldorder[j][i]) != BL_FLUSH; ++i) {
            /* when the core marks a field as disabled, it doesn't call
               status_update() to tell us to throw away the old value, so
               polymorph leaves stale XP and rehumanize leaves stale HD */
            if (!status_activefields[fld])
                *status_vals[fld] = '\0';
            text = status_vals[fld];
            if (i == 0 && *text == ' ')
                ++text;
            /* most fields already include a leading space; we don't try to
               count those separately, they're just part of field's length */
            switch (fld) {
            case BL_EXP:
                spacing[fld] = 0; /* no leading or extra space */
                if (!exp_points)
                    continue;
                break;
            case BL_HPMAX:
            case BL_ENEMAX:
                spacing[fld] = 0; /* no leading or extra space */
                break;
            case BL_DX:
            case BL_CO:
            case BL_IN:
            case BL_WI:
            case BL_CH:
                spacing[fld] = 0; /* leading space but no extra space */
                break;
            case BL_TITLE:
                if (iflags.wc2_hitpointbar) {
                    w += 2; /* count '[' and ']' */
                    t = (int) strlen(text);
                    if (t != 30) /* HPbar() will use modified copy of title */
                        w -= (t - 30); /* '+= strlen()' below will add 't';
                                        * functional result being 'w += 30' */
                }
                /*FALLTHRU*/
            case BL_ALIGN:
            case BL_LEVELDESC:
                spacing[fld] = (i > 0 ? 1 : 0); /* extra space unless first */
                break;
            case BL_HUNGER:
                spacing[fld] = (cap_and_hunger & 1);
                break;
            case BL_CAP:
                spacing[fld] = (cap_and_hunger == 2);
                break;
            case BL_CONDITION:
                text = cbuf; /* for 'w += strlen(text)' below */
                spacing[fld] = (cap_and_hunger == 0);
                break;
            case BL_STR:
            case BL_HP:
            case BL_ENE:
            case BL_AC:
            case BL_GOLD:
                spacing[fld] = 1; /* always extra space */
                break;
            case BL_XP:
            case BL_HD:
            case BL_TIME:
                spacing[fld] = status_activefields[fld] ? 1 : 0;
                break;
            case BL_SCORE:
                spacing[fld] = sho_score ? 1 : 0;
                break;
            default:
                break;
            }
            w += (int) strlen(text);
            /* if preceding field has any trailing spaces, don't add extra;
               (should only apply to prev==title; status_update() handles
               others that used to have trailing spaces by stripping such) */
            if (spacing[fld] > 0 && prev_fld != BL_FLUSH
                && *(p = status_vals[prev_fld]) && *(eos(p) - 1) == ' '
                && (prev_fld != BL_TITLE || !iflags.wc2_hitpointbar))
                spacing[fld] = 0;
            xtra += spacing[fld];

            prev_fld = fld;
        }
        /* if the line is too long, first avoid extra spaces */
        fld = MAXBLSTATS;
        while (xtra > 0 && w + xtra > width) {
            while (--fld >= 0) /* [assumes 'fld' is not unsigned!] */
                if (spacing[fld] > 0) {
                    xtra -= spacing[fld];
                    spacing[fld] = 0;
                    break;
                }
        }
        w += xtra; /* simplify further width checks */
        /* if showing exper points and line is too wide, don't show them */
        if (w > width && exp_points && j == valline[BL_EXP]
            && ((*cbuf && j == valline[BL_CONDITION])
                || (cap_and_hunger && j == valline[BL_HUNGER]))) {
            exp_points = 0;
            goto startover;
        }
#ifdef SCORE_ON_BOTL
        if (sho_score && j == valline[BL_SCORE]) {
            /* no point in letting score become truncated on the right
               because showing fewer than all digits would be useless */
            if (w > width) {
                if (w - 2 <= width) {
                    sho_score |= 2; /* strip "S:" prefix */
                    w -= 2;
                } else {
                    sho_score = 0;
                    goto startover;
                }
            }
            /* right justify score unless window is very wide */
            t = COLNO + (int) strlen(status_vals[BL_SCORE]);
            if (t > width)
                t = width;
            if (w < t)
                spacing[BL_SCORE] += (t - w);
        }
#endif

        /* second pass for line #j -- render it */
        x = y = border ? 1 : 0;
        wmove(win, y + j, x);
        for (i = 0; (fld = fieldorder[j][i]) != BL_FLUSH; ++i) {
            if (!status_activefields[fld])
                continue;

            /* output any extra spaces that precede the current field
               (if current field will be suppressed for any reason then
               its spacing[] should be 0) */
            for (t = spacing[fld]; t > 0; --t)
                waddch(win, ' ');

            text = status_vals[fld];
            if (i == 0 && *text == ' ')
                ++text; /* for first field of line, discard leading space */

            switch (fld) {
            case BL_EXP:
                /* might be 'active' but suppressed due to lack of room */
                if (!exp_points)
                    continue;
                break;
            case BL_HUNGER:
                if (number_of_lines == 3) {
                    /* remember hunger's position */
                    getyx(win, conddummy, condstart);
                    /* if hunger won't be shown, figure out where cap
                       will be; if cap won't be shown either, use where
                       conditions would go if they were on this line */
                    condstart += (cap_and_hunger == 2) ? spacing[BL_CAP]
                                 : (cap_and_hunger == 0) ? 1 : 0;
                    nhUse(conddummy);   /* getyx needed 3 args */
                }
                if (!(cap_and_hunger & 1))
                    continue;
                break;
            case BL_CAP:
                /* always enabled but might be empty */
                if (!(cap_and_hunger & 2))
                    continue;
                /* check whether encumbrance is going to go past right edge
                   and wrap; if so, truncate it; (won't wrap on last line
                   of borderless window, but will when there's a border);
                   could only do that after all extra spaces are gone */
                if (!xtra) {
                     getyx(win, ey, ex);
                     t = (int) strlen(text);
                     if (ex + t > width - (border ? 0 : 1)) {
                         text = strcpy(ebuf, text);
                         t = (width - (border ? 0 : 1)) - (ex - 1);
                         ebuf[max(t, 2)] = '\0'; /* might still wrap... */
                     }
                     nhUse(ey); /* getyx needed 3 args */
                }
                break;
            case BL_SCORE:
#ifdef SCORE_ON_BOTL
                if ((sho_score & 2) != 0) { /* strip "S:" prefix */
                    if ((colon = index(text, ':')) != 0)
                        text = strcat(strcpy(sbuf, " "), colon + 1);
                    else
                        sho_score = 0;
                }
#endif
                if (!sho_score)
                    continue;
                break;
            default:
                break;
            }

            if (fld == BL_TITLE && iflags.wc2_hitpointbar) {
                /* hitpointbar using hp percent calculation; title width
                   is padded to 30 if shorter, truncated at 30 if longer;
                   overall width is 32 because of the enclosing brackets */
                curs_HPbar(text, 0);

            } else if (fld != BL_CONDITION) {
                /* regular field, including title if no hitpointbar */
#ifdef STATUS_HILITES
                coloridx = curses_status_colors[fld]; /* includes attribute */
                if (iflags.hilite_delta && coloridx != NO_COLOR) {
                    /* expect 1 leading space; don't highlight it */
                    while (*text == ' ') {
                        waddch(win, ' ');
                        ++text;
                    }
                    attrmask = (coloridx >> 8) & 0x00FF;
                    if (attrmask) {
                        attrmask = nhattr2curses(attrmask);
                        wattron(win, attrmask);
                    }
#ifdef TEXTCOLOR
                    coloridx &= 0x00FF;
                    if (coloridx != NO_COLOR && coloridx != CLR_MAX)
                        curses_toggle_color_attr(win, coloridx, NONE, ON);
#endif
                }
#endif /* STATUS_HILITES */

                waddstr(win, text);

#ifdef STATUS_HILITES
                if (iflags.hilite_delta) {
#ifdef TEXTCOLOR
                    if (coloridx != NO_COLOR)
                        curses_toggle_color_attr(win, coloridx, NONE, OFF);
#endif
                    if (attrmask)
                        wattroff(win, attrmask);
                }
#endif /* STATUS_HILITES */

            } else {
                /* status conditions */
                if (curses_condition_bits) {
                    getyx(win, y, x);
                    /* encumbrance is truncated if too wide, but other fields
                       aren't; if window is narrower than normal, last field
                       written might have wrapped to the next line */
                    if (y > j + (border ? 1 : 0))
                        x = width - (border ? -1 : 0), /* (width-=2 above) */
                        y = j + (border ? 1 : 0);
                    /* cbuf[] was populated above; clen is its length */
                    if (number_of_lines == 3) {
                        /*
                         * For 3-line status, align conditions with hunger
                         * (or where it would have been, when not shown),
                         * or if that doesn't provide enough room, right
                         * align with window's edge, or just put out at
                         * current spot if too long for right alignment.
                         */
                        /* both cbuf[] and hunger start with a leading
                           space, so clen and condstart reflect that;
                           'border' adjustments have already been made
                           for x (offset by 1) and width (reduced by 2) */
                        if (x + clen < width) {
                            if (x < condstart && condstart + clen < width)
                                wmove(win, y, condstart);
                            else
                                wmove(win, y, width + (border ? 1 : 0) - clen);
                        }
                    }
                    /* 'asis' was set up by first curs_stat_conds() call
                       above; True means that none of the conditions
                       need highlighting; but we won't use concatenated
                       condition string as-is if it will overflow; we
                       want curs_stat_conds() to write '+' in last column
                       if any conditions are all the way off the edge */
                    if (x + clen > width - (border ? 1 : 0))
                        asis = FALSE;

                    if (asis)
                        waddstr(win, cbuf);
                    else /* cond by cond if any cond specifies highlighting */
                        curs_stat_conds(0, 0, &x, &y,
                                        (char *) 0, (boolean *) 0);
                } /* curses_condition_bits */
            } /* hitpointbar vs regular field vs conditions */
        } /* i (fld) */
        wclrtoeol(win); /* [superfluous? draw_status() calls werase()] */
    } /* j (line) */
    return;
}

/* vertical layout, to left or right of map */
static void
draw_vertical(boolean border)
{
    /* for blank lines, the digit prefix is the order in which they get
       removed if we need to shrink to fit within height limit (very rare) */
    static const enum statusfields fieldorder[] = {
        BL_TITLE, /* might be overlaid by hitpoint bar */
        /* 4:blank */
        BL_HP, BL_HPMAX,
        BL_ENE, BL_ENEMAX,
        BL_AC,
        /* 3:blank */
        BL_LEVELDESC,
        BL_ALIGN,
        BL_XP, BL_EXP, BL_HD,
        BL_GOLD,
        /* 2:blank (but only if time or score or both enabled) */
        BL_TIME,
        BL_SCORE,
        /* 1:blank */
        BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH,
        /* 5:blank (if any of hunger, encumbrance, or conditions appear) */
        BL_HUNGER, BL_CAP, /* these two are shown on same line */
        BL_CONDITION, /* shown three per line so may take up to four lines */
        BL_FLUSH
    };
    static const enum statusfields shrinkorder[] = {
         BL_STR, BL_SCORE, BL_TIME, BL_LEVELDESC, BL_HP,
         BL_CONDITION, BL_CAP, BL_HUNGER
    };
    xchar spacing[MAXBLSTATS];
    int i, fld, cap_and_hunger, time_and_score, cond_count, per_line;
    char *text;
#ifdef STATUS_HILITES
    char *p = 0, savedch = '\0';
    int coloridx = NO_COLOR, attrmask = 0;
#endif /* STATUS_HILITES */
    int height_needed, height, width, x = 0, y = 0;
    WINDOW *win = curses_get_nhwin(STATUS_WIN);

    /* note: getmaxyx() is a macro which assigns values to height and width */
    getmaxyx(win, height, width);

    /* basic update formats fields for horizontal; derive vertical from them */
    if (vert_status_dirty)
        curs_vert_status_vals(width - (border ? 2 : 0));

    /*
     * Possible refinements:
     *  If "<name> the <rank-or-monster>" is too wide, split it across
     *  two lines; alternatively, use the old curses status code to
     *  truncate the two portions separately.  (<name> is already being
     *  truncated to 10 chars by the botl.c code, so we don't really
     *  need to do anything further unless we want to override that.)
     */

    cap_and_hunger = 0;
    if (*status_vals_long[BL_HUNGER])
        cap_and_hunger |= 1;
    if (*status_vals_long[BL_CAP])
        cap_and_hunger |= 2;
    time_and_score = 0;
    if (status_activefields[BL_TIME])
        time_and_score |= 1;
    if (status_activefields[BL_SCORE])
        time_and_score |= 2;
    cond_count = 0;
    if (curses_condition_bits) {
        for (i = 0; i < CONDITION_COUNT; ++i)
            if (curses_condition_bits & (1 << i))
                ++cond_count;
    }
    per_line = 2; /* will be changed to 3 if status becomes too tall */

    /* count how many lines we'll need; we normally space several groups of
       fields with blank lines but might need to compress some of those out */
    height_needed = border ? 2 : 0;
    for (i = 0; (fld = i) < SIZE(spacing); ++i) {
        switch ((enum statusfields) fld) {
        case BL_HPMAX:
        case BL_ENEMAX:
        case BL_EXP:
            spacing[fld] = 0; /* these will continue the previous line */
            break;
        case BL_HP:
        case BL_LEVELDESC:
        case BL_STR:
            spacing[fld] = 2; /* simple group separation (no conditionals) */
            break;
        case BL_TIME:
            /* time will be separated from gold unless it is inactive */
            spacing[fld] = (time_and_score & 1) ? 2 : 0;
            break;
        case BL_SCORE:
            /* unlike hunger+cap, score is shown on separate line from time;
               needs time+score separator if time is inactive */
            spacing[fld] = (time_and_score == 2) ? 2
                           : (time_and_score & 1) ? 1 : 0;
            break;
        case BL_HUNGER:
            /* separated from characteristics unless blank */
            spacing[fld] = (cap_and_hunger & 1) ? 2 : 0;
            break;
        case BL_CAP:
            /* on same line as hunger if both are non-blank,
               otherwise needs blank line if hunger is being omitted */
            spacing[fld] = (cap_and_hunger == 2) ? 2 : 0;
            break;
        case BL_CONDITION:
            /* need blank line if hunger and encumbrance are both omitted,
               otherwise just start on next line; if more than 3 conditions
               are present, this will consume multiple lines from height */
            spacing[fld] = cond_count ? (!cap_and_hunger ? 2 : 1) : 0;
            /* first line handled via '+= spacing[]' below */
            if (cond_count > per_line)
                height_needed += (cond_count - 1) / per_line;
            break;
        case BL_XP:
        case BL_HD:
        default:
            /* might be inactive, otherwise normal case of 'on next line' */
            spacing[fld] = status_activefields[fld] ? 1 : 0;
            break;
        }
        height_needed += spacing[fld];
    }
    if (height_needed > height) {
        /* if there are a lot of status conditions, compress them first */
        if (per_line == 2 && cond_count > per_line) {
             height_needed -= (cond_count - 1) / per_line;
             per_line = 3;
             height_needed += (cond_count - 1) / per_line;
        }
        if (height_needed > height) {
            for (i = 0; i < SIZE(shrinkorder); ++i) {
                fld = shrinkorder[i];
                if (spacing[fld] == 2) {
                    spacing[fld] = 1; /* suppress planned blank line */
                    if (--height_needed <= height)
                        break;
                }
            }
        }
#ifdef SCORE_ON_BOTL
        /* with all optional fields and every status condition (12 out
           of the 13 since two are mutually exclusive) active, we need
           21 non-blank lines; curses_create_main_windows() used to
           require 24 lines or more in order to enable vertical status,
           but that has been relaxed to 20 so height_needed might still
           be too high after suppressing all the blank lines */
        if (height_needed > height && status_activefields[BL_SCORE]) {
            height_needed -= spacing[BL_SCORE];
            spacing[BL_SCORE] = 0;
            time_and_score &= ~2;
            /* height_needed isn't used beyond here but we keep it accurate */
            nhUse(height_needed);
        }
#endif
    }

    if (border)
        x++, y++;
    for (i = 0; (fld = fieldorder[i]) != BL_FLUSH; ++i) {
        if (!status_activefields[fld])
            continue;
        if ((fld == BL_HUNGER && !(cap_and_hunger & 1))
            || (fld == BL_CAP && !(cap_and_hunger & 2))
            || (fld == BL_TIME && !(time_and_score & 1))
            || (fld == BL_SCORE && !(time_and_score & 2)))
            continue;

        if (spacing[fld]) {
            wmove(win, y++, x); /* move to next line */
            if (spacing[fld] == 2)
                 wmove(win, y++, x); /* skip a line */
        }

        if (fld == BL_TITLE && iflags.wc2_hitpointbar) {
            /* 4: left+right borders and open+close brackets; 2: brackets */
            curs_HPbar(status_vals_long[fld], width - (border ? 4 : 2));

        } else if (fld != BL_CONDITION) {
            /* regular field (including title if no hitpoint bar) */
            text = status_vals_long[fld];
            /* hunger and encumbrance come with a leading space;
               we'll put them on the same line and omit that space for
               the first (or only) and keep it for the second (if both) */
            if (*text == ' '
                && (fld == BL_HUNGER
                    || (fld == BL_CAP && cap_and_hunger != 3)))
                ++text;
#ifdef STATUS_HILITES
            coloridx = curses_status_colors[fld]; /* includes attributes */
            if (iflags.hilite_delta && coloridx != NO_COLOR) {
                /* most status_vals_long[] are "long-text : value" and
                   unlike horizontal status's abbreviated "ab:value",
                   we highlight just the value portion */
                p = (fld != BL_TITLE) ? index(text, ':') : 0;
                p = !p ? text : p + 1;
                while (*p == ' ')
                    ++p;
                if ((fld == BL_EXP && *p == '/')
                    || ((fld == BL_HPMAX || fld == BL_ENEMAX) && *p == '('))
                    ++p;
                /* prefix portion, if any, is output without highlighting */
                if (p > text) {
                    savedch = *p;
                    *p = '\0';
                    waddstr(win, text); /* output the prefix */
                    *p = savedch;
                    text = p; /* rest of field */
                    if ((fld == BL_HPMAX || fld == BL_ENEMAX)
                        && (p = index(text, ')')) != 0) {
                        savedch = *p;
                        *p = '\0';
                    } else
                        savedch = '\0';
                }
                attrmask = (coloridx >> 8) & 0x00FF;
                if (attrmask) {
                    attrmask = nhattr2curses(attrmask);
                    wattron(win, attrmask);
                }
#ifdef TEXTCOLOR
                coloridx &= 0x00FF;
                if (coloridx != NO_COLOR && coloridx != CLR_MAX)
                    curses_toggle_color_attr(win, coloridx, NONE, ON);
#endif
            } /* highlighting active */
#endif /* STATUS_HILITES */

            waddstr(win, text);

#ifdef STATUS_HILITES
            if (iflags.hilite_delta) {
#ifdef TEXTCOLOR
                if (coloridx != NO_COLOR)
                    curses_toggle_color_attr(win, coloridx, NONE, OFF);
#endif
                if (attrmask)
                    wattroff(win, attrmask);
            } /* resume normal rendition */
            if ((fld == BL_HPMAX || fld == BL_ENEMAX) && savedch == ')') {
                *p = savedch;
                waddstr(win, p);
            }
#endif /* STATUS_HILITES */

        } else {
            /* status conditions */
            if (cond_count) {
                /* output active conditions; usually two per line, but
                   window isn't tall enough, it's increased to three per line;
                   cursor is already positioned where they should start */
                curs_stat_conds(1, per_line, &x, &y,
                                (char *) 0, (boolean *) 0);
            }
        } /* hitpointbar vs regular field vs conditions */
    } /* fld loop */
    return;
}

/* hitpointbar using hp percent calculation */
static void
curs_HPbar(char *text, /* pre-padded with trailing spaces if short */
           int bar_len) /* width of space within the brackets */
{
#ifdef STATUS_HILITES
#ifdef TEXTCOLOR
    int coloridx = 0;
#endif
#endif /* STATUS_HILITES */
    int k, bar_pos;
    char bar[STATVAL_WIDTH], *bar2 = (char *) 0, savedch = '\0';
    boolean twoparts = (hpbar_percent < 100);
    WINDOW *win = curses_get_nhwin(STATUS_WIN);

    if (bar_len < 1 || bar_len > 30)
        bar_len = 30;
    if (bar_len > (k = (int) strlen(text))) /* 26 for vertical status */
        bar_len = k;
    (void) strncpy(bar, text, bar_len);
    bar[bar_len] = '\0';

    bar_pos = (bar_len * hpbar_percent) / 100;
    if (bar_pos < 1 && hpbar_percent > 0)
        bar_pos = 1;
    if (bar_pos >= bar_len && hpbar_percent < 100)
        bar_pos = bar_len - 1;
    if (twoparts) {
        bar2 = &bar[bar_pos];
        savedch = *bar2;
        *bar2 = '\0';
    }

    waddch(win, '[');
    if (*bar) { /* True unless dead (0 HP => bar_pos == 0) */
        /* fixed attribute, not nhattr2curses((hpbar_color >> 8) & 0x00FF) */
        wattron(win, A_REVERSE); /* do this even if hilite_delta is 0 */
#ifdef STATUS_HILITES
#ifdef TEXTCOLOR
        if (iflags.hilite_delta) {
            coloridx = hpbar_color & 0x00FF;
            if (coloridx != NO_COLOR)
                curses_toggle_color_attr(win, coloridx, NONE, ON);
        }
#endif
#endif /* STATUS_HILITES */

        /* portion of title corresponding to current hit points */
        waddstr(win, bar);

#ifdef STATUS_HILITES
#ifdef TEXTCOLOR
        if (iflags.hilite_delta) {
            if (coloridx != NO_COLOR)
                curses_toggle_color_attr(win, coloridx, NONE, OFF);
        }
#endif
#endif /* STATUS_HILITES */
        wattroff(win, A_REVERSE); /* do this even if hilite_delta is 0 */
    } /* *bar (current HP > 0) */

    if (twoparts) {
        /* unhighlighted portion, corresponding to current injuries */
        *bar2 = savedch;
        waddstr(win, bar2);
    }
    waddch(win, ']');
}

/* conditions[] is used primarily for parsing hilite_status rules, but
   we can use it for condition names and mask bits, avoiding duplication */
extern const struct conditions_t conditions[]; /* botl.c */
extern int cond_idx[CONDITION_COUNT];

DISABLE_WARNING_FORMAT_NONLITERAL

static void
curs_stat_conds(
    int vert_cond,     /* 0 => horizontal, 1 => vertical */
    int per_line,      /* for vertical number of conditions per line */
    int *x, int *y,    /* real for vertical, ignored otherwise */
    char *condbuf,     /* optional output; collect string of conds */
    boolean *nohilite) /* optional output; indicates whether -*/
{                      /*+ condbuf[] could be used as-is      */
    char condnam[20];
    int i, ci;
    long bitmsk;

    if (condbuf) {
        /* construct string " cond1 cond2 cond3" of all active conditions;
           if none of them specify highlighting, set the nohilite flag so
           caller can output the string as is; otherwise its length can be
           used to decide how to position prior to calling us back without
           'condbuf' in order to perform cond by cond output */
        condbuf[0] = '\0';
        if (nohilite)
            *nohilite = TRUE; /* assume ok */
        for (i = 0; i < CONDITION_COUNT; ++i) {
            ci = cond_idx[i];
            bitmsk = conditions[ci].mask;
            if (curses_condition_bits & bitmsk) {
                Strcpy(condnam, conditions[ci].text[0]);
                Strcat(strcat(condbuf, " "), upstart(condnam));
#ifdef STATUS_HILITES
                if (nohilite && *nohilite
                    && (condcolor(bitmsk, curses_colormasks) != NO_COLOR
                        || condattr(bitmsk, curses_colormasks) != 0))
                    *nohilite = FALSE;
#endif /* STATUS_HILITES */
            }
        }
    } else if (curses_condition_bits) {
        unsigned long cond_bits;
        const char *vert_fmt = 0;
        int height = 0, width, cx, cy, cy0, cndlen;
#ifdef STATUS_HILITES
        int attrmask = 0, color = NO_COLOR;
#endif /* STATUS_HILITES */
        boolean border, do_vert = (vert_cond != 0);
        WINDOW *win = curses_get_nhwin(STATUS_WIN);

        getmaxyx(win, height, width);
        border = curses_window_has_border(STATUS_WIN);
        cy0 = height - (border ? 2 : 1);
        if (vert_cond)
            vert_fmt = (per_line == 2) ? "%-12.12s" : "%-8.8s";

        cond_bits = curses_condition_bits;
        /* show active conditions directly; for vertical, 2 or 3 per line */
        for (i = 0; i < CONDITION_COUNT; ++i) {
            ci = cond_idx[i];
            bitmsk = conditions[ci].mask;
            if (cond_bits & bitmsk) {
                if (!vert_fmt)
                    Strcpy(condnam, conditions[ci].text[0]);
                else
                    Sprintf(condnam, vert_fmt, conditions[ci].text[0]);
                cndlen = 1 + (int) strlen(condnam); /* count leading space */
                if (!do_vert) {
                    getyx(win, cy, cx);
                    if (cy > cy0) /* wrap to next line shouldn't happen */
                        cx = width, cy = cy0;
                    if (cx + cndlen > width - (border ? 2 : 1)) {
                        /* not enough room for current condition */
                        if (cx + 1 > width - (border ? 2 : 1))
                            break; /* no room at all; skip it and the rest */
                        /* room for part; truncate it to avoid wrapping */
                        condnam[width - (border ? 2 : 1) - cx] = '\0';
                    }
                }
                cond_bits &= ~bitmsk; /* nonzero if another cond after this */
                /* output unhighlighted leading space unless at #1 of 3 */
                if (!do_vert || (vert_cond % per_line) != 1)
                    waddch(win, ' ');
#ifdef STATUS_HILITES
                if (iflags.hilite_delta) {
                    if ((attrmask = condattr(bitmsk, curses_colormasks))
                        != 0) {
                        attrmask = nhattr2curses(attrmask);
                        wattron(win, attrmask);
                    }
#ifdef TEXTCOLOR
                    if ((color = condcolor(bitmsk, curses_colormasks))
                        != NO_COLOR)
                        curses_toggle_color_attr(win, color, NONE, ON);
#endif
                }
#endif /* STATUS_HILITES */

                /* output the condition name */
                waddstr(win, upstart(condnam));

#ifdef STATUS_HILITES
                if (iflags.hilite_delta) {
#ifdef TEXTCOLOR
                    if (color != NO_COLOR)
                        curses_toggle_color_attr(win, color, NONE, OFF);
#endif
                    if (attrmask)
                        wattroff(win, attrmask);
                }
#endif /* STATUS_HILITES */
                /* if that was #3 of 3 advance to next line */
                if (do_vert && (++vert_cond % per_line) == 1)
                    wmove(win, (*y)++, *x);
            } /* if cond_bits & bitmask */
        } /* for i */

        /* if we stopped before showing all the conditions, put a '+'
           in the rightmost column to indicate that there would be more
           shown if there were more room available */
        if (cond_bits != 0L) {
            wmove(win, cy0, width - (border ? 2 : 1));
            waddch(win, '+');
        }
    }
    return;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* status_update() sets up values for horizontal status; do vertical */
void
curs_vert_status_vals(int win_width)
{
    const char *lbl; /* field name used as label */
    const char *text, *colon;
    char leadingspace[15]; /* 5 suffices */
    boolean use_name;
    int fldidx, hp_width, en_tmp, fld_width, lbl_width;

    /* width of bigger of full HP and full En regardless of current value */
    hp_width = (int) strlen(status_vals[BL_HPMAX]) - 2; /* -2: "("...")" */
    en_tmp = (int) strlen(status_vals[BL_ENEMAX]) - 2;
    if (en_tmp > hp_width)
        hp_width = en_tmp;
    /*
     * status_vals[] are used for horizontal orientation
     *  (wide lines of multiple short values).
     * status_vals_long[] are used for vertical orientation
     *  (narrow-ish lines of one long value each [mostly]).
     */
    lbl_width = 13;
 startover:
    for (fldidx = 0; fldidx < MAXBLSTATS; ++fldidx) {
        if (!status_activefields[fldidx] || fldidx == BL_CONDITION) {
            *status_vals_long[fldidx] = '\0';
        } else {
            text = status_vals[fldidx];
            if (fldidx != BL_TITLE && fldidx != BL_LEVELDESC) {
                if ((colon = index(text, ':')) != 0)
                    text = colon + 1;
            }
            lbl = status_fieldnm[fldidx];
            use_name = TRUE;
            leadingspace[0] = '\0';
            /* classify type of field (labeled or not) and make some fixups */
            switch ((enum statusfields) fldidx) {
            case BL_XP:
                 /* "experience-level : N" is too long and becomes misleading
                    if value is shown as 'N/experience-points' */
                lbl = "experience";
                break;
            case BL_LEVELDESC:
                /* "dungeon-level" is redundant when value is "Dlvl-N" */
                lbl = "location";
                break;
            case BL_HD:
                /* "HD" is too oscure; 0 actually means 1d4 (so about 1/2);
                   "hit-dice" is obscure too but doesn't stand out as such */
                lbl = (!strcmp(text, "1") || !strcmp(text, "0")) ? "hit-die"
                      : "hit-dice";
                break;
            case BL_ALIGN:
                /* don't want sprintf(": %s") below inserting second space */
                if (*text == ' ')
                    ++text;
                break;
            case BL_HP:
            case BL_ENE:
                /* pad HP and En so that they're right aligned */
                fld_width = (int) strlen(text);
                if (fld_width < hp_width)
                    Sprintf(leadingspace, "%*s", hp_width - fld_width, " ");
                break;
            case BL_STR:
            case BL_DX:
            case BL_CO:
            case BL_IN:
            case BL_WI:
            case BL_CH:
                /* for vertical orientation, right justify characteristics;
                   exceptional strength, if present, will protrude to right */
                if (strlen(text) == 1)
                     Strcpy(leadingspace, " ");
                break;
            case BL_HPMAX:
            case BL_ENEMAX:
                 /* pad HPmax and Enmax so that they're right aligned with
                    each other with a one-space gap after current HP/En */
                fld_width = (int) strlen(text);
                if (fld_width < hp_width + 3) /* +3: " " gap and "("...")" */
                    Sprintf(leadingspace, "%*s",
                            (hp_width + 3) - fld_width, " ");
                /*FALLTHRU*/
            case BL_EXP:
            case BL_HUNGER:
            case BL_CAP:
            case BL_TITLE:
                use_name = FALSE;
                break;
            default:
                break;
            }
            if (use_name) {
                Sprintf(status_vals_long[fldidx], "%*.*s: %s%s",
                        -lbl_width, lbl_width, lbl, leadingspace, text);
                *status_vals_long[fldidx] = highc(*status_vals_long[fldidx]);
            } else if ((fldidx == BL_HUNGER || fldidx == BL_CAP) && *text) {
                /* hunger and enbumbrance are shown side-by-side in
                   a 26 character or wider window; if leading space is
                   present, get rid of it, then add one we're sure about */
                if (*text == ' ')
                    ++text;
                Sprintf(status_vals_long[fldidx], " %-12.12s", text);
            } else {
                /* unlabeled: title, hp-max, en-max, exp-points, hunger+cap */
                Sprintf(status_vals_long[fldidx], "%s%s", leadingspace, text);
                /* when appending, base field uses field name as label */
                use_name = (fldidx == BL_HPMAX || fldidx == BL_ENEMAX
                            || fldidx == BL_EXP);
            }
            /* check whether 'label : value' is too wide; if so, we'll
               shorten the label's allowed width and try again */
            if (use_name) {
                fld_width = (int) strlen(status_vals_long[fldidx]);
                /* each extension field is preceded by its base field in
                   order to append, so base's _vals_long[] has been set */
                switch ((enum statusfields) fldidx) {
                case BL_HPMAX:
                    fld_width += (int) strlen(status_vals_long[BL_HP]);
                    break;
                case BL_ENEMAX:
                    fld_width += (int) strlen(status_vals_long[BL_ENE]);
                    break;
                case BL_EXP:
                    fld_width += (int) strlen(status_vals_long[BL_XP]);
                    break;
                default:
                    break;
                }
                if (fld_width > win_width && lbl_width > 10) {
                    lbl_width -= (fld_width - win_width);
                    if (lbl_width < 10)
                        lbl_width = 10;
                    goto startover;
                }
            }
        }
    }
    vert_status_dirty = 0;
}

#ifdef STATUS_HILITES
#ifdef TEXTCOLOR
/*
 * Return what color this condition should
 * be displayed in based on user settings.
 */
static int
condcolor(long bm, unsigned long *bmarray)
{
    int i;

    if (bm && bmarray)
        for (i = 0; i < CLR_MAX; ++i) {
            if ((bmarray[i] & bm) != 0)
                return i;
        }
    return NO_COLOR;
}
#endif /* TEXTCOLOR */

static int
condattr(long bm, unsigned long *bmarray)
{
    int i, attr = 0;

    if (bm && bmarray) {
        for (i = HL_ATTCLR_DIM; i < BL_ATTCLR_MAX; ++i) {
            if ((bmarray[i] & bm) != 0) {
                switch (i) {
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
                default:
                    break;
                }
            }
        }
    }
    return attr;
}
/* convert tty attributes to curses attributes;
   despite similar names, the mask fields have different values */
static int
nhattr2curses(int attrmask)
{
    int result = 0;

    if (attrmask & HL_BOLD)
        result |= A_BOLD;
    if (attrmask & HL_INVERSE)
        result |= A_REVERSE;
    if (attrmask & HL_ULINE)
        result |= A_UNDERLINE;
    if (attrmask & HL_BLINK)
        result |= A_BLINK;
    if (attrmask & HL_DIM)
        result |= A_DIM;

    return result;
}
#endif /* STATUS_HILITES */

/* ======================================================================== */


#if 0 /* old stuff; some may be re-incorporated, most should be discarded */
/* Private declarations */

/* Used to track previous value of things, to highlight changes. */
typedef struct nhs {
    long value;
    int highlight_turns;
    int highlight_color;
} nhstat;

static attr_t get_trouble_color(const char *);
static void draw_trouble_str(const char *);
static void print_statdiff(const char *append, nhstat *, long, int);
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

        Strcpy(buf, pmname(&mons[u.umonnum], flags.female ? FEMALE : MALE));
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
print_statdiff(const char *append, nhstat *stat, long new, int type)
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
            if (COLORS <= 16)
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
    orient statorient = (orient) curses_get_window_orientation(STATUS_WIN);
    boolean horiz = (statorient != ALIGN_RIGHT && statorient != ALIGN_LEFT);
    boolean border = curses_window_has_border(STATUS_WIN);

    /* Clear the window */
    werase(win);

    /* Figure out if we have proper window dimensions for horizontal status */
    if (horiz) {
        /* correct y */
        int cy = 3;
        if (iflags.statuslines < 3)
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

            /* Reset XP highlight (since classic_status and new show
               different numbers) */
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

    if (horiz)
        draw_horizontal(x, y, hp, hpmax);
    else
        draw_vertical(x, y, hp, hpmax);

    if (border)
        box(win, 0, 0);

    wnoutrefresh(win);

    if (first) {
        first = FALSE;

        /* Zero highlight timers. This will call curses_update_status again
           if needed */
        curses_decrement_highlights(TRUE);
    }
}

static void
draw_horizontal(int x, int y, int hp, int hpmax)
{
    if (iflags.statuslines >= 3) {
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
    sprintf(buf, "%s the %s", g.plname, rank);

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

    print_statdiff("$", &prevau, money_cnt(g.invent), STAT_GOLD);

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
    else if (flags.showexp) {
        print_statdiff(" Xp:", &prevlevel, u.ulevel, STAT_OTHER);
        /* use waddch, we don't want to highlight the '/' */
        waddch(win, '/');
        print_statdiff("", &prevexp, u.uexp, STAT_OTHER);
    }
    else
        print_statdiff(" Exp:", &prevlevel, u.ulevel, STAT_OTHER);

    if (flags.time)
        print_statdiff(" T:", &prevtime, g.moves, STAT_TIME);

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
    Strcpy(race, g.urace.adj);
    race[0] = highc(race[0]);
    wprintw(win, "%s the %s %s%s%s", g.plname,
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

    print_statdiff(" $", &prevau, money_cnt(g.invent), STAT_GOLD);

#ifdef SCORE_ON_BOTL
    if (flags.showscore)
        print_statdiff(" S:", &prevscore, botl_score(), STAT_OTHER);
#endif /* SCORE_ON_BOTL */

    if (flags.time)
        print_statdiff(" T:", &prevtime, g.moves, STAT_TIME);

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
    int namelen = strlen(g.plname);
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
    sprintf(buf, "%-*s the %-*s", namelen, g.plname, ranklen, rank);
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

    print_statdiff("Gold:          ", &prevau, money_cnt(g.invent), STAT_GOLD);
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
    else if (flags.showexp) {
        print_statdiff("Experience:    ", &prevlevel, u.ulevel, STAT_OTHER);
        /* use waddch, we don't want to highlight the '/' */
        waddch(win, '/');
        print_statdiff("", &prevexp, u.uexp, STAT_OTHER);
    }
    else
        print_statdiff("Level:         ", &prevlevel, u.ulevel, STAT_OTHER);
    wmove(win, y++, x);

    if (flags.time) {
        print_statdiff("Time:          ", &prevtime, g.moves, STAT_TIME);
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
    unhighlight |= decrement_highlight(&prevexp, zero);
    unhighlight |= decrement_highlight(&prevtime, zero);
#ifdef SCORE_ON_BOTL
    unhighlight |= decrement_highlight(&prevscore, zero);
#endif

    if (unhighlight)
        curses_update_stats();
}
#endif /*0*/

/*cursstat.c*/

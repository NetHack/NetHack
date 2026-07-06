/* NetHack 3.6	winstr.c	$NHDT-Date: 1432512795 2015/05/25 00:13:15 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Gregg Wonderly, Naperville, Illinois,  1991,1992,1993. */
/* NetHack may be freely redistributed.  See license for details. */

#include "windefs.h"
#include "winext.h"
#include "winproto.h"
/* Put a string into the indicated window using the indicated attribute */

void
amii_putstr(winid window, int attr, const char *str)
{
    int fudge;
    int len;
    struct Window *w;
    register struct amii_WinDesc *cw;
    char *ob;
    int i, j, n0, bottom, totalvis, wheight;
    static int wrapping = 0;


    /* Always try to avoid a panic when there is no window */
    if (window == WIN_ERR) {
        window = WIN_BASE;
        if (window == WIN_ERR)
            window = WIN_BASE = amii_create_nhwindow(NHW_BASE);
    }

    if (window == WIN_ERR || (cw = amii_wins[window]) == NULL) {
        iflags.window_inited = 0;
        panic(winpanicstr, window, "putstr");
    }

    w = cw->win;

    if (!str)
        return;
    amiIDisplay->lastwin = window; /* do we care??? */

    /* NHW_MENU windows are not opened immediately, so check if we
     * have the window pointer yet
     */

    if (w) {
        /* Set the drawing mode and pen colors */
        SetDrMd(w->RPort, JAM2);
        amii_sethipens(w, cw->type, attr);
    } else if (cw->type != NHW_MENU && cw->type != NHW_TEXT) {
        panic("NULL window pointer in putstr 2: %d", window);
    }

    /* Okay now do the work for each type */

    switch (cw->type) {
    case NHW_MESSAGE:
        if (WINVERS_AMIV)
            fudge = 2;
        else {
            /* 8 for --more--, 1 for preceeding sp, 1 for putstr pad */
            fudge = 10;
        }

        /* There is a one pixel border at the borders, so subtract two */
        bottom = amii_msgborder(w);

        wheight = (w->Height - w->BorderTop - w->BorderBottom - 3)
                  / w->RPort->TxHeight;

        if (scrollmsg || wheight > 1)
            fudge = 0;

        amii_scrollmsg(w, cw);

        while (isspace(*str))
            str++;
        strncpy(gt.toplines, str, TBUFSZ);
        gt.toplines[TBUFSZ - 1] = 0;

        /* For initial message to be visible, we need to explicitly position
         * the
         * cursor.  This flag, cw->curx == -1 is set elsewhere to force the
         * cursor to be repositioned to the "bottom".
         */
        if (cw->curx == -1) {
            amii_curs(WIN_MESSAGE, 1, bottom);
            cw->curx = 0;
        }

        /* If used all of history lines, move them down */
        if (cw->maxrow >= iflags.msg_history) {
            if (cw->data[0])
                free(cw->data[0]);
            memcpy(cw->data, &cw->data[1],
                   (iflags.msg_history - 1) * sizeof(char *));
            cw->data[iflags.msg_history - 1] =
                (char *) alloc(strlen(gt.toplines) + SOFF + 4);
            strcpy(cw->data[i = iflags.msg_history - 1] + SOFF
                       + (scrollmsg != 0),
                   gt.toplines);
        } else {
            /* Otherwise, allocate a new one and copy the line in */
            cw->data[cw->maxrow] = (char *) alloc(strlen(gt.toplines) + SOFF + 4);
            strcpy(cw->data[i = cw->maxrow++] + SOFF + (scrollmsg != 0),
                   gt.toplines);
        }
        cw->data[i][SEL_ITEM] = 1;
        cw->data[i][VATTR] = attr + 1;

        if (scrollmsg) {
            cw->curx = 0;
            cw->data[i][2] = (cw->wflags & FLMSG_FIRST) ? '>' : ' ';
        }

        str = cw->data[i] + SOFF;
        if (cw->curx + strlen(str) >= (cw->cols - fudge)) {
            int i;
            char *ostr = (char *) str;
            char *p;

            while (cw->curx + strlen(str) >= (cw->cols - fudge)) {
                for (p = ((char *) &str[cw->cols - 1 - cw->curx]) - fudge;
                     !isspace(*p) && p > str;)
                    --p;
                if (p < str)
                    p = (char *) str;

                if (p == str) {
                    /* No whitespace within visible width. */
                    if (cw->curx > 0) {
                        /* Mid-line: clear it and retry at column 0. */
                        outmore(cw);
                        continue;
                    }
                    /* Already at line start: word is longer than one
                     * line, so force-break it at the column boundary. */
                    i = cw->cols - 1 - fudge;
                    if (i <= 0)
                        i = 1;
                    if ((size_t) i > strlen(str))
                        i = strlen(str);
                    outsubstr(cw, (char *) str, i, fudge);
                    cw->curx += i;
                    str += i;
                    if (*str)
                        amii_scrollmsg(w, cw);
                    amii_cl_end(cw, cw->curx);
                    continue;
                }

                i = (long) p - (long) str;
                outsubstr(cw, (char *) str, i, fudge);
                cw->curx += i;

                while (isspace(*p))
                    p++;
                str = p;

                if (*str)
                    amii_scrollmsg(w, cw);
                amii_cl_end(cw, cw->curx);
            }

            if (*str) {
                if (str != ostr) {
                    outsubstr(cw, "+", 1, fudge);
                    cw->curx += 2;
                }
                while (isspace(*str))
                    ++str;
                outsubstr(cw, (char *) str, i = strlen((char *) str), fudge);
                cw->curx += i;
                amii_cl_end(cw, cw->curx);
            }
        } else {
            outsubstr(cw, (char *) str, i = strlen((char *) str), fudge);
            cw->curx += i;
            amii_cl_end(cw, cw->curx);
        }
        cw->wflags &= ~FLMSG_FIRST;
        len = 0;
        if (scrollmsg) {
            totalvis = CountLines(window);
            SetPropInfo(w, &MsgScroll,
                        (w->Height - w->BorderTop - w->BorderBottom)
                            / w->RPort->TxHeight,
                        totalvis, totalvis);
        }
        i = strlen(gt.toplines + SOFF);
        cw->maxcol = max(cw->maxcol, i);
        cw->vwy = cw->maxrow;
        break;

    case NHW_STATUS:
        if (cw->data[cw->cury] == NULL)
            panic("NULL pointer for status window");
        ob = &cw->data[cw->cury][j = cw->curx];
        if (disp.botlx)
            *ob = 0;

        /* Display when beam at top to avoid flicker... */
        WaitTOF();
        {   int slen = strlen((char *) str);
            if (slen > cw->cols) slen = cw->cols;
            Text(w->RPort, (char *) str, slen);
        }
        if (cw->cols > strlen(str))
            TextSpaces(w->RPort, cw->cols - strlen(str));

        (void) strncpy(cw->data[cw->cury], str, cw->cols);
        cw->data[cw->cury][cw->cols - 1] = '\0'; /* null terminate */
        cw->cury = (cw->cury + 1) % 2;
        cw->curx = 0;
        break;

    case NHW_MAP:
    case NHW_BASE:
        if (cw->type == NHW_BASE && wrapping) {
            amii_curs(window, cw->curx + 1, cw->cury);
            TextSpaces(w->RPort, cw->cols);
            if (cw->cury < cw->rows) {
                amii_curs(window, cw->curx + 1, cw->cury + 1);
                TextSpaces(w->RPort, cw->cols);
                cw->cury--;
            }
            wrapping = 0;
        }
        amii_curs(window, cw->curx + 1, cw->cury);
        Text(w->RPort, (char *) str, strlen((char *) str));
        cw->curx = 0;
        /* CR-LF is automatic in these windows */
        cw->cury++;
        if (cw->type == NHW_BASE && cw->cury >= cw->rows) {
            cw->cury = 0;
            wrapping = 1;
        }
        break;

    case NHW_MENU:
    case NHW_TEXT:

        /* always grows one at a time, but alloc 12 at a time */

        if (cw->cury >= cw->rows || !cw->data) {
            char **tmp;

            /* Allocate 12 more rows */
            cw->rows += 12;
            tmp = (char **) alloc(sizeof(char *) * cw->rows);

            /* Copy the old lines */
            for (i = 0; i < cw->cury; i++)
                tmp[i] = cw->data[i];

            if (cw->data) {
                free(cw->data);
                cw->data = NULL;
            }

            cw->data = tmp;

            /* Null out the unused entries. */
            for (i = cw->cury; i < cw->rows; i++)
                cw->data[i] = 0;
        }

        if (!cw->data)
            panic("no data storage");

        /* Shouldn't need to do this, but... */

        if (cw->data && cw->data[cw->cury]) {
            free(cw->data[cw->cury]);
            cw->data[cw->cury] = NULL;
        }

        n0 = strlen(str) + 1;
        cw->data[cw->cury] = (char *) alloc(n0 + SOFF);

        /* avoid nuls, for convenience */
        cw->data[cw->cury][VATTR] = attr + 1;
        cw->data[cw->cury][SEL_ITEM] = 0;
        Strcpy(cw->data[cw->cury] + SOFF, str);

        if (n0 > cw->maxcol)
            cw->maxcol = n0;
        if (++cw->cury > cw->maxrow)
            cw->maxrow = cw->cury;
        break;

    default:
        panic("Invalid or unset window type in putstr()");
    }
}

void
amii_scrollmsg(struct Window *w, struct amii_WinDesc *cw)
{
    int bottom, wheight;

    bottom = amii_msgborder(w);

    wheight =
        (w->Height - w->BorderTop - w->BorderBottom - 3) / w->RPort->TxHeight;

    if (scrollmsg) {
        if (++cw->disprows > wheight) {
            outmore(cw);
            cw->disprows = 1; /* count this line... */
        } else {
            ScrollRaster(w->RPort, 0, w->RPort->TxHeight, w->BorderLeft,
                         w->BorderTop + 1, w->Width - w->BorderRight - 1,
                         w->Height - w->BorderBottom - 1);
        }
        amii_curs(WIN_MESSAGE, 1, bottom);
    }
}

int
amii_msgborder(struct Window *w)
{
    register int bottom;

    /* There is a one pixel border at the borders, so subtract two */
    bottom = w->Height - w->BorderTop - w->BorderBottom - 2;
    bottom /= w->RPort->TxHeight;
    if (bottom > 0)
        --bottom;
    return (bottom);
}

void
outmore(struct amii_WinDesc *cw)
{
    struct Window *w = cw->win;

    if ((cw->wflags & FLMAP_SKIP) == 0) {
        if (scrollmsg) {
            int bottom;

            bottom = amii_msgborder(w);

            ScrollRaster(w->RPort, 0, w->RPort->TxHeight, w->BorderLeft,
                         w->BorderTop + 1, w->Width - w->BorderRight - 1,
                         w->Height - w->BorderBottom - 1);
            amii_curs(WIN_MESSAGE, 1, bottom); /* -1 for inner border */
            Text(w->RPort, "--more--", 8);
        } else
            Text(w->RPort, " --more--", 9);

        /* Make sure there are no events in the queue */
        flushIDCMP(HackPort);

        /* Allow mouse clicks to clear --more-- */
        WindowGetchar();
        if (lastevent.type == WEKEY && lastevent.un.key == '\33')
            cw->wflags |= FLMAP_SKIP;
    }
    if (!scrollmsg) {
        amii_curs(WIN_MESSAGE, 1, 0);
        amii_cl_end(cw, cw->curx);
    }
}

void
outsubstr(struct amii_WinDesc *cw, char *str, int len, int fudge)
{
    struct Window *w = cw->win;

    if (cw->curx) {
        /* Check if this string and --more-- fit, if not,
         * then put out --more-- and wait for a key.
         */
        if ((len + fudge) + cw->curx >= cw->cols) {
            if (!scrollmsg)
                outmore(cw);
        } else {
            /* Otherwise, move and put out a blank separator */
            Text(w->RPort, spaces, 1);
            cw->curx += 1;
        }
    }

    Text(w->RPort, str, len);
}

/* Put a graphics character onto the screen */

void
amii_putsym(winid st, int i, int y, CHAR_P c)
{
    amii_curs(st, i, y);
    Text(amii_wins[st]->win->RPort, &c, 1);
}

/* Add to the last line in the message window */

void
amii_addtopl(const char *s)
{
    register struct amii_WinDesc *cw = amii_wins[WIN_MESSAGE];

    while (*s) {
        if (cw->curx == cw->cols - 1)
            amii_putstr(WIN_MESSAGE, 0, "");
        amii_putsym(WIN_MESSAGE, cw->curx + 1, amii_msgborder(cw->win), *s++);
        cw->curx++;
    }
}

void
TextSpaces(struct RastPort *rp, int nr)
{
    if (nr < 1)
        return;

    while (nr > sizeof(spaces) - 1) {
        Text(rp, spaces, (long) sizeof(spaces) - 1);
        nr -= sizeof(spaces) - 1;
    }
    if (nr > 0)
        Text(rp, spaces, (long) nr);
}

void
amii_remember_topl(void)
{
    /* ignore for now.  I think this will be done automatically by
     * the code writing to the message window, but I could be wrong.
     */
}

int
amii_doprev_message(void)
{
    struct amii_WinDesc *cw;
    struct Window *w;
    char *str;


    if (WIN_MESSAGE == WIN_ERR || (cw = amii_wins[WIN_MESSAGE]) == NULL
        || (w = cw->win) == NULL) {
        panic(winpanicstr, WIN_MESSAGE, "doprev_message");
    }

    /* When an interlaced/tall screen is in use, the scroll bar will be there
     */
    /* Or in some other cases as well */
    if (scrollmsg) {
        struct Gadget *gd;
        struct PropInfo *pip;
        int hidden, topidx, i, total, wheight;

        for (gd = w->FirstGadget; gd && gd->GadgetID != 1;)
            gd = gd->NextGadget;

        if (gd) {
            pip = (struct PropInfo *) gd->SpecialInfo;
            wheight = (w->Height - w->BorderTop - w->BorderBottom - 2)
                      / w->RPort->TxHeight;
            hidden = max(cw->maxrow - wheight, 0);
            topidx = (((ULONG) hidden * pip->VertPot) + (MAXPOT / 2)) >> 16;
            for (total = i = 0; i < cw->maxrow; ++i) {
                if (cw->data[i][1] != 0)
                    ++total;
            }

            i = 0;
            topidx -= wheight / 4 + 1;
            if (topidx < 0)
                topidx = 0;
            SetPropInfo(w, &MsgScroll, wheight, total, topidx);
            DisplayData(WIN_MESSAGE, topidx);
        }
        return (0);
    }

    if (--cw->vwy < 0) {
        cw->maxcol = 0;
        DisplayBeep(NULL);
        str = "\0\0No more history saved...";
    } else
        str = cw->data[cw->vwy];

    amii_cl_end(cw, 0);
    amii_curs(WIN_MESSAGE, 1, 0);
    amii_setdrawpens(amii_wins[WIN_MESSAGE]->win, NHW_MESSAGE);
    Text(w->RPort, str + SOFF, strlen(str + SOFF));
    cw->curx = cw->cols + 1;

    return (0);
}

/* Native status renderer: per-field color/attrs from the core's
 * STATUS_HILITES rules, modeled on sys/mac68k/macstat.c.
 */

extern const char *status_fieldfmt[MAXBLSTATS];
extern const char *status_fieldnm[MAXBLSTATS];
extern char *status_vals[MAXBLSTATS];
extern boolean status_activefields[MAXBLSTATS];
extern int foreg[AMII_MAXCOLORS], backg[AMII_MAXCOLORS];

static int stat_inited = 0;
static int stat_colors[MAXBLSTATS];
static unsigned long stat_cond_bits = 0UL;
static unsigned long *stat_colormasks = (unsigned long *) 0;
static int stat_hpbar_percent = 0, stat_hpbar_crit = 0;

static void amii_status_redraw(void);
static void stat_draw_str(struct RastPort *, const char *, int);
static void stat_draw_hpbar(struct RastPort *);
static void stat_draw_conds(struct RastPort *);
static int stat_condcolor(unsigned long, unsigned long *);
static int stat_condattr(unsigned long, unsigned long *);

/* same two-line layout as genl_status_update's default fieldorder */
static const enum statusfields stat_fieldorder[2][15] = {
    { BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_ALIGN,
      BL_SCORE, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH,
      BL_FLUSH },
    { BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX, BL_AC,
      BL_XP, BL_EXP, BL_HD, BL_TIME, BL_HUNGER, BL_CAP, BL_CONDITION,
      BL_FLUSH },
};

/* NetHack CLR_* to AMIV pen (amiv_init_map order in winami.c) */
static const int amiv_color_pen[CLR_MAX] = {
    C_BLACK,    /* CLR_BLACK */
    7,          /* CLR_RED */
    5,          /* CLR_GREEN */
    11,         /* CLR_BROWN */
    4,          /* CLR_BLUE */
    10,         /* CLR_MAGENTA */
    2,          /* CLR_CYAN */
    6,          /* CLR_GRAY */
    1,          /* NO_COLOR */
    3,          /* CLR_ORANGE */
    8,          /* CLR_BRIGHT_GREEN */
    9,          /* CLR_YELLOW */
    4,          /* CLR_BRIGHT_BLUE */
    10,         /* CLR_BRIGHT_MAGENTA */
    2,          /* CLR_BRIGHT_CYAN */
    1,          /* CLR_WHITE */
};

/* map a NetHack color to fg/bg pens; leaves *fgp/*bgp (the caller's
   defaults) alone for NO_COLOR or out-of-range */
void
amii_pens_for_color(int color, int *fgp, int *bgp)
{
    if (color < 0 || color >= CLR_MAX || color == NO_COLOR)
        return;
    if (WINVERS_AMIV) {
        *fgp = amiv_color_pen[color];
    } else {
        /* 16 colors into 8 pens via the map display's fg/bg trick */
        *fgp = foreg[color];
        *bgp = backg[color];
    }
}

void
amii_status_init(void)
{
    int i;

    for (i = 0; i < MAXBLSTATS; ++i)
        stat_colors[i] = NO_COLOR;
    stat_cond_bits = 0UL;
    stat_colormasks = (unsigned long *) 0;
    genl_status_init();
    stat_inited = 1;
}

void
amii_status_finish(void)
{
    stat_inited = 0;
    genl_status_finish();
}

void
amii_status_enablefield(
    int fieldidx, const char *nm, const char *fmt, boolean enable)
{
    genl_status_enablefield(fieldidx, nm, fmt, enable);
}

void
amii_status_update(
    int idx, genericptr_t ptr, int chg UNUSED, int percent,
    int color, unsigned long *colormasks)
{
    char *text = (char *) ptr;

    if (idx == BL_FLUSH || idx == BL_RESET) {
        amii_status_redraw();
        return;
    }
    if (idx < 0 || idx >= MAXBLSTATS || !status_activefields[idx]
        || !status_vals[idx])
        return;
    stat_colors[idx] = color;
    if (idx == BL_HP && iflags.wc2_hitpointbar) {
        stat_hpbar_percent = percent;
        stat_hpbar_crit = critically_low_hp(TRUE) ? 1 : 0;
        stat_colors[BL_TITLE] = (color & 0x00ff)
                                | ((HL_INVERSE
                                    | (stat_hpbar_crit ? HL_BLINK : 0))
                                   << 8);
    }
    if (idx == BL_CONDITION) {
        stat_cond_bits = ptr ? *(unsigned long *) ptr : 0UL;
        stat_colormasks = colormasks;
    } else if (idx == BL_GOLD) {
        /* gold arrives glyph-encoded; decode to the symset's symbol */
        status_vals[BL_GOLD][0] = ' ';
        (void) decode_mixed(&status_vals[BL_GOLD][1], text ? text : "");
    } else {
        Snprintf(status_vals[idx], MAXCO,
                 status_fieldfmt[idx] ? status_fieldfmt[idx] : "%s",
                 text ? text : "");
    }
}

static void
amii_status_redraw(void)
{
    struct amii_WinDesc *cw;
    struct Window *w;
    struct RastPort *rp;
    int line, i, right;

    if (!stat_inited || WIN_STATUS == WIN_ERR
        || (cw = amii_wins[WIN_STATUS]) == NULL || (w = cw->win) == NULL)
        return;

    rp = w->RPort;
    right = w->Width - w->BorderRight;
    WaitTOF();
    SetDrMd(rp, JAM2);
    for (line = 0; line < 2; line++) {
        Move(rp, w->BorderLeft + 2,
             (line * (rp->TxHeight + 1)) + w->BorderTop + rp->TxBaseline
                 + 1);
        for (i = 0; stat_fieldorder[line][i] != BL_FLUSH; i++) {
            enum statusfields f = stat_fieldorder[line][i];

            if (!status_activefields[f])
                continue;
            if (f == BL_CONDITION)
                stat_draw_conds(rp);
            else if (f == BL_TITLE && iflags.wc2_hitpointbar)
                stat_draw_hpbar(rp);
            else if (status_vals[f] && *status_vals[f])
                stat_draw_str(rp, status_vals[f], stat_colors[f]);
        }
        SetAPen(rp, amii_statAPen);
        SetBPen(rp, amii_statBPen);
        if (rp->cp_x < right)
            TextSpaces(rp, (right - rp->cp_x) / rp->TxWidth + 1);
    }
}

/* "[title]" with the leading hpbar_percent portion of title drawn in
   BL_TITLE's packed color/HL_INVERSE, per the tty hitpointbar layout */
static void
stat_draw_hpbar(struct RastPort *rp)
{
    char bar[30 + 1], *bar2 = (char *) 0, savedch = '\0';
    int bar_pos, bar_len;

    Sprintf(bar, "%-30.30s",
            status_vals[BL_TITLE] ? status_vals[BL_TITLE] : "");
    if (stat_hpbar_crit)
        repad_with_dashes(bar);
    bar_len = (int) strlen(bar);
    if (stat_hpbar_percent < 100) {
        bar_pos = (bar_len * stat_hpbar_percent) / 100;
        if (bar_pos < 1 && stat_hpbar_percent > 0)
            bar_pos = 1;
        if (bar_pos >= bar_len)
            bar_pos = bar_len - 1;
        bar2 = &bar[bar_pos];
        savedch = *bar2;
        *bar2 = '\0';
    }
    stat_draw_str(rp, "[", NO_COLOR);
    if (*bar)
        stat_draw_str(rp, bar, stat_colors[BL_TITLE]);
    if (bar2) {
        *bar2 = savedch;
        stat_draw_str(rp, bar2, NO_COLOR);
    }
    stat_draw_str(rp, "]", NO_COLOR);
}

/* draw one field at the pen, styled from the packed CLR_|(HL_<<8) */
static void
stat_draw_str(struct RastPort *rp, const char *str, int packed)
{
    int color = packed & 0x00ff;
    int attr = (packed >> 8) & 0x00ff;
    int fg = amii_statAPen, bg = amii_statBPen, t;
    ULONG style = FS_NORMAL;

    amii_pens_for_color(color, &fg, &bg);
    if (attr & HL_INVERSE) {
        t = fg;
        fg = bg;
        bg = t;
    }
    if (attr & HL_BOLD)
        style |= FSF_BOLD;
    if (attr & HL_ULINE)
        style |= FSF_UNDERLINED;
    if (attr & HL_ITALIC)
        style |= FSF_ITALIC;

    SetAPen(rp, fg);
    SetBPen(rp, bg);
    if (style != FS_NORMAL)
        SetSoftStyle(rp, style, AskSoftStyle(rp));
    Text(rp, (char *) str, strlen(str));
    if (style != FS_NORMAL)
        SetSoftStyle(rp, FS_NORMAL, AskSoftStyle(rp));
}

static void
stat_draw_conds(struct RastPort *rp)
{
    int i, k, color, attr, packed;
    char buf[32];

    for (k = 0; k < CONDITION_COUNT; k++) {
        i = cond_idx[k];
        if (!(stat_cond_bits & (unsigned long) conditions[i].mask))
            continue;
        color = stat_condcolor((unsigned long) conditions[i].mask,
                               stat_colormasks);
        attr = stat_condattr((unsigned long) conditions[i].mask,
                             stat_colormasks);
        packed = (color & 0x00ff) | (attr << 8);
        Sprintf(buf, " %s", conditions[i].text[0]);
        stat_draw_str(rp, buf, packed);
    }
}

static int
stat_condcolor(unsigned long bm, unsigned long *bmarray)
{
    int i;

    if (bm && bmarray)
        for (i = 0; i < CLR_MAX; ++i)
            if ((bmarray[i] & bm) != 0)
                return i;
    return NO_COLOR;
}

static int
stat_condattr(unsigned long bm, unsigned long *bmarray)
{
    int i, attr = 0;

    if (bm && bmarray)
        for (i = HL_ATTCLR_BOLD; i < BL_ATTCLR_MAX; ++i)
            if ((bmarray[i] & bm) != 0)
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

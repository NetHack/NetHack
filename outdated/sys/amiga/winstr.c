/* NetHack 3.6	winstr.c	$NHDT-Date: 1432512795 2015/05/25 00:13:15 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Gregg Wonderly, Naperville, Illinois,  1991,1992,1993. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef CROSS_TO_AMIGA
#include "NH:sys/amiga/windefs.h"
#include "NH:sys/amiga/winext.h"
#include "NH:sys/amiga/winproto.h"
#else
#include "windefs.h"
#include "winext.h"
#include "winproto.h"
#endif
/* Put a string into the indicated window using the indicated attribute */

void
amii_putstr(window, attr, str)
winid window;
int attr;
const char *str;
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
                (char *) alloc(strlen(gt.toplines) + 5);
            strcpy(cw->data[i = iflags.msg_history - 1] + SOFF
                       + (scrollmsg != 0),
                   gt.toplines);
        } else {
            /* Otherwise, allocate a new one and copy the line in */
            cw->data[cw->maxrow] = (char *) alloc(strlen(gt.toplines) + 5);
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
                    /*    p = (char *)&str[ cw->cols ]; */
                    outmore(cw);
                    continue;
                }

                i = (long) p - (long) str;
                outsubstr(cw, (char *) str, i, fudge);
                cw->curx += i;

                while (isspace(*p))
                    p++;
                str = p;

#if 0
		if( str != ostr ) {
		    outsubstr( cw, "+", 1, fudge );
		    cw->curx+=2;
		}
#endif
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
        if (gc.context.botlx)
            *ob = 0;

        /* Display when beam at top to avoid flicker... */
        WaitTOF();
        Text(w->RPort, (char *) str, strlen((char *) str));
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
amii_scrollmsg(w, cw)
register struct Window *w;
register struct amii_WinDesc *cw;
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
amii_msgborder(w)
struct Window *w;
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
outmore(cw)
register struct amii_WinDesc *cw;
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
outsubstr(cw, str, len, fudge)
register struct amii_WinDesc *cw;
char *str;
int len;
int fudge;
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
amii_putsym(st, i, y, c)
winid st;
int i, y;
CHAR_P c;
{
    amii_curs(st, i, y);
    Text(amii_wins[st]->win->RPort, &c, 1);
}

/* Add to the last line in the message window */

void
amii_addtopl(s)
const char *s;
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
TextSpaces(rp, nr)
struct RastPort *rp;
int nr;
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
amii_remember_topl()
{
    /* ignore for now.  I think this will be done automatically by
     * the code writing to the message window, but I could be wrong.
     */
}

int
amii_doprev_message()
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

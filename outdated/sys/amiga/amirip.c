/* NetHack 3.6	amirip.c	$NHDT-Date: 1450453302 2015/12/18 15:41:42 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.16 $ */
/* Copyright (c) Kenneth Lorber, Bethesda, Maryland 1991,1992,1993,1995,1996.
 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include <exec/types.h>
#include <exec/io.h>
#include <exec/alerts.h>
#include <exec/devices.h>
#include <devices/console.h>
#include <devices/conunit.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <libraries/dosextens.h>
#include <ctype.h>
#include <string.h>
#include "winami.h"
#include "windefs.h"
#include "winext.h"
#include "winproto.h"

static struct RastPort *rp;

#ifdef AMII_GRAPHICS

#undef NULL
#define NULL 0

#ifdef AZTEC_C
#include <functions.h>
#else
#ifdef _DCC
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/console_protos.h>
#include <clib/diskfont_protos.h>
#else
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/console.h>
#include <proto/diskfont.h>
#endif

static char *load_list[] = { "tomb.iff", 0 };
static BitMapHeader tomb_bmhd;
static struct BitMap *tbmp[1] = { 0 };

static int cols[2] = { 154, 319 }; /* X location of center of columns */
static int cno = 0; /* current column */
#define TEXT_TOP (65 + yoff)

static xoff, yoff; /* image centering */

/* terrible kludge */
/* this is why prototypes should have ONLY types in them! */
#undef red
#undef green
#undef blue
#ifdef _DCC
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#else
#include <proto/graphics.h>
#include <proto/intuition.h>
#endif
#endif /* AZTEC_C */

static struct Window *ripwin = 0;
static void tomb_text(char *);
static void dofade(int, int, int);
static int search_cmap(int, int, int);

#define STONE_LINE_LEN                 \
    13 /* # chars that fit on one line \
        * (note 1 ' ' border) */

#define DEATH_LINE 10
#define YEAR_LINE 15

static unsigned short tomb_line;

extern struct amii_DisplayDesc *amiIDisplay;
extern struct Screen *HackScreen;
extern int havelace;

static unsigned short transpalette[AMII_MAXCOLORS] = {
    0x0000,
};

static struct NewWindow newwin = { 0, 0, 640, 200, 1, 0,
                                   MOUSEBUTTONS | VANILLAKEY | NOCAREREFRESH,
                                   BORDERLESS | ACTIVATE | SMART_REFRESH,
                                   NULL, NULL, (UBYTE *) NULL, NULL, NULL, -1,
                                   -1, 0xffff, 0xffff, CUSTOMSCREEN };

int wh; /* was local in outrip, but needed for SCALE macro */

int cmap_white, cmap_black;

void
amii_outrip(tmpwin, how, when)
winid tmpwin;
int how;
time_t when;
{
    int just_return = 0;
    int done, rtxth;
    struct IntuiMessage *imsg;
    int i;
    register char *dpx;
    char buf[200];
    int line, tw, ww;
    char *errstr = NULL;
    long year;

    if (!WINVERS_AMIV || HackScreen->RastPort.BitMap->Depth < 4)
        goto cleanup;

    /* Use the users display size */
    newwin.Height = amiIDisplay->ypix - newwin.TopEdge;
    newwin.Width = amiIDisplay->xpix;
    newwin.Screen = HackScreen;

    for (i = 0; i < amii_numcolors; ++i)
        sysflags.amii_curmap[i] = GetRGB4(HackScreen->ViewPort.ColorMap, i);

    ripwin = OpenWindow((void *) &newwin);
    if (!ripwin)
        goto cleanup;

    LoadRGB4(&HackScreen->ViewPort, transpalette, amii_numcolors);

    rp = ripwin->RPort;
    wh = ripwin->Height;
    ww = ripwin->Width;

#ifdef HACKFONT
    if (HackFont)
        SetFont(rp, HackFont);
#endif

    tomb_bmhd = ReadImageFiles(load_list, tbmp, &errstr);
    if (errstr)
        goto cleanup;
    if (tomb_bmhd.w > ww || tomb_bmhd.h > wh)
        goto cleanup;

#define GENOFF(full, used) ((((full) - (used)) / 2) & ~7)
    xoff = GENOFF(ww, tomb_bmhd.w);
    yoff = GENOFF(wh, tomb_bmhd.h);
    for (i = 0; i < SIZE(cols); i++)
        cols[i] += xoff;

    cmap_white = search_cmap(0, 0, 0);
    cmap_black = search_cmap(15, 15, 15);

    BltBitMap(*tbmp, 0, 0, rp->BitMap, xoff, yoff, tomb_bmhd.w, tomb_bmhd.h,
              0xc0, 0xff, NULL);

    /* Put together death description */
    formatkiller(buf, sizeof buf, how, FALSE);

    tw = TextLength(rp, buf, STONE_LINE_LEN) + 40;

    {
        char *p = buf;
        int x, tmp;
        for (x = STONE_LINE_LEN; x; x--)
            *p++ = 'W';
        *p = '\0';
        tmp = TextLength(rp, buf, STONE_LINE_LEN) + 40;
        tw = max(tw, tmp);
    }

    /* There are 5 lines of text on the stone. */
    rtxth = ripwin->RPort->TxHeight * 5;

    SetAfPt(rp, (UWORD *) NULL, 0);
    SetDrPt(rp, 0xFFFF);

    tomb_line = TEXT_TOP;

    SetDrMd(rp, JAM1);

    /* Put name on stone */
    Sprintf(buf, "%s", g.plname);
    buf[STONE_LINE_LEN] = 0;
    tomb_text(buf);

    /* Put $ on stone */
    Sprintf(buf, "%ld Au", g.done_money);
    buf[STONE_LINE_LEN] = 0; /* It could be a *lot* of gold :-) */
    tomb_text(buf);

    /* Put together death description */
    formatkiller(buf, sizeof buf, how, FALSE);

    /* Put death type on stone */
    for (line = DEATH_LINE, dpx = buf; line < YEAR_LINE; line++) {
        register int i, i0;
        char tmpchar;

        if ((i0 = strlen(dpx)) > STONE_LINE_LEN) {
            for (i = STONE_LINE_LEN; ((i0 > STONE_LINE_LEN) && i); i--) {
                if (dpx[i] == ' ')
                    i0 = i;
            }
            if (!i)
                i0 = STONE_LINE_LEN;
        }

        tmpchar = dpx[i0];
        dpx[i0] = 0;
        tomb_text(dpx);

        if (tmpchar != ' ') {
            dpx[i0] = tmpchar;
            dpx = &dpx[i0];
        } else {
            dpx = &dpx[i0 + 1];
        }
    }

    /* Put year on stone */
    year = yyyymmdd(when) / 10000L;
    Sprintf(buf, "%4ld", year);
    tomb_text(buf);

#ifdef NH320_DEDICATION
    /* dedication */
    cno = 1;
    tomb_line = TEXT_TOP;
    tomb_text("This release");
    tomb_text("of NetHack");
    tomb_text("is dedicated");
    tomb_text("to the");
    tomb_text("memory of");
    tomb_text("");
    tomb_text("Izchak");
    tomb_text(" Miller");
    tomb_text("");
    tomb_text("1935-1994");
    tomb_text("");
    tomb_text("Ascended");
#endif
    /* Fade from black to full color */
    dofade(0, 16, 1);

    /* Flush all messages to avoid typeahead */
    while (imsg = (struct IntuiMessage *) GetMsg(ripwin->UserPort))
        ReplyMsg((struct Message *) imsg);
    done = 0;
    while (!done) {
        WaitPort(ripwin->UserPort);
        while (imsg = (struct IntuiMessage *) GetMsg(ripwin->UserPort)) {
            switch (imsg->Class) {
            case MOUSEBUTTONS:
            case VANILLAKEY:
                done = 1;
                break;
            }
            ReplyMsg((struct Message *) imsg);
        }
    }

    /* Fade out */
    dofade(16, 0, -1);
    just_return = 1;

cleanup:
    /* free everything */
    if (ripwin) {
        Forbid();
        while (imsg = (struct IntuiMessage *) GetMsg(ripwin->UserPort))
            ReplyMsg((struct Message *) imsg);
        CloseWindow(ripwin);
        Permit();
    }
    LoadRGB4(&HackScreen->ViewPort, sysflags.amii_curmap, amii_numcolors);

    if (tbmp[0])
        FreeImageFiles(load_list, tbmp);
    if (just_return)
        return;
    /* fall back to the straight-ASCII version */
    genl_outrip(tmpwin, how, when);
}

static void
tomb_text(p)
char *p;
{
    char buf[STONE_LINE_LEN * 2];
    int l;

    tomb_line += rp->TxHeight;

    if (!*p)
        return;
    sprintf(buf, " %s ", p);
    l = TextLength(rp, buf, strlen(buf));

    SetAPen(rp, cmap_white);
    Move(rp, cols[cno] - (l / 2) - 1, tomb_line);
    Text(rp, buf, strlen(buf));

    SetAPen(rp, cmap_white);
    Move(rp, cols[cno] - (l / 2) + 1, tomb_line);
    Text(rp, buf, strlen(buf));

    SetAPen(rp, cmap_white);
    Move(rp, cols[cno] - (l / 2), tomb_line - 1);
    Text(rp, buf, strlen(buf));

    SetAPen(rp, cmap_white);
    Move(rp, cols[cno] - (l / 2), tomb_line + 1);
    Text(rp, buf, strlen(buf));

    SetAPen(rp, cmap_black);
    Move(rp, cols[cno] - (l / 2), tomb_line);
    Text(rp, buf, strlen(buf));
}

/* search colormap for best match to given color */
static int
search_cmap(int r0, int g0, int b0)
{
    int best = 0;
    int bdiff = 0x0fffffff;
    int x;
    for (x = 0; x < amii_numcolors; x++) {
        int r = r0 - ((amiv_init_map[x] >> 8) & 15);
        int g = g0 - ((amiv_init_map[x] >> 4) & 15);
        int b = b0 - ((amiv_init_map[x]) & 15);
        int diff = (r * r) + (g * g) + (b * b);
        if (diff < bdiff) {
            bdiff = diff;
            best = x;
        }
    }
    return best;
}

/* caution: this is NOT general! */
static void
dofade(int start, int stop, int inc)
{
    int i, j;
    for (i = start; (i * inc) <= stop; i += inc) {
        for (j = 0; j < amii_numcolors; ++j) {
            int r, g, b;

            r = (amiv_init_map[j] & 0xf00) >> 8;
            g = (amiv_init_map[j] & 0xf0) >> 4;
            b = (amiv_init_map[j] & 0xf);
            r = (r * i) / 16;
            g = (g * i) / 16;
            b = (b * i) / 16;
            transpalette[j] = ((r << 8) | (g << 4) | b);
        }
        LoadRGB4(&HackScreen->ViewPort, transpalette, amii_numcolors);
        Delay(1);
    }
}

#endif /* AMII_GRAPHICS */

/*
TODO:
        memory leaks
        fix ReadImageFiles to return error instead of panic on error
*/

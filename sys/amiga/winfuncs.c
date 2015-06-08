/* NetHack 3.6	winfuncs.c	$NHDT-Date: 1433806596 2015/06/08 23:36:36 $  $NHDT-Branch: master $:$NHDT-Revision: 1.15 $ */
/* Copyright (c) Gregg Wonderly, Naperville, Illinois,  1991,1992,1993,1996.
 */
/* NetHack may be freely redistributed.  See license for details. */

#include "NH:sys/amiga/windefs.h"
#include "NH:sys/amiga/winext.h"
#include "NH:sys/amiga/winproto.h"
#include "patchlevel.h"
#include "date.h"

extern struct TagItem scrntags[];

static BitMapHeader amii_bmhd;
static void cursor_common(struct RastPort *, int, int);

#ifdef CLIPPING
int CO, LI;

/* Changing clipping region, skip clear of screen in overview window. */
int reclip;

/* Must be set to at least two or you will get stuck! */
int xclipbord = 4, yclipbord = 2;
#endif

int mxsize, mysize;
struct Rectangle amii_oldover;
struct Rectangle amii_oldmsg;

extern struct TextFont *RogueFont;

int amii_msgAPen;
int amii_msgBPen;
int amii_statAPen;
int amii_statBPen;
int amii_menuAPen;
int amii_menuBPen;
int amii_textAPen;
int amii_textBPen;
int amii_otherAPen;
int amii_otherBPen;
long amii_libvers = LIBRARY_FONT_VERSION;

void
ami_wininit_data(dir)
int dir;
{
    extern unsigned short amii_init_map[AMII_MAXCOLORS];
    extern unsigned short amiv_init_map[AMII_MAXCOLORS];

    if (dir != WININIT)
        return;

    if (!WINVERS_AMIV) {
#ifdef TEXTCOLOR
        amii_numcolors = 8;
#else
        amii_numcolors = 4;
#endif
        amii_defpens[0] = C_BLACK; /* DETAILPEN        */
        amii_defpens[1] = C_BLUE;  /* BLOCKPEN         */
        amii_defpens[2] = C_BROWN; /* TEXTPEN          */
        amii_defpens[3] = C_WHITE; /* SHINEPEN         */
        amii_defpens[4] = C_BLUE;  /* SHADOWPEN        */
        amii_defpens[5] = C_CYAN;  /* FILLPEN          */
        amii_defpens[6] = C_WHITE; /* FILLTEXTPEN      */
        amii_defpens[7] = C_CYAN;  /* BACKGROUNDPEN    */
        amii_defpens[8] = C_RED;   /* HIGHLIGHTTEXTPEN */
        amii_defpens[9] = C_WHITE; /* BARDETAILPEN     */
        amii_defpens[10] = C_CYAN; /* BARBLOCKPEN      */
        amii_defpens[11] = C_BLUE; /* BARTRIMPEN       */
        amii_defpens[12] = (unsigned short) ~0;

        amii_msgAPen = C_WHITE;
        amii_msgBPen = C_BLACK;
        amii_statAPen = C_WHITE;
        amii_statBPen = C_BLACK;
        amii_menuAPen = C_WHITE;
        amii_menuBPen = C_BLACK;
        amii_textAPen = C_WHITE;
        amii_textBPen = C_BLACK;
        amii_otherAPen = C_RED;
        amii_otherBPen = C_BLACK;

        mxsize = 8;
        mysize = 8;

        amii_libvers = LIBRARY_FONT_VERSION;
        memcpy(amii_initmap, amii_init_map, sizeof(amii_initmap));
    } else {
        mxsize = 16;
        mysize = 16;

        amii_numcolors = 16;

        amii_defpens[0] = C_BLACK;     /* DETAILPEN        */
        amii_defpens[1] = C_WHITE;     /* BLOCKPEN         */
        amii_defpens[2] = C_BLACK;     /* TEXTPEN          */
        amii_defpens[3] = C_CYAN;      /* SHINEPEN         */
        amii_defpens[4] = C_BLUE;      /* SHADOWPEN        */
        amii_defpens[5] = C_GREYBLUE;  /* FILLPEN          */
        amii_defpens[6] = C_LTGREY;    /* FILLTEXTPEN      */
        amii_defpens[7] = C_GREYBLUE;  /* BACKGROUNDPEN    */
        amii_defpens[8] = C_RED;       /* HIGHLIGHTTEXTPEN */
        amii_defpens[9] = C_WHITE;     /* BARDETAILPEN     */
        amii_defpens[10] = C_GREYBLUE; /* BARBLOCKPEN      */
        amii_defpens[11] = C_BLUE;     /* BARTRIMPEN       */
        amii_defpens[12] = (unsigned short) ~0;

        amii_msgAPen = C_WHITE;
        amii_msgBPen = C_GREYBLUE;
        amii_statAPen = C_WHITE;
        amii_statBPen = C_GREYBLUE;
        amii_menuAPen = C_BLACK;
        amii_menuBPen = C_LTGREY;
        amii_textAPen = C_BLACK;
        amii_textBPen = C_LTGREY;
        amii_otherAPen = C_RED;
        amii_otherBPen = C_BLACK;
        amii_libvers = LIBRARY_TILE_VERSION;

        memcpy(amii_initmap, amiv_init_map, sizeof(amii_initmap));
    }
#ifdef OPT_DISPMAP
    dispmap_sanity();
#endif
    memcpy(sysflags.amii_dripens, amii_defpens,
           sizeof(sysflags.amii_dripens));
}

#ifdef INTUI_NEW_LOOK
struct Hook SM_FilterHook;
struct Hook fillhook;
struct TagItem wintags[] = {
    { WA_BackFill, (ULONG) &fillhook },
    { WA_PubScreenName, (ULONG) "NetHack" },
    { TAG_END, 0 },
};
#endif

void amii_destroy_nhwindow(win) /* just hide */
register winid win;
{
    int i;
    int type;
    register struct amii_WinDesc *cw;

    if (win == WIN_ERR || (cw = amii_wins[win]) == NULL) {
        panic(winpanicstr, win, "destroy_nhwindow");
    }

    if (WINVERS_AMIV) {
        if (cw->type == NHW_MAP) {
            /* If inventory is up, close it now, it will be freed later */
            if (alwaysinvent && WIN_INVEN != WIN_ERR && amii_wins[WIN_INVEN]
                && amii_wins[WIN_INVEN]->win) {
                dismiss_nhwindow(WIN_INVEN);
            }

            /* Tear down overview window if it is up */
            if (WIN_OVER != WIN_ERR) {
                amii_destroy_nhwindow(WIN_OVER);
                WIN_OVER = WIN_ERR;
            }
        } else if (cw->type == NHW_OVER) {
            struct Window *w = amii_wins[WIN_OVER]->win;
            amii_oldover.MinX = w->LeftEdge;
            amii_oldover.MinY = w->TopEdge;
            amii_oldover.MaxX = w->Width;
            amii_oldover.MaxY = w->Height;

            if (WIN_MESSAGE != WIN_ERR && amii_wins[WIN_MESSAGE]) {
                w = amii_wins[WIN_MESSAGE]->win;
                amii_oldmsg.MinX = w->LeftEdge;
                amii_oldmsg.MinY = w->TopEdge;
                amii_oldmsg.MaxX = w->Width;
                amii_oldmsg.MaxY = w->Height;
                SizeWindow(amii_wins[WIN_MESSAGE]->win,
                           (amiIDisplay->xpix
                            - amii_wins[WIN_MESSAGE]->win->LeftEdge)
                               - amii_wins[WIN_MESSAGE]->win->Width,
                           0);
            }
        }
    }

    /* Tear down the Intuition stuff */
    dismiss_nhwindow(win);
    type = cw->type;

    if (cw->resp) {
        free(cw->resp);
        cw->resp = NULL;
    }
    if (cw->canresp) {
        free(cw->canresp);
        cw->canresp = NULL;
    }
    if (cw->morestr) {
        free(cw->morestr);
        cw->morestr = NULL;
    }
    if (cw->hook) {
        free(cw->hook);
        cw->hook = NULL;
    }

    if (cw->data && (cw->type == NHW_MESSAGE || cw->type == NHW_MENU
                     || cw->type == NHW_TEXT)) {
        for (i = 0; i < cw->maxrow; ++i) {
            if (cw->data[i])
                free(cw->data[i]);
        }
        free(cw->data);
    }

    free(cw);
    amii_wins[win] = NULL;

    /* Set globals to WIN_ERR for known one-of-a-kind windows. */
    if (win == WIN_MAP)
        WIN_MAP = WIN_ERR;
    else if (win == WIN_STATUS)
        WIN_STATUS = WIN_ERR;
    else if (win == WIN_MESSAGE)
        WIN_MESSAGE = WIN_ERR;
    else if (win == WIN_INVEN)
        WIN_INVEN = WIN_ERR;
}

#ifdef INTUI_NEW_LOOK
struct FillParams {
    struct Layer *layer;
    struct Rectangle bounds;
    WORD offsetx;
    WORD offsety;
};

#ifdef __GNUC__
#ifdef __PPC__
void PPC_LayerFillHook(void);
struct EmulLibEntry LayerFillHook = { TRAP_LIB, 0,
                                      (void (*)(void)) PPC_LayerFillHook };
void
PPC_LayerFillHook(void)
{
    struct Hook *hk = (struct Hook *) REG_A0;
    struct RastPort *rp = (struct RastPort *) REG_A2;
    struct FillParams *fp = (struct FillParams *) REG_A1;
#else
void
LayerFillHook(void)
{
    register struct Hook *hk asm("a0");
    register struct RastPort *rp asm("a2");
    register struct FillParams *fp asm("a1");
#endif
#else
void
#ifndef _DCC
    __interrupt
#endif
        __saveds __asm LayerFillHook(register __a0 struct Hook *hk,
                                     register __a2 struct RastPort *rp,
                                     register __a1 struct FillParams *fp)
{
#endif

    register long x, y, xmax, ymax;
    register int apen;
    struct RastPort rptmp;

    memcpy(&rptmp, rp, sizeof(struct RastPort));
    rptmp.Layer = NULL;

    switch ((int) hk->h_Data) {
    case NHW_STATUS:
        apen = amii_statBPen;
        break;
    case NHW_MESSAGE:
        apen = amii_msgBPen;
        break;
    case NHW_TEXT:
        apen = amii_textBPen;
        break;
    case NHW_MENU:
        apen = amii_menuBPen;
        break;
    case -2:
        apen = amii_otherBPen;
        break;
    case NHW_BASE:
    case NHW_MAP:
    case NHW_OVER:
    default:
        apen = C_BLACK;
        break;
    }

    x = fp->bounds.MinX;
    y = fp->bounds.MinY;
    xmax = fp->bounds.MaxX;
    ymax = fp->bounds.MaxY;

    SetAPen(&rptmp, apen);
    SetBPen(&rptmp, apen);
    SetDrMd(&rptmp, JAM2);
    RectFill(&rptmp, x, y, xmax, ymax);
}
#endif

amii_create_nhwindow(type) register int type;
{
    register struct Window *w = NULL;
    register struct NewWindow *nw = NULL;
    register struct amii_WinDesc *wd = NULL;
    struct Window *mapwin = NULL, *stwin = NULL, *msgwin = NULL;
    register int newid;
    int maph, stath, scrfontysize;

    scrfontysize = HackScreen->Font->ta_YSize;

    /*
     * Initial mapwindow height, this might change later in tilemode
     * and low screen
     */
    maph = (21 * mxsize) + 2
           + (bigscreen
                  ? HackScreen->WBorTop + HackScreen->WBorBottom
                        + scrfontysize + 1
                  : 0);

    /* Status window height, avoids having to calculate many times */
    stath =
        txheight * 2 + 2 + (WINVERS_AMIV || bigscreen
                                ? HackScreen->WBorTop + HackScreen->WBorBottom
                                      + (bigscreen ? scrfontysize + 1 : 0)
                                : 0);

    if (WIN_STATUS != WIN_ERR && amii_wins[WIN_STATUS])
        stwin = amii_wins[WIN_STATUS]->win;

    if (WIN_MESSAGE != WIN_ERR && amii_wins[WIN_MESSAGE])
        msgwin = amii_wins[WIN_MESSAGE]->win;

    if (WIN_MAP != WIN_ERR && amii_wins[WIN_MAP])
        mapwin = amii_wins[WIN_MAP]->win;

    /* Create Port anytime that we need it */

    if (HackPort == NULL) {
        HackPort = CreateMsgPort();
        if (!HackPort)
            panic("no memory for msg port");
    }

    nw = &new_wins[type].newwin;
    nw->Width = amiIDisplay->xpix;
    nw->Screen = HackScreen;

    if (WINVERS_AMIV) {
        nw->DetailPen = C_WHITE;
        nw->BlockPen = C_GREYBLUE;
    } else {
        nw->DetailPen = C_WHITE;
        nw->BlockPen = C_BLACK;
    }

    if (type == NHW_BASE) {
        nw->LeftEdge = 0;
        nw->TopEdge = HackScreen->BarHeight + 1;
        nw->Width = HackScreen->Width;
        nw->Height = HackScreen->Height - nw->TopEdge;
    } else if (!WINVERS_AMIV && type == NHW_MAP) {
        nw->LeftEdge = 0;
        nw->Height = maph;

        if (msgwin && stwin) {
            nw->TopEdge = stwin->TopEdge - maph;
        } else {
            panic("msgwin and stwin must open before map");
        }
        if (nw->TopEdge < 0)
            panic("Too small screen to fit map");
    } else if (type == NHW_MAP && WINVERS_AMIV) {
        struct Window *w;

        w = amii_wins[WIN_MESSAGE]->win;
        nw->LeftEdge = 0;
        nw->TopEdge = w->TopEdge + w->Height;
        nw->Width = amiIDisplay->xpix - nw->LeftEdge;

        w = amii_wins[WIN_STATUS]->win;
        nw->Height = w->TopEdge - nw->TopEdge;
        nw->MaxHeight = 0xffff;
        nw->MaxWidth = 0xffff;

        if (nw->TopEdge + nw->Height > amiIDisplay->ypix - 1)
            nw->Height = amiIDisplay->ypix - nw->TopEdge - 1;
    } else if (type == NHW_STATUS) {
        if (!WINVERS_AMIV && (WIN_MAP != WIN_ERR && amii_wins[WIN_MAP]))
            w = amii_wins[WIN_MAP]->win;
        else if (WIN_BASE != WIN_ERR && amii_wins[WIN_BASE])
            w = amii_wins[WIN_BASE]->win;
        else
            panic("No window to base STATUS location from");

        nw->Height = stath;
        nw->TopEdge = amiIDisplay->ypix - nw->Height;
        nw->LeftEdge = w->LeftEdge;

        if (nw->LeftEdge + nw->Width >= amiIDisplay->xpix)
            nw->LeftEdge = 0;

        if (nw->Width >= amiIDisplay->xpix - nw->LeftEdge)
            nw->Width = amiIDisplay->xpix - nw->LeftEdge;
    } else if (WINVERS_AMIV && type == NHW_OVER) {
        nw->Flags |= WINDOWSIZING | WINDOWDRAG | WINDOWCLOSE;
        nw->IDCMPFlags |= CLOSEWINDOW;
        /* Bring up window as half the width of the message window, and make
         * the message window change to one half the width...
         */
        if (amii_oldover.MaxX != 0) {
            nw->LeftEdge = amii_oldover.MinX;
            nw->TopEdge = amii_oldover.MinY;
            nw->Width = amii_oldover.MaxX;
            nw->Height = amii_oldover.MaxY;
            ChangeWindowBox(amii_wins[WIN_MESSAGE]->win, amii_oldmsg.MinX,
                            amii_oldmsg.MinY, amii_oldmsg.MaxX,
                            amii_oldmsg.MaxY);
        } else {
            nw->LeftEdge = (amii_wins[WIN_MESSAGE]->win->Width * 4) / 9;
            nw->TopEdge = amii_wins[WIN_MESSAGE]->win->TopEdge;
            nw->Width = amiIDisplay->xpix - nw->LeftEdge;
            nw->Height = amii_wins[WIN_MESSAGE]->win->Height;
            SizeWindow(amii_wins[WIN_MESSAGE]->win,
                       nw->LeftEdge - amii_wins[WIN_MESSAGE]->win->Width, 0);
        }
    } else if (type == NHW_MESSAGE) {
        if (!WINVERS_AMIV && (WIN_MAP != WIN_ERR && amii_wins[WIN_MAP]))
            w = amii_wins[WIN_MAP]->win;
        else if (WIN_BASE != WIN_ERR && amii_wins[WIN_BASE])
            w = amii_wins[WIN_BASE]->win;
        else
            panic("No window to base STATUS location from");

        nw->TopEdge = bigscreen ? HackScreen->BarHeight + 1 : 0;

        /* Assume highest possible message window */
        nw->Height = HackScreen->Height - nw->TopEdge - maph - stath;

        /* In tilemode we can cope with this */
        if (WINVERS_AMIV && nw->Height < 0)
            nw->Height = 0;

        /* If in fontmode messagewindow is too small, open it with 3 lines
           and overlap it with map */
        if (nw->Height < txheight + 2) {
            nw->Height = txheight * 4 + 3 + HackScreen->WBorTop
                         + HackScreen->WBorBottom;
        }

        if ((nw->Height - 2) / txheight < 3) {
            scrollmsg = 0;
            nw->Title = 0;
        } else {
            nw->FirstGadget = &MsgScroll;
            nw->Flags |= WINDOWSIZING | WINDOWDRAG;
            nw->Flags &= ~BORDERLESS;

            if (WINVERS_AMIV || nw->Height == 0) {
                if (WINVERS_AMIV) {
                    nw->Height = TextsFont->tf_YSize + HackScreen->WBorTop + 3
                                 + HackScreen->WBorBottom;
                    if (bigscreen)
                        nw->Height += (txheight * 6);
                    else
                        nw->Height += (txheight * 3);
                } else {
                    nw->Height =
                        HackScreen->Height - nw->TopEdge - stath - maph;
                }
            }
        }

        /* Do we have room for larger message window ?
         * This is possible if we can show full height map in tile
         * mode	with default scaling.
         */
        if (nw->Height + stath + maph < HackScreen->Height - nw->TopEdge)
            nw->Height = HackScreen->Height - nw->TopEdge - 1 - maph - stath;

#ifdef INTUI_NEW_LOOK
        if (IntuitionBase->LibNode.lib_Version >= 37) {
            MsgPropScroll.Flags |= PROPNEWLOOK;
            PropScroll.Flags |= PROPNEWLOOK;
        }
#endif
    }

    nw->IDCMPFlags |= MENUPICK;

    /* Check if there is "Room" for all this stuff... */
    if ((WINVERS_AMIV || bigscreen) && type != NHW_BASE) {
        nw->Flags &= ~(BORDERLESS | BACKDROP);

        if (WINVERS_AMIV) {
            if (type == NHW_STATUS) {
                nw->Flags &=
                    ~(WINDOWDRAG | WINDOWDEPTH | SIZEBRIGHT | WINDOWSIZING);
                nw->IDCMPFlags &= ~NEWSIZE;
            } else {
                nw->Flags |=
                    (WINDOWDRAG | WINDOWDEPTH | SIZEBRIGHT | WINDOWSIZING);
                nw->IDCMPFlags |= NEWSIZE;
            }
        } else {
            if (HackScreen->Width < 657) {
                nw->Flags |= (WINDOWDRAG | WINDOWDEPTH);
            } else {
                nw->Flags |= (WINDOWDRAG | WINDOWDEPTH | SIZEBRIGHT);
            }
        }
    }

    if (WINVERS_AMII && type == NHW_MAP)
        nw->Flags &= ~WINDOWSIZING;

    if (type == NHW_MESSAGE && scrollmsg) {
        nw->Flags |= WINDOWDRAG | WINDOWDEPTH | SIZEBRIGHT | WINDOWSIZING;
        nw->Flags &= ~BORDERLESS;
    }

    /* No titles on a hires only screen except for messagewindow */
    if (!(WINVERS_AMIV && type == NHW_MAP) && !bigscreen
        && type != NHW_MESSAGE)
        nw->Title = 0;

    wd = (struct amii_WinDesc *) alloc(sizeof(struct amii_WinDesc));
    memset(wd, 0, sizeof(struct amii_WinDesc));

    /* Both, since user may have changed the pen settings so respect those */
    if (WINVERS_AMII || WINVERS_AMIV) {
        /* Special backfill for these types of layers */
        switch (type) {
        case NHW_MESSAGE:
        case NHW_STATUS:
        case NHW_TEXT:
        case NHW_MENU:
        case NHW_BASE:
        case NHW_OVER:
        case NHW_MAP:
            if (wd) {
#ifdef __GNUC__
                fillhook.h_Entry = (void *) &LayerFillHook;
#else
                fillhook.h_Entry = (ULONG (*) ()) LayerFillHook;
#endif
                fillhook.h_Data = (void *) type;
                fillhook.h_SubEntry = 0;
                wd->hook = alloc(sizeof(fillhook));
                memcpy(wd->hook, &fillhook, sizeof(fillhook));
                memcpy(wd->wintags, wintags, sizeof(wd->wintags));
                wd->wintags[0].ti_Data = (long) wd->hook;
                nw->Extension = (void *) wd->wintags;
            }
            break;
        }
    }

    /* Don't open MENU or TEXT windows yet */

    if (type == NHW_MENU || type == NHW_TEXT)
        w = NULL;
    else
        w = OpenShWindow((void *) nw);

    if (w == NULL && type != NHW_MENU && type != NHW_TEXT) {
        char buf[100];

        sprintf(buf, "nw type (%d) dims l: %d, t: %d, w: %d, h: %d", type,
                nw->LeftEdge, nw->TopEdge, nw->Width, nw->Height);
        raw_print(buf);
        panic("bad openwin %d", type);
    }

    /* Check for an empty slot */

    for (newid = 0; newid < MAXWIN + 1; newid++) {
        if (amii_wins[newid] == 0)
            break;
    }

    if (newid == MAXWIN + 1)
        panic("time to write re-alloc code\n");

    /* Set wincnt accordingly */

    if (newid > wincnt)
        wincnt = newid;

    /* Do common initialization */

    amii_wins[newid] = wd;

    wd->newwin = NULL;
    wd->win = w;
    wd->type = type;
    wd->wflags = 0;
    wd->active = FALSE;
    wd->curx = wd->cury = 0;
    wd->resp = wd->canresp = wd->morestr = 0; /* CHECK THESE */
    wd->maxrow = new_wins[type].maxrow;
    wd->maxcol = new_wins[type].maxcol;

    if (type != NHW_TEXT && type != NHW_MENU) {
        if (TextsFont && (type == NHW_MESSAGE || type == NHW_STATUS)) {
            SetFont(w->RPort, TextsFont);
            txheight = w->RPort->TxHeight;
            txwidth = w->RPort->TxWidth;
            txbaseline = w->RPort->TxBaseline;
            if (type == NHW_MESSAGE) {
                if (scrollmsg) {
                    if (WINVERS_AMIV) {
                        WindowLimits(w, 100, w->BorderTop + w->BorderBottom
                                                 + ((txheight + 1) * 2) + 1,
                                     0, 0);
                    } else {
                        WindowLimits(w, w->Width,
                                     w->BorderTop + w->BorderBottom
                                         + ((txheight + 1) * 2) + 1,
                                     0, 0);
                    }
                } else {
                    WindowLimits(w, w->Width, w->BorderTop + w->BorderBottom
                                                  + txheight + 2,
                                 0, 0);
                }
            }
        }
        if (type != NHW_MAP) {
            SetFont(w->RPort, TextsFont);
        }
#ifdef HACKFONT
        else if (HackFont)
            SetFont(w->RPort, HackFont);
#endif
    }

    /* Text and menu windows are not opened yet */
    if (w) {
        wd->rows = (w->Height - w->BorderTop - w->BorderBottom - 2)
                   / w->RPort->TxHeight;
        wd->cols = (w->Width - w->BorderLeft - w->BorderRight - 2)
                   / w->RPort->TxWidth;
    }

    /* Okay, now do the individual type initialization */

    switch (type) {
    /* History lines for MESSAGE windows are stored in cw->data[?].
     * maxcol and maxrow are used as cursors.  maxrow is the count
     * of the number of history lines stored.  maxcol is the cursor
     * to the last line that was displayed by ^P.
     */
    case NHW_MESSAGE:
        SetMenuStrip(w, MenuStrip);
        MsgScroll.TopEdge = HackScreen->WBorTop + scrfontysize + 1;
        iflags.msg_history = wd->rows * 10;
        if (iflags.msg_history < 40)
            iflags.msg_history = 40;
        if (iflags.msg_history > 400)
            iflags.msg_history = 400;
        iflags.window_inited = TRUE;
        wd->data = (char **) alloc(iflags.msg_history * sizeof(char *));
        memset(wd->data, 0, iflags.msg_history * sizeof(char *));
        wd->maxrow = wd->maxcol = 0;
        /* Indicate that we have not positioned the cursor yet */
        wd->curx = -1;
        break;

    /* A MENU contains a list of lines in wd->data[?].  These
     * lines are created in amii_putstr() by reallocating the size
     * of wd->data to hold enough (char *)'s.  wd->rows is the
     * number of (char *)'s allocated.  wd->maxrow is the number
     * used.  wd->maxcol is used to track how wide the menu needs
     * to be.  wd->resp[x] contains the characters that correspond
     * to selecting wd->data[x].  wd->resp[x] corresponds to
     * wd->data[x] for any x. Elements of wd->data[?] that are not
     * valid selections have the corresponding element of
     * wd->resp[] set to a value of '\01';  i.e. a ^A which is
     * not currently a valid keystroke for responding to any
     * MENU or TEXT window.
     */
    case NHW_MENU:
        MenuScroll.TopEdge = HackScreen->WBorTop + scrfontysize + 1;
        wd->resp = (char *) alloc(256);
        wd->resp[0] = 0;
        wd->rows = wd->maxrow = 0;
        wd->cols = wd->maxcol = 0;
        wd->data = NULL;
        break;

    /* See the explanation of MENU above.  Except, wd->resp[] is not
     * used for TEXT windows since there is no selection of a
     * a line performed/allowed.  The window is always full
     * screen width.
     */
    case NHW_TEXT:
        MenuScroll.TopEdge = HackScreen->WBorTop + scrfontysize + 1;
        wd->rows = wd->maxrow = 0;
        wd->cols = wd->maxcol = amiIDisplay->cols;
        wd->data = NULL;
        wd->morestr = NULL;
        break;

    /* The status window has only two lines.  These are stored in
     * wd->data[], and here we allocate the space for them.
     */
    case NHW_STATUS:
        SetMenuStrip(w, MenuStrip);
        /* wd->cols is the number of characters which fit across the
         * screen.
         */
        wd->data = (char **) alloc(3 * sizeof(char *));
        wd->data[0] = (char *) alloc(wd->cols + 10);
        wd->data[1] = (char *) alloc(wd->cols + 10);
        wd->data[2] = NULL;
        break;

    /* NHW_OVER does not use wd->data[] or the other text
     * manipulating members of the amii_WinDesc structure.
     */
    case NHW_OVER:
        SetMenuStrip(w, MenuStrip);
        break;

    /* NHW_MAP does not use wd->data[] or the other text
     * manipulating members of the amii_WinDesc structure.
     */
    case NHW_MAP:
        SetMenuStrip(w, MenuStrip);
        if (WINVERS_AMIV) {
            CO = (w->Width - w->BorderLeft - w->BorderRight) / mxsize;
            LI = (w->Height - w->BorderTop - w->BorderBottom) / mysize;
            amii_setclipped();
            SetFont(w->RPort, RogueFont);
            SetAPen(w->RPort, C_WHITE); /* XXX not sufficient */
            SetBPen(w->RPort, C_BLACK);
            SetDrMd(w->RPort, JAM2);
        } else {
            if (HackFont)
                SetFont(w->RPort, HackFont);
        }
        break;

    /* The base window must exist until CleanUp() deletes it. */
    case NHW_BASE:
        SetMenuStrip(w, MenuStrip);
        /* Make our requesters come to our screen */
        {
            register struct Process *myProcess =
                (struct Process *) FindTask(NULL);
            pr_WindowPtr = (struct Window *) (myProcess->pr_WindowPtr);
            myProcess->pr_WindowPtr = (APTR) w;
        }

        /* Need this for RawKeyConvert() */

        ConsoleIO.io_Data = (APTR) w;
        ConsoleIO.io_Length = sizeof(struct Window);
        ConsoleIO.io_Message.mn_ReplyPort = CreateMsgPort();
        if (OpenDevice("console.device", -1L, (struct IORequest *) &ConsoleIO,
                       0L) != 0) {
            Abort(AG_OpenDev | AO_ConsoleDev);
        }

        ConsoleDevice = (struct Library *) ConsoleIO.io_Device;

        KbdBuffered = 0;

#ifdef HACKFONT
        if (TextsFont)
            SetFont(w->RPort, TextsFont);
        else if (HackFont)
            SetFont(w->RPort, HackFont);
#endif
        txwidth = w->RPort->TxWidth;
        txheight = w->RPort->TxHeight;
        txbaseline = w->RPort->TxBaseline;
        break;

    default:
        panic("bad create_nhwindow( %d )\n", type);
        return WIN_ERR;
    }

    return (newid);
}

#ifdef __GNUC__
#ifdef __PPC__
int PPC_SM_Filter(void);
struct EmulLibEntry SM_Filter = { TRAP_LIB, 0,
                                  (int (*)(void)) PPC_SM_Filter };
int
PPC_SM_Filter(void)
{
    struct Hook *hk = (struct Hook *) REG_A0;
    ULONG modeID = (ULONG) REG_A1;
    struct ScreenModeRequester *smr = (struct ScreenModeRequester *) REG_A2;
#else
int
SM_Filter(void)
{
    register struct Hook *hk asm("a0");
    register ULONG modeID asm("a1");
    register struct ScreenModeRequester *smr asm("a2");
#endif
#else
int
#ifndef _DCC
    __interrupt
#endif
        __saveds __asm SM_Filter(
            register __a0 struct Hook *hk, register __a1 ULONG modeID,
            register __a2 struct ScreenModeRequester *smr)
{
#endif
    struct DimensionInfo dims;
    struct DisplayInfo disp;
    DisplayInfoHandle handle;
    handle = FindDisplayInfo(modeID);
    if (handle) {
        GetDisplayInfoData(handle, (char *) &dims, sizeof(dims), DTAG_DIMS,
                           modeID);
        GetDisplayInfoData(handle, (char *) &disp, sizeof(disp), DTAG_DISP,
                           modeID);
        if (!disp.NotAvailable && dims.MaxDepth <= 8
            && dims.StdOScan.MaxX >= WIDTH - 1
            && dims.StdOScan.MaxY >= SCREENHEIGHT - 1) {
            return 1;
        }
    }
    return 0;
}

/* Initialize the windowing environment */

void
amii_init_nhwindows(argcp, argv)
int *argcp;
char **argv;
{
    int i;
    struct Screen *wbscr;
    int forcenobig = 0;

    if (HackScreen)
        panic("init_nhwindows() called twice", 0);

    /* run args & set bigscreen from -L(1)/-l(-1) */
    {
        int lclargc = *argcp;
        int t;
        char **argv_in = argv;
        char **argv_out = argv;

        for (t = 1; t <= lclargc; t++) {
            if (!strcmp("-L", *argv_in) || !strcmp("-l", *argv_in)) {
                bigscreen = (*argv_in[1] == 'l') ? -1 : 1;
                /* and eat the flag */
                (*argcp)--;
            } else {
                *argv_out = *argv_in; /* keep the flag */
                argv_out++;
            }
            argv_in++;
        }
        *argv_out = 0;
    }

    WIN_MESSAGE = WIN_ERR;
    WIN_MAP = WIN_ERR;
    WIN_STATUS = WIN_ERR;
    WIN_INVEN = WIN_ERR;
    WIN_BASE = WIN_ERR;
    WIN_OVER = WIN_ERR;

    if ((IntuitionBase = (struct IntuitionBase *) OpenLibrary(
             "intuition.library", amii_libvers)) == NULL) {
        Abort(AG_OpenLib | AO_Intuition);
    }

    if ((GfxBase = (struct GfxBase *) OpenLibrary("graphics.library",
                                                  amii_libvers)) == NULL) {
        Abort(AG_OpenLib | AO_GraphicsLib);
    }

    if ((LayersBase = (struct Library *) OpenLibrary("layers.library",
                                                     amii_libvers)) == NULL) {
        Abort(AG_OpenLib | AO_LayersLib);
    }

    if ((GadToolsBase = OpenLibrary("gadtools.library", amii_libvers))
        == NULL) {
        Abort(AG_OpenLib | AO_GadTools);
    }

    if ((AslBase = OpenLibrary("asl.library", amii_libvers)) == NULL) {
        Abort(AG_OpenLib);
    }

    amiIDisplay =
        (struct amii_DisplayDesc *) alloc(sizeof(struct amii_DisplayDesc));
    memset(amiIDisplay, 0, sizeof(struct amii_DisplayDesc));

    /* Use Intuition sizes for overscan screens... */

    amiIDisplay->xpix = 0;
#ifdef INTUI_NEW_LOOK
    if (IntuitionBase->LibNode.lib_Version >= 37) {
        if (wbscr = LockPubScreen("Workbench")) {
            amiIDisplay->xpix = wbscr->Width;
            amiIDisplay->ypix = wbscr->Height;
            UnlockPubScreen(NULL, wbscr);
        }
    }
#endif
    if (amiIDisplay->xpix == 0) {
        amiIDisplay->ypix = GfxBase->NormalDisplayRows;
        amiIDisplay->xpix = GfxBase->NormalDisplayColumns;
    }

    amiIDisplay->cols = amiIDisplay->xpix / FONTWIDTH;

    amiIDisplay->toplin = 0;
    amiIDisplay->rawprint = 0;
    amiIDisplay->lastwin = 0;

    if (bigscreen == 0) {
        if ((GfxBase->ActiView->ViewPort->Modes & LACE) == LACE) {
            amiIDisplay->ypix *= 2;
            NewHackScreen.ViewModes |= LACE;
            bigscreen = 1;
        } else if (GfxBase->NormalDisplayRows >= 300
                   || amiIDisplay->ypix >= 300) {
            bigscreen = 1;
        }
    } else if (bigscreen == -1) {
        bigscreen = 0;
        forcenobig = 1;
    } else if (bigscreen) {
        /* If bigscreen requested and we don't have enough rows in
         * noninterlaced mode, switch to interlaced...
         */
        if (GfxBase->NormalDisplayRows < 300) {
            amiIDisplay->ypix *= 2;
            NewHackScreen.ViewModes |= LACE;
        }
    }

    if (!bigscreen) {
        alwaysinvent = 0;
    }
    amiIDisplay->rows = amiIDisplay->ypix / FONTHEIGHT;

#ifdef HACKFONT
    /*
     *  Load the fonts that we need.
     */

    if (DiskfontBase = OpenLibrary("diskfont.library", amii_libvers)) {
        Hack80.ta_Name -= SIZEOF_DISKNAME;
        HackFont = OpenDiskFont(&Hack80);
        Hack80.ta_Name += SIZEOF_DISKNAME;

        /* Textsfont13 is filled in with "FONT=" settings. The default is
         * courier/13.
         */
        TextsFont = NULL;
        if (bigscreen)
            TextsFont = OpenDiskFont(&TextsFont13);

        /* Try hack/8 for texts if no user specified font */
        if (TextsFont == NULL) {
            Hack80.ta_Name -= SIZEOF_DISKNAME;
            TextsFont = OpenDiskFont(&Hack80);
            Hack80.ta_Name += SIZEOF_DISKNAME;
        }

        /* If no fonts, make everything topaz 8 for non-view windows.
         */
        Hack80.ta_Name = "topaz.font";
        RogueFont = OpenFont(&Hack80);
        if (!RogueFont)
            panic("Can't get topaz:8");
        if (!HackFont || !TextsFont) {
            if (!HackFont) {
                HackFont = OpenFont(&Hack80);
                if (!HackFont)
                    panic("Can't get a map font, topaz:8");
            }

            if (!TextsFont) {
                TextsFont = OpenFont(&Hack80);
                if (!TextsFont)
                    panic("Can't open text font");
            }
        }
        CloseLibrary(DiskfontBase);
        DiskfontBase = NULL;
    }
#endif

    /* Adjust getlin window size to font */

    if (TextsFont) {
        extern SHORT BorderVectors1[];
        extern SHORT BorderVectors2[];
        extern struct Gadget Gadget2;
        extern struct Gadget String;
        extern struct NewWindow StrWindow;
        BorderVectors1[2] +=
            (TextsFont->tf_XSize - 8) * 6; /* strlen("Cancel") == 6 */
        BorderVectors1[4] += (TextsFont->tf_XSize - 8) * 6;
        BorderVectors1[5] += TextsFont->tf_YSize - 8;
        BorderVectors1[7] += TextsFont->tf_YSize - 8;
        BorderVectors2[2] += (TextsFont->tf_XSize - 8) * 6;
        BorderVectors2[4] += (TextsFont->tf_XSize - 8) * 6;
        BorderVectors2[5] += TextsFont->tf_YSize - 8;
        BorderVectors2[7] += TextsFont->tf_YSize - 8;
        Gadget2.TopEdge += TextsFont->tf_YSize - 8;
        Gadget2.Width += (TextsFont->tf_XSize - 8) * 6;
        Gadget2.Height += TextsFont->tf_YSize - 8;
        String.LeftEdge += (TextsFont->tf_XSize - 8) * 6;
        String.TopEdge += TextsFont->tf_YSize - 8;
        String.Width += TextsFont->tf_XSize - 8;
        String.Height += TextsFont->tf_YSize - 8;
        StrWindow.Width += (TextsFont->tf_XSize - 8) * 7;
        StrWindow.Height +=
            (TextsFont->tf_YSize - 8) * 2; /* Titlebar + 1 row of gadgets */
    }

    /* This is the size screen we want to open, within reason... */

    NewHackScreen.Width = max(WIDTH, amiIDisplay->xpix);
    NewHackScreen.Height = max(SCREENHEIGHT, amiIDisplay->ypix);
    {
        static char fname[18];
        sprintf(fname, "NetHack %d.%d.%d", VERSION_MAJOR, VERSION_MINOR,
                PATCHLEVEL);
        NewHackScreen.DefaultTitle = fname;
    }
#if 0
    NewHackScreen.BlockPen = C_BLACK;
    NewHackScreen.DetailPen = C_WHITE;
#endif
#ifdef INTUI_NEW_LOOK
    if (IntuitionBase->LibNode.lib_Version >= 37) {
        int i;
        struct DimensionInfo dims;
        DisplayInfoHandle handle;
        struct DisplayInfo disp;
        ULONG modeid = DEFAULT_MONITOR_ID | HIRES_KEY;

        NewHackScreen.Width = STDSCREENWIDTH;
        NewHackScreen.Height = STDSCREENHEIGHT;

#ifdef HACKFONT
        if (TextsFont) {
            NewHackScreen.Font = &TextsFont13;
        }
#endif

        if (amii_scrnmode == 0xffffffff) {
            struct ScreenModeRequester *SMR;
#ifdef __GNUC__
            SM_FilterHook.h_Entry = (void *) &SM_Filter;
#else
            SM_FilterHook.h_Entry = (ULONG (*) ()) SM_Filter;
#endif
            SM_FilterHook.h_Data = 0;
            SM_FilterHook.h_SubEntry = 0;
            SMR = AllocAslRequest(ASL_ScreenModeRequest, NULL);
            if (AslRequestTags(SMR, ASLSM_FilterFunc, (ULONG) &SM_FilterHook,
                               TAG_END))
                amii_scrnmode = SMR->sm_DisplayID;
            else
                amii_scrnmode = 0;
            FreeAslRequest(SMR);
        }

        if (forcenobig == 0) {
            if ((wbscr = LockPubScreen("Workbench")) != NULL
                || (wbscr = LockPubScreen(NULL)) != NULL) {
                /* Get the default pub screen's size */
                modeid = GetVPModeID(&wbscr->ViewPort);
                if (modeid == INVALID_ID || ModeNotAvailable(modeid)
                    || (handle = FindDisplayInfo(modeid)) == NULL
                    || GetDisplayInfoData(handle, (char *) &dims,
                                          sizeof(dims), DTAG_DIMS,
                                          modeid) <= 0
                    || GetDisplayInfoData(handle, (char *) &disp,
                                          sizeof(disp), DTAG_DISP,
                                          modeid) <= 0) {
                    modeid = DEFAULT_MONITOR_ID | HIRES_KEY;
                    /* If the display database seems to not work, use the
                     * screen
                     * dimensions
                     */
                    NewHackScreen.Height = wbscr->Height;
                    NewHackScreen.Width = wbscr->Width;

                    /*
                     * Request LACE if it looks laced.  For 2.1/3.0, we will
                     * get
                     * promoted to the users choice of modes (if promotion is
                     * allowed)
                     * If the user is using a dragable screen, things will get
                     * hosed
                     * but that is life...
                     */
                    if (wbscr->ViewPort.Modes & LACE)
                        NewHackScreen.ViewModes |= LACE;
                    modeid = -1;
                } else {
                    /* Use the display database to get the correct information
                     */
                    if (disp.PropertyFlags & DIPF_IS_LACE)
                        NewHackScreen.ViewModes |= LACE;
                    NewHackScreen.Height = dims.StdOScan.MaxY + 1;
                    NewHackScreen.Width = dims.StdOScan.MaxX + 1;
                }
                NewHackScreen.TopEdge = 0;
                NewHackScreen.LeftEdge = 0;

                UnlockPubScreen(NULL, wbscr);
            }
        }

        for (i = 0; scrntags[i].ti_Tag != TAG_DONE; ++i) {
            switch (scrntags[i].ti_Tag) {
            case SA_DisplayID:
                if (!amii_scrnmode || ModeNotAvailable(amii_scrnmode)) {
                    if (ModeNotAvailable(modeid)) {
                        scrntags[i].ti_Tag = TAG_IGNORE;
                        break;
                    } else
                        scrntags[i].ti_Data = (long) modeid;
                } else
                    modeid = scrntags[i].ti_Data = (long) amii_scrnmode;
                if ((handle = FindDisplayInfo(modeid)) != NULL
                    && GetDisplayInfoData(handle, (char *) &dims,
                                          sizeof(dims), DTAG_DIMS, modeid) > 0
                    && GetDisplayInfoData(handle, (char *) &disp,
                                          sizeof(disp), DTAG_DISP,
                                          modeid) > 0) {
                    if (disp.PropertyFlags & DIPF_IS_LACE)
                        NewHackScreen.ViewModes |= LACE;
                    NewHackScreen.Height = dims.StdOScan.MaxY + 1;
                    NewHackScreen.Width = dims.StdOScan.MaxX + 1;
                }
                break;

            case SA_Pens:
                scrntags[i].ti_Data = (long) sysflags.amii_dripens;
                break;
            }
        }
    }
#endif

    if (WINVERS_AMIV)
        amii_bmhd = ReadTileImageFiles();
    else
        memcpy(amii_initmap, amii_init_map, sizeof(amii_initmap));
    memcpy(sysflags.amii_curmap, amii_initmap, sizeof(sysflags.amii_curmap));

    /* Find out how deep the screen needs to be, 32 planes is enough! */
    for (i = 0; i < 32; ++i) {
        if ((1L << i) >= amii_numcolors)
            break;
    }

    NewHackScreen.Depth = i;

    /* If for some reason Height/Width became smaller than the required,
        have the required one */
    if (NewHackScreen.Height < SCREENHEIGHT)
        NewHackScreen.Height = SCREENHEIGHT;
    if (NewHackScreen.Width < WIDTH)
        NewHackScreen.Width = WIDTH;
#ifdef HACKFONT
    i = max(TextsFont->tf_XSize, HackFont->tf_XSize);
    if (NewHackScreen.Width < 80 * i + 4)
        NewHackScreen.Width = 80 * i + 4;
#endif

    /* While openscreen fails try fewer colors to see if that is the problem.
     */
    while ((HackScreen = OpenScreen((void *) &NewHackScreen)) == NULL) {
#ifdef TEXTCOLOR
        if (--NewHackScreen.Depth < 3)
#else
        if (--NewHackScreen.Depth < 2)
#endif
            Abort(AN_OpenScreen & ~AT_DeadEnd);
    }
    amii_numcolors = 1L << NewHackScreen.Depth;
    if (HackScreen->Height > 300 && forcenobig == 0)
        bigscreen = 1;
    else
        bigscreen = 0;

#ifdef INTUI_NEW_LOOK
    if (IntuitionBase->LibNode.lib_Version >= 37)
        PubScreenStatus(HackScreen, 0);
#endif

    amiIDisplay->ypix = HackScreen->Height;
    amiIDisplay->xpix = HackScreen->Width;

    LoadRGB4(&HackScreen->ViewPort, sysflags.amii_curmap, amii_numcolors);

    VisualInfo = GetVisualInfo(HackScreen, TAG_END);
    MenuStrip = CreateMenus(GTHackMenu, TAG_END);
    LayoutMenus(MenuStrip, VisualInfo, GTMN_NewLookMenus, TRUE, TAG_END);

    /* Display the copyright etc... */

    if (WIN_BASE == WIN_ERR)
        WIN_BASE = amii_create_nhwindow(NHW_BASE);
    amii_clear_nhwindow(WIN_BASE);
    amii_putstr(WIN_BASE, 0, "");
    amii_putstr(WIN_BASE, 0, "");
    amii_putstr(WIN_BASE, 0, "");
    amii_putstr(WIN_BASE, 0, COPYRIGHT_BANNER_A);
    amii_putstr(WIN_BASE, 0, COPYRIGHT_BANNER_B);
    amii_putstr(WIN_BASE, 0, COPYRIGHT_BANNER_C);
    amii_putstr(WIN_BASE, 0, COPYRIGHT_BANNER_D);
    amii_putstr(WIN_BASE, 0, "");

    Initialized = 1;
}

void
amii_sethipens(struct Window *w, int type, int attr)
{
    switch (type) {
    default:
        SetAPen(w->RPort, attr ? C_RED : amii_otherAPen);
        SetBPen(w->RPort, C_BLACK);
        break;
    case NHW_STATUS:
        SetAPen(w->RPort, attr ? C_WHITE : amii_statAPen);
        SetBPen(w->RPort, amii_statBPen);
        break;
    case NHW_MESSAGE:
        SetAPen(w->RPort, attr ? C_WHITE : amii_msgAPen);
        SetBPen(w->RPort, amii_msgBPen);
        break;
    case NHW_MENU:
        SetAPen(w->RPort, attr ? C_BLACK : amii_menuAPen);
        SetBPen(w->RPort, amii_menuBPen);
        break;
    case NHW_TEXT:
        SetAPen(w->RPort, attr ? C_BLACK : amii_textAPen);
        SetBPen(w->RPort, amii_textBPen);
    case -2:
        SetBPen(w->RPort, amii_otherBPen);
        SetAPen(w->RPort, attr ? C_RED : amii_otherAPen);
        break;
    }
}

void
amii_setfillpens(struct Window *w, int type)
{
    switch (type) {
    case NHW_MESSAGE:
        SetAPen(w->RPort, amii_msgBPen);
        SetBPen(w->RPort, amii_msgBPen);
        break;
    case NHW_STATUS:
        SetAPen(w->RPort, amii_statBPen);
        SetBPen(w->RPort, amii_statBPen);
        break;
    case NHW_MENU:
        SetAPen(w->RPort, amii_menuBPen);
        SetBPen(w->RPort, amii_menuBPen);
        break;
    case NHW_TEXT:
        SetAPen(w->RPort, amii_textBPen);
        SetBPen(w->RPort, amii_textBPen);
        break;
    case NHW_MAP:
    case NHW_BASE:
    case NHW_OVER:
    default:
        SetAPen(w->RPort, C_BLACK);
        SetBPen(w->RPort, C_BLACK);
        break;
    case -2:
        SetAPen(w->RPort, amii_otherBPen);
        SetBPen(w->RPort, amii_otherBPen);
        break;
    }
}

void
amii_setdrawpens(struct Window *w, int type)
{
    switch (type) {
    case NHW_MESSAGE:
        SetAPen(w->RPort, amii_msgAPen);
        SetBPen(w->RPort, amii_msgBPen);
        break;
    case NHW_STATUS:
        SetAPen(w->RPort, amii_statAPen);
        SetBPen(w->RPort, amii_statBPen);
        break;
    case NHW_MENU:
        SetAPen(w->RPort, amii_menuAPen);
        SetBPen(w->RPort, amii_menuBPen);
        break;
    case NHW_TEXT:
        SetAPen(w->RPort, amii_textAPen);
        SetBPen(w->RPort, amii_textBPen);
        break;
    case NHW_MAP:
    case NHW_BASE:
    case NHW_OVER:
        SetAPen(w->RPort, C_WHITE);
        SetBPen(w->RPort, C_BLACK);
        break;
    default:
        SetAPen(w->RPort, amii_otherAPen);
        SetBPen(w->RPort, amii_otherBPen);
        break;
    }
}

/* Clear the indicated window */

void
amii_clear_nhwindow(win)
register winid win;
{
    register struct amii_WinDesc *cw;
    register struct Window *w;

    if (reclip == 2)
        return;

    if (win == WIN_ERR || (cw = amii_wins[win]) == NULL)
        panic(winpanicstr, win, "clear_nhwindow");

    /* Clear the overview window too if it is displayed */
    if (WINVERS_AMIV
        && (cw->type == WIN_MAP && WIN_OVER != WIN_ERR && reclip == 0)) {
        amii_clear_nhwindow(WIN_OVER);
    }

    if (w = cw->win)
        SetDrMd(w->RPort, JAM2);
    else
        return;

    if ((cw->wflags & FLMAP_CURSUP)) {
        if (cw->type != NHW_MAP)
            cursor_off(win);
        else
            cw->wflags &= ~FLMAP_CURSUP;
    }

    amii_setfillpens(w, cw->type);
    SetDrMd(w->RPort, JAM2);

    if (cw->type == NHW_MENU || cw->type == NHW_TEXT) {
        RectFill(w->RPort, w->BorderLeft, w->BorderTop,
                 w->Width - w->BorderRight - 1,
                 w->Height - w->BorderBottom - 1);
    } else {
        if (cw->type == NHW_MESSAGE) {
            amii_curs(win, 1, 0);
            if (!scrollmsg)
                TextSpaces(w->RPort, cw->cols);
        } else {
            RectFill(w->RPort, w->BorderLeft, w->BorderTop,
                     w->Width - w->BorderRight - 1,
                     w->Height - w->BorderBottom - 1);
        }
    }

    cw->cury = 0;
    cw->curx = 0;
    amii_curs(win, 1, 0);
}

/* Dismiss the window from the screen */

void
dismiss_nhwindow(win)
register winid win;
{
    register struct Window *w;
    register struct amii_WinDesc *cw;

    if (win == WIN_ERR || (cw = amii_wins[win]) == NULL) {
        panic(winpanicstr, win, "dismiss_nhwindow");
    }

    w = cw->win;

    if (w) {
        /* All windows have this stuff attached to them. */
        if (cw->type == NHW_MAP || cw->type == NHW_OVER
            || cw->type == NHW_BASE || cw->type == NHW_MESSAGE
            || cw->type == NHW_STATUS) {
            ClearMenuStrip(w);
        }

        /* Save where user like inventory to appear */
        if (win == WIN_INVEN) {
            lastinvent.MinX = w->LeftEdge;
            lastinvent.MinY = w->TopEdge;
            lastinvent.MaxX = w->Width;
            lastinvent.MaxY = w->Height;
        }

        /* Close the window */
        CloseShWindow(w);
        cw->win = NULL;

        /* Free copy of NewWindow structure for TEXT/MENU windows. */
        if (cw->newwin)
            FreeNewWindow((void *) cw->newwin);
        cw->newwin = NULL;
    }
}

void
amii_exit_nhwindows(str)
const char *str;
{
    /* Seems strange to have to do this... but we need the BASE window
     * left behind...
     */
    kill_nhwindows(0);
    if (WINVERS_AMIV)
        FreeTileImageFiles();

    if (str) {
        raw_print(""); /* be sure we're not under the top margin */
        raw_print(str);
    }
}

void
amii_display_nhwindow(win, blocking)
winid win;
boolean blocking;
{
    menu_item *mip;
    int cnt;
    static int lastwin = -1;
    struct amii_WinDesc *cw;

    if (!Initialized)
        return;
    lastwin = win;

    if (win == WIN_ERR || (cw = amii_wins[win]) == NULL)
        panic(winpanicstr, win, "display_nhwindow");

    if (cw->type == NHW_MESSAGE)
        cw->wflags &= ~FLMAP_SKIP;

    if (cw->type == NHW_MESSAGE || cw->type == NHW_STATUS)
        return;

    if (WIN_MAP != WIN_ERR && amii_wins[WIN_MAP]) {
        flush_glyph_buffer(amii_wins[WIN_MAP]->win);
    }

    if (cw->type == NHW_MENU || cw->type == NHW_TEXT) {
        cnt = DoMenuScroll(win, blocking, PICK_ONE, &mip);
    } else if (cw->type == NHW_MAP) {
        amii_end_glyphout(win);
        /* Do more if it is time... */
        if (blocking == TRUE && amii_wins[WIN_MESSAGE]->curx) {
            outmore(amii_wins[WIN_MESSAGE]);
        }
    }
}

void
amii_curs(window, x, y)
winid window;
register int x, y; /* not xchar: perhaps xchar is unsigned and
              curx-x would be unsigned as well */
{
    register struct amii_WinDesc *cw;
    register struct Window *w;
    register struct RastPort *rp;

    if (window == WIN_ERR || (cw = amii_wins[window]) == NULL)
        panic(winpanicstr, window, "curs");
    if ((w = cw->win) == NULL) {
        if (cw->type == NHW_MENU || cw->type == NHW_TEXT)
            return;
        else
            panic("No window open yet in curs() for winid %d\n", window);
    }
    amiIDisplay->lastwin = window;

    /* Make sure x is within bounds */
    if (x > 0)
        --x; /* column 0 is never used */
    else
        x = 0;

    cw->curx = x;
    cw->cury = y;

#ifdef DEBUG
    if (x < 0 || y < 0 || y >= cw->rows || x >= cw->cols) {
        char *s = "[unknown type]";
        switch (cw->type) {
        case NHW_MESSAGE:
            s = "[topl window]";
            break;
        case NHW_STATUS:
            s = "[status window]";
            break;
        case NHW_MAP:
            s = "[map window]";
            break;
        case NHW_MENU:
            s = "[menu window]";
            break;
        case NHW_TEXT:
            s = "[text window]";
            break;
        case NHW_BASE:
            s = "[base window]";
            break;
        case NHW_OVER:
            s = "[overview window]";
            break;
        }
        impossible("bad curs positioning win %d %s (%d,%d)", window, s, x, y);
        return;
    }
#endif

#ifdef CLIPPING
    if (clipping && cw->type == NHW_MAP) {
        x -= clipx;
        y -= clipy;
    }
#endif

    /* Output all saved output before doing cursor movements for MAP */

    if (cw->type == NHW_MAP) {
        flush_glyph_buffer(w);
    }

    /* Actually do it */

    rp = w->RPort;
    if (cw->type == NHW_MENU) {
        if (WINVERS_AMIV) {
            if (window == WIN_INVEN) {
                Move(rp, (x * rp->TxWidth) + w->BorderLeft + 1
                             + pictdata.xsize + 4,
                     (y * max(rp->TxHeight, pictdata.ysize + 3))
                         + rp->TxBaseline + pictdata.ysize - rp->TxHeight
                         + w->BorderTop + 4);
            } else {
                Move(rp, (x * rp->TxWidth) + w->BorderLeft + 1,
                     (y * rp->TxHeight) + rp->TxBaseline + w->BorderTop + 1);
            }
        } else {
            Move(rp, (x * rp->TxWidth) + w->BorderLeft + 1,
                 (y * rp->TxHeight) + rp->TxBaseline + w->BorderTop + 1);
        }
    } else if (cw->type == NHW_TEXT) {
        Move(rp, (x * rp->TxWidth) + w->BorderLeft + 1,
             (y * rp->TxHeight) + rp->TxBaseline + w->BorderTop + 1);
    } else if (cw->type == NHW_MAP || cw->type == NHW_BASE) {
        /* These coordinate calculations must be synced with those
         * in flush_glyph_buffer() in winchar.c.  curs_on_u() will
         * use this code, all other drawing occurs through the glyph
         * code.  In order for the cursor to appear on top of the hero,
         * the code must compute X,Y in the same manner relative to
         * the RastPort coordinates.
         *
         * y = w->BorderTop + (g_nodes[i].y-2) * rp->TxHeight +
         *   rp->TxBaseline + 1;
         * x = g_nodes[i].x * rp->TxWidth + w->BorderLeft;
         */

        if (WINVERS_AMIV) {
            if (cw->type == NHW_MAP) {
                if (Is_rogue_level(&u.uz)) {
#if 0
int qqx= (x * w->RPort->TxWidth) + w->BorderLeft;
int qqy= w->BorderTop + ( (y+1) * w->RPort->TxHeight ) + 1;
printf("pos: (%d,%d)->(%d,%d)\n",x,y,qqx,qqy);
#endif
                    SetAPen(w->RPort,
                            C_WHITE); /* XXX should be elsewhere (was 4)*/
                    Move(rp, (x * w->RPort->TxWidth) + w->BorderLeft,
                         w->BorderTop + ((y + 1) * w->RPort->TxHeight) + 1);
                } else {
                    Move(rp, (x * mxsize) + w->BorderLeft,
                         w->BorderTop + ((y + 1) * mysize) + 1);
                }
            } else {
                Move(rp, (x * w->RPort->TxWidth) + w->BorderLeft,
                     w->BorderTop + ((y + 1) * w->RPort->TxHeight)
                         + w->RPort->TxBaseline + 1);
            }
        } else {
            Move(rp, (x * w->RPort->TxWidth) + w->BorderLeft,
                 w->BorderTop + (y * w->RPort->TxHeight)
                     + w->RPort->TxBaseline + 1);
        }
    } else if (WINVERS_AMIV && cw->type == NHW_OVER) {
        Move(rp, (x * w->RPort->TxWidth) + w->BorderLeft + 2,
             w->BorderTop + w->RPort->TxBaseline + 3);
    } else if (cw->type == NHW_MESSAGE && !scrollmsg) {
        Move(rp, (x * w->RPort->TxWidth) + w->BorderLeft + 2,
             w->BorderTop + w->RPort->TxBaseline + 3);
    } else if (cw->type == NHW_STATUS) {
        Move(rp, (x * w->RPort->TxWidth) + w->BorderLeft + 2,
             (y * (w->RPort->TxHeight + 1)) + w->BorderTop
                 + w->RPort->TxBaseline + 1);
    } else {
        Move(rp, (x * w->RPort->TxWidth) + w->BorderLeft + 2,
             (y * w->RPort->TxHeight) + w->BorderTop + w->RPort->TxBaseline
                 + 1);
    }
}

void
amii_set_text_font(name, size)
char *name;
int size;
{
    register int i;
    register struct amii_WinDesc *cw;
    int osize = TextsFont13.ta_YSize;
    static char nname[100];

    strncpy(nname, name, sizeof(nname) - 1);
    nname[sizeof(nname) - 1] = 0;

    TextsFont13.ta_Name = nname;
    TextsFont13.ta_YSize = size;

    /* No alternate text font allowed for 640x269 or smaller */
    if (!HackScreen || !bigscreen)
        return;

    /* Look for windows to set, and change them */

    if (DiskfontBase = OpenLibrary("diskfont.library", amii_libvers)) {
        TextsFont = OpenDiskFont(&TextsFont13);
        for (i = 0; TextsFont && i < MAXWIN; ++i) {
            if ((cw = amii_wins[i]) && cw->win != NULL) {
                switch (cw->type) {
                case NHW_STATUS:
                    MoveWindow(cw->win, 0, -(size - osize) * 2);
                    SizeWindow(cw->win, 0, (size - osize) * 2);
                    SetFont(cw->win->RPort, TextsFont);
                    break;
                case NHW_MESSAGE:
                case NHW_MAP:
                case NHW_BASE:
                case NHW_OVER:
                    SetFont(cw->win->RPort, TextsFont);
                    break;
                }
            }
        }
    }
    CloseLibrary(DiskfontBase);
    DiskfontBase = NULL;
}

void
kill_nhwindows(all)
register int all;
{
    register int i;
    register struct amii_WinDesc *cw;

    /* Foreach open window in all of amii_wins[], CloseShWindow, free memory
     */

    for (i = 0; i < MAXWIN; ++i) {
        if ((cw = amii_wins[i]) && (cw->type != NHW_BASE || all)) {
            amii_destroy_nhwindow(i);
        }
    }
}

void
amii_cl_end(cw, curs_pos)
register struct amii_WinDesc *cw;
register int curs_pos;
{
    register struct Window *w = cw->win;
    register int oy, ox;

    if (!w)
        panic("NULL window pointer in amii_cl_end()");

    oy = w->RPort->cp_y;
    ox = w->RPort->cp_x;

    TextSpaces(w->RPort, cw->cols - curs_pos);

    Move(w->RPort, ox, oy);
}

void
cursor_off(window)
winid window;
{
    register struct amii_WinDesc *cw;
    register struct Window *w;
    register struct RastPort *rp;
    int curx, cury;
    int x, y;
    long dmode;
    short apen, bpen;
    unsigned char ch;

    if (window == WIN_ERR || (cw = amii_wins[window]) == NULL) {
        iflags.window_inited = 0;
        panic(winpanicstr, window, "cursor_off");
    }

    if (!(cw->wflags & FLMAP_CURSUP))
        return;

    w = cw->win;

    if (!w)
        return;

    cw->wflags &= ~FLMAP_CURSUP;
    rp = w->RPort;

    /* Save the current information */
    curx = rp->cp_x;
    cury = rp->cp_y;
    x = cw->cursx;
    y = cw->cursy;
    dmode = rp->DrawMode;
    apen = rp->FgPen;
    bpen = rp->BgPen;
    SetAPen(rp, cw->curs_apen);
    SetBPen(rp, cw->curs_bpen);
    SetDrMd(rp, COMPLEMENT);
    /*printf("CURSOR OFF: %d %d\n",x,y);*/

    if (WINVERS_AMIV && cw->type == NHW_MAP) {
        cursor_common(rp, x, y);
        if (Is_rogue_level(&u.uz))
            Move(rp, curx, cury);
    } else {
        ch = CURSOR_CHAR;
        Move(rp, x, y);
        Text(rp, &ch, 1);

        /* Put back the other stuff */

        Move(rp, curx, cury);
    }
    SetDrMd(rp, dmode);
    SetAPen(rp, apen);
    SetBPen(rp, bpen);
}

void
cursor_on(window)
winid window;
{
    int x, y;
    register struct amii_WinDesc *cw;
    register struct Window *w;
    register struct RastPort *rp;
    unsigned char ch;
    long dmode;
    short apen, bpen;

    if (window == WIN_ERR || (cw = amii_wins[window]) == NULL) {
        /* tty does this differently - is this OK? */
        iflags.window_inited = 0;
        panic(winpanicstr, window, "cursor_on");
    }

    /*printf("CURSOR ON: %d %d\n",cw->win->RPort->cp_x,
     * cw->win->RPort->cp_y);*/
    if ((cw->wflags & FLMAP_CURSUP))
        cursor_off(window);

    w = cw->win;

    if (!w)
        return;

    cw->wflags |= FLMAP_CURSUP;
    rp = w->RPort;

/* Save the current information */

#ifdef DISPMAP
    if (WINVERS_AMIV && cw->type == NHW_MAP && !Is_rogue_level(&u.uz))
        x = cw->cursx = (rp->cp_x & -8) + 8;
    else
#endif
        x = cw->cursx = rp->cp_x;
    y = cw->cursy = rp->cp_y;
    apen = rp->FgPen;
    bpen = rp->BgPen;
    dmode = rp->DrawMode;

    /* Draw in complement mode. The cursor body will be C_WHITE */

    cw->curs_apen = 0xff; /* Last color/all planes, regardless of depth */
    cw->curs_bpen = 0xff;
    SetAPen(rp, cw->curs_apen);
    SetBPen(rp, cw->curs_bpen);
    SetDrMd(rp, COMPLEMENT);
    if (WINVERS_AMIV && cw->type == NHW_MAP) {
        cursor_common(rp, x, y);
    } else {
        Move(rp, x, y);
        ch = CURSOR_CHAR;
        Text(rp, &ch, 1);
        Move(rp, x, y);
    }

    SetDrMd(rp, dmode);
    SetAPen(rp, apen);
    SetBPen(rp, bpen);
}

static void
cursor_common(rp, x, y)
struct RastPort *rp;
int x, y;
{
    int x1, x2, y1, y2;

    if (Is_rogue_level(&u.uz)) {
        x1 = x - 2;
        y1 = y - rp->TxHeight;
        x2 = x + rp->TxWidth + 1;
        y2 = y + 3;
        /*printf("COMM: (%d %d) (%d %d)  (%d %d) (%d
         * %d)\n",x1,y1,x2,y2,x1+2,y1+2,x2-2,y2-2);*/
    } else {
        x1 = x;
        y1 = y - mysize - 1;
        x2 = x + mxsize - 1;
        y2 = y - 2;
        RectFill(rp, x1, y1, x2, y2);
    }

    RectFill(rp, x1 + 2, y1 + 2, x2 - 2, y2 - 2);
}

void
amii_suspend_nhwindows(str)
const char *str;
{
    if (HackScreen)
        ScreenToBack(HackScreen);
}

void
amii_resume_nhwindows()
{
    if (HackScreen)
        ScreenToFront(HackScreen);
}

void
amii_bell()
{
    DisplayBeep(NULL);
}

void
removetopl(cnt)
int cnt;
{
    struct amii_WinDesc *cw = amii_wins[WIN_MESSAGE];
    /* NB - this is sufficient for
     * yn_function, but that's it
     */
    if (cw->curx < cnt)
        cw->curx = 0;
    else
        cw->curx -= cnt;

    amii_curs(WIN_MESSAGE, cw->curx + 1, cw->cury);
    amii_cl_end(cw, cw->curx);
}
/*#endif /* AMIGA_INTUITION */

#ifdef PORT_HELP
void
port_help()
{
    display_file(PORT_HELP, 1);
}
#endif

/*
 *  print_glyph
 *
 *  Print the glyph to the output device.  Don't flush the output device.
 *
 *  Since this is only called from show_glyph(), it is assumed that the
 *  position and glyph are always correct (checked there)!
 */

void
amii_print_glyph(win, x, y, glyph, bkglyph)
winid win;
xchar x, y;
int glyph, bkglyph;
{
    struct amii_WinDesc *cw;
    uchar ch;
    int color, och;
    extern const int zapcolors[];
    unsigned special;

    /* In order for the overview window to work, we can not clip here */
    if (!WINVERS_AMIV) {
#ifdef CLIPPING
        /* If point not in visible part of window just skip it */
        if (clipping) {
            if (x <= clipx || y < clipy || x >= clipxmax || y >= clipymax)
                return;
        }
#endif
    }

    if (win == WIN_ERR || (cw = amii_wins[win]) == NULL
        || cw->type != NHW_MAP) {
        panic(winpanicstr, win, "amii_print_glyph");
    }

#if 0
{
static int x=-1;
if(u.uz.dlevel != x){
 fprintf(stderr,"lvlchg: %d (%d)\n",u.uz.dlevel,Is_rogue_level(&u.uz));
 x = u.uz.dlevel;
}
}
#endif
    if (WINVERS_AMIV && !Is_rogue_level(&u.uz)) {
        amii_curs(win, x, y);
        amiga_print_glyph(win, 0, glyph);
    } else /* AMII, or Rogue level in either version */
    {
        /* map glyph to character and color */
        (void) mapglyph(glyph, &och, &color, &special, x, y);
        ch = (uchar) och;
        if (WINVERS_AMIV) { /* implies Rogue level here */
            amii_curs(win, x, y);
            amiga_print_glyph(win, NO_COLOR, ch + 10000);
        } else {
            /* Move the cursor. */
            amii_curs(win, x, y + 2);

#ifdef TEXTCOLOR
            /* Turn off color if rogue level. */
            if (Is_rogue_level(&u.uz))
                color = NO_COLOR;

            amiga_print_glyph(win, color, ch);
#else
            g_putch(ch); /* print the character */
#endif
            cw->curx++; /* one character over */
        }
    }
}

/* Make sure the user sees a text string when no windowing is available */

void
amii_raw_print(s)
register const char *s;
{
    int argc = 0;

    if (!s)
        return;
    if (amiIDisplay)
        amiIDisplay->rawprint++;

    if (!Initialized) { /* Not yet screen open ... */
        puts(s);
        fflush(stdout);
        return;
    }

    if (Initialized == 0 && WIN_BASE == WIN_ERR)
        init_nhwindows(&argc, (char **) 0);

    if (amii_rawprwin != WIN_ERR)
        amii_putstr(amii_rawprwin, 0, s);
    else if (WIN_MAP != WIN_ERR && amii_wins[WIN_MAP])
        amii_putstr(WIN_MAP, 0, s);
    else if (WIN_BASE != WIN_ERR && amii_wins[WIN_BASE])
        amii_putstr(WIN_BASE, 0, s);
    else {
        puts(s);
        fflush(stdout);
    }
}

/* Make sure the user sees a bold text string when no windowing
 * is available
 */

void
amii_raw_print_bold(s)
register const char *s;
{
    int argc = 0;

    if (!s)
        return;

    if (amiIDisplay)
        amiIDisplay->rawprint++;

    if (!Initialized) { /* Not yet screen open ... */
        puts(s);
        fflush(stdout);
        return;
    }

    if (Initialized == 0 && WIN_BASE == WIN_ERR)
        init_nhwindows(&argc, (char **) 0);

    if (amii_rawprwin != WIN_ERR)
        amii_putstr(amii_rawprwin, 1, s);
    else if (WIN_MAP != WIN_ERR && amii_wins[WIN_MAP])
        amii_putstr(WIN_MAP, 1, s);
    else if (WIN_BASE != WIN_ERR && amii_wins[WIN_BASE])
        amii_putstr(WIN_BASE, 1, s);
    else {
        printf("\33[1m%s\33[0m\n", s);
        fflush(stdout);
    }
}

/* Rebuild/update the inventory if the window is up.
 */
void
amii_update_inventory()
{
    register struct amii_WinDesc *cw;

    if (WIN_INVEN != WIN_ERR && (cw = amii_wins[WIN_INVEN])
        && cw->type == NHW_MENU && cw->win) {
        display_inventory(NULL, FALSE);
    }
}

/* Humm, doesn't really do anything useful */

void
amii_mark_synch()
{
    if (!amiIDisplay)
        fflush(stderr);
    /* anything else?  do we need this much? */
}

/* Wait for everything to sync.  Nothing is asynchronous, so we just
 * ask for a key to be pressed.
 */
void
amii_wait_synch()
{
    if (!amiIDisplay || amiIDisplay->rawprint) {
        if (amiIDisplay)
            amiIDisplay->rawprint = 0;
    } else {
        if (WIN_MAP != WIN_ERR) {
            display_nhwindow(WIN_MAP, TRUE);
            flush_glyph_buffer(amii_wins[WIN_MAP]->win);
        }
    }
}

void
amii_setclipped()
{
#ifdef CLIPPING
    clipping = TRUE;
    clipx = clipy = 0;
    clipxmax = CO;
    clipymax = LI;
/* some of this is now redundant with top of amii_cliparound XXX */
#endif
}

/* XXX still to do: suppress scrolling if we violate the boundary but the
 * edge of the map is already displayed
 */
void
amii_cliparound(x, y)
register int x, y;
{
    extern boolean restoring;
#ifdef CLIPPING
    int oldx = clipx, oldy = clipy;
    int oldxmax = clipxmax, oldymax = clipymax;
    int COx, LIx;
#define SCROLLCNT 1            /* Get there in 3 moves... */
    int scrollcnt = SCROLLCNT; /* ...or 1 if we changed level */
    if (!clipping) /* And 1 in anycase, cleaner, simpler, quicker */
        return;

    if (Is_rogue_level(&u.uz)) {
        struct Window *w = amii_wins[WIN_MAP]->win;
        struct RastPort *rp = w->RPort;

        COx = (w->Width - w->BorderLeft - w->BorderRight) / rp->TxWidth;
        LIx = (w->Height - w->BorderTop - w->BorderBottom) / rp->TxHeight;
    } else {
        COx = CO;
        LIx = LI;
    }
    /*
     * On a level change, move the clipping region so that for a
     * reasonablely large window extra motion is avoided; for
     * the rogue level hopefully this means no motion at all.
     */
    {
        static d_level saved_level = { 127, 127 }; /* XXX */

        if (!on_level(&u.uz, &saved_level)) {
            scrollcnt = 1; /* jump with blanking */
            clipx = clipy = 0;
            clipxmax = COx;
            clipymax = LIx;
            saved_level = u.uz; /* save as new current level */
        }
    }

    if (x <= clipx + xclipbord) {
        clipx = max(0, x - (clipxmax - clipx) / 2);
        clipxmax = clipx + COx;
    } else if (x > clipxmax - xclipbord) {
        clipxmax = min(COLNO, x + (clipxmax - clipx) / 2);
        clipx = clipxmax - COx;
    }

    if (y <= clipy + yclipbord) {
        clipy = max(0, y - (clipymax - clipy) / 2);
        clipymax = clipy + LIx;
    } else if (y > clipymax - yclipbord) {
        clipymax = min(ROWNO, y + (clipymax - clipy) / 2);
        clipy = clipymax - LIx;
    }

    reclip = 1;
    if (clipx != oldx || clipy != oldy || clipxmax != oldxmax
        || clipymax != oldymax) {
#ifndef NOSCROLLRASTER
        struct Window *w = amii_wins[WIN_MAP]->win;
        struct RastPort *rp = w->RPort;
        int xdelta, ydelta, xmod, ymod, i;
        int incx, incy, mincx, mincy;
        int savex, savey, savexmax, saveymax;
        int scrx, scry;

        if (Is_rogue_level(&u.uz)) {
            scrx = rp->TxWidth;
            scry = rp->TxHeight;
        } else {
            scrx = mxsize;
            scry = mysize;
        }

        /* Ask that the glyph routines not draw the overview window */
        reclip = 2;
        cursor_off(WIN_MAP);

        /* Compute how far we are moving in terms of tiles */
        mincx = clipx - oldx;
        mincy = clipy - oldy;

        /* How many tiles to get there in SCROLLCNT moves */
        incx = (clipx - oldx) / scrollcnt;
        incy = (clipy - oldy) / scrollcnt;

        /* If less than SCROLLCNT tiles, then move by 1 tile if moving at all
         */
        if (incx == 0)
            incx = (mincx != 0);
        if (incy == 0)
            incy = (mincy != 0);

        /* Get count of pixels to move each iteration and final pixel count */
        xdelta = ((clipx - oldx) * scrx) / scrollcnt;
        xmod = ((clipx - oldx) * scrx) % scrollcnt;
        ydelta = ((clipy - oldy) * scry) / scrollcnt;
        ymod = ((clipy - oldy) * scry) % scrollcnt;

        /* Preserve the final move location */
        savex = clipx;
        savey = clipy;
        saveymax = clipymax;
        savexmax = clipxmax;

/*
 * Set clipping rectangle to be just the region that will be exposed so
 * that drawing will be faster
 */
#if 0 /* Doesn't seem to work quite the way it should */
	/* In some cases hero is 'centered' offscreen */
	if( xdelta < 0 )
	{
	    clipx = oldx;
	    clipxmax = clipx + incx;
	}
	else if( xdelta > 0 )
	{
	    clipxmax = oldxmax;
	    clipx = clipxmax - incx;
	}
	else
	{
	    clipx = oldx;
	    clipxmax = oldxmax;
	}

	if( ydelta < 0 )
	{
	    clipy = oldy;
	    clipymax = clipy + incy;
	}
	else if( ydelta > 0 )
	{
	    clipymax = oldymax;
	    clipy = clipymax - incy;
	}
	else
	{
	    clipy = oldy;
	    clipymax = oldymax;
	}
#endif
        /* Now, in scrollcnt moves, move the picture toward the final view */
        for (i = 0; i < scrollcnt; ++i) {
#ifdef DISPMAP
            if (i == scrollcnt - 1 && (xmod != 0 || ymod != 0)
                && (xdelta != 0 || ydelta != 0)) {
                incx += (clipx - oldx) % scrollcnt;
                incy += (clipy - oldy) % scrollcnt;
                xdelta += xmod;
                ydelta += ymod;
            }
#endif
            /* Scroll the raster if we are scrolling */
            if (xdelta != 0 || ydelta != 0) {
                ScrollRaster(rp, xdelta, ydelta, w->BorderLeft, w->BorderTop,
                             w->Width - w->BorderRight - 1,
                             w->Height - w->BorderBottom - 1);

                if (mincx == 0)
                    incx = 0;
                else
                    mincx -= incx;

                clipx += incx;
                clipxmax += incx;

                if (mincy == 0)
                    incy = 0;
                else
                    mincy -= incy;

                clipy += incy;
                clipymax += incy;

                /* Draw the exposed portion */
                if (on_level(&u.uz0, &u.uz) && !restoring)
                    (void) doredraw();
                flush_glyph_buffer(amii_wins[WIN_MAP]->win);
            }
        }

        clipx = savex;
        clipy = savey;
        clipymax = saveymax;
        clipxmax = savexmax;
#endif
        if (on_level(&u.uz0, &u.uz) && !restoring && moves > 1)
            (void) doredraw();
        flush_glyph_buffer(amii_wins[WIN_MAP]->win);
    }
    reclip = 0;
#endif
}

void
flushIDCMP(port)
struct MsgPort *port;
{
    struct Message *msg;
    while (msg = GetMsg(port))
        ReplyMsg(msg);
}

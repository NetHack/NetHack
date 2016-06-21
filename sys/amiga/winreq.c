/* NetHack 3.6	winreq.c	$NHDT-Date: 1432512796 2015/05/25 00:13:16 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) Gregg Wonderly, Naperville, Illinois,  1991,1992,1993. */
/* NetHack may be freely redistributed.  See license for details. */

#include "NH:sys/amiga/windefs.h"
#include "NH:sys/amiga/winext.h"
#include "NH:sys/amiga/winproto.h"

#define GADBLUEPEN 2
#define GADREDPEN 3
#define GADGREENPEN 4
#define GADCOLOKAY 5
#define GADCOLCANCEL 6
#define GADCOLSAVE 7

UBYTE UNDOBUFFER[300];
SHORT BorderVectors1[] = { 0, 0, 57, 0, 57, 11, 0, 11, 0, 0 };
struct Border Border1 = { -1, -1, 3, 0, JAM1, 5, BorderVectors1, NULL };
struct IntuiText IText1 = { 3, 0, JAM1, 4, 1, NULL, (UBYTE *) "Cancel",
                            NULL };
struct Gadget Gadget2 = { NULL, 9, 15, 56, 10, NULL, RELVERIFY, BOOLGADGET,
                          (APTR) &Border1, NULL, &IText1, NULL, NULL, 1,
                          NULL };
UBYTE StrStringSIBuff[300];
struct StringInfo StrStringSInfo = { StrStringSIBuff, UNDOBUFFER, 0, 300, 0,
                                     0, 0, 0, 0, 0, 0, 0, NULL };
SHORT BorderVectors2[] = { 0, 0, 439, 0, 439, 11, 0, 11, 0, 0 };
struct Border Border2 = { -1, -1, 3, 0, JAM1, 5, BorderVectors2, NULL };
struct Gadget String = { &Gadget2, 77, 15, 438, 10, NULL,
                         RELVERIFY + STRINGCENTER, STRGADGET, (APTR) &Border2,
                         NULL, NULL, NULL, (APTR) &StrStringSInfo, 2, NULL };

#define StrString \
    ((char *) (((struct StringInfo *) (String.SpecialInfo))->Buffer))

struct NewWindow StrWindow = {
    57, 74, 526, 31, 0, 1, GADGETUP + CLOSEWINDOW + ACTIVEWINDOW + VANILLAKEY,
    WINDOWDRAG + WINDOWDEPTH + WINDOWCLOSE + ACTIVATE + NOCAREREFRESH,
    &String, NULL, NULL, NULL, NULL, 5, 5, 0xffff, 0xffff, CUSTOMSCREEN
};

#include "NH:sys/amiga/colorwin.c"

#define XSIZE 2
#define YSIZE 3
#define XCLIP 4
#define YCLIP 5
#define GADOKAY 6
#define GADCANCEL 7

#include "NH:sys/amiga/clipwin.c"

void ClearCol(struct Window *w);

void
EditColor()
{
    extern char configfile[];
    int i, done = 0, okay = 0;
    long code, qual, class;
    register struct Gadget *gd, *dgad;
    register struct Window *nw;
    register struct IntuiMessage *imsg;
    register struct PropInfo *pip;
    register struct Screen *scrn;
    long aidx;
    int msx, msy;
    int curcol = 0, drag = 0;
    int bxorx, bxory, bxxlen, bxylen;
    static UWORD colors[AMII_MAXCOLORS];
    static UWORD svcolors[AMII_MAXCOLORS];
    static int once = 0;
    scrn = HackScreen;

    if (!once) {
        if (WINVERS_AMIV) {
            Col_NewWindowStructure1.Width += 300;
            Col_NewWindowStructure1.Height += 20;
            Col_NewWindowStructure1.LeftEdge -= 150;
            Col_BluePen.Width += 300;
            Col_RedPen.Width += 300;
            Col_GreenPen.Width += 300;
            Col_Cancel.LeftEdge += 300;
            Col_Okay.LeftEdge += 150;
            Col_Cancel.TopEdge += 20;
            Col_Save.TopEdge += 20;
            Col_Okay.TopEdge += 20;
        }
        SetBorder(&Col_Okay);
        SetBorder(&Col_Cancel);
        SetBorder(&Col_Save);
        once = 1;
    }

    bxylen = Col_NewWindowStructure1.Height
             - (Col_BluePen.TopEdge + Col_BluePen.Height + 6);
    bxxlen = Col_BluePen.Width;
    bxorx = Col_BluePen.LeftEdge;
    bxory = Col_BluePen.TopEdge + Col_BluePen.Height + 2;

    /* Save the current colors */
    for (i = 0; i < amii_numcolors; ++i)
        svcolors[i] = colors[i] = GetRGB4(scrn->ViewPort.ColorMap, i);

    Col_NewWindowStructure1.Screen = scrn;
#ifdef INTUI_NEW_LOOK
    if (IntuitionBase->LibNode.lib_Version >= 37) {
        ((struct PropInfo *) Col_BluePen.SpecialInfo)->Flags |= PROPNEWLOOK;
        ((struct PropInfo *) Col_RedPen.SpecialInfo)->Flags |= PROPNEWLOOK;
        ((struct PropInfo *) Col_GreenPen.SpecialInfo)->Flags |= PROPNEWLOOK;
    }
#endif
    if (WINVERS_AMIV || WINVERS_AMII) {
#ifdef INTUI_NEW_LOOK
        Col_NewWindowStructure1.Extension = wintags;
        Col_NewWindowStructure1.Flags |= WFLG_NW_EXTENDED;
#ifdef __GNUC__
        fillhook.h_Entry = (void *) &LayerFillHook;
#else
        fillhook.h_Entry = (ULONG (*) ()) LayerFillHook;
#endif
        fillhook.h_Data = (void *) -2;
        fillhook.h_SubEntry = 0;
#endif
    }

    nw = OpenWindow((void *) &Col_NewWindowStructure1);

    if (nw == NULL) {
        DisplayBeep(NULL);
        return;
    }

    PrintIText(nw->RPort, &Col_IntuiTextList1, 0, 0);

    ClearCol(nw);
    DrawCol(nw, curcol, colors);
    while (!done) {
        WaitPort(nw->UserPort);

        while (imsg = (struct IntuiMessage *) GetMsg(nw->UserPort)) {
            gd = (struct Gadget *) imsg->IAddress;
            code = imsg->Code;
            class = imsg->Class;
            qual = imsg->Qualifier;
            msx = imsg->MouseX;
            msy = imsg->MouseY;

            ReplyMsg((struct Message *) imsg);

            switch (class) {
            case VANILLAKEY:
                if (code == 'v' && qual == AMIGALEFT)
                    okay = done = 1;
                else if (code == 'b' && qual == AMIGALEFT)
                    okay = 0, done = 1;
                else if (code == 'o' || code == 'O')
                    okay = done = 1;
                else if (code == 'c' || code == 'C')
                    okay = 0, done = 1;
                break;

            case CLOSEWINDOW:
                done = 1;
                break;

            case GADGETUP:
                drag = 0;
                if (gd->GadgetID == GADREDPEN || gd->GadgetID == GADBLUEPEN
                    || gd->GadgetID == GADGREENPEN) {
                    pip = (struct PropInfo *) gd->SpecialInfo;
                    aidx = pip->HorizPot / (MAXPOT / 15);
                    if (gd->GadgetID == GADREDPEN) {
                        colors[curcol] =
                            (colors[curcol] & ~0xf00) | (aidx << 8);
                        LoadRGB4(&scrn->ViewPort, colors, amii_numcolors);
                    } else if (gd->GadgetID == GADBLUEPEN) {
                        colors[curcol] = (colors[curcol] & ~0xf) | aidx;
                        LoadRGB4(&scrn->ViewPort, colors, amii_numcolors);
                    } else if (gd->GadgetID == GADGREENPEN) {
                        colors[curcol] =
                            (colors[curcol] & ~0x0f0) | (aidx << 4);
                        LoadRGB4(&scrn->ViewPort, colors, amii_numcolors);
                    }
                    DispCol(nw, curcol, colors);
                } else if (gd->GadgetID == GADCOLOKAY) {
                    done = 1;
                    okay = 1;
                } else if (gd->GadgetID == GADCOLSAVE) {
                    FILE *fp, *nfp;
                    char buf[300], nname[300], oname[300];
                    int once = 0;

                    fp = fopen(configfile, "r");
                    if (!fp) {
                        pline("can't find NetHack.cnf");
                        break;
                    }

                    strcpy(oname, dirname((char *) configfile));
                    if (oname[strlen(oname) - 1] != ':') {
                        sprintf(nname, "%s/New_NetHack.cnf", oname);
                        strcat(oname, "/");
                        strcat(oname, "Old_NetHack.cnf");
                    } else {
                        sprintf(nname, "%sNew_NetHack.cnf", oname);
                        strcat(oname, "Old_NetHack.cnf");
                    }

                    nfp = fopen(nname, "w");
                    if (!nfp) {
                        pline("can't write to New_NetHack.cnf");
                        fclose(fp);
                        break;
                    }
                    while (fgets(buf, sizeof(buf), fp)) {
                        if (strncmp(buf, "PENS=", 5) == 0) {
                            once = 1;
                            fputs("PENS=", nfp);
                            for (i = 0; i < amii_numcolors; ++i) {
                                fprintf(nfp, "%03x", colors[i]);
                                if ((i + 1) < amii_numcolors)
                                    putc('/', nfp);
                            }
                            putc('\n', nfp);
                        } else {
                            fputs(buf, nfp);
                        }
                    }

                    /* If none in the file yet, now write it */
                    if (!once) {
                        fputs("PENS=", nfp);
                        for (i = 0; i < amii_numcolors; ++i) {
                            fprintf(nfp, "%03x", colors[i]);
                            if ((i + 1) < amii_numcolors)
                                putc(',', nfp);
                        }
                        putc('\n', nfp);
                    }
                    fclose(fp);
                    fclose(nfp);
                    unlink(oname);
                    if (filecopy((char *) configfile, oname) == 0)
                        if (filecopy(nname, (char *) configfile) == 0)
                            unlink(nname);
                    done = 1;
                    okay = 1;
                } else if (gd->GadgetID == GADCOLCANCEL) {
                    done = 1;
                    okay = 0;
                }
                break;

            case GADGETDOWN:
                drag = 1;
                dgad = gd;
                break;

            case MOUSEMOVE:
                if (!drag)
                    break;
                pip = (struct PropInfo *) dgad->SpecialInfo;
                aidx = pip->HorizPot / (MAXPOT / 15);
                if (dgad->GadgetID == GADREDPEN) {
                    colors[curcol] = (colors[curcol] & ~0xf00) | (aidx << 8);
                    LoadRGB4(&scrn->ViewPort, colors, amii_numcolors);
                } else if (dgad->GadgetID == GADBLUEPEN) {
                    colors[curcol] = (colors[curcol] & ~0xf) | aidx;
                    LoadRGB4(&scrn->ViewPort, colors, amii_numcolors);
                } else if (dgad->GadgetID == GADGREENPEN) {
                    colors[curcol] = (colors[curcol] & ~0x0f0) | (aidx << 4);
                    LoadRGB4(&scrn->ViewPort, colors, amii_numcolors);
                }
                DispCol(nw, curcol, colors);
                break;

            case MOUSEBUTTONS:
                if (code == SELECTDOWN) {
                    if (msy > bxory && msy < bxory + bxylen - 1 && msx > bxorx
                        && msx < bxorx + bxxlen - 1) {
                        curcol = (msx - bxorx) / (bxxlen / amii_numcolors);
                        if (curcol >= 0 && curcol < amii_numcolors)
                            DrawCol(nw, curcol, colors);
                    }
                }
                break;
            }
        }
    }

    if (okay) {
        for (i = 0; i < (amii_numcolors); ++i)
            sysflags.amii_curmap[i] = colors[i];
        LoadRGB4(&scrn->ViewPort, sysflags.amii_curmap, amii_numcolors);
    } else
        LoadRGB4(&scrn->ViewPort, svcolors, amii_numcolors);
    CloseWindow(nw);
}

void
ShowClipValues(struct Window *nw)
{
    char buf[50];
    struct Gadget *gd;

    SetAPen(nw->RPort, 5);
    SetBPen(nw->RPort, amii_otherBPen);
    SetDrMd(nw->RPort, JAM2);

    sprintf(buf, "%d ", mxsize);
    gd = &ClipXSIZE;
    Move(nw->RPort, gd->LeftEdge + (nw->Width + gd->Width) + 8,
         gd->TopEdge + nw->RPort->TxBaseline);
    Text(nw->RPort, buf, strlen(buf));

    sprintf(buf, "%d ", mysize);
    gd = &ClipYSIZE;
    Move(nw->RPort, gd->LeftEdge + (nw->Width + gd->Width) + 8,
         gd->TopEdge + nw->RPort->TxBaseline);
    Text(nw->RPort, buf, strlen(buf));

    sprintf(buf, "%d ", xclipbord);
    gd = &ClipXCLIP;
    Move(nw->RPort, gd->LeftEdge + (nw->Width + gd->Width) + 8,
         gd->TopEdge + nw->RPort->TxBaseline);
    Text(nw->RPort, buf, strlen(buf));

    sprintf(buf, "%d ", yclipbord);
    gd = &ClipYCLIP;
    Move(nw->RPort, gd->LeftEdge + (nw->Width + gd->Width) + 8,
         gd->TopEdge + nw->RPort->TxBaseline);
    Text(nw->RPort, buf, strlen(buf));
}

void
EditClipping(void)
{
    int i;
    long mflags;
    static int sizes[] = { 8, 16, 20, 24, 28, 32, 36 };
    char buf[40];
    int done = 0, okay = 0;
    long code, qual, class;
    register struct Gadget *gd, *dgad;
    register struct Window *nw;
    register struct IntuiMessage *imsg;
    register struct PropInfo *pip;
    register struct Screen *scrn;
    long aidx;
    int lmxsize = mxsize, lmysize = mysize;
    int lxclipbord = xclipbord, lyclipbord = yclipbord;
    int msx, msy;
    int drag = 0;
    static int once = 0;

    scrn = HackScreen;

    if (!once) {
        SetBorder(&ClipOkay);
        SetBorder(&ClipCancel);
        once = 1;
    }
    ClipNewWindowStructure1.Screen = scrn;
#ifdef INTUI_NEW_LOOK
    if (IntuitionBase->LibNode.lib_Version >= 37) {
        ((struct PropInfo *) ClipXSIZE.SpecialInfo)->Flags |= PROPNEWLOOK;
        ((struct PropInfo *) ClipYSIZE.SpecialInfo)->Flags |= PROPNEWLOOK;
        ((struct PropInfo *) ClipXCLIP.SpecialInfo)->Flags |= PROPNEWLOOK;
        ((struct PropInfo *) ClipYCLIP.SpecialInfo)->Flags |= PROPNEWLOOK;
    }
#endif
    if (WINVERS_AMIV || WINVERS_AMII) {
#ifdef INTUI_NEW_LOOK
        ClipNewWindowStructure1.Extension = wintags;
        ClipNewWindowStructure1.Flags |= WFLG_NW_EXTENDED;
#ifdef __GNUC__
        fillhook.h_Entry = (void *) &LayerFillHook;
#else
        fillhook.h_Entry = (ULONG (*) ()) LayerFillHook;
#endif
        fillhook.h_Data = (void *) -2;
        fillhook.h_SubEntry = 0;
#endif
    }

    nw = OpenWindow((void *) &ClipNewWindowStructure1);

    if (nw == NULL) {
        DisplayBeep(NULL);
        return;
    }

    ShowClipValues(nw);
    mflags = AUTOKNOB | FREEHORIZ;
#ifdef INTUI_NEW_LOOK
    if (IntuitionBase->LibNode.lib_Version >= 37) {
        mflags |= PROPNEWLOOK;
    }
#endif

    for (i = 0; i < 7; ++i) {
        if (mxsize <= sizes[i])
            break;
    }
    NewModifyProp(&ClipXSIZE, nw, NULL, mflags, (i * MAXPOT) / 6, 0,
                  MAXPOT / 6, 0, 1);
    for (i = 0; i < 7; ++i) {
        if (mysize <= sizes[i])
            break;
    }
    NewModifyProp(&ClipYSIZE, nw, NULL, mflags, (i * MAXPOT) / 6, 0,
                  MAXPOT / 6, 0, 1);

    NewModifyProp(&ClipXCLIP, nw, NULL, mflags,
                  ((xclipbord - 2) * MAXPOT) / 6, 0, MAXPOT / 6, 0, 1);
    NewModifyProp(&ClipYCLIP, nw, NULL, mflags,
                  ((yclipbord - 2) * MAXPOT) / 6, 0, MAXPOT / 6, 0, 1);

    while (!done) {
        WaitPort(nw->UserPort);

        while (imsg = (struct IntuiMessage *) GetMsg(nw->UserPort)) {
            gd = (struct Gadget *) imsg->IAddress;
            code = imsg->Code;
            class = imsg->Class;
            qual = imsg->Qualifier;
            msx = imsg->MouseX;
            msy = imsg->MouseY;

            ReplyMsg((struct Message *) imsg);

            switch (class) {
            case VANILLAKEY:
                if (code == '\33')
                    okay = 0, done = 1;
                else if (code == 'v' && qual == AMIGALEFT)
                    okay = done = 1;
                else if (code == 'b' && qual == AMIGALEFT)
                    okay = 0, done = 1;
                else if (code == 'o' || code == 'O')
                    okay = done = 1;
                else if (code == 'c' || code == 'C')
                    okay = 0, done = 1;
                break;

            case CLOSEWINDOW:
                done = 1;
                break;

            case GADGETUP:
                drag = 0;
                if (gd->GadgetID == XSIZE || gd->GadgetID == YSIZE
                    || gd->GadgetID == XCLIP || gd->GadgetID == YCLIP) {
                    pip = (struct PropInfo *) gd->SpecialInfo;
                    aidx = pip->HorizPot / (MAXPOT / 6);
                    if (gd->GadgetID == XSIZE) {
                        mxsize = sizes[aidx];
                    } else if (gd->GadgetID == YSIZE) {
                        mysize = sizes[aidx];
                    } else if (gd->GadgetID == XCLIP) {
                        xclipbord = aidx + 2;
                    } else if (gd->GadgetID == YCLIP) {
                        yclipbord = aidx + 2;
                    }
                    ShowClipValues(nw);
#ifdef OPT_DISPMAP
                    dispmap_sanity();
#endif
                } else if (gd->GadgetID == GADOKAY) {
                    done = 1;
                    okay = 1;
                } else if (gd->GadgetID == GADCANCEL) {
                    done = 1;
                    okay = 0;
                }
                ReportMouse(0, nw);
                reclip = 2;
                doredraw();
                flush_glyph_buffer(amii_wins[WIN_MAP]->win);
                reclip = 0;
                break;

            case GADGETDOWN:
                drag = 1;
                dgad = gd;
                ReportMouse(1, nw);
                break;

            case MOUSEMOVE:
                if (!drag)
                    break;
                pip = (struct PropInfo *) dgad->SpecialInfo;
                aidx = pip->HorizPot / (MAXPOT / 6);
                Move(nw->RPort,
                     dgad->LeftEdge + (nw->Width + dgad->Width) + 8,
                     dgad->TopEdge + nw->RPort->TxBaseline);
                if (dgad->GadgetID == XSIZE) {
                    mxsize = sizes[aidx];
                    sprintf(buf, "%d ", lmxsize);
                } else if (dgad->GadgetID == YSIZE) {
                    mysize = sizes[aidx];
                    sprintf(buf, "%d ", mysize);
                } else if (dgad->GadgetID == XCLIP) {
                    xclipbord = aidx + 2;
                    sprintf(buf, "%d ", xclipbord);
                } else if (dgad->GadgetID == YCLIP) {
                    yclipbord = aidx + 2;
                    sprintf(buf, "%d ", yclipbord);
                }
                SetAPen(nw->RPort, 5);
                SetBPen(nw->RPort, amii_otherBPen);
                SetDrMd(nw->RPort, JAM2);
                Text(nw->RPort, buf, strlen(buf));
#ifdef OPT_DISPMAP
                dispmap_sanity();
#endif
                break;
            }
        }
    }

    CloseWindow(nw);

    /* Restore oldvalues if cancelled. */
    if (!okay) {
        mxsize = lmxsize;
        mysize = lmysize;
        xclipbord = lxclipbord;
        yclipbord = lyclipbord;
    }
}

char *
dirname(str)
char *str;
{
    char *t, c;
    static char dir[300];

    t = strrchr(str, '/');
    if (!t)
        t = strrchr(str, ':');
    if (!t)
        t = str;
    else {
        c = *t;
        *t = 0;
        strcpy(dir, str);
        *t = c;
    }
    return (dir);
}

char *
basename(str)
char *str;
{
    char *t;

    t = strrchr(str, '/');
    if (!t)
        t = strrchr(str, ':');
    if (!t)
        t = str;
    else
        ++t;
    return (t);
}

filecopy(from, to) char *from, *to;
{
    char *buf;
    int i = 0;

    buf = (char *) alloc(strlen(to) + strlen(from) + 20);
    if (buf) {
        sprintf(buf, "c:copy \"%s\" \"%s\" clone", from, to);

/* Check SysBase instead?  Shouldn't matter  */
#ifdef INTUI_NEW_LOOK
        if (IntuitionBase->LibNode.lib_Version >= 37)
            i = System(buf, NULL);
        else
#endif
            Execute(buf, NULL, NULL);
        free(buf);
    } else {
        return (-1);
    }
    return (i);
}

/* The colornames, and the default values for the pens */
static struct COLDEF {
    char *name, *defval;
};
struct COLDEF amii_colnames[AMII_MAXCOLORS] = {
    "Black", "(000)", "White",   "(fff)", "Brown", "(830)", "Cyan", "(7ac)",
    "Green", "(181)", "Magenta", "(c06)", "Blue",  "(23e)", "Red",  "(c00)",
};

struct COLDEF amiv_colnames[AMII_MAXCOLORS] = {
    "Black",     "(000)", "White",       "(fff)", "Cyan",        "(0bf)",
    "Orange",    "(f60)", "Blue",        "(00f)", "Green",       "(090)",
    "Grey",      "(69b)", "Red",         "(f00)", "Light Green", "(6f0)",
    "Yellow",    "(ff0)", "Magenta",     "(f0f)", "Brown",       "(940)",
    "Grey Blue", "(466)", "Light Brown", "(c40)", "Light Grey",  "(ddb)",
    "Peach",     "(fb9)", "Col 16",      "(222)", "Col 17",      "(eee)",
    "Col 18",    "(000)", "Col 19",      "(ccc)", "Col 20",      "(bbb)",
    "Col 21",    "(aaa)", "Col 22",      "(999)", "Col 23",      "(888)",
    "Col 24",    "(777)", "Col 25",      "(666)", "Col 26",      "(555)",
    "Col 27",    "(444)", "Col 28",      "(333)", "Col 29",      "(18f)",
    "Col 30",    "(f81)", "Col 31",      "(fff)",
};

void
ClearCol(struct Window *w)
{
    int bxorx, bxory, bxxlen, bxylen;
    int incx, incy;

    bxylen = Col_Okay.TopEdge - (Col_BluePen.TopEdge + Col_BluePen.Height) - 1
             - txheight - 3;
    bxxlen = Col_BluePen.Width - 2;
    bxorx = Col_BluePen.LeftEdge + 1;
    bxory = Col_BluePen.TopEdge + Col_BluePen.Height + 2;

    incx = bxxlen / amii_numcolors;
    incy = bxylen - 2;

    bxxlen /= incx;
    bxxlen *= incx;
    bxxlen += 2;

    SetAPen(w->RPort, C_WHITE);
    SetDrMd(w->RPort, JAM1);
    RectFill(w->RPort, bxorx, bxory, bxorx + bxxlen + 1, bxory + bxylen);

    SetAPen(w->RPort, C_BLACK);
    RectFill(w->RPort, bxorx + 1, bxory + 1, bxorx + bxxlen,
             bxory + bxylen - 1);
}

void
DrawCol(w, idx, colors)
struct Window *w;
int idx;
UWORD *colors;
{
    int bxorx, bxory, bxxlen, bxylen;
    int i, incx, incy, r, g, b;
    long mflags;

    bxylen = Col_Okay.TopEdge - (Col_BluePen.TopEdge + Col_BluePen.Height) - 1
             - txheight - 3;
    bxxlen = Col_BluePen.Width - 2;
    bxorx = Col_BluePen.LeftEdge + 1;
    bxory = Col_BluePen.TopEdge + Col_BluePen.Height + 2;

    incx = bxxlen / amii_numcolors;
    incy = bxylen - 2;

    bxxlen /= incx;
    bxxlen *= incx;
    bxxlen += 2;

    for (i = 0; i < amii_numcolors; ++i) {
        int x, y;
        x = bxorx + 2 + (i * incx);
        y = bxory + 2;

        if (i == idx) {
            SetAPen(w->RPort, sysflags.amii_dripens[SHADOWPEN]);
            Move(w->RPort, x, y + bxylen - 4);
            Draw(w->RPort, x, y);
            Draw(w->RPort, x + incx - 1, y);

            Move(w->RPort, x + 1, y + bxylen - 5);
            Draw(w->RPort, x + 1, y + 1);
            Draw(w->RPort, x + incx - 2, y + 1);

            SetAPen(w->RPort, sysflags.amii_dripens[SHINEPEN]);
            Move(w->RPort, x + incx - 1, y + 1);
            Draw(w->RPort, x + incx - 1, y + bxylen - 4);
            Draw(w->RPort, x, y + bxylen - 4);

            Move(w->RPort, x + incx - 2, y + 2);
            Draw(w->RPort, x + incx - 2, y + bxylen - 5);
            Draw(w->RPort, x + 1, y + bxylen - 5);
        } else {
            SetAPen(w->RPort, C_BLACK);
            Move(w->RPort, x, y);
            Draw(w->RPort, x + incx - 1, y);
            Draw(w->RPort, x + incx - 1, y + bxylen - 4);
            Draw(w->RPort, x, y + bxylen - 4);
            Draw(w->RPort, x, y);
            SetAPen(w->RPort, C_BLACK);
            Move(w->RPort, x + 1, y + 1);
            Draw(w->RPort, x + incx - 2, y + 1);
            Draw(w->RPort, x + incx - 2, y + bxylen - 6);
            Draw(w->RPort, x + 1, y + bxylen - 6);
            Draw(w->RPort, x + 1, y + 1);
        }

        SetAPen(w->RPort, i);
        RectFill(w->RPort, x + 3, y + 3, x + incx - 4, y + bxylen - 6);
    }

    DispCol(w, idx, colors);

    r = (colors[idx] & 0xf00) >> 8;
    g = (colors[idx] & 0x0f0) >> 4;
    b = colors[idx] & 0x00f;

    mflags = AUTOKNOB | FREEHORIZ;
#ifdef INTUI_NEW_LOOK
    if (IntuitionBase->LibNode.lib_Version >= 37) {
        mflags |= PROPNEWLOOK;
    }
#endif
    NewModifyProp(&Col_RedPen, w, NULL, mflags, (r * MAXPOT) / 15, 0,
                  MAXPOT / 15, 0, 1);
    NewModifyProp(&Col_GreenPen, w, NULL, mflags, (g * MAXPOT) / 15, 0,
                  MAXPOT / 15, 0, 1);
    NewModifyProp(&Col_BluePen, w, NULL, mflags, (b * MAXPOT) / 15, 0,
                  MAXPOT / 15, 0, 1);
}

void
DispCol(w, idx, colors)
struct Window *w;
int idx;
UWORD *colors;
{
    char buf[50];
    char *colname, *defval;

    if (WINVERS_AMIV) {
        colname = amiv_colnames[idx].name;
        defval = amiv_colnames[idx].defval;
    } else {
        colname = amii_colnames[idx].name;
        defval = amii_colnames[idx].defval;
    }

    if (colname == NULL) {
        colname = "unknown";
        defval = "unknown";
    }
    Move(w->RPort, Col_Save.LeftEdge, Col_Save.TopEdge - 7);
    sprintf(buf, "%s=%03x default=%s%s", colname, colors[idx], defval,
            "              " + strlen(colname) + 1);
    SetAPen(w->RPort, C_RED);
    SetBPen(w->RPort, amii_otherBPen);
    SetDrMd(w->RPort, JAM2);
    Text(w->RPort, buf, strlen(buf));
}

void
amii_setpens(int count)
{
#ifdef INTUI_NEW_LOOK
    struct EasyStruct ea = { sizeof(struct EasyStruct), 0l, "NetHack Request",
                             "Number of pens requested(%ld) not correct",
                             "Use default pens|Use requested pens" };
    struct EasyStruct ea2 = { sizeof(struct EasyStruct), 0l,
                              "NetHack Request",
                              "Number of pens requested(%ld) not\ncompatible "
                              "with game configuration(%ld)",
                              "Use default pens|Use requested pens" };
#endif
/* If the pens in amii_curmap are
 * more pens than in amii_numcolors, then we choose to ignore
 * those pens.
 */
#ifdef INTUI_NEW_LOOK
    if (IntuitionBase && IntuitionBase->LibNode.lib_Version >= 39) {
        if (count != amii_numcolors) {
            long args[2];
            args[0] = count;
            args[1] = amii_numcolors;
            if (EasyRequest(NULL, &ea2, NULL, args) == 1) {
                memcpy(sysflags.amii_curmap, amii_initmap,
                       amii_numcolors * sizeof(amii_initmap[0]));
            }
        }
    } else if (IntuitionBase && IntuitionBase->LibNode.lib_Version >= 37) {
        if (count != amii_numcolors) {
            if (EasyRequest(NULL, &ea, NULL, NULL) == 1) {
                memcpy(sysflags.amii_curmap, amii_initmap,
                       amii_numcolors * sizeof(amii_initmap[0]));
            }
        }
    } else
#endif
        if (count != amii_numcolors) {
        memcpy(sysflags.amii_curmap, amii_initmap,
               amii_numcolors * sizeof(amii_initmap[0]));
    }

    /* If the pens are set in NetHack.cnf, we can get called before
     * HackScreen has been opened.
     */
    if (HackScreen != NULL) {
        LoadRGB4(&HackScreen->ViewPort, sysflags.amii_curmap, amii_numcolors);
    }
}

/* Generate a requester for a string value. */

void
amii_getlin(prompt, bufp)
const char *prompt;
char *bufp;
{
    getlind(prompt, bufp, 0);
}

/* and with default */
void
getlind(prompt, bufp, dflt)
const char *prompt;
char *bufp;
const char *dflt;
{
#ifndef TOPL_GETLINE
    register struct Window *cwin;
    register struct IntuiMessage *imsg;
    register long class, code, qual;
    register int aredone = 0;
    register struct Gadget *gd;
    static int once;

    *StrString = 0;
    if (dflt)
        strcpy(StrString, dflt);
    StrWindow.Title = (UBYTE *) prompt;
    StrWindow.Screen = HackScreen;

    if (!once) {
        if (bigscreen) {
            StrWindow.LeftEdge =
                (HackScreen->Width / 2) - (StrWindow.Width / 2);
            if (amii_wins[WIN_MAP]) {
                StrWindow.TopEdge = amii_wins[WIN_MAP]->win->TopEdge;
            } else {
                StrWindow.TopEdge =
                    (HackScreen->Height / 2) - (StrWindow.Height / 2);
            }
        }
        SetBorder(&String);
        SetBorder(&Gadget2);
        once = 1;
    }

    if (WINVERS_AMIV || WINVERS_AMII) {
#ifdef INTUI_NEW_LOOK
        StrWindow.Extension = wintags;
        StrWindow.Flags |= WFLG_NW_EXTENDED;
#ifdef __GNUC__
        fillhook.h_Entry = (void *) &LayerFillHook;
#else
        fillhook.h_Entry = (ULONG (*) ()) LayerFillHook;
#endif
        fillhook.h_Data = (void *) -2;
        fillhook.h_SubEntry = 0;
#endif
    }

    if ((cwin = OpenWindow((void *) &StrWindow)) == NULL) {
        return;
    }

    while (!aredone) {
        WaitPort(cwin->UserPort);
        while ((imsg = (void *) GetMsg(cwin->UserPort)) != NULL) {
            class = imsg->Class;
            code = imsg->Code;
            qual = imsg->Qualifier;
            gd = (struct Gadget *) imsg->IAddress;

            switch (class) {
            case VANILLAKEY:
                if (code == '\033'
                    && (qual & (IEQUALIFIER_LALT | IEQUALIFIER_RALT
                                | IEQUALIFIER_LCOMMAND
                                | IEQUALIFIER_RCOMMAND)) == 0) {
                    if (bufp) {
                        bufp[0] = '\033';
                        bufp[1] = 0;
                    }
                    aredone = 1;
                } else {
                    ActivateGadget(&String, cwin, NULL);
                }
                break;

            case ACTIVEWINDOW:
                ActivateGadget(&String, cwin, NULL);
                break;

            case GADGETUP:
                switch (gd->GadgetID) {
                case 2:
                    aredone = 1;
                    if (bufp)
                        strcpy(bufp, StrString);
                    break;

                case 1:
                    if (bufp) {
                        bufp[0] = '\033';
                        bufp[1] = 0;
                    }
                    aredone = 1;
                    break;
                }
                break;

            case CLOSEWINDOW:
                if (bufp) {
                    bufp[0] = '\033';
                    bufp[1] = 0;
                }
                aredone = 1;
                break;
            }
            ReplyMsg((struct Message *) imsg);
        }
    }

    CloseWindow(cwin);
#else
    struct amii_WinDesc *cw;
    struct Window *w;
    int colx, ocolx, c;
    char *obufp;

    amii_clear_nhwindow(WIN_MESSAGE);
    amii_putstr(WIN_MESSAGE, 0, prompt);
    cw = amii_wins[WIN_MESSAGE];
    w = cw->win;
    ocolx = colx = strlen(prompt) + 1;

    obufp = bufp;
    cursor_on(WIN_MESSAGE);
    while ((c = WindowGetchar()) != EOF) {
        cursor_off(WIN_MESSAGE);
        amii_curs(WIN_MESSAGE, colx, 0);
        if (c == '\033') {
            *obufp = c;
            obufp[1] = 0;
            return;
        } else if (c == '\b') {
            if (bufp != obufp) {
                bufp--;
                amii_curs(WIN_MESSAGE, --colx, 0);
                Text(w->RPort, "\177 ", 2);
                amii_curs(WIN_MESSAGE, colx, 0);
            } else
                DisplayBeep(NULL);
        } else if (c == '\n' || c == '\r') {
            *bufp = 0;
            amii_addtopl(obufp);
            return;
        } else if (' ' <= c && c < '\177') {
            /* avoid isprint() - some people don't have it
               ' ' is not always a printing char */
            *bufp = c;
            bufp[1] = 0;

            Text(w->RPort, bufp, 1);
            Text(w->RPort, "\177", 1);
            if (bufp - obufp < BUFSZ - 1 && bufp - obufp < COLNO) {
                colx++;
                bufp++;
            }
        } else if (c == ('X' - 64) || c == '\177') {
            amii_curs(WIN_MESSAGE, ocolx, 0);
            Text(w->RPort, "                                                 "
                           "           ",
                 colx - ocolx);
            amii_curs(WIN_MESSAGE, colx = ocolx, 0);
        } else
            DisplayBeep(NULL);
        cursor_on(WIN_MESSAGE);
    }
    cursor_off(WIN_MESSAGE);
    *bufp = 0;
#endif
}

void
amii_change_color(pen, val, rev)
int pen, rev;
long val;
{
    if (rev)
        sysflags.amii_curmap[pen] = ~val;
    else
        sysflags.amii_curmap[pen] = val;

    if (HackScreen)
        LoadRGB4(&HackScreen->ViewPort, sysflags.amii_curmap, amii_numcolors);
}

char *
amii_get_color_string()
{
    int i;
    char s[10];
    static char buf[BUFSZ];

    *buf = 0;
    for (i = 0; i < min(32, amii_numcolors); ++i) {
        sprintf(s, "%s%03lx", i ? "/" : "", (long) sysflags.amii_curmap[i]);
        strcat(buf, s);
    }

    return (buf);
}

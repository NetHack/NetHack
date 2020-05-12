/* NetHack 3.6	amiwind.c	$NHDT-Date: 1432512794 2015/05/25 00:13:14 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/*    Copyright (c) Olaf Seibert (KosmoSoft), 1989, 1992	  */
/*    Copyright (c) Kenneth Lorber, Bethesda, Maryland 1993,1996  */
/* NetHack may be freely redistributed.  See license for details. */

#include "NH:sys/amiga/windefs.h"
#include "NH:sys/amiga/winext.h"
#include "NH:sys/amiga/winproto.h"

/* Have to undef CLOSE as display.h and intuition.h both use it */
#undef CLOSE

#ifdef AMII_GRAPHICS /* too early in the file? too late? */

#ifdef AMIFLUSH
static struct Message *FDECL(GetFMsg, (struct MsgPort *));
#endif

static int BufferGetchar(void);
static void ProcessMessage(register struct IntuiMessage *message);

#define BufferQueueChar(ch) (KbdBuffer[KbdBuffered++] = (ch))

struct Library *ConsoleDevice;

#include "NH:sys/amiga/amimenu.c"

/* Now our own variables */

struct IntuitionBase *IntuitionBase;
struct Screen *HackScreen;
struct Window *pr_WindowPtr;
struct MsgPort *HackPort;
struct IOStdReq ConsoleIO;
struct Menu *MenuStrip;
APTR *VisualInfo;
char Initialized = 0;
WEVENT lastevent;

#ifdef HACKFONT
struct GfxBase *GfxBase;
struct Library *DiskfontBase;
#endif

#define KBDBUFFER 10
static unsigned char KbdBuffer[KBDBUFFER];
unsigned char KbdBuffered;

#ifdef HACKFONT

struct TextFont *TextsFont = NULL;
struct TextFont *HackFont = NULL;
struct TextFont *RogueFont = NULL;

UBYTE FontName[] = "NetHack:hack.font";
/* # chars in "NetHack:": */
#define SIZEOF_DISKNAME 8

#endif

struct TextAttr Hack80 = {
#ifdef HACKFONT
    &FontName[SIZEOF_DISKNAME],
#else
    (UBYTE *) "topaz.font",
#endif
    8, FS_NORMAL, FPF_DISKFONT | FPF_DESIGNED | FPF_ROMFONT
};

struct TextAttr TextsFont13 = { (UBYTE *) "courier.font", 13, FS_NORMAL,
                                FPF_DISKFONT | FPF_DESIGNED
#ifndef HACKFONT
                                    | FPF_ROMFONT
#endif
};

/* Avoid doing a ReplyMsg through a window that no longer exists. */
static enum { NoAction, CloseOver } delayed_key_action = NoAction;

/*
 * Open a window that shares the HackPort IDCMP. Use CloseShWindow()
 * to close.
 */

struct Window *
OpenShWindow(nw)
struct NewWindow *nw;
{
    register struct Window *win;
    register ULONG idcmpflags;

    if (!HackPort) /* Sanity check */
        return (struct Window *) 0;

    idcmpflags = nw->IDCMPFlags;
    nw->IDCMPFlags = 0;
    if (!(win = OpenWindow((void *) nw))) {
        nw->IDCMPFlags = idcmpflags;
        return (struct Window *) 0;
    }

    nw->IDCMPFlags = idcmpflags;
    win->UserPort = HackPort;
    ModifyIDCMP(win, idcmpflags);
    return win;
}

/*
 * Close a window that shared the HackPort IDCMP port.
 */

void FDECL(CloseShWindow, (struct Window *));
void
CloseShWindow(win)
struct Window *win;
{
    register struct IntuiMessage *msg;

    if (!HackPort)
        panic("HackPort NULL in CloseShWindow");
    if (!win)
        return;

    Forbid();
    /* Flush all messages for all windows to avoid typeahead and other
     * similar problems...
     */
    while (msg = (struct IntuiMessage *) GetMsg(win->UserPort))
        ReplyMsg((struct Message *) msg);
    KbdBuffered = 0;
    win->UserPort = (struct MsgPort *) 0;
    ModifyIDCMP(win, 0L);
    Permit();
    CloseWindow(win);
}

static int
BufferGetchar()
{
    register int c;

    if (KbdBuffered > 0) {
        c = KbdBuffer[0];
        KbdBuffered--;
        /* Move the remaining characters */
        if (KbdBuffered < sizeof(KbdBuffer))
            memcpy(KbdBuffer, KbdBuffer + 1, KbdBuffered);
        return c;
    }

    return NO_CHAR;
}

/*
 *  This should remind you remotely of DeadKeyConvert, but we are cheating
 *  a bit. We want complete control over the numeric keypad, and no dead
 *  keys... (they are assumed to be on Alted keys).
 *
 *  Also assumed is that the IntuiMessage is of type RAWKEY.  For some
 *  reason, IECODE_UP_PREFIX events seem to be lost when they  occur while
 *  our console window is inactive. This is particulary  troublesome with
 *  qualifier keys... Is this because I never RawKeyConvert those events???
 */

int
ConvertKey(message)
register struct IntuiMessage *message;
{
    static struct InputEvent theEvent;
    static char numpad[] = "bjnh.lyku";
    static char ctrl_numpad[] = "\x02\x0A\x0E\x08.\x0C\x19\x0B\x15";
    static char shift_numpad[] = "BJNH.LYKU";

    unsigned char buffer[10];
    struct Window *w = message->IDCMPWindow;
    register int length;
    register ULONG qualifier;
    char numeric_pad, shift, control, alt;

    if (amii_wins[WIN_MAP])
        w = amii_wins[WIN_MAP]->win;
    qualifier = message->Qualifier;

    control = (qualifier & IEQUALIFIER_CONTROL) != 0;
    shift = (qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) != 0;
    alt = (qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT)) != 0;

    /* Allow ALT to function as a META key ... */
    /* But make it switchable - alt is needed for some non-US keymaps */
    if (sysflags.altmeta)
        qualifier &= ~(IEQUALIFIER_LALT | IEQUALIFIER_RALT);
    numeric_pad = (qualifier & IEQUALIFIER_NUMERICPAD) != 0;

    /*
     *  Shortcut for HELP and arrow keys. I suppose this is allowed.
     *  The defines are in intuition/intuition.h, and the keys don't
     *  serve 'text' input, normally. Also, parsing their escape
     *  sequences is such a mess...
     */

    switch (message->Code) {
    case RAWHELP:
        if (alt) {
            EditColor();
            return (-1);
        }
#ifdef CLIPPING
        else if (WINVERS_AMIV && control) {
            EditClipping();

            CO = (w->Width - w->BorderLeft - w->BorderRight) / mxsize;
            LI = (w->Height - w->BorderTop - w->BorderBottom) / mysize;
            clipxmax = CO + clipx;
            clipymax = LI + clipy;
            if (CO < COLNO || LI < ROWNO) {
                clipping = TRUE;
                amii_cliparound(u.ux, u.uy);
            } else {
                clipping = FALSE;
                clipx = clipy = 0;
            }
            BufferQueueChar('R' - 64);
            return (-1);
        }
#endif
        else if (WINVERS_AMIV && shift) {
            if (WIN_OVER == WIN_ERR) {
                WIN_OVER = amii_create_nhwindow(NHW_OVER);
                BufferQueueChar('R' - 64);
            } else {
                delayed_key_action = CloseOver;
            }
            return (-1);
        }
        return ('?');
        break;
    case CURSORLEFT:
        length = '4';
        numeric_pad = 1;
        goto arrow;
    case CURSORDOWN:
        length = '2';
        numeric_pad = 1;
        goto arrow;
    case CURSORUP:
        length = '8';
        numeric_pad = 1;
        goto arrow;
    case CURSORRIGHT:
        length = '6';
        numeric_pad = 1;
        goto arrow;
    }

    theEvent.ie_Class = IECLASS_RAWKEY;
    theEvent.ie_Code = message->Code;
    theEvent.ie_Qualifier = numeric_pad ? IEQUALIFIER_NUMERICPAD : qualifier;
    theEvent.ie_EventAddress = (APTR)(message->IAddress);

    length = RawKeyConvert(&theEvent, (char *) buffer, (long) sizeof(buffer),
                           NULL);

    if (length == 1) { /* Plain ASCII character */
        length = buffer[0];
    /*
     *  If iflags.num_pad is set, movement is by 4286.
     *  If not set, translate 4286 into hjkl.
     *  This way, the numeric pad can /always/ be used
     *  for moving, though best results are when it is off.
     */
    arrow:
        if (!iflags.num_pad && numeric_pad && length >= '1'
            && length <= '9') {
            length -= '1';
            if (control) {
                length = ctrl_numpad[length];
            } else if (shift) {
                length = shift_numpad[length];
            } else {
                length = numpad[length];
            }
        }

        /* Kludge to allow altmeta on eg. scandinavian keymap (# ==
           shift+alt+3)
           and prevent it from interfering with # command (M-#) */
        if (length == ('#' | 0x80))
            return '#';
        if (alt && sysflags.altmeta)
            length |= 0x80;
        return (length);
    } /* else shift, ctrl, alt, amiga, F-key, shift-tab, etc */
    else if (length > 1) {
        int i;

        if (length == 3 && buffer[0] == 155 && buffer[2] == 126) {
            int got = 1;
            switch (buffer[1]) {
            case 53:
                mxsize = mysize = 8;
                break;
            case 54:
                mxsize = mysize = 16;
                break;
            case 55:
                mxsize = mysize = 24;
                break;
            case 56:
                mxsize = mysize = 32;
                break;
            case 57:
                mxsize = mysize = 48;
                break;
            default:
                got = 0;
                break;
            }
#ifdef OPT_DISPMAP
            dispmap_sanity();
#endif

            if (got) {
                CO = (w->Width - w->BorderLeft - w->BorderRight) / mxsize;
                LI = (w->Height - w->BorderTop - w->BorderBottom) / mysize;
                clipxmax = CO + clipx;
                clipymax = LI + clipy;
                if (CO < COLNO || LI < ROWNO) {
                    amii_cliparound(u.ux, u.uy);
                } else {
                    CO = COLNO;
                    LI = ROWNO;
                }
                reclip = 1;
                doredraw();
                flush_screen(1);
                reclip = 0;
                /*BufferQueueChar( 'R'-64 );*/
                return (-1);
            }
        }
        printf("Unrecognized key: %d ", (int) buffer[0]);
        for (i = 1; i < length; ++i)
            printf("%d ", (int) buffer[i]);
        printf("\n");
    }
    return (-1);
}

/*
 *  Process an incoming IntuiMessage.
 *  It would certainly look nicer if this could be done using a
 *  PA_SOFTINT message port, but we cannot call RawKeyConvert()
 *  during a software interrupt.
 *  Anyway, amikbhit()/kbhit() is called often enough, and usually gets
 *  ahead of input demands, when the user types ahead.
 */

static void
ProcessMessage(message)
register struct IntuiMessage *message;
{
    int c;
    int cnt;
    menu_item *mip;
    static int skip_mouse = 0; /* need to ignore next mouse event on
                                * a window activation */
    struct Window *w = message->IDCMPWindow;

    switch (message->Class) {
    case ACTIVEWINDOW:
        if (alwaysinvent && WIN_INVEN != WIN_ERR
            && w == amii_wins[WIN_INVEN]->win) {
            cnt = DoMenuScroll(WIN_INVEN, 0, PICK_NONE, &mip);
        } else if (scrollmsg && WIN_MESSAGE != WIN_ERR
                   && w == amii_wins[WIN_MESSAGE]->win) {
            cnt = DoMenuScroll(WIN_MESSAGE, 0, PICK_NONE, &mip);
        } else {
            skip_mouse = 1;
        }
        break;

    case MOUSEBUTTONS: {
        if (skip_mouse) {
            skip_mouse = 0;
            break;
        }

        if (!amii_wins[WIN_MAP] || w != amii_wins[WIN_MAP]->win)
            break;

        if (message->Code == SELECTDOWN) {
            lastevent.type = WEMOUSE;
            lastevent.un.mouse.x = message->MouseX;
            lastevent.un.mouse.y = message->MouseY;
            /* With shift equals RUN */
            lastevent.un.mouse.qual =
                (message->Qualifier
                 & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) != 0;
        }
    } break;

    case MENUPICK: {
        USHORT thismenu;
        struct MenuItem *item;

        thismenu = message->Code;
        while (thismenu != MENUNULL) {
            item = ItemAddress(MenuStrip, (ULONG) thismenu);
            if (KbdBuffered < KBDBUFFER)
                BufferQueueChar((char) (GTMENUITEM_USERDATA(item)));
            thismenu = item->NextSelect;
        }
    } break;

    case REFRESHWINDOW: {
        if (scrollmsg && amii_wins[WIN_MESSAGE]
            && w == amii_wins[WIN_MESSAGE]->win) {
            cnt = DoMenuScroll(WIN_MESSAGE, 0, PICK_NONE, &mip);
        }
    } break;

    case CLOSEWINDOW:
        if (WIN_INVEN != WIN_ERR && w == amii_wins[WIN_INVEN]->win) {
            dismiss_nhwindow(WIN_INVEN);
        }
        if (WINVERS_AMIV
            && (WIN_OVER != WIN_ERR && w == amii_wins[WIN_OVER]->win)) {
            destroy_nhwindow(WIN_OVER);
            WIN_OVER = WIN_ERR;
        }
        break;

    case RAWKEY:
        if (!(message->Code & IECODE_UP_PREFIX)) {
            /* May queue multiple characters
             * but doesn't do that yet...
             */
            if ((c = ConvertKey(message)) > 0)
                BufferQueueChar(c);
        }
        break;

    case GADGETDOWN:
        if (WIN_MESSAGE != WIN_ERR && w == amii_wins[WIN_MESSAGE]->win) {
            cnt = DoMenuScroll(WIN_MESSAGE, 0, PICK_NONE, &mip);
        } else if (WIN_INVEN != WIN_ERR && w == amii_wins[WIN_INVEN]->win) {
            cnt = DoMenuScroll(WIN_INVEN, 0, PICK_NONE, &mip);
        }
        break;

    case NEWSIZE:
        if (WIN_MESSAGE != WIN_ERR && w == amii_wins[WIN_MESSAGE]->win) {
            if (WINVERS_AMIV) {
                /* Make sure that new size is honored for good. */
                SetAPen(w->RPort, amii_msgBPen);
                SetBPen(w->RPort, amii_msgBPen);
                SetDrMd(w->RPort, JAM2);
                RectFill(w->RPort, w->BorderLeft, w->BorderTop,
                         w->Width - w->BorderRight - 1,
                         w->Height - w->BorderBottom - 1);
            }
            ReDisplayData(WIN_MESSAGE);
        } else if (WIN_INVEN != WIN_ERR && w == amii_wins[WIN_INVEN]->win) {
            ReDisplayData(WIN_INVEN);
        } else if (WINVERS_AMIV && (WIN_OVER != WIN_ERR
                                    && w == amii_wins[WIN_OVER]->win)) {
            BufferQueueChar('R' - 64);
        } else if (WIN_MAP != WIN_ERR && w == amii_wins[WIN_MAP]->win) {
#ifdef CLIPPING
            CO = (w->Width - w->BorderLeft - w->BorderRight) / mxsize;
            LI = (w->Height - w->BorderTop - w->BorderBottom) / mysize;
            clipxmax = CO + clipx;
            clipymax = LI + clipy;
            if (CO < COLNO || LI < ROWNO) {
                amii_cliparound(u.ux, u.uy);
            } else {
                clipping = FALSE;
                clipx = clipy = 0;
            }
            BufferQueueChar('R' - 64);
#endif
        }
        break;
    }
    ReplyMsg((struct Message *) message);

    switch (delayed_key_action) {
    case CloseOver:
        amii_destroy_nhwindow(WIN_OVER);
        WIN_OVER = WIN_ERR;
        delayed_key_action = NoAction;
    case NoAction:
        ; /* null */
    }
}

#endif /* AMII_GRAPHICS */
       /*
        *  Get all incoming messages and fill up the keyboard buffer,
        *  thus allowing Intuition to (maybe) free up the IntuiMessages.
        *  Return when no more messages left, or keyboard buffer half full.
        *  We need to do this since there is no one-to-one correspondence
        *  between characters and incoming messages.
        */

#if defined(TTY_GRAPHICS) && !defined(AMII_GRAPHICS)
int
kbhit()
{
    return 0;
}
#else
int
kbhit()
{
    int c;
#ifdef TTY_GRAPHICS
    /* a kludge to defuse the mess in allmain.c */
    /* I hope this is the right approach */
    if (windowprocs.win_init_nhwindows == amii_procs.win_init_nhwindows)
        return 0;
#endif
    c = amikbhit();
    if (c <= 0)
        return (0);
    return (c);
}
#endif

#ifdef AMII_GRAPHICS

int
amikbhit()
{
    register struct IntuiMessage *message;
    while (KbdBuffered < KBDBUFFER / 2) {
#ifdef AMIFLUSH
        message = (struct IntuiMessage *) GetFMsg(HackPort);
#else
        message = (struct IntuiMessage *) GetMsg(HackPort);
#endif
        if (message) {
            ProcessMessage(message);
            if (lastevent.type != WEUNK && lastevent.type != WEKEY)
                break;
        } else
            break;
    }
    return (lastevent.type == WEUNK) ? KbdBuffered : -1;
}

/*
 *  Get a character from the keyboard buffer, waiting if not available.
 *  Ignore other kinds of events that happen in the mean time.
 */

int
WindowGetchar()
{
    while ((lastevent.type = WEUNK), amikbhit() <= 0) {
        WaitPort(HackPort);
    }
    return BufferGetchar();
}

WETYPE
WindowGetevent()
{
    lastevent.type = WEUNK;
    while (amikbhit() == 0) {
        WaitPort(HackPort);
    }

    if (KbdBuffered) {
        lastevent.type = WEKEY;
        lastevent.un.key = BufferGetchar();
    }
    return (lastevent.type);
}

/*
 *  Clean up everything. But before we do, ask the user to hit return
 *  when there is something that s/he should read.
 */

void
amii_cleanup()
{
    register struct IntuiMessage *msg;

    /* Close things up */
    if (HackPort) {
        amii_raw_print("");
        amii_getret();
    }

    if (ConsoleIO.io_Device)
        CloseDevice((struct IORequest *) &ConsoleIO);
    ConsoleIO.io_Device = 0;

    if (ConsoleIO.io_Message.mn_ReplyPort)
        DeleteMsgPort(ConsoleIO.io_Message.mn_ReplyPort);
    ConsoleIO.io_Message.mn_ReplyPort = 0;

    /* Strip messages before deleting the port */
    if (HackPort) {
        Forbid();
        while (msg = (struct IntuiMessage *) GetMsg(HackPort))
            ReplyMsg((struct Message *) msg);
        kill_nhwindows(1);
        DeleteMsgPort(HackPort);
        HackPort = NULL;
        Permit();
    }

    /* Close the screen, under v37 or greater it is a pub screen and there may
     * be visitors, so check close status and wait till everyone is gone.
     */
    if (HackScreen) {
#ifdef INTUI_NEW_LOOK
        if (IntuitionBase->LibNode.lib_Version >= 37) {
            if (MenuStrip)
                FreeMenus(MenuStrip);
            if (VisualInfo)
                FreeVisualInfo(VisualInfo);
            while (CloseScreen(HackScreen) == FALSE) {
                struct EasyStruct easy = {
                    sizeof(struct EasyStruct), 0, "Nethack Problem",
                    "Can't Close Screen, Close Visiting Windows", "Okay"
                };
                EasyRequest(NULL, &easy, NULL, NULL);
            }
        } else
#endif
        {
            CloseScreen(HackScreen);
        }
        HackScreen = NULL;
    }

#ifdef HACKFONT
    if (HackFont) {
        CloseFont(HackFont);
        HackFont = NULL;
    }

    if (TextsFont) {
        CloseFont(TextsFont);
        TextsFont = NULL;
    }

    if (RogueFont) {
        CloseFont(RogueFont);
        RogueFont = NULL;
    }

    if (DiskfontBase) {
        CloseLibrary(DiskfontBase);
        DiskfontBase = NULL;
    }
#endif

    if (GadToolsBase) {
        CloseLibrary((struct Library *) GadToolsBase);
        GadToolsBase = NULL;
    }

    if (LayersBase) {
        CloseLibrary((struct Library *) LayersBase);
        LayersBase = NULL;
    }

    if (GfxBase) {
        CloseLibrary((struct Library *) GfxBase);
        GfxBase = NULL;
    }

    if (IntuitionBase) {
        CloseLibrary((struct Library *) IntuitionBase);
        IntuitionBase = NULL;
    }

#ifdef SHAREDLIB
    if (DOSBase) {
        CloseLibrary((struct Library *) DOSBase);
        DOSBase = NULL;
    }
#endif

    ((struct Process *) FindTask(NULL))->pr_WindowPtr = (APTR) pr_WindowPtr;

    Initialized = 0;
}

#endif /* AMII_GRAPHICS */

#ifndef SHAREDLIB
void
Abort(rc)
long rc;
{
    int fault = 1;
#ifdef CHDIR
    extern char orgdir[];
    chdir(orgdir);
#endif
#ifdef AMII_GRAPHICS
    if (Initialized && ConsoleDevice
        && windowprocs.win_init_nhwindows == amii_procs.win_init_nhwindows) {
        printf("\n\nAbort with alert code %08lx...\n", rc);
        amii_getret();
    } else
#endif
        printf("\n\nAbort with alert code %08lx...\n", rc);
/* Alert(rc);              this is too severe */
#ifdef __SASC
#ifdef INTUI_NEW_LOOK
    if (IntuitionBase->LibNode.lib_Version >= 37) {
        struct EasyStruct es = {
            sizeof(struct EasyStruct), 0, "NetHack Panic Request",
            "NetHack is Aborting with code == 0x%08lx",
            "Continue Abort|Return to Program|Clean up and exit",
        };
        fault = EasyRequest(NULL, &es, NULL, (long) rc);
        if (fault == 2)
            return;
    }
#endif
    if (fault == 1) {
        /*  __emit(0x4afc); */ /* illegal instruction */
        __emit(0x40fc);        /* divide by */
        __emit(0x0000);        /*  #0  */
        /* NOTE: don't move amii_cleanup() above here - */
        /* it is too likely to kill the system     */
        /* before it can get the SnapShot out, if  */
        /* there is something really wrong.    */
    }
#endif
#ifdef AMII_GRAPHICS
    if (windowprocs.win_init_nhwindows == amii_procs.win_init_nhwindows)
        amii_cleanup();
#endif
#undef exit
#ifdef AZTEC_C
    _abort();
#endif
    exit((int) rc);
}

void
CleanUp()
{
    amii_cleanup();
}
#endif

#ifdef AMII_GRAPHICS

#ifdef AMIFLUSH
/* This routine adapted from AmigaMail IV-37 by Michael Sinz */
static struct Message *
GetFMsg(port)
struct MsgPort *port;
{
    struct IntuiMessage *msg, *succ, *succ1;

    if (msg = (struct IntuiMessage *) GetMsg(port)) {
        if (!sysflags.amiflush)
            return ((struct Message *) msg);
        if (msg->Class == RAWKEY) {
            Forbid();
            succ = (struct IntuiMessage *) (port->mp_MsgList.lh_Head);
            while (succ1 = (struct IntuiMessage *) (succ->ExecMessage.mn_Node
                                                        .ln_Succ)) {
                if (succ->Class == RAWKEY) {
                    Remove((struct Node *) succ);
                    ReplyMsg((struct Message *) succ);
                }
                succ = succ1;
            }
            Permit();
        }
    }
    return ((struct Message *) msg);
}
#endif

struct NewWindow *
DupNewWindow(win)
struct NewWindow *win;
{
    struct NewWindow *nwin;
    struct Gadget *ngd, *gd, *pgd = NULL;
    struct PropInfo *pip;
    struct StringInfo *sip;

    /* Copy the (Ext)NewWindow structure */

    nwin = (struct NewWindow *) alloc(sizeof(struct NewWindow));
    *nwin = *win;

    /* Now do the gadget list */

    nwin->FirstGadget = NULL;
    for (gd = win->FirstGadget; gd; gd = gd->NextGadget) {
        ngd = (struct Gadget *) alloc(sizeof(struct Gadget));
        *ngd = *gd;
        if (gd->GadgetType == STRGADGET) {
            sip = (struct StringInfo *) alloc(sizeof(struct StringInfo));
            *sip = *((struct StringInfo *) gd->SpecialInfo);
            sip->Buffer = (UBYTE *) alloc(sip->MaxChars);
            *sip->Buffer = 0;
            ngd->SpecialInfo = (APTR) sip;
        } else if (gd->GadgetType == PROPGADGET) {
            pip = (struct PropInfo *) alloc(sizeof(struct PropInfo));
            *pip = *((struct PropInfo *) gd->SpecialInfo);
            ngd->SpecialInfo = (APTR) pip;
        }
        if (pgd)
            pgd->NextGadget = ngd;
        else
            nwin->FirstGadget = ngd;
        pgd = ngd;
        ngd->NextGadget = NULL;
        ngd->UserData = (APTR) 0x45f35c3d; // magic cookie for FreeNewWindow()
    }
    return (nwin);
}

void
FreeNewWindow(win)
struct NewWindow *win;
{
    register struct Gadget *gd, *pgd;
    register struct StringInfo *sip;

    for (gd = win->FirstGadget; gd; gd = pgd) {
        pgd = gd->NextGadget;
        if ((ULONG) gd->UserData == 0x45f35c3d) {
            if (gd->GadgetType == STRGADGET) {
                sip = (struct StringInfo *) gd->SpecialInfo;
                free(sip->Buffer);
                free(sip);
            } else if (gd->GadgetType == PROPGADGET) {
                free((struct PropInfo *) gd->SpecialInfo);
            }
            free(gd);
        }
    }
    free(win);
}

void
bell()
{
    if (flags.silent)
        return;
    DisplayBeep(NULL);
}

void
amii_delay_output()
{
    /* delay 50 ms */
    Delay(2L);
}

void
amii_number_pad(state)
int state;
{
}
#endif /* AMII_GRAPHICS */

#ifndef SHAREDLIB
void
amiv_loadlib(void)
{
}

void
amii_loadlib(void)
{
}

/* fatal error */
/*VARARGS1*/
void error
VA_DECL(const char *, s)
{
    VA_START(s);
    VA_INIT(s, char *);

    putchar('\n');
    vprintf(s, VA_ARGS);
    putchar('\n');

    VA_END();
    Abort(0L);
}
#endif

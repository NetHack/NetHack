/* NetHack 5.0	mrecover.c	$NHDT-Date: 1432512798 2015/05/25 00:13:18 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/*      Copyright (c) David Hairston, 1993.                       */
/* NetHack may be freely redistributed.  See license for details. */

/* Macintosh Recovery Application */

/* based on code in util/recover.c.  the significant differences are:
 * - GUI vs. CLI.  the vast majority of code here supports the GUI.
 * - Mac toolbox equivalents are used in place of ANSI functions.
 * - void restore_savefile(void) is event driven.
 * - integral type substitutions here and there.
 */

/*
 * Think C 5.0.4 project specs:
 * signature: 'nhRc'
 * SIZE (-1) info: flags: 0x5880, size: 65536L/65536L (64k/64k)
 * libraries: MacTraps [yes], MacTraps2 (HFileStuff) [yes], ANSI [no]
 * compatibility: system 6 and system 7
 * misc: sizeof(int): 2, "\x00": unsigned char, enum size varies,
 *   prototypes required, type checking enforced, no optimizers,
 *   FAR CODE [no], FAR DATA [no], SEPARATE STRS [no], single segment,
 *   short macsbug symbols
 */

#include "config.h"

/**** Toolbox defines ****/

/* MPW C headers (99.44% pure) */
#include <Errors.h>
#include <OSUtils.h>
#include <Resources.h>
#include <Files.h>
#include <Menus.h>
#include <Devices.h>
#include <Events.h>
#include <DiskInit.h>
#include <Notification.h>
#include <Packages.h>
#include <Script.h>
#include <StandardFile.h>
#include <ToolUtils.h>
#include <Processes.h>
#include <Fonts.h>
#include <TextUtils.h>

#include "maccompat.h" /* P_STRING_CONV */

/**** Application defines ****/

/* Memory */
typedef struct memBytes /* format of 'memB' resource, preloaded/locked */
    {
    short memReserved;
    short memCleanup; /* 4   - memory monitor activity limit */
    long memPreempt;  /* 32k - start iff FreeMem() > */
    long memWarning;  /* 12k - warn if MaxMem() < */
    long memAbort;    /* 4k  - abort if MaxMem() < */
    long memIOBuf;    /* 16k - read/write buffer size */
} memBytes, *memBytesPtr, **memBytesHandle;

#define membID 128 /* 'memB' resource ID */

/* Cursor */
#define CURS_FRAME 4L    /* 1/15 second - spin cursor */
#define CURS_LATENT 60L  /* pause before spin cursor */
#define curs_Init (-1)   /* token for set arrow */
#define curs_Total 8     /* maybe 'acur' would be better */
#define cursorOffset 128 /* GetCursor(cursorOffset + i) */

/* Menu */
enum {
    mbar_Init = -1,
    mbarAppl,    /* normal mode */
    mbarRecover, /* in recovery mode */
    mbarDA       /* DA in front mode */
};
enum {
    menuApple,
    menuFile,
    menuEdit,
    menu_Total,

    muidApple = 128,
    muidFile,
    muidEdit
};
enum {
    /* Apple menu */
    mitmAbout = 1,
    mitmHelp,
    ____128_1,

    /* File menu */
    mitmOpen = 1,
    ____129_1,
    mitmClose_DA,
    ____129_2,
    mitmQuit

    /* standard minimum required Edit menu */
};

/* Alerts */
enum {
    alrtNote, /* general messages */
    alrtHelp, /* help message */
    alrt_Total,

    alertAppleMenu = 127, /* menuItem to alert ID offset */
    alidNote,
    alidHelp
};

#define aboutBufSize 80 /* i.e. 2 lines of 320 pixels */

/* Notification */
#define nmBufSize (32 + aboutBufSize + 32)
typedef struct notifRec {
    NMRec nmr;
    struct notifRec *nmNext;
    short nmDispose;
    unsigned char nmBuf[nmBufSize];
} notifRec, *notifPtr;

#define nmPending nmRefCon /* &in.Notify */
#define iconNotifyID 128
#define ics_1_and_4 0x00000300

/* Dialogs */
enum { dlogProgress = 256 };
enum { uitmThermo = 1 };
enum { initItem, invalItem, drawItem };

/* Miscellaneous */
typedef struct modeFlags {
    short Front;   /* fg/bg event handling */
    short Notify;  /* level of pending NM notifications */
    short Dialog;  /* a modeless dialog is open */
    short Recover; /* restoration progress index */
} modeFlags;

/* convenient define to allow easier (for me) parsing of 'vers' resource */
typedef struct versXRec {
    NumVersion numVers;
    short placeCode;
    unsigned char versStr[]; /* (small string)(large string) */
} versXRec, *versXPtr, **versXHandle;

/**** Global variables ****/
modeFlags in = { 1 }; /* in Front */
EventRecord wnEvt;
SysEnvRec sysEnv;
unsigned char aboutBuf[aboutBufSize]; /* vers 1 "Get Info" string */
memBytesPtr pBytes;                   /* memory management */
unsigned short memActivity;           /* more memory management */
MenuHandle mHnd[menu_Total];
CursPtr cPtr[curs_Total];    /* busy cursors */
unsigned long timeCursor;    /* next cursor frame time */
short oldCursor = curs_Init; /* see adjustGUI() below */
notifPtr pNMQ;               /* notification queue pointer */
notifRec nmt;                /* notification template */
DialogTHndl thermoTHnd;
DialogRecord dlgThermo; /* progress thermometer */
#define DLGTHM ((DialogPtr) &dlgThermo)
#define WNDTHM ((WindowPtr) &dlgThermo)
#define GRFTHM ((GrafPtr) &dlgThermo)

Point sfGetWhere;      /* top left corner of get file dialog */
Ptr pIOBuf;            /* read/write buffer pointer */
short vRefNum;         /* SFGetFile working directory/volume refnum */
long dirID;            /* directory i.d. */
NMUPP nmCompletionUPP; /* UPP for nmCompletion */
FileFilterUPP basenameFileFilterUPP; /* UPP for basenameFileFilter */
UserItemUPP drawThermoUPP;           /* UPP for progress callback */

#define MAX_RECOVER_COUNT 256

#define APP_NAME_RES_ID (-16396) /* macfile.h */
#define PLAYER_NAME_RES_ID 1001  /* macfile.h */

/* variables from util/recover.c; SAVESIZE comes from fnamesiz.h, which is
   what src/files.c sizes the level-0 header's savename field with */
unsigned char savename[SAVESIZE]; /* originally a C string */
unsigned char lock[256];          /* pascal string */

int hpid;         /* NetHack (unix-style) process i.d. */
short saveRefNum; /* save file descriptor */
short gameRefNum; /* level 0 file descriptor */
short levRefNum;  /* level n file descriptor */

/**** Prototypes ****/
static void warmup(void);
static Handle alignTemplate(ResType, short, short, short, Point *);
pascal void nmCompletion(NMRec *);
static void noteErrorMessage(unsigned char *, unsigned char *);
static void note(short, short, unsigned char *);
static void adjustGUI(void);
static void adjustMemory(void);
static void optionMemStats(void);
static void RecoverMenuEvent(long);
static void eventLoop(void);
static void cooldown(void);

pascal void drawThermo(WindowPtr, short);
static void itemizeThermo(short);
pascal Boolean basenameFileFilter(ParmBlkPtr);
static void beginRecover(void);
static void continueRecover(void);
static void endRecover(void);
static short saveRezStrings(void);

/* analogous prototypes from util/recover.c */
static void set_levelfile_name(long);
static short open_levelfile(long);
static short create_savefile(unsigned char *);
static void copy_bytes(short, short);
static void restore_savefile(void);

/* auxiliary prototypes */
static long read_levelfile(short, Ptr, long);
static long write_savefile(short, Ptr, long);
static void close_file(short *);
static void unlink_file(unsigned char *);

/**** Routines ****/

int
main(void)
{
    /* heap adjust */
    MaxApplZone();
    MoreMasters();
    MoreMasters();

    /* manager initialization */
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(0L);
    InitCursor();
    nmCompletionUPP = NewNMProc(nmCompletion);
    basenameFileFilterUPP = NewFileFilterProc(basenameFileFilter);
    drawThermoUPP = NewUserItemProc(drawThermo);

    /* get system environment, notification requires 6.0 or better */
    (void) SysEnvirons(curSysEnvVers, &sysEnv);
    if (sysEnv.systemVersion < 0x0600) {
        ParamText(P_STRING_CONV("Abort: System 6.0 is required"), P_EMPTY_STRING, P_EMPTY_STRING,
                  P_EMPTY_STRING);
        (void) Alert(alidNote, (ModalFilterUPP) 0L);
        ExitToShell();
    }

    warmup();
    eventLoop();

    /* normally these routines are never reached from here */
    cooldown();
    ExitToShell();
    return 0;
}

static void
warmup(void)
{
    short i;

    /* pre-System 7 MultiFinder hack for smooth launch */
    for (i = 0; i < 10; i++) {
        if (WaitNextEvent(osMask, &wnEvt, 2L, (RgnHandle) 0L))
            if (((wnEvt.message & osEvtMessageMask) >> 24)
                == suspendResumeMessage)
                in.Front = (wnEvt.message & resumeFlag);
    }

    /* fill out the notification template */
    nmt.nmr.qType = nmType;
    nmt.nmr.nmMark = 1;
    nmt.nmr.nmSound = (Handle) -1L; /* system beep */
    nmt.nmr.nmStr = nmt.nmBuf;
    nmt.nmr.nmResp = nmCompletionUPP;
    nmt.nmr.nmPending = (long) &in.Notify;

    {
        /* get the app name */
        ProcessInfoRec info;
        ProcessSerialNumber psn;

        info.processInfoLength = sizeof(info);
        info.processName = nmt.nmBuf;
        info.processAppSpec = NULL;
        GetCurrentProcess(&psn);
        GetProcessInformation(&psn, &info);
    }

    /* add formatting (two line returns) */
    nmt.nmBuf[++(nmt.nmBuf[0])] = '\r';
    nmt.nmBuf[++(nmt.nmBuf[0])] = '\r';

    /**** note() is usable now but not aesthetically complete ****/

    /* get notification icon */
    if (sysEnv.systemVersion < 0x0700) {
        if (!(nmt.nmr.nmIcon = GetResource('SICN', iconNotifyID)))
            note(nilHandleErr, 0, P_STRING_CONV("Nil SICN Handle"));
    } else {
        if (GetIconSuite(&nmt.nmr.nmIcon, iconNotifyID, ics_1_and_4))
            note(nilHandleErr, 0, P_STRING_CONV("Bad Icon Family"));
    }

    /* load and align various dialog/alert templates */
    (void) alignTemplate('ALRT', alidNote, 0, 4, (Point *) 0L);
    (void) alignTemplate('ALRT', alidHelp, 0, 4, (Point *) 0L);

    thermoTHnd = (DialogTHndl) alignTemplate('DLOG', dlogProgress, 20, 8,
                                             (Point *) 0L);

    (void) alignTemplate('DLOG', getDlgID, 0, 6, (Point *) &sfGetWhere);

    /* get the "busy cursors" (preloaded/locked) */
    for (i = 0; i < curs_Total; i++) {
        CursHandle cHnd;

        if (!(cHnd = GetCursor(i + cursorOffset)))
            note(nilHandleErr, 0, P_STRING_CONV("Nil CURS Handle"));

        cPtr[i] = *cHnd;
    }

    /* get the 'vers' 1 long (Get Info) string - About Recover... */
    {
        versXHandle vHnd;

        if (!(vHnd = (versXHandle) GetResource('vers', 1)))
            note(nilHandleErr, 0, P_STRING_CONV("Nil vers Handle"));

        i = (**vHnd).versStr[0] + 1; /* offset to Get Info pascal string */

        if ((aboutBuf[0] = (**vHnd).versStr[i]) > (aboutBufSize - 1))
            aboutBuf[0] = aboutBufSize - 1;

        i++;

        MoveHHi((Handle) vHnd); /* DEE - Fense ... */
        HLock((Handle) vHnd);
        BlockMove(&((**vHnd).versStr[i]), &(aboutBuf[1]), aboutBuf[0]);
        ReleaseResource((Handle) vHnd);
    }

    /* form the menubar */
    for (i = 0; i < menu_Total; i++) {
        if (!(mHnd[i] = GetMenu(i + muidApple)))
            note(nilHandleErr, 0, P_STRING_CONV("Nil MENU Handle"));

        /* expand the apple menu */
        if (i == menuApple)
            AppendResMenu(mHnd[menuApple], 'DRVR');

        InsertMenu(mHnd[i], 0);
    }

    /* pre-emptive memory check */
    {
        memBytesHandle hBytes;
        Size grow;

        if (!(hBytes = (memBytesHandle) GetResource('memB', membID)))
            note(nilHandleErr, 0, P_STRING_CONV("Nil Memory Handle"));

        pBytes = *hBytes;

        if (MaxMem(&grow) < pBytes->memPreempt)
            note(memFullErr, 0, P_STRING_CONV("More Memory Required\rTry adding 16k"));

        memActivity = pBytes->memCleanup; /* force initial cleanup */
    }

    /* get the I/O buffer */
    if (!(pIOBuf = NewPtr(pBytes->memIOBuf)))
        note(memFullErr, 0, P_STRING_CONV("Nil I/O Pointer"));
}

/* align a window-related template to the main screen */
static Handle
alignTemplate(ResType rezType, short rezID, short vOff, short vDenom,
              Point *pPt)
{
    Handle rtnHnd;
    Rect *pRct;

    vOff += GetMBarHeight();

    if (!(rtnHnd = GetResource(rezType, rezID)))
        note(nilHandleErr, 0, P_STRING_CONV("Nil Template Handle"));

    pRct = (Rect *) *rtnHnd;

    /* don't move memory while aligning rect */
    pRct->right -= pRct->left; /* width */
    pRct->bottom -= pRct->top; /* height */
    pRct->left = (qd.screenBits.bounds.right - pRct->right) / 2;
    pRct->top = (qd.screenBits.bounds.bottom - pRct->bottom - vOff) / vDenom;
    pRct->top += vOff;
    pRct->right += pRct->left;
    pRct->bottom += pRct->top;

    if (pPt)
        *pPt = *(Point *) pRct; /* top left corner */

    return rtnHnd;
}

/* notification completion routine */
pascal void
nmCompletion(NMRec *pNMR)
{
    (void) NMRemove(pNMR);

    (*(short *) (pNMR->nmPending))--; /* decrement pending note level */
    ((notifPtr) pNMR)->nmDispose = 1; /* allow DisposePtr() */
}

/*
 * handle errors inside of note().  the error message is appended to the
 * given message but on a separate line and must fit within nmBufSize.
 */
static void
noteErrorMessage(unsigned char *msg, unsigned char *errMsg)
{
    short i = nmt.nmBuf[0] + 1; /* insertion point */

    BlockMove(&msg[1], &nmt.nmBuf[i], msg[0]);
    nmt.nmBuf[i + msg[0]] = '\r';
    nmt.nmBuf[0] += (msg[0] + 1);

    note(memFullErr, 0, errMsg);
}

/*
 * display messages using Notification Manager or an alert.
 * no run-length checking is done.  the messages are created to fit
 * in the allocated space (nmBufSize and aboutBufSize).
 */
static void
note(short errorSignal, short alertID, unsigned char *msg)
{
    if (!errorSignal) {
        Size grow;

        if (MaxMem(&grow) < pBytes->memAbort)
            noteErrorMessage(msg, P_STRING_CONV("Out of Memory"));
    }

    if (errorSignal || !in.Front) {
        notifPtr pNMR;
        short i = nmt.nmBuf[0] + 1; /* insertion point */

        if (errorSignal) /* use notification template */
        {
            pNMR = &nmt;

            /* we're going to abort so add in this prefix */
            BlockMove("Abort: ", &nmt.nmBuf[i], 7);
            i += 7;
            nmt.nmBuf[0] += 7;
        } else /* allocate a notification record */
        {
            if (!(pNMR = (notifPtr) NewPtr(sizeof(notifRec))))
                noteErrorMessage(msg, P_STRING_CONV("Nil New Pointer"));

            /* initialize it */
            *pNMR = nmt;
            pNMR->nmr.nmStr = (StringPtr) & (pNMR->nmBuf);

            /* update the notification queue */
            if (!pNMQ)
                pNMQ = pNMR;
            else {
                notifPtr pNMX;

                /* find the end of the queue */
                for (pNMX = pNMQ; pNMX->nmNext; pNMX = pNMX->nmNext)
                    ;

                pNMX->nmNext = pNMR;
            }
        }

        /* concatenate the message */
        BlockMove(&msg[1], &((pNMR->nmBuf)[i]), msg[0]);
        (pNMR->nmBuf)[0] += msg[0];

        in.Notify++; /* increase note pending level */

        NMInstall((NMRec *) pNMR);

        if (errorSignal)
            cooldown();

        return;
    }

    /* in front and no error so use an alert */
    ParamText(msg, P_EMPTY_STRING, P_EMPTY_STRING, P_EMPTY_STRING);
    (void) Alert(alertID, (ModalFilterUPP) 0L);
    ResetAlertStage();

    memActivity++;
}

static void
adjustGUI(void)
{
    static short oldMenubar = mbar_Init; /* force initial update */
    short newMenubar;
    WindowPeek frontWindow;

    /* oldCursor is external so it can be reset in endRecover() */
    static short newCursor = curs_Init;
    unsigned long timeNow;
    short useArrow;

    /* adjust menubar 1st */
    newMenubar = in.Recover ? mbarRecover : mbarAppl;

    /* desk accessories take precedence */
    if ((frontWindow = (WindowPeek) FrontWindow()) != 0L)
        if (frontWindow->windowKind < 0)
            newMenubar = mbarDA;

    if (newMenubar != oldMenubar) {
        /* adjust menus */
        switch (oldMenubar = newMenubar) {
        case mbarAppl:
            EnableItem(mHnd[menuFile], mitmOpen);
            SetItemMark(mHnd[menuFile], mitmOpen, noMark);
            DisableItem(mHnd[menuFile], mitmClose_DA);
            DisableItem(mHnd[menuEdit], 0);
            break;

        case mbarRecover:
            DisableItem(mHnd[menuFile], mitmOpen);
            SetItemMark(mHnd[menuFile], mitmOpen, checkMark);
            DisableItem(mHnd[menuFile], mitmClose_DA);
            DisableItem(mHnd[menuEdit], 0);
            break;

        case mbarDA:
            DisableItem(mHnd[menuFile], mitmOpen);
            EnableItem(mHnd[menuFile], mitmClose_DA);
            EnableItem(mHnd[menuEdit], 0);
            break;
        }

        DrawMenuBar();
    }

    /* now adjust the cursor */
    if ((useArrow = (!in.Recover || (newMenubar == mbarDA))) != 0)
        newCursor = curs_Init;
    else if ((timeNow = TickCount()) >= timeCursor) /* spin cursor */
    {
        timeCursor = timeNow + CURS_FRAME;
        if (++newCursor >= curs_Total)
            newCursor = 0;
    }

    if (newCursor != oldCursor) {
        oldCursor = newCursor;

        SetCursor(useArrow ? &qd.arrow : cPtr[newCursor]);
    }
}

static void
adjustMemory(void)
{
    Size grow;

    memActivity = 0;

    if (MaxMem(&grow) < pBytes->memWarning)
        note(noErr, alidNote, P_STRING_CONV("Warning: Memory is running low"));

    (void) ReserveMem((Size) FreeMem()); /* move all handles high */
}

/* show memory stats: FreeMem, MaxBlock, PurgeSpace, and StackSpace */
static void
optionMemStats(void)
{
    unsigned char *pFormat =
        (unsigned char *) P_STRING_CONV("Free:#k  Max:#k  Purge:#k  Stack:#k");
    char *pSub = "#"; /* not a pascal string */
    Str255 nBuf;      /* NumToString's declared output size */
    long nStat, contig;
    Handle strHnd;
    long nOffset;
    short i;

    if (wnEvt.modifiers & shiftKey)
        adjustMemory();

    if (!(strHnd = NewHandle((Size) 128))) {
        note(noErr, alidNote, P_STRING_CONV("Oops: Memory stats unavailable!"));
        return;
    }

    SetString((StringHandle) strHnd, pFormat);
    nOffset = 1L;

    for (i = 1; i <= 4; i++) {
        /* get the replacement number stat */
        switch (i) {
        case 1:
            nStat = FreeMem();
            break;
        case 2:
            nStat = MaxBlock();
            break;
        case 3:
            PurgeSpace(&nStat, &contig);
            break;
        case 4:
            nStat = StackSpace();
            break;
        }

        NumToString((nStat >> 10), nBuf);

        **strHnd += nBuf[0] - 1;
        nOffset =
            Munger(strHnd, nOffset, (Ptr) pSub, 1L, (Ptr) &nBuf[1], nBuf[0]);
    }

    MoveHHi(strHnd);
    HLock(strHnd);
    note(noErr, alidNote, (unsigned char *) *strHnd);
    DisposeHandle(strHnd);
}

static void
RecoverMenuEvent(long menuEntry)
{
    short menuID = HiWord(menuEntry);
    short menuItem = LoWord(menuEntry);

    switch (menuID) {
    case muidApple:
        switch (menuItem) {
        case mitmAbout:
            if (wnEvt.modifiers & optionKey)
                optionMemStats();
        /* fall thru */
        case mitmHelp:
            note(noErr, (alertAppleMenu + menuItem), aboutBuf);
            break;

        default: /* DA's or apple menu items */
        {
            Str255 daName; /* GetMenuItemText may write up to 256 bytes */

            GetMenuItemText(mHnd[menuApple], menuItem, daName);
            (void) OpenDeskAcc(daName);

            memActivity++;
        } break;
        }
        break;

    case muidFile:
        switch (menuItem) {
        case mitmOpen:
            beginRecover();
            break;

        case mitmClose_DA: {
            WindowPeek frontWindow;
            short refNum;

            if ((frontWindow = (WindowPeek) FrontWindow()) != 0L)
                if ((refNum = frontWindow->windowKind) < 0)
                    CloseDeskAcc(refNum);

            memActivity++;
        } break;

        case mitmQuit:
            cooldown();
            break;
        }
        break;

    case muidEdit:
        (void) SystemEdit(menuItem - 1);
        break;
    }

    HiliteMenu(0);
}

static void
eventLoop(void)
{
    short wneMask = (in.Front ? everyEvent : (osMask + updateMask));
    long wneSleep = (in.Front ? 0L : 3L);

    while (1) {
        if (in.Front)
            adjustGUI();

        if (memActivity >= pBytes->memCleanup)
            adjustMemory();

        (void) WaitNextEvent(wneMask, &wnEvt, wneSleep, (RgnHandle) 0L);

        if (in.Dialog)
            (void) IsDialogEvent(&wnEvt);

        switch (wnEvt.what) {
        case osEvt:
            if (((wnEvt.message & osEvtMessageMask) >> 24)
                == suspendResumeMessage) {
                in.Front = (wnEvt.message & resumeFlag);
                wneMask = (in.Front ? everyEvent : (osMask + updateMask));
                wneSleep = (in.Front ? 0L : 3L);
            }
            break;

        case nullEvent:
            /* adjust the FIFO notification queue */
            if (pNMQ && pNMQ->nmDispose) {
                notifPtr pNMX = pNMQ->nmNext;

                DisposePtr((Ptr) pNMQ);
                pNMQ = pNMX;

                memActivity++;
            }

            if (in.Recover)
                continueRecover();
            break;

        case mouseDown: {
            WindowPtr whichWindow;

            switch (FindWindow(wnEvt.where, &whichWindow)) {
            case inMenuBar:
                RecoverMenuEvent(MenuSelect(wnEvt.where));
                break;

            case inSysWindow:
                SystemClick(&wnEvt, whichWindow);
                break;

            case inDrag: {
                Rect boundsRect = qd.screenBits.bounds;
                Point offsetPt;

                InsetRect(&boundsRect, 4, 4);
                boundsRect.top += GetMBarHeight();

                DragWindow(whichWindow, *((Point *) &wnEvt.where),
                           &boundsRect);

                boundsRect = whichWindow->portRect;
                offsetPt.v = whichWindow->portBits.bounds.top;
                offsetPt.h = whichWindow->portBits.bounds.left;
                OffsetRect(&boundsRect, -offsetPt.h, -offsetPt.v);

                /* remember the new position in the dialog template so a
                   re-opened thermometer appears where the user left it */
                (**thermoTHnd).boundsRect = boundsRect;
            } break;
            }
        } break;

        case keyDown: {
            char key = (wnEvt.message & charCodeMask);

            if (wnEvt.modifiers & cmdKey) {
                if (key == '.') {
                    if (in.Recover) {
                        endRecover();
                        note(noErr, alidNote, P_STRING_CONV("Sorry: Recovery aborted"));
                    }
                } else
                    RecoverMenuEvent(MenuKey(key));
            }
        } break;

        /* without windows these events belong to our thermometer */
        case updateEvt:
        case activateEvt: {
            DialogPtr dPtr;
            short itemHit;

            (void) DialogSelect(&wnEvt, &dPtr, &itemHit);
        } break;

        case diskEvt:
            if (HiWord(wnEvt.message)) {
                Point pt = { 60, 60 };

                (void) DIBadMount(pt, wnEvt.message);
                DIUnload();

                memActivity++;
            }
            break;
        } /* switch (wnEvt.what) */
    }     /* while (1) */
}

static void
cooldown(void)
{
    if (in.Recover)
        endRecover();

    /* wait for pending notifications to complete */
    while (in.Notify)
        (void) WaitNextEvent(0, &wnEvt, 3L, (RgnHandle) 0L);

    ExitToShell();
}

/* draw the progress thermometer and frame.  1 level <=> 1 horiz. pixel */
pascal void
drawThermo(WindowPtr wPtr, short inum)
{
    itemizeThermo(drawItem);
}

/* manage progress thermometer dialog */
static void
itemizeThermo(short itemMode)
{
    short iTyp, iTmp;
    Handle iHnd;
    Rect iRct;

    GetDialogItem(DLGTHM, uitmThermo, &iTyp, &iHnd, &iRct);

    switch (itemMode) {
    case initItem:
        SetDialogItem(DLGTHM, uitmThermo, iTyp, (Handle) drawThermoUPP, &iRct);
        break;

    case invalItem: {
        GrafPtr oldPort;

        GetPort(&oldPort);
        SetPort(GRFTHM);

        InsetRect(&iRct, 1, 1);
        InvalRect(&iRct);

        SetPort(oldPort);
    } break;

    case drawItem:
        FrameRect(&iRct);
        InsetRect(&iRct, 1, 1);

        iTmp = iRct.right;
        iRct.right = iRct.left + in.Recover;
        PaintRect(&iRct);

        iRct.left = iRct.right;
        iRct.right = iTmp;
        EraseRect(&iRct);
        break;
    }
}

/* show only <pid-plname>.0 files in get file dialog */
pascal Boolean
basenameFileFilter(ParmBlkPtr pPB)
{
    unsigned char *pC;

    if (!(pC = (unsigned char *) pPB->fileParam.ioNamePtr))
        return true;

    if ((*pC < 4) || (*pC > 28)) /* save/ 1name .0 */
        return true;

    if ((pC[*pC - 1] == '.') && (pC[*pC] == '0')) /* bingo! */
        return false;

    return true;
}

static void
beginRecover(void)
{
    SFTypeList levlType = { 'LEVL' };
    SFReply sfGetReply;

    SFGetFile(sfGetWhere, P_EMPTY_STRING, basenameFileFilterUPP, 1, levlType,
              (DlgHookUPP) 0L, &sfGetReply);

    memActivity++;

    if (!sfGetReply.good)
        return;

    /* get volume (working directory) refnum, basename, and directory i.d. */
    vRefNum = sfGetReply.vRefNum;
    BlockMove(sfGetReply.fName, lock, sfGetReply.fName[0] + 1);
    {
        static CInfoPBRec catInfo;

        catInfo.hFileInfo.ioNamePtr = (StringPtr) sfGetReply.fName;
        catInfo.hFileInfo.ioVRefNum = sfGetReply.vRefNum;
        catInfo.hFileInfo.ioDirID = 0L;

        if (PBGetCatInfoSync(&catInfo)) {
            note(noErr, alidNote, P_STRING_CONV("Sorry: Bad File Info"));
            return;
        }

        dirID = catInfo.hFileInfo.ioFlParID;
    }

    /* open the progress thermometer dialog */
    (void) GetNewDialog(dlogProgress, (Ptr) &dlgThermo, (WindowPtr) -1L);
    if (ResError() || MemError())
        note(noErr, alidNote, P_STRING_CONV("Oops: Progress thermometer unavailable"));
    else {
        in.Dialog = 1;
        memActivity++;

        itemizeThermo(initItem);

        ShowWindow(WNDTHM);
    }

    timeCursor = TickCount() + CURS_LATENT;
    saveRefNum = gameRefNum = levRefNum = -1;
    in.Recover = 1;
}

static void
continueRecover(void)
{
    restore_savefile();

    /* update the thermometer */
    if (in.Dialog && !(in.Recover % 4))
        itemizeThermo(invalItem);

    if (in.Recover <= MAX_RECOVER_COUNT)
        return;

    endRecover();

    if (saveRezStrings())
        return;

    note(noErr, alidNote, P_STRING_CONV("OK: Recovery succeeded"));
}

/* no messages from here (since we might be quitting) */
static void
endRecover(void)
{
    in.Recover = 0;

    oldCursor = curs_Init;
    SetCursor(&qd.arrow);

    /* clean up abandoned files */
    if (gameRefNum >= 0)
        (void) FSClose(gameRefNum);

    if (levRefNum >= 0)
        (void) FSClose(levRefNum);

    if (saveRefNum >= 0) {
        (void) FSClose(saveRefNum);
        (void) FlushVol((StringPtr) 0L, vRefNum);
        /* its corrupted so trash it ... */
        (void) HDelete(vRefNum, dirID, savename);
    }

    saveRefNum = gameRefNum = levRefNum = -1;

    /* close the progress thermometer dialog */
    in.Dialog = 0;
    CloseDialog(DLGTHM);
    DisposeHandle(dlgThermo.items);
    memActivity++;
}

/* add friendly, non-essential resource strings to save file */
static short
saveRezStrings(void)
{
    short sRefNum;
    StringHandle strHnd;
    short i, rezID;
    unsigned char plName[256];
    short skip;
    int pid = hpid;

    HCreateResFile(vRefNum, dirID, savename);

    sRefNum = HOpenResFile(vRefNum, dirID, savename, fsRdWrPerm);
    if (sRefNum == -1) {
        note(noErr, alidNote, P_STRING_CONV("OK: Minor resource map error"));
        return 1;
    }

    /* savename is "save/<pid><plname>" (pascal); skip "save/" and the pid
       digits to isolate the player name.  Work on a copy: savename is still
       needed intact by endRecover should a later recovery fail. */
    skip = 5; /* "save/" */
    do {
        skip++;
        pid /= 10;
    } while (pid);
    plName[0] = (*savename > skip) ? (unsigned char) (*savename - skip) : 0;
    BlockMove(savename + 1 + skip, plName + 1, plName[0]);

    for (i = 1; i <= 2; i++) {
        switch (i) {
        case 1:
            rezID = PLAYER_NAME_RES_ID;
            strHnd = NewString(*(Str255 *) plName);
            break;

        case 2:
            rezID = APP_NAME_RES_ID;
            strHnd = NewString(*(Str255 *) P_STRING_CONV("NetHack"));
            break;
        }

        if (!strHnd) {
            note(noErr, alidNote, P_STRING_CONV("OK: Minor 'STR ' resource error"));
            CloseResFile(sRefNum);
            return 1;
        }

        /* should check for errors... */
        AddResource((Handle) strHnd, 'STR ', rezID, *(Str255 *) P_EMPTY_STRING);
    }

    memActivity++;

    /* should check for errors... */
    CloseResFile(sRefNum);
    return 0;
}

static void
set_levelfile_name(long lev)
{
    unsigned char *tf;

    /* find the dot, scanning backward; bounded so that a dotless name
       (which basenameFileFilter should have rejected) cannot walk off the
       front of lock[] */
    for (tf = (lock + *lock); tf > lock && *tf != '.'; tf--)
        ;
    if (tf == lock) {
        endRecover();
        note(noErr, alidNote, P_STRING_CONV("Sorry: File Name Error"));
        return;
    }

    /* replace everything after the dot with the level number (pascal) */
    lock[0] = (unsigned char) (tf - lock); /* length through the dot */
    NumToString(lev, *(Str255 *) tf);
    lock[0] += *tf;
    *tf = '.';
}

static short
open_levelfile(long lev)
{
    OSErr openErr;
    short fRefNum;

    set_levelfile_name(lev);
    if (!in.Recover)
        return (-1);

    if ((openErr = HOpen(vRefNum, dirID, lock, fsRdWrPerm, &fRefNum))
        && (openErr != fnfErr)) {
        endRecover();
        note(noErr, alidNote, P_STRING_CONV("Sorry: File Open Error"));
        return (-1);
    }

    return (openErr ? -1 : fRefNum);
}

static short
create_savefile(unsigned char *savename)
{
    short fRefNum;

    /* translate savename to a pascal string (in place) */
    {
        unsigned char *pC;
        short nameLen;

        for (pC = savename; *pC; pC++)
            ;

        nameLen = pC - savename;

        for (; pC > savename; pC--)
            *pC = *(pC - 1);

        *savename = nameLen;
    }

    if (HCreate(vRefNum, dirID, savename, MAC_CREATOR, SAVE_TYPE)
        || HOpen(vRefNum, dirID, savename, fsRdWrPerm, &fRefNum)) {
        endRecover();
        note(noErr, alidNote, P_STRING_CONV("Sorry: File Create Error"));
        return (-1);
    }

    return fRefNum;
}

static void
copy_bytes(short inRefNum, short outRefNum)
{
    char *buf = (char *) pIOBuf;
    long bufSiz = pBytes->memIOBuf;

    long nfrom, nto;

    do {
        nfrom = read_levelfile(inRefNum, buf, bufSiz);
        if (!in.Recover)
            return;

        nto = write_savefile(outRefNum, buf, nfrom);
        if (!in.Recover)
            return;

        if (nto != nfrom) {
            endRecover();
            note(noErr, alidNote, P_STRING_CONV("Sorry: File Copy Error"));
            return;
        }
    } while (nfrom == bufSiz);
}

/* One step of the recovery state machine.
 *
 * Called once per null event from eventLoop -> continueRecover, so the GUI
 * (thermometer, menus, DAs) stays responsive during a long recovery.
 * in.Recover is a 1-based step counter, not just an "active" flag:
 *   step 1            read the level-0 header (hpid, savelev, savename,
 *                     version), create the save file, copy the current
 *                     level (savelev) and game state into it;
 *   steps 2..n        copy one numbered level file per call (savelev is
 *                     skipped -- already copied; missing levels are fine);
 *   last step         close the save file (continueRecover then calls
 *                     endRecover and saveRezStrings).
 * Any error calls endRecover(), which clears in.Recover and deletes the
 * partial save file.  savelev is static: it carries the current-level
 * number, read in step 1, across subsequent calls.
 */
static void
restore_savefile(void)
{
    static int savelev;
    long saveTemp, lev;
    xint8 levc;
    struct version_info version_data;

    /* level 0 file contains:
     *	pid of creating process (ignored here)
     *	level number for current level of save file
     *	name of save file nethack would have created
     *	and game state
     */

    lev = in.Recover - 1;
    if (lev == 0L) {
        gameRefNum = open_levelfile(0L);

        if (in.Recover)
            (void) read_levelfile(gameRefNum, (Ptr) &hpid, sizeof(hpid));

        if (in.Recover)
            saveTemp =
                read_levelfile(gameRefNum, (Ptr) &savelev, sizeof(savelev));

        if (in.Recover && (saveTemp != sizeof(savelev))) {
            endRecover();
            note(noErr, alidNote,
                 P_STRING_CONV("Sorry: \"checkpoint\" was not enabled"));
            return;
        }

        if (in.Recover)
            (void) read_levelfile(gameRefNum, (Ptr) savename,
                                  sizeof(savename));
        if (in.Recover)
            (void) read_levelfile(gameRefNum, (Ptr) &version_data,
                                  sizeof version_data);

        /* save file should contain:
         *	current level (including pets)
         *	(non-level-based) game state
         *	other levels
         */
        if (in.Recover)
            saveRefNum = create_savefile(savename);

        if (in.Recover)
            levRefNum = open_levelfile(savelev);

        if (in.Recover)
            (void) write_savefile(saveRefNum, (Ptr) &version_data,
                                  sizeof version_data);
        if (in.Recover)
            copy_bytes(levRefNum, saveRefNum);

        if (in.Recover)
            close_file(&levRefNum);

        if (in.Recover)
            unlink_file(lock);

        if (in.Recover)
            copy_bytes(gameRefNum, saveRefNum);

        if (in.Recover)
            close_file(&gameRefNum);

        if (in.Recover)
            set_levelfile_name(0L);

        if (in.Recover)
            unlink_file(lock);
    } else if (lev != savelev) {
        levRefNum = open_levelfile(lev);
        if (levRefNum >= 0) {
            /* any or all of these may not exist */
            levc = (xint8) lev;

            (void) write_savefile(saveRefNum, (Ptr) &levc, sizeof(levc));

            if (in.Recover)
                copy_bytes(levRefNum, saveRefNum);

            if (in.Recover)
                close_file(&levRefNum);

            if (in.Recover)
                unlink_file(lock);
        }
    }

    if (in.Recover == MAX_RECOVER_COUNT)
        close_file(&saveRefNum);

    if (in.Recover)
        in.Recover++;
}

static long
read_levelfile(short rdRefNum, Ptr bufPtr, long count)
{
    OSErr rdErr;
    long rdCount = count;

    if ((rdErr = FSRead(rdRefNum, &rdCount, bufPtr)) && (rdErr != eofErr)) {
        endRecover();
        note(noErr, alidNote, P_STRING_CONV("Sorry: File Read Error"));
        return (-1L);
    }

    return rdCount;
}

static long
write_savefile(short wrRefNum, Ptr bufPtr, long count)
{
    long wrCount = count;

    if (FSWrite(wrRefNum, &wrCount, bufPtr)) {
        endRecover();
        note(noErr, alidNote, P_STRING_CONV("Sorry: File Write Error"));
        return (-1L);
    }

    return wrCount;
}

static void
close_file(short *pFRefNum)
{
    if (FSClose(*pFRefNum) || FlushVol((StringPtr) 0L, vRefNum)) {
        endRecover();
        note(noErr, alidNote, P_STRING_CONV("Sorry: File Close Error"));
        return;
    }

    *pFRefNum = -1;
}

static void
unlink_file(unsigned char *filename)
{
    if (HDelete(vRefNum, dirID, filename)) {
        endRecover();
        note(noErr, alidNote, P_STRING_CONV("Sorry: File Delete Error"));
        return;
    }
}

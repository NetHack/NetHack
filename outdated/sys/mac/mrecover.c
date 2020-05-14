/* NetHack 3.6	mrecover.c	$NHDT-Date: 1432512798 2015/05/25 00:13:18 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
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
 * misc: sizeof(int): 2, "\p": unsigned char, enum size varies,
 *   prototypes required, type checking enforced, no optimizers,
 *   FAR CODE [no], FAR DATA [no], SEPARATE STRS [no], single segment,
 *   short macsbug symbols
 */

/*
 * To do (maybe, just maybe):
 * - Merge with the code in util/recover.c.
 * - Document launch (e.g. GUI equivalent of 'recover basename').
 * - Drag and drop.
 * - Internal memory tweaks (stack and heap usage).
 * - Use status file to allow resuming aborted recoveries.
 * - Bundle 'LEVL' files with recover (easier document launch).
 * - Prohibit recovering games "in progress".
 * - Share AppleEvents with NetHack to auto-recover crashed games.
 */

#include "config.h"

/**** Toolbox defines ****/

/* MPW C headers (99.44% pure) */
#include <Errors.h>
#include <OSUtils.h>
#include <Resources.h>
#include <Files.h>
#ifdef applec
#include <SysEqu.h>
#endif
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

#ifdef THINK /* glue for System 7 Icon Family call (needed by Think C 5.0.4) \
                */
pascal OSErr GetIconSuite(Handle *theIconSuite, short theResID,
                          long selector) = { 0x303C, 0x0501, 0xABC9 };
#endif

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

/* variables from util/recover.c */
#define SAVESIZE FILENAME
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

main()
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
        ParamText("\pAbort: System 6.0 is required", "\p", "\p", "\p");
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
warmup()
{
    short i;

    /* pre-System 7 MultiFinder hack for smooth launch */
    for (i = 0; i < 10; i++) {
        if (WaitNextEvent(osMask, &wnEvt, 2L, (RgnHandle) 0L))
            if (((wnEvt.message & osEvtMessageMask) >> 24)
                == suspendResumeMessage)
                in.Front = (wnEvt.message & resumeFlag);
    }

#if 0 // ???
	/* clear out the Finder info */
	{
		short	message, count;

		CountAppFiles(&message, &count);
		while(count)
			ClrAppFiles(count--);
	}
#endif

    /* fill out the notification template */
    nmt.nmr.qType = nmType;
    nmt.nmr.nmMark = 1;
    nmt.nmr.nmSound = (Handle) -1L; /* system beep */
    nmt.nmr.nmStr = nmt.nmBuf;
    nmt.nmr.nmResp = nmCompletionUPP;
    nmt.nmr.nmPending = (long) &in.Notify;

#if 1
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
#else
    /* prepend app name (31 chars or less) to notification buffer */
    {
        short apRefNum;
        Handle apParams;

        GetAppParms(*(Str255 *) &nmt.nmBuf, &apRefNum, &apParams);
    }
#endif

    /* add formatting (two line returns) */
    nmt.nmBuf[++(nmt.nmBuf[0])] = '\r';
    nmt.nmBuf[++(nmt.nmBuf[0])] = '\r';

    /**** note() is usable now but not aesthetically complete ****/

    /* get notification icon */
    if (sysEnv.systemVersion < 0x0700) {
        if (!(nmt.nmr.nmIcon = GetResource('SICN', iconNotifyID)))
            note(nilHandleErr, 0, "\pNil SICN Handle");
    } else {
        if (GetIconSuite(&nmt.nmr.nmIcon, iconNotifyID, ics_1_and_4))
            note(nilHandleErr, 0, "\pBad Icon Family");
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
            note(nilHandleErr, 0, "\pNil CURS Handle");

        cPtr[i] = *cHnd;
    }

    /* get the 'vers' 1 long (Get Info) string - About Recover... */
    {
        versXHandle vHnd;

        if (!(vHnd = (versXHandle) GetResource('vers', 1)))
            note(nilHandleErr, 0, "\pNil vers Handle");

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
            note(nilHandleErr, 0, "\pNil MENU Handle");

        /* expand the apple menu */
        if (i == menuApple)
            AddResMenu(mHnd[menuApple], 'DRVR');

        InsertMenu(mHnd[i], 0);
    }

    /* pre-emptive memory check */
    {
        memBytesHandle hBytes;
        Size grow;

        if (!(hBytes = (memBytesHandle) GetResource('memB', membID)))
            note(nilHandleErr, 0, "\pNil Memory Handle");

        pBytes = *hBytes;

        if (MaxMem(&grow) < pBytes->memPreempt)
            note(memFullErr, 0, "\pMore Memory Required\rTry adding 16k");

        memActivity = pBytes->memCleanup; /* force initial cleanup */
    }

    /* get the I/O buffer */
    if (!(pIOBuf = NewPtr(pBytes->memIOBuf)))
        note(memFullErr, 0, "\pNil I/O Pointer");
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
        note(nilHandleErr, 0, "\pNil Template Handle");

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
    ((notifPtr) pNMR)->nmDispose = 1; /* allow DisposPtr() */
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
            noteErrorMessage(msg, "\pOut of Memory");
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
                noteErrorMessage(msg, "\pNil New Pointer");

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
    ParamText(msg, "\p", "\p", "\p");
    (void) Alert(alertID, (ModalFilterUPP) 0L);
    ResetAlrtStage();

    memActivity++;
}

static void
adjustGUI()
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
    if (frontWindow = (WindowPeek) FrontWindow())
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
    if (useArrow = (!in.Recover || (newMenubar == mbarDA)))
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
adjustMemory()
{
    Size grow;

    memActivity = 0;

    if (MaxMem(&grow) < pBytes->memWarning)
        note(noErr, alidNote, "\pWarning: Memory is running low");

    (void) ResrvMem((Size) FreeMem()); /* move all handles high */
}

/* show memory stats: FreeMem, MaxBlock, PurgeSpace, and StackSpace */
static void
optionMemStats()
{
    unsigned char *pFormat = "\pFree:#k  Max:#k  Purge:#k  Stack:#k";
    char *pSub = "#"; /* not a pascal string */
    unsigned char nBuf[16];
    long nStat, contig;
    Handle strHnd;
    long nOffset;
    short i;

    if (wnEvt.modifiers & shiftKey)
        adjustMemory();

    if (!(strHnd = NewHandle((Size) 128))) {
        note(noErr, alidNote, "\pOops: Memory stats unavailable!");
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

        NumToString((nStat >> 10), *(Str255 *) &nBuf);

        **strHnd += nBuf[0] - 1;
        nOffset =
            Munger(strHnd, nOffset, (Ptr) pSub, 1L, (Ptr) &nBuf[1], nBuf[0]);
    }

    MoveHHi(strHnd);
    HLock(strHnd);
    note(noErr, alidNote, (unsigned char *) *strHnd);
    DisposHandle(strHnd);
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
            unsigned char daName[32];

            GetItem(mHnd[menuApple], menuItem, *(Str255 *) &daName);
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

            if (frontWindow = (WindowPeek) FrontWindow())
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
eventLoop()
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

                DisposPtr((Ptr) pNMQ);
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
                offsetPt = *(Point *) &(whichWindow->portBits.bounds);
                OffsetRect(&boundsRect, -offsetPt.h, -offsetPt.v);

                *(Rect *) *thermoTHnd = boundsRect;
            } break;
            }
        } break;

        case keyDown: {
            char key = (wnEvt.message & charCodeMask);

            if (wnEvt.modifiers & cmdKey) {
                if (key == '.') {
                    if (in.Recover) {
                        endRecover();
                        note(noErr, alidNote, "\pSorry: Recovery aborted");
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
        }

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
cooldown()
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

    GetDItem(DLGTHM, uitmThermo, &iTyp, &iHnd, &iRct);

    switch (itemMode) {
    case initItem:
        SetDItem(DLGTHM, uitmThermo, iTyp, (Handle) drawThermoUPP, &iRct);
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
beginRecover()
{
    SFTypeList levlType = { 'LEVL' };
    SFReply sfGetReply;

    SFGetFile(sfGetWhere, "\p", basenameFileFilterUPP, 1, levlType,
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
            note(noErr, alidNote, "\pSorry: Bad File Info");
            return;
        }

        dirID = catInfo.hFileInfo.ioFlParID;
    }

    /* open the progress thermometer dialog */
    (void) GetNewDialog(dlogProgress, (Ptr) &dlgThermo, (WindowPtr) -1L);
    if (ResError() || MemError())
        note(noErr, alidNote, "\pOops: Progress thermometer unavailable");
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
continueRecover()
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

    note(noErr, alidNote, "\pOK: Recovery succeeded");
}

/* no messages from here (since we might be quitting) */
static void
endRecover()
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
    DisposHandle(dlgThermo.items);
    memActivity++;
}

/* add friendly, non-essential resource strings to save file */
static short
saveRezStrings()
{
    short sRefNum;
    StringHandle strHnd;
    short i, rezID;
    unsigned char *plName;

    HCreateResFile(vRefNum, dirID, savename);

    sRefNum = HOpenResFile(vRefNum, dirID, savename, fsRdWrPerm);
    if (sRefNum <= 0) {
        note(noErr, alidNote, "\pOK: Minor resource map error");
        return 1;
    }

    /* savename and hpid get mutilated here... */
    plName = savename + 5; /* save/ */
    *savename -= 5;
    do {
        plName++;
        (*savename)--;
        hpid /= 10;
    } while (hpid);
    *plName = *savename;

    for (i = 1; i <= 2; i++) {
        switch (i) {
        case 1:
            rezID = PLAYER_NAME_RES_ID;
            strHnd = NewString(*(Str255 *) plName);
            break;

        case 2:
            rezID = APP_NAME_RES_ID;
            strHnd = NewString(*(Str255 *) "\pNetHack");
            break;
        }

        if (!strHnd) {
            note(noErr, alidNote, "\pOK: Minor \'STR \' resource error");
            CloseResFile(sRefNum);
            return 1;
        }

        /* should check for errors... */
        AddResource((Handle) strHnd, 'STR ', rezID, *(Str255 *) "\p");
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

    /* find the dot.  this is guaranteed to happen. */
    for (tf = (lock + *lock); *tf != '.'; tf--, lock[0]--)
        ;

    /* append the level number string (pascal) */
    if (tf > lock) {
        NumToString(lev, *(Str255 *) tf);
        lock[0] += *tf;
        *tf = '.';
    } else /* huh??? */
    {
        endRecover();
        note(noErr, alidNote, "\pSorry: File Name Error");
    }
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
        note(noErr, alidNote, "\pSorry: File Open Error");
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
        note(noErr, alidNote, "\pSorry: File Create Error");
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
            note(noErr, alidNote, "\pSorry: File Copy Error");
            return;
        }
    } while (nfrom == bufSiz);
}

static void
restore_savefile()
{
    static int savelev;
    long saveTemp, lev;
    xchar levc;
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
            note(noErr, alidNote, "\pSorry: \"checkpoint\" was not enabled");
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
            levc = (xchar) lev;

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
        note(noErr, alidNote, "\pSorry: File Read Error");
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
        note(noErr, alidNote, "\pSorry: File Write Error");
        return (-1L);
    }

    return wrCount;
}

static void
close_file(short *pFRefNum)
{
    if (FSClose(*pFRefNum) || FlushVol((StringPtr) 0L, vRefNum)) {
        endRecover();
        note(noErr, alidNote, "\pSorry: File Close Error");
        return;
    }

    *pFRefNum = -1;
}

static void
unlink_file(unsigned char *filename)
{
    if (HDelete(vRefNum, dirID, filename)) {
        endRecover();
        note(noErr, alidNote, "\pSorry: File Delete Error");
        return;
    }
}

/* NetHack 5.0	macwin.c	$NHDT-Date: 1432512796 2015/05/25 00:13:16 $  $NHDT-Branch: master $:$NHDT-Revision: 1.26 $ */
/* Copyright (c) Jon W{tte, Hao-Yang Wang, Jonathan Handler 1992. */
/* NetHack may be freely redistributed.  See license for details. */

/**********************************************************************
 *	Imported variables and functions
 */

#include "hack.h"
#include "func_tab.h"
#include "macwin.h"
#include "mactty.h"
#include "wintty.h"
#include "mactile.h"
#include "macmap.h"

extern short gTileMenuNeedsUpdate;
extern int extcmd_via_menu(void); /* cmd.c */

#include <LowMem.h>
#include <AppleEvents.h>
#include <Gestalt.h>
#include <TextUtils.h>
#include <DiskInit.h>

/**********************************************************************
 *	Local variables and functions
 */

static void GeneralKey(EventRecord *, WindowPtr);
static void macKeyMenu(EventRecord *, WindowPtr);
static void macKeyText(EventRecord *, WindowPtr);

static void macClickMessage(EventRecord *, WindowPtr);
static void macClickTerm(EventRecord *, WindowPtr);
static void macClickMenu(EventRecord *, WindowPtr);
static void macClickText(EventRecord *, WindowPtr);

static short macDoNull(EventRecord *, WindowPtr);
static short macUpdateMessage(EventRecord *, WindowPtr);
static short macUpdateMenu(EventRecord *, WindowPtr);
static short GeneralUpdate(EventRecord *, WindowPtr);

static void macCursorTerm(EventRecord *, WindowPtr, RgnHandle);
static void GeneralCursor(EventRecord *, WindowPtr, RgnHandle);

static void TextUpdate(NhWindow *wind);
static void MenwUpdate(NhWindow *wind);
static void record_menu_line_style(NhWindow *, short, short, int, int);

NhWindow *theWindows = (NhWindow *) 0;
Cursor qdarrow;

/* Borrowed from the Mac tty port */
extern WindowPtr _mt_window;

#define SBARWIDTH 15
#define SBARHEIGHT 15

/*
 * We put a TE on the message window for the "top line" queries.
 * top_line is the TE that holds both the query and the user's
 * response.  The first topl_query_len characters in top_line are
 * the query, the rests are the response.  topl_resp is the valid
 * response to a yn query, while topl_resp[topl_def_idx] is the
 * default response to a yn query.
 */
static TEHandle top_line = (TEHandle) nil;
static int topl_query_len;
static int topl_def_idx = -1;
static char topl_resp[BUFSZ] = "";
/* extended-command TAB completion: the prefix typed before the first TAB
   and a counter that cycles through the matches; reset by enter_topl_mode
   and by any non-TAB key (see topl_key_body) */
static int topl_extcmd_tabi = 0;
static char topl_extcmd_prefix[BUFSZ];
/* previous WIN_MESSAGE line was transient (ATR_NOHISTORY), so the next
   transient line replaces it in place; mac_clear_nhwindow resets it */
static char gLastMsgTransient = 0;
static short gLastTransientLines = 0; /* rows the last transient stored */

#define CHAR_ANY '\n'

/*
 * inSelect means we have a menu window up for selection or
 * something similar. It makes the window with win number ==
 * inSelect a movable modal (unfortunately without the border)
 * and clicking the close box forces an RET into the key
 * buffer. Don't forget to set inSelect to WIN_ERR when you're
 * done...
 */
static winid inSelect = WIN_ERR;

/*
 * The key queue ring buffer where Read is where to take from,
 * Write is where next char goes and count is queue depth.
 */
static unsigned char keyQueue[QUEUE_LEN];
static int keyQueueRead = 0, keyQueueWrite = 0, keyQueueCount = 0;

static Boolean gClickedToMove = 0; /* For ObscureCursor */

static Point clicked_pos; /* For nh_poskey */
static int clicked_mod;
static Boolean cursor_locked = false;

static ControlActionUPP
    MoveScrollUPP; /* scrolling callback, init'ed in InitMac */

/* called from getpos.c during farlook; the flag is recorded but nothing
   reads it yet (cursor shaping during farlook is on the UI punch list) */
void
lock_mouse_cursor(Boolean new_cursor_locked)
{
    cursor_locked = new_cursor_locked;
}

/*
 * Add key to input queue, force means flush left and replace if full
 */
void
AddToKeyQueue(unsigned char ch, Boolean force)
{
    if (keyQueueCount < QUEUE_LEN) {
        keyQueue[keyQueueWrite++] = ch;
        keyQueueCount++;
    } else if (force) {
        keyQueue[keyQueueWrite++] = ch;
        keyQueueRead++;
        if (keyQueueRead >= QUEUE_LEN)
            keyQueueRead = 0;
        keyQueueCount = QUEUE_LEN;
    }
    if (keyQueueWrite >= QUEUE_LEN)
        keyQueueWrite = 0;
}

/*
 * Get key from queue
 */
unsigned char
GetFromKeyQueue(void)
{
    unsigned char ret;

    if (keyQueueCount) {
        ret = keyQueue[keyQueueRead++];
        keyQueueCount--;
        if (keyQueueRead >= QUEUE_LEN)
            keyQueueRead = 0;
    } else
        ret = 0;
    return ret;
}

/*
 * Cursor movement
 */
static RgnHandle gMouseRgn = (RgnHandle) 0;

/*
 * _Gestalt madness - we rely heavily on the _Gestalt glue, since we
 * don't check for the trap...
 */
MacFlags macFlags;

/*
 * The screen layouts on the small 512x342 screen need special cares.
 */
Boolean small_screen = 0;

#ifdef NHW_BASE
#undef NHW_BASE
#endif
#define NHW_BASE 0

static int filter_scroll_key(const int, NhWindow *);

static void DoScrollBar(Point, short, ControlHandle, NhWindow *);
static pascal void MoveScrollBar(ControlHandle, short);

typedef void (*CbFunc)(EventRecord *, WindowPtr);
typedef short (*CbUpFunc)(EventRecord *, WindowPtr);
typedef void (*CbCursFunc)(EventRecord *, WindowPtr, RgnHandle);

#define NUM_FUNCS 6
static const CbFunc winKeyFuncs[NUM_FUNCS] = { GeneralKey, GeneralKey,
                                               GeneralKey, GeneralKey,
                                               macKeyMenu, macKeyText };

static const CbFunc winClickFuncs[NUM_FUNCS] = {
    (CbFunc) macDoNull, macClickMessage, macClickTerm,
    macClickTerm,       macClickMenu,    macClickText
};

static const CbUpFunc winUpdateFuncs[NUM_FUNCS] = {
    macDoNull, macUpdateMessage, image_tty,
    image_tty, macUpdateMenu,    GeneralUpdate
};

static const CbCursFunc winCursorFuncs[NUM_FUNCS] = {
    (CbCursFunc) macDoNull, GeneralCursor, macCursorTerm,
    macCursorTerm,          GeneralCursor, GeneralCursor
};

static NhWindow *
GetNhWin(WindowPtr mac_win)
{
    if (mac_win == _mt_window) /* WRefCon points at the tty struct, not us */
        return theWindows;
    /* Map window uses MACMAP_REFCON, not an NhWindow pointer. */
    if (mac_win && GetWRefCon(mac_win) == MACMAP_REFCON
        && WIN_MAP != WIN_ERR) {
        return &theWindows[WIN_MAP];
    }
    else {
        NhWindow *aWin = (NhWindow *) GetWRefCon(mac_win);
        if (aWin >= theWindows && aWin < &theWindows[NUM_MACWINDOWS])
            return aWin;
    }
    return ((NhWindow *) nil);
}

Boolean
CheckNhWin(WindowPtr mac_win)
{
    return GetNhWin(mac_win) != nil;
}

static pascal OSErr
AppleEventHandler(const AppleEvent *inAppleEvent, AppleEvent *outAEReply,
                  long inRefCon)
{
    Size actualSize;
    DescType typeCode;
    AEEventID EventID;
    OSErr err;

    err = AEGetAttributePtr(inAppleEvent, keyEventIDAttr, typeType, &typeCode,
                            &EventID, sizeof(EventID), &actualSize);
    if (err == noErr) {
        switch (EventID) {
        default:
        case kAEOpenApplication:
            macFlags.gotOpen = 1;
        /* fall through */
        case kAEPrintDocuments:
            err = errAEEventNotHandled;
            break;
        case kAEQuitApplication:
            /* Flush key queue */
            keyQueueCount = keyQueueWrite = keyQueueRead = 0;
            AddToKeyQueue('S', 1);
            break;
        case kAEOpenDocuments: {
            FSSpec fss;
            FInfo fndrInfo;
            AEKeyword keywd;
            AEDescList docList;
            long index, itemsInList;

            if ((err = AEGetParamDesc(inAppleEvent, keyDirectObject,
                                      typeAEList, &docList)) != noErr
                || (err = AECountItems(&docList, &itemsInList)) != noErr) {
                if (err == errAEDescNotFound)
                    itemsInList = 0;
                else
                    break;
            }

            for (index = 1; index <= itemsInList; index++) {
                err = AEGetNthPtr(&docList, index, typeFSS, &keywd, &typeCode,
                                  (Ptr) &fss, sizeof(FSSpec), &actualSize);
                if (noErr != err)
                    break;

                err = FSpGetFInfo(&fss, &fndrInfo);
                if (noErr != err)
                    break;

                if (fndrInfo.fdType != SAVE_TYPE)
                    continue; /* only look at save files */

                process_openfile(fss.vRefNum, fss.parID, fss.name,
                                 fndrInfo.fdType);
                if (macFlags.gotOpen)
                    break; /* got our save file */
            }
            err = AEDisposeDesc(&docList);
            break;
        }
        }
    }

    /* Verify all required parameters for this event type are present */
    if (err == noErr) {
        err =
            AEGetAttributePtr(inAppleEvent, keyMissedKeywordAttr,
                              typeWildCard, &typeCode, NULL, 0, &actualSize);
        if (err == errAEDescNotFound)
            err = noErr;       /* got all the required parameters */
        else if (err == noErr) /* missed a required parameter */
            err = errAEEventNotHandled;
    }

    return err;
}

short win_fonts[NHW_TEXT + 1];


void
InitMac(void)
{
    short i;
    long l;
    Str255 volName;

    if (LMGetDefltStack() < 256 * 1024L) {
        SetApplLimit((void *) ((long) LMGetCurStackBase() - (256 * 1024L)));
    }
    MaxApplZone();
    for (i = 0; i < 5; i++)
        MoreMasters();

    /* Zero the A5-relative QD globals (A5-4..A5-206): the Segment Loader
       doesn't, and ROM Color QD reads them, so stale values crash relaunch */
    {
        char *a5 = (char *) SetCurrentA5();
        memset(a5 - 206, 0, 206);
    }
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    InitDialogs(0L);
    TEInit();

    memset(&macFlags, 0, sizeof(macFlags));
    if (!Gestalt(gestaltOSAttr, &l))
        macFlags.hasDebugger = (l & (1 << gestaltSysDebuggerSupport)) ? 1 : 0;
    if (!Gestalt(gestaltQuickdrawVersion, &l))
        macFlags.color = (l >= gestalt8BitQD) ? 1 : 0;

    gMouseRgn = NewRgn();
    if (!gMouseRgn)
        error("InitMac: NewRgn (mouse region) failed");
    InitCursor();
    qdarrow = qd.arrow;
    ObscureCursor();

    MoveScrollUPP = NewControlActionUPP(MoveScrollBar);
    if (!MoveScrollUPP)
        error("InitMac: NewControlActionUPP failed");

    /* Set up base fonts for all window types */
    GetFNum(P_STRING_CONV("HackFont"), &i);
    if (i == 0)
        i = kFontIDMonaco;
    win_fonts[NHW_BASE] = win_fonts[NHW_MAP] = win_fonts[NHW_STATUS] = i;
    win_fonts[NHW_MENU] = i;   /* fixed-width so menu columns line up */
    GetFNum(P_STRING_CONV("PSHackFont"), &i);
    if (i == 0)
        i = kFontIDGeneva;
    win_fonts[NHW_MESSAGE] = i;
    win_fonts[NHW_TEXT] = kFontIDGeneva;

    macFlags.hasAE = 0;
    if (!Gestalt(gestaltAppleEventsAttr, &l)
        && (l & (1L << gestaltAppleEventsPresent))) {
        if (AEInstallEventHandler(kCoreEventClass, typeWildCard,
                                  NewAEEventHandlerUPP(AppleEventHandler), 0,
                                  FALSE) == noErr)
            macFlags.hasAE = 1;
    }

    GetVol(volName, &theDirs.dataRefNum);
    GetWDInfo(theDirs.dataRefNum, &theDirs.dataRefNum, &theDirs.dataDirID,
              &l);
    /* truncate at the first ':' (and to Str32) to get the bare volume name */
    if (volName[0] > 31)
        volName[0] = 31;
    for (l = 1; l <= volName[0]; l++) {
        if (volName[l] == ':') {
            volName[l] = 0;
            volName[0] = l - 1;
            break;
        }
    }
    BlockMove(volName, theDirs.dataName, volName[0] + 1);
    BlockMove(volName, theDirs.saveName, volName[0] + 1);
    BlockMove(volName, theDirs.levelName, volName[0] + 1);
    theDirs.saveRefNum = theDirs.levelRefNum = theDirs.dataRefNum;
    theDirs.saveDirID = theDirs.levelDirID = theDirs.dataDirID;

    /* Create the "record" file, if necessary */
    check_recordfile("");

    return;
}

/* options.c (pfxfn_font, under #ifdef MAC68K) calls this by name when the
   winprocs set_font_name macro is inactive (NO_CHANGE_COLOR builds). */
short
set_font_name(int window_type, char *font_name)
{
    return set_tty_font_name(window_type, font_name);
}

/* Change default window fonts. */
short
set_tty_font_name(int window_type, char *font_name)
{
    short fnum;
    Str255 new_font;

    if (window_type < NHW_BASE || window_type > NHW_TEXT)
        return general_failure;

    C2P(font_name, new_font);
    GetFNum(new_font, &(fnum));
    if (!fnum)
        return general_failure;
    win_fonts[window_type] = fnum;
    return noErr;
}

static Boolean in_topl_mode(void);

/* Height of the prompt-button strip; on small screens reserved only while
   buttons are drawn (--More-- queues a response without drawing one). */
static short
msg_bottom_inset(void)
{
    return (!small_screen || (in_topl_mode() && topl_resp[0])) ? SBARHEIGHT
                                                               : 0;
}

static void
DrawScrollbar(NhWindow *aWin)
{
    WindowPtr theWindow = aWin->its_window;
    Rect crect, wrect;
    Boolean vis;
    short val, lin, win_height;

    if (!aWin->scrollBar)
        return;
    crect = (**aWin->scrollBar).contrlRect;
    GetWindowPortBounds(aWin->its_window, &wrect);
    OffsetRect(&wrect, -wrect.left, -wrect.top);
    win_height = wrect.bottom - wrect.top;

    if (crect.top != wrect.top - 1 || crect.left != wrect.right - SBARWIDTH) {
        MoveControl(aWin->scrollBar, wrect.right - SBARWIDTH, wrect.top - 1);
    }
    {
        /* small-screen bar spans the full edge and never auto-hides */
        Boolean msg_small =
            small_screen && (aWin == theWindows + WIN_MESSAGE);
        short sb_bottom =
            msg_small ? (wrect.bottom + 1) : (wrect.bottom - SBARHEIGHT);

        if (crect.bottom != sb_bottom || crect.right != wrect.right + 1) {
            SizeControl(aWin->scrollBar, SBARWIDTH + 1,
                        sb_bottom - (wrect.top - 1));
        }
        vis = msg_small || (win_height > (50 + SBARHEIGHT));
    }
    if (vis != ((** aWin->scrollBar).contrlVis != 0)) {
        if (vis)
            ShowControl(aWin->scrollBar);
        else
            HideControl(aWin->scrollBar);
    }
    lin = aWin->y_size;
    if (aWin == theWindows + WIN_MESSAGE) {
        lin -= (win_height - msg_bottom_inset()) / aWin->row_height;
        if (lin < 0)
            lin = 0;
        val = 0; /* always have message scrollbar active */
    } else {
        lin -= win_height / aWin->row_height;
        if (lin < 0)
            lin = 0;
        if (lin)
            val = 0;   /* off-screen lines exist: activate */
        else
            val = 255; /* none: grey out */
    }
    SetControlMaximum(aWin->scrollBar, lin);
    HiliteControl(aWin->scrollBar, val);
    val = GetControlValue(aWin->scrollBar);
    if (val != aWin->scrollPos) {
        InvalWindowRect(theWindow, &wrect);
        aWin->scrollPos = val;
    }
}

#define MIN_HEIGHT 50

int
SanePositions(void)
{
    WindowPtr mapw = (WIN_MAP != WIN_ERR && theWindows[WIN_MAP].its_window)
                     ? theWindows[WIN_MAP].its_window : _mt_window;
    short left, top;
    int ix, numText = 0, numMenu = 0;
    int mbar_height = GetMBarHeight();
    BitMap qbitmap;
    Rect screenArea;
    WindowPtr theWindow;
    WindowPtr msgw   = theWindows[WIN_MESSAGE].its_window;
    WindowPtr statw  = _mt_window;
    Rect mr, statr;
    short msg_h, map_h, stat_h, content_left, content_w;

    screenArea = qd.screenBits.bounds;
    OffsetRect(&screenArea, -screenArea.left, -screenArea.top);

    /* status window was shrunk to its status rows at creation; read that height */
    GetWindowPortBounds(statw, &statr);
    stat_h = statr.bottom - statr.top;

    {
        short title_h = small_screen ? 0 : 20;   /* title bar + small gap */
        short y = mbar_height + (small_screen ? 2 : 4);
        short msg_top, map_top, stat_top, avail_map_h, avail_map_w;
        /* Honor saved window positions only on large screens, where windows
           are movable. Small (borderless) screens always get the clean stack
           since the windows can't be dragged/saved anyway. */
        Boolean honor = !small_screen;

        msg_h = 4 * theWindows[WIN_MESSAGE].row_height + 4;   /* ~4 lines */
        /* keep the big-screen bar past DrawScrollbar's auto-hide threshold */
        if (!small_screen
            && theWindows[WIN_MESSAGE].scrollBar && msg_h <= 50 + SBARHEIGHT)
            msg_h = 50 + SBARHEIGHT + 2;

        /* Fit the map into the space left by menu bar, message+status windows,
           and title bars; macmap_fit resizes the map window + viewport/backing */
        msg_top = y + title_h;
        map_top = msg_top + msg_h + 2 + title_h;
        avail_map_h = screenArea.bottom - map_top - (title_h + stat_h + 2);
        avail_map_w = screenArea.right - 4;
        macmap_fit(avail_map_w, avail_map_h);

        /* Re-read the fitted map size; center the stack on its width. */
        GetWindowPortBounds(mapw, &mr);
        map_h = mr.bottom - mr.top;
        content_w = mr.right - mr.left;
        content_left = (screenArea.right - content_w) / 2;
        if (content_left < 0) content_left = 0;

        /* Messages on top (honor a saved position on large screens). */
        if (!(honor && RetrievePosition(kMessageWindow, &top, &left))) {
            top = msg_top; left = content_left;
        }
        MoveWindow(msgw, left, top, 1);
        SizeWindow(msgw, content_w, msg_h, 1);
        if (theWindows[WIN_MESSAGE].scrollBar)
            DrawScrollbar(&theWindows[WIN_MESSAGE]);

        /* Map in the middle. */
        if (!(honor && RetrievePosition(kMapWindow, &top, &left))) {
            top = map_top; left = content_left;
        }
        MoveWindow(mapw, left, top, 1);

        /* Status on the bottom; keep it on-screen. */
        stat_top = map_top + map_h + 2 + title_h;
        if (stat_top + stat_h > screenArea.bottom)
            stat_top = screenArea.bottom - stat_h - 2;
        if (stat_top < mbar_height + 2)
            stat_top = mbar_height + 2;
        if (!(honor && RetrievePosition(kStatusWindow, &top, &left))) {
            top = stat_top; left = content_left;
        }
        MoveWindow(statw, left, top, 1);
        /* match map width so the status' right edge aligns; stat_h already
           carries the +2 frame allowance from creation */
        SizeWindow(statw, content_w, stat_h, 1);
        /* MoveWindow reset the port origin; restore the 1px frame inset and
           repaint so the status rows stay visible */
        SetPortWindowPort(statw);
        SetOrigin(-1, -1);
        {
            Rect sfull;
            GetWindowPortBounds(statw, &sfull);
            InvalWindowRect(statw, &sfull);
        }
    }

    /* Handle other windows (NHW_MENU / NHW_TEXT) */
    for (ix = 0; ix < NUM_MACWINDOWS; ix++) {
        if (ix != WIN_STATUS && ix != WIN_MESSAGE && ix != WIN_MAP
            && ix != BASE_WINDOW) {
            theWindow = theWindows[ix].its_window;
            if (theWindow && ((WindowPeek) theWindow)->visible) {
                int shift;
                if (((WindowPeek) theWindow)->windowKind
                    == WIN_BASE_KIND + NHW_MENU) {
                    if (!RetrievePosition(kMenuWindow, &top, &left)) {
                        top = mbar_height * 2;
                        left = 2;
                    }
                    top += (numMenu * mbar_height);
                    numMenu++;
                    shift = 20;
                } else {
                    if (!RetrievePosition(kTextWindow, &top, &left)) {
                        top = mbar_height * 2;
                        left = screenArea.right - 3
                               - (theWindow->portRect.right
                                  - theWindow->portRect.left);
                    }
                    top += (numText * mbar_height);
                    numText++;
                    shift = -20;
                }
                while (top > screenArea.bottom - MIN_HEIGHT) {
                    top -= screenArea.bottom - mbar_height * 2;
                    left += shift;
                }
                MoveWindow(theWindow, left, top, 1);
            }
        }
    }
    SelectWindow(mapw);
    return (0);
}

void
mac_init_nhwindows(int *argcp, char **argv)
{
    Rect r;

    {
        Rect scr = (*GetGrayRgn())->rgnBBox;
        small_screen =
            scr.bottom - scr.top <= (iflags.large_font ? 12 * 40 : 9 * 40);
    }

    InitMenuRes();

    theWindows = (NhWindow *) NewPtrClear(NUM_MACWINDOWS * sizeof(NhWindow));
    if (MemError())
        error("mac_init_nhwindows: Couldn't allocate memory for windows.");

    DimMenuBar();

    tty_init_nhwindows(argcp, argv);
    iflags.window_inited = TRUE;

    /* Enable color if the display supports it.
       _mt_in_color is set by tty_init_nhwindows via Gestalt check. */
    if (has_color(CLR_RED)) {
        iflags.use_color = TRUE;
        iflags.wc_color = TRUE;
    }

    mac_create_nhwindow(NHW_BASE);
    tty_create_nhwindow(NHW_MESSAGE);

    /* Only move/size if a SAVED position exists: Retrieve* leave outputs
       untouched on a miss, so acting unconditionally would use garbage */
    if (theWindows[NHW_MESSAGE].its_window
        && RetrievePosition(kMessageWindow, &r.top, &r.left)) {
        MoveWindow(theWindows[NHW_MESSAGE].its_window, r.left, r.top, false);
        if (RetrieveSize(kMessageWindow, r.top, r.left, &r.bottom, &r.right))
            SizeWindow(theWindows[NHW_MESSAGE].its_window, r.right, r.bottom,
                       true);
    }
    return;
}

winid
mac_create_nhwindow(int kind)
{
    int i;
    NhWindow *aWin;
    FontInfo fi;

    if (kind < NHW_BASE || kind > NHW_TEXT) {
        error("cre_win: Invalid kind %d.", kind);
        return WIN_ERR;
    }

    for (i = 0; i < NUM_MACWINDOWS; i++) {
        if (!theWindows[i].its_window)
            break;
    }
    if (i >= NUM_MACWINDOWS) {
        error("cre_win: Win full; freeing extras");
        for (i = 0; i < NUM_MACWINDOWS; i++) {
            /* only sacrifice hidden menu/text windows (and never WIN_INVEN) */
            if (IsWindowVisible(theWindows[i].its_window) || i == WIN_INVEN
                || (GetWindowKind(theWindows[i].its_window)
                        != WIN_BASE_KIND + NHW_MENU
                    && GetWindowKind(theWindows[i].its_window)
                           != WIN_BASE_KIND + NHW_TEXT))
                continue;
            mac_destroy_nhwindow(i);
            goto got1;
        }
        error("cre_win: Out of ids!");
        return WIN_ERR;
    }

got1:
    aWin = &theWindows[i];
    aWin->windowTextLen = 0L;
    aWin->scrollBar = (ControlHandle) 0;
    aWin->menuInfo = 0;
    aWin->menuSelected = 0;
    aWin->menuStyle = 0;
    aWin->miLen = 0;
    aWin->miSize = 0;
    aWin->menuChar = 'a';

    if (kind == NHW_MAP) {
        /* Map gets its own window, but still populate wintty's wins[i] slot so
           tty internals (e.g. docorner's wins[WIN_MAP] deref) stay safe. */
        if (i != tty_create_nhwindow(kind)) {
            mac_dprintf("cre_win: error creating kind %d", kind);
        }
        wins[i]->offy = 0; /* message box is in a separate window */
        if (!macmap_create(aWin)) {
            mac_dprintf("cre_win: macmap_create failed for NHW_MAP\n");
            /* Fall back to legacy _mt_window sharing for safety. */
            aWin->its_window = _mt_window;
        }
        {
            short x_sz, x_sz_p, y_sz, y_sz_p;
            get_tty_metrics(_mt_window, &x_sz, &y_sz, &x_sz_p, &y_sz_p,
                            &aWin->font_number, &aWin->font_size,
                            &aWin->char_width, &aWin->row_height);
        }
        /* Now that font + cell metrics are populated, do the deferred
           backing/tile-mode initialization with correct values. */
        if (aWin->its_window != _mt_window)
            macmap_finalize(aWin);
        return i;
    } else if (kind == NHW_BASE || kind == NHW_STATUS) {
        short x_sz, x_sz_p, y_sz, y_sz_p;
        if (kind != NHW_BASE) {
            if (i != tty_create_nhwindow(kind)) {
                mac_dprintf("cre_win: error creating kind %d", kind);
            }
        }
        aWin->its_window = _mt_window;
        get_tty_metrics(aWin->its_window, &x_sz, &y_sz, &x_sz_p, &y_sz_p,
                        &aWin->font_number, &aWin->font_size,
                        &aWin->char_width, &aWin->row_height);
        /* This window now shows ONLY the status lines; draw them at the top of
           the offscreen (offy = 0) so a plain origin shows them. */
        if (kind == NHW_STATUS && wins[i]) {
            short row_h = aWin->row_height;
            short rows  = (short) wins[i]->rows;       /* 2 or 3 */
            short content_h;
            short content_w = x_sz_p;
            if (rows < 1) rows = 2;                    /* defensive */
            content_h = rows * row_h;
            wins[i]->offy = 0;     /* status renders at offscreen rows 0..rows-1 */
            SetPortWindowPort(_mt_window);
            SizeWindow(_mt_window, content_w + 2, content_h + 2, 1);
            SetOrigin(-1, -1);     /* 1px frame inset only (no off_y slice) */
            Rect full;
            GetWindowPortBounds(_mt_window, &full);
            InvalWindowRect(_mt_window, &full);
            /* _mt_window is the status window now; retitle it */
            SetWTitle(_mt_window, P_STRING_CONV("Status"));
        }
        return i;
    }

    {
        short res_id = (kind == NHW_MESSAGE && small_screen)
                           ? kWindMsgBorderless
                           : (WIN_BASE_RES + kind);
        /* Menus need a color (CGrafPort) window so menucolors render; a plain
           GetNewWindow makes a 1-bit GrafPort where RGBForeColor is a no-op
           (TextFace still works, which is why headings were bold but color
           never showed). On a B&W screen a color window just draws B&W. */
        if (kind == NHW_MENU)
            aWin->its_window =
                (WindowPtr) GetNewCWindow(res_id, (WindowPtr) 0L, (WindowPtr) -1L);
        else
            aWin->its_window =
                GetNewWindow(res_id, (WindowPtr) 0L, (WindowPtr) -1L);
        if (!aWin->its_window) {
            error("cre_win: GetNewWindow %d failed", res_id);
            return WIN_ERR;
        }
    }
    SetWindowKind(aWin->its_window, WIN_BASE_KIND + kind);
    SetWRefCon(aWin->its_window, (long) aWin);
    if (!(aWin->windowText = NewHandle(TEXT_BLOCK))) {
        error("cre_win: NewHandle fail(%ld)", (long) TEXT_BLOCK);
        DisposeWindow(aWin->its_window);
        aWin->its_window = (WindowPtr) 0;
        return WIN_ERR;
    }
    aWin->x_size = aWin->y_size = 0;
    aWin->x_curs = aWin->y_curs = 0;
    aWin->drawn = TRUE;
    mac_clear_nhwindow(i);

    SetPortWindowPort(aWin->its_window);

    if (kind == NHW_MESSAGE) {
        /* override the WIND's "Message" title */
        SetWTitle(aWin->its_window, P_STRING_CONV("Messages"));
        aWin->font_number = win_fonts[NHW_MESSAGE];
        aWin->font_size = iflags.wc_fontsiz_message
                              ? iflags.wc_fontsiz_message
                              : iflags.large_font ? 12 : 9;
        if (!top_line) {
            const Rect out_of_scr = { 10000, 10000, 10100, 10100 };
            TextFont(aWin->font_number);
            TextSize(aWin->font_size);
            TextFace(bold);
            top_line = TENew(&out_of_scr, &out_of_scr);
            if (!top_line)
                error("cre_win: TENew (top_line) failed");
            TEActivate(top_line);
            TextFace(normal);
        }
    } else if (kind == NHW_MENU) {
        aWin->font_number = win_fonts[NHW_MENU];   /* fixed-width for alignment */
        aWin->font_size = iflags.wc_fontsiz_menu ? iflags.wc_fontsiz_menu
                          : iflags.wc_fontsiz_text ? iflags.wc_fontsiz_text : 9;
    } else {
        aWin->font_number = win_fonts[NHW_TEXT];
        aWin->font_size = iflags.wc_fontsiz_text ? iflags.wc_fontsiz_text : 9;
    }

    TextFont(aWin->font_number);
    TextSize(aWin->font_size);

    GetFontInfo(&fi);
    aWin->ascent_height = fi.ascent + fi.leading;
    aWin->row_height = aWin->ascent_height + fi.descent;
    aWin->char_width = fi.widMax;

    if (kind == NHW_MENU || kind == NHW_TEXT || kind == NHW_MESSAGE) {
        Rect r;

        GetWindowPortBounds(aWin->its_window, &r);
        r.right -= (r.left - 1);
        r.left = r.right - SBARWIDTH;
        r.bottom -= (r.top
                     + ((kind == NHW_MESSAGE && small_screen) ? -1
                                                              : SBARHEIGHT));
        r.top = -1;
        aWin->scrollBar =
            NewControl(aWin->its_window, &r, P_EMPTY_STRING,
                       (r.bottom > r.top + 50), 0, 0, 0, 16, 0L);
        aWin->scrollPos = 0;
    }
    return i;
}

void
mac_clear_nhwindow(winid win)
{
    long l;
    Rect r;
    NhWindow *aWin = &theWindows[win];
    WindowPtr theWindow = aWin->its_window;

    if (win < 0 || win >= NUM_MACWINDOWS || !theWindow) {
        error("clr_win: Invalid win %d.", win);
        return;
    }
    if (win == WIN_MAP) {
        macmap_clear(aWin);
        return;
    }
    if (theWindow == _mt_window) {
        tty_clear_nhwindow(win);
        return;
    }
    if (!aWin->drawn)
        return;

    SetPortWindowPort(theWindow);
    GetWindowPortBounds(theWindow, &r);
    OffsetRect(&r, -r.left, -r.top);
    if (aWin->scrollBar)
        r.right -= SBARWIDTH;

    switch (GetWindowKind(theWindow) - WIN_BASE_KIND) {
    case NHW_MESSAGE:
        gLastMsgTransient = 0;   /* a clear invalidates the transient-line state */
        /* Trim old messages to msg_history limit: find the offset past the Nth
           CR, then discard everything before it with one BlockMove */
        {
            long off = 0;
            int trimmed = 0;
            int lines_to_trim = aWin->y_size - iflags.msg_history;
            if (lines_to_trim > 0) {
                long tlen = aWin->windowTextLen;
                HLock(aWin->windowText);
                {
                    char *p = *aWin->windowText;
                    while (trimmed < lines_to_trim && off < tlen) {
                        if (p[off] == CHAR_CR)
                            trimmed++;
                        off++;
                    }
                    if (off > 0 && off <= tlen) {
                        aWin->windowTextLen -= off;
                        BlockMove(p + off, p, aWin->windowTextLen);
                    }
                }
                HUnlock(aWin->windowText);
                aWin->y_size -= trimmed;
            }
            aWin->last_more_lin = aWin->y_size;
            aWin->save_lin = aWin->y_size;
            /* continuous log: trim history, keep the view on the newest
               lines (jumping the last line to the top left a stale row) */
            {
                /* row_height is 0 during cre_win's initial clear */
                short visible =
                    (aWin->row_height > 0)
                        ? (r.bottom - r.top - msg_bottom_inset())
                              / aWin->row_height
                        : 0;
                short floor_pos = visible ? aWin->y_size - visible : 0;
                if (floor_pos < 0)
                    floor_pos = 0;
                if (!trimmed && aWin->scrollPos == floor_pos)
                    return;   /* nothing moved, nothing to redraw */
                aWin->scrollPos = floor_pos;
            }
        }
        break;
    case NHW_MENU:
        if (aWin->menuInfo) {
            DisposeHandle((Handle) aWin->menuInfo);
            aWin->menuInfo = NULL;
        }
        if (aWin->menuSelected) {
            DisposeHandle((Handle) aWin->menuSelected);
            aWin->menuSelected = NULL;
        }
        if (aWin->menuStyle) {
            DisposeHandle(aWin->menuStyle);
            aWin->menuStyle = NULL;
        }
        aWin->menuChar = 'a';
        aWin->miSelLen = 0;
        aWin->miLen = 0;
        aWin->miSize = 0;
    /* Fall-Through */
    default:
        SetHandleSize(aWin->windowText, TEXT_BLOCK);
        aWin->windowTextLen = 0L;
        aWin->x_size = 0;
        aWin->y_size = 0;
        aWin->scrollPos = 0;
        break;
    }
    if (aWin->scrollBar) {
        SetControlMaximum(aWin->scrollBar, aWin->y_size);
        SetControlValue(aWin->scrollBar, aWin->scrollPos);
    }
    aWin->y_curs = 0;
    aWin->x_curs = 0;
    aWin->drawn = FALSE;
    InvalWindowRect(theWindow, &r);
}

static Boolean
ClosingWindowChar(const int c)
{
    return (c == CHAR_ESC || c == CHAR_BLANK || c == CHAR_LF || c == CHAR_CR);
}

static Boolean
in_topl_mode(void)
{
    Rect rect;
    WindowPtr w;

    /* Validate BEFORE dereferencing: on exit mac_destroy_nhwindow sets
       WIN_MESSAGE = WIN_ERR (-1), and this is still reached from the event
       loop; theWindows[-1].its_window then reads garbage and GetWindowPortBounds
       faults (bus error in in_topl_mode). */
    if (WIN_MESSAGE == WIN_ERR || !top_line)
        return FALSE;
    w = theWindows[WIN_MESSAGE].its_window;
    if (!w)
        return FALSE;
    GetWindowPortBounds(w, &rect);
    OffsetRect(&rect, -rect.left, -rect.top);
    return ((*top_line)->viewRect.left < rect.right);
}

#define BTN_IND 2
#define BTN_W 40
#define BTN_H (SBARHEIGHT - 3)

static void
topl_resp_rect(int resp_idx, Rect *r)
{
    Rect rect;

    GetWindowPortBounds(theWindows[WIN_MESSAGE].its_window, &rect);
    OffsetRect(&rect, -rect.left, -rect.top);
    r->left = (BTN_IND + BTN_W) * resp_idx + BTN_IND;
    r->right = r->left + BTN_W;
    r->bottom = rect.bottom - 1;
    r->top = r->bottom - BTN_H;
    return;
}

/* Invalidate the yn-button strip (len button slots) and clear the
   stored responses + default index; callers set new ones afterwards
   if they need any. */
static void
invalidate_topl_buttons(int len)
{
    Rect frame;

    topl_resp_rect(0, &frame);
    frame.right = (BTN_IND + BTN_W) * len + BTN_IND;
    InvalWindowRect(theWindows[WIN_MESSAGE].its_window, &frame);
    memset(topl_resp, 0, sizeof topl_resp);
    topl_def_idx = -1;
}

void
enter_topl_mode(char *query)
{
    if (in_topl_mode())
        return;

    /* Clear any leftover button state from a previous prompt */
    if (topl_resp[0])
        invalidate_topl_buttons(strlen(topl_resp));

    putstr(WIN_MESSAGE, ATR_BOLD, query);

    topl_extcmd_tabi = 0;
    topl_query_len = strlen(query);
    (*top_line)->selStart = topl_query_len;
    (*top_line)->selEnd = topl_query_len;
    (*top_line)->viewRect.left = 0;
    PtrToXHand(query, (*top_line)->hText, topl_query_len);
    TECalText(top_line);

    DimMenuBar();
    mac_display_nhwindow(WIN_MESSAGE, FALSE);
}

void
leave_topl_mode(char *answer) /* answer must have room for BUFSZ bytes */
{
    /*unsigned*/ char *ap, *bp;

    int ans_len = (*top_line)->teLength - topl_query_len;
    NhWindow *aWin = theWindows + WIN_MESSAGE;

    if (!in_topl_mode())
        return;

    /* Cap length of reply */
    if (ans_len >= BUFSZ)
        ans_len = BUFSZ - 1;

    /* remove unprintables from the answer */
    HLock((*top_line)->hText);
    for (ap = *(*top_line)->hText + topl_query_len, bp = answer; ans_len > 0;
         ans_len--, ap++) {
        if (*ap >= ' ' && *ap < 128) {
            *bp++ = *ap;
        }
    }
    *bp = 0;
    HUnlock((*top_line)->hText);

    if (aWin->windowTextLen
        && (*aWin->windowText)[aWin->windowTextLen - 1] == CHAR_CR) {
        --aWin->windowTextLen;
        --aWin->y_size;
    }
    putstr(WIN_MESSAGE, ATR_BOLD, answer);

    /* Invalidate the button area so stale buttons get erased */
    if (topl_resp[0])
        invalidate_topl_buttons(strlen(topl_resp));

    (*top_line)->viewRect.left += 10000;
    UndimMenuBar();
}

/* set selection by hand, not TESetSelect: the latter flushes pending keys */
static void
topl_set_select(short selStart, short selEnd)
{
    TEDeactivate(top_line);
    (*top_line)->selStart = selStart;
    (*top_line)->selEnd = selEnd;
    TEActivate(top_line);
}

/* Delete [from, to) without ever drawing a selection: classic TE extends
   a range hilite to the view's right edge when it ends at teLength (always,
   on a one-line TE), inverting the prompt. Deactivated, TEDelete paints no
   hilite. */
static void
topl_delete_silent(short from, short to)
{
    TEDeactivate(top_line);
    (*top_line)->selStart = from;
    (*top_line)->selEnd = to;
    TEDelete(top_line);
    TEActivate(top_line); /* selection is now an insertion point at 'from' */
}

static void
topl_replace(char *new_ans)
{
    topl_delete_silent(topl_query_len, (*top_line)->teLength);
    TEInsert(new_ans, strlen(new_ans), top_line);
}

static Boolean topl_key_body(unsigned char ch, Boolean ext);

/* TE draws into the current GrafPort; the events processed while waiting
   for this key (HandleUpdate etc.) can leave any window's port current,
   so anchor all of topl_key's TE calls to the message window */
Boolean
topl_key(unsigned char ch, Boolean ext)
{
    GrafPtr savePort;
    Boolean ret;
    WindowPtr msg_win;

    /* guard the theWindows[-1] read; same crash class as in_topl_mode's */
    if (WIN_MESSAGE == WIN_ERR
        || !(msg_win = theWindows[WIN_MESSAGE].its_window))
        return topl_key_body(ch, ext);

    GetPort(&savePort);
    SetPortWindowPort(msg_win);
    ret = topl_key_body(ch, ext);
    SetPort(savePort);
    return ret;
}

static Boolean
topl_key_body(unsigned char ch, Boolean ext)
{
    /* any key other than TAB restarts the extended-command completion
       cycle (see the '\t' case below) */
    if (ext && ch != '\t')
        topl_extcmd_tabi = 0;

    switch (ch) {
    case CHAR_ESC:
        topl_replace("\x1b"); /* leave ESC as the answer text */
        /* FALLTHROUGH -- like enter, ESC ends top-line input */
    case CHAR_ENTER:
    case CHAR_CR:
    case CHAR_LF:
        return false;

    case 0x1f & 'P':
        mac_doprev_message();
        return true;
    case '\x1e' /* up arrow */:
        topl_replace("");
        return true;

    case '\t':
        /* Extended-command completion (as on the Amiga): each TAB replaces
           the line with the next extcmds_match candidate for the prefix
           typed before the first TAB, autocomplete-flagged first. Typing or
           backspace resets the cycle. */
        if (!ext) {
            TEKey(ch, top_line); /* getlin: TAB is just another character */
            return true;
        } else {
            int typed, n_all, n_ac, k, j, i, eidx = -1;
            int *m;

            if (topl_extcmd_tabi == 0) {
                typed = (*top_line)->teLength - topl_query_len;
                if (typed < 0)
                    typed = 0;
                if (typed > (int) sizeof topl_extcmd_prefix - 1)
                    typed = (int) sizeof topl_extcmd_prefix - 1;
                memcpy(topl_extcmd_prefix,
                       *(*top_line)->hText + topl_query_len, typed);
                topl_extcmd_prefix[typed] = '\0';
            }
            n_all = extcmds_match(topl_extcmd_prefix, ECM_IGNOREAC, &m);
            if (n_all == 0) {
                nhbell(); /* no match for the typed prefix */
                return true;
            }
            /* extcmds_match hands back one shared static list, so re-fetch
               each variant immediately before reading it */
            n_ac = extcmds_match(topl_extcmd_prefix, ECM_NOFLAGS, &m);
            k = topl_extcmd_tabi++ % n_all;
            if (k < n_ac) {
                eidx = m[k]; /* m == the autocomplete-only matches */
            } else {
                n_all = extcmds_match(topl_extcmd_prefix, ECM_IGNOREAC, &m);
                j = k - n_ac;
                for (i = 0; i < n_all; i++) {
                    if (extcmds_getentry(m[i])->flags & AUTOCOMPLETE)
                        continue;
                    if (j-- == 0) {
                        eidx = m[i];
                        break;
                    }
                }
            }
            if (eidx >= 0) {
                topl_replace((char *) extcmds_getentry(eidx)->ef_txt);
                /* caret to end so the name can be edited further; an empty
                   selection at teLength is a caret, not a drawn hilite */
                topl_set_select((*top_line)->teLength,
                                (*top_line)->teLength);
            }
            return true;
        }

    case '?':
        /* '?' opens the extended-command menu (as on the Amiga).  The menu
           resolves to a command index; stuff its name into the line and end
           input so mac_get_ext_cmd's normal lookup returns the same index. */
        if (ext) {
            int sel = extcmd_via_menu();

            SetPortWindowPort(theWindows[WIN_MESSAGE].its_window);
            if (sel >= 0)
                topl_replace((char *) extcmds_getentry(sel)->ef_txt);
            else
                topl_replace("\x1b"); /* cancelled: answer ESC */
            return false;
        }
        TEKey(ch, top_line); /* getlin: '?' is an ordinary character */
        return true;

    case CHAR_BS:
    case '\x1c' /* left arrow */:
        if ((*top_line)->selEnd <= topl_query_len)
            return true;
        /* TEKey deletes one typed char (BS) or moves the caret left */
        TEKey(ch, top_line);
        return true;

    default:
        TEKey(ch, top_line);
        return true;
    }
}

static void
topl_flash_resp(int resp_idx)
{
    unsigned long dont_care;
    Rect frame;
    SetPortWindowPort(theWindows[WIN_MESSAGE].its_window);
    topl_resp_rect(resp_idx, &frame);
    InsetRect(&frame, 1, 1);
    InvertRect(&frame);
    Delay(GetDblTime() / 2, &dont_care);
    InvertRect(&frame);
}

static void
topl_set_def(int new_def_idx)
{
    Rect frame;
    SetPortWindowPort(theWindows[WIN_MESSAGE].its_window);
    topl_resp_rect(topl_def_idx, &frame);
    InvalWindowRect(theWindows[WIN_MESSAGE].its_window, &frame);
    topl_def_idx = new_def_idx;
    topl_resp_rect(new_def_idx, &frame);
    InvalWindowRect(theWindows[WIN_MESSAGE].its_window, &frame);
}

void
topl_set_resp(char *resp, char def)
{
    char *loc;
    int r_len, r_len1;

    if (!resp) {
        const char any_str[2] = { CHAR_ANY, '\0' };
        resp = (char *) any_str;
        def = CHAR_ANY;
    }

    SetPortWindowPort(theWindows[WIN_MESSAGE].its_window);
    r_len1 = strlen(resp);
    r_len = strlen(topl_resp);
    if (r_len < r_len1)
        r_len = r_len1; /* cover both the old and the new strip */
    invalidate_topl_buttons(r_len);
    strncpy(topl_resp, resp, sizeof topl_resp - 1);
    loc = strchr(topl_resp, def);
    topl_def_idx = loc ? loc - topl_resp : -1;

    /* strip appears with the buttons; re-pin so the last line clears it */
    if (small_screen && topl_resp[0] && in_topl_mode()) {
        NhWindow *aWin = &theWindows[WIN_MESSAGE];
        Rect r;
        short min;

        GetWindowPortBounds(aWin->its_window, &r);
        OffsetRect(&r, -r.left, -r.top);
        min = aWin->y_size
              - (r.bottom - r.top - SBARHEIGHT) / aWin->row_height;
        if (aWin->scrollPos < min) {
            aWin->scrollPos = min;
            if (aWin->scrollBar)
                SetControlValue(aWin->scrollBar, min);
            InvalWindowRect(aWin->its_window, &r);
        }
    }
}

static char topl_resp_key_body(char);

static char
topl_resp_key(char ch)
{
    if (strlen(topl_resp) > 0) {
        /* same port hazard as topl_key: this runs from GeneralKey during
           HandleEvent, where update handling can leave any port current,
           and the TEKey calls below draw into the prompt TE */
        GrafPtr savePort;
        WindowPtr msg_win = (WIN_MESSAGE != WIN_ERR)
                                ? theWindows[WIN_MESSAGE].its_window
                                : (WindowPtr) 0;

        GetPort(&savePort);
        if (msg_win)
            SetPortWindowPort(msg_win);
        ch = topl_resp_key_body(ch);
        SetPort(savePort);
    }
    return ch;
}

static char
topl_resp_key_body(char ch)
{
    {
        char *loc = strchr(topl_resp, ch);

        if (!loc) {
            if (ch == '\x9' /* tab */) {
                topl_set_def(topl_def_idx <= 0 ? strlen(topl_resp) - 1
                                               : topl_def_idx - 1);
                ch = '\0';
            } else if (ch == CHAR_ESC) {
                loc = strchr(topl_resp, 'q');
                if (!loc) {
                    loc = strchr(topl_resp, 'n');
                    if (!loc && topl_def_idx >= 0)
                        loc = topl_resp + topl_def_idx;
                }
            } else if (ch == (0x1f & 'P')) {
                mac_doprev_message();
                ch = '\0';
            } else if (topl_def_idx >= 0) {
                if (ch == CHAR_ENTER || ch == CHAR_CR || ch == CHAR_LF
                    || ch == CHAR_BLANK
                    || topl_resp[topl_def_idx] == CHAR_ANY)
                    loc = topl_resp + topl_def_idx;

                else if (strchr(topl_resp, '#')) {
                    if (digit(ch)) {
                        topl_set_def(strchr(topl_resp, '#') - topl_resp);
                        TEKey(ch, top_line);
                        ch = '\0';

                    } else if (topl_resp[topl_def_idx] == '#') {
                        if (ch == '\x1e' /* up arrow */) {
                            topl_set_select(topl_query_len, topl_query_len);
                            ch = '\0';
                        } else if (ch == '\x1d'    /* right arrow */
                                   || ch == '\x1f' /* down arrow */
                                   || ch == CHAR_BS
                                   || (ch == '\x1c' /* left arrow, but not
                                                       into the prompt */
                                       && (*top_line)->selEnd
                                              > topl_query_len)) {
                            TEKey(ch, top_line);
                            ch = '\0';
                        }
                    }
                }
            }
        }

        if (loc) {
            topl_flash_resp(loc - topl_resp);
            if (*loc != CHAR_ANY)
                ch = *loc;
            TEKey(ch, top_line);
        }
    }

    return ch;
}

static void
adjust_window_pos(NhWindow *aWin, short width, short height)
{
    WindowRef theWindow = aWin->its_window;
    Rect scr_r = (*GetGrayRgn())->rgnBBox;
    const Rect win_ind = { 2, 2, 3, 3 };
    const short min_w = theWindow->portRect.right - theWindow->portRect.left,
                max_w =
                    scr_r.right - scr_r.left - win_ind.left - win_ind.right;
    Point pos;
    short max_h;

    SetPortWindowPort(theWindow);
    if (!RetrieveWinPos(theWindow, &pos.v, &pos.h)) {
        pos.v = 0; /* take window's existing position */
        pos.h = 0;
        LocalToGlobal(&pos);
    }

    max_h = scr_r.bottom - win_ind.bottom - pos.v;
    if (height > max_h)
        height = max_h;
    if (height < MIN_HEIGHT)
        height = MIN_HEIGHT;
    if (width < min_w)
        width = min_w;
    if (width > max_w)
        width = max_w;
    SizeWindow(theWindow, width, height, true);

    if (pos.v + height + win_ind.bottom > scr_r.bottom)
        pos.v = scr_r.bottom - height - win_ind.bottom;
    if (pos.h + width + win_ind.right > scr_r.right)
        pos.h = scr_r.right - width - win_ind.right;
    MoveWindow(theWindow, pos.h, pos.v, false);
    if (aWin->scrollBar)
        DrawScrollbar(aWin);
    return;
}

/*
 * display/select/update the window.
 * If f is true, this window should be "modal" - don't return
 * until presumed seen.
 */
void
mac_display_nhwindow(winid win, boolean f)
{
    NhWindow *aWin = &theWindows[win];
    WindowPtr theWindow = aWin->its_window;

    if (win < 0 || win >= NUM_MACWINDOWS || !theWindow) {
        error("disp_win: Invalid window %d.", win);
        return;
    }

    if (theWindow == _mt_window) {
        tty_display_nhwindow(win, f);
        return;
    }

    /* the map window owns its own size/position; skip adjust_window_pos */
    if (win == WIN_MAP) {
        if (!IsWindowVisible(theWindow)) {
            SelectWindow(theWindow);
            ShowWindow(theWindow);
        }
        macmap_flush();   /* per-frame flush: blit the batched cell draws once */
        return;
    }

    if (f && inSelect == WIN_ERR && win == WIN_MESSAGE) {
        topl_set_resp((char *) 0, 0);
        if (aWin->windowTextLen > 0
            && (*aWin->windowText)[aWin->windowTextLen - 1] == CHAR_CR) {
            --aWin->windowTextLen;
            --aWin->y_size;
        }
        putstr(win, flags.standout ? ATR_INVERSE : ATR_NONE, " --More--");
    }

    if (!IsWindowVisible(theWindow)) {
        if (win != WIN_MESSAGE)
            adjust_window_pos(aWin, aWin->x_size + SBARWIDTH + 1,
                              aWin->y_size * aWin->row_height);

        SelectWindow(theWindow);
        ShowWindow(theWindow);
    }

    if (f && inSelect == WIN_ERR) {
        int ch;

        DimMenuBar();
        inSelect = win;
        do {
            ch = mac_nhgetch();
        } while (!ClosingWindowChar(ch));
        inSelect = WIN_ERR;
        UndimMenuBar();

        if (win == WIN_MESSAGE)
            topl_set_resp("", '\0');
        else
            HideWindow(theWindow);
    }
}

void
mac_destroy_nhwindow(winid win)
{
    WindowPtr theWindow;
    NhWindow *aWin = &theWindows[win];
    int kind;

    if (win < 0 || win >= NUM_MACWINDOWS) {
        if (iflags.window_inited)
            error("dest_win: Invalid win %d.", win);
        return;
    }
    theWindow = aWin->its_window;
    if (!theWindow) {
        error("dest_win: Not allocated win %d.", win);
        return;
    }

    /* The base window never goes away; standard windows stay until exit. */
    if (theWindow == _mt_window) {
        return;
    }
    /* map window has its own GWorld/palette/owner state; macmap_destroy frees it */
    if (win == WIN_MAP) {
        macmap_destroy(aWin);
        return;
    }
    if (win == WIN_INVEN || win == WIN_MESSAGE) {
        if (iflags.window_inited) {
            if (flags.tombstone && svk.killer.name[0]) {
                /* tombstone window wants a monospaced font */
                win_fonts[NHW_TEXT] = kFontIDMonaco;
            }
            return;
        }
        if (win == WIN_MESSAGE)
            WIN_MESSAGE = WIN_ERR;
    }

    kind = GetWindowKind(theWindow) - WIN_BASE_KIND;

    if ((!IsWindowVisible(theWindow)
         || (kind != NHW_MENU && kind != NHW_TEXT))) {
        DisposeWindow(theWindow);
        if (aWin->windowText) {
            DisposeHandle(aWin->windowText);
        }
        if (aWin->menuStyle) {
            DisposeHandle(aWin->menuStyle);
            aWin->menuStyle = (Handle) 0;
        }
        aWin->its_window = (WindowPtr) 0;
        aWin->windowText = (Handle) 0;
    }
}

void
mac_number_pad(int pad)
{ /* no effect */
    return;
}

/* Toggle hilite of a menu line (line is relative to the scrollbar). */
static void
ToggleMenuSelect(NhWindow *aWin, int line)
{
    Rect r;

    GetWindowPortBounds(aWin->its_window, &r);
    OffsetRect(&r, -r.left, -r.top);
    if (aWin->scrollBar)
        r.right -= SBARWIDTH;
    r.top = line * aWin->row_height;
    r.bottom = r.top + aWin->row_height;

    LMSetHiliteMode((UInt8)(LMGetHiliteMode() & 0x7F));
    InvertRect(&r);
}

/*
 * Check to see if given item is selected, return index if it is
 */
static int
ListItemSelected(NhWindow *aWin, int item)
{
    int i;

    HLock((char **) aWin->menuSelected);
    for (i = aWin->miSelLen - 1; i >= 0; i--) {
        if ((*aWin->menuSelected)[i] == item)
            break;
    }
    HUnlock((char **) aWin->menuSelected);
    return i;
}

/*
 * Add item to selection list if it's not selected already
 * If it is selected already, remove it from the list.
 */
static void
ToggleMenuListItemSelected(NhWindow *aWin, short item)
{
    int i = ListItemSelected(aWin, item);

    HLock((char **) aWin->menuSelected);
    if (i < 0) { /* add */
        (*aWin->menuSelected)[aWin->miSelLen] = item;
        aWin->miSelLen++;
    } else { /* remove */
        short *mi = &(*aWin->menuSelected)[i];
        aWin->miSelLen--;
        memcpy(mi, mi + 1, (aWin->miSelLen - i) * sizeof(short));
    }
    HUnlock((char **) aWin->menuSelected);
}

/*
 * Find menu item in list given a line number on the window
 */
static short
ListCoordinateToItem(NhWindow *aWin, short Row)
{
    int i, item = -1;
    MacMHMenuItem *mi;

    HLock((char **) aWin->menuInfo);
    for (i = 0, mi = *aWin->menuInfo; i < aWin->miLen; i++, mi++) {
        if (mi->line == Row + aWin->scrollPos) {
            item = i;
            break;
        }
    }
    HUnlock((char **) aWin->menuInfo);
    return item;
}

static pascal void
MoveScrollBar(ControlHandle theBar, short part)
{
    EventRecord fake = {0};
    Rect r;
    RgnHandle rgn;
    int now, amtToScroll;
    WindowPtr theWin;
    NhWindow *winToScroll;

    if (!part)
        return;

    theWin = (**theBar).contrlOwner;
    GetWindowPortBounds(theWin, &r);
    OffsetRect(&r, -r.left, -r.top);
    winToScroll = (NhWindow *) (GetWRefCon(theWin));
    now = GetControlValue(theBar);

    if (part == kControlPageUpPart || part == kControlPageDownPart)
        amtToScroll = (r.bottom - r.top) / winToScroll->row_height;
    else
        amtToScroll = 1;

    if (part == kControlPageUpPart || part == kControlUpButtonPart) {
        int bound = GetControlMinimum(theBar);
        if (now - bound < amtToScroll)
            amtToScroll = now - bound;
        amtToScroll = -amtToScroll;
    } else {
        int bound = GetControlMaximum(theBar);
        if (bound - now < amtToScroll)
            amtToScroll = bound - now;
    }

    if (!amtToScroll)
        return;

    SetControlValue(theBar, now + amtToScroll);
    winToScroll->scrollPos = now + amtToScroll;
    r.right -= SBARWIDTH;
    if (winToScroll == theWindows + WIN_MESSAGE)
        r.bottom -= msg_bottom_inset();
    rgn = NewRgn();
    if (!rgn)
        return;
    ScrollRect(&r, 0, -amtToScroll * winToScroll->row_height, rgn);
    {
        InvalWindowRgn(theWin, rgn);
        BeginUpdate(theWin);
    }
    {
        int kind = GetWindowKind(theWin) - WIN_BASE_KIND;
        if (kind >= 0 && kind < NUM_FUNCS)
            winUpdateFuncs[kind](&fake, theWin);
    }
    EndUpdate(theWin);
    DisposeRgn(rgn);
}

static void
DoScrollBar(Point p, short code, ControlHandle theBar, NhWindow *aWin)
{
    ControlActionUPP func = NULL;
    Rect rect;

    if (code == kControlUpButtonPart || code == kControlPageUpPart
        || code == kControlDownButtonPart || code == kControlPageDownPart)
        func = MoveScrollUPP;
    (void) TrackControl(theBar, p, func);
    if (!func) {
        if (aWin->scrollPos != GetControlValue(theBar)) {
            aWin->scrollPos = GetControlValue(theBar);
            GetWindowPortBounds(aWin->its_window, &rect);
            OffsetRect(&rect, -rect.left, -rect.top);
            InvalWindowRect(aWin->its_window, &rect);
        }
    }
}

static int
filter_scroll_key(const int ch, NhWindow *aWin)
{
    if (aWin->scrollBar
        && GetControlValue(aWin->scrollBar)
               < GetControlMaximum(aWin->scrollBar)) {
        short part = 0;
        if (ch == CHAR_BLANK) {
            part = kControlPageDownPart;
        } else if (ch == CHAR_CR || ch == CHAR_LF) {
            part = kControlDownButtonPart;
        }
        if (part) {
            SetPortWindowPort(aWin->its_window);
            MoveScrollBar(aWin->scrollBar, part);
            return 0;
        }
    }
    return ch;
}

int
mac_doprev_message(void)
{
    if (WIN_MESSAGE != WIN_ERR) {
        NhWindow *winToScroll = &theWindows[WIN_MESSAGE];
        mac_display_nhwindow(WIN_MESSAGE, FALSE);
        if (winToScroll->scrollBar) {
            SetPortWindowPort(winToScroll->its_window);
            MoveScrollBar(winToScroll->scrollBar, kControlUpButtonPart);
        }
    }
    return 0;
}

static void
draw_growicon_vert_only(WindowPtr wind)
{
    GrafPtr org_port;
    RgnHandle org_clip = NewRgn();
    Rect r;

    GetPort(&org_port);
    SetPortWindowPort(wind);
    GetClip(org_clip);
    GetWindowPortBounds(wind, &r);
    OffsetRect(&r, -r.left, -r.top);
    r.left = r.right - SBARWIDTH;
    ClipRect(&r);
    DrawGrowIcon(wind);
    SetClip(org_clip);
    DisposeRgn(org_clip);
    SetPort(org_port);
}

static void
WindowGoAway(EventRecord *theEvent, WindowPtr theWindow)
{
    NhWindow *aWin = GetNhWin(theWindow);

    if (!theEvent || TrackGoAway(theWindow, theEvent->where)) {
        if (aWin - theWindows == BASE_WINDOW && !iflags.window_inited) {
            AddToKeyQueue('\033', 1);
        } else {
            HideWindow(theWindow);
            if (aWin - theWindows != inSelect)
                mac_destroy_nhwindow(aWin - theWindows);
            else /* if this IS the inSelect window put a close char */
                AddToKeyQueue(CHAR_CR,
                              1); /* in queue to exit and maintain inSelect */
        }
    }
}

void
mac_get_nh_event(void)
{
    EventRecord anEvent;

    if (!iflags.window_inited)
        return;

    /* Also wired to win_wait_synch: setftty() clears TA_ALWAYS_REFRESH during
       play, so flush the offscreen here or buffered status never reaches screen */
    if (_mt_window) update_tty(_mt_window);

    (void) WaitNextEvent(everyEvent, &anEvent, 1, gMouseRgn);
    HandleEvent(&anEvent);
    return;
}

int
mac_nhgetch(void)
{
    int ch;
    long doDawdle = 1L;
    EventRecord anEvent;

      /* don't dawdle while keys are buffered */
    if (keyQueueCount)
        doDawdle = 0L;
    else {
        long total, contig;
        static char warn = 0;

        doDawdle = (in_topl_mode() ? GetCaretTime() : 120L);
        /* Since we have time, check memory */
        PurgeSpace(&total, &contig);
        if (contig < 25000L || total < 50000L) {
            if (!warn) {
                pline("Low Memory!");
                warn = 1;
            }
        } else {
            warn = 0;
        }
    }

    /* flush buffered status output before blocking, else it lags a turn */
    if (_mt_window)
        update_tty(_mt_window);

    do {
        (void) WaitNextEvent(everyEvent, &anEvent, doDawdle, gMouseRgn);
        HandleEvent(&anEvent);
        if (in_topl_mode()) {
            /* blink the TE caret in the prompt; doDawdle is GetCaretTime
               in topl mode so we wake often enough */
            GrafPtr savePort;

            GetPort(&savePort);
            SetPortWindowPort(theWindows[WIN_MESSAGE].its_window);
            TEIdle(top_line);
            SetPort(savePort);
        }
        ch = GetFromKeyQueue();
    } while (!ch && !gClickedToMove);

    if (!gClickedToMove)
        ObscureCursor();
    else
        gClickedToMove = 0;

    return (ch);
}

void
mac_delay_output(void)
{
    long destTicks = TickCount() + 1;

    while (TickCount() < destTicks) {
        mac_get_nh_event();
    }
}

#ifdef CLIPPING
void
mac_cliparound(int x, int y)
{
    if (WIN_MAP != WIN_ERR) {
        macmap_cliparound(&theWindows[WIN_MAP], x, y);
    }
}
#endif

void
mac_exit_nhwindows(const char *s)
{
    clear_screen();
    tty_exit_nhwindows(s);
    mac_destroy_nhwindow(WIN_MESSAGE);
    mac_destroy_nhwindow(WIN_INVEN);
}

/* Word-wrap a message into CHAR_CR rows that each fit availw pixels (port
   font must be the message window's). startw is the width already on the
   current row (a topl answer continues the query line). Returns the
   wrapped length; dst is NUL-terminated, truncating overlong input. */
static long
wrap_message(const char *src, char *dst, long dstsz, short availw,
             short startw)
{
    long di = 0, lineStart = 0, lastSpace = -1;
    long linew = startw;

    while (*src && di < dstsz - 2) {
        unsigned char uch = (unsigned char) *src++;
        short cw;

        if (uch == CHAR_LF || uch == CHAR_CR) {
            dst[di++] = CHAR_CR;
            lineStart = di;
            lastSpace = -1;
            linew = 0;
            continue;
        }
        cw = CharWidth((short) uch);
        if (linew + cw > availw && (di > lineStart || linew > 0)) {
            if (lastSpace >= lineStart) {
                /* break at the last space: it becomes the CR */
                dst[lastSpace] = CHAR_CR;
                lineStart = lastSpace + 1;
                lastSpace = -1;
                linew = (di > lineStart)
                            ? TextWidth(dst + lineStart, 0,
                                        (short) (di - lineStart))
                            : 0;
                if (linew + cw > availw && di > lineStart) {
                    dst[di++] = CHAR_CR; /* still too wide: hard break */
                    lineStart = di;
                    linew = 0;
                }
            } else {
                dst[di++] = CHAR_CR;     /* no space on row: hard break */
                lineStart = di;
                linew = 0;
            }
        }
        if (uch == ' ')
            lastSpace = di;
        dst[di++] = (char) uch;
        linew += cw;
    }
    dst[di] = 0;
    return di;
}

/*
 * Don't forget to decrease in_putstr before returning...
 */
void
mac_putstr(winid win, int attr, const char *str)
{
    long len, slen;
    NhWindow *aWin = &theWindows[win];
    static char in_putstr = 0;
    short newWidth, maxWidth, y_before;
    Rect r;
    char *src, *sline, *dst, ch;
    char wrapped[BUFSZ * 4];

    if (win < 0 || win >= NUM_MACWINDOWS || !aWin->its_window) {
        /* During early init, WIN_MESSAGE is -1; use raw_print instead */
        if (win < 0 && str) {
            raw_print(str);
            return;
        }
        error("putstr: Invalid win %d (Max %d).", win, NUM_MACWINDOWS);
        return;
    }

    if (aWin->its_window == _mt_window) {
        tty_putstr(win, attr, str);
        return;
    }

    if (in_putstr > 3)
        return;

    in_putstr++;

    SetPortWindowPort(aWin->its_window);
    GetWindowPortBounds(aWin->its_window, &r);
    OffsetRect(&r, -r.left, -r.top);

    /* Wrap to the window width before storing: the scroll math counts
       CHAR_CR lines, so a row TETextBox wraps on its own adds a display
       row it never sees, pushing everything below out of view. A topl
       answer continues the query line, hence the startw measurement. */
    if (win == WIN_MESSAGE) {
        short availw = r.right - r.left
                       - (aWin->scrollBar ? SBARWIDTH : 0) - 3;
        if (availw > 50) {
            short startw = 0;
            TextFont(aWin->font_number);
            TextSize(aWin->font_size);
            TextFace(normal);
            if (aWin->windowTextLen > 0) {
                long n = aWin->windowTextLen, ls;
                HLock(aWin->windowText);
                {
                    char *p = *aWin->windowText;
                    ls = n;
                    while (ls > 0 && p[ls - 1] != CHAR_CR)
                        ls--;
                    if (n > ls && n - ls < 0x7FFF)
                        startw = TextWidth(p + ls, 0, (short) (n - ls));
                }
                HUnlock(aWin->windowText);
            }
            (void) wrap_message(str, wrapped, (long) sizeof wrapped,
                                availw, startw);
            str = wrapped;
        }
    }
    slen = strlen(str);

    if (win == WIN_MESSAGE) {
        if (aWin->scrollBar)
            r.right -= SBARWIDTH;
        r.bottom -= msg_bottom_inset();
        if (flags.safe_wait
            && aWin->last_more_lin
                   <= aWin->y_size - (r.bottom - r.top) / aWin->row_height) {
            aWin->last_more_lin = aWin->y_size;
            mac_display_nhwindow(win, TRUE);
        }
    }

    /* append the text to windowText; attributes are not retained */
    len = GetHandleSize(aWin->windowText);
    while (aWin->windowTextLen + slen + 1 > len) {
        len = (len > 2048) ? (len + 2048) : (len * 2);
        SetHandleSize(aWin->windowText, len);
        if (MemError()) {
            error("putstr: SetHandleSize");
            aWin->windowTextLen = 0L;
            aWin->save_lin = 0;
            aWin->y_curs = 0;
            aWin->y_size = 0;
            in_putstr--;
            return;
        }
    }

    /* Transient (ATR_NOHISTORY) message after another transient one: drop the
       prior one so this replaces it in place rather than piling up.  A
       wrapped transient stored several rows, so back out as many rows
       as it added. */
    if (win == WIN_MESSAGE && (attr & ATR_NOHISTORY) && gLastMsgTransient
        && aWin->windowTextLen > 0) {
        long n = aWin->windowTextLen;
        short k = (gLastTransientLines > 0) ? gLastTransientLines : 1;
        HLock(aWin->windowText);
        {
            char *p = *aWin->windowText;
            while (k-- > 0 && n > 0) {
                if (p[n - 1] == CHAR_CR) n--;          /* trailing CR */
                while (n > 0 && p[n - 1] != CHAR_CR)
                    n--;                               /* back to line start */
                if (aWin->y_size > 0) aWin->y_size--;
                if (aWin->y_curs > 0) aWin->y_curs--;
            }
        }
        HUnlock(aWin->windowText);
        aWin->windowTextLen = n;
    }

    y_before = aWin->y_size;
    len = aWin->windowTextLen;
    HLock(aWin->windowText);
    dst = *(aWin->windowText) + len;
    sline = src = (char *) str;
    maxWidth = newWidth = 0;
    for (ch = *src; ch; ch = *src) {
        if (ch == CHAR_LF)
            ch = CHAR_CR;
        *dst++ = ch;
        if (ch == CHAR_CR) {
            aWin->y_curs++;
            aWin->y_size++;
            aWin->x_curs = 0;
            /* the message window has a fixed width; only the other
               window kinds use maxWidth (x_size below), so skip the
               per-line TextWidth traps on the hottest putstr path */
            if (win != WIN_MESSAGE) {
                newWidth = TextWidth(sline, 0, src - sline);
                if (newWidth > maxWidth) {
                    maxWidth = newWidth;
                }
            }
            sline = src + 1;
        } else
            aWin->x_curs++;
        src++;
    }

    if (win != WIN_MESSAGE) {
        newWidth = TextWidth(sline, 0, src - sline);
        if (newWidth > maxWidth) {
            maxWidth = newWidth;
        }
    }

    aWin->windowTextLen += slen;

    /* terminate the line unless the text already ended with CR/LF (the
       loop above exits with ch == 0, so test the last STORED byte; an
       empty string still gets a CR -- a deliberate blank line) */
    if (slen == 0 || (*(aWin->windowText))[len + slen - 1] != CHAR_CR) {
        (*(aWin->windowText))[len + slen] = CHAR_CR;
        aWin->windowTextLen++;
        aWin->y_curs++;
        aWin->y_size++;
        aWin->x_curs = 0;
    }
    HUnlock(aWin->windowText);

    if (win == WIN_MESSAGE) {
        gLastMsgTransient = (attr & ATR_NOHISTORY) != 0;
        if (gLastMsgTransient)
            gLastTransientLines = aWin->y_size - y_before;
        short min = aWin->y_size - (r.bottom - r.top) / aWin->row_height;
        if (aWin->scrollPos < min) {
            aWin->scrollPos = min;
            if (aWin->scrollBar) {
                SetControlMaximum(aWin->scrollBar, aWin->y_size);
                SetControlValue(aWin->scrollBar, min);
            }
        }
        InvalWindowRect(aWin->its_window, &r);
    } else /* Message has a fixed width, other windows base on content */
        if (maxWidth > aWin->x_size)
        aWin->x_size = maxWidth;
    in_putstr--;
}

void
mac_curs(winid win, int x, int y)
{
    NhWindow *aWin = &theWindows[win];

    if (aWin->its_window == _mt_window) {
        tty_curs(win, x, y);
        return;
    }

    /* Macmap window: software cursor for getpos/farlook. */
    if (WIN_MAP != WIN_ERR && win == WIN_MAP
        && GetWRefCon(aWin->its_window) == MACMAP_REFCON) {
        macmap_curs(aWin, x, y);
        return;
    }

    SetPortWindowPort(aWin->its_window);
    MoveTo(x * aWin->char_width,
           (y * aWin->row_height) + aWin->ascent_height);
    aWin->x_curs = x;
    aWin->y_curs = y;
}

int
mac_nh_poskey(coordxy *a, coordxy *b, int *c)
{
    int ch = mac_nhgetch();
    *a = clicked_pos.h;
    *b = clicked_pos.v;
    *c = clicked_mod;
    return ch;
}

void
mac_start_menu(winid win, unsigned long mbehavior)
{
    HideWindow(theWindows[win].its_window);
    mac_clear_nhwindow(win);
}

void
mac_add_menu(winid win, const glyph_info *glyphinfo UNUSED,
             const anything *any, char menuChar,
             char groupAcc, int attr, int clr,
             const char *inStr, unsigned int itemflags)
{
    NhWindow *aWin = &theWindows[win];
    short line0;
    const char *str;
    char locStr[4 + BUFSZ];
    MacMHMenuItem *item;
    int preselected = ((itemflags & MENU_ITEMFLAGS_SELECTED) != 0);

    if (!inStr)
        return;

    if (any->a_void != 0) {
#define kMenuSizeBump 26
        if (!aWin->miSize) {
            aWin->menuInfo = (MacMHMenuItem **) NewHandle(
                sizeof(MacMHMenuItem) * kMenuSizeBump);
            if (!aWin->menuInfo) {
                error("Can't alloc menu handle");
                return;
            }
            aWin->menuSelected =
                (short **) NewHandle(sizeof(short) * kMenuSizeBump);
            if (!aWin->menuSelected) {
                DisposeHandle((Handle) aWin->menuInfo);
                aWin->menuInfo = NULL;
                error("Can't alloc menu select handle");
                return;
            }
            aWin->miSize = kMenuSizeBump;
        }

        if (aWin->miLen >= aWin->miSize) {
            SetHandleSize((Handle) aWin->menuInfo,
                          sizeof(MacMHMenuItem)
                              * (aWin->miLen + kMenuSizeBump));
            if (MemError()) {
                error("Can't resize menu handle");
                return;
            }
            SetHandleSize((Handle) aWin->menuSelected,
                          sizeof(short) * (aWin->miLen + kMenuSizeBump));
            if (MemError()) {
                error("Can't resize menu select handle");
                return;
            }
            aWin->miSize += kMenuSizeBump;
        }

        if (menuChar == 0) {
            if (('a' <= aWin->menuChar && aWin->menuChar <= 'z')
                || ('A' <= aWin->menuChar && aWin->menuChar <= 'Z')) {
                menuChar = aWin->menuChar++;
                if (menuChar == 'z')
                    aWin->menuChar = 'A';
            }
        }

        Sprintf(locStr, "%c - %s", (menuChar ? menuChar : ' '), inStr);
        str = locStr;
        HLock((char **) aWin->menuInfo);
        HLock((char **) aWin->menuSelected);
        (*aWin->menuSelected)[aWin->miLen] = preselected;
        item = &(*aWin->menuInfo)[aWin->miLen];
        aWin->miLen++;
        item->id = *any;
        item->accelerator = menuChar;
        item->groupAcc = groupAcc;
        item->line = aWin->y_size;
        HUnlock((char **) aWin->menuInfo);
        HUnlock((char **) aWin->menuSelected);
    } else
        str = inStr;

    line0 = aWin->y_size;
    putstr(win, attr, str);
    /* record the style for the line(s) putstr just appended so the menu
       renderer can draw headings bold and apply menucolors */
    record_menu_line_style(aWin, line0, aWin->y_size, attr, clr);
}

/* End an NHW_MENU window; morestr is an optional prompt (window title). */
void
mac_end_menu(winid win, const char *morestr)
{
    Str255 buf;
    NhWindow *aWin = &theWindows[win];

    buf[0] = 0;
    if (morestr)
        C2P(morestr, buf);
    SetWTitle(aWin->its_window, buf);
}

int
mac_select_menu(winid win, int how, menu_item **selected_list)
{
    int c;
    NhWindow *aWin = &theWindows[win];
    WindowPtr theWin = aWin->its_window;

    inSelect = win;

    mac_display_nhwindow(win, FALSE);

    aWin->how = (short) how;
    for (;;) {
        c = map_menu_cmd(mac_nhgetch());
        if (c == CHAR_ESC) {
            /* deselect everything */
            aWin->miSelLen = 0;
            HideWindow(theWin);
            *selected_list = 0;
            inSelect = WIN_ERR;
            return -1; /* cancelled */
        } else if (ClosingWindowChar(c)) {
            break;
        } else {
            nhbell();
        }
    }

    HideWindow(theWin);

    if (aWin->miSelLen) {
        menu_item *mp;
        MacMHMenuItem *mi;
        *selected_list = mp =
            (menu_item *) alloc(aWin->miSelLen * sizeof(menu_item));
        HLock((char **) aWin->menuInfo);
        HLock((char **) aWin->menuSelected);
        for (c = 0; c < aWin->miSelLen; c++) {
            mi = &(*aWin->menuInfo)[(*aWin->menuSelected)[c]];
            mp->item = mi->id;
            mp->count = -1L;
            mp++;
        }
        HUnlock((char **) aWin->menuInfo);
        HUnlock((char **) aWin->menuSelected);
    } else
        *selected_list = 0;

    inSelect = WIN_ERR;

    return aWin->miSelLen;
}

#include "dlb.h"

static void
mac_display_file(const char *name, boolean complain)
{
    Ptr buf;
    int win;
    dlb *fp = dlb_fopen(name, "r");

    if (fp) {
        long l;
        (void) dlb_fseek(fp, 0, SEEK_END);
        l = dlb_ftell(fp);
        (void) dlb_fseek(fp, 0, SEEK_SET);
        buf = NewPtr(l + 1);
        if (buf) {
            l = dlb_fread(buf, 1, l, fp);
            if (l > 0) {
                buf[l] = '\0';
                win = create_nhwindow(NHW_TEXT);
                if (WIN_ERR == win) {
                    if (complain)
                        error("Cannot make window.");
                } else {
                    putstr(win, 0, buf);
                    display_nhwindow(win, TRUE);
                    destroy_nhwindow(win);
                }
            }
            DisposePtr(buf);
        }
        dlb_fclose(fp);
    } else if (complain)
        error("Cannot open %s.", name);
}

void
port_help()
{
    display_file(PORT_HELP, TRUE);
}

/* optfn_hicolor: Mac-specific option handler for "hicolor"
 * (same as palette but reversed). Referenced from optlist.h NHOPTC. */
int
optfn_hicolor(int optidx UNUSED, int req UNUSED, boolean negated UNUSED,
              char *opts UNUSED, char *op UNUSED)
{
    return 1; /* optn_ok; stub */
}

static void
mac_player_selection(void)
{
    /* Player selection handled via mac_askname / macmenu.c */
}

static void
mac_resume_nhwindows(void)
{
    /* noop on classic Mac OS */
}

static void
mac_mark_synch(void)
{
    /* Flush buffered tty writes to screen: setftty() clears TA_ALWAYS_REFRESH
       during play, so the offscreen is correct but the window shows stale text
       until this runs (core calls mark_synch after each status_update). */
    if (_mt_window && iflags.window_inited)
        update_tty(_mt_window);
}

static void
mac_raw_print(const char *str)
{
    if (str && *str && _mt_window && iflags.window_inited) {
        add_tty_string(_mt_window, str);
        add_tty_char(_mt_window, CHAR_CR);
        update_tty(_mt_window);
    }
}

static void
mac_raw_print_bold(const char *str)
{
    if (str && *str && _mt_window && iflags.window_inited) {
        term_start_raw_bold();
        add_tty_string(_mt_window, str);
        add_tty_char(_mt_window, CHAR_CR);
        term_end_raw_bold();
        update_tty(_mt_window);
    }
}

static void
mac_print_glyph(winid win, coordxy x, coordxy y,
                const glyph_info *glyphinfo,
                const glyph_info *bkglyphinfo UNUSED)
{
    if (win == WIN_MAP && win >= 0 && win < NUM_MACWINDOWS) {
        macmap_print_glyph(&theWindows[win], (int) x, (int) y, glyphinfo);
        return;
    }

    /* tty path for non-map windows */
    int ch;
    tty_curs(win, x, y);
    ch = (glyphinfo && glyphinfo->ttychar) ? glyphinfo->ttychar : ' ';
    term_start_color(glyphinfo ? glyphinfo->gm.sym.color : NO_COLOR);
    add_tty_char(_mt_window, (short) ch);
    term_end_color();
    wins[win]->curx++;
    ttyDisplay->curx++;
    update_tty(_mt_window);
}

static void
mac_update_inventory(int arg UNUSED)
{
    /* stub - could trigger inventory window redraw */
}

static win_request_info *
mac_ctrl_nhwindow(winid win UNUSED, int request UNUSED,
                  win_request_info *wri UNUSED)
{
    return (win_request_info *) 0;
}

static void
mac_suspend_nhwindows(const char *foo)
{
    /*	Can't really do that :-)		*/
}

/* Drain the queued keys (through the first CR/LF) into bufp as a C string.
   bufp must have room for at least QUEUE_LEN + 1 bytes.  Returns 1 if any
   keys were queued, 0 otherwise. */
int
try_key_queue(char *bufp)
{
    if (keyQueueCount) {
        char ch;
        int i = 0;
        for (ch = GetFromKeyQueue();; ch = GetFromKeyQueue()) {
            if (ch == CHAR_LF || ch == CHAR_CR)
                ch = 0;
            if (i < QUEUE_LEN)
                bufp[i++] = ch;
            if (ch == 0)
                break;
        }
        bufp[QUEUE_LEN] = 0;
        return 1;
    }
    return 0;
}

/**********************************************************************
 *	Base window
 */

static void
BaseClick(NhWindow *wind, Point pt, UInt32 modifiers)
{
    int col, row;
    if (wind == &theWindows[WIN_MAP]) {
        if (macmap_click(wind, pt, modifiers))
            return;   /* click handled by the scrollbars/grow box */
        macmap_pixel_to_cell(wind, pt, &col, &row);
    } else {
        col = pt.h / wind->char_width + 1;
        row = pt.v / wind->row_height;
    }
    clicked_mod = (modifiers & shiftKey) ? CLICK_2 : CLICK_1;
    clicked_pos.h = (short) col;
    clicked_pos.v = (short) row;
    /* mac_nhgetch checks gClickedToMove to exit its loop and return 0;
       the core then reads the position via mac_nh_poskey() */
    gClickedToMove = 1;
}

static void
BaseCursor(NhWindow *wind, Point pt)
{
    CursHandle ch;

    /* direction-based cursors (CURS 513-520) exist in the rsrc fork but are
       not currently used */
    ch = GetCursor(512);
    if (ch) {
        HLock((Handle) ch);
        SetCursor(*ch);
        HUnlock((Handle) ch);
    } else {
        SetCursor(&qdarrow);
    }
    return;
}

static void
macClickTerm(EventRecord *theEvent, WindowPtr theWindow)
{
    Point where = theEvent->where;

    GlobalToLocal(&where);
    BaseClick(GetNhWin(theWindow), where, theEvent->modifiers);
    return;
}

static void
macCursorTerm(EventRecord *theEvent, WindowPtr theWindow, RgnHandle mouseRgn)
{
    GrafPtr gp;
    Point where = theEvent->where;
    Rect r = { 0, 0, 1, 1 };

    GetPort(&gp);
    SetPortWindowPort(theWindow);
    GlobalToLocal(&where);
    BaseCursor(GetNhWin(theWindow), where);
    OffsetRect(&r, theEvent->where.h, theEvent->where.v);
    RectRgn(mouseRgn, &r);
    SetPort(gp);
    return;
}

/**********************************************************************
 *	Status subwindow
 */

/**********************************************************************
 *	Map subwindow
 */

/**********************************************************************
 *	Message window
 */

static void
MsgClick(NhWindow *wind, Point pt)
{
    int r_idx = 0;

    while (topl_resp[r_idx] && topl_resp[r_idx] != '\033' && r_idx < 10) {
        Rect frame;
        topl_resp_rect(r_idx, &frame);
        InsetRect(&frame, 1, 1);
        if (PtInRect(pt, &frame)) {
            Boolean in_btn = true;

            InvertRect(&frame);
            while (WaitMouseUp()) {
                SystemTask();
                GetMouse(&pt);
                if (PtInRect(pt, &frame) != in_btn) {
                    in_btn = !in_btn;
                    InvertRect(&frame);
                }
            }
            if (in_btn) {
                InvertRect(&frame);
                AddToKeyQueue(topl_resp[r_idx], 1);
            }
            return;
        }
        ++r_idx;
    }
    return;
}

/* Draw one yn-prompt button: centered label + rounded frame (heavier for
   the default). Takes a C string and converts it to Pascal here. */
static void
draw_topl_button(const Rect *frame, const char *label, Boolean is_default)
{
    Str255 name;
    FontInfo font;
    C2P(label, name);
    TextFont(kFontIDGeneva);
    TextSize(9);
    GetFontInfo(&font);
    MoveTo((frame->left + frame->right - StringWidth(name)) / 2,
           (frame->top + frame->bottom + font.ascent - font.descent
            - font.leading - 1) / 2);
    DrawString(name);
    PenNormal();
    if (is_default)
        PenSize(2, 2);
    FrameRoundRect(frame, 4, 4);
}

static void
MsgUpdate(NhWindow *wind)
{
    RgnHandle org_clip = NewRgn(), clip = NewRgn();
    Rect r;
    int l;

    if (!org_clip || !clip) {
        if (org_clip) DisposeRgn(org_clip);
        if (clip) DisposeRgn(clip);
        return;
    }
    GetClip(org_clip);
    GetWindowPortBounds(wind->its_window, &r);
    OffsetRect(&r, -r.left, -r.top);

    DrawControls(wind->its_window);
    if (wind->scrollBar && !small_screen)
        DrawGrowIcon(wind->its_window); /* borderless windows aren't growable */

    if (wind->scrollBar)
        r.right -= SBARWIDTH;
    r.bottom -= msg_bottom_inset();
    /* clip to portrect minus scrollbar/growicon BEFORE growing r past the window */
    RectRgn(clip, &r);
    SectRgn(clip, org_clip, clip);
    if (r.right < MIN_RIGHT)
        r.right = MIN_RIGHT;
    r.top -= wind->scrollPos * wind->row_height;

    /* The dotted divider line is drawn after the text (below). Clipping it out
       of the region here instead would flicker less but draw slower. */

    SetClip(clip); /* install clip BEFORE any text drawing */
    if (in_topl_mode()) {
        RgnHandle topl_rgn = NewRgn();
        Rect topl_r = r;
        topl_r.top += (wind->y_size - 1) * wind->row_height;
        l = (*top_line)->destRect.right - (*top_line)->destRect.left;
        (*top_line)->viewRect = topl_r;
        (*top_line)->destRect = topl_r;
        if (l != topl_r.right - topl_r.left)
            TECalText(top_line);
        TEUpdate(&topl_r, top_line);
        RectRgn(topl_rgn, &topl_r);
        DiffRgn(clip, topl_rgn, clip);
        DisposeRgn(topl_rgn);
        SetClip(clip); /* update clip to exclude topl area from TETextBox */
    }
    DisposeRgn(clip);

    TextFont(wind->font_number);
    TextSize(wind->font_size);
    HLock(wind->windowText);
    {
        long hsize = GetHandleSize(wind->windowText);
        long tlen = wind->windowTextLen;
        if (tlen > hsize)
            tlen = hsize;
        /* Bound TE work to the visible lines: TETextBox wraps and draws a
           throwaway record over all it is given, so the whole history made
           each message O(history). Slice lines [first, last) (1:1 with
           CHAR_CR) and draw at their offset; the box still spans r.bottom,
           so its implicit erase covers the same area. */
        char *base = *wind->windowText;
        long first = wind->scrollPos;
        long last, line, i;
        long startOff = 0, endOff = tlen;
        Rect box = r;

        if (first < 0)
            first = 0;
        /* r.top is already offset by -scrollPos rows, so the view top
           sits at r.top + first * row_height */
        box.top = r.top + (short) first * wind->row_height;
        last = first + (r.bottom - box.top) / wind->row_height + 1;
        for (i = 0, line = 0; i < tlen && line < last; i++) {
            if (base[i] == CHAR_CR) {
                line++;
                if (line == first)
                    startOff = i + 1;
            }
        }
        if (line >= last)
            endOff = i;       /* just past the last visible line's CR */
        if (line < first)
            startOff = tlen;  /* scrolled past the end: erase only */
        TETextBox(base + startOff, endOff - startOff, &box, teJustLeft);
    }
    HUnlock(wind->windowText);

    r.bottom = r.top + wind->save_lin * wind->row_height;
    r.top = r.bottom - 1;
    FillRect(&r, (void *) &qd.gray);

    /* Draw buttons LAST so TETextBox can't overwrite them */
    if (in_topl_mode() && topl_resp[0]) {
        SetClip(org_clip); /* restore full clip for button area */
        for (l = 0; topl_resp[l] && topl_resp[l] != '\033' && l < 10; l++) {
            Boolean is_def = (l == topl_def_idx);
            Rect frame;
            topl_resp_rect(l, &frame);
            switch (topl_resp[l]) {
            case 'y':  draw_topl_button(&frame, "yes", is_def); break;
            case 'n':  draw_topl_button(&frame, "no", is_def); break;
            case 'N':  draw_topl_button(&frame, "None", is_def); break;
            case 'a':  draw_topl_button(&frame, "all", is_def); break;
            case 'q':  draw_topl_button(&frame, "quit", is_def); break;
            case CHAR_ANY:
                draw_topl_button(&frame, "any key", is_def);
                break;
            default: {
                char one[2];
                one[0] = (char) topl_resp[l];
                one[1] = '\0';
                draw_topl_button(&frame, one, is_def);
                break;
            }
            }
        }
    }

    SetClip(org_clip);
    DisposeRgn(org_clip);
    return;
}

static void
macClickMessage(EventRecord *theEvent, WindowPtr theWindow)
{
    Point mouse = theEvent->where;

    GlobalToLocal(&mouse);
    MsgClick(GetNhWin(theWindow), mouse);
    macClickText(theEvent, theWindow);
}

static short
macUpdateMessage(EventRecord *theEvent, WindowPtr theWindow)
{
    if (!theEvent)
        return 0;
    MsgUpdate(GetNhWin(theWindow));
    return 0;
}

/**********************************************************************
 *	Menu windows
 */

/* Bulk selection commands for a PICK_ANY menu, mirroring the tty/Amiga menu
   keys (standard defaults): '.' select all, '-' deselect all, '@' invert all;
   ',' '\' '~' do the same for just the visible page. Updates the selection data
   for the affected items, then repaints via MenwUpdate. Returns true if ch was a
   selection command (handled), false to let normal accelerator handling run. */
static Boolean
MenwSelectCmd(NhWindow *wind, char ch)
{
    int act;          /* +1 = select, 0 = deselect, -1 = invert */
    Boolean page;     /* limit to the currently visible page */
    int i, vis_rows = 0;

    if (!wind || wind->how != PICK_ANY || !wind->menuInfo)
        return false;
    switch (ch) {
    case '.':  act = +1; page = false; break;
    case '-':  act =  0; page = false; break;
    case '@':  act = -1; page = false; break;
    case ',':  act = +1; page = true;  break;
    case '\\': act =  0; page = true;  break;
    case '~':  act = -1; page = true;  break;
    default:   return false;
    }

    if (page && wind->its_window) {
        Rect cr;
        GetWindowPortBounds(wind->its_window, &cr);
        vis_rows = (cr.bottom - cr.top) / wind->row_height;
    }

    HLock((char **) wind->menuInfo);
    for (i = 0; i < wind->miLen; i++) {
        int cur, want;
        if (page) {
            int row = (*wind->menuInfo)[i].line - wind->scrollPos;
            if (row <= 0 || row > vis_rows)
                continue;   /* not on the visible page */
        }
        cur  = (ListItemSelected(wind, i) >= 0);
        want = (act < 0) ? !cur : act;
        if (want != cur)
            ToggleMenuListItemSelected(wind, i);
    }
    HUnlock((char **) wind->menuInfo);

    SetPortWindowPort(wind->its_window);
    MenwUpdate(wind);   /* repaint text + re-hilite from the updated selection */
    return true;
}

static void
MenwKey(NhWindow *wind, char ch)
{
    MacMHMenuItem *mi;
    int i;

    ch = filter_scroll_key(ch, wind);
    if (!ch)
        return;
    if (ClosingWindowChar(ch)) {
        AddToKeyQueue(CHAR_CR, 1);
        return;
    }

    if (!wind || !wind->menuInfo)
        return;
    if (MenwSelectCmd(wind, ch))
        return;
    HLock((char **) wind->menuInfo);
    for (i = 0, mi = *wind->menuInfo; i < wind->miLen; i++, mi++) {
        if (mi->accelerator == ch) {
            ToggleMenuListItemSelected(wind, i);
            if (mi->line >= wind->scrollPos && mi->line <= wind->y_size) {
                SetPortWindowPort(wind->its_window);
                ToggleMenuSelect(wind, mi->line - wind->scrollPos);
            }
            /* Dismiss window if only picking one item */
            if (wind->how != PICK_ANY)
                AddToKeyQueue(CHAR_CR, 1);
            break;
        }
    }
    HUnlock((char **) wind->menuInfo);
    return;
}

static void
MenwClick(NhWindow *wind, Point pt)
{
    Rect wrect;

    GetWindowPortBounds(wind->its_window, &wrect);
    OffsetRect(&wrect, -wrect.left, -wrect.top);
    if (inSelect != WIN_ERR && wind->how != PICK_NONE) {
        short currentRow = -1, previousRow = -1;
        short previousItem = -1, item = -1;
        Boolean majorSelectState, firstRow = TRUE;

        do {
            SystemTask();
            GetMouse(&pt);
            currentRow = pt.v / wind->row_height;
            if (pt.h < wrect.left || pt.h > wrect.right || pt.v < 0
                || pt.v > wrect.bottom || currentRow >= wind->y_size) {
                continue; /* not in window range */
            }

            item = ListCoordinateToItem(wind, currentRow);

            if (item != previousItem) {
                /* typical Mac multiple-selection drag behavior */
                Boolean itemIsSelected = (ListItemSelected(wind, item) >= 0);

                if (firstRow) {
                    /* drag toggles toward the opposite of the first row's state */
                    majorSelectState = !itemIsSelected;
                    firstRow = FALSE;
                }

                if (wind->how == PICK_ONE && previousItem != -1) {
                    /* PICK_ONE: deselect the previous row first */
                    ToggleMenuListItemSelected(wind, previousItem);
                    ToggleMenuSelect(wind, previousRow);
                    previousItem = -1;
                }

                if (item == -1)
                    continue; /* header line */

                if (majorSelectState != itemIsSelected) {
                    ToggleMenuListItemSelected(wind, item);
                    ToggleMenuSelect(wind, currentRow);
                }

                previousRow = currentRow;
                previousItem = item;
            }
        } while (StillDown());

        /* Dismiss window if only picking one item */
        if (wind->how == PICK_ONE)
            AddToKeyQueue(CHAR_CR, 1);
    }
    return;
}

/* NetHack color index -> RGB for menu text on the WHITE menu background.
   (Same values as the map's table; the white-bg adjustments are in
   set_menu_text_color, not here.) */
static const RGBColor menuColorRGB[16] = {
    {0x0000, 0x0000, 0x0000},   /* 0  black   */
    {0xC0C0, 0x0000, 0x0000},   /* 1  red     */
    {0x0000, 0x8080, 0x0000},   /* 2  green   */
    {0x8080, 0x8080, 0x0000},   /* 3  brown   */
    {0x0000, 0x0000, 0xC0C0},   /* 4  blue    */
    {0x8080, 0x0000, 0x8080},   /* 5  magenta */
    {0x0000, 0x8080, 0x8080},   /* 6  cyan    */
    {0x8080, 0x8080, 0x8080},   /* 7  gray    */
    {0x0000, 0x0000, 0x0000},   /* 8  no color (unused; -> black) */
    {0xFFFF, 0x8080, 0x0000},   /* 9  orange  */
    {0x0000, 0xC0C0, 0x0000},   /* 10 bright green (darkened for white bg) */
    {0x8080, 0x8080, 0x0000},   /* 11 yellow (darkened for white bg) */
    {0x0000, 0x0000, 0xFFFF},   /* 12 bright blue */
    {0xC0C0, 0x0000, 0xC0C0},   /* 13 bright magenta */
    {0x0000, 0x8080, 0x8080},   /* 14 bright cyan (darkened for white bg) */
    {0x0000, 0x0000, 0x0000}    /* 15 white (-> black on white bg) */
};

/* Map a NetHack menu attribute to a QuickDraw text face. Headings come through
   as ATR_BOLD or (the default) ATR_INVERSE; both render bold here. */
static short
menu_attr_face(int attr)
{
    switch (attr) {
    case ATR_BOLD:
    case ATR_INVERSE: return bold;
    case ATR_ULINE:   return underline;
    default:          return normal;
    }
}

/* Set the pen color for a menu line. The menu background is white, so NO_COLOR
   and white become black, and on sub-4-bit screens colors are skipped. */
static void
set_menu_text_color(int color)
{
    RGBColor black = { 0, 0, 0 };
    GDHandle gd = GetMainDevice();
    short depth = gd ? (*(*gd)->gdPMap)->pixelSize : 1;

    if (depth < 4 || color == NO_COLOR || color == CLR_WHITE
        || color < 0 || color >= CLR_MAX)
        RGBForeColor(&black);
    else
        RGBForeColor(&menuColorRGB[color]);
}

/* Store {attr,color} for menu lines [from,to). Grows the per-line style handle. */
static void
record_menu_line_style(NhWindow *aWin, short from, short to, int attr, int color)
{
    long need = (long) to * 2;
    short l;
    unsigned char *b;

    if (to <= from)
        return;
    if (!aWin->menuStyle) {
        aWin->menuStyle = NewHandle(need > 128 ? need : 128);
        if (!aWin->menuStyle)
            return;
    } else if (GetHandleSize(aWin->menuStyle) < need) {
        SetHandleSize(aWin->menuStyle, need + 128);
        if (MemError())
            return;
    }
    HLock(aWin->menuStyle);
    b = (unsigned char *) *aWin->menuStyle;
    for (l = from; l < to; l++) {
        b[l * 2]     = (unsigned char) attr;
        b[l * 2 + 1] = (unsigned char) color;
    }
    HUnlock(aWin->menuStyle);
}

/* Draw a menu's text line-by-line with per-line face (bold headings) and color
   (menucolors), replacing TETextBox so each line can have its own style. */
static void
MenwDrawStyled(NhWindow *wind)
{
    Rect r, r2;
    RgnHandle h = (RgnHandle) 0;
    Boolean vis;
    char *base;
    long tlen, i, lineStart;
    short lineIdx, row, vis_rows;

    GetWindowPortBounds(wind->its_window, &r);
    OffsetRect(&r, -r.left, -r.top);
    r2 = r;
    r2.left = r2.right - SBARWIDTH;
    r2.right += 1;
    r2.top -= 1;
    vis = (r2.bottom > r2.top + 50);

    EraseRect(&r);   /* clear old text/hilites (white background) */
    draw_growicon_vert_only(wind->its_window);
    DrawControls(wind->its_window);

    /* clip text to exclude the scrollbar strip, preserving any update clip */
    if (vis && (h = NewRgn())) {
        RgnHandle tmp = NewRgn();
        if (!tmp) {
            DisposeRgn(h);
            h = (RgnHandle) 0;
        } else {
            GetClip(h);
            RectRgn(tmp, &r2);
            DiffRgn(h, tmp, tmp);
            SetClip(tmp);
            DisposeRgn(tmp);
        }
    }

    vis_rows = (r.bottom - r.top) / wind->row_height + 1;
    TextMode(srcOr);

    HLock(wind->windowText);
    if (wind->menuStyle)
        HLock(wind->menuStyle);
    base = *wind->windowText;
    tlen = wind->windowTextLen;
    lineStart = 0;
    lineIdx = 0;
    for (i = 0; i <= tlen; i++) {
        if (i == tlen || base[i] == CHAR_CR) {
            long llen = i - lineStart;
            row = lineIdx - wind->scrollPos;
            if (row >= 0 && row <= vis_rows && llen > 0) {
                int attr = ATR_NONE, color = NO_COLOR;
                if (wind->menuStyle
                    && (long) (lineIdx + 1) * 2 <= GetHandleSize(wind->menuStyle)) {
                    unsigned char *sb = (unsigned char *) *wind->menuStyle;
                    attr  = sb[lineIdx * 2];
                    color = sb[lineIdx * 2 + 1];
                }
                TextFace(menu_attr_face(attr));
                set_menu_text_color(color);
                MoveTo(r.left, row * wind->row_height + wind->ascent_height);
                /* pass the line via the pointer: lineStart can exceed
                   DrawText's 16-bit byte offset in a >32KB text window */
                if (llen > 0x7FFF)
                    llen = 0x7FFF; /* DrawText byteCount is a short */
                DrawText(base + lineStart, 0, (short) llen);
            }
            lineStart = i + 1;
            lineIdx++;
            if (i == tlen)
                break;
        }
    }
    {
        RGBColor black = { 0, 0, 0 };
        TextFace(normal);
        RGBForeColor(&black);
    }
    if (wind->menuStyle)
        HUnlock(wind->menuStyle);
    HUnlock(wind->windowText);

    if (h) {
        SetClip(h);
        DisposeRgn(h);
    }
}

static void
MenwUpdate(NhWindow *wind)
{
    int i, line;
    MacMHMenuItem *mi;

    MenwDrawStyled(wind);
    if (!wind->menuInfo || !wind->menuSelected || wind->miSelLen <= 0)
        return;
    HLock((Handle) wind->menuInfo);
    HLock((Handle) wind->menuSelected);
    for (i = 0; i < wind->miSelLen; i++) {
        mi = &(*wind->menuInfo)[(*wind->menuSelected)[i]];
        line = mi->line;
        if (line > wind->scrollPos && line <= wind->y_size)
            ToggleMenuSelect(wind, line - wind->scrollPos);
    }
    HUnlock((Handle) wind->menuInfo);
    HUnlock((Handle) wind->menuSelected);
    return;
}

static void
macKeyMenu(EventRecord *theEvent, WindowPtr theWindow)
{
    MenwKey(GetNhWin(theWindow), theEvent->message & 0xff);
    return;
}

static void
macClickMenu(EventRecord *theEvent, WindowRef theWindow)
{
    Point p;
    NhWindow *aWin = GetNhWin(theWindow);

    if (aWin->scrollBar && ((** aWin->scrollBar).contrlVis != 0)) {
        short code;
        ControlHandle theBar;

        p = theEvent->where;
        GlobalToLocal(&p);
        code = FindControl(p, theWindow, &theBar);
        if (code) {
            DoScrollBar(p, code, theBar, aWin);
            return;
        }
    }
    MenwClick(aWin, theEvent->where);
}

static short
macUpdateMenu(EventRecord *theEvent, WindowPtr theWindow)
{
    MenwUpdate(GetNhWin(theWindow));
    return 0;
}

/**********************************************************************
 *	Text windows
 */

static void
TextKey(NhWindow *wind, char ch)
{
    ch = filter_scroll_key(ch, wind);
    if (!ch)
        return;
    if (inSelect == WIN_ERR && ClosingWindowChar(ch)) {
        HideWindow(wind->its_window);
        mac_destroy_nhwindow(wind - theWindows);
    } else
        AddToKeyQueue(topl_resp_key(ch), TRUE);
    return;
}

static void
TextUpdate(NhWindow *wind)
{
    Rect r, r2;
    RgnHandle h;
    Boolean vis;

    GetWindowPortBounds(wind->its_window, &r);
    OffsetRect(&r, -r.left, -r.top);
    r2 = r;
    r2.left = r2.right - SBARWIDTH;
    r2.right += 1;
    r2.top -= 1;
    vis = (r2.bottom > r2.top + 50);

    draw_growicon_vert_only(wind->its_window);
    DrawControls(wind->its_window);

    h = (RgnHandle) 0;
    if (vis && (h = NewRgn())) {
        RgnHandle tmp = NewRgn();
        if (!tmp) {
            DisposeRgn(h);
            h = (RgnHandle) 0;
        } else {
            GetClip(h);
            RectRgn(tmp, &r2);
            DiffRgn(h, tmp, tmp);
            SetClip(tmp);
            DisposeRgn(tmp);
        }
    }
    if (r.right < MIN_RIGHT)
        r.right = MIN_RIGHT;
    r.top -= wind->scrollPos * wind->row_height;
    r.right -= SBARWIDTH;
    HLock(wind->windowText);
    {
        long hsize = GetHandleSize(wind->windowText);
        long tlen = wind->windowTextLen;
        if (tlen > hsize)
            tlen = hsize;
        TETextBox(*wind->windowText, tlen, &r, teJustLeft);
    }
    HUnlock(wind->windowText);
    if (h) {
        SetClip(h);
        DisposeRgn(h);
    }
    return;
}

static void
macKeyText(EventRecord *theEvent, WindowPtr theWindow)
{
    TextKey(GetNhWin(theWindow), theEvent->message & 0xff);
    return;
}

static void
macClickText(EventRecord *theEvent, WindowPtr theWindow)
{
    NhWindow *aWin = GetNhWin(theWindow);

    if (aWin->scrollBar && ((** aWin->scrollBar).contrlVis != 0)) {
        short code;
        Point p = theEvent->where;
        ControlHandle theBar;

        GlobalToLocal(&p);
        code = FindControl(p, theWindow, &theBar);
        if (code) {
            DoScrollBar(p, code, theBar, aWin);
        }
    }
}

/**********************************************************************
 *	Global events
 */

static short
macDoNull(EventRecord *theEvent, WindowPtr theWindow)
{
    return 0;
}

/* theWindow may be null here: keyDown can dispatch with no front window */
static void
GeneralKey(EventRecord *theEvent, WindowPtr theWindow)
{
    unsigned char ch;

    if (theEvent->modifiers & optionKey) {
        /* Option = Meta: re-translate without Option to get the base char,
           then set the high bit so the core sees M-<key> */
        unsigned short keyCode = (theEvent->message >> 8) & 0xff;
        unsigned long state = 0;
        Handle kchr = GetResource('KCHR', 0);
        if (kchr) {
            unsigned long result = KeyTranslate(*kchr, keyCode, &state);
            ch = (result & 0xff);
            if (ch)
                ch |= 0x80;
            else
                ch = theEvent->message & 0xff;
        } else {
            ch = theEvent->message & 0xff;
        }
    } else {
        ch = theEvent->message & 0xff;
    }
    AddToKeyQueue(topl_resp_key(ch), TRUE);
}

static void
HandleKey(EventRecord *theEvent)
{
    WindowPtr theWindow = FrontWindow();

    if (theEvent->modifiers & cmdKey) {
        if ((theEvent->message & 0xff) == '.') {
            /* Flush key queue */
            keyQueueCount = keyQueueWrite = keyQueueRead = 0;
            theEvent->message = '\033';
            goto dispatchKey;
        } else {
            UndimMenuBar();
            DoMenuEvt(MenuKey(theEvent->message & 0xff));
        }
    } else {
    dispatchKey:
        if (theWindow) {
            int kind = GetWindowKind(theWindow) - WIN_BASE_KIND;
            if (kind >= 0 && kind < NUM_FUNCS)
                winKeyFuncs[kind](theEvent, theWindow);
        } else {
            GeneralKey(theEvent, (WindowPtr) 0);
        }
    }
}

static void
HandleClick(EventRecord *theEvent)
{
    int code;
    unsigned long l;
    WindowPtr theWindow;
    NhWindow *aWin;
    Rect r;
    Boolean not_inSelect;

    r = (*GetGrayRgn())->rgnBBox;
    InsetRect(&r, 4, 4);

    code = FindWindow(theEvent->where, &theWindow);
    aWin = GetNhWin(theWindow);
    not_inSelect = (inSelect == WIN_ERR || aWin - theWindows == inSelect);

    switch (code) {
    case inContent:
        if (not_inSelect) {
            int kind = GetWindowKind(theWindow) - WIN_BASE_KIND;
            if (kind >= 0 && kind < NUM_FUNCS) {
                winCursorFuncs[kind](theEvent, theWindow, gMouseRgn);
                SelectWindow(theWindow);
                SetPortWindowPort(theWindow);
                winClickFuncs[kind](theEvent, theWindow);
            }
        } else {
            nhbell();
        }
        break;

    case inDrag:
        if (not_inSelect) {
            SetCursor(&qdarrow);
            DragWindow(theWindow, theEvent->where, &r);
            SaveWindowPos(theWindow); /* into the prefs file */
        } else {
            nhbell();
        }
        break;

    case inGrow:
        if (not_inSelect) {
            SetCursor(&qdarrow);
            if (GetWRefCon(theWindow) == MACMAP_REFCON) {
                Rect growLimits;   /* (minW, minH, maxW, maxH) */
                SetRect(&growLimits, 200, 80, 2048, 1536);
                l = GrowWindow(theWindow, theEvent->where, &growLimits);
                if (l)
                    macmap_grow_event(&theWindows[WIN_MAP], l);
            } else {
                SetRect(&r, 80, 2 * aWin->row_height + 1, r.right, r.bottom);
                if (aWin == theWindows + WIN_MESSAGE)
                    r.top += SBARHEIGHT;
                l = GrowWindow(theWindow, theEvent->where, &r);
                SizeWindow(theWindow, l & 0xffff, l >> 16, FALSE);
                SaveWindowSize(theWindow);
                SetPortWindowPort(theWindow);
                GetWindowPortBounds(theWindow, &r);
                OffsetRect(&r, -r.left, -r.top);
                InvalWindowRect(theWindow, &r);
                if (aWin->scrollBar) {
                    DrawScrollbar(aWin);
                }
            }
        } else {
            nhbell();
        }
        break;

    case inGoAway:
        WindowGoAway(theEvent, theWindow);
        break;

    case inMenuBar:
        DoMenuEvt(MenuSelect(theEvent->where));
        break;

    case inSysWindow:
        SystemClick(theEvent, theWindow);
    default:
        break;
    }
}

static short
GeneralUpdate(EventRecord *theEvent, WindowPtr theWindow)
{
    if (!theEvent)
        return 0;
    TextUpdate(GetNhWin(theWindow));
    return 0;
}

static void
HandleUpdate(EventRecord *theEvent)
{
    WindowPtr theWindow = (WindowPtr) theEvent->message;
    NhWindow *aWin = GetNhWin(theWindow);
    Rect r;
    EventRecord fake = {0};

    char existing_update_region = FALSE;
    Rect rect;

    if (!aWin && theWindow != _mt_window) {
        /* Check if this is the dedicated map window. */
        if (WIN_MAP != WIN_ERR && theWindows[WIN_MAP].its_window == theWindow) {
            BeginUpdate(theWindow);
            macmap_update_event(&theWindows[WIN_MAP]);
            EndUpdate(theWindow);
            return;
        }
        BeginUpdate(theWindow);
        EndUpdate(theWindow);
        return;
    }

    if (theWindow == _mt_window) {
        existing_update_region =
            (get_invalid_region(theWindow, &rect) == noErr);
    }
    BeginUpdate(theWindow);
    SetPortWindowPort(theWindow);
    GetWindowPortBounds(theWindow, &r);
    OffsetRect(&r, -r.left, -r.top);
    EraseRect(&r);
    {
        int kind = GetWindowKind(theWindow) - WIN_BASE_KIND;
        /* Distinguish the macmap window (its own Mac WindowPtr) from
           _mt_window — both share kind=NHW_MAP for legacy reasons. */
        if (GetWRefCon(theWindow) == MACMAP_REFCON && WIN_MAP != WIN_ERR) {
            macmap_update_event(&theWindows[WIN_MAP]);
        } else if (kind >= 0 && kind < NUM_FUNCS) {
            winUpdateFuncs[kind](&fake, theWindow);
        }
    }

    if (theWindow == _mt_window && existing_update_region) {
        set_invalid_region(theWindow, &rect);
    }
    if (aWin)
        aWin->drawn = TRUE;
    EndUpdate(theWindow);
}

static void
GeneralCursor(EventRecord *theEvent, WindowPtr theWindow, RgnHandle mouseRgn)
{
    Rect r = { -1, -1, 2, 2 };

    SetCursor(&qdarrow);
    OffsetRect(&r, theEvent->where.h, theEvent->where.v);
    RectRgn(mouseRgn, &r);
}

static void
DoOsEvt(EventRecord *theEvent)
{
    WindowRef win;
    short code;
    unsigned long msgClass = (theEvent->message >> 24) & 0xFF;

    if (msgClass == 0xFA) {
        /* Mouse Moved */

        code = FindWindow(theEvent->where, &win);
        if (code != inContent) {
            Rect r = { -1, -1, 2, 2 };

            SetCursor(&qdarrow);
            OffsetRect(&r, theEvent->where.h, theEvent->where.v);
            RectRgn(gMouseRgn, &r);
        } else {
            int kind = GetWindowKind(win) - WIN_BASE_KIND;
            if (kind >= 0 && kind <= NHW_TEXT) {
                winCursorFuncs[kind](theEvent, win, gMouseRgn);
            }
        }
    } else if (msgClass == suspendResumeMessage) {
        /* Suspend / Resume */
        if (theEvent->message & resumeFlag) {
            /* Resuming: re-check if tile mode is still available */
            NhWindow *map = (WIN_MAP != WIN_ERR) ? &theWindows[WIN_MAP] : NULL;
            if (map && map->tile_mode && !mactile_available()) {
                /* Route through macmap_set_mode so palette cleanup runs. */
                macmap_set_mode(map, false);
                iflags.wc_tiled_map = FALSE;          /* keep NHDeflts in sync */
                InvalWindowRect(map->its_window, &map->its_window->portRect);
                gTileMenuNeedsUpdate = 1;
            }
        }
    }
}

void
HandleEvent(EventRecord *theEvent)
{
    switch (theEvent->what) {
    case autoKey:
    case keyDown:
        HandleKey(theEvent);
        break;
    case updateEvt:
        HandleUpdate(theEvent);
        break;
    case mouseDown:
        HandleClick(theEvent);
        break;
    case diskEvt:
        if ((theEvent->message & 0xffff0000) != 0) {
            Point p = { 150, 150 };
            (void) DIBadMount(p, theEvent->message);
        }
        break;
    case osEvt:
        DoOsEvt(theEvent);
        break;
    case kHighLevelEvent:
        AEProcessAppleEvent(theEvent);
        break;
    default:
        /* Idle: refresh menu state if mactile signals an update. */
        mactile_menu_refresh();
        break;
    }
}

/**********************************************************************
 *	Interface definition, for windows.c
 */

/* mttymain.c: Mac-specific color functions (renamed to avoid conflict
   with wintty.c's versions when both are linked) */
extern void mac_change_color(int, long, int);
extern void mac_change_background(int);
extern char *mac_get_color_string(void);

struct window_procs mac_procs = {
    WPID(mac),
    WC_COLOR | WC_HILITE_PET | WC_FONT_MAP | WC_FONT_MENU | WC_FONT_MESSAGE
        | WC_FONT_STATUS | WC_FONT_TEXT | WC_FONTSIZ_MAP | WC_FONTSIZ_MENU
        | WC_FONTSIZ_MESSAGE | WC_FONTSIZ_STATUS | WC_FONTSIZ_TEXT
        | WC_TILED_MAP,
    WC2_SUPPRESS_HIST,   /* honor ATR_NOHISTORY: transient msgs (e.g. farlook
                            descriptions) replace the line instead of logging */
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    mac_init_nhwindows,
    mac_player_selection,
    mac_askname, mac_get_nh_event, mac_exit_nhwindows, mac_suspend_nhwindows,
    mac_resume_nhwindows, mac_create_nhwindow, mac_clear_nhwindow,
    mac_display_nhwindow, mac_destroy_nhwindow, mac_curs, mac_putstr,
    genl_putmixed, mac_display_file, mac_start_menu, mac_add_menu,
    mac_end_menu, mac_select_menu, genl_message_menu,
    mac_mark_synch, mac_get_nh_event, /* wait_synch */
#ifdef CLIPPING
    mac_cliparound,
#endif
#ifdef POSITIONBAR
    donull,
#endif
    mac_print_glyph, mac_raw_print, mac_raw_print_bold, mac_nhgetch,
    mac_nh_poskey, tty_nhbell, mac_doprev_message, mac_yn_function,
    mac_getlin, mac_get_ext_cmd, mac_number_pad, mac_delay_output,
#ifdef CHANGE_COLOR
    mac_change_color,
#ifdef MAC68K
    mac_change_background, set_tty_font_name,
#endif
    mac_get_color_string,
#endif
    genl_outrip, genl_preference_update,
    genl_getmsghistory, genl_putmsghistory,
    genl_status_init, genl_status_finish, genl_status_enablefield,
    genl_status_update,
    genl_can_suspend_no,
    mac_update_inventory,
    mac_ctrl_nhwindow,
};

/*macwin.c*/

/* NetHack 3.6	maccurs.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) Jon W{tte, 1992.				  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mactty.h"
#include "macwin.h"

#if 1 /*!TARGET_API_MAC_CARBON*/
#include <Folders.h>
#include <TextUtils.h>
#include <Resources.h>
#endif

static Boolean winFileInit = 0;
static unsigned char winFileName[32] = "\pNetHack Preferences";
static long winFileDir;
static short winFileVol;

typedef struct WinPosSave {
    char validPos;
    char validSize;
    short top;
    short left;
    short height;
    short width;
} WinPosSave;

static WinPosSave savePos[kLastWindowKind + 1];

static void
InitWinFile(void)
{
    StringHandle sh;
    long len;
    short ref = 0;

    if (winFileInit) {
        return;
    }
    /* We trust the glue. If there's an error, store in game dir. */
    if (FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder,
                   &winFileVol, &winFileDir)) {
        winFileVol = 0;
        winFileDir = 0;
    }
    sh = GetString(128);
    if (sh && *sh) {
        BlockMove(*sh, winFileName, **sh + 1);
        ReleaseResource((Handle) sh);
    }
    if (HOpen(winFileVol, winFileDir, winFileName, fsRdPerm, &ref)) {
        return;
    }
    len = sizeof(savePos);
    if (!FSRead(ref, &len, savePos)) {
        winFileInit = 1;
    }
    FSClose(ref);
}

static void
FlushWinFile(void)
{
    short ref;
    long len;

    if (!winFileInit) {
        if (!winFileName[0]) {
            return;
        }
        HCreate(winFileVol, winFileDir, winFileName, MAC_CREATOR, PREF_TYPE);
        HCreateResFile(winFileVol, winFileDir, winFileName);
    }
    if (HOpen(winFileVol, winFileDir, winFileName, fsWrPerm, &ref)) {
        return;
    }
    winFileInit = 1;
    len = sizeof(savePos);
    (void) FSWrite(ref, &len, savePos); /* Don't care about error */
    FSClose(ref);
}

Boolean
RetrievePosition(short kind, short *top, short *left)
{
    Point p;

    if (kind < 0 || kind > kLastWindowKind) {
        dprintf("Retrieve Bad kind %d", kind);
        return 0;
    }
    InitWinFile();
    if (!savePos[kind].validPos) {
        dprintf("Retrieve Not stored kind %d", kind);
        return 0;
    }
    p.v = savePos[kind].top;
    p.h = savePos[kind].left;
    *left = p.h;
    *top = p.v;
    dprintf("Retrieve Kind %d Pt (%d,%d)", kind, p.h, p.v);
    return (PtInRgn(p, GetGrayRgn()));
}

Boolean
RetrieveSize(short kind, short top, short left, short *height, short *width)
{
    Point p;

    if (kind < 0 || kind > kLastWindowKind) {
        return 0;
    }
    InitWinFile();
    if (!savePos[kind].validSize) {
        return 0;
    }
    *width = savePos[kind].width;
    *height = savePos[kind].height;
    p.h = left + *width;
    p.v = top + *height;
    return PtInRgn(p, GetGrayRgn());
}

static void
SavePosition(short kind, short top, short left)
{
    if (kind < 0 || kind > kLastWindowKind) {
        dprintf("Save bad kind %d", kind);
        return;
    }
    InitWinFile();
    savePos[kind].validPos = 1;
    savePos[kind].top = top;
    savePos[kind].left = left;
    dprintf("Save kind %d pt (%d,%d)", kind, left, top);
    FlushWinFile();
}

static void
SaveSize(short kind, short height, short width)
{
    if (kind < 0 || kind > kLastWindowKind) {
        dprintf("Save bad kind %d", kind);
        return;
    }
    InitWinFile();
    savePos[kind].validSize = 1;
    savePos[kind].width = width;
    savePos[kind].height = height;
    FlushWinFile();
}

static short
GetWinKind(WindowPtr win)
{
    short kind;

    if (!CheckNhWin(win)) {
        return -1;
    }
    kind = GetWindowKind(win) - WIN_BASE_KIND;
    if (kind < 0 || kind > NHW_TEXT) {
        return -1;
    }
    dprintf("In win kind %d (%lx)", kind, win);
    switch (kind) {
    case NHW_MAP:
    case NHW_STATUS:
    case NHW_BASE:
        kind = kMapWindow;
        break;
    case NHW_MESSAGE:
        kind = kMessageWindow;
        break;
    case NHW_MENU:
        kind = kMenuWindow;
        break;
    default:
        kind = kTextWindow;
        break;
    }
    dprintf("Out kind %d", kind);
    return kind;
}

Boolean
RetrieveWinPos(WindowPtr win, short *top, short *left)
{
    return RetrievePosition(GetWinKind(win), top, left);
}

void
SaveWindowPos(WindowPtr win)
{
    Rect r;

    GetWindowBounds(win, kWindowContentRgn, &r);
    SavePosition(GetWinKind(win), r.top, r.left);
}

void
SaveWindowSize(WindowPtr win)
{
    short width, height;
    Rect r;

    GetWindowBounds(win, kWindowContentRgn, &r);
    width = r.right - r.left;
    height = r.bottom - r.top;
    SaveSize(GetWinKind(win), height, width);
}

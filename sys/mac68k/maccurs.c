/* NetHack 5.0	maccurs.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) Jon W{tte, 1992.				  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mactty.h"
#include "macwin.h"
#include "macmap.h"

#include <Folders.h>
#include <TextUtils.h>
#include <Resources.h>

static Boolean winFileInit = 0;
static FSSpec winFileSpec; /* set in InitWinFile; name[0]==0 => unusable */

typedef struct WinPosSave {
    char validPos;
    char validSize;
    short top;
    short left;
    short height;
    short width;
} WinPosSave;

static WinPosSave savePos[kLastWindowKind + 1];

/* UI settings stored after savePos[] in the same file; zeroed (valid==0)
   when the file predates them or the version doesn't match */
static UiPrefs uiPrefs;

static void
InitWinFile(void)
{
    StringHandle sh;
    long len;
    short ref = 0;
    OSErr err;
    unsigned char name[32]; /* Pascal string */
    long dir;
    short vol;

    if (winFileInit) {
        return;
    }
    /* We trust the glue. If there's an error, store in game dir. */
    if (FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder,
                   &vol, &dir)) {
        vol = 0;
        dir = 0;
    }
    C2P("NetHack Preferences", name); /* default; STR 128 overrides */
    sh = GetString(128);
    if (sh) {
        if (*sh && **sh < sizeof(name))
            BlockMove(*sh, name, **sh + 1);
        ReleaseResource((Handle) sh);
    }
    /* fnfErr just means no prefs file yet; the spec is still good for
       the create in FlushWinFile */
    err = FSMakeFSSpec(vol, dir, name, &winFileSpec);
    if (err != noErr && err != fnfErr) {
        winFileSpec.name[0] = 0;
        return;
    }
    if (FSpOpenDF(&winFileSpec, fsRdPerm, &ref)) {
        return;
    }
    len = sizeof(savePos);
    if (FSRead(ref, &len, savePos) != noErr || len < (long) sizeof(savePos)) {
        /* error or short read (e.g. prefs from an older layout):
           don't trust partial data */
        memset(savePos, 0, sizeof savePos);
        memset(&uiPrefs, 0, sizeof uiPrefs);
    } else {
        len = sizeof(uiPrefs);
        if (FSRead(ref, &len, &uiPrefs) != noErr
            || len < (long) sizeof(uiPrefs)
            || uiPrefs.version != UIPREFS_VERSION)
            memset(&uiPrefs, 0, sizeof uiPrefs);
    }
    winFileInit = 1; /* don't retry on every call */
    FSClose(ref);
}

static void
FlushWinFile(void)
{
    short ref;
    long len;

    if (!winFileInit) {
        InitWinFile();
        if (!winFileSpec.name[0]) {
            return;
        }
        FSpCreate(&winFileSpec, MAC_CREATOR, PREF_TYPE, smSystemScript);
    }
    if (FSpOpenDF(&winFileSpec, fsWrPerm, &ref)) {
        return;
    }
    winFileInit = 1;
    len = sizeof(savePos);
    (void) FSWrite(ref, &len, savePos); /* Don't care about error */
    len = sizeof(uiPrefs);
    (void) FSWrite(ref, &len, &uiPrefs);
    FSClose(ref);
}

Boolean
RetrieveUiPrefs(UiPrefs *up)
{
    InitWinFile();
    if (!uiPrefs.valid)
        return 0;
    *up = uiPrefs;
    return 1;
}

void
StoreUiPrefs(const UiPrefs *up)
{
    InitWinFile();
    uiPrefs = *up;
    uiPrefs.version = UIPREFS_VERSION;
    FlushWinFile();
}

Boolean
RetrievePosition(short kind, short *top, short *left)
{
    Point p;

    if (kind < 0 || kind > kLastWindowKind) {
        mac_dprintf("Retrieve Bad kind %d", kind);
        return 0;
    }
    InitWinFile();
    if (!savePos[kind].validPos) {
        mac_dprintf("Retrieve Not stored kind %d", kind);
        return 0;
    }
    p.v = savePos[kind].top;
    p.h = savePos[kind].left;
    *left = p.h;
    *top = p.v;
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
    /* reject degenerate sizes (e.g. from prefs written by builds that
       saved GrowWindow's 0-on-no-change result) */
    if (savePos[kind].width <= 0 || savePos[kind].height <= 0) {
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
        mac_dprintf("Save bad kind %d", kind);
        return;
    }
    InitWinFile();
    savePos[kind].validPos = 1;
    savePos[kind].top = top;
    savePos[kind].left = left;
    FlushWinFile();
}

static void
SaveSize(short kind, short height, short width)
{
    if (kind < 0 || kind > kLastWindowKind) {
        mac_dprintf("Save bad kind %d", kind);
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
    extern WindowPtr _mt_window;

    if (!CheckNhWin(win)) {
        return -1;
    }
    if (GetWRefCon(win) == MACMAP_REFCON)   /* dedicated map window */
        return kMapWindow;
    if (win == _mt_window)                  /* shared base/status tty window */
        return kStatusWindow;
    if (WIN_INVEN != WIN_ERR && win == theWindows[WIN_INVEN].its_window)
        return kInvenWindow;                /* perm-invent windoid */
    kind = GetWindowKind(win) - WIN_BASE_KIND;
    if (kind < 0 || kind > NHW_TEXT) {
        return -1;
    }
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
    GrafPtr org_port;
    Point p;
    Rect r;

    /* consumers (adjust_window_pos/MoveWindow) treat the saved value as
       a GLOBAL screen position; portRect's top/left are port-local
       (always 0,0, or the SetOrigin offset), so convert first */
    GetPort(&org_port);
    SetPortWindowPort(win);
    GetWindowPortBounds(win, &r);
    p.h = r.left;
    p.v = r.top;
    LocalToGlobal(&p);
    SetPort(org_port);
    SavePosition(GetWinKind(win), p.v, p.h);
}

/* For windows whose saved size depends on more than the window identity:
   the map window keeps one size per display mode (kMapWindow for text,
   kMapTileWindow for tiles), chosen by macmap.c. */
void
SaveSizeForKind(short kind, short height, short width)
{
    SaveSize(kind, height, width);
}

void
SaveWindowSize(WindowPtr win)
{
    short width, height;
    Rect r;

    GetWindowPortBounds(win, &r);
    width = r.right - r.left;
    height = r.bottom - r.top;
    SaveSize(GetWinKind(win), height, width);
}

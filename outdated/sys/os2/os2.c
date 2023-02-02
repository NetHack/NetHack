/* NetHack 3.6	os2.c	$NHDT-Date: 1432512793 2015/05/25 00:13:13 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/*	Copyright (c) Timo Hakulinen, 1990, 1991, 1992, 1993, 1996. */
/*	NetHack may be freely redistributed.  See license for details. */

/*
 *  OS/2 system functions.
 */

#define NEED_VARARGS
#include "hack.h"

#ifdef OS2

#include "tcap.h"

/* OS/2 system definitions */

#ifdef __EMX__
#undef CLR_BLACK
#undef CLR_WHITE
#undef CLR_BLUE
#undef CLR_RED
#undef CLR_GREEN
#undef CLR_CYAN
#undef CLR_YELLOW
#undef CLR_BROWN
#endif

#include "def_os2.h"

#include <ctype.h>

static char DOSgetch(void);
static char BIOSgetch(void);

int
tgetch()
{
    char ch;

    /* BIOSgetch can use the numeric key pad on IBM compatibles. */
    if (iflags.BIOS)
        ch = BIOSgetch();
    else
        ch = DOSgetch();
    return ((ch == '\r') ? '\n' : ch);
}

/*
 *  Keyboard translation tables.
 */
#define KEYPADLO 0x47
#define KEYPADHI 0x53

#define PADKEYS (KEYPADHI - KEYPADLO + 1)
#define iskeypad(x) (KEYPADLO <= (x) && (x) <= KEYPADHI)

/*
 * Keypad keys are translated to the normal values below.
 * When iflags.BIOS is active, shifted keypad keys are translated to the
 *    shift values below.
 */
static const struct pad {
    char normal, shift, cntrl;
} keypad[PADKEYS] =
    {
      { 'y', 'Y', C('y') },    /* 7 */
      { 'k', 'K', C('k') },    /* 8 */
      { 'u', 'U', C('u') },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { 'h', 'H', C('h') },    /* 4 */
      { 'g', 'g', 'g' },       /* 5 */
      { 'l', 'L', C('l') },    /* 6 */
      { 'p', 'P', C('p') },    /* + */
      { 'b', 'B', C('b') },    /* 1 */
      { 'j', 'J', C('j') },    /* 2 */
      { 'n', 'N', C('n') },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
    },
  numpad[PADKEYS] = {
      { '7', M('7'), '7' },    /* 7 */
      { '8', M('8'), '8' },    /* 8 */
      { '9', M('9'), '9' },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { '4', M('4'), '4' },    /* 4 */
      { 'g', 'G', 'g' },       /* 5 */
      { '6', M('6'), '6' },    /* 6 */
      { 'p', 'P', C('p') },    /* + */
      { '1', M('1'), '1' },    /* 1 */
      { '2', M('2'), '2' },    /* 2 */
      { '3', M('3'), '3' },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
  };

/*
 * Unlike Ctrl-letter, the Alt-letter keystrokes have no specific ASCII
 * meaning unless assigned one by a keyboard conversion table, so the
 * keyboard BIOS normally does not return a character code when Alt-letter
 * is pressed.	So, to interpret unassigned Alt-letters, we must use a
 * scan code table to translate the scan code into a letter, then set the
 * "meta" bit for it.  -3.
 */
#define SCANLO 0x10
#define SCANHI 0x32
#define SCANKEYS (SCANHI - SCANLO + 1)
#define inmap(x) (SCANLO <= (x) && (x) <= SCANHI)

static const char scanmap[SCANKEYS] = {
    /* ... */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a',
    's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x',
    'c', 'v', 'b', 'N', 'm' /* ... */
};

/*
 * BIOSgetch emulates the MSDOS way of getting keys directly with a BIOS call.
 */
#define SHIFT_KEY (0x1 | 0x2)
#define CTRL_KEY 0x4
#define ALT_KEY 0x8

static char
BIOSgetch()
{
    unsigned char scan, shift, ch;
    const struct pad *kpad;

    KBDKEYINFO CharData;
    USHORT IOWait = 0;
    HKBD KbdHandle = 0;

    KbdCharIn(&CharData, IOWait, KbdHandle);
    ch = CharData.chChar;
    scan = CharData.chScan;
    shift = CharData.fsState;

    /* Translate keypad keys */
    if (iskeypad(scan)) {
        kpad = iflags.num_pad ? numpad : keypad;
        if (shift & SHIFT_KEY)
            ch = kpad[scan - KEYPADLO].shift;
        else if (shift & CTRL_KEY)
            ch = kpad[scan - KEYPADLO].cntrl;
        else
            ch = kpad[scan - KEYPADLO].normal;
    }
    /* Translate unassigned Alt-letters */
    if ((shift & ALT_KEY) && !ch) {
        if (inmap(scan))
            ch = scanmap[scan - SCANLO];
        return (isprint(ch) ? M(ch) : ch);
    }
    return ch;
}

static char
DOSgetch()
{
    KBDKEYINFO CharData;
    USHORT IOWait = 0;
    HKBD KbdHandle = 0;

    KbdCharIn(&CharData, IOWait, KbdHandle);
    if (CharData.chChar == 0) { /* an extended code -- not yet supported */
        KbdCharIn(&CharData, IOWait, KbdHandle); /* eat the next character */
        CharData.chChar = 0;                     /* and return a 0 */
    }
    return (CharData.chChar);
}

char
switchar()
{
    return '/';
}

int
kbhit()
{
    KBDKEYINFO CharData;
    HKBD KbdHandle = 0;

    KbdPeek(&CharData, KbdHandle);
    return (CharData.fbStatus & (1 << 6));
}

long
freediskspace(path)
char *path;
{
    FSALLOCATE FSInfoBuf;
#ifdef OS2_32BITAPI
    ULONG
#else
    USHORT
#endif
    DriveNumber, FSInfoLevel = 1, res;

    if (path[0] && path[1] == ':')
        DriveNumber = (toupper(path[0]) - 'A') + 1;
    else
        DriveNumber = 0;
    res =
#ifdef OS2_32BITAPI
        DosQueryFSInfo(DriveNumber, FSInfoLevel, (PVOID) &FSInfoBuf,
                       (ULONG) sizeof(FSInfoBuf));
#else
        DosQFSInfo(DriveNumber, FSInfoLevel, (PBYTE) &FSInfoBuf,
                   (USHORT) sizeof(FSInfoBuf));
#endif
    if (res)
        return -1L; /* error */
    else
        return ((long) FSInfoBuf.cSectorUnit * FSInfoBuf.cUnitAvail
                * FSInfoBuf.cbSector);
}

/*
 * Functions to get filenames using wildcards
 */

#ifdef OS2_32BITAPI
static FILEFINDBUF3 ResultBuf;
#else
static FILEFINDBUF ResultBuf;
#endif
static HDIR DirHandle;

int
findfirst(path)
char *path;
{
#ifdef OS2_32BITAPI
    ULONG
#else
    USHORT
#endif
    res, SearchCount = 1;

    DirHandle = 1;
    res =
#ifdef OS2_32BITAPI
        DosFindFirst((PSZ) path, &DirHandle, 0L, (PVOID) &ResultBuf,
                     (ULONG) sizeof(ResultBuf), &SearchCount, 1L);
#else
        DosFindFirst((PSZ) path, &DirHandle, 0, &ResultBuf,
                     (USHORT) sizeof(ResultBuf), &SearchCount, 0L);
#endif
    return (!res);
}

int
findnext()
{
#ifdef OS2_32BITAPI
    ULONG
#else
    USHORT
#endif
    res, SearchCount = 1;

    res =
#ifdef OS2_32BITAPI
        DosFindNext(DirHandle, (PVOID) &ResultBuf, (ULONG) sizeof(ResultBuf),
                    &SearchCount);
#else
        DosFindNext(DirHandle, &ResultBuf, (USHORT) sizeof(ResultBuf),
                    &SearchCount);
#endif
    return (!res);
}

char *
foundfile_buffer()
{
    return (ResultBuf.achName);
}

long
filesize(file)
char *file;
{
    if (findfirst(file)) {
        return (*(long *) (ResultBuf.cbFileAlloc));
    } else
        return -1L;
}

/*
 * Chdrive() changes the default drive.
 */
void
chdrive(str)
char *str;
{
    char *ptr;
    char drive;

    if ((ptr = strchr(str, ':')) != (char *) 0) {
        drive = toupper(*(ptr - 1));
#ifdef OS2_32BITAPI
        DosSetDefaultDisk((ULONG)(drive - 'A' + 1));
#else
        DosSelectDisk((USHORT)(drive - 'A' + 1));
#endif
    }
}

void
disable_ctrlP()
{
    KBDINFO KbdInfo;
    HKBD KbdHandle = 0;

    if (!iflags.rawio)
        return;
    KbdInfo.cb = sizeof(KbdInfo);
    KbdGetStatus(&KbdInfo, KbdHandle);
    KbdInfo.fsMask &= 0xFFF7; /* ASCII off */
    KbdInfo.fsMask |= 0x0004; /* BINARY on */
    KbdSetStatus(&KbdInfo, KbdHandle);
}

void
enable_ctrlP()
{
    KBDINFO KbdInfo;
    HKBD KbdHandle = 0;

    if (!iflags.rawio)
        return;
    KbdInfo.cb = sizeof(KbdInfo);
    KbdGetStatus(&KbdInfo, KbdHandle);
    KbdInfo.fsMask &= 0xFFFB; /* BINARY off */
    KbdInfo.fsMask |= 0x0008; /* ASCII on */
    KbdSetStatus(&KbdInfo, KbdHandle);
}

void
get_scr_size()
{
    VIOMODEINFO ModeInfo;
    HVIO VideoHandle = 0;

    ModeInfo.cb = sizeof(ModeInfo);

    (void) VioGetMode(&ModeInfo, VideoHandle);

    CO = ModeInfo.col;
    LI = ModeInfo.row;
}

void
gotoxy(x, y)
int x, y;
{
    HVIO VideoHandle = 0;

    x--;
    y--; /* (0,0) is upper right corner */

    (void) VioSetCurPos(x, y, VideoHandle);
}

char *
get_username(lan_username_size)
int *lan_username_size;
{
    return (char *) 0;
}
#ifdef X11_GRAPHICS
int errno;
#endif
#endif /* OS2 */

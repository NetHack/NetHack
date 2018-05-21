/* NetHack 3.6	winnt.c	$NHDT-Date: 1524321419 2018/04/21 14:36:59 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.30 $ */
/* Copyright (c) NetHack PC Development Team 1993, 1994 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  WIN32 system functions.
 *
 *  Initial Creation: Michael Allison - January 31/93
 *
 */

#define NEED_VARARGS
#include "hack.h"
#include <dos.h>
#ifndef __BORLANDC__
#include <direct.h>
#endif
#include <ctype.h>
#include "win32api.h"
#ifdef TTY_GRAPHICS
#include "wintty.h"
#endif
#ifdef WIN32

/*
 * The following WIN32 API routines are used in this file.
 *
 * GetDiskFreeSpace
 * GetVolumeInformation
 * GetUserName
 * FindFirstFile
 * FindNextFile
 * FindClose
 *
 */

/* globals required within here */
HANDLE ffhandle = (HANDLE) 0;
WIN32_FIND_DATA ffd;
typedef HWND(WINAPI *GETCONSOLEWINDOW)();
static HWND GetConsoleHandle(void);
static HWND GetConsoleHwnd(void);

/* The function pointer nt_kbhit contains a kbhit() equivalent
 * which varies depending on which window port is active.
 * For the tty port it is tty_kbhit() [from nttty.c]
 * For the win32 port it is win32_kbhit() [from winmain.c]
 * It is initialized to point to def_kbhit [in here] for safety.
 */

int def_kbhit(void);
int (*nt_kbhit)() = def_kbhit;

char
switchar()
{
    /* Could not locate a WIN32 API call for this- MJA */
    return '-';
}

long
freediskspace(path)
char *path;
{
    char tmppath[4];
    DWORD SectorsPerCluster = 0;
    DWORD BytesPerSector = 0;
    DWORD FreeClusters = 0;
    DWORD TotalClusters = 0;

    tmppath[0] = *path;
    tmppath[1] = ':';
    tmppath[2] = '\\';
    tmppath[3] = '\0';
    GetDiskFreeSpace(tmppath, &SectorsPerCluster, &BytesPerSector,
                     &FreeClusters, &TotalClusters);
    return (long) (SectorsPerCluster * BytesPerSector * FreeClusters);
}

/*
 * Functions to get filenames using wildcards
 */
int
findfirst(path)
char *path;
{
    if (ffhandle) {
        FindClose(ffhandle);
        ffhandle = (HANDLE) 0;
    }
    ffhandle = FindFirstFile(path, &ffd);
    return (ffhandle == INVALID_HANDLE_VALUE) ? 0 : 1;
}

int
findnext()
{
    return FindNextFile(ffhandle, &ffd) ? 1 : 0;
}

char *
foundfile_buffer()
{
    return &ffd.cFileName[0];
}

long
filesize(file)
char *file;
{
    if (findfirst(file)) {
        return ((long) ffd.nFileSizeLow);
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
    if ((ptr = index(str, ':')) != (char *) 0) {
        drive = toupper((uchar) *(ptr - 1));
        _chdrive((drive - 'A') + 1);
    }
}

static int
max_filename()
{
    DWORD maxflen;
    int status = 0;

    status = GetVolumeInformation((LPTSTR) 0, (LPTSTR) 0, 0, (LPDWORD) 0,
                                  &maxflen, (LPDWORD) 0, (LPTSTR) 0, 0);
    if (status)
        return maxflen;
    else
        return 0;
}

int
def_kbhit()
{
    return 0;
}

/*
 * Strip out troublesome file system characters.
 */

void nt_regularize(s) /* normalize file name */
register char *s;
{
    register unsigned char *lp;

    for (lp = s; *lp; lp++)
        if (*lp == '?' || *lp == '"' || *lp == '\\' || *lp == '/'
            || *lp == '>' || *lp == '<' || *lp == '*' || *lp == '|'
            || *lp == ':' || (*lp > 127))
            *lp = '_';
}

/*
 * This is used in nhlan.c to implement some of the LAN_FEATURES.
 */
char *
get_username(lan_username_size)
int *lan_username_size;
{
    static TCHAR username_buffer[BUFSZ];
    unsigned int status;
    DWORD i = BUFSZ - 1;

    /* i gets updated with actual size */
    status = GetUserName(username_buffer, &i);
    if (status)
        username_buffer[i] = '\0';
    else
        Strcpy(username_buffer, "NetHack");
    if (lan_username_size)
        *lan_username_size = strlen(username_buffer);
    return username_buffer;
}

#if 0
char *getxxx()
{
char     szFullPath[MAX_PATH] = "";
HMODULE  hInst = NULL;  	/* NULL gets the filename of this module */

GetModuleFileName(hInst, szFullPath, sizeof(szFullPath));
return &szFullPath[0];
}
#endif

/* fatal error */
/*VARARGS1*/
void error
VA_DECL(const char *, s)
{
    char buf[BUFSZ];
    VA_START(s);
    VA_INIT(s, const char *);
    /* error() may get called before tty is initialized */
    if (iflags.window_inited)
        end_screen();
    if (windowprocs.name != NULL && !strncmpi(windowprocs.name, "tty", 3)) {
        buf[0] = '\n';
        (void) vsprintf(&buf[1], s, VA_ARGS);
        Strcat(buf, "\n");
        msmsg(buf);
    } else {
        (void) vsprintf(buf, s, VA_ARGS);
        Strcat(buf, "\n");
        raw_printf(buf);
    }
    VA_END();
    exit(EXIT_FAILURE);
}

void
Delay(int ms)
{
    (void) Sleep(ms);
}

#ifdef TTY_GRAPHICS
extern void NDECL(backsp);
#else
void
backsp()
{
}
#endif

void
win32_abort()
{
    if (wizard) {
        int c, ci, ct;

        if (!iflags.window_inited)
            c = 'n';
        ct = 0;
        msmsg("Execute debug breakpoint wizard?");
        while ((ci = nhgetch()) != '\n') {
            if (ct > 0) {
                backsp(); /* \b is visible on NT */
                (void) putchar(' ');
                backsp();
                ct = 0;
                c = 'n';
            }
            if (ci == 'y' || ci == 'n' || ci == 'Y' || ci == 'N') {
                ct = 1;
                c = ci;
                msmsg("%c", c);
            }
        }
        if (c == 'y')
            DebugBreak();
    }
    abort();
}

static char interjection_buf[INTERJECTION_TYPES][1024];
static int interjection[INTERJECTION_TYPES];

void
interject_assistance(num, interjection_type, ptr1, ptr2)
int num;
int interjection_type;
genericptr_t ptr1;
genericptr_t ptr2;
{
    switch (num) {
    case 1: {
        char *panicmsg = (char *) ptr1;
        char *datadir = (char *) ptr2;
        char *tempdir = nh_getenv("TEMP");
        interjection_type = INTERJECT_PANIC;
        interjection[INTERJECT_PANIC] = 1;
        /*
         * ptr1 = the panic message about to be delivered.
         * ptr2 = the directory prefix of the dungeon file
         *        that failed to open.
         * Check to see if datadir matches tempdir or a
         * common windows temp location. If it does, inform
         * the user that they are probably trying to run the
         * game from within their unzip utility, so the required
         * files really don't exist at the location. Instruct
         * them to unpack them first.
         */
        if (panicmsg && datadir) {
            if (!strncmpi(datadir, "C:\\WINDOWS\\TEMP", 15)
                || strstri(datadir, "TEMP")
                || (tempdir && strstri(datadir, tempdir))) {
                (void) strncpy(
                    interjection_buf[INTERJECT_PANIC],
                    "\nOne common cause of this error is attempting to "
                    "execute\n"
                    "the game by double-clicking on it while it is "
                    "displayed\n"
                    "inside an unzip utility.\n\n"
                    "You have to unzip the contents of the zip file into a\n"
                    "folder on your system, and then run \"NetHack.exe\" or "
                    "\n"
                    "\"NetHackW.exe\" from there.\n\n"
                    "If that is not the situation, you are encouraged to\n"
                    "report the error as shown above.\n\n",
                    1023);
            }
        }
    } break;
    }
}

void
interject(interjection_type)
int interjection_type;
{
    if (interjection_type >= 0 && interjection_type < INTERJECTION_TYPES)
        msmsg(interjection_buf[interjection_type]);
}

#ifdef RUNTIME_PASTEBUF_SUPPORT

void port_insert_pastebuf(buf)
char *buf;
{
    /* This implementation will utilize the windows clipboard
     * to accomplish this.
     */

    char *tmp = buf;
    HGLOBAL hglbCopy; 
    WCHAR *w, w2[2];
    int cc, rc, abytes;
    LPWSTR lpwstrCopy;
    HANDLE hresult;

    if (!buf)
        return; 
 
    cc = strlen(buf);
    /* last arg=0 means "tell me the size of the buffer that I need" */
    rc = MultiByteToWideChar(GetConsoleOutputCP(), 0, buf, -1, w2, 0);
    if (!rc) return;

    abytes = rc * sizeof(WCHAR);
    w = (WCHAR *)alloc(abytes);     
    /* Housekeeping need: +free(w) */

    rc = MultiByteToWideChar(GetConsoleOutputCP(), 0, buf, -1, w, rc);
    if (!rc) {
        free(w);
        return;
    }
    if (!OpenClipboard(NULL)) {
        free(w);
        return;
    }
    /* Housekeeping need: +CloseClipboard(), free(w) */

    EmptyClipboard(); 

    /* allocate global mem obj to hold the text */
 
    hglbCopy = GlobalAlloc(GMEM_MOVEABLE, abytes);
    if (hglbCopy == NULL) { 
        CloseClipboard(); 
        free(w);
        return;
    } 
    /* Housekeeping need: +GlobalFree(hglbCopy), CloseClipboard(), free(w) */
 
    lpwstrCopy = (LPWSTR)GlobalLock(hglbCopy);
    /* Housekeeping need: +GlobalUnlock(hglbCopy), GlobalFree(hglbCopy),
                            CloseClipboard(), free(w) */

    memcpy(lpwstrCopy, w, abytes);
    GlobalUnlock(hglbCopy);
    /* Housekeeping need: GlobalFree(hglbCopy), CloseClipboard(), free(w) */

    /* put it on the clipboard */
    hresult = SetClipboardData(CF_UNICODETEXT, hglbCopy);
    if (!hresult) {
        raw_printf("Error copying to clipboard.\n");
        GlobalFree(hglbCopy); /* only needed if clipboard didn't accept data */
    }
    /* Housekeeping need: CloseClipboard(), free(w) */
 
    CloseClipboard(); 
    free(w);
    return;
}

static HWND
GetConsoleHandle(void)
{
    HMODULE hMod = GetModuleHandle("kernel32.dll");
    GETCONSOLEWINDOW pfnGetConsoleWindow =
        (GETCONSOLEWINDOW) GetProcAddress(hMod, "GetConsoleWindow");
    if (pfnGetConsoleWindow)
        return pfnGetConsoleWindow();
    else
        return GetConsoleHwnd();
}

static HWND
GetConsoleHwnd(void)
{
    int iterations = 0;
    HWND hwndFound = 0;
    char OldTitle[1024], NewTitle[1024], TestTitle[1024];

    /* Get current window title */
    GetConsoleTitle(OldTitle, sizeof OldTitle);

    (void) sprintf(NewTitle, "NETHACK%d/%d", GetTickCount(),
                   GetCurrentProcessId());
    SetConsoleTitle(NewTitle);

    GetConsoleTitle(TestTitle, sizeof TestTitle);
    while (strcmp(TestTitle, NewTitle) != 0) {
        iterations++;
        /* sleep(0); */
        GetConsoleTitle(TestTitle, sizeof TestTitle);
    }
    hwndFound = FindWindow(NULL, NewTitle);
    SetConsoleTitle(OldTitle);
    /*       printf("%d iterations\n", iterations); */
    return hwndFound;
}

#endif

#ifdef RUNTIME_PORT_ID
/*
 * _M_IX86 is Defined for x86 processors. This is not defined for x64
 * processors.
 * _M_X64  is Defined for x64 processors.
 * _M_IA64 is Defined for Itanium Processor Family 64-bit processors.
 * _WIN64  is Defined for applications for Win64.
 */
#ifndef _M_IX86
#ifdef _M_X64
#define TARGET_PORT "x64"
#endif
#ifdef _M_IA64
#define TARGET_PORT "IA64"
#endif
#endif

#ifndef TARGET_PORT
#define TARGET_PORT "x86"
#endif

char *
get_port_id(buf)
char *buf;
{
    Strcpy(buf, TARGET_PORT);
    return buf;
}
#endif /* RUNTIME_PORT_ID */

/* ntassert_failed is called when an ntassert's condition is false */
void ntassert_failed(const char * exp, const char * file, int line)
{
    char message[128];
    _snprintf(message, sizeof(message),
                "NHASSERT(%s) in '%s' at line %d\n", exp, file, line);

    if (IsDebuggerPresent()) {
        OutputDebugStringA(message);
        DebugBreak();
    }

    // strip off the newline
    message[strlen(message) - 1] = '\0';
    error(message);
}

/* nethack_enter_winnt() is the first thing called from main */
void nethack_enter_winnt()
{
#ifdef TTY_GRAPHICS
    nethack_enter_nttty();
#endif
}
#endif /* WIN32 */

/*winnt.c*/

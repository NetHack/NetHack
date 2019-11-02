/* NetHack 3.6	winnt.c	$NHDT-Date: 1524321419 2018/04/21 14:36:59 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.30 $ */
/* Copyright (c) NetHack PC Development Team 1993, 1994 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  WIN32 system functions.
 * 
 *  Included in both console and window based clients on the windows platform.
 *
 *  Initial Creation: Michael Allison - January 31/93
 *
 */

#include "win10.h"
#include "winos.h"

#define NEED_VARARGS
#include "hack.h"
#include <dos.h>
#ifndef __BORLANDC__
#include <direct.h>
#endif
#include <ctype.h>
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

/* runtime cursor display control switch */
boolean win32_cursorblink;

/* globals required within here */
HANDLE ffhandle = (HANDLE) 0;
WIN32_FIND_DATA ffd;
extern int GUILaunched;
boolean getreturn_enabled;
int redirect_stdout;

typedef HWND(WINAPI *GETCONSOLEWINDOW)();
static HWND GetConsoleHandle(void);
static HWND GetConsoleHwnd(void);
#if !defined(TTY_GRAPHICS)
extern void NDECL(backsp);
#endif
int NDECL(windows_console_custom_nhgetch);
extern void NDECL(safe_routines);

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
    DWORD i = BUFSZ - 1;
    BOOL allowUserName = TRUE;

    Strcpy(username_buffer, "NetHack");

#ifndef WIN32CON
    /* Our privacy policy for the windows store version of nethack makes
     * a promise about not collecting any personally identifiable information.
     * Do not allow getting user name if we being run from windows store
     * version of nethack.  In 3.7, we should remove use of username.
     */
    allowUserName = !win10_is_desktop_bridge_application();
#endif

    if (allowUserName) {
        /* i gets updated with actual size */
        if (GetUserName(username_buffer, &i))
            username_buffer[i] = '\0';
    }

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

extern void NDECL(mswin_raw_print_flush);
extern void FDECL(mswin_raw_print, (const char *));

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
    if (WINDOWPORT("tty")) {
        buf[0] = '\n';
        (void) vsprintf(&buf[1], s, VA_ARGS);
        Strcat(buf, "\n");
        msmsg(buf);
    } else {
        (void) vsprintf(buf, s, VA_ARGS);
        Strcat(buf, "\n");
        raw_printf(buf);
    }
    if (windowprocs.win_raw_print == mswin_raw_print)
        mswin_raw_print_flush();
    VA_END();
    exit(EXIT_FAILURE);
}

void
Delay(int ms)
{
    (void) Sleep(ms);
}

void
win32_abort()
{
    int c;

    if (WINDOWPORT("mswin") || WINDOWPORT("tty")) {
        if (iflags.window_inited)
            exit_nhwindows((char *) 0);
        iflags.window_inited = FALSE;
    }
    if (!WINDOWPORT("mswin") && !WINDOWPORT("safe-startup"))
        safe_routines();
    if (wizard) {
        raw_print("Execute debug breakpoint wizard?");
        if ((c = nhgetch()) == 'y' || c == 'Y')
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

/* nhassert_failed is called when an nhassert's condition is false */
void nhassert_failed(const char * exp, const char * file, int line)
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

void
nethack_exit(code)
int code;
{
    /* Only if we started from the GUI, not the command prompt,
     * we need to get one last return, so the score board does
     * not vanish instantly after being created.
     * GUILaunched is defined and set in nttty.c.
     */


    if (!GUILaunched) {
        windowprocs = *get_safe_procs(1);
        /* use our custom version which works
           a little cleaner than the stdio one */
        windowprocs.win_nhgetch = windows_console_custom_nhgetch;
    }
    if (getreturn_enabled)
        wait_synch();
    exit(code);
}

#undef kbhit
#include <conio.h>

int
windows_console_custom_nhgetch(VOID_ARGS)
{
    return _getch();
}


void
getreturn(str)
const char *str;
{
    static boolean in_getreturn = FALSE;
    char buf[BUFSZ];

    if (in_getreturn || !getreturn_enabled)
        return;
    in_getreturn = TRUE;
    Sprintf(buf,"Hit <Enter> %s.", str);
    raw_print(buf);
    wait_synch();
    in_getreturn = FALSE;
    return;
}

/* nethack_enter_winnt() is called from main immediately after
   initializing the window port */
void nethack_enter_winnt()
{
	if (WINDOWPORT("tty"))
		nethack_enter_nttty();
}

/* CP437 to Unicode mapping according to the Unicode Consortium */
const WCHAR cp437[256] = {
    0x0020, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
    0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
    0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
    0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2302,
    0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
    0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
    0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
    0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
    0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
    0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
    0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f,
    0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b,
    0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
    0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
    0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
    0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248,
    0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0
};

WCHAR *
winos_ascii_to_wide_str(const unsigned char * src, WCHAR * dst, size_t dstLength)
{
    size_t i = 0;
    while(i < dstLength - 1 && src[i] != 0)
        dst[i++] = cp437[src[i]];
    dst[i] = 0;
    return dst;
}

WCHAR
winos_ascii_to_wide(const unsigned char c)
{
    return cp437[c];
}

BOOL winos_font_support_cp437(HFONT hFont)
{
    BOOL allFound = FALSE;
    HDC hdc = GetDC(NULL);
    HFONT oldFont = SelectObject(hdc, hFont);

    DWORD size = GetFontUnicodeRanges(hdc, NULL);
    GLYPHSET *glyphSet = (GLYPHSET *) malloc(size);

    if (glyphSet != NULL) {
        GetFontUnicodeRanges(hdc, glyphSet);

        allFound = TRUE;
        for (int i = 0; i < 256 && allFound; i++) {
            WCHAR wc = cp437[i];
            BOOL found = FALSE;
            for (DWORD j = 0; j < glyphSet->cRanges && !found; j++) {
                WCHAR first = glyphSet->ranges[j].wcLow;
                WCHAR last = first + glyphSet->ranges[j].cGlyphs - 1;

                if (wc >= first && wc <= last)
                    found = TRUE;
            }
            if (!found)
                allFound = FALSE;
        }

        free(glyphSet);
    }

    SelectObject(hdc, oldFont);
    ReleaseDC(NULL, hdc);

    return allFound;
}

int
windows_early_options(window_opt)
const char *window_opt;
{
    /*
     * If you return 2, the game will exit before it begins.
     * Return 1, to say the option parsed okay.
     * Return 0, to say the option was bad.
     */

    if (match_optname(window_opt, "cursorblink", 5, FALSE)) {
        win32_cursorblink = TRUE;
        return 1;
    } else {
        raw_printf(
            "-%swindows:cursorblink is the only supported option.\n");
    }
    return 0;
}

/*
 * Add a backslash to any name not ending in /, \ or :	 There must
 * be room for the \
 */
void
append_slash(name)
char *name;
{
    char *ptr;

    if (!*name)
        return;
    ptr = name + (strlen(name) - 1);
    if (*ptr != '\\' && *ptr != '/' && *ptr != ':') {
        *++ptr = '\\';
        *++ptr = '\0';
    }
    return;
}

#include <bcrypt.h>     /* Windows Crypto Next Gen (CNG) */

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0
#endif
#ifndef STATUS_NOT_FOUND
#define STATUS_NOT_FOUND 0xC0000225
#endif
#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL 0xC0000001
#endif

unsigned long
sys_random_seed(VOID_ARGS)
{
    unsigned long ourseed = 0UL;
    BCRYPT_ALG_HANDLE hRa = (BCRYPT_ALG_HANDLE) 0;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    boolean Plan_B = TRUE;

    status = BCryptOpenAlgorithmProvider(&hRa, BCRYPT_RNG_ALGORITHM,
                                         (LPCWSTR) 0, 0);
    if (hRa && status == STATUS_SUCCESS) {
        status = BCryptGenRandom(hRa, (PUCHAR) &ourseed,
                                 (ULONG) sizeof ourseed, 0);
        if (status == STATUS_SUCCESS) {
            BCryptCloseAlgorithmProvider(hRa,0);
            has_strong_rngseed = TRUE;
            Plan_B = FALSE;
        }
    }

    if (Plan_B) {
        time_t datetime = 0;
        const char *emsg;

        if (status == STATUS_NOT_FOUND)
            emsg = "BCRYPT_RNG_ALGORITHM not avail, falling back";
        else
            emsg = "Other failure than algorithm not avail";
        paniclog("sys_random_seed", emsg); /* leaves clue, doesn't exit */
        (void) time(&datetime);
        ourseed = (unsigned long) datetime;
    }
    return ourseed;
}

#endif /* WIN32 */

/*winnt.c*/

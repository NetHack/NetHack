/* NetHack 3.7	windsys.c	$NHDT-Date: 1710949760 2024/03/20 15:49:20 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.95 $ */
/* Copyright (c) NetHack PC Development Team 1993, 1994 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  WIN32 system functions.
 *
 *  Included in both console-based and window-based clients on the windows platform.
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
#include <inttypes.h>

#ifdef WIN32
#include <VersionHelpers.h>

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
extern boolean getreturn_enabled;
int redirect_stdout;

#ifdef WIN32CON
typedef HWND(WINAPI *GETCONSOLEWINDOW)(void);
#ifdef WIN32CON
static HWND GetConsoleHandle(void);
static HWND GetConsoleHwnd(void);
#endif
#if !defined(TTY_GRAPHICS)
extern void backsp(void);
#endif
int windows_console_custom_nhgetch(void);
extern void safe_routines(void);
int windows_early_options(const char *window_opt);
unsigned long sys_random_seed(void);
#if 0
static int max_filename(void);
#endif


/* The function pointer nt_kbhit contains a kbhit() equivalent
 * which varies depending on which window port is active.
 * For the tty port it is tty_kbhit() [from consoletty.c]
 * For the win32 port it is win32_kbhit() [from winmain.c]
 * It is initialized to point to def_kbhit [in here] for safety.
 */

int def_kbhit(void);
int (*nt_kbhit)(void) = def_kbhit;
#endif /* WIN32CON */

#ifndef WIN32CON
/* this is used as a printf() replacement when the window
 * system isn't initialized yet
 */
void msmsg
VA_DECL(const char *, fmt)
{
    VA_START(fmt);
    VA_INIT(fmt, const char *);
    VA_END();
    return;
}
#endif

char
switchar(void)
{
    /* Could not locate a WIN32 API call for this- MJA */
    return '-';
}

long
freediskspace(char *path)
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
findfirst(char *path)
{
    if (ffhandle) {
        FindClose(ffhandle);
        ffhandle = (HANDLE) 0;
    }
    ffhandle = FindFirstFile(path, &ffd);
    return (ffhandle == INVALID_HANDLE_VALUE) ? 0 : 1;
}

int
findnext(void)
{
    return FindNextFile(ffhandle, &ffd) ? 1 : 0;
}

char *
foundfile_buffer(void)
{
    return &ffd.cFileName[0];
}

long
filesize(char *file)
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
chdrive(char *str)
{
    char *ptr;
    char drive;
    if ((ptr = strchr(str, ':')) != (char *) 0) {
        drive = toupper((uchar) *(ptr - 1));
        (void) _chdrive((drive - 'A') + 1);
    }
}

#if 0
static int
max_filename(void)
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
#endif

int
def_kbhit(void)
{
    return 0;
}

/*
 * Strip out troublesome file system characters.
 */

void nt_regularize(char* s) /* normalize file name */
{
    unsigned char *lp;

    for (lp = (unsigned char *) s; *lp; lp++)
        if (*lp == '?' || *lp == '"' || *lp == '\\' || *lp == '/'
            || *lp == '>' || *lp == '<' || *lp == '*' || *lp == '|'
            || *lp == ':' || (*lp > 127))
            *lp = '_';
}

#if 0
char *getxxx(void)
{
char     szFullPath[MAX_PATH] = "";
HMODULE  hInst = NULL;  /* NULL gets the filename of this module */

GetModuleFileName(hInst, szFullPath, sizeof(szFullPath));
return &szFullPath[0];
}
#endif

#ifdef MSWIN_GRAPHICS
extern void mswin_raw_print_flush(void);
extern void mswin_raw_print(const char *);
#endif

DISABLE_WARNING_FORMAT_NONLITERAL

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
    if (WINDOWPORT(tty)) {
        buf[0] = '\n';
        (void) vsnprintf(&buf[1], sizeof buf - (1 + sizeof "\n"), s, VA_ARGS);
        Strcat(buf, "\n");
        msmsg(buf);
    } else {
        (void) vsnprintf(buf, sizeof buf - sizeof "\n", s, VA_ARGS);
        Strcat(buf, "\n");
        raw_printf(buf);
    }
#ifdef MSWIN_GRAPHICS
    if (windowprocs.win_raw_print == mswin_raw_print)
        mswin_raw_print_flush();
#endif
    VA_END();
    exit(EXIT_FAILURE);
}

RESTORE_WARNING_FORMAT_NONLITERAL

void
Delay(int ms)
{
    (void) Sleep(ms);
}

void
win32_abort(void)
{
    int c;

    if (WINDOWPORT(mswin) || WINDOWPORT(tty)) {
        if (iflags.window_inited)
            exit_nhwindows((char *) 0);
        iflags.window_inited = FALSE;
    }
#ifdef WIN32CON
    if (!WINDOWPORT(mswin) && !WINDOWPORT(safestartup))
        safe_routines();
#endif
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
interject_assistance(int num, int interjection_type, genericptr_t ptr1, genericptr_t ptr2)
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
interject(int interjection_type)
{
    if (interjection_type >= 0 && interjection_type < INTERJECTION_TYPES)
        msmsg("%s", interjection_buf[interjection_type]);
}

#ifdef RUNTIME_PASTEBUF_SUPPORT

void port_insert_pastebuf(char *buf)
{
    /* This implementation will utilize the windows clipboard
     * to accomplish this.
     */

    HGLOBAL hglbCopy;
    WCHAR *w, w2[2];
    /* int cc; */
    int rc, abytes;
    LPWSTR lpwstrCopy;
    HANDLE hresult;

    if (!buf)
        return;

    /* cc = strlen(buf); */
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

    if (lpwstrCopy) {
        memcpy(lpwstrCopy, w, abytes);
        GlobalUnlock(hglbCopy);
        /* Housekeeping need: GlobalFree(hglbCopy), CloseClipboard(), free(w)
         */

        /* put it on the clipboard */
        hresult = SetClipboardData(CF_UNICODETEXT, hglbCopy);
        if (!hresult) {
            raw_printf("Error copying to clipboard.\n");
            GlobalFree(
                hglbCopy); /* only needed if clipboard didn't accept data */
        }
        /* Housekeeping need: CloseClipboard(), free(w) */
    }
    CloseClipboard();
    free(w);
    return;
}

#ifdef WIN32CON
static HWND
GetConsoleHandle(void)
{
    HMODULE hMod = GetModuleHandle("kernel32.dll");

    if (hMod) {
        GETCONSOLEWINDOW pfnGetConsoleWindow =
            (GETCONSOLEWINDOW) GetProcAddress(hMod, "GetConsoleWindow");
        if (pfnGetConsoleWindow)
            return pfnGetConsoleWindow();
    }
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

    (void) sprintf(NewTitle, "NETHACK%lld/%ld", GetTickCount64(),
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
#endif /* WIN32CON */
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
get_port_id(char *buf)
{
    Strcpy(buf, TARGET_PORT);
    return buf;
}
#endif /* RUNTIME_PORT_ID */

void
nethack_exit(int code)
{
    /* Only if we started from the GUI, not the command prompt,
     * we need to get one last return, so the score board does
     * not vanish instantly after being created.
     * GUILaunched is defined and set in consoletty.c.
     */


#ifdef WIN32CON
    if (!GUILaunched) {
        windowprocs = *get_safe_procs(1);
        /* use our custom version which works
           a little cleaner than the stdio one */
        windowprocs.win_nhgetch = windows_console_custom_nhgetch;
    } else
#endif
    if (getreturn_enabled) {
        raw_print("\n");
        if (iflags.window_inited)
            wait_synch();
    }
    exit(code);
}

#ifdef WIN32CON
#undef kbhit
#include <conio.h>

int
windows_console_custom_nhgetch(void)
{
    return _getch();
}

extern int windows_console_custom_nhgetch(void);

void
getreturn(const char *str)
{
    static boolean in_getreturn = FALSE;
    char buf[BUFSZ];

    if (in_getreturn || !getreturn_enabled)
        return;
    in_getreturn = TRUE;
    Sprintf(buf,"Hit <Enter> %s.", str);
    raw_print(buf);
    if (WINDOWPORT(tty))
        windows_console_custom_nhgetch();
    else
        wait_synch();
    in_getreturn = FALSE;
    return;
}
#endif

/* nethack_enter_windows() is called from main immediately after
   initializing the window port */
void nethack_enter_windows(void)
{
#ifdef WIN32CON
    if (WINDOWPORT(tty))
        nethack_enter_consoletty();
#endif
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
    while(i < dstLength - 1 && src[i] != 0) {
        dst[i] = cp437[src[i]];
        i++;
    }
    dst[i] = 0;
    return dst;
}

void
winos_ascii_to_wide(WCHAR dst[3], UINT32 c)
{
#ifdef ENHANCED_SYMBOLS
    if (SYMHANDLING(H_UTF8)) {
        if (c <= 0xFFFF) {
            dst[0] = (WCHAR) c;
            dst[1] = L'\0';
        } else {
            /* UTF-16 surrogate pair */
            dst[0] = (WCHAR) ((c >> 10) + 0xD7C0);
            dst[1] = (WCHAR) ((c & 0x3FF) + 0xDC00);
            dst[2] = L'\0';
        }
    } else
#endif
    {
        dst[0] = cp437[c];
        dst[1] = L'\0';
    }
}

BOOL winos_font_support_cp437(HFONT hFont)
{
    BOOL allFound = FALSE;
    HDC hdc = GetDC(NULL);
    HFONT oldFont = SelectObject(hdc, hFont);


    DWORD size = (size_t) GetFontUnicodeRanges(hdc, NULL);
    if (size) {
        GLYPHSET *glyphSet = (GLYPHSET *) malloc((size_t) size);
        if (glyphSet != NULL) {
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 6386 )
#endif
            size = GetFontUnicodeRanges(hdc, glyphSet);
#ifdef _MSC_VER
#pragma warning( pop )
#endif
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
    }
    SelectObject(hdc, oldFont);
    ReleaseDC(NULL, hdc);

    return allFound;
}

int
windows_early_options(const char *window_opt)
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
            "-%s windows:cursorblink is the only supported option.\n",
            window_opt);
    }
    return 0;
}

/*
 * Add a backslash to any name not ending in /, \ or : There must
 * be room for the \
 */
void
append_slash(char *name)
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
sys_random_seed(void)
{
    unsigned long ourseed = 0UL;
    BCRYPT_ALG_HANDLE hRa = (BCRYPT_ALG_HANDLE) 0;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    boolean Plan_B = TRUE;

    status = BCryptOpenAlgorithmProvider(&hRa, BCRYPT_RNG_ALGORITHM,
                                         (LPCWSTR) 0, 0);
    if (hRa && status == (NTSTATUS) STATUS_SUCCESS) {
        status = BCryptGenRandom(hRa, (PUCHAR) &ourseed,
                                 (ULONG) sizeof ourseed, 0);
        if (status == (NTSTATUS) STATUS_SUCCESS) {
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

/* nt_assert_failed is called when an nhassert's condition is false */
void
nt_assert_failed(const char *expression, const char *filepath, int line)
{
    const char * filename;

    filename = strrchr(filepath, '\\');
    filename = (filename == NULL ? filepath : filename + 1);

    if (IsDebuggerPresent()) {
        char message[BUFSIZ];
        snprintf(message, sizeof(message),
            "nhassert(%s) failed in file '%s' at line %d",
            expression, filename, line);
        OutputDebugStringA(message);
        DebugBreak();
    }

    /* get file name from path */
    impossible("nhassert(%s) failed in file '%s' at line %d",
                expression, filename, line);
}

/* Windows helpers for CRASHREPORT etc */
#ifdef CRASHREPORT
struct CRctxt {
    BCRYPT_ALG_HANDLE bah;
    BCRYPT_HASH_HANDLE bhh;
    PBYTE pbhashobj;
    DWORD cbhashobj; /* temp */
    DWORD cbhash;    /* hash length */
    DWORD cbdata;    /* temp */
    PBYTE pbhash;    /* binary hash */
    NTSTATUS st;
} ctxp_ = { NULL, NULL, NULL, 0, 0, 0, NULL, 0 };
struct CRctxt *ctxp = &ctxp_;    // XXX should this now be in gc.* ?

#define win32err(fn) errname = fn; goto error

int
win32_cr_helper(char cmd, struct CRctxt *ctxp, void *p, int d){
    char *errname = "unknown";
    switch (cmd) {
    default:
        /* Not panic - we don't want to upgrade an impossible to a
         * panic due to a bug in the CRASHREPORT code. */
        impossible("win_cr_helper bad cmd %d", cmd);
        return 1;
    case 'D': {
        char *bidstr = (char *) p;
        wchar_t lbidstr[40];        // sizeof(bid), but const
        swprintf_s(lbidstr, 40, L"%S", bidstr);
            // XXX TODO: need something that will allow copy of just the bid
        return MessageBoxW(NULL, lbidstr, L"bidshow", MB_SETFOREGROUND);
    }
        break;
    case 'i': /* HASH_INIT(ctxp) */
        if (!IsWindowsVistaOrGreater())
            return 1; // CNG not available.
        ctxp->bah = NULL;
        ctxp->bhh = NULL;
        ctxp->pbhashobj = NULL;
        ctxp->cbhashobj = 0;
        ctxp->cbhash = 0;
        ctxp->cbdata = 0;
        ctxp->pbhash = NULL;
        ctxp->st = 0;
        // win32err("test");        // TESTING - FAKE AN ERROR
        if (0 > (ctxp->st = BCryptOpenAlgorithmProvider(
                     &ctxp->bah, BCRYPT_MD4_ALGORITHM, NULL, 0))) {
            win32err("BCryptOpenAlgorithmProvider");
        };
        if (0 > (ctxp->st =
                     BCryptGetProperty(ctxp->bah, BCRYPT_OBJECT_LENGTH,
                                       (unsigned char *) &ctxp->cbhashobj,
                                       sizeof(DWORD), &ctxp->cbdata, 0))) {
            win32err("BCryptGetProperty1");
        };
        if (0
            == (ctxp->pbhashobj =
                    HeapAlloc(GetProcessHeap(), 0, ctxp->cbhashobj))) {
            win32err("HeapAlloc1");
        };
        if (0 > (ctxp->st = BCryptGetProperty(
                     ctxp->bah, BCRYPT_HASH_LENGTH, (PBYTE) &ctxp->cbhash,
                     sizeof(DWORD), &ctxp->cbdata, 0))) {
            win32err("BCryptGetProperty2");
        }
        if (0
            == (ctxp->pbhash =
                    HeapAlloc(GetProcessHeap(), 0, ctxp->cbhash))) {

            win32err("HeapAlloc2\n");
        }
        if (0 > BCryptCreateHash(ctxp->bah, &ctxp->bhh, ctxp->pbhashobj,
                                 ctxp->cbhashobj, NULL, 0, 0)) {
            win32err("BCryptCreateHash");
        }
        break;
    case 'u':        /* HASH_UPDATE(ctxp, ptr, len) */
        if (0 > (ctxp->st = BCryptHashData(ctxp->bhh, p, d, 0))) {
            win32err("BCryptHashData");
        }
        break;
    case 'f':        /* HASH_FINISH(ctxp) */
        if (0 > BCryptFinishHash(ctxp->bhh, ctxp->pbhash, ctxp->cbhash, 0)) {
            win32err("BCryptFinishHash");
        }
        break;
    case 'c':        /* HASH_CLEANUP(ctxp) */
        if (ctxp->bah) {
            BCryptCloseAlgorithmProvider(ctxp->bah, 0);
        }
        if (ctxp->bhh) {
            BCryptDestroyHash(ctxp->bhh);
        }
        if (ctxp->pbhashobj) {
            HeapFree(GetProcessHeap(), 0, ctxp->pbhashobj);
        }
        if (ctxp->pbhash) {
            HeapFree(GetProcessHeap(), 0, ctxp->pbhash);
        }
        break;
    case 's':        /* HASH_RESULT_SIZE(ctxp) */
        return ctxp->cbhash;
    case 'r':        /* HASH_RESULT(ctxp, resp) */
        *(unsigned char **)p = ctxp->pbhash;
        break;
    case 'b':        /* HASH_BINFILE(NULL,&binfile,0) */
            // XXX This buffer should be allocated, not static (and freed in
            //   HASH_CLEANUP).
            // NB: assumes !longPathAware in manifest (Win10+)
        {
            static char binfile[MAX_PATH];
            DWORD rv = GetModuleFileNameA(NULL, binfile, sizeof(binfile));
            if (rv == 0 || rv == sizeof(binfile))
                return 1;
#if (NH_DEVEL_STATUS == NH_STATUS_BETA)
            printf("FILE '%s'\n", binfile);
#endif
            *(unsigned char **) p = (unsigned char *) binfile;
            return 0;
        }
    }
    return 0;        /* ok */
error:
    raw_printf("WIN32 function %s failed: status=%" PRIx64 "\n",
            errname, (uint64)ctxp->st);
    return 1;        /* fail */
}
#undef win32err


#include <shellapi.h>
#define MAX_SYM_SIZE 100
#ifdef __GNUC__
        // gcc can't generate .pdb files.  llvm can almost do it.
        // For these platforms, use github/ianlancetaylor/libbacktrace.
// XXX this doesn't work yet - we get correct addresses but no symbol info
// XXX so still needs cleanup
// XXX no mark (overflow held to last valid segment) handling yet
// XXX libbacktrace isn't available by default, so don't try until it works
//#define USE_BACKTRACE
#ifdef USE_BACKTRACE
#include <backtrace.h>

struct userstate {
    int error_count;
    int good_count;
    char *out;
    int outsize;
    int maxframes;
} userstate;

//backtrace_full_callback
static int
btfcb_fn(void *us0, uintptr_t pc, const char *filename,
                int lineno, const char *fnname){
    struct userstate *us = us0;
        //XXX generate a stack frame line
printf("C: pc=%llx f=%s line=%d fn=%s\n",pc,filename,lineno,fnname);
    us->good_count++;
    return 0;
}

//backtrace_error_callback
static void
btecb_fn(void *us0, const char *msg, int errnum){
    struct userstate *us = us0;
    us->error_count++;
    if(errnum < 0){
printf("E1: M=%s e=%d\n",msg,errnum);
        // XXX save error message
    } else {
printf("E2: M=%s e=%d\n",msg,errnum);
        // errnum is an errno
    //XXX save error message with strerror
    }
}
#endif

int
win32_cr_gettrace(int maxframes, char *out, int outsize){
#ifdef USE_BACKTRACE
    userstate.error_count = 0;
    userstate.good_count = 0;
    userstate.out = out;
    userstate.outsize = outsize;
    userstate.maxframes = maxframes;
    static char binfile[MAX_PATH];// assumes !longPathAware in manifest (Win10+)
        DWORD rv = GetModuleFileNameA(NULL, binfile, sizeof(binfile));
    struct backtrace_state *state
        = backtrace_create_state(binfile, 0, btecb_fn, &userstate);
    if(!state) return userstate.error_count + userstate.good_count;
    rv=backtrace_full(state, 0, btfcb_fn, btecb_fn, &userstate);
printf("rv=%d\n",rv);
    // XXX rv not checked
    // XXX this API leaks its memory; there is no free function
    return userstate.error_count + userstate.good_count;
#else
    return 0;
#endif
}
#else
// Use win32 API with Visual Studio (and probably MSVC).
#include <dbghelp.h>

int
win32_cr_gettrace(int maxframes, char *out, int outsize){
    char *mark = out;
    void *frames[200];  // XXX why does VS fail var array?  wrong C std?
    int x;
    int tmp;
#define BSIZE (MAX_SYM_SIZE+50)
    char buf[BSIZE];
    HANDLE me = GetCurrentProcess();
    SetLastError(0);
        // XXX may need to pass the binary's dir
        //XXX check for different flags
    if(!SymInitialize(me, NULL, TRUE)){
        snprintf(buf, BSIZE, "no stack trace: SymInitialize: %d\n",
                (int) GetLastError());
        return 1;
    }
    int fcount = CaptureStackBackTrace(0, maxframes,frames,NULL);
    if(!fcount)goto finish;
    char symbol_info_space[sizeof(SYMBOL_INFO)+MAX_SYM_SIZE];
    SYMBOL_INFO *si = (SYMBOL_INFO *)symbol_info_space;
    si->MaxNameLen = MAX_SYM_SIZE;
    si->SizeOfStruct = sizeof(SYMBOL_INFO);

    for(x=0;x<fcount;x++){
        DWORD64 disp64 =0 ;
        DWORD64 adr = (DWORD64)(frames[x]);
        if(!adr) break;
        if(SymFromAddr(me, adr, &disp64, si)){
//printf("A %x\n",GetLastError());fflush(stdout);
            tmp = snprintf(buf, BSIZE, "%d %p %s+%lld\n",
                        x, frames[x], &si->Name[0], (long long int)disp64);
            if(swr_add_uricoded(buf, &out, &outsize, mark))
                goto finish;

#if 1
// XXX does this block do anything useful?
            DWORD disp = (DWORD) disp64;
            IMAGEHLP_LINE ihl;
            ihl.SizeOfStruct = sizeof(IMAGEHLP_LINE);
            if (SymGetLineFromAddr(me, adr, &disp, &ihl)) {
                printf("L=%d\n", (int) ihl.LineNumber);
            } else {
// 7e/1e7 - no info.  May need to call SymLoadModule if we need those addrs
// BUT probably system code, so we don't care - experiment
//                printf("SGLFA failed: $%08x\n", GetLastError());
            }
//XXXnow format the line
#endif
        } else {
            // Error 487 (invalid address) seems to mean
            // "I can't find any info for this address".
            tmp = snprintf(buf, BSIZE, "%d %p (error %d)\n",
                        x, frames[x], (int) GetLastError());
            if(swr_add_uricoded(buf, &out, &outsize, mark))
                goto finish;
        }
        if(tmp < 0 || tmp >= outsize){   // XXX is test now wrong?
//printf("FAIL tmp=%d\n",tmp);
                fcount = x+1;
                goto finish;
        }
        mark = out;
    }
finish:
    SymCleanup(me);
    return fcount;            // XXX if output truncated, fcount could be wrong
}
#endif

int *
win32_cr_shellexecute(const char *url){
//XXX Docs say to do this, but has so many caveats I'm going to try skipping it.
//CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    int *rv = (int*)ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    return rv;
}
#endif /* CRASHREPORT */

#endif /* WIN32 */

/*windsys.c*/

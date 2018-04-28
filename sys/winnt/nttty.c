/* NetHack 3.6	nttty.c	$NHDT-Date: 1524931557 2018/04/28 16:05:57 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.71 $ */
/* Copyright (c) NetHack PC Development Team 1993    */
/* NetHack may be freely redistributed.  See license for details. */

/* tty.c - (Windows NT) version */

/*
 * Initial Creation 				M. Allison	1993/01/31
 * Switch to low level console output routines	M. Allison	2003/10/01
 * Restrict cursor movement until input pending	M. Lehotay	2003/10/02
 * Call Unicode version of output API on NT	R. Chason       2005/10/28
 *
 */

#ifdef WIN32
#define NEED_VARARGS /* Uses ... */
#include "hack.h"
#include "wintty.h"
#include <sys\types.h>
#include <sys\stat.h>
#include "win32api.h"

/*
 * The following WIN32 Console API routines are used in this file.
 *
 * CreateFile
 * GetConsoleScreenBufferInfo
 * GetStdHandle
 * SetConsoleCursorPosition
 * SetConsoleTextAttribute
 * SetConsoleCtrlHandler
 * PeekConsoleInput
 * ReadConsoleInput
 * WriteConsoleOutputCharacter
 * FillConsoleOutputAttribute
 * GetConsoleOutputCP
 */

static BOOL FDECL(CtrlHandler, (DWORD));
static void FDECL(xputc_core, (char));
void FDECL(cmov, (int, int));
void FDECL(nocmov, (int, int));
int FDECL(process_keystroke,
          (INPUT_RECORD *, boolean *, BOOLEAN_P numberpad, int portdebug));
static void NDECL(init_ttycolor);
static void NDECL(really_move_cursor);
static void NDECL(check_and_set_font);
static boolean NDECL(check_font_widths);
static void NDECL(set_known_good_console_font);
static void NDECL(restore_original_console_font);

/* Win32 Console handles for input and output */
HANDLE hConIn;
HANDLE hConOut;

/* Win32 Screen buffer,coordinate,console I/O information */
CONSOLE_SCREEN_BUFFER_INFO csbi, origcsbi;
COORD ntcoord;
INPUT_RECORD ir;

/* Support for changing console font if existing glyph widths are too wide */
boolean console_font_changed;
CONSOLE_FONT_INFOEX original_console_font_info;
UINT original_console_code_page;

extern boolean getreturn_enabled; /* from sys/share/pcsys.c */
extern int redirect_stdout;

/* Flag for whether NetHack was launched via the GUI, not the command line.
 * The reason we care at all, is so that we can get
 * a final RETURN at the end of the game when launched from the GUI
 * to prevent the scoreboard (or panic message :-|) from vanishing
 * immediately after it is displayed, yet not bother when started
 * from the command line.
 */
int GUILaunched;
/* Flag for whether unicode is supported */
static boolean has_unicode;
static boolean init_ttycolor_completed;
#ifdef PORT_DEBUG
static boolean display_cursor_info = FALSE;
#endif
#ifdef CHANGE_COLOR
static void NDECL(adjust_palette);
static int FDECL(match_color_name, (const char *));
typedef HWND(WINAPI *GETCONSOLEWINDOW)();
static HWND GetConsoleHandle(void);
static HWND GetConsoleHwnd(void);
static boolean altered_palette;
static COLORREF UserDefinedColors[CLR_MAX];
static COLORREF NetHackColors[CLR_MAX] = {
    0x00000000, 0x00c80000, 0x0000c850, 0x00b4b432, 0x000000d2, 0x00800080,
    0x000064b4, 0x00c0c0c0, 0x00646464, 0x00f06464, 0x0000ff00, 0x00ffff00,
    0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
};
static COLORREF DefaultColors[CLR_MAX] = {
    0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080,
    0x00008080, 0x00c0c0c0, 0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00,
    0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
};
#endif
struct console_t {
    WORD background;
    WORD foreground;
    WORD attr;
    int current_nhcolor;
    int current_nhattr[ATR_INVERSE+1];
    COORD cursor;
} console = {
    0,
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED),
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED),
    NO_COLOR,
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
    {0, 0}
};
static DWORD ccount, acount;
#ifndef CLR_MAX
#define CLR_MAX 16
#endif

int ttycolors[CLR_MAX];
int ttycolors_inv[CLR_MAX];

#define MAX_OVERRIDES 256
unsigned char key_overrides[MAX_OVERRIDES];
static char nullstr[] = "";
char erase_char, kill_char;
#define DEFTEXTCOLOR ttycolors[7]

/* dynamic keystroke handling .DLL support */
typedef int(__stdcall *PROCESS_KEYSTROKE)(HANDLE, INPUT_RECORD *, boolean *,
                                          BOOLEAN_P, int);

typedef int(__stdcall *NHKBHIT)(HANDLE, INPUT_RECORD *);

typedef int(__stdcall *CHECKINPUT)(HANDLE, INPUT_RECORD *, DWORD *, BOOLEAN_P,
                                   int, int *, coord *);

typedef int(__stdcall *SOURCEWHERE)(char **);

typedef int(__stdcall *SOURCEAUTHOR)(char **);

typedef int(__stdcall *KEYHANDLERNAME)(char **, int);

HANDLE hLibrary;
PROCESS_KEYSTROKE pProcessKeystroke;
NHKBHIT pNHkbhit;
CHECKINPUT pCheckInput;
SOURCEWHERE pSourceWhere;
SOURCEAUTHOR pSourceAuthor;
KEYHANDLERNAME pKeyHandlerName;

/*
 * Called after returning from ! or ^Z
 */
void
gettty()
{
    console_font_changed = FALSE;

    check_and_set_font();

#ifndef TEXTCOLOR
    int k;
#endif
    erase_char = '\b';
    kill_char = 21; /* cntl-U */
    iflags.cbreak = TRUE;
#ifdef TEXTCOLOR
    init_ttycolor();
#else
    for (k = 0; k < CLR_MAX; ++k)
        ttycolors[k] = NO_COLOR;
#endif
}

/* reset terminal to original state */
void
settty(s)
const char *s;
{
    cmov(ttyDisplay->curx, ttyDisplay->cury);
    end_screen();
    if (s)
        raw_print(s);

    restore_original_console_font();
}

/* called by init_nhwindows() and resume_nhwindows() */
void
setftty()
{
#ifdef CHANGE_COLOR
    if (altered_palette)
        adjust_palette();
#endif
    start_screen();
    has_unicode = ((GetVersion() & 0x80000000) == 0);
}

void
tty_startup(wid, hgt)
int *wid, *hgt;
{
    int twid = origcsbi.srWindow.Right - origcsbi.srWindow.Left + 1;

    if (twid > 80)
        twid = 80;
    *wid = twid;
    *hgt = origcsbi.srWindow.Bottom - origcsbi.srWindow.Top + 1;
    set_option_mod_status("mouse_support", SET_IN_GAME);
}

void
tty_number_pad(state)
int state;
{
}

void
tty_start_screen()
{
    if (iflags.num_pad)
        tty_number_pad(1); /* make keypad send digits */
}

void
tty_end_screen()
{
    clear_screen();
    really_move_cursor();
    if (GetConsoleScreenBufferInfo(hConOut, &csbi)) {
        DWORD ccnt;
        COORD newcoord;

        newcoord.X = 0;
        newcoord.Y = 0;
        FillConsoleOutputAttribute(
            hConOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
            csbi.dwSize.X * csbi.dwSize.Y, newcoord, &ccnt);
        FillConsoleOutputCharacter(
            hConOut, ' ', csbi.dwSize.X * csbi.dwSize.Y, newcoord, &ccnt);
    }
    FlushConsoleInputBuffer(hConIn);
}

static BOOL
CtrlHandler(ctrltype)
DWORD ctrltype;
{
    switch (ctrltype) {
    /*	case CTRL_C_EVENT: */
    case CTRL_BREAK_EVENT:
        clear_screen();
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        getreturn_enabled = FALSE;
#ifndef NOSAVEONHANGUP
        hangup(0);
#endif
#if defined(SAFERHANGUP)
        CloseHandle(hConIn); /* trigger WAIT_FAILED */
        return TRUE;
#endif
    default:
        return FALSE;
    }
}

/* called by init_tty in wintty.c for WIN32 port only */
void
nttty_open(mode)
int mode;
{
    HANDLE hStdOut;
    DWORD cmode;
    long mask;

    GUILaunched = 0;

    try :
        /* The following lines of code were suggested by
         * Bob Landau of Microsoft WIN32 Developer support,
         * as the only current means of determining whether
         * we were launched from the command prompt, or from
         * the NT program manager. M. Allison
         */
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hStdOut) {
        GetConsoleScreenBufferInfo(hStdOut, &origcsbi);
    } else if (mode) {
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

        if (!hStdOut && !hStdIn) {
            /* Bool rval; */
            AllocConsole();
            AttachConsole(GetCurrentProcessId());
            /* 	rval = SetStdHandle(STD_OUTPUT_HANDLE, hWrite); */
            freopen("CON", "w", stdout);
            freopen("CON", "r", stdin);
        }
        mode = 0;
        goto try;
    } else {
        return;
    }

    /* Obtain handles for the standard Console I/O devices */
    hConIn = GetStdHandle(STD_INPUT_HANDLE);
    hConOut = GetStdHandle(STD_OUTPUT_HANDLE);

    load_keyboard_handler();
    /* Initialize the function pointer that points to
    * the kbhit() equivalent, in this TTY case nttty_kbhit()
    */
    nt_kbhit = nttty_kbhit;

    GetConsoleMode(hConIn, &cmode);
#ifdef NO_MOUSE_ALLOWED
    mask = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT
           | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT;
#else
    mask = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT
           | ENABLE_WINDOW_INPUT;
#endif
    /* Turn OFF the settings specified in the mask */
    cmode &= ~mask;
#ifndef NO_MOUSE_ALLOWED
    cmode |= ENABLE_MOUSE_INPUT;
#endif
    SetConsoleMode(hConIn, cmode);
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, TRUE)) {
        /* Unable to set control handler */
        cmode = 0; /* just to have a statement to break on for debugger */
    }
    get_scr_size();
    console.cursor.X = console.cursor.Y = 0;
    really_move_cursor();
}

int
process_keystroke(ir, valid, numberpad, portdebug)
INPUT_RECORD *ir;
boolean *valid;
boolean numberpad;
int portdebug;
{
    int ch = pProcessKeystroke(hConIn, ir, valid, numberpad, portdebug);
    /* check for override */
    if (ch && ch < MAX_OVERRIDES && key_overrides[ch])
        ch = key_overrides[ch];
    return ch;
}

int
nttty_kbhit()
{
    return pNHkbhit(hConIn, &ir);
}

void
get_scr_size()
{
    int lines, cols;
    
    GetConsoleScreenBufferInfo(hConOut, &csbi);
    
    lines = csbi.srWindow.Bottom - (csbi.srWindow.Top + 1);
    cols = csbi.srWindow.Right - (csbi.srWindow.Left + 1);

    LI = lines;
    CO = min(cols, 80);
    
    if ((LI < 25) || (CO < 80)) {
        COORD newcoord;

        LI = 25;
        CO = 80;

        newcoord.Y = LI;
        newcoord.X = CO;

        SetConsoleScreenBufferSize(hConOut, newcoord);
    }
}

int
tgetch()
{
    int mod;
    coord cc;
    DWORD count;
    really_move_cursor();
    return (program_state.done_hup)
               ? '\033'
               : pCheckInput(hConIn, &ir, &count, iflags.num_pad, 0, &mod,
                             &cc);
}

int
ntposkey(x, y, mod)
int *x, *y, *mod;
{
    int ch;
    coord cc;
    DWORD count;
    really_move_cursor();
    ch = (program_state.done_hup)
             ? '\033'
             : pCheckInput(hConIn, &ir, &count, iflags.num_pad, 1, mod, &cc);
    if (!ch) {
        *x = cc.x;
        *y = cc.y;
    }
    return ch;
}

static void
really_move_cursor()
{
#ifdef PORT_DEBUG
    char oldtitle[BUFSZ], newtitle[BUFSZ];
    if (display_cursor_info && wizard) {
        oldtitle[0] = '\0';
        if (GetConsoleTitle(oldtitle, BUFSZ)) {
            oldtitle[39] = '\0';
        }
        Sprintf(newtitle, "%-55s tty=(%02d,%02d) nttty=(%02d,%02d)", oldtitle,
                ttyDisplay->curx, ttyDisplay->cury, console.cursor.X, console.cursor.Y);
        (void) SetConsoleTitle(newtitle);
    }
#endif
    if (ttyDisplay) {
        console.cursor.X = ttyDisplay->curx;
        console.cursor.Y = ttyDisplay->cury;
    }
    SetConsoleCursorPosition(hConOut, console.cursor);
}

void
cmov(x, y)
register int x, y;
{
    ttyDisplay->cury = y;
    ttyDisplay->curx = x;
    console.cursor.X = x;
    console.cursor.Y = y;
}

void
nocmov(x, y)
int x, y;
{
    console.cursor.X = x;
    console.cursor.Y = y;
    ttyDisplay->curx = x;
    ttyDisplay->cury = y;
}

void
xputc(ch)
char ch;
{
    console.cursor.X = ttyDisplay->curx;
    console.cursor.Y = ttyDisplay->cury;
    xputc_core(ch);
}

void
xputs(s)
const char *s;
{
    int k;
    int slen = strlen(s);

    if (ttyDisplay) {
        console.cursor.X = ttyDisplay->curx;
        console.cursor.Y = ttyDisplay->cury;
    }

    if (s) {
        for (k = 0; k < slen && s[k]; ++k)
            xputc_core(s[k]);
    }
}

/* xputc_core() and g_putch() are the only
 * two routines that actually place output
 * on the display.
 */
void
xputc_core(ch)
char ch;
{
    boolean inverse = FALSE;
    switch (ch) {
    case '\n':
        console.cursor.Y++;
    /* fall through */
    case '\r':
        console.cursor.X = 1;
        break;
    case '\b':
        console.cursor.X--;
        break;
    default:
        inverse = (console.current_nhattr[ATR_INVERSE] && iflags.wc_inverse);
        console.attr = (inverse) ?
                        ttycolors_inv[console.current_nhcolor] :
                        ttycolors[console.current_nhcolor];
        if (console.current_nhattr[ATR_BOLD])
                console.attr |= (inverse) ?
                                BACKGROUND_INTENSITY : FOREGROUND_INTENSITY;
        WriteConsoleOutputAttribute(hConOut, &console.attr, 1, console.cursor, &acount);
        if (has_unicode) {
            /* Avoid bug in ANSI API on WinNT */
            WCHAR c2[2];
            int rc;
            rc = MultiByteToWideChar(GetConsoleOutputCP(), 0, &ch, 1, c2, 2);
            WriteConsoleOutputCharacterW(hConOut, c2, rc, console.cursor, &ccount);
        } else {
            WriteConsoleOutputCharacterA(hConOut, &ch, 1, console.cursor, &ccount);
        }
        console.cursor.X++;
    }
}

/*
 * Overrides wintty.c function of the same name
 * for win32. It is used for glyphs only, not text.
 */

/* CP437 to Unicode mapping according to the Unicode Consortium */
static const WCHAR cp437[] = {
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

void
g_putch(in_ch)
int in_ch;
{
    boolean inverse = FALSE;
    unsigned char ch = (unsigned char) in_ch;

    console.cursor.X = ttyDisplay->curx;
    console.cursor.Y = ttyDisplay->cury;

    inverse = (console.current_nhattr[ATR_INVERSE] && iflags.wc_inverse);
    console.attr = (console.current_nhattr[ATR_INVERSE] && iflags.wc_inverse) ?
                    ttycolors_inv[console.current_nhcolor] :
                    ttycolors[console.current_nhcolor];
    if (console.current_nhattr[ATR_BOLD])
        console.attr |= (inverse) ? BACKGROUND_INTENSITY : FOREGROUND_INTENSITY;
    WriteConsoleOutputAttribute(hConOut, &console.attr, 1, console.cursor, &acount);

    if (has_unicode)
        WriteConsoleOutputCharacterW(hConOut, &cp437[ch], 1, console.cursor, &ccount);
    else
        WriteConsoleOutputCharacterA(hConOut, &ch, 1, console.cursor, &ccount);
}

void
cl_end()
{
    int cx;
    console.cursor.X = ttyDisplay->curx;
    console.cursor.Y = ttyDisplay->cury;
    cx = CO - console.cursor.X;
    FillConsoleOutputAttribute(hConOut, DEFTEXTCOLOR, cx, console.cursor, &acount);
    FillConsoleOutputCharacter(hConOut, ' ', cx, console.cursor, &ccount);
    tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1, (int) ttyDisplay->cury);
}

void
raw_clear_screen()
{
    if (GetConsoleScreenBufferInfo(hConOut, &csbi)) {
        DWORD ccnt;
        COORD newcoord;

        newcoord.X = 0;
        newcoord.Y = 0;
        FillConsoleOutputAttribute(
            hConOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
            csbi.dwSize.X * csbi.dwSize.Y, newcoord, &ccnt);
        FillConsoleOutputCharacter(
            hConOut, ' ', csbi.dwSize.X * csbi.dwSize.Y, newcoord, &ccnt);
    }
}

void
clear_screen()
{
    raw_clear_screen();
    home();
}

void
home()
{
    console.cursor.X = console.cursor.Y = 0;
    ttyDisplay->curx = ttyDisplay->cury = 0;
}

void
backsp()
{
    console.cursor.X = ttyDisplay->curx;
    console.cursor.Y = ttyDisplay->cury;
    xputc_core('\b');
}

void
cl_eos()
{
    int cy = ttyDisplay->cury + 1;
    if (GetConsoleScreenBufferInfo(hConOut, &csbi)) {
        DWORD ccnt;
        COORD newcoord;

        newcoord.X = ttyDisplay->curx;
        newcoord.Y = ttyDisplay->cury;
        FillConsoleOutputAttribute(
            hConOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
            csbi.dwSize.X * csbi.dwSize.Y - cy, newcoord, &ccnt);
        FillConsoleOutputCharacter(hConOut, ' ',
                                   csbi.dwSize.X * csbi.dwSize.Y - cy,
                                   newcoord, &ccnt);
    }
    tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1, (int) ttyDisplay->cury);
}

void
tty_nhbell()
{
    if (flags.silent)
        return;
    Beep(8000, 500);
}

volatile int junk; /* prevent optimizer from eliminating loop below */

void
tty_delay_output()
{
    /* delay 50 ms - uses ANSI C clock() function now */
    clock_t goal;
    int k;

    goal = 50 + clock();
    while (goal > clock()) {
        k = junk; /* Do nothing */
    }
}

#ifdef TEXTCOLOR
/*
 * CLR_BLACK		0
 * CLR_RED		1
 * CLR_GREEN		2
 * CLR_BROWN		3	low-intensity yellow
 * CLR_BLUE		4
 * CLR_MAGENTA 		5
 * CLR_CYAN		6
 * CLR_GRAY		7	low-intensity white
 * NO_COLOR		8
 * CLR_ORANGE		9
 * CLR_BRIGHT_GREEN	10
 * CLR_YELLOW		11
 * CLR_BRIGHT_BLUE	12
 * CLR_BRIGHT_MAGENTA  	13
 * CLR_BRIGHT_CYAN	14
 * CLR_WHITE		15
 * CLR_MAX		16
 * BRIGHT		8
 */

static void
init_ttycolor()
{
#ifdef TEXTCOLOR
    ttycolors[CLR_BLACK]        = FOREGROUND_INTENSITY; /* fix by Quietust */
    ttycolors[CLR_RED]          = FOREGROUND_RED;
    ttycolors[CLR_GREEN]        = FOREGROUND_GREEN;
    ttycolors[CLR_BROWN]        = FOREGROUND_GREEN | FOREGROUND_RED;
    ttycolors[CLR_BLUE]         = FOREGROUND_BLUE;
    ttycolors[CLR_MAGENTA]      = FOREGROUND_BLUE | FOREGROUND_RED;
    ttycolors[CLR_CYAN]         = FOREGROUND_GREEN | FOREGROUND_BLUE;
    ttycolors[CLR_GRAY]         = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE;
    ttycolors[NO_COLOR]         = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED;
    ttycolors[CLR_ORANGE]       = FOREGROUND_RED | FOREGROUND_INTENSITY;
    ttycolors[CLR_BRIGHT_GREEN] = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    ttycolors[CLR_YELLOW]       = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
    ttycolors[CLR_BRIGHT_BLUE]  = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    ttycolors[CLR_BRIGHT_MAGENTA]=FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY;
    ttycolors[CLR_BRIGHT_CYAN]  = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    ttycolors[CLR_WHITE]        = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED
                                    | FOREGROUND_INTENSITY;

    ttycolors_inv[CLR_BLACK]       = BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_RED
                                        | BACKGROUND_INTENSITY;
    ttycolors_inv[CLR_RED]         = BACKGROUND_RED | BACKGROUND_INTENSITY;
    ttycolors_inv[CLR_GREEN]       = BACKGROUND_GREEN;
    ttycolors_inv[CLR_BROWN]       = BACKGROUND_GREEN | BACKGROUND_RED;
    ttycolors_inv[CLR_BLUE]        = BACKGROUND_BLUE | BACKGROUND_INTENSITY;
    ttycolors_inv[CLR_MAGENTA]     = BACKGROUND_BLUE | BACKGROUND_RED;
    ttycolors_inv[CLR_CYAN]        = BACKGROUND_GREEN | BACKGROUND_BLUE;
    ttycolors_inv[CLR_GRAY]        = BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_BLUE;
    ttycolors_inv[NO_COLOR]        = BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_RED;
    ttycolors_inv[CLR_ORANGE]      = BACKGROUND_RED | BACKGROUND_INTENSITY;
    ttycolors_inv[CLR_BRIGHT_GREEN]= BACKGROUND_GREEN | BACKGROUND_INTENSITY;
    ttycolors_inv[CLR_YELLOW]      = BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
    ttycolors_inv[CLR_BRIGHT_BLUE] = BACKGROUND_BLUE | BACKGROUND_INTENSITY;
    ttycolors_inv[CLR_BRIGHT_MAGENTA] =BACKGROUND_BLUE | BACKGROUND_RED | BACKGROUND_INTENSITY;
    ttycolors_inv[CLR_BRIGHT_CYAN] = BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
    ttycolors_inv[CLR_WHITE]       = BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_RED
                                       | BACKGROUND_INTENSITY;
#else
    int k;
    ttycolors[0] = FOREGROUND_INTENSITY;
    ttycolors_inv[0] = BACKGROUND_INTENSITY;
    for (k = 1; k < SIZE(ttycolors); ++k) {
        ttycolors[k] = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED;
        ttycolors_inv[k] = BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_RED;
    }
#endif
    init_ttycolor_completed = TRUE;
}
#endif /* TEXTCOLOR */

int
has_color(int color)
{
#ifdef TEXTCOLOR
    if ((color >= 0) && (color < CLR_MAX))
        return 1;
#else
    if ((color == CLR_BLACK) || (color == CLR_WHITE) || (color == NO_COLOR))
        return 1;
#endif
    else
        return 0;
}

void
term_start_attr(int attrib)
{
    console.current_nhattr[attrib] = TRUE;
    if (attrib) console.current_nhattr[ATR_NONE] = FALSE;
}

void
term_end_attr(int attrib)
{
    int k;

    switch (attrib) {
    case ATR_INVERSE:
    case ATR_ULINE:
    case ATR_BLINK:
    case ATR_BOLD:
        break;
    }
    console.current_nhattr[attrib] = FALSE;
    console.current_nhattr[ATR_NONE] = TRUE;
    /* re-evaluate all attr now for performance at output time */
    for (k=ATR_NONE; k <= ATR_INVERSE; ++k) {
        if (console.current_nhattr[k])
            console.current_nhattr[ATR_NONE] = FALSE;
    }
}

void
term_end_raw_bold(void)
{
    term_end_attr(ATR_BOLD);
}

void
term_start_raw_bold(void)
{
    term_start_attr(ATR_BOLD);
}

void
term_start_color(int color)
{
#ifdef TEXTCOLOR
    if (color >= 0 && color < CLR_MAX) {
        console.current_nhcolor = color;
    } else
#endif
    console.current_nhcolor = NO_COLOR;
}

void
term_end_color(void)
{
#ifdef TEXTCOLOR
    console.foreground = DEFTEXTCOLOR;
#endif
    console.attr = (console.foreground | console.background);
    console.current_nhcolor = NO_COLOR;
}

void
standoutbeg()
{
    term_start_attr(ATR_BOLD);
}

void
standoutend()
{
    term_end_attr(ATR_BOLD);
}

#ifndef NO_MOUSE_ALLOWED
void
toggle_mouse_support()
{
    DWORD cmode;
    GetConsoleMode(hConIn, &cmode);
    if (iflags.wc_mouse_support)
        cmode |= ENABLE_MOUSE_INPUT;
    else
        cmode &= ~ENABLE_MOUSE_INPUT;
    SetConsoleMode(hConIn, cmode);
}
#endif

/* handle tty options updates here */
void
nttty_preference_update(pref)
const char *pref;
{
    if (stricmp(pref, "mouse_support") == 0) {
#ifndef NO_MOUSE_ALLOWED
        toggle_mouse_support();
#endif
    }
    if (stricmp(pref, "symset") == 0)
        check_and_set_font();
    return;
}

#ifdef PORT_DEBUG
void
win32con_debug_keystrokes()
{
    DWORD count;
    boolean valid = 0;
    int ch;
    xputs("\n");
    while (!valid || ch != 27) {
        nocmov(ttyDisplay->curx, ttyDisplay->cury);
        ReadConsoleInput(hConIn, &ir, 1, &count);
        if ((ir.EventType == KEY_EVENT) && ir.Event.KeyEvent.bKeyDown)
            ch = process_keystroke(&ir, &valid, iflags.num_pad, 1);
    }
    (void) doredraw();
}
void
win32con_handler_info()
{
    char *buf;
    int ci;
    if (!pSourceAuthor && !pSourceWhere)
        pline("Keyboard handler source info and author unavailable.");
    else {
        if (pKeyHandlerName && pKeyHandlerName(&buf, 1)) {
            xputs("\n");
            xputs("Keystroke handler loaded: \n    ");
            xputs(buf);
        }
        if (pSourceAuthor && pSourceAuthor(&buf)) {
            xputs("\n");
            xputs("Keystroke handler Author: \n    ");
            xputs(buf);
        }
        if (pSourceWhere && pSourceWhere(&buf)) {
            xputs("\n");
            xputs("Keystroke handler source code available at:\n    ");
            xputs(buf);
        }
        xputs("\nPress any key to resume.");
        ci = nhgetch();
        (void) doredraw();
    }
}

void
win32con_toggle_cursor_info()
{
    display_cursor_info = !display_cursor_info;
}
#endif

void
map_subkeyvalue(op)
register char *op;
{
    char digits[] = "0123456789";
    int length, i, idx, val;
    char *kp;

    idx = -1;
    val = -1;
    kp = index(op, '/');
    if (kp) {
        *kp = '\0';
        kp++;
        length = strlen(kp);
        if (length < 1 || length > 3)
            return;
        for (i = 0; i < length; i++)
            if (!index(digits, kp[i]))
                return;
        val = atoi(kp);
        length = strlen(op);
        if (length < 1 || length > 3)
            return;
        for (i = 0; i < length; i++)
            if (!index(digits, op[i]))
                return;
        idx = atoi(op);
    }
    if (idx >= MAX_OVERRIDES || idx < 0 || val >= MAX_OVERRIDES || val < 1)
        return;
    key_overrides[idx] = val;
}

void
load_keyboard_handler()
{
    char suffx[] = ".dll";
    char *truncspot;
#define MAX_DLLNAME 25
    char kh[MAX_ALTKEYHANDLER];
    if (iflags.altkeyhandler[0]) {
        if (hLibrary) { /* already one loaded apparently */
            FreeLibrary(hLibrary);
            hLibrary = (HANDLE) 0;
            pNHkbhit = (NHKBHIT) 0;
            pCheckInput = (CHECKINPUT) 0;
            pSourceWhere = (SOURCEWHERE) 0;
            pSourceAuthor = (SOURCEAUTHOR) 0;
            pKeyHandlerName = (KEYHANDLERNAME) 0;
            pProcessKeystroke = (PROCESS_KEYSTROKE) 0;
        }
        if ((truncspot = strstri(iflags.altkeyhandler, suffx)) != 0)
            *truncspot = '\0';
        (void) strncpy(kh, iflags.altkeyhandler,
                       (MAX_ALTKEYHANDLER - sizeof suffx) - 1);
        kh[(MAX_ALTKEYHANDLER - sizeof suffx) - 1] = '\0';
        Strcat(kh, suffx);
        Strcpy(iflags.altkeyhandler, kh);
        hLibrary = LoadLibrary(kh);
        if (hLibrary) {
            pProcessKeystroke = (PROCESS_KEYSTROKE) GetProcAddress(
                hLibrary, TEXT("ProcessKeystroke"));
            pNHkbhit = (NHKBHIT) GetProcAddress(hLibrary, TEXT("NHkbhit"));
            pCheckInput =
                (CHECKINPUT) GetProcAddress(hLibrary, TEXT("CheckInput"));
            pSourceWhere =
                (SOURCEWHERE) GetProcAddress(hLibrary, TEXT("SourceWhere"));
            pSourceAuthor =
                (SOURCEAUTHOR) GetProcAddress(hLibrary, TEXT("SourceAuthor"));
            pKeyHandlerName = (KEYHANDLERNAME) GetProcAddress(
                hLibrary, TEXT("KeyHandlerName"));
        }
    }
    if (!pProcessKeystroke || !pNHkbhit || !pCheckInput) {
        if (hLibrary) {
            FreeLibrary(hLibrary);
            hLibrary = (HANDLE) 0;
            pNHkbhit = (NHKBHIT) 0;
            pCheckInput = (CHECKINPUT) 0;
            pSourceWhere = (SOURCEWHERE) 0;
            pSourceAuthor = (SOURCEAUTHOR) 0;
            pKeyHandlerName = (KEYHANDLERNAME) 0;
            pProcessKeystroke = (PROCESS_KEYSTROKE) 0;
        }
        (void) strncpy(kh, "nhdefkey.dll",
                       (MAX_ALTKEYHANDLER - sizeof suffx) - 1);
        kh[(MAX_ALTKEYHANDLER - sizeof suffx) - 1] = '\0';
        Strcpy(iflags.altkeyhandler, kh);
        hLibrary = LoadLibrary(kh);
        if (hLibrary) {
            pProcessKeystroke = (PROCESS_KEYSTROKE) GetProcAddress(
                hLibrary, TEXT("ProcessKeystroke"));
            pCheckInput =
                (CHECKINPUT) GetProcAddress(hLibrary, TEXT("CheckInput"));
            pNHkbhit = (NHKBHIT) GetProcAddress(hLibrary, TEXT("NHkbhit"));
            pSourceWhere =
                (SOURCEWHERE) GetProcAddress(hLibrary, TEXT("SourceWhere"));
            pSourceAuthor =
                (SOURCEAUTHOR) GetProcAddress(hLibrary, TEXT("SourceAuthor"));
            pKeyHandlerName = (KEYHANDLERNAME) GetProcAddress(
                hLibrary, TEXT("KeyHandlerName"));
        }
    }
    if (!pProcessKeystroke || !pNHkbhit || !pCheckInput) {
        if (!hLibrary)
            raw_printf("\nNetHack was unable to load keystroke handler.\n");
        else {
            FreeLibrary(hLibrary);
            hLibrary = (HANDLE) 0;
            raw_printf("\nNetHack keystroke handler is invalid.\n");
        }
        exit(EXIT_FAILURE);
    }
}

/* this is used as a printf() replacement when the window
 * system isn't initialized yet
 */
void msmsg
VA_DECL(const char *, fmt)
{
    char buf[ROWNO * COLNO]; /* worst case scenario */
    VA_START(fmt);
    VA_INIT(fmt, const char *);
    Vsprintf(buf, fmt, VA_ARGS);
    if (redirect_stdout)
        fprintf(stdout, "%s", buf);
    else {
        if(!init_ttycolor_completed)
            init_ttycolor();

        xputs(buf);
        if (ttyDisplay)
            curs(BASE_WINDOW, console.cursor.X + 1, console.cursor.Y);
    }
    VA_END();
    return;
}

/* fatal error */
/*VARARGS1*/
void nttty_error
VA_DECL(const char *, s)
{
    char buf[BUFSZ];
    VA_START(s);
    VA_INIT(s, const char *);
    /* error() may get called before tty is initialized */
    if (iflags.window_inited)
        end_screen();
    buf[0] = '\n';
    (void) vsprintf(&buf[1], s, VA_ARGS);
    msmsg(buf);
    really_move_cursor();
    VA_END();
    exit(EXIT_FAILURE);
}

void
synch_cursor()
{
    really_move_cursor();
}

#ifdef CHANGE_COLOR
void
tty_change_color(color_number, rgb, reverse)
int color_number, reverse;
long rgb;
{
    /* Map NetHack color index to NT Console palette index */
    int idx, win32_color_number[] = {
        0,  /* CLR_BLACK           0 */
        4,  /* CLR_RED             1 */
        2,  /* CLR_GREEN           2 */
        6,  /* CLR_BROWN           3 */
        1,  /* CLR_BLUE            4 */
        5,  /* CLR_MAGENTA         5 */
        3,  /* CLR_CYAN            6 */
        7,  /* CLR_GRAY            7 */
        8,  /* NO_COLOR            8 */
        12, /* CLR_ORANGE          9 */
        10, /* CLR_BRIGHT_GREEN   10 */
        14, /* CLR_YELLOW         11 */
        9,  /* CLR_BRIGHT_BLUE    12 */
        13, /* CLR_BRIGHT_MAGENTA 13 */
        11, /* CLR_BRIGHT_CYAN    14 */
        15  /* CLR_WHITE          15 */
    };
    int k;
    if (color_number < 0) { /* indicates OPTIONS=palette with no value */
        /* copy the NetHack palette into UserDefinedColors */
        for (k = 0; k < CLR_MAX; k++)
            UserDefinedColors[k] = NetHackColors[k];
    } else if (color_number >= 0 && color_number < CLR_MAX) {
        if (!altered_palette) {
            /* make sure a full suite is available */
            for (k = 0; k < CLR_MAX; k++)
                UserDefinedColors[k] = DefaultColors[k];
        }
        idx = win32_color_number[color_number];
        UserDefinedColors[idx] = rgb;
    }
    altered_palette = TRUE;
}

char *
tty_get_color_string()
{
    return "";
}

int
match_color_name(c)
const char *c;
{
    const struct others {
        int idx;
        const char *colorname;
    } othernames[] = {
        { CLR_MAGENTA, "purple" },
        { CLR_BRIGHT_MAGENTA, "bright purple" },
        { NO_COLOR, "dark gray" },
        { NO_COLOR, "dark grey" },
        { CLR_GRAY, "grey" },
    };

    int cnt;
    for (cnt = 0; cnt < CLR_MAX; ++cnt) {
        if (!strcmpi(c, c_obj_colors[cnt]))
            return cnt;
    }
    for (cnt = 0; cnt < SIZE(othernames); ++cnt) {
        if (!strcmpi(c, othernames[cnt].colorname))
            return othernames[cnt].idx;
    }
    return -1;
}

/*
 * Returns 0 if badoption syntax
 */
int
alternative_palette(op)
char *op;
{
    /*
     *	palette:color-R-G-B
     *	OPTIONS=palette:green-4-3-1, palette:0-0-0-0
     */
    int fieldcnt, color_number, rgb, red, green, blue;
    char *fields[4], *cp;

    if (!op) {
        change_color(-1, 0, 0); /* indicates palette option with
                                   no value meaning "load an entire
                                   hard-coded NetHack palette." */
        return 1;
    }

    cp = fields[0] = op;
    for (fieldcnt = 1; fieldcnt < 4; ++fieldcnt) {
        cp = index(cp, '-');
        if (!cp)
            return 0;
        fields[fieldcnt] = cp;
        cp++;
    }
    for (fieldcnt = 1; fieldcnt < 4; ++fieldcnt) {
        *(fields[fieldcnt]) = '\0';
        ++fields[fieldcnt];
    }
    rgb = 0;
    for (fieldcnt = 0; fieldcnt < 4; ++fieldcnt) {
        if (fieldcnt == 0 && isalpha(*(fields[0]))) {
            color_number = match_color_name(fields[0]);
            if (color_number == -1)
                return 0;
        } else {
            int dcount = 0, cval = 0;
            cp = fields[fieldcnt];
            if (*cp == '\\' && index("0123456789xXoO", cp[1])) {
                const char *dp, *hex = "00112233445566778899aAbBcCdDeEfF";

                cp++;
                if (*cp == 'x' || *cp == 'X')
                    for (++cp; (dp = index(hex, *cp)) && (dcount++ < 2); cp++)
                        cval = (int) ((cval * 16) + (dp - hex) / 2);
                else if (*cp == 'o' || *cp == 'O')
                    for (++cp; (index("01234567", *cp)) && (dcount++ < 3);
                         cp++)
                        cval = (cval * 8) + (*cp - '0');
                else
                    return 0;
            } else {
                for (; *cp && (index("0123456789", *cp)) && (dcount++ < 3);
                     cp++)
                    cval = (cval * 10) + (*cp - '0');
            }
            switch (fieldcnt) {
            case 0:
                color_number = cval;
                break;
            case 1:
                red = cval;
                break;
            case 2:
                green = cval;
                break;
            case 3:
                blue = cval;
                break;
            }
        }
    }
    rgb = RGB(red, green, blue);
    if (color_number >= 0 && color_number < CLR_MAX)
        change_color(color_number, rgb, 0);
    return 1;
}

/*
 *  This uses an undocumented method to set console attributes
 *  at runtime including console palette
 *
 * 	VOID WINAPI SetConsolePalette(COLORREF palette[16])
 *
 *  Author: James Brown at www.catch22.net
 *
 *  Set palette of current console.
 *  Palette should be of the form:
 *
 * 	COLORREF DefaultColors[CLR_MAX] =
 * 	{
 *		0x00000000, 0x00800000, 0x00008000, 0x00808000,
 *		0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
 *		0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00,
 *		0x000000ff, 0x00ff00ff,	0x0000ffff, 0x00ffffff
 *	 };
 */

#pragma pack(push, 1)

/*
 *	Structure to send console via WM_SETCONSOLEINFO
 */
typedef struct _CONSOLE_INFO {
    ULONG Length;
    COORD ScreenBufferSize;
    COORD WindowSize;
    ULONG WindowPosX;
    ULONG WindowPosY;

    COORD FontSize;
    ULONG FontFamily;
    ULONG FontWeight;
    WCHAR FaceName[32];

    ULONG CursorSize;
    ULONG FullScreen;
    ULONG QuickEdit;
    ULONG AutoPosition;
    ULONG InsertMode;

    USHORT ScreenColors;
    USHORT PopupColors;
    ULONG HistoryNoDup;
    ULONG HistoryBufferSize;
    ULONG NumberOfHistoryBuffers;

    COLORREF ColorTable[16];

    ULONG CodePage;
    HWND Hwnd;

    WCHAR ConsoleTitle[0x100];
} CONSOLE_INFO;

#pragma pack(pop)

BOOL SetConsoleInfo(HWND hwndConsole, CONSOLE_INFO *pci);
static void GetConsoleSizeInfo(CONSOLE_INFO *pci);
VOID WINAPI SetConsolePalette(COLORREF crPalette[16]);

void
adjust_palette(VOID_ARGS)
{
    SetConsolePalette(UserDefinedColors);
    altered_palette = 0;
}

/*
/* only in Win2k+  (use FindWindow for NT4) */
/* HWND WINAPI GetConsoleWindow(); */

/*  Undocumented console message */
#define WM_SETCONSOLEINFO (WM_USER + 201)

VOID WINAPI
SetConsolePalette(COLORREF palette[16])
{
    CONSOLE_INFO ci = { sizeof(ci) };
    int i;
    HWND hwndConsole = GetConsoleHandle();

    /* get current size/position settings rather than using defaults.. */
    GetConsoleSizeInfo(&ci);

    /* set these to zero to keep current settings */
    ci.FontSize.X = 0; /* def = 8  */
    ci.FontSize.Y = 0; /* def = 12 */
    ci.FontFamily = 0; /* def = 0x30 = FF_MODERN|FIXED_PITCH */
    ci.FontWeight = 0; /* 0x400;   */
    /* lstrcpyW(ci.FaceName, L"Terminal"); */
    ci.FaceName[0] = L'\0';

    ci.CursorSize = 25;
    ci.FullScreen = FALSE;
    ci.QuickEdit = TRUE;
    ci.AutoPosition = 0x10000;
    ci.InsertMode = TRUE;
    ci.ScreenColors = MAKEWORD(0x7, 0x0);
    ci.PopupColors = MAKEWORD(0x5, 0xf);

    ci.HistoryNoDup = FALSE;
    ci.HistoryBufferSize = 50;
    ci.NumberOfHistoryBuffers = 4;

    // colour table
    for (i = 0; i < 16; i++)
        ci.ColorTable[i] = palette[i];

    ci.CodePage = GetConsoleOutputCP();
    ci.Hwnd = hwndConsole;

    lstrcpyW(ci.ConsoleTitle, L"");

    SetConsoleInfo(hwndConsole, &ci);
}

/*
 *  Wrapper around WM_SETCONSOLEINFO. We need to create the
 *  necessary section (file-mapping) object in the context of the
 *  process which owns the console, before posting the message
 */
BOOL
SetConsoleInfo(HWND hwndConsole, CONSOLE_INFO *pci)
{
    DWORD dwConsoleOwnerPid;
    HANDLE hProcess;
    HANDLE hSection, hDupSection;
    PVOID ptrView = 0;
    HANDLE hThread;

    /*
     *	Open the process which "owns" the console
     */
    GetWindowThreadProcessId(hwndConsole, &dwConsoleOwnerPid);
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwConsoleOwnerPid);

    /*
     * Create a SECTION object backed by page-file, then map a view of
     * this section into the owner process so we can write the contents
     * of the CONSOLE_INFO buffer into it
     */
    hSection = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0,
                                 pci->Length, 0);

    /*
     *	Copy our console structure into the section-object
     */
    ptrView = MapViewOfFile(hSection, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0,
                            pci->Length);
    memcpy(ptrView, pci, pci->Length);
    UnmapViewOfFile(ptrView);

    /*
     *	Map the memory into owner process
     */
    DuplicateHandle(GetCurrentProcess(), hSection, hProcess, &hDupSection, 0,
                    FALSE, DUPLICATE_SAME_ACCESS);

    /*  Send console window the "update" message */
    SendMessage(hwndConsole, WM_SETCONSOLEINFO, (WPARAM) hDupSection, 0);

    /*
     * clean up
     */
    hThread = CreateRemoteThread(hProcess, 0, 0,
                                 (LPTHREAD_START_ROUTINE) CloseHandle,
                                 hDupSection, 0, 0);

    CloseHandle(hThread);
    CloseHandle(hSection);
    CloseHandle(hProcess);

    return TRUE;
}

/*
 *  Fill the CONSOLE_INFO structure with information
 *  about the current console window
 */
static void
GetConsoleSizeInfo(CONSOLE_INFO *pci)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleScreenBufferInfo(hConsoleOut, &csbi);

    pci->ScreenBufferSize = csbi.dwSize;
    pci->WindowSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    pci->WindowSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    pci->WindowPosX = csbi.srWindow.Left;
    pci->WindowPosY = csbi.srWindow.Top;
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
#endif /*CHANGE_COLOR*/

static int CALLBACK EnumFontCallback(
    const LOGFONTW * lf, const TEXTMETRICW * tm, DWORD fontType, LPARAM lParam)
{
    LOGFONTW * lf_ptr = (LOGFONTW *) lParam;
    *lf_ptr = *lf;
    return 0;
}

/* check_and_set_font ensures that the current font will render the symbols
 * that are currently being used correctly.  If they will not be rendered
 * correctly, then it will change the font to a known good font.
 */
void
check_and_set_font()
{
    if (!check_font_widths()) {
        raw_print("WARNING: glyphs too wide in console font."
                  "  Changing code page to 437 and font to Consolas\n");
        set_known_good_console_font();
    }
}

/* check_font_widths returns TRUE if all glyphs in current console font
 * fit within the width of a single console cell.
 */
boolean
check_font_widths()
{
    CONSOLE_FONT_INFOEX console_font_info;
    console_font_info.cbSize = sizeof(console_font_info);
    BOOL success = GetCurrentConsoleFontEx(hConOut, FALSE,
                                            &console_font_info);

    /* get console window and DC
     * NOTE: the DC from the console window does not have the correct
     *       font selected at this point.
     */
    HWND hWnd = GetConsoleWindow();
    HDC hDC = GetDC(hWnd);

    LOGFONTW logical_font;
    logical_font.lfCharSet = DEFAULT_CHARSET;
    wcscpy(logical_font.lfFaceName, console_font_info.FaceName);
    logical_font.lfPitchAndFamily = 0;

    /* getting matching console font */
    LOGFONTW matching_log_font = { 0 };
    EnumFontFamiliesExW(hDC, &logical_font, EnumFontCallback,
                                        (LPARAM) &matching_log_font, 0);

    if (matching_log_font.lfHeight == 0) {
        raw_print("Unable to enumerate system fonts\n");
        return FALSE;
    }

    /* create font matching console font */
    LOGFONTW console_font_log_font = matching_log_font;
    console_font_log_font.lfWeight = console_font_info.FontWeight;
    console_font_log_font.lfHeight = console_font_info.dwFontSize.Y;
    console_font_log_font.lfWidth = console_font_info.dwFontSize.X;
    HFONT console_font = CreateFontIndirectW(&console_font_log_font);

    if (console_font == NULL) {
        raw_print("Unable to create console font\n");
        return FALSE;
    }

    /* select font */
    HGDIOBJ saved_font = SelectObject(hDC, console_font);

    /* determine whether it is a true type font */
    TEXTMETRICA tm;
    success = GetTextMetricsA(hDC, &tm);

    if (!success) {
        raw_print("Unable to get console font text metrics\n");
        goto clean_up;
    }

    boolean isTrueType = (tm.tmPitchAndFamily & TMPF_TRUETYPE) != 0;

    /* determine which glyphs are used */
    boolean used[256];
    memset(used, 0, sizeof(used));
    for (int i = 0; i < SYM_MAX; i++) {
        used[l_syms[i]] = TRUE;
        used[r_syms[i]] = TRUE;
    }

    int wcUsedCount = 0;
    wchar_t wcUsed[256];
    for (int i = 0; i < sizeof(used); i++)
        if (used[i])
            wcUsed[wcUsedCount++] = cp437[i];

    /* measure the set of used glyphs to ensure they fit */
    boolean all_glyphs_fit = TRUE;

    for (int i = 0; i < wcUsedCount; i++) {
        int width;
        if (isTrueType) {
            ABC abc;
            success = GetCharABCWidthsW(hDC, wcUsed[i], wcUsed[i], &abc);
            width = abc.abcA + abc.abcB + abc.abcC;
        } else {
            success = GetCharWidthW(hDC, wcUsed[i], wcUsed[i], &width);
        }

        if (success && width > console_font_info.dwFontSize.X) {
            all_glyphs_fit = FALSE;
            break;
        }
    }

clean_up:

    SelectObject(hDC, saved_font);
    DeleteObject(console_font);

    return all_glyphs_fit;
}

/* set_known_good_console_font sets the code page and font used by the console
 * to settings know to work well with NetHack.  It also saves the original
 * settings so that they can be restored prior to NetHack exit.
 */
void
set_known_good_console_font()
{
    CONSOLE_FONT_INFOEX console_font_info;
    console_font_info.cbSize = sizeof(console_font_info);
    BOOL success = GetCurrentConsoleFontEx(hConOut, FALSE,
                                            &console_font_info);

    console_font_changed = TRUE;
    original_console_font_info = console_font_info;
    original_console_code_page = GetConsoleOutputCP();

    wcscpy_s(console_font_info.FaceName,
        sizeof(console_font_info.FaceName)
            / sizeof(console_font_info.FaceName[0]),
        L"Consolas");

    success = SetConsoleOutputCP(437);
    if (!success)
        raw_print("Unable to set console code page to 437\n");

    success = SetCurrentConsoleFontEx(hConOut, FALSE, &console_font_info);
    if (!success)
        raw_print("Unable to set console font to Consolas\n");
}

/* restore_original_console_font will restore the console font and code page
 * settings to what they were when NetHack was launched.
 */
void
restore_original_console_font()
{
    if (console_font_changed) {
        BOOL success;
        raw_print("Restoring original font and code page\n");
        success = SetConsoleOutputCP(original_console_code_page);
        if (!success)
            raw_print("Unable to restore original code page\n");

        success = SetCurrentConsoleFontEx(hConOut, FALSE,
                                            &original_console_font_info);
        if (!success)
            raw_print("Unable to restore original font\n");

        console_font_changed = FALSE;
    }
}

#endif /* WIN32 */

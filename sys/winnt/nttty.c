/* NetHack 3.6	nttty.c	$NHDT-Date: 1524931557 2018/04/28 16:05:57 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.71 $ */
/* Copyright (c) NetHack PC Development Team 1993    */
/* NetHack may be freely redistributed.  See license for details. */

/* tty.c - (Windows NT) version */

/*
 * Initial Creation 				M. Allison	1993/01/31
 * Switch to low level console output routines	M. Allison	2003/10/01
 * Restrict cursor movement until input pending	M. Lehotay	2003/10/02
 * Call Unicode version of output API on NT     R. Chason   2005/10/28
 * Use of back buffer to improve performance    B. House    2018/05/06
 *
 */


#ifdef WIN32
#define NEED_VARARGS /* Uses ... */
#include "hack.h"
#include "wintty.h"
#include <sys\types.h>
#include <sys\stat.h>
#include "win32api.h"

extern boolean getreturn_enabled; /* from sys/share/pcsys.c */
extern int redirect_stdout;

#ifdef TTY_GRAPHICS
/*
 * Console Buffer Flipping Support
 *
 * To minimize the number of calls into the WriteConsoleOutputXXX methods,
 * we implement a notion of a console back buffer which keeps the next frame
 * of console output as it is being composed.  When ready to show the new
 * frame, we compare this next frame to what is currently being output and
 * only call WriteConsoleOutputXXX for those console values that need to
 * change.
 *
 */

#define CONSOLE_CLEAR_ATTRIBUTE (FOREGROUND_RED | FOREGROUND_GREEN \
                                                | FOREGROUND_BLUE)
#define CONSOLE_CLEAR_CHARACTER (' ')

#define CONSOLE_UNDEFINED_ATTRIBUTE (0)
#define CONSOLE_UNDEFINED_CHARACTER ('\0')

typedef struct {
    WCHAR   character;
    WORD    attribute;
} cell_t;

cell_t clear_cell = { CONSOLE_CLEAR_CHARACTER, CONSOLE_CLEAR_ATTRIBUTE };
cell_t undefined_cell = { CONSOLE_UNDEFINED_CHARACTER,
                          CONSOLE_UNDEFINED_ATTRIBUTE };

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

/* Win32 Screen buffer,coordinate,console I/O information */
COORD ntcoord;
INPUT_RECORD ir;

/* Support for changing console font if existing glyph widths are too wide */

/* Flag for whether NetHack was launched via the GUI, not the command line.
 * The reason we care at all, is so that we can get
 * a final RETURN at the end of the game when launched from the GUI
 * to prevent the scoreboard (or panic message :-|) from vanishing
 * immediately after it is displayed, yet not bother when started
 * from the command line.
 */
int GUILaunched = FALSE;
/* Flag for whether unicode is supported */
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
    HANDLE hConOut;
    HANDLE hConIn;
    CONSOLE_SCREEN_BUFFER_INFO origcsbi;
    int width;
    int height;
    boolean has_unicode;
    int buffer_size;
    cell_t * front_buffer;
    cell_t * back_buffer;
    WCHAR cpMap[256];
    boolean font_changed;
    CONSOLE_FONT_INFOEX original_font_info;
    UINT original_code_page;
} console = {
    0,
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED),
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED),
    NO_COLOR,
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
    {0, 0},
    NULL,
    NULL,
    { 0 },
    0,
    0,
    FALSE,
    0,
    NULL,
    NULL,
    { 0 },
    FALSE,
    { 0 },
    0
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

typedef struct {
    char *              name;       // name without DLL extension
    HANDLE              hLibrary;
    PROCESS_KEYSTROKE   pProcessKeystroke;
    NHKBHIT             pNHkbhit;
    CHECKINPUT          pCheckInput;
    SOURCEWHERE         pSourceWhere;
    SOURCEAUTHOR        pSourceAuthor;
    KEYHANDLERNAME      pKeyHandlerName;
} keyboard_handler_t;

keyboard_handler_t keyboard_handler;


/* Console buffer flipping support */

static void back_buffer_flip()
{
    cell_t * back = console.back_buffer;
    cell_t * front = console.front_buffer;
    COORD pos;
    DWORD unused;

    for (pos.Y = 0; pos.Y < console.height; pos.Y++) {
        for (pos.X = 0; pos.X < console.width; pos.X++) {
            if (back->attribute != front->attribute) {
                WriteConsoleOutputAttribute(console.hConOut, &back->attribute,
                                            1, pos, &unused);
                front->attribute = back->attribute;
            }
            if (back->character != front->character) {
                if (console.has_unicode) {
                    WriteConsoleOutputCharacterW(console.hConOut,
                        &back->character, 1, pos, &unused);
                } else {
                    char ch = (char)back->character;
                    WriteConsoleOutputCharacterA(console.hConOut, &ch, 1, pos,
                                                    &unused);
                }
                *front = *back;
            }
            back++;
            front++;
        }
    }
}

void buffer_fill_to_end(cell_t * buffer, cell_t * fill, int x, int y)
{
    ntassert(x >= 0 && x < console.width);
    ntassert(y >= 0 && ((y < console.height) || (y == console.height && 
                                                 x == 0)));

    cell_t * dst = buffer + console.width * y + x;
    cell_t * sentinel = buffer + console.buffer_size;
    while (dst != sentinel)
        *dst++ = *fill;

    if (iflags.debug.immediateflips && buffer == console.back_buffer)
        back_buffer_flip();
}

static void buffer_clear_to_end_of_line(cell_t * buffer, int x, int y)
{
    ntassert(x >= 0 && x < console.width);
    ntassert(y >= 0 && ((y < console.height) || (y == console.height && 
                                                 x == 0)));
    cell_t * dst = buffer + console.width * y + x;
    cell_t *sentinel = buffer + console.width * (y + 1);

    while (dst != sentinel)
        *dst++ = clear_cell;

    if (iflags.debug.immediateflips)
        back_buffer_flip();
}

void buffer_write(cell_t * buffer, cell_t * cell, COORD pos)
{
    ntassert(pos.X >= 0 && pos.X < console.width);
    ntassert(pos.Y >= 0 && pos.Y < console.height);

    cell_t * dst = buffer + (console.width * pos.Y) + pos.X;
    *dst = *cell;

    if (iflags.debug.immediateflips && buffer == console.back_buffer)
        back_buffer_flip();
}

/*
 * Called after returning from ! or ^Z
 */
void
gettty()
{
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
}

void
tty_startup(wid, hgt)
int *wid, *hgt;
{
    *wid = console.width;
    *hgt = console.height;
    set_option_mod_status("mouse_support", SET_IN_GAME);
}

void
tty_number_pad(state)
int state;
{
    // do nothing
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
    buffer_fill_to_end(console.back_buffer, &clear_cell, 0, 0);
    back_buffer_flip();
    FlushConsoleInputBuffer(console.hConIn);
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
        CloseHandle(console.hConIn); /* trigger WAIT_FAILED */
        return TRUE;
#endif
    default:
        return FALSE;
    }
}

/* called by pcmain() and process_options() */
void
nttty_open(mode)
int mode; // unused
{
    DWORD cmode;

    /* Initialize the function pointer that points to
     * the kbhit() equivalent, in this TTY case nttty_kbhit()
     */
    nt_kbhit = nttty_kbhit;

    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, TRUE)) {
        /* Unable to set control handler */
        cmode = 0; /* just to have a statement to break on for debugger */
    }

    LI = console.height;
    CO = console.width;

    really_move_cursor();
}

int
process_keystroke(ir, valid, numberpad, portdebug)
INPUT_RECORD *ir;
boolean *valid;
boolean numberpad;
int portdebug;
{
    int ch = keyboard_handler.pProcessKeystroke(
                    console.hConIn, ir, valid, numberpad, portdebug);
    /* check for override */
    if (ch && ch < MAX_OVERRIDES && key_overrides[ch])
        ch = key_overrides[ch];
    return ch;
}

int
nttty_kbhit()
{
    return keyboard_handler.pNHkbhit(console.hConIn, &ir);
}

int
tgetch()
{
    int mod;
    coord cc;
    DWORD count;
    really_move_cursor();
    if (iflags.debug_fuzzer)
        return randomkey();
    return (program_state.done_hup)
               ? '\033'
               : keyboard_handler.pCheckInput(
                   console.hConIn, &ir, &count, iflags.num_pad, 0, &mod, &cc);
}

int
ntposkey(x, y, mod)
int *x, *y, *mod;
{
    int ch;
    coord cc;
    DWORD count;
    really_move_cursor();
    if (iflags.debug_fuzzer)
        return randomkey();
    ch = (program_state.done_hup)
             ? '\033'
             : keyboard_handler.pCheckInput(
                   console.hConIn, &ir, &count, iflags.num_pad, 1, mod, &cc);
    if (!ch) {
        *x = cc.x;
        *y = cc.y;
    }
    return ch;
}

static void set_console_cursor(int x, int y)
{
    ntassert(x >= 0 && x < console.width);
    ntassert(y >= 0 && y < console.height);

    console.cursor.X = max(0, min(console.width - 1, x));
    console.cursor.Y = max(0, min(console.height - 1, y));
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
                ttyDisplay->curx, ttyDisplay->cury,
                console.cursor.X, console.cursor.Y);
        (void) SetConsoleTitle(newtitle);
    }
#endif
    if (ttyDisplay)
        set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);

    back_buffer_flip();
    SetConsoleCursorPosition(console.hConOut, console.cursor);
}

void
cmov(x, y)
register int x, y;
{
    ttyDisplay->cury = y;
    ttyDisplay->curx = x;

    set_console_cursor(x, y);
}

void
nocmov(x, y)
int x, y;
{
    ttyDisplay->curx = x;
    ttyDisplay->cury = y;

    set_console_cursor(x, y);
}

void
xputc(ch)
char ch;
{
    set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
    xputc_core(ch);
}

void
xputs(s)
const char *s;
{
    int k;
    int slen = strlen(s);

    if (ttyDisplay)
        set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);

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
    ntassert(console.cursor.X >= 0 && console.cursor.X < console.width);
    ntassert(console.cursor.Y >= 0 && console.cursor.Y < console.height);

    boolean inverse = FALSE;
    cell_t cell;

    switch (ch) {
    case '\n':
        if (console.cursor.Y < console.height - 1)
            console.cursor.Y++;
    /* fall through */
    case '\r':
        console.cursor.X = 1;
        break;
    case '\b':
        if (console.cursor.X > 1) {
            console.cursor.X--;
        } else if(console.cursor.Y > 0) {
            console.cursor.X = console.width - 1;
            console.cursor.Y--;
        }
        break;
    default:
        inverse = (console.current_nhattr[ATR_INVERSE] && iflags.wc_inverse);
        console.attr = (inverse) ?
                        ttycolors_inv[console.current_nhcolor] :
                        ttycolors[console.current_nhcolor];
        if (console.current_nhattr[ATR_BOLD])
                console.attr |= (inverse) ?
                                BACKGROUND_INTENSITY : FOREGROUND_INTENSITY;

        cell.attribute = console.attr;
        cell.character = (console.has_unicode ? console.cpMap[ch] : ch);

        buffer_write(console.back_buffer, &cell, console.cursor);

        if (console.cursor.X == console.width - 1) {
            if (console.cursor.Y < console.height - 1) {
                console.cursor.X = 1;
                console.cursor.Y++;
            }
        } else {
            console.cursor.X++;
        }
    }

    ntassert(console.cursor.X >= 0 && console.cursor.X < console.width);
    ntassert(console.cursor.Y >= 0 && console.cursor.Y < console.height);
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

    set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);

    inverse = (console.current_nhattr[ATR_INVERSE] && iflags.wc_inverse);
    console.attr = (console.current_nhattr[ATR_INVERSE] && iflags.wc_inverse) ?
                    ttycolors_inv[console.current_nhcolor] :
                    ttycolors[console.current_nhcolor];
    if (console.current_nhattr[ATR_BOLD])
        console.attr |= (inverse) ? BACKGROUND_INTENSITY : FOREGROUND_INTENSITY;

    cell_t cell;

    cell.attribute = console.attr;
    cell.character = (console.has_unicode ? cp437[ch] : ch);

    buffer_write(console.back_buffer, &cell, console.cursor);
}

void
cl_end()
{
    set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
    buffer_clear_to_end_of_line(console.back_buffer, console.cursor.X,
                                console.cursor.Y);
    tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1, (int) ttyDisplay->cury);
}

void
raw_clear_screen()
{
    buffer_fill_to_end(console.back_buffer, &clear_cell, 0, 0);
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
    ttyDisplay->curx = ttyDisplay->cury = 0;
    set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
}

void
backsp()
{
    set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
    xputc_core('\b');
}

void
cl_eos()
{
    buffer_fill_to_end(console.back_buffer, &clear_cell, ttyDisplay->curx,
                        ttyDisplay->cury);
    tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1, (int) ttyDisplay->cury);
}

void
tty_nhbell()
{
    if (flags.silent || iflags.debug_fuzzer)
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

    if (iflags.debug_fuzzer)
        return;

    goal = 50 + clock();
    back_buffer_flip();
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
    GetConsoleMode(console.hConIn, &cmode);
    if (iflags.wc_mouse_support)
        cmode |= ENABLE_MOUSE_INPUT;
    else
        cmode &= ~ENABLE_MOUSE_INPUT;
    SetConsoleMode(console.hConIn, cmode);
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
        ReadConsoleInput(console.hConIn, &ir, 1, &count);
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
    if (!keyboard_handler.pSourceAuthor && !keyboard_handler.pSourceWhere)
        pline("Keyboard handler source info and author unavailable.");
    else {
        if (keyboard_handler.pKeyHandlerName &&
            keyboard_handler.pKeyHandlerName(&buf, 1)) {
            xputs("\n");
            xputs("Keystroke handler loaded: \n    ");
            xputs(buf);
        }
        if (keyboard_handler.pSourceAuthor &&
            keyboard_handler.pSourceAuthor(&buf)) {
            xputs("\n");
            xputs("Keystroke handler Author: \n    ");
            xputs(buf);
        }
        if (keyboard_handler.pSourceWhere &&
            keyboard_handler.pSourceWhere(&buf)) {
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

void unload_keyboard_handler()
{
    ntassert(keyboard_handler.hLibrary != NULL);

    FreeLibrary(keyboard_handler.hLibrary);
    memset(&keyboard_handler, 0, sizeof(keyboard_handler_t));
}

boolean
load_keyboard_handler(const char * inName)
{
    char path[MAX_ALTKEYHANDLER + 4];
    strcpy(path, inName);
    strcat(path, ".dll");

    HANDLE hLibrary = LoadLibrary(path);

    if (hLibrary == NULL)
        return FALSE;

    PROCESS_KEYSTROKE pProcessKeystroke = (PROCESS_KEYSTROKE) GetProcAddress(
        hLibrary, TEXT("ProcessKeystroke"));
    NHKBHIT pNHkbhit = (NHKBHIT) GetProcAddress(
        hLibrary, TEXT("NHkbhit"));
    CHECKINPUT pCheckInput =
        (CHECKINPUT) GetProcAddress(hLibrary, TEXT("CheckInput"));

    if (!pProcessKeystroke || !pNHkbhit || !pCheckInput)
    {
        return FALSE;
    } else {
        if (keyboard_handler.hLibrary != NULL)
            unload_keyboard_handler();

        keyboard_handler.hLibrary = hLibrary;

        keyboard_handler.pProcessKeystroke = pProcessKeystroke;
        keyboard_handler.pNHkbhit = pNHkbhit;
        keyboard_handler.pCheckInput = pCheckInput;

        keyboard_handler.pSourceWhere =
            (SOURCEWHERE) GetProcAddress(hLibrary, TEXT("SourceWhere"));
        keyboard_handler.pSourceAuthor =
            (SOURCEAUTHOR) GetProcAddress(hLibrary, TEXT("SourceAuthor"));
        keyboard_handler.pKeyHandlerName = (KEYHANDLERNAME) GetProcAddress(
            hLibrary, TEXT("KeyHandlerName"));
    }

    return TRUE;
}

void set_altkeyhandler(const char * inName)
{
    if (strlen(inName) >= MAX_ALTKEYHANDLER) {
        config_error_add("altkeyhandler name '%s' is too long", inName);
        return;
    }

    char name[MAX_ALTKEYHANDLER];
    strcpy(name, inName);

    /* We support caller mistakenly giving name with '.dll' extension */
    char * ext = strchr(name, '.');
    if (ext != NULL) *ext = '\0';

    if (load_keyboard_handler(name))
        strcpy(iflags.altkeyhandler, name);
    else {
        config_error_add("unable to load altkeyhandler '%s'", name);
        return;
    }

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
    BOOL success = GetCurrentConsoleFontEx(console.hConOut, FALSE,
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
    BOOL success = GetCurrentConsoleFontEx(console.hConOut, FALSE,
                                            &console_font_info);

    console.font_changed = TRUE;
    console.original_font_info = console_font_info;
    console.original_code_page = GetConsoleOutputCP();

    wcscpy_s(console_font_info.FaceName,
        sizeof(console_font_info.FaceName)
            / sizeof(console_font_info.FaceName[0]),
        L"Consolas");

    success = SetConsoleOutputCP(437);
    ntassert(success);

    success = SetCurrentConsoleFontEx(console.hConOut, FALSE, &console_font_info);
    ntassert(success);
}

/* restore_original_console_font will restore the console font and code page
 * settings to what they were when NetHack was launched.
 */
void
restore_original_console_font()
{
    if (console.font_changed) {
        BOOL success;
        raw_print("Restoring original font and code page\n");
        success = SetConsoleOutputCP(console.original_code_page);
        if (!success)
            raw_print("Unable to restore original code page\n");

        success = SetCurrentConsoleFontEx(console.hConOut, FALSE,
                                            &console.original_font_info);
        if (!success)
            raw_print("Unable to restore original font\n");

        console.font_changed = FALSE;
    }
}

/* set_cp_map() creates a mapping of every possible character of a code
 * page to its corresponding WCHAR.  This is necessary due to the high
 * cost of making calls to MultiByteToWideChar() for every character we
 * wish to print to the console.
 */

void set_cp_map()
{
    if (console.has_unicode) {
        UINT codePage = GetConsoleOutputCP();

        for (int i = 0; i < 256; i++) {
            char c = (char)i;
            int count = MultiByteToWideChar(codePage, 0, &c, 1,
                                            &console.cpMap[i], 1);
            ntassert(count == 1);
        }
    }
}

/* early_raw_print() is used during early game intialization prior to the
 * setting up of the windowing system.  This allows early errors and panics
 * to have there messages displayed.
 *
 * early_raw_print() eventually gets replaced by tty_raw_print().
 *
 */

void early_raw_print(const char *s)
{
    if (console.hConOut == NULL)
        return;

    ntassert(console.cursor.X >= 0 && console.cursor.X < console.width);
    ntassert(console.cursor.Y >= 0 && console.cursor.Y < console.height);

    WORD attribute = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE;
    DWORD unused;

    while (*s != '\0') {
        switch (*s) {
        case '\n':
            if (console.cursor.Y < console.height - 1)
                console.cursor.Y++;
        /* fall through */
        case '\r':
            console.cursor.X = 1;
            break;
        case '\b':
            if (console.cursor.X > 1) {
                console.cursor.X--;
            } else if(console.cursor.Y > 0) {
                console.cursor.X = console.width - 1;
                console.cursor.Y--;
            }
            break;
        default:
            WriteConsoleOutputAttribute(console.hConOut, &attribute,
                                            1, console.cursor, &unused);
            WriteConsoleOutputCharacterA(console.hConOut, s,
                                            1, console.cursor, &unused);
            if (console.cursor.X == console.width - 1) {
                if (console.cursor.Y < console.height - 1) {
                    console.cursor.X = 1;
                    console.cursor.Y++;
                }
            } else {
                console.cursor.X++;
            }
        }
        s++;
    }

    ntassert(console.cursor.X >= 0 && console.cursor.X < console.width);
    ntassert(console.cursor.Y >= 0 && console.cursor.Y < console.height);

    SetConsoleCursorPosition(console.hConOut, console.cursor);

}

/* nethack_enter_nttty() is the first thing that is called from main.
 *
 * We initialize all console state to support rendering to the console
 * through out flipping support at this time.  This allows us to support
 * raw_print prior to our returning.
 *
 * During this early initialization, we also determine the width and
 * height of the console that will be used.  This width and height will
 * not later change.
 *
 * We also check and set the console font to a font that we know will work
 * well with nethack.
 *
 * The intent of this early initialization is to get all state that is
 * not dependent upon game options initialized allowing us to simplify
 * any additional initialization that might be needed when we are actually
 * asked to open.
 *
 * Other then the call below which clears the entire console buffer, no
 * other code outputs directly to the console other then the code that
 * handles flipping the back buffer.
 *
 */

void nethack_enter_nttty()
{
    /* set up state needed by early_raw_print() */
    windowprocs.win_raw_print = early_raw_print;

    console.hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    ntassert(console.hConOut != NULL); // NOTE: this assert will not print

    GetConsoleScreenBufferInfo(console.hConOut, &console.origcsbi);

    /* Testing of widths != COLNO has not turned up any problems.  Need
     * to do a bit more testing and then we are likely to enable having
     * console width match window width.
     */
#if 0
    console.width = console.origcsbi.srWindow.Right -
                     console.origcsbi.srWindow.Left + 1;
    console.Width = max(console.Width, COLNO);
#else
    console.width = COLNO;
#endif

    console.height = console.origcsbi.srWindow.Bottom -
                     console.origcsbi.srWindow.Top + 1;
    console.height = max(console.height, ROWNO + 3);

    console.buffer_size = console.width * console.height;


    /* clear the entire console buffer */
    int size = console.origcsbi.dwSize.X * console.origcsbi.dwSize.Y;
    DWORD unused;
    set_console_cursor(0, 0);
    FillConsoleOutputAttribute(
        console.hConOut, CONSOLE_CLEAR_ATTRIBUTE,
        size, console.cursor, &unused);

    FillConsoleOutputCharacter(
        console.hConOut, CONSOLE_CLEAR_CHARACTER,
        size, console.cursor, &unused);

    set_console_cursor(1, 0);
    SetConsoleCursorPosition(console.hConOut, console.cursor);

    /* At this point early_raw_print will work */

    console.hConIn = GetStdHandle(STD_INPUT_HANDLE);
    ntassert(console.hConIn  != NULL);

    /* grow the size of the console buffer if it is not wide enough */
    if (console.origcsbi.dwSize.X < console.width) {
        COORD size = {
            size.Y = console.origcsbi.dwSize.Y,
            size.X = console.width
        };

        SetConsoleScreenBufferSize(console.hConOut, size);
    }

    /* setup front and back buffers */
    int buffer_size_bytes = sizeof(cell_t) * console.buffer_size;

    console.front_buffer = (cell_t *)malloc(buffer_size_bytes);
    buffer_fill_to_end(console.front_buffer, &undefined_cell, 0, 0);

    console.back_buffer = (cell_t *)malloc(buffer_size_bytes);
    buffer_fill_to_end(console.back_buffer, &clear_cell, 0, 0);

    /* determine whether OS version has unicode support */
    console.has_unicode = ((GetVersion() & 0x80000000) == 0);

    /* check the font before we capture the code page map */
    check_and_set_font();
    set_cp_map();

    /* Set console mode */
    DWORD cmode, mask;
    GetConsoleMode(console.hConIn, &cmode);
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
    SetConsoleMode(console.hConIn, cmode);

    /* load default keyboard handler */
    HKL keyboard_layout = GetKeyboardLayout(0);
    DWORD primary_language = (UINT_PTR) keyboard_layout & 0x3f;

    if (primary_language == LANG_ENGLISH) {
        if (!load_keyboard_handler("nhdefkey"))
            error("Unable to load nhdefkey.dll");
    } else {
        if (!load_keyboard_handler("nhraykey"))
            error("Unable to load nhraykey.dll");
    }
}
#endif TTY_GRAPHICS

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
#ifdef TTY_GRAPHICS
        if(!init_ttycolor_completed)
            init_ttycolor();
        /* if we have generated too many messages ... ask the user to
         * confirm and then clear.
         */
        if (console.cursor.Y > console.height - 4) {
            xputs("Hit <Enter> to continue.");
            while (pgetchar() != '\n')
                ;
            raw_clear_screen();
            set_console_cursor(1, 0);
        }
        xputs(buf);
        if (ttyDisplay)
            curs(BASE_WINDOW, console.cursor.X + 1, console.cursor.Y);
#else
        fprintf(stdout, "%s", buf);
#endif
    }
    VA_END();
    return;
}

#endif /* WIN32 */

/* NetHack 3.6	nttty.c	$NHDT-Date: 1554215932 2019/04/02 14:38:52 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.99 $ */
/* Copyright (c) NetHack PC Development Team 1993    */
/* NetHack may be freely redistributed.  See license for details. */

/* tty.c - (Windows NT) version */

/*
 * Initial Creation 				M. Allison	1993/01/31
 * Switch to low level console output routines	M. Allison	2003/10/01
 * Restrict cursor movement until input pending	M. Lehotay	2003/10/02
 * Call Unicode version of output API on NT     R. Chason	2005/10/28
 * Use of back buffer to improve performance    B. House	2018/05/06
 *
 */

#ifdef WIN32
#define NEED_VARARGS /* Uses ... */
#include "win32api.h"
#include "winos.h"
#include "hack.h"
#include "wintty.h"
#include <sys\types.h>
#include <sys\stat.h>

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
extern void NDECL(safe_routines);

/* Win32 Screen buffer,coordinate,console I/O information */
COORD ntcoord;
INPUT_RECORD ir;
static boolean orig_QuickEdit;

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

int default_processkeystroke(HANDLE, INPUT_RECORD *, boolean *, boolean, int);
int default_kbhit(HANDLE, INPUT_RECORD *);
int default_checkinput(HANDLE, INPUT_RECORD *, DWORD *, boolean,
                       int, int *, coord *);

int ray_processkeystroke(HANDLE, INPUT_RECORD *, boolean *, boolean, int);
int ray_kbhit(HANDLE, INPUT_RECORD *);
int ray_checkinput(HANDLE, INPUT_RECORD *, DWORD *, boolean,
                       int, int *, coord *);

int nh340_processkeystroke(HANDLE, INPUT_RECORD *, boolean *, boolean, int);
int nh340_kbhit(HANDLE, INPUT_RECORD *);
int nh340_checkinput(HANDLE, INPUT_RECORD *, DWORD *, boolean,
                       int, int *, coord *);

struct keyboard_handling_t {
    char *pKeyHandlingName;
    int (*pProcessKeystroke)(HANDLE, INPUT_RECORD *, boolean *,
                                          boolean, int);
    int (*pNHkbhit)(HANDLE, INPUT_RECORD *);
    int (*pCheckInput)(HANDLE, INPUT_RECORD *, DWORD *, boolean,
                                   int, int *, coord *);
} keyboard_handling = {
    (char *) no_keyhandling,
    default_processkeystroke,
    default_kbhit,
    default_checkinput
};

static INPUT_RECORD bogus_key;


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
    nhassert(x >= 0 && x < console.width);
    nhassert(y >= 0 && ((y < console.height) || (y == console.height &&
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
    nhassert(x >= 0 && x < console.width);
    nhassert(y >= 0 && ((y < console.height) || (y == console.height &&
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
    nhassert(pos.X >= 0 && pos.X < console.width);
    nhassert(pos.Y >= 0 && pos.Y < console.height);

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
    if (orig_QuickEdit) {
        DWORD cmode;

        GetConsoleMode(console.hConIn, &cmode);
        cmode |= (ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS);
        SetConsoleMode(console.hConIn, cmode);
    }
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

void
nttty_exit()
{
    /* go back to using the safe routines */
    safe_routines();
}

int
process_keystroke(ir, valid, numberpad, portdebug)
INPUT_RECORD *ir;
boolean *valid;
boolean numberpad;
int portdebug;
{
    int ch;

#ifdef QWERTZ_SUPPORT
    if (Cmd.swap_yz)
        numberpad |= 0x10;
#endif
    ch = keyboard_handling.pProcessKeystroke(
                    console.hConIn, ir, valid, numberpad, portdebug);
#ifdef QWERTZ_SUPPORT
    numberpad &= ~0x10;
#endif
    /* check for override */
    if (ch && ch < MAX_OVERRIDES && key_overrides[ch])
        ch = key_overrides[ch];
    return ch;
}

int
nttty_kbhit()
{
    return keyboard_handling.pNHkbhit(console.hConIn, &ir);
}

int
tgetch()
{
    int mod;
    coord cc;
    DWORD count;
    boolean numpad = iflags.num_pad;

    really_move_cursor();
    if (iflags.debug_fuzzer)
        return randomkey();
#ifdef QWERTZ_SUPPORT
    if (Cmd.swap_yz)
        numpad |= 0x10;
#endif

    return (program_state.done_hup)
               ? '\033'
               : keyboard_handling.pCheckInput(
                   console.hConIn, &ir, &count, numpad, 0, &mod, &cc);
}

int
ntposkey(x, y, mod)
int *x, *y, *mod;
{
    int ch;
    coord cc;
    DWORD count;
    boolean numpad = iflags.num_pad;

    really_move_cursor();
    if (iflags.debug_fuzzer)
        return randomkey();
#ifdef QWERTZ_SUPPORT
    if (Cmd.swap_yz)
        numpad |= 0x10;
#endif
    ch = (program_state.done_hup)
             ? '\033'
             : keyboard_handling.pCheckInput(
                   console.hConIn, &ir, &count, numpad, 1, mod, &cc);
#ifdef QWERTZ_SUPPORT
    numpad &= ~0x10;
#endif
    if (!ch) {
        *x = cc.x;
        *y = cc.y;
    }
    return ch;
}

static void set_console_cursor(int x, int y)
{
    nhassert(x >= 0 && x < console.width);
    nhassert(y >= 0 && y < console.height);

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

/* same signature as 'putchar()' with potential failure result ignored */
int
xputc(ch)
int ch;
{
    set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
    xputc_core((char) ch);
    return 0;
}

void
xputs(s)
const char *s;
{
    int k;
    int slen = (int) strlen(s);

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
    nhassert(console.cursor.X >= 0 && console.cursor.X < console.width);
    nhassert(console.cursor.Y >= 0 && console.cursor.Y < console.height);

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

    nhassert(console.cursor.X >= 0 && console.cursor.X < console.width);
    nhassert(console.cursor.Y >= 0 && console.cursor.Y < console.height);
}

/*
 * Overrides wintty.c function of the same name
 * for win32. It is used for glyphs only, not text.
 */

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
    if (WINDOWPORT("tty")) {
        cell_t * back = console.back_buffer;
        cell_t * front = console.front_buffer;
        COORD pos;
        DWORD unused;

        for (pos.Y = 0; pos.Y < console.height; pos.Y++) {
            for (pos.X = 0; pos.X < console.width; pos.X++) {
                 WriteConsoleOutputAttribute(console.hConOut, &back->attribute,
                                             1, pos, &unused);
                 front->attribute = back->attribute;
                 if (console.has_unicode) {
                     WriteConsoleOutputCharacterW(console.hConOut,
                             &back->character, 1, pos, &unused);
                 } else {
                     char ch = (char)back->character;
                     WriteConsoleOutputCharacterA(console.hConOut, &ch, 1, pos,
                                                         &unused);
                 }
                 *front = *back;
                 back++;
                 front++;
            }
        }
    }
}

void
clear_screen()
{
    buffer_fill_to_end(console.back_buffer, &clear_cell, 0, 0);
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

    goal = 50 + clock();
    back_buffer_flip();
    if (iflags.debug_fuzzer)
        return;

    while (goal > clock()) {
        k = junk; /* Do nothing */
    }
}

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

#if 0
int
has_color(int color)        /* this function is commented out */
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
#endif

int
term_attr_fixup(int attrmask)
{
    return attrmask;
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
    static int qeinit = 0;
    DWORD cmode;

    GetConsoleMode(console.hConIn, &cmode);
    if (!qeinit) {
        qeinit = 1;
        orig_QuickEdit = ((cmode & ENABLE_QUICK_EDIT_MODE) != 0);
    }
    switch(iflags.wc_mouse_support) {
        case 2:
                cmode |= ENABLE_MOUSE_INPUT;
                break;
        case 1:
                cmode |= ENABLE_MOUSE_INPUT;
                cmode &= ~ENABLE_QUICK_EDIT_MODE;
                cmode |= ENABLE_EXTENDED_FLAGS;
                break;
        case 0:
                /*FALLTHRU*/
        default:
                cmode &= ~ENABLE_MOUSE_INPUT;
                if (orig_QuickEdit)
                    cmode |= (ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS);
    }
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
    (void) vsnprintf(&buf[1], sizeof buf - 1, s, VA_ARGS);
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
        used[primary_syms[i]] = TRUE;
        used[rogue_syms[i]] = TRUE;
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
    nhassert(success);

    success = SetCurrentConsoleFontEx(console.hConOut, FALSE, &console_font_info);
    nhassert(success);
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

        if (codePage == 437) {
            memcpy(console.cpMap, cp437, sizeof(console.cpMap));
        } else {
            for (int i = 0; i < 256; i++) {
                char c = (char)i;
                int count = MultiByteToWideChar(codePage, 0, &c, 1,
                                                &console.cpMap[i], 1);
                nhassert(count == 1);

                // If a character was mapped to unicode control codes,
                // remap to the appropriate unicode character per our
                // code page 437 mappings.
                if (console.cpMap[i] < 32)
                    console.cpMap[i] = cp437[console.cpMap[i]];
            }
        }

    }
}

#if 0
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

    nhassert(console.cursor.X >= 0 && console.cursor.X < console.width);
    nhassert(console.cursor.Y >= 0 && console.cursor.Y < console.height);

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

    nhassert(console.cursor.X >= 0 && console.cursor.X < console.width);
    nhassert(console.cursor.Y >= 0 && console.cursor.Y < console.height);

    SetConsoleCursorPosition(console.hConOut, console.cursor);

}
#endif

/* nethack_enter_nttty() is the first thing that is called from main
 * once the tty port is confirmed.
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
#if 0
    /* set up state needed by early_raw_print() */
    windowprocs.win_raw_print = early_raw_print;
#endif
    console.hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    nhassert(console.hConOut != NULL); // NOTE: this assert will not print

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
    nhassert(console.hConIn  != NULL);

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

    /* This was overriding the handler that had already
       been loaded during options parsing. Needs to
       check first */
    if (iflags.key_handling == no_keyhandling) {
        if (primary_language == LANG_ENGLISH) {
            set_altkeyhandling("default");
        } else {
            set_altkeyhandling("ray");
        }
    }
}
#endif /* TTY_GRAPHICS */

/* this is used as a printf() replacement when the window
 * system isn't initialized yet
 */
void msmsg
VA_DECL(const char *, fmt)
{
    char buf[ROWNO * COLNO]; /* worst case scenario */
    VA_START(fmt);
    VA_INIT(fmt, const char *);
    (void) vsnprintf(buf, sizeof buf, fmt, VA_ARGS);
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


/*
 *  Keyboard translation tables.
 *  (Adopted from the MSDOS port)
 */

#define KEYPADLO 0x47
#define KEYPADHI 0x53

#define PADKEYS (KEYPADHI - KEYPADLO + 1)
#define iskeypad(x) (KEYPADLO <= (x) && (x) <= KEYPADHI)
#define isnumkeypad(x) \
    (KEYPADLO <= (x) && (x) <= 0x51 && (x) != 0x4A && (x) != 0x4E)

#ifdef QWERTZ_SUPPORT
/* when 'numberpad' is 0 and Cmd.swap_yz is True
   (signaled by setting 0x10 on boolean numpad argument)
   treat keypress of numpad 7 as 'z' rather than 'y' */
static boolean qwertz = FALSE;
#endif
#define inmap(x, vk) (((x) > 'A' && (x) < 'Z') || (vk) == 0xBF || (x) == '2')

struct pad {
    uchar normal, shift, cntrl;
};

/*
 * default key handling
 *
 * This is the default NetHack keystroke processing.
 * Use the .nethackrc "altkeyhandling" option to set a
 * different handling type.
 *
 */
/*
 * Keypad keys are translated to the normal values below.
 * Shifted keypad keys are translated to the
 *    shift values below.
 */

static const struct pad default_keypad[PADKEYS] = {
      { 'y', 'Y', C('y') },    /* 7 */
      { 'k', 'K', C('k') },    /* 8 */
      { 'u', 'U', C('u') },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { 'h', 'H', C('h') },    /* 4 */
      { 'g', 'G', 'g' },       /* 5 */
      { 'l', 'L', C('l') },    /* 6 */
      { '+', 'P', C('p') },    /* + */
      { 'b', 'B', C('b') },    /* 1 */
      { 'j', 'J', C('j') },    /* 2 */
      { 'n', 'N', C('n') },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
}, default_numpad[PADKEYS] = {
      { '7', M('7'), '7' },    /* 7 */
      { '8', M('8'), '8' },    /* 8 */
      { '9', M('9'), '9' },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { '4', M('4'), '4' },    /* 4 */
      { '5', M('5'), '5' },    /* 5 */
      { '6', M('6'), '6' },    /* 6 */
      { '+', 'P', C('p') },    /* + */
      { '1', M('1'), '1' },    /* 1 */
      { '2', M('2'), '2' },    /* 2 */
      { '3', M('3'), '3' },    /* 3 */
      { '0', M('0'), '0' },    /* Ins */
      { '.', ':', ':' }        /* Del */
};

/*
 * Keypad keys are translated to the normal values below.
 * Shifted keypad keys are translated to the
 *    shift values below.
 */

static const struct pad
ray_keypad[PADKEYS] = {
      { 'y', 'Y', C('y') },    /* 7 */
      { 'k', 'K', C('k') },    /* 8 */
      { 'u', 'U', C('u') },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { 'h', 'H', C('h') },    /* 4 */
      { 'g', 'G', 'g' },       /* 5 */
      { 'l', 'L', C('l') },    /* 6 */
      { '+', 'P', C('p') },    /* + */
      { 'b', 'B', C('b') },    /* 1 */
      { 'j', 'J', C('j') },    /* 2 */
      { 'n', 'N', C('n') },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
},
ray_numpad[PADKEYS] = {
      { '7', M('7'), '7' },    /* 7 */
      { '8', M('8'), '8' },    /* 8 */
      { '9', M('9'), '9' },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { '4', M('4'), '4' },    /* 4 */
      { 'g', 'G', 'g' },       /* 5 */
      { '6', M('6'), '6' },    /* 6 */
      { '+', 'P', C('p') },    /* + */
      { '1', M('1'), '1' },    /* 1 */
      { '2', M('2'), '2' },    /* 2 */
      { '3', M('3'), '3' },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
};

static const struct pad
nh340_keypad[PADKEYS] = {
      { 'y', 'Y', C('y') },    /* 7 */
      { 'k', 'K', C('k') },    /* 8 */
      { 'u', 'U', C('u') },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { 'h', 'H', C('h') },    /* 4 */
      { 'g', 'G', 'g' },       /* 5 */
      { 'l', 'L', C('l') },    /* 6 */
      { '+', 'P', C('p') },    /* + */
      { 'b', 'B', C('b') },    /* 1 */
      { 'j', 'J', C('j') },    /* 2 */
      { 'n', 'N', C('n') },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
},
nh340_numpad[PADKEYS] = {
      { '7', M('7'), '7' },    /* 7 */
      { '8', M('8'), '8' },    /* 8 */
      { '9', M('9'), '9' },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { '4', M('4'), '4' },    /* 4 */
      { 'g', 'G', 'g' },       /* 5 */
      { '6', M('6'), '6' },    /* 6 */
      { '+', 'P', C('p') },    /* + */
      { '1', M('1'), '1' },    /* 1 */
      { '2', M('2'), '2' },    /* 2 */
      { '3', M('3'), '3' },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
};

static struct pad keypad[PADKEYS], numpad[PADKEYS];
static BYTE KeyState[256];
static const char default_name[] = "default";
const char *const legal_key_handling[] = {
    "none",
    "default",
    "ray",
    "340",
};
enum windows_key_handling keyh[] = { no_keyhandling, default_keyhandling, ray_keyhandling,
                                     nh340_keyhandling };

void set_altkeyhandling(const char *inName)
{
    int i, k;
    /*backward compatibility - so people's existing config files
      may work as is */
    if (!strcmpi(inName, "nhraykey.dll"))
        inName = legal_key_handling[ray_keyhandling];
    else if (!strcmpi(inName, "nh340key.dll"))
        inName = legal_key_handling[nh340_keyhandling];
    else if (!strcmpi(inName, "nhdefkey.dll"))
        inName = legal_key_handling[default_keyhandling];

    for (i = default_keyhandling; i < SIZE(legal_key_handling); i++) {
        if (!strcmpi(inName, legal_key_handling[i])) {
            iflags.key_handling = keyh[i];
            if (keyboard_handling.pKeyHandlingName) {
                free((genericptr_t) keyboard_handling.pKeyHandlingName);
                keyboard_handling.pKeyHandlingName = (char *) 0;
            }
            switch(iflags.key_handling) {
            case ray_keyhandling:
                keyboard_handling.pKeyHandlingName = strdup("ray");
                keyboard_handling.pProcessKeystroke = ray_processkeystroke;
                keyboard_handling.pNHkbhit = ray_kbhit;
                keyboard_handling.pCheckInput = ray_checkinput;
                /* A bogus key that will be filtered when received, to keep ReadConsole
                 * from blocking */
                bogus_key.EventType = KEY_EVENT;
                bogus_key.Event.KeyEvent.bKeyDown = 1;
                bogus_key.Event.KeyEvent.wRepeatCount = 1;
                bogus_key.Event.KeyEvent.wVirtualKeyCode = 0;
                bogus_key.Event.KeyEvent.wVirtualScanCode = 0;
                bogus_key.Event.KeyEvent.uChar.AsciiChar = (uchar) 0x80;
                bogus_key.Event.KeyEvent.dwControlKeyState = 0;
                for (k = 0; k < SIZE(keypad); ++k) {
                    keypad[k] = ray_keypad[k];
                    numpad[k] = ray_numpad[k];
                }
                break;
            case nh340_keyhandling:
                keyboard_handling.pKeyHandlingName = strdup("340");
                keyboard_handling.pProcessKeystroke = nh340_processkeystroke;
                keyboard_handling.pNHkbhit = nh340_kbhit;
                keyboard_handling.pCheckInput = nh340_checkinput;
                for (k = 0; k < SIZE(keypad); ++k) {
                    keypad[k] = nh340_keypad[k];
                    numpad[k] = nh340_numpad[k];
                }
                break;
            case default_keyhandling:
            default:
                keyboard_handling.pKeyHandlingName = strdup("default");
                keyboard_handling.pProcessKeystroke
                                            = default_processkeystroke;
                keyboard_handling.pNHkbhit = default_kbhit;
                keyboard_handling.pCheckInput = default_checkinput;
                for (k = 0; k < SIZE(keypad); ++k) {
                    keypad[k] = default_keypad[k];
                    numpad[k] = default_numpad[k];
                }
                break;
            }
            return;
        }
    }
    config_error_add("invalid altkeyhandling '%s'", inName);
    return;
}

int
set_keyhandling_via_option(void)
{
    winid tmpwin;
    anything any;
    int i;
    menu_item *console_key_handling_pick = (menu_item *) 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);
    any = zeroany;
    for (i = default_keyhandling; i < SIZE(legal_key_handling); i++) {
        any.a_int = i + 1;
        add_menu(tmpwin, 0, &any, 'a' + i,
                 0, ATR_NONE,
                 legal_key_handling[i], 0);
    }
    end_menu(tmpwin, "Select windows console key handling:");
    if (select_menu(tmpwin, PICK_ONE, &console_key_handling_pick) > 0) {
        iflags.key_handling = keyh[console_key_handling_pick->item.a_int - 1];
        free((genericptr_t) console_key_handling_pick);
    }
    destroy_nhwindow(tmpwin);
    set_altkeyhandling(legal_key_handling[iflags.key_handling]);
    return 1;       /* optn_ok */
}

int
default_processkeystroke(
    HANDLE hConIn,
    INPUT_RECORD* ir,
    boolean* valid,
    boolean numberpad,
    int portdebug)
{
    int k = 0;
    int keycode, vk;
    unsigned char ch, pre_ch;
    unsigned short int scan;
    unsigned long shiftstate;
    int altseq = 0;
    const struct pad *kpad;

#ifdef QWERTZ_SUPPORT
    if (numberpad & 0x10) {
        numberpad &= ~0x10;
        qwertz = TRUE;
    } else {
        qwertz = FALSE;
    }
#endif
    shiftstate = 0L;
    ch = pre_ch = ir->Event.KeyEvent.uChar.AsciiChar;
    scan = ir->Event.KeyEvent.wVirtualScanCode;
    vk = ir->Event.KeyEvent.wVirtualKeyCode;
    keycode = MapVirtualKey(vk, 2);
    shiftstate = ir->Event.KeyEvent.dwControlKeyState;
    KeyState[VK_SHIFT] = (shiftstate & SHIFT_PRESSED) ? 0x81 : 0;
    KeyState[VK_CONTROL] =
        (shiftstate & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) ? 0x81 : 0;
    KeyState[VK_CAPITAL] = (shiftstate & CAPSLOCK_ON) ? 0x81 : 0;

    if (shiftstate & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
        if (ch || inmap(keycode, vk))
            altseq = 1;
        else
            altseq = -1; /* invalid altseq */
    }
    if (ch || (iskeypad(scan)) || (altseq > 0))
        *valid = TRUE;
    /* if (!valid) return 0; */
    /*
     * shiftstate can be checked to see if various special
     * keys were pressed at the same time as the key.
     * Currently we are using the ALT & SHIFT & CONTROLS.
     *
     *           RIGHT_ALT_PRESSED, LEFT_ALT_PRESSED,
     *           RIGHT_CTRL_PRESSED, LEFT_CTRL_PRESSED,
     *           SHIFT_PRESSED,NUMLOCK_ON, SCROLLLOCK_ON,
     *           CAPSLOCK_ON, ENHANCED_KEY
     *
     * are all valid bit masks to use on shiftstate.
     * eg. (shiftstate & LEFT_CTRL_PRESSED) is true if the
     *      left control key was pressed with the keystroke.
     */
    if (iskeypad(scan)) {
        kpad = numberpad ? numpad : keypad;
        if (shiftstate & SHIFT_PRESSED) {
            ch = kpad[scan - KEYPADLO].shift;
        } else if (shiftstate & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
            ch = kpad[scan - KEYPADLO].cntrl;
        } else {
            ch = kpad[scan - KEYPADLO].normal;
        }
#ifdef QWERTZ_SUPPORT
        /* OPTIONS=number_pad:-1 is for qwertz keyboard; for that setting,
           'numberpad' will be 0; core swaps y to zap, z to move northwest;
           we want numpad 7 to move northwest, so when qwertz is set,
           tell core that user who types numpad 7 typed z rather than y */
        if (qwertz && kpad[scan - KEYPADLO].normal == 'y')
            ch += 1; /* changes y to z, Y to Z, ^Y to ^Z */
#endif /*QWERTZ_SUPPORT*/
    } else if (altseq > 0) { /* ALT sequence */
        if (vk == 0xBF)
            ch = M('?');
        else
            ch = M(tolower((uchar) keycode));
    }
    /* Attempt to work better with international keyboards. */
    else {
        WORD chr[2];
        k = ToAscii(vk, scan, KeyState, chr, 0);
        if (k <= 2)
            switch (k) {
            case 2: /* two characters */
                ch = (unsigned char) chr[1];
                *valid = TRUE;
                break;
            case 1: /* one character */
                ch = (unsigned char) chr[0];
                *valid = TRUE;
                break;
            case 0:  /* no translation */
            default: /* negative */
                *valid = FALSE;
            }
    }
    if (ch == '\r')
        ch = '\n';
#ifdef PORT_DEBUG
    if (portdebug) {
        char buf[BUFSZ];
        Sprintf(buf, "PORTDEBUG (%s): ch=%u, sc=%u, vk=%d, pre=%d, sh=0x%lX, "
                     "ta=%d (ESC to end)",
                "default", ch, scan, vk, pre_ch, shiftstate, k);
        fprintf(stdout, "\n%s", buf);
    }
#endif
    return ch;
}

int
default_kbhit(HANDLE hConIn, INPUT_RECORD *ir)
{
    int done = 0; /* true =  "stop searching"        */
    int retval;   /* true =  "we had a match"        */
    DWORD count;
    unsigned short int scan;
    unsigned char ch;
    unsigned long shiftstate;
    int altseq = 0, keycode, vk;
    done = 0;
    retval = 0;
    while (!done) {
        count = 0;
        PeekConsoleInput(hConIn, ir, 1, &count);
        if (count > 0) {
            if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.bKeyDown) {
                ch = ir->Event.KeyEvent.uChar.AsciiChar;
                scan = ir->Event.KeyEvent.wVirtualScanCode;
                shiftstate = ir->Event.KeyEvent.dwControlKeyState;
                vk = ir->Event.KeyEvent.wVirtualKeyCode;
                keycode = MapVirtualKey(vk, 2);
                if (shiftstate & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
                    if (ch || inmap(keycode, vk))
                        altseq = 1;
                    else
                        altseq = -1; /* invalid altseq */
                }
                if (ch || iskeypad(scan) || altseq) {
                    done = 1;   /* Stop looking         */
                    retval = 1; /* Found what we sought */
                } else {
                    /* Strange Key event; let's purge it to avoid trouble */
                    ReadConsoleInput(hConIn, ir, 1, &count);
                }

            } else if ((ir->EventType == MOUSE_EVENT
                        && (ir->Event.MouseEvent.dwButtonState
                            & MOUSEMASK))) {
                done = 1;
                retval = 1;
            }

            else /* Discard it, it's an insignificant event */
                ReadConsoleInput(hConIn, ir, 1, &count);
        } else /* There are no events in console event queue */ {
            done = 1; /* Stop looking               */
            retval = 0;
        }
    }
    return retval;
}

int
default_checkinput(
    HANDLE hConIn,
    INPUT_RECORD *ir,
    DWORD* count,
    boolean numpad,
    int mode,
    int *mod,
    coord *cc)
{
#if defined(SAFERHANGUP)
    DWORD dwWait;
#endif
    int ch = 0;
    boolean valid = 0, done = 0;

#ifdef QWERTZ_SUPPORT
    if (numpad & 0x10) {
        numpad &= ~0x10;
        qwertz = TRUE;
    } else {
        qwertz = FALSE;
    }
#endif
    while (!done) {
#if defined(SAFERHANGUP)
        dwWait = WaitForSingleObjectEx(hConIn,   // event object to wait for
                                       INFINITE, // waits indefinitely
                                       TRUE);    // alertable wait enabled
        if (dwWait == WAIT_FAILED)
            return '\033';
#endif
        ReadConsoleInput(hConIn, ir, 1, count);
        if (mode == 0) {
            if ((ir->EventType == KEY_EVENT) && ir->Event.KeyEvent.bKeyDown) {
                ch = default_processkeystroke(hConIn, ir, &valid, numpad, 0);
                done = valid;
            }
        } else {
            if (count > 0) {
                if (ir->EventType == KEY_EVENT
                    && ir->Event.KeyEvent.bKeyDown) {
#ifdef QWERTZ_SUPPORT
                    if (qwertz)
                        numpad |= 0x10;
#endif
                    ch = default_processkeystroke(hConIn, ir, &valid, numpad, 0);
#ifdef QWERTZ_SUPPORT
                    numpad &= ~0x10;
#endif
                    if (valid)
                        return ch;
                } else if (ir->EventType == MOUSE_EVENT) {
                    if ((ir->Event.MouseEvent.dwEventFlags == 0)
                        && (ir->Event.MouseEvent.dwButtonState & MOUSEMASK)) {
                        cc->x = ir->Event.MouseEvent.dwMousePosition.X + 1;
                        cc->y = ir->Event.MouseEvent.dwMousePosition.Y - 1;

                        if (ir->Event.MouseEvent.dwButtonState & LEFTBUTTON)
                            *mod = CLICK_1;
                        else if (ir->Event.MouseEvent.dwButtonState
                                 & RIGHTBUTTON)
                            *mod = CLICK_2;
#if 0 /* middle button */
				    else if (ir->Event.MouseEvent.dwButtonState & MIDBUTTON)
			      		*mod = CLICK_3;
#endif
                        return 0;
                    }
                }
            } else
                done = 1;
        }
    }
    return mode ? 0 : ch;
}

/*
 * Keystroke handling contributed by Ray Chason.
 * The following text was written by Ray Chason.
 *
 * The problem
 * ===========
 *
 * The console-mode Nethack wants both keyboard and mouse input.  The
 * problem is that the Windows API provides no easy way to get mouse input
 * and also keyboard input properly translated according to the user's
 * chosen keyboard layout.
 *
 * The ReadConsoleInput function returns a stream of keyboard and mouse
 * events.  Nethack is interested in those events that represent a key
 * pressed, or a click on a mouse button.  The keyboard events from
 * ReadConsoleInput are not translated according to the keyboard layout,
 * and do not take into account the shift, control, or alt keys.
 *
 * The PeekConsoleInput function works similarly to ReadConsoleInput,
 * except that it does not remove an event from the queue and it returns
 * instead of blocking when the queue is empty.
 *
 * A program can also use ReadConsole to get a properly translated stream
 * of characters.  Unfortunately, ReadConsole does not return mouse events,
 * does not distinguish the keypad from the main keyboard, does not return
 * keys shifted with Alt, and does not even return the ESC key when
 * pressed.
 *
 * We want both the functionality of ReadConsole and the functionality of
 * ReadConsoleInput.  But Microsoft didn't seem to think of that.
 *
 *
 * The solution, in the original code
 * ==================================
 *
 * The original 3.4.1 distribution tries to get proper keyboard translation
 * by passing keyboard events to the ToAscii function.  This works, to some
 * extent -- it takes the shift key into account, and it processes dead
 * keys properly.  But it doesn't take non-US keyboards into account.  It
 * appears that ToAscii is meant for windowed applications, and does not
 * have enough information to do its job properly in a console application.
 *
 *
 * The Finnish keyboard patch
 * ==========================
 *
 * This patch adds the "subkeyvalue" option to the defaults.nh file.  The
 * user can then add OPTIONS=sukeyvalue:171/92, for instance, to replace
 * the 171 character with 92, which is \.  This works, once properly
 * configured, but places too much burden on the user.  It also bars the
 * use of the substituted characters in naming objects or monsters.
 *
 *
 * The solution presented here
 * ===========================
 *
 * The best way I could find to combine the functionality of ReadConsole
 * with that of ReadConsoleInput is simple in concept.  First, call
 * PeekConsoleInput to get the first event.  If it represents a key press,
 * call ReadConsole to retrieve the key.  Otherwise, pop it off the queue
 * with ReadConsoleInput and, if it's a mouse click, return it as such.
 *
 * But the Devil, as they say, is in the details.  The problem is in
 * recognizing an event that ReadConsole will return as a key.  We don't
 * want to call ReadConsole unless we know that it will immediately return:
 * if it blocks, the mouse and the Alt sequences will cease to function
 * until it returns.
 *
 * Separating process_keystroke into two functions, one for commands and a
 * new one, process_keystroke2, for answering prompts, makes the job a lot
 * easier.  process_keystroke2 doesn't have to worry about mouse events or
 * Alt sequences, and so the consequences are minor if ReadConsole blocks.
 * process_keystroke, OTOH, never needs to return a non-ASCII character
 * that was read from ReadConsole; it returns bytes with the high bit set
 * only in response to an Alt sequence.
 *
 * So in process_keystroke, before calling ReadConsole, a bogus key event
 * is pushed on the queue.  This event causes ReadConsole to return, even
 * if there was no other character available.  Because the bogus key has
 * the eighth bit set, it is filtered out.  This is not done in
 * process_keystroke2, because that would render dead keys unusable.
 *
 * A separate process_keystroke2 can also process the numeric keypad in a
 * way that makes sense for prompts:  just return the corresponding symbol,
 * and pay no mind to number_pad or the num lock key.
 *
 * The recognition of Alt sequences is modified, to support the use of
 * characters generated with the AltGr key.  A keystroke is an Alt sequence
 * if an Alt key is seen that can't be an AltGr (since an AltGr sequence
 * could be a character, and in some layouts it could even be an ASCII
 * character).  This recognition is different on NT-based and 95-based
 * Windows:
 *
 *    * On NT-based Windows, AltGr signals as right Alt and left Ctrl
 *      together.  So an Alt sequence is recognized if either Alt key is
 *      pressed and if right Alt and left Ctrl are not both present.  This
 *      is true even if the keyboard in use does not have an AltGr key, and
 *      uses right Alt for AltGr.
 *
 *    * On 95-based Windows, with a keyboard that lacks the AltGr key, the
 *      right Alt key is used instead.  But it still signals as right Alt,
 *      without left Ctrl.  There is no way for the application to know
 *      whether right Alt is Alt or AltGr, and so it is always assumed
 *      to be AltGr.  This means that Alt sequences must be formed with
 *      left Alt.
 *
 * So the patch processes keystrokes as follows:
 *
 *     * If the scan and virtual key codes are both 0, it's the bogus key,
 *       and we ignore it.
 *
 *     * Keys on the numeric keypad are processed for commands as in the
 *       unpatched Nethack, and for prompts by returning the ASCII
 *       character, even if the num lock is off.
 *
 *     * Alt sequences are processed for commands as in the unpatched
 *       Nethack, and ignored for prompts.
 *
 *     * Control codes are returned as received, because ReadConsole will
 *       not return the ESC key.
 *
 *     * Other key-down events are passed to ReadConsole.  The use of
 *       ReadConsole is different for commands than for prompts:
 *
 *       o For commands, the bogus key is pushed onto the queue before
 *         ReadConsole is called.  On return, non-ASCII characters are
 *         filtered, so they are not mistaken for Alt sequences; this also
 *         filters the bogus key.
 *
 *       o For prompts, the bogus key is not used, because that would
 *         interfere with dead keys.  Eight bit characters may be returned,
 *         and are coded in the configured code page.
 *
 *
 * Possible improvements
 * =====================
 *
 * Some possible improvements remain:
 *
 *     * Integrate the existing Finnish keyboard patch, for use with non-
 *       QWERTY layouts such as the German QWERTZ keyboard or Dvorak.
 *
 *     * Fix the keyboard glitches in the graphical version.  Namely, dead
 *       keys don't work, and input comes in as ISO-8859-1 but is displayed
 *       as code page 437 if IBMgraphics is set on startup.
 *
 *     * Transform incoming text to ISO-8859-1, for full compatibility with
 *       the graphical version.
 *
 *     * After pushing the bogus key and calling ReadConsole, check to see
 *       if we got the bogus key; if so, and an Alt is pressed, process the
 *       event as an Alt sequence.
 *
 */

#if 0
static char where_to_get_source[] = "http://www.nethack.org/";
static char author[] = "Ray Chason";
#endif

int process_keystroke2(HANDLE hConIn, INPUT_RECORD *ir, boolean *valid);

/* Use ray_processkeystroke for key commands, process_keystroke2 for prompts */
/* int ray_processkeystroke(INPUT_RECORD *ir, boolean *valid, int
 *                          portdebug);
 */

int process_keystroke2(HANDLE, INPUT_RECORD *ir, boolean *valid);
static int is_altseq(unsigned long shiftstate);

static int
is_altseq(unsigned long shiftstate)
{
    /* We need to distinguish the Alt keys from the AltGr key.
     * On NT-based Windows, AltGr signals as right Alt and left Ctrl together;
     * on 95-based Windows, AltGr signals as right Alt only.
     * So on NT, we signal Alt if either Alt is pressed and left Ctrl is not,
     * and on 95, we signal Alt for left Alt only. */
    switch (shiftstate
            & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED)) {
    case LEFT_ALT_PRESSED:
    case LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED:
        return 1;

    case RIGHT_ALT_PRESSED:
    case RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED:
        return (GetVersion() & 0x80000000) == 0;

    default:
        return 0;
    }
}

int ray_processkeystroke(
    HANDLE hConIn,
    INPUT_RECORD *ir,
    boolean *valid,
    boolean numberpad,
    int portdebug)
{
    int keycode, vk;
    unsigned char ch, pre_ch;
    unsigned short int scan;
    unsigned long shiftstate;
    int altseq = 0;
    const struct pad *kpad;
    DWORD count;

#ifdef QWERTZ_SUPPORT
    if (numberpad & 0x10) {
        numberpad &= ~0x10;
        qwertz = TRUE;
    } else {
        qwertz = FALSE;
    }
#endif
    shiftstate = 0L;
    ch = pre_ch = ir->Event.KeyEvent.uChar.AsciiChar;
    scan = ir->Event.KeyEvent.wVirtualScanCode;
    vk = ir->Event.KeyEvent.wVirtualKeyCode;
    keycode = MapVirtualKey(vk, 2);
    shiftstate = ir->Event.KeyEvent.dwControlKeyState;
    if (scan == 0 && vk == 0) {
        /* It's the bogus_key */
        ReadConsoleInput(hConIn, ir, 1, &count);
        *valid = FALSE;
        return 0;
    }

    if (is_altseq(shiftstate)) {
        if (ch || inmap(keycode, vk))
            altseq = 1;
        else
            altseq = -1; /* invalid altseq */
    }
    if (ch || (iskeypad(scan)) || (altseq > 0))
        *valid = TRUE;
    /* if (!valid) return 0; */
    /*
     * shiftstate can be checked to see if various special
     * keys were pressed at the same time as the key.
     * Currently we are using the ALT & SHIFT & CONTROLS.
     *
     *           RIGHT_ALT_PRESSED, LEFT_ALT_PRESSED,
     *           RIGHT_CTRL_PRESSED, LEFT_CTRL_PRESSED,
     *           SHIFT_PRESSED,NUMLOCK_ON, SCROLLLOCK_ON,
     *           CAPSLOCK_ON, ENHANCED_KEY
     *
     * are all valid bit masks to use on shiftstate.
     * eg. (shiftstate & LEFT_CTRL_PRESSED) is true if the
     *      left control key was pressed with the keystroke.
     */
    if (iskeypad(scan)) {
        ReadConsoleInput(hConIn, ir, 1, &count);
        kpad = numberpad ? numpad : keypad;
        if (shiftstate & SHIFT_PRESSED) {
            ch = kpad[scan - KEYPADLO].shift;
        } else if (shiftstate & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
            ch = kpad[scan - KEYPADLO].cntrl;
        } else {
            ch = kpad[scan - KEYPADLO].normal;
        }
#ifdef QWERTZ_SUPPORT
        /* OPTIONS=number_pad:-1 is for qwertz keyboard; for that setting,
           'numberpad' will be 0; core swaps y to zap, z to move northwest;
           we want numpad 7 to move northwest, so when qwertz is set,
           tell core that user who types numpad 7 typed z rather than y */
        if (qwertz && kpad[scan - KEYPADLO].normal == 'y')
            ch += 1; /* changes y to z, Y to Z, ^Y to ^Z */
#endif /*QWERTZ_SUPPORT*/
    } else if (altseq > 0) { /* ALT sequence */
        ReadConsoleInput(hConIn, ir, 1, &count);
        if (vk == 0xBF)
            ch = M('?');
        else
            ch = M(tolower((uchar) keycode));
    } else if (ch < 32 && !isnumkeypad(scan)) {
        /* Control code; ReadConsole seems to filter some of these,
         * including ESC */
        ReadConsoleInput(hConIn, ir, 1, &count);
    }
    /* Attempt to work better with international keyboards. */
    else {
        CHAR ch2;
        DWORD written;
        /* The bogus_key guarantees that ReadConsole will return,
         * and does not itself do anything */
        WriteConsoleInput(hConIn, &bogus_key, 1, &written);
        ReadConsole(hConIn, &ch2, 1, &count, NULL);
        /* Prevent high characters from being interpreted as alt
         * sequences; also filter the bogus_key */
        if (ch2 & 0x80)
            *valid = FALSE;
        else
            ch = ch2;
        if (ch == 0)
            *valid = FALSE;
    }
    if (ch == '\r')
        ch = '\n';
#ifdef PORT_DEBUG
    if (portdebug) {
        char buf[BUFSZ];
        Sprintf(buf, "PORTDEBUG: ch=%u, scan=%u, vk=%d, pre=%d, "
                     "shiftstate=0x%lX (ESC to end)\n",
                ch, scan, vk, pre_ch, shiftstate);
        fprintf(stdout, "\n%s", buf);
    }
#endif
    return ch;
}

int
process_keystroke2(
    HANDLE hConIn,
    INPUT_RECORD *ir,
    boolean *valid)
{
    /* Use these values for the numeric keypad */
    static const char keypad_nums[] = "789-456+1230.";

    unsigned char ch;
    int vk;
    unsigned short int scan;
    unsigned long shiftstate;
    int altseq;
    DWORD count;

    ch = ir->Event.KeyEvent.uChar.AsciiChar;
    vk = ir->Event.KeyEvent.wVirtualKeyCode;
    scan = ir->Event.KeyEvent.wVirtualScanCode;
    shiftstate = ir->Event.KeyEvent.dwControlKeyState;

    if (scan == 0 && vk == 0) {
        /* It's the bogus_key */
        ReadConsoleInput(hConIn, ir, 1, &count);
        *valid = FALSE;
        return 0;
    }

    altseq = is_altseq(shiftstate);
    if (ch || (iskeypad(scan)) || altseq)
        *valid = TRUE;
    /* if (!valid) return 0; */
    /*
     * shiftstate can be checked to see if various special
     * keys were pressed at the same time as the key.
     * Currently we are using the ALT & SHIFT & CONTROLS.
     *
     *           RIGHT_ALT_PRESSED, LEFT_ALT_PRESSED,
     *           RIGHT_CTRL_PRESSED, LEFT_CTRL_PRESSED,
     *           SHIFT_PRESSED,NUMLOCK_ON, SCROLLLOCK_ON,
     *           CAPSLOCK_ON, ENHANCED_KEY
     *
     * are all valid bit masks to use on shiftstate.
     * eg. (shiftstate & LEFT_CTRL_PRESSED) is true if the
     *      left control key was pressed with the keystroke.
     */
    if (iskeypad(scan) && !altseq) {
        ReadConsoleInput(hConIn, ir, 1, &count);
        ch = keypad_nums[scan - KEYPADLO];
    } else if (ch < 32 && !isnumkeypad(scan)) {
        /* Control code; ReadConsole seems to filter some of these,
         * including ESC */
        ReadConsoleInput(hConIn, ir, 1, &count);
    }
    /* Attempt to work better with international keyboards. */
    else {
        CHAR ch2;
        ReadConsole(hConIn, &ch2, 1, &count, NULL);
        ch = ch2 & 0xFF;
        if (ch == 0)
            *valid = FALSE;
    }
    if (ch == '\r')
        ch = '\n';
    return ch;
}

int
ray_checkinput(
    HANDLE hConIn,
    INPUT_RECORD *ir,
    DWORD *count,
    boolean numpad,
    int mode,
    int *mod,
    coord *cc)
{
#if defined(SAFERHANGUP)
    DWORD dwWait;
#endif
    int ch = 0;
    boolean valid = 0, done = 0;

#ifdef QWERTZ_SUPPORT
    if (numpad & 0x10) {
        numpad &= ~0x10;
        qwertz = TRUE;
    } else {
        qwertz = FALSE;
    }
#endif
    while (!done) {
        *count = 0;
        dwWait = WaitForSingleObject(hConIn, INFINITE);
#if defined(SAFERHANGUP)
        if (dwWait == WAIT_FAILED)
            return '\033';
#endif
        PeekConsoleInput(hConIn, ir, 1, count);
        if (mode == 0) {
            if ((ir->EventType == KEY_EVENT) && ir->Event.KeyEvent.bKeyDown) {
                ch = process_keystroke2(hConIn, ir, &valid);
                done = valid;
            } else
                ReadConsoleInput(hConIn, ir, 1, count);
        } else {
            ch = 0;
            if (count > 0) {
                if (ir->EventType == KEY_EVENT
                    && ir->Event.KeyEvent.bKeyDown) {
#ifdef QWERTZ_SUPPORT
                    if (qwertz)
                        numpad |= 0x10;
#endif
                    ch = ray_processkeystroke(hConIn, ir, &valid, numpad,
#ifdef PORTDEBUG
                                          1);
#else
                                          0);
#endif
#ifdef QWERTZ_SUPPORT
                    numpad &= ~0x10;
#endif
                    if (valid)
                        return ch;
                } else {
                    ReadConsoleInput(hConIn, ir, 1, count);
                    if (ir->EventType == MOUSE_EVENT) {
                        if ((ir->Event.MouseEvent.dwEventFlags == 0)
                            && (ir->Event.MouseEvent.dwButtonState
                                & MOUSEMASK)) {
                            cc->x =
                                ir->Event.MouseEvent.dwMousePosition.X + 1;
                            cc->y =
                                ir->Event.MouseEvent.dwMousePosition.Y - 1;

                            if (ir->Event.MouseEvent.dwButtonState
                                & LEFTBUTTON)
                                *mod = CLICK_1;
                            else if (ir->Event.MouseEvent.dwButtonState
                                     & RIGHTBUTTON)
                                *mod = CLICK_2;
#if 0 /* middle button */
				else if (ir->Event.MouseEvent.dwButtonState & MIDBUTTON)
			      		*mod = CLICK_3;
#endif
                            return 0;
                        }
                    }
#if 0
			/* We ignore these types of console events */
		        else if (ir->EventType == FOCUS_EVENT) {
		        }
		        else if (ir->EventType == MENU_EVENT) {
		        }
#endif
                }
            } else
                done = 1;
        }
    }
    *mod = 0;
    return ch;
}

int
ray_kbhit(
    HANDLE hConIn,
    INPUT_RECORD *ir)
{
    int done = 0; /* true =  "stop searching"        */
    int retval;   /* true =  "we had a match"        */
    DWORD count;
    unsigned short int scan;
    unsigned char ch;
    unsigned long shiftstate;
    int altseq = 0, keycode, vk;

    done = 0;
    retval = 0;
    while (!done) {
        count = 0;
        PeekConsoleInput(hConIn, ir, 1, &count);
        if (count > 0) {
            if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.bKeyDown) {
                ch = ir->Event.KeyEvent.uChar.AsciiChar;
                scan = ir->Event.KeyEvent.wVirtualScanCode;
                shiftstate = ir->Event.KeyEvent.dwControlKeyState;
                vk = ir->Event.KeyEvent.wVirtualKeyCode;
                if (scan == 0 && vk == 0) {
                    /* It's the bogus_key.  Discard it */
                    ReadConsoleInput(hConIn,ir,1,&count);
                } else {
                    keycode = MapVirtualKey(vk, 2);
                    if (is_altseq(shiftstate)) {
                        if (ch || inmap(keycode, vk))
                            altseq = 1;
                        else
                            altseq = -1; /* invalid altseq */
                    }
                    if (ch || iskeypad(scan) || altseq) {
                        done = 1;   /* Stop looking         */
                        retval = 1; /* Found what we sought */
                    } else {
                        /* Strange Key event; let's purge it to avoid trouble */
                        ReadConsoleInput(hConIn, ir, 1, &count);
                    }
                }

            } else if ((ir->EventType == MOUSE_EVENT
                        && (ir->Event.MouseEvent.dwButtonState
                            & MOUSEMASK))) {
                done = 1;
                retval = 1;
            }

            else /* Discard it, it's an insignificant event */
                ReadConsoleInput(hConIn, ir, 1, &count);
        } else /* There are no events in console event queue */ {
            done = 1; /* Stop looking               */
            retval = 0;
        }
    }
    return retval;
}

/*
 * nh340 key handling
 *
 * This is the NetHack keystroke processing from NetHack 3.4.0.
 * It can be built as a run-time loadable dll (nh340key.dll),
 * placed in the same directory as the nethack.exe executable,
 * and loaded by specifying OPTIONS=altkeyhandler:nh340key
 * in defaults.nh
 *
 * Keypad keys are translated to the normal values below.
 * Shifted keypad keys are translated to the
 *    shift values below.
 */

int
nh340_processkeystroke(
    HANDLE hConIn,
    INPUT_RECORD *ir,
    boolean *valid,
    boolean numberpad,
    int portdebug)
{
    int keycode, vk;
    unsigned char ch, pre_ch;
    unsigned short int scan;
    unsigned long shiftstate;
    int altseq = 0;
    const struct pad *kpad;

#ifdef QWERTZ_SUPPORT
    if (numberpad & 0x10) {
        numberpad &= ~0x10;
        qwertz = TRUE;
    } else {
        qwertz = FALSE;
    }
#endif

    shiftstate = 0L;
    ch = pre_ch = ir->Event.KeyEvent.uChar.AsciiChar;
    scan = ir->Event.KeyEvent.wVirtualScanCode;
    vk = ir->Event.KeyEvent.wVirtualKeyCode;
    keycode = MapVirtualKey(vk, 2);
    shiftstate = ir->Event.KeyEvent.dwControlKeyState;

    if (shiftstate & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
        if (ch || inmap(keycode, vk))
            altseq = 1;
        else
            altseq = -1; /* invalid altseq */
    }
    if (ch || (iskeypad(scan)) || (altseq > 0))
        *valid = TRUE;
    /* if (!valid) return 0; */
    /*
     * shiftstate can be checked to see if various special
     * keys were pressed at the same time as the key.
     * Currently we are using the ALT & SHIFT & CONTROLS.
     *
     *           RIGHT_ALT_PRESSED, LEFT_ALT_PRESSED,
     *           RIGHT_CTRL_PRESSED, LEFT_CTRL_PRESSED,
     *           SHIFT_PRESSED,NUMLOCK_ON, SCROLLLOCK_ON,
     *           CAPSLOCK_ON, ENHANCED_KEY
     *
     * are all valid bit masks to use on shiftstate.
     * eg. (shiftstate & LEFT_CTRL_PRESSED) is true if the
     *      left control key was pressed with the keystroke.
     */
    if (iskeypad(scan)) {
        kpad = numberpad ? numpad : keypad;
        if (shiftstate & SHIFT_PRESSED) {
            ch = kpad[scan - KEYPADLO].shift;
        } else if (shiftstate & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
            ch = kpad[scan - KEYPADLO].cntrl;
        } else {
            ch = kpad[scan - KEYPADLO].normal;
        }
#ifdef QWERTZ_SUPPORT
        /* OPTIONS=number_pad:-1 is for qwertz keyboard; for that setting,
           'numberpad' will be 0; core swaps y to zap, z to move northwest;
           we want numpad 7 to move northwest, so when qwertz is set,
           tell core that user who types numpad 7 typed z rather than y */
        if (qwertz && kpad[scan - KEYPADLO].normal == 'y')
            ch += 1; /* changes y to z, Y to Z, ^Y to ^Z */
#endif /*QWERTZ_SUPPORT*/
    } else if (altseq > 0) { /* ALT sequence */
        if (vk == 0xBF)
            ch = M('?');
        else
            ch = M(tolower((uchar) keycode));
    }
    if (ch == '\r')
        ch = '\n';
#ifdef PORT_DEBUG
    if (portdebug) {
        char buf[BUFSZ];
        Sprintf(buf,
                "PORTDEBUG (%s): ch=%u, sc=%u, vk=%d, sh=0x%lX (ESC to end)",
                "nh340", ch, scan, vk, shiftstate);
        fprintf(stdout, "\n%s", buf);
    }
#endif
    return ch;
}

int
nh340_kbhit(
    HANDLE hConIn,
    INPUT_RECORD *ir)
{
    int done = 0; /* true =  "stop searching"        */
    int retval;   /* true =  "we had a match"        */
    DWORD count;
    unsigned short int scan;
    unsigned char ch;
    unsigned long shiftstate;
    int altseq = 0, keycode, vk;
    done = 0;
    retval = 0;
    while (!done) {
        count = 0;
        PeekConsoleInput(hConIn, ir, 1, &count);
        if (count > 0) {
            if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.bKeyDown) {
                ch = ir->Event.KeyEvent.uChar.AsciiChar;
                scan = ir->Event.KeyEvent.wVirtualScanCode;
                shiftstate = ir->Event.KeyEvent.dwControlKeyState;
                vk = ir->Event.KeyEvent.wVirtualKeyCode;
                keycode = MapVirtualKey(vk, 2);
                if (shiftstate & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
                    if (ch || inmap(keycode, vk))
                        altseq = 1;
                    else
                        altseq = -1; /* invalid altseq */
                }
                if (ch || iskeypad(scan) || altseq) {
                    done = 1;   /* Stop looking         */
                    retval = 1; /* Found what we sought */
                } else {
                    /* Strange Key event; let's purge it to avoid trouble */
                    ReadConsoleInput(hConIn, ir, 1, &count);
                }

            } else if ((ir->EventType == MOUSE_EVENT
                        && (ir->Event.MouseEvent.dwButtonState
                            & MOUSEMASK))) {
                done = 1;
                retval = 1;
            }

            else /* Discard it, it's an insignificant event */
                ReadConsoleInput(hConIn, ir, 1, &count);
        } else /* There are no events in console event queue */ {
            done = 1; /* Stop looking               */
            retval = 0;
        }
    }
    return retval;
}

int
nh340_checkinput(
    HANDLE hConIn,
    INPUT_RECORD *ir,
    DWORD *count,
    boolean numpad,
    int mode,
    int *mod,
    coord *cc)
{
#if defined(SAFERHANGUP)
    DWORD dwWait;
#endif
    int ch = 0;
    boolean valid = 0, done = 0;

#ifdef QWERTZ_SUPPORT
    if (numpad & 0x10) {
        numpad &= ~0x10;
        qwertz = TRUE;
    } else {
        qwertz = FALSE;
    }
#endif
    while (!done) {
#if defined(SAFERHANGUP)
        dwWait = WaitForSingleObjectEx(hConIn,   // event object to wait for
                                       INFINITE, // waits indefinitely
                                       TRUE);    // alertable wait enabled
        if (dwWait == WAIT_FAILED)
            return '\033';
#endif
        ReadConsoleInput(hConIn, ir, 1, count);
        if (mode == 0) {
            if ((ir->EventType == KEY_EVENT) && ir->Event.KeyEvent.bKeyDown) {
#ifdef QWERTZ_SUPPORT
                if (qwertz)
                    numpad |= 0x10;
#endif
                ch = nh340_processkeystroke(hConIn, ir, &valid, numpad, 0);
#ifdef QWERTZ_SUPPORT
                numpad &= ~0x10;
#endif
                done = valid;
            }
        } else {
            if (count > 0) {
                if (ir->EventType == KEY_EVENT
                    && ir->Event.KeyEvent.bKeyDown) {
                    ch = nh340_processkeystroke(hConIn, ir, &valid, numpad, 0);
                    if (valid)
                        return ch;
                } else if (ir->EventType == MOUSE_EVENT) {
                    if ((ir->Event.MouseEvent.dwEventFlags == 0)
                        && (ir->Event.MouseEvent.dwButtonState & MOUSEMASK)) {
                        cc->x = ir->Event.MouseEvent.dwMousePosition.X + 1;
                        cc->y = ir->Event.MouseEvent.dwMousePosition.Y - 1;

                        if (ir->Event.MouseEvent.dwButtonState & LEFTBUTTON)
                            *mod = CLICK_1;
                        else if (ir->Event.MouseEvent.dwButtonState
                                 & RIGHTBUTTON)
                            *mod = CLICK_2;
#if 0 /* middle button */
				    else if (ir->Event.MouseEvent.dwButtonState & MIDBUTTON)
			      		*mod = CLICK_3;
#endif
                        return 0;
                    }
                }
            } else
                done = 1;
        }
    }
    return mode ? 0 : ch;
}

#endif /* WIN32 */

/* $NHDT-Header: NetHack 5.0 consoletty.c 2026/10/17_14:49:13 NetHack-5.0:1.210 $ */
/* Copyright (c) Michael Allison, 2026 */
/* Copyright (c) NetHack PC Development Team 1993    */
/* NetHack may be freely redistributed.  See license for details. */

/* tty.c - (Windows console) version */

/*
 * Initial Creation                             M. Allison      1993/01/31
 * Switch to low level console output routines  M. Allison      2003/10/01
 * Restrict cursor movement until input pending M. Lehotay      2003/10/02
 * Call Unicode version of output API on NT     R. Chason       2005/10/28
 * Use of back buffer to improve performance    B. House        2018/05/06
 *
 */

#ifndef NO_VT
#define UTF8_FROM_CORE
#endif

#ifdef WIN32
#define NEED_VARARGS /* Uses ... */
#include "win32api.h"
#include "winos.h"
#include "hack.h"
#include "wintty.h"
#include <VersionHelpers.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <tlhelp32.h>  /* for WindowsTerminal confirmation */
#include <locale.h>
#ifndef INTEGER_H
#include "integer.h"
#endif

#ifdef __MINGW32__
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
#endif
#endif

extern boolean getreturn_enabled; /* from windmain.c */
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

enum console_attributes {
    atr_bold = 1,
    atr_dim = 2,
    atr_uline = 4,
    atr_blink = 8,
    atr_inverse = 16
};


/* Win32 Screen buffer,coordinate,console I/O information */
COORD ntcoord;
INPUT_RECORD gbl_ir;
static boolean settty_needed = FALSE;
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

static char tgotobuf[20];
#define tgoto(fmt, x, y) (Sprintf(tgotobuf, fmt, y + 1, x + 1), tgotobuf)
/* Mapping data for the six terminfo colors that resolve to pairs of nethack
 * colors.  Black and white are handled specially.
 */

#ifndef COLOR_BLACK
enum curses_colors {
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE
};
#endif

const struct {
    int ti_color, nh_color, nh_bright_color;
} ti_map[6] = { { COLOR_RED, CLR_RED, CLR_ORANGE },
                { COLOR_GREEN, CLR_GREEN, CLR_BRIGHT_GREEN },
                { COLOR_YELLOW, CLR_BROWN, CLR_YELLOW },
                { COLOR_BLUE, CLR_BLUE, CLR_BRIGHT_BLUE },
                { COLOR_MAGENTA, CLR_MAGENTA, CLR_BRIGHT_MAGENTA },
                { COLOR_CYAN, CLR_CYAN, CLR_BRIGHT_CYAN } };

typedef struct {
    unsigned char r, g, b;
} RGB;

static char nilstring[] = "";
/*
 * Define the console_t data structs.
 *    cell_t        substruct of console_t, used twice (fg and bg) use
 *                  by console.ConsoleAPI
 *    console_t     track various Windows console-related data.
 *    tset_t        substruct of console_t, used twice (orig and active)
 *    console_api_t additional values used by console.ConsoleAPI
 */

#define MAX_UTF8_SEQUENCE 7

typedef struct {
    /* these two are used only by console API */
    WCHAR character_consoleAPI;
    WORD attribute_consoleAPI;
    /* the following are used only by WindowsTerminal */
    uint8 utf8str[MAX_UTF8_SEQUENCE];
    WCHAR wcharacter;
    WORD attr;
    const char *bkcolorseq;
    const char *colorseq;
} cell_t;

#define CONSOLE_CLEAR_ATTRIBUTE \
    (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define CONSOLE_CLEAR_CHARACTER (' ')
#define CONSOLE_UNDEFINED_CHARACTER ('\0')
#define CONSOLE_UNDEFINED_ATTRIBUTE (0)

cell_t clear_cell = {
    CONSOLE_CLEAR_CHARACTER,
    CONSOLE_CLEAR_ATTRIBUTE,
    { CONSOLE_CLEAR_CHARACTER, 0, 0, 0, 0, 0, 0 },
    CONSOLE_CLEAR_CHARACTER, /* wcharacter */
    0,                       /* attr */
    "\x1b[0m",               /* bkcolorseq */
    0                        /* colorseq */
};
cell_t undefined_cell = {
    CONSOLE_UNDEFINED_CHARACTER,
    CONSOLE_UNDEFINED_ATTRIBUTE,
    { CONSOLE_UNDEFINED_CHARACTER, 0, 0, 0, 0, 0, 0 },
    CONSOLE_UNDEFINED_CHARACTER, /* wcharacter */
    0,                           /* attr */
    (const char *) 0,            /* bkcolorseq */
    (const char *) 0             /* colorseq */
};


struct tset_t {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    CONSOLE_FONT_INFOEX font_info;
    char *localestr;
    UINT code_page;
    DWORD in_cmode;
    DWORD out_cmode;
    boolean QuickEdit;
};

struct console_api_t {
    int32_t color;
    int32_t bkcolor;
    int32_t attribute;
    int32_t nhattr[ATR_INVERSE + 1];
    /*
     * Console Buffer Flipping Support
     *
     * To minimize the number of calls into the WriteConsoleOutputXXX methods,
     * we implement a notion of a console back buffer which keeps the next
     * frame of console output as it is being composed.  When ready to show
     * the new frame, we compare this next frame to what is currently being
     * output and only call WriteConsoleOutputXXX for those console values
     * that need to change.
     *
     */
    cell_t *front_buffer;
    cell_t *back_buffer;
};

struct console_t {
    boolean is_ready;
    boolean WindowsTerminal;
    boolean ConsoleAPI;
    HWND hWnd;
    WORD background;
    WORD foreground;
    COORD cursor;
    HANDLE hConOut;
    HANDLE hConIn;
    int width;
    int height;
    int buffer_size;
    boolean has_unicode;
    boolean font_changed;
    WCHAR cpMap[256];
    struct tset_t orig;
    struct tset_t active;
    struct console_api_t api;
    /* */
} console = {
    FALSE, /* is_ready */
    FALSE, /* WindowsTerminal */
    FALSE, /* ConsoleAPI */
    0,     /* hWnd */
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED), /* background */
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED), /* foreground */
    { 0, 0 },                                              /* cursor */
    NULL,                                                  /* hConIn */
    NULL,                                                  /* hConOut*/
    0,                                                     /* width */
    0,                                                     /* height */
    0,                                                     /* buffer_size */
    FALSE,                                                 /* has_unicode */
    FALSE,                                                 /* font_changed */
    { 0 },                                                 /* cpMap */
    {
        { { 0, 0 }, { 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0 } }, /* orig.cbsi */
        { 0UL, 0, { 0, 0 }, 0U, 0U, { 0 } }, /* orig.font_info */
        NULL,                                /* orig.localestr */
        0U,                                  /* orig.code_page */
        0,                                   /* orig.in_cmode */
        0,                                   /* orig.out_cmode */
        FALSE,                               /* orig.QuickEdit */
    },
    {
        { { 0, 0 }, { 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0 } }, /* active.cbsi */
        { 0UL, 0, { 0, 0 }, 0U, 0U, { 0 } }, /* active.font_info */
        NULL,                                /* active.localestr */
        0U,                                  /* active.code_page */
        0,                                   /* active.in_cmode */
        0,                                   /* active.out_cmode */
        FALSE,                               /* active.QuickEdit */
    },
    {
        (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED), /* api.color */
        (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED), /* api.bkcolor */
        0,                       /* api.attribute */
        { 0, 0, 0, 0, 0, 0, 0 }, /* api.nhattr[] */
        NULL, /* api.front_buffer */
        NULL, /* api.back_buffer */
    }
};

const char *const legal_key_handling[] = {
    "none",
    "default",
    "ray",
    "340",
};

enum windows_key_handling keyh[] = { no_keyhandling, default_keyhandling,
                                     ray_keyhandling, nh340_keyhandling };
int default_processkeystroke(HANDLE, INPUT_RECORD *, boolean *, uchar, int);
int default_kbhit(HANDLE, INPUT_RECORD *);
int default_checkinput(HANDLE, INPUT_RECORD *, DWORD *, uchar, int, int *,
                       coord *);

int ray_processkeystroke(HANDLE, INPUT_RECORD *, boolean *, uchar, int);
int ray_kbhit(HANDLE, INPUT_RECORD *);
int ray_checkinput(HANDLE, INPUT_RECORD *, DWORD *, uchar, int, int *,
                   coord *);

int nh340_processkeystroke(HANDLE, INPUT_RECORD *, boolean *, uchar, int);
int nh340_kbhit(HANDLE, INPUT_RECORD *);
int nh340_checkinput(HANDLE, INPUT_RECORD *, DWORD *, uchar, int, int *,
                     coord *);

struct keyboard_handling_t {
    enum windows_key_handling khid;
    int (*pProcessKeystroke)(HANDLE, INPUT_RECORD *, boolean *, uchar, int);
    int (*pNHkbhit)(HANDLE, INPUT_RECORD *);
    int (*pCheckInput)(HANDLE, INPUT_RECORD *, DWORD *, uchar, int, int *,
                       coord *);
} keyboard_handling = { no_keyhandling, default_processkeystroke,
                        default_kbhit, default_checkinput };

// static DWORD ccount;
#ifndef CLR_MAX
#define CLR_MAX 16
#endif

int ttycolors[CLR_MAX];
int ttycolors_inv[CLR_MAX];

#define MAX_OVERRIDES 256
unsigned char key_overrides[MAX_OVERRIDES];
char erase_char, kill_char;
#define DEFTEXTCOLOR ttycolors[7]
static INPUT_RECORD bogus_key;

/*
Windows console palette:
Color Name      Console Legacy RGB Values  New Default RGB Values
BLACK           0,0,0                      12,12,12
DARK_BLUE       0,0,128                    0,55,218
DARK_GREEN      0,128,0                    19,161,14
DARK_CYAN       0,128,128                  58,150,221
DARK_RED        128,0,0                    197,15,31
DARK_MAGENTA    128,0,128                  136,23,152
DARK_YELLOW     128,128,0                  193,156,0
DARK_WHITE      192,192,192                204,204,204
BRIGHT_BLACK    128,128,128                118,118,118
BRIGHT_BLUE     0,0,255                    59,120,255
BRIGHT_GREEN    0,255,0                    22,198,12
BRIGHT_CYAN     0,255,255                  97,214,214
BRIGHT_RED      255,0,0                    231,72,86
BRIGHT_MAGENTA  255,0,255                  180,0,158
BRIGHT_YELLOW   255,255,0                  249,241,165
WHITE           255,255,255                242,242,242
*/

int32 customcolors[CLR_MAX];
const char *esc_seq_colors[CLR_MAX] = { 0 };
const char *esc_seq_bkcolors[CLR_MAX] = { 0 };


/* function prototypes */

static BOOL CtrlHandler(DWORD);
static void xputc_core_ConsoleAPI(char);
static void xputc_core_wt(int, int);
void cmov(int, int);
void nocmov(int, int);
int process_keystroke(INPUT_RECORD *, boolean *, uchar numberpad,
                      int portdebug);
static void init_ttycolor(void);
static void really_move_cursor(void);
static void check_and_set_font(void);
static boolean check_font_widths(void);
static void set_known_good_console_font(void);
static void restore_original_console_font(void);
void tty_ibmgraphics_fixup(void);
extern void (*ibmgraphics_mode_callback)(void);  /* symbols.c */
extern void (*utf8graphics_mode_callback)(void); /* symbols.c */

static boolean OnWindows95_98_Me(void);

static void init_custom_colors(void);
static void free_custom_colors(void);
static void buffer_write_wt(uint8_t *buffer, size_t sz);
static char *s_atr2str(int);
static char *e_atr2str(int);
static void buffer_fill_to_end_ConsoleAPI(cell_t *buffer, cell_t *fill,
                                          int x, int y);
static void back_buffer_flip_ConsoleAPI(void);
static void buffer_write_ConsoleAPI(cell_t *buffer, cell_t *cell, COORD pos);
static void buffer_clear_to_eol_ConsoleAPI(cell_t *buffer, int x, int y);
static void set_console_cursor(int x, int y);

/*
 * termcap-like support for Windows Terminal
 */
void emit_start_bold(void);
void emit_stop_bold(void);
void emit_start_dim(void);
void emit_stop_dim(void);
void emit_start_blink(void);
void emit_stop_blink(void);
void emit_start_underline(void);
void emit_stop_underline(void);
void emit_start_inverse(void);
void emit_stop_inverse(void);
void emit_start_24bitcolor(long color24bit);
void emit_start_256color(int u256coloridx);
void emit_default_color(void);
void emit_return_to_default(void);
void emit_hide_cursor(void);
void emit_show_cursor(void);
void emit_cursor_down(int);
void emit_cursor_horizontal_pos(int);
void emit_cursor_left(void);
void emit_cursor_left_n(int);
void emit_cursor_right(void);
void emit_cursor_right_n(int);
void emit_cursor_goto(int, int);
void emit_clear_screen(void);
void emit_home(void);
void emit_clear_to_eol(void);
void emit_VE(void);
void emit_TE(void);
void emit_KS(void);
void emit_KE(void);
void emit_TI(void);
void emit_ts(void);
void emit_AS(void);
void emit_AE(void);
void emit_HE(void);
void emit_ND(void);
static void emit24bit(long mcolor);
static void emit256(int u256coloridx);

/**************************************************************
 * termcap alternatives
 **************************************************************/

#if defined(ASCIIGRAPH)
static boolean GFlag = FALSE;
static boolean HE_resets_AS; /* see termcap.c */
#endif

#define tcfmtstr "\033[38;2;%ld;%ld;%ldm"
#define tcfmtstr256 "\033[38;5;%dm"
#define tcfmtstr24bit "\x1b[38;2;%ld;%ld;%ldm"

void
emit_hide_cursor(void)
{
    //    DWORD unused;
    static const char escseq[] = "\x1b[?25l";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int)
    //    strlen(escseq),
    //                  &unused, NULL);
}

void
emit_show_cursor(void)
{
    //   DWORD unused;
    static const char escseq[] = "\x1b[?25h";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int)
    //    strlen(escseq),
    //                  &unused, NULL);
}

void
emit_start_bold(void)
{
    //    DWORD unused;
    static const char escseq[] = "\x1b[1m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //   WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
    //                 &unused, NULL);
}

void
emit_stop_bold(void)
{
    //   DWORD unused;
    static const char escseq[] = "\x1b[24m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int)
    //    strlen(escseq),
    //                  &unused, NULL);
}

void
emit_start_dim(void)
{
    static const char escseq[] = "\x1b[4m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz); 
}

void
emit_stop_dim(void)
{
    static const char escseq[] = "\x1b[24m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz); 
}

void
emit_start_blink(void)
{
    //    DWORD unused;
    static const char escseq[] = "\x1b[5m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int)
    //    strlen(escseq),
    //                  &unused, NULL);
}

void
emit_stop_blink(void)
{
    //    DWORD unused;
    static const char escseq[] = "\x1b[25m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //   WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
    //                 &unused, NULL);
}

void
emit_start_underline(void)
{
    //    DWORD unused;
    static const char escseq[] = "\x1b[4m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //   WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
    //                 &unused, NULL);
}

void
emit_stop_underline(void)
{
    //    DWORD unused;
    static const char escseq[] = "\x1b[24m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int)
    //    strlen(escseq),
    //                  &unused, NULL);
}
void
emit_start_inverse(void)
{
    //    DWORD unused;
    static const char escseq[] = "\x1b[7m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int)
    //    strlen(escseq),
    //                  &unused, NULL);
}

void
emit_stop_inverse(void)
{
    //    DWORD unused;
    static const char escseq[] = "\x1b[27m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int)
    //    strlen(escseq),
    //                  &unused, NULL);
}

void
emit_start_256color(int u256coloridx)
{
    //    DWORD unused;
    static char tcolorbuf[QBUFSZ];
    Snprintf(tcolorbuf, sizeof tcolorbuf, tcfmtstr256, (long) u256coloridx);
    size_t sz = strlen(tcolorbuf);

    buffer_write_wt((uint8_t *) tcolorbuf, sz);
    //    WriteConsoleA(console.hConOut, (LPCSTR) tcolorbuf,
    //                  (int) strlen(tcolorbuf), &unused, NULL);
}

void
emit_start_24bitcolor(long color24bit)
{
    //    DWORD unused;
    static char tcolorbuf[QBUFSZ];
    uint32 mcolor = COLORVAL(color24bit);
    size_t sz;

    Snprintf(tcolorbuf, sizeof tcolorbuf, tcfmtstr24bit,
             (long) ((mcolor >> 16) & 0xFF), /* red */
             (long) ((mcolor >> 8) & 0xFF),  /* green */
             (long) ((mcolor >> 0) & 0xFF)); /* blue */
    sz = strlen(tcolorbuf);

    buffer_write_wt((uint8_t *) tcolorbuf, sz);
    //   WriteConsoleA(console.hConOut, (LPCSTR) tcolorbuf,
    //                 (int) strlen(tcolorbuf), &unused, NULL);
}

void
emit_default_color(void)
{
    //    DWORD unused;
    static char escseq[] = "\x1b[39m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //   WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
    //                 &unused, NULL);
}
void
emit_return_to_default(void)
{
    //    DWORD unused;
    static char escseq[] = "\x1b[0m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
    //   WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
    //                 &unused, NULL);
}

DISABLE_WARNING_FORMAT_NONLITERAL

void
emit_cursor_up(int nlines)
{
    static char escseq[] = "\x1b[%dA";
    static char buf[11];
    size_t sz;

    sz = sprintf(buf, escseq, nlines);
    buffer_write_wt((uint8_t *) buf, sz);
}
void
emit_cursor_down(int nlines)
{
    static char escseq[] = "\x1b[%dB";
    static char buf[11];
    size_t sz;

    sz = sprintf(buf, escseq, nlines);
    buffer_write_wt((uint8_t *) buf, sz);
}

void
emit_cursor_horizontal_pos(int pos)
{
    static char escseq[] = "\x1b[%dG";
    static char buf[11];
    size_t sz;

    sz = sprintf(buf, escseq, pos);
    buffer_write_wt((uint8_t *) buf, sz);
}

void
emit_cursor_left(void)
{
    static char escseq[] = "\x1b[D";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_cursor_left_n(int cnt)
{
    static char escseq[] = "\x1b[%dD";
    static char buf[11];
    size_t sz;

    sz = sprintf(buf, escseq, cnt);
    buffer_write_wt((uint8_t *) buf, sz);
}

void
emit_cursor_right_n(int cnt)
{
    static char escseq[] = "\x1b[%dC";
    static char buf[11];
    size_t sz;

    sz = sprintf(buf, escseq, cnt);
    buffer_write_wt((uint8_t *) buf, sz);
}

void
emit_cursor_right(void)
{
    static char escseq[] = "\x1b[C";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_cursor_goto(int x, int y)
{
    static char escseq[] = "\x1b[%d;%dH";
    static char buf[15];
    size_t sz;

    sz = sprintf(buf, escseq, y, x);
    buffer_write_wt((uint8_t *) buf, sz);
}

void
emit_clear_screen(void)
{
    static const char escseq[] = "\x1b[2J";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_home(void)
{
    static const char escseq[] = "\x1b[H";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_clear_to_eol(void)
{
    static const char escseq[] = "\x1b[K";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}
void
emit_TE(void)
{
    static char escseq[] = "\x1b[?1049l";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_VE(void)
{
    static char escseq[] = "\x1b[?25h";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}
void
emit_KS(void)
{
    static char escseq[] = "\x1b[?1h";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_KE(void)
{
    static char escseq[] = "\x1b[?1l";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_TI(void)
{
    static char escseq[] = "\x1b[?1049h";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_VS(void)
{
    static char escseq[] = "\x1b[?12;25h";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_AS(void)
{
    static char escseq[] = "\x1b[11m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_AE(void)
{
    static char escseq[] = "\x1b[10m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_HE(void)
{
    static char escseq[] = "\x1b[0m";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_ND(void)
{
    static char escseq[] = "\x1b[C";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}

void
emit_ts(void)
{
    static char escseq[] = "\x1b]0";
    static size_t sz = sizeof escseq - 1;

    buffer_write_wt((uint8_t *) escseq, sz);
}
RESTORE_WARNING_FORMAT_NONLITERAL

void
term_startup(int *wid, int *hgt)
{
    *wid = console.width;
    *hgt = console.height;
    set_option_mod_status("mouse_support", set_in_game);
}

void
term_shutdown(void)
{
    console_exit();
}

void
tty_number_pad(int state)
{
    if (console.WindowsTerminal) {
        switch (state) {
        case -1: /* activate keypad mode (escape sequences) */
            emit_KS();
            break;
        case 1: /* activate numeric mode for keypad (digits) */
            emit_KE();
            break;
        case 0: /* don't need to do anything--leave terminal as-is */
        default:
            break;
        }
    }
}

void
term_start_screen(void)
{
    if (console.WindowsTerminal) {
        emit_TI();
        emit_VS();
#ifdef TERMLIB
        if (SYMHANDLING(H_DEC))
            tty_decgraphics_termcap_fixup();
        /* set up callback in case option is not set yet but toggled later */
        decgraphics_mode_callback = tty_decgraphics_termcap_fixup;
#endif
#ifdef ENHANCED_SYMBOLS
        utf8graphics_mode_callback = tty_utf8graphics_fixup;
#endif
        if (gc.Cmd.num_pad)
            tty_number_pad(1); /* make keypad send digits */
    } else {
        /* console.ConsoleAPI */
        if (iflags.num_pad)
            tty_number_pad(1); /* make keypad send digits */
                               //   if (console.WindowsTerminal) {
        ibmgraphics_mode_callback = tty_ibmgraphics_fixup;
#ifdef ENHANCED_SYMBOLS
#ifdef UTF8_FROM_CORE
        utf8graphics_mode_callback = tty_utf8graphics_fixup;
#endif
#endif
    }
}

void
term_end_screen(void)
{
    if (console.WindowsTerminal) {
        term_clear_screen();
        emit_VE();
        emit_TE();
    } else {
        /* console.ConsoleAPI */
        term_clear_screen();
        really_move_cursor();
        buffer_fill_to_end_ConsoleAPI(console.api.back_buffer, &clear_cell, 0,
                                      0);
        back_buffer_flip_ConsoleAPI();
        FlushConsoleInputBuffer(console.hConIn);
    }
}
DISABLE_WARNING_FORMAT_NONLITERAL

void
nocmov(int x, int y)
{
    if (console.WindowsTerminal) {
        if ((int) ttyDisplay->cury > y) {
            while ((int) ttyDisplay->cury > y) { /* Go up. */
                emit_cursor_up(1);
                ttyDisplay->cury--;
            }
        } else if ((int) ttyDisplay->cury < y) { /* Go down. */
            while ((int) ttyDisplay->cury < y) {
                emit_cursor_down(1);
                ttyDisplay->cury++;
            }
        }
        if ((int) ttyDisplay->curx < x) { /* Go to the right. */
            /* Go to the right. */
            while ((int) ttyDisplay->curx < x) {
                emit_ND();
                ttyDisplay->curx++;
            }
        } else if ((int) ttyDisplay->curx > x) {
            while ((int) ttyDisplay->curx > x) { /* Go to the left. */
                emit_cursor_left();
                ttyDisplay->curx--;
            }
        }
    } else {
        /* console.ConsoleAPI */
        ttyDisplay->curx = x;
        ttyDisplay->cury = y;

        set_console_cursor(x, y);
    }
}

void
cmov(int x, int y)
{
    if (console.WindowsTerminal) {
        static char escseq[] = "\x1b[%d;%dH";
        size_t sz;

        sz = sprintf(tgotobuf, escseq, y + 1, x + 1);
        buffer_write_wt((uint8_t *) tgotobuf, sz);
    } else {
        /* console.ConsoleAPI */
        if (x >= console.width)
            x = console.width - 1;
        if (y >= console.height)
            y = console.height - 1;
    }
    ttyDisplay->cury = y;
    ttyDisplay->curx = x;
    if (console.ConsoleAPI) {
        set_console_cursor(x, y);
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

void
xputs(const char *s)
{
    int k;

    if (s) {
        if (console.WindowsTerminal) {
            for (k = 0; s[k]; ++k)
                xputc_core_wt(s[k], 0);
        } else {
            /* console.ConsoleAPI */
            int slen = (int) strlen(s);

            if (ttyDisplay)
                set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
            else
                set_console_cursor(console.cursor.X, console.cursor.Y);
            for (k = 0; k < slen && s[k]; ++k) {
                xputc_core_ConsoleAPI((int) s[k]);
            }
        }
    }
}

/* xputc_core() and g_putch() are the only routines that actually place
   output. same signature as 'putchar()' with potential failure result ignored
 */
int
xputc(int ch)
{
    if (console.WindowsTerminal) {
        xputc_core_wt(ch, 0);
    } else {
        int x = ttyDisplay->curx, y = ttyDisplay->cury;

        if (x < console.width && y < console.height) {
            set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
            xputc_core_ConsoleAPI((char) ch);
        }
    }
    return ch;
}

void
xputc_core_wt(int ch, int xcwflags)
{
    DWORD ccount = 0;
    WCHAR wch[2];
    static uint8 utf8str[MAX_UTF8_SEQUENCE];

    // emit_show_cursor();
    if (console.has_unicode && xcwflags == 0) {
        switch (ch) {
        case '\n':
            emit_cursor_down(1);
            //            if (console.cursor.Y < console.height - 1)
            //                console.cursor.Y++;
            FALLTHROUGH;
        /* FALLTHRU */
        case '\r':
            emit_cursor_horizontal_pos(1);
            break;
        case '\b':
            emit_cursor_left();
            break;
        default:
            wch[0] = (xcwflags == 0 && ch >= 0 && ch < SIZE(console.cpMap))
                         ? console.cpMap[ch]
                         : ch;
            wch[1] = 0;
            /* we have to convert it to UTF-8 for utf8str */
            ccount =
                WideCharToMultiByte(CP_UTF8, 0, wch, -1, (char *) utf8str,
                                    (int) sizeof utf8str, NULL, NULL);
        }
    } else {
        /* we can just use the UTF-8 utf8str field, since ascii is a
           single-byte representation of a small subset of unicode */
        utf8str[0] = ch;
        utf8str[1] = 0;
        ccount = 1;
    }
    if (ccount) {
        buffer_write_wt(utf8str, ccount);
    }
}

void
xputc_core_ConsoleAPI(char ch)
{
    DWORD ccount = 1; /* default to non-zero */
    boolean inverse = FALSE;

    nhassert(console.cursor.X >= 0 && console.cursor.X < console.width);
    nhassert(console.cursor.Y >= 0 && console.cursor.Y < console.height);

    cell_t cell;

    switch (ch) {
    case '\n':
        if (console.cursor.Y < console.height - 1)
            console.cursor.Y++;
        FALLTHROUGH;
    /* FALLTHRU */
    case '\r':
        console.cursor.X = 1;
        break;
    case '\b':
        if (console.cursor.X > 1) {
            console.cursor.X--;
        } else if (console.cursor.Y > 0) {
            console.cursor.X = console.width - 1;
            console.cursor.Y--;
        }
        break;
    default:
        inverse = (console.api.nhattr[ATR_INVERSE] && iflags.wc_inverse);
        console.api.attribute = (inverse)
                                       ? ttycolors_inv[console.api.color]
                                       : ttycolors[console.api.color];
        if (console.api.nhattr[ATR_BOLD])
            console.api.attribute |=
                (inverse) ? BACKGROUND_INTENSITY : FOREGROUND_INTENSITY;

        cell.attribute_consoleAPI = console.api.attribute;
        cell.character_consoleAPI =
            (console.has_unicode ? console.cpMap[ch] : ch);
        if (ccount) {
            buffer_write_ConsoleAPI(console.api.back_buffer, &cell,
                                    console.cursor);
            if (console.cursor.X == console.width - 1) {
                if (console.cursor.Y < console.height - 1) {
                    console.cursor.X = 1;
                    console.cursor.Y++;
                }
            } else {
                console.cursor.X++;
            }
        }
    }
    nhassert(console.cursor.X >= 0 && console.cursor.X < console.width);
    nhassert(console.cursor.Y >= 0 && console.cursor.Y < console.height);
}

/*
 * Overrides wintty.c function of the same name
 * for win32. It is used for glyphs only, not text.
 */
#ifdef UTF8_FROM_CORE
/* See g_pututf8() for a corresponding UTF-8 sequence
 * version, rather than a single character.
 */
#endif

void
console_g_putch(int in_ch)
{
    unsigned char ch = (unsigned char) in_ch;
    cell_t cell;
    boolean inverse = FALSE;

    if (console.WindowsTerminal) {
        static int32_t last_color = -1, last_bkcolor = -1;
        static uint8 utf8str[MAX_UTF8_SEQUENCE];

#if defined(ASCIIGRAPH)
        if (SYMHANDLING(H_UTF8)) {
            (void) putchar(ch);
        } else if (SYMHANDLING(H_IBM)
                   /* for DECgraphics, lower-case letters with high bit set
                      mean switch character set and render with high bit
                      clear; might want 8-bits for other characters */
                   || (iflags.eight_bit_tty
                       && (!SYMHANDLING(H_DEC) || (in_ch & 0x7f) < 0x60))) {
            /* IBM-compatible displays don't need other stuff */
            xputc_core_wt(ch, 1);
        } else if (ch & 0x80) {
            if (!GFlag || HE_resets_AS) {
                graph_on();
                GFlag = TRUE;
            }
            xputc_core_wt((ch ^ 0x80), 1); /* Strip 8th bit */
        } else {
            if (GFlag
#ifdef DECgraphicsOptimization
                /* for DECgraphics, we only need to switch back from the
                   line drawing character set to the normal one if 'ch' is
                   a lowercase letter or one of a handful of punctuation
                   characters (the range is contiguous but somewhat odd);
                   deferring graph_off() now might allow skipping both it
                   and next potential graph_on() */
                && ch >= 0x5f && ch <= 0x7e
#endif
            ) {
                graph_off();
                GFlag = FALSE;
            }
            xputc_core_wt(ch, 1);
        }
#else /* ?ASCIIGRAPH */
        xputc_core_wt(ch, 1);

#endif /* ASCIIGRAPH */
        return;
    } else {
        /* old ConsoleAPI */
        set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
        inverse = (console.api.nhattr[ATR_INVERSE] && iflags.wc_inverse);
        console.api.attribute =
            (console.api.nhattr[ATR_INVERSE] && iflags.wc_inverse)
                ? ttycolors_inv[console.api.color]
                : ttycolors[console.api.color];
        if (console.api.nhattr[ATR_BOLD])
            console.api.attribute |=
                (inverse) ? BACKGROUND_INTENSITY : FOREGROUND_INTENSITY;
        cell.attribute_consoleAPI = console.api.attribute;
        cell.character_consoleAPI = (console.has_unicode ? cp437[ch] : ch);
        buffer_write_ConsoleAPI(console.api.back_buffer, &cell, console.cursor);
    }
}

/*
 * Overrides wintty.c function of the same name
 * for win32. It is used for glyphs only, not text and
 * only when a UTF-8 sequence is involved for the
 * representation. Single character representations
 * use g_putch() instead.
 */
void
g_pututf8(uint8 *sequence)
{
    DWORD ccount = 0;
    static uint8 utf8str[MAX_UTF8_SEQUENCE];

    if (console.WindowsTerminal) {
#ifdef UTF8_FROM_CORE
        ccount = snprintf((char *) utf8str, sizeof utf8str, "%s",
                          (char *) sequence);

        if (ttyDisplay->topl_utf8 && (windowprocs.wincap2 & WC2_U_UTF8STR)) {
            ttyDisplay->curx++;
        }
        buffer_write_wt(utf8str, ccount);
#endif /* UTF8_FROM_CORE */
    }
}

const char *
win32con_topl_fixup(const char *str)
{
#ifdef ENHANCED_SYMBOLS
    if (ttyDisplay->topl_utf8 && (windowprocs.wincap2 & WC2_U_UTF8STR)
        && SYMHANDLING(H_UTF8)) {
        int i;
        uint8 seq[7];
        const char *c = str;

        for (i = 0; i < sizeof seq - 1 && *c && *c != ' '; ++i) {
            seq[i] = (uint8) *c;
            c++;
            str++;
        }
        seq[i] = '\0';
        g_pututf8(seq);
    }
#endif
    return str;
}

void
cl_end(void)
{
    if (console.WindowsTerminal) {
        emit_clear_to_eol();
    } else {
        /* console.ConsoleAPI */
        if (ttyDisplay->curx < console.width
            && ttyDisplay->cury < console.height) {
            set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
            buffer_clear_to_eol_ConsoleAPI(
                console.api.back_buffer, console.cursor.X, console.cursor.Y);
            tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1,
                     (int) ttyDisplay->cury);
        }
    }
}
void
term_clear_screen(void)
{
    if (console.WindowsTerminal) {
        emit_clear_screen();
        home();
        /* set remembered data to all spaces */
        erase_tty_screen();
    } else {
        /* console.ConsoleAPI */
        buffer_fill_to_end_ConsoleAPI(console.api.back_buffer, &clear_cell, 0, 0);
        home();
    }
}

void
home(void)
{
    if (console.WindowsTerminal) {
        emit_home();
    } else {
        /* console.ConsoleAPI */
        set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
    }
    ttyDisplay->curx = ttyDisplay->cury = 0;
}

void
standoutbeg(void)
{
    if (console.WindowsTerminal) {
        emit_start_inverse();
    } else {
        /* console.ConsoleAPI */
        term_start_attr(ATR_BOLD);
    }
}

void
standoutend(void)
{
    if (console.WindowsTerminal) {
        emit_stop_inverse();
    } else {
        /* console.ConsoleAPI */
        term_end_attr(ATR_BOLD);
    }
}

void
backsp(void)
{
    if (console.WindowsTerminal) {
        emit_cursor_left();
    } else {
        /* console.ConsoleAPI */
        set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
        if (console.WindowsTerminal)
            xputc_core_wt('\b', 0);
        else
            xputc_core_ConsoleAPI('\b');
    }
}

void
tty_nhbell(void)
{
    if (flags.silent || iflags.debug_fuzzer)
        return;
    Beep(8000, 500);
}

/* stub tcap replacements for linkage from wintty.c */

#ifdef ASCIIGRAPH
void
graph_on(void)
{
    if (console.WindowsTerminal)
        emit_AS();
}

void
graph_off(void)
{
    if (console.WindowsTerminal)
        emit_AE();
}
#endif

volatile int junk; /* prevent optimizer from eliminating loop below */

void
tty_delay_output(void)
{
    /* delay 50 ms - uses ANSI C clock() function now */
    clock_t goal;
    int k;

    if (console.WindowsTerminal) {
        /* do nothing */
    } else {
        /* console.ConsoleAPI */
        back_buffer_flip_ConsoleAPI();
    }
    if (iflags.debug_fuzzer)
        return;
    goal = 50 + clock();
    while (goal > clock()) {
        k = junk; /* Do nothing */
    }
    nhUse(k);
}

void
cl_eos(void)
{
    if (console.WindowsTerminal) {
        static const char escseq[] = "\x1b[J";
        static size_t sz = sizeof escseq - 1;

        buffer_write_wt((uint8_t *) escseq, sz);
    } else {
        buffer_fill_to_end_ConsoleAPI(console.api.back_buffer, &clear_cell,
                                      ttyDisplay->curx, ttyDisplay->cury);
        tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1,
                 (int) ttyDisplay->cury);
    }
}

static char nulstr[] = "";
static char ZH[] = "\x1b[3m", ZR[] = "\x1b[23m", MB[] = "\x1b[5m",
            reset_MB[] = "\x1b[25m", nh_US[] = "\x1b[4m", UE[] = "\x1b[24m",
            MD[] = "\x1b[1m", HE[] = "\x1b[21m", MR[] = "\x1b[7m",
            reset_MR[] = "\x1b[27m", MH[] = "\x1b[2m",
            reset_MH[] = "\x1b[22m", ME[] = "\x1b[0m";

struct atr_s {
    char *escseq;
    size_t escseq_sz;
};
static struct atr_s atrtable[] = {
    { ME, sizeof ME - 1 },       /* ATR_NONE    0 */
    { MD, sizeof MD - 1 },       /* ATR_BOLD    1 */
    { MH, sizeof MH - 1 },       /* ATR_DIM     2 */
    { ZH, sizeof ZH - 1 },       /* ATR_ITALIC  3 */
    { nh_US, sizeof nh_US - 1 }, /* ATR_ULINE   4 */
    { MB, sizeof MB - 1 },       /* ATR_BLINK   5 */
    { ME, sizeof ME - 1 },       /* filler      6 */
    { MR, sizeof MR - 1 },       /* ATR_INVERSE 7 */
};
static struct atr_s atrtable_r[] = {
    { ME, sizeof ME - 1 },             /* ATR_NONE    0 */
    { HE, sizeof HE - 1 },             /* ATR_BOLD    1 */
    { reset_MH, sizeof reset_MH - 1 }, /* ATR_DIM     2 */
    { ZR, sizeof ZR - 1 },             /* ATR_ITALIC  3 */
    { UE, sizeof UE - 1 },             /* ATR_ULINE   4 */
    { reset_MB, sizeof reset_MB - 1 }, /* ATR_BLINK   5 */
    { ME, sizeof ME - 1 },             /* filler      6 */
    { reset_MR, sizeof reset_MR - 1 }, /* ATR_INVERSE 7 */
};

static char *
s_atr2str(int n)
{
    if (n >= 0 && n < SIZE(atrtable))
        return atrtable[n].escseq;
    return nulstr;
}

static char *
e_atr2str(int n)
{
    if (n >= 0 && n < SIZE(atrtable))
        return atrtable_r[n].escseq;
    return nulstr;
}

/* suppress nonfunctional highlights so render_status() might be able to
   optimize more; keep this in sync with s_atr2str() */
int
term_attr_fixup(int attrmsk)
{
    if (console.WindowsTerminal) {
        /* underline is converted to bold if its start sequence isn't
         * available */
        if ((attrmsk & HL_ULINE) && (!nh_US || !*nh_US)) {
            attrmsk |= HL_BOLD;
            attrmsk &= ~HL_ULINE;
        }
        /* blink used to be converted to bold unconditionally; now depends on
         * MB
         */
        if ((attrmsk & HL_BLINK) && (!MB || !*MB)) {
            attrmsk |= HL_BOLD;
            attrmsk &= ~HL_BLINK;
        }
        /* dim is ignored if its start sequence isn't available */
        if ((attrmsk & HL_DIM) && (!MH || !*MH)) {
            attrmsk &= ~HL_DIM;
        }
        return attrmsk;
    } else {
        /* console.ConsoleAPI */
        return attrmsk;
    }
}

void
term_start_attr(int attrib)
{
    if (attrib) {
        if (console.WindowsTerminal) {
            const char *astr = s_atr2str(attrib);

            if (astr && *astr)
                buffer_write_wt((uint8_t *) astr, strlen(astr));
        } else {
            /* console.ConsoleAPI */
            switch (attrib) {
            case ATR_INVERSE:
                console.api.attribute |= atr_inverse;
                break;
            case ATR_ULINE:
                console.api.attribute |= atr_uline;
                break;
            case ATR_BLINK:
                console.api.attribute |= atr_blink;
                break;
            case ATR_BOLD:
                console.api.attribute |= atr_bold;
                break;
            }
            console.api.nhattr[attrib] = TRUE;
        }
    } else {
        console.api.nhattr[ATR_NONE] = FALSE;
    }
}

void
term_end_attr(int attrib)
{
    if (console.WindowsTerminal) {
        const char *astr = e_atr2str(attrib);

        if (astr && *astr)
            buffer_write_wt((uint8_t *) astr, strlen(astr));
    } else {
        /* console.ConsoleAPI */
        int k;

        switch (attrib) {
        case ATR_INVERSE:
            console.api.attribute &= ~atr_inverse;
            break;
        case ATR_ULINE:
            console.api.attribute &= ~atr_uline;
            break;
        case ATR_BLINK:
            console.api.attribute &= ~atr_blink;
            break;
        case ATR_BOLD:
            console.api.attribute &= ~atr_bold;
            break;
        }
        console.api.nhattr[attrib] = FALSE;
        console.api.nhattr[ATR_NONE] = TRUE;
        /* re-evaluate all attr now for performance at output time */
        for (k = ATR_NONE; k <= ATR_INVERSE; ++k) {
            if (console.api.nhattr[k])
                console.api.nhattr[ATR_NONE] = FALSE;
        }
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
    if (console.WindowsTerminal) {
        if (color == NO_COLOR)
            emit_HE(); /* inline term_end_color() */
        buffer_write_wt((uint8_t *) esc_seq_colors[color],
                        strlen(esc_seq_colors[color]));
    } else {
        /* console.ConsoleAPI */
        if (color == NO_COLOR) {
            term_end_color();
        } else if (color >= 0 && color < CLR_MAX) {
            console.api.color = color;
        } else {
            console.api.color = NO_COLOR;
        }
    }
}

void
term_start_bgcolor(int color)
{
    if (console.WindowsTerminal) {
#if 0
        Sprintf(tmp, "\033[%dm", ((color % 8) + 40));
#endif
        buffer_write_wt((uint8_t *) esc_seq_bkcolors[color],
                        strlen(esc_seq_bkcolors[color]));
    } else {
        /* console.ConsoleAPI */
        if (color != NO_COLOR && (color >= 0 && color < CLR_MAX)) {
            console.api.bkcolor = color;
        } else {
            console.api.bkcolor = NO_COLOR;
        }
    }
}

void
term_end_color(void)
{
    if (console.WindowsTerminal) {
        emit_HE();
    } else {
        /* console.ConsoleAPI */
        console.foreground = DEFTEXTCOLOR;
        if (console.ConsoleAPI)
            console.api.attribute =
                (console.foreground | console.background);
        console.api.color = NO_COLOR;
        console.api.bkcolor = NO_COLOR;
    }
}

void
term_curs_set(int visibility)
{
    static int vis = -1;

    if (vis == visibility)
        return;

    if (console.WindowsTerminal) {
        if (visibility) {
            emit_show_cursor();
        } else {
            emit_hide_cursor();
        }
    } else {
        /* console.ConsoleAPI */
        static CONSOLE_CURSOR_INFO cursorinfo = { 0, 0 };

        if (!cursorinfo.dwSize) {
            GetConsoleCursorInfo(console.hConOut, &cursorinfo);
            vis = cursorinfo.bVisible ? 1 : 0;
        }
        cursorinfo.bVisible = visibility ? (BOOL) TRUE : (BOOL) FALSE;
        SetConsoleCursorInfo(console.hConOut, &cursorinfo);
    }
    vis = visibility;
}

static void
emit24bit(long mcolor)
{
    static char buf[QBUFSZ];
    size_t sz;

    sz =
        snprintf(buf, sizeof buf, tcfmtstr, ((mcolor >> 16) & 0xFF), /* red */
                 ((mcolor >> 8) & 0xFF),  /* green */
                 ((mcolor >> 0) & 0xFF)); /* blue */
    buffer_write_wt((uint8_t *) buf, sz);
}

static void
emit256(int color256idx)
{
    static char buf[15];
    size_t sz;

    sz = sprintf(buf, tcfmtstr256, color256idx);
    buffer_write_wt((uint8_t *) buf, sz);
}

void
term_start_extracolor(uint32 customcolor, uint16 color256idx)
{
    /* color 0 has bit 0x1000000 set */
    long mcolor = (customcolor & 0xFFFFFF);
    if (iflags.colorcount == 256)
        emit256(color256idx);
    else
        emit24bit(mcolor);
}

void
term_end_extracolor(void)
{
    static char escseq[] = "\x1b[0m";
    static size_t sz = sizeof escseq;

    buffer_write_wt((uint8_t *) escseq, sz);
}


void buffer_fill_to_end_ConsoleAPI(cell_t * buffer, cell_t * fill, int x, int y);
void buffer_write_ConsoleAPI(cell_t * buffer, cell_t * cell, COORD pos);
static int32 rgbtable_to_int32(const struct nethack_color *tbl);
void term_start_256color(int idx);
void set_cp_map(void);
#ifdef PORT_DEBUG
void win32con_debug_keystrokes(void);
void win32con_toggle_cursor_info(void);
#endif

/* GDI typically represents color as BGR (Blue-Green-Red) in memory */
static int32
colortable_to_bgr_int32(const struct nethack_color *tbl)
{
    int32 bgrint32 = (tbl->r << 0) | (tbl->g << 8) | (tbl->b << 16);
    return bgrint32;
}

#define rgbtable_offset 16

static void
init_custom_colors(void)
{
    char bkcolorbuf[32];

    customcolors[CLR_BLACK] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 131]);
    customcolors[CLR_RED] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 5]);
    customcolors[CLR_GREEN] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 31]);
    customcolors[CLR_BROWN] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 104]);
    customcolors[CLR_BLUE] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 58]);
    customcolors[CLR_MAGENTA] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 76]);
    customcolors[CLR_CYAN] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 48]);
    customcolors[CLR_GRAY] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 73]);
    customcolors[NO_COLOR] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 137]);
    customcolors[CLR_ORANGE] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 15]);
    customcolors[CLR_BRIGHT_GREEN] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 34]);
    customcolors[CLR_YELLOW] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 18]);
    customcolors[CLR_BRIGHT_BLUE] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 69]);
    customcolors[CLR_BRIGHT_MAGENTA] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 84]);
    customcolors[CLR_BRIGHT_CYAN] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 49]);
    customcolors[CLR_WHITE] = colortable_to_bgr_int32(&colortable[rgbtable_offset + 138]);

/*    esc_seq_colors[CLR_BLACK] = "\x1b[30m"; */
    esc_seq_colors[CLR_BLACK] = "\x1b[38;2;47;79;79m";
    esc_seq_colors[CLR_RED] = "\x1b[31m";
    esc_seq_colors[CLR_GREEN] = "\x1b[32m";
    esc_seq_colors[CLR_YELLOW] = "\x1b[38;2;255;255;0m";
    esc_seq_colors[CLR_BLUE] = "\x1b[38;2;100;149;237m";
    esc_seq_colors[CLR_MAGENTA] = "\x1b[35m";
    esc_seq_colors[CLR_CYAN] = "\x1b[36m";
    esc_seq_colors[CLR_WHITE] = "\x1b[37m";

    esc_seq_colors[CLR_BROWN] = "\x1b[38;2;205;133;63m";
//    esc_seq_colors[CLR_GRAY] = "\x1b[31m\x1b[32m\x1b[34m";
    esc_seq_colors[CLR_GRAY] = "\x1b[90m";
    esc_seq_colors[NO_COLOR] = "\x1b[39m";
    esc_seq_colors[CLR_ORANGE] = "\x1b[38;2;255;140;0m";
    esc_seq_colors[CLR_BRIGHT_GREEN] = "\x1b[39m";
    esc_seq_colors[CLR_BRIGHT_BLUE] = "\x1b[34m\x1b[94m";
    esc_seq_colors[CLR_BRIGHT_MAGENTA] = "\x1b[35m\x1b[95m";
    esc_seq_colors[CLR_BRIGHT_CYAN] = "\x1b[36m\x1b[96m";

     /* Sprintf(tmp, "\033[%dm", ((color % 8) + 40)); */
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_BLACK % 8) + 40));
    esc_seq_bkcolors[CLR_BLACK] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_RED % 8) + 40));
    esc_seq_bkcolors[CLR_RED] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_GREEN % 8) + 40));
    esc_seq_bkcolors[CLR_GREEN] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_YELLOW % 8) + 40));
    esc_seq_bkcolors[CLR_YELLOW] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_BLUE % 8) + 40));
    esc_seq_bkcolors[CLR_BLUE] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_MAGENTA % 8) + 40));
    esc_seq_bkcolors[CLR_MAGENTA] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_CYAN % 8) + 40));
    esc_seq_bkcolors[CLR_CYAN] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_WHITE % 8) + 40));
    esc_seq_bkcolors[CLR_WHITE] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_BROWN % 8) + 40));
    esc_seq_bkcolors[CLR_BROWN] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_GRAY % 8) + 40));
    esc_seq_bkcolors[CLR_GRAY] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((NO_COLOR % 8) + 40));
    esc_seq_bkcolors[NO_COLOR] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_ORANGE % 8) + 40));
    esc_seq_bkcolors[CLR_ORANGE] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_BRIGHT_BLUE % 8) + 40));
    esc_seq_bkcolors[CLR_BRIGHT_BLUE] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_BRIGHT_GREEN % 8) + 40));
    esc_seq_bkcolors[CLR_BRIGHT_GREEN] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_BRIGHT_MAGENTA % 8) + 40));
    esc_seq_bkcolors[CLR_BRIGHT_MAGENTA] = dupstr(bkcolorbuf);
    Snprintf(bkcolorbuf, sizeof bkcolorbuf, "\x1b[%dm", ((CLR_BRIGHT_CYAN % 8) + 40));
    esc_seq_bkcolors[CLR_BRIGHT_CYAN] = dupstr(bkcolorbuf);
}

static void
free_custom_colors(void)
{
#define CLR_FREE(c) \
    if (esc_seq_bkcolors[(c)] != 0) \
        free((genericptr_t) esc_seq_bkcolors[(c)]), esc_seq_bkcolors[(c)] = 0

    CLR_FREE(CLR_BLACK);
    CLR_FREE(CLR_RED);
    CLR_FREE(CLR_GREEN);
    CLR_FREE(CLR_YELLOW);
    CLR_FREE(CLR_BLUE);
    CLR_FREE(CLR_MAGENTA);
    CLR_FREE(CLR_CYAN);
    CLR_FREE(CLR_WHITE);
    CLR_FREE(CLR_BROWN);
    CLR_FREE(CLR_GRAY);
    CLR_FREE(NO_COLOR);
    CLR_FREE(CLR_ORANGE);
    CLR_FREE(CLR_BRIGHT_GREEN);
    CLR_FREE(CLR_BRIGHT_BLUE);
    CLR_FREE(CLR_BRIGHT_MAGENTA);
    CLR_FREE(CLR_BRIGHT_CYAN);
#undef CLR_FREE
}

    
static boolean color24_on = TRUE;

/* for debugging */
WORD what_is_there_now;
//BOOL success;
DWORD error_result;

/* Console buffer flipping support */
enum do_flags {
    do_utf8_content = 0x01,
    do_wide_content = 0x02,
    do_colorseq     = 0x04,
    do_color24      = 0x08,
    do_newattr      = 0x10,
    do_bkcolorseq   = 0x20,
};
enum did_flags {
    did_utf8_content = 0x01,
    did_wide_content = 0x02,
    did_colorseq     = 0x04,
    did_color24      = 0x08,
    did_newattr      = 0x10,
    did_bkcolorseq   = 0x20,
};

static void
back_buffer_flip_ConsoleAPI(void)
{
    cell_t *back = console.api.back_buffer;
    cell_t *front = console.api.front_buffer;
    COORD pos;
    DWORD unused;

    if (!console.is_ready || !console.api.back_buffer || !console.api.front_buffer)
        return;

    for (pos.Y = 0; pos.Y < console.height; pos.Y++) {
        for (pos.X = 0; pos.X < console.width; pos.X++) {
            if (back->attribute_consoleAPI != front->attribute_consoleAPI) {
                WriteConsoleOutputAttribute(console.hConOut, &back->attribute_consoleAPI,
                                            1, pos, &unused);
                front->attribute_consoleAPI = back->attribute_consoleAPI;
            }
            if (back->character_consoleAPI != front->character_consoleAPI) {
                if (console.has_unicode) {
                    WriteConsoleOutputCharacterW(
                        console.hConOut, &back->character_consoleAPI, 1, pos, &unused);
                } else {
                    char ch = (char) back->character_consoleAPI;
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

void
buffer_fill_to_end_ConsoleAPI(cell_t *buffer, cell_t *fill, int x, int y)
{
    if (!console.is_ready || !console.api.back_buffer || !console.api.front_buffer)
        return;

    nhassert(x >= 0 && x < console.width);
    nhassert(y >= 0
             && ((y < console.height) || (y == console.height && x == 0)));

    cell_t *dst = buffer + console.width * y + x;
    cell_t *sentinel = buffer + console.buffer_size;
    while (dst != sentinel)
        *dst++ = *fill;

    if ((iflags.debug.immediateflips || !program_state.in_moveloop)
        && buffer == console.api.back_buffer) {
        back_buffer_flip_ConsoleAPI();
    }
}

static void
buffer_clear_to_eol_ConsoleAPI(cell_t *buffer, int x, int y)
{

    if (!console.is_ready || !console.api.back_buffer || !console.api.front_buffer)
    return;

    nhassert(x >= 0 && x < console.width);
    nhassert(y >= 0
             && ((y < console.height) || (y == console.height && x == 0)));
    cell_t *dst = buffer + console.width * y + x;
    cell_t *sentinel = buffer + console.width * (y + 1);

    while (dst != sentinel)
        *dst++ = clear_cell;

    if (iflags.debug.immediateflips || !program_state.in_moveloop) {
        back_buffer_flip_ConsoleAPI();
    }
}

void
buffer_write_ConsoleAPI(cell_t *buffer, cell_t *cell, COORD pos)
{

    if (!console.is_ready || !console.api.back_buffer || !console.api.front_buffer)
        return;

    if (console.WindowsTerminal) {
          /* do nothing */
    } else {
        /* console.ConsoleAPI */
        cell_t *dst = buffer + (console.width * pos.Y) + pos.X;

        *dst = *cell;

        nhassert(pos.X >= 0 && pos.X < console.width);
        nhassert(pos.Y >= 0 && pos.Y < console.height);

        if ((iflags.debug.immediateflips || !program_state.in_moveloop)
            && buffer == console.api.back_buffer) {
            back_buffer_flip_ConsoleAPI();
        }
    }
}

static boolean
OnWindows95_98_Me(void)
{
    return ((GetVersion() & 0x80000000) != 0);
}

//#include <windows.h>
//#include <stdio.h>
//#include <wchar.h>

#define MAX_PROC_ITERATIONS 10

boolean
confirm_windows_terminal(void)
{
    DWORD curpid = GetCurrentProcessId();
    DWORD parentpid = 0;
    PROCESSENTRY32W process_entry;
    HANDLE snapshot_handle;
    process_entry.dwSize = sizeof(PROCESSENTRY32W);
    int num_ancestors = 0;
    boolean result = FALSE;

    snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot_handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (Process32FirstW(snapshot_handle, &process_entry)) {
        do {
            if (process_entry.th32ProcessID == curpid) {
                parentpid = process_entry.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(snapshot_handle, &process_entry));
    }

    while (parentpid != 0 && num_ancestors < MAX_PROC_ITERATIONS) {
        DWORD nextparentpid = 0;
        BOOL foundParent = FALSE;

        if (Process32FirstW(snapshot_handle, &process_entry)) {
            do {
                if (process_entry.th32ProcessID == parentpid) {
                    // Check if the current ancestor is Windows Terminal
                    if ((_wcsicmp(process_entry.szExeFile, L"WindowsTerminal.exe") == 0)
                        || (_wcsicmp(process_entry.szExeFile, L"vsDebugConsole.exe") == 0)) {
                        result = TRUE;
                        parentpid = 0; // Terminate loop safely
                        foundParent = TRUE;
                        break;
                    }
                    // Prepare to move up to the grandparent process
                    nextparentpid = process_entry.th32ParentProcessID;
                    foundParent = TRUE;
                    break;
                }
            } while (Process32NextW(snapshot_handle, &process_entry));
        }

        if (!foundParent || nextparentpid == parentpid) {
            break; 
        }
        parentpid = nextparentpid;
        num_ancestors++;
    }
    CloseHandle(snapshot_handle);
    return result;
}

/* nethack_enter_consoletty() is the first thing that is called from main
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

DISABLE_WARNING_CONDEXPR_IS_CONSTANT

void
nethack_enter_consoletty(void)
{
    int width;
    /* console.WindowsTerminal */
    char buf[BUFSZ], *bp, *localestr;
    BOOL apisuccess;
    DWORD mask = 0, disablemask = 0;

#if 0
    /* prevent re-sizing of the console window */
    if (!console.hWnd)
        console.hWnd = GetConsoleWindow();
    SetWindowLong(console.hWnd, GWL_STYLE,
                  GetWindowLong(console.hWnd, GWL_STYLE)
                     & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
#endif
    if ((iflags.wt_how == wt_not_in_config_at_all)
        /* the default is WindowsTerminal now */
        || (iflags.wt_how == wt_in_config_not_negated)) {
        console.WindowsTerminal = TRUE;
    } else if ((iflags.wt_how == wt_in_config_negated)
               || !confirm_windows_terminal()) {
        console.WindowsTerminal = FALSE;
    }
    console.ConsoleAPI = !console.WindowsTerminal;
    gn.no_terms = console.ConsoleAPI;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        /* srWindow identifies the visible area; dwSize identifies the buffer
         */
        width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#ifdef DEBUG
        if (wizard && NH_DEVEL_STATUS != NH_STATUS_RELEASED)
            fprintf(stdout, "width = %d\n", width);
#endif
    }

    console.hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    nhassert(console.hConOut != NULL); // NOTE: this assert will not print
    GetConsoleScreenBufferInfo(console.hConOut, &console.orig.csbi);
    // COORD screencheck = GetLargestConsoleWindowSize(console.hConOut);

    GetConsoleMode(console.hConOut, &console.orig.out_cmode);
    console.active.out_cmode = console.orig.out_cmode;
    if (console.WindowsTerminal) {
        console.active.out_cmode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(console.hConOut, console.active.out_cmode);
#if 0
        /* tests */
        WriteConsoleA(console.hConOut, "\033[8;;133t",9, &unused, NULL);
        for (i = 0; i < 13; ++i) {
            WriteConsoleA(console.hConOut, "0123456789", 10, &unused, NULL);
        }
        WriteConsoleA(console.hConOut, "\033[3;133ftest", 12, &unused, NULL);
        GetConsoleScreenBufferInfo(console.hConOut, &console.orig.csbi);
        /* Testing of widths != COLNO has not turned up any problems.  Need
         * to do a bit more testing and then we are likely to enable having
         * console width match window width.
         */
#endif
    }
    console.width = console.orig.csbi.srWindow.Right
                    - console.orig.csbi.srWindow.Left + 1;
    if (console.width < COLNO)
        console.width = COLNO;

    console.height = console.orig.csbi.srWindow.Bottom
                     - console.orig.csbi.srWindow.Top + 1;
    //    console.height = max(console.height, ROWNO + 3);
    if (console.height < (ROWNO + 2 + 1))
        console.height = (ROWNO + 2 + 1);
    console.buffer_size = console.width * console.height;

    /* clear the entire console buffer */
    // int size = console.orig.csbi.dwSize.X * console.orig.csbi.dwSize.Y;
    // DWORD unused;
    // set_console_cursor(0, 0);
    //  FillConsoleOutputAttribute(
    //      console.hConOut, CONSOLE_CLEAR_ATTRIBUTE,
    //      size, console.cursor, &unused);

    //  FillConsoleOutputCharacter(
    //      console.hConOut, CONSOLE_CLEAR_CHARACTER,
    //      size, console.cursor, &unused);

    // set_console_cursor(1, 0);
    // SetConsoleCursorPosition(console.hConOut, console.cursor);

    /* At this point early_raw_print will work */

    console.hConIn = GetStdHandle(STD_INPUT_HANDLE);
    nhassert(console.hConIn != NULL);

    /* grow the size of the console buffer if it is not wide enough */
    if (console.orig.csbi.dwSize.X < console.width) {
        COORD screen_size = { 0 };

        screen_size.Y = console.orig.csbi.dwSize.Y,
        screen_size.X = console.width;
        SetConsoleScreenBufferSize(console.hConOut, screen_size);
    }

    if (console.WindowsTerminal) {
        //ring_buffer_init(&console.rb);
    } else {
        /* ConsoleAPI */
        /* setup front and back buffers */
        int buffer_size_bytes = sizeof(cell_t) * console.buffer_size;

        console.api.front_buffer = (cell_t *) alloc(buffer_size_bytes);
        buffer_fill_to_end_ConsoleAPI(console.api.front_buffer, &undefined_cell,
                                      0, 0);

        console.api.back_buffer = (cell_t *) alloc(buffer_size_bytes);
        buffer_fill_to_end_ConsoleAPI(console.api.back_buffer, &clear_cell, 0, 0);
    }
    /* determine whether OS version has unicode support */
    /* console.has_unicode = (IsWindows8OrGreater()); */
    console.has_unicode = !OnWindows95_98_Me();
    /* store the original code page*/
    console.orig.code_page = GetConsoleOutputCP();
    if (console.WindowsTerminal) {
        /* store the original locale */
        console.orig.localestr = NULL;
        localestr = setlocale(LC_ALL, "");
        if (localestr)
            console.orig.localestr = dupstr(localestr);

        /* store the original font */
        console.orig.font_info.cbSize = sizeof(console.orig.font_info);
        apisuccess = GetCurrentConsoleFontEx(console.hConOut, FALSE,
                                             &console.orig.font_info);
        console.active.font_info = console.orig.font_info;

        /* adjust the locale */
        Snprintf(buf, sizeof buf, "%s", console.orig.localestr);
        if ((bp = strstri(buf, ".utf8")) != 0)
            *bp = '\0';
        localestr = setlocale(LC_ALL, buf);
        if (localestr) {
            if (console.active.localestr)
                free(console.active.localestr);
            console.active.localestr = dupstr(localestr);
        }
        if (console.has_unicode) {
            if (console.active.code_page != 437)
                apisuccess = SetConsoleOutputCP(437);
        } else if (console.active.code_page != 1252) {
            apisuccess = SetConsoleOutputCP(1252);
        }
    }
    console.active.code_page = GetConsoleOutputCP();
    /* check the font before we capture the code page map */
    check_and_set_font();
    set_cp_map();
    /* input console mode */
    GetConsoleMode(console.hConIn, &console.orig.in_cmode);
    console.active.in_cmode = console.orig.in_cmode;
    if (console.WindowsTerminal) {
        GetConsoleMode(console.hConOut, &console.orig.out_cmode);
    }
    disablemask = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT
                  | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT;
#ifdef NO_MOUSE_ALLOWED
    disablemask |= ENABLE_MOUSE_INPUT;
#else
    mask |= ENABLE_MOUSE_INPUT;
#endif
    /* Turn ON the settings specified in the mask */
    console.active.in_cmode |= mask;
    /* Turn OFF the settings specified in the disabledmask */
    console.active.in_cmode &= ~disablemask;

    SetConsoleMode(console.hConIn, console.active.in_cmode);
    GetConsoleMode(console.hConIn, &console.active.in_cmode);
    if (console.WindowsTerminal) {
        console.active.out_cmode = console.orig.out_cmode;
        if ((console.active.out_cmode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)
            == 0) {
            /* recognize escape sequences */
            console.active.out_cmode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(console.hConOut, console.active.out_cmode);
        }
        GetConsoleMode(console.hConOut, &console.active.out_cmode);
    }
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
    if (console.WindowsTerminal) {
        init_custom_colors();
    } else {
        /* console.ConsoleAPI */
        console.api.color = NO_COLOR;
        console.api.bkcolor = 0;
    }
    console.is_ready = TRUE;
    nhUse(apisuccess);
}

/*
 * Called after returning from ! or ^Z
 */
void
gettty(void)
{
    erase_char = '\b';
    kill_char = 21; /* cntl-U */
    iflags.cbreak = TRUE;
    init_ttycolor();
    settty_needed = TRUE;
}

/* reset terminal to original state */
void
settty(const char *s)
{
#ifdef TTY_GRAPHICS
    if (WINDOWPORT(tty))
        term_end_screen();
#endif
    if (s)
        raw_print(s);
    restore_original_console_font();
    SetConsoleMode(console.hConIn, console.orig.in_cmode);
    SetConsoleMode(console.hConOut, console.orig.out_cmode);
    settty_needed = FALSE;
}

/* called by init_nhwindows() and resume_nhwindows() */
void
setftty(void)
{
#ifdef TTY_GRAPHICS
    if (WINDOWPORT(tty))
        term_start_screen();
#endif
}

static BOOL
CtrlHandler(DWORD ctrltype)
{
    switch (ctrltype) {
    /* case CTRL_C_EVENT: */
    case CTRL_BREAK_EVENT:
        term_clear_screen();
        FALLTHROUGH;
        /* FALLTHRU */
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
consoletty_open(int mode UNUSED)
{
    int debugvar;

    /* Initialize the function pointer that points to
     * the kbhit() equivalent, in this TTY case consoletty_kbhit()
     */
    nt_kbhit = consoletty_kbhit;

    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, TRUE)) {
        /* Unable to set control handler */
        debugvar = 0; /* just to have a statement to break on for debugger */
    }

    LI = console.height;
    CO = console.width;

    really_move_cursor();
    nhUse(debugvar);
}
extern void set_emergency_io(void);

void
console_exit(void)
{
    free_custom_colors();
    if (console.api.front_buffer)
        free((genericptr_t) console.api.front_buffer);
    if (console.api.back_buffer)
        free((genericptr_t) console.api.back_buffer);
    console.api.front_buffer = console.api.back_buffer = 0;
    if (console.active.localestr)
        free((genericptr_t) console.active.localestr), console.active.localestr = 0;
    if (console.orig.localestr)
        free((genericptr_t) console.orig.localestr),
            console.orig.localestr = 0;
    set_emergency_io();
}

int
process_keystroke(
    INPUT_RECORD *ir,
    boolean *valid,
    uchar numberpad,
    int portdebug)
{
    int ch;

#ifdef QWERTZ_SUPPORT
    if (gc.Cmd.swap_yz)
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
consoletty_kbhit(void)
{
    return keyboard_handling.pNHkbhit(console.hConIn, &gbl_ir);
}

int
tgetch(void)
{
    int mod;
    coord cc;
    DWORD count;
    uchar numberpad = iflags.num_pad;

    really_move_cursor();
    if (iflags.debug_fuzzer)
        return randomkey();
#ifdef QWERTZ_SUPPORT
    if (gc.Cmd.swap_yz)
        numberpad |= 0x10;
#endif

    return (program_state.done_hup)
               ? '\033'
               : keyboard_handling.pCheckInput(
                   console.hConIn, &gbl_ir, &count, numberpad, 0, &mod, &cc);
}

int
console_poskey(coordxy *x, coordxy *y, int *mod)
{
    int ch;
    coord cc = { 0, 0 };
    DWORD count;
    boolean numberpad = iflags.num_pad;

    really_move_cursor();
    if (iflags.debug_fuzzer) {
        int poskey = randomkey();

        if (poskey == 0) {
            *x = rn2(console.width);
            *y = rn2(console.height);
        }
        return poskey;
    }
#ifdef QWERTZ_SUPPORT
    if (gc.Cmd.swap_yz)
        numberpad |= 0x10;
#endif
    term_curs_set(1);
    ch = (program_state.done_hup)
             ? '\033'
             : keyboard_handling.pCheckInput(
                   console.hConIn, &gbl_ir, &count, numberpad, 1, mod, &cc);
#ifdef QWERTZ_SUPPORT
    numberpad &= ~0x10;
#endif
    if (!ch) {
        *x = cc.x;
        *y = cc.y;
    }
    term_curs_set(0);
    return ch;
}

DISABLE_WARNING_FORMAT_NONLITERAL

static void 
set_console_cursor(int x, int y)
{
    if (console.WindowsTerminal) {
        static const char escseq[] = "\x1b[%d;%dH";
        char tmp[11];
        size_t sz = sizeof tmp - 1;

        tmp[0] = '\0';
        if (y < 1000 && y >= 0 && x < 1000 && x >= 0) {
            sz = sprintf(tmp, escseq, y, x);
        }
        buffer_write_wt((uint8_t *) tmp, sz);
    } else {
        /* console.ConsoleAPI) */
        nhassert(x >= 0 && x < console.width);
        nhassert(y >= 0 && y < console.height);

        console.cursor.X = max(0, min(console.width - 1, x));
        console.cursor.Y = max(0, min(console.height - 1, y));
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

static void
really_move_cursor(void)
{
    if (console.WindowsTerminal) {
        /* do nothing */
    } else {
        /* console.ConsoleAPI */
#ifdef PORT_DEBUG
        char oldtitle[BUFSZ], newtitle[BUFSZ];
        if (display_cursor_info && wizard) {
            oldtitle[0] = '\0';
            if (GetConsoleTitle(oldtitle, BUFSZ)) {
                oldtitle[39] = '\0';
            }
            Snprintf(newtitle, sizeof newtitle,
                     "%-55s tty=(%02d,%02d) consoletty=(%02d,%02d)", oldtitle,
                     ttyDisplay->curx, ttyDisplay->cury, console.cursor.X,
                     console.cursor.Y);
            (void) SetConsoleTitle(newtitle);
        }
#endif
        if (ttyDisplay)
            set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
        back_buffer_flip_ConsoleAPI();
        SetConsoleCursorPosition(console.hConOut, console.cursor);
    }
}

static void
buffer_write_wt(uint8_t *buffer, size_t sz)
{
    DWORD unused;

    if (!console.is_ready)
        return;

    WriteConsoleA(console.hConOut, (LPCSTR) buffer,
                  (int) sz, &unused, NULL);
}

void
raw_clear_screen(void)
{
    if (console.WindowsTerminal) {
        static const char escseq[] = "\x1b[2J\x1b[H";
        static size_t sz = sizeof escseq - 1;

        buffer_write_wt((uint8_t *) escseq, sz); 
    } else {
        /* console.ConsoleAPI */
        if (WINDOWPORT(tty)) {
            cell_t *back = console.api.back_buffer;
            cell_t *front = console.api.front_buffer;
            COORD pos;
            DWORD unused;

            if (console.WindowsTerminal) {
                pos.Y = 0;
                pos.X = 0;
                SetConsoleCursorPosition(console.hConOut, pos);
                emit_return_to_default();
            }
            if (console.ConsoleAPI) {
                for (pos.Y = 0; pos.Y < console.height; pos.Y++) {
                    for (pos.X = 0; pos.X < console.width; pos.X++) {
                        if (console.WindowsTerminal) {
                            *back = clear_cell;
                            if (console.has_unicode)
                                WriteConsoleW(console.hConOut,
                                              &back->wcharacter, 1, &unused,
                                              NULL);
                            else
                                WriteConsoleA(
                                    console.hConOut, (LPCSTR) back->utf8str,
                                    (int) strlen((char *) back->utf8str),
                                    &unused, NULL);
                        }
                        if (console.ConsoleAPI) {
                            WriteConsoleOutputAttribute(
                                console.hConOut, &back->attribute_consoleAPI,
                                1, pos, &unused);
                            front->attribute_consoleAPI =
                                back->attribute_consoleAPI;
                            if (console.has_unicode) {
                                WriteConsoleOutputCharacterW(
                                    console.hConOut,
                                    &back->character_consoleAPI, 1, pos,
                                    &unused);
                            } else {
                                char ch = (char) back->character_consoleAPI;
                                WriteConsoleOutputCharacterA(
                                    console.hConOut, &ch, 1, pos, &unused);
                            }
                        }
                        *front = *back;
                        back++;
                        front++;
                    }
                }
            }
        }
    }
}


/*
 * CLR_BLACK             0
 * CLR_RED               1
 * CLR_GREEN             2
 * CLR_BROWN             3  low-intensity yellow
 * CLR_BLUE              4
 * CLR_MAGENTA           5
 * CLR_CYAN              6
 * CLR_GRAY              7  low-intensity white
 * NO_COLOR              8
 * CLR_ORANGE            9
 * CLR_BRIGHT_GREEN     10
 * CLR_YELLOW           11
 * CLR_BRIGHT_BLUE      12
 * CLR_BRIGHT_MAGENTA   13
 * CLR_BRIGHT_CYAN      14
 * CLR_WHITE            15
 * CLR_MAX              16
 * BRIGHT                8
 */

static void
init_ttycolor(void)
{
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
    init_ttycolor_completed = TRUE;
}



#ifndef NO_MOUSE_ALLOWED
void
toggle_mouse_support(void)
{
    static int qeinit = 0;

    GetConsoleMode(console.hConIn, &console.active.in_cmode);
    if (!qeinit) {
        qeinit = 1;
        console.orig.QuickEdit =
            ((console.active.in_cmode & ENABLE_QUICK_EDIT_MODE) != 0);
    }

    switch (iflags.wc_mouse_support) {
    case 2:
        console.active.in_cmode |= ENABLE_MOUSE_INPUT;
        break;
    case 1:
        console.active.in_cmode |= ENABLE_MOUSE_INPUT;
        console.active.in_cmode &= ~ENABLE_QUICK_EDIT_MODE;
        console.active.in_cmode |= ENABLE_EXTENDED_FLAGS;
        break;
    case 0:
        FALLTHROUGH;
        /*FALLTHRU*/
    default:
        console.active.in_cmode &= ~ENABLE_MOUSE_INPUT;
        if (console.orig.QuickEdit) {
            console.active.in_cmode |=
                (ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS);
        }
    }
    SetConsoleMode(console.hConIn, console.active.in_cmode);
}
#endif  /* NO_MOUSE_ALLOWED */

/* handle tty options updates here */
void
consoletty_preference_update(const char *pref)
{
    if (stricmp(pref, "mouse_support") == 0) {
#ifndef NO_MOUSE_ALLOWED
        toggle_mouse_support();
#endif
    }
    if (stricmp(pref, "symset") == 0) {
#ifdef UTF8_FROM_CORE
        if ((tty_procs.wincap2 & WC2_U_UTF8STR) && SYMHANDLING(H_UTF8)) {
#ifdef ENHANCED_SYMBOLS
            tty_utf8graphics_fixup();
#endif
        } else if ((tty_procs.wincap2 & WC2_U_UTF8STR) && SYMHANDLING(H_IBM)) {
#else
        if (SYMHANDLING(H_IBM)) {
#endif
            tty_ibmgraphics_fixup();
        }
        check_and_set_font();
    }
    return;
}

#ifdef UTF8_FROM_CORE
/*
 * This is called when making the switch to a symset
 * with a UTF8 handler to allow any operating system
 * specific changes to be carried out.
 */
void
tty_utf8graphics_fixup(void)
{
    CONSOLE_FONT_INFOEX console_font_info;

    if (console.WindowsTerminal) {
        if ((tty_procs.wincap2 & WC2_U_UTF8STR) && SYMHANDLING(H_UTF8)) {
            char *localestr = 0;

            if (!console.hConOut) {
                console.hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
                if (console.orig.localestr) {
                    free(console.orig.localestr);
                    console.orig.localestr = NULL;
                }
                console.orig.localestr = setlocale(LC_ALL, ".UTF8");
            }
            /* the locale */
            if (console.active.localestr) {
                free(console.active.localestr);
                console.active.localestr = NULL;
            }
            localestr = setlocale(LC_ALL, ".UTF8");
            if (localestr)
                console.active.localestr = dupstr(localestr);
            /* the code page */
            SetConsoleOutputCP(65001);
            console.active.code_page = GetConsoleOutputCP();
            /* the font */
            console_font_info.cbSize = sizeof(console_font_info);
            BOOL success = GetCurrentConsoleFontEx(console.hConOut, FALSE,
                                                   &console_font_info);
            /* Try DejaVu Sans Mono for Powerline */
            wcscpy_s(console_font_info.FaceName,
                     sizeof(console_font_info.FaceName)
                         / sizeof(console_font_info.FaceName[0]),
                     L"DejaVu Sans Mono for Powerline");
            console_font_info.cbSize = sizeof(console_font_info);
            success = SetCurrentConsoleFontEx(console.hConOut, FALSE,
                                              &console_font_info);
            if (!success) {
                /* Next, try Lucida Console */
                wcscpy_s(console_font_info.FaceName,
                         sizeof(console_font_info.FaceName)
                             / sizeof(console_font_info.FaceName[0]),
                         L"Lucida Console");
                console_font_info.cbSize = sizeof(console_font_info);
                success = SetCurrentConsoleFontEx(console.hConOut, FALSE,
                                                  &console_font_info);
            }
            nhassert(success);
            if (success) {
                console.active.font_info = console_font_info;
                console.font_changed = TRUE;
            }
        }
        /* the console mode */
        GetConsoleMode(console.hConOut, &console.active.out_cmode);
        if ((console.active.out_cmode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)
            == 0) {
            /* recognize escape sequences */
            console.active.out_cmode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(console.hConOut, console.active.out_cmode);
        }
    } else {
        /* ConsoleAPI */
        console.active.out_cmode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(console.hConOut, console.active.out_cmode);
    }
    GetConsoleMode(console.hConOut, &console.active.out_cmode);
}
#endif /* UTF8_FROM_CORE */

/*
 * This is called when making the switch to a symset
 * with an IBM handler to allow any operating system
 * specific changes to be carried out.
 */
void
tty_ibmgraphics_fixup(void)
{
    if (console.WindowsTerminal) {
        char buf[BUFSZ], *bp, *localestr;

        if (!console.hConOut)
            console.hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
        /* the locale */
        Snprintf(buf, sizeof buf, "%s", console.orig.localestr);
        if ((bp = strstri(buf, ".utf8")) != 0)
            *bp = '\0';
        localestr = setlocale(LC_ALL, buf);
        if (localestr) {
            if (console.active.localestr)
                free(console.active.localestr);
            console.active.localestr = dupstr(localestr);
        }
        set_known_good_console_font();
    }
    /* the console mode */
    GetConsoleMode(console.hConOut, &console.active.out_cmode);
    if (console.WindowsTerminal) {
        if ((console.active.out_cmode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)
            == 0) {
            console.active.out_cmode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(console.hConOut, console.active.out_cmode);
        }
    } else {
        /* ConsoleAPI */
        if ((console.active.out_cmode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            console.active.out_cmode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(console.hConOut, console.active.out_cmode);
        }
    }
    GetConsoleMode(console.hConOut, &console.active.out_cmode);
}

#ifdef PORT_DEBUG
void
win32con_debug_keystrokes(void)
{
    DWORD count;
    boolean valid = 0;
    int ch = 0;
    xputs("\n");
    while (!valid || ch != 27) {
        nocmov(ttyDisplay->curx, ttyDisplay->cury);
        ReadConsoleInput(console.hConIn, &gbl_ir, 1, &count);
        if ((gbl_ir.EventType == KEY_EVENT) && gbl_ir.Event.KeyEvent.bKeyDown)
            ch = process_keystroke(&gbl_ir, &valid, (uchar) iflags.num_pad, 1);
    }
    (void) doredraw();
}
void
win32con_toggle_cursor_info(void)
{
    display_cursor_info = !display_cursor_info;
}
#endif

void
map_subkeyvalue(char *op)
{
    char digits[] = "0123456789";
    int length, i, idx, val;
    char *kp;

    idx = -1;
    val = -1;
    kp = strchr(op, '/');
    if (kp) {
        *kp = '\0';
        kp++;
        length = strlen(kp);
        if (length < 1 || length > 3)
            return;
        for (i = 0; i < length; i++)
            if (!strchr(digits, kp[i]))
                return;
        val = atoi(kp);
        length = strlen(op);
        if (length < 1 || length > 3)
            return;
        for (i = 0; i < length; i++)
            if (!strchr(digits, op[i]))
                return;
        idx = atoi(op);
    }
    if (idx >= MAX_OVERRIDES || idx < 0 || val >= MAX_OVERRIDES || val < 1)
        return;
    key_overrides[idx] = val;
}

void
synch_cursor(void)
{
    really_move_cursor();
}

static int CALLBACK EnumFontCallback(
    const LOGFONTW * lf, const TEXTMETRICW * tm UNUSED,
    DWORD fontType UNUSED, LPARAM lParam)
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
check_and_set_font(void)
{
    if (console.ConsoleAPI) {
        if (!check_font_widths()) {
            if (wizard) {
                const char *msg =
                    "WARNING: glyphs too wide in console font."
                    " Changing code page to 437 and font to Consolas";

                if (iflags.window_inited)
                    pline("%s", msg);
                else
                    raw_printf("%s\n", msg);
            }
            set_known_good_console_font();
        }
    }
}

/* check_font_widths returns TRUE if all glyphs in current console font
 * fit within the width of a single console cell.
 */
boolean
check_font_widths(void)
{
    CONSOLE_FONT_INFOEX console_font_info;
    console_font_info.cbSize = sizeof(console_font_info);
    BOOL success = GetCurrentConsoleFontEx(console.hConOut, FALSE,
                                            &console_font_info);

    /* get console window and DC
     * NOTE: the DC from the console window does not have the correct
     *       font selected at this point.
     */
    console.hWnd = GetConsoleWindow();
    HDC hDC = GetDC(console.hWnd);

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

    /* measure the set of used glyphs to ensure they fit */
    boolean all_glyphs_fit = FALSE;

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
        used[gp.primary_syms[i]] = TRUE;
        used[gr.rogue_syms[i]] = TRUE;
    }

    int wcUsedCount = 0;
    wchar_t wcUsed[256];
    for (int i = 0; i < (int) sizeof(used); i++)
        if (used[i])
            wcUsed[wcUsedCount++] = cp437[i];

    all_glyphs_fit = TRUE;

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
set_known_good_console_font(void)
{
    CONSOLE_FONT_INFOEX console_font_info;
    console_font_info.cbSize = sizeof(console_font_info);
    BOOL success = GetCurrentConsoleFontEx(console.hConOut, FALSE,
                                            &console_font_info);

    success = SetConsoleOutputCP(437);
    nhassert(success);
    console.font_changed = TRUE;
    if (console.ConsoleAPI) {
        console.orig.font_info = console_font_info;
        console.orig.code_page = GetConsoleOutputCP();
    } else {
        /* console.WindowsTerminal */
        console.active.code_page = GetConsoleOutputCP();
    }
    wcscpy_s(console_font_info.FaceName,
        sizeof(console_font_info.FaceName)
            / sizeof(console_font_info.FaceName[0]),
        L"Consolas");
    if (console.ConsoleAPI) {
        success = SetConsoleOutputCP(437);
        nhassert(success);
    }
    success = SetCurrentConsoleFontEx(console.hConOut, FALSE, &console_font_info);
    nhassert(success);
    if (console.WindowsTerminal) {
        if (success)
            console.active.font_info = console_font_info;
    }
}

/* restore_original_console_font will restore the console font and code page
 * settings to what they were when NetHack was launched.
 */
void
restore_original_console_font(void)
{
    char *tmplocalestr;

    if (console.font_changed) {
        BOOL success;
        if (console.WindowsTerminal) {
            if (wizard)
                raw_print("Restoring original font, code page and locale\n");

            tmplocalestr = setlocale(LC_ALL, console.orig.localestr);
            if (tmplocalestr) {
                if (console.active.localestr)
                    free(console.active.localestr);
                console.active.localestr = dupstr(tmplocalestr);
            }
            success = SetConsoleOutputCP(console.orig.code_page);
        } else {
            /* console.ConsoleAPI */
            raw_print("Restoring original font and code page\n");
            success = SetConsoleOutputCP(console.orig.code_page);
        }
        if (!success)
            raw_print("Unable to restore original code page\n");

        success = SetCurrentConsoleFontEx(console.hConOut, FALSE,
                                            &console.orig.font_info);
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

void
set_cp_map(void)
{
    if (console.has_unicode) {
        if (console.WindowsTerminal) {
            console.active.code_page = GetConsoleOutputCP();

            if (console.active.code_page == 437) {
                memcpy(console.cpMap, cp437, sizeof(console.cpMap));
            } else {
                for (int i = 0; i < 256; i++) {
                    char c = (char) i;
                    int count =
                        MultiByteToWideChar(console.active.code_page, 0, &c,
                                            1, &console.cpMap[i], 1);
                    nhassert(count == 1);

                    // If a character was mapped to unicode control codes,
                    // remap to the appropriate unicode character per our
                    // code page 437 mappings.
                    if (console.cpMap[i] < 32)
                        console.cpMap[i] = cp437[console.cpMap[i]];
                }
            }
        } else { 
            /* console.ConsoleAPI */
            UINT codePage = GetConsoleOutputCP();

            if (codePage == 437) {
                memcpy(console.cpMap, cp437, sizeof(console.cpMap));
            } else {
                for (int i = 0; i < 256; i++) {
                    char c = (char) i;
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
}

int
get_approx_display_cols(void)
{
    return console.width;
}

int
get_approx_display_rows(void)
{
    return console.height;
}

RESTORE_WARNING_CONDEXPR_IS_CONSTANT

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
    if (redirect_stdout || program_state.early_options)
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
   (signaled by setting 0x10 on uchar numberpad argument)
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
            switch(iflags.key_handling) {
            case ray_keyhandling:
                keyboard_handling.khid = ray_keyhandling;
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
                keyboard_handling.khid = nh340_keyhandling;
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
                keyboard_handling.khid = default_keyhandling;
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
    int i, clr = 0;
    menu_item *console_key_handling_pick = (menu_item *) 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = default_keyhandling; i < SIZE(legal_key_handling); i++) {
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, 'a' + i,
                 0, ATR_NONE, clr,
                 legal_key_handling[i], MENU_ITEMFLAGS_NONE);
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
    HANDLE hConIn UNUSED,
    INPUT_RECORD *ir,
    boolean *valid,
    uchar numberpad,
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
    DWORD *count,
    uchar numberpad,
    int mode,
    int *mod,
    coord *cc)
{
#if defined(SAFERHANGUP)
    DWORD dwWait;
#endif
    int ch = 0;
    boolean valid = 0, done = 0, done_a_checkpoint = FALSE;
    DWORD how_many_milliseconds = 0;

#ifdef QWERTZ_SUPPORT
    if (numberpad & 0x10) {
        numberpad &= ~0x10;
        qwertz = TRUE;
    } else {
        qwertz = FALSE;
    }
#endif
    done_a_checkpoint = FALSE;
    how_many_milliseconds = (iflags.idlecheckpoint)
        ? (IDLECHECKPOINT_WAIT_TIME * 1000)
        : INFINITE;
    while (!done) {
        dwWait = WaitForSingleObjectEx(hConIn,   // event object to wait for
                                       how_many_milliseconds,
                                       TRUE);    // alertable wait enabled
#if defined(SAFERHANGUP)
        if (dwWait == WAIT_FAILED)
            return '\033';
#endif
        if (iflags.idlecheckpoint
            && dwWait == WAIT_TIMEOUT && !done_a_checkpoint) {
            /* no input for 30 seconds, so let's take
             * advantage and do a game checkpoint,
             * then resume the wait.
             */
            do_idlecheckpoint();
            done_a_checkpoint = TRUE;
        } else
        {
            ReadConsoleInput(hConIn, ir, 1, count);
            if (mode == 0) {
                if ((ir->EventType == KEY_EVENT)
                    && ir->Event.KeyEvent.bKeyDown) {
                    ch = default_processkeystroke(hConIn, ir, &valid,
                                                  numberpad, 0);
                    done = valid;
                }
            } else {
                if (*count > 0) {
                    if (ir->EventType == KEY_EVENT
                        && ir->Event.KeyEvent.bKeyDown) {
#ifdef QWERTZ_SUPPORT
                        if (qwertz)
                            numberpad |= 0x10;
#endif
                        ch = default_processkeystroke(hConIn, ir, &valid,
                                                      numberpad, 0);
#ifdef QWERTZ_SUPPORT
                        numberpad &= ~0x10;
#endif
                        if (valid)
                            return ch;
                    } else if (ir->EventType == MOUSE_EVENT) {
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
                } else
                    done = 1;
            }
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
        return (IsWindows8OrGreater());

    default:
        return 0;
    }
}

int ray_processkeystroke(
    HANDLE hConIn,
    INPUT_RECORD *ir,
    boolean *valid,
    uchar numberpad,
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
    uchar numberpad,
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
    if (numberpad & 0x10) {
        numberpad &= ~0x10;
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
            if (*count > 0) {
                if (ir->EventType == KEY_EVENT
                    && ir->Event.KeyEvent.bKeyDown) {
#ifdef QWERTZ_SUPPORT
                    if (qwertz)
                        numberpad |= 0x10;
#endif
                    ch = ray_processkeystroke(hConIn, ir, &valid, numberpad,
#ifdef PORTDEBUG
                                          1);
#else
                                          0);
#endif
#ifdef QWERTZ_SUPPORT
                    numberpad &= ~0x10;
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
    HANDLE hConIn UNUSED,
    INPUT_RECORD *ir,
    boolean *valid,
    uchar numberpad,
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
    uchar numberpad,
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
    if (numberpad & 0x10) {
        numberpad &= ~0x10;
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
                    numberpad |= 0x10;
#endif
                ch = nh340_processkeystroke(hConIn, ir, &valid, numberpad, 0);
#ifdef QWERTZ_SUPPORT
                numberpad &= ~0x10;
#endif
                done = valid;
            }
        } else {
            if (*count > 0) {
                if (ir->EventType == KEY_EVENT
                    && ir->Event.KeyEvent.bKeyDown) {
                    ch = nh340_processkeystroke(hConIn, ir, &valid, numberpad, 0);
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
#undef kbhit
#include <conio.h>

int
windows_console_custom_nhgetch(void)
{
//    set_console_cursor(console.cursor.X, console.cursor.Y);
    really_move_cursor();
    return _getch();
}

boolean
wincon_WindowsTerminalActive(void)
{
    return console.WindowsTerminal;
}
#endif /* WIN32 */

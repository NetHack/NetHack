/* NetHack 3.7	consoletty.c	$NHDT-Date: 1596498316 2020/08/03 23:45:16 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.117 $ */
/* Copyright (c) NetHack PC Development Team 1993    */
/* NetHack may be freely redistributed.  See license for details. */

/* tty.c - (Windows console) version */

/*
 * Initial Creation 				M. Allison	1993/01/31
 * Switch to low level console output routines	M. Allison	2003/10/01
 * Restrict cursor movement until input pending	M. Lehotay	2003/10/02
 * Call Unicode version of output API on NT     R. Chason	2005/10/28
 * Use of back buffer to improve performance    B. House	2018/05/06
 *
 */

#ifndef NO_VT
#define VIRTUAL_TERMINAL_SEQUENCES
#define UTF8_FROM_CORE
#endif

#ifdef WIN32
#define NEED_VARARGS /* Uses ... */
#include "win32api.h"
#include "winos.h"
#include "hack.h"
#include "wintty.h"
#include <sys\types.h>
#include <sys\stat.h>
#ifdef VIRTUAL_TERMINAL_SEQUENCES
#include <locale.h>
#ifndef INTEGER_H
#include "integer.h"
#endif
#endif /* VIRTUAL_TERMINAL_SEQUENCES */

#ifdef __MINGW32__
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
#endif
#endif

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
#ifndef VIRTUAL_TERMINAL_SEQUENCES
#define CONSOLE_CLEAR_ATTRIBUTE \
    (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define CONSOLE_UNDEFINED_ATTRIBUTE (0)
#else /* VIRTUAL_TERMINAL_SEQUENCES */
#define CONSOLE_CLEAR_ATTRIBUTE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#endif /* VIRTUAL_TERMINAL_SEQUENCES */

#define CONSOLE_CLEAR_CHARACTER (' ')
#define CONSOLE_UNDEFINED_CHARACTER ('\0')

#ifdef VIRTUAL_TERMINAL_SEQUENCES
enum console_attributes {
    atr_bold = 1,
    atr_dim = 2,
    atr_uline = 4,
    atr_blink = 8,
    atr_inverse = 16
};
#define MAX_UTF8_SEQUENCE 7
#endif /* VIRTUAL_TERMINAL_SEQUENCES */

typedef struct {
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    WCHAR   character;
    WORD    attribute;
#else /* VIRTUAL_TERMINAL_SEQUENCES */
    uint8 utf8str[MAX_UTF8_SEQUENCE];
    WCHAR wcharacter;
    WORD attr;
    long color24;
    int color256idx;
    const char *colorseq;
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
} cell_t;

#ifndef VIRTUAL_TERMINAL_SEQUENCES
cell_t clear_cell = { CONSOLE_CLEAR_CHARACTER, CONSOLE_CLEAR_ATTRIBUTE };
cell_t undefined_cell = { CONSOLE_UNDEFINED_CHARACTER,
                          CONSOLE_UNDEFINED_ATTRIBUTE };
#else /* VIRTUAL_TERMINAL_SEQUENCES */
cell_t clear_cell = { { CONSOLE_CLEAR_CHARACTER, 0, 0, 0, 0, 0, 0 },
                        CONSOLE_CLEAR_CHARACTER, 0, 0L, 0, "\x1b[0m" };
cell_t undefined_cell = { { CONSOLE_UNDEFINED_CHARACTER, 0, 0, 0, 0, 0, 0 },
                            CONSOLE_UNDEFINED_CHARACTER, 0, 0L, 0, (const char *) 0 };
static const uint8 empty_utf8str[MAX_UTF8_SEQUENCE] = { 0 };
#endif /* VIRTUAL_TERMINAL_SEQUENCES */

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
 * GetConsoleOutputCP
#ifndef VIRTUAL_TERMINAL_SEQUENCES
 * WriteConsoleOutputCharacter
 * FillConsoleOutputAttribute
#endif
 */

static BOOL CtrlHandler(DWORD);
#ifndef VIRTUAL_TERMINAL_SEQUENCES
static void xputc_core(char);
#else /* VIRTUAL_TERMINAL_SEQUENCES */
static void xputc_core(int);
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
void cmov(int, int);
void nocmov(int, int);
int process_keystroke(INPUT_RECORD *, boolean *, boolean numberpad,
                      int portdebug);
static void init_ttycolor(void);
static void really_move_cursor(void);
static void check_and_set_font(void);
static boolean check_font_widths(void);
static void set_known_good_console_font(void);
static void restore_original_console_font(void);
extern void safe_routines(void);
void tty_ibmgraphics_fixup(void);
#ifdef VIRTUAL_TERMINAL_SEQUENCES
extern void (*ibmgraphics_mode_callback)(void);  /* symbols.c */
extern void (*utf8graphics_mode_callback)(void); /* symbols.c */
#endif /* VIRTUAL_TERMINAL_SEQUENCES */

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
struct console_t {
    boolean is_ready;
    HWND hWnd;
    WORD background;
    WORD foreground;
    WORD attr;
    int current_nhcolor;
    int current_nhattr[ATR_INVERSE+1];
    COORD cursor;
    HANDLE hConOut;
    HANDLE hConIn;
    CONSOLE_SCREEN_BUFFER_INFO orig_csbi;
    int width;
    int height;
    boolean has_unicode;
    int buffer_size;
    cell_t * front_buffer;
    cell_t * back_buffer;
    WCHAR cpMap[256];
    boolean font_changed;
    CONSOLE_FONT_INFOEX orig_font_info;
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    UINT original_code_page;
} console = {
    0,
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED),
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED),
    NO_COLOR,
#else /* VIRTUAL_TERMINAL_SEQUENCES */
    UINT orig_code_page;
    char *orig_localestr;
    DWORD orig_in_cmode;
    DWORD orig_out_cmode;
    CONSOLE_FONT_INFOEX font_info;
    UINT code_page;
    char *localestr;
    DWORD in_cmode;
    DWORD out_cmode;
    long color24;
    int color256idx;
} console = {
    FALSE,
    0,
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED), /* background */
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED), /* foreground */
    0,                                                     /* attr */
    0,                                                     /* current_nhcolor */
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
#ifndef VIRTUAL_TERMINAL_SEQUENCES
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
#else /* VIRTUAL_TERMINAL_SEQUENCES */
    { 0, 0 },                                              /* cursor */
    NULL,     /* hConOut*/
    NULL,     /* hConIn */
    { 0 },    /* cbsi */
    0,        /* width */
    0,        /* height */
    FALSE,    /* has_unicode */
    0,        /* buffer_size */
    NULL,     /* front_buffer */
    NULL,     /* back_buffer */
    { 0 },    /* cpMap */
    FALSE,    /* font_changed */
    { 0 },    /* orig_font_info */
    0U,       /* orig_code_page */
    NULL,     /* orig_localestr */
    0,        /* orig_in_cmode */
    0,        /* orig_out_cmode */
    { 0 },    /* font_info */
    0U,       /* code_page */
    NULL,     /* localestr */
    0,        /* in_cmode */
    0,        /* out_cmode */
    0L,       /* color24 */
    0         /* color256idx */
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
};

static const char default_name[] = "default";
const char *const legal_key_handling[] = {
    "none",
    "default",
    "ray",
    "340",
};

enum windows_key_handling keyh[] = { no_keyhandling, default_keyhandling, ray_keyhandling,
                                     nh340_keyhandling };

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
    enum windows_key_handling khid;
    int (*pProcessKeystroke)(HANDLE, INPUT_RECORD *, boolean *,
                                          boolean, int);
    int (*pNHkbhit)(HANDLE, INPUT_RECORD *);
    int (*pCheckInput)(HANDLE, INPUT_RECORD *, DWORD *, boolean,
                                   int, int *, coord *);
} keyboard_handling = {
    no_keyhandling,
    default_processkeystroke,
    default_kbhit,
    default_checkinput
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
static INPUT_RECORD bogus_key;

#ifdef VIRTUAL_TERMINAL_SEQUENCES
long customcolors[CLR_MAX];
const char *esc_seq_colors[CLR_MAX];

struct rgbvalues {
    int idx;
    const char *name;
    const char *hexval;
    long r, gn, b;
} rgbtable[] = {
    { 0, "maroon", "#800000", 128, 0, 0 },
    { 1, "dark red", "#8B0000", 139, 0, 0 },
    { 2, "brown", "#A52A2A", 165, 42, 42 },
    { 3, "firebrick", "#B22222", 178, 34, 34 },
    { 4, "crimson", "#DC143C", 220, 20, 60 },
    { 5, "red", "#FF0000", 255, 0, 0 },
    { 6, "tomato", "#FF6347", 255, 99, 71 },
    { 7, "coral", "#FF7F50", 255, 127, 80 },
    { 8, "indian red", "#CD5C5C", 205, 92, 92 },
    { 9, "light coral", "#F08080", 240, 128, 128 },
    { 10, "dark salmon", "#E9967A", 233, 150, 122 },
    { 11, "salmon", "#FA8072", 250, 128, 114 },
    { 12, "light salmon", "#FFA07A", 255, 160, 122 },
    { 13, "orange red", "#FF4500", 255, 69, 0 },
    { 14, "dark orange", "#FF8C00", 255, 140, 0 },
    { 15, "orange", "#FFA500", 255, 165, 0 },
    { 16, "gold", "#FFD700", 255, 215, 0 },
    { 17, "dark golden rod", "#B8860B", 184, 134, 11 },
    { 18, "golden rod", "#DAA520", 218, 165, 32 },
    { 19, "pale golden rod", "#EEE8AA", 238, 232, 170 },
    { 20, "dark khaki", "#BDB76B", 189, 183, 107 },
    { 21, "khaki", "#F0E68C", 240, 230, 140 },
    { 22, "olive", "#808000", 128, 128, 0 },
    { 23, "yellow", "#FFFF00", 255, 255, 0 },
    { 24, "yellow green", "#9ACD32", 154, 205, 50 },
    { 25, "dark olive green", "#556B2F", 85, 107, 47 },
    { 26, "olive drab", "#6B8E23", 107, 142, 35 },
    { 27, "lawn green", "#7CFC00", 124, 252, 0 },
    { 28, "chart reuse", "#7FFF00", 127, 255, 0 },
    { 29, "green yellow", "#ADFF2F", 173, 255, 47 },
    { 30, "dark green", "#006400", 0, 100, 0 },
    { 31, "green", "#008000", 0, 128, 0 },
    { 32, "forest green", "#228B22", 34, 139, 34 },
    { 33, "lime", "#00FF00", 0, 255, 0 },
    { 34, "lime green", "#32CD32", 50, 205, 50 },
    { 35, "light green", "#90EE90", 144, 238, 144 },
    { 36, "pale green", "#98FB98", 152, 251, 152 },
    { 37, "dark sea green", "#8FBC8F", 143, 188, 143 },
    { 38, "medium spring green", "#00FA9A", 0, 250, 154 },
    { 39, "spring green", "#00FF7F", 0, 255, 127 },
    { 40, "sea green", "#2E8B57", 46, 139, 87 },
    { 41, "medium aqua marine", "#66CDAA", 102, 205, 170 },
    { 42, "medium sea green", "#3CB371", 60, 179, 113 },
    { 43, "light sea green", "#20B2AA", 32, 178, 170 },
    { 44, "dark slate gray", "#2F4F4F", 47, 79, 79 },
    { 45, "teal", "#008080", 0, 128, 128 },
    { 46, "dark cyan", "#008B8B", 0, 139, 139 },
    { 47, "aqua", "#00FFFF", 0, 255, 255 },
    { 48, "cyan", "#00FFFF", 0, 255, 255 },
    { 49, "light cyan", "#E0FFFF", 224, 255, 255 },
    { 50, "dark turquoise", "#00CED1", 0, 206, 209 },
    { 51, "turquoise", "#40E0D0", 64, 224, 208 },
    { 52, "medium turquoise", "#48D1CC", 72, 209, 204 },
    { 53, "pale turquoise", "#AFEEEE", 175, 238, 238 },
    { 54, "aqua marine", "#7FFFD4", 127, 255, 212 },
    { 55, "powder blue", "#B0E0E6", 176, 224, 230 },
    { 56, "cadet blue", "#5F9EA0", 95, 158, 160 },
    { 57, "steel blue", "#4682B4", 70, 130, 180 },
    { 58, "corn flower blue", "#6495ED", 100, 149, 237 },
    { 59, "deep sky blue", "#00BFFF", 0, 191, 255 },
    { 60, "dodger blue", "#1E90FF", 30, 144, 255 },
    { 61, "light blue", "#ADD8E6", 173, 216, 230 },
    { 62, "sky blue", "#87CEEB", 135, 206, 235 },
    { 63, "light sky blue", "#87CEFA", 135, 206, 250 },
    { 64, "midnight blue", "#191970", 25, 25, 112 },
    { 65, "navy", "#000080", 0, 0, 128 },
    { 66, "dark blue", "#00008B", 0, 0, 139 },
    { 67, "medium blue", "#0000CD", 0, 0, 205 },
    { 68, "blue", "#0000FF", 0, 0, 255 },
    { 69, "royal blue", "#4169E1", 65, 105, 225 },
    { 70, "blue violet", "#8A2BE2", 138, 43, 226 },
    { 71, "indigo", "#4B0082", 75, 0, 130 },
    { 72, "dark slate blue", "#483D8B", 72, 61, 139 },
    { 73, "slate blue", "#6A5ACD", 106, 90, 205 },
    { 74, "medium slate blue", "#7B68EE", 123, 104, 238 },
    { 75, "medium purple", "#9370DB", 147, 112, 219 },
    { 76, "dark magenta", "#8B008B", 139, 0, 139 },
    { 77, "dark violet", "#9400D3", 148, 0, 211 },
    { 78, "dark orchid", "#9932CC", 153, 50, 204 },
    { 79, "medium orchid", "#BA55D3", 186, 85, 211 },
    { 80, "purple", "#800080", 128, 0, 128 },
    { 81, "thistle", "#D8BFD8", 216, 191, 216 },
    { 82, "plum", "#DDA0DD", 221, 160, 221 },
    { 83, "violet", "#EE82EE", 238, 130, 238 },
    { 84, "magenta / fuchsia", "#FF00FF", 255, 0, 255 },
    { 85, "orchid", "#DA70D6", 218, 112, 214 },
    { 86, "medium violet red", "#C71585", 199, 21, 133 },
    { 87, "pale violet red", "#DB7093", 219, 112, 147 },
    { 88, "deep pink", "#FF1493", 255, 20, 147 },
    { 89, "hot pink", "#FF69B4", 255, 105, 180 },
    { 90, "light pink", "#FFB6C1", 255, 182, 193 },
    { 91, "pink", "#FFC0CB", 255, 192, 203 },
    { 92, "antique white", "#FAEBD7", 250, 235, 215 },
    { 93, "beige", "#F5F5DC", 245, 245, 220 },
    { 94, "bisque", "#FFE4C4", 255, 228, 196 },
    { 95, "blanched almond", "#FFEBCD", 255, 235, 205 },
    { 96, "wheat", "#F5DEB3", 245, 222, 179 },
    { 97, "corn silk", "#FFF8DC", 255, 248, 220 },
    { 98, "lemon chiffon", "#FFFACD", 255, 250, 205 },
    { 99, "light golden rod yellow", "#FAFAD2", 250, 250, 210 },
    { 100, "light yellow", "#FFFFE0", 255, 255, 224 },
    { 101, "saddle brown", "#8B4513", 139, 69, 19 },
    { 102, "sienna", "#A0522D", 160, 82, 45 },
    { 103, "chocolate", "#D2691E", 210, 105, 30 },
    { 104, "peru", "#CD853F", 205, 133, 63 },
    { 105, "sandy brown", "#F4A460", 244, 164, 96 },
    { 106, "burly wood", "#DEB887", 222, 184, 135 },
    { 107, "tan", "#D2B48C", 210, 180, 140 },
    { 108, "rosy brown", "#BC8F8F", 188, 143, 143 },
    { 109, "moccasin", "#FFE4B5", 255, 228, 181 },
    { 110, "navajo white", "#FFDEAD", 255, 222, 173 },
    { 111, "peach puff", "#FFDAB9", 255, 218, 185 },
    { 112, "misty rose", "#FFE4E1", 255, 228, 225 },
    { 113, "lavender blush", "#FFF0F5", 255, 240, 245 },
    { 114, "linen", "#FAF0E6", 250, 240, 230 },
    { 115, "old lace", "#FDF5E6", 253, 245, 230 },
    { 116, "papaya whip", "#FFEFD5", 255, 239, 213 },
    { 117, "sea shell", "#FFF5EE", 255, 245, 238 },
    { 118, "mint cream", "#F5FFFA", 245, 255, 250 },
    { 119, "slate gray", "#708090", 112, 128, 144 },
    { 120, "light slate gray", "#778899", 119, 136, 153 },
    { 121, "light steel blue", "#B0C4DE", 176, 196, 222 },
    { 122, "lavender", "#E6E6FA", 230, 230, 250 },
    { 123, "floral white", "#FFFAF0", 255, 250, 240 },
    { 124, "alice blue", "#F0F8FF", 240, 248, 255 },
    { 125, "ghost white", "#F8F8FF", 248, 248, 255 },
    { 126, "honeydew", "#F0FFF0", 240, 255, 240 },
    { 127, "ivory", "#FFFFF0", 255, 255, 240 },
    { 128, "azure", "#F0FFFF", 240, 255, 255 },
    { 129, "snow", "#FFFAFA", 255, 250, 250 },
    { 130, "black", "#000000", 0, 0, 0 },
    { 131, "dim gray / dim grey", "#696969", 105, 105, 105 },
    { 132, "gray / grey", "#808080", 128, 128, 128 },
    { 133, "dark gray / dark grey", "#A9A9A9", 169, 169, 169 },
    { 134, "silver", "#C0C0C0", 192, 192, 192 },
    { 135, "light gray / light grey", "#D3D3D3", 211, 211, 211 },
    { 136, "gainsboro", "#DCDCDC", 220, 220, 220 },
    { 137, "white smoke", "#F5F5F5", 245, 245, 245 },
    { 138, "white", "#FFFFFF", 255, 255, 255 },
};

long
rgbtable_to_long(struct rgbvalues *tbl)
{
    long rgblong = (tbl->r << 0) | (tbl->gn << 8) | (tbl->b << 16);
    return rgblong;
}

static void
init_custom_colors(void)
{
    customcolors[CLR_BLACK] = rgbtable_to_long(&rgbtable[131]);
    customcolors[CLR_RED] = rgbtable_to_long(&rgbtable[5]);
    customcolors[CLR_GREEN] = rgbtable_to_long(&rgbtable[31]);
    customcolors[CLR_BROWN] = rgbtable_to_long(&rgbtable[104]);
    customcolors[CLR_BLUE] = rgbtable_to_long(&rgbtable[58]);
    customcolors[CLR_MAGENTA] = rgbtable_to_long(&rgbtable[76]);
    customcolors[CLR_CYAN] = rgbtable_to_long(&rgbtable[48]);
    customcolors[CLR_GRAY] = rgbtable_to_long(&rgbtable[73]);
    customcolors[NO_COLOR] = rgbtable_to_long(&rgbtable[137]);
    customcolors[CLR_ORANGE] = rgbtable_to_long(&rgbtable[15]);
    customcolors[CLR_BRIGHT_GREEN] = rgbtable_to_long(&rgbtable[34]);
    customcolors[CLR_YELLOW] = rgbtable_to_long(&rgbtable[18]);
    customcolors[CLR_BRIGHT_BLUE] = rgbtable_to_long(&rgbtable[69]);
    customcolors[CLR_BRIGHT_MAGENTA] = rgbtable_to_long(&rgbtable[84]);
    customcolors[CLR_BRIGHT_CYAN] = rgbtable_to_long(&rgbtable[49]);
    customcolors[CLR_WHITE] = rgbtable_to_long(&rgbtable[138]);

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
}

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
void emit_show_curor(void);

void
emit_hide_cursor(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[?25l";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}

void
emit_show_cursor(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[?25h";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}

void
emit_start_bold(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[4m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}

void
emit_stop_bold(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[24m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}
#if 0
emit_start_dim(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[4m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}

void
emit_stop_dim(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[24m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}
#endif

void
emit_start_blink(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[5m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}

void
emit_stop_blink(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[25m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}

void
emit_start_underline(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[4m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}

void
emit_stop_underline(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[24m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}
void
emit_start_inverse(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[7m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}

void
emit_stop_inverse(void)
{
    DWORD unused, reserved;
    static const char escseq[] = "\x1b[27m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}

#if 0
#define tcfmtstr "\x1b[38;2;%d;%d;%dm"
#if 0
#define tcfmtstr "\x1b[38:2:%d:%d:%dm"
#endif
#endif

#ifndef SEP2
#define tcfmtstr24bit "\x1b[38;2;%ld;%ld;%ldm"
#define tcfmtstr256 "\x1b[38;5;%ldm"
#else
#define tcfmtstr24bit "\x1b[38:2:%ld:%ld:%ldm"
#define tcfmtstr256 "\x1b[38:5:%dm"
#endif
 
void
emit_start_256color(int u256coloridx)
{
    DWORD unused, reserved;
    static char tcolorbuf[QBUFSZ];
    Snprintf(tcolorbuf, sizeof tcolorbuf, tcfmtstr256, u256coloridx);
    WriteConsoleA(console.hConOut, (LPCSTR) tcolorbuf,
                  (int) strlen(tcolorbuf), &unused, &reserved);
}

void
emit_start_24bitcolor(long color24bit)
{
    DWORD unused, reserved;
    static char tcolorbuf[QBUFSZ];
    long mcolor =
        (color24bit & 0xFFFFFF); /* color 0 has bit 0x1000000 set */
    Snprintf(tcolorbuf, sizeof tcolorbuf, tcfmtstr24bit,
             ((mcolor >> 16) & 0xFF),   /* red */
             ((mcolor >>  8) & 0xFF),   /* green */
             ((mcolor >>  0) & 0xFF));  /* blue */
    WriteConsoleA(console.hConOut, (LPCSTR) tcolorbuf,
                  (int) strlen(tcolorbuf), &unused, &reserved);
}

void
emit_default_color(void)
{
    DWORD unused, reserved;
    static char escseq[] = "\x1b[39m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}
void
emit_return_to_default(void)
{
    DWORD unused, reserved;
    static char escseq[] = "\x1b[0m";

    WriteConsoleA(console.hConOut, (LPCSTR) escseq, (int) strlen(escseq),
                  &unused, &reserved);
}
static boolean newattr_on = TRUE;
static boolean color24_on = TRUE;

/* for debugging */
WORD what_is_there_now;
BOOL success;
DWORD error_result;
#endif /* VIRTUAL_TERMINAL_SEQUENCES */

/* Console buffer flipping support */
#ifdef VIRTUAL_TERMINAL_SEQUENCES
enum do_flags {
    do_utf8_content = 1,
    do_wide_content = 2,
    do_colorseq = 4,
    do_color24 = 8,
    do_newattr = 16
};
enum did_flags {
    did_utf8_content = 1,
    did_wide_content = 2,
    did_colorseq = 4,
    did_color24 = 8,
    did_newattr = 16
};

static void
back_buffer_flip(void)
{
    cell_t *back = console.back_buffer;
    cell_t *front = console.front_buffer;
    COORD pos;
    DWORD unused;
    DWORD reserved;
    unsigned do_anything, did_anything;

    if (!console.is_ready)
        return;

    emit_hide_cursor();
    for (pos.Y = 0; pos.Y < console.height; pos.Y++) {
        for (pos.X = 0; pos.X < console.width; pos.X++) {
            boolean pos_set = FALSE;
            do_anything = did_anything = 0U;
            if (back->color24 != front->color24)
                do_anything |= do_color24;
            if (back->colorseq != front->colorseq)
                do_anything |= do_colorseq;
            if (back->attr != front->attr)
                do_anything |= do_newattr;
#ifdef UTF8_FROM_CORE
            if (!SYMHANDLING(H_UTF8)) {
                if (console.has_unicode
                    && (back->wcharacter != front->wcharacter))
                    do_anything |= do_wide_content;
            } else {
#endif
                if (strcmp((const char *) back->utf8str,
                           (const char *) front->utf8str))
                    do_anything |= do_utf8_content;
#ifdef UTF8_FROM_CORE
            }
#endif
            if (do_anything) {
                SetConsoleCursorPosition(console.hConOut, pos);
                pos_set = TRUE;
                did_anything |= did_newattr;
                if (back->attr) {
                    if (back->attr & atr_bold)
                        emit_start_bold();
                    //  if (back->attr & atr_dim)
                    //      emit_start_dim();
                    if (back->attr & atr_uline)
                        emit_start_underline();
                    // if (back->attr & atr_blink)
                    //    emit_start_blink();
                    if (back->attr & atr_inverse)
                        emit_start_inverse();
                    // front->attr = back->attr; /* will happen below due
                    // to did_newattr */
                } else {
                    emit_return_to_default();
                }

                if (color24_on && back->color24) {
                    did_anything |= did_color24;
                    if (back->color24) {
                        if (!iflags.use_truecolor && iflags.colorcount == 256)
                            emit_start_256color(back->color256idx);
                        else
                            emit_start_24bitcolor(back->color24);
                    }
                } else if (back->colorseq) {
                    did_anything |= did_colorseq;
                    WriteConsoleA(console.hConOut, back->colorseq,
                                  (int) strlen(back->colorseq), &unused,
                                  &reserved);
                } else {
                    did_anything |= did_colorseq;
                    emit_default_color();
                }
                if (did_anything
                    || (do_anything & (do_wide_content | do_utf8_content))) {
#ifdef UTF8_FROM_CORE
                    if (SYMHANDLING(H_UTF8) || !console.has_unicode) {
                        WriteConsoleA(console.hConOut, (LPCSTR) back->utf8str,
                                      (int) strlen((char *) back->utf8str),
                                      &unused, &reserved);
                        did_anything |= did_utf8_content;
                    } else {
#endif
                    WriteConsoleW(console.hConOut, &back->wcharacter, 1,
                                  &unused, &reserved);
                    did_anything |= did_wide_content;
#ifdef UTF8_FROM_CORE
                    }
#endif
                }
            }
            if (did_anything) {
                if (!pos_set) {
                    SetConsoleCursorPosition(console.hConOut, pos);
                    pos_set = TRUE;
                }
                emit_return_to_default();
                *front = *back;
            }
            back++;
            front++;
        }
    }
    emit_show_cursor();
}
#else
static void
back_buffer_flip(void)
{
    cell_t *back = console.back_buffer;
    cell_t *front = console.front_buffer;
    COORD pos;
    DWORD unused;

    if (!console.is_ready)
        return;

    for (pos.Y = 0; pos.Y < console.height; pos.Y++) {
        for (pos.X = 0; pos.X < console.width; pos.X++) {
            if (back->attribute != front->attribute) {
                WriteConsoleOutputAttribute(console.hConOut, &back->attribute,
                                            1, pos, &unused);
                front->attribute = back->attribute;
            }
            if (back->character != front->character) {
                if (console.has_unicode) {
                    WriteConsoleOutputCharacterW(
                        console.hConOut, &back->character, 1, pos, &unused);
                } else {
                    char ch = (char) back->character;
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
#endif

void buffer_fill_to_end(cell_t * buffer, cell_t * fill, int x, int y)
{
    nhassert(x >= 0 && x < console.width);
    nhassert(y >= 0 && ((y < console.height) || (y == console.height &&
                                                 x == 0)));

    cell_t * dst = buffer + console.width * y + x;
    cell_t * sentinel = buffer + console.buffer_size;
    while (dst != sentinel)
        *dst++ = *fill;

    if ((iflags.debug.immediateflips || !g.program_state.in_moveloop)
        && buffer == console.back_buffer)
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

    if (iflags.debug.immediateflips || !g.program_state.in_moveloop)
        back_buffer_flip();
}

void buffer_write(cell_t * buffer, cell_t * cell, COORD pos)
{
    nhassert(pos.X >= 0 && pos.X < console.width);
    nhassert(pos.Y >= 0 && pos.Y < console.height);

    cell_t * dst = buffer + (console.width * pos.Y) + pos.X;
    *dst = *cell;

    if ((iflags.debug.immediateflips || !g.program_state.in_moveloop)
        && buffer == console.back_buffer)
        back_buffer_flip();
}

/*
 * Called after returning from ! or ^Z
 */
void
gettty(void)
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
settty(const char* s)
{
    cmov(ttyDisplay->curx, ttyDisplay->cury);
    end_screen();
    if (s)
        raw_print(s);
    restore_original_console_font();
    if (orig_QuickEdit) {
#ifndef VIRTUAL_TERMINAL_SEQUENCES
        DWORD cmode;

        GetConsoleMode(console.hConIn, &cmode);
        cmode |= (ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS);
        SetConsoleMode(console.hConIn, cmode);
#else /* VIRTUAL_TERMINAL_SEQUENCES */
        GetConsoleMode(console.hConIn, &console.in_cmode);
        console.in_cmode |= (ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS);
        SetConsoleMode(console.hConIn, console.in_cmode);
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
    }
}

/* called by init_nhwindows() and resume_nhwindows() */
void
setftty(void)
{
    start_screen();
}

void
tty_startup(int *wid, int *hgt)
{
    *wid = console.width;
    *hgt = console.height;
    set_option_mod_status("mouse_support", set_in_game);
    iflags.colorcount = 16777216;
//    iflags.colorcount = 256;
}

void
tty_number_pad(int state)
{
    // do nothing
}

void
tty_start_screen(void)
{
    if (iflags.num_pad)
        tty_number_pad(1); /* make keypad send digits */
#ifdef VIRTUAL_TERMINAL_SEQUENCES
    ibmgraphics_mode_callback = tty_ibmgraphics_fixup;
#ifdef ENHANCED_SYMBOLS
#ifdef UTF8_FROM_CORE
    utf8graphics_mode_callback = tty_utf8graphics_fixup;
#endif
#endif
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
}

void
tty_end_screen(void)
{
    clear_screen();
    really_move_cursor();
    buffer_fill_to_end(console.back_buffer, &clear_cell, 0, 0);
    back_buffer_flip();
    FlushConsoleInputBuffer(console.hConIn);
}

static BOOL
CtrlHandler(DWORD ctrltype)
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
consoletty_open(int mode)
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
}

void
consoletty_exit(void)
{
    /* go back to using the safe routines */
    safe_routines();
}

int
process_keystroke(
    INPUT_RECORD *ir,
    boolean *valid,
    boolean numberpad,
    int portdebug)
{
    int ch;

#ifdef QWERTZ_SUPPORT
    if (g.Cmd.swap_yz)
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
    return keyboard_handling.pNHkbhit(console.hConIn, &ir);
}

int
tgetch(void)
{
    int mod;
    coord cc;
    DWORD count;
    boolean numpad = iflags.num_pad;

    really_move_cursor();
    if (iflags.debug_fuzzer)
        return randomkey();
#ifdef QWERTZ_SUPPORT
    if (g.Cmd.swap_yz)
        numpad |= 0x10;
#endif

    return (g.program_state.done_hup)
               ? '\033'
               : keyboard_handling.pCheckInput(
                   console.hConIn, &ir, &count, numpad, 0, &mod, &cc);
}

int
console_poskey(coordxy *x, coordxy *y, int *mod)
{
    int ch;
    coord cc = { 0, 0 };
    DWORD count;
    boolean numpad = iflags.num_pad;

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
    if (g.Cmd.swap_yz)
        numpad |= 0x10;
#endif
    ch = (g.program_state.done_hup)
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
really_move_cursor(void)
{
#ifdef PORT_DEBUG
    char oldtitle[BUFSZ], newtitle[BUFSZ];
    if (display_cursor_info && wizard) {
        oldtitle[0] = '\0';
        if (GetConsoleTitle(oldtitle, BUFSZ)) {
            oldtitle[39] = '\0';
        }
        Sprintf(newtitle, "%-55s tty=(%02d,%02d) consoletty=(%02d,%02d)", oldtitle,
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
cmov(int x, int y)
{
    if (x >= console.width)
        x = console.width - 1;
    if (y >= console.height)
        y = console.height - 1;

    ttyDisplay->cury = y;
    ttyDisplay->curx = x;

    set_console_cursor(x, y);
}

void
nocmov(int x, int y)
{
    ttyDisplay->curx = x;
    ttyDisplay->cury = y;

    set_console_cursor(x, y);
}

void
xputs(const char* s)
{
    int k;
    int slen = (int) strlen(s);

    if (ttyDisplay)
        set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);

    if (s) {
        for (k = 0; k < slen && s[k]; ++k)
#ifndef VIRTUAL_TERMINAL_SEQUENCES
            xputc_core(s[k]);
#else
            xputc_core((int) s[k]);
#endif
    }
}

/* xputc_core() and g_putch() are the only routines that actually place output.
   same signature as 'putchar()' with potential failure result ignored */
int
xputc(int ch)
{
    int x = ttyDisplay->curx, y = ttyDisplay->cury;
    if (x < console.width && y < console.height) {
        set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
        xputc_core(ch);
    }
    return 0;
}

#ifndef VIRTUAL_TERMINAL_SEQUENCES
void
xputc_core(char ch) 
#else
void
xputc_core(int ch)
#endif
{
    ccount = 1; /* default to non-zero */
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    boolean inverse = FALSE;
#else
    WCHAR wch[2];
#endif
    nhassert(console.cursor.X >= 0 && console.cursor.X < console.width);
    nhassert(console.cursor.Y >= 0 && console.cursor.Y < console.height);

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
        } else if (console.cursor.Y > 0) {
            console.cursor.X = console.width - 1;
            console.cursor.Y--;
        }
        break;
    default:
#ifdef VIRTUAL_TERMINAL_SEQUENCES
        /* this causes way too much performance degradation */
        /* cell.color24 = customcolors[console.current_nhcolor]; */
        cell.colorseq = esc_seq_colors[console.current_nhcolor];
        cell.attr = console.attr;
        // if (console.color24)
        //    __debugbreak();
        cell.color24 = 0L;
        cell.color256idx = 0;
        wch[1] = 0;
        if (console.has_unicode) {
            wch[0] = (ch >= 0 && ch < SIZE(console.cpMap)) ? console.cpMap[ch]
                                                           : ch;
#ifdef UTF8_FROM_CORE
            if (SYMHANDLING(H_UTF8)) {
                /* we have to convert it to UTF-8 for cell.utf8str */
                ccount = WideCharToMultiByte(
                    CP_UTF8, 0, wch, -1, (char *) cell.utf8str,
                    (int) sizeof cell.utf8str, NULL, NULL);
            } else {
#endif
            /* store the wide version here also, so we don't slow
               down back_buffer_flip() with conversions */
            cell.wcharacter = wch[0];
#ifdef UTF8_FROM_CORE
            }
#endif
        } else {
            /* we can just use the UTF-8 utf8str field, since ascii is a
               single-byte representation of a small subset of unicode */
            cell.utf8str[0] = ch;
            cell.utf8str[1] = 0;
        }
#else /* VIRTUAL_TERMINAL_SEQUENCES */
        inverse = (console.current_nhattr[ATR_INVERSE] && iflags.wc_inverse);
        console.attr = (inverse) ? ttycolors_inv[console.current_nhcolor]
                                 : ttycolors[console.current_nhcolor];
        if (console.current_nhattr[ATR_BOLD])
            console.attr |=
                (inverse) ? BACKGROUND_INTENSITY : FOREGROUND_INTENSITY;

        cell.attribute = console.attr;
        cell.character = (console.has_unicode ? console.cpMap[ch] : ch);
#endif
        if (ccount) {
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
g_putch(int in_ch)
{
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    boolean inverse = FALSE;
#else /* VIRTUAL_TERMINAL_SEQUENCES */
    int ccount = 0;
    WCHAR wch[2];
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
    unsigned char ch = (unsigned char) in_ch;

    set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
#ifndef VIRTUAL_TERMINAL_SEQUENCES

    inverse = (console.current_nhattr[ATR_INVERSE] && iflags.wc_inverse);
    console.attr = (console.current_nhattr[ATR_INVERSE] && iflags.wc_inverse) ?
                    ttycolors_inv[console.current_nhcolor] :
                    ttycolors[console.current_nhcolor];
    if (console.current_nhattr[ATR_BOLD])
        console.attr |= (inverse) ? BACKGROUND_INTENSITY : FOREGROUND_INTENSITY;

#endif /* ! VIRTUAL_TERMINAL_SEQUENCES */
    cell_t cell;
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    cell.attribute = console.attr;
    cell.character = (console.has_unicode ? cp437[ch] : ch);
#else
    cell.attr = console.attr;
    cell.colorseq = esc_seq_colors[console.current_nhcolor];
    cell.color24 = console.color24 ? console.color24 : 0L;
    cell.color256idx = 0;
    wch[1] = 0;
    if (console.has_unicode) {
        wch[0] = (ch >= 0 && ch < SIZE(console.cpMap)) ? console.cpMap[ch] : ch;
#ifdef UTF8_FROM_CORE
        if (SYMHANDLING(H_UTF8)) {
            /* we have to convert it to UTF-8 for cell.utf8str */
            ccount = WideCharToMultiByte(
                CP_UTF8, 0, wch, -1, (char *) cell.utf8str,
                (int) sizeof cell.utf8str, NULL, NULL);
        } else {
#endif
            /* store the wide version here also, so we don't slow
               down back_buffer_flip() with conversions */
            cell.wcharacter = wch[0];
#ifdef UTF8_FROM_CORE
        }
#endif
    } else {
        /* we can just use the UTF-8 utf8str field, since ascii is a
           single-byte representation of a small subset of unicode */
        cell.utf8str[0] = ch;
        cell.utf8str[1] = 0;
        ccount = 2;
    }
#endif
    buffer_write(console.back_buffer, &cell, console.cursor);
}

#ifdef VIRTUAL_TERMINAL_SEQUENCES
#ifdef UTF8_FROM_CORE
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
    set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
    cell_t cell;
    cell.attr = console.attr;
    cell.colorseq = esc_seq_colors[console.current_nhcolor];
    cell.color24 = console.color24 ? console.color24 : 0L;
    cell.color256idx =console.color256idx ? console.color256idx : 0;
    Snprintf((char *) cell.utf8str, sizeof cell.utf8str, "%s",
             (char *) sequence);
    buffer_write(console.back_buffer, &cell, console.cursor);
}
#endif /* UTF8_FROM_CORE */

void
term_start_24bitcolor(struct unicode_representation *uval)
{
    console.color24 = uval->ucolor; /* color 0 has bit 0x1000000 set */
    console.color256idx = uval->u256coloridx;
}

void
term_end_24bitcolor(void)
{
    console.color24 = 0L;
    console.color256idx = 0;
}
#endif /* VIRTUAL_TERMINAL_SEQUENCES */

void
cl_end(void)
{
    if (ttyDisplay->curx < console.width 
            && ttyDisplay->cury < console.height) {
        set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
        buffer_clear_to_end_of_line(console.back_buffer, console.cursor.X,
                                    console.cursor.Y);
        tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1,
                 (int) ttyDisplay->cury);
    }
}

void
raw_clear_screen(void)
{
    if (WINDOWPORT(tty)) {
        cell_t * back = console.back_buffer;
        cell_t * front = console.front_buffer;
        COORD pos;
        DWORD unused;
#ifdef VIRTUAL_TERMINAL_SEQUENCES
        DWORD reserved;
#endif

#ifdef VIRTUAL_TERMINAL_SEQUENCES
        pos.Y = 0;
        pos.X = 0;
        SetConsoleCursorPosition(console.hConOut, pos);
        emit_return_to_default();
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
        for (pos.Y = 0; pos.Y < console.height; pos.Y++) {
            for (pos.X = 0; pos.X < console.width; pos.X++) {
#ifndef VIRTUAL_TERMINAL_SEQUENCES
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
#else /* VIRTUAL_TERMINAL_SEQUENCES */
                *back = clear_cell;
                if (console.has_unicode)
                    WriteConsoleW(console.hConOut, &back->wcharacter, 1,
                                  &unused, &reserved);
                else
                    WriteConsoleA(console.hConOut, (LPCSTR) back->utf8str,
                                    (int) strlen((char *) back->utf8str), &unused, &reserved);
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
                *front = *back;
                back++;
                front++;
            }
        }
    }
}

void
clear_screen(void)
{
    buffer_fill_to_end(console.back_buffer, &clear_cell, 0, 0);
    home();
}

void
home(void)
{
    ttyDisplay->curx = ttyDisplay->cury = 0;
    set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
}

void
backsp(void)
{
    set_console_cursor(ttyDisplay->curx, ttyDisplay->cury);
    xputc_core('\b');
}

void
cl_eos(void)
{
    buffer_fill_to_end(console.back_buffer, &clear_cell, ttyDisplay->curx,
                        ttyDisplay->cury);
    tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1, (int) ttyDisplay->cury);
}

void
tty_nhbell(void)
{
    if (flags.silent || iflags.debug_fuzzer)
        return;
    Beep(8000, 500);
}

volatile int junk; /* prevent optimizer from eliminating loop below */

void
tty_delay_output(void)
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

int
term_attr_fixup(int attrmask)
{
    return attrmask;
}

void
term_start_attr(int attrib)
{
#ifdef VIRTUAL_TERMINAL_SEQUENCES
    switch (attrib) {
    case ATR_INVERSE:
        console.attr |= atr_inverse;
        break;
    case ATR_ULINE:
        console.attr |= atr_uline;
        break;
    case ATR_BLINK:
        console.attr |= atr_blink;
        break;
    case ATR_BOLD:
        console.attr |= atr_bold;
        break;
    }
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
    console.current_nhattr[attrib] = TRUE;
    if (attrib) console.current_nhattr[ATR_NONE] = FALSE;
}

void
term_end_attr(int attrib)
{
    int k;

    switch (attrib) {
    case ATR_INVERSE:
#ifdef VIRTUAL_TERMINAL_SEQUENCES
        console.attr &= ~atr_inverse;
        break;
#endif
    case ATR_ULINE:
#ifdef VIRTUAL_TERMINAL_SEQUENCES
        console.attr &= ~atr_uline;
        break;
#endif
    case ATR_BLINK:
#ifdef VIRTUAL_TERMINAL_SEQUENCES
        console.attr &= ~atr_blink;
        break;
#endif
    case ATR_BOLD:
#ifdef VIRTUAL_TERMINAL_SEQUENCES
        console.attr &= ~atr_bold;
#endif
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
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    console.attr = (console.foreground | console.background);
#endif /* ! VIRTUAL_TERMINAL_SEQUENCES */
    console.current_nhcolor = NO_COLOR;
}

void
standoutbeg(void)
{
    term_start_attr(ATR_BOLD);
}

void
standoutend(void)
{
    term_end_attr(ATR_BOLD);
}

#ifndef NO_MOUSE_ALLOWED
void
toggle_mouse_support(void)
{
    static int qeinit = 0;
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    DWORD cmode;
#endif /* ! VIRTUAL_TERMINAL_SEQUENCES */

#ifndef VIRTUAL_TERMINAL_SEQUENCES
    GetConsoleMode(console.hConIn, &cmode);
#else /* VIRTUAL_TERMINAL_SEQUENCES */
    GetConsoleMode(console.hConIn, &console.in_cmode);
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
    if (!qeinit) {
        qeinit = 1;
#ifndef VIRTUAL_TERMINAL_SEQUENCES
        orig_QuickEdit = ((cmode & ENABLE_QUICK_EDIT_MODE) != 0);
#else /* VIRTUAL_TERMINAL_SEQUENCES */
        orig_QuickEdit = ((console.in_cmode & ENABLE_QUICK_EDIT_MODE) != 0);
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
    }
    switch(iflags.wc_mouse_support) {
        case 2:
#ifndef VIRTUAL_TERMINAL_SEQUENCES
                cmode |= ENABLE_MOUSE_INPUT;
#else /* VIRTUAL_TERMINAL_SEQUENCES */
                console.in_cmode |= ENABLE_MOUSE_INPUT;
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
                break;
        case 1:
#ifndef VIRTUAL_TERMINAL_SEQUENCES
                cmode |= ENABLE_MOUSE_INPUT;
                cmode &= ~ENABLE_QUICK_EDIT_MODE;
                cmode |= ENABLE_EXTENDED_FLAGS;
#else /* VIRTUAL_TERMINAL_SEQUENCES */
                console.in_cmode |= ENABLE_MOUSE_INPUT;
                console.in_cmode &= ~ENABLE_QUICK_EDIT_MODE;
                console.in_cmode |= ENABLE_EXTENDED_FLAGS;
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
                break;
        case 0:
                /*FALLTHRU*/
        default:
#ifndef VIRTUAL_TERMINAL_SEQUENCES
                cmode &= ~ENABLE_MOUSE_INPUT;
#else /* VIRTUAL_TERMINAL_SEQUENCES */
                console.in_cmode &= ~ENABLE_MOUSE_INPUT;
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
                if (orig_QuickEdit)
#ifndef VIRTUAL_TERMINAL_SEQUENCES
                    cmode |= (ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS);
#else /* VIRTUAL_TERMINAL_SEQUENCES */
                    console.in_cmode |= (ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS);
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
    }
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    SetConsoleMode(console.hConIn, cmode);
#else /* VIRTUAL_TERMINAL_SEQUENCES */
    SetConsoleMode(console.hConIn, console.in_cmode);
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
}
#endif

/* handle tty options updates here */
void
consoletty_preference_update(const char* pref)
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
#ifdef VIRTUAL_TERMINAL_SEQUENCES
/*
 * This is called when making the switch to a symset
 * with a UTF8 handler to allow any operating system
 * specific changes to be carried out.
 */
void
tty_utf8graphics_fixup(void)
{
    CONSOLE_FONT_INFOEX console_font_info;
    if ((tty_procs.wincap2 & WC2_U_UTF8STR) && SYMHANDLING(H_UTF8)) {
        if (!console.hConOut)
            console.hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
        /* the locale */
        if (console.localestr)
            free(console.localestr);
        console.localestr = strdup(setlocale(LC_ALL, ".UTF8"));
        /* the code page */
        SetConsoleOutputCP(65001);
        console.code_page = GetConsoleOutputCP();
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
            console.font_info = console_font_info;
            console.font_changed = TRUE;
        }
        /* the console mode */
        GetConsoleMode(console.hConOut, &console.out_cmode);
#if 1
        if ((console.out_cmode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0) {
            /* recognize escape sequences */
            console.out_cmode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(console.hConOut, console.out_cmode);
        }
#else
        console.out_cmode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
#endif
    }
}
#endif
#endif /* UTF8_FROM_CORE */

/*
 * This is called when making the switch to a symset
 * with an IBM handler to allow any operating system
 * specific changes to be carried out.
 */
void
tty_ibmgraphics_fixup(void)
{
#ifdef VIRTUAL_TERMINAL_SEQUENCES
    char buf[BUFSZ], *bp, *localestr;

    if (!console.hConOut)
        console.hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    /* the locale */
    Snprintf(buf, sizeof buf, "%s", console.orig_localestr);
    if ((bp = strstri(buf, ".utf8")) != 0)
        *bp = '\0';
    localestr = setlocale(LC_ALL, buf);
    if (localestr) {
        if (console.localestr)
            free(console.localestr);
        console.localestr = strdup(localestr);
    }
    set_known_good_console_font();
    /* the console mode */
    GetConsoleMode(console.hConOut, &console.out_cmode);
    if ((console.out_cmode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0) {
        console.out_cmode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(console.hConOut, console.out_cmode);
        GetConsoleMode(console.hConOut, &console.out_cmode);
    }
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
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
        ReadConsoleInput(console.hConIn, &ir, 1, &count);
        if ((ir.EventType == KEY_EVENT) && ir.Event.KeyEvent.bKeyDown)
            ch = process_keystroke(&ir, &valid, iflags.num_pad, 1);
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
map_subkeyvalue(char* op)
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
void consoletty_error
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
synch_cursor(void)
{
    really_move_cursor();
}

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
check_and_set_font(void)
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
#ifndef VIRTUAL_TERMINAL_SEQUENCES
check_font_widths(void)
#else /* VIRTUAL_TERMINAL_SEQUENCES */
check_font_widths(void)
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
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
        used[g.primary_syms[i]] = TRUE;
        used[g.rogue_syms[i]] = TRUE;
    }

    int wcUsedCount = 0;
    wchar_t wcUsed[256];
    for (int i = 0; i < sizeof(used); i++)
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
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    console.orig_font_info = console_font_info;
    console.original_code_page = GetConsoleOutputCP();

#else /* VIRTUAL_TERMINAL_SEQUENCES */
    console.code_page = GetConsoleOutputCP();
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
    wcscpy_s(console_font_info.FaceName,
        sizeof(console_font_info.FaceName)
            / sizeof(console_font_info.FaceName[0]),
        L"Consolas");
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    success = SetConsoleOutputCP(437);
    nhassert(success);
#endif /* ! VIRTUAL_TERMINAL_SEQUENCES */
    success = SetCurrentConsoleFontEx(console.hConOut, FALSE, &console_font_info);
    nhassert(success);
#ifdef VIRTUAL_TERMINAL_SEQUENCES
    if (success)
        console.font_info = console_font_info;
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
}

/* restore_original_console_font will restore the console font and code page
 * settings to what they were when NetHack was launched.
 */
void
restore_original_console_font(void)
{
#ifdef VIRTUAL_TERMINAL_SEQUENCES
    char *tmplocalestr;
#endif /* VIRTUAL_TERMINAL_SEQUENCES */

    if (console.font_changed) {
        BOOL success;
#ifndef VIRTUAL_TERMINAL_SEQUENCES
        raw_print("Restoring original font and code page\n");
        success = SetConsoleOutputCP(console.original_code_page);
#else /* VIRTUAL_TERMINAL_SEQUENCES */
        if (wizard)
            raw_print("Restoring original font, code page and locale\n");

        tmplocalestr = setlocale(LC_ALL, console.orig_localestr);
        if (tmplocalestr) {
            if (console.localestr)
                free(console.localestr);
            console.localestr = strdup(tmplocalestr);
        }
        success = SetConsoleOutputCP(console.orig_code_page);
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
        if (!success)
            raw_print("Unable to restore original code page\n");

        success = SetCurrentConsoleFontEx(console.hConOut, FALSE,
                                            &console.orig_font_info);
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

void set_cp_map(void)
{
    if (console.has_unicode) {
#ifndef VIRTUAL_TERMINAL_SEQUENCES
        UINT codePage = GetConsoleOutputCP();
#else
        console.code_page = GetConsoleOutputCP();
#endif

#ifndef VIRTUAL_TERMINAL_SEQUENCES
        if (codePage == 437) {
#else
        if (console.code_page == 437) {
#endif
            memcpy(console.cpMap, cp437, sizeof(console.cpMap));
        } else {
            for (int i = 0; i < 256; i++) {
                char c = (char)i;
                int count = MultiByteToWideChar(
#ifndef VIRTUAL_TERMINAL_SEQUENCES
                                                codePage,
#else
                                                console.code_page,
#endif
                                              0, &c, 1, &console.cpMap[i], 1);
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

void nethack_enter_consoletty(void)
{
#ifdef VIRTUAL_TERMINAL_SEQUENCES
    char buf[BUFSZ], *bp, *localestr;
    BOOL success;
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
#if 0
    /* set up state needed by early_raw_print() */
    windowprocs.win_raw_print = early_raw_print;
#endif
#if 0
    /* prevent re-sizing of the console window */
    if (!console.hWnd)
        console.hWnd = GetConsoleWindow();
    SetWindowLong(console.hWnd, GWL_STYLE,
                  GetWindowLong(console.hWnd, GWL_STYLE)
                     & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
#endif
    console.hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    nhassert(console.hConOut != NULL); // NOTE: this assert will not print
    GetConsoleScreenBufferInfo(console.hConOut, &console.orig_csbi);
    /* Testing of widths != COLNO has not turned up any problems.  Need
     * to do a bit more testing and then we are likely to enable having
     * console width match window width.
     */
#if 0
    console.width = console.orig_csbi.srWindow.Right -
                     console.orig_csbi.srWindow.Left + 1;
    console.Width = max(console.Width, COLNO);
#else
//    console.width = COLNO;
    console.width = console.orig_csbi.srWindow.Right -
                     console.orig_csbi.srWindow.Left + 1;
    if (console.width < COLNO)
        console.width = COLNO;
#endif

    console.height = console.orig_csbi.srWindow.Bottom -
                     console.orig_csbi.srWindow.Top + 1;
//    console.height = max(console.height, ROWNO + 3);
    if (console.height < (ROWNO + 2 + 1))
        console.height = (ROWNO + 2 + 1);
    console.buffer_size = console.width * console.height;


    /* clear the entire console buffer */
    int size = console.orig_csbi.dwSize.X * console.orig_csbi.dwSize.Y;
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
    if (console.orig_csbi.dwSize.X < console.width) {
        COORD screen_size = {0};

        screen_size.Y = console.orig_csbi.dwSize.Y,
        screen_size.X = console.width;
        SetConsoleScreenBufferSize(console.hConOut, screen_size);
    }

    /* setup front and back buffers */
    int buffer_size_bytes = sizeof(cell_t) * console.buffer_size;

    console.front_buffer = (cell_t *)malloc(buffer_size_bytes);
    buffer_fill_to_end(console.front_buffer, &undefined_cell, 0, 0);

    console.back_buffer = (cell_t *)malloc(buffer_size_bytes);
    buffer_fill_to_end(console.back_buffer, &clear_cell, 0, 0);

    /* determine whether OS version has unicode support */
    console.has_unicode = ((GetVersion() & 0x80000000) == 0);

#ifdef VIRTUAL_TERMINAL_SEQUENCES
    /* store the original code page*/
    console.orig_code_page = GetConsoleOutputCP();

    /* store the original locale */
    console.orig_localestr = dupstr(setlocale(LC_ALL, ""));

    /* store the original font */
    console.orig_font_info.cbSize = sizeof(console.orig_font_info);
    success = GetCurrentConsoleFontEx(console.hConOut,
                                      FALSE, &console.orig_font_info);
    console.font_info = console.orig_font_info;

    /* adjust the locale */
    Snprintf(buf, sizeof buf, "%s", console.orig_localestr);
    if ((bp = strstri(buf, ".utf8")) != 0)
        *bp = '\0';
    localestr = setlocale(LC_ALL, buf);
    if (localestr) {
        if (console.localestr)
            free(console.localestr);
        console.localestr = strdup(localestr);
    }
    console.code_page = console.orig_code_page;
    if (console.has_unicode) {
        if (console.code_page != 437)
            success = SetConsoleOutputCP(437);
    } else if (console.code_page != 1252) {
        success = SetConsoleOutputCP(1252);
    }
    console.code_page = GetConsoleOutputCP();
#endif /* VIRTUAL_TERMINAL_SEQUENCES */

    /* check the font before we capture the code page map */
    check_and_set_font();
    set_cp_map();

#ifndef VIRTUAL_TERMINAL_SEQUENCES
    /* Set console mode */
    DWORD cmode, mask;
    GetConsoleMode(console.hConIn, &cmode);
#else /* VIRTUAL_TERMINAL_SEQUENCES */
    /* input console mode */
    DWORD disablemask;
    GetConsoleMode(console.hConIn, &console.orig_in_cmode);
    GetConsoleMode(console.hConOut, &console.orig_out_cmode);

    console.in_cmode = console.orig_in_cmode;
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
#ifdef NO_MOUSE_ALLOWED
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    mask = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT
#else
    disablemask = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT
#endif
           | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT;
#else
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    mask = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT
#else
    disablemask = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT
#endif
           | ENABLE_WINDOW_INPUT;
#endif
    /* Turn OFF the settings specified in the mask */
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    cmode &= ~mask;
#else
    console.in_cmode &= ~disablemask;
#endif
#ifndef NO_MOUSE_ALLOWED
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    cmode |= ENABLE_MOUSE_INPUT;
#else /* VIRTUAL_TERMINAL_SEQUENCES */
    console.in_cmode |= ENABLE_MOUSE_INPUT;
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
#endif /* NO_MOUSE_ALLOWED */
#ifndef VIRTUAL_TERMINAL_SEQUENCES
    SetConsoleMode(console.hConIn, cmode);
#else /* VIRTUAL_TERMINAL_SEQUENCES */
    SetConsoleMode(console.hConIn, console.in_cmode);

    console.out_cmode = console.orig_out_cmode;
    if ((console.out_cmode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0) {
        /* recognize escape sequences */
        console.out_cmode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(console.hConOut, console.out_cmode);
    }
    GetConsoleMode(console.hConOut, &console.out_cmode);
#endif /* VIRTUAL_TERMINAL_SEQUENCES */

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
#ifdef VIRTUAL_TERMINAL_SEQUENCES
    init_custom_colors();
#endif /* VIRTUAL_TERMINAL_SEQUENCES */
    console.current_nhcolor = NO_COLOR;
    console.is_ready = TRUE;
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
#ifndef VIRTUAL_TERMINAL_SEQUENCES

#endif /* ! VIRTUAL_TERMINAL_SEQUENCES */

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

/* NetHack 3.6	mhcmd.c	$NHDT-Date: 1524689383 2018/04/25 20:49:43 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.15 $ */
/*      Copyright (c) 2009 by Michael Allison              */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include <assert.h>
#include "mhcmd.h"
#include "mhinput.h"
#include "mhcolor.h"

static TCHAR szNHCmdWindowClass[] = TEXT("MSNethackCmdWndClass");

#ifndef C
#define C(c) (0x1f & (c))
#endif

/* cell status 0 */
#define NH_CST_CHECKED 1

/* fonts */
#define NH_CMDPAD_FONT_NORMAL 0
#define NH_CMDPAD_FONT_MAX 0

/* type of the cell */
#define NH_CELL_REG 0
#define NH_CELL_CTRL 1
#define NH_CELL_CAP 2
#define NH_CELL_SHIFT 3
#define NH_CELL_LAYOUT_NEW 4
#define NH_CELL_LAYOUT_MENU 5

#define NH_CMDSET_MAXSIZE 64

/* Keypad cell information

        NHCmdPadCell.cell_type    NHCmdPadCell.data
        -----------               ----------
        NH_CELL_REG				  (int)>=0 - index in the
   current keypad layout set (loads a new layout)
                                   -1      - restore default (saved) layout
        NH_CELL_CTRL			  not used
        NH_CELL_CAP				  not used
        NH_CELL_SHIFT			  not used
        NH_CELL_LAYOUT_NEW		  pointer to the new keypad layout
   layout (NHCmdLayout*)
        NH_CELL_LAYOUT_MENU       pointer to the layout set (NHCmdSet* - if
   NULL then nhcmdset_default is used)
*/
typedef struct t_NHCmdPadCell {
    UINT cmd_code; /* Windows command code (menu processing - not implemented
                      - set to -1) */
    char f_char[16]; /* nethack char */
    char text[16];   /* display text */
    int image;       /* >0 - image ID in IDB_KEYPAD bitmap
        <=0 - absolute index of the font table */
    int type;        /* cell type */
    int mult;        /* cell width multiplier */
    void *data;      /* internal data for the cell type */
} NHCmdPadCell, *PNHCmdPadCell;

/* command layout */
typedef struct t_NHCmdLayout {
    char name[64];
    int rows;
    int columns;
    NHCmdPadCell cells[];
} NHCmdLayout, *PNHCmdLayout;

/* set of command layouts */
typedef struct t_NHCmdSet {
    int count;
    struct t_NHCmdSetElem {
        PNHCmdLayout layout;
        BOOL free_on_destroy;
    } elements[NH_CMDSET_MAXSIZE];
} NHCmdSet, *PNHCmdSet;

/* display cell layout */
typedef struct t_NHCmdPadLayoutCell {
    POINT orig; /* origin of the cell rect */
    BYTE type;  /* cell type */
    int state;  /* cell state */
} NHCmdPadLayoutCell, *PNHCmdPadLayoutCell;

/* command window data */
typedef struct mswin_nethack_cmd_window {
    SIZE cell_size;                     /* cell size */
    HFONT font[NH_CMDPAD_FONT_MAX + 1]; /* fonts for cell text */
    HBITMAP images;                     /* key images map */
    int active_cell;                    /* current active cell */

    boolean is_caps;  /* is CAPS selected */
    boolean is_ctrl;  /* is CRTL selected */
    boolean is_shift; /* is SHIFT selected */

    PNHCmdLayout layout_current; /* current layout */
    PNHCmdLayout layout_save;    /* saved layout */
    PNHCmdPadLayoutCell cells;   /* display cells */

#if defined(WIN_CE_SMARTPHONE)
    PNHCmdLayout layout_selected; /* since we use
                                                                     layout
                     command for menu also
                     we need to store the layout
                                                                     that was
                     selected by a user
                                                                   */
#endif
} NHCmdWindow, *PNHCmdWindow;

LRESULT CALLBACK NHCommandWndProc(HWND, UINT, WPARAM, LPARAM);
static void register_command_window_class();
static void LayoutCmdWindow(HWND hWnd);
static void SetCmdWindowLayout(HWND hWnd, PNHCmdLayout layout);
static int CellFromPoint(PNHCmdWindow data, POINT pt);
static void CalculateCellSize(HWND hWnd, LPSIZE pSize, LPSIZE windowSize);
static void HighlightCell(HWND hWnd, int cell, BOOL isSelected);
static void ActivateCell(HWND hWnd, int cell);
static void PushNethackCommand(const char *cmd_char_str, int is_ctrl);

/*------------------- keyboard keys layout functions -----------------------*/
PNHCmdLayout nhcmdlayout_create(const char *name, int rows, int columns);
void nhcmdlayout_init(PNHCmdLayout p, PNHCmdPadCell cells);
#define nhcmdlayout_rows(p) ((p)->rows)
#define nhcmdlayout_columns(p) ((p)->columns)
#define nhcmdlayout_row(p, x) (&((p)->cells[(p)->columns * (x)]))
#define nhcmdlayout_cell(p, x, y) (&((p)->cells[(p)->columns * (x) + (y)]))
#define nhcmdlayout_cell_direct(p, i) (&((p)->cells[(i)]))
void nhcmdlayout_destroy(PNHCmdLayout p);

/*----------------- keyboard keys layout set functions ---------------------*/
PNHCmdSet nhcmdset_create();
int nhcmdset_count(PNHCmdSet p);
PNHCmdLayout nhcmdset_get(PNHCmdSet p, int index);
const char *nhcmdset_get_name(PNHCmdSet p, int index);
void nhcmdset_add(PNHCmdSet p, PNHCmdLayout layout);
void nhcmdset_destroy(PNHCmdSet p);

/*-------------------- message handlers -----------------------------------*/
static void onPaint(HWND hWnd);                                // on WM_PAINT
static void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam); // on WM_CREATE
static void onMouseDown(HWND hWnd, WPARAM wParam,
                        LPARAM lParam); // on WM_LBUTTONDOWN
static void onMouseMove(HWND hWnd, WPARAM wParam,
                        LPARAM lParam); // on WM_MOUSEMOVE
static void onMouseUp(HWND hWnd, WPARAM wParam,
                      LPARAM lParam); // on WM_LBUTTONUP

/*----------------------- static data -------------------------------------*/
static PNHCmdSet nhcmdset_current = 0;
static PNHCmdSet nhcmdset_default = 0;

/*---------------------- Pre-definde keyboard layouts --------------------*/
#ifdef WIN_CE_SMARTPHONE

/* dimensions of the command pad */
#define NH_CMDPAD_ROWS 4
#define NH_CMDPAD_COLS 3
#define NH_CMDPAD_CELLNUM (NH_CMDPAD_COLS * NH_CMDPAD_ROWS)

/* layout indexes */
#define NH_LAYOUT_GENERAL 0
#define NH_LAYOUT_MOVEMENT 1
#define NH_LAYOUT_ATTACK 2
#define NH_LAYOUT_ITEM_HANDLING 3
#define NH_LAYOUT_CONTROLS 4
#define NH_LAYOUT_ADV_MOVEMENT 5
#define NH_LAYOUT_ITEM_LOOKUP 6
#define NH_LAYOUT_WIZARD 7

/* template menu layout */
NHCmdPadCell cells_layout_menu[NH_CMDPAD_CELLNUM] = {
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "<<", -NH_CMDPAD_FONT_NORMAL, NH_CELL_LAYOUT_NEW, 1, NULL },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", ">>", -NH_CMDPAD_FONT_NORMAL, NH_CELL_LAYOUT_NEW, 1, NULL }
};

/* movement layout */
NHCmdPadCell cells_layout_movement[NH_CMDPAD_CELLNUM] = {
    { -1, "7", "7", 1, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "8", "8", 2, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "9", "9", 3, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "4", "4", 4, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ".", ".", 5, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "6", "6", 6, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "1", "1", 7, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "2", "2", 8, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "3", "3", 9, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "<", "<", 10, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ">", ">", 12, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "X", "X", 13, NH_CELL_LAYOUT_MENU, 1, 0 }
};

/* attack layout */
NHCmdPadCell cells_layout_attack[NH_CMDPAD_CELLNUM] = {
    { -1, "t", "t", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "w", "w", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "x", "x", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "f", "f", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "z", "z", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "Z", "Z", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "r", "r", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "a", "a", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "q", "q", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "\x04", "^D", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "F", "F", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) NH_LAYOUT_MOVEMENT },
    { -1, "X", "X", 13, NH_CELL_LAYOUT_MENU, 1, 0 }
};

/* item handling layout */
NHCmdPadCell cells_layout_item_handling[NH_CMDPAD_CELLNUM] = {
    { -1, "W", "W", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "P", "P", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "d", "d", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "T", "T", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "R", "R", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "D", "D", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "=", "=", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "i", "i", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "Q", "Q", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "A", "A", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "I", "I", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "X", "X", 13, NH_CELL_LAYOUT_MENU, 1, 0 }
};

/* General */
NHCmdPadCell cells_layout_general[NH_CMDPAD_CELLNUM] = {
    { -1, "q", "q", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "e", "e", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "l", "l", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "s", "s", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "E", "E", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "\x01", "^A", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "c", "c", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "o", "o", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "p", "p", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ":", ":", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ",", ",", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "X", "X", 13, NH_CELL_LAYOUT_MENU, 1, 0 }
};

/* game controls layout */
NHCmdPadCell cells_layout_game[NH_CMDPAD_CELLNUM] = {
    { -1, "S", "S", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "h", "h", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "C", "C", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "@", "@", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "\\", "\\", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "O", "O", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "&", "&", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "\x18", "^X", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "\x10", "^P", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "X", "X", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "#", "#", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "X", "X", 13, NH_CELL_LAYOUT_MENU, 1, 0 }
};

/* advanced movement layout */
NHCmdPadCell cells_layout_adv_movement[NH_CMDPAD_CELLNUM] = {
    { -1, "g", "g", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) NH_LAYOUT_MOVEMENT },
    { -1, "G", "G", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) NH_LAYOUT_MOVEMENT },
    { -1, "m", "m", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) NH_LAYOUT_MOVEMENT },
    { -1, "M", "M", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) NH_LAYOUT_MOVEMENT },
    { -1, "_", "_", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "\x14", "^T", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "j", "j", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "<", "<", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ">", ">", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "^", "^", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "X", "X", 13, NH_CELL_LAYOUT_MENU, 1, 0 }
};

/* item lookup layout */
NHCmdPadCell cells_layout_lookup[NH_CMDPAD_CELLNUM] = {
    { -1, ";", ";", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "^", "^", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "]", "]", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ")", ")", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "=", "=", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "*", "*", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "(", "(", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "\"", "\"", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "$", "$", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "+", "+", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "X", "X", 13, NH_CELL_LAYOUT_MENU, 1, 0 }
};

/* wizard mode layout */
NHCmdPadCell cells_layout_wizard[NH_CMDPAD_CELLNUM] = {
    { -1, "\x05", "^e", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "\x06", "^f", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "\x07", "^g", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "\x09", "^i", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "\x0f", "^o", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "\x16", "^v", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "\x17", "^w", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "\x14", "^T", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1,
      (void *) - 1 },
    { -1, "#", "#", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "X", "X", 13, NH_CELL_LAYOUT_MENU, 1, 0 }
};

#else /* !WIN_CE_SMARTPHONE */

/* dimensions of the command pad */
#define NH_CMDPAD_ROWS 4
#define NH_CMDPAD_COLS 14
#define NH_CMDPAD_CELLNUM (NH_CMDPAD_COLS * NH_CMDPAD_ROWS)

/* lowercase layout */
NHCmdPadCell cells_layout_mod1[NH_CMDPAD_ROWS * NH_CMDPAD_COLS] = {
    { -1, "7", "7", 1, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "8", "8", 2, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "9", "9", 3, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "\x1b", "Esc", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 2, NULL },
    { -1, " ", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0,
      NULL }, /* complement for ESC */
    { -1, "?", "?", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "*", "*", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ",", ",", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "/", "/", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ":", ":", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ";", ";", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "-", "-", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "#", "#", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "^", "^", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },

    { -1, "4", "4", 4, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "5", "5", 5, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "6", "6", 6, NH_CELL_REG, 1, (void *) - 1 },
    { -1, " ", "CAP", -NH_CMDPAD_FONT_NORMAL, NH_CELL_CAP, 2, NULL },
    { -1, " ", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0,
      NULL }, /* complement for CAPS */
    { -1, "a", "a", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "b", "b", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "c", "c", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "d", "d", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "e", "e", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "f", "f", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "g", "g", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "h", "h", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "i", "i", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },

    { -1, "1", "1", 7, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "2", "2", 8, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "3", "3", 9, NH_CELL_REG, 1, (void *) - 1 },
    { -1, " ", "Shft", -NH_CMDPAD_FONT_NORMAL, NH_CELL_SHIFT, 2, NULL },
    { -1, " ", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0,
      NULL }, /* complement for shift */
    { -1, "j", "j", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "k", "k", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "l", "l", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "m", "m", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "n", "n", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "o", "o", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "p", "p", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "q", "q", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "r", "r", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },

    { -1, "<", "<", 10, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ".", ".", 11, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ">", ">", 12, NH_CELL_REG, 1, (void *) - 1 },
    { -1, " ", "Ctrl", -NH_CMDPAD_FONT_NORMAL, NH_CELL_CTRL, 2, NULL },
    { -1, " ", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0,
      NULL }, /* complement for CTRL */
    { -1, "s", "s", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "t", "t", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "u", "u", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "v", "v", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "w", "w", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "x", "x", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "y", "y", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "z", "z", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "\\", "\\", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 }
};

/* uppercase layout */
NHCmdPadCell cells_layout_mod2[-NH_CMDPAD_ROWS * -NH_CMDPAD_COLS] = {
    { -1, "7", "7", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "8", "8", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "9", "9", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "\x1b", "Esc", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 2, NULL },
    { -1, " ", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0,
      NULL }, /* complement for ESC */
    { -1, "?", "?", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "*", "*", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "[", "[", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "(", "(", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ")", ")", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "+", "+", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "=", "=", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "\"", "\"", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "$", "$", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },

    { -1, "4", "4", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "5", "5", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "6", "6", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, " ", "CAP", -NH_CMDPAD_FONT_NORMAL, NH_CELL_CAP, 2, NULL },
    { -1, " ", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0,
      NULL }, /* complement for CAPS */
    { -1, "A", "A", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "B", "B", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "C", "C", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "D", "D", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "E", "E", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "F", "F", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "G", "G", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "H", "H", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "I", "I", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },

    { -1, "1", "1", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "2", "2", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "3", "3", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, " ", "Shft", -NH_CMDPAD_FONT_NORMAL, NH_CELL_SHIFT, 2, NULL },
    { -1, " ", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0,
      NULL }, /* complement for shift */
    { -1, "J", "J", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "K", "K", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "L", "L", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "M", "M", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "N", "N", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "O", "O", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "P", "P", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "Q", "Q", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "R", "R", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },

    { -1, "<", "<", 10, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "0", "0", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, ">", ">", 12, NH_CELL_REG, 1, (void *) - 1 },
    { -1, " ", "Ctrl", -NH_CMDPAD_FONT_NORMAL, NH_CELL_CTRL, 2, NULL },
    { -1, " ", "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0,
      NULL }, /* complement for CTRL */
    { -1, "S", "S", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "T", "T", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "U", "U", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "V", "V", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "W", "W", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "X", "X", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "Y", "Y", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "Z", "Z", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 },
    { -1, "@", "@", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1, (void *) - 1 }
};

#endif /* !WIN_CE_SMARTPHONE */

/*-------------------------------------------------------------------------*/
HWND
mswin_init_command_window()
{
    static int run_once = 0;
    HWND ret;

    /* register window class */
    if (!run_once) {
        register_command_window_class();
        run_once = 1;
    }

    /* create window */
    ret = CreateWindow(
        szNHCmdWindowClass,         /* registered class name */
        NULL,                       /* window name */
        WS_CHILD | WS_CLIPSIBLINGS, /* window style */
        0, /* horizontal position of window - set it later */
        0, /* vertical position of window - set it later */
        0, /* window width - set it later */
        0, /* window height - set it later*/
        GetNHApp()->hMainWnd, /* handle to parent or owner window */
        NULL,                 /* menu handle or child identifier */
        GetNHApp()->hApp,     /* handle to application instance */
        NULL);                /* window-creation data */
    if (!ret) {
        panic("Cannot create command window");
    }
    return ret;
}
/*-------------------------------------------------------------------------*/
/* calculate mimimum window size */
void
mswin_command_window_size(HWND hwnd, LPSIZE sz)
{
    SIZE cell_size;
    PNHCmdWindow data;
    data = (PNHCmdWindow) GetWindowLong(hwnd, GWL_USERDATA);
    if (!data) {
        sz->cx = sz->cy = 0;
    } else {
        CalculateCellSize(hwnd, &cell_size, sz);
        sz->cx = max(cell_size.cx * nhcmdlayout_columns(data->layout_current)
                         + 2 * GetSystemMetrics(SM_CXBORDER),
                     sz->cx);
        sz->cy = max(cell_size.cy * nhcmdlayout_rows(data->layout_current)
                         + 2 * GetSystemMetrics(SM_CYBORDER),
                     sz->cy);
    }
}
/*-------------------------------------------------------------------------*/
void
register_command_window_class()
{
    WNDCLASS wcex;
    PNHCmdLayout plt;
    ZeroMemory(&wcex, sizeof(wcex));

    /* window class */
    wcex.style = CS_NOCLOSE;
    wcex.lpfnWndProc = (WNDPROC) NHCommandWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetNHApp()->hApp;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = mswin_get_brush(NHW_KEYPAD, MSWIN_COLOR_BG);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szNHCmdWindowClass;

    if (!RegisterClass(&wcex)) {
        panic("cannot register Map window class");
    }

    /* create default command set */
    nhcmdset_current = nhcmdset_default = nhcmdset_create();

#ifdef WIN_CE_SMARTPHONE
    plt = nhcmdlayout_create("General", NH_CMDPAD_ROWS, NH_CMDPAD_COLS);
    nhcmdlayout_init(plt, cells_layout_general);
    nhcmdset_add(nhcmdset_current, plt);

    plt = nhcmdlayout_create("Movement", NH_CMDPAD_ROWS, NH_CMDPAD_COLS);
    nhcmdlayout_init(plt, cells_layout_movement);
    nhcmdset_add(nhcmdset_current, plt);

    plt = nhcmdlayout_create("Attack", NH_CMDPAD_ROWS, NH_CMDPAD_COLS);
    nhcmdlayout_init(plt, cells_layout_attack);
    nhcmdset_add(nhcmdset_current, plt);

    plt = nhcmdlayout_create("Item Handling", NH_CMDPAD_ROWS, NH_CMDPAD_COLS);
    nhcmdlayout_init(plt, cells_layout_item_handling);
    nhcmdset_add(nhcmdset_current, plt);

    plt = nhcmdlayout_create("Game Controls", NH_CMDPAD_ROWS, NH_CMDPAD_COLS);
    nhcmdlayout_init(plt, cells_layout_game);
    nhcmdset_add(nhcmdset_current, plt);

    plt = nhcmdlayout_create("Advanced Movement", NH_CMDPAD_ROWS,
                             NH_CMDPAD_COLS);
    nhcmdlayout_init(plt, cells_layout_adv_movement);
    nhcmdset_add(nhcmdset_current, plt);

    plt = nhcmdlayout_create("Item Lookup", NH_CMDPAD_ROWS, NH_CMDPAD_COLS);
    nhcmdlayout_init(plt, cells_layout_lookup);
    nhcmdset_add(nhcmdset_current, plt);

    if (wizard) {
        plt =
            nhcmdlayout_create("Wizard Mode", NH_CMDPAD_ROWS, NH_CMDPAD_COLS);
        nhcmdlayout_init(plt, cells_layout_wizard);
        nhcmdset_add(nhcmdset_current, plt);
    }

#else  /* ! WIN_CE_SMARTPHONE */
    plt = nhcmdlayout_create("lowercase", NH_CMDPAD_ROWS, NH_CMDPAD_COLS);
    nhcmdlayout_init(plt, cells_layout_mod1);
    nhcmdset_add(nhcmdset_current, plt);

    plt = nhcmdlayout_create("uppercase", NH_CMDPAD_ROWS, NH_CMDPAD_COLS);
    nhcmdlayout_init(plt, cells_layout_mod2);
    nhcmdset_add(nhcmdset_current, plt);
#endif /* WIN_CE_SMARTPHONE */
}
/*-------------------------------------------------------------------------*/
LRESULT CALLBACK
NHCommandWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PNHCmdWindow data;
    int i;

    switch (message) {
    case WM_CREATE:
        onCreate(hWnd, wParam, lParam);
        break;

    case WM_PAINT:
        onPaint(hWnd);
        break;

    case WM_SIZE:
        LayoutCmdWindow(hWnd);
        break;

    case WM_LBUTTONDOWN:
        onMouseDown(hWnd, wParam, lParam);
        return 0;

    case WM_MOUSEMOVE:
        /* proceed only if if have mouse focus (set in onMouseDown() -
           left mouse button is pressed) */
        if (GetCapture() == hWnd) {
            onMouseMove(hWnd, wParam, lParam);
            return 0;
        } else {
            return 1;
        }
        break;

    case WM_LBUTTONUP:
        /* proceed only if if have mouse focus (set in onMouseDown()) */
        if (GetCapture() == hWnd) {
            onMouseUp(hWnd, wParam, lParam);
            return 0;
        } else {
            return 1;
        }
        break;

    case WM_DESTROY:
        data = (PNHCmdWindow) GetWindowLong(hWnd, GWL_USERDATA);
        for (i = 0; i <= NH_CMDPAD_FONT_MAX; i++)
            if (data->font[i])
                DeleteObject(data->font[i]);
        free(data);
        SetWindowLong(hWnd, GWL_USERDATA, (LONG) 0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return FALSE;
}
/*-------------------------------------------------------------------------*/
void
onPaint(HWND hWnd)
{
    PNHCmdWindow data;
    PAINTSTRUCT ps;
    HDC hDC;
    int x, y;
    TCHAR wbuf[BUFSZ];
    HGDIOBJ saveFont;
    BITMAP bm;
    int cell_index;

    /* get window data */
    data = (PNHCmdWindow) GetWindowLong(hWnd, GWL_USERDATA);

    hDC = BeginPaint(hWnd, &ps);

    if (!IsRectEmpty(&ps.rcPaint)) {
        HGDIOBJ oldBr;
        HBRUSH hbrPattern;
        COLORREF OldBg, OldFg;
        HPEN hPen;
        HGDIOBJ hOldPen;

        saveFont = SelectObject(hDC, data->font[NH_CMDPAD_FONT_NORMAL]);
        OldBg = SetBkColor(hDC, mswin_get_color(NHW_KEYPAD, MSWIN_COLOR_BG));
        OldFg =
            SetTextColor(hDC, mswin_get_color(NHW_KEYPAD, MSWIN_COLOR_FG));

        GetObject(data->images, sizeof(BITMAP), (LPVOID) &bm);

        hbrPattern = CreatePatternBrush(data->images);
        hPen = CreatePen(PS_SOLID, 1,
                         mswin_get_color(NHW_KEYPAD, MSWIN_COLOR_FG));

        for (x = 0, cell_index = 0;
             x < nhcmdlayout_rows(data->layout_current); x++)
            for (y = 0; y < nhcmdlayout_columns(data->layout_current);
                 y++, cell_index++) {
                RECT cell_rt;
                POINT pt[5];
                PNHCmdPadCell p_cell_data;

                p_cell_data =
                    nhcmdlayout_cell_direct(data->layout_current, cell_index);

                /* calculate the cell rectangle */
                cell_rt.left = data->cells[cell_index].orig.x;
                cell_rt.top = data->cells[cell_index].orig.y;
                cell_rt.right = data->cells[cell_index].orig.x
                                + data->cell_size.cx * p_cell_data->mult;
                cell_rt.bottom =
                    data->cells[cell_index].orig.y + data->cell_size.cy;

                /* draw border */
                hOldPen = SelectObject(hDC, hPen);
                pt[0].x = cell_rt.left;
                pt[0].y = cell_rt.top;
                pt[1].x = cell_rt.right;
                pt[1].y = cell_rt.top;
                pt[2].x = cell_rt.right;
                pt[2].y = cell_rt.bottom;
                pt[3].x = cell_rt.left;
                pt[3].y = cell_rt.bottom;
                pt[4].x = cell_rt.left;
                pt[4].y = cell_rt.top;
                Polyline(hDC, pt, 5);
                SelectObject(hDC, hOldPen);

                /* calculate clipping rectangle for the text */
                cell_rt.left++;
                cell_rt.top++;
                cell_rt.right--;
                cell_rt.bottom--;

                /* draw the cell text */
                if (p_cell_data->image <= 0) {
                    SelectObject(hDC, data->font[-p_cell_data->image]);
                    DrawText(hDC, NH_A2W(p_cell_data->text, wbuf, BUFSZ),
                             strlen(p_cell_data->text), &cell_rt,
                             DT_CENTER | DT_VCENTER | DT_SINGLELINE
                                 | DT_NOPREFIX);
                } else {
                    /* draw bitmap */
                    int bmOffset;
                    RECT bitmap_rt;

                    bmOffset = (p_cell_data->image - 1) * bm.bmHeight;

                    bitmap_rt.left =
                        ((cell_rt.left + cell_rt.right)
                         - min(bm.bmHeight, (cell_rt.right - cell_rt.left)))
                        / 2;
                    bitmap_rt.top =
                        ((cell_rt.bottom + cell_rt.top)
                         - min(bm.bmHeight, (cell_rt.bottom - cell_rt.top)))
                        / 2;
                    bitmap_rt.right =
                        bitmap_rt.left
                        + min(bm.bmHeight, (cell_rt.right - cell_rt.left));
                    bitmap_rt.bottom =
                        bitmap_rt.top
                        + min(bm.bmHeight, (cell_rt.bottom - cell_rt.top));

                    SetBrushOrgEx(hDC, bitmap_rt.left - bmOffset,
                                  bitmap_rt.top, NULL);
                    oldBr = SelectObject(hDC, hbrPattern);
                    PatBlt(hDC, bitmap_rt.left, bitmap_rt.top,
                           bitmap_rt.right - bitmap_rt.left,
                           bitmap_rt.bottom - bitmap_rt.top, PATCOPY);
                    SelectObject(hDC, oldBr);
                }

                /* invert the cell if it is selected */
                if (data->cells[cell_index].state == NH_CST_CHECKED) {
                    IntersectRect(&cell_rt, &cell_rt, &ps.rcPaint);
                    PatBlt(hDC, cell_rt.left, cell_rt.top,
                           cell_rt.right - cell_rt.left,
                           cell_rt.bottom - cell_rt.top, DSTINVERT);
                }
            }

        SetTextColor(hDC, OldFg);
        SetBkColor(hDC, OldBg);
        SelectObject(hDC, saveFont);
        DeleteObject(hbrPattern);
        DeleteObject(hPen);
    }
    EndPaint(hWnd, &ps);
}
/*-------------------------------------------------------------------------*/
void
onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHCmdWindow data;

    /* set window data */
    data = (PNHCmdWindow) malloc(sizeof(NHCmdWindow));
    if (!data)
        panic("out of memory");

    ZeroMemory(data, sizeof(NHCmdWindow));
    SetWindowLong(hWnd, GWL_USERDATA, (LONG) data);

    data->active_cell = -1;

    /* load images bitmap */
    data->images = LoadBitmap(GetNHApp()->hApp, MAKEINTRESOURCE(IDB_KEYPAD));
    if (!data->images)
        panic("cannot load keypad bitmap");

    /* create default layouts */
    data->layout_current = 0;
    data->layout_save = 0;
    data->cells = 0;

#if defined(WIN_CE_SMARTPHONE)
    data->layout_selected = nhcmdset_get(nhcmdset_current, 0);
#endif

    /* set default layout */
    SetCmdWindowLayout(hWnd, nhcmdset_get(nhcmdset_current, 0));
}
/*-------------------------------------------------------------------------*/
void
LayoutCmdWindow(HWND hWnd)
{
    RECT clrt;
    SIZE windowSize;
    PNHCmdWindow data;
    int i, j;
    int x, y;
    LOGFONT lgfnt;
    int index;

    GetClientRect(hWnd, &clrt);
    if (IsRectEmpty(&clrt))
        return;

    data = (PNHCmdWindow) GetWindowLong(hWnd, GWL_USERDATA);
    if (!data->layout_current)
        return;

    /* calculate cell size */
    windowSize.cx = clrt.right - clrt.left;
    windowSize.cy = clrt.bottom - clrt.top;
    CalculateCellSize(hWnd, &data->cell_size, &windowSize);

    /* initialize display cells aray */
    x = 0;
    y = 0;
    for (i = 0, index = 0; i < nhcmdlayout_rows(data->layout_current); i++) {
        for (j = 0; j < nhcmdlayout_columns(data->layout_current);
             j++, index++) {
            data->cells[index].orig.x = x;
            data->cells[index].orig.y = y;
            data->cells[index].type =
                nhcmdlayout_cell_direct(data->layout_current, index)->type;

            switch (data->cells[index].type) {
            case NH_CELL_CTRL:
                data->cells[index].state = data->is_ctrl ? NH_CST_CHECKED : 0;
                break;
            case NH_CELL_CAP:
                data->cells[index].state = data->is_caps ? NH_CST_CHECKED : 0;
                break;
            case NH_CELL_SHIFT:
                data->cells[index].state =
                    data->is_shift ? NH_CST_CHECKED : 0;
                break;
            default:
                data->cells[index].state = 0;
            }

            x += data->cell_size.cx
                 * nhcmdlayout_cell_direct(data->layout_current, index)->mult;
        }
        x = 0;
        y += data->cell_size.cy;
    }

    /* create font for display cell text */
    for (i = 0; i <= NH_CMDPAD_FONT_MAX; i++)
        if (data->font[i])
            DeleteObject(data->font[i]);

    ZeroMemory(&lgfnt, sizeof(lgfnt));
    lgfnt.lfHeight = data->cell_size.cy;       // height of font
    lgfnt.lfWidth = 0;                         // average character width
    lgfnt.lfEscapement = 0;                    // angle of escapement
    lgfnt.lfOrientation = 0;                   // base-line orientation angle
    lgfnt.lfWeight = FW_NORMAL;                // font weight
    lgfnt.lfItalic = FALSE;                    // italic attribute option
    lgfnt.lfUnderline = FALSE;                 // underline attribute option
    lgfnt.lfStrikeOut = FALSE;                 // strikeout attribute option
    lgfnt.lfCharSet = ANSI_CHARSET;            // character set identifier
    lgfnt.lfOutPrecision = OUT_DEFAULT_PRECIS; // output precision
    lgfnt.lfClipPrecision = CLIP_CHARACTER_PRECIS; // clipping precision
    lgfnt.lfQuality = DEFAULT_QUALITY;             // output quality
    if (iflags.wc_font_message && *iflags.wc_font_message) {
        lgfnt.lfPitchAndFamily = DEFAULT_PITCH; // pitch and family
        NH_A2W(iflags.wc_font_message, lgfnt.lfFaceName, LF_FACESIZE);
    } else {
        lgfnt.lfPitchAndFamily = VARIABLE_PITCH; // pitch and family
    }
    data->font[NH_CMDPAD_FONT_NORMAL] = CreateFontIndirect(&lgfnt);

    InvalidateRect(hWnd, NULL, TRUE);
}
/*-------------------------------------------------------------------------*/
void
SetCmdWindowLayout(HWND hWnd, PNHCmdLayout layout)
{
    PNHCmdWindow data;
    int size;

    data = (PNHCmdWindow) GetWindowLong(hWnd, GWL_USERDATA);
    if (data->layout_current == layout)
        return;

    data->layout_current = layout;
    size = sizeof(NHCmdPadLayoutCell) * nhcmdlayout_rows(layout)
           * nhcmdlayout_columns(layout);
    data->cells = (PNHCmdPadLayoutCell) realloc(data->cells, size);
    ZeroMemory(data->cells, size);
    LayoutCmdWindow(hWnd);
}
/*-------------------------------------------------------------------------*/
void
onMouseDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHCmdWindow data;
    POINT mpt;

    /* get mouse coordinates */
    mpt.x = LOWORD(lParam);
    mpt.y = HIWORD(lParam);

    /* map mouse coordinates to the display cell */
    data = (PNHCmdWindow) GetWindowLong(hWnd, GWL_USERDATA);
    data->active_cell = CellFromPoint(data, mpt);
    if (data->active_cell == -1)
        return;

    /* set mouse focus to the current window */
    SetCapture(hWnd);

    /* invert the selection */
    HighlightCell(hWnd, data->active_cell,
                  (data->cells[data->active_cell].state != NH_CST_CHECKED));
}
/*-------------------------------------------------------------------------*/
void
onMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHCmdWindow data;
    POINT mpt;
    int newActiveCell;

    /* get mouse coordinates */
    mpt.x = LOWORD(lParam);
    mpt.y = HIWORD(lParam);

    /* map mouse coordinates to the display cell */
    data = (PNHCmdWindow) GetWindowLong(hWnd, GWL_USERDATA);
    newActiveCell = CellFromPoint(data, mpt);
    if (data->active_cell == -1)
        return;

    /* if mouse is within orginal display cell - select the cell otherwise
       clear the selection */
    switch (nhcmdlayout_cell_direct(data->layout_current, data->active_cell)
                ->type) {
    case NH_CELL_REG:
        HighlightCell(hWnd, data->active_cell,
                      (newActiveCell == data->active_cell));
        break;

    case NH_CELL_CTRL:
        HighlightCell(hWnd, data->active_cell,
                      ((newActiveCell == data->active_cell) ? !data->is_ctrl
                                                            : data->is_ctrl));
        break;

    case NH_CELL_CAP:
        HighlightCell(hWnd, data->active_cell,
                      ((newActiveCell == data->active_cell) ? !data->is_caps
                                                            : data->is_caps));
        break;
    }
}
/*-------------------------------------------------------------------------*/
void
onMouseUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHCmdWindow data;

    /* release mouse capture */
    ReleaseCapture();

    /* get active display cell */
    data = (PNHCmdWindow) GetWindowLong(hWnd, GWL_USERDATA);
    if (data->active_cell == -1)
        return;

    ActivateCell(hWnd, data->active_cell);

    data->active_cell = -1;
}
/*-------------------------------------------------------------------------*/
void
ActivateCell(HWND hWnd, int cell)
{
    PNHCmdWindow data;
    PNHCmdPadCell p_cell_data;
    int i;

    data = (PNHCmdWindow) GetWindowLong(hWnd, GWL_USERDATA);
    if (!data)
        return;
    p_cell_data = nhcmdlayout_cell_direct(data->layout_current, cell);

    /* act depending on the cell type:
            CAPS - change layout
            CTRL - modify CTRL status
            REG  - place keyboard event on the nethack input queue
    */
    switch (p_cell_data->type) {
    case NH_CELL_REG:
        if (data->is_ctrl) {
            PushNethackCommand(p_cell_data->f_char, 1);

            data->is_ctrl = 0;
            for (i = 0; i < nhcmdlayout_rows(data->layout_current)
                                * nhcmdlayout_columns(data->layout_current);
                 i++) {
                if (nhcmdlayout_cell_direct(data->layout_current, i)->type
                    == NH_CELL_CTRL) {
                    HighlightCell(hWnd, i, data->is_ctrl);
                }
            }
        } else {
            PushNethackCommand(p_cell_data->f_char, 0);
        }
        HighlightCell(hWnd, cell, FALSE);

        // select a new layout if present
        i = (int) p_cell_data->data;
        if (i == -1) {
            if (data->layout_save)
                SetCmdWindowLayout(hWnd, data->layout_save);
            data->layout_save = NULL;
        } else {
            if (!data->layout_save)
                data->layout_save = data->layout_current;
            SetCmdWindowLayout(hWnd, nhcmdset_get(nhcmdset_current, i));
        }

        if (!data->is_shift)
            break;
    // else fall through and reset the shift

    case NH_CELL_SHIFT:
        data->is_shift = !data->is_shift;
        SetCmdWindowLayout(hWnd, (data->is_shift ^ data->is_caps)
                                     ? nhcmdset_get(nhcmdset_current, 1)
                                     : nhcmdset_get(nhcmdset_current, 0));
        data->cells[cell].state = data->is_shift ? NH_CST_CHECKED : 0;
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case NH_CELL_CTRL:
        data->is_ctrl = !data->is_ctrl;
        HighlightCell(hWnd, cell, data->is_ctrl);
        break;

    case NH_CELL_CAP:
        data->is_caps = !data->is_caps;
        SetCmdWindowLayout(hWnd, (data->is_shift ^ data->is_caps)
                                     ? nhcmdset_get(nhcmdset_current, 1)
                                     : nhcmdset_get(nhcmdset_current, 0));
        data->cells[cell].state = data->is_caps ? NH_CST_CHECKED : 0;
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case NH_CELL_LAYOUT_NEW: {
        PNHCmdLayout pLayout;

        HighlightCell(hWnd, cell, FALSE);

        pLayout = (PNHCmdLayout) p_cell_data->data;
        if (pLayout) {
            SetCmdWindowLayout(hWnd, pLayout);
        }
    } break;

    case NH_CELL_LAYOUT_MENU: {
        winid wid;
        int i;
        anything any;
        menu_item *selected = 0;
        PNHCmdSet pSet;

        HighlightCell(hWnd, cell, FALSE);

        pSet = (PNHCmdSet) p_cell_data->data;
        if (!pSet)
            pSet = nhcmdset_default;

        wid = mswin_create_nhwindow(NHW_MENU);
        mswin_start_menu(wid);
        for (i = 0; i < nhcmdset_count(pSet); i++) {
            any.a_void = nhcmdset_get(pSet, i);
            mswin_add_menu(wid, NO_GLYPH, &any, 'a' + i, 0, ATR_NONE,
                           nhcmdset_get_name(pSet, i), FALSE);
        }
        mswin_end_menu(wid, "Select keypad layout");
        i = select_menu(wid, PICK_ONE, &selected);
        mswin_destroy_nhwindow(wid);

        if (i == 1) {
#if defined(WIN_CE_SMARTPHONE)
            data->layout_selected = (PNHCmdLayout) selected[0].item.a_void;
#endif
            SetCmdWindowLayout(hWnd, (PNHCmdLayout) selected[0].item.a_void);
        }
    } break;
    }
}
/*-------------------------------------------------------------------------*/
int
CellFromPoint(PNHCmdWindow data, POINT pt)
{
    int i;
    for (i = 0; i < nhcmdlayout_rows(data->layout_current)
                        * nhcmdlayout_columns(data->layout_current);
         i++) {
        RECT cell_rt;
        cell_rt.left = data->cells[i].orig.x;
        cell_rt.top = data->cells[i].orig.y;
        cell_rt.right =
            data->cells[i].orig.x
            + data->cell_size.cx
                  * nhcmdlayout_cell_direct(data->layout_current, i)->mult;
        cell_rt.bottom = data->cells[i].orig.y + data->cell_size.cy;
        if (PtInRect(&cell_rt, pt))
            return i;
    }
    return -1;
}
/*-------------------------------------------------------------------------*/
void
CalculateCellSize(HWND hWnd, LPSIZE pSize, LPSIZE pWindowSize)
{
    HDC hdc;
    PNHCmdWindow data;
    data = (PNHCmdWindow) GetWindowLong(hWnd, GWL_USERDATA);
    if (!data)
        return;

    hdc = GetDC(hWnd);

    /* if windows size is specified - attempt ro stretch cells across
       the window size. If not - make default cell size based on
       10 points font. Make sure that cell cesize does not exceeds 20 points
       */
    if (pWindowSize->cx > 0)
        pSize->cx =
            pWindowSize->cx / nhcmdlayout_columns(data->layout_current);
    else
        pSize->cx = 10 * GetDeviceCaps(hdc, LOGPIXELSX) / 72;
    pSize->cx = min(pSize->cx, 20 * GetDeviceCaps(hdc, LOGPIXELSX) / 72);

    if (pWindowSize->cy > 0)
        pSize->cy = pWindowSize->cy / nhcmdlayout_rows(data->layout_current);
    else
        pSize->cy = 10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72;
    pSize->cy = min(pSize->cy, 20 * GetDeviceCaps(hdc, LOGPIXELSY) / 72);

    ReleaseDC(hWnd, hdc);
}
/*-------------------------------------------------------------------------*/
void
HighlightCell(HWND hWnd, int cell, BOOL isSelected)
{
    HDC hDC;
    PNHCmdWindow data;
    int prevState;

    data = (PNHCmdWindow) GetWindowLong(hWnd, GWL_USERDATA);
    prevState = data->cells[cell].state;
    data->cells[cell].state = (isSelected) ? NH_CST_CHECKED : 0;

    if (prevState != data->cells[cell].state) {
        hDC = GetDC(hWnd);
        PatBlt(hDC, data->cells[cell].orig.x + 1,
               data->cells[cell].orig.y + 1,
               data->cell_size.cx
                       * nhcmdlayout_cell_direct(data->layout_current, cell)
                             ->mult
                   - 2,
               data->cell_size.cy - 2, DSTINVERT);
        ReleaseDC(hWnd, hDC);
    }
}
/*-------------------------------------------------------------------------*/
void
PushNethackCommand(const char *cmd_char_str, int is_ctrl)
{
    while (*cmd_char_str) {
        if (is_ctrl) {
            NHEVENT_KBD(C(*cmd_char_str));
        } else {
            NHEVENT_KBD(*cmd_char_str);
        }
        cmd_char_str++;
    }
}

/*-------------------------------------------------------------------------*/
/*------------------- keyboard keys layout functions ----------------------*/
/*-------------------------------------------------------------------------*/
PNHCmdLayout
nhcmdlayout_create(const char *name, int rows, int columns)
{
    PNHCmdLayout p;
    int i;

    i = sizeof(NHCmdLayout) + rows * columns * sizeof(NHCmdPadCell);
    p = (PNHCmdLayout) malloc(i);
    ZeroMemory(p, i);
    p->rows = rows;
    p->columns = columns;
    strncpy(p->name, name, sizeof(p->name) - 1);
    for (i = 0; i < rows * columns; i++) {
        p->cells[i].cmd_code = -1;
        p->cells[i].image = -NH_CMDPAD_FONT_NORMAL;
        p->cells[i].type = 1;
        p->cells[i].mult = 1;
    }
    return p;
}
/*-------------------------------------------------------------------------*/
void
nhcmdlayout_init(PNHCmdLayout p, PNHCmdPadCell cells)
{
    memcpy(p->cells, cells, p->rows * p->columns * sizeof(NHCmdPadCell));
}

void
nhcmdlayout_destroy(PNHCmdLayout p)
{
    free(p);
}

/*-------------------------------------------------------------------------*/
/*----------------- keyboard keys layout set functions --------------------*/
/*-------------------------------------------------------------------------*/
PNHCmdSet
nhcmdset_create()
{
    PNHCmdSet p;
    p = (PNHCmdSet) malloc(sizeof(NHCmdSet));
    ZeroMemory(p, sizeof(NHCmdSet));
    return p;
}
/*-------------------------------------------------------------------------*/
int
nhcmdset_count(PNHCmdSet p)
{
    assert(p);
    return p->count;
}
/*-------------------------------------------------------------------------*/
PNHCmdLayout
nhcmdset_get(PNHCmdSet p, int index)
{
    assert(p);
    assert(index >= 0 && index < p->count);
    return p->elements[index].layout;
}
/*-------------------------------------------------------------------------*/
const char *
nhcmdset_get_name(PNHCmdSet p, int index)
{
    assert(p);
    assert(index >= 0 && index < p->count);
    return p->elements[index].layout->name;
}
/*-------------------------------------------------------------------------*/
void
nhcmdset_add(PNHCmdSet p, PNHCmdLayout layout)
{
    assert(p);
    assert(p->count < NH_CMDSET_MAXSIZE);
    p->elements[p->count].layout = layout;
    p->elements[p->count].free_on_destroy = 0;
    p->count++;
}
/*-------------------------------------------------------------------------*/
void
nhcmdset_destroy(PNHCmdSet p)
{
    int i = 0;
    assert(p);
    for (i = 0; i < p->count; i++) {
        if (p->elements[i].free_on_destroy) {
            nhcmdlayout_destroy(p->elements[i].layout);
        }
    }
    free(p);
}
/*-------------------------------------------------------------------------*/

#if defined(WIN_CE_SMARTPHONE)
/* special keypad input handling for SmartPhone
   the phone keypad maps to VK_* as shown below.
   some keys might not be present, e.g. VK_TFLIP
    sofkey1     softkey2    VK_TSOFT1, VK_TSOFT2
            ^               VK_TUP
        <   +   >           VK_TLEFT, VK_TACTION, VK_TRIGHT
            v               VK_TDOWN
    home        back        VK_THOME, VK_TBACK
    talk        end         VK_TTALK, VK_TEND
    1       2       3       VK_T0..VK_T9
    4       5       6       ...
    7       8       9       ...
    *       0       #       VK_TSTAR, VK_TPOUND
   other buttons include
    VK_TRECORD
    VK_TPOWER, VK_TVOLUMEUP, VK_TVOLUMEDOWN
    VK_TFLIP
*/
BOOL
NHSPhoneTranslateKbdMessage(WPARAM wParam, LPARAM lParam, BOOL keyDown)
{
    PNHCmdWindow data;
    int index = -1;

    /* get window data */
    data = (PNHCmdWindow) GetWindowLong(GetNHApp()->hCmdWnd, GWL_USERDATA);
    if (!data)
        return FALSE;

    switch (wParam) {
    case VK_T0:
        index = 10;
        break;

    case VK_T1:
        index = 0;
        break;

    case VK_T2:
        index = 1;
        break;

    case VK_T3:
        index = 2;
        break;

    case VK_T4:
        index = 3;
        break;

    case VK_T5:
        index = 4;
        break;

    case VK_T6:
        index = 5;
        break;

    case VK_T7:
        index = 6;
        break;

    case VK_T8:
        index = 7;
        break;

    case VK_T9:
        index = 8;
        break;

    case VK_TSTAR:
        index = 9;
        break;

    case VK_TPOUND:
        index = 11;
        break;
    }

    if (index >= 0) {
        HighlightCell(GetNHApp()->hCmdWnd, index, keyDown);
        if (keyDown)
            ActivateCell(GetNHApp()->hCmdWnd, index);
        return TRUE;
    } else {
        return FALSE;
    }
}
/*-------------------------------------------------------------------------*/
void
NHSPhoneSetKeypadFromString(const char *str)
{
    PNHCmdWindow data;
    PNHCmdSet p = 0;
    PNHCmdLayout layout_prev = 0;
    PNHCmdLayout layout_cur = 0;
    char buf[2][BUFSZ];
    int i, lcount;
    char *s;

    assert(NH_CMDPAD_ROWS == 4);

    data = (PNHCmdWindow) GetWindowLong(GetNHApp()->hCmdWnd, GWL_USERDATA);
    if (!data)
        return;

    p = nhcmdset_create();

    ZeroMemory(buf, sizeof(buf));
    if (sscanf(str, "%s or %s", buf[1], buf[0]) != 2) {
        ZeroMemory(buf, sizeof(buf));
        strncpy(buf[0], str, sizeof(buf[0]) - 1);
    }

    lcount = 10; /* create new layout on the first iteration */
    for (i = 0; i < 2; i++) {
        s = buf[i];
        while (*s) {
            char c_start, c_end, c_char;

            /* parse character ranges */
            if (isalnum((c_start = s[0])) && s[1] == '-'
                && isalnum((c_end = s[2]))) {
                s += 2;
            } else {
                c_start = c_end = *s;
            }

            for (c_char = c_start; c_char <= c_end; c_char++) {
                if (lcount >= 10) {
                    /* create layout */
                    lcount = 0;
                    layout_prev = layout_cur;
                    layout_cur = nhcmdlayout_create("noname", NH_CMDPAD_ROWS,
                                                    NH_CMDPAD_COLS);
                    nhcmdlayout_init(layout_cur, cells_layout_menu);

                    nhcmdlayout_cell(layout_cur, 3, 0)->data = layout_prev;
                    nhcmdlayout_cell(layout_cur, 3, 2)->data = 0;

                    nhcmdset_add(p, layout_cur);
                    p->elements[p->count - 1].free_on_destroy = 1;

                    if (layout_prev) {
                        nhcmdlayout_cell(layout_prev, 3, 2)->data =
                            layout_cur;
                    }
                }

                if (lcount == 9)
                    lcount = 10; // skip '#'
                nhcmdlayout_cell_direct(layout_cur, lcount)->f_char[0] =
                    c_char;
                if (c_char == '\033') {
                    strcpy(nhcmdlayout_cell_direct(layout_cur, lcount)->text,
                           "esc");
                    nhcmdlayout_cell_direct(layout_cur, lcount)->image =
                        14; /* 14 is a ESC symbol in IDB_KEYPAD */
                } else {
                    nhcmdlayout_cell_direct(layout_cur, lcount)->text[0] =
                        c_char;
                    nhcmdlayout_cell_direct(layout_cur, lcount)->text[1] =
                        '\x0';
                }

                /* increment character count in the current layout */
                lcount++;
            }

            /* prepareg next charcter from the source string */
            s++;
        }
    }

    /* install the new set */
    if (nhcmdset_current != nhcmdset_default)
        nhcmdset_destroy(nhcmdset_current);
    nhcmdset_current = p;
    SetCmdWindowLayout(GetNHApp()->hCmdWnd,
                       nhcmdset_get(nhcmdset_current, 0));
}
/*-------------------------------------------------------------------------*/
void
NHSPhoneSetKeypadDirection()
{
    PNHCmdWindow data;

    data = (PNHCmdWindow) GetWindowLong(GetNHApp()->hCmdWnd, GWL_USERDATA);
    if (!data)
        return;

    if (nhcmdset_current != nhcmdset_default)
        nhcmdset_destroy(nhcmdset_current);
    nhcmdset_current = nhcmdset_default;
    SetCmdWindowLayout(GetNHApp()->hCmdWnd,
                       nhcmdset_get(nhcmdset_current, NH_LAYOUT_MOVEMENT));
}
/*-------------------------------------------------------------------------*/
void
NHSPhoneSetKeypadDefault()
{
    PNHCmdWindow data;

    data = (PNHCmdWindow) GetWindowLong(GetNHApp()->hCmdWnd, GWL_USERDATA);
    if (!data)
        return;

    if (nhcmdset_current != nhcmdset_default)
        nhcmdset_destroy(nhcmdset_current);
    nhcmdset_current = nhcmdset_default;
    SetCmdWindowLayout(GetNHApp()->hCmdWnd,
                       data->layout_selected
                           ? data->layout_selected
                           : nhcmdset_get(nhcmdset_current, 0));
}

#endif /* defined (WIN_CE_SMARTHPONE) */

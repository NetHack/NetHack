/* NetHack 3.6	winMS.h	$NHDT-Date: 1433806609 2015/06/08 23:36:49 $  $NHDT-Branch: master $:$NHDT-Revision: 1.29 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINMS_H
#define WINMS_H

#pragma warning(disable : 4142) /* benign redefinition of type */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <newres.h>
#include "resource.h"
#include "hack.h"

#if defined(WIN_CE_POCKETPC)
#include <aygshell.h>
#include <sipapi.h>
#endif

#if defined(WIN_CE_SMARTPHONE)
#include <aygshell.h>
#include <sipapi.h>
#include <shlobj.h>
#include <prsht.h>
#include <Tpcshell.h>
#include <windowsm.h>
#include <KEYBD.H>
#endif

#if defined(WIN_CE_PS2xx) || defined(WIN32_PLATFORM_HPCPRO)
#include <sipapi.h>
#endif

/* Taskbar Menu height */
#define MENU_HEIGHT 26

/* Create an array to keep track of the various windows */

#ifndef MAXWINDOWS
#define MAXWINDOWS 15
#endif

/* RIP window ID */
#define NHW_RIP 32
#define NHW_KEYPAD 33

/* size of tiles */
#ifndef TILE_X
#define TILE_X 16
#endif
#define TILE_Y 16

/* tiles per line in the bitmap */
#define TILES_PER_LINE 40

/* tile background color */
#define TILE_BK_COLOR RGB(71, 108, 108)

/* minimum/maximum font size (in points - 1/72 inch) */
#define NHFONT_DEFAULT_SIZE 9
#define NHFONT_STATUS_DEFAULT_SIZE 6
#define NHFONT_SIZE_MIN 3
#define NHFONT_SIZE_MAX 20

typedef struct mswin_nhwindow_data {
    HWND win;
    int type;
    int dead;
} MSNHWinData, *PMSNHWinData;

/* global application data - alailable thour GetNHApp() */
typedef struct mswin_nhwindow_app {
    HINSTANCE hApp;     /* hInstance from WinMain */
    int nCmdShow;       /* main window mode flag */
    HWND hMainWnd;      /* main window handle */
    HACCEL hAccelTable; /* accelerator table */
    HWND hPopupWnd;     /* active dialog window (nethack menu, text, etc) */
    HWND hMenuBar;      /* menu bar */

    MSNHWinData windowlist[MAXWINDOWS]; /* nethack windows array */

    HBITMAP bmpTiles;    /* nethack tiles */
    HBITMAP bmpPetMark;  /* pet mark Bitmap */
    HBITMAP bmpMapTiles; /* alternative map tiles */
    int mapTile_X;       /* alt. tiles width */
    int mapTile_Y;       /* alt. tiles height */
    int mapTilesPerLine; /* number of tile per row in the bitmap */

    boolean bNoHScroll; /* disable cliparound for horizontal grid (map) */
    boolean bNoVScroll; /* disable cliparound for vertical grid (map) */

    int mapDisplayModeSave; /* saved map display mode */

    int bCmdPad;  /* command pad - on-screen keyboard */
    HWND hCmdWnd; /* handle of on-screen keyboard window */

    /* options */
    boolean bWrapText;       /* format text to fit the window */
    boolean bFullScreen;     /* run nethack in full-screen mode  */
    boolean bHideScrollBars; /* hide scroll bars */
    boolean bUseSIP;         /* use SIP (built-in software keyboard) for menus
                                (PocketPC only) */
} NHWinApp, *PNHWinApp;
extern PNHWinApp GetNHApp();

#define E extern

E struct window_procs mswin_procs;

#undef E

/* Some prototypes */
void mswin_init_nhwindows(int *argc, char **argv);
void mswin_player_selection(void);
void mswin_askname(void);
void mswin_get_nh_event(void);
void mswin_exit_nhwindows(const char *);
void mswin_suspend_nhwindows(const char *);
void mswin_resume_nhwindows(void);
winid mswin_create_nhwindow(int type);
void mswin_clear_nhwindow(winid wid);
void mswin_display_nhwindow(winid wid, BOOLEAN_P block);
void mswin_destroy_nhwindow(winid wid);
void mswin_curs(winid wid, int x, int y);
void mswin_putstr(winid wid, int attr, const char *text);
void mswin_putstr_ex(winid wid, int attr, const char *text, boolean append);
void mswin_display_file(const char *filename, BOOLEAN_P must_exist);
void mswin_start_menu(winid wid);
void mswin_add_menu(winid wid, int glyph, const ANY_P *identifier,
                    CHAR_P accelerator, CHAR_P group_accel, int attr,
                    const char *str, BOOLEAN_P presel);
void mswin_end_menu(winid wid, const char *prompt);
int mswin_select_menu(winid wid, int how, MENU_ITEM_P **selected);
void mswin_update_inventory(void);
void mswin_mark_synch(void);
void mswin_wait_synch(void);
void mswin_cliparound(int x, int y);
void mswin_print_glyph(winid wid, XCHAR_P x, XCHAR_P y, int glyph, int bkglyph);
void mswin_raw_print(const char *str);
void mswin_raw_print_bold(const char *str);
int mswin_nhgetch(void);
int mswin_nh_poskey(int *x, int *y, int *mod);
void mswin_nhbell(void);
int mswin_doprev_message(void);
char mswin_yn_function(const char *question, const char *choices, CHAR_P def);
void mswin_getlin(const char *question, char *input);
int mswin_get_ext_cmd(void);
void mswin_number_pad(int state);
void mswin_delay_output(void);
void mswin_change_color(void);
char *mswin_get_color_string(void);
void mswin_start_screen(void);
void mswin_end_screen(void);
void mswin_outrip(winid wid, int how, time_t when);
void mswin_preference_update(const char *pref);

/* helper function */
HWND mswin_hwnd_from_winid(winid wid);
winid mswin_winid_from_type(int type);
winid mswin_winid_from_handle(HWND hWnd);
void mswin_window_mark_dead(winid wid);
void bail(const char *mesg);
void nhapply_image_transparent(HDC hDC, int x, int y, int width, int height,
                               HDC sourceDC, int s_x, int s_y, int s_width,
                               int s_height, COLORREF cTransparent);
void mswin_popup_display(HWND popup, int *done_indicator);
void mswin_popup_destroy(HWND popup);

#if defined(WIN_CE_SMARTPHONE)
void NHSPhoneDialogSetup(HWND hDlg, UINT nToolBarId, BOOL is_edit,
                         BOOL is_fullscreen);
#endif

void mswin_read_reg(void);
void mswin_destroy_reg(void);
void mswin_write_reg(void);

BOOL mswin_has_keyboard(void);

void mswin_set_fullscreen(BOOL is_fullscreen);

extern winid WIN_STATUS;

#endif /* WINmswin_H */

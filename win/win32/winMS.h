/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINMS_H
#define WINMS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "hack.h"

/* Create an array to keep track of the various windows */

#ifndef MAXWINDOWS
#define MAXWINDOWS 15
#endif

#define NHW_RIP  32

#ifndef TILE_X
#define TILE_X 16
#endif
#define TILE_Y 16

#define TILES_PER_LINE  40

/* tile background color */
#define TILE_BK_COLOR RGB(71, 108, 108)

/* minimum/maximum font size (in points - 1/72 inch) */
#define NHFONT_DEFAULT_SIZE 9
#define NHFONT_SIZE_MIN 3
#define NHFONT_SIZE_MAX 20

#define MAX_LOADSTRING 100

typedef struct mswin_nhwindow_data {
  HWND	      win;
  int		  type;
  int		  dead;
} MSNHWinData, *PMSNHWinData;

typedef struct mswin_nhwindow_app {
	HINSTANCE   hApp;
	HWND		hMainWnd;
	HACCEL		hAccelTable;
	HWND		hPopupWnd;	/* current popup window  */

	MSNHWinData windowlist[MAXWINDOWS];

	HBITMAP		bmpTiles;
	HBITMAP		bmpPetMark;
	HBITMAP		bmpMapTiles; /* custom tiles bitmap */
	HBITMAP		bmpRip;
	HBITMAP		bmpSplash;
	int			mapTile_X;	/* tile width */
	int			mapTile_Y;	/* tile height */
	int			mapTilesPerLine;	/* number of tile per row in the bitmap */

	boolean		bNoHScroll;	/* disable cliparound for horizontal grid (map) */
	boolean		bNoVScroll; /* disable cliparound for vertical grid (map) */

	int			mapDisplayModeSave;	/* saved map display mode */

	char*		saved_text;

    DWORD       saveRegistrySettings; /* Flag if we should save this time */
    DWORD       regNetHackMode;   /* NetHack mode means no Windows keys in some places */
} NHWinApp, *PNHWinApp;

#define E extern

E PNHWinApp GetNHApp(void);
E struct window_procs mswin_procs;

#undef E

/* Some prototypes */
void mswin_init_nhwindows(int* argc, char** argv);
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
void mswin_display_file(const char *filename,BOOLEAN_P must_exist);
void mswin_start_menu(winid wid);
void mswin_add_menu(winid wid, int glyph, const ANY_P * identifier,
		CHAR_P accelerator, CHAR_P group_accel, int attr, 
		const char *str, BOOLEAN_P presel);
void mswin_end_menu(winid wid, const char *prompt);
int  mswin_select_menu(winid wid, int how, MENU_ITEM_P **selected);
void mswin_update_inventory(void);
void mswin_mark_synch(void);
void mswin_wait_synch(void);
void mswin_cliparound(int x, int y);
void mswin_print_glyph(winid wid,XCHAR_P x,XCHAR_P y,int glyph);
void mswin_raw_print(const char *str);
void mswin_raw_print_bold(const char *str);
int  mswin_nhgetch(void);
int  mswin_nh_poskey(int *x, int *y, int *mod);
void mswin_nhbell(void);
int  mswin_doprev_message(void);
char mswin_yn_function(const char *question, const char *choices,
		CHAR_P def);
void mswin_getlin(const char *question, char *input);
int  mswin_get_ext_cmd(void);
void mswin_number_pad(int state);
void mswin_delay_output(void);
void mswin_change_color(void);
char *mswin_get_color_string(void);
void mswin_start_screen(void);
void mswin_end_screen(void);
void mswin_outrip(winid wid, int how);
void mswin_preference_update(const char *pref);

/* helper function */
HWND mswin_hwnd_from_winid(winid wid);
winid mswin_winid_from_type(int type);
winid mswin_winid_from_handle(HWND hWnd);
void mswin_window_mark_dead(winid wid);
void bail(const char *mesg);
void nhapply_image_transparent( 
	HDC hDC, int x, int y, int width, int height,
	HDC sourceDC, int s_x, int s_y, int s_width, int s_height,
	COLORREF cTransparent
);

void mswin_read_reg(void);
void mswin_destroy_reg(void);
void mswin_write_reg(void);

extern HBRUSH menu_bg_brush;
extern HBRUSH menu_fg_brush;
extern HBRUSH text_bg_brush;
extern HBRUSH text_fg_brush;
extern HBRUSH status_bg_brush;
extern HBRUSH status_fg_brush;
extern HBRUSH message_bg_brush;
extern HBRUSH message_fg_brush;

extern COLORREF menu_bg_color;
extern COLORREF menu_fg_color;
extern COLORREF text_bg_color;
extern COLORREF text_fg_color;
extern COLORREF status_bg_color;
extern COLORREF status_fg_color;
extern COLORREF message_bg_color;
extern COLORREF message_fg_color;


#define SYSCLR_TO_BRUSH(x) ((HBRUSH)((x) + 1))

/* unicode stuff */
#ifdef UNICODE
	#define NH_W2A(w, a, cb)     ( WideCharToMultiByte(                              \
												   CP_ACP,                      \
												   0,                           \
												   (w),                           \
												   -1,                          \
												   (a),                           \
												   (cb),                          \
												   NULL,                        \
												   NULL), (a) )

	#define NH_A2W(a, w, cb)     ( MultiByteToWideChar(                              \
												   CP_ACP,                      \
												   0,                           \
												   (a),                           \
												   -1,                          \
												   (w),                           \
												   (cb)), (w) )
#else
	#define NH_W2A(w, a, cb)     (strncpy((a), (w), (cb)))

	#define NH_A2W(a, w, cb)     (strncpy((w), (a), (cb)))
#endif

#endif /* WINmswin_H */

/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "resource.h"
#include "mhcmd.h"
#include "mhinput.h"
#include "mhcolor.h"

static TCHAR szNHCmdWindowClass[] = TEXT("MSNethackCmdWndClass");

#ifndef C
#  define C(c)		(0x1f & (c))
#endif

/* cell status 0 */
#define NH_CST_CHECKED		1

/* fonts */
#define NH_CMDPAD_FONT_NORMAL    0
#define NH_CMDPAD_FONT_MAX		 0
#define NHCMD_DEFAULT_FONT_NAME TEXT("Tahoma")

/* type of the cell */
#define NH_CELL_REG		0
#define NH_CELL_CTRL	1
#define NH_CELL_CAP		2
#define NH_CELL_SHIFT	3

/* dimensions of the command pad */
#define NH_CMDPAD_ROWS 3
#define NH_CMDPAD_COLS 19
#define NH_CMDPAD_CELLNUM (NH_CMDPAD_COLS*NH_CMDPAD_ROWS)

/* cell information */
typedef struct t_NHCmdPadCell {
	UINT		cmd_code;	/* Windows command code (menu processing - not implemented - set to -1) */
	char		f_char;		/* nethack char */
	const char* text;		/* display text */
	int			image;		/* >0 - image ID - not implented 
	                           <=0 - absolute index of the font table */
	int			type;		/* cell type */
	int			mult;		/* cell width multiplier */
} NHCmdPadCell, *PNHCmdPadCell;

/* lowercase layout */
NHCmdPadCell layout_mod1[NH_CMDPAD_ROWS*NH_CMDPAD_COLS] = 
{
	{ -1, '7', "7", 1, NH_CELL_REG, 1 },
	{ -1, '8', "8", 2, NH_CELL_REG, 1 },
	{ -1, '9', "9", 3, NH_CELL_REG, 1 },
	{ -1, '<', "<", 10, NH_CELL_REG, 1 },
	{ -1, ' ', "CAP", -NH_CMDPAD_FONT_NORMAL, NH_CELL_CAP, 2 },
		{ -1, ' ', "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0 }, /* complement for CAPS */
	{ -1, '\x1b', "Esc", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 2 },
		{ -1, ' ', "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0 }, /* complement for ESC */
	{ -1, '?', "?", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '*', "*", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, ',', ",", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '/', "/", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, ':', ":", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, ';', ";", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '-', "-", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '#', "#", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '^', "^", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '&', "&", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '\\', "\\", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },

	{ -1, '4', "4", 4, NH_CELL_REG, 1 },
	{ -1, '5', "5", 5, NH_CELL_REG, 1 },
	{ -1, '6', "6", 6, NH_CELL_REG, 1 },
	{ -1, '.', ".", 11, NH_CELL_REG, 1 },
	{ -1, ' ', "Shft", -NH_CMDPAD_FONT_NORMAL, NH_CELL_SHIFT, 2 },
		{ -1, ' ', "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0 }, /* complement for shift */
	{ -1, 'a', "a", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'b', "b", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'c', "c", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'd', "d", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'e', "e", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'f', "f", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'g', "g", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'h', "h", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'i', "i", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'j', "j", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'k', "k", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'l', "l", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'm', "m", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },

	{ -1, '1', "1", 7, NH_CELL_REG, 1 },
	{ -1, '2', "2", 8, NH_CELL_REG, 1 },
	{ -1, '3', "3", 9, NH_CELL_REG, 1 },
	{ -1, '>', ">", 12, NH_CELL_REG, 1 },
	{ -1, ' ', "Ctrl", -NH_CMDPAD_FONT_NORMAL, NH_CELL_CTRL, 2 },
		{ -1, ' ', "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0 }, /* complement for CTRL */
	{ -1, 'n', "n", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'o', "o", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'p', "p", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'q', "q", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'r', "r", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 's', "s", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 't', "t", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'u', "u", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'v', "v", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'w', "w", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'x', "x", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'y', "y", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'z', "z", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 }
};

/* uppercase layout */
NHCmdPadCell layout_mod2[-NH_CMDPAD_ROWS*-NH_CMDPAD_COLS] = 
{ 
	{ -1, '7', "7", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '8', "8", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '9', "9", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '<', "<", 10, NH_CELL_REG, 1 },
	{ -1, ' ', "CAP", -NH_CMDPAD_FONT_NORMAL, NH_CELL_CAP, 2 },
		{ -1, ' ', "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0 }, /* complement for CAPS */
	{ -1, '\x1b', "Esc", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 2 },
		{ -1, ' ', "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0 }, /* complement for ESC */
	{ -1, '?', "?", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '*', "*", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '[', "[", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, ']', "]", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '(', "(", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, ')', ")", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '+', "+", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '=', "=", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '"', "\"", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '$', "$", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '@', "@", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },

	{ -1, '4', "4", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '5', "5", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '6', "6", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '0', "0", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, ' ', "Shft", -NH_CMDPAD_FONT_NORMAL, NH_CELL_SHIFT, 2 },
		{ -1, ' ', "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0 }, /* complement for shift */
	{ -1, 'A', "A", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'B', "B", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'C', "C", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'D', "D", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'E', "E", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'F', "F", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'G', "G", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'H', "H", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'I', "I", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'J', "J", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'K', "K", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'L', "L", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'M', "M", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },

	{ -1, '1', "1", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '2', "2", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '3', "3", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, '>', ">", 12, NH_CELL_REG, 1 },
	{ -1, ' ', "Ctrl", -NH_CMDPAD_FONT_NORMAL, NH_CELL_CTRL, 2 },
		{ -1, ' ', "", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 0 }, /* complement for CTRL */
	{ -1, 'N', "N", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'O', "O", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'P', "P", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'Q', "Q", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'R', "R", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'S', "S", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'T', "T", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'U', "U", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'V', "V", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'W', "W", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'X', "X", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'Y', "Y", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 },
	{ -1, 'Z', "Z", -NH_CMDPAD_FONT_NORMAL, NH_CELL_REG, 1 }
};

/* display cell layout */
typedef struct t_NHCmdPadLayoutCell {
	POINT   orig;		/* origin of the cell rect */
	BYTE	type;		/* cell type */
	int		state;		/* cell state */
} NHCmdPadLayoutCell, *PNHCmdPadLayoutCell;

/* command window data */
typedef struct mswin_nethack_cmd_window {
	NHCmdPadLayoutCell	cells[NH_CMDPAD_CELLNUM];	/* display cells */
	SIZE				cell_size;					/* cell size */
	PNHCmdPadCell		layout;						/* current layout */
	HFONT				font[NH_CMDPAD_FONT_MAX+1];	/* fonts for cell text */
	HBITMAP				images;						/* key images map */
	int					active_cell;				/* current active cell */

	boolean				is_caps;					/* is CAPS selected */
	boolean				is_ctrl;					/* is CRTL selected */
	boolean				is_shift;					/* is SHIFT selected */
} NHCmdWindow, *PNHCmdWindow;

LRESULT CALLBACK NHCommandWndProc(HWND, UINT, WPARAM, LPARAM);
static void register_command_window_class();
static void LayoutCmdWindow(HWND hWnd);
static void SetCmdWindowLayout(HWND hWnd, PNHCmdPadCell layout);
static int CellFromPoint(PNHCmdWindow data, POINT pt );
static void CalculateCellSize(HWND hWnd, LPSIZE pSize);
static void SelectCell(HWND hWnd, int cell, BOOL isSelected);

/* message handlers */
static void onPaint(HWND hWnd);				// on WM_PAINT
static void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);	// on WM_CREATE
static void onMouseDown(HWND hWnd, WPARAM wParam, LPARAM lParam);	// on WM_LBUTTONDOWN
static void onMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam);	// on WM_MOUSEMOVE
static void onMouseUp(HWND hWnd, WPARAM wParam, LPARAM lParam);		// on WM_LBUTTONUP


HWND mswin_init_command_window () {
	static int run_once = 0;
	HWND ret;

	/* register window class */
	if( !run_once ) {
		register_command_window_class();
		run_once = 1;
	}
	
	/* create window */
	ret = CreateWindow(
			szNHCmdWindowClass,		/* registered class name */
			NULL,					/* window name */
			WS_CHILD | WS_CLIPSIBLINGS, /* window style */
			0,  /* horizontal position of window - set it later */
			0,  /* vertical position of window - set it later */
			0,  /* window width - set it later */
			0,  /* window height - set it later*/
			GetNHApp()->hMainWnd,	/* handle to parent or owner window */
			NULL,					/* menu handle or child identifier */
			GetNHApp()->hApp,		/* handle to application instance */
			NULL );					/* window-creation data */
	if( !ret ) {
		panic("Cannot create command window");
	}
	return ret;
}

/* calculate mimimum window size */
void mswin_command_window_size (HWND hwnd, LPSIZE sz)
{
	RECT rt;
	SIZE cell_size;
	
	CalculateCellSize(hwnd, &cell_size);
	GetWindowRect(GetNHApp()->hMainWnd, &rt);
	sz->cx = rt.right - rt.left;
	sz->cy = cell_size.cy*NH_CMDPAD_ROWS+2*GetSystemMetrics(SM_CYBORDER);
}

void register_command_window_class()
{
	WNDCLASS wcex;
	ZeroMemory( &wcex, sizeof(wcex));

	/* window class */
	wcex.style			= CS_NOCLOSE;
	wcex.lpfnWndProc	= (WNDPROC)NHCommandWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= GetNHApp()->hApp;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= mswin_get_brush(NHW_KEYPAD, MSWIN_COLOR_BG);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szNHCmdWindowClass;

	if( !RegisterClass(&wcex) ) {
		panic("cannot register Map window class");
	}
}

LRESULT CALLBACK NHCommandWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PNHCmdWindow data;
	int i;

	switch (message) 
	{
	case WM_CREATE:
		onCreate( hWnd, wParam, lParam );
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
		if( GetCapture()==hWnd ) { 
			onMouseMove(hWnd, wParam, lParam);
			return 0;
		} else {
			return 1;
		}
		break;
		
	case WM_LBUTTONUP:
		/* proceed only if if have mouse focus (set in onMouseDown()) */
		if( GetCapture()==hWnd ) { 
			onMouseUp(hWnd, wParam, lParam);
			return 0;
		} else {
			return 1;
		}
		break;

	case WM_DESTROY:
		data = (PNHCmdWindow)GetWindowLong(hWnd, GWL_USERDATA);
		for(i=0; i<=NH_CMDPAD_FONT_MAX; i++ )
			if( data->font[i] ) DeleteObject(data->font[i]);
		free(data);
		SetWindowLong(hWnd, GWL_USERDATA, (LONG)0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return FALSE;
}

void onPaint(HWND hWnd)
{
	PNHCmdWindow data;
	PAINTSTRUCT ps;
	HDC hDC;
	int i;
	TCHAR wbuf[BUFSZ];
	HGDIOBJ saveFont;
	BITMAP	bm;

	/* get window data */
	data = (PNHCmdWindow)GetWindowLong(hWnd, GWL_USERDATA);

	hDC = BeginPaint(hWnd, &ps);

	if( !IsRectEmpty(&ps.rcPaint) ) {
		HGDIOBJ oldBr;
		HBRUSH hbrPattern;
		COLORREF OldBg, OldFg;
		HPEN hPen;
		HGDIOBJ hOldPen;

		saveFont = SelectObject(hDC, data->font[NH_CMDPAD_FONT_NORMAL]);
		OldBg = SetBkColor(hDC, mswin_get_color(NHW_KEYPAD, MSWIN_COLOR_BG));
		OldFg = SetTextColor(hDC, mswin_get_color(NHW_KEYPAD, MSWIN_COLOR_FG)); 

		GetObject(data->images, sizeof(BITMAP), (LPVOID)&bm);

		hbrPattern = CreatePatternBrush(data->images);
		hPen = CreatePen(PS_SOLID, 1, mswin_get_color(NHW_KEYPAD, MSWIN_COLOR_FG));

		for( i=0; i<NH_CMDPAD_CELLNUM; i++ ) {
			RECT cell_rt;
			POINT pt[5];

			/* calculate the cell rectangle */
			cell_rt.left = data->cells[i].orig.x;
			cell_rt.top = data->cells[i].orig.y;
			cell_rt.right = data->cells[i].orig.x + data->cell_size.cx*data->layout[i].mult;
			cell_rt.bottom = data->cells[i].orig.y + data->cell_size.cy;

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
			cell_rt.top ++;
			cell_rt.right--;
			cell_rt.bottom--;

			/* draw the cell text */
			if( data->layout[i].image<=0 ) {
				SelectObject(hDC, data->font[ -data->layout[i].image ]);
				DrawText(hDC, 
						NH_A2W(data->layout[i].text, wbuf, BUFSZ), 
						strlen(data->layout[i].text),
						&cell_rt,
						DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX
						);
			} else {
				/* draw bitmap */
				int bmOffset;
				RECT bitmap_rt;

				bmOffset = (data->layout[i].image - 1)*bm.bmHeight;

				bitmap_rt.left = ((cell_rt.left+cell_rt.right) - min(bm.bmHeight, (cell_rt.right-cell_rt.left)))/2;
				bitmap_rt.top = ((cell_rt.bottom+cell_rt.top) - min(bm.bmHeight, (cell_rt.bottom-cell_rt.top)))/2;
				bitmap_rt.right = bitmap_rt.left + min(bm.bmHeight, (cell_rt.right-cell_rt.left));
				bitmap_rt.bottom = bitmap_rt.top + min(bm.bmHeight, (cell_rt.bottom-cell_rt.top));

				SetBrushOrgEx(hDC, bitmap_rt.left-bmOffset, bitmap_rt.top, NULL);
				oldBr = SelectObject(hDC, hbrPattern);
				PatBlt( 
					hDC, 
					bitmap_rt.left, 
					bitmap_rt.top, 
					bitmap_rt.right-bitmap_rt.left, 
					bitmap_rt.bottom-bitmap_rt.top,
					PATCOPY);
				SelectObject(hDC, oldBr);
			}

			/* invert the cell if it is selected */
			if( data->cells[i].state == NH_CST_CHECKED ) {
				IntersectRect( &cell_rt, &cell_rt, &ps.rcPaint);
				PatBlt( hDC, 
						cell_rt.left,
						cell_rt.top,
						cell_rt.right - cell_rt.left,
						cell_rt.bottom - cell_rt.top,
						DSTINVERT
						);
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

void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHCmdWindow data;

	/* set window data */
	data = (PNHCmdWindow)malloc(sizeof(NHCmdWindow));
	if( !data ) panic("out of memory");

	ZeroMemory(data, sizeof(NHCmdWindow));
	SetWindowLong(hWnd, GWL_USERDATA, (LONG)data);

	data->active_cell = -1;

	/* load images bitmap */
	data->images = LoadBitmap(GetNHApp()->hApp, MAKEINTRESOURCE(IDB_KEYPAD));
	if( !data->images ) panic("cannot load keypad bitmap");

	/* set default layout */
	SetCmdWindowLayout(hWnd, layout_mod1);
}

void LayoutCmdWindow(HWND hWnd)
{
	RECT clrt;
	PNHCmdWindow data;
	int i, j;
	int x, y;
	LOGFONT lgfnt;

	GetClientRect(hWnd, &clrt);
	if( IsRectEmpty(&clrt) ) return;

	data = (PNHCmdWindow)GetWindowLong(hWnd, GWL_USERDATA);
	if( !data->layout ) return;

	/* calculate cell size */
	CalculateCellSize(hWnd, &data->cell_size);
	 
	/* initialize display cells aray */
	x = 0;
	y = 0;
	for( i=0; i<NH_CMDPAD_ROWS; i++ ) {
		for( j=0; j<NH_CMDPAD_COLS; j++ ) {
			int index;

			index = (i*NH_CMDPAD_COLS)+j;
			data->cells[index].orig.x = x;
			data->cells[index].orig.y = y;
			data->cells[index].type = data->layout[index].type;

			switch(data->cells[index].type) {
			case NH_CELL_CTRL:
				data->cells[index].state = data->is_ctrl? NH_CST_CHECKED : 0;
				break;
			case NH_CELL_CAP:
				data->cells[index].state = data->is_caps? NH_CST_CHECKED : 0;
				break;
			case NH_CELL_SHIFT:
				data->cells[index].state = data->is_shift? NH_CST_CHECKED : 0;
				break;
			default:
				data->cells[index].state = 0;
			}

			x += data->cell_size.cx * data->layout[index].mult;
		}
		x = 0;
		y += data->cell_size.cy;
	}

	/* create font for display cell text */
	for(i=0; i<=NH_CMDPAD_FONT_MAX; i++ )
		if( data->font[i] ) DeleteObject(data->font[i]);

	ZeroMemory( &lgfnt, sizeof(lgfnt) );
	lgfnt.lfHeight			=	data->cell_size.cy; // height of font
	lgfnt.lfWidth			=	0; // average character width
	lgfnt.lfEscapement		=	0;					 // angle of escapement
	lgfnt.lfOrientation		=	0;					 // base-line orientation angle
	lgfnt.lfWeight			=	FW_NORMAL;			// font weight
	lgfnt.lfItalic			=	FALSE;			     // italic attribute option
	lgfnt.lfUnderline		=	FALSE;				// underline attribute option
	lgfnt.lfStrikeOut		=	FALSE;				// strikeout attribute option
	lgfnt.lfCharSet			=	ANSI_CHARSET;     // character set identifier
	lgfnt.lfOutPrecision	=	OUT_DEFAULT_PRECIS;  // output precision
	lgfnt.lfClipPrecision	=	CLIP_CHARACTER_PRECIS; // clipping precision
	lgfnt.lfQuality			=	DEFAULT_QUALITY;     // output quality
	lgfnt.lfPitchAndFamily	=	VARIABLE_PITCH;		 // pitch and family
	_tcsncpy(lgfnt.lfFaceName, NHCMD_DEFAULT_FONT_NAME, LF_FACESIZE);
	data->font[NH_CMDPAD_FONT_NORMAL] = CreateFontIndirect(&lgfnt);
}


void SetCmdWindowLayout(HWND hWnd, PNHCmdPadCell layout)
{
	PNHCmdWindow data;

	data = (PNHCmdWindow)GetWindowLong(hWnd, GWL_USERDATA);
	data->layout = layout;
	ZeroMemory(data->cells, sizeof(data->cells));
	LayoutCmdWindow(hWnd);
}

void onMouseDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHCmdWindow data;
	POINT mpt;

	/* get mouse coordinates */
	mpt.x = LOWORD(lParam);
	mpt.y = HIWORD(lParam);

	/* map mouse coordinates to the display cell */
	data = (PNHCmdWindow)GetWindowLong(hWnd, GWL_USERDATA);
	data->active_cell = CellFromPoint(data, mpt);
	if( data->active_cell==-1 ) return;

	/* set mouse focus to the current window */
	SetCapture(hWnd);

	/* invert the selection */
	SelectCell(hWnd, data->active_cell, (data->cells[data->active_cell].state!=NH_CST_CHECKED) );
}

void onMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHCmdWindow data;
	POINT mpt;
	int newActiveCell;

	/* get mouse coordinates */
	mpt.x = LOWORD(lParam);
	mpt.y = HIWORD(lParam);

	/* map mouse coordinates to the display cell */
	data = (PNHCmdWindow)GetWindowLong(hWnd, GWL_USERDATA);
	newActiveCell = CellFromPoint(data, mpt);
	if( data->active_cell == -1 ) return;

	/* if mouse is within orginal display cell - select the cell otherwise
	   clear the selection */
	switch( data->layout[data->active_cell].type ) {
	case NH_CELL_REG: 
		SelectCell(hWnd, data->active_cell, 
			       (newActiveCell==data->active_cell) );
		break;

	case NH_CELL_CTRL:
		SelectCell(hWnd, data->active_cell, 
			((newActiveCell==data->active_cell)? !data->is_ctrl : data->is_ctrl) );
		break;

	case NH_CELL_CAP:
		SelectCell(hWnd, data->active_cell, 
			((newActiveCell==data->active_cell)? !data->is_caps : data->is_caps) );
		break;
	}
}

void onMouseUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHCmdWindow data;
	int i;

	/* release mouse capture */
	ReleaseCapture();

	/* get active display cell */
	data = (PNHCmdWindow)GetWindowLong(hWnd, GWL_USERDATA);
	if( data->active_cell == -1 ) return;

	/* act depending on the cell type:
		CAPS - change layout 
		CTRL - modify CTRL status
		REG  - place keyboard event on the nethack input queue
	*/
	switch( data->layout[data->active_cell].type ) {
	case NH_CELL_REG: 
		if( data->is_ctrl ) {
			NHEVENT_KBD(C(data->layout[data->active_cell].f_char));

			data->is_ctrl = 0;
			for( i=0; i<NH_CMDPAD_CELLNUM; i++ ) {
				if( data->layout[i].type == NH_CELL_CTRL ) {
					SelectCell(hWnd, i, data->is_ctrl);
				}
			}
		} else {
			NHEVENT_KBD(data->layout[data->active_cell].f_char);
		}
		SelectCell(hWnd, data->active_cell, FALSE);

		if( !data->is_shift ) break;
		// else fall through and reset the shift

	case NH_CELL_SHIFT:
		data->is_shift = !data->is_shift;
		SetCmdWindowLayout(
			hWnd,
			(data->is_shift ^ data->is_caps)? layout_mod2 : layout_mod1
		);
		data->cells[data->active_cell].state = data->is_shift? NH_CST_CHECKED : 0;
		InvalidateRect(hWnd, NULL, TRUE);
		break;

	case NH_CELL_CTRL:
		data->is_ctrl = !data->is_ctrl;
		SelectCell(hWnd, data->active_cell, data->is_ctrl);
		break;

	case NH_CELL_CAP:
		data->is_caps = !data->is_caps;
		SetCmdWindowLayout(
			hWnd,
			(data->is_shift ^ data->is_caps)? layout_mod2 : layout_mod1
		);
		data->cells[data->active_cell].state = data->is_caps? NH_CST_CHECKED : 0;
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	}

	data->active_cell = -1;
}

int CellFromPoint(PNHCmdWindow data, POINT pt )
{
	int i;
	for( i=0; i<NH_CMDPAD_CELLNUM; i++ ) {
		RECT cell_rt;
		cell_rt.left = data->cells[i].orig.x;
		cell_rt.top = data->cells[i].orig.y;
		cell_rt.right = data->cells[i].orig.x + data->cell_size.cx*data->layout[i].mult;
		cell_rt.bottom = data->cells[i].orig.y + data->cell_size.cy;
		if( PtInRect(&cell_rt, pt) ) return i;
	}
	return -1;
}

void CalculateCellSize(HWND hWnd, LPSIZE pSize)
{
	HDC hdc;
	RECT clrt;

	GetClientRect(hWnd, &clrt);
	
	hdc = GetDC(hWnd);
	pSize->cx = (clrt.right - clrt.left)/NH_CMDPAD_COLS;
	pSize->cy = max(pSize->cx, 10*GetDeviceCaps(hdc, LOGPIXELSY)/72);
	ReleaseDC(hWnd, hdc);
}

void SelectCell(HWND hWnd, int cell, BOOL isSelected)
{
	HDC hDC;
	PNHCmdWindow data;
	int prevState;

	data = (PNHCmdWindow)GetWindowLong(hWnd, GWL_USERDATA);
	prevState = data->cells[cell].state;
	data->cells[cell].state = (isSelected)? NH_CST_CHECKED : 0;

	if( prevState!=data->cells[cell].state ) {
		hDC = GetDC(hWnd);
		PatBlt( hDC, 
			data->cells[cell].orig.x+1,
			data->cells[cell].orig.y+1,
			data->cell_size.cx*data->layout[cell].mult - 2,
			data->cell_size.cy - 2,
			DSTINVERT
			);
		ReleaseDC(hWnd, hDC);
	}
}
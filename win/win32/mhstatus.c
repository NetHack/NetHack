/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "mhstatus.h"
#include "mhmsg.h"
#include "mhfont.h"

#define NHSW_LINES    2
#define MAXWINDOWTEXT BUFSZ

typedef struct mswin_nethack_status_window {
	int   index;
	char  window_text[NHSW_LINES][MAXWINDOWTEXT+1];
} NHStatusWindow, *PNHStatusWindow;

static TCHAR szStatusWindowClass[] = TEXT("MSNHStatusWndClass");
LRESULT CALLBACK	StatusWndProc(HWND, UINT, WPARAM, LPARAM);
static void register_status_window_class(void);

#define DEFAULT_COLOR_BG_STATUS	COLOR_WINDOW
#define DEFAULT_COLOR_FG_STATUS	COLOR_WINDOWTEXT

HWND mswin_init_status_window () {
	static int run_once = 0;
	HWND ret;
	NHStatusWindow* data;

	if( !run_once ) {
		register_status_window_class( );
		run_once = 1;
	}
	
	ret = CreateWindow(                                
			szStatusWindowClass,
			NULL,
			WS_CHILD | WS_DISABLED | WS_CLIPSIBLINGS,
			0,  /* x position */
			0,  /* y position */
			0,  /* x-size - we will set it later */
			0,  /* y-size - we will set it later */
			GetNHApp()->hMainWnd,
			NULL,
			GetNHApp()->hApp,
			NULL );
	if( !ret ) panic("Cannot create status window");
	
	EnableWindow(ret, FALSE);

	data = (PNHStatusWindow)malloc(sizeof(NHStatusWindow));
	if( !data ) panic("out of memory");

	ZeroMemory(data, sizeof(NHStatusWindow));
	SetWindowLong(ret, GWL_USERDATA, (LONG)data);
	return ret;
}

void register_status_window_class()
{
	WNDCLASS wcex;
	ZeroMemory( &wcex, sizeof(wcex));

	wcex.style			= CS_NOCLOSE;
	wcex.lpfnWndProc	= (WNDPROC)StatusWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= GetNHApp()->hApp;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= status_bg_brush 
		? status_bg_brush : SYSCLR_TO_BRUSH(DEFAULT_COLOR_BG_STATUS);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szStatusWindowClass;

	RegisterClass(&wcex);
}
    
    
LRESULT CALLBACK StatusWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rt;
	PAINTSTRUCT ps;
	HDC hdc;
	PNHStatusWindow data;
	
	data = (PNHStatusWindow)GetWindowLong(hWnd, GWL_USERDATA);
	switch (message) 
	{
	case WM_MSNH_COMMAND: {
		switch( wParam ) {
		case MSNH_MSG_PUTSTR: {
			PMSNHMsgPutstr msg_data = (PMSNHMsgPutstr)lParam;
			strncpy(data->window_text[data->index], msg_data->text, 
				    MAXWINDOWTEXT);
			strncat(data->window_text[data->index], "\n", 
				    MAXWINDOWTEXT-strlen(data->window_text[data->index]));
			data->index = (data->index+1) % NHSW_LINES;
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		case MSNH_MSG_CLEAR_WINDOW:
			data->index = 0;
			ZeroMemory(data->window_text, sizeof(data->window_text));
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
	} break;

	case WM_PAINT: {
		    int i;
			SIZE sz;
			HGDIOBJ oldFont;
			TCHAR wbuf[BUFSZ];
			COLORREF OldBg, OldFg;

			hdc = BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &rt);
			
			oldFont = SelectObject(hdc, mswin_get_font(NHW_STATUS, ATR_NONE, hdc, FALSE));

			OldBg = SetBkColor(hdc, status_bg_brush 
				? status_bg_color : (COLORREF)GetSysColor(DEFAULT_COLOR_BG_STATUS));
			OldFg = SetTextColor(hdc, status_fg_brush 
				? status_fg_color : (COLORREF)GetSysColor(DEFAULT_COLOR_FG_STATUS));
			
			for(i=0; i<NHSW_LINES; i++ ) {
				GetTextExtentPoint32(hdc, NH_A2W(data->window_text[i], wbuf, sizeof(wbuf)), strlen(data->window_text[i]), &sz);
				NH_A2W(data->window_text[i], wbuf, BUFSZ);
				DrawText(hdc, wbuf, strlen(data->window_text[i]), &rt, DT_LEFT | DT_END_ELLIPSIS);
				rt.top += sz.cy;
			}

			SelectObject(hdc, oldFont);
			SetTextColor (hdc, OldFg);
			SetBkColor (hdc, OldBg);
			EndPaint(hWnd, &ps);
		} break;

	case WM_DESTROY:
		free(data);
		SetWindowLong(hWnd, GWL_USERDATA, (LONG)0);
		break;

	case WM_SETFOCUS:
		SetFocus(GetNHApp()->hMainWnd);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

void mswin_status_window_size (HWND hWnd, LPSIZE sz)
{
    TEXTMETRIC tm;
	HGDIOBJ saveFont;
	HDC hdc;
	PNHStatusWindow data;
	RECT rt;
	GetWindowRect(hWnd, &rt);
	sz->cx = rt.right - rt.left;
	sz->cy = rt.bottom - rt.top;

	data = (PNHStatusWindow)GetWindowLong(hWnd, GWL_USERDATA);
	if(data) {
		hdc = GetDC(hWnd);
		saveFont = SelectObject(hdc, mswin_get_font(NHW_STATUS, ATR_NONE, hdc, FALSE));
		GetTextMetrics(hdc, &tm);

		sz->cy = tm.tmHeight * NHSW_LINES;

		SelectObject(hdc, saveFont);
		ReleaseDC(hWnd, hdc);
	}
}

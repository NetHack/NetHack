/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "resource.h"
#include "mhtext.h"
#include "mhmsg.h"
#include "mhfont.h"
#include "mhcolor.h"

typedef struct mswin_nethack_text_window {
	TCHAR*  window_text;
} NHTextWindow, *PNHTextWindow;

LRESULT CALLBACK	TextWndProc(HWND, UINT, WPARAM, LPARAM);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void LayoutText(HWND hwnd);

HWND mswin_init_text_window () {
	HWND ret;
	PNHTextWindow data;

	ret = CreateDialog(
			GetNHApp()->hApp,
			MAKEINTRESOURCE(IDD_NHTEXT),
			GetNHApp()->hMainWnd,
			TextWndProc
	);
	if( !ret ) panic("Cannot create text window");

	data = (PNHTextWindow)malloc(sizeof(NHTextWindow));
	if( !data ) panic("out of memory");

	ZeroMemory(data, sizeof(NHTextWindow));
	SetWindowLong(ret, GWL_USERDATA, (LONG)data);
	return ret;
}

void mswin_display_text_window (HWND hWnd)
{
	PNHTextWindow data;
	
	data = (PNHTextWindow)GetWindowLong(hWnd, GWL_USERDATA);
	if( data && data->window_text ) {
		HWND control;
		control = GetDlgItem(hWnd, IDC_TEXT_CONTROL);
		SendMessage(control, EM_FMTLINES, 1, 0 );
		SetWindowText(GetDlgItem(hWnd, IDC_TEXT_CONTROL), data->window_text);
	}

	mswin_popup_display(hWnd, NULL);
	mswin_popup_destroy(hWnd);
}
    
LRESULT CALLBACK TextWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND control;
	HDC hdc;
	PNHTextWindow data;
	
	data = (PNHTextWindow)GetWindowLong(hWnd, GWL_USERDATA);
	switch (message) 
	{
	case WM_INITDIALOG:
	    /* set text control font */
		control = GetDlgItem(hWnd, IDC_TEXT_CONTROL);
		if( !control ) {
			panic("cannot get text view window");
		}

		hdc = GetDC(control);
		SendMessage(control, WM_SETFONT, (WPARAM)mswin_get_font(NHW_TEXT, ATR_NONE, hdc, FALSE), 0);
		ReleaseDC(control, hdc);

		SetFocus(control);
	return FALSE;

	case WM_MSNH_COMMAND:
		onMSNHCommand(hWnd, wParam, lParam);
	break;

	case WM_SIZE:
		LayoutText(hWnd);
	return FALSE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
        { 
          case IDOK: 
		  case IDCANCEL:
			mswin_window_mark_dead(mswin_winid_from_handle(hWnd));
			if( GetNHApp()->hMainWnd==hWnd ) 
				GetNHApp()->hMainWnd=NULL;
			DestroyWindow(hWnd);
			SetFocus(GetNHApp()->hMainWnd);
			return TRUE;
		}
	break;

	case WM_CTLCOLORBTN:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC: { /* sent by edit control before it is drawn */
		HDC hdcEdit = (HDC) wParam; 
		HWND hwndEdit = (HWND) lParam;
		if( hwndEdit == GetDlgItem(hWnd, IDC_TEXT_CONTROL) ) {
			SetBkColor(hdcEdit, mswin_get_color(NHW_TEXT, MSWIN_COLOR_BG));
			SetTextColor(hdcEdit, mswin_get_color(NHW_TEXT, MSWIN_COLOR_FG)); 
			return (BOOL)mswin_get_brush(NHW_TEXT, MSWIN_COLOR_BG);
		}
	} return FALSE;

	case WM_DESTROY:
		if( data ) {
			if( data->window_text ) free(data->window_text);
			free(data);
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)0);
		}
	break;

	}
	return FALSE;
}

void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHTextWindow data;
	
	data = (PNHTextWindow)GetWindowLong(hWnd, GWL_USERDATA);
	switch( wParam ) {
	case MSNH_MSG_PUTSTR: {
		PMSNHMsgPutstr msg_data = (PMSNHMsgPutstr)lParam;
		TCHAR	wbuf[NHSTR_BUFSIZE];
		size_t text_size;

		if( !data->window_text ) {
			text_size = strlen(msg_data->text) + 4;
			data->window_text = (TCHAR*)malloc(text_size*sizeof(data->window_text[0]));
			ZeroMemory(data->window_text, text_size*sizeof(data->window_text[0]));
		} else {
			text_size = _tcslen(data->window_text) + strlen(msg_data->text) + 4;
			data->window_text = (TCHAR*)realloc(data->window_text, text_size*sizeof(data->window_text[0]));
		}
		if( !data->window_text ) break;
		
		_tcscat(data->window_text, NH_A2W(msg_data->text, wbuf, NHSTR_BUFSIZE)); 
		_tcscat(data->window_text, TEXT("\r\n"));
		break;
	}
	}
}

void LayoutText(HWND hWnd) 
{
	HWND  btn_ok;
	HWND  text;
	RECT  clrt, rt;
	POINT pt_elem, pt_ok;
	SIZE  sz_elem, sz_ok;

	text = GetDlgItem(hWnd, IDC_TEXT_CONTROL);
	btn_ok = GetDlgItem(hWnd, IDOK);

	/* get window coordinates */
	GetClientRect(hWnd, &clrt );
	
	/* set window placements */
	GetWindowRect(btn_ok, &rt);
	sz_ok.cx = clrt.right - clrt.left;
	sz_ok.cy = rt.bottom-rt.top;
	pt_ok.x = clrt.left;
	pt_ok.y = clrt.bottom - sz_ok.cy;

	pt_elem.x = clrt.left;
	pt_elem.y = clrt.top;
	sz_elem.cx = clrt.right - clrt.left;
	sz_elem.cy = pt_ok.y;

	MoveWindow(text, pt_elem.x, pt_elem.y, sz_elem.cx, sz_elem.cy, TRUE );
	MoveWindow(btn_ok, pt_ok.x, pt_ok.y, sz_ok.cx, sz_ok.cy, TRUE );
}

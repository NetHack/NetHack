/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "resource.h"
#include "mhtext.h"
#include "mhmsg.h"
#include "mhfont.h"

LRESULT CALLBACK	TextWndProc(HWND, UINT, WPARAM, LPARAM);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void LayoutText(HWND hwnd);

HWND mswin_init_text_window () {
	HWND ret;

	ret = CreateDialog(
			GetNHApp()->hApp,
			MAKEINTRESOURCE(IDD_TEXT),
			GetNHApp()->hMainWnd,
			TextWndProc
	);
	if( !ret ) panic("Cannot create text window");
	return ret;
}

void mswin_display_text_window (HWND hWnd)
{
	MSG msg;
	RECT rt;
	HWND map_wnd;

	GetNHApp()->hMenuWnd = hWnd;
	map_wnd = mswin_hwnd_from_winid(WIN_MAP);
	if( !IsWindow(map_wnd) ) map_wnd = GetNHApp()->hMainWnd;
	GetWindowRect(map_wnd, &rt);
	MoveWindow(hWnd, rt.left, rt.top, rt.right-rt.left, rt.bottom-rt.top, TRUE);
	ShowWindow(hWnd, SW_SHOW);
	SetFocus(hWnd);

	while( IsWindow(hWnd) && 
		   GetMessage(&msg, NULL, 0, 0)!=0 ) {
		if( !IsDialogMessage(hWnd, &msg) ) {
			if (!TranslateAccelerator(msg.hwnd, GetNHApp()->hAccelTable, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	GetNHApp()->hMenuWnd = NULL;
}
    
LRESULT CALLBACK TextWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND control;
	HDC hdc;

	switch (message) 
	{

	case WM_INITDIALOG:
	    /* set text control font */
		control = GetDlgItem(hWnd, IDC_TEXT_VIEW);
		hdc = GetDC(control);
		SendMessage(control, WM_SETFONT, (WPARAM)mswin_create_font(NHW_TEXT, ATR_NONE, hdc), 0);
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
	}
	return FALSE;
}

void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch( wParam ) {
	case MSNH_MSG_PUTSTR: {
		PMSNHMsgPutstr msg_data = (PMSNHMsgPutstr)lParam;
		HWND   text_view;
		TCHAR*  text;
		TCHAR	wbuf[BUFSZ];
		size_t text_size;

		text_view = GetDlgItem(hWnd, IDC_TEXT_VIEW);
		if( !text_view ) panic("cannot get text view window");
		
		text_size = GetWindowTextLength(text_view) + strlen(msg_data->text) + 3;
		text = (TCHAR*)malloc(text_size*sizeof(text[0]));
		if( !text ) break;
		ZeroMemory(text, text_size*sizeof(text[0]));
		
		GetWindowText(text_view, text, GetWindowTextLength(text_view));
		_tcscat(text, NH_A2W(msg_data->text, wbuf, sizeof(wbuf))); 
		_tcscat(text, TEXT("\n"));
		SetWindowText(text_view, text);

		free(text);
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

	text = GetDlgItem(hWnd, IDC_TEXT_VIEW);
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

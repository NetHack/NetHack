/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "mhtext.h"
#include "mhmsg.h"
#include "mhfont.h"
#include "mhcolor.h"
#include "mhtxtbuf.h"

typedef struct mswin_nethack_text_window {
	PNHTextBuffer  window_text;
	int done;
} NHTextWindow, *PNHTextWindow;

static WNDPROC editControlWndProc = NULL;

LRESULT CALLBACK	TextWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	NHTextControlWndProc(HWND, UINT, WPARAM, LPARAM);
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
	data->window_text = mswin_init_text_buffer(
			program_state.gameover? FALSE : GetNHApp()->bWrapText
		);
	SetWindowLong(ret, GWL_USERDATA, (LONG)data);
	return ret;
}

void mswin_display_text_window (HWND hWnd)
{
	PNHTextWindow data;
	
	data = (PNHTextWindow)GetWindowLong(hWnd, GWL_USERDATA);
	if( data ) {
		HWND control;
		control = GetDlgItem(hWnd, IDC_TEXT_CONTROL);
		SendMessage(control, EM_FMTLINES, 1, 0 );
		mswin_render_text(data->window_text, GetDlgItem(hWnd, IDC_TEXT_CONTROL));

		data->done = 0;
		mswin_popup_display(hWnd, &data->done);
		mswin_popup_destroy(hWnd);
	}
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

#if defined(WIN_CE_SMARTPHONE)
		/* special initialization for SmartPhone dialogs */ 
		NHSPhoneDialogSetup(hWnd, FALSE, GetNHApp()->bFullScreen);
#endif
		/* subclass edit control */
		editControlWndProc = (WNDPROC)GetWindowLong(control, GWL_WNDPROC);
		SetWindowLong(control, GWL_WNDPROC, (LONG)NHTextControlWndProc);

		if( !program_state.gameover && GetNHApp()->bWrapText ) {
			DWORD styles;
			styles = GetWindowLong(control, GWL_STYLE);
			if( styles ) {
				SetWindowLong(control, GWL_STYLE, styles & (~WS_HSCROLL));
				SetWindowPos(control, NULL, 0, 0, 0, 0, 
							  SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE );
			}
		}

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
			data->done = 1;
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
			mswin_free_text_buffer(data->window_text);
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
		mswin_add_text(data->window_text, msg_data->attr, msg_data->text);
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
	if( IsWindow(btn_ok) ) {
		GetWindowRect(btn_ok, &rt);
		sz_ok.cx = clrt.right - clrt.left;
		sz_ok.cy = rt.bottom-rt.top;
		pt_ok.x = clrt.left;
		pt_ok.y = clrt.bottom - sz_ok.cy;
		MoveWindow(btn_ok, pt_ok.x, pt_ok.y, sz_ok.cx, sz_ok.cy, TRUE );

		pt_elem.x = clrt.left;
		pt_elem.y = clrt.top;
		sz_elem.cx = clrt.right - clrt.left;
		sz_elem.cy = pt_ok.y;
		MoveWindow(text, pt_elem.x, pt_elem.y, sz_elem.cx, sz_elem.cy, TRUE );
	} else {
		pt_elem.x = clrt.left;
		pt_elem.y = clrt.top;
		sz_elem.cx = clrt.right - clrt.left;
		sz_elem.cy = clrt.bottom - clrt.top;
		MoveWindow(text, pt_elem.x, pt_elem.y, sz_elem.cx, sz_elem.cy, TRUE );
	}
}

/* Text control window proc - implements close on space and scrolling on arrows */
LRESULT CALLBACK NHTextControlWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case WM_KEYUP:
		switch( wParam ) {
		case VK_SPACE:
		case VK_RETURN:
			/* close on space */
			PostMessage(GetParent(hWnd), WM_COMMAND, MAKELONG(IDOK, 0), 0);
			return 0;
		
		case VK_UP:
			/* scoll up */
			PostMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), (LPARAM)NULL);
			return 0;

		case VK_DOWN:
			/* scoll down */
			PostMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), (LPARAM)NULL);
			return 0;

		case VK_LEFT:
			/* scoll left */
			PostMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_LINELEFT, 0), (LPARAM)NULL);
			return 0;

		case VK_RIGHT:
			/* scoll right */
			PostMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_LINERIGHT, 0), (LPARAM)NULL);
			return 0;
		}
		break; /* case WM_KEYUP: */
	}

	if( editControlWndProc ) 
		return CallWindowProc(editControlWndProc, hWnd, message, wParam, lParam);
	else 
		return 0;
}

/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "resource.h"
#include "mhtext.h"
#include "mhmsg.h"
#include "mhfont.h"

PNHWinApp GetNHApp(void);

typedef struct mswin_nethack_text_window {
	TCHAR*  window_text;
} NHTextWindow, *PNHTextWindow;

static WNDPROC  editControlWndProc = 0;
#define DEFAULT_COLOR_BG_TEXT	COLOR_WINDOW
#define DEFAULT_COLOR_FG_TEXT	COLOR_WINDOWTEXT

BOOL	CALLBACK	NHTextWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	NHEditHookWndProc(HWND, UINT, WPARAM, LPARAM);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void LayoutText(HWND hwnd);

HWND mswin_init_text_window () {
	HWND ret;
	PNHTextWindow data;

	ret = CreateDialog(
			GetNHApp()->hApp,
			MAKEINTRESOURCE(IDD_NHTEXT),
			GetNHApp()->hMainWnd,
			NHTextWndProc
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
    
BOOL CALLBACK NHTextWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND control;
	HDC hdc;
	PNHTextWindow data;
    TCHAR title[MAX_LOADSTRING];
	
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

		/* subclass edit control */
		editControlWndProc = (WNDPROC)GetWindowLong(control, GWL_WNDPROC);
		SetWindowLong(control, GWL_WNDPROC, (LONG)NHEditHookWndProc);

		SetFocus(control);

        /* Even though the dialog has no caption, you can still set the title 
           which shows on Alt-Tab */
        LoadString(GetNHApp()->hApp, IDS_APP_TITLE, title, MAX_LOADSTRING);
        SetWindowText(hWnd, title);
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
          case IDC_TEXT_CONTROL:
            switch (HIWORD(wParam))
            {
              case EN_SETFOCUS:
                HideCaret((HWND)lParam);
                return TRUE;
            }
		}
	break;

	case WM_CTLCOLORSTATIC: { /* sent by edit control before it is drawn */
		HDC hdcEdit = (HDC) wParam; 
		HWND hwndEdit = (HWND) lParam;
		if( hwndEdit == GetDlgItem(hWnd, IDC_TEXT_CONTROL) ) {
			SetBkColor(hdcEdit, 
				text_bg_brush ? text_bg_color : (COLORREF)GetSysColor(DEFAULT_COLOR_BG_TEXT)
				);
			SetTextColor(hdcEdit, 
				text_fg_brush ? text_fg_color : (COLORREF)GetSysColor(DEFAULT_COLOR_FG_TEXT) 
				); 
			return (BOOL)(text_bg_brush 
					? text_bg_brush : SYSCLR_TO_BRUSH(DEFAULT_COLOR_BG_TEXT));
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
		TCHAR	wbuf[BUFSZ];
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
		
		_tcscat(data->window_text, NH_A2W(msg_data->text, wbuf, BUFSZ)); 
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

/* Edit box hook */
LRESULT CALLBACK NHEditHookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {

	case WM_KEYDOWN:
		switch (wParam)
        {
    	/* close on space in Windows mode
           page down on space in NetHack mode */
        case VK_SPACE:
        {   
            SCROLLINFO si;

            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
            GetScrollInfo(hWnd, SB_VERT, &si);
            /* If nethackmode and not at the end of the list */
            if (GetNHApp()->regNetHackMode &&
                    (si.nPos + (int)si.nPage) <= (si.nMax - si.nMin))
                SendMessage(hWnd, EM_SCROLL, SB_PAGEDOWN, 0);
            else
			    PostMessage(GetParent(hWnd), WM_COMMAND, MAKELONG(IDOK, 0), 0);
            return 0;
        }
        case VK_NEXT:
            SendMessage(hWnd, EM_SCROLL, SB_PAGEDOWN, 0);
            return 0;
        case VK_PRIOR:
            SendMessage(hWnd, EM_SCROLL, SB_PAGEUP, 0);
            return 0;
        case VK_UP:
            SendMessage(hWnd, EM_SCROLL, SB_LINEUP, 0);
            return 0;
        case VK_DOWN:
            SendMessage(hWnd, EM_SCROLL, SB_LINEDOWN, 0);
            return 0;

		}
	break;
	}

	if( editControlWndProc ) 
		return CallWindowProc(editControlWndProc, hWnd, message, wParam, lParam);
	else 
		return 0;
}

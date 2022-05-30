/* NetHack 3.7	mhtext.c	$NHDT-Date: 1596498362 2020/08/03 23:46:02 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.31 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "resource.h"
#include "mhtext.h"
#include "mhmsg.h"
#include "mhfont.h"

PNHWinApp GetNHApp(void);

typedef struct mswin_nethack_text_window {
    TCHAR *window_text;
} NHTextWindow, *PNHTextWindow;

static WNDPROC editControlWndProc = 0;
#define DEFAULT_COLOR_BG_TEXT COLOR_WINDOW
#define DEFAULT_COLOR_FG_TEXT COLOR_WINDOWTEXT

INT_PTR CALLBACK NHTextWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NHEditHookWndProc(HWND, UINT, WPARAM, LPARAM);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void LayoutText(HWND hwnd);

HWND
mswin_init_text_window(void)
{
    HWND ret;
    RECT rt;

    /* get window position */
    if (GetNHApp()->bAutoLayout) {
        SetRect(&rt, 0, 0, 0, 0);
    } else {
        mswin_get_window_placement(NHW_TEXT, &rt);
    }

    /* create text widnow object */
    ret = CreateDialog(GetNHApp()->hApp, MAKEINTRESOURCE(IDD_NHTEXT),
                       GetNHApp()->hMainWnd, NHTextWndProc);
    if (!ret)
        panic("Cannot create text window");

    /* move it in the predefined position */
    if (!GetNHApp()->bAutoLayout) {
        MoveWindow(ret, rt.left, rt.top, rt.right - rt.left,
                   rt.bottom - rt.top, TRUE);
    }

    /* Set window caption */
    SetWindowText(ret, "Text");

    mswin_apply_window_style(ret);

    return ret;
}

void
mswin_display_text_window(HWND hWnd)
{
    PNHTextWindow data;

    data = (PNHTextWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (data && data->window_text) {
        HWND control;
        control = GetDlgItem(hWnd, IDC_TEXT_CONTROL);
        SendMessage(control, EM_FMTLINES, 1, 0);
        SetWindowText(control, data->window_text);
    }

    mswin_popup_display(hWnd, NULL);
    mswin_popup_destroy(hWnd);
}

INT_PTR CALLBACK
NHTextWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PNHTextWindow data = (PNHTextWindow)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (message) {
    case WM_INITDIALOG: {
        data = (PNHTextWindow)malloc(sizeof(NHTextWindow));
        if (!data)
            panic("out of memory");
        ZeroMemory(data, sizeof(NHTextWindow));
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)data);

        HWND control = GetDlgItem(hWnd, IDC_TEXT_CONTROL);
        HDC hdc = GetDC(control);
        cached_font * font = mswin_get_font(NHW_TEXT, ATR_NONE, hdc, FALSE);
        /* set text control font */
        
        if (!control) {
            panic("cannot get text view window");
        }

        SendMessage(control, WM_SETFONT, (WPARAM) font->hFont, 0);
        ReleaseDC(control, hdc);

        /* subclass edit control */
        editControlWndProc =
            (WNDPROC)GetWindowLongPtr(control, GWLP_WNDPROC);
        SetWindowLongPtr(control, GWLP_WNDPROC, (LONG_PTR)NHEditHookWndProc);

        SetFocus(control);

        /* Even though the dialog has no caption, you can still set the title
           which shows on Alt-Tab */
        TCHAR title[MAX_LOADSTRING];
        LoadString(GetNHApp()->hApp, IDS_APP_TITLE, title, MAX_LOADSTRING);
        SetWindowText(hWnd, title);
    } break;

    case WM_MSNH_COMMAND:
        onMSNHCommand(hWnd, wParam, lParam);
        break;

    case WM_SIZE: {
        RECT rt;

        GetWindowRect(hWnd, &rt);
        ScreenToClient(GetNHApp()->hMainWnd, (LPPOINT) &rt);
        ScreenToClient(GetNHApp()->hMainWnd, ((LPPOINT) &rt) + 1);
        mswin_update_window_placement(NHW_TEXT, &rt);

        LayoutText(hWnd);
    }
        return FALSE;

    case WM_MOVE: {
        RECT rt;
        GetWindowRect(hWnd, &rt);
        ScreenToClient(GetNHApp()->hMainWnd, (LPPOINT) &rt);
        ScreenToClient(GetNHApp()->hMainWnd, ((LPPOINT) &rt) + 1);
        mswin_update_window_placement(NHW_TEXT, &rt);
    }
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            mswin_window_mark_dead(mswin_winid_from_handle(hWnd));
            if (GetNHApp()->hMainWnd == hWnd)
                GetNHApp()->hMainWnd = NULL;
            DestroyWindow(hWnd);
            SetFocus(GetNHApp()->hMainWnd);
            return TRUE;
        }
        break;

    case WM_CTLCOLORSTATIC: { /* sent by edit control before it is drawn */
        HDC hdcEdit = (HDC) wParam;
        HWND hwndEdit = (HWND) lParam;
        if (hwndEdit == GetDlgItem(hWnd, IDC_TEXT_CONTROL)) {
            SetBkColor(hdcEdit, text_bg_brush ? text_bg_color
                                              : (COLORREF) GetSysColor(
                                                    DEFAULT_COLOR_BG_TEXT));
            SetTextColor(hdcEdit, text_fg_brush ? text_fg_color
                                                : (COLORREF) GetSysColor(
                                                      DEFAULT_COLOR_FG_TEXT));
            return (INT_PTR)(text_bg_brush
                                 ? text_bg_brush
                                 : SYSCLR_TO_BRUSH(DEFAULT_COLOR_BG_TEXT));
        }
    }
        return FALSE;

    case WM_DESTROY:
        if (data) {
            if (data->window_text)
                free(data->window_text);
            free(data);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) 0);
        }
        break;

    }


    return FALSE;
}

void
onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHTextWindow data;

    data = (PNHTextWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (wParam) {
    case MSNH_MSG_PUTSTR: {
        PMSNHMsgPutstr msg_data = (PMSNHMsgPutstr) lParam;
        TCHAR wbuf[BUFSZ];
        size_t text_size;

        if (!data->window_text) {
            text_size = strlen(msg_data->text) + 4;
            data->window_text =
                (TCHAR *) malloc(text_size * sizeof(data->window_text[0]));
            ZeroMemory(data->window_text,
                       text_size * sizeof(data->window_text[0]));
        } else {
            text_size =
                _tcslen(data->window_text) + strlen(msg_data->text) + 4;
            data->window_text = (TCHAR *) realloc(
                data->window_text, text_size * sizeof(data->window_text[0]));
        }
        if (!data->window_text)
            break;

        _tcscat(data->window_text, NH_A2W(msg_data->text, wbuf, BUFSZ));
        _tcscat(data->window_text, TEXT("\r\n"));
        break;
    }

        case MSNH_MSG_RANDOM_INPUT: {
            PostMessage(GetDlgItem(hWnd, IDC_TEXT_CONTROL), 
            WM_MSNH_COMMAND, MSNH_MSG_RANDOM_INPUT, 0);
        }
        break;

    }
}

void
LayoutText(HWND hWnd)
{
    HWND btn_ok;
    HWND text;
    RECT clrt, rt;
    POINT pt_elem, pt_ok;
    SIZE sz_elem, sz_ok;

    text = GetDlgItem(hWnd, IDC_TEXT_CONTROL);
    btn_ok = GetDlgItem(hWnd, IDOK);

    /* get window coordinates */
    GetClientRect(hWnd, &clrt);

    if( !GetNHApp()->regNetHackMode ) {
        /* set window placements */
        GetWindowRect(btn_ok, &rt);
        sz_ok.cx = clrt.right - clrt.left;
        sz_ok.cy = rt.bottom - rt.top;
        pt_ok.x = clrt.left;
        pt_ok.y = clrt.bottom - sz_ok.cy;

        pt_elem.x = clrt.left;
        pt_elem.y = clrt.top;
        sz_elem.cx = clrt.right - clrt.left;
        sz_elem.cy = pt_ok.y;

        MoveWindow(text, pt_elem.x, pt_elem.y, sz_elem.cx, sz_elem.cy, TRUE);
        MoveWindow(btn_ok, pt_ok.x, pt_ok.y, sz_ok.cx, sz_ok.cy, TRUE);
    } else {
        sz_ok.cx = sz_ok.cy = 0;

        pt_ok.x = pt_ok.y = 0;
        pt_elem.x = clrt.left;
        pt_elem.y = clrt.top;

        sz_elem.cx = clrt.right - clrt.left;
        sz_elem.cy = clrt.bottom - clrt.top;

        ShowWindow(btn_ok, SW_HIDE);
        MoveWindow(text, pt_elem.x, pt_elem.y, sz_elem.cx, sz_elem.cy, TRUE );
    }
    mswin_apply_window_style(text);
}

/* Edit box hook */
LRESULT CALLBACK
NHEditHookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hWndParent = GetParent(hWnd);
    HDC hDC;
    RECT rc;

    switch (message) {
    case WM_ERASEBKGND:
        hDC = (HDC) wParam;
        GetClientRect(hWnd, &rc);
        FillRect(hDC, &rc, text_bg_brush
                               ? text_bg_brush
                               : SYSCLR_TO_BRUSH(DEFAULT_COLOR_BG_TEXT));
        return 1;

    case WM_KEYDOWN:
        switch (wParam) {
        /* close on space in Windows mode
           page down on space in NetHack mode */
        case VK_SPACE: {
            SCROLLINFO si;

            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
            GetScrollInfo(hWnd, SB_VERT, &si);
            /* If nethackmode and not at the end of the list */
            if (GetNHApp()->regNetHackMode
                && (si.nPos + (int) si.nPage) <= (si.nMax - si.nMin))
                SendMessage(hWnd, EM_SCROLL, SB_PAGEDOWN, 0);
            else
                PostMessage(hWndParent, WM_COMMAND, MAKELONG(IDOK, 0), 0);
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

    case WM_CHAR:
        switch(wParam) {
        case MENU_FIRST_PAGE:
            SendMessage(hWnd, EM_SCROLL, SB_TOP, 0);
            return 0;
        case MENU_LAST_PAGE:
            SendMessage(hWnd, EM_SCROLL, SB_BOTTOM, 0);
            return 0;
        case MENU_NEXT_PAGE:
            SendMessage(hWnd, EM_SCROLL, SB_PAGEDOWN, 0);
            return 0;
        case MENU_PREVIOUS_PAGE:
            SendMessage(hWnd, EM_SCROLL, SB_PAGEUP, 0);
            return 0;
        }
        break;

    /* edit control needs to know nothing of focus. We will take care of it
     * for it */
    case WM_SETFOCUS:
        HideCaret(hWnd);
        return 0;

    case WM_MSNH_COMMAND:
        if (wParam == MSNH_MSG_RANDOM_INPUT) {
            char c = randomkey();
            if (c == '\n')
                PostMessage(hWndParent, WM_COMMAND, MAKELONG(IDOK, 0), 0);
            else if (c == '\033')
                PostMessage(hWndParent, WM_COMMAND, MAKELONG(IDCANCEL, 0), 0);
            else
                PostMessage(hWnd, WM_CHAR, c, 0);
            return 0;
        }
        break;

    }

    if (editControlWndProc)
        return CallWindowProc(editControlWndProc, hWnd, message, wParam,
                              lParam);
    else
        return 0;
}

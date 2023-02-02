/* NetHack 3.6	mhtext.c	$NHDT-Date: 1432512802 2015/05/25 00:13:22 $  $NHDT-Branch: master $:$NHDT-Revision: 1.22 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "mhtext.h"
#include "mhmsg.h"
#include "mhfont.h"
#include "mhcolor.h"
#include "mhtxtbuf.h"

typedef struct mswin_nethack_text_window {
    PNHTextBuffer window_text;
    int done;
} NHTextWindow, *PNHTextWindow;

static WNDPROC editControlWndProc = NULL;

LRESULT CALLBACK TextWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NHTextControlWndProc(HWND, UINT, WPARAM, LPARAM);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void LayoutText(HWND hwnd);
static void ToggleWrapStatus(HWND hDlg, BOOL bWrap);

HWND
mswin_init_text_window()
{
    HWND ret;
    PNHTextWindow data;

    ret = CreateDialog(GetNHApp()->hApp, MAKEINTRESOURCE(IDD_NHTEXT),
                       GetNHApp()->hMainWnd, TextWndProc);
    if (!ret)
        panic("Cannot create text window");

    data = (PNHTextWindow) malloc(sizeof(NHTextWindow));
    if (!data)
        panic("out of memory");

    ZeroMemory(data, sizeof(NHTextWindow));
    data->window_text = mswin_init_text_buffer(
        gp.program_state.gameover ? FALSE : GetNHApp()->bWrapText);
    SetWindowLong(ret, GWL_USERDATA, (LONG) data);
    return ret;
}

void
mswin_display_text_window(HWND hWnd)
{
    PNHTextWindow data;

    data = (PNHTextWindow) GetWindowLong(hWnd, GWL_USERDATA);
    if (data) {
        ToggleWrapStatus(hWnd, mswin_get_text_wrap(data->window_text));
        data->done = 0;
        mswin_popup_display(hWnd, &data->done);
        mswin_popup_destroy(hWnd);
    }
}

LRESULT CALLBACK
TextWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND control;
    HDC hdc;
    PNHTextWindow data;

    data = (PNHTextWindow) GetWindowLong(hWnd, GWL_USERDATA);
    switch (message) {
    case WM_INITDIALOG:
        /* set text control font */
        control = GetDlgItem(hWnd, IDC_TEXT_CONTROL);
        if (!control) {
            panic("cannot get text view window");
        }

        hdc = GetDC(control);
        SendMessage(control, WM_SETFONT,
                    (WPARAM) mswin_get_font(NHW_TEXT, ATR_NONE, hdc, FALSE),
                    0);
        ReleaseDC(control, hdc);

#if defined(WIN_CE_SMARTPHONE)
        /* special initialization for SmartPhone dialogs */
        NHSPhoneDialogSetup(hWnd, IDC_SPHONE_TEXTDIALOGBAR, FALSE,
                            GetNHApp()->bFullScreen);
#endif
        /* subclass edit control */
        editControlWndProc = (WNDPROC) GetWindowLong(control, GWL_WNDPROC);
        SetWindowLong(control, GWL_WNDPROC, (LONG) NHTextControlWndProc);

        SetFocus(control);
        return FALSE;

    case WM_MSNH_COMMAND:
        onMSNHCommand(hWnd, wParam, lParam);
        break;

    case WM_SIZE:
        LayoutText(hWnd);
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            data->done = 1;
            return TRUE;
        case IDC_TEXT_TOGGLE_WRAP:
            ToggleWrapStatus(hWnd, !mswin_get_text_wrap(data->window_text));
            return TRUE;
        }
        break;

    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: { /* sent by edit control before it is drawn */
        HDC hdcEdit = (HDC) wParam;
        HWND hwndEdit = (HWND) lParam;
        if (hwndEdit == GetDlgItem(hWnd, IDC_TEXT_CONTROL)) {
            SetBkColor(hdcEdit, mswin_get_color(NHW_TEXT, MSWIN_COLOR_BG));
            SetTextColor(hdcEdit, mswin_get_color(NHW_TEXT, MSWIN_COLOR_FG));
            return (BOOL) mswin_get_brush(NHW_TEXT, MSWIN_COLOR_BG);
        }
    }
        return FALSE;

    case WM_DESTROY:
        if (data) {
            mswin_free_text_buffer(data->window_text);
            free(data);
            SetWindowLong(hWnd, GWL_USERDATA, (LONG) 0);
        }
        break;
    }
    return FALSE;
}

void
onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHTextWindow data;

    data = (PNHTextWindow) GetWindowLong(hWnd, GWL_USERDATA);
    switch (wParam) {
    case MSNH_MSG_PUTSTR: {
        PMSNHMsgPutstr msg_data = (PMSNHMsgPutstr) lParam;
        mswin_add_text(data->window_text, msg_data->attr, msg_data->text);
        break;
    }
    }
}

void
ToggleWrapStatus(HWND hDlg, BOOL bWrap)
{
    DWORD styles;
    PNHTextWindow data;
    HWND control;
    TCHAR wbuf[BUFSZ];

    data = (PNHTextWindow) GetWindowLong(hDlg, GWL_USERDATA);

    control = GetDlgItem(hDlg, IDC_TEXT_CONTROL);
    if (!control) {
        panic("cannot get text view window");
    }

    /* set horizontal scrollbar status */
    styles = GetWindowLong(control, GWL_STYLE);
    if (styles) {
        SetWindowLong(control, GWL_STYLE, (bWrap ? (styles & (~WS_HSCROLL))
                                                 : (styles | WS_HSCROLL)));
        SetWindowPos(control, NULL, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE
                         | SWP_NOSIZE);
    }

    /* set text wrap mode */
    mswin_set_text_wrap(data->window_text, bWrap);
    mswin_render_text(data->window_text, control);

    /* change button status */
    ZeroMemory(wbuf, sizeof(wbuf));
    if (!LoadString(GetNHApp()->hApp,
                    (bWrap ? IDS_TEXT_UNWRAP : IDS_TEXT_WRAP), wbuf, BUFSZ)) {
        panic("cannot load text window strings");
    }

#if defined(WIN_CE_SMARTPHONE)
    {
        TBBUTTONINFO tbbi;

        ZeroMemory(&tbbi, sizeof(tbbi));
        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_TEXT;
        tbbi.pszText = wbuf;
        if (!SendMessage(SHFindMenuBar(hDlg), TB_SETBUTTONINFO,
                         IDC_TEXT_TOGGLE_WRAP, (LPARAM) &tbbi)) {
            error("Cannot update IDC_TEXT_TOGGLE_WRAP menu item.");
        }
    }
#else
    SendDlgItemMessage(hDlg, IDC_TEXT_TOGGLE_WRAP, WM_SETTEXT, (WPARAM) 0,
                       (LPARAM) wbuf);
#endif
}

void
LayoutText(HWND hWnd)
{
    HWND btn_ok, btn_wrap;
    HWND text;
    RECT clrt, rt;
    POINT pt_elem, pt_ok, pt_wrap;
    SIZE sz_elem, sz_ok, sz_wrap;

    text = GetDlgItem(hWnd, IDC_TEXT_CONTROL);
    btn_ok = GetDlgItem(hWnd, IDOK);
    btn_wrap = GetDlgItem(hWnd, IDC_TEXT_TOGGLE_WRAP);

    /* get window coordinates */
    GetClientRect(hWnd, &clrt);

    /* set window placements */
    if (IsWindow(btn_ok)) {
        GetWindowRect(btn_ok, &rt);
        sz_ok.cx = (clrt.right - clrt.left) / 2;
        sz_ok.cy = rt.bottom - rt.top;
        pt_ok.x = clrt.left;
        pt_ok.y = clrt.bottom - sz_ok.cy;
        MoveWindow(btn_ok, pt_ok.x, pt_ok.y, sz_ok.cx, sz_ok.cy, TRUE);

        sz_wrap.cx = (clrt.right - clrt.left) / 2;
        sz_wrap.cy = rt.bottom - rt.top;
        pt_wrap.x = clrt.left + sz_ok.cx;
        pt_wrap.y = clrt.bottom - sz_ok.cy;
        MoveWindow(btn_wrap, pt_wrap.x, pt_wrap.y, sz_wrap.cx, sz_wrap.cy,
                   TRUE);

        pt_elem.x = clrt.left;
        pt_elem.y = clrt.top;
        sz_elem.cx = clrt.right - clrt.left;
        sz_elem.cy = pt_ok.y;
        MoveWindow(text, pt_elem.x, pt_elem.y, sz_elem.cx, sz_elem.cy, TRUE);
    } else {
        pt_elem.x = clrt.left;
        pt_elem.y = clrt.top;
        sz_elem.cx = clrt.right - clrt.left;
        sz_elem.cy = clrt.bottom - clrt.top;
        MoveWindow(text, pt_elem.x, pt_elem.y, sz_elem.cx, sz_elem.cy, TRUE);
    }
}

/* Text control window proc - implements close on space and scrolling on
 * arrows */
LRESULT CALLBACK
NHTextControlWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    /* tell Windows not to process arrow keys (we want them) */
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_SPACE:
        case VK_RETURN:
            /* close on space */
            PostMessage(GetParent(hWnd), WM_COMMAND, MAKELONG(IDOK, 0), 0);
            return 0;

        case VK_UP:
            /* scoll up */
            PostMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0),
                        (LPARAM) NULL);
            return 0;

        case VK_DOWN:
            /* scoll down */
            PostMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0),
                        (LPARAM) NULL);
            return 0;

        case VK_LEFT:
            /* scoll left */
            PostMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_LINELEFT, 0),
                        (LPARAM) NULL);
            return 0;

        case VK_RIGHT:
            /* scoll right */
            PostMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_LINERIGHT, 0),
                        (LPARAM) NULL);
            return 0;
        }
        break; /* case WM_KEYDOWN: */
    }

    if (editControlWndProc)
        return CallWindowProc(editControlWndProc, hWnd, message, wParam,
                              lParam);
    else
        return 0;
}

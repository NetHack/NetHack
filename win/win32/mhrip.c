/* NetHack 3.6	mhrip.c	$NHDT-Date: 1432512812 2015/05/25 00:13:32 $  $NHDT-Branch: master $:$NHDT-Revision: 1.19 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "resource.h"
#include "mhrip.h"
#include "mhmsg.h"
#include "mhfont.h"

#define RIP_WIDTH 400
#define RIP_HEIGHT 200

#define RIP_GRAVE_HEIGHT 120
#define RIP_GRAVE_WIDTH 115
#define RIP_GRAVE_X 90
#define RIP_GRAVE_Y 60

#define RIP_OFFSET_X 10
#define RIP_OFFSET_Y 10

PNHWinApp GetNHApp(void);

typedef struct mswin_nethack_text_window {
    HANDLE rip_bmp;
    TCHAR *window_text;
    TCHAR *rip_text;
} NHRIPWindow, *PNHRIPWindow;

INT_PTR CALLBACK NHRIPWndProc(HWND, UINT, WPARAM, LPARAM);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);

HWND
mswin_init_RIP_window()
{
    HWND ret;
    PNHRIPWindow data;

    ret = CreateDialog(GetNHApp()->hApp, MAKEINTRESOURCE(IDD_NHRIP),
                       GetNHApp()->hMainWnd, NHRIPWndProc);
    if (!ret)
        panic("Cannot create rip window");

    data = (PNHRIPWindow) malloc(sizeof(NHRIPWindow));
    if (!data)
        panic("out of memory");

    ZeroMemory(data, sizeof(NHRIPWindow));
    SetWindowLongPtr(ret, GWLP_USERDATA, (LONG_PTR) data);
    return ret;
}

void
mswin_display_RIP_window(HWND hWnd)
{
    MSG msg;
    RECT rt;
    PNHRIPWindow data;
    HWND mapWnd;
    RECT riprt;
    RECT clientrect;
    RECT textrect;
    HDC hdc;
    HFONT OldFont;

    data = (PNHRIPWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    GetNHApp()->hPopupWnd = hWnd;
    mapWnd = mswin_hwnd_from_winid(WIN_MAP);
    if (!IsWindow(mapWnd))
        mapWnd = GetNHApp()->hMainWnd;
    GetWindowRect(mapWnd, &rt);
    GetWindowRect(hWnd, &riprt);
    GetClientRect(hWnd, &clientrect);
    textrect = clientrect;
    textrect.top += RIP_OFFSET_Y;
    textrect.left += RIP_OFFSET_X;
    textrect.right -= RIP_OFFSET_X;
    if (data->window_text) {
        hdc = GetDC(hWnd);
        OldFont = SelectObject(hdc, mswin_get_font(NHW_TEXT, 0, hdc, FALSE));
        DrawText(hdc, data->window_text, strlen(data->window_text), &textrect,
                 DT_LEFT | DT_NOPREFIX | DT_CALCRECT);
        SelectObject(hdc, OldFont);
        ReleaseDC(hWnd, hdc);
    }
    if (textrect.right - textrect.left > RIP_WIDTH)
        clientrect.right = textrect.right + RIP_OFFSET_X - clientrect.right;
    else
        clientrect.right =
            textrect.left + 2 * RIP_OFFSET_X + RIP_WIDTH - clientrect.right;
    clientrect.bottom =
        textrect.bottom + RIP_HEIGHT + RIP_OFFSET_Y - clientrect.bottom;
    GetWindowRect(GetDlgItem(hWnd, IDOK), &textrect);
    textrect.right -= textrect.left;
    textrect.bottom -= textrect.top;
    clientrect.bottom += textrect.bottom + RIP_OFFSET_Y;
    riprt.right -= riprt.left;
    riprt.bottom -= riprt.top;
    riprt.right += clientrect.right;
    riprt.bottom += clientrect.bottom;
    rt.left += (rt.right - rt.left - riprt.right) / 2;
    rt.top += (rt.bottom - rt.top - riprt.bottom) / 2;

    MoveWindow(hWnd, rt.left, rt.top, riprt.right, riprt.bottom, TRUE);
    GetClientRect(hWnd, &clientrect);
    MoveWindow(GetDlgItem(hWnd, IDOK),
               (clientrect.right - clientrect.left - textrect.right) / 2,
               clientrect.bottom - textrect.bottom - RIP_OFFSET_Y,
               textrect.right, textrect.bottom, TRUE);
    ShowWindow(hWnd, SW_SHOW);

    while (IsWindow(hWnd) && GetMessage(&msg, NULL, 0, 0) != 0) {
        if (!IsDialogMessage(hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    GetNHApp()->hPopupWnd = NULL;
}

INT_PTR CALLBACK
NHRIPWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PNHRIPWindow data;

    data = (PNHRIPWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (message) {
    case WM_INITDIALOG:
        /* set text control font */
        hdc = GetDC(hWnd);
        SendMessage(hWnd, WM_SETFONT,
                    (WPARAM) mswin_get_font(NHW_TEXT, ATR_NONE, hdc, FALSE),
                    0);
        ReleaseDC(hWnd, hdc);

        SetFocus(GetDlgItem(hWnd, IDOK));
        return FALSE;

    case WM_MSNH_COMMAND:
        onMSNHCommand(hWnd, wParam, lParam);
        break;

    case WM_PAINT: {
        int bitmap_offset;
        RECT clientrect;
        RECT textrect;
        HDC hdcBitmap;
        HANDLE OldBitmap;
        PAINTSTRUCT ps;
        HFONT OldFont;

        hdc = BeginPaint(hWnd, &ps);
        OldFont = SelectObject(hdc, mswin_get_font(NHW_TEXT, 0, hdc, FALSE));
        hdcBitmap = CreateCompatibleDC(hdc);
        SetBkMode(hdc, TRANSPARENT);
        GetClientRect(hWnd, &clientrect);
        textrect = clientrect;
        textrect.top += RIP_OFFSET_Y;
        textrect.left += RIP_OFFSET_X;
        textrect.right -= RIP_OFFSET_X;
        if (data->window_text) {
            DrawText(hdc, data->window_text, strlen(data->window_text),
                     &textrect, DT_LEFT | DT_NOPREFIX | DT_CALCRECT);
            DrawText(hdc, data->window_text, strlen(data->window_text),
                     &textrect, DT_LEFT | DT_NOPREFIX);
        }
        OldBitmap = SelectObject(hdcBitmap, GetNHApp()->bmpRip);
        SetBkMode(hdc, OPAQUE);
        bitmap_offset = (textrect.right - textrect.left - RIP_WIDTH) / 2;
        BitBlt(hdc, textrect.left + bitmap_offset, textrect.bottom, RIP_WIDTH,
               RIP_HEIGHT, hdcBitmap, 0, 0, SRCCOPY);
        SetBkMode(hdc, TRANSPARENT);
        if (data->rip_text) {
            textrect.left += RIP_GRAVE_X + bitmap_offset;
            textrect.top = textrect.bottom + RIP_GRAVE_Y;
            textrect.right = textrect.left + RIP_GRAVE_WIDTH;
            textrect.bottom = textrect.top + RIP_GRAVE_HEIGHT;
            DrawText(hdc, data->rip_text, strlen(data->rip_text), &textrect,
                     DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_WORDBREAK);
        }
        SelectObject(hdcBitmap, OldBitmap);
        SelectObject(hdc, OldFont);
        DeleteDC(hdcBitmap);
        EndPaint(hWnd, &ps);
    } break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            mswin_window_mark_dead(mswin_winid_from_handle(hWnd));
            if (GetNHApp()->hMainWnd == hWnd)
                GetNHApp()->hMainWnd = NULL;
            DestroyWindow(hWnd);
            SetFocus(GetNHApp()->hMainWnd);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        /* if we get this here, we saved the bones so we can just force a quit
         */

        mswin_window_mark_dead(mswin_winid_from_handle(hWnd));
        if (GetNHApp()->hMainWnd == hWnd)
            GetNHApp()->hMainWnd = NULL;
        DestroyWindow(hWnd);
        SetFocus(GetNHApp()->hMainWnd);
        program_state.stopprint++;
        return TRUE;

    case WM_DESTROY:
        if (data) {
            if (data->window_text)
                free(data->window_text);
            if (data->rip_text)
                free(data->rip_text);
            if (data->rip_bmp != NULL)
                DeleteObject(data->rip_bmp);
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
    PNHRIPWindow data;
    static int InRipText = 1;
    data = (PNHRIPWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
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
    case MSNH_MSG_DIED: {
        data->rip_text = data->window_text;
        data->window_text = NULL;
        break;
    }
    }
}

void
mswin_finish_rip_text(winid wid)
{
    SendMessage(mswin_hwnd_from_winid(wid), WM_MSNH_COMMAND, MSNH_MSG_DIED,
                0);
}

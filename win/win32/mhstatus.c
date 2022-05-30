/* NetHack 3.7	mhstatus.c	$NHDT-Date: 1596498360 2020/08/03 23:46:00 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.35 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include <assert.h>
#include "winos.h"
#include "winMS.h"
#include "mhstatus.h"
#include "mhmsg.h"
#include "mhfont.h"

#ifndef STATUS_HILITES
#error STATUS_HILITES not defined
#endif

#define MAXWINDOWTEXT BUFSZ

#define STATUS_BLINK_INTERVAL 500 // milliseconds


extern COLORREF nhcolor_to_RGB(int c); /* from mhmap */

typedef struct back_buffer {
    HWND hWnd;
    HDC hdc;
    HBITMAP bm;
    HBITMAP orig_bm;
    int width;
    int height;
} back_buffer_t;

void back_buffer_free(back_buffer_t * back_buffer)
{
    if (back_buffer->bm != NULL) {
        SelectObject(back_buffer->hdc, back_buffer->orig_bm);
        DeleteObject(back_buffer->bm);
        back_buffer->bm = NULL;
    }
}

void back_buffer_allocate(back_buffer_t * back_buffer, int width, int height)
{
    HDC hdc = GetDC(back_buffer->hWnd);
    back_buffer->bm = CreateCompatibleBitmap(hdc, width, height);
    back_buffer->orig_bm =  SelectObject(back_buffer->hdc, back_buffer->bm);
    back_buffer->width = width;
    back_buffer->height = height;
    ReleaseDC(back_buffer->hWnd, hdc);
}

void back_buffer_size(back_buffer_t * back_buffer, int width, int height)
{
    if (back_buffer->bm == NULL || back_buffer->width != width
                                || back_buffer->height != height) {
        back_buffer_free(back_buffer);
        back_buffer_allocate(back_buffer, width, height);
    }
}

void back_buffer_init(back_buffer_t * back_buffer, HWND hWnd, int width, int height)
{
    back_buffer->hWnd = hWnd;
    back_buffer->hdc = CreateCompatibleDC(NULL);
    back_buffer->bm = NULL;

    back_buffer_size(back_buffer, width, height);
}


typedef struct mswin_nethack_status_window {
    int index;
    char window_text[NHSW_LINES][MAXWINDOWTEXT + 1];
    mswin_status_lines * status_lines;
    back_buffer_t back_buffer;
    boolean blink_state; /* true = invert blink text */
    boolean has_blink_fields; /* true if one or more has blink attriubte */
} NHStatusWindow, *PNHStatusWindow;


static TCHAR szStatusWindowClass[] = TEXT("MSNHStatusWndClass");
LRESULT CALLBACK StatusWndProc(HWND, UINT, WPARAM, LPARAM);
static void register_status_window_class(void);
static LRESULT onWMPaint(HWND hWnd, WPARAM wParam, LPARAM lParam);

#define DEFAULT_COLOR_BG_STATUS COLOR_WINDOW
#define DEFAULT_COLOR_FG_STATUS COLOR_WINDOWTEXT

HWND
mswin_init_status_window(void)
{
    static int run_once = 0;
    HWND ret;
    NHStatusWindow *data;
    RECT rt;
    int width, height;

    if (!run_once) {
        register_status_window_class();
        run_once = 1;
    }

    /* get window position */
    if (GetNHApp()->bAutoLayout) {
        SetRect(&rt, 0, 0, 0, 0);
    } else {
        mswin_get_window_placement(NHW_STATUS, &rt);
    }

    /* create status window object */
    width = rt.right - rt.left;
    height = rt.bottom - rt.top;
    ret = CreateWindow(szStatusWindowClass, NULL,
                       WS_CHILD | WS_CLIPSIBLINGS | WS_SIZEBOX,
                       rt.left, /* horizontal position of window */
                       rt.top,  /* vertical position of window */
                       width,   /* window width */
                       height,  /* window height */
                       GetNHApp()->hMainWnd, NULL, GetNHApp()->hApp, NULL);
    if (!ret)
        panic("Cannot create status window");

    /* Set window caption */
    SetWindowText(ret, "Status");

    /* create window data */
    data = (PNHStatusWindow) malloc(sizeof(NHStatusWindow));
    if (!data)
        panic("out of memory");

    ZeroMemory(data, sizeof(NHStatusWindow));
    SetWindowLongPtr(ret, GWLP_USERDATA, (LONG_PTR) data);

    back_buffer_init(&data->back_buffer, ret, width, height);

    mswin_apply_window_style(ret);

    if (status_bg_brush == NULL) {
        status_bg_color = (COLORREF)GetSysColor(DEFAULT_COLOR_BG_STATUS);
        status_bg_brush = CreateSolidBrush(status_bg_color);
    }

    if (status_fg_brush == NULL) {
        status_fg_color = (COLORREF)GetSysColor(DEFAULT_COLOR_FG_STATUS);
        status_fg_brush = CreateSolidBrush(status_fg_color);
    }

    /* set cursor blink timer */
    SetTimer(ret, 0, STATUS_BLINK_INTERVAL, NULL);

    return ret;
}

void
register_status_window_class(void)
{
    WNDCLASS wcex;
    ZeroMemory(&wcex, sizeof(wcex));

    wcex.style = CS_NOCLOSE;
    wcex.lpfnWndProc = (WNDPROC) StatusWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetNHApp()->hApp;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szStatusWindowClass;

    RegisterClass(&wcex);
}

LRESULT CALLBACK
StatusWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PNHStatusWindow data;

    data = (PNHStatusWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (message) {
    case WM_MSNH_COMMAND: {
        switch (wParam) {
        case MSNH_MSG_PUTSTR: {
            PMSNHMsgPutstr msg_data = (PMSNHMsgPutstr) lParam;
            strncpy(data->window_text[data->index], msg_data->text,
                    MAXWINDOWTEXT);
            data->index = (data->index + 1) % NHSW_LINES;
            InvalidateRect(hWnd, NULL, TRUE);
        } break;

        case MSNH_MSG_CLEAR_WINDOW: {
            data->index = 0;
            ZeroMemory(data->window_text, sizeof(data->window_text));
            InvalidateRect(hWnd, NULL, TRUE);
        } break;

        case MSNH_MSG_GETTEXT: {
            PMSNHMsgGetText msg_data = (PMSNHMsgGetText) lParam;
            msg_data->buffer[0] = '\0';
            size_t space_remaining = msg_data->max_size;

            for (int line = 0; line < NHSW_LINES; line++) {
                mswin_status_line *status_line = data->status_lines[line].lines;
                for (int i = 0; i < status_line->status_strings.count; i++) {
                    mswin_status_string * status_string = status_line->status_strings.status_strings[i];
                    if (status_string->space_in_front) {
                        strncat(msg_data->buffer, " ", space_remaining);
                        space_remaining = msg_data->max_size - strlen(msg_data->buffer);
                    }
                    strncat(msg_data->buffer, status_string->str, space_remaining);
                    space_remaining = msg_data->max_size - strlen(msg_data->buffer);
                }
                strncat(msg_data->buffer, "\r\n", space_remaining);
                space_remaining = msg_data->max_size - strlen(msg_data->buffer);
            }
        } break;

        case MSNH_MSG_UPDATE_STATUS: {
            PMSNHMsgUpdateStatus msg_data = (PMSNHMsgUpdateStatus) lParam;
            data->status_lines = msg_data->status_lines;
            InvalidateRect(hWnd, NULL, TRUE);
        } break;

                case MSNH_MSG_RANDOM_INPUT:
                    nhassert(0); // unexpected
                    break;

        } /* end switch( wParam ) { */
    } break;

    case WM_PAINT:
        return onWMPaint(hWnd, wParam, lParam);

    case WM_SIZE: {
        RECT rt;
        GetWindowRect(hWnd, &rt);
        ScreenToClient(GetNHApp()->hMainWnd, (LPPOINT) &rt);
        ScreenToClient(GetNHApp()->hMainWnd, ((LPPOINT) &rt) + 1);
        mswin_update_window_placement(NHW_STATUS, &rt);
    }
        return FALSE;

    case WM_MOVE: {
        RECT rt;
        GetWindowRect(hWnd, &rt);
        ScreenToClient(GetNHApp()->hMainWnd, (LPPOINT) &rt);
        ScreenToClient(GetNHApp()->hMainWnd, ((LPPOINT) &rt) + 1);
        mswin_update_window_placement(NHW_STATUS, &rt);
    }
        return FALSE;

    case WM_DESTROY:
        free(data);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) 0);
        break;

    case WM_SETFOCUS:
        SetFocus(GetNHApp()->hMainWnd);
        break;

    case WM_TIMER:
        data->blink_state = !data->blink_state;
        if (data->has_blink_fields)
            InvalidateRect(hWnd, NULL, TRUE);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

static LRESULT
onWMPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    SIZE sz;
    WCHAR wbuf[BUFSZ];
    RECT rt;
    PAINTSTRUCT ps;
    PNHStatusWindow data;
    int width, height;
    RECT clear_rect;
    HDC front_buffer_hdc;

    data = (PNHStatusWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    front_buffer_hdc = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rt);

    width = rt.right - rt.left;
    height = rt.bottom - rt.top;

    back_buffer_size(&data->back_buffer, width, height);

    HDC hdc = data->back_buffer.hdc;

    SetBkColor(hdc, status_bg_color);
    SetTextColor(hdc, status_fg_color);

    clear_rect.left = 0;
    clear_rect.top = 0;
    clear_rect.right = width;
    clear_rect.bottom = height;

    FillRect(hdc, &clear_rect, status_bg_brush);

    if (data->status_lines != NULL) {
        
        data->has_blink_fields = FALSE;

        for (int line = 0; line < NHSW_LINES; line++) {
            LONG left = rt.left;
            LONG cy = 0;
            int vlen;

            mswin_status_line * status_line = &data->status_lines->lines[line];

            for (int i = 0; i < status_line->status_strings.count; i++) {
                mswin_status_string * status_string = status_line->status_strings.status_strings[i];
                int clr, atr;
                int fntatr = ATR_NONE;
                COLORREF nFg, nBg;

                if (status_string->str == NULL || status_string->str[0] == '\0')
                    continue;

                clr = status_string->color;
                atr = status_string->attribute;

                const char *str = status_string->str;

                vlen = strlen(str);

                if (atr & HL_BOLD)
                    fntatr = ATR_BOLD;
                else if (atr & HL_INVERSE)
                    fntatr = ATR_INVERSE;
                else if (atr & HL_ULINE)
                    fntatr = ATR_ULINE;
                else if (atr & HL_BLINK) {
                    data->has_blink_fields = TRUE;
                    if (data->blink_state) {
                        fntatr = ATR_INVERSE;
                        atr |= HL_INVERSE;
                    }
                }
                else if (atr & HL_DIM)
                    fntatr = ATR_DIM;

                cached_font * fnt = mswin_get_font(NHW_STATUS, fntatr, hdc, FALSE);

                BOOL useUnicode = fnt->supportsUnicode;

                winos_ascii_to_wide_str((const unsigned char *) str, wbuf, SIZE(wbuf));

                nFg = (clr == NO_COLOR ? status_fg_color
                    : ((clr >= 0 && clr < CLR_MAX) ? nhcolor_to_RGB(clr)
                        : status_fg_color));

                if (atr & HL_DIM) {
                    /* make a dim representation - this can produce color shift */
                    float redReduction = 0.5f;
                    float greenReduction = 0.5f;
                    float blueReduction = 0.25f;
                    uchar red = (uchar)(GetRValue(nFg) * (1.0f - redReduction));
                    uchar green = (uchar)(GetGValue(nFg) * (1.0f - greenReduction));
                    uchar blue = (uchar)(GetBValue(nFg) * (1.0f - blueReduction));
                    nFg = RGB(red, green, blue);
                }

                nBg = status_bg_color;

                if (status_string->space_in_front)
                    rt.left += fnt->width;

                sz.cy = -1;

                if (status_string->draw_bar && iflags.wc2_hitpointbar) {
                    /* NOTE: we current don't support bar attributes */

                    /* when we are drawing bar we need to look at the hp status
                     * field to get the correct percentage and color */
                    COLORREF bar_color = nhcolor_to_RGB(status_string->bar_color);
                    HBRUSH back_brush = CreateSolidBrush(bar_color);
                    RECT barrect;

                    /* prepare for drawing */
                    SelectObject(hdc, fnt->hFont);
                    SetBkMode(hdc, OPAQUE);
                    SetBkColor(hdc, status_bg_color);
                    SetTextColor(hdc, status_fg_color);

                    if (useUnicode) {
                        /* get bounding rectangle */
                        GetTextExtentPoint32W(hdc, wbuf, vlen, &sz);

                        /* first draw title normally */
                        DrawTextW(hdc, wbuf, vlen, &rt, DT_LEFT);
                    }
                    else {
                        /* get bounding rectangle */
                        GetTextExtentPoint32A(hdc, str, vlen, &sz);

                        /* first draw title normally */
                        DrawTextA(hdc, str, vlen, &rt, DT_LEFT);
                    }
                    int bar_percent = status_string->bar_percent;
                    if (bar_percent > 0) {
                        /* calc bar length */
                        barrect.left = rt.left;
                        barrect.top = rt.top;
                        barrect.bottom = sz.cy;
                        if (bar_percent > 0)
                            barrect.right = (int)((bar_percent * sz.cx) / 100);
                        /* else
                            barrect.right = sz.cx; */

                            /* then draw hpbar on top of title */
                        FillRect(hdc, &barrect, back_brush);
                        SetBkMode(hdc, TRANSPARENT);
                        SetTextColor(hdc, nBg);

                        if (useUnicode)
                            DrawTextW(hdc, wbuf, vlen, &barrect, DT_LEFT);
                        else
                            DrawTextA(hdc, str, vlen, &barrect, DT_LEFT);
                    }
                    DeleteObject(back_brush);
                }
                else {
                    if (atr & HL_INVERSE) {
                        COLORREF tmp = nFg;
                        nFg = nBg;
                        nBg = tmp;
                    }

                    /* prepare for drawing */
                    SelectObject(hdc, fnt->hFont);
                    SetBkMode(hdc, OPAQUE);
                    SetBkColor(hdc, nBg);
                    SetTextColor(hdc, nFg);

                    if (useUnicode) {
                        /* get bounding rectangle */
                        GetTextExtentPoint32W(hdc, wbuf, vlen, &sz);

                        /* draw */
                        DrawTextW(hdc, wbuf, vlen, &rt, DT_LEFT);
                    }
                    else {
                        /* get bounding rectangle */
                        GetTextExtentPoint32A(hdc, str, vlen, &sz);

                        /* draw */
                        DrawTextA(hdc, str, vlen, &rt, DT_LEFT);
                    }
                }
                assert(sz.cy >= 0);

                rt.left += sz.cx;
                cy = max(cy, sz.cy);
            }

            rt.left = left;
            rt.top += cy;
        }
    }

    BitBlt(front_buffer_hdc, 0, 0, width, height, hdc, 0, 0, SRCCOPY);

    EndPaint(hWnd, &ps);

    return 0;
}

void
mswin_status_window_size(HWND hWnd, LPSIZE sz)
{
    RECT rt;

    GetClientRect(hWnd, &rt);

    PNHStatusWindow data = (PNHStatusWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (data) {
        HDC hdc = GetDC(hWnd);
        cached_font * font = mswin_get_font(NHW_STATUS, ATR_NONE, hdc, FALSE);
        HGDIOBJ saveFont = SelectObject(hdc, font->hFont);

        SIZE text_sz;
        GetTextExtentPoint32(hdc, _T("W"), 1, &text_sz);

        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);

        rt.bottom = rt.top + text_sz.cy * NHSW_LINES;

        SelectObject(hdc, saveFont);
        ReleaseDC(hWnd, hdc);
    }
    AdjustWindowRect(&rt, GetWindowLong(hWnd, GWL_STYLE), FALSE);
    sz->cx = rt.right - rt.left;
    sz->cy = rt.bottom - rt.top;
}

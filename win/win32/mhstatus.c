/* NetHack 3.6	mhstatus.c	$NHDT-Date: 1536411224 2018/09/08 12:53:44 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.29 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include <assert.h>
#include "winMS.h"
#include "mhstatus.h"
#include "mhmsg.h"
#include "mhfont.h"

#define NHSW_LINES 2
#define MAXWINDOWTEXT BUFSZ

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
    int n_fields;
    const char **vals;
    boolean *activefields;
    int *percents;
    int *colors;
    back_buffer_t back_buffer;
} NHStatusWindow, *PNHStatusWindow;

static int fieldorder1[] = { BL_TITLE, BL_STR, BL_DX,    BL_CO,    BL_IN,
                             BL_WI,    BL_CH,  BL_ALIGN, BL_SCORE, -1 };
static int fieldorder2[] = { BL_LEVELDESC, BL_GOLD,      BL_HP,   BL_HPMAX,
                             BL_ENE,       BL_ENEMAX,    BL_AC,   BL_XP,
                             BL_EXP,       BL_HD,        BL_TIME, BL_HUNGER,
                             BL_CAP,       BL_CONDITION, -1 };
static int *fieldorders[] = { fieldorder1, fieldorder2, NULL };

static TCHAR szStatusWindowClass[] = TEXT("MSNHStatusWndClass");
LRESULT CALLBACK StatusWndProc(HWND, UINT, WPARAM, LPARAM);
static void register_status_window_class(void);
static LRESULT onWMPaint(HWND hWnd, WPARAM wParam, LPARAM lParam);

#define DEFAULT_COLOR_BG_STATUS COLOR_WINDOW
#define DEFAULT_COLOR_FG_STATUS COLOR_WINDOWTEXT

HWND
mswin_init_status_window()
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

    return ret;
}

void
register_status_window_class()
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
#ifdef STATUS_HILITES
            int **fop;
            int *f;

            msg_data->buffer[0] = '\0';
            if (data->n_fields > 0) {
                for (fop = fieldorders; *fop; fop++) {
                    for (f = *fop; *f != -1; f++) {
                        if (data->activefields[*f])
                            strncat(msg_data->buffer, data->vals[*f],
                                    msg_data->max_size
                                    - strlen(msg_data->buffer));
                    }
                    strncat(msg_data->buffer, "\r\n",
                            msg_data->max_size - strlen(msg_data->buffer));
                }
            }
#else
            strncpy(msg_data->buffer, data->window_text[0],
                    msg_data->max_size);
            strncat(msg_data->buffer, "\r\n",
                    msg_data->max_size - strlen(msg_data->buffer));
            strncat(msg_data->buffer, data->window_text[1],
                    msg_data->max_size - strlen(msg_data->buffer));
#endif
        } break;

        case MSNH_MSG_UPDATE_STATUS: {
            PMSNHMsgUpdateStatus msg_data = (PMSNHMsgUpdateStatus) lParam;
            data->n_fields = msg_data->n_fields;
            data->vals = msg_data->vals;
            data->activefields = msg_data->activefields;
            data->percents = msg_data->percents;
            data->colors = msg_data->colors;
            InvalidateRect(hWnd, NULL, TRUE);
        } break;
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

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

#ifdef STATUS_HILITES
static LRESULT
onWMPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    int hpbar_percent = 100;
    int hpbar_color = NO_COLOR;
    int *f;
    int **fop;
    SIZE sz;
    HGDIOBJ normalFont, boldFont;
    TCHAR wbuf[BUFSZ];
    RECT rt;
    PAINTSTRUCT ps;
    HDC hdc;
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

    hdc = data->back_buffer.hdc;

    normalFont = mswin_get_font(NHW_STATUS, ATR_NONE, hdc, FALSE);
    boldFont = mswin_get_font(NHW_STATUS, ATR_BOLD, hdc, FALSE);

    SelectObject(hdc, normalFont);

    SetBkColor(hdc, status_bg_color);
    SetTextColor(hdc, status_fg_color);

    if (iflags.wc2_hitpointbar && BL_HP < data->n_fields
        && data->activefields[BL_HP]) {
        hpbar_percent = data->percents[BL_HP];
        hpbar_color = data->colors[BL_HP] & 0x00ff;
    }

    clear_rect.left = 0;
    clear_rect.top = 0;
    clear_rect.right = width;
    clear_rect.bottom = height;

    FillRect(hdc, &clear_rect, status_bg_brush);

    for (fop = fieldorders; *fop; fop++) {
        LONG left = rt.left;
        LONG cy = 0;
        int vlen;
        for (f = *fop; *f != BL_FLUSH; f++) {
            int clr, atr;
            int fntatr = ATR_NONE;
            HGDIOBJ fnt;
            COLORREF nFg, nBg;

            if (((*f) >= data->n_fields) || (!data->activefields[*f]))
                continue;
            clr = data->colors[*f] & 0x00ff;
            atr = (data->colors[*f] & 0xff00) >> 8;
            vlen = strlen(data->vals[*f]);
            NH_A2W(data->vals[*f], wbuf, SIZE(wbuf));

            if (atr & HL_BOLD)
                fntatr = ATR_BOLD;
            else if (atr & HL_INVERSE)
                fntatr = ATR_INVERSE;
            else if (atr & HL_ULINE)
                fntatr = ATR_ULINE;
            else if (atr & HL_BLINK)
                fntatr = ATR_BLINK;
            else if (atr & HL_DIM)
                fntatr = ATR_DIM;
            fnt = mswin_get_font(NHW_STATUS, fntatr, hdc, FALSE);
            nFg = (clr == NO_COLOR ? status_fg_color
                   : ((clr >= 0 && clr < CLR_MAX) ? nhcolor_to_RGB(clr)
                      : status_fg_color));
            nBg = status_bg_color;

            sz.cy = -1;
            if (*f == BL_TITLE && iflags.wc2_hitpointbar) {
                HBRUSH back_brush = CreateSolidBrush(nhcolor_to_RGB(hpbar_color));
                RECT barrect;

                /* prepare for drawing */
                SelectObject(hdc, fnt);
                SetBkMode(hdc, OPAQUE);
                SetBkColor(hdc, status_bg_color);
                /* SetTextColor(hdc, nhcolor_to_RGB(hpbar_color)); */
				SetTextColor(hdc, status_fg_color);

                /* get bounding rectangle */
                GetTextExtentPoint32(hdc, wbuf, vlen, &sz);

                /* first draw title normally */
                DrawText(hdc, wbuf, vlen, &rt, DT_LEFT);

                if (hpbar_percent > 0) {
                    /* calc bar length */
                    barrect.left = rt.left;
                    barrect.top = rt.top;
                    barrect.bottom = sz.cy;
                    if (hpbar_percent > 0)
                        barrect.right = (int)((hpbar_percent * sz.cx) / 100);
                    /* else
                        barrect.right = sz.cx; */

                    /* then draw hpbar on top of title */
                    FillRect(hdc, &barrect, back_brush);
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, nBg);
                    DrawText(hdc, wbuf, vlen, &barrect, DT_LEFT);
                }
                DeleteObject(back_brush);
            } else {
                if (atr & HL_INVERSE) {
                    COLORREF tmp = nFg;
                    nFg = nBg;
                    nBg = tmp;
                }

                /* prepare for drawing */
                SelectObject(hdc, fnt);
                SetBkMode(hdc, OPAQUE);
                SetBkColor(hdc, nBg);
                SetTextColor(hdc, nFg);

                /* get bounding rectangle */
                GetTextExtentPoint32(hdc, wbuf, vlen, &sz);

                /* draw */
                DrawText(hdc, wbuf, vlen, &rt, DT_LEFT);
            }
            assert(sz.cy >= 0);

            rt.left += sz.cx;
            cy = max(cy, sz.cy);
        }

        rt.left = left;
        rt.top += cy;
    }

    BitBlt(front_buffer_hdc, 0, 0, width, height, hdc, 0, 0, SRCCOPY);

    EndPaint(hWnd, &ps);

    return 0;
}
#else
static LRESULT
onWMPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    int i;
    SIZE sz;
    HGDIOBJ oldFont;
    TCHAR wbuf[BUFSZ];
    COLORREF OldBg, OldFg;
    RECT rt;
    PAINTSTRUCT ps;
    HDC hdc;
    PNHStatusWindow data;

    data = (PNHStatusWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    hdc = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rt);

    oldFont =
        SelectObject(hdc, mswin_get_font(NHW_STATUS, ATR_NONE, hdc, FALSE));

    OldBg = SetBkColor(hdc, status_bg_brush ? status_bg_color
                                            : (COLORREF) GetSysColor(
                                                  DEFAULT_COLOR_BG_STATUS));
    OldFg = SetTextColor(hdc, status_fg_brush ? status_fg_color
                                              : (COLORREF) GetSysColor(
                                                    DEFAULT_COLOR_FG_STATUS));

    for (i = 0; i < NHSW_LINES; i++) {
        int wlen = strlen(data->window_text[i]);
        NH_A2W(data->window_text[i], wbuf, SIZE(wbuf));
        GetTextExtentPoint32(hdc, wbuf, wlen, &sz);
        DrawText(hdc, wbuf, wlen, &rt, DT_LEFT | DT_END_ELLIPSIS);
        rt.top += sz.cy;
    }

    SelectObject(hdc, oldFont);
    SetTextColor(hdc, OldFg);
    SetBkColor(hdc, OldBg);
    EndPaint(hWnd, &ps);

    return 0;
}
#endif /* !STATUS_HILITES */

void
mswin_status_window_size(HWND hWnd, LPSIZE sz)
{
    TEXTMETRIC tm;
    HGDIOBJ saveFont;
    HDC hdc;
    PNHStatusWindow data;
    RECT rt;
    SIZE text_sz;

    GetClientRect(hWnd, &rt);

    data = (PNHStatusWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (data) {
        hdc = GetDC(hWnd);
        saveFont = SelectObject(
            hdc, mswin_get_font(NHW_STATUS, ATR_NONE, hdc, FALSE));
        GetTextExtentPoint32(hdc, _T("W"), 1, &text_sz);
        GetTextMetrics(hdc, &tm);

        rt.bottom = rt.top + text_sz.cy * NHSW_LINES;

        SelectObject(hdc, saveFont);
        ReleaseDC(hWnd, hdc);
    }
    AdjustWindowRect(&rt, GetWindowLong(hWnd, GWL_STYLE), FALSE);
    sz->cx = rt.right - rt.left;
    sz->cy = rt.bottom - rt.top;
}

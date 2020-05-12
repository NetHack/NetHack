/* NetHack 3.6	mhmsgwnd.c	$NHDT-Date: 1432512802 2015/05/25 00:13:22 $  $NHDT-Branch: master $:$NHDT-Revision: 1.20 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "mhmsgwnd.h"
#include "mhmsg.h"
#include "mhcmd.h"
#include "mhfont.h"
#include "mhcolor.h"

#define MSG_WRAP_TEXT

#define MSG_VISIBLE_LINES max(iflags.wc_vary_msgcount, 2)
#define MAX_MSG_LINES 32
#define MSG_LINES (int) min(iflags.msg_history, MAX_MSG_LINES)
#define MAXWINDOWTEXT 200

struct window_line {
    int attr;
    char text[MAXWINDOWTEXT];
};

typedef struct mswin_nethack_message_window {
    size_t max_text;
    struct window_line window_text[MAX_MSG_LINES];

    int xChar;           /* horizontal scrolling unit */
    int yChar;           /* vertical scrolling unit */
    int xUpper;          /* average width of uppercase letters */
    int xPos;            /* current horizontal scrolling position */
    int yPos;            /* current vertical scrolling position */
    int xMax;            /* maximum horizontal scrolling position */
    int yMax;            /* maximum vertical scrolling position */
    int xPage;           /* page size of horizontal scroll bar */
    int lines_last_turn; /* lines added during the last turn */
    int dont_care; /* flag the user does not care if messages are lost */
} NHMessageWindow, *PNHMessageWindow;

static TCHAR szMessageWindowClass[] = TEXT("MSNHMessageWndClass");
LRESULT CALLBACK NHMessageWndProc(HWND, UINT, WPARAM, LPARAM);
static void register_message_window_class();
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onMSNH_VScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
#ifndef MSG_WRAP_TEXT
static void onMSNH_HScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
#endif
static COLORREF setMsgTextColor(HDC hdc, int gray);
static void onPaint(HWND hWnd);
static void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);

#ifdef USER_SOUNDS
extern void play_sound_for_message(const char *str);
#endif

HWND
mswin_init_message_window()
{
    static int run_once = 0;
    HWND ret;
    DWORD style;

    if (!run_once) {
        register_message_window_class();
        run_once = 1;
    }

#ifdef MSG_WRAP_TEXT
    style = WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL;
#else
    style = WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | WS_HSCROLL;
#endif

    ret = CreateWindow(
        szMessageWindowClass, /* registered class name */
        NULL,                 /* window name */
        style,                /* window style */
        0,                    /* horizontal position of window */
        0,                    /* vertical position of window */
        0,                    /* window width */
        0,                    /* window height - set it later */
        GetNHApp()->hMainWnd, /* handle to parent or owner window */
        NULL,                 /* menu handle or child identifier */
        GetNHApp()->hApp,     /* handle to application instance */
        NULL);                /* window-creation data */

    if (!ret)
        panic("Cannot create message window");

    return ret;
}

void
register_message_window_class()
{
    WNDCLASS wcex;
    ZeroMemory(&wcex, sizeof(wcex));

    wcex.style = CS_NOCLOSE;
    wcex.lpfnWndProc = (WNDPROC) NHMessageWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetNHApp()->hApp;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = mswin_get_brush(NHW_MESSAGE, MSWIN_COLOR_BG);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szMessageWindowClass;

    RegisterClass(&wcex);
}

LRESULT CALLBACK
NHMessageWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        onCreate(hWnd, wParam, lParam);
        break;

    case WM_MSNH_COMMAND:
        onMSNHCommand(hWnd, wParam, lParam);
        break;

    case WM_PAINT:
        onPaint(hWnd);
        break;

    case WM_SETFOCUS:
        SetFocus(GetNHApp()->hMainWnd);
        break;

#ifndef MSG_WRAP_TEXT
    case WM_HSCROLL:
        onMSNH_HScroll(hWnd, wParam, lParam);
        break;
#endif

    case WM_VSCROLL:
        onMSNH_VScroll(hWnd, wParam, lParam);
        break;

    case WM_DESTROY: {
        PNHMessageWindow data;
        data = (PNHMessageWindow) GetWindowLong(hWnd, GWL_USERDATA);
        free(data);
        SetWindowLong(hWnd, GWL_USERDATA, (LONG) 0);
    } break;

    case WM_SIZE: {
        SCROLLINFO si;
        int xNewSize;
        int yNewSize;
        PNHMessageWindow data;

        data = (PNHMessageWindow) GetWindowLong(hWnd, GWL_USERDATA);

        xNewSize = LOWORD(lParam);
        yNewSize = HIWORD(lParam);

        if (xNewSize > 0 || yNewSize > 0) {
#ifndef MSG_WRAP_TEXT
            data->xPage = xNewSize / data->xChar;
            data->xMax = max(0, (int) (1 + data->max_text - data->xPage));
            data->xPos = min(data->xPos, data->xMax);

            ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si);
            si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
            si.nMin = 0;
            si.nMax = data->max_text;
            si.nPage = data->xPage;
            si.nPos = data->xPos;
            SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
#endif

            data->yMax = MSG_LINES - 1;
            data->yPos = min(data->yPos, data->yMax);

            ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si);
            si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
            si.nMin = MSG_VISIBLE_LINES;
            si.nMax = data->yMax + MSG_VISIBLE_LINES - 1;
            si.nPage = MSG_VISIBLE_LINES;
            si.nPos = data->yPos;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
        }
    } break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void
onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHMessageWindow data;

    data = (PNHMessageWindow) GetWindowLong(hWnd, GWL_USERDATA);
    switch (wParam) {
    case MSNH_MSG_PUTSTR: {
        PMSNHMsgPutstr msg_data = (PMSNHMsgPutstr) lParam;
        SCROLLINFO si;
        char *p;

        if (msg_data->append) {
            strncat(data->window_text[MSG_LINES - 1].text, msg_data->text,
                    MAXWINDOWTEXT
                        - strlen(data->window_text[MSG_LINES - 1].text));
        } else {
            /* check if the string is empty */
            for (p = data->window_text[MSG_LINES - 1].text; *p && isspace(*p);
                 p++)
                ;

            if (*p) {
                /* last string is not empty - scroll up */
                memmove(&data->window_text[0], &data->window_text[1],
                        (MSG_LINES - 1) * sizeof(data->window_text[0]));
            }

            /* append new text to the end of the array */
            data->window_text[MSG_LINES - 1].attr = msg_data->attr;
            strncpy(data->window_text[MSG_LINES - 1].text, msg_data->text,
                    MAXWINDOWTEXT);
        }

        /* reset V-scroll position to display new text */
        data->yPos = data->yMax;

        ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        si.nPos = data->yPos;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

        /* deal with overflows */
        data->lines_last_turn++;
        if (!data->dont_care && data->lines_last_turn >= MSG_LINES - 2) {
            char c;
            BOOL done;

            /* append "--More--" to the message window text (cannot call
               putstr
               here - infinite recursion) */
            memmove(&data->window_text[0], &data->window_text[1],
                    (MSG_LINES - 1) * sizeof(data->window_text[0]));
            data->window_text[MSG_LINES - 1].attr = ATR_NONE;
            strncpy(data->window_text[MSG_LINES - 1].text, "--More--",
                    MAXWINDOWTEXT);

            /* update window content */
            InvalidateRect(hWnd, NULL, TRUE);

#if defined(WIN_CE_SMARTPHONE)
            NHSPhoneSetKeypadFromString("\033- <>");
#endif

            done = FALSE;
            while (!done) {
                int x, y, mod;
                c = mswin_nh_poskey(&x, &y, &mod);
                switch (c) {
                /* ESC indicates that we can safely discard any further
                 * messages during this turn */
                case '\033':
                    data->dont_care = 1;
                    done = TRUE;
                    break;

                case '<':
                    SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0),
                                (LPARAM) NULL);
                    break;

                case '>':
                    SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0),
                                (LPARAM) NULL);
                    break;

                /* continue scrolling on any key */
                default:
                    data->lines_last_turn = 0;
                    done = TRUE;
                    break;
                }
            }

#if defined(WIN_CE_SMARTPHONE)
            NHSPhoneSetKeypadDefault();
#endif
            /* remove "--More--" from the message window text */
            data->window_text[MSG_LINES - 1].attr = ATR_NONE;
            strncpy(data->window_text[MSG_LINES - 1].text, " ",
                    MAXWINDOWTEXT);
        }

        /* update window content */
        InvalidateRect(hWnd, NULL, TRUE);

#ifdef USER_SOUNDS
        play_sound_for_message(msg_data->text);
#endif
    } break;

    case MSNH_MSG_CLEAR_WINDOW:
        data->lines_last_turn = 0;
        data->dont_care = 0;
        break;
    }
}

void
onMSNH_VScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHMessageWindow data;
    SCROLLINFO si;
    int yInc;

    /* get window data */
    data = (PNHMessageWindow) GetWindowLong(hWnd, GWL_USERDATA);

    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_POS;
    GetScrollInfo(hWnd, SB_VERT, &si);

    switch (LOWORD(wParam)) {
    // User clicked the shaft above the scroll box.

    case SB_PAGEUP:
        yInc = -(int) si.nPage;
        break;

    // User clicked the shaft below the scroll box.

    case SB_PAGEDOWN:
        yInc = si.nPage;
        break;

    // User clicked the top arrow.

    case SB_LINEUP:
        yInc = -1;
        break;

    // User clicked the bottom arrow.

    case SB_LINEDOWN:
        yInc = 1;
        break;

    // User dragged the scroll box.

    case SB_THUMBTRACK:
        yInc = HIWORD(wParam) - data->yPos;
        break;

    default:
        yInc = 0;
    }

    // If applying the vertical scrolling increment does not
    // take the scrolling position out of the scrolling range,
    // increment the scrolling position, adjust the position
    // of the scroll box, and update the window. UpdateWindow
    // sends the WM_PAINT message.

    if (yInc = max(MSG_VISIBLE_LINES - data->yPos,
                   min(yInc, data->yMax - data->yPos))) {
        data->yPos += yInc;
        /* ScrollWindowEx(hWnd, 0, -data->yChar * yInc,
                (CONST RECT *) NULL, (CONST RECT *) NULL,
                (HRGN) NULL, (LPRECT) NULL, SW_INVALIDATE | SW_ERASE);
        */
        InvalidateRect(hWnd, NULL, TRUE);

        ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        si.nPos = data->yPos;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

        UpdateWindow(hWnd);
    }
}

#ifndef MSG_WRAP_TEXT
void
onMSNH_HScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHMessageWindow data;
    SCROLLINFO si;
    int xInc;

    /* get window data */
    data = (PNHMessageWindow) GetWindowLong(hWnd, GWL_USERDATA);

    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE;
    GetScrollInfo(hWnd, SB_HORZ, &si);

    switch (LOWORD(wParam)) {
    // User clicked shaft left of the scroll box.

    case SB_PAGEUP:
        xInc = -(int) si.nPage;
        break;

    // User clicked shaft right of the scroll box.

    case SB_PAGEDOWN:
        xInc = si.nPage;
        break;

    // User clicked the left arrow.

    case SB_LINEUP:
        xInc = -1;
        break;

    // User clicked the right arrow.

    case SB_LINEDOWN:
        xInc = 1;
        break;

    // User dragged the scroll box.

    case SB_THUMBTRACK:
        xInc = HIWORD(wParam) - data->xPos;
        break;

    default:
        xInc = 0;
    }

    // If applying the horizontal scrolling increment does not
    // take the scrolling position out of the scrolling range,
    // increment the scrolling position, adjust the position
    // of the scroll box, and update the window.

    if (xInc = max(-data->xPos, min(xInc, data->xMax - data->xPos))) {
        data->xPos += xInc;
        ScrollWindowEx(hWnd, -data->xChar * xInc, 0, (CONST RECT *) NULL,
                       (CONST RECT *) NULL, (HRGN) NULL, (LPRECT) NULL,
                       SW_INVALIDATE | SW_ERASE);

        ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        si.nPos = data->xPos;
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
        UpdateWindow(hWnd);
    }
}
#endif // MSG_WRAP_TEXT

COLORREF
setMsgTextColor(HDC hdc, int gray)
{
    COLORREF fg, color1, color2;
    if (gray) {
        color1 = mswin_get_color(NHW_MESSAGE, MSWIN_COLOR_BG);
        color2 = mswin_get_color(NHW_MESSAGE, MSWIN_COLOR_FG);
        /* Make a "gray" color by taking the average of the individual R,G,B
           components of two colors. Thanks to Jonathan del Strother */
        fg = RGB((GetRValue(color1) + GetRValue(color2)) / 2,
                 (GetGValue(color1) + GetGValue(color2)) / 2,
                 (GetBValue(color1) + GetBValue(color2)) / 2);
    } else {
        fg = mswin_get_color(NHW_MESSAGE, MSWIN_COLOR_FG);
    }

    return SetTextColor(hdc, fg);
}

void
onPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    PNHMessageWindow data;
    RECT client_rt, draw_rt;
    int FirstLine, LastLine;
    int i, x, y;
    HGDIOBJ oldFont;
    TCHAR wbuf[MAXWINDOWTEXT + 2];
    size_t wlen;
    COLORREF OldBg, OldFg;

    hdc = BeginPaint(hWnd, &ps);

    OldBg = SetBkColor(hdc, mswin_get_color(NHW_MESSAGE, MSWIN_COLOR_BG));
    OldFg = setMsgTextColor(hdc, 0);

    data = (PNHMessageWindow) GetWindowLong(hWnd, GWL_USERDATA);

    GetClientRect(hWnd, &client_rt);

    if (!IsRectEmpty(&ps.rcPaint)) {
        FirstLine = max(
            0, data->yPos - (client_rt.bottom - ps.rcPaint.top) / data->yChar
                   + 1);
        LastLine =
            min(MSG_LINES - 1,
                data->yPos
                    - (client_rt.bottom - ps.rcPaint.bottom) / data->yChar);
        y = min(ps.rcPaint.bottom, client_rt.bottom - 2);
        for (i = LastLine; i >= FirstLine; i--) {
            if (i == MSG_LINES - 1) {
                x = data->xChar * (2 - data->xPos);
            } else {
                x = data->xChar * (4 - data->xPos);
            }

            if (strlen(data->window_text[i].text) > 0) {
                /* convert to UNICODE */
                NH_A2W(data->window_text[i].text, wbuf, sizeof(wbuf));
                wlen = _tcslen(wbuf);

                /* calculate text height */
                draw_rt.left = x;
                draw_rt.right = client_rt.right;
                draw_rt.top = y - data->yChar;
                draw_rt.bottom = y;

                oldFont = SelectObject(
                    hdc,
                    mswin_get_font(NHW_MESSAGE, data->window_text[i].attr,
                                   hdc, FALSE));

                setMsgTextColor(hdc, i < (MSG_LINES - data->lines_last_turn));
#ifdef MSG_WRAP_TEXT
                DrawText(hdc, wbuf, wlen, &draw_rt,
                         DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
                draw_rt.top = y - (draw_rt.bottom - draw_rt.top);
                draw_rt.bottom = y;
                DrawText(hdc, wbuf, wlen, &draw_rt,
                         DT_NOPREFIX | DT_WORDBREAK);
#else
                DrawText(hdc, wbuf, wlen, &draw_rt, DT_NOPREFIX);
#endif
                SelectObject(hdc, oldFont);

                y -= draw_rt.bottom - draw_rt.top;
            } else {
                y -= data->yChar;
            }
        }
    }

    SetTextColor(hdc, OldFg);
    SetBkColor(hdc, OldBg);
    EndPaint(hWnd, &ps);
}

void
onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    TEXTMETRIC tm;
    PNHMessageWindow data;
    HGDIOBJ saveFont;

    /* set window data */
    data = (PNHMessageWindow) malloc(sizeof(NHMessageWindow));
    if (!data)
        panic("out of memory");
    ZeroMemory(data, sizeof(NHMessageWindow));
    data->max_text = MAXWINDOWTEXT;
    SetWindowLong(hWnd, GWL_USERDATA, (LONG) data);

    /* Get the handle to the client area's device context. */
    hdc = GetDC(hWnd);
    saveFont =
        SelectObject(hdc, mswin_get_font(NHW_MESSAGE, ATR_NONE, hdc, FALSE));

    /* Extract font dimensions from the text metrics. */
    GetTextMetrics(hdc, &tm);
    data->xChar = tm.tmAveCharWidth;
    data->xUpper = (tm.tmPitchAndFamily & 1 ? 3 : 2) * data->xChar / 2;
    data->yChar = tm.tmHeight + tm.tmExternalLeading;
    data->xPage = 1;

    /* Free the device context.  */
    SelectObject(hdc, saveFont);
    ReleaseDC(hWnd, hdc);

    /* create command pad (keyboard emulator) */
    if (!GetNHApp()->hCmdWnd)
        GetNHApp()->hCmdWnd = mswin_init_command_window();
}

void
mswin_message_window_size(HWND hWnd, LPSIZE sz)
{
    PNHMessageWindow data;
    RECT rt, client_rt;

    GetWindowRect(hWnd, &rt);

    sz->cx = rt.right - rt.left;
    sz->cy = rt.bottom - rt.top;

    data = (PNHMessageWindow) GetWindowLong(hWnd, GWL_USERDATA);
    if (data) {
        /* set size to accomodate MSG_VISIBLE_LINES, highligh rectangle and
           horizontal scroll bar (difference between window rect and client
           rect */
        GetClientRect(hWnd, &client_rt);
        sz->cy = sz->cy - (client_rt.bottom - client_rt.top)
                 + data->yChar * MSG_VISIBLE_LINES + 4;
    }
}

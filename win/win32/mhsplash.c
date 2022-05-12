/* NetHack 3.7	mhsplash.c	$NHDT-Date: 1596498360 2020/08/03 23:46:00 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.36 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "win10.h"
#include "winMS.h"
#include "resource.h"
#include "mhsplash.h"
#include "mhmsg.h"
#include "mhfont.h"
#include "config.h"
#if !defined(VERSION_MAJOR)
#include "patchlevel.h"
#endif
#include "dlb.h"

#define LLEN 128

PNHWinApp GetNHApp(void);

INT_PTR CALLBACK NHSplashWndProc(HWND, UINT, WPARAM, LPARAM);

#define SPLASH_WIDTH_96DPI 440
#define SPLASH_HEIGHT_96DPI 322
#define SPLASH_OFFSET_X_96DPI 10
#define SPLASH_OFFSET_Y_96DPI 10
#define SPLASH_VERSION_X_96DPI 280
#define SPLASH_VERSION_Y_96DPI 0

typedef struct {
    int boarder_width;
    int boarder_height;
    int client_width;
    int client_height;
    int ok_control_width;
    int ok_control_height;
    int ok_control_offset_x;
    int ok_control_offset_y;
    int text_control_width;
    int text_control_height;
    int text_control_offset_x;
    int text_control_offset_y;
    int window_width;
    int window_height;
    int width;
    int height;
    int offset_x;
    int offset_y;
    int version_x;
    int version_y;
    HFONT hFont;
} SplashData;

static void
mswin_set_splash_data(HWND hWnd, SplashData * sd, double scale)
{
    RECT client_rect;
    RECT window_rect;
    RECT ok_control_rect;
    RECT text_control_rect;

    GetClientRect(hWnd, &client_rect);
    GetWindowRect(hWnd, &window_rect);
    GetWindowRect(GetDlgItem(hWnd, IDOK), &ok_control_rect);
    GetWindowRect(GetDlgItem(hWnd, IDC_EXTRAINFO), &text_control_rect);

    sd->boarder_width = (window_rect.right - window_rect.left) -
                                (client_rect.right - client_rect.left);
    sd->boarder_height = (window_rect.bottom - window_rect.top) -
        (client_rect.bottom - client_rect.top);

    sd->ok_control_width = ok_control_rect.right - ok_control_rect.left;
    sd->ok_control_height = ok_control_rect.bottom - ok_control_rect.top;

    sd->width = (int)(scale * SPLASH_WIDTH_96DPI);
    sd->height = (int)(scale * SPLASH_HEIGHT_96DPI);
    sd->offset_x = (int)(scale * SPLASH_OFFSET_X_96DPI);
    sd->offset_y = (int)(scale * SPLASH_OFFSET_Y_96DPI);
    sd->version_x = (int)(scale * SPLASH_VERSION_X_96DPI);
    sd->version_y = (int)(scale * SPLASH_VERSION_Y_96DPI);

    sd->client_width = sd->width + sd->offset_x * 2;
    sd->client_height = sd->height + sd->ok_control_height +
        sd->offset_y * 3;

    sd->window_width = sd->client_width + sd->boarder_width;
    sd->window_height = sd->client_height + sd->boarder_height;

    sd->ok_control_offset_x = (sd->client_width - sd->ok_control_width) / 2;
    sd->ok_control_offset_y = sd->client_height - sd->ok_control_height - sd->offset_y;

    sd->text_control_width = sd->client_width - sd->offset_x * 2;
    sd->text_control_height = text_control_rect.bottom - text_control_rect.top;

    sd->text_control_offset_x = sd->offset_x;
    sd->text_control_offset_y = sd->ok_control_offset_y - sd->offset_y -
                               sd->text_control_height;

    if (sd->hFont != NULL)
        DeleteObject(sd->hFont);

    sd->hFont = mswin_create_splashfont(hWnd);

    MoveWindow(hWnd, window_rect.left, window_rect.top,
        sd->window_width, sd->window_height, TRUE);

    MoveWindow(GetDlgItem(hWnd, IDOK),
        sd->ok_control_offset_x, sd->ok_control_offset_y,
        sd->ok_control_width, sd->ok_control_height, TRUE);

    MoveWindow(GetDlgItem(hWnd, IDC_EXTRAINFO),
        sd->text_control_offset_x, sd->text_control_offset_y,
        sd->text_control_width, sd->text_control_height, TRUE);

}

void
mswin_display_splash_window(BOOL show_ver)
{
    MSG msg;
    strbuf_t strbuf;

    strbuf_init(&strbuf);

    HWND hWnd = CreateDialog(GetNHApp()->hApp, MAKEINTRESOURCE(IDD_SPLASH),
                        GetNHApp()->hMainWnd, NHSplashWndProc);
    if (!hWnd)
        panic("Cannot create Splash window");

    MonitorInfo monitorInfo;
    win10_monitor_info(hWnd, &monitorInfo);

    SplashData splashData;
    memset(&splashData, 0, sizeof(splashData));
    mswin_set_splash_data(hWnd, &splashData, monitorInfo.scale);
 
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) &splashData);
    
    GetNHApp()->hPopupWnd = hWnd;

    int left = monitorInfo.left + (monitorInfo.width - splashData.window_width) / 2;
    int top = monitorInfo.top + (monitorInfo.height - splashData.window_height) / 2;
    MoveWindow(hWnd, left, top, splashData.window_width, splashData.window_height, TRUE);

    /* Fill the text control */
    strbuf_reserve(&strbuf, BUFSIZ);
    Sprintf(strbuf.str, "%s\n%s\n%s\n%s\n\n", COPYRIGHT_BANNER_A,
            COPYRIGHT_BANNER_B, nomakedefs.copyright_banner_c, COPYRIGHT_BANNER_D);

    if (show_ver) {
        /* Show complete version information */
        dlb *f;
        char verbuf[BUFSZ];
        /* int verstrsize = 0; */

        getversionstring(verbuf, sizeof verbuf);
        strbuf_append(&strbuf, verbuf);
        strbuf_append(&strbuf, "\n\n");
            
        /* Add compile options */
        f = dlb_fopen(OPTIONS_USED, RDTMODE);
        if (f) {
            char line[LLEN + 1];

            while (dlb_fgets(line, LLEN, f))
                strbuf_append(&strbuf, line);
            (void) dlb_fclose(f);
        }
    } else {
        /* Show news, if any */
        if (iflags.news) {
            FILE *nf;

            iflags.news = 0; /* prevent newgame() from re-displaying news */
            /* BUG: this relies on current working directory */
            nf = fopen(NEWS, "r");
            if (nf != NULL) {
                char line[LLEN + 1];

                while (fgets(line, LLEN, nf))
                    strbuf_append(&strbuf, line);
                (void) fclose(nf);
            } else {
                strbuf_append(&strbuf, "No news.");
            }
        }
    }

    strbuf_nl_to_crlf(&strbuf);
    SetWindowText(GetDlgItem(hWnd, IDC_EXTRAINFO), strbuf.str);
    strbuf_empty(&strbuf);
    ShowWindow(hWnd, SW_SHOW);

    while (IsWindow(hWnd) && GetMessage(&msg, NULL, 0, 0) != 0) {
        if (!IsDialogMessage(hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    GetNHApp()->hPopupWnd = NULL;
    DeleteObject(splashData.hFont);
}

INT_PTR CALLBACK
NHSplashWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message) {
    case WM_INITDIALOG: {
        HDC hdc = GetDC(hWnd);
        cached_font * font = mswin_get_font(NHW_TEXT, ATR_NONE, hdc, FALSE);
        /* set text control font */
        SendMessage(hWnd, WM_SETFONT, (WPARAM)font->hFont,  0);
        ReleaseDC(hWnd, hdc);

        SetFocus(GetDlgItem(hWnd, IDOK));
    } break;

    case WM_PAINT: {
        char VersionString[BUFSZ];
        RECT rt;
        HDC hdcBitmap;
        HANDLE OldBitmap;
        HANDLE OldFont;
        PAINTSTRUCT ps;

        SplashData *splashData = (SplashData *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

        HDC hdc = BeginPaint(hWnd, &ps);
        /* Show splash graphic */

        hdcBitmap = CreateCompatibleDC(hdc);
        SetBkMode(hdc, OPAQUE);
        OldBitmap = SelectObject(hdcBitmap, GetNHApp()->bmpSplash);
        (*GetNHApp()->lpfnTransparentBlt)(hdc,
            splashData->offset_x, splashData->offset_y,
            splashData->width, splashData->height, hdcBitmap,
            0, 0, SPLASH_WIDTH_96DPI, SPLASH_HEIGHT_96DPI,
            TILE_BK_COLOR);

        SelectObject(hdcBitmap, OldBitmap);
        DeleteDC(hdcBitmap);

        SetBkMode(hdc, TRANSPARENT);
        /* Print version number */

        SetTextColor(hdc, RGB(0, 0, 0));
        rt.right = rt.left = splashData->offset_x + splashData->version_x;
        rt.bottom = rt.top = splashData->offset_y + splashData->version_y;
        Sprintf(VersionString, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR,
                PATCHLEVEL);
        OldFont = SelectObject(hdc, splashData->hFont);
        DrawText(hdc, VersionString, strlen(VersionString), &rt,
                 DT_LEFT | DT_NOPREFIX | DT_CALCRECT);
        DrawText(hdc, VersionString, strlen(VersionString), &rt,
                 DT_LEFT | DT_NOPREFIX);
        EndPaint(hWnd, &ps);
    } break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDOK:
            mswin_window_mark_dead(mswin_winid_from_handle(hWnd));
            if (GetNHApp()->hMainWnd == hWnd)
                GetNHApp()->hMainWnd = NULL;
            DestroyWindow(hWnd);
            SetFocus(GetNHApp()->hMainWnd);
            return TRUE;
        }
        break;

    case WM_DPICHANGED: {
        SplashData *splashData = (SplashData *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

        MonitorInfo monitorInfo;
        win10_monitor_info(hWnd, &monitorInfo);

        mswin_set_splash_data(hWnd, splashData, monitorInfo.scale);

        InvalidateRect(hWnd, NULL, TRUE);
    } break;

    }
    return FALSE;
}

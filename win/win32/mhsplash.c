/* NetHack 3.6	mhsplash.c	$NHDT-Date: 1449751714 2015/12/10 12:48:34 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.27 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "win10.h"
#include "winMS.h"
#include "resource.h"
#include "mhsplash.h"
#include "mhmsg.h"
#include "mhfont.h"
#include "date.h"
#include "patchlevel.h"
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

extern HFONT version_splash_font;

typedef struct {
    int width;
    int height;
    int offsetX;
    int offsetY;
    int versionX;
    int versionY;
} SplashData;

void
mswin_display_splash_window(BOOL show_ver)
{
    MSG msg;
    int left, top;
    RECT splashrt;
    RECT clientrt;
    RECT controlrt;
    int buttop;
    strbuf_t strbuf;

    strbuf_init(&strbuf);

    HWND hWnd = CreateDialog(GetNHApp()->hApp, MAKEINTRESOURCE(IDD_SPLASH),
                        GetNHApp()->hMainWnd, NHSplashWndProc);
    if (!hWnd)
        panic("Cannot create Splash window");

    MonitorInfo monitorInfo;
    win10_monitor_info(hWnd, &monitorInfo);
    
    SplashData splashData;

    splashData.width = (int) (monitorInfo.scale * SPLASH_WIDTH_96DPI);
    splashData.height = (int) (monitorInfo.scale * SPLASH_HEIGHT_96DPI);
    splashData.offsetX = (int) (monitorInfo.scale * SPLASH_OFFSET_X_96DPI);
    splashData.offsetY = (int) (monitorInfo.scale * SPLASH_OFFSET_Y_96DPI);
    splashData.versionX = (int) (monitorInfo.scale * SPLASH_VERSION_X_96DPI);
    splashData.versionY = (int) (monitorInfo.scale * SPLASH_VERSION_Y_96DPI);
 
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) &splashData);
    
    mswin_init_splashfonts(hWnd);
    GetNHApp()->hPopupWnd = hWnd;
    /* Get control size */
    GetWindowRect(GetDlgItem(hWnd, IDOK), &controlrt);
    controlrt.right -= controlrt.left;
    controlrt.bottom -= controlrt.top;
    /* Get current client area */
    GetClientRect(hWnd, &clientrt);
    /* Get window size */
    GetWindowRect(hWnd, &splashrt);
    splashrt.right -= splashrt.left;
    splashrt.bottom -= splashrt.top;
    /* Get difference between requested client area and current value */
    splashrt.right += splashData.width + splashData.offsetX * 2
                   - clientrt.right;
    splashrt.bottom += splashData.height + controlrt.bottom 
                    + splashData.offsetY * 3
                    - clientrt.bottom;
    /* Place the window centered */
    /* On the screen, not on the parent window */
    left = (monitorInfo.width - splashrt.right) / 2;
    top = (monitorInfo.height - splashrt.bottom) / 2;
    MoveWindow(hWnd, left, top, splashrt.right, splashrt.bottom, TRUE);
    /* Place the OK control */
    GetClientRect(hWnd, &clientrt);
    MoveWindow(GetDlgItem(hWnd, IDOK),
               (clientrt.right - clientrt.left - controlrt.right) / 2,
               clientrt.bottom - controlrt.bottom - splashData.offsetY,
               controlrt.right, controlrt.bottom, TRUE);
    buttop = clientrt.bottom - controlrt.bottom - splashData.offsetY;
    /* Place the text control */
    GetWindowRect(GetDlgItem(hWnd, IDC_EXTRAINFO), &controlrt);
    controlrt.right -= controlrt.left;
    controlrt.bottom -= controlrt.top;
    GetClientRect(hWnd, &clientrt);
    MoveWindow(GetDlgItem(hWnd, IDC_EXTRAINFO),
        clientrt.left + splashData.offsetX,
        buttop - controlrt.bottom - splashData.offsetY,
        clientrt.right - 2 * splashData.offsetX, controlrt.bottom, TRUE);

    /* Fill the text control */
    strbuf_reserve(&strbuf, BUFSIZ);
    Sprintf(strbuf.str, "%s\n%s\n%s\n%s\n\n", COPYRIGHT_BANNER_A,
            COPYRIGHT_BANNER_B, COPYRIGHT_BANNER_C, COPYRIGHT_BANNER_D);

    if (show_ver) {
        /* Show complete version information */
        dlb *f;
        char verbuf[BUFSZ];
        int verstrsize = 0;
 
        getversionstring(verbuf);
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
    mswin_destroy_splashfonts();
}

INT_PTR CALLBACK
NHSplashWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;

    UNREFERENCED_PARAMETER(lParam);

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

    case WM_PAINT: {
        char VersionString[BUFSZ];
        RECT rt;
        HDC hdcBitmap;
        HANDLE OldBitmap;
        HANDLE OldFont;
        PAINTSTRUCT ps;

        SplashData *splashData = (SplashData *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

        hdc = BeginPaint(hWnd, &ps);
        /* Show splash graphic */

        hdcBitmap = CreateCompatibleDC(hdc);
        SetBkMode(hdc, OPAQUE);
        OldBitmap = SelectObject(hdcBitmap, GetNHApp()->bmpSplash);
        (*GetNHApp()->lpfnTransparentBlt)(hdc,
            splashData->offsetX, splashData->offsetY,
            splashData->width, splashData->height, hdcBitmap,
            0, 0, SPLASH_WIDTH_96DPI, SPLASH_HEIGHT_96DPI,
            TILE_BK_COLOR);

        SelectObject(hdcBitmap, OldBitmap);
        DeleteDC(hdcBitmap);

        SetBkMode(hdc, TRANSPARENT);
        /* Print version number */

        SetTextColor(hdc, RGB(0, 0, 0));
        rt.right = rt.left = splashData->offsetX + splashData->versionX;
        rt.bottom = rt.top = splashData->offsetY + splashData->versionY;
        Sprintf(VersionString, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR,
                PATCHLEVEL);
        OldFont = SelectObject(hdc, version_splash_font);
        DrawText(hdc, VersionString, strlen(VersionString), &rt,
                 DT_LEFT | DT_NOPREFIX | DT_CALCRECT);
        DrawText(hdc, VersionString, strlen(VersionString), &rt,
                 DT_LEFT | DT_NOPREFIX);
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
    }
    return FALSE;
}

/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "resource.h"
#include "mhsplash.h"
#include "mhmsg.h"
#include "mhfont.h"
#include "patchlevel.h"

PNHWinApp GetNHApp(void);

BOOL CALLBACK NHSplashWndProc(HWND, UINT, WPARAM, LPARAM);

#define SPLASH_WIDTH		440
#define SPLASH_HEIGHT		240
#define SPLASH_VERSION_X		290
#define SPLASH_VERSION_Y		10
#define SPLASH_EXTRA_X_BEGIN	15
#define SPLASH_EXTRA_X_END	415
#define SPLASH_EXTRA_Y		150
#define SPLASH_OFFSET_X		10
#define SPLASH_OFFSET_Y		10

extern HFONT version_splash_font;
extern HFONT extrainfo_splash_font;

void mswin_display_splash_window ()
{
	MSG msg;
	RECT rt;
	HWND mapWnd;
	RECT splashrt;
	RECT clientrt;
	RECT controlrt;
	HWND hWnd;

	hWnd = CreateDialog(GetNHApp()->hApp, MAKEINTRESOURCE(IDD_SPLASH),
	    GetNHApp()->hMainWnd, NHSplashWndProc);
	if( !hWnd ) panic("Cannot create Splash window");
	mswin_init_splashfonts(hWnd);
	GetNHApp()->hPopupWnd = hWnd;
	mapWnd = mswin_hwnd_from_winid(WIN_MAP);
	if( !IsWindow(mapWnd) ) mapWnd = GetNHApp()->hMainWnd;
	/* Get control size */
	GetWindowRect (GetDlgItem(hWnd, IDOK), &controlrt);
	controlrt.right -= controlrt.left;
	controlrt.bottom -= controlrt.top;
	/* Get current client area */
	GetClientRect (hWnd, &clientrt);
	/* Get window size */
	GetWindowRect(hWnd, &splashrt);
	splashrt.right -= splashrt.left;
	splashrt.bottom -= splashrt.top;
	/* Get difference between requested client area and current value */
	splashrt.right += SPLASH_WIDTH + SPLASH_OFFSET_X * 2 - clientrt.right;
	splashrt.bottom += SPLASH_HEIGHT + controlrt.bottom + SPLASH_OFFSET_Y * 3 - clientrt.bottom;
	/* Place the window centered */
	GetWindowRect(mapWnd, &rt);
	rt.left += (rt.right - rt.left - splashrt.right) / 2;
	rt.top += (rt.bottom - rt.top - splashrt.bottom) / 2;
	MoveWindow(hWnd, rt.left, rt.top, splashrt.right, splashrt.bottom, TRUE);
	/* Place the control */
	GetClientRect (hWnd, &clientrt);
	MoveWindow (GetDlgItem(hWnd, IDOK),
	    (clientrt.right - clientrt.left - controlrt.right) / 2,
	    clientrt.bottom - controlrt.bottom - SPLASH_OFFSET_Y,
	    controlrt.right, controlrt.bottom, TRUE);
	ShowWindow(hWnd, SW_SHOW);

	while( IsWindow(hWnd) &&
		   GetMessage(&msg, NULL, 0, 0)!=0 ) {
		if( !IsDialogMessage(hWnd, &msg) ) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	GetNHApp()->hPopupWnd = NULL;
	mswin_destroy_splashfonts();
}

BOOL CALLBACK NHSplashWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	switch (message)
	{
	case WM_INITDIALOG:
	    /* set text control font */
		hdc = GetDC(hWnd);
		hdc = GetDC(hWnd);
		SendMessage(hWnd, WM_SETFONT,
			(WPARAM)mswin_get_font(NHW_TEXT, ATR_NONE, hdc, FALSE), 0);
		ReleaseDC(hWnd, hdc);

		ReleaseDC(hWnd, hdc);

		SetFocus(GetDlgItem(hWnd, IDOK));
	return FALSE;

	case WM_PAINT:
	{
		char VersionString[BUFSZ];
		char InfoString[BUFSZ];
		RECT rt;
		HDC hdcBitmap;
		HANDLE OldBitmap;
		HANDLE OldFont;
		PAINTSTRUCT ps;

		hdc = BeginPaint (hWnd, &ps);
		/* Show splash graphic */

		hdcBitmap = CreateCompatibleDC(hdc);
		SetBkMode (hdc, OPAQUE);
		OldBitmap = SelectObject(hdcBitmap, GetNHApp()->bmpSplash);
		BitBlt (hdc, SPLASH_OFFSET_X, SPLASH_OFFSET_Y, SPLASH_WIDTH,
		    SPLASH_HEIGHT, hdcBitmap, 0, 0, SRCCOPY);
		SelectObject (hdcBitmap, OldBitmap);
		DeleteDC (hdcBitmap);

		SetBkMode (hdc, TRANSPARENT);
		/* Print version number */

		SetTextColor (hdc, RGB(0, 0, 0));
		rt.right = rt.left = SPLASH_VERSION_X;
		rt.bottom = rt.top = SPLASH_VERSION_Y;
		Sprintf (VersionString, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR,
		    PATCHLEVEL);
		OldFont = SelectObject(hdc, version_splash_font);
		DrawText (hdc, VersionString, strlen(VersionString), &rt,
		    DT_LEFT | DT_NOPREFIX | DT_CALCRECT);
		DrawText (hdc, VersionString, strlen(VersionString), &rt,
		    DT_LEFT | DT_NOPREFIX);

		/* Print copyright banner */

		SetTextColor (hdc, RGB(255, 255, 255));
		Sprintf (InfoString, "%s\n%s\n%s\n", COPYRIGHT_BANNER_A, COPYRIGHT_BANNER_B,
		    COPYRIGHT_BANNER_C);
		SelectObject(hdc, extrainfo_splash_font);
		rt.left = SPLASH_EXTRA_X_BEGIN;
		rt.right = SPLASH_EXTRA_X_END;
		rt.bottom = rt.top = SPLASH_EXTRA_Y;
		DrawText (hdc, InfoString, strlen(InfoString), &rt,
		    DT_LEFT | DT_NOPREFIX | DT_LEFT | DT_VCENTER | DT_CALCRECT);
		DrawText (hdc, InfoString, strlen(InfoString), &rt,
		    DT_LEFT | DT_NOPREFIX | DT_LEFT | DT_VCENTER);

		SelectObject(hdc, OldFont);
		EndPaint (hWnd, &ps);
	}
	break;

	case WM_COMMAND:
	switch (LOWORD(wParam))
        {
		case IDOK:
			mswin_window_mark_dead(mswin_winid_from_handle(hWnd));
			if( GetNHApp()->hMainWnd==hWnd )
				GetNHApp()->hMainWnd=NULL;
			DestroyWindow(hWnd);
			SetFocus(GetNHApp()->hMainWnd);
			return TRUE;
		}
	break;
	}
	return FALSE;
}

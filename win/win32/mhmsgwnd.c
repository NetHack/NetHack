/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "mhmsgwnd.h"
#include "mhmsg.h"
#include "mhfont.h"

#define MSG_WRAP_TEXT 

#define MSG_VISIBLE_LINES     max(iflags.wc_vary_msgcount, 2)
#define MAX_MSG_LINES		  32
#define MSG_LINES			  (int)min(iflags.msg_history, MAX_MSG_LINES)
#define MAXWINDOWTEXT		  200

#define DEFAULT_COLOR_BG_MSG	COLOR_WINDOW
#define DEFAULT_COLOR_FG_MSG	COLOR_WINDOWTEXT

struct window_line {
	int  attr;
	char text[MAXWINDOWTEXT];
};

typedef struct mswin_nethack_message_window {
	size_t max_text;
	struct window_line window_text[MAX_MSG_LINES];

	int  xChar;       /* horizontal scrolling unit */
	int  yChar;       /* vertical scrolling unit */
	int  xUpper;      /* average width of uppercase letters */
	int  xPos;        /* current horizontal scrolling position */
	int  yPos;        /* current vertical scrolling position */
	int  xMax;        /* maximum horizontal scrolling position */
	int  yMax;        /* maximum vertical scrolling position */
	int	 xPage;		  /* page size of horizontal scroll bar */
 } NHMessageWindow, *PNHMessageWindow;

static TCHAR szMessageWindowClass[] = TEXT("MSNHMessageWndClass");
LRESULT CALLBACK	NHMessageWndProc(HWND, UINT, WPARAM, LPARAM);
static void register_message_window_class(void);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onMSNH_VScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
#ifndef MSG_WRAP_TEXT
static void onMSNH_HScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
#endif
static void onPaint(HWND hWnd);
static void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);
static HDC prepareDC( HDC hdc );

HWND mswin_init_message_window () {
	static int run_once = 0;
	HWND ret;
	DWORD style;

	if( !run_once ) {
		register_message_window_class( );
		run_once = 1;
	}

#ifdef MSG_WRAP_TEXT			
	style =	WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL;
#else
	style = WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL	| WS_HSCROLL;
#endif

	ret = CreateWindowEx(
			WS_EX_CLIENTEDGE,
			szMessageWindowClass,	/* registered class name */
			NULL,					/* window name */			
			style, /* window style */
			0,   /* horizontal position of window */
			0,   /* vertical position of window */
			0,   /* window width */
			0,   /* window height - set it later */
			GetNHApp()->hMainWnd,	/* handle to parent or owner window */
			NULL,					/* menu handle or child identifier */
			GetNHApp()->hApp,		/* handle to application instance */
			NULL );					/* window-creation data */

	if( !ret ) panic("Cannot create message window");

	return ret;
}

void register_message_window_class()
{
	WNDCLASS wcex;
	ZeroMemory( &wcex, sizeof(wcex));

	wcex.style			= CS_NOCLOSE;
	wcex.lpfnWndProc	= (WNDPROC)NHMessageWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= GetNHApp()->hApp;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= message_bg_brush ? message_bg_brush : SYSCLR_TO_BRUSH(DEFAULT_COLOR_BG_MSG);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szMessageWindowClass;

	RegisterClass(&wcex);
}
    
LRESULT CALLBACK NHMessageWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_CREATE:
		onCreate( hWnd, wParam, lParam );
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

	case WM_DESTROY: 
	{
		PNHMessageWindow data;
		data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);
		free(data);
		SetWindowLong(hWnd, GWL_USERDATA, (LONG)0);
	}	break;

    case WM_SIZE: 
    { 
		SCROLLINFO si;
        int xNewSize; 
        int yNewSize; 
		PNHMessageWindow data;
	
		data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);
 
        xNewSize = LOWORD(lParam); 
        yNewSize = HIWORD(lParam); 

		if( xNewSize>0 || yNewSize>0 ) {

#ifndef MSG_WRAP_TEXT
			data->xPage = xNewSize/data->xChar;
			data->xMax = max(0, (int)(1 + data->max_text - data->xPage));
			data->xPos = min(data->xPos, data->xMax);

			ZeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si); 
			si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
			si.nMin   = 0; 
			si.nMax   = data->max_text; 
			si.nPage  = data->xPage; 
			si.nPos   = data->xPos;
			SetScrollInfo(hWnd, SB_HORZ, &si, TRUE); 
#endif
		
			data->yMax = MSG_LINES - MSG_VISIBLE_LINES - 1;
 			data->yPos = min(data->yPos, data->yMax);

			ZeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si); 
			si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
			si.nMin   = 0; 
			si.nMax   = MSG_LINES; 
			si.nPage  = MSG_VISIBLE_LINES;
			si.nPos   = data->yPos;
			SetScrollInfo(hWnd, SB_VERT, &si, TRUE); 
		}
    } 
    break; 

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHMessageWindow data;
	
	data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);
	switch( wParam ) {
	case MSNH_MSG_PUTSTR: 
	{
		PMSNHMsgPutstr msg_data = (PMSNHMsgPutstr)lParam;
		SCROLLINFO si;
		char* p;

		if( msg_data->append ) {
			strncat(data->window_text[MSG_LINES-1].text, msg_data->text, 
				    MAXWINDOWTEXT - strlen(data->window_text[MSG_LINES-1].text));
		} else {
			/* check if the string is empty */
			for(p = data->window_text[MSG_LINES-1].text; *p && isspace(*p); p++);

			if( *p ) {
				/* last string is not empty - scroll up */
				memmove(&data->window_text[0],
						&data->window_text[1],
						(MSG_LINES-1)*sizeof(data->window_text[0]));
			}

			/* append new text to the end of the array */
			data->window_text[MSG_LINES-1].attr = msg_data->attr;
			strncpy(data->window_text[MSG_LINES-1].text, msg_data->text, MAXWINDOWTEXT);
		}
		
		/* reset V-scroll position to display new text */
		data->yPos = data->yMax;

		ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(si);
        si.fMask  = SIF_POS; 
        si.nPos   = data->yPos; 
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

		/* update window content */
		InvalidateRect(hWnd, NULL, TRUE);
	}
	break;

	case MSNH_MSG_CLEAR_WINDOW:
	{
		MSNHMsgPutstr data;

		/* append an empty line to the message window (send message to itself) */
		data.attr = ATR_NONE;
		data.text = " ";
		onMSNHCommand(hWnd, (WPARAM)MSNH_MSG_PUTSTR, (LPARAM)&data);

		InvalidateRect(hWnd, NULL, TRUE);
		break;
	}
	}
}

void onMSNH_VScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHMessageWindow data;
	SCROLLINFO si; 
	int yInc;
 
	/* get window data */
	data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);
	
	ZeroMemory(&si, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_POS;
	GetScrollInfo(hWnd, SB_VERT, &si);

	switch(LOWORD (wParam)) 
	{ 
    // User clicked the shaft above the scroll box. 

    case SB_PAGEUP: 
         yInc = -(int)si.nPage; 
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

	if (yInc = max(-data->yPos, min(yInc, data->yMax - data->yPos))) 
	{ 
		data->yPos += yInc; 
		/* ScrollWindowEx(hWnd, 0, -data->yChar * yInc, 
			(CONST RECT *) NULL, (CONST RECT *) NULL, 
			(HRGN) NULL, (LPRECT) NULL, SW_INVALIDATE | SW_ERASE); 
		*/
		InvalidateRect(hWnd, NULL, TRUE);

		ZeroMemory(&si, sizeof(si));
		si.cbSize = sizeof(si); 
		si.fMask  = SIF_POS; 
		si.nPos   = data->yPos; 
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE); 

		UpdateWindow (hWnd); 
	} 
}

#ifndef MSG_WRAP_TEXT
void onMSNH_HScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHMessageWindow data;
	SCROLLINFO si; 
	int xInc;
 
	/* get window data */
	data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);
	
	ZeroMemory(&si, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE;
	GetScrollInfo(hWnd, SB_HORZ, &si);

    switch(LOWORD (wParam)) 
    { 
        // User clicked shaft left of the scroll box. 

        case SB_PAGEUP: 
             xInc = - (int)si.nPage; 
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

    if (xInc = max (-data->xPos, min (xInc, data->xMax - data->xPos))) 
    { 
        data->xPos += xInc; 
        ScrollWindowEx (hWnd, -data->xChar * xInc, 0, 
            (CONST RECT *) NULL, (CONST RECT *) NULL, 
            (HRGN) NULL, (LPRECT) NULL, SW_INVALIDATE | SW_ERASE); 

		ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(si); 
        si.fMask  = SIF_POS; 
        si.nPos   = data->xPos; 
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE); 
        UpdateWindow (hWnd); 
    } 
}
#endif // MSG_WRAP_TEXT

void onPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc;
	PNHMessageWindow data;
	RECT client_rt, draw_rt;
	int FirstLine, LastLine;
	int i, x, y;
	HGDIOBJ oldFont;
	TCHAR wbuf[MAXWINDOWTEXT+2];
	size_t wlen;
	COLORREF OldBg, OldFg;

	hdc = BeginPaint(hWnd, &ps);

	OldBg = SetBkColor(hdc, message_bg_brush ? message_bg_color : (COLORREF)GetSysColor(DEFAULT_COLOR_BG_MSG));
	OldFg = SetTextColor(hdc, message_fg_brush ? message_fg_color : (COLORREF)GetSysColor(DEFAULT_COLOR_FG_MSG));

	data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);

	GetClientRect(hWnd, &client_rt);

	if( !IsRectEmpty(&ps.rcPaint) ) {
		FirstLine = max (0, data->yPos + ps.rcPaint.top/data->yChar - 1); 
		LastLine = min (MSG_LINES-1, data->yPos + ps.rcPaint.bottom/data->yChar); 
		y = min( ps.rcPaint.bottom, client_rt.bottom - 2); 
 		for (i=LastLine; i>=FirstLine; i--) { 
			if( i==MSG_LINES-1 ) {
				x = data->xChar * (2 - data->xPos); 
			} else {
				x = data->xChar * (4 - data->xPos); 
			}

			if( strlen(data->window_text[i].text)>0 ) {
				/* convert to UNICODE */
				NH_A2W(data->window_text[i].text, wbuf, sizeof(wbuf));
				wlen = _tcslen(wbuf);

				/* calculate text height */
				draw_rt.left = x;
				draw_rt.right = client_rt.right;
				draw_rt.top = y - data->yChar;
				draw_rt.bottom = y;

				oldFont = SelectObject(hdc, mswin_get_font(NHW_MESSAGE, data->window_text[i].attr, hdc, FALSE));

#ifdef MSG_WRAP_TEXT				
				DrawText(hdc, wbuf, wlen, &draw_rt, DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
				draw_rt.top = y - (draw_rt.bottom - draw_rt.top);
				draw_rt.bottom = y;
				DrawText(hdc, wbuf, wlen, &draw_rt, DT_NOPREFIX | DT_WORDBREAK);
#else
				DrawText(hdc, wbuf, wlen, &draw_rt, DT_NOPREFIX );
#endif
				SelectObject(hdc, oldFont);

				y -= draw_rt.bottom - draw_rt.top;
			} else {
				y -= data->yChar;
			}

			/* highligh the last line */
			if( i==MSG_LINES-1 ) {
				draw_rt.left = client_rt.left;
				draw_rt.right = draw_rt.left + 2*data->xChar;
				DrawText(hdc, TEXT("> "), 2, &draw_rt, DT_NOPREFIX );

				y -= 2;
				draw_rt.left = client_rt.left;
				draw_rt.right = client_rt.right;
				draw_rt.top -= 2;
				draw_rt.bottom = client_rt.bottom;
				DrawEdge(hdc, &draw_rt, EDGE_SUNKEN, BF_TOP | BF_ADJUST);
				DrawEdge(hdc, &draw_rt, EDGE_SUNKEN, BF_BOTTOM | BF_ADJUST);
			}
		}
	}

	SetTextColor (hdc, OldFg);
	SetBkColor (hdc, OldBg);
	EndPaint(hWnd, &ps);
}

void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	TEXTMETRIC tm; 
	PNHMessageWindow data;
	HGDIOBJ saveFont;

	/* set window data */
	data = (PNHMessageWindow)malloc(sizeof(NHMessageWindow));
	if( !data ) panic("out of memory");
	ZeroMemory(data, sizeof(NHMessageWindow));
	data->max_text = MAXWINDOWTEXT;
	SetWindowLong(hWnd, GWL_USERDATA, (LONG)data);

    /* Get the handle to the client area's device context. */
    hdc = prepareDC( GetDC(hWnd) ); 
	saveFont = SelectObject(hdc, mswin_get_font(NHW_MESSAGE, ATR_NONE, hdc, FALSE));

    /* Extract font dimensions from the text metrics. */
    GetTextMetrics (hdc, &tm); 
    data->xChar = tm.tmAveCharWidth; 
    data->xUpper = (tm.tmPitchAndFamily & 1 ? 3 : 2) * data->xChar/2; 
    data->yChar = tm.tmHeight + tm.tmExternalLeading; 
	data->xPage = 1;

    /* Free the device context.  */
	SelectObject(hdc, saveFont);
    ReleaseDC (hWnd, hdc); 
}

HDC prepareDC( HDC hdc )
{
	// set font here
	return hdc;
}


void mswin_message_window_size (HWND hWnd, LPSIZE sz)
{
	PNHMessageWindow data;
	RECT rt, client_rt;

	GetWindowRect(hWnd, &rt);

	sz->cx = rt.right - rt.left;
	sz->cy = rt.bottom - rt.top;

	data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);
	if(data) {
		/* set size to accomodate MSG_VISIBLE_LINES, highligh rectangle and
		   horizontal scroll bar (difference between window rect and client rect */
		GetClientRect(hWnd, &client_rt);
		sz->cy = sz->cy-(client_rt.bottom - client_rt.top) +
			     data->yChar * MSG_VISIBLE_LINES + 4;
	}
}
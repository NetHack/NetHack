/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "mhmsgwnd.h"
#include "mhmsg.h"
#include "mhfont.h"


#define MSG_VISIBLE_LINES     4
#define MAX_MSG_LINES		  32
#define MSG_LINES			  (int)min(iflags.msg_history, MAX_MSG_LINES)
#define MAXWINDOWTEXT		  200
#define NHMSG_BKCOLOR         RGB(192, 192, 192)

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
 } NHMessageWindow, *PNHMessageWindow;

static TCHAR szMessageWindowClass[] = TEXT("MSNHMessageWndClass");
LRESULT CALLBACK	MessageWndProc(HWND, UINT, WPARAM, LPARAM);
static void register_message_window_class();
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onMSNH_VScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onMSNH_HScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onPaint(HWND hWnd);
static void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);
static HDC prepareDC( HDC hdc );

HWND mswin_init_message_window () {
	static int run_once = 0;
	HWND ret;

	if( !run_once ) {
		register_message_window_class( );
		run_once = 1;
	}
	
	ret = CreateWindow(                                            
			szMessageWindowClass,	/* registered class name */
			NULL,					/* window name */			
			WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | WS_HSCROLL, /* window style */
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
	wcex.lpfnWndProc	= (WNDPROC)MessageWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= GetNHApp()->hApp;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)CreateSolidBrush(NHMSG_BKCOLOR);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szMessageWindowClass;

	RegisterClass(&wcex);
}
    
LRESULT CALLBACK MessageWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

	case WM_HSCROLL:
		onMSNH_HScroll(hWnd, wParam, lParam);
		break;

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

		data->xMax = max(0, (int)(1 + data->max_text - xNewSize/data->xChar));
		data->xPos = min(data->xPos, data->xMax);

		si.cbSize = sizeof(si); 
        si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
        si.nMin   = 0; 
        si.nMax   = data->max_text; 
        si.nPage  = xNewSize/data->xChar; 
        si.nPos   = data->xPos;
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE); 
		
		data->yMax = MSG_LINES - MSG_VISIBLE_LINES + 1;
 		data->yPos = min(data->yPos, data->yMax);

		si.cbSize = sizeof(si); 
        si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
        si.nMin   = 0; 
        si.nMax   = MSG_LINES; 
        si.nPage  = MSG_VISIBLE_LINES;
        si.nPos   = data->yPos;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE); 
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
		int i;

		memmove(&data->window_text[0],
				&data->window_text[1],
				(MSG_LINES-1)*sizeof(data->window_text[0]));
		data->window_text[MSG_LINES-1].attr = msg_data->attr;
		strncpy(data->window_text[MSG_LINES-1].text, msg_data->text, MAXWINDOWTEXT);
		
		data->yPos = data->yMax;
        si.cbSize = sizeof(si);
        si.fMask  = SIF_POS; 
        si.nPos   = data->yPos; 
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

		data->max_text = 0;
		for( i=0; i<MSG_LINES; i++ )
			if( data->max_text < strlen(data->window_text[i].text) )
				data->max_text = strlen(data->window_text[i].text);

        si.cbSize = sizeof(si);
        si.fMask  = SIF_PAGE; 
		GetScrollInfo(hWnd, SB_HORZ, &si);

		data->xMax = max(0, (int)(1 + data->max_text - si.nPage) );
		data->xPos = min(data->xPos, data->xMax);
        si.cbSize = sizeof(si);
        si.fMask  = SIF_POS | SIF_RANGE; 
		si.nMin   = 0;
		si.nMax   = data->max_text;
        si.nPos   = data->xPos; 
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

		InvalidateRect(hWnd, NULL, TRUE);
	}
	break;

	case MSNH_MSG_CLEAR_WINDOW:
	{
		// do nothing
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

		si.cbSize = sizeof(si); 
		si.fMask  = SIF_POS; 
		si.nPos   = data->yPos; 
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE); 

		UpdateWindow (hWnd); 
	} 
}

void onMSNH_HScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHMessageWindow data;
	SCROLLINFO si; 
	int xInc;
 
	/* get window data */
	data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);
	
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
        si.cbSize = sizeof(si); 
        si.fMask  = SIF_POS; 
        si.nPos   = data->xPos; 
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE); 
        UpdateWindow (hWnd); 
    } 
}

void onPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc;
	PNHMessageWindow data;
	RECT client_rt, draw_rt;
	int FirstLine, LastLine;
	int i, x, y;
	HGDIOBJ oldFont;
	char draw_buf[MAXWINDOWTEXT+2];
	TCHAR wbuf[MAXWINDOWTEXT+2];

	hdc = BeginPaint(hWnd, &ps);

	SetBkColor(hdc, NHMSG_BKCOLOR);

	data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);

	GetClientRect(hWnd, &client_rt);

    FirstLine = max (0, data->yPos + ps.rcPaint.top/data->yChar - 1); 
    LastLine = min (MSG_LINES, data->yPos + ps.rcPaint.bottom/data->yChar); 
 	for (i=FirstLine; i<LastLine; i++) { 
		if( i==MSG_LINES-1 ) {
			x = data->xChar * (-data->xPos); 

			SetRect( &draw_rt, 
			 client_rt.left, client_rt.bottom - data->yChar - 4, client_rt.right, client_rt.bottom );
			DrawEdge(hdc, &draw_rt, EDGE_SUNKEN, BF_TOP | BF_ADJUST);
			DrawEdge(hdc, &draw_rt, EDGE_SUNKEN, BF_BOTTOM | BF_ADJUST);

			draw_rt.left = x;
			draw_rt.right = client_rt.right - x;

			strcpy( draw_buf, "> " );
			strcat( draw_buf, data->window_text[i].text );
		} else {
			y = client_rt.bottom - data->yChar * (LastLine - i) - 4; 
			x = data->xChar * (4 - data->xPos); 

			SetRect( &draw_rt, 
			 x, y, max(client_rt.right, client_rt.right-x), y+data->yChar );

			strcpy( draw_buf, data->window_text[i].text );
		}

		if( strlen(draw_buf)>0 ) {
			oldFont = SelectObject(hdc, mswin_create_font(NHW_MESSAGE, data->window_text[i].attr, hdc));
			DrawText(hdc, NH_A2W(draw_buf, wbuf, sizeof(wbuf)), strlen(draw_buf), &draw_rt, DT_VCENTER | DT_NOPREFIX);
			mswin_destroy_font(SelectObject(hdc, oldFont));
		}
	}
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
	saveFont = SelectObject(hdc, mswin_create_font(NHW_STATUS, ATR_NONE, hdc));

    /* Extract font dimensions from the text metrics. */
    GetTextMetrics (hdc, &tm); 
    data->xChar = tm.tmAveCharWidth; 
    data->xUpper = (tm.tmPitchAndFamily & 1 ? 3 : 2) * data->xChar/2; 
    data->yChar = tm.tmHeight + tm.tmExternalLeading; 

    /* Free the device context.  */
	mswin_destroy_font(SelectObject(hdc, saveFont));
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
	RECT rt;

	GetWindowRect(hWnd, &rt);

	sz->cx = rt.right - rt.left;
	sz->cy = rt.bottom - rt.top;

	data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);
	if(data) {
		sz->cy = data->yChar * MSG_VISIBLE_LINES + 4;
	}
}
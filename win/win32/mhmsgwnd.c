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
#define MAXWINDOWTEXT		  TBUFSZ

#define DEFAULT_COLOR_BG_MSG	COLOR_WINDOW
#define DEFAULT_COLOR_FG_MSG	COLOR_WINDOWTEXT

#define MORE "--More--"

struct window_line {
	int  attr;
	char text[MAXWINDOWTEXT];
};

typedef struct mswin_nethack_message_window {
	size_t max_text;
	struct window_line window_text[MAX_MSG_LINES];
#ifdef MSG_WRAP_TEXT
    int  window_text_lines[MAX_MSG_LINES]; /* How much space this text line takes */
#endif
    int  lines_last_turn; /* lines added during the last turn */
    int  cleared;     /* clear was called */
    int  last_line;   /* last line in the message history */
    struct window_line new_line;
    int  lines_not_seen;  /* lines not yet seen by user after last turn or --More-- */
    int  in_more;   /* We are in a --More-- prompt */
    int  nevermore;   /* We want no more --More-- prompts */

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
static COLORREF setMsgTextColor(HDC hdc, int gray);
static void onPaint(HWND hWnd);
static void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);

#ifdef USER_SOUNDS
extern void play_sound_for_message(const char* str);
#endif

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
		
			data->yMax = MSG_LINES-1;
 			data->yPos = min(data->yPos, data->yMax);

			ZeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si); 
			si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
			si.nMin   = MSG_VISIBLE_LINES; 
			si.nMax   = data->yMax + MSG_VISIBLE_LINES - 1; 
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

		if( msg_data->append == 1) {
		    /* Forcibly append to line, even if we pass the edge */
		    strncat(data->window_text[data->last_line].text, msg_data->text, 
			    MAXWINDOWTEXT - strlen(data->window_text[data->last_line].text));
		} else if( msg_data->append < 0) {
            /* remove that many chars */
		    int len = strlen(data->window_text[data->last_line].text);
            int newend = max(len + msg_data->append, 0);
		    data->window_text[data->last_line].text[newend] = '\0';
        } else {
		    /* Try to append but move the whole message to the next line if 
		       it doesn't fit */
		    /* just schedule for displaying */
		    data->new_line.attr = msg_data->attr;
		    strncpy(data->new_line.text, msg_data->text, MAXWINDOWTEXT);
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

#ifdef USER_SOUNDS
		play_sound_for_message(msg_data->text);
#endif
	}
	break;

	case MSNH_MSG_CLEAR_WINDOW:
	{
		data->cleared = 1;
		data->lines_not_seen = 0;
		/* do --More-- again if needed */
		data->nevermore = 0;
		break;
	}
    case MSNH_MSG_CARET:
        /* Create or destroy a caret */
        if (*(int *)lParam)
            CreateCaret(hWnd, NULL, 0, data->yChar);
	    else {
            DestroyCaret();
		/* this means we just did something interactive in this window, so we
		   don't need a --More-- for the lines above.
		   */
		data->lines_not_seen = 0;
	    }
        break;


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

	if (yInc = max( MSG_VISIBLE_LINES - data->yPos, 
		            min(yInc, data->yMax - data->yPos))) 
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

COLORREF setMsgTextColor(HDC hdc, int gray)
{
    COLORREF fg, color1, color2;
    if (gray) {
	if (message_bg_brush) {
	    color1 = message_bg_color;
	    color2 = message_fg_color;
	} else {
	    color1 = (COLORREF)GetSysColor(DEFAULT_COLOR_BG_MSG);
	    color2 = (COLORREF)GetSysColor(DEFAULT_COLOR_FG_MSG);
	}
	/* Make a "gray" color by taking the average of the individual R,G,B 
	   components of two colors. Thanks to Jonathan del Strother */
	fg = RGB((GetRValue(color1)+GetRValue(color2))/2,
		(GetGValue(color1)+GetGValue(color2))/2,
		(GetBValue(color1)+GetBValue(color2))/2);
    } else {
	fg = message_fg_brush ? message_fg_color : (COLORREF)GetSysColor(DEFAULT_COLOR_FG_MSG);
    }


    return SetTextColor(hdc, fg);
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
	TCHAR wbuf[MAXWINDOWTEXT+2];
	size_t wlen;
	COLORREF OldBg, OldFg;
    int do_more = 0;

	hdc = BeginPaint(hWnd, &ps);

	OldBg = SetBkColor(hdc, message_bg_brush ? message_bg_color : (COLORREF)GetSysColor(DEFAULT_COLOR_BG_MSG));
    OldFg = setMsgTextColor(hdc, 0);

	data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);

	GetClientRect(hWnd, &client_rt);

	if( !IsRectEmpty(&ps.rcPaint) ) {
		FirstLine = max (0, data->yPos - (client_rt.bottom - ps.rcPaint.top)/data->yChar + 1); 
		LastLine = min (MSG_LINES-1, data->yPos - (client_rt.bottom - ps.rcPaint.bottom)/data->yChar); 
		y = min( ps.rcPaint.bottom, client_rt.bottom ); 
 		for (i=LastLine; i>=FirstLine; i--) { 
				int lineidx = (data->last_line + 1 + i) % MSG_LINES;
				x = data->xChar * (2 - data->xPos); 

				draw_rt.left = x;
				draw_rt.right = client_rt.right;
				draw_rt.top = y - data->yChar;
				draw_rt.bottom = y;

	    oldFont = SelectObject(hdc, mswin_get_font(NHW_MESSAGE, data->window_text[lineidx].attr, hdc, FALSE));

	    /* find out if we can concatenate the scheduled message without wrapping,
        but only if no clear_nhwindow was done just before putstr'ing this one,
        and only if not in a more prompt already (to prevent concatenating to
        a line containing --More-- when resizing while --More-- is displayed.) 
	       */
	    if (i == MSG_LINES-1 
  && strlen(data->new_line.text) > 0
  && !data->in_more) {
		/* concatenate to the previous line if that is not empty, and
		   if it has the same attribute, and no clear was done.
		   */
		if (strlen(data->window_text[lineidx].text) > 0
			&& (data->window_text[lineidx].attr 
			    == data->new_line.attr)
			&& !data->cleared) {
		    RECT tmpdraw_rt = draw_rt;
		    /* assume this will never work when textsize is near MAXWINDOWTEXT */
		    char tmptext[MAXWINDOWTEXT];
		    TCHAR tmpwbuf[MAXWINDOWTEXT+2];

		    strcpy(tmptext, data->window_text[lineidx].text);
		    strncat(tmptext, "  ", 
			    MAXWINDOWTEXT - strlen(tmptext));
		    strncat(tmptext, data->new_line.text, 
			    MAXWINDOWTEXT - strlen(tmptext));
		    /* Always keep room for a --More-- */
		    strncat(tmptext, MORE, 
			    MAXWINDOWTEXT - strlen(tmptext));
		    NH_A2W(tmptext, tmpwbuf, sizeof(tmpwbuf));
		    /* Find out how large the bounding rectangle of the text is */                
		    DrawText(hdc, tmpwbuf, _tcslen(tmpwbuf), &tmpdraw_rt, DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
		    if ((tmpdraw_rt.bottom - tmpdraw_rt.top) == (draw_rt.bottom - draw_rt.top)  /* fits pixelwise */
			    && (strlen(data->window_text[lineidx].text) 
				+ strlen(data->new_line.text) < MAXWINDOWTEXT)) /* fits charwise */
		    {
			/* strip off --More-- of this combined line and make it so */
			tmptext[strlen(tmptext) - strlen(MORE)] = '\0';
			strcpy(data->window_text[lineidx].text, tmptext);
			data->new_line.text[0] = '\0';
			i++; /* Start from the last line again */
			continue;
		    }
		}
		if (strlen(data->new_line.text) > 0) {
		    /* if we get here, the new line was not concatenated. Add it on a new line,
		       but first check whether we should --More--. */
		    RECT tmpdraw_rt = draw_rt;
		    TCHAR tmpwbuf[MAXWINDOWTEXT+2];
		    HGDIOBJ oldFont;
		    int new_screen_lines;
		    int screen_lines_not_seen = 0;
		    /* Count how many screen lines we haven't seen yet. */
#ifdef MSG_WRAP_TEXT				
		    {
			int n;
			for (n = data->lines_not_seen - 1; n >= 0; n--) {
			    screen_lines_not_seen += 
				data->window_text_lines[(data->last_line - n + MSG_LINES) % MSG_LINES];
			}
		    }
#else
		    screen_lines_not_seen = data->lines_not_seen;
#endif
		    /* Now find out how many screen lines we would like to add */
		    NH_A2W(data->new_line.text, tmpwbuf, sizeof(tmpwbuf));
		    /* Find out how large the bounding rectangle of the text is */                
		    oldFont = SelectObject(hdc, mswin_get_font(NHW_MESSAGE, data->window_text[lineidx].attr, hdc, FALSE));
		    DrawText(hdc, tmpwbuf, _tcslen(tmpwbuf), &tmpdraw_rt, DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
		    SelectObject(hdc, oldFont);
		    new_screen_lines = (tmpdraw_rt.bottom - tmpdraw_rt.top) / data->yChar;
		    /* If this together is more than fits on the window, we must 
		       --More--, unless:
		       - We are in --More-- already (the user is scrolling the window)
		       - The user pressed ESC
		       */
		    if (screen_lines_not_seen + new_screen_lines > MSG_VISIBLE_LINES
			    && !data->in_more && !data->nevermore) {
			data->in_more = 1;
			/* Show --More-- on last line */
			strcat(data->window_text[data->last_line].text, MORE);
			/* Go on drawing, but remember we must do a more afterwards */
			do_more = 1;
		    } else if (!data->in_more) {
			data->last_line++;
			data->last_line %= MSG_LINES; 
			data->window_text[data->last_line].attr = data->new_line.attr;
			strncpy(data->window_text[data->last_line].text, data->new_line.text, MAXWINDOWTEXT);
			data->new_line.text[0] = '\0';
			if (data->cleared) {
			    /* now we are drawing a new line, the old lines can be redrawn in grey.*/
			    data->lines_last_turn = 0;
			    data->cleared = 0;
			}
			data->lines_last_turn++;
			data->lines_not_seen++;
			/* and start over */
			i++; /* Start from the last line again */
			continue;
		    }
		}
	    }
	    /* convert to UNICODE */
	    NH_A2W(data->window_text[lineidx].text, wbuf, sizeof(wbuf));
	    wlen = _tcslen(wbuf);
	    setMsgTextColor(hdc, i < (MSG_LINES - data->lines_last_turn));
#ifdef MSG_WRAP_TEXT				
	    /* Find out how large the bounding rectangle of the text is */                
				DrawText(hdc, wbuf, wlen, &draw_rt, DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
	    /* move that rectangle up, so that the bottom remains at the same height */
				draw_rt.top = y - (draw_rt.bottom - draw_rt.top);
				draw_rt.bottom = y;
	    /* Remember the height of this line for subsequent --More--'s */
	    data->window_text_lines[lineidx] = (draw_rt.bottom - draw_rt.top) / data->yChar;
	    /* Now really draw it */
				DrawText(hdc, wbuf, wlen, &draw_rt, DT_NOPREFIX | DT_WORDBREAK);
                
                /* Find out the cursor (caret) position */
	    if (i == MSG_LINES-1) {
                    int nnum, numfit;
                    SIZE size;
                    TCHAR *nbuf;
                    int nlen;

                    nbuf = wbuf;
                    nlen = wlen;
                    while (nlen) {
                        /* Get the number of characters that fit on the line */
                        GetTextExtentExPoint(hdc, nbuf, nlen, draw_rt.right - draw_rt.left, &numfit, NULL, &size);
                        /* Search back to a space */
                        nnum = numfit;
                        if (numfit < nlen) {
                            while (nnum > 0 && nbuf[nnum] != ' ')
                                nnum--;
                            /* If no space found, break wherever */
                            if (nnum == 0)
                                nnum = numfit;
                        }
                        nbuf += nnum;
                        nlen -= nnum;
                        if (*nbuf == ' ') {
                            nbuf++;
                            nlen--;
                        }
                    }
                    /* The last size is the size of the last line. Set the caret there.
                       This will fail automatically if we don't own the caret (i.e.,
                       when not in a question.)
                     */
                    SetCaretPos(draw_rt.left + size.cx, draw_rt.bottom - data->yChar);
                }
#else
				DrawText(hdc, wbuf, wlen, &draw_rt, DT_NOPREFIX );
                SetCaretPos(draw_rt.left + size.cx, draw_rt.bottom - data->yChar);
#endif
				SelectObject(hdc, oldFont);
				y -= draw_rt.bottom - draw_rt.top;
			}
	if (do_more) {
	    int okkey = 0;
	    int chop;
	    // @@@ Ok respnses

	    while (!okkey) {
		char c = mswin_nhgetch();

		switch (c)
		{
		    /* space or enter */
		    case ' ':
		    case '\015':
			okkey = 1;
			break;
			/* ESC */
		    case '\033':
			data->nevermore = 1;
			okkey = 1;
			break;
		    default:
			break;
			}
		}
	    chop = strlen(data->window_text[data->last_line].text) 
		- strlen(MORE);
	    data->window_text[data->last_line].text[chop] = '\0';
	    data->in_more = 0;
	    data->lines_not_seen = 0;
	    /* We did the --More--, reset the lines_not_seen; now draw that
	       new line. This is the easiest method */
	    InvalidateRect(hWnd, NULL, TRUE);
	}
	}
	SetTextColor (hdc, OldFg);
	SetBkColor (hdc, OldBg);
	EndPaint(hWnd, &ps);
}

void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHMessageWindow data;
	SIZE	dummy;

	/* set window data */
	data = (PNHMessageWindow)malloc(sizeof(NHMessageWindow));
	if( !data ) panic("out of memory");
	ZeroMemory(data, sizeof(NHMessageWindow));
	data->max_text = MAXWINDOWTEXT;
	SetWindowLong(hWnd, GWL_USERDATA, (LONG)data);

	/* re-calculate window size (+ font size) */
	mswin_message_window_size(hWnd, &dummy);
}

void mswin_message_window_size (HWND hWnd, LPSIZE sz)
{
	HDC hdc;
	HGDIOBJ saveFont;
	TEXTMETRIC tm; 
	PNHMessageWindow data;
	RECT rt, client_rt;

	data = (PNHMessageWindow)GetWindowLong(hWnd, GWL_USERDATA);
	if( !data ) return;

	/* -- Calculate the font size -- */
    /* Get the handle to the client area's device context. */
    hdc = GetDC(hWnd); 
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
	
	/* -- calculate window size -- */
	GetWindowRect(hWnd, &rt);
	sz->cx = rt.right - rt.left;
	sz->cy = rt.bottom - rt.top;

	/* set size to accomodate MSG_VISIBLE_LINES and
	   horizontal scroll bar (difference between window rect and client rect */
	GetClientRect(hWnd, &client_rt);
	sz->cy = sz->cy - (client_rt.bottom - client_rt.top) +
			 data->yChar * MSG_VISIBLE_LINES;
}

/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "resource.h"
#include "mhstatus.h"
#include "mhmsg.h"
#include "mhinput.h"
#include "mhfont.h"

#define MAXWINDOWTEXT 255
extern short glyph2tile[];

/* map window data */
typedef struct mswin_nethack_map_window {
	int map[COLNO][ROWNO];		/* glyph map */

	int xPos, yPos;
	int xPageSize, yPageSize;
	int xCur, yCur;
} NHMapWindow, *PNHMapWindow;

static TCHAR szNHMapWindowClass[] = TEXT("MSNethackMapWndClass");
LRESULT CALLBACK	MapWndProc(HWND, UINT, WPARAM, LPARAM);
static void register_map_window_class();
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onMSNH_VScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onMSNH_HScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onPaint(HWND hWnd);
static void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);
#ifdef TEXTCOLOR
static COLORREF nhcolor_to_RGB(int c);
#endif

HWND mswin_init_map_window () {
	static int run_once = 0;
	HWND ret;

	if( !run_once ) {
		register_map_window_class();
		run_once = 1;
	}
	
	ret = CreateWindow(
			szNHMapWindowClass,		/* registered class name */
			NULL,					/* window name */
			WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_CLIPSIBLINGS, /* window style */
			0,  /* horizontal position of window - set it later */
			0,  /* vertical position of window - set it later */
			0,  /* window width - set it later */
			0,  /* window height - set it later*/
			GetNHApp()->hMainWnd,	/* handle to parent or owner window */
			NULL,					/* menu handle or child identifier */
			GetNHApp()->hApp,		/* handle to application instance */
			NULL );					/* window-creation data */
	if( !ret ) {
		panic("Cannot create map window");
	}
	return ret;
}

void register_map_window_class()
{
	WNDCLASS wcex;
	ZeroMemory( &wcex, sizeof(wcex));

	/* window class */
	wcex.style			= CS_NOCLOSE;
	wcex.lpfnWndProc	= (WNDPROC)MapWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= GetNHApp()->hApp;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= CreateSolidBrush(RGB(0, 0, 0)); /* set backgroup here */
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szNHMapWindowClass;

	if( !RegisterClass(&wcex) ) {
		panic("cannot register Map window class");
	}
}
    
    
LRESULT CALLBACK MapWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PNHMapWindow data;
	
	data = (PNHMapWindow)GetWindowLong(hWnd, GWL_USERDATA);
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
		/* transfer focus back to the main window */
		SetFocus(GetNHApp()->hMainWnd);
		break;

	case WM_HSCROLL:
		onMSNH_HScroll(hWnd, wParam, lParam);
		break;

	case WM_VSCROLL:
		onMSNH_VScroll(hWnd, wParam, lParam);
		break;

    case WM_SIZE: 
    { 
		SCROLLINFO si;
        int xNewSize; 
        int yNewSize; 
 
        xNewSize = LOWORD(lParam); 
        yNewSize = HIWORD(lParam); 

		/* adjust horizontal scroll bar */
		if( xNewSize/TILE_X >= COLNO ) {
			data->xPos = 0;
			GetNHApp()->bNoHScroll = TRUE;
		} else {
			GetNHApp()->bNoHScroll = FALSE;
			data->xPos = max(0, min(COLNO, u.ux - xNewSize/TILE_X/2));
		}

        si.cbSize = sizeof(si); 
        si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
        si.nMin   = 0; 
        si.nMax   = COLNO; 
        si.nPage  = xNewSize/TILE_X; 
        si.nPos   = data->xPos; 
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE); 
 
		/* adjust vertical scroll bar */
		if( yNewSize/TILE_Y >= ROWNO ) {
			data->yPos = 0;
			GetNHApp()->bNoVScroll = TRUE;
		} else {
			GetNHApp()->bNoVScroll = FALSE;
			data->yPos = max(0, min(ROWNO, u.uy - yNewSize/TILE_Y/2));
		}

        si.cbSize = sizeof(si); 
        si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
        si.nMin   = 0; 
        si.nMax   = ROWNO; 
        si.nPage  = yNewSize/TILE_Y; 
        si.nPos   = data->yPos; 
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE); 

		/* erase window */
		InvalidateRect(hWnd, NULL, TRUE);
    } 
    break; 

	case WM_LBUTTONDOWN:
		NHEVENT_MS( 
			min(COLNO, data->xPos + LOWORD(lParam)/TILE_X),
			min(ROWNO, data->yPos + HIWORD(lParam)/TILE_Y) 
		);
	break;

	case WM_DESTROY:
		free(data);
		SetWindowLong(hWnd, GWL_USERDATA, (LONG)0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHMapWindow data;
	RECT rt;

	data = (PNHMapWindow)GetWindowLong(hWnd, GWL_USERDATA);
	switch(wParam) {
	case MSNH_MSG_PRINT_GLYPH: 
	{
		PMSNHMsgPrintGlyph msg_data = (PMSNHMsgPrintGlyph)lParam;
		data->map[msg_data->x][msg_data->y] = msg_data->glyph;

		/* invalidate the update area */
		rt.left = msg_data->x*TILE_X - TILE_X*data->xPos;
		rt.top = msg_data->y*TILE_Y - TILE_Y*data->yPos;
		rt.right = rt.left + TILE_X;
		rt.bottom = rt.top + TILE_Y;

		InvalidateRect(hWnd, &rt, TRUE);
	} 
	break;

	case MSNH_MSG_CLIPAROUND: 
	{
		PMSNHMsgClipAround msg_data = (PMSNHMsgClipAround)lParam;
		SCROLLINFO si; 
		int xPage, yPage;
		int x, y;
 
		/* get page size */
		if( !GetNHApp()->bNoHScroll ) {
			si.cbSize = sizeof(si);
			si.fMask = SIF_PAGE;
			GetScrollInfo(hWnd, SB_HORZ, &si);
			xPage = si.nPage;

			x = max(0, min(COLNO, msg_data->x - xPage/2));

			SendMessage( hWnd, WM_HSCROLL, (WPARAM)MAKELONG(SB_THUMBTRACK, x), (LPARAM)NULL	);
		}

		if( !GetNHApp()->bNoVScroll ) {
			si.cbSize = sizeof(si);
			si.fMask = SIF_PAGE;
			GetScrollInfo(hWnd, SB_VERT, &si);
			yPage = si.nPage;

			y = max(0, min(ROWNO, msg_data->y - yPage/2));

			SendMessage( hWnd, WM_VSCROLL, (WPARAM)MAKELONG(SB_THUMBTRACK, y), (LPARAM)NULL );
		}
	} 
	break;

	case MSNH_MSG_CLEAR_WINDOW: 
	{
		int i, j;
		for(i=0; i<COLNO; i++) 
			for(j=0; j<ROWNO; j++) {
			data->map[i][j] = -1;
		}
		InvalidateRect(hWnd, NULL, TRUE);
	} break;

	case MSNH_MSG_CURSOR:
	{
		PMSNHMsgCursor msg_data = (PMSNHMsgCursor)lParam;
		HDC    hdc;
		RECT   rt;

		/* move focus rectangle at the cursor postion */
		hdc = GetDC(hWnd);
		rt.left = (data->xCur - data->xPos)*TILE_X;
		rt.top = (data->yCur - data->yPos)*TILE_Y;
		rt.right = rt.left + TILE_X;
		rt.bottom = rt.top + TILE_Y;
		DrawFocusRect(hdc, &rt);
		
		data->xCur = msg_data->x;
		data->yCur = msg_data->y;
		rt.left = (data->xCur - data->xPos)*TILE_X;
		rt.top = (data->yCur - data->yPos)*TILE_Y;
		rt.right = rt.left + TILE_X;
		rt.bottom = rt.top + TILE_Y;
		DrawFocusRect(hdc, &rt);

		ReleaseDC(hWnd, hdc);
	} break;
	}
}

void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHMapWindow data;
	int i,j;

	/* set window data */
	data = (PNHMapWindow)malloc(sizeof(NHMapWindow));
	if( !data ) panic("out of memory");

	ZeroMemory(data, sizeof(NHMapWindow));
	for(i=0; i<COLNO; i++) 
		for(j=0; j<ROWNO; j++) {
		data->map[i][j] = -1;
	}
	SetWindowLong(hWnd, GWL_USERDATA, (LONG)data);
}

void onPaint(HWND hWnd) 
{
	PNHMapWindow data;
	PAINTSTRUCT ps;
	HDC hDC;
	HDC tileDC;
	HGDIOBJ saveBmp;
	RECT paint_rt;
	int i, j;

	/* get window data */
	data = (PNHMapWindow)GetWindowLong(hWnd, GWL_USERDATA);

	hDC = BeginPaint(hWnd, &ps);

	/* calculate paint rectangle */
	if( !IsRectEmpty(&ps.rcPaint) ) {
		/* calculate paint rectangle */
		paint_rt.top = data->yPos + ps.rcPaint.top/TILE_X;
		paint_rt.left = data->xPos + ps.rcPaint.left/TILE_Y;
		paint_rt.bottom = min(data->yPos+ps.rcPaint.bottom/TILE_Y+1, ROWNO);
		paint_rt.right = min(data->xPos+ps.rcPaint.right/TILE_X+1, COLNO);

#ifdef REINCARNATION
		if (Is_rogue_level(&u.uz)) {
			/* You enter a VERY primitive world! */
			HGDIOBJ oldFont;
			int offset;

			oldFont = SelectObject(hDC, mswin_create_font(NHW_MAP, ATR_NONE, hDC));
			SetBkMode(hDC, TRANSPARENT);

			/* draw the map */
			for(i=paint_rt.left; i<paint_rt.right; i++) 
			for(j=paint_rt.top; j<paint_rt.bottom; j++) 
			if(data->map[i][j]>0) {
				uchar ch;
				TCHAR wch;
				RECT  glyph_rect;
				unsigned short g=data->map[i][j];
				
				/* (from wintty, naturally)
				 *
				 *  Map the glyph back to a character.
				 *
				 *  Warning:  For speed, this makes an assumption on the order of
				 *		  offsets.  The order is set in display.h.
				 */

#ifdef TEXTCOLOR
				int	    color;

#define zap_color(n)  color = iflags.use_color ? zapcolors[n] : NO_COLOR
#define cmap_color(n) color = iflags.use_color ? defsyms[n].color : NO_COLOR
#define obj_color(n)  color = iflags.use_color ? objects[n].oc_color : NO_COLOR
#define mon_color(n)  color = iflags.use_color ? mons[n].mcolor : NO_COLOR
#define pet_color(n)  color = iflags.use_color ? mons[n].mcolor : NO_COLOR
#define warn_color(n) color = iflags.use_color ? def_warnsyms[n].color : NO_COLOR

# else /* no text color */

#define zap_color(n)
#define cmap_color(n)
#define obj_color(n)
#define mon_color(n)
#define pet_color(c)
#define warn_color(c)
				SetTextColor( hDC, nhcolor_to_RGB(CLR_WHITE) );
#endif

				if ((offset = (g - GLYPH_WARNING_OFF)) >= 0) { 	  /* a warning flash */
					ch = warnsyms[offset];
					warn_color(offset);
				} else if ((offset = (g - GLYPH_SWALLOW_OFF)) >= 0) {	/* swallow */
					/* see swallow_to_glyph() in display.c */
					ch = (uchar) showsyms[S_sw_tl + (offset & 0x7)];
					mon_color(offset >> 3);
				} else if ((offset = (g - GLYPH_ZAP_OFF)) >= 0) {	/* zap beam */
					/* see zapdir_to_glyph() in display.c */
					ch = showsyms[S_vbeam + (offset & 0x3)];
					zap_color((offset >> 2));
				} else if ((offset = (g - GLYPH_CMAP_OFF)) >= 0) {	/* cmap */
					ch = showsyms[offset];
					cmap_color(offset);
				} else if ((offset = (g - GLYPH_OBJ_OFF)) >= 0) {	/* object */
					ch = oc_syms[(int)objects[offset].oc_class];
					obj_color(offset);
				} else if ((offset = (g - GLYPH_BODY_OFF)) >= 0) {	/* a corpse */
					ch = oc_syms[(int)objects[CORPSE].oc_class];
					mon_color(offset);
				} else if ((offset = (g - GLYPH_PET_OFF)) >= 0) {	/* a pet */
					ch = monsyms[(int)mons[offset].mlet];
					pet_color(offset);
				} else {							/* a monster */
					ch = monsyms[(int)mons[g].mlet];
					mon_color(g);
				}	
				// end of wintty code

#ifdef TEXTCOLOR
				if( color == NO_COLOR ) continue;
				else SetTextColor( hDC,  nhcolor_to_RGB(color) );
#endif
				glyph_rect.left = (i-data->xPos)*TILE_X;
				glyph_rect.top  = (j-data->yPos)*TILE_Y;
				glyph_rect.right = glyph_rect.left + TILE_X;
				glyph_rect.bottom = glyph_rect.top + TILE_Y;
				DrawText(hDC, 
						 NH_A2W(&ch, &wch, 1),
						 1,
						 &glyph_rect,
						 DT_CENTER | DT_VCENTER | DT_NOPREFIX
						 );
			}
			mswin_destroy_font( SelectObject(hDC, oldFont) );
		} else
#endif
		{
			/* prepare tiles DC for mapping */
			tileDC = CreateCompatibleDC(hDC);
			saveBmp = SelectObject(tileDC, GetNHApp()->bmpTiles);

			/* draw the map */
			for(i=paint_rt.left; i<paint_rt.right; i++) 
			for(j=paint_rt.top; j<paint_rt.bottom; j++) 
				if(data->map[i][j]>0) {
					short ntile;
					int t_x, t_y;

					ntile = glyph2tile[ data->map[i][j] ];
					t_x = (ntile % TILES_PER_LINE)*TILE_X;
					t_y = (ntile / TILES_PER_LINE)*TILE_Y;

					BitBlt(hDC, (i-data->xPos)*TILE_X, (j-data->yPos)*TILE_Y, TILE_X, TILE_Y, tileDC, t_x, t_y, SRCCOPY );
				}
			SelectObject(tileDC, saveBmp);
			DeleteDC(tileDC);
		}

		/* draw focus rect */
		paint_rt.left = (data->xCur - data->xPos)*TILE_X;
		paint_rt.top = (data->yCur - data->yPos)*TILE_Y;
		paint_rt.right = paint_rt.left + TILE_X;
		paint_rt.bottom = paint_rt.top + TILE_Y;
		DrawFocusRect(hDC, &paint_rt);
	}
	EndPaint(hWnd, &ps);
}

void onMSNH_VScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHMapWindow data;
	SCROLLINFO si; 
	int yNewPos;
	int yDelta;
 
	/* get window data */
	data = (PNHMapWindow)GetWindowLong(hWnd, GWL_USERDATA);
	
	/* get page size */
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE;
	GetScrollInfo(hWnd, SB_VERT, &si);

    switch(LOWORD (wParam)) 
    { 
        /* User clicked shaft left of the scroll box. */
        case SB_PAGEUP: 
             yNewPos = data->yPos-si.nPage; 
             break; 

        /* User clicked shaft right of the scroll box. */
        case SB_PAGEDOWN: 
             yNewPos = data->yPos+si.nPage; 
             break; 

        /* User clicked the left arrow. */
        case SB_LINEUP: 
             yNewPos = data->yPos-1; 
             break; 

        /* User clicked the right arrow. */
        case SB_LINEDOWN: 
             yNewPos = data->yPos+1; 
             break; 

        /* User dragged the scroll box. */
        case SB_THUMBTRACK: 
             yNewPos = HIWORD(wParam); 
             break; 

        default: 
             yNewPos = data->yPos; 
    } 

	yNewPos = max(0, yNewPos);
	yNewPos = min(ROWNO, yNewPos);
	if( yNewPos == data->yPos ) return;
	
	yDelta = yNewPos - data->yPos;
	data->yPos = yNewPos;

    ScrollWindowEx (hWnd, 0, -TILE_Y * yDelta, 
            (CONST RECT *) NULL, (CONST RECT *) NULL, 
            (HRGN) NULL, (LPRECT) NULL, SW_INVALIDATE | SW_ERASE); 

    si.cbSize = sizeof(si); 
    si.fMask  = SIF_POS; 
    si.nPos   = data->yPos; 
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE); 
}

void onMSNH_HScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHMapWindow data;
	SCROLLINFO si; 
	int xNewPos;
	int xDelta;
 
	/* get window data */
	data = (PNHMapWindow)GetWindowLong(hWnd, GWL_USERDATA);
	
	/* get page size */
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE;
	GetScrollInfo(hWnd, SB_HORZ, &si);

    switch(LOWORD (wParam)) 
    { 
        /* User clicked shaft left of the scroll box. */
        case SB_PAGEUP: 
             xNewPos = data->xPos-si.nPage; 
             break; 

        /* User clicked shaft right of the scroll box. */
        case SB_PAGEDOWN: 
             xNewPos = data->xPos+si.nPage; 
             break; 

        /* User clicked the left arrow. */
        case SB_LINEUP: 
             xNewPos = data->xPos-1; 
             break; 

        /* User clicked the right arrow. */
        case SB_LINEDOWN: 
             xNewPos = data->xPos+1; 
             break; 

        /* User dragged the scroll box. */
        case SB_THUMBTRACK: 
             xNewPos = HIWORD(wParam); 
             break; 

        default: 
             xNewPos = data->xPos; 
    } 

	xNewPos = max(0, xNewPos);
	xNewPos = min(COLNO, xNewPos);
	if( xNewPos == data->xPos ) return;
	
	xDelta = xNewPos - data->xPos;
	data->xPos = xNewPos;

    ScrollWindowEx (hWnd, -TILE_X * xDelta, 0, 
            (CONST RECT *) NULL, (CONST RECT *) NULL, 
            (HRGN) NULL, (LPRECT) NULL, SW_INVALIDATE | SW_ERASE); 


    si.cbSize = sizeof(si); 
    si.fMask  = SIF_POS; 
    si.nPos   = data->xPos; 
    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE); 
}

#ifdef TEXTCOLOR
static
COLORREF nhcolor_to_RGB(int c)
{
	switch(c) {
	case CLR_BLACK:			return	RGB(  0,   0,   0);		
	case CLR_RED:			return RGB(255,   0,   0);		
	case CLR_GREEN:			return RGB(  0, 128,   0);		
	case CLR_BROWN:			return RGB(165,  42,   42);
	case CLR_BLUE:			return RGB(  0,   0, 255);	
	case CLR_MAGENTA:		return RGB(255,   0, 255);	
	case CLR_CYAN:			return RGB(  0, 255, 255);		
	case CLR_GRAY:			return RGB(192, 192, 192);	
	case NO_COLOR:			return RGB(  0,   0,   0);		
	case CLR_ORANGE:		return RGB(255, 165,   0);	
	case CLR_BRIGHT_GREEN:	return RGB(  0, 255,   0);
	case CLR_YELLOW:		return RGB(255, 255,   0);	
	case CLR_BRIGHT_BLUE:	return RGB(0,   191, 255);
	case CLR_BRIGHT_MAGENTA: return RGB(255, 127, 255);
	case CLR_BRIGHT_CYAN:	return RGB(127, 255, 255);	/* something close to aquamarine */
	case CLR_WHITE:			return RGB(255, 255, 255);
	default:				return RGB(  0,   0,   0);	/* black */
	}
}
#endif

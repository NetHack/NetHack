/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "patchlevel.h"
#include "resource.h"
#include "mhmsg.h"
#include "mhinput.h"
#include "mhmain.h"
#include "mhmenu.h"
#include "mhstatus.h"
#include "mhmsgwnd.h"
#include "mhmap.h"

typedef struct mswin_nethack_main_window {
	int mapAcsiiModeSave;
} NHMainWindow, *PNHMainWindow;

static TCHAR szMainWindowClass[] = TEXT("MSNHMainWndClass");
static TCHAR szTitle[MAX_LOADSTRING];

LRESULT CALLBACK	MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
static LRESULT  onWMCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void		onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void		register_main_window_class(void);
static int		menuid2mapmode(int menuid);
static int		mapmode2menuid(int map_mode);

HWND mswin_init_main_window () {
	static int run_once = 0;
	HWND ret;

	/* register window class */
	if( !run_once ) {
		LoadString(GetNHApp()->hApp, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
		register_main_window_class( );
		run_once = 1;
	}
	
	/* create the main window */
	ret = CreateWindow(
			szMainWindowClass,		/* registered class name */
			szTitle,				/* window name */
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, /* window style */
			CW_USEDEFAULT,			/* horizontal position of window */
			CW_USEDEFAULT,			/* vertical position of window */
			CW_USEDEFAULT,			/* window width */
			CW_USEDEFAULT,			/* window height */
			NULL,					/* handle to parent or owner window */
			NULL,					/* menu handle or child identifier */
			GetNHApp()->hApp,		/* handle to application instance */
			NULL					/* window-creation data */
		);

	if( !ret ) panic("Cannot create main window");

	return ret;
}

void register_main_window_class()
{
	WNDCLASS wcex;
	
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= GetNHApp()->hApp;
	wcex.hIcon			= LoadIcon(GetNHApp()->hApp, (LPCTSTR)IDI_NETHACKW);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (TCHAR*)IDC_NETHACKW;
	wcex.lpszClassName	= szMainWindowClass;

	RegisterClass(&wcex);
}

/*
 * Keypad keys are translated to the normal values below.
 * Shifted keypad keys are translated to the
 *    shift values below.
 */

enum KEY_INDEXES {
KEY_NW, KEY_N, KEY_NE, KEY_MINUS,
KEY_W, KEY_GOINTERESTING, KEY_E, KEY_PLUS,
KEY_SW, KEY_S, KEY_SE,
KEY_INV, KEY_WAITLOOK,
KEY_LAST};

static const unsigned char
/* normal, shift, control */
keypad[KEY_LAST][3] = {
	{'y', 'Y', C('y')}, /* 7 */
	{'k', 'K', C('k')}, /* 8 */
	{'u', 'U', C('u')}, /* 9 */
	{'m', C('p'), C('p')}, /* - */
	{'h', 'H', C('h')}, /* 4 */
	{'g', 'G', 'g'}, /* 5 */
	{'l', 'L', C('l')}, /* 6 */
	{'+', 'P', C('p')}, /* + */
	{'b', 'B', C('b')}, /* 1 */
	{'j', 'J', C('j')}, /* 2 */
	{'n', 'N', C('n')}, /* 3 */
	{'i', 'I', C('i')}, /* Ins */
	{'.', ':', ':'} /* Del */
}, 
numpad[KEY_LAST][3] = {
	{'7', M('7'), '7'}, /* 7 */
	{'8', M('8'), '8'}, /* 8 */
	{'9', M('9'), '9'}, /* 9 */
	{'m', C('p'), C('p')}, /* - */
	{'4', M('4'), '4'}, /* 4 */
	{'g', 'G', 'g'}, /* 5 */
	{'6', M('6'), '6'}, /* 6 */
	{'+', 'P', C('p')}, /* + */
	{'1', M('1'), '1'}, /* 1 */
	{'2', M('2'), '2'}, /* 2 */
	{'3', M('3'), '3'}, /* 3 */
	{'i', 'I', C('i')}, /* Ins */
	{'.', ':', ':'} /* Del */
};

#define STATEON(x) ((GetKeyState(x) & 0xFFFE) != 0)
#define KEYTABLE_REGULAR(x) ((iflags.num_pad ? numpad : keypad)[x][0])
#define KEYTABLE_SHIFT(x) ((iflags.num_pad ? numpad : keypad)[x][1])
#define KEYTABLE(x) (STATEON(VK_SHIFT) ? KEYTABLE_SHIFT(x) : KEYTABLE_REGULAR(x))

/* map mode macros */
#define IS_MAP_FIT_TO_SCREEN(mode) ((mode)==MAP_MODE_ASCII_FIT_TO_SCREEN || \
							  (mode)==MAP_MODE_TILES_FIT_TO_SCREEN )
  
#define IS_MAP_ASCII(mode) ((mode)!=MAP_MODE_TILES && (mode)!=MAP_MODE_TILES_FIT_TO_SCREEN)

static const char *extendedlist = "acdefijlmnopqrstuvw?2";

#define SCANLO		0x02
static const char scanmap[] = { 	/* ... */
	'1','2','3','4','5','6','7','8','9','0',0,0,0,0,
	'q','w','e','r','t','y','u','i','o','p','[',']', '\n',
	0, 'a','s','d','f','g','h','j','k','l',';','\'', '`',
	0, '\\', 'z','x','c','v','b','n','m',',','.','?'	/* ... */
};

/*
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
*/
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PNHMainWindow data;

	switch (message) 
	{
		case WM_CREATE:
			/* set window data */
			data = (PNHMainWindow)malloc(sizeof(NHMainWindow));
			if( !data ) panic("out of memory");
			ZeroMemory(data, sizeof(NHMainWindow));
			data->mapAcsiiModeSave = MAP_MODE_ASCII12x16;
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)data);

			GetNHApp()->hMainWnd = hWnd;
		break;

		case WM_MSNH_COMMAND:
			onMSNHCommand(hWnd, wParam, lParam);
		break;

        case WM_KEYDOWN: 
		{
			data = (PNHMainWindow)GetWindowLong(hWnd, GWL_USERDATA);

			/* translate arrow keys into nethack commands */
            switch (wParam) 
            { 
			case VK_LEFT:
				if( STATEON(VK_CONTROL) ) {
					/* scroll map window one line left */
					SendMessage(
						mswin_hwnd_from_winid(WIN_MAP),
						WM_HSCROLL,
						MAKEWPARAM(SB_LINEUP, 0),
						(LPARAM)NULL
					);
				} else {
					NHEVENT_KBD(KEYTABLE(KEY_W));
				}
			return 0;

			case VK_RIGHT:
				if( STATEON(VK_CONTROL) ) {
					/* scroll map window one line right */
					SendMessage(
						mswin_hwnd_from_winid(WIN_MAP),
						WM_HSCROLL,
						MAKEWPARAM(SB_LINEDOWN, 0),
						(LPARAM)NULL
					);
				} else {
					NHEVENT_KBD(KEYTABLE(KEY_E));
				}
			return 0;

			case VK_UP:
				if( STATEON(VK_CONTROL) ) {
					/* scroll map window one line up */
					SendMessage(
						mswin_hwnd_from_winid(WIN_MAP),
						WM_VSCROLL,
						MAKEWPARAM(SB_LINEUP, 0),
						(LPARAM)NULL
					);
				} else {
					NHEVENT_KBD(KEYTABLE(KEY_N));
				}
			return 0;

			case VK_DOWN:
				if( STATEON(VK_CONTROL) ) {
					/* scroll map window one line down */
					SendMessage(
						mswin_hwnd_from_winid(WIN_MAP),
						WM_VSCROLL,
						MAKEWPARAM(SB_LINEDOWN, 0),
						(LPARAM)NULL
					);
				} else {
					NHEVENT_KBD(KEYTABLE(KEY_S));
				}
			return 0;

			case VK_HOME:
				if( STATEON(VK_CONTROL) ) {
					/* scroll map window to upper left corner */
					SendMessage(
						mswin_hwnd_from_winid(WIN_MAP),
						WM_VSCROLL,
						MAKEWPARAM(SB_THUMBTRACK, 0),
						(LPARAM)NULL
					);

					SendMessage(
						mswin_hwnd_from_winid(WIN_MAP),
						WM_HSCROLL,
						MAKEWPARAM(SB_THUMBTRACK, 0),
						(LPARAM)NULL
					);
				} else {
					NHEVENT_KBD(KEYTABLE(KEY_NW));
				}
			return 0;

			case VK_END:
				if( STATEON(VK_CONTROL) ) {
					/* scroll map window to lower right corner */
					SendMessage(
						mswin_hwnd_from_winid(WIN_MAP),
						WM_VSCROLL,
						MAKEWPARAM(SB_THUMBTRACK, ROWNO),
						(LPARAM)NULL
					);

					SendMessage(
						mswin_hwnd_from_winid(WIN_MAP),
						WM_HSCROLL,
						MAKEWPARAM(SB_THUMBTRACK, COLNO),
						(LPARAM)NULL
					);
				} else {
					NHEVENT_KBD(KEYTABLE(KEY_SW));
				}
			return 0;

			case VK_PRIOR:
				if( STATEON(VK_CONTROL) ) {
					/* scroll map window one page up */
					SendMessage(
						mswin_hwnd_from_winid(WIN_MAP),
						WM_VSCROLL,
						MAKEWPARAM(SB_PAGEUP, 0),
						(LPARAM)NULL
					);
				} else {
					NHEVENT_KBD(KEYTABLE(KEY_NE));
				}
			return 0;

			case VK_NEXT:
				if( STATEON(VK_CONTROL) ) {
					/* scroll map window one page down */
					SendMessage(
						mswin_hwnd_from_winid(WIN_MAP),
						WM_VSCROLL,
						MAKEWPARAM(SB_PAGEDOWN, 0),
						(LPARAM)NULL
					);
				} else {
					NHEVENT_KBD(KEYTABLE(KEY_SE));
				}
			return 0;

			case VK_DECIMAL:
			case VK_DELETE:
				NHEVENT_KBD(KEYTABLE(KEY_WAITLOOK));
			return 0;

			case VK_INSERT:
				NHEVENT_KBD(KEYTABLE(KEY_INV));
			return 0;

			case VK_SUBTRACT:
				NHEVENT_KBD(KEYTABLE(KEY_MINUS));
			return 0;

			case VK_ADD:
				NHEVENT_KBD(KEYTABLE(KEY_PLUS));
			return 0;

			case VK_CLEAR: /* This is the '5' key */
				NHEVENT_KBD(KEYTABLE(KEY_GOINTERESTING));
			return 0;

			case VK_F4:
				if( IS_MAP_FIT_TO_SCREEN(iflags.wc_map_mode) ) {
					mswin_select_map_mode(
						IS_MAP_ASCII(iflags.wc_map_mode)? 
							data->mapAcsiiModeSave :
							MAP_MODE_TILES
					);
				} else {
					mswin_select_map_mode(
						IS_MAP_ASCII(iflags.wc_map_mode)?
							MAP_MODE_ASCII_FIT_TO_SCREEN :
							MAP_MODE_TILES_FIT_TO_SCREEN
					);
				}
			return 0;

			case VK_F5:
				if( IS_MAP_ASCII(iflags.wc_map_mode) ) {
					if( IS_MAP_FIT_TO_SCREEN(iflags.wc_map_mode) ) {
						mswin_select_map_mode(MAP_MODE_TILES_FIT_TO_SCREEN);
					} else {
						mswin_select_map_mode(MAP_MODE_TILES);
					}
				} else {
					if( IS_MAP_FIT_TO_SCREEN(iflags.wc_map_mode) ) {
						mswin_select_map_mode(MAP_MODE_ASCII_FIT_TO_SCREEN);
					} else {
						mswin_select_map_mode(data->mapAcsiiModeSave);
					}
				}
			return 0;

			default: {
				WORD c;
				BYTE kbd_state[256];

				c = 0;
				ZeroMemory(kbd_state, sizeof(kbd_state));
				GetKeyboardState(kbd_state);

				if( ToAscii( wParam, (lParam>>16)&0xFF, kbd_state, &c, 0) ) {
					NHEVENT_KBD( c&0xFF );
					return 0;
				} else {
					return 1;
				}
			}

			} /* end switch */
		} break;

        case WM_SYSCHAR: /* Alt-char pressed */
        {
            /*
              If not nethackmode, don't handle Alt-keys here.
              If no Alt-key pressed it can never be an extended command 
            */
	    if (GetNHApp()->regNetHackMode && ((lParam & 1<<29) != 0))
            {
                unsigned char c = (unsigned char)(wParam & 0xFF);
		unsigned char scancode = (lParam >> 16) & 0xFF;
                if (index(extendedlist, tolower(c)) != 0)
		{
		    NHEVENT_KBD(M(tolower(c)));
		} else if (scancode == (SCANLO + SIZE(scanmap)) - 1) {
		    NHEVENT_KBD(M('?'));
		}
		return 0;
            }
            return DefWindowProc(hWnd, message, wParam, lParam);
        } 
        break;

		case WM_COMMAND:
			/* process commands - menu commands mostly */
			if( onWMCommand(hWnd, wParam, lParam) )
				return DefWindowProc(hWnd, message, wParam, lParam);
			else
				return 0;

		case WM_MOVE:
		case WM_SIZE:
			mswin_layout_main_window(NULL);
			break;

		case WM_SETFOCUS:
			/* if there is a menu window out there -
			   transfer input focus to it */
			if( IsWindow( GetNHApp()->hPopupWnd ) ) {
				SetFocus( GetNHApp()->hPopupWnd );
			}
			break;

		case WM_CLOSE: 
		{
			/* exit gracefully */
			if (program_state.gameover)
			{
			    /* assume the user really meant this, as the game is already over... */
			    /* to make sure we still save bones, just set stop printing flag */
			    program_state.stopprint++;
			    NHEVENT_KBD('\033'); /* and send keyboard input as if user pressed ESC */
			    /* additional code for this is done in menu and rip windows */
			}
			else
			{
			    switch(MessageBox(hWnd, TEXT("Save?"), TEXT("NetHack for Windows"), MB_YESNOCANCEL | MB_ICONQUESTION)) {
			    case IDYES: NHEVENT_KBD('y'); dosave(); break;
			    case IDNO: NHEVENT_KBD('q'); done(QUIT); break;
			    case IDCANCEL: break;
			    }
			}
		} return 0;

		case WM_DESTROY:
			/* apparently we never get here 
			   TODO: work on exit routines - need to send
			   WM_QUIT somehow */  

			/* clean up */
			free( (PNHMainWindow)GetWindowLong(hWnd, GWL_USERDATA) );
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)0);

			// PostQuitMessage(0);
			exit(1); 
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch(wParam) {

	/* new window was just added */
	case MSNH_MSG_ADDWND: {
		PMSNHMsgAddWnd msg_param = (PMSNHMsgAddWnd)lParam;
		HWND child;

		if( GetNHApp()->windowlist[msg_param->wid].type == NHW_MAP )
			mswin_select_map_mode(iflags.wc_map_mode);
		
		child = GetNHApp()->windowlist[msg_param->wid].win;
		if( child ) mswin_layout_main_window(child);
	} break;

	}
}

/* adjust windows to fit main window layout 
   ---------------------------
   |        Status           |
   +-------------------------+
   |                         |
   |                         |
   |          MAP            |
   |                         |
   |                         |
   +-------------------------+
   |       Messages          |
   ---------------------------
*/
void mswin_layout_main_window(HWND changed_child)
{
	winid i;
	POINT pt;
	RECT client_rt, wnd_rect;
	SIZE menu_size;
	POINT status_org;
	SIZE status_size;
	POINT msg_org;
	SIZE msg_size;
	POINT map_org;
	SIZE map_size;
	HWND wnd_status, wnd_msg;
	PNHMainWindow  data;

	GetClientRect(GetNHApp()->hMainWnd, &client_rt);
	data = (PNHMainWindow)GetWindowLong(GetNHApp()->hMainWnd, GWL_USERDATA);

	/* get sizes of child windows */
	wnd_status = mswin_hwnd_from_winid(WIN_STATUS);
	if( IsWindow(wnd_status) ) { 
		mswin_status_window_size(wnd_status, &status_size);
	} else {
		status_size.cx = status_size.cy = 0;
	}

	wnd_msg = mswin_hwnd_from_winid(WIN_MESSAGE);
	if( IsWindow(wnd_msg) ) { 
		mswin_message_window_size(wnd_msg, &msg_size);
	} else {
		msg_size.cx = msg_size.cy = 0;
	}

	/* set window positions */
	SetRect(&wnd_rect, client_rt.left, client_rt.top, client_rt.right, client_rt.bottom);
	switch(iflags.wc_align_status) {
	case ALIGN_LEFT:
		status_size.cx = (wnd_rect.right-wnd_rect.left)/4;
		status_size.cy = (wnd_rect.bottom-wnd_rect.top); // that won't look good
		status_org.x = wnd_rect.left;
		status_org.y = wnd_rect.top;
		wnd_rect.left += status_size.cx;
		break;

	case ALIGN_RIGHT:  
		status_size.cx = (wnd_rect.right-wnd_rect.left)/4; 
		status_size.cy = (wnd_rect.bottom-wnd_rect.top); // that won't look good
		status_org.x = wnd_rect.right - status_size.cx;
		status_org.y = wnd_rect.top;
		wnd_rect.right -= status_size.cx;
		break;

	case ALIGN_TOP:    
		status_size.cx = (wnd_rect.right-wnd_rect.left);
		status_org.x = wnd_rect.left;
		status_org.y = wnd_rect.top;
		wnd_rect.top += status_size.cy;
		break;

	case ALIGN_BOTTOM:
	default:
		status_size.cx = (wnd_rect.right-wnd_rect.left);
		status_org.x = wnd_rect.left;
		status_org.y = wnd_rect.bottom - status_size.cy;
		wnd_rect.bottom -= status_size.cy;
		break;
	}

	switch(iflags.wc_align_message) {
	case ALIGN_LEFT:
		msg_size.cx = (wnd_rect.right-wnd_rect.left)/4;
		msg_size.cy = (wnd_rect.bottom-wnd_rect.top); 
		msg_org.x = wnd_rect.left;
		msg_org.y = wnd_rect.top;
		wnd_rect.left += msg_size.cx;
		break;

	case ALIGN_RIGHT:  
		msg_size.cx = (wnd_rect.right-wnd_rect.left)/4; 
		msg_size.cy = (wnd_rect.bottom-wnd_rect.top); 
		msg_org.x = wnd_rect.right - msg_size.cx;
		msg_org.y = wnd_rect.top;
		wnd_rect.right -= msg_size.cx;
		break;

	case ALIGN_TOP:    
		msg_size.cx = (wnd_rect.right-wnd_rect.left);
		msg_org.x = wnd_rect.left;
		msg_org.y = wnd_rect.top;
		wnd_rect.top += msg_size.cy;
		break;

	case ALIGN_BOTTOM:
	default:
		msg_size.cx = (wnd_rect.right-wnd_rect.left);
		msg_org.x = wnd_rect.left;
		msg_org.y = wnd_rect.bottom - msg_size.cy;
		wnd_rect.bottom -= msg_size.cy;
		break;
	}

	map_org.x = wnd_rect.left;
	map_org.y = wnd_rect.top;
	map_size.cx = wnd_rect.right - wnd_rect.left;
	map_size.cy = wnd_rect.bottom - wnd_rect.top;

	/* go through the windows list and adjust sizes */
	for( i=0; i<MAXWINDOWS; i++ ) {
		if(GetNHApp()->windowlist[i].win && !GetNHApp()->windowlist[i].dead) {
			switch( GetNHApp()->windowlist[i].type ) {
			case NHW_STATUS:
				MoveWindow(GetNHApp()->windowlist[i].win, 
					       status_org.x,
						   status_org.y,
						   status_size.cx, 
						   status_size.cy, 
						   TRUE );
				break;

			case NHW_MAP:
				MoveWindow(GetNHApp()->windowlist[i].win, 
					       map_org.x, 
						   map_org.y,
						   map_size.cx, 
						   map_size.cy, 
						   TRUE );
				break;

			case NHW_MESSAGE:
				MoveWindow(GetNHApp()->windowlist[i].win, 
					       msg_org.x, 
						   msg_org.y,
						   msg_size.cx, 
						   msg_size.cy, 
						   TRUE );
				break;

			case NHW_MENU:
				mswin_menu_window_size(GetNHApp()->windowlist[i].win, &menu_size);
				menu_size.cx = min(menu_size.cx, (client_rt.right-client_rt.left));

				pt.x = map_org.x + max(0, (int)(map_size.cx-menu_size.cx));
				pt.y = map_org.y;
				ClientToScreen(GetNHApp()->hMainWnd, &pt);
				MoveWindow(GetNHApp()->windowlist[i].win, 
						   pt.x, 
						   pt.y,
						   min(menu_size.cx, map_size.cx), 
						   map_size.cy, 
						   TRUE );
				break;
			}
			ShowWindow(GetNHApp()->windowlist[i].win, SW_SHOW);
		}
	}
}

LRESULT onWMCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PNHMainWindow  data;

	data = (PNHMainWindow)GetWindowLong(hWnd, GWL_USERDATA);
	wmId    = LOWORD(wParam); 
	wmEvent = HIWORD(wParam); 

	// Parse the menu selections:
	switch (wmId)
	{
		case IDM_ABOUT:
		   DialogBox(GetNHApp()->hApp, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
		   break;

		case IDM_EXIT:
		   done2();
		   break;

		case IDM_SAVE:
		   dosave();
		   break;

		case IDM_MAP_TILES:
		case IDM_MAP_ASCII4X6:
		case IDM_MAP_ASCII6X8:
		case IDM_MAP_ASCII8X8:
		case IDM_MAP_ASCII16X8:
		case IDM_MAP_ASCII7X12:
		case IDM_MAP_ASCII8X12:
		case IDM_MAP_ASCII12X16:
		case IDM_MAP_ASCII16X12:
		case IDM_MAP_ASCII10X18:
			mswin_select_map_mode(menuid2mapmode(wmId));
			break;

		case IDM_MAP_FIT_TO_SCREEN:
			if( IS_MAP_FIT_TO_SCREEN(iflags.wc_map_mode) ) {
				mswin_select_map_mode(
					IS_MAP_ASCII(iflags.wc_map_mode)? 
						data->mapAcsiiModeSave :
						MAP_MODE_TILES
				);
			} else {
				mswin_select_map_mode(
					IS_MAP_ASCII(iflags.wc_map_mode)?
						MAP_MODE_ASCII_FIT_TO_SCREEN :
						MAP_MODE_TILES_FIT_TO_SCREEN
				);
			}
			break;

        case IDM_NHMODE:
        {
            GetNHApp()->regNetHackMode = GetNHApp()->regNetHackMode ? 0 : 1;
            mswin_menu_check_intf_mode();
            break;
        }
        case IDM_CLEARSETTINGS:
        {
            mswin_destroy_reg();
            /* Notify the user that windows settings will not be saved this time. */
            MessageBox(GetNHApp()->hMainWnd, 
                "Your Windows Settings will not be stored when you exit this time.", 
                "NetHack", MB_OK | MB_ICONINFORMATION);
            break;
        }
		case IDM_HELP_LONG:	
			display_file(HELP, TRUE);  
			break;
		
		case IDM_HELP_COMMANDS:	
			display_file(SHELP, TRUE);  
			break;
		
		case IDM_HELP_HISTORY:
			(void) dohistory();  
			break;
		
		case IDM_HELP_INFO_CHAR:
			(void) dowhatis();  
			break;
		
		case IDM_HELP_INFO_KEY:
			(void) dowhatdoes();  
			break;
		
		case IDM_HELP_OPTIONS:
			option_help();  
			break;
		
		case IDM_HELP_OPTIONS_LONG:
			display_file(OPTIONFILE, TRUE);  
			break;
		
		case IDM_HELP_EXTCMD:
			(void) doextlist();  
			break;
		
		case IDM_HELP_LICENSE:
			display_file(LICENSE, TRUE);  
			break;

		case IDM_HELP_PORTHELP:
			display_file(PORT_HELP, TRUE);  
			break;

		default:
		   return 1;
	}
	return 0;
}

// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[BUFSZ];
	TCHAR wbuf[BUFSZ];
	RECT   main_rt, dlg_rt;
	SIZE   dlg_sz;

	switch (message)
	{
		case WM_INITDIALOG:
				getversionstring(buf);
				SetDlgItemText(hDlg, IDC_ABOUT_VERSION, NH_A2W(buf, wbuf, sizeof(wbuf)));

				SetDlgItemText(hDlg, IDC_ABOUT_COPYRIGHT,
							NH_A2W(
								COPYRIGHT_BANNER_A "\n"
								COPYRIGHT_BANNER_B "\n"
								COPYRIGHT_BANNER_C,
								wbuf,
								BUFSZ
							) );
						          

				/* center dialog in the main window */
				GetWindowRect(GetNHApp()->hMainWnd, &main_rt);
				GetWindowRect(hDlg, &dlg_rt);
				dlg_sz.cx = dlg_rt.right - dlg_rt.left;
				dlg_sz.cy = dlg_rt.bottom - dlg_rt.top;

				dlg_rt.left = (main_rt.left+main_rt.right-dlg_sz.cx)/2;
				dlg_rt.right = dlg_rt.left + dlg_sz.cx;
				dlg_rt.top = (main_rt.top+main_rt.bottom-dlg_sz.cy)/2;
				dlg_rt.bottom = dlg_rt.top + dlg_sz.cy;
				MoveWindow( hDlg,
							(main_rt.left+main_rt.right-dlg_sz.cx)/2,
							(main_rt.top+main_rt.bottom-dlg_sz.cy)/2,
							dlg_sz.cx,
							dlg_sz.cy,
							TRUE );

				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

void mswin_menu_check_intf_mode()
{
    HMENU hMenu = GetMenu(GetNHApp()->hMainWnd);

    if (GetNHApp()->regNetHackMode)
        CheckMenuItem(hMenu, IDM_NHMODE, MF_CHECKED);
    else
        CheckMenuItem(hMenu, IDM_NHMODE, MF_UNCHECKED);

}

void mswin_select_map_mode(int mode)
{
	PNHMainWindow  data;
	winid map_id;

	map_id = WIN_MAP;
	data = (PNHMainWindow)GetWindowLong(GetNHApp()->hMainWnd, GWL_USERDATA);

	/* override for Rogue level */
#ifdef REINCARNATION
    if( Is_rogue_level(&u.uz) && !IS_MAP_ASCII(mode) ) return;
#endif

	/* set map mode menu mark */
	if( IS_MAP_ASCII(mode) ) {
		CheckMenuRadioItem(
			GetMenu(GetNHApp()->hMainWnd), 
			IDM_MAP_TILES, 
			IDM_MAP_ASCII10X18, 
			mapmode2menuid( IS_MAP_FIT_TO_SCREEN(mode)? data->mapAcsiiModeSave : mode ), 
			MF_BYCOMMAND);
	} else {
		CheckMenuRadioItem(
			GetMenu(GetNHApp()->hMainWnd), 
			IDM_MAP_TILES, 
			IDM_MAP_ASCII10X18, 
			mapmode2menuid( MAP_MODE_TILES ), 
			MF_BYCOMMAND);
	}

	/* set fit-to-screen mode mark */
	CheckMenuItem(
		GetMenu(GetNHApp()->hMainWnd),
		IDM_MAP_FIT_TO_SCREEN,
		MF_BYCOMMAND | 
		(IS_MAP_FIT_TO_SCREEN(mode)? MF_CHECKED : MF_UNCHECKED)
	);

	if( IS_MAP_ASCII(iflags.wc_map_mode) && !IS_MAP_FIT_TO_SCREEN(iflags.wc_map_mode)) {
		data->mapAcsiiModeSave = iflags.wc_map_mode;
	}

	iflags.wc_map_mode = mode;
	
	/* 
	** first, check if WIN_MAP has been inialized.
	** If not - attempt to retrieve it by type, then check it again
	*/
	if( map_id==WIN_ERR ) 
		map_id = mswin_winid_from_type(NHW_MAP);
	if( map_id!=WIN_ERR )
		mswin_map_mode(mswin_hwnd_from_winid(map_id), mode);
}

static struct t_menu2mapmode {
	int menuID;
	int mapMode;
} _menu2mapmode[] = 
{
	{ IDM_MAP_TILES, MAP_MODE_TILES },
	{ IDM_MAP_ASCII4X6, MAP_MODE_ASCII4x6 },
	{ IDM_MAP_ASCII6X8, MAP_MODE_ASCII6x8 },
	{ IDM_MAP_ASCII8X8, MAP_MODE_ASCII8x8 },
	{ IDM_MAP_ASCII16X8, MAP_MODE_ASCII16x8 },
	{ IDM_MAP_ASCII7X12, MAP_MODE_ASCII7x12 },
	{ IDM_MAP_ASCII8X12, MAP_MODE_ASCII8x12 },
	{ IDM_MAP_ASCII12X16, MAP_MODE_ASCII12x16 },
	{ IDM_MAP_ASCII16X12, MAP_MODE_ASCII16x12 },
	{ IDM_MAP_ASCII10X18, MAP_MODE_ASCII10x18 },
	{ IDM_MAP_FIT_TO_SCREEN, MAP_MODE_ASCII_FIT_TO_SCREEN },
	{ -1, -1 }
};

int	menuid2mapmode(int menuid)
{
	struct t_menu2mapmode* p;
	for( p = _menu2mapmode; p->mapMode!=-1; p++ ) 
		if(p->menuID==menuid ) return p->mapMode;
	return -1;
}

int	mapmode2menuid(int map_mode)
{
	struct t_menu2mapmode* p;
	for( p = _menu2mapmode; p->mapMode!=-1; p++ ) 
		if(p->mapMode==map_mode ) return p->menuID;
	return -1;
}

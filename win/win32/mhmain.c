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

#define MAX_LOADSTRING 100

typedef struct mswin_nethack_main_window {
	int dummy;
} NHMainWindow, *PNHMainWindow;

static TCHAR szMainWindowClass[] = TEXT("MSNHMainWndClass");
static TCHAR szTitle[MAX_LOADSTRING];

LRESULT CALLBACK	MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
static LRESULT  onWMCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void		onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void		register_main_window_class();

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
	wcex.hIcon			= LoadIcon(GetNHApp()->hApp, (LPCTSTR)IDI_WINHACK);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (TCHAR*)IDC_WINHACK;
	wcex.lpszClassName	= szMainWindowClass;

	RegisterClass(&wcex);
}
    
    
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
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)data);

			GetNHApp()->hMainWnd = hWnd;
		break;

		case WM_MSNH_COMMAND:
			onMSNHCommand(hWnd, wParam, lParam);
		break;

        case WM_KEYDOWN: 
		{
			/* translate arrow keys into nethack commands */
            switch (wParam) 
            { 
            case VK_LEFT: 
				NHEVENT_KBD('4')
                return 0; 

            case VK_RIGHT: 
				NHEVENT_KBD('6')
                return 0; 

            case VK_UP: 
				NHEVENT_KBD('8')
                return 0; 

            case VK_DOWN: 
				NHEVENT_KBD('2')
                return 0; 

            case VK_HOME: 
				NHEVENT_KBD('7')
                return 0; 

            case VK_END: 
				NHEVENT_KBD('1')
                return 0; 

            case VK_PRIOR: 
				NHEVENT_KBD('9')
                return 0; 

            case VK_NEXT: 
				NHEVENT_KBD('3')
                return 0; 
            }
			return 1;
		} break;

		case WM_CHAR:
			/* all characters go to nethack */
			NHEVENT_KBD(wParam);
		return 0;

		case WM_COMMAND:
			/* process commands - menu commands mostly */
			if( onWMCommand(hWnd, wParam, lParam) )
				return DefWindowProc(hWnd, message, wParam, lParam);
			else
				return 0;

		case WM_SIZE:
			mswin_layout_main_window(NULL);
			break;

		case WM_SETFOCUS:
			/* if there is a menu window out there -
			   transfer input focus to it */
			if( IsWindow( GetNHApp()->hMenuWnd ) ) {
				SetFocus( GetNHApp()->hMenuWnd );
			}
			break;

		case WM_CLOSE: 
		{
			/* exit gracefully */
			switch(MessageBox(hWnd, TEXT("Save?"), TEXT("WinHack"), MB_YESNOCANCEL | MB_ICONQUESTION)) {
			case IDYES:	NHEVENT_KBD('y'); dosave(); break;
			case IDNO: NHEVENT_KBD('y'); done2(); break;
			case IDCANCEL: break;
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
		HWND child = GetNHApp()->windowlist[msg_param->wid].win;
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
	RECT client_rt;
	SIZE menu_size;
	SIZE status_size;
	SIZE msg_size;
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
		status_size.cx = status_size.cy = 0;
	}

	/* go through the windows list and adjust sizes */
	for( i=0; i<MAXWINDOWS; i++ ) {
		if(GetNHApp()->windowlist[i].win && !GetNHApp()->windowlist[i].dead) {
			switch( GetNHApp()->windowlist[i].type ) {
			case NHW_STATUS:
				MoveWindow(GetNHApp()->windowlist[i].win, 
					       client_rt.left, 
						   client_rt.top,
						   client_rt.right-client_rt.left, 
						   status_size.cy, 
						   TRUE );
				break;

			case NHW_MAP:
				MoveWindow(GetNHApp()->windowlist[i].win, 
					       client_rt.left, 
						   client_rt.top+status_size.cy,
						   client_rt.right-client_rt.left, 
						   client_rt.bottom-client_rt.top-msg_size.cy-status_size.cy, 
						   TRUE );
				break;

			case NHW_MESSAGE:
				MoveWindow(GetNHApp()->windowlist[i].win, 
					       client_rt.left,
						   client_rt.bottom-msg_size.cy,
						   client_rt.right-client_rt.left, 
						   msg_size.cy, 
						   TRUE );
				break;

			case NHW_MENU:
				mswin_menu_window_size(GetNHApp()->windowlist[i].win, &menu_size);
				menu_size.cx = min(menu_size.cx, (client_rt.right-client_rt.left));

				pt.x = max(0, (int)(client_rt.right-menu_size.cx));
				pt.y = client_rt.top+status_size.cy;
				ClientToScreen(GetNHApp()->hMainWnd, &pt);
				MoveWindow(GetNHApp()->windowlist[i].win, 
						   pt.x, 
						   pt.y,
						   min(menu_size.cx, client_rt.right), 
						   client_rt.bottom-client_rt.top-msg_size.cy-status_size.cy, 
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


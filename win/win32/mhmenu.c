/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include <assert.h>
#include "resource.h"
#include "mhmenu.h"
#include "mhmain.h"
#include "mhmsg.h"
#include "mhfont.h"

#define MENU_MARGIN			0
#define NHMENU_STR_SIZE     50

typedef struct mswin_menu_item {
	int				glyph;
	ANY_P			identifier;
	CHAR_P			accelerator;
	CHAR_P			group_accel;
	int				attr;
	char			str[NHMENU_STR_SIZE];
	BOOLEAN_P		presel;
} NHMenuItem, *PNHMenuItem;

typedef struct mswin_nethack_menu_window {
	int type;
	int how;

	union {
		struct menu_list {
			int				 size;
			int				 allocated;
			PNHMenuItem		 items;
		} menu;

		struct menu_text {
			TCHAR*			text;
		} text;
	};
	int result;
	int done;

	HBITMAP bmpChecked;
	HBITMAP bmpNotChecked;
} NHMenuWindow, *PNHMenuWindow;

extern short glyph2tile[];

#define NHMENU_IS_SELECTABLE(item) ((item).identifier.a_obj!=NULL)

LRESULT CALLBACK	MenuWndProc(HWND, UINT, WPARAM, LPARAM);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static LRESULT onMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam);
static LRESULT onDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam);
static LRESULT onListChar(HWND hWnd, HWND hwndList, WORD ch);
static void LayoutMenu(HWND hwnd);
static void SetMenuType(HWND hwnd, int type);
static void SetMenuListType(HWND hwnd, int now);
static HWND GetMenuControl(HWND hwnd);
static int GetListPageSize( HWND hwndList );

HWND mswin_init_menu_window (int type) {
	HWND ret;

	ret = CreateDialog(
			GetNHApp()->hApp,
			MAKEINTRESOURCE(IDD_MENU),
			GetNHApp()->hMainWnd,
			MenuWndProc
	);
	if( !ret ) panic("Cannot create menu window");
	
	SetMenuType(ret, type);
	return ret;
}


int mswin_menu_window_select_menu (HWND hWnd, int how, MENU_ITEM_P ** _selected)
{
	MSG msg;
	PNHMenuWindow data;
	int ret_val;
    MENU_ITEM_P *selected = NULL;
	int i;

	assert( _selected!=NULL );
	*_selected = NULL;
	ret_val = -1;

	data = (PNHMenuWindow)GetWindowLong(hWnd, GWL_USERDATA);

	/* Ok, now give items a unique accelerators */
	if( data->type == MENU_TYPE_MENU ) {
		char next_char = 'a';

		for( i=0; i<data->menu.size;  i++) {
			if( data->menu.items[i].accelerator!=0 ) {
				next_char = (char)(data->menu.items[i].accelerator+1);
			} else if( NHMENU_IS_SELECTABLE(data->menu.items[i]) ) {
				if ( (next_char>='a' && next_char<='z') ||
					 (next_char>='A' && next_char<='Z') )  {
					 data->menu.items[i].accelerator = next_char;
				} else {
					if( next_char > 'z' ) next_char = 'A';
					else if ( next_char > 'Z' ) break;

					data->menu.items[i].accelerator = next_char;
				}

				next_char ++;
			}
		}
	}

	/* activate the menu window */
	GetNHApp()->hMenuWnd = hWnd;

	SetMenuListType(hWnd, how);

	mswin_layout_main_window(hWnd);

	EnableWindow(mswin_hwnd_from_winid(WIN_MAP), FALSE);
	EnableWindow(mswin_hwnd_from_winid(WIN_MESSAGE), FALSE);
	EnableWindow(mswin_hwnd_from_winid(WIN_STATUS), FALSE);
	EnableWindow(GetNHApp()->hMainWnd, FALSE);

	SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

	while( IsWindow(hWnd) && 
		   !data->done &&
		   GetMessage(&msg, NULL, 0, 0)!=0 ) {
		if( !IsDialogMessage(hWnd, &msg) ) {
			if (!TranslateAccelerator(msg.hwnd, GetNHApp()->hAccelTable, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	/* get the result */
	if( data->result != -1 ) {
		if(how==PICK_NONE) {
			if(data->result>=0) ret_val=0;
			else				ret_val=-1;
		} else if(how==PICK_ONE) {
			HWND menu_list;
			int nItem;

			menu_list = GetMenuControl(hWnd);
			nItem = SendMessage(menu_list, LB_GETCURSEL, 0, 0);
			if(nItem==LB_ERR) {
				ret_val = 0;
			} else {
				assert(nItem>=0 && nItem<data->menu.size);

				ret_val = 1;
				selected =  malloc( sizeof(MENU_ITEM_P) );
				assert (selected != NULL);
				selected[0].item = data->menu.items[nItem].identifier;
				selected[0].count = -1;
				*_selected = selected;
			}
		} else if(how==PICK_ANY) {
			HWND menu_list;
			int  buffer[255];
			int  n_sel_items_in_buffer;
			int  i;

			menu_list = GetMenuControl(hWnd); 
			ret_val = SendMessage(menu_list, LB_GETSELCOUNT, 0, 0); 
			if( ret_val==LB_ERR ) ret_val = 0;

			if( ret_val > 0 ) {
				n_sel_items_in_buffer = SendMessage(menu_list, LB_GETSELITEMS, sizeof(buffer)/sizeof(buffer[0]), (LPARAM) buffer); 
				ret_val = n_sel_items_in_buffer;

				selected =  malloc( n_sel_items_in_buffer*sizeof(MENU_ITEM_P) );
				assert (selected != NULL);

				ret_val = 0;
				for(i=0; i<n_sel_items_in_buffer; i++ ) {
					assert(buffer[i]>=0 && buffer[i]<data->menu.size);

					if( NHMENU_IS_SELECTABLE(data->menu.items[buffer[i]]) ) {
						selected[ret_val].item = data->menu.items[buffer[i]].identifier;
						selected[ret_val].count = -1;
						ret_val++;
					}
				}
				*_selected = selected;
			}
		}
	}

	/* restore window state */
	EnableWindow(GetNHApp()->hMainWnd, TRUE);
	EnableWindow(mswin_hwnd_from_winid(WIN_MAP), TRUE);
	EnableWindow(mswin_hwnd_from_winid(WIN_MESSAGE), TRUE);
	EnableWindow(mswin_hwnd_from_winid(WIN_STATUS), TRUE);

	SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
	GetNHApp()->hMenuWnd = NULL;

	mswin_window_mark_dead( mswin_winid_from_handle(hWnd) );
	DestroyWindow(hWnd);
	mswin_layout_main_window(hWnd);

	SetFocus(GetTopWindow(GetNHApp()->hMainWnd));

	return ret_val;
}
   
LRESULT CALLBACK MenuWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PNHMenuWindow data;
	int nItem;
	HDC hdc;

	data = (PNHMenuWindow)GetWindowLong(hWnd, GWL_USERDATA);
	switch (message) 
	{
	case WM_INITDIALOG:
		data = (PNHMenuWindow)malloc(sizeof(NHMenuWindow));
		ZeroMemory(data, sizeof(NHMenuWindow));
		data->type = MENU_TYPE_TEXT;
		data->how = PICK_NONE;
		data->result = 0;
		data->done = 0;
		data->bmpChecked = LoadBitmap(GetNHApp()->hApp, MAKEINTRESOURCE(IDB_MENU_SEL));
		data->bmpNotChecked = LoadBitmap(GetNHApp()->hApp, MAKEINTRESOURCE(IDB_MENU_UNSEL));
		SetWindowLong(hWnd, GWL_USERDATA, (LONG)data);

		/* set text control font & the rest*/
		if( data->type==MENU_TYPE_MENU ) {
			hdc = GetDC(GetDlgItem(hWnd, IDC_MENU_TEXT));
			SendDlgItemMessage(hWnd, IDC_MENU_TEXT, WM_SETFONT, (WPARAM)mswin_create_font(NHW_MENU, ATR_NONE, hdc), 0);
			ReleaseDC(GetDlgItem(hWnd, IDC_MENU_TEXT), hdc);

			SetFocus(GetMenuControl(hWnd));
		} else {
			SetFocus(GetDlgItem(hWnd, IDOK));
		}
		return FALSE;
	break;

	case WM_MSNH_COMMAND:
		onMSNHCommand(hWnd, wParam, lParam);
	break;

	case WM_SIZE:
		LayoutMenu(hWnd);
	break;
	return FALSE;

	case WM_COMMAND: 
	{
		switch (LOWORD(wParam)) 
        { 
		case IDCANCEL:
			data->result = -1;
			data->done = 1;
		return TRUE;

		case IDOK:
			data->done = 1;
			data->result = 0;
		return TRUE;

		case IDC_MENU_LIST:
		{
			if( !data || data->type!=MENU_TYPE_MENU ) break;

			switch(HIWORD(wParam)) {
			case LBN_DBLCLK: 
				if(data->how==PICK_ONE) {
					nItem = SendMessage((HWND)lParam, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
					if( nItem!=LB_ERR &&
						nItem>0 &&
						nItem<data->menu.size &&
						data->menu.items[nItem].accelerator!=0 ) {
						data->done = 1;
						data->result = 0;
						return TRUE;
					}
				}
			break;
			}
		}
		break;
		}
	} break;

	case WM_SETFOCUS:
		if( hWnd!=GetNHApp()->hMenuWnd ) {
			SetFocus(GetNHApp()->hMainWnd );
		}
	break;
	
    case WM_MEASUREITEM: 
		return onMeasureItem(hWnd, wParam, lParam);

    case WM_DRAWITEM:
		return onDrawItem(hWnd, wParam, lParam);

	case WM_VKEYTOITEM:
	{ 
		WORD c[4];
		BYTE kbd_state[256];
		
		ZeroMemory(kbd_state, sizeof(kbd_state));
		ZeroMemory(c, sizeof(c));
		GetKeyboardState(kbd_state);
		
		if( ToAscii( LOWORD(wParam), 0, kbd_state, c, 0)==1 ) {
			return onListChar(hWnd, (HWND)lParam, c[0]);
		}
	} return -1;

	case WM_DESTROY:
		DeleteObject(data->bmpChecked);
		DeleteObject(data->bmpNotChecked);
		if( data->type == MENU_TYPE_TEXT ) {
			if( data->text.text ) free(data->text.text);
		}
		free(data);
		SetWindowLong(hWnd, GWL_USERDATA, (LONG)0);
		return TRUE;
	}
	return FALSE;
}

void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PNHMenuWindow data;

	data = (PNHMenuWindow)GetWindowLong(hWnd, GWL_USERDATA);
	switch( wParam ) {
	case MSNH_MSG_PUTSTR: 
	{
		PMSNHMsgPutstr msg_data = (PMSNHMsgPutstr)lParam;
		HWND   text_view;
		TCHAR	wbuf[BUFSZ];
		size_t text_size;

		if( data->type!=MENU_TYPE_TEXT )
			SetMenuType(hWnd, MENU_TYPE_TEXT);

		if( !data->text.text ) {
			text_size = strlen(msg_data->text) + 4;
			data->text.text = (TCHAR*)malloc(text_size*sizeof(data->text.text[0]));
			ZeroMemory(data->text.text, text_size*sizeof(data->text.text[0]));
		} else {
			text_size = _tcslen(data->text.text) + strlen(msg_data->text) + 4;
			data->text.text = (TCHAR*)realloc(data->text.text, text_size*sizeof(data->text.text[0]));
		}
		if( !data->text.text ) break;
		
		_tcscat(data->text.text, NH_A2W(msg_data->text, wbuf, BUFSZ)); 
		_tcscat(data->text.text, TEXT("\r\n"));
		
		text_view = GetDlgItem(hWnd, IDC_MENU_TEXT);
		if( !text_view ) panic("cannot get text view window");
		SetWindowText(text_view, data->text.text);
	} break;

	case MSNH_MSG_STARTMENU:
		if( data->type!=MENU_TYPE_MENU )
			SetMenuType(hWnd, MENU_TYPE_MENU);

		if( data->menu.items ) free(data->menu.items);
		data->how = PICK_NONE;
		data->menu.items = NULL;
		data->menu.size = 0;
		data->menu.allocated = 0;
		data->done = 0;
		data->result = 0;
	break;

	case MSNH_MSG_ADDMENU:
	{
		PMSNHMsgAddMenu msg_data = (PMSNHMsgAddMenu)lParam;
		
		if( data->type!=MENU_TYPE_MENU ) break;
		if( strlen(msg_data->str)==0 ) break;

		if( data->menu.size==data->menu.allocated ) {
			data->menu.allocated += 10;
			data->menu.items = (PNHMenuItem)realloc(data->menu.items, data->menu.allocated*sizeof(NHMenuItem));
		}

		data->menu.items[data->menu.size].glyph = msg_data->glyph;
		data->menu.items[data->menu.size].identifier = *msg_data->identifier;
		data->menu.items[data->menu.size].accelerator = msg_data->accelerator;
		data->menu.items[data->menu.size].group_accel = msg_data->group_accel;
		data->menu.items[data->menu.size].attr = msg_data->attr;
		strncpy(data->menu.items[data->menu.size].str, msg_data->str, NHMENU_STR_SIZE);
		data->menu.items[data->menu.size].presel = msg_data->presel;

		data->menu.size++;
	} break;
	}
}

void LayoutMenu(HWND hWnd) 
{
	PNHMenuWindow data;
	HWND  menu_ok;
	HWND  menu_cancel;
	RECT  clrt, rt;
	POINT pt_elem, pt_ok, pt_cancel;
	SIZE  sz_elem, sz_ok, sz_cancel;

	data = (PNHMenuWindow)GetWindowLong(hWnd, GWL_USERDATA);
	menu_ok = GetDlgItem(hWnd, IDOK);
	menu_cancel = GetDlgItem(hWnd, IDCANCEL);

	/* get window coordinates */
	GetClientRect(hWnd, &clrt );
	
	/* set window placements */
	GetWindowRect(menu_ok, &rt);
	sz_ok.cx = (clrt.right - clrt.left)/2 - 2*MENU_MARGIN;
	sz_ok.cy = rt.bottom-rt.top;
	pt_ok.x = clrt.left + MENU_MARGIN;
	pt_ok.y = clrt.bottom - MENU_MARGIN - sz_ok.cy;

	GetWindowRect(menu_cancel, &rt);
	sz_cancel.cx = (clrt.right - clrt.left)/2 - 2*MENU_MARGIN;
	sz_cancel.cy = rt.bottom-rt.top;
	pt_cancel.x = (clrt.left + clrt.right)/2 + MENU_MARGIN;
	pt_cancel.y = clrt.bottom - MENU_MARGIN - sz_cancel.cy;

	pt_elem.x = clrt.left + MENU_MARGIN;
	pt_elem.y = clrt.top + MENU_MARGIN;
	sz_elem.cx = (clrt.right - clrt.left) - 2*MENU_MARGIN;
	sz_elem.cy = min(pt_cancel.y, pt_ok.y) - 2*MENU_MARGIN;

	MoveWindow(GetMenuControl(hWnd), pt_elem.x, pt_elem.y, sz_elem.cx, sz_elem.cy, TRUE );
	MoveWindow(menu_ok, pt_ok.x, pt_ok.y, sz_ok.cx, sz_ok.cy, TRUE );
	MoveWindow(menu_cancel, pt_cancel.x, pt_cancel.y, sz_cancel.cx, sz_cancel.cy, TRUE );
}

void SetMenuType(HWND hWnd, int type)
{
	PNHMenuWindow data;
	HWND list, text;

	data = (PNHMenuWindow)GetWindowLong(hWnd, GWL_USERDATA);

	data->type = type;
	
	text = GetDlgItem(hWnd, IDC_MENU_TEXT);
	list = GetDlgItem(hWnd, IDC_MENU_LIST);
	if(data->type==MENU_TYPE_TEXT) {
		ShowWindow(list, SW_HIDE);
		EnableWindow(list, FALSE);
		EnableWindow(text, TRUE);
		ShowWindow(text, SW_SHOW);
	} else {
		ShowWindow(text, SW_HIDE);
		EnableWindow(text, FALSE);
		EnableWindow(list, TRUE);
		ShowWindow(list, SW_SHOW);
	}
	LayoutMenu(hWnd);
}

void SetMenuListType(HWND hWnd, int how)
{
	PNHMenuWindow data;
	RECT rt;
	DWORD dwStyles;
	char buf[255];
	int nItem;
	int i;
	HWND control;
	LRESULT fnt;

	data = (PNHMenuWindow)GetWindowLong(hWnd, GWL_USERDATA);
	if( data->type != MENU_TYPE_MENU ) return;

	data->how = how;

	switch(how) {
	case PICK_NONE: 
		dwStyles = WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_CHILD 
			| WS_VSCROLL | WS_HSCROLL | LBS_WANTKEYBOARDINPUT
			| LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | LBS_NOSEL 
			| LBS_OWNERDRAWFIXED; 
		break;
	case PICK_ONE: 
		dwStyles = WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_CHILD 
			| WS_VSCROLL | WS_HSCROLL | LBS_WANTKEYBOARDINPUT
			| LBS_NOTIFY | LBS_NOINTEGRALHEIGHT	| LBS_OWNERDRAWFIXED 
			| LBS_HASSTRINGS; 
		break;
	case PICK_ANY: 
		dwStyles = WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_CHILD 
			| WS_VSCROLL | WS_HSCROLL | LBS_WANTKEYBOARDINPUT
			| LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_MULTIPLESEL 
			| LBS_OWNERDRAWFIXED | LBS_HASSTRINGS; 
		break;
	default: panic("how should be one of PICK_NONE, PICK_ONE or PICK_ANY");
	};

	GetWindowRect(GetDlgItem(hWnd, IDC_MENU_LIST), &rt);
	DestroyWindow(GetDlgItem(hWnd, IDC_MENU_LIST));
	control = CreateWindow(
		TEXT("LISTBOX"),	/* registered class name */
		NULL,				/* window name */
		dwStyles,			/* window style */
		rt.left,			/* horizontal position of window */
		rt.top,				/* vertical position of window */
		rt.right - rt.left, /* window width */
		rt.bottom - rt.top, /* window height */
		hWnd,				/* handle to parent or owner window */
		(HMENU)IDC_MENU_LIST, /* menu handle or child identifier */
		GetNHApp()->hApp,	/* handle to application instance */
		NULL );				/* window-creation data */
	fnt = SendMessage(hWnd, WM_GETFONT, (WPARAM)0, (LPARAM)0);
	SendMessage(control, WM_SETFONT, (WPARAM)fnt, (LPARAM)0);

	for(i=0; i<data->menu.size; i++ ) {
		TCHAR wbuf[255];
		sprintf(buf, "%c - %s", max(data->menu.items[i].accelerator, ' '), data->menu.items[i].str );
		nItem = SendMessage(control, LB_ADDSTRING, (WPARAM)0, (LPARAM) NH_A2W(buf, wbuf, sizeof(wbuf))); 
		if( data->menu.items[i].presel ) {
			if( data->how==PICK_ONE ) {
				nItem = SendMessage(control, LB_SETCURSEL, (WPARAM)nItem, (LPARAM)0); 
			} else if( data->how==PICK_ANY ) {
				nItem = SendMessage(control, LB_SETSEL, (WPARAM)TRUE, (LPARAM)nItem); 
			}
		}
	}
	SetFocus(control);
}


HWND GetMenuControl(HWND hWnd)
{
	PNHMenuWindow data;

	data = (PNHMenuWindow)GetWindowLong(hWnd, GWL_USERDATA);

	if(data->type==MENU_TYPE_TEXT) {
		return GetDlgItem(hWnd, IDC_MENU_TEXT);
	} else {
		return GetDlgItem(hWnd, IDC_MENU_LIST);
	}
}


LRESULT onMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPMEASUREITEMSTRUCT lpmis; 
    TEXTMETRIC tm;
	HGDIOBJ saveFont;
	HDC hdc;
	PNHMenuWindow data;
	RECT client_rect;

    lpmis = (LPMEASUREITEMSTRUCT) lParam; 
	data = (PNHMenuWindow)GetWindowLong(hWnd, GWL_USERDATA);

	hdc = GetDC(GetMenuControl(hWnd));
	saveFont = SelectObject(hdc, mswin_create_font(NHW_MENU, ATR_INVERSE, hdc));
	GetTextMetrics(hdc, &tm);

    /* Set the height of the list box items. */
    lpmis->itemHeight = max(tm.tmHeight, TILE_Y)+2;

	/* Set windth of list box items */
	GetClientRect(hWnd, &client_rect);
	lpmis->itemWidth = client_rect.right - client_rect.left;

	mswin_destroy_font(SelectObject(hdc, saveFont));
	ReleaseDC(GetMenuControl(hWnd), hdc);
	return TRUE;
}

LRESULT onDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPDRAWITEMSTRUCT lpdis; 
	PNHMenuItem item;
	PNHMenuWindow data;
    TEXTMETRIC tm;
	HGDIOBJ saveFont;
	HGDIOBJ savePen;
	HPEN    pen;
	HDC tileDC;
	short ntile;
	int t_x, t_y;
	int x, y;
	TCHAR wbuf[255];

	lpdis = (LPDRAWITEMSTRUCT) lParam; 

    /* If there are no list box items, skip this message. */
    if ( (int)(lpdis->itemID) < 0) return FALSE;

	data = (PNHMenuWindow)GetWindowLong(hWnd, GWL_USERDATA);
    switch (lpdis->itemAction) 
    { 
        case ODA_SELECT: 
        case ODA_DRAWENTIRE: 
            item = &data->menu.items[lpdis->itemID];

			tileDC = CreateCompatibleDC(lpdis->hDC);
			saveFont = SelectObject(lpdis->hDC, mswin_create_font(NHW_MENU, item->attr, lpdis->hDC));
            GetTextMetrics(lpdis->hDC, &tm);

			x = lpdis->rcItem.left;

			/* print check mark */
			if( NHMENU_IS_SELECTABLE(*item) ) {
				HGDIOBJ saveBmp;
				char buf[2];

				saveBmp = SelectObject(tileDC, 
					(lpdis->itemState & ODS_SELECTED)? data->bmpChecked: data->bmpNotChecked );

		        y = (lpdis->rcItem.bottom + lpdis->rcItem.top - TILE_Y) / 2; 
				BitBlt(lpdis->hDC, x, y, TILE_X, TILE_Y, tileDC, 0, 0, SRCCOPY );
				x += TILE_X + 5;

				if(item->accelerator!=0) {
					y = (lpdis->rcItem.bottom + lpdis->rcItem.top - 
						tm.tmHeight) / 2; 
					buf[0] = item->accelerator;
					buf[1] = '\x0';
					TextOut(lpdis->hDC, x, y, NH_A2W(buf, wbuf, sizeof(wbuf)), 1); 
				}
				x += tm.tmAveCharWidth + 5;
				SelectObject(tileDC, saveBmp);
			}

			/* print glyph if present */
			if( item->glyph != NO_GLYPH ) {
				HGDIOBJ saveBmp;

				saveBmp = SelectObject(tileDC, GetNHApp()->bmpTiles);				
				ntile = glyph2tile[ item->glyph ];
				t_x = (ntile % TILES_PER_LINE)*TILE_X;
				t_y = (ntile / TILES_PER_LINE)*TILE_Y;

				y = (lpdis->rcItem.bottom + lpdis->rcItem.top - TILE_Y) / 2; 

				nhapply_image_transparent(
					lpdis->hDC, x, y, TILE_X, TILE_Y, 
					tileDC, t_x, t_y, TILE_X, TILE_Y, TILE_BK_COLOR );
				SelectObject(tileDC, saveBmp);
			}
			x += TILE_X + 5;

            y = (lpdis->rcItem.bottom + lpdis->rcItem.top - 
                tm.tmHeight) / 2; 

            TextOut(lpdis->hDC, 
                x, 
                y, 
                NH_A2W(item->str, wbuf, sizeof(wbuf)), 
                strlen(item->str)); 

			mswin_destroy_font(SelectObject(lpdis->hDC, saveFont));
			DeleteDC(tileDC);
			
            break; 

        case ODA_FOCUS:
			if( data->how==PICK_NONE ) break;

			if( lpdis->itemState & ODS_FOCUS )
				pen = CreatePen(PS_DOT, 0, RGB(0,0,0));
			else 
				pen = CreatePen(PS_DOT, 0, GetBkColor(lpdis->hDC));

			savePen = SelectObject(lpdis->hDC, pen);
			
			MoveToEx(lpdis->hDC,lpdis->rcItem.left, lpdis->rcItem.top, NULL);
			LineTo(lpdis->hDC, lpdis->rcItem.right-1, lpdis->rcItem.top);
			LineTo(lpdis->hDC, lpdis->rcItem.right-1, lpdis->rcItem.bottom-1);
			LineTo(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.bottom-1);
			LineTo(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top);

			DeleteObject(SelectObject(lpdis->hDC, savePen));
           break; 
    } 
	return TRUE;
}

LRESULT onListChar(HWND hWnd, HWND hwndList, WORD ch)
{
	int i = 0;
	PNHMenuWindow data;
	int topIndex, pageSize;

	data = (PNHMenuWindow)GetWindowLong(hWnd, GWL_USERDATA);

	switch( ch ) {
	case MENU_FIRST_PAGE:
		SendMessage(hwndList, LB_SETTOPINDEX, 0, 0);
	return -2;

	case MENU_LAST_PAGE:
		SendMessage(hwndList, 
					LB_SETTOPINDEX, 
					(WPARAM)max(0, data->menu.size-1),
					(LPARAM)0);
	return -2;

	case MENU_NEXT_PAGE:
		topIndex = (int)SendMessage( hwndList, LB_GETTOPINDEX, 0, 0 );
		pageSize = GetListPageSize( hwndList );
		SendMessage(hwndList, 
					LB_SETTOPINDEX, 
					(WPARAM)min(topIndex+pageSize, data->menu.size-1),
					(LPARAM)0);
	return -2;

	case MENU_PREVIOUS_PAGE:
		topIndex = (int)SendMessage( hwndList, LB_GETTOPINDEX, 0, 0 );
		pageSize = GetListPageSize( hwndList );
		SendMessage(hwndList, 
					LB_SETTOPINDEX, 
					(WPARAM)max(topIndex-pageSize, 0),
					(LPARAM)0);
	break;

	case MENU_SELECT_ALL:
		if( data->how == PICK_ANY ) {
			for(i=0; i<data->menu.size; i++ ) {
				SendMessage(hwndList, LB_SETSEL, (WPARAM)TRUE, (LPARAM)i);
			}
			return -2;
		}
	break;

	case MENU_UNSELECT_ALL:
		if( data->how == PICK_ANY ) {
			for(i=0; i<data->menu.size; i++ ) {
				SendMessage(hwndList, LB_SETSEL, (WPARAM)FALSE, (LPARAM)i);
			}
			return -2;
		}
	break;

	case MENU_INVERT_ALL:
		if( data->how == PICK_ANY ) {
			for(i=0; i<data->menu.size; i++ ) {
				SendMessage(hwndList, 
							LB_SETSEL, 
							(WPARAM)!SendMessage(hwndList, LB_GETSEL, (WPARAM)i, (LPARAM)0),
							(LPARAM)i);
			}
			return -2;
		}
	break;

	case MENU_SELECT_PAGE:
		if( data->how == PICK_ANY ) {
			topIndex = (int)SendMessage( hwndList, LB_GETTOPINDEX, 0, 0 );
			pageSize = GetListPageSize( hwndList );
			for(i=0; i<pageSize; i++ ) {
				SendMessage(hwndList, LB_SETSEL, (WPARAM)TRUE, (LPARAM)topIndex+i);
			}
			return -2;
		}
	break;

	case MENU_UNSELECT_PAGE:
		if( data->how == PICK_ANY ) {
			topIndex = (int)SendMessage( hwndList, LB_GETTOPINDEX, 0, 0 );
			pageSize = GetListPageSize( hwndList );
			for(i=0; i<pageSize; i++ ) {
				SendMessage(hwndList, LB_SETSEL, (WPARAM)FALSE, (LPARAM)topIndex+i);
			}
			return -2;
		}
	break;

	case MENU_INVERT_PAGE:
		if( data->how == PICK_ANY ) {
			topIndex = (int)SendMessage( hwndList, LB_GETTOPINDEX, 0, 0 );
			pageSize = GetListPageSize( hwndList );
			for(i=0; i<pageSize; i++ ) {
				SendMessage(hwndList, 
							LB_SETSEL, 
							(WPARAM)!SendMessage(hwndList, LB_GETSEL, (WPARAM)topIndex+i, (LPARAM)0),
							(LPARAM)topIndex+i);
			}
			return -2;
		}
	break;

	case MENU_SEARCH:
	    if( data->how==PICK_ANY || data->how==PICK_ONE ) {
			char buf[BUFSZ];
			mswin_getlin("Search for:", buf);
			if (!*buf || *buf == '\033') return -2;
			for(i=0; i<data->menu.size; i++ ) {
				if( NHMENU_IS_SELECTABLE(data->menu.items[i])
					&& strstr(data->menu.items[i].str, buf) ) {
					if (data->how == PICK_ANY) {
						SendMessage(hwndList, 
									LB_SETSEL, 
									(WPARAM)!SendMessage(hwndList, LB_GETSEL, (WPARAM)i, (LPARAM)0),
									(LPARAM)i);
					} else if( data->how == PICK_ONE ) {
						SendMessage(hwndList, LB_SETCURSEL, (WPARAM)i, (LPARAM)0);
						break;
					}
				}
			} 
		} else {
			mswin_nhbell();
	    }
	return -2;

	case ' ':
		if( data->how==PICK_ONE || data->how==PICK_NONE ) {
			data->done = 1;
			data->result = 0;
			return -2;
		}
	break;
	
	default:
		if( (ch>='a' && ch<='z') ||
			(ch>='A' && ch<='Z') ) {
			for(i=0; i<data->menu.size; i++ ) {
				if( data->menu.items[i].accelerator == ch ) {
					if( data->how == PICK_ANY ) {
						SendMessage(hwndList, 
									LB_SETSEL, 
									(WPARAM)!SendMessage(hwndList, LB_GETSEL, (WPARAM)i, (LPARAM)0),
									(LPARAM)i);
						return -2;
					} else if( data->how == PICK_ONE ) {
						SendMessage(hwndList, LB_SETCURSEL, (WPARAM)i, (LPARAM)0);
						data->result = 0;
						data->done = 1;
						return -2;
					}
				}
			}
		}
	break;
	}
	
	return -1;
}

void mswin_menu_window_size (HWND hWnd, LPSIZE sz)
{
    TEXTMETRIC tm;
	HGDIOBJ saveFont;
	HDC hdc;
	PNHMenuWindow data;
	int i;
	RECT rt;

	GetWindowRect(hWnd, &rt);
	sz->cx = rt.right - rt.left;
	sz->cy = rt.bottom - rt.top;

	data = (PNHMenuWindow)GetWindowLong(hWnd, GWL_USERDATA);
	if(data && data->type==MENU_TYPE_MENU ) {
		hdc = GetDC(GetMenuControl(hWnd));
		saveFont = SelectObject(hdc, mswin_create_font(NHW_MENU, ATR_INVERSE, hdc));
		GetTextMetrics(hdc, &tm);

		/* Set the height of the list box items. */
		for(i=0; i<data->menu.size; i++ ) {
			sz->cx = max(sz->cx, 
				(LONG)(2*TILE_X + tm.tmAveCharWidth*(strlen(data->menu.items[i].str)+1)));
		}

		mswin_destroy_font(SelectObject(hdc, saveFont));
		ReleaseDC(GetMenuControl(hWnd), hdc);
	}
}

int GetListPageSize( HWND hwndList ) 
{
   int ntop, nRectheight, nVisibleItems;
   RECT rc, itemrect;

   ntop = SendMessage(hwndList, LB_GETTOPINDEX, 0, 0);	/* Top item index. */
   GetClientRect(hwndList, &rc);						/* Get list box rectangle. */
   nRectheight = rc.bottom - rc.top;					/* Compute list box height. */

   SendMessage(hwndList, LB_GETITEMRECT, ntop, (DWORD)(&itemrect)); /* Get current line's rectangle. */
   nVisibleItems = nRectheight/(itemrect.bottom - itemrect.top);
   return max(1, nVisibleItems);
}

/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "resource.h"
#include "mhcmd.h"
#include "mhinput.h"

LRESULT CALLBACK	CommandWndProc(HWND, UINT, WPARAM, LPARAM);

struct cmd2key_map {
	UINT		cmd_code;
	char		f_char;
	const char* text;
	UINT		image;
} cmd2key[] = {
	{ IDC_CMD_MOVE_NW,			'7',	 "7",		0 },
	{ IDC_CMD_MOVE_N,			'8',	 "8",		0 },
	{ IDC_CMD_MOVE_NE,			'9',	 "9",		0 },
	{ IDC_CMD_MOVE_W,			'4',	 "4",		0 },
	{ IDC_CMD_MOVE_SELF,		'.',	 ".",		0 },
	{ IDC_CMD_MOVE_E,			'6',	 "6",		0 },
	{ IDC_CMD_MOVE_SW,			'1',	 "1",		0 },
	{ IDC_CMD_MOVE_S,			'2',	 "2",		0 },
	{ IDC_CMD_MOVE_SE,			'3',	 "3",		0 },
	{ IDC_CMD_MOVE_UP,			'<',	 "<",		0 },
	{ IDC_CMD_MOVE_DOWN,		'>',	 ">",		0 },
	{ 0, 0 }
};

HWND mswin_init_command_window () {
	HWND ret;

	ret = CreateDialog(
			GetNHApp()->hApp,
			MAKEINTRESOURCE(IDD_COMMANDS),
			GetNHApp()->hMainWnd,
			CommandWndProc
	);
	if( !ret ) panic("Cannot create command window");
	return ret;
}

void mswin_command_window_size (HWND hwnd, LPSIZE sz)
{
	RECT rt;
	GetWindowRect(hwnd, &rt);
	sz->cx = rt.right - rt.left;
	sz->cy = rt.bottom - rt.top;
}
    
LRESULT CALLBACK CommandWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	struct cmd2key_map* cmd_p;

	switch (message) 
	{
	case WM_COMMAND:
		switch(HIWORD(wParam)) {
		case BN_CLICKED:
			for( cmd_p=cmd2key; cmd_p->cmd_code>0; cmd_p++ ) {
				if( cmd_p->cmd_code==LOWORD(wParam) ) break;
			}

			if( cmd_p->cmd_code>0 ) {
				MSNHEvent event;
				ZeroMemory(&event, sizeof(event));

				event.ch = cmd_p->f_char;
				mswin_input_push(&event);
			}

			SetFocus(hWnd);
		break;
		}
	}
	return FALSE;
}


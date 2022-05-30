/* NetHack 3.7	mhmain.c	$NHDT-Date: 1596498352 2020/08/03 23:45:52 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.76 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include <commdlg.h>
#include "config.h"
#if !defined(PATCHLEVEL_H)
#include "patchlevel.h"
#endif
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

extern winid WIN_STATUS;

static TCHAR szMainWindowClass[] = TEXT("MSNHMainWndClass");
static TCHAR szTitle[MAX_LOADSTRING];
extern void mswin_display_splash_window(BOOL);

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
static LRESULT onWMCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void register_main_window_class(void);
static int menuid2mapmode(int menuid);
static int mapmode2menuid(int map_mode);
static void nhlock_windows(BOOL lock);
static char *nh_compose_ascii_screenshot();
static void mswin_apply_window_style_all();
// returns strdup() created pointer - callee assumes the ownership

HWND
mswin_init_main_window(void)
{
    static int run_once = 0;
    HWND ret;
    WINDOWPLACEMENT wp;

    /* register window class */
    if (!run_once) {
        LoadString(GetNHApp()->hApp, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
        register_main_window_class();
        run_once = 1;
    }

    /* create the main window */
    ret =
        CreateWindow(szMainWindowClass, /* registered class name */
                     szTitle,           /* window name */
                     WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, /* window style */
                     CW_USEDEFAULT,    /* horizontal position of window */
                     CW_USEDEFAULT,    /* vertical position of window */
                     CW_USEDEFAULT,    /* window width */
                     CW_USEDEFAULT,    /* window height */
                     NULL,             /* handle to parent or owner window */
                     NULL,             /* menu handle or child identifier */
                     GetNHApp()->hApp, /* handle to application instance */
                     NULL              /* window-creation data */
                     );

    if (!ret)
        panic("Cannot create main window");

    if (GetNHApp()->regMainMinX != CW_USEDEFAULT) {
        wp.length = sizeof(wp);
        wp.showCmd = GetNHApp()->regMainShowState;

        wp.ptMinPosition.x = GetNHApp()->regMainMinX;
        wp.ptMinPosition.y = GetNHApp()->regMainMinY;

        wp.ptMaxPosition.x = GetNHApp()->regMainMaxX;
        wp.ptMaxPosition.y = GetNHApp()->regMainMaxY;

        wp.rcNormalPosition.left = GetNHApp()->regMainLeft;
        wp.rcNormalPosition.top = GetNHApp()->regMainTop;
        wp.rcNormalPosition.right = GetNHApp()->regMainRight;
        wp.rcNormalPosition.bottom = GetNHApp()->regMainBottom;
        SetWindowPlacement(ret, &wp);
    } else
        ShowWindow(ret, SW_SHOWDEFAULT);

    UpdateWindow(ret);
    return ret;
}

void
register_main_window_class(void)
{
    WNDCLASS wcex;

    ZeroMemory(&wcex, sizeof(wcex));
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC) MainWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetNHApp()->hApp;
    wcex.hIcon = LoadIcon(GetNHApp()->hApp, (LPCTSTR) IDI_NETHACKW);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wcex.lpszMenuName = (TCHAR *) IDC_NETHACKW;
    wcex.lpszClassName = szMainWindowClass;

    RegisterClass(&wcex);
}

/*
 * Keypad keys are translated to the normal values below.
 * Shifted keypad keys are translated to the
 *    shift values below.
 */

enum KEY_INDEXES {
    KEY_NW,
    KEY_N,
    KEY_NE,
    KEY_MINUS,
    KEY_W,
    KEY_GOINTERESTING,
    KEY_E,
    KEY_PLUS,
    KEY_SW,
    KEY_S,
    KEY_SE,
    KEY_INV,
    KEY_WAITLOOK,
    KEY_LAST
};

static const unsigned char
    /* normal, shift, control */
    keypad[KEY_LAST][3] =
        {
          { 'y', 'Y', C('y') },    /* 7 */
          { 'k', 'K', C('k') },    /* 8 */
          { 'u', 'U', C('u') },    /* 9 */
          { 'm', C('p'), C('p') }, /* - */
          { 'h', 'H', C('h') },    /* 4 */
          { 'g', 'G', 'g' },       /* 5 */
          { 'l', 'L', C('l') },    /* 6 */
          { '+', 'P', C('p') },    /* + */
          { 'b', 'B', C('b') },    /* 1 */
          { 'j', 'J', C('j') },    /* 2 */
          { 'n', 'N', C('n') },    /* 3 */
          { 'i', 'I', C('i') },    /* Ins */
          { '.', ':', ':' }        /* Del */
        },
    numpad[KEY_LAST][3] = {
        { '7', M('7'), '7' },    /* 7 */
        { '8', M('8'), '8' },    /* 8 */
        { '9', M('9'), '9' },    /* 9 */
        { 'm', C('p'), C('p') }, /* - */
        { '4', M('4'), '4' },    /* 4 */
        { '5', M('5'), '5' },    /* 5 */
        { '6', M('6'), '6' },    /* 6 */
        { '+', 'P', C('p') },    /* + */
        { '1', M('1'), '1' },    /* 1 */
        { '2', M('2'), '2' },    /* 2 */
        { '3', M('3'), '3' },    /* 3 */
        { '0', M('0'), '0' },    /* Ins */
        { '.', ':', ':' }        /* Del */
    };

#define STATEON(x) ((GetKeyState(x) & 0xFFFE) != 0)
#define KEYTABLE_REGULAR(x) ((iflags.num_pad ? numpad : keypad)[x][0])
#define KEYTABLE_SHIFT(x) ((iflags.num_pad ? numpad : keypad)[x][1])
#define KEYTABLE(x) \
    (STATEON(VK_SHIFT) ? KEYTABLE_SHIFT(x) : KEYTABLE_REGULAR(x))

static const char *extendedlist = "acdefijlmnopqrstuvw?2";

#define SCANLO 0x02
static const char scanmap[] = {
    /* ... */
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0, 0, 0, 0, 'q', 'w',
    'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's', 'd',
    'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '?' /* ... */
};

#define IDT_FUZZ_TIMER 100 

/*
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
*/
LRESULT CALLBACK
MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PNHMainWindow data = (PNHMainWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (message) {
    case WM_CREATE:
        /* set window data */
        data = (PNHMainWindow) malloc(sizeof(NHMainWindow));
        if (!data)
            panic("out of memory");
        ZeroMemory(data, sizeof(NHMainWindow));
        data->mapAcsiiModeSave = MAP_MODE_ASCII12x16;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) data);

        /* update menu items */
        CheckMenuItem(
            GetMenu(hWnd), IDM_SETTING_LOCKWINDOWS,
            MF_BYCOMMAND
                | (GetNHApp()->bWindowsLocked ? MF_CHECKED : MF_UNCHECKED));

        CheckMenuItem(GetMenu(hWnd), IDM_SETTING_AUTOLAYOUT,
                      GetNHApp()->bAutoLayout ? MF_CHECKED : MF_UNCHECKED);

        /* store handle to the mane menu in the application record */
        GetNHApp()->hMainWnd = hWnd;
        break;

    case WM_MSNH_COMMAND:
        onMSNHCommand(hWnd, wParam, lParam);
        break;

    case WM_KEYDOWN: {

        /* translate arrow keys into nethack commands */
        switch (wParam) {
        case VK_LEFT:
            if (STATEON(VK_CONTROL)) {
                /* scroll map window one line left */
                SendMessage(mswin_hwnd_from_winid(WIN_MAP), WM_HSCROLL,
                            MAKEWPARAM(SB_LINEUP, 0), (LPARAM) NULL);
            } else {
                NHEVENT_KBD(KEYTABLE(KEY_W));
            }
            return 0;

        case VK_RIGHT:
            if (STATEON(VK_CONTROL)) {
                /* scroll map window one line right */
                SendMessage(mswin_hwnd_from_winid(WIN_MAP), WM_HSCROLL,
                            MAKEWPARAM(SB_LINEDOWN, 0), (LPARAM) NULL);
            } else {
                NHEVENT_KBD(KEYTABLE(KEY_E));
            }
            return 0;

        case VK_UP:
            if (STATEON(VK_CONTROL)) {
                /* scroll map window one line up */
                SendMessage(mswin_hwnd_from_winid(WIN_MAP), WM_VSCROLL,
                            MAKEWPARAM(SB_LINEUP, 0), (LPARAM) NULL);
            } else {
                NHEVENT_KBD(KEYTABLE(KEY_N));
            }
            return 0;

        case VK_DOWN:
            if (STATEON(VK_CONTROL)) {
                /* scroll map window one line down */
                SendMessage(mswin_hwnd_from_winid(WIN_MAP), WM_VSCROLL,
                            MAKEWPARAM(SB_LINEDOWN, 0), (LPARAM) NULL);
            } else {
                NHEVENT_KBD(KEYTABLE(KEY_S));
            }
            return 0;

        case VK_HOME:
            if (STATEON(VK_CONTROL)) {
                /* scroll map window to upper left corner */
                SendMessage(mswin_hwnd_from_winid(WIN_MAP), WM_VSCROLL,
                            MAKEWPARAM(SB_THUMBTRACK, 0), (LPARAM) NULL);

                SendMessage(mswin_hwnd_from_winid(WIN_MAP), WM_HSCROLL,
                            MAKEWPARAM(SB_THUMBTRACK, 0), (LPARAM) NULL);
            } else {
                NHEVENT_KBD(KEYTABLE(KEY_NW));
            }
            return 0;

        case VK_END:
            if (STATEON(VK_CONTROL)) {
                /* scroll map window to lower right corner */
                SendMessage(mswin_hwnd_from_winid(WIN_MAP), WM_VSCROLL,
                            MAKEWPARAM(SB_THUMBTRACK, ROWNO), (LPARAM) NULL);

                SendMessage(mswin_hwnd_from_winid(WIN_MAP), WM_HSCROLL,
                            MAKEWPARAM(SB_THUMBTRACK, COLNO), (LPARAM) NULL);
            } else {
                NHEVENT_KBD(KEYTABLE(KEY_SW));
            }
            return 0;

        case VK_PRIOR:
            if (STATEON(VK_CONTROL)) {
                /* scroll map window one page up */
                SendMessage(mswin_hwnd_from_winid(WIN_MAP), WM_VSCROLL,
                            MAKEWPARAM(SB_PAGEUP, 0), (LPARAM) NULL);
            } else {
                NHEVENT_KBD(KEYTABLE(KEY_NE));
            }
            return 0;

        case VK_NEXT:
            if (STATEON(VK_CONTROL)) {
                /* scroll map window one page down */
                SendMessage(mswin_hwnd_from_winid(WIN_MAP), WM_VSCROLL,
                            MAKEWPARAM(SB_PAGEDOWN, 0), (LPARAM) NULL);
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

#if defined(DEBUG) && defined(_MSC_VER)
        case VK_PAUSE:
            if (IsDebuggerPresent()) {
                iflags.debug_fuzzer = !iflags.debug_fuzzer;
                return 0;
            }
            break;
#endif

        case VK_CLEAR: /* This is the '5' key */
            NHEVENT_KBD(KEYTABLE(KEY_GOINTERESTING));
            return 0;

        case VK_F4:
            if (IS_MAP_FIT_TO_SCREEN(iflags.wc_map_mode)) {
                mswin_select_map_mode(IS_MAP_ASCII(iflags.wc_map_mode)
                                          ? data->mapAcsiiModeSave
                                          : MAP_MODE_TILES);
            } else {
                mswin_select_map_mode(IS_MAP_ASCII(iflags.wc_map_mode)
                                          ? MAP_MODE_ASCII_FIT_TO_SCREEN
                                          : MAP_MODE_TILES_FIT_TO_SCREEN);
            }
            return 0;

        case VK_F5:
            if (IS_MAP_ASCII(iflags.wc_map_mode)) {
                if (IS_MAP_FIT_TO_SCREEN(iflags.wc_map_mode)) {
                    mswin_select_map_mode(MAP_MODE_TILES_FIT_TO_SCREEN);
                } else {
                    mswin_select_map_mode(MAP_MODE_TILES);
                }
            } else {
                if (IS_MAP_FIT_TO_SCREEN(iflags.wc_map_mode)) {
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

            if (ToAscii((UINT) wParam, (lParam >> 16) & 0xFF, kbd_state, &c, 0)) {
                NHEVENT_KBD(c & 0xFF);
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
        if (GetNHApp()->regNetHackMode && ((lParam & 1 << 29) != 0)) {
            unsigned char c = (unsigned char) (wParam & 0xFF);
            unsigned char scancode = (lParam >> 16) & 0xFF;
            if (index(extendedlist, tolower(c)) != 0) {
                NHEVENT_KBD(M(tolower(c)));
            } else if (scancode == (SCANLO + SIZE(scanmap)) - 1) {
                NHEVENT_KBD(M('?'));
            }
            return 0;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    } break;

    case WM_COMMAND:
        /* process commands - menu commands mostly */
        if (onWMCommand(hWnd, wParam, lParam))
            return DefWindowProc(hWnd, message, wParam, lParam);
        else
            return 0;

    case WM_MOVE:
    case WM_SIZE: {
        WINDOWPLACEMENT wp;

        mswin_layout_main_window(NULL);

        wp.length = sizeof(wp);
        if (GetWindowPlacement(hWnd, &wp)) {
            GetNHApp()->regMainShowState =
                (wp.showCmd == SW_SHOWMAXIMIZED ? SW_SHOWMAXIMIZED
                                                : SW_SHOWNORMAL);

            GetNHApp()->regMainMinX = wp.ptMinPosition.x;
            GetNHApp()->regMainMinY = wp.ptMinPosition.y;

            GetNHApp()->regMainMaxX = wp.ptMaxPosition.x;
            GetNHApp()->regMainMaxY = wp.ptMaxPosition.y;

            GetNHApp()->regMainLeft = wp.rcNormalPosition.left;
            GetNHApp()->regMainTop = wp.rcNormalPosition.top;
            GetNHApp()->regMainRight = wp.rcNormalPosition.right;
            GetNHApp()->regMainBottom = wp.rcNormalPosition.bottom;
        }
        break;
    }
    case WM_SETFOCUS:
        /* if there is a menu window out there -
           transfer input focus to it */
        if (IsWindow(GetNHApp()->hPopupWnd)) {
            SetFocus(GetNHApp()->hPopupWnd);
        }
        break;

    case WM_CLOSE: {
        /* exit gracefully */
        if (g.program_state.gameover) {
            /* assume the user really meant this, as the game is already
             * over... */
            /* to make sure we still save bones, just set stop printing flag
             */
            g.program_state.stopprint++;
            NHEVENT_KBD(
                '\033'); /* and send keyboard input as if user pressed ESC */
            /* additional code for this is done in menu and rip windows */
        } else if (!g.program_state.something_worth_saving) {
            /* User exited before the game started, e.g. during splash display
             */
            /* Just get out. */
            bail((char *) 0);
        } else {
            /* prompt user for action */
            switch (NHMessageBox(hWnd, TEXT("Save?"),
                                 MB_YESNOCANCEL | MB_ICONQUESTION)) {
            case IDYES:
#ifdef SAFERHANGUP
                /* destroy popup window - it has its own loop and we need to
                return control to NetHack core at this point */
                if (IsWindow(GetNHApp()->hPopupWnd))
                    SendMessage(GetNHApp()->hPopupWnd, WM_COMMAND, IDCANCEL,
                                0);

                /* tell NetHack core that "hangup" is requested */
                hangup(1);
#else
                NHEVENT_KBD('y');
                dosave();
#endif
                break;
            case IDNO:
                NHEVENT_KBD('q');
                done(QUIT);
                break;
            case IDCANCEL:
                break;
            }
        }
    }
        return 0;

    case WM_DESTROY:
        /* apparently we never get here
           TODO: work on exit routines - need to send
           WM_QUIT somehow */

        /* clean up */
        free((PNHMainWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA));
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) 0);

        // PostQuitMessage(0);
        exit(1);
        break;

    case WM_DPICHANGED: {
        mswin_layout_main_window(NULL);
    } break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void
onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    switch (wParam) {
    /* new window was just added */
    case MSNH_MSG_ADDWND: {
        PMSNHMsgAddWnd msg_param = (PMSNHMsgAddWnd) lParam;
        HWND child;

        if (GetNHApp()->windowlist[msg_param->wid].type == NHW_MAP)
            mswin_select_map_mode(iflags.wc_map_mode);

        child = GetNHApp()->windowlist[msg_param->wid].win;
    } break;

    case MSNH_MSG_RANDOM_INPUT: {
        nhassert(iflags.debug_fuzzer);
        NHEVENT_KBD(randomkey());
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
void
mswin_layout_main_window(HWND changed_child)
{
    winid i;
    RECT client_rt, wnd_rect;
    POINT status_org;
    SIZE status_size;
    POINT msg_org;
    SIZE msg_size;
    POINT map_org;
    SIZE map_size;
    SIZE menu_size;
    HWND wnd_status, wnd_msg;
    PNHMainWindow data;

    if (GetNHApp()->bAutoLayout) {
        GetClientRect(GetNHApp()->hMainWnd, &client_rt);
        data = (PNHMainWindow) GetWindowLongPtr(GetNHApp()->hMainWnd,
                                                GWLP_USERDATA);

        /* get sizes of child windows */
        wnd_status = mswin_hwnd_from_winid(WIN_STATUS);
        if (IsWindow(wnd_status)) {
            mswin_status_window_size(wnd_status, &status_size);
        } else {
            status_size.cx = status_size.cy = 0;
        }

        wnd_msg = mswin_hwnd_from_winid(WIN_MESSAGE);
        if (IsWindow(wnd_msg)) {
            mswin_message_window_size(wnd_msg, &msg_size);
        } else {
            msg_size.cx = msg_size.cy = 0;
        }

        /* find all menu windows and calculate the size */
        menu_size.cx = menu_size.cy = 0;
        for (i = 0; i < MAXWINDOWS; i++) {
            SIZE tmp_size;
            if (GetNHApp()->windowlist[i].win
                && !GetNHApp()->windowlist[i].dead
                && GetNHApp()->windowlist[i].type == NHW_MENU) {
                mswin_menu_window_size(GetNHApp()->windowlist[i].win,
                                       &tmp_size);
                menu_size.cx = max(menu_size.cx, tmp_size.cx);
                menu_size.cy = max(menu_size.cy, tmp_size.cy);
            }
        }

        /* set window positions */
        SetRect(&wnd_rect, client_rt.left, client_rt.top, client_rt.right,
                client_rt.bottom);
        switch (iflags.wc_align_status) {
        case ALIGN_LEFT:
            status_size.cx = (wnd_rect.right - wnd_rect.left) / 4;
            status_size.cy =
                (wnd_rect.bottom - wnd_rect.top); // that won't look good
            status_org.x = wnd_rect.left;
            status_org.y = wnd_rect.top;
            wnd_rect.left += status_size.cx;
            break;

        case ALIGN_RIGHT:
            status_size.cx = (wnd_rect.right - wnd_rect.left) / 4;
            status_size.cy =
                (wnd_rect.bottom - wnd_rect.top); // that won't look good
            status_org.x = wnd_rect.right - status_size.cx;
            status_org.y = wnd_rect.top;
            wnd_rect.right -= status_size.cx;
            break;

        case ALIGN_TOP:
            status_size.cx = (wnd_rect.right - wnd_rect.left);
            status_org.x = wnd_rect.left;
            status_org.y = wnd_rect.top;
            wnd_rect.top += status_size.cy;
            break;

        case ALIGN_BOTTOM:
        default:
            status_size.cx = (wnd_rect.right - wnd_rect.left);
            status_org.x = wnd_rect.left;
            status_org.y = wnd_rect.bottom - status_size.cy;
            wnd_rect.bottom -= status_size.cy;
            break;
        }

        switch (iflags.wc_align_message) {
        case ALIGN_LEFT:
            msg_size.cx = (wnd_rect.right - wnd_rect.left) / 4;
            msg_size.cy = (wnd_rect.bottom - wnd_rect.top);
            msg_org.x = wnd_rect.left;
            msg_org.y = wnd_rect.top;
            wnd_rect.left += msg_size.cx;
            break;

        case ALIGN_RIGHT:
            msg_size.cx = (wnd_rect.right - wnd_rect.left) / 4;
            msg_size.cy = (wnd_rect.bottom - wnd_rect.top);
            msg_org.x = wnd_rect.right - msg_size.cx;
            msg_org.y = wnd_rect.top;
            wnd_rect.right -= msg_size.cx;
            break;

        case ALIGN_TOP:
            msg_size.cx = (wnd_rect.right - wnd_rect.left);
            msg_org.x = wnd_rect.left;
            msg_org.y = wnd_rect.top;
            wnd_rect.top += msg_size.cy;
            break;

        case ALIGN_BOTTOM:
        default:
            msg_size.cx = (wnd_rect.right - wnd_rect.left);
            msg_org.x = wnd_rect.left;
            msg_org.y = wnd_rect.bottom - msg_size.cy;
            wnd_rect.bottom -= msg_size.cy;
            break;
        }

        /* map window */
        map_org.x = wnd_rect.left;
        map_org.y = wnd_rect.top;
        map_size.cx = wnd_rect.right - wnd_rect.left;
        map_size.cy = wnd_rect.bottom - wnd_rect.top;

        GetNHApp()->rtStatusWindow.left = status_org.x;
        GetNHApp()->rtStatusWindow.top = status_org.y;
        GetNHApp()->rtStatusWindow.right = status_org.x + status_size.cx;
        GetNHApp()->rtStatusWindow.bottom = status_org.y + status_size.cy;

        GetNHApp()->rtTextWindow.left = map_org.x;
        GetNHApp()->rtTextWindow.top = map_org.y;
        GetNHApp()->rtTextWindow.right =
            map_org.x + (wnd_rect.right - wnd_rect.left);
        GetNHApp()->rtTextWindow.bottom = map_org.y + map_size.cy;

        GetNHApp()->rtMapWindow.left = map_org.x;
        GetNHApp()->rtMapWindow.top = map_org.y;
        GetNHApp()->rtMapWindow.right = map_org.x + map_size.cx;
        GetNHApp()->rtMapWindow.bottom = map_org.y + map_size.cy;

        GetNHApp()->rtMsgWindow.left = msg_org.x;
        GetNHApp()->rtMsgWindow.top = msg_org.y;
        GetNHApp()->rtMsgWindow.right = msg_org.x + msg_size.cx;
        GetNHApp()->rtMsgWindow.bottom = msg_org.y + msg_size.cy;

        /*  map_width/4 < menu_width < map_width*2/3 */
        GetNHApp()->rtMenuWindow.left =
            GetNHApp()->rtMapWindow.right
            - min(map_size.cx * 2 / 3, max(map_size.cx / 4, menu_size.cx));
        GetNHApp()->rtMenuWindow.top = GetNHApp()->rtMapWindow.top;
        GetNHApp()->rtMenuWindow.right = GetNHApp()->rtMapWindow.right;
        GetNHApp()->rtMenuWindow.bottom = GetNHApp()->rtMapWindow.bottom;

        GetNHApp()->rtInvenWindow.left = GetNHApp()->rtMenuWindow.left;
        GetNHApp()->rtInvenWindow.top = GetNHApp()->rtMenuWindow.top;
        GetNHApp()->rtInvenWindow.right = GetNHApp()->rtMenuWindow.right;
        GetNHApp()->rtInvenWindow.bottom = GetNHApp()->rtMenuWindow.bottom;

        /* adjust map window size only if perm_invent is set */
        if (iflags.perm_invent)
            GetNHApp()->rtMapWindow.right = GetNHApp()->rtMenuWindow.left;
    }

    /* go through the windows list and adjust sizes */
    for (i = 0; i < MAXWINDOWS; i++) {
        if (GetNHApp()->windowlist[i].win
            && !GetNHApp()->windowlist[i].dead) {
            RECT rt;
            /* kludge - inventory window should have its own type (same as
               menu-text
               as a matter of fact) */
            if (iflags.perm_invent && i == WIN_INVEN) {
                mswin_get_window_placement(NHW_INVEN, &rt);
            } else {
                mswin_get_window_placement(GetNHApp()->windowlist[i].type,
                                           &rt);
            }

            MoveWindow(GetNHApp()->windowlist[i].win, rt.left, rt.top,
                       rt.right - rt.left, rt.bottom - rt.top, TRUE);
        }
    }
    if (IsWindow(changed_child))
        SetForegroundWindow(changed_child);
}

VOID CALLBACK FuzzTimerProc(
    _In_ HWND     hwnd,
    _In_ UINT     uMsg,
    _In_ UINT_PTR idEvent,
    _In_ DWORD    dwTime
    )
{
        INPUT input[16];
        int i_pos = 0;
        int c = randomkey();
        SHORT k = VkKeyScanA(c);
        BOOL gen_alt = (rn2(50) == 0) && isalpha(c);

        if (!iflags.debug_fuzzer) {
        	KillTimer(hwnd, IDT_FUZZ_TIMER);
        	return;
        }

        if (!GetFocus())
            return;

        ZeroMemory(input, sizeof(input));
        if (gen_alt) {
        	input[i_pos].type = INPUT_KEYBOARD;
        	input[i_pos].ki.dwFlags = KEYEVENTF_SCANCODE;
        	input[i_pos].ki.wScan = MapVirtualKey(VK_MENU, 0);
        	i_pos++;
        }

        if (HIBYTE(k) & 1) {
        	input[i_pos].type = INPUT_KEYBOARD;
        	input[i_pos].ki.dwFlags = KEYEVENTF_SCANCODE;
        	input[i_pos].ki.wScan = MapVirtualKey(VK_LSHIFT, 0);
        	i_pos++;
        }

        input[i_pos].type = INPUT_KEYBOARD;
        input[i_pos].ki.dwFlags = KEYEVENTF_SCANCODE;
        input[i_pos].ki.wScan = MapVirtualKey(LOBYTE(k), 0);
        i_pos++;

        if (HIBYTE(k) & 1) {
        	input[i_pos].type = INPUT_KEYBOARD;
        	input[i_pos].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        	input[i_pos].ki.wScan = MapVirtualKey(VK_LSHIFT, 0);
        	i_pos++;
        }
        if (gen_alt) {
        	input[i_pos].type = INPUT_KEYBOARD;
        	input[i_pos].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        	input[i_pos].ki.wScan = MapVirtualKey(VK_MENU, 0);
        	i_pos++;
        }
        SendInput(i_pos, input, sizeof(input[0]));
}

LRESULT
onWMCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PNHMainWindow data;

    UNREFERENCED_PARAMETER(lParam);

    data = (PNHMainWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    wmId = LOWORD(wParam);
    wmEvent = HIWORD(wParam);

    // Parse the menu selections:
    switch (wmId) {
    case IDM_ABOUT:
        mswin_display_splash_window(TRUE);
        break;

    case IDM_FUZZ:
        if (iflags.debug_fuzzer)
            KillTimer(hWnd, IDT_FUZZ_TIMER);
        else
            SetTimer(hWnd, IDT_FUZZ_TIMER, 10, FuzzTimerProc);
        iflags.debug_fuzzer = !iflags.debug_fuzzer;
        break;
    case IDM_EXIT:
        if (iflags.debug_fuzzer)
            break;
        done2();
        break;

    case IDM_SAVE:
        if (iflags.debug_fuzzer)
            break;
        if (!g.program_state.gameover && !g.program_state.done_hup)
            dosave();
        else
            MessageBeep(0);
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
        if (IS_MAP_FIT_TO_SCREEN(iflags.wc_map_mode)) {
            mswin_select_map_mode(IS_MAP_ASCII(iflags.wc_map_mode)
                                      ? data->mapAcsiiModeSave
                                      : MAP_MODE_TILES);
        } else {
            mswin_select_map_mode(IS_MAP_ASCII(iflags.wc_map_mode)
                                      ? MAP_MODE_ASCII_FIT_TO_SCREEN
                                      : MAP_MODE_TILES_FIT_TO_SCREEN);
        }
        break;

    case IDM_SETTING_SCREEN_TO_CLIPBOARD: {
        char *p;
        size_t len;
        HANDLE hglbCopy;
        char *p_copy;

        p = nh_compose_ascii_screenshot();
        if (!p)
            return 0;
        len = strlen(p);

        if (!OpenClipboard(hWnd)) {
            NHMessageBox(hWnd, TEXT("Cannot open clipboard"),
                         MB_OK | MB_ICONERROR);
            free(p);
            return 0;
        }

        EmptyClipboard();

        hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(char));
        if (hglbCopy == NULL) {
            CloseClipboard();
            free(p);
            return FALSE;
        }

        p_copy = (char *) GlobalLock(hglbCopy);
        strncpy(p_copy, p, len);
        p_copy[len] = 0; // null character
        GlobalUnlock(hglbCopy);

        SetClipboardData(SYMHANDLING(H_IBM) ? CF_OEMTEXT : CF_TEXT, hglbCopy);

        CloseClipboard();

        free(p);
    } break;

    case IDM_SETTING_SCREEN_TO_FILE: {
        OPENFILENAME ofn;
        TCHAR filename[1024];
        TCHAR whackdir[MAX_PATH];
        FILE *pFile;
        char *text;
        wchar_t *wtext;
        int tlen = 0;

        if (iflags.debug_fuzzer)
            break;

        ZeroMemory(filename, sizeof(filename));
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hWnd;
        ofn.hInstance = GetNHApp()->hApp;
        ofn.lpstrFilter = TEXT("Text Files (*.txt)\x0*.txt\x0")
            TEXT("All Files (*.*)\x0*.*\x0") TEXT("\x0\x0");
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = SIZE(filename);
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NH_A2W(g.hackdir, whackdir, MAX_PATH);
        ofn.lpstrTitle = NULL;
        ofn.Flags = OFN_LONGNAMES | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = TEXT("txt");
        ofn.lCustData = 0;
        ofn.lpfnHook = 0;
        ofn.lpTemplateName = 0;

        if (!GetSaveFileName(&ofn))
            return FALSE;

        text = nh_compose_ascii_screenshot();
        if (!text)
            return FALSE;

        pFile = _tfopen(filename, TEXT("wt+,ccs=UTF-8"));
        if (!pFile) {
            TCHAR buf[4096];
            _stprintf(buf, TEXT("Cannot open %s for writing!"), filename);
            NHMessageBox(hWnd, buf, MB_OK | MB_ICONERROR);
            free(text);
            return FALSE;
        }

        tlen = strlen(text);
        wtext = (wchar_t *) malloc(tlen * sizeof(wchar_t));
        if (!wtext)
            panic("out of memory");
        MultiByteToWideChar(NH_CODEPAGE, 0, text, -1, wtext, tlen);
        fwrite(wtext, tlen * sizeof(wchar_t), 1, pFile);
        fclose(pFile);
        free(text);
        free(wtext);
    } break;

    case IDM_NHMODE: {
        GetNHApp()->regNetHackMode = GetNHApp()->regNetHackMode ? 0 : 1;
        mswin_menu_check_intf_mode();
        mswin_apply_window_style_all();
        break;
    }
    case IDM_CLEARSETTINGS: {
        mswin_destroy_reg();
        /* Notify the user that windows settings will not be saved this time.
         */
        NHMessageBox(GetNHApp()->hMainWnd,
                     TEXT("Your Windows Settings will not be stored when you "
                          "exit this time."),
                     MB_OK | MB_ICONINFORMATION);
        break;
    }

    case IDM_SETTING_AUTOLAYOUT:
        GetNHApp()->bAutoLayout = !GetNHApp()->bAutoLayout;
        mswin_layout_main_window(NULL);

        /* Update menu item check-mark */
        CheckMenuItem(GetMenu(GetNHApp()->hMainWnd), IDM_SETTING_AUTOLAYOUT,
                      GetNHApp()->bAutoLayout ? MF_CHECKED : MF_UNCHECKED);
        break;

    case IDM_SETTING_LOCKWINDOWS:
        nhlock_windows(!GetNHApp()->bWindowsLocked);
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

    case IDM_HELP_PORTHELP:
        display_file(PORT_HELP, TRUE);
        break;

    default:
        return 1;
    }
    return 0;
}

// Mesage handler for about box.
LRESULT CALLBACK
About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buf[BUFSZ];
    TCHAR wbuf[BUFSZ];
    RECT main_rt, dlg_rt;
    SIZE dlg_sz;

    UNREFERENCED_PARAMETER(lParam);

    switch (message) {
    case WM_INITDIALOG:
        getversionstring(buf, sizeof buf);
        SetDlgItemText(hDlg, IDC_ABOUT_VERSION,
                       NH_A2W(buf, wbuf, sizeof(wbuf)));

        Sprintf(buf, "%s\n%s\n%s\n%s",
                COPYRIGHT_BANNER_A, COPYRIGHT_BANNER_B,
                nomakedefs.copyright_banner_c, COPYRIGHT_BANNER_D);
        SetDlgItemText(hDlg, IDC_ABOUT_COPYRIGHT,
                       NH_A2W(buf, wbuf, sizeof(wbuf)));

        /* center dialog in the main window */
        GetWindowRect(GetNHApp()->hMainWnd, &main_rt);
        GetWindowRect(hDlg, &dlg_rt);
        dlg_sz.cx = dlg_rt.right - dlg_rt.left;
        dlg_sz.cy = dlg_rt.bottom - dlg_rt.top;

        dlg_rt.left = (main_rt.left + main_rt.right - dlg_sz.cx) / 2;
        dlg_rt.right = dlg_rt.left + dlg_sz.cx;
        dlg_rt.top = (main_rt.top + main_rt.bottom - dlg_sz.cy) / 2;
        dlg_rt.bottom = dlg_rt.top + dlg_sz.cy;
        MoveWindow(hDlg, (main_rt.left + main_rt.right - dlg_sz.cx) / 2,
                   (main_rt.top + main_rt.bottom - dlg_sz.cy) / 2, dlg_sz.cx,
                   dlg_sz.cy, TRUE);

        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void
mswin_menu_check_intf_mode(void)
{
    HMENU hMenu = GetMenu(GetNHApp()->hMainWnd);

    if (GetNHApp()->regNetHackMode)
        CheckMenuItem(hMenu, IDM_NHMODE, MF_CHECKED);
    else
        CheckMenuItem(hMenu, IDM_NHMODE, MF_UNCHECKED);
}

void
mswin_select_map_mode(int mode)
{
    PNHMainWindow data;
    winid map_id;

    map_id = WIN_MAP;
    data =
        (PNHMainWindow) GetWindowLongPtr(GetNHApp()->hMainWnd, GWLP_USERDATA);

    /* override for Rogue level */
    if (Is_rogue_level(&u.uz) && !IS_MAP_ASCII(mode))
        return;

    /* set map mode menu mark */
    if (IS_MAP_ASCII(mode)) {
        CheckMenuRadioItem(
            GetMenu(GetNHApp()->hMainWnd), IDM_MAP_TILES, IDM_MAP_ASCII10X18,
            mapmode2menuid(IS_MAP_FIT_TO_SCREEN(mode) ? data->mapAcsiiModeSave
                                                      : mode),
            MF_BYCOMMAND);
    } else {
        CheckMenuRadioItem(GetMenu(GetNHApp()->hMainWnd), IDM_MAP_TILES,
                           IDM_MAP_ASCII10X18, mapmode2menuid(MAP_MODE_TILES),
                           MF_BYCOMMAND);
    }

    /* set fit-to-screen mode mark */
    CheckMenuItem(GetMenu(GetNHApp()->hMainWnd), IDM_MAP_FIT_TO_SCREEN,
                  MF_BYCOMMAND | (IS_MAP_FIT_TO_SCREEN(mode) ? MF_CHECKED
                                                             : MF_UNCHECKED));

    if (IS_MAP_ASCII(iflags.wc_map_mode)
        && !IS_MAP_FIT_TO_SCREEN(iflags.wc_map_mode)) {
        data->mapAcsiiModeSave = iflags.wc_map_mode;
    }

    iflags.wc_map_mode = mode;

    /*
    ** first, check if WIN_MAP has been inialized.
    ** If not - attempt to retrieve it by type, then check it again
    */
    if (map_id == WIN_ERR)
        map_id = mswin_winid_from_type(NHW_MAP);
    if (map_id != WIN_ERR)
        mswin_map_mode(mswin_hwnd_from_winid(map_id), mode);
}

static struct t_menu2mapmode {
    int menuID;
    int mapMode;
} _menu2mapmode[] = { { IDM_MAP_TILES, MAP_MODE_TILES },
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
                      { -1, -1 } };

int
menuid2mapmode(int menuid)
{
    struct t_menu2mapmode *p;
    for (p = _menu2mapmode; p->mapMode != -1; p++)
        if (p->menuID == menuid)
            return p->mapMode;
    return -1;
}

int
mapmode2menuid(int map_mode)
{
    struct t_menu2mapmode *p;
    for (p = _menu2mapmode; p->mapMode != -1; p++)
        if (p->mapMode == map_mode)
            return p->menuID;
    return -1;
}

void
nhlock_windows(BOOL lock)
{
    /* update menu */
    GetNHApp()->bWindowsLocked = lock;
    CheckMenuItem(GetMenu(GetNHApp()->hMainWnd), IDM_SETTING_LOCKWINDOWS,
                  MF_BYCOMMAND | (lock ? MF_CHECKED : MF_UNCHECKED));

    /* restyle windows */
    mswin_apply_window_style_all();
}

void
mswin_apply_window_style(HWND hwnd) {
    DWORD style = 0, exstyle = 0;

    style = GetWindowLong(hwnd, GWL_STYLE);
    exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    if( !GetNHApp()->bWindowsLocked ) {
        style = WS_CHILD|WS_CLIPSIBLINGS|WS_CAPTION|WS_SIZEBOX|(style & (WS_VISIBLE|WS_VSCROLL|WS_HSCROLL));
        exstyle = WS_EX_WINDOWEDGE;
    } else if (GetNHApp()->regNetHackMode) {
        /* do away borders */
        style = WS_CHILD|WS_CLIPSIBLINGS|(style & (WS_VISIBLE|WS_VSCROLL|WS_HSCROLL));
        exstyle = 0;
    } else {
        style = WS_CHILD|WS_CLIPSIBLINGS|WS_THICKFRAME|(style & (WS_VISIBLE|WS_VSCROLL|WS_HSCROLL));
        exstyle = WS_EX_WINDOWEDGE;
    }

    SetWindowLong(hwnd, GWL_STYLE, style);
    SetWindowLong(hwnd, GWL_EXSTYLE, exstyle);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOCOPYBITS);
}

void
mswin_apply_window_style_all(void)
{
    int i;
    for (i = 0; i < MAXWINDOWS; i++) {
        if (IsWindow(GetNHApp()->windowlist[i].win)
            && !GetNHApp()->windowlist[i].dead) {
            mswin_apply_window_style(GetNHApp()->windowlist[i].win);
        }
    }
    mswin_layout_main_window(NULL);
}

// returns strdup() created pointer - callee assumes the ownership
char *
nh_compose_ascii_screenshot(void)
{
    char *retval;
    PMSNHMsgGetText text;

    retval = (char *) malloc(3 * TEXT_BUFFER_SIZE);

    text =
        (PMSNHMsgGetText) malloc(sizeof(MSNHMsgGetText) + TEXT_BUFFER_SIZE);
    text->max_size =
        TEXT_BUFFER_SIZE
        - 1; /* make sure we always have 0 at the end of the buffer */

    ZeroMemory(text->buffer, TEXT_BUFFER_SIZE);
    SendMessage(mswin_hwnd_from_winid(WIN_MESSAGE), WM_MSNH_COMMAND,
                (WPARAM) MSNH_MSG_GETTEXT, (LPARAM) text);
    strcpy(retval, text->buffer);

    ZeroMemory(text->buffer, TEXT_BUFFER_SIZE);
    SendMessage(mswin_hwnd_from_winid(WIN_MAP), WM_MSNH_COMMAND,
                (WPARAM) MSNH_MSG_GETTEXT, (LPARAM) text);
    strcat(retval, text->buffer);

    ZeroMemory(text->buffer, TEXT_BUFFER_SIZE);
    SendMessage(mswin_hwnd_from_winid(WIN_STATUS), WM_MSNH_COMMAND,
                (WPARAM) MSNH_MSG_GETTEXT, (LPARAM) text);
    strcat(retval, text->buffer);

    free(text);
    return retval;
}

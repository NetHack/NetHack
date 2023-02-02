/* NetHack 3.6	mhmain.c	$NHDT-Date: 1432512800 2015/05/25 00:13:20 $  $NHDT-Branch: master $:$NHDT-Revision: 1.46 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "mhmsg.h"
#include "mhinput.h"
#include "mhmain.h"
#include "mhmenu.h"
#include "mhstatus.h"
#include "mhmsgwnd.h"
#include "mhcmd.h"
#include "mhmap.h"
#include "date.h"
#include "patchlevel.h"

#define MAX_LOADSTRING 100

typedef struct mswin_nethack_main_window {
    int mapAcsiiModeSave;
} NHMainWindow, *PNHMainWindow;

TCHAR szMainWindowClass[] = TEXT("MSNHMainWndClass");
static TCHAR szTitle[MAX_LOADSTRING];

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
static LRESULT onWMCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void register_main_window_class();
static void select_map_mode(int map_mode);
static int menuid2mapmode(int menuid);
static int mapmode2menuid(int map_mode);
static HMENU _get_main_menu(UINT menu_id);
static void mswin_direct_command();

HWND
mswin_init_main_window()
{
    static int run_once = 0;
    HWND ret;
    RECT rc;

    /* register window class */
    if (!run_once) {
        LoadString(GetNHApp()->hApp, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
        register_main_window_class();
        run_once = 1;
    }

    /* create the main window */
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);

    ret = CreateWindow(szMainWindowClass,  /* registered class name */
                       szTitle,            /* window name */
                       WS_CLIPCHILDREN,    /* window style */
                       rc.left,            /* horizontal position of window */
                       rc.top,             /* vertical position of window */
                       rc.right - rc.left, /* window width */
                       rc.bottom - rc.top, /* window height */
                       NULL, /* handle to parent or owner window */
                       NULL, /* menu handle or child identifier */
                       GetNHApp()->hApp, /* handle to application instance */
                       NULL              /* window-creation data */
                       );

    if (!ret)
        panic("Cannot create main window");
    return ret;
}

void
register_main_window_class()
{
    WNDCLASS wcex;

    ZeroMemory(&wcex, sizeof(wcex));
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC) MainWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetNHApp()->hApp;
    wcex.hIcon = LoadIcon(GetNHApp()->hApp, (LPCTSTR) IDI_WINHACK);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
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
        { 'g', 'G', 'g' },       /* 5 */
        { '6', M('6'), '6' },    /* 6 */
        { '+', 'P', C('p') },    /* + */
        { '1', M('1'), '1' },    /* 1 */
        { '2', M('2'), '2' },    /* 2 */
        { '3', M('3'), '3' },    /* 3 */
        { 'i', 'I', C('i') },    /* Ins */
        { '.', ':', ':' }        /* Del */
    };

#define STATEON(x) ((GetKeyState(x) & 0xFFFE) != 0)
#define KEYTABLE_REGULAR(x) ((iflags.num_pad ? numpad : keypad)[x][0])
#define KEYTABLE_SHIFT(x) ((iflags.num_pad ? numpad : keypad)[x][1])
#define KEYTABLE(x) \
    (STATEON(VK_SHIFT) ? KEYTABLE_SHIFT(x) : KEYTABLE_REGULAR(x))

/* map mode macros */
#define IS_MAP_FIT_TO_SCREEN(mode)          \
    ((mode) == MAP_MODE_ASCII_FIT_TO_SCREEN \
     || (mode) == MAP_MODE_TILES_FIT_TO_SCREEN)

#define IS_MAP_ASCII(mode) \
    ((mode) != MAP_MODE_TILES && (mode) != MAP_MODE_TILES_FIT_TO_SCREEN)

/*
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
*/
LRESULT CALLBACK
MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PNHMainWindow data;

    switch (message) {
    /*-----------------------------------------------------------------------*/
    case WM_CREATE: {
#if defined(WIN_CE_POCKETPC) || defined(WIN_CE_SMARTPHONE)
        SHMENUBARINFO menubar;
#endif
        /* set window data */
        data = (PNHMainWindow) malloc(sizeof(NHMainWindow));
        if (!data)
            panic("out of memory");
        ZeroMemory(data, sizeof(NHMainWindow));
        data->mapAcsiiModeSave = MAP_MODE_ASCII12x16;
        SetWindowLong(hWnd, GWL_USERDATA, (LONG) data);

        GetNHApp()->hMainWnd = hWnd;

/* create menu bar */
#if defined(WIN_CE_POCKETPC) || defined(WIN_CE_SMARTPHONE)
        ZeroMemory(&menubar, sizeof(menubar));
        menubar.cbSize = sizeof(menubar);
        menubar.hwndParent = hWnd;
        menubar.dwFlags = 0;
        menubar.nToolBarId = IDC_WINHACK;
        menubar.hInstRes = GetNHApp()->hApp;
#if defined(WIN_CE_POCKETPC)
        menubar.nBmpId = IDB_MENUBAR;
        menubar.cBmpImages = 2;
#else
        menubar.nBmpId = 0;
        menubar.cBmpImages = 0;
#endif
        if (!SHCreateMenuBar(&menubar))
            panic("cannot create menu");
        GetNHApp()->hMenuBar = menubar.hwndMB;
#else
        GetNHApp()->hMenuBar = CommandBar_Create(GetNHApp()->hApp, hWnd, 1);
        if (!GetNHApp()->hMenuBar)
            panic("cannot create menu");
        CommandBar_InsertMenubar(GetNHApp()->hMenuBar, GetNHApp()->hApp,
                                 IDC_WINHACK, 0);
#endif
        CheckMenuItem(
            _get_main_menu(ID_VIEW), IDM_VIEW_KEYPAD,
            MF_BYCOMMAND | (GetNHApp()->bCmdPad ? MF_CHECKED : MF_UNCHECKED));

    } break;

    /*-----------------------------------------------------------------------*/

    case WM_MSNH_COMMAND:
        onMSNHCommand(hWnd, wParam, lParam);
        break;

    /*-----------------------------------------------------------------------*/

    case WM_KEYDOWN:
        data = (PNHMainWindow) GetWindowLong(hWnd, GWL_USERDATA);

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

        case VK_RETURN: {
            int x, y;
            if (WIN_MAP != WIN_ERR) {
                mswin_map_get_cursor(mswin_hwnd_from_winid(WIN_MAP), &x, &y);
            } else {
                x = u.ux;
                y = u.uy;
            }
            NHEVENT_MS(CLICK_1, x, y);
        }
            return 0;
        }

#if defined(WIN_CE_SMARTPHONE)
        if (GetNHApp()->bCmdPad
            && NHSPhoneTranslateKbdMessage(wParam, lParam, TRUE))
            return 0;
#endif
        return 1; /* end of WM_KEYDOWN */

/*-----------------------------------------------------------------------*/

#if defined(WIN_CE_SMARTPHONE)
    case WM_KEYUP:
        if (GetNHApp()->bCmdPad
            && NHSPhoneTranslateKbdMessage(wParam, lParam, FALSE))
            return 0;
        return 1; /* end of WM_KEYUP */
#endif
    /*-----------------------------------------------------------------------*/

    case WM_CHAR:
#if defined(WIN_CE_SMARTPHONE)
        /* if smartphone cmdpad is up then translation happens - disable
           WM_CHAR processing
           to avoid double input */
        if (GetNHApp()->bCmdPad) {
            return 1;
        }
#endif

        if (wParam == '\n' || wParam == '\r' || wParam == C('M'))
            return 0; /* we already processed VK_RETURN */

        /* all characters go to nethack except Ctrl-P that scrolls message
         * window up */
        if (wParam == C('P') || wParam == C('p')) {
            SendMessage(mswin_hwnd_from_winid(WIN_MESSAGE), WM_VSCROLL,
                        MAKEWPARAM(SB_LINEUP, 0), (LPARAM) NULL);
        } else {
            NHEVENT_KBD((lParam & 1 << 29) ? M(tolower(wParam)) : wParam);
        }
        return 0;

    /*-----------------------------------------------------------------------*/

    case WM_COMMAND:
        /* process commands - menu commands mostly */
        if (IsWindow(GetNHApp()->hPopupWnd)) {
            return SendMessage(GetNHApp()->hPopupWnd, message, wParam,
                               lParam);
        } else if (onWMCommand(hWnd, wParam, lParam))
            return DefWindowProc(hWnd, message, wParam, lParam);
        else
            return 0;

    /*-----------------------------------------------------------------------*/

    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE) {
#if defined(WIN_CE_POCKETPC) || defined(WIN_CE_SMARTPHONE)
            if (GetNHApp()->bFullScreen)
                SHFullScreen(GetNHApp()->hMainWnd,
                             SHFS_HIDETASKBAR | SHFS_HIDESTARTICON);
            else
                SHFullScreen(GetNHApp()->hMainWnd,
                             SHFS_SHOWTASKBAR | SHFS_SHOWSTARTICON);
#endif
            mswin_layout_main_window(NULL);
        }
        break;

    case WM_SETTINGCHANGE:
#if defined(WIN_CE_POCKETPC) || defined(WIN_CE_SMARTPHONE)
        if (GetNHApp()->bFullScreen)
            SHFullScreen(GetNHApp()->hMainWnd,
                         SHFS_HIDETASKBAR | SHFS_HIDESTARTICON);
        else
            SHFullScreen(GetNHApp()->hMainWnd,
                         SHFS_SHOWTASKBAR | SHFS_SHOWSTARTICON);
#endif
        mswin_layout_main_window(NULL);
        break;

    case WM_SIZE:
        mswin_layout_main_window(NULL);
        break;

    /*-----------------------------------------------------------------------*/

    case WM_SETFOCUS:
        /* if there is a menu window out there -
           transfer input focus to it */
        if (IsWindow(GetNHApp()->hPopupWnd)) {
            SetFocus(GetNHApp()->hPopupWnd);
        }
        break;

    /*-----------------------------------------------------------------------*/

    case WM_CLOSE: {
/* exit gracefully */
#ifdef SAFERHANGUP
        /* destroy popup window - it has its own loop and we need to
           return control to NetHack core at this point */
        if (IsWindow(GetNHApp()->hPopupWnd))
            SendMessage(GetNHApp()->hPopupWnd, WM_COMMAND, IDCANCEL, 0);

        /* tell NetHack core that "hangup" is requested */
        hangup(1);
#else
        dosave0();
        nh_terminate(EXIT_SUCCESS);
#endif
    }
        return 0;

    /*-----------------------------------------------------------------------*/

    case WM_DESTROY: {
        /* apparently we never get here
           TODO: work on exit routines - need to send
           WM_QUIT somehow */

        /* clean up */
        free((PNHMainWindow) GetWindowLong(hWnd, GWL_USERDATA));
        SetWindowLong(hWnd, GWL_USERDATA, (LONG) 0);

        nh_terminate(EXIT_SUCCESS);
    } break;

    /*-----------------------------------------------------------------------*/

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void
onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam) {
    /* new window was just added */
    case MSNH_MSG_ADDWND: {
        PMSNHMsgAddWnd msg_param = (PMSNHMsgAddWnd) lParam;
        HWND child = GetNHApp()->windowlist[msg_param->wid].win;

        if (GetNHApp()->windowlist[msg_param->wid].type == NHW_MAP)
            mswin_select_map_mode(iflags.wc_map_mode);

        if (child)
            mswin_layout_main_window(child);
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
   |      Command pad        |
   +-------------------------+
   |        Messages         |
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
    POINT cmd_org;
    SIZE cmd_size;
    HWND wnd_status, wnd_msg;
    PNHMainWindow data;
#if defined(WIN_CE_POCKETPC)
    SIPINFO sip;
    RECT menu_bar;
    RECT visible_rt;
    POINT pt;
#endif

    GetClientRect(GetNHApp()->hMainWnd, &client_rt);

#if defined(WIN_CE_POCKETPC)
    ZeroMemory(&sip, sizeof(sip));
    sip.cbSize = sizeof(sip);
    SHSipInfo(SPI_GETSIPINFO, 0, &sip, 0);
    if (GetNHApp()->bFullScreen)
        sip.rcVisibleDesktop.top = 0;

    /* adjust client rectangle size */
    GetWindowRect(GetNHApp()->hMenuBar, &menu_bar);
    client_rt.bottom -= menu_bar.bottom - menu_bar.top;

    /* calcuate visible rect in client coordinates */
    pt.x = sip.rcVisibleDesktop.left;
    pt.y = sip.rcVisibleDesktop.top;
    ScreenToClient(GetNHApp()->hMainWnd, &pt);
    SetRect(&wnd_rect, pt.x, pt.y,
            pt.x + sip.rcVisibleDesktop.right - sip.rcVisibleDesktop.left,
            pt.y + sip.rcVisibleDesktop.bottom - sip.rcVisibleDesktop.top);
    IntersectRect(&visible_rt, &client_rt, &wnd_rect);
#else
#if !defined(WIN_CE_SMARTPHONE)
    client_rt.top += CommandBar_Height(GetNHApp()->hMenuBar);
#else
    /* Smartphone only */
    if (GetNHApp()->bFullScreen) {
        RECT menu_bar;
        GetWindowRect(GetNHApp()->hMenuBar, &menu_bar);
        client_rt.bottom -= menu_bar.bottom - menu_bar.top;
    }
#endif
#endif

    /* get window data */
    data = (PNHMainWindow) GetWindowLong(GetNHApp()->hMainWnd, GWL_USERDATA);

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

    cmd_size.cx = cmd_size.cy = 0;
    if (GetNHApp()->bCmdPad && IsWindow(GetNHApp()->hCmdWnd)) {
        mswin_command_window_size(GetNHApp()->hCmdWnd, &cmd_size);
    }

/* set window positions */

/* calculate the application windows size */
#if defined(WIN_CE_POCKETPC)
    SetRect(&wnd_rect, visible_rt.left, visible_rt.top, visible_rt.right,
            visible_rt.bottom);
    if (sip.fdwFlags & SIPF_ON)
        cmd_size.cx = cmd_size.cy = 0; /* hide keypad window */
#else
    SetRect(&wnd_rect, client_rt.left, client_rt.top, client_rt.right,
            client_rt.bottom);
#endif

#if !defined(WIN_CE_SMARTPHONE)
    /* other ports have it at the bottom of the screen */
    cmd_size.cx = (wnd_rect.right - wnd_rect.left);
    cmd_org.x = wnd_rect.left;
    cmd_org.y = wnd_rect.bottom - cmd_size.cy;
    wnd_rect.bottom -= cmd_size.cy;
#endif

    /* status window */
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

    /* message window */
    switch (iflags.wc_align_message) {
    case ALIGN_LEFT:
#if defined(WIN_CE_SMARTPHONE)
        /* smartphone has a keypad window on the right (bottom) side of the
         * message window */
        msg_size.cx = cmd_size.cx = max(msg_size.cx, cmd_size.cx);
        msg_size.cy = (wnd_rect.bottom - wnd_rect.top) - cmd_size.cy;
        msg_org.x = cmd_org.x = wnd_rect.left;
        msg_org.y = wnd_rect.top;
        cmd_org.y = msg_org.y + msg_size.cy;
#else
        msg_size.cx = (wnd_rect.right - wnd_rect.left) / 4;
        msg_size.cy = (wnd_rect.bottom - wnd_rect.top);
        msg_org.x = wnd_rect.left;
        msg_org.y = wnd_rect.top;
#endif
        wnd_rect.left += msg_size.cx;

        break;

    case ALIGN_RIGHT:
#if defined(WIN_CE_SMARTPHONE)
        /* smartphone has a keypad window on the right (bottom) side of the
         * message window */
        msg_size.cx = cmd_size.cx = max(msg_size.cx, cmd_size.cx);
        msg_size.cy = (wnd_rect.bottom - wnd_rect.top) - cmd_size.cy;
        msg_org.x = cmd_org.x = wnd_rect.right - msg_size.cx;
        msg_org.y = wnd_rect.top;
        cmd_org.y = msg_org.y + msg_size.cy;
#else
        msg_size.cx = (wnd_rect.right - wnd_rect.left) / 4;
        msg_size.cy = (wnd_rect.bottom - wnd_rect.top);
        msg_org.x = wnd_rect.right - msg_size.cx;
        msg_org.y = wnd_rect.top;
#endif

        wnd_rect.right -= msg_size.cx;
        break;

    case ALIGN_TOP:
#if defined(WIN_CE_SMARTPHONE)
        /* smartphone has a keypad window on the right side of the message
         * window */
        msg_size.cy = cmd_size.cy = max(msg_size.cy, cmd_size.cy);
        msg_size.cx = (wnd_rect.right - wnd_rect.left) - cmd_size.cx;
        msg_org.x = wnd_rect.left;
        cmd_org.x = msg_org.x + msg_size.cx;
        msg_org.y = cmd_org.y = wnd_rect.bottom - msg_size.cy;
#else
        msg_size.cx = (wnd_rect.right - wnd_rect.left);
        msg_org.x = wnd_rect.left;
        msg_org.y = wnd_rect.top;
#endif
        wnd_rect.top += msg_size.cy;
        break;

    case ALIGN_BOTTOM:
    default:
#if defined(WIN_CE_SMARTPHONE)
        /* smartphone has a keypad window on the right side of the message
         * window */
        msg_size.cy = cmd_size.cy = max(msg_size.cy, cmd_size.cy);
        msg_size.cx = (wnd_rect.right - wnd_rect.left) - cmd_size.cx;
        msg_org.x = wnd_rect.left;
        cmd_org.x = msg_org.x + msg_size.cx;
        msg_org.y = cmd_org.y = wnd_rect.bottom - msg_size.cy;
#else
        msg_size.cx = (wnd_rect.right - wnd_rect.left);
        msg_org.x = wnd_rect.left;
        msg_org.y = wnd_rect.bottom - msg_size.cy;
#endif
        wnd_rect.bottom -= msg_size.cy;
        break;
    }

    map_org.x = wnd_rect.left;
    map_org.y = wnd_rect.top;
    map_size.cx = wnd_rect.right - wnd_rect.left;
    map_size.cy = wnd_rect.bottom - wnd_rect.top;

    /* go through the windows list and adjust sizes */
    for (i = 0; i < MAXWINDOWS; i++) {
        if (GetNHApp()->windowlist[i].win
            && !GetNHApp()->windowlist[i].dead) {
            switch (GetNHApp()->windowlist[i].type) {
            case NHW_MESSAGE:
                MoveWindow(GetNHApp()->windowlist[i].win, msg_org.x,
                           msg_org.y, msg_size.cx, msg_size.cy, TRUE);
                break;
            case NHW_MAP:
                MoveWindow(GetNHApp()->windowlist[i].win, map_org.x,
                           map_org.y, map_size.cx, map_size.cy, TRUE);
                break;
            case NHW_STATUS:
                MoveWindow(GetNHApp()->windowlist[i].win, status_org.x,
                           status_org.y, status_size.cx, status_size.cy,
                           TRUE);
                break;

            case NHW_TEXT:
            case NHW_MENU:
            case NHW_RIP: {
                POINT menu_org;
                SIZE menu_size;

                menu_org.x = client_rt.left;
                menu_org.y = client_rt.top;
#if defined(WIN_CE_POCKETPC)
                menu_size.cx = min(sip.rcVisibleDesktop.right
                                       - sip.rcVisibleDesktop.left,
                                   client_rt.right - client_rt.left);
                menu_size.cy = min(sip.rcVisibleDesktop.bottom
                                       - sip.rcVisibleDesktop.top,
                                   client_rt.bottom - client_rt.top);
#else
                menu_size.cx = client_rt.right - client_rt.left;
                menu_size.cy = client_rt.bottom - client_rt.top;
#endif

#if defined(WIN_CE_SMARTPHONE)
                /* leave room for the command window */
                if (GetNHApp()->windowlist[i].type == NHW_MENU) {
                    menu_size.cy -= cmd_size.cy;
                }

                /* dialogs are popup windows unde SmartPhone so we need
                   to convert to screen coordinates */
                ClientToScreen(GetNHApp()->hMainWnd, &menu_org);
#endif
                MoveWindow(GetNHApp()->windowlist[i].win, menu_org.x,
                           menu_org.y, menu_size.cx, menu_size.cy, TRUE);
            } break;
            }
            ShowWindow(GetNHApp()->windowlist[i].win, SW_SHOW);
            InvalidateRect(GetNHApp()->windowlist[i].win, NULL, TRUE);
        }
    }

    if (IsWindow(GetNHApp()->hCmdWnd)) {
        /* show command window only if it exists and
           the game is ready (plname is set) */
        if (GetNHApp()->bCmdPad && cmd_size.cx > 0 && cmd_size.cy > 0
            && *gp.plname) {
            MoveWindow(GetNHApp()->hCmdWnd, cmd_org.x, cmd_org.y, cmd_size.cx,
                       cmd_size.cy, TRUE);
            ShowWindow(GetNHApp()->hCmdWnd, SW_SHOW);
        } else {
            ShowWindow(GetNHApp()->hCmdWnd, SW_HIDE);
        }
    }
}

LRESULT
onWMCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PNHMainWindow data;

    data = (PNHMainWindow) GetWindowLong(hWnd, GWL_USERDATA);
    wmId = LOWORD(wParam);
    wmEvent = HIWORD(wParam);

    // process the menu selections:
    switch (wmId) {
    case IDM_ABOUT:
        DialogBox(GetNHApp()->hApp, (LPCTSTR) IDD_ABOUTBOX, hWnd,
                  (DLGPROC) About);
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

    case IDM_VIEW_KEYPAD:
        GetNHApp()->bCmdPad = !GetNHApp()->bCmdPad;
        CheckMenuItem(
            _get_main_menu(ID_VIEW), IDM_VIEW_KEYPAD,
            MF_BYCOMMAND | (GetNHApp()->bCmdPad ? MF_CHECKED : MF_UNCHECKED));
        mswin_layout_main_window(GetNHApp()->hCmdWnd);
        break;

    case IDM_VIEW_OPTIONS:
        doset();
        break;

    case IDM_DIRECT_COMMAND: /* SmartPhone: display dialog to type in arbitary
                                command text */
        mswin_direct_command();
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

    case IDM_HELP_MENU:
        dohelp();
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
    TCHAR wbuf[NHSTR_BUFSIZE];
    RECT main_rt, dlg_rt;
    SIZE dlg_sz;

    switch (message) {
    case WM_INITDIALOG:
        getversionstring(buf);
        SetDlgItemText(hDlg, IDC_ABOUT_VERSION,
                       NH_A2W(buf, wbuf, NHSTR_BUFSIZE));

        SetDlgItemText(hDlg, IDC_ABOUT_COPYRIGHT,
                       NH_A2W(COPYRIGHT_BANNER_A "\n" COPYRIGHT_BANNER_B
                                                 "\n" COPYRIGHT_BANNER_C
                                                 "\n" COPYRIGHT_BANNER_D,
                              wbuf, NHSTR_BUFSIZE));

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

/* Set map display mode */
void
mswin_select_map_mode(int mode)
{
    HMENU hmenuMap;
    PNHMainWindow data;
    winid map_id;

    map_id = WIN_MAP;
    data = (PNHMainWindow) GetWindowLong(GetNHApp()->hMainWnd, GWL_USERDATA);
#if defined(WIN_CE_SMARTPHONE)
    /* Smartphone manu has only 2 items */
    hmenuMap = _get_main_menu(ID_VIEW);
#else
    hmenuMap = _get_main_menu(ID_MAP);
#endif

    /* override for Rogue level */
    if (Is_rogue_level(&u.uz) && !IS_MAP_ASCII(mode))
        return;

    /* set map mode menu mark */
    if (IS_MAP_ASCII(mode)) {
        CheckMenuRadioItem(hmenuMap, IDM_MAP_TILES, IDM_MAP_FIT_TO_SCREEN,
                           mapmode2menuid(IS_MAP_FIT_TO_SCREEN(mode)
                                              ? data->mapAcsiiModeSave
                                              : mode),
                           MF_BYCOMMAND);
    } else {
        CheckMenuRadioItem(hmenuMap, IDM_MAP_TILES, IDM_MAP_FIT_TO_SCREEN,
                           mapmode2menuid(MAP_MODE_TILES), MF_BYCOMMAND);
    }

#if defined(WIN_CE_SMARTPHONE)
    /* update "Fit To Screen" item text */
    {
        TCHAR wbuf[BUFSZ];
        TBBUTTONINFO tbbi;

        ZeroMemory(wbuf, sizeof(wbuf));
        if (!LoadString(GetNHApp()->hApp,
                        (IS_MAP_FIT_TO_SCREEN(mode) ? IDS_CAP_NORMALMAP
                                                    : IDS_CAP_ENTIREMAP),
                        wbuf, BUFSZ)) {
            panic("cannot load main menu strings");
        }

        ZeroMemory(&tbbi, sizeof(tbbi));
        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_TEXT;
        tbbi.pszText = wbuf;
        if (!SendMessage(GetNHApp()->hMenuBar, TB_SETBUTTONINFO,
                         IDM_MAP_FIT_TO_SCREEN, (LPARAM) &tbbi)) {
            error("Cannot update IDM_MAP_FIT_TO_SCREEN menu item.");
        }
    }
#else
    /* set fit-to-screen mode mark */
    CheckMenuItem(hmenuMap, IDM_MAP_FIT_TO_SCREEN,
                  MF_BYCOMMAND | (IS_MAP_FIT_TO_SCREEN(mode) ? MF_CHECKED
                                                             : MF_UNCHECKED));
#endif

    if (IS_MAP_ASCII(iflags.wc_map_mode)
        && !IS_MAP_FIT_TO_SCREEN(iflags.wc_map_mode)) {
        data->mapAcsiiModeSave = iflags.wc_map_mode;
    }

    iflags.wc_map_mode = mode;

    /*
    ** first, check if WIN_MAP has been inialized.
    ** If not - attempt to retrieve it by type, then check it again
    */
    if (WIN_MAP != WIN_ERR)
        mswin_map_mode(mswin_hwnd_from_winid(WIN_MAP), mode);
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

HMENU
_get_main_menu(UINT menu_id)
{
    HMENU hmenuMap;
#if defined(WIN_CE_POCKETPC) || defined(WIN_CE_SMARTPHONE)
    TBBUTTONINFO tbbi;
#endif

#if defined(WIN_CE_POCKETPC) || defined(WIN_CE_SMARTPHONE)
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_LPARAM;
    SendMessage(GetNHApp()->hMenuBar, TB_GETBUTTONINFO, menu_id,
                (LPARAM) &tbbi);
    hmenuMap = (HMENU) tbbi.lParam;
#else
    hmenuMap = CommandBar_GetMenu(GetNHApp()->hMenuBar, 0);
#endif
    return hmenuMap;
}

/* SmartPhone: display dialog to type arbitrary command text */
void
mswin_direct_command()
{
    char cmd[BUFSZ];
    ZeroMemory(cmd, sizeof(cmd));
    mswin_getlin("Type cmd text", cmd);
    if (cmd[0]) {
        /* feed command to nethack */
        char *p = cmd;
        cmd[32] = '\x0'; /* truncate at 32 chars */
        while (*p) {
            NHEVENT_KBD(*p);
            p++;
        }
        if (cmd[0] != '\033')
            mswin_putstr(WIN_MESSAGE, ATR_NONE, cmd);
    }
}

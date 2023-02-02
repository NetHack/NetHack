/* NetHack 3.6  mhmenu.c       $NHDT-Date: 1524689398 2018/04/25 20:49:58 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.28 $ */
/*      Copyright (c) 2009 by Michael Allison              */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include <assert.h>
#include "mhmenu.h"
#include "mhmain.h"
#include "mhmsg.h"
#include "mhcmd.h"
#include "mhinput.h"
#include "mhfont.h"
#include "mhcolor.h"
#include "mhtxtbuf.h"

#define MENU_MARGIN 0
#define NHMENU_STR_SIZE BUFSZ
#define MIN_TABSTOP_SIZE 0
#define NUMTABS 15
#define TAB_SEPARATION 10 /* pixels between each tab stop */

typedef struct mswin_menu_item {
    int glyph;
    ANY_P identifier;
    CHAR_P accelerator;
    CHAR_P group_accel;
    int attr;
    char str[NHMENU_STR_SIZE];
    BOOLEAN_P presel;
    int count;
    BOOL has_focus;
    BOOL has_tab;
} NHMenuItem, *PNHMenuItem;

typedef struct mswin_nethack_menu_window {
    int type; /* MENU_TYPE_TEXT or MENU_TYPE_MENU */
    int how;  /* for menus: PICK_NONE, PICK_ONE, PICK_ANY */

    union {
        struct menu_list {
            int size;            /* number of items in items[] */
            int allocated;       /* number of allocated slots in items[] */
            PNHMenuItem items;   /* menu items */
            char gacc[QBUFSZ];   /* group accelerators */
            BOOL counting;       /* counting flag */
            char prompt[QBUFSZ]; /* menu prompt */
            int tab_stop_size[NUMTABS]; /* tabstops to align option values */
        } menu;

        struct menu_text {
            PNHTextBuffer text;
        } text;
    };
    int result;
    int done;

    HBITMAP bmpChecked;
    HBITMAP bmpCheckedCount;
    HBITMAP bmpNotChecked;
} NHMenuWindow, *PNHMenuWindow;

extern short glyph2tile[];

static WNDPROC wndProcListViewOrig = NULL;
static WNDPROC editControlWndProc = NULL;

#define NHMENU_IS_SELECTABLE(item) ((item).identifier.a_obj != NULL)
#define NHMENU_IS_SELECTED(item) ((item).count != 0)

LRESULT CALLBACK MenuWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NHMenuListWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NHMenuTextWndProc(HWND, UINT, WPARAM, LPARAM);
static void CheckInputDialog(HWND hWnd, UINT message, WPARAM wParam,
                             LPARAM lParam);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static LRESULT onMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam);
static LRESULT onDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void LayoutMenu(HWND hwnd);
static void SetMenuType(HWND hwnd, int type);
static void SetMenuListType(HWND hwnd, int now);
static HWND GetMenuControl(HWND hwnd);
static void SelectMenuItem(HWND hwndList, PNHMenuWindow data, int item,
                           int count);
static void reset_menu_count(HWND hwndList, PNHMenuWindow data);
static LRESULT onListChar(HWND hWnd, HWND hwndList, WORD ch);
static char *parse_menu_str(char *dest, const char *src, size_t size);

HWND
mswin_init_menu_window(int type)
{
    HWND ret;

    ret = CreateDialog(GetNHApp()->hApp, MAKEINTRESOURCE(IDD_MENU),
                       GetNHApp()->hMainWnd, MenuWndProc);
    if (!ret) {
        panic("Cannot create menu window");
    }

    SetMenuType(ret, type);
    return ret;
}

int
mswin_menu_window_select_menu(HWND hWnd, int how, MENU_ITEM_P **_selected)
{
    PNHMenuWindow data;
    int ret_val;
    MENU_ITEM_P *selected = NULL;
    int i;
    char *ap;
    char accell_str[256];

    assert(_selected != NULL);
    *_selected = NULL;
    ret_val = -1;

    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);

    /* set menu type */
    SetMenuListType(hWnd, how);

    /* Ok, now give items a unique accelerators */
    ZeroMemory(accell_str, sizeof(accell_str));
    ap = accell_str;

#if defined(WIN_CE_SMARTPHONE)
    if (data->menu.size > 10) {
        *ap++ = MENU_FIRST_PAGE;
        *ap++ = MENU_LAST_PAGE;
        *ap++ = MENU_NEXT_PAGE;
        *ap++ = MENU_PREVIOUS_PAGE;
        if (data->how == PICK_ANY) {
            *ap++ = MENU_SELECT_ALL;
            *ap++ = MENU_UNSELECT_ALL;
            *ap++ = MENU_INVERT_ALL;
            *ap++ = MENU_SELECT_PAGE;
            *ap++ = MENU_UNSELECT_PAGE;
            *ap++ = MENU_INVERT_PAGE;
        }
        *ap++ = MENU_SEARCH;
    }
#endif

    if (data->type == MENU_TYPE_MENU) {
        char next_char = 'a';

        for (i = 0; i < data->menu.size; i++) {
            if (data->menu.items[i].accelerator != 0) {
                *ap++ = data->menu.items[i].accelerator;
                next_char = (char) (data->menu.items[i].accelerator + 1);
            } else if (NHMENU_IS_SELECTABLE(data->menu.items[i])) {
                if ((next_char >= 'a' && next_char <= 'z')
                    || (next_char >= 'A' && next_char <= 'Z')) {
                    data->menu.items[i].accelerator = next_char;
                    *ap++ = data->menu.items[i].accelerator;
                } else {
                    if (next_char > 'z')
                        next_char = 'A';
                    else if (next_char > 'Z')
                        break;

                    data->menu.items[i].accelerator = next_char;
                    *ap++ = data->menu.items[i].accelerator;
                }

                next_char++;
            }
        }

        /* collect group accelerators */
        data->menu.gacc[0] = '\0';
        ap = data->menu.gacc;
        if (data->how != PICK_NONE) {
            for (i = 0; i < data->menu.size; i++) {
                if (data->menu.items[i].group_accel
                    && !strchr(data->menu.gacc,
                               data->menu.items[i].group_accel)) {
                    *ap++ = data->menu.items[i].group_accel;
                    *ap = '\x0';
                }
            }
        }

        reset_menu_count(NULL, data);
    }

#if defined(WIN_CE_SMARTPHONE)
    if (data->type == MENU_TYPE_MENU)
        NHSPhoneSetKeypadFromString(accell_str);
#endif

    mswin_popup_display(hWnd, &data->done);

    /* get the result */
    if (data->result != -1) {
        if (how == PICK_NONE) {
            if (data->result >= 0)
                ret_val = 0;
            else
                ret_val = -1;
        } else if (how == PICK_ONE || how == PICK_ANY) {
            /* count selected items */
            ret_val = 0;
            for (i = 0; i < data->menu.size; i++) {
                if (NHMENU_IS_SELECTABLE(data->menu.items[i])
                    && NHMENU_IS_SELECTED(data->menu.items[i])) {
                    ret_val++;
                }
            }
            if (ret_val > 0) {
                int sel_ind;

                selected =
                    (MENU_ITEM_P *) malloc(ret_val * sizeof(MENU_ITEM_P));
                if (!selected)
                    panic("out of memory");

                sel_ind = 0;
                for (i = 0; i < data->menu.size; i++) {
                    if (NHMENU_IS_SELECTABLE(data->menu.items[i])
                        && NHMENU_IS_SELECTED(data->menu.items[i])) {
                        selected[sel_ind].item =
                            data->menu.items[i].identifier;
                        selected[sel_ind].count = data->menu.items[i].count;
                        sel_ind++;
                    }
                }
                ret_val = sel_ind;
                *_selected = selected;
            }
        }
    }

    mswin_popup_destroy(hWnd);

#if defined(WIN_CE_SMARTPHONE)
    if (data->type == MENU_TYPE_MENU)
        NHSPhoneSetKeypadDefault();
#endif

    return ret_val;
}

LRESULT CALLBACK
MenuWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PNHMenuWindow data;

    CheckInputDialog(hWnd, message, wParam, lParam);

    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);
    switch (message) {
    case WM_INITDIALOG: {
        HWND text_control;
        HDC hDC;

        text_control = GetDlgItem(hWnd, IDC_MENU_TEXT);

        data = (PNHMenuWindow) malloc(sizeof(NHMenuWindow));
        ZeroMemory(data, sizeof(NHMenuWindow));
        data->type = MENU_TYPE_TEXT;
        data->how = PICK_NONE;
        data->result = 0;
        data->done = 0;
        data->bmpChecked =
            LoadBitmap(GetNHApp()->hApp, MAKEINTRESOURCE(IDB_MENU_SEL));
        data->bmpCheckedCount =
            LoadBitmap(GetNHApp()->hApp, MAKEINTRESOURCE(IDB_MENU_SEL_COUNT));
        data->bmpNotChecked =
            LoadBitmap(GetNHApp()->hApp, MAKEINTRESOURCE(IDB_MENU_UNSEL));
        SetWindowLong(hWnd, GWL_USERDATA, (LONG) data);

        /* subclass edit control */
        editControlWndProc =
            (WNDPROC) GetWindowLong(text_control, GWL_WNDPROC);
        SetWindowLong(text_control, GWL_WNDPROC, (LONG) NHMenuTextWndProc);

        /* set text window font */
        hDC = GetDC(text_control);
        SendMessage(text_control, WM_SETFONT,
                    (WPARAM) mswin_get_font(NHW_TEXT, ATR_NONE, hDC, FALSE),
                    (LPARAM) 0);
        ReleaseDC(text_control, hDC);

#if defined(WIN_CE_SMARTPHONE)
        /* special initialization for SmartPhone dialogs */
        NHSPhoneDialogSetup(hWnd, IDC_SPHONE_DIALOGBAR, FALSE,
                            GetNHApp()->bFullScreen);
#endif
    } break;

    case WM_MSNH_COMMAND:
        onMSNHCommand(hWnd, wParam, lParam);
        break;

    case WM_SIZE:
        LayoutMenu(hWnd);
        return FALSE;

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            if (data->type == MENU_TYPE_MENU
                && (data->how == PICK_ONE || data->how == PICK_ANY)
                && data->menu.counting) {
                HWND list;
                int i;

                /* reset counter if counting is in progress */
                list = GetMenuControl(hWnd);
                i = ListView_GetNextItem(list, -1, LVNI_FOCUSED);
                if (i >= 0) {
                    SelectMenuItem(list, data, i, 0);
                }
                return FALSE;
            } else {
                data->result = -1;
                data->done = 1;
            }
            return FALSE;

        case IDOK:
            data->done = 1;
            data->result = 0;
            return FALSE;
        }
    } break;

    case WM_NOTIFY: {
        LPNMHDR lpnmhdr = (LPNMHDR) lParam;
        switch (LOWORD(wParam)) {
        case IDC_MENU_LIST: {
            if (!data || data->type != MENU_TYPE_MENU)
                break;

            switch (lpnmhdr->code) {
            case LVN_ITEMACTIVATE: {
                LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW) lParam;
                if (data->how == PICK_ONE) {
                    if (lpnmlv->iItem >= 0 && lpnmlv->iItem < data->menu.size
                        && NHMENU_IS_SELECTABLE(
                               data->menu.items[lpnmlv->iItem])) {
                        SelectMenuItem(lpnmlv->hdr.hwndFrom, data,
                                       lpnmlv->iItem, -1);
                        data->done = 1;
                        data->result = 0;
                        return TRUE;
                    }
                } else if (data->how == PICK_ANY) {
                    if (lpnmlv->iItem >= 0 && lpnmlv->iItem < data->menu.size
                        && NHMENU_IS_SELECTABLE(
                               data->menu.items[lpnmlv->iItem])) {
                        SelectMenuItem(lpnmlv->hdr.hwndFrom, data,
                                       lpnmlv->iItem,
                                       NHMENU_IS_SELECTED(
                                           data->menu.items[lpnmlv->iItem])
                                           ? 0
                                           : -1);
                    }
                }
            } break;

            case NM_CLICK: {
                LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW) lParam;
                if (lpnmlv->iItem == -1)
                    return 0;
                if (data->how == PICK_ANY) {
                    SelectMenuItem(
                        lpnmlv->hdr.hwndFrom, data, lpnmlv->iItem,
                        NHMENU_IS_SELECTED(data->menu.items[lpnmlv->iItem])
                            ? 0
                            : -1);
                }
            } break;

            case LVN_ITEMCHANGED: {
                LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW) lParam;
                if (lpnmlv->iItem == -1)
                    return 0;
                if (!(lpnmlv->uChanged & LVIF_STATE))
                    return 0;

                /* update item that has the focus */
                data->menu.items[lpnmlv->iItem].has_focus =
                    !!(lpnmlv->uNewState & LVIS_FOCUSED);
                ListView_RedrawItems(lpnmlv->hdr.hwndFrom, lpnmlv->iItem,
                                     lpnmlv->iItem);

                /* update count for single-selection menu (follow the listview
                 * selection) */
                if (data->how == PICK_ONE) {
                    if (lpnmlv->uNewState & LVIS_SELECTED) {
                        SelectMenuItem(lpnmlv->hdr.hwndFrom, data,
                                       lpnmlv->iItem, -1);
                    }
                }

                /* check item focus */
                data->menu.items[lpnmlv->iItem].has_focus =
                    !!(lpnmlv->uNewState & LVIS_FOCUSED);
                ListView_RedrawItems(lpnmlv->hdr.hwndFrom, lpnmlv->iItem,
                                     lpnmlv->iItem);
            } break;

            case NM_KILLFOCUS:
                reset_menu_count(lpnmhdr->hwndFrom, data);
                break;
            }
        } break;
        }
    } break;

    case WM_SETFOCUS:
        if (hWnd != GetNHApp()->hPopupWnd) {
            SetFocus(GetNHApp()->hPopupWnd);
            return 0;
        }
        break;

    case WM_MEASUREITEM:
        if (wParam == IDC_MENU_LIST)
            return onMeasureItem(hWnd, wParam, lParam);
        else
            return FALSE;

    case WM_DRAWITEM:
        if (wParam == IDC_MENU_LIST)
            return onDrawItem(hWnd, wParam, lParam);
        else
            return FALSE;

    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: { /* sent by edit control before it is drawn */
        HDC hdcEdit = (HDC) wParam;
        HWND hwndEdit = (HWND) lParam;
        if (hwndEdit == GetDlgItem(hWnd, IDC_MENU_TEXT)) {
            SetBkColor(hdcEdit, mswin_get_color(NHW_TEXT, MSWIN_COLOR_BG));
            SetTextColor(hdcEdit, mswin_get_color(NHW_TEXT, MSWIN_COLOR_FG));
            return (BOOL) mswin_get_brush(NHW_TEXT, MSWIN_COLOR_BG);
        }
    }
        return FALSE;

    case WM_DESTROY:
        if (data) {
            DeleteObject(data->bmpChecked);
            DeleteObject(data->bmpCheckedCount);
            DeleteObject(data->bmpNotChecked);
            if (data->type == MENU_TYPE_TEXT) {
                if (data->text.text) {
                    mswin_free_text_buffer(data->text.text);
                    data->text.text = NULL;
                }
            }
            free(data);
            SetWindowLong(hWnd, GWL_USERDATA, (LONG) 0);
        }
        return TRUE;
    }
    return FALSE;
}

void
CheckInputDialog(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#if defined(WIN_CE_POCKETPC)
    PNHMenuWindow data;

    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);

    if (!(data && data->type == MENU_TYPE_MENU
          && (data->how == PICK_ONE || data->how == PICK_ANY)))
        return;

    switch (message) {
    case WM_SETFOCUS:
        if (GetNHApp()->bUseSIP)
            SHSipPreference(hWnd, SIP_UP);
        return;

    case WM_DESTROY:
    case WM_KILLFOCUS:
        if (GetNHApp()->bUseSIP)
            SHSipPreference(hWnd, SIP_DOWN);
        return;

    case WM_NOTIFY: {
        LPNMHDR lpnmhdr = (LPNMHDR) lParam;
        switch (lpnmhdr->code) {
        case NM_SETFOCUS:
            if (GetNHApp()->bUseSIP)
                SHSipPreference(hWnd, SIP_UP);
            break;
        case NM_KILLFOCUS:
            if (GetNHApp()->bUseSIP)
                SHSipPreference(hWnd, SIP_DOWN);
            break;
        }
    }
        return;

    case WM_COMMAND:
        switch (HIWORD(wParam)) {
        case BN_SETFOCUS:
            if (GetNHApp()->bUseSIP)
                SHSipPreference(hWnd, SIP_UP);
            break;
        case BN_KILLFOCUS:
            if (GetNHApp()->bUseSIP)
                SHSipPreference(hWnd, SIP_DOWN);
            break;
        }
        return;

    } /* end switch */
#endif
}

void
onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHMenuWindow data;

    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);
    switch (wParam) {
    case MSNH_MSG_PUTSTR: {
        PMSNHMsgPutstr msg_data = (PMSNHMsgPutstr) lParam;
        HWND text_view;

        if (data->type != MENU_TYPE_TEXT)
            SetMenuType(hWnd, MENU_TYPE_TEXT);

        if (!data->text.text) {
            data->text.text = mswin_init_text_buffer(
                gp.program_state.gameover ? FALSE : GetNHApp()->bWrapText);
            if (!data->text.text)
                break;
        }

        mswin_add_text(data->text.text, msg_data->attr, msg_data->text);

        text_view = GetDlgItem(hWnd, IDC_MENU_TEXT);
        if (!text_view)
            panic("cannot get text view window");
        mswin_render_text(data->text.text, text_view);
    } break;

    case MSNH_MSG_STARTMENU: {
        int i;

        if (data->type != MENU_TYPE_MENU)
            SetMenuType(hWnd, MENU_TYPE_MENU);

        if (data->menu.items)
            free(data->menu.items);
        data->how = PICK_NONE;
        data->menu.items = NULL;
        data->menu.size = 0;
        data->menu.allocated = 0;
        data->done = 0;
        data->result = 0;
        for (i = 0; i < NUMTABS; ++i)
            data->menu.tab_stop_size[i] = MIN_TABSTOP_SIZE;
    } break;

    case MSNH_MSG_ADDMENU: {
        PMSNHMsgAddMenu msg_data = (PMSNHMsgAddMenu) lParam;
        char *p, *p1;
        int new_item;
        HDC hDC;
        int column;
        HFONT saveFont;

        if (data->type != MENU_TYPE_MENU)
            break;
        if (strlen(msg_data->str) == 0)
            break;

        if (data->menu.size == data->menu.allocated) {
            data->menu.allocated += 10;
            data->menu.items = (PNHMenuItem) realloc(
                data->menu.items, data->menu.allocated * sizeof(NHMenuItem));
        }

        new_item = data->menu.size;
        ZeroMemory(&data->menu.items[new_item],
                   sizeof(data->menu.items[new_item]));
        data->menu.items[new_item].glyph = msg_data->glyph;
        data->menu.items[new_item].identifier = *msg_data->identifier;
        data->menu.items[new_item].accelerator = msg_data->accelerator;
        data->menu.items[new_item].group_accel = msg_data->group_accel;
        data->menu.items[new_item].attr = msg_data->attr;
        parse_menu_str(data->menu.items[new_item].str, msg_data->str,
                       NHMENU_STR_SIZE);
        data->menu.items[new_item].presel = msg_data->presel;

        /* calculate tabstop size */
        p = strchr(data->menu.items[new_item].str, '\t');
        if (p) {
            data->menu.items[new_item].has_tab = TRUE;
            hDC = GetDC(hWnd);
            saveFont = SelectObject(
                hDC, mswin_get_font(NHW_MENU, msg_data->attr, hDC, FALSE));
            p1 = data->menu.items[new_item].str;
            column = 0;
            for (;;) {
                TCHAR wbuf[BUFSZ];
                RECT drawRect;
                SetRect(&drawRect, 0, 0, 1, 1);
                if (p != NULL)
                    *p = '\0'; /* for time being, view tab field as zstring */
                DrawText(hDC, NH_A2W(p1, wbuf, BUFSZ), strlen(p1), &drawRect,
                         DT_CALCRECT | DT_LEFT | DT_VCENTER | DT_EXPANDTABS
                             | DT_SINGLELINE);
                data->menu.tab_stop_size[column] =
                    max(data->menu.tab_stop_size[column],
                        drawRect.right - drawRect.left);
                if (p != NULL)
                    *p = '\t';
                else /* last string so, */
                    break;

                ++column;
                p1 = p + 1;
                p = strchr(p1, '\t');
            }
            SelectObject(hDC, saveFont);
            ReleaseDC(hWnd, hDC);
        } else {
            data->menu.items[new_item].has_tab = FALSE;
        }

        /* increment size */
        data->menu.size++;
    } break;

    case MSNH_MSG_ENDMENU: {
        PMSNHMsgEndMenu msg_data = (PMSNHMsgEndMenu) lParam;
        if (msg_data->text) {
            strncpy(data->menu.prompt, msg_data->text,
                    sizeof(data->menu.prompt) - 1);
        } else {
            ZeroMemory(data->menu.prompt, sizeof(data->menu.prompt));
        }
    } break;

    } /* end switch */
}

void
LayoutMenu(HWND hWnd)
{
    PNHMenuWindow data;
    HWND menu_ok;
    HWND menu_cancel;
    RECT clrt, rt;
    POINT pt_elem, pt_ok, pt_cancel;
    SIZE sz_elem, sz_ok, sz_cancel;

    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);
    menu_ok = GetDlgItem(hWnd, IDOK);
    menu_cancel = GetDlgItem(hWnd, IDCANCEL);

    /* get window coordinates */
    GetClientRect(hWnd, &clrt);

    /* set window placements */
    if (IsWindow(menu_ok)) {
        GetWindowRect(menu_ok, &rt);
        sz_ok.cx = (clrt.right - clrt.left) / 2 - 2 * MENU_MARGIN;
        sz_ok.cy = rt.bottom - rt.top;
        pt_ok.x = clrt.left + MENU_MARGIN;
        pt_ok.y = clrt.bottom - MENU_MARGIN - sz_ok.cy;
        MoveWindow(menu_ok, pt_ok.x, pt_ok.y, sz_ok.cx, sz_ok.cy, TRUE);
    } else {
        pt_ok.x = 0;
        pt_ok.y = clrt.bottom;
        sz_ok.cx = sz_ok.cy = 0;
    }

    if (IsWindow(menu_cancel)) {
        GetWindowRect(menu_cancel, &rt);
        sz_cancel.cx = (clrt.right - clrt.left) / 2 - 2 * MENU_MARGIN;
        sz_cancel.cy = rt.bottom - rt.top;
        pt_cancel.x = (clrt.left + clrt.right) / 2 + MENU_MARGIN;
        pt_cancel.y = clrt.bottom - MENU_MARGIN - sz_cancel.cy;
        MoveWindow(menu_cancel, pt_cancel.x, pt_cancel.y, sz_cancel.cx,
                   sz_cancel.cy, TRUE);
    } else {
        pt_cancel.x = 0;
        pt_cancel.y = clrt.bottom;
        sz_cancel.cx = sz_cancel.cy = 0;
    }

    pt_elem.x = clrt.left + MENU_MARGIN;
    pt_elem.y = clrt.top + MENU_MARGIN;
    sz_elem.cx = (clrt.right - clrt.left) - 2 * MENU_MARGIN;
    sz_elem.cy = min(pt_cancel.y, pt_ok.y) - MENU_MARGIN - pt_elem.y;
    MoveWindow(GetMenuControl(hWnd), pt_elem.x, pt_elem.y, sz_elem.cx,
               sz_elem.cy, TRUE);

    /* reformat text for the text menu */
    if (data && data->type == MENU_TYPE_TEXT && data->text.text)
        mswin_render_text(data->text.text, GetMenuControl(hWnd));
}

void
SetMenuType(HWND hWnd, int type)
{
    PNHMenuWindow data;
    HWND list, text;

    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);

    data->type = type;

    text = GetDlgItem(hWnd, IDC_MENU_TEXT);
    list = GetDlgItem(hWnd, IDC_MENU_LIST);
    if (data->type == MENU_TYPE_TEXT) {
        ShowWindow(list, SW_HIDE);
        EnableWindow(list, FALSE);
        EnableWindow(text, TRUE);
        ShowWindow(text, SW_SHOW);
        SetFocus(text);
    } else {
        ShowWindow(text, SW_HIDE);
        EnableWindow(text, FALSE);
        EnableWindow(list, TRUE);
        ShowWindow(list, SW_SHOW);
        SetFocus(list);
    }
    LayoutMenu(hWnd);
}

void
SetMenuListType(HWND hWnd, int how)
{
    PNHMenuWindow data;
    RECT rt;
    DWORD dwStyles;
    char buf[BUFSZ];
    TCHAR wbuf[BUFSZ];
    int nItem;
    int i;
    HWND control;
    LVCOLUMN lvcol;
    LRESULT fnt;
    SIZE wnd_size;

    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);
    if (data->type != MENU_TYPE_MENU)
        return;

    data->how = how;

    switch (how) {
    case PICK_NONE:
        dwStyles = WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_CHILD | WS_VSCROLL
                   | WS_HSCROLL | LVS_REPORT | LVS_OWNERDRAWFIXED
                   | LVS_SINGLESEL;
        break;
    case PICK_ONE:
        dwStyles = WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_CHILD | WS_VSCROLL
                   | WS_HSCROLL | LVS_REPORT | LVS_OWNERDRAWFIXED
                   | LVS_SINGLESEL;
        break;
    case PICK_ANY:
        dwStyles = WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_CHILD | WS_VSCROLL
                   | WS_HSCROLL | LVS_REPORT | LVS_OWNERDRAWFIXED
                   | LVS_SINGLESEL;
        break;
    default:
        panic("how should be one of PICK_NONE, PICK_ONE or PICK_ANY");
    };
    if (strlen(data->menu.prompt) == 0) {
        dwStyles |= LVS_NOCOLUMNHEADER;
    }

    GetWindowRect(GetDlgItem(hWnd, IDC_MENU_LIST), &rt);
    DestroyWindow(GetDlgItem(hWnd, IDC_MENU_LIST));
    control = CreateWindow(WC_LISTVIEW, NULL, dwStyles, rt.left, rt.top,
                           rt.right - rt.left, rt.bottom - rt.top, hWnd,
                           (HMENU) IDC_MENU_LIST, GetNHApp()->hApp, NULL);
    if (!control)
        panic("cannot create menu control");

    /* install the hook for the control window procedure */
    wndProcListViewOrig = (WNDPROC) GetWindowLong(control, GWL_WNDPROC);
    SetWindowLong(control, GWL_WNDPROC, (LONG) NHMenuListWndProc);

    /* set control font */
    fnt = SendMessage(hWnd, WM_GETFONT, (WPARAM) 0, (LPARAM) 0);
    SendMessage(control, WM_SETFONT, (WPARAM) fnt, (LPARAM) 0);

    /* set control colors */
    ListView_SetBkColor(control, mswin_get_color(NHW_MENU, MSWIN_COLOR_BG));
    ListView_SetTextBkColor(control,
                            mswin_get_color(NHW_MENU, MSWIN_COLOR_BG));
    ListView_SetTextColor(control, mswin_get_color(NHW_MENU, MSWIN_COLOR_FG));

    /* add column to the list view */
    mswin_menu_window_size(hWnd, &wnd_size);

    ZeroMemory(&lvcol, sizeof(lvcol));
    lvcol.mask = LVCF_WIDTH | LVCF_TEXT;
    lvcol.cx = max(wnd_size.cx, GetSystemMetrics(SM_CXSCREEN));
    lvcol.pszText = NH_A2W(data->menu.prompt, wbuf, BUFSZ);
    ListView_InsertColumn(control, 0, &lvcol);

    /* add items to the list view */
    for (i = 0; i < data->menu.size; i++) {
        LVITEM lvitem;
        ZeroMemory(&lvitem, sizeof(lvitem));
        sprintf(buf, "%c - %s", max(data->menu.items[i].accelerator, ' '),
                data->menu.items[i].str);

        lvitem.mask = LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
        lvitem.iItem = i;
        lvitem.iSubItem = 0;
        lvitem.state = data->menu.items[i].presel ? LVIS_SELECTED : 0;
        lvitem.pszText = NH_A2W(buf, wbuf, BUFSZ);
        lvitem.lParam = (LPARAM) &data->menu.items[i];
        nItem = SendMessage(control, LB_ADDSTRING, (WPARAM) 0, (LPARAM) buf);
        if (ListView_InsertItem(control, &lvitem) == -1) {
            panic("cannot insert menu item");
        }
    }
    SetFocus(control);
}

HWND
GetMenuControl(HWND hWnd)
{
    PNHMenuWindow data;

    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);

    if (data->type == MENU_TYPE_TEXT) {
        return GetDlgItem(hWnd, IDC_MENU_TEXT);
    } else {
        return GetDlgItem(hWnd, IDC_MENU_LIST);
    }
}

LRESULT
onMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPMEASUREITEMSTRUCT lpmis;
    TEXTMETRIC tm;
    HGDIOBJ saveFont;
    HDC hdc;
    PNHMenuWindow data;
    RECT list_rect;

    lpmis = (LPMEASUREITEMSTRUCT) lParam;
    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);
    GetClientRect(GetMenuControl(hWnd), &list_rect);

    hdc = GetDC(GetMenuControl(hWnd));
    saveFont =
        SelectObject(hdc, mswin_get_font(NHW_MENU, ATR_INVERSE, hdc, FALSE));
    GetTextMetrics(hdc, &tm);

    /* Set the height of the list box items. */
    lpmis->itemHeight = max(tm.tmHeight, TILE_Y) + 2;
    lpmis->itemWidth = list_rect.right - list_rect.left;

    SelectObject(hdc, saveFont);
    ReleaseDC(GetMenuControl(hWnd), hdc);
    return TRUE;
}

LRESULT
onDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPDRAWITEMSTRUCT lpdis;
    PNHMenuItem item;
    PNHMenuWindow data;
    TEXTMETRIC tm;
    HGDIOBJ saveFont;
    HDC tileDC;
    short ntile;
    int t_x, t_y;
    int x, y;
    TCHAR wbuf[BUFSZ];
    RECT drawRect;
    COLORREF OldBg, OldFg, NewBg;
    char *p, *p1;
    int column;

    lpdis = (LPDRAWITEMSTRUCT) lParam;

    /* If there are no list box items, skip this message. */
    if (lpdis->itemID == -1)
        return FALSE;

    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);

    item = &data->menu.items[lpdis->itemID];

    tileDC = CreateCompatibleDC(lpdis->hDC);
    saveFont = SelectObject(
        lpdis->hDC, mswin_get_font(NHW_MENU, item->attr, lpdis->hDC, FALSE));
    NewBg = mswin_get_color(NHW_MENU, MSWIN_COLOR_BG);
    OldBg = SetBkColor(lpdis->hDC, NewBg);
    OldFg =
        SetTextColor(lpdis->hDC, mswin_get_color(NHW_MENU, MSWIN_COLOR_FG));

    GetTextMetrics(lpdis->hDC, &tm);

    x = lpdis->rcItem.left + 1;

    /* print check mark if it is a "selectable" menu */
    if (data->how != PICK_NONE) {
        if (NHMENU_IS_SELECTABLE(*item)) {
            HGDIOBJ saveBrush;
            HBRUSH hbrCheckMark;
            char buf[2];

            switch (item->count) {
            case -1:
                hbrCheckMark = CreatePatternBrush(data->bmpChecked);
                break;
            case 0:
                hbrCheckMark = CreatePatternBrush(data->bmpNotChecked);
                break;
            default:
                hbrCheckMark = CreatePatternBrush(data->bmpCheckedCount);
                break;
            }

            y = (lpdis->rcItem.bottom + lpdis->rcItem.top - TILE_Y) / 2;
            SetBrushOrgEx(lpdis->hDC, x, y, NULL);
            saveBrush = SelectObject(lpdis->hDC, hbrCheckMark);
            PatBlt(lpdis->hDC, x, y, TILE_X, TILE_Y, PATCOPY);
            SelectObject(lpdis->hDC, saveBrush);
            DeleteObject(hbrCheckMark);

            x += TILE_X + 5;

            if (item->accelerator != 0) {
                buf[0] = item->accelerator;
                buf[1] = '\x0';

                SetRect(&drawRect, x, lpdis->rcItem.top, lpdis->rcItem.right,
                        lpdis->rcItem.bottom);
                DrawText(lpdis->hDC, NH_A2W(buf, wbuf, 2), 1, &drawRect,
                         DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            }
            x += tm.tmAveCharWidth + tm.tmOverhang + 5;
        } else {
            x += TILE_X + tm.tmAveCharWidth + tm.tmOverhang + 10;
        }
    }

    /* print glyph if present */
    if (item->glyph != NO_GLYPH) {
        HGDIOBJ saveBmp;

        saveBmp = SelectObject(tileDC, GetNHApp()->bmpTiles);
        ntile = glyph2tile[item->glyph];
        t_x = (ntile % TILES_PER_LINE) * TILE_X;
        t_y = (ntile / TILES_PER_LINE) * TILE_Y;

        y = (lpdis->rcItem.bottom + lpdis->rcItem.top - TILE_Y) / 2;

        nhapply_image_transparent(lpdis->hDC, x, y, TILE_X, TILE_Y, tileDC,
                                  t_x, t_y, TILE_X, TILE_Y, TILE_BK_COLOR);
        SelectObject(tileDC, saveBmp);
    }

    x += TILE_X + 5;

    /* draw item text */
    if (item->has_tab) {
        p1 = item->str;
        p = strchr(item->str, '\t');
        column = 0;
        SetRect(&drawRect, x, lpdis->rcItem.top,
                min(x + data->menu.tab_stop_size[0], lpdis->rcItem.right),
                lpdis->rcItem.bottom);
        for (;;) {
            TCHAR wbuf[BUFSZ];
            if (p != NULL)
                *p = '\0'; /* for time being, view tab field as zstring */
            DrawText(lpdis->hDC, NH_A2W(p1, wbuf, BUFSZ), strlen(p1),
                     &drawRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            if (p != NULL)
                *p = '\t';
            else /* last string so, */
                break;

            p1 = p + 1;
            p = strchr(p1, '\t');
            drawRect.left = drawRect.right + TAB_SEPARATION;
            ++column;
            drawRect.right =
                min(drawRect.left + data->menu.tab_stop_size[column],
                    lpdis->rcItem.right);
        }
    } else {
        TCHAR wbuf[BUFSZ];
        SetRect(&drawRect, x, lpdis->rcItem.top, lpdis->rcItem.right,
                lpdis->rcItem.bottom);
        DrawText(lpdis->hDC, NH_A2W(item->str, wbuf, BUFSZ),
                 strlen(item->str), &drawRect,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    /* draw focused item */
    if (item->has_focus) {
        RECT client_rt;
        HBRUSH bkBrush;

        GetClientRect(lpdis->hwndItem, &client_rt);
        if (NHMENU_IS_SELECTABLE(*item)
            && data->menu.items[lpdis->itemID].count > 0
            && item->glyph != NO_GLYPH) {
            if (data->menu.items[lpdis->itemID].count == -1) {
                _stprintf(wbuf, TEXT("Count: All"));
            } else {
                _stprintf(wbuf, TEXT("Count: %d"),
                          data->menu.items[lpdis->itemID].count);
            }

            SelectObject(lpdis->hDC, mswin_get_font(NHW_MENU, ATR_BLINK,
                                                    lpdis->hDC, FALSE));

            /* calculate text rectangle */
            SetRect(&drawRect, client_rt.left, lpdis->rcItem.top,
                    client_rt.right, lpdis->rcItem.bottom);
            DrawText(lpdis->hDC, wbuf, _tcslen(wbuf), &drawRect,
                     DT_CALCRECT | DT_RIGHT | DT_VCENTER | DT_SINGLELINE
                         | DT_NOPREFIX);

            /* erase text rectangle */
            drawRect.left =
                max(client_rt.left + 1,
                    client_rt.right - (drawRect.right - drawRect.left) - 10);
            drawRect.right = client_rt.right - 1;
            drawRect.top = lpdis->rcItem.top;
            drawRect.bottom = lpdis->rcItem.bottom;
            bkBrush = CreateSolidBrush(GetBkColor(lpdis->hDC));
            FillRect(lpdis->hDC, &drawRect, bkBrush);
            DeleteObject(bkBrush);

            /* draw text */
            DrawText(lpdis->hDC, wbuf, _tcslen(wbuf), &drawRect,
                     DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }

        /* draw focus rect */
        SetRect(&drawRect, client_rt.left, lpdis->rcItem.top, client_rt.right,
                lpdis->rcItem.bottom);
        DrawFocusRect(lpdis->hDC, &drawRect);
    }

    SetTextColor(lpdis->hDC, OldFg);
    SetBkColor(lpdis->hDC, OldBg);
    SelectObject(lpdis->hDC, saveFont);
    DeleteDC(tileDC);
    return TRUE;
}

BOOL
onListChar(HWND hWnd, HWND hwndList, WORD ch)
{
    int i = 0;
    PNHMenuWindow data;
    int curIndex, topIndex, pageSize;
    boolean is_accelerator = FALSE;

    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);

    switch (ch) {
    case MENU_FIRST_PAGE:
        i = 0;
        ListView_SetItemState(hwndList, i, LVIS_FOCUSED, LVIS_FOCUSED);
        ListView_EnsureVisible(hwndList, i, FALSE);
        return -2;

    case MENU_LAST_PAGE:
        i = max(0, data->menu.size - 1);
        ListView_SetItemState(hwndList, i, LVIS_FOCUSED, LVIS_FOCUSED);
        ListView_EnsureVisible(hwndList, i, FALSE);
        return -2;

    case MENU_NEXT_PAGE:
        topIndex = ListView_GetTopIndex(hwndList);
        pageSize = ListView_GetCountPerPage(hwndList);
        curIndex = ListView_GetNextItem(hwndList, -1, LVNI_FOCUSED);
        /* Focus down one page */
        i = min(curIndex + pageSize, data->menu.size - 1);
        ListView_SetItemState(hwndList, i, LVIS_FOCUSED, LVIS_FOCUSED);
        /* Scrollpos down one page */
        i = min(topIndex + (2 * pageSize - 1), data->menu.size - 1);
        ListView_EnsureVisible(hwndList, i, FALSE);
        return -2;

    case MENU_PREVIOUS_PAGE:
        topIndex = ListView_GetTopIndex(hwndList);
        pageSize = ListView_GetCountPerPage(hwndList);
        curIndex = ListView_GetNextItem(hwndList, -1, LVNI_FOCUSED);
        /* Focus up one page */
        i = max(curIndex - pageSize, 0);
        ListView_SetItemState(hwndList, i, LVIS_FOCUSED, LVIS_FOCUSED);
        /* Scrollpos up one page */
        i = max(topIndex - pageSize, 0);
        ListView_EnsureVisible(hwndList, i, FALSE);
        break;

    case MENU_SELECT_ALL:
        if (data->how == PICK_ANY) {
            reset_menu_count(hwndList, data);
            for (i = 0; i < data->menu.size; i++) {
                SelectMenuItem(hwndList, data, i, -1);
            }
            return -2;
        }
        break;

    case MENU_UNSELECT_ALL:
        if (data->how == PICK_ANY) {
            reset_menu_count(hwndList, data);
            for (i = 0; i < data->menu.size; i++) {
                SelectMenuItem(hwndList, data, i, 0);
            }
            return -2;
        }
        break;

    case MENU_INVERT_ALL:
        if (data->how == PICK_ANY) {
            reset_menu_count(hwndList, data);
            for (i = 0; i < data->menu.size; i++) {
                SelectMenuItem(hwndList, data, i,
                               NHMENU_IS_SELECTED(data->menu.items[i]) ? 0
                                                                       : -1);
            }
            return -2;
        }
        break;

    case MENU_SELECT_PAGE:
        if (data->how == PICK_ANY) {
            int from, to;
            reset_menu_count(hwndList, data);
            topIndex = ListView_GetTopIndex(hwndList);
            pageSize = ListView_GetCountPerPage(hwndList);
            from = max(0, topIndex);
            to = min(data->menu.size, from + pageSize);
            for (i = from; i < to; i++) {
                SelectMenuItem(hwndList, data, i, -1);
            }
            return -2;
        }
        break;

    case MENU_UNSELECT_PAGE:
        if (data->how == PICK_ANY) {
            int from, to;
            reset_menu_count(hwndList, data);
            topIndex = ListView_GetTopIndex(hwndList);
            pageSize = ListView_GetCountPerPage(hwndList);
            from = max(0, topIndex);
            to = min(data->menu.size, from + pageSize);
            for (i = from; i < to; i++) {
                SelectMenuItem(hwndList, data, i, 0);
            }
            return -2;
        }
        break;

    case MENU_INVERT_PAGE:
        if (data->how == PICK_ANY) {
            int from, to;
            reset_menu_count(hwndList, data);
            topIndex = ListView_GetTopIndex(hwndList);
            pageSize = ListView_GetCountPerPage(hwndList);
            from = max(0, topIndex);
            to = min(data->menu.size, from + pageSize);
            for (i = from; i < to; i++) {
                SelectMenuItem(hwndList, data, i,
                               NHMENU_IS_SELECTED(data->menu.items[i]) ? 0
                                                                       : -1);
            }
            return -2;
        }
        break;

    case MENU_SEARCH:
        if (data->how == PICK_ANY || data->how == PICK_ONE) {
            char buf[BUFSZ];
            int selected_item;

            reset_menu_count(hwndList, data);
            mswin_getlin("Search for:", buf);
            if (!*buf || *buf == '\033')
                return -2;
            selected_item = -1;
            for (i = 0; i < data->menu.size; i++) {
                if (NHMENU_IS_SELECTABLE(data->menu.items[i])
                    && strstr(data->menu.items[i].str, buf)) {
                    if (data->how == PICK_ANY) {
                        SelectMenuItem(
                            hwndList, data, i,
                            NHMENU_IS_SELECTED(data->menu.items[i]) ? 0 : -1);
                        /* save the first item - we will move focus to it */
                        if (selected_item == -1)
                            selected_item = i;
                    } else if (data->how == PICK_ONE) {
                        SelectMenuItem(hwndList, data, i, -1);
                        selected_item = i;
                        break;
                    }
                }
            }

            if (selected_item > 0) {
                ListView_SetItemState(hwndList, selected_item, LVIS_FOCUSED,
                                      LVIS_FOCUSED);
                ListView_EnsureVisible(hwndList, selected_item, FALSE);
            }
        } else {
            mswin_nhbell();
        }
        return -2;

    case ' ':
        /* ends menu for PICK_ONE/PICK_NONE
           select item for PICK_ANY */
        if (data->how == PICK_ONE || data->how == PICK_NONE) {
            data->done = 1;
            data->result = 0;
            return -2;
        } else if (data->how == PICK_ANY) {
            i = ListView_GetNextItem(hwndList, -1, LVNI_FOCUSED);
            if (i >= 0) {
                SelectMenuItem(hwndList, data, i,
                               NHMENU_IS_SELECTED(data->menu.items[i]) ? 0
                                                                       : -1);
            }
            return -2;
        }
        break;

    default:
        if (strchr(data->menu.gacc, ch)
            && !(ch == '0' && data->menu.counting)) {
            /* matched a group accelerator */
            if (data->how == PICK_ANY || data->how == PICK_ONE) {
                reset_menu_count(hwndList, data);
                for (i = 0; i < data->menu.size; i++) {
                    if (NHMENU_IS_SELECTABLE(data->menu.items[i])
                        && data->menu.items[i].group_accel == ch) {
                        if (data->how == PICK_ANY) {
                            SelectMenuItem(
                                hwndList, data, i,
                                NHMENU_IS_SELECTED(data->menu.items[i]) ? 0
                                                                        : -1);
                        } else if (data->how == PICK_ONE) {
                            SelectMenuItem(hwndList, data, i, -1);
                            data->result = 0;
                            data->done = 1;
                            return -2;
                        }
                    }
                }
                return -2;
            } else {
                mswin_nhbell();
                return -2;
            }
        }

        if (isdigit(ch)) {
            int count;
            i = ListView_GetNextItem(hwndList, -1, LVNI_FOCUSED);
            if (i >= 0) {
                count = data->menu.items[i].count;
                if (count == -1)
                    count = 0;
                count *= 10L;
                count += (int) (ch - '0');
                if (count != 0) /* ignore leading zeros */ {
                    data->menu.counting = TRUE;
                    data->menu.items[i].count = min(100000, count);
                    ListView_RedrawItems(hwndList, i,
                                         i); /* update count mark */
                }
            }
            return -2;
        }

        is_accelerator = FALSE;
        for (i = 0; i < data->menu.size; i++) {
            if (data->menu.items[i].accelerator == ch) {
                is_accelerator = TRUE;
                break;
            }
        }

        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
            || is_accelerator) {
            if (data->how == PICK_ANY || data->how == PICK_ONE) {
                for (i = 0; i < data->menu.size; i++) {
                    if (data->menu.items[i].accelerator == ch) {
                        if (data->how == PICK_ANY) {
                            SelectMenuItem(
                                hwndList, data, i,
                                NHMENU_IS_SELECTED(data->menu.items[i]) ? 0
                                                                        : -1);
                            ListView_SetItemState(hwndList, i, LVIS_FOCUSED,
                                                  LVIS_FOCUSED);
                            ListView_EnsureVisible(hwndList, i, FALSE);
                            return -2;
                        } else if (data->how == PICK_ONE) {
                            SelectMenuItem(hwndList, data, i, -1);
                            data->result = 0;
                            data->done = 1;
                            return -2;
                        }
                    }
                }
            }
        }
        break;
    }

    reset_menu_count(hwndList, data);
    return -1;
}

void
mswin_menu_window_size(HWND hWnd, LPSIZE sz)
{
    TEXTMETRIC tm;
    HWND control;
    HGDIOBJ saveFont;
    HDC hdc;
    PNHMenuWindow data;
    int i;
    RECT rt, wrt;
    int extra_cx;

    GetClientRect(hWnd, &rt);
    sz->cx = rt.right - rt.left;
    sz->cy = rt.bottom - rt.top;

    GetWindowRect(hWnd, &wrt);
    extra_cx = (wrt.right - wrt.left) - sz->cx;

    data = (PNHMenuWindow) GetWindowLong(hWnd, GWL_USERDATA);
    if (data) {
        control = GetMenuControl(hWnd);
        hdc = GetDC(control);

        if (data->type == MENU_TYPE_MENU) {
            /* Calculate the width of the list box. */
            saveFont = SelectObject(
                hdc, mswin_get_font(NHW_MENU, ATR_NONE, hdc, FALSE));
            GetTextMetrics(hdc, &tm);
            for (i = 0; i < data->menu.size; i++) {
                LONG menuitemwidth = 0;
                int column;
                char *p, *p1;

                p1 = data->menu.items[i].str;
                p = strchr(data->menu.items[i].str, '\t');
                column = 0;
                for (;;) {
                    TCHAR wbuf[BUFSZ];
                    RECT tabRect;
                    SetRect(&tabRect, 0, 0, 1, 1);
                    if (p != NULL)
                        *p = '\0'; /* for time being, view tab field as
                                      zstring */
                    DrawText(hdc, NH_A2W(p1, wbuf, BUFSZ), strlen(p1),
                             &tabRect, DT_CALCRECT | DT_LEFT | DT_VCENTER
                                           | DT_SINGLELINE);
                    /* it probably isn't necessary to recompute the tab width
                     * now, but do so
                     * just in case, honoring the previously computed value
                     */
                    menuitemwidth += max(data->menu.tab_stop_size[column],
                                         tabRect.right - tabRect.left);
                    if (p != NULL)
                        *p = '\t';
                    else /* last string so, */
                        break;
                    /* add the separation only when not the last item */
                    /* in the last item, we break out of the loop, in the
                     * statement just above */
                    menuitemwidth += TAB_SEPARATION;
                    ++column;
                    p1 = p + 1;
                    p = strchr(p1, '\t');
                }

                sz->cx = max(sz->cx, (LONG)(2 * TILE_X + menuitemwidth
                                            + tm.tmAveCharWidth * 12
                                            + tm.tmOverhang));
            }
            SelectObject(hdc, saveFont);
        } else {
            /* do not change size for text output - the text will be formatted
               to
               fit any window */
        }
        sz->cx += extra_cx;

        ReleaseDC(control, hdc);
    }
}

void
SelectMenuItem(HWND hwndList, PNHMenuWindow data, int item, int count)
{
    int i;

    if (item < 0 || item >= data->menu.size)
        return;

    if (data->how == PICK_ONE && count != 0) {
        for (i = 0; i < data->menu.size; i++)
            if (item != i && data->menu.items[i].count != 0) {
                data->menu.items[i].count = 0;
                ListView_RedrawItems(hwndList, i, i);
            };
    }

    data->menu.items[item].count = count;
    ListView_RedrawItems(hwndList, item, item);
    reset_menu_count(hwndList, data);
}

void
reset_menu_count(HWND hwndList, PNHMenuWindow data)
{
    int i;
    data->menu.counting = FALSE;
    if (IsWindow(hwndList)) {
        i = ListView_GetNextItem((hwndList), -1, LVNI_FOCUSED);
        if (i >= 0)
            ListView_RedrawItems(hwndList, i, i);
    }
}

/* List window Proc */
LRESULT CALLBACK
NHMenuListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bUpdateFocusItem = FALSE;

    switch (message) {
/* filter keyboard input for the control */
#if !defined(WIN_CE_SMARTPHONE)
    case WM_KEYDOWN:
    case WM_KEYUP: {
        MSG msg;

        if (PeekMessage(&msg, hWnd, WM_CHAR, WM_CHAR, PM_REMOVE)) {
            if (onListChar(GetParent(hWnd), hWnd, (char) msg.wParam) == -2) {
                return 0;
            }
        }

        if (wParam == VK_LEFT || wParam == VK_RIGHT)
            bUpdateFocusItem = TRUE;
    } break;

    /* tell Windows not to process arrow keys */
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;

#else /* defined(WIN_CE_SMARTPHONE) */
    case WM_KEYDOWN:
        if (wParam == VK_TACTION) {
            if (onListChar(GetParent(hWnd), hWnd, ' ') == -2) {
                return 0;
            }
        } else if (NHSPhoneTranslateKbdMessage(wParam, lParam, TRUE)) {
            PMSNHEvent evt;
            BOOL processed = FALSE;
            if (mswin_have_input()) {
                evt = mswin_input_pop();
                if (evt->type == NHEVENT_CHAR
                    && onListChar(GetParent(hWnd), hWnd, evt->kbd.ch) == -2) {
                    processed = TRUE;
                }

                /* eat the rest of the events */
                if (mswin_have_input())
                    mswin_input_pop();
            }
            if (processed)
                return 0;
        }

        if (wParam == VK_LEFT || wParam == VK_RIGHT)
            bUpdateFocusItem = TRUE;
        break;

    case WM_KEYUP:
        /* translate SmartPhone keyboard message */
        if (NHSPhoneTranslateKbdMessage(wParam, lParam, FALSE))
            return 0;
        break;

    /* tell Windows not to process default button on VK_RETURN */
    case WM_GETDLGCODE:
        return DLGC_DEFPUSHBUTTON | DLGC_WANTALLKEYS
               | (wndProcListViewOrig
                      ? CallWindowProc(wndProcListViewOrig, hWnd, message,
                                       wParam, lParam)
                      : 0);
#endif

    case WM_SIZE:
    case WM_HSCROLL:
        bUpdateFocusItem = TRUE;
        break;
    }

    if (bUpdateFocusItem) {
        int i;
        RECT rt;

        /* invalidate the focus rectangle */
        i = ListView_GetNextItem(hWnd, -1, LVNI_FOCUSED);
        if (i != -1) {
            ListView_GetItemRect(hWnd, i, &rt, LVIR_BOUNDS);
            InvalidateRect(hWnd, &rt, TRUE);
        }
    }

    if (wndProcListViewOrig)
        return CallWindowProc(wndProcListViewOrig, hWnd, message, wParam,
                              lParam);
    else
        return 0;
}

/* Text control window proc - implements close on space */
LRESULT CALLBACK
NHMenuTextWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_KEYUP:
        switch (wParam) {
        case VK_SPACE:
        case VK_RETURN:
            /* close on space */
            PostMessage(GetParent(hWnd), WM_COMMAND, MAKELONG(IDOK, 0), 0);
            return 0;

        case VK_UP:
            /* scoll up */
            PostMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0),
                        (LPARAM) NULL);
            return 0;

        case VK_DOWN:
            /* scoll down */
            PostMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0),
                        (LPARAM) NULL);
            return 0;

        case VK_LEFT:
            /* scoll left */
            PostMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_LINELEFT, 0),
                        (LPARAM) NULL);
            return 0;

        case VK_RIGHT:
            /* scoll right */
            PostMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_LINERIGHT, 0),
                        (LPARAM) NULL);
            return 0;
        }
        break; /* case WM_KEYUP: */
    }

    if (editControlWndProc)
        return CallWindowProc(editControlWndProc, hWnd, message, wParam,
                              lParam);
    else
        return 0;
}
/*----------------------------------------------------------------------------*/
char *
parse_menu_str(char *dest, const char *src, size_t size)
{
    char *p1, *p2;
    if (!dest || size == 0)
        return NULL;

    strncpy(dest, src, size);
    dest[size - 1] = '\x0';

    /* replace "[ ]*\[" with "\t\[" */
    p1 = p2 = strstr(dest, " [");
    if (p1) {
        while (p1 != dest && *p1 == ' ')
            p1--;
        p1++; /* backup to space */
        *p2 = '\t';
        memmove(p1, p2, strlen(p2));
        p1[strlen(p2)] = '\x0';
    }
    return dest;
}

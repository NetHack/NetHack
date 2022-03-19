/* NetHack 3.7	mhmenu.c	$NHDT-Date: 1596498354 2020/08/03 23:45:54 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.74 $ */
/* Copyright (c) Alex Kompel, 2002                                */
/* NetHack may be freely redistributed.  See license for details. */

#include "win10.h"
#include "winMS.h"
#include <assert.h>
#include "resource.h"
#include "mhmenu.h"
#include "mhmain.h"
#include "mhmsg.h"
#include "mhfont.h"
#include "mhdlg.h"

#define MENU_MARGIN 0
#define NHMENU_STR_SIZE BUFSZ
#define MIN_TABSTOP_SIZE 0
#define NUMTABS 15
#define TAB_SEPARATION 10 /* pixels between each tab stop */

#define DEFAULT_COLOR_BG_TEXT COLOR_WINDOW
#define DEFAULT_COLOR_FG_TEXT COLOR_WINDOWTEXT
#define DEFAULT_COLOR_BG_MENU COLOR_WINDOW
#define DEFAULT_COLOR_FG_MENU COLOR_WINDOWTEXT

#define CHECK_WIDTH 16
#define CHECK_HEIGHT 16

typedef struct mswin_menu_item {
    glyph_info glyphinfo;
    ANY_P identifier;
    char accelerator;
    char group_accel;
    int attr;
    char str[NHMENU_STR_SIZE];
    boolean presel;
    unsigned int itemflags;
    int count;
    BOOL has_focus;
} NHMenuItem, *PNHMenuItem;

union menu_innards {
    struct menu_list {
        int size;            /* number of items in items[] */
        int allocated;       /* number of allocated slots in items[] */
        PNHMenuItem items;   /* menu items */
        char gacc[QBUFSZ];   /* group accelerators */
        BOOL counting;       /* counting flag */
        char prompt[QBUFSZ]; /* menu prompt */
        int tab_stop_size[NUMTABS]; /* tabstops to align option values */
        int menu_cx;                /* menu width */
    } menu;

    struct menu_text {
        TCHAR *text;
        SIZE text_box_size;
    } text;
};

typedef struct mswin_nethack_menu_window {
    int type; /* MENU_TYPE_TEXT or MENU_TYPE_MENU */
    int how;  /* for menus: PICK_NONE, PICK_ONE, PICK_ANY */

    union menu_innards menui;
    int result;
    int done;

    HBITMAP bmpChecked;
    HBITMAP bmpCheckedCount;
    HBITMAP bmpNotChecked;
    HDC bmpDC;

    BOOL is_active;
} NHMenuWindow, *PNHMenuWindow;

static WNDPROC wndProcListViewOrig = NULL;
static WNDPROC editControlWndProc = NULL;

#define NHMENU_IS_SELECTABLE(item) ((item).identifier.a_obj != NULL)
#define NHMENU_IS_SELECTED(item) ((item).count != 0)
#define NHMENU_HAS_GLYPH(item) ((item).glyphinfo.glyph != NO_GLYPH)

INT_PTR CALLBACK MenuWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NHMenuListWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NHMenuTextWndProc(HWND, UINT, WPARAM, LPARAM);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL onMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL onDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void LayoutMenu(HWND hwnd);
static void SetMenuType(HWND hwnd, int type);
static void SetMenuListType(HWND hwnd, int now);
static HWND GetMenuControl(HWND hwnd);
static void SelectMenuItem(HWND hwndList, PNHMenuWindow data, int item,
                           int count);
static void reset_menu_count(HWND hwndList, PNHMenuWindow data);
static BOOL onListChar(HWND hWnd, HWND hwndList, WORD ch);

/*-----------------------------------------------------------------------------*/
HWND
mswin_init_menu_window(int type)
{
    HWND ret;
    RECT rt;

    /* get window position */
    if (GetNHApp()->bAutoLayout) {
        SetRect(&rt, 0, 0, 0, 0);
    } else {
        mswin_get_window_placement(NHW_MENU, &rt);
    }

    /* create menu window object */
    ret = CreateDialog(GetNHApp()->hApp, MAKEINTRESOURCE(IDD_MENU),
                       GetNHApp()->hMainWnd, MenuWndProc);
    if (!ret) {
        panic("Cannot create menu window");
    }

    /* move it in the predefined position */
    if (!GetNHApp()->bAutoLayout) {
        MoveWindow(ret, rt.left, rt.top, rt.right - rt.left,
                   rt.bottom - rt.top, TRUE);
    }

    /* Set window caption */
    SetWindowText(ret, "Menu/Text");

    mswin_apply_window_style(ret);

    SetMenuType(ret, type);
    return ret;
}
/*-----------------------------------------------------------------------------*/
int
mswin_menu_window_select_menu(HWND hWnd, int how, MENU_ITEM_P **_selected,
                              BOOL activate)
{
    PNHMenuWindow data;
    int ret_val;
    MENU_ITEM_P *selected = NULL;
    int i;
    char *ap;

    assert(_selected != NULL);
    *_selected = NULL;
    ret_val = -1;

    data = (PNHMenuWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    /* force activate for certain menu types */
    if (data->type == MENU_TYPE_MENU
        && (how == PICK_ONE || how == PICK_ANY)) {
        activate = TRUE;
    }

    data->is_active = activate && !GetNHApp()->regNetHackMode;

    /* set menu type */
    SetMenuListType(hWnd, how);

    /* Ok, now give items a unique accelerators */
    if (data->type == MENU_TYPE_MENU) {
        char next_char = 'a';

        data->menui.menu.gacc[0] = '\0';
        ap = data->menui.menu.gacc;
        for (i = 0; i < data->menui.menu.size; i++) {
            if (data->menui.menu.items[i].accelerator != 0) {
                if (isalpha(data->menui.menu.items[i].accelerator)) {
                    next_char = (char)(data->menui.menu.items[i].accelerator + 1);
                }
            } else if (NHMENU_IS_SELECTABLE(data->menui.menu.items[i])) {
                if (isalpha(next_char)) {
                    data->menui.menu.items[i].accelerator = next_char;
                } else {
                    if (next_char > 'z')
                        next_char = 'A';
                    else if (next_char > 'Z')
                        next_char = 'a';

                    data->menui.menu.items[i].accelerator = next_char;
                }

                next_char++;
            }
        }

        /* collect group accelerators */
        for (i = 0; i < data->menui.menu.size; i++) {
            if (data->how != PICK_NONE) {
                if (data->menui.menu.items[i].group_accel
                    && !strchr(data->menui.menu.gacc,
                               data->menui.menu.items[i].group_accel)) {
                    *ap++ = data->menui.menu.items[i].group_accel;
                    *ap = '\x0';
                }
            }
        }

        reset_menu_count(NULL, data);
    }

    LayoutMenu(hWnd); // show dialog buttons

    if (activate) {
        mswin_popup_display(hWnd, &data->done);
    } else {
        SetFocus(GetNHApp()->hMainWnd);
        mswin_layout_main_window(hWnd);
    }

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
            for (i = 0; i < data->menui.menu.size; i++) {
                if (NHMENU_IS_SELECTABLE(data->menui.menu.items[i])
                    && NHMENU_IS_SELECTED(data->menui.menu.items[i])) {
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
                for (i = 0; i < data->menui.menu.size; i++) {
                    if (NHMENU_IS_SELECTABLE(data->menui.menu.items[i])
                        && NHMENU_IS_SELECTED(data->menui.menu.items[i])) {
                        selected[sel_ind].item =
                            data->menui.menu.items[i].identifier;
                        selected[sel_ind].count = data->menui.menu.items[i].count;
                        sel_ind++;
                    }
                }
                ret_val = sel_ind;
                *_selected = selected;
            }
        }
    }

    if (activate) {
        data->is_active = FALSE;
        LayoutMenu(hWnd); // hide dialog buttons
        mswin_popup_destroy(hWnd);

        /* If we just used the permanent inventory window to pick something,
         * set the menu back to its display inventory state.
         */
        if (iflags.perm_invent && mswin_winid_from_handle(hWnd) == WIN_INVEN
            && how != PICK_NONE) {
            data->menui.menu.prompt[0] = '\0';
            SetMenuListType(hWnd, PICK_NONE);
            for (i = 0; i < data->menui.menu.size; i++)
                data->menui.menu.items[i].count = 0;
            LayoutMenu(hWnd);
        }
    }

    return ret_val;
}
/*-----------------------------------------------------------------------------*/
INT_PTR CALLBACK
MenuWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PNHMenuWindow data = (PNHMenuWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    HWND control = GetDlgItem(hWnd, IDC_MENU_TEXT);
    TCHAR title[MAX_LOADSTRING];

    switch (message) {
    case WM_INITDIALOG: {

        HDC hdc = GetDC(control);
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
        data->bmpDC = CreateCompatibleDC(hdc);
        data->is_active = FALSE;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) data);

        /* set font for the text cotrol */
        cached_font * font = mswin_get_font(NHW_MENU, ATR_NONE, hdc, FALSE);
        SendMessage(control, WM_SETFONT,
                    (WPARAM) font->hFont,
                    (LPARAM) 0);
        ReleaseDC(control, hdc);

        /* subclass edit control */
        editControlWndProc =
            (WNDPROC) GetWindowLongPtr(control, GWLP_WNDPROC);
        SetWindowLongPtr(control, GWLP_WNDPROC, (LONG_PTR) NHMenuTextWndProc);

        /* Even though the dialog has no caption, you can still set the title
           which shows on Alt-Tab */
        LoadString(GetNHApp()->hApp, IDS_APP_TITLE, title, MAX_LOADSTRING);
        SetWindowText(hWnd, title);

        /* set focus to text control for now */
        SetFocus(control);
    }
        return FALSE;

    case WM_MSNH_COMMAND:
        onMSNHCommand(hWnd, wParam, lParam);
        break;

    case WM_SIZE: {
        RECT rt;
        LayoutMenu(hWnd);
        GetWindowRect(hWnd, &rt);
        ScreenToClient(GetNHApp()->hMainWnd, (LPPOINT) &rt);
        ScreenToClient(GetNHApp()->hMainWnd, ((LPPOINT) &rt) + 1);
        if (iflags.perm_invent && mswin_winid_from_handle(hWnd) == WIN_INVEN)
            mswin_update_window_placement(NHW_INVEN, &rt);
        else
            mswin_update_window_placement(NHW_MENU, &rt);
    }
        return FALSE;

    case WM_MOVE: {
        RECT rt;
        GetWindowRect(hWnd, &rt);
        ScreenToClient(GetNHApp()->hMainWnd, (LPPOINT) &rt);
        ScreenToClient(GetNHApp()->hMainWnd, ((LPPOINT) &rt) + 1);
        if (iflags.perm_invent && mswin_winid_from_handle(hWnd) == WIN_INVEN)
            mswin_update_window_placement(NHW_INVEN, &rt);
        else
            mswin_update_window_placement(NHW_MENU, &rt);
    }
        return FALSE;

    case WM_CLOSE:
        if (g.program_state.gameover) {
            data->result = -1;
            data->done = 1;
            g.program_state.stopprint++;
            return TRUE;
        } else
            return FALSE;

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            if (data->type == MENU_TYPE_MENU
                && (data->how == PICK_ONE || data->how == PICK_ANY)
                && data->menui.menu.counting) {
                HWND list;
                int i;

                /* reset counter if counting is in progress */
                list = GetMenuControl(hWnd);
                i = ListView_GetNextItem(list, -1, LVNI_FOCUSED);
                if (i >= 0) {
                    SelectMenuItem(list, data, i, 0);
                }
                return TRUE;
            } else {
                data->result = -1;
                data->done = 1;
            }
            return TRUE;

        case IDOK:
            data->done = 1;
            data->result = 0;
            return TRUE;
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
                    if (lpnmlv->iItem >= 0 && lpnmlv->iItem < data->menui.menu.size
                        && NHMENU_IS_SELECTABLE(
                               data->menui.menu.items[lpnmlv->iItem])) {
                        SelectMenuItem(lpnmlv->hdr.hwndFrom, data,
                                       lpnmlv->iItem, -1);
                        data->done = 1;
                        data->result = 0;
                        return TRUE;
                    }
                }
            } break;

            case NM_CLICK: {
                LPNMLISTVIEW lpnmitem = (LPNMLISTVIEW) lParam;
                if (lpnmitem->iItem == -1)
                    return 0;
                if (data->how == PICK_ANY) {
                    SelectMenuItem(
                        lpnmitem->hdr.hwndFrom, data, lpnmitem->iItem,
                        NHMENU_IS_SELECTED(data->menui.menu.items[lpnmitem->iItem])
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

                if (data->how == PICK_ONE || data->how == PICK_ANY) {
                    data->menui.menu.items[lpnmlv->iItem].has_focus =
                        !!(lpnmlv->uNewState & LVIS_FOCUSED);
                    ListView_RedrawItems(lpnmlv->hdr.hwndFrom, lpnmlv->iItem,
                                         lpnmlv->iItem);
                }

                /* update count for single-selection menu (follow the listview
                 * selection) */
                if (data->how == PICK_ONE) {
                    if (lpnmlv->uNewState & LVIS_SELECTED) {
                        SelectMenuItem(lpnmlv->hdr.hwndFrom, data,
                                       lpnmlv->iItem, -1);
                    }
                }

                /* check item focus */
                if (data->how == PICK_ONE || data->how == PICK_ANY) {
                    data->menui.menu.items[lpnmlv->iItem].has_focus =
                        !!(lpnmlv->uNewState & LVIS_FOCUSED);
                    ListView_RedrawItems(lpnmlv->hdr.hwndFrom, lpnmlv->iItem,
                                         lpnmlv->iItem);
                }
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
            SetFocus(GetNHApp()->hMainWnd);
        } else {
            if (IsWindow(GetMenuControl(hWnd)))
                SetFocus(GetMenuControl(hWnd));
        }
        return FALSE;

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

    case WM_CTLCOLORSTATIC: { /* sent by edit control before it is drawn */
        HDC hdcEdit = (HDC) wParam;
        HWND hwndEdit = (HWND) lParam;
        if (hwndEdit == GetDlgItem(hWnd, IDC_MENU_TEXT)) {
            SetBkColor(hdcEdit, text_bg_brush ? text_bg_color
                                              : (COLORREF) GetSysColor(
                                                    DEFAULT_COLOR_BG_TEXT));
            SetTextColor(hdcEdit, text_fg_brush ? text_fg_color
                                                : (COLORREF) GetSysColor(
                                                      DEFAULT_COLOR_FG_TEXT));
            return (INT_PTR)(text_bg_brush
                                 ? text_bg_brush
                                 : SYSCLR_TO_BRUSH(DEFAULT_COLOR_BG_TEXT));
        }
    }
    return FALSE;

    case WM_CTLCOLORDLG:
        return (INT_PTR)(text_bg_brush
                            ? text_bg_brush
                            : SYSCLR_TO_BRUSH(DEFAULT_COLOR_BG_TEXT));

    case WM_DESTROY:
        if (data) {
            DeleteDC(data->bmpDC);
            DeleteObject(data->bmpChecked);
            DeleteObject(data->bmpCheckedCount);
            DeleteObject(data->bmpNotChecked);
            if (data->type == MENU_TYPE_TEXT) {
                if (data->menui.text.text)
                    free(data->menui.text.text);
            }
            free(data);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) 0);
        }
        return TRUE;
    }
    return FALSE;
}
/*-----------------------------------------------------------------------------*/
void
onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHMenuWindow data;

    data = (PNHMenuWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (wParam) {
    case MSNH_MSG_PUTSTR: {
        PMSNHMsgPutstr msg_data = (PMSNHMsgPutstr) lParam;
        HWND text_view;
        TCHAR wbuf[BUFSZ];
        size_t text_size;
        RECT text_rt;
        HGDIOBJ saveFont;
        HDC hdc;

        if (data->type != MENU_TYPE_TEXT)
            SetMenuType(hWnd, MENU_TYPE_TEXT);

        if (!data->menui.text.text) {
            text_size = strlen(msg_data->text) + 4;
            data->menui.text.text =
                (TCHAR *) malloc(text_size * sizeof(data->menui.text.text[0]));
            ZeroMemory(data->menui.text.text,
                       text_size * sizeof(data->menui.text.text[0]));
        } else {
            text_size = _tcslen(data->menui.text.text) + strlen(msg_data->text) + 4;
            data->menui.text.text = (TCHAR *) realloc(
                data->menui.text.text, text_size * sizeof(data->menui.text.text[0]));
        }
        if (!data->menui.text.text)
            break;

        _tcscat(data->menui.text.text, NH_A2W(msg_data->text, wbuf, BUFSZ));
        _tcscat(data->menui.text.text, TEXT("\r\n"));

        text_view = GetDlgItem(hWnd, IDC_MENU_TEXT);
        if (!text_view)
            panic("cannot get text view window");
        SetWindowText(text_view, data->menui.text.text);

        /* calculate dimensions of the added line of text */
        hdc = GetDC(text_view);
        cached_font * font = mswin_get_font(NHW_MENU, ATR_NONE, hdc, FALSE);
        saveFont = SelectObject(hdc, font->hFont);
        SetRect(&text_rt, 0, 0, 0, 0);
        DrawTextA(hdc, msg_data->text, strlen(msg_data->text), &text_rt,
                 DT_CALCRECT | DT_TOP | DT_LEFT | DT_NOPREFIX
                     | DT_SINGLELINE);
        data->menui.text.text_box_size.cx =
            max(text_rt.right - text_rt.left, data->menui.text.text_box_size.cx);
        data->menui.text.text_box_size.cy += text_rt.bottom - text_rt.top;
        SelectObject(hdc, saveFont);
        ReleaseDC(text_view, hdc);
    } break;

    case MSNH_MSG_STARTMENU: {
        int i;
        if (data->type != MENU_TYPE_MENU)
            SetMenuType(hWnd, MENU_TYPE_MENU);

        if (data->menui.menu.items)
            free(data->menui.menu.items);
        data->how = PICK_NONE;
        data->menui.menu.items = NULL;
        data->menui.menu.size = 0;
        data->menui.menu.allocated = 0;
        data->done = 0;
        data->result = 0;
        for (i = 0; i < NUMTABS; ++i)
            data->menui.menu.tab_stop_size[i] = MIN_TABSTOP_SIZE;
    } break;

    case MSNH_MSG_ADDMENU: {
        PMSNHMsgAddMenu msg_data = (PMSNHMsgAddMenu) lParam;
        char *p, *p1;
        int new_item;
        HDC hDC;
        int column;
        HFONT saveFont;
        LONG menuitemwidth = 0;
        TEXTMETRIC tm;

        if (data->type != MENU_TYPE_MENU)
            break;

        if (data->menui.menu.size == data->menui.menu.allocated) {
            data->menui.menu.allocated += 10;
            data->menui.menu.items = (PNHMenuItem) realloc(
                data->menui.menu.items, data->menui.menu.allocated * sizeof(NHMenuItem));
        }

        new_item = data->menui.menu.size;
        ZeroMemory(&data->menui.menu.items[new_item],
                   sizeof(data->menui.menu.items[new_item]));
        data->menui.menu.items[new_item].glyphinfo = msg_data->glyphinfo;
        data->menui.menu.items[new_item].identifier = *msg_data->identifier;
        data->menui.menu.items[new_item].accelerator = msg_data->accelerator;
        data->menui.menu.items[new_item].group_accel = msg_data->group_accel;
        data->menui.menu.items[new_item].attr = msg_data->attr;
        strncpy(data->menui.menu.items[new_item].str, msg_data->str,
                NHMENU_STR_SIZE);
	/* prevent & being interpreted as a mnemonic start */
        strNsubst(data->menui.menu.items[new_item].str, "&", "&&", 0);
        data->menui.menu.items[new_item].presel = msg_data->presel;
        data->menui.menu.items[new_item].itemflags = msg_data->itemflags;

        /* calculate tabstop size */
        hDC = GetDC(hWnd);
        cached_font * font = mswin_get_font(NHW_MENU, msg_data->attr, hDC, FALSE);
        saveFont = SelectObject(hDC, font->hFont);
        GetTextMetrics(hDC, &tm);
        p1 = data->menui.menu.items[new_item].str;
        p = strchr(data->menui.menu.items[new_item].str, '\t');
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
            data->menui.menu.tab_stop_size[column] =
                max(data->menui.menu.tab_stop_size[column],
                    drawRect.right - drawRect.left);

            menuitemwidth += data->menui.menu.tab_stop_size[column];

            if (p != NULL)
                *p = '\t';
            else /* last string so, */
                break;

            /* add the separation only when not the last item */
            /* in the last item, we break out of the loop, in the statement
             * just above */
            menuitemwidth += TAB_SEPARATION;

            ++column;
            p1 = p + 1;
            p = strchr(p1, '\t');
        }
        SelectObject(hDC, saveFont);
        ReleaseDC(hWnd, hDC);

        /* calculate new menu width */
        data->menui.menu.menu_cx =
            max(data->menui.menu.menu_cx,
                2 * TILE_X + menuitemwidth
                    + (tm.tmAveCharWidth + tm.tmOverhang) * 12);

        /* increment size */
        data->menui.menu.size++;
    } break;

    case MSNH_MSG_ENDMENU: {
        PMSNHMsgEndMenu msg_data = (PMSNHMsgEndMenu) lParam;
        if (msg_data->text) {
            strncpy(data->menui.menu.prompt, msg_data->text,
                    sizeof(data->menui.menu.prompt) - 1);
        } else {
            ZeroMemory(data->menui.menu.prompt, sizeof(data->menui.menu.prompt));
        }
    } break;

	case MSNH_MSG_RANDOM_INPUT: {
        PostMessage(GetMenuControl(hWnd),
            WM_MSNH_COMMAND, MSNH_MSG_RANDOM_INPUT, 0);
	} break;

    }
}
/*-----------------------------------------------------------------------------*/
void
LayoutMenu(HWND hWnd)
{
    PNHMenuWindow data;
    HWND menu_ok;
    HWND menu_cancel;
    RECT clrt, rt;
    POINT pt_elem, pt_ok, pt_cancel;
    SIZE sz_elem, sz_ok, sz_cancel;

    data = (PNHMenuWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    menu_ok = GetDlgItem(hWnd, IDOK);
    menu_cancel = GetDlgItem(hWnd, IDCANCEL);

    /* get window coordinates */
    GetClientRect(hWnd, &clrt);

    // OK button
    if (data->is_active) {
        GetWindowRect(menu_ok, &rt);
        if (data->type == MENU_TYPE_TEXT
            || (data->type == MENU_TYPE_MENU && data->how == PICK_NONE)) {
            sz_ok.cx = (clrt.right - clrt.left) - 2 * MENU_MARGIN;
        } else {
            sz_ok.cx = (clrt.right - clrt.left) / 2 - 2 * MENU_MARGIN;
        }
        sz_ok.cy = rt.bottom - rt.top;
        pt_ok.x = clrt.left + MENU_MARGIN;
        pt_ok.y = clrt.bottom - MENU_MARGIN - sz_ok.cy;
        ShowWindow(menu_ok, SW_SHOW);
        MoveWindow(menu_ok, pt_ok.x, pt_ok.y, sz_ok.cx, sz_ok.cy, TRUE);
    } else {
        sz_ok.cx = sz_ok.cy = 0;
        pt_ok.x = pt_ok.y = 0;
        ShowWindow(menu_ok, SW_HIDE);
    }

    // CANCEL button
    if (data->is_active
        && !(data->type == MENU_TYPE_TEXT
             || (data->type == MENU_TYPE_MENU && data->how == PICK_NONE))) {
        GetWindowRect(menu_ok, &rt);
        sz_cancel.cx = (clrt.right - clrt.left) / 2 - 2 * MENU_MARGIN;
        pt_cancel.x = (clrt.left + clrt.right) / 2 + MENU_MARGIN;
        sz_cancel.cy = rt.bottom - rt.top;
        pt_cancel.y = clrt.bottom - MENU_MARGIN - sz_cancel.cy;
        ShowWindow(menu_cancel, SW_SHOW);
        MoveWindow(menu_cancel, pt_cancel.x, pt_cancel.y, sz_cancel.cx,
                   sz_cancel.cy, TRUE);
    } else {
        sz_cancel.cx = sz_cancel.cy = 0;
        pt_cancel.x = pt_cancel.y = 0;
        ShowWindow(menu_cancel, SW_HIDE);
    }

    // main menu control
    pt_elem.x = clrt.left + MENU_MARGIN;
    pt_elem.y = clrt.top + MENU_MARGIN;
    sz_elem.cx = (clrt.right - clrt.left) - 2 * MENU_MARGIN;
    if (data->is_active) {
        sz_elem.cy = (clrt.bottom - clrt.top) - max(sz_ok.cy, sz_cancel.cy)
                     - 3 * MENU_MARGIN;
    } else {
        sz_elem.cy = (clrt.bottom - clrt.top) - 2 * MENU_MARGIN;
    }

    if (data->type == MENU_TYPE_MENU) {
        ListView_SetColumnWidth(
            GetMenuControl(hWnd), 0,
            max(clrt.right - clrt.left - GetSystemMetrics(SM_CXVSCROLL),
                data->menui.menu.menu_cx));
    }

    MoveWindow(GetMenuControl(hWnd), pt_elem.x, pt_elem.y, sz_elem.cx,
               sz_elem.cy, TRUE);
}
/*-----------------------------------------------------------------------------*/
void
SetMenuType(HWND hWnd, int type)
{
    PNHMenuWindow data;
    HWND list, text;

    data = (PNHMenuWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    data->type = type;

    text = GetDlgItem(hWnd, IDC_MENU_TEXT);
    list = GetDlgItem(hWnd, IDC_MENU_LIST);
    if (data->type == MENU_TYPE_TEXT) {
        ShowWindow(list, SW_HIDE);
        EnableWindow(list, FALSE);
        EnableWindow(text, TRUE);
        ShowWindow(text, SW_SHOW);
        if (data->is_active)
            SetFocus(text);
    } else {
        ShowWindow(text, SW_HIDE);
        EnableWindow(text, FALSE);
        EnableWindow(list, TRUE);
        ShowWindow(list, SW_SHOW);
        if (data->is_active)
            SetFocus(list);
    }
    LayoutMenu(hWnd);
}
/*-----------------------------------------------------------------------------*/
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

    data = (PNHMenuWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (data->type != MENU_TYPE_MENU)
        return;

    data->how = how;

    switch (how) {
    case PICK_NONE:
        dwStyles = WS_VISIBLE | WS_TABSTOP | WS_CHILD | WS_VSCROLL
                   | WS_HSCROLL | LVS_REPORT | LVS_OWNERDRAWFIXED
                   | LVS_SINGLESEL;
        break;
    case PICK_ONE:
        dwStyles = WS_VISIBLE | WS_TABSTOP | WS_CHILD | WS_VSCROLL
                   | WS_HSCROLL | LVS_REPORT | LVS_OWNERDRAWFIXED
                   | LVS_SINGLESEL;
        break;
    case PICK_ANY:
        dwStyles = WS_VISIBLE | WS_TABSTOP | WS_CHILD | WS_VSCROLL
                   | WS_HSCROLL | LVS_REPORT | LVS_OWNERDRAWFIXED
                   | LVS_SINGLESEL;
        break;
    default:
        panic("how should be one of PICK_NONE, PICK_ONE or PICK_ANY");
    };

    if (strlen(data->menui.menu.prompt) == 0) {
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
    wndProcListViewOrig = (WNDPROC) GetWindowLongPtr(control, GWLP_WNDPROC);
    SetWindowLongPtr(control, GWLP_WNDPROC, (LONG_PTR) NHMenuListWndProc);

    /* set control colors */
    ListView_SetBkColor(control, menu_bg_brush ? menu_bg_color
                                               : (COLORREF) GetSysColor(
                                                     DEFAULT_COLOR_BG_MENU));
    ListView_SetTextBkColor(
        control, menu_bg_brush ? menu_bg_color : (COLORREF) GetSysColor(
                                                     DEFAULT_COLOR_BG_MENU));
    ListView_SetTextColor(
        control, menu_fg_brush ? menu_fg_color : (COLORREF) GetSysColor(
                                                     DEFAULT_COLOR_FG_MENU));

    /* set control font */
    fnt = SendMessage(hWnd, WM_GETFONT, (WPARAM) 0, (LPARAM) 0);
    SendMessage(control, WM_SETFONT, (WPARAM) fnt, (LPARAM) 0);

    /* add column to the list view */
    MonitorInfo monitorInfo;
    win10_monitor_info(hWnd, &monitorInfo);
    ZeroMemory(&lvcol, sizeof(lvcol));
    lvcol.mask = LVCF_WIDTH | LVCF_TEXT;
    lvcol.cx = monitorInfo.width;
    lvcol.pszText = NH_A2W(data->menui.menu.prompt, wbuf, BUFSZ);
    ListView_InsertColumn(control, 0, &lvcol);

    /* add items to the list view */
    for (i = 0; i < data->menui.menu.size; i++) {
        LVITEM lvitem;
        ZeroMemory(&lvitem, sizeof(lvitem));
        sprintf(buf, "%c - %s", max(data->menui.menu.items[i].accelerator, ' '),
                data->menui.menu.items[i].str);

        lvitem.mask = LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
        lvitem.iItem = i;
        lvitem.iSubItem = 0;
        lvitem.state = data->menui.menu.items[i].presel ? LVIS_SELECTED : 0;
        lvitem.pszText = NH_A2W(buf, wbuf, BUFSZ);
        lvitem.lParam = (LPARAM) &data->menui.menu.items[i];
        nItem = (int) SendMessage(control, LB_ADDSTRING, (WPARAM) 0,
                                  (LPARAM) buf);
        if (ListView_InsertItem(control, &lvitem) == -1) {
            panic("cannot insert menu item");
        }
    }
    if (data->is_active)
        SetFocus(control);
}
/*-----------------------------------------------------------------------------*/
HWND
GetMenuControl(HWND hWnd)
{
    PNHMenuWindow data;

    data = (PNHMenuWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

	/* We may continue getting window messages after a window's WM_DESTROY is
	   called.  We need to handle the case that USERDATA has been freed. */
	if (data == NULL)
		return NULL;

    if (data->type == MENU_TYPE_TEXT) {
        return GetDlgItem(hWnd, IDC_MENU_TEXT);
    } else {
        return GetDlgItem(hWnd, IDC_MENU_LIST);
    }
}
/*-----------------------------------------------------------------------------*/
BOOL
onMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPMEASUREITEMSTRUCT lpmis;
    TEXTMETRIC tm;
    HGDIOBJ saveFont;
    HDC hdc;
    PNHMenuWindow data;
    RECT list_rect;
    int i;

    UNREFERENCED_PARAMETER(wParam);

    lpmis = (LPMEASUREITEMSTRUCT) lParam;
    data = (PNHMenuWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    GetClientRect(GetMenuControl(hWnd), &list_rect);

    hdc = GetDC(GetMenuControl(hWnd));
    cached_font * font = mswin_get_font(NHW_MENU, ATR_INVERSE, hdc, FALSE);
    saveFont = SelectObject(hdc, font->hFont);
    GetTextMetrics(hdc, &tm);

    /* Set the height of the list box items to max height of the individual
     * items */
    for (i = 0; i < data->menui.menu.size; i++) {
        if (NHMENU_HAS_GLYPH(data->menui.menu.items[i])
            && !IS_MAP_ASCII(iflags.wc_map_mode)) {
            lpmis->itemHeight =
                max(lpmis->itemHeight,
                    (UINT) max(tm.tmHeight, GetNHApp()->mapTile_Y) + 2);
        } else {
            lpmis->itemHeight =
                max(lpmis->itemHeight, (UINT) max(tm.tmHeight, TILE_Y) + 2);
        }
    }

    /* set width to the window width */
    lpmis->itemWidth = list_rect.right - list_rect.left;

    SelectObject(hdc, saveFont);
    ReleaseDC(GetMenuControl(hWnd), hdc);
    return TRUE;
}
/*-----------------------------------------------------------------------------*/
BOOL
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
    int spacing = 0;

    int color = NO_COLOR, attr;
    boolean menucolr = FALSE;
    double monitorScale = win10_monitor_scale(hWnd);
    int tileXScaled = (int) (TILE_X * monitorScale);
    int tileYScaled = (int) (TILE_Y * monitorScale);

    UNREFERENCED_PARAMETER(wParam);

    lpdis = (LPDRAWITEMSTRUCT) lParam;

    /* If there are no list box items, skip this message. */
    if (lpdis->itemID == -1)
        return FALSE;

    data = (PNHMenuWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    item = &data->menui.menu.items[lpdis->itemID];

    tileDC = CreateCompatibleDC(lpdis->hDC);
    cached_font * font = mswin_get_font(NHW_MENU, item->attr, lpdis->hDC, FALSE);
    saveFont = SelectObject(lpdis->hDC, font->hFont);
    NewBg = menu_bg_brush ? menu_bg_color
                          : (COLORREF) GetSysColor(DEFAULT_COLOR_BG_MENU);
    OldBg = SetBkColor(lpdis->hDC, NewBg);
    OldFg = SetTextColor(lpdis->hDC,
                         menu_fg_brush
                             ? menu_fg_color
                             : (COLORREF) GetSysColor(DEFAULT_COLOR_FG_MENU));

    GetTextMetrics(lpdis->hDC, &tm);
    spacing = tm.tmAveCharWidth;

    /* set initial offset */
    x = lpdis->rcItem.left + 1;

    /* print check mark and letter */
    if (NHMENU_IS_SELECTABLE(*item)) {
        char buf[2];
        if (data->how != PICK_NONE) {
            HBITMAP bmpCheck;
            HBITMAP bmpSaved;

            switch (item->count) {
            case -1:
                bmpCheck = data->bmpChecked;
                break;
            case 0:
                bmpCheck = data->bmpNotChecked;
                break;
            default:
                bmpCheck = data->bmpCheckedCount;
                break;
            }

            y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tileYScaled) / 2;
            bmpSaved = SelectBitmap(data->bmpDC, bmpCheck);
            StretchBlt(lpdis->hDC, x, y, tileXScaled, tileYScaled,
                data->bmpDC, 0, 0,  CHECK_WIDTH, CHECK_HEIGHT, SRCCOPY);
            SelectObject(data->bmpDC, bmpSaved);
        }

        x += tileXScaled + spacing;

        if (item->accelerator != 0) {
            buf[0] = item->accelerator;
            buf[1] = '\x0';

            if (iflags.use_menu_color
                && (menucolr = get_menu_coloring(item->str, &color, &attr))) {
                cached_font * menu_font = mswin_get_font(NHW_MENU, attr, lpdis->hDC, FALSE);
                SelectObject(lpdis->hDC, menu_font->hFont);
                if (color != NO_COLOR)
                    SetTextColor(lpdis->hDC, nhcolor_to_RGB(color));
            }

            SetRect(&drawRect, x, lpdis->rcItem.top, lpdis->rcItem.right,
                    lpdis->rcItem.bottom);
            DrawText(lpdis->hDC, NH_A2W(buf, wbuf, 2), 1, &drawRect,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
        x += tm.tmAveCharWidth + tm.tmOverhang + spacing;
    } else {
        x += tileXScaled + tm.tmAveCharWidth + tm.tmOverhang + 2 * spacing;
    }

    /* print glyph if present */
    if (NHMENU_HAS_GLYPH(*item)) {
        if (!IS_MAP_ASCII(iflags.wc_map_mode)) {
            HGDIOBJ saveBmp;
            double monitorScale2;

            nhUse(monitorScale2);
            monitorScale2 = win10_monitor_scale(hWnd);

            saveBmp = SelectObject(tileDC, GetNHApp()->bmpMapTiles);
            ntile = item->glyphinfo.gm.tileidx;
            t_x =
                (ntile % GetNHApp()->mapTilesPerLine) * GetNHApp()->mapTile_X;
            t_y =
                (ntile / GetNHApp()->mapTilesPerLine) * GetNHApp()->mapTile_Y;

            y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tileYScaled) / 2;

            if (GetNHApp()->bmpMapTiles == GetNHApp()->bmpTiles) {
                /* using original nethack tiles - apply image transparently */
                (*GetNHApp()->lpfnTransparentBlt)(lpdis->hDC, x, y,
                                          tileXScaled, tileYScaled,
                                          tileDC, t_x, t_y, TILE_X, TILE_Y,
                                          TILE_BK_COLOR);
            } else {
                /* using custom tiles - simple blt */
                StretchBlt(lpdis->hDC, x, y, tileXScaled, tileYScaled,
                    tileDC, t_x, t_y, GetNHApp()->mapTile_X, GetNHApp()->mapTile_Y, SRCCOPY);
            }
            SelectObject(tileDC, saveBmp);
            x += tileXScaled;
        } else {
            const char *sel_ind;
            switch (item->count) {
            case -1:
                sel_ind = "+";
                break;
            case 0:
                sel_ind = "-";
                break;
            default:
                sel_ind = "#";
                break;
            }

            SetRect(&drawRect, x, lpdis->rcItem.top,
                    min(x + tm.tmAveCharWidth, lpdis->rcItem.right),
                    lpdis->rcItem.bottom);
            DrawText(lpdis->hDC, NH_A2W(sel_ind, wbuf, BUFSZ), 1, &drawRect,
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            x += tm.tmAveCharWidth;
        }
    } else {
        /* no glyph - need to adjust so help window won't look to cramped */
        x += tileXScaled;
    }

    x += spacing;

    /* draw item text */
    p1 = item->str;
    p = strchr(item->str, '\t');
    column = 0;
    SetRect(&drawRect, x, lpdis->rcItem.top,
            min(x + data->menui.menu.tab_stop_size[0], lpdis->rcItem.right),
            lpdis->rcItem.bottom);
    for (;;) {
        TCHAR wbuf2[BUFSZ];
        if (p != NULL)
            *p = '\0'; /* for time being, view tab field as zstring */
        DrawText(lpdis->hDC, NH_A2W(p1, wbuf2, BUFSZ), strlen(p1), &drawRect,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        if (p != NULL)
            *p = '\t';
        else /* last string so, */
            break;

        p1 = p + 1;
        p = strchr(p1, '\t');
        drawRect.left = drawRect.right + TAB_SEPARATION;
        ++column;
        drawRect.right = min(drawRect.left + data->menui.menu.tab_stop_size[column],
                             lpdis->rcItem.right);
    }

    /* draw focused item */
    if (item->has_focus || (NHMENU_IS_SELECTABLE(*item)
                            && data->menui.menu.items[lpdis->itemID].count != -1)) {
        RECT client_rt;

        GetClientRect(lpdis->hwndItem, &client_rt);
        client_rt.right = min(client_rt.right, lpdis->rcItem.right);
        if (NHMENU_IS_SELECTABLE(*item)
            && data->menui.menu.items[lpdis->itemID].count != 0
            && item->glyphinfo.glyph != NO_GLYPH) {
            if (data->menui.menu.items[lpdis->itemID].count == -1) {
                _stprintf(wbuf, TEXT("Count: All"));
            } else {
                _stprintf(wbuf, TEXT("Count: %d"),
                          data->menui.menu.items[lpdis->itemID].count);
            }

            /* TOOD: add blinking for blink text */

            cached_font * blink_font = mswin_get_font(NHW_MENU, ATR_BLINK, lpdis->hDC, FALSE);
            SelectObject(lpdis->hDC, blink_font->hFont);

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
            FillRect(lpdis->hDC, &drawRect,
                     menu_bg_brush ? menu_bg_brush
                                   : SYSCLR_TO_BRUSH(DEFAULT_COLOR_BG_MENU));

            /* draw text */
            DrawText(lpdis->hDC, wbuf, _tcslen(wbuf), &drawRect,
                     DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
    }
    if (item->has_focus) {
        /* draw focus rect */
        RECT client_rt;

        GetClientRect(lpdis->hwndItem, &client_rt);
        SetRect(&drawRect, client_rt.left, lpdis->rcItem.top,
                client_rt.left + ListView_GetColumnWidth(lpdis->hwndItem, 0),
                lpdis->rcItem.bottom);
        DrawFocusRect(lpdis->hDC, &drawRect);
    }

    SetTextColor(lpdis->hDC, OldFg);
    SetBkColor(lpdis->hDC, OldBg);
    SelectObject(lpdis->hDC, saveFont);
    DeleteDC(tileDC);
    return TRUE;
}
/*-----------------------------------------------------------------------------*/
BOOL
onListChar(HWND hWnd, HWND hwndList, WORD ch)
{
    int i = 0;
    PNHMenuWindow data;
    int curIndex, topIndex, pageSize;
    boolean is_accelerator = FALSE;

    data = (PNHMenuWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    is_accelerator = FALSE;
    for (i = 0; i < data->menui.menu.size; i++) {
        if (data->menui.menu.items[i].accelerator == ch) {
            is_accelerator = TRUE;
            break;
        }
    }

    /* Don't use switch if input matched an accelerator.  Sometimes
     * accelerators can conflict with menu actions.  For example, when
     * engraving the extra choice of using fingers matches MENU_UNSELECT_ALL.
     */
    if (is_accelerator)
        goto accelerator;

    switch (ch) {
    case MENU_FIRST_PAGE:
        i = 0;
        ListView_SetItemState(hwndList, i, LVIS_FOCUSED, LVIS_FOCUSED);
        ListView_EnsureVisible(hwndList, i, FALSE);
        return -2;

    case MENU_LAST_PAGE:
        i = max(0, data->menui.menu.size - 1);
        ListView_SetItemState(hwndList, i, LVIS_FOCUSED, LVIS_FOCUSED);
        ListView_EnsureVisible(hwndList, i, FALSE);
        return -2;

    case MENU_NEXT_PAGE:
        topIndex = ListView_GetTopIndex(hwndList);
        pageSize = ListView_GetCountPerPage(hwndList);
        curIndex = ListView_GetNextItem(hwndList, -1, LVNI_FOCUSED);
        /* Focus down one page */
        i = min(curIndex + pageSize, data->menui.menu.size - 1);
        ListView_SetItemState(hwndList, i, LVIS_FOCUSED, LVIS_FOCUSED);
        /* Scrollpos down one page */
        i = min(topIndex + (2 * pageSize - 1), data->menui.menu.size - 1);
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
            for (i = 0; i < data->menui.menu.size; i++) {
                if (!menuitem_invert_test(1, data->menui.menu.items[i].itemflags,
                                NHMENU_IS_SELECTED(data->menui.menu.items[i])))
                    continue;
                SelectMenuItem(hwndList, data, i, -1);
            }
            return -2;
        }
        break;

    case MENU_UNSELECT_ALL:
        if (data->how == PICK_ANY) {
            reset_menu_count(hwndList, data);
            for (i = 0; i < data->menui.menu.size; i++) {
                if (!menuitem_invert_test(2, data->menui.menu.items[i].itemflags,
                                NHMENU_IS_SELECTED(data->menui.menu.items[i])))
                    continue;
                SelectMenuItem(hwndList, data, i, 0);
            }
            return -2;
        }
        break;

    case MENU_INVERT_ALL:
        if (data->how == PICK_ANY) {
            reset_menu_count(hwndList, data);
            for (i = 0; i < data->menui.menu.size; i++) {
                if (!menuitem_invert_test(0, data->menui.menu.items[i].itemflags,
                                NHMENU_IS_SELECTED(data->menui.menu.items[i])))
                    continue;
                SelectMenuItem(hwndList, data, i,
                               NHMENU_IS_SELECTED(data->menui.menu.items[i]) ? 0
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
            to = min(data->menui.menu.size, from + pageSize);
            for (i = from; i < to; i++) {
                if (!menuitem_invert_test(1, data->menui.menu.items[i].itemflags,
                                NHMENU_IS_SELECTED(data->menui.menu.items[i])))
                    continue;
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
            to = min(data->menui.menu.size, from + pageSize);
            for (i = from; i < to; i++) {
                if (!menuitem_invert_test(2, data->menui.menu.items[i].itemflags,
                                NHMENU_IS_SELECTED(data->menui.menu.items[i])))
                    continue;
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
            to = min(data->menui.menu.size, from + pageSize);
            for (i = from; i < to; i++) {
                if (!menuitem_invert_test(0, data->menui.menu.items[i].itemflags,
                                NHMENU_IS_SELECTED(data->menui.menu.items[i])))
                    continue;
                SelectMenuItem(hwndList, data, i,
                               NHMENU_IS_SELECTED(data->menui.menu.items[i]) ? 0
                                                                       : -1);
            }
            return -2;
        }
        break;

    case MENU_SEARCH:
        if (data->how == PICK_ANY || data->how == PICK_ONE) {
            char buf[BUFSZ];

            reset_menu_count(hwndList, data);
            if (mswin_getlin_window("Search for:", buf, BUFSZ) == IDCANCEL) {
                strcpy(buf, "\033");
            }
            if (data->is_active)
                SetFocus(hwndList); // set focus back to the list control
            if (!*buf || *buf == '\033')
                return -2;
            for (i = 0; i < data->menui.menu.size; i++) {
                if (NHMENU_IS_SELECTABLE(data->menui.menu.items[i])
                    && strstr(data->menui.menu.items[i].str, buf)) {
                    if (data->how == PICK_ANY) {
                        SelectMenuItem(
                            hwndList, data, i,
                            NHMENU_IS_SELECTED(data->menui.menu.items[i]) ? 0 : -1);
                    } else if (data->how == PICK_ONE) {
                        SelectMenuItem(hwndList, data, i, -1);
                        ListView_SetItemState(hwndList, i, LVIS_FOCUSED,
                                              LVIS_FOCUSED);
                        ListView_EnsureVisible(hwndList, i, FALSE);
                        break;
                    }
                }
            }
        } else {
            mswin_nhbell();
        }
        return -2;

    case ' ': {
        if (GetNHApp()->regNetHackMode) {
            /* NetHack mode: Scroll down one page,
               ends menu when on last page. */
            SCROLLINFO si;

            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
            GetScrollInfo(hwndList, SB_VERT, &si);
            if ((si.nPos + (int) si.nPage) > (si.nMax - si.nMin)) {
                /* We're at the bottom: dismiss. */
                data->done = 1;
                data->result = 0;
                return -2;
            }
            /* We're not at the bottom: page down. */
            topIndex = ListView_GetTopIndex(hwndList);
            pageSize = ListView_GetCountPerPage(hwndList);
            curIndex = ListView_GetNextItem(hwndList, -1, LVNI_FOCUSED);
            /* Focus down one page */
            i = min(curIndex + pageSize, data->menui.menu.size - 1);
            ListView_SetItemState(hwndList, i, LVIS_FOCUSED, LVIS_FOCUSED);
            /* Scrollpos down one page */
            i = min(topIndex + (2 * pageSize - 1), data->menui.menu.size - 1);
            ListView_EnsureVisible(hwndList, i, FALSE);

            return -2;
        } else {
            /* Windows mode: ends menu for PICK_ONE/PICK_NONE
               select item for PICK_ANY */
            if (data->how == PICK_ONE || data->how == PICK_NONE) {
                data->done = 1;
                data->result = 0;
                return -2;
            } else if (data->how == PICK_ANY) {
                i = ListView_GetNextItem(hwndList, -1, LVNI_FOCUSED);
                if (i >= 0) {
                    SelectMenuItem(
                        hwndList, data, i,
                        NHMENU_IS_SELECTED(data->menui.menu.items[i]) ? 0 : -1);
                }
            }
        }
    } break;

    accelerator:
    default:
        if (strchr(data->menui.menu.gacc, ch)
            && !(ch == '0' && data->menui.menu.counting)) {
            /* matched a group accelerator */
            if (data->how == PICK_ANY || data->how == PICK_ONE) {
                reset_menu_count(hwndList, data);
                for (i = 0; i < data->menui.menu.size; i++) {
                    if (NHMENU_IS_SELECTABLE(data->menui.menu.items[i])
                        && data->menui.menu.items[i].group_accel == ch) {
                        if (data->how == PICK_ANY) {
                            SelectMenuItem(
                                hwndList, data, i,
                                NHMENU_IS_SELECTED(data->menui.menu.items[i]) ? 0
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

        if (isdigit((uchar) ch)) {
            int count;
            i = ListView_GetNextItem(hwndList, -1, LVNI_FOCUSED);
            if (i >= 0) {
                count = data->menui.menu.items[i].count;
                if (count == -1)
                    count = 0;
                count *= 10L;
                count += (int) (ch - '0');
                if (count != 0) /* ignore leading zeros */ {
                    data->menui.menu.counting = TRUE;
                    data->menui.menu.items[i].count = min(100000, count);
                    ListView_RedrawItems(hwndList, i,
                                         i); /* update count mark */
                }
            }
            return -2;
        }

        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
            || is_accelerator) {
            if (data->how == PICK_ANY || data->how == PICK_ONE) {
                topIndex = ListView_GetTopIndex(hwndList);
                if( topIndex < 0 || topIndex > data->menui.menu.size ) break; // impossible?
                int iter = topIndex;
                do {
                    i = iter % data->menui.menu.size;
                    if (data->menui.menu.items[i].accelerator == ch) {
                        if (data->how == PICK_ANY) {
                            SelectMenuItem(
                                hwndList, data, i,
                                NHMENU_IS_SELECTED(data->menui.menu.items[i]) ? 0
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
                } while( (++iter % data->menui.menu.size) != topIndex );
            }
        }
        break;
    }

    reset_menu_count(hwndList, data);
    return -1;
}
/*-----------------------------------------------------------------------------*/
void
mswin_menu_window_size(HWND hWnd, LPSIZE sz)
{
    HWND control;
    PNHMenuWindow data;
    RECT rt, wrt;
    int extra_cx;

    data = (PNHMenuWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (data) {
        control = GetMenuControl(hWnd);

        /* get the control size */
        GetClientRect(control, &rt);
        sz->cx = rt.right - rt.left;
        sz->cy = rt.bottom - rt.top;

        /* calculate "extra" space around the control */
        GetWindowRect(hWnd, &wrt);
        extra_cx = (wrt.right - wrt.left) - sz->cx;

        if (data->type == MENU_TYPE_MENU) {
            sz->cx = data->menui.menu.menu_cx + GetSystemMetrics(SM_CXVSCROLL);
        } else {
            /* Use the width of the text box */
            sz->cx = data->menui.text.text_box_size.cx
                     + 2 * GetSystemMetrics(SM_CXVSCROLL);
        }
        sz->cx += extra_cx;
    } else {
        /* uninitilized window */
        GetClientRect(hWnd, &rt);
        sz->cx = rt.right - rt.left;
        sz->cy = rt.bottom - rt.top;
    }
}
/*-----------------------------------------------------------------------------*/
void
SelectMenuItem(HWND hwndList, PNHMenuWindow data, int item, int count)
{
    int i;

    if (item < 0 || item >= data->menui.menu.size)
        return;

    if (data->how == PICK_ONE && count != 0) {
        for (i = 0; i < data->menui.menu.size; i++)
            if (item != i && data->menui.menu.items[i].count != 0) {
                data->menui.menu.items[i].count = 0;
                ListView_RedrawItems(hwndList, i, i);
            };
    }

    data->menui.menu.items[item].count = count;
    ListView_RedrawItems(hwndList, item, item);
    reset_menu_count(hwndList, data);
}
/*-----------------------------------------------------------------------------*/
void
reset_menu_count(HWND hwndList, PNHMenuWindow data)
{
    int i;
    data->menui.menu.counting = FALSE;
    if (IsWindow(hwndList)) {
        i = ListView_GetNextItem((hwndList), -1, LVNI_FOCUSED);
        if (i >= 0)
            ListView_RedrawItems(hwndList, i, i);
    }
}
/*-----------------------------------------------------------------------------*/
/* List window Proc */
LRESULT CALLBACK
NHMenuListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hWndParent = GetParent(hWnd);
    BOOL bUpdateFocusItem;

    /* we will redraw focused item whenever horizontal scrolling occurs
       since "Count: XXX" indicator is garbled by scrolling */
    bUpdateFocusItem = FALSE;

    switch (message) {
    case WM_KEYDOWN:
        if (wParam == VK_LEFT || wParam == VK_RIGHT)
            bUpdateFocusItem = TRUE;
        break;

    case WM_CHAR: /* filter keyboard input for the control */
        if (wParam > 0 && wParam < 256
            && onListChar(GetParent(hWnd), hWnd, (char) wParam) == -2) {
            return 0;
        } else {
            return 1;
        }
        break;

    case WM_SIZE:
    case WM_HSCROLL:
        bUpdateFocusItem = TRUE;
        break;

    case WM_SETFOCUS:
        if (GetParent(hWnd) != GetNHApp()->hPopupWnd) {
            SetFocus(GetNHApp()->hMainWnd);
        }
        return FALSE;

    case WM_MSNH_COMMAND:
        if (wParam == MSNH_MSG_RANDOM_INPUT) {
            char c = randomkey();
            if (c == '\n')
                PostMessage(hWndParent, WM_COMMAND, MAKELONG(IDOK, 0), 0);
            else if (c == '\033')
                PostMessage(hWndParent, WM_COMMAND, MAKELONG(IDCANCEL, 0), 0);
            else
                PostMessage(hWnd, WM_CHAR, c, 0);
            return 0;
        }
        break;

    }

    /* update focused item */
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

    /* call ListView control window proc */
    if (wndProcListViewOrig)
        return CallWindowProc(wndProcListViewOrig, hWnd, message, wParam,
                              lParam);
    else
        return 0;
}
/*-----------------------------------------------------------------------------*/
/* Text control window proc - implements scrolling without a cursor */
LRESULT CALLBACK
NHMenuTextWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hWndParent = GetParent(hWnd);
    HDC hDC;
    RECT rc;

    switch (message) {
    case WM_ERASEBKGND:
        hDC = (HDC) wParam;
        GetClientRect(hWnd, &rc);
        FillRect(hDC, &rc, text_bg_brush
                               ? text_bg_brush
                               : SYSCLR_TO_BRUSH(DEFAULT_COLOR_BG_TEXT));
        return 0;

    case WM_KEYDOWN:
        switch (wParam) {
        /* close on space in Windows mode
           page down on space in NetHack mode */
        case VK_SPACE: {
            SCROLLINFO si;

            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
            GetScrollInfo(hWnd, SB_VERT, &si);
            /* If nethackmode and not at the end of the list */
            if (GetNHApp()->regNetHackMode
                && (si.nPos + (int) si.nPage) <= (si.nMax - si.nMin))
                SendMessage(hWnd, EM_SCROLL, SB_PAGEDOWN, 0);
            else
                PostMessage(hWndParent, WM_COMMAND, MAKELONG(IDOK, 0), 0);
            return 0;
        }
        case VK_NEXT:
            SendMessage(hWnd, EM_SCROLL, SB_PAGEDOWN, 0);
            return 0;
        case VK_PRIOR:
            SendMessage(hWnd, EM_SCROLL, SB_PAGEUP, 0);
            return 0;
        case VK_UP:
            SendMessage(hWnd, EM_SCROLL, SB_LINEUP, 0);
            return 0;
        case VK_DOWN:
            SendMessage(hWnd, EM_SCROLL, SB_LINEDOWN, 0);
            return 0;
        }
        break;

    case WM_CHAR:
        switch(wParam) {
        case MENU_FIRST_PAGE:
            SendMessage(hWnd, EM_SCROLL, SB_TOP, 0);
            return 0;
        case MENU_LAST_PAGE:
            SendMessage(hWnd, EM_SCROLL, SB_BOTTOM, 0);
            return 0;
        case MENU_NEXT_PAGE:
            SendMessage(hWnd, EM_SCROLL, SB_PAGEDOWN, 0);
            return 0;
        case MENU_PREVIOUS_PAGE:
            SendMessage(hWnd, EM_SCROLL, SB_PAGEUP, 0);
            return 0;
        }
        break;

    /* edit control needs to know nothing of its focus */
    case WM_SETFOCUS:
        HideCaret(hWnd);
        return 0;

    case WM_MSNH_COMMAND:
        if (wParam == MSNH_MSG_RANDOM_INPUT) {
            char c = randomkey();
            if (c == '\n')
                PostMessage(hWndParent, WM_COMMAND, MAKELONG(IDOK, 0), 0);
            else if (c == '\033')
                PostMessage(hWndParent, WM_COMMAND, MAKELONG(IDCANCEL, 0), 0);
            else
                PostMessage(hWnd, WM_CHAR, c, 0);
            return 0;
        }
        break;

    }

    if (editControlWndProc)
        return CallWindowProc(editControlWndProc, hWnd, message, wParam,
                              lParam);
    else
        return 0;
}
/*-----------------------------------------------------------------------------*/

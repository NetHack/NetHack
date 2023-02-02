/* NetHack 3.6	mhdlg.c	$NHDT-Date: 1432512802 2015/05/25 00:13:22 $  $NHDT-Branch: master $:$NHDT-Revision: 1.18 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

/* various dialog boxes are defined here */

#include "winMS.h"
#include "hack.h"
#include "func_tab.h"
#include "mhdlg.h"
#include "mhmain.h"

#define CheckDlgButton(dlg, btn_id, st) \
    SendDlgItemMessage((dlg), (btn_id), BM_SETCHECK, (WPARAM)(st), 0)

/*---------------------------------------------------------------*/
/* data for getlin dialog */
struct getlin_data {
    const char *question;
    char *result;
    size_t result_size;
};

LRESULT CALLBACK GetlinDlgProc(HWND, UINT, WPARAM, LPARAM);

int
mswin_getlin_window(const char *question, char *result, size_t result_size)
{
    int ret;
    struct getlin_data data;

    /* initilize dialog data */
    ZeroMemory(&data, sizeof(data));
    data.question = question;
    data.result = result;
    data.result_size = result_size;

    /* create modal dialog window */
    ret = DialogBoxParam(GetNHApp()->hApp, MAKEINTRESOURCE(IDD_GETLIN),
                         GetNHApp()->hMainWnd, GetlinDlgProc, (LPARAM) &data);
    if (ret == -1)
        panic("Cannot create getlin window");

    return ret;
}

LRESULT CALLBACK
GetlinDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct getlin_data *data;
    RECT main_rt, text_rt, dlg_rt, edit_rt;
    SIZE dlg_sz;
    TCHAR wbuf[BUFSZ];
    HDC hdc;
    HWND control;
    HWND hwndMap;

#if defined(WIN_CE_POCKETPC)
    SHInputDialog(hWnd, message, wParam);
#endif

    switch (message) {
    case WM_INITDIALOG:
        data = (struct getlin_data *) lParam;
        SetWindowText(hWnd, NH_A2W(data->question, wbuf, sizeof(wbuf)));
        SetWindowLong(hWnd, GWL_USERDATA, lParam);

        /* get title text width */
        SetRect(&text_rt, 0, 0, 100, 50);
        hdc = GetWindowDC(hWnd);
        DrawText(hdc, wbuf, _tcslen(wbuf), &text_rt,
                 DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX | DT_LEFT
                     | DT_VCENTER);
        ReleaseDC(hWnd, hdc);

        /* center dialog in the main window */
        GetWindowRect(hWnd, &dlg_rt);
        hwndMap = mswin_hwnd_from_winid(WIN_MAP);
        GetWindowRect(IsWindow(hwndMap) ? hwndMap : GetNHApp()->hMainWnd,
                      &main_rt);
        dlg_sz.cx = max(
            dlg_rt.right - dlg_rt.left,
            min(text_rt.right - text_rt.left + GetSystemMetrics(SM_CXICON),
                main_rt.right - main_rt.left));
        dlg_sz.cy =
            min(dlg_rt.bottom - dlg_rt.top, main_rt.bottom - main_rt.top);
        dlg_rt.left = (main_rt.left + main_rt.right - dlg_sz.cx) / 2;
        dlg_rt.right = dlg_rt.left + dlg_sz.cx;
        dlg_rt.top = (main_rt.top + main_rt.bottom - dlg_sz.cy) / 2;
        dlg_rt.bottom = dlg_rt.top + dlg_sz.cy;
        MoveWindow(hWnd, (main_rt.left + main_rt.right - dlg_sz.cx) / 2,
                   (main_rt.top + main_rt.bottom - dlg_sz.cy) / 2, dlg_sz.cx,
                   dlg_sz.cy, TRUE);

        /* change layout of controls */
        GetClientRect(hWnd, &dlg_rt);

        control = GetDlgItem(hWnd, IDC_GETLIN_EDIT);
        GetWindowRect(control, &edit_rt);
        MoveWindow(control, 0, 0, dlg_rt.right - dlg_rt.left,
                   edit_rt.bottom - edit_rt.top, TRUE);

        control = GetDlgItem(hWnd, IDOK);
        GetWindowRect(control, &text_rt);
        MoveWindow(control, 0, edit_rt.bottom - edit_rt.top,
                   (dlg_rt.right - dlg_rt.left) / 2,
                   text_rt.bottom - text_rt.top, TRUE);

        control = GetDlgItem(hWnd, IDCANCEL);
        GetWindowRect(control, &text_rt);
        MoveWindow(control, (dlg_rt.right - dlg_rt.left) / 2,
                   edit_rt.bottom - edit_rt.top,
                   (dlg_rt.right - dlg_rt.left) / 2,
                   text_rt.bottom - text_rt.top, TRUE);

#if defined(WIN_CE_SMARTPHONE)
        NHSPhoneDialogSetup(hWnd, IDC_SPHONE_DIALOGBAR, TRUE, FALSE);
#endif

        /* set focus to the edit control */
        SetFocus(GetDlgItem(hWnd, IDC_GETLIN_EDIT));

        /* tell windows that we've set the focus */
        return FALSE;
        break;

    case WM_COMMAND: {
        TCHAR wbuf[BUFSZ];

        switch (LOWORD(wParam)) {
        /* OK button was pressed */
        case IDOK:
            data = (struct getlin_data *) GetWindowLong(hWnd, GWL_USERDATA);
            SendDlgItemMessage(hWnd, IDC_GETLIN_EDIT, WM_GETTEXT,
                               (WPARAM) sizeof(wbuf), (LPARAM) wbuf);
            NH_W2A(wbuf, data->result, data->result_size);

        /* Fall through. */

        /* cancel button was pressed */
        case IDCANCEL:
            EndDialog(hWnd, wParam);
            return TRUE;
        }
    } break;

#if defined(WIN_CE_SMARTPHONE)
    case WM_HOTKEY:
        if (VK_TBACK == HIWORD(lParam)) {
            SHSendBackToFocusWindow(message, wParam, lParam);
        }
        break;
#endif

    } /* end switch (message) */
    return FALSE;
}

/*---------------------------------------------------------------*/
/* dialog data for the list of extended commands */
struct extcmd_data {
    int *selection;
};

LRESULT CALLBACK ExtCmdDlgProc(HWND, UINT, WPARAM, LPARAM);

int
mswin_ext_cmd_window(int *selection)
{
    int ret;
    struct extcmd_data data;

    /* init dialog data */
    ZeroMemory(&data, sizeof(data));
    *selection = -1;
    data.selection = selection;

    /* create modal dialog window */
    ret = DialogBoxParam(GetNHApp()->hApp, MAKEINTRESOURCE(IDD_EXTCMD),
                         GetNHApp()->hMainWnd, ExtCmdDlgProc, (LPARAM) &data);
    if (ret == -1)
        panic("Cannot create extcmd window");
    return ret;
}

LRESULT CALLBACK
ExtCmdDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct extcmd_data *data;
    RECT main_rt, dlg_rt;
    SIZE dlg_sz;
    int i;
    const char *ptr;
    TCHAR wbuf[255];

    switch (message) {
    case WM_INITDIALOG:
        data = (struct extcmd_data *) lParam;
        SetWindowLong(hWnd, GWL_USERDATA, lParam);

        /* center dialog in the main window */
        GetWindowRect(GetNHApp()->hMainWnd, &main_rt);
        GetWindowRect(hWnd, &dlg_rt);
        dlg_sz.cx = dlg_rt.right - dlg_rt.left;
        dlg_sz.cy = dlg_rt.bottom - dlg_rt.top;

        dlg_rt.left = (main_rt.left + main_rt.right - dlg_sz.cx) / 2;
        dlg_rt.right = dlg_rt.left + dlg_sz.cx;
        dlg_rt.top = (main_rt.top + main_rt.bottom - dlg_sz.cy) / 2;
        dlg_rt.bottom = dlg_rt.top + dlg_sz.cy;
        MoveWindow(hWnd, (main_rt.left + main_rt.right - dlg_sz.cx) / 2,
                   (main_rt.top + main_rt.bottom - dlg_sz.cy) / 2, dlg_sz.cx,
                   dlg_sz.cy, TRUE);

        /* fill combobox with extended commands */
        for (i = 0; (ptr = extcmdlist[i].ef_txt); i++) {
            SendDlgItemMessage(hWnd, IDC_EXTCMD_LIST, LB_ADDSTRING,
                               (WPARAM) 0,
                               (LPARAM) NH_A2W(ptr, wbuf, sizeof(wbuf)));
        }

#if defined(WIN_CE_SMARTPHONE)
        NHSPhoneDialogSetup(hWnd, IDC_SPHONE_DIALOGBAR, FALSE, FALSE);

        GetClientRect(hWnd, &dlg_rt);
        MoveWindow(GetDlgItem(hWnd, IDC_EXTCMD_LIST), dlg_rt.left, dlg_rt.top,
                   dlg_rt.right - dlg_rt.left, dlg_rt.bottom - dlg_rt.top,
                   TRUE);
#endif

        /* set focus to the list control */
        SetFocus(GetDlgItem(hWnd, IDC_EXTCMD_LIST));

        /* tell windows we set the focus */
        return FALSE;
        break;

    case WM_COMMAND:
        data = (struct extcmd_data *) GetWindowLong(hWnd, GWL_USERDATA);
        switch (LOWORD(wParam)) {
        /* OK button ws clicked */
        case IDOK:
            *data->selection = SendDlgItemMessage(
                hWnd, IDC_EXTCMD_LIST, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
            if (*data->selection == LB_ERR)
                *data->selection = -1;
        /* Fall through. */

        /* CANCEL button ws clicked */
        case IDCANCEL:
            EndDialog(hWnd, wParam);
            return TRUE;

        /* list control events */
        case IDC_EXTCMD_LIST:
            switch (HIWORD(wParam)) {
            case LBN_DBLCLK:
                /* double click within the list
                       wParam
                         The low-order word is the list box identifier.
                         The high-order word is the notification message.
                       lParam
                         Handle to the list box
                      */
                *data->selection = SendMessage((HWND) lParam, LB_GETCURSEL,
                                               (WPARAM) 0, (LPARAM) 0);
                if (*data->selection == LB_ERR)
                    *data->selection = -1;
                EndDialog(hWnd, IDOK);
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

/*---------------------------------------------------------------*/
/* player selector dialog data */
struct plsel_data {
    int *selection;
};

BOOL CALLBACK PlayerSelectorDlgProc(HWND, UINT, WPARAM, LPARAM);
static void plselInitDialog(HWND hWnd);
static void plselAdjustLists(HWND hWnd, int changed_opt);
static int plselFinalSelection(HWND hWnd, int *selection);

int
mswin_player_selection_window(int *selection)
{
    int ret;
    struct plsel_data data;

    /* init dialog data */
    ZeroMemory(&data, sizeof(data));
    data.selection = selection;

    /* create modal dialog */
    ret = DialogBoxParam(
        GetNHApp()->hApp, MAKEINTRESOURCE(IDD_PLAYER_SELECTOR),
        GetNHApp()->hMainWnd, PlayerSelectorDlgProc, (LPARAM) &data);
    if (ret == -1)
        panic("Cannot create getlin window");

    return ret;
}

BOOL CALLBACK
PlayerSelectorDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct plsel_data *data;
    RECT main_rt, dlg_rt;
    SIZE dlg_sz;

    switch (message) {
    case WM_INITDIALOG:
        data = (struct plsel_data *) lParam;
        SetWindowLong(hWnd, GWL_USERDATA, lParam);

        /* center dialog in the main window */
        GetWindowRect(GetNHApp()->hMainWnd, &main_rt);
        GetWindowRect(hWnd, &dlg_rt);
        dlg_sz.cx = dlg_rt.right - dlg_rt.left;
        dlg_sz.cy = dlg_rt.bottom - dlg_rt.top;

        dlg_rt.left = (main_rt.left + main_rt.right - dlg_sz.cx) / 2;
        dlg_rt.right = dlg_rt.left + dlg_sz.cx;
        dlg_rt.top = (main_rt.top + main_rt.bottom - dlg_sz.cy) / 2;
        dlg_rt.bottom = dlg_rt.top + dlg_sz.cy;
        MoveWindow(hWnd, (main_rt.left + main_rt.right - dlg_sz.cx) / 2,
                   (main_rt.top + main_rt.bottom - dlg_sz.cy) / 2, dlg_sz.cx,
                   dlg_sz.cy, TRUE);

        /* init dialog */
        plselInitDialog(hWnd);

#if defined(WIN_CE_SMARTPHONE)
        NHSPhoneDialogSetup(hWnd, IDC_SPHONE_DIALOGBAR, FALSE, FALSE);
#endif
        /* set focus on the role checkbox (random) field */
        SetFocus(GetDlgItem(hWnd, IDC_PLSEL_ROLE_RANDOM));

        /* tell windows we set the focus */
        return FALSE;
        break;

    case WM_COMMAND:
        data = (struct plsel_data *) GetWindowLong(hWnd, GWL_USERDATA);
        switch (LOWORD(wParam)) {
        /* OK button was clicked */
        case IDOK:
            if (plselFinalSelection(hWnd, data->selection)) {
                EndDialog(hWnd, wParam);
            } else {
                MessageBox(
                    hWnd, TEXT("Cannot match this role. Try something else."),
                    TEXT("STOP"), MB_OK);
            }
            return TRUE;

        /* CANCEL button was clicked */
        case IDCANCEL:
            *data->selection = -1;
            EndDialog(hWnd, wParam);
            return TRUE;

        /* following are events from dialog controls:
           "random" checkboxes send BN_CLICKED messages;
           role/race/... combo-boxes send CBN_SELENDOK
           if something was selected;
        */
        case IDC_PLSEL_ROLE_RANDOM:
            if (HIWORD(wParam) == BN_CLICKED) {
                /* enable corresponding list window if "random"
                   checkbox was "unchecked" */
                EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_ROLE_LIST),
                             SendMessage((HWND) lParam, BM_GETCHECK, 0, 0)
                                 == BST_UNCHECKED);
            }
            break;

        case IDC_PLSEL_RACE_RANDOM:
            if (HIWORD(wParam) == BN_CLICKED) {
                EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_RACE_LIST),
                             SendMessage((HWND) lParam, BM_GETCHECK, 0, 0)
                                 == BST_UNCHECKED);
            }
            break;

        case IDC_PLSEL_GENDER_RANDOM:
            if (HIWORD(wParam) == BN_CLICKED) {
                EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_GENDER_LIST),
                             SendMessage((HWND) lParam, BM_GETCHECK, 0, 0)
                                 == BST_UNCHECKED);
            }
            break;

        case IDC_PLSEL_ALIGN_RANDOM:
            if (HIWORD(wParam) == BN_CLICKED) {
                EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_ALIGN_LIST),
                             SendMessage((HWND) lParam, BM_GETCHECK, 0, 0)
                                 == BST_UNCHECKED);
            }
            break;

        case IDC_PLSEL_ROLE_LIST:
            if (HIWORD(wParam) == CBN_SELENDOK) {
                /* filter out invalid options if
                   the selection was made */
                plselAdjustLists(hWnd, LOWORD(wParam));
            }
            break;

        case IDC_PLSEL_RACE_LIST:
            if (HIWORD(wParam) == CBN_SELENDOK) {
                plselAdjustLists(hWnd, LOWORD(wParam));
            }
            break;

        case IDC_PLSEL_GENDER_LIST:
            if (HIWORD(wParam) == CBN_SELENDOK) {
                plselAdjustLists(hWnd, LOWORD(wParam));
            }
            break;

        case IDC_PLSEL_ALIGN_LIST:
            if (HIWORD(wParam) == CBN_SELENDOK) {
                plselAdjustLists(hWnd, LOWORD(wParam));
            }
            break;
        }
        break;
    }
    return FALSE;
}

void
setComboBoxValue(HWND hWnd, int combo_box, int value)
{
    int index_max = SendDlgItemMessage(hWnd, combo_box, CB_GETCOUNT, 0, 0);
    int index;
    int value_to_set = LB_ERR;
    for (index = 0; index < index_max; index++) {
        if (SendDlgItemMessage(hWnd, combo_box, CB_GETITEMDATA,
                               (WPARAM) index, 0) == value) {
            value_to_set = index;
            break;
        }
    }
    SendDlgItemMessage(hWnd, combo_box, CB_SETCURSEL, (WPARAM) value_to_set,
                       0);
}

/* initialize player selector dialog */
void
plselInitDialog(HWND hWnd)
{
    TCHAR wbuf[BUFSZ];

    /* set player name */
    SetDlgItemText(hWnd, IDC_PLSEL_NAME, NH_A2W(gp.plname, wbuf, sizeof(wbuf)));

    /* check flags for consistency */
    if (flags.initrole >= 0) {
        if (flags.initrace >= 0
            && !validrace(flags.initrole, flags.initrace)) {
            flags.initrace = ROLE_NONE;
        }

        if (flags.initgend >= 0
            && !validgend(flags.initrole, flags.initrace, flags.initgend)) {
            flags.initgend = ROLE_NONE;
        }

        if (flags.initalign >= 0
            && !validalign(flags.initrole, flags.initrace, flags.initalign)) {
            flags.initalign = ROLE_NONE;
        }
    }

    /* populate select boxes */
    plselAdjustLists(hWnd, -1);

    /* intialize roles list */
    if (flags.initrole < 0
        || !ok_role(flags.initrole, ROLE_NONE, ROLE_NONE, ROLE_NONE)) {
        CheckDlgButton(hWnd, IDC_PLSEL_ROLE_RANDOM, BST_CHECKED);
        EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_ROLE_LIST), FALSE);
    } else {
        CheckDlgButton(hWnd, IDC_PLSEL_ROLE_RANDOM, BST_UNCHECKED);
        EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_ROLE_LIST), TRUE);
        setComboBoxValue(hWnd, IDC_PLSEL_ROLE_LIST, flags.initrole);
    }

    /* intialize races list */
    if (flags.initrace < 0
        || !ok_race(flags.initrole, flags.initrace, ROLE_NONE, ROLE_NONE)) {
        CheckDlgButton(hWnd, IDC_PLSEL_RACE_RANDOM, BST_CHECKED);
        EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_RACE_LIST), FALSE);
    } else {
        CheckDlgButton(hWnd, IDC_PLSEL_RACE_RANDOM, BST_UNCHECKED);
        EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_RACE_LIST), TRUE);
        setComboBoxValue(hWnd, IDC_PLSEL_RACE_LIST, flags.initrace);
    }

    /* intialize genders list */
    if (flags.initgend < 0
        || !ok_gend(flags.initrole, flags.initrace, flags.initgend,
                    ROLE_NONE)) {
        CheckDlgButton(hWnd, IDC_PLSEL_GENDER_RANDOM, BST_CHECKED);
        EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_GENDER_LIST), FALSE);
    } else {
        CheckDlgButton(hWnd, IDC_PLSEL_GENDER_RANDOM, BST_UNCHECKED);
        EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_GENDER_LIST), TRUE);
        setComboBoxValue(hWnd, IDC_PLSEL_GENDER_LIST, flags.initgend);
    }

    /* intialize alignments list */
    if (flags.initalign < 0
        || !ok_align(flags.initrole, flags.initrace, flags.initgend,
                     flags.initalign)) {
        CheckDlgButton(hWnd, IDC_PLSEL_ALIGN_RANDOM, BST_CHECKED);
        EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_ALIGN_LIST), FALSE);
    } else {
        CheckDlgButton(hWnd, IDC_PLSEL_ALIGN_RANDOM, BST_UNCHECKED);
        EnableWindow(GetDlgItem(hWnd, IDC_PLSEL_ALIGN_LIST), TRUE);
        setComboBoxValue(hWnd, IDC_PLSEL_ALIGN_LIST, flags.initalign);
    }
}

/* adjust role/race/alignment/gender list - filter out
   invalid combinations
   changed_sel points to the list where selection occurred
   (-1 if unknown)
*/
void
plselAdjustLists(HWND hWnd, int changed_sel)
{
    HWND control_role, control_race, control_gender, control_align;
    int initrole, initrace, initgend, initalign;
    int i;
    int ind;
    int valid_opt;
    TCHAR wbuf[255];

    /* get control handles */
    control_role = GetDlgItem(hWnd, IDC_PLSEL_ROLE_LIST);
    control_race = GetDlgItem(hWnd, IDC_PLSEL_RACE_LIST);
    control_gender = GetDlgItem(hWnd, IDC_PLSEL_GENDER_LIST);
    control_align = GetDlgItem(hWnd, IDC_PLSEL_ALIGN_LIST);

    /* get current selections */
    ind = SendMessage(control_role, CB_GETCURSEL, 0, 0);
    initrole = (ind == LB_ERR)
                   ? flags.initrole
                   : SendMessage(control_role, CB_GETITEMDATA, ind, 0);

    ind = SendMessage(control_race, CB_GETCURSEL, 0, 0);
    initrace = (ind == LB_ERR)
                   ? flags.initrace
                   : SendMessage(control_race, CB_GETITEMDATA, ind, 0);

    ind = SendMessage(control_gender, CB_GETCURSEL, 0, 0);
    initgend = (ind == LB_ERR)
                   ? flags.initgend
                   : SendMessage(control_gender, CB_GETITEMDATA, ind, 0);

    ind = SendMessage(control_align, CB_GETCURSEL, 0, 0);
    initalign = (ind == LB_ERR)
                    ? flags.initalign
                    : SendMessage(control_align, CB_GETITEMDATA, ind, 0);

    /* intialize roles list */
    if (changed_sel == -1) {
        valid_opt = 0;

        /* reset content and populate the list */
        SendMessage(control_role, CB_RESETCONTENT, 0, 0);
        for (i = 0; roles[i].name.m; i++) {
            if (ok_role(i, initrace, initgend, initalign)) {
                if (initgend >= 0 && flags.female && roles[i].name.f)
                    ind = SendMessage(
                        control_role, CB_ADDSTRING, (WPARAM) 0,
                        (LPARAM) NH_A2W(roles[i].name.f, wbuf, sizeof(wbuf)));
                else
                    ind = SendMessage(
                        control_role, CB_ADDSTRING, (WPARAM) 0,
                        (LPARAM) NH_A2W(roles[i].name.m, wbuf, sizeof(wbuf)));

                SendMessage(control_role, CB_SETITEMDATA, (WPARAM) ind,
                            (LPARAM) i);
                if (i == initrole) {
                    SendMessage(control_role, CB_SETCURSEL, (WPARAM) ind,
                                (LPARAM) 0);
                    valid_opt = 1;
                }
            }
        }

        /* set selection to the previously selected role
           if it is still valid */
        if (!valid_opt) {
            initrole = ROLE_NONE;
            initrace = ROLE_NONE;
            initgend = ROLE_NONE;
            initalign = ROLE_NONE;
            SendMessage(control_role, CB_SETCURSEL, (WPARAM) -1, (LPARAM) 0);
        }

        /* trigger change of the races list */
        changed_sel = IDC_PLSEL_ROLE_LIST;
    }

    /* intialize races list */
    if (changed_sel == IDC_PLSEL_ROLE_LIST) {
        valid_opt = 0;

        /* reset content and populate the list */
        SendMessage(control_race, CB_RESETCONTENT, 0, 0);
        for (i = 0; races[i].noun; i++)
            if (ok_race(initrole, i, ROLE_NONE, ROLE_NONE)) {
                ind = SendMessage(
                    control_race, CB_ADDSTRING, (WPARAM) 0,
                    (LPARAM) NH_A2W(races[i].noun, wbuf, sizeof(wbuf)));
                SendMessage(control_race, CB_SETITEMDATA, (WPARAM) ind,
                            (LPARAM) i);
                if (i == initrace) {
                    SendMessage(control_race, CB_SETCURSEL, (WPARAM) ind,
                                (LPARAM) 0);
                    valid_opt = 1;
                }
            }

        /* set selection to the previously selected race
           if it is still valid */
        if (!valid_opt) {
            initrace = ROLE_NONE;
            initgend = ROLE_NONE;
            initalign = ROLE_NONE;
            SendMessage(control_race, CB_SETCURSEL, (WPARAM) -1, (LPARAM) 0);
        }

        /* trigger change of the genders list */
        changed_sel = IDC_PLSEL_RACE_LIST;
    }

    /* intialize genders list */
    if (changed_sel == IDC_PLSEL_RACE_LIST) {
        valid_opt = 0;

        /* reset content and populate the list */
        SendMessage(control_gender, CB_RESETCONTENT, 0, 0);
        for (i = 0; i < ROLE_GENDERS; i++)
            if (ok_gend(initrole, initrace, i, ROLE_NONE)) {
                ind = SendMessage(
                    control_gender, CB_ADDSTRING, (WPARAM) 0,
                    (LPARAM) NH_A2W(genders[i].adj, wbuf, sizeof(wbuf)));
                SendMessage(control_gender, CB_SETITEMDATA, (WPARAM) ind,
                            (LPARAM) i);
                if (i == initgend) {
                    SendMessage(control_gender, CB_SETCURSEL, (WPARAM) ind,
                                (LPARAM) 0);
                    valid_opt = 1;
                }
            }

        /* set selection to the previously selected gender
           if it is still valid */
        if (!valid_opt) {
            initgend = ROLE_NONE;
            initalign = ROLE_NONE;
            SendMessage(control_gender, CB_SETCURSEL, (WPARAM) -1,
                        (LPARAM) 0);
        }

        /* trigger change of the alignments list */
        changed_sel = IDC_PLSEL_GENDER_LIST;
    }

    /* intialize alignments list */
    if (changed_sel == IDC_PLSEL_GENDER_LIST) {
        valid_opt = 0;

        /* reset content and populate the list */
        SendMessage(control_align, CB_RESETCONTENT, 0, 0);
        for (i = 0; i < ROLE_ALIGNS; i++)
            if (ok_align(initrole, initrace, initgend, i)) {
                ind = SendMessage(
                    control_align, CB_ADDSTRING, (WPARAM) 0,
                    (LPARAM) NH_A2W(aligns[i].adj, wbuf, sizeof(wbuf)));
                SendMessage(control_align, CB_SETITEMDATA, (WPARAM) ind,
                            (LPARAM) i);
                if (i == initalign) {
                    SendMessage(control_align, CB_SETCURSEL, (WPARAM) ind,
                                (LPARAM) 0);
                    valid_opt = 1;
                }
            }

        /* set selection to the previously selected alignment
           if it is still valid */
        if (!valid_opt) {
            initalign = ROLE_NONE;
            SendMessage(control_align, CB_SETCURSEL, (WPARAM) -1, (LPARAM) 0);
        }
    }
}

/* player made up his mind - get final selection here */
int
plselFinalSelection(HWND hWnd, int *selection)
{
    int ind;

    /* get current selections */
    if (SendDlgItemMessage(hWnd, IDC_PLSEL_ROLE_RANDOM, BM_GETCHECK, 0, 0)
        == BST_CHECKED) {
        flags.initrole = ROLE_RANDOM;
    } else {
        ind =
            SendDlgItemMessage(hWnd, IDC_PLSEL_ROLE_LIST, CB_GETCURSEL, 0, 0);
        flags.initrole = (ind == LB_ERR)
                             ? ROLE_RANDOM
                             : SendDlgItemMessage(hWnd, IDC_PLSEL_ROLE_LIST,
                                                  CB_GETITEMDATA, ind, 0);
    }

    if (SendDlgItemMessage(hWnd, IDC_PLSEL_RACE_RANDOM, BM_GETCHECK, 0, 0)
        == BST_CHECKED) {
        flags.initrace = ROLE_RANDOM;
    } else {
        ind =
            SendDlgItemMessage(hWnd, IDC_PLSEL_RACE_LIST, CB_GETCURSEL, 0, 0);
        flags.initrace = (ind == LB_ERR)
                             ? ROLE_RANDOM
                             : SendDlgItemMessage(hWnd, IDC_PLSEL_RACE_LIST,
                                                  CB_GETITEMDATA, ind, 0);
    }

    if (SendDlgItemMessage(hWnd, IDC_PLSEL_GENDER_RANDOM, BM_GETCHECK, 0, 0)
        == BST_CHECKED) {
        flags.initgend = ROLE_RANDOM;
    } else {
        ind = SendDlgItemMessage(hWnd, IDC_PLSEL_GENDER_LIST, CB_GETCURSEL, 0,
                                 0);
        flags.initgend = (ind == LB_ERR)
                             ? ROLE_RANDOM
                             : SendDlgItemMessage(hWnd, IDC_PLSEL_GENDER_LIST,
                                                  CB_GETITEMDATA, ind, 0);
    }

    if (SendDlgItemMessage(hWnd, IDC_PLSEL_ALIGN_RANDOM, BM_GETCHECK, 0, 0)
        == BST_CHECKED) {
        flags.initalign = ROLE_RANDOM;
    } else {
        ind = SendDlgItemMessage(hWnd, IDC_PLSEL_ALIGN_LIST, CB_GETCURSEL, 0,
                                 0);
        flags.initalign = (ind == LB_ERR)
                              ? ROLE_RANDOM
                              : SendDlgItemMessage(hWnd, IDC_PLSEL_ALIGN_LIST,
                                                   CB_GETITEMDATA, ind, 0);
    }

    /* check the role */
    if (flags.initrole == ROLE_RANDOM) {
        flags.initrole = pick_role(flags.initrace, flags.initgend,
                                   flags.initalign, PICK_RANDOM);
        if (flags.initrole < 0) {
            MessageBox(hWnd, TEXT("Incompatible role!"), TEXT("STOP"), MB_OK);
            return FALSE;
        }
    }

    /* Select a race, if necessary */
    /* force compatibility with role */
    if (flags.initrace == ROLE_RANDOM
        || !validrace(flags.initrole, flags.initrace)) {
        /* pre-selected race not valid */
        if (flags.initrace == ROLE_RANDOM) {
            flags.initrace = pick_race(flags.initrole, flags.initgend,
                                       flags.initalign, PICK_RANDOM);
        }

        if (flags.initrace < 0) {
            MessageBox(hWnd, TEXT("Incompatible race!"), TEXT("STOP"), MB_OK);
            return FALSE;
        }
    }

    /* Select a gender, if necessary */
    /* force compatibility with role/race, try for compatibility with
     * pre-selected alignment */
    if (flags.initgend < 0
        || !validgend(flags.initrole, flags.initrace, flags.initgend)) {
        /* pre-selected gender not valid */
        if (flags.initgend == ROLE_RANDOM) {
            flags.initgend = pick_gend(flags.initrole, flags.initrace,
                                       flags.initalign, PICK_RANDOM);
        }

        if (flags.initgend < 0) {
            MessageBox(hWnd, TEXT("Incompatible gender!"), TEXT("STOP"),
                       MB_OK);
            return FALSE;
        }
    }

    /* Select an alignment, if necessary */
    /* force compatibility with role/race/gender */
    if (flags.initalign < 0
        || !validalign(flags.initrole, flags.initrace, flags.initalign)) {
        /* pre-selected alignment not valid */
        if (flags.initalign == ROLE_RANDOM) {
            flags.initalign = pick_align(flags.initrole, flags.initrace,
                                         flags.initgend, PICK_RANDOM);
        } else {
            MessageBox(hWnd, TEXT("Incompatible alignment!"), TEXT("STOP"),
                       MB_OK);
            return FALSE;
        }
    }

    return TRUE;
}

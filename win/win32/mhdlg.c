/* NetHack 3.6	mhdlg.c	$NHDT-Date: 1544695946 2018/12/13 10:12:26 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.30 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

/* various dialog boxes are defined here */

#include "winMS.h"
#include "hack.h"
#include "func_tab.h"
#include "resource.h"
#include "mhdlg.h"

#include <assert.h>

/*---------------------------------------------------------------*/
/* data for getlin dialog */
struct getlin_data {
    const char *question;
    char *result;
    size_t result_size;
};

INT_PTR CALLBACK GetlinDlgProc(HWND, UINT, WPARAM, LPARAM);

int
mswin_getlin_window(const char *question, char *result, size_t result_size)
{
    if (iflags.debug_fuzzer) {
        random_response(result, (int) result_size);
        if (result[0] != '\0')
            return IDOK;
        else
            return IDCANCEL;
    }

    INT_PTR ret;
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

    return (int) ret;
}

INT_PTR CALLBACK
GetlinDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct getlin_data *data;
    RECT main_rt, dlg_rt;
    SIZE dlg_sz;
    TCHAR wbuf[BUFSZ];
    HDC WindowDC;
    HWND ControlHWND;
    SIZE WindowExtents;
    SIZE ViewPortExtents;
    RECT ControlRect;
    RECT ClientRect;
    LONG Division;
    LONG ButtonOffset;

    switch (message) {
    case WM_INITDIALOG:
        data = (struct getlin_data *) lParam;
        SetWindowText(hWnd, NH_A2W(data->question, wbuf, sizeof(wbuf)));
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) data);

        /* center dialog in the main window */
        GetWindowRect(hWnd, &dlg_rt);
        GetWindowRect(GetNHApp()->hMainWnd, &main_rt);
        WindowDC = GetWindowDC(hWnd);

        if (!GetWindowExtEx(WindowDC, &WindowExtents)
            || !GetViewportExtEx(WindowDC, &ViewPortExtents)
            || !GetTextExtentPoint32(GetWindowDC(hWnd), wbuf, _tcslen(wbuf),
                                     &dlg_sz)) {
            dlg_sz.cx = 0;
        } else {
            /* I think we need to do the following scaling */
            dlg_sz.cx *= ViewPortExtents.cx;
            dlg_sz.cx /= WindowExtents.cx;
            /* Add the size of the various items in the caption bar */
            dlg_sz.cx += GetSystemMetrics(SM_CXSIZE)
                         + 2 * (GetSystemMetrics(SM_CXBORDER)
                                + GetSystemMetrics(SM_CXFRAME));
        }

        if (dlg_sz.cx < dlg_rt.right - dlg_rt.left)
            dlg_sz.cx = dlg_rt.right - dlg_rt.left;
        dlg_sz.cy = dlg_rt.bottom - dlg_rt.top;
        dlg_rt.left = (main_rt.left + main_rt.right - dlg_sz.cx) / 2;
        dlg_rt.right = dlg_rt.left + dlg_sz.cx;
        dlg_rt.top = (main_rt.top + main_rt.bottom - dlg_sz.cy) / 2;
        dlg_rt.bottom = dlg_rt.top + dlg_sz.cy;
        MoveWindow(hWnd, (main_rt.left + main_rt.right - dlg_sz.cx) / 2,
                   (main_rt.top + main_rt.bottom - dlg_sz.cy) / 2, dlg_sz.cx,
                   dlg_sz.cy, TRUE);

        /* set focus and size of the edit control */
        ControlHWND = GetDlgItem(hWnd, IDC_GETLIN_EDIT);
        SetFocus(ControlHWND);
        GetClientRect(hWnd, &ClientRect);
        GetWindowRect(ControlHWND, &ControlRect);
        MoveWindow(ControlHWND, 0, 0, ClientRect.right - ClientRect.left,
                   ControlRect.bottom - ControlRect.top, TRUE);
        ButtonOffset = ControlRect.bottom - ControlRect.top;

        /* Now get the OK and CANCEL buttons */
        ControlHWND = GetDlgItem(hWnd, IDOK);
        GetWindowRect(ControlHWND, &ControlRect);
        Division = ((ClientRect.right - ClientRect.left)
                    - 2 * (ControlRect.right - ControlRect.left)) / 3;
        MoveWindow(ControlHWND, Division, ButtonOffset,
                   ControlRect.right - ControlRect.left,
                   ControlRect.bottom - ControlRect.top, TRUE);
        ControlHWND = GetDlgItem(hWnd, IDCANCEL);
        MoveWindow(ControlHWND,
                   Division * 2 + ControlRect.right - ControlRect.left,
                   ButtonOffset, ControlRect.right - ControlRect.left,
                   ControlRect.bottom - ControlRect.top, TRUE);

        /* tell windows that we've set the focus */
        return FALSE;
        break;

    case WM_COMMAND: {
        TCHAR wbuf[BUFSZ];

        switch (LOWORD(wParam)) {
        /* OK button was pressed */
        case IDOK:
            data =
                (struct getlin_data *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
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

    } /* end switch (message) */
    return FALSE;
}

/*---------------------------------------------------------------*/
/* dialog data for the list of extended commands */
struct extcmd_data {
    int *selection;
};

INT_PTR CALLBACK ExtCmdDlgProc(HWND, UINT, WPARAM, LPARAM);

int
mswin_ext_cmd_window(int *selection)
{
    if (iflags.debug_fuzzer) {
        *selection = rnd_extcmd_idx();

        if (*selection != -1)
            return IDOK;
        else
            return IDCANCEL;
    }

    INT_PTR ret;
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
    return (int) ret;
}

INT_PTR CALLBACK
ExtCmdDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct extcmd_data *data;
    RECT main_rt, dlg_rt;
    SIZE dlg_sz;
    int i;
    TCHAR wbuf[255];

    switch (message) {
    case WM_INITDIALOG:
        data = (struct extcmd_data *) lParam;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) data);

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
        for (i = 0; extcmdlist[i].ef_txt; i++) {
            SendDlgItemMessage(
                hWnd, IDC_EXTCMD_LIST, LB_ADDSTRING, (WPARAM) 0,
                (LPARAM) NH_A2W(extcmdlist[i].ef_txt, wbuf, sizeof(wbuf)));
        }

        /* set focus to the list control */
        SetFocus(GetDlgItem(hWnd, IDC_EXTCMD_LIST));

        /* tell windows we set the focus */
        return FALSE;
        break;

    case WM_COMMAND:
        data = (struct extcmd_data *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
        switch (LOWORD(wParam)) {
        /* OK button ws clicked */
        case IDOK:
            *data->selection = (int) SendDlgItemMessage(
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
                *data->selection = (int) SendMessage(
                    (HWND) lParam, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
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
/* player selector dialog */
typedef struct plsel_data {
    int config_race;
    int config_role;
    int config_gender;
    int config_alignment;
    HWND control_role;
    HWND control_race;
    HWND control_genders[ROLE_GENDERS];
    HWND control_aligns[ROLE_ALIGNS];
    int role_count;
    int race_count;
    HWND focus;
} plsel_data_t;

INT_PTR CALLBACK PlayerSelectorDlgProc(HWND, UINT, WPARAM, LPARAM);
static void plselInitDialog(HWND hWnd);
static void plselAdjustSelections(HWND hWnd);
static boolean plselRandomize(plsel_data_t * data);
static BOOL plselDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam);

boolean
mswin_player_selection_window()
{
    INT_PTR ret;
    plsel_data_t data;
    boolean ok = TRUE;

    /* save away configuration settings */
    data.config_role = flags.initrole;
    data.config_race = flags.initrace;
    data.config_gender = flags.initgend;
    data.config_alignment = flags.initalign;

    if (!plselRandomize(&data)) {
        /* create modal dialog */
        ret = DialogBoxParam(
            GetNHApp()->hApp, MAKEINTRESOURCE(IDD_PLAYER_SELECTOR),
            GetNHApp()->hMainWnd, PlayerSelectorDlgProc, (LPARAM) &data);
        if (ret == -1)
            panic("Cannot create getlin window");
        ok = (ret == IDOK);
    }

    return ok;
}

INT_PTR CALLBACK
PlayerSelectorDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct plsel_data *data;
    RECT main_rt, dlg_rt;
    SIZE dlg_sz;

    switch (message) {
    case WM_INITDIALOG:
        data = (struct plsel_data *) lParam;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) data);

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

        /* tell windows to set the focus */
        return TRUE;
        break;

    case WM_DRAWITEM:
        if (wParam == IDC_PLSEL_ROLE_LIST ||  wParam == IDC_PLSEL_RACE_LIST)
            return plselDrawItem(hWnd, wParam, lParam);
        break;

    case WM_NOTIFY:
        {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            HWND control = nmhdr->hwndFrom;

            data = (struct plsel_data *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

            switch (nmhdr->code) {
            case LVN_KEYDOWN:
                {
                    LPNMLVKEYDOWN lpnmkeydown = (LPNMLVKEYDOWN) lParam;

                    if (lpnmkeydown->wVKey == ' ') {
                        if (control == data->control_role) {
                            int i = ListView_GetNextItem(data->control_role, -1, LVNI_FOCUSED);
                            assert(i == -1 || ListView_GetNextItem(data->control_role, i, LVNI_FOCUSED) == -1);
                            flags.initrole = i;
                            plselAdjustSelections(hWnd);
                        } else if (control == data->control_race) {
                            int i = ListView_GetNextItem(data->control_race, -1, LVNI_FOCUSED);
                            assert(i == -1 || ListView_GetNextItem(data->control_race, i, LVNI_FOCUSED) == -1);
                            if (ok_race(flags.initrole, i, ROLE_RANDOM, ROLE_RANDOM)) {
                                flags.initrace = i;
                                plselAdjustSelections(hWnd);
                            }
                        }
                    }
                }
            break;
            case NM_CLICK:
                {
                    LPNMLISTVIEW lpnmitem = (LPNMLISTVIEW)lParam;
                    int i = lpnmitem->iItem;
                    if (i == -1)
                        return FALSE;
                    if (control == data->control_role) {
                        flags.initrole = i;
                        plselAdjustSelections(hWnd);
                    } else if(control == data->control_race) {
                        if (ok_race(flags.initrole, i, ROLE_RANDOM, ROLE_RANDOM)) {
                            flags.initrace = i;
                            plselAdjustSelections(hWnd);
                        }
                    }
                }
                break;
            case NM_KILLFOCUS:
                {
                    if (data->focus == data->control_race) {
                        data->focus = NULL;
                        ListView_RedrawItems(data->control_race, 0, data->race_count - 1);
                    } else if (data->focus == data->control_role) {
                        data->focus = NULL;
                        ListView_RedrawItems(data->control_role, 0, data->role_count - 1);
                    }
                }
                break;
            case NM_SETFOCUS:
                {
                    data->focus = control;

                    if (control == data->control_race) {
                        data->focus = data->control_race;
                        plselAdjustSelections(hWnd);
                    } else if (control == data->control_role) {
                        data->focus = data->control_role;
                        plselAdjustSelections(hWnd);
                    }
                }
                break;
            }
        }
        break;

    case WM_COMMAND:
        data = (struct plsel_data *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
        switch (LOWORD(wParam)) {
        /* OK button was clicked */
        case IDOK:
            EndDialog(hWnd, wParam);
            return TRUE;

        /* CANCEL button was clicked */
        case IDCANCEL:
            EndDialog(hWnd, wParam);
            return TRUE;

        case IDC_PLSEL_RANDOM:
            plselRandomize(data);
            plselAdjustSelections(hWnd);
            return TRUE;

        case IDC_PLSEL_GENDER_MALE:
        case IDC_PLSEL_GENDER_FEMALE:
            if (HIWORD(wParam) == BN_CLICKED) {
                int i = LOWORD(wParam) - IDC_PLSEL_GENDER_MALE;
                if (ok_gend(flags.initrole, flags.initrace, i, ROLE_RANDOM)) {
                    flags.initgend = i;
                    plselAdjustSelections(hWnd);
                }
            }
            break;


        case IDC_PLSEL_ALIGN_LAWFUL:
        case IDC_PLSEL_ALIGN_NEUTRAL:
        case IDC_PLSEL_ALIGN_CHAOTIC:
            if (HIWORD(wParam) == BN_CLICKED) {
                int i = LOWORD(wParam) - IDC_PLSEL_ALIGN_LAWFUL;
                if (ok_align(flags.initrole, flags.initrace, flags.initgend, i)) {
                    flags.initalign = i;
                    plselAdjustSelections(hWnd);
                }
            }
            break;

        }
        break;
    }
    return FALSE;
}

/* initialize player selector dialog */
void
plselInitDialog(HWND hWnd)
{
    struct plsel_data * data = (plsel_data_t *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    TCHAR wbuf[BUFSZ];
    LVCOLUMN lvcol;
    data->control_role = GetDlgItem(hWnd, IDC_PLSEL_ROLE_LIST);
    data->control_race = GetDlgItem(hWnd, IDC_PLSEL_RACE_LIST);

    ZeroMemory(&lvcol, sizeof(lvcol));
    lvcol.mask = LVCF_WIDTH;
    lvcol.cx = GetSystemMetrics(SM_CXFULLSCREEN);

    /* build role list */
    ListView_InsertColumn(data->control_role, 0, &lvcol);
    data->role_count = 0;
    for (int i = 0; roles[i].name.m; i++) {
        LVITEM lvitem;
        ZeroMemory(&lvitem, sizeof(lvitem));

        lvitem.mask = LVIF_STATE | LVIF_TEXT;
        lvitem.iItem = i;
        lvitem.iSubItem = 0;
        lvitem.state = 0;
        lvitem.stateMask = LVIS_FOCUSED;
        if (flags.female && roles[i].name.f)
            lvitem.pszText = NH_A2W(roles[i].name.f, wbuf, BUFSZ);
        else
            lvitem.pszText = NH_A2W(roles[i].name.m, wbuf, BUFSZ);
        if (ListView_InsertItem(data->control_role, &lvitem) == -1) {
            panic("cannot insert menu item");
        }
        data->role_count++;
    }

    /* build race list */
    ListView_InsertColumn(data->control_race, 0, &lvcol);
    data->race_count = 0;
    for (int i = 0; races[i].noun; i++) {
        LVITEM lvitem;
        ZeroMemory(&lvitem, sizeof(lvitem));

        lvitem.mask = LVIF_STATE | LVIF_TEXT;
        lvitem.iItem = i;
        lvitem.iSubItem = 0;
        lvitem.state = 0;
        lvitem.stateMask = LVIS_FOCUSED;
        lvitem.pszText = NH_A2W(races[i].noun, wbuf, BUFSZ);
        if (ListView_InsertItem(data->control_race, &lvitem) == -1) {
            panic("cannot insert menu item");
        }
        data->race_count++;
    }

    for(int i = 0; i < ROLE_GENDERS; i++)
        data->control_genders[i] = GetDlgItem(hWnd, IDC_PLSEL_GENDER_MALE + i);

    for(int i = 0; i < ROLE_ALIGNS; i++)
        data->control_aligns[i] = GetDlgItem(hWnd, IDC_PLSEL_ALIGN_LAWFUL + i);

    /* set gender radio button state */
    for (int i = 0; i < ROLE_GENDERS; i++)
        Button_Enable(data->control_genders[i], TRUE);

    Button_SetCheck(data->control_genders[0], BST_CHECKED);

    /* set alignment radio button state */
    for (int i = 0; i < ROLE_ALIGNS; i++)
        Button_Enable(data->control_aligns[i], TRUE);

    Button_SetCheck(data->control_aligns[0], BST_CHECKED);

    /* set player name */
    SetDlgItemText(hWnd, IDC_PLSEL_NAME, NH_A2W(plname, wbuf, sizeof(wbuf)));

    plselRandomize(data);

    /* populate select boxes */
    plselAdjustSelections(hWnd);

    /* set tab order */
    SetWindowPos(GetDlgItem(hWnd, IDCANCEL), NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(GetDlgItem(hWnd, IDC_PLSEL_RANDOM), NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(GetDlgItem(hWnd, IDOK), NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    for(int i = ROLE_GENDERS - 1; i >= 0; i--)
        SetWindowPos(data->control_genders[i], NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    for(int i = ROLE_ALIGNS - 1; i >= 0; i--)
        SetWindowPos(data->control_aligns[i], NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(data->control_race, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(data->control_role, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

}

void
plselAdjustSelections(HWND hWnd)
{
    struct plsel_data * data = (plsel_data_t *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (!ok_race(flags.initrole, flags.initrace, ROLE_RANDOM, ROLE_RANDOM))
        flags.initrace = pick_race(flags.initrole, ROLE_RANDOM, ROLE_RANDOM, ROLE_RANDOM);

    if (!ok_gend(flags.initrole, flags.initrace, flags.initgend, ROLE_RANDOM))
        flags.initgend = pick_gend(flags.initrole, flags.initrace, ROLE_RANDOM , ROLE_RANDOM);

    if (!ok_align(flags.initrole, flags.initrace, flags.initgend, flags.initalign))
        flags.initalign = pick_align(flags.initrole, flags.initrace, flags.initgend , ROLE_RANDOM);

    ListView_RedrawItems(data->control_role, 0, data->role_count - 1);
    ListView_RedrawItems(data->control_race, 0, data->race_count - 1);

    /* set gender radio button state */
    for (int i = 0; i < ROLE_GENDERS; i++) {
        BOOL enable = ok_gend(flags.initrole, flags.initrace, i, flags.initalign);
        Button_Enable(data->control_genders[i], enable);
        LRESULT state = Button_GetCheck(data->control_genders[i]);
        if (state == BST_CHECKED && flags.initgend != i)
            Button_SetCheck(data->control_genders[i], BST_UNCHECKED);
        if (state == BST_UNCHECKED && flags.initgend == i)
            Button_SetCheck(data->control_genders[i], BST_CHECKED);
    }

    /* set alignment radio button state */
    for (int i = 0; i < ROLE_ALIGNS; i++) {
        BOOL enable = ok_align(flags.initrole, flags.initrace, flags.initgend, i);
        Button_Enable(data->control_aligns[i], enable);
        LRESULT state = Button_GetCheck(data->control_aligns[i]);
        if (state == BST_CHECKED && flags.initalign != i)
            Button_SetCheck(data->control_aligns[i], BST_UNCHECKED);
        if (state == BST_UNCHECKED && flags.initalign == i)
            Button_SetCheck(data->control_aligns[i], BST_CHECKED);
    }

}

/* player made up his mind - get final selection here */
int
plselFinalSelection(HWND hWnd)
{
    int role = flags.initrole;
    int race = flags.initrace;
    int gender = flags.initgend;
    int alignment = flags.initalign;

    assert(role != ROLE_RANDOM && role != ROLE_NONE);
    assert(race != ROLE_RANDOM && race != ROLE_NONE);
    assert(gender != ROLE_RANDOM && gender != ROLE_NONE);
    assert(alignment != ROLE_RANDOM && alignment != ROLE_NONE);
    assert(ok_role(role, race, gender, alignment));
    assert(ok_race(role, race, gender, alignment));
    assert(ok_gend(role, race, gender, alignment));
    assert(ok_align(role, race, gender, alignment));

    return TRUE;
}

static boolean plselRandomize(plsel_data_t * data)
{
    boolean fully_specified = TRUE;

    // restore back to configuration settings
    flags.initrole = data->config_role;
    flags.initrace = data->config_race;
    flags.initgend = data->config_gender;
    flags.initalign = data->config_alignment;

    if (!flags.randomall) {
        if(flags.initrole == ROLE_NONE || flags.initrace == ROLE_NONE
            || flags.initgend == ROLE_NONE || flags.initalign == ROLE_NONE)
            fully_specified = FALSE;
    }

    if (flags.initrole == ROLE_NONE)
        flags.initrole = ROLE_RANDOM;
    if (flags.initrace == ROLE_NONE)
        flags.initrace = ROLE_RANDOM;
    if (flags.initgend == ROLE_NONE)
        flags.initgend = ROLE_RANDOM;
    if (flags.initalign == ROLE_NONE)
        flags.initalign = ROLE_RANDOM;

    rigid_role_checks();

    int role = flags.initrole;
    int race = flags.initrace;
    int gender = flags.initgend;
    int alignment = flags.initalign;

    assert(role != ROLE_RANDOM && role != ROLE_NONE);
    assert(race != ROLE_RANDOM && race != ROLE_NONE);
    assert(gender != ROLE_RANDOM && gender != ROLE_NONE);
    assert(alignment != ROLE_RANDOM && alignment != ROLE_NONE);

    if (!ok_role(role, race, gender, alignment)) {
        fully_specified = FALSE;
        flags.initrole = ROLE_RANDOM;
    }

    if (!ok_race(role, race, gender, alignment)) {
        fully_specified = FALSE;
        flags.initrace = ROLE_RANDOM;
    }

    if (!ok_gend(role, race, gender, alignment)) {
        fully_specified = FALSE;
        flags.initgend = ROLE_RANDOM;
    }

    if(!ok_align(role, race, gender, alignment))
    {
        fully_specified = FALSE;
        flags.initalign = ROLE_RANDOM;
    }

    rigid_role_checks();

    role = flags.initrole;
    race = flags.initrace;
    gender = flags.initgend;
    alignment = flags.initalign;

    assert(role != ROLE_RANDOM && role != ROLE_NONE);
    assert(race != ROLE_RANDOM && race != ROLE_NONE);
    assert(gender != ROLE_RANDOM && gender != ROLE_NONE);
    assert(alignment != ROLE_RANDOM && alignment != ROLE_NONE);
    assert(ok_role(role, race, gender, alignment));
    assert(ok_race(role, race, gender, alignment));
    assert(ok_gend(role, race, gender, alignment));
    assert(ok_align(role, race, gender, alignment));

    return fully_specified;
}

/*-----------------------------------------------------------------------------*/
BOOL
plselDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
     LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
    struct plsel_data * data = (plsel_data_t *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    /* If there are no list box items, skip this message. */
    if (lpdis->itemID < 0)
        return FALSE;

    HWND control = GetDlgItem(hWnd, (int) wParam);
    int i = lpdis->itemID;

    const char * string;

    boolean ok = TRUE;
    boolean selected;

    if (wParam == IDC_PLSEL_ROLE_LIST) {
        if (flags.female && roles[i].name.f)
            string = roles[i].name.f;
        else
            string = roles[i].name.m;
        selected = (flags.initrole == i);
    } else {
        assert(wParam == IDC_PLSEL_RACE_LIST);
        ok = ok_race(flags.initrole, i, ROLE_RANDOM, ROLE_RANDOM);
        string = races[i].noun;
        selected = (flags.initrace == i);
    }

    SetBkMode(lpdis->hDC, OPAQUE);
    HBRUSH brush;
    if (selected) {
        brush = CreateSolidBrush(RGB(0, 0, 0));
        SetTextColor(lpdis->hDC, RGB(255, 255, 255));
        SetBkColor(lpdis->hDC, RGB(0, 0, 0));
    } else {
        brush = CreateSolidBrush(RGB(255, 255, 255));
        if (!ok)
            SetTextColor(lpdis->hDC, RGB(220, 0, 0));
        else
            SetTextColor(lpdis->hDC, RGB(0, 0, 0));
        SetBkColor(lpdis->hDC, RGB(255, 255, 255));
    }

    FillRect(lpdis->hDC, &lpdis->rcItem, brush);
    RECT rect = lpdis->rcItem;
    rect.left += 5;
    DrawTextA(lpdis->hDC, string, strlen(string), &rect,
        DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    if (data->focus == control) {
        if (lpdis->itemState & LVIS_FOCUSED) {
            /* draw focus rect */
            RECT client_rt;

            GetClientRect(lpdis->hwndItem, &client_rt);
            SetRect(&rect, client_rt.left, lpdis->rcItem.top,
                    client_rt.left + ListView_GetColumnWidth(lpdis->hwndItem, 0),
                    lpdis->rcItem.bottom);
            DrawFocusRect(lpdis->hDC, &rect);
        }
    }

    DeleteObject(brush);

    return TRUE;
}

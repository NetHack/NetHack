/* NetHack 3.7	mhdlg.c	$NHDT-Date: 1596498347 2020/08/03 23:45:47 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.36 $ */
/* Copyright (C) 2001 by Alex Kompel */
/* NetHack may be freely redistributed.  See license for details. */

/* various dialog boxes are defined here */

#include "win10.h"
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
        TCHAR wbuf2[BUFSZ];

        switch (LOWORD(wParam)) {
        /* OK button was pressed */
        case IDOK:
            data =
                (struct getlin_data *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
            SendDlgItemMessage(hWnd, IDC_GETLIN_EDIT, WM_GETTEXT,
                               (WPARAM) sizeof(wbuf2), (LPARAM) wbuf2);
            NH_W2A(wbuf2, data->result, data->result_size);

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

/* NOTE: this enumeration is in control tab order */
enum player_selector_control {
    psc_name_group,
    psc_role_group,
    psc_race_group,
    psc_alignment_group,
    psc_gender_group,
    psc_name_box,
    psc_role_list,
    psc_race_list,
    psc_lawful_button,
    psc_neutral_button,
    psc_chaotic_button,
    psc_male_button,
    psc_female_button,
    psc_play_button,
    psc_random_button,
    psc_quit_button,
    psc_control_count
};

static const int s_psc_id[psc_control_count] = {
    IDC_PLSEL_NAME_GROUP,
    IDC_PLSEL_ROLE_GROUP,
    IDC_PLSEL_RACE_GROUP,
    IDC_PLSEL_ALIGNMENT_GROUP,
    IDC_PLSEL_GENDER_GROUP,
    IDC_PLSEL_NAME,
    IDC_PLSEL_ROLE_LIST,
    IDC_PLSEL_RACE_LIST,
    IDC_PLSEL_ALIGN_LAWFUL,
    IDC_PLSEL_ALIGN_NEUTRAL,
    IDC_PLSEL_ALIGN_CHAOTIC,
    IDC_PLSEL_GENDER_MALE,
    IDC_PLSEL_GENDER_FEMALE,
    IDOK,
    IDC_PLSEL_RANDOM,
    IDCANCEL
};

typedef struct {
    int     id;
    POINT   pos;
    SIZE    size;
    HWND    hWnd;
} control_t;

typedef struct plsel_data {
    HWND dialog;
    HWND focus;
    control_t controls[psc_control_count];
    SIZE client_size;
    int config_race;
    int config_role;
    int config_gender;
    int config_alignment;
    int role_count;
    int race_count;
} plsel_data_t;

INT_PTR CALLBACK PlayerSelectorDlgProc(HWND, UINT, WPARAM, LPARAM);
static void plselAdjustSelections(HWND hWnd);
static boolean plselRandomize(plsel_data_t * data);
static BOOL plselDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam);

boolean
mswin_player_selection_window(void)
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

int
list_view_height(HWND hWnd, int count)
{
    return (ListView_ApproximateViewRect(hWnd, -1, -1, count)) >> 16;
}

/* calculate the size and position of the controls taking into account
   the per-monitor DPI expressed as a scaling factor on sizes at 96 DPI */
void 
calculate_player_selector_layout(plsel_data_t * data)
{
    MonitorInfo monitorInfo;
    win10_monitor_info(data->dialog, &monitorInfo);

    double scale = monitorInfo.scale;

    /* Note these hard coded sizes are in 96DPI pixels and must be 
       scaled by the per-monitor DPI scaling factor */
    int list_width = (int) (120 * scale);
    int group_border = (int) (16 * scale);
    int client_border = (int) (16 * scale);
    int group_spacing = (int) (16 * scale);
    int button_width = (int) (80 * scale);
    int button_height = (int) (28 * scale);

    /* set control sizes */
    control_t * name_box = &data->controls[psc_name_box];
    name_box->size.cx = (int) (280 * scale);
    name_box->size.cy = (int) (24 * scale);

    control_t * role_list = &data->controls[psc_role_list];
    /* NOTE: we dont' scale the list view reported height as it appears these
             values are the actual size the control will be drawn at using the
             existing DPI value */
    role_list->size.cy = list_view_height(role_list->hWnd, data->role_count);
    role_list->size.cx = list_width;

    control_t * race_list = &data->controls[psc_race_list];
    race_list->size.cy = list_view_height(race_list->hWnd, data->race_count);
    race_list->size.cx = list_width;

    for(int i = psc_lawful_button; i <= psc_quit_button; i++) {
        data->controls[i].size.cx = button_width;
        data->controls[i].size.cy = button_height;
    }

    for(int i = 0; i < 3; i++) {
        control_t * group_control = &data->controls[psc_name_group + i];
        control_t * inner_control = &data->controls[psc_name_box + i];
        group_control->size.cx = inner_control->size.cx + (2 * group_border);
        group_control->size.cy = inner_control->size.cy + (2 * group_border);
    }

    control_t * alignment_group = &data->controls[psc_alignment_group];
    alignment_group->size.cx = button_width + (2 * group_border);
    alignment_group->size.cy = (3 * button_height) + (2 * group_border);

    control_t * gender_group = &data->controls[psc_gender_group];
    gender_group->size.cx = button_width + (2 * group_border);
    gender_group->size.cy = (2 * button_height) + (2 * group_border);

    /* set control positions */
    control_t * name_group = &data->controls[psc_name_group];
    name_group->pos.x = client_border;
    name_group->pos.y = client_border;

    control_t * role_group = &data->controls[psc_role_group];
    role_group->pos.x = client_border;
    role_group->pos.y = name_group->pos.y + name_group->size.cy + group_spacing;

    control_t * race_group = &data->controls[psc_race_group];
    race_group->pos.x = role_group->pos.x + role_group->size.cx + group_spacing;
    race_group->pos.y = role_group->pos.y;

    for(int i = 0; i < 3; i++) {
        control_t * group_control = &data->controls[psc_name_group + i];
        control_t * inner_control = &data->controls[psc_name_box + i];
        inner_control->pos.x = group_control->pos.x + group_border;
        inner_control->pos.y = group_control->pos.y + group_border;
    }

    alignment_group->pos.x = race_group->pos.x + race_group->size.cx + group_spacing;
    alignment_group->pos.y = race_group->pos.y;

    for(int i = psc_lawful_button; i <= psc_chaotic_button; i++) {
        data->controls[i].pos.x = alignment_group->pos.x + group_border;
        data->controls[i].pos.y = alignment_group->pos.y + group_border + 
                                  ((i -psc_lawful_button) * button_height);
    }

    gender_group->pos.x = alignment_group->pos.x;
    gender_group->pos.y = alignment_group->pos.y + alignment_group->size.cy + group_spacing;

    for(int i = psc_male_button; i <= psc_female_button; i++) {
        data->controls[i].pos.x = gender_group->pos.x + group_border;
        data->controls[i].pos.y = gender_group->pos.y + group_border + 
                                  ((i - psc_male_button)  * button_height);
    }

    int group_bottom = role_group->pos.y + role_group->size.cy;
    if (group_bottom < race_group->pos.y + race_group->size.cy) 
        group_bottom = race_group->pos.y + race_group->size.cy;
    if (group_bottom < gender_group->pos.y + gender_group->size.cy) 
        group_bottom = gender_group->pos.y + gender_group->size.cy;

    control_t * play_button = &data->controls[psc_play_button];
    play_button->pos.y = group_bottom + group_spacing;
    play_button->pos.x = role_group->pos.x;

    control_t * random_button = &data->controls[psc_random_button];
    random_button->pos.y = play_button->pos.y;
    random_button->pos.x = race_list->pos.x;

    control_t * quit_button = &data->controls[psc_quit_button];
    quit_button->pos.y = play_button->pos.y;
    quit_button->pos.x = data->controls[psc_female_button].pos.x;

    data->client_size.cx = alignment_group->pos.x + alignment_group->size.cx;
    data->client_size.cy = quit_button->pos.y + quit_button->size.cy;
    data->client_size.cx += client_border;
    data->client_size.cy += client_border;
}

void 
get_rect_size(RECT * rect, SIZE * size)
{
    size->cx = rect->right - rect->left + 1;
    size->cy = rect->bottom - rect->top + 1;
}

/* center given dialog in the main window */
void 
center_dialog(HWND dialog)
{
    RECT main_rect;
    SIZE main_size;
    RECT dialog_rect;
    SIZE dialog_size;
    POINT pos;

    GetWindowRect(GetNHApp()->hMainWnd, &main_rect);
    get_rect_size(&main_rect, &main_size);

    GetWindowRect(dialog, &dialog_rect);
    get_rect_size(&dialog_rect, &dialog_size);

    pos.x = main_rect.left + (main_size.cx - dialog_size.cx) / 2;
    pos.y = main_rect.top + (main_size.cy - dialog_size.cy) / 2;

    MoveWindow(dialog, pos.x, pos.y, dialog_size.cx,  dialog_size.cy,
        TRUE);
}

/* size the dialog such that it has the given client rect size */
void 
size_dialog(HWND dialog, SIZE new_client_size)
{
    RECT dialog_rect;
    SIZE dialog_size;
    RECT client_rect;
    SIZE client_size;

    GetWindowRect(dialog, &dialog_rect);
    get_rect_size(&dialog_rect, &dialog_size);

    GetClientRect(dialog, &client_rect);
    get_rect_size(&client_rect, &client_size);

    dialog_size.cx += new_client_size.cx - client_size.cx;
    dialog_size.cy += new_client_size.cy - client_size.cy;

    MoveWindow(dialog, dialog_rect.left, dialog_rect.top,
                       dialog_size.cx,  dialog_size.cy, TRUE);
}

/* helper routine to move all controls according to there position
   and size information */
void 
move_controls(control_t * controls, int count)
{
    control_t * control = controls;
    while(count-- > 0) {
        MoveWindow(control->hWnd, control->pos.x, control->pos.y,
            control->size.cx, control->size.cy, TRUE);
        control++;
    }
}

/* adjust the size and positions of all controls in the player
   selection dialog taking into account the per-monitor DPI. */
void
do_player_selector_layout(plsel_data_t * data)
{
    calculate_player_selector_layout(data);
    move_controls(data->controls, psc_control_count);
    size_dialog(data->dialog, data->client_size);
}

/* initialize player selector dialog */
void
plselInitDialog(struct plsel_data * data)
{
    TCHAR wbuf[BUFSZ];
    LVCOLUMN lvcol;

    SetWindowLongPtr(data->dialog, GWLP_USERDATA, (LONG_PTR) data);

    for(int i = 0; i < psc_control_count; i++) {
        data->controls[i].id = s_psc_id[i];
        data->controls[i].hWnd = GetDlgItem(data->dialog, s_psc_id[i]);
    }

    control_t * role_list = &data->controls[psc_role_list];

    ZeroMemory(&lvcol, sizeof(lvcol));
    lvcol.mask = LVCF_WIDTH;
    lvcol.cx = 1024;

    /* build role list */
    ListView_InsertColumn(role_list->hWnd, 0, &lvcol);
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
        if (ListView_InsertItem(role_list->hWnd, &lvitem) == -1) {
            panic("cannot insert menu item");
        }
        data->role_count++;
    }

    /* build race list */
    control_t * race_list = &data->controls[psc_race_list];
    ListView_InsertColumn(race_list->hWnd, 0, &lvcol);
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
        if (ListView_InsertItem(race_list->hWnd, &lvitem) == -1) {
            panic("cannot insert menu item");
        }
        data->race_count++;
    }

    /* set gender radio button state */
    control_t * gender_buttons = &data->controls[psc_male_button];
    for (int i = 0; i < ROLE_GENDERS; i++)
        Button_Enable(gender_buttons[i].hWnd, TRUE);

    Button_SetCheck(data->controls[psc_male_button].hWnd, BST_CHECKED);

    /* set alignment radio button state */
    control_t * alignment_buttons = &data->controls[psc_lawful_button];
    for (int i = 0; i < ROLE_ALIGNS; i++)
        Button_Enable(alignment_buttons[i].hWnd, TRUE);

    Button_SetCheck(data->controls[psc_lawful_button].hWnd, BST_CHECKED);

    /* set player name */
    control_t * name_box = &data->controls[psc_name_box];
    SetDlgItemText(data->dialog, name_box->id, NH_A2W(g.plname, wbuf, sizeof(wbuf)));

    plselRandomize(data);

    /* populate select boxes */
    plselAdjustSelections(data->dialog);

    /* set tab order */
    control_t * control = &data->controls[psc_quit_button];
    for(int i = psc_quit_button; i >= psc_name_box; i--, control++)
        SetWindowPos(control->hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    do_player_selector_layout(data);

    center_dialog(data->dialog);
}

INT_PTR CALLBACK
PlayerSelectorDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    plsel_data_t *data;

    switch (message) {
    case WM_INITDIALOG:

        data = (plsel_data_t *) lParam;
        data->dialog = hWnd;

        plselInitDialog(data);

        /* tell windows to set the focus */
        return TRUE;

    case WM_DRAWITEM:
        if (wParam == IDC_PLSEL_ROLE_LIST ||  wParam == IDC_PLSEL_RACE_LIST)
            return plselDrawItem(hWnd, wParam, lParam);
        break;

    case WM_NOTIFY:
        {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            HWND control = nmhdr->hwndFrom;

            data = (struct plsel_data *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

            control_t * role_control = &data->controls[psc_role_list];
            control_t * race_control = &data->controls[psc_race_list];

            switch (nmhdr->code) {
            case LVN_KEYDOWN:
                {
                    LPNMLVKEYDOWN lpnmkeydown = (LPNMLVKEYDOWN) lParam;

                    if (lpnmkeydown->wVKey == ' ') {
                        if (control == role_control->hWnd) {
                            int i = ListView_GetNextItem(control, -1, LVNI_FOCUSED);
                            assert(i == -1 || ListView_GetNextItem(control, i, LVNI_FOCUSED) == -1);
                            flags.initrole = i;
                            plselAdjustSelections(hWnd);
                        } else if (control == race_control->hWnd) {
                            int i = ListView_GetNextItem(control, -1, LVNI_FOCUSED);
                            assert(i == -1 || ListView_GetNextItem(control, i, LVNI_FOCUSED) == -1);
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
                    if (control == role_control->hWnd) {
                        flags.initrole = i;
                        plselAdjustSelections(hWnd);
                    } else if(control == race_control->hWnd) {
                        if (ok_race(flags.initrole, i, ROLE_RANDOM, ROLE_RANDOM)) {
                            flags.initrace = i;
                            plselAdjustSelections(hWnd);
                        }
                    }
                }
                break;
            case NM_KILLFOCUS:
                {
                    if (data->focus == race_control->hWnd) {
                        data->focus = NULL;
                        ListView_RedrawItems(race_control->hWnd, 0, data->race_count - 1);
                    } else if (data->focus == role_control->hWnd) {
                        data->focus = NULL;
                        ListView_RedrawItems(role_control->hWnd, 0, data->role_count - 1);
                    }
                }
                break;
            case NM_SETFOCUS:
                {
                    data->focus = control;

                    if (control == race_control->hWnd) {
                        data->focus = control;
                        plselAdjustSelections(hWnd);
                    } else if (control == role_control->hWnd) {
                        data->focus = control;
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

    case WM_DPICHANGED:
        {
            data = (struct plsel_data *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

            do_player_selector_layout(data);

            InvalidateRect(hWnd, NULL, TRUE);
        } break;
    }

    return FALSE;
}


void
plselAdjustSelections(HWND hWnd)
{
    struct plsel_data * data = (plsel_data_t *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    control_t * role_control = &data->controls[psc_role_list];
    control_t * race_control = &data->controls[psc_race_list];

    if (!ok_race(flags.initrole, flags.initrace, ROLE_RANDOM, ROLE_RANDOM))
        flags.initrace = pick_race(flags.initrole, ROLE_RANDOM, ROLE_RANDOM, ROLE_RANDOM);

    if (!ok_gend(flags.initrole, flags.initrace, flags.initgend, ROLE_RANDOM))
        flags.initgend = pick_gend(flags.initrole, flags.initrace, ROLE_RANDOM , ROLE_RANDOM);

    if (!ok_align(flags.initrole, flags.initrace, flags.initgend, flags.initalign))
        flags.initalign = pick_align(flags.initrole, flags.initrace, flags.initgend , ROLE_RANDOM);

    ListView_RedrawItems(role_control->hWnd, 0, data->role_count - 1);
    ListView_RedrawItems(race_control->hWnd, 0, data->race_count - 1);

    /* set gender radio button state */
    for (int i = 0; i < ROLE_GENDERS; i++) {
        HWND button = data->controls[psc_male_button+i].hWnd;
        BOOL enable = ok_gend(flags.initrole, flags.initrace, i, flags.initalign);
        Button_Enable(button, enable);
        LRESULT state = Button_GetCheck(button);
        if (state == BST_CHECKED && flags.initgend != i)
            Button_SetCheck(button, BST_UNCHECKED);
        if (state == BST_UNCHECKED && flags.initgend == i)
            Button_SetCheck(button, BST_CHECKED);
    }

    /* set alignment radio button state */
    for (int i = 0; i < ROLE_ALIGNS; i++) {
        HWND button = data->controls[psc_lawful_button+i].hWnd;
        BOOL enable = ok_align(flags.initrole, flags.initrace, flags.initgend, i);
        Button_Enable(button, enable);
        LRESULT state = Button_GetCheck(button);
        if (state == BST_CHECKED && flags.initalign != i)
            Button_SetCheck(button, BST_UNCHECKED);
        if (state == BST_UNCHECKED && flags.initalign == i)
            Button_SetCheck(button, BST_CHECKED);
    }

}

/* player made up his mind - get final selection here */
int
plselFinalSelection(HWND hWnd)
{
    int role, race, gender, alignment;

    nhUse(role);
    nhUse(race);
    nhUse(gender);
    nhUse(alignment);
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

    return TRUE;
}

static boolean plselRandomize(plsel_data_t * data)
{
    int role, race, gender, alignment;
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

    role = flags.initrole;
    race = flags.initrace;
    gender = flags.initgend;
    alignment = flags.initalign;

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

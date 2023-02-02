/* NetHack 3.6	mhstatus.c	$NHDT-Date: 1432512798 2015/05/25 00:13:18 $  $NHDT-Branch: master $:$NHDT-Revision: 1.17 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "mhstatus.h"
#include "mhmsg.h"
#include "mhfont.h"
#include "mhcolor.h"

#define MAXWINDOWTEXT 255

#define NHSTAT_LINES_2 2
#define NHSTAT_LINES_4 4
typedef struct mswin_nethack_status_window {
    int nhstat_format;
    char window_text[MAXWINDOWTEXT];
} NHStatusWindow, *PNHStatusWindow;

static TCHAR szStatusWindowClass[] = TEXT("MSNHStatusWndClass");
LRESULT CALLBACK StatusWndProc(HWND, UINT, WPARAM, LPARAM);
static void register_status_window_class(void);
static void FormatStatusString(char *text, int format);

HWND
mswin_init_status_window()
{
    static int run_once = 0;
    HWND ret;
    NHStatusWindow *data;

    if (!run_once) {
        register_status_window_class();
        run_once = 1;
    }

    ret = CreateWindow(szStatusWindowClass, NULL,
                       WS_CHILD | WS_DISABLED | WS_CLIPSIBLINGS,
                       0, /* x position */
                       0, /* y position */
                       0, /* x-size - we will set it later */
                       0, /* y-size - we will set it later */
                       GetNHApp()->hMainWnd, NULL, GetNHApp()->hApp, NULL);
    if (!ret)
        panic("Cannot create status window");

    EnableWindow(ret, FALSE);

    data = (PNHStatusWindow) malloc(sizeof(NHStatusWindow));
    if (!data)
        panic("out of memory");

    ZeroMemory(data, sizeof(NHStatusWindow));
    data->nhstat_format = NHSTAT_LINES_4;
    SetWindowLong(ret, GWL_USERDATA, (LONG) data);
    return ret;
}

void
register_status_window_class()
{
    WNDCLASS wcex;

    ZeroMemory(&wcex, sizeof(wcex));
    wcex.style = CS_NOCLOSE;
    wcex.lpfnWndProc = (WNDPROC) StatusWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetNHApp()->hApp;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = mswin_get_brush(NHW_STATUS, MSWIN_COLOR_BG);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szStatusWindowClass;

    RegisterClass(&wcex);
}

LRESULT CALLBACK
StatusWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rt;
    PAINTSTRUCT ps;
    HDC hdc;
    PNHStatusWindow data;

    data = (PNHStatusWindow) GetWindowLong(hWnd, GWL_USERDATA);
    switch (message) {
    case WM_MSNH_COMMAND: {
        switch (wParam) {
        case MSNH_MSG_PUTSTR:
        case MSNH_MSG_CLEAR_WINDOW:
            ZeroMemory(data->window_text, sizeof(data->window_text));
            FormatStatusString(data->window_text, data->nhstat_format);
            break;

        case MSNH_MSG_CURSOR: {
            PMSNHMsgCursor msg_data = (PMSNHMsgCursor) lParam;
            if (msg_data->y == 0) {
                InvalidateRect(hWnd, NULL, TRUE);
            }
        } break;
        }
    } break;

    case WM_PAINT: {
        HGDIOBJ oldFont;
        TCHAR wbuf[MAXWINDOWTEXT];
        COLORREF OldBg, OldFg;

        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rt);

        oldFont = SelectObject(
            hdc, mswin_get_font(NHW_STATUS, ATR_NONE, hdc, FALSE));
        OldBg = SetBkColor(hdc, mswin_get_color(NHW_STATUS, MSWIN_COLOR_BG));
        OldFg =
            SetTextColor(hdc, mswin_get_color(NHW_STATUS, MSWIN_COLOR_FG));

        DrawText(hdc, NH_A2W(data->window_text, wbuf, MAXWINDOWTEXT),
                 strlen(data->window_text), &rt, DT_LEFT | DT_NOPREFIX);

        SetTextColor(hdc, OldFg);
        SetBkColor(hdc, OldBg);
        SelectObject(hdc, oldFont);
        EndPaint(hWnd, &ps);
    } break;

    case WM_DESTROY:
        free(data);
        SetWindowLong(hWnd, GWL_USERDATA, (LONG) 0);
        break;

    case WM_SETFOCUS:
        SetFocus(GetNHApp()->hMainWnd);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void
mswin_status_window_size(HWND hWnd, LPSIZE sz)
{
    TEXTMETRIC tm;
    HGDIOBJ saveFont;
    HDC hdc;
    PNHStatusWindow data;
    RECT rt;
    GetWindowRect(hWnd, &rt);
    sz->cx = rt.right - rt.left;
    sz->cy = rt.bottom - rt.top;

    data = (PNHStatusWindow) GetWindowLong(hWnd, GWL_USERDATA);
    if (data) {
        hdc = GetDC(hWnd);
        saveFont = SelectObject(
            hdc, mswin_get_font(NHW_STATUS, ATR_NONE, hdc, FALSE));
        GetTextMetrics(hdc, &tm);

        /* see if the status window can fit 80 characters per line */
        if ((80 * tm.tmMaxCharWidth) >= sz->cx)
            data->nhstat_format = NHSTAT_LINES_4;
        else
            data->nhstat_format = NHSTAT_LINES_2;

        /* set height of the status box */
        sz->cy = tm.tmHeight * data->nhstat_format;

        SelectObject(hdc, saveFont);
        ReleaseDC(hWnd, hdc);
    }
}
extern const char *hu_stat[];  /* defined in eat.c */
extern const char *enc_stat[]; /* define in botl.c */
void
FormatStatusString(char *text, int format)
{
    register char *nb;
    int hp, hpmax;
    int cap = near_capacity();

    Strcpy(text, gp.plname);
    if ('a' <= text[0] && text[0] <= 'z')
        text[0] += 'A' - 'a';
    text[10] = 0;
    Sprintf(nb = eos(text), " the ");

    if (Upolyd) {
        char mbot[BUFSZ];
        int k = 0;

        Strcpy(mbot, mons[u.umonnum].mname);
        while (mbot[k] != 0) {
            if ((k == 0 || (k > 0 && mbot[k - 1] == ' ')) && 'a' <= mbot[k]
                && mbot[k] <= 'z')
                mbot[k] += 'A' - 'a';
            k++;
        }
        Sprintf(nb = eos(nb), mbot);
    } else
        Sprintf(nb = eos(nb), rank_of(u.ulevel, Role_switch, flags.female));

    if (format == NHSTAT_LINES_4)
        Sprintf(nb = eos(nb), "\r\n");

    if (ACURR(A_STR) > 18) {
        if (ACURR(A_STR) > STR18(100))
            Sprintf(nb = eos(nb), "St:%2d ", ACURR(A_STR) - 100);
        else if (ACURR(A_STR) < STR18(100))
            Sprintf(nb = eos(nb), "St:18/%02d ", ACURR(A_STR) - 18);
        else
            Sprintf(nb = eos(nb), "St:18/** ");
    } else
        Sprintf(nb = eos(nb), "St:%-1d ", ACURR(A_STR));
    Sprintf(nb = eos(nb), "Dx:%-1d Co:%-1d In:%-1d Wi:%-1d Ch:%-1d",
            ACURR(A_DEX), ACURR(A_CON), ACURR(A_INT), ACURR(A_WIS),
            ACURR(A_CHA));
    Sprintf(nb = eos(nb),
            (u.ualign.type == A_CHAOTIC)
                ? "  Chaotic"
                : (u.ualign.type == A_NEUTRAL) ? "  Neutral" : "  Lawful");
#ifdef SCORE_ON_BOTL
    if (flags.showscore)
        Sprintf(nb = eos(nb), " S:%ld", botl_score());
#endif
    if (format == NHSTAT_LINES_4 || format == NHSTAT_LINES_2)
        strcat(text, "\r\n");

    /* third line */
    hp = Upolyd ? u.mh : u.uhp;
    hpmax = Upolyd ? u.mhmax : u.uhpmax;

    if (hp < 0)
        hp = 0;
    (void) describe_level(nb = eos(nb));
    Sprintf(nb = eos(nb), "%c:%-2ld HP:%d(%d) Pw:%d(%d) AC:%-2d",
            showsyms[COIN_CLASS + SYM_OFF_O], money_cnt(gi.invent), hp, hpmax,
            u.uen, u.uenmax, u.uac);

    if (Upolyd)
        Sprintf(nb = eos(nb), " HD:%d", mons[u.umonnum].mlevel);
    else if (flags.showexp)
        Sprintf(nb = eos(nb), " Xp:%u/%-1ld", u.ulevel, u.uexp);
    else
        Sprintf(nb = eos(nb), " Exp:%u", u.ulevel);
    if (format == NHSTAT_LINES_4)
        strcat(text, "\r\n");
    else
        strcat(text, " ");

    /* forth line */
    if (flags.time)
        Sprintf(nb = eos(nb), "T:%ld ", gm.moves);

    if (strcmp(hu_stat[u.uhs], "        ")) {
        Strcat(text, hu_stat[u.uhs]);
        Sprintf(nb = eos(nb), " ");
    }
    if (Confusion)
        Sprintf(nb = eos(nb), "Conf");
    if (Sick) {
        if (u.usick_type & SICK_VOMITABLE)
            Sprintf(nb = eos(nb), " FoodPois");
        if (u.usick_type & SICK_NONVOMITABLE)
            Sprintf(nb = eos(nb), " Ill");
    }
    if (Blind)
        Sprintf(nb = eos(nb), " Blind");
    if (Stunned)
        Sprintf(nb = eos(nb), " Stun");
    if (Hallucination)
        Sprintf(nb = eos(nb), " Hallu");
    if (Slimed)
        Sprintf(nb = eos(nb), " Slime");
    if (cap > UNENCUMBERED)
        Sprintf(nb = eos(nb), " %s", enc_stat[cap]);
}

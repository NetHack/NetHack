/* NetHack 3.6	mhmap.c	$NHDT-Date: 1435002695 2015/06/22 19:51:35 $  $NHDT-Branch: master $:$NHDT-Revision: 1.56 $ */
/* Copyright (C) 2001 by Alex Kompel      */
/* NetHack may be freely redistributed.  See license for details. */

#include "winMS.h"
#include "resource.h"
#include "mhmap.h"
#include "mhmsg.h"
#include "mhinput.h"
#include "mhfont.h"

#include "color.h"
#include "patchlevel.h"

#define NHMAP_FONT_NAME TEXT("Terminal")
#define MAXWINDOWTEXT 255

extern short glyph2tile[];

#define TILEBMP_X(ntile) ((ntile % GetNHApp()->mapTilesPerLine) * GetNHApp()->mapTile_X)
#define TILEBMP_Y(ntile) ((ntile / GetNHApp()->mapTilesPerLine) * GetNHApp()->mapTile_Y)

/* map window data */
typedef struct mswin_nethack_map_window {
    int map[COLNO][ROWNO]; /* glyph map */
    int bkmap[COLNO][ROWNO]; /* backround glyph map */

    int mapMode;              /* current map mode */
    boolean bAsciiMode;       /* switch ASCII/tiled mode */
    boolean bFitToScreenMode; /* switch Fit map to screen mode on/off */
    int xPos, yPos;           /* scroll position */
    int xPageSize, yPageSize; /* scroll page size */
    int xCur, yCur;           /* position of the cursor */
    int xScrTile, yScrTile;   /* size of display tile */
    POINT map_orig;           /* map origin point */

    HFONT hMapFont;           /* font for ASCII mode */
} NHMapWindow, *PNHMapWindow;

static TCHAR szNHMapWindowClass[] = TEXT("MSNethackMapWndClass");
LRESULT CALLBACK MapWndProc(HWND, UINT, WPARAM, LPARAM);
static void register_map_window_class(void);
static void onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onMSNH_VScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onMSNH_HScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void onPaint(HWND hWnd);
static void onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void nhcoord2display(PNHMapWindow data, int x, int y, LPRECT lpOut);
#if (VERSION_MAJOR < 4) && (VERSION_MINOR < 4) && (PATCHLEVEL < 2)
static void nhglyph2charcolor(short glyph, uchar *ch, int *color);
#endif

HWND
mswin_init_map_window()
{
    static int run_once = 0;
    HWND ret;
    RECT rt;

    if (!run_once) {
        register_map_window_class();
        run_once = 1;
    }

    /* get window position */
    if (GetNHApp()->bAutoLayout) {
        SetRect(&rt, 0, 0, 0, 0);
    } else {
        mswin_get_window_placement(NHW_MAP, &rt);
    }

    /* create map window object */
    ret = CreateWindow(
        szNHMapWindowClass, /* registered class name */
        NULL,               /* window name */
        WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_CLIPSIBLINGS
            | WS_SIZEBOX,     /* window style */
        rt.left,              /* horizontal position of window */
        rt.top,               /* vertical position of window */
        rt.right - rt.left,   /* window width */
        rt.bottom - rt.top,   /* window height */
        GetNHApp()->hMainWnd, /* handle to parent or owner window */
        NULL,                 /* menu handle or child identifier */
        GetNHApp()->hApp,     /* handle to application instance */
        NULL);                /* window-creation data */
    if (!ret) {
        panic("Cannot create map window");
    }

    /* Set window caption */
    SetWindowText(ret, "Map");

    mswin_apply_window_style(ret);

    return ret;
}

void
mswin_map_stretch(HWND hWnd, LPSIZE lpsz, BOOL redraw)
{
    PNHMapWindow data;
    RECT client_rt;
    SCROLLINFO si;
    SIZE wnd_size;
    LOGFONT lgfnt;

    /* check arguments */
    if (!IsWindow(hWnd) || !lpsz || lpsz->cx <= 0 || lpsz->cy <= 0)
        return;

    /* calculate window size */
    GetClientRect(hWnd, &client_rt);
    wnd_size.cx = client_rt.right - client_rt.left;
    wnd_size.cy = client_rt.bottom - client_rt.top;

    /* set new screen tile size */
    data = (PNHMapWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    data->xScrTile =
        max(1, (data->bFitToScreenMode ? wnd_size.cx : lpsz->cx) / COLNO);
    data->yScrTile =
        max(1, (data->bFitToScreenMode ? wnd_size.cy : lpsz->cy) / ROWNO);

    /* set map origin point */
    data->map_orig.x =
        max(0, client_rt.left + (wnd_size.cx - data->xScrTile * COLNO) / 2);
    data->map_orig.y =
        max(0, client_rt.top + (wnd_size.cy - data->yScrTile * ROWNO) / 2);

    data->map_orig.x -= data->map_orig.x % data->xScrTile;
    data->map_orig.y -= data->map_orig.y % data->yScrTile;

    /* adjust horizontal scroll bar */
    if (data->bFitToScreenMode)
        data->xPageSize = COLNO + 1; /* disable scroll bar */
    else
        data->xPageSize = wnd_size.cx / data->xScrTile;

    if (data->xPageSize >= COLNO) {
        data->xPos = 0;
        GetNHApp()->bNoHScroll = TRUE;
    } else {
        GetNHApp()->bNoHScroll = FALSE;
        data->xPos = max(
            0, min(COLNO - data->xPageSize + 1, u.ux - data->xPageSize / 2));
    }

    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = COLNO;
    si.nPage = data->xPageSize;
    si.nPos = data->xPos;
    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

    /* adjust vertical scroll bar */
    if (data->bFitToScreenMode)
        data->yPageSize = ROWNO + 1; /* disable scroll bar */
    else
        data->yPageSize = wnd_size.cy / data->yScrTile;

    if (data->yPageSize >= ROWNO) {
        data->yPos = 0;
        GetNHApp()->bNoVScroll = TRUE;
    } else {
        GetNHApp()->bNoVScroll = FALSE;
        data->yPos = max(
            0, min(ROWNO - data->yPageSize + 1, u.uy - data->yPageSize / 2));
    }

    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = ROWNO;
    si.nPage = data->yPageSize;
    si.nPos = data->yPos;
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

    /* create font */
    if (data->hMapFont)
        DeleteObject(data->hMapFont);
    ZeroMemory(&lgfnt, sizeof(lgfnt));
    lgfnt.lfHeight = -data->yScrTile;          // height of font
    lgfnt.lfWidth = -data->xScrTile;           // average character width
    lgfnt.lfEscapement = 0;                    // angle of escapement
    lgfnt.lfOrientation = 0;                   // base-line orientation angle
    lgfnt.lfWeight = FW_NORMAL;                // font weight
    lgfnt.lfItalic = FALSE;                    // italic attribute option
    lgfnt.lfUnderline = FALSE;                 // underline attribute option
    lgfnt.lfStrikeOut = FALSE;                 // strikeout attribute option
    lgfnt.lfCharSet = mswin_charset();         // character set identifier
    lgfnt.lfOutPrecision = OUT_DEFAULT_PRECIS; // output precision
    lgfnt.lfClipPrecision = CLIP_DEFAULT_PRECIS; // clipping precision
    lgfnt.lfQuality = DEFAULT_QUALITY;           // output quality
    if (iflags.wc_font_map && *iflags.wc_font_map) {
        lgfnt.lfPitchAndFamily = DEFAULT_PITCH; // pitch and family
        NH_A2W(iflags.wc_font_map, lgfnt.lfFaceName, LF_FACESIZE);
    } else {
        lgfnt.lfPitchAndFamily = FIXED_PITCH; // pitch and family
        NH_A2W(NHMAP_FONT_NAME, lgfnt.lfFaceName, LF_FACESIZE);
    }
    data->hMapFont = CreateFontIndirect(&lgfnt);

    mswin_cliparound(data->xCur, data->yCur);

    if (redraw)
        InvalidateRect(hWnd, NULL, TRUE);
}

/* set map mode */
int
mswin_map_mode(HWND hWnd, int mode)
{
    PNHMapWindow data;
    int oldMode;
    SIZE mapSize;

    data = (PNHMapWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (mode == data->mapMode)
        return mode;

    oldMode = data->mapMode;
    data->mapMode = mode;

    switch (data->mapMode) {
    case MAP_MODE_ASCII4x6:
        data->bAsciiMode = TRUE;
        data->bFitToScreenMode = FALSE;
        mapSize.cx = 4 * COLNO;
        mapSize.cy = 6 * ROWNO;
        break;

    case MAP_MODE_ASCII6x8:
        data->bAsciiMode = TRUE;
        data->bFitToScreenMode = FALSE;
        mapSize.cx = 6 * COLNO;
        mapSize.cy = 8 * ROWNO;
        break;

    case MAP_MODE_ASCII8x8:
        data->bAsciiMode = TRUE;
        data->bFitToScreenMode = FALSE;
        mapSize.cx = 8 * COLNO;
        mapSize.cy = 8 * ROWNO;
        break;

    case MAP_MODE_ASCII16x8:
        data->bAsciiMode = TRUE;
        data->bFitToScreenMode = FALSE;
        mapSize.cx = 16 * COLNO;
        mapSize.cy = 8 * ROWNO;
        break;

    case MAP_MODE_ASCII7x12:
        data->bAsciiMode = TRUE;
        data->bFitToScreenMode = FALSE;
        mapSize.cx = 7 * COLNO;
        mapSize.cy = 12 * ROWNO;
        break;

    case MAP_MODE_ASCII8x12:
        data->bAsciiMode = TRUE;
        data->bFitToScreenMode = FALSE;
        mapSize.cx = 8 * COLNO;
        mapSize.cy = 12 * ROWNO;
        break;

    case MAP_MODE_ASCII16x12:
        data->bAsciiMode = TRUE;
        data->bFitToScreenMode = FALSE;
        mapSize.cx = 16 * COLNO;
        mapSize.cy = 12 * ROWNO;
        break;

    case MAP_MODE_ASCII12x16:
        data->bAsciiMode = TRUE;
        data->bFitToScreenMode = FALSE;
        mapSize.cx = 12 * COLNO;
        mapSize.cy = 16 * ROWNO;
        break;

    case MAP_MODE_ASCII10x18:
        data->bAsciiMode = TRUE;
        data->bFitToScreenMode = FALSE;
        mapSize.cx = 10 * COLNO;
        mapSize.cy = 18 * ROWNO;
        break;

    case MAP_MODE_ASCII_FIT_TO_SCREEN: {
        RECT client_rt;
        GetClientRect(hWnd, &client_rt);
        mapSize.cx = client_rt.right - client_rt.left;
        mapSize.cy = client_rt.bottom - client_rt.top;

        data->bAsciiMode = TRUE;
        data->bFitToScreenMode = TRUE;
    } break;

    case MAP_MODE_TILES_FIT_TO_SCREEN: {
        RECT client_rt;
        GetClientRect(hWnd, &client_rt);
        mapSize.cx = client_rt.right - client_rt.left;
        mapSize.cy = client_rt.bottom - client_rt.top;

        data->bAsciiMode = FALSE;
        data->bFitToScreenMode = TRUE;
    } break;

    case MAP_MODE_TILES:
    default:
        data->bAsciiMode = FALSE;
        data->bFitToScreenMode = FALSE;
        mapSize.cx = GetNHApp()->mapTile_X * COLNO;
        mapSize.cy = GetNHApp()->mapTile_Y * ROWNO;
        break;
    }

    mswin_map_stretch(hWnd, &mapSize, TRUE);

    mswin_update_inventory(); /* for perm_invent to hide/show tiles */

    return oldMode;
}

/* register window class for map window */
void
register_map_window_class()
{
    WNDCLASS wcex;
    ZeroMemory(&wcex, sizeof(wcex));

    /* window class */
    wcex.style = CS_NOCLOSE | CS_DBLCLKS;
    wcex.lpfnWndProc = (WNDPROC) MapWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetNHApp()->hApp;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground =
        CreateSolidBrush(RGB(0, 0, 0)); /* set backgroup here */
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szNHMapWindowClass;

    if (!RegisterClass(&wcex)) {
        panic("cannot register Map window class");
    }
}

/* map window procedure */
LRESULT CALLBACK
MapWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PNHMapWindow data;

    data = (PNHMapWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (message) {
    case WM_CREATE:
        onCreate(hWnd, wParam, lParam);
        break;

    case WM_MSNH_COMMAND:
        onMSNHCommand(hWnd, wParam, lParam);
        break;

    case WM_PAINT:
        onPaint(hWnd);
        break;

    case WM_SETFOCUS:
        /* transfer focus back to the main window */
        SetFocus(GetNHApp()->hMainWnd);
        break;

    case WM_HSCROLL:
        onMSNH_HScroll(hWnd, wParam, lParam);
        break;

    case WM_VSCROLL:
        onMSNH_VScroll(hWnd, wParam, lParam);
        break;

    case WM_SIZE: {
        RECT rt;
        SIZE size;

        if (data->bFitToScreenMode) {
            size.cx = LOWORD(lParam);
            size.cy = HIWORD(lParam);
        } else {
            /* mapping factor is unchaged we just need to adjust scroll bars
             */
            size.cx = data->xScrTile * COLNO;
            size.cy = data->yScrTile * ROWNO;
        }
        mswin_map_stretch(hWnd, &size, TRUE);

        /* update window placement */
        GetWindowRect(hWnd, &rt);
        ScreenToClient(GetNHApp()->hMainWnd, (LPPOINT) &rt);
        ScreenToClient(GetNHApp()->hMainWnd, ((LPPOINT) &rt) + 1);
        mswin_update_window_placement(NHW_MAP, &rt);
    } break;

    case WM_MOVE: {
        RECT rt;
        GetWindowRect(hWnd, &rt);
        ScreenToClient(GetNHApp()->hMainWnd, (LPPOINT) &rt);
        ScreenToClient(GetNHApp()->hMainWnd, ((LPPOINT) &rt) + 1);
        mswin_update_window_placement(NHW_MAP, &rt);
    }
        return FALSE;

    case WM_LBUTTONDOWN:
        NHEVENT_MS(CLICK_1,
                   max(0, min(COLNO, data->xPos
                                         + (LOWORD(lParam) - data->map_orig.x)
                                               / data->xScrTile)),
                   max(0, min(ROWNO, data->yPos
                                         + (HIWORD(lParam) - data->map_orig.y)
                                               / data->yScrTile)));
        return 0;

    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
        NHEVENT_MS(CLICK_2,
                   max(0, min(COLNO, data->xPos
                                         + (LOWORD(lParam) - data->map_orig.x)
                                               / data->xScrTile)),
                   max(0, min(ROWNO, data->yPos
                                         + (HIWORD(lParam) - data->map_orig.y)
                                               / data->yScrTile)));
        return 0;

    case WM_DESTROY:
        if (data->hMapFont)
            DeleteObject(data->hMapFont);
        free(data);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) 0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

/* on WM_COMMAND */
void
onMSNHCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHMapWindow data;
    RECT rt;

    data = (PNHMapWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (wParam) {
    case MSNH_MSG_PRINT_GLYPH: {
        PMSNHMsgPrintGlyph msg_data = (PMSNHMsgPrintGlyph) lParam;
        if ((data->map[msg_data->x][msg_data->y] != msg_data->glyph)
            || (data->bkmap[msg_data->x][msg_data->y] != msg_data->bkglyph)) {
            data->map[msg_data->x][msg_data->y] = msg_data->glyph;
            data->bkmap[msg_data->x][msg_data->y] = msg_data->bkglyph;

            /* invalidate the update area. Erase backround if there 
               is nothing to paint or we are in text mode */
            nhcoord2display(data, msg_data->x, msg_data->y, &rt);
            InvalidateRect(hWnd, &rt, 
                           (((msg_data->glyph == NO_GLYPH) && (msg_data->bkglyph == NO_GLYPH))
                            || data->bAsciiMode 
                            || Is_rogue_level(&u.uz)));
        }
    } break;

    case MSNH_MSG_CLIPAROUND: {
        PMSNHMsgClipAround msg_data = (PMSNHMsgClipAround) lParam;
        int x, y;
        BOOL scroll_x, scroll_y;
        int mcam = iflags.wc_scroll_margin;

        /* calculate if you should clip around */
        scroll_x =
            !GetNHApp()->bNoHScroll
            && (msg_data->x < (data->xPos + mcam)
                || msg_data->x > (data->xPos + data->xPageSize - mcam));
        scroll_y =
            !GetNHApp()->bNoVScroll
            && (msg_data->y < (data->yPos + mcam)
                || msg_data->y > (data->yPos + data->yPageSize - mcam));

        mcam += iflags.wc_scroll_amount - 1;
        /* get page size and center horizontally on x-position */
        if (scroll_x) {
            if (data->xPageSize <= 2 * mcam) {
                x = max(0, min(COLNO, msg_data->x - data->xPageSize / 2));
            } else if (msg_data->x < data->xPos + data->xPageSize / 2) {
                x = max(0, min(COLNO, msg_data->x - mcam));
            } else {
                x = max(0, min(COLNO, msg_data->x - data->xPageSize + mcam));
            }
            SendMessage(hWnd, WM_HSCROLL, (WPARAM) MAKELONG(SB_THUMBTRACK, x),
                        (LPARAM) NULL);
        }

        /* get page size and center vertically on y-position */
        if (scroll_y) {
            if (data->yPageSize <= 2 * mcam) {
                y = max(0, min(ROWNO, msg_data->y - data->yPageSize / 2));
            } else if (msg_data->y < data->yPos + data->yPageSize / 2) {
                y = max(0, min(ROWNO, msg_data->y - mcam));
            } else {
                y = max(0, min(ROWNO, msg_data->y - data->yPageSize + mcam));
            }
            SendMessage(hWnd, WM_VSCROLL, (WPARAM) MAKELONG(SB_THUMBTRACK, y),
                        (LPARAM) NULL);
        }
    } break;

    case MSNH_MSG_CLEAR_WINDOW: {
        int i, j;
        for (i = 0; i < COLNO; i++)
            for (j = 0; j < ROWNO; j++) {
                data->map[i][j] = NO_GLYPH;
                data->bkmap[i][j] = NO_GLYPH;
            }
        InvalidateRect(hWnd, NULL, TRUE);
    } break;

    case MSNH_MSG_CURSOR: {
        PMSNHMsgCursor msg_data = (PMSNHMsgCursor) lParam;
        HDC hdc;
        RECT rt;

        /* move focus rectangle at the cursor postion */
        hdc = GetDC(hWnd);

        nhcoord2display(data, data->xCur, data->yCur, &rt);
        if (data->bAsciiMode) {
            PatBlt(hdc, rt.left, rt.top, rt.right - rt.left,
                   rt.bottom - rt.top, DSTINVERT);
        } else {
            DrawFocusRect(hdc, &rt);
        }

        data->xCur = msg_data->x;
        data->yCur = msg_data->y;

        nhcoord2display(data, data->xCur, data->yCur, &rt);
        if (data->bAsciiMode) {
            PatBlt(hdc, rt.left, rt.top, rt.right - rt.left,
                   rt.bottom - rt.top, DSTINVERT);
        } else {
            DrawFocusRect(hdc, &rt);
        }

        ReleaseDC(hWnd, hdc);
    } break;

    case MSNH_MSG_GETTEXT: {
        PMSNHMsgGetText msg_data = (PMSNHMsgGetText) lParam;
        size_t index;
        int col, row;
        int color;
        unsigned special;
        int mgch;

        index = 0;
        for (row = 0; row < ROWNO; row++) {
            for (col = 0; col < COLNO; col++) {
                if (index >= msg_data->max_size)
                    break;
                if (data->map[col][row] == NO_GLYPH) {
                    mgch = ' ';
                } else {
                    (void) mapglyph(data->map[col][row], &mgch, &color,
                                    &special, col, row);
                }
                msg_data->buffer[index] = mgch;
                index++;
            }
            if (index >= msg_data->max_size - 1)
                break;
            msg_data->buffer[index++] = '\r';
            msg_data->buffer[index++] = '\n';
        }
    } break;

    } /* end switch(wParam) */
}

/* on WM_CREATE */
void
onCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHMapWindow data;
    int i, j;

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    /* set window data */
    data = (PNHMapWindow) malloc(sizeof(NHMapWindow));
    if (!data)
        panic("out of memory");

    ZeroMemory(data, sizeof(NHMapWindow));
    for (i = 0; i < COLNO; i++)
        for (j = 0; j < ROWNO; j++) {
            data->map[i][j] = NO_GLYPH;
            data->bkmap[i][j] = NO_GLYPH;
        }

    data->bAsciiMode = FALSE;

    data->xScrTile = GetNHApp()->mapTile_X;
    data->yScrTile = GetNHApp()->mapTile_Y;

    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) data);
}

/* on WM_PAINT */
void
onPaint(HWND hWnd)
{
    PNHMapWindow data;
    PAINTSTRUCT ps;
    HDC hDC;
    HDC tileDC;
    HGDIOBJ saveBmp;
    RECT paint_rt;
    int i, j;

    /* get window data */
    data = (PNHMapWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    hDC = BeginPaint(hWnd, &ps);

    /* calculate paint rectangle */
    if (!IsRectEmpty(&ps.rcPaint)) {
        /* calculate paint rectangle */
        paint_rt.left =
            max(data->xPos
                    + (ps.rcPaint.left - data->map_orig.x) / data->xScrTile,
                0);
        paint_rt.top = max(
            data->yPos + (ps.rcPaint.top - data->map_orig.y) / data->yScrTile,
            0);
        paint_rt.right = min(
            data->xPos
                + (ps.rcPaint.right - data->map_orig.x) / data->xScrTile + 1,
            COLNO);
        paint_rt.bottom = min(
            data->yPos
                + (ps.rcPaint.bottom - data->map_orig.y) / data->yScrTile + 1,
            ROWNO);

        if (data->bAsciiMode || Is_rogue_level(&u.uz)) {
            /* You enter a VERY primitive world! */
            HGDIOBJ oldFont;

            oldFont = SelectObject(hDC, data->hMapFont);
            SetBkMode(hDC, TRANSPARENT);

            /* draw the map */
            for (i = paint_rt.left; i < paint_rt.right; i++)
                for (j = paint_rt.top; j < paint_rt.bottom; j++)
                    if (data->map[i][j] >= 0) {
                        char ch;
                        TCHAR wch;
                        RECT glyph_rect;
                        int color;
                        unsigned special;
                        int mgch;
                        HBRUSH back_brush;
                        COLORREF OldFg;

                        nhcoord2display(data, i, j, &glyph_rect);

#if (VERSION_MAJOR < 4) && (VERSION_MINOR < 4) && (PATCHLEVEL < 2)
                        nhglyph2charcolor(data->map[i][j], &ch, &color);
                        OldFg = SetTextColor(hDC, nhcolor_to_RGB(color));
#else
                        /* rely on NetHack core helper routine */
                        (void) mapglyph(data->map[i][j], &mgch, &color,
                                        &special, i, j);
                        ch = (char) mgch;
                        if (((special & MG_PET) && iflags.hilite_pet)
                            || ((special & (MG_DETECT | MG_BW_LAVA))
                                && iflags.use_inverse)) {
                            back_brush =
                                CreateSolidBrush(nhcolor_to_RGB(CLR_GRAY));
                            FillRect(hDC, &glyph_rect, back_brush);
                            DeleteObject(back_brush);
                            switch (color) {
                            case CLR_GRAY:
                            case CLR_WHITE:
                                OldFg = SetTextColor(
                                    hDC, nhcolor_to_RGB(CLR_BLACK));
                                break;
                            default:
                                OldFg =
                                    SetTextColor(hDC, nhcolor_to_RGB(color));
                            }
                        } else {
                            OldFg = SetTextColor(hDC, nhcolor_to_RGB(color));
                        }
#endif

                        DrawText(hDC, NH_A2W(&ch, &wch, 1), 1, &glyph_rect,
                                 DT_CENTER | DT_VCENTER | DT_NOPREFIX);
                        SetTextColor(hDC, OldFg);
                    }
            SelectObject(hDC, oldFont);
        } else {
            short ntile;
            int t_x, t_y;
            int glyph, bkglyph;
            RECT glyph_rect;
            int layer;
#ifdef USE_PILEMARK
            int color;
            unsigned special;
            int mgch;
#endif
            /* prepare tiles DC for mapping */
            tileDC = CreateCompatibleDC(hDC);
            saveBmp = SelectObject(tileDC, GetNHApp()->bmpMapTiles);

            /* draw the map */
            for (i = paint_rt.left; i < paint_rt.right; i++)
                for (j = paint_rt.top; j < paint_rt.bottom; j++) {
                    layer = 0;
                    glyph = data->map[i][j];
                    bkglyph = data->bkmap[i][j];

                    if (bkglyph != NO_GLYPH) {
                        ntile = glyph2tile[bkglyph];
                        t_x = TILEBMP_X(ntile);
                        t_y = TILEBMP_Y(ntile);
                        nhcoord2display(data, i, j, &glyph_rect);

                        StretchBlt(hDC, glyph_rect.left, glyph_rect.top,
                                   data->xScrTile, data->yScrTile, tileDC,
                                   t_x, t_y, GetNHApp()->mapTile_X,
                                   GetNHApp()->mapTile_Y, SRCCOPY);
                        layer ++;
                    }

                    if ((glyph != NO_GLYPH) && (glyph != bkglyph)) {
                        ntile = glyph2tile[glyph];
                        t_x = TILEBMP_X(ntile);
                        t_y = TILEBMP_Y(ntile);
                        nhcoord2display(data, i, j, &glyph_rect);

                        if (layer > 0) {
                            (*GetNHApp()->lpfnTransparentBlt)(
                                hDC, glyph_rect.left, glyph_rect.top,
                                data->xScrTile, data->yScrTile, tileDC,
                                t_x, t_y, GetNHApp()->mapTile_X,
                                GetNHApp()->mapTile_Y, TILE_BK_COLOR);
                        } else {
                            StretchBlt(hDC, glyph_rect.left, glyph_rect.top,
                                       data->xScrTile, data->yScrTile, tileDC,
                                       t_x, t_y, GetNHApp()->mapTile_X,
                                       GetNHApp()->mapTile_Y, SRCCOPY);
                        }

                        layer ++;
                     }

#ifdef USE_PILEMARK
                     /* rely on NetHack core helper routine */
                     (void) mapglyph(data->map[i][j], &mgch, &color,
                                        &special, i, j);
                     if ((glyph != NO_GLYPH) && (special & MG_PET) 
#else
                     if ((glyph != NO_GLYPH) && glyph_is_pet(glyph)
#endif
                           && iflags.wc_hilite_pet) {
                        /* apply pet mark transparently over
                           pet image */
                        HDC hdcPetMark;
                        HBITMAP bmPetMarkOld;

                        /* this is DC for petmark bitmap */
                        hdcPetMark = CreateCompatibleDC(hDC);
                        bmPetMarkOld = SelectObject(
                            hdcPetMark, GetNHApp()->bmpPetMark);

                        (*GetNHApp()->lpfnTransparentBlt)(
                            hDC, glyph_rect.left, glyph_rect.top,
                            data->xScrTile, data->yScrTile, hdcPetMark, 0,
                            0, TILE_X, TILE_Y, TILE_BK_COLOR);
                        SelectObject(hdcPetMark, bmPetMarkOld);
                        DeleteDC(hdcPetMark);
                    }
#ifdef USE_PILEMARK
                    if ((glyph != NO_GLYPH)
                        && (special & MG_OBJPILE) && iflags.hilite_pile) {
                        /* apply pilemark transparently over other image */
                        HDC hdcPileMark;
                        HBITMAP bmPileMarkOld;

                        /* this is DC for pilemark bitmap */
                        hdcPileMark = CreateCompatibleDC(hDC);
                        bmPileMarkOld = SelectObject(
                            hdcPileMark, GetNHApp()->bmpPileMark);

                        (*GetNHApp()->lpfnTransparentBlt)(
                            hDC, glyph_rect.left, glyph_rect.top,
                            data->xScrTile, data->yScrTile, hdcPileMark, 0,
                            0, TILE_X, TILE_Y, TILE_BK_COLOR);
                        SelectObject(hdcPileMark, bmPileMarkOld);
                        DeleteDC(hdcPileMark);                        
		    }
#endif
                }

            SelectObject(tileDC, saveBmp);
            DeleteDC(tileDC);
        }

        /* draw focus rect */
        nhcoord2display(data, data->xCur, data->yCur, &paint_rt);
        if (data->bAsciiMode) {
            PatBlt(hDC, paint_rt.left, paint_rt.top,
                   paint_rt.right - paint_rt.left,
                   paint_rt.bottom - paint_rt.top, DSTINVERT);
        } else {
            DrawFocusRect(hDC, &paint_rt);
        }
    }
    EndPaint(hWnd, &ps);
}

/* on WM_VSCROLL */
void
onMSNH_VScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHMapWindow data;
    SCROLLINFO si;
    int yNewPos;
    int yDelta;

    UNREFERENCED_PARAMETER(lParam);

    /* get window data */
    data = (PNHMapWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (LOWORD(wParam)) {
    /* User clicked shaft left of the scroll box. */
    case SB_PAGEUP:
        yNewPos = data->yPos - data->yPageSize;
        break;

    /* User clicked shaft right of the scroll box. */
    case SB_PAGEDOWN:
        yNewPos = data->yPos + data->yPageSize;
        break;

    /* User clicked the left arrow. */
    case SB_LINEUP:
        yNewPos = data->yPos - 1;
        break;

    /* User clicked the right arrow. */
    case SB_LINEDOWN:
        yNewPos = data->yPos + 1;
        break;

    /* User dragged the scroll box. */
    case SB_THUMBTRACK:
        yNewPos = HIWORD(wParam);
        break;

    default:
        yNewPos = data->yPos;
    }

    yNewPos = max(0, min(ROWNO - data->yPageSize + 1, yNewPos));
    if (yNewPos == data->yPos)
        return;

    yDelta = yNewPos - data->yPos;
    data->yPos = yNewPos;

    ScrollWindowEx(hWnd, 0, -data->yScrTile * yDelta, (CONST RECT *) NULL,
                   (CONST RECT *) NULL, (HRGN) NULL, (LPRECT) NULL,
                   SW_INVALIDATE | SW_ERASE);

    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    si.nPos = data->yPos;
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
}

/* on WM_HSCROLL */
void
onMSNH_HScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PNHMapWindow data;
    SCROLLINFO si;
    int xNewPos;
    int xDelta;

    UNREFERENCED_PARAMETER(lParam);

    /* get window data */
    data = (PNHMapWindow) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (LOWORD(wParam)) {
    /* User clicked shaft left of the scroll box. */
    case SB_PAGEUP:
        xNewPos = data->xPos - data->xPageSize;
        break;

    /* User clicked shaft right of the scroll box. */
    case SB_PAGEDOWN:
        xNewPos = data->xPos + data->xPageSize;
        break;

    /* User clicked the left arrow. */
    case SB_LINEUP:
        xNewPos = data->xPos - 1;
        break;

    /* User clicked the right arrow. */
    case SB_LINEDOWN:
        xNewPos = data->xPos + 1;
        break;

    /* User dragged the scroll box. */
    case SB_THUMBTRACK:
        xNewPos = HIWORD(wParam);
        break;

    default:
        xNewPos = data->xPos;
    }

    xNewPos = max(0, min(COLNO - data->xPageSize + 1, xNewPos));
    if (xNewPos == data->xPos)
        return;

    xDelta = xNewPos - data->xPos;
    data->xPos = xNewPos;

    ScrollWindowEx(hWnd, -data->xScrTile * xDelta, 0, (CONST RECT *) NULL,
                   (CONST RECT *) NULL, (HRGN) NULL, (LPRECT) NULL,
                   SW_INVALIDATE | SW_ERASE);

    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    si.nPos = data->xPos;
    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
}

/* map nethack map coordinates to the screen location */
void
nhcoord2display(PNHMapWindow data, int x, int y, LPRECT lpOut)
{
    lpOut->left = (x - data->xPos) * data->xScrTile + data->map_orig.x;
    lpOut->top = (y - data->yPos) * data->yScrTile + data->map_orig.y;
    lpOut->right = lpOut->left + data->xScrTile;
    lpOut->bottom = lpOut->top + data->yScrTile;
}

#if (VERSION_MAJOR < 4) && (VERSION_MINOR < 4) && (PATCHLEVEL < 2)
/* map glyph to character/color combination */
void
nhglyph2charcolor(short g, uchar *ch, int *color)
{
    int offset;
#ifdef TEXTCOLOR

#define zap_color(n) *color = iflags.use_color ? zapcolors[n] : NO_COLOR
#define cmap_color(n) *color = iflags.use_color ? defsyms[n].color : NO_COLOR
#define obj_color(n) \
    *color = iflags.use_color ? objects[n].oc_color : NO_COLOR
#define mon_color(n) *color = iflags.use_color ? mons[n].mcolor : NO_COLOR
#define pet_color(n) *color = iflags.use_color ? mons[n].mcolor : NO_COLOR
#define warn_color(n) \
    *color = iflags.use_color ? def_warnsyms[n].color : NO_COLOR

#else /* no text color */

#define zap_color(n)
#define cmap_color(n)
#define obj_color(n)
#define mon_color(n)
#define pet_color(c)
#define warn_color(c)
    *color = CLR_WHITE;
#endif

    if ((offset = (g - GLYPH_WARNING_OFF)) >= 0) { /* a warning flash */
        *ch = showsyms[offset + SYM_OFF_W];
        warn_color(offset);
    } else if ((offset = (g - GLYPH_SWALLOW_OFF)) >= 0) { /* swallow */
        /* see swallow_to_glyph() in display.c */
        *ch = (uchar) showsyms[(S_sw_tl + (offset & 0x7)) + SYM_OFF_P];
        mon_color(offset >> 3);
    } else if ((offset = (g - GLYPH_ZAP_OFF)) >= 0) { /* zap beam */
        /* see zapdir_to_glyph() in display.c */
        *ch = showsyms[(S_vbeam + (offset & 0x3)) + SYM_OFF_P];
        zap_color((offset >> 2));
    } else if ((offset = (g - GLYPH_CMAP_OFF)) >= 0) { /* cmap */
        *ch = showsyms[offset + SYM_OFF_P];
        cmap_color(offset);
    } else if ((offset = (g - GLYPH_OBJ_OFF)) >= 0) { /* object */
        *ch = showsyms[(int) objects[offset].oc_class + SYM_OFF_O];
        obj_color(offset);
    } else if ((offset = (g - GLYPH_BODY_OFF)) >= 0) { /* a corpse */
        *ch = showsyms[(int) objects[CORPSE].oc_class + SYM_OFF_O];
        mon_color(offset);
    } else if ((offset = (g - GLYPH_PET_OFF)) >= 0) { /* a pet */
        *ch = showsyms[(int) mons[offset].mlet + SYM_OFF_M];
        pet_color(offset);
    } else { /* a monster */
        *ch = showsyms[(int) mons[g].mlet + SYM_OFF_M];
        mon_color(g);
    }
    // end of wintty code
}
#endif

/* map nethack color to RGB */
COLORREF
nhcolor_to_RGB(int c)
{
    if (c >= 0 && c < CLR_MAX)
        return GetNHApp()->regMapColors[c];
    return RGB(0x00, 0x00, 0x00);
}


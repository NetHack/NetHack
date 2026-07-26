/* NetHack 5.0	macprefs.c	*/
/* NetHack may be freely redistributed.  See license for details. */

/* Preferences dialog: UI settings (map mode, hitpointbar, statuslines,
 * per-window fonts/sizes) persisted in the "NetHack Preferences" file
 * alongside the window positions (maccurs.c).  Applied at startup after
 * initoptions(), so a saved record overrides the NetHack Defaults file;
 * "Forget Settings" clears the record and the config file rules again.
 * All changes take effect at the next launch (one game per launch).
 */

#include "hack.h"
#include "macwin.h"

#include <Dialogs.h>
#include <Menus.h>
#include <TextUtils.h>
#include <ToolUtils.h>

#define PREFS_DLOG 6100 /* DLOG/DITL in nhmenus.r */
#define PREFS_FONT_MENU 6100

/* DITL items; the font popups and size fields are contiguous runs in
   uiprefs_fonts order (map, status, message, menu, text) */
enum {
    prefSave = 1,
    prefCancel,
    prefForget,
    prefTiles,
    prefHPbar,
    prefLines2,
    prefLines3,
    prefFontFirst, /* 8..12: font popup user items */
    prefFontLast = prefFontFirst + UIPREFS_NFONTS - 1,
    prefSizeFirst, /* 13..17: size edit fields */
    prefSizeLast = prefSizeFirst + UIPREFS_NFONTS - 1,
    /* 18..22 labels, 23 note */
    prefMenuTiles = 24,
    prefDefaultRing = 25, /* bold outline around the Save button */
    prefSeparator = 26    /* gray rule above the button row */
};

#define PREFS_SIZE_MENU 6101 /* transient size popup, built per click */

static MenuHandle fontMenu; /* "(default)" + AppendResMenu('FONT') */
static short fontSel[UIPREFS_NFONTS]; /* 1-based menu item; 1 = default */
static short sizeVal[UIPREFS_NFONTS]; /* point size; 0 = default */

static const short std_sizes[] = { 9, 10, 12, 14, 18, 24, 36, 48 };
#define NUM_STD_SIZES (short) (sizeof std_sizes / sizeof std_sizes[0])

/* window kind each font row configures (win_fonts[] index) */
static const short uifont_nhw[UIPREFS_NFONTS] = {
    NHW_MAP, NHW_STATUS, NHW_MESSAGE, NHW_MENU, NHW_TEXT
};

extern short win_fonts[NHW_TEXT + 1]; /* macwin.c */

static void
set_check(DialogRef dlog, short item, Boolean on)
{
    short type;
    Handle h;
    Rect r;

    GetDialogItem(dlog, item, &type, &h, &r);
    SetControlValue((ControlHandle) h, on ? 1 : 0);
}

static Boolean
get_check(DialogRef dlog, short item)
{
    short type;
    Handle h;
    Rect r;

    GetDialogItem(dlog, item, &type, &h, &r);
    return GetControlValue((ControlHandle) h) != 0;
}

/* menu item matching a stored font name; 1 = "(default)" on no match */
static short
menu_item_for_font(const unsigned char *name)
{
    short n, i;
    Str255 s;

    if (!name[0])
        return 1;
    n = CountMenuItems(fontMenu);
    for (i = 2; i <= n; i++) {
        GetMenuItemText(fontMenu, i, s);
        if (EqualString(s, name, false, true))
            return i;
    }
    return 1;
}

/* point size a row currently resolves to: config value, else the live
   window's font size, else the port fallback -- used to seed the size
   popups so they show the real size instead of "(default)" */
static short
effective_size(short row)
{
    short sz = 0;
    winid w = WIN_ERR;

    switch (row) {
    case uiFontMap:
        sz = iflags.wc_fontsiz_map;
        w = WIN_MAP;
        break;
    case uiFontStatus:
        sz = iflags.wc_fontsiz_status;
        w = WIN_STATUS;
        break;
    case uiFontMessage:
        sz = iflags.wc_fontsiz_message;
        w = WIN_MESSAGE;
        break;
    case uiFontMenu:
        sz = iflags.wc_fontsiz_menu;
        break;
    case uiFontText:
        sz = iflags.wc_fontsiz_text;
        break;
    }
    if (!sz && w != WIN_ERR && w >= 0 && w < NUM_MACWINDOWS
        && theWindows && theWindows[w].its_window)
        sz = theWindows[w].font_size;
    if (!sz) /* cre_win's fallbacks: 9pt menus/text, 12pt elsewhere */
        sz = (row == uiFontMenu || row == uiFontText) ? 9 : 12;
    return sz;
}

/* font number a row currently resolves to (explicit choice, else the
   port default in win_fonts[]); drives RealFont in the size popup */
static short
row_font_number(short row)
{
    Str255 s;
    short fnum = 0;

    if (fontSel[row] > 1) {
        GetMenuItemText(fontMenu, fontSel[row], s);
        GetFNum(s, &fnum);
    }
    if (!fnum)
        fnum = win_fonts[uifont_nhw[row]];
    return fnum;
}

/* user items: font/size popups (framed box, drop shadow, down arrow,
   selection text), the default-button ring, and the separator rule */
static pascal void
pref_redraw(DialogRef dlog, DialogItemIndex item)
{
    short type, j;
    Handle h;
    Rect r;
    Str255 s;

    if (item == prefDefaultRing) {
        GetDialogItem(dlog, item, &type, &h, &r);
        PenSize(3, 3);
        FrameRoundRect(&r, 16, 16);
        PenSize(1, 1);
        return;
    }
    if (item == prefSeparator) {
        GetDialogItem(dlog, item, &type, &h, &r);
        PenPat(&qd.gray);
        MoveTo(r.left, r.top);
        LineTo(r.right - 1, r.top);
        PenNormal();
        return;
    }
    if (item >= prefFontFirst && item <= prefFontLast) {
        GetMenuItemText(fontMenu, fontSel[item - prefFontFirst], s);
    } else if (item >= prefSizeFirst && item <= prefSizeLast) {
        short val = sizeVal[item - prefSizeFirst];

        if (val > 0)
            NumToString((long) val, s);
        else
            BlockMove(P_STRING_CONV("(default)"), s, 10);
    } else
        return;
    GetDialogItem(dlog, item, &type, &h, &r);
    EraseRect(&r);
    FrameRect(&r);
    MoveTo(r.left + 3, r.bottom);
    LineTo(r.right, r.bottom);
    LineTo(r.right, r.top + 3);
    MoveTo(r.left + 6, r.bottom - 6);
    DrawString(s);
    /* System 7 popup cue: down arrow at the right edge */
    for (j = 0; j < 5; j++) {
        MoveTo((short) (r.right - 15 + j),
               (short) ((r.top + r.bottom) / 2 - 3 + j));
        LineTo((short) (r.right - 7 - j),
               (short) ((r.top + r.bottom) / 2 - 3 + j));
    }
}

/* size popup: "(default)" + the standard sizes, bitmap-available sizes
   for the row's font in outline style (the classic Size-menu cue) */
static short
pick_size(short row, Rect *r)
{
    MenuHandle m;
    Str255 s;
    Point pt;
    long sel;
    short i, fnum, curitem = 1, val = sizeVal[row];

    m = NewMenu(PREFS_SIZE_MENU, P_STRING_CONV("Sizes"));
    if (!m)
        return val;
    /* AppendMenu treats '(' as a disable metacharacter;
       SetMenuItemText stores the text verbatim */
    AppendMenu(m, P_STRING_CONV("x"));
    SetMenuItemText(m, 1, P_STRING_CONV("(default)"));
    fnum = row_font_number(row);
    for (i = 0; i < NUM_STD_SIZES; i++) {
        NumToString((long) std_sizes[i], s);
        AppendMenu(m, s);
        if (RealFont(fnum, std_sizes[i]))
            SetItemStyle(m, i + 2, outline);
        if (std_sizes[i] == val)
            curitem = i + 2;
    }
    InsertMenu(m, hierMenu);
    pt.v = r->top;
    pt.h = r->left;
    LocalToGlobal(&pt);
    CheckMenuItem(m, curitem, true);
    sel = PopUpMenuSelect(m, pt.v, pt.h, curitem);
    DeleteMenu(PREFS_SIZE_MENU);
    DisposeMenu(m);
    if (sel) {
        i = LoWord(sel);
        val = (i <= 1) ? 0 : std_sizes[i - 2];
    }
    return val;
}

static pascal Boolean
pref_filter(DialogRef wind, EventRecord *event, DialogItemIndex *item)
{
    char ch;

    /* movable modal: ModalDialog only redraws its own window; route
       update events for the game windows behind us to the app handler
       or dragging the dialog leaves white holes over them */
    if (event->what == updateEvt
        && (WindowPtr) event->message != GetDialogWindow(wind)) {
        mac_handle_update_event(event);
        return FALSE; /* BeginUpdate/EndUpdate ran; nothing left to do */
    }
    /* movable modal: ModalDialog doesn't track the title bar itself */
    if (event->what == mouseDown) {
        WindowPtr w;
        short part = FindWindow(event->where, &w);

        if (part == inDrag && w == GetDialogWindow(wind)) {
            Rect limits = qd.screenBits.bounds;

            InsetRect(&limits, 4, 4);
            DragWindow(w, event->where, &limits);
            *item = 0;
            return TRUE;
        }
    }
    if (event->what == keyDown || event->what == autoKey) {
        ch = (char) (event->message & 0xFF);
        if (ch == 0x0D || ch == 0x03) { /* return / enter */
            FlashButton(wind, prefSave);
            *item = prefSave;
            return TRUE;
        }
        if (ch == 0x1B /* escape */
            || ((event->modifiers & cmdKey) && ch == '.')) {
            FlashButton(wind, prefCancel);
            *item = prefCancel;
            return TRUE;
        }
    }
    return FALSE;
}

void
macprefs_dialog(void)
{
    GrafPtr oldport;
    DialogRef dlog;
    short item, type, i;
    Handle h;
    Rect r;
    Point pt;
    long sel;
    UiPrefs up;
    UserItemUPP redraw;
    ModalFilterUPP filter;

    if (!RetrieveUiPrefs(&up)) {
        /* no saved record: seed from the currently effective settings */
        memset(&up, 0, sizeof up);
        up.tiled_map = iflags.wc_tiled_map ? 1 : 0;
        up.hitpointbar = iflags.wc2_hitpointbar ? 1 : 0;
        up.statuslines = (iflags.wc2_statuslines == 3) ? 3 : 2;
        up.menutiles = iflags.use_menu_glyphs ? 1 : 0;
        up.sizes[uiFontMap] = iflags.wc_fontsiz_map;
        up.sizes[uiFontStatus] = iflags.wc_fontsiz_status;
        up.sizes[uiFontMessage] = iflags.wc_fontsiz_message;
        up.sizes[uiFontMenu] = iflags.wc_fontsiz_menu;
        up.sizes[uiFontText] = iflags.wc_fontsiz_text;
    }

    fontMenu = NewMenu(PREFS_FONT_MENU, P_STRING_CONV("Fonts"));
    if (!fontMenu)
        return;
    /* AppendMenu treats '(' as a disable metacharacter;
       SetMenuItemText stores the text verbatim */
    AppendMenu(fontMenu, P_STRING_CONV("x"));
    SetMenuItemText(fontMenu, 1, P_STRING_CONV("(default)"));
    AppendResMenu(fontMenu, 'FONT');
    InsertMenu(fontMenu, hierMenu);

    redraw = NewUserItemUPP(pref_redraw);
    filter = NewModalFilterUPP(pref_filter);
    dlog = GetNewDialog(PREFS_DLOG, NULL, (WindowRef) -1);
    if (!dlog) {
        DeleteMenu(PREFS_FONT_MENU);
        DisposeMenu(fontMenu);
        fontMenu = NULL;
        DisposeUserItemUPP(redraw);
        DisposeModalFilterUPP(filter);
        return;
    }
    GetPort(&oldport);
    SetPortDialogPort(dlog);

    set_check(dlog, prefTiles, up.tiled_map);
    set_check(dlog, prefHPbar, up.hitpointbar);
    set_check(dlog, prefMenuTiles, up.menutiles);
    set_check(dlog, prefLines2, up.statuslines != 3);
    set_check(dlog, prefLines3, up.statuslines == 3);
    for (i = 0; i < UIPREFS_NFONTS; i++) {
        if (up.fonts[i][0]) {
            fontSel[i] = menu_item_for_font(up.fonts[i]);
        } else {
            /* no explicit choice: show the currently effective font
               (saving then pins it; "(default)" reverts) */
            Str255 cur;

            GetFontName(win_fonts[uifont_nhw[i]], cur);
            fontSel[i] = menu_item_for_font(cur);
        }
        /* no explicit choice: show the currently effective size (saving
           then pins it; picking "(default)" reverts, same as fonts) */
        sizeVal[i] = up.sizes[i] ? up.sizes[i] : effective_size(i);
        GetDialogItem(dlog, prefFontFirst + i, &type, &h, &r);
        SetDialogItem(dlog, prefFontFirst + i, type, (Handle) redraw, &r);
        GetDialogItem(dlog, prefSizeFirst + i, &type, &h, &r);
        SetDialogItem(dlog, prefSizeFirst + i, type, (Handle) redraw, &r);
    }
    GetDialogItem(dlog, prefDefaultRing, &type, &h, &r);
    SetDialogItem(dlog, prefDefaultRing, type, (Handle) redraw, &r);
    GetDialogItem(dlog, prefSeparator, &type, &h, &r);
    SetDialogItem(dlog, prefSeparator, type, (Handle) redraw, &r);
    /* the initial GetNewDialog draw ran before the user-item procs were
       installed; repaint so the font popups aren't blank */
    GetWindowPortBounds(GetDialogWindow(dlog), &r);
    InvalWindowRect(GetDialogWindow(dlog), &r);

    do {
        ModalDialog(filter, &item);
        if (item == prefTiles || item == prefHPbar
            || item == prefMenuTiles) {
            set_check(dlog, item, !get_check(dlog, item));
        } else if (item == prefLines2 || item == prefLines3) {
            set_check(dlog, prefLines2, item == prefLines2);
            set_check(dlog, prefLines3, item == prefLines3);
        } else if (item >= prefFontFirst && item <= prefFontLast) {
            i = item - prefFontFirst;
            GetDialogItem(dlog, item, &type, &h, &r);
            pt.v = r.top;
            pt.h = r.left;
            LocalToGlobal(&pt);
            CheckMenuItem(fontMenu, fontSel[i], true);
            sel = PopUpMenuSelect(fontMenu, pt.v, pt.h, fontSel[i]);
            CheckMenuItem(fontMenu, fontSel[i], false);
            if (sel)
                fontSel[i] = LoWord(sel);
            InvalWindowRect(GetDialogWindow(dlog), &r);
        } else if (item >= prefSizeFirst && item <= prefSizeLast) {
            i = item - prefSizeFirst;
            GetDialogItem(dlog, item, &type, &h, &r);
            sizeVal[i] = pick_size(i, &r);
            InvalWindowRect(GetDialogWindow(dlog), &r);
        }
    } while (item != prefSave && item != prefCancel && item != prefForget);

    if (item == prefSave) {
        up.version = UIPREFS_VERSION;
        up.valid = 1;
        up.tiled_map = get_check(dlog, prefTiles) ? 1 : 0;
        up.hitpointbar = get_check(dlog, prefHPbar) ? 1 : 0;
        up.menutiles = get_check(dlog, prefMenuTiles) ? 1 : 0;
        up.statuslines = get_check(dlog, prefLines3) ? 3 : 2;
        for (i = 0; i < UIPREFS_NFONTS; i++) {
            if (fontSel[i] > 1) {
                Str255 fname; /* Str31 in UiPrefs; don't let
                                 GetMenuItemText overrun it */

                GetMenuItemText(fontMenu, fontSel[i], fname);
                if (fname[0] > 31)
                    fname[0] = 31;
                BlockMove(fname, up.fonts[i], (long) fname[0] + 1);
            } else
                up.fonts[i][0] = 0;
            up.sizes[i] = sizeVal[i];
        }
        StoreUiPrefs(&up);
    } else if (item == prefForget) {
        memset(&up, 0, sizeof up); /* valid = 0: config file rules again */
        StoreUiPrefs(&up);
    }

    DisposeDialog(dlog);
    SetPort(oldport);
    DeleteMenu(PREFS_FONT_MENU);
    DisposeMenu(fontMenu);
    fontMenu = NULL;
    DisposeUserItemUPP(redraw);
    DisposeModalFilterUPP(filter);
}

/* called from main() right after initoptions(): a saved record overrides
   the config file's UI options before any game window exists */
void
macprefs_apply_startup(void)
{
    UiPrefs up;
    short i, fnum;

    if (!RetrieveUiPrefs(&up))
        return;
    iflags.wc_tiled_map = up.tiled_map ? TRUE : FALSE;
    iflags.wc2_hitpointbar = up.hitpointbar ? TRUE : FALSE;
    iflags.wc2_statuslines = (up.statuslines == 3) ? 3 : 2;
    iflags.use_menu_glyphs = up.menutiles ? TRUE : FALSE;
    for (i = 0; i < UIPREFS_NFONTS; i++) {
        if (up.fonts[i][0]) {
            fnum = 0;
            GetFNum(up.fonts[i], &fnum);
            if (fnum) /* 0 = not found (and system font; skip both) */
                win_fonts[uifont_nhw[i]] = fnum;
        }
    }
    if (up.sizes[uiFontMap])
        iflags.wc_fontsiz_map = up.sizes[uiFontMap];
    if (up.sizes[uiFontStatus])
        iflags.wc_fontsiz_status = up.sizes[uiFontStatus];
    if (up.sizes[uiFontMessage])
        iflags.wc_fontsiz_message = up.sizes[uiFontMessage];
    if (up.sizes[uiFontMenu])
        iflags.wc_fontsiz_menu = up.sizes[uiFontMenu];
    if (up.sizes[uiFontText])
        iflags.wc_fontsiz_text = up.sizes[uiFontText];
}

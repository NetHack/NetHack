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
    prefSizeLast = prefSizeFirst + UIPREFS_NFONTS - 1
};

static MenuHandle fontMenu; /* "(default)" + AppendResMenu('FONT') */
static short fontSel[UIPREFS_NFONTS]; /* 1-based menu item; 1 = default */

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

static void
set_size_field(DialogRef dlog, short item, short val)
{
    short type;
    Handle h;
    Rect r;
    Str255 s;

    GetDialogItem(dlog, item, &type, &h, &r);
    if (val > 0)
        NumToString((long) val, s);
    else
        s[0] = 0;
    SetDialogItemText(h, s);
}

static short
get_size_field(DialogRef dlog, short item)
{
    short type;
    Handle h;
    Rect r;
    Str255 s;
    long val = 0;

    GetDialogItem(dlog, item, &type, &h, &r);
    GetDialogItemText(h, s);
    if (s[0])
        StringToNum(s, &val);
    if (val <= 0)
        return 0; /* empty/garbage => port default */
    if (val < 6)
        val = 6;
    if (val > 48)
        val = 48;
    return (short) val;
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

/* draw a font popup user item: framed box + drop shadow + selection */
static pascal void
pref_redraw(DialogRef dlog, DialogItemIndex item)
{
    short type;
    Handle h;
    Rect r;
    Str255 s;

    if (item < prefFontFirst || item > prefFontLast)
        return;
    GetDialogItem(dlog, item, &type, &h, &r);
    EraseRect(&r);
    FrameRect(&r);
    MoveTo(r.left + 3, r.bottom);
    LineTo(r.right, r.bottom);
    LineTo(r.right, r.top + 3);
    GetMenuItemText(fontMenu, fontSel[item - prefFontFirst], s);
    MoveTo(r.left + 6, r.bottom - 6);
    DrawString(s);
}

static pascal Boolean
pref_filter(DialogRef wind, EventRecord *event, DialogItemIndex *item)
{
    char ch;

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
        up.sizes[uiFontMap] = iflags.wc_fontsiz_map;
        up.sizes[uiFontStatus] = iflags.wc_fontsiz_status;
        up.sizes[uiFontMessage] = iflags.wc_fontsiz_message;
        up.sizes[uiFontMenu] = iflags.wc_fontsiz_menu;
        up.sizes[uiFontText] = iflags.wc_fontsiz_text;
    }

    fontMenu = NewMenu(PREFS_FONT_MENU, P_STRING_CONV("Fonts"));
    if (!fontMenu)
        return;
    AppendMenu(fontMenu, P_STRING_CONV("(default)"));
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
    set_check(dlog, prefLines2, up.statuslines != 3);
    set_check(dlog, prefLines3, up.statuslines == 3);
    for (i = 0; i < UIPREFS_NFONTS; i++) {
        fontSel[i] = menu_item_for_font(up.fonts[i]);
        GetDialogItem(dlog, prefFontFirst + i, &type, &h, &r);
        SetDialogItem(dlog, prefFontFirst + i, type, (Handle) redraw, &r);
        set_size_field(dlog, prefSizeFirst + i, up.sizes[i]);
    }
    /* the initial GetNewDialog draw ran before the user-item procs were
       installed; repaint so the font popups aren't blank */
    GetWindowPortBounds(GetDialogWindow(dlog), &r);
    InvalWindowRect(GetDialogWindow(dlog), &r);

    do {
        ModalDialog(filter, &item);
        if (item == prefTiles || item == prefHPbar) {
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
        }
    } while (item != prefSave && item != prefCancel && item != prefForget);

    if (item == prefSave) {
        up.version = UIPREFS_VERSION;
        up.valid = 1;
        up.tiled_map = get_check(dlog, prefTiles) ? 1 : 0;
        up.hitpointbar = get_check(dlog, prefHPbar) ? 1 : 0;
        up.statuslines = get_check(dlog, prefLines3) ? 3 : 2;
        for (i = 0; i < UIPREFS_NFONTS; i++) {
            if (fontSel[i] > 1)
                GetMenuItemText(fontMenu, fontSel[i], up.fonts[i]);
            else
                up.fonts[i][0] = 0;
            up.sizes[i] = get_size_field(dlog, prefSizeFirst + i);
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

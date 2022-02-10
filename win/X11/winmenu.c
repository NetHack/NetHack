/* NetHack 3.7	winmenu.c	$NHDT-Date: 1644531504 2022/02/10 22:18:24 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.50 $ */
/* Copyright (c) Dean Luick, 1992				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * File for creating menus.
 *
 *	+ Global functions: start_menu, add_menu, end_menu, select_menu
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV /* X11 include files may define SYSV */
#endif

#include <X11/Xresource.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Box.h>
#include <X11/Xos.h>

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#include "hack.h"
#include "winX.h"

static void menu_size_change_handler(Widget, XtPointer, XEvent *,
                                     Boolean *);
static void menu_select(Widget, XtPointer, XtPointer);
static void invert_line(struct xwindow *, x11_menu_item *, int, long);
static void menu_ok(Widget, XtPointer, XtPointer);
static void menu_cancel(Widget, XtPointer, XtPointer);
static void menu_all(Widget, XtPointer, XtPointer);
static void menu_none(Widget, XtPointer, XtPointer);
static void menu_invert(Widget, XtPointer, XtPointer);
static void menu_search(Widget, XtPointer, XtPointer);
static void search_menu(struct xwindow *);
static void select_all(struct xwindow *);
static void select_none(struct xwindow *);
static void select_match(struct xwindow *, char *);
static void invert_all(struct xwindow *);
static void invert_match(struct xwindow *, char *);
static void menu_popdown(struct xwindow *);
static unsigned menu_scrollmask(struct xwindow *);
static void menu_unscroll(struct xwindow *);
static Widget menu_create_buttons(struct xwindow *, Widget, Widget);
static void menu_create_entries(struct xwindow *, struct menu *);
static void destroy_menu_entry_widgets(struct xwindow *);
static void create_menu_translation_tables(void);

static void move_menu(struct menu *, struct menu *);
static void free_menu_line_entries(struct menu *);
static void free_menu(struct menu *);
static void reset_menu_to_default(struct menu *);
static void clear_old_menu(struct xwindow *);
static char *copy_of(const char *);

#define reset_menu_count(mi) ((mi)->counting = FALSE, (mi)->menu_count = 0L)

static const char menu_translations[] = "#override\n\
     <Key>Left: scroll(4)\n\
     <Key>Right: scroll(6)\n\
     <Key>Up: scroll(8)\n\
     <Key>Down: scroll(2)\n\
     <Btn4Down>: scroll(8)\n\
     <Btn5Down>: scroll(2)\n\
     <Key>: menu_key()";

static const char menu_entry_translations[] = "#override\n\
     <Btn4Down>: scroll(8)\n\
     <Btn5Down>: scroll(2)";

XtTranslations menu_entry_translation_table = (XtTranslations) 0;
XtTranslations menu_translation_table = (XtTranslations) 0;
XtTranslations menu_del_translation_table = (XtTranslations) 0;

static void
create_menu_translation_tables(void)
{
    if (!menu_translation_table) {
        menu_translation_table = XtParseTranslationTable(menu_translations);
        menu_entry_translation_table
            = XtParseTranslationTable(menu_entry_translations);
        menu_del_translation_table
            = XtParseTranslationTable("<Message>WM_PROTOCOLS: menu_delete()");
    }
}

/*ARGSUSED*/
static void
menu_size_change_handler(Widget w, XtPointer ptr, XEvent *event, Boolean *flag)
{
    struct xwindow *wp = (struct xwindow *) ptr;

    nhUse(w);
    nhUse(flag);

    if (!wp || !event)
        return;

    if (iflags.perm_invent && wp == &window_list[WIN_INVEN]
        && wp->menu_information->how == PICK_NONE) {
        get_widget_window_geometry(wp->popup,
                                   &wp->menu_information->permi_x,
                                   &wp->menu_information->permi_y,
                                   &wp->menu_information->permi_w,
                                   &wp->menu_information->permi_h);
    }
}


/*
 * Menu callback.
 */
/* ARGSUSED */
static void
menu_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    struct menu_info_t *menu_info;
    long how_many;
    x11_menu_item *curr = (x11_menu_item *) client_data;
    struct xwindow *wp;
    Arg args[2];

    nhUse(call_data);

    if (!curr)
        return;

    wp = &window_list[curr->window];

    menu_info = wp->menu_information;
    how_many = menu_info->counting ? menu_info->menu_count : -1L;
    reset_menu_count(menu_info);

    /* if the menu is not active or don't have an identifier, try again */
    if (!menu_info->is_active || curr->identifier.a_void == 0) {
        X11_nhbell();
        return;
    }

    /* if we've reached here, we've found our selected item */
    if (menu_info->how != PICK_ONE || !curr->preselected)
        curr->selected = !curr->selected;
    curr->preselected = FALSE;
    if (curr->selected) {
        curr->str[2] = (how_many != -1L) ? '#' : '+';
        curr->pick_count = how_many;
    } else {
        curr->str[2] = '-';
        curr->pick_count = -1L;
    }

    XtSetArg(args[0], nhStr(XtNlabel), curr->str);
    XtSetValues(w, args, ONE);

    if (menu_info->how == PICK_ONE)
        menu_popdown(wp);
}

/*
 * Called when menu window is deleted.
 */
/* ARGSUSED */
void
menu_delete(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    menu_cancel((Widget) None, (XtPointer) find_widget(w), (XtPointer) 0);
}

/*
 * Invert the count'th line (curr) in the given window.
 */
/*ARGSUSED*/
static void
invert_line(struct xwindow *wp, x11_menu_item *curr, int which, long how_many)
{
    Arg args[2];

    nhUse(which);
    reset_menu_count(wp->menu_information);
    /* invert selection unless explicitly choosing the preselected
       entry of a PICK_ONE menu */
    if (wp->menu_information->how != PICK_ONE || !curr->preselected)
        curr->selected = !curr->selected;
    curr->preselected = FALSE;
    if (curr->selected) {
        curr->str[2] = (how_many != -1) ? '#' : '+';
        XtSetArg(args[0], nhStr(XtNlabel), curr->str);
        XtSetValues(curr->w, args, ONE);
        curr->pick_count = how_many;
    } else {
        curr->str[2] = '-';
        XtSetArg(args[0], nhStr(XtNlabel), curr->str);
        XtSetValues(curr->w, args, ONE);
        curr->pick_count = -1L;
    }
}

static XEvent fake_perminv_event;

/*
 * Called when we get a key press event on a menu window.
 */
/* ARGSUSED */
void
menu_key(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    struct menu_info_t *menu_info;
    x11_menu_item *curr;
    struct xwindow *wp;
    Widget hbar, vbar;
    char ch;
    int count;
    boolean selected_something,
            perminv_scrolling = (event == &fake_perminv_event);

    nhUse(params);
    nhUse(num_params);

    wp = find_widget(w);
    menu_info = wp->menu_information;

    if (!perminv_scrolling)
        ch = key_event_to_char((XKeyEvent *) event);
    else
        ch = (char) fake_perminv_event.type;

    if (ch == '\0') { /* don't accept nul char/modifier event */
        /* don't beep */
        return;
    }

    /* don't exclude PICK_NONE menus; doing so disables scrolling via keys */
    if (menu_info->is_active || perminv_scrolling) { /* handle the input */
        /* first check for an explicit selector match, so that it won't be
           overridden if it happens to duplicate a mapped menu command (':'
           to look inside a container vs ':' to select via search string) */
        for (curr = menu_info->curr_menu.base; curr; curr = curr->next)
            if (curr->identifier.a_void != 0 && curr->selector == ch)
                goto make_selection;

        ch = map_menu_cmd(ch);
        if (ch == '\033') { /* quit */
            if (menu_info->counting || perminv_scrolling) {
                /* when there's a count in progress, ESC discards it
                   rather than dismissing the whole menu */
                reset_menu_count(menu_info);
                return;
            }
            select_none(wp);
        } else if (ch == '\n' || ch == '\r') {
            if (perminv_scrolling) {
                menu_unscroll(wp);
                return; /* skip menu_popdown() */
            }
            ; /* accept */
        } else if (digit(ch)) {
            /* special case: '0' is also the default ball class;
               some menus use digits as potential group accelerators
               but their entries don't rely on counts */
            if (!menu_info->counting
                && index(menu_info->curr_menu.gacc, ch))
                goto group_accel;
            menu_info->menu_count *= 10L;
            menu_info->menu_count += (long) (ch - '0');
            if (menu_info->menu_count != 0L) /* ignore leading zeros */
                menu_info->counting = TRUE;
            return;
        } else if (ch == MENU_SEARCH) { /* search */
            search_menu(wp);
            if (menu_info->how == PICK_ANY)
                return;
        } else if (ch == MENU_SELECT_ALL || ch == MENU_SELECT_PAGE) {
            if (menu_info->how == PICK_ANY)
                select_all(wp);
            else
                X11_nhbell();
            return;
        } else if (ch == MENU_UNSELECT_ALL || ch == MENU_UNSELECT_PAGE) {
            if (menu_info->how == PICK_ANY)
                select_none(wp);
            else
                X11_nhbell();
            return;
        } else if (ch == MENU_INVERT_ALL || ch == MENU_INVERT_PAGE) {
            if (menu_info->how == PICK_ANY)
                invert_all(wp);
            else
                X11_nhbell();
            return;
        } else if (ch == MENU_FIRST_PAGE || ch == MENU_LAST_PAGE) {
            float top = (ch == MENU_FIRST_PAGE) ? 0.0 : 1.0;

            find_scrollbars(wp->w, wp->popup, &hbar, &vbar);
            if (vbar)
                XtCallCallbacks(vbar, XtNjumpProc, &top);
            return;
        } else if (ch == MENU_NEXT_PAGE || ch == MENU_PREVIOUS_PAGE) {
            find_scrollbars(wp->w, wp->popup, &hbar, &vbar);
            if (vbar) {
                float shown, top;
                Arg arg[2];

                XtSetArg(arg[0], nhStr(XtNshown), &shown);
                XtSetArg(arg[1], nhStr(XtNtopOfThumb), &top);
                XtGetValues(vbar, arg, TWO);
                top += ((ch == MENU_NEXT_PAGE) ? shown : -shown);
                XtCallCallbacks(vbar, XtNjumpProc, &top);
            }
            return;
        } else if (ch == MENU_SHIFT_RIGHT || ch == MENU_SHIFT_LEFT) {
            find_scrollbars(wp->w, wp->popup, &hbar, &vbar);
            if (hbar) {
                float shown, halfshown, left;
                Arg arg[2];

                XtSetArg(arg[0], nhStr(XtNshown), &shown);
                XtSetArg(arg[1], nhStr(XtNtopOfThumb), &left);
                XtGetValues(hbar, arg, TWO);
                halfshown = shown * 0.5;
                left += ((ch == MENU_SHIFT_RIGHT) ? halfshown : -halfshown);
                XtCallCallbacks(hbar, XtNjumpProc, &left);
            }
            return;
        } else if (index(menu_info->curr_menu.gacc, ch)) {
 group_accel:
            /* matched a group accelerator */
            if (menu_info->how == PICK_ANY || menu_info->how == PICK_ONE) {
                for (count = 0, curr = menu_info->curr_menu.base; curr;
                     curr = curr->next, count++) {
                    if (curr->identifier.a_void != 0
                        && curr->gselector == ch) {
                        invert_line(wp, curr, count, -1L);
                        /* for PICK_ONE, a group accelerator will
                           only be included in gacc[] if it matches
                           exactly one entry, so this must be it... */
                        if (menu_info->how == PICK_ONE)
                            goto menu_done; /* pop down */
                    }
                }
            } else
                X11_nhbell();
            return;
        } else {
 make_selection:
            selected_something = FALSE;
            for (count = 0, curr = menu_info->curr_menu.base; curr;
                 curr = curr->next, count++)
                if (curr->identifier.a_void != 0 && curr->selector == ch)
                    break;

            if (curr) {
                invert_line(wp, curr, count,
                            menu_info->counting ? menu_info->menu_count : -1L);
                selected_something = curr->selected;
            } else {
                X11_nhbell(); /* no match */
            }
            if (!(selected_something && menu_info->how == PICK_ONE))
                return; /* keep going */
        }
        /* pop down */
    } else { /* permanent inventory window */
        if (ch != '\033') {
            X11_nhbell();
            return;
        }
        /* pop down on ESC */
    }

 menu_done:
    menu_popdown(wp);
}

/* ARGSUSED */
static void
menu_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
    struct xwindow *wp = (struct xwindow *) client_data;

    nhUse(w);
    nhUse(call_data);

    menu_popdown(wp);
}

/* ARGSUSED */
static void
menu_cancel(Widget w, /* don't use - may be None */
            XtPointer client_data, XtPointer call_data)
{
    struct xwindow *wp = (struct xwindow *) client_data;

    nhUse(w);
    nhUse(call_data);

    if (wp->menu_information->is_active) {
        select_none(wp);
        wp->menu_information->cancelled = TRUE;
    }
    menu_popdown(wp);
}

/* ARGSUSED */
static void
menu_all(Widget w, XtPointer client_data, XtPointer call_data)
{
    nhUse(w);
    nhUse(call_data);

    select_all((struct xwindow *) client_data);
}

/* ARGSUSED */
static void
menu_none(Widget w, XtPointer client_data, XtPointer call_data)
{
    nhUse(w);
    nhUse(call_data);

    select_none((struct xwindow *) client_data);
}

/* ARGSUSED */
static void
menu_invert(Widget w, XtPointer client_data, XtPointer call_data)
{
    nhUse(w);
    nhUse(call_data);

    invert_all((struct xwindow *) client_data);
}

/* ARGSUSED */
static void
menu_search(Widget w, XtPointer client_data, XtPointer call_data)
{
    struct xwindow *wp = (struct xwindow *) client_data;
    struct menu_info_t *menu_info = wp->menu_information;

    nhUse(w);
    nhUse(call_data);

    search_menu(wp);
    if (menu_info->how == PICK_ONE)
        menu_popdown(wp);
}

/* common to menu_search and menu_key */
static void
search_menu(struct xwindow *wp)
{
    char *pat, buf[BUFSZ + 2]; /* room for '*' + BUFSZ-1 + '*' + '\0' */
    struct menu_info_t *menu_info = wp->menu_information;

    buf[0] = buf[1] = '\0';
    pat = &buf[1]; /* leave room to maybe insert '*' at front */
    if (menu_info->how != PICK_NONE) {
        X11_getlin("Search for:", pat);
        if (!*pat || *pat == '\033')
            return;
        /* convert "string" into "*string*" for use with pmatch() */
        if (*pat != '*')
            *--pat = '*'; /* now points to &buf[0] */
        if (*(eos(pat) - 1) != '*')
            Strcat(pat, "*");
    }

    switch (menu_info->how) {
    case PICK_ANY:
        invert_match(wp, pat);
        break;
    case PICK_ONE:
        select_match(wp, pat);
        break;
    default: /* PICK_NONE */
        X11_nhbell();
        break;
    }
}

static void
select_all(struct xwindow *wp)
{
    x11_menu_item *curr;
    int count;

    reset_menu_count(wp->menu_information);
    for (count = 0, curr = wp->menu_information->curr_menu.base; curr;
         curr = curr->next, count++)
        if (curr->identifier.a_void != 0)
            if (!curr->selected) {
                invert_line(wp, curr, count, -1L);
            }

}

static void
select_none(struct xwindow *wp)
{
    x11_menu_item *curr;
    int count;

    reset_menu_count(wp->menu_information);
    for (count = 0, curr = wp->menu_information->curr_menu.base; curr;
         curr = curr->next, count++)
        if (curr->identifier.a_void != 0)
            if (curr->selected) {
                invert_line(wp, curr, count, -1L);
            }

}

static void
invert_all(struct xwindow *wp)
{
    x11_menu_item *curr;
    int count;

    reset_menu_count(wp->menu_information);
    for (count = 0, curr = wp->menu_information->curr_menu.base; curr;
         curr = curr->next, count++) {
        if (!menuitem_invert_test(0, curr->itemflags, curr->selected))
            continue;
        if (curr->identifier.a_void != 0)
            invert_line(wp, curr, count, -1L);
    }
}

static void
invert_match(struct xwindow *wp,
             char *match) /* wildcard pattern for pmatch() */
{
    x11_menu_item *curr;
    int count;

    reset_menu_count(wp->menu_information);
    for (count = 0, curr = wp->menu_information->curr_menu.base; curr;
         curr = curr->next, count++)
        if (curr->identifier.a_void != 0) {
            if (pmatchi(match, curr->str)) {
                invert_line(wp, curr, count, -1L);
            }
            curr->preselected = FALSE;
        }

}

static void
select_match(struct xwindow *wp,
             char *match) /* wildcard pattern for pmatch() */
{
    x11_menu_item *curr, *found = 0;
    int count;

    reset_menu_count(wp->menu_information);
    for (count = 0, curr = wp->menu_information->curr_menu.base; curr;
         curr = curr->next, count++)
        if (curr->identifier.a_void != 0) {
            if (!found && pmatchi(match, curr->str))
                found = curr;
            curr->preselected = FALSE;
        }

    if (found) {
        if (!found->selected) {
            invert_line(wp, found, count, -1L);
        }
    } else {
        /* no match */
        X11_nhbell();
    }
}

static void
menu_popdown(struct xwindow *wp)
{
    nh_XtPopdown(wp->popup); /* remove the event grab */
    XtDestroyWidget(wp->popup);
    wp->w = wp->popup = (Widget) 0;
    if (wp->menu_information->is_active)
        exit_x_event = TRUE;             /* exit our event handler */
    wp->menu_information->is_up = FALSE; /* menu is down */
}

/* construct a bit mask specifying which scrolling operations are allowed */
static unsigned
menu_scrollmask(struct xwindow *wp)
{
    float shown, top;
    Arg args[2];
    Widget hbar = (Widget) 0, vbar = (Widget) 0;
    unsigned scrlmask = 0U;

    /* set up args once, then use twice (provided that both scrollbars are
       present); 'top' is left for horizontal scrollbar */
    (void) memset(args, 0, sizeof args);
    XtSetArg(args[0], nhStr(XtNshown), &shown);
    XtSetArg(args[1], nhStr(XtNtopOfThumb), &top);

    find_scrollbars(wp->w, wp->popup, &hbar, &vbar);
    if (vbar) {
        XtGetValues(vbar, args, TWO);
        if (top > 0.0)
            scrlmask |= 1U; /* not at top; can scroll up */
        if (top + shown < 1.0)
            scrlmask |= 2U; /* more beyond bottom; can scroll down */
    }
    if (hbar) {
        XtGetValues(hbar, args, TWO);
        if (top > 0.0)
            scrlmask |= 4U; /* not at left edge; can scroll to left */
        if (top + shown < 1.0)
            scrlmask |= 8U; /* more beyond right side; can scroll to right */
    }
    return scrlmask;
}

/* if a menu is scrolled vertically and/horizontallty, return it to the top
   and far left */
static void
menu_unscroll(struct xwindow *wp)
{
    float top, left, zero = 0.0;
    Arg arg;
    Widget hbar = (Widget) 0, vbar = (Widget) 0;

    find_scrollbars(wp->w, wp->popup, &hbar, &vbar);
    if (hbar) {
        XtSetArg(arg, nhStr(XtNtopOfThumb), &left);
        XtGetValues(hbar, &arg, ONE);
        if (left > 0.0)
            XtCallCallbacks(hbar, XtNjumpProc, &zero);
    }
    if (vbar) {
        XtSetArg(arg, nhStr(XtNtopOfThumb), &top);
        XtGetValues(vbar, &arg, ONE);
        if (top > 0.0)
            XtCallCallbacks(vbar, XtNjumpProc, &zero);
    }
    return;
}

/* Global functions ======================================================= */

/* called by X11_update_inventory() if persistent inventory is currently
   displayed but the 'perm_invent' option is now Off */
void
x11_no_perminv(struct xwindow *wp)
{
    if (wp && wp->type == NHW_MENU && wp->menu_information->is_up) {
        destroy_menu_entry_widgets(wp);
        menu_popdown(wp);
    }
}

/* called by X11_update_inventory() if user has executed #perminv command */
void
x11_scroll_perminv(int arg UNUSED) /* arg is always 1 */
{
    static const char extrakeys[] = "\033 \n\r\003\177\b";
    char ch, menukeys[QBUFSZ];
    boolean save_is_active;
    unsigned scrlmask;
    Cardinal no_args = 0;
    struct xwindow *wp = &window_list[WIN_INVEN];

    /* caller has ensured that perm_invent is enabled, but the window
       might not be displayed; if that's the case, display it */
    if (wp->type == NHW_MENU && !wp->menu_information->is_up)
        X11_update_inventory(0);
    /* if it's still not displayed for some reason, bail out now */
    if (wp->type != NHW_MENU || !wp->menu_information->is_up) {
        X11_nhbell();
        return;
    }

    do {
        scrlmask = menu_scrollmask(wp);
        (void) collect_menu_keys(menukeys, scrlmask, FALSE);
        /*
         * Add quitchars plus a few others to the player's scrolling keys.
         * We accept some extra characters that menus usually ignore:
         * ^C will be treated like <escape>, leaving menu positioned as-is
         * and returning to play; <delete> or <backspace> will be treated
         * like <return> and <space>, reseting the menu to its top and
         * returning to play; other characters will either be rejected by
         * yn_function or stay here for scrolling.
         */
        Strcat(menukeys, extrakeys);
        /* append any scrolling keys excluded by scrlmask, after the \033
           added by extrakeys; they'll be acceptable but not shown */
        (void) collect_menu_keys(eos(menukeys), ~scrlmask, FALSE);

        /* normally the perm_invent menu is not flagged 'is_active' because
           it doesn't accept input, so menu_popdown() doesn't set the flag
           for the event loop to exit; force 'is_active' while this prompt
           is in progress so that the prompt won't be left pending if
           player closes the menu via mouse */
        save_is_active = wp->menu_information->is_active;
        wp->menu_information->is_active = TRUE;
        ch = X11_yn_function_core("Inventory scroll:", menukeys,
                                  0, (YN_NO_LOGMESG | YN_NO_DEFAULT));
        if (wp->menu_information->is_up)
            wp->menu_information->is_active = save_is_active;
        else
            ch = 0;

        if (ch == C('c')) /* ^C */
            ch = '\033';
        else if (ch == '\177' || ch == '\b') /* <delete> or <backspace> */
            ch = '\n';

        if (ch && ch != '\033') {
            /* in case persistent inventory window is covered, force it
               to be on top; does not grab pointer or keyboard focus */
            XMapRaised(XtDisplay(wp->popup), XtWindow(wp->popup));

            /* the fake event never goes onto X's event queue; it is only
               examined by menu_key(), so we shortcut the messy details in
               favor of easy to handle union type code; might conceivably
               confuse a sophisticated debugger so we should possibly redo
               this to set it up properly:  event->keyevent->keycode */
            fake_perminv_event.type = ch;
            menu_key(wp->w, &fake_perminv_event, (String *) 0, &no_args);
            fake_perminv_event.type = 0;
        }

        /* if yn_function() is using a popup (the 'slow=False' setting
           in NetHack.ad) for its prompt+response and there is any
           overlap between the persistent inventory and main windows,
           perm_invent would be pushed behind the map every iteration of
           this loop, so handle only one character at a time for !slow */
        if (!appResources.slow)
            break;
    } while (ch && !index(quitchars, ch));

    return;
}

void
X11_start_menu(winid window, unsigned long mbehavior UNUSED)
{
    struct xwindow *wp;
    check_winid(window);

    wp = &window_list[window];

    if (wp->menu_information->is_menu) {
        /* make sure we'ere starting with a clean slate */
        free_menu(&wp->menu_information->new_menu);
    } else {
        wp->menu_information->is_menu = TRUE;
    }
}

/*ARGSUSED*/
void
X11_add_menu(winid window,
             const glyph_info *glyphinfo UNUSED,
             const anything *identifier,
             char ch,  /* selector letter; 0 if not selectable */
             char gch, /* group accelerator (0 = no group) */
             int attr,
             const char *str,
             unsigned itemflags)
{
    x11_menu_item *item;
    struct menu_info_t *menu_info;
    boolean preselected = (itemflags & MENU_ITEMFLAGS_SELECTED) != 0;

    check_winid(window);
    menu_info = window_list[window].menu_information;
    if (!menu_info->is_menu) {
        impossible("add_menu:  called before start_menu");
        return;
    }

    item = (x11_menu_item *) alloc((unsigned) sizeof(x11_menu_item));
    item->next = (x11_menu_item *) 0;
    item->identifier = *identifier;
    item->attr = attr;
    item->itemflags = itemflags;
    item->selected = item->preselected = preselected;
    item->pick_count = -1L;
    item->window = window;
    item->w = (Widget) 0;

    if (identifier->a_void) {
        char buf[4 + BUFSZ];
        int len = strlen(str);

        if (!ch) {
            /* Supply a keyboard accelerator.  Only the first 52 get one. */

            if (menu_info->new_menu.curr_selector) {
                ch = menu_info->new_menu.curr_selector++;
                if (ch == 'z')
                    menu_info->new_menu.curr_selector = 'A';
                else if (ch == 'Z')
                    menu_info->new_menu.curr_selector = 0; /* out */
            }
        }

        if (len >= BUFSZ) {
            /* We *think* everything's coming in off at most BUFSZ bufs... */
            impossible("Menu item too long (%d).", len);
            len = BUFSZ - 1;
        }
        Sprintf(buf, "%c %c ", ch ? ch : ' ', preselected ? '+' : '-');
        (void) strncpy(buf + 4, str, len);
        buf[4 + len] = '\0';
        item->str = copy_of(buf);
    } else {
        /* no keyboard accelerator */
        item->str = copy_of(str);
        ch = 0;
    }

    item->selector = ch;
    item->gselector = gch;

    debugpline2("X11_add_menu(%i,%s)", window, item->str);

    if (menu_info->new_menu.last) {
        menu_info->new_menu.last->next = item;
    } else {
        menu_info->new_menu.base = item;
    }
    menu_info->new_menu.last = item;
    menu_info->new_menu.count++;
}

void
X11_end_menu(winid window, const char *query)
{
    struct menu_info_t *menu_info;

    check_winid(window);
    menu_info = window_list[window].menu_information;
    if (!menu_info->is_menu) {
        impossible("end_menu:  called before start_menu");
        return;
    }
    debugpline2("X11_end_menu(%i, %s)", window, query);
    menu_info->new_menu.query = copy_of(query);
}

int
X11_select_menu(winid window, int how, menu_item **menu_list)
{
    x11_menu_item *curr;
    struct xwindow *wp;
    struct menu_info_t *menu_info;
    Arg args[10];
    Cardinal num_args;
    int retval;
    Dimension v_pixel_width, v_pixel_height;
    boolean labeled;
    Widget viewport_widget, form, label, all;
    char gacc[QBUFSZ], *ap;
    boolean permi;

    *menu_list = (menu_item *) 0;
    check_winid(window);
    wp = &window_list[window];
    menu_info = wp->menu_information;
    if (!menu_info->is_menu) {
        impossible("select_menu:  called before start_menu");
        return 0;
    }

    debugpline2("X11_select_menu(%i, %i)", window, how);

    create_menu_translation_tables();

    if (menu_info->permi && how != PICK_NONE) {
        /* Core is reusing perm_invent window for picking an item.
           But it could be even on a different screen!
           Create a new temp window for it instead. */
        winid newwin = X11_create_nhwindow(NHW_MENU);
        struct xwindow *nwp = &window_list[newwin];

        X11_start_menu(newwin, MENU_BEHAVE_STANDARD);
        move_menu(&menu_info->new_menu, &nwp->menu_information->new_menu);
        for (curr = nwp->menu_information->new_menu.base; curr;
             curr = curr->next)
            curr->window = newwin;
        nwp->menu_information->permi = FALSE;
        retval = X11_select_menu(newwin, how, menu_list);
        destroy_menu_entry_widgets(nwp);
        X11_destroy_nhwindow(newwin);
        return retval;
    }

    menu_info->how = (short) how;

    /* collect group accelerators; for PICK_NONE, they're ignored;
       for PICK_ONE, only those which match exactly one entry will be
       accepted; for PICK_ANY, those which match any entry are okay */
    gacc[0] = '\0';
    if (menu_info->how != PICK_NONE) {
        int i, n, gcnt[128];
#define GSELIDX(c) ((c) & 127) /* guard against `signed char' */

        for (i = 0; i < SIZE(gcnt); i++)
            gcnt[i] = 0;
        for (n = 0, curr = menu_info->new_menu.base; curr; curr = curr->next)
            if (curr->gselector && curr->gselector != curr->selector) {
                ++n;
                ++gcnt[GSELIDX(curr->gselector)];
            }

        if (n > 0) /* at least one group accelerator found */
            for (ap = gacc, curr = menu_info->new_menu.base; curr;
                 curr = curr->next)
                if (curr->gselector && !index(gacc, curr->gselector)
                    && (menu_info->how == PICK_ANY
                        || gcnt[GSELIDX(curr->gselector)] == 1)) {
                    *ap++ = curr->gselector;
                    *ap = '\0'; /* re-terminate for index() */
                }
    }
    menu_info->new_menu.gacc = copy_of(gacc);
    reset_menu_count(menu_info);

    labeled = (menu_info->new_menu.query && *(menu_info->new_menu.query))
                  ? TRUE
                  : FALSE;

    permi = (window == WIN_INVEN && iflags.perm_invent && how == PICK_NONE);

    if (menu_info->is_up && !menu_info->permi) {
        destroy_menu_entry_widgets(wp);
        menu_popdown(wp);
    }

    if (!menu_info->is_up) {
        menu_info->permi = permi;

        num_args = 0;
        XtSetArg(args[num_args], XtNallowShellResize, True);
        num_args++;
        if (permi && menu_info->permi_x != -1) {
            XtSetArg(args[num_args], nhStr(XtNwidth), menu_info->permi_w);
            num_args++;
            XtSetArg(args[num_args], nhStr(XtNheight), menu_info->permi_h);
            num_args++;
        }
        if (wp->title) {
            XtSetArg(args[num_args], nhStr(XtNtitle), wp->title); num_args++;
        }
        wp->popup = XtCreatePopupShell((window == WIN_INVEN)
                                           ? "inventory" : "menu",
                                       (how == PICK_NONE)
                                           ? topLevelShellWidgetClass
                                           : transientShellWidgetClass,
                                       toplevel, args, num_args);
        XtOverrideTranslations(wp->popup, menu_del_translation_table);

        if (permi)
            XtAddEventHandler(wp->popup, StructureNotifyMask, False,
                              menu_size_change_handler, (XtPointer) wp);

        num_args = 0;
        XtSetArg(args[num_args], XtNtranslations,
                 menu_translation_table); num_args++;
        form = XtCreateManagedWidget("mform", formWidgetClass, wp->popup,
                                     args, num_args);

        num_args = 0;
        XtSetArg(args[num_args], XtNborderWidth, 0); num_args++;
        XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
        XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
        XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
        XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;

        label = labeled ? XtCreateManagedWidget(menu_info->new_menu.query,
                                                labelWidgetClass, form,
                                                args, num_args)
                        : (Widget) 0;

        all = menu_create_buttons(wp, form, label);

        num_args = 0;
        XtSetArg(args[num_args], nhStr(XtNallowVert), True); num_args++;
        /* allow horizontal scroll bar for persistent inventory window;
           it could be allowed for any menu, but when scrolled to the side
           the selector letters aren't visible so we won't do that [yet?] */
        XtSetArg(args[num_args], nhStr(XtNallowHoriz), permi ? True : False);
                                                                   num_args++;
        XtSetArg(args[num_args], nhStr(XtNuseBottom), True); num_args++;
        XtSetArg(args[num_args], nhStr(XtNuseRight), True); num_args++;
#if 0
        XtSetArg(args[num_args], nhStr(XtNforceBars), True); num_args++;
#endif
        XtSetArg(args[num_args], nhStr(XtNfromVert), all); num_args++;
        XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
        XtSetArg(args[num_args], nhStr(XtNbottom), XtChainBottom); num_args++;
        XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
        XtSetArg(args[num_args], nhStr(XtNright), XtChainRight); num_args++;
        XtSetArg(args[num_args], XtNtranslations,
                 menu_translation_table); num_args++;
        viewport_widget = XtCreateManagedWidget("menu_viewport",
                                                viewportWidgetClass, form,
                                                args, num_args);

        num_args = 0;
        XtSetArg(args[num_args], XtNwidth, 100);
        num_args++;
        XtSetArg(args[num_args], XtNheight, 500);
        num_args++;

        wp->w = XtCreateManagedWidget("menu_list", formWidgetClass,
                                      viewport_widget, args, num_args);

    }

    if (menu_info->is_up && permi && menu_info->curr_menu.base) {
        /* perm_invent window - explicitly destroy old menu entry widgets,
           without recreating whole window */
        destroy_menu_entry_widgets(wp);
        free_menu_line_entries(&menu_info->curr_menu);
    }

    /* make new menu the current menu */
    move_menu(&menu_info->new_menu, &menu_info->curr_menu);
    menu_create_entries(wp, &menu_info->curr_menu);

    /* if viewport will be bigger than the screen, limit its height */
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, &v_pixel_width); num_args++;
    XtSetArg(args[num_args], XtNheight, &v_pixel_height); num_args++;
    XtGetValues(wp->w, args, num_args);
    if ((Dimension) XtScreen(wp->w)->height * 5 / 6 < v_pixel_height) {
        /* scrollbar is 14 pixels wide.  Widen the form to accommodate it. */
        v_pixel_width += 14;

        /* shrink to fit vertically */
        v_pixel_height = XtScreen(wp->w)->height * 5 / 6;

        num_args = 0;
        XtSetArg(args[num_args], XtNwidth, v_pixel_width); num_args++;
        XtSetArg(args[num_args], XtNheight, v_pixel_height); num_args++;
        XtSetValues(wp->w, args, num_args);
    }
    XtRealizeWidget(wp->popup); /* need to realize before we position */

    /* if menu is not up, position it */
    if (!menu_info->is_up) {
        positionpopup(wp->popup, FALSE);
    }

    menu_info->is_up = TRUE;
    if (permi) {
        if (menu_info->permi_x != -1) {
            /* Cannot set window x,y at creation time,
               we must move the window now instead */
            XMoveWindow(XtDisplay(wp->popup), XtWindow(wp->popup),
                        menu_info->permi_x, menu_info->permi_y);
        }
        /* cant use nh_XtPopup() because it may try to grab the focus */
        XtPopup(wp->popup, (int) XtGrabNone);
        if (menu_info->permi_x == -1) {
            /* remember perm_invent window geometry the first time */
            get_widget_window_geometry(wp->popup,
                                       &menu_info->permi_x,
                                       &menu_info->permi_y,
                                       &menu_info->permi_w,
                                       &menu_info->permi_h);
        }
        if (!updated_inventory) {
            XMapRaised(XtDisplay(wp->popup), XtWindow(wp->popup));
        }
        XSetWMProtocols(XtDisplay(wp->popup), XtWindow(wp->popup),
                        &wm_delete_window, 1);
        retval = 0;
    } else {
        menu_info->is_active = TRUE; /* waiting for user response */
        menu_info->cancelled = FALSE;
        nh_XtPopup(wp->popup, (int) XtGrabExclusive, wp->w);
        (void) x_event(EXIT_ON_EXIT);
        menu_info->is_active = FALSE;
        if (menu_info->cancelled)
            return -1;

        retval = 0;
        for (curr = menu_info->curr_menu.base; curr; curr = curr->next)
            if (curr->selected)
                retval++;

        if (retval) {
            menu_item *mi;
            boolean toomany = (how == PICK_ONE && retval > 1);

            if (toomany)
                retval = 1;
            *menu_list = mi = (menu_item *) alloc(retval * sizeof(menu_item));
            for (curr = menu_info->curr_menu.base; curr; curr = curr->next)
                if (curr->selected) {
                    mi->item = curr->identifier;
                    mi->count = curr->pick_count;
                    if (!toomany)
                        mi++;
                    if (how == PICK_ONE && !curr->preselected)
                        break;
                }
        }
    } /* ?(WIN_INVEN && PICK_NONE) */

    return retval;
}

/* End global functions =================================================== */

/*
 * Allocate a copy of the given string.  If null, return a string of
 * zero length.
 */
static char *
copy_of(const char *s)
{
    if (!s)
        s = "";
    return dupstr(s);
}

/*
 * Create ok, cancel, all, none, invert, and search buttons.
 */
static Widget
menu_create_buttons(struct xwindow *wp, Widget form, Widget under)
{
    Arg args[15];
    Cardinal num_args;
    int how = wp->menu_information->how;
    Boolean sens;
    Widget ok, cancel, all, none, invert, search, lblwidget[6];
    Dimension lblwidth[6], maxlblwidth;
    Widget label = under;

    maxlblwidth = 0;
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), label); num_args++;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    ok = XtCreateManagedWidget("OK", commandWidgetClass, form,
                               args, num_args);
    XtAddCallback(ok, XtNcallback, menu_ok, (XtPointer) wp);
    XtSetArg(args[0], XtNwidth, &lblwidth[0]);
    XtGetValues(lblwidget[0] = ok, args, ONE);
    if (lblwidth[0] > maxlblwidth)
        maxlblwidth = lblwidth[0];

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), label); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromHoriz), ok); num_args++;
#if 0   /* [cancel] is a viable choice even for PICK_NONE */
    XtSetArg(args[num_args], nhStr(XtNsensitive), how != PICK_NONE); num_args++;
#endif
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    cancel = XtCreateManagedWidget("cancel", commandWidgetClass, form,
                                   args, num_args);
    XtAddCallback(cancel, XtNcallback, menu_cancel, (XtPointer) wp);
    XtSetArg(args[0], XtNwidth, &lblwidth[1]);
    XtGetValues(lblwidget[1] = cancel, args, ONE);
    if (lblwidth[1] > maxlblwidth)
        maxlblwidth = lblwidth[1];

    sens = (how == PICK_ANY);
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), label); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromHoriz), cancel); num_args++;
    XtSetArg(args[num_args], nhStr(XtNsensitive), sens); num_args++;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    all = XtCreateManagedWidget("all", commandWidgetClass, form,
                                args, num_args);
    XtAddCallback(all, XtNcallback, menu_all, (XtPointer) wp);
    XtSetArg(args[0], XtNwidth, &lblwidth[2]);
    XtGetValues(lblwidget[2] = all, args, ONE);
    if (lblwidth[2] > maxlblwidth)
        maxlblwidth = lblwidth[2];

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), label); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromHoriz), all); num_args++;
    XtSetArg(args[num_args], nhStr(XtNsensitive), sens); num_args++;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    none = XtCreateManagedWidget("none", commandWidgetClass, form,
                                 args, num_args);
    XtAddCallback(none, XtNcallback, menu_none, (XtPointer) wp);
    XtSetArg(args[0], XtNwidth, &lblwidth[3]);
    XtGetValues(lblwidget[3] = none, args, ONE);
    if (lblwidth[3] > maxlblwidth)
        maxlblwidth = lblwidth[3];

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), label); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromHoriz), none); num_args++;
    XtSetArg(args[num_args], nhStr(XtNsensitive), sens); num_args++;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    invert = XtCreateManagedWidget("invert", commandWidgetClass, form,
                                   args, num_args);
    XtAddCallback(invert, XtNcallback, menu_invert, (XtPointer) wp);
    XtSetArg(args[0], XtNwidth, &lblwidth[4]);
    XtGetValues(lblwidget[4] = invert, args, ONE);
    if (lblwidth[4] > maxlblwidth)
        maxlblwidth = lblwidth[4];

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), label); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromHoriz), invert); num_args++;
    XtSetArg(args[num_args], nhStr(XtNsensitive), how != PICK_NONE);
                                                                   num_args++;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    search = XtCreateManagedWidget("search", commandWidgetClass, form,
                                   args, num_args);
    XtAddCallback(search, XtNcallback, menu_search, (XtPointer) wp);
    XtSetArg(args[0], XtNwidth, &lblwidth[5]);
    XtGetValues(lblwidget[5] = search, args, ONE);
    if (lblwidth[5] > maxlblwidth)
        maxlblwidth = lblwidth[5];

    /* make all buttons be the same width */
    {
        int i;

        XtSetArg(args[0], XtNwidth, maxlblwidth);
        for (i = 0; i < 6; ++i)
            if (lblwidth[i] < maxlblwidth)
                XtSetValues(lblwidget[i], args, ONE);
    }

    return all;
}

static void
menu_create_entries(struct xwindow *wp, struct menu *curr_menu)
{
    x11_menu_item *curr;
    int menulineidx = 0;
    Widget prevlinewidget;
    int how = wp->menu_information->how;
    Arg args[15];
    Cardinal num_args;

    for (curr = curr_menu->base; curr; curr = curr->next) {
        char tmpbuf[BUFSZ];
        Widget linewidget;
        String str = (String) curr->str;
        int attr = ATR_NONE;
        int color = NO_COLOR;
        boolean canpick = (how != PICK_NONE && curr->identifier.a_void);

        num_args = 0;
        XtSetArg(args[num_args], nhStr(XtNlabel), str); num_args++;
        XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
        XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
        XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
        XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
        XtSetArg(args[num_args], nhStr(XtNborderWidth), 0); num_args++;
        XtSetArg(args[num_args], nhStr(XtNvertDistance), 0); num_args++;

        if (!iflags.use_menu_color || wp->menu_information->disable_mcolors
            || !get_menu_coloring(curr->str, &color, &attr))
            attr = curr->attr;

        if (color != NO_COLOR) {
            if (attr != ATR_INVERSE)
                XtSetArg(args[num_args], nhStr(XtNforeground),
                         get_nhcolor(wp, color).pixel); num_args++;
        }

        /* TODO: ATR_DIM, ATR_ULINE, ATR_BLINK */

        if (attr == ATR_INVERSE) {
            XtSetArg(args[num_args], nhStr(XtNforeground),
                     get_nhcolor(wp, CLR_BLACK).pixel); num_args++;
            XtSetArg(args[num_args], nhStr(XtNbackground),
                     get_nhcolor(wp, color).pixel); num_args++;
        }

        if (menulineidx) {
            XtSetArg(args[num_args], nhStr(XtNfromVert), prevlinewidget);
                                                                   num_args++;
        } else {
            XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
        }

        XtSetArg(args[num_args], XtNtranslations,
                 menu_entry_translation_table); num_args++;

        menulineidx++;
        Sprintf(tmpbuf, "menuline_%s", (canpick) ? "command" : "label");
        curr->w = linewidget = XtCreateManagedWidget(tmpbuf,
                                                     canpick
                                                       ? commandWidgetClass
                                                       : labelWidgetClass,
                                                     wp->w, args, num_args);

        if (attr == ATR_BOLD) {
            load_boldfont(wp, curr->w);
            num_args = 0;
            XtSetArg(args[num_args], nhStr(XtNfont),
                     wp->boldfs); num_args++;
            XtSetValues(curr->w, args, num_args);
        }

        if (canpick)
            XtAddCallback(linewidget, XtNcallback, menu_select,
                          (XtPointer) curr);
        prevlinewidget = linewidget;
    }
}

static void
destroy_menu_entry_widgets(struct xwindow *wp)
{
    WidgetList wlist;
    Cardinal numchild;
    Arg args[5];
    Cardinal num_args;
    x11_menu_item *curr;
    struct menu_info_t *menu_info;

    if (!wp || !wp->w)
        return;

    menu_info = wp->menu_information;
    num_args = 0;
    XtSetArg(args[num_args], XtNchildren, &wlist); num_args++;
    XtSetArg(args[num_args], XtNnumChildren, &numchild); num_args++;
    XtGetValues(wp->w, args, num_args);
    XtUnmanageChildren(wlist, numchild);
    for (curr = menu_info->curr_menu.base; curr; curr = curr->next)
        if (curr->w)
            XtDestroyWidget(curr->w);
}

static void
move_menu(struct menu *src_menu, struct menu *dest_menu)
{
    free_menu(dest_menu);   /* toss old menu */
    *dest_menu = *src_menu; /* make new menu current */
    /* leave no dangling ptrs */
    reset_menu_to_default(src_menu);
}

static void
free_menu_line_entries(struct menu *mp)
{
    /* We're not freeing menu entry widgets here, but let XtDestroyWidget()
       on the parent widget take care of that */
    while (mp->base) {
        mp->last = mp->base;
        mp->base = mp->base->next;
        free((genericptr_t) mp->last->str);
        free((genericptr_t) mp->last);
    }
}

static void
free_menu(struct menu *mp)
{
    free_menu_line_entries(mp);
    if (mp->query)
        free((genericptr_t) mp->query);
    if (mp->gacc)
        free((genericptr_t) mp->gacc);
    reset_menu_to_default(mp);
}

static void
reset_menu_to_default(struct menu *mp)
{
    mp->base = mp->last = (x11_menu_item *) 0;
    mp->query = (const char *) 0;
    mp->gacc = (const char *) 0;
    mp->count = 0;
    mp->curr_selector = 'a'; /* first accelerator */
}

static void
clear_old_menu(struct xwindow *wp)
{
    struct menu_info_t *menu_info = wp->menu_information;

    free_menu(&menu_info->curr_menu);
    free_menu(&menu_info->new_menu);

    if (menu_info->is_up) {
        nh_XtPopdown(wp->popup);
        menu_info->is_up = FALSE;
        XtDestroyWidget(wp->popup);
        wp->w = wp->popup = (Widget) 0;
    }
}

void
create_menu_window(struct xwindow *wp)
{
    wp->type = NHW_MENU;
    wp->menu_information =
        (struct menu_info_t *) alloc(sizeof(struct menu_info_t));
    (void) memset((genericptr_t) wp->menu_information, '\0',
                  sizeof(struct menu_info_t));
    reset_menu_to_default(&wp->menu_information->curr_menu);
    reset_menu_to_default(&wp->menu_information->new_menu);
    reset_menu_count(wp->menu_information);
    wp->w = wp->popup = (Widget) 0;
    wp->menu_information->permi_x = -1;
    wp->menu_information->permi_y = -1;
    wp->menu_information->permi_w = -1;
    wp->menu_information->permi_h = -1;
}

void
destroy_menu_window(struct xwindow *wp)
{
    clear_old_menu(wp); /* this will also destroy the widgets */
    free((genericptr_t) wp->menu_information);
    wp->menu_information = (struct menu_info_t *) 0;
    wp->type = NHW_NONE; /* allow re-use */
}

/*winmenu.c*/

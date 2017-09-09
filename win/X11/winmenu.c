/* NetHack 3.6	winmenu.c	$NHDT-Date: 1453448854 2016/01/22 07:47:34 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.13 $ */
/* Copyright (c) Dean Luick, 1992				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * File for creating menus.
 *
 *	+ Global functions: start_menu, add_menu, end_menu, select_menu
 */
/*#define USE_FWF*/ /* use FWF's list widget */

#ifndef SYSV
#define PRESERVE_NO_SYSV /* X11 include files may define SYSV */
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Box.h>
#ifdef USE_FWF
#include <X11/Xfwf/MultiList.h>
#else
#include <X11/Xaw/List.h>
#endif
#include <X11/Xos.h>

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#include "hack.h"
#include "winX.h"

static void menu_select(Widget, XtPointer, XtPointer);
static void invert_line(struct xwindow *, x11_menu_item *, int, long);
static void menu_ok(Widget, XtPointer, XtPointer);
static void menu_cancel(Widget, XtPointer, XtPointer);
static void menu_all(Widget, XtPointer, XtPointer);
static void menu_none(Widget, XtPointer, XtPointer);
static void menu_invert(Widget, XtPointer, XtPointer);
static void menu_search(Widget, XtPointer, XtPointer);
static void select_all(struct xwindow *);
static void select_none(struct xwindow *);
static void select_match(struct xwindow *, char *);
static void invert_all(struct xwindow *);
static void invert_match(struct xwindow *, char *);
static void menu_popdown(struct xwindow *);
#ifdef USE_FWF
static void sync_selected(struct menu_info_t *, int, int *);
#endif

static void move_menu(struct menu *, struct menu *);
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
     <Key>: menu_key()";

/*
 * Menu callback.
 */
/* ARGSUSED */
static void
menu_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    struct xwindow *wp;
    struct menu_info_t *menu_info;
#ifdef USE_FWF
    XfwfMultiListReturnStruct *lrs = (XfwfMultiListReturnStruct *) call_data;
#else
    XawListReturnStruct *lrs = (XawListReturnStruct *) call_data;
    int i;
    x11_menu_item *curr;
#endif
    long how_many;

    nhUse(client_data);

    wp = find_widget(w);
    menu_info = wp->menu_information;
    how_many = menu_info->counting ? menu_info->menu_count : -1L;
    reset_menu_count(menu_info);

#ifdef USE_FWF
    /* if we've reached here, we've found our selected item */
    switch (lrs->action) {
    case XfwfMultiListActionNothing:
        pline("menu_select: nothing action?");
        break;
    case XfwfMultiListActionStatus:
        pline("menu_select: status action?");
        break;
    case XfwfMultiListActionHighlight:
    case XfwfMultiListActionUnhighlight:
        sync_selected(menu_info, lrs->num_selected, lrs->selected_items);
        break;
    }
#else
    for (i = 0, curr = menu_info->curr_menu.base; i < lrs->list_index; i++) {
        if (!curr)
            panic("menu_select: out of menu items!");
        curr = curr->next;
    }
    XawListUnhighlight(w); /* unhilight item */

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
    XawListChange(wp->w, menu_info->curr_menu.list_pointer, 0, 0, True);
#endif

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
#ifndef USE_FWF
    nhUse(which);
#endif
    reset_menu_count(wp->menu_information);
    /* invert selection unless explicitly choosing the preselected
       entry of a PICK_ONE menu */
    if (wp->menu_information->how != PICK_ONE || !curr->preselected)
        curr->selected = !curr->selected;
    curr->preselected = FALSE;
    if (curr->selected) {
#ifdef USE_FWF
        XfwfMultiListHighlightItem((XfwfMultiListWidget) wp->w, which);
#else
        curr->str[2] = (how_many != -1) ? '#' : '+';
#endif
        curr->pick_count = how_many;
    } else {
#ifdef USE_FWF
        XfwfMultiListUnhighlightItem((XfwfMultiListWidget) wp->w, which);
#else
        curr->str[2] = '-';
#endif
        curr->pick_count = -1L;
    }
}

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
    char ch;
    int count;
    boolean selected_something;

    nhUse(params);
    nhUse(num_params);

    wp = find_widget(w);
    menu_info = wp->menu_information;

    ch = key_event_to_char((XKeyEvent *) event);

    if (ch == '\0') { /* don't accept nul char/modifier event */
        /* don't beep */
        return;
    }

    if (menu_info->is_active) { /* waiting for input */
        /* first check for an explicit selector match, so that it won't be
           overridden if it happens to duplicate a mapped menu command (':'
           to look inside a container vs ':' to select via search string) */
        for (curr = menu_info->curr_menu.base; curr; curr = curr->next)
            if (curr->identifier.a_void != 0 && curr->selector == ch)
                goto make_selection;

        ch = map_menu_cmd(ch);
        if (ch == '\033') { /* quit */
            if (menu_info->counting) {
                /* when there's a count in progress, ESC discards it
                   rather than dismissing the whole menu */
                reset_menu_count(menu_info);
                return;
            }
            select_none(wp);
        } else if (ch == '\n' || ch == '\r') {
            ; /* accept */
        } else if (digit(ch)) {
            /* special case: '0' is also the default ball class */
            if (ch == '0' && !menu_info->counting
                && index(menu_info->curr_menu.gacc, ch))
                goto group_accel;
            menu_info->menu_count *= 10L;
            menu_info->menu_count += (long) (ch - '0');
            if (menu_info->menu_count != 0L) /* ignore leading zeros */
                menu_info->counting = TRUE;
            return;
        } else if (ch == MENU_SEARCH) { /* search */
            if (menu_info->how == PICK_ANY || menu_info->how == PICK_ONE) {
                char buf[BUFSZ + 2], tmpbuf[BUFSZ];

                X11_getlin("Search for:", tmpbuf);
                if (!*tmpbuf || *tmpbuf == '\033')
                    return;
                /* convert "string" into "*string*" for use with pmatch() */
                Sprintf(buf, "*%s*", tmpbuf);

                if (menu_info->how == PICK_ANY) {
                    invert_match(wp, buf);
                    return;
                } else {
                    select_match(wp, buf);
                }
            } else {
                X11_nhbell();
                return;
            }
        } else if (ch == MENU_SELECT_ALL) { /* select all */
            if (menu_info->how == PICK_ANY)
                select_all(wp);
            else
                X11_nhbell();
            return;
        } else if (ch == MENU_UNSELECT_ALL) { /* unselect all */
            if (menu_info->how == PICK_ANY)
                select_none(wp);
            else
                X11_nhbell();
            return;
        } else if (ch == MENU_INVERT_ALL) { /* invert all */
            if (menu_info->how == PICK_ANY)
                invert_all(wp);
            else
                X11_nhbell();
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
#ifndef USE_FWF
                XawListChange(wp->w, menu_info->curr_menu.list_pointer, 0, 0,
                              True);
#endif
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
#ifndef USE_FWF
                XawListChange(wp->w, menu_info->curr_menu.list_pointer, 0, 0,
                              True);
#endif
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
            XtPointer client_data,
            XtPointer call_data)
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
    char buf[BUFSZ + 2], tmpbuf[BUFSZ];

    nhUse(w);
    nhUse(call_data);

    X11_getlin("Search for:", tmpbuf);
    if (!*tmpbuf || *tmpbuf == '\033')
        return;
    /* convert "string" into "*string*" for use with pmatch() */
    Sprintf(buf, "*%s*", tmpbuf);

    if (menu_info->how == PICK_ANY)
        invert_match(wp, buf);
    else
        select_match(wp, buf);

    if (menu_info->how == PICK_ONE)
        menu_popdown(wp);
}

static void
select_all(struct xwindow *wp)
{
    x11_menu_item *curr;
    int count;
    boolean changed = FALSE;

    reset_menu_count(wp->menu_information);
    for (count = 0, curr = wp->menu_information->curr_menu.base; curr;
         curr = curr->next, count++)
        if (curr->identifier.a_void != 0)
            if (!curr->selected) {
                invert_line(wp, curr, count, -1L);
                changed = TRUE;
            }

#ifndef USE_FWF
    if (changed)
        XawListChange(wp->w, wp->menu_information->curr_menu.list_pointer,
                      0, 0, True);
#endif
}

static void
select_none(struct xwindow *wp)
{
    x11_menu_item *curr;
    int count;
    boolean changed = FALSE;

    reset_menu_count(wp->menu_information);
    for (count = 0, curr = wp->menu_information->curr_menu.base; curr;
         curr = curr->next, count++)
        if (curr->identifier.a_void != 0)
            if (curr->selected) {
                invert_line(wp, curr, count, -1L);
                changed = TRUE;
            }

#ifndef USE_FWF
    if (changed)
        XawListChange(wp->w, wp->menu_information->curr_menu.list_pointer,
                      0, 0, True);
#endif
}

static void
invert_all(struct xwindow *wp)
{
    x11_menu_item *curr;
    int count;

    reset_menu_count(wp->menu_information);
    for (count = 0, curr = wp->menu_information->curr_menu.base; curr;
         curr = curr->next, count++)
        if (curr->identifier.a_void != 0)
            invert_line(wp, curr, count, -1L);

#ifndef USE_FWF
    XawListChange(wp->w, wp->menu_information->curr_menu.list_pointer,
                  0, 0, True);
#endif
}

static void
invert_match(struct xwindow *wp,
             char *match) /* wildcard pattern for pmatch() */
{
    x11_menu_item *curr;
    int count;
    boolean changed = FALSE;

    reset_menu_count(wp->menu_information);
    for (count = 0, curr = wp->menu_information->curr_menu.base; curr;
         curr = curr->next, count++)
        if (curr->identifier.a_void != 0) {
            if (pmatchi(match, curr->str)) {
                invert_line(wp, curr, count, -1L);
                changed = TRUE;
            }
            curr->preselected = FALSE;
        }

#ifndef USE_FWF
    if (changed)
        XawListChange(wp->w, wp->menu_information->curr_menu.list_pointer,
                      0, 0, True);
#endif
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
#ifndef USE_FWF
            XawListChange(wp->w, wp->menu_information->curr_menu.list_pointer,
                          0, 0, True);
#endif
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
    if (wp->menu_information->is_active)
        exit_x_event = TRUE;             /* exit our event handler */
    wp->menu_information->is_up = FALSE; /* menu is down */
}

#ifdef USE_FWF
/*
 * Make sure our idea of selected matches the FWF Multilist's idea of what
 * is currently selected.  The MultiList's selected list can change without
 * notifying us if one or more items are selected and then another is
 * selected (not toggled).  Then the items that were selected are deselected
 * but we are not notified.
 */
static void
sync_selected(struct menu_info_t *menu_info, int num_selected, int *items)
{
    int i, j, *ip;
    x11_menu_item *curr;
    Boolean found;

    for (i = 0, curr = menu_info->curr_menu.base; curr;
         i++, curr = curr->next) {
        found = False;
        for (j = 0, ip = items; j < num_selected; j++, ip++)
            if (*ip == i) {
                found = True;
                break;
            }
#if 0
	if (curr->selected && !found)
	    printf("sync: deselecting %s\n", curr->str);
	else if (!curr->selected && found)
	    printf("sync: selecting %s\n", curr->str);
#endif
        curr->selected = found ? TRUE : FALSE;
        /* once active selection takes place, preselection becomes history */
        curr->preselected = FALSE;
    }
}
#endif /* USE_FWF */

/* Global functions ========================================================
 */

void
X11_start_menu(winid window)
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
             int glyph, /* unused (for now) */
             const anything *identifier,
             char ch,
             char gch, /* group accelerator (0 = no group) */
             int attr,
             const char *str,
             boolean preselected)
{
    x11_menu_item *item;
    struct menu_info_t *menu_info;

    nhUse(glyph);

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
    item->selected = item->preselected = preselected;
    item->pick_count = -1L;

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
    String *ptr;
    int retval;
    Dimension v_pixel_width, v_pixel_height, lblwidth[6], maxlblwidth;
    boolean labeled;
    Widget viewport_widget, form, label,
           ok, cancel, all, none, invert, search, lblwidget[6];
    Boolean sens;
#ifdef USE_FWF
    Boolean *boolp;
#endif
    char gacc[QBUFSZ], *ap;

    *menu_list = (menu_item *) 0;
    check_winid(window);
    wp = &window_list[window];
    menu_info = wp->menu_information;
    if (!menu_info->is_menu) {
        impossible("select_menu:  called before start_menu");
        return 0;
    }

    menu_info->how = (short) how;

    /* collect group accelerators; for PICK_NONE, they're ignored;
       for PICK_ONE, only those which match exactly one entry will be
       accepted; for PICK_ANY, those which match any entry are okay */
    gacc[0] = '\0';
    if (menu_info->how != PICK_NONE) {
        int i, n, gcnt[128];
#define GSELIDX(c) ((c) &127) /* guard against `signed char' */

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

    /*
     * Create a string and sensitive list for the new menu.
     */
    menu_info->new_menu.list_pointer = ptr = (String *) alloc(
        (unsigned) (sizeof(String) * (menu_info->new_menu.count + 1)));
    for (curr = menu_info->new_menu.base; curr; ptr++, curr = curr->next)
        *ptr = (String) curr->str;
    *ptr = 0; /* terminate list with null */

#ifdef USE_FWF
    menu_info->new_menu.sensitive = boolp = (Boolean *) alloc(
        (unsigned) (sizeof(Boolean) * (menu_info->new_menu.count)));
    for (curr = menu_info->new_menu.base; curr; boolp++, curr = curr->next)
        *boolp = (curr->identifier.a_void != 0);
#else
    menu_info->new_menu.sensitive = (Boolean *) 0;
#endif
    labeled = (menu_info->new_menu.query && *(menu_info->new_menu.query))
                  ? TRUE
                  : FALSE;

    /*
     * Menus don't appear to size components correctly, except
     * when first created.  For 3.2.0 release, just recreate
     * each time.
     */
    if (menu_info->valid_widgets
        && (window != WIN_INVEN || !flags.perm_invent)) {
        XtDestroyWidget(wp->popup);
        menu_info->valid_widgets = FALSE;
        menu_info->is_up = FALSE;
    }

    if (!menu_info->valid_widgets) {
        Dimension row_spacing;

        num_args = 0;
        XtSetArg(args[num_args], XtNallowShellResize, True);
        num_args++;
        wp->popup = XtCreatePopupShell((window == WIN_INVEN)
                                           ? "inventory" : "menu",
                                       (how == PICK_NONE)
                                           ? topLevelShellWidgetClass
                                           : transientShellWidgetClass,
                                       toplevel, args, num_args);
        XtOverrideTranslations(wp->popup,
                               XtParseTranslationTable(
                                     "<Message>WM_PROTOCOLS: menu_delete()"));

        num_args = 0;
        XtSetArg(args[num_args], XtNtranslations,
                 XtParseTranslationTable(menu_translations)); num_args++;
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

        /*
         * Create ok, cancel, all, none, invert, and search buttons..
         */
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
        XtSetArg(args[num_args], nhStr(XtNsensitive), how != PICK_NONE);
                                                                   num_args++;
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

        num_args = 0;
        XtSetArg(args[num_args], nhStr(XtNallowVert), True); num_args++;
        XtSetArg(args[num_args], nhStr(XtNallowHoriz), False); num_args++;
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
        viewport_widget = XtCreateManagedWidget(
            "menu_viewport",           /* name */
            viewportWidgetClass, form, /* parent widget */
            args, num_args);           /* values, and number of values */

        /* make new menu the current menu */
        move_menu(&menu_info->new_menu, &menu_info->curr_menu);

        num_args = 0;
        XtSetArg(args[num_args], nhStr(XtNforceColumns), True); num_args++;
        XtSetArg(args[num_args], nhStr(XtNcolumnSpacing), 1); num_args++;
        XtSetArg(args[num_args], nhStr(XtNdefaultColumns), 1); num_args++;
        XtSetArg(args[num_args], nhStr(XtNlist),
                 menu_info->curr_menu.list_pointer); num_args++;
#ifdef USE_FWF
        XtSetArg(args[num_args], nhStr(XtNsensitiveArray),
                 menu_info->curr_menu.sensitive); num_args++;
        XtSetArg(args[num_args], nhStr(XtNmaxSelectable),
                 menu_info->curr_menu.count); num_args++;
#endif
        wp->w = XtCreateManagedWidget("menu_list", /* name */
#ifdef USE_FWF
                                      xfwfMultiListWidgetClass,
#else
                                      listWidgetClass,
#endif
                                      viewport_widget, /* parent widget */
                                      args,            /* set some values */
                                      num_args); /* number of values to set */

        XtAddCallback(wp->w, XtNcallback, menu_select, (XtPointer) 0);

        /* Get the font and margin information. */
        num_args = 0;
        XtSetArg(args[num_args], XtNfont, &menu_info->fs); num_args++;
        XtSetArg(args[num_args], XtNinternalHeight,
                 &menu_info->internal_height); num_args++;
        XtSetArg(args[num_args], XtNinternalWidth,
                 &menu_info->internal_width); num_args++;
        XtSetArg(args[num_args], nhStr(XtNrowSpacing), &row_spacing);
                                                                   num_args++;
        XtGetValues(wp->w, args, num_args);

        /* font height is ascent + descent */
        menu_info->line_height = menu_info->fs->max_bounds.ascent
                                 + menu_info->fs->max_bounds.descent
                                 + row_spacing;

        menu_info->valid_widgets = TRUE;

        num_args = 0;
        XtSetArg(args[num_args], XtNwidth, &v_pixel_width); num_args++;
        XtSetArg(args[num_args], XtNheight, &v_pixel_height); num_args++;
        XtGetValues(wp->w, args, num_args);
    } else {
        Dimension len;

        viewport_widget = XtParent(wp->w);

        /* get the longest string on new menu */
        v_pixel_width = 0;
        for (ptr = menu_info->new_menu.list_pointer; *ptr; ptr++) {
            len = XTextWidth(menu_info->fs, *ptr, strlen(*ptr));
            if (len > v_pixel_width)
                v_pixel_width = len;
        }

        /* add viewport internal border */
        v_pixel_width += 2 * menu_info->internal_width;
        v_pixel_height = (2 * menu_info->internal_height)
                       + (menu_info->new_menu.count * menu_info->line_height);

        /* make new menu the current menu */
        move_menu(&menu_info->new_menu, &menu_info->curr_menu);
#ifdef USE_FWF
        XfwfMultiListSetNewData((XfwfMultiListWidget) wp->w,
                                menu_info->curr_menu.list_pointer, 0, 0, TRUE,
                                menu_info->curr_menu.sensitive);
#else
        XawListChange(wp->w, menu_info->curr_menu.list_pointer, 0, 0, TRUE);
#endif
    }

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
    if (!menu_info->is_up)
        positionpopup(wp->popup, FALSE);

    menu_info->is_up = TRUE;
    if (window == WIN_INVEN && how == PICK_NONE) {
        /* cant use nh_XtPopup() because it may try to grab the focus */
        XtPopup(wp->popup, (int) XtGrabNone);
        if (!updated_inventory)
            XMapRaised(XtDisplay(wp->popup), XtWindow(wp->popup));
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

/* End global functions ====================================================
 */

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

static void
move_menu(struct menu *src_menu, struct menu *dest_menu)
{
    free_menu(dest_menu);   /* toss old menu */
    *dest_menu = *src_menu; /* make new menu current */
    /* leave no dangling ptrs */
    reset_menu_to_default(src_menu);
}

static void
free_menu(struct menu *mp)
{
    while (mp->base) {
        mp->last = mp->base;
        mp->base = mp->base->next;

        free((genericptr_t) mp->last->str);
        free((genericptr_t) mp->last);
    }
    if (mp->query)
        free((genericptr_t) mp->query);
    if (mp->gacc)
        free((genericptr_t) mp->gacc);
    if (mp->list_pointer)
        free((genericptr_t) mp->list_pointer);
    if (mp->sensitive)
        free((genericptr_t) mp->sensitive);
    reset_menu_to_default(mp);
}

static void
reset_menu_to_default(struct menu *mp)
{
    mp->base = mp->last = (x11_menu_item *) 0;
    mp->query = (const char *) 0;
    mp->gacc = (const char *) 0;
    mp->count = 0;
    mp->list_pointer = (String *) 0;
    mp->sensitive = (Boolean *) 0;
    mp->curr_selector = 'a'; /* first accelerator */
}

static void
clear_old_menu(struct xwindow *wp)
{
    struct menu_info_t *menu_info = wp->menu_information;

    free_menu(&menu_info->curr_menu);
    free_menu(&menu_info->new_menu);

    if (menu_info->valid_widgets) {
        nh_XtPopdown(wp->popup);
        menu_info->is_up = FALSE;
        XtDestroyWidget(wp->popup);
        menu_info->valid_widgets = FALSE;
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

/* NetHack 3.7	winmesg.c	$NHDT-Date: 1596498373 2020/08/03 23:46:13 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Dean Luick, 1992                                 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Message window routines.
 *
 * Global functions:
 *      create_message_window()
 *      destroy_message_window()
 *      display_message_window()
 *      append_message()
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV /* X11 include files may define SYSV */
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xatom.h>

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#include "xwindow.h" /* Window widget declarations */

#include "hack.h"
#include "winX.h"

static struct line_element *get_previous(struct line_element *);
static void set_circle_buf(struct mesg_info_t *, int);
static char *split(char *, XFontStruct *, Dimension);
static void add_line(struct mesg_info_t *, const char *);
static void redraw_message_window(struct xwindow *);
static void mesg_check_size_change(struct xwindow *);
static void mesg_exposed(Widget, XtPointer, XtPointer);
static void get_gc(Widget, struct mesg_info_t *);
static void mesg_resized(Widget, XtPointer, XtPointer);

static char mesg_translations[] = "#override\n\
 <Key>Left:     scroll(4)\n\
 <Key>Right:    scroll(6)\n\
 <Key>Up:       scroll(8)\n\
 <Key>Down:     scroll(2)\n\
 <Btn4Down>:    scroll(8)\n\
 <Btn5Down>:    scroll(2)\n\
 <Key>:         input()";

/* Move the message window's vertical scrollbar's slider to the bottom. */
void
set_message_slider(struct xwindow *wp)
{
    Widget scrollbar;
    float top;

    scrollbar = XtNameToWidget(XtParent(wp->w), "vertical");

    if (scrollbar) {
        top = 1.0;
        XtCallCallbacks(scrollbar, XtNjumpProc, &top);
    }
}

void
create_message_window(struct xwindow *wp, /* window pointer */
                      boolean create_popup, Widget parent)
{
    Arg args[8];
    Cardinal num_args;
    Widget viewport;
    struct mesg_info_t *mesg_info;

    wp->type = NHW_MESSAGE;

    wp->mesg_information = mesg_info =
        (struct mesg_info_t *) alloc(sizeof (struct mesg_info_t));

    mesg_info->fs = 0;
    mesg_info->num_lines = 0;
    mesg_info->head = mesg_info->line_here = mesg_info->last_pause =
        mesg_info->last_pause_head = (struct line_element *) 0;
    mesg_info->dirty = False;
    mesg_info->viewport_width = mesg_info->viewport_height = 0;

    if (iflags.msg_history < (unsigned) appResources.message_lines)
        iflags.msg_history = (unsigned) appResources.message_lines;
    if (iflags.msg_history > MAX_HISTORY) /* a sanity check */
        iflags.msg_history = MAX_HISTORY;

    set_circle_buf(mesg_info, (int) iflags.msg_history);

    /* Create a popup that becomes the parent. */
    if (create_popup) {
        num_args = 0;
        XtSetArg(args[num_args], XtNallowShellResize, True);
        num_args++;

        wp->popup = parent =
            XtCreatePopupShell("message_popup", topLevelShellWidgetClass,
                               toplevel, args, num_args);
        /*
         * If we're here, then this is an auxiliary message window.  If we're
         * cancelled via a delete window message, we should just pop down.
         */
    }

    /*
     * Create the viewport.  We only want the vertical scroll bar ever to be
     * visible.  If we allow the horizontal scrollbar to be visible it will
     * always be visible, due to the stupid way the Athena viewport operates.
     */
    num_args = 0;
    XtSetArg(args[num_args], XtNallowVert, True);
    num_args++;
    viewport = XtCreateManagedWidget(
        "mesg_viewport",     /* name */
        viewportWidgetClass, /* widget class from Window.h */
        parent,              /* parent widget */
        args,                /* set some values */
        num_args);           /* number of values to set */

    /*
     * Create a message window.  We will change the width and height once
     * we know what font we are using.
     */
    num_args = 0;
    if (!create_popup) {
        XtSetArg(args[num_args], XtNtranslations,
                 XtParseTranslationTable(mesg_translations));
        num_args++;
    }
    wp->w = XtCreateManagedWidget(
        "message",         /* name */
        windowWidgetClass, /* widget class from Window.h */
        viewport,          /* parent widget */
        args,              /* set some values */
        num_args);         /* number of values to set */

    XtAddCallback(wp->w, XtNexposeCallback, mesg_exposed, (XtPointer) 0);

    /*
     * Now adjust the height and width of the message window so that it
     * is appResources.message_lines high and DEFAULT_MESSAGE_WIDTH wide.
     */

    /* Get the font information. */
    num_args = 0;
    XtSetArg(args[num_args], XtNfont, &mesg_info->fs);
    num_args++;
    XtGetValues(wp->w, args, num_args);

    /* Save character information for fast use later. */
    mesg_info->char_width = mesg_info->fs->max_bounds.width;
    mesg_info->char_height =
        mesg_info->fs->max_bounds.ascent + mesg_info->fs->max_bounds.descent;
    mesg_info->char_ascent = mesg_info->fs->max_bounds.ascent;
    mesg_info->char_lbearing = -mesg_info->fs->min_bounds.lbearing;

    get_gc(wp->w, mesg_info);

    wp->pixel_height = ((int) iflags.msg_history) * mesg_info->char_height;

    /* If a variable spaced font, only use 2/3 of the default size */
    if (mesg_info->fs->min_bounds.width != mesg_info->fs->max_bounds.width) {
        wp->pixel_width = ((2 * DEFAULT_MESSAGE_WIDTH) / 3)
                          * mesg_info->fs->max_bounds.width;
    } else
        wp->pixel_width =
            (DEFAULT_MESSAGE_WIDTH * mesg_info->fs->max_bounds.width);

    /* Set the new width and height. */
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, wp->pixel_width);
    num_args++;
    XtSetArg(args[num_args], XtNheight, wp->pixel_height);
    num_args++;
    XtSetValues(wp->w, args, num_args);

    /* make sure viewport height makes sense before realizing it */
    num_args = 0;
    mesg_info->viewport_height =
        appResources.message_lines * mesg_info->char_height;
    XtSetArg(args[num_args], XtNheight, mesg_info->viewport_height);
    num_args++;
    XtSetValues(viewport, args, num_args);

    XtAddCallback(wp->w, XtNresizeCallback, mesg_resized, (XtPointer) 0);

    /*
     * If we have created our own popup, then realize it so that the
     * viewport is also realized.
     */
    if (create_popup) {
        XtRealizeWidget(wp->popup);
        XSetWMProtocols(XtDisplay(wp->popup), XtWindow(wp->popup),
                        &wm_delete_window, 1);
    }
}

void
destroy_message_window(struct xwindow *wp)
{
    if (wp->popup) {
        nh_XtPopdown(wp->popup);
        if (!wp->keep_window)
            XtDestroyWidget(wp->popup), wp->popup = (Widget) 0;
    }
    if (wp->mesg_information) {
        set_circle_buf(wp->mesg_information, 0); /* free buffer list */
        free((genericptr_t) wp->mesg_information), wp->mesg_information = 0;
    }
    if (wp->keep_window)
        XtRemoveCallback(wp->w, XtNexposeCallback, mesg_exposed,
                         (XtPointer) 0);
    else
        wp->type = NHW_NONE;
}

/* Redraw message window if new lines have been added. */
void
display_message_window(struct xwindow *wp)
{
    set_message_slider(wp);
    if (wp->mesg_information->dirty)
        redraw_message_window(wp);
}

/*
 * Append a line of text to the message window.  Split the line if the
 * rendering of the text is too long for the window.
 */
void
append_message(struct xwindow *wp, const char *str)
{
    char *mark, *remainder, buf[BUFSZ];

    if (!str)
        return;

    Strcpy(buf, str); /* we might mark it up */

    remainder = buf;
    do {
        mark = remainder;
        remainder = split(mark, wp->mesg_information->fs, wp->pixel_width);
        add_line(wp->mesg_information, mark);
    } while (remainder);
}

/* private functions =======================================================
 */

/*
 * Return the element in the circular linked list just before the given
 * element.
 */
static struct line_element *
get_previous(struct line_element *mark)
{
    struct line_element *curr;

    if (!mark)
        return (struct line_element *) 0;

    for (curr = mark; curr->next != mark; curr = curr->next)
        ;
    return curr;
}

/*
 * Set the information buffer size to count lines.  We do this by creating
 * a circular linked list of elements, each of which represents a line of
 * text.  New buffers are created as needed, old ones are freed if they
 * are no longer used.
 */
static void
set_circle_buf(struct mesg_info_t *mesg_info, int count)
{
    int i;
    struct line_element *tail, *curr, *head;

    if (count < 0)
        panic("set_circle_buf: bad count [= %d]", count);
    if (count == mesg_info->num_lines)
        return; /* no change in size */

    if (count < mesg_info->num_lines) {
        /*
         * Toss num_lines - count line entries from our circular list.
         *
         * We lose lines from the front (top) of the list.  We _know_
         * the list is non_empty.
         */
        tail = get_previous(mesg_info->head);
        for (i = mesg_info->num_lines - count; i > 0; i--) {
            curr = mesg_info->head;
            mesg_info->head = curr->next;
            if (curr->line)
                free((genericptr_t) curr->line);
            free((genericptr_t) curr);
        }
        if (count == 0) {
            /* make sure we don't have a dangling pointer */
            mesg_info->head = (struct line_element *) 0;
        } else {
            tail->next = mesg_info->head; /* link the tail to the head */
        }
    } else {
        /*
         * Add count - num_lines blank lines to the head of the list.
         *
         * Create a separate list, keeping track of the tail.
         */
        for (head = tail = 0, i = 0; i < count - mesg_info->num_lines; i++) {
            curr = (struct line_element *) alloc(sizeof(struct line_element));
            curr->line = 0;
            curr->buf_length = 0;
            curr->str_length = 0;
            if (tail) {
                tail->next = curr;
                tail = curr;
            } else {
                head = tail = curr;
            }
        }
        /*
         * Complete the circle by making the new tail point to the old head
         * and the old tail point to the new head.  If our line count was
         * zero, then make the new list circular.
         */
        if (mesg_info->num_lines) {
            curr = get_previous(mesg_info->head); /* get end of old list */

            tail->next = mesg_info->head; /* new tail -> old head */
            curr->next = head;            /* old tail -> new head */
        } else {
            tail->next = head;
        }
        mesg_info->head = head;
    }

    mesg_info->num_lines = count;
    /* Erase the line on a resize. */
    mesg_info->last_pause = (struct line_element *) 0;
}

/*
 * Make sure the given string is shorter than the given pixel width.  If
 * not, back up from the end by words until we find a place to split.
 */
static char *
split(char *s,
      XFontStruct *fs, /* Font for the window. */
      Dimension pixel_width)
{
    char save, *end, *remainder;

    save = '\0';
    remainder = 0;
    end = eos(s); /* point to null at end of string */

    /* assume that if end == s, XXXXXX returns 0) */
    while ((Dimension) XTextWidth(fs, s, (int) strlen(s)) > pixel_width) {
        *end-- = save;
        while (*end != ' ') {
            if (end == s)
                panic("split: eos!");
            --end;
        }
        save = *end;
        *end = '\0';
        remainder = end + 1;
    }
    return remainder;
}

/*
 * Add a line of text to the window.  The first line in the curcular list
 * becomes the last.  So all we have to do is copy the new line over the
 * old one.  If the line buffer is too small, then allocate a new, larger
 * one.
 */
static void
add_line(struct mesg_info_t *mesg_info, const char *s)
{
    register struct line_element *curr = mesg_info->head;
    register int new_line_length = strlen(s);

    if (new_line_length + 1 > curr->buf_length) {
        if (curr->line)
            free(curr->line); /* free old line */

        curr->buf_length = new_line_length + 1;
        curr->line = (char *) alloc((unsigned) curr->buf_length);
    }

    Strcpy(curr->line, s);              /* copy info */
    curr->str_length = new_line_length; /* save string length */

    mesg_info->head = mesg_info->head->next; /* move head to next line */
    mesg_info->dirty = True;                 /* we have undrawn lines */
}

/*
 * Save a position in the text buffer so we can draw a line to seperate
 * text from the last time this function was called.
 *
 * Save the head position, since it is the line "after" the last displayed
 * line in the message window.  The window redraw routine will draw a
 * line above this saved pointer.
 */
void
set_last_pause(struct xwindow *wp)
{
    register struct mesg_info_t *mesg_info = wp->mesg_information;

#ifdef ERASE_LINE
    /*
     * If we've erased the pause line and haven't added any new lines,
     * don't try to erase the line again.
     */
    if (!mesg_info->last_pause
        && mesg_info->last_pause_head == mesg_info->head)
        return;

    if (mesg_info->last_pause == mesg_info->head) {
        /* No new messages in last turn.  Redraw window to erase line. */
        mesg_info->last_pause = (struct line_element *) 0;
        mesg_info->last_pause_head = mesg_info->head;
        redraw_message_window(wp);
    } else {
#endif
        mesg_info->last_pause = mesg_info->head;
#ifdef ERASE_LINE
    }
#endif
}

static void
redraw_message_window(struct xwindow *wp)
{
    struct mesg_info_t *mesg_info = wp->mesg_information;
    register struct line_element *curr;
    register int row, y_base;

    /*
     * Do this the cheap and easy way.  Clear the window and just redraw
     * the whole thing.
     *
     * This could be done more effecently with one call to XDrawText() instead
     * of many calls to XDrawString().  Maybe later.
     *
     * Only need to clear if window has new text.
     */
    if (mesg_info->dirty) {
        XClearWindow(XtDisplay(wp->w), XtWindow(wp->w));
        mesg_info->line_here = mesg_info->last_pause;
    }

    /* For now, just update the whole shootn' match. */
    for (y_base = row = 0, curr = mesg_info->head; row < mesg_info->num_lines;
         row++, y_base += mesg_info->char_height, curr = curr->next) {
        XDrawString(XtDisplay(wp->w), XtWindow(wp->w), mesg_info->gc,
                    mesg_info->char_lbearing, mesg_info->char_ascent + y_base,
                    curr->line, curr->str_length);
        /*
         * This draws a line at the _top_ of the line of text pointed to by
         * mesg_info->last_pause.
         */
        if (appResources.message_line && curr == mesg_info->line_here) {
            XDrawLine(XtDisplay(wp->w), XtWindow(wp->w), mesg_info->gc, 0,
                      y_base, wp->pixel_width, y_base);
        }
    }

    mesg_info->dirty = False;
}

/*
 * Check the size of the viewport.  If it has shrunk, then we want to
 * move the vertical slider to the bottom.
 */
static void
mesg_check_size_change(struct xwindow *wp)
{
    struct mesg_info_t *mesg_info = wp->mesg_information;
    Arg arg[2];
    Dimension new_width, new_height;
    Widget viewport;

    viewport = XtParent(wp->w);

    XtSetArg(arg[0], XtNwidth, &new_width);
    XtSetArg(arg[1], XtNheight, &new_height);
    XtGetValues(viewport, arg, TWO);

    /* Only move slider to bottom if new size is smaller. */
    if (new_width < mesg_info->viewport_width
        || new_height < mesg_info->viewport_height) {
        set_message_slider(wp);
    }

    mesg_info->viewport_width = new_width;
    mesg_info->viewport_height = new_height;
}

/* Event handler for message window expose events. */
/*ARGSUSED*/
static void
mesg_exposed(Widget w,
             XtPointer client_data, /* unused */
             XtPointer widget_data) /* expose event from Window widget */
{
    XExposeEvent *event = (XExposeEvent *) widget_data;

    nhUse(client_data);

    if (XtIsRealized(w) && event->count == 0) {
        struct xwindow *wp;
        Display *dpy;
        Window win;
        XEvent evt;

        /*
         * Drain all pending expose events for the message window;
         * we'll redraw the whole thing at once.
         */
        dpy = XtDisplay(w);
        win = XtWindow(w);
        while (XCheckTypedWindowEvent(dpy, win, Expose, &evt))
            continue;

        wp = find_widget(w);
        if (wp->keep_window && !wp->mesg_information)
            return;
        mesg_check_size_change(wp);
        redraw_message_window(wp);
    }
}

static void
get_gc(Widget w, struct mesg_info_t *mesg_info)
{
    XGCValues values;
    XtGCMask mask = GCFunction | GCForeground | GCBackground | GCFont;
    Pixel fgpixel, bgpixel;
    Arg arg[2];

    XtSetArg(arg[0], XtNforeground, &fgpixel);
    XtSetArg(arg[1], XtNbackground, &bgpixel);
    XtGetValues(w, arg, TWO);

    values.foreground = fgpixel;
    values.background = bgpixel;
    values.function = GXcopy;
    values.font = WindowFont(w);
    mesg_info->gc = XtGetGC(w, mask, &values);
}

/*
 * Handle resizes on a message window.  Correct saved pixel height and width.
 * Adjust circle buffer to accomidate the new size.
 *
 * Problem:  If the resize decreases the width of the window such that
 * some lines are now longer than the window, they will be cut off by
 * X itself.  All new lines will be split to the new size, but the ends
 * of the old ones will not be seen again unless the window is lengthened.
 * I don't deal with this problem because it isn't worth the trouble.
 */
/* ARGSUSED */
static void
mesg_resized(Widget w, XtPointer call_data, XtPointer client_data)
{
    Arg args[4];
    Cardinal num_args;
    Dimension pixel_width, pixel_height;
    struct xwindow *wp;
#ifdef VERBOSE
    int old_lines;

    old_lines = wp->mesg_information->num_lines;
    ;
#endif

    nhUse(call_data);
    nhUse(client_data);

    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, &pixel_width);
    num_args++;
    XtSetArg(args[num_args], XtNheight, &pixel_height);
    num_args++;
    XtGetValues(w, args, num_args);

    wp = find_widget(w);
    wp->pixel_width = pixel_width;
    wp->pixel_height = pixel_height;

    set_circle_buf(wp->mesg_information,
                   (int) pixel_height / wp->mesg_information->char_height);

#ifdef VERBOSE
    printf("Message resize.  Pixel: width = %d, height = %d;  Lines: old = "
           "%d, new = %d\n",
           pixel_width, pixel_height, old_lines,
           wp->mesg_information->num_lines);
#endif
}

/*winmesg.c*/

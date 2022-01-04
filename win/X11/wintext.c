/* NetHack 3.7	wintext.c	$NHDT-Date: 1597967808 2020/08/20 23:56:48 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.22 $ */
/* Copyright (c) Dean Luick, 1992				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * File for dealing with text windows.
 *
 *	+ No global functions.
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV /* X11 include files may define SYSV */
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xos.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xatom.h>

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#include "hack.h"
#include "winX.h"
#include "xwindow.h"

#ifdef GRAPHIC_TOMBSTONE
#include <X11/xpm.h>
#endif

#define TRANSIENT_TEXT /* text window is a transient window (no positioning) \
                          */

static const char text_translations[] = "#override\n\
     <BtnDown>: dismiss_text()\n\
     <Key>: key_dismiss_text()";

#ifdef GRAPHIC_TOMBSTONE
static const char rip_translations[] = "#override\n\
     <BtnDown>: rip_dismiss_text()\n\
     <Key>: rip_dismiss_text()";

static Widget create_ripout_widget(Widget);
#endif

/*ARGSUSED*/
void
delete_text(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    struct xwindow *wp;
    struct text_info_t *text_info;

    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    wp = find_widget(w);
    text_info = wp->text_information;

    nh_XtPopdown(wp->popup);

    if (text_info->blocked) {
        exit_x_event = TRUE;
    } else if (text_info->destroy_on_ack) {
        destroy_text_window(wp);
    }
}

/*
 * Callback used for all text windows.  The window is poped down on any key
 * or button down event.  It is destroyed if the main nethack code is done
 * with it.
 */
/*ARGSUSED*/
void
dismiss_text(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    struct xwindow *wp;
    struct text_info_t *text_info;

    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    wp = find_widget(w);
    text_info = wp->text_information;

    nh_XtPopdown(wp->popup);

    if (text_info->blocked) {
        exit_x_event = TRUE;
    } else if (text_info->destroy_on_ack) {
        destroy_text_window(wp);
    }
}

/* Dismiss when a non-modifier key pressed. */
void
key_dismiss_text(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    char ch = key_event_to_char((XKeyEvent *) event);
    if (ch)
        dismiss_text(w, event, params, num_params);
}

#ifdef GRAPHIC_TOMBSTONE
/* Dismiss from clicking on rip image. */
void
rip_dismiss_text(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    dismiss_text(XtParent(w), event, params, num_params);
}
#endif

/* ARGSUSED */
void
add_to_text_window(struct xwindow *wp, int attr, /* currently unused */
                   const char *str)
{
    struct text_info_t *text_info = wp->text_information;
    int width;

    nhUse(attr);

    append_text_buffer(&text_info->text, str, FALSE);

    /* Calculate text width and save longest line */
    width = XTextWidth(text_info->fs, str, (int) strlen(str));
    if (width > text_info->max_width)
        text_info->max_width = width;
}

void
display_text_window(struct xwindow *wp, boolean blocking)
{
    struct text_info_t *text_info;
    Arg args[8];
    Cardinal num_args;
    Dimension width, height, font_height;
    int nlines;

    text_info = wp->text_information;
    width = text_info->max_width + text_info->extra_width;
    text_info->blocked = blocking;
    text_info->destroy_on_ack = FALSE;
    font_height = nhFontHeight(wp->w);

    /*
     * Calculate the number of lines to use.  First, find the number of
     * lines that would fit on the screen.  Next, remove four of these
     * lines to give room for a possible window manager titlebar (some
     * wm's put a titlebar on transient windows).  Make sure we have
     * _some_ lines.  Finally, use the number of lines in the text if
     * there are fewer than the max.
     */
    nlines = (XtScreen(wp->w)->height - text_info->extra_height) / font_height;
    nlines -= 4;

    if (nlines > text_info->text.num_lines)
        nlines = text_info->text.num_lines;
    if (nlines <= 0)
        nlines = 1;

    height = nlines * font_height + text_info->extra_height;

    num_args = 0;

    if (nlines < text_info->text.num_lines) {
        /* add on width of scrollbar.  Really should look this up,
         * but can't until the window is realized.  Chicken-and-egg problem.
         */
        width += 20;
    }

#ifdef GRAPHIC_TOMBSTONE
    if (text_info->is_rip) {
        Widget rip = create_ripout_widget(XtParent(wp->w));

        if (rip) {
            XtSetArg(args[num_args], nhStr(XtNfromVert), rip);
            num_args++;
        } else
            text_info->is_rip = FALSE;
    }
#endif

    if (width > (Dimension) XtScreen(wp->w)->width) { /* too wide for screen */
        /* Back off some amount - we really need to back off the scrollbar */
        /* width plus some extra.					   */
        width = XtScreen(wp->w)->width - 20;
    }
    XtSetArg(args[num_args], XtNstring, text_info->text.text);
    num_args++;
    XtSetArg(args[num_args], XtNwidth, width);
    num_args++;
    XtSetArg(args[num_args], XtNheight, height);
    num_args++;
    XtSetValues(wp->w, args, num_args);

#ifdef TRANSIENT_TEXT
    XtRealizeWidget(wp->popup);
    XSetWMProtocols(XtDisplay(wp->popup), XtWindow(wp->popup),
                    &wm_delete_window, 1);
    positionpopup(wp->popup, FALSE);
#endif

    nh_XtPopup(wp->popup, (int) XtGrabNone, wp->w);

    /* Kludge alert.  Scrollbars are not sized correctly by the Text widget */
    /* if added before the window is displayed, so do it afterward. */
    num_args = 0;
    if (nlines < text_info->text.num_lines) { /* add vert scrollbar */
        XtSetArg(args[num_args], nhStr(XtNscrollVertical),
                 XawtextScrollAlways);
        num_args++;
    }
    if (width >= (Dimension)(XtScreen(wp->w)->width - 20)) { /* too wide */
        XtSetArg(args[num_args], nhStr(XtNscrollHorizontal),
                 XawtextScrollAlways);
        num_args++;
    }
    if (num_args)
        XtSetValues(wp->w, args, num_args);

    /* We want the user to acknowlege. */
    if (blocking) {
        (void) x_event(EXIT_ON_EXIT);
        nh_XtPopdown(wp->popup);
    }
}

void
create_text_window(struct xwindow *wp)
{
    struct text_info_t *text_info;
    Arg args[8];
    Cardinal num_args;
    Position top_margin, bottom_margin, left_margin, right_margin;
    Widget form;

    wp->type = NHW_TEXT;

    wp->text_information = text_info =
        (struct text_info_t *) alloc(sizeof(struct text_info_t));

    init_text_buffer(&text_info->text);
    text_info->max_width = 0;
    text_info->extra_width = 0;
    text_info->extra_height = 0;
    text_info->blocked = FALSE;
    text_info->destroy_on_ack = TRUE; /* Ok to destroy before display */
#ifdef GRAPHIC_TOMBSTONE
    text_info->is_rip = FALSE;
#endif

    num_args = 0;
    XtSetArg(args[num_args], XtNallowShellResize, True), num_args++;
    XtSetArg(args[num_args], XtNtranslations,
             XtParseTranslationTable(text_translations)), num_args++;

#ifdef TRANSIENT_TEXT
    wp->popup = XtCreatePopupShell("text", transientShellWidgetClass,
                                   toplevel, args, num_args);
#else
    wp->popup = XtCreatePopupShell("text", topLevelShellWidgetClass, toplevel,
                                   args, num_args);
#endif
    XtOverrideTranslations(
        wp->popup,
        XtParseTranslationTable("<Message>WM_PROTOCOLS: delete_text()"));

    num_args = 0;
    XtSetArg(args[num_args], XtNallowShellResize, True), num_args++;
    XtSetArg(args[num_args], XtNtranslations,
             XtParseTranslationTable(text_translations)), num_args++;
    form = XtCreateManagedWidget("form", formWidgetClass, wp->popup, args,
                                 num_args);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNdisplayCaret), False);
    num_args++;
    XtSetArg(args[num_args], XtNresize, XawtextResizeBoth);
    num_args++;
    XtSetArg(args[num_args], XtNtranslations,
             XtParseTranslationTable(text_translations));
    num_args++;

    wp->w = XtCreateManagedWidget(g.killer.name[0] && WIN_MAP == WIN_ERR
                                      ? "tombstone"
                                      : "text_text", /* name */
                                  asciiTextWidgetClass,
                                  form,      /* parent widget */
                                  args,      /* set some values */
                                  num_args); /* number of values to set */

    /* Get the font and margin information. */
    num_args = 0;
    XtSetArg(args[num_args], XtNfont, &text_info->fs);
    num_args++;
    XtSetArg(args[num_args], nhStr(XtNtopMargin), &top_margin);
    num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottomMargin), &bottom_margin);
    num_args++;
    XtSetArg(args[num_args], nhStr(XtNleftMargin), &left_margin);
    num_args++;
    XtSetArg(args[num_args], nhStr(XtNrightMargin), &right_margin);
    num_args++;
    XtGetValues(wp->w, args, num_args);

    text_info->extra_width = left_margin + right_margin;
    text_info->extra_height = top_margin + bottom_margin;
}

void
destroy_text_window(struct xwindow *wp)
{
    /* Don't need to pop down, this only called from dismiss_text(). */

    struct text_info_t *text_info = wp->text_information;

    /*
     * If the text window was blocked, then the user has already ACK'ed
     * it and we are free to really destroy the window.  Otherwise, don't
     * destroy until the user dismisses the window via a key or button
     * press.
     */
    if (text_info->blocked || text_info->destroy_on_ack) {
        XtDestroyWidget(wp->popup);
        free_text_buffer(&text_info->text);
        free((genericptr_t) text_info), wp->text_information = 0;
        wp->type = NHW_NONE; /* allow reuse */
    } else {
        text_info->destroy_on_ack = TRUE; /* destroy on next ACK */
    }
}

void
clear_text_window(struct xwindow *wp)
{
    clear_text_buffer(&wp->text_information->text);
}

/* text buffer routines ----------------------------------------------------
 */

/* Append a line to the text buffer. */
void
append_text_buffer(struct text_buffer *tb, const char *str, boolean concat)
{
    char *copy;
    int length;

    if (!tb->text)
        panic("append_text_buffer:  null text buffer");

    if (str) {
        length = strlen(str);
    } else {
        length = 0;
    }

    if (length + tb->text_last + 1 >= tb->text_size) {
/* we need to go to a bigger buffer! */
#ifdef VERBOSE
        printf(
            "append_text_buffer: text buffer growing from %d to %d bytes\n",
            tb->text_size, 2 * tb->text_size);
#endif
        copy = (char *) alloc((unsigned) tb->text_size * 2);
        (void) memcpy(copy, tb->text, tb->text_last);
        free(tb->text);
        tb->text = copy;
        tb->text_size *= 2;
    }

    if (tb->num_lines) { /* not first --- append a newline */
        char appchar = '\n';

        if (concat && !index("!.?'\")", tb->text[tb->text_last - 1])) {
            appchar = ' ';
            tb->num_lines--; /* offset increment at end of function */
        }

        *(tb->text + tb->text_last) = appchar;
        tb->text_last++;
    }

    if (str) {
        (void) memcpy((tb->text + tb->text_last), str, length + 1);
        if (length) {
            /* Remove all newlines. Otherwise we have a confused line count. */
            copy = (tb->text + tb->text_last);
            while ((copy = index(copy, '\n')) != (char *) 0)
                *copy = ' ';
        }

        tb->text_last += length;
    }
    tb->text[tb->text_last] = '\0';
    tb->num_lines++;
}

/* Initialize text buffer. */
void
init_text_buffer(struct text_buffer *tb)
{
    tb->text = (char *) alloc(START_SIZE);
    tb->text[0] = '\0';
    tb->text_size = START_SIZE;
    tb->text_last = 0;
    tb->num_lines = 0;
}

/* Empty the text buffer */
void
clear_text_buffer(struct text_buffer *tb)
{
    tb->text_last = 0;
    tb->text[0] = '\0';
    tb->num_lines = 0;
}

/* Free up allocated memory. */
void
free_text_buffer(struct text_buffer *tb)
{
    free(tb->text);
    tb->text = (char *) 0;
    tb->text_size = 0;
    tb->text_last = 0;
    tb->num_lines = 0;
}

#ifdef GRAPHIC_TOMBSTONE

static void rip_exposed(Widget, XtPointer, XtPointer);

static XImage *rip_image = 0;

#define STONE_LINE_LEN 16 /* # chars that fit on one line */
#define NAME_LINE 0       /* line # for player name */
#define GOLD_LINE 1       /* line # for amount of gold */
#define DEATH_LINE 2      /* line # for death description */
#define YEAR_LINE 6       /* line # for year */

static char rip_line[YEAR_LINE + 1][STONE_LINE_LEN + 1];

void
calculate_rip_text(int how, time_t when)
{
    /* Follows same algorithm as genl_outrip() */

    char buf[BUFSZ];
    char *dpx;
    int line, year;
    long cash;

    /* Put name on stone */
    Sprintf(rip_line[NAME_LINE], "%.16s", g.plname); /* STONE_LINE_LEN */

    /* Put $ on stone */
    cash = max(g.done_money, 0L);
    /* arbitrary upper limit; practical upper limit is quite a bit less */
    if (cash > 999999999L)
        cash = 999999999L;
    Sprintf(buf, "%ld Au", cash);
    Sprintf(rip_line[GOLD_LINE], "%ld Au", cash);

    /* Put together death description */
    formatkiller(buf, sizeof buf, how, FALSE);

    /* Put death type on stone */
    for (line = DEATH_LINE, dpx = buf; line < YEAR_LINE; line++) {
        register int i, i0;
        char tmpchar;

        if ((i0 = strlen(dpx)) > STONE_LINE_LEN) {
            for (i = STONE_LINE_LEN; ((i0 > STONE_LINE_LEN) && i); i--)
                if (dpx[i] == ' ')
                    i0 = i;
            if (!i)
                i0 = STONE_LINE_LEN;
        }
        tmpchar = dpx[i0];
        dpx[i0] = 0;
        strcpy(rip_line[line], dpx);
        if (tmpchar != ' ') {
            dpx[i0] = tmpchar;
            dpx = &dpx[i0];
        } else
            dpx = &dpx[i0 + 1];
    }

    /* Put year on stone */
    year = (int) ((yyyymmdd(when) / 10000L) % 10000L);
    Sprintf(rip_line[YEAR_LINE], "%4d", year);
}

/*
 * RIP image expose callback.
 */
/*ARGSUSED*/
static void
rip_exposed(Widget w, XtPointer client_data UNUSED,
            XtPointer widget_data) /* expose event from Window widget */
{
    XExposeEvent *event = (XExposeEvent *) widget_data;
    Display *dpy = XtDisplay(w);
    Arg args[8];
    XGCValues values;
    XtGCMask mask;
    GC gc;
    static Pixmap rip_pixmap = None;
    int i, x, y;

    if (!XtIsRealized(w) || event->count > 0)
        return;

    if (rip_pixmap == None && rip_image) {
        rip_pixmap = XCreatePixmap(dpy, XtWindow(w), rip_image->width,
                                   rip_image->height,
                                   DefaultDepth(dpy, DefaultScreen(dpy)));
        XPutImage(dpy, rip_pixmap, DefaultGC(dpy, DefaultScreen(dpy)),
                  rip_image, 0, 0, 0, 0, /* src, dest top left */
                  rip_image->width, rip_image->height);
        XDestroyImage(rip_image); /* data bytes free'd also */
    }

    mask = GCFunction | GCForeground | GCGraphicsExposures | GCFont;
    values.graphics_exposures = False;
    XtSetArg(args[0], XtNforeground, &values.foreground);
    XtGetValues(w, args, 1);
    values.function = GXcopy;
    values.font = WindowFont(w);
    gc = XtGetGC(w, mask, &values);

    if (rip_pixmap != None) {
        XCopyArea(dpy, rip_pixmap, XtWindow(w), gc, event->x, event->y,
                  event->width, event->height, event->x, event->y);
    }

    x = appResources.tombtext_x;
    y = appResources.tombtext_y;
    for (i = 0; i <= YEAR_LINE; i++) {
        int len = strlen(rip_line[i]);
        XFontStruct *font = WindowFontStruct(w);
        int width = XTextWidth(font, rip_line[i], len);

        XDrawString(dpy, XtWindow(w), gc, x - width / 2, y, rip_line[i], len);
        x += appResources.tombtext_dx;
        y += appResources.tombtext_dy;
    }

    XtReleaseGC(w, gc);
}

/*
 * The ripout window creation routine.
 */
static Widget
create_ripout_widget(Widget parent)
{
    Widget imageport;
    Arg args[16];
    Cardinal num_args;

    static int rip_width, rip_height;

    if (!rip_image) {
        XpmAttributes attributes;
        int errorcode;

        attributes.valuemask = XpmCloseness;
        attributes.closeness = 65535; /* Try anything */
        errorcode = XpmReadFileToImage(XtDisplay(parent),
                                       appResources.tombstone,
                                       &rip_image, 0, &attributes);
        if (errorcode != XpmSuccess) {
            char buf[BUFSZ];

            Sprintf(buf, "Failed to load %s: %s", appResources.tombstone,
                    XpmGetErrorString(errorcode));
            X11_raw_print(buf);
            return (Widget) 0;
        }
        rip_width = rip_image->width;
        rip_height = rip_image->height;
    }

    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, rip_width);
    num_args++;
    XtSetArg(args[num_args], XtNheight, rip_height);
    num_args++;
    XtSetArg(args[num_args], XtNtranslations,
             XtParseTranslationTable(rip_translations));
    num_args++;

    imageport = XtCreateManagedWidget("rip", windowWidgetClass, parent, args,
                                      num_args);

    XtAddCallback(imageport, XtNexposeCallback, rip_exposed, (XtPointer) 0);

    return imageport;
}

#endif /* GRAPHIC_TOMBSTONE */

/*wintext.c*/

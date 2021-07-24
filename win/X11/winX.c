/* NetHack 3.7	winX.c	$NHDT-Date: 1613985000 2021/02/22 09:10:00 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.102 $ */
/* Copyright (c) Dean Luick, 1992                                 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * "Main" file for the X window-port.  This contains most of the interface
 * routines.  Please see doc/window.doc for an description of the window
 * interface.
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV /* X11 include files may define SYSV */
#endif

#ifdef MSDOS /* from compiler */
#define SHORT_FILENAMES
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>

/* for color support */
#ifdef SHORT_FILENAMES
#include <X11/IntrinsP.h>
#else
#include <X11/IntrinsicP.h>
#endif

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#ifdef SHORT_FILENAMES
#undef SHORT_FILENAMES /* hack.h will reset via global.h if necessary */
#endif

#include "hack.h"
#include "winX.h"
#include "dlb.h"
#include "xwindow.h"

#ifndef NO_SIGNAL
#include <signal.h>
#endif

/* Should be defined in <X11/Intrinsic.h> but you never know */
#ifndef XtSpecificationRelease
#define XtSpecificationRelease 0
#endif

/*
 * Icons.
 */
#include "../win/X11/nh72icon"
#include "../win/X11/nh56icon"
#include "../win/X11/nh32icon"

static struct icon_info {
    const char *name;
    unsigned char *bits;
    unsigned width, height;
} icon_data[] = { { "nh72", nh72icon_bits, nh72icon_width, nh72icon_height },
                  { "nh56", nh56icon_bits, nh56icon_width, nh56icon_height },
                  { "nh32", nh32icon_bits, nh32icon_width, nh32icon_height },
                  { (const char *) 0, (unsigned char *) 0, 0, 0 } };

/*
 * Private global variables (shared among the window port files).
 */
struct xwindow window_list[MAX_WINDOWS];
AppResources appResources;
void (*input_func)(Widget, XEvent *, String *, Cardinal *);
int click_x, click_y, click_button; /* Click position on a map window   */
                                    /* (filled by set_button_values()). */
int updated_inventory; /* used to indicate perm_invent updating */

static int (*old_error_handler) (Display *, XErrorEvent *);

#if !defined(NO_SIGNAL) && defined(SAFERHANGUP)
#if XtSpecificationRelease >= 6
#define X11_HANGUP_SIGNAL
static XtSignalId X11_sig_id;
#endif
#endif

/* Interface definition, for windows.c */
struct window_procs X11_procs = {
    "X11",
    ( WC_COLOR | WC_INVERSE | WC_HILITE_PET | WC_ASCII_MAP | WC_TILED_MAP
     | WC_PLAYER_SELECTION | WC_PERM_INVENT | WC_MOUSE_SUPPORT ),
    /* status requires VIA_WINDOWPORT(); WC2_FLUSH_STATUS ensures that */
    ( WC2_FLUSH_STATUS | WC2_SELECTSAVED
#ifdef STATUS_HILITES
      | WC2_RESET_STATUS | WC2_HILITE_STATUS
#endif
      | WC2_MENU_SHIFT ),
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, /* color availability */
    X11_init_nhwindows,
    X11_player_selection, X11_askname, X11_get_nh_event, X11_exit_nhwindows,
    X11_suspend_nhwindows, X11_resume_nhwindows, X11_create_nhwindow,
    X11_clear_nhwindow, X11_display_nhwindow, X11_destroy_nhwindow, X11_curs,
    X11_putstr, genl_putmixed, X11_display_file, X11_start_menu, X11_add_menu,
    X11_end_menu, X11_select_menu,
    genl_message_menu, /* no need for X-specific handling */
    X11_update_inventory, X11_mark_synch, X11_wait_synch,
#ifdef CLIPPING
    X11_cliparound,
#endif
#ifdef POSITIONBAR
    donull,
#endif
    X11_print_glyph, X11_raw_print, X11_raw_print_bold, X11_nhgetch,
    X11_nh_poskey, X11_nhbell, X11_doprev_message, X11_yn_function,
    X11_getlin, X11_get_ext_cmd, X11_number_pad, X11_delay_output,
#ifdef CHANGE_COLOR /* only a Mac option currently */
    donull, donull,
#endif
    /* other defs that really should go away (they're tty specific) */
    X11_start_screen, X11_end_screen,
#ifdef GRAPHIC_TOMBSTONE
    X11_outrip,
#else
    genl_outrip,
#endif
    X11_preference_update, X11_getmsghistory, X11_putmsghistory,
    X11_status_init, X11_status_finish, X11_status_enablefield,
    X11_status_update,
    genl_can_suspend_no, /* XXX may not always be correct */
};

/*
 * Local functions.
 */
static winid find_free_window(void);
#ifdef TEXTCOLOR
static void nhFreePixel(XtAppContext, XrmValuePtr, XtPointer, XrmValuePtr,
                        Cardinal *);
#endif
static boolean new_resource_macro(String, unsigned);
static void load_default_resources(void);
static void release_default_resources(void);
static int panic_on_error(Display *, XErrorEvent *);
#ifdef X11_HANGUP_SIGNAL
static void X11_sig(int);
static void X11_sig_cb(XtPointer, XtSignalId *);
#endif
static void d_timeout(XtPointer, XtIntervalId *);
static void X11_hangup(Widget, XEvent *, String *, Cardinal *);
static void X11_bail(const char *) NORETURN;
static void askname_delete(Widget, XEvent *, String *, Cardinal *);
static void askname_done(Widget, XtPointer, XtPointer);
static void done_button(Widget, XtPointer, XtPointer);
static void getline_delete(Widget, XEvent *, String *, Cardinal *);
static void abort_button(Widget, XtPointer, XtPointer);
static void release_getline_widgets(void);
static void yn_delete(Widget, XEvent *, String *, Cardinal *);
static void yn_key(Widget, XEvent *, String *, Cardinal *);
static void release_yn_widgets(void);
static int input_event(int);
static void win_visible(Widget, XtPointer, XEvent *, Boolean *);
static void init_standard_windows(void);

/*
 * Local variables.
 */
static boolean x_inited = FALSE;    /* TRUE if window system is set up.     */
static winid message_win = WIN_ERR, /* These are the winids of the message, */
             map_win = WIN_ERR,     /*   map, and status windows, when they */
             status_win = WIN_ERR;  /*   are created in init_windows().     */
static Pixmap icon_pixmap = None;   /* Pixmap for icon.                     */

void
X11_putmsghistory(const char *msg, boolean is_restoring)
{
    if (WIN_MESSAGE != WIN_ERR) {
        struct xwindow *wp = &window_list[WIN_MESSAGE];
        debugpline2("X11_putmsghistory('%s',%i)", msg, is_restoring);
        if (msg)
            append_message(wp, msg);
    }
}

char *
X11_getmsghistory(boolean init)
{
    if (WIN_MESSAGE != WIN_ERR) {
        static struct line_element *curr = (struct line_element *) 0;
        static int numlines = 0;
        struct xwindow *wp = &window_list[WIN_MESSAGE];

        if (init)
            curr = (struct line_element *) 0;

        if (!curr) {
            curr = wp->mesg_information->head;
            numlines = 0;
        }

        if (numlines < wp->mesg_information->num_lines) {
            curr = curr->next;
            numlines++;
            debugpline2("X11_getmsghistory(%i)='%s'", init, curr->line);
            return curr->line;
        }
    }
    return (char *) 0;
}

/*
 * Find the window structure that corresponds to the given widget.  Note
 * that this is not the popup widget, nor the viewport, but the child.
 */
struct xwindow *
find_widget(Widget w)
{
    int windex;
    struct xwindow *wp;

    /*
     * Search to find the corresponding window.  Look at the main widget,
     * popup, the parent of the main widget, then parent of the widget.
     */
    for (windex = 0, wp = window_list; windex < MAX_WINDOWS; windex++, wp++)
        if (wp->type != NHW_NONE && (wp->w == w || wp->popup == w
                                     || (wp->w && (XtParent(wp->w)) == w)
                                     || (wp->popup == XtParent(w))))
            break;

    if (windex == MAX_WINDOWS)
        panic("find_widget:  can't match widget");
    return wp;
}

/*
 * Find a free window slot for use.
 */
static winid
find_free_window(void)
{
    int windex;
    struct xwindow *wp;

    for (windex = 0, wp = &window_list[0]; windex < MAX_WINDOWS;
         windex++, wp++)
        if (wp->type == NHW_NONE)
            break;

    if (windex == MAX_WINDOWS)
        panic("find_free_window: no free windows!");
    return (winid) windex;
}


XColor
get_nhcolor(struct xwindow *wp, int clr)
{
    init_menu_nhcolors(wp);
    /* FIXME: init_menu_nhcolors may fail */

    if (clr >= 0 && clr < CLR_MAX)
        return wp->nh_colors[clr];

    return wp->nh_colors[0];
}

void
init_menu_nhcolors(struct xwindow *wp)
{
    static const char *mapCLR_to_res[CLR_MAX] = {
        XtNblack,
        XtNred,
        XtNgreen,
        XtNbrown,
        XtNblue,
        XtNmagenta,
        XtNcyan,
        XtNgray,
        XtNforeground,
        XtNorange,
        XtNbright_green,
        XtNyellow,
        XtNbright_blue,
        XtNbright_magenta,
        XtNbright_cyan,
        XtNwhite,
    };
    static const char *wintypenames[NHW_TEXT] = {
        "message", "status", "map", "menu", "text"
    };
    Display *dpy;
    Colormap screen_colormap;
    XrmDatabase rDB;
    XrmValue value;
    Status rc;
    int color;
    char *ret_type[32];
    char wtn_up[20], clr_name[BUFSZ], clrclass[BUFSZ];
    const char *wtn;

    if (wp->nh_colors_inited || !wp->type)
        return;

    wtn = wintypenames[wp->type - 1];
    Strcpy(wtn_up, wtn);
    (void) upstart(wtn_up);

    dpy = XtDisplay(wp->w);
    screen_colormap = DefaultColormap(dpy, DefaultScreen(dpy));
    rDB = XrmGetDatabase(dpy);

    for (color = 0; color < CLR_MAX; color++) {
        Sprintf(clr_name, "nethack.%s.%s", wtn, mapCLR_to_res[color]);
        Sprintf(clrclass, "NetHack.%s.%s", wtn_up, mapCLR_to_res[color]);

        if (!XrmGetResource(rDB, clr_name, clrclass, ret_type, &value)) {
            Sprintf(clr_name, "nethack.map.%s", mapCLR_to_res[color]);
            Sprintf(clrclass, "NetHack.Map.%s", mapCLR_to_res[color]);
        }

        if (!XrmGetResource(rDB, clr_name, clrclass, ret_type, &value)) {
            impossible("XrmGetResource error (%s)", clr_name);
        } else if (!strcmp(ret_type[0], "String")) {
            char tmpbuf[256];

            if (value.size >= sizeof tmpbuf)
                value.size = sizeof tmpbuf - 1;
            (void) strncpy(tmpbuf, (char *) value.addr, (int) value.size);
            tmpbuf[value.size] = '\0';
            /* tmpbuf now contains the color name from the named resource */

            rc = XAllocNamedColor(dpy, screen_colormap, tmpbuf,
                                  &wp->nh_colors[color],
                                  &wp->nh_colors[color]);
            if (rc == 0) {
                impossible("XAllocNamedColor failed for color %i (%s)",
                           color, clr_name);
            }
        }
    }

    wp->nh_colors_inited = TRUE;
}

/*
 * Color conversion.  The default X11 color converters don't try very
 * hard to find matching colors in PseudoColor visuals.  If they can't
 * allocate the exact color, they puke and give you something stupid.
 * This is an attempt to find some close readonly cell and use it.
 */
XtConvertArgRec const nhcolorConvertArgs[] = {
    { XtWidgetBaseOffset,
      (XtPointer) (ptrdiff_t) XtOffset(Widget, core.screen),
      sizeof (Screen *) },
    { XtWidgetBaseOffset,
      (XtPointer) (ptrdiff_t) XtOffset(Widget, core.colormap),
      sizeof (Colormap) }
};

#define done(type, value)                             \
    {                                                 \
        if (toVal->addr != 0) {                       \
            if (toVal->size < sizeof(type)) {         \
                toVal->size = sizeof(type);           \
                return False;                         \
            }                                         \
            *(type *)(toVal->addr) = (value);         \
        } else {                                      \
            static type static_val;                   \
            static_val = (value);                     \
            toVal->addr = (genericptr_t) &static_val; \
        }                                             \
        toVal->size = sizeof(type);                   \
        return True;                                  \
    }

/*
 * Find a color that approximates the color named in "str".
 * The "str" color may be a color name ("red") or number ("#7f0000").
 * If str is Null, then "color" is assumed to contain the RGB color wanted.
 * The approximate color found is returned in color as well.
 * Return True if something close was found.
 */
Boolean
nhApproxColor(Screen *screen,    /* screen to use */
              Colormap colormap, /* the colormap to use */
              char *str,         /* color name */
              XColor *color)     /* the X color structure; changed only if successful */
{
    int ncells;
    long cdiff = 16777216; /* 2^24; hopefully our map is smaller */
    XColor tmp;
    static XColor *table = 0;
    register int i, j;
    register long tdiff;

    /* if the screen doesn't have a big colormap, don't waste our time
       or if it's huge, and _some_ match should have been possible */
    if ((ncells = CellsOfScreen(screen)) < 256 || ncells > 4096)
        return False;

    if (str != (char *) 0) {
        if (!XParseColor(DisplayOfScreen(screen), colormap, str, &tmp))
            return False;
    } else {
        tmp = *color;
        tmp.flags = 7; /* force to use all 3 of RGB */
    }

    if (!table) {
        table = (XColor *) XtCalloc(ncells, sizeof(XColor));
        for (i = 0; i < ncells; i++)
            table[i].pixel = i;
        XQueryColors(DisplayOfScreen(screen), colormap, table, ncells);
    }

/* go thru cells and look for the one with smallest diff        */
/* diff is calculated abs(reddiff)+abs(greendiff)+abs(bluediff) */
/* a more knowledgeable color person might improve this -dlc    */
try_again:
    for (i = 0; i < ncells; i++) {
        if (table[i].flags == tmp.flags) {
            j = (int) table[i].red - (int) tmp.red;
            if (j < 0)
                j = -j;
            tdiff = j;
            j = (int) table[i].green - (int) tmp.green;
            if (j < 0)
                j = -j;
            tdiff += j;
            j = (int) table[i].blue - (int) tmp.blue;
            if (j < 0)
                j = -j;
            tdiff += j;
            if (tdiff < cdiff) {
                cdiff = tdiff;
                tmp.pixel = i; /* table[i].pixel == i */
            }
        }
    }

    if (cdiff == 16777216)
        return False; /* nothing found?! */

    /*
     * Found something.  Return it and mark this color as used to avoid
     * reuse.  Reuse causes major contrast problems :-)
     */
    *color = table[tmp.pixel];
    table[tmp.pixel].flags = 0;
    /* try to alloc the color, so no one else can change it */
    if (!XAllocColor(DisplayOfScreen(screen), colormap, color)) {
        cdiff = 16777216;
        goto try_again;
    }
    return True;
}

Boolean
nhCvtStringToPixel(Display *dpy, XrmValuePtr args, Cardinal *num_args,
                   XrmValuePtr fromVal, XrmValuePtr toVal,
                   XtPointer *closure_ret)
{
    String str = (String) fromVal->addr;
    XColor screenColor;
    XColor exactColor;
    Screen *screen;
    XtAppContext app = XtDisplayToApplicationContext(dpy);
    Colormap colormap;
    Status status;
    String params[1];
    Cardinal num_params = 1;

    if (*num_args != 2) {
        XtAppWarningMsg(app, "wrongParameters",
                        "cvtStringToPixel", "XtToolkitError",
             "String to pixel conversion needs screen and colormap arguments",
                        (String *) 0, (Cardinal *) 0);
        return False;
    }

    screen = *((Screen **) args[0].addr);
    colormap = *((Colormap *) args[1].addr);

/* If Xt colors, use the Xt routine and hope for the best */
#if (XtSpecificationRelease >= 5)
    if ((strcmpi(str, XtDefaultBackground) == 0)
        || (strcmpi(str, XtDefaultForeground) == 0)) {
        return XtCvtStringToPixel(dpy, args, num_args, fromVal, toVal,
                                  closure_ret);
    }
#else
    if (strcmpi(str, XtDefaultBackground) == 0) {
        *closure_ret = (char *) False;
        done(Pixel, WhitePixelOfScreen(screen));
    }
    if (strcmpi(str, XtDefaultForeground) == 0) {
        *closure_ret = (char *) False;
        done(Pixel, BlackPixelOfScreen(screen));
    }
#endif

    status = XAllocNamedColor(DisplayOfScreen(screen), colormap, (char *) str,
                              &screenColor, &exactColor);
    if (status == 0) {
        String msg, type;

        /* some versions of XAllocNamedColor don't allow #xxyyzz names */
        if (str[0] == '#' && XParseColor(DisplayOfScreen(screen), colormap,
                                         str, &exactColor)
            && XAllocColor(DisplayOfScreen(screen), colormap, &exactColor)) {
            *closure_ret = (char *) True;
            done(Pixel, exactColor.pixel);
        }

        params[0] = str;
        /* Server returns a specific error code but Xlib discards it.  Ugh */
        if (XLookupColor(DisplayOfScreen(screen), colormap, (char *) str,
                         &exactColor, &screenColor)) {
            /* try to find another color that will do */
            if (nhApproxColor(screen, colormap, (char *) str, &screenColor)) {
                *closure_ret = (char *) True;
                done(Pixel, screenColor.pixel);
            }
            type = nhStr("noColormap");
            msg = nhStr("Cannot allocate colormap entry for \"%s\"");
        } else {
            /* some versions of XLookupColor also don't allow #xxyyzz names */
            if (str[0] == '#'
                && (nhApproxColor(screen, colormap, (char *) str,
                                  &screenColor))) {
                *closure_ret = (char *) True;
                done(Pixel, screenColor.pixel);
            }
            type = nhStr("badValue");
            msg = nhStr("Color name \"%s\" is not defined");
        }

        XtAppWarningMsg(app, type, "cvtStringToPixel", "XtToolkitError", msg,
                        params, &num_params);
        *closure_ret = False;
        return False;
    } else {
        *closure_ret = (char *) True;
        done(Pixel, screenColor.pixel);
    }
}

/* Ask the WM for window frame size */
void
get_window_frame_extents(Widget w,
                         long *top, long *bottom,
                         long *left, long *right)
{
    XEvent event;
    Display *dpy = XtDisplay(w);
    Window win = XtWindow(w);
    Atom prop, retprop;
    int retfmt;
    unsigned long nitems;
    unsigned long nbytes;
    unsigned char *data = 0;
    long *extents;

    prop = XInternAtom(dpy, "_NET_FRAME_EXTENTS", True);
    if (prop == None) {
        /*
         * FIXME!
         */
#ifdef MACOS
        /*
         * Default window manager doesn't support _NET_FRAME_EXTENTS.
         * Without this position tweak, the persistent inventory window
         * creeps downward by approximately the height of its title bar
         * and also a smaller amount to the left every time it gets
         * updated.  Caveat:  amount determined by trial and error and
         * could change depending upon monitor resolution....
         */
        *top = 22;
        *left = 0;
#endif
        return;
    }

    while (XGetWindowProperty(dpy, win, prop,
                              0, 4, False, AnyPropertyType,
                              &retprop, &retfmt,
                              &nitems, &nbytes, &data) != Success
           || nitems != 4 || nbytes != 0) {
        XNextEvent(dpy, &event);
    }

    extents = (long *) data;

    *left = extents[0];
    *right = extents[1];
    *top = extents[2];
    *bottom = extents[3];
}

void
get_widget_window_geometry(Widget w, int *x, int *y, int *width, int *height)
{
    long top, bottom, left, right;
    Arg args[5];
    XtSetArg(args[0], nhStr(XtNx), x);
    XtSetArg(args[1], nhStr(XtNy), y);
    XtSetArg(args[2], nhStr(XtNwidth), width);
    XtSetArg(args[3], nhStr(XtNheight), height);
    XtGetValues(w, args, 4);
    get_window_frame_extents(w, &top, &bottom, &left, &right);
    *x -= left;
    *y -= top;
}

/* Change the full font name string so the weight is "bold" */
char *
fontname_boldify(const char *fontname)
{
    static char buf[BUFSZ];
    char *bufp = buf;
    int idx = 0;

    while (*fontname) {
        if (*fontname == '-')
            idx++;
        *bufp = *fontname;
        if (idx == 3) {
            strcat(buf, "bold");
            bufp += 5;
            do {
                fontname++;
            } while (*fontname && *fontname != '-');
        } else {
            bufp++;
            fontname++;
        }
    }
    *bufp = '\0';
    return buf;
}

void
load_boldfont(struct xwindow *wp, Widget w)
{
    Arg args[1];
    XFontStruct *fs;
    unsigned long ret;
    char *fontname;
    Display *dpy;

    if (wp->boldfs)
        return;

    XtSetArg(args[0], nhStr(XtNfont), &fs);
    XtGetValues(w, args, 1);

    if (!XGetFontProperty(fs, XA_FONT, &ret))
        return;

    wp->boldfs_dpy = dpy = XtDisplay(w);
    fontname = fontname_boldify(XGetAtomName(dpy, (Atom)ret));
    wp->boldfs = XLoadQueryFont(dpy, fontname);
}

#ifdef TEXTCOLOR
/* ARGSUSED */
static void
nhFreePixel(XtAppContext app,
            XrmValuePtr toVal,
            XtPointer closure,
            XrmValuePtr args,
            Cardinal *num_args)
{
    Screen *screen;
    Colormap colormap;

    if (*num_args != 2) {
        XtAppWarningMsg(app, "wrongParameters", "freePixel", "XtToolkitError",
                     "Freeing a pixel requires screen and colormap arguments",
                        (String *) 0, (Cardinal *) 0);
        return;
    }

    screen = *((Screen **) args[0].addr);
    colormap = *((Colormap *) args[1].addr);

    if (closure) {
        XFreeColors(DisplayOfScreen(screen), colormap,
                    (unsigned long *) toVal->addr, 1, (unsigned long) 0);
    }
}
#endif /*TEXTCOLOR*/

/* [ALI] Utility function to ask Xaw for font height, since the previous
 * assumption of ascent + descent is not always valid.
 */
Dimension
nhFontHeight(Widget w)
{
#ifdef _XawTextSink_h
    Widget sink;
    XawTextPosition pos = 0;
    int resWidth, resHeight;
    Arg args[1];

    XtSetArg(args[0], XtNtextSink, &sink);
    XtGetValues(w, args, 1);

    XawTextSinkFindPosition(sink, pos, 0, 0, 0, &pos, &resWidth, &resHeight);
    return resHeight;
#else
    XFontStruct *fs;
    Arg args[1];

    XtSetArg(args[0], XtNfont, &fs);
    XtGetValues(w, args, 1);

    /* Assume font height is ascent + descent. */
    return = fs->ascent + fs->descent;
#endif
}

static String *default_resource_data = 0, /* NULL-terminated arrays */
              *def_rsrc_macr = 0, /* macro names */
              *def_rsrc_valu = 0; /* macro values */

/* caller found "#define"; parse into macro name and its expansion value */
static boolean
new_resource_macro(String inbuf, /* points past '#define' rather than to start of buffer */
                   unsigned numdefs) /* array slot to fill */
{
    String p, q;

    /* we expect inbuf to be terminated by newline; get rid of it */
    q = eos(inbuf);
    if (q > inbuf && q[-1] == '\n')
        q[-1] = '\0';

    /* figure out macro's name */
    for (p = inbuf; *p == ' ' || *p == '\t'; ++p)
        continue; /* skip whitespace */
    for (q = p; *q && *q != ' ' && *q != '\t'; ++q)
        continue; /* token consists of non-whitespace */
    Strcat(q, " "); /* guarantee something beyond '#define FOO' */
    *q++ = '\0'; /* p..(q-1) contains macro name */
    if (!*p) /* invalid definition: '#define' followed by nothing */
        return FALSE;
    def_rsrc_macr[numdefs] = dupstr(p);

    /* figure out macro's value; empty value is supported but not expected */
    while (*q == ' ' || *q == '\t')
        ++q; /* skip whitespace between name and value */
    for (p = eos(q); --p > q && (*p == ' ' || *p == '\t'); )
        continue; /* discard trailing whitespace */
    *++p = '\0'; /* q..p containes macro value */
    def_rsrc_valu[numdefs] = dupstr(q);
    return TRUE;
}

/* read the template NetHack.ad into default_resource_data[] to supply
   fallback resources to XtAppInitialize() */
static void
load_default_resources(void)
{
    FILE *fp;
    String inbuf;
    unsigned insiz, linelen, longlen, numlines, numdefs, midx;
    boolean comment, isdef;

    /*
     * Running nethack via the shell script adds $HACKDIR to the path used
     * by X to find resources, but running it directly doesn't.  So, if we
     * can find the template file for NetHack.ad in the current directory,
     * load its contents into memory so that the application startup call
     * in X11_init_nhwindows() can use them as fallback resources.
     *
     * No attempt to support the 'include' directive has been made, nor
     * backslash+newline continuation lines.  Macro expansion (at most
     * one substitution per line) is supported.  '#define' to introduce
     * a macro must be at start of line (no whitespace before or after
     * the '#' character).
     */
    fp = fopen("./NetHack.ad", "r");
    if (!fp)
        return;

    /* measure the file without retaining its contents */
    insiz = BUFSIZ; /* stdio BUFSIZ, not nethack BUFSZ */
    inbuf = (String) alloc(insiz);
    linelen = longlen = 0;
    numlines = numdefs = 0;
    comment = isdef = FALSE; /* lint suppression */
    while (fgets(inbuf, insiz, fp)) {
        if (!linelen) {
            /* !linelen: inbuf has start of record; treat empty as comment */
            comment = (*inbuf == '!' || *inbuf == '\n');
            isdef = !strncmp(inbuf, "#define", 7);
            ++numdefs;
        }
        linelen += strlen(inbuf);
        if (!index(inbuf, '\n'))
            continue;
        if (linelen > longlen)
            longlen = linelen;
        linelen = 0;
        if (!comment && !isdef)
            ++numlines;
    }
    insiz = longlen + 1;
    if (numdefs) { /* don't alloc if 0; no need for any terminator */
        def_rsrc_macr = (String *) alloc(numdefs * sizeof (String));
        def_rsrc_valu = (String *) alloc(numdefs * sizeof (String));
        insiz += BUFSIZ; /* include room for macro expansion within buffer */
    }
    if (insiz > BUFSIZ) {
        free((genericptr_t) inbuf);
        inbuf = (String) alloc(insiz);
    }
    ++numlines; /* room for terminator */
    default_resource_data = (String *) alloc(numlines * sizeof (String));

    /* now re-read the file, storing its contents into the allocated array
       after performing macro substitutions */
    (void) rewind(fp);
    numlines = numdefs = 0;
    while (fgets(inbuf, insiz, fp)) {
        if (!strncmp(inbuf, "#define", 7)) {
            if (new_resource_macro(&inbuf[7], numdefs))
                ++numdefs;
        } else if (*inbuf != '!' && *inbuf != '\n') {
            if (numdefs) {
                /*
                 * Macro expansion:  we assume at most one substitution
                 * per line.  That's all that our sample NetHack.ad uses.
                 *
                 * If we ever need more, this will have to become a lot
                 * more sophisticated.  It will need to find the first
                 * instance within inbuf[] rather than first macro which
                 * appears, and to avoid finding names within substituted
                 * expansion values.
                 *
                 * Any substitution which would exceed the buffer size is
                 * skipped.  A sophisticated implementation would need to
                 * be prepared to allocate a bigger buffer when needed.
                 */
                linelen = strlen(inbuf);
                for (midx = 0; midx < numdefs; ++midx) {
                    if ((linelen + strlen(def_rsrc_valu[midx])
                         < insiz - strlen(def_rsrc_macr[midx]))
                        && strNsubst(inbuf, def_rsrc_macr[midx],
                                     def_rsrc_valu[midx], 1))
                        break;
                }
            }
            default_resource_data[numlines++] = dupstr(inbuf);
        }
    }
    default_resource_data[numlines] = (String) 0;
    (void) fclose(fp);
    free((genericptr_t) inbuf);
    if (def_rsrc_macr) { /* implies def_rsrc_valu is non-Null too */
        for (midx = 0; midx < numdefs; ++midx) {
            free((genericptr_t) def_rsrc_macr[midx]);
            free((genericptr_t) def_rsrc_valu[midx]);
        }
        free((genericptr_t) def_rsrc_macr), def_rsrc_macr = 0;
        free((genericptr_t) def_rsrc_valu), def_rsrc_valu = 0;
    }
}

static void
release_default_resources(void)
{
    if (default_resource_data) {
        unsigned idx;

        for (idx = 0; default_resource_data[idx]; idx++)
            free((genericptr_t) default_resource_data[idx]);
        free((genericptr_t) default_resource_data), default_resource_data = 0;
    }
    /* def_rsrc_macr[] and def_rsrc_valu[] have already been released */
}

/* Global Functions ======================================================= */
void
X11_raw_print(const char *str)
{
    (void) puts(str);
}

void
X11_raw_print_bold(const char *str)
{
    (void) puts(str);
}

void
X11_curs(winid window, int x, int y)
{
    check_winid(window);

    if (x < 0 || x >= COLNO) {
        impossible("curs:  bad x value [%d]", x);
        x = 0;
    }
    if (y < 0 || y >= ROWNO) {
        impossible("curs:  bad y value [%d]", y);
        y = 0;
    }

    window_list[window].cursx = x;
    window_list[window].cursy = y;
}

void
X11_putstr(winid window, int attr, const char *str)
{
    winid new_win;
    struct xwindow *wp;

    check_winid(window);
    wp = &window_list[window];

    switch (wp->type) {
    case NHW_MESSAGE:
        (void) strncpy(g.toplines, str, TBUFSZ); /* for Norep(). */
        g.toplines[TBUFSZ - 1] = 0;
        append_message(wp, str);
        break;
#ifndef STATUS_HILITES
    case NHW_STATUS:
        adjust_status(wp, str);
        break;
#endif
    case NHW_MAP:
        impossible("putstr: called on map window \"%s\"", str);
        break;
    case NHW_MENU:
        if (wp->menu_information->is_menu) {
            impossible("putstr:  called on a menu window, \"%s\" discarded",
                       str);
            break;
        }
        /*
         * Change this menu window into a text window by creating a
         * new text window, then copying it to this winid.
         */
        new_win = X11_create_nhwindow(NHW_TEXT);
        X11_destroy_nhwindow(window);
        *wp = window_list[new_win];
        window_list[new_win].type = NHW_NONE; /* allow re-use */
        /* fall through */
    case NHW_TEXT:
        add_to_text_window(wp, attr, str);
        break;
    default:
        impossible("putstr: unknown window type [%d] \"%s\"", wp->type, str);
    }
}

/* We do event processing as a callback, so this is a null routine. */
void
X11_get_nh_event(void)
{
    return;
}

int
X11_nhgetch(void)
{
    return input_event(EXIT_ON_KEY_PRESS);
}

int
X11_nh_poskey(int *x, int *y, int *mod)
{
    int val = input_event(EXIT_ON_KEY_OR_BUTTON_PRESS);

    if (val == 0) { /* user clicked on a map window */
        *x = click_x;
        *y = click_y;
        *mod = click_button;
    }
    return val;
}

winid
X11_create_nhwindow(int type)
{
    winid window;
    struct xwindow *wp;

    if (!x_inited)
        panic("create_nhwindow:  windows not initialized");

#ifdef X11_HANGUP_SIGNAL
    /* set up our own signal handlers on the first call.  Can't do this in
     * X11_init_nhwindows because unixmain sets its handler after calling
     * all the init routines. */
    if (X11_sig_id == 0) {
        X11_sig_id = XtAppAddSignal(app_context, X11_sig_cb, (XtPointer) 0);
#ifdef SA_RESTART
        {
            struct sigaction sact;

            (void) memset((char *) &sact, 0, sizeof(struct sigaction));
            sact.sa_handler = (SIG_RET_TYPE) X11_sig;
            (void) sigaction(SIGHUP, &sact, (struct sigaction *) 0);
#ifdef SIGXCPU
            (void) sigaction(SIGXCPU, &sact, (struct sigaction *) 0);
#endif
        }
#else
        (void) signal(SIGHUP, (SIG_RET_TYPE) X11_sig);
#ifdef SIGXCPU
        (void) signal(SIGXCPU, (SIG_RET_TYPE) X11_sig);
#endif
#endif
    }
#endif

    /*
     * We have already created the standard message, map, and status
     * windows in the window init routine.  The first window of that
     * type to be created becomes the standard.
     *
     * A better way to do this would be to say that init_nhwindows()
     * has already defined these three windows.
     */
    if (type == NHW_MAP && map_win != WIN_ERR) {
        window = map_win;
        map_win = WIN_ERR;
        return window;
    }
    if (type == NHW_MESSAGE && message_win != WIN_ERR) {
        window = message_win;
        message_win = WIN_ERR;
        return window;
    }
    if (type == NHW_STATUS && status_win != WIN_ERR) {
        window = status_win;
        status_win = WIN_ERR;
        return window;
    }

    window = find_free_window();
    wp = &window_list[window];

    /* The create routines will set type, popup, w, and Win_info. */
    wp->prevx = wp->prevy = wp->cursx = wp->cursy = wp->pixel_width =
        wp->pixel_height = 0;
    wp->keep_window = FALSE;
    wp->nh_colors_inited = FALSE;
    wp->boldfs = (XFontStruct *) 0;
    wp->boldfs_dpy = (Display *) 0;
    wp->title = (char *) 0;

    switch (type) {
    case NHW_MAP:
        create_map_window(wp, TRUE, (Widget) 0);
        break;
    case NHW_MESSAGE:
        create_message_window(wp, TRUE, (Widget) 0);
        break;
    case NHW_STATUS:
        create_status_window(wp, TRUE, (Widget) 0);
        break;
    case NHW_MENU:
        create_menu_window(wp);
        break;
    case NHW_TEXT:
        create_text_window(wp);
        break;
    default:
        panic("create_nhwindow: unknown type [%d]", type);
        break;
    }
    return window;
}

void
X11_clear_nhwindow(winid window)
{
    struct xwindow *wp;

    check_winid(window);
    wp = &window_list[window];

    switch (wp->type) {
    case NHW_MAP:
        clear_map_window(wp);
        break;
    case NHW_TEXT:
        clear_text_window(wp);
        break;
    case NHW_STATUS:
    case NHW_MENU:
    case NHW_MESSAGE:
        /* do nothing for these window types */
        break;
    default:
        panic("clear_nhwindow: unknown window type [%d]", wp->type);
        break;
    }
}

void
X11_display_nhwindow(winid window, boolean blocking)
{
    struct xwindow *wp;

    check_winid(window);
    wp = &window_list[window];

    switch (wp->type) {
    case NHW_MAP:
        if (wp->popup)
            nh_XtPopup(wp->popup, (int) XtGrabNone, wp->w);
        display_map_window(wp); /* flush map */

        /*
         * We need to flush the message window here due to the way the tty
         * port is set up.  To flush a window, you need to call this
         * routine.  However, the tty port _pauses_ with a --more-- if we
         * do a display_nhwindow(WIN_MESSAGE, FALSE).  Thus, we can't call
         * display_nhwindow(WIN_MESSAGE,FALSE) in parse() because then we
         * get a --more-- after every line.
         *
         * Perhaps the window document should mention that when the map
         * is flushed, everything on the three main windows should be
         * flushed.  Note: we don't need to flush the status window
         * because we don't buffer changes.
         */
        if (WIN_MESSAGE != WIN_ERR)
            display_message_window(&window_list[WIN_MESSAGE]);
        if (blocking)
            (void) x_event(EXIT_ON_KEY_OR_BUTTON_PRESS);
        break;
    case NHW_MESSAGE:
        if (wp->popup)
            nh_XtPopup(wp->popup, (int) XtGrabNone, wp->w);
        display_message_window(wp); /* flush messages */
        break;
    case NHW_STATUS:
        if (wp->popup)
            nh_XtPopup(wp->popup, (int) XtGrabNone, wp->w);
        break; /* no flushing necessary */
    case NHW_MENU: {
        int n;
        menu_item *selected;

        /* pop up menu */
        n = X11_select_menu(window, PICK_NONE, &selected);
        if (n) {
            impossible("perminvent: %d selected??", n);
            free((genericptr_t) selected);
        }
        break;
    }
    case NHW_TEXT:
        display_text_window(wp, blocking); /* pop up text window */
        break;
    default:
        panic("display_nhwindow: unknown window type [%d]", wp->type);
        break;
    }
}

void
X11_destroy_nhwindow(winid window)
{
    struct xwindow *wp;

    check_winid(window);
    wp = &window_list[window];
    /*
     * "Zap" known windows, but don't destroy them.  We need to keep the
     * toplevel widget popped up so that later windows (e.g. tombstone)
     * are visible on DECWindow systems.  This is due to the virtual
     * roots that the DECWindow wm creates.
     */
    if (window == WIN_MESSAGE) {
        wp->keep_window = TRUE;
        WIN_MESSAGE = WIN_ERR;
        iflags.window_inited = 0;
    } else if (window == WIN_MAP) {
        wp->keep_window = TRUE;
        WIN_MAP = WIN_ERR;
    } else if (window == WIN_STATUS) {
        wp->keep_window = TRUE;
        WIN_STATUS = WIN_ERR;
    } else if (window == WIN_INVEN) {
        /* don't need to keep this one */
        WIN_INVEN = WIN_ERR;
    }

    if (wp->boldfs) {
        XFreeFont(wp->boldfs_dpy, wp->boldfs);
        wp->boldfs = (XFontStruct *) 0;
        wp->boldfs_dpy = (Display *) 0;
    }

    if (wp->title) {
        free(wp->title);
        wp->title = (char *) 0;
    }

    switch (wp->type) {
    case NHW_MAP:
        destroy_map_window(wp);
        break;
    case NHW_MENU:
        destroy_menu_window(wp);
        break;
    case NHW_TEXT:
        destroy_text_window(wp);
        break;
    case NHW_STATUS:
        destroy_status_window(wp);
        break;
    case NHW_MESSAGE:
        destroy_message_window(wp);
        break;
    default:
        panic("destroy_nhwindow: unknown window type [%d]", wp->type);
        break;
    }
}

/* display persistent inventory in its own window */
void
X11_update_inventory(int arg)
{
    struct xwindow *wp = 0;

    if (!x_inited)
        return;

    if (iflags.perm_invent) {
        /* skip any calls to update_inventory() before in_moveloop starts */
        if (g.program_state.in_moveloop || g.program_state.gameover) {
            updated_inventory = 1; /* hack to avoid mapping&raising window */
            if (!arg) {
                (void) display_inventory((char *) 0, FALSE);
            } else {
                x11_scroll_perminv(arg);
            }
            updated_inventory = 0;
        }
    } else if ((wp = &window_list[WIN_INVEN]) != 0
               && wp->type == NHW_MENU && wp->menu_information->is_up) {
        /* persistent inventory is up but perm_invent is off, take it down */
        x11_no_perminv(wp);
    }
    return;
}

/* The current implementation has all of the saved lines on the screen. */
int
X11_doprev_message(void)
{
    return 0;
}

void
X11_nhbell(void)
{
    /* We can't use XBell until toplevel has been initialized. */
    if (x_inited)
        XBell(XtDisplay(toplevel), 0);
    /* else print ^G ?? */
}

void
X11_mark_synch(void)
{
    if (x_inited) {
        /*
         * The window document is unclear about the status of text
         * that has been pline()d but not displayed w/display_nhwindow().
         * Both the main and tty code assume that a pline() followed
         * by mark_synch() results in the text being seen, even if
         * display_nhwindow() wasn't called.  Duplicate this behavior.
         */
        if (WIN_MESSAGE != WIN_ERR)
            display_message_window(&window_list[WIN_MESSAGE]);
        XSync(XtDisplay(toplevel), False);
    }
}

void
X11_wait_synch(void)
{
    if (x_inited)
        XFlush(XtDisplay(toplevel));
}

/* Both resume_ and suspend_ are called from ioctl.c and unixunix.c. */
void
X11_resume_nhwindows(void)
{
    return;
}
/* ARGSUSED */
void
X11_suspend_nhwindows(const char *str)
{
    nhUse(str);

    return;
}

/* Under X, we don't need to initialize the number pad. */
/* ARGSUSED */
void
X11_number_pad(int state) /* called from options.c */
{
    nhUse(state);

    return;
}

/* called from setftty() in unixtty.c */
void
X11_start_screen(void)
{
    return;
}

/* called from settty() in unixtty.c */
void
X11_end_screen(void)
{
    return;
}

#ifdef GRAPHIC_TOMBSTONE
void
X11_outrip(winid window, int how, time_t when)
{
    struct xwindow *wp;
    FILE *rip_fp = 0;

    check_winid(window);
    wp = &window_list[window];

    /* make sure the graphical tombstone is available; it's not easy to
       revert to the ASCII-art text tombstone once we're past this point */
    if (appResources.tombstone && *appResources.tombstone)
        rip_fp = fopen(appResources.tombstone, "r"); /* "rip.xpm" */
    if (!rip_fp) {
        genl_outrip(window, how, when);
        return;
    }
    (void) fclose(rip_fp);

    if (wp->type == NHW_TEXT) {
        wp->text_information->is_rip = TRUE;
    } else {
        panic("ripout on non-text window (window type [%d])", wp->type);
    }

    calculate_rip_text(how, when);
}
#endif

/* init and exit nhwindows ------------------------------------------------ */

XtAppContext app_context;     /* context of application */
Widget toplevel = (Widget) 0; /* toplevel widget */
Atom wm_delete_window;        /* pop down windows */

static XtActionsRec actions[] = {
    { nhStr("dismiss_text"), dismiss_text }, /* text widget button action */
    { nhStr("delete_text"), delete_text },   /* text widget delete action */
    { nhStr("key_dismiss_text"), key_dismiss_text }, /* text key action   */
#ifdef GRAPHIC_TOMBSTONE
    { nhStr("rip_dismiss_text"), rip_dismiss_text }, /* rip in text widget */
#endif
    { nhStr("menu_key"), menu_key },             /* menu accelerators */
    { nhStr("yn_key"), yn_key },                 /* yn accelerators */
    { nhStr("yn_delete"), yn_delete },           /* yn delete-window */
    { nhStr("askname_delete"), askname_delete }, /* askname delete-window */
    { nhStr("getline_delete"), getline_delete }, /* getline delete-window */
    { nhStr("menu_delete"), menu_delete },       /* menu delete-window */
    { nhStr("ec_key"), ec_key },                 /* extended commands */
    { nhStr("ec_delete"), ec_delete },           /* ext-com menu delete */
    { nhStr("ps_key"), ps_key },                 /* player selection */
    { nhStr("plsel_quit"), plsel_quit },   /* player selection dialog */
    { nhStr("plsel_play"), plsel_play },   /* player selection dialog */
    { nhStr("plsel_rnd"), plsel_randomize }, /* player selection dialog */
    { nhStr("race_key"), race_key },             /* race selection */
    { nhStr("gend_key"), gend_key },             /* gender selection */
    { nhStr("algn_key"), algn_key },             /* alignment selection */
    { nhStr("X11_hangup"), X11_hangup },         /* delete of top-level */
    { nhStr("input"), map_input },               /* key input */
    { nhStr("scroll"), nh_keyscroll },           /* scrolling by keys */
};

static XtResource resources[] = {
    { nhStr("slow"), nhStr("Slow"), XtRBoolean, sizeof(Boolean),
      XtOffset(AppResources *, slow), XtRString, nhStr("True") },
    { nhStr("fancy_status"), nhStr("Fancy_status"),
      XtRBoolean, sizeof(Boolean),
      XtOffset(AppResources *, fancy_status), XtRString, nhStr("True") },
    { nhStr("autofocus"), nhStr("AutoFocus"), XtRBoolean, sizeof(Boolean),
      XtOffset(AppResources *, autofocus), XtRString, nhStr("False") },
    { nhStr("message_line"), nhStr("Message_line"), XtRBoolean,
      sizeof(Boolean), XtOffset(AppResources *, message_line), XtRString,
      nhStr("False") },
    { nhStr("highlight_prompt"), nhStr("Highlight_prompt"), XtRBoolean,
      sizeof(Boolean), XtOffset(AppResources *, highlight_prompt), XtRString,
      nhStr("True") },
    { nhStr("double_tile_size"), nhStr("Double_tile_size"), XtRBoolean,
      sizeof(Boolean), XtOffset(AppResources *, double_tile_size), XtRString,
      nhStr("False") },
    { nhStr("tile_file"), nhStr("Tile_file"), XtRString, sizeof(String),
      XtOffset(AppResources *, tile_file), XtRString, nhStr("x11tiles") },
    { nhStr("icon"), nhStr("Icon"), XtRString, sizeof(String),
      XtOffset(AppResources *, icon), XtRString, nhStr("nh72") },
    { nhStr("message_lines"), nhStr("Message_lines"), XtRInt, sizeof(int),
      XtOffset(AppResources *, message_lines), XtRString, nhStr("12") },
    { nhStr("extcmd_height_delta"), nhStr("Extcmd_height_delta"),
      XtRInt, sizeof (int),
      XtOffset(AppResources *, extcmd_height_delta), XtRString, nhStr("0") },
    { nhStr("pet_mark_bitmap"), nhStr("Pet_mark_bitmap"), XtRString,
      sizeof(String), XtOffset(AppResources *, pet_mark_bitmap), XtRString,
      nhStr("pet_mark.xbm") },
    { nhStr("pet_mark_color"), nhStr("Pet_mark_color"), XtRPixel,
      sizeof(XtRPixel), XtOffset(AppResources *, pet_mark_color), XtRString,
      nhStr("Red") },
    { nhStr("pilemark_bitmap"), nhStr("Pilemark_bitmap"), XtRString,
      sizeof(String), XtOffset(AppResources *, pilemark_bitmap), XtRString,
      nhStr("pilemark.xbm") },
    { nhStr("pilemark_color"), nhStr("Pilemark_color"), XtRPixel,
      sizeof(XtRPixel), XtOffset(AppResources *, pilemark_color), XtRString,
      nhStr("Green") },
#ifdef GRAPHIC_TOMBSTONE
    { nhStr("tombstone"), nhStr("Tombstone"), XtRString, sizeof(String),
      XtOffset(AppResources *, tombstone), XtRString, nhStr("rip.xpm") },
    { nhStr("tombtext_x"), nhStr("Tombtext_x"), XtRInt, sizeof(int),
      XtOffset(AppResources *, tombtext_x), XtRString, nhStr("155") },
    { nhStr("tombtext_y"), nhStr("Tombtext_y"), XtRInt, sizeof(int),
      XtOffset(AppResources *, tombtext_y), XtRString, nhStr("78") },
    { nhStr("tombtext_dx"), nhStr("Tombtext_dx"), XtRInt, sizeof(int),
      XtOffset(AppResources *, tombtext_dx), XtRString, nhStr("0") },
    { nhStr("tombtext_dy"), nhStr("Tombtext_dy"), XtRInt, sizeof(int),
      XtOffset(AppResources *, tombtext_dy), XtRString, nhStr("13") },
#endif
};

static int
panic_on_error(Display *display, XErrorEvent *error)
{
    char buf[BUFSZ];
    XGetErrorText(display, error->error_code, buf, BUFSZ);
    fprintf(stderr,
            "X Error: code %i (%s), request %i, minor %i, serial %lu\n",
            error->error_code, buf,
            error->request_code, error->minor_code, error->serial);
    panic("X Error");
    return 0;
}

static void
X11_error_handler(String str)
{
    nhUse(str);
    hangup(1);
}

static int
X11_io_error_handler(Display *display)
{
    nhUse(display);
    hangup(1);
    return 0;
}

void
X11_init_nhwindows(int *argcp, char **argv)
{
    int i;
    Cardinal num_args;
    Arg args[4];
    uid_t savuid;

    /* Init windows to nothing. */
    for (i = 0; i < MAX_WINDOWS; i++)
        window_list[i].type = NHW_NONE;

    /* add another option that can be set */
    set_wc_option_mod_status(WC_TILED_MAP, set_in_game);
    set_option_mod_status("mouse_support", set_in_game);

    load_default_resources(); /* create default_resource_data[] */

    /*
     * setuid hack: make sure that if nethack is setuid, to use real uid
     * when opening X11 connections, in case the user is using xauth, since
     * the "games" or whatever user probably doesn't have permission to open
     * a window on the user's display.  This code is harmless if the binary
     * is not installed setuid.  See include/system.h on compilation failures.
     */
    savuid = geteuid();
    (void) seteuid(getuid());

    XSetIOErrorHandler((XIOErrorHandler) X11_io_error_handler);

    num_args = 0;
    XtSetArg(args[num_args], XtNallowShellResize, True); num_args++;
    XtSetArg(args[num_args], XtNtitle, "NetHack"); num_args++;

    toplevel = XtAppInitialize(&app_context, "NetHack",     /* application  */
                               (XrmOptionDescList) 0, 0,    /* options list */
                               argcp, (String *) argv,      /* command line */
                               default_resource_data, /* fallback resources */
                               (ArgList) args, num_args);
    XtOverrideTranslations(toplevel,
              XtParseTranslationTable("<Message>WM_PROTOCOLS: X11_hangup()"));

    /* We don't need to realize the top level widget. */

    old_error_handler = XSetErrorHandler(panic_on_error);

#ifdef TEXTCOLOR
    /* add new color converter to deal with overused colormaps */
    XtSetTypeConverter(XtRString, XtRPixel, nhCvtStringToPixel,
                       (XtConvertArgList) nhcolorConvertArgs,
                       XtNumber(nhcolorConvertArgs), XtCacheByDisplay,
                       nhFreePixel);
#endif /* TEXTCOLOR */

    /* Register the actions mentioned in "actions". */
    XtAppAddActions(app_context, actions, XtNumber(actions));

    /* Get application-wide resources */
    XtGetApplicationResources(toplevel, (XtPointer) &appResources, resources,
                              XtNumber(resources), (ArgList) 0, ZERO);

    /* Initialize other things. */
    init_standard_windows();

    /* Give the window manager an icon to use;  toplevel must be realized. */
    if (appResources.icon && *appResources.icon) {
        struct icon_info *ip;

        for (ip = icon_data; ip->name; ip++)
            if (!strcmp(appResources.icon, ip->name)) {
                icon_pixmap = XCreateBitmapFromData(
                    XtDisplay(toplevel), XtWindow(toplevel),
                    (genericptr_t) ip->bits, ip->width, ip->height);
                if (icon_pixmap != None) {
                    XWMHints hints;

                    (void) memset((genericptr_t) &hints, 0, sizeof(XWMHints));
                    hints.flags = IconPixmapHint;
                    hints.icon_pixmap = icon_pixmap;
                    XSetWMHints(XtDisplay(toplevel), XtWindow(toplevel),
                                &hints);
                }
                break;
            }
    }

    /* end of setuid hack: reset uid back to the "games" uid */
    (void) seteuid(savuid);

    x_inited = TRUE; /* X is now initialized */
    plsel_ask_name = FALSE;

    release_default_resources();

    /* Display the startup banner in the message window. */
    for (i = 1; i <= 4 + 2; ++i) /* (values beyond 4 yield blank lines) */
        X11_putstr(WIN_MESSAGE, 0, copyright_banner_line(i));
}

/*
 * All done.
 */
/* ARGSUSED */
void
X11_exit_nhwindows(const char *dummy)
{
    extern Pixmap tile_pixmap; /* from winmap.c */

    nhUse(dummy);

    /* explicitly free the icon and tile pixmaps */
    if (icon_pixmap != None) {
        XFreePixmap(XtDisplay(toplevel), icon_pixmap);
        icon_pixmap = None;
    }
    if (tile_pixmap != None) {
        XFreePixmap(XtDisplay(toplevel), tile_pixmap);
        tile_pixmap = None;
    }
    if (WIN_INVEN != WIN_ERR)
        X11_destroy_nhwindow(WIN_INVEN);
    if (WIN_STATUS != WIN_ERR)
        X11_destroy_nhwindow(WIN_STATUS);
    if (WIN_MAP != WIN_ERR)
        X11_destroy_nhwindow(WIN_MAP);
    if (WIN_MESSAGE != WIN_ERR)
        X11_destroy_nhwindow(WIN_MESSAGE);

    release_getline_widgets();
    release_yn_widgets();
}

#ifdef X11_HANGUP_SIGNAL
static void
X11_sig(int sig) /* Unix signal handler */
{
    XtNoticeSignal(X11_sig_id);
    hangup(sig);
}

static void
X11_sig_cb(XtPointer not_used, XtSignalId *id)
{
    XEvent event;
    XClientMessageEvent *mesg;

    nhUse(not_used);
    nhUse(id);

    /* Set up a fake message to the event handler. */
    mesg = (XClientMessageEvent *) &event;
    mesg->type = ClientMessage;
    mesg->message_type = XA_STRING;
    mesg->format = 8;

    XSendEvent(XtDisplay(window_list[WIN_MAP].w),
               XtWindow(window_list[WIN_MAP].w), False, NoEventMask,
               (XEvent *) mesg);
}
#endif

/* delay_output ----------------------------------------------------------- */

/*
 * Timeout callback for delay_output().  Send a fake message to the map
 * window.
 */
/* ARGSUSED */
static void
d_timeout(XtPointer client_data, XtIntervalId *id)
{
    XEvent event;
    XClientMessageEvent *mesg;

    nhUse(client_data);
    nhUse(id);

    /* Set up a fake message to the event handler. */
    mesg = (XClientMessageEvent *) &event;
    mesg->type = ClientMessage;
    mesg->message_type = XA_STRING;
    mesg->format = 8;
    XSendEvent(XtDisplay(window_list[WIN_MAP].w),
               XtWindow(window_list[WIN_MAP].w), False, NoEventMask,
               (XEvent *) mesg);
}

/*
 * Delay for 50ms.  This is not implemented asynch.  Maybe later.
 * Start the timeout, then wait in the event loop.  The timeout
 * function will send an event to the map window which will be waiting
 * for a sent event.
 */
void
X11_delay_output(void)
{
    if (!x_inited)
        return;

    (void) XtAppAddTimeOut(app_context, 30L, d_timeout, (XtPointer) 0);

    /* The timeout function will enable the event loop exit. */
    (void) x_event(EXIT_ON_SENT_EVENT);
}

/* X11_hangup ------------------------------------------------------------- */
/* ARGSUSED */
static void
X11_hangup(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    nhUse(w);
    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    hangup(1); /* 1 is commonly SIGHUP, but ignored anyway */
    exit_x_event = TRUE;
}

/* X11_bail --------------------------------------------------------------- */
/* clean up and quit */
static void
X11_bail(const char *mesg)
{
    g.program_state.something_worth_saving = 0;
    clearlocks();
    X11_exit_nhwindows(mesg);
    nh_terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}

/* askname ---------------------------------------------------------------- */
/* ARGSUSED */
static void
askname_delete(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    nh_XtPopdown(w);
    (void) strcpy(g.plname, "Mumbles"); /* give them a name... ;-) */
    exit_x_event = TRUE;
}

/* Callback for askname dialog widget. */
/* ARGSUSED */
static void
askname_done(Widget w, XtPointer client_data, XtPointer call_data)
{
    unsigned len;
    char *s;
    Widget dialog = (Widget) client_data;

    nhUse(w);
    nhUse(call_data);

    s = (char *) GetDialogResponse(dialog);

    len = strlen(s);
    if (len == 0) {
        X11_nhbell();
        return;
    }

    /* Truncate name if necessary */
    if (len >= sizeof g.plname - 1)
        len = sizeof g.plname - 1;

    (void) strncpy(g.plname, s, len);
    g.plname[len] = '\0';
    XtFree(s);

    nh_XtPopdown(XtParent(dialog));
    exit_x_event = TRUE;
}

/* ask player for character's name to replace generic name "player" (or other
   values; see config.h) after 'nethack -u player' or OPTIONS=name:player */
void
X11_askname(void)
{
    Widget popup, dialog;
    Arg args[1];

#ifdef SELECTSAVED
    if (iflags.wc2_selectsaved && !iflags.renameinprogress)
        switch (restore_menu(WIN_MAP)) {
        case -1: /* quit */
            X11_bail("Until next time then...");
            /*NOTREACHED*/
        case 0: /* no game chosen; start new game */
            break;
        case 1: /* save game selected, plname[] has been set */
            return;
        }
#else
    nhUse(X11_bail);
#endif /* SELECTSAVED */

    if (iflags.wc_player_selection == VIA_DIALOG) {
        /* X11_player_selection_dialog() handles name query */
        plsel_ask_name = TRUE;
        iflags.defer_plname = TRUE;
        return;
    } /* else iflags.wc_player_selection == VIA_PROMPTS */

    XtSetArg(args[0], XtNallowShellResize, True);

    popup = XtCreatePopupShell("askname", transientShellWidgetClass, toplevel,
                               args, ONE);
    XtOverrideTranslations(popup,
          XtParseTranslationTable("<Message>WM_PROTOCOLS: askname_delete()"));

    dialog = CreateDialog(popup, nhStr("dialog"), askname_done,
                          (XtCallbackProc) 0);

    SetDialogPrompt(dialog, nhStr("What is your name?")); /* set prompt */
    SetDialogResponse(dialog, g.plname, PL_NSIZ); /* set default answer */

    XtRealizeWidget(popup);
    positionpopup(popup, TRUE); /* center,bottom */

    nh_XtPopup(popup, (int) XtGrabExclusive, dialog);

    /* The callback will enable the event loop exit. */
    (void) x_event(EXIT_ON_EXIT);

    /* tty's character selection uses this; we might someday;
       since we let user pick an arbitrary name now, he/she can
       pick another one during role selection */
    iflags.renameallowed = TRUE;

    XtDestroyWidget(dialog);
    XtDestroyWidget(popup);
}

/* getline ---------------------------------------------------------------- */
/* This uses Tim Theisen's dialog widget set (from GhostView). */

static Widget getline_popup, getline_dialog;

#define CANCEL_STR "\033"
static char *getline_input; /* buffer to hold user input; getline's output */

/* Callback for getline dialog widget. */
/* ARGSUSED */
static void
done_button(Widget w, XtPointer client_data, XtPointer call_data)
{
    int len;
    char *s;
    Widget dialog = (Widget) client_data;

    nhUse(w);
    nhUse(call_data);

    s = (char *) GetDialogResponse(dialog);
    len = strlen(s);

    /* Truncate input if necessary */
    if (len >= BUFSZ)
        len = BUFSZ - 1;

    (void) strncpy(getline_input, s, len);
    getline_input[len] = '\0';
    XtFree(s);

    nh_XtPopdown(XtParent(dialog));
    exit_x_event = TRUE;
}

/* ARGSUSED */
static void
getline_delete(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    Strcpy(getline_input, CANCEL_STR);
    nh_XtPopdown(w);
    exit_x_event = TRUE;
}

/* Callback for getline dialog widget. */
/* ARGSUSED */
static void
abort_button(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget dialog = (Widget) client_data;

    nhUse(w);
    nhUse(call_data);

    Strcpy(getline_input, CANCEL_STR);
    nh_XtPopdown(XtParent(dialog));
    exit_x_event = TRUE;
}

static void
release_getline_widgets(void)
{
    if (getline_dialog)
        XtDestroyWidget(getline_dialog), getline_dialog = (Widget) 0;
    if (getline_popup)
        XtDestroyWidget(getline_popup), getline_popup = (Widget) 0;
}

/* ask user for a line of text */
void
X11_getlin(const char *question, /* prompt */
           char *input)          /* user's input, getlin's _output_ buffer */
{
    getline_input = input; /* used by popup actions */

    flush_screen(1); /* tell core to make sure that map is up to date */
    if (!getline_popup) {
        Arg args[1];

        XtSetArg(args[0], XtNallowShellResize, True);

        getline_popup = XtCreatePopupShell("getline",
                                           transientShellWidgetClass,
                                           toplevel, args, ONE);
        XtOverrideTranslations(getline_popup,
                               XtParseTranslationTable(
                                  "<Message>WM_PROTOCOLS: getline_delete()"));

        getline_dialog = CreateDialog(getline_popup, nhStr("dialog"),
                                      done_button, abort_button);

        XtRealizeWidget(getline_popup);
        XSetWMProtocols(XtDisplay(getline_popup), XtWindow(getline_popup),
                        &wm_delete_window, 1);
    }
    SetDialogPrompt(getline_dialog, (String) question); /* set prompt */
    /* 60:  make the answer widget be wide enough to hold 60 characters,
       or the length of the prompt string, whichever is bigger.  User's
       response can be longer--when limit is reached, value-so-far will
       slide left hiding some chars at the beginning of the response but
       making room to type more.  [Prior to 3.6.1, width wasn't specifiable
       and answer box always got sized to match the width of the prompt.] */
#ifdef EDIT_GETLIN
    SetDialogResponse(getline_dialog, input, 60); /* set default answer */
#else
    SetDialogResponse(getline_dialog, nhStr(""), 60); /* set default answer */
#endif
    positionpopup(getline_popup, TRUE);           /* center,bottom */

    nh_XtPopup(getline_popup, (int) XtGrabExclusive, getline_dialog);

    /* The callback will enable the event loop exit. */
    (void) x_event(EXIT_ON_EXIT);

    /* we get here after the popup has exited;
       put prompt and response into the message window (and into
       core's dumplog history) unless play hasn't started yet */
    if (g.program_state.in_moveloop || g.program_state.gameover) {
        /* single space has meaning (to remove a previously applied name) so
           show it clearly; don't care about legibility of multiple spaces */
        const char *visanswer = !input[0] ? "<empty>"
                                : (input[0] == ' ' && !input[1]) ? "<space>"
                                  : (input[0] == '\033') ? "<esc>"
                                    : input;
        int promptlen = (int) strlen(question),
            answerlen = (int) strlen(visanswer);

        /* prompt should be limited to QBUFSZ-1 by caller; enforce that
           here for the echoed value; answer could be up to BUFSZ-1 long;
           pline() will truncate the whole message to that amount so we
           truncate the answer for display; this only affects the echoed
           response, not the actual response being returned to the core */
        if (promptlen >= QBUFSZ)
            promptlen = QBUFSZ - 1;
        if (answerlen + 1 + promptlen >= BUFSZ) /* +1: separating space */
            answerlen = BUFSZ - (1 + promptlen) - 1;
        pline("%.*s %.*s", promptlen, question, answerlen, visanswer);
    }

    /* clear static pointer that's about to go stale */
    getline_input = 0;
}

/* Display file ----------------------------------------------------------- */

/* uses a menu (with no selectors specified) rather than a text window
   to allow previous_page and first_menu actions to move backwards */
void
X11_display_file(const char *str, boolean complain)
{
    dlb *fp;
    winid newwin;
    struct xwindow *wp;
    anything any;
    menu_item *menu_list;
#define LLEN 128
    char line[LLEN];

    /* Use the port-independent file opener to see if the file exists. */
    fp = dlb_fopen(str, RDTMODE);
    if (!fp) {
        if (complain)
            pline("Cannot open %s.  Sorry.", str);
        return; /* it doesn't exist, ignore */
    }

    newwin = X11_create_nhwindow(NHW_MENU);
    wp = &window_list[newwin];
    X11_start_menu(newwin, MENU_BEHAVE_STANDARD);

    any = cg.zeroany;
    while (dlb_fgets(line, LLEN, fp)) {
        X11_add_menu(newwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                     line, MENU_ITEMFLAGS_NONE);
    }
    (void) dlb_fclose(fp);

    /* show file name as the window title */
    if (str)
        wp->title = dupstr(str);

    wp->menu_information->permi = FALSE;
    wp->menu_information->disable_mcolors = TRUE;
    (void) X11_select_menu(newwin, PICK_NONE, &menu_list);
    X11_destroy_nhwindow(newwin);
}

/* yn_function ------------------------------------------------------------ */
/* (not threaded) */

static const char *yn_quitchars = " \n\r";
static const char *yn_choices; /* string of acceptable input */
static char yn_def;
static char yn_return;           /* return value */
static char yn_esc_map;          /* ESC maps to this char. */
static Widget yn_popup;          /* popup for the yn fuction (created once) */
static Widget yn_label;          /* label for yn function (created once) */
static boolean yn_getting_num;   /* TRUE if accepting digits */
static boolean yn_preserve_case; /* default is to force yn to lower case */
static boolean yn_no_default;    /* don't convert ESC or quitchars to def */
static int yn_ndigits;           /* digit count */
static long yn_val;              /* accumulated value */

static const char yn_translations[] = "#override\n\
     <Key>: yn_key()";

/*
 * Convert the given key event into a character.  If the key maps to
 * more than one character only the first is returned.  If there is
 * no conversion (i.e. just the CTRL key hit) a NUL is returned.
 */
char
key_event_to_char(XKeyEvent *key)
{
    char keystring[MAX_KEY_STRING];
    int nbytes;
    boolean meta = !!(key->state & Mod1Mask);

    nbytes = XLookupString(key, keystring, MAX_KEY_STRING, (KeySym *) 0,
                           (XComposeStatus *) 0);

    /* Modifier keys return a zero lengh string when pressed. */
    if (nbytes == 0)
        return '\0';

    return (char) (((int) keystring[0]) + (meta ? 0x80 : 0));
}

/*
 * Called when we get a WM_DELETE_WINDOW event on a yn window.
 */
/* ARGSUSED */
static void
yn_delete(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    nhUse(w);
    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    yn_getting_num = FALSE;
    /* Only use yn_esc_map if we have choices.  Otherwise, return ESC. */
    yn_return = yn_choices ? yn_esc_map : '\033';
    exit_x_event = TRUE; /* exit our event handler */
}

/*
 * Called when we get a key press event on a yn window.
 */
/* ARGSUSED */
static void
yn_key(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    char ch;

    if (appResources.slow && !input_func)
        map_input(w, event, params, num_params);

    ch = key_event_to_char((XKeyEvent *) event);

    if (ch == '\0') { /* don't accept nul char or modifier event */
        /* no bell */
        return;
    }

    if (!yn_choices /* accept any input */
        || (yn_no_default && (ch == '\033' || index(yn_quitchars, ch)))) {
        yn_return = ch;
    } else {
        if (!yn_preserve_case)
            ch = lowc(ch); /* move to lower case */

        if (ch == '\033') {
            yn_getting_num = FALSE;
            yn_return = yn_esc_map;
        } else if (index(yn_quitchars, ch)) {
            yn_return = yn_def;
        } else if (index(yn_choices, ch)) {
            if (ch == '#') {
                if (yn_getting_num) { /* don't select again */
                    X11_nhbell();
                    return;
                }
                yn_getting_num = TRUE;
                yn_ndigits = 0;
                yn_val = 0;
                return; /* wait for more input */
            }
            yn_return = ch;
            if (ch != 'y')
                yn_getting_num = FALSE;
        } else {
            if (yn_getting_num) {
                if (digit(ch)) {
                    yn_ndigits++;
                    yn_val = (yn_val * 10) + (long) (ch - '0');
                    return; /* wait for more input */
                }
                if (yn_ndigits && (ch == '\b' || ch == 127 /*DEL*/)) {
                    yn_ndigits--;
                    yn_val = yn_val / 10;
                    return; /* wait for more input */
                }
            }
            X11_nhbell(); /* no match */
            return;
        }

        if (yn_getting_num) {
            yn_return = '#';
            if (yn_val < 0)
                yn_val = 0;
            yn_number = yn_val; /* assign global */
        }
    }
    exit_x_event = TRUE; /* exit our event handler */
}

/* called at exit time */
static void
release_yn_widgets(void)
{
    if (yn_label)
        XtDestroyWidget(yn_label), yn_label = (Widget) 0;
    if (yn_popup)
        XtDestroyWidget(yn_popup), yn_popup = (Widget) 0;
}

/* guts of the X11_yn_function(), not to be confused with core code;
   requires an extra argument: ynflags */
char
X11_yn_function_core(
    const char *ques,     /* prompt text */
    const char *choices,  /* allowed response chars; any char if Null */
    char def,             /* default if user hits <space> or <return> */
    unsigned ynflags)     /* flags; currently just log-it or not */
{
    static XFontStruct *yn_font = 0;
    static Dimension yn_minwidth = 0;
    char buf[BUFSZ], buf2[BUFSZ];
    Arg args[4];
    Cardinal num_args;
    boolean suppress_logging = (ynflags & YN_NO_LOGMESG) != 0U,
            no_default_cnvrt = (ynflags & YN_NO_DEFAULT) != 0U;

    yn_choices = choices; /* set up globals for callback to use */
    yn_def = def;
    yn_no_default = no_default_cnvrt;
    yn_preserve_case = !choices; /* preserve case when an arbitrary
                                  * response is allowed */

    /*
     * This is sort of a kludge.  There are quite a few places in the main
     * nethack code where a pline containing information is followed by a
     * call to yn_function().  There is no flush of the message window
     * (it is implicit in the tty window port), so the line never shows
     * up for us!  Solution: do our own flush.
     */
    if (WIN_MESSAGE != WIN_ERR)
        display_message_window(&window_list[WIN_MESSAGE]);

    if (choices) {
        char *cb, choicebuf[QBUFSZ];

        Strcpy(choicebuf, choices); /* anything beyond <esc> is hidden */
        /* default when choices are present is to force yn answer to
           lowercase unless one or more choices are explicitly uppercase;
           check this before stripping the hidden choices */
        for (cb = choicebuf; *cb; ++cb)
            if ('A' <= *cb && *cb <= 'Z') {
                yn_preserve_case = TRUE;
                break;
            }
        if ((cb = index(choicebuf, '\033')) != 0)
            *cb = '\0';
        /* ques [choices] (def) */
        int ln = ((int) strlen(ques)        /* prompt text */
                  + 3                       /* " []" */
                  + (int) strlen(choicebuf) /* choices within "[]" */
                  + 4                       /* " (c)" */
                  + 1                       /* a trailing space */
                  + 1);                     /* \0 terminator */
        if (ln >= BUFSZ)
            panic("X11_yn_function:  question too long (%d)", ln);
        Snprintf(buf, sizeof buf, "%.*s [%s]", QBUFSZ - 1, ques, choicebuf);
        if (def)
            Sprintf(eos(buf), " (%c)", def);
        Strcat(buf, " ");

        /* escape maps to 'q' or 'n' or default, in that order */
        yn_esc_map = (index(choices, 'q') ? 'q'
                      : index(choices, 'n') ? 'n'
                        : def);
    } else {
        int ln = ((int) strlen(ques)        /* prompt text */
                  + 1                       /* a trailing space */
                  + 1);                     /* \0 terminator */
        if (ln >= BUFSZ)
            panic("X11_yn_function:  question too long (%d)", ln);
        Strcpy(buf, ques);
        Strcat(buf, " ");
    }
    /* for popup-style, add some extra elbow room to the prompt to
       enhance its visibility; there's no cursor shown, just the text */
    if (!appResources.slow) {
        /* insert one leading space and two extra trailing spaces */
        Strcpy(buf2, buf);
        Snprintf(buf, sizeof buf, " %s  ", buf2);
    }

    /*
     * The 'slow' resource is misleadingly named.  When it is True, we
     * re-use the same one-line widget above the map for all yn prompt
     * responses instead of re-using a popup widget for each one.  It's
     * crucial for client/server configs where the server is slow putting
     * up a popup or requires click-to-focus to respond to the popup, but
     * is also useful for players who just want to be able to look at the
     * same location to read questions they're being asked to answer.
     */

    if (appResources.slow) {
        /*
         * 'slow' is True:  the yn_label widget was created when the map
         * and status widgets were, and is positioned between them.  It
         * will persist until end of game.  All we need to do for
         * yn_function is direct keystroke input to the yn response
         * handler and reset its label to be the prompt text (below).
         */
        input_func = yn_key;
        highlight_yn(FALSE); /* expose yn_label as separate from map */
    } else {
        /*
         * 'slow' is False; create a persistent widget that will be popped
         * up as needed, then down again, and last until end of game.  The
         * associated yn_label widget is used to track whether it exists.
         */
        if (!yn_label) {
            XtSetArg(args[0], XtNallowShellResize, True);
            yn_popup = XtCreatePopupShell("query", transientShellWidgetClass,
                                          toplevel, args, ONE);
            XtOverrideTranslations(yn_popup,
               XtParseTranslationTable("<Message>WM_PROTOCOLS: yn_delete()"));

            num_args = 0;
            XtSetArg(args[num_args], nhStr(XtNjustify), XtJustifyLeft);
                                                                   num_args++;
            XtSetArg(args[num_args], XtNtranslations,
                     XtParseTranslationTable(yn_translations)); num_args++;
            yn_label = XtCreateManagedWidget("yn_label", labelWidgetClass,
                                             yn_popup, args, num_args);

            XtRealizeWidget(yn_popup);
            XSetWMProtocols(XtDisplay(yn_popup), XtWindow(yn_popup),
                            &wm_delete_window, 1);

            /* get font that will be used; we'll need it to measure text */
            (void) memset((genericptr_t) args, 0, sizeof args);
            XtSetArg(args[0], nhStr(XtNfont), &yn_font);
            XtGetValues(yn_label, args, ONE);

            /* set up minimum yn_label width; we don't actually set
               the XtNminWidth attribute */
            (void) memset(buf2, 'X', 25), buf2[25] = '\0'; /* 25 'X's */
            yn_minwidth = (Dimension) XTextWidth(yn_font, buf2,
                                                 (int) strlen(buf2));
        }
    }

    /* set the label of the yn widget to be the prompt text */
    (void) memset((genericptr_t) args, 0, sizeof args);
    num_args = 0;
    XtSetArg(args[num_args], XtNlabel, buf); num_args++;
    XtSetValues(yn_label, args, num_args);

    /* for !slow, pop up the prompt+response widget */
    if (!appResources.slow) {
        /*
         * Setting the text doesn't always perform a resize when it
         * should.  We used to just set the text a second time, but that
         * wasn't a reliable workaround, at least on OSX with XQuartz.
         * This seems to work reliably.
         *
         * It also enforces a minimum prompt width, which wasn't being
         * done before, so that really short prompts are more noticeable
         * if they pop up where the pointer is parked and it happens to
         * be setting somewhere the player isn't looking.
         */
        Dimension promptwidth, labelwidth = 0;

        (void) memset((genericptr_t) args, 0, sizeof args);
        num_args = 0;
        XtSetArg(args[num_args], XtNwidth, &labelwidth); num_args++;
        XtGetValues(yn_label, args, num_args);

        promptwidth = (Dimension) XTextWidth(yn_font, buf, (int) strlen(buf));
        if (labelwidth != promptwidth || labelwidth < yn_minwidth) {
            labelwidth = max(promptwidth, yn_minwidth);
            (void) memset((genericptr_t) args, 0, sizeof args);
            num_args = 0;
            XtSetArg(args[num_args], XtNwidth, labelwidth); num_args++;
            XtSetValues(yn_label, args, num_args);
        }

        positionpopup(yn_popup, TRUE);
        nh_XtPopup(yn_popup, (int) XtGrabExclusive, yn_label);
    }

    yn_getting_num = FALSE;
    (void) x_event(EXIT_ON_EXIT); /* get keystroke(s) */

    /* erase or remove the prompt */
    if (appResources.slow) {
        (void) memset((genericptr_t) args, 0, sizeof args);
        num_args = 0;
        XtSetArg(args[num_args], XtNlabel, " "); num_args++;
        XtSetValues(yn_label, args, num_args);

        input_func = 0; /* keystrokes now belong to the map */
        highlight_yn(FALSE); /* disguise yn_label as part of map */
    } else {
        nh_XtPopdown(yn_popup); /* this removes the event grab */
    }

    if (!suppress_logging) {
        char *p = trimspaces(buf); /* remove !slow's extra whitespace */

        pline("%s %s", p, (yn_return == '\033') ? "ESC" : visctrl(yn_return));
    }

    return yn_return;
}

/* X11-specific edition of yn_function(), the routine called by the core
   to show a prompt and get a single key answer, often 'y' vs 'n' */
char
X11_yn_function(
    const char *ques,     /* prompt text */
    const char *choices,  /* allowed response chars; any char if Null */
    char def)             /* default if user hits <space> or <return> */
{
    char res = X11_yn_function_core(ques, choices, def, YN_NORMAL);

    return res;
}

/* used when processing window-capability-specific run-time options;
   we support toggling tiles on and off via iflags.wc_tiled_map */
void
X11_preference_update(const char *pref)
{
    if (!strcmp(pref, "tiled_map")) {
        if (WIN_MAP != WIN_ERR)
            display_map_window(&window_list[WIN_MAP]);
    } else if (!strcmp(pref, "perm_invent")) {
        ; /* TODO... */
    }
}

/* End global functions =================================================== */

/*
 * Before we wait for input via nhgetch() and nh_poskey(), we need to
 * do some pre-processing.
 */
static int
input_event(int exit_condition)
{
    if (appResources.fancy_status && WIN_STATUS != WIN_ERR)
        check_turn_events(); /* hilighting on the fancy status window */
    if (WIN_MAP != WIN_ERR) /* make sure cursor is not clipped */
        check_cursor_visibility(&window_list[WIN_MAP]);
    if (WIN_MESSAGE != WIN_ERR) /* reset pause line */
        set_last_pause(&window_list[WIN_MESSAGE]);

    return x_event(exit_condition);
}

/*ARGSUSED*/
void
msgkey(Widget w, XtPointer data, XEvent *event, Boolean *continue_to_dispatch)
{
    Cardinal num = 0;

    nhUse(w);
    nhUse(data);
    nhUse(continue_to_dispatch);

    map_input(window_list[WIN_MAP].w, event, (String *) 0, &num);
}

/* only called for autofocus */
/*ARGSUSED*/
static void
win_visible(Widget w, XtPointer data, /* client_data not used */
            XEvent *event,
            Boolean *flag) /* continue_to_dispatch flag not used */
{
    XVisibilityEvent *vis_event = (XVisibilityEvent *) event;

    nhUse(data);
    nhUse(flag);

    if (vis_event->state != VisibilityFullyObscured) {
        /* one-time operation; cancel ourself */
        XtRemoveEventHandler(toplevel, VisibilityChangeMask, False,
                             win_visible, (XtPointer) 0);
        /* grab initial input focus */
        XSetInputFocus(XtDisplay(w), XtWindow(w), RevertToNone, CurrentTime);
    }
}

/* if 'slow' and 'highlight_prompt', set the yn_label widget to look like
   part of the map when idle or to invert background and foreground when
   a prompt is active */
void
highlight_yn(boolean init)
{
    struct xwindow *xmap;

    if (!appResources.slow || !appResources.highlight_prompt)
        return;

    /* first time through, WIN_MAP isn't fully initiialized yet */
    xmap = ((map_win != WIN_ERR) ? &window_list[map_win]
               : (WIN_MAP != WIN_ERR) ? &window_list[WIN_MAP] : 0);

    if (init && xmap) {
        Arg args[2];
        XGCValues vals;
        unsigned long fg_bg = (GCForeground | GCBackground);
        GC gc = (xmap->map_information->is_tile
                    ? xmap->map_information->tile_map.white_gc
                    : xmap->map_information->text_map.copy_gc);

        (void) memset((genericptr_t) &vals, 0, sizeof vals);
        if (XGetGCValues(XtDisplay(xmap->w), gc, fg_bg, &vals)) {
            XtSetArg(args[0], XtNforeground, vals.foreground);
            XtSetArg(args[1], XtNbackground, vals.background);
            XtSetValues(yn_label, args, TWO);
        }
    } else
        swap_fg_bg(yn_label);
}

/*
 * Set up the playing console.  This has three major parts:  the
 * message window, the map, and the status window.
 *
 * For configs specifying the 'slow' resource, the yn_label widget
 * is placed above the map and below the message window.  Prompts
 * requiring a single character response are displayed there rather
 * than using a popup.
 */
static void
init_standard_windows(void)
{
    Widget form, message_viewport, map_viewport, status;
    Arg args[8];
    Cardinal num_args;
    Dimension message_vp_width, map_vp_width, status_width, max_width;
    int map_vp_hd, status_hd;
    struct xwindow *wp;

    num_args = 0;
    XtSetArg(args[num_args], XtNallowShellResize, True); num_args++;
    form = XtCreateManagedWidget("nethack", panedWidgetClass, toplevel, args,
                                 num_args);

    XtAddEventHandler(form, KeyPressMask, False, (XtEventHandler) msgkey,
                      (XtPointer) 0);

    if (appResources.autofocus)
        XtAddEventHandler(toplevel, VisibilityChangeMask, False, win_visible,
                          (XtPointer) 0);

    /*
     * Create message window.
     */
    WIN_MESSAGE = message_win = find_free_window();
    wp = &window_list[message_win];
    wp->cursx = wp->cursy = wp->pixel_width = wp->pixel_height = 0;
    wp->popup = (Widget) 0;
    create_message_window(wp, FALSE, form);
    message_viewport = XtParent(wp->w);

    /* Tell the form that contains it that resizes are OK. */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNresizable), True); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetValues(message_viewport, args, num_args);

    if (appResources.slow) {
        num_args = 0;
        XtSetArg(args[num_args], XtNtranslations,
                 XtParseTranslationTable(yn_translations)); num_args++;
        yn_label = XtCreateManagedWidget("yn_label", labelWidgetClass, form,
                                         args, num_args);
        num_args = 0;
        XtSetArg(args[num_args], nhStr(XtNfromVert), message_viewport);
                                                                   num_args++;
        XtSetArg(args[num_args], nhStr(XtNjustify), XtJustifyLeft);
                                                                  num_args++;
        XtSetArg(args[num_args], nhStr(XtNresizable), True); num_args++;
        XtSetArg(args[num_args], nhStr(XtNlabel), " "); num_args++;
        XtSetValues(yn_label, args, num_args);
    }

    /*
     * Create the map window & viewport and chain the viewport beneath the
     * message_viewport.
     */
    map_win = find_free_window();
    wp = &window_list[map_win];
    wp->cursx = wp->cursy = wp->pixel_width = wp->pixel_height = 0;
    wp->popup = (Widget) 0;
    create_map_window(wp, FALSE, form);
    map_viewport = XtParent(wp->w);

    /* Chain beneath message_viewport or yn window. */
    num_args = 0;
    if (appResources.slow) {
        XtSetArg(args[num_args], nhStr(XtNfromVert), yn_label); num_args++;
    } else {
        XtSetArg(args[num_args], nhStr(XtNfromVert), message_viewport);
                                                                   num_args++;
    }
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainBottom); num_args++;
    XtSetValues(map_viewport, args, num_args);

    /* Create the status window, with the form as it's parent. */
    status_win = find_free_window();
    wp = &window_list[status_win];
    wp->cursx = wp->cursy = wp->pixel_width = wp->pixel_height = 0;
    wp->popup = (Widget) 0;
    create_status_window(wp, FALSE, form);
    status = wp->w;

    /*
     * Chain the status window beneath the viewport.  Mark the left and right
     * edges so that they stay a fixed distance from the left edge of the
     * parent, as well as the top and bottom edges so that they stay a fixed
     * distance from the bottom of the parent.  We do this so that the status
     * will never expand or contract.
     */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), map_viewport); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainBottom); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainBottom); num_args++;
    XtSetValues(status, args, num_args);

    /*
     * Realize the popup so that the status widget knows it's size.
     *
     * If we unset MappedWhenManaged then the DECwindow driver doesn't
     * attach the nethack toplevel to the highest virtual root window.
     * So don't do it.
     */
    /* XtSetMappedWhenManaged(toplevel, False); */
    XtRealizeWidget(toplevel);
    wm_delete_window = XInternAtom(XtDisplay(toplevel),
                                   "WM_DELETE_WINDOW", False);
    XSetWMProtocols(XtDisplay(toplevel), XtWindow(toplevel),
                    &wm_delete_window, 1);

    /*
     * Resize to at most full-screen.
     */
    {
#define TITLEBAR_SPACE 18 /* Leave SOME screen for window decorations */

        int screen_width = WidthOfScreen(XtScreen(wp->w));
        int screen_height = HeightOfScreen(XtScreen(wp->w)) - TITLEBAR_SPACE;
        Dimension form_width, form_height;

        XtSetArg(args[0], XtNwidth, &form_width);
        XtSetArg(args[1], XtNheight, &form_height);
        XtGetValues(toplevel, args, TWO);

        if (form_width > screen_width || form_height > screen_height) {
            XtSetArg(args[0], XtNwidth, min(form_width, screen_width));
            XtSetArg(args[1], XtNheight, min(form_height, screen_height));
            XtSetValues(toplevel, args, TWO);
            XMoveWindow(XtDisplay(toplevel), XtWindow(toplevel), 0,
                        TITLEBAR_SPACE);
        }
#undef TITLEBAR_SPACE
    }

    post_process_tiles(); /* after toplevel is realized */

    /*
     * Now get the default widths of the windows.
     */
    XtSetArg(args[0], nhStr(XtNwidth), &message_vp_width);
    XtGetValues(message_viewport, args, ONE);
    XtSetArg(args[0], nhStr(XtNwidth), &map_vp_width);
    XtSetArg(args[1], nhStr(XtNhorizDistance), &map_vp_hd);
    XtGetValues(map_viewport, args, TWO);
    XtSetArg(args[0], nhStr(XtNwidth), &status_width);
    XtSetArg(args[1], nhStr(XtNhorizDistance), &status_hd);
    XtGetValues(status, args, TWO);

    /*
     * Adjust positions and sizes.  The message viewport widens out to the
     * widest width.  Both the map and status are centered by adjusting
     * their horizDistance.
     */
    if (map_vp_width < status_width || map_vp_width < message_vp_width) {
        if (status_width > message_vp_width) {
            XtSetArg(args[0], nhStr(XtNwidth), status_width);
            XtSetValues(message_viewport, args, ONE);
            max_width = status_width;
        } else {
            max_width = message_vp_width;
        }
        XtSetArg(args[0], nhStr(XtNhorizDistance),
                 map_vp_hd + ((int) (max_width - map_vp_width) / 2));
        XtSetValues(map_viewport, args, ONE);

    } else { /* map is widest */
        XtSetArg(args[0], nhStr(XtNwidth), map_vp_width);
        XtSetValues(message_viewport, args, ONE);
    }
    /*
     * Clear all data values on the fancy status widget so that the values
     * used for spacing don't appear.  This needs to be called some time
     * after the fancy status widget is realized (above, with the game popup),
     * but before it is popped up.
     */
    if (appResources.fancy_status)
        null_out_status();
    /*
     * Set the map size to its standard size.  As with the message window
     * above, the map window needs to be set to its constrained size until
     * its parent (the viewport widget) was realized.
     *
     * Move the message window's slider to the bottom.
     */
    set_map_size(&window_list[map_win], COLNO, ROWNO);
    set_message_slider(&window_list[message_win]);

    /* attempt to catch fatal X11 errors before the program quits */
    (void) XtAppSetErrorHandler(app_context, X11_error_handler);

    highlight_yn(TRUE); /* switch foreground and background */

    /* We can now print to the message window. */
    iflags.window_inited = 1;
}

void
nh_XtPopup(Widget w,        /* widget */
           int grb,         /* type of grab */
           Widget childwid) /* child to receive focus (can be None) */
{
    XtPopup(w, (XtGrabKind) grb);
    XSetWMProtocols(XtDisplay(w), XtWindow(w), &wm_delete_window, 1);
    if (appResources.autofocus)
        XtSetKeyboardFocus(toplevel, childwid);
}

void
nh_XtPopdown(Widget w)
{
    XtPopdown(w);
    if (appResources.autofocus)
        XtSetKeyboardFocus(toplevel, None);
}

void
win_X11_init(int dir)
{
    if (dir != WININIT)
        return;

    return;
}

void
find_scrollbars(
    Widget w,      /* widget of interest; scroll bars are probably attached
                      to its parent or grandparent */
    Widget last_w, /* if non-zero, don't search ancestory beyond this point */
    Widget *horiz, /* output: horizontal scrollbar */
    Widget *vert)  /* output: vertical scrollbar */
{
    *horiz = *vert = (Widget) 0;
    /* for 3.6 this looked for an ancestor with both scrollbars but
       menus might have only vertical */
    if (w) {
        do {
            *horiz = XtNameToWidget(w, "*horizontal");
            *vert = XtNameToWidget(w, "*vertical");
            if (*horiz || *vert)
                break;
            w = XtParent(w);
        } while (w && (!last_w || w != last_w));
    }
}

/* Callback
 * Scroll a viewport, using standard NH 1,2,3,4,6,7,8,9 directions.
 */
/*ARGSUSED*/
void
nh_keyscroll(Widget viewport, XEvent *event, String *params,
             Cardinal *num_params)
{
    Arg arg[2];
    Widget horiz_sb = (Widget) 0, vert_sb = (Widget) 0;
    float top, shown;
    Boolean do_call;
    int direction;
    Cardinal in_nparams = (num_params ? *num_params : 0);

    nhUse(event);

    if (in_nparams != 1)
        return; /* bad translation */

    direction = atoi(params[0]);

    find_scrollbars(viewport, (Widget) 0, &horiz_sb, &vert_sb);

#define H_DELTA 0.25 /* distance of horiz shift */
    /* vert shift is half of curr distance */
    /* The V_DELTA is 1/2 the value of shown. */

    if (horiz_sb) {
        XtSetArg(arg[0], nhStr(XtNshown), &shown);
        XtSetArg(arg[1], nhStr(XtNtopOfThumb), &top);
        XtGetValues(horiz_sb, arg, TWO);

        do_call = True;

        switch (direction) {
        case 1:
        case 4:
        case 7:
            top -= H_DELTA;
            if (top < 0.0)
                top = 0.0;
            break;
        case 3:
        case 6:
        case 9:
            top += H_DELTA;
            if (top + shown > 1.0)
                top = 1.0 - shown;
            break;
        default:
            do_call = False;
        }

        if (do_call) {
            XtCallCallbacks(horiz_sb, XtNjumpProc, &top);
        }
    }

    if (vert_sb) {
        XtSetArg(arg[0], nhStr(XtNshown), &shown);
        XtSetArg(arg[1], nhStr(XtNtopOfThumb), &top);
        XtGetValues(vert_sb, arg, TWO);

        do_call = True;

        switch (direction) {
        case 7:
        case 8:
        case 9:
            top -= shown / 2.0;
            if (top < 0.0)
                top = 0;
            break;
        case 1:
        case 2:
        case 3:
            top += shown / 2.0;
            if (top + shown > 1.0)
                top = 1.0 - shown;
            break;
        default:
            do_call = False;
        }

        if (do_call) {
            XtCallCallbacks(vert_sb, XtNjumpProc, &top);
        }
    }
}

/*winX.c*/

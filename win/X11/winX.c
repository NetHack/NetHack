/*	SCCS Id: @(#)winX.c	3.4	1999/12/21	*/
/* Copyright (c) Dean Luick, 1992				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * "Main" file for the X window-port.  This contains most of the interface
 * routines.  Please see doc/window.doc for an description of the window
 * interface.
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV	/* X11 include files may define SYSV */
#endif

#ifdef MSDOS			/* from compiler */
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
# ifdef SYSV
#  undef SYSV
# endif
# undef PRESERVE_NO_SYSV
#endif

#ifdef SHORT_FILENAMES
#undef SHORT_FILENAMES	/* hack.h will reset via global.h if necessary */
#endif

#include "hack.h"
#include "winX.h"
#include "dlb.h"
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
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
} icon_data[] = {
	{ "nh72", nh72icon_bits, nh72icon_width, nh72icon_height },
	{ "nh56", nh56icon_bits, nh56icon_width, nh56icon_height },
	{ "nh32", nh32icon_bits, nh32icon_width, nh32icon_height },
	{ (const char *)0, (unsigned char *)0, 0, 0 }
};

/*
 * Private global variables (shared among the window port files).
 */
struct xwindow window_list[MAX_WINDOWS];
AppResources appResources;
void FDECL((*input_func), (Widget,XEvent *,String *,Cardinal *));
int click_x, click_y, click_button;	/* Click position on a map window   */
					/* (filled by set_button_values()). */
int updated_inventory;


/* Interface definition, for windows.c */
struct window_procs X11_procs = {
    "X11",
    WC_COLOR|WC_HILITE_PET,
    X11_init_nhwindows,
    X11_player_selection,
    X11_askname,
    X11_get_nh_event,
    X11_exit_nhwindows,
    X11_suspend_nhwindows,
    X11_resume_nhwindows,
    X11_create_nhwindow,
    X11_clear_nhwindow,
    X11_display_nhwindow,
    X11_destroy_nhwindow,
    X11_curs,
    X11_putstr,
    X11_display_file,
    X11_start_menu,
    X11_add_menu,
    X11_end_menu,
    X11_select_menu,
    genl_message_menu,		/* no need for X-specific handling */
    X11_update_inventory,
    X11_mark_synch,
    X11_wait_synch,
#ifdef CLIPPING
    X11_cliparound,
#endif
#ifdef POSITIONBAR
    donull,
#endif
    X11_print_glyph,
    X11_raw_print,
    X11_raw_print_bold,
    X11_nhgetch,
    X11_nh_poskey,
    X11_nhbell,
    X11_doprev_message,
    X11_yn_function,
    X11_getlin,
    X11_get_ext_cmd,
    X11_number_pad,
    X11_delay_output,
#ifdef CHANGE_COLOR	/* only a Mac option currently */
    donull,
    donull,
#endif
    /* other defs that really should go away (they're tty specific) */
    X11_start_screen,
    X11_end_screen,
#ifdef GRAPHIC_TOMBSTONE
    X11_outrip,
#else
    genl_outrip,
#endif
    genl_preference_update,
};

/*
 * Local functions.
 */
static void FDECL(dismiss_file, (Widget, XEvent*, String*, Cardinal*));
static void FDECL(delete_file, (Widget, XEvent*, String*, Cardinal*));
static void FDECL(yn_key, (Widget, XEvent*, String*, Cardinal*));
static void FDECL(yn_delete, (Widget, XEvent*, String*, Cardinal*));
static void FDECL(askname_delete, (Widget, XEvent*, String*, Cardinal*));
static void FDECL(getline_delete, (Widget, XEvent*, String*, Cardinal*));
static void FDECL(X11_hangup, (Widget, XEvent*, String*, Cardinal*));
static int FDECL(input_event, (int));
static void FDECL(win_visible, (Widget,XtPointer,XEvent *,Boolean *));
static void NDECL(init_standard_windows);


/*
 * Local variables.
 */
static boolean x_inited = FALSE;	/* TRUE if window system is set up. */
static winid message_win = WIN_ERR,	/* These are the winids of the	    */
	     map_win     = WIN_ERR,	/*   message, map, and status	    */
	     status_win  = WIN_ERR;	/*   windows, when they are created */
					/*   in init_windows().		    */
static Pixmap icon_pixmap = None;	/* Pixmap for icon.		    */

/*
 * Find the window structure that corresponds to the given widget.  Note
 * that this is not the popup widget, nor the viewport, but the child.
 */
struct xwindow *
find_widget(w)
    Widget w;
{
    int windex;
    struct xwindow *wp;

    /* Search to find the corresponding window.  Look at the main widget, */
    /* popup, the parent of the main widget, then parent of the widget. */
    for (windex = 0, wp = window_list; windex < MAX_WINDOWS; windex++, wp++)
	if (wp->type != NHW_NONE &&
	    (wp->w == w || wp->popup == w || (wp->w && (XtParent(wp->w)) == w)
		|| (wp->popup == XtParent(w))))
	    break;

    if (windex == MAX_WINDOWS) panic("find_widget:  can't match widget");
    return wp;
}

/*
 * Find a free window slot for use.
 */
static winid
find_free_window()
{
    int windex;
    struct xwindow *wp;

    for (windex = 0, wp = &window_list[0]; windex < MAX_WINDOWS; windex++, wp++)
	if (wp->type == NHW_NONE) break;

    if (windex == MAX_WINDOWS)
	panic("find_free_window: no free windows!");
    return (winid) windex;
}

/*
 * Color conversion.  The default X11 color converters don't try very
 * hard to find matching colors in PseudoColor visuals.  If they can't
 * allocate the exact color, they puke and give you something stupid.
 * This is an attempt to find some close readonly cell and use it.
 */
XtConvertArgRec const nhcolorConvertArgs[] = {
    {XtWidgetBaseOffset, (XtPointer)XtOffset(Widget, core.screen),
     sizeof(Screen *)},
    {XtWidgetBaseOffset, (XtPointer)XtOffset(Widget, core.colormap),
     sizeof(Colormap)}
};

#define done(type, value) \
	{							\
	    if (toVal->addr != 0) {				\
		if (toVal->size < sizeof(type)) {		\
		    toVal->size = sizeof(type);			\
		    return False;				\
		}						\
		*(type*)(toVal->addr) = (value);		\
	    }							\
	    else {						\
		static type static_val;				\
		static_val = (value);				\
		toVal->addr = (genericptr_t)&static_val;	\
	    }							\
	    toVal->size = sizeof(type);				\
	    return True;					\
	}

/* decl.h declares these, but it screws up structure references -dlc */
#undef red
#undef green
#undef blue

/*
 * Find a color that approximates the color named in "str".  The "str" color
 * may be a color name ("red") or number ("#7f0000").  If str == NULL, then
 * "color" is assumed to contain the RGB color wanted.
 * The approximate color found is returned in color as well.
 * Return True if something close was found.
 */
Boolean
nhApproxColor(screen, colormap, str, color)
Screen	 *screen;	/* screen to use */
Colormap colormap;	/* the colormap to use */
char     *str;		/* color name */
XColor   *color;	/* the X color structure; changed only if successful */
{
    int		ncells;
    long	cdiff = 16777216; /* 2^24; hopefully our map is smaller */
    XColor	tmp;
    static	XColor *table = 0;
    register int i, j;
    register long tdiff;

    /* if the screen doesn't have a big colormap, don't waste our time */
    /* or if it's huge, and _some_ match should have been possible */
    if((ncells = CellsOfScreen(screen)) < 256 || ncells > 4096)
	return False;

    if (str != (char *)0) {
	if (!XParseColor(DisplayOfScreen(screen), colormap, str, &tmp))
	    return False;
    } else {
	tmp = *color;
	tmp.flags = 7;	/* force to use all 3 of RGB */
    }

    if (!table) {
	table = (XColor *) XtCalloc(ncells, sizeof(XColor));
	for(i=0; i<ncells; i++)
	    table[i].pixel = i;
	XQueryColors(DisplayOfScreen(screen), colormap, table, ncells);
    }

    /* go thru cells and look for the one with smallest diff */
    /* diff is calculated abs(reddiff)+abs(greendiff)+abs(bluediff) */
    /* a more knowledgeable color person might improve this -dlc */
try_again:
    for(i=0; i<ncells; i++) {
	if(table[i].flags == tmp.flags) {
	    j = (int)table[i].red - (int)tmp.red;
	    if(j < 0) j = -j;
	    tdiff = j;
	    j = (int)table[i].green - (int)tmp.green;
	    if(j < 0) j = -j;
	    tdiff += j;
	    j = (int)table[i].blue - (int)tmp.blue;
	    if(j < 0) j = -j;
	    tdiff += j;
	    if(tdiff < cdiff) {
		cdiff = tdiff;
		tmp.pixel = i; /* table[i].pixel == i */
	    }
	}
    }

    if(cdiff == 16777216) return False;	/* nothing found?! */

    /*
     * Found something.  Return it and mark this color as used to avoid
     * reuse.  Reuse causes major contrast problems :-)
     */
    *color = table[tmp.pixel];
    table[tmp.pixel].flags = 0;
    /* try to alloc the color, so no one else can change it */
    if(!XAllocColor(DisplayOfScreen(screen), colormap, color)) {
	cdiff = 16777216;
	goto try_again;
    }
    return True;
}

Boolean
nhCvtStringToPixel(dpy, args, num_args, fromVal, toVal, closure_ret)
Display*	dpy;
XrmValuePtr	args;
Cardinal	*num_args;
XrmValuePtr	fromVal;
XrmValuePtr	toVal;
XtPointer	*closure_ret;
{
    String	    str = (String)fromVal->addr;
    XColor	    screenColor;
    XColor	    exactColor;
    Screen	    *screen;
    XtAppContext    app = XtDisplayToApplicationContext(dpy);
    Colormap	    colormap;
    Status	    status;
    String          params[1];
    Cardinal	    num_params=1;

    if (*num_args != 2) {
     XtAppWarningMsg(app, "wrongParameters", "cvtStringToPixel",
	"XtToolkitError",
	"String to pixel conversion needs screen and colormap arguments",
	(String *)0, (Cardinal *)0);
     return False;
    }

    screen = *((Screen **) args[0].addr);
    colormap = *((Colormap *) args[1].addr);

    /* If Xt colors, use the Xt routine and hope for the best */
#if (XtSpecificationRelease >= 5)
    if ((strcmpi(str, XtDefaultBackground) == 0) ||
	(strcmpi(str, XtDefaultForeground) == 0)) {
	return
	  XtCvtStringToPixel(dpy, args, num_args, fromVal, toVal, closure_ret);
    }
#else
    if (strcmpi(str, XtDefaultBackground) == 0) {
	*closure_ret = (char*)False;
	done(Pixel, WhitePixelOfScreen(screen));
    }
    if (strcmpi(str, XtDefaultForeground) == 0) {
	*closure_ret = (char*)False;
	done(Pixel, BlackPixelOfScreen(screen));
    }
#endif

    status = XAllocNamedColor(DisplayOfScreen(screen), colormap,
			      (char*)str, &screenColor, &exactColor);
    if (status == 0) {
	String msg, type;

	/* some versions of XAllocNamedColor don't allow #xxyyzz names */
	if (str[0] == '#' &&
	    XParseColor(DisplayOfScreen(screen), colormap, str, &exactColor) &&
	    XAllocColor(DisplayOfScreen(screen), colormap, &exactColor)) {
		*closure_ret = (char*)True;
		done(Pixel, exactColor.pixel);
	}

	params[0] = str;
	/* Server returns a specific error code but Xlib discards it.  Ugh */
	if (XLookupColor(DisplayOfScreen(screen), colormap, (char*)str,
			 &exactColor, &screenColor)) {
	    /* try to find another color that will do */
	    if (nhApproxColor(screen, colormap, (char*) str, &screenColor)) {
		*closure_ret = (char*)True;
		done(Pixel, screenColor.pixel);
	    }
	    type = "noColormap";
	    msg = "Cannot allocate colormap entry for \"%s\"";
	}
	else {
	    /* some versions of XLookupColor also don't allow #xxyyzz names */
	    if(str[0] == '#' &&
	       (nhApproxColor(screen, colormap, (char*) str, &screenColor))) {
		*closure_ret = (char*)True;
		done(Pixel, screenColor.pixel);
	    }
	    type = "badValue";
	    msg = "Color name \"%s\" is not defined";
	}

	XtAppWarningMsg(app, type, "cvtStringToPixel",
			"XtToolkitError", msg, params, &num_params);
	*closure_ret = False;
	return False;
    } else {
	*closure_ret = (char*)True;
	done(Pixel, screenColor.pixel);
    }
}

/* ARGSUSED */
static void
nhFreePixel(app, toVal, closure, args, num_args)
XtAppContext	app;
XrmValuePtr	toVal;
XtPointer	closure;
XrmValuePtr	args;
Cardinal	*num_args;
{
    Screen	    *screen;
    Colormap	    colormap;

    if (*num_args != 2) {
     XtAppWarningMsg(app, "wrongParameters",
		     "freePixel", "XtToolkitError",
		     "Freeing a pixel requires screen and colormap arguments",
		     (String *)0, (Cardinal *)0);
     return;
    }

    screen = *((Screen **) args[0].addr);
    colormap = *((Colormap *) args[1].addr);

    if (closure) {
	XFreeColors( DisplayOfScreen(screen), colormap,
		     (unsigned long*)toVal->addr, 1, (unsigned long)0
		    );
    }
}

/* [ALI] Utility function to ask Xaw for font height, since the previous
 * assumption of ascent + descent is not always valid.
 */
Dimension
nhFontHeight(w)
Widget w;
#ifdef _XawTextSink_h
{
    Widget sink;
    XawTextPosition pos = 0;
    int resWidth, resHeight;
    Arg args[1];

    XtSetArg(args[0], XtNtextSink, &sink);
    XtGetValues(w, args, 1);

    XawTextSinkFindPosition(sink, pos, 0, 0, 0, &pos, &resWidth, &resHeight);
    return resHeight;
}
#else
{
    XFontStruct *fs;
    Arg args[1];

    XtSetArg(args[0], XtNfont, &fs);
    XtGetValues(w, args, 1);

    /* Assume font height is ascent + descent. */
    return = fs->ascent + fs->descent;
}
#endif

/* Global Functions ======================================================== */
void
X11_raw_print(str)
    const char *str;
{
    (void) puts(str);
}

void
X11_raw_print_bold(str)
    const char *str;
{
    (void) puts(str);
}

void
X11_curs(window, x, y)
    winid window;
    int x, y;
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
X11_putstr(window, attr, str)
    winid window;
    int attr;
    const char *str;
{
    winid new_win;
    struct xwindow *wp;

    check_winid(window);
    wp = &window_list[window];

    switch (wp->type) {
	case NHW_MESSAGE:
	    (void) strncpy(toplines, str, TBUFSZ);	/* for Norep(). */
	    toplines[TBUFSZ - 1] = 0;
	    append_message(wp, str);
	    break;
	case NHW_STATUS:
	    adjust_status(wp, str);
	    break;
	case NHW_MAP:
	    impossible("putstr: called on map window \"%s\"", str);
	    break;
	case NHW_MENU:
	    if (wp->menu_information->is_menu) {
		impossible(
			"putstr:  called on a menu window, \"%s\" discarded",
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
	    window_list[new_win].type = NHW_NONE;	/* allow re-use */
	    /* fall though to add text */
	case NHW_TEXT:
	    add_to_text_window(wp, attr, str);
	    break;
	default:
	    impossible("putstr: unknown window type [%d] \"%s\"",
							    wp->type, str);
    }
}

/* We do event processing as a callback, so this is a null routine. */
void X11_get_nh_event() { return; }

int
X11_nhgetch()
{
    return input_event(EXIT_ON_KEY_PRESS);
}


int
X11_nh_poskey(x, y, mod)
    int *x, *y, *mod;
{
    int val = input_event(EXIT_ON_KEY_OR_BUTTON_PRESS);

    if (val == 0) {	/* user clicked on a map window */
	*x   = click_x;
	*y   = click_y;
	*mod = click_button;
    }
    return val;
}


winid
X11_create_nhwindow(type)
    int type;
{
    winid window;
    struct xwindow *wp;

    if (!x_inited)
	panic("create_nhwindow:  windows not initialized");

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
    wp->prevx = wp->prevy = wp->cursx = wp->cursy =
				wp->pixel_width = wp->pixel_height = 0;
    wp->keep_window = FALSE;

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
	    panic("create_nhwindow: unknown type [%d]\n", type);
	    break;
    }
    return window;
}

void
X11_clear_nhwindow(window)
    winid window;
{
    struct xwindow *wp;

    check_winid(window);
    wp = &window_list[window];

    switch (wp->type) {
	case NHW_MAP:	clear_map_window(wp); break;
	case NHW_TEXT:	clear_text_window(wp); break;
	case NHW_STATUS:
	case NHW_MENU:
	case NHW_MESSAGE:
	    /* do nothing for these window types */
	    break;
	default:
	    panic("clear_nhwindow: unknown window type [%d]\n", wp->type);
	    break;
    }
}

void
X11_display_nhwindow(window, blocking)
    winid window;
    boolean blocking;
{
    struct xwindow *wp;

    check_winid(window);
    wp = &window_list[window];

    switch (wp->type) {
	case NHW_MAP:
	    if (wp->popup)
		nh_XtPopup(wp->popup, (int)XtGrabNone, wp->w);
	    display_map_window(wp);	/* flush map */
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
		 nh_XtPopup(wp->popup, (int)XtGrabNone, wp->w);
	    display_message_window(wp);	/* flush messages */
	    break;
	case NHW_STATUS:
	    if (wp->popup)
		nh_XtPopup(wp->popup, (int)XtGrabNone, wp->w);
	    break;			/* no flushing necessary */
	case NHW_MENU: {
	    int n;
	    menu_item *selected;

	    /* pop up menu */
	    n = X11_select_menu(window, PICK_NONE, &selected);
	    if (n) {
		impossible("perminvent: %d selected??", n);
		free((genericptr_t)selected);
	    }
	    break;
	}
	case NHW_TEXT:
	    display_text_window(wp, blocking);	/* pop up text window */
	    break;
	default:
	    panic("display_nhwindow: unknown window type [%d]\n", wp->type);
	    break;
    }
}

void
X11_destroy_nhwindow(window)
    winid window;
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

void
X11_update_inventory()
{
    if (x_inited && window_list[WIN_INVEN].menu_information->is_up) {
	updated_inventory = 1;	/* hack to avoid mapping&raising window */
	(void) display_inventory((char *)0, FALSE);
	updated_inventory = 0;
    }
}

/* The current implementation has all of the saved lines on the screen. */
int X11_doprev_message() { return 0; }

void
X11_nhbell()
{
    /* We can't use XBell until toplevel has been initialized. */
    if (x_inited)
	XBell(XtDisplay(toplevel), 0);
    /* else print ^G ?? */
}

void X11_mark_synch()
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

void X11_wait_synch() { if (x_inited) XFlush(XtDisplay(toplevel)); }


/* Both resume_ and suspend_ are called from ioctl.c and unixunix.c. */
void X11_resume_nhwindows() { return; }

/* ARGSUSED */
void X11_suspend_nhwindows(str) const char *str; { return; }

/* Under X, we don't need to initialize the number pad. */
/* ARGSUSED */
void X11_number_pad(state) int state; { return; } /* called from options.c */


void X11_start_screen() { return; } /* called from setftty() in unixtty.c */
void X11_end_screen() { return; }   /* called from settty() in unixtty.c */

#ifdef GRAPHIC_TOMBSTONE
void X11_outrip(window, how)
    winid window;
    int how;
{
    struct xwindow *wp;

    check_winid(window);
    wp = &window_list[window];

    if (wp->type == NHW_TEXT) {
	wp->text_information->is_rip = TRUE;
    } else {
	panic("ripout on non-text window (window type [%d])\n", wp->type);
    }

    calculate_rip_text(how);
}
#endif

/* init and exit nhwindows ------------------------------------------------- */

XtAppContext app_context;		/* context of application */
Widget	     toplevel = (Widget) 0;	/* toplevel widget */
Atom         wm_delete_window;		/* pop down windows */

static XtActionsRec actions[] = {
    {"dismiss_file",	dismiss_file},	/* action for file viewing widget */
    {"delete_file",	delete_file},	/* action for file delete-window */
    {"dismiss_text",	dismiss_text},	/* button action for text widget */
    {"delete_text",	delete_text},	/* delete action for text widget */
    {"key_dismiss_text",key_dismiss_text},/* key action for text widget */
#ifdef GRAPHIC_TOMBSTONE
    {"rip_dismiss_text",rip_dismiss_text},/* action for rip in text widget */
#endif
    {"menu_key",	menu_key},	/* action for menu accelerators */
    {"yn_key",		yn_key},	/* action for yn accelerators */
    {"yn_delete",	yn_delete},	/* action for yn delete-window */
    {"askname_delete",	askname_delete},/* action for askname delete-window */
    {"getline_delete",	getline_delete},/* action for getline delete-window */
    {"menu_delete",	menu_delete},	/* action for menu delete-window */
    {"ec_key",		ec_key},	/* action for extended commands */
    {"ec_delete",	ec_delete},	/* action for ext-com menu delete */
    {"ps_key",		ps_key},	/* action for player selection */
    {"race_key",	race_key},	/* action for race selection */
    {"gend_key",	gend_key},	/* action for gender selection */
    {"algn_key",	algn_key},	/* action for alignment selection */
    {"X11_hangup",	X11_hangup},	/* action for delete of top-level */
    {"input",		map_input},	/* action for key input */
    {"scroll",		nh_keyscroll},	/* action for scrolling by keys */
};

static XtResource resources[] = {
    { "slow", "Slow", XtRBoolean, sizeof(Boolean),
      XtOffset(AppResources *,slow), XtRString, "True" },
    { "autofocus", "AutoFocus", XtRBoolean, sizeof(Boolean),
      XtOffset(AppResources *,autofocus), XtRString, "False" },
    { "message_line", "Message_line", XtRBoolean, sizeof(Boolean),
      XtOffset(AppResources *,message_line), XtRString, "False" },
    { "double_tile_size", "Double_tile_size", XtRBoolean, sizeof(Boolean),
      XtOffset(AppResources *,double_tile_size), XtRString, "False" },
    { "tile_file", "Tile_file", XtRString, sizeof(String),
      XtOffset(AppResources *,tile_file), XtRString, "" },
    { "icon", "Icon", XtRString, sizeof(String),
      XtOffset(AppResources *,icon), XtRString, "nh72" },
    { "message_lines", "Message_lines", XtRInt, sizeof(int),
      XtOffset(AppResources *,message_lines), XtRString, "12" },
    { "pet_mark_bitmap", "Pet_mark_bitmap", XtRString, sizeof(String),
      XtOffset(AppResources *,pet_mark_bitmap), XtRString, "pet_mark.xbm" },
    { "pet_mark_color", "Pet_mark_color", XtRPixel, sizeof(XtRPixel),
      XtOffset(AppResources *,pet_mark_color), XtRString, "Red" },
#ifdef GRAPHIC_TOMBSTONE
    { "tombstone", "Tombstone", XtRString, sizeof(String),
      XtOffset(AppResources *,tombstone), XtRString, "rip.xpm" },
    { "tombtext_x", "Tombtext_x", XtRInt, sizeof(int),
      XtOffset(AppResources *,tombtext_x), XtRString, "155" },
    { "tombtext_y", "Tombtext_y", XtRInt, sizeof(int),
      XtOffset(AppResources *,tombtext_y), XtRString, "78" },
    { "tombtext_dx", "Tombtext_dx", XtRInt, sizeof(int),
      XtOffset(AppResources *,tombtext_dx), XtRString, "0" },
    { "tombtext_dy", "Tombtext_dy", XtRInt, sizeof(int),
      XtOffset(AppResources *,tombtext_dy), XtRString, "13" },
#endif
};

void
X11_init_nhwindows(argcp,argv)
int* argcp;
char** argv;
{
    static const char *banner_text[] = {
	COPYRIGHT_BANNER_A,
	COPYRIGHT_BANNER_B,
	COPYRIGHT_BANNER_C,
	"",
	"",
	0
    };
    register const char **pp;
    int i;
    Cardinal num_args;
    Arg args[4];
    uid_t savuid;

    /* Init windows to nothing. */
    for (i = 0; i < MAX_WINDOWS; i++)
	window_list[i].type = NHW_NONE;

    /*
     * setuid hack: make sure that if nethack is setuid, to use real uid
     * when opening X11 connections, in case the user is using xauth, since
     * the "games" or whatever user probably doesn't have permission to open
     * a window on the user's display.  This code is harmless if the binary
     * is not installed setuid.  See include/system.h on compilation failures.
     */
    savuid = geteuid();
    (void) seteuid(getuid());

    XSetIOErrorHandler((XIOErrorHandler) hangup);

    num_args = 0;
    XtSetArg(args[num_args], XtNallowShellResize, True);	num_args++;
    toplevel = XtAppInitialize(
		    &app_context,
		    "NetHack",			/* application class */
		    (XrmOptionDescList)0, 0,	/* options list */
		    argcp, (String *)argv,	/* command line args */
		    (String *)0,		/* fallback resources */
		    (ArgList)args, num_args);
    XtOverrideTranslations(toplevel,
	XtParseTranslationTable("<Message>WM_PROTOCOLS: X11_hangup()"));

    /* We don't need to realize the top level widget. */

#ifdef TEXTCOLOR
    /* add new color converter to deal with overused colormaps */
    XtSetTypeConverter(XtRString, XtRPixel, nhCvtStringToPixel,
		       (XtConvertArgList)nhcolorConvertArgs,
		       XtNumber(nhcolorConvertArgs),
		       XtCacheByDisplay, nhFreePixel);
#endif /* TEXTCOLOR */

    /* Register the actions mentioned in "actions". */
    XtAppAddActions(app_context, actions, XtNumber(actions));

    /* Get application-wide resources */
    XtGetApplicationResources(toplevel, (XtPointer)&appResources,
			      resources, XtNumber(resources),
			      (ArgList)0, ZERO);

    /* Initialize other things. */
    init_standard_windows();

    /* Give the window manager an icon to use;  toplevel must be realized. */
    if (appResources.icon && *appResources.icon) {
	struct icon_info *ip;

	for (ip = icon_data; ip->name; ip++)
	    if (!strcmp(appResources.icon, ip->name)) {
		icon_pixmap = XCreateBitmapFromData(XtDisplay(toplevel),
				XtWindow(toplevel),
				(genericptr_t)ip->bits, ip->width, ip->height);
		if (icon_pixmap != None) {
		    XWMHints hints;

		    (void) memset((genericptr_t)&hints, 0, sizeof(XWMHints));
		    hints.flags = IconPixmapHint;
		    hints.icon_pixmap = icon_pixmap;
		    XSetWMHints(XtDisplay(toplevel),
				XtWindow(toplevel), &hints);
		}
		break;
	    }
    }

    /* end of setuid hack: reset uid back to the "games" uid */
    (void) seteuid(savuid);

    x_inited = TRUE;	/* X is now initialized */

    /* Display the startup banner in the message window. */
    for (pp = banner_text; *pp; pp++)
	X11_putstr(WIN_MESSAGE, 0, *pp);
}

/*
 * All done.
 */
/* ARGSUSED */
void X11_exit_nhwindows(dummy)
    const char *dummy;
{
    extern Pixmap tile_pixmap;	/* from winmap.c */

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
}


/* delay_output ------------------------------------------------------------ */

/*
 * Timeout callback for delay_output().  Send a fake message to the map
 * window.
 */
/* ARGSUSED */
static void
d_timeout(client_data, id)
    XtPointer client_data;
    XtIntervalId *id;
{
    XEvent event;
    XClientMessageEvent *mesg;

    /* Set up a fake message to the event handler. */
    mesg = (XClientMessageEvent *) &event;
    mesg->type = ClientMessage;
    mesg->message_type = XA_STRING;
    mesg->format = 8;
    XSendEvent(XtDisplay(window_list[WIN_MAP].w),
		XtWindow(window_list[WIN_MAP].w),
		False,
		NoEventMask,
		(XEvent*) mesg);
}

/*
 * Delay for 50ms.  This is not implemented asynch.  Maybe later.
 * Start the timeout, then wait in the event loop.  The timeout
 * function will send an event to the map window which will be waiting
 * for a sent event.
 */
void
X11_delay_output()
{
    if (!x_inited) return;

    (void) XtAppAddTimeOut(app_context, 30L, d_timeout, (XtPointer) 0);

    /* The timeout function will enable the event loop exit. */
    (void) x_event(EXIT_ON_SENT_EVENT);
}

/* X11_hangup -------------------------------------------------------------- */
/* ARGSUSED */
static void
X11_hangup(w, event, params, num_params)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    hangup(1);		/* 1 is commonly SIGHUP, but ignored anyway */
}

/* askname ----------------------------------------------------------------- */
/* ARGSUSED */
static void
askname_delete(w, event, params, num_params)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    nh_XtPopdown(w);
    (void) strcpy(plname, "Mumbles");	/* give them a name... ;-) */
    exit_x_event = TRUE;
}

/* Callback for askname dialog widget. */
/* ARGSUSED */
static void
askname_done(w, client_data, call_data)
    Widget w;
    XtPointer client_data;
    XtPointer call_data;
{
    int len;
    char *s;
    Widget dialog = (Widget) client_data;

    s = (char *) GetDialogResponse(dialog);

    len = strlen(s);
    if (len == 0) {
	X11_nhbell();
	return;
    }

    /* Truncate name if necessary */
    if (len >= sizeof(plname)-1)
	len = sizeof(plname)-1;

    (void) strncpy(plname, s, len);
    plname[len] = '\0';
    XtFree(s);

    nh_XtPopdown(XtParent(dialog));
    exit_x_event = TRUE;
}

void
X11_askname()
{
    Widget popup, dialog;
    Arg args[1];

    XtSetArg(args[0], XtNallowShellResize, True);

    popup = XtCreatePopupShell("askname", transientShellWidgetClass,
				   toplevel, args, ONE);
    XtOverrideTranslations(popup,
	XtParseTranslationTable("<Message>WM_PROTOCOLS: askname_delete()"));

    dialog = CreateDialog(popup, "dialog",
				    askname_done, (XtCallbackProc) 0);

    SetDialogPrompt(dialog, "What is your name?");	/* set prompt */
    SetDialogResponse(dialog, "");		/* set default answer */

    XtRealizeWidget(popup);
    positionpopup(popup, TRUE);		/* center,bottom */

    nh_XtPopup(popup, (int)XtGrabExclusive, dialog);

    /* The callback will enable the event loop exit. */
    (void) x_event(EXIT_ON_EXIT);
}


/* getline ----------------------------------------------------------------- */
/* This uses Tim Theisen's dialog widget set (from GhostView). */

static Widget getline_popup, getline_dialog;

#define CANCEL_STR "\033"
static char *getline_input;


/* Callback for getline dialog widget. */
/* ARGSUSED */
static void
done_button(w, client_data, call_data)
    Widget w;
    XtPointer client_data;
    XtPointer call_data;
{
    int len;
    char *s;
    Widget dialog = (Widget) client_data;

    s = (char *) GetDialogResponse(dialog);
    len = strlen(s);

    /* Truncate input if necessary */
    if (len >= BUFSZ) len = BUFSZ - 1;

    (void) strncpy(getline_input, s, len);
    getline_input[len] = '\0';
    XtFree(s);

    nh_XtPopdown(XtParent(dialog));
    exit_x_event = TRUE;
}

/* ARGSUSED */
static void
getline_delete(w, event, params, num_params)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    Strcpy(getline_input, CANCEL_STR);
    nh_XtPopdown(w);
    exit_x_event = TRUE;
}

/* Callback for getline dialog widget. */
/* ARGSUSED */
static void
abort_button(w, client_data, call_data)
    Widget w;
    XtPointer client_data;
    XtPointer call_data;
{
    Widget dialog = (Widget) client_data;

    Strcpy(getline_input, CANCEL_STR);
    nh_XtPopdown(XtParent(dialog));
    exit_x_event = TRUE;
}


void
X11_getlin(question, input)
    const char *question;
    char *input;
{
    static boolean need_to_init = True;

    getline_input = input;

    flush_screen(1);
    if (need_to_init) {
	Arg args[1];

	need_to_init = False;

	XtSetArg(args[0], XtNallowShellResize, True);

	getline_popup = XtCreatePopupShell("getline",transientShellWidgetClass,
				   toplevel, args, ONE);
	XtOverrideTranslations(getline_popup,
	    XtParseTranslationTable("<Message>WM_PROTOCOLS: getline_delete()"));

	getline_dialog = CreateDialog(getline_popup, "dialog",
				    done_button, abort_button);

	XtRealizeWidget(getline_popup);
	XSetWMProtocols(XtDisplay(getline_popup), XtWindow(getline_popup),
			&wm_delete_window, 1);
    }
    SetDialogPrompt(getline_dialog, (String)question);	/* set prompt */
    SetDialogResponse(getline_dialog, "");	/* set default answer */
    positionpopup(getline_popup, TRUE);		/* center,bottom */

    nh_XtPopup(getline_popup, (int)XtGrabExclusive, getline_dialog);

    /* The callback will enable the event loop exit. */
    (void) x_event(EXIT_ON_EXIT);
}


/* Display file ------------------------------------------------------------ */
static const char display_translations[] =
    "#override\n\
     <Key>q: dismiss_file()\n\
     <Key>Escape: dismiss_file()\n\
     <BtnDown>: dismiss_file()";


/* WM_DELETE_WINDOW callback for file dismissal. */
/*ARGSUSED*/
static void
delete_file(w, event, params, num_params)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    nh_XtPopdown(w);
    XtDestroyWidget(w);
}

/* Callback for file dismissal. */
/*ARGSUSED*/
static void
dismiss_file(w, event, params, num_params)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    Widget popup = XtParent(w);
    nh_XtPopdown(popup);
    XtDestroyWidget(popup);
}

void
X11_display_file(str, complain)
    const char *str;
    boolean complain;
{
    dlb *fp;
    Arg args[12];
    Cardinal num_args;
    Widget popup, dispfile;
    Position top_margin, bottom_margin, left_margin, right_margin;
    XFontStruct *fs;
    int new_width, new_height;
#define LLEN 128
    char line[LLEN];
    int num_lines;
    char *textlines;
    int charcount;

    /* Use the port-independent file opener to see if the file exists. */
    fp = dlb_fopen(str, RDTMODE);

    if (!fp) {
	if(complain) pline("Cannot open %s.  Sorry.", str);

	return;	/* it doesn't exist, ignore */
    }

    /*
     * Count the number of lines and characters in the file.
     */
    num_lines = 0;
    charcount = 1;
    while (dlb_fgets(line, LLEN, fp)) {
	num_lines++;
	charcount += strlen(line);
    }

    (void) dlb_fclose(fp);

    /* Ignore empty files */
    if (num_lines == 0) return;

    /* If over the max window size, truncate the window size to the max */
    if (num_lines >= DISPLAY_FILE_SIZE)
	num_lines = DISPLAY_FILE_SIZE;

    /*
     * Re-open the file and read the data into a buffer.  Cannot use
     * the XawAsciiFile type of widget, because that is not DLB-aware.
     */
    textlines = (char *) alloc((unsigned int) charcount);
    textlines[0] = '\0';

    fp = dlb_fopen(str, RDTMODE);

    while (dlb_fgets(line, LLEN, fp)) {
	(void) strcat(textlines, line);
    }

    (void) dlb_fclose(fp);

    num_args = 0;
    XtSetArg(args[num_args], XtNtitle, str);	num_args++;

    popup = XtCreatePopupShell("display_file", topLevelShellWidgetClass,
					       toplevel, args, num_args);
    XtOverrideTranslations(popup,
	XtParseTranslationTable("<Message>WM_PROTOCOLS: delete_file()"));

    num_args = 0;
    XtSetArg(args[num_args], XtNscrollHorizontal,
				XawtextScrollWhenNeeded);	num_args++;
    XtSetArg(args[num_args], XtNscrollVertical,
				XawtextScrollWhenNeeded);	num_args++;
    XtSetArg(args[num_args], XtNtype, XawAsciiString);		num_args++;
    XtSetArg(args[num_args], XtNstring, textlines);		num_args++;
    XtSetArg(args[num_args], XtNdisplayCaret, False);		num_args++;
    XtSetArg(args[num_args], XtNtranslations,
	XtParseTranslationTable(display_translations));		num_args++;

    dispfile = XtCreateManagedWidget(
			"text",			/* name */
			asciiTextWidgetClass,
			popup,			/* parent widget */
			args,			/* set some values */
			num_args);		/* number of values to set */

    /* Get font and border information. */
    num_args = 0;
    XtSetArg(args[num_args], XtNfont,	      &fs);	       num_args++;
    XtSetArg(args[num_args], XtNtopMargin,    &top_margin);    num_args++;
    XtSetArg(args[num_args], XtNbottomMargin, &bottom_margin); num_args++;
    XtSetArg(args[num_args], XtNleftMargin,   &left_margin);   num_args++;
    XtSetArg(args[num_args], XtNrightMargin,  &right_margin);  num_args++;
    XtGetValues(dispfile, args, num_args);

    /*
     * The data files are currently set up assuming an 80 char wide window
     * and a fixed width font.  Soo..
     */
    new_height = num_lines * nhFontHeight(dispfile) +
						top_margin + bottom_margin;
    new_width  = 80 * fs->max_bounds.width + left_margin + right_margin;

    /* Set the new width and height. */
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth,  new_width);  num_args++;
    XtSetArg(args[num_args], XtNheight, new_height); num_args++;
    XtSetValues(dispfile, args, num_args);

    nh_XtPopup(popup, (int)XtGrabNone, (Widget)0);
    free(textlines);
}


/* yn_function ------------------------------------------------------------- */
/* (not threaded) */

static const char *yn_quitchars = " \n\r";
static const char *yn_choices;	/* string of acceptable input */
static char yn_def;
static char yn_return;		/* return value */
static char yn_esc_map;		/* ESC maps to this char. */
static Widget yn_popup;		/* popup for the yn fuction (created once) */
static Widget yn_label;		/* label for yn function (created once) */
static boolean yn_getting_num;	/* TRUE if accepting digits */
static int yn_ndigits;		/* digit count */
static long yn_val;		/* accumulated value */

static const char yn_translations[] =
    "#override\n\
     <Key>: yn_key()";

/*
 * Convert the given key event into a character.  If the key maps to
 * more than one character only the first is returned.  If there is
 * no conversion (i.e. just the CTRL key hit) a NUL is returned.
 */
char
key_event_to_char(key)
    XKeyEvent *key;
{
    char keystring[MAX_KEY_STRING];
    int nbytes;
    boolean meta = !!(key->state & Mod1Mask);

    nbytes = XLookupString(key, keystring, MAX_KEY_STRING,
			   (KeySym *)0, (XComposeStatus *)0);

    /* Modifier keys return a zero lengh string when pressed. */
    if (nbytes == 0) return '\0';

    return (char) (((int) keystring[0]) + (meta ? 0x80 : 0));
}

/*
 * Called when we get a WM_DELETE_WINDOW event on a yn window.
 */
/* ARGSUSED */
static void
yn_delete(w, event, params, num_params)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    yn_getting_num = FALSE;
    /* Only use yn_esc_map if we have choices.  Otherwise, return ESC. */
    yn_return = yn_choices ? yn_esc_map : '\033';
    exit_x_event = TRUE;	/* exit our event handler */
}

/*
 * Called when we get a key press event on a yn window.
 */
/* ARGSUSED */
static void
yn_key(w, event, params, num_params)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    char ch;

    if(appResources.slow && !input_func)
	map_input(w, event, params, num_params);

    ch = key_event_to_char((XKeyEvent *) event);

    if (ch == '\0') {	/* don't accept nul char or modifier event */
	/* no bell */
	return;
    }

    if (!yn_choices) {			/* accept any input */
	yn_return = ch;
    } else {
	ch = lowc(ch);			/* move to lower case */

	if (ch == '\033') {
	    yn_getting_num = FALSE;
	    yn_return = yn_esc_map;
	} else if (index(yn_quitchars, ch)) {
	    yn_return = yn_def;
	} else if (index(yn_choices, ch)) {
	    if (ch == '#') {
		if (yn_getting_num) {	/* don't select again */
		    X11_nhbell();
		    return;
		}
		yn_getting_num = TRUE;
		yn_ndigits = 0;
		yn_val = 0;
		return;			/* wait for more input */
	    }
	    yn_return = ch;
	    if (ch != 'y') yn_getting_num = FALSE;
	} else {
	    if (yn_getting_num) {
		if (digit(ch)) {
		    yn_ndigits++;
		    yn_val = (yn_val * 10) + (long) (ch - '0');
		    return;			/* wait for more input */
		}
		if (yn_ndigits && (ch == '\b' || ch == 127/*DEL*/)) {
		    yn_ndigits--;
		    yn_val = yn_val/ 10;
		    return;			/* wait for more input */
		}
	    }
	    X11_nhbell();		/* no match */
	    return;
	}

	if (yn_getting_num) {
	    yn_return = '#';
	    if (yn_val < 0) yn_val = 0;
	    yn_number = yn_val;	/* assign global */
	}
    }
    exit_x_event = TRUE;	/* exit our event handler */
}


char
X11_yn_function(ques, choices, def)
    const char *ques;
    const char *choices;
    char def;
{
    static Boolean need_to_init = True;
    char buf[QBUFSZ];
    Arg args[4];
    Cardinal num_args;

    yn_choices = choices;	/* set up globals for callback to use */
    yn_def     = def;

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

	Strcpy(choicebuf, choices);	/* anything beyond <esc> is hidden */
	if ((cb = index(choicebuf, '\033')) != 0) *cb = '\0';
	/* ques [choices] (def) */
	if ((int)(1 + strlen(ques) + 2 + strlen(choicebuf) + 4) >= QBUFSZ)
	    panic("yn_function:  question too long");
	Sprintf(buf, "%s [%s] ", ques, choicebuf);
	if (def) Sprintf(eos(buf), "(%c) ", def);

	/* escape maps to 'q' or 'n' or default, in that order */
	yn_esc_map = (index(choices, 'q') ? 'q' :
		     (index(choices, 'n') ? 'n' :
					    def));
    } else {
	if ((int)(1 + strlen(ques)) >= QBUFSZ)
	    panic("yn_function:  question too long");
	Strcpy(buf, ques);
    }

    if (!appResources.slow && need_to_init) {
	need_to_init = False;

	XtSetArg(args[0], XtNallowShellResize, True);
	yn_popup = XtCreatePopupShell("query", transientShellWidgetClass,
					toplevel, args, ONE);
	XtOverrideTranslations(yn_popup,
	    XtParseTranslationTable("<Message>WM_PROTOCOLS: yn_delete()"));

	num_args = 0;
	XtSetArg(args[num_args], XtNtranslations,
		XtParseTranslationTable(yn_translations));	num_args++;
	yn_label = XtCreateManagedWidget("yn_label",
				labelWidgetClass,
				yn_popup,
				args, num_args);

	XtRealizeWidget(yn_popup);
	XSetWMProtocols(XtDisplay(yn_popup), XtWindow(yn_popup),
			&wm_delete_window, 1);
    }

    if(appResources.slow)
	input_func = yn_key;

    num_args = 0;
    XtSetArg(args[num_args], XtNlabel, buf);	num_args++;
    XtSetValues(yn_label, args, num_args);

    if(!appResources.slow) {
	/*
	 * Due to some kind of weird bug in the X11R4 and X11R5 shell, we
	 * need to set the label twice to get the size to change.
	 */
	num_args = 0;
	XtSetArg(args[num_args], XtNlabel, buf); num_args++;
	XtSetValues(yn_label, args, num_args);

	positionpopup(yn_popup, TRUE);
	nh_XtPopup(yn_popup, (int)XtGrabExclusive, yn_label);
    }

    yn_getting_num = FALSE;
    (void) x_event(EXIT_ON_EXIT);

    if(appResources.slow) {
	input_func = 0;
	num_args = 0;
	XtSetArg(args[num_args], XtNlabel, " ");	num_args++;
	XtSetValues(yn_label, args, num_args);
    } else {
	nh_XtPopdown(yn_popup);	/* this removes the event grab */
    }

    return yn_return;
}

/* End global functions ==================================================== */

/*
 * Before we wait for input via nhgetch() and nh_poskey(), we need to
 * do some pre-processing.
 */
static int
input_event(exit_condition)
    int exit_condition;
{
    if (WIN_STATUS != WIN_ERR)	/* hilighting on the fancy status window */
	check_turn_events();
    if (WIN_MAP != WIN_ERR)	/* make sure cursor is not clipped */
	check_cursor_visibility(&window_list[WIN_MAP]);
    if (WIN_MESSAGE != WIN_ERR)	/* reset pause line */
	set_last_pause(&window_list[WIN_MESSAGE]);

    return x_event(exit_condition);
}


/*ARGSUSED*/
void
msgkey(w, data, event)
    Widget w;
    XtPointer data;
    XEvent *event;
{
    Cardinal num = 0;
    map_input(window_list[WIN_MAP].w, event, (String*) 0, &num);
}

/*ARGSUSED*/
static void
win_visible(w, data, event, flag)	/* only called for autofocus */
    Widget w;
    XtPointer data;	/* client_data not used */
    XEvent *event;
    Boolean *flag;	/* continue_to_dispatch flag not used */
{
    XVisibilityEvent *vis_event = (XVisibilityEvent *)event;

    if (vis_event->state != VisibilityFullyObscured) {
	/* one-time operation; cancel ourself */
	XtRemoveEventHandler(toplevel, VisibilityChangeMask, False,
			     win_visible, (XtPointer) 0);
	/* grab initial input focus */
	XSetInputFocus(XtDisplay(w), XtWindow(w), RevertToNone, CurrentTime);
    }
}

/*
 * Set up the playing console.  This has three major parts:  the
 * message window, the map, and the status window.
 */
static void
init_standard_windows()
{
    Widget form, message_viewport, map_viewport, status;
    Arg args[8];
    Cardinal num_args;
    Dimension message_vp_width, map_vp_width, status_width, max_width;
    int map_vp_hd, status_hd;
    struct xwindow *wp;


    num_args = 0;
    XtSetArg(args[num_args], XtNallowShellResize, True);	num_args++;
    form = XtCreateManagedWidget("nethack",
				panedWidgetClass,
				toplevel, args, num_args);

    XtAddEventHandler(form, KeyPressMask, False,
		      (XtEventHandler) msgkey, (XtPointer) 0);

    if (appResources.autofocus)
	XtAddEventHandler(toplevel, VisibilityChangeMask, False,
			  win_visible, (XtPointer) 0);

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
    XtSetArg(args[num_args], XtNresizable, True);		num_args++;
    XtSetArg(args[num_args], XtNleft,	   XtChainLeft);	num_args++;
    XtSetArg(args[num_args], XtNtop,	   XtChainTop);		num_args++;
    XtSetValues(message_viewport, args, num_args);

    if(appResources.slow) {
	num_args = 0;
	XtSetArg(args[num_args], XtNtranslations,
		 XtParseTranslationTable(yn_translations)); num_args++;
	yn_label = XtCreateManagedWidget("yn_label",
					 labelWidgetClass,
					 form,
					 args, num_args);
	num_args = 0;
	XtSetArg(args[num_args], XtNfromVert, message_viewport); num_args++;
	XtSetArg(args[num_args], XtNjustify, XtJustifyLeft);	num_args++;
	XtSetArg(args[num_args], XtNresizable, True);	num_args++;
	XtSetArg(args[num_args], XtNlabel, " ");	num_args++;
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
    if(appResources.slow) {
	XtSetArg(args[num_args], XtNfromVert, yn_label);	num_args++;
    } else {
	XtSetArg(args[num_args], XtNfromVert, message_viewport);num_args++;
    }
    XtSetArg(args[num_args], XtNbottom, XtChainBottom);		num_args++;
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
    XtSetArg(args[num_args], XtNfromVert, map_viewport);	num_args++;
    XtSetArg(args[num_args], XtNleft,	  XtChainLeft);		num_args++;
    XtSetArg(args[num_args], XtNright,	  XtChainLeft);		num_args++;
    XtSetArg(args[num_args], XtNtop,	  XtChainBottom);	num_args++;
    XtSetArg(args[num_args], XtNbottom,	  XtChainBottom);	num_args++;
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

	int screen_width  = WidthOfScreen(XtScreen(wp->w));
	int screen_height = HeightOfScreen(XtScreen(wp->w)) - TITLEBAR_SPACE;
	Dimension form_width, form_height;

	XtSetArg(args[0], XtNwidth, &form_width);
	XtSetArg(args[1], XtNheight, &form_height);
	XtGetValues(toplevel, args, TWO);
	
	if (form_width > screen_width || form_height > screen_height) {
	    XtSetArg(args[0], XtNwidth, min(form_width,screen_width));
	    XtSetArg(args[1], XtNheight, min(form_height,screen_height));
	    XtSetValues(toplevel, args, TWO);
	    XMoveWindow(XtDisplay(toplevel),XtWindow(toplevel),
		0, TITLEBAR_SPACE);
	}
#undef TITLEBAR_SPACE
    }

    post_process_tiles();	/* after toplevel is realized */

    /*
     * Now get the default widths of the windows.
     */
    XtSetArg(args[0], XtNwidth, &message_vp_width);
    XtGetValues(message_viewport, args, ONE);
    XtSetArg(args[0], XtNwidth, &map_vp_width);
    XtSetArg(args[1], XtNhorizDistance, &map_vp_hd);
    XtGetValues(map_viewport, args, TWO);
    XtSetArg(args[0], XtNwidth, &status_width);
    XtSetArg(args[1], XtNhorizDistance, &status_hd);
    XtGetValues(status, args, TWO);

    /*
     * Adjust positions and sizes.  The message viewport widens out to the
     * widest width.  Both the map and status are centered by adjusting
     * their horizDistance.
     */
    if (map_vp_width < status_width || map_vp_width < message_vp_width) {
	if (status_width > message_vp_width) {
	    XtSetArg(args[0], XtNwidth, status_width);
	    XtSetValues(message_viewport, args, ONE);
	    max_width = status_width;
	} else {
/***** The status display looks better when left justified.
	    XtSetArg(args[0], XtNhorizDistance,
				status_hd+((message_vp_width-status_width)/2));
	    XtSetValues(status, args, ONE);
*****/
	    max_width = message_vp_width;
	}
	XtSetArg(args[0], XtNhorizDistance, map_vp_hd+((int)(max_width-map_vp_width)/2));
	XtSetValues(map_viewport, args, ONE);

    } else {	/* map is widest */
	XtSetArg(args[0], XtNwidth, map_vp_width);
	XtSetValues(message_viewport, args, ONE);

/***** The status display looks better when left justified.
	XtSetArg(args[0], XtNhorizDistance,
				status_hd+((map_vp_width-status_width)/2));

	XtSetValues(status, args, ONE);
*****/
    }
    /*
     * Clear all data values on the fancy status widget so that the values
     * used for spacing don't appear.  This needs to be called some time
     * after the fancy status widget is realized (above, with the game popup),
     * but before it is popped up.
     */
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
    (void) XtAppSetErrorHandler(app_context, (XtErrorHandler) hangup);

    /* We can now print to the message window. */
    iflags.window_inited = 1;
}


void
nh_XtPopup(w, g, childwid)
    Widget w;		/* widget */
    int    g;		/* type of grab */
    Widget childwid;	/* child to recieve focus (can be None) */
{
    XtPopup(w, (XtGrabKind)g);
    XSetWMProtocols(XtDisplay(w), XtWindow(w), &wm_delete_window, 1);
    if (appResources.autofocus) XtSetKeyboardFocus(toplevel, childwid);
}

void
nh_XtPopdown(w)
    Widget w;
{
    XtPopdown(w);
    if (appResources.autofocus) XtSetKeyboardFocus(toplevel, None);
}

void
win_X11_init()
{
#ifdef OPENWINBUG
    /* With the OpenWindows 3.0 libraries and the SunOS 4.1.2 ld, these
     * two routines will not be found when linking.  An apparently correct
     * executable is produced, along with nasty messages and a failure code
     * returned to make.  The routines are in the static libXmu.a and
     * libXmu.sa.4.0, but not in libXmu.so.4.0.  Rather than fiddle with
     * static linking, we do this.
     */
    if (rn2(2) > 2) {
	/* i.e., FALSE that an optimizer probably can't find */
	get_wmShellWidgetClass();
	get_applicationShellWidgetClass();
    }
#endif
    return;
}

/* Callback
 * Scroll a viewport, using standard NH 1,2,3,4,6,7,8,9 directions.
 */
/*ARGSUSED*/
void
nh_keyscroll(viewport, event, params, num_params)
    Widget   viewport;
    XEvent   *event;
    String   *params;
    Cardinal *num_params;
{
    Arg arg[2];
    Widget horiz_sb, vert_sb;
    float top, shown;
    Boolean do_call;
    int direction;
    Cardinal in_nparams = (num_params ? *num_params : 0);

    if (in_nparams != 1) return; /* bad translation */

    direction=atoi(params[0]);

    horiz_sb = XtNameToWidget(viewport, "*horizontal");
    vert_sb  = XtNameToWidget(viewport, "*vertical");

    if (!horiz_sb && !vert_sb) {
	/* Perhaps the widget enclosing this has scrollbars (could use while) */
	Widget parent=XtParent(viewport);
	if (parent) {
	    horiz_sb = XtNameToWidget(parent, "horizontal");
	    vert_sb  = XtNameToWidget(parent, "vertical");
	}
    }

#define H_DELTA 0.25		/* distance of horiz shift */
				/* vert shift is half of curr distance */
/* The V_DELTA is 1/2 the value of shown. */

    if (horiz_sb) {
	XtSetArg(arg[0], XtNshown,	&shown);
	XtSetArg(arg[1], XtNtopOfThumb, &top);
	XtGetValues(horiz_sb, arg, TWO);

	do_call = True;

	switch (direction) {
	  case 1: case 4: case 7:
	    top -= H_DELTA;
	    if (top < 0.0) top = 0.0;
	break; case 3: case 6: case 9:
	    top += H_DELTA;
	    if (top + shown > 1.0) top = 1.0 - shown;
	break; default:
	    do_call = False;
	}

	if (do_call) {
	    XtCallCallbacks(horiz_sb, XtNjumpProc, &top);
	}
    }

    if (vert_sb) {
	XtSetArg(arg[0], XtNshown,      &shown);
	XtSetArg(arg[1], XtNtopOfThumb, &top);
	XtGetValues(vert_sb, arg, TWO);

	do_call = True;

	switch (direction) {
	  case 7: case 8: case 9:
	    top -= shown / 2.0;
	    if (top < 0.0) top = 0;
	break; case 1: case 2: case 3:
	    top += shown / 2.0;
	    if (top + shown > 1.0) top = 1.0 - shown;
	break; default:
	    do_call = False;
	}

	if (do_call) {
	    XtCallCallbacks(vert_sb, XtNjumpProc, &top);
	}
    }
}

/*winX.c*/

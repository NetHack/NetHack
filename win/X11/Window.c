/*	SCCS Id: @(#)Window.c	3.4	1993/02/02	*/
/* Copyright (c) Dean Luick, 1992				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Data structures and support routines for the Window widget.  This is a
 * drawing canvas with 16 colors and one font.
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV	/* X11 include files may define SYSV */
#endif

#ifdef MSDOS			/* from compiler */
#define SHORT_FILENAMES
#endif

#ifdef SHORT_FILENAMES
#include <X11/IntrinsP.h>
#else
#include <X11/IntrinsicP.h>
#endif
#include <X11/StringDefs.h>

#ifdef PRESERVE_NO_SYSV
# ifdef SYSV
#  undef SYSV
# endif
# undef PRESERVE_NO_SYSV
#endif

#include "xwindowp.h"

#include "config.h"

static XtResource resources[] = {
#define offset(field) XtOffset(WindowWidget, window.field)
    /* {name, class, type, size, offset, default_type, default_addr}, */
    { XtNrows, XtCRows, XtRDimension, sizeof(Dimension),
	  offset(rows), XtRImmediate, (XtPointer) 21},
    { XtNcolumns, XtCColumns, XtRDimension, sizeof(Dimension),
	  offset(columns), XtRImmediate, (XtPointer) 80},
    { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	  offset(foreground), XtRString, XtDefaultForeground },

    { XtNblack, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(black), XtRString, "black"},
    { XtNred, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(red), XtRString, "red" },
    { XtNgreen, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(green), XtRString, "pale green" },
    { XtNbrown, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(brown), XtRString, "brown" },
    { XtNblue, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(blue), XtRString, "blue" },
    { XtNmagenta, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(magenta), XtRString, "magenta" },
    { XtNcyan, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(cyan), XtRString, "light cyan" },
    { XtNgray, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(gray), XtRString, "gray" },
    { XtNorange, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(orange), XtRString, "orange" },
    { XtNbright_green, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(bright_green), XtRString, "green" },
    { XtNyellow, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(yellow), XtRString, "yellow" },
    { XtNbright_blue, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(bright_blue), XtRString, "royal blue" },
    { XtNbright_magenta, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(bright_magenta), XtRString, "violet" },
    { XtNbright_cyan, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(bright_cyan), XtRString, "cyan" },
    { XtNwhite, XtCColor, XtRPixel, sizeof(Pixel),
	  offset(white), XtRString, "white" },

    { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
	  offset(font), XtRString, XtDefaultFont },
    { XtNexposeCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	  offset(expose_callback), XtRCallback, (char *)0 },
    { XtNcallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	  offset(input_callback), XtRCallback, (char *)0 },
    { XtNresizeCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	  offset(resize_callback), XtRCallback, (char *)0 },
#undef offset
};

/* ARGSUSED */
static void no_op(w, event, params, num_params)
    Widget   w;			/* unused */
    XEvent   *event;		/* unused */
    String   *params;		/* unused */
    Cardinal *num_params;	/* unused */
{
}

static XtActionsRec actions[] =
{
    {"no-op",	no_op},
};

static char translations[] =
"<BtnDown>:     input() \
";

/* ARGSUSED */
static void Redisplay(w, event, region)
    Widget w;
    XEvent *event;
    Region region;	/* unused */
{
    /* This isn't correct - we need to call the callback with region. */
    XtCallCallbacks(w, XtNexposeCallback, (caddr_t) event);
}

/* ARGSUSED */
static void Resize(w)
    Widget w;
{
    XtCallCallbacks(w, XtNresizeCallback, (caddr_t) 0);
}


WindowClassRec windowClassRec = {
  { /* core fields */
    /* superclass		*/	(WidgetClass) &widgetClassRec,
    /* class_name		*/	"Window",
    /* widget_size		*/	sizeof(WindowRec),
    /* class_initialize		*/	0,
    /* class_part_initialize	*/	0,
    /* class_inited		*/	FALSE,
    /* initialize		*/	0,
    /* initialize_hook		*/	0,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	actions,
    /* num_actions		*/	XtNumber(actions),
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	0,
    /* resize			*/	Resize,
    /* expose			*/	Redisplay,
    /* set_values		*/	0,
    /* set_values_hook		*/	0,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	0,
    /* accept_focus		*/	0,
    /* version			*/	XtVersion,
    /* callback_private		*/	0,
    /* tm_table			*/	translations,
    /* query_geometry		*/	XtInheritQueryGeometry,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	0
  },
  { /* window fields */
    /* empty			*/	0
  }
};

WidgetClass windowWidgetClass = (WidgetClass)&windowClassRec;

Font
WindowFont(w) Widget w; { return ((WindowWidget)w)->window.font->fid; }

XFontStruct *
WindowFontStruct(w) Widget w; { return ((WindowWidget)w)->window.font; }

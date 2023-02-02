/* NetHack 3.7	Window.c	$NHDT-Date: 1596498371 2020/08/03 23:46:11 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Dean Luick, 1992                                 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Data structures and support routines for the Window widget.  This is a
 * drawing canvas with 16 colors and one font.
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV /* X11 include files may define SYSV */
#endif

#ifdef MSDOS /* from compiler */
#define SHORT_FILENAMES
#endif

#ifdef SHORT_FILENAMES
#include <X11/IntrinsP.h>
#else
#include <X11/IntrinsicP.h>
#endif
#include <X11/StringDefs.h>

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#include "xwindowp.h"

#include "config.h" /* #define for const for non __STDC__ compilers */
#include "lint.h"   /* for nethack's nhStr() macro */
#include "winX.h"   /* to make sure protoypes match corresponding functions */

static XtResource resources[] = {
#define offset(field) XtOffset(WindowWidget, window.field)
    /* {name, class, type, size, offset, default_type, default_addr}, */
    { nhStr(XtNrows), nhStr(XtCRows), XtRDimension, sizeof(Dimension),
      offset(rows), XtRImmediate, (XtPointer) 21 },
    { nhStr(XtNcolumns), nhStr(XtCColumns), XtRDimension, sizeof(Dimension),
      offset(columns), XtRImmediate, (XtPointer) 80 },
    { nhStr(XtNforeground), XtCForeground, XtRPixel, sizeof(Pixel),
      offset(foreground), XtRString, (XtPointer) XtDefaultForeground },

    { nhStr(XtNblack), XtCColor, XtRPixel, sizeof(Pixel), offset(black),
      XtRString, (XtPointer) "black" },
    { nhStr(XtNred), XtCColor, XtRPixel, sizeof(Pixel), offset(red),
      XtRString, (XtPointer) "red" },
    { nhStr(XtNgreen), XtCColor, XtRPixel, sizeof(Pixel), offset(green),
      XtRString, (XtPointer) "pale green" },
    { nhStr(XtNbrown), XtCColor, XtRPixel, sizeof(Pixel), offset(brown),
      XtRString, (XtPointer) "brown" },
    { nhStr(XtNblue), XtCColor, XtRPixel, sizeof(Pixel), offset(blue),
      XtRString, (XtPointer) "blue" },
    { nhStr(XtNmagenta), XtCColor, XtRPixel, sizeof(Pixel), offset(magenta),
      XtRString, (XtPointer) "magenta" },
    { nhStr(XtNcyan), XtCColor, XtRPixel, sizeof(Pixel), offset(cyan),
      XtRString, (XtPointer) "light cyan" },
    { nhStr(XtNgray), XtCColor, XtRPixel, sizeof(Pixel), offset(gray),
      XtRString, (XtPointer) "gray" },
    { nhStr(XtNorange), XtCColor, XtRPixel, sizeof(Pixel), offset(orange),
      XtRString, (XtPointer) "orange" },
    { nhStr(XtNbright_green), XtCColor, XtRPixel, sizeof(Pixel),
      offset(bright_green), XtRString, (XtPointer) "green" },
    { nhStr(XtNyellow), XtCColor, XtRPixel, sizeof(Pixel), offset(yellow),
      XtRString, (XtPointer) "yellow" },
    { nhStr(XtNbright_blue), XtCColor, XtRPixel, sizeof(Pixel),
      offset(bright_blue), XtRString, (XtPointer) "royal blue" },
    { nhStr(XtNbright_magenta), XtCColor, XtRPixel, sizeof(Pixel),
      offset(bright_magenta), XtRString, (XtPointer) "violet" },
    { nhStr(XtNbright_cyan), XtCColor, XtRPixel, sizeof(Pixel),
      offset(bright_cyan), XtRString, (XtPointer) "cyan" },
    { nhStr(XtNwhite), XtCColor, XtRPixel, sizeof(Pixel), offset(white),
      XtRString, (XtPointer) "white" },

    { nhStr(XtNfont), XtCFont, XtRFontStruct, sizeof(XFontStruct *),
      offset(font), XtRString, (XtPointer) XtDefaultFont },
    { nhStr(XtNexposeCallback), XtCCallback, XtRCallback,
      sizeof(XtCallbackList), offset(expose_callback), XtRCallback,
      (char *) 0 },
    { nhStr(XtNcallback), XtCCallback, XtRCallback, sizeof(XtCallbackList),
      offset(input_callback), XtRCallback, (char *) 0 },
    { nhStr(XtNresizeCallback), XtCCallback, XtRCallback,
      sizeof(XtCallbackList), offset(resize_callback), XtRCallback,
      (char *) 0 },
#undef offset
};

/* ARGSUSED */
static void
no_op(Widget w,             /* unused */
      XEvent *event,        /* unused */
      String *params,       /* unused */
      Cardinal *num_params) /* unused */
{
    nhUse(w);
    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    return;
}

static XtActionsRec actions[] = {
    { nhStr("no-op"), no_op },
};

static char translations[] = "<BtnDown>:     input() \
";

/* ARGSUSED */
static void
Redisplay(Widget w, XEvent *event, Region region /* unused */)
{
    nhUse(region);

    /* This isn't correct - we need to call the callback with region. */
    XtCallCallbacks(w, XtNexposeCallback, (XtPointer)event);
}

/* ARGSUSED */
static void
Resize(Widget w)
{
    XtCallCallbacks(w, XtNresizeCallback, (XtPointer) 0);
}

WindowClassRec windowClassRec = {
    { /* core fields */
      /* superclass             */ (WidgetClass) &widgetClassRec,
      /* class_name             */ nhStr("Window"),
      /* widget_size            */ sizeof(WindowRec),
      /* class_initialize       */ 0,
      /* class_part_initialize  */ 0,
      /* class_inited           */ FALSE,
      /* initialize             */ 0,
      /* initialize_hook        */ 0,
      /* realize                */ XtInheritRealize,
      /* actions                */ actions,
      /* num_actions            */ XtNumber(actions),
      /* resources              */ resources,
      /* num_resources          */ XtNumber(resources),
      /* xrm_class              */ NULLQUARK,
      /* compress_motion        */ TRUE,
      /* compress_exposure      */ TRUE,
      /* compress_enterleave    */ TRUE,
      /* visible_interest       */ FALSE,
      /* destroy                */ 0,
      /* resize                 */ Resize,
      /* expose                 */ Redisplay,
      /* set_values             */ 0,
      /* set_values_hook        */ 0,
      /* set_values_almost      */ XtInheritSetValuesAlmost,
      /* get_values_hook        */ 0,
      /* accept_focus           */ 0,
      /* version                */ XtVersion,
      /* callback_private       */ 0,
      /* tm_table               */ translations,
      /* query_geometry         */ XtInheritQueryGeometry,
      /* display_accelerator    */ XtInheritDisplayAccelerator,
      /* extension              */ 0 },
    { /* window fields */
      /* empty                  */ 0 }
};

WidgetClass windowWidgetClass = (WidgetClass) &windowClassRec;

Font
WindowFont(Widget w)
{
    return ((WindowWidget) w)->window.font->fid;
}

XFontStruct *
WindowFontStruct(Widget w)
{
    return ((WindowWidget) w)->window.font;
}

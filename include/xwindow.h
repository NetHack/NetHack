/* NetHack 3.7	xwindow.h	$NHDT-Date: 1596498574 2020/08/03 23:49:34 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) Dean Luick, 1992                                 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _xwindow_h
#define _xwindow_h

/****************************************************************
 *
 * Window widget
 *
 ****************************************************************/

/* Resources:

 Name                Class              RepType         Default Value
 ----                -----              -------         -------------
 background          Background         Pixel           XtDefaultBackground
 border              BorderColor        Pixel           XtDefaultForeground
 borderWidth         BorderWidth        Dimension       1
 destroyCallback     Callback           Pointer         NULL
 height              Height             Dimension       0
 mappedWhenManaged   MappedWhenManaged  Boolean         True
 sensitive           Sensitive          Boolean         True
 width               Width              Dimension       0
 x                   Position           Position        0
 y                   Position           Position        0

 rows                Width              Dimension       21
 columns             Height             Dimension       80
 foreground          Color              Pixel           XtDefaultForeground

 black               Color              Pixel           "black"
 red                 Color              Pixel           "red"
 green               Color              Pixel           "pale green"
 brown               Color              Pixel           "brown"
 blue                Color              Pixel           "blue"
 magenta             Color              Pixel           "magenta"
 cyan                Color              Pixel           "light cyan"
 gray                Color              Pixel           "gray"
 //no color//
 orange              Color              Pixel           "orange"
 bright_green        Color              Pixel           "green"
 yellow              Color              Pixel           "yellow"
 bright_blue         Color              Pixel           "royal blue"
 bright_magenta      Color              Pixel           "violet"
 bright_cyan         Color              Pixel           "cyan"
 white               Color              Pixel           "white"

 font                Font               XFontStruct*    XtDefaultFont
 exposeCallback      Callback           Callback        NULL
 callback            Callback           Callback        NULL
 resizeCallback      Callback           Callback        NULL
*/

/* define any special resource names here that are not in <X11/StringDefs.h>
 */

#define XtNrows "rows"
#define XtNcolumns "columns"
#define XtNblack "black"
#define XtNred "red"
#define XtNgreen "green"
#define XtNbrown "brown"
#define XtNblue "blue"
#define XtNmagenta "magenta"
#define XtNcyan "cyan"
#define XtNgray "gray"
#define XtNorange "orange"
#define XtNbright_green "bright_green"
#define XtNyellow "yellow"
#define XtNbright_blue "bright_blue"
#define XtNbright_magenta "bright_magenta"
#define XtNbright_cyan "bright_cyan"
#define XtNwhite "white"
#define XtNexposeCallback "exposeCallback"
#define XtNresizeCallback "resizeCallback"

extern XFontStruct *WindowFontStruct(Widget);
extern Font WindowFont(Widget);

#define XtCWindowResource "WindowResource"
#define XtCRows "Rows"
#define XtCColumns "Columns"

/* declare specific WindowWidget class and instance datatypes */

typedef struct _WindowClassRec *WindowWidgetClass;
typedef struct _WindowRec *WindowWidget;

/* declare the class constant */

extern WidgetClass windowWidgetClass;

#endif /* _xwindow_h */

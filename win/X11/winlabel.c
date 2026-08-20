/* NetHack 5.0	winlabel.c	$NHDT-Date: 1781973110 2026/06/20 16:31:50 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.50 $ */
/* Copyright (c) Ray Chason, 2026                                 */
/* NetHack may be freely redistributed.  See license for details. */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Label.h>

#include <inttypes.h>

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#include "hack.h"
#include "winX.h"

/* Declarations depending on Xft support or none */
static int X11_text_width(Display *, X11_Font *, const char *, size_t,
                          const int *, unsigned);

/* Data attached to a wrapped widget */
typedef struct WidgetData {
    /* Displayed text */
    Pixmap pixmap;

    /* Attributes */
    unsigned attrs;
    boolean highlight;

    /* Fonts for italic, bold and bold-italic */
    X11_Font *font[4]; /* To use the fonts */
#ifdef USE_XFT
    int win_type;
#else /* !USE_XFT */
    X11_Font *font_ptr[4]; /* To free the fonts */
#endif /* ?USE_XFT */

    /* Percentage bar */
    unsigned percent;
    Pixel bar_color;

    /* Columns */
    int *columns;
    unsigned num_cols;
} WidgetData;

/* Bits for WidgetData::font */
enum { font_bold = 1, font_italic = 2 };

static boolean check_label(Widget);
static void delete_callback(Widget, XtPointer, XtPointer);
static void update_label(Widget, WidgetData *);
static void allocate_font(Widget, WidgetData *, unsigned);
static void free_fonts(Widget, WidgetData *);

static WidgetData *add_widget(Widget w);
static void delete_widget(Widget w);
static WidgetData *get_widget_data(Widget w);

/*
 * Create a wrapper for labelWidgetClass and its descendant classes.
 *
 * A table is maintained to associate each widget with a data block.
 * The block contains additional fonts for italic, bold and bold-italic,
 * attribute flags and a percentage bar.
 */
void
X11_wrap_widget(Widget w, int win_type)
{
    /* We shouldn't use this for anything other than labels and subclasses
       of labels */
    if (!check_label(w)) {
        impossible("Widget is not a Label or of a class derived from Label");
        return;
    }

    /* Only wrap once */
    if (get_widget_data(w)) {
        impossible("Tried to wrap a widget twice");
        return;
    }

    /* Attach a destroy callback to reclaim resources attached to the widget */
    XtAddCallback(w, XtNdestroyCallback, delete_callback, NULL);

    /* Create the structure with its initial settings */
    WidgetData *data = add_widget(w);

    /* Copy resources from the created widget */
#ifdef USE_XFT
    data->win_type = win_type;
    allocate_font(w, data, 0);
#else /* !USE_XFT */
    Cardinal num_args2 = 0;
    Arg args2[1];
    XtSetArg(args2[num_args2], XtNfont,  &data->font[0]); num_args2++;
    XtGetValues(w, args2, num_args2);
    nhUse(win_type);
#endif /* ?USE_XFT */

    /* Create the pixmap for the first time */
    update_label(w, data);
}

/* Callback when the widget is deleted */
static void
delete_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    nhUse(client_data);
    nhUse(call_data);

    /*
     * We shouldn't get here with any widget for which check_label returns
     * false, because such widgets will not have this callback set
     */
    WidgetData *data = get_widget_data(w);
    if (data != NULL) {
        Display *display = XtDisplay(w);
        if (data->pixmap != 0) {
            XFreePixmap(display, data->pixmap);
        }
        free_fonts(w, data);
        free(data->columns);
        free(data);
    }

    delete_widget(w);
}

/* Return true if the widget is a Label or of a class derived from Label */
static boolean
check_label(Widget w)
{
    return XtIsSubclass(w, labelWidgetClass);
}

/* Update the label's pixmap */
void
X11_update_label(Widget w)
{
    WidgetData *data = get_widget_data(w);
    if (data != NULL) {
        update_label(w, data);
    }
}

void
X11_set_attrs(Widget w, unsigned attrs)
{
    WidgetData *data = get_widget_data(w);
    if (data != NULL) {
        data->attrs = attrs;
        update_label(w, data);
    }
}

void
X11_set_highlight(Widget w, boolean highlight)
{
    WidgetData *data = get_widget_data(w);
    if (data != NULL) {
        data->highlight = highlight;
        update_label(w, data);
    }
}

void
X11_set_percent(Widget w, unsigned percent, Pixel color)
{
    WidgetData *data = get_widget_data(w);
    if (data != NULL) {
        data->percent = percent;
        data->bar_color = color;
        update_label(w, data);
    }
}

void
X11_set_column_widths(Widget w, const int *col_widths, unsigned num_cols)
{
    WidgetData *data = get_widget_data(w);
    if (data != NULL) {
        free(data->columns);
        data->columns = (int *) alloc(sizeof(data->columns[0]) * (num_cols + 1));
        data->columns[0] = 0;
        if (num_cols != 0) {
            for (unsigned i = 0; i < num_cols; ++i) {
                data->columns[i+1] = data->columns[i] + col_widths[i];
            }
        }
        data->num_cols = num_cols;
        update_label(w, data);
    }
}

/* Update the label's pixmap */
/* This is called from functions that have altered the data block, and already
   have a pointer to it */
static void
update_label(Widget w, WidgetData *data)
{
    Display *display = XtDisplay(w);
    Screen *screen = DefaultScreenOfDisplay(display);
    int depth = DefaultDepthOfScreen(screen);

    Pixmap new_pixmap = 0;
    Cardinal num_args;
    Arg args[10];

    /* Select the font */
    unsigned font_idx = 0;
    if (data->attrs & HL_BOLD) {
        font_idx |= font_bold;
        allocate_font(w, data, font_idx);
    }
    if (data->attrs & HL_ITALIC) {
        font_idx |= font_italic;
        allocate_font(w, data, font_idx);
    }
    X11_Font *font = data->font[font_idx];

    String label;
    Boolean sens;       /* Make dim if not sensitive */
    XtJustify justify;  /* Left, center, right */
    Pixel fgpixel;      /* Colors for the first pass */
    Pixel bgpixel;
    Boolean resize;     /* Resizable? */
    num_args = 0;
    XtSetArg(args[num_args], XtNlabel, &label); num_args++;
    XtSetArg(args[num_args], XtNsensitive, &sens); num_args++;
    XtSetArg(args[num_args], XtNjustify, &justify); num_args++;
    XtSetArg(args[num_args], XtNforeground, &fgpixel); num_args++;
    XtSetArg(args[num_args], XtNbackground, &bgpixel); num_args++;
    XtSetArg(args[num_args], XtNresize, &resize); num_args++;
    XtGetValues(w, args, num_args);
    unsigned attrs = data->attrs;
    if (!sens) {
        attrs |= HL_DIM;
    }

    /* Dimensions of pixmap */
    Dimension width;
    Dimension height;
    if (resize) {
        /* Render the widest width and the total height of the lines */
        size_t i = 0;
        width = 0;
        height = 0;
        while (label[i] != '\0') {
            size_t line1 = strcspn(label + i, "\n");
            size_t line2 = line1;
            /* Exclude \r from the rendering */
            if (line2 != 0 && label[i + line2 - 1] == '\r') {
                --line2;
            }

            /* Get the width of the line */
            int lwidth = X11_text_width(display, font, label + i, line2,
                                        data->columns, data->num_cols);

            /* Update pixmap dimensions */
            width = max(lwidth, width);
            ++height;

            /* Advance to next line */
            i += line1;
            if (label[i] == '\n') {
                ++i;
            }
        }

        /* Always size for at least one line */
        height = max(height, 1);

        /* Convert height to pixels */
        height *= X11_font_height(font);
    } else {
        num_args = 0;
        XtSetArg(args[num_args], XtNwidth, &width); num_args++;
        XtSetArg(args[num_args], XtNheight, &height); num_args++;
        XtGetValues(w, args, num_args);
    }
    if (width < 2 || height < 2) {
        /* Pixmap must not have zero size, or a crash will ensue;
           minimum size 2 simplifies border logic */
        width = max(width, 2);
        height = max(height, 2);
        fgpixel = bgpixel;
    }

    /* Use the full width of the widget if a percentage bar is set or inverse
       is in effect */
    if (data->percent != 0 || (attrs & HL_INVERSE) != 0 || data->highlight) {
        Dimension wwidth, iwidth;
        num_args = 0;
        XtSetArg(args[num_args], XtNwidth, &wwidth); num_args++;
        XtSetArg(args[num_args], XtNinternalWidth, &iwidth); num_args++;
        XtGetValues(w, args, num_args);
        width = max(width, wwidth - iwidth*2);
    }

    /* Create the pixmap */
    new_pixmap = XCreatePixmap(display, RootWindowOfScreen(screen),
                               width, height, depth);

    /* If a percent bar is specified, make two passes over the text and render
       the percent bar in the second pass */
    for (unsigned pass = 0; pass < 2; ++pass) {
        XGCValues values;

        if (!!(attrs & HL_INVERSE) ^ !!data->highlight) {
            values.foreground = bgpixel;
            values.background = fgpixel;
        } else {
            values.foreground = fgpixel;
            values.background = bgpixel;
        }
        if (attrs & HL_DIM) {
            values.foreground = (values.foreground & 0xFEFEFE) >> 1;
            values.background = (values.background & 0xFEFEFE) >> 1;
        }
        if ((attrs & HL_BLINK) && X11_blink) {
            values.foreground = values.background;
        }

#ifdef USE_XFT
        Visual *visual = DefaultVisualOfScreen(screen);
        Colormap cmap = DefaultColormapOfScreen(screen);
        XftDraw *draw = XftDrawCreate(display, new_pixmap, visual, cmap);
        XftColor fgcolor, bgcolor;
        X11_new_color(w, values.foreground, &fgcolor);
        X11_new_color(w, values.background, &bgcolor);
#else /* !USE_XFT */
        values.font = font->fid;
        values.function = GXcopy;
        GC ggc = XtGetGC(w,
                         GCFunction | GCForeground | GCBackground | GCFont,
                         &values);
#endif /* ?USE_XFT */
        if (pass == 1) {
            /* Percent bar will occupy this area */
            XRectangle clip = {
                .x = 0,
                .y = 0,
                .width = width * data->percent / 100,
                .height = height
            };
#ifdef USE_XFT
            XftDrawSetClipRectangles(draw, 0, 0, &clip, 1);
#else /* !USE_XFT */
            XSetClipRectangles(display, ggc, 0, 0, &clip, 1, Unsorted);
#endif /* ?USE_XFT */
        }

        /* Draw a border for inverse and highlight together */
        /* Otherwise, fill with the background color */
#ifdef USE_XFT
        if (!((attrs & HL_INVERSE) && data->highlight)) {
            XftDrawRect(draw, &bgcolor, 0, 0, width, height);
        } else {
            XftDrawRect(draw, &fgcolor, 0, 0, width, height);
        }
#else /* !USE_XFT */
        if (!((attrs & HL_INVERSE) && data->highlight)) {
            XSetForeground(display, ggc, values.background);
        }
        XFillRectangle(display, new_pixmap, ggc, 0, 0, width, height);
        XSetForeground(display, ggc, values.foreground);
#endif /* ?USE_XFT */

        int y = font->ascent;
        size_t i = 0;
        while (label[i] != '\0') {
            size_t line1 = strcspn(label + i, "\n");
            size_t line2 = line1;
            /* Exclude \r from the rendering */
            if (line2 != 0 && label[i + line2 - 1] == '\r') {
                --line2;
            }

            /* Get the width of the line */
            int lwidth = X11_text_width(display, font, label + i, line2,
                                        data->columns, data->num_cols);

            /* Place the line horizontally */
            int x = 0;
            switch (justify) {
                case XtJustifyLeft:
                    x = 0;
                    break;

                case XtJustifyCenter:
                    x = (width - lwidth) / 2;
                    break;

                case XtJustifyRight:
                    x = width - lwidth;
                    break;
            }

            /* Render the line */
#ifdef USE_XFT
            XftDrawRect(draw, &bgcolor, x, y - font->ascent, lwidth, X11_font_height(font));
#else
            XSetForeground(display, ggc, values.background);
            XFillRectangle(display, new_pixmap, ggc, x, y - font->ascent, lwidth, X11_font_height(font));
            XSetForeground(display, ggc, values.foreground);
#endif
            size_t j = 0;
            unsigned col = 0;
            while (j < line2) {
                size_t line3;
                int pos;
                if (data->num_cols == 0) {
                    /* Columns not set */
                    line3 = line2;
                    pos = 0;
                } else {
                    line3 = min(strcspn(label + i + j, "\t\n"), line2);
                    pos = data->columns[min(col, data->num_cols)];
                }
#ifdef USE_XFT
                XftDrawString8(draw, &fgcolor, font, x + pos, y,
                               (const FcChar8 *) (label + i + j), line3);
#else
                XDrawString(display, new_pixmap, ggc,
                            x + pos, y,
                            label + i + j, line3);
#endif
                j += line3;
                if (j < line2) {
                    ++j;
                }
                ++col;
            }

            /* Draw the underline if requested */
            if (attrs & HL_ULINE) {
#ifdef USE_XFT
                XftDrawRect(draw, &fgcolor, x, y, lwidth, 1);
#else
                XDrawLine(display, new_pixmap, ggc,
                          x, y,
                          x + lwidth - 1, y);
#endif
            }

            y += X11_font_height(font);

            /* Advance to next line */
            i += line1;
            if (label[i] == '\n') {
                ++i;
            }
        }

        /* Ensure a one-pixel border if both inverse and highlight */
        if ((attrs & HL_INVERSE) && data->highlight) {
#ifdef USE_XFT
            XftDrawRect(draw, &fgcolor, 0, 0,        width, 1);
            XftDrawRect(draw, &fgcolor, 0, height-1, width, 1);
            XftDrawRect(draw, &fgcolor, 0, 0,        1,     height);
            XftDrawRect(draw, &fgcolor, width-1, 0,  1,     height);
#else
            XDrawRectangle(display, new_pixmap, ggc, 0, 0, width-1, height-1);
#endif
        }

#ifdef USE_XFT
        XftColorFree(display, visual, cmap, &fgcolor);
        XftColorFree(display, visual, cmap, &bgcolor);
        XftDrawDestroy(draw);
#else
        XtReleaseGC(w, ggc);
#endif

        /* Set up to display the percent bar on the second pass */
        if (data->percent == 0) {
            break;
        }
        fgpixel = bgpixel;
        bgpixel = data->bar_color;
    }

    /* Update the pixmap */
    num_args = 0;
    XtSetArg(args[num_args], XtNbitmap, new_pixmap); num_args++;
    XtSetValues(w, args, num_args);
    if (data->pixmap != 0) {
        XFreePixmap(display, data->pixmap);
    }
    data->pixmap = new_pixmap;
}

/* Allocate a bold or italic font */
static void
allocate_font(Widget w, WidgetData *data, unsigned font_idx)
{
#ifdef USE_XFT
    static const unsigned attrs[4] = {
        0, HL_BOLD, HL_ITALIC, HL_BOLD | HL_ITALIC
    };
    data->font[font_idx] = X11_new_font(w, attrs[font_idx], data->win_type);
#else
    Display *display = XtDisplay(w);
    X11_Font *font1 = (font_idx == (font_bold | font_italic))
                       ? data->font[font_bold]
                       : data->font[0];
    data->font_ptr[font_idx] = (font_idx == font_bold)
                             ? X11_bold_font(display, font1)
                             : X11_italic_font(display, font1);
    data->font[font_idx] = data->font_ptr[font_idx]
                         ? data->font_ptr[font_idx]
                         : font1;
#endif
}

/* Free any allocated fonts */
static void
free_fonts(Widget w, WidgetData *data)
{
    Display *display = XtDisplay(w);
    for (unsigned i = 0; i < 4; ++i) {
#ifdef USE_XFT
        if (data->font[i] != NULL) {
            XftFontClose(display, data->font[i]);
        }
        data->font[i] = NULL;
#else
        if (data->font_ptr[i] != NULL) {
            XFreeFont(display, data->font_ptr[i]);
        }
        data->font[i] = NULL;
        data->font_ptr[i] = NULL;
#endif
    }
}

static int
X11_text_width(Display *display, X11_Font *font, const char *text, size_t length,
               const int *columns, unsigned num_cols)
{
    if (num_cols == 0) {
        /* Columns have not been set */
        return X11_column_width(display, font, text, length);
    }

    /* Width is the position of the last column, plus the width of the string
       in the last column */

    unsigned col = 0;
    size_t i = 0;
    while (col < num_cols) {
        size_t len = strcspn(text + i, "\t\n");
        if (i+len >= length || text[i+len] != '\t') {
            break;
        }
        ++col;
        i += len + 1;
    }

    int pos = columns[col] + X11_column_width(display, font, text + i, length - i);

    return pos;
}

//////////////////////////////////////////////////////////////////////////////
//             Functions that depend on the font rendering API              //
//////////////////////////////////////////////////////////////////////////////

#ifdef USE_XFT
int
X11_column_width(Display *display, X11_Font *font, const char *text, size_t length)
{
    XGlyphInfo extents;
    XftTextExtents8(display, font, (const FcChar8*) text, length, &extents);
    return extents.width - extents.x;
}

int
X11_font_height(X11_Font *font)
{
    return max(font->height, font->ascent + font->descent);
}
#else /* !USE_XFT */
int
X11_column_width(Display *display, X11_Font *font, const char *text, size_t length)
{
    nhUse(display);
    return XTextWidth(font, text, length);
}

int
X11_font_height(X11_Font *font)
{
    return font->ascent + font->descent;
}
#endif /* !USE_XFT */

//////////////////////////////////////////////////////////////////////////////
//                 A table of widgets and their data blocks                 //
//////////////////////////////////////////////////////////////////////////////

typedef struct WidgetBucket {
    Widget w;
    WidgetData *data;
} WidgetBucket;

#define MAX_WIDGETS 1024
static WidgetBucket widget_table[MAX_WIDGETS];
static unsigned num_widgets;

/* bsearch compare function to search widget-table */
static int
widget_compare(const void *key_, const void *value_)
{
    const Widget *key = key_;
    const WidgetBucket *value = value_;

    if ((uintptr_t)key < (uintptr_t)value->w) {
        return -1;
    }
    if ((uintptr_t)key > (uintptr_t)value->w) {
        return +1;
    }
    return 0;
}

/* Given the widget, return the entry in widget_table, or NULL if not found */
static WidgetBucket *
find_widget_bucket(Widget w)
{
    WidgetBucket *bucket =
            bsearch(w, widget_table, num_widgets, sizeof(WidgetBucket),
                    widget_compare);
    return bucket;
}

/* Given the widget, return the WidgetData structure, or NULL if not found */
static WidgetData *
get_widget_data(Widget w)
{
    WidgetBucket *bucket = find_widget_bucket(w);
    return (bucket != NULL) ? bucket->data : NULL;
}

/* Add a widget to the table, set its normal and bold fonts and provide for
   its removal from the table */
static WidgetData *
add_widget(Widget w)
{
    /* Panic rather than overflow the array */
    if (num_widgets >= MAX_WIDGETS) {
        panic("Widget table is full\n");
    }

    /* Insert the widget into the table, maintaining its order */
    unsigned i;
    for (i = num_widgets;
         i != 0 && (uintptr_t)widget_table[i-1].w > (uintptr_t)w;
         --i) {
        widget_table[i] = widget_table[i-1];
    }
    ++num_widgets;
    widget_table[i].w = w;

    /* Create the data block */
    WidgetData *data = (WidgetData *)alloc(sizeof(*data));
    memset(data, 0, sizeof(*data));
    widget_table[i].data = data;

    return data;
}

/* Remove the widget from the table */
static void
delete_widget(Widget w)
{
    /* Find its location in the table */
    WidgetBucket *bucket = find_widget_bucket(w);
    if (bucket == NULL) {
        return;
    }

    /* Remove the widget from the table */
    for (unsigned i = (unsigned)(bucket - widget_table);
         i + 1 < MAX_WIDGETS;
         ++i) {
        widget_table[i] = widget_table[i+1];
    }
    --num_widgets;
}

//////////////////////////////////////////////////////////////////////////////

/* Call every time the blink flag changes */
void
X11_blink_labels(void)
{
    for (unsigned i = 0; i < num_widgets; ++i) {
        WidgetBucket *bucket = &widget_table[i];
        if (bucket->data->attrs & HL_BLINK) {
            update_label(bucket->w, bucket->data);
        }
    }
}

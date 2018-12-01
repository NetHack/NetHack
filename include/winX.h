/* NetHack 3.6	winX.h	$NHDT-Date: 1457079196 2016/03/04 08:13:16 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.23 $ */
/* Copyright (c) Dean Luick, 1992                                 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Definitions for the X11 window-port.  See doc/window.doc for details on
 * the window interface.
 */
#ifndef WINX_H
#define WINX_H

#ifndef E
#define E extern
#endif

#if defined(BOS) || defined(NHSTDC)
#define DIMENSION_P int
#else
#ifdef WIDENED_PROTOTYPES
#define DIMENSION_P unsigned int
#else
#define DIMENSION_P Dimension
#endif
#endif

/*
 * Generic text buffer.
 */
#define START_SIZE 512 /* starting text buffer size */
struct text_buffer {
    char *text;
    int text_size;
    int text_last;
    int num_lines;
};

/*
 * Information specific to a map window.
 */
struct text_map_info_t {
    unsigned char text[ROWNO][COLNO]; /* Actual displayed screen. */
#ifdef TEXTCOLOR
    unsigned char colors[ROWNO][COLNO]; /* Color of each character. */
    GC color_gcs[CLR_MAX],              /* GC for each color */
        inv_color_gcs[CLR_MAX];         /* GC for each inverse color */
#define copy_gc color_gcs[NO_COLOR]
#define inv_copy_gc inv_color_gcs[NO_COLOR]
#else
    GC copy_gc,      /* Drawing GC */
        inv_copy_gc; /* Inverse drawing GC */
#endif

    int square_width,  /* Saved font information so      */
        square_height, /*   we can calculate the correct */
        square_ascent, /*   placement of changes.        */
        square_lbearing;
};

struct tile_glyph_info_t {
    unsigned short glyph;
    unsigned special;
};

struct tile_map_info_t {
    struct tile_glyph_info_t glyphs[ROWNO][COLNO]; /* Saved glyph numbers. */
    GC white_gc;
    GC black_gc;
    unsigned long image_width; /* dimensions of tile image */
    unsigned long image_height;

    int square_width,  /* Saved tile information so      */
        square_height, /*   we can calculate the correct */
        square_ascent, /*   placement of changes.        */
        square_lbearing;
};

struct map_info_t {
    Dimension viewport_width,     /* Saved viewport size, so we can */
        viewport_height;          /*   clip to cursor on a resize.  */
    unsigned char t_start[ROWNO], /* Starting column for new info. */
        t_stop[ROWNO];            /* Ending column for new info. */

    boolean is_tile; /* true if currently using tiles */
    struct text_map_info_t text_map;
    struct tile_map_info_t tile_map;
};

/*
 * Information specific to a message window.
 */
struct line_element {
    struct line_element *next;
    char *line;     /* char buffer */
    int buf_length; /* length of buffer */
    int str_length; /* length of string in buffer */
};

struct mesg_info_t {
    XFontStruct *fs;                 /* Font for the window. */
    int num_lines;                   /* line count */
    struct line_element *head;       /* head of circular line queue */
    struct line_element *line_here;  /* current drawn line position */
    struct line_element *last_pause; /* point to the line after the prev */
                                     /*     bottom of screen             */
    struct line_element *last_pause_head; /* pointer to head of previous */
                                          /* turn                        */
    GC gc;           /* GC for text drawing */
    int char_width,  /* Saved font information so we can  */
        char_height, /*   calculate the correct placement */
        char_ascent, /*   of changes.                     */
        char_lbearing;
    Dimension viewport_width,  /* Saved viewport size, so we can adjust */
              viewport_height; /*   the slider on a resize.             */
    Boolean dirty;            /* Lines have been added to the window. */
};

/*
 * Information specific to a "text" status window.
 */
struct status_info_t {
    struct text_buffer text; /* Just a text buffer. */
};

/*
 * Information specific to a menu window.  First a structure for each
 * menu entry, then the structure for each menu window.
 */
typedef struct x11_mi {
    struct x11_mi *next;
    anything identifier; /* Opaque type to identify this selection */
    long pick_count;     /* specific selection count; -1 if none */
    char *str;           /* The text of the item. */
    int attr;            /* Attribute for the line. */
    boolean selected;    /* Been selected? */
    boolean preselected; /*   in advance?  */
    char selector;       /* Char used to select this entry. */
    char gselector;      /* Group selector. */
    Widget w;
    int window;
} x11_menu_item;

struct menu {
    x11_menu_item *base;  /* Starting pointer for item list. */
    x11_menu_item *last;  /* End pointer for item list. */
    const char *query;    /* Query string. */
    const char *gacc;     /* Group accelerators. */
    int count;            /* Number of strings. */
    char curr_selector;   /* Next keyboard accelerator to assign, */
                          /*   if 0, then we're out.              */
};

struct menu_info_t {
    struct menu curr_menu; /* Menu being displayed. */
    struct menu new_menu;  /* New menu being built. */

    XFontStruct *fs;           /* Font for the window. */
    long menu_count;           /* number entered by user */
    Dimension line_height;     /* Total height of a line of text. */
    Dimension internal_height; /* Internal height between widget & border */
    Dimension internal_width;  /* Internal width between widget & border */
    short how;                 /* Menu mode PICK_NONE, PICK_ONE, PICK_ANY */
    boolean is_menu;   /* Has been confirmed to being a menu window. */
    boolean is_active; /* TRUE when waiting for user input. */
    boolean is_up;     /* TRUE when window is popped-up. */
    boolean cancelled; /* Menu has been explicitly cancelled. */
    boolean counting;  /* true when menu_count has a valid value */
    boolean permi;
    boolean disable_mcolors; /* disable menucolors */

    int permi_x, permi_y; /* perm_invent window x,y */
    int permi_w, permi_h; /* perm_invent window wid, hei */
};

/*
 * Information specific to a text window.
 */
struct text_info_t {
    struct text_buffer text;
    XFontStruct *fs;        /* Font for the text window. */
    int max_width;          /* Width of widest line so far. */
    int extra_width,        /* Sum of left and right border widths. */
        extra_height;       /* Sum of top and bottom border widths. */
    boolean blocked;        /*  */
    boolean destroy_on_ack; /* Destroy this window when acknowledged. */
#ifdef GRAPHIC_TOMBSTONE
    boolean is_rip; /* This window needs a tombstone. */
#endif
};

/*
 * Basic window structure.
 */
struct xwindow {
    int type;              /* type of nethack window */
    Widget popup;          /* direct parent of widget w or viewport */
    Widget w;              /* the widget that does things */
    Dimension pixel_width; /* window size, in pixels */
    Dimension pixel_height;
    int prevx, cursx; /* Cursor position, only used by    */
    int prevy, cursy; /*   map and "plain" status windows.*/

    boolean nh_colors_inited;
    XColor nh_colors[CLR_MAX];
    XFontStruct *boldfs;       /* Bold font */
    Display *boldfs_dpy;       /* Bold font display */
    char *title;

    union {
        struct map_info_t *Map_info;       /* map window info */
        struct mesg_info_t *Mesg_info;     /* message window info */
        struct status_info_t *Status_info; /* status window info */
        struct menu_info_t *Menu_info;     /* menu window info */
        struct text_info_t *Text_info;     /* menu window info */
    } Win_info;
    boolean keep_window;
};

/* Defines to use for the window information union. */
#define map_information Win_info.Map_info
#define mesg_information Win_info.Mesg_info
#define status_information Win_info.Status_info
#define menu_information Win_info.Menu_info
#define text_information Win_info.Text_info

#define MAX_WINDOWS 20 /* max number of open windows */

#define NHW_NONE 0 /* Unallocated window type.  Must be    */
                   /* different from any other NHW_* type. */

#define NO_CLICK 0 /* No click occurred on the map window. Must */
                   /* be different than CLICK_1 and CLICK_2.    */

#define DEFAULT_MESSAGE_WIDTH 60 /* width in chars of the message window */

#define DISPLAY_FILE_SIZE 35 /* Max number of lines in the default */
                             /* file display window.               */

#define MAX_KEY_STRING 64 /* String size for converting a keypress */
                          /* event into a character(s)             */

#define DEFAULT_LINES_DISPLAYED 12 /* # of lines displayed message window */
#define MAX_HISTORY 60             /* max history saved on message window */

/* Window variables (winX.c). */
E struct xwindow window_list[MAX_WINDOWS];
E XtAppContext app_context; /* context of application */
E Widget toplevel;          /* toplevel widget */
E Atom wm_delete_window;    /* delete window protocol */
E boolean exit_x_event;     /* exit condition for event loop */
#define EXIT_ON_KEY_PRESS 0 /* valid values for exit_x_event */
#define EXIT_ON_KEY_OR_BUTTON_PRESS 1
#define EXIT_ON_EXIT 2
#define EXIT_ON_SENT_EVENT 3
E int click_x, click_y, click_button, updated_inventory;
E boolean plsel_ask_name;

typedef struct {
    Boolean slow;             /* issue prompts between map and message wins */
    Boolean fancy_status;     /* use "fancy" status vs. TTY-style status */
    Boolean autofocus;        /* grab pointer focus for popup windows */
    Boolean message_line;     /* separate current turn mesgs from prev ones */
    Boolean highlight_prompt; /* if 'slow', highlight yn prompts */
    Boolean double_tile_size; /* double tile size */
    String tile_file;         /* name of file to open for tiles */
    String icon;              /* name of desired icon */
    int message_lines;        /* number of lines to attempt to show */
    int extcmd_height_delta;  /* bottom margin for extended command menu */
    String pet_mark_bitmap;   /* X11 bitmap file used to mark pets */
    Pixel pet_mark_color;     /* color of pet mark */
    String pilemark_bitmap;   /* X11 bitmap file used to mark item piles */
    Pixel pilemark_color;     /* color of item pile mark */
#ifdef GRAPHIC_TOMBSTONE
    String tombstone; /* name of XPM file for tombstone */
    int tombtext_x;   /* x-coord of center of first tombstone text */
    int tombtext_y;   /* y-coord of center of first tombstone text */
    int tombtext_dx;  /* x-displacement between tombstone line */
    int tombtext_dy;  /* y-displacement between tombstone line */
#endif
} AppResources;

E AppResources appResources;
E void (*input_func)();

extern struct window_procs X11_procs;

/* Check for an invalid window id. */
#define check_winid(window)                                             \
    if ((window) < 0 || (window) >= MAX_WINDOWS) {                      \
        panic("illegal windid [%d] in %s at line %d", window, __FILE__, \
              __LINE__);                                                \
    }

/* ### dialogs.c ### */
E Widget
FDECL(CreateDialog, (Widget, String, XtCallbackProc, XtCallbackProc));
E void FDECL(SetDialogPrompt, (Widget, String));
E String FDECL(GetDialogResponse, (Widget));
E void FDECL(SetDialogResponse, (Widget, String, unsigned));
E void FDECL(positionpopup, (Widget, BOOLEAN_P));

/* ### winX.c ### */
E struct xwindow *FDECL(find_widget, (Widget));
E XColor FDECL(get_nhcolor, (struct xwindow *, int));
E void FDECL(init_menu_nhcolors, (struct xwindow *));
E void FDECL(load_boldfont, (struct xwindow *, Widget));
E Boolean FDECL(nhApproxColor, (Screen *, Colormap, char *, XColor *));
E void FDECL(get_widget_window_geometry, (Widget, int *, int *, int *, int *));
E char *FDECL(fontname_boldify, (const char *));
E Dimension FDECL(nhFontHeight, (Widget));
E char FDECL(key_event_to_char, (XKeyEvent *));
E void FDECL(msgkey, (Widget, XtPointer, XEvent *));
E void FDECL(highlight_yn, (BOOLEAN_P));
E void FDECL(nh_XtPopup, (Widget, int, Widget));
E void FDECL(nh_XtPopdown, (Widget));
E void FDECL(win_X11_init, (int));
E void FDECL(find_scrollbars, (Widget, Widget *, Widget *));
E void FDECL(nh_keyscroll, (Widget, XEvent *, String *, Cardinal *));

/* ### winmesg.c ### */
E void FDECL(set_message_slider, (struct xwindow *));
E void FDECL(create_message_window, (struct xwindow *, BOOLEAN_P, Widget));
E void FDECL(destroy_message_window, (struct xwindow *));
E void FDECL(display_message_window, (struct xwindow *));
E void FDECL(append_message, (struct xwindow *, const char *));
E void FDECL(set_last_pause, (struct xwindow *));

/* ### winmap.c ### */
E void NDECL(post_process_tiles);
E void FDECL(check_cursor_visibility, (struct xwindow *));
E void FDECL(display_map_window, (struct xwindow *));
E void FDECL(clear_map_window, (struct xwindow *));
E void FDECL(map_input, (Widget, XEvent *, String *, Cardinal *));
E void FDECL(set_map_size, (struct xwindow *, DIMENSION_P, DIMENSION_P));
E void FDECL(create_map_window, (struct xwindow *, BOOLEAN_P, Widget));
E void FDECL(destroy_map_window, (struct xwindow *));
E int FDECL(x_event, (int));

/* ### winmenu.c ### */
E void FDECL(menu_delete, (Widget, XEvent *, String *, Cardinal *));
E void FDECL(menu_key, (Widget, XEvent *, String *, Cardinal *));
E void FDECL(create_menu_window, (struct xwindow *));
E void FDECL(destroy_menu_window, (struct xwindow *));

/* ### winmisc.c ### */
E void FDECL(ps_key, (Widget, XEvent *, String *,
                      Cardinal *)); /* player selection action */
E void FDECL(race_key, (Widget, XEvent *, String *,
                        Cardinal *)); /* race selection action */
E void FDECL(gend_key, (Widget, XEvent *, String *, Cardinal *)); /* gender */
E void FDECL(algn_key,
             (Widget, XEvent *, String *, Cardinal *)); /* alignment */
E void FDECL(ec_delete, (Widget, XEvent *, String *, Cardinal *));
E void FDECL(ec_key, (Widget, XEvent *, String *,
                      Cardinal *)); /* extended command action */
E void FDECL(plsel_quit, (Widget, XEvent *, String *,
                      Cardinal *)); /* player selection dialog */
E void FDECL(plsel_play, (Widget, XEvent *, String *,
                      Cardinal *)); /* player selection dialog */
E void FDECL(plsel_randomize, (Widget, XEvent *, String *,
                      Cardinal *)); /* player selection dialog */
E void NDECL(release_extended_cmds);

/* ### winstatus.c ### */
E void FDECL(create_status_window, (struct xwindow *, BOOLEAN_P, Widget));
E void FDECL(destroy_status_window, (struct xwindow *));
E void FDECL(adjust_status, (struct xwindow *, const char *));
E void NDECL(null_out_status);
E void NDECL(check_turn_events);

/* ### wintext.c ### */
E void FDECL(delete_text, (Widget, XEvent *, String *, Cardinal *));
E void FDECL(dismiss_text, (Widget, XEvent *, String *, Cardinal *));
E void FDECL(key_dismiss_text, (Widget, XEvent *, String *, Cardinal *));
#ifdef GRAPHIC_TOMBSTONE
E void FDECL(rip_dismiss_text, (Widget, XEvent *, String *, Cardinal *));
#endif
E void FDECL(add_to_text_window, (struct xwindow *, int, const char *));
E void FDECL(display_text_window, (struct xwindow *, BOOLEAN_P));
E void FDECL(create_text_window, (struct xwindow *));
E void FDECL(destroy_text_window, (struct xwindow *));
E void FDECL(clear_text_window, (struct xwindow *));
E void FDECL(append_text_buffer, (struct text_buffer *, const char *,
                                  BOOLEAN_P)); /* text buffer routines */
E void FDECL(init_text_buffer, (struct text_buffer *));
E void FDECL(clear_text_buffer, (struct text_buffer *));
E void FDECL(free_text_buffer, (struct text_buffer *));
#ifdef GRAPHIC_TOMBSTONE
E void FDECL(calculate_rip_text, (int, time_t));
#endif

/* ### winval.c ### */
E Widget FDECL(create_value, (Widget, const char *));
E void FDECL(set_name, (Widget, const char *));
E void FDECL(set_name_width, (Widget, int));
E int FDECL(get_name_width, (Widget));
E void FDECL(set_value, (Widget, const char *));
E void FDECL(set_value_width, (Widget, int));
E int FDECL(get_value_width, (Widget));
E void FDECL(hilight_value, (Widget));
E void FDECL(swap_fg_bg, (Widget));

/* external declarations */
E char *FDECL(X11_getmsghistory, (BOOLEAN_P));
E void FDECL(X11_putmsghistory, (const char *, BOOLEAN_P));
E void FDECL(X11_init_nhwindows, (int *, char **));
E void NDECL(X11_player_selection);
E void NDECL(X11_askname);
E void NDECL(X11_get_nh_event);
E void FDECL(X11_exit_nhwindows, (const char *));
E void FDECL(X11_suspend_nhwindows, (const char *));
E void NDECL(X11_resume_nhwindows);
E winid FDECL(X11_create_nhwindow, (int));
E void FDECL(X11_clear_nhwindow, (winid));
E void FDECL(X11_display_nhwindow, (winid, BOOLEAN_P));
E void FDECL(X11_destroy_nhwindow, (winid));
E void FDECL(X11_curs, (winid, int, int));
E void FDECL(X11_putstr, (winid, int, const char *));
E void FDECL(X11_display_file, (const char *, BOOLEAN_P));
E void FDECL(X11_start_menu, (winid));
E void FDECL(X11_add_menu, (winid, int, const ANY_P *, CHAR_P, CHAR_P, int,
                            const char *, BOOLEAN_P));
E void FDECL(X11_end_menu, (winid, const char *));
E int FDECL(X11_select_menu, (winid, int, MENU_ITEM_P **));
E void NDECL(X11_update_inventory);
E void NDECL(X11_mark_synch);
E void NDECL(X11_wait_synch);
#ifdef CLIPPING
E void FDECL(X11_cliparound, (int, int));
#endif
E void FDECL(X11_print_glyph, (winid, XCHAR_P, XCHAR_P, int, int));
E void FDECL(X11_raw_print, (const char *));
E void FDECL(X11_raw_print_bold, (const char *));
E int NDECL(X11_nhgetch);
E int FDECL(X11_nh_poskey, (int *, int *, int *));
E void NDECL(X11_nhbell);
E int NDECL(X11_doprev_message);
E char FDECL(X11_yn_function, (const char *, const char *, CHAR_P));
E void FDECL(X11_getlin, (const char *, char *));
E int NDECL(X11_get_ext_cmd);
E void FDECL(X11_number_pad, (int));
E void NDECL(X11_delay_output);
E void NDECL(X11_status_init);
E void NDECL(X11_status_finish);
E void FDECL(X11_status_enablefield, (int, const char *, const char *, BOOLEAN_P));
E void FDECL(X11_status_update, (int, genericptr_t, int, int, int, unsigned long *));

/* other defs that really should go away (they're tty specific) */
E void NDECL(X11_start_screen);
E void NDECL(X11_end_screen);

#ifdef GRAPHIC_TOMBSTONE
E void FDECL(X11_outrip, (winid, int, time_t));
#else
E void FDECL(genl_outrip, (winid, int, time_t));
#endif

E void FDECL(X11_preference_update, (const char *));

#endif /* WINX_H */

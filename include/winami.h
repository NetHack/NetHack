/* NetHack 3.7	winami.h	$NHDT-Date: 1596498569 2020/08/03 23:49:29 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) Kenneth Lorber, Bethesda, Maryland, 1991. */
/* Copyright (c) Gregg Wonderly, Naperville, Illinois, 1992, 1993. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINAMI_H
#define WINAMI_H

#define MAXWINTAGS 5

/*
 * Information specific to a menu window.  First a structure for each
 * menu entry, then the structure for each menu window.
 */
typedef struct amii_mi {
    struct amii_mi *next;
    anything identifier; /* Opaque type to identify this selection */
    long glyph;          /* Glyph for menu item */
    long count;          /* Object count */
    char selected;       /* Been selected? */
    char selector;       /* Char used to select this entry. */
    char gselector;      /* Group selector */
    char canselect;      /* Can user select this entry. */
    char attr;           /* Attribute for the line. */
    char *str;           /* The text of the item. */
} amii_menu_item;

struct amii_menu {
    amii_menu_item *items; /* Starting pointer for item list. */
    amii_menu_item *last;  /* End pointer for item list. */
    const char *query;     /* Query string */
    int count;             /* Number of strings. */
    char chr;              /* Character to assign for accelerator */
};

/* descriptor for Amiga Intuition-based windows.  If we decide to cope with
 * tty-style windows also, then things will need to change. */
/* per-window data */
struct amii_WinDesc {
    xint16 type; /* type of window */
    struct amii_menu menu;
    boolean active; /* true if window is active */
    boolean wasup;  /* true if menu/text window was already open */
    short
        disprows; /* Rows displayed so far (used for paging in message win) */
    coordxy offx, offy;         /* offset from topleft of display */
    short vwx, vwy, vcx, vcy; /* View cursor location */
    short rows, cols;         /* dimensions */
    short curx, cury;         /* current cursor position */
    short maxrow, maxcol;     /* the maximum size used -- for INVEN wins */
    /* maxcol is also used by WIN_MESSAGE for */
    /* tracking the ^P command */
    char **data;        /* window data [row][column] */
    menu_item *mi;      /* Menu information */
    char *resp;         /* valid menu responses (for NHW_INVEN) */
    char *canresp;      /* cancel responses; 1st is the return value */
    char *morestr;      /* string to display instead of default */
                        /* amiga stuff */
    struct Window *win; /* Intuition window pointer */
#ifdef INTUI_NEW_LOOK
    struct ExtNewWindow *newwin; /* NewWindow alloc'd */
#else
    struct NewWindow *newwin; /* ExtNewWindow alloc'd */
#endif
#ifdef INTUI_NEW_LOOK
    struct TagItem wintags[MAXWINTAGS]; /* Tag items for this window */
#else
    long wintags[MAXWINTAGS * 2];
#endif
    void *hook;         /* Hook structure pointer for tiles version */
#define FLMAP_INGLYPH 1 /* An NHW_MAP window is in glyph mode */
#define FLMAP_CURSUP 2  /* An NHW_MAP window has the cursor displayed */
#define FLMAP_SKIP 4
#define FLMSG_FIRST \
    1 /* First message in the NHW_MESSAGE window for this turn */
    long wflags;
    short cursx, cursy; /* Where the cursor is displayed at */
    short curs_apen,    /* Color cursor is displayed in */
        curs_bpen;
};

/* descriptor for intuition-based displays -- all the per-display data */
/* this is a generic thing - think of it as Screen level */

struct amii_DisplayDesc {
    /* we need this for Screen size (which will vary with display mode) */
    uchar rows, cols; /* width & height of display in text units */
    short xpix, ypix; /* width and height of display in pixels */
    int toplin;       /* flag for topl stuff */
    int rawprint;     /* number of raw_printed lines since synch */
    winid lastwin;    /* last window used for I/O */
};

typedef enum {
    WEUNK,
    WEKEY,
    WEMOUSE,
    WEMENU,
} WETYPE;

typedef struct WEVENT {
    WETYPE type;
    union {
        int key;
        struct {
            int x, y;
            int qual;
        } mouse;
        long menucode;
    } un;
} WEVENT;

#define MAXWIN 20 /* maximum number of windows, cop-out */

/* port specific variable declarations */
extern winid WIN_BASE;
extern winid WIN_OVER;
#define NHW_BASE 6
#define NHW_OVER 7 /* overview window */

extern struct amii_WinDesc *amii_wins[MAXWIN + 1];

extern struct amii_DisplayDesc
    *amiIDisplay; /* the Amiga Intuition descriptor */

extern char morc;         /* last character typed to xwaitforspace */
extern char defmorestr[]; /* default --more-- prompt */

#endif /* WINAMI_H */

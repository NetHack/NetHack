/* NetHack 3.6	wintty.h	$NHDT-Date: 1558330405 2019/05/20 05:33:25 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.34 $ */
/* Copyright (c) David Cohrs, 1991,1992				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINTTY_H
#define WINTTY_H

#define E extern

#ifndef WINDOW_STRUCTS
#define WINDOW_STRUCTS

/* menu structure */
typedef struct tty_mi {
    struct tty_mi *next;
    anything identifier; /* user identifier */
    long count;          /* user count */
    char *str;           /* description string (including accelerator) */
    int attr;            /* string attribute */
    boolean selected;    /* TRUE if selected by user */
    char selector;       /* keyboard accelerator */
    char gselector;      /* group accelerator */
} tty_menu_item;

/* descriptor for tty-based windows */
struct WinDesc {
    int flags;           /* window flags */
    xchar type;          /* type of window */
    boolean active;      /* true if window is active */
    short offx, offy;    /* offset from topleft of display */
    long rows, cols;     /* dimensions */
    long curx, cury;     /* current cursor position */
    long maxrow, maxcol; /* the maximum size used -- for MENU wins */
    /* maxcol is also used by WIN_MESSAGE for */
    /* tracking the ^P command */
    short *datlen;         /* allocation size for *data */
    char **data;           /* window data [row][column] */
    char *morestr;         /* string to display instead of default */
    tty_menu_item *mlist;  /* menu information (MENU) */
    tty_menu_item **plist; /* menu page pointers (MENU) */
    long plist_size;       /* size of allocated plist (MENU) */
    long npages;           /* number of pages in menu (MENU) */
    long nitems;           /* total number of items (MENU) */
    short how;             /* menu mode - pick 1 or N (MENU) */
    char menu_ch;          /* menu char (MENU) */
};

/* window flags */
#define WIN_CANCELLED 1
#define WIN_STOP 1        /* for NHW_MESSAGE; stops output */
#define WIN_LOCKHISTORY 2 /* for NHW_MESSAGE; suppress history updates */

/* descriptor for tty-based displays -- all the per-display data */
struct DisplayDesc {
    short rows, cols; /* width and height of tty display */
    short curx, cury; /* current cursor position on the screen */
#ifdef TEXTCOLOR
    int color; /* current color */
#endif
    int attrs;         /* attributes in effect */
    int toplin;        /* flag for topl stuff */
    int rawprint;      /* number of raw_printed lines since synch */
    int inmore;        /* non-zero if more() is active */
    int inread;        /* non-zero if reading a character */
    int intr;          /* non-zero if inread was interrupted */
    winid lastwin;     /* last window used for I/O */
    char dismiss_more; /* extra character accepted at --More-- */
};

#endif /* WINDOW_STRUCTS */

#ifdef STATUS_HILITES
struct tty_status_fields {
    int idx;
    int color;
    int attr;
    int x, y;
    size_t lth;
    boolean valid;
    boolean dirty;
    boolean redraw;
    boolean sanitycheck; /* was 'last_in_row' */
};
#endif

#define MAXWIN 20 /* maximum number of windows, cop-out */

/* tty dependent window types */
#ifdef NHW_BASE
#undef NHW_BASE
#endif
#define NHW_BASE 6

extern struct window_procs tty_procs;

/* port specific variable declarations */
extern winid BASE_WINDOW;

extern struct WinDesc *wins[MAXWIN];

extern struct DisplayDesc *ttyDisplay; /* the tty display descriptor */

extern char morc;         /* last character typed to xwaitforspace */
extern char defmorestr[]; /* default --more-- prompt */

/* port specific external function references */

/* ### getline.c ### */
E void xwaitforspace(const char *);

/* ### termcap.c, video.c ### */

E void tty_startup(int *, int *);
#ifndef NO_TERMS
E void tty_shutdown(void);
#endif
E int xputc(int);
E void xputs(const char *);
#if defined(SCREEN_VGA) || defined(SCREEN_8514)
E void xputg(int, int, unsigned);
#endif
E void cl_end(void);
E void clear_screen(void);
E void home(void);
E void standoutbeg(void);
E void standoutend(void);
#if 0
E void revbeg(void);
E void boldbeg(void);
E void blinkbeg(void);
E void dimbeg(void);
E void m_end(void);
#endif
E void backsp(void);
E void graph_on(void);
E void graph_off(void);
E void cl_eos(void);

/*
 * termcap.c (or facsimiles in other ports) is the right place for doing
 * strange and arcane things such as outputting escape sequences to select
 * a color or whatever.  wintty.c should concern itself with WHERE to put
 * stuff in a window.
 */
E int term_attr_fixup(int);
E void term_start_attr(int attr);
E void term_end_attr(int attr);
E void term_start_raw_bold(void);
E void term_end_raw_bold(void);

#ifdef TEXTCOLOR
E void term_end_color(void);
E void term_start_color(int color);
E int has_color(int color);
#endif /* TEXTCOLOR */

/* ### topl.c ### */

E void show_topl(const char *);
E void remember_topl(void);
E void addtopl(const char *);
E void more(void);
E void update_topl(const char *);
E void putsyms(const char *);

/* ### wintty.c ### */
#ifdef CLIPPING
E void setclipped(void);
#endif
E void docorner(int, int);
E void end_glyphout(void);
E void g_putch(int);
E void win_tty_init(int);

/* external declarations */
E void tty_init_nhwindows(int *, char **);
E void tty_preference_update(const char *);
E void tty_player_selection(void);
E void tty_askname(void);
E void tty_get_nh_event(void);
E void tty_exit_nhwindows(const char *);
E void tty_suspend_nhwindows(const char *);
E void tty_resume_nhwindows(void);
E winid tty_create_nhwindow(int);
E void tty_clear_nhwindow(winid);
E void tty_display_nhwindow(winid, boolean);
E void tty_dismiss_nhwindow(winid);
E void tty_destroy_nhwindow(winid);
E void tty_curs(winid, int, int);
E void tty_putstr(winid, int, const char *);
E void tty_display_file(const char *, boolean);
E void tty_start_menu(winid);
E void tty_add_menu(winid, int, const ANY_P *, char, char, int,
                            const char *, boolean);
E void tty_end_menu(winid, const char *);
E int tty_select_menu(winid, int, MENU_ITEM_P **);
E char tty_message_menu(char, int, const char *);
E void tty_update_inventory(void);
E void tty_mark_synch(void);
E void tty_wait_synch(void);
#ifdef CLIPPING
E void tty_cliparound(int, int);
#endif
#ifdef POSITIONBAR
E void tty_update_positionbar(char *);
#endif
E void tty_print_glyph(winid, xchar, xchar, int, int);
E void tty_raw_print(const char *);
E void tty_raw_print_bold(const char *);
E int tty_nhgetch(void);
E int tty_nh_poskey(int *, int *, int *);
E void tty_nhbell(void);
E int tty_doprev_message(void);
E char tty_yn_function(const char *, const char *, char);
E void tty_getlin(const char *, char *);
E int tty_get_ext_cmd(void);
E void tty_number_pad(int);
E void tty_delay_output(void);
#ifdef CHANGE_COLOR
E void tty_change_color(int color, long rgb, int reverse);
#ifdef MAC
E void tty_change_background(int white_or_black);
E short set_tty_font_name(winid, char *);
#endif
E char *tty_get_color_string(void);
#endif
E void tty_status_enablefield(int, const char *, const char *, boolean);
E void tty_status_init(void);
E void tty_status_update(int, genericptr_t, int, int, int, unsigned long *);

/* other defs that really should go away (they're tty specific) */
E void tty_start_screen(void);
E void tty_end_screen(void);

E void genl_outrip(winid, int, time_t);

E char *tty_getmsghistory(boolean);
E void tty_putmsghistory(const char *, boolean);

#ifdef NO_TERMS
#ifdef MAC
#ifdef putchar
#undef putchar
#undef putc
#endif
#define putchar term_putc
#define fflush term_flush
#define puts term_puts
E int term_putc(int c);
E int term_flush(void *desc);
E int term_puts(const char *str);
#endif /* MAC */
#if defined(MSDOS) || defined(WIN32)
#if defined(SCREEN_BIOS) || defined(SCREEN_DJGPPFAST) || defined(WIN32)
#undef putchar
#undef putc
#undef puts
#define putchar(x) xputc(x) /* these are in video.c, nttty.c */
#define putc(x) xputc(x)
#define puts(x) xputs(x)
#endif /*SCREEN_BIOS || SCREEN_DJGPPFAST || WIN32 */
#ifdef POSITIONBAR
E void video_update_positionbar(char *);
#endif
#endif /*MSDOS*/
#endif /*NO_TERMS*/

#undef E

#endif /* WINTTY_H */

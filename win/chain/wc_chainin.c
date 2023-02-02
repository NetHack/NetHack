/* NetHack 3.7	wc_chainin.c	$NHDT-Date: 1596498323 2020/08/03 23:45:23 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Kenneth Lorber, 2012                             */
/* NetHack may be freely redistributed.  See license for details. */

/* -chainin is an internal processor that changes the flow from window_procs
 * to chain_procs. */

#include "hack.h"

void chainin_init_nhwindows(int *, char **);
void chainin_player_selection(void);
void chainin_askname(void);
void chainin_get_nh_event(void);
void chainin_exit_nhwindows(const char *);
void chainin_suspend_nhwindows(const char *);
void chainin_resume_nhwindows(void);
winid chainin_create_nhwindow(int);
void chainin_clear_nhwindow(winid);
void chainin_display_nhwindow(winid, boolean);
void chainin_destroy_nhwindow(winid);
void chainin_curs(winid, int, int);
void chainin_putstr(winid, int, const char *);
void chainin_putmixed(winid, int, const char *);
void chainin_display_file(const char *, boolean);
void chainin_start_menu(winid, unsigned long);
void chainin_add_menu(winid, const glyph_info *, const ANY_P *,
                         char, char, int, int,
                         const char *, unsigned int);
void chainin_end_menu(winid, const char *);
int chainin_select_menu(winid, int, MENU_ITEM_P **);
char chainin_message_menu(char, int, const char *);
void chainin_mark_synch(void);
void chainin_wait_synch(void);
#ifdef CLIPPING
void chainin_cliparound(int, int);
#endif
#ifdef POSITIONBAR
void chainin_update_positionbar(char *);
#endif
void chainin_print_glyph(winid, coordxy, coordxy,
                            const glyph_info *, const glyph_info *);
void chainin_raw_print(const char *);
void chainin_raw_print_bold(const char *);
int chainin_nhgetch(void);
int chainin_nh_poskey(coordxy *, coordxy *, int *);
void chainin_nhbell(void);
int chainin_doprev_message(void);
char chainin_yn_function(const char *, const char *, char);
void chainin_getlin(const char *, char *);
int chainin_get_ext_cmd(void);
void chainin_number_pad(int);
void chainin_delay_output(void);
#ifdef CHANGE_COLOR
void chainin_change_color(int, long, int);
#ifdef MAC
void chainin_change_background(int);
short chainin_set_font_name(winid, char *);
#endif
char *chainin_get_color_string(void);
#endif

    /* other defs that really should go away (they're tty specific) */
void chainin_start_screen(void);
void chainin_end_screen(void);
void chainin_outrip(winid, int, time_t);
void chainin_preference_update(const char *);
char *chainin_getmsghistory(boolean);
void chainin_putmsghistory(const char *, boolean);
void chainin_status_init(void);
void chainin_status_finish(void);
void chainin_status_enablefield(int, const char *, const char *,
                                boolean);
void chainin_status_update(int, genericptr_t, int, int, int,
                           unsigned long *);

boolean chainin_can_suspend(void);
void chainin_update_inventory(int);
win_request_info *chainin_ctrl_nhwindow(winid, int, win_request_info *);

void *chainin_procs_chain(int cmd, int n, void *me, void *nextprocs, void *nextdata);
void chainin_procs_init(int dir);

struct chainin_data {
    struct chain_procs *nprocs;
    void *ndata;

    int linknum;
};

/* Normally, a processor gets this information from the first parm of each
 * call, but here we are keeping the original API, so that parm doesn't exist,
 * so we use this instead. */
static struct chainin_data *cibase;

void *
chainin_procs_chain(
    int cmd,
    int n,
    void *me,
    void *nextprocs,
    void *nextdata)
{
    struct chainin_data *tdp = 0;

    switch (cmd) {
    case WINCHAIN_ALLOC:
        tdp = (struct chainin_data *) alloc(sizeof *tdp);
        tdp->nprocs = 0;
        tdp->ndata = 0;
        tdp->linknum = n;
        cibase = 0;
        break;
    case WINCHAIN_INIT:
        tdp = me;
        tdp->nprocs = nextprocs;
        tdp->ndata = nextdata;
        break;
    default:
        panic("chainin_procs_chain: bad cmd\n");
        /*NOTREACHED*/
    }
    return tdp;
}

/* XXX if we don't need this, take it out of the table */
void
chainin_procs_init(
    int dir UNUSED)
{
}

/***
 *** winprocs
 ***/

void
chainin_init_nhwindows(
    int *argcp,
    char **argv)
{
    (*cibase->nprocs->win_init_nhwindows)(cibase->ndata, argcp, argv);
}

void
chainin_player_selection(void)
{
    (*cibase->nprocs->win_player_selection)(cibase->ndata);
}

void
chainin_askname(void)
{
    (*cibase->nprocs->win_askname)(cibase->ndata);
}

void
chainin_get_nh_event(void)
{
    (*cibase->nprocs->win_get_nh_event)(cibase->ndata);
}

void
chainin_exit_nhwindows(
    const char *str)
{
    (*cibase->nprocs->win_exit_nhwindows)(cibase->ndata, str);
}

void
chainin_suspend_nhwindows(
    const char *str)
{
    (*cibase->nprocs->win_suspend_nhwindows)(cibase->ndata, str);
}

void
chainin_resume_nhwindows(void)
{
    (*cibase->nprocs->win_resume_nhwindows)(cibase->ndata);
}

winid
chainin_create_nhwindow(
    int type)
{
    winid rv;

    rv = (*cibase->nprocs->win_create_nhwindow)(cibase->ndata, type);

    return rv;
}

void
chainin_clear_nhwindow(
    winid window)
{
    (*cibase->nprocs->win_clear_nhwindow)(cibase->ndata, window);
}

void
chainin_display_nhwindow(
    winid window,
    boolean blocking)
{
    (*cibase->nprocs->win_display_nhwindow)(cibase->ndata, window, blocking);
}

void
chainin_destroy_nhwindow(
    winid window)
{
    (*cibase->nprocs->win_destroy_nhwindow)(cibase->ndata, window);
}

void
chainin_curs(
    winid window,
    int x,
    int y)
{
    (*cibase->nprocs->win_curs)(cibase->ndata, window, x, y);
}

void
chainin_putstr(
    winid window,
    int attr,
    const char *str)
{
    (*cibase->nprocs->win_putstr)(cibase->ndata, window, attr, str);
}

void
chainin_putmixed(
    winid window,
    int attr,
    const char *str)
{
    (*cibase->nprocs->win_putmixed)(cibase->ndata, window, attr, str);
}

void
chainin_display_file(
    const char *fname,
    boolean complain)
{
    (*cibase->nprocs->win_display_file)(cibase->ndata, fname, complain);
}

void
chainin_start_menu(
    winid window,
    unsigned long mbehavior)
{
    (*cibase->nprocs->win_start_menu)(cibase->ndata, window, mbehavior);
}

void
chainin_add_menu(
    winid window,               /* window to use, must be of type NHW_MENU */
    const glyph_info *glyphinfo, /* glyph and other info to display with item */
    const anything *identifier, /* what to return if selected */
    char ch,                    /* keyboard accelerator (0 = pick our own) */
    char gch,                   /* group accelerator (0 = no group) */
    int attr,                   /* attribute for string (like tty_putstr()) */
    int clr,                    /* attribute for string (like tty_putstr()) */
    const char *str,            /* menu string */
    unsigned int itemflags)     /* flags such as item is marked as selected
                               MENU_ITEMFLAGS_SELECTED */
{
    (*cibase->nprocs->win_add_menu)(cibase->ndata, window, glyphinfo,
                                    identifier, ch, gch, attr, clr, str, itemflags);
}

void
chainin_end_menu(
    winid window,
    const char *prompt)
{
    (*cibase->nprocs->win_end_menu)(cibase->ndata, window, prompt);
}

int
chainin_select_menu(
    winid window,
    int how,
    menu_item **menu_list)
{
    int rv;

    rv = (*cibase->nprocs->win_select_menu)(cibase->ndata, window, how,
                                            (void *) menu_list);

    return rv;
}

char
chainin_message_menu(
    char let,
    int how,
    const char *mesg)
{
    char rv;

    rv = (*cibase->nprocs->win_message_menu)(cibase->ndata, let, how, mesg);

    return rv;
}

void
chainin_update_inventory(int arg)
{
    (*cibase->nprocs->win_update_inventory)(cibase->ndata, arg);
}

void
chainin_mark_synch(void)
{
    (*cibase->nprocs->win_mark_synch)(cibase->ndata);
}

void
chainin_wait_synch(void)
{
    (*cibase->nprocs->win_wait_synch)(cibase->ndata);
}

#ifdef CLIPPING
void
chainin_cliparound(int x, int y)
{
    (*cibase->nprocs->win_cliparound)(cibase->ndata, x, y);
}
#endif

#ifdef POSITIONBAR
void
chainin_update_positionbar(char *posbar)
{
    (*cibase->nprocs->win_update_positionbar)(cibase->ndata, posbar);
}
#endif

/* XXX can we decode the glyph in a meaningful way? */
void
chainin_print_glyph(
    winid window,
    coordxy x,
    coordxy y,
    const glyph_info *glyphinfo,
    const glyph_info *bkglyphinfo)
{
    (*cibase->nprocs->win_print_glyph)(cibase->ndata, window, x, y, glyphinfo, bkglyphinfo);
}

void
chainin_raw_print(const char *str)
{
    (*cibase->nprocs->win_raw_print)(cibase->ndata, str);
}

void
chainin_raw_print_bold(const char *str)
{
    (*cibase->nprocs->win_raw_print_bold)(cibase->ndata, str);
}

int
chainin_nhgetch(void)
{
    int rv;

    rv = (*cibase->nprocs->win_nhgetch)(cibase->ndata);

    return rv;
}

int
chainin_nh_poskey(
    coordxy *x,
    coordxy *y,
    int *mod)
{
    int rv;

    rv = (*cibase->nprocs->win_nh_poskey)(cibase->ndata, x, y, mod);

    return rv;
}

void
chainin_nhbell(void)
{
    (*cibase->nprocs->win_nhbell)(cibase->ndata);
}

int
chainin_doprev_message(void)
{
    int rv;

    rv = (*cibase->nprocs->win_doprev_message)(cibase->ndata);

    return rv;
}

char
chainin_yn_function(
    const char *query,
    const char *resp,
    char def)
{
    int rv;

    rv = (*cibase->nprocs->win_yn_function)(cibase->ndata, query, resp, def);

    return rv;
}

void
chainin_getlin(
    const char *query,
    char *bufp)
{
    (*cibase->nprocs->win_getlin)(cibase->ndata, query, bufp);
}

int
chainin_get_ext_cmd(void)
{
    int rv;

    rv = (*cibase->nprocs->win_get_ext_cmd)(cibase->ndata);

    return rv;
}

void
chainin_number_pad(int state)
{
    (*cibase->nprocs->win_number_pad)(cibase->ndata, state);
}

void
chainin_delay_output(void)
{
    (*cibase->nprocs->win_delay_output)(cibase->ndata);
}

#ifdef CHANGE_COLOR
void
chainin_change_color(
    int color,
    long value,
    int reverse)
{
    (*cibase->nprocs->win_change_color)(cibase->ndata, color, value, reverse);
}

#ifdef MAC
void
chainin_change_background(int bw)
{
    (*cibase->nprocs->win_change_background)(cibase->ndata, bw);
}

short
chainin_set_font_name(
    winid window,
    char *font)
{
    short rv;

    rv = (*cibase->nprocs->win_set_font_name)(cibase->ndata, window, font);

    return rv;
}
#endif

char *
trace_get_color_string(void)
{
    char *rv;

    rv = (*cibase->nprocs->win_get_color_string)(cibase->ndata);

    return rv;
}

#endif

/* other defs that really should go away (they're tty specific) */
void
chainin_start_screen(void)
{
    (*cibase->nprocs->win_start_screen)(cibase->ndata);
}

void
chainin_end_screen(void)
{
    (*cibase->nprocs->win_end_screen)(cibase->ndata);
}

void
chainin_outrip(
    winid tmpwin,
    int how,
    time_t when)
{
    (*cibase->nprocs->win_outrip)(cibase->ndata, tmpwin, how, when);
}

void
chainin_preference_update(const char *pref)
{
    (*cibase->nprocs->win_preference_update)(cibase->ndata, pref);
}

char *
chainin_getmsghistory(boolean init)
{
    char *rv;

    rv = (*cibase->nprocs->win_getmsghistory)(cibase->ndata, init);

    return rv;
}

void
chainin_putmsghistory(
    const char *msg,
    boolean is_restoring)
{
    (*cibase->nprocs->win_putmsghistory)(cibase->ndata, msg, is_restoring);
}

void
chainin_status_init(void)
{
    (*cibase->nprocs->win_status_init)(cibase->ndata);
}

void
chainin_status_finish(void)
{
    (*cibase->nprocs->win_status_finish)(cibase->ndata);
}

void
chainin_status_enablefield(
    int fieldidx,
    const char *nm,
    const char *fmt,
    boolean enable)
{
    (*cibase->nprocs->win_status_enablefield)(cibase->ndata, fieldidx, nm,
                                              fmt, enable);
}

void chainin_status_update(
    int idx,
    genericptr_t ptr,
    int chg, int percent, int color,
    unsigned long *colormasks)
{
    (*cibase->nprocs->win_status_update)(cibase->ndata, idx, ptr, chg,
                                         percent, color, colormasks);
}

boolean
chainin_can_suspend(void)
{
    boolean rv;

    rv = (*cibase->nprocs->win_can_suspend)(cibase->ndata);

    return rv;
}

win_request_info *
chainin_ctrl_nhwindow(
    winid window,
    int request,
    win_request_info *wri)
{
    boolean rv;

    rv = (*cibase->nprocs->win_ctrl_nhwindow)(cibase->ndata, window,
                                                   request, wri);
    return rv;
}

struct window_procs chainin_procs = {
    WPIDMINUS(chainin), 0, /* wincap */
    0,             /* wincap2 */
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, /* color availability */
    /*
    XXX problem - the above need to come from the real window port, possibly
    modified.  May need to do something to call an additional init fn later
    or if this is the only place like this the choose_windows fn can do the
    fixup
    (but not if the value can be modified by the stack?)  TBD
    */
    chainin_init_nhwindows,
    chainin_player_selection, chainin_askname, chainin_get_nh_event,
    chainin_exit_nhwindows, chainin_suspend_nhwindows,
    chainin_resume_nhwindows, chainin_create_nhwindow, chainin_clear_nhwindow,
    chainin_display_nhwindow, chainin_destroy_nhwindow, chainin_curs,
    chainin_putstr, chainin_putmixed, chainin_display_file,
    chainin_start_menu, chainin_add_menu, chainin_end_menu,
    chainin_select_menu, chainin_message_menu, chainin_update_inventory,
    chainin_mark_synch, chainin_wait_synch,
#ifdef CLIPPING
    chainin_cliparound,
#endif
#ifdef POSITIONBAR
    chainin_update_positionbar,
#endif
    chainin_print_glyph, chainin_raw_print, chainin_raw_print_bold,
    chainin_nhgetch, chainin_nh_poskey, chainin_nhbell,
    chainin_doprev_message, chainin_yn_function, chainin_getlin,
    chainin_get_ext_cmd, chainin_number_pad, chainin_delay_output,
#ifdef CHANGE_COLOR
    chainin_change_color,
#ifdef MAC
    chainin_change_background, chainin_set_font_name,
#endif
    chainin_get_color_string,
#endif

    chainin_start_screen, chainin_end_screen,

    chainin_outrip, chainin_preference_update, chainin_getmsghistory,
    chainin_putmsghistory,
    chainin_status_init, chainin_status_finish, chainin_status_enablefield,
    chainin_status_update,
    chainin_can_suspend,
    chainin_update_inventory,
    chainin_ctrl_nhwindow,
};

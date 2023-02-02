/* NetHack 3.7	wc_chainout.c	$NHDT-Date: 1596498324 2020/08/03 23:45:24 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.13 $ */
/* Copyright (c) Kenneth Lorber, 2012                             */
/* NetHack may be freely redistributed.  See license for details. */

/* -chainout is an internal processor that changes the flow from chain_procs
 * back to window_procs. */

#include "hack.h"

void chainout_init_nhwindows(void *,int *, char **);
void chainout_player_selection(void *);
void chainout_askname(void *);
void chainout_get_nh_event(void *);
void chainout_exit_nhwindows(void *,const char *);
void chainout_suspend_nhwindows(void *,const char *);
void chainout_resume_nhwindows(void *);
winid chainout_create_nhwindow(void *,int);
void chainout_clear_nhwindow(void *,winid);
void chainout_display_nhwindow(void *,winid, boolean);
void chainout_destroy_nhwindow(void *,winid);
void chainout_curs(void *,winid, int, int);
void chainout_putstr(void *,winid, int, const char *);
void chainout_putmixed(void *,winid, int, const char *);
void chainout_display_file(void *,const char *, boolean);
void chainout_start_menu(void *,winid, unsigned long);
void chainout_add_menu(void *,winid, const glyph_info *, const ANY_P *,
                         char, char, int, int,
                         const char *, unsigned int);
void chainout_end_menu(void *,winid, const char *);
int chainout_select_menu(void *,winid, int, MENU_ITEM_P **);
char chainout_message_menu(void *,char, int, const char *);
void chainout_mark_synch(void *);
void chainout_wait_synch(void *);
#ifdef CLIPPING
void chainout_cliparound(void *,int, int);
#endif
#ifdef POSITIONBAR
void chainout_update_positionbar(void *,char *);
#endif
void chainout_print_glyph(void *,winid, coordxy, coordxy,
                            const glyph_info *, const glyph_info *);
void chainout_raw_print(void *,const char *);
void chainout_raw_print_bold(void *,const char *);
int chainout_nhgetch(void *);
int chainout_nh_poskey(void *,coordxy *, coordxy *, int *);
void chainout_nhbell(void *);
int chainout_doprev_message(void *);
char chainout_yn_function(void *,const char *, const char *, char);
void chainout_getlin(void *,const char *, char *);
int chainout_get_ext_cmd(void *);
void chainout_number_pad(void *,int);
void chainout_delay_output(void *);
#ifdef CHANGE_COLOR
void chainout_change_color(void *,int, long, int);
#ifdef MAC
void chainout_change_background(void *,int);
short chainout_set_font_name(void *,winid, char *);
#endif
char *chainout_get_color_string(void *);
#endif

    /* other defs that really should go away (they're tty specific) */
void chainout_start_screen(void *);
void chainout_end_screen(void *);
void chainout_outrip(void *,winid, int, time_t);
void chainout_preference_update(void *,const char *);
char *chainout_getmsghistory(void *,boolean);
void chainout_putmsghistory(void *,const char *, boolean);
void chainout_status_init(void *);
void chainout_status_finish(void *);
void chainout_status_enablefield(void *,int, const char *, const char *,
                                boolean);
void chainout_status_update(void *,int, genericptr_t, int, int, int,
                           unsigned long *);

boolean chainout_can_suspend(void *);
void chainout_update_inventory(void *, int);
win_request_info *chainout_ctrl_nhwindow(void *, winid, int, win_request_info *);

void chainout_procs_init(int dir);
void *chainout_procs_chain(int cmd, int n, void *me, void *nextprocs, void *nextdata);

struct chainout_data {
    struct window_procs *nprocs;
#if 0
    void *ndata;

#endif
    int linknum;
};

void *
chainout_procs_chain(
    int cmd,
    int n,
    void *me,
    void *nextprocs,
    void *nextdata UNUSED)
{
    struct chainout_data *tdp = 0;

    switch (cmd) {
    case WINCHAIN_ALLOC:
        tdp = (struct chainout_data *) alloc(sizeof *tdp);
        tdp->nprocs = 0;
        tdp->linknum = n;
        break;
    case WINCHAIN_INIT:
        tdp = me;
        tdp->nprocs = nextprocs;
        break;
    default:
        panic("chainout_procs_chain: bad cmd\n");
        /*NOTREACHED*/
    }
    return tdp;
}

/* XXX if we don't need this, take it out of the table */
void
chainout_procs_init(int dir UNUSED)
{
}

/***
 *** winprocs
 ***/

void
chainout_init_nhwindows(
    void *vp,
    int *argcp,
    char **argv)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_init_nhwindows)(argcp, argv);
}

void
chainout_player_selection(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_player_selection)();
}

void
chainout_askname(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_askname)();
}

void
chainout_get_nh_event(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_get_nh_event)();
}

void
chainout_exit_nhwindows(
    void *vp,
    const char *str)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_exit_nhwindows)(str);
}

void
chainout_suspend_nhwindows(
    void *vp,
    const char *str)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_suspend_nhwindows)(str);
}

void
chainout_resume_nhwindows(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_resume_nhwindows)();
}

winid
chainout_create_nhwindow(
    void *vp,
    int type)
{
    struct chainout_data *tdp = vp;
    winid rv;

    rv = (*tdp->nprocs->win_create_nhwindow)(type);

    return rv;
}

void
chainout_clear_nhwindow(
    void *vp,
    winid window)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_clear_nhwindow)(window);
}

void
chainout_display_nhwindow(
    void *vp,
    winid window,
    boolean blocking)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_display_nhwindow)(window, blocking);
}

void
chainout_destroy_nhwindow(
    void *vp,
    winid window)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_destroy_nhwindow)(window);
}

void
chainout_curs(
    void *vp,
    winid window,
    int x,
    int y)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_curs)(window, x, y);
}

void
chainout_putstr(
    void *vp,
    winid window,
    int attr,
    const char *str)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_putstr)(window, attr, str);
}

void
chainout_putmixed(
    void *vp,
    winid window,
    int attr,
    const char *str)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_putmixed)(window, attr, str);
}

void
chainout_display_file(
    void *vp,
    const char *fname,
    boolean complain)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_display_file)(fname, complain);
}

void
chainout_start_menu(
    void *vp,
    winid window,
    unsigned long mbehavior)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_start_menu)(window, mbehavior);
}

void
chainout_add_menu(
    void *vp,
    winid window,                /* window to use, must be of type NHW_MENU */
    const glyph_info *glyphinfo, /* glyph plus info to display with item */
    const anything *identifier,  /* what to return if selected */
    char ch,                     /* keyboard accelerator (0 = pick our own) */
    char gch,                    /* group accelerator (0 = no group) */
    int attr,                    /* attribute for string (like tty_putstr()) */
    int clr,                     /* clr for string */
    const char *str,             /* menu string */
    unsigned int itemflags)      /* itemflags such as marked as selected */
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_add_menu)(window, glyphinfo, identifier, ch, gch,
                                 attr, clr, str, itemflags);
}

void
chainout_end_menu(
    void *vp,
    winid window,
    const char *prompt)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_end_menu)(window, prompt);
}

int
chainout_select_menu(
    void *vp,
    winid window,
    int how,
    menu_item **menu_list)
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_select_menu)(window, how, (void *) menu_list);

    return rv;
}

char
chainout_message_menu(
    void *vp,
    char let,
    int how,
    const char *mesg)
{
    struct chainout_data *tdp = vp;
    char rv;

    rv = (*tdp->nprocs->win_message_menu)(let, how, mesg);

    return rv;
}

void
chainout_update_inventory(void *vp, int arg)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_update_inventory)(arg);
}

void
chainout_mark_synch(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_mark_synch)();
}

void
chainout_wait_synch(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_wait_synch)();
}

#ifdef CLIPPING
void
chainout_cliparound(
    void *vp,
    int x,
    int y)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_cliparound)(x, y);
}
#endif

#ifdef POSITIONBAR
void
chainout_update_positionbar(
    void *vp,
    char *posbar)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_update_positionbar)(posbar);
}
#endif

void
chainout_print_glyph(
    void *vp,
    winid window,
    coordxy x,
    coordxy y,
    const glyph_info *glyphinfo,
    const glyph_info *bkglyphinfo)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_print_glyph)(window, x, y, glyphinfo, bkglyphinfo);
}

void
chainout_raw_print(
    void *vp,
    const char *str)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_raw_print)(str);
}

void
chainout_raw_print_bold(void *vp, const char *str)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_raw_print_bold)(str);
}

int
chainout_nhgetch(void *vp)
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_nhgetch)();

    return rv;
}

int
chainout_nh_poskey(
    void *vp,
    coordxy *x,
    coordxy *y,
    int *mod)
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_nh_poskey)(x, y, mod);

    return rv;
}

void
chainout_nhbell(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_nhbell)();
}

int
chainout_doprev_message(void *vp)
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_doprev_message)();

    return rv;
}

char
chainout_yn_function(
    void *vp,
    const char *query,
    const char *resp,
    char def)
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_yn_function)(query, resp, def);

    return rv;
}

void
chainout_getlin(
    void *vp,
    const char *query,
    char *bufp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_getlin)(query, bufp);
}

int
chainout_get_ext_cmd(void *vp)
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_get_ext_cmd)();

    return rv;
}

void
chainout_number_pad(void *vp, int state)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_number_pad)(state);
}

void
chainout_delay_output(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_delay_output)();
}

#ifdef CHANGE_COLOR
void
chainout_change_color(
    void *vp,
    int color,
    long value,
    int reverse)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_change_color)(color, value, reverse);
}

#ifdef MAC
void
chainout_change_background(
    void *vp,
    int bw)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_change_background)(bw);
}

short
chainout_set_font_name(
    void *vp,
    winid window,
    char *font)
{
    struct chainout_data *tdp = vp;
    short rv;

    rv = (*tdp->nprocs->win_set_font_name)(window, font);

    return rv;
}
#endif

char *
trace_get_color_string(void *vp)
{
    struct chainout_data *tdp = vp;
    char *rv;

    rv = (*tdp->nprocs->win_get_color_string)();

    return rv;
}

#endif

/* other defs that really should go away (they're tty specific) */
void
chainout_start_screen(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_start_screen)();
}

void
chainout_end_screen(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_end_screen)();
}

void
chainout_outrip(
    void *vp,
    winid tmpwin,
    int how,
    time_t when)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_outrip)(tmpwin, how, when);
}

void
chainout_preference_update(
    void *vp,
    const char *pref)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_preference_update)(pref);
}

char *
chainout_getmsghistory(
    void *vp,
    boolean init)
{
    struct chainout_data *tdp = vp;
    char *rv;

    rv = (*tdp->nprocs->win_getmsghistory)(init);

    return rv;
}

void
chainout_putmsghistory(
    void *vp,
    const char *msg,
    boolean is_restoring)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_putmsghistory)(msg, is_restoring);
}

void
chainout_status_init(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_status_init)();
}

void
chainout_status_finish(void *vp)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_status_finish)();
}

void
chainout_status_enablefield(
    void *vp,
    int fieldidx,
    const char *nm,
    const char *fmt,
    boolean enable)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_status_enablefield)(fieldidx, nm, fmt, enable);
}

void
chainout_status_update(
    void *vp,
    int idx,
    genericptr_t ptr,
    int chg,
    int percent,
    int color,
    unsigned long *colormasks)
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_status_update)(idx, ptr, chg, percent, color, colormasks);
}

boolean
chainout_can_suspend(void *vp)
{
    struct chainout_data *tdp = vp;
    boolean rv;

    rv = (*tdp->nprocs->win_can_suspend)();

    return rv;
}

win_request_info *
chainout_ctrl_nhwindow(
    winid window,
    int request,
    win_request_info *wri)
{
    struct chainout_data *tdp = vp;
    boolean rv;

    rv = (*tdp->nprocs->win_ctrl_nhwindow)(window,
                                           request, wri);
    return rv;
}

struct chain_procs chainout_procs = {
    WPIDMINUS(chainout), 0, /* wincap */
    0,              /* wincap2 */
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, /* color availability */
    /*
    XXX problem - the above need to come from the real window port, possibly
    modified.  May need to do something to call an additional init fn later
    or if this is the only place like this the choose_windows fn can do the
    fixup
    (but not if the value can be modified by the stack?)  TBD
    */
    chainout_init_nhwindows,
    chainout_player_selection, chainout_askname, chainout_get_nh_event,
    chainout_exit_nhwindows, chainout_suspend_nhwindows,
    chainout_resume_nhwindows, chainout_create_nhwindow,
    chainout_clear_nhwindow, chainout_display_nhwindow,
    chainout_destroy_nhwindow, chainout_curs, chainout_putstr,
    chainout_putmixed, chainout_display_file, chainout_start_menu,
    chainout_add_menu, chainout_end_menu, chainout_select_menu,
    chainout_message_menu, chainout_update_inventory, chainout_mark_synch,
    chainout_wait_synch,
#ifdef CLIPPING
    chainout_cliparound,
#endif
#ifdef POSITIONBAR
    chainout_update_positionbar,
#endif
    chainout_print_glyph, chainout_raw_print, chainout_raw_print_bold,
    chainout_nhgetch, chainout_nh_poskey, chainout_nhbell,
    chainout_doprev_message, chainout_yn_function, chainout_getlin,
    chainout_get_ext_cmd, chainout_number_pad, chainout_delay_output,
#ifdef CHANGE_COLOR
    chainout_change_color,
#ifdef MAC
    chainout_change_background, chainout_set_font_name,
#endif
    chainout_get_color_string,
#endif

    chainout_start_screen, chainout_end_screen,

    chainout_outrip, chainout_preference_update, chainout_getmsghistory,
    chainout_putmsghistory,
    chainout_status_init, chainout_status_finish, chainout_status_enablefield,
    chainout_status_update,
    chainout_can_suspend,
    chainout_update_inventory,
    chainout_ctrl_nhwindow,
};

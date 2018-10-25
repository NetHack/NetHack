/* NetHack 3.6	wc_chainout.c	$NHDT-Date: 1433806611 2015/06/08 23:36:51 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Kenneth Lorber, 2012				  */
/* NetHack may be freely redistributed.  See license for details. */

/* -chainout is an internal processor that changes the flow from chain_procs
 * back to window_procs. */

#include "hack.h"

struct chainout_data {
    struct window_procs *nprocs;
#if 0
    void *ndata;

#endif
    int linknum;
};

void *
chainout_procs_chain(cmd, n, me, nextprocs, nextdata)
int cmd;
int n;
void *me;
void *nextprocs;
void *nextdata UNUSED;
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
chainout_procs_init(dir)
int dir UNUSED;
{
}

/***
 *** winprocs
 ***/

void
chainout_init_nhwindows(vp, argcp, argv)
void *vp;
int *argcp;
char **argv;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_init_nhwindows)(argcp, argv);
}

void
chainout_player_selection(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_player_selection)();
}

void
chainout_askname(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_askname)();
}

void
chainout_get_nh_event(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_get_nh_event)();
}

void
chainout_exit_nhwindows(vp, str)
void *vp;
const char *str;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_exit_nhwindows)(str);
}

void
chainout_suspend_nhwindows(vp, str)
void *vp;
const char *str;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_suspend_nhwindows)(str);
}

void
chainout_resume_nhwindows(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_resume_nhwindows)();
}

winid
chainout_create_nhwindow(vp, type)
void *vp;
int type;
{
    struct chainout_data *tdp = vp;
    winid rv;

    rv = (*tdp->nprocs->win_create_nhwindow)(type);

    return rv;
}

void
chainout_clear_nhwindow(vp, window)
void *vp;
winid window;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_clear_nhwindow)(window);
}

void
chainout_display_nhwindow(vp, window, blocking)
void *vp;
winid window;
BOOLEAN_P blocking;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_display_nhwindow)(window, blocking);
}

void
chainout_destroy_nhwindow(vp, window)
void *vp;
winid window;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_destroy_nhwindow)(window);
}

void
chainout_curs(vp, window, x, y)
void *vp;
winid window;
int x;
int y;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_curs)(window, x, y);
}

void
chainout_putstr(vp, window, attr, str)
void *vp;
winid window;
int attr;
const char *str;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_putstr)(window, attr, str);
}

void
chainout_putmixed(vp, window, attr, str)
void *vp;
winid window;
int attr;
const char *str;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_putmixed)(window, attr, str);
}

void
chainout_display_file(vp, fname, complain)
void *vp;
const char *fname;
boolean complain;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_display_file)(fname, complain);
}

void
chainout_start_menu(vp, window)
void *vp;
winid window;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_start_menu)(window);
}

void
chainout_add_menu(vp, window, glyph, identifier, ch, gch, attr, str,
                  preselected)
void *vp;
winid window;               /* window to use, must be of type NHW_MENU */
int glyph;                  /* glyph to display with item (unused) */
const anything *identifier; /* what to return if selected */
char ch;                    /* keyboard accelerator (0 = pick our own) */
char gch;                   /* group accelerator (0 = no group) */
int attr;                   /* attribute for string (like tty_putstr()) */
const char *str;            /* menu string */
boolean preselected;        /* item is marked as selected */
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_add_menu)(window, glyph, identifier, ch, gch, attr,
                                 str, preselected);
}

void
chainout_end_menu(vp, window, prompt)
void *vp;
winid window;
const char *prompt;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_end_menu)(window, prompt);
}

int
chainout_select_menu(vp, window, how, menu_list)
void *vp;
winid window;
int how;
menu_item **menu_list;
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_select_menu)(window, how, (void *) menu_list);

    return rv;
}

char
chainout_message_menu(vp, let, how, mesg)
void *vp;
char let;
int how;
const char *mesg;
{
    struct chainout_data *tdp = vp;
    char rv;

    rv = (*tdp->nprocs->win_message_menu)(let, how, mesg);

    return rv;
}

void
chainout_update_inventory(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_update_inventory)();
}

void
chainout_mark_synch(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_mark_synch)();
}

void
chainout_wait_synch(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_wait_synch)();
}

#ifdef CLIPPING
void
chainout_cliparound(vp, x, y)
void *vp;
int x;
int y;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_cliparound)(x, y);
}
#endif

#ifdef POSITIONBAR
void
chainout_update_positionbar(vp, posbar)
void *vp;
char *posbar;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_update_positionbar)(posbar);
}
#endif

void
chainout_print_glyph(vp, window, x, y, glyph, bkglyph)
void *vp;
winid window;
xchar x, y;
int glyph, bkglyph;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_print_glyph)(window, x, y, glyph, bkglyph);
}

void
chainout_raw_print(vp, str)
void *vp;
const char *str;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_raw_print)(str);
}

void
chainout_raw_print_bold(vp, str)
void *vp;
const char *str;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_raw_print_bold)(str);
}

int
chainout_nhgetch(vp)
void *vp;
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_nhgetch)();

    return rv;
}

int
chainout_nh_poskey(vp, x, y, mod)
void *vp;
int *x;
int *y;
int *mod;
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_nh_poskey)(x, y, mod);

    return rv;
}

void
chainout_nhbell(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_nhbell)();
}

int
chainout_doprev_message(vp)
void *vp;
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_doprev_message)();

    return rv;
}

char
chainout_yn_function(vp, query, resp, def)
void *vp;
const char *query, *resp;
char def;
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_yn_function)(query, resp, def);

    return rv;
}

void
chainout_getlin(vp, query, bufp)
void *vp;
const char *query;
char *bufp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_getlin)(query, bufp);
}

int
chainout_get_ext_cmd(vp)
void *vp;
{
    struct chainout_data *tdp = vp;
    int rv;

    rv = (*tdp->nprocs->win_get_ext_cmd)();

    return rv;
}

void
chainout_number_pad(vp, state)
void *vp;
int state;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_number_pad)(state);
}

void
chainout_delay_output(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_delay_output)();
}

#ifdef CHANGE_COLOR
void
chainout_change_color(vp, color, value, reverse)
void *vp;
int color;
long value;
int reverse;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_change_color)(color, value, reverse);
}

#ifdef MAC
void
chainout_change_background(vp, bw)
void *vp;
int bw;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_change_background)(bw);
}

short
chainout_set_font_name(vp, window, font)
void *vp;
winid window;
char *font;
{
    struct chainout_data *tdp = vp;
    short rv;

    rv = (*tdp->nprocs->win_set_font_name)(window, font);

    return rv;
}
#endif

char *
trace_get_color_string(vp)
void *vp;
{
    struct chainout_data *tdp = vp;
    char *rv;

    rv = (*tdp->nprocs->win_get_color_string)();

    return rv;
}

#endif

/* other defs that really should go away (they're tty specific) */
void
chainout_start_screen(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_start_screen)();
}

void
chainout_end_screen(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_end_screen)();
}

void
chainout_outrip(vp, tmpwin, how, when)
void *vp;
winid tmpwin;
int how;
time_t when;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_outrip)(tmpwin, how, when);
}

void
chainout_preference_update(vp, pref)
void *vp;
const char *pref;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_preference_update)(pref);
}

char *
chainout_getmsghistory(vp, init)
void *vp;
boolean init;
{
    struct chainout_data *tdp = vp;
    char *rv;

    rv = (*tdp->nprocs->win_getmsghistory)(init);

    return rv;
}

void
chainout_putmsghistory(vp, msg, is_restoring)
void *vp;
const char *msg;
boolean is_restoring;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_putmsghistory)(msg, is_restoring);
}

void
chainout_status_init(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_status_init)();
}

void
chainout_status_finish(vp)
void *vp;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_status_finish)();
}

void
chainout_status_enablefield(vp, fieldidx, nm, fmt, enable)
void *vp;
int fieldidx;
const char *nm;
const char *fmt;
boolean enable;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_status_enablefield)(fieldidx, nm, fmt, enable);
}

void
chainout_status_update(vp, idx, ptr, chg, percent, color, colormasks)
void *vp;
int idx, chg, percent, color;
genericptr_t ptr;
unsigned long *colormasks;
{
    struct chainout_data *tdp = vp;

    (*tdp->nprocs->win_status_update)(idx, ptr, chg, percent, color, colormasks);
}

boolean
chainout_can_suspend(vp)
void *vp;
{
    struct chainout_data *tdp = vp;
    boolean rv;

    rv = (*tdp->nprocs->win_can_suspend)();

    return rv;
}

struct chain_procs chainout_procs = {
    "-chainout", 0, /* wincap */
    0,              /* wincap2 */
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
};

/* NetHack 3.6	wc_chainin.c	$NHDT-Date: 1433806610 2015/06/08 23:36:50 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Kenneth Lorber, 2012				  */
/* NetHack may be freely redistributed.  See license for details. */

/* -chainin is an internal processor that changes the flow from window_procs
 * to chain_procs. */

#include "hack.h"

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
chainin_procs_chain(cmd, n, me, nextprocs, nextdata)
int cmd;
int n;
void *me;
void *nextprocs;
void *nextdata;
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
chainin_procs_init(dir)
int dir UNUSED;
{
}

/***
 *** winprocs
 ***/

void
chainin_init_nhwindows(argcp, argv)
int *argcp;
char **argv;
{
    (*cibase->nprocs->win_init_nhwindows)(cibase->ndata, argcp, argv);
}

void
chainin_player_selection()
{
    (*cibase->nprocs->win_player_selection)(cibase->ndata);
}

void
chainin_askname()
{
    (*cibase->nprocs->win_askname)(cibase->ndata);
}

void
chainin_get_nh_event()
{
    (*cibase->nprocs->win_get_nh_event)(cibase->ndata);
}

void
chainin_exit_nhwindows(str)
const char *str;
{
    (*cibase->nprocs->win_exit_nhwindows)(cibase->ndata, str);
}

void
chainin_suspend_nhwindows(str)
const char *str;
{
    (*cibase->nprocs->win_suspend_nhwindows)(cibase->ndata, str);
}

void
chainin_resume_nhwindows()
{
    (*cibase->nprocs->win_resume_nhwindows)(cibase->ndata);
}

winid
chainin_create_nhwindow(type)
int type;
{
    winid rv;

    rv = (*cibase->nprocs->win_create_nhwindow)(cibase->ndata, type);

    return rv;
}

void
chainin_clear_nhwindow(window)
winid window;
{
    (*cibase->nprocs->win_clear_nhwindow)(cibase->ndata, window);
}

void
chainin_display_nhwindow(window, blocking)
winid window;
BOOLEAN_P blocking;
{
    (*cibase->nprocs->win_display_nhwindow)(cibase->ndata, window, blocking);
}

void
chainin_destroy_nhwindow(window)
winid window;
{
    (*cibase->nprocs->win_destroy_nhwindow)(cibase->ndata, window);
}

void
chainin_curs(window, x, y)
winid window;
int x;
int y;
{
    (*cibase->nprocs->win_curs)(cibase->ndata, window, x, y);
}

void
chainin_putstr(window, attr, str)
winid window;
int attr;
const char *str;
{
    (*cibase->nprocs->win_putstr)(cibase->ndata, window, attr, str);
}

void
chainin_putmixed(window, attr, str)
winid window;
int attr;
const char *str;
{
    (*cibase->nprocs->win_putmixed)(cibase->ndata, window, attr, str);
}

void
chainin_display_file(fname, complain)
const char *fname;
boolean complain;
{
    (*cibase->nprocs->win_display_file)(cibase->ndata, fname, complain);
}

void
chainin_start_menu(window)
winid window;
{
    (*cibase->nprocs->win_start_menu)(cibase->ndata, window);
}

void
chainin_add_menu(window, glyph, identifier, ch, gch, attr, str, preselected)
winid window;               /* window to use, must be of type NHW_MENU */
int glyph;                  /* glyph to display with item (unused) */
const anything *identifier; /* what to return if selected */
char ch;                    /* keyboard accelerator (0 = pick our own) */
char gch;                   /* group accelerator (0 = no group) */
int attr;                   /* attribute for string (like tty_putstr()) */
const char *str;            /* menu string */
boolean preselected;        /* item is marked as selected */
{
    (*cibase->nprocs->win_add_menu)(cibase->ndata, window, glyph, identifier,
                                    ch, gch, attr, str, preselected);
}

void
chainin_end_menu(window, prompt)
winid window;
const char *prompt;
{
    (*cibase->nprocs->win_end_menu)(cibase->ndata, window, prompt);
}

int
chainin_select_menu(window, how, menu_list)
winid window;
int how;
menu_item **menu_list;
{
    int rv;

    rv = (*cibase->nprocs->win_select_menu)(cibase->ndata, window, how,
                                            (void *) menu_list);

    return rv;
}

char
chainin_message_menu(let, how, mesg)
char let;
int how;
const char *mesg;
{
    char rv;

    rv = (*cibase->nprocs->win_message_menu)(cibase->ndata, let, how, mesg);

    return rv;
}

void
chainin_update_inventory()
{
    (*cibase->nprocs->win_update_inventory)(cibase->ndata);
}

void
chainin_mark_synch()
{
    (*cibase->nprocs->win_mark_synch)(cibase->ndata);
}

void
chainin_wait_synch()
{
    (*cibase->nprocs->win_wait_synch)(cibase->ndata);
}

#ifdef CLIPPING
void
chainin_cliparound(x, y)
int x;
int y;
{
    (*cibase->nprocs->win_cliparound)(cibase->ndata, x, y);
}
#endif

#ifdef POSITIONBAR
void
chainin_update_positionbar(posbar)
char *posbar;
{
    (*cibase->nprocs->win_update_positionbar)(cibase->ndata, posbar);
}
#endif

/* XXX can we decode the glyph in a meaningful way? */
void
chainin_print_glyph(window, x, y, glyph, bkglyph)
winid window;
xchar x, y;
int glyph, bkglyph;
{
    (*cibase->nprocs->win_print_glyph)(cibase->ndata, window, x, y, glyph, bkglyph);
}

void
chainin_raw_print(str)
const char *str;
{
    (*cibase->nprocs->win_raw_print)(cibase->ndata, str);
}

void
chainin_raw_print_bold(str)
const char *str;
{
    (*cibase->nprocs->win_raw_print_bold)(cibase->ndata, str);
}

int
chainin_nhgetch()
{
    int rv;

    rv = (*cibase->nprocs->win_nhgetch)(cibase->ndata);

    return rv;
}

int
chainin_nh_poskey(x, y, mod)
int *x;
int *y;
int *mod;
{
    int rv;

    rv = (*cibase->nprocs->win_nh_poskey)(cibase->ndata, x, y, mod);

    return rv;
}

void
chainin_nhbell()
{
    (*cibase->nprocs->win_nhbell)(cibase->ndata);
}

int
chainin_doprev_message()
{
    int rv;

    rv = (*cibase->nprocs->win_doprev_message)(cibase->ndata);

    return rv;
}

char
chainin_yn_function(query, resp, def)
const char *query, *resp;
char def;
{
    int rv;

    rv = (*cibase->nprocs->win_yn_function)(cibase->ndata, query, resp, def);

    return rv;
}

void
chainin_getlin(query, bufp)
const char *query;
char *bufp;
{
    (*cibase->nprocs->win_getlin)(cibase->ndata, query, bufp);
}

int
chainin_get_ext_cmd()
{
    int rv;

    rv = (*cibase->nprocs->win_get_ext_cmd)(cibase->ndata);

    return rv;
}

void
chainin_number_pad(state)
int state;
{
    (*cibase->nprocs->win_number_pad)(cibase->ndata, state);
}

void
chainin_delay_output()
{
    (*cibase->nprocs->win_delay_output)(cibase->ndata);
}

#ifdef CHANGE_COLOR
void
chainin_change_color(color, value, reverse)
int color;
long value;
int reverse;
{
    (*cibase->nprocs->win_change_color)(cibase->ndata, color, value, reverse);
}

#ifdef MAC
void
chainin_change_background(bw)
int bw;
{
    (*cibase->nprocs->win_change_background)(cibase->ndata, bw);
}

short
chainin_set_font_name(window, font)
winid window;
char *font;
{
    short rv;

    rv = (*cibase->nprocs->win_set_font_name)(cibase->ndata, window, font);

    return rv;
}
#endif

char *
trace_get_color_string()
{
    char *rv;

    rv = (*cibase->nprocs->win_get_color_string)(cibase->ndata);

    return rv;
}

#endif

/* other defs that really should go away (they're tty specific) */
void
chainin_start_screen()
{
    (*cibase->nprocs->win_start_screen)(cibase->ndata);
}

void
chainin_end_screen()
{
    (*cibase->nprocs->win_end_screen)(cibase->ndata);
}

void
chainin_outrip(tmpwin, how, when)
winid tmpwin;
int how;
time_t when;
{
    (*cibase->nprocs->win_outrip)(cibase->ndata, tmpwin, how, when);
}

void
chainin_preference_update(pref)
const char *pref;
{
    (*cibase->nprocs->win_preference_update)(cibase->ndata, pref);
}

char *
chainin_getmsghistory(init)
boolean init;
{
    char *rv;

    rv = (*cibase->nprocs->win_getmsghistory)(cibase->ndata, init);

    return rv;
}

void
chainin_putmsghistory(msg, is_restoring)
const char *msg;
boolean is_restoring;
{
    (*cibase->nprocs->win_putmsghistory)(cibase->ndata, msg, is_restoring);
}

void
chainin_status_init()
{
    (*cibase->nprocs->win_status_init)(cibase->ndata);
}

void
chainin_status_finish()
{
    (*cibase->nprocs->win_status_finish)(cibase->ndata);
}

void
chainin_status_enablefield(fieldidx, nm, fmt, enable)
int fieldidx;
const char *nm;
const char *fmt;
boolean enable;
{
    (*cibase->nprocs->win_status_enablefield)(cibase->ndata, fieldidx, nm,
                                              fmt, enable);
}

void
chainin_status_update(idx, ptr, chg, percent, color, colormasks)
int idx, chg, percent, color;
genericptr_t ptr;
unsigned long *colormasks;
{
    (*cibase->nprocs->win_status_update)(cibase->ndata, idx, ptr, chg,
                                         percent, color, colormasks);
}

boolean
chainin_can_suspend()
{
    boolean rv;

    rv = (*cibase->nprocs->win_can_suspend)(cibase->ndata);

    return rv;
}

struct window_procs chainin_procs = {
    "-chainin", 0, /* wincap */
    0,             /* wincap2 */
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
};

/* NetHack 3.7	wc_trace.c	$NHDT-Date: 1596498324 2020/08/03 23:45:24 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.14 $ */
/* Copyright (c) Kenneth Lorber, 2012				  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "wintty.h"
#include "func_tab.h"

#include <ctype.h>
#include <errno.h>

FILE *wc_tracelogf; /* Should be static, but it's just too useful to have
                     * access to this logfile from arbitrary other files. */
static unsigned int indent_level; /* Some winfuncs call other winfuncs, so
                                   * we need to support nesting. */

static char indentdata[10] = "         ";
#define ISCALE 1
#if 1
#define INDENT                                                             \
    &indentdata[((indent_level * ISCALE) < (sizeof(indentdata)))           \
                    ? ((sizeof(indentdata) - 1) - (indent_level * ISCALE)) \
                    : 0]
#else
/* for debugging this file */
#define INDENT                                                          \
    ({                                                                  \
        static char buf[50];                                            \
        sprintf(buf, "[%s:%d %d]\t", __func__, __LINE__, indent_level); \
        buf;                                                            \
    })
#endif
#define PRE indent_level++
#define POST indent_level--

void trace_init_nhwindows(void *,int *, char **);
void trace_player_selection(void *);
void trace_askname(void *);
void trace_get_nh_event(void *);
void trace_exit_nhwindows(void *,const char *);
void trace_suspend_nhwindows(void *,const char *);
void trace_resume_nhwindows(void *);
winid trace_create_nhwindow(void *,int);
void trace_clear_nhwindow(void *,winid);
void trace_display_nhwindow(void *,winid, boolean);
void trace_destroy_nhwindow(void *,winid);
void trace_curs(void *,winid, int, int);
void trace_putstr(void *,winid, int, const char *);
void trace_putmixed(void *,winid, int, const char *);
void trace_display_file(void *,const char *, boolean);
void trace_start_menu(void *,winid, unsigned long);
void trace_add_menu(void *,winid, const glyph_info *, const ANY_P *,
		                         char, char, int, int,
					                          const char *, unsigned int);
void trace_end_menu(void *,winid, const char *);
int trace_select_menu(void *,winid, int, MENU_ITEM_P **);
char trace_message_menu(void *,char, int, const char *);
void trace_update_inventory(void *,int);
void trace_mark_synch(void *);
void trace_wait_synch(void *);
#ifdef CLIPPING
void trace_cliparound(void *,int, int);
#endif
#ifdef POSITIONBAR
void trace_update_positionbar(void *,char *);
#endif
void trace_print_glyph(void *,winid, coordxy, coordxy,
		                            const glyph_info *, const glyph_info *);
void trace_raw_print(void *,const char *);
void trace_raw_print_bold(void *,const char *);
int trace_nhgetch(void *);
int trace_nh_poskey(void *,coordxy *, coordxy *, int *);
void trace_nhbell(void *);
int trace_doprev_message(void *);
char trace_yn_function(void *,const char *, const char *, char);
void trace_getlin(void *,const char *, char *);
int trace_get_ext_cmd(void *);
void trace_number_pad(void *,int);
void trace_delay_output(void *);
#ifdef CHANGE_COLOR
void trace_change_color(void *,int, long, int);
#ifdef MAC
void trace_change_background(void *,int);
short trace_set_font_name(void *,winid, char *);
#endif
char *trace_get_color_string(void *);
#endif

    /* other defs that really should go away (they're tty specific) */
void trace_start_screen(void *);
void trace_end_screen(void *);
void trace_outrip(void *,winid, int, time_t);
void trace_preference_update(void *,const char *);
char *trace_getmsghistory(void *,boolean);
void trace_putmsghistory(void *,const char *, boolean);
void trace_status_init(void *);
void trace_status_finish(void *);
void trace_status_enablefield(void *,int, const char *, const char *,
		                                boolean);
void trace_status_update(void *,int, genericptr_t, int, int, int,
		                           unsigned long *);

boolean trace_can_suspend(void *);

void trace_procs_init(int dir);
void *trace_procs_chain(int cmd, int n, void *me, void *nextprocs, void *nextdata);

struct trace_data {
    struct chain_procs *nprocs;
    void *ndata;

    int linknum;
};

void *
trace_procs_chain(
    int cmd,
    int n,
    void *me,
    void *nextprocs,
    void *nextdata)
{
    struct trace_data *tdp = 0;

    switch (cmd) {
    case WINCHAIN_ALLOC:
        tdp = (struct trace_data *) alloc(sizeof *tdp);
        tdp->nprocs = 0;
        tdp->ndata = 0;
        tdp->linknum = n;
        break;
    case WINCHAIN_INIT:
        tdp = me;
        tdp->nprocs = nextprocs;
        tdp->ndata = nextdata;
        break;
    default:
        panic("trace_procs_chain: bad cmd\n");
        /*NOTREACHED*/
    }
    return tdp;
}

void
trace_procs_init(int dir)
{
    char fname[200];
    long pid;

    /* processors shouldn't need this test, but just in case */
    if (dir != WININIT)
        return;

    pid = (long) getpid();
    Sprintf(fname, "%s/tlog.%ld", HACKDIR, pid);
    wc_tracelogf = fopen(fname, "w");
    if (!wc_tracelogf) {
        fprintf(stderr, "Can't open trace log file %s: %s\n", fname,
                strerror(errno));
        nh_terminate(EXIT_FAILURE);
    }
    setvbuf(wc_tracelogf, (char *) 0, _IONBF, 0);
    fprintf(wc_tracelogf, "Trace log started for pid %ld\n", pid);

    indent_level = 0;
}

/***
 *** winprocs
 ***/

void
trace_init_nhwindows(
    void *vp,
    int *argcp,
    char **argv)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sinit_nhwindows(%d,*)\n", INDENT, *argcp);

    PRE;
    (*tdp->nprocs->win_init_nhwindows)(tdp->ndata, argcp, argv);
    POST;
}

void
trace_player_selection(void *vp)
{
    struct trace_data *tdp = vp;
    fprintf(wc_tracelogf, "%splayer_selection()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_player_selection)(tdp->ndata);
    POST;
}

void
trace_askname(void *vp)
{
    struct trace_data *tdp = vp;
    fprintf(wc_tracelogf, "%saskname()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_askname)(tdp->ndata);
    POST;
}

void
trace_get_nh_event(void *vp)
{
    struct trace_data *tdp = vp;
    fprintf(wc_tracelogf, "%sget_nh_event()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_get_nh_event)(tdp->ndata);
    POST;
}

void
trace_exit_nhwindows(
    void *vp,
    const char *str)
{
    struct trace_data *tdp = vp;
    fprintf(wc_tracelogf, "%sexit_nhwindows(%s)\n", INDENT, str);

    PRE;
    (*tdp->nprocs->win_exit_nhwindows)(tdp->ndata, str);
    POST;
}

void
trace_suspend_nhwindows(
    void *vp,
    const char *str)
{
    struct trace_data *tdp = vp;
    fprintf(wc_tracelogf, "%ssuspend_nhwindows(%s)\n", INDENT, str);

    PRE;
    (*tdp->nprocs->win_suspend_nhwindows)(tdp->ndata, str);
    POST;
}

void
trace_resume_nhwindows(void *vp)
{
    struct trace_data *tdp = vp;
    fprintf(wc_tracelogf, "%sresume_nhwindows()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_resume_nhwindows)(tdp->ndata);
    POST;
}

static const char *
NHWname(int type)
{
    switch (type) {
    case NHW_MESSAGE:
        return "MESSAGE";
    case NHW_STATUS:
        return "STATUS";
    case NHW_MAP:
        return "MAP";
    case NHW_MENU:
        return "MENU";
    case NHW_TEXT:
        return "TEXT";
    case NHW_BASE:
        return "BASE";
    default: {
        static char b[20];
        sprintf(b, "(%d)", type);
        return b;
    }
    }
}

winid
trace_create_nhwindow(
    void *vp,
    int type)
{
    struct trace_data *tdp = vp;
    const char *typestring = NHWname(type);
    winid rv;

    fprintf(wc_tracelogf, "%screate_nhwindow(%s)\n", INDENT, typestring);

    PRE;
    rv = (*tdp->nprocs->win_create_nhwindow)(tdp->ndata, type);
    POST;

    fprintf(wc_tracelogf, "%s=> %d\n", INDENT, rv);
    return rv;
}

void
trace_clear_nhwindow(
    void *vp,
    winid window)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sclear_nhwindow(%d)\n", INDENT, window);

    PRE;
    (*tdp->nprocs->win_clear_nhwindow)(tdp->ndata, window);
    POST;
}

void
trace_display_nhwindow(
    void *vp,
    winid window,
    boolean blocking)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sdisplay_nhwindow(%d, %d)\n", INDENT, window,
            blocking);

    PRE;
    (*tdp->nprocs->win_display_nhwindow)(tdp->ndata, window, blocking);
    POST;
}

void
trace_destroy_nhwindow(
    void *vp,
    winid window)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sdestroy_nhwindow(%d)\n", INDENT, window);

    PRE;
    (*tdp->nprocs->win_destroy_nhwindow)(tdp->ndata, window);
    POST;
}

void
trace_curs(
    void *vp,
    winid window,
    int x,
    int y)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%scurs(%d, %d, %d)\n", INDENT, window, x, y);

    PRE;
    (*tdp->nprocs->win_curs)(tdp->ndata, window, x, y);
    POST;
}

void
trace_putstr(
    void *vp,
    winid window,
    int attr,
    const char *str)
{
    struct trace_data *tdp = vp;

    if (str) {
        fprintf(wc_tracelogf, "%sputstr(%d, %d, '%s'(%d))\n", INDENT, window,
                attr, str, (int) strlen(str));
    } else {
        fprintf(wc_tracelogf, "%sputstr(%d, %d, NULL)\n", INDENT, window,
                attr);
    }

    PRE;
    (*tdp->nprocs->win_putstr)(tdp->ndata, window, attr, str);
    POST;
}

void
trace_putmixed(
    void *vp,
    winid window,
    int attr,
    const char *str)
{
    struct trace_data *tdp = vp;

    if (str) {
        fprintf(wc_tracelogf, "%sputmixed(%d, %d, '%s'(%d))\n", INDENT,
                window, attr, str, (int) strlen(str));
    } else {
        fprintf(wc_tracelogf, "%sputmixed(%d, %d, NULL)\n", INDENT, window,
                attr);
    }

    PRE;
    (*tdp->nprocs->win_putmixed)(tdp->ndata, window, attr, str);
    POST;
}

void
trace_display_file(
    void *vp,
    const char *fname,
    boolean complain)
{
    struct trace_data *tdp = vp;

    if (fname) {
        fprintf(wc_tracelogf, "%sdisplay_file('%s'(%d), %d)\n", INDENT, fname,
                (int) strlen(fname), complain);
    } else {
        fprintf(wc_tracelogf, "%sdisplay_file(NULL, %d)\n", INDENT, complain);
    }

    PRE;
    (*tdp->nprocs->win_display_file)(tdp->ndata, fname, complain);
    POST;
}

void
trace_start_menu(
    void *vp,
    winid window,
    unsigned long mbehavior)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sstart_menu(%d, %lu)\n", INDENT,
            window, mbehavior);

    PRE;
    (*tdp->nprocs->win_start_menu)(tdp->ndata, window, mbehavior);
    POST;
}

void
trace_add_menu(
    void *vp,
    winid window,               /* window to use, must be of type NHW_MENU */
    const glyph_info *glyphinfo, /* glyph plus glyph info to display with item */
    const anything *identifier, /* what to return if selected */
    char ch,                    /* keyboard accelerator (0 = pick our own) */
    char gch,                   /* group accelerator (0 = no group) */
    int attr,                   /* attribute for string (like tty_putstr()) */
    int clr,                   /* color for string */
    const char *str,            /* menu string */
    unsigned int itemflags)     /* itemflags such as marked as selected */
{
    struct trace_data *tdp = vp;

    char buf_ch[10];
    char buf_gch[10];

    if (isprint(ch)) {
        sprintf(buf_ch, "'%c'(%d)", ch, ch);
    } else {
        sprintf(buf_ch, "(%d)", ch);
    }

    if (isprint(gch)) {
        sprintf(buf_gch, "'%c'(%d)", gch, gch);
    } else {
        sprintf(buf_gch, "(%d)", gch);
    }

    if (str) {
        fprintf(wc_tracelogf,
                "%sadd_menu(%d, %d, %u, %p, %s, %s, %d, '%s'(%d), %u)\n", INDENT,
                window, glyphinfo->glyph, glyphinfo->gm.glyphflags, (void *) identifier,
                buf_ch, buf_gch, attr, str, (int) strlen(str), itemflags);
    } else {
        fprintf(wc_tracelogf,
                "%sadd_menu(%d, %d, %u, %p, %s, %s, %d, NULL, %u)\n", INDENT,
                window, glyphinfo->glyph, glyphinfo->gm.glyphflags, (void *) identifier,
                buf_ch, buf_gch, attr, itemflags);
    }

    PRE;
    (*tdp->nprocs->win_add_menu)(tdp->ndata, window, glyphinfo,
                                 identifier,ch, gch, attr, clr, str, itemflags);
    POST;
}

void
trace_end_menu(
    void *vp,
    winid window,
    const char *prompt)
{
    struct trace_data *tdp = vp;

    if (prompt) {
        fprintf(wc_tracelogf, "%send_menu(%d, '%s'(%d))\n", INDENT, window,
                prompt, (int) strlen(prompt));
    } else {
        fprintf(wc_tracelogf, "%send_menu(%d, NULL)\n", INDENT, window);
    }

    PRE;
    (*tdp->nprocs->win_end_menu)(tdp->ndata, window, prompt);
    POST;
}

int
trace_select_menu(
    void *vp,
    winid window,
    int how,
    menu_item **menu_list)
{
    struct trace_data *tdp = vp;
    int rv;

    fprintf(wc_tracelogf, "%sselect_menu(%d, %d, %p)\n", INDENT, window, how,
            (void *) menu_list);

    PRE;
    rv = (*tdp->nprocs->win_select_menu)(tdp->ndata, window, how,
                                         (void *) menu_list);
    POST;

    fprintf(wc_tracelogf, "%s=> %d\n", INDENT, rv);
    return rv;
}

char
trace_message_menu(
    void *vp,
    char let,
    int how,
    const char *mesg)
{
    struct trace_data *tdp = vp;
    char buf_let[10];
    char rv;

    if (isprint(let)) {
        sprintf(buf_let, "'%c'(%d)", let, let);
    } else {
        sprintf(buf_let, "(%d)", let);
    }

    if (mesg) {
        fprintf(wc_tracelogf, "%smessage_menu(%s, %d, '%s'(%d))\n", INDENT,
                buf_let, how, mesg, (int) strlen(mesg));
    } else {
        fprintf(wc_tracelogf, "%smessage_menu(%s, %d, NULL)\n", INDENT,
                buf_let, how);
    }

    PRE;
    rv = (*tdp->nprocs->win_message_menu)(tdp->ndata, let, how, mesg);
    POST;

    if (isprint(rv)) {
        sprintf(buf_let, "'%c'(%d)", rv, rv);
    } else {
        sprintf(buf_let, "(%d)", rv);
    }
    fprintf(wc_tracelogf, "%s=> %s\n", INDENT, buf_let);

    return rv;
}

void
trace_update_inventory(void *vp, int arg)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%supdate_inventory(%d)\n", INDENT, arg);

    PRE;
    (*tdp->nprocs->win_update_inventory)(tdp->ndata, arg);
    POST;
}

void
trace_mark_synch(void *vp)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%smark_synch()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_mark_synch)(tdp->ndata);
    POST;
}

void
trace_wait_synch(void *vp)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%swait_synch()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_wait_synch)(tdp->ndata);
    POST;
}

#ifdef CLIPPING
void
trace_cliparound(
    void *vp,
    int x,
    int y)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%scliparound(%d, %d)\n", INDENT, x, y);

    PRE;
    (*tdp->nprocs->win_cliparound)(tdp->ndata, x, y);
    POST;
}
#endif

#ifdef POSITIONBAR
void
trace_update_positionbar(
    void *vp,
    char *posbar)
{
    struct trace_data *tdp = vp;

    if (posbar) {
        fprintf(wc_tracelogf, "%supdate_positionbar('%s'(%d))\n", INDENT,
                posbar, (int) strlen(posbar));
    } else {
        fprintf(wc_tracelogf, "%supdate_positionbar(NULL)\n", INDENT);
    }
    PRE;
    (*tdp->nprocs->win_update_positionbar)(tdp->ndata, posbar);
    POST;
}
#endif

/* XXX can we decode the glyph in a meaningful way? see map_glyphinfo()?
 genl_putmixed?  */
void
trace_print_glyph(
    void *vp,
    winid window,
    coordxy x,
    coordxy y,
    const glyph_info *glyphinfo,
    const glyph_info *bkglyphinfo)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sprint_glyph(%d, %d, %d, %d, %d)\n", INDENT, window,
            x, y, glyphinfo->glyph, bkglyphinfo->glyph);

    PRE;
    (*tdp->nprocs->win_print_glyph)(tdp->ndata, window, x, y, glyphinfo, bkglyphinfo);
    POST;
}

void
trace_raw_print(
    void *vp,
    const char *str)
{
    struct trace_data *tdp = vp;

    if (str) {
        fprintf(wc_tracelogf, "%sraw_print('%s'(%d))\n", INDENT, str,
                (int) strlen(str));
    } else {
        fprintf(wc_tracelogf, "%sraw_print(NULL)\n", INDENT);
    }

    PRE;
    (*tdp->nprocs->win_raw_print)(tdp->ndata, str);
    POST;
}

void
trace_raw_print_bold(
    void *vp,
    const char *str)
{
    struct trace_data *tdp = vp;

    if (str) {
        fprintf(wc_tracelogf, "%sraw_print_bold('%s'(%d))\n", INDENT, str,
                (int) strlen(str));
    } else {
        fprintf(wc_tracelogf, "%sraw_print_bold(NULL)\n", INDENT);
    }

    PRE;
    (*tdp->nprocs->win_raw_print_bold)(tdp->ndata, str);
    POST;
}

int
trace_nhgetch(void *vp)
{
    struct trace_data *tdp = vp;
    int rv;
    char buf[10];

    fprintf(wc_tracelogf, "%snhgetch()\n", INDENT);

    PRE;
    rv = (*tdp->nprocs->win_nhgetch)(tdp->ndata);
    POST;

    if (rv > 0 && rv < 256 && isprint(rv)) {
        sprintf(buf, "'%c'(%d)", rv, rv);
    } else {
        sprintf(buf, "(%d)", rv);
    }
    fprintf(wc_tracelogf, "%s=> %s\n", INDENT, buf);

    return rv;
}

int
trace_nh_poskey(
    void *vp,
    coordxy *x,
    coordxy *y,
    int *mod)
{
    struct trace_data *tdp = vp;
    int rv;
    char buf[10];

    fprintf(wc_tracelogf, "%snh_poskey(%d, %d, %d)\n", INDENT, *x, *y, *mod);

    PRE;
    rv = (*tdp->nprocs->win_nh_poskey)(tdp->ndata, x, y, mod);
    POST;
    if (rv > 0 && rv < 256 && isprint(rv)) {
        sprintf(buf, "'%c'(%d)", rv, rv);
    } else {
        sprintf(buf, "(%d)", rv);
    }
    fprintf(wc_tracelogf, "%s=> %s (%d, %d, %d)\n", INDENT, buf,
            (int) *x, (int) *y,
            *mod);

    return rv;
}

void
trace_nhbell(void *vp)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%snhbell()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_nhbell)(tdp->ndata);
    POST;
}

int
trace_doprev_message(void *vp)
{
    struct trace_data *tdp = vp;
    int rv;

    fprintf(wc_tracelogf, "%sdoprev_message()\n", INDENT);

    PRE;
    rv = (*tdp->nprocs->win_doprev_message)(tdp->ndata);
    POST;

    fprintf(wc_tracelogf, "%s=> %d\n", INDENT, rv);

    return rv;
}

char
trace_yn_function(
    void *vp,
    const char *query,
    const char *resp,
    char def)
{
    struct trace_data *tdp = vp;
    char rv;
    char buf[10];

    if (query) {
        fprintf(wc_tracelogf, "%syn_function('%s'(%d), ", INDENT, query,
                (int) strlen(query));
    } else {
        fprintf(wc_tracelogf, "%syn_function(NULL, ", INDENT);
    }

    if (resp) {
        fprintf(wc_tracelogf, "'%s'(%d), ", resp, (int) strlen(resp));
    } else {
        fprintf(wc_tracelogf, "NULL, ");
    }

    if (isprint(def)) {
        sprintf(buf, "'%c'(%d)", def, def);
    } else {
        sprintf(buf, "(%d)", def);
    }

    fprintf(wc_tracelogf, "%s)\n", buf);

    PRE;
    rv = (*tdp->nprocs->win_yn_function)(tdp->ndata, query, resp, def);
    POST;

    if (isprint(rv)) {
        sprintf(buf, "'%c'(%d)", rv, rv);
    } else {
        sprintf(buf, "(%d)", rv);
    }

    fprintf(wc_tracelogf, "%s=> %s\n", INDENT, buf);

    return rv;
}

void
trace_getlin(
    void *vp,
    const char *query,
    char *bufp)
{
    struct trace_data *tdp = vp;

    if (query) {
        fprintf(wc_tracelogf, "%sgetlin('%s'(%d), ", INDENT, query,
                (int) strlen(query));
    } else {
        fprintf(wc_tracelogf, "%sgetlin(NULL, ", INDENT);
    }

    if (bufp) {
        fprintf(wc_tracelogf, "%s)\n", fmt_ptr((genericptr_t) bufp));
    } else {
        fprintf(wc_tracelogf, "NULL)\n");
    }

    PRE;
    (*tdp->nprocs->win_getlin)(tdp->ndata, query, bufp);
    POST;
}

int
trace_get_ext_cmd(void *vp)
{
    struct trace_data *tdp = vp;
    int rv;
    int ecl_size = 0;

    /* this is ugly, but the size isn't exposed */
    const struct ext_func_tab *efp;
    for (efp = extcmdlist; efp->ef_txt; efp++)
        ecl_size++;

    fprintf(wc_tracelogf, "%sget_ext_cmd()\n", INDENT);

    PRE;
    rv = (*tdp->nprocs->win_get_ext_cmd)(tdp->ndata);
    POST;

    if (rv < 0 || rv >= ecl_size) {
        fprintf(wc_tracelogf, "%s=> (%d)\n", INDENT, rv);
    } else {
        fprintf(wc_tracelogf, "%s=> %d/%s\n", INDENT, rv,
                extcmdlist[rv].ef_txt);
    }

    return rv;
}

void
trace_number_pad(
    void *vp,
    int state)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%snumber_pad(%d)\n", INDENT, state);

    PRE;
    (*tdp->nprocs->win_number_pad)(tdp->ndata, state);
    POST;
}

void
trace_delay_output(void *vp)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sdelay_output()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_delay_output)(tdp->ndata);
    POST;
}

#ifdef CHANGE_COLOR
void
trace_change_color(
    void *vp,
    int color,
    long value,
    int reverse)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%schange_color(%d, $%lx, %d)\n", INDENT, color,
            value, reverse);

    PRE;
    (*tdp->nprocs->win_change_color)(tdp->ndata, color, value, reverse);
    POST;
}

#ifdef MAC
void
trace_change_background(
    void *vp,
    int bw)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%schange_background(%d)\n", INDENT, bw);

    PRE;
    (*tdp->nprocs->win_change_background)(tdp->ndata, bw);
    POST;
}

short
trace_set_font_name(
    void *vp,
    winid window,
    char *font)
{
    struct trace_data *tdp = vp;
    short rv;

    if (font) {
        fprintf(wc_tracelogf, "%sset_font_name(%d, '%s'(%d))\n", INDENT,
                window, font, (int) (strlen(font)));
    } else {
        fprintf(wc_tracelogf, "%sset_font_name(%d, NULL)\n", INDENT, window);
    }

    PRE;
    rv = (*tdp->nprocs->win_set_font_name)(tdp->ndata, window, font);
    POST;

    fprintf(wc_tracelogf, "%s=> %d\n", INDENT, rv);

    return rv;
}
#endif

char *
trace_get_color_string(void *vp)
{
    struct trace_data *tdp = vp;
    char *rv;

    fprintf(wc_tracelogf, "%sget_color_string()\n", INDENT);

    PRE;
    rv = (*tdp->nprocs->win_get_color_string)(tdp->ndata);
    POST;

    if (rv) {
        fprintf(wc_tracelogf, "%s=> '%s'(%d)\n", INDENT, rv,
                (int) strlen(rv));
    } else {
        fprintf(wc_tracelogf, "%s=> NULL\n", INDENT);
    }

    return rv;
}

#endif

/* other defs that really should go away (they're tty specific) */
void
trace_start_screen(void *vp)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sstart_screen()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_start_screen)(tdp->ndata);
    POST;
}

void
trace_end_screen(void *vp)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%send_screen()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_end_screen)(tdp->ndata);
    POST;
}

void
trace_outrip(
    void *vp,
    winid tmpwin,
    int how,
    time_t when)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%soutrip(%d, %d, %ld)\n", INDENT, (int) tmpwin,
            how, (long) when);

    PRE;
    (*tdp->nprocs->win_outrip)(tdp->ndata, tmpwin, how, when);
    POST;
}

void
trace_preference_update(
    void *vp,
    const char *pref)
{
    struct trace_data *tdp = vp;

    if (pref) {
        fprintf(wc_tracelogf, "%spreference_update('%s'(%d))\n", INDENT, pref,
                (int) strlen(pref));
    } else {
        fprintf(wc_tracelogf, "%spreference_update(NULL)\n", INDENT);
    }

    PRE;
    (*tdp->nprocs->win_preference_update)(tdp->ndata, pref);
    POST;
}

char *
trace_getmsghistory(
    void *vp,
    boolean init)
{
    struct trace_data *tdp = vp;
    char *rv;

    fprintf(wc_tracelogf, "%sgetmsghistory(%d)\n", INDENT, init);

    PRE;
    rv = (*tdp->nprocs->win_getmsghistory)(tdp->ndata, init);
    POST;

    if (rv) {
        fprintf(wc_tracelogf, "%s=> '%s'(%d)\n", INDENT, rv,
                (int) strlen(rv));
    } else {
        fprintf(wc_tracelogf, "%s=> NULL\n", INDENT);
    }

    return rv;
}

void
trace_putmsghistory(
    void *vp,
    const char *msg,
    boolean is_restoring)
{
    struct trace_data *tdp = vp;

    if (msg) {
        fprintf(wc_tracelogf, "%sputmsghistory('%s'(%d), %d)\n", INDENT, msg,
                (int) strlen(msg), is_restoring);
    } else {
        fprintf(wc_tracelogf, "%sputmghistory(NULL, %d)\n", INDENT,
                is_restoring);
    }

    PRE;
    (*tdp->nprocs->win_putmsghistory)(tdp->ndata, msg, is_restoring);
    POST;
}

void
trace_status_init(void *vp)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sstatus_init()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_status_init)(tdp->ndata);
    POST;
}

void
trace_status_finish(void *vp)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sstatus_finish()\n", INDENT);

    PRE;
    (*tdp->nprocs->win_status_finish)(tdp->ndata);
    POST;
}

void
trace_status_enablefield(
    void *vp,
    int fieldidx,
    const char *nm,
    const char *fmt,
    boolean enable)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sstatus_enablefield(%d, ", INDENT, fieldidx);
    if (nm) {
        fprintf(wc_tracelogf, "'%s'(%d), ", nm, (int) strlen(nm));
    } else {
        fprintf(wc_tracelogf, "NULL, ");
    }
    if (fmt) {
        fprintf(wc_tracelogf, "'%s'(%d), ", fmt, (int) strlen(fmt));
    } else {
        fprintf(wc_tracelogf, "NULL, ");
    }
    fprintf(wc_tracelogf, "%d)\n", enable);

    PRE;
    (*tdp->nprocs->win_status_enablefield)(tdp->ndata, fieldidx, nm, fmt,
                                           enable);
    POST;
}

void
trace_status_update(
    void *vp,
    int idx,
    genericptr_t ptr,
    int chg, 
    int percent,
    int color,
    unsigned long *colormasks)
{
    struct trace_data *tdp = vp;

    fprintf(wc_tracelogf, "%sstatus_update(%d, %p, %d, %d)\n", INDENT, idx,
            ptr, chg, percent);

    PRE;
    (*tdp->nprocs->win_status_update)(tdp->ndata, idx, ptr, chg, percent,
                                      color, colormasks);
    POST;
}

boolean
trace_can_suspend(void *vp)
{
    struct trace_data *tdp = vp;
    boolean rv;

    fprintf(wc_tracelogf, "%scan_suspend()\n", INDENT);

    PRE;
    rv = (*tdp->nprocs->win_can_suspend)(tdp->ndata);
    POST;

    fprintf(wc_tracelogf, "%s=> %d\n", INDENT, rv);

    return rv;
}

struct chain_procs trace_procs = {
    "+trace", 0, /* wincap */
    0,           /* wincap2 */
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, /* color availability */
    /*
    XXX problem - the above need to come from the real window port, possibly
    modified.  May need to do something to call an additional init fn later
    or if this is the only place like this the choose_windows fn can do the
    fixup
    (but not if the value can be modified by the stack?)  TBD
    */
    trace_init_nhwindows,
    trace_player_selection, trace_askname, trace_get_nh_event,
    trace_exit_nhwindows, trace_suspend_nhwindows, trace_resume_nhwindows,
    trace_create_nhwindow, trace_clear_nhwindow, trace_display_nhwindow,
    trace_destroy_nhwindow, trace_curs, trace_putstr, trace_putmixed,
    trace_display_file, trace_start_menu, trace_add_menu, trace_end_menu,
    trace_select_menu, trace_message_menu, trace_update_inventory,
    trace_mark_synch, trace_wait_synch,
#ifdef CLIPPING
    trace_cliparound,
#endif
#ifdef POSITIONBAR
    trace_update_positionbar,
#endif
    trace_print_glyph, trace_raw_print, trace_raw_print_bold, trace_nhgetch,
    trace_nh_poskey, trace_nhbell, trace_doprev_message, trace_yn_function,
    trace_getlin, trace_get_ext_cmd, trace_number_pad, trace_delay_output,
#ifdef CHANGE_COLOR
    trace_change_color,
#ifdef MAC
    trace_change_background, trace_set_font_name,
#endif
    trace_get_color_string,
#endif

    trace_start_screen, trace_end_screen,

    trace_outrip, trace_preference_update, trace_getmsghistory,
    trace_putmsghistory,
    trace_status_init, trace_status_finish, trace_status_enablefield,
    trace_status_update,
    trace_can_suspend,
};

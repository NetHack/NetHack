/* NetHack 3.6	windows.c	$NHDT-Date: 1448013599 2015/11/20 09:59:59 $  $NHDT-Branch: master $:$NHDT-Revision: 1.35 $ */
/* Copyright (c) D. Cohrs, 1993. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#ifdef TTY_GRAPHICS
#include "wintty.h"
#endif
#ifdef X11_GRAPHICS
/* Cannot just blindly include winX.h without including all of X11 stuff
   and must get the order of include files right.  Don't bother. */
extern struct window_procs X11_procs;
extern void FDECL(win_X11_init, (int));
#endif
#ifdef QT_GRAPHICS
extern struct window_procs Qt_procs;
#endif
#ifdef GEM_GRAPHICS
#include "wingem.h"
#endif
#ifdef MAC
extern struct window_procs mac_procs;
#endif
#ifdef BEOS_GRAPHICS
extern struct window_procs beos_procs;
extern void FDECL(be_win_init, (int));
FAIL /* be_win_init doesn't exist? XXX*/
#endif
#ifdef AMIGA_INTUITION
extern struct window_procs amii_procs;
extern struct window_procs amiv_procs;
extern void FDECL(ami_wininit_data, (int));
#endif
#ifdef WIN32_GRAPHICS
extern struct window_procs win32_procs;
#endif
#ifdef GNOME_GRAPHICS
#include "winGnome.h"
extern struct window_procs Gnome_procs;
#endif
#ifdef MSWIN_GRAPHICS
extern struct window_procs mswin_procs;
#endif
#ifdef WINCHAIN
extern struct window_procs chainin_procs;
extern void FDECL(chainin_procs_init, (int));
extern void *FDECL(chainin_procs_chain, (int, int, void *, void *, void *));

extern struct chain_procs chainout_procs;
extern void FDECL(chainout_procs_init, (int));
extern void *FDECL(chainout_procs_chain, (int, int, void *, void *, void *));

extern struct chain_procs trace_procs;
extern void FDECL(trace_procs_init, (int));
extern void *FDECL(trace_procs_chain, (int, int, void *, void *, void *));
#endif

STATIC_DCL void FDECL(def_raw_print, (const char *s));

#ifdef HANGUPHANDLING
volatile
#endif
    NEARDATA struct window_procs windowprocs;

#ifdef WINCHAIN
#define CHAINR(x) , x
#else
#define CHAINR(x)
#endif

static struct win_choices {
    struct window_procs *procs;
    void FDECL((*ini_routine), (int)); /* optional (can be 0) */
#ifdef WINCHAIN
    void *FDECL((*chain_routine), (int, int, void *, void *, void *));
#endif
} winchoices[] = {
#ifdef TTY_GRAPHICS
    { &tty_procs, win_tty_init CHAINR(0) },
#endif
#ifdef X11_GRAPHICS
    { &X11_procs, win_X11_init CHAINR(0) },
#endif
#ifdef QT_GRAPHICS
    { &Qt_procs, 0 CHAINR(0) },
#endif
#ifdef GEM_GRAPHICS
    { &Gem_procs, win_Gem_init CHAINR(0) },
#endif
#ifdef MAC
    { &mac_procs, 0 CHAINR(0) },
#endif
#ifdef BEOS_GRAPHICS
    { &beos_procs, be_win_init CHAINR(0) },
#endif
#ifdef AMIGA_INTUITION
    { &amii_procs,
      ami_wininit_data CHAINR(0) }, /* Old font version of the game */
    { &amiv_procs,
      ami_wininit_data CHAINR(0) }, /* Tile version of the game */
#endif
#ifdef WIN32_GRAPHICS
    { &win32_procs, 0 CHAINR(0) },
#endif
#ifdef GNOME_GRAPHICS
    { &Gnome_procs, 0 CHAINR(0) },
#endif
#ifdef MSWIN_GRAPHICS
    { &mswin_procs, 0 CHAINR(0) },
#endif
#ifdef WINCHAIN
    { &chainin_procs, chainin_procs_init, chainin_procs_chain },
    { (struct window_procs *) &chainout_procs, chainout_procs_init,
      chainout_procs_chain },

    { (struct window_procs *) &trace_procs, trace_procs_init,
      trace_procs_chain },
#endif
    { 0, 0 CHAINR(0) } /* must be last */
};

#ifdef WINCHAIN
struct winlink {
    struct winlink *nextlink;
    struct win_choices *wincp;
    void *linkdata;
};
/* NB: this chain does not contain the terminal real window system pointer */

static struct winlink *chain = 0;

static struct winlink *
wl_new()
{
    return calloc(1, sizeof(struct winlink));
}
static void
wl_addhead(struct winlink *wl)
{
    wl->nextlink = chain;
    chain = wl;
}
static void
wl_addtail(struct winlink *wl)
{
    struct winlink *p = chain;

    if (!chain) {
        chain = wl;
        return;
    }
    while (p->nextlink) {
        p = p->nextlink;
    }
    p->nextlink = wl;
    return;
}
#endif /* WINCHAIN */

static struct win_choices *last_winchoice = 0;

boolean
genl_can_suspend_no(VOID_ARGS)
{
    return FALSE;
}

boolean
genl_can_suspend_yes(VOID_ARGS)
{
    return TRUE;
}

STATIC_OVL
void
def_raw_print(s)
const char *s;
{
    puts(s);
}

#ifdef WINCHAIN
static struct win_choices *
win_choices_find(s)
const char *s;
{
    register int i;

    for (i = 0; winchoices[i].procs; i++) {
        if (!strcmpi(s, winchoices[i].procs->name)) {
            return &winchoices[i];
        }
    }
    return (struct win_choices *) 0;
}
#endif

void
choose_windows(s)
const char *s;
{
    register int i;

    for (i = 0; winchoices[i].procs; i++) {
        if ('+' == winchoices[i].procs->name[0])
            continue;
        if ('-' == winchoices[i].procs->name[0])
            continue;
        if (!strcmpi(s, winchoices[i].procs->name)) {
            windowprocs = *winchoices[i].procs;

            if (last_winchoice && last_winchoice->ini_routine)
                (*last_winchoice->ini_routine)(WININIT_UNDO);
            if (winchoices[i].ini_routine)
                (*winchoices[i].ini_routine)(WININIT);
            last_winchoice = &winchoices[i];
            return;
        }
    }

    if (!windowprocs.win_raw_print)
        windowprocs.win_raw_print = def_raw_print;

    if (!winchoices[0].procs) {
        raw_printf("No window types?");
        exit(EXIT_FAILURE);
    }
    if (!winchoices[1].procs) {
        raw_printf("Window type %s not recognized.  The only choice is: %s.",
                   s, winchoices[0].procs->name);
    } else {
        raw_printf("Window type %s not recognized.  Choices are:", s);
        for (i = 0; winchoices[i].procs; i++) {
            if ('+' == winchoices[i].procs->name[0])
                continue;
            if ('-' == winchoices[i].procs->name[0])
                continue;
            raw_printf("        %s", winchoices[i].procs->name);
        }
    }

    if (windowprocs.win_raw_print == def_raw_print)
        terminate(EXIT_SUCCESS);
    wait_synch();
}

#ifdef WINCHAIN
void
addto_windowchain(s)
const char *s;
{
    register int i;

    for (i = 0; winchoices[i].procs; i++) {
        if ('+' != winchoices[i].procs->name[0])
            continue;
        if (!strcmpi(s, winchoices[i].procs->name)) {
            struct winlink *p = wl_new();
            p->wincp = &winchoices[i];
            wl_addtail(p);
            /* NB: The ini_routine() will be called during commit. */
            return;
        }
    }

    windowprocs.win_raw_print = def_raw_print;

    raw_printf("Window processor %s not recognized.  Choices are:", s);
    for (i = 0; winchoices[i].procs; i++) {
        if ('+' != winchoices[i].procs->name[0])
            continue;
        raw_printf("        %s", winchoices[i].procs->name);
    }

    exit(EXIT_FAILURE);
}

void
commit_windowchain()
{
    struct winlink *p;
    int n;
    int wincap, wincap2;

    if (!chain)
        return;

    /* Save wincap* from the real window system - we'll restore it below. */
    wincap = windowprocs.wincap;
    wincap2 = windowprocs.wincap2;

    /* add -chainin at head and -chainout at tail */
    p = wl_new();
    p->wincp = win_choices_find("-chainin");
    if (!p->wincp) {
        raw_printf("Can't locate processor '-chainin'");
        exit(EXIT_FAILURE);
    }
    wl_addhead(p);

    p = wl_new();
    p->wincp = win_choices_find("-chainout");
    if (!p->wincp) {
        raw_printf("Can't locate processor '-chainout'");
        exit(EXIT_FAILURE);
    }
    wl_addtail(p);

    /* Now alloc() init() similar to Objective-C. */
    for (n = 1, p = chain; p; n++, p = p->nextlink) {
        p->linkdata = (*p->wincp->chain_routine)(WINCHAIN_ALLOC, n, 0, 0, 0);
    }

    for (n = 1, p = chain; p; n++, p = p->nextlink) {
        if (p->nextlink) {
            (void) (*p->wincp->chain_routine)(WINCHAIN_INIT, n, p->linkdata,
                                              p->nextlink->wincp->procs,
                                              p->nextlink->linkdata);
        } else {
            (void) (*p->wincp->chain_routine)(WINCHAIN_INIT, n, p->linkdata,
                                              last_winchoice->procs, 0);
        }
    }

    /* Restore the saved wincap* values.  We do it here to give the
     * ini_routine()s a chance to change or check them. */
    chain->wincp->procs->wincap = wincap;
    chain->wincp->procs->wincap2 = wincap2;

    /* Call the init procs.  Do not re-init the terminal real win. */
    p = chain;
    while (p->nextlink) {
        if (p->wincp->ini_routine) {
            (*p->wincp->ini_routine)(WININIT);
        }
        p = p->nextlink;
    }

    /* Install the chain into window procs very late so ini_routine()s
     * can raw_print on error. */
    windowprocs = *chain->wincp->procs;

    p = chain;
    while (p) {
        struct winlink *np = p->nextlink;
        free(p);
        p = np; /* assignment, not proof */
    }
}
#endif /* WINCHAIN */

/*
 * tty_message_menu() provides a means to get feedback from the
 * --More-- prompt; other interfaces generally don't need that.
 */
/*ARGSUSED*/
char
genl_message_menu(let, how, mesg)
char let UNUSED;
int how UNUSED;
const char *mesg;
{
    pline("%s", mesg);
    return 0;
}

/*ARGSUSED*/
void
genl_preference_update(pref)
const char *pref UNUSED;
{
    /* window ports are expected to provide
       their own preference update routine
       for the preference capabilities that
       they support.
       Just return in this genl one. */
    return;
}

char *
genl_getmsghistory(init)
boolean init UNUSED;
{
    /* window ports can provide
       their own getmsghistory() routine to
       preserve message history between games.
       The routine is called repeatedly from
       the core save routine, and the window
       port is expected to successively return
       each message that it wants saved, starting
       with the oldest message first, finishing
       with the most recent.
       Return null pointer when finished.
     */
    return (char *) 0;
}

void
genl_putmsghistory(msg, is_restoring)
const char *msg;
boolean is_restoring;
{
    /* window ports can provide
       their own putmsghistory() routine to
       load message history from a saved game.
       The routine is called repeatedly from
       the core restore routine, starting with
       the oldest saved message first, and
       finishing with the latest.
       The window port routine is expected to
       load the message recall buffers in such
       a way that the ordering is preserved.
       The window port routine should make no
       assumptions about how many messages are
       forthcoming, nor should it assume that
       another message will follow this one,
       so it should keep all pointers/indexes
       intact at the end of each call.
    */

    /* this doesn't provide for reloading the message window with the
       previous session's messages upon restore, but it does put the quest
       message summary lines there by treating them as ordinary messages */
    if (!is_restoring)
        pline("%s", msg);
    return;
}

#ifdef HANGUPHANDLING
/*
 * Dummy windowing scheme used to replace current one with no-ops
 * in order to avoid all terminal I/O after hangup/disconnect.
 */

static int NDECL(hup_nhgetch);
static char FDECL(hup_yn_function, (const char *, const char *, CHAR_P));
static int FDECL(hup_nh_poskey, (int *, int *, int *));
static void FDECL(hup_getlin, (const char *, char *));
static void FDECL(hup_init_nhwindows, (int *, char **));
static void FDECL(hup_exit_nhwindows, (const char *));
static winid FDECL(hup_create_nhwindow, (int));
static int FDECL(hup_select_menu, (winid, int, MENU_ITEM_P **));
static void FDECL(hup_add_menu, (winid, int, const anything *, CHAR_P, CHAR_P,
                                 int, const char *, BOOLEAN_P));
static void FDECL(hup_end_menu, (winid, const char *));
static void FDECL(hup_putstr, (winid, int, const char *));
static void FDECL(hup_print_glyph, (winid, XCHAR_P, XCHAR_P, int, int));
static void FDECL(hup_outrip, (winid, int, time_t));
static void FDECL(hup_curs, (winid, int, int));
static void FDECL(hup_display_nhwindow, (winid, BOOLEAN_P));
static void FDECL(hup_display_file, (const char *, BOOLEAN_P));
#ifdef CLIPPING
static void FDECL(hup_cliparound, (int, int));
#endif
#ifdef CHANGE_COLOR
static void FDECL(hup_change_color, (int, long, int));
#ifdef MAC
static short FDECL(hup_set_font_name, (winid, char *));
#endif
static char *NDECL(hup_get_color_string);
#endif /* CHANGE_COLOR */
#ifdef STATUS_VIA_WINDOWPORT
static void FDECL(hup_status_update, (int, genericptr_t, int, int));
#endif

static int NDECL(hup_int_ndecl);
static void NDECL(hup_void_ndecl);
static void FDECL(hup_void_fdecl_int, (int));
static void FDECL(hup_void_fdecl_winid, (winid));
static void FDECL(hup_void_fdecl_constchar_p, (const char *));

static struct window_procs hup_procs = {
    "hup", 0L, 0L, hup_init_nhwindows,
    hup_void_ndecl,                                    /* player_selection */
    hup_void_ndecl,                                    /* askname */
    hup_void_ndecl,                                    /* get_nh_event */
    hup_exit_nhwindows, hup_void_fdecl_constchar_p,    /* suspend_nhwindows */
    hup_void_ndecl,                                    /* resume_nhwindows */
    hup_create_nhwindow, hup_void_fdecl_winid,         /* clear_nhwindow */
    hup_display_nhwindow, hup_void_fdecl_winid,        /* destroy_nhwindow */
    hup_curs, hup_putstr, hup_putstr,                  /* putmixed */
    hup_display_file, hup_void_fdecl_winid,            /* start_menu */
    hup_add_menu, hup_end_menu, hup_select_menu, genl_message_menu,
    hup_void_ndecl,                                    /* update_inventory */
    hup_void_ndecl,                                    /* mark_synch */
    hup_void_ndecl,                                    /* wait_synch */
#ifdef CLIPPING
    hup_cliparound,
#endif
#ifdef POSITIONBAR
    (void FDECL((*), (char *))) hup_void_fdecl_constchar_p,
                                                      /* update_positionbar */
#endif
    hup_print_glyph,
    hup_void_fdecl_constchar_p,                       /* raw_print */
    hup_void_fdecl_constchar_p,                       /* raw_print_bold */
    hup_nhgetch, hup_nh_poskey, hup_void_ndecl,       /* nhbell  */
    hup_int_ndecl,                                    /* doprev_message */
    hup_yn_function, hup_getlin, hup_int_ndecl,       /* get_ext_cmd */
    hup_void_fdecl_int,                               /* number_pad */
    hup_void_ndecl,                                   /* delay_output  */
#ifdef CHANGE_COLOR
    hup_change_color,
#ifdef MAC
    hup_void_fdecl_int,                               /* change_background */
    hup_set_font_name,
#endif
    hup_get_color_string,
#endif /* CHANGE_COLOR */
    hup_void_ndecl,                                   /* start_screen */
    hup_void_ndecl,                                   /* end_screen */
    hup_outrip, genl_preference_update, genl_getmsghistory,
    genl_putmsghistory,
#ifdef STATUS_VIA_WINDOWPORT
    hup_void_ndecl,                                   /* status_init */
    hup_void_ndecl,                                   /* status_finish */
    genl_status_enablefield, hup_status_update,
#ifdef STATUS_HILITES
    genl_status_threshold,
#endif
#endif /* STATUS_VIA_WINDOWPORT */
    genl_can_suspend_no,
};

static void FDECL((*previnterface_exit_nhwindows), (const char *)) = 0;

/* hangup has occurred; switch to no-op user interface */
void
nhwindows_hangup()
{
    char *FDECL((*previnterface_getmsghistory), (BOOLEAN_P)) = 0;

#ifdef ALTMETA
    /* command processor shouldn't look for 2nd char after seeing ESC */
    iflags.altmeta = FALSE;
#endif

    /* don't call exit_nhwindows() directly here; if a hangup occurs
       while interface code is executing, exit_nhwindows could knock
       the interface's active data structures out from under itself */
    if (iflags.window_inited
        && windowprocs.win_exit_nhwindows != hup_exit_nhwindows)
        previnterface_exit_nhwindows = windowprocs.win_exit_nhwindows;

    /* also, we have to leave the old interface's getmsghistory()
       in place because it will be called while saving the game */
    if (windowprocs.win_getmsghistory != hup_procs.win_getmsghistory)
        previnterface_getmsghistory = windowprocs.win_getmsghistory;

    windowprocs = hup_procs;

    if (previnterface_getmsghistory)
        windowprocs.win_getmsghistory = previnterface_getmsghistory;
}

static void
hup_exit_nhwindows(lastgasp)
const char *lastgasp;
{
    /* core has called exit_nhwindows(); call the previous interface's
       shutdown routine now; xxx_exit_nhwindows() needs to call other
       xxx_ routines directly rather than through windowprocs pointers */
    if (previnterface_exit_nhwindows) {
        lastgasp = 0; /* don't want exit routine to attempt extra output */
        (*previnterface_exit_nhwindows)(lastgasp);
        previnterface_exit_nhwindows = 0;
    }
    iflags.window_inited = 0;
}

static int
hup_nhgetch(VOID_ARGS)
{
    return '\033'; /* ESC */
}

/*ARGSUSED*/
static char
hup_yn_function(prompt, resp, deflt)
const char *prompt UNUSED, *resp UNUSED;
char deflt;
{
    if (!deflt)
        deflt = '\033';
    return deflt;
}

/*ARGSUSED*/
static int
hup_nh_poskey(x, y, mod)
int *x UNUSED, *y UNUSED, *mod UNUSED;
{
    return '\033';
}

/*ARGSUSED*/
static void
hup_getlin(prompt, outbuf)
const char *prompt UNUSED;
char *outbuf;
{
    Strcpy(outbuf, "\033");
}

/*ARGSUSED*/
static void
hup_init_nhwindows(argc_p, argv)
int *argc_p UNUSED;
char **argv UNUSED;
{
    iflags.window_inited = 1;
}

/*ARGUSED*/
static winid
hup_create_nhwindow(type)
int type UNUSED;
{
    return WIN_ERR;
}

/*ARGSUSED*/
static int
hup_select_menu(window, how, menu_list)
winid window UNUSED;
int how UNUSED;
struct mi **menu_list UNUSED;
{
    return -1;
}

/*ARGSUSED*/
static void
hup_add_menu(window, glyph, identifier, sel, grpsel, attr, txt, preselected)
winid window UNUSED;
int glyph UNUSED, attr UNUSED;
const anything *identifier UNUSED;
char sel UNUSED, grpsel UNUSED;
const char *txt UNUSED;
boolean preselected UNUSED;
{
    return;
}

/*ARGSUSED*/
static void
hup_end_menu(window, prompt)
winid window UNUSED;
const char *prompt UNUSED;
{
    return;
}

/*ARGSUSED*/
static void
hup_putstr(window, attr, text)
winid window UNUSED;
int attr UNUSED;
const char *text UNUSED;
{
    return;
}

/*ARGSUSED*/
static void
hup_print_glyph(window, x, y, glyph, bkglyph)
winid window UNUSED;
xchar x UNUSED, y UNUSED;
int glyph UNUSED;
int bkglyph UNUSED;
{
    return;
}

/*ARGSUSED*/
static void
hup_outrip(tmpwin, how, when)
winid tmpwin UNUSED;
int how UNUSED;
time_t when UNUSED;
{
    return;
}

/*ARGSUSED*/
static void
hup_curs(window, x, y)
winid window UNUSED;
int x UNUSED, y UNUSED;
{
    return;
}

/*ARGSUSED*/
static void
hup_display_nhwindow(window, blocking)
winid window UNUSED;
boolean blocking UNUSED;
{
    return;
}

/*ARGSUSED*/
static void
hup_display_file(fname, complain)
const char *fname UNUSED;
boolean complain UNUSED;
{
    return;
}

#ifdef CLIPPING
/*ARGSUSED*/
static void
hup_cliparound(x, y)
int x UNUSED, y UNUSED;
{
    return;
}
#endif

#ifdef CHANGE_COLOR
/*ARGSUSED*/
static void
hup_change_color(color, rgb, reverse)
int color, reverse;
long rgb;
{
    return;
}

#ifdef MAC
/*ARGSUSED*/
static short
hup_set_font_name(window, fontname)
winid window;
char *fontname;
{
    return 0;
}
#endif /* MAC */

static char *
hup_get_color_string(VOID_ARGS)
{
    return (char *) 0;
}
#endif /* CHANGE_COLOR */

#ifdef STATUS_VIA_WINDOWPORT
/*ARGSUSED*/
static void
hup_status_update(idx, ptr, chg, percent)
int idx UNUSED, chg UNUSED, percent UNUSED;
genericptr_t ptr UNUSED;
{
    return;
}
#endif /* STATUS_VIA_WINDOWPORT */

/*
 * Non-specific stubs.
 */

static int
hup_int_ndecl(VOID_ARGS)
{
    return -1;
}

static void
hup_void_ndecl(VOID_ARGS)
{
    return;
}

/*ARGUSED*/
static void
hup_void_fdecl_int(arg)
int arg UNUSED;
{
    return;
}

/*ARGUSED*/
static void
hup_void_fdecl_winid(window)
winid window UNUSED;
{
    return;
}

/*ARGUSED*/
static void
hup_void_fdecl_constchar_p(string)
const char *string UNUSED;
{
    return;
}

#endif /* HANGUPHANDLING */

#ifdef STATUS_VIA_WINDOWPORT

/****************************************************************************/
/* genl backward compat stuff                                               */
/****************************************************************************/

const char *status_fieldnm[MAXBLSTATS];
const char *status_fieldfmt[MAXBLSTATS];
char *status_vals[MAXBLSTATS];
boolean status_activefields[MAXBLSTATS];
NEARDATA winid WIN_STATUS;

void
genl_status_init()
{
    int i;
    for (i = 0; i < MAXBLSTATS; ++i) {
        status_vals[i] = (char *) alloc(MAXCO);
        *status_vals[i] = '\0';
        status_activefields[i] = FALSE;
        status_fieldfmt[i] = (const char *) 0;
    }
    /* Use a window for the genl version; backward port compatibility */
    WIN_STATUS = create_nhwindow(NHW_STATUS);
    display_nhwindow(WIN_STATUS, FALSE);
}

void
genl_status_finish()
{
    /* tear down routine */
    int i;

    /* free alloc'd memory here */
    for (i = 0; i < MAXBLSTATS; ++i) {
        if (status_vals[i])
            free((genericptr_t) status_vals[i]);
        status_vals[i] = (char *) 0;
    }
}

void
genl_status_enablefield(fieldidx, nm, fmt, enable)
int fieldidx;
const char *nm;
const char *fmt;
boolean enable;
{
    status_fieldfmt[fieldidx] = fmt;
    status_fieldnm[fieldidx] = nm;
    status_activefields[fieldidx] = enable;
}

void
genl_status_update(idx, ptr, chg, percent)
int idx, chg, percent;
genericptr_t ptr;
{
    char newbot1[MAXCO], newbot2[MAXCO];
    long cond, *condptr = (long *) ptr;
    register int i;
    char *text = (char *) ptr;

    enum statusfields fieldorder[2][15] = {
        { BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_ALIGN,
          BL_SCORE, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH,
          BL_FLUSH },
        { BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX,
          BL_AC, BL_XP, BL_EXP, BL_HD, BL_TIME, BL_HUNGER,
          BL_CAP, BL_CONDITION, BL_FLUSH }
    };

    if (idx != BL_FLUSH) {
        if (!status_activefields[idx])
            return;
        switch (idx) {
        case BL_CONDITION:
            cond = *condptr;
            *status_vals[idx] = '\0';
            if (cond & BL_MASK_BLIND)
                Strcat(status_vals[idx], " Blind");
            if (cond & BL_MASK_CONF)
                Strcat(status_vals[idx], " Conf");
            if (cond & BL_MASK_FOODPOIS)
                Strcat(status_vals[idx], " FoodPois");
            if (cond & BL_MASK_ILL)
                Strcat(status_vals[idx], " Ill");
            if (cond & BL_MASK_STUNNED)
                Strcat(status_vals[idx], " Stun");
            if (cond & BL_MASK_HALLU)
                Strcat(status_vals[idx], " Hallu");
            if (cond & BL_MASK_SLIMED)
                Strcat(status_vals[idx], " Slime");
            break;
        default:
            Sprintf(status_vals[idx],
                    status_fieldfmt[idx] ? status_fieldfmt[idx] : "%s", text);
            break;
        }
    }

    /* This genl version updates everything on the display, everytime */
    newbot1[0] = '\0';
    for (i = 0; fieldorder[0][i] != BL_FLUSH; ++i) {
        int idx1 = fieldorder[0][i];
        if (status_activefields[idx1])
            Strcat(newbot1, status_vals[idx1]);
    }
    newbot2[0] = '\0';
    for (i = 0; fieldorder[1][i] != BL_FLUSH; ++i) {
        int idx2 = fieldorder[1][i];
        if (status_activefields[idx2])
            Strcat(newbot2, status_vals[idx2]);
    }
    curs(WIN_STATUS, 1, 0);
    putstr(WIN_STATUS, 0, newbot1);
    curs(WIN_STATUS, 1, 1);
    putmixed(WIN_STATUS, 0, newbot2); /* putmixed() due to GOLD glyph */
}

#ifdef STATUS_HILITES
void
genl_status_threshold(fldidx, thresholdtype, threshold, behavior, under, over)
int fldidx, thresholdtype;
int behavior, under, over;
anything threshold;
{
    return;
}
#endif /* STATUS_HILITES */
#endif /* STATUS_VIA_WINDOWPORT */

/*windows.c*/

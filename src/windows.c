/* NetHack 3.7	windows.c	$NHDT-Date: 1661202202 2022/08/22 21:03:22 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.97 $ */
/* Copyright (c) D. Cohrs, 1993. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"
#ifdef TTY_GRAPHICS
#include "wintty.h"
#endif
#ifdef CURSES_GRAPHICS
extern struct window_procs curses_procs;
#endif
#ifdef X11_GRAPHICS
/* Cannot just blindly include winX.h without including all of X11 stuff
   and must get the order of include files right.  Don't bother. */
extern struct window_procs X11_procs;
extern void win_X11_init(int);
#endif
#ifdef QT_GRAPHICS
extern struct window_procs Qt_procs;
#endif
#ifdef GEM_GRAPHICS
/*#include "wingem.h"*/
#endif
#ifdef MAC
extern struct window_procs mac_procs;
#endif
#ifdef BEOS_GRAPHICS
extern struct window_procs beos_procs;
extern void be_win_init(int);
FAIL /* be_win_init doesn't exist? XXX*/
#endif
#ifdef AMIGA_INTUITION
extern struct window_procs amii_procs;
extern struct window_procs amiv_procs;
extern void ami_wininit_data(int);
#endif
#ifdef GNOME_GRAPHICS
/*#include "winGnome.h"*/
extern struct window_procs Gnome_procs;
#endif
#ifdef MSWIN_GRAPHICS
extern struct window_procs mswin_procs;
#endif
#ifdef SHIM_GRAPHICS
extern struct window_procs shim_procs;
#endif
#ifdef WINCHAIN
extern struct window_procs chainin_procs;
extern void chainin_procs_init(int);
extern void *chainin_procs_chain(int, int, void *, void *, void *);

extern struct chain_procs chainout_procs;
extern void chainout_procs_init(int);
extern void *chainout_procs_chain(int, int, void *, void *, void *);

extern struct chain_procs trace_procs;
extern void trace_procs_init(int);
extern void *trace_procs_chain(int, int, void *, void *, void *);
#endif

static void def_raw_print(const char *s);
static void def_wait_synch(void);

#ifdef DUMPLOG
static winid dump_create_nhwindow(int);
static void dump_clear_nhwindow(winid);
static void dump_display_nhwindow(winid, boolean);
static void dump_destroy_nhwindow(winid);
static void dump_start_menu(winid, unsigned long);
static void dump_add_menu(winid, const glyph_info *, const ANY_P *, char,
                          char, int, int, const char *, unsigned int);
static void dump_end_menu(winid, const char *);
static int dump_select_menu(winid, int, MENU_ITEM_P **);
static void dump_putstr(winid, int, const char *);
#endif /* DUMPLOG */

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
    void (*ini_routine)(int); /* optional (can be 0) */
#ifdef WINCHAIN
    void *(*chain_routine)(int, int, void *, void *, void *);
#endif
} winchoices[] = {
#ifdef TTY_GRAPHICS
    { &tty_procs, win_tty_init CHAINR(0) },
#endif
#ifdef CURSES_GRAPHICS
    { &curses_procs, 0 },
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
#ifdef GNOME_GRAPHICS
    { &Gnome_procs, 0 CHAINR(0) },
#endif
#ifdef MSWIN_GRAPHICS
    { &mswin_procs, 0 CHAINR(0) },
#endif
#ifdef SHIM_GRAPHICS
    { &shim_procs, 0 CHAINR(0) },
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
wl_new(void)
{
    struct winlink *wl = (struct winlink *) alloc(sizeof *wl);

    wl->nextlink = 0;
    wl->wincp = 0;
    wl->linkdata = 0;

    return wl;
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

boolean
genl_can_suspend_no(void)
{
    return FALSE;
}

boolean
genl_can_suspend_yes(void)
{
    return TRUE;
}

static
void
def_raw_print(const char *s)
{
    puts(s);
}

static
void
def_wait_synch(void)
{
    /* Config file error handling routines
     * call wait_sync() without checking to
     * see if it actually has a value,
     * leading to spectacular violations
     * when you try to execute address zero.
     * The existence of this allows early
     * processing to have something to execute
     * even though it essentially does nothing
     */
     return;
}

#ifdef WINCHAIN
static struct win_choices *
win_choices_find(const char *s)
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
choose_windows(const char *s)
{
    int i;
    char *tmps = 0;

    for (i = 0; winchoices[i].procs; i++) {
        if ('+' == winchoices[i].procs->name[0])
            continue;
        if ('-' == winchoices[i].procs->name[0])
            continue;
        if (!strcmpi(s, winchoices[i].procs->name)) {
            windowprocs = *winchoices[i].procs;

            if (gl.last_winchoice && gl.last_winchoice->ini_routine)
                (*gl.last_winchoice->ini_routine)(WININIT_UNDO);
            if (winchoices[i].ini_routine)
                (*winchoices[i].ini_routine)(WININIT);
            gl.last_winchoice = &winchoices[i];
            return;
        }
    }

    if (!windowprocs.win_raw_print)
        windowprocs.win_raw_print = def_raw_print;
    if (!windowprocs.win_wait_synch)
        /* early config file error processing routines call this */
        windowprocs.win_wait_synch = def_wait_synch;

    if (!winchoices[0].procs) {
        raw_printf("No window types supported?");
        nh_terminate(EXIT_FAILURE);
    }
    /* 50: arbitrary, no real window_type names are anywhere near that long;
       used to prevent potential raw_printf() overflow if user supplies a
       very long string (on the order of 1200 chars) on the command line
       (config file options can't get that big; they're truncated at 1023) */
#define WINDOW_TYPE_MAXLEN 50
    if (strlen(s) >= WINDOW_TYPE_MAXLEN) {
        tmps = (char *) alloc(WINDOW_TYPE_MAXLEN);
        (void) strncpy(tmps, s, WINDOW_TYPE_MAXLEN - 1);
        tmps[WINDOW_TYPE_MAXLEN - 1] = '\0';
        s = tmps;
    }
#undef WINDOW_TYPE_MAXLEN

    if (!winchoices[1].procs) {
        config_error_add(
                     "Window type %s not recognized.  The only choice is: %s",
                         s, winchoices[0].procs->name);
    } else {
        char buf[BUFSZ];
        boolean first = TRUE;

        buf[0] = '\0';
        for (i = 0; winchoices[i].procs; i++) {
            if ('+' == winchoices[i].procs->name[0])
                continue;
            if ('-' == winchoices[i].procs->name[0])
                continue;
            Sprintf(eos(buf), "%s%s",
                    first ? "" : ", ", winchoices[i].procs->name);
            first = FALSE;
        }
        config_error_add("Window type %s not recognized.  Choices are:  %s",
                         s, buf);
    }
    if (tmps)
        free((genericptr_t) tmps) /*, tmps = 0*/ ;

    if (windowprocs.win_raw_print == def_raw_print || WINDOWPORT(safestartup))
        nh_terminate(EXIT_SUCCESS);
}

#ifdef WINCHAIN
void
addto_windowchain(const char *s)
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

    nh_terminate(EXIT_FAILURE);
}

void
commit_windowchain(void)
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
                                              gl.last_winchoice->procs, 0);
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
genl_message_menu(char let UNUSED,
                  int how UNUSED,
                  const char *mesg)
{
    pline("%s", mesg);
    return 0;
}

/*ARGSUSED*/
void
genl_preference_update(const char *pref UNUSED)
{
    /* window ports are expected to provide
       their own preference update routine
       for the preference capabilities that
       they support.
       Just return in this genl one. */
    return;
}

char *
genl_getmsghistory(boolean init UNUSED)
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
genl_putmsghistory(const char *msg, boolean is_restoring)
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

static int hup_nhgetch(void);
static char hup_yn_function(const char *, const char *, char);
static int hup_nh_poskey(coordxy *, coordxy *, int *);
static void hup_getlin(const char *, char *);
static void hup_init_nhwindows(int *, char **);
static void hup_exit_nhwindows(const char *);
static winid hup_create_nhwindow(int);
static int hup_select_menu(winid, int, MENU_ITEM_P **);
static void hup_add_menu(winid, const glyph_info *, const anything *, char,
                         char, int, int, const char *, unsigned int);
static void hup_end_menu(winid, const char *);
static void hup_putstr(winid, int, const char *);
static void hup_print_glyph(winid, coordxy, coordxy, const glyph_info *,
                            const glyph_info *);
static void hup_outrip(winid, int, time_t);
static void hup_curs(winid, int, int);
static void hup_display_nhwindow(winid, boolean);
static void hup_display_file(const char *, boolean);
#ifdef CLIPPING
static void hup_cliparound(int, int);
#endif
#ifdef CHANGE_COLOR
static void hup_change_color(int, long, int);
#ifdef MAC
static short hup_set_font_name(winid, char *);
#endif
static char *hup_get_color_string(void);
#endif /* CHANGE_COLOR */
static void hup_status_update(int, genericptr_t, int, int, int,
                              unsigned long *);

static int hup_int_ndecl(void);
static void hup_void_ndecl(void);
static void hup_void_fdecl_int(int);
static void hup_void_fdecl_winid(winid);
static void hup_void_fdecl_winid_ulong(winid, unsigned long);
static void hup_void_fdecl_constchar_p(const char *);
static win_request_info *hup_ctrl_nhwindow(winid, int, win_request_info *);

static struct window_procs hup_procs = {
    WPID(hup), 0L, 0L,
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, /* colors */
    hup_init_nhwindows,
    hup_void_ndecl,                                    /* player_selection */
    hup_void_ndecl,                                    /* askname */
    hup_void_ndecl,                                    /* get_nh_event */
    hup_exit_nhwindows, hup_void_fdecl_constchar_p,    /* suspend_nhwindows */
    hup_void_ndecl,                                    /* resume_nhwindows */
    hup_create_nhwindow, hup_void_fdecl_winid,         /* clear_nhwindow */
    hup_display_nhwindow, hup_void_fdecl_winid,        /* destroy_nhwindow */
    hup_curs, hup_putstr, hup_putstr,                  /* putmixed */
    hup_display_file, hup_void_fdecl_winid_ulong,      /* start_menu */
    hup_add_menu, hup_end_menu, hup_select_menu, genl_message_menu,
    hup_void_ndecl,                                    /* mark_synch */
    hup_void_ndecl,                                    /* wait_synch */
#ifdef CLIPPING
    hup_cliparound,
#endif
#ifdef POSITIONBAR
    (void (*)(char *)) hup_void_fdecl_constchar_p,    /* update_positionbar */
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
    hup_void_ndecl,                                   /* status_init */
    hup_void_ndecl,                                   /* status_finish */
    genl_status_enablefield, hup_status_update,
    genl_can_suspend_no,
    hup_void_fdecl_int,                               /* update_inventory */
    hup_ctrl_nhwindow,
};

static void (*previnterface_exit_nhwindows)(const char *) = 0;

/* hangup has occurred; switch to no-op user interface */
void
nhwindows_hangup(void)
{
    char *(*previnterface_getmsghistory)(boolean) = 0;

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
hup_exit_nhwindows(const char *lastgasp)
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
hup_nhgetch(void)
{
    return '\033'; /* ESC */
}

/*ARGSUSED*/
static char
hup_yn_function(
    const char *prompt UNUSED,
    const char *resp UNUSED,
    char deflt)
{
    if (!deflt)
        deflt = '\033';
    return deflt;
}

/*ARGSUSED*/
static int
hup_nh_poskey(coordxy *x UNUSED, coordxy *y UNUSED, int *mod UNUSED)
{
    return '\033';
}

/*ARGSUSED*/
static void
hup_getlin(const char *prompt UNUSED, char *outbuf)
{
    Strcpy(outbuf, "\033");
}

/*ARGSUSED*/
static void
hup_init_nhwindows(int *argc_p UNUSED, char **argv UNUSED)
{
    iflags.window_inited = 1;
}

/*ARGUSED*/
static winid
hup_create_nhwindow(int type UNUSED)
{
    return WIN_ERR;
}

/*ARGSUSED*/
static int
hup_select_menu(
    winid window UNUSED,
    int how UNUSED,
    struct mi **menu_list UNUSED)
{
    return -1;
}

/*ARGSUSED*/
static void
hup_add_menu(
    winid window UNUSED,
    const glyph_info *glyphinfo UNUSED,
    const anything *identifier UNUSED,
    char sel UNUSED,
    char grpsel UNUSED,
    int attr UNUSED,
    int clr UNUSED,
    const char *txt UNUSED,
    unsigned int itemflags UNUSED)
{
    return;
}

/*ARGSUSED*/
static void
hup_end_menu(winid window UNUSED, const char *prompt UNUSED)
{
    return;
}

/*ARGSUSED*/
static void
hup_putstr(winid window UNUSED, int attr UNUSED, const char *text UNUSED)
{
    return;
}

/*ARGSUSED*/
static void
hup_print_glyph(
    winid window UNUSED,
    coordxy x UNUSED, coordxy y UNUSED,
    const glyph_info *glyphinfo UNUSED,
    const glyph_info *bkglyphinfo UNUSED)
{
    return;
}

/*ARGSUSED*/
static void
hup_outrip(winid tmpwin UNUSED, int how UNUSED, time_t when UNUSED)
{
    return;
}

/*ARGSUSED*/
static void
hup_curs(winid window UNUSED, int x UNUSED, int y UNUSED)
{
    return;
}

/*ARGSUSED*/
static void
hup_display_nhwindow(winid window UNUSED, boolean blocking UNUSED)
{
    return;
}

/*ARGSUSED*/
static void
hup_display_file(const char *fname UNUSED, boolean complain UNUSED)
{
    return;
}

#ifdef CLIPPING
/*ARGSUSED*/
static void
hup_cliparound(int x UNUSED, int y UNUSED)
{
    return;
}
#endif

#ifdef CHANGE_COLOR
/*ARGSUSED*/
static void
hup_change_color(int color, int reverse, long rgb)
{
    return;
}

#ifdef MAC
/*ARGSUSED*/
static short
hup_set_font_name(winid window, char *fontname)
{
    return 0;
}
#endif /* MAC */

static char *
hup_get_color_string(void)
{
    return (char *) 0;
}
#endif /* CHANGE_COLOR */

/*ARGSUSED*/
static void
hup_status_update(
    int idx UNUSED, genericptr_t ptr UNUSED,
    int chg UNUSED, int pc UNUSED,
    int color UNUSED, unsigned long *colormasks UNUSED)
{
    return;
}

/*
 * Non-specific stubs.
 */

static int
hup_int_ndecl(void)
{
    return -1;
}

static void
hup_void_ndecl(void)
{
    return;
}

/*ARGUSED*/
static void
hup_void_fdecl_int(int arg UNUSED)
{
    return;
}

/*ARGUSED*/
static void
hup_void_fdecl_winid(winid window UNUSED)
{
    return;
}

/*ARGUSED*/
static void
hup_void_fdecl_winid_ulong(
    winid window UNUSED,
    unsigned long mbehavior UNUSED)
{
    return;
}

/*ARGUSED*/
static void
hup_void_fdecl_constchar_p(const char *string UNUSED)
{
    return;
}

/*ARGUSED*/
win_request_info *
hup_ctrl_nhwindow(
    winid window UNUSED,  /* window to use, must be of type NHW_MENU */
    int request UNUSED,
    win_request_info *wri UNUSED)
{
    return (win_request_info *) 0;
}

#endif /* HANGUPHANDLING */


/****************************************************************************/
/* genl backward compat stuff                                               */
/****************************************************************************/

const char *status_fieldnm[MAXBLSTATS];
const char *status_fieldfmt[MAXBLSTATS];
char *status_vals[MAXBLSTATS];
boolean status_activefields[MAXBLSTATS];

void
genl_status_init(void)
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
genl_status_finish(void)
{
    /* tear down routine */
    int i;

    /* free alloc'd memory here */
    for (i = 0; i < MAXBLSTATS; ++i) {
        if (status_vals[i])
            free((genericptr_t) status_vals[i]), status_vals[i] = (char *) 0;
    }
}

void
genl_status_enablefield(
    int fieldidx,
    const char *nm,
    const char *fmt,
    boolean enable)
{
    status_fieldfmt[fieldidx] = fmt;
    status_fieldnm[fieldidx] = nm;
    status_activefields[fieldidx] = enable;
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* call once for each field, then call with BL_FLUSH to output the result */
void
genl_status_update(
    int idx,
    genericptr_t ptr,
    int chg UNUSED, int percent UNUSED,
    int color UNUSED, unsigned long *colormasks UNUSED)
{
    char newbot1[MAXCO], newbot2[MAXCO];
    long cond, *condptr = (long *) ptr;
    register int i;
    unsigned pass, lndelta;
    enum statusfields idx1, idx2, *fieldlist;
    char *nb, *text = (char *) ptr;

    static enum statusfields fieldorder[][15] = {
        /* line one */
        { BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_ALIGN,
          BL_SCORE, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH,
          BL_FLUSH },
        /* line two, default order */
        { BL_LEVELDESC, BL_GOLD,
          BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX, BL_AC,
          BL_XP, BL_EXP, BL_HD,
          BL_TIME,
          BL_HUNGER, BL_CAP, BL_CONDITION,
          BL_FLUSH },
        /* move time to the end */
        { BL_LEVELDESC, BL_GOLD,
          BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX, BL_AC,
          BL_XP, BL_EXP, BL_HD,
          BL_HUNGER, BL_CAP, BL_CONDITION,
          BL_TIME, BL_FLUSH },
        /* move experience and time to the end */
        { BL_LEVELDESC, BL_GOLD,
          BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX, BL_AC,
          BL_HUNGER, BL_CAP, BL_CONDITION,
          BL_XP, BL_EXP, BL_HD, BL_TIME, BL_FLUSH },
        /* move level description plus gold and experience and time to end */
        { BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX, BL_AC,
          BL_HUNGER, BL_CAP, BL_CONDITION,
          BL_LEVELDESC, BL_GOLD, BL_XP, BL_EXP, BL_HD, BL_TIME, BL_FLUSH },
    };

    /* in case interface is using genl_status_update() but has not
       specified WC2_FLUSH_STATUS (status_update() for field values
       is buffered so final BL_FLUSH is needed to produce output) */
    windowprocs.wincap2 |= WC2_FLUSH_STATUS;

    if (idx >= 0) {
        if (!status_activefields[idx])
            return;
        switch (idx) {
        case BL_CONDITION:
            cond = condptr ? *condptr : 0L;
            nb = status_vals[idx];
            *nb = '\0';
            if (cond & BL_MASK_STONE)
                Strcpy(nb = eos(nb), " Stone");
            if (cond & BL_MASK_SLIME)
                Strcpy(nb = eos(nb), " Slime");
            if (cond & BL_MASK_STRNGL)
                Strcpy(nb = eos(nb), " Strngl");
            if (cond & BL_MASK_FOODPOIS)
                Strcpy(nb = eos(nb), " FoodPois");
            if (cond & BL_MASK_TERMILL)
                Strcpy(nb = eos(nb), " TermIll");
            if (cond & BL_MASK_BLIND)
                Strcpy(nb = eos(nb), " Blind");
            if (cond & BL_MASK_DEAF)
                Strcpy(nb = eos(nb), " Deaf");
            if (cond & BL_MASK_STUN)
                Strcpy(nb = eos(nb), " Stun");
            if (cond & BL_MASK_CONF)
                Strcpy(nb = eos(nb), " Conf");
            if (cond & BL_MASK_HALLU)
                Strcpy(nb = eos(nb), " Hallu");
            if (cond & BL_MASK_LEV)
                Strcpy(nb = eos(nb), " Lev");
            if (cond & BL_MASK_FLY)
                Strcpy(nb = eos(nb), " Fly");
            if (cond & BL_MASK_RIDE)
                Strcpy(nb = eos(nb), " Ride");
            break;
        default:
            Sprintf(status_vals[idx],
                    status_fieldfmt[idx] ? status_fieldfmt[idx] : "%s",
                    text ? text : "");
            break;
        }
        return; /* processed one field other than BL_FLUSH */
    } /* (idx >= 0, thus not BL_FLUSH, BL_RESET, BL_CHARACTERISTICS) */

    /* does BL_RESET require any specific code to ensure all fields ? */

    if (!(idx == BL_FLUSH || idx == BL_RESET))
        return;

    /* We've received BL_FLUSH; time to output the gathered data */
    nb = newbot1;
    *nb = '\0';
    /* BL_FLUSH is the only pseudo-index value we need to check for
       in the loop below because it is the only entry used to pad the
       end of the fieldorder array. We could stop on any
       negative (illegal) index, but this should be fine */
    for (i = 0; (idx1 = fieldorder[0][i]) != BL_FLUSH; ++i) {
        if (status_activefields[idx1])
            Strcpy(nb = eos(nb), status_vals[idx1]);
    }
    /* if '$' is encoded, buffer length of \GXXXXNNNN is 9 greater than
       single char; we want to subtract that 9 when checking display length */
    lndelta = (status_activefields[BL_GOLD]
               && strstr(status_vals[BL_GOLD], "\\G")) ? 9 : 0;
    /* basic bot2 formats groups of second line fields into five buffers,
       then decides how to order those buffers based on comparing lengths
       of [sub]sets of them to the width of the map; we have more control
       here but currently emulate that behavior */
    for (pass = 1; pass <= 4; pass++) {
        fieldlist = fieldorder[pass];
        nb = newbot2;
        *nb = '\0';
        for (i = 0; (idx2 = fieldlist[i]) != BL_FLUSH; ++i) {
            if (status_activefields[idx2]) {
                const char *val = status_vals[idx2];

                switch (idx2) {
                case BL_HP: /* for pass 4, Hp comes first; mungspaces()
                               will strip the unwanted leading spaces */
                case BL_XP: case BL_HD:
                case BL_TIME:
                    Strcpy(nb = eos(nb), " ");
                    break;
                case BL_LEVELDESC:
                    /* leveldesc has no leading space, so if we've moved
                       it past the first position, provide one */
                    if (i != 0)
                        Strcpy(nb = eos(nb), " ");
                    break;
                /*
                 * We want "  hunger encumbrance conditions"
                 *   or    "  encumbrance conditions"
                 *   or    "  hunger conditions"
                 *   or    "  conditions"
                 * 'hunger'      is either " " or " hunger_text";
                 * 'encumbrance' is either " " or " encumbrance_text";
                 * 'conditions'  is either ""  or " cond1 cond2...".
                 */
                case BL_HUNGER:
                    /* hunger==" " - keep it, end up with " ";
                       hunger!=" " - insert space and get "  hunger" */
                    if (strcmp(val, " "))
                        Strcpy(nb = eos(nb), " ");
                    break;
                case BL_CAP:
                    /* cap==" " - suppress it, retain "  hunger" or " ";
                       cap!=" " - use it, get "  hunger cap" or "  cap" */
                    if (!strcmp(val, " "))
                        ++val;
                    break;
                default:
                    break;
                }
                Strcpy(nb = eos(nb), val); /* status_vals[idx2] */
            } /* status_activefields[idx2] */

            if (idx2 == BL_CONDITION && pass < 4
                && strlen(newbot2) - lndelta > COLNO)
                break; /* switch to next order */
        } /* i */

        if (idx2 == BL_FLUSH) { /* made it past BL_CONDITION */
            if (pass > 1)
                mungspaces(newbot2);
            break;
        }
    } /* pass */
    curs(WIN_STATUS, 1, 0);
    putstr(WIN_STATUS, 0, newbot1);
    curs(WIN_STATUS, 1, 1);
    putmixed(WIN_STATUS, 0, newbot2); /* putmixed() due to GOLD glyph */
}

RESTORE_WARNING_FORMAT_NONLITERAL

static struct window_procs dumplog_windowprocs_backup;
static FILE *dumplog_file;

#ifdef DUMPLOG
static time_t dumplog_now;

char *
dump_fmtstr(
    const char *fmt,
    char *buf,
    boolean fullsubs) /* True -> full substitution for file name,
                       * False -> partial substitution for '--showpaths'
                       * feedback where there's no game in progress */
{
    const char *fp = fmt;
    char *bp = buf;
    int slen, len = 0;
    char tmpbuf[BUFSZ];
    char verbuf[BUFSZ];
    long uid;
    time_t now;

    now = dumplog_now;
    uid = (long) getuid();

    /*
     * Note: %t and %T assume that time_t is a 'long int' number of
     * seconds since some epoch value.  That's quite iffy....  The
     * unit of time might be different and the datum size might be
     * some variant of 'long long int'.  [Their main purpose is to
     * construct a unique file name rather than record the date and
     * time; violating the 'long seconds since base-date' assumption
     * may or may not interfere with that usage.]
     */

    while (fp && *fp && len < BUFSZ - 1) {
        if (*fp == '%') {
            fp++;
            switch (*fp) {
            default:
                goto finish;
            case '\0': /* fallthrough */
            case '%':  /* literal % */
                Sprintf(tmpbuf, "%%");
                break;
            case 't': /* game start, timestamp */
                if (fullsubs)
                    Sprintf(tmpbuf, "%lu", (unsigned long) ubirthday);
                else
                    Strcpy(tmpbuf, "{game start cookie}");
                break;
            case 'T': /* current time, timestamp */
                if (fullsubs)
                    Sprintf(tmpbuf, "%lu", (unsigned long) now);
                else
                    Strcpy(tmpbuf, "{current time cookie}");
                break;
            case 'd': /* game start, YYYYMMDDhhmmss */
                if (fullsubs)
                    Sprintf(tmpbuf, "%08ld%06ld",
                            yyyymmdd(ubirthday), hhmmss(ubirthday));
                else
                    Strcpy(tmpbuf, "{game start date+time}");
                break;
            case 'D': /* current time, YYYYMMDDhhmmss */
                if (fullsubs)
                    Sprintf(tmpbuf, "%08ld%06ld", yyyymmdd(now), hhmmss(now));
                else
                    Strcpy(tmpbuf, "{current date+time}");
                break;
            case 'v': /* version, eg. "3.7.0-0" */
                Sprintf(tmpbuf, "%s", version_string(verbuf, sizeof verbuf));
                break;
            case 'u': /* UID */
                Sprintf(tmpbuf, "%ld", uid);
                break;
            case 'n': /* player name */
                if (fullsubs)
                    Sprintf(tmpbuf, "%s", *gp.plname ? gp.plname : "unknown");
                else
                    Strcpy(tmpbuf, "{hero name}");
                break;
            case 'N': /* first character of player name */
                if (fullsubs)
                    Sprintf(tmpbuf, "%c", *gp.plname ? *gp.plname : 'u');
                else
                    Strcpy(tmpbuf, "{hero initial}");
                break;
            }
            if (fullsubs) {
                /* replace potentially troublesome characters (including
                   <space> even though it might be an acceptable file name
                   character); user shouldn't be able to get ' ' or '/'
                   or '\\' into plname[] but play things safe */
                (void) strNsubst(tmpbuf, " ", "_", 0);
                (void) strNsubst(tmpbuf, "/", "_", 0);
                (void) strNsubst(tmpbuf, "\\", "_", 0);
                /* note: replacements are only done on field substitutions,
                   not on the template (from sysconf or DUMPLOG_FILE) */
            }

            slen = (int) strlen(tmpbuf);
            if (len + slen < BUFSZ - 1) {
                len += slen;
                Sprintf(bp, "%s", tmpbuf);
                bp += slen;
                if (*fp)
                    fp++;
            } else
                break;
        } else {
            *bp = *fp;
            bp++;
            fp++;
            len++;
        }
    }
 finish:
    *bp = '\0';
    return buf;
}
#endif /* DUMPLOG */

void
dump_open_log(time_t now)
{
#ifdef DUMPLOG
    char buf[BUFSZ];
    char *fname;

    dumplog_now = now;
#ifdef SYSCF
    if (!sysopt.dumplogfile)
        return;
    fname = dump_fmtstr(sysopt.dumplogfile, buf, TRUE);
#else
    fname = dump_fmtstr(DUMPLOG_FILE, buf, TRUE);
#endif
    dumplog_file = fopen(fname, "w");
    dumplog_windowprocs_backup = windowprocs;

#else /*!DUMPLOG*/
    nhUse(now);
#endif /*?DUMPLOG*/
}

void
dump_close_log(void)
{
    if (dumplog_file) {
        (void) fclose(dumplog_file);
        dumplog_file = (FILE *) 0;
    }
}

void
dump_forward_putstr(winid win, int attr, const char *str, int no_forward)
{
    if (dumplog_file)
        fprintf(dumplog_file, "%s\n", str);
    if (!no_forward)
        putstr(win, attr, str);
}

/*ARGSUSED*/
static void
dump_putstr(winid win UNUSED, int attr UNUSED, const char *str)
{
    if (dumplog_file)
        fprintf(dumplog_file, "%s\n", str);
}

static winid
dump_create_nhwindow(int type UNUSED)
{
    return WIN_ERR;
}

/*ARGUSED*/
static void
dump_clear_nhwindow(winid win UNUSED)
{
    return;
}

/*ARGSUSED*/
static void
dump_display_nhwindow(winid win UNUSED, boolean p UNUSED)
{
    return;
}

/*ARGUSED*/
static void
dump_destroy_nhwindow(winid win UNUSED)
{
    return;
}

/*ARGUSED*/
static void
dump_start_menu(winid win UNUSED, unsigned long mbehavior UNUSED)
{
    return;
}

/*ARGSUSED*/
static void
dump_add_menu(winid win UNUSED,
              const glyph_info *glyphinfo,
              const anything *identifier UNUSED,
              char ch,
              char gch UNUSED,
              int attr UNUSED,
              int clr UNUSED,
              const char *str,
              unsigned int itemflags UNUSED)
{
    if (dumplog_file) {
        if (glyphinfo->glyph == NO_GLYPH)
            fprintf(dumplog_file, " %s\n", str);
        else
            fprintf(dumplog_file, "  %c - %s\n", ch, str);
    }
}

/*ARGSUSED*/
static void
dump_end_menu(winid win UNUSED, const char *str)
{
    if (dumplog_file) {
        if (str)
            fprintf(dumplog_file, "%s\n", str);
        else
            fputs("\n", dumplog_file);
    }
}

static int
dump_select_menu(winid win UNUSED, int how UNUSED, menu_item **item)
{
    *item = (menu_item *) 0;
    return 0;
}

void
dump_redirect(boolean onoff_flag)
{
    if (dumplog_file) {
        if (onoff_flag) {
            windowprocs.win_create_nhwindow = dump_create_nhwindow;
            windowprocs.win_clear_nhwindow = dump_clear_nhwindow;
            windowprocs.win_display_nhwindow = dump_display_nhwindow;
            windowprocs.win_destroy_nhwindow = dump_destroy_nhwindow;
            windowprocs.win_start_menu = dump_start_menu;
            windowprocs.win_add_menu = dump_add_menu;
            windowprocs.win_end_menu = dump_end_menu;
            windowprocs.win_select_menu = dump_select_menu;
            windowprocs.win_putstr = dump_putstr;
        } else {
            windowprocs = dumplog_windowprocs_backup;
        }
        iflags.in_dumplog = onoff_flag;
    } else {
        iflags.in_dumplog = FALSE;
    }
}

#ifdef TTY_GRAPHICS
#ifdef TEXTCOLOR
#ifdef TOS
extern const char *hilites[CLR_MAX];
#else
extern NEARDATA char *hilites[CLR_MAX];
#endif
#endif
#endif

int
has_color(int color)
{
    return (iflags.use_color && windowprocs.name
            && (windowprocs.wincap & WC_COLOR) && windowprocs.has_color[color]
#ifdef TTY_GRAPHICS
#if defined(TEXTCOLOR) && defined(TERMLIB) && !defined(NO_TERMS)
             && (hilites[color] != 0)
#endif
#endif
    );
}

int
glyph2ttychar(int glyph)
{
    glyph_info glyphinfo;

    map_glyphinfo(0, 0, glyph, 0, &glyphinfo);
    return glyphinfo.ttychar;
}

int
glyph2symidx(int glyph)
{
    glyph_info glyphinfo;

    map_glyphinfo(0, 0, glyph, 0, &glyphinfo);
    return glyphinfo.gm.sym.symidx;
}

char *
encglyph(int glyph)
{
    static char encbuf[20]; /* 10+1 would suffice */

    Sprintf(encbuf, "\\G%04X%04X", gc.context.rndencode, glyph);
    return encbuf;
}

int
decode_glyph(const char *str, int *glyph_ptr)
{
    static const char hex[] = "00112233445566778899aAbBcCdDeEfF";
    int rndchk = 0, dcount = 0, retval = 0;
    const char *dp;

    for (; *str && ++dcount <= 4; ++str) {
        if ((dp = strchr(hex, *str)) != 0) {
            retval++;
            rndchk = (rndchk * 16) + ((int) (dp - hex) / 2);
        } else
            break;
    }
    if (rndchk == gc.context.rndencode) {
        *glyph_ptr = dcount = 0;
        for (; *str && ++dcount <= 4; ++str) {
            if ((dp = strchr(hex, *str)) != 0) {
                retval++;
                *glyph_ptr = (*glyph_ptr * 16) + ((int) (dp - hex) / 2);
            } else
                break;
        }
        return retval;
    }
    return 0;
}

char *
decode_mixed(char *buf, const char *str)
{
    char *put = buf;
    glyph_info glyphinfo = nul_glyphinfo;

    if (!str)
        return strcpy(buf, "");

    while (*str) {
        if (*str == '\\') {
            int dcount, so, ggv;
            const char *save_str;

            save_str = str++;
            switch (*str) {
            case 'G': /* glyph value \GXXXXNNNN*/
                if ((dcount = decode_glyph(str + 1, &ggv))) {
                    str += (dcount + 1);
                    map_glyphinfo(0, 0, ggv, 0, &glyphinfo);
                    so = glyphinfo.gm.sym.symidx;
                    *put++ = gs.showsyms[so];
                    /* 'str' is ready for the next loop iteration and '*str'
                       should not be copied at the end of this iteration */
                    continue;
                } else {
                    /* possible forgery - leave it the way it is */
                    str = save_str;
                }
                break;
            case '\\':
                break;
            case '\0':
                /* String ended with '\\'.  This can happen when someone
                   names an object with a name ending with '\\', drops the
                   named object on the floor nearby and does a look at all
                   nearby objects. */
                /* brh - should we perhaps not allow things to have names
                   that contain '\\' */
                str = save_str;
                break;
            }
        }
        *put++ = *str++;
    }
    *put = '\0';
    return buf;
}


/*
 * This differs from putstr() because the str parameter can
 * contain a sequence of characters representing:
 *        \GXXXXNNNN    a glyph value, encoded by encglyph().
 *
 * For window ports that haven't yet written their own
 * XXX_putmixed() routine, this general one can be used.
 * It replaces the encoded glyph sequence with a single
 * showsyms[] char, then just passes that string onto
 * putstr().
 */

void
genl_putmixed(winid window, int attr, const char *str)
{
    char buf[BUFSZ];

    /* now send it to the normal putstr */
    putstr(window, attr, decode_mixed(buf, str));
}

/* possibly called to show usage info during command line processing when
   an interface hasn't yet been chosen and set up */
void
genl_display_file(const char *fname, boolean complain)
{
    char buf[BUFSZ];
    dlb *f = dlb_fopen(fname, "r");

    if (!f) {
        if (complain) /* send complaint to stdout rather than to stderr */
            fprintf(stdout, "\nCannot open \"%s\".\n", fname);
    } else {
        /* straight copy to stdout, no pagination or other interaction */
        while (dlb_fgets(buf, BUFSZ, f)) {
            if (fputs(buf, stdout) < 0)
                break;
        }
        (void) dlb_fclose(f);
    }
}

/*
 * Window port helper function for menu invert routines to move the decision
 * logic into one place instead of 7 different window-port routines.
 */
boolean
menuitem_invert_test(
    int mode UNUSED,        /* 0: invert; 1: select; 2: deselect */
    unsigned itemflags,     /* itemflags for the item */
    boolean is_selected)    /* current selection status of the item */
{
    boolean skipinvert = (itemflags & MENU_ITEMFLAGS_SKIPINVERT) != 0;

    if (!skipinvert) /* if not flagged SKIPINVERT, always pass test */
        return TRUE;
    /*
     * mode 0: inverting current on/off state;
     *      1: unconditionally setting on;
     *      2: unconditionally setting off.
     * menuinvertmode 0: treat entries flagged with skipinvert as ordinary
     *                   (same as if not flagged);
     * menuinvertmode 1: don't toggle bulk invert or bulk select entries On;
     *                   allow toggling to Off (for invert and deselect;
     *                   select doesn't do Off);
     * menuinvertmode 2: don't toggle skipinvert entries either On or Off
     *                   when any bulk change is performed.
     */
    if (iflags.menuinvertmode == 2) {
        return FALSE;
    } else if (iflags.menuinvertmode == 1) {
        return is_selected ? TRUE : FALSE;
    }
    return TRUE;
}

/*windows.c*/

/* NetHack 3.7	windows.c	$NHDT-Date: 1700012891 2023/11/15 01:48:11 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.109 $ */
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

#if defined(WINCHAIN) || defined(TTY_GRAPHICS)
staticfn struct win_choices *win_choices_find(const char *s) NONNULLARG1;
#endif

staticfn void def_raw_print(const char *s) NONNULLARG1;
staticfn void def_wait_synch(void);
staticfn boolean get_menu_coloring(const char *, int *, int *) NONNULLPTRS;

staticfn winid dump_create_nhwindow(int);
staticfn void dump_clear_nhwindow(winid);
staticfn void dump_display_nhwindow(winid, boolean);
staticfn void dump_destroy_nhwindow(winid);
staticfn void dump_start_menu(winid, unsigned long);
staticfn void dump_add_menu(winid, const glyph_info *, const ANY_P *, char,
                          char, int, int, const char *, unsigned int);
staticfn void dump_end_menu(winid, const char *);
staticfn int dump_select_menu(winid, int, MENU_ITEM_P **);
staticfn void dump_putstr(winid, int, const char *);

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

staticfn struct winlink *
wl_new(void)
{
    struct winlink *wl = (struct winlink *) alloc(sizeof *wl);

    wl->nextlink = 0;
    wl->wincp = 0;
    wl->linkdata = 0;

    return wl;
}

staticfn void
wl_addhead(struct winlink *wl)
{
    wl->nextlink = chain;
    chain = wl;
}

staticfn void
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

staticfn
void
def_raw_print(const char *s)
{
    puts(s);
    if (*s)
        iflags.raw_printed++;
}

staticfn
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

#ifdef TTY_GRAPHICS
boolean
check_tty_wincap(unsigned long wincap)
{
    struct win_choices *wc = win_choices_find("tty");

    if (wc)
        return ((wc->procs->wincap & wincap) == wincap);
    return FALSE;
}

boolean
check_tty_wincap2(unsigned long wincap2)
{
    struct win_choices *wc = win_choices_find("tty");

    if (wc)
        return ((wc->procs->wincap2 & wincap2) == wincap2);
    return FALSE;
}
#endif

#if defined(WINCHAIN) || defined(TTY_GRAPHICS)
staticfn struct win_choices *
win_choices_find(const char *s)
{
    int i;

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
    int i;

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

staticfn int hup_nhgetch(void);
staticfn char hup_yn_function(const char *, const char *, char);
staticfn int hup_nh_poskey(coordxy *, coordxy *, int *);
staticfn void hup_getlin(const char *, char *);
staticfn void hup_init_nhwindows(int *, char **);
staticfn void hup_exit_nhwindows(const char *);
staticfn winid hup_create_nhwindow(int);
staticfn int hup_select_menu(winid, int, MENU_ITEM_P **);
staticfn void hup_add_menu(winid, const glyph_info *, const anything *, char,
                         char, int, int, const char *, unsigned int);
staticfn void hup_end_menu(winid, const char *);
staticfn void hup_putstr(winid, int, const char *);
staticfn void hup_print_glyph(winid, coordxy, coordxy, const glyph_info *,
                            const glyph_info *);
staticfn void hup_outrip(winid, int, time_t);
staticfn void hup_curs(winid, int, int);
staticfn void hup_display_nhwindow(winid, boolean);
staticfn void hup_display_file(const char *, boolean);
#ifdef CLIPPING
staticfn void hup_cliparound(int, int);
#endif
#ifdef CHANGE_COLOR
staticfn void hup_change_color(int, long, int);
#ifdef MAC
staticfn short hup_set_font_name(winid, char *);
#endif
staticfn char *hup_get_color_string(void);
#endif /* CHANGE_COLOR */
staticfn void hup_status_update(int, genericptr_t, int, int, int,
                              unsigned long *);

staticfn int hup_int_ndecl(void);
staticfn void hup_void_ndecl(void);
staticfn void hup_void_fdecl_int(int);
staticfn void hup_void_fdecl_winid(winid);
staticfn void hup_void_fdecl_winid_ulong(winid, unsigned long);
staticfn void hup_void_fdecl_constchar_p(const char *);
staticfn win_request_info *hup_ctrl_nhwindow(winid, int, win_request_info *);

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
    hup_void_ndecl,                                   /* nh_delay_output  */
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

staticfn void
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
    iflags.window_inited = FALSE;
}

staticfn int
hup_nhgetch(void)
{
    return '\033'; /* ESC */
}

/*ARGSUSED*/
staticfn char
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
staticfn int
hup_nh_poskey(coordxy *x UNUSED, coordxy *y UNUSED, int *mod UNUSED)
{
    return '\033';
}

/*ARGSUSED*/
staticfn void
hup_getlin(const char *prompt UNUSED, char *outbuf)
{
    Strcpy(outbuf, "\033");
}

/*ARGSUSED*/
staticfn void
hup_init_nhwindows(int *argc_p UNUSED, char **argv UNUSED)
{
    iflags.window_inited = TRUE;
}

/*ARGUSED*/
staticfn winid
hup_create_nhwindow(int type UNUSED)
{
    return WIN_ERR;
}

/*ARGSUSED*/
staticfn int
hup_select_menu(
    winid window UNUSED,
    int how UNUSED,
    struct mi **menu_list UNUSED)
{
    return -1;
}

/*ARGSUSED*/
staticfn void
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
staticfn void
hup_end_menu(winid window UNUSED, const char *prompt UNUSED)
{
    return;
}

/*ARGSUSED*/
staticfn void
hup_putstr(winid window UNUSED, int attr UNUSED, const char *text UNUSED)
{
    return;
}

/*ARGSUSED*/
staticfn void
hup_print_glyph(
    winid window UNUSED,
    coordxy x UNUSED, coordxy y UNUSED,
    const glyph_info *glyphinfo UNUSED,
    const glyph_info *bkglyphinfo UNUSED)
{
    return;
}

/*ARGSUSED*/
staticfn void
hup_outrip(winid tmpwin UNUSED, int how UNUSED, time_t when UNUSED)
{
    return;
}

/*ARGSUSED*/
staticfn void
hup_curs(winid window UNUSED, int x UNUSED, int y UNUSED)
{
    return;
}

/*ARGSUSED*/
staticfn void
hup_display_nhwindow(winid window UNUSED, boolean blocking UNUSED)
{
    return;
}

/*ARGSUSED*/
staticfn void
hup_display_file(const char *fname UNUSED, boolean complain UNUSED)
{
    return;
}

#ifdef CLIPPING
/*ARGSUSED*/
staticfn void
hup_cliparound(int x UNUSED, int y UNUSED)
{
    return;
}
#endif

#ifdef CHANGE_COLOR
/*ARGSUSED*/
staticfn void
hup_change_color(int color UNUSED, long rgb UNUSED, int reverse UNUSED)
{
    return;
}

#ifdef MAC
/*ARGSUSED*/
staticfn short
hup_set_font_name(winid window UNUSED, char *fontname UNUSED)
{
    return 0;
}
#endif /* MAC */

staticfn char *
hup_get_color_string(void)
{
    return (char *) 0;
}
#endif /* CHANGE_COLOR */

/*ARGSUSED*/
staticfn void
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

staticfn int
hup_int_ndecl(void)
{
    return -1;
}

staticfn void
hup_void_ndecl(void)
{
    return;
}

/*ARGUSED*/
staticfn void
hup_void_fdecl_int(int arg UNUSED)
{
    return;
}

/*ARGUSED*/
staticfn void
hup_void_fdecl_winid(winid window UNUSED)
{
    return;
}

/*ARGUSED*/
staticfn void
hup_void_fdecl_winid_ulong(
    winid window UNUSED,
    unsigned long mbehavior UNUSED)
{
    return;
}

/*ARGUSED*/
staticfn void
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
    int i;
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
                    Sprintf(tmpbuf, "%s",
                            *svp.plname ? svp.plname : "unknown");
                else
                    Strcpy(tmpbuf, "{hero name}");
                break;
            case 'N': /* first character of player name */
                if (fullsubs)
                    Sprintf(tmpbuf, "%c", *svp.plname ? *svp.plname : 'u');
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
staticfn void
dump_putstr(winid win UNUSED, int attr UNUSED, const char *str)
{
    if (dumplog_file)
        fprintf(dumplog_file, "%s\n", str);
}

staticfn winid
dump_create_nhwindow(int type UNUSED)
{
    return WIN_ERR;
}

/*ARGUSED*/
staticfn void
dump_clear_nhwindow(winid win UNUSED)
{
    return;
}

/*ARGSUSED*/
staticfn void
dump_display_nhwindow(winid win UNUSED, boolean p UNUSED)
{
    return;
}

/*ARGUSED*/
staticfn void
dump_destroy_nhwindow(winid win UNUSED)
{
    return;
}

/*ARGUSED*/
staticfn void
dump_start_menu(winid win UNUSED, unsigned long mbehavior UNUSED)
{
    return;
}

/*ARGSUSED*/
staticfn void
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
staticfn void
dump_end_menu(winid win UNUSED, const char *str)
{
    if (dumplog_file) {
        if (str)
            fprintf(dumplog_file, "%s\n", str);
        else
            fputs("\n", dumplog_file);
    }
}

staticfn int
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
#ifdef TOS
extern const char *hilites[CLR_MAX];
#else
extern NEARDATA char *hilites[CLR_MAX];
#endif
#endif

int
has_color(int color)
{
    return (iflags.use_color && windowprocs.name
            && (windowprocs.wincap & WC_COLOR) && windowprocs.has_color[color]
#ifdef TTY_GRAPHICS
#if defined(TERMLIB) && !defined(NO_TERMS)
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

    Sprintf(encbuf, "\\G%04X%04X", svc.context.rndencode, glyph);
    return encbuf;
}

/* hexdd[] is defined in decl.c */

int
decode_glyph(const char *str, int *glyph_ptr)
{
    int rndchk = 0, dcount = 0, retval = 0;
    const char *dp;

    for (; *str && ++dcount <= 4; ++str) {
        if ((dp = strchr(hexdd, *str)) != 0) {
            retval++;
            rndchk = (rndchk * 16) + ((int) (dp - hexdd) / 2);
        } else
            break;
    }
    if (rndchk == svc.context.rndencode) {
        *glyph_ptr = dcount = 0;
        for (; *str && ++dcount <= 4; ++str) {
            if ((dp = strchr(hexdd, *str)) != 0) {
                retval++;
                *glyph_ptr = (*glyph_ptr * 16) + ((int) (dp - hexdd) / 2);
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

/*
 * helper routine if a window port wants to extract the glyph
 * information from a glyph number representation in the string;
 * the returned string is the remainder of the string after
 * extracting the \GNNNNNNNN information. The glyph details,
 * including the utf8 representation under ENHANCED_SYMBOLS,
 * will be stored in the glyph_info struct pointed to by gip.
 */
const char *
mixed_to_glyphinfo(const char *str, glyph_info *gip)
{
    int dcount, ggv;

    if (!str || !gip)
        return " ";

    *gip = nul_glyphinfo;
    if (*str == '\\' && *(str + 1) == 'G') {
        if ((dcount = decode_glyph(str + 2, &ggv))) {
            map_glyphinfo(0, 0, ggv, 0, gip);
            /* 'str' is ready for the next loop iteration and
                '*str' should not be copied at the end of this
                iteration */
            str += (dcount + 2);
        }
    }
    return str;
}

/*
 * This is a somewhat generic menu for taking a list of NetHack style
 * class choices and presenting them via a description
 * rather than the traditional NetHack characters.
 * (Benefits users whose first exposure to NetHack is via tiles).
 *
 * prompt
 *           The title at the top of the menu.
 *
 * category: 0 = monster class
 *           1 = object  class
 *
 * way
 *           FALSE = PICK_ONE, TRUE = PICK_ANY
 *
 * class_list
 *           a null terminated string containing the list of choices.
 *
 * class_selection
 *           a null terminated string containing the selected characters.
 *
 * Returns number selected.
 */
int
choose_classes_menu(const char *prompt,
                    int category,
                    boolean way,
                    char *class_list,
                    char *class_select)
{
    menu_item *pick_list = (menu_item *) 0;
    winid win;
    anything any;
    char buf[BUFSZ];
    const char *text = 0;
    boolean selected;
    int ret, i, n, next_accelerator, accelerator;
    int clr = NO_COLOR;

    if (!class_list || !class_select)
        return 0;
    accelerator = 0;
    next_accelerator = 'a';
    any = cg.zeroany;
    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);
    while (*class_list) {
        int idx;

        selected = FALSE;
        switch (category) {
        case 0:
            idx = def_char_to_monclass(*class_list);
            if (!IndexOk(idx, def_monsyms)) {
                panic("choose_classes_menu: invalid monclass '%c'",
                      *class_list);
                /*NOTREACHED*/
            }
            text = def_monsyms[idx].explain;
            accelerator = *class_list;
            Sprintf(buf, "%s", text);
            break;
        case 1:
            idx = def_char_to_objclass(*class_list);
            if (!IndexOk(idx, def_oc_syms)) {
                panic("choose_classes_menu: invalid objclass '%c'",
                      *class_list);
                /*NOTREACHED*/
            }
            text = def_oc_syms[idx].explain;
            accelerator = next_accelerator;
            Sprintf(buf, "%c  %s", *class_list, text);
            break;
        default:
            panic("choose_classes_menu: invalid category %d", category);
            /*NOTREACHED*/
        }
        if (way && *class_select) { /* Selections there already */
            if (strchr(class_select, *class_list)) {
                selected = TRUE;
            }
        }
        any.a_int = *class_list;
        add_menu(win, &nul_glyphinfo, &any, accelerator,
                 category ? *class_list : 0, ATR_NONE, clr, buf,
                 selected ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
        if (category > 0) {
            if (next_accelerator == 'Z')
                break;
            else if (next_accelerator == 'z')
                next_accelerator = 'A';
            else
                ++next_accelerator;
        }
        ++class_list;
    }
    if (category == 1 && next_accelerator <= 'z') {
        /* for objects, add "A - ' '  all classes", after a separator */
        add_menu_str(win, "");
        any = cg.zeroany;
        any.a_int = (int) ' ';
        Sprintf(buf, "%c  %s", (char) any.a_int, "All classes of objects");
        /* we won't preselect this even if the incoming list is empty;
           having it selected means that it would have to be explicitly
           de-selected in order to select anything else */
        add_menu(win, &nul_glyphinfo, &any, 'A', 0,
                 ATR_NONE, clr, buf, MENU_ITEMFLAGS_SKIPINVERT);
        if (!strcmp(prompt, "Autopickup what?")) {
            add_menu_str(win,
                   "Note: when no choices are selected, \"all\" is implied.");
            /* for 'O', "toggle" should be intuitive; for 'm O', it would
               probably be better to say "Set 'autopickup' to true|false" */
            add_menu_str(win, flags.pickup
                        ? "Toggle off 'autopickup' to not pick up anything."
           : "Toggle on 'autopickup' to automatically pick these things up.");
        }
    }
    end_menu(win, prompt);
    n = select_menu(win, way ? PICK_ANY : PICK_ONE, &pick_list);
    destroy_nhwindow(win);
    if (n > 0) {
        if (category == 1) {
            /* for object classes, first check for 'all'; it means 'use
               a blank list' rather than 'collect every possible choice' */
            for (i = 0; i < n; ++i)
                if (pick_list[i].item.a_int == ' ') {
                    pick_list[0].item.a_int = ' ';
                    n = 1; /* return 1; also an implicit 'break;' */
                }
        }
        for (i = 0; i < n; ++i)
            *class_select++ = (char) pick_list[i].item.a_int;
        free((genericptr_t) pick_list);
        ret = n;
    } else if (n == -1) {
        class_select = eos(class_select);
        ret = -1;
    } else {
        ret = 0;
    }
    *class_select = '\0';
    return ret;
}

/* enum and structs are defined in wintype.h */

win_request_info zerowri = { { 0L, 0, 0, 0, 0, 0, 0, 0 },
                             { 0, 0, { NO_COLOR, ATR_NONE }}};

void
adjust_menu_promptstyle(winid window, color_attr *style)
{
    win_request_info wri = zerowri;
    wri.fromcore.menu_promptstyle.color = style->color;
    wri.fromcore.menu_promptstyle.attr = style->attr;
    /*  relay the style change to the window port */
    (void) ctrl_nhwindow(window, set_menu_promptstyle, &wri);
    go.opt_need_promptstyle = FALSE;
}

/*
 *   Common code point leading into the interface-specific
 *   add_menu() to allow single-spot adjustments to the parameters,
 *   such as those done by menu_colors.
 */
void
add_menu(
    winid window,  /* window to use, must be of type NHW_MENU */
    const glyph_info *glyphinfo, /* glyph info with glyph to
                                  * display with item */
    const anything *identifier, /* what to return if selected */
    char ch,                    /* selector letter (0 = pick our own) */
    char gch,                   /* group accelerator (0 = no group) */
    int attr,                   /* attribute for menu text (str) */
    int color,                  /* color for menu text (str) */
    const char *str,            /* menu text */
    unsigned int itemflags)     /* itemflags such as MENU_ITEMFLAGS_SELECTED */
{
    if (!str) {
        /* if 'str' is Null, just return without adding any menu entry */
        debugpline0("add_menu(Null)");
        return;
    }

    if (iflags.use_menu_color) {
        if ((itemflags & MENU_ITEMFLAGS_SKIPMENUCOLORS) == 0)
            (void) get_menu_coloring(str, &color, &attr);
    }
    /* this is the only function that cared about this flag; remove it now */
    itemflags &= ~MENU_ITEMFLAGS_SKIPMENUCOLORS;

    (*windowprocs.win_add_menu)(window, glyphinfo, identifier,
                                ch, gch, attr, color, str, itemflags);
}

/* insert a non-selectable, possibly highlighted line of text into a menu */
void
add_menu_heading(winid tmpwin, const char *buf)
{
    anything any = cg.zeroany;
    int attr = iflags.menu_headings.attr,
        color = iflags.menu_headings.color;

    /* suppress highlighting during end-of-game disclosure */
    if (program_state.gameover)
        attr = ATR_NONE, color = NO_COLOR;

    add_menu(tmpwin, &nul_glyphinfo, &any, '\0', '\0', attr, color,
             buf, MENU_ITEMFLAGS_SKIPMENUCOLORS);
}

/* insert a non-selectable, unhighlighted line of text into a menu */
void
add_menu_str(winid tmpwin, const char *buf)
{
    anything any = cg.zeroany;

    add_menu(tmpwin, &nul_glyphinfo, &any, '\0', '\0', ATR_NONE, NO_COLOR,
             buf, MENU_ITEMFLAGS_NONE);
}

staticfn boolean
get_menu_coloring(const char *str, int *color, int *attr)
{
    struct menucoloring *tmpmc;

    if (iflags.use_menu_color)
        for (tmpmc = gm.menu_colorings; tmpmc; tmpmc = tmpmc->next)
            if (regex_match(str, tmpmc->match)) {
                *color = tmpmc->color;
                *attr = tmpmc->attr;
                return TRUE;
            }
    return FALSE;
}

int select_menu(winid window, int how, menu_item **menu_list)
{
    int reslt;
    boolean old_bot_disabled = gb.bot_disabled;

    gb.bot_disabled = TRUE;
    reslt = (*windowprocs.win_select_menu)(window, how, menu_list);
    gb.bot_disabled = old_bot_disabled;
    return reslt;
}

void
getlin(const char *query, char *bufp)
{
    boolean old_bot_disabled = gb.bot_disabled;

    program_state.in_getlin = 1;
    gb.bot_disabled = TRUE;
    (*windowprocs.win_getlin)(query, bufp);
    gb.bot_disabled = old_bot_disabled;
    program_state.in_getlin = 0;
}
/*windows.c*/

/*	SCCS Id: @(#)windows.c	3.5	2007/02/01	*/
/* Copyright (c) D. Cohrs, 1993. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#ifdef TTY_GRAPHICS
#include "wintty.h"
#endif
#ifdef X11_GRAPHICS
/* cannot just blindly include winX.h without including all of X11 stuff */
/* and must get the order of include files right.  Don't bother */
extern struct window_procs X11_procs;
extern void NDECL(win_X11_init);
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
extern void NDECL(be_win_init);
#endif
#ifdef AMIGA_INTUITION
extern struct window_procs amii_procs;
extern struct window_procs amiv_procs;
extern void NDECL(ami_wininit_data);
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

STATIC_DCL void FDECL(def_raw_print, (const char *s));

#ifdef HANGUPHANDLING
volatile
#endif
NEARDATA struct window_procs windowprocs;

static
struct win_choices {
    struct window_procs *procs;
    void NDECL((*ini_routine));		/* optional (can be 0) */
} winchoices[] = {
#ifdef TTY_GRAPHICS
    { &tty_procs, win_tty_init },
#endif
#ifdef X11_GRAPHICS
    { &X11_procs, win_X11_init },
#endif
#ifdef QT_GRAPHICS
    { &Qt_procs, 0 },
#endif
#ifdef GEM_GRAPHICS
    { &Gem_procs, win_Gem_init },
#endif
#ifdef MAC
    { &mac_procs, 0 },
#endif
#ifdef BEOS_GRAPHICS
    { &beos_procs, be_win_init },
#endif
#ifdef AMIGA_INTUITION
    { &amii_procs, ami_wininit_data },		/* Old font version of the game */
    { &amiv_procs, ami_wininit_data },		/* Tile version of the game */
#endif
#ifdef WIN32_GRAPHICS
    { &win32_procs, 0 },
#endif
#ifdef GNOME_GRAPHICS
    { &Gnome_procs, 0 },
#endif
#ifdef MSWIN_GRAPHICS
    { &mswin_procs, 0 },
#endif
    { 0, 0 }		/* must be last */
};

STATIC_OVL
void
def_raw_print(s)
const char *s;
{
    puts(s);
}

void
choose_windows(s)
const char *s;
{
    register int i;

    for(i=0; winchoices[i].procs; i++)
	if (!strcmpi(s, winchoices[i].procs->name)) {
	    windowprocs = *winchoices[i].procs;
	    if (winchoices[i].ini_routine) (*winchoices[i].ini_routine)();
	    return;
	}

    if (!windowprocs.win_raw_print)
	windowprocs.win_raw_print = def_raw_print;

    raw_printf("Window type %s not recognized.  Choices are:", s);
    for(i=0; winchoices[i].procs; i++)
	raw_printf("        %s", winchoices[i].procs->name);

    if (windowprocs.win_raw_print == def_raw_print)
	terminate(EXIT_SUCCESS);
    wait_synch();
}

/*
 * tty_message_menu() provides a means to get feedback from the
 * --More-- prompt; other interfaces generally don't need that.
 */
/*ARGSUSED*/
char
genl_message_menu(let, how, mesg)
char let;
int how;
const char *mesg;
{
    pline("%s", mesg);
    return 0;
}

/*ARGSUSED*/
void
genl_preference_update(pref)
const char *pref;
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
boolean init;
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
	 return (char *)0;
}

/*ARGSUSED*/
void
genl_putmsghistory(msg)
const char *msg;
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
	return;
}

#ifdef HANGUPHANDLING
    /*
     * Dummy windowing scheme used to replace current one with no-ops
     * in order to avoid all terminal I/O after hangup/disconnect.
     */

static int NDECL(hup_nhgetch);
static char FDECL(hup_yn_function, (const char *,const char *,CHAR_P));
static int FDECL(hup_nh_poskey, (int *,int *,int *));
static void FDECL(hup_getlin, (const char *,char *));
static void FDECL(hup_init_nhwindows, (int *,char **));
static void FDECL(hup_exit_nhwindows, (const char *));
static winid FDECL(hup_create_nhwindow, (int));
static int FDECL(hup_select_menu, (winid,int,MENU_ITEM_P **));
static void FDECL(hup_add_menu, (winid,int,const anything *,CHAR_P,CHAR_P,
				 int,const char *,BOOLEAN_P));
static void FDECL(hup_end_menu, (winid,const char *));
static void FDECL(hup_putstr, (winid,int,const char *));
static void FDECL(hup_print_glyph, (winid,XCHAR_P,XCHAR_P,int));
static void FDECL(hup_outrip, (winid,int));
static void FDECL(hup_curs, (winid,int,int));
static void FDECL(hup_display_nhwindow, (winid,BOOLEAN_P));
static void FDECL(hup_display_file, (const char *,BOOLEAN_P));
# ifdef CLIPPING
static void FDECL(hup_cliparound, (int,int));
# endif
# ifdef CHANGE_COLOR
static void FDECL(hup_change_color, (int,long,int));
#  ifdef MAC
static short FDECL(hup_set_font_name, (winid,char *));
#  endif
static char *NDECL(hup_get_color_string);
# endif /* CHANGE_COLOR */
# ifdef STATUS_VIA_WINDOWPORT
static void FDECL(hup_status_update, (int,genericptr_t,int,int));
# endif

static int NDECL(hup_int_ndecl);
static void NDECL(hup_void_ndecl);
static void FDECL(hup_void_fdecl_int, (int));
static void FDECL(hup_void_fdecl_winid, (winid));
static void FDECL(hup_void_fdecl_constchar_p, (const char *));

static struct window_procs hup_procs = {
    "hup",
    0L,
    0L,
    hup_init_nhwindows,
    hup_void_ndecl,		/* player_selection */
    hup_void_ndecl,		/* askname */
    hup_void_ndecl,		/* get_nh_event */
    hup_exit_nhwindows,
    hup_void_fdecl_constchar_p, /* suspend_nhwindows */
    hup_void_ndecl,		/* resume_nhwindows */
    hup_create_nhwindow,
    hup_void_fdecl_winid,	/* clear_nhwindow */
    hup_display_nhwindow,
    hup_void_fdecl_winid,	/* destroy_nhwindow */
    hup_curs,
    hup_putstr,
    hup_putstr,			/* putmixed */
    hup_display_file,
    hup_void_fdecl_winid,	/* start_menu */
    hup_add_menu,
    hup_end_menu,
    hup_select_menu,
    genl_message_menu,
    hup_void_ndecl,		/* update_inventory */
    hup_void_ndecl,		/* mark_synch */
    hup_void_ndecl,		/* wait_synch */
# ifdef CLIPPING
    hup_cliparound,
# endif
# ifdef POSITIONBAR
    (void FDECL((*),(char *)))hup_void_fdecl_constchar_p, /* update_positionbar */
# endif
    hup_print_glyph,
    hup_void_fdecl_constchar_p,	/* raw_print */
    hup_void_fdecl_constchar_p,	/* raw_print_bold */
    hup_nhgetch,
    hup_nh_poskey,
    hup_void_ndecl,		/* nhbell  */
    hup_int_ndecl,		/* doprev_message */
    hup_yn_function,
    hup_getlin,
    hup_int_ndecl,		/* get_ext_cmd */
    hup_void_fdecl_int,		/* number_pad */
    hup_void_ndecl,		/* delay_output  */
# ifdef CHANGE_COLOR
    hup_change_color,
#  ifdef MAC
    hup_void_fdecl_int,		/* change_background */
    hup_set_font_name,
#  endif
    hup_get_color_string,
# endif /* CHANGE_COLOR */
    hup_void_ndecl,		/* start_screen */
    hup_void_ndecl,		/* end_screen */
    hup_outrip,
    genl_preference_update,
    genl_getmsghistory,
    genl_putmsghistory,
# ifdef STATUS_VIA_WINDOWPORT
    hup_void_ndecl,		/* status_init */
    hup_void_ndecl,		/* status_finish */
    genl_status_enablefield,
    hup_status_update,
#  ifdef STATUS_HILITES
    genl_status_threshold,
#  endif
# endif /* STATUS_VIA_WINDOWPORT */
};

static void FDECL((*previnterface_exit_nhwindows), (const char *)) = 0;

/* hangup has occurred; switch to no-op user interface */
void
nhwindows_hangup()
{
    /* don't call exit_nhwindows() directly here; if a hangup occurs
       while interface code is executing, exit_nhwindows could knock
       the interface's active data structures out from under itself */
    if (iflags.window_inited)
	previnterface_exit_nhwindows = exit_nhwindows;
    windowprocs = hup_procs;
}

static void
hup_exit_nhwindows(lastgasp)
const char *lastgasp;
{
    /* core has called exit_nhwindows(); call the previous interface's
       shutdown routine now; xxx_exit_nhwindows() needs to call other
       xxx_ routines directly rather than through windowprocs pointers */
    if (previnterface_exit_nhwindows) {
	lastgasp = 0;	/* don't want exit routine to attempt extra output */
	(*previnterface_exit_nhwindows)(lastgasp);
	previnterface_exit_nhwindows = 0;
    }
    iflags.window_inited = 0;
}

static int
hup_nhgetch( VOID_ARGS )
{
    return '\033';	/* ESC */
}

/*ARGSUSED*/
static char
hup_yn_function(prompt, resp, deflt)
const char *prompt, *resp;
char deflt;
{
    if (!deflt) deflt = '\033';
    return deflt;
}

/*ARGSUSED*/
static int
hup_nh_poskey(x, y, mod)
int *x, *y, *mod;
{
    return '\033';
}

/*ARGSUSED*/
static void
hup_getlin(prompt, outbuf)
const char *prompt;
char *outbuf;
{
    Strcpy(outbuf, "\033");
}

/*ARGUSED*/
static void
hup_init_nhwindows(argc_p, argv)
int *argc_p;
char **argv;
{
    iflags.window_inited = 1;
}

/*ARGSUSED*/
static winid
hup_create_nhwindow(type)
int type;
{
    return WIN_ERR;
}

/*ARGSUSED*/
static int
hup_select_menu(window, how, menu_list)
winid window;
int how;
struct mi **menu_list;
{
    return -1;
}

/*ARGSUSED*/
static void
hup_add_menu(window, glyph, identifier, sel, grpsel, attr, txt, preselected)
winid window;
int glyph, attr;
const anything *identifier;
char sel, grpsel;
const char *txt;
boolean preselected;
{
    return;
}

/*ARGSUSED*/
static void
hup_end_menu(window, prompt)
winid window;
const char *prompt;
{
    return;
}

/*ARGSUSED*/
static void
hup_putstr(window, attr, text)
winid window;
int attr;
const char *text;
{
    return;
}

/*ARGSUSED*/
static void
hup_print_glyph(window, x, y, glyph)
winid window;
xchar x, y;
int glyph;
{
    return;
}

/*ARGSUSED*/
static void
hup_outrip(tmpwin, how)
winid tmpwin;
int how;
{
    return;
}

/*ARGSUSED*/
static void
hup_curs(window, x, y)
winid window;
int x, y;
{
    return;
}

/*ARGSUSED*/
static void
hup_display_nhwindow(window, blocking)
winid window;
boolean blocking;
{
    return;
}

/*ARGSUSED*/
static void
hup_display_file(fname, complain)
const char *fname;
boolean complain;
{
    return;
}

# ifdef CLIPPING
/*ARGSUSED*/
static void
hup_cliparound(x, y)
int x, y;
{
    return;
}
# endif

# ifdef CHANGE_COLOR
/*ARGSUSED*/
static void
hup_change_color(color, rgb, reverse)
int color, reverse;
long rgb;
{
    return;
}

#  ifdef MAC
/*ARGSUSED*/
static short
hup_set_font_name(window, fontname)
winid window;
char *fontname;
{
    return 0;
}
#  endif /* MAC */

static char *
hup_get_color_string(VOID_ARGS)
{
    return (char *)0;
}
# endif /* CHANGE_COLOR */

# ifdef STATUS_VIA_WINDOWPORT
/*ARGSUSED*/
static void
hup_status_update(idx, ptr, chg, percent)
int idx, chg, percent;
genericptr_t ptr;
{
    return;
}
# endif /* STATUS_VIA_WINDOWPORT */

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
int arg;
{
    return;
}

/*ARGUSED*/
static void
hup_void_fdecl_winid(window)
winid window;
{
    return;
}

/*ARGUSED*/
static void
hup_void_fdecl_constchar_p(string)
const char *string;
{
    return;
}

#endif /* HANGUPHANDLING */

/*windows.c*/

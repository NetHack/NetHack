/* NetHack 3.7	safeproc.c   */	
/* Copyright (c) Michael Allison, 2018                            */
/* NetHack may be freely redistributed.  See license for details. */

/* must #define SAFEPROCS in xxxconf.h or via CFLAGS or this won't compile */
#include "hack.h"

/*
 * ***********************************************************
 * This is a complete WindowPort implementation that can be
 * assigned to the windowproc function pointers very early
 * in the startup initialization, perhaps immediately even.
 * It requires only the following call:
 *          windowprocs = *get_safe_procs(0);
 *
 * The game startup can trigger functions in other modules
 * that make assumptions on a WindowPort being available
 * and bad things can happen if any function pointers are
 * null at that time.
 *
 * Some ports prior to 3.6.2 made attempts to early init
 * various pieces of one of their WindowPorts, but that
 * caused conflicts if that particular WindowPort wasn't
 * the one that the user ended up selecting in their
 * config file later. The WindowPort interfaced was designed
 * to allow multiple WindowPorts to be linked into the same
 * game binary.
 *
 * The base functions established by a call to get_safe_procs()
 * accomplish the goal of preventing crashes, but not much
 * else.
 * 
 * There are also a few additional functions provided in here
 * that can be selected optionally to provide some startup
 * functionality for getting messages out to the user about
 * issues that are being experienced during startup in
 * general or during options parsing. The ones in here are
 * deliberately free from any platforms or OS specific code.
 * Please leave them using stdio C routines as much as
 * possible.  That isn't to say you can't do fancier functions
 * prior to initialization of the primary WindowPort, but you
 * can provide those platform-specific functions elsewhere,
 * and assign them the same way that these more generic versions
 * are assigned.
 *
 * The additional platform-independent, but more functional
 * routines provided in here should be assigned after the
 *    windowprocs = *get_safe_procs(n)
 * call.
 *
 *  Usage:
 *   
 *    windowprocs = *get_safe_procs(0);
 *   initializes a set of winprocs function pointers that ensure
 *   none of the function pointers are left null, but that's all
 *   it does.
 *   
 *    windowprocs = *get_safe_procs(1);
 *   initializes a set of winprocs functions pointers that ensure
 *   none of the function pointers are left null, but also
 *   provides some basic output and input functionality using
 *   nothing other than C stdio routines (no platform-specific
 *   or OS-specific code).
 *
 * ***********************************************************
 */

struct window_procs safe_procs = {
    "safe-startup", 0L, 0L,
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   /* color availability */
    safe_init_nhwindows, safe_player_selection, safe_askname, safe_get_nh_event,
    safe_exit_nhwindows, safe_suspend_nhwindows, safe_resume_nhwindows,
    safe_create_nhwindow, safe_clear_nhwindow, safe_display_nhwindow,
    safe_destroy_nhwindow, safe_curs, safe_putstr, safe_putmixed,
    safe_display_file, safe_start_menu, safe_add_menu, safe_end_menu,
    safe_select_menu, safe_message_menu, safe_update_inventory, safe_mark_synch,
    safe_wait_synch,
#ifdef CLIPPING
    safe_cliparound,
#endif
#ifdef POSITIONBAR
    safe_update_positionbar,
#endif
    safe_print_glyph, safe_raw_print, safe_raw_print_bold, safe_nhgetch,
    safe_nh_poskey, safe_nhbell, safe_doprev_message, safe_yn_function,
    safe_getlin, safe_get_ext_cmd, safe_number_pad, safe_delay_output,
#ifdef CHANGE_COLOR /* the Mac uses a palette device */
    safe_change_color,
#ifdef MAC
    safe_change_background, set_safe_font_name,
#endif
    safe_get_color_string,
#endif
    safe_start_screen, safe_end_screen, safe_outrip,
    safe_preference_update,
    safe_getmsghistory, safe_putmsghistory,
    safe_status_init,
    safe_status_finish, safe_status_enablefield,
    safe_status_update,
    safe_can_suspend,
};

struct window_procs *
get_safe_procs(int optn)
{
    if (optn) {
        /* include the slightly more functional stdc versions */
        safe_procs.win_raw_print = stdio_raw_print;
        safe_procs.win_raw_print_bold = stdio_raw_print_bold;
        safe_procs.win_nhgetch = stdio_nhgetch;
        safe_procs.win_wait_synch = stdio_wait_synch;
        if (optn == 2)
            safe_procs.win_raw_print = stdio_nonl_raw_print;        
    }
    return &safe_procs;
}

/*ARGSUSED*/
void
safe_init_nhwindows(int *argcp UNUSED, char **argv UNUSED)
{
    return;
}

void
safe_player_selection(void)
{
    return;
}

void
safe_askname(void)
{
    return;
}

void
safe_get_nh_event(void)
{
    return;
}

void
safe_suspend_nhwindows(const char *str)
{
    return;
}

void
safe_resume_nhwindows(void)
{
    return;
}

void
safe_exit_nhwindows(const char *str)
{
    return;
}

winid
safe_create_nhwindow(int type)
{
    return WIN_ERR;
}

void
safe_clear_nhwindow(winid window)
{
    return;
}

/*ARGSUSED*/
void
safe_display_nhwindow(winid window,boolean blocking)
{
    return;
}

void
safe_dismiss_nhwindow(winid window)
{
    return;
}

void
safe_destroy_nhwindow(winid window)
{
    return;
}

void
safe_curs(winid window, int x, int y)
{
    return;
}

void
safe_putstr(winid window, int attr, const char *str)
{
    return;
}

void
safe_putmixed(winid window, int attr, const char *str)
{
    return;
}

void
safe_display_file(const char * fname, boolean complain)
{
    return;
}

void
safe_start_menu(winid window, unsigned long mbehavior)
{
    return;
}

/*ARGSUSED*/
/*
 * Add a menu item to the beginning of the menu list.  This list is reversed
 * later.
 */
void
safe_add_menu(
    winid window,               /* window to use, must be of type NHW_MENU */
    const glyph_info *glyphinfo UNUSED, /* glyph plus glyph info */
    const anything *identifier, /* what to return if selected */
    char ch,                    /* keyboard accelerator (0 = pick our own) */
    char gch,                   /* group accelerator (0 = no group) */
    int attr,                   /* attribute for string (like safe_putstr()) */
    const char *str,            /* menu string */
    unsigned int itemflags)     /* itemflags such as marked as selected */
{
    return;
}

/*
 * End a menu in this window, window must a type NHW_MENU.
 */
void
safe_end_menu(
    winid window,       /* menu to use */
    const char *prompt) /* prompt to for menu */
{
    return;
}

int
safe_select_menu(
    winid window,
    int how,
    menu_item **menu_list)
{
    return 0;
}

/* special hack for treating top line --More-- as a one item menu */
char
safe_message_menu(
    char let,
    int how,
    const char *mesg)
{
    return '\033';
}

void
safe_update_inventory(void)
{
    return;
}

void
safe_mark_synch(void)
{
}

void
safe_wait_synch(void)
{
}

#ifdef CLIPPING
void
safe_cliparound(int x, int y)
{
}
#endif /* CLIPPING */

/*
 *  safe_print_glyph
 *
 *  Print the glyph to the output device.  Don't flush the output device.
 */
void
safe_print_glyph(
    winid window UNUSED,
    xchar x UNUSED,
    xchar y UNUSED,
    const glyph_info *glyphinfo UNUSED,
    const glyph_info *bkglyphinfo UNUSED)
{
    return;
}

void
safe_raw_print(const char *str)
{
    return;
}

void
safe_raw_print_bold(const char *str)
{
    return;
}

int
safe_nhgetch(void)
{
    return '\033';
}

/*
 * return a key, or 0, in which case a mouse button was pressed
 * mouse events should be returned as character postitions in the map window.
 * Since normal tty's don't have mice, just return a key.
 */
/*ARGSUSED*/
int
safe_nh_poskey(int *x, int *y, int *mod)
{
    return '\033';
}

void
win_safe_init(int dir)
{
    return;
}

#ifdef POSITIONBAR
void
safe_update_positionbar(char *posbar)
{
    return;
}
#endif /* POSITIONBAR */

/*
 *  safe_status_init()
 *      -- initialize the port-specific data structures.
 */
void
safe_status_init(void)
{
    return;
}

boolean
safe_can_suspend(void)
{
    return FALSE;
}

void
safe_nhbell(void)
{
    return;
}

int
safe_doprev_message(void)
{
    return 0;
}

char
safe_yn_function(const char * query, const char* resp, char def)
{
    return '\033';
}

/*ARGSUSED*/
void
safe_getlin(const char* prompt UNUSED, char *outbuf)
{
    Strcpy(outbuf, "\033");
}

int
safe_get_ext_cmd(void)
{
    return '\033';
}

void
safe_number_pad(int mode)
{
    return;
}

void
safe_delay_output(void)
{
    return;
}

void
safe_start_screen(void)
{
    return;
}

void
safe_end_screen(void)
{
    return;
}

void
safe_outrip(winid tmpwin, int how, time_t when)
{
    return;
}

/*ARGSUSED*/
void
safe_preference_update(const char* pref UNUSED)
{
    return;
}

char *
safe_getmsghistory(boolean init UNUSED)
{
    return (char *) 0;
}

void
safe_putmsghistory(
    const char *msg,
    boolean is_restoring)
{
}

void
safe_status_finish(void)
{
}

void
safe_status_enablefield(
    int fieldidx,
    const char *nm,
    const char *fmt,
    boolean enable)
{
}

/* call once for each field, then call with BL_FLUSH to output the result */
void
safe_status_update(
    int idx,
    genericptr_t ptr,
    int chg UNUSED,
    int percent UNUSED, 
    int color UNUSED,
    unsigned long *colormasks UNUSED)
{
}

/**************************************************************
 * These are some optionally selectable routines that add
 * some base functionality over the safe_* versions above.
 * The safe_* versions are primarily designed to ensure that
 * there are no null function pointers remaining at early
 * game startup/initialization time.
 *
 * The slightly more functional versions in here should be kept
 * free of platform-specific code or OS-specific code. If you
 * want to use versions that involve platform-specific or
 * OS-specific code, go right ahead but use your own replacement
 * version of the functions in a platform-specific or
 * OS-specific source file, not in here.
 ***************************************************************/

/* Add to your code: windowprocs.win_raw_print = stdio_wait_synch; */
void
stdio_wait_synch(void)
{
    char valid[] = {' ', '\n', '\r', '\033', '\0'};

    fprintf(stdout, "--More--");
    (void) fflush(stdout);
    while (!index(valid, nhgetch()))
        ;
}

/* Add to your code: windowprocs.win_raw_print = stdio_raw_print; */
void
stdio_raw_print(const char* str)
{
    if (str)
        fprintf(stdout, "%s\n", str);
    return;
}

/* no newline variation, add to your code:
    windowprocs.win_raw_print = stdio_nonl_raw_print;  */
void
stdio_nonl_raw_print(const char* str)
{
    if (str)
        fprintf(stdout, "%s", str);
    return;
}

/* Add to your code: windowprocs.win_raw_print_bold = stdio_raw_print_bold; */
void
stdio_raw_print_bold(const char* str)
{
    stdio_raw_print(str);
    return;
}

/* Add to your code: windowprocs.win_nhgetch = stdio_nhgetch; */
int
stdio_nhgetch(void)
{
    return getchar();
}


/* safeprocs.c */

/* NetHack 3.7	winmisc.c	$NHDT-Date: 1596498374 2020/08/03 23:46:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.49 $ */
/* Copyright (c) Dean Luick, 1992                                 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Misc. popup windows: player selection and extended commands.
 *
 *      + Global functions: player_selection() and get_ext_cmd().
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV /* X11 include files may define SYSV */
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/List.h>
#include <X11/Xos.h> /* for strchr() */
#include <X11/Xatom.h>

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#define X11_BUILD
#include "hack.h"
#undef X11_BUILD

#include "func_tab.h"
#include "winX.h"

static Widget extended_command_popup = 0;
static Widget extended_command_form;
static Widget *extended_commands = 0;
static const char **command_list;
static short *command_indx;
static int extended_cmd_selected; /* index of the selected command; */
static int ps_selected;               /* index of selected role */
#define PS_RANDOM (-50)
#define PS_QUIT (-75)
/* 'r' for random won't work for role but will for race, gender, alignment */
static const char ps_randchars[] = "*@\n\rrR";
static const char ps_quitchars[] = "\033qQ";

#define EC_NCHARS 32
static boolean ec_full_list = FALSE;
static boolean ec_active = FALSE;
static int ec_nchars = 0;
static char ec_chars[EC_NCHARS];
static Time ec_time;

boolean plsel_ask_name;

static const char plsel_dialog_translations[] = "#override\n\
     <Key>Escape: plsel_quit()\n\
     <Key>Return: plsel_play()";

static const char plsel_input_accelerators[] = "#override\n\
     <Key>Escape: plsel_quit()\n\
     <Key>Return: plsel_play()\n\
     <Key>Tab: \n";

static const char extended_command_translations[] = "#override\n\
     <Key>Left: scroll(4)\n\
     <Key>Right: scroll(6)\n\
     <Key>Up: scroll(8)\n\
     <Key>Down: scroll(2)\n\
     <Btn4Down>: scroll(8)\n\
     <Btn5Down>: scroll(2)\n\
     <Key>: ec_key()";

static const char player_select_translations[] = "#override\n\
     <Key>: ps_key()";
static const char race_select_translations[] = "#override\n\
     <Key>: race_key()";
static const char gend_select_translations[] = "#override\n\
     <Key>: gend_key()";
static const char algn_select_translations[] = "#override\n\
     <Key>: algn_key()";

static const char popup_entry_translations[] = "#override\n\
     <Btn4Down>: scroll(8)\n\
     <Btn5Down>: scroll(2)";

static void plsel_dialog_acceptvalues(void);
static void plsel_set_play_button(boolean);
static void plsel_set_sensitivities(boolean);
static void X11_player_selection_randomize(void);
static void X11_player_selection_setupOthers(void);
static void racetoggleCallback(Widget, XtPointer, XtPointer);
static void roletoggleCallback(Widget, XtPointer, XtPointer);
static void gendertoggleCallback(Widget, XtPointer, XtPointer);
static void aligntoggleCallback(Widget, XtPointer, XtPointer);
static void plsel_random_btn_callback(Widget, XtPointer, XtPointer);
static void plsel_play_btn_callback(Widget, XtPointer, XtPointer);
static void plsel_quit_btn_callback(Widget, XtPointer, XtPointer);
static Widget X11_create_player_selection_name(Widget);
static void X11_player_selection_dialog(void);
static void X11_player_selection_prompts(void);
static void ps_quit(Widget, XtPointer, XtPointer);
static void ps_random(Widget, XtPointer, XtPointer);
static void ps_select(Widget, XtPointer, XtPointer);
static void extend_select(Widget, XtPointer, XtPointer);
static void extend_dismiss(Widget, XtPointer, XtPointer);
static void extend_help(Widget, XtPointer, XtPointer);
static void popup_delete(Widget, XEvent *, String *, Cardinal *);
static void ec_dismiss(void);
static void ec_scroll_to_view(int);
static void init_extended_commands_popup(void);
static Widget make_menu(const char *, const char *, const char *, const char *,
                        XtCallbackProc, const char *, XtCallbackProc, int,
                        const char **, Widget **, XtCallbackProc, Widget *);

/* Bad Hack alert. Using integers instead of XtPointers */
XtPointer
i2xtp(int i)
{
    return (XtPointer) (ptrdiff_t) i;
}

int
xtp2i(XtPointer x)
{
    return (int) (ptrdiff_t) x;
}

/* Player Selection ------------------------------------------------------- */
/* ARGSUSED */
static void
ps_quit(Widget w, XtPointer client_data, XtPointer call_data)
{
    nhUse(w);
    nhUse(client_data);
    nhUse(call_data);

    ps_selected = PS_QUIT;
    exit_x_event = TRUE; /* leave event loop */
}

/* ARGSUSED */
static void
ps_random(Widget w, XtPointer client_data, XtPointer call_data)
{
    nhUse(w);
    nhUse(client_data);
    nhUse(call_data);

    ps_selected = PS_RANDOM;
    exit_x_event = TRUE; /* leave event loop */
}

/* ARGSUSED */
static void
ps_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    nhUse(w);
    nhUse(call_data);

    ps_selected = (int) (ptrdiff_t) client_data;
    exit_x_event = TRUE; /* leave event loop */
}

/* ARGSUSED */
void
ps_key(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    char ch, *mark;
    char rolechars[QBUFSZ];
    int i;

    nhUse(w);
    nhUse(params);
    nhUse(num_params);

    (void) memset(rolechars, '\0', sizeof rolechars); /* for strchr() */
    for (i = 0; roles[i].name.m; ++i) {
        ch = lowc(*roles[i].name.m);
        /* if (flags.female && roles[i].name.f) ch = lowc(*roles[i].name.f);
         */
        /* this supports at most two roles with the same first letter */
        if (strchr(rolechars, ch))
            ch = highc(ch);
        rolechars[i] = ch;
    }
    ch = key_event_to_char((XKeyEvent *) event);
    if (ch == '\0') { /* don't accept nul char/modifier event */
        /* don't beep */
        return;
    }
    mark = strchr(rolechars, ch);
    if (!mark)
        mark = strchr(rolechars, lowc(ch));
    if (!mark)
        mark = strchr(rolechars, highc(ch));
    if (!mark) {
        if (strchr(ps_randchars, ch))
            ps_selected = PS_RANDOM;
        else if (strchr(ps_quitchars, ch))
            ps_selected = PS_QUIT;
        else {
            X11_nhbell(); /* no such class */
            return;
        }
    } else
        ps_selected = (int) (mark - rolechars);
    exit_x_event = TRUE;
}

/* ARGSUSED */
void
race_key(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    char ch, *mark;
    char racechars[QBUFSZ];
    int i;

    nhUse(w);
    nhUse(params);
    nhUse(num_params);

    (void) memset(racechars, '\0', sizeof racechars); /* for strchr() */
    for (i = 0; races[i].noun; ++i) {
        ch = lowc(*races[i].noun);
        /* this supports at most two races with the same first letter */
        if (strchr(racechars, ch))
            ch = highc(ch);
        racechars[i] = ch;
    }
    ch = key_event_to_char((XKeyEvent *) event);
    if (ch == '\0') { /* don't accept nul char/modifier event */
        /* don't beep */
        return;
    }
    mark = strchr(racechars, ch);
    if (!mark)
        mark = strchr(racechars, lowc(ch));
    if (!mark)
        mark = strchr(racechars, highc(ch));
    if (!mark) {
        if (strchr(ps_randchars, ch))
            ps_selected = PS_RANDOM;
        else if (strchr(ps_quitchars, ch))
            ps_selected = PS_QUIT;
        else {
            X11_nhbell(); /* no such race */
            return;
        }
    } else
        ps_selected = (int) (mark - racechars);
    exit_x_event = TRUE;
}

/* ARGSUSED */
void
gend_key(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    char ch, *mark;
    static char gendchars[] = "mf";

    nhUse(w);
    nhUse(params);
    nhUse(num_params);

    ch = key_event_to_char((XKeyEvent *) event);
    if (ch == '\0') { /* don't accept nul char/modifier event */
        /* don't beep */
        return;
    }
    mark = strchr(gendchars, ch);
    if (!mark)
        mark = strchr(gendchars, lowc(ch));
    if (!mark) {
        if (strchr(ps_randchars, ch))
            ps_selected = PS_RANDOM;
        else if (strchr(ps_quitchars, ch))
            ps_selected = PS_QUIT;
        else {
            X11_nhbell(); /* no such gender */
            return;
        }
    } else
        ps_selected = (int) (mark - gendchars);
    exit_x_event = TRUE;
}

/* ARGSUSED */
void
algn_key(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    char ch, *mark;
    static char algnchars[] = "LNC";

    nhUse(w);
    nhUse(params);
    nhUse(num_params);

    ch = key_event_to_char((XKeyEvent *) event);
    if (ch == '\0') { /* don't accept nul char/modifier event */
        /* don't beep */
        return;
    }
    mark = strchr(algnchars, ch);
    if (!mark)
        mark = strchr(algnchars, highc(ch));
    if (!mark) {
        if (strchr(ps_randchars, ch))
            ps_selected = PS_RANDOM;
        else if (strchr(ps_quitchars, ch))
            ps_selected = PS_QUIT;
        else {
            X11_nhbell(); /* no such alignment */
            return;
        }
    } else
        ps_selected = (int) (mark - algnchars);
    exit_x_event = TRUE;
}

int plsel_n_races, plsel_n_roles;
Widget *plsel_race_radios = (Widget *) 0;
Widget *plsel_role_radios = (Widget *) 0;
Widget *plsel_gend_radios = (Widget *) 0;
Widget *plsel_align_radios = (Widget *) 0;

Widget plsel_name_input;

Widget plsel_btn_play;

static void
plsel_dialog_acceptvalues(void)
{
    Arg args[2];
    String s;

    flags.initrace = xtp2i(XawToggleGetCurrent(plsel_race_radios[0])) - 1;
    flags.initrole = xtp2i(XawToggleGetCurrent(plsel_role_radios[0])) - 1;
    flags.initgend = xtp2i(XawToggleGetCurrent(plsel_gend_radios[0])) - 1;
    flags.initalign = xtp2i(XawToggleGetCurrent(plsel_align_radios[0])) - 1;

    XtSetArg(args[0], nhStr(XtNstring), &s);
    XtGetValues(plsel_name_input, args, ONE);

    (void) strncpy(gp.plname, (char *) s, sizeof gp.plname - 1);
    gp.plname[sizeof gp.plname - 1] = '\0';
    (void) mungspaces(gp.plname);
    if (strlen(gp.plname) < 1)
        (void) strcpy(gp.plname, "Mumbles");
    iflags.renameinprogress = FALSE;
}

/* ARGSUSED */
void
plsel_quit(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    nhUse(w);
    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    ps_selected = PS_QUIT;
    exit_x_event = TRUE; /* leave event loop */
}

/* ARGSUSED */
void
plsel_play(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Arg args[2];
    Boolean state;

    nhUse(w);
    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    XtSetArg(args[0], nhStr(XtNsensitive), &state);
    XtGetValues(plsel_btn_play, args, ONE);

    if (state) {
        plsel_dialog_acceptvalues();
        exit_x_event = TRUE; /* leave event loop */
    } else {
        X11_nhbell();
    }
}

/* ARGSUSED */
void
plsel_randomize(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    nhUse(w);
    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    X11_player_selection_randomize();
}

/* enable or disable the Play button */
static void
plsel_set_play_button(boolean state)
{
    Arg args[2];

    XtSetArg(args[0], nhStr(XtNsensitive), !state);
    XtSetValues(plsel_btn_play, args, ONE);
}

static void
plsel_set_sensitivities(boolean setcurr)
{
    Arg args[2];
    int j, valid;
    int c = 0;
    int ra = xtp2i(XawToggleGetCurrent(plsel_race_radios[0]))-1;
    int ro = xtp2i(XawToggleGetCurrent(plsel_role_radios[0]))-1;

    plsel_set_play_button(ra < 0 || ro < 0);

    if (ra < 0 || ro < 0)
        return;

    valid = -1;

    for (j = 0; roles[j].name.m; j++) {
        boolean v = validrace(j, ra);

        if (j == ro)
            c = j;
        XtSetArg(args[0], nhStr(XtNsensitive), v);
        XtSetValues(plsel_role_radios[j], args, ONE);
        if (valid < 0 && v)
            valid = j;
    }
    if (!validrace(ro, c))
        c = valid;

    if (setcurr)
        XawToggleSetCurrent(plsel_role_radios[0], i2xtp(c + 1));

    valid = -1;

    for (j = 0; races[j].noun; j++) {
        boolean v = validrace(ro, j);

        if (j == ra)
            c = j;
        XtSetArg(args[0], nhStr(XtNsensitive), v);
        XtSetValues(plsel_race_radios[j], args, ONE);
        if (valid < 0 && v)
            valid = j;
    }
    if (!validrace(ro, c))
        c = valid;

    if (setcurr)
        XawToggleSetCurrent(plsel_race_radios[0], i2xtp(c + 1));

    X11_player_selection_setupOthers();
}

static void
X11_player_selection_randomize(void)
{
    int nrole = plsel_n_roles;
    int nrace = plsel_n_races;
    int ro, ra, al, gend;
    boolean choose_race_first;
    boolean picksomething = (flags.initrole == ROLE_NONE
                             || flags.initrace == ROLE_NONE
                             || flags.initgend == ROLE_NONE
                             || flags.initalign == ROLE_NONE);

    if (flags.randomall && picksomething) {
        if (flags.initrole == ROLE_NONE)
            flags.initrole = ROLE_RANDOM;
        if (flags.initrace == ROLE_NONE)
            flags.initrace = ROLE_RANDOM;
        if (flags.initgend == ROLE_NONE)
            flags.initgend = ROLE_RANDOM;
        if (flags.initalign == ROLE_NONE)
            flags.initalign = ROLE_RANDOM;
    }

    rigid_role_checks();

    /* Randomize race and role, unless specified in config */
    ro = flags.initrole;
    if (ro == ROLE_NONE || ro == ROLE_RANDOM) {
        ro = rn2(nrole);
    }
    ra = flags.initrace;
    if (ra == ROLE_NONE || ra == ROLE_RANDOM) {
        ra = rn2(nrace);
        if (flags.initrace != ROLE_RANDOM) {
        }
    }

    /* make sure we have a valid combination, honoring
       the users request if possible. */
    choose_race_first = FALSE;
    if (flags.initrace >= 0 && flags.initrole < 0) {
        choose_race_first = TRUE;
    }

    while (!validrace(ro, ra)) {
        if (choose_race_first) {
            ro = rn2(nrole);
        } else {
            ra = rn2(nrace);
        }
    }

    gend = flags.initgend;
    if (gend == ROLE_NONE) {
        gend = rn2(ROLE_GENDERS);
    }
    while (!validgend(ro, ra, gend)) {
        gend = rn2(ROLE_GENDERS);
    }

    al = flags.initalign;
    if (al == ROLE_NONE) {
        al = rn2(ROLE_ALIGNS);
    }
    while (!validalign(ro, ra, al)) {
        al = rn2(ROLE_ALIGNS);
    }

    XawToggleSetCurrent(plsel_gend_radios[0], i2xtp(gend + 1));
    XawToggleSetCurrent(plsel_align_radios[0], i2xtp(al + 1));
    XawToggleSetCurrent(plsel_race_radios[0], i2xtp(ra + 1));
    XawToggleSetCurrent(plsel_role_radios[0], i2xtp(ro + 1));
    plsel_set_sensitivities(FALSE);
}

static void
X11_player_selection_setupOthers(void)
{
    Arg args[2];
    int ra = xtp2i(XawToggleGetCurrent(plsel_race_radios[0])) - 1;
    int ro = xtp2i(XawToggleGetCurrent(plsel_role_radios[0])) - 1;
    int valid = -1, c = 0, j;
    int gchecked = xtp2i(XawToggleGetCurrent(plsel_gend_radios[0])) - 1;
    int achecked = xtp2i(XawToggleGetCurrent(plsel_align_radios[0])) - 1;

    if (ro < 0 || ra < 0)
        return;

    for (j = 0; j < ROLE_GENDERS; j++) {
        boolean v = validgend(ro, ra, j);

        if (j == gchecked)
            c = j;
        XtSetArg(args[0], nhStr(XtNsensitive), v);
        XtSetValues(plsel_gend_radios[j], args, ONE);
        if (valid < 0 && v)
            valid = j;
    }
    if (!validgend(ro, ra, c))
        c = valid;

    XawToggleSetCurrent(plsel_gend_radios[0], i2xtp(c + 1));

    valid = -1;

    for (j = 0; j < ROLE_ALIGNS; j++) {
        boolean v = validalign(ro, ra, j);

        if (j == achecked)
            c = j;
        XtSetArg(args[0], nhStr(XtNsensitive), v);
        XtSetValues(plsel_align_radios[j], args, ONE);
        if (valid < 0 && v)
            valid = j;
    }
    if (!validalign(ro, ra, c))
        c = valid;

    XawToggleSetCurrent(plsel_align_radios[0], i2xtp(c + 1));
}

static void
racetoggleCallback(Widget w, XtPointer client, XtPointer call)
{
    Arg args[2];
    int j, valid;
    int c = 0;
    int ra = xtp2i(XawToggleGetCurrent(plsel_race_radios[0])) - 1;
    int ro = xtp2i(XawToggleGetCurrent(plsel_role_radios[0])) - 1;

    nhUse(w);
    nhUse(client);
    nhUse(call);

    plsel_set_play_button(ra < 0 || ro < 0);

    if (ra < 0 || ro < 0)
        return;

    valid = -1;

    for (j = 0; roles[j].name.m; j++) {
        boolean v = validrace(j, ra);

        if (j == ro)
            c = j;
        XtSetArg(args[0], nhStr(XtNsensitive), v);
        XtSetValues(plsel_role_radios[j], args, ONE);
        if (valid < 0 && v)
            valid = j;
    }
    if (!validrace(c, ra))
        c = valid;

    j = c + 1;
    XawToggleSetCurrent(plsel_role_radios[0], i2xtp(j));

    X11_player_selection_setupOthers();
}

static void
roletoggleCallback(Widget w, XtPointer client, XtPointer call)
{
    Arg args[2];
    int j, valid;
    int c = 0;
    int ra = xtp2i(XawToggleGetCurrent(plsel_race_radios[0])) - 1;
    int ro = xtp2i(XawToggleGetCurrent(plsel_role_radios[0])) - 1;

    nhUse(w);
    nhUse(client);
    nhUse(call);

    plsel_set_play_button(ra < 0 || ro < 0);

    if (ra < 0 || ro < 0)
        return;

    valid = -1;

    for (j = 0; races[j].noun; j++) {
        boolean v = validrace(ro, j);

        if (j == ra)
            c = j;
        XtSetArg(args[0], nhStr(XtNsensitive), v);
        XtSetValues(plsel_race_radios[j], args, ONE);
        if (valid < 0 && v)
            valid = j;
    }
    if (!validrace(ro, c))
        c = valid;

    j = c + 1;
    XawToggleSetCurrent(plsel_race_radios[0], i2xtp(j));

    X11_player_selection_setupOthers();
}

static void
gendertoggleCallback(Widget w, XtPointer client, XtPointer call)
{
    int i, r = xtp2i(XawToggleGetCurrent(plsel_gend_radios[0])) - 1;

    nhUse(w);
    nhUse(client);
    nhUse(call);

    plsel_set_play_button(r < 0);

    for (i = 0; roles[i].name.m; i++) {
        if (roles[i].name.f) {
            Arg args[2];

            XtSetArg(args[0], XtNlabel,
                     (r < 1) ? roles[i].name.m : roles[i].name.f);
            XtSetValues(plsel_role_radios[i], args, ONE);
        }
    }
}

static void
aligntoggleCallback(Widget w, XtPointer client, XtPointer call)
{
    int r = xtp2i(XawToggleGetCurrent(plsel_align_radios[0])) - 1;

    nhUse(w);
    nhUse(client);
    nhUse(call);

    plsel_set_play_button(r < 0);
}

static void
plsel_random_btn_callback(Widget w, XtPointer client, XtPointer call)
{
    nhUse(w);
    nhUse(client);
    nhUse(call);

    X11_player_selection_randomize();
}

static void
plsel_play_btn_callback(Widget w, XtPointer client, XtPointer call)
{
    nhUse(w);
    nhUse(client);
    nhUse(call);

    plsel_dialog_acceptvalues();
    exit_x_event = TRUE; /* leave event loop */
}

static void
plsel_quit_btn_callback(Widget w, XtPointer client, XtPointer call)
{
    nhUse(w);
    nhUse(client);
    nhUse(call);

    ps_selected = PS_QUIT;
    exit_x_event = TRUE; /* leave event loop */
}

static Widget
X11_create_player_selection_name(Widget form)
{
    Widget namelabel, name_vp, name_form;
    Arg args[10];
    Cardinal num_args;

    /* name viewport */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainRight); num_args++;
    XtSetArg(args[num_args], XtNforceBars, False); num_args++;
    XtSetArg(args[num_args], XtNallowVert, False); num_args++;
    XtSetArg(args[num_args], XtNallowHoriz, False); num_args++;
    name_vp = XtCreateManagedWidget("name_vp", viewportWidgetClass, form,
                                    args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainRight); num_args++;
    name_form = XtCreateManagedWidget("name_form", formWidgetClass, name_vp,
                                      args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNlabel), "Name"); num_args++;
    XtSetArg(args[num_args], nhStr(XtNjustify), XtJustifyLeft); num_args++;

    namelabel = XtCreateManagedWidget("name_label",
                                      labelWidgetClass, name_form,
                                      args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), namelabel); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainRight); num_args++;
    XtSetArg(args[num_args], nhStr(XtNresizable), True); num_args++;
    XtSetArg(args[num_args], nhStr(XtNeditType),
             !plsel_ask_name ? XawtextRead : XawtextEdit); num_args++;
    XtSetArg(args[num_args], nhStr(XtNresize), XawtextResizeWidth); num_args++;
    XtSetArg(args[num_args], nhStr(XtNstring), gp.plname); num_args++;
    XtSetArg(args[num_args], XtNinsertPosition, strlen(gp.plname)); num_args++;
    XtSetArg(args[num_args], nhStr(XtNaccelerators),
             XtParseAcceleratorTable(plsel_input_accelerators)); num_args++;
    plsel_name_input = XtCreateManagedWidget("name_input",
                                             asciiTextWidgetClass, name_form,
                                             args, num_args);

    XtInstallAccelerators(plsel_name_input, plsel_name_input);
    if (plsel_ask_name) {
        XtSetKeyboardFocus(form, plsel_name_input);
    } else {
        XtSetArg(args[0], nhStr(XtNdisplayCaret), False);
        XtSetValues(plsel_name_input, args, ONE);
    }

    return name_vp;
}

static void
X11_player_selection_dialog(void)
{
    Widget popup, popup_vp;
    Widget form;
    Widget name_vp;
    Widget racelabel, race_form, race_vp, race_form2;
    Widget rolelabel, role_form, role_vp, role_form2;
    Widget gendlabel, gend_form,
           gend_radio_m, gend_radio_f, gend_vp, gend_form2;
    Widget alignlabel, align_form,
           align_radio_l, align_radio_n, align_radio_c, align_vp, align_form2;
    Widget btn_vp, btn_form, random_btn, play_btn, quit_btn;
    Widget tmpwidget;
    Arg args[10];
    Cardinal num_args;
    int i;
    int winwid = 400;
    int cwid = (winwid / 3) - 14;

    num_args = 0;
    XtSetArg(args[num_args], XtNallowShellResize, True); num_args++;
    XtSetArg(args[num_args], XtNtitle, "Player Selection"); num_args++;
    popup = XtCreatePopupShell("player_selection_dialog",
                               transientShellWidgetClass,
                               toplevel, args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], XtNtranslations,
             XtParseTranslationTable(plsel_dialog_translations)); num_args++;
    popup_vp = XtCreateManagedWidget("plsel_vp", viewportWidgetClass,
                                     popup, args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], XtNtranslations,
             XtParseTranslationTable(plsel_dialog_translations)); num_args++;
    form = XtCreateManagedWidget("plsel_form", formWidgetClass, popup_vp,
                                 args, num_args);

    name_vp = X11_create_player_selection_name(form);

    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, winwid); num_args++;
    XtSetValues(name_vp, args, num_args);

    /*
     * Layout; role is centered rather than first:
     *
     *   +------------------------------------+
     *   | name                               |
     *   +------------------------------------+
     *   +--------+   +------------+   +---------+
     *   | human  |   |archeologist|   |  male   |
     *   |  elf   |   | barbarian  |   | female  |
     *   | dwarf  |   |  caveman   |   +---------+
     *   | gnome  |   |   healer   |   +---------+
     *   |  orc   |   |   knight   |   | lawful  |
     *   +--------+   |    monk    |   | neutral |
     *                |   priest   |   | chaotic |
     *                |   rogue    |   +---------+
     *                |   ranger   |   +--------+
     *                |  samurai   |   + Random +
     *                |  tourist   |   +  Play  +
     *                |  valkyrie  |   +  Quit  +
     *                |   wizard   |   +--------+
     *                +------------+
     *
     * TODO:
     *  make name box same width as race+gap+role+gap+gender/alignment
     *  (resize it after the other boxes have been placed);
     *  make Random/Play/Quit buttons same width as gender/alignment and
     *  align bottom of them with bottom of role (they already specify
     *  the same width for the label text but different decorations--
     *  room for radio button box--of the other widgets results in the
     *  total width being different);
     *  add 'random' to each of the four boxes and Choose to the Random/
     *  Play/Quit buttons; if none of the four 'random's are currently
     *  selected, gray-out Choose; conversely, when Choose or Play is
     *  clicked on, make the random assignments for any/all of the four
     *  boxes which have 'random' selected.
     *  Maybe:  move gender box underneath race, bottom aligned with role
     *  and move alignment up to where gender currently is.  If that's
     *  done, move role column first and race+gender to middle.
     */

    /********************************************/

    /* Race */

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), name_vp); num_args++;
    race_form = XtCreateManagedWidget("race_form", formWidgetClass, form,
                                      args, num_args);

    /* race label */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNjustify), XtJustifyLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNlabel), "Race"); num_args++;
    racelabel = XtCreateManagedWidget("race_label",
                                      labelWidgetClass, race_form,
                                      args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNtopMargin), 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromVert), racelabel); num_args++;
    XtSetArg(args[num_args], XtNforceBars, False); num_args++;
    XtSetArg(args[num_args], XtNallowVert, True); num_args++;
    XtSetArg(args[num_args], XtNallowHoriz, False); num_args++;
    race_vp = XtCreateManagedWidget("race_vp", viewportWidgetClass, race_form,
                                    args, num_args);

    num_args = 0;
    race_form2 = XtCreateManagedWidget("race_form2", formWidgetClass, race_vp,
                                      args, num_args);

    for (i = 0; races[i].noun; i++)
        continue;
    plsel_n_races = i;

    plsel_race_radios = (Widget *) alloc(sizeof (Widget) * plsel_n_races);

    /* race radio buttons */
    for (i = 0; races[i].noun; i++) {
        Widget racewidget;

        num_args = 0;
        if (i > 0) {
            XtSetArg(args[num_args], nhStr(XtNfromVert),
                     tmpwidget); num_args++;
        }
        XtSetArg(args[num_args], XtNwidth, cwid); num_args++;
        if (i > 0) {
            XtSetArg(args[num_args], nhStr(XtNradioGroup),
                     plsel_race_radios[0]); num_args++;
        }
        XtSetArg(args[num_args], nhStr(XtNradioData), (i + 1)); num_args++;

        racewidget = XtCreateManagedWidget(races[i].noun,
                                           toggleWidgetClass,
                                           race_form2,
                                           args, num_args);
        XtAddCallback(racewidget, XtNcallback, racetoggleCallback, i2xtp(i));
        tmpwidget = racewidget;
        plsel_race_radios[i] = racewidget;
    }

    XawToggleUnsetCurrent(plsel_race_radios[0]);

    /********************************************/

    /* Role */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), name_vp); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromHoriz), race_form); num_args++;
    role_form = XtCreateManagedWidget("role_form", formWidgetClass, form,
                                      args, num_args);

    /* role label */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNjustify), XtJustifyLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNlabel), "Role"); num_args++;
    rolelabel = XtCreateManagedWidget("role_label", labelWidgetClass,
                                      role_form, args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNtopMargin), 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromVert), rolelabel); num_args++;
    XtSetArg(args[num_args], XtNforceBars, False); num_args++;
    XtSetArg(args[num_args], XtNallowVert, True); num_args++;
    XtSetArg(args[num_args], XtNallowHoriz, False); num_args++;
    role_vp = XtCreateManagedWidget("role_vp", viewportWidgetClass, role_form,
                                    args, num_args);

    num_args = 0;
    role_form2 = XtCreateManagedWidget("role_form2", formWidgetClass, role_vp,
                                      args, num_args);

    for (i = 0; roles[i].name.m; i++)
        continue;
    plsel_n_roles = i;

    plsel_role_radios = (Widget *) alloc(sizeof (Widget) * plsel_n_roles);

    /* role radio buttons */
    for (i = 0; roles[i].name.m; i++) {
        Widget rolewidget;

        num_args = 0;
        if (i > 0) {
            XtSetArg(args[num_args], nhStr(XtNfromVert),
                     tmpwidget); num_args++;
        }
        XtSetArg(args[num_args], nhStr(XtNwidth), cwid); num_args++;
        if (i > 0) {
            XtSetArg(args[num_args], nhStr(XtNradioGroup),
                     plsel_role_radios[0]); num_args++;
        }
        XtSetArg(args[num_args], nhStr(XtNradioData), (i + 1)); num_args++;

        rolewidget = XtCreateManagedWidget(roles[i].name.m, toggleWidgetClass,
                                           role_form2, args, num_args);
        XtAddCallback(rolewidget, XtNcallback, roletoggleCallback, i2xtp(i));
        tmpwidget = rolewidget;
        plsel_role_radios[i] = rolewidget;
    }
    XawToggleUnsetCurrent(plsel_role_radios[0]);

    /********************************************/

    /* Gender*/

    plsel_gend_radios = (Widget *) alloc(sizeof (Widget) * ROLE_GENDERS);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), name_vp); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromHoriz), role_form); num_args++;
    gend_form = XtCreateManagedWidget("gender_form", formWidgetClass, form,
                                      args, num_args);

    /* gender label */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNjustify), XtJustifyLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNlabel), "Gender"); num_args++;
    gendlabel = XtCreateManagedWidget("gender_label", labelWidgetClass,
                                      gend_form, args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNtopMargin), 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromVert), gendlabel); num_args++;
    XtSetArg(args[num_args], XtNforceBars, False); num_args++;
    XtSetArg(args[num_args], XtNallowVert, True); num_args++;
    XtSetArg(args[num_args], XtNallowHoriz, False); num_args++;
    gend_vp = XtCreateManagedWidget("gender_vp", viewportWidgetClass,
                                    gend_form, args, num_args);

    num_args = 0;
    gend_form2 = XtCreateManagedWidget("gender_form2", formWidgetClass,
                                       gend_vp, args, num_args);

    /* gender radio buttons */
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, cwid); num_args++;
    XtSetArg(args[num_args], nhStr(XtNradioData), 1); num_args++;
    plsel_gend_radios[0] = gend_radio_m
        =  XtCreateManagedWidget("Male", toggleWidgetClass,
                                 gend_form2, args, num_args);
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), gend_radio_m); num_args++;
    XtSetArg(args[num_args], XtNwidth, cwid); num_args++;
    XtSetArg(args[num_args], nhStr(XtNradioGroup),
             plsel_gend_radios[0]); num_args++;
    XtSetArg(args[num_args], nhStr(XtNradioData), 2); num_args++;
    plsel_gend_radios[1] = gend_radio_f
        =  XtCreateManagedWidget("Female", toggleWidgetClass,
                                 gend_form2, args, num_args);

    XawToggleUnsetCurrent(plsel_gend_radios[0]);

    XtAddCallback(gend_radio_m, XtNcallback,
                  gendertoggleCallback, (XtPointer) (1));
    XtAddCallback(gend_radio_f, XtNcallback,
                  gendertoggleCallback, (XtPointer) (2));

    /********************************************/

    /* Alignment */

    plsel_align_radios = (Widget *) alloc(sizeof (Widget) * ROLE_ALIGNS);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), gend_form); num_args++;
    XtSetArg(args[num_args], nhStr(XtNvertDistance), 30); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromHoriz), role_form); num_args++;
    align_form = XtCreateManagedWidget("align_form", formWidgetClass, form,
                                       args, num_args);

    /* align label */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNjustify), XtJustifyLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNlabel), "Alignment"); num_args++;
    alignlabel = XtCreateManagedWidget("align_label", labelWidgetClass,
                                       align_form, args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNtopMargin), 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromVert), alignlabel); num_args++;
    XtSetArg(args[num_args], XtNforceBars, False); num_args++;
    XtSetArg(args[num_args], XtNallowVert, True); num_args++;
    XtSetArg(args[num_args], XtNallowHoriz, False); num_args++;
    align_vp = XtCreateManagedWidget("align_vp", viewportWidgetClass,
                                     align_form, args, num_args);

    num_args = 0;
    align_form2 = XtCreateManagedWidget("align_form2", formWidgetClass,
                                        align_vp, args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, cwid); num_args++;
    XtSetArg(args[num_args], nhStr(XtNradioData), 1); num_args++;
    plsel_align_radios[0] = align_radio_l
        =  XtCreateManagedWidget("Lawful", toggleWidgetClass,
                                 align_form2, args, num_args);
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), align_radio_l); num_args++;
    XtSetArg(args[num_args], XtNwidth, cwid); num_args++;
    XtSetArg(args[num_args], nhStr(XtNradioGroup),
             plsel_align_radios[0]); num_args++;
    XtSetArg(args[num_args], nhStr(XtNradioData), 2); num_args++;
    plsel_align_radios[1] = align_radio_n
        = XtCreateManagedWidget("Neutral", toggleWidgetClass,
                                align_form2, args, num_args);
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), align_radio_n); num_args++;
    XtSetArg(args[num_args], XtNwidth, cwid); num_args++;
    XtSetArg(args[num_args], nhStr(XtNradioGroup),
             plsel_align_radios[0]); num_args++;
    XtSetArg(args[num_args], nhStr(XtNradioData), 3); num_args++;
    plsel_align_radios[2] = align_radio_c
        =  XtCreateManagedWidget("Chaotic", toggleWidgetClass,
                                 align_form2, args, num_args);

    XawToggleUnsetCurrent(plsel_align_radios[0]);

    XtAddCallback(align_radio_l, XtNcallback,
                  aligntoggleCallback, (XtPointer) (1));
    XtAddCallback(align_radio_n, XtNcallback,
                  aligntoggleCallback, (XtPointer) (2));
    XtAddCallback(align_radio_c, XtNcallback,
                  aligntoggleCallback, (XtPointer) (3));

    /********************************************/

    /* Buttons! */

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), align_form); num_args++;
    XtSetArg(args[num_args], nhStr(XtNvertDistance), 30); num_args++;
    XtSetArg(args[num_args], nhStr(XtNrightMargin), 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromHoriz), role_form); num_args++;
    XtSetArg(args[num_args], XtNforceBars, False); num_args++;
    XtSetArg(args[num_args], XtNallowVert, False); num_args++;
    XtSetArg(args[num_args], XtNallowHoriz, False); num_args++;
    btn_vp = XtCreateManagedWidget("btn_vp", viewportWidgetClass, form,
                                   args, num_args);

    num_args = 0;
    btn_form = XtCreateManagedWidget("btn_form", formWidgetClass, btn_vp,
                                     args, num_args);

    /* "Random" button */

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainRight); num_args++;
    XtSetArg(args[num_args], XtNwidth, cwid); num_args++;
    XtSetArg(args[num_args], nhStr(XtNlabel), "Random"); num_args++;
    random_btn = XtCreateManagedWidget("random", commandWidgetClass, btn_form,
                                       args, num_args);
    XtAddCallback(random_btn, XtNcallback, plsel_random_btn_callback, form);

    /* "Play" button */

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), random_btn); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainRight); num_args++;
    XtSetArg(args[num_args], XtNwidth, cwid); num_args++;
    XtSetArg(args[num_args], nhStr(XtNlabel), "Play"); num_args++;
    plsel_btn_play = play_btn
        = XtCreateManagedWidget("play", commandWidgetClass, btn_form,
                                args, num_args);
    XtAddCallback(play_btn, XtNcallback, plsel_play_btn_callback, form);

    /* "Quit" button */

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), play_btn); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainBottom); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainRight); num_args++;
    XtSetArg(args[num_args], XtNwidth, cwid); num_args++;
    XtSetArg(args[num_args], nhStr(XtNlabel), "Quit"); num_args++;
    quit_btn = XtCreateManagedWidget("quit", commandWidgetClass, btn_form,
                                     args, num_args);
    XtAddCallback(quit_btn, XtNcallback, plsel_quit_btn_callback, form);

    /********************************************/

    XtRealizeWidget(popup);
    X11_player_selection_randomize();

    if (flags.randomall) {
        plsel_dialog_acceptvalues();
    } else {
        ps_selected = -1;
        nh_XtPopup(popup, (int) XtGrabExclusive, form);
        /* The callback will enable the event loop exit. */
        (void) x_event(EXIT_ON_EXIT);
    }

    nh_XtPopdown(popup);
    XtDestroyWidget(popup);

    if (plsel_race_radios)
        free(plsel_race_radios);

    if (plsel_role_radios)
        free(plsel_role_radios);

    if (plsel_gend_radios)
        free(plsel_gend_radios);

    if (plsel_align_radios)
        free(plsel_align_radios);

    if (ps_selected == PS_QUIT
#if defined(HANGUPHANDLING)
        || gp.program_state.done_hup
#endif
       ) {
        clearlocks();
        X11_exit_nhwindows((char *) 0);
        nh_terminate(0);
    }
}

static void
X11_player_selection_prompts(void)
{
    int num_roles, num_races, num_gends, num_algns, i, availcount, availindex;
    Widget popup, player_form;
    const char **choices;
    char qbuf[QBUFSZ], plbuf[QBUFSZ];

    /* avoid unnecessary prompts further down */
    rigid_role_checks();

    (void) root_plselection_prompt(plbuf, QBUFSZ - 1, flags.initrole,
                                   flags.initrace, flags.initgend,
                                   flags.initalign);

    while (flags.initrole < 0) {
        if (flags.initrole == ROLE_RANDOM || flags.randomall) {
            flags.initrole = pick_role(flags.initrace, flags.initgend,
                                       flags.initalign, PICK_RANDOM);
            break;
        }

        /* select a role */
        for (num_roles = 0; roles[num_roles].name.m; ++num_roles)
            continue;
        choices = (const char **) alloc(sizeof (char *) * num_roles);
        for (;;) {
            availcount = 0;
            for (i = 0; i < num_roles; i++) {
                choices[i] = 0;
                if (ok_role(i, flags.initrace, flags.initgend,
                            flags.initalign)) {
                    choices[i] = roles[i].name.m;
                    if (flags.initgend >= 0 && flags.female
                        && roles[i].name.f)
                        choices[i] = roles[i].name.f;
                    ++availcount;
                }
            }
            if (availcount > 0)
                break;
            else if (flags.initalign >= 0)
                flags.initalign = -1; /* reset */
            else if (flags.initgend >= 0)
                flags.initgend = -1;
            else if (flags.initrace >= 0)
                flags.initrace = -1;
            else
                panic("no available ROLE+race+gender+alignment combinations");
        }
        Sprintf(qbuf, "Choose your %s Role", s_suffix(plbuf));
        popup =
            make_menu("player_selection", qbuf, player_select_translations,
                      "quit", ps_quit, "random", ps_random, num_roles,
                      choices, (Widget **) 0, ps_select, &player_form);

        ps_selected = -1;
        positionpopup(popup, FALSE);
        nh_XtPopup(popup, (int) XtGrabExclusive, player_form);

        /* The callbacks will enable the event loop exit. */
        (void) x_event(EXIT_ON_EXIT);

        nh_XtPopdown(popup);
        XtDestroyWidget(popup);
        free((genericptr_t) choices), choices = 0;

        if (ps_selected == PS_QUIT
#if defined(HANGUPHANDLING)
            || gp.program_state.done_hup
#endif
           ) {
            clearlocks();
            X11_exit_nhwindows((char *) 0);
            nh_terminate(0);
        } else if (ps_selected == PS_RANDOM) {
            flags.initrole = ROLE_RANDOM;
        } else if (ps_selected < 0 || ps_selected >= num_roles) {
            panic("player_selection: bad role select value %d", ps_selected);
        } else {
            flags.initrole = ps_selected;
        }
    }

    (void) root_plselection_prompt(plbuf, QBUFSZ - 1, flags.initrole,
                                   flags.initrace, flags.initgend,
                                   flags.initalign);

    while (!validrace(flags.initrole, flags.initrace)) {
        if (flags.initrace == ROLE_RANDOM || flags.randomall) {
            flags.initrace = pick_race(flags.initrole, flags.initgend,
                                       flags.initalign, PICK_RANDOM);
            break;
        }
        /* select a race */
        for (num_races = 0; races[num_races].noun; ++num_races)
            continue;
        choices = (const char **) alloc(sizeof(char *) * num_races);
        for (;;) {
            availcount = availindex = 0;
            for (i = 0; i < num_races; i++) {
                choices[i] = 0;
                if (ok_race(flags.initrole, i, flags.initgend,
                            flags.initalign)) {
                    choices[i] = races[i].noun;
                    ++availcount;
                    availindex = i; /* used iff only one */
                }
            }
            if (availcount > 0)
                break;
            else if (flags.initalign >= 0)
                flags.initalign = -1; /* reset */
            else if (flags.initgend >= 0)
                flags.initgend = -1;
            else
                panic("no available role+RACE+gender+alignment combinations");
        }

        if (availcount == 1) {
            flags.initrace = availindex;
            free((genericptr_t) choices), choices = 0;
        } else {
            Sprintf(qbuf, "Pick your %s race", s_suffix(plbuf));
            popup =
                make_menu("race_selection", qbuf, race_select_translations,
                          "quit", ps_quit, "random", ps_random, num_races,
                          choices, (Widget **) 0, ps_select, &player_form);

            ps_selected = -1;
            positionpopup(popup, FALSE);
            nh_XtPopup(popup, (int) XtGrabExclusive, player_form);

            /* The callbacks will enable the event loop exit. */
            (void) x_event(EXIT_ON_EXIT);

            nh_XtPopdown(popup);
            XtDestroyWidget(popup);
            free((genericptr_t) choices), choices = 0;

            if (ps_selected == PS_QUIT
#if defined(HANGUPHANDLING)
                || gp.program_state.done_hup
#endif
               ) {
                clearlocks();
                X11_exit_nhwindows((char *) 0);
                nh_terminate(0);
            } else if (ps_selected == PS_RANDOM) {
                flags.initrace = ROLE_RANDOM;
            } else if (ps_selected < 0 || ps_selected >= num_races) {
                panic("player_selection: bad race select value %d",
                      ps_selected);
            } else {
                flags.initrace = ps_selected;
            }
        } /* more than one race choice available */
    }

    (void) root_plselection_prompt(plbuf, QBUFSZ - 1, flags.initrole,
                                   flags.initrace, flags.initgend,
                                   flags.initalign);

    while (!validgend(flags.initrole, flags.initrace, flags.initgend)) {
        if (flags.initgend == ROLE_RANDOM || flags.randomall) {
            flags.initgend = pick_gend(flags.initrole, flags.initrace,
                                       flags.initalign, PICK_RANDOM);
            break;
        }
        /* select a gender */
        num_gends = 2; /* genders[2] isn't allowed */
        choices = (const char **) alloc(sizeof(char *) * num_gends);
        for (;;) {
            availcount = availindex = 0;
            for (i = 0; i < num_gends; i++) {
                choices[i] = 0;
                if (ok_gend(flags.initrole, flags.initrace, i,
                            flags.initalign)) {
                    choices[i] = genders[i].adj;
                    ++availcount;
                    availindex = i; /* used iff only one */
                }
            }
            if (availcount > 0)
                break;
            else if (flags.initalign >= 0)
                flags.initalign = -1; /* reset */
            else
                panic("no available role+race+GENDER+alignment combinations");
        }

        if (availcount == 1) {
            flags.initgend = availindex;
            free((genericptr_t) choices), choices = 0;
        } else {
            Sprintf(qbuf, "Your %s gender?", s_suffix(plbuf));
            popup =
                make_menu("gender_selection", qbuf, gend_select_translations,
                          "quit", ps_quit, "random", ps_random, num_gends,
                          choices, (Widget **) 0, ps_select, &player_form);

            ps_selected = -1;
            positionpopup(popup, FALSE);
            nh_XtPopup(popup, (int) XtGrabExclusive, player_form);

            /* The callbacks will enable the event loop exit. */
            (void) x_event(EXIT_ON_EXIT);

            nh_XtPopdown(popup);
            XtDestroyWidget(popup);
            free((genericptr_t) choices), choices = 0;

            if (ps_selected == PS_QUIT
#if defined(HANGUPHANDLING)
			    || gp.program_state.done_hup
#endif
                ) {
                clearlocks();
                X11_exit_nhwindows((char *) 0);
                nh_terminate(0);
            } else if (ps_selected == PS_RANDOM) {
                flags.initgend = ROLE_RANDOM;
            } else if (ps_selected < 0 || ps_selected >= num_gends) {
                panic("player_selection: bad gender select value %d",
                      ps_selected);
            } else {
                flags.initgend = ps_selected;
            }
        } /* more than one gender choice available */
    }

    (void) root_plselection_prompt(plbuf, QBUFSZ - 1, flags.initrole,
                                   flags.initrace, flags.initgend,
                                   flags.initalign);

    while (!validalign(flags.initrole, flags.initrace, flags.initalign)) {
        if (flags.initalign == ROLE_RANDOM || flags.randomall) {
            flags.initalign = pick_align(flags.initrole, flags.initrace,
                                         flags.initgend, PICK_RANDOM);
            break;
        }
        /* select an alignment */
        num_algns = 3; /* aligns[3] isn't allowed */
        choices = (const char **) alloc(sizeof(char *) * num_algns);
        for (;;) {
            availcount = availindex = 0;
            for (i = 0; i < num_algns; i++) {
                choices[i] = 0;
                if (ok_align(flags.initrole, flags.initrace, flags.initgend,
                             i)) {
                    choices[i] = aligns[i].adj;
                    ++availcount;
                    availindex = i; /* used iff only one */
                }
            }
            if (availcount > 0)
                break;
            else
                panic("no available role+race+gender+ALIGNMENT combinations");
        }

        if (availcount == 1) {
            flags.initalign = availindex;
            free((genericptr_t) choices), choices = 0;
        } else {
            Sprintf(qbuf, "Your %s alignment?", s_suffix(plbuf));
            popup = make_menu("alignment_selection", qbuf,
                              algn_select_translations, "quit", ps_quit,
                              "random", ps_random, num_algns, choices,
                              (Widget **) 0, ps_select, &player_form);

            ps_selected = -1;
            positionpopup(popup, FALSE);
            nh_XtPopup(popup, (int) XtGrabExclusive, player_form);

            /* The callbacks will enable the event loop exit. */
            (void) x_event(EXIT_ON_EXIT);

            nh_XtPopdown(popup);
            XtDestroyWidget(popup);
            free((genericptr_t) choices), choices = 0;

            if (ps_selected == PS_QUIT
#if defined(HANGUPHANDLING)
		|| gp.program_state.done_hup
#endif
               ) {
                clearlocks();
                X11_exit_nhwindows((char *) 0);
                nh_terminate(0);
            } else if (ps_selected == PS_RANDOM) {
                flags.initalign = ROLE_RANDOM;
            } else if (ps_selected < 0 || ps_selected >= num_algns) {
                panic("player_selection: bad alignment select value %d",
                      ps_selected);
            } else {
                flags.initalign = ps_selected;
            }
        } /* more than one alignment choice available */
    }
}

/* Global functions ======================================================== */

void
X11_player_selection(void)
{
    if (iflags.wc_player_selection == VIA_DIALOG) {
        if (!*gp.plname) {
#ifdef UNIX
            char *defplname = get_login_name();
#else
            char *defplname = (char *)0;
#endif
            (void) strncpy(gp.plname, defplname ? defplname : "Mumbles",
                           sizeof gp.plname - 1);
            gp.plname[sizeof gp.plname - 1] = '\0';
            iflags.renameinprogress = TRUE;
        }
        X11_player_selection_dialog();
    } else { /* iflags.wc_player_selection == VIA_PROMPTS */
        X11_player_selection_prompts();
    }
}

/* called by core to have the player pick an extended command */
int
X11_get_ext_cmd(void)
{
    if (iflags.extmenu != ec_full_list) {
        /* player has toggled the 'extmenu' option, toss the old widgets */
        if (extended_commands)
            release_extended_cmds(); /* will set extended_commands to Null */
        ec_full_list = iflags.extmenu;
    }
    if (!extended_commands)
        init_extended_commands_popup();

    extended_cmd_selected = -1; /* reset selected value */
    ec_scroll_to_view(-1); /* force scroll bar to top */

    positionpopup(extended_command_popup, FALSE); /* center on cursor */
    nh_XtPopup(extended_command_popup, (int) XtGrabExclusive,
               extended_command_form);

    /* The callbacks will enable the event loop exit. */
    (void) x_event(EXIT_ON_EXIT);

    if (extended_cmd_selected < 0) {
        return -1;
    }
    return command_indx[extended_cmd_selected];
}

void
release_extended_cmds(void)
{
    if (extended_commands) {
        XtDestroyWidget(extended_command_popup), extended_command_popup = 0;
        free((genericptr_t) extended_commands), extended_commands = 0;
        free((genericptr_t) command_list), command_list = (const char **) 0;
        free((genericptr_t) command_indx), command_indx = (short *) 0;
    }
}

/* End global functions =================================================== */

/* Extended Command ------------------------------------------------------- */
/* ARGSUSED */
static void
extend_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    int selected = (int) (ptrdiff_t) client_data;

    nhUse(w);
    nhUse(call_data);

    if (extended_cmd_selected != selected) {
        /* visibly deselect old one */
        if (extended_cmd_selected >= 0)
            swap_fg_bg(extended_commands[extended_cmd_selected]);

        /* select new one */
        swap_fg_bg(extended_commands[selected]);
        extended_cmd_selected = selected;
    }

    nh_XtPopdown(extended_command_popup);
    /* reset colors while popped down */
    swap_fg_bg(extended_commands[extended_cmd_selected]);
    ec_active = FALSE;
    exit_x_event = TRUE; /* leave event loop */
}

/* ARGSUSED */
static void
extend_dismiss(Widget w, XtPointer client_data, XtPointer call_data)
{
    nhUse(w);
    nhUse(client_data);
    nhUse(call_data);

    ec_dismiss();
}

/* ARGSUSED */
static void
extend_help(Widget w, XtPointer client_data, XtPointer call_data)
{
    nhUse(w);
    nhUse(client_data);
    nhUse(call_data);

    /* We might need to make it known that we already have one listed. */
    (void) doextlist();
}

/* ARGSUSED */
void
ec_delete(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (w == extended_command_popup) {
        ec_dismiss();
    } else {
        popup_delete(w, event, params, num_params);
    }
}

/* ARGSUSED */
static void
popup_delete(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    nhUse(event);
    nhUse(params);
    nhUse(num_params);

    ps_selected = PS_QUIT;
    nh_XtPopdown(w);
    exit_x_event = TRUE; /* leave event loop */
}

static void
ec_dismiss(void)
{
    /* unselect while still visible */
    if (extended_cmd_selected >= 0)
        swap_fg_bg(extended_commands[extended_cmd_selected]);
    extended_cmd_selected = -1; /* dismiss */
    nh_XtPopdown(extended_command_popup);
    ec_active = FALSE;
    exit_x_event = TRUE; /* leave event loop */
}

/* scroll the extended command menu if necessary
   so that choices extended_cmd_selected through ec_indx will be visible */
static void
ec_scroll_to_view(int ec_indx) /* might be greater than extended_cmd_selected */
{
    Widget viewport, scrollbar, tmpw;
    Arg args[5];
    Cardinal num_args;
    Position lo_y, hi_y; /* ext cmd label y */
    float s_shown, s_top; /* scrollbar pos */
    float s_min, s_max;
    Dimension h, hh, wh, vh; /* widget and viewport heights */
    Dimension border_width;
    int distance = 0;
    boolean force_top = (ec_indx < 0);

    /*
     * If the extended command menu needs to be scrolled in order to move
     * either the highlighted entry (extended_cmd_selected) or the target
     * entry (ec_indx) into view, we want to make both end up visible.
     * [Highligthed one is the first matching entry when the user types
     * something, such as "adjust" after typing 'a', and will be chosen
     * by pressing <return>.  Target entry is one past the last matching
     * entry (or last matching entry itself if at end of command list),
     * showing the user the other potential matches so far.]
     *
     * If that's not possible (maybe menu has been resized so that it's
     * too small), the highlighted entry takes precedence and the target
     * will be decremented until close enough to fit.
     */

    if (force_top)
        ec_indx = 0;

    /* get viewport and scrollbar widgets */
    tmpw = extended_commands[ec_indx];
    viewport = XtParent(tmpw);
    do {
        scrollbar = XtNameToWidget(tmpw, "*vertical");
        if (scrollbar)
            break;
        tmpw = XtParent(tmpw);
    } while (tmpw);

    if (scrollbar && viewport) {
        /* get selected ext command label y position and height */
        num_args = 0;
        XtSetArg(args[num_args], XtNy, &hi_y); num_args++;
        XtSetArg(args[num_args], XtNheight, &h); num_args++;
        XtSetArg(args[num_args], nhStr(XtNborderWidth), &border_width);
                                                                   num_args++;
        XtSetArg(args[num_args], nhStr(XtNdefaultDistance), &distance);
                                                                   num_args++;
        XtGetValues(extended_commands[ec_indx], args, num_args);
        if (distance < 1 || distance > 32766) /* defaultDistance is weird */
            distance = 4;
        /* vertical distance between top of one command widget and the next */
        hh = h + distance + 2 * border_width;
        /* location of the highlighted entry, if any */
        if (extended_cmd_selected >= 0) {
            XtSetArg(args[0], XtNy, &lo_y);
            XtGetValues(extended_commands[extended_cmd_selected], args, ONE);
        } else
            lo_y = hi_y;

        /* get menu widget and viewport heights */
        XtSetArg(args[0], XtNheight, &wh);
        XtGetValues(tmpw, args, ONE);
        XtSetArg(args[0], XtNheight, &vh);
        XtGetValues(viewport, args, ONE);

        /* widget might be too small if it has been resized or
           there are a very large number of ambiguous choices */
        if (hi_y - lo_y > wh) {
            ec_indx = extended_cmd_selected;
            if (wh > hh)
                ec_indx += (wh / hh);
            XtSetArg(args[0], XtNy, &hi_y);
            XtGetValues(extended_commands[ec_indx], args, num_args);
        }

        /* get scrollbar "height" and "top" position; floats between 0-1 */
        num_args = 0;
        XtSetArg(args[num_args], XtNshown, &s_shown); num_args++;
        XtSetArg(args[num_args], nhStr(XtNtopOfThumb), &s_top); num_args++;
        XtGetValues(scrollbar, args, num_args);

        s_min = s_top * vh;
        s_max = (s_top + s_shown) * vh;

        /* scroll if outside the view */
        if (force_top) {
            s_min = 0.0;
            XtCallCallbacks(scrollbar, XtNjumpProc, &s_min);
        } else if ((int) lo_y <= (int) s_min) {
            s_min = (float) (lo_y / (float) vh);
            XtCallCallbacks(scrollbar, XtNjumpProc, &s_min);
        } else if ((int) (hi_y + h) >= (int) s_max) {
            s_min = (float) ((hi_y + h) / (float) vh) - s_shown;
            XtCallCallbacks(scrollbar, XtNjumpProc, &s_min);
        }
    }
}

/* ARGSUSED */
void
ec_key(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    char ch;
    int i, pass;
    float shown, top;
    Arg arg[2];
    Widget hbar, vbar;
    XKeyEvent *xkey = (XKeyEvent *) event;

    nhUse(params);
    nhUse(num_params);

    ch = key_event_to_char(xkey);

    if (ch == '\0') { /* don't accept nul char/modifier event */
        /* don't beep */
        return;
    } else if (ch == '?') {
        extend_help((Widget) 0, (XtPointer) 0, (XtPointer) 0);
        return;
    } else if (strchr("\033\n\r", ch)) {
        if (ch == '\033') {
            /* unselect while still visible */
            if (extended_cmd_selected >= 0)
                swap_fg_bg(extended_commands[extended_cmd_selected]);
            extended_cmd_selected = -1; /* dismiss */
        }

        nh_XtPopdown(extended_command_popup);
        /* unselect while invisible */
        if (extended_cmd_selected >= 0)
            swap_fg_bg(extended_commands[extended_cmd_selected]);

        exit_x_event = TRUE; /* leave event loop */
        ec_active = FALSE;
        return;
    } else if (ch == MENU_FIRST_PAGE || ch == MENU_LAST_PAGE) {
        find_scrollbars(w, (Widget) 0, &hbar, &vbar);
        if (vbar) {
            top = (ch == MENU_FIRST_PAGE) ? 0.0 : 1.0;
            XtCallCallbacks(vbar, XtNjumpProc, &top);
        }
        return;
    } else if (ch == MENU_NEXT_PAGE || ch == MENU_PREVIOUS_PAGE) {
        find_scrollbars(w, (Widget) 0, &hbar, &vbar);
        if (vbar) {
            XtSetArg(arg[0], nhStr(XtNshown), &shown);
            XtSetArg(arg[1], nhStr(XtNtopOfThumb), &top);
            XtGetValues(vbar, arg, TWO);
            top += ((ch == MENU_NEXT_PAGE) ? shown : -shown);
            XtCallCallbacks(vbar, XtNjumpProc, &top);
        }
        return;
    }

    /*
     * If too much time has elapsed, treat current key as starting a new
     * choice, otherwise it is a continuation of the choice in progress.
     * Extra letters might be needed to disambiguate between choices
     * ("ride" vs "rub", for instance), or player may just be typing in
     * the whole word.
     */
    if (ec_active && (xkey->time - ec_time) > 2500) /* 2.5 seconds */
        ec_active = FALSE;

    if (!ec_active) {
        ec_nchars = 0;
        ec_active = TRUE;
    }

    ec_time = xkey->time;
    ec_chars[ec_nchars++] = ch;
    if (ec_nchars >= EC_NCHARS)
        ec_nchars = EC_NCHARS - 1; /* don't overflow */

    for (pass = 0; pass < 2; pass++) {
        if (pass == 1) {
            /* first pass finished, but no matching command was found */
            /* start a new one with the last char entered */
            if (extended_cmd_selected >= 0)
                swap_fg_bg(extended_commands[extended_cmd_selected]);
            extended_cmd_selected = -1; /* dismiss */
            ec_chars[0] = ec_chars[ec_nchars - 1];
            ec_nchars = 1;
        }
        for (i = 0; command_list[i]; ++i) {
            if (!strncmp(ec_chars, command_list[i], ec_nchars)) {
                if (extended_cmd_selected != i) {
                    /* I should use set() and unset() actions, but how do
                       I send the an action to the widget? */
                    if (extended_cmd_selected >= 0)
                        swap_fg_bg(extended_commands[extended_cmd_selected]);
                    extended_cmd_selected = i;
                    swap_fg_bg(extended_commands[extended_cmd_selected]);
                }
                /* advance to one past last matching entry, so that all
                   ambiguous choices, plus one to show thare aren't any
                   more such, will scroll into view */
                do {
                    if (!command_list[i + 1])
                        break; /* end of list */
                    ++i;
                } while (!strncmp(ec_chars, command_list[i], ec_nchars));

                ec_scroll_to_view(i);
                return;
            }
        }
    }
}

/*
 * Use our own home-brewed version menu because simpleMenu is designed to
 * be used from a menubox.
 */
static void
init_extended_commands_popup(void)
{
    int i, num_commands;
    int *matches;
    int ecmflags = ECM_NO1CHARCMD;

    if (ec_full_list)
        ecmflags |= ECM_IGNOREAC;

    num_commands = extcmds_match(NULL, ecmflags, &matches);

    i = num_commands + 1; /* room for each extcmd, plus terminator */
    command_list = (const char **) alloc((unsigned) (i * sizeof(char *)));
    command_indx = (short *) alloc((unsigned) (i * sizeof(short)));

    for (i = 0; i < num_commands; i++) {
        struct ext_func_tab *ec = extcmds_getentry(matches[i]);

        command_indx[i] = matches[i];
        command_list[i] = ec->ef_txt;
    }
    command_list[i] = (char *) 0;
    command_indx[i] = -1;

    extended_command_popup =
        make_menu("extended_commands", "Extended Commands",
                  extended_command_translations, "dismiss", extend_dismiss,
                  "help", extend_help, num_commands, command_list,
                  &extended_commands, extend_select, &extended_command_form);
}

/* ------------------------------------------------------------------------ */

/*
 * Create a popup widget of the following form:
 *
 *                    popup_label
 *              ----------- ------------
 *              |left_name| |right_name|
 *              ----------- ------------
 *              ------------------------
 *              |       name1          |
 *              ------------------------
 *              ------------------------
 *              |       name2          |
 *              ------------------------
 *                        .
 *                        .
 *              ------------------------
 *              |       nameN          |
 *              ------------------------
 */
static Widget
make_menu(const char *popup_name, const char *popup_label,
          const char *popup_translations, const char *left_name,
          XtCallbackProc left_callback, const char *right_name,
          XtCallbackProc right_callback, int num_names,
          const char **widget_names, /* return array of command widgets */
          Widget **command_widgets,
          XtCallbackProc name_callback, Widget *formp) /* return */
{
    Widget popup, popform, form, label, above, left, right, view;
    Widget *commands, *curr;
    int i;
    Arg args[12];
    Cardinal num_args;
    Dimension width, other_width, max_width, border_width,
              height, cumulative_height, screen_height;
    int distance, skip;
    char btnname[BUFSZ];

    commands = (Widget *) alloc((unsigned) num_names * sizeof (Widget));

    num_args = 0;
    XtSetArg(args[num_args], XtNallowShellResize, True); num_args++;
    XtSetArg(args[num_args], XtNborderWidth, 0); num_args++;
    popup = XtCreatePopupShell(popup_name, transientShellWidgetClass,
                               toplevel, args, num_args);
    XtOverrideTranslations(
        popup, XtParseTranslationTable("<Message>WM_PROTOCOLS: ec_delete()"));

    num_args = 0;
    XtSetArg(args[num_args], XtNtranslations,
             XtParseTranslationTable(popup_translations)); num_args++;
    XtSetArg(args[num_args], XtNborderWidth, 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNdefaultDistance), 0); num_args++;
    popform = XtCreateManagedWidget("topmenuform", formWidgetClass, popup,
                                    args, num_args);


    num_args = 0;
    XtSetArg(args[num_args], XtNforceBars, False); num_args++;
    XtSetArg(args[num_args], XtNallowVert, True); num_args++;
    /*XtSetArg(args[num_args], XtNborderWidth, 0); num_args++;*/
    XtSetArg(args[num_args], nhStr(XtNuseBottom), True); num_args++;
    XtSetArg(args[num_args], nhStr(XtNuseRight), True); num_args++;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainBottom); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainRight); num_args++;
    XtSetArg(args[num_args], XtNtranslations,
             XtParseTranslationTable(popup_translations)); num_args++;
    view = XtCreateManagedWidget("menuformview", viewportWidgetClass, popform,
                                 args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], XtNtranslations,
             XtParseTranslationTable(popup_translations)); num_args++;
    XtSetArg(args[num_args], XtNborderWidth, 0); num_args++;
    XtSetArg(args[num_args], nhStr(XtNdefaultDistance), 0); num_args++;
    *formp = form = XtCreateManagedWidget("menuform", formWidgetClass, view,
                                          args, num_args);

    /*
     * Get the default distance between objects in the viewport widget.
     * (Something is fishy here:  'distance' ends up being 0 but there
     * is a non-zero gap between the borders of the internal widgets.
     * It matches exactly the default value of 4 for defaultDistance.)
     */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNdefaultDistance), &distance); num_args++;
    XtSetArg(args[num_args], nhStr(XtNborderWidth), &border_width); num_args++;
    XtGetValues(view, args, num_args);
    if (distance < 1 || distance > 32766)
        distance = 4;

    /*
     * Create the label.
     */
    num_args = 0;
    XtSetArg(args[num_args], XtNborderWidth, 0); num_args++;
    label = XtCreateManagedWidget(popup_label, labelWidgetClass, form, args,
                                  num_args);

    cumulative_height = 0;
    XtSetArg(args[0], XtNheight, &height);
    XtGetValues(label, args, ONE);
    cumulative_height += distance + height; /* no border for label */

    /*
     * Create the left button.
     */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), label); num_args++;
    XtSetArg(args[num_args], nhStr(XtNlabel), left_name); num_args++;
#if 0
    XtSetArg(args[num_args], nhStr(XtNshapeStyle),
                              XmuShapeRoundedRectangle); num_args++;
#endif
    Sprintf(btnname, "btn_%s", left_name);
    left = XtCreateManagedWidget(btnname, commandWidgetClass, form, args,
                                 num_args);
    XtAddCallback(left, XtNcallback, left_callback, (XtPointer) 0);
    skip = (distance < 4) ? 8 : 2 * distance;

    num_args = 0;
    XtSetArg(args[0], XtNheight, &height);
    XtGetValues(left, args, ONE);
    cumulative_height += distance + height + 2 * border_width;

    /*
     * Create right button.
     */
    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromHoriz), left); num_args++;
    XtSetArg(args[num_args], nhStr(XtNhorizDistance), skip); num_args++;
    XtSetArg(args[num_args], nhStr(XtNfromVert), label); num_args++;
    XtSetArg(args[num_args], nhStr(XtNlabel), right_name); num_args++;
#if 0
    XtSetArg(args[num_args], nhStr(XtNshapeStyle),
                              XmuShapeRoundedRectangle); num_args++;
#endif
    Sprintf(btnname, "btn_%s", right_name);
    right = XtCreateManagedWidget(btnname, commandWidgetClass, form, args,
                                  num_args);
    XtAddCallback(right, XtNcallback, right_callback, (XtPointer) 0);

    XtInstallAccelerators(form, left);
    XtInstallAccelerators(form, right);

    /*
     * Create and place the command widgets.
     */
    for (i = 0, above = left, curr = commands; i < num_names; i++) {
        if (!widget_names[i])
            continue;
        num_args = 0;
        XtSetArg(args[num_args], XtNtranslations,
                 XtParseTranslationTable(popup_entry_translations)); num_args++;
        XtSetArg(args[num_args], nhStr(XtNfromVert), above); num_args++;
        if (above == left) {
            /* if first, we are farther apart */
            XtSetArg(args[num_args], nhStr(XtNvertDistance), skip); num_args++;
            cumulative_height += skip;
        } else
            cumulative_height += distance;
        cumulative_height += height + 2 * border_width;

        *curr = XtCreateManagedWidget(widget_names[i], commandWidgetClass,
                                      form, args, num_args);
        XtAddCallback(*curr, XtNcallback, name_callback,
                      (XtPointer) (ptrdiff_t) i);
        above = *curr++;
    }
    cumulative_height += distance; /* space at bottom of form */

    /*
     * Now find the largest width.  Start with width of left + right buttons
     * ('dismiss' + 'help' or 'quit' + 'random'), since they are adjacent.
     */
    XtSetArg(args[0], XtNwidth, &max_width);
    XtGetValues(left, args, ONE);
    XtSetArg(args[0], XtNwidth, &width);
    XtGetValues(right, args, ONE);
    /* doesn't count leftmost 'distance + border_width' and
       rightmost 'border_width + distance' since all entries have those */
    max_width = max_width + border_width + skip + border_width + width;

    /* Next, the title. */
    XtSetArg(args[0], XtNwidth, &width);
    XtGetValues(label, args, ONE);
    if (width > max_width)
        max_width = width;

    /* Finally, the commands. */
    for (i = 0, curr = commands; i < num_names; i++) {
        if (!widget_names[i])
            continue;
        XtSetArg(args[0], XtNwidth, &width);
        XtGetValues(*curr, args, ONE);
        if (width > max_width)
            max_width = width;
        curr++;
    }

    /*
     * Re-do the two side-by-side widgets to take up half the width each.
     *
     * With max_width and skip both having even values, we never have to
     * tweak left or right to maybe be one pixel wider than the other.
     */
    if (max_width % 2)
        ++max_width;
    XtSetArg(args[0], XtNwidth, &width);
    XtGetValues(left, args, ONE);
    XtSetArg(args[0], XtNwidth, &other_width);
    XtGetValues(right, args, ONE);
    if (width + border_width + skip / 2 < max_width / 2
        && other_width + border_width + skip / 2 < max_width / 2) {
        /* both are narrower than half */
        width = other_width = max_width / 2 - border_width - skip / 2;
        XtSetArg(args[0], XtNwidth, width);
        XtSetValues(left, args, ONE);
        XtSetArg(args[0], XtNwidth, other_width);
        XtSetValues(right, args, ONE);
    } else if (width + border_width + skip / 2 < max_width / 2) {
        /* 'other_width' (right) is half or more */
        width = max_width - other_width - 2 * border_width - skip;
        XtSetArg(args[0], XtNwidth, width);
        XtSetValues(left, args, ONE);
    } else if (other_width + border_width + skip / 2 < max_width / 2) {
        /* 'width' (left) is half or more */
        other_width = max_width - width - 2 * border_width - skip;
        XtSetArg(args[0], XtNwidth, other_width);
        XtSetValues(right, args, ONE);
    } else {
        ; /* both are exactly half... */
    }

    /*
     * Finally, set all of the single line widgets to the largest width.
     */
    XtSetArg(args[0], XtNwidth, max_width);
    XtSetValues(label, args, ONE);

    for (i = 0, curr = commands; i < num_names; i++) {
        if (!widget_names[i])
            continue;
        XtSetArg(args[0], XtNwidth, max_width);
        XtSetValues(*curr, args, ONE);
        curr++;
    }

    if (command_widgets)
        *command_widgets = commands;
    else
        free((char *) commands);

    /*
     * If the menu's complete height is too big for the display,
     * forcing the height to be smaller will cause the vertical
     * scroll bar (enabled but not forced above) to be included.
     */
    screen_height = XHeightOfScreen(XtScreen(popup));
    screen_height -= appResources.extcmd_height_delta; /* NetHack.ad */
    if (cumulative_height >= screen_height) {
        /* 25 is a guesstimate for scrollbar width;
           window manager might override the request for y==1 */
        num_args = 0;
        XtSetArg(args[num_args], XtNy, 1); num_args++;
        XtSetArg(args[num_args], XtNwidth, max_width + 25); num_args++;
        XtSetArg(args[num_args], XtNheight, screen_height - 1); num_args++;
        XtSetValues(popup, args, num_args);
    }
    XtRealizeWidget(popup);
    XSetWMProtocols(XtDisplay(popup), XtWindow(popup), &wm_delete_window, 1);

    /* during role selection, highlight "random" as pre-selected choice */
    if (right_callback == ps_random && strchr(ps_randchars, '\n'))
        swap_fg_bg(right);

    return popup;
}

/*winmisc.c*/

/* NetHack 3.7	cmd.c	$NHDT-Date: 1710029089 2024/03/10 00:04:49 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.712 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2013. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "func_tab.h"

#ifdef UNIX
/*
 * Some systems may have getchar() return EOF for various reasons, and
 * we should not quit before seeing at least NR_OF_EOFS consecutive EOFs.
 */
#if defined(SYSV) || defined(DGUX) || defined(HPUX)
#define NR_OF_EOFS 20
#endif
#endif

#ifdef DUMB /* stuff commented out in extern.h, but needed here */
extern int doapply(void);            /**/
extern int dorub(void);              /**/
extern int dojump(void);             /**/
extern int doextlist(void);          /**/
extern int enter_explore_mode(void); /**/
extern int dodrop(void);             /**/
extern int doddrop(void);            /**/
extern int dodown(void);             /**/
extern int doup(void);               /**/
extern int donull(void);             /**/
extern int dowipe(void);             /**/
extern int docallcnd(void);          /**/
extern int dotakeoff(void);          /**/
extern int doremring(void);          /**/
extern int dowear(void);             /**/
extern int doputon(void);            /**/
extern int doddoremarm(void);        /**/
extern int dokick(void);             /**/
extern int dofire(void);             /**/
extern int dothrow(void);            /**/
extern int doeat(void);              /**/
extern int done2(void);              /**/
extern int dovanquished(void);       /**/
extern int doengrave(void);          /**/
extern int dopickup(void);           /**/
extern int ddoinv(void);             /**/
extern int dotypeinv(void);          /**/
extern int dolook(void);             /**/
extern int doprgold(void);           /**/
extern int doprwep(void);            /**/
extern int doprarm(void);            /**/
extern int doprring(void);           /**/
extern int dopramulet(void);         /**/
extern int doprtool(void);           /**/
extern int dosuspend(void);          /**/
extern int doforce(void);            /**/
extern int doopen(void);             /**/
extern int doclose(void);            /**/
extern int dosh(void);               /**/
extern int dodiscovered(void);       /**/
extern int doclassdisco(void);       /**/
extern int doset_simple(void);       /**/
extern int doset(void);              /**/
extern int dotogglepickup(void);     /**/
extern int dowhatis(void);           /**/
extern int doquickwhatis(void);      /**/
extern int dowhatdoes(void);         /**/
extern int dohelp(void);             /**/
extern int dohistory(void);          /**/
extern int doloot(void);             /**/
extern int dodrink(void);            /**/
extern int dodip(void);              /**/
extern int dosacrifice(void);        /**/
extern int dopray(void);             /**/
extern int dotip(void);              /**/
extern int doturn(void);             /**/
extern int doredraw(void);           /**/
extern int doread(void);             /**/
extern int dosave(void);             /**/
extern int dosearch(void);           /**/
extern int doidtrap(void);           /**/
extern int dopay(void);              /**/
extern int dosit(void);              /**/
extern int dotalk(void);             /**/
extern int docast(void);             /**/
extern int dovspell(void);           /**/
extern int dotelecmd(void);          /**/
extern int dountrap(void);           /**/
extern int doversion(void);          /**/
extern int doextversion(void);       /**/
extern int doswapweapon(void);       /**/
extern int dowield(void);            /**/
extern int dowieldquiver(void);      /**/
extern int dozap(void);              /**/
extern int doorganize(void);         /**/
#endif /* DUMB */

staticfn int dosuspend_core(void);
staticfn int dosh_core(void);
staticfn int doherecmdmenu(void);
staticfn int dotherecmdmenu(void);
staticfn int doprev_message(void);
staticfn int timed_occupation(void);
staticfn boolean can_do_extcmd(const struct ext_func_tab *);
staticfn int dotravel(void);
staticfn int dotravel_target(void);
staticfn int doclicklook(void);
staticfn boolean yn_menuable_resp(const char *);
staticfn void yn_func_menu_opt(winid, char, const char *, char);
staticfn boolean yn_function_menu(const char *, const char *, char, char *);
staticfn int domouseaction(void);
staticfn int doterrain(void);
staticfn boolean u_have_seen_whole_selection(struct selectionvar *);
staticfn boolean u_have_seen_bounds_selection(struct selectionvar *);
staticfn boolean u_can_see_whole_selection(struct selectionvar *);
staticfn int dolookaround_floodfill_findroom(coordxy, coordxy);
staticfn void lookaround_known_room(coordxy, coordxy);

#if defined(__BORLANDC__) && !defined(_WIN32)
extern void show_borlandc_stats(winid);
#endif
staticfn boolean accept_menu_prefix(const struct ext_func_tab *);
staticfn void reset_cmd_vars(boolean);

staticfn void mcmd_addmenu(winid, int, const char *);
staticfn int there_cmd_menu_self(winid, coordxy, coordxy, int *);
staticfn int there_cmd_menu_next2u(winid, coordxy, coordxy, int, int *);
staticfn int there_cmd_menu_far(winid, coordxy, coordxy, int);
staticfn int there_cmd_menu_common(winid, coordxy, coordxy, int, int *);
staticfn void act_on_act(int, coordxy, coordxy);
staticfn char there_cmd_menu(coordxy, coordxy, int);
staticfn char here_cmd_menu(void);

staticfn char readchar_core(coordxy *, coordxy *, int *);
staticfn int parse(void);
staticfn void show_direction_keys(winid, char, boolean);
staticfn boolean help_dir(char, uchar, const char *);

staticfn void handler_rebind_keys_add(boolean);
staticfn boolean bind_key_fn(uchar, int (*)(void));
staticfn void commands_init(void);
staticfn boolean keylist_func_has_key(const struct ext_func_tab *, boolean *);
staticfn int keylist_putcmds(winid, boolean, int, int, boolean *);
staticfn const char *spkey_name(int);

staticfn int (*timed_occ_fn)(void);
staticfn char *doc_extcmd_flagstr(winid, const struct ext_func_tab *);
staticfn int dummyfunction(void);

static const char *readchar_queue = "";

/* for rejecting attempts to use wizard mode commands
 * Also used in wizcmds.c  */
const char unavailcmd[] = "Unavailable command '%s'.";

/* for rejecting #if !SHELL, !SUSPEND */
static const char cmdnotavail[] = "'%s' command not available.";

/* the #prevmsg command */
staticfn int
doprev_message(void)
{
    (void) nh_doprev_message();
    return ECMD_OK;
}

/* Count down by decrementing multi */
staticfn int
timed_occupation(void)
{
    (*timed_occ_fn)();
    if (gm.multi > 0)
        gm.multi--;
    return gm.multi > 0;
}

/* If you have moved since initially setting some occupations, they
 * now shouldn't be able to restart.
 *
 * The basic rule is that if you are carrying it, you can continue
 * since it is with you.  If you are acting on something at a distance,
 * your orientation to it must have changed when you moved.
 *
 * The exception to this is taking off items, since they can be taken
 * off in a number of ways in the intervening time, screwing up ordering.
 *
 *      Currently:      Take off all armor.
 *                      Picking Locks / Forcing Chests.
 *                      Setting traps.
 */
void
reset_occupations(void)
{
    reset_remarm();
    reset_pick();
    reset_trapset();
}

/* If a time is given, use it to timeout this function, otherwise the
 * function times out by its own means.
 */
void
set_occupation(int (*fn)(void), const char *txt, cmdcount_nht xtime)
{
    if (xtime) {
        go.occupation = timed_occupation;
        timed_occ_fn = fn;
    } else
        go.occupation = fn;
    go.occtxt = txt;
    go.occtime = 0;
    return;
}

/*
void
cmdq_print(int q)
{
    struct _cmd_queue *cq = gc.command_queue[q];
    char buf[QBUFSZ];

    pline("CQ:%i", q);
    while (cq) {
        switch (cq->typ) {
        case CMDQ_KEY: pline("(key:%s)", key2txt(cq->key, buf)); break;
        case CMDQ_EXTCMD: pline("(extcmd:#%s)", cq->ec_entry->ef_txt); break;
        case CMDQ_DIR: pline("(dir:%i,%i,%i)", cq->dirx, cq->diry, cq->dirz); break;
        case CMDQ_USER_INPUT: pline1("(userinput)"); break;
        case CMDQ_INT: pline("(int:%i)", cq->intval); break;
        default: pline("(ERROR:%i)",cq->typ); break;
        }
        cq = cq->next;
    }
}
*/

/* add extended command function to the command queue */
void
cmdq_add_ec(int q, int (*fn)(void))
{
    struct _cmd_queue *tmp = (struct _cmd_queue *) alloc(sizeof *tmp);
    struct _cmd_queue *cq = gc.command_queue[q];

    tmp->typ = CMDQ_EXTCMD;
    tmp->ec_entry = ext_func_tab_from_func(fn);
    tmp->next = NULL;

    while (cq && cq->next)
        cq = cq->next;

    if (cq)
        cq->next = tmp;
    else
        gc.command_queue[q] = tmp;
}

/* add a key to the command queue */
void
cmdq_add_key(int q, char key)
{
    struct _cmd_queue *tmp = (struct _cmd_queue *) alloc(sizeof *tmp);
    struct _cmd_queue *cq = gc.command_queue[q];

    tmp->typ = CMDQ_KEY;
    tmp->key = key;
    tmp->next = NULL;

    while (cq && cq->next)
        cq = cq->next;

    if (cq)
        cq->next = tmp;
    else
        gc.command_queue[q] = tmp;
}

/* add a direction to the command queue */
void
cmdq_add_dir(int q, schar dx, schar dy, schar dz)
{
    struct _cmd_queue *tmp = (struct _cmd_queue *) alloc(sizeof *tmp);
    struct _cmd_queue *cq = gc.command_queue[q];

    tmp->typ = CMDQ_DIR;
    tmp->dirx = dx;
    tmp->diry = dy;
    tmp->dirz = dz;
    tmp->next = NULL;

    while (cq && cq->next)
        cq = cq->next;

    if (cq)
        cq->next = tmp;
    else
        gc.command_queue[q] = tmp;
}

/* add placeholder to the command queue, allows user input there */
void
cmdq_add_userinput(int q)
{
    struct _cmd_queue *tmp = (struct _cmd_queue *) alloc(sizeof *tmp);
    struct _cmd_queue *cq = gc.command_queue[q];

    tmp->typ = CMDQ_USER_INPUT;
    tmp->next = NULL;

    while (cq && cq->next)
        cq = cq->next;

    if (cq)
        cq->next = tmp;
    else
        gc.command_queue[q] = tmp;
}

/* add integer to the command queue */
void
cmdq_add_int(int q, int val)
{
    struct _cmd_queue *tmp = (struct _cmd_queue *) alloc(sizeof *tmp);
    struct _cmd_queue *cq = gc.command_queue[q];

    tmp->typ = CMDQ_INT;
    tmp->intval = val;
    tmp->next = NULL;

    while (cq && cq->next)
        cq = cq->next;

    if (cq)
        cq->next = tmp;
    else
        gc.command_queue[q] = tmp;
}

/* shift the last entry in command queue to first */
void
cmdq_shift(int q)
{
    struct _cmd_queue *tmp = NULL;
    struct _cmd_queue *cq = gc.command_queue[q];

    while (cq && cq->next && cq->next->next)
        cq = cq->next;

    if (cq)
        tmp = cq->next;
    if (tmp) {
        tmp->next = gc.command_queue[q];
        gc.command_queue[q] = tmp;
        cq->next = NULL;
    }
}

struct _cmd_queue *
cmdq_reverse(struct _cmd_queue *head)
{
    struct _cmd_queue *prev = NULL, *curr = head, *next;

    while (curr) {
        next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    }
    return prev;
}

struct _cmd_queue *
cmdq_copy(int q)
{
    struct _cmd_queue *tmp = NULL;
    struct _cmd_queue *cq = gc.command_queue[q];

    while (cq) {
        struct _cmd_queue *tmp2 = (struct _cmd_queue *) alloc(sizeof *tmp2);

        *tmp2 = *cq;
        tmp2->next = tmp;
        tmp = tmp2;
        cq = cq->next;
    }

    tmp = cmdq_reverse(tmp);

    return tmp;
}

/* pop off the topmost command from the command queue.
 * caller is responsible for freeing the returned _cmd_queue.
 */
struct _cmd_queue *
cmdq_pop(void)
{
    int q = (gi.in_doagain) ? CQ_REPEAT : CQ_CANNED;
    struct _cmd_queue *tmp = gc.command_queue[q];

    if (tmp) {
        gc.command_queue[q] = tmp->next;
        tmp->next = NULL;
    }
    return tmp;
}

/* get the top entry without popping it */
struct _cmd_queue *
cmdq_peek(int q)
{
    return gc.command_queue[q];
}

/* clear all commands from the command queue */
void
cmdq_clear(int q)
{
    struct _cmd_queue *tmp = gc.command_queue[q];
    struct _cmd_queue *tmp2;

    while (tmp) {
        tmp2 = tmp->next;
        free(tmp);
        tmp = tmp2;
    }
    gc.command_queue[q] = NULL;
}

char
pgetchar(void) /* courtesy of aeb@cwi.nl */
{
    int ch = '\0';

    if (iflags.debug_fuzzer)
        return randomkey();
    ch = nhgetch();
    return (char) ch;
}

/* '#' or whatever has been bound to doextcmd() in its place */
char
extcmd_initiator(void)
{
    return gc.Cmd.extcmd_char;
}

staticfn boolean
can_do_extcmd(const struct ext_func_tab *extcmd)
{
    int ecflags = extcmd->flags;

    if (gl.luacore && nhcb_counts[NHCB_CMD_BEFORE]) {
        lua_getglobal(gl.luacore, "nh_callback_run");
        lua_pushstring(gl.luacore, nhcb_name[NHCB_CMD_BEFORE]);
        lua_pushstring(gl.luacore, extcmd->ef_txt);
        nhl_pcall_handle(gl.luacore, 2, 1, "can_do_extcmd", NHLpa_panic);
        if (!lua_toboolean(gl.luacore, -1)) {
            lua_settop(gl.luacore, 0);
            return FALSE;
        }
        lua_settop(gl.luacore, 0);
    }

    if (!wizard && (ecflags & WIZMODECMD)) {
        pline(unavailcmd, extcmd->ef_txt);
        return FALSE;
    } else if (u.uburied && !(ecflags & IFBURIED)) {
        You_cant("do that while you are buried!");
        return FALSE;
    } else if (iflags.debug_fuzzer && (ecflags & NOFUZZERCMD)) {
        return FALSE;
    }
    return TRUE;
}

/* here after # - now read a full-word command */
int
doextcmd(void)
{
    int idx, retval;
    int (*func)(void);

    /* keep repeating until we don't run help or quit */
    do {
        idx = get_ext_cmd();
        if (idx < 0)
            return ECMD_OK; /* quit */

        func = extcmdlist[idx].ef_funct;
        if (!can_do_extcmd(&extcmdlist[idx]))
            return ECMD_OK;
        if (iflags.menu_requested && !accept_menu_prefix(&extcmdlist[idx])) {
            pline("'%s' prefix has no effect for the %s command.",
                  visctrl(cmd_from_func(do_reqmenu)),
                  extcmdlist[idx].ef_txt);
            iflags.menu_requested = FALSE;
        }
        /* tell rhack() what command is actually executing */
        ge.ext_tlist = &extcmdlist[idx];

        retval = (*func)();
    } while (func == doextlist);

    return retval;
}

/* format extended command flags for display */
staticfn char *
doc_extcmd_flagstr(
    winid menuwin,
    const struct ext_func_tab *efp) /* if Null, add a footnote to the menu */
{
    static char Abuf[10]; /* 5 would suffice: {'[','m','A',']','\0'} */

    /* note: tag shown for menu prefix is 'm' even if m-prefix action
       has been bound to some other key */
    if (!efp) {
        char qbuf[QBUFSZ];

        add_menu_str(menuwin, "[A] Command autocompletes");
        Sprintf(qbuf, "[m] Command accepts '%s' prefix",
                visctrl(cmd_from_func(do_reqmenu)));
        add_menu_str(menuwin, qbuf);
        return (char *) 0;
    } else {
        boolean mprefix = accept_menu_prefix(efp),
                autocomplete = (efp->flags & AUTOCOMPLETE) != 0;
        char *p = Abuf;

        /* "" or "[m]" or "[A]" or "[mA]" */
        if (mprefix || autocomplete) {
            *p++ = '[';
            if (mprefix)
                *p++ = 'm';
            if (autocomplete)
                *p++ = 'A';
            *p++ = ']';
        }
        *p = '\0';
        return Abuf;
    }
}

/* here after #? - now list all full-word commands and provide
   some navigation capability through the long list */
int
doextlist(void)
{
    const struct ext_func_tab *efp = (struct ext_func_tab *) 0;
    char buf[BUFSZ], searchbuf[BUFSZ], descbuf[BUFSZ], promptbuf[QBUFSZ];
    const char *cmd_desc;
    winid menuwin;
    anything any;
    menu_item *selected;
    int n, pass;
    int menumode = 0, menushown[2], onelist = 0;
    boolean redisplay = TRUE, search = FALSE;
    static const char *const headings[] = { "Extended commands",
                                      "Debugging Extended Commands" };
    int clr = NO_COLOR;

    searchbuf[0] = '\0';
    menuwin = create_nhwindow(NHW_MENU);

    while (redisplay) {
        redisplay = FALSE;
        any = cg.zeroany;
        start_menu(menuwin, MENU_BEHAVE_STANDARD);
        add_menu_str(menuwin, "Extended Commands List");
        add_menu_str(menuwin, "");

        Sprintf(buf, "Switch to %s commands that don't autocomplete",
                menumode ? "including" : "excluding");
        any.a_int = 1;
        add_menu(menuwin, &nul_glyphinfo, &any, 'a', 0, ATR_NONE, clr, buf,
                 MENU_ITEMFLAGS_NONE);

        if (!*searchbuf) {
            any.a_int = 2;
            /* was 's', but then using ':' handling within the interface
               would only examine the two or three meta entries, not the
               actual list of extended commands shown via separator lines;
               having ':' as an explicit selector overrides the default
               menu behavior for it; we retain 's' as a group accelerator */
            add_menu(menuwin, &nul_glyphinfo, &any, ':', 's', ATR_NONE,
                     clr, "Search extended commands",
                     MENU_ITEMFLAGS_NONE);
        } else {
            Strcpy(buf, "Switch back from search");
            if (strlen(buf) + strlen(searchbuf) + strlen(" (\"\")") < QBUFSZ)
                Sprintf(eos(buf), " (\"%s\")", searchbuf);
            any.a_int = 3;
            /* specifying ':' as a group accelerator here is mostly a
               statement of intent (we'd like to accept it as a synonym but
               also want to hide it from general menu use) because it won't
               work for interfaces which support ':' to search; use as a
               general menu command takes precedence over group accelerator */
            add_menu(menuwin, &nul_glyphinfo, &any, 's', ':', ATR_NONE,
                     clr, buf, MENU_ITEMFLAGS_NONE);
        }
        if (wizard) {
            any.a_int = 4;
            add_menu(menuwin, &nul_glyphinfo, &any, 'z', 0, ATR_NONE, clr,
          onelist ? "Switch to showing debugging commands in separate section"
       : "Switch to showing all alphabetically, including debugging commands",
                     MENU_ITEMFLAGS_NONE);
        }
        add_menu_str(menuwin, "");
        menushown[0] = menushown[1] = 0;
        n = 0;
        for (pass = 0; pass <= 1; ++pass) {
            /* skip second pass if not in wizard mode or wizard mode
               commands are being integrated into a single list */
            if (pass == 1 && (onelist || !wizard))
                break;
            for (efp = extcmdlist; efp->ef_txt; efp++) {
                int wizc;

                if ((efp->flags & (CMD_NOT_AVAILABLE | INTERNALCMD)) != 0)
                    continue;
                /* if hiding non-autocomplete commands, skip such */
                if (menumode == 1 && (efp->flags & AUTOCOMPLETE) == 0)
                    continue;
                /* skip wizard mode commands if not in wizard mode;
                   when showing two sections, skip wizard mode commands
                   in pass==0 and skip other commands in pass==1 */
                wizc = (efp->flags & WIZMODECMD) != 0;
                if (wizc && !wizard)
                    continue;
                if (!onelist && pass != wizc)
                    continue;
                /* command description might get modified on the fly */
                cmd_desc = efp->ef_desc;
                /* suppress part of the description for #genocided if it
                   doesn't apply during the current game */
                if (!wizard && !discover
                    && (efp->flags & GENERALCMD) != 0 /* minor optimization */
                    && strstri(cmd_desc, "extinct"))
                    cmd_desc = strsubst(strcpy(descbuf, cmd_desc),
                                        " been genocided or become extinct",
                                        " been genocided");
                /* if searching, skip this command if it doesn't match */
                if (*searchbuf
                    /* first try case-insensitive substring match */
                    && !strstri(efp->ef_txt, searchbuf)
                    && !strstri(cmd_desc, searchbuf)
                    /* wildcard support; most interfaces use case-insensitive
                       pmatch rather than regexp for menu searching */
                    && !pmatchi(searchbuf, efp->ef_txt)
                    && !pmatchi(searchbuf, cmd_desc))
                    continue;

                /* We're about to show an item, have we shown the menu yet?
                   Doing menu in inner loop like this on demand avoids a
                   heading with no subordinate entries on the search
                   results menu. */
                if (!menushown[pass]) {
                    Strcpy(buf, headings[pass]);
                    add_menu_heading(menuwin, buf);
                    menushown[pass] = 1;
                }
                /* longest ef_txt at present is "wizrumorcheck" (13 chars);
                   2nd field will be "    " or " [A]" or " [m]" or "[mA]" */
                Sprintf(buf, " %-14s %4s %s", efp->ef_txt,
                        doc_extcmd_flagstr(menuwin, efp), cmd_desc);
                add_menu_str(menuwin, buf);
                ++n;
            }
            if (n)
                add_menu_str(menuwin, "");
        }
        if (*searchbuf && !n)
            add_menu_str(menuwin, "no matches");
        else
            (void) doc_extcmd_flagstr(menuwin, (struct ext_func_tab *) 0);

        end_menu(menuwin, (char *) 0);
        n = select_menu(menuwin, PICK_ONE, &selected);
        if (n > 0) {
            switch (selected[0].item.a_int) {
            case 1: /* 'a': toggle show/hide non-autocomplete */
                menumode = 1 - menumode;  /* toggle 0 -> 1, 1 -> 0 */
                redisplay = TRUE;
                break;
            case 2: /* ':' when not searching yet: enable search */
                search = TRUE;
                break;
            case 3: /* 's' when already searching: disable search */
                search = FALSE;
                searchbuf[0] = '\0';
                redisplay = TRUE;
                break;
            case 4: /* 'z': toggle showing wizard mode commands separately */
                search = FALSE;
                searchbuf[0] = '\0';
                onelist = 1 - onelist;  /* toggle 0 -> 1, 1 -> 0 */
                redisplay = TRUE;
                break;
            }
            free((genericptr_t) selected);
        } else {
            search = FALSE;
            searchbuf[0] = '\0';
        }
        if (search) {
            Strcpy(promptbuf, "Extended command list search phrase");
            Strcat(promptbuf, "?");
            getlin(promptbuf, searchbuf);
            (void) mungspaces(searchbuf);
            if (searchbuf[0] == '\033')
                searchbuf[0] = '\0';
            if (*searchbuf)
                redisplay = TRUE;
            search = FALSE;
        }
    }
    destroy_nhwindow(menuwin);
    return ECMD_OK;
}

#if defined(TTY_GRAPHICS) || defined(CURSES_GRAPHICS)
#define MAX_EXT_CMD 200 /* Change if we ever have more ext cmds */

DISABLE_WARNING_FORMAT_NONLITERAL

/*
 * This is currently used only by the tty interface and is
 * controlled via runtime option 'extmenu'.  (Most other interfaces
 * already use a menu all the time for extended commands.)
 *
 * ``# ?'' is counted towards the limit of the number of commands,
 * so we actually support MAX_EXT_CMD-1 "real" extended commands.
 *
 * Here after # - now show pick-list of possible commands.
 */
int
extcmd_via_menu(void)
{
    const struct ext_func_tab *efp;
    menu_item *pick_list = (menu_item *) 0;
    winid win;
    anything any;
    const struct ext_func_tab *choices[MAX_EXT_CMD + 1];
    char buf[BUFSZ];
    char cbuf[QBUFSZ], prompt[QBUFSZ], fmtstr[20];
    int i, n, nchoices, acount;
    int ret, len, biggest;
    int accelerator, prevaccelerator;
    int matchlevel = 0;
    boolean wastoolong, one_per_line;
    int clr = NO_COLOR;

    ret = 0;
    cbuf[0] = '\0';
    biggest = 0;
    while (!ret) {
        i = n = 0;
        any = cg.zeroany;
        /* populate choices */
        for (efp = extcmdlist; efp->ef_txt; efp++) {
            if ((efp->flags & (CMD_NOT_AVAILABLE|INTERNALCMD))
                || !(efp->flags & AUTOCOMPLETE)
                || (!wizard && (efp->flags & WIZMODECMD)))
                continue;
            if (!matchlevel || !strncmp(efp->ef_txt, cbuf, matchlevel)) {
                choices[i] = efp;
                if ((len = (int) strlen(efp->ef_desc)) > biggest)
                    biggest = len;
                if (++i > MAX_EXT_CMD) {
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
                    impossible(
      "Exceeded %d extended commands in doextcmd() menu; 'extmenu' disabled.",
                               MAX_EXT_CMD);
#endif /* NH_DEVEL_STATUS != NH_STATUS_RELEASED */
                    iflags.extmenu = FALSE;
                    return -1;
                }
            }
        }
        choices[i] = (struct ext_func_tab *) 0;
        nchoices = i;
        /* if we're down to one, we have our selection so get out of here */
        if (nchoices  <= 1) {
            ret = (nchoices == 1) ? (int) (choices[0] - extcmdlist) : -1;
            break;
        }

        /* otherwise... */
        win = create_nhwindow(NHW_MENU);
        start_menu(win, MENU_BEHAVE_STANDARD);
        Sprintf(fmtstr, "%%-%ds", biggest + 15);
        prompt[0] = '\0';
        wastoolong = FALSE; /* True => had to wrap due to line width
                             * ('w' in wizard mode) */
        /* -3: two line menu header, 1 line menu footer (for prompt) */
        one_per_line = (nchoices < ROWNO - 3);
        accelerator = prevaccelerator = 0;
        acount = 0;
        for (i = 0; choices[i]; ++i) {
            accelerator = choices[i]->ef_txt[matchlevel];
            if (accelerator != prevaccelerator || one_per_line)
                wastoolong = FALSE;
            if (accelerator != prevaccelerator || one_per_line
                || (acount >= 2
                    /* +4: + sizeof " or " - sizeof "" */
                    && (strlen(prompt) + 4 + strlen(choices[i]->ef_txt)
                        /* -6: enough room for 1 space left margin
                         *   + "%c - " menu selector + 1 space right margin */
                        >= min(sizeof prompt, COLNO - 6)))) {
                if (acount) {
                    /* flush extended cmds for that letter already in buf */
                    Sprintf(buf, fmtstr, prompt);
                    any.a_char = prevaccelerator;
                    add_menu(win, &nul_glyphinfo, &any, any.a_char,
                             0, ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
                    acount = 0;
                    if (!(accelerator != prevaccelerator || one_per_line))
                        wastoolong = TRUE;
                }
            }
            prevaccelerator = accelerator;
            if (!acount || one_per_line) {
                Sprintf(prompt, "%s%s [%s]", wastoolong ? "or " : "",
                        choices[i]->ef_txt, choices[i]->ef_desc);
            } else if (acount == 1) {
                Sprintf(prompt, "%s%s or %s", wastoolong ? "or " : "",
                        choices[i - 1]->ef_txt, choices[i]->ef_txt);
            } else {
                Strcat(prompt, " or ");
                Strcat(prompt, choices[i]->ef_txt);
            }
            ++acount;
        }
        if (acount) {
            /* flush buf */
            Sprintf(buf, fmtstr, prompt);
            any.a_char = prevaccelerator;
            add_menu(win, &nul_glyphinfo, &any, any.a_char, 0,
                     ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
        }
        Snprintf(prompt, sizeof(prompt), "Extended Command: %s", cbuf);
        end_menu(win, prompt);
        n = select_menu(win, PICK_ONE, &pick_list);
        destroy_nhwindow(win);
        if (n == 1) {
            if (matchlevel > (QBUFSZ - 2)) {
                free((genericptr_t) pick_list);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
                impossible("Too many chars (%d) entered in extcmd_via_menu()",
                           matchlevel);
#endif
                ret = -1;
            } else {
                cbuf[matchlevel++] = pick_list[0].item.a_char;
                cbuf[matchlevel] = '\0';
                free((genericptr_t) pick_list);
            }
        } else {
            if (matchlevel) {
                ret = 0;
                matchlevel = 0;
            } else
                ret = -1;
        }
    }
    return ret;
}

RESTORE_WARNING_FORMAT_NONLITERAL

#endif /* TTY_GRAPHICS */

/* #monster command - use special monster ability while polymorphed */
int
domonability(void)
{
    struct permonst *uptr = gy.youmonst.data;
    boolean might_hide = (is_hider(uptr) || hides_under(uptr));
    char c = '\0';

    if (might_hide && webmaker(uptr)) {
        c = yn_function("Hide [h] or spin a web [s]?", "hsq", 'q', TRUE);
        if (c == 'q' || c == '\033')
            return ECMD_OK;
    }
    if (can_breathe(uptr))
        return dobreathe();
    else if (attacktype(uptr, AT_SPIT))
        return dospit();
    else if (uptr->mlet == S_NYMPH)
        return doremove();
    else if (attacktype(uptr, AT_GAZE))
        return dogaze();
    else if (is_were(uptr))
        return dosummon();
    else if (c ? c == 'h' : might_hide)
        return dohide();
    else if (c ? c == 's' : webmaker(uptr))
        return dospinweb();
    else if (is_mind_flayer(uptr))
        return domindblast();
    else if (u.umonnum == PM_GREMLIN) {
        if (IS_FOUNTAIN(levl[u.ux][u.uy].typ)) {
            if (split_mon(&gy.youmonst, (struct monst *) 0))
                dryup(u.ux, u.uy, TRUE);
        } else
            There("is no fountain here.");
    } else if (is_unicorn(uptr)) {
        use_unicorn_horn((struct obj **) 0);
        return ECMD_TIME;
    } else if (uptr->msound == MS_SHRIEK) {
        You("shriek.");
        if (u.uburied)
            pline("Unfortunately sound does not carry well through rock.");
        else
            aggravate();
    } else if (is_vampire(uptr) || is_vampshifter(&gy.youmonst)) {
        return dopoly();
    } else if (u.usteed && can_breathe(u.usteed->data)) {
        (void) pet_ranged_attk(u.usteed);
        return ECMD_TIME;
    } else if (Upolyd) {
        pline("Any special ability you may have is purely reflexive.");
    } else {
        You("don't have a special ability in your normal form!");
    }
    return ECMD_OK;
}

int
enter_explore_mode(void)
{
    if (discover) {
        You("are already in explore mode.");
    } else {
        const char *oldmode = !wizard ? "normal game" : "debug mode";

        if (!authorize_explore_mode()) {
            if (!wizard) {
                You("cannot access explore mode.");
                return ECMD_OK;
            } else {
                pline(
                 "Note: normally you wouldn't be allowed into explore mode.");
                /* keep going */
            }
        }
        pline("Beware!  From explore mode there will be no return to %s,",
              oldmode);
        if (paranoid_query(ParanoidQuit,
                           "Do you want to enter explore mode?")) {
            discover = TRUE;
            wizard = FALSE;
            clear_nhwindow(WIN_MESSAGE);
            You("are now in non-scoring explore mode.");
        } else {
            clear_nhwindow(WIN_MESSAGE);
            pline("Continuing with %s.", oldmode);
        }
    }
    return ECMD_OK;
}

void
makemap_prepost(boolean pre, boolean wiztower)
{
    NHFILE tmpnhfp;
    struct monst *mtmp;

    if (pre) {
        makemap_remove_mons();
        rm_mapseen(ledger_no(&u.uz)); /* discard overview info for level */
        {
            static const char Unachieve[] = "%s achievement revoked.";

            /* achievement tracking; if replacing a level that has a
               special prize, lose credit for previously finding it and
               reset for the new instance of that prize */
            if (Is_mineend_level(&u.uz)) {
                if (remove_achievement(ACH_MINE_PRIZE))
                    pline(Unachieve, "Mine's-end");
                gc.context.achieveo.mines_prize_oid = 0;
            } else if (Is_sokoend_level(&u.uz)) {
                if (remove_achievement(ACH_SOKO_PRIZE))
                    pline(Unachieve, "Soko-prize");
                gc.context.achieveo.soko_prize_oid = 0;
            }
        }
        if (Punished) {
            ballrelease(FALSE);
            unplacebc();
        }
        /* reset lock picking unless it's for a carried container */
        maybe_reset_pick((struct obj *) 0);
        /* reset interrupted digging if it was taking place on this level */
        if (on_level(&gc.context.digging.level, &u.uz))
            (void) memset((genericptr_t) &gc.context.digging, 0,
                          sizeof (struct dig_info));
        /* reset cached targets */
        iflags.travelcc.x = iflags.travelcc.y = 0; /* travel destination */
        gc.context.polearm.hitmon = (struct monst *) 0; /* polearm target */
        /* escape from trap */
        reset_utrap(FALSE);
        check_special_room(TRUE); /* room exit */
        (void) memset((genericptr_t) &gd.dndest, 0, sizeof (dest_area));
        (void) memset((genericptr_t) &gu.updest, 0, sizeof (dest_area));
        u.ustuck = (struct monst *) 0;
        u.uswallow = u.uswldtim = 0;
        set_uinwater(0); /* u.uinwater = 0 */
        u.uundetected = 0; /* not hidden, even if means are available */
        dmonsfree(); /* purge dead monsters from 'fmon' */
        dobjsfree();

        /* discard current level; "saving" is used to release dynamic data */
        zero_nhfile(&tmpnhfp);  /* also sets fd to -1 as desired */
        tmpnhfp.mode = FREEING;
        savelev(&tmpnhfp, ledger_no(&u.uz));
    } else {
        vision_reset();
        gv.vision_full_recalc = 1;
        cls();
        /* was using safe_teleds() but that doesn't honor arrival region
           on levels which have such; we don't force stairs, just area */
        u_on_rndspot((u.uhave.amulet ? 1 : 0) /* 'going up' flag */
                     | (wiztower ? 2 : 0));
        losedogs();
        kill_genocided_monsters();
        /* u_on_rndspot() might pick a spot that has a monster, or losedogs()
           might pick the hero's spot (only if there isn't already a monster
           there), so we might have to move hero or the co-located monster */
        if ((mtmp = m_at(u.ux, u.uy)) != 0)
            u_collide_m(mtmp);
        initrack();
        if (Punished) {
            unplacebc();
            placebc();
        }
        docrt();
        flush_screen(1);
        deliver_splev_message(); /* level entry */
        check_special_room(FALSE); /* room entry */
#ifdef INSURANCE
        save_currentstate();
#endif
    }
}

/* temporary? hack, since level type codes aren't the same as screen
   symbols and only the latter have easily accessible descriptions.
   Also used by wizcmds.c */
const char *levltyp[MAX_TYPE + 2] = {
    "stone", "vertical wall", "horizontal wall", "top-left corner wall",
    "top-right corner wall", "bottom-left corner wall",
    "bottom-right corner wall", "cross wall", "tee-up wall", "tee-down wall",
    "tee-left wall", "tee-right wall", "drawbridge wall", "tree",
    "secret door", "secret corridor", "pool", "moat", "water",
    "drawbridge up", "lava pool", "lava wall", "iron bars", "door",
    "corridor", "room", "stairs", "ladder", "fountain", "throne", "sink",
    "grave", "altar", "ice", "drawbridge down", "air", "cloud",
    /* not a real terrain type, but used for undiggable stone
       by wiz_map_levltyp() */
    "unreachable/undiggable",
    /* padding in case the number of entries above is odd */
    ""
};

const char *
levltyp_to_name(int typ)
{
    if (typ >= 0 && typ < MAX_TYPE)
        return levltyp[typ];
    return NULL;
}

/* #terrain command -- show known map, inspired by crawl's '|' command */
staticfn int
doterrain(void)
{
    winid men;
    menu_item *sel;
    anything any;
    int n;
    int which;
    int clr = NO_COLOR;

    /* this used to be done each time vision was recalculated, so would
       always be up to date (hopefully); now we do it on demand instead */
    recalc_mapseen();

    /*
     * normal play: choose between known map without mons, obj, and traps
     *  (to see underlying terrain only), or
     *  known map without mons and objs (to see traps under mons and objs), or
     *  known map without mons (to see objects under monsters);
     * explore mode: normal choices plus full map (w/o mons, objs, traps);
     * wizard mode: normal and explore choices plus
     *  a dump of the internal levl[][].typ codes w/ level flags, or
     *  a legend for the levl[][].typ codes dump
     */
    men = create_nhwindow(NHW_MENU);
    start_menu(men, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    any.a_int = 1;
    add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
             "known map without monsters, objects, and traps",
             MENU_ITEMFLAGS_SELECTED);
    any.a_int = 2;
    add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
             clr, "known map without monsters and objects",
             MENU_ITEMFLAGS_NONE);
    any.a_int = 3;
    add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
             clr, "known map without monsters",
             MENU_ITEMFLAGS_NONE);
    if (discover || wizard) {
        any.a_int = 4;
        add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, "full map without monsters, objects, and traps",
                 MENU_ITEMFLAGS_NONE);
        if (wizard) {
            any.a_int = 5;
            add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                     clr, "internal levl[][].typ codes in base-36",
                     MENU_ITEMFLAGS_NONE);
            any.a_int = 6;
            add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                     clr, "legend of base-36 levl[][].typ codes",
                     MENU_ITEMFLAGS_NONE);
        }
    }
    end_menu(men, "View which?");

    n = select_menu(men, PICK_ONE, &sel);
    destroy_nhwindow(men);
    /*
     * n <  0: player used ESC to cancel;
     * n == 0: preselected entry was explicitly chosen and got toggled off;
     * n == 1: preselected entry was implicitly chosen via <space>|<enter>;
     * n == 2: another entry was explicitly chosen, so skip preselected one.
     */
    which = (n < 0) ? -1 : (n == 0) ? 1 : sel[0].item.a_int;
    if (n > 1 && which == 1)
        which = sel[1].item.a_int;
    if (n > 0)
        free((genericptr_t) sel);

    switch (which) {
    case 1: /* known map */
        reveal_terrain(TER_MAP);
        break;
    case 2: /* known map with known traps */
        reveal_terrain(TER_MAP | TER_TRP);
        break;
    case 3: /* known map with known traps and objects */
        reveal_terrain(TER_MAP | TER_TRP | TER_OBJ);
        break;
    case 4: /* full map */
        reveal_terrain(TER_MAP | TER_FULL);
        break;
    case 5: /* map internals */
        wiz_map_levltyp();
        break;
    case 6: /* internal details */
        wiz_levltyp_legend();
        break;
    default:
        break;
    }
    return ECMD_OK; /* no time elapses */
}

/* has hero seen all locations in selection? */
staticfn boolean
u_have_seen_whole_selection(struct selectionvar *sel)
{
    coordxy x, y;
    NhRect rect = cg.zeroNhRect;

    selection_getbounds(sel, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++)
            if (isok(x,y) && selection_getpoint(x, y, sel)
                && glyph_at(x, y) == GLYPH_UNEXPLORED)
                return FALSE;

    return TRUE;
}

/* has hero seen all location of the rectangular outline in the selection */
staticfn boolean
u_have_seen_bounds_selection(struct selectionvar *sel)
{
    coordxy x, y;
    NhRect rect = cg.zeroNhRect;

    selection_getbounds(sel, &rect);

    for (x = rect.lx; x <= rect.hx; x++) {
        y = rect.ly;
        if (isok(x,y) && selection_getpoint(x, y, sel)
            && glyph_at(x, y) == GLYPH_UNEXPLORED)
            return FALSE;
        y = rect.hy;
        if (isok(x,y) && selection_getpoint(x, y, sel)
            && glyph_at(x, y) == GLYPH_UNEXPLORED)
            return FALSE;
    }
    for (y = rect.ly; y <= rect.hy; y++) {
        x = rect.lx;
        if (isok(x,y) && selection_getpoint(x, y, sel)
            && glyph_at(x, y) == GLYPH_UNEXPLORED)
            return FALSE;
        x = rect.hx;
        if (isok(x,y) && selection_getpoint(x, y, sel)
            && glyph_at(x, y) == GLYPH_UNEXPLORED)
            return FALSE;
    }

    return TRUE;
}

/* can hero currently see all locations in the selection */
staticfn boolean
u_can_see_whole_selection(struct selectionvar *sel)
{
    coordxy x, y;
    NhRect rect = cg.zeroNhRect;

    selection_getbounds(sel, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++)
            if (isok(x,y) && selection_getpoint(x, y, sel) && !cansee(x, y))
                return FALSE;

    return TRUE;
}

/* selection_floofill callback to get all locations in a room */
staticfn int
dolookaround_floodfill_findroom(coordxy x, coordxy y)
{
    schar typ = levl[x][y].typ;

    if (IS_STWALL(typ) || IS_DOOR(typ) || IS_TREE(typ)
        || IS_WATERWALL(typ) || typ == LAVAWALL || typ == IRONBARS
        || typ == SCORR || typ == SDOOR || typ == DRAWBRIDGE_UP)
        return FALSE;
    return TRUE;
}

/* describe the room at x,y */
staticfn void
lookaround_known_room(coordxy x, coordxy y)
{
    struct selectionvar *sel = selection_new();
    int rmno = u.urooms[0] - ROOMOFFSET;
    char qbuf[QBUFSZ];

    set_selection_floodfillchk(dolookaround_floodfill_findroom);
    selection_floodfill(sel, x, y, TRUE);

    if (!u_at(x, y))
        set_msg_xy(x, y);

    if (u_have_seen_whole_selection(sel)) {
        boolean u_in = (boolean) selection_getpoint(x, y, sel);

        You("%s %s %s.",
            u_at(x, y) && u_in && u_can_see_whole_selection(sel) ? "are in"
            : (u_at(x, y)) ? "remember this as" : "remember that as",
            an(selection_size_description(sel, qbuf)),
            rmno >= 0 ? "room" : "area");
    } else if (u_have_seen_bounds_selection(sel)) {
        You("guess %s to be %s %s.",
            u_at(x, y) ? "this" : "that",
            an(selection_size_description(sel, qbuf)),
            rmno >= 0 ? "room" : "area");
    } else {
        You("can't guess the size of %s area.",
            u_at(x, y) ? "this" : "that");
    }
    selection_free(sel, TRUE);
}

/* #lookaround - describe what the hero can see, in text */
int
dolookaround(void)
{
    coordxy x, y;
    int tmp_getloc_filter = iflags.getloc_filter;
    boolean tmp_accessiblemsg = a11y.accessiblemsg;
    boolean corr_next2u = FALSE;

    a11y.accessiblemsg = TRUE;
    if (levl[u.ux][u.uy].typ == CORR) {
        /* In a corridor, mention corridors next to you. */
        corr_next2u = TRUE;
        /* TODO: if we know, describe where the corridor goes,
           perhaps by describing the rooms? */
    } else if (IS_DOOR(levl[u.ux][u.uy].typ)) {
        /* In a doorway, describe the rooms next to you */
        int i;

        for (i = DIR_W; i < N_DIRS; i += 2) {
            x = u.ux + xdir[i];
            y = u.uy + ydir[i];
            if (isok(x, y) && IS_ROOM(levl[x][y].typ))
                lookaround_known_room(x, y);
        }
        corr_next2u = TRUE;
    } else {
        lookaround_known_room(u.ux, u.uy);
    }

    /* TODO: maybe describe stuff outside the current room differently? */

    iflags.getloc_filter = GFILTER_VIEW;
    for (y = 0; y < ROWNO; y++)
        for (x = 1; x < COLNO; x++) {
            int glyph, mapsym;
            boolean iscorr = (corr_next2u
                              && (glyph = glyph_at(x, y)) >= 0
                              && glyph_is_cmap(glyph)
                              && ((mapsym = glyph_to_cmap(glyph)) == S_corr
                                   || mapsym == S_litcorr));

            if (!u_at(x, y)
                && (gather_locs_interesting(x, y, GLOC_INTERESTING)
                    /* note: GLOC_INTERESTING catches S_engrcorr */
                    || iscorr)) {
                char buf[BUFSZ];
                coord cc;
                int sym = 0;
                const char *firstmatch = 0;

                cc.x = x, cc.y = y;
                do_screen_description(cc, TRUE, sym, buf, &firstmatch, NULL);
                pline_xy(x, y, "%s.", firstmatch);
            }
        }

    iflags.getloc_filter = tmp_getloc_filter;
    a11y.accessiblemsg = tmp_accessiblemsg;

    return ECMD_OK;
}

void
set_move_cmd(int dir, int run)
{
    u.dz = zdir[dir];
    u.dx = xdir[dir];
    u.dy = ydir[dir];
    /* #reqmenu -prefix disables autopickup during movement */
    if (iflags.menu_requested)
        gc.context.nopick = 1;
    gc.context.travel = gc.context.travel1 = 0;
    if (!gd.domove_attempting && !u.dz) {
        gc.context.run = run;
        gd.domove_attempting |= (!run ? DOMOVE_WALK : DOMOVE_RUSH);
    }
}

/* move or attack */
int
do_move_west(void)
{
    set_move_cmd(DIR_W, 0);
    return ECMD_TIME;
}

int
do_move_northwest(void)
{
    set_move_cmd(DIR_NW, 0);
    return ECMD_TIME;
}

int
do_move_north(void)
{
    set_move_cmd(DIR_N, 0);
    return ECMD_TIME;
}

int
do_move_northeast(void)
{
    set_move_cmd(DIR_NE, 0);
    return ECMD_TIME;
}

int
do_move_east(void)
{
    set_move_cmd(DIR_E, 0);
    return ECMD_TIME;
}

int
do_move_southeast(void)
{
    set_move_cmd(DIR_SE, 0);
    return ECMD_TIME;
}

int
do_move_south(void)
{
    set_move_cmd(DIR_S, 0);
    return ECMD_TIME;
}

int
do_move_southwest(void)
{
    set_move_cmd(DIR_SW, 0);
    return ECMD_TIME;
}

/* rush */
int
do_rush_west(void)
{
    set_move_cmd(DIR_W, 3);
    return ECMD_TIME;
}

int
do_rush_northwest(void)
{
    set_move_cmd(DIR_NW, 3);
    return ECMD_TIME;
}

int
do_rush_north(void)
{
    set_move_cmd(DIR_N, 3);
    return ECMD_TIME;
}

int
do_rush_northeast(void)
{
    set_move_cmd(DIR_NE, 3);
    return ECMD_TIME;
}

int
do_rush_east(void)
{
    set_move_cmd(DIR_E, 3);
    return ECMD_TIME;
}

int
do_rush_southeast(void)
{
    set_move_cmd(DIR_SE, 3);
    return ECMD_TIME;
}

int
do_rush_south(void)
{
    set_move_cmd(DIR_S, 3);
    return ECMD_TIME;
}

int
do_rush_southwest(void)
{
    set_move_cmd(DIR_SW, 3);
    return ECMD_TIME;
}

/* run */
int
do_run_west(void)
{
    set_move_cmd(DIR_W, 1);
    return ECMD_TIME;
}

int
do_run_northwest(void)
{
    set_move_cmd(DIR_NW, 1);
    return ECMD_TIME;
}

int
do_run_north(void)
{
    set_move_cmd(DIR_N, 1);
    return ECMD_TIME;
}

int
do_run_northeast(void)
{
    set_move_cmd(DIR_NE, 1);
    return ECMD_TIME;
}

int
do_run_east(void)
{
    set_move_cmd(DIR_E, 1);
    return ECMD_TIME;
}

int
do_run_southeast(void)
{
    set_move_cmd(DIR_SE, 1);
    return ECMD_TIME;
}

int
do_run_south(void)
{
    set_move_cmd(DIR_S, 1);
    return ECMD_TIME;
}

int
do_run_southwest(void)
{
    set_move_cmd(DIR_SW, 1);
    return ECMD_TIME;
}

/* #reqmenu, prefix command to modify some others */
int
do_reqmenu(void)
{
    if (iflags.menu_requested) {
        Norep("Double %s prefix, canceled.",
              visctrl(cmd_from_func(do_reqmenu)));
        iflags.menu_requested = FALSE;
        return ECMD_CANCEL;
    }

    iflags.menu_requested = TRUE;
    return ECMD_OK;
}

/* #rush */
int
do_rush(void)
{
    if ((gd.domove_attempting & DOMOVE_RUSH)) {
        Norep("Double rush prefix, canceled.");
        gc.context.run = 0;
        gd.domove_attempting = 0;
        return ECMD_CANCEL;
    }

    gc.context.run = 2;
    gd.domove_attempting |= DOMOVE_RUSH;
    return ECMD_OK;
}

/* #run */
int
do_run(void)
{
    if ((gd.domove_attempting & DOMOVE_RUSH)) {
        Norep("Double run prefix, canceled.");
        gc.context.run = 0;
        gd.domove_attempting = 0;
        return ECMD_CANCEL;
    }

    gc.context.run = 3;
    gd.domove_attempting |= DOMOVE_RUSH;
    return ECMD_OK;
}

/* #fight */
int
do_fight(void)
{
    if (gc.context.forcefight) {
        Norep("Double fight prefix, canceled.");
        gc.context.forcefight = 0;
        gd.domove_attempting = 0;
        return ECMD_CANCEL;
    }

    gc.context.forcefight = 1;
    gd.domove_attempting |= DOMOVE_WALK;
    return ECMD_OK;
}

/* #repeat */
int
do_repeat(void)
{
    int res = ECMD_OK;

    if (!gi.in_doagain) {
        struct _cmd_queue *repeat_copy;

        if (!cmdq_peek(CQ_REPEAT)) {
            Norep("There is no command available to repeat.");
            return ECMD_FAIL;
        }
        repeat_copy = cmdq_copy(CQ_REPEAT);
        gi.in_doagain = TRUE;
        rhack(0); /* read and execute command */
        gi.in_doagain = FALSE;
        cmdq_clear(CQ_REPEAT);
        gc.command_queue[CQ_REPEAT] = repeat_copy;
        iflags.menu_requested = FALSE;
        if (gc.context.move)
            res = ECMD_TIME;
    }
    return res;
}

/* extcmdlist: full command list, ordered by command name;
   commands with no keystroke or with only a meta keystroke generally
   need to be flagged as autocomplete and ones with a regular keystroke
   or control keystroke generally should not be; there are a few exceptions
   such as ^O/#overview and C/N/#name */
struct ext_func_tab extcmdlist[] = {
    { '#',    "#", "enter and perform an extended command",
              doextcmd, IFBURIED | GENERALCMD | CMD_M_PREFIX, NULL },
    { M('?'), "?", "list all extended commands",
              doextlist, IFBURIED | AUTOCOMPLETE | GENERALCMD | CMD_M_PREFIX,
              NULL },
    { M('a'), "adjust", "adjust inventory letters",
              doorganize, IFBURIED | AUTOCOMPLETE, NULL },
    { M('A'), "annotate", "name current level",
              donamelevel, IFBURIED | AUTOCOMPLETE | GENERALCMD, NULL },
    { 'a',    "apply", "apply (use) a tool (pick-axe, key, lamp...)",
              doapply, CMD_M_PREFIX, NULL },
    { C('x'), "attributes", "show your attributes",
              doattributes, IFBURIED, NULL },
    { '@',    "autopickup", "toggle the 'autopickup' option on/off",
              dotogglepickup, IFBURIED, NULL },
#ifdef CRASHREPORT
    { '\0',   "bugreport", "file a bug report",
              dobugreport, GENERALCMD | NOFUZZERCMD, NULL },
#endif
    { 'C',    "call", "name a monster, specific object, or type of object",
              docallcmd, IFBURIED, NULL },
    { 'Z',    "cast", "zap (cast) a spell",
              docast, IFBURIED, NULL },
    { M('c'), "chat", "talk to someone",
              dotalk, IFBURIED | AUTOCOMPLETE, NULL },
    { '\0',   "chronicle", "show journal of major events",
              do_gamelog, IFBURIED | AUTOCOMPLETE | GENERALCMD, NULL },
    { 'c',    "close", "close a door",
              doclose, 0, NULL },
    { M('C'), "conduct", "list voluntary challenges you have maintained",
              doconduct, IFBURIED | AUTOCOMPLETE | GENERALCMD, NULL },
    { '\0',   "debugfuzzer", "start the fuzz tester",
              wiz_fuzzer, IFBURIED | WIZMODECMD | NOFUZZERCMD, NULL },
    { M('d'), "dip", "dip an object into something",
              dodip, AUTOCOMPLETE | CMD_M_PREFIX, NULL },
    { '>',    "down", "go down a staircase",
              /* allows 'm' prefix (for move without autopickup) but not the
                 g/G/F movement modifiers; not flagged as MOVEMENTCMD because
                 that would suppress it from dokeylist output */
              dodown, CMD_M_PREFIX, NULL },
    { 'd',    "drop", "drop an item",
              dodrop, 0, NULL },
    { 'D',    "droptype", "drop specific item types",
              doddrop, 0, NULL },
    { 'e',    "eat", "eat something",
              doeat, CMD_M_PREFIX, NULL },
    { 'E',    "engrave", "engrave writing on the floor",
              doengrave, 0, NULL },
    { M('e'), "enhance", "advance or check weapon and spell skills",
              enhance_weapon_skill, IFBURIED | AUTOCOMPLETE, NULL },
    /* #exploremode should be flagged AUTOCOMPETE but that would negatively
       impact frequently used #enhance by making #e become ambiguous */
    { M('X'), "exploremode", "enter explore (discovery) mode",
              enter_explore_mode, IFBURIED | GENERALCMD | NOFUZZERCMD, NULL },
    { 'F',    "fight", "prefix: force fight even if you don't see a monster",
              do_fight, PREFIXCMD, NULL },
    { 'f',    "fire", "fire ammunition from quiver",
              dofire, 0, NULL },
    { M('f'), "force", "force a lock",
              doforce, AUTOCOMPLETE, NULL },
    { M('g'), "genocided",
              "list monsters that have been genocided or become extinct",
              dogenocided,
              IFBURIED | AUTOCOMPLETE | GENERALCMD | CMD_M_PREFIX, NULL },
    { ';',    "glance", "show what type of thing a map symbol corresponds to",
              doquickwhatis, IFBURIED | GENERALCMD, NULL },
    { '?',    "help", "give a help message",
              dohelp, IFBURIED | GENERALCMD, NULL },
    { '\0',   "herecmdmenu", "show menu of commands you can do here",
              doherecmdmenu, IFBURIED | AUTOCOMPLETE | GENERALCMD, NULL },
    { 'V',    "history", "show long version and game history",
              dohistory, IFBURIED | GENERALCMD, NULL },
    { 'i',    "inventory", "show your inventory",
              ddoinv, IFBURIED, NULL },
    { 'I',    "inventtype", "show inventory of one specific item class",
              dotypeinv, IFBURIED, NULL },
    { M('i'), "invoke", "invoke an object's special powers",
              doinvoke, IFBURIED | AUTOCOMPLETE, NULL },
    { M('j'), "jump", "jump to another location",
              dojump, AUTOCOMPLETE, NULL },
    { C('d'), "kick", "kick something",
              dokick, 0, NULL },
    { '\\',   "known", "show what object types have been discovered",
              dodiscovered, IFBURIED | GENERALCMD | CMD_M_PREFIX, NULL },
    { '`',    "knownclass", "show discovered types for one class of objects",
              doclassdisco, IFBURIED | GENERALCMD | CMD_M_PREFIX, NULL },
    { '\0',   "levelchange", "change experience level",
              wiz_level_change, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { '\0',   "lightsources", "show mobile light sources",
              wiz_light_sources, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { ':',    "look", "look at what is here",
              dolook, IFBURIED, NULL },
    { '\0',   "lookaround", "describe what you can see",
              dolookaround, IFBURIED | GENERALCMD, NULL },
    { M('l'), "loot", "loot a box on the floor",
              doloot, AUTOCOMPLETE | CMD_M_PREFIX, NULL },
    { '\0',   "migratemons",
#ifdef DEBUG_MIGRATING_MONSTERS
              "show migrating monsters and migrate N random ones",
#else
              "show migrating monsters",
#endif
              wiz_migrate_mons, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { M('m'), "monster", "use monster's special ability",
              domonability, IFBURIED | AUTOCOMPLETE, NULL },
    { M('n'), "name", "same as call; name a monster or object or object type",
              docallcmd, IFBURIED | AUTOCOMPLETE, NULL },
    { M('o'), "offer", "offer a sacrifice to the gods",
              dosacrifice, AUTOCOMPLETE | CMD_M_PREFIX, NULL },
    { 'o',    "open", "open a door",
              doopen, 0, NULL },
    /* 'm #options' runs doset() */
    { 'O',    "options", "show option settings",
              doset_simple, IFBURIED | GENERALCMD | CMD_M_PREFIX, NULL },
    /* 'm #optionsfull' runs doset_simple() */
    { '\0',   "optionsfull", "show all option settings, possibly change them",
              doset, IFBURIED | GENERALCMD | CMD_M_PREFIX, NULL },
    /* #overview used to need autocomplete and has retained that even
       after being assigned to ^O [old wizard mode ^O is now #wizwhere];
       'm' prefix displays overview as a menu where player can choose a
       level to supply with an annotation */
    { C('o'), "overview", "show a summary of the explored dungeon",
              dooverview,
              IFBURIED | AUTOCOMPLETE | GENERALCMD | CMD_M_PREFIX, NULL },
    /* [should #panic actually autocomplete?] */
    { '\0',   "panic", "test panic routine (fatal to game)",
              wiz_panic, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { 'p',    "pay", "pay your shopping bill",
              dopay, CMD_M_PREFIX, NULL },
    { '|',    "perminv", "scroll persistent inventory display",
              doperminv, IFBURIED | GENERALCMD | NOFUZZERCMD, NULL },
    { ',',    "pickup", "pick up things at the current location",
              dopickup, CMD_M_PREFIX, NULL },
    { '\0',   "polyself", "polymorph self",
              wiz_polyself, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { M('p'), "pray", "pray to the gods for help",
              dopray, IFBURIED | AUTOCOMPLETE, NULL },
    { C('p'), "prevmsg", "view recent game messages",
              doprev_message, IFBURIED | GENERALCMD | CMD_INSANE, NULL },
    { 'P',    "puton", "put on an accessory (ring, amulet, etc)",
              doputon, 0, NULL },
    { 'q',    "quaff", "quaff (drink) something",
              dodrink, CMD_M_PREFIX, NULL },
    { '\0', "quit", "exit without saving current game",
              done2, IFBURIED | AUTOCOMPLETE | GENERALCMD | NOFUZZERCMD,
              NULL },
    { 'Q',    "quiver", "select ammunition for quiver",
              dowieldquiver, 0, NULL },
    { 'r',    "read", "read a scroll or spellbook",
              doread, 0, NULL },
    { C('r'), "redraw", "redraw screen",
              doredraw, IFBURIED | GENERALCMD | CMD_INSANE, NULL },
    { 'R',    "remove", "remove an accessory (ring, amulet, etc)",
              doremring, 0, NULL },
    { C('a'), "repeat", "repeat a previous command",
              do_repeat, IFBURIED | GENERALCMD, NULL },
    /* "modify command" is a vague description for use as no-autopickup,
       no-attack movement as well as miscellaneous non-movement things;
       key2extcmddesc() constructs a more explicit two line description
       for display by the '&' command and expects to find "prefix:" as
       the start of the text here */
    { 'm',    "reqmenu", "prefix: request menu or modify command",
              do_reqmenu, PREFIXCMD, NULL },
    { C('_'), "retravel", "travel to previously selected travel location",
              dotravel_target, 0, NULL },
    { M('R'), "ride", "mount or dismount a saddled steed",
              doride, AUTOCOMPLETE, NULL },
    { M('r'), "rub", "rub a lamp or a stone",
              dorub, AUTOCOMPLETE, NULL },
    { 'G',    "run", "prefix: run until something interesting is seen",
              do_run, PREFIXCMD, NULL },
    { 'g',    "rush", "prefix: rush until something interesting is seen",
              do_rush, PREFIXCMD, NULL },
    { 'S',    "save", "save the game and exit",
              dosave, IFBURIED | GENERALCMD | NOFUZZERCMD, NULL },
    { '\0',   "saveoptions", "save the game configuration",
              do_write_config_file,
              IFBURIED | GENERALCMD | NOFUZZERCMD, NULL },
    { 's',    "search", "search for traps and secret doors",
              dosearch, IFBURIED | CMD_M_PREFIX, "searching" },
    { '*',    "seeall", "show all equipment in use",
              doprinuse, IFBURIED | CMD_M_PREFIX, NULL },
    { AMULET_SYM, "seeamulet", "show the amulet currently worn",
              dopramulet, IFBURIED | CMD_M_PREFIX, NULL },
    { ARMOR_SYM, "seearmor", "show the armor currently worn",
              doprarm, IFBURIED | CMD_M_PREFIX, NULL },
    { RING_SYM, "seerings", "show the ring(s) currently worn",
              doprring, IFBURIED | CMD_M_PREFIX, NULL },
    { TOOL_SYM, "seetools", "show the tools currently in use",
              doprtool, IFBURIED | CMD_M_PREFIX, NULL },
    { WEAPON_SYM, "seeweapon", "show the weapon currently wielded",
              doprwep, IFBURIED | CMD_M_PREFIX, NULL },
    { '!', "shell", "leave game to enter a sub-shell ('exit' to come back)",
              dosh_core, (IFBURIED | GENERALCMD | NOFUZZERCMD
#ifndef SHELL
                        | CMD_NOT_AVAILABLE
#endif /* SHELL */
                        ), NULL },
    /* $ is like ),=,&c but is not included with *, so not called "seegold" */
    { GOLD_SYM, "showgold", "show gold, possibly shop credit or debt",
              doprgold, IFBURIED | CMD_M_PREFIX, NULL },
    { SPBOOK_SYM, "showspells", "list and reorder known spells",
              dovspell, IFBURIED, NULL },
    { '^',    "showtrap", "describe an adjacent, discovered trap",
              doidtrap, IFBURIED, NULL },
    { M('s'), "sit", "sit down",
              dosit, AUTOCOMPLETE, NULL },
    { '\0',   "stats", "show memory statistics",
              wiz_show_stats, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { C('z'), "suspend", "push game to background ('fg' to come back)",
              dosuspend_core, (IFBURIED | GENERALCMD | NOFUZZERCMD
#ifndef SUSPEND
                               | CMD_NOT_AVAILABLE
#endif /* SUSPEND */
                               ), NULL },
    { 'x',    "swap", "swap wielded and secondary weapons",
              doswapweapon, 0, NULL },
    { 'T',    "takeoff", "take off one piece of armor",
              dotakeoff, 0, NULL },
    { 'A',    "takeoffall", "remove all armor",
              doddoremarm, 0, NULL },
    { C('t'), "teleport", "teleport around the level",
              dotelecmd, IFBURIED | CMD_M_PREFIX, NULL },
    /* \177 == <del> aka <delete> aka <rubout>; some terminals have an
       option to swap it with <backspace> so if there's a key labeled
       <delete> it may or may not actually invoke the #terrain command */
    { '\177', "terrain",
              "view map without monsters or objects obstructing it",
              doterrain, IFBURIED | AUTOCOMPLETE, NULL },
    { '\0',   "therecmdmenu",
              "menu of commands you can do from here to adjacent spot",
              dotherecmdmenu, AUTOCOMPLETE | GENERALCMD | MOUSECMD, NULL },
    { 't',    "throw", "throw something",
              dothrow, 0, NULL },
    { '\0',   "timeout", "look at timeout queue and hero's timed intrinsics",
              wiz_timeout_queue, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { M('T'), "tip", "empty a container",
              dotip, AUTOCOMPLETE | CMD_M_PREFIX, NULL },
    { '_',    "travel", "travel to a specific location on the map",
              dotravel, CMD_M_PREFIX, NULL },
    { M('t'), "turn", "turn undead away",
              doturn, IFBURIED | AUTOCOMPLETE, NULL },
    { 'X',    "twoweapon", "toggle two-weapon combat",
              dotwoweapon, 0, NULL },
    { M('u'), "untrap", "untrap something",
              dountrap, AUTOCOMPLETE, NULL },
    { '<',    "up", "go up a staircase",
              /* (see comment for dodown() above */
              doup, CMD_M_PREFIX, NULL },
    { M('V'), "vanquished", "list vanquished monsters",
              dovanquished,
              IFBURIED | AUTOCOMPLETE | GENERALCMD | CMD_M_PREFIX, NULL },
    { M('v'), "version",
              "list compile time options for this version of NetHack",
              doextversion, IFBURIED | AUTOCOMPLETE | GENERALCMD, NULL },
    { 'v',    "versionshort", "show version and date+time program was built",
              doversion, IFBURIED | GENERALCMD, NULL },
    { '\0',   "vision", "show vision array",
              wiz_show_vision, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { '.',    "wait", "rest one move while doing nothing",
              donull, IFBURIED | CMD_M_PREFIX, "waiting" },
    { 'W',    "wear", "wear a piece of armor",
              dowear, 0, NULL },
    { '&',    "whatdoes", "tell what a command does",
              dowhatdoes, IFBURIED, NULL },
    { '/',    "whatis", "show what type of thing a symbol corresponds to",
              dowhatis, IFBURIED | GENERALCMD, NULL },
    { 'w',    "wield", "wield (put in use) a weapon",
              dowield, 0, NULL },
    { M('w'), "wipe", "wipe off your face",
              dowipe, AUTOCOMPLETE, NULL },
    { '\0',   "wizborn", "show stats of monsters created",
              doborn, IFBURIED | WIZMODECMD, NULL },
#ifdef DEBUG
    { '\0',   "wizbury", "bury objs under and around you",
              wiz_debug_cmd_bury, IFBURIED | AUTOCOMPLETE | WIZMODECMD,
              NULL },
#endif
    { '\0',   "wizcast", "cast any spell",
              dowizcast, IFBURIED | WIZMODECMD, NULL },
    { '\0',   "wizcustom", "show customized glyphs",
              wiz_custom, IFBURIED | WIZMODECMD | NOFUZZERCMD, NULL },
    { C('e'), "wizdetect", "reveal hidden things within a small radius",
              wiz_detect, IFBURIED | WIZMODECMD, NULL },
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED) || defined(DEBUG)
    { '\0',   "wizdispmacros", "validate the display macro ranges",
              wiz_display_macros, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
#endif
    { '\0',   "wizfliplevel", "flip the level",
              wiz_flip_level, IFBURIED | WIZMODECMD, NULL },
    { C('g'), "wizgenesis", "create a monster",
              wiz_genesis, IFBURIED | WIZMODECMD, NULL },
    { C('i'), "wizidentify", "identify all items in inventory",
              wiz_identify, IFBURIED | WIZMODECMD, NULL },
    { '\0',   "wizintrinsic", "set an intrinsic",
              wiz_intrinsic, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { '\0',   "wizkill", "slay a monster",
              wiz_kill, (IFBURIED | AUTOCOMPLETE | WIZMODECMD
                         | CMD_M_PREFIX | NOFUZZERCMD), NULL },
    { C('v'), "wizlevelport", "teleport to another level",
              wiz_level_tele, IFBURIED | WIZMODECMD | CMD_M_PREFIX, NULL },
    { '\0',   "wizloaddes", "load and execute a des-file lua script",
              wiz_load_splua, IFBURIED | WIZMODECMD | NOFUZZERCMD, NULL },
    { '\0',   "wizloadlua", "load and execute a lua script",
              wiz_load_lua, IFBURIED | WIZMODECMD | NOFUZZERCMD, NULL },
    { '\0',   "wizmakemap", "recreate the current level",
              wiz_makemap, IFBURIED | WIZMODECMD, NULL },
    { C('f'), "wizmap", "map the level",
              wiz_map, IFBURIED | WIZMODECMD, NULL },
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED) || defined(DEBUG)
    { '\0',   "wizmondiff", "validate the difficulty ratings of monsters",
              wiz_mon_diff, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
#endif
    { '\0',   "wizrumorcheck", "verify rumor boundaries",
              wiz_rumor_check, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { '\0',   "wizseenv", "show map locations' seen vectors",
              wiz_show_seenv, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { '\0',   "wizsmell", "smell monster",
              wiz_smell, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { '\0',   "wiztelekinesis", "telekinesis",
              wiz_telekinesis, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { '\0',   "wizwhere", "show locations of special levels",
              wiz_where, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { C('w'), "wizwish", "wish for something",
              wiz_wish, IFBURIED | WIZMODECMD, NULL },
    { '\0',   "wmode", "show wall modes",
              wiz_show_wmodes, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { 'z',    "zap", "zap a wand",
              dozap, 0, NULL },
    /* movement commands will be bound by reset_commands() */
    /* move or attack; accept m/g/G/F prefixes */
    { '\0', "movewest", "move west (screen left)",
            do_move_west, MOVEMENTCMD | CMD_MOVE_PREFIXES, NULL },
    { '\0', "movenorthwest", "move northwest (screen upper left)",
            do_move_northwest, MOVEMENTCMD | CMD_MOVE_PREFIXES, NULL },
    { '\0', "movenorth", "move north (screen up)",
            do_move_north, MOVEMENTCMD | CMD_MOVE_PREFIXES, NULL },
    { '\0', "movenortheast", "move northeast (screen upper right)",
            do_move_northeast, MOVEMENTCMD | CMD_MOVE_PREFIXES, NULL },
    { '\0', "moveeast", "move east (screen right)",
            do_move_east, MOVEMENTCMD | CMD_MOVE_PREFIXES, NULL },
    { '\0', "movesoutheast", "move southeast (screen lower right)",
            do_move_southeast, MOVEMENTCMD | CMD_MOVE_PREFIXES, NULL },
    { '\0', "movesouth", "move south (screen down)",
            do_move_south, MOVEMENTCMD | CMD_MOVE_PREFIXES, NULL },
    { '\0', "movesouthwest", "move southwest (screen lower left)",
            do_move_southwest, MOVEMENTCMD | CMD_MOVE_PREFIXES, NULL },
    /* rush; accept m prefix but not g/G/F */
    { '\0', "rushwest", "rush west (screen left)",
            do_rush_west, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "rushnorthwest", "rush northwest (screen upper left)",
            do_rush_northwest, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "rushnorth", "rush north (screen up)",
            do_rush_north, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "rushnortheast", "rush northeast (screen upper right)",
            do_rush_northeast, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "rusheast", "rush east (screen right)",
            do_rush_east, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "rushsoutheast", "rush southeast (screen lower right)",
            do_rush_southeast, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "rushsouth", "rush south (screen down)",
            do_rush_south, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "rushsouthwest", "rush southwest (screen lower left)",
            do_rush_southwest, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    /* run; accept m prefix but not g/G/F */
    { '\0', "runwest", "run west (screen left)",
            do_run_west, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "runnorthwest", "run northwest (screen upper left)",
            do_run_northwest, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "runnorth", "run north (screen up)",
            do_run_north, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "runnortheast", "run northeast (screen upper right)",
            do_run_northeast, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "runeast", "run east (screen right)",
            do_run_east, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "runsoutheast", "run southeast (screen lower right)",
            do_run_southeast, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "runsouth", "run south (screen down)",
            do_run_south, MOVEMENTCMD | CMD_M_PREFIX, NULL },
    { '\0', "runsouthwest", "run southwest (screen lower left)",
            do_run_southwest, MOVEMENTCMD | CMD_M_PREFIX, NULL },

    /* internal commands: only used by game core, not available for user */
    { '\0', "clicklook", NULL, doclicklook, INTERNALCMD | MOUSECMD, NULL },
    { '\0', "mouseaction", NULL, domouseaction, INTERNALCMD | MOUSECMD, NULL },
    { '\0', "altdip", NULL, dip_into, INTERNALCMD, NULL },
    { '\0', "altadjust", NULL, adjust_split, INTERNALCMD, NULL },
    { '\0', "altunwield", NULL, remarm_swapwep, INTERNALCMD, NULL },
    { '\0', (char *) 0, (char *) 0, donull, 0, (char *) 0 } /* sentinel */
};

/* mapping direction and move mode to extended command function */
static int (*move_funcs[N_DIRS_Z][N_MOVEMODES])(void) = {
    { do_move_west,      do_run_west,      do_rush_west },
    { do_move_northwest, do_run_northwest, do_rush_northwest },
    { do_move_north,     do_run_north,     do_rush_north },
    { do_move_northeast, do_run_northeast, do_rush_northeast },
    { do_move_east,      do_run_east,      do_rush_east },
    { do_move_southeast, do_run_southeast, do_rush_southeast },
    { do_move_south,     do_run_south,     do_rush_south },
    { do_move_southwest, do_run_southwest, do_rush_southwest },
    /* misleading; rush and run for down or up are rejected by rhack()
       because dodown() and doup() lack the CMD_gGF_PREFIX flag */
    { dodown,            dodown,           dodown },
    { doup,              doup,             doup },
};

/* used by dokeylist() and by key2extcmddesc() for dowhatdoes() */
static const struct {
    int nhkf;
    const char *desc;
    boolean numpad;
} misc_keys[] = {
    { NHKF_ESC, "cancel current prompt or pending prefix", FALSE },
    { NHKF_COUNT,
      "Prefix: for digits when preceding a command with a count", TRUE },
    { 0, (const char *) 0, FALSE }
};

static int extcmdlist_length = SIZE(extcmdlist) - 1;

/* get entry i in the extended commands list. for windowport use. */
struct ext_func_tab *
extcmds_getentry(int i)
{
    if (i < 0 || i > extcmdlist_length)
        return 0;
    return &extcmdlist[i];
}

/* return number of extended commands bound to a non-default key */
int
count_bind_keys(void)
{
    int nbinds = 0;
    int i;

    for (i = 0; i < extcmdlist_length; i++)
        if (extcmdlist[i].key
            && gc.Cmd.commands[extcmdlist[i].key] != &extcmdlist[i])
            nbinds++;
    return nbinds;
}

/* show changed key bindings in text, or if sbuf is non-null, append to it */
void
get_changed_key_binds(strbuf_t *sbuf)
{
    winid win = WIN_ERR;
    int i;
    char buf[BUFSZ];
    char buf2[QBUFSZ];

    if (!sbuf)
        win = create_nhwindow(NHW_TEXT);
    for (i = 0; i < extcmdlist_length; i++) {
        struct ext_func_tab *ec = &extcmdlist[i];

        if (ec->key && gc.Cmd.commands[ec->key]
            && gc.Cmd.commands[ec->key] != ec) {
            Sprintf(buf, "BIND=%s:%s%s", key2txt(ec->key, buf2),
                    gc.Cmd.commands[ec->key]->ef_txt,
                    sbuf ? "\n" : "");
            if (sbuf)
                strbuf_append(sbuf, buf);
            else
                putstr(win, 0, buf);
        }
    }
    if (!sbuf) {
        display_nhwindow(win, TRUE);
        destroy_nhwindow(win);
    }
}

/* interactive key binding */
staticfn void
handler_rebind_keys_add(boolean keyfirst)
{
    struct ext_func_tab *ec;
    winid win;
    anything any;
    int i, npick;
    menu_item *picks = (menu_item *) 0;
    char buf[BUFSZ];
    char buf2[QBUFSZ];
    uchar key = '\0';
    int clr = NO_COLOR;

    if (keyfirst) {
        pline("Bind which key? ");
        key = pgetchar();

        if (!key || key == '\027')
            return;
    }

    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;

    if (key) {
        if (gc.Cmd.commands[key]) {
            Sprintf(buf, "Key '%s' is currently bound to \"%s\".",
                    key2txt(key, buf2), gc.Cmd.commands[key]->ef_txt);
        } else {
            Sprintf(buf, "Key '%s' is not bound to anything.",
                    key2txt(key, buf2));
        }
        add_menu_str(win, buf);
        add_menu_str(win, "");
    }

    any.a_int = -1;
    add_menu(win, &nul_glyphinfo, &any, '\0', 0, ATR_NONE, clr,
             "nothing: unbind the key",
             MENU_ITEMFLAGS_NONE);

    add_menu_str(win, "");

    for (i = 0; i < extcmdlist_length; i++) {
        ec = &extcmdlist[i];

        if ((ec->flags & (MOVEMENTCMD|INTERNALCMD|CMD_NOT_AVAILABLE)) != 0)
            continue;

        any.a_int = (i + 1);
        Sprintf(buf, "%s: %s", ec->ef_txt, ec->ef_desc);
        add_menu(win, &nul_glyphinfo, &any, '\0', 0, ATR_NONE, clr, buf,
             MENU_ITEMFLAGS_NONE);
    }
    if (key)
        Sprintf(buf, "Bind '%s' to what command?", key2txt(key, buf2));
    else
        Sprintf(buf, "Bind what command?");
    end_menu(win, buf);
    npick = select_menu(win, PICK_ONE, &picks);
    destroy_nhwindow(win);
    if (npick > 0) {
        const struct ext_func_tab *prevec;
        const char *cmdstr;

        i = picks->item.a_int;
        free((genericptr_t) picks);

        if (i == -1) {
            ec = NULL;
            cmdstr = "nothing";
            goto bindit;
        } else {
            ec = &extcmdlist[i-1];
            cmdstr = ec->ef_txt;
        }
bindit:
        if (!key) {
            pline("Bind which key? ");
            key = pgetchar();

            if (!key || key == '\027')
                return;
        }

        prevec = gc.Cmd.commands[key];

        if (bind_key(key, cmdstr)) {
            if (prevec && prevec != ec) {
                pline("Changed key '%s' from \"%s\" to \"%s\".",
                      key2txt(key, buf2), prevec->ef_txt, cmdstr);
            } else if (!prevec) {
                pline("Bound key '%s' to \"%s\".", key2txt(key, buf2), cmdstr);
            }
        } else {
            pline("Key binding failed?!");
        }
    }
}

void
handler_rebind_keys(void)
{
    winid win;
    anything any;
    int i, npick;
    menu_item *picks = (menu_item *) 0;
    int clr = NO_COLOR;

redo_rebind:

    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;

    any.a_int = 1;
    add_menu(win, &nul_glyphinfo, &any, '\0', 0, ATR_NONE, clr,
             "bind key to a command", MENU_ITEMFLAGS_NONE);
    any.a_int = 2;
    add_menu(win, &nul_glyphinfo, &any, '\0', 0, ATR_NONE, clr,
             "bind command to a key", MENU_ITEMFLAGS_NONE);
    if (count_bind_keys()) {
        any.a_int = 3;
        add_menu(win, &nul_glyphinfo, &any, '\0', 0, ATR_NONE, clr,
                 "view changed key binds", MENU_ITEMFLAGS_NONE);
    }
    end_menu(win, "Do what?");
    npick = select_menu(win, PICK_ONE, &picks);
    destroy_nhwindow(win);
    if (npick > 0) {
        i = picks->item.a_int;
        free((genericptr_t) picks);

        if (i == 1 || i == 2) {
            handler_rebind_keys_add((i == 1));
        } else if (i == 3) {
            get_changed_key_binds(NULL);
        }
        goto redo_rebind;
    }
}

/* find extended command entries matching findstr.
   if findstr is NULL, returns all available entries.
   returns: number of matching extended commands,
            and the entry indexes in matchlist.
   for windowport use. */
int
extcmds_match(const char *findstr, int ecmflags, int **matchlist)
{
    static int retmatchlist[SIZE(extcmdlist)] = DUMMY;
    int i, mi = 0;
    int fslen = findstr ? Strlen(findstr) : 0;
    boolean ignoreac = (ecmflags & ECM_IGNOREAC) != 0;
    boolean exactmatch = (ecmflags & ECM_EXACTMATCH) != 0;
    boolean no1charcmd = (ecmflags & ECM_NO1CHARCMD) != 0;

    for (i = 0; extcmdlist[i].ef_txt; i++) {
        if (extcmdlist[i].flags & (CMD_NOT_AVAILABLE|INTERNALCMD))
            continue;
        if (!wizard && (extcmdlist[i].flags & WIZMODECMD))
            continue;
        if (!ignoreac && !(extcmdlist[i].flags & AUTOCOMPLETE))
            continue;
        if (no1charcmd && (strlen(extcmdlist[i].ef_txt) == 1))
            continue;
        if (!findstr) {
            retmatchlist[mi++] = i;
        } else if (exactmatch) {
            if (!strcmpi(findstr, extcmdlist[i].ef_txt)) {
                retmatchlist[mi++] = i;
            }
        } else {
            if (!strncmpi(findstr, extcmdlist[i].ef_txt, fslen)) {
                retmatchlist[mi++] = i;
            }
        }
    }

    if (matchlist)
        *matchlist = retmatchlist;

    return mi;
}

const char *
key2extcmddesc(uchar key)
{
    static char key2cmdbuf[QBUFSZ];
    const char *txt;
    int k, i, j;
    uchar M_5 = (uchar) M('5'), M_0 = (uchar) M('0');

    /* need to check for movement commands before checking the extended
       commands table because it contains entries for number_pad commands
       that match !number_pad movement (like 'j' for "jump") */
    key2cmdbuf[0] = '\0';
    if (movecmd(k = key, MV_WALK))
        Strcpy(key2cmdbuf, "move"); /* "move or attack"? */
    else if (movecmd(k = key, MV_RUSH))
        Strcpy(key2cmdbuf, "rush");
    else if (movecmd(k = key, MV_RUN))
        Strcpy(key2cmdbuf, "run");
    if (digit(key) || (gc.Cmd.num_pad && digit(unmeta(key)))) {
        key2cmdbuf[0] = '\0';
        if (!gc.Cmd.num_pad)
            Strcpy(key2cmdbuf, "start of, or continuation of, a count");
        else if (key == '5' || key == M_5)
            Sprintf(key2cmdbuf, "%s prefix",
                    (!!gc.Cmd.pcHack_compat ^ (key == M_5)) ? "run" : "rush");
        else if (key == '0' || (gc.Cmd.pcHack_compat && key == M_0))
            Strcpy(key2cmdbuf, "synonym for 'i'");
        if (*key2cmdbuf)
            return key2cmdbuf;
    }
    /* check prefixes before regular commands; includes ^A pseudo-command */
    for (i = 0; misc_keys[i].desc; ++i) {
        if (misc_keys[i].numpad && !iflags.num_pad)
            continue;
        j = misc_keys[i].nhkf;
        if (key == (uchar) gc.Cmd.spkeys[j])
            return misc_keys[i].desc;
    }
    /* finally, check whether 'key' is a command */
    if (gc.Cmd.commands[key] && (txt = gc.Cmd.commands[key]->ef_txt) != 0) {
        Sprintf(key2cmdbuf, "%s (#%s)", gc.Cmd.commands[key]->ef_desc, txt);

        /* special case: for reqmenu prefix (normally 'm'), replace
           "prefix: request menu or modify command (#reqmenu)"
           with two-line "movement prefix:...\nnon-movement prefix:..." */
        if (!strncmpi(key2cmdbuf, "prefix:", 7) && !strcmpi(txt, "reqmenu"))
            (void) strsubst(key2cmdbuf, "prefix:",
                     /* relies on implicit concatenation of literal strings */
                            "movement prefix:"
                            " move without autopickup and without attacking"
                            "\n"
                            "non-movement prefix:"); /* and rest of buf */

        /* another special case: 'txt' for '#' is "#" and showing that as
           "perform an extended command (##)" looks silly; strip "(##)" off */
        return strsubst(key2cmdbuf, " (##)", "");
    }
    return (char *) 0;
}

boolean
bind_mousebtn(int btn, const char *command)
{
    struct ext_func_tab *extcmd;

    if (btn < 1 || btn > NUM_MOUSE_BUTTONS) {
        config_error_add("Wrong mouse button, valid are 1-%i",
                         NUM_MOUSE_BUTTONS);
        return FALSE;
    }
    btn--;

    /* special case: "nothing" is reserved for unbinding */
    if (!strcmpi(command, "nothing")) {
        gc.Cmd.mousebtn[btn] = (struct ext_func_tab *) 0;
        return TRUE;
    }

    for (extcmd = extcmdlist; extcmd->ef_txt; extcmd++) {
        if (strcmpi(command, extcmd->ef_txt))
            continue;
        if (!(extcmd->flags & MOUSECMD))
            continue;
        gc.Cmd.mousebtn[btn] = extcmd;
#if 0 /* silently accept key binding for unavailable command (!SHELL,&c) */
        if ((extcmd->flags & CMD_NOT_AVAILABLE) != 0) {
            char buf[BUFSZ];

            Sprintf(buf, cmdnotavail, extcmd->ef_txt);
            config_error_add("%s", buf);
        }
#endif
        return TRUE;
    }

    return FALSE;
}

boolean
bind_key(uchar key, const char *command)
{
    struct ext_func_tab *extcmd;

    /* special case: "nothing" is reserved for unbinding */
    if (!strcmpi(command, "nothing")) {
        gc.Cmd.commands[key] = (struct ext_func_tab *) 0;
        return TRUE;
    }

    for (extcmd = extcmdlist; extcmd->ef_txt; extcmd++) {
        if (strcmpi(command, extcmd->ef_txt))
            continue;
        if ((extcmd->flags & INTERNALCMD) != 0)
            continue;
        gc.Cmd.commands[key] = extcmd;
#if 0 /* silently accept key binding for unavailable command (!SHELL,&c) */
        if ((extcmd->flags & CMD_NOT_AVAILABLE) != 0) {
            char buf[BUFSZ];

            Sprintf(buf, cmdnotavail, extcmd->ef_txt);
            config_error_add("%s", buf);
        }
#endif
        return TRUE;
    }

    return FALSE;
}

/* bind key by ext cmd function */
staticfn boolean
bind_key_fn(uchar key, int (*fn)(void))
{
    struct ext_func_tab *extcmd;

    for (extcmd = extcmdlist; extcmd->ef_txt; extcmd++) {
        if (extcmd->ef_funct != fn)
            continue;
        if ((extcmd->flags & INTERNALCMD) != 0)
            continue;
        gc.Cmd.commands[key] = extcmd;
        return TRUE;
    }

    return FALSE;
}

/* initialize all keyboard commands */
staticfn void
commands_init(void)
{
    struct ext_func_tab *extcmd;

    for (extcmd = extcmdlist; extcmd->ef_txt; extcmd++)
        if (extcmd->key)
            gc.Cmd.commands[extcmd->key] = extcmd;

    (void) bind_mousebtn(1, "therecmdmenu");
    (void) bind_mousebtn(2, "clicklook");

    /* number_pad */
    (void) bind_key(C('l'), "redraw");
    (void) bind_key('h',    "help");
    (void) bind_key('j',    "jump");
    (void) bind_key('k',    "kick");
    (void) bind_key('l',    "loot");
    (void) bind_key(C('n'), "annotate");
    (void) bind_key('N',    "name");
    (void) bind_key('u',    "untrap");
    (void) bind_key('5',    "run");
    (void) bind_key(M('5'), "rush");
    (void) bind_key('-',    "fight");

    /* alt keys: */
    (void) bind_key(M('O'), "overview");
    (void) bind_key(M('2'), "twoweapon");
    (void) bind_key(M('N'), "name");
#if 0
    /* don't do this until the rest_on_space option is set or cleared */
    (void) bind_key(' ',    "wait");
#endif
}

staticfn boolean
keylist_func_has_key(const struct ext_func_tab *extcmd,
                     boolean *skip_keys_used) /* boolean keys_used[256] */
{
    int i;

    for (i = 0; i < 256; ++i) {
        if (skip_keys_used[i])
            continue;

        if (gc.Cmd.commands[i] == extcmd)
            return TRUE;
    }
    return FALSE;
}

staticfn int
keylist_putcmds(winid datawin, boolean docount,
                int incl_flags, int excl_flags,
                boolean *keys_used) /* boolean keys_used[256] */
{
    const struct ext_func_tab *extcmd;
    int i;
    char buf[BUFSZ], buf2[QBUFSZ];
    boolean keys_already_used[256]; /* copy of keys_used[] before updates */
    int count = 0;

    for (i = 0; i < 256; i++) {
        uchar key = (uchar) i;

        keys_already_used[i] = keys_used[i];
        if (keys_used[i])
            continue;
        if (key == ' ' && !flags.rest_on_space)
            continue;
        if ((extcmd = gc.Cmd.commands[i]) != (struct ext_func_tab *) 0) {
            if ((incl_flags && !(extcmd->flags & incl_flags))
                || (excl_flags && (extcmd->flags & excl_flags)))
                continue;
            if (docount) {
                count++;
                continue;
            }
            Sprintf(buf, "%-7s %-13s %s", key2txt(key, buf2),
                    extcmd->ef_txt, extcmd->ef_desc);
            putstr(datawin, 0, buf);
            keys_used[i] = TRUE;
        }
    }
    /* also list commands that lack key assignments; most are wizard mode */
    for (extcmd = extcmdlist; extcmd->ef_txt; ++extcmd) {
        if ((incl_flags && !(extcmd->flags & incl_flags))
            || (excl_flags && (extcmd->flags & excl_flags)))
            continue;
        /* can't just check for non-Null extcmd->key; it holds the
           default assignment and a user-specified binding might hijack
           this command's default key for some other command; or this
           command might have been assigned a key being used for
           movement or as a prefix, intercepting that keystroke */
        if (keylist_func_has_key(extcmd, keys_already_used))
            continue;
        /* found a command for current category without any key assignment */
        if (docount) {
            count++;
            continue;
        }
        /* '#'+20 for one column here == 7+' '+13 for two columns above */
        Sprintf(buf, "#%-20s %s", extcmd->ef_txt, extcmd->ef_desc);
        putstr(datawin, 0, buf);
    }
    return count;
}

/* list all keys and their bindings, like dat/hh but dynamic */
void
dokeylist(void)
{
    const struct ext_func_tab *extcmd;
    winid datawin;
    char buf[BUFSZ], buf2[BUFSZ];
    uchar key;
    boolean spkey_gap, keys_used[256], mov_seen[256];
    int i, j, pfx_seen[256];

    (void) memset((genericptr_t) keys_used, 0, sizeof keys_used);
    (void) memset((genericptr_t) pfx_seen, 0, sizeof pfx_seen);

#ifndef NO_SIGNAL
    /* this is actually ambiguous; tty raw mode will override SIGINT;
       when enabled, treat it like a movement command since assigning
       other commands to this keystroke would be unwise... */
    key = (uchar) C('c');
    keys_used[key] = TRUE;
#endif

    /* movement keys have been flagged in keys_used[]; clone them */
    (void) memcpy((genericptr_t) mov_seen, (genericptr_t) keys_used,
                  sizeof mov_seen);

    spkey_gap = FALSE;
    for (i = 0; misc_keys[i].desc; ++i) {
        if (misc_keys[i].numpad && !iflags.num_pad)
            continue;
        j = misc_keys[i].nhkf;
        key = (uchar) gc.Cmd.spkeys[j];
        if (key && !mov_seen[key] && !pfx_seen[key]) {
            keys_used[key] = TRUE;
            pfx_seen[key] = j;
        } else
            spkey_gap = TRUE;
    }

    datawin = create_nhwindow(NHW_TEXT);
    putstr(datawin, 0, "");
    Sprintf(buf, "%7s %s", "", "    Full Current Key Bindings List");
    putstr(datawin, 0, buf);
    for (extcmd = extcmdlist; extcmd->ef_txt; ++extcmd)
        if (spkey_gap || !keylist_func_has_key(extcmd, keys_used)) {
            Sprintf(buf, "%7s %s", "",
                               "(also commands with no key assignment)");
            putstr(datawin, 0, buf);
            break;
        }

    /* directional keys */
    putstr(datawin, 0, "");
    putstr(datawin, 0, "Directional keys:");
    show_direction_keys(datawin, '.', FALSE); /* '.'==self in direct'n grid */

    if (!iflags.num_pad) {
        putstr(datawin, 0, "");
        putstr(datawin, 0,
     "Ctrl+<direction> will run in specified direction until something very");
        Sprintf(buf, "%7s %s", "", "interesting is seen.");
        putstr(datawin, 0, buf);
        Strcpy(buf, "Shift"); /* append the rest below */
    } else {
        /* num_pad */
        putstr(datawin, 0, "");
        Strcpy(buf, "Meta"); /* append the rest next */
    }
    Strcat(buf,
          "+<direction> will run in specified direction until you encounter");
    putstr(datawin, 0, buf);
    Sprintf(buf, "%7s %s", "", "an obstacle.");
    putstr(datawin, 0, buf);

    putstr(datawin, 0, "");
    putstr(datawin, 0, "Miscellaneous keys:");
    for (i = 0; misc_keys[i].desc; ++i) {
        if (misc_keys[i].numpad && !iflags.num_pad)
            continue;
        j = misc_keys[i].nhkf;
        key = (uchar) gc.Cmd.spkeys[j];
        if (key && !mov_seen[key]
            && (pfx_seen[key] == j)) {
            Sprintf(buf, "%-7s %s", key2txt(key, buf2), misc_keys[i].desc);
            putstr(datawin, 0, buf);
        }
    }
    /* (see above) */
    key = (uchar) C('c');
#ifndef NO_SIGNAL
    /* last of the special keys */
    Sprintf(buf, "%-7s", key2txt(key, buf2));
#else
    /* first of the keyless commands */
    Sprintf(buf2, "[%s]", key2txt(key, buf));
    Sprintf(buf, "%-21s", buf2);
#endif
    Strcat(buf, " interrupt: break out of NetHack (SIGINT)");
    putstr(datawin, 0, buf);
    /* keyless special key commands, if any */
    if (spkey_gap) {
        for (i = 0; misc_keys[i].desc; ++i) {
            if (misc_keys[i].numpad && !iflags.num_pad)
                continue;
            j = misc_keys[i].nhkf;
            key = (uchar) gc.Cmd.spkeys[j];
            if (!key || (pfx_seen[key] != j)) {
                Sprintf(buf2, "[%s]", spkey_name(j));
                /* lines up with the other unassigned commands which use
                   "#%-20s ", but not with the other special keys */
                Snprintf(buf, sizeof(buf), "%-21s %s", buf2,
                         misc_keys[i].desc);
                putstr(datawin, 0, buf);
            }
        }
    }

#define IGNORECMD (WIZMODECMD | INTERNALCMD | MOVEMENTCMD)

    putstr(datawin, 0, "");
    show_menu_controls(datawin, TRUE);

    if (keylist_putcmds(datawin, TRUE, GENERALCMD, IGNORECMD, keys_used)) {
        putstr(datawin, 0, "");
        putstr(datawin, 0, "General commands:");
        (void) keylist_putcmds(datawin, FALSE, GENERALCMD,
                               IGNORECMD, keys_used);
    }

    if (keylist_putcmds(datawin, TRUE, 0, GENERALCMD | IGNORECMD, keys_used)) {
        putstr(datawin, 0, "");
        putstr(datawin, 0, "Game commands:");
        (void) keylist_putcmds(datawin, FALSE, 0,
                               GENERALCMD | IGNORECMD,
                               keys_used);
    }

    if (wizard && keylist_putcmds(datawin, TRUE,
                                  WIZMODECMD, INTERNALCMD, keys_used)) {
        putstr(datawin, 0, "");
        putstr(datawin, 0, "Debug mode commands:");
        (void) keylist_putcmds(datawin, FALSE,
                               WIZMODECMD, INTERNALCMD, keys_used);
    }

    display_nhwindow(datawin, FALSE);
    destroy_nhwindow(datawin);
#undef IGNORECMD
}

const struct ext_func_tab *
ext_func_tab_from_func(int (*fn)(void))
{
    const struct ext_func_tab *extcmd;

    for (extcmd = extcmdlist; extcmd->ef_txt; ++extcmd)
        if (extcmd->ef_funct == fn)
            return extcmd;

    return NULL;
}

/* returns the key bound to a movement command for given DIR_ and MV_ mode */
char
cmd_from_dir(int dir, int mode)
{
    return cmd_from_func(move_funcs[dir][mode]);
}

/* return the key bound to extended command */
char
cmd_from_func(int (*fn)(void))
{
    int i;
    char ret = '\0';

    /* skip NUL; allowing it would wreak havoc */
    for (i = 1; i < 256; ++i) {
        /* skip space; we'll use it below as last resort if no other
           keystroke invokes space's command */
        if (i == ' ')
            continue;
        /* skip digits if number_pad is Off; also skip '-' unless it has
           been bound to something other than what number_pad assigns */
        if (((i >= '0' && i <= '9') || (i == '-' && fn == do_fight))
            && !gc.Cmd.num_pad)
            continue;

        if (gc.Cmd.commands[i] && gc.Cmd.commands[i]->ef_funct == fn) {
            if (i >= ' ' && i <= '~')
                return (char) i;
            else {
                ret = (char) i;
            }
        }
    }
    if (gc.Cmd.commands[' '] && gc.Cmd.commands[' ']->ef_funct == fn)
        return ' ';
    return ret;
}

/* return visual interpretation of the key bound to extended command,
   or the ext cmd name if not bound to any key. */
char *
cmd_from_ecname(const char *ecname)
{
    static char cmdnamebuf[QBUFSZ];
    const struct ext_func_tab *extcmd;

    for (extcmd = extcmdlist; extcmd->ef_txt; ++extcmd)
        if (!strcmp(extcmd->ef_txt, ecname)) {
            char key = cmd_from_func(extcmd->ef_funct);

            if (key)
                Sprintf(cmdnamebuf, "%s", visctrl(key));
            else
                Sprintf(cmdnamebuf, "#%s", ecname);
            return cmdnamebuf;
        }

    cmdnamebuf[0] = '\0';
    return cmdnamebuf;
}

const char *
ecname_from_fn(int (*fn)(void))
{
    const struct ext_func_tab *extcmd, *cmdptr = 0;

    for (extcmd = extcmdlist; extcmd->ef_txt; ++extcmd)
        if (extcmd->ef_funct == fn) {
            cmdptr = extcmd;
            return cmdptr->ef_txt;
        }
    return (char *) 0;
}

/* return extended command name (without leading '#') for command (*fn)() */
const char *
cmdname_from_func(
    int (*fn)(void),  /* function whose command name is wanted */
    char outbuf[],    /* place to store the result */
    boolean fullname) /* False: just enough to disambiguate */
{
    const struct ext_func_tab *extcmd, *cmdptr = 0;
    const char *res = 0;

    for (extcmd = extcmdlist; extcmd->ef_txt; ++extcmd)
        if (extcmd->ef_funct == fn) {
            cmdptr = extcmd;
            res = cmdptr->ef_txt;
            break;
        }

    if (!res) {
        /* make sure output buffer doesn't contain junk or stale data;
           return Null below */
        outbuf[0] = '\0';
    } else if (fullname) {
        /* easy; the entire command name */
        res = strcpy(outbuf, res);
    } else {
        const struct ext_func_tab *matchcmd = extcmdlist;
        unsigned len = 0, maxlen = Strlen(res);

        /* find the shortest leading substring which is unambiguous */
        do {
            if (++len >= maxlen)
                break;
            for (extcmd = matchcmd; extcmd->ef_txt; ++extcmd) {
                if (extcmd == cmdptr)
                    continue;
                if ((extcmd->flags & CMD_NOT_AVAILABLE) != 0
                    || ((extcmd->flags & WIZMODECMD) != 0 && !wizard))
                    continue;
                if (!strncmp(res, extcmd->ef_txt, len)) {
                    matchcmd = extcmd;
                    break;
                }
            }
        } while (extcmd->ef_txt);
        copynchars(outbuf, res, len);
        /* [note: for Qt, this debugpline writes a couple dozen lines to
            stdout during menu setup when message window isn't ready yet] */
        debugpline2("shortened %s: \"%s\"", res, outbuf);
        res = outbuf;
    }
    return res;
}

static struct {
    int nhkf;
    uchar key;
    const char *name;
} const spkeys_binds[] = {
    { NHKF_ESC,              '\033', (char *) 0 }, /* no binding */
    { NHKF_GETDIR_SELF,      '.', "getdir.self" },
    { NHKF_GETDIR_SELF2,     's', "getdir.self2" },
    { NHKF_GETDIR_HELP,      '?', "getdir.help" },
    { NHKF_GETDIR_MOUSE,     '_', "getdir.mouse" },
    { NHKF_COUNT,            'n', "count" },
    { NHKF_GETPOS_SELF,      '@', "getpos.self" },
    { NHKF_GETPOS_PICK,      '.', "getpos.pick" },
    { NHKF_GETPOS_PICK_Q,    ',', "getpos.pick.quick" },
    { NHKF_GETPOS_PICK_O,    ';', "getpos.pick.once" },
    { NHKF_GETPOS_PICK_V,    ':', "getpos.pick.verbose" },
    { NHKF_GETPOS_SHOWVALID, '$', "getpos.valid" },
    { NHKF_GETPOS_AUTODESC,  '#', "getpos.autodescribe" },
    { NHKF_GETPOS_MON_NEXT,  'm', "getpos.mon.next" },
    { NHKF_GETPOS_MON_PREV,  'M', "getpos.mon.prev" },
    { NHKF_GETPOS_OBJ_NEXT,  'o', "getpos.obj.next" },
    { NHKF_GETPOS_OBJ_PREV,  'O', "getpos.obj.prev" },
    { NHKF_GETPOS_DOOR_NEXT, 'd', "getpos.door.next" },
    { NHKF_GETPOS_DOOR_PREV, 'D', "getpos.door.prev" },
    { NHKF_GETPOS_UNEX_NEXT, 'x', "getpos.unexplored.next" },
    { NHKF_GETPOS_UNEX_PREV, 'X', "getpos.unexplored.prev" },
    { NHKF_GETPOS_VALID_NEXT, 'z', "getpos.valid.next" },
    { NHKF_GETPOS_VALID_PREV, 'Z', "getpos.valid.prev" },
    { NHKF_GETPOS_INTERESTING_NEXT, 'a', "getpos.all.next" },
    { NHKF_GETPOS_INTERESTING_PREV, 'A', "getpos.all.prev" },
    { NHKF_GETPOS_HELP,      '?', "getpos.help" },
    { NHKF_GETPOS_LIMITVIEW, '"', "getpos.filter" },
    { NHKF_GETPOS_MOVESKIP,  '*', "getpos.moveskip" },
    { NHKF_GETPOS_MENU,      '!', "getpos.menu" }
};

boolean
bind_specialkey(uchar key, const char *command)
{
    int i;

    for (i = 0; i < SIZE(spkeys_binds); i++) {
        if (!spkeys_binds[i].name || strcmp(command, spkeys_binds[i].name))
            continue;
        gc.Cmd.spkeys[spkeys_binds[i].nhkf] = key;
        return TRUE;
    }
    return FALSE;
}

staticfn const char *
spkey_name(int nhkf)
{
    const char *name = 0;
    int i;

    for (i = 0; i < SIZE(spkeys_binds); i++) {
        if (spkeys_binds[i].nhkf == nhkf) {
            name = (nhkf == NHKF_ESC) ? "escape" : spkeys_binds[i].name;
            break;
        }
    }
    return name;
}

/* returns the text for a one-byte encoding;
 * must be shorter than a tab for proper formatting */
char *
key2txt(uchar c, char *txt) /* sufficiently long buffer */
{
    /* should probably switch to "SPC", "ESC", "RET"
       since nethack's documentation uses ESC for <escape> */
    if (c == ' ')
        Sprintf(txt, "<space>");
    else if (c == '\033')
        Sprintf(txt, "<esc>"); /* "<escape>" won't fit */
    else if (c == '\n')
        Sprintf(txt, "<enter>"); /* "<return>" won't fit */
    else if (c == '\177')
        Sprintf(txt, "<del>"); /* "<delete>" won't fit */
    else
        Strcpy(txt, visctrl((char) c));
    return txt;
}


void
parseautocomplete(char *autocomplete, boolean condition)
{
    struct ext_func_tab *efp;
    char *autoc;

    /* break off first autocomplete from the rest; parse the rest */
    if ((autoc = strchr(autocomplete, ',')) != 0
        || (autoc = strchr(autocomplete, ':')) != 0) {
        *autoc++ = '\0';
        parseautocomplete(autoc, condition);
    }

    /* strip leading and trailing white space */
    autocomplete = trimspaces(autocomplete);

    if (!*autocomplete)
        return;

    /* take off negation */
    if (*autocomplete == '!') {
        /* unlike most options, a leading "no" might actually be a part of
         * the extended command.  Thus you have to use ! */
        autocomplete++;
        autocomplete = trimspaces(autocomplete);
        condition = !condition;
    }

    /* find and modify the extended command */
    for (efp = extcmdlist; efp->ef_txt; efp++) {
        if (!strcmp(autocomplete, efp->ef_txt)) {
            if (condition == ((efp->flags & AUTOCOMPLETE) ? FALSE : TRUE))
                efp->flags |= AUTOCOMP_ADJ;
            if (condition)
                efp->flags |= AUTOCOMPLETE;
            else
                efp->flags &= ~AUTOCOMPLETE;
            return;
        }
    }

    /* not a real extended command */
    raw_printf("Bad autocomplete: invalid extended command '%s'.",
               autocomplete);
    wait_synch();
}

/* add changed autocompletions to the string buffer in config file format */
void
all_options_autocomplete(strbuf_t *sbuf)
{
    struct ext_func_tab *efp;
    char buf[BUFSZ];

    for (efp = extcmdlist; efp->ef_txt; efp++)
        if ((efp->flags & AUTOCOMP_ADJ) != 0) {
            Sprintf(buf, "AUTOCOMPLETE=%s%s\n",
                    (efp->flags & AUTOCOMPLETE) ? "" : "!",
                    efp->ef_txt);
            strbuf_append(sbuf, buf);
        }
}

/* save&clear the mouse button actions, or restore the saved ones */
void
lock_mouse_buttons(boolean savebtns)
{
    static const struct ext_func_tab *mousebtn[NUM_MOUSE_BUTTONS] = { 0 };
    int i;

    if (savebtns) {
        for (i = 0; i < NUM_MOUSE_BUTTONS; i++) {
            mousebtn[i] = gc.Cmd.mousebtn[i];
            gc.Cmd.mousebtn[i] = NULL;
        }
    } else {
        for (i = 0; i < NUM_MOUSE_BUTTONS; i++)
            gc.Cmd.mousebtn[i] = mousebtn[i];
    }
}

/* called at startup and after number_pad is twiddled */
void
reset_commands(boolean initial)
{
    static const char sdir[] = "hykulnjb><",
                      sdir_swap_yz[] = "hzkulnjb><",
                      ndir[] = "47896321><",
                      ndir_phone_layout[] = "41236987><";
    static const int ylist[] = {
        'y', 'Y', C('y'), M('y'), M('Y'), M(C('y'))
    };
    static struct ext_func_tab *back_dir_cmd[N_DIRS][N_MOVEMODES];
    static uchar back_dir_key[N_DIRS][N_MOVEMODES];
    static boolean backed_dir_cmd = FALSE;
    const struct ext_func_tab *cmdtmp;
    boolean flagtemp;
    int c, i, updated = 0;
    int dir, mode;

    if (initial) {
        updated = 1;
        gc.Cmd.num_pad = FALSE;
        gc.Cmd.pcHack_compat = gc.Cmd.phone_layout = gc.Cmd.swap_yz = FALSE;
        for (i = 0; i < SIZE(spkeys_binds); i++)
            gc.Cmd.spkeys[spkeys_binds[i].nhkf] = spkeys_binds[i].key;
        commands_init();
    } else {
        if (backed_dir_cmd) {
            for (dir = 0; dir < N_DIRS; dir++) {
                for (mode = 0; mode < N_MOVEMODES; mode++) {
                    gc.Cmd.commands[back_dir_key[dir][mode]]
                        = back_dir_cmd[dir][mode];
                }
            }
        }

        /* basic num_pad */
        flagtemp = iflags.num_pad;
        if (flagtemp != gc.Cmd.num_pad) {
            gc.Cmd.num_pad = flagtemp;
            ++updated;
        }
        /* swap_yz mode (only applicable for !num_pad); intended for
           QWERTZ keyboard used in Central Europe, particularly Germany */
        flagtemp = (iflags.num_pad_mode & 1) ? !gc.Cmd.num_pad : FALSE;
        if (flagtemp != gc.Cmd.swap_yz) {
            gc.Cmd.swap_yz = flagtemp;
            ++updated;
            /* FIXME? should Cmd.spkeys[] be scanned for y and/or z to swap?
               Cmd.swap_yz has been toggled;
               perform the swap (or reverse previous one) */
            for (i = 0; i < SIZE(ylist); i++) {
                c = ylist[i] & 0xff;
                cmdtmp = gc.Cmd.commands[c];                /* tmp = [y] */
                gc.Cmd.commands[c] = gc.Cmd.commands[c + 1]; /* [y] = [z] */
                gc.Cmd.commands[c + 1] = cmdtmp;            /* [z] = tmp */
            }
        }
        /* MSDOS compatibility mode (only applicable for num_pad) */
        flagtemp = (iflags.num_pad_mode & 1) ? gc.Cmd.num_pad : FALSE;
        if (flagtemp != gc.Cmd.pcHack_compat) {
            gc.Cmd.pcHack_compat = flagtemp;
            ++updated;
            /* pcHack_compat has been toggled */
#if 0
            c = M('5') & 0xff;
            cmdtmp = gc.Cmd.commands['5'];
            gc.Cmd.commands['5'] = gc.Cmd.commands[c];
            gc.Cmd.commands[c] = cmdtmp;
#endif
            /* FIXME: NHKF_DOINV2 ought to be implemented instead of this */
            c = M('0') & 0xff;
            gc.Cmd.commands[c] = gc.Cmd.pcHack_compat ? gc.Cmd.commands['I']
                                                      : 0;
        }
        /* phone keypad layout (only applicable for num_pad) */
        flagtemp = (iflags.num_pad_mode & 2) ? gc.Cmd.num_pad : FALSE;
        if (flagtemp != gc.Cmd.phone_layout) {
            gc.Cmd.phone_layout = flagtemp;
            ++updated;
            /* phone_layout has been toggled */
            for (i = 0; i < 3; i++) {
                c = '1' + i;             /* 1,2,3 <-> 7,8,9 */
                cmdtmp = gc.Cmd.commands[c];                 /* tmp = [1] */
                gc.Cmd.commands[c] = gc.Cmd.commands[c + 6]; /* [1] = [7] */
                gc.Cmd.commands[c + 6] = cmdtmp;             /* [7] = tmp */
                c = (M('1') & 0xff) + i; /* M-1,M-2,M-3 <-> M-7,M-8,M-9 */
                cmdtmp = gc.Cmd.commands[c];                 /* tmp = [M-1] */
                gc.Cmd.commands[c] = gc.Cmd.commands[c + 6]; /* [M-1]=[M-7] */
                gc.Cmd.commands[c + 6] = cmdtmp;             /* [M-7] = tmp */
            }
        }
    } /*?initial*/

    /* choose updated movement keys */
    if (updated)
        gc.Cmd.serialno++;
    gc.Cmd.dirchars = !gc.Cmd.num_pad
                       ? (!gc.Cmd.swap_yz ? sdir : sdir_swap_yz)
                       : (!gc.Cmd.phone_layout ? ndir : ndir_phone_layout);
    gc.Cmd.alphadirchars = !gc.Cmd.num_pad ? gc.Cmd.dirchars : sdir;

    /* back up the commands & keys overwritten by new movement keys */
    for (dir = 0; dir < N_DIRS; dir++) {
        for (mode = MV_WALK; mode < N_MOVEMODES; mode++) {
            uchar di = (uchar) gc.Cmd.dirchars[dir];

            if (!gc.Cmd.num_pad) {
                if (mode == MV_RUN) di = highc(di);
                else if (mode == MV_RUSH) di = C(di);
            } else {
                if (mode == MV_RUN) di = M(di);
                else if (mode == MV_RUSH) di = M(di);
            }
            back_dir_key[dir][mode] = di;
            back_dir_cmd[dir][mode]
                = (struct ext_func_tab *) gc.Cmd.commands[di];
            gc.Cmd.commands[di] = (struct ext_func_tab *) 0;
        }
    }
    backed_dir_cmd = TRUE;

    /* bind the new keys to movement commands */
    for (i = 0; i < N_DIRS; i++) {
        (void) bind_key_fn(gc.Cmd.dirchars[i], move_funcs[i][MV_WALK]);
        if (!gc.Cmd.num_pad) {
            (void) bind_key_fn(highc(gc.Cmd.dirchars[i]),
                               move_funcs[i][MV_RUN]);
            (void) bind_key_fn(C(gc.Cmd.dirchars[i]), move_funcs[i][MV_RUSH]);
        } else {
            /* M(number) works when altmeta is on */
            (void) bind_key_fn(M(gc.Cmd.dirchars[i]), move_funcs[i][MV_RUN]);
            /* can't bind highc() or C() of digits. just use the 5 prefix. */
        }
    }
    update_rest_on_space();
    gc.Cmd.extcmd_char = cmd_from_func(doextcmd);
}

/* called when 'rest_on_space' is toggled, also called by reset_commands()
   from initoptions_init() which takes place before key bindings have been
   processed, and by initoptions_finish() after key bindings so that we
   can remember anything bound to <space> in 'unrestonspace' */
void
update_rest_on_space(void)
{
    /* cloned from extcmdlist['.'], then slightly modified to be distinct;
       donull is all that's needed for it to operate; command name and
       description get shown by help menu's "Info on what a given key does"
       (which runs the '&' command) and "Full list of keyboard commands" */
    static const struct ext_func_tab restonspace = {
        ' ', "wait", "rest one move via 'rest_on_space' option",
        donull, (IFBURIED | CMD_M_PREFIX), "waiting"
    };
    static const struct ext_func_tab *unrestonspace = 0;
    const struct ext_func_tab *bound_f = gc.Cmd.commands[' '];

    /* when 'rest_on_space' is On, <space> will run the #wait command;
       when it is Off, <space> will use 'unrestonspace' which will either
       be Null and elicit "Unknown command ' '." or have some non-Null
       command bound in player's RC file */
    if (bound_f != 0 && bound_f != &restonspace)
        unrestonspace = bound_f;
    gc.Cmd.commands[' '] = flags.rest_on_space ? &restonspace : unrestonspace;
}

/* commands which accept 'm' prefix to request menu operation or other
   alternate behavior; it's also overloaded for move-without-autopickup;
   there is no overlap between the two groups of commands */
staticfn boolean
accept_menu_prefix(const struct ext_func_tab *ec)
{
    return (ec && ((ec->flags & CMD_M_PREFIX) != 0));
}

char
randomkey(void)
{
    static unsigned i = 0;
    char c;

    switch (rn2(16)) {
    default:
        c = '\033';
        break;
    case 0:
        c = '\n';
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        c = (char) rn1('~' - ' ' + 1, ' ');
        break;
    case 5:
        c = (char) (rn2(2) ? '\t' : ' ');
        break;
    case 6:
        c = (char) rn1('z' - 'a' + 1, 'a');
        break;
    case 7:
        c = (char) rn1('Z' - 'A' + 1, 'A');
        break;
    case 8:
        c = extcmdlist[i++ % SIZE(extcmdlist)].key;
        break;
    case 9:
        c = '#';
        break;
    case 10:
    case 11:
    case 12:
        {
            int d = rn2(N_DIRS);
            int m = rn2(7) ? MV_WALK : (!rn2(3) ? MV_RUSH : MV_RUN);

            c = cmd_from_dir(d, m);
        }
        break;
    case 13:
        c = (char) rn1('9' - '0' + 1, '0');
        break;
    case 14:
        /* any char, but avoid '\0' because it's used for mouse click */
        c = (char) rnd(iflags.wc_eight_bit_input ? 255 : 127);
        break;
    }

    return c;
}

void
random_response(char *buf, int sz)
{
    char c;
    int count = 0;

    for (;;) {
        c = randomkey();
        if (c == '\n')
            break;
        if (c == '\033') {
            count = 0;
            break;
        }
        if (count < sz - 1)
            buf[count++] = c;
    }
    buf[count] = '\0';
}

int
rnd_extcmd_idx(void)
{
    return rn2(extcmdlist_length + 1) - 1;
}

staticfn void
reset_cmd_vars(boolean reset_cmdq)
{
    gc.context.run = 0;
    gc.context.nopick = gc.context.forcefight = FALSE;
    gc.context.move = gc.context.mv = FALSE;
    gd.domove_attempting = 0;
    gm.multi = 0;
    iflags.menu_requested = FALSE;
    gc.context.travel = gc.context.travel1 = 0;
    if (gt.travelmap) {
        selection_free(gt.travelmap, TRUE);
        gt.travelmap = NULL;
    }
    if (reset_cmdq) {
        cmdq_clear(CQ_CANNED);
        cmdq_clear(CQ_REPEAT);
    }
}

void
rhack(int key)
{
    boolean bad_command, firsttime = (key == 0);
    struct _cmd_queue cq, *cmdq = NULL;
    const struct ext_func_tab *cmdq_ec = 0, *prefix_seen = 0;
    boolean was_m_prefix = FALSE;
    int (*func)(void) = dummyfunction;

    iflags.menu_requested = FALSE;
    gc.context.nopick = 0;
 got_prefix_input:
#ifdef SAFERHANGUP
    if (gp.program_state.done_hup)
        end_of_input();
#endif
    if ((cmdq = cmdq_pop()) != 0) {
        /* doing queued commands */
        cq = *cmdq;
        free(cmdq);
        if (cq.typ == CMDQ_EXTCMD && (cmdq_ec = cq.ec_entry) != 0)
            goto do_cmdq_extcmd;
        /* already handled a queued command (goto do_cmdq_extcmd);
           if something other than a key is queued, we'll drop down
           to the !*cmd handling which clears out the command-queue */
        key = (cq.typ == CMDQ_KEY) ? cq.key : 0;
    } else if (firsttime) {
        key = parse();
        /* parse() pushed a cmd but didn't return any key */
        if (!key && cmdq_peek(CQ_CANNED))
            goto got_prefix_input;
    }

    /* if there's no command, there's nothing to do except reset */
    if (!key || key == (char) 0377
        || key == gc.Cmd.spkeys[NHKF_ESC]) {
        if (!key || key != gc.Cmd.spkeys[NHKF_ESC])
            nhbell();
        reset_cmd_vars(TRUE);
        return;
    }

    /* handle most movement commands */
    gc.context.travel = gc.context.travel1 = 0;
    {
        const struct ext_func_tab *tlist;
        int res;

 do_cmdq_extcmd:
        if (cmdq_ec)
            tlist = cmdq_ec;
        else
            tlist = gc.Cmd.commands[key & 0xff];

        /* current - use key to directly index cmdlist array */
        if (tlist != 0) {
            if (!can_do_extcmd(tlist)) {
                /* can_do_extcmd() already gave a message */
                reset_cmd_vars(TRUE);
                res = ECMD_OK;
            } else if (prefix_seen && !(tlist->flags & PREFIXCMD)
                       && !(tlist->flags & (was_m_prefix ? CMD_M_PREFIX
                                                         : CMD_gGF_PREFIX))) {
                char pfxidx = cmd_from_func(prefix_seen->ef_funct);
                const char *which = (pfxidx != 0) ? visctrl(pfxidx)
                                    : (prefix_seen->ef_funct == do_reqmenu)
                                      ? "move-no-pickup or request-menu"
                                      : prefix_seen->ef_txt;

                /*
                 * We got a prefix previously and looped for another
                 * command instead of returning, but the command we got
                 * doesn't accept a prefix.  The feedback here supersedes
                 * the former call to help_dir() (for 'bad_command' below).
                 */
                if (was_m_prefix) {
                    pline("The %s command does not accept '%s' prefix.",
                          tlist->ef_txt, which);
                } else {
                    uchar ch = tlist->key;
                    boolean up = (ch == '<' || tlist->ef_funct == doup),
                            down = (ch == '>' || tlist->ef_funct == dodown);

                    pline(
                "The '%s' prefix should be followed by a movement command%s.",
                          which, (up || down) ? " other than up or down" : "");
                }
                res = ECMD_FAIL;
                prefix_seen = 0;
                was_m_prefix = FALSE;
            } else {
                /* we discard 'const' because some compilers seem to have
                   trouble with the pointer passed to set_occupation() */
                func = ((struct ext_func_tab *) tlist)->ef_funct;
                if (tlist->f_text && !go.occupation && gm.multi)
                    set_occupation(func, tlist->f_text, gm.multi);
                ge.ext_tlist = NULL;

                if (!gi.in_doagain && func != do_repeat && func != doextcmd) {
                    if (!prefix_seen)
                        cmdq_clear(CQ_REPEAT);
                    cmdq_add_ec(CQ_REPEAT,
                                ((struct ext_func_tab *) tlist)->ef_funct);
                } else {
                    if (func == doextcmd) {
                        cmdq_clear(CQ_REPEAT);
                    }
                }
                /* some commands shouldn't trigger sanity_check() because
                   if it produces output that might interfere with them;
                   note: if sanity_check is False, this has no effect */
                if ((tlist->flags & CMD_INSANE) != 0)
                    iflags.sanity_no_check = iflags.sanity_check;

                res = (*func)(); /* perform the command */
                /* if 'func' is doextcmd(), 'tlist' is for Cmd.commands['#']
                   rather than for the command that doextcmd() just ran;
                   doextcmd() notifies us what that was via ext_tlist;
                   other commands leave it Null */
                if (ge.ext_tlist) {
                    tlist = ge.ext_tlist, ge.ext_tlist = NULL;
                    /* Add the command post-execution */
                    cmdq_add_ec(CQ_REPEAT,
                                ((struct ext_func_tab *) tlist)->ef_funct);
                    /* shift the command to first */
                    cmdq_shift(CQ_REPEAT);
                }

                if ((tlist->flags & PREFIXCMD) != 0) {
                    /* it was a prefix command, mark and get another cmd */
                    if ((res & ECMD_CANCEL) != 0) {
                        /* prefix commands cancel if pressed twice */
                        reset_cmd_vars(TRUE);
                        return;
                    }
                    prefix_seen = tlist;
                    bad_command = FALSE;
                    cmdq_ec = NULL;
                    if (func == do_reqmenu)
                        was_m_prefix = TRUE;
                    goto got_prefix_input;
                } else if (!(tlist->flags & MOVEMENTCMD)
                           && gd.domove_attempting) {
                    /* not a movement command, but a move prefix earlier? */
                    ; /* just do nothing */
                } else if (((gd.domove_attempting
                             & (DOMOVE_RUSH | DOMOVE_WALK)) != 0L)
                           && !gc.context.travel && !dxdy_moveok()) {
                    /* trying to move diagonally as a grid bug */
                    You_cant("get there from here...");
                    reset_cmd_vars(TRUE);
                    return;
                } else if ((gd.domove_attempting & DOMOVE_WALK) != 0L) {
                    if (gm.multi)
                        gc.context.mv = TRUE;
                    domove();
                    gc.context.forcefight = 0;
                    iflags.menu_requested = FALSE;
                    return;
                } else if ((gd.domove_attempting & DOMOVE_RUSH) != 0L) {
                    if (firsttime) {
                        if (!gm.multi)
                            gm.multi = max(COLNO, ROWNO);
                        u.last_str_turn = 0;
                    }
                    gc.context.mv = TRUE;
                    domove();
                    iflags.menu_requested = FALSE;
                    return;
                }
                prefix_seen = 0;
                was_m_prefix = FALSE;
            }
            /* it is possible to have a result of (ECMD_TIME|ECMD_CANCEL)
               [for example, using 'f'ire, manually filling quiver with
               wielded weapon or dual-wielded swap-weapon, then cancelling
               at the direction prompt; using time to unwield should take
               precedence over general cancellation] */
            if ((res & (ECMD_CANCEL | ECMD_FAIL)) != 0) {
                /* command was canceled by user, maybe they declined to
                   pick an object to act on, or command failed to finish */
                reset_cmd_vars(TRUE);
            } else if ((res & (ECMD_OK | ECMD_TIME)) == ECMD_OK) {
                reset_cmd_vars(gm.multi < 0);
            }
            /* reset_cmd_vars() sets context.move to False so we might
               need to change it [back] to True */
            if ((res & ECMD_TIME) != 0) {
                gc.context.move = TRUE;
                if (func != dokick) {
                    /* hero did something else than kicking a location;
                       reset the location, so pets don't avoid it */
                    gk.kickedloc.x = 0, gk.kickedloc.y = 0;
                }
            }
            return;
        }
        /* if we reach here, cmd wasn't found in cmdlist[] */
        bad_command = TRUE;
    }

    if (bad_command) {
        Norep("Unknown command '%s'.", visctrl(key));
        cmdq_clear(CQ_CANNED);
        cmdq_clear(CQ_REPEAT);
    }
    /* didn't move */
    gc.context.move = FALSE;
    gm.multi = 0;
    return;
}

/* convert an x,y pair into a direction code */
coordxy
xytod(coordxy x, coordxy y)
{
    int dd;

    for (dd = 0; dd < N_DIRS; dd++)
        if (x == xdir[dd] && y == ydir[dd])
            return dd;
    return DIR_ERR;
}

/* convert a direction code into an x,y pair */
void
dtoxy(coord *cc, int dd)
{
    if (dd > DIR_ERR && dd < N_DIRS_Z) {
        cc->x = xdir[dd];
        cc->y = ydir[dd];
    }
}

/* also sets u.dz, but returns false for <> */
int
movecmd(char sym, int mode)
{
    int d = DIR_ERR;

    if (gc.Cmd.commands[(uchar) sym]) {
        int (*fnc)(void) = gc.Cmd.commands[(uchar) sym]->ef_funct;

        if (mode == MV_ANY) {
            for (d = N_DIRS_Z - 1; d > DIR_ERR; d--)
                if (fnc == move_funcs[d][MV_WALK]
                    || fnc == move_funcs[d][MV_RUN]
                    || fnc == move_funcs[d][MV_RUSH])
                    break;
        } else {
            for (d = N_DIRS_Z - 1; d > DIR_ERR; d--)
                if (fnc == move_funcs[d][mode])
                    break;
        }
    }

    if (d != DIR_ERR) {
        u.dx = xdir[d];
        u.dy = ydir[d];
        u.dz = zdir[d];
        return !u.dz;
    }
    u.dz = 0;
    return 0;
}

/* grid bug handling */
int
dxdy_moveok(void)
{
    if (u.dx && u.dy && NODIAG(u.umonnum))
        u.dx = u.dy = 0;
    return u.dx || u.dy;
}

/* decide whether character (user input keystroke) requests screen repaint */
boolean
redraw_cmd(char c)
{
    uchar uc = (uchar) c;
    const struct ext_func_tab *cmd = gc.Cmd.commands[uc];

    return (boolean) (cmd && cmd->ef_funct == doredraw);
}

/*
 * uses getdir() but unlike getdir() it specifically
 * produces coordinates using the direction from getdir()
 * and verifies that those coordinates are ok.
 *
 * If the call to getdir() returns 0, Never_mind is displayed.
 * If the resulting coordinates are not okay, emsg is displayed.
 *
 * Returns non-zero if coordinates in cc are valid.
 */
int
get_adjacent_loc(
    const char *prompt,
    const char *emsg,
    coordxy x, coordxy y,
    coord *cc)
{
    coordxy new_x, new_y;
    if (!getdir(prompt)) {
        pline1(Never_mind);
        return 0;
    }
    new_x = x + u.dx;
    new_y = y + u.dy;
    if (cc && isok(new_x, new_y)) {
        cc->x = new_x;
        cc->y = new_y;
    } else {
        if (emsg)
            pline1(emsg);
        return 0;
    }
    return 1;
}

/* prompt for a direction (specified via movement keystroke) and return it
   in u.dx, u.dy, and u.dz; function return value is 1 for ok, 0 otherwise */
int
getdir(const char *s)
{
    char dirsym;
    int is_mov;
    struct _cmd_queue *cmdq = cmdq_pop();

    if (cmdq) {
        if (cmdq->typ == CMDQ_DIR) {
            if (!cmdq->dirz) {
                dirsym = gc.Cmd.dirchars[xytod(cmdq->dirx, cmdq->diry)];
            } else {
                dirsym = gc.Cmd.dirchars[(cmdq->dirz > 0) ? DIR_DOWN : DIR_UP];
            }
        } else if (cmdq->typ == CMDQ_KEY) {
            dirsym = cmdq->key;
        } else {
            cmdq_clear(CQ_CANNED);
            dirsym = '\0';
            impossible("getdir: command queue had no dir?");
        }
        free(cmdq);
        goto got_dirsym;
    }

 retry:
    gp.program_state.input_state = getdirInp;
    if (gi.in_doagain || *readchar_queue)
        dirsym = readchar();
    else
        dirsym = yn_function((s && *s != '^') ? s : "In what direction?",
                             (char *) 0, '\0', FALSE);
    /* remove the prompt string so caller won't have to */
    clear_nhwindow(WIN_MESSAGE);

    if (redraw_cmd(dirsym)) { /* ^R */
        docrt();              /* redraw */
        goto retry;
    }
    if (!gi.in_doagain)
        cmdq_add_key(CQ_REPEAT, dirsym);

 got_dirsym:
    if (dirsym == gc.Cmd.spkeys[NHKF_GETDIR_SELF]
        || dirsym == gc.Cmd.spkeys[NHKF_GETDIR_SELF2]) {
        u.dx = u.dy = u.dz = 0;
    } else if (dirsym == gc.Cmd.spkeys[NHKF_GETDIR_MOUSE]) {
        char qbuf[QBUFSZ];
        coord cc;
        int pos, mod;

        /*
         * For #therecmdmenu:
         * Player has entered the 'simulated mouse' key ('_' by default)
         * at the "which direction?" prompt so we use getpos() to get a
         * simulated click after moving cursor to the desired location.
         *
         * getpos() returns 0..3 for period, comma, semi-colon, colon.
         * We treat "," as left click and "." as right click due to
         * their positions relative to each other on the keyboard.
         * Using ";" as synonym for "," and ":" for "." is due to their
         * shapes rather than to their keyboard location.
         *
         * Those keys aren't separately bindable for being treated as
         * clicks but we do honor their getpos bindings if player has
         * changed them.  (Bound values might have scrambled keyboard
         * locations relative to each other so ruin the memory aid of
         * "," being left of ".".)
         */
        Sprintf(qbuf,
            "desired location, then type '%s' for left click, '%s' for right",
                /* visctrl() cycles through several static buffers for its
                   return value so using two in the same expression is ok */
                visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK_Q]), /* ',' */
                visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK])); /* '.' */
        cc.x = u.ux, cc.y = u.uy; /* starting cursor location for getpos() */
        pos = getpos(&cc, TRUE, qbuf);

        if (pos < 0) {
            /* ESC or other rejection */
            u.dx = u.dy = u.dz = 0;
            mod = 0; /* neither CLICK_1 nor CLICK_2 */
        } else {
            /* caller expects simulated click to be relative to hero's spot */
            u.dx = cc.x - u.ux;
            u.dy = cc.y - u.uy;
            /* non-zero getdir_click actually means ok to click farther than
               one spot away from hero; adjacent click is always allowed */
            if (!iflags.getdir_click) {
                u.dx = sgn(u.dx);
                u.dy = sgn(u.dy);
            }
            u.dz = 0;

            switch (pos + NHKF_GETPOS_PICK) {
            case NHKF_GETPOS_PICK_Q: /* 1: quick:   ',' */
            case NHKF_GETPOS_PICK_O: /* 2: once:    ';' */
                mod = CLICK_1;
                break;
            case NHKF_GETPOS_PICK:   /* 0: normal:  '.' */
            case NHKF_GETPOS_PICK_V: /* 3: verbose: ':' */
                mod = CLICK_2;
                break;
            default:
                /* could plug in bound values for spkeys[NHKF_GETPOS_PICK],&c
                   but that feels like overkill for something which should
                   never happen; just show their default values */
                impossible("getpos successful but not one of [.,;:] (%d)",
                           pos);
                mod = 0; /* neither CLICK_1 nor CLICK_2 */
                pos = -1; /* return failure */
                break;
            }
        }
        if (iflags.getdir_click)
            iflags.getdir_click = mod;
        return (pos >= 0);
    } else if (!(is_mov = movecmd(dirsym, MV_ANY)) && !u.dz) {
        boolean did_help = FALSE, help_requested;

        if (!strchr(quitchars, dirsym)) {
            help_requested = (dirsym == gc.Cmd.spkeys[NHKF_GETDIR_HELP]);
            if (help_requested || iflags.cmdassist) {
                did_help = help_dir((s && *s == '^') ? dirsym : '\0',
                                    gc.Cmd.spkeys[NHKF_ESC],
                                    help_requested ? (const char *) 0
                                                   : "Invalid direction key!");
                if (help_requested)
                    goto retry;
            }
            if (!did_help)
                pline("What a strange direction!");
        }
        return 0;
    } else if (is_mov && !dxdy_moveok()) {
        You_cant("orient yourself that direction.");
        return 0;
    }
    if (!u.dz)
        confdir(FALSE);
    return 1;
}

staticfn void
show_direction_keys(
    winid win, /* should specify a window which is using a fixed-width font */
    char centerchar, /* '.' or '@' or ' ' */
    boolean nodiag)
{
    char buf[BUFSZ];

    if (!centerchar)
        centerchar = ' ';

    if (nodiag) {
        Sprintf(buf, "             %s   ",
                visctrl(cmd_from_func(do_move_north)));
        putstr(win, 0, buf);
        putstr(win, 0, "             |   ");
        Sprintf(buf, "          %s- %c -%s",
                visctrl(cmd_from_func(do_move_west)),
                centerchar,
                visctrl(cmd_from_func(do_move_east)));
        putstr(win, 0, buf);
        putstr(win, 0, "             |   ");
        Sprintf(buf, "             %s   ",
                visctrl(cmd_from_func(do_move_south)));
        putstr(win, 0, buf);
    } else {
        Sprintf(buf, "          %s  %s  %s",
                visctrl(cmd_from_func(do_move_northwest)),
                visctrl(cmd_from_func(do_move_north)),
                visctrl(cmd_from_func(do_move_northeast)));
        putstr(win, 0, buf);
        putstr(win, 0, "           \\ | / ");
        Sprintf(buf, "          %s- %c -%s",
                visctrl(cmd_from_func(do_move_west)),
                centerchar,
                visctrl(cmd_from_func(do_move_east)));
        putstr(win, 0, buf);
        putstr(win, 0, "           / | \\ ");
        Sprintf(buf, "          %s  %s  %s",
                visctrl(cmd_from_func(do_move_southwest)),
                visctrl(cmd_from_func(do_move_south)),
                visctrl(cmd_from_func(do_move_southeast)));
        putstr(win, 0, buf);
    };
}

/* explain choices if player has asked for getdir() help or has given
   an invalid direction after a prefix key ('F', 'g', 'm', &c), which
   might be bogus but could be up, down, or self when not applicable */
staticfn boolean
help_dir(
    char sym,
    uchar spkey, /* actual key; either prefix or ESC */
    const char *msg)
{
    static const char wiz_only_list[] = "EFGIVW";
    char ctrl;
    winid win;
    char buf[BUFSZ], buf2[BUFSZ], *explain;
    const char *dothat /*, *how */;
    boolean prefixhandling /*, viawindow */;

    /* NHKF_ESC indicates that player asked for help at getdir prompt */
    /* viawindow = (spkey == gc.Cmd.spkeys[NHKF_ESC] || iflags.cmdassist); */
    prefixhandling = (spkey != gc.Cmd.spkeys[NHKF_ESC]);
    /*
     * Handling for prefix keys that don't want special directions.
     * Delivered via pline if 'cmdassist' is off, or instead of the
     * general message if it's on.
     */
    dothat = "do that";
    /* how = " at"; */ /* for "<action> at yourself"; not used for up/down */

    buf[0] = '\0';
#if 0   /* Since prefix keys got 'promoted' to commands, feedback for
         * invalid prefix is done in rhack() these days.
         */
    /* for movement prefix followed by '.' or (numpad && 's') to mean 'self';
       note: '-' for hands (inventory form of 'self') is not handled here */
    if (prefixhandling
        && (sym == gc.Cmd.spkeys[NHKF_GETDIR_SELF]
            || (gc.Cmd.num_pad && sym == gc.Cmd.spkeys[NHKF_GETDIR_SELF2]))) {
        Sprintf(buf, "You can't %s%s yourself.", dothat, how);
    /* for movement prefix followed by up or down */
    } else if (prefixhandling && (sym == '<' || sym == '>')) {
        Sprintf(buf, "You can't %s %s.", dothat,
                /* was "upwards" and "downwards", but they're considered
                   to be variants of canonical "upward" and "downward" */
                (sym == '<') ? "upward" : "downward");
    }

    /* if '!cmdassist', display via pline() and we're done (note: asking
       for help at getdir() prompt forces cmdassist for this operation) */
    if (!viawindow) {
        if (prefixhandling) {
            if (!*buf)
                Sprintf(buf, "Invalid direction for '%s' prefix.",
                        visctrl(spkey));
            pline("%s", buf);
            return TRUE;
        }
        /* when 'cmdassist' is off and caller doesn't insist, do nothing */
        return FALSE;
    }
#else
    nhUse(prefixhandling);
#endif

    win = create_nhwindow(NHW_TEXT);
    if (!win)
        return FALSE;

    if (*buf) {
        /* show bad-prefix message instead of general invalid-direction one */
        putstr(win, 0, buf);
        putstr(win, 0, "");
    } else if (msg) {
        Sprintf(buf, "cmdassist: %s", msg);
        putstr(win, 0, buf);
        putstr(win, 0, "");
    }

    if (!prefixhandling && (letter(sym) || sym == '[')) {
        /* '[': old 'cmdhelp' showed ESC as ^[ */
        sym = highc(sym); /* @A-Z[ (note: letter() accepts '@') */
        ctrl = (sym - 'A') + 1; /* 0-27 (note: 28-31 aren't applicable) */
        if ((explain = dowhatdoes_core(ctrl, buf2)) != 0
            && (!strchr(wiz_only_list, sym) || wizard)) {
            Sprintf(buf, "Are you trying to use ^%c%s?", sym,
                    strchr(wiz_only_list, sym) ? ""
                        : " as specified in the Guidebook");
            putstr(win, 0, buf);
            putstr(win, 0, "");
            putstr(win, 0, explain);
            putstr(win, 0, "");
            putstr(win, 0,
                  "To use that command, hold down the <Ctrl> key as a shift");
            Sprintf(buf, "and press the <%c> key.", sym);
            putstr(win, 0, buf);
            putstr(win, 0, "");
        }
    }

    Sprintf(buf, "Valid direction keys%s%s%s are:",
            prefixhandling ? " to " : "", prefixhandling ? dothat : "",
            NODIAG(u.umonnum) ? " in your current form" : "");
    putstr(win, 0, buf);
    show_direction_keys(win, !prefixhandling ? '.' : ' ', NODIAG(u.umonnum));

    if (!prefixhandling) {
        /* NOPICKUP: unlike the other prefix keys, 'm' allows up/down for
           stair traversal; we won't get here when "m<" or "m>" has been
           given but we include up and down for 'm'+invalid_direction;
           self is excluded as a viable direction for every prefix */
        putstr(win, 0, "");
        putstr(win, 0, "          <  up");
        putstr(win, 0, "          >  down");
        if (!prefixhandling) {
            int selfi = gc.Cmd.num_pad ? NHKF_GETDIR_SELF2 : NHKF_GETDIR_SELF;

            Sprintf(buf,   "       %4s  direct at yourself",
                    visctrl(gc.Cmd.spkeys[selfi]));
            putstr(win, 0, buf);
        }
    }

    if (msg) {
        /* non-null msg means that this wasn't an explicit user request */
        putstr(win, 0, "");
        putstr(win, 0,
               "(Suppress this message with !cmdassist in config file.)");
    }
    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);
    return TRUE;
}

/* if hero is impaired, pick random movement direction */
void
confdir(boolean force_impairment)
{
    if (force_impairment || u_maybe_impaired()) {
        int kmax = NODIAG(u.umonnum) ? (N_DIRS / 2) : N_DIRS,
            k = (int) dirs_ord[rn2(kmax)];

        u.dx = xdir[k];
        u.dy = ydir[k];
    }
    return;
}

const char *
directionname(int dir)
{
    static NEARDATA const char *const dirnames[N_DIRS_Z] = {
        "west",      "northwest", "north",     "northeast", "east",
        "southeast", "south",     "southwest", "down",      "up",
    };

    if (dir < 0 || dir >= N_DIRS_Z)
        return "invalid";
    return dirnames[dir];
}

int
isok(coordxy x, coordxy y)
{
    /* x corresponds to curx, so x==1 is the first column. Ach. %% */
    return x >= 1 && x <= COLNO - 1 && y >= 0 && y <= ROWNO - 1;
}

/* #herecmdmenu command */
staticfn int
doherecmdmenu(void)
{
    char ch = here_cmd_menu();

    return (ch && ch != '\033') ? ECMD_TIME : ECMD_OK;
}

/* #therecmdmenu command, a way to test there_cmd_menu without mouse */
staticfn int
dotherecmdmenu(void)
{
    char ch;
    int dir, click;
    coordxy x = gc.clicklook_cc.x;
    coordxy y = gc.clicklook_cc.y;

    iflags.getdir_click = CLICK_1 | CLICK_2; /* allow 'far' click */

    if (isok(x, y)) {
        if (x == u.ux && y == u.uy)
            ch = here_cmd_menu();
        else
            ch = there_cmd_menu(x, y, iflags.getdir_click);
        gc.clicklook_cc.x = gc.clicklook_cc.y = -1;
        iflags.getdir_click = 0;
        return (ch && ch != '\033') ? ECMD_TIME : ECMD_OK;
    }

    dir = getdir((const char *) 0);
    click = iflags.getdir_click;
    iflags.getdir_click = 0;

    if (!dir || !isok(u.ux + u.dx, u.uy + u.dy))
        return ECMD_CANCEL;

    if (u.dx || u.dy)
        ch = there_cmd_menu(u.ux + u.dx, u.uy + u.dy, click);
    else
        ch = here_cmd_menu();

    return (ch && ch != '\033') ? ECMD_TIME : ECMD_OK;
}

/* commands for [t]herecmdmenu */
enum menucmd {
    MCMD_NOTHING = 0,
    MCMD_OPEN_DOOR,
    MCMD_LOCK_DOOR,
    MCMD_UNTRAP_DOOR,
    MCMD_KICK_DOOR,
    MCMD_CLOSE_DOOR,
    MCMD_SEARCH,
    MCMD_LOOK_TRAP,
    MCMD_UNTRAP_TRAP,
    MCMD_MOVE_DIR,
    MCMD_RIDE,
    MCMD_REMOVE_SADDLE,
    MCMD_APPLY_SADDLE,
    MCMD_TALK,
    MCMD_NAME,

    MCMD_QUAFF,
    MCMD_DIP,
    MCMD_SIT,
    MCMD_UP,
    MCMD_DOWN,
    MCMD_DISMOUNT,
    MCMD_MONABILITY,
    MCMD_PICKUP,
    MCMD_LOOT,
    MCMD_TIP,
    MCMD_EAT,
    MCMD_DROP,
    MCMD_REST,
    MCMD_LOOK_HERE,
    MCMD_LOOK_AT,
    MCMD_ATTACK_NEXT2U,
    MCMD_UNTRAP_HERE,
    MCMD_OFFER,
    MCMD_INVENTORY,
    MCMD_CAST_SPELL,

    MCMD_THROW_OBJ,
    MCMD_TRAVEL,
};

staticfn void
mcmd_addmenu(winid win, int act, const char *txt)
{
    anything any;
    int clr = NO_COLOR;

    /* TODO: fixed letters for the menu entries? */
    any = cg.zeroany;
    any.a_int = act;
    add_menu(win, &nul_glyphinfo, &any, '\0', 0, ATR_NONE, clr, txt,
             MENU_ITEMFLAGS_NONE);
}

/* command menu entries when targeting self */
staticfn int
there_cmd_menu_self(winid win, coordxy x, coordxy y, int *act UNUSED)
{
    int K = 0;
    char buf[BUFSZ];
    schar typ = levl[x][y].typ;
    stairway *stway = stairway_at(x, y);
    struct trap *ttmp;

    if (!u_at(x, y))
        return K;

    if ((IS_FOUNTAIN(typ) || IS_SINK(typ)) && can_reach_floor(FALSE)) {
        Sprintf(buf, "Drink from the %s",
                defsyms[IS_FOUNTAIN(typ) ? S_fountain : S_sink].explanation);
        mcmd_addmenu(win, MCMD_QUAFF, buf), ++K;
    }
    if (IS_FOUNTAIN(typ) && can_reach_floor(FALSE))
        mcmd_addmenu(win, MCMD_DIP, "Dip something into the fountain"), ++K;
    if (IS_THRONE(typ))
        mcmd_addmenu(win, MCMD_SIT, "Sit on the throne"), ++K;
    if (IS_ALTAR(typ))
        mcmd_addmenu(win, MCMD_OFFER, "Sacrifice something on the altar"), ++K;

    if (stway && stway->up) {
        Sprintf(buf, "Go up the %s",
                stway->isladder ? "ladder" : "stairs");
        mcmd_addmenu(win, MCMD_UP, buf), ++K;
    }
    if (stway && !stway->up) {
        Sprintf(buf, "Go down the %s",
                stway->isladder ? "ladder" : "stairs");
        mcmd_addmenu(win, MCMD_DOWN, buf), ++K;
    }
    if (u.usteed) { /* another movement choice */
        Sprintf(buf, "Dismount %s",
                x_monnam(u.usteed, ARTICLE_THE, (char *) 0,
                         SUPPRESS_SADDLE, FALSE));
        mcmd_addmenu(win, MCMD_DISMOUNT, buf), ++K;
    }

#if 0
    if (Upolyd) { /* before objects */
        Sprintf(buf, "Use %s special ability",
                s_suffix(pmname(&mons[u.umonnum], Ugender)));
        mcmd_addmenu(win, MCMD_MONABILITY, buf), ++K;
    }
#endif

    if (OBJ_AT(x, y)) {
        struct obj *otmp = gl.level.objects[x][y];

        Sprintf(buf, "Pick up %s", otmp->nexthere ? "items" : doname(otmp));
        mcmd_addmenu(win, MCMD_PICKUP, buf), ++K;

        if (Is_container(otmp)) {
            Sprintf(buf, "Loot %s", doname(otmp));
            mcmd_addmenu(win, MCMD_LOOT, buf), ++K;

            Sprintf(buf, "Tip %s", doname(otmp));
            mcmd_addmenu(win, MCMD_TIP, buf), ++K;
        }
        if (otmp->oclass == FOOD_CLASS) {
            Sprintf(buf, "Eat %s", doname(otmp));
            mcmd_addmenu(win, MCMD_EAT, buf), ++K;
        }
    }


    if (gi.invent) {
        mcmd_addmenu(win, MCMD_INVENTORY, "Inventory"), ++K;
        mcmd_addmenu(win, MCMD_DROP, "Drop items"), ++K;
    }
    mcmd_addmenu(win, MCMD_REST, "Rest one turn"), ++K;
    mcmd_addmenu(win, MCMD_SEARCH, "Search around you"), ++K;
    mcmd_addmenu(win, MCMD_LOOK_HERE, "Look at what is here"), ++K;

    if (num_spells() > 0)
        mcmd_addmenu(win, MCMD_CAST_SPELL, "Cast a spell"), ++K;

    if ((ttmp = t_at(x, y)) != 0 && ttmp->tseen) {
        if (ttmp->ttyp != VIBRATING_SQUARE)
            mcmd_addmenu(win, MCMD_UNTRAP_HERE,
                         "Attempt to disarm trap"), ++K;
    }
    return K;
}

/* add entries to there_cmd_menu, when x,y is next to hero */
staticfn int
there_cmd_menu_next2u(
    winid win,
    coordxy x, coordxy y,
    int mod,
    int *act)
{
    int K = 0;
    char buf[BUFSZ];
    schar typ = levl[x][y].typ;
    struct trap *ttmp;
    struct monst *mtmp;

    if (!next2u(x, y))
        return K;

    if (IS_DOOR(typ)) {
        boolean key_or_pick, card;
        int dm = levl[x][y].doormask;

        if ((dm & (D_CLOSED | D_LOCKED))) {
            mcmd_addmenu(win, MCMD_OPEN_DOOR, "Open the door"), ++K;
            /* unfortunately there's no lknown flag for doors to
               remember the locked/unlocked state */
            key_or_pick = (carrying(SKELETON_KEY) || carrying(LOCK_PICK));
            card = (carrying(CREDIT_CARD) != 0);
            if (key_or_pick || card) {
                Sprintf(buf, "%sunlock the door",
                        key_or_pick ? "lock or " : "");
                mcmd_addmenu(win, MCMD_LOCK_DOOR, upstart(buf)), ++K;
            }
            /* unfortunately there's no tknown flag for doors (or chests)
               to remember whether a trap had been found */
            mcmd_addmenu(win, MCMD_UNTRAP_DOOR,
                         "Search the door for a trap"), ++K;
            /* [what about #force?] */
            mcmd_addmenu(win, MCMD_KICK_DOOR, "Kick the door"), ++K;
        } else if ((dm & D_ISOPEN) && (mod == CLICK_2)) {
            mcmd_addmenu(win, MCMD_CLOSE_DOOR, "Close the door"), ++K;
        }
    }

    if (typ <= SCORR)
        mcmd_addmenu(win, MCMD_SEARCH, "Search for secret doors"), ++K;

    if ((ttmp = t_at(x, y)) != 0 && ttmp->tseen) {
        mcmd_addmenu(win, MCMD_LOOK_TRAP, "Examine trap"), ++K;
        if (ttmp->ttyp != VIBRATING_SQUARE)
            mcmd_addmenu(win, MCMD_UNTRAP_TRAP,
                                 "Attempt to disarm trap"), ++K;
        mcmd_addmenu(win, MCMD_MOVE_DIR, "Move on the trap"), ++K;
    }

    if (levl[x][y].glyph == objnum_to_glyph(BOULDER))
        mcmd_addmenu(win, MCMD_MOVE_DIR, "Push the boulder"), ++K;

    mtmp = m_at(x, y);
    if (mtmp && !canspotmon(mtmp))
        mtmp = 0;
    if (mtmp && which_armor(mtmp, W_SADDLE)) {
        char *mnam = x_monnam(mtmp, ARTICLE_THE, (char *) 0,
                              SUPPRESS_SADDLE, FALSE);

        if (!u.usteed) {
            Sprintf(buf, "Ride %s", mnam);
            mcmd_addmenu(win, MCMD_RIDE, buf), ++K;
        }
        Sprintf(buf, "Remove saddle from %s", mnam);
        mcmd_addmenu(win, MCMD_REMOVE_SADDLE, buf), ++K;
    }
    if (mtmp && can_saddle(mtmp) && !which_armor(mtmp, W_SADDLE)
        && carrying(SADDLE)) {
        Sprintf(buf, "Put saddle on %s", mon_nam(mtmp));
        mcmd_addmenu(win, MCMD_APPLY_SADDLE, buf), ++K;
    }
    if (mtmp && (mtmp->mpeaceful || mtmp->mtame)) {
        Sprintf(buf, "Talk to %s", mon_nam(mtmp));
        mcmd_addmenu(win, MCMD_TALK, buf), ++K;

        Sprintf(buf, "Swap places with %s", mon_nam(mtmp));
        mcmd_addmenu(win, MCMD_MOVE_DIR, buf), ++K;

        Sprintf(buf, "%s %s",
                !has_mgivenname(mtmp) ? "Name" : "Rename",
                mon_nam(mtmp));
        mcmd_addmenu(win, MCMD_NAME, buf), ++K;
    }

    if ((mtmp && !(mtmp->mpeaceful || mtmp->mtame))
        || glyph_is_invisible(glyph_at(x, y))) {
        Sprintf(buf, "Attack %s", mtmp ? mon_nam(mtmp) : "unseen creature");
        mcmd_addmenu(win, MCMD_ATTACK_NEXT2U, buf), ++K;
        /* attacking overrides any other automatic action */
        *act = MCMD_ATTACK_NEXT2U;
    } else {
        /* "Move %s", direction - handled below */
    }
    return K;
}

staticfn int
there_cmd_menu_far(winid win, coordxy x, coordxy y, int mod)
{
    int K = 0;

    if (mod == CLICK_1) {
        if (linedup(u.ux, u.uy, x, y, 1)
            && dist2(u.ux, u.uy, x, y) < 18*18)
            mcmd_addmenu(win, MCMD_THROW_OBJ, "Throw something"), ++K;

        mcmd_addmenu(win, MCMD_TRAVEL, "Travel here"), ++K;
    }
    return K;
}

staticfn int
there_cmd_menu_common(
    winid win,
    coordxy x, coordxy y,
    int mod,
    int *act UNUSED)
{
    int K = 0;

    if (mod == CLICK_1 || mod == CLICK_2) { /* ignore iflags.clicklook here */
        /* for self, only include "look at map symbol" if it isn't the
           ordinary hero symbol (steed, invisible w/o see invisible, ?) */
        if (!u_at(x, y) || Upolyd || glyph_at(x, y) != hero_glyph)
            mcmd_addmenu(win, MCMD_LOOK_AT, "Look at map symbol"), ++K;
    }
    return K;
}

/* queue up command(s) to perform #therecmdmenu action */
staticfn void
act_on_act(
    int act,                /* action */
    coordxy dx, coordxy dy) /* delta to adjacent spot (farther sometimes) */
{
    struct obj *otmp;
    int dir;

    /* a few there_cmd_menu_far() actions use dx,dy differently */
    switch (act) {
    case MCMD_THROW_OBJ:
    case MCMD_TRAVEL:
    case MCMD_LOOK_AT:
        /* keep dx,dy as-is */
        break;
    default:
        /* force dx and dy to be +1, 0, or -1 */
        dx = sgn(dx);
        dy = sgn(dy);
        break;
    }

    switch (act) {
    case MCMD_TRAVEL:
        /* FIXME: player has explicitly picked "travel to this location"
           from the menu but it will only work if flags.travelcmd is True.
           That option is intended as way to guard against stray mouse
           clicks and shouldn't inhibit explicit travel. */
        iflags.travelcc.x = u.tx = u.ux + dx;
        iflags.travelcc.y = u.ty = u.uy + dy;
        cmdq_add_ec(CQ_CANNED, dotravel_target);
        break;
    case MCMD_THROW_OBJ:
        cmdq_add_ec(CQ_CANNED, dothrow);
        cmdq_add_userinput(CQ_CANNED);
        cmdq_add_dir(CQ_CANNED, dx, dy, 0);
        break;
    case MCMD_OPEN_DOOR:
        cmdq_add_ec(CQ_CANNED, doopen);
        cmdq_add_dir(CQ_CANNED, dx, dy, 0);
        break;
    case MCMD_LOCK_DOOR:
        otmp = carrying(SKELETON_KEY);
        if (!otmp)
            otmp = carrying(LOCK_PICK);
        if (!otmp)
            otmp = carrying(CREDIT_CARD);
        if (otmp) {
            cmdq_add_ec(CQ_CANNED, doapply);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            cmdq_add_dir(CQ_CANNED, dx, dy, 0);
            cmdq_add_key(CQ_CANNED, 'y'); /* "Lock it?" */
        }
        break;
    case MCMD_UNTRAP_DOOR:
        cmdq_add_ec(CQ_CANNED, dountrap);
        cmdq_add_dir(CQ_CANNED, dx, dy, 0);
        break;
    case MCMD_KICK_DOOR:
        cmdq_add_ec(CQ_CANNED, dokick);
        cmdq_add_dir(CQ_CANNED, dx, dy, 0);
        break;
    case MCMD_CLOSE_DOOR:
        cmdq_add_ec(CQ_CANNED, doclose);
        cmdq_add_dir(CQ_CANNED, dx, dy, 0);
        break;
    case MCMD_SEARCH:
        cmdq_add_ec(CQ_CANNED, dosearch);
        break;
    case MCMD_LOOK_TRAP:
        cmdq_add_ec(CQ_CANNED, doidtrap);
        cmdq_add_dir(CQ_CANNED, dx, dy, 0);
        break;
    case MCMD_UNTRAP_TRAP:
        cmdq_add_ec(CQ_CANNED, dountrap);
        cmdq_add_dir(CQ_CANNED, dx, dy, 0);
        break;
    case MCMD_MOVE_DIR:
        dir = xytod(dx, dy);
        cmdq_add_ec(CQ_CANNED, move_funcs[dir][MV_WALK]);
        break;
    case MCMD_RIDE:
        cmdq_add_ec(CQ_CANNED, doride);
        cmdq_add_dir(CQ_CANNED, dx, dy, 0);
        break;
    case MCMD_REMOVE_SADDLE:
        /* m-prefix for #loot: skip any floor containers */
        cmdq_add_ec(CQ_CANNED, do_reqmenu);
        cmdq_add_ec(CQ_CANNED, doloot);
        cmdq_add_dir(CQ_CANNED, dx, dy, 0);
        cmdq_add_key(CQ_CANNED, 'y'); /* "Do you want to remove saddle? */
        break;
    case MCMD_APPLY_SADDLE:
        if ((otmp = carrying(SADDLE)) != 0) {
            cmdq_add_ec(CQ_CANNED, doapply);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            cmdq_add_dir(CQ_CANNED, dx, dy, 0);
        }
        break;
    case MCMD_ATTACK_NEXT2U:
        dir = xytod(dx, dy);
        cmdq_add_ec(CQ_CANNED, move_funcs[dir][MV_WALK]);
        break;
    case MCMD_TALK:
        cmdq_add_ec(CQ_CANNED, dotalk);
        cmdq_add_dir(CQ_CANNED, dx, dy, 0);
        break;
    case MCMD_NAME:
        cmdq_add_ec(CQ_CANNED, docallcmd);
        cmdq_add_key(CQ_CANNED, 'm'); /* name a monster */
        cmdq_add_dir(CQ_CANNED, dx, dy, 0); /* getpos() uses u.ux+dx,u.uy+dy */
        break;
    case MCMD_QUAFF:
        cmdq_add_ec(CQ_CANNED, dodrink);
        cmdq_add_key(CQ_CANNED, 'y'); /* "Drink from the fountain?" */
        break;
    case MCMD_DIP:
        cmdq_add_ec(CQ_CANNED, dodip);
        cmdq_add_userinput(CQ_CANNED);
        cmdq_add_key(CQ_CANNED, 'y'); /* "Dip foo into the fountain?" */
        break;
    case MCMD_SIT:
        cmdq_add_ec(CQ_CANNED, dosit);
        break;
    case MCMD_UP:
        cmdq_add_ec(CQ_CANNED, doup);
        break;
    case MCMD_DOWN:
        cmdq_add_ec(CQ_CANNED, dodown);
        break;
    case MCMD_DISMOUNT:
        cmdq_add_ec(CQ_CANNED, doride);
        break;
    case MCMD_MONABILITY:
        cmdq_add_ec(CQ_CANNED, domonability);
        break;
    case MCMD_PICKUP:
        cmdq_add_ec(CQ_CANNED, dopickup);
        break;
    case MCMD_LOOT:
        cmdq_add_ec(CQ_CANNED, doloot);
        break;
    case MCMD_TIP:
        cmdq_add_ec(CQ_CANNED, dotip);
        cmdq_add_key(CQ_CANNED, 'y'); /* "There is foo here; tip it?" */
        break;
    case MCMD_EAT:
        cmdq_add_ec(CQ_CANNED, doeat);
        cmdq_add_key(CQ_CANNED, 'y'); /* "There is foo here; eat it?" */
        break;
    case MCMD_DROP:
        cmdq_add_ec(CQ_CANNED, dodrop);
        break;
    case MCMD_INVENTORY:
        cmdq_add_ec(CQ_CANNED, ddoinv);
        break;
    case MCMD_REST:
        cmdq_add_ec(CQ_CANNED, donull);
        break;
    case MCMD_LOOK_HERE:
        cmdq_add_ec(CQ_CANNED, dolook);
        break;
    case MCMD_LOOK_AT:
        gc.clicklook_cc.x = u.ux + dx;
        gc.clicklook_cc.y = u.uy + dy;
        cmdq_add_ec(CQ_CANNED, doclicklook);
        break;
    case MCMD_UNTRAP_HERE:
        cmdq_add_ec(CQ_CANNED, dountrap);
        cmdq_add_dir(CQ_CANNED, 0, 0, 1);
        break;
    case MCMD_OFFER:
        cmdq_add_ec(CQ_CANNED, dosacrifice);
        cmdq_add_userinput(CQ_CANNED);
        break;
    case MCMD_CAST_SPELL:
        cmdq_add_ec(CQ_CANNED, docast);
        break;
    default:
        break;
    }
}

/* offer choice of actions to perform at adjacent location <x,y>;
   a few choices can be farther away */
staticfn char
there_cmd_menu(coordxy x, coordxy y, int mod)
{
    winid win;
    char ch = '\0';
    int npick = 0, K = 0;
    menu_item *picks = (menu_item *) 0;
    /*int dx = sgn(x - u.ux), dy = sgn(y - u.uy);*/
    coordxy dx = x - u.ux, dy = y - u.uy;
    int act = MCMD_NOTHING;

    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);

    if (u_at(x, y))
        K += there_cmd_menu_self(win, x, y, &act);
    else if (next2u(x, y))
        K += there_cmd_menu_next2u(win, x, y, mod, &act);
    else
        K += there_cmd_menu_far(win, x, y, mod);
    K += there_cmd_menu_common(win, x, y, mod, &act);

    if (!K) {
        /* no menu options, try to move */
        if (next2u(x, y) && test_move(u.ux, u.uy, dx, dy, TEST_MOVE)) {
            int dir = xytod(dx, dy);

            cmdq_add_ec(CQ_CANNED, move_funcs[dir][MV_WALK]);
        } else if (flags.travelcmd) {
            iflags.travelcc.x = u.tx = x;
            iflags.travelcc.y = u.ty = y;
            cmdq_add_ec(CQ_CANNED, dotravel_target);
        }
        npick = 0;
        ch = '\0';
    } else if (K == 1 && act != MCMD_NOTHING && act != MCMD_TRAVEL) {
        destroy_nhwindow(win);

        act_on_act(act, dx, dy);
        return '\0';
    } else {
        end_menu(win, "What do you want to do?");
        npick = select_menu(win, PICK_ONE, &picks);
        ch = '\033';
    }
    destroy_nhwindow(win);
    if (npick > 0) {
        act = picks->item.a_int;
        free((genericptr_t) picks);

        act_on_act(act, dx, dy);
        return '\0';
    }
    return ch;
}

staticfn char
here_cmd_menu(void)
{
    there_cmd_menu(u.ux, u.uy, CLICK_1);
    return '\0';
}

void
click_to_cmd(coordxy x, coordxy y, int mod)
{
    gc.clicklook_cc.x = x;
    gc.clicklook_cc.y = y;

    if (gc.Cmd.mousebtn[mod-1])
        cmdq_add_ec(CQ_CANNED, gc.Cmd.mousebtn[mod-1]->ef_funct);
}

staticfn int
domouseaction(void)
{
    coordxy x, y;
    struct obj *o;
    int dir;

    x = gc.clicklook_cc.x - u.ux;
    y = gc.clicklook_cc.y - u.uy;

    if (flags.travelcmd) {
        if (abs(x) <= 1 && abs(y) <= 1) {
            x = sgn(x), y = sgn(y);
        } else {
            iflags.travelcc.x = u.tx = u.ux + x;
            iflags.travelcc.y = u.ty = u.uy + y;
            cmdq_add_ec(CQ_CANNED, dotravel_target);
            return ECMD_OK;
        }

        if (x == 0 && y == 0) {
            /* here */
            if (IS_FOUNTAIN(levl[u.ux][u.uy].typ)
                || IS_SINK(levl[u.ux][u.uy].typ)) {
                cmdq_add_ec(CQ_CANNED, dodrink);
                return ECMD_OK;
            } else if (IS_THRONE(levl[u.ux][u.uy].typ)) {
                cmdq_add_ec(CQ_CANNED, dosit);
                return ECMD_OK;
            } else if (On_stairs_up(u.ux, u.uy)) {
                cmdq_add_ec(CQ_CANNED, doup);
                return ECMD_OK;
            } else if (On_stairs_dn(u.ux, u.uy)) {
                cmdq_add_ec(CQ_CANNED, dodown);
                return ECMD_OK;
            } else if ((o = vobj_at(u.ux, u.uy)) != 0) {
                cmdq_add_ec(CQ_CANNED, Is_container(o) ? doloot : dopickup);
                return ECMD_OK;
            } else {
                cmdq_add_ec(CQ_CANNED, donull); /* just rest */
                return ECMD_OK;
            }
        }

        /* directional commands */

        dir = xytod(x, y);
        if (!m_at(u.ux + x, u.uy + y)
            && !test_move(u.ux, u.uy, x, y, TEST_MOVE)) {
            if (IS_DOOR(levl[u.ux + x][u.uy + y].typ)) {
                /* slight assistance to player: choose kick/open for them */
                if (levl[u.ux + x][u.uy + y].doormask & D_LOCKED) {
                    cmdq_add_ec(CQ_CANNED, dokick);
                    return ECMD_OK;
                }
                if (levl[u.ux + x][u.uy + y].doormask & D_CLOSED) {
                    cmdq_add_ec(CQ_CANNED, doopen);
                    return ECMD_OK;
                }
            }
            if (levl[u.ux + x][u.uy + y].typ <= SCORR) {
                cmdq_add_ec(CQ_CANNED, dosearch);
                return ECMD_OK;
            }
            cmdq_add_ec(CQ_CANNED, move_funcs[dir][MV_WALK]);
            return ECMD_OK;
        }
    } else {
        /* convert without using floating point, allowing sloppy clicking */
        if (x > 2 * abs(y))
            x = 1, y = 0;
        else if (y > 2 * abs(x))
            x = 0, y = 1;
        else if (x < -2 * abs(y))
            x = -1, y = 0;
        else if (y < -2 * abs(x))
            x = 0, y = -1;
        else
            x = sgn(x), y = sgn(y);

        if (x == 0 && y == 0) {
            /* map click on player to "rest" command */
            cmdq_add_ec(CQ_CANNED, donull);
            return ECMD_OK;
        }
        dir = xytod(x, y);
    }

    /* move, attack, etc. */
    cmdq_add_ec(CQ_CANNED, move_funcs[dir][MV_WALK]);
    return ECMD_OK;
}

/* gather typed digits into a number in *count; return the next non-digit */
char
get_count(
    const char *allowchars, /* what comes after digits; if Null, anything */
    char inkey,          /* if caller already got first digit, this is it */
    long maxcount,       /* if user tries to enter a bigger count, use this */
    cmdcount_nht *count, /* primary output */
    unsigned gc_flags)   /* control flags: GC_SAVEHIST, GC_ECHOFIRST */
{
    char qbuf[QBUFSZ];
    int key, save_input_state = gp.program_state.input_state;
    long cnt = 0L, first = inkey ? (long) (inkey - '0') : 0L;
    boolean backspaced = FALSE, showzero = TRUE,
            /* should "Count: 123" go into message history? */
            historicmsg = (gc_flags & GC_SAVEHIST) != 0,
            /* put "Count: N" into mesg hist unless N is the same as the
               [first digit] value passed in via 'inkey' */
            conditionalmsg = (gc_flags & GC_CONDHIST) != 0,
            /* normally "Count: 12" isn't echoed until the second digit */
            echoalways = (gc_flags & GC_ECHOFIRST) != 0;
    /* this should be done in port code so that we have erase_char
       and kill_char available; we can at least fake erase_char */
#define STANDBY_erase_char '\177'

    *count = 0;
    for (;;) {
        if (inkey) {
            key = inkey;
            inkey = '\0';
        } else {
            /* if readchar() has already been called in this loop, it will
               have reset input_state; put that back to its previous value */
            gp.program_state.input_state = save_input_state;
            key = readchar();
        }

        if (digit(key)) {
            cnt = 10L * cnt + (long) (key - '0');
            if (cnt < 0L)
                cnt = 0L;
            else if (maxcount > 0L && cnt > maxcount)
                cnt = maxcount;
            /* if we've backed up to nothing, then typed 0, show that 0 */
            showzero = (key == '0');
        } else if (key == '\b' || key == STANDBY_erase_char) {
            if (!cnt && !echoalways)
                break;
            showzero = FALSE;
            cnt = cnt / 10L;
            backspaced = TRUE;
        } else if (key == gc.Cmd.spkeys[NHKF_ESC]) {
            break;
        } else if (!allowchars || strchr(allowchars, key)) {
            *count = (cmdcount_nht) cnt;
            if ((long) *count != cnt)
                impossible("get_count: cmdcount_nht");
            break;
        }

        if (cnt > 9 || backspaced || echoalways) {
            clear_nhwindow(WIN_MESSAGE);
            if (backspaced && !cnt && !showzero) {
                Sprintf(qbuf, "Count: ");
            } else {
                Sprintf(qbuf, "Count: %ld", cnt);
                backspaced = FALSE;
            }
            custompline(SUPPRESS_HISTORY, "%s", qbuf);
            mark_synch();
        }
    }

    if (historicmsg || (conditionalmsg && *count != first)) {
        Sprintf(qbuf, "Count: %ld ", *count);
        (void) key2txt((uchar) key, eos(qbuf));
        putmsghistory(qbuf, FALSE);
    }

    return key;
}


staticfn int
parse(void)
{
    int foo;

    iflags.in_parse = TRUE;
    gc.command_count = 0;
    gc.context.move = TRUE; /* assume next command will take game time */
    flush_screen(1); /* Flush screen buffer. Put the cursor on the hero. */

    /* affects readchar() behavior for ESC iff 'altmeta' option is On;
       reset to 0 by readchar() */
    gp.program_state.input_state = commandInp;
    if (!gc.Cmd.num_pad || (foo = readchar()) == gc.Cmd.spkeys[NHKF_COUNT]) {
        foo = get_count((char *) 0, '\0', LARGEST_INT,
                        &gc.command_count, GC_NOFLAGS);
        gl.last_command_count = gc.command_count;
    }

    if (foo == gc.Cmd.spkeys[NHKF_ESC]) { /* esc cancels count (TH) */
        clear_nhwindow(WIN_MESSAGE);
        gc.command_count = 0;
        gl.last_command_count = 0;
    } else if (gi.in_doagain) {
        gc.command_count = gl.last_command_count;
    } else if (foo && gc.Cmd.commands[foo & 0xff]
               /* these shouldn't go into the do-again buffer */
               && (gc.Cmd.commands[foo & 0xff]->ef_funct == do_repeat
                   || gc.Cmd.commands[foo & 0xff]->ef_funct == doprev_message
                   /* this one might get put into the do-again buffer but
                      only if the interface code tells the core to do it */
                   || gc.Cmd.commands[foo & 0xff]->ef_funct == doextcmd)) {
        /* gc.command_count will be set again when we
           re-enter with gi.in_doagain set true */
        gc.command_count = gl.last_command_count;
    }

    gm.multi = gc.command_count;
    if (gm.multi)
        gm.multi--;

    gc.cmd_key = foo;
    clear_nhwindow(WIN_MESSAGE);

    iflags.in_parse = FALSE;
    return gc.cmd_key;
}

#ifdef HANGUPHANDLING
/* some very old systems, or descendents of such systems, expect signal
   handlers to have return type `int', but they don't actually inspect
   the return value so we should be safe using `void' unconditionally */
/*ARGUSED*/
void
hangup(
    int sig_unused UNUSED)   /* called as signal() handler, so sent
                              * at least one arg */
{
    if (gp.program_state.exiting)
        gp.program_state.in_moveloop = 0;
    nhwindows_hangup();
#ifdef SAFERHANGUP
    /* When using SAFERHANGUP, the done_hup flag is tested in rhack
       and a couple of other places; actual hangup handling occurs then.
       This is 'safer' because it disallows certain cheats and also
       protects against losing objects in the process of being thrown,
       but also potentially riskier because the disconnected program
       must continue running longer before attempting a hangup save. */
    gp.program_state.done_hup++;
    /* defer hangup iff game appears to be in progress */
    if (gp.program_state.in_moveloop
        && gp.program_state.something_worth_saving)
        return;
#endif /* SAFERHANGUP */
    end_of_input();
}

void
end_of_input(void)
{
#ifdef NOSAVEONHANGUP
#ifdef INSURANCE
    if (flags.ins_chkpt && gp.program_state.something_worth_saving)
        program_state.preserve_locks = 1; /* keep files for recovery */
#endif
    gp.program_state.something_worth_saving = 0; /* don't save */
#endif

#ifndef SAFERHANGUP
    if (!gp.program_state.done_hup++)
#endif
        if (gp.program_state.something_worth_saving)
            (void) dosave0();
    if (soundprocs.sound_exit_nhsound)
        (*soundprocs.sound_exit_nhsound)("end_of_input");
    if (iflags.window_inited)
        exit_nhwindows((char *) 0);
    clearlocks();
    nh_terminate(EXIT_SUCCESS);
    /*NOTREACHED*/ /* not necessarily true for vms... */
    return;
}
#endif /* HANGUPHANDLING */

staticfn char
readchar_core(coordxy *x, coordxy *y, int *mod)
{
    int sym;

    if (iflags.debug_fuzzer) {
        sym = randomkey();
        goto readchar_done;
    }
    if (*readchar_queue)
        sym = *readchar_queue++;
    else if (gi.in_doagain)
        sym = pgetchar();
    else
        sym = nh_poskey(x, y, mod);

#ifdef NR_OF_EOFS
    if (sym == EOF) {
        int cnt = NR_OF_EOFS;
        /*
         * Some SYSV systems seem to return EOFs for various reasons
         * (?like when one hits break or for interrupted systemcalls?),
         * and we must see several before we quit.
         */
        do {
            clearerr(stdin); /* omit if clearerr is undefined */
            sym = pgetchar();
        } while (--cnt && sym == EOF);
    }
#endif /* NR_OF_EOFS */

    if (sym == EOF) {
#ifdef HANGUPHANDLING
        hangup(0); /* call end_of_input() or set program_state.done_hup */
#endif
        sym = '\033';
#ifdef ALTMETA
    } else if (sym == '\033' && iflags.altmeta
               && gp.program_state.input_state != otherInp) {
        /* iflags.altmeta: treat two character ``ESC c'' as single `M-c' but
           only when we're called by parse() [possibly via get_count()]
           or getpos() [to support Alt+digit] or getdir() [for arrow keys
           under curses] */
        sym = *readchar_queue ? *readchar_queue++ : pgetchar();
        if (sym == EOF || sym == 0)
            sym = '\033';
        else if (sym != '\033')
            sym |= 0200; /* force 8th bit on */
#endif /*ALTMETA*/
    } else if (sym == 0) {
        /* click event */
        gc.clicklook_cc.x = gc.clicklook_cc.y = -1;
        click_to_cmd(*x, *y, *mod);
    }

 readchar_done:
    /* next readchar() will be for an ordinary char unless parse()
       sets this back to non-zero */
    gp.program_state.input_state = otherInp;
    return (char) sym;
}

/* get a character */
char
readchar(void)
{
    char ch;
    coordxy x = u.ux, y = u.uy;
    int mod = 0;

    ch = readchar_core(&x, &y, &mod);
    return ch;
}

/* used by getpos() to accept mouse input as well as keyboard input */
char
readchar_poskey(coordxy *x, coordxy *y, int *mod)
{
    char ch;

    gp.program_state.input_state = getposInp;
    ch = readchar_core(x, y, mod);
    return ch;
}

/* '_' command, #travel, via keyboard rather than mouse click */
staticfn int
dotravel(void)
{
    coord cc;

    /*
     * Traveling used to be a no-op if user toggled 'travel' option
     * Off.  However, travel was initially implemented as a mouse-only
     * command and the original purpose of the option was to be able
     * to prevent clicks on the map from initiating travel.
     *
     * Travel via '_' came later.  Since it requires a destination--
     * which offers the user a chance to cancel if it was accidental--
     * there's no reason for the option to disable travel-by-keys.
     */

    cc.x = iflags.travelcc.x;
    cc.y = iflags.travelcc.y;
    if (cc.x == 0 && cc.y == 0) {
        /* No cached destination, start attempt from current position */
        cc.x = u.ux;
        cc.y = u.uy;
    }
    iflags.getloc_travelmode = TRUE;
    if (iflags.menu_requested) {
        int gfilt = iflags.getloc_filter;

        iflags.getloc_filter = GFILTER_VIEW;
        if (!getpos_menu(&cc, GLOC_INTERESTING)) {
            iflags.getloc_filter = gfilt;
            iflags.getloc_travelmode = FALSE;
            return ECMD_OK;
        }
        iflags.getloc_filter = gfilt;
    } else {
        pline("Where do you want to travel to?");
        if (getpos(&cc, TRUE, "the desired destination") < 0) {
            /* user pressed ESC */
            iflags.getloc_travelmode = FALSE;
            return ECMD_CANCEL;
        }
    }
    iflags.travelcc.x = u.tx = cc.x;
    iflags.travelcc.y = u.ty = cc.y;

    return dotravel_target();
}

/* #retravel, travel to iflags.travelcc, which must be set */
staticfn int
dotravel_target(void)
{
    if (!isok(iflags.travelcc.x, iflags.travelcc.y)) {
        /* assume <0,0>, the value assigned when travel reaches destination */
        pline("No travel destination set.");
        return ECMD_OK;
    } else if (u_at(iflags.travelcc.x, iflags.travelcc.y)) {
        /* maybe interrupted while traveling then just walked rest of way
           so destination hasn't been reset yet */
        You("are already here.");
        iflags.travelcc.x = iflags.travelcc.y = 0;
        return ECMD_OK;
    }

    iflags.getloc_travelmode = FALSE;

    gc.context.travel = 1;
    gc.context.travel1 = 1;
    gc.context.run = 8;
    gc.context.nopick = 1;
    gd.domove_attempting |= DOMOVE_RUSH;

    if (!gm.multi)
        gm.multi = max(COLNO, ROWNO);
    u.last_str_turn = 0;
    gc.context.mv = TRUE;

    domove();
    return ECMD_TIME;
}

/* mouse click look command */
staticfn int
doclicklook(void)
{
    if (!isok(gc.clicklook_cc.x, gc.clicklook_cc.y))
        return ECMD_OK;

    gc.context.move = FALSE;
    auto_describe(gc.clicklook_cc.x, gc.clicklook_cc.y);

    return ECMD_OK;
}

/* can we use menu entries to respond to a query? */
staticfn boolean
yn_menuable_resp(const char *resp)
{
    return iflags.query_menu && iflags.window_inited
        && (resp == ynchars || resp == ynqchars || resp == ynaqchars
            || resp == rightleftchars);
}

staticfn void
yn_func_menu_opt(winid win, char key, const char *text, char def)
{
    anything any;

    any = cg.zeroany;
    any.a_char = key;
    add_menu(win, &nul_glyphinfo, &any, key, 0,
             ATR_NONE, NO_COLOR, text,
             (def == key) ? MENU_ITEMFLAGS_SELECTED
                          : MENU_ITEMFLAGS_NONE);

}

/* use a menu to ask a specific response to a query.
   returns TRUE if the menu was shown to the user.
   puts the response char into res. */
staticfn boolean
yn_function_menu(
    const char *query,
    const char *resp,
    char def,
    char *res)
{
    if (yn_menuable_resp(resp)) {
        winid win = create_nhwindow(NHW_MENU);
        menu_item *sel;
        int n;
        char keybuf[QBUFSZ];

        start_menu(win, MENU_BEHAVE_STANDARD);
        if (resp == rightleftchars) {
            yn_func_menu_opt(win, 'r', "Right", def);
            yn_func_menu_opt(win, 'l', "Left", def);
        } else {
            yn_func_menu_opt(win, 'y', "Yes", def);
            yn_func_menu_opt(win, 'n', "No", def);
        }
        if (resp == ynaqchars)
            yn_func_menu_opt(win, 'a', "All", def);
        if (resp == ynqchars || resp == ynaqchars)
            yn_func_menu_opt(win, 'q', "Quit", def);
        end_menu(win, query);
        n = select_menu(win, PICK_ONE, &sel);
        destroy_nhwindow(win);
        if (n > 0) {
            *res = sel[0].item.a_char;
            /* two were selected? use the one that wasn't the default */
            if (n > 1 && *res == def)
                *res = sel[1].item.a_char;
            free((genericptr_t) sel);
        } else {
            *res = def;
        }
        pline("%s %s", query, key2txt(*res, keybuf));
        clear_nhwindow(WIN_MESSAGE);
        return TRUE;
    }
    return FALSE;
}

/*
 *   Parameter validator for generic yes/no function to prevent
 *   the core from sending too long a prompt string to the
 *   window port causing a buffer overflow there.
 */
char
yn_function(
    const char *query,
    const char *resp,
    char def,
    boolean addcmdq)
{
    char res = '\033', qbuf[QBUFSZ];
    struct _cmd_queue cq, *cmdq;
#ifdef DUMPLOG_CORE
    unsigned idx = gs.saved_pline_index;
    /* buffer to hold query+space+formatted_single_char_response */
    char dumplog_buf[QBUFSZ + 1 + 15]; /* [QBUFSZ+1+7] should suffice */
#endif

    iflags.last_msg = PLNMSG_UNKNOWN; /* most recent pline is clobbered */

    /* maximum acceptable length is QBUFSZ-1 */
    if (strlen(query) >= QBUFSZ) {
        /* caller shouldn't have passed anything this long */
        paniclog("Query truncated: ", query);
        (void) strncpy(qbuf, query, QBUFSZ - 1 - 3);
        Strcpy(&qbuf[QBUFSZ - 1 - 3], "...");
        query = qbuf;
    }

    if ((cmdq = cmdq_pop()) != 0) {
        cq = *cmdq;
        free(cmdq);
    } else {
        cq.typ = CMDQ_USER_INPUT;
        cq.key = '\0'; /* lint suppression */
    }

    if (cq.typ != CMDQ_USER_INPUT) {
        if (cq.typ == CMDQ_KEY)
            res = cq.key;
        else
            cmdq_clear(CQ_CANNED); /* 'res' is ESC */
    } else {
#ifdef SND_SPEECH
        if ((gp.pline_flags & PLINE_SPEECH) != 0) {
            sound_speak(query);
            gp.pline_flags &= ~PLINE_SPEECH;
        }
#endif
        if (!yn_function_menu(query, resp, def, &res)) {
            res = (*windowprocs.win_yn_function)(query, resp, def);
        }
        if (addcmdq)
            cmdq_add_key(CQ_REPEAT, res);
    }

#ifdef DUMPLOG_CORE
    if (idx == gs.saved_pline_index) {
        /* when idx is still the same as gs.saved_pline_index, the interface
           didn't put the prompt into gs.saved_plines[]; we put a simplified
           version in there now (without response choices or default) */
        Sprintf(dumplog_buf, "%s ", query);
        (void) key2txt((uchar) res, eos(dumplog_buf));
        dumplogmsg(dumplog_buf);
    }
#endif
    /* in case we're called via getdir() which sets input_state */
    gp.program_state.input_state = otherInp;
    return res;
}

/* for paranoid_confirm:quit,die,attack,&c prompting; allows yes, n|no,
   or q|quit; result is one of 'y' or 'n' or 'q'; ESC yields 'q' */
char
paranoid_ynq(
    boolean be_paranoid,
    const char *prompt,
    boolean accept_q)
{
    char c = 'n'; /* default result */

    /* when paranoid, player must respond with "yes" rather than just 'y'
       to give the go-ahead for this query; default is "no" unless the
       ParanoidConfirm flag is set in which case there's no default */
    if (be_paranoid) {
        char pbuf[BUFSZ], qbuf[QBUFSZ], ans[BUFSZ];
        const char *promptprefix = "", /* empty for first iteration */
            *responsetype = ParanoidConfirm ? (accept_q ? "[yes|no|quit]"
                                               : "[yes|no]")
                                            /* default of 'n' is shown for
                                             * the !ParanoidConfirm cases */
                                            : (accept_q ? "[yes|n|q] (n)"
                                               : "[yes|n] (n)");
        int k, trylimit = 6; /* 1 normal, 5 more with "Yes or No:" prefix */

        copynchars(pbuf, prompt, BUFSZ - 1);
        /* in addition to being paranoid about this particular
           query, we might be even more paranoid about all paranoia
           responses (ie, ParanoidConfirm is set) in which case we
           require "no" to reject in addition to "yes" to confirm
           (except we won't loop if response is ESC; it means no) */
        do {
            /* make sure we won't overflow a QBUFSZ sized buffer */
            k = (int) (strlen(promptprefix) + 1 + strlen(responsetype));
            if ((int) strlen(pbuf) + k > QBUFSZ - 1) {
                /* chop off some at the end */
                Strcpy(pbuf + (QBUFSZ - 1) - k - 4, "...?"); /* -4: "...?" */
            }

            Snprintf(qbuf, sizeof qbuf, "%s%s %s", promptprefix, pbuf,
                     responsetype);
            *ans = '\0';
            getlin(qbuf, ans);
            (void) mungspaces(ans);
            if (!strcmpi(ans, "yes")) {
                c = 'y';
                break;
            }
            if (!strcmpi(ans, "quit") || *ans == '\033') {
                c = 'q';
                break;
            }
            /* we don't bother adding "or \"Quit\"" for the accept_q case */
            promptprefix = "\"Yes\" or \"No\": ";
            /* for empty input, return value c will already be 'n' */
        } while (ParanoidConfirm && strcmpi(ans, "no") && --trylimit);
    } else if (accept_q) {
        c = ynq(prompt); /* 'y', 'n', or 'q' */
    } else {
        c = y_n(prompt); /* 'y' or 'n' */
    }
    if (c != 'y' && (c != 'q' || !accept_q))
        c = 'n';
    return c;
}

/* for paranoid_confirm:quit,die,attack,&c prompting; allows yes or n|no;
   result is True for yes; n|no and ESC yield False */
boolean
paranoid_query(boolean be_paranoid, const char *prompt)
{
    return (paranoid_ynq(be_paranoid, prompt, FALSE) == 'y');
}

/* ^Z command, #suspend */
staticfn int
dosuspend_core(void)
{
#ifdef SUSPEND
    /* Does current window system support suspend? */
    if ((*windowprocs.win_can_suspend)()) {
        time_t now = getnow();

        urealtime.realtime += timet_delta(now, urealtime.start_timing);
        urealtime.start_timing = now; /* as a safeguard against panic save */
        /* NB: SYSCF SHELLERS handled in port code. */
        dosuspend();
        urealtime.start_timing = getnow(); /* resume keeping track of time */
    } else
#endif
        Norep(cmdnotavail, "#suspend");
    return ECMD_OK;
}

/* '!' command, #shell */
staticfn int
dosh_core(void)
{
#ifdef SHELL
    time_t now = getnow();

    urealtime.realtime += timet_delta(now, urealtime.start_timing);
    urealtime.start_timing = now; /* (see dosuspend_core) */
    /* access restrictions, if any, are handled in port code */
    dosh();
    urealtime.start_timing = getnow();
#else
    Norep(cmdnotavail, "#shell");
#endif
    return ECMD_OK;
}

staticfn int
dummyfunction(void)
{
    return ECMD_CANCEL;
}

/*cmd.c*/

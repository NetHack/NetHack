/* NetHack 3.7	cmd.c	$NHDT-Date: 1649272000 2022/04/06 19:06:40 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.539 $ */
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
extern int vanquished(void);         /**/
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

static const char *ecname_from_fn(int (*)(void));
static int dosuspend_core(void);
static int dosh_core(void);
static int doherecmdmenu(void);
static int dotherecmdmenu(void);
static int doprev_message(void);
static int timed_occupation(void);
static boolean can_do_extcmd(const struct ext_func_tab *);
static int dotravel(void);
static int dotravel_target(void);
static int doclicklook(void);
static int doterrain(void);
static int wiz_wish(void);
static int wiz_identify(void);
static int wiz_map(void);
static int wiz_makemap(void);
static int wiz_genesis(void);
static int wiz_where(void);
static int wiz_detect(void);
static int wiz_panic(void);
static int wiz_polyself(void);
static int wiz_load_lua(void);
static int wiz_level_tele(void);
static int wiz_level_change(void);
static int wiz_flip_level(void);
static int wiz_show_seenv(void);
static int wiz_show_vision(void);
static int wiz_smell(void);
static int wiz_intrinsic(void);
static int wiz_show_wmodes(void);
static int wiz_show_stats(void);
static int wiz_rumor_check(void);
#ifdef DEBUG_MIGRATING_MONS
static int wiz_migrate_mons(void);
#endif

static void wiz_map_levltyp(void);
static void wiz_levltyp_legend(void);
#if defined(__BORLANDC__) && !defined(_WIN32)
extern void show_borlandc_stats(winid);
#endif
static int size_monst(struct monst *, boolean);
static int size_obj(struct obj *);
static void count_obj(struct obj *, long *, long *, boolean, boolean);
static void obj_chain(winid, const char *, struct obj *, boolean, long *,
                      long *);
static void mon_invent_chain(winid, const char *, struct monst *, long *,
                             long *);
static void mon_chain(winid, const char *, struct monst *, boolean, long *,
                      long *);
static void contained_stats(winid, const char *, long *, long *);
static void misc_stats(winid, long *, long *);
static boolean accept_menu_prefix(const struct ext_func_tab *);
static void reset_cmd_vars(boolean);
static void mcmd_addmenu(winid, int, const char *);
static char here_cmd_menu(void);
static char there_cmd_menu(int, int, int);
static char readchar_core(int *, int *, int *);
static char *parse(void);
static void show_direction_keys(winid, char, boolean);
static boolean help_dir(char, int, const char *);

static boolean bind_key_fn(uchar, int (*)(void));
static void commands_init(void);
static boolean keylist_func_has_key(const struct ext_func_tab *, boolean *);
static int keylist_putcmds(winid, boolean, int, int, boolean *);
static const char *spkey_name(int);

static int (*timed_occ_fn)(void);
static char *doc_extcmd_flagstr(winid, const struct ext_func_tab *);

static const char *readchar_queue = "";
/* for rejecting attempts to use wizard mode commands */
static const char unavailcmd[] = "Unavailable command '%s'.";
/* for rejecting #if !SHELL, !SUSPEND */
static const char cmdnotavail[] = "'%s' command not available.";

/* the #prevmsg command */
static int
doprev_message(void)
{
    (void) nh_doprev_message();
    return ECMD_OK;
}

/* Count down by decrementing multi */
static int
timed_occupation(void)
{
    (*timed_occ_fn)();
    if (g.multi > 0)
        g.multi--;
    return g.multi > 0;
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
        g.occupation = timed_occupation;
        timed_occ_fn = fn;
    } else
        g.occupation = fn;
    g.occtxt = txt;
    g.occtime = 0;
    return;
}

/* add extended command function to the command queue */
void
cmdq_add_ec(int (*fn)(void))
{
    struct _cmd_queue *tmp = (struct _cmd_queue *) alloc(sizeof *tmp);
    struct _cmd_queue *cq = g.command_queue;

    tmp->typ = CMDQ_EXTCMD;
    tmp->ec_entry = ext_func_tab_from_func(fn);
    tmp->next = NULL;

    while (cq && cq->next)
        cq = cq->next;

    if (cq)
        cq->next = tmp;
    else
        g.command_queue = tmp;
}

/* add a key to the command queue */
void
cmdq_add_key(char key)
{
    struct _cmd_queue *tmp = (struct _cmd_queue *) alloc(sizeof *tmp);
    struct _cmd_queue *cq = g.command_queue;

    tmp->typ = CMDQ_KEY;
    tmp->key = key;
    tmp->next = NULL;

    while (cq && cq->next)
        cq = cq->next;

    if (cq)
        cq->next = tmp;
    else
        g.command_queue = tmp;
}

/* add a direction to the command queue */
void
cmdq_add_dir(schar dx, schar dy, schar dz)
{
    struct _cmd_queue *tmp = (struct _cmd_queue *) alloc(sizeof *tmp);
    struct _cmd_queue *cq = g.command_queue;

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
        g.command_queue = tmp;
}

/* add placeholder to the command queue, allows user input there */
void
cmdq_add_userinput(void)
{
    struct _cmd_queue *tmp = (struct _cmd_queue *) alloc(sizeof *tmp);
    struct _cmd_queue *cq = g.command_queue;

    tmp->typ = CMDQ_USER_INPUT;
    tmp->next = NULL;

    while (cq && cq->next)
        cq = cq->next;

    if (cq)
        cq->next = tmp;
    else
        g.command_queue = tmp;
}

/* pop off the topmost command from the command queue.
 * caller is responsible for freeing the returned _cmd_queue.
 */
struct _cmd_queue *
cmdq_pop(void)
{
    struct _cmd_queue *tmp = g.command_queue;

    if (tmp) {
        g.command_queue = tmp->next;
        tmp->next = NULL;
    }
    return tmp;
}

/* get the top entry without popping it */
struct _cmd_queue *
cmdq_peek(void)
{
    return g.command_queue;
}

/* clear all commands from the command queue */
void
cmdq_clear(void)
{
    struct _cmd_queue *tmp = g.command_queue;
    struct _cmd_queue *tmp2;

    while (tmp) {
        tmp2 = tmp->next;
        free(tmp);
        tmp = tmp2;
    }
    g.command_queue = NULL;
}

static char popch(void);

static char
popch(void)
{
    /* If occupied, return '\0', letting tgetch know a character should
     * be read from the keyboard.  If the character read is not the
     * ABORT character (as checked in pcmain.c), that character will be
     * pushed back on the pushq.
     */
    if (g.occupation)
        return '\0';
    if (g.in_doagain)
        return (char) ((g.shead != g.stail) ? g.saveq[g.stail++] : '\0');
    else
        return (char) ((g.phead != g.ptail) ? g.pushq[g.ptail++] : '\0');
}

char
pgetchar(void) /* courtesy of aeb@cwi.nl */
{
    register int ch;

    if (iflags.debug_fuzzer)
        return randomkey();
    if (!(ch = popch()))
        ch = nhgetch();
    return (char) ch;
}

/* A ch == 0 resets the pushq */
void
pushch(char ch)
{
    if (!ch)
        g.phead = g.ptail = 0;
    if (g.phead < BSIZE)
        g.pushq[g.phead++] = ch;
    return;
}

/* A ch == 0 resets the saveq.  Only save keystrokes when not
 * replaying a previous command.
 */
void
savech(char ch)
{
    if (!g.in_doagain) {
        if (!ch)
            g.phead = g.ptail = g.shead = g.stail = 0;
        else if (g.shead < BSIZE)
            g.saveq[g.shead++] = ch;
    }
    return;
}

static boolean
can_do_extcmd(const struct ext_func_tab *extcmd)
{
    int ecflags = extcmd->flags;

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
        g.ext_tlist = &extcmdlist[idx];

        retval = (*func)();
    } while (func == doextlist);

    return retval;
}

/* format extended command flags for display */
static char *
doc_extcmd_flagstr(winid menuwin,
                   /* if Null, add a footnote to the menu */
                   const struct ext_func_tab *efp)
{
    static char Abuf[10]; /* 5 would suffice: {'[','m','A',']','\0'} */

    /* note: tag shown for menu prefix is 'm' even if m-prefix action
       has been bound to some other key */
    if (!efp) {
        char qbuf[QBUFSZ];
        anything any = cg.zeroany;

        add_menu(menuwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 "[A] Command autocompletes", MENU_ITEMFLAGS_NONE);
        Sprintf(qbuf, "[m] Command accepts '%s' prefix",
                visctrl(cmd_from_func(do_reqmenu)));
        add_menu(menuwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, qbuf,
                 MENU_ITEMFLAGS_NONE);
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
    register const struct ext_func_tab *efp = (struct ext_func_tab *) 0;
    char buf[BUFSZ], searchbuf[BUFSZ], promptbuf[QBUFSZ];
    winid menuwin;
    anything any;
    menu_item *selected;
    int n, pass;
    int menumode = 0, menushown[2], onelist = 0;
    boolean redisplay = TRUE, search = FALSE;
    static const char *const headings[] = { "Extended commands",
                                      "Debugging Extended Commands" };

    searchbuf[0] = '\0';
    menuwin = create_nhwindow(NHW_MENU);

    while (redisplay) {
        redisplay = FALSE;
        any = cg.zeroany;
        start_menu(menuwin, MENU_BEHAVE_STANDARD);
        add_menu(menuwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 "Extended Commands List",
                 MENU_ITEMFLAGS_NONE);
        add_menu(menuwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 "", MENU_ITEMFLAGS_NONE);

        Sprintf(buf, "Switch to %s commands that don't autocomplete",
                menumode ? "including" : "excluding");
        any.a_int = 1;
        add_menu(menuwin, &nul_glyphinfo, &any, 'a', 0, ATR_NONE, buf,
                 MENU_ITEMFLAGS_NONE);

        if (!*searchbuf) {
            any.a_int = 2;
            /* was 's', but then using ':' handling within the interface
               would only examine the two or three meta entries, not the
               actual list of extended commands shown via separator lines;
               having ':' as an explicit selector overrides the default
               menu behavior for it; we retain 's' as a group accelerator */
            add_menu(menuwin, &nul_glyphinfo, &any, ':', 's', ATR_NONE,
                     "Search extended commands",
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
                     buf, MENU_ITEMFLAGS_NONE);
        }
        if (wizard) {
            any.a_int = 4;
            add_menu(menuwin, &nul_glyphinfo, &any, 'z', 0, ATR_NONE,
          onelist ? "Switch to showing debugging commands in separate section"
       : "Switch to showing all alphabetically, including debugging commands",
                     MENU_ITEMFLAGS_NONE);
        }
        any = cg.zeroany;
        add_menu(menuwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 "", MENU_ITEMFLAGS_NONE);
        menushown[0] = menushown[1] = 0;
        n = 0;
        for (pass = 0; pass <= 1; ++pass) {
            /* skip second pass if not in wizard mode or wizard mode
               commands are being integrated into a single list */
            if (pass == 1 && (onelist || !wizard))
                break;
            for (efp = extcmdlist; efp->ef_txt; efp++) {
                int wizc;

                if ((efp->flags & (CMD_NOT_AVAILABLE|INTERNALCMD)) != 0)
                    continue;
                /* if hiding non-autocomplete commands, skip such */
                if (menumode == 1 && (efp->flags & AUTOCOMPLETE) == 0)
                    continue;
                /* if searching, skip this command if it doesn't match */
                if (*searchbuf
                    /* first try case-insensitive substring match */
                    && !strstri(efp->ef_txt, searchbuf)
                    && !strstri(efp->ef_desc, searchbuf)
                    /* wildcard support; most interfaces use case-insensitve
                       pmatch rather than regexp for menu searching */
                    && !pmatchi(searchbuf, efp->ef_txt)
                    && !pmatchi(searchbuf, efp->ef_desc))
                    continue;
                /* skip wizard mode commands if not in wizard mode;
                   when showing two sections, skip wizard mode commands
                   in pass==0 and skip other commands in pass==1 */
                wizc = (efp->flags & WIZMODECMD) != 0;
                if (wizc && !wizard)
                    continue;
                if (!onelist && pass != wizc)
                    continue;

                /* We're about to show an item, have we shown the menu yet?
                   Doing menu in inner loop like this on demand avoids a
                   heading with no subordinate entries on the search
                   results menu. */
                if (!menushown[pass]) {
                    Strcpy(buf, headings[pass]);
                    add_menu(menuwin, &nul_glyphinfo, &any, 0, 0,
                             iflags.menu_headings, buf,
                             MENU_ITEMFLAGS_NONE);
                    menushown[pass] = 1;
                }
                /* longest ef_txt at present is "wizrumorcheck" (13 chars);
                   2nd field will be "    " or " [A]" or " [m]" or "[mA]" */
                Sprintf(buf, " %-14s %4s %s", efp->ef_txt,
                        doc_extcmd_flagstr(menuwin, efp), efp->ef_desc);
                add_menu(menuwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                         buf, MENU_ITEMFLAGS_NONE);
                ++n;
            }
            if (n)
                add_menu(menuwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                         "", MENU_ITEMFLAGS_NONE);
        }
        if (*searchbuf && !n)
            add_menu(menuwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                     "no matches", MENU_ITEMFLAGS_NONE);
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
                    iflags.extmenu = 0;
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
                             0, ATR_NONE, buf, MENU_ITEMFLAGS_NONE);
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
                     ATR_NONE, buf, MENU_ITEMFLAGS_NONE);
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
    struct permonst *uptr = g.youmonst.data;
    boolean might_hide = (is_hider(uptr) || hides_under(uptr));
    char c = '\0';

    if (might_hide && webmaker(uptr)) {
        c = yn_function("Hide [h] or spin a web [s]?", "hsq", 'q');
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
            if (split_mon(&g.youmonst, (struct monst *) 0))
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
    } else if (is_vampire(uptr) || is_vampshifter(&g.youmonst)) {
        return dopoly();
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

#ifdef SYSCF
#if defined(UNIX)
        if (!sysopt.explorers || !sysopt.explorers[0]
            || !check_user_string(sysopt.explorers)) {
            if (!wizard) {
                You("cannot access explore mode.");
                return ECMD_OK;
            } else {
                pline(
                 "Note: normally you wouldn't be allowed into explore mode.");
                /* keep going */
            }
        }
#endif
#endif
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

/* #wizwish command - wish for something */
static int
wiz_wish(void) /* Unlimited wishes for debug mode by Paul Polderman */
{
    if (wizard) {
        boolean save_verbose = flags.verbose;

        flags.verbose = FALSE;
        makewish();
        flags.verbose = save_verbose;
        (void) encumber_msg();
    } else
        pline(unavailcmd, ecname_from_fn(wiz_wish));
    return ECMD_OK;
}

/* #wizidentify command - reveal and optionally identify hero's inventory */
static int
wiz_identify(void)
{
    if (wizard) {
        iflags.override_ID = (int) cmd_from_func(wiz_identify);
        /* command remapping might leave #wizidentify as the only way
           to invoke us, in which case cmd_from_func() will yield NUL;
           it won't matter to display_inventory()/display_pickinv()
           if ^I invokes some other command--what matters is that
           display_pickinv() and xname() see override_ID as nonzero */
        if (!iflags.override_ID)
            iflags.override_ID = C('I');
        (void) display_inventory((char *) 0, FALSE);
        iflags.override_ID = 0;
    } else
        pline(unavailcmd, ecname_from_fn(wiz_identify));
    return ECMD_OK;
}

void
makemap_prepost(boolean pre, boolean wiztower)
{
    NHFILE tmpnhfp;
    struct monst *mtmp;

    if (pre) {
        /* keep steed and other adjacent pets after releasing them
           from traps, stopping eating, &c as if hero were ascending */
        keepdogs(TRUE); /* (pets-only; normally we'd be using 'FALSE' here) */

        rm_mapseen(ledger_no(&u.uz));
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            int ndx = monsndx(mtmp->data);
            if (mtmp->isgd) { /* vault is going away; get rid of guard */
                mtmp->isgd = 0;
                mongone(mtmp);
            }
            if (mtmp->data->geno & G_UNIQ)
                g.mvitals[ndx].mvflags &= ~(G_EXTINCT);
            if (g.mvitals[ndx].born)
                g.mvitals[ndx].born--;
            if (DEADMONSTER(mtmp))
                continue;
            if (mtmp->isshk)
                setpaid(mtmp);
        }
        {
            static const char Unachieve[] = "%s achievement revoked.";

            /* achievement tracking; if replacing a level that has a
               special prize, lose credit for previously finding it and
               reset for the new instance of that prize */
            if (Is_mineend_level(&u.uz)) {
                if (remove_achievement(ACH_MINE_PRIZE))
                    pline(Unachieve, "Mine's-end");
                g.context.achieveo.mines_prize_oid = 0;
            } else if (Is_sokoend_level(&u.uz)) {
                if (remove_achievement(ACH_SOKO_PRIZE))
                    pline(Unachieve, "Soko-prize");
                g.context.achieveo.soko_prize_oid = 0;
            }
        }
        if (Punished) {
            ballrelease(FALSE);
            unplacebc();
        }
        /* reset lock picking unless it's for a carried container */
        maybe_reset_pick((struct obj *) 0);
        /* reset interrupted digging if it was taking place on this level */
        if (on_level(&g.context.digging.level, &u.uz))
            (void) memset((genericptr_t) &g.context.digging, 0,
                          sizeof (struct dig_info));
        /* reset cached targets */
        iflags.travelcc.x = iflags.travelcc.y = 0; /* travel destination */
        g.context.polearm.hitmon = (struct monst *) 0; /* polearm target */
        /* escape from trap */
        reset_utrap(FALSE);
        check_special_room(TRUE); /* room exit */
        (void) memset((genericptr_t)&g.dndest, 0, sizeof (dest_area));
        (void) memset((genericptr_t)&g.updest, 0, sizeof (dest_area));
        u.ustuck = (struct monst *) 0;
        u.uswallow = u.uswldtim = 0;
        set_uinwater(0); /* u.uinwater = 0 */
        u.uundetected = 0; /* not hidden, even if means are available */
        dmonsfree(); /* purge dead monsters from 'fmon' */

        /* discard current level; "saving" is used to release dynamic data */
        zero_nhfile(&tmpnhfp);  /* also sets fd to -1 as desired */
        tmpnhfp.mode = FREEING;
        savelev(&tmpnhfp, ledger_no(&u.uz));
    } else {
        vision_reset();
        g.vision_full_recalc = 1;
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

/* #wizmakemap - discard current dungeon level and replace with a new one */
static int
wiz_makemap(void)
{
    if (wizard) {
        boolean was_in_W_tower = In_W_tower(u.ux, u.uy, &u.uz);

        makemap_prepost(TRUE, was_in_W_tower);
        /* create a new level; various things like bestowing a guardian
           angel on Astral or setting off alarm on Ft.Ludios are handled
           by goto_level(do.c) so won't occur for replacement levels */
        mklev();
        makemap_prepost(FALSE, was_in_W_tower);
    } else {
        pline(unavailcmd, ecname_from_fn(wiz_makemap));
    }
    return ECMD_OK;
}

/* the #wizmap command - reveal the level map and any traps on it */
static int
wiz_map(void)
{
    if (wizard) {
        struct trap *t;
        long save_Hconf = HConfusion, save_Hhallu = HHallucination;

        HConfusion = HHallucination = 0L;
        for (t = g.ftrap; t != 0; t = t->ntrap) {
            t->tseen = 1;
            map_trap(t, TRUE);
        }
        do_mapping();
        HConfusion = save_Hconf;
        HHallucination = save_Hhallu;
    } else
        pline(unavailcmd, ecname_from_fn(wiz_map));
    return ECMD_OK;
}

/* #wizgenesis - generate monster(s); a count prefix will be honored */
static int
wiz_genesis(void)
{
    if (wizard)
        (void) create_particular();
    else
        pline(unavailcmd, ecname_from_fn(wiz_genesis));
    return ECMD_OK;
}

/* #wizwhere command - display dungeon layout */
static int
wiz_where(void)
{
    if (wizard)
        (void) print_dungeon(FALSE, (schar *) 0, (xchar *) 0);
    else
        pline(unavailcmd, ecname_from_fn(wiz_where));
    return ECMD_OK;
}

/* the #wizdetect command - detect secret doors, traps, hidden monsters */
static int
wiz_detect(void)
{
    if (wizard)
        (void) findit();
    else
        pline(unavailcmd, ecname_from_fn(wiz_detect));
    return ECMD_OK;
}

/* the #wizloadlua command - load an arbitrary lua file */
static int
wiz_load_lua(void)
{
    if (wizard) {
        char buf[BUFSZ];

        buf[0] = '\0';
        getlin("Load which lua file?", buf);
        if (buf[0] == '\033' || buf[0] == '\0')
            return 0;
        if (!strchr(buf, '.'))
            strcat(buf, ".lua");
        (void) load_lua(buf);
    } else
        pline(unavailcmd, ecname_from_fn(wiz_load_lua));
    return ECMD_OK;
}

/* the #wizloaddes command - load a special level lua file */
static int
wiz_load_splua(void)
{
    if (wizard) {
        char buf[BUFSZ];

        buf[0] = '\0';
        getlin("Load which des lua file?", buf);
        if (buf[0] == '\033' || buf[0] == '\0')
            return 0;
        if (!strchr(buf, '.'))
            strcat(buf, ".lua");

        lspo_reset_level(NULL);
        (void) load_special(buf);
        lspo_finalize_level(NULL);

    } else
        pline(unavailcmd, ecname_from_fn(wiz_load_splua));
    return ECMD_OK;
}

/* the #wizlevelport command - level teleport */
static int
wiz_level_tele(void)
{
    if (wizard)
        level_tele();
    else
        pline(unavailcmd, ecname_from_fn(wiz_level_tele));
    return ECMD_OK;
}

/* #wizfliplevel - transpose the current level */
static int
wiz_flip_level(void)
{
    static const char choices[] = "0123",
        prmpt[] = "Flip 0=randomly, 1=vertically, 2=horizonally, 3=both:";

    /*
     * Does not handle
     *   levregions,
     *   monster mtrack,
     *   migrating monsters aimed at returning to specific coordinates
     *     on this level
     * as flipping is normally done only during level creation.
     */
    if (wizard) {
        char c = yn_function(prmpt, choices, '\0');

        if (c && index(choices, c)) {
            c -= '0';

            if (!c)
                flip_level_rnd(3, TRUE);
            else
                flip_level((int) c, TRUE);

            docrt();
        } else {
            pline("%s", Never_mind);
        }
    }
    return ECMD_OK;
}

/* #levelchange command - adjust hero's experience level */
static int
wiz_level_change(void)
{
    char buf[BUFSZ] = DUMMY;
    int newlevel = 0;
    int ret;

    getlin("To what experience level do you want to be set?", buf);
    (void) mungspaces(buf);
    if (buf[0] == '\033' || buf[0] == '\0')
        ret = 0;
    else
        ret = sscanf(buf, "%d", &newlevel);

    if (ret != 1) {
        pline1(Never_mind);
        return ECMD_OK;
    }
    if (newlevel == u.ulevel) {
        You("are already that experienced.");
    } else if (newlevel < u.ulevel) {
        if (u.ulevel == 1) {
            You("are already as inexperienced as you can get.");
            return ECMD_OK;
        }
        if (newlevel < 1)
            newlevel = 1;
        while (u.ulevel > newlevel)
            losexp("#levelchange");
    } else {
        if (u.ulevel >= MAXULEV) {
            You("are already as experienced as you can get.");
            return ECMD_OK;
        }
        if (newlevel > MAXULEV)
            newlevel = MAXULEV;
        while (u.ulevel < newlevel)
            pluslvl(FALSE);
    }
    u.ulevelmax = u.ulevel;
    return ECMD_OK;
}

DISABLE_WARNING_CONDEXPR_IS_CONSTANT

/* #wiztelekinesis */
static int
wiz_telekinesis(void)
{
    int ans = 0;
    coord cc;
    struct monst *mtmp = (struct monst *) 0;

    cc.x = u.ux;
    cc.y = u.uy;

    pline("Pick a monster to hurtle.");
    do {
        ans = getpos(&cc, TRUE, "a monster");
        if (ans < 0 || cc.x < 0)
            return ECMD_CANCEL;

        if ((((mtmp = m_at(cc.x, cc.y)) != 0) && canspotmon(mtmp))
            || u_at(cc.x, cc.y)) {
            if (!getdir("which direction?"))
                return ECMD_CANCEL;

            if (mtmp) {
                mhurtle(mtmp, u.dx, u.dy, 6);
                if (!DEADMONSTER(mtmp) && canspotmon(mtmp)) {
                    cc.x = mtmp->mx;
                    cc.y = mtmp->my;
                }
            } else {
                hurtle(u.dx, u.dy, 6, FALSE);
                cc.x = u.ux, cc.y = u.uy;
            }
        }

    } while (TRUE);
    return ECMD_OK;
}

RESTORE_WARNING_CONDEXPR_IS_CONSTANT

/* #panic command - test program's panic handling */
static int
wiz_panic(void)
{
    if (iflags.debug_fuzzer) {
        u.uhp = u.uhpmax = 1000;
        u.uen = u.uenmax = 1000;
        return ECMD_OK;
    }
    if (paranoid_query(TRUE,
                       "Do you want to call panic() and end your game?"))
        panic("Crash test.");
    return ECMD_OK;
}

/* #polyself command - change hero's form */
static int
wiz_polyself(void)
{
    polyself(1);
    return ECMD_OK;
}

/* #seenv command */
static int
wiz_show_seenv(void)
{
    winid win;
    int x, y, v, startx, stopx, curx;
    char row[COLNO + 1];

    win = create_nhwindow(NHW_TEXT);
    /*
     * Each seenv description takes up 2 characters, so center
     * the seenv display around the hero.
     */
    startx = max(1, u.ux - (COLNO / 4));
    stopx = min(startx + (COLNO / 2), COLNO);
    /* can't have a line exactly 80 chars long */
    if (stopx - startx == COLNO / 2)
        startx++;

    for (y = 0; y < ROWNO; y++) {
        for (x = startx, curx = 0; x < stopx; x++, curx += 2) {
            if (u_at(x, y)) {
                row[curx] = row[curx + 1] = '@';
            } else {
                v = levl[x][y].seenv & 0xff;
                if (v == 0)
                    row[curx] = row[curx + 1] = ' ';
                else
                    Sprintf(&row[curx], "%02x", v);
            }
        }
        /* remove trailing spaces */
        for (x = curx - 1; x >= 0; x--)
            if (row[x] != ' ')
                break;
        row[x + 1] = '\0';

        putstr(win, 0, row);
    }
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return ECMD_OK;
}

/* #vision command */
static int
wiz_show_vision(void)
{
    winid win;
    int x, y, v;
    char row[COLNO + 1];

    win = create_nhwindow(NHW_TEXT);
    Sprintf(row, "Flags: 0x%x could see, 0x%x in sight, 0x%x temp lit",
            COULD_SEE, IN_SIGHT, TEMP_LIT);
    putstr(win, 0, row);
    putstr(win, 0, "");
    for (y = 0; y < ROWNO; y++) {
        for (x = 1; x < COLNO; x++) {
            if (u_at(x, y)) {
                row[x] = '@';
            } else {
                v = g.viz_array[y][x]; /* data access should be hidden */
                row[x] = (v == 0) ? ' ' : ('0' + v);
            }
        }
        /* remove trailing spaces */
        for (x = COLNO - 1; x >= 1; x--)
            if (row[x] != ' ')
                break;
        row[x + 1] = '\0';

        putstr(win, 0, &row[1]);
    }
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return ECMD_OK;
}

/* #wmode command */
static int
wiz_show_wmodes(void)
{
    winid win;
    int x, y;
    char row[COLNO + 1];
    struct rm *lev;
    boolean istty = WINDOWPORT("tty");

    win = create_nhwindow(NHW_TEXT);
    if (istty)
        putstr(win, 0, ""); /* tty only: blank top line */
    for (y = 0; y < ROWNO; y++) {
        for (x = 0; x < COLNO; x++) {
            lev = &levl[x][y];
            if (u_at(x, y))
                row[x] = '@';
            else if (IS_WALL(lev->typ) || lev->typ == SDOOR)
                row[x] = '0' + (lev->wall_info & WM_MASK);
            else if (lev->typ == CORR)
                row[x] = '#';
            else if (IS_ROOM(lev->typ) || IS_DOOR(lev->typ))
                row[x] = '.';
            else
                row[x] = 'x';
        }
        row[COLNO] = '\0';
        /* map column 0, levl[0][], is off the left edge of the screen */
        putstr(win, 0, &row[1]);
    }
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return ECMD_OK;
}

/* wizard mode variant of #terrain; internal levl[][].typ values in base-36 */
static void
wiz_map_levltyp(void)
{
    winid win;
    int x, y, terrain;
    char row[COLNO + 1];
    boolean istty = !strcmp(windowprocs.name, "tty");

    win = create_nhwindow(NHW_TEXT);
    /* map row 0, levl[][0], is drawn on the second line of tty screen */
    if (istty)
        putstr(win, 0, ""); /* tty only: blank top line */
    for (y = 0; y < ROWNO; y++) {
        /* map column 0, levl[0][], is off the left edge of the screen;
           it should always have terrain type "undiggable stone" */
        for (x = 1; x < COLNO; x++) {
            terrain = levl[x][y].typ;
            /* assumes there aren't more than 10+26+26 terrain types */
            row[x - 1] = (char) ((terrain == STONE && !may_dig(x, y))
                                    ? '*'
                                    : (terrain < 10)
                                       ? '0' + terrain
                                       : (terrain < 36)
                                          ? 'a' + terrain - 10
                                          : 'A' + terrain - 36);
        }
        x--;
        if (levl[0][y].typ != STONE || may_dig(0, y))
            row[x++] = '!';
        row[x] = '\0';
        putstr(win, 0, row);
    }

    {
        char dsc[COLBUFSZ];
        s_level *slev = Is_special(&u.uz);

        Sprintf(dsc, "D:%d,L:%d", u.uz.dnum, u.uz.dlevel);
        /* [dungeon branch features currently omitted] */
        /* special level features */
        if (slev) {
            Sprintf(eos(dsc), " \"%s\"", slev->proto);
            /* special level flags (note: dungeon.def doesn't set `maze'
               or `hell' for any specific levels so those never show up) */
            if (slev->flags.maze_like)
                Strcat(dsc, " mazelike");
            if (slev->flags.hellish)
                Strcat(dsc, " hellish");
            if (slev->flags.town)
                Strcat(dsc, " town");
            if (slev->flags.rogue_like)
                Strcat(dsc, " roguelike");
            /* alignment currently omitted to save space */
        }
        /* level features */
        if (g.level.flags.nfountains)
            Sprintf(eos(dsc), " %c:%d", defsyms[S_fountain].sym,
                    (int) g.level.flags.nfountains);
        if (g.level.flags.nsinks)
            Sprintf(eos(dsc), " %c:%d", defsyms[S_sink].sym,
                    (int) g.level.flags.nsinks);
        if (g.level.flags.has_vault)
            Strcat(dsc, " vault");
        if (g.level.flags.has_shop)
            Strcat(dsc, " shop");
        if (g.level.flags.has_temple)
            Strcat(dsc, " temple");
        if (g.level.flags.has_court)
            Strcat(dsc, " throne");
        if (g.level.flags.has_zoo)
            Strcat(dsc, " zoo");
        if (g.level.flags.has_morgue)
            Strcat(dsc, " morgue");
        if (g.level.flags.has_barracks)
            Strcat(dsc, " barracks");
        if (g.level.flags.has_beehive)
            Strcat(dsc, " hive");
        if (g.level.flags.has_swamp)
            Strcat(dsc, " swamp");
        /* level flags */
        if (g.level.flags.noteleport)
            Strcat(dsc, " noTport");
        if (g.level.flags.hardfloor)
            Strcat(dsc, " noDig");
        if (g.level.flags.nommap)
            Strcat(dsc, " noMMap");
        if (!g.level.flags.hero_memory)
            Strcat(dsc, " noMem");
        if (g.level.flags.shortsighted)
            Strcat(dsc, " shortsight");
        if (g.level.flags.graveyard)
            Strcat(dsc, " graveyard");
        if (g.level.flags.is_maze_lev)
            Strcat(dsc, " maze");
        if (g.level.flags.is_cavernous_lev)
            Strcat(dsc, " cave");
        if (g.level.flags.arboreal)
            Strcat(dsc, " tree");
        if (Sokoban)
            Strcat(dsc, " sokoban-rules");
        /* non-flag info; probably should include dungeon branching
           checks (extra stairs and magic portals) here */
        if (Invocation_lev(&u.uz))
            Strcat(dsc, " invoke");
        if (On_W_tower_level(&u.uz))
            Strcat(dsc, " tower");
        /* append a branch identifier for completeness' sake */
        if (u.uz.dnum == 0)
            Strcat(dsc, " dungeon");
        else if (u.uz.dnum == mines_dnum)
            Strcat(dsc, " mines");
        else if (In_sokoban(&u.uz))
            Strcat(dsc, " sokoban");
        else if (u.uz.dnum == quest_dnum)
            Strcat(dsc, " quest");
        else if (Is_knox(&u.uz))
            Strcat(dsc, " ludios");
        else if (u.uz.dnum == 1)
            Strcat(dsc, " gehennom");
        else if (u.uz.dnum == tower_dnum)
            Strcat(dsc, " vlad");
        else if (In_endgame(&u.uz))
            Strcat(dsc, " endgame");
        else {
            /* somebody's added a dungeon branch we're not expecting */
            const char *brname = g.dungeons[u.uz.dnum].dname;

            if (!brname || !*brname)
                brname = "unknown";
            if (!strncmpi(brname, "the ", 4))
                brname += 4;
            Sprintf(eos(dsc), " %s", brname);
        }
        /* limit the line length to map width */
        if (strlen(dsc) >= COLNO)
            dsc[COLNO - 1] = '\0'; /* truncate */
        putstr(win, 0, dsc);
    }

    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return;
}

/* temporary? hack, since level type codes aren't the same as screen
   symbols and only the latter have easily accessible descriptions */
const char *levltyp[] = {
    "stone", "vertical wall", "horizontal wall", "top-left corner wall",
    "top-right corner wall", "bottom-left corner wall",
    "bottom-right corner wall", "cross wall", "tee-up wall", "tee-down wall",
    "tee-left wall", "tee-right wall", "drawbridge wall", "tree",
    "secret door", "secret corridor", "pool", "moat", "water",
    "drawbridge up", "lava pool", "iron bars", "door", "corridor", "room",
    "stairs", "ladder", "fountain", "throne", "sink", "grave", "altar", "ice",
    "drawbridge down", "air", "cloud",
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

DISABLE_WARNING_FORMAT_NONLITERAL

/* explanation of base-36 output from wiz_map_levltyp() */
static void
wiz_levltyp_legend(void)
{
    winid win;
    int i, j, last, c;
    const char *dsc, *fmt;
    char buf[BUFSZ];

    win = create_nhwindow(NHW_TEXT);
    putstr(win, 0, "#terrain encodings:");
    putstr(win, 0, "");
    fmt = " %c - %-28s"; /* TODO: include tab-separated variant for win32 */
    *buf = '\0';
    /* output in pairs, left hand column holds [0],[1],...,[N/2-1]
       and right hand column holds [N/2],[N/2+1],...,[N-1];
       N ('last') will always be even, and may or may not include
       the empty string entry to pad out the final pair, depending
       upon how many other entries are present in levltyp[] */
    last = SIZE(levltyp) & ~1;
    for (i = 0; i < last / 2; ++i)
        for (j = i; j < last; j += last / 2) {
            dsc = levltyp[j];
            c = !*dsc ? ' '
                   : !strncmp(dsc, "unreachable", 11) ? '*'
                      /* same int-to-char conversion as wiz_map_levltyp() */
                      : (j < 10) ? '0' + j
                         : (j < 36) ? 'a' + j - 10
                            : 'A' + j - 36;
            Sprintf(eos(buf), fmt, c, dsc);
            if (j > i) {
                putstr(win, 0, buf);
                *buf = '\0';
            }
        }
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return;
}

RESTORE_WARNING_FORMAT_NONLITERAL

DISABLE_WARNING_CONDEXPR_IS_CONSTANT

/* #wizsmell command - test usmellmon(). */
static int
wiz_smell(void)
{
    int ans = 0;
    int mndx;  /* monster index */
    coord cc;  /* screen pos of unknown glyph */
    int glyph; /* glyph at selected position */

    cc.x = u.ux;
    cc.y = u.uy;
    mndx = 0; /* gcc -Wall lint */
    if (!olfaction(g.youmonst.data)) {
        You("are incapable of detecting odors in your present form.");
        return ECMD_OK;
    }

    pline("You can move the cursor to a monster that you want to smell.");
    do {
        pline("Pick a monster to smell.");
        ans = getpos(&cc, TRUE, "a monster");
        if (ans < 0 || cc.x < 0) {
            return ECMD_CANCEL; /* done */
        }
        /* Convert the glyph at the selected position to a mndxbol. */
        glyph = glyph_at(cc.x, cc.y);
        if (glyph_is_monster(glyph))
            mndx = glyph_to_mon(glyph);
        else
            mndx = 0;
        /* Is it a monster? */
        if (mndx) {
            if (!usmellmon(&mons[mndx]))
                pline("That monster seems to give off no smell.");
        } else
            pline("That is not a monster.");
    } while (TRUE);
    return ECMD_OK;
}

RESTORE_WARNING_CONDEXPR_IS_CONSTANT

DISABLE_WARNING_FORMAT_NONLITERAL

#define DEFAULT_TIMEOUT_INCR 30

/* #wizinstrinsic command to set some intrinsics for testing */
static int
wiz_intrinsic(void)
{
    if (wizard) {
        extern const struct propname {
            int prop_num;
            const char *prop_name;
        } propertynames[]; /* timeout.c */
        static const char wizintrinsic[] = "#wizintrinsic";
        static const char fmt[] = "You are%s %s.";
        winid win;
        anything any;
        char buf[BUFSZ];
        int i, j, n, p, amt, typ;
        long oldtimeout, newtimeout;
        const char *propname;
        menu_item *pick_list = (menu_item *) 0;

        any = cg.zeroany;
        win = create_nhwindow(NHW_MENU);
        start_menu(win, MENU_BEHAVE_STANDARD);
        if (iflags.cmdassist) {
            /* start menu with a subtitle */
            Sprintf(buf,
        "[Precede any selection with a count to increment by other than %d.]",
                    DEFAULT_TIMEOUT_INCR);
            any.a_int = 0;
            add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, buf,
                     MENU_ITEMFLAGS_NONE);
        }
        for (i = 0; (propname = propertynames[i].prop_name) != 0; ++i) {
            p = propertynames[i].prop_num;
            if (p == HALLUC_RES) {
                /* Grayswandir vs hallucination; ought to be redone to
                   use u.uprops[HALLUC].blocked instead of being treated
                   as a separate property; letting in be manually toggled
                   even only in wizard mode would be asking for trouble... */
                continue;
            }
            if (p == FIRE_RES) {
                /* FIRE_RES and properties beyond it (in the propertynames[]
                   ordering, not their numerical PROP values), can only be
                   set to timed values here so show a separator */
                any.a_int = 0;
                add_menu(win, &nul_glyphinfo, &any, 0, 0,
                         ATR_NONE, "--", MENU_ITEMFLAGS_NONE);
            }
            any.a_int = i + 1; /* +1: avoid 0 */
            oldtimeout = u.uprops[p].intrinsic & TIMEOUT;
            if (oldtimeout)
                Sprintf(buf, "%-27s [%li]", propname, oldtimeout);
            else
                Sprintf(buf, "%s", propname);
            add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, buf,
                     MENU_ITEMFLAGS_NONE);
        }
        end_menu(win, "Which intrinsics?");
        n = select_menu(win, PICK_ANY, &pick_list);
        destroy_nhwindow(win);

        for (j = 0; j < n; ++j) {
            i = pick_list[j].item.a_int - 1; /* -1: reverse +1 above */
            p = propertynames[i].prop_num;
            oldtimeout = u.uprops[p].intrinsic & TIMEOUT;
            amt = (pick_list[j].count == -1L) ? DEFAULT_TIMEOUT_INCR
                                              : (int) pick_list[j].count;
            if (amt <= 0) /* paranoia */
                continue;
            newtimeout = oldtimeout + (long) amt;

            switch (p) {
            case SICK:
            case SLIMED:
            case STONED:
                if (oldtimeout > 0L && newtimeout > oldtimeout)
                    newtimeout = oldtimeout;
                break;
            }

            switch (p) {
            case BLINDED:
                make_blinded(newtimeout, TRUE);
                break;
#if 0       /* make_confused() only gives feedback when confusion is
             * ending so use the 'default' case for it instead */
            case CONFUSION:
                make_confused(newtimeout, TRUE);
                break;
#endif /*0*/
            case DEAF:
                make_deaf(newtimeout, TRUE);
                break;
            case HALLUC:
                make_hallucinated(newtimeout, TRUE, 0L);
                break;
            case SICK:
                typ = !rn2(2) ? SICK_VOMITABLE : SICK_NONVOMITABLE;
                make_sick(newtimeout, wizintrinsic, TRUE, typ);
                break;
            case SLIMED:
                Sprintf(buf, fmt,
                        !Slimed ? "" : " still", "turning into slime");
                make_slimed(newtimeout, buf);
                break;
            case STONED:
                Sprintf(buf, fmt,
                        !Stoned ? "" : " still", "turning into stone");
                make_stoned(newtimeout, buf, KILLED_BY, wizintrinsic);
                break;
            case STUNNED:
                make_stunned(newtimeout, TRUE);
                break;
            case VOMITING:
                Sprintf(buf, fmt, !Vomiting ? "" : " still", "vomiting");
                make_vomiting(newtimeout, FALSE);
                pline1(buf);
                break;
            case WARN_OF_MON:
                if (!Warn_of_mon) {
                    g.context.warntype.speciesidx = PM_GRID_BUG;
                    g.context.warntype.species
                                       = &mons[g.context.warntype.speciesidx];
                }
                goto def_feedback;
            case GLIB:
                /* slippery fingers might need a persistent inventory update
                   so needs more than simple incr_itimeout() but we want
                   the pline() issued with that */
                make_glib((int) newtimeout);
                /*FALLTHRU*/
            default:
 def_feedback:
                if (p != GLIB)
                    incr_itimeout(&u.uprops[p].intrinsic, amt);
                g.context.botl = 1; /* have pline() do a status update */
                pline("Timeout for %s %s %d.", propertynames[i].prop_name,
                      oldtimeout ? "increased by" : "set to", amt);
                break;
            }
            /* this has to be after incr_timeout() */
            if (p == LEVITATION || p == FLYING)
                float_vs_flight();
        }
        if (n >= 1)
            free((genericptr_t) pick_list);
        doredraw();
    } else
        pline(unavailcmd, ecname_from_fn(wiz_intrinsic));
    return ECMD_OK;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* #wizrumorcheck command - verify each rumor access */
static int
wiz_rumor_check(void)
{
    rumor_check();
    return ECMD_OK;
}

/* #terrain command -- show known map, inspired by crawl's '|' command */
static int
doterrain(void)
{
    winid men;
    menu_item *sel;
    anything any;
    int n;
    int which;

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
    add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
             "known map without monsters, objects, and traps",
             MENU_ITEMFLAGS_SELECTED);
    any.a_int = 2;
    add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
             "known map without monsters and objects",
             MENU_ITEMFLAGS_NONE);
    any.a_int = 3;
    add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
             "known map without monsters",
             MENU_ITEMFLAGS_NONE);
    if (discover || wizard) {
        any.a_int = 4;
        add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 "full map without monsters, objects, and traps",
                 MENU_ITEMFLAGS_NONE);
        if (wizard) {
            any.a_int = 5;
            add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                     "internal levl[][].typ codes in base-36",
                     MENU_ITEMFLAGS_NONE);
            any.a_int = 6;
            add_menu(men, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                     "legend of base-36 levl[][].typ codes",
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
        reveal_terrain(0, TER_MAP);
        break;
    case 2: /* known map with known traps */
        reveal_terrain(0, TER_MAP | TER_TRP);
        break;
    case 3: /* known map with known traps and objects */
        reveal_terrain(0, TER_MAP | TER_TRP | TER_OBJ);
        break;
    case 4: /* full map */
        reveal_terrain(1, TER_MAP);
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

void
set_move_cmd(int dir, int run)
{
    u.dz = zdir[dir];
    u.dx = xdir[dir];
    u.dy = ydir[dir];
    /* #reqmenu -prefix disables autopickup during movement */
    if (iflags.menu_requested)
        g.context.nopick = 1;
    g.context.travel = g.context.travel1 = 0;
    if (!g.domove_attempting && !u.dz) {
        g.context.run = run;
        g.domove_attempting |= (!run ? DOMOVE_WALK : DOMOVE_RUSH);
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
    if ((g.domove_attempting & DOMOVE_RUSH)) {
        Norep("Double rush prefix, canceled.");
        g.context.run = 0;
        g.domove_attempting = 0;
        return ECMD_CANCEL;
    }

    g.context.run = 2;
    g.domove_attempting |= DOMOVE_RUSH;
    return ECMD_OK;
}

/* #run */
int
do_run(void)
{
    if ((g.domove_attempting & DOMOVE_RUSH)) {
        Norep("Double run prefix, canceled.");
        g.context.run = 0;
        g.domove_attempting = 0;
        return ECMD_CANCEL;
    }

    g.context.run = 3;
    g.domove_attempting |= DOMOVE_RUSH;
    return ECMD_OK;
}

/* #fight */
int
do_fight(void)
{
    if (g.context.forcefight) {
        Norep("Double fight prefix, canceled.");
        g.context.forcefight = 0;
        g.domove_attempting = 0;
        return ECMD_CANCEL;
    }

    g.context.forcefight = 1;
    g.domove_attempting |= DOMOVE_WALK;
    return ECMD_OK;
}

/* #repeat */
int
do_repeat(void)
{
    if (!g.in_doagain && g.saveq[0]) {
        g.in_doagain = TRUE;
        g.stail = 0;
        rhack((char *) 0); /* read and execute command */
        g.in_doagain = FALSE;
        iflags.menu_requested = FALSE;
        return ECMD_TIME;
    }
    return ECMD_OK;
}

/* extcmdlist: full command list, ordered by command name;
   commands with no keystroke or with only a meta keystroke generally
   need to be flagged as autocomplete and ones with a regular keystroke
   or control keystroke generally should not be; there are a few exceptions
   such as ^O/#overview and C/N/#name */
struct ext_func_tab extcmdlist[] = {
    { '#',    "#", "perform an extended command",
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
    { M('d'), "dip", "dip an object into something",
              dodip, AUTOCOMPLETE | CMD_M_PREFIX, NULL },
    { '>',    "down", "go down a staircase",
              /* allows 'm' prefix (for move without autopickup) but not the
                 g/G/F movement modifiers; not flagged as MOVEMENTCMD because
                 that would would suppress it from dokeylist output */
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
    { M('l'), "loot", "loot a box on the floor",
              doloot, AUTOCOMPLETE | CMD_M_PREFIX, NULL },
#ifdef DEBUG_MIGRATING_MONS
    { '\0',   "migratemons", "migrate N random monsters",
              wiz_migrate_mons, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
#endif
    { M('m'), "monster", "use monster's special ability",
              domonability, IFBURIED | AUTOCOMPLETE, NULL },
    { 'N',    "name", "same as call; name a monster or object or object type",
              docallcmd, IFBURIED | AUTOCOMPLETE, NULL },
    { M('o'), "offer", "offer a sacrifice to the gods",
              dosacrifice, AUTOCOMPLETE | CMD_M_PREFIX, NULL },
    { 'o',    "open", "open a door",
              doopen, 0, NULL },
    { 'O',    "options", "show option settings, possibly change them",
              doset, IFBURIED | GENERALCMD, NULL },
    /* #overview used to need autocomplete and has retained that even
       after being assigned to ^O [old wizard mode ^O is now #wizwhere] */
    { C('o'), "overview", "show a summary of the explored dungeon",
              dooverview, IFBURIED | AUTOCOMPLETE | GENERALCMD, NULL },
    /* [should #panic actually autocomplete?] */
    { '\0',   "panic", "test panic routine (fatal to game)",
              wiz_panic, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { 'p',    "pay", "pay your shopping bill",
              dopay, 0, NULL },
    { '|',    "perminv", "scroll persistent inventory display",
              doperminv, IFBURIED | GENERALCMD | NOFUZZERCMD, NULL },
    { ',',    "pickup", "pick up things at the current location",
              dopickup, CMD_M_PREFIX, NULL },
    { '\0',   "polyself", "polymorph self",
              wiz_polyself, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
    { M('p'), "pray", "pray to the gods for help",
              dopray, IFBURIED | AUTOCOMPLETE, NULL },
    { C('p'), "prevmsg", "view recent game messages",
              doprev_message, IFBURIED | GENERALCMD, NULL },
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
              doredraw, IFBURIED | GENERALCMD, NULL },
    { 'R',    "remove", "remove an accessory (ring, amulet, etc)",
              doremring, 0, NULL },
    { C('a'), "repeat", "repeat a previous command",
              do_repeat, IFBURIED | GENERALCMD, NULL },
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
    { 's',    "search", "search for traps and secret doors",
              dosearch, IFBURIED | CMD_M_PREFIX, "searching" },
    { '*',    "seeall", "show all equipment in use",
              doprinuse, IFBURIED, NULL },
    { AMULET_SYM, "seeamulet", "show the amulet currently worn",
              dopramulet, IFBURIED, NULL },
    { ARMOR_SYM, "seearmor", "show the armor currently worn",
              doprarm, IFBURIED, NULL },
    { RING_SYM, "seerings", "show the ring(s) currently worn",
              doprring, IFBURIED, NULL },
    { TOOL_SYM, "seetools", "show the tools currently in use",
              doprtool, IFBURIED, NULL },
    { WEAPON_SYM, "seeweapon", "show the weapon currently wielded",
              doprwep, IFBURIED, NULL },
    { '!', "shell", "leave game to enter a sub-shell ('exit' to come back)",
              dosh_core, (IFBURIED | GENERALCMD | NOFUZZERCMD
#ifndef SHELL
                        | CMD_NOT_AVAILABLE
#endif /* SHELL */
                        ), NULL },
    /* $ is like ),=,&c but is not included with *, so not called "seegold" */
    { GOLD_SYM, "showgold", "show gold, possibly shop credit or debt",
              doprgold, IFBURIED, NULL },
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
    /* therecmdmenu does not work as intended, should probably be removed */
    { '\0',   "therecmdmenu",
              "menu of commands you can do from here to adjacent spot",
              dotherecmdmenu, AUTOCOMPLETE | GENERALCMD, NULL },
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
    { '\0',   "vanquished", "list vanquished monsters",
              dovanquished, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
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
    { C('e'), "wizdetect", "reveal hidden things within a small radius",
              wiz_detect, IFBURIED | WIZMODECMD, NULL },
    { '\0',   "wizfliplevel", "flip the level",
              wiz_flip_level, IFBURIED | WIZMODECMD, NULL },
    { C('g'), "wizgenesis", "create a monster",
              wiz_genesis, IFBURIED | WIZMODECMD, NULL },
    { C('i'), "wizidentify", "identify all items in inventory",
              wiz_identify, IFBURIED | WIZMODECMD, NULL },
    { '\0',   "wizintrinsic", "set an intrinsic",
              wiz_intrinsic, IFBURIED | AUTOCOMPLETE | WIZMODECMD, NULL },
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
    { '\0', "clicklook", NULL, doclicklook, INTERNALCMD, NULL },
    { '\0', "altdip", NULL, dip_into, INTERNALCMD, NULL },
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

/* used by dokeylist() and by key2extcmdesc() for dowhatdoes() */
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

int extcmdlist_length = SIZE(extcmdlist) - 1;

/* get entry i in the extended commands list. for windowport use. */
struct ext_func_tab *
extcmds_getentry(int i)
{
    if (i < 0 || i > extcmdlist_length)
        return 0;
    return &extcmdlist[i];
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
    if (digit(key) || (g.Cmd.num_pad && digit(unmeta(key)))) {
        key2cmdbuf[0] = '\0';
        if (!g.Cmd.num_pad)
            Strcpy(key2cmdbuf, "start of, or continuation of, a count");
        else if (key == '5' || key == M_5)
            Sprintf(key2cmdbuf, "%s prefix",
                    (!!g.Cmd.pcHack_compat ^ (key == M_5)) ? "run" : "rush");
        else if (key == '0' || (g.Cmd.pcHack_compat && key == M_0))
            Strcpy(key2cmdbuf, "synonym for 'i'");
        if (*key2cmdbuf)
            return key2cmdbuf;
    }
    /* check prefixes before regular commands; includes ^A pseudo-command */
    for (i = 0; misc_keys[i].desc; ++i) {
        if (misc_keys[i].numpad && !iflags.num_pad)
            continue;
        j = misc_keys[i].nhkf;
        if (key == (uchar) g.Cmd.spkeys[j])
            return misc_keys[i].desc;
    }
    /* finally, check whether 'key' is a command */
    if (g.Cmd.commands[key]) {
        if (g.Cmd.commands[key]->ef_txt) {
            Sprintf(key2cmdbuf, "%s (#%s)",
                    g.Cmd.commands[key]->ef_desc,
                    g.Cmd.commands[key]->ef_txt);
            return key2cmdbuf;
        }
    }
    return (char *) 0;
}

boolean
bind_key(uchar key, const char *command)
{
    struct ext_func_tab *extcmd;

    /* special case: "nothing" is reserved for unbinding */
    if (!strcmpi(command, "nothing")) {
        g.Cmd.commands[key] = (struct ext_func_tab *) 0;
        return TRUE;
    }

    for (extcmd = extcmdlist; extcmd->ef_txt; extcmd++) {
        if (strcmpi(command, extcmd->ef_txt))
            continue;
        if ((extcmd->flags & INTERNALCMD) != 0)
            continue;
        g.Cmd.commands[key] = extcmd;
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
static boolean
bind_key_fn(uchar key, int (*fn)(void))
{
    struct ext_func_tab *extcmd;

    for (extcmd = extcmdlist; extcmd->ef_txt; extcmd++) {
        if (extcmd->ef_funct != fn)
            continue;
        if ((extcmd->flags & INTERNALCMD) != 0)
            continue;
        g.Cmd.commands[key] = extcmd;
        return TRUE;
    }

    return FALSE;
}

/* initialize all keyboard commands */
static void
commands_init(void)
{
    struct ext_func_tab *extcmd;

    for (extcmd = extcmdlist; extcmd->ef_txt; extcmd++)
        if (extcmd->key)
            g.Cmd.commands[extcmd->key] = extcmd;

    /* number_pad */
    (void) bind_key(C('l'), "redraw");
    (void) bind_key('h',    "help");
    (void) bind_key('j',    "jump");
    (void) bind_key('k',    "kick");
    (void) bind_key('l',    "loot");
    (void) bind_key(C('n'), "annotate");
    (void) bind_key('u',    "untrap");
    (void) bind_key('5',    "run");
    (void) bind_key(M('5'), "rush");
    (void) bind_key('-',    "fight");

    /* alt keys: */
    (void) bind_key(M('O'), "overview");
    (void) bind_key(M('2'), "twoweapon");
    (void) bind_key(M('n'), "name");
    (void) bind_key(M('N'), "name");
#if 0
    /* don't do this until the rest_on_space option is set or cleared */
    (void) bind_key(' ',    "wait");
#endif
}

static boolean
keylist_func_has_key(const struct ext_func_tab *extcmd,
                     boolean *skip_keys_used) /* boolean keys_used[256] */
{
    int i;

    for (i = 0; i < 256; ++i) {
        if (skip_keys_used[i])
            continue;

        if (g.Cmd.commands[i] == extcmd)
            return TRUE;
    }
    return FALSE;
}

static int
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
        if ((extcmd = g.Cmd.commands[i]) != (struct ext_func_tab *) 0) {
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
        key = (uchar) g.Cmd.spkeys[j];
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
        key = (uchar) g.Cmd.spkeys[j];
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
            key = (uchar) g.Cmd.spkeys[j];
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

char
cmd_from_func(int (*fn)(void))
{
    int i;

    /* skip NUL; allowing it would wreak havoc */
    for (i = 1; i < 256; ++i) {
        /* skip space; we'll use it below as last resort if no other
           keystroke invokes space's command */
        if (i == ' ')
            continue;
        /* skip digits if number_pad is Off; also skip '-' unless it has
           been bound to something other than what number_pad assigns */
        if (((i >= '0' && i <= '9') || (i == '-' && fn == do_fight))
            && !g.Cmd.num_pad)
            continue;

        if (g.Cmd.commands[i] && g.Cmd.commands[i]->ef_funct == fn)
            return (char) i;
    }
    if (g.Cmd.commands[' '] && g.Cmd.commands[' ']->ef_funct == fn)
        return ' ';
    return '\0';
}

static const char *
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
        int len = 0;

        /* find the shortest leading substring which is unambiguous */
        do {
            if (++len >= (int) strlen(res))
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
        debugpline2("shortened %s: \"%s\"", res, outbuf);
        res = outbuf;
    }
    return res;
}

/*
 * wizard mode sanity_check code
 */

static const char template[] = "%-27s  %4ld  %6ld";
static const char stats_hdr[] = "                             count  bytes";
static const char stats_sep[] = "---------------------------  ----- -------";

static int
size_obj(struct obj *otmp)
{
    int sz = (int) sizeof (struct obj);

    if (otmp->oextra) {
        sz += (int) sizeof (struct oextra);
        if (ONAME(otmp))
            sz += (int) strlen(ONAME(otmp)) + 1;
        if (OMONST(otmp))
            sz += size_monst(OMONST(otmp), FALSE);
        if (OMAILCMD(otmp))
            sz += (int) strlen(OMAILCMD(otmp)) + 1;
        /* sz += (int) sizeof (unsigned); -- now part of oextra itself */
    }
    return sz;
}

static void
count_obj(struct obj *chain, long *total_count, long *total_size,
          boolean top, boolean recurse)
{
    long count, size;
    struct obj *obj;

    for (count = size = 0, obj = chain; obj; obj = obj->nobj) {
        if (top) {
            count++;
            size += size_obj(obj);
        }
        if (recurse && obj->cobj)
            count_obj(obj->cobj, total_count, total_size, TRUE, TRUE);
    }
    *total_count += count;
    *total_size += size;
}

DISABLE_WARNING_FORMAT_NONLITERAL  /* RESTORE_WARNING follows show_wiz_stats */

static void
obj_chain(winid win, const char *src, struct obj *chain, boolean force,
          long *total_count, long *total_size)
{
    char buf[BUFSZ];
    long count = 0L, size = 0L;

    count_obj(chain, &count, &size, TRUE, FALSE);

    if (count || size || force) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, src, count, size);
        putstr(win, 0, buf);
    }
}

static void
mon_invent_chain(winid win, const char *src, struct monst *chain,
                 long *total_count, long *total_size)
{
    char buf[BUFSZ];
    long count = 0, size = 0;
    struct monst *mon;

    for (mon = chain; mon; mon = mon->nmon)
        count_obj(mon->minvent, &count, &size, TRUE, FALSE);

    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, src, count, size);
        putstr(win, 0, buf);
    }
}

static void
contained_stats(winid win, const char *src, long *total_count,
                long *total_size)
{
    char buf[BUFSZ];
    long count = 0, size = 0;
    struct monst *mon;

    count_obj(g.invent, &count, &size, FALSE, TRUE);
    count_obj(fobj, &count, &size, FALSE, TRUE);
    count_obj(g.level.buriedobjlist, &count, &size, FALSE, TRUE);
    count_obj(g.migrating_objs, &count, &size, FALSE, TRUE);
    /* DEADMONSTER check not required in this loop since they have no
     * inventory */
    for (mon = fmon; mon; mon = mon->nmon)
        count_obj(mon->minvent, &count, &size, FALSE, TRUE);
    for (mon = g.migrating_mons; mon; mon = mon->nmon)
        count_obj(mon->minvent, &count, &size, FALSE, TRUE);

    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, src, count, size);
        putstr(win, 0, buf);
    }
}

static int
size_monst(struct monst *mtmp, boolean incl_wsegs)
{
    int sz = (int) sizeof (struct monst);

    if (mtmp->wormno && incl_wsegs)
        sz += size_wseg(mtmp);

    if (mtmp->mextra) {
        sz += (int) sizeof (struct mextra);
        if (MGIVENNAME(mtmp))
            sz += (int) strlen(MGIVENNAME(mtmp)) + 1;
        if (EGD(mtmp))
            sz += (int) sizeof (struct egd);
        if (EPRI(mtmp))
            sz += (int) sizeof (struct epri);
        if (ESHK(mtmp))
            sz += (int) sizeof (struct eshk);
        if (EMIN(mtmp))
            sz += (int) sizeof (struct emin);
        if (EDOG(mtmp))
            sz += (int) sizeof (struct edog);
        /* mextra->mcorpsenm doesn't point to more memory */
    }
    return sz;
}

static void
mon_chain(winid win, const char *src, struct monst *chain,
          boolean force, long *total_count, long *total_size)
{
    char buf[BUFSZ];
    long count, size;
    struct monst *mon;
    /* mon->wormno means something different for migrating_mons and mydogs */
    boolean incl_wsegs = !strcmpi(src, "fmon");

    count = size = 0L;
    for (mon = chain; mon; mon = mon->nmon) {
        count++;
        size += size_monst(mon, incl_wsegs);
    }
    if (count || size || force) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, src, count, size);
        putstr(win, 0, buf);
    }
}

static void
misc_stats(winid win, long *total_count, long *total_size)
{
    char buf[BUFSZ], hdrbuf[QBUFSZ];
    long count, size;
    int idx;
    struct trap *tt;
    struct damage *sd; /* shop damage */
    struct kinfo *k; /* delayed killer */
    struct cemetery *bi; /* bones info */

    /* traps and engravings are output unconditionally;
     * others only if nonzero
     */
    count = size = 0L;
    for (tt = g.ftrap; tt; tt = tt->ntrap) {
        ++count;
        size += (long) sizeof *tt;
    }
    *total_count += count;
    *total_size += size;
    Sprintf(hdrbuf, "traps, size %ld", (long) sizeof (struct trap));
    Sprintf(buf, template, hdrbuf, count, size);
    putstr(win, 0, buf);

    count = size = 0L;
    engr_stats("engravings, size %ld+text", hdrbuf, &count, &size);
    *total_count += count;
    *total_size += size;
    Sprintf(buf, template, hdrbuf, count, size);
    putstr(win, 0, buf);

    count = size = 0L;
    light_stats("light sources, size %ld", hdrbuf, &count, &size);
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    timer_stats("timers, size %ld", hdrbuf, &count, &size);
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    for (sd = g.level.damagelist; sd; sd = sd->next) {
        ++count;
        size += (long) sizeof *sd;
    }
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(hdrbuf, "shop damage, size %ld",
                (long) sizeof (struct damage));
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    region_stats("regions, size %ld+%ld*rect+N", hdrbuf, &count, &size);
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    for (k = g.killer.next; k; k = k->next) {
        ++count;
        size += (long) sizeof *k;
    }
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(hdrbuf, "delayed killer%s, size %ld",
                plur(count), (long) sizeof (struct kinfo));
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    for (bi = g.level.bonesinfo; bi; bi = bi->next) {
        ++count;
        size += (long) sizeof *bi;
    }
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(hdrbuf, "bones history, size %ld",
                (long) sizeof (struct cemetery));
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    for (idx = 0; idx < NUM_OBJECTS; ++idx)
        if (objects[idx].oc_uname) {
            ++count;
            size += (long) (strlen(objects[idx].oc_uname) + 1);
        }
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Strcpy(hdrbuf, "object type names, text");
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }
}

/* the #stats command
 * Display memory usage of all monsters and objects on the level.
 */
static int
wiz_show_stats(void)
{
    char buf[BUFSZ];
    winid win;
    long total_obj_size, total_obj_count,
         total_mon_size, total_mon_count,
         total_ovr_size, total_ovr_count,
         total_misc_size, total_misc_count;

    win = create_nhwindow(NHW_TEXT);
    putstr(win, 0, "Current memory statistics:");

    total_obj_count = total_obj_size = 0L;
    putstr(win, 0, stats_hdr);
    Sprintf(buf, "  Objects, base size %ld", (long) sizeof (struct obj));
    putstr(win, 0, buf);
    obj_chain(win, "invent", g.invent, TRUE,
              &total_obj_count, &total_obj_size);
    obj_chain(win, "fobj", fobj, TRUE, &total_obj_count, &total_obj_size);
    obj_chain(win, "buried", g.level.buriedobjlist, FALSE,
              &total_obj_count, &total_obj_size);
    obj_chain(win, "migrating obj", g.migrating_objs, FALSE,
              &total_obj_count, &total_obj_size);
    obj_chain(win, "billobjs", g.billobjs, FALSE,
              &total_obj_count, &total_obj_size);
    mon_invent_chain(win, "minvent", fmon, &total_obj_count, &total_obj_size);
    mon_invent_chain(win, "migrating minvent", g.migrating_mons,
                     &total_obj_count, &total_obj_size);
    contained_stats(win, "contained", &total_obj_count, &total_obj_size);
    putstr(win, 0, stats_sep);
    Sprintf(buf, template, "  Obj total", total_obj_count, total_obj_size);
    putstr(win, 0, buf);

    total_mon_count = total_mon_size = 0L;
    putstr(win, 0, "");
    Sprintf(buf, "  Monsters, base size %ld", (long) sizeof (struct monst));
    putstr(win, 0, buf);
    mon_chain(win, "fmon", fmon, TRUE, &total_mon_count, &total_mon_size);
    mon_chain(win, "migrating", g.migrating_mons, FALSE,
              &total_mon_count, &total_mon_size);
    /* 'g.mydogs' is only valid during level change or end of game disclosure,
       but conceivably we've been called from within debugger at such time */
    if (g.mydogs) /* monsters accompanying hero */
        mon_chain(win, "mydogs", g.mydogs, FALSE,
                  &total_mon_count, &total_mon_size);
    putstr(win, 0, stats_sep);
    Sprintf(buf, template, "  Mon total", total_mon_count, total_mon_size);
    putstr(win, 0, buf);

    total_ovr_count = total_ovr_size = 0L;
    putstr(win, 0, "");
    putstr(win, 0, "  Overview");
    overview_stats(win, template, &total_ovr_count, &total_ovr_size);
    putstr(win, 0, stats_sep);
    Sprintf(buf, template, "  Over total", total_ovr_count, total_ovr_size);
    putstr(win, 0, buf);

    total_misc_count = total_misc_size = 0L;
    putstr(win, 0, "");
    putstr(win, 0, "  Miscellaneous");
    misc_stats(win, &total_misc_count, &total_misc_size);
    putstr(win, 0, stats_sep);
    Sprintf(buf, template, "  Misc total", total_misc_count, total_misc_size);
    putstr(win, 0, buf);

    putstr(win, 0, "");
    putstr(win, 0, stats_sep);
    Sprintf(buf, template, "  Grand total",
            (total_obj_count + total_mon_count
             + total_ovr_count + total_misc_count),
            (total_obj_size + total_mon_size
             + total_ovr_size + total_misc_size));
    putstr(win, 0, buf);

#if defined(__BORLANDC__) && !defined(_WIN32)
    show_borlandc_stats(win);
#endif

    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);
    return ECMD_OK;
}

RESTORE_WARNING_FORMAT_NONLITERAL

void
sanity_check(void)
{
    (void) check_invent_gold("invent");
    obj_sanity_check();
    timer_sanity_check();
    mon_sanity_check();
    light_sources_sanity_check();
    bc_sanity_check();
    trap_sanity_check();
}

#ifdef DEBUG_MIGRATING_MONS
static int
wiz_migrate_mons(void)
{
    int mcount = 0;
    char inbuf[BUFSZ] = DUMMY;
    struct permonst *ptr;
    struct monst *mtmp;
    d_level tolevel;

    getlin("How many random monsters to migrate? [0]", inbuf);
    if (*inbuf == '\033')
        return ECMD_OK;
    mcount = atoi(inbuf);
    if (mcount < 0 || mcount > (COLNO * ROWNO) || Is_botlevel(&u.uz))
        return ECMD_OK;
    while (mcount > 0) {
        if (Is_stronghold(&u.uz))
            assign_level(&tolevel, &valley_level);
        else
            get_level(&tolevel, depth(&u.uz) + 1);
        ptr = rndmonst();
        mtmp = makemon(ptr, 0, 0, MM_NOMSG);
        if (mtmp)
            migrate_to_level(mtmp, ledger_no(&tolevel), MIGR_RANDOM,
                             (coord *) 0);
        mcount--;
    }
    return ECMD_OK;
}
#endif

static struct {
    int nhkf;
    uchar key;
    const char *name;
} const spkeys_binds[] = {
    { NHKF_ESC,              '\033', (char *) 0 }, /* no binding */
    { NHKF_GETDIR_SELF,      '.', "getdir.self" },
    { NHKF_GETDIR_SELF2,     's', "getdir.self2" },
    { NHKF_GETDIR_HELP,      '?', "getdir.help" },
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
        g.Cmd.spkeys[spkeys_binds[i].nhkf] = key;
        return TRUE;
    }
    return FALSE;
}

static const char *
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
    register char *autoc;

    /* break off first autocomplete from the rest; parse the rest */
    if ((autoc = index(autocomplete, ',')) != 0
        || (autoc = index(autocomplete, ':')) != 0) {
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
        g.Cmd.num_pad = FALSE;
        g.Cmd.pcHack_compat = g.Cmd.phone_layout = g.Cmd.swap_yz = FALSE;
        for (i = 0; i < SIZE(spkeys_binds); i++)
            g.Cmd.spkeys[spkeys_binds[i].nhkf] = spkeys_binds[i].key;
        commands_init();
    } else {
        if (backed_dir_cmd) {
            for (dir = 0; dir < N_DIRS; dir++) {
                for (mode = 0; mode < N_MOVEMODES; mode++) {
                    g.Cmd.commands[back_dir_key[dir][mode]] = back_dir_cmd[dir][mode];
                }
            }
        }

        /* basic num_pad */
        flagtemp = iflags.num_pad;
        if (flagtemp != g.Cmd.num_pad) {
            g.Cmd.num_pad = flagtemp;
            ++updated;
        }
        /* swap_yz mode (only applicable for !num_pad); intended for
           QWERTZ keyboard used in Central Europe, particularly Germany */
        flagtemp = (iflags.num_pad_mode & 1) ? !g.Cmd.num_pad : FALSE;
        if (flagtemp != g.Cmd.swap_yz) {
            g.Cmd.swap_yz = flagtemp;
            ++updated;
            /* FIXME? should Cmd.spkeys[] be scanned for y and/or z to swap?
               Cmd.swap_yz has been toggled;
               perform the swap (or reverse previous one) */
            for (i = 0; i < SIZE(ylist); i++) {
                c = ylist[i] & 0xff;
                cmdtmp = g.Cmd.commands[c];                /* tmp = [y] */
                g.Cmd.commands[c] = g.Cmd.commands[c + 1]; /* [y] = [z] */
                g.Cmd.commands[c + 1] = cmdtmp;            /* [z] = tmp */
            }
        }
        /* MSDOS compatibility mode (only applicable for num_pad) */
        flagtemp = (iflags.num_pad_mode & 1) ? g.Cmd.num_pad : FALSE;
        if (flagtemp != g.Cmd.pcHack_compat) {
            g.Cmd.pcHack_compat = flagtemp;
            ++updated;
            /* pcHack_compat has been toggled */
#if 0
            c = M('5') & 0xff;
            cmdtmp = g.Cmd.commands['5'];
            g.Cmd.commands['5'] = g.Cmd.commands[c];
            g.Cmd.commands[c] = cmdtmp;
#endif
            /* FIXME: NHKF_DOINV2 ought to be implemented instead of this */
            c = M('0') & 0xff;
            g.Cmd.commands[c] = g.Cmd.pcHack_compat ? g.Cmd.commands['I'] : 0;
        }
        /* phone keypad layout (only applicable for num_pad) */
        flagtemp = (iflags.num_pad_mode & 2) ? g.Cmd.num_pad : FALSE;
        if (flagtemp != g.Cmd.phone_layout) {
            g.Cmd.phone_layout = flagtemp;
            ++updated;
            /* phone_layout has been toggled */
            for (i = 0; i < 3; i++) {
                c = '1' + i;             /* 1,2,3 <-> 7,8,9 */
                cmdtmp = g.Cmd.commands[c];              /* tmp = [1] */
                g.Cmd.commands[c] = g.Cmd.commands[c + 6]; /* [1] = [7] */
                g.Cmd.commands[c + 6] = cmdtmp;          /* [7] = tmp */
                c = (M('1') & 0xff) + i; /* M-1,M-2,M-3 <-> M-7,M-8,M-9 */
                cmdtmp = g.Cmd.commands[c];              /* tmp = [M-1] */
                g.Cmd.commands[c] = g.Cmd.commands[c + 6]; /* [M-1] = [M-7] */
                g.Cmd.commands[c + 6] = cmdtmp;          /* [M-7] = tmp */
            }
        }
    } /*?initial*/

    /* choose updated movement keys */
    if (updated)
        g.Cmd.serialno++;
    g.Cmd.dirchars = !g.Cmd.num_pad
                       ? (!g.Cmd.swap_yz ? sdir : sdir_swap_yz)
                       : (!g.Cmd.phone_layout ? ndir : ndir_phone_layout);
    g.Cmd.alphadirchars = !g.Cmd.num_pad ? g.Cmd.dirchars : sdir;

    /* back up the commands & keys overwritten by new movement keys */
    for (dir = 0; dir < N_DIRS; dir++) {
        for (mode = MV_WALK; mode < N_MOVEMODES; mode++) {
            uchar di = (uchar) g.Cmd.dirchars[dir];

            if (!g.Cmd.num_pad) {
                if (mode == MV_RUN) di = highc(di);
                else if (mode == MV_RUSH) di = C(di);
            } else {
                if (mode == MV_RUN) di = M(di);
                else if (mode == MV_RUSH) di = M(di);
            }
            back_dir_key[dir][mode] = di;
            back_dir_cmd[dir][mode] = (struct ext_func_tab *) g.Cmd.commands[di];
            g.Cmd.commands[di] = (struct ext_func_tab *) 0;
        }
    }
    backed_dir_cmd = TRUE;

    /* bind the new keys to movement commands */
    for (i = 0; i < N_DIRS; i++) {
        (void) bind_key_fn(g.Cmd.dirchars[i], move_funcs[i][MV_WALK]);
        if (!g.Cmd.num_pad) {
            (void) bind_key_fn(highc(g.Cmd.dirchars[i]), move_funcs[i][MV_RUN]);
            (void) bind_key_fn(C(g.Cmd.dirchars[i]), move_funcs[i][MV_RUSH]);
        } else {
            /* M(number) works when altmeta is on */
            (void) bind_key_fn(M(g.Cmd.dirchars[i]), move_funcs[i][MV_RUN]);
            /* can't bind highc() or C() of numbers. just use the 5 prefix. */
        }
    }
    update_rest_on_space();
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
    const struct ext_func_tab *bound_f = g.Cmd.commands[' '];

    /* when 'rest_on_space' is On, <space> will run the #wait command;
       when it is Off, <space> will use 'unrestonspace' which will either
       be Null and elicit "Unknown command ' '." or have some non-Null
       command bound in player's RC file */
    if (bound_f != 0 && bound_f != &restonspace)
        unrestonspace = bound_f;
    g.Cmd.commands[' '] = flags.rest_on_space ? &restonspace : unrestonspace;
}

/* commands which accept 'm' prefix to request menu operation or other
   alternate behavior; it's also overloaded for move-without-autopickup;
   there is no overlap between the two groups of commands */
static boolean
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

static void
reset_cmd_vars(boolean reset_cmdq)
{
    g.context.run = 0;
    g.context.nopick = g.context.forcefight = FALSE;
    g.context.move = g.context.mv = FALSE;
    g.domove_attempting = 0;
    g.multi = 0;
    iflags.menu_requested = FALSE;
    g.context.travel = g.context.travel1 = 0;
    if (g.travelmap) {
        selection_free(g.travelmap, TRUE);
        g.travelmap = NULL;
    }
    if (reset_cmdq)
        cmdq_clear();
}

void
rhack(char *cmd)
{
    int spkey = NHKF_ESC;
    boolean bad_command, firsttime = (cmd == 0);
    struct _cmd_queue *cmdq = NULL;
    const struct ext_func_tab *cmdq_ec = 0, *prefix_seen = 0;
    boolean was_m_prefix = FALSE;

    reset_cmd_vars(FALSE);
 got_prefix_input:
#ifdef SAFERHANGUP
    if (g.program_state.done_hup)
        end_of_input();
#endif
    if ((cmdq = cmdq_pop()) != 0) {
        /* doing queued commands */
        if (cmdq->typ == CMDQ_KEY) {
            static char commandline[2];

            if (!cmd)
                cmd = commandline;
            cmd[0] = cmdq->key;
            cmd[1] = '\0';
        } else if (cmdq->typ == CMDQ_EXTCMD) {
            cmdq_ec = cmdq->ec_entry;
        }
        free(cmdq);
        if (cmdq_ec)
            goto do_cmdq_extcmd;
    } else if (firsttime) {
        cmd = parse();
        /* parse() pushed a cmd but didn't return any key */
        if (!*cmd && g.command_queue)
            goto got_prefix_input;
    }

    if (*cmd == g.Cmd.spkeys[NHKF_ESC]) {
        reset_cmd_vars(TRUE);
        return;
    }

    /* Special case of *cmd == ' ' handled better below */
    if (!*cmd || *cmd == (char) 0377) {
        nhbell();
        reset_cmd_vars(TRUE);
        return; /* probably we just had an interrupt */
    }

    /* handle most movement commands */
    g.context.travel = g.context.travel1 = 0;

    {
        register const struct ext_func_tab *tlist;
        int res, (*func)(void);

 do_cmdq_extcmd:
        if (cmdq_ec)
            tlist = cmdq_ec;
        else
            tlist = g.Cmd.commands[*cmd & 0xff];

        /* current - use *cmd to directly index cmdlist array */
        if (tlist != 0) {
            if (!can_do_extcmd(tlist)) {
                /* can_do_extcmd() already gave a message */
                reset_cmd_vars(TRUE);
                res = ECMD_OK;
            } else if (prefix_seen && !(tlist->flags & PREFIXCMD)
                       && !(tlist->flags & (was_m_prefix ? CMD_M_PREFIX
                                                         : CMD_gGF_PREFIX))) {
                /* got prefix previously but this command doesn't accept one */
                char pfxidx = cmd_from_func(prefix_seen->ef_funct);
                const char *which = (pfxidx != 0) ? visctrl(pfxidx)
                                    : (prefix_seen->ef_funct == do_reqmenu)
                                      ? "move-no-pickup or request-menu"
                                      : prefix_seen->ef_txt;

                pline("The %s command does not accept '%s' prefix.",
                      tlist->ef_txt, which);
                reset_cmd_vars(TRUE);
                res = ECMD_OK;
                prefix_seen = 0;
                was_m_prefix = FALSE;
            } else {
                /* we discard 'const' because some compilers seem to have
                   trouble with the pointer passed to set_occupation() */
                func = ((struct ext_func_tab *) tlist)->ef_funct;
                if (tlist->f_text && !g.occupation && g.multi)
                    set_occupation(func, tlist->f_text, g.multi);
                g.ext_tlist = NULL;
                res = (*func)(); /* perform the command */
                /* if 'func' is doextcmd(), 'tlist' is for Cmd.commands['#']
                   rather than for the command that doextcmd() just ran;
                   doextcmd() notifies us what that was via ext_tlist;
                   other commands leave it Null */
                if (g.ext_tlist)
                    tlist = g.ext_tlist, g.ext_tlist = NULL;

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
                           && g.domove_attempting) {
                    /* not a movement command, but a move prefix earlier? */
                    ; /* just do nothing */
                } else if (((g.domove_attempting & (DOMOVE_RUSH | DOMOVE_WALK))
                            != 0L)
                           && !g.context.travel && !dxdy_moveok()) {
                    /* trying to move diagonally as a grid bug */
                    You_cant("get there from here...");
                    reset_cmd_vars(TRUE);
                    return;
                } else if ((g.domove_attempting & DOMOVE_WALK) != 0L) {
                    if (g.multi)
                        g.context.mv = TRUE;
                    domove();
                    g.context.forcefight = 0;
                    iflags.menu_requested = FALSE;
                    return;
                } else if ((g.domove_attempting & DOMOVE_RUSH) != 0L) {
                    if (firsttime) {
                        if (!g.multi)
                            g.multi = max(COLNO, ROWNO);
                        u.last_str_turn = 0;
                    }
                    g.context.mv = TRUE;
                    domove();
                    iflags.menu_requested = FALSE;
                    return;
                }
                prefix_seen = 0;
                was_m_prefix = FALSE;
            }
            if ((res & (ECMD_CANCEL | ECMD_FAIL)) != 0) {
                /* command was canceled by user, maybe they declined to
                   pick an object to act on, or command failed to finish */
                reset_cmd_vars(TRUE);
            } else if ((res & ECMD_TIME) != 0) {
                g.context.move = TRUE;
            } else { /* ECMD_OK */
                reset_cmd_vars(FALSE);
            }
            return;
        }
        /* if we reach here, cmd wasn't found in cmdlist[] */
        bad_command = TRUE;
    }

    if (bad_command) {
        char expcmd[20]; /* we expect 'cmd' to point to 1 or 2 chars */
        char c, c1 = cmd[1];

        expcmd[0] = '\0';
        while ((c = *cmd++) != '\0')
            Strcat(expcmd, visctrl(c)); /* add 1..4 chars plus terminator */

        if (!prefix_seen || !help_dir(c1, spkey, "Invalid direction key!"))
            Norep("Unknown command '%s'.", expcmd);
        cmdq_clear();
    }
    /* didn't move */
    g.context.move = FALSE;
    g.multi = 0;
    return;
}

/* convert an x,y pair into a direction code */
int
xytod(schar x, schar y)
{
    register int dd;

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

    if (g.Cmd.commands[(uchar)sym]) {
        int (*fnc)(void) = g.Cmd.commands[(uchar)sym]->ef_funct;

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
    return (boolean) (g.Cmd.commands[(uchar)c]
                      && g.Cmd.commands[(uchar)c]->ef_funct == doredraw);
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
get_adjacent_loc(const char *prompt, const char *emsg,
                 xchar x, xchar y, coord *cc)
{
    xchar new_x, new_y;
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

int
getdir(const char *s)
{
    char dirsym;
    int is_mov;
    struct _cmd_queue *cmdq = cmdq_pop();

    if (cmdq) {
        if (cmdq->typ == CMDQ_DIR) {
            if (!cmdq->dirz) {
                dirsym = g.Cmd.dirchars[xytod(cmdq->dirx, cmdq->diry)];
            } else {
                dirsym = g.Cmd.dirchars[(cmdq->dirz > 0) ? DIR_DOWN : DIR_UP];
            }
        } else {
            cmdq_clear();
            dirsym = '\0';
            impossible("getdir: command queue had no dir?");
        }
        free(cmdq);
        goto got_dirsym;
    }

 retry:
    if (g.in_doagain || *readchar_queue)
        dirsym = readchar();
    else
        dirsym = yn_function((s && *s != '^') ? s : "In what direction?",
                             (char *) 0, '\0');
    /* remove the prompt string so caller won't have to */
    clear_nhwindow(WIN_MESSAGE);

    if (redraw_cmd(dirsym)) { /* ^R */
        docrt();              /* redraw */
        goto retry;
    }
    savech(dirsym);

 got_dirsym:
    if (dirsym == g.Cmd.spkeys[NHKF_GETDIR_SELF]
        || dirsym == g.Cmd.spkeys[NHKF_GETDIR_SELF2]) {
        u.dx = u.dy = u.dz = 0;
    } else if (!(is_mov = movecmd(dirsym, MV_ANY)) && !u.dz) {
        boolean did_help = FALSE, help_requested;

        if (!index(quitchars, dirsym)) {
            help_requested = (dirsym == g.Cmd.spkeys[NHKF_GETDIR_HELP]);
            if (help_requested || iflags.cmdassist) {
                did_help = help_dir((s && *s == '^') ? dirsym : '\0',
                                    NHKF_ESC,
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
    if (!u.dz && (Stunned || (Confusion && !rn2(5))))
        confdir();
    return 1;
}

static void
show_direction_keys(winid win, /* should specify a window which is
                                * using a fixed-width font... */
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
static boolean
help_dir(char sym,
         int spkey, /* NHKF_ code for prefix key, if used, or for ESC */
         const char *msg)
{
    static const char wiz_only_list[] = "EFGIVW";
    char ctrl;
    winid win;
    char buf[BUFSZ], buf2[BUFSZ], *explain;
    const char *dothat, *how;
    boolean prefixhandling, viawindow;

    /* NHKF_ESC indicates that player asked for help at getdir prompt */
    viawindow = (spkey == NHKF_ESC || iflags.cmdassist);
    prefixhandling = (spkey != NHKF_ESC);
    /*
     * Handling for prefix keys that don't want special directions.
     * Delivered via pline if 'cmdassist' is off, or instead of the
     * general message if it's on.
     */
    dothat = "do that";
    how = " at"; /* for "<action> at yourself"; not used for up/down */

    buf[0] = '\0';
    /* for movement prefix followed by '.' or (numpad && 's') to mean 'self';
       note: '-' for hands (inventory form of 'self') is not handled here */
    if (prefixhandling
        && (sym == g.Cmd.spkeys[NHKF_GETDIR_SELF]
            || (g.Cmd.num_pad && sym == g.Cmd.spkeys[NHKF_GETDIR_SELF2]))) {
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
                        visctrl(g.Cmd.spkeys[spkey]));
            pline("%s", buf);
            return TRUE;
        }
        /* when 'cmdassist' is off and caller doesn't insist, do nothing */
        return FALSE;
    }

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
            && (!index(wiz_only_list, sym) || wizard)) {
            Sprintf(buf, "Are you trying to use ^%c%s?", sym,
                    index(wiz_only_list, sym) ? ""
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
            int selfi = g.Cmd.num_pad ? NHKF_GETDIR_SELF2 : NHKF_GETDIR_SELF;

            Sprintf(buf,   "       %4s  direct at yourself",
                    visctrl(g.Cmd.spkeys[selfi]));
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

void
confdir(void)
{
    register int x = NODIAG(u.umonnum) ? dirs_ord[rn2(4)] : rn2(N_DIRS);

    u.dx = xdir[x];
    u.dy = ydir[x];
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
isok(register int x, register int y)
{
    /* x corresponds to curx, so x==1 is the first column. Ach. %% */
    return x >= 1 && x <= COLNO - 1 && y >= 0 && y <= ROWNO - 1;
}

/* #herecmdmenu command */
static int
doherecmdmenu(void)
{
    char ch = here_cmd_menu();

    return (ch && ch != '\033') ? ECMD_TIME : ECMD_OK;
}

/* #therecmdmenu command, a way to test there_cmd_menu without mouse */
static int
dotherecmdmenu(void)
{
    char ch;

    if (!getdir((const char *) 0) || !isok(u.ux + u.dx, u.uy + u.dy))
        return ECMD_CANCEL;

    if (u.dx || u.dy)
        ch = there_cmd_menu(u.ux + u.dx, u.uy + u.dy, CLICK_1);
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

    MCMD_QUAFF,
    MCMD_DIP,
    MCMD_SIT,
    MCMD_UP,
    MCMD_DOWN,
    MCMD_DISMOUNT,
    MCMD_MONABILITY,
    MCMD_PICKUP,
    MCMD_LOOT,
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

static void
mcmd_addmenu(winid win, int act, const char *txt)
{
    anything any;

    /* TODO: fixed letters for the menu entries? */
    any = cg.zeroany;
    any.a_int = act;
    add_menu(win, &nul_glyphinfo, &any, '\0', 0, ATR_NONE, txt, MENU_ITEMFLAGS_NONE);
}

/* command menu entries when targeting self */
static int
there_cmd_menu_self(winid win, int x, int y, int *act UNUSED)
{
    int K = 0;
    char buf[BUFSZ];
    schar typ = levl[x][y].typ;
    stairway *stway = stairway_at(x, y);
    struct trap *ttmp;

    if (!u_at(x,y))
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
        struct obj *otmp = g.level.objects[x][y];

        Sprintf(buf, "Pick up %s", otmp->nexthere ? "items" : doname(otmp));
        mcmd_addmenu(win, MCMD_PICKUP, buf), ++K;

        if (Is_container(otmp)) {
            Sprintf(buf, "Loot %s", doname(otmp));
            mcmd_addmenu(win, MCMD_LOOT, buf), ++K;
        }
        if (otmp->oclass == FOOD_CLASS) {
            Sprintf(buf, "Eat %s", doname(otmp));
            mcmd_addmenu(win, MCMD_EAT, buf), ++K;
        }
    }


    if (g.invent) {
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
static int
there_cmd_menu_next2u(winid win, int x, int y, int mod, int *act)
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
    }
#if 0
    if (mtmp) {
        Sprintf(buf, "%s %s", mon_nam(mtmp),
                !has_mname(mtmp) ? "Name" : "Rename"), ++K;
        /* need a way to pass mtmp or <ux+dx,uy+dy>to an 'int f()' function
           as well as reorganizinging do_mname() to use that function */
        add_herecmd_menuitem(win, XXX(), buf);
    }
#endif

    if (mtmp || glyph_is_invisible(glyph_at(x, y))) {
        Sprintf(buf, "Attack %s", mtmp ? mon_nam(mtmp) : "unseen creature");
        mcmd_addmenu(win, MCMD_ATTACK_NEXT2U, buf), ++K;
        /* attacking overrides any other automatic action */
        *act = MCMD_ATTACK_NEXT2U;
    } else {
        /* "Move %s", direction - handled below */
    }
    return K;
}

static int
there_cmd_menu_far(winid win, xchar x, xchar y, int mod)
{
    int K = 0;

    if (mod != CLICK_2)
        return K;

    if (linedup(u.ux, u.uy, x, y, 1) && (dist2(u.ux, u.uy, x, y) < 18*18)) {
        mcmd_addmenu(win, MCMD_THROW_OBJ, "Throw something"), ++K;
    }

    if (flags.travelcmd) {
        mcmd_addmenu(win, MCMD_TRAVEL, "Travel here"), ++K;
    }

    return K;
}

static int
there_cmd_menu_common(winid win, xchar x UNUSED, xchar y UNUSED, int mod, int *act UNUSED)
{
    int K = 0;

    if (mod == CLICK_2 && iflags.clicklook) {
        mcmd_addmenu(win, MCMD_LOOK_AT, "Look at map symbol"), ++K;
    }

    return K;
}

/* offer choice of actions to perform at adjacent location <x,y> */
static char
there_cmd_menu(int x, int y, int mod)
{
    winid win;
    char ch = '\0';
    int npick = 0, K = 0;
    menu_item *picks = (menu_item *) 0;
    int dx = sgn(x - u.ux), dy = sgn(y - u.uy);
    int dir = xytod(dx, dy);
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
        if (next2u(x, y) && !test_move(u.ux, u.uy, dx, dy, TEST_MOVE))
            cmdq_add_ec(move_funcs[dir][MV_WALK]);
        else if (flags.travelcmd) {
            iflags.travelcc.x = u.tx = x;
            iflags.travelcc.y = u.ty = y;
            cmdq_add_ec(dotravel_target);
        }
        npick = 0;
        ch = '\0';
    } else if ((K == 1) && (act != MCMD_NOTHING)) {
        destroy_nhwindow(win);
        goto act_on_act;
    } else {
        end_menu(win, "What do you want to do?");
        npick = select_menu(win, PICK_ONE, &picks);
        ch = '\033';
    }
    destroy_nhwindow(win);
    if (npick > 0) {
        act = picks->item.a_int;

        free((genericptr_t) picks);

    act_on_act:
        switch (act) {
        case MCMD_TRAVEL:
            iflags.travelcc.x = u.tx = x;
            iflags.travelcc.y = u.ty = y;
            cmdq_add_ec(dotravel_target);
            break;
        case MCMD_THROW_OBJ:
            cmdq_add_ec(dothrow);
            cmdq_add_userinput();
            cmdq_add_dir(dx, dy, 0);
            break;
        case MCMD_OPEN_DOOR:
            cmdq_add_ec(doopen);
            cmdq_add_dir(dx, dy, 0);
            break;
        case MCMD_LOCK_DOOR:
            {
                struct obj *otmp = carrying(SKELETON_KEY);
                if (!otmp) otmp = carrying(LOCK_PICK);
                if (!otmp) otmp = carrying(CREDIT_CARD);
                if (otmp) {
                    cmdq_add_ec(doapply);
                    cmdq_add_key(otmp->invlet);
                    cmdq_add_dir(dx, dy, 0);
                    cmdq_add_key('y'); /* "Lock it?" */
                }
            }
            break;
        case MCMD_UNTRAP_DOOR:
            cmdq_add_ec(dountrap);
            cmdq_add_dir(dx, dy, 0);
            break;
        case MCMD_KICK_DOOR:
            cmdq_add_ec(dokick);
            cmdq_add_dir(dx, dy, 0);
            break;
        case MCMD_CLOSE_DOOR:
            cmdq_add_ec(doclose);
            cmdq_add_dir(dx, dy, 0);
            break;
        case MCMD_SEARCH:
            cmdq_add_ec(dosearch);
            break;
        case MCMD_LOOK_TRAP:
            cmdq_add_ec(doidtrap);
            cmdq_add_dir(dx, dy, 0);
            break;
        case MCMD_UNTRAP_TRAP:
            cmdq_add_ec(dountrap);
            cmdq_add_dir(dx, dy, 0);
            break;
        case MCMD_MOVE_DIR:
            cmdq_add_ec(move_funcs[xytod(dx, dy)][MV_WALK]);
            break;
        case MCMD_RIDE:
            cmdq_add_ec(doride);
            cmdq_add_dir(dx, dy, 0);
            break;
        case MCMD_REMOVE_SADDLE:
            cmdq_add_ec(doloot);
            cmdq_add_dir(dx, dy, 0);
            cmdq_add_key('y'); /* "Do you want to remove the saddle ..." */
            break;
        case MCMD_APPLY_SADDLE:
            {
                struct obj *otmp = carrying(SADDLE);
                if (otmp) {
                    cmdq_add_ec(doapply);
                    cmdq_add_key(otmp->invlet);
                    cmdq_add_dir(dx, dy, 0);
                }
            }
            break;
        case MCMD_ATTACK_NEXT2U:
            cmdq_add_ec(move_funcs[dir][MV_WALK]);
            break;
        case MCMD_TALK:
            cmdq_add_ec(dotalk);
            cmdq_add_dir(dx, dy, 0);
            break;
        case MCMD_QUAFF:
            cmdq_add_ec(dodrink);
            cmdq_add_key('y'); /* "Drink from the fountain?" */
            break;
        case MCMD_DIP:
            cmdq_add_ec(dodip);
            cmdq_add_userinput();
            cmdq_add_key('y'); /* "Dip foo into the fountain?" */
            break;
        case MCMD_SIT:
            cmdq_add_ec(dosit);
            break;
        case MCMD_UP:
            cmdq_add_ec(doup);
            break;
        case MCMD_DOWN:
            cmdq_add_ec(dodown);
            break;
        case MCMD_DISMOUNT:
            cmdq_add_ec(doride);
            break;
        case MCMD_MONABILITY:
            cmdq_add_ec(domonability);
            break;
        case MCMD_PICKUP:
            cmdq_add_ec(dopickup);
            break;
        case MCMD_LOOT:
            cmdq_add_ec(doloot);
            break;
        case MCMD_EAT:
            cmdq_add_ec(doeat);
            cmdq_add_key('y'); /* "There is foo here; eat it?" */
            break;
        case MCMD_DROP:
            cmdq_add_ec(dodrop);
            break;
        case MCMD_INVENTORY:
            cmdq_add_ec(ddoinv);
            break;
        case MCMD_REST:
            cmdq_add_ec(donull);
            break;
        case MCMD_LOOK_HERE:
            cmdq_add_ec(dolook);
            break;
        case MCMD_LOOK_AT:
            g.clicklook_cc.x = x;
            g.clicklook_cc.y = y;
            cmdq_add_ec(doclicklook);
            break;
        case MCMD_UNTRAP_HERE:
            cmdq_add_ec(dountrap);
            cmdq_add_dir(0, 0, 1);
            break;
        case MCMD_OFFER:
            cmdq_add_ec(dosacrifice);
            cmdq_add_userinput();
            break;
        case MCMD_CAST_SPELL:
            cmdq_add_ec(docast);
            break;
        default: break;
        }
        return '\0';
    }
    return ch;
}

static char
here_cmd_menu(void)
{
    there_cmd_menu(u.ux, u.uy, CLICK_1);
    return '\0';
}

/*
 * convert a MAP window position into a movement key usable with movecmd()
 */
const char *
click_to_cmd(int x, int y, int mod)
{
    static char cmd[4];
    struct obj *o;
    int dir;

    cmd[0] = cmd[1] = '\0';
    if (!iflags.herecmd_menu && iflags.clicklook && mod == CLICK_2) {
        g.clicklook_cc.x = x;
        g.clicklook_cc.y = y;
        cmdq_add_ec(doclicklook);
        return cmd;
    }
    if (iflags.herecmd_menu && isok(x, y)) {
        (void) there_cmd_menu(x, y, mod);
        return cmd;
    }

    x -= u.ux;
    y -= u.uy;

    if (flags.travelcmd) {
        if (abs(x) <= 1 && abs(y) <= 1) {
            x = sgn(x), y = sgn(y);
        } else {
            iflags.travelcc.x = u.tx = u.ux + x;
            iflags.travelcc.y = u.ty = u.uy + y;
            cmdq_add_ec(dotravel_target);
            return cmd;
        }

        if (x == 0 && y == 0) {
            /* here */
            if (IS_FOUNTAIN(levl[u.ux][u.uy].typ)
                || IS_SINK(levl[u.ux][u.uy].typ)) {
                cmd[0] = cmd_from_func(mod == CLICK_1 ? dodrink : dodip);
                return cmd;
            } else if (IS_THRONE(levl[u.ux][u.uy].typ)) {
                cmd[0] = cmd_from_func(dosit);
                return cmd;
            } else if (On_stairs_up(u.ux, u.uy)) {
                cmd[0] = cmd_from_func(doup);
                return cmd;
            } else if (On_stairs_dn(u.ux, u.uy)) {
                cmd[0] = cmd_from_func(dodown);
                return cmd;
            } else if ((o = vobj_at(u.ux, u.uy)) != 0) {
                cmd[0] = cmd_from_func(Is_container(o) ? doloot : dopickup);
                return cmd;
            } else {
                cmd[0] = cmd_from_func(donull); /* just rest */
                return cmd;
            }
        }

        /* directional commands */

        dir = xytod(x, y);
        if (!m_at(u.ux + x, u.uy + y)
            && !test_move(u.ux, u.uy, x, y, TEST_MOVE)) {
            cmd[1] = cmd_from_func(move_funcs[dir][MV_WALK]);
            cmd[2] = '\0';

            if (IS_DOOR(levl[u.ux + x][u.uy + y].typ)) {
                /* slight assistance to player: choose kick/open for them */
                if (levl[u.ux + x][u.uy + y].doormask & D_LOCKED) {
                    cmd[0] = cmd_from_func(dokick);
                    return cmd;
                }
                if (levl[u.ux + x][u.uy + y].doormask & D_CLOSED) {
                    cmd[0] = cmd_from_func(doopen);
                    return cmd;
                }
            }
            if (levl[u.ux + x][u.uy + y].typ <= SCORR) {
                cmd[0] = cmd_from_func(dosearch);
                cmd[1] = 0;
                return cmd;
            }
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
            cmd[0] = cmd_from_func(donull);
            return cmd;
        }
        dir = xytod(x, y);
    }

    /* move, attack, etc. */
    cmd[1] = 0;
    if (mod == CLICK_1) {
        cmd[0] = cmd_from_func(move_funcs[dir][MV_WALK]);
    } else {
        cmd[0] = cmd_from_func(move_funcs[dir][MV_RUN]);
    }

    return cmd;
}

/* gather typed digits into a number in *count; return the next non-digit */
char
get_count(
    char *allowchars,
    char inkey,
    long maxcount,
    cmdcount_nht *count,
    boolean historicmsg) /* whether to include in ^P history: True => yes */
{
    char qbuf[QBUFSZ];
    int key;
    long cnt = 0L;
    boolean backspaced = FALSE;
    /* this should be done in port code so that we have erase_char
       and kill_char available; we can at least fake erase_char */
#define STANDBY_erase_char '\177'

    for (;;) {
        if (inkey) {
            key = inkey;
            inkey = '\0';
        } else
            key = readchar();

        if (digit(key)) {
            cnt = 10L * cnt + (long) (key - '0');
            if (cnt < 0L)
                cnt = 0L;
            else if (maxcount > 0L && cnt > maxcount)
                cnt = maxcount;
        } else if (cnt && (key == '\b' || key == STANDBY_erase_char)) {
            cnt = cnt / 10L;
            backspaced = TRUE;
        } else if (key == g.Cmd.spkeys[NHKF_ESC]) {
            break;
        } else if (!allowchars || index(allowchars, key)) {
            *count = cnt;
            break;
        }

        if (cnt > 9 || backspaced) {
            clear_nhwindow(WIN_MESSAGE);
            if (backspaced && !cnt) {
                Sprintf(qbuf, "Count: ");
            } else {
                Sprintf(qbuf, "Count: %ld", cnt);
                backspaced = FALSE;
            }
            custompline(SUPPRESS_HISTORY, "%s", qbuf);
            mark_synch();
        }
    }

    if (historicmsg) {
        Sprintf(qbuf, "Count: %ld ", *count);
        (void) key2txt((uchar) key, eos(qbuf));
        putmsghistory(qbuf, FALSE);
    }

    return key;
}


static char *
parse(void)
{
    register int foo;

    iflags.in_parse = TRUE;
    g.command_count = 0;
    g.context.move = TRUE; /* assume next command will take game time */
    flush_screen(1); /* Flush screen buffer. Put the cursor on the hero. */

    g.program_state.getting_a_command = 1; /* affects readchar() behavior for
                                            * ESC iff 'altmeta' option is On;
                                            * reset to 0 by readchar() */
    if (!g.Cmd.num_pad || (foo = readchar()) == g.Cmd.spkeys[NHKF_COUNT]) {
        foo = get_count((char *) 0, '\0', LARGEST_INT,
                        &g.command_count, FALSE);
        g.last_command_count = g.command_count;
    }

    if (foo == g.Cmd.spkeys[NHKF_ESC]) { /* esc cancels count (TH) */
        clear_nhwindow(WIN_MESSAGE);
        g.command_count = 0;
        g.last_command_count = 0;
    } else if (g.in_doagain) {
        g.command_count = g.last_command_count;
    } else if (foo && foo == cmd_from_func(do_repeat)) {
        // g.command_count will be set again when we
        // re-enter with g.in_doagain set true
        g.command_count = g.last_command_count;
    } else {
        g.last_command_count = g.command_count;
        savech(0); /* reset input queue */
        savech((char) foo);
    }

    g.multi = g.command_count;
    if (g.multi)
        g.multi--;

    g.command_line[0] = foo;
    g.command_line[1] = '\0';
    clear_nhwindow(WIN_MESSAGE);

    iflags.in_parse = FALSE;
    return g.command_line;
}

#ifdef HANGUPHANDLING
/* some very old systems, or descendents of such systems, expect signal
   handlers to have return type `int', but they don't actually inspect
   the return value so we should be safe using `void' unconditionally */
/*ARGUSED*/
void
hangup(int sig_unused UNUSED)   /* called as signal() handler, so sent
                                   at least one arg */
{
    if (g.program_state.exiting)
        g.program_state.in_moveloop = 0;
    nhwindows_hangup();
#ifdef SAFERHANGUP
    /* When using SAFERHANGUP, the done_hup flag it tested in rhack
       and a couple of other places; actual hangup handling occurs then.
       This is 'safer' because it disallows certain cheats and also
       protects against losing objects in the process of being thrown,
       but also potentially riskier because the disconnected program
       must continue running longer before attempting a hangup save. */
    g.program_state.done_hup++;
    /* defer hangup iff game appears to be in progress */
    if (g.program_state.in_moveloop && g.program_state.something_worth_saving)
        return;
#endif /* SAFERHANGUP */
    end_of_input();
}

void
end_of_input(void)
{
#ifdef NOSAVEONHANGUP
#ifdef INSURANCE
    if (flags.ins_chkpt && g.program_state.something_worth_saving)
        program_state.preserve_locks = 1; /* keep files for recovery */
#endif
    g.program_state.something_worth_saving = 0; /* don't save */
#endif

#ifndef SAFERHANGUP
    if (!g.program_state.done_hup++)
#endif
        if (g.program_state.something_worth_saving)
            (void) dosave0();
    if (iflags.window_inited)
        exit_nhwindows((char *) 0);
    clearlocks();
    nh_terminate(EXIT_SUCCESS);
    /*NOTREACHED*/ /* not necessarily true for vms... */
    return;
}
#endif /* HANGUPHANDLING */

static char
readchar_core(int *x, int *y, int *mod)
{
    register int sym;

    if (iflags.debug_fuzzer)
        return randomkey();
    if (*readchar_queue)
        sym = *readchar_queue++;
    else
        sym = g.in_doagain ? pgetchar() : nh_poskey(x, y, mod);

#ifdef NR_OF_EOFS
    if (sym == EOF) {
        register int cnt = NR_OF_EOFS;
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
               && g.program_state.getting_a_command) {
        /* iflags.altmeta: treat two character ``ESC c'' as single `M-c' but
           only when we're called by parse() [possibly via get_count()] */
        sym = *readchar_queue ? *readchar_queue++ : pgetchar();
        if (sym == EOF || sym == 0)
            sym = '\033';
        else if (sym != '\033')
            sym |= 0200; /* force 8th bit on */
#endif /*ALTMETA*/
    } else if (sym == 0) {
        /* click event */
        readchar_queue = click_to_cmd(*x, *y, *mod);
        sym = *readchar_queue++;
    }
    g.program_state.getting_a_command = 0; /* next readchar() will be for an
                                            * ordinary char unless parse()
                                            * sets this back to 1 */
    return (char) sym;
}

char
readchar(void)
{
    char ch;
    int x = u.ux, y = u.uy, mod = 0;

    ch = readchar_core(&x, &y, &mod);
    return ch;
}

char
readchar_poskey(int *x, int *y, int *mod)
{
    char ch;

    ch = readchar_core(x, y, mod);
    return ch;
}

/* '_' command, #travel, via keyboard rather than mouse click */
static int
dotravel(void)
{
    coord cc;

    /*
     * Travelling used to be a no-op if user toggled 'travel' option
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
        int gf = iflags.getloc_filter;
        iflags.getloc_filter = GFILTER_VIEW;
        if (!getpos_menu(&cc, GLOC_INTERESTING)) {
            iflags.getloc_filter = gf;
            iflags.getloc_travelmode = FALSE;
            return ECMD_OK;
        }
        iflags.getloc_filter = gf;
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
static int
dotravel_target(void)
{
    if (!isok(iflags.travelcc.x, iflags.travelcc.y))
        return ECMD_OK;

    iflags.getloc_travelmode = FALSE;

    g.context.travel = 1;
    g.context.travel1 = 1;
    g.context.run = 8;
    g.context.nopick = 1;
    g.domove_attempting |= DOMOVE_RUSH;

    if (!g.multi)
        g.multi = max(COLNO, ROWNO);
    u.last_str_turn = 0;
    g.context.mv = TRUE;

    domove();
    return ECMD_TIME;
}

/* mouse click look command */
static int
doclicklook(void)
{
    if (!isok(g.clicklook_cc.x, g.clicklook_cc.y))
        return 0;

    g.context.move = FALSE;
    (void) do_look(2, &g.clicklook_cc);

    return ECMD_OK;
}

/*
 *   Parameter validator for generic yes/no function to prevent
 *   the core from sending too long a prompt string to the
 *   window port causing a buffer overflow there.
 */
char
yn_function(const char *query, const char *resp, char def)
{
    char res, qbuf[QBUFSZ];
    struct _cmd_queue *cmdq = cmdq_pop();
#ifdef DUMPLOG
    unsigned idx = g.saved_pline_index;
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
    if (cmdq && cmdq->typ == CMDQ_KEY) {
        res = cmdq->key;
    } else {
        res = (*windowprocs.win_yn_function)(query, resp, def);
    }
    free(cmdq);
#ifdef DUMPLOG
    if (idx == g.saved_pline_index) {
        /* when idx is still the same as g.saved_pline_index, the interface
           didn't put the prompt into g.saved_plines[]; we put a simplified
           version in there now (without response choices or default) */
        Sprintf(dumplog_buf, "%s ", query);
        (void) key2txt((uchar) res, eos(dumplog_buf));
        dumplogmsg(dumplog_buf);
    }
#endif
    return res;
}

/* for paranoid_confirm:quit,die,attack prompting */
boolean
paranoid_query(boolean be_paranoid, const char *prompt)
{
    boolean confirmed_ok;

    /* when paranoid, player must respond with "yes" rather than just 'y'
       to give the go-ahead for this query; default is "no" unless the
       ParanoidConfirm flag is set in which case there's no default */
    if (be_paranoid) {
        char pbuf[BUFSZ], qbuf[QBUFSZ], ans[BUFSZ];
        const char *promptprefix = "",
                *responsetype = ParanoidConfirm ? "(yes|no)" : "(yes) [no]";
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

            Snprintf(qbuf, sizeof(qbuf), "%s%s %s", promptprefix, pbuf,
                     responsetype);
            *ans = '\0';
            getlin(qbuf, ans);
            (void) mungspaces(ans);
            confirmed_ok = !strcmpi(ans, "yes");
            if (confirmed_ok || *ans == '\033')
                break;
            promptprefix = "\"Yes\" or \"No\": ";
        } while (ParanoidConfirm && strcmpi(ans, "no") && --trylimit);
    } else
        confirmed_ok = (yn(prompt) == 'y');

    return confirmed_ok;
}

/* ^Z command, #suspend */
static int
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
static int
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

/*cmd.c*/

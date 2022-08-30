/* NetHack 3.7	topl.c	$NHDT-Date: 1596498344 2020/08/03 23:45:44 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.72 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef TTY_GRAPHICS

#include "tcap.h"
#include "wintty.h"

static void redotoplin(const char *);
static void topl_putsym(char);
static void removetopl(int);
static void msghistory_snapshot(boolean);
static void free_msghistory_snapshot(boolean);

int
tty_doprev_message(void)
{
    register struct WinDesc *cw = wins[WIN_MESSAGE];
    winid prevmsg_win;
    int i;

    if ((iflags.prevmsg_window != 's')
        && !ttyDisplay->inread) {           /* not single */
        if (iflags.prevmsg_window == 'f') { /* full */
            prevmsg_win = create_nhwindow(NHW_MENU);
            putstr(prevmsg_win, 0, "Message History");
            putstr(prevmsg_win, 0, "");
            cw->maxcol = cw->maxrow;
            i = cw->maxcol;
            do {
                if (cw->data[i] && strcmp(cw->data[i], ""))
                    putstr(prevmsg_win, 0, cw->data[i]);
                i = (i + 1) % cw->rows;
            } while (i != cw->maxcol);
            putstr(prevmsg_win, 0, g.toplines);
            display_nhwindow(prevmsg_win, TRUE);
            destroy_nhwindow(prevmsg_win);
        } else if (iflags.prevmsg_window == 'c') { /* combination */
            do {
                morc = 0;
                if (cw->maxcol == cw->maxrow) {
                    ttyDisplay->dismiss_more = C('p'); /* ^P ok at --More-- */
                    redotoplin(g.toplines);
                    cw->maxcol--;
                    if (cw->maxcol < 0)
                        cw->maxcol = cw->rows - 1;
                    if (!cw->data[cw->maxcol])
                        cw->maxcol = cw->maxrow;
                } else if (cw->maxcol == (cw->maxrow - 1)) {
                    ttyDisplay->dismiss_more = C('p'); /* ^P ok at --More-- */
                    redotoplin(cw->data[cw->maxcol]);
                    cw->maxcol--;
                    if (cw->maxcol < 0)
                        cw->maxcol = cw->rows - 1;
                    if (!cw->data[cw->maxcol])
                        cw->maxcol = cw->maxrow;
                } else {
                    prevmsg_win = create_nhwindow(NHW_MENU);
                    putstr(prevmsg_win, 0, "Message History");
                    putstr(prevmsg_win, 0, "");
                    cw->maxcol = cw->maxrow;
                    i = cw->maxcol;
                    do {
                        if (cw->data[i] && strcmp(cw->data[i], ""))
                            putstr(prevmsg_win, 0, cw->data[i]);
                        i = (i + 1) % cw->rows;
                    } while (i != cw->maxcol);
                    putstr(prevmsg_win, 0, g.toplines);
                    display_nhwindow(prevmsg_win, TRUE);
                    destroy_nhwindow(prevmsg_win);
                }

            } while (morc == C('p'));
            ttyDisplay->dismiss_more = 0;
        } else { /* reversed */
            morc = 0;
            prevmsg_win = create_nhwindow(NHW_MENU);
            putstr(prevmsg_win, 0, "Message History");
            putstr(prevmsg_win, 0, "");
            putstr(prevmsg_win, 0, g.toplines);
            cw->maxcol = cw->maxrow - 1;
            if (cw->maxcol < 0)
                cw->maxcol = cw->rows - 1;
            do {
                putstr(prevmsg_win, 0, cw->data[cw->maxcol]);
                cw->maxcol--;
                if (cw->maxcol < 0)
                    cw->maxcol = cw->rows - 1;
                if (!cw->data[cw->maxcol])
                    cw->maxcol = cw->maxrow;
            } while (cw->maxcol != cw->maxrow);

            display_nhwindow(prevmsg_win, TRUE);
            destroy_nhwindow(prevmsg_win);
            cw->maxcol = cw->maxrow;
            ttyDisplay->dismiss_more = 0;
        }
    } else if (iflags.prevmsg_window == 's') { /* single */
        ttyDisplay->dismiss_more = C('p'); /* <ctrl/P> allowed at --More-- */
        do {
            morc = 0;
            if (cw->maxcol == cw->maxrow)
                redotoplin(g.toplines);
            else if (cw->data[cw->maxcol])
                redotoplin(cw->data[cw->maxcol]);
            cw->maxcol--;
            if (cw->maxcol < 0)
                cw->maxcol = cw->rows - 1;
            if (!cw->data[cw->maxcol])
                cw->maxcol = cw->maxrow;
        } while (morc == C('p'));
        ttyDisplay->dismiss_more = 0;
    }
    return 0;
}

static void
redotoplin(const char *str)
{
    int otoplin = ttyDisplay->toplin;

    home();
    if (!ttyDisplay->topl_utf8) {
        if (*str & 0x80) {
            /* kludge for the / command, the only time we ever want a */
            /* graphics character on the top line */
            g_putch((int) *str++);
            ttyDisplay->curx++;
        }
        end_glyphout(); /* in case message printed during graphics output */
    }
    putsyms(str);
    cl_end();
    ttyDisplay->toplin = TOPLINE_NEED_MORE;
    if (ttyDisplay->cury && otoplin != TOPLINE_SPECIAL_PROMPT)
        more();
}

/* for use by tty_putstr() */
void
show_topl(const char *str)
{
    struct WinDesc *cw = wins[WIN_MESSAGE];

    /* show if either STOP isn't set or current message specifies NOSTOP */
    if ((cw->flags & (WIN_STOP | WIN_NOSTOP)) != WIN_STOP) {
        /* NOSTOP cancels persistent STOP and is a one-shot operation;
           force both to be cleared (no-op for either bit that isn't set) */
        cw->flags &= ~(WIN_STOP | WIN_NOSTOP);

        if (ttyDisplay->cury && ttyDisplay->toplin == TOPLINE_NON_EMPTY)
            tty_clear_nhwindow(WIN_MESSAGE);

        cw->curx = cw->cury = 0;
        home();
        cl_end();
        addtopl(str);

        if (ttyDisplay->cury && ttyDisplay->toplin != TOPLINE_SPECIAL_PROMPT)
            ttyDisplay->toplin = TOPLINE_NON_EMPTY;
    }
}

/* used by update_topl(); also by tty_putstr() */
void
remember_topl(void)
{
    register struct WinDesc *cw = wins[WIN_MESSAGE];
    int idx = cw->maxrow;
    unsigned len = strlen(g.toplines) + 1;

    if ((cw->flags & WIN_LOCKHISTORY) || !*g.toplines)
        return;

    if (len > (unsigned) cw->datlen[idx]) {
        if (cw->data[idx])
            free(cw->data[idx]);
        len += (8 - (len & 7)); /* pad up to next multiple of 8 */
        cw->data[idx] = (char *) alloc(len);
        cw->datlen[idx] = (short) len;
    }
    Strcpy(cw->data[idx], g.toplines);
    if (!g.program_state.in_checkpoint) {
        *g.toplines = '\0';
        cw->maxcol = cw->maxrow = (idx + 1) % cw->rows;
    }
}

void
addtopl(const char *s)
{
    struct WinDesc *cw = wins[WIN_MESSAGE];

    tty_curs(BASE_WINDOW, cw->curx + 1, cw->cury);
    putsyms(s);
    cl_end();
    ttyDisplay->toplin = TOPLINE_NEED_MORE;
}

void
more(void)
{
    struct WinDesc *cw = wins[WIN_MESSAGE];

    if (iflags.debug_fuzzer)
        return;

    /* avoid recursion -- only happens from interrupts */
    if (ttyDisplay->inmore)
        return;

    ttyDisplay->inmore++;

    if (ttyDisplay->toplin) {
        tty_curs(BASE_WINDOW, cw->curx + 1, cw->cury);
        if (cw->curx >= CO - 8)
            topl_putsym('\n');
    }

    if (flags.standout)
        standoutbeg();
    putsyms(defmorestr);
    if (flags.standout)
        standoutend();

    xwaitforspace("\033 ");

    if (morc == '\033') {
        if (!(cw->flags & WIN_NOSTOP))
            cw->flags |= WIN_STOP;
    }

    if (ttyDisplay->toplin && cw->cury) {
        docorner(1, cw->cury + 1, 0);
        cw->curx = cw->cury = 0;
        home();
    } else if (morc == '\033') {
        cw->curx = cw->cury = 0;
        home();
        cl_end();
    }
    ttyDisplay->toplin = TOPLINE_EMPTY;
    ttyDisplay->inmore = 0;
}

void
update_topl(register const char *bp)
{
    register char *tl, *otl;
    register int n0;
    int notdied = 1;
    struct WinDesc *cw = wins[WIN_MESSAGE];
    boolean skip = (cw->flags & (WIN_STOP | WIN_NOSTOP)) == WIN_STOP;

    /* If there is room on the line, print message on same line */
    /* But messages like "You die..." deserve their own line */
    n0 = strlen(bp);
    if ((ttyDisplay->toplin == TOPLINE_NEED_MORE || skip)
        && cw->cury == 0
        && n0 + (int) strlen(g.toplines) + 3 < CO - 8 /* room for --More-- */
        && (notdied = strncmp(bp, "You die", 7)) != 0) {
        Strcat(g.toplines, "  ");
        Strcat(g.toplines, bp);
        cw->curx += 2;
        if (!skip)
            addtopl(bp);
        return;
    } else if (!skip) {
        if (ttyDisplay->toplin == TOPLINE_NEED_MORE) {
            more();
        } else if (cw->cury) { /* for toplin==TOPLINE_NON_EMPTY && cury > 1 */
            docorner(1, cw->cury + 1, 0); /* reset cury = 0 if redraw screen */
            cw->curx = cw->cury = 0;   /* from home--cls() & docorner(1,n,0) */
        }
    }
    remember_topl();
    (void) strncpy(g.toplines, bp, TBUFSZ);
    g.toplines[TBUFSZ - 1] = 0;

    for (tl = g.toplines; n0 >= CO; ) {
        otl = tl;
        for (tl += CO - 1; tl != otl; --tl)
            if (*tl == ' ')
                break;
        if (tl == otl) {
            /* Eek!  A huge token.  Try splitting after it. */
            tl = index(otl, ' ');
            if (!tl)
                break; /* No choice but to spit it out whole. */
        }
        *tl++ = '\n';
        n0 = strlen(tl);
    }
    if (!notdied) /* double negative => "You die"; avoid suppressing mesg */
        cw->flags &= ~WIN_STOP, skip = FALSE;
    if (!skip)
        redotoplin(g.toplines);
}

static void
topl_putsym(char c)
{
    register struct WinDesc *cw = wins[WIN_MESSAGE];

    if (cw == (struct WinDesc *) 0)
        panic("Putsym window MESSAGE nonexistent");

    switch (c) {
    case '\b':
        if (ttyDisplay->curx == 0 && ttyDisplay->cury > 0)
            tty_curs(BASE_WINDOW, CO, (int) ttyDisplay->cury - 1);
        backsp();
        nhassert(ttyDisplay->curx > 0);
        ttyDisplay->curx--;
        cw->curx = ttyDisplay->curx;
        return;
    case '\n':
        cl_end();
        ttyDisplay->curx = 0;
        ttyDisplay->cury++;
        cw->cury = ttyDisplay->cury;
#ifdef WIN32CON
        (void) putchar(c);
#endif
        break;
    default:
        if (ttyDisplay->curx == CO - 1)
            topl_putsym('\n'); /* 1 <= curx < CO; avoid CO */
#ifdef WIN32CON
        (void) putchar(c);
#endif
        ttyDisplay->curx++;
    }
    cw->curx = ttyDisplay->curx;
    if (cw->curx == 0)
        cl_end();
#ifndef WIN32CON
    (void) putchar(c);
#endif
}

void
putsyms(const char *str)
{
    while (*str)
        topl_putsym(*str++);
}

static void
removetopl(register int n)
{
    /* assume addtopl() has been done, so ttyDisplay->toplin is already set */
    while (n-- > 0)
        putsyms("\b \b");
}

extern char erase_char; /* from xxxtty.c; don't need kill_char */

/* returns a single keystroke; also sets 'yn_number' */
char
tty_yn_function(
    const char *query,
    const char *resp,
    char def)
{
    /*
     * Generic yes/no function.  'def' is the default (returned by space
     * or return; 'esc' returns 'q', or 'n', or the default, depending on
     * what's in the expected-response string.  The 'query' string is
     * printed before the user is asked about the string.
     *
     * If resp is NULL, any single character is accepted and returned.
     * If not-NULL, only characters in it are allowed (exceptions:  the
     * quitchars are always allowed, and if it contains '#' then digits
     * are allowed).  If it includes an <esc>, anything beyond that won't
     * be shown in the prompt to the user but will be acceptable as input.
     */
    register char q;
    char rtmp[40];
    boolean digit_ok, allow_num, preserve_case = FALSE;
    struct WinDesc *cw = wins[WIN_MESSAGE];
    boolean doprev = 0;
    char prompt[BUFSZ];

    yn_number = 0L;
    if (ttyDisplay->toplin == TOPLINE_NEED_MORE
        && (cw->flags & (WIN_STOP | WIN_NOSTOP)) != WIN_STOP)
        more();
    cw->flags &= ~(WIN_STOP | WIN_NOSTOP);
    ttyDisplay->toplin = TOPLINE_SPECIAL_PROMPT;
    ttyDisplay->inread++;
    if (resp) {
        char *rb, respbuf[QBUFSZ];

        allow_num = (index(resp, '#') != 0);
        Strcpy(respbuf, resp);
        /* normally we force lowercase, but if any uppercase letters
           are present in the allowed response, preserve case;
           check this before stripping the hidden choices */
        for (rb = respbuf; *rb; ++rb)
            if ('A' <= *rb && *rb <= 'Z') {
                preserve_case = TRUE;
                break;
            }
        /* any acceptable responses that follow <esc> aren't displayed */
        if ((rb = index(respbuf, '\033')) != 0)
            *rb = '\0';
        (void) strncpy(prompt, query, QBUFSZ - 1);
        prompt[QBUFSZ - 1] = '\0';
        Sprintf(eos(prompt), " [%s]", respbuf);
        if (def)
            Sprintf(eos(prompt), " (%c)", def);
        /* not pline("%s ", prompt);
           trailing space is wanted here in case of reprompt */
        Strcat(prompt, " ");
        custompline(OVERRIDE_MSGTYPE | SUPPRESS_HISTORY, "%s", prompt);
    } else {
        /* no restriction on allowed response, so always preserve case */
        /* preserve_case = TRUE; -- moot since we're jumping to the end */
        Sprintf(prompt, "%s ", query);
        custompline(OVERRIDE_MSGTYPE | SUPPRESS_HISTORY, "%s", prompt);
        q = readchar();
        goto clean_up;
    }

    do { /* loop until we get valid input */
        q = readchar();
        if (!preserve_case)
            q = lowc(q);
        if (q == '\020') { /* ctrl-P */
            if (iflags.prevmsg_window != 's') {
                int sav = ttyDisplay->inread;
                ttyDisplay->inread = 0;
                (void) tty_doprev_message();
                ttyDisplay->inread = sav;
                tty_clear_nhwindow(WIN_MESSAGE);
                cw->maxcol = cw->maxrow;
                addtopl(prompt);
            } else {
                if (!doprev)
                    (void) tty_doprev_message(); /* need two initially */
                (void) tty_doprev_message();
                doprev = 1;
            }
            q = '\0'; /* force another loop iteration */
            continue;
        } else if (doprev) {
            /* BUG[?]: this probably ought to check whether the
               character which has just been read is an acceptable
               response; if so, skip the reprompt and use it. */
            tty_clear_nhwindow(WIN_MESSAGE);
            cw->maxcol = cw->maxrow;
            doprev = 0;
            addtopl(prompt);
            q = '\0'; /* force another loop iteration */
            continue;
        }
        digit_ok = allow_num && digit(q);
        if (q == '\033') {
            if (index(resp, 'q'))
                q = 'q';
            else if (index(resp, 'n'))
                q = 'n';
            else
                q = def;
            break;
        } else if (index(quitchars, q)) {
            q = def;
            break;
        }
        if (!index(resp, q) && !digit_ok) {
            tty_nhbell();
            q = (char) 0;
        } else if (q == '#' || digit_ok) {
            char z, digit_string[2];
            int n_len = 0;
            long value = 0;

            addtopl("#"), n_len++;
            digit_string[1] = '\0';
            if (q != '#') {
                digit_string[0] = q;
                addtopl(digit_string), n_len++;
                value = q - '0';
                q = '#';
            }
            do { /* loop until we get a non-digit */
                z = readchar();
                if (!preserve_case)
                    z = lowc(z);
                if (digit(z)) {
                    value = (10 * value) + (z - '0');
                    if (value < 0)
                        break; /* overflow: try again */
                    digit_string[0] = z;
                    addtopl(digit_string), n_len++;
                } else if (z == 'y' || index(quitchars, z)) {
                    if (z == '\033')
                        value = -1; /* abort */
                    z = '\n';       /* break */
                } else if (z == erase_char || z == '\b') {
                    if (n_len <= 1) {
                        value = -1;
                        break;
                    } else {
                        value /= 10;
                        removetopl(1), n_len--;
                    }
                } else {
                    value = -1; /* abort */
                    tty_nhbell();
                    break;
                }
            } while (z != '\n');
            if (value > 0)
                yn_number = value;
            else if (value == 0)
                q = 'n'; /* 0 => "no" */
            else {       /* remove number from top line, then try again */
                removetopl(n_len), n_len = 0;
                q = '\0';
            }
        }
    } while (!q);

 clean_up:
    if (yn_number)
        Sprintf(rtmp, "#%ld", yn_number);
    else
        (void) key2txt(q, rtmp);
    /* addtopl(rtmp); -- rewrite g.toplines instead */
    Sprintf(g.toplines, "%s%s", prompt, rtmp);
#ifdef DUMPLOG
    dumplogmsg(g.toplines);
#endif
    ttyDisplay->inread--;
    ttyDisplay->toplin = TOPLINE_NON_EMPTY;
    if (ttyDisplay->intr)
        ttyDisplay->intr--;
    if (wins[WIN_MESSAGE]->cury)
        tty_clear_nhwindow(WIN_MESSAGE);

    return q;
}

/* shared by tty_getmsghistory() and tty_putmsghistory() */
static char **snapshot_mesgs = 0;

/* collect currently available message history data into a sequential array;
   optionally, purge that data from the active circular buffer set as we go */
static void
msghistory_snapshot(
    boolean purge) /* clear message history buffer as we copy it */
{
    char *mesg;
    int i, inidx, outidx;
    struct WinDesc *cw;

    /* paranoia (too early or too late panic save attempt?) */
    if (WIN_MESSAGE == WIN_ERR || !wins[WIN_MESSAGE])
        return;
    cw = wins[WIN_MESSAGE];

    /* flush g.toplines[], moving most recent message to history */
    remember_topl();

    /* for a passive snapshot, we just copy pointers, so can't allow further
       history updating to take place because that could clobber them */
    if (!purge)
        cw->flags |= WIN_LOCKHISTORY;

    snapshot_mesgs = (char **) alloc((cw->rows + 1) * sizeof(char *));
    outidx = 0;
    inidx = cw->maxrow;
    for (i = 0; i < cw->rows; ++i) {
        snapshot_mesgs[i] = (char *) 0;
        mesg = cw->data[inidx];
        if (mesg && *mesg) {
            snapshot_mesgs[outidx++] = mesg;
            if (purge) {
                /* we're taking this pointer away; subsequest history
                   updates will eventually allocate a new one to replace it */
                cw->data[inidx] = (char *) 0;
                cw->datlen[inidx] = 0;
            }
        }
        inidx = (inidx + 1) % cw->rows;
    }
    snapshot_mesgs[cw->rows] = (char *) 0; /* sentinel */

    /* for a destructive snapshot, history is now completely empty */
    if (purge)
        cw->maxcol = cw->maxrow = 0;
}

/* release memory allocated to message history snapshot */
static void
free_msghistory_snapshot(
    boolean purged) /* True: took history's pointers,
                     * False: just cloned them */
{
    if (snapshot_mesgs) {
        /* snapshot pointers are no longer in use */
        if (purged) {
            int i;

            for (i = 0; snapshot_mesgs[i]; ++i)
                free((genericptr_t) snapshot_mesgs[i]);
        }

        free((genericptr_t) snapshot_mesgs), snapshot_mesgs = (char **) 0;

        /* history can resume being updated at will now... */
        if (!purged)
            wins[WIN_MESSAGE]->flags &= ~WIN_LOCKHISTORY;
    }
}

/*
 * This is called by the core save routines.
 * Each time we are called, we return one string from the
 * message history starting with the oldest message first.
 * When none are left, we return a final null string.
 *
 * History is collected at the time of the first call.
 * Any new messages issued after that point will not be
 * included among the output of the subsequent calls.
 */
char *
tty_getmsghistory(boolean init)
{
    static int nxtidx;
    char *nextmesg;
    char *result = 0;

    if (init) {
        msghistory_snapshot(FALSE);
        nxtidx = 0;
    }

    if (snapshot_mesgs) {
        nextmesg = snapshot_mesgs[nxtidx++];
        if (nextmesg) {
            result = (char *) nextmesg;
        } else {
            free_msghistory_snapshot(FALSE);
        }
    }
    return result;
}

/*
 * This is called by the core savefile restore routines.
 * Each time we are called, we stuff the string into our message
 * history recall buffer. The core will send the oldest message
 * first (actually it sends them in the order they exist in the
 * save file, but that is supposed to be the oldest first).
 * These messages get pushed behind any which have been issued
 * since this session with the program has been started, since
 * they come from a previous session and logically precede
 * anything (like "Restoring save file...") that's happened now.
 *
 * Called with a null pointer to finish up restoration.
 *
 * It's also called by the quest pager code when a block message
 * has a one-line summary specified.  We put that line directly
 * into message history for ^P recall without having displayed it.
 */
void
tty_putmsghistory(const char *msg, boolean restoring_msghist)
{
    static boolean initd = FALSE;
    int idx;

    if (restoring_msghist && !initd) {
        /* we're restoring history from the previous session, but new
           messages have already been issued this session ("Restoring...",
           for instance); collect current history (ie, those new messages),
           and also clear it out so that nothing will be present when the
           restored ones are being put into place */
        msghistory_snapshot(TRUE);
        initd = TRUE;
#ifdef DUMPLOG
        /* this suffices; there's no need to scrub saved_pline[] pointers */
        g.saved_pline_index = 0;
#endif
    }

    if (msg) {
        /* Caller is asking us to remember a top line that needed more.
           Should we call more?  This can happen when the player has set
           iflags.force_invmenu and they attempt to shoot with nothing in
           the quiver. */
        if (ttyDisplay && ttyDisplay->toplin == TOPLINE_NEED_MORE)
            ttyDisplay->toplin = TOPLINE_NON_EMPTY;

        /* move most recent message to history, make this become most recent */
        remember_topl();
        Strcpy(g.toplines, msg);
#ifdef DUMPLOG
        dumplogmsg(g.toplines);
#endif
    } else if (snapshot_mesgs) {
        nhassert(ttyDisplay == NULL ||
                 ttyDisplay->toplin != TOPLINE_NEED_MORE);

        /* done putting arbitrary messages in; put the snapshot ones back */
        for (idx = 0; snapshot_mesgs[idx]; ++idx) {
            remember_topl();
            Strcpy(g.toplines, snapshot_mesgs[idx]);
#ifdef DUMPLOG
            dumplogmsg(g.toplines);
#endif
        }
        /* now release the snapshot */
        free_msghistory_snapshot(TRUE);
        initd = FALSE; /* reset */
    }
}

#endif /* TTY_GRAPHICS */

/*topl.c*/

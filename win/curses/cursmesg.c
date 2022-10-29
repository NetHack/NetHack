/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.7 cursmesg.c */
/* Copyright (c) Karl Garrison, 2010. */
/* NetHack may be freely redistributed.  See license for details. */

#include "curses.h"
#include "hack.h"
#include "wincurs.h"
#include "cursmesg.h"
#include <ctype.h>

/* defined in sys/<foo>/<foo>tty.c or cursmain.c as last resort;
   set up by curses_init_nhwindows() */
extern char erase_char, kill_char;

/*
 * Note: references to "More>>" mean ">>", the curses rendition of "--More--".
 */

/* player can type ESC at More>> prompt to avoid seeing more messages
   for the current move; but hero might get more than one move per turn,
   so the input routines need to be able to cancel this */
long curs_mesg_suppress_seq = -1L;
/* if a message is marked urgent, existing suppression will be overridden
   so that messages resume being shown; this is used in case the urgent
   message triggers More>> for the previous message and the player responds
   with ESC; we need to avoid initiating suppression in that situation */
boolean curs_mesg_no_suppress = FALSE;

/* Message window routines for curses interface */

/* Private declatations */

typedef struct nhpm {
    char *str;                  /* Message text */
    long turn;                  /* Turn number for message */
    struct nhpm *prev_mesg;     /* Pointer to previous message */
    struct nhpm *next_mesg;     /* Pointer to next message */
} nhprev_mesg;

static void scroll_window(winid wid);
static void unscroll_window(winid wid);
static void directional_scroll(winid wid, int nlines);
static void mesg_add_line(const char *mline);
static nhprev_mesg *get_msg_line(boolean reverse, int mindex);

static int turn_lines = 0;
static int mx = 0;
static int my = 0;              /* message window text location */
static nhprev_mesg *first_mesg = NULL;
static nhprev_mesg *last_mesg = NULL;
static int max_messages;
static int num_messages = 0;
static int last_messages = 0;

/* Write string to the message window.  Attributes set by calling function. */

void
curses_message_win_puts(const char *message, boolean recursed)
{
    int height, width, border_space, linespace;
    char *tmpstr;
    WINDOW *win = curses_get_nhwin(MESSAGE_WIN);
    boolean bold, border = curses_window_has_border(MESSAGE_WIN);
    int message_length = (int) strlen(message);

#if 0
    /*
     * This was useful when curses used genl_putmsghistory() but is not
     * needed now that it has its own curses_putmsghistory() which is
     * capable of putting something into the ^P recall history without
     * displaying it at the same time.
     */
    if (strncmp("Count:", message, 6) == 0) {
        curses_count_window(message);
        return;
    }
#endif

    if (curs_mesg_suppress_seq == g.hero_seq) {
        return; /* user has typed ESC to avoid seeing remaining messages. */
    }

    curses_get_window_size(MESSAGE_WIN, &height, &width);
    border_space = (border ? 1 : 0);
    if (mx < border_space)
        mx = border_space;
    if (my < border_space)
        my = border_space;

    if (strcmp(message, "#") == 0) {    /* Extended command or Count: */
        if ((strcmp(g.toplines, "#") != 0)
            /* Bottom of message window */
            && (my >= (height - 1 + border_space)) && (height != 1)) {
            scroll_window(MESSAGE_WIN);
            mx = width;
            my--;
            Strcpy(g.toplines, message);
        }
        return;
    }

    if (!recursed) {
        strcpy(g.toplines, message);
        mesg_add_line(message);
    }

    /* -2: room for trailing ">>" (if More>> is needed) or leading "  "
       (if combining this message with preceding one) */
    linespace = (width - 1) - 2 - (mx - border_space);

    if (linespace < message_length) {
        if (my - border_space >= height - 1) {
            /* bottom of message win */
            if (++turn_lines > height
                || (turn_lines == height && mx > border_space)) {
                 /* pause until key is hit - ESC suppresses further messages
                    this turn unless an urgent message is being delivered */
                if (curses_more() == '\033'
                    && !curs_mesg_no_suppress) {
                    curs_mesg_suppress_seq = g.hero_seq;
                    return;
                }
                /* turn_lines reset to 0 by more()->block()->got_input() */
            } else {
                scroll_window(MESSAGE_WIN);
            }
        } else {
            if (mx != border_space) {
                my++;
                mx = border_space;
                ++turn_lines;
            }
        }
    } else { /* don't need to move to next line */
        /* if we aren't at the start of the line, we're combining multiple
           messages on one line; use 2-space separation */
        if (mx > border_space) {
            waddstr(win, "  ");
            mx += 2;
        }
    }

    bold = (height > 1 && !last_messages);
    if (bold)
        curses_toggle_color_attr(win, NONE, A_BOLD, ON);

    /* will this message fit as-is or do we need to split it? */
    if (mx == border_space && message_length > width - 2) {
        /* split needed */
        tmpstr = curses_break_str(message, (width - 2), 1);
        mvwprintw(win, my, mx, "%s", tmpstr), mx += (int) strlen(tmpstr);
        /* one space to separate first part of message from rest [is this
           actually useful?] */
        if (mx < width - 2)
            ++mx;
        free(tmpstr);
        if (bold)
            curses_toggle_color_attr(win, NONE, A_BOLD, OFF);
        tmpstr = curses_str_remainder(message, (width - 2), 1);
        curses_message_win_puts(tmpstr, TRUE);
        free(tmpstr);
    } else {
        mvwprintw(win, my, mx, "%s", message), mx += message_length;
        if (bold)
            curses_toggle_color_attr(win, NONE, A_BOLD, OFF);
    }
    wrefresh(win);
}

void
curses_got_input(void)
{
    /* if messages are being suppressed, reenable them */
    curs_mesg_suppress_seq = -1L;

    /* misleadingly named; represents number of lines delivered since
       player was sure to have had a chance to read them; if player
       has just given input then there aren't any such lines right now;
       that includes responding to More>> even though it stays same turn */
    turn_lines = 0;
}

int
curses_block(boolean noscroll) /* noscroll - blocking because of msgtype
                                * = stop/alert else blocking because
                                * window is full, so need to scroll after */
{
    static const char resp[] = " \r\n\033"; /* space, enter, esc */
    static int prev_x = -1, prev_y = -1, blink = 0;
    int height, width, moreattr, oldcrsr, ret = 0,
        brdroffset = curses_window_has_border(MESSAGE_WIN) ? 1 : 0;
    WINDOW *win = curses_get_nhwin(MESSAGE_WIN);

    curses_get_window_size(MESSAGE_WIN, &height, &width);
    if (mx - brdroffset > width - 3) { /* -3: room for ">>_" */
        if (my - brdroffset < height - 1)
            ++my, mx = brdroffset;
        else
            mx = width - 3 + brdroffset;
    }
    /* if ">>" (--More--) is being rendered at the same spot as before,
       toggle attributes so that the first '>' starts blinking if it wasn't
       or stops blinking if it was */
    if (mx == prev_x && my == prev_y) {
        blink = 1 - blink;
    } else {
        prev_x = mx, prev_y = my;
        blink = 0;
    }
    moreattr = !iflags.wc2_guicolor ? (int) A_REVERSE : NONE;
    curses_toggle_color_attr(win, MORECOLOR, moreattr, ON);
    if (blink) {
        wattron(win, A_BLINK);
        mvwprintw(win, my, mx, ">"), mx += 1;
        wattroff(win, A_BLINK);
        waddstr(win, ">"), mx += 1;
    } else {
        mvwprintw(win, my, mx, ">>"), mx += 2;
    }
    curses_toggle_color_attr(win, MORECOLOR, moreattr, OFF);
    wrefresh(win);

    /* cancel mesg suppression; all messages will have had chance to be read */
    curses_got_input();

    oldcrsr = curs_set(1);
    do {
        if (iflags.debug_fuzzer)
            ret = '\n';
        else
            ret = curses_read_char();
        if (ret == ERR || ret == '\0')
            ret = '\n';
        /* msgtype=stop should require space/enter rather than any key,
           as we want to prevent YASD from direction keys. */
    } while (!strchr(resp, (char) ret));
    if (oldcrsr >= 0)
        (void) curs_set(oldcrsr);

    if (height == 1) {
        curses_clear_unhighlight_message_window();
    } else {
        mx -= 2, mvwprintw(win, my, mx, "  "); /* back up and blank out ">>" */
        if (!noscroll) {
            scroll_window(MESSAGE_WIN);
        }
        wrefresh(win);
    }
    return ret;
}

int
curses_more(void)
{
    return curses_block(FALSE);
}


/* Clear the message window if one line; otherwise unhighlight old messages */

void
curses_clear_unhighlight_message_window(void)
{
    int mh, mw, count,
        brdroffset = curses_window_has_border(MESSAGE_WIN) ? 1 : 0;
    WINDOW *win = curses_get_nhwin(MESSAGE_WIN);

    turn_lines = 0;
    curses_get_window_size(MESSAGE_WIN, &mh, &mw);

    if (mh == 1) {
        curses_clear_nhwin(MESSAGE_WIN);
        mx = my = brdroffset;
    } else {
        mx = mw + brdroffset; /* Force new line on new turn */

        for (count = 0; count < mh; count++)
            mvwchgat(win, count + brdroffset, brdroffset,
                     mw, COLOR_PAIR(8), A_NORMAL, NULL);
        wnoutrefresh(win);
    }
    wmove(win, my, mx);
}


/* Reset message window cursor to starting position, and display most
   recent messages. */

void
curses_last_messages(void)
{
    nhprev_mesg *mesg;
    int i, height, width;
    int border = curses_window_has_border(MESSAGE_WIN) ? 1 : 0;
    WINDOW *win = curses_get_nhwin(MESSAGE_WIN);

    curses_get_window_size(MESSAGE_WIN, &height, &width);
    werase(win);
    mx = my = border;

    /*
     * FIXME!
     *  This shouldn't be relying on a naive line count to decide where
     *  to start and stop because curses_message_win_puts() combines short
     *  lines.  So we can end up with blank lines at bottom of the message
     *  window, missing out on one or more older messages which could have
     *  been included at the top.  Also long messages might wrap and take
     *  more than one line apiece.
     *
     *  3.6.2 showed oldest available N-1 lines (by starting at
     *  num_mesages - 1 and working back toward 0 until window height was
     *  reached [via index 'j' which is gone now]) plus the latest line
     *  (via toplines[]), rather than most recent N (start at height - 1
     *  and work way up through 0).  So it showed wrong subset of lines
     *  even if 'N lines' had been the right way to handle this.
     */
    ++last_messages;
    for (i = min(height, num_messages) - 1; i > 0; --i) {
        mesg = get_msg_line(TRUE, i);
        if (mesg && mesg->str && *mesg->str)
            curses_message_win_puts(mesg->str, TRUE);
    }
    curses_message_win_puts(g.toplines, TRUE);
    --last_messages;

    if (border)
        box(win, 0, 0);
    wrefresh(win);
}


/* Initialize list for message history */

void
curses_init_mesg_history(void)
{
    max_messages = iflags.msg_history;

    if (max_messages < 1) {
        max_messages = 1;
    }

    if (max_messages > MESG_HISTORY_MAX) {
        max_messages = MESG_HISTORY_MAX;
    }
}

/* Delete message history at game end. */

void
curses_teardown_messages(void)
{
    nhprev_mesg *current_mesg;

    while ((current_mesg = first_mesg) != 0) {
        first_mesg = current_mesg->next_mesg;
        free(current_mesg->str);
        free(current_mesg);
    }
    last_mesg = (nhprev_mesg *) 0;
    num_messages = 0;
}

/* Display previous messages in a popup (via menu so can scroll backwards) */

void
curses_prev_mesg(void)
{
    int count;
    winid wid;
    long turn = 0;
    anything Id;
    nhprev_mesg *mesg;
    menu_item *selected = NULL;
    boolean do_lifo = (iflags.prevmsg_window != 'f');
#ifdef DEBUG
    static int showturn = 0; /* 1: show hero_seq value in separators */
    int clr = 0;

    /*
     * Set DEBUGFILES=MesgTurn in environment or sysconf to decorate
     * separator line between blocks of messages with the turn they
     * were issued.
     */
    if (!showturn)
        showturn = (wizard && explicitdebug("MesgTurn")) ? 1 : -1;
#endif

    wid = curses_get_wid(NHW_MENU);
    curses_create_nhmenu(wid, 0UL);
    Id = cg.zeroany;

    for (count = 0; count < num_messages; ++count) {
        mesg = get_msg_line(do_lifo, count);
        if (mesg->turn != turn) {
            if (count > 0) { /* skip separator for first line */
                char sepbuf[50];

                Strcpy(sepbuf, "---");
#ifdef DEBUG
                if (showturn == 1)
                    Sprintf(sepbuf, "- %ld+%ld",
                            (mesg->turn >> 3), (mesg->turn & 7L));
#endif
                curses_add_menu(wid, &nul_glyphinfo, &Id, 0, 0,
                                A_NORMAL, clr, sepbuf, MENU_ITEMFLAGS_NONE);
            }
            turn = mesg->turn;
        }
        curses_add_menu(wid, &nul_glyphinfo, &Id, 0, 0,
                        A_NORMAL, clr, mesg->str, MENU_ITEMFLAGS_NONE);
    }
    if (!count)
        curses_add_menu(wid, &nul_glyphinfo, &Id, 0, 0,
                        A_NORMAL, clr, "[No past messages available.]",
                        MENU_ITEMFLAGS_NONE);

    curses_end_menu(wid, "");
    if (!do_lifo)
        curs_menu_set_bottom_heavy(wid);
    curses_select_menu(wid, PICK_NONE, &selected);
    if (selected) /* should always be null for PICK_NONE but be paranoid */
        free((genericptr_t) selected);
    curses_del_wid(wid);
}


/* Display at the bottom of the message window without remembering the
   line for ^P recall.  Used for putstr(WIN_MESSAGE,ATR_NOHISTORY,text)
   which core currently uses for 'Count: 123' and dolook's autodescribe.
   popup_dialog is not currently implemented for this function. */

void
curses_count_window(const char *count_text)
{
    static WINDOW *countwin = NULL;
    int winx, winy;
    int messageh, messagew, border;

    if (!count_text) {
        if (countwin)
            delwin(countwin), countwin = NULL;
        counting = FALSE;
        return;
    }

    /* position of message window, not current position within message window
       (so <0,0> for align_message:Top but will vary for other alignings) */
    curses_get_window_xy(MESSAGE_WIN, &winx, &winy);
    /* size of message window, with space for borders already subtracted */
    curses_get_window_size(MESSAGE_WIN, &messageh, &messagew);

    /* decide where to put the one-line counting window */
    border = curses_window_has_border(MESSAGE_WIN) ? 1 : 0;
    winx += border; /* first writeable message column */
    winy += border + (messageh - 1); /* last writable message line */

    /* if most recent message (probably prompt leading to this instance of
       counting window) is going to be covered up, scroll mesgs up a line */
    if (!counting && my == border + (messageh - 1) && mx > border) {
        scroll_window(MESSAGE_WIN);
        if (messageh > 1) {
            /* handling for next message will behave as if we're currently
               positioned at the end of next to last line of message window */
            my = border + (messageh - 1) - 1;
            mx = border + (messagew - 1); /* (0 + 80 - 1) or (1 + 78 - 1) */
        } else {
            /* for a one-line window, use beginning of only line instead */
            my = mx = border; /* 0 or 1 */
        }
        /* wmove(curses_get_nhwin(MESSAGE_WIN), my, mx); -- not needed */
    }
    /* in case we're being called from clear_nhwindow(MESSAGE_WIN)
       which gets called for every command keystroke; it sends an
       empty string to get the scroll-up-one-line effect above and
       we want to avoid the curses overhead for the operations below... */
    if (!*count_text)
        return;

    counting = TRUE;
#ifdef PDCURSES
    if (countwin)
        curses_destroy_win(countwin), countwin = NULL;
#endif /* PDCURSES */
    /* this used to specify a width of 25; that was adequate for 'Count: 123'
       but not for dolook's autodescribe when it refers to a named monster */
    if (!countwin)
        countwin = newwin(1, messagew, winy, winx);
    werase(countwin);

    mvwprintw(countwin, 0, 0, "%s", count_text);
    wrefresh(countwin);
    if (activemenu) {
        touchwin(activemenu);
        wrefresh(activemenu);
    }
}

/* Gets a "line" (buffer) of input. */
void
curses_message_win_getline(const char *prompt, char *answer, int buffer)
{
    int height, width; /* of window */
    char *tmpbuf, *p_answer; /* combined prompt + answer */
    int nlines, maxlines, i; /* prompt + answer */
    int promptline;
    int promptx;
    char **linestarts; /* pointers to start of each line */
    char *tmpstr; /* for free() */
    int maxy, maxx; /* linewrap / scroll */
    int ch;
    int border_space = 0;
    int ltmp, len; /* of answer string */
    boolean border = curses_window_has_border(MESSAGE_WIN);
    WINDOW *win = curses_get_nhwin(MESSAGE_WIN);

    orig_cursor = curs_set(0);

    curses_get_window_size(MESSAGE_WIN, &height, &width);
    if (border) {
        /* height -= 2, width -= 2; -- sizes already account for border */
        border_space = 1;
        if (mx < 1)
            mx = 1;
        if (my < 1)
            my = 1;
    }
    maxy = height - 1 + border_space;
    maxx = width - 1 + border_space;

    /* +2? buffer already includes room for terminator; +1: "prompt answer" */
    tmpbuf = (char *) alloc((unsigned) ((int) strlen(prompt) + buffer + 2));
    maxlines = buffer / width * 2;
    Strcpy(tmpbuf, prompt);
    Strcat(tmpbuf, " ");
    p_answer = tmpbuf + strlen(tmpbuf);
#ifdef EDIT_GETLIN
    len = (int) strlen(answer);
    if (len >= buffer) {
        len = buffer - 1;
        answer[len] = '\0';
    }
    Strcpy(p_answer, answer);
#else
    len = 0;
    *answer = '\0';
#endif
    nlines = curses_num_lines(tmpbuf, width);
    maxlines += nlines * 2;
    linestarts = (char **) alloc((unsigned) (maxlines * sizeof (char *)));
    linestarts[0] = tmpbuf;

    if (mx > border_space) { /* newline */
        if (my >= maxy)
            scroll_window(MESSAGE_WIN);
        else
            my++;
        mx = border_space;
    }

    curses_toggle_color_attr(win, NONE, A_BOLD, ON);

    for (i = 0; i < nlines - 1; i++) {
        tmpstr = curses_break_str(linestarts[i], width - 1, 1);
        linestarts[i + 1] = linestarts[i] + (int) strlen(tmpstr);
        if (*linestarts[i + 1] == ' ')
            linestarts[i + 1]++;
        mvwaddstr(win, my, mx, tmpstr);
        free(tmpstr);
        if (++my >= maxy) {
            scroll_window(MESSAGE_WIN);
            my--;
        }
    }
    promptline = nlines - 1;
    mvwaddstr(win, my, mx, linestarts[promptline]);
    ltmp = (int) strlen(linestarts[promptline]);
    mx = promptx = ltmp + border_space;
#ifdef EDIT_GETLIN
    if (len <= ltmp) {
        /* preloaded answer fits on same line as [last line of] prompt */
        promptx -= len;
    } else {
        int ltmp2 = len;

        /* preloaded answer spans lines so will be trickier to erase
           if that is called for; find where the end of the prompt will
           be without the answer appended */
        while (ltmp2 > 0) {
            if ((ltmp2 -= ltmp) < 0) {
                ltmp = -ltmp2;
                break;
            }
            promptline -= 1;
            ltmp = linestarts[promptline + 1] - linestarts[promptline];
        }
        promptx = ltmp + border_space;
    }
#endif

    while (1) {
        mx = (int) strlen(linestarts[nlines - 1]) + border_space;
        if (mx > maxx) {
            if (nlines < maxlines) {
                tmpstr = curses_break_str(linestarts[nlines - 1],
                                          width - 1, 1);
                mx = (int) strlen(tmpstr) + border_space;
                mvwprintw(win, my, mx, "%*c", maxx - mx + 1, ' ');
                if (++my > maxy) {
                    scroll_window(MESSAGE_WIN);
                    my--;
                }
                mx = border_space;
                linestarts[nlines] = linestarts[nlines - 1]
                                    + (int) strlen(tmpstr);
                if (*linestarts[nlines] == ' ')
                    linestarts[nlines]++;
                mvwaddstr(win, my, mx, linestarts[nlines]);
                mx = (int) strlen(linestarts[nlines]) + border_space;
                nlines++;
                free(tmpstr);
            } else {
                p_answer[--len] = '\0';
                mvwaddch(win, my, --mx, ' ');
            }
        }
        wmove(win, my, mx);
        curs_set(1);
        wrefresh(win);
        curses_got_input(); /* despite its name, before rather than after... */
#ifdef PDCURSES
        ch = wgetch(win);
#else
        ch = curses_read_char();
#endif
        curs_set(0);

        if (erase_char && ch == (int) (uchar) erase_char) {
            ch = '\177'; /* match switch-case below */

        /* honor kill_char if it's ^U or similar, but not if it's '@' */
        } else if (kill_char && ch == (int) (uchar) kill_char
                   && (ch < ' ' || ch >= '\177')) { /*ASCII*/
            if (len == 0) /* nothing to kill; just start over */
                continue;
            ch = '\033'; /* get rid of all current input, then start over */
        }

        switch (ch) {
        case ERR: /* should not happen */
            *answer = '\0';
            goto alldone;
        case '\033': /* DOESCAPE */
            /* if there isn't any input yet, return ESC */
            if (len == 0) {
                Strcpy(answer, "\033");
                goto alldone;
            }
            /* otherwise, discard current input and start over;
               first need to blank it from the screen */
            while (nlines - 1 > promptline) {
                if (nlines-- > height) {
                    unscroll_window(MESSAGE_WIN);
                    tmpstr = curses_break_str(linestarts[nlines - height],
                                              width - 1, 1);
                    mvwaddstr(win, border_space, border_space, tmpstr);
                    free(tmpstr);
                } else {
                    mx = border_space;
                    mvwprintw(win, my, mx, "%*c", maxx - mx, ' ');
                    my--;
                }
            }
            mx = promptx;
            mvwprintw(win, my, mx, "%*c", maxx - mx, ' ');
            *p_answer = '\0';
            len = 0;
            break;
        case '\r':
        case '\n':
            (void) strncpy(answer, p_answer, buffer);
            answer[buffer - 1] = '\0';
            Strcpy(g.toplines, tmpbuf);
            mesg_add_line(tmpbuf);
#if 1
            /* position at end of current line so next message will be
               written on next line regardless of whether it could fit here */
            mx = border_space ? (width + 1) : (width - 1);
            wmove(win, my, mx);
#else       /* after various other changes, this resulted in getline()
             * prompt+answer being following by a blank message line */
            if (++my > maxy) {
                scroll_window(MESSAGE_WIN);
                my--;
            }
            mx = border_space;
#endif /*0*/
            goto alldone;
        case '\177': /* DEL/Rubout */
        case KEY_DC: /* delete-character */
        case '\b': /* ^H (Backspace: '\010') */
        case KEY_BACKSPACE:
            if (len < 1) {
                len = 1;
                mx = promptx;
            }
            p_answer[--len] = '\0';
            mvwaddch(win, my, --mx, ' ');
            /* try to unwrap back to the previous line if there is one */
            if (nlines > 1 && (int) strlen(linestarts[nlines - 2]) < width) {
                mvwaddstr(win, my - 1, border_space, linestarts[nlines - 2]);
                if (nlines-- > height) {
                    unscroll_window(MESSAGE_WIN);
                    tmpstr = curses_break_str(linestarts[nlines - height],
                                              width - 1, 1);
                    mvwaddstr(win, border_space, border_space, tmpstr);
                    free(tmpstr);
                } else {
                    /* clean up the leftovers on the next line,
                       if we didn't scroll it away */
                    mvwprintw(win, my--, border_space, "%*c",
                              (int) strlen(linestarts[nlines]), ' ');
                }
            }
            break;
        default:
            p_answer[len++] = ch;
            if (len >= buffer)
                len = buffer - 1;
            else
                mvwaddch(win, my, mx, ch);
            p_answer[len] = '\0';
        }
    }

 alldone:
    free(linestarts);
    free(tmpbuf);
    curses_toggle_color_attr(win, NONE, A_BOLD, OFF);
    curs_set(orig_cursor);
    return;
}

/* Scroll lines upward in given window, or clear window if only one line. */
static void
scroll_window(winid wid)
{
    directional_scroll(wid, 1);
}

static void
unscroll_window(winid wid)
{
    directional_scroll(wid, -1);
}

static void
directional_scroll(winid wid, int nlines)
{
    int wh, ww, s_top, s_bottom;
    boolean border = curses_window_has_border(wid);
    WINDOW *win = curses_get_nhwin(wid);

    curses_get_window_size(wid, &wh, &ww);
    if (wh == 1) {
        curses_clear_nhwin(wid);
        return;
    }
    if (border) {
        s_top = 1;
        s_bottom = wh;
    } else {
        s_top = 0;
        s_bottom = wh - 1;
    }
    scrollok(win, TRUE);
    wsetscrreg(win, s_top, s_bottom);
    wscrl(win, nlines);
    scrollok(win, FALSE);
    if (wid == MESSAGE_WIN) {
        mx = border ? 1 : 0;
    }
    if (border) {
        box(win, 0, 0);
    }
    wrefresh(win);
}


/* Add given line to message history */

static void
mesg_add_line(const char *mline)
{
    nhprev_mesg *current_mesg;

    /*
     * Messages are kept in a doubly linked list, with head 'first_mesg',
     * tail 'last_mesg', and a maximum capacity of 'max_messages'.
     */
    if (num_messages < max_messages) {
        /* create a new list element */
        current_mesg = (nhprev_mesg *) alloc((unsigned) sizeof (nhprev_mesg));
        current_mesg->str = dupstr(mline);
        current_mesg->next_mesg = current_mesg->prev_mesg = (nhprev_mesg *) 0;
    } else {
        /* instead of discarding list element being forced out, reuse it */
        current_mesg = first_mesg;
        /* whenever new 'mline' is shorter, extra allocation size of the
           original element will be frittered away, until eventually we'll
           discard this 'str' and dupstr() a replacement; we could easily
           track the allocation size but don't really need to do so */
        if (strlen(mline) <= strlen(current_mesg->str)) {
            Strcpy(current_mesg->str, mline);
        } else {
            free((genericptr_t) current_mesg->str);
            current_mesg->str = dupstr(mline);
        }
    }
    current_mesg->turn = g.hero_seq;

    if (num_messages == 0) {
        /* very first message; set up head */
        first_mesg = current_mesg;
    } else {
        /* not first message; tail exists */
        last_mesg->next_mesg = current_mesg;
    }
    current_mesg->prev_mesg = last_mesg;
    last_mesg = current_mesg; /* new tail */

    if (num_messages < max_messages) {
        /* wasn't at capacity yet */
        ++num_messages;
    } else {
        /* at capacity; old head is being removed */
        first_mesg = first_mesg->next_mesg; /* new head */
        first_mesg->prev_mesg = NULL; /* head has no prev_mesg */
    }
    /* since 'current_mesg' might be reusing 'first_mesg' and has now
       become 'last_mesg', this update must be after head replacement */
    last_mesg->next_mesg = NULL; /* tail has no next_mesg */
}


/* Returns specified line from message history, or NULL if out of bounds */

static nhprev_mesg *
get_msg_line(boolean reverse, int mindex)
{
    int count;
    nhprev_mesg *current_mesg;

    if (reverse) {
        current_mesg = last_mesg;
        for (count = 0; count < mindex; count++) {
            if (!current_mesg)
                break;
            current_mesg = current_mesg->prev_mesg;
        }
    } else {
        current_mesg = first_mesg;
        for (count = 0; count < mindex; count++) {
            if (!current_mesg)
                break;
            current_mesg = current_mesg->next_mesg;
        }
    }
    return current_mesg;
}

/* save/restore code retrieves one ^P message at a time during save and
   puts it into save file; if any new messages are added to the list while
   that is taking place, the results are likely to be scrambled */
char *
curses_getmsghistory(boolean init)
{
    static int nxtidx;
    nhprev_mesg *mesg;

    if (init)
        nxtidx = 0;
    else
        ++nxtidx;

    if (nxtidx < num_messages) {
        /* we could encode mesg->turn with the text of the message,
           but then that text might need to be truncated, and more
           significantly, restoring the save file with another
           interface wouldn't know how find and decode or remove it;
           likewise, restoring another interface's save file with
           curses wouldn't find the expected turn info;
           so, we live without that */
        mesg = get_msg_line(FALSE, nxtidx);
    } else
        mesg = (nhprev_mesg *) 0;

    return mesg ? mesg->str : (char *) 0;
}

/*
 * This is called by the core savefile restore routines.
 * Each time we are called, we stuff the string into our message
 * history recall buffer. The core will send the oldest message
 * first (actually it sends them in the order they exist in the
 * save file, but that is supposed to be the oldest first).
 * These messages get pushed behind any which have been issued
 * during this session since they come from a previous session
 * and logically precede anything (like "Restoring save file...")
 * that's happened now.
 *
 * Called with a null pointer to finish up restoration.
 *
 * It's also called by the quest pager code when a block message
 * has a one-line summary specified.  We put that line directly
 * into message history for ^P recall without having displayed it.
 */
void
curses_putmsghistory(const char *msg, boolean restoring_msghist)
{
    /* FIXME: these should be moved to struct instance_globals g */
    static boolean initd = FALSE;
    static int stash_count;
    static nhprev_mesg *stash_head = 0;

    if (restoring_msghist && !initd) {
        /* hide any messages we've gathered since starting current session
           so that the ^P data will start out empty as we add ones being
           restored from save file; we'll put these back after that's done */
        stash_count = num_messages, num_messages = 0;
        stash_head = first_mesg, first_mesg = (nhprev_mesg *) 0;
        last_mesg = (nhprev_mesg *) 0; /* no need to remember the tail */
        initd = TRUE;
#ifdef DUMPLOG
        /* this suffices; there's no need to scrub g.saved_pline[] pointers */
        g.saved_pline_index = 0;
#endif
    }

    if (msg) {
        mesg_add_line(msg);
        /* treat all saved and restored messages as turn #1;
           however, we aren't only called when restoring history;
           core uses putmsghistory() for other stuff during play
           and those messages should have a normal turn value */
        last_mesg->turn = restoring_msghist ? (1L << 3) : g.hero_seq;
#ifdef DUMPLOG
        dumplogmsg(last_mesg->str);
#endif
    } else if (stash_count) {
        nhprev_mesg *mesg;
        long mesg_turn;

        /* put any messages generated during the beginning of the current
           session back; they logically follow any from the previous
           session's save file */
        while (stash_count > 0) {
            /* we could manipulate the linked list directly but treating
               stashed messages as newly occurring ones is much simpler;
               we ignore the backlinks because the list is destroyed as it
               gets processed hence there can't be any other traversals */
            mesg = stash_head;
            stash_head = mesg->next_mesg;
            --stash_count;
            mesg_turn = mesg->turn;
            mesg_add_line(mesg->str);
            /* added line became new tail */
            last_mesg->turn = mesg_turn;
#ifdef DUMPLOG
            dumplogmsg(mesg->str);
#endif
            free((genericptr_t) mesg->str);
            free((genericptr_t) mesg);
        }
        initd = FALSE; /* reset */
    }
}

/*cursmesg.c*/

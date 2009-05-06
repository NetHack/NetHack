/* NetHack 3.5	topl.c	$Date$  $Revision$ */
/*	SCCS Id: @(#)topl.c	3.5	2003/10/05	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef TTY_GRAPHICS

#include "tcap.h"
#include "wintty.h"
#include <ctype.h>

#ifndef C	/* this matches src/cmd.c */
#define C(c)	(0x1f & (c))
#endif

STATIC_DCL void FDECL(redotoplin, (const nhwchar*));
STATIC_DCL void FDECL(topl_putsym, (NHWCHAR_P));
STATIC_DCL void NDECL(remember_topl);
STATIC_DCL void FDECL(removetopl, (int));
STATIC_DCL void FDECL(msghistory_snapshot, (BOOLEAN_P));
STATIC_DCL void FDECL(free_msghistory_snapshot, (BOOLEAN_P));

extern nhwchar emptysym[];

int
tty_doprev_message()
{
    register struct WinDesc *cw = wins[WIN_MESSAGE];

    winid prevmsg_win;
    int i;
#ifdef UNICODE_WIDEWINPORT
    char buf[BUFSZ];
#endif
    if ((iflags.prevmsg_window != 's') && !ttyDisplay->inread) { /* not single */
        if(iflags.prevmsg_window == 'f') { /* full */
            prevmsg_win = create_nhwindow(NHW_MENU);
            putstr(prevmsg_win, 0, "Message History");
            putstr(prevmsg_win, 0, "");
            cw->maxcol = cw->maxrow;
            i = cw->maxcol;
            do {
#ifdef UNICODE_WIDEWINPORT
                if(cw->data[i] && nhwstrcmp(cw->data[i], "") ) {
		    strnhwcpy(buf, cw->data[i]);
                    putstr(prevmsg_win, 0, buf);
#else
                if(cw->data[i] && strcmp(cw->data[i], "") ) {
                    putstr(prevmsg_win, 0, cw->data[i]);
#endif
                }
                i = (i + 1) % cw->rows;
            } while (i != cw->maxcol);
#ifdef UNICODE_WIDEWINPORT
	    strnhwcpy(buf, toplines);
            putstr(prevmsg_win, 0, buf);
#else
            putstr(prevmsg_win, 0, toplines);
#endif
            display_nhwindow(prevmsg_win, TRUE);
            destroy_nhwindow(prevmsg_win);
        } else if (iflags.prevmsg_window == 'c') {		/* combination */
            do {
                morc = 0;
                if (cw->maxcol == cw->maxrow) {
                    ttyDisplay->dismiss_more = C('p');	/* <ctrl/P> allowed at --More-- */
                    redotoplin(toplines);
                    cw->maxcol--;
                    if (cw->maxcol < 0) cw->maxcol = cw->rows-1;
                    if (!cw->data[cw->maxcol])
                        cw->maxcol = cw->maxrow;
                } else if (cw->maxcol == (cw->maxrow - 1)){
                    ttyDisplay->dismiss_more = C('p');	/* <ctrl/P> allowed at --More-- */
                    redotoplin(cw->data[cw->maxcol]);
                    cw->maxcol--;
                    if (cw->maxcol < 0) cw->maxcol = cw->rows-1;
                    if (!cw->data[cw->maxcol])
                        cw->maxcol = cw->maxrow;
                } else {
                    prevmsg_win = create_nhwindow(NHW_MENU);
                    putstr(prevmsg_win, 0, "Message History");
                    putstr(prevmsg_win, 0, "");
                    cw->maxcol = cw->maxrow;
                    i = cw->maxcol;
                    do {
#ifdef UNICODE_WIDEWINPORT
                        if(cw->data[i] && nhwstrcmp(cw->data[i], "") ) {
			    strnhwcpy(buf, cw->data[i]);
                            putstr(prevmsg_win, 0, buf);
#else
                        if(cw->data[i] && strcmp(cw->data[i], "") ) {
                            putstr(prevmsg_win, 0, cw->data[i]);
#endif
			}
                        i = (i + 1) % cw->rows;
                    } while (i != cw->maxcol);

#ifdef UNICODE_WIDEWINPORT
		    strnhwcpy(buf, toplines);
                    putstr(prevmsg_win, 0, buf);
#else
                    putstr(prevmsg_win, 0, toplines);
#endif
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
#ifdef UNICODE_WIDEWINPORT
            strnhwcpy(buf, toplines);
            putstr(prevmsg_win, 0, buf);
#else
            putstr(prevmsg_win, 0, toplines);
#endif
            cw->maxcol=cw->maxrow-1;
            if(cw->maxcol < 0) cw->maxcol = cw->rows-1;
            do {
#ifdef UNICODE_WIDEWINPORT
            	strnhwcpy(buf, cw->data[cw->maxcol]);
                putstr(prevmsg_win, 0, buf);
#else
		putstr(prevmsg_win, 0, cw->data[cw->maxcol]);

#endif
                cw->maxcol--;
                if (cw->maxcol < 0) cw->maxcol = cw->rows-1;
                if (!cw->data[cw->maxcol])
                    cw->maxcol = cw->maxrow;
            } while (cw->maxcol != cw->maxrow);

            display_nhwindow(prevmsg_win, TRUE);
            destroy_nhwindow(prevmsg_win);
            cw->maxcol = cw->maxrow;
            ttyDisplay->dismiss_more = 0;
        }
    } else if(iflags.prevmsg_window == 's') { /* single */
        ttyDisplay->dismiss_more = C('p');  /* <ctrl/P> allowed at --More-- */
        do {
            morc = 0;
            if (cw->maxcol == cw->maxrow)
                redotoplin(toplines);
            else if (cw->data[cw->maxcol])
                redotoplin(cw->data[cw->maxcol]);
            cw->maxcol--;
            if (cw->maxcol < 0) cw->maxcol = cw->rows-1;
            if (!cw->data[cw->maxcol])
                cw->maxcol = cw->maxrow;
        } while (morc == C('p'));
        ttyDisplay->dismiss_more = 0;
    }
    return 0;
}

STATIC_OVL void
redotoplin(symstr)
    const nhwchar *symstr;
{
	int otoplin = ttyDisplay->toplin;
	home();
#ifdef UNICODE_WIDEWINPORT
	if(*symstr >= 0x80) {
#else
	if(*symstr & 0x80) {
#endif
		/* kludge for the / command, the only time we ever want a */
		/* graphics character on the top line */
		g_putch((int)*symstr++);
		ttyDisplay->curx++;
	}
	end_glyphout();	/* in case message printed during graphics output */
	putsyms(symstr);
	cl_end();
	ttyDisplay->toplin = 1;
	if(ttyDisplay->cury && otoplin != 3)
		more();
}

STATIC_OVL void
remember_topl()
{
    register struct WinDesc *cw = wins[WIN_MESSAGE];
    int idx = cw->maxrow;
    unsigned len;

    if ((cw->flags & WIN_LOCKHISTORY) || !*toplines) return;

#ifdef UNICODE_WIDEWINPORT
    len = nhwlen(toplines) + 1;
#else
    len = strlen(toplines) + 1;
#endif
    if (len > (unsigned)cw->datlen[idx]) {
	if (cw->data[idx]) free(cw->data[idx]);
	len += (8 - (len & 7));		/* pad up to next multiple of 8 */
	cw->data[idx] = (nhwchar *)alloc(sizeof(nhwchar) * len);
	cw->datlen[idx] = (short)len;
    }
#ifdef UNICODE_WIDEWINPORT
    (void)nhwcpy(cw->data[idx], toplines);
#else
    Strcpy(cw->data[idx], toplines);
#endif
    *toplines = '\0';
    cw->maxcol = cw->maxrow = (idx + 1) % cw->rows;
}

void
addtopl(s)
const nhwchar *s;
{
    register struct WinDesc *cw = wins[WIN_MESSAGE];

    tty_curs(BASE_WINDOW,cw->curx+1,cw->cury);
    putsyms(s);
    cl_end();
    ttyDisplay->toplin = 1;
}

void
more()
{
    struct WinDesc *cw = wins[WIN_MESSAGE];

    /* avoid recursion -- only happens from interrupts */
    if(ttyDisplay->inmore++)
	return;

    if(ttyDisplay->toplin) {
	tty_curs(BASE_WINDOW, cw->curx+1, cw->cury);
	if(cw->curx >= CO - 8)
#ifdef UNICODE_WIDEWINPORT
		topl_putsym(L'\n');
#else
		topl_putsym('\n');
#endif
    }

    if(flags.standout)
	standoutbeg();
    putsyms(defmorestr);
    if(flags.standout)
	standoutend();

    xwaitforspace("\033 ");

    if(morc == '\033')
	cw->flags |= WIN_STOP;

    if(ttyDisplay->toplin && cw->cury) {
	docorner(1, cw->cury+1);
	cw->curx = cw->cury = 0;
	home();
    } else if(morc == '\033') {
	cw->curx = cw->cury = 0;
	home();
	cl_end();
    }
    ttyDisplay->toplin = 0;
    ttyDisplay->inmore = 0;
}

void
update_topl(bp)
	register const nhwchar *bp;
{
	register nhwchar *tl, *otl;
	register int n0;
	int notdied = 1;
	struct WinDesc *cw = wins[WIN_MESSAGE];

	/* If there is room on the line, print message on same line */
	/* But messages like "You die..." deserve their own line */
#ifdef UNICODE_WIDEWINPORT
	n0 = nhwlen(bp);
#else
	n0 = strlen(bp);
#endif
	if ((ttyDisplay->toplin == 1 || (cw->flags & WIN_STOP)) &&
	    cw->cury == 0 &&
#ifdef UNICODE_WIDEWINPORT
	    n0 + (int)nhwlen(toplines) + 3 < CO-8 &&  /* room for --More-- */
	    (notdied = nhwncmp(bp, L"You die", 7))) {
		(void)nhwcat(toplines, L"  ");
		(void)nhwcat(toplines, bp);
#else
	    n0 + (int)strlen(toplines) + 3 < CO-8 &&  /* room for --More-- */
	    (notdied = strncmp(bp, "You die", 7))) {
		Strcat(toplines, "  ");
		Strcat(toplines, bp);
#endif
		cw->curx += 2;
		if(!(cw->flags & WIN_STOP))
		    addtopl(bp);
		return;
	} else if (!(cw->flags & WIN_STOP)) {
	    if(ttyDisplay->toplin == 1) more();
	    else if(cw->cury) {	/* for when flags.toplin == 2 && cury > 1 */
		docorner(1, cw->cury+1); /* reset cury = 0 if redraw screen */
		cw->curx = cw->cury = 0;/* from home--cls() & docorner(1,n) */
	    }
	}
	remember_topl();
#ifdef UNICODE_WIDEWINPORT
	(void) nhwncpy(toplines, bp, TBUFSZ);
#else
	(void) strncpy(toplines, bp, TBUFSZ);
#endif
	toplines[TBUFSZ - 1] = 0;

	for(tl = toplines; n0 >= CO; ){
	    otl = tl;
	    for(tl+=CO-1; tl != otl && !isspace(*tl); --tl) ;
	    if(tl == otl) {
		/* Eek!  A huge token.  Try splitting after it. */
#ifdef UNICODE_WIDEWINPORT
		tl = nhwindex(otl, ' ');
#else
		tl = index(otl, ' ');
#endif
		if (!tl) break;    /* No choice but to spit it out whole. */
	    }
#ifdef UNICODE_WIDEWINPORT
	    *tl++ = (nhwchar)'\n';
	    n0 = nhwlen(tl);
#else
	    *tl++ = '\n';
	    n0 = strlen(tl);
#endif
	}
	if(!notdied) cw->flags &= ~WIN_STOP;
	if(!(cw->flags & WIN_STOP)) redotoplin(toplines);
}

#ifdef UNICODE_WIDEWINPORT
#define T(x) L##x
#else
#define T(x) x
#endif

STATIC_OVL
void
topl_putsym(c)
    nhwchar c;
{
    register struct WinDesc *cw = wins[WIN_MESSAGE];

    if(cw == (struct WinDesc *) 0) panic("Putsym window MESSAGE nonexistant");
	
    switch(c) {
    case T('\b'):
	if(ttyDisplay->curx == 0 && ttyDisplay->cury > 0)
	    tty_curs(BASE_WINDOW, CO, (int)ttyDisplay->cury-1);
	backsp();
	ttyDisplay->curx--;
	cw->curx = ttyDisplay->curx;
	return;
    case T('\n'):
	cl_end();
	ttyDisplay->curx = 0;
	ttyDisplay->cury++;
	cw->cury = ttyDisplay->cury;
#ifdef WIN32CON
    (void) putchar(c);
#endif
	break;
    default:
	if(ttyDisplay->curx == CO-1)
	    topl_putsym(T('\n')); /* 1 <= curx <= CO; avoid CO */
#ifdef WIN32CON
    (void) putchar(c);
#endif
	ttyDisplay->curx++;
    }
    cw->curx = ttyDisplay->curx;
    if(cw->curx == 0) cl_end();
#ifndef WIN32CON
    (void) putchar(c);
#endif
}

#undef T

void
putsyms(symstr)
    const nhwchar *symstr;
{
    while(*symstr)
	topl_putsym(*symstr++);
}

STATIC_OVL void
removetopl(n)
register int n;
{
    /* assume addtopl() has been done, so ttyDisplay->toplin is already set */
    while (n-- > 0)
#ifdef UNICODE_WIDEWINPORT
	putsyms(L"\b \b");
#else
	putsyms("\b \b");
#endif
}

extern char erase_char;		/* from xxxtty.c; don't need kill_char */

char
tty_yn_function(query,resp, def)
const char *query,*resp;
char def;
/*
 *   Generic yes/no function. 'def' is the default (returned by space or
 *   return; 'esc' returns 'q', or 'n', or the default, depending on
 *   what's in the string. The 'query' string is printed before the user
 *   is asked about the string.
 *   If resp is NULL, any single character is accepted and returned.
 *   If not-NULL, only characters in it are allowed (exceptions:  the
 *   quitchars are always allowed, and if it contains '#' then digits
 *   are allowed); if it includes an <esc>, anything beyond that won't
 *   be shown in the prompt to the user but will be acceptable as input.
 */
{
	register char q;
	char rtmp[40];
	boolean digit_ok, allow_num;
	struct WinDesc *cw = wins[WIN_MESSAGE];
	boolean doprev = 0;
	char prompt[BUFSZ];
#ifdef UNICODE_WIDEWINPORT
	nhwchar wprompt[BUFSZ];
#endif

	if(ttyDisplay->toplin == 1 && !(cw->flags & WIN_STOP)) more();
	cw->flags &= ~WIN_STOP;
	ttyDisplay->toplin = 3; /* special prompt state */
	ttyDisplay->inread++;
	if (resp) {
	    char *rb, respbuf[QBUFSZ];

	    allow_num = (index(resp, '#') != 0);
	    Strcpy(respbuf, resp);
	    /* any acceptable responses that follow <esc> aren't displayed */
	    if ((rb = index(respbuf, '\033')) != 0) *rb = '\0';
	    (void)strncpy(prompt, query, QBUFSZ-1);
	    prompt[QBUFSZ-1] = '\0';
	    Sprintf(eos(prompt), " [%s]", respbuf);
	    if (def) Sprintf(eos(prompt), " (%c)", def);
	    /* not pline("%s ", prompt);
	       trailing space is wanted here in case of reprompt */
	    Strcat(prompt, " ");
	    pline("%s", prompt);
	} else {
	    pline("%s ", query);
	    q = readchar();
	    goto clean_up;
	}

	do {	/* loop until we get valid input */
	    q = lowc(readchar());
	    if (q == '\020') { /* ctrl-P */
		if (iflags.prevmsg_window != 's') {
		    int sav = ttyDisplay->inread;
		    ttyDisplay->inread = 0;
		    (void) tty_doprev_message();
		    ttyDisplay->inread = sav;
		    tty_clear_nhwindow(WIN_MESSAGE);
		    cw->maxcol = cw->maxrow;
#ifdef UNICODE_WIDEWINPORT
		    nhwstrcpy(wprompt, prompt);
		    addtopl(wprompt);
#else
		    addtopl(prompt);
#endif
		} else {
		    if(!doprev)
			(void) tty_doprev_message(); /* need two initially */
		    (void) tty_doprev_message();
		    doprev = 1;
		}
		q = '\0';	/* force another loop iteration */
		continue;
	    } else if (doprev) {
		/* BUG[?]: this probably ought to check whether the
		   character which has just been read is an acceptable
		   response; if so, skip the reprompt and use it. */
		tty_clear_nhwindow(WIN_MESSAGE);
		cw->maxcol = cw->maxrow;
		doprev = 0;
#ifdef UNICODE_WIDEWINPORT
		nhwstrcpy(wprompt, prompt);
		addtopl(wprompt);
#else
		addtopl(prompt);
#endif
		q = '\0';	/* force another loop iteration */
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
		q = (char)0;
	    } else if (q == '#' || digit_ok) {
		char z;
		nhwchar digit_string[2];
		int n_len = 0;
		long value = 0;
#ifdef UNICODE_WIDEWINPORT
		addtopl(L"#"),  n_len++;
#else
		addtopl("#"),  n_len++;
#endif
		digit_string[1] = (nhwchar)0;
		if (q != '#') {
		    digit_string[0] = (nhwchar)q;
		    addtopl(digit_string),  n_len++;
		    value = q - '0';
		    q = '#';
		}
		do {	/* loop until we get a non-digit */
		    z = lowc(readchar());
		    if (digit(z)) {
			value = (10 * value) + (z - '0');
			if (value < 0) break;	/* overflow: try again */
			digit_string[0] = (nhwchar)z;
			addtopl(digit_string),  n_len++;
		    } else if (z == 'y' || index(quitchars, z)) {
			if (z == '\033')  value = -1;	/* abort */
			z = '\n';	/* break */
		    } else if (z == erase_char || z == '\b') {
			if (n_len <= 1) { value = -1;  break; }
			else { value /= 10;  removetopl(1),  n_len--; }
		    } else {
			value = -1;	/* abort */
			tty_nhbell();
			break;
		    }
		} while (z != '\n');
		if (value > 0) yn_number = value;
		else if (value == 0) q = 'n';		/* 0 => "no" */
		else {	/* remove number from top line, then try again */
			removetopl(n_len),  n_len = 0;
			q = '\0';
		}
	    }
	} while(!q);

	if (q != '#') {
		Sprintf(rtmp, "%c", q);
#ifdef UNICODE_WIDEWINPORT
		nhwstrcpy(wprompt, rtmp);   /* rtmp[40] -> wprompt[256] ok */
		addtopl(wprompt);
#else
		addtopl(rtmp);
#endif
	}
    clean_up:
	ttyDisplay->inread--;
	ttyDisplay->toplin = 2;
	if (ttyDisplay->intr) ttyDisplay->intr--;
	if(wins[WIN_MESSAGE]->cury)
	    tty_clear_nhwindow(WIN_MESSAGE);

	return q;
}

/* shared by tty_getmsghistory() and tty_putmsghistory() */
static nhwchar **snapshot_mesgs = 0;

/* collect currently available message history data into a sequential array;
   optionally, purge that data from the active circular buffer set as we go */
STATIC_OVL void
msghistory_snapshot(purge)
boolean purge;		/* clear message history buffer as we copy it */
{
    nhwchar *mesg;
    int i, inidx, outidx;
    struct WinDesc *cw;

    /* paranoia (too early or too late panic save attempt?) */
    if (WIN_MESSAGE == WIN_ERR || !wins[WIN_MESSAGE]) return;
    cw = wins[WIN_MESSAGE];

    /* flush toplines[], moving most recent message to history */
    remember_topl();

    /* for a passive snapshot, we just copy pointers, so can't allow further
       history updating to take place because that could clobber them */
    if (!purge) cw->flags |= WIN_LOCKHISTORY;

    snapshot_mesgs = (nhwchar **)alloc((cw->rows + 1) * sizeof (nhwchar *));
    outidx = 0;
    inidx = cw->maxrow;
    for (i = 0; i < cw->rows; ++i) {
	snapshot_mesgs[i] = (nhwchar *)0;
	mesg = cw->data[inidx];
	if (mesg && *mesg) {
	    snapshot_mesgs[outidx++] = mesg;
	    if (purge) {
		/* we're taking this pointer away; subsequest history
		   updates will eventually allocate a new one to replace it */
		cw->data[inidx] = (nhwchar *)0;
		cw->datlen[inidx] = 0;
	    }
	}
	inidx = (inidx + 1) % cw->rows;
    }
    snapshot_mesgs[cw->rows] = (nhwchar *)0;	/* sentinel */

    /* for a destructive snapshot, history is now completely empty */
    if (purge) cw->maxcol = cw->maxrow = 0;
}

/* release memory allocated to message history snapshot */
STATIC_OVL void
free_msghistory_snapshot(purged)
boolean purged;	/* True: took history's pointers, False: just cloned them */
{
    if (snapshot_mesgs) {
	/* snapshot pointers are no longer in use */
	if (purged) {
	    int i;

	    for (i = 0; snapshot_mesgs[i]; ++i)
		free((genericptr_t)snapshot_mesgs[i]);
	}

	free((genericptr_t)snapshot_mesgs), snapshot_mesgs = (nhwchar **)0;

	/* history can resume being updated at will now... */
	if (!purged) wins[WIN_MESSAGE]->flags &= ~WIN_LOCKHISTORY;
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
tty_getmsghistory(init)
boolean init;
{
    static int nxtidx;
    nhwchar *nextmesg;
    char *result = 0;

    if (init) {
	msghistory_snapshot(FALSE);
	nxtidx = 0;
    }

    if (snapshot_mesgs) {
	nextmesg = snapshot_mesgs[nxtidx++];
	if (nextmesg) {
#ifdef UNICODE_WIDEWINPORT
	    static char histbuf[BUFSZ];

	    result = strnhwcpy(histbuf, nextmesg);
#else
	    result = (char *)nextmesg;
#endif
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
 * message history for ^P recall without having displayed it.
 */
void
tty_putmsghistory(msg, restoring)
const char *msg;
boolean restoring;
{
    static boolean initd = FALSE;
    int idx;

    if (restoring && !initd) {
	/* we're restoring history from the previous session, but new
	   messages have already been issued this session ("Restoring...",
	   for instance); collect current history (ie, those new messages),
	   and also clear it out so that nothing will be present when the
	   restored ones are being put into place */
	msghistory_snapshot(TRUE);
	initd = TRUE;
    }

    if (msg) {
	/* move most recent message to history, make this become most recent */
	remember_topl();
#ifdef UNICODE_WIDEWINPORT
	(void)nhwstrcpy(toplines, msg);
#else
	Strcpy(toplines, msg);
#endif
    } else if (snapshot_mesgs) {
	/* done putting arbitrary messages in; put the snapshot ones back */
	for (idx = 0; snapshot_mesgs[idx]; ++idx) {
	    remember_topl();
#ifdef UNICODE_WIDEWINPORT
	    (void)nhwcpy(toplines, snapshot_mesgs[idx]);
#else
	    Strcpy(toplines, snapshot_mesgs[idx]);
#endif
	}
	/* now release the snapshot */
	free_msghistory_snapshot(TRUE);
	initd = FALSE;	/* reset */
    }
}

#endif /* TTY_GRAPHICS */

/*topl.c*/

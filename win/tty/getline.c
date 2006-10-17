/*	SCCS Id: @(#)getline.c	3.5	2002/10/06	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef TTY_GRAPHICS

#if !defined(MAC)
#define NEWAUTOCOMP
#endif

#include "wintty.h"
#include "func_tab.h"

char morc = 0;	/* tell the outside world what char you chose */
STATIC_DCL boolean FDECL(ext_cmd_getlin_hook, (char *));

typedef boolean FDECL((*getlin_hook_proc), (char *));

STATIC_DCL void FDECL(hooked_tty_getlin, (const char*,char*,getlin_hook_proc));
extern int NDECL(extcmd_via_menu);	/* cmd.c */

extern char erase_char, kill_char;	/* from appropriate tty.c file */

#ifdef UNICODE_WIDEWINPORT
#define T(x) L##x
#else
#define T(x) x
#endif

/*
 * Read a line closed with '\n' into the array char bufp[BUFSZ].
 * (The '\n' is not stored. The string is closed with a '\0'.)
 * Reading can be interrupted by an escape ('\033') - now the
 * resulting string is "\033".
 */
void
tty_getlin(query, bufp)
const char *query;
register char *bufp;
{
    hooked_tty_getlin(query, bufp, (getlin_hook_proc) 0);
}

STATIC_OVL void
hooked_tty_getlin(query, bufp, hook)
const char *query;
register char *bufp;
getlin_hook_proc hook;
{
	register char *obufp = bufp;
	register int c;
	struct WinDesc *cw = wins[WIN_MESSAGE];
	boolean doprev = 0;
#ifdef UNICODE_WIDEWINPORT
	nhwchar wbuf[BUFSZ];
#endif

	if(ttyDisplay->toplin == 1 && !(cw->flags & WIN_STOP)) more();
	cw->flags &= ~WIN_STOP;
	ttyDisplay->toplin = 3; /* special prompt state */
	ttyDisplay->inread++;
	pline("%s ", query);
	*obufp = 0;
	for(;;) {
#ifdef UNICODE_WIDEWINPORT
		char buf[BUFSZ];
		(void) fflush(stdout);
		Sprintf(buf, "%s ", query);
		Strcat(buf, obufp);
		nhwstrcpy(wbuf, buf);
		(void)nhwcpy(toplines, wbuf);
#else
		(void) fflush(stdout);
		Sprintf(toplines, "%s ", query);
		Strcat(toplines, obufp);
#endif
		if((c = Getchar()) == EOF) c = '\033';
		if(c == '\033') {
			*obufp = c;
			obufp[1] = 0;
			break;
		}
		if (ttyDisplay->intr) {
		    ttyDisplay->intr--;
		    *bufp = 0;
		}
		if(c == '\020') { /* ctrl-P */
		    if (iflags.prevmsg_window != 's') {
			int sav = ttyDisplay->inread;
			ttyDisplay->inread = 0;
			(void) tty_doprev_message();
			ttyDisplay->inread = sav;
			tty_clear_nhwindow(WIN_MESSAGE);
			cw->maxcol = cw->maxrow;
#ifdef UNICODE_WIDEWINPORT
			nhwstrcpy(wbuf, query);
			addtopl(wbuf);
			addtopl(L" ");
			*bufp = 0;
			nhwstrcpy(wbuf, obufp);
			addtopl(wbuf);
#else
			addtopl(query);
			addtopl(" ");
			*bufp = 0;
			addtopl(obufp);
#endif
		    } else {
			if (!doprev)
			    (void) tty_doprev_message();/* need two initially */
			(void) tty_doprev_message();
			doprev = 1;
			continue;
		    }
		} else if (doprev && iflags.prevmsg_window == 's') {
		    tty_clear_nhwindow(WIN_MESSAGE);
		    cw->maxcol = cw->maxrow;
		    doprev = 0;
#ifdef UNICODE_WIDEWINPORT
		    nhwstrcpy(wbuf, query);
		    addtopl(wbuf);
		    addtopl(L" ");
#else
		    addtopl(query);
		    addtopl(" ");
#endif

		    *bufp = 0;
#ifdef UNICODE_WIDEWINPORT
		    nhwstrcpy(wbuf, obufp);
		    addtopl(wbuf);
#else
		    addtopl(obufp);
#endif
		}
		if(c == erase_char || c == '\b') {
			if(bufp != obufp) {
#ifdef NEWAUTOCOMP
				char *i;

#endif /* NEWAUTOCOMP */
				bufp--;
#ifndef NEWAUTOCOMP
				putsyms(T("\b \b"));/* putsym converts \b */
#else /* NEWAUTOCOMP */
				putsyms(T("\b"));
				for (i = bufp; *i; ++i) putsyms(T(" "));
				for (; i > bufp; --i) putsyms(T("\b"));
				*bufp = 0;
#endif /* NEWAUTOCOMP */
			} else	tty_nhbell();
#if defined(apollo)
		} else if(c == '\n' || c == '\r') {
#else
		} else if(c == '\n') {
#endif
#ifndef NEWAUTOCOMP
			*bufp = 0;
#endif /* not NEWAUTOCOMP */
			break;
		} else if(' ' <= (unsigned char) c && c != '\177' &&
			    (bufp-obufp < BUFSZ-1 && bufp-obufp < COLNO)) {
				/* avoid isprint() - some people don't have it
				   ' ' is not always a printing char */
#ifdef NEWAUTOCOMP
			char *i = eos(bufp);

#endif /* NEWAUTOCOMP */
			*bufp = c;
			bufp[1] = 0;
#ifdef UNICODE_WIDEWINPORT
			nhwstrcpy(wbuf, bufp);
			putsyms(wbuf);
#else
			putsyms(bufp);
#endif
			bufp++;
			if (hook && (*hook)(obufp)) {
#ifdef UNICODE_WIDEWINPORT
			    nhwstrcpy(wbuf, bufp);
			    putsyms(wbuf);
#else
			    putsyms(bufp);
#endif
#ifndef NEWAUTOCOMP
			    bufp = eos(bufp);
#else /* NEWAUTOCOMP */
			    /* pointer and cursor left where they were */
			    for (i = bufp; *i; ++i) putsyms(T("\b"));
			} else if (i > bufp) {
			    char *s = i;

			    /* erase rest of prior guess */
			    for (; i > bufp; --i) putsyms(T(" "));
			    for (; s > bufp; --s) putsyms(T("\b"));
#endif /* NEWAUTOCOMP */
			}
		} else if(c == kill_char || c == '\177') { /* Robert Viduya */
				/* this test last - @ might be the kill_char */
#ifndef NEWAUTOCOMP
			while(bufp != obufp) {
				bufp--;
				putsyms(T("\b \b"));
			}
#else /* NEWAUTOCOMP */
			for (; *bufp; ++bufp) putsyms(T(" "));
			for (; bufp != obufp; --bufp) putsyms(T("\b \b"));
			*bufp = 0;
#endif /* NEWAUTOCOMP */
		} else
			tty_nhbell();
	}
	ttyDisplay->toplin = 2;		/* nonempty, no --More-- required */
	ttyDisplay->inread--;
	clear_nhwindow(WIN_MESSAGE);	/* clean up after ourselves */
}

#undef T

void
xwaitforspace(s)
register const char *s;	/* chars allowed besides return */
{
    register int c, x = ttyDisplay ? (int) ttyDisplay->dismiss_more : '\n';

    morc = 0;

    while((c = tty_nhgetch()) != '\n') {
	if(iflags.cbreak) {
	    if (c == EOF || c == '\033') {
		ttyDisplay->dismiss_more = 1;
		morc = '\033';
		break;
	    }
	    if ((s && index(s,c)) || c == x) {
		morc = (char) c;
		break;
	    }
	    tty_nhbell();
	}
    }

}

/*
 * Implement extended command completion by using this hook into
 * tty_getlin.  Check the characters already typed, if they uniquely
 * identify an extended command, expand the string to the whole
 * command.
 *
 * Return TRUE if we've extended the string at base.  Otherwise return FALSE.
 * Assumptions:
 *
 *	+ we don't change the characters that are already in base
 *	+ base has enough room to hold our string
 */
STATIC_OVL boolean
ext_cmd_getlin_hook(base)
	char *base;
{
	int oindex, com_index;

	com_index = -1;
	for (oindex = 0; extcmdlist[oindex].ef_txt != (char *)0; oindex++) {
		if (!strncmpi(base, extcmdlist[oindex].ef_txt, strlen(base))) {
			if (com_index == -1)	/* no matches yet */
			    com_index = oindex;
			else			/* more than 1 match */
			    return FALSE;
		}
	}
	if (com_index >= 0) {
		Strcpy(base, extcmdlist[com_index].ef_txt);
		return TRUE;
	}

	return FALSE;	/* didn't match anything */
}

/*
 * Read in an extended command, doing command line completion.  We
 * stop when we have found enough characters to make a unique command.
 */
int
tty_get_ext_cmd()
{
	int i;
	char buf[BUFSZ];

	if (iflags.extmenu) return extcmd_via_menu();
	/* maybe a runtime option? */
	/* hooked_tty_getlin("#", buf, flags.cmd_comp ? ext_cmd_getlin_hook : (getlin_hook_proc) 0); */
#ifdef REDO
	hooked_tty_getlin("#", buf, in_doagain ? (getlin_hook_proc)0
		: ext_cmd_getlin_hook);
#else
	hooked_tty_getlin("#", buf, ext_cmd_getlin_hook);
#endif
	(void) mungspaces(buf);
	if (buf[0] == 0 || buf[0] == '\033') return -1;

	for (i = 0; extcmdlist[i].ef_txt != (char *)0; i++)
		if (!strcmpi(buf, extcmdlist[i].ef_txt)) break;

#ifdef REDO
	if (!in_doagain) {
	    int j;
	    for (j = 0; buf[j]; j++)
		savech(buf[j]);
	    savech('\n');
	}
#endif

	if (extcmdlist[i].ef_txt == (char *)0) {
		pline("%s: unknown extended command.", buf);
		i = -1;
	}

	return i;
}

#endif /* TTY_GRAPHICS */

/*getline.c*/

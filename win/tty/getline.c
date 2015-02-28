/* NetHack 3.5	getline.c	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	getline.c	$Date: 2011/12/11 01:54:56 $  $Revision: 1.18 $ */
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

/* cloned from topl.c, but not identical
 */
#ifdef UNICODE_WIDEWINPORT
    /* nhwchar is wchar; data from core needs narrow-to-wide conversion;
       data going back to core needs wide-to-narrow conversion; data
       used within tty routines typically needs wide-to-wide awareness */
STATIC_VAR nhwchar	getl_wbuf[BUFSZ];
STATIC_VAR char		getl_nbuf[BUFSZ];
#define T(x) L##x
#define Waddtopl(str)		addtopl(nhwstrcpy(getl_wbuf,str))
#define Wputsyms(str)		putsyms(nhwstrcpy(getl_wbuf,str))
#define NWstrcpy(wdst,src)	nhwstrcpy(wdst,src)	/* narrow-to-wide */
#else	/*!UNICODE_WIDEWINPORT*/
    /* nhwchar is char; no conversions needed */
#define T(x) x
#define Waddtopl(str)		addtopl(str)
#define Wputsyms(str)		putsyms(str)
#endif	/*?UNICODE_WIDEWINPORT*/

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

	if(ttyDisplay->toplin == 1 && !(cw->flags & WIN_STOP)) more();
	cw->flags &= ~WIN_STOP;
	ttyDisplay->toplin = 3; /* special prompt state */
	ttyDisplay->inread++;
	pline("%s ", query);
	*obufp = 0;
	for(;;) {
		(void) fflush(stdout);
#ifdef UNICODE_WIDEWINPORT
		Strcat(strcat(strcpy(getl_nbuf, query), " "), obufp);
		(void)NWstrcpy(toplines, getl_nbuf);
#else
		Strcat(strcat(strcpy(toplines, query), " "), obufp);
#endif
		c = pgetchar();
		if (c == '\033' || c == EOF) {
		    obufp[0] = '\033';
		    obufp[1] = '\0';
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
			Waddtopl(query);
			Waddtopl(T(" "));
			*bufp = 0;
			Waddtopl(obufp);
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
		    Waddtopl(query);
		    Waddtopl(T(" "));
		    *bufp = 0;
		    Waddtopl(obufp);
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
			Wputsyms(bufp);
			bufp++;
			if (hook && (*hook)(obufp)) {
			    Wputsyms(bufp);
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
    while (
#ifdef HANGUPHANDLING
	   !program_state.done_hup &&
#endif
	   (c = tty_nhgetch()) != EOF) {
	if (c == '\n') break;

	if(iflags.cbreak) {
	    if (c == '\033') {
		if (ttyDisplay) ttyDisplay->dismiss_more = 1;
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
	hooked_tty_getlin("#", buf, in_doagain ? (getlin_hook_proc)0
		: ext_cmd_getlin_hook);
	(void) mungspaces(buf);
	if (buf[0] == 0 || buf[0] == '\033') return -1;

	for (i = 0; extcmdlist[i].ef_txt != (char *)0; i++)
		if (!strcmpi(buf, extcmdlist[i].ef_txt)) break;

	if (!in_doagain) {
	    int j;
	    for (j = 0; buf[j]; j++)
		savech(buf[j]);
	    savech('\n');
	}

	if (extcmdlist[i].ef_txt == (char *)0) {
		pline("%s: unknown extended command.", buf);
		i = -1;
	}

	return i;
}

#endif /* TTY_GRAPHICS */

/*getline.c*/

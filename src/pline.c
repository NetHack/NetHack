/*	SCCS Id: @(#)pline.c	3.4	1999/11/28	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS /* Uses ... */	/* comment line for pre-compiled headers */
#include "hack.h"
#include "epri.h"
#ifdef WIZARD
#include "edog.h"
#endif

#ifdef OVLB

static boolean no_repeat = FALSE;

static char *FDECL(You_buf, (int));

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(vpline, (const char *, va_list));

void
pline VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, char *);
	vpline(line, VA_ARGS);
	VA_END();
}

# ifdef USE_STDARG
static void
vpline(const char *line, va_list the_args) {
# else
static void
vpline(line, the_args) const char *line; va_list the_args; {
# endif

#else	/* USE_STDARG | USE_VARARG */

#define vpline pline

void
pline VA_DECL(const char *, line)
#endif	/* USE_STDARG | USE_VARARG */

	char pbuf[BUFSZ];
/* Do NOT use VA_START and VA_END in here... see above */

	if (!line || !*line) return;
	if (index(line, '%')) {
	    Vsprintf(pbuf,line,VA_ARGS);
	    line = pbuf;
	}
	if (!iflags.window_inited) {
	    raw_print(line);
	    return;
	}
#ifndef MAC
	if (no_repeat && !strcmp(line, toplines))
	    return;
#endif /* MAC */
	if (vision_full_recalc) vision_recalc(0);
	if (u.ux) flush_screen(1);		/* %% */
	putstr(WIN_MESSAGE, 0, line);
}

/*VARARGS1*/
void
Norep VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, const char *);
	no_repeat = TRUE;
	vpline(line, VA_ARGS);
	no_repeat = FALSE;
	VA_END();
	return;
}

/* work buffer for You(), &c and verbalize() */
static char *you_buf = 0;
static int you_buf_siz = 0;

static char *
You_buf(siz)
int siz;
{
	if (siz > you_buf_siz) {
		if (you_buf) free((genericptr_t) you_buf);
		you_buf_siz = siz + 10;
		you_buf = (char *) alloc((unsigned) you_buf_siz);
	}
	return you_buf;
}

void
free_youbuf()
{
	if (you_buf) free((genericptr_t) you_buf),  you_buf = (char *)0;
	you_buf_siz = 0;
}

/* `prefix' must be a string literal, not a pointer */
#define YouPrefix(pointer,prefix,text) \
 Strcpy((pointer = You_buf((int)(strlen(text) + sizeof prefix))), prefix)

#define YouMessage(pointer,prefix,text) \
 strcat((YouPrefix(pointer, prefix, text), pointer), text)

/*VARARGS1*/
void
You VA_DECL(const char *, line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "You ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
Your VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "Your ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
You_feel VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "You feel ", line), VA_ARGS);
	VA_END();
}


/*VARARGS1*/
void
You_cant VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "You can't ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
pline_The VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "The ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
There VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "There ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
You_hear VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	if (Underwater)
		YouPrefix(tmp, "You barely hear ", line);
	else if (u.usleep)
		YouPrefix(tmp, "You dream that you hear ", line);
	else
		YouPrefix(tmp, "You hear ", line);
	vpline(strcat(tmp, line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
verbalize VA_DECL(const char *,line)
	char *tmp;
	if (!flags.soundok) return;
	VA_START(line);
	VA_INIT(line, const char *);
	tmp = You_buf((int)strlen(line) + sizeof "\"\"");
	Strcpy(tmp, "\"");
	Strcat(tmp, line);
	Strcat(tmp, "\"");
	vpline(tmp, VA_ARGS);
	VA_END();
}

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(vraw_printf,(const char *,va_list));

void
raw_printf VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, char *);
	vraw_printf(line, VA_ARGS);
	VA_END();
}

# ifdef USE_STDARG
static void
vraw_printf(const char *line, va_list the_args) {
# else
static void
vraw_printf(line, the_args) const char *line; va_list the_args; {
# endif

#else  /* USE_STDARG | USE_VARARG */

void
raw_printf VA_DECL(const char *, line)
#endif
/* Do NOT use VA_START and VA_END in here... see above */

	if(!index(line, '%'))
	    raw_print(line);
	else {
	    char pbuf[BUFSZ];
	    Vsprintf(pbuf,line,VA_ARGS);
	    raw_print(pbuf);
	}
}


/*VARARGS1*/
void
impossible VA_DECL(const char *, s)
	VA_START(s);
	VA_INIT(s, const char *);
	vpline(s,VA_ARGS);
	pline("Program in disorder - perhaps you'd better #quit.");
	VA_END();
}

const char *
align_str(alignment)
    aligntyp alignment;
{
    switch ((int)alignment) {
	case A_CHAOTIC: return "chaotic";
	case A_NEUTRAL: return "neutral";
	case A_LAWFUL:	return "lawful";
	case A_NONE:	return "unaligned";
    }
    return "unknown";
}

void
mstatusline(mtmp)
register struct monst *mtmp;
{
	aligntyp alignment;
	char info[BUFSZ], monnambuf[BUFSZ];

	if (mtmp->ispriest || mtmp->data == &mons[PM_ALIGNED_PRIEST]
				|| mtmp->data == &mons[PM_ANGEL])
		alignment = EPRI(mtmp)->shralign;
	else
		alignment = mtmp->data->maligntyp;
	alignment = (alignment > 0) ? A_LAWFUL :
		(alignment < 0) ? A_CHAOTIC :
		A_NEUTRAL;

	info[0] = 0;
	if (mtmp->mtame) {	  Strcat(info, ", tame");
#ifdef WIZARD
	    if (wizard) {
		Sprintf(eos(info), " (%d", mtmp->mtame);
		if (!mtmp->isminion)
		    Sprintf(eos(info), "; hungry %ld; apport %d",
			EDOG(mtmp)->hungrytime, EDOG(mtmp)->apport);
		Strcat(info, ")");
	    }
#endif
	}
	else if (mtmp->mpeaceful) Strcat(info, ", peaceful");
	if (mtmp->meating)	  Strcat(info, ", eating");
	if (mtmp->mcan)		  Strcat(info, ", cancelled");
	if (mtmp->mconf)	  Strcat(info, ", confused");
	if (mtmp->mblinded || !mtmp->mcansee)
				  Strcat(info, ", blind");
	if (mtmp->mstun)	  Strcat(info, ", stunned");
	if (mtmp->msleeping)	  Strcat(info, ", asleep");
#if 0	/* unfortunately mfrozen covers temporary sleep and being busy
	   (donning armor, for instance) as well as paralysis */
	else if (mtmp->mfrozen)	  Strcat(info, ", paralyzed");
#else
	else if (mtmp->mfrozen || !mtmp->mcanmove)
				  Strcat(info, ", can't move");
#endif
				  /* [arbitrary reason why it isn't moving] */
	else if (mtmp->mstrategy & STRAT_WAITMASK)
				  Strcat(info, ", meditating");
	else if (mtmp->mflee)	  Strcat(info, ", scared");
	if (mtmp->mtrapped)	  Strcat(info, ", trapped");
	if (mtmp->mspeed)	  Strcat(info,
					mtmp->mspeed == MFAST ? ", fast" :
					mtmp->mspeed == MSLOW ? ", slow" :
					", ???? speed");
	if (mtmp->mundetected)	  Strcat(info, ", concealed");
	if (mtmp->minvis)	  Strcat(info, ", invisible");
	if (mtmp == u.ustuck)	  Strcat(info,
			(sticks(youmonst.data)) ? ", held by you" :
				u.uswallow ? (is_animal(u.ustuck->data) ?
				", swallowed you" :
				", engulfed you") :
				", holding you");
#ifdef STEED
	if (mtmp == u.usteed)	  Strcat(info, ", carrying you");
#endif

	/* avoid "Status of the invisible newt ..., invisible" */
	/* and unlike a normal mon_nam, use "saddled" even if it has a name */
	Strcpy(monnambuf, x_monnam(mtmp, ARTICLE_THE, (char *)0,
	    (SUPPRESS_IT|SUPPRESS_INVISIBLE), FALSE));

	pline("Status of %s (%s):  Level %d  HP %d(%d)  AC %d%s.",
		monnambuf,
		align_str(alignment),
		mtmp->m_lev,
		mtmp->mhp,
		mtmp->mhpmax,
		find_mac(mtmp),
		info);
}

void
ustatusline()
{
	char info[BUFSZ];

	info[0] = '\0';
	if (Sick) {
		Strcat(info, ", dying from");
		if (u.usick_type & SICK_VOMITABLE)
			Strcat(info, " food poisoning");
		if (u.usick_type & SICK_NONVOMITABLE) {
			if (u.usick_type & SICK_VOMITABLE)
				Strcat(info, " and");
			Strcat(info, " illness");
		}
	}
	if (Stoned)		Strcat(info, ", solidifying");
	if (Slimed)		Strcat(info, ", becoming slimy");
	if (Strangled)		Strcat(info, ", being strangled");
	if (Vomiting)		Strcat(info, ", nauseated"); /* !"nauseous" */
	if (Confusion)		Strcat(info, ", confused");
	if (Blind) {
	    Strcat(info, ", blind");
	    if (u.ucreamed) {
		if ((long)u.ucreamed < Blinded || Blindfolded
						|| !haseyes(youmonst.data))
		    Strcat(info, ", cover");
		Strcat(info, "ed by sticky goop");
	    }	/* note: "goop" == "glop"; variation is intentional */
	}
	if (Stunned)		Strcat(info, ", stunned");
#ifdef STEED
	if (!u.usteed)
#endif
	if (Wounded_legs) {
	    const char *what = body_part(LEG);
	    if ((Wounded_legs & BOTH_SIDES) == BOTH_SIDES)
		what = makeplural(what);
				Sprintf(eos(info), ", injured %s", what);
	}
	if (Glib)		Sprintf(eos(info), ", slippery %s",
					makeplural(body_part(HAND)));
	if (u.utrap)		Strcat(info, ", trapped");
	if (Fast)		Strcat(info, Very_fast ?
						", very fast" : ", fast");
	if (u.uundetected)	Strcat(info, ", concealed");
	if (Invis)		Strcat(info, ", invisible");
	if (u.ustuck) {
	    if (sticks(youmonst.data))
		Strcat(info, ", holding ");
	    else
		Strcat(info, ", held by ");
	    Strcat(info, mon_nam(u.ustuck));
	}

	pline("Status of %s (%s%s):  Level %d  HP %d(%d)  AC %d%s.",
		plname,
		    (u.ualign.record >= 20) ? "piously " :
		    (u.ualign.record > 13) ? "devoutly " :
		    (u.ualign.record > 8) ? "fervently " :
		    (u.ualign.record > 3) ? "stridently " :
		    (u.ualign.record == 3) ? "" :
		    (u.ualign.record >= 1) ? "haltingly " :
		    (u.ualign.record == 0) ? "nominally " :
					    "insufficiently ",
		align_str(u.ualign.type),
		Upolyd ? mons[u.umonnum].mlevel : u.ulevel,
		Upolyd ? u.mh : u.uhp,
		Upolyd ? u.mhmax : u.uhpmax,
		u.uac,
		info);
}

void
self_invis_message()
{
	pline("%s %s.",
	    Hallucination ? "Far out, man!  You" : "Gee!  All of a sudden, you",
	    See_invisible ? "can see right through yourself" :
		"can't see yourself");
}

#endif /* OVLB */
/*pline.c*/

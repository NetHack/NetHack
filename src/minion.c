/*	SCCS Id: @(#)minion.c	3.4	2002/01/23	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "emin.h"
#include "epri.h"

void
msummon(ptr)		/* ptr summons a monster */
register struct permonst *ptr;
{
	register int dtype = NON_PM, cnt = 0;
	aligntyp atyp = (ptr->maligntyp==A_NONE) ? A_NONE : sgn(ptr->maligntyp);

	if (is_dprince(ptr) || (ptr == &mons[PM_WIZARD_OF_YENDOR])) {
	    dtype = (!rn2(20)) ? dprince(atyp) :
				 (!rn2(4)) ? dlord(atyp) : ndemon(atyp);
	    cnt = (!rn2(4) && is_ndemon(&mons[dtype])) ? 2 : 1;
	} else if (is_dlord(ptr)) {
	    dtype = (!rn2(50)) ? dprince(atyp) :
				 (!rn2(20)) ? dlord(atyp) : ndemon(atyp);
	    cnt = (!rn2(4) && is_ndemon(&mons[dtype])) ? 2 : 1;
	} else if (is_ndemon(ptr)) {
	    dtype = (!rn2(20)) ? dlord(atyp) :
				 (!rn2(6)) ? ndemon(atyp) : monsndx(ptr);
	    cnt = 1;
	} else if (is_lminion(ptr)) {
	    dtype = (is_lord(ptr) && !rn2(20)) ? llord() :
		     (is_lord(ptr) || !rn2(6)) ? lminion() : monsndx(ptr);
	    cnt = (!rn2(4) && !is_lord(&mons[dtype])) ? 2 : 1;
	}

	if (dtype == NON_PM) return;

	/* sanity checks */
	if (cnt > 1 && (mons[dtype].geno & G_UNIQ)) cnt = 1;
	/*
	 * If this daemon is unique and being re-summoned (the only way we
	 * could get this far with an extinct dtype), try another.
	 */
	if (mvitals[dtype].mvflags & G_GONE) {
	    dtype = ndemon(atyp);
	    if (dtype == NON_PM) return;
	}

	while (cnt > 0) {
	    (void)makemon(&mons[dtype], u.ux, u.uy, NO_MM_FLAGS);
	    cnt--;
	}
	return;
}

void
summon_minion(alignment, talk)
aligntyp alignment;
boolean talk;
{
    register struct monst *mon;
    int mnum;

    switch ((int)alignment) {
	case A_LAWFUL:
	    mnum = lminion();
	    break;
	case A_NEUTRAL:
	    mnum = PM_AIR_ELEMENTAL + rn2(4);
	    break;
	case A_CHAOTIC:
	case A_NONE:
	    mnum = ndemon(alignment);
	    break;
	default:
	    impossible("unaligned player?");
	    mnum = ndemon(A_NONE);
	    break;
    }
    if (mnum == NON_PM) {
	mon = 0;
    } else if (mons[mnum].pxlth == 0) {
	struct permonst *pm = &mons[mnum];
	mon = makemon(pm, u.ux, u.uy, MM_EMIN);
	if (mon) {
	    mon->isminion = TRUE;
	    EMIN(mon)->min_align = alignment;
	}
    } else if (mnum == PM_ANGEL) {
	mon = makemon(&mons[mnum], u.ux, u.uy, NO_MM_FLAGS);
	if (mon) {
	    mon->isminion = TRUE;
	    EPRI(mon)->shralign = alignment;	/* always A_LAWFUL here */
	}
    } else
	mon = makemon(&mons[mnum], u.ux, u.uy, NO_MM_FLAGS);
    if (mon) {
	if (talk) {
	    pline_The("voice of %s booms:", align_gname(alignment));
	    verbalize("Thou shalt pay for thy indiscretion!");
	    if (!Blind)
		pline("%s appears before you.", Amonnam(mon));
	}
	mon->mpeaceful = FALSE;
	/* don't call set_malign(); player was naughty */
    }
}

#define Athome	(Inhell && !mtmp->cham)

int
demon_talk(mtmp)		/* returns 1 if it won't attack. */
register struct monst *mtmp;
{
	long	demand, offer;

	if (uwep && uwep->oartifact == ART_EXCALIBUR) {
	    pline("%s looks very angry.", Amonnam(mtmp));
	    mtmp->mpeaceful = mtmp->mtame = 0;
	    newsym(mtmp->mx, mtmp->my);
	    return 0;
	}

	/* Slight advantage given. */
	if (is_dprince(mtmp->data) && mtmp->minvis) {
	    mtmp->minvis = mtmp->perminvis = 0;
	    if (!Blind) pline("%s appears before you.", Amonnam(mtmp));
	    newsym(mtmp->mx,mtmp->my);
	}
	if (youmonst.data->mlet == S_DEMON) {	/* Won't blackmail their own. */
	    pline("%s says, \"Good hunting, %s.\"",
		  Amonnam(mtmp), flags.female ? "Sister" : "Brother");
	    if (!tele_restrict(mtmp)) rloc(mtmp);
	    return(1);
	}
#ifndef GOLDOBJ
	demand = (u.ugold * (rnd(80) + 20 * Athome)) /
#else
	demand = (money_cnt(invent) * (rnd(80) + 20 * Athome)) /
#endif
	    (100 * (1 + (sgn(u.ualign.type) == sgn(mtmp->data->maligntyp))));
	if (!demand)		/* you have no gold */
	    return mtmp->mpeaceful = 0;
	else {
	    pline("%s demands %ld %s for safe passage.",
		  Amonnam(mtmp), demand, currency(demand));

	    if ((offer = bribe(mtmp)) >= demand) {
		pline("%s vanishes, laughing about cowardly mortals.",
		      Amonnam(mtmp));
	    } else {
		if ((long)rnd(40) > (demand - offer)) {
		    pline("%s scowls at you menacingly, then vanishes.",
			  Amonnam(mtmp));
		} else {
		    pline("%s gets angry...", Amonnam(mtmp));
		    return mtmp->mpeaceful = 0;
		}
	    }
	}
	mongone(mtmp);
	return(1);
}

long
bribe(mtmp)
struct monst *mtmp;
{
	char buf[BUFSZ];
	long offer;
#ifdef GOLDOBJ
	long umoney = money_cnt(invent);
#endif

	getlin("How much will you offer?", buf);
	if (sscanf(buf, "%ld", &offer) != 1) offer = 0L;

	/*Michael Paddon -- fix for negative offer to monster*/
	/*JAR880815 - */
	if (offer < 0L) {
		You("try to shortchange %s, but fumble.",
			mon_nam(mtmp));
		return 0L;
	} else if (offer == 0L) {
		You("refuse.");
		return 0L;
#ifndef GOLDOBJ
	} else if (offer >= u.ugold) {
		You("give %s all your gold.", mon_nam(mtmp));
		offer = u.ugold;
	} else {
		You("give %s %ld %s.", mon_nam(mtmp), offer, currency(offer));
	}
	u.ugold -= offer;
	mtmp->mgold += offer;
#else
	} else if (offer >= umoney) {
		You("give %s all your gold.", mon_nam(mtmp));
		offer = umoney;
	} else {
		You("give %s %ld %s.", mon_nam(mtmp), offer, currency(offer));
	}
	(void) money2mon(mtmp, offer);
#endif
	flags.botl = 1;
	return(offer);
}

int
dprince(atyp)
aligntyp atyp;
{
	int tryct, pm;

	for (tryct = 0; tryct < 20; tryct++) {
	    pm = rn1(PM_DEMOGORGON + 1 - PM_ORCUS, PM_ORCUS);
	    if (!(mvitals[pm].mvflags & G_GONE) &&
		    (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
		return(pm);
	}
	return(dlord(atyp));	/* approximate */
}

int
dlord(atyp)
aligntyp atyp;
{
	int tryct, pm;

	for (tryct = 0; tryct < 20; tryct++) {
	    pm = rn1(PM_YEENOGHU + 1 - PM_JUIBLEX, PM_JUIBLEX);
	    if (!(mvitals[pm].mvflags & G_GONE) &&
		    (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
		return(pm);
	}
	return(ndemon(atyp));	/* approximate */
}

/* create lawful (good) lord */
int
llord()
{
	if (!(mvitals[PM_ARCHON].mvflags & G_GONE))
		return(PM_ARCHON);

	return(lminion());	/* approximate */
}

int
lminion()
{
	int	tryct;
	struct	permonst *ptr;

	for (tryct = 0; tryct < 20; tryct++) {
	    ptr = mkclass(S_ANGEL,0);
	    if (ptr && !is_lord(ptr))
		return(monsndx(ptr));
	}

	return NON_PM;
}

int
ndemon(atyp)
aligntyp atyp;
{
	int	tryct;
	struct	permonst *ptr;

	for (tryct = 0; tryct < 20; tryct++) {
	    ptr = mkclass(S_DEMON, 0);
	    if (ptr && is_ndemon(ptr) &&
		    (atyp == A_NONE || sgn(ptr->maligntyp) == sgn(atyp)))
		return(monsndx(ptr));
	}

	return NON_PM;
}

/*minion.c*/

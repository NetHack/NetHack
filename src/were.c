/*	SCCS Id: @(#)were.c	3.4	1997/05/25	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef OVL0

void
were_change(mon)
register struct monst *mon;
{
	if (!is_were(mon->data))
	    return;

	if (is_human(mon->data)) {
	    if (!Protection_from_shape_changers &&
		    !rn2(night() ? (flags.moonphase == FULL_MOON ?  3 : 30)
				 : (flags.moonphase == FULL_MOON ? 10 : 50))) {
		new_were(mon);		/* change into animal form */
		if (flags.soundok) {
		    const char *howler;

		    switch (monsndx(mon->data)) {
		    case PM_HUMAN_WEREWOLF:	  howler = "wolf";    break;
		    case PM_HUMAN_WEREJACKAL: howler = "jackal";  break;
		    default:		  howler = (char *)0; break;
		    }
		    if (howler)
			You_hear("a %s howling at the moon.", howler);
		}
	    }
	} else if (!rn2(30) || Protection_from_shape_changers) {
	    new_were(mon);		/* change back into human form */
	}
}

#endif /* OVL0 */
#ifdef OVLB

STATIC_DCL int FDECL(counter_were,(int));

STATIC_OVL int
counter_were(pm)
int pm;
{
	switch(pm) {
	    case PM_WEREWOLF:	      return(PM_HUMAN_WEREWOLF);
	    case PM_HUMAN_WEREWOLF:   return(PM_WEREWOLF);
	    case PM_WEREJACKAL:	      return(PM_HUMAN_WEREJACKAL);
	    case PM_HUMAN_WEREJACKAL: return(PM_WEREJACKAL);
	    case PM_WERERAT:	      return(PM_HUMAN_WERERAT);
	    case PM_HUMAN_WERERAT:    return(PM_WERERAT);
	    default:		      return(0);
	}
}

void
new_were(mon)
register struct monst *mon;
{
	register int pm;

	pm = counter_were(monsndx(mon->data));
	if(!pm) {
	    impossible("unknown lycanthrope %s.", mon->data->mname);
	    return;
	}

	if(canseemon(mon) && !Hallucination)
	    pline("%s changes into a %s.", Monnam(mon),
			is_human(&mons[pm]) ? "human" :
			mons[pm].mname+4);

	set_mon_data(mon, &mons[pm], 0);
	if (mon->msleeping || !mon->mcanmove) {
	    /* transformation wakens and/or revitalizes */
	    mon->msleeping = 0;
	    mon->mfrozen = 0;	/* not asleep or paralyzed */
	    mon->mcanmove = 1;
	}
	/* regenerate by 1/4 of the lost hit points */
	mon->mhp += (mon->mhpmax - mon->mhp) / 4;
	newsym(mon->mx,mon->my);
	mon_break_armor(mon, FALSE);
	possibly_unwield(mon);
}

boolean
were_summon(ptr,yours)	/* were-creature (even you) summons a horde */
register struct permonst *ptr;
register boolean yours;
{
	register int i, typ, pm = monsndx(ptr);
	register struct monst *mtmp;
	boolean success = FALSE;

	if(Protection_from_shape_changers && !yours)
		return FALSE;
	for(i = rnd(5); i > 0; i--) {
	   switch(pm) {

		case PM_WERERAT:
		case PM_HUMAN_WERERAT:
			typ = rn2(3) ? PM_SEWER_RAT : rn2(3) ? PM_GIANT_RAT : PM_RABID_RAT ;
			break;
		case PM_WEREJACKAL:
		case PM_HUMAN_WEREJACKAL:
			typ = PM_JACKAL;
			break;
		case PM_WEREWOLF:
		case PM_HUMAN_WEREWOLF:
			typ = rn2(5) ? PM_WOLF : PM_WINTER_WOLF ;
			break;
		default:
			continue;
	    }
	    mtmp = makemon(&mons[typ], u.ux, u.uy, NO_MM_FLAGS);
	    if (mtmp) success = TRUE;
	    if (yours && mtmp)
		(void) tamedog(mtmp, (struct obj *) 0);
	}
	return success;
}

void
you_were()
{
	char qbuf[QBUFSZ];

	if (Unchanging || (u.umonnum == u.ulycn)) return;
	if (Polymorph_control) {
	    /* `+4' => skip "were" prefix to get name of beast */
	    Sprintf(qbuf, "Do you want to change into %s? ",
		    an(mons[u.ulycn].mname+4));
	    if(yn(qbuf) == 'n') return;
	}
	(void) polymon(u.ulycn);
}

void
you_unwere(purify)
boolean purify;
{
	if (purify) {
	    You_feel("purified.");
	    u.ulycn = NON_PM;	/* cure lycanthropy */
	}
	if (!Unchanging && is_were(youmonst.data) &&
		(!Polymorph_control || yn("Remain in beast form?") == 'n'))
	    rehumanize();
}

#endif /* OVLB */

/*were.c*/

/*	SCCS Id: @(#)worn.c	3.4	2002/02/07	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL void FDECL(m_lose_armor, (struct monst *,struct obj *));
STATIC_DCL void FDECL(m_dowear_type, (struct monst *,long,BOOLEAN_P));
STATIC_DCL int FDECL(extra_pref, (struct monst *, struct obj *));

const struct worn {
	long w_mask;
	struct obj **w_obj;
} worn[] = {
	{ W_ARM, &uarm },
	{ W_ARMC, &uarmc },
	{ W_ARMH, &uarmh },
	{ W_ARMS, &uarms },
	{ W_ARMG, &uarmg },
	{ W_ARMF, &uarmf },
#ifdef TOURIST
	{ W_ARMU, &uarmu },
#endif
	{ W_RINGL, &uleft },
	{ W_RINGR, &uright },
	{ W_WEP, &uwep },
	{ W_SWAPWEP, &uswapwep },
	{ W_QUIVER, &uquiver },
	{ W_AMUL, &uamul },
	{ W_TOOL, &ublindf },
	{ W_BALL, &uball },
	{ W_CHAIN, &uchain },
	{ 0, 0 }
};

/* This only allows for one blocking item per property */
#define w_blocks(o,m) \
		((o->otyp == MUMMY_WRAPPING && ((m) & W_ARMC)) ? INVIS : \
		 (o->otyp == CORNUTHAUM && ((m) & W_ARMH) && \
			!Role_if(PM_WIZARD)) ? CLAIRVOYANT : 0)
		/* note: monsters don't have clairvoyance, so your role
		   has no significant effect on their use of w_blocks() */


/* Updated to use the extrinsic and blocked fields. */
void
setworn(obj, mask)
register struct obj *obj;
long mask;
{
	register const struct worn *wp;
	register struct obj *oobj;
	register int p;

	if ((mask & (W_ARM|I_SPECIAL)) == (W_ARM|I_SPECIAL)) {
	    /* restoring saved game; no properties are conferred via skin */
	    uskin = obj;
	 /* assert( !uarm ); */
	} else {
	    for(wp = worn; wp->w_mask; wp++) if(wp->w_mask & mask) {
		oobj = *(wp->w_obj);
		if(oobj && !(oobj->owornmask & wp->w_mask))
			impossible("Setworn: mask = %ld.", wp->w_mask);
		if(oobj) {
		    if (u.twoweap && (oobj->owornmask & (W_WEP|W_SWAPWEP)))
			u.twoweap = 0;
		    oobj->owornmask &= ~wp->w_mask;
		    if (wp->w_mask & ~(W_SWAPWEP|W_QUIVER)) {
			/* leave as "x = x <op> y", here and below, for broken
			 * compilers */
			p = objects[oobj->otyp].oc_oprop;
			u.uprops[p].extrinsic =
					u.uprops[p].extrinsic & ~wp->w_mask;
			if ((p = w_blocks(oobj,mask)) != 0)
			    u.uprops[p].blocked &= ~wp->w_mask;
			if (oobj->oartifact)
			    set_artifact_intrinsic(oobj, 0, mask);
		    }
		}
		*(wp->w_obj) = obj;
		if(obj) {
		    obj->owornmask |= wp->w_mask;
		    /* Prevent getting/blocking intrinsics from wielding
		     * potions, through the quiver, etc.
		     * Allow weapon-tools, too.
		     * wp_mask should be same as mask at this point.
		     */
		    if (wp->w_mask & ~(W_SWAPWEP|W_QUIVER)) {
			if (obj->oclass == WEAPON_CLASS || is_weptool(obj) ||
					    mask != W_WEP) {
			    p = objects[obj->otyp].oc_oprop;
			    u.uprops[p].extrinsic =
					u.uprops[p].extrinsic | wp->w_mask;
			    if ((p = w_blocks(obj, mask)) != 0)
				u.uprops[p].blocked |= wp->w_mask;
			}
			if (obj->oartifact)
			    set_artifact_intrinsic(obj, 1, mask);
		    }
		}
	    }
	}
	update_inventory();
}

/* called e.g. when obj is destroyed */
/* Updated to use the extrinsic and blocked fields. */
void
setnotworn(obj)
register struct obj *obj;
{
	register const struct worn *wp;
	register int p;

	if (!obj) return;
	if (obj == uwep || obj == uswapwep) u.twoweap = 0;
	for(wp = worn; wp->w_mask; wp++)
	    if(obj == *(wp->w_obj)) {
		*(wp->w_obj) = 0;
		p = objects[obj->otyp].oc_oprop;
		u.uprops[p].extrinsic = u.uprops[p].extrinsic & ~wp->w_mask;
		obj->owornmask &= ~wp->w_mask;
		if (obj->oartifact)
		    set_artifact_intrinsic(obj, 0, wp->w_mask);
		if ((p = w_blocks(obj,wp->w_mask)) != 0)
		    u.uprops[p].blocked &= ~wp->w_mask;
	    }
	update_inventory();
}

void
mon_set_minvis(mon)
struct monst *mon;
{
	mon->perminvis = 1;
	if (!mon->invis_blkd) {
	    mon->minvis = 1;
	    newsym(mon->mx, mon->my);		/* make it disappear */
	    if (mon->wormno) see_wsegs(mon);	/* and any tail too */
	}
}

void
mon_adjust_speed(mon, adjust, obj)
struct monst *mon;
int adjust;	/* positive => increase speed, negative => decrease */
struct obj *obj;	/* item to make known if effect can be seen */
{
    struct obj *otmp;
    boolean give_msg = !in_mklev;
    unsigned int oldspeed = mon->mspeed;

    switch (adjust) {
     case  2:
	mon->permspeed = MFAST;
	give_msg = FALSE;	/* special case monster creation */
	break;
     case  1:
	if (mon->permspeed == MSLOW) mon->permspeed = 0;
	else mon->permspeed = MFAST;
	break;
     case  0:			/* just check for worn speed boots */
	break;
     case -1:
	if (mon->permspeed == MFAST) mon->permspeed = 0;
	else mon->permspeed = MSLOW;
	break;
     case -2:
	mon->permspeed = MSLOW;
	give_msg = FALSE;	/* (not currently used) */
	break;
    }

    for (otmp = mon->minvent; otmp; otmp = otmp->nobj)
	if (otmp->owornmask && objects[otmp->otyp].oc_oprop == FAST)
	    break;
    if (otmp)		/* speed boots */
	mon->mspeed = MFAST;
    else
	mon->mspeed = mon->permspeed;

    if (give_msg && mon->mspeed != oldspeed && canseemon(mon)) {
	/* fast to slow (skipping intermediate state) or vice versa */
	const char *howmuch = (mon->mspeed + oldspeed == MFAST + MSLOW) ?
				"much " : "";

	if (adjust > 0 || mon->mspeed == MFAST)
	    pline("%s is suddenly moving %sfaster.", Monnam(mon), howmuch);
	else
	    pline("%s seems to be moving %sslower.", Monnam(mon), howmuch);

	/* might discover an object if we see the speed change happen, but
	   avoid making possibly forgotten book known when casting its spell */
	if (obj != 0 && obj->dknown &&
		objects[obj->otyp].oc_class != SPBOOK_CLASS)
	    makeknown(obj->otyp);
    }
}

/* armor put on or taken off; might be magical variety */
void
update_mon_intrinsics(mon, obj, on)
struct monst *mon;
struct obj *obj;
boolean on;
{
    int unseen;
    uchar mask;
    struct obj *otmp;
    int which = (int) objects[obj->otyp].oc_oprop;

    unseen = !canseemon(mon);
    if (!which) goto maybe_blocks;

    if (on) {
	switch (which) {
	 case INVIS:
	    mon->minvis = !mon->invis_blkd;
	    break;
	 case FAST:
	    mon_adjust_speed(mon, 0, obj);
	    break;
	/* properties handled elsewhere */
	 case ANTIMAGIC:
	 case REFLECTING:
	    break;
	/* properties which have no effect for monsters */
	 case CLAIRVOYANT:
	 case STEALTH:
	 case TELEPAT:
	    break;
	/* properties which should have an effect but aren't implemented */
	 case LEVITATION:
	 case WWALKING:
	    break;
	/* properties which maybe should have an effect but don't */
	 case DISPLACED:
	 case FUMBLING:
	 case JUMPING:
	 case PROTECTION:
	    break;
	 default:
	    if (which <= 8) {	/* 1 thru 8 correspond to MR_xxx mask values */
		/* FIRE,COLD,SLEEP,DISINT,SHOCK,POISON,ACID,STONE */
		mask = (uchar) (1 << (which - 1));
		mon->mintrinsics |= (unsigned short) mask;
	    }
	    break;
	}
    } else {	    /* off */
	switch (which) {
	 case INVIS:
	    mon->minvis = mon->perminvis;
	    break;
	 case FAST:
	    mon_adjust_speed(mon, 0, obj);
	    break;
	 case FIRE_RES:
	 case COLD_RES:
	 case SLEEP_RES:
	 case DISINT_RES:
	 case SHOCK_RES:
	 case POISON_RES:
	 case ACID_RES:
	 case STONE_RES:
	    mask = (uchar) (1 << (which - 1));
	    /* If the monster doesn't have this resistance intrinsically,
	       check whether any other worn item confers it.  Note that
	       we don't currently check for anything conferred via simply
	       carrying an object. */
	    if (!(mon->data->mresists & mask)) {
		for (otmp = mon->minvent; otmp; otmp = otmp->nobj)
		    if (otmp->owornmask &&
			    (int) objects[otmp->otyp].oc_oprop == which)
			break;
		if (!otmp)
		    mon->mintrinsics &= ~((unsigned short) mask);
	    }
	    break;
	 default:
	    break;
	}
    }

 maybe_blocks:
    /* obj->owornmask has been cleared by this point, so we can't use it.
       However, since monsters don't wield armor, we don't have to guard
       against that and can get away with a blanket worn-mask value. */
    switch (w_blocks(obj,~0L)) {
     case INVIS:
	mon->invis_blkd = on ? 1 : 0;
	mon->minvis = on ? 0 : mon->perminvis;
	break;
     default:
	break;
    }

#ifdef STEED
	if (!on && mon == u.usteed && obj->otyp == SADDLE)
	    dismount_steed(DISMOUNT_FELL);
#endif

    /* if couldn't see it but now can, or vice versa, update display */
    if (unseen ^ !canseemon(mon))
	newsym(mon->mx, mon->my);
}

int
find_mac(mon)
register struct monst *mon;
{
	register struct obj *obj;
	int base = mon->data->ac;
	long mwflags = mon->misc_worn_check;

	for (obj = mon->minvent; obj; obj = obj->nobj) {
	    if (obj->owornmask & mwflags)
		base -= ARM_BONUS(obj);
		/* since ARM_BONUS is positive, subtracting it increases AC */
	}
	return base;
}

/* weapons are handled separately; rings and eyewear aren't used by monsters */

/* Wear the best object of each type that the monster has.  During creation,
 * the monster can put everything on at once; otherwise, wearing takes time.
 * This doesn't affect monster searching for objects--a monster may very well
 * search for objects it would not want to wear, because we don't want to
 * check which_armor() each round.
 *
 * We'll let monsters put on shirts and/or suits under worn cloaks, but
 * not shirts under worn suits.  This is somewhat arbitrary, but it's
 * too tedious to have them remove and later replace outer garments,
 * and preventing suits under cloaks makes it a little bit too easy for
 * players to influence what gets worn.  Putting on a shirt underneath
 * already worn body armor is too obviously buggy...
 */
void
m_dowear(mon, creation)
register struct monst *mon;
boolean creation;
{
	/* Note the restrictions here are the same as in dowear in do_wear.c
	 * except for the additional restriction on intelligence.  (Players
	 * are always intelligent, even if polymorphed).
	 */
	if (verysmall(mon->data) || nohands(mon->data) || is_animal(mon->data))
		return;
	/* give mummies a chance to wear their wrappings */
	if (mindless(mon->data) && (mon->data->mlet != S_MUMMY || !creation))
		return;

	m_dowear_type(mon, W_AMUL, creation);
#ifdef TOURIST
	/* can't put on shirt if already wearing suit */
	if (!cantweararm(mon->data) || (mon->misc_worn_check & W_ARM))
	    m_dowear_type(mon, W_ARMU, creation);
#endif
	/* treating small as a special case allows
	   hobbits, gnomes, and kobolds to wear cloaks */
	if (!cantweararm(mon->data) || mon->data->msize == MZ_SMALL)
	    m_dowear_type(mon, W_ARMC, creation);
	m_dowear_type(mon, W_ARMH, creation);
	if (!MON_WEP(mon) || !bimanual(MON_WEP(mon)))
	    m_dowear_type(mon, W_ARMS, creation);
	m_dowear_type(mon, W_ARMG, creation);
	if (!slithy(mon->data) && mon->data->mlet != S_CENTAUR)
	    m_dowear_type(mon, W_ARMF, creation);
	if (!cantweararm(mon->data))
	    m_dowear_type(mon, W_ARM, creation);
}

STATIC_OVL void
m_dowear_type(mon, flag, creation)
struct monst *mon;
long flag;
boolean creation;
{
	struct obj *old, *best, *obj;
	int m_delay = 0;

	if (mon->mfrozen) return; /* probably putting previous item on */

	old = which_armor(mon, flag);
	if (old && old->cursed) return;
	if (old && flag == W_AMUL) return; /* no such thing as better amulets */
	best = old;

	for(obj = mon->minvent; obj; obj = obj->nobj) {
	    switch(flag) {
		case W_AMUL:
		    if (obj->oclass != AMULET_CLASS ||
			    (obj->otyp != AMULET_OF_LIFE_SAVING &&
				obj->otyp != AMULET_OF_REFLECTION))
			continue;
		    best = obj;
		    goto outer_break; /* no such thing as better amulets */
#ifdef TOURIST
		case W_ARMU:
		    if (!is_shirt(obj)) continue;
		    break;
#endif
		case W_ARMC:
		    if (!is_cloak(obj)) continue;
		    break;
		case W_ARMH:
		    if (!is_helmet(obj)) continue;
		    break;
		case W_ARMS:
		    if (!is_shield(obj)) continue;
		    break;
		case W_ARMG:
		    if (!is_gloves(obj)) continue;
		    break;
		case W_ARMF:
		    if (!is_boots(obj)) continue;
		    break;
		case W_ARM:
		    if (!is_suit(obj)) continue;
		    break;
	    }
	    if (obj->owornmask) continue;
	    /* I'd like to define a VISIBLE_ARM_BONUS which doesn't assume the
	     * monster knows obj->spe, but if I did that, a monster would keep
	     * switching forever between two -2 caps since when it took off one
	     * it would forget spe and once again think the object is better
	     * than what it already has.
	     */
	    if (best && (ARM_BONUS(best) + extra_pref(mon,best) >= ARM_BONUS(obj) + extra_pref(mon,obj)))
		continue;
	    best = obj;
	}
outer_break:
	if (!best || best == old) return;

	/* if wearing a cloak, account for the time spent removing
	   and re-wearing it when putting on a suit or shirt */
	if ((flag == W_ARM
#ifdef TOURIST
	  || flag == W_ARMU
#endif
			  ) && (mon->misc_worn_check & W_ARMC))
	    m_delay += 2;
	/* when upgrading a piece of armor, account for time spent
	   taking off current one */
	if (old)
	    m_delay += objects[old->otyp].oc_delay;

	if (old) /* do this first to avoid "(being worn)" */
	    old->owornmask = 0L;
	if (!creation) {
	    if (canseemon(mon)) {
		char buf[BUFSZ];

		if (old)
		    Sprintf(buf, " removes %s and", distant_name(old, doname));
		else
		    buf[0] = '\0';
		pline("%s%s puts on %s.", Monnam(mon),
		      buf, distant_name(best,doname));
	    } /* can see it */
	    m_delay += objects[best->otyp].oc_delay;
	    mon->mfrozen = m_delay;
	    if (mon->mfrozen) mon->mcanmove = 0;
	}
	if (old)
	    update_mon_intrinsics(mon, old, FALSE);
	mon->misc_worn_check |= flag;
	best->owornmask |= flag;
	update_mon_intrinsics(mon, best, TRUE);
}

struct obj *
which_armor(mon, flag)
struct monst *mon;
long flag;
{
	register struct obj *obj;

	for(obj = mon->minvent; obj; obj = obj->nobj)
		if (obj->owornmask & flag) return obj;
	return((struct obj *)0);
}

/* remove an item of armor and then drop it */
STATIC_OVL void
m_lose_armor(mon, obj)
struct monst *mon;
struct obj *obj;
{
	mon->misc_worn_check &= ~obj->owornmask;
	obj->owornmask = 0L;
	update_mon_intrinsics(mon, obj, FALSE);

	obj_extract_self(obj);
	place_object(obj, mon->mx, mon->my);
	/* call stackobj() if we ever drop anything that can merge */
	newsym(mon->mx, mon->my);
}

void
clear_bypasses()
{
	struct obj *otmp, *nobj;

	for (otmp = fobj; otmp; otmp = nobj) {
	    nobj = otmp->nobj;
	    if (otmp->bypass) {
		otmp->bypass = 0;
#if 0
		/*  setting otmp->bypass changes mergability.
		 *  If monster can ever drop anything that
                 *  can and should merge, this code block
		 *  should be enabled.
		 */
		{
		    struct obj *obj;
		    xchar ox, oy;
		    (void) get_obj_location(otmp, &ox, &oy, 0);
		    obj_extract_self(otmp);
		    obj = merge_choice(fobj, otmp);
		    /* If it can't merge, then place it */
		    if (!obj || (obj && !merged(&obj, &otmp)))
		        place_object(otmp, ox, oy);
		    newsym(ox, oy);
		}
#endif
	    }
	}
	flags.bypasses = FALSE;
}

void
bypass_obj(obj)
struct obj *obj;
{
	obj->bypass = 1;
	flags.bypasses = TRUE;
}

void
mon_break_armor(mon, polyspot)
struct monst *mon;
boolean polyspot;
{
	register struct obj *otmp;
	struct permonst *mdat = mon->data;
	boolean vis = cansee(mon->mx, mon->my);
	const char *pronoun = mhim(mon),
			*ppronoun = mhis(mon);

	if (breakarm(mdat)) {
	    if ((otmp = which_armor(mon, W_ARM)) != 0) {
		if ((Is_dragon_scales(otmp) &&
			mdat == Dragon_scales_to_pm(otmp)) ||
		    (Is_dragon_mail(otmp) && mdat == Dragon_mail_to_pm(otmp)))
		    ;	/* no message here;
			   "the dragon merges with his scaly armor" is odd
			   and the monster's previous form is already gone */
		else if (vis)
		    pline("%s breaks out of %s armor!", Monnam(mon), ppronoun);
		else
		    You_hear("a cracking sound.");
		m_useup(mon, otmp);
	    }
	    if ((otmp = which_armor(mon, W_ARMC)) != 0) {
		if (otmp->oartifact) {
		    if (vis)
			pline("%s %s falls off!", s_suffix(Monnam(mon)),
				cloak_simple_name(otmp));
		    if (polyspot) bypass_obj(otmp);
		    m_lose_armor(mon, otmp);
		} else {
		    if (vis)
			pline("%s %s tears apart!", s_suffix(Monnam(mon)),
				cloak_simple_name(otmp));
		    else
			You_hear("a ripping sound.");
		    m_useup(mon, otmp);
		}
	    }
#ifdef TOURIST
	    if ((otmp = which_armor(mon, W_ARMU)) != 0) {
		if (vis)
		    pline("%s shirt rips to shreds!", s_suffix(Monnam(mon)));
		else
		    You_hear("a ripping sound.");
		m_useup(mon, otmp);
	    }
#endif
	} else if (sliparm(mdat)) {
	    if ((otmp = which_armor(mon, W_ARM)) != 0) {
		if (vis)
		    pline("%s armor falls around %s!",
				 s_suffix(Monnam(mon)), pronoun);
		else
		    You_hear("a thud.");
		if (polyspot) bypass_obj(otmp);
		m_lose_armor(mon, otmp);
	    }
	    if ((otmp = which_armor(mon, W_ARMC)) != 0) {
		if (vis) {
		    if (is_whirly(mon->data))
			pline("%s %s falls, unsupported!",
				     s_suffix(Monnam(mon)), cloak_simple_name(otmp));
		    else
			pline("%s shrinks out of %s %s!", Monnam(mon),
						ppronoun, cloak_simple_name(otmp));
		}
		if (polyspot) bypass_obj(otmp);
		m_lose_armor(mon, otmp);
	    }
#ifdef TOURIST
	    if ((otmp = which_armor(mon, W_ARMU)) != 0) {
		if (vis) {
		    if (sliparm(mon->data))
			pline("%s seeps right through %s shirt!",
					Monnam(mon), ppronoun);
		    else
			pline("%s becomes much too small for %s shirt!",
					Monnam(mon), ppronoun);
		}
		if (polyspot) bypass_obj(otmp);
		m_lose_armor(mon, otmp);
	    }
#endif
	}
	if (nohands(mdat) || verysmall(mdat)) {
	    if ((otmp = which_armor(mon, W_ARMG)) != 0) {
		if (vis)
		    pline("%s drops %s gloves%s!", Monnam(mon), ppronoun,
					MON_WEP(mon) ? " and weapon" : "");
		possibly_unwield(mon);
		if (polyspot) bypass_obj(otmp);
		m_lose_armor(mon, otmp);
	    }
	    if ((otmp = which_armor(mon, W_ARMS)) != 0) {
		if (vis)
		    pline("%s can no longer hold %s shield!", Monnam(mon),
								ppronoun);
		else
		    You_hear("a clank.");
		if (polyspot) bypass_obj(otmp);
		m_lose_armor(mon, otmp);
	    }
	    if ((otmp = which_armor(mon, W_ARMH)) != 0) {
		if (vis)
		    pline("%s helmet falls to the %s!",
			  s_suffix(Monnam(mon)), surface(mon->mx, mon->my));
		else
		    You_hear("a clank.");
		if (polyspot) bypass_obj(otmp);
		m_lose_armor(mon, otmp);
	    }
	}
	if (nohands(mdat) || verysmall(mdat) || slithy(mdat) ||
	    mdat->mlet == S_CENTAUR) {
	    if ((otmp = which_armor(mon, W_ARMF)) != 0) {
		if (vis) {
		    if (is_whirly(mon->data))
			pline("%s boots fall away!",
				       s_suffix(Monnam(mon)));
		    else pline("%s boots %s off %s feet!",
			s_suffix(Monnam(mon)),
			verysmall(mdat) ? "slide" : "are pushed", ppronoun);
		}
		if (polyspot) bypass_obj(otmp);
		m_lose_armor(mon, otmp);
	    }
	}
#ifdef STEED
	if (!can_saddle(mon)) {
	    if ((otmp = which_armor(mon, W_SADDLE)) != 0) {
		if (polyspot) bypass_obj(otmp);
		m_lose_armor(mon, otmp);
		if (vis)
		    pline("%s saddle falls off.", s_suffix(Monnam(mon)));
	    }
	    if (mon == u.usteed)
		goto noride;
	} else if (mon == u.usteed && !can_ride(mon)) {
	noride:
	    You("can no longer ride %s.", mon_nam(mon));
	    if (touch_petrifies(u.usteed->data) &&
			!Stone_resistance && rnl(3)) {
		char buf[BUFSZ];

		You("touch %s.", mon_nam(u.usteed));
		Sprintf(buf, "falling off %s",
				an(u.usteed->data->mname));
		instapetrify(buf);
	    }
	    dismount_steed(DISMOUNT_FELL);
	}
#endif
	return;
}

/* bias a monster's preferences towards armor that has special benefits. */
/* currently only does speed boots, but might be expanded if monsters get to
   use more armor abilities */
static int
extra_pref(mon, obj)
struct monst *mon;
struct obj *obj;
{
    if (obj) {
	if (obj->otyp == SPEED_BOOTS && mon->permspeed != MFAST)
	    return 20;
    }
    return 0;
}
/*worn.c*/

/*	SCCS Id: @(#)steal.c	3.4	2002/09/07	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_PTR int NDECL(stealarm);

#ifdef OVLB
STATIC_DCL const char *FDECL(equipname, (struct obj *));

STATIC_OVL const char *
equipname(otmp)
register struct obj *otmp;
{
	return (
#ifdef TOURIST
		(otmp == uarmu) ? "shirt" :
#endif
		(otmp == uarmf) ? "boots" :
		(otmp == uarms) ? "shield" :
		(otmp == uarmg) ? "gloves" :
		(otmp == uarmc) ? cloak_simple_name(otmp) :
		(otmp == uarmh) ? "helmet" : "armor");
}

#ifndef GOLDOBJ
long		/* actually returns something that fits in an int */
somegold()
{
#ifdef LINT	/* long conv. ok */
	return(0L);
#else
	return (long)( (u.ugold < 100) ? u.ugold :
		(u.ugold > 10000) ? rnd(10000) : rnd((int) u.ugold) );
#endif
}

void
stealgold(mtmp)
register struct monst *mtmp;
{
	register struct obj *gold = g_at(u.ux, u.uy);
	register long tmp;

	if (gold && ( !u.ugold || gold->quan > u.ugold || !rn2(5))) {
	    mtmp->mgold += gold->quan;
	    delobj(gold);
	    newsym(u.ux, u.uy);
	    pline("%s quickly snatches some gold from between your %s!",
		    Monnam(mtmp), makeplural(body_part(FOOT)));
	    if(!u.ugold || !rn2(5)) {
		if (!tele_restrict(mtmp)) rloc(mtmp);
		/* do not set mtmp->mavenge here; gold on the floor is fair game */
		monflee(mtmp, 0, FALSE, FALSE);
	    }
	} else if(u.ugold) {
	    u.ugold -= (tmp = somegold());
	    Your("purse feels lighter.");
	    mtmp->mgold += tmp;
	if (!tele_restrict(mtmp)) rloc(mtmp);
	    mtmp->mavenge = 1;
	    monflee(mtmp, 0, FALSE, FALSE);
	    flags.botl = 1;
	}
}

#else /* !GOLDOBJ */

long		/* actually returns something that fits in an int */
somegold(umoney)
long umoney;
{
#ifdef LINT	/* long conv. ok */
	return(0L);
#else
	return (long)( (umoney < 100) ? umoney :
		(umoney > 10000) ? rnd(10000) : rnd((int) umoney) );
#endif
}

/*
Find the first (and hopefully only) gold object in a chain.
Used when leprechaun (or you as leprechaun) looks for
someone else's gold.  Returns a pointer so the gold may
be seized without further searching.
May search containers too.
Deals in gold only, as leprechauns don't care for lesser coins.
*/
struct obj *
findgold(chain)
register struct obj *chain;
{
        while (chain && chain->otyp != GOLD_PIECE) chain = chain->nobj;
        return chain;
}

/* 
Steal gold coins only.  Leprechauns don't care for lesser coins.
*/
void
stealgold(mtmp)
register struct monst *mtmp;
{
	register struct obj *fgold = g_at(u.ux, u.uy);
	register struct obj *ygold;
	register long tmp;

        /* skip lesser coins on the floor */        
        while (fgold && fgold->otyp != GOLD_PIECE) fgold = fgold->nexthere; 

        /* Do you have real gold? */
        ygold = findgold(invent);

	if (fgold && ( !ygold || fgold->quan > ygold->quan || !rn2(5))) {
            obj_extract_self(fgold);
	    add_to_minv(mtmp, fgold);
	    newsym(u.ux, u.uy);
	    pline("%s quickly snatches some gold from between your %s!",
		    Monnam(mtmp), makeplural(body_part(FOOT)));
	    if(!ygold || !rn2(5)) {
		if (!tele_restrict(mtmp)) rloc(mtmp);
		monflee(mtmp, 0, FALSE, FALSE);
	    }
	} else if(ygold) {
            const int gold_price = objects[GOLD_PIECE].oc_cost;
	    tmp = (somegold(money_cnt(invent)) + gold_price - 1) / gold_price;
	    tmp = min(tmp, ygold->quan);
            if (tmp < ygold->quan) ygold = splitobj(ygold, tmp);
            freeinv(ygold);
            add_to_minv(mtmp, ygold);
	    Your("purse feels lighter.");
	    if (!tele_restrict(mtmp)) rloc(mtmp);
	    monflee(mtmp, 0, FALSE, FALSE);
	    flags.botl = 1;
	}
}
#endif /* GOLDOBJ */

/* steal armor after you finish taking it off */
unsigned int stealoid;		/* object to be stolen */
unsigned int stealmid;		/* monster doing the stealing */

STATIC_PTR int
stealarm()
{
	register struct monst *mtmp;
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj) {
	    if(otmp->o_id == stealoid) {
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		    if(mtmp->m_id == stealmid) {
			if(DEADMONSTER(mtmp)) impossible("stealarm(): dead monster stealing"); 
			if(!dmgtype(mtmp->data, AD_SITM)) /* polymorphed */
			    goto botm;
			if(otmp->unpaid)
			    subfrombill(otmp, shop_keeper(*u.ushops));
			freeinv(otmp);
			pline("%s steals %s!", Monnam(mtmp), doname(otmp));
			(void) mpickobj(mtmp,otmp);	/* may free otmp */
			/* Implies seduction, "you gladly hand over ..."
			   so we don't set mavenge bit here. */
			monflee(mtmp, 0, FALSE, FALSE);
			if (!tele_restrict(mtmp)) rloc(mtmp);
		        break;
		    }
		}
		break;
	    }
	}
botm:   stealoid = 0;
	return 0;
}

/* An object you're wearing has been taken off by a monster (theft or
   seduction).  Also used if a worn item gets transformed (stone to flesh). */
void
remove_worn_item(obj, unchain_ball)
struct obj *obj;
boolean unchain_ball;	/* whether to unpunish or just unwield */
{
	if (donning(obj))
	    cancel_don();
	if (!obj->owornmask)
	    return;

	if (obj->owornmask & W_ARMOR) {
	    if (obj == uskin) {
		impossible("Removing embedded scales?");
		skinback(TRUE);		/* uarm = uskin; uskin = 0; */
	    }
	    if (obj == uarm) (void) Armor_off();
	    else if (obj == uarmc) (void) Cloak_off();
	    else if (obj == uarmf) (void) Boots_off();
	    else if (obj == uarmg) (void) Gloves_off();
	    else if (obj == uarmh) (void) Helmet_off();
	    else if (obj == uarms) (void) Shield_off();
	    else setworn((struct obj *)0, obj->owornmask & W_ARMOR);
	} else if (obj->owornmask & W_AMUL) {
	    Amulet_off();
	} else if (obj->owornmask & W_RING) {
	    Ring_gone(obj);
	} else if (obj->owornmask & W_TOOL) {
	    Blindf_off(obj);
	} else if (obj->owornmask & (W_WEP|W_SWAPWEP|W_QUIVER)) {
	    if (obj == uwep)
		uwepgone();
	    if (obj == uswapwep)
		uswapwepgone();
	    if (obj == uquiver)
		uqwepgone();
	}

	if (obj->owornmask & (W_BALL|W_CHAIN)) {
	    if (unchain_ball) unpunish();
	} else if (obj->owornmask) {
	    /* catchall */
	    setnotworn(obj);
	}
}

/* Returns 1 when something was stolen (or at least, when N should flee now)
 * Returns -1 if the monster died in the attempt
 * Avoid stealing the object stealoid
 */
int
steal(mtmp, objnambuf)
struct monst *mtmp;
char *objnambuf;
{
	struct obj *otmp;
	int tmp, could_petrify, named = 0, armordelay;
	boolean monkey_business; /* true iff an animal is doing the thievery */

	if (objnambuf) *objnambuf = '\0';
	/* the following is true if successful on first of two attacks. */
	if(!monnear(mtmp, u.ux, u.uy)) return(0);

	/* food being eaten might already be used up but will not have
	   been removed from inventory yet; we don't want to steal that,
	   so this will cause it to be removed now */
	if (occupation) (void) maybe_finished_meal(FALSE);

	if (!invent || (inv_cnt() == 1 && uskin)) {
nothing_to_steal:
	    /* Not even a thousand men in armor can strip a naked man. */
	    if(Blind)
	      pline("Somebody tries to rob you, but finds nothing to steal.");
	    else
	      pline("%s tries to rob you, but there is nothing to steal!",
		Monnam(mtmp));
	    return(1);	/* let her flee */
	}

	monkey_business = is_animal(mtmp->data);
	if (monkey_business) {
	    ;	/* skip ring special cases */
	} else if (Adornment & LEFT_RING) {
	    otmp = uleft;
	    goto gotobj;
	} else if (Adornment & RIGHT_RING) {
	    otmp = uright;
	    goto gotobj;
	}

	tmp = 0;
	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if ((!uarm || otmp != uarmc) && otmp != uskin
#ifdef INVISIBLE_OBJECTS
				&& (!otmp->oinvis || perceives(mtmp->data))
#endif
				)
		tmp += ((otmp->owornmask &
			(W_ARMOR | W_RING | W_AMUL | W_TOOL)) ? 5 : 1);
	if (!tmp) goto nothing_to_steal;
	tmp = rn2(tmp);
	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if ((!uarm || otmp != uarmc) && otmp != uskin
#ifdef INVISIBLE_OBJECTS
				&& (!otmp->oinvis || perceives(mtmp->data))
#endif
			)
		if((tmp -= ((otmp->owornmask &
			(W_ARMOR | W_RING | W_AMUL | W_TOOL)) ? 5 : 1)) < 0)
			break;
	if(!otmp) {
		impossible("Steal fails!");
		return(0);
	}
	/* can't steal gloves while wielding - so steal the wielded item. */
	if (otmp == uarmg && uwep)
	    otmp = uwep;
	/* can't steal armor while wearing cloak - so steal the cloak. */
	else if(otmp == uarm && uarmc) otmp = uarmc;
#ifdef TOURIST
	else if(otmp == uarmu && uarmc) otmp = uarmc;
	else if(otmp == uarmu && uarm) otmp = uarm;
#endif
gotobj:
	if(otmp->o_id == stealoid) return(0);

	/* animals can't overcome curse stickiness nor unlock chains */
	if (monkey_business) {
	    boolean ostuck;
	    /* is the player prevented from voluntarily giving up this item?
	       (ignores loadstones; the !can_carry() check will catch those) */
	    if (otmp == uball)
		ostuck = TRUE;	/* effectively worn; curse is implicit */
	    else if (otmp == uquiver || (otmp == uswapwep && !u.twoweap))
		ostuck = FALSE;	/* not really worn; curse doesn't matter */
	    else
		ostuck = (otmp->cursed && otmp->owornmask);

	    if (ostuck || !can_carry(mtmp, otmp)) {
		static const char * const how[] = { "steal","snatch","grab","take" };
 cant_take:
		pline("%s tries to %s your %s but gives up.",
		      Monnam(mtmp), how[rn2(SIZE(how))],
		      (otmp->owornmask & W_ARMOR) ? equipname(otmp) :
		       cxname(otmp));
		/* the fewer items you have, the less likely the thief
		   is going to stick around to try again (0) instead of
		   running away (1) */
		return !rn2(inv_cnt() / 5 + 2);
	    }
	}

	if (otmp->otyp == LEASH && otmp->leashmon) {
	    if (monkey_business && otmp->cursed) goto cant_take;
	    o_unleash(otmp);
	}

	/* you're going to notice the theft... */
	stop_occupation();

	if((otmp->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL))){
		switch(otmp->oclass) {
		case TOOL_CLASS:
		case AMULET_CLASS:
		case RING_CLASS:
		case FOOD_CLASS: /* meat ring */
		    remove_worn_item(otmp, TRUE);
		    break;
		case ARMOR_CLASS:
		    armordelay = objects[otmp->otyp].oc_delay;
		    /* Stop putting on armor which has been stolen. */
		    if (donning(otmp)) {
			remove_worn_item(otmp, TRUE);
			break;
		    } else if (monkey_business) {
			/* animals usually don't have enough patience
			   to take off items which require extra time */
			if (armordelay >= 1 && rn2(10)) goto cant_take;
			remove_worn_item(otmp, TRUE);
			break;
		    } else {
			int curssv = otmp->cursed;
			int slowly;
			boolean seen = canspotmon(mtmp);

			otmp->cursed = 0;
			/* can't charm you without first waking you */
			if (multi < 0 && is_fainted()) unmul((char *)0);
			slowly = (armordelay >= 1 || multi < 0);
			if(flags.female)
			    pline("%s charms you.  You gladly %s your %s.",
				  !seen ? "She" : Monnam(mtmp),
				  curssv ? "let her take" :
				  slowly ? "start removing" : "hand over",
				  equipname(otmp));
			else
			    pline("%s seduces you and %s off your %s.",
				  !seen ? "She" : Adjmonnam(mtmp, "beautiful"),
				  curssv ? "helps you to take" :
				  slowly ? "you start taking" : "you take",
				  equipname(otmp));
			named++;
			/* the following is to set multi for later on */
			nomul(-armordelay);
			remove_worn_item(otmp, TRUE);
			otmp->cursed = curssv;
			if(multi < 0){
				/*
				multi = 0;
				nomovemsg = 0;
				afternmv = 0;
				*/
				stealoid = otmp->o_id;
				stealmid = mtmp->m_id;
				afternmv = stealarm;
				return(0);
			}
		    }
		    break;
		default:
		    impossible("Tried to steal a strange worn thing. [%d]",
			       otmp->oclass);
		}
	}
	else if (otmp->owornmask)
	    remove_worn_item(otmp, TRUE);

	/* do this before removing it from inventory */
	if (objnambuf) Strcpy(objnambuf, yname(otmp));
	/* set mavenge bit so knights won't suffer an
	 * alignment penalty during retaliation;
	 */
	mtmp->mavenge = 1;

	freeinv(otmp);
	pline("%s stole %s.", named ? "She" : Monnam(mtmp), doname(otmp));
	could_petrify = (otmp->otyp == CORPSE &&
			 touch_petrifies(&mons[otmp->corpsenm]));
	(void) mpickobj(mtmp,otmp);	/* may free otmp */
	if (could_petrify && !(mtmp->misc_worn_check & W_ARMG)) {
	    minstapetrify(mtmp, TRUE);
	    return -1;
	}
	return((multi < 0) ? 0 : 1);
}

#endif /* OVLB */
#ifdef OVL1

/* Returns 1 if otmp is free'd, 0 otherwise. */
int
mpickobj(mtmp,otmp)
register struct monst *mtmp;
register struct obj *otmp;
{
    int freed_otmp;

#ifndef GOLDOBJ
    if (otmp->oclass == COIN_CLASS) {
	mtmp->mgold += otmp->quan;
	obfree(otmp, (struct obj *)0);
	freed_otmp = 1;
    } else {
#endif
    boolean snuff_otmp = FALSE;
    /* don't want hidden light source inside the monster; assumes that
       engulfers won't have external inventories; whirly monsters cause
       the light to be extinguished rather than letting it shine thru */
    if (otmp->lamplit &&  /* hack to avoid function calls for most objs */
      	obj_sheds_light(otmp) &&
	attacktype(mtmp->data, AT_ENGL)) {
	/* this is probably a burning object that you dropped or threw */
	if (u.uswallow && mtmp == u.ustuck && !Blind)
	    pline("%s out.", Tobjnam(otmp, "go"));
	snuff_otmp = TRUE;
    }
    /* Must do carrying effects on object prior to add_to_minv() */
    carry_obj_effects(otmp);
    /* add_to_minv() might free otmp [if merged with something else],
       so we have to call it after doing the object checks */
    freed_otmp = add_to_minv(mtmp, otmp);
    /* and we had to defer this until object is in mtmp's inventory */
    if (snuff_otmp) snuff_light_source(mtmp->mx, mtmp->my);
#ifndef GOLDOBJ
    }
#endif
    return freed_otmp;
}

#endif /* OVL1 */
#ifdef OVLB

void
stealamulet(mtmp)
struct monst *mtmp;
{
    struct obj *otmp = (struct obj *)0;
    int real=0, fake=0;

    /* select the artifact to steal */
    if(u.uhave.amulet) {
	real = AMULET_OF_YENDOR;
	fake = FAKE_AMULET_OF_YENDOR;
    } else if(u.uhave.questart) {
	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if(is_quest_artifact(otmp)) break;
	if (!otmp) return;	/* should we panic instead? */
    } else if(u.uhave.bell) {
	real = BELL_OF_OPENING;
	fake = BELL;
    } else if(u.uhave.book) {
	real = SPE_BOOK_OF_THE_DEAD;
    } else if(u.uhave.menorah) {
	real = CANDELABRUM_OF_INVOCATION;
    } else return;	/* you have nothing of special interest */

    if (!otmp) {
	/* If we get here, real and fake have been set up. */
	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if(otmp->otyp == real || (otmp->otyp == fake && !mtmp->iswiz))
		break;
    }

    if (otmp) { /* we have something to snatch */
	if (otmp->owornmask)
	    remove_worn_item(otmp, TRUE);
	freeinv(otmp);
	/* mpickobj wont merge otmp because none of the above things
	   to steal are mergable */
	(void) mpickobj(mtmp,otmp);	/* may merge and free otmp */
	pline("%s stole %s!", Monnam(mtmp), doname(otmp));
	if (can_teleport(mtmp->data) && !tele_restrict(mtmp))
	    rloc(mtmp);
    }
}

#endif /* OVLB */
#ifdef OVL0

/* release the objects the creature is carrying */
void
relobj(mtmp,show,is_pet)
register struct monst *mtmp;
register int show;
boolean is_pet;		/* If true, pet should keep wielded/worn items */
{
	register struct obj *otmp;
	register int omx = mtmp->mx, omy = mtmp->my;
	struct obj *keepobj = 0;
	struct obj *wep = MON_WEP(mtmp);
	boolean item1 = FALSE, item2 = FALSE;

	if (!is_pet || mindless(mtmp->data) || is_animal(mtmp->data))
		item1 = item2 = TRUE;
	if (!tunnels(mtmp->data) || !needspick(mtmp->data))
		item1 = TRUE;
	while ((otmp = mtmp->minvent) != 0) {
		obj_extract_self(otmp);
		/* special case: pick-axe and unicorn horn are non-worn */
		/* items that we also want pets to keep 1 of */
		/* (It is a coincidence that these can also be wielded. */
		if (otmp->owornmask || otmp == wep ||
		    ((!item1 && otmp->otyp == PICK_AXE) ||
		     (!item2 && otmp->otyp == UNICORN_HORN && !otmp->cursed))) {
			if (is_pet) { /* dont drop worn/wielded item */
				if (otmp->otyp == PICK_AXE)
					item1 = TRUE;
				if (otmp->otyp == UNICORN_HORN && !otmp->cursed)
					item2 = TRUE;
				otmp->nobj = keepobj;
				keepobj = otmp;
				continue;
			}
			mtmp->misc_worn_check &= ~(otmp->owornmask);
#ifdef STEED
			/* don't charge for an owned saddle on dead pet */
			if (mtmp->mtame && mtmp->mhp == 0 &&
			    (otmp->owornmask & W_SADDLE) && !otmp->unpaid &&
			    costly_spot(mtmp->mx, mtmp->my))
				otmp->no_charge = 1;
#endif
			if (otmp->owornmask)
			    update_mon_intrinsics(mtmp, otmp, FALSE, TRUE);
		     /* obj_no_longer_held(otmp); -- done by place_object */
			if (otmp->owornmask & W_WEP)
			    setmnotwielded(mtmp, otmp);
			otmp->owornmask = 0L;
		}
		if (is_pet && cansee(omx, omy) && flags.verbose)
			pline("%s drops %s.", Monnam(mtmp),
					distant_name(otmp, doname));
		if (flooreffects(otmp, omx, omy, "fall")) continue;
		place_object(otmp, omx, omy);
		stackobj(otmp);
	}
	/* put kept objects back */
	while ((otmp = keepobj) != (struct obj *)0) {
	    keepobj = otmp->nobj;
	    (void) add_to_minv(mtmp, otmp);
	}
#ifndef GOLDOBJ
	if (mtmp->mgold) {
		register long g = mtmp->mgold;
		(void) mkgold(g, omx, omy);
		if (is_pet && cansee(omx, omy) && flags.verbose)
			pline("%s drops %ld gold piece%s.", Monnam(mtmp),
				g, plur(g));
		mtmp->mgold = 0L;
	}
#endif
	
	if (show & cansee(omx, omy))
		newsym(omx, omy);
}

#endif /* OVL0 */

/*steal.c*/

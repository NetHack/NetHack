/*	SCCS Id: @(#)steed.c	3.4	2002/03/09	*/
/* Copyright (c) Kevin Hugo, 1998-1999. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"


#ifdef STEED

/* Monsters that might be ridden */
static NEARDATA const char steeds[] = {
	S_QUADRUPED, S_UNICORN, S_ANGEL, S_CENTAUR, S_DRAGON, S_JABBERWOCK, '\0'
};

STATIC_DCL boolean FDECL(landing_spot, (coord *, int));

/*** Putting the saddle on ***/

/* Can this monster wear a saddle? */
boolean
can_saddle(mtmp)
	struct monst *mtmp;
{
	struct permonst *ptr = mtmp->data;

	return (index(steeds, ptr->mlet) && (ptr->msize >= MZ_MEDIUM) &&
			(!humanoid(ptr) || ptr->mlet == S_CENTAUR) &&
			!amorphous(ptr) && !noncorporeal(ptr) &&
			!is_whirly(ptr) && !unsolid(ptr));
}


int
use_saddle(otmp)
	struct obj *otmp;
{
	struct monst *mtmp;
	struct permonst *ptr;
	int chance;
	const char *s;


	/* Can you use it? */
	if (nohands(youmonst.data)) {
		You("have no hands!");	/* not `body_part(HAND)' */
		return 0;
	} else if (!freehand()) {
		You("have no free %s.", body_part(HAND));
		return 0;
	}

	/* Select an animal */
	if (u.uswallow || Underwater || !getdir((char *)0)) {
	    pline(Never_mind);
	    return 0;
	}
	if (!u.dx && !u.dy) {
	    pline("Saddle yourself?  Very funny...");
	    return 0;
	}
	if (!isok(u.ux+u.dx, u.uy+u.dy) ||
			!(mtmp = m_at(u.ux+u.dx, u.uy+u.dy)) ||
			!canspotmon(mtmp)) {
	    pline("I see nobody there.");
	    return 1;
	}

	/* Is this a valid monster? */
	if (mtmp->misc_worn_check & W_SADDLE ||
			which_armor(mtmp, W_SADDLE)) {
	    pline("%s doesn't need another one.", Monnam(mtmp));
	    return 1;
	}
	ptr = mtmp->data;
	if (touch_petrifies(ptr) && !Stone_resistance) {
	    char kbuf[BUFSZ];

	    You("touch %s.", mon_nam(mtmp));
 	    if (!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
			Sprintf(kbuf, "attempting to saddle %s", a_monnam(mtmp));
			instapetrify(kbuf);
 	    }
	}
	if (ptr == &mons[PM_INCUBUS] || ptr == &mons[PM_SUCCUBUS]) {
	    pline("Shame on you!");
	    exercise(A_WIS, FALSE);
	    return 1;
	}
	if (mtmp->isminion || mtmp->isshk || mtmp->ispriest ||
			mtmp->isgd || mtmp->iswiz) {
	    pline("I think %s would mind.", mon_nam(mtmp));
	    return 1;
	}
	if (!can_saddle(mtmp)) {
		You_cant("saddle such a creature.");
		return 1;
	}

	/* Calculate your chance */
	chance = ACURR(A_DEX) + ACURR(A_CHA)/2 + 2*mtmp->mtame;
	chance += u.ulevel * (mtmp->mtame ? 20 : 5);
	if (!mtmp->mtame) chance -= 10*mtmp->m_lev;
	if (Role_if(PM_KNIGHT))
	    chance += 20;
	switch (P_SKILL(P_RIDING)) {
	case P_ISRESTRICTED:
	case P_UNSKILLED:
	default:
	    chance -= 20;	break;
	case P_BASIC:
	    break;
	case P_SKILLED:
	    chance += 15;	break;
	case P_EXPERT:
	    chance += 30;	break;
	}
	if (Confusion || Fumbling || Glib)
	    chance -= 20;
	else if (uarmg &&
		(s = OBJ_DESCR(objects[uarmg->otyp])) != (char *)0 &&
		!strncmp(s, "riding ", 7))
	    /* Bonus for wearing "riding" (but not fumbling) gloves */
	    chance += 10;
	else if (uarmf &&
		(s = OBJ_DESCR(objects[uarmf->otyp])) != (char *)0 &&
		!strncmp(s, "riding ", 7))
	    /* ... or for "riding boots" */
	    chance += 10;
	if (otmp->cursed)
	    chance -= 50;

	/* Make the attempt */
	if (rn2(100) < chance) {
	    You("put the saddle on %s.", mon_nam(mtmp));
	    freeinv(otmp);
	    /* mpickobj may free otmp it if merges, but we have already
	       checked for a saddle above, so no merger should happen */
	    (void) mpickobj(mtmp, otmp);
	    mtmp->misc_worn_check |= W_SADDLE;
	    otmp->owornmask = W_SADDLE;
	    otmp->leashmon = mtmp->m_id;
	    update_mon_intrinsics(mtmp, otmp, TRUE);
	} else
	    pline("%s resists!", Monnam(mtmp));
	return 1;
}


/*** Riding the monster ***/

/* Can we ride this monster?  Caller should also check can_saddle() */
boolean
can_ride(mtmp)
	struct monst *mtmp;
{
	return (mtmp->mtame && humanoid(youmonst.data) &&
			!verysmall(youmonst.data) && !bigmonst(youmonst.data) &&
			(!Underwater || is_swimmer(mtmp->data)));
}


int
doride()
{
	boolean forcemount = FALSE;

	if (u.usteed)
	    dismount_steed(DISMOUNT_BYCHOICE);
	else if (getdir((char *)0) && isok(u.ux+u.dx, u.uy+u.dy)) {
#ifdef WIZARD
	if (wizard && yn("Force the mount to succeed?") == 'y')
		forcemount = TRUE;
#endif
	    return (mount_steed(m_at(u.ux+u.dx, u.uy+u.dy), forcemount));
	} else
	    return 0;
	return 1;
}


/* Start riding, with the given monster */
boolean
mount_steed(mtmp, force)
	struct monst *mtmp;	/* The animal */
	boolean force;		/* Quietly force this animal */
{
	struct obj *otmp;
	char buf[BUFSZ];
	struct permonst *ptr;


	/* Sanity checks */
	if (u.usteed) {
	    if (!force)
	    	You("are already riding %s.", mon_nam(u.usteed));
	    return (FALSE);
	}

	/* Is the player in the right form? */
	if (Hallucination && !force) {
	    pline("Maybe you should find a designated driver.");
	    return (FALSE);
	}
	/* While riding Wounded_legs refers to the steed's,
	 * not the hero's legs.
	 * That opens up a potential abuse where the player
	 * can mount a steed, then dismount immediately to
	 * heal leg damage, because leg damage is always
	 * healed upon dismount (Wounded_legs context switch).
	 * By preventing a hero with Wounded_legs from
	 * mounting a steed, the potential for abuse is
	 * minimized, if not eliminated altogether.
	 */
	if (Wounded_legs) {
	    Your("%s are in no shape for riding.", makeplural(body_part(LEG)));
#ifdef WIZARD
	    if (force && wizard && yn("Heal your legs?") == 'y')
		HWounded_legs = EWounded_legs = 0;
	    else
#endif
	    return (FALSE);
	}
	
	if (Upolyd && (!humanoid(youmonst.data) || verysmall(youmonst.data) ||
			bigmonst(youmonst.data))) {
	    if (!force)
	    	You("won't fit on a saddle.");
	    return (FALSE);
	}
	if(!force && (near_capacity() > SLT_ENCUMBER)) {
	    You_cant("do that while carrying so much stuff.");
	    return (FALSE);
	}

	/* Can the player reach and see the monster? */
    if (u.uswallow || u.ustuck || u.utrap || Punished) {
        if (!force) {
		if (Punished)
			You("are unable to swing your %s over.",
				body_part(LEG)); 
		else
        		You("are stuck here for now.");
	}
        return (FALSE);
    }
	if (!mtmp || (!force && ((Blind && !Blind_telepat) ||
		mtmp->mundetected ||
		mtmp->m_ap_type == M_AP_FURNITURE ||
		mtmp->m_ap_type == M_AP_OBJECT))) {
	    if (!force)
	    	pline("I see nobody there.");
	    return (FALSE);
	}

	/* Is this a valid monster? */
	otmp = which_armor(mtmp, W_SADDLE);
	if (!otmp) {
	    pline("%s is not saddled.", Monnam(mtmp));
	    return (FALSE);
	}
	ptr = mtmp->data;
	if (touch_petrifies(ptr) && !Stone_resistance) {
	    char kbuf[BUFSZ];

	    You("touch %s.", mon_nam(mtmp));
 	    Sprintf(kbuf, "attempting to ride %s", an(mtmp->data->mname));
	    instapetrify(kbuf);
	}
	if (!mtmp->mtame || mtmp->isminion) {
	    if (!force)
	    	pline("I think %s would mind.", mon_nam(mtmp));
	    return (FALSE);
	}
	if (!force && !Role_if(PM_KNIGHT) && !(--mtmp->mtame)) {
	    /* no longer tame */
	    newsym(mtmp->mx, mtmp->my);
	    pline("%s resists%s!", Monnam(mtmp),
		  mtmp->mleashed ? " and its leash comes off" : "");
	    if (mtmp->mleashed) m_unleash(mtmp, FALSE);
	    return (FALSE);
	}
	if (!force && Underwater && !is_swimmer(ptr)) {
	    You_cant("ride that creature while under water.");
	    return (FALSE);
	}
	if (!can_saddle(mtmp) || !can_ride(mtmp)) {
	    if (!force)
	    	You_cant("ride such a creature.");
	    return (0);
	}

	/* Is the player impaired? */
	if (!force && !is_floater(ptr) && !is_flyer(ptr) &&
			Levitation && !Lev_at_will) {
	    You("cannot reach %s.", mon_nam(mtmp));
	    return (FALSE);
	}
	if (!force && uarm && is_metallic(uarm) &&
			greatest_erosion(uarm)) {
	    Your("%s armor is too stiff to be able to mount %s.",
			uarm->oeroded ? "rusty" : "corroded",
			mon_nam(mtmp));
	    return (FALSE);
	}
	if (!force && (Confusion || Fumbling || Glib || Wounded_legs ||
		otmp->cursed || (u.ulevel+mtmp->mtame < rnd(MAXULEV/2+5)))) {
	    if (Levitation) {
		pline("%s slips away from you.", Monnam(mtmp));
		return FALSE;
	    }
	    You("slip while trying to get on %s.", mon_nam(mtmp));

	    /* Unfortunately we don't have a version of the monster-naming
	     * function that works well with "a" and "the" but ignores
	     * hallucination.  Fortunately, we know the monster must be saddled
	     * at this point, and that it can't have type_is_pname(), so we
	     * don't need to worry about the special cases such a function
	     * would have to consider.
	     */
	    Sprintf(buf, "slipped while mounting a saddled %s",
		    m_monnam(mtmp));
	    losehp(rn1(5,10), buf, NO_KILLER_PREFIX);
	    return (FALSE);
	}

	/* Success */
	if (!force) {
	    if (Levitation && !is_floater(ptr) && !is_flyer(ptr))
	    	/* Must have Lev_at_will at this point */
	    	pline("%s magically floats up!", Monnam(mtmp));
	    You("mount %s.", mon_nam(mtmp));
	}
	u.usteed = mtmp;
	remove_monster(mtmp->mx, mtmp->my);
	teleds(mtmp->mx, mtmp->my);
	return (TRUE);
}


/* You and your steed have moved */
void
exercise_steed()
{
	if (!u.usteed)
		return;

	/* It takes many turns of riding to exercise skill */
	if (u.urideturns++ >= 100) {
	    u.urideturns = 0;
	    use_skill(P_RIDING, 1);
	}
	return;
}


/* The player kicks or whips the steed */
void
kick_steed()
{
	char He[4];
	if (!u.usteed)
	    return;

	/* [ALI] Various effects of kicking sleeping/paralyzed steeds */
	if (u.usteed->msleeping || !u.usteed->mcanmove) {
	    /* We assume a message has just been output of the form
	     * "You kick <steed>."
	     */
	    Strcpy(He, mhe(u.usteed));
	    *He = highc(*He);
	    if ((u.usteed->mcanmove || u.usteed->mfrozen) && !rn2(2)) {
		if (u.usteed->mcanmove)
		    u.usteed->msleeping = 0;
		else if (u.usteed->mfrozen > 2)
		    u.usteed->mfrozen -= 2;
		else {
		    u.usteed->mfrozen = 0;
		    u.usteed->mcanmove = 1;
		}
		if (u.usteed->msleeping || !u.usteed->mcanmove)
		    pline("%s stirs.", He);
		else
		    pline("%s rouses %sself!", He, mhim(u.usteed));
	    } else
		pline("%s does not respond.", He);
	    return;
	}

	/* Make the steed less tame and check if it resists */
	if (u.usteed->mtame) u.usteed->mtame--;
	if (!u.usteed->mtame && u.usteed->mleashed) m_unleash(u.usteed, TRUE);
	if (!u.usteed->mtame || (u.ulevel+u.usteed->mtame < rnd(MAXULEV/2+5))) {
	    newsym(u.usteed->mx, u.usteed->my);
	    dismount_steed(DISMOUNT_THROWN);
	    return;
	}

	pline("%s gallops!", Monnam(u.usteed));
	u.ugallop += rn1(20, 30);
	return;
}

/*
 * Try to find a dismount point adjacent to the steed's location.
 * If all else fails, try enexto().  Use enexto() as a last resort because
 * enexto() chooses its point randomly, possibly even outside the
 * room's walls, which is not what we want.
 * Adapted from mail daemon code.
 */
STATIC_OVL boolean
landing_spot(spot, forceit)
coord *spot;	/* landing position (we fill it in) */
int forceit;
{
    int x, y, distance, min_distance = -1;
    boolean found = FALSE;
    
    for (x = u.ux-1; x <= u.ux+1; x++)
  	for (y = u.uy-1; y <= u.uy+1; y++) {
	    if (!isok(x, y) || (x == u.ux && y == u.uy)) continue;

	    if (ACCESSIBLE(levl[x][y].typ) &&
			!MON_AT(x,y) && !closed_door(x,y)) {
		distance = distu(x,y);
		if (min_distance < 0 || distance < min_distance ||
			(distance == min_distance && rn2(2))) {
		    spot->x = x;
		    spot->y = y;
		    min_distance = distance;
		    found = TRUE;
		}
	    }
	}
    
    /* If we didn't find a good spot and forceit is on, try enexto(). */
    if (forceit && min_distance < 0 &&
		!enexto(spot, u.ux, u.uy, youmonst.data))
	return FALSE;

    return found;
}

/* Stop riding the current steed */
void
dismount_steed(reason)
	int reason;		/* Player was thrown off etc. */
{
	struct monst *mtmp;
	struct obj *otmp;
	coord cc;
	const char *verb = "fall";
	boolean repair_leg_damage = TRUE;
	unsigned save_utrap = u.utrap;
	boolean have_spot = landing_spot(&cc,0);
	
	mtmp = u.usteed;		/* make a copy of steed pointer */
	/* Sanity check */
	if (!mtmp)		/* Just return silently */
	    return;

	/* Check the reason for dismounting */
	otmp = which_armor(mtmp, W_SADDLE);
	switch (reason) {
	    case DISMOUNT_THROWN:
		verb = "are thrown";
	    case DISMOUNT_FELL:
		You("%s off of %s!", verb, mon_nam(mtmp));
		if (!have_spot) have_spot = landing_spot(&cc,1);
		losehp(rn1(10,10), "riding accident", KILLED_BY_AN);
		set_wounded_legs(BOTH_SIDES, (int)HWounded_legs + rn1(5,5));
		repair_leg_damage = FALSE;
		break;
	    case DISMOUNT_POLY:
		You("can no longer ride %s.", mon_nam(u.usteed));
		if (!have_spot) have_spot = landing_spot(&cc,1);
		break;
	    case DISMOUNT_ENGULFED:
		/* caller displays message */
		break;
	    case DISMOUNT_BONES:
		/* hero has just died... */
		break;
	    case DISMOUNT_GENERIC:
		/* no messages, just make it so */
		break;
	    case DISMOUNT_BYCHOICE:
	    default:
		if (otmp && otmp->cursed) {
		    You("can't.  The saddle seems to be cursed.");
		    otmp->bknown = TRUE;
		    return;
		}
		if (!have_spot) {
		    You("can't. There isn't anywhere for you to stand.");
		    return;
		}
		if (!mtmp->mnamelth) {
			pline("You've been through the dungeon on %s with no name.",
				an(mtmp->data->mname));
			if (Hallucination)
				pline("It felt good to get out of the rain.");
		} else
			You("dismount %s.", mon_nam(mtmp));
	}
	/* While riding these refer to the steed's legs
	 * so after dismounting they refer to the player's
	 * legs once again.
	 */
	if (repair_leg_damage) HWounded_legs = EWounded_legs = 0;

	/* Release the steed and saddle */
	u.usteed = 0;
	u.ugallop = 0L;

	/* Set player and steed's position.  Try moving the player first
	   unless we're in the midst of creating a bones file. */
	if (reason == DISMOUNT_BONES) {
	    /* move the steed to an adjacent square */
	    if (enexto(&cc, u.ux, u.uy, mtmp->data))
		rloc_to(mtmp, cc.x, cc.y);
	    else	/* evidently no room nearby; move steed elsewhere */
		rloc(mtmp);
	    return;
	}
	if (!DEADMONSTER(mtmp)) {
	    place_monster(mtmp, u.ux, u.uy);
	    if (!u.uswallow && !u.ustuck && have_spot) {
		struct permonst *mdat = mtmp->data;

		/* The steed may drop into water/lava */
		if (!is_flyer(mdat) && !is_floater(mdat) && !is_clinger(mdat)) {
		    if (is_pool(u.ux, u.uy)) {
			if (!Underwater)
			    pline("%s falls into the %s!", Monnam(mtmp),
							surface(u.ux, u.uy));
			if (!is_swimmer(mdat) && !amphibious(mdat)) {
			    killed(mtmp);
			    adjalign(-1);
			}
		    } else if (is_lava(u.ux, u.uy)) {
			pline("%s is pulled into the lava!", Monnam(mtmp));
			if (!likes_lava(mdat)) {
			    killed(mtmp);
			    adjalign(-1);
			}
		    }
		}
	    /* Steed dismounting consists of two steps: being moved to another
	     * square, and descending to the floor.  We have functions to do
	     * each of these activities, but they're normally called
	     * individually and include an attempt to look at or pick up the
	     * objects on the floor:
	     * teleds() --> spoteffects() --> pickup()
	     * float_down() --> pickup()
	     * We use this kludge to make sure there is only one such attempt.
	     *
	     * Clearly this is not the best way to do it.  A full fix would
	     * involve having these functions not call pickup() at all, instead
	     * calling them first and calling pickup() afterwards.  But it
	     * would take a lot of work to keep this change from having any
	     * unforseen side effects (for instance, you would no longer be
	     * able to walk onto a square with a hole, and autopickup before
	     * falling into the hole).
	     */
		/* [ALI] No need to move the player if the steed died. */
		if (!DEADMONSTER(mtmp)) {
		    /* Keep steed here, move the player to cc;
		     * teleds() clears u.utrap
		     */
		    in_steed_dismounting = TRUE;
		    teleds(cc.x, cc.y);
		    in_steed_dismounting = FALSE;

		    /* Put your steed in your trap */
		    if (save_utrap)
			(void) mintrap(mtmp);
		}
	    /* Couldn't... try placing the steed */
	    } else if (enexto(&cc, u.ux, u.uy, mtmp->data)) {
		/* Keep player here, move the steed to cc */
		rloc_to(mtmp, cc.x, cc.y);
		/* Player stays put */
	    /* Otherwise, kill the steed */
	    } else {
		killed(mtmp);
		adjalign(-1);
	    }
	}

	/* Return the player to the floor */
	in_steed_dismounting = TRUE;
	(void) float_down(0L, W_SADDLE);
	in_steed_dismounting = FALSE;
	flags.botl = 1;
	if (reason != DISMOUNT_ENGULFED) {
		(void)encumber_msg();
		vision_full_recalc = 1;
	}
	return;
}

void
place_monster(mon, x, y)
struct monst *mon;
int x, y;
{
    if (mon == u.usteed ||
	    /* special case is for convoluted vault guard handling */
	    (DEADMONSTER(mon) && !(mon->isgd && x == 0 && y == 0))) {
	impossible("placing %s onto map?",
		   (mon == u.usteed) ? "steed" : "defunct monster");
	return;
    }
    mon->mx = x, mon->my = y;
    level.monsters[x][y] = mon;
}

#endif /* STEED */

/*steed.c*/

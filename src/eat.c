/*	SCCS Id: @(#)eat.c	3.4	2003/02/13	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
/* #define DEBUG */	/* uncomment to enable new eat code debugging */

#ifdef DEBUG
# ifdef WIZARD
#define debugpline	if (wizard) pline
# else
#define debugpline	pline
# endif
#endif

STATIC_PTR int NDECL(eatmdone);
STATIC_PTR int NDECL(eatfood);
STATIC_PTR int NDECL(opentin);
STATIC_PTR int NDECL(unfaint);

#ifdef OVLB
STATIC_DCL const char *FDECL(food_xname, (struct obj *,BOOLEAN_P));
STATIC_DCL void FDECL(choke, (struct obj *));
STATIC_DCL void NDECL(recalc_wt);
STATIC_DCL struct obj *FDECL(touchfood, (struct obj *));
STATIC_DCL void NDECL(do_reset_eat);
STATIC_DCL void FDECL(done_eating, (BOOLEAN_P));
STATIC_DCL void FDECL(cprefx, (int));
STATIC_DCL int FDECL(intrinsic_possible, (int,struct permonst *));
STATIC_DCL void FDECL(givit, (int,struct permonst *));
STATIC_DCL void FDECL(cpostfx, (int));
STATIC_DCL void FDECL(start_tin, (struct obj *));
STATIC_DCL int FDECL(eatcorpse, (struct obj *));
STATIC_DCL void FDECL(start_eating, (struct obj *));
STATIC_DCL void FDECL(fprefx, (struct obj *));
STATIC_DCL void FDECL(accessory_has_effect, (struct obj *));
STATIC_DCL void FDECL(fpostfx, (struct obj *));
STATIC_DCL int NDECL(bite);
STATIC_DCL int FDECL(edibility_prompts, (struct obj *));

STATIC_DCL int FDECL(rottenfood, (struct obj *));
STATIC_DCL void NDECL(eatspecial);
STATIC_DCL void FDECL(eataccessory, (struct obj *));
STATIC_DCL const char *FDECL(foodword, (struct obj *));

char msgbuf[BUFSZ];

#endif /* OVLB */

/* hunger texts used on bottom line (each 8 chars long) */
#define SATIATED	0
#define NOT_HUNGRY	1
#define HUNGRY		2
#define WEAK		3
#define FAINTING	4
#define FAINTED		5
#define STARVED		6

/* also used to see if you're allowed to eat cats and dogs */
#define CANNIBAL_ALLOWED() (Role_if(PM_CAVEMAN) || Race_if(PM_ORC))

#ifndef OVLB

STATIC_DCL NEARDATA const char comestibles[];
STATIC_DCL NEARDATA const char allobj[];
STATIC_DCL boolean force_save_hs;

#else

STATIC_OVL NEARDATA const char comestibles[] = { FOOD_CLASS, 0 };

/* Gold must come first for getobj(). */
STATIC_OVL NEARDATA const char allobj[] = {
	COIN_CLASS, WEAPON_CLASS, ARMOR_CLASS, POTION_CLASS, SCROLL_CLASS,
	WAND_CLASS, RING_CLASS, AMULET_CLASS, FOOD_CLASS, TOOL_CLASS,
	GEM_CLASS, ROCK_CLASS, BALL_CLASS, CHAIN_CLASS, SPBOOK_CLASS, 0 };

STATIC_OVL boolean force_save_hs = FALSE;

const char *hu_stat[] = {
	"Satiated",
	"        ",
	"Hungry  ",
	"Weak    ",
	"Fainting",
	"Fainted ",
	"Starved "
};

#endif /* OVLB */
#ifdef OVL1

/*
 * Decide whether a particular object can be eaten by the possibly
 * polymorphed character.  Not used for monster checks.
 */
boolean
is_edible(obj)
register struct obj *obj;
{
	/* protect invocation tools but not Rider corpses (handled elsewhere)*/
     /* if (obj->oclass != FOOD_CLASS && obj_resists(obj, 0, 0)) */
	if (objects[obj->otyp].oc_unique)
		return FALSE;
	/* above also prevents the Amulet from being eaten, so we must never
	   allow fake amulets to be eaten either [which is already the case] */

	if (metallivorous(youmonst.data) && is_metallic(obj) &&
	    (youmonst.data != &mons[PM_RUST_MONSTER] || is_rustprone(obj)))
		return TRUE;
	if (u.umonnum == PM_GELATINOUS_CUBE && is_organic(obj) &&
		/* [g.cubes can eat containers and retain all contents
		    as engulfed items, but poly'd player can't do that] */
	    !Has_contents(obj))
		return TRUE;

     /* return((boolean)(!!index(comestibles, obj->oclass))); */
	return (boolean)(obj->oclass == FOOD_CLASS);
}

#endif /* OVL1 */
#ifdef OVLB

void
init_uhunger()
{
	u.uhunger = 900;
	u.uhs = NOT_HUNGRY;
}

static const struct { const char *txt; int nut; } tintxts[] = {
	{"deep fried",	 60},
	{"pickled",	 40},
	{"soup made from", 20},
	{"pureed",	500},
#define ROTTEN_TIN 4
	{"rotten",	-50},
#define HOMEMADE_TIN 5
	{"homemade",	 50},
	{"stir fried",   80},
	{"candied",      100},
	{"boiled",       50},
	{"dried",        55},
	{"szechuan",     70},
#define FRENCH_FRIED_TIN 11
	{"french fried", 40},
	{"sauteed",      95},
	{"broiled",      80},
	{"smoked",       50},
	{"", 0}
};
#define TTSZ	SIZE(tintxts)

static NEARDATA struct {
	struct	obj *tin;
	int	usedtime, reqtime;
} tin;

static NEARDATA struct {
	struct	obj *piece;	/* the thing being eaten, or last thing that
				 * was partially eaten, unless that thing was
				 * a tin, which uses the tin structure above,
				 * in which case this should be 0 */
	/* doeat() initializes these when piece is valid */
	int	usedtime,	/* turns spent eating */
		reqtime;	/* turns required to eat */
	int	nmod;		/* coded nutrition per turn */
	Bitfield(canchoke,1);	/* was satiated at beginning */

	/* start_eating() initializes these */
	Bitfield(fullwarn,1);	/* have warned about being full */
	Bitfield(eating,1);	/* victual currently being eaten */
	Bitfield(doreset,1);	/* stop eating at end of turn */
} victual;

static char *eatmbuf = 0;	/* set by cpostfx() */

STATIC_PTR
int
eatmdone()		/* called after mimicing is over */
{
	/* release `eatmbuf' */
	if (eatmbuf) {
	    if (nomovemsg == eatmbuf) nomovemsg = 0;
	    free((genericptr_t)eatmbuf),  eatmbuf = 0;
	}
	/* update display */
	if (youmonst.m_ap_type) {
	    youmonst.m_ap_type = M_AP_NOTHING;
	    newsym(u.ux,u.uy);
	}
	return 0;
}

/* ``[the(] singular(food, xname) [)]'' with awareness of unique monsters */
STATIC_OVL const char *
food_xname(food, the_pfx)
struct obj *food;
boolean the_pfx;
{
	const char *result;
	int mnum = food->corpsenm;

	if (food->otyp == CORPSE && (mons[mnum].geno & G_UNIQ)) {
	    /* grab xname()'s modifiable return buffer for our own use */
	    char *bufp = xname(food);
	    Sprintf(bufp, "%s%s corpse",
		    (the_pfx && !type_is_pname(&mons[mnum])) ? "the " : "",
		    s_suffix(mons[mnum].mname));
	    result = bufp;
	} else {
	    /* the ordinary case */
	    result = singular(food, xname);
	    if (the_pfx) result = the(result);
	}
	return result;
}

/* Created by GAN 01/28/87
 * Amended by AKP 09/22/87: if not hard, don't choke, just vomit.
 * Amended by 3.  06/12/89: if not hard, sometimes choke anyway, to keep risk.
 *		  11/10/89: if hard, rarely vomit anyway, for slim chance.
 */
STATIC_OVL void
choke(food)	/* To a full belly all food is bad. (It.) */
	register struct obj *food;
{
	/* only happens if you were satiated */
	if (u.uhs != SATIATED) {
		if (!food || food->otyp != AMULET_OF_STRANGULATION)
			return;
	} else if (Role_if(PM_KNIGHT) && u.ualign.type == A_LAWFUL) {
			adjalign(-1);		/* gluttony is unchivalrous */
			You_feel("like a glutton!");
	}

	exercise(A_CON, FALSE);

	if (Breathless || (!Strangled && !rn2(20))) {
		/* choking by eating AoS doesn't involve stuffing yourself */
		if (food && food->otyp == AMULET_OF_STRANGULATION) {
			You("choke, but recover your composure.");
			return;
		}
		You("stuff yourself and then vomit voluminously.");
		morehungry(1000);	/* you just got *very* sick! */
		nomovemsg = 0;
		vomit();
	} else {
		killer_format = KILLED_BY_AN;
		/*
		 * Note all "killer"s below read "Choked on %s" on the
		 * high score list & tombstone.  So plan accordingly.
		 */
		if(food) {
			You("choke over your %s.", foodword(food));
			if (food->oclass == COIN_CLASS) {
				killer = "a very rich meal";
			} else {
				killer = food_xname(food, FALSE);
			}
		} else {
			You("choke over it.");
			killer = "quick snack";
		}
		You("die...");
		done(CHOKING);
	}
}

/* modify object wt. depending on time spent consuming it */
STATIC_OVL void
recalc_wt()
{
	struct obj *piece = victual.piece;

#ifdef DEBUG
	debugpline("Old weight = %d", piece->owt);
	debugpline("Used time = %d, Req'd time = %d",
		victual.usedtime, victual.reqtime);
#endif
	piece->owt = weight(piece);
#ifdef DEBUG
	debugpline("New weight = %d", piece->owt);
#endif
}

void
reset_eat()		/* called when eating interrupted by an event */
{
    /* we only set a flag here - the actual reset process is done after
     * the round is spent eating.
     */
	if(victual.eating && !victual.doreset) {
#ifdef DEBUG
	    debugpline("reset_eat...");
#endif
	    victual.doreset = TRUE;
	}
	return;
}

STATIC_OVL struct obj *
touchfood(otmp)
register struct obj *otmp;
{
	if (otmp->quan > 1L) {
	    if(!carried(otmp))
		(void) splitobj(otmp, otmp->quan - 1L);
	    else
		otmp = splitobj(otmp, 1L);
#ifdef DEBUG
	    debugpline("split object,");
#endif
	}

	if (!otmp->oeaten) {
	    if(((!carried(otmp) && costly_spot(otmp->ox, otmp->oy) &&
		 !otmp->no_charge)
		 || otmp->unpaid)) {
		/* create a dummy duplicate to put on bill */
		verbalize("You bit it, you bought it!");
		bill_dummy_object(otmp);
	    }
	    otmp->oeaten = (otmp->otyp == CORPSE ?
				mons[otmp->corpsenm].cnutrit :
				objects[otmp->otyp].oc_nutrition);
	}

	if (carried(otmp)) {
	    freeinv(otmp);
	    if (inv_cnt() >= 52) {
		sellobj_state(SELL_DONTSELL);
		dropy(otmp);
		sellobj_state(SELL_NORMAL);
	    } else {
		otmp->oxlth++;		/* hack to prevent merge */
		otmp = addinv(otmp);
		otmp->oxlth--;
	    }
	}
	return(otmp);
}

/* When food decays, in the middle of your meal, we don't want to dereference
 * any dangling pointers, so set it to null (which should still trigger
 * do_reset_eat() at the beginning of eatfood()) and check for null pointers
 * in do_reset_eat().
 */
void
food_disappears(obj)
register struct obj *obj;
{
	if (obj == victual.piece) victual.piece = (struct obj *)0;
	if (obj->timed) obj_stop_timers(obj);
}

/* renaming an object usually results in it having a different address;
   so the sequence start eating/opening, get interrupted, name the food,
   resume eating/opening would restart from scratch */
void
food_substitution(old_obj, new_obj)
struct obj *old_obj, *new_obj;
{
	if (old_obj == victual.piece) victual.piece = new_obj;
	if (old_obj == tin.tin) tin.tin = new_obj;
}

STATIC_OVL void
do_reset_eat()
{
#ifdef DEBUG
	debugpline("do_reset_eat...");
#endif
	if (victual.piece) {
		victual.piece = touchfood(victual.piece);
		recalc_wt();
	}
	victual.fullwarn = victual.eating = victual.doreset = FALSE;
	/* Do not set canchoke to FALSE; if we continue eating the same object
	 * we need to know if canchoke was set when they started eating it the
	 * previous time.  And if we don't continue eating the same object
	 * canchoke always gets recalculated anyway.
	 */
	stop_occupation();
	newuhs(FALSE);
}

STATIC_PTR
int
eatfood()		/* called each move during eating process */
{
	if(!victual.piece ||
	 (!carried(victual.piece) && !obj_here(victual.piece, u.ux, u.uy))) {
		/* maybe it was stolen? */
		do_reset_eat();
		return(0);
	}
	if(!victual.eating) return(0);

	if(++victual.usedtime <= victual.reqtime) {
	    if(bite()) return(0);
	    return(1);	/* still busy */
	} else {	/* done */
	    done_eating(TRUE);
	    return(0);
	}
}

STATIC_OVL void
done_eating(message)
boolean message;
{
	victual.piece->in_use = TRUE;
	occupation = 0; /* do this early, so newuhs() knows we're done */
	newuhs(FALSE);
	if (nomovemsg) {
		if (message) pline(nomovemsg);
		nomovemsg = 0;
	} else if (message)
		You("finish eating %s.", food_xname(victual.piece, TRUE));

	if(victual.piece->otyp == CORPSE)
		cpostfx(victual.piece->corpsenm);
	else
		fpostfx(victual.piece);

	if (carried(victual.piece)) useup(victual.piece);
	else useupf(victual.piece, 1L);
	victual.piece = (struct obj *) 0;
	victual.fullwarn = victual.eating = victual.doreset = FALSE;
}

STATIC_OVL void
cprefx(pm)
register int pm;
{
	if (!CANNIBAL_ALLOWED() && your_race(&mons[pm])) {
		if (Upolyd)
			You("have a bad feeling deep inside.");
		You("cannibal!  You will regret this!");
		HAggravate_monster |= FROMOUTSIDE;
		change_luck(-rn1(4,2));		/* -5..-2 */
	}

	if (touch_petrifies(&mons[pm]) || pm == PM_MEDUSA) {
	    if (!Stone_resistance &&
		!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
		Sprintf(killer_buf, "tasting %s meat", mons[pm].mname);
		killer_format = KILLED_BY;
		killer = killer_buf;
		You("turn to stone.");
		done(STONING);
		if (victual.piece)
		    victual.eating = FALSE;
		return; /* lifesaved */
	    }
	}

	switch(pm) {
	    case PM_LITTLE_DOG:
	    case PM_DOG:
	    case PM_LARGE_DOG:
	    case PM_KITTEN:
	    case PM_HOUSECAT:
	    case PM_LARGE_CAT:
		if (!CANNIBAL_ALLOWED()) {
		    You_feel("that eating the %s was a bad idea.", mons[pm].mname);
		    HAggravate_monster |= FROMOUTSIDE;
		}
		break;
	    case PM_LIZARD:
		if (Stoned) fix_petrification();
		break;
	    case PM_DEATH:
	    case PM_PESTILENCE:
	    case PM_FAMINE:
		{ char buf[BUFSZ];
		    pline("Eating that is instantly fatal.");
		    Sprintf(buf, "unwisely ate the body of %s",
			    mons[pm].mname);
		    killer = buf;
		    killer_format = NO_KILLER_PREFIX;
		    done(DIED);
		    /* It so happens that since we know these monsters */
		    /* cannot appear in tins, victual.piece will always */
		    /* be what we want, which is not generally true. */
		    if (revive_corpse(victual.piece))
			victual.piece = (struct obj *)0;
		    return;
		}
	    case PM_GREEN_SLIME:
		if (!Slimed && !Unchanging &&
			youmonst.data != &mons[PM_FIRE_VORTEX] &&
			youmonst.data != &mons[PM_FIRE_ELEMENTAL] &&
			youmonst.data != &mons[PM_SALAMANDER] &&
			youmonst.data != &mons[PM_GREEN_SLIME]) {
		    You("don't feel very well.");
		    Slimed = 10L;
		    flags.botl = 1;
		}
		/* Fall through */
	    default:
		if (acidic(&mons[pm]) && Stoned)
		    fix_petrification();
		break;
	}
}

void
fix_petrification()
{
	Stoned = 0;
	delayed_killer = 0;
	if (Hallucination)
	    pline("What a pity - you just ruined a future piece of %sart!",
		  ACURR(A_CHA) > 15 ? "fine " : "");
	else
	    You_feel("limber!");
}

/*
 * If you add an intrinsic that can be gotten by eating a monster, add it
 * to intrinsic_possible() and givit().  (It must already be in prop.h to
 * be an intrinsic property.)
 * It would be very easy to make the intrinsics not try to give you one
 * that you already had by checking to see if you have it in
 * intrinsic_possible() instead of givit().
 */

/* intrinsic_possible() returns TRUE iff a monster can give an intrinsic. */
STATIC_OVL int
intrinsic_possible(type, ptr)
int type;
register struct permonst *ptr;
{
	switch (type) {
	    case FIRE_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_FIRE) {
			debugpline("can get fire resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_FIRE);
#endif
	    case SLEEP_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_SLEEP) {
			debugpline("can get sleep resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_SLEEP);
#endif
	    case COLD_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_COLD) {
			debugpline("can get cold resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_COLD);
#endif
	    case DISINT_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_DISINT) {
			debugpline("can get disintegration resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_DISINT);
#endif
	    case SHOCK_RES:	/* shock (electricity) resistance */
#ifdef DEBUG
		if (ptr->mconveys & MR_ELEC) {
			debugpline("can get shock resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_ELEC);
#endif
	    case POISON_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_POISON) {
			debugpline("can get poison resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_POISON);
#endif
	    case TELEPORT:
#ifdef DEBUG
		if (can_teleport(ptr)) {
			debugpline("can get teleport");
			return(TRUE);
		} else  return(FALSE);
#else
		return(can_teleport(ptr));
#endif
	    case TELEPORT_CONTROL:
#ifdef DEBUG
		if (control_teleport(ptr)) {
			debugpline("can get teleport control");
			return(TRUE);
		} else  return(FALSE);
#else
		return(control_teleport(ptr));
#endif
	    case TELEPAT:
#ifdef DEBUG
		if (telepathic(ptr)) {
			debugpline("can get telepathy");
			return(TRUE);
		} else  return(FALSE);
#else
		return(telepathic(ptr));
#endif
	    default:
		return(FALSE);
	}
	/*NOTREACHED*/
}

/* givit() tries to give you an intrinsic based on the monster's level
 * and what type of intrinsic it is trying to give you.
 */
STATIC_OVL void
givit(type, ptr)
int type;
register struct permonst *ptr;
{
	register int chance;

#ifdef DEBUG
	debugpline("Attempting to give intrinsic %d", type);
#endif
	/* some intrinsics are easier to get than others */
	switch (type) {
		case POISON_RES:
			if ((ptr == &mons[PM_KILLER_BEE] ||
					ptr == &mons[PM_SCORPION]) && !rn2(4))
				chance = 1;
			else
				chance = 15;
			break;
		case TELEPORT:
			chance = 10;
			break;
		case TELEPORT_CONTROL:
			chance = 12;
			break;
		case TELEPAT:
			chance = 1;
			break;
		default:
			chance = 15;
			break;
	}

	if (ptr->mlevel <= rn2(chance))
		return;		/* failed die roll */

	switch (type) {
	    case FIRE_RES:
#ifdef DEBUG
		debugpline("Trying to give fire resistance");
#endif
		if(!(HFire_resistance & FROMOUTSIDE)) {
			You(Hallucination ? "be chillin'." :
			    "feel a momentary chill.");
			HFire_resistance |= FROMOUTSIDE;
		}
		break;
	    case SLEEP_RES:
#ifdef DEBUG
		debugpline("Trying to give sleep resistance");
#endif
		if(!(HSleep_resistance & FROMOUTSIDE)) {
			You_feel("wide awake.");
			HSleep_resistance |= FROMOUTSIDE;
		}
		break;
	    case COLD_RES:
#ifdef DEBUG
		debugpline("Trying to give cold resistance");
#endif
		if(!(HCold_resistance & FROMOUTSIDE)) {
			You_feel("full of hot air.");
			HCold_resistance |= FROMOUTSIDE;
		}
		break;
	    case DISINT_RES:
#ifdef DEBUG
		debugpline("Trying to give disintegration resistance");
#endif
		if(!(HDisint_resistance & FROMOUTSIDE)) {
			You_feel(Hallucination ?
			    "totally together, man." :
			    "very firm.");
			HDisint_resistance |= FROMOUTSIDE;
		}
		break;
	    case SHOCK_RES:	/* shock (electricity) resistance */
#ifdef DEBUG
		debugpline("Trying to give shock resistance");
#endif
		if(!(HShock_resistance & FROMOUTSIDE)) {
			if (Hallucination)
				You_feel("grounded in reality.");
			else
				Your("health currently feels amplified!");
			HShock_resistance |= FROMOUTSIDE;
		}
		break;
	    case POISON_RES:
#ifdef DEBUG
		debugpline("Trying to give poison resistance");
#endif
		if(!(HPoison_resistance & FROMOUTSIDE)) {
			You_feel(Poison_resistance ?
				 "especially healthy." : "healthy.");
			HPoison_resistance |= FROMOUTSIDE;
		}
		break;
	    case TELEPORT:
#ifdef DEBUG
		debugpline("Trying to give teleport");
#endif
		if(!(HTeleportation & FROMOUTSIDE)) {
			You_feel(Hallucination ? "diffuse." :
			    "very jumpy.");
			HTeleportation |= FROMOUTSIDE;
		}
		break;
	    case TELEPORT_CONTROL:
#ifdef DEBUG
		debugpline("Trying to give teleport control");
#endif
		if(!(HTeleport_control & FROMOUTSIDE)) {
			You_feel(Hallucination ?
			    "centered in your personal space." :
			    "in control of yourself.");
			HTeleport_control |= FROMOUTSIDE;
		}
		break;
	    case TELEPAT:
#ifdef DEBUG
		debugpline("Trying to give telepathy");
#endif
		if(!(HTelepat & FROMOUTSIDE)) {
			You_feel(Hallucination ?
			    "in touch with the cosmos." :
			    "a strange mental acuity.");
			HTelepat |= FROMOUTSIDE;
			/* If blind, make sure monsters show up. */
			if (Blind) see_monsters();
		}
		break;
	    default:
#ifdef DEBUG
		debugpline("Tried to give an impossible intrinsic");
#endif
		break;
	}
}

STATIC_OVL void
cpostfx(pm)		/* called after completely consuming a corpse */
register int pm;
{
	register int tmp = 0;
	boolean catch_lycanthropy = FALSE;

	/* in case `afternmv' didn't get called for previously mimicking
	   gold, clean up now to avoid `eatmbuf' memory leak */
	if (eatmbuf) (void)eatmdone();

	switch(pm) {
	    case PM_NEWT:
		/* MRKR: "eye of newt" may give small magical energy boost */
		if (rn2(3) || 3 * u.uen <= 2 * u.uenmax) {
		    int old_uen = u.uen;
		    u.uen += rnd(3);
		    if (u.uen > u.uenmax) {
			if (!rn2(3)) u.uenmax++;
			u.uen = u.uenmax;
		    }
		    if (old_uen != u.uen) {
			    You_feel("a mild buzz.");
			    flags.botl = 1;
		    }
		}
		break;
	    case PM_WRAITH:
		pluslvl(FALSE);
		break;
	    case PM_HUMAN_WERERAT:
		catch_lycanthropy = TRUE;
		u.ulycn = PM_WERERAT;
		break;
	    case PM_HUMAN_WEREJACKAL:
		catch_lycanthropy = TRUE;
		u.ulycn = PM_WEREJACKAL;
		break;
	    case PM_HUMAN_WEREWOLF:
		catch_lycanthropy = TRUE;
		u.ulycn = PM_WEREWOLF;
		break;
	    case PM_NURSE:
		if (Upolyd) u.mh = u.mhmax;
		else u.uhp = u.uhpmax;
		flags.botl = 1;
		break;
	    case PM_STALKER:
		if(!Invis) {
			set_itimeout(&HInvis, (long)rn1(100, 50));
		} else {
			if (!(HInvis & INTRINSIC)) You_feel("hidden!");
			HInvis |= FROMOUTSIDE;
			HSee_invisible |= FROMOUTSIDE;
		}
		newsym(u.ux, u.uy);
		/* fall into next case */
	    case PM_YELLOW_LIGHT:
		/* fall into next case */
	    case PM_GIANT_BAT:
		make_stunned(HStun + 30,FALSE);
		/* fall into next case */
	    case PM_BAT:
		make_stunned(HStun + 30,FALSE);
		break;
	    case PM_GIANT_MIMIC:
		tmp += 10;
		/* fall into next case */
	    case PM_LARGE_MIMIC:
		tmp += 20;
		/* fall into next case */
	    case PM_SMALL_MIMIC:
		tmp += 20;
		if (youmonst.data->mlet != S_MIMIC && !Unchanging) {
		    char buf[BUFSZ];
		    You_cant("resist the temptation to mimic %s.",
			Hallucination ? "an orange" : "a pile of gold");
#ifdef STEED
                    /* A pile of gold can't ride. */
		    if (u.usteed) dismount_steed(DISMOUNT_FELL);
#endif
		    nomul(-tmp);
		    Sprintf(buf, Hallucination ?
			"You suddenly dread being peeled and mimick %s again!" :
			"You now prefer mimicking %s again.",
			an(Upolyd ? youmonst.data->mname : urace.noun));
		    eatmbuf = strcpy((char *) alloc(strlen(buf) + 1), buf);
		    nomovemsg = eatmbuf;
		    afternmv = eatmdone;
		    /* ??? what if this was set before? */
		    youmonst.m_ap_type = M_AP_OBJECT;
		    youmonst.mappearance = Hallucination ? ORANGE : GOLD_PIECE;
		    newsym(u.ux,u.uy);
		    curs_on_u();
		    /* make gold symbol show up now */
		    display_nhwindow(WIN_MAP, TRUE);
		}
		break;
	    case PM_QUANTUM_MECHANIC:
		Your("velocity suddenly seems very uncertain!");
		if (HFast & INTRINSIC) {
			HFast &= ~INTRINSIC;
			You("seem slower.");
		} else {
			HFast |= FROMOUTSIDE;
			You("seem faster.");
		}
		break;
	    case PM_LIZARD:
		if (HStun > 2)  make_stunned(2L,FALSE);
		if (HConfusion > 2)  make_confused(2L,FALSE);
		break;
	    case PM_CHAMELEON:
	    case PM_DOPPELGANGER:
	 /* case PM_SANDESTIN: */
		if (!Unchanging) {
		    You_feel("a change coming over you.");
		    polyself(FALSE);
		}
		break;
	    case PM_MIND_FLAYER:
	    case PM_MASTER_MIND_FLAYER:
		if (ABASE(A_INT) < ATTRMAX(A_INT)) {
			if (!rn2(2)) {
				pline("Yum! That was real brain food!");
				(void) adjattrib(A_INT, 1, FALSE);
				break;	/* don't give them telepathy, too */
			}
		}
		else {
			pline("For some reason, that tasted bland.");
		}
		/* fall through to default case */
	    default: {
		register struct permonst *ptr = &mons[pm];
		int i, count;

		if (dmgtype(ptr, AD_STUN) || dmgtype(ptr, AD_HALU) ||
		    pm == PM_VIOLET_FUNGUS) {
			pline ("Oh wow!  Great stuff!");
			make_hallucinated(HHallucination + 200,FALSE,0L);
		}
		if(is_giant(ptr)) gainstr((struct obj *)0, 0);

		/* Check the monster for all of the intrinsics.  If this
		 * monster can give more than one, pick one to try to give
		 * from among all it can give.
		 *
		 * If a monster can give 4 intrinsics then you have
		 * a 1/1 * 1/2 * 2/3 * 3/4 = 1/4 chance of getting the first,
		 * a 1/2 * 2/3 * 3/4 = 1/4 chance of getting the second,
		 * a 1/3 * 3/4 = 1/4 chance of getting the third,
		 * and a 1/4 chance of getting the fourth.
		 *
		 * And now a proof by induction:
		 * it works for 1 intrinsic (1 in 1 of getting it)
		 * for 2 you have a 1 in 2 chance of getting the second,
		 *	otherwise you keep the first
		 * for 3 you have a 1 in 3 chance of getting the third,
		 *	otherwise you keep the first or the second
		 * for n+1 you have a 1 in n+1 chance of getting the (n+1)st,
		 *	otherwise you keep the previous one.
		 * Elliott Kleinrock, October 5, 1990
		 */

		 count = 0;	/* number of possible intrinsics */
		 tmp = 0;	/* which one we will try to give */
		 for (i = 1; i <= LAST_PROP; i++) {
			if (intrinsic_possible(i, ptr)) {
				count++;
				/* a 1 in count chance of replacing the old
				 * one with this one, and a count-1 in count
				 * chance of keeping the old one.  (note
				 * that 1 in 1 and 0 in 1 are what we want
				 * for the first one
				 */
				if (!rn2(count)) {
#ifdef DEBUG
					debugpline("Intrinsic %d replacing %d",
								i, tmp);
#endif
					tmp = i;
				}
			}
		 }

		 /* if any found try to give them one */
		 if (count) givit(tmp, ptr);
	    }
	    break;
	}

	if (catch_lycanthropy && defends(AD_WERE, uwep)) {
	    if (!touch_artifact(uwep, &youmonst)) {
		dropx(uwep);
		uwepgone();
	    }
	}

	return;
}

void
violated_vegetarian()
{
    u.uconduct.unvegetarian++;
    if (Role_if(PM_MONK)) {
	You_feel("guilty.");
	adjalign(-1);
    }
    return;
}

STATIC_PTR
int
opentin()		/* called during each move whilst opening a tin */
{
	register int r;
	const char *what;
	int which;

	if(!carried(tin.tin) && !obj_here(tin.tin, u.ux, u.uy))
					/* perhaps it was stolen? */
		return(0);		/* %% probably we should use tinoid */
	if(tin.usedtime++ >= 50) {
		You("give up your attempt to open the tin.");
		return(0);
	}
	if(tin.usedtime < tin.reqtime)
		return(1);		/* still busy */
	if(tin.tin->otrapped ||
	   (tin.tin->cursed && tin.tin->spe != -1 && !rn2(8))) {
		b_trapped("tin", 0);
		goto use_me;
	}
	You("succeed in opening the tin.");
	if(tin.tin->spe != 1) {
	    if (tin.tin->corpsenm == NON_PM) {
		pline("It turns out to be empty.");
		tin.tin->dknown = tin.tin->known = TRUE;
		goto use_me;
	    }
	    r = tin.tin->cursed ? ROTTEN_TIN :	/* always rotten if cursed */
		    (tin.tin->spe == -1) ? HOMEMADE_TIN :  /* player made it */
			rn2(TTSZ-1);		/* else take your pick */
	    if (r == ROTTEN_TIN && (tin.tin->corpsenm == PM_LIZARD ||
			tin.tin->corpsenm == PM_LICHEN))
		r = HOMEMADE_TIN;		/* lizards don't rot */
	    else if (tin.tin->spe == -1 && !tin.tin->blessed && !rn2(7))
		r = ROTTEN_TIN;			/* some homemade tins go bad */
	    which = 0;	/* 0=>plural, 1=>as-is, 2=>"the" prefix */
	    if (Hallucination) {
		what = rndmonnam();
	    } else {
		what = mons[tin.tin->corpsenm].mname;
		if (mons[tin.tin->corpsenm].geno & G_UNIQ)
		    which = type_is_pname(&mons[tin.tin->corpsenm]) ? 1 : 2;
	    }
	    if (which == 0) what = makeplural(what);
	    pline("It smells like %s%s.", (which == 2) ? "the " : "", what);
	    if (yn("Eat it?") == 'n') {
		if (!Hallucination) tin.tin->dknown = tin.tin->known = TRUE;
		if (flags.verbose) You("discard the open tin.");
		goto use_me;
	    }
	    /* in case stop_occupation() was called on previous meal */
	    victual.piece = (struct obj *)0;
	    victual.fullwarn = victual.eating = victual.doreset = FALSE;

	    You("consume %s %s.", tintxts[r].txt,
			mons[tin.tin->corpsenm].mname);

	    /* KMH, conduct */
	    u.uconduct.food++;
	    if (!vegan(&mons[tin.tin->corpsenm]))
		u.uconduct.unvegan++;
	    if (!vegetarian(&mons[tin.tin->corpsenm]))
		violated_vegetarian();

	    tin.tin->dknown = tin.tin->known = TRUE;
	    cprefx(tin.tin->corpsenm); cpostfx(tin.tin->corpsenm);

	    if(((!carried(tin.tin) && costly_spot(tin.tin->ox, tin.tin->oy) &&
		 !tin.tin->no_charge)
		|| tin.tin->unpaid)) {
		verbalize("You open it, you bought it!");
		bill_dummy_object(tin.tin);
	    }

	    /* check for vomiting added by GAN 01/16/87 */
	    if(tintxts[r].nut < 0) make_vomiting((long)rn1(15,10), FALSE);
	    else lesshungry(tintxts[r].nut);

	    if(r == 0 || r == FRENCH_FRIED_TIN) {
	        /* Assume !Glib, because you can't open tins when Glib. */
		incr_itimeout(&Glib, rnd(15));
		pline("Eating deep fried food made your %s very slippery.",
		      makeplural(body_part(FINGER)));
	    }
	} else {
	    if (tin.tin->cursed)
		pline("It contains some decaying%s%s substance.",
			Blind ? "" : " ", Blind ? "" : hcolor(NH_GREEN));
	    else
		pline("It contains spinach.");

	    if (yn("Eat it?") == 'n') {
		if (!Hallucination && !tin.tin->cursed)
		    tin.tin->dknown = tin.tin->known = TRUE;
		if (flags.verbose)
		    You("discard the open tin.");
		goto use_me;
	    }

	    tin.tin->dknown = tin.tin->known = TRUE;
	    if(((!carried(tin.tin) && costly_spot(tin.tin->ox, tin.tin->oy) &&
		 !tin.tin->no_charge)
		|| tin.tin->unpaid)) {
		verbalize("You open it, you bought it!");
		bill_dummy_object(tin.tin);
	    }

	    if (!tin.tin->cursed)
		pline("This makes you feel like %s!",
		      Hallucination ? "Swee'pea" : "Popeye");
	    lesshungry(600);
	    gainstr(tin.tin, 0);
	    u.uconduct.food++;
	}
use_me:
	if (carried(tin.tin)) useup(tin.tin);
	else useupf(tin.tin, 1L);
	tin.tin = (struct obj *) 0;
	return(0);
}

STATIC_OVL void
start_tin(otmp)		/* called when starting to open a tin */
	register struct obj *otmp;
{
	register int tmp;

	if (metallivorous(youmonst.data)) {
		You("bite right into the metal tin...");
		tmp = 1;
	} else if (nolimbs(youmonst.data)) {
		You("cannot handle the tin properly to open it.");
		return;
	} else if (otmp->blessed) {
		pline_The("tin opens like magic!");
		tmp = 1;
	} else if(uwep) {
		switch(uwep->otyp) {
		case TIN_OPENER:
			tmp = 1;
			break;
		case DAGGER:
		case SILVER_DAGGER:
		case ELVEN_DAGGER:
		case ORCISH_DAGGER:
		case ATHAME:
		case CRYSKNIFE:
			tmp = 3;
			break;
		case PICK_AXE:
		case AXE:
			tmp = 6;
			break;
		default:
			goto no_opener;
		}
		pline("Using your %s you try to open the tin.",
			aobjnam(uwep, (char *)0));
	} else {
no_opener:
		pline("It is not so easy to open this tin.");
		if(Glib) {
			pline_The("tin slips from your %s.",
			      makeplural(body_part(FINGER)));
			if(otmp->quan > 1L) {
			    otmp = splitobj(otmp, 1L);
			}
			if (carried(otmp)) dropx(otmp);
			else stackobj(otmp);
			return;
		}
		tmp = rn1(1 + 500/((int)(ACURR(A_DEX) + ACURRSTR)), 10);
	}
	tin.reqtime = tmp;
	tin.usedtime = 0;
	tin.tin = otmp;
	set_occupation(opentin, "opening the tin", 0);
	return;
}

int
Hear_again()		/* called when waking up after fainting */
{
	flags.soundok = 1;
	return 0;
}

/* called on the "first bite" of rotten food */
STATIC_OVL int
rottenfood(obj)
struct obj *obj;
{
	pline("Blecch!  Rotten %s!", foodword(obj));
	if(!rn2(4)) {
		if (Hallucination) You_feel("rather trippy.");
		else You_feel("rather %s.", body_part(LIGHT_HEADED));
		make_confused(HConfusion + d(2,4),FALSE);
	} else if(!rn2(4) && !Blind) {
		pline("Everything suddenly goes dark.");
		make_blinded((long)d(2,10),FALSE);
		if (!Blind) Your(vision_clears);
	} else if(!rn2(3)) {
		const char *what, *where;
		if (!Blind)
		    what = "goes",  where = "dark";
		else if (Levitation || Is_airlevel(&u.uz) ||
			 Is_waterlevel(&u.uz))
		    what = "you lose control of",  where = "yourself";
		else
		    what = "you slap against the", where =
#ifdef STEED
			   (u.usteed) ? "saddle" :
#endif
			   surface(u.ux,u.uy);
		pline_The("world spins and %s %s.", what, where);
		flags.soundok = 0;
		nomul(-rnd(10));
		nomovemsg = "You are conscious again.";
		afternmv = Hear_again;
		return(1);
	}
	return(0);
}

STATIC_OVL int
eatcorpse(otmp)		/* called when a corpse is selected as food */
	register struct obj *otmp;
{
	int tp = 0, mnum = otmp->corpsenm;
	long rotted = 0L;
	boolean uniq = !!(mons[mnum].geno & G_UNIQ);
	int retcode = 0;
	boolean stoneable = (touch_petrifies(&mons[mnum]) && !Stone_resistance &&
				!poly_when_stoned(youmonst.data));

	/* KMH, conduct */
	if (!vegan(&mons[mnum])) u.uconduct.unvegan++;
	if (!vegetarian(&mons[mnum])) violated_vegetarian();

	if (mnum != PM_LIZARD && mnum != PM_LICHEN) {
		long age = peek_at_iced_corpse_age(otmp);

		rotted = (monstermoves - age)/(10L + rn2(20));
		if (otmp->cursed) rotted += 2L;
		else if (otmp->blessed) rotted -= 2L;
	}

	if (mnum != PM_ACID_BLOB && !stoneable && rotted > 5L) {
		pline("Ulch - that %s was tainted!",
		      mons[mnum].mlet == S_FUNGUS ? "fungoid vegetation" :
		      !vegetarian(&mons[mnum]) ? "meat" : "protoplasm");
		if (Sick_resistance) {
			pline("It doesn't seem at all sickening, though...");
		} else {
			char buf[BUFSZ];
			long sick_time;

			sick_time = (long) rn1(10, 10);
			/* make sure new ill doesn't result in improvement */
			if (Sick && (sick_time > Sick))
			    sick_time = (Sick > 1L) ? Sick - 1L : 1L;
			if (!uniq)
			    Sprintf(buf, "rotted %s", corpse_xname(otmp,TRUE));
			else
			    Sprintf(buf, "%s%s rotted corpse",
				    !type_is_pname(&mons[mnum]) ? "the " : "",
				    s_suffix(mons[mnum].mname));
			make_sick(sick_time, buf, TRUE, SICK_VOMITABLE);
		}
		if (carried(otmp)) useup(otmp);
		else useupf(otmp, 1L);
		return(2);
	} else if (acidic(&mons[mnum]) && !Acid_resistance) {
		tp++;
		You("have a very bad case of stomach acid."); /* not body_part() */
		losehp(rnd(15), "acidic corpse", KILLED_BY_AN);
	} else if (poisonous(&mons[mnum]) && rn2(5)) {
		tp++;
		pline("Ecch - that must have been poisonous!");
		if(!Poison_resistance) {
			losestr(rnd(4));
			losehp(rnd(15), "poisonous corpse", KILLED_BY_AN);
		} else	You("seem unaffected by the poison.");
	/* now any corpse left too long will make you mildly ill */
	} else if ((rotted > 5L || (rotted > 3L && rn2(5)))
					&& !Sick_resistance) {
		tp++;
		You_feel("%ssick.", (Sick) ? "very " : "");
		losehp(rnd(8), "cadaver", KILLED_BY_AN);
	}

	/* delay is weight dependent */
	victual.reqtime = 3 + (mons[mnum].cwt >> 6);

	if (!tp && mnum != PM_LIZARD && mnum != PM_LICHEN &&
			(otmp->orotten || !rn2(7))) {
	    if (rottenfood(otmp)) {
		otmp->orotten = TRUE;
		(void)touchfood(otmp);
		retcode = 1;
	    }

	    if (!mons[otmp->corpsenm].cnutrit) {
		/* no nutrution: rots away, no message if you passed out */
		if (!retcode) pline_The("corpse rots away completely.");
		if (carried(otmp)) useup(otmp);
		else useupf(otmp, 1L);
		retcode = 2;
	    }
		    
	    if (!retcode) consume_oeaten(otmp, 2);	/* oeaten >>= 2 */
	} else {
	    pline("%s%s %s!",
		  !uniq ? "This " : !type_is_pname(&mons[mnum]) ? "The " : "",
		  food_xname(otmp, FALSE),
		  (vegan(&mons[mnum]) ?
		   (!carnivorous(youmonst.data) && herbivorous(youmonst.data)) :
		   (carnivorous(youmonst.data) && !herbivorous(youmonst.data)))
		  ? "is delicious" : "tastes terrible");
	}

	return(retcode);
}

STATIC_OVL void
start_eating(otmp)		/* called as you start to eat */
	register struct obj *otmp;
{
#ifdef DEBUG
	debugpline("start_eating: %lx (victual = %lx)", otmp, victual.piece);
	debugpline("reqtime = %d", victual.reqtime);
	debugpline("(original reqtime = %d)", objects[otmp->otyp].oc_delay);
	debugpline("nmod = %d", victual.nmod);
	debugpline("oeaten = %d", otmp->oeaten);
#endif
	victual.fullwarn = victual.doreset = FALSE;
	victual.eating = TRUE;

	if (otmp->otyp == CORPSE) {
	    cprefx(victual.piece->corpsenm);
	    if (!victual.piece || !victual.eating) {
		/* rider revived, or died and lifesaved */
		return;
	    }
	}

	if (bite()) return;

	if (++victual.usedtime >= victual.reqtime) {
	    /* print "finish eating" message if they just resumed -dlc */
	    done_eating(victual.reqtime > 1 ? TRUE : FALSE);
	    return;
	}

	Sprintf(msgbuf, "eating %s", food_xname(otmp, TRUE));
	set_occupation(eatfood, msgbuf, 0);
}


/*
 * called on "first bite" of (non-corpse) food.
 * used for non-rotten non-tin non-corpse food
 */
STATIC_OVL void
fprefx(otmp)
struct obj *otmp;
{
	switch(otmp->otyp) {
	    case FOOD_RATION:
		if(u.uhunger <= 200)
		    pline(Hallucination ? "Oh wow, like, superior, man!" :
			  "That food really hit the spot!");
		else if(u.uhunger <= 700) pline("That satiated your %s!",
						body_part(STOMACH));
		break;
	    case TRIPE_RATION:
		if (carnivorous(youmonst.data) && !humanoid(youmonst.data))
		    pline("That tripe ration was surprisingly good!");
		else if (maybe_polyd(is_orc(youmonst.data), Race_if(PM_ORC)))
		    pline(Hallucination ? "Tastes great! Less filling!" :
			  "Mmm, tripe... not bad!");
		else {
		    pline("Yak - dog food!");
		    more_experienced(1,0);
		    newexplevel();
		    /* not cannibalism, but we use similar criteria
		       for deciding whether to be sickened by this meal */
		    if (rn2(2) && !CANNIBAL_ALLOWED())
			make_vomiting((long)rn1(victual.reqtime, 14), FALSE);
		}
		break;
	    case MEATBALL:
	    case MEAT_STICK:
	    case HUGE_CHUNK_OF_MEAT:
	    case MEAT_RING:
		goto give_feedback;
	     /* break; */
	    case CLOVE_OF_GARLIC:
		if (is_undead(youmonst.data)) {
			make_vomiting((long)rn1(victual.reqtime, 5), FALSE);
			break;
		}
		/* Fall through otherwise */
	    default:
		if (otmp->otyp==SLIME_MOLD && !otmp->cursed
			&& otmp->spe == current_fruit)
		    pline("My, that was a %s %s!",
			  Hallucination ? "primo" : "yummy",
			  singular(otmp, xname));
		else
#ifdef UNIX
		if (otmp->otyp == APPLE || otmp->otyp == PEAR) {
		    if (!Hallucination) pline("Core dumped.");
		    else {
/* This is based on an old Usenet joke, a fake a.out manual page */
			int x = rnd(100);
			if (x <= 75)
			    pline("Segmentation fault -- core dumped.");
			else if (x <= 99)
			    pline("Bus error -- core dumped.");
			else pline("Yo' mama -- core dumped.");
		    }
		} else
#endif
#ifdef MAC	/* KMH -- Why should Unix have all the fun? */
		if (otmp->otyp == APPLE) {
			pline("Delicious!  Must be a Macintosh!");
		} else
#endif
		if (otmp->otyp == EGG && stale_egg(otmp)) {
		    pline("Ugh.  Rotten egg.");	/* perhaps others like it */
		    make_vomiting(Vomiting+d(10,4), TRUE);
		} else
 give_feedback:
		    pline("This %s is %s", singular(otmp, xname),
		      otmp->cursed ? (Hallucination ? "grody!" : "terrible!") :
		      (otmp->otyp == CRAM_RATION
		      || otmp->otyp == K_RATION
		      || otmp->otyp == C_RATION)
		      ? "bland." :
		      Hallucination ? "gnarly!" : "delicious!");
		break;
	}
}

STATIC_OVL void
accessory_has_effect(otmp)
struct obj *otmp;
{
	pline("Magic spreads through your body as you digest the %s.",
	    otmp->oclass == RING_CLASS ? "ring" : "amulet");
}

STATIC_OVL void
eataccessory(otmp)
struct obj *otmp;
{
	int typ = otmp->otyp;
	long oldprop;

	/* Note: rings are not so common that this is unbalancing. */
	/* (How often do you even _find_ 3 rings of polymorph in a game?) */
	oldprop = u.uprops[objects[typ].oc_oprop].intrinsic;
	if (otmp == uleft || otmp == uright) {
	    Ring_gone(otmp);
	    if (u.uhp <= 0) return; /* died from sink fall */
	}
	otmp->known = otmp->dknown = 1; /* by taste */
	if (!rn2(otmp->oclass == RING_CLASS ? 3 : 5)) {
	  switch (otmp->otyp) {
	    default:
	        if (!objects[typ].oc_oprop) break; /* should never happen */

		if (!(u.uprops[objects[typ].oc_oprop].intrinsic & FROMOUTSIDE))
		    accessory_has_effect(otmp);

		u.uprops[objects[typ].oc_oprop].intrinsic |= FROMOUTSIDE;

		switch (typ) {
		  case RIN_SEE_INVISIBLE:
		    set_mimic_blocking();
		    see_monsters();
		    if (Invis && !oldprop && !ESee_invisible &&
				!perceives(youmonst.data) && !Blind) {
			newsym(u.ux,u.uy);
			pline("Suddenly you can see yourself.");
			makeknown(typ);
		    }
		    break;
		  case RIN_INVISIBILITY:
		    if (!oldprop && !EInvis && !BInvis &&
					!See_invisible && !Blind) {
			newsym(u.ux,u.uy);
			Your("body takes on a %s transparency...",
				Hallucination ? "normal" : "strange");
			makeknown(typ);
		    }
		    break;
		  case RIN_PROTECTION_FROM_SHAPE_CHAN:
		    rescham();
		    break;
		  case RIN_LEVITATION:
		    /* undo the `.intrinsic |= FROMOUTSIDE' done above */
		    u.uprops[LEVITATION].intrinsic = oldprop;
		    if (!Levitation) {
			float_up();
			incr_itimeout(&HLevitation, d(10,20));
			makeknown(typ);
		    }
		    break;
		}
		break;
	    case RIN_ADORNMENT:
		accessory_has_effect(otmp);
		if (adjattrib(A_CHA, otmp->spe, -1))
		    makeknown(typ);
		break;
	    case RIN_GAIN_STRENGTH:
		accessory_has_effect(otmp);
		if (adjattrib(A_STR, otmp->spe, -1))
		    makeknown(typ);
		break;
	    case RIN_GAIN_CONSTITUTION:
		accessory_has_effect(otmp);
		if (adjattrib(A_CON, otmp->spe, -1))
		    makeknown(typ);
		break;
	    case RIN_INCREASE_ACCURACY:
		accessory_has_effect(otmp);
		u.uhitinc += otmp->spe;
		break;
	    case RIN_INCREASE_DAMAGE:
		accessory_has_effect(otmp);
		u.udaminc += otmp->spe;
		break;
	    case RIN_PROTECTION:
		accessory_has_effect(otmp);
		HProtection |= FROMOUTSIDE;
		u.ublessed += otmp->spe;
		flags.botl = 1;
		break;
	    case RIN_FREE_ACTION:
		/* Give sleep resistance instead */
		if (!(HSleep_resistance & FROMOUTSIDE))
		    accessory_has_effect(otmp);
		if (!Sleep_resistance)
		    You_feel("wide awake.");
		HSleep_resistance |= FROMOUTSIDE;
		break;
	    case AMULET_OF_CHANGE:
		accessory_has_effect(otmp);
		makeknown(typ);
		change_sex();
		You("are suddenly very %s!",
		    flags.female ? "feminine" : "masculine");
		flags.botl = 1;
		break;
	    case AMULET_OF_STRANGULATION: /* bad idea! */
		/* no message--this gives no permanent effect */
		choke(otmp);
		break;
	    case AMULET_OF_RESTFUL_SLEEP: /* another bad idea! */
		if (!(HSleeping & FROMOUTSIDE))
		    accessory_has_effect(otmp);
		HSleeping = FROMOUTSIDE | rnd(100);
		break;
	    case RIN_SUSTAIN_ABILITY:
	    case AMULET_OF_UNCHANGING:
	    case AMULET_OF_LIFE_SAVING:
	    case AMULET_OF_REFLECTION: /* nice try */
	    /* can't eat Amulet of Yendor or fakes,
	     * and no oc_prop even if you could -3.
	     */
		break;
	  }
	}
}

STATIC_OVL void
eatspecial() /* called after eating non-food */
{
	register struct obj *otmp = victual.piece;

	lesshungry(victual.nmod);
	victual.piece = (struct obj *)0;
	victual.eating = 0;
	if (otmp->oclass == COIN_CLASS) {
#ifdef GOLDOBJ
		if (carried(otmp))
		    useupall(otmp);
		else
#endif
		dealloc_obj(otmp);
		return;
	}
	if (otmp->oclass == POTION_CLASS) {
		otmp->quan++; /* dopotion() does a useup() */
		(void)dopotion(otmp);
	}
	if (otmp->oclass == RING_CLASS || otmp->oclass == AMULET_CLASS)
		eataccessory(otmp);
	else if (otmp->otyp == LEASH && otmp->leashmon)
		o_unleash(otmp);

	/* KMH -- idea by "Tommy the Terrorist" */
	if ((otmp->otyp == TRIDENT) && !otmp->cursed)
	{
		pline(Hallucination ? "Four out of five dentists agree." :
				"That was pure chewing satisfaction!");
		exercise(A_WIS, TRUE);
	}
	if ((otmp->otyp == FLINT) && !otmp->cursed)
	{
		pline("Yabba-dabba delicious!");
		exercise(A_CON, TRUE);
	}

	if (otmp == uwep && otmp->quan == 1L) uwepgone();
	if (otmp == uquiver && otmp->quan == 1L) uqwepgone();
	if (otmp == uswapwep && otmp->quan == 1L) uswapwepgone();

	if (otmp == uball) unpunish();
	if (otmp == uchain) unpunish(); /* but no useup() */
	else if (carried(otmp)) useup(otmp);
	else useupf(otmp, 1L);
}

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
static const char *foodwords[] = {
	"meal", "liquid", "wax", "food", "meat",
	"paper", "cloth", "leather", "wood", "bone", "scale",
	"metal", "metal", "metal", "silver", "gold", "platinum", "mithril",
	"plastic", "glass", "rich food", "stone"
};

STATIC_OVL const char *
foodword(otmp)
register struct obj *otmp;
{
	if (otmp->oclass == FOOD_CLASS) return "food";
	if (otmp->oclass == GEM_CLASS &&
	    objects[otmp->otyp].oc_material == GLASS &&
	    otmp->dknown)
		makeknown(otmp->otyp);
	return foodwords[objects[otmp->otyp].oc_material];
}

STATIC_OVL void
fpostfx(otmp)		/* called after consuming (non-corpse) food */
register struct obj *otmp;
{
	switch(otmp->otyp) {
	    case SPRIG_OF_WOLFSBANE:
		if (u.ulycn >= LOW_PM || is_were(youmonst.data))
		    you_unwere(TRUE);
		break;
	    case CARROT:
		make_blinded((long)u.ucreamed,TRUE);
		break;
	    case FORTUNE_COOKIE:
		outrumor(bcsign(otmp), BY_COOKIE);
		if (!Blind) u.uconduct.literate++;
		break;
	    case LUMP_OF_ROYAL_JELLY:
		/* This stuff seems to be VERY healthy! */
		gainstr(otmp, 1);
		if (Upolyd) {
		    u.mh += otmp->cursed ? -rnd(20) : rnd(20);
		    if (u.mh > u.mhmax) {
			if (!rn2(17)) u.mhmax++;
			u.mh = u.mhmax;
		    } else if (u.mh <= 0) {
			rehumanize();
		    }
		} else {
		    u.uhp += otmp->cursed ? -rnd(20) : rnd(20);
		    if (u.uhp > u.uhpmax) {
			if(!rn2(17)) u.uhpmax++;
			u.uhp = u.uhpmax;
		    } else if (u.uhp <= 0) {
			killer_format = KILLED_BY_AN;
			killer = "rotten lump of royal jelly";
			done(POISONING);
		    }
		}
		if(!otmp->cursed) heal_legs();
		break;
	    case EGG:
		if (touch_petrifies(&mons[otmp->corpsenm])) {
		    if (!Stone_resistance &&
			!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
			if (!Stoned) Stoned = 5;
			killer_format = KILLED_BY_AN;
			Sprintf(killer_buf, "%s egg", mons[otmp->corpsenm].mname);
			delayed_killer = killer_buf;
		    }
		}
		break;
	    case EUCALYPTUS_LEAF:
		if (Sick && !otmp->cursed)
		    make_sick(0L, (char *)0, TRUE, SICK_ALL);
		if (Vomiting && !otmp->cursed)
		    make_vomiting(0L, TRUE);
		break;
	}
	return;
}

/*
 * return 0 if the food was not dangerous.
 * return 1 if the food was dangerous and you chose to stop.
 * return 2 if the food was dangerous and you chose to eat it anyway.
 */
STATIC_OVL int
edibility_prompts(otmp)
struct obj *otmp;
{
	/* blessed food detection granted you a one-use
	   ability to detect food that is unfit for consumption
	   or dangerous and avoid it. */

	char buf[BUFSZ], foodsmell[BUFSZ],
	     it_or_they[QBUFSZ], eat_it_anyway[QBUFSZ];
	boolean cadaver = (otmp->otyp == CORPSE),
		stoneorslime = FALSE;
	int material = objects[otmp->otyp].oc_material,
	    mnum = otmp->corpsenm;
	long rotted = 0L;

	Strcpy(foodsmell, Tobjnam(otmp, "smell"));
	Strcpy(it_or_they, (otmp->quan == 1L) ? "it" : "they");
	Sprintf(eat_it_anyway, "Eat %s anyway?",
		(otmp->quan == 1L) ? "it" : "one");

	if (cadaver || otmp->otyp == EGG || otmp->otyp == TIN) {
		/* These checks must match those in eatcorpse() */
		stoneorslime = (touch_petrifies(&mons[mnum]) &&
				!Stone_resistance &&
				!poly_when_stoned(youmonst.data));

		if (mnum == PM_GREEN_SLIME)
		    stoneorslime = (!Unchanging &&
			youmonst.data != &mons[PM_FIRE_VORTEX] &&
			youmonst.data != &mons[PM_FIRE_ELEMENTAL] &&
			youmonst.data != &mons[PM_SALAMANDER] &&
			youmonst.data != &mons[PM_GREEN_SLIME]);

		if (cadaver && mnum != PM_LIZARD && mnum != PM_LICHEN) {
			long age = peek_at_iced_corpse_age(otmp);
			/* worst case rather than random
			   in this calculation to force prompt */
			rotted = (monstermoves - age)/(10L + 0 /* was rn2(20) */);
			if (otmp->cursed) rotted += 2L;
			else if (otmp->blessed) rotted -= 2L;
		}
	}

	/*
	 * These problems with food should be checked in
	 * order from most detrimental to least detrimental.
	 */

	if (cadaver && mnum != PM_ACID_BLOB && rotted > 5L && !Sick_resistance) {
		/* Tainted meat */
		Sprintf(buf, "%s like %s could be tainted! %s",
			foodsmell, it_or_they, eat_it_anyway);
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (stoneorslime) {
		Sprintf(buf, "%s like %s could be something very dangerous! %s",
			foodsmell, it_or_they, eat_it_anyway);
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (otmp->orotten || (cadaver && rotted > 3L)) {
		/* Rotten */
		Sprintf(buf, "%s like %s could be rotten! %s",
			foodsmell, it_or_they, eat_it_anyway);
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (cadaver && poisonous(&mons[mnum]) && !Poison_resistance) {
		/* poisonous */
		Sprintf(buf, "%s like %s might be poisonous! %s",
			foodsmell, it_or_they, eat_it_anyway);
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (cadaver && !vegetarian(&mons[mnum]) &&
	    !u.uconduct.unvegetarian && Role_if(PM_MONK)) {
		Sprintf(buf, "%s unhealthy. %s",
			foodsmell, eat_it_anyway);
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (cadaver && acidic(&mons[mnum]) && !Acid_resistance) {
		Sprintf(buf, "%s rather acidic. %s",
			foodsmell, eat_it_anyway);
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (Upolyd && u.umonnum == PM_RUST_MONSTER &&
	    is_metallic(otmp) && otmp->oerodeproof) {
		Sprintf(buf, "%s disgusting to you right now. %s",
			foodsmell, eat_it_anyway);
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}

	/*
	 * Breaks conduct, but otherwise safe.
	 */
	 
	if (!u.uconduct.unvegan &&
	    ((material == LEATHER || material == BONE ||
	      material == DRAGON_HIDE || material == WAX) ||
	     (cadaver && !vegan(&mons[mnum])))) {
		Sprintf(buf, "%s foul and unfamiliar to you. %s",
			foodsmell, eat_it_anyway);
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (!u.uconduct.unvegetarian &&
	    ((material == LEATHER || material == BONE ||
	      material == DRAGON_HIDE) ||
	     (cadaver && !vegetarian(&mons[mnum])))) {
		Sprintf(buf, "%s unfamiliar to you. %s",
			foodsmell, eat_it_anyway);
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}

	if (cadaver && mnum != PM_ACID_BLOB && rotted > 5L && Sick_resistance) {
		/* Tainted meat with Sick_resistance */
		Sprintf(buf, "%s like %s could be tainted! %s",
			foodsmell, it_or_they, eat_it_anyway);
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	return 0;
}

int
doeat()		/* generic "eat" command funtion (see cmd.c) */
{
	register struct obj *otmp;
	int basenutrit;			/* nutrition of full item */
	boolean dont_start = FALSE;
	
	if (Strangled) {
		pline("If you can't breathe air, how can you consume solids?");
		return 0;
	}
	if (!(otmp = floorfood("eat", 0))) return 0;
	if (check_capacity((char *)0)) return 0;

	if (u.uedibility) {
		int res = edibility_prompts(otmp);
		if (res) {
		    Your("%s stops tingling and your sense of smell returns to normal.",
			body_part(NOSE));
		    u.uedibility = 0;
		    if (res == 1) return 0;
		}
	}

	/* We have to make non-foods take 1 move to eat, unless we want to
	 * do ridiculous amounts of coding to deal with partly eaten plate
	 * mails, players who polymorph back to human in the middle of their
	 * metallic meal, etc....
	 */
	if (!is_edible(otmp)) {
	    You("cannot eat that!");
	    return 0;
	} else if ((otmp->owornmask & (W_ARMOR|W_TOOL|W_AMUL
#ifdef STEED
			|W_SADDLE
#endif
			)) != 0) {
	    /* let them eat rings */
	    You_cant("eat %s you're wearing.", something);
	    return 0;
	}
	if (is_metallic(otmp) &&
	    u.umonnum == PM_RUST_MONSTER && otmp->oerodeproof) {
	    	otmp->rknown = TRUE;
		if (otmp->quan > 1L) {
		    if(!carried(otmp))
			(void) splitobj(otmp, otmp->quan - 1L);
		    else
			otmp = splitobj(otmp, 1L);
		}
		pline("Ulch - That %s was rustproofed!", xname(otmp));
		/* The regurgitated object's rustproofing is gone now */
		otmp->oerodeproof = 0;
		make_stunned(HStun + rn2(10), TRUE);
		You("spit %s out onto the %s.", the(xname(otmp)),
			surface(u.ux, u.uy));
		if (carried(otmp)) {
			freeinv(otmp);
			dropy(otmp);
		}
		stackobj(otmp);
		return 1;
	}
	/* KMH -- Slow digestion is... undigestable */
	if (otmp->otyp == RIN_SLOW_DIGESTION) {
		pline("This ring is undigestable!");
		(void) rottenfood(otmp);
		if (otmp->dknown && !objects[otmp->otyp].oc_name_known
				&& !objects[otmp->otyp].oc_uname)
			docall(otmp);
		return (1);
	}
	if (otmp->oclass != FOOD_CLASS) {
	    int material;
	    victual.reqtime = 1;
	    victual.piece = otmp;
		/* Don't split it, we don't need to if it's 1 move */
	    victual.usedtime = 0;
	    victual.canchoke = (u.uhs == SATIATED);
		/* Note: gold weighs 1 pt. for each 1000 pieces (see */
		/* pickup.c) so gold and non-gold is consistent. */
	    if (otmp->oclass == COIN_CLASS)
		basenutrit = ((otmp->quan > 200000L) ? 2000
			: (int)(otmp->quan/100L));
	    else if(otmp->oclass == BALL_CLASS || otmp->oclass == CHAIN_CLASS)
		basenutrit = weight(otmp);
	    /* oc_nutrition is usually weight anyway */
	    else basenutrit = objects[otmp->otyp].oc_nutrition;
	    victual.nmod = basenutrit;
	    victual.eating = TRUE; /* needed for lesshungry() */

	    material = objects[otmp->otyp].oc_material;
	    if (material == LEATHER ||
		material == BONE || material == DRAGON_HIDE) {
		u.uconduct.unvegan++;
		violated_vegetarian();
	    } else if (material == WAX)
		u.uconduct.unvegan++;
	    u.uconduct.food++;
	    
	    if (otmp->cursed)
		(void) rottenfood(otmp);

	    if (otmp->oclass == WEAPON_CLASS && otmp->opoisoned) {
		pline("Ecch - that must have been poisonous!");
		if(!Poison_resistance) {
		    losestr(rnd(4));
		    losehp(rnd(15), xname(otmp), KILLED_BY_AN);
		} else
		    You("seem unaffected by the poison.");
	    } else if (!otmp->cursed)
		pline("This %s is delicious!",
		      otmp->oclass == COIN_CLASS ? foodword(otmp) :
		      singular(otmp, xname));

	    eatspecial();
	    return 1;
	}

	if(otmp == victual.piece) {
	/* If they weren't able to choke, they don't suddenly become able to
	 * choke just because they were interrupted.  On the other hand, if
	 * they were able to choke before, if they lost food it's possible
	 * they shouldn't be able to choke now.
	 */
	    if (u.uhs != SATIATED) victual.canchoke = FALSE;
	    victual.piece = touchfood(otmp);
	    You("resume your meal.");
	    start_eating(victual.piece);
	    return(1);
	}

	/* nothing in progress - so try to find something. */
	/* tins are a special case */
	/* tins must also check conduct separately in case they're discarded */
	if(otmp->otyp == TIN) {
	    start_tin(otmp);
	    return(1);
	}

	/* KMH, conduct */
	u.uconduct.food++;

	victual.piece = otmp = touchfood(otmp);
	victual.usedtime = 0;

	/* Now we need to calculate delay and nutritional info.
	 * The base nutrition calculated here and in eatcorpse() accounts
	 * for normal vs. rotten food.  The reqtime and nutrit values are
	 * then adjusted in accordance with the amount of food left.
	 */
	if(otmp->otyp == CORPSE) {
	    int tmp = eatcorpse(otmp);
	    if (tmp == 2) {
		/* used up */
		victual.piece = (struct obj *)0;
		return(1);
	    } else if (tmp)
		dont_start = TRUE;
	    /* if not used up, eatcorpse sets up reqtime and may modify
	     * oeaten */
	} else {
	    /* No checks for WAX, LEATHER, BONE, DRAGON_HIDE.  These are
	     * all handled in the != FOOD_CLASS case, above */
	    switch (objects[otmp->otyp].oc_material) {
	    case FLESH:
		u.uconduct.unvegan++;
		if (otmp->otyp != EGG) {
		    violated_vegetarian();
		}
		break;

	    default:
		if (otmp->otyp == PANCAKE ||
		    otmp->otyp == FORTUNE_COOKIE || /* eggs */
		    otmp->otyp == CREAM_PIE ||
		    otmp->otyp == CANDY_BAR || /* milk */
		    otmp->otyp == LUMP_OF_ROYAL_JELLY)
		    u.uconduct.unvegan++;
		break;
	    }

	    victual.reqtime = objects[otmp->otyp].oc_delay;
	    if (otmp->otyp != FORTUNE_COOKIE &&
		(otmp->cursed ||
		 (((monstermoves - otmp->age) > (int) otmp->blessed ? 50:30) &&
		(otmp->orotten || !rn2(7))))) {

		if (rottenfood(otmp)) {
		    otmp->orotten = TRUE;
		    dont_start = TRUE;
		}
		consume_oeaten(otmp, 1);	/* oeaten >>= 1 */
	    } else fprefx(otmp);
	}

	/* re-calc the nutrition */
	if (otmp->otyp == CORPSE) basenutrit = mons[otmp->corpsenm].cnutrit;
	else basenutrit = objects[otmp->otyp].oc_nutrition;

#ifdef DEBUG
	debugpline("before rounddiv: victual.reqtime == %d", victual.reqtime);
	debugpline("oeaten == %d, basenutrit == %d", otmp->oeaten, basenutrit);
#endif
	victual.reqtime = (basenutrit == 0 ? 0 :
		rounddiv(victual.reqtime * (long)otmp->oeaten, basenutrit));
#ifdef DEBUG
	debugpline("after rounddiv: victual.reqtime == %d", victual.reqtime);
#endif
	/* calculate the modulo value (nutrit. units per round eating)
	 * note: this isn't exact - you actually lose a little nutrition
	 *	 due to this method.
	 * TODO: add in a "remainder" value to be given at the end of the
	 *	 meal.
	 */
	if (victual.reqtime == 0 || otmp->oeaten == 0)
	    /* possible if most has been eaten before */
	    victual.nmod = 0;
	else if ((int)otmp->oeaten >= victual.reqtime)
	    victual.nmod = -((int)otmp->oeaten / victual.reqtime);
	else
	    victual.nmod = victual.reqtime % otmp->oeaten;
	victual.canchoke = (u.uhs == SATIATED);

	if (!dont_start) start_eating(otmp);
	return(1);
}

/* Take a single bite from a piece of food, checking for choking and
 * modifying usedtime.  Returns 1 if they choked and survived, 0 otherwise.
 */
STATIC_OVL int
bite()
{
	if(victual.canchoke && u.uhunger >= 2000) {
		choke(victual.piece);
		return 1;
	}
	if (victual.doreset) {
		do_reset_eat();
		return 0;
	}
	force_save_hs = TRUE;
	if(victual.nmod < 0) {
		lesshungry(-victual.nmod);
		consume_oeaten(victual.piece, victual.nmod); /* -= -nmod */
	} else if(victual.nmod > 0 && (victual.usedtime % victual.nmod)) {
		lesshungry(1);
		consume_oeaten(victual.piece, -1);		  /* -= 1 */
	}
	force_save_hs = FALSE;
	recalc_wt();
	return 0;
}

#endif /* OVLB */
#ifdef OVL0

void
gethungry()	/* as time goes by - called by moveloop() and domove() */
{
	if (u.uinvulnerable) return;	/* you don't feel hungrier */

	if ((!u.usleep || !rn2(10))	/* slow metabolic rate while asleep */
		&& (carnivorous(youmonst.data) || herbivorous(youmonst.data))
		&& !Slow_digestion)
	    u.uhunger--;		/* ordinary food consumption */

	if (moves % 2) {	/* odd turns */
	    /* Regeneration uses up food, unless due to an artifact */
	    if (HRegeneration || ((ERegeneration & (~W_ART)) &&
				(ERegeneration != W_WEP || !uwep->oartifact)))
			u.uhunger--;
	    if (near_capacity() > SLT_ENCUMBER) u.uhunger--;
	} else {		/* even turns */
	    if (Hunger) u.uhunger--;
	    /* Conflict uses up food too */
	    if (HConflict || (EConflict & (~W_ARTI))) u.uhunger--;
	    /* +0 charged rings don't do anything, so don't affect hunger */
	    /* Slow digestion still uses ring hunger */
	    switch ((int)(moves % 20)) {	/* note: use even cases only */
	     case  4: if (uleft &&
			  (uleft->spe || !objects[uleft->otyp].oc_charged))
			    u.uhunger--;
		    break;
	     case  8: if (uamul) u.uhunger--;
		    break;
	     case 12: if (uright &&
			  (uright->spe || !objects[uright->otyp].oc_charged))
			    u.uhunger--;
		    break;
	     case 16: if (u.uhave.amulet) u.uhunger--;
		    break;
	     default: break;
	    }
	}
	newuhs(TRUE);
}

#endif /* OVL0 */
#ifdef OVLB

void
morehungry(num)	/* called after vomiting and after performing feats of magic */
register int num;
{
	u.uhunger -= num;
	newuhs(TRUE);
}


void
lesshungry(num)	/* called after eating (and after drinking fruit juice) */
register int num;
{
	/* See comments in newuhs() for discussion on force_save_hs */
	boolean iseating = (occupation == eatfood) || force_save_hs;
#ifdef DEBUG
	debugpline("lesshungry(%d)", num);
#endif
	u.uhunger += num;
	if(u.uhunger >= 2000) {
	    if (!iseating || victual.canchoke) {
		if (iseating) {
		    choke(victual.piece);
		    reset_eat();
		} else
		    choke(occupation == opentin ? tin.tin : (struct obj *)0);
		/* no reset_eat() */
	    }
	} else {
	    /* Have lesshungry() report when you're nearly full so all eating
	     * warns when you're about to choke.
	     */
	    if (u.uhunger >= 1500) {
		if (!victual.eating || (victual.eating && !victual.fullwarn)) {
		    pline("You're having a hard time getting all of it down.");
		    nomovemsg = "You're finally finished.";
		    if (!victual.eating)
			multi = -2;
		    else {
			victual.fullwarn = TRUE;
			if (victual.canchoke && victual.reqtime > 1) {
			    /* a one-gulp food will not survive a stop */
			    if (yn_function("Stop eating?",ynchars,'y')=='y') {
				reset_eat();
				nomovemsg = (char *)0;
			    }
			}
		    }
		}
	    }
	}
	newuhs(FALSE);
}

STATIC_PTR
int
unfaint()
{
	(void) Hear_again();
	if(u.uhs > FAINTING)
		u.uhs = FAINTING;
	stop_occupation();
	flags.botl = 1;
	return 0;
}

#endif /* OVLB */
#ifdef OVL0

boolean
is_fainted()
{
	return((boolean)(u.uhs == FAINTED));
}

void
reset_faint()	/* call when a faint must be prematurely terminated */
{
	if(is_fainted()) nomul(0);
}

#if 0
void
sync_hunger()
{

	if(is_fainted()) {

		flags.soundok = 0;
		nomul(-10+(u.uhunger/10));
		nomovemsg = "You regain consciousness.";
		afternmv = unfaint;
	}
}
#endif

void
newuhs(incr)		/* compute and comment on your (new?) hunger status */
boolean incr;
{
	unsigned newhs;
	static unsigned save_hs;
	static boolean saved_hs = FALSE;
	int h = u.uhunger;

	newhs = (h > 1000) ? SATIATED :
		(h > 150) ? NOT_HUNGRY :
		(h > 50) ? HUNGRY :
		(h > 0) ? WEAK : FAINTING;

	/* While you're eating, you may pass from WEAK to HUNGRY to NOT_HUNGRY.
	 * This should not produce the message "you only feel hungry now";
	 * that message should only appear if HUNGRY is an endpoint.  Therefore
	 * we check to see if we're in the middle of eating.  If so, we save
	 * the first hunger status, and at the end of eating we decide what
	 * message to print based on the _entire_ meal, not on each little bit.
	 */
	/* It is normally possible to check if you are in the middle of a meal
	 * by checking occupation == eatfood, but there is one special case:
	 * start_eating() can call bite() for your first bite before it
	 * sets the occupation.
	 * Anyone who wants to get that case to work _without_ an ugly static
	 * force_save_hs variable, feel free.
	 */
	/* Note: If you become a certain hunger status in the middle of the
	 * meal, and still have that same status at the end of the meal,
	 * this will incorrectly print the associated message at the end of
	 * the meal instead of the middle.  Such a case is currently
	 * impossible, but could become possible if a message for SATIATED
	 * were added or if HUNGRY and WEAK were separated by a big enough
	 * gap to fit two bites.
	 */
	if (occupation == eatfood || force_save_hs) {
		if (!saved_hs) {
			save_hs = u.uhs;
			saved_hs = TRUE;
		}
		u.uhs = newhs;
		return;
	} else {
		if (saved_hs) {
			u.uhs = save_hs;
			saved_hs = FALSE;
		}
	}

	if(newhs == FAINTING) {
		if(is_fainted()) newhs = FAINTED;
		if(u.uhs <= WEAK || rn2(20-u.uhunger/10) >= 19) {
			if(!is_fainted() && multi >= 0 /* %% */) {
				/* stop what you're doing, then faint */
				stop_occupation();
				You("faint from lack of food.");
				flags.soundok = 0;
				nomul(-10+(u.uhunger/10));
				nomovemsg = "You regain consciousness.";
				afternmv = unfaint;
				newhs = FAINTED;
			}
		} else
		if(u.uhunger < -(int)(200 + 20*ACURR(A_CON))) {
			u.uhs = STARVED;
			flags.botl = 1;
			bot();
			You("die from starvation.");
			killer_format = KILLED_BY;
			killer = "starvation";
			done(STARVING);
			/* if we return, we lifesaved, and that calls newuhs */
			return;
		}
	}

	if(newhs != u.uhs) {
		if(newhs >= WEAK && u.uhs < WEAK)
			losestr(1);	/* this may kill you -- see below */
		else if(newhs < WEAK && u.uhs >= WEAK)
			losestr(-1);
		switch(newhs){
		case HUNGRY:
			if (Hallucination) {
			    You((!incr) ?
				"now have a lesser case of the munchies." :
				"are getting the munchies.");
			} else
			    You((!incr) ? "only feel hungry now." :
				  (u.uhunger < 145) ? "feel hungry." :
				   "are beginning to feel hungry.");
			if (incr && occupation &&
			    (occupation != eatfood && occupation != opentin))
			    stop_occupation();
			break;
		case WEAK:
			if (Hallucination)
			    pline((!incr) ?
				  "You still have the munchies." :
      "The munchies are interfering with your motor capabilities.");
			else if (incr &&
				(Role_if(PM_WIZARD) || Race_if(PM_ELF) ||
				 Role_if(PM_VALKYRIE)))
			    pline("%s needs food, badly!",
				  (Role_if(PM_WIZARD) || Role_if(PM_VALKYRIE)) ?
				  urole.name.m : "Elf");
			else
			    You((!incr) ? "feel weak now." :
				  (u.uhunger < 45) ? "feel weak." :
				   "are beginning to feel weak.");
			if (incr && occupation &&
			    (occupation != eatfood && occupation != opentin))
			    stop_occupation();
			break;
		}
		u.uhs = newhs;
		flags.botl = 1;
		bot();
		if ((Upolyd ? u.mh : u.uhp) < 1) {
			You("die from hunger and exhaustion.");
			killer_format = KILLED_BY;
			killer = "exhaustion";
			done(STARVING);
			return;
		}
	}
}

#endif /* OVL0 */
#ifdef OVLB

/* Returns an object representing food.  Object may be either on floor or
 * in inventory.
 */
struct obj *
floorfood(verb,corpsecheck)	/* get food from floor or pack */
	const char *verb;
	int corpsecheck; /* 0, no check, 1, corpses, 2, tinnable corpses */
{
	register struct obj *otmp;
	char qbuf[QBUFSZ];
	char c;
	boolean feeding = (!strcmp(verb, "eat"));

	/* if we can't touch floor objects then use invent food only */
	if (!can_reach_floor() ||
#ifdef STEED
		(feeding && u.usteed) || /* can't eat off floor while riding */
#endif
		((is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)) &&
		    (Wwalking || is_clinger(youmonst.data) ||
			(Flying && !Breathless))))
	    goto skipfloor;

	if (feeding && metallivorous(youmonst.data)) {
	    struct obj *gold;
	    struct trap *ttmp = t_at(u.ux, u.uy);

	    if (ttmp && ttmp->tseen && ttmp->ttyp == BEAR_TRAP) {
		/* If not already stuck in the trap, perhaps there should
		   be a chance to becoming trapped?  Probably not, because
		   then the trap would just get eaten on the _next_ turn... */
		Sprintf(qbuf, "There is a bear trap here (%s); eat it?",
			(u.utrap && u.utraptype == TT_BEARTRAP) ?
				"holding you" : "armed");
		if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
		    u.utrap = u.utraptype = 0;
		    deltrap(ttmp);
		    return mksobj(BEARTRAP, TRUE, FALSE);
		} else if (c == 'q') {
		    return (struct obj *)0;
		}
	    }

	    if (youmonst.data != &mons[PM_RUST_MONSTER] &&
		(gold = g_at(u.ux, u.uy)) != 0) {
		if (gold->quan == 1L)
		    Sprintf(qbuf, "There is 1 gold piece here; eat it?");
		else
		    Sprintf(qbuf, "There are %ld gold pieces here; eat them?",
			    gold->quan);
		if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
		    obj_extract_self(gold);
		    return gold;
		} else if (c == 'q') {
		    return (struct obj *)0;
		}
	    }
	}

	/* Is there some food (probably a heavy corpse) here on the ground? */
	for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
		if(corpsecheck ?
		(otmp->otyp==CORPSE && (corpsecheck == 1 || tinnable(otmp))) :
		    feeding ? (otmp->oclass != COIN_CLASS && is_edible(otmp)) :
						otmp->oclass==FOOD_CLASS) {
			Sprintf(qbuf, "There %s %s here; %s %s?",
				otense(otmp, "are"),
				doname(otmp), verb,
				(otmp->quan == 1L) ? "it" : "one");
			if((c = yn_function(qbuf,ynqchars,'n')) == 'y')
				return(otmp);
			else if(c == 'q')
				return((struct obj *) 0);
		}
	}

 skipfloor:
	/* We cannot use ALL_CLASSES since that causes getobj() to skip its
	 * "ugly checks" and we need to check for inedible items.
	 */
	otmp = getobj(feeding ? (const char *)allobj :
				(const char *)comestibles, verb);
	if (corpsecheck && otmp)
	    if (otmp->otyp != CORPSE || (corpsecheck == 2 && !tinnable(otmp))) {
		You_cant("%s that!", verb);
		return (struct obj *)0;
	    }
	return otmp;
}

/* Side effects of vomiting */
/* added nomul (MRS) - it makes sense, you're too busy being sick! */
void
vomit()		/* A good idea from David Neves */
{
	make_sick(0L, (char *) 0, TRUE, SICK_VOMITABLE);
	nomul(-2);
}

int
eaten_stat(base, obj)
register int base;
register struct obj *obj;
{
	long uneaten_amt, full_amount;

	uneaten_amt = (long)obj->oeaten;
	full_amount = (obj->otyp == CORPSE) ? (long)mons[obj->corpsenm].cnutrit
					: (long)objects[obj->otyp].oc_nutrition;
	if (uneaten_amt > full_amount) {
	    impossible(
	  "partly eaten food (%ld) more nutritious than untouched food (%ld)",
		       uneaten_amt, full_amount);
	    uneaten_amt = full_amount;
	}

	base = (int)(full_amount ? (long)base * uneaten_amt / full_amount : 0L);
	return (base < 1) ? 1 : base;
}

/* reduce obj's oeaten field, making sure it never hits or passes 0 */
void
consume_oeaten(obj, amt)
struct obj *obj;
int amt;
{
    /*
     * This is a hack to try to squelch several long standing mystery
     * food bugs.  A better solution would be to rewrite the entire
     * victual handling mechanism from scratch using a less complex
     * model.  Alternatively, this routine could call done_eating()
     * or food_disappears() but its callers would need revisions to
     * cope with victual.piece unexpectedly going away.
     *
     * Multi-turn eating operates by setting the food's oeaten field
     * to its full nutritional value and then running a counter which
     * independently keeps track of whether there is any food left.
     * The oeaten field can reach exactly zero on the last turn, and
     * the object isn't removed from inventory until the next turn
     * when the "you finish eating" message gets delivered, so the
     * food would be restored to the status of untouched during that
     * interval.  This resulted in unexpected encumbrance messages
     * at the end of a meal (if near enough to a threshold) and would
     * yield full food if there was an interruption on the critical
     * turn.  Also, there have been reports over the years of food
     * becoming massively heavy or producing unlimited satiation;
     * this would occur if reducing oeaten via subtraction attempted
     * to drop it below 0 since its unsigned type would produce a
     * huge positive value instead.  So far, no one has figured out
     * _why_ that inappropriate subtraction might sometimes happen.
     */

    if (amt > 0) {
	/* bit shift to divide the remaining amount of food */
	obj->oeaten >>= amt;
    } else {
	/* simple decrement; value is negative so we actually add it */
	if ((int) obj->oeaten > -amt)
	    obj->oeaten += amt;
	else
	    obj->oeaten = 0;
    }

    if (obj->oeaten == 0) {
	if (obj == victual.piece)	/* always true unless wishing... */
	    victual.reqtime = victual.usedtime;	/* no bites left */
	obj->oeaten = 1;	/* smallest possible positive value */
    }
}

#endif /* OVLB */
#ifdef OVL1

/* called when eatfood occupation has been interrupted,
   or in the case of theft, is about to be interrupted */
boolean
maybe_finished_meal(stopping)
boolean stopping;
{
	/* in case consume_oeaten() has decided that the food is all gone */
	if (occupation == eatfood && victual.usedtime >= victual.reqtime) {
	    if (stopping) occupation = 0;	/* for do_reset_eat */
	    (void) eatfood(); /* calls done_eating() to use up victual.piece */
	    return TRUE;
	}
	return FALSE;
}

#endif /* OVL1 */

/*eat.c*/

/*	SCCS Id: @(#)mkobj.c	3.5	2007/01/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL void FDECL(mkbox_cnts,(struct obj *));
STATIC_DCL void FDECL(obj_timer_checks,(struct obj *, XCHAR_P, XCHAR_P, int));
STATIC_DCL void FDECL(container_weight, (struct obj *));
STATIC_DCL struct obj *FDECL(save_mtraits, (struct obj *, struct monst *));
#ifdef WIZARD
STATIC_DCL const char *FDECL(where_name, (int));
STATIC_DCL void FDECL(check_contained, (struct obj *,const char *));
#endif

extern struct obj *thrownobj;		/* defined in dothrow.c */

/*#define DEBUG_EFFECTS*/	/* show some messages for debugging */

struct icp {
    int  iprob;		/* probability of an item type */
    char iclass;	/* item class */
};

const struct icp mkobjprobs[] = {
{10, WEAPON_CLASS},
{10, ARMOR_CLASS},
{20, FOOD_CLASS},
{ 8, TOOL_CLASS},
{ 8, GEM_CLASS},
{16, POTION_CLASS},
{16, SCROLL_CLASS},
{ 4, SPBOOK_CLASS},
{ 4, WAND_CLASS},
{ 3, RING_CLASS},
{ 1, AMULET_CLASS}
};

const struct icp boxiprobs[] = {
{18, GEM_CLASS},
{15, FOOD_CLASS},
{18, POTION_CLASS},
{18, SCROLL_CLASS},
{12, SPBOOK_CLASS},
{ 7, COIN_CLASS},
{ 6, WAND_CLASS},
{ 5, RING_CLASS},
{ 1, AMULET_CLASS}
};

#ifdef REINCARNATION
const struct icp rogueprobs[] = {
{12, WEAPON_CLASS},
{12, ARMOR_CLASS},
{22, FOOD_CLASS},
{22, POTION_CLASS},
{22, SCROLL_CLASS},
{ 5, WAND_CLASS},
{ 5, RING_CLASS}
};
#endif

const struct icp hellprobs[] = {
{20, WEAPON_CLASS},
{20, ARMOR_CLASS},
{16, FOOD_CLASS},
{12, TOOL_CLASS},
{10, GEM_CLASS},
{ 1, POTION_CLASS},
{ 1, SCROLL_CLASS},
{ 8, WAND_CLASS},
{ 8, RING_CLASS},
{ 4, AMULET_CLASS}
};

struct oextra *
newoextra()
{
	struct oextra *oextra;
	oextra = (struct oextra *)alloc(sizeof(struct oextra));
	if (oextra)
	    (void) memset((genericptr_t)oextra, 0, sizeof(struct oextra));
	return oextra;
}

void
dealloc_oextra(x)
struct oextra *x;
{
	if (x) {
		if (x->oname) free((genericptr_t)x->oname);
		if (x->omonst) free((genericptr_t)x->omonst);
		if (x->omid) free((genericptr_t)x->omid);
		if (x->olong) free((genericptr_t)x->olong);
		if (x->omailcmd) free((genericptr_t)x->omailcmd);
		free((genericptr_t)x);
	}
}

void
newomonst(otmp)
struct obj *otmp;
{
	if (!otmp->oextra) otmp->oextra = newoextra();
	if (!OMONST(otmp)) {
	    OMONST(otmp) = (struct monst *)alloc(sizeof(struct monst));
	    (void) memset((genericptr_t) OMONST(otmp), 0, sizeof(struct monst));
	}
}

void
free_omonst(otmp)
struct obj *otmp;
{
	if (otmp->oextra && OMONST(otmp)) {
		free((genericptr_t) OMONST(otmp));
		OMONST(otmp) = (struct monst *)0;
	}
}

void
newomid(otmp)
struct obj *otmp;
{
	if (!otmp->oextra) otmp->oextra = newoextra();
	if (!OMID(otmp)) {
	    OMID(otmp) = (unsigned *)alloc(sizeof(unsigned));
	    (void) memset((genericptr_t) OMID(otmp), 0, sizeof(unsigned));
	}
}

void
free_omid(otmp)
struct obj *otmp;
{
	if (otmp->oextra && OMID(otmp)) {
		free((genericptr_t) OMID(otmp));
		OMID(otmp) = (unsigned *)0;
	}
}

void
newolong(otmp)
struct obj *otmp;
{
	if (!otmp->oextra) otmp->oextra = newoextra();
	if (!OLONG(otmp)) {
	    OLONG(otmp) = (long *)alloc(sizeof(long));
	    (void) memset((genericptr_t) OLONG(otmp), 0, sizeof(long));
	}
}

void
free_olong(otmp)
struct obj *otmp;
{
	if (otmp->oextra && OLONG(otmp)) {
		free((genericptr_t) OLONG(otmp));
		OLONG(otmp) = (long *)0;
	}
}

void
new_omailcmd(otmp, response_cmd)
struct obj *otmp;
const char *response_cmd;
{
	unsigned lth = strlen(response_cmd) + 1;

	if (!otmp->oextra) otmp->oextra = newoextra();
	if (OMAILCMD(otmp)) free_omailcmd(otmp);
	OMAILCMD(otmp) = (char *)alloc(lth);
	Strcpy(OMAILCMD(otmp), response_cmd);
}

void
free_omailcmd(otmp)
struct obj *otmp;
{
	if (otmp->oextra && OMAILCMD(otmp)) {
		free((genericptr_t) OMAILCMD(otmp));
		OMAILCMD(otmp) = (char *)0;
	}
}

struct obj *
mkobj_at(let, x, y, artif)
char let;
int x, y;
boolean artif;
{
	struct obj *otmp;

	otmp = mkobj(let, artif);
	place_object(otmp, x, y);
	return(otmp);
}

struct obj *
mksobj_at(otyp, x, y, init, artif)
int otyp, x, y;
boolean init, artif;
{
	struct obj *otmp;

	otmp = mksobj(otyp, init, artif);
	place_object(otmp, x, y);
	return(otmp);
}

struct obj *
mkobj(oclass, artif)
char oclass;
boolean artif;
{
	int tprob, i, prob = rnd(1000);

	if(oclass == RANDOM_CLASS) {
		const struct icp *iprobs =
#ifdef REINCARNATION
				    (Is_rogue_level(&u.uz)) ?
				    (const struct icp *)rogueprobs :
#endif
				    Inhell ? (const struct icp *)hellprobs :
				    (const struct icp *)mkobjprobs;

		for(tprob = rnd(100);
		    (tprob -= iprobs->iprob) > 0;
		    iprobs++);
		oclass = iprobs->iclass;
	}

	i = bases[(int)oclass];
	while((prob -= objects[i].oc_prob) > 0) i++;

	if(objects[i].oc_class != oclass || !OBJ_NAME(objects[i]))
		panic("probtype error, oclass=%d i=%d", (int) oclass, i);

	return(mksobj(i, TRUE, artif));
}

STATIC_OVL void
mkbox_cnts(box)
struct obj *box;
{
	register int n;
	register struct obj *otmp;

	box->cobj = (struct obj *) 0;

	switch (box->otyp) {
	case ICE_BOX:		n = 20; break;
	case CHEST:		n = 5; break;
	case LARGE_BOX:		n = 3; break;
	case SACK:
	case OILSKIN_SACK:
				/* initial inventory: sack starts out empty */
				if (moves <= 1 && !in_mklev) { n = 0; break; }
				/*else FALLTHRU*/
	case BAG_OF_HOLDING:	n = 1; break;
	default:		n = 0; break;
	}

	for (n = rn2(n+1); n > 0; n--) {
	    if (box->otyp == ICE_BOX) {
		if (!(otmp = mksobj(CORPSE, TRUE, TRUE))) continue;
		/* Note: setting age to 0 is correct.  Age has a different
		 * from usual meaning for objects stored in ice boxes. -KAA
		 */
		otmp->age = 0L;
		if (otmp->timed) {
		    (void) stop_timer(ROT_CORPSE, obj_to_any(otmp));
		    (void) stop_timer(REVIVE_MON, obj_to_any(otmp));
		}
	    } else {
		register int tprob;
		const struct icp *iprobs = boxiprobs;

		for (tprob = rnd(100); (tprob -= iprobs->iprob) > 0; iprobs++)
		    ;
		if (!(otmp = mkobj(iprobs->iclass, TRUE))) continue;

		/* handle a couple of special cases */
		if (otmp->oclass == COIN_CLASS) {
		    /* 2.5 x level's usual amount; weight adjusted below */
		    otmp->quan = (long)(rnd(level_difficulty()+2) * rnd(75));
		    otmp->owt = weight(otmp);
		} else while (otmp->otyp == ROCK) {
		    otmp->otyp = rnd_class(DILITHIUM_CRYSTAL, LOADSTONE);
		    if (otmp->quan > 2L) otmp->quan = 1L;
		    otmp->owt = weight(otmp);
		}
		if (box->otyp == BAG_OF_HOLDING) {
		    if (Is_mbag(otmp)) {
			otmp->otyp = SACK;
			otmp->spe = 0;
			otmp->owt = weight(otmp);
		    } else while (otmp->otyp == WAN_CANCELLATION)
			    otmp->otyp = rnd_class(WAN_LIGHT, WAN_LIGHTNING);
		}
	    }
	    (void) add_to_container(box, otmp);
	}
}

int
rndmonnum()	/* select a random, common monster type */
{
	register struct permonst *ptr;
	register int	i;
	unsigned short excludeflags;

	/* Plan A: get a level-appropriate common monster */
	ptr = rndmonst();
	if (ptr) return(monsndx(ptr));

	/* Plan B: get any common monster */
	excludeflags = G_UNIQ | G_NOGEN | (Inhell ? G_NOHELL : G_HELL);
	do {
	    i = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
	    ptr = &mons[i];
	} while ((ptr->geno & excludeflags) != 0);

	return(i);
}

void
copy_oextra(obj2, obj1)
struct obj *obj2, *obj1;
{
	if(!obj2 || !obj1 || !obj1->oextra) return;

	if (!obj2->oextra) obj2->oextra = newoextra();
	if (has_oname(obj1)) oname(obj2, ONAME(obj1));
	if (has_omonst(obj1)) {
		if (!OMONST(obj2)) newomonst(obj2);
		(void)memcpy((genericptr_t)OMONST(obj2),
				(genericptr_t)OMONST(obj1),
				sizeof(struct monst));
		OMONST(obj2)->mextra = (struct mextra *)0;
		OMONST(obj2)->nmon = (struct monst *)0;
#if 0
		OMONST(obj2)->m_id = context.ident++;
		if (OMONST(obj2)->m_id) /* ident overflowed */
			OMONST(obj2)->m_id = context.ident++;
#endif
		if (OMONST(obj1)->mextra)
			copy_mextra(OMONST(obj2),OMONST(obj1));
	}
	if (has_omid(obj1)) {
		if (!OMID(obj2)) newomid(obj2);
		(void)memcpy((genericptr_t)OMID(obj2),
				(genericptr_t)OMID(obj1),
				sizeof(unsigned));
	}
	if (has_olong(obj1)) {
		if (!OLONG(obj2)) newolong(obj2);
		(void)memcpy((genericptr_t)OLONG(obj2),
				(genericptr_t)OLONG(obj1),
				sizeof(long));
	}
	if (has_omailcmd(obj1)) {
		new_omailcmd(obj2, OMAILCMD(obj1));
	}
}

/*
 * Split obj so that it gets size gets reduced by num. The quantity num is
 * put in the object structure delivered by this call.  The returned object
 * has its wornmask cleared and is positioned just following the original
 * in the nobj chain (and nexthere chain when on the floor).
 */
struct obj *
splitobj(obj, num)
struct obj *obj;
long num;
{
	struct obj *otmp;

	if (obj->cobj || num <= 0L || obj->quan <= num)
	    panic("splitobj");	/* can't split containers */
	otmp = newobj();
	*otmp = *obj;		/* copies whole structure */
	otmp->oextra = (struct oextra *)0;
	otmp->o_id = context.ident++;
	if (!otmp->o_id) otmp->o_id = context.ident++;	/* ident overflowed */
	otmp->timed = 0;	/* not timed, yet */
	otmp->lamplit = 0;	/* ditto */
	otmp->owornmask = 0L;	/* new object isn't worn */
	obj->quan -= num;
	obj->owt = weight(obj);
	otmp->quan = num;
	otmp->owt = weight(otmp);	/* -= obj->owt ? */
	obj->nobj = otmp;
	/* Only set nexthere when on the floor, nexthere is also used */
	/* as a back pointer to the container object when contained. */
	if (obj->where == OBJ_FLOOR)
	    obj->nexthere = otmp;
	copy_oextra(otmp, obj);
	if (has_omid(otmp)) free_omid(otmp);	/* only one association with m_id*/
	if (obj->unpaid) splitbill(obj,otmp);
	if (obj->timed) obj_split_timers(obj, otmp);
	if (obj_sheds_light(obj)) obj_split_light_source(obj, otmp);
	return otmp;
}

/*
 * Insert otmp right after obj in whatever chain(s) it is on.  Then extract
 * obj from the chain(s).  This function does a literal swap.  It is up to
 * the caller to provide a valid context for the swap.  When done, obj will
 * still exist, but not on any chain.
 *
 * Note:  Don't use use obj_extract_self() -- we are doing an in-place swap,
 * not actually moving something.
 */
void
replace_object(obj, otmp)
struct obj *obj;
struct obj *otmp;
{
    otmp->where = obj->where;
    switch (obj->where) {
    case OBJ_FREE:
	/* do nothing */
	break;
    case OBJ_INVENT:
	otmp->nobj = obj->nobj;
	obj->nobj = otmp;
	extract_nobj(obj, &invent);
	break;
    case OBJ_CONTAINED:
	otmp->nobj = obj->nobj;
	otmp->ocontainer = obj->ocontainer;
	obj->nobj = otmp;
	extract_nobj(obj, &obj->ocontainer->cobj);
	break;
    case OBJ_MINVENT:
	otmp->nobj = obj->nobj;
	otmp->ocarry =  obj->ocarry;
	obj->nobj = otmp;
	extract_nobj(obj, &obj->ocarry->minvent);
	break;
    case OBJ_FLOOR:
	otmp->nobj = obj->nobj;
	otmp->nexthere = obj->nexthere;
	otmp->ox = obj->ox;
	otmp->oy = obj->oy;
	obj->nobj = otmp;
	obj->nexthere = otmp;
	extract_nobj(obj, &fobj);
	extract_nexthere(obj, &level.objects[obj->ox][obj->oy]);
	break;
    default:
	panic("replace_object: obj position");
	break;
    }
}

/*
 * Create a dummy duplicate to put on shop bill.  The duplicate exists
 * only in the billobjs chain.  This function is used when a shop object
 * is being altered, and a copy of the original is needed for billing
 * purposes.  For example, when eating, where an interruption will yield
 * an object which is different from what it started out as; the "I x"
 * command needs to display the original object.
 *
 * [BUG:  The cost might end up being different if item originally had a
 * surcharge but doesn't get one now or vice versa.  Having the dummy keep
 * the same o_id value as the original would avoid this; is that viable?
 * (Mustn't give the original itself a new o_id, so can't just swap them.)]
 *
 * The caller is responsible for checking otmp->unpaid and
 * costly_spot(u.ux, u.uy).  This function will make otmp no charge.
 *
 * Note that check_unpaid_usage() should be used instead for partial
 * usage of an object.
 */
void
bill_dummy_object(otmp)
register struct obj *otmp;
{
	register struct obj *dummy;

	if (otmp->unpaid)
	    subfrombill(otmp, shop_keeper(*u.ushops));
	dummy = newobj();
	*dummy = *otmp;
	dummy->oextra = (struct oextra *)0;
	dummy->where = OBJ_FREE;
	dummy->o_id = context.ident++;
	if (!dummy->o_id) dummy->o_id = context.ident++;	/* ident overflowed */
	dummy->timed = 0;
	copy_oextra(dummy, otmp);
	if (has_omid(dummy)) free_omid(dummy);	/* only one association with m_id*/
	if (Is_candle(dummy)) dummy->lamplit = 0;
	addtobill(dummy, FALSE, TRUE, TRUE);
	/* no_charge is only valid for some locations */
	otmp->no_charge = (otmp->where == OBJ_FLOOR ||
			   otmp->where == OBJ_CONTAINED) ? 1 : 0;
	otmp->unpaid = 0;
	return;
}

/* alteration types; must match COST_xxx macros in hack.h */
static const char * const alteration_verbs[] = {
			"cancel", "drain", "uncharge", "unbless", "uncurse",
			"disenchant", "degrade", "dilute", "erase", "burn",
			"neutralize", "destroy", "splatter", "bite", "open",
			"break the lock on",
};

/* possibly bill for an object which the player has just modified */
void
costly_alteration(obj, alter_type)
struct obj *obj;
int alter_type;
{
    xchar ox, oy;
    char objroom;
    boolean set_bknown;
    const char *those, *them;
    struct monst *shkp = 0;

    if (alter_type < 0 || alter_type >= SIZE(alteration_verbs)) {
	impossible("invalid alteration type (%d)", alter_type);
	alter_type = 0;
    }

    ox = oy = 0;	/* lint suppression */
    objroom = '\0';	/* ditto */
    if (carried(obj) || obj->where == OBJ_FREE) {
	/* OBJ_FREE catches obj_no_longer_held()'s transformation
	   of crysknife back into worm tooth; the object has been
	   removed from inventory but not necessarily placed at
	   its new location yet--the unpaid flag will still be set
	   if this item is owned by a shop */
	if (!obj->unpaid) return;
    } else {
	/* this get_obj_location shouldn't fail, but if it does,
	   use hero's location */
	if (!get_obj_location(obj, &ox, &oy, CONTAINED_TOO))
	    ox = u.ux, oy = u.uy;
	if (!costly_spot(ox, oy)) return;
	objroom = *in_rooms(ox, oy, SHOPBASE);
	/* if no shop cares about it, we're done */
	if (!billable(&shkp, obj, objroom, FALSE)) return;
    }

    if (obj->quan == 1L)
	those = "that", them = "it";
    else
	those = "those", them = "them";

    /* when shopkeeper describes the object as being uncursed or unblessed
       hero will know that it is now uncursed; will also make the feedback
       from `I x' after bill_dummy_object() be more specific for this item */
    set_bknown = (alter_type == COST_UNCURS || alter_type == COST_UNBLSS);

    switch (obj->where) {
    case OBJ_FREE:	/* obj_no_longer_held() */
    case OBJ_INVENT:
	if (set_bknown) obj->bknown = 1;
	verbalize("You %s %s %s, you pay for %s!",
		  alteration_verbs[alter_type], those,
		  simpleonames(obj), them);
	bill_dummy_object(obj);
	break;
    case OBJ_FLOOR:
	if (set_bknown) obj->bknown = 1;
	if (costly_spot(u.ux, u.uy) && objroom == *u.ushops) {
	    verbalize("You %s %s, you pay for %s!",
		      alteration_verbs[alter_type], those, them);
	    bill_dummy_object(obj);
	} else {
	    (void) stolen_value(obj, ox, oy, FALSE, FALSE);
	}
	break;
    }
}

static const char dknowns[] = {
		WAND_CLASS, RING_CLASS, POTION_CLASS, SCROLL_CLASS,
		GEM_CLASS, SPBOOK_CLASS, WEAPON_CLASS, TOOL_CLASS, 0
};

struct obj *
mksobj(otyp, init, artif)
int otyp;
boolean init;
boolean artif;
{
	int mndx, tryct;
	struct obj *otmp;
	char let = objects[otyp].oc_class;

	otmp = newobj();
	*otmp = zeroobj;
	otmp->age = monstermoves;
	otmp->o_id = context.ident++;
	if (!otmp->o_id) otmp->o_id = context.ident++;	/* ident overflowed */
	otmp->quan = 1L;
	otmp->oclass = let;
	otmp->otyp = otyp;
	otmp->where = OBJ_FREE;
	otmp->dknown = index(dknowns, let) ? 0 : 1;
	if ((otmp->otyp >= ELVEN_SHIELD && otmp->otyp <= ORCISH_SHIELD) ||
			otmp->otyp == SHIELD_OF_REFLECTION)
		otmp->dknown = 0;
	if (!objects[otmp->otyp].oc_uses_known)
		otmp->known = 1;
	otmp->lknown = 0;
	otmp->cknown = 0;
#ifdef INVISIBLE_OBJECTS
	otmp->oinvis = !rn2(1250);
#endif
	if (init) switch (let) {
	case WEAPON_CLASS:
		otmp->quan = is_multigen(otmp) ? (long) rn1(6,6) : 1L;
		if(!rn2(11)) {
			otmp->spe = rne(3);
			otmp->blessed = rn2(2);
		} else if(!rn2(10)) {
			curse(otmp);
			otmp->spe = -rne(3);
		} else	blessorcurse(otmp, 10);
		if (is_poisonable(otmp) && !rn2(100))
			otmp->opoisoned = 1;

		if (artif && !rn2(20))
		    otmp = mk_artifact(otmp, (aligntyp)A_NONE);
		break;
	case FOOD_CLASS:
	    otmp->oeaten = 0;
	    switch(otmp->otyp) {
	    case CORPSE:
		/* possibly overridden by mkcorpstat() */
		tryct = 50;
		do otmp->corpsenm = undead_to_corpse(rndmonnum());
		while ((mvitals[otmp->corpsenm].mvflags & G_NOCORPSE) && (--tryct > 0));
		if (tryct == 0) {
		/* perhaps rndmonnum() only wants to make G_NOCORPSE monsters on
		   this level; let's create an adventurer's corpse instead, then */
			otmp->corpsenm = PM_HUMAN;
		}
		/* timer set below */
		break;
	    case EGG:
		otmp->corpsenm = NON_PM;	/* generic egg */
		if (!rn2(3)) for (tryct = 200; tryct > 0; --tryct) {
		    mndx = can_be_hatched(rndmonnum());
		    if (mndx != NON_PM && !dead_species(mndx, TRUE)) {
			otmp->corpsenm = mndx;		/* typed egg */
			break;
		    }
		}
		/* timer set below */
		break;
	    case TIN:
		otmp->corpsenm = NON_PM;	/* empty (so far) */
		if (!rn2(6))
		    set_tin_variety(otmp, SPINACH_TIN);
		else for (tryct = 200; tryct > 0; --tryct) {
		    mndx = undead_to_corpse(rndmonnum());
		    if (mons[mndx].cnutrit &&
			    !(mvitals[mndx].mvflags & G_NOCORPSE)) {
			otmp->corpsenm = mndx;
			set_tin_variety(otmp, RANDOM_TIN);
			break;
		    }
		}
		blessorcurse(otmp, 10);
		break;
	    case SLIME_MOLD:
		otmp->spe = current_fruit;
		break;
	    case KELP_FROND:
		otmp->quan = (long) rnd(2);
		break;
	    }
	    if (otmp->otyp == CORPSE || otmp->otyp == MEAT_RING ||
		otmp->otyp == KELP_FROND) break;
	    /* fall into next case */

	case GEM_CLASS:
		if (otmp->otyp == LOADSTONE) curse(otmp);
		else if (otmp->otyp == ROCK) otmp->quan = (long) rn1(6,6);
		else if (otmp->otyp != LUCKSTONE && !rn2(6)) otmp->quan = 2L;
		else otmp->quan = 1L;
		break;
	case TOOL_CLASS:
	    switch(otmp->otyp) {
		case TALLOW_CANDLE:
		case WAX_CANDLE:	otmp->spe = 1;
					otmp->age = 20L * /* 400 or 200 */
					      (long)objects[otmp->otyp].oc_cost;
					otmp->lamplit = 0;
					otmp->quan = 1L +
					      (long)(rn2(2) ? rn2(7) : 0);
					blessorcurse(otmp, 5);
					break;
		case BRASS_LANTERN:
		case OIL_LAMP:		otmp->spe = 1;
					otmp->age = (long) rn1(500,1000);
					otmp->lamplit = 0;
					blessorcurse(otmp, 5);
					break;
		case MAGIC_LAMP:	otmp->spe = 1;
					otmp->lamplit = 0;
					blessorcurse(otmp, 2);
					break;
		case CHEST:
		case LARGE_BOX:		otmp->olocked = !!(rn2(5));
					otmp->otrapped = !(rn2(10));
		case ICE_BOX:
		case SACK:
		case OILSKIN_SACK:
		case BAG_OF_HOLDING:	mkbox_cnts(otmp);
					break;
#ifdef TOURIST
		case EXPENSIVE_CAMERA:
#endif
		case TINNING_KIT:
		case MAGIC_MARKER:	otmp->spe = rn1(70,30);
					break;
		case CAN_OF_GREASE:	otmp->spe = rnd(25);
					blessorcurse(otmp, 10);
					break;
		case CRYSTAL_BALL:	otmp->spe = rnd(5);
					blessorcurse(otmp, 2);
					break;
		case HORN_OF_PLENTY:
		case BAG_OF_TRICKS:	otmp->spe = rnd(20);
					break;
		case FIGURINE:	{	int tryct2 = 0;
					do
					    otmp->corpsenm = rndmonnum();
					while(is_human(&mons[otmp->corpsenm])
						&& tryct2++ < 30);
					blessorcurse(otmp, 4);
					break;
				}
		case BELL_OF_OPENING:   otmp->spe = 3;
					break;
		case MAGIC_FLUTE:
		case MAGIC_HARP:
		case FROST_HORN:
		case FIRE_HORN:
		case DRUM_OF_EARTHQUAKE:
					otmp->spe = rn1(5,4);
					break;
	    }
	    break;
	case AMULET_CLASS:
		if (otmp->otyp == AMULET_OF_YENDOR) context.made_amulet = TRUE;
		if(rn2(10) && (otmp->otyp == AMULET_OF_STRANGULATION ||
		   otmp->otyp == AMULET_OF_CHANGE ||
		   otmp->otyp == AMULET_OF_RESTFUL_SLEEP)) {
			curse(otmp);
		} else	blessorcurse(otmp, 10);
	case VENOM_CLASS:
	case CHAIN_CLASS:
	case BALL_CLASS:
		break;
	case POTION_CLASS:
		if (otmp->otyp == POT_OIL)
		    otmp->age = MAX_OIL_IN_FLASK;	/* amount of oil */
		/* fall through */
	case SCROLL_CLASS:
#ifdef MAIL
		if (otmp->otyp != SCR_MAIL)
#endif
			blessorcurse(otmp, 4);
		break;
	case SPBOOK_CLASS:
		blessorcurse(otmp, 17);
		break;
	case ARMOR_CLASS:
		if(rn2(10) && (otmp->otyp == FUMBLE_BOOTS ||
		   otmp->otyp == LEVITATION_BOOTS ||
		   otmp->otyp == HELM_OF_OPPOSITE_ALIGNMENT ||
		   otmp->otyp == GAUNTLETS_OF_FUMBLING ||
		   !rn2(11))) {
			curse(otmp);
			otmp->spe = -rne(3);
		} else if(!rn2(10)) {
			otmp->blessed = rn2(2);
			otmp->spe = rne(3);
		} else	blessorcurse(otmp, 10);
		if (artif && !rn2(40))                
		    otmp = mk_artifact(otmp, (aligntyp)A_NONE);
		/* simulate lacquered armor for samurai */
		if (Role_if(PM_SAMURAI) && otmp->otyp == SPLINT_MAIL &&
		    (moves <= 1 || In_quest(&u.uz))) {
#ifdef UNIXPC
			/* optimizer bitfield bug */
			otmp->oerodeproof = 1;
			otmp->rknown = 1;
#else
			otmp->oerodeproof = otmp->rknown = 1;
#endif
		}
		break;
	case WAND_CLASS:
		if(otmp->otyp == WAN_WISHING) otmp->spe = rnd(3); else
		otmp->spe = rn1(5,
			(objects[otmp->otyp].oc_dir == NODIR) ? 11 : 4);
		blessorcurse(otmp, 17);
		otmp->recharged = 0; /* used to control recharging */
		break;
	case RING_CLASS:
		if(objects[otmp->otyp].oc_charged) {
		    blessorcurse(otmp, 3);
		    if(rn2(10)) {
			if(rn2(10) && bcsign(otmp))
			    otmp->spe = bcsign(otmp) * rne(3);
			else otmp->spe = rn2(2) ? rne(3) : -rne(3);
		    }
		    /* make useless +0 rings much less common */
		    if (otmp->spe == 0) otmp->spe = rn2(4) - rn2(3);
		    /* negative rings are usually cursed */
		    if (otmp->spe < 0 && rn2(5)) curse(otmp);
		} else if(rn2(10) && (otmp->otyp == RIN_TELEPORTATION ||
			  otmp->otyp == RIN_POLYMORPH ||
			  otmp->otyp == RIN_AGGRAVATE_MONSTER ||
			  otmp->otyp == RIN_HUNGER || !rn2(9))) {
			curse(otmp);
		}
		break;
	case ROCK_CLASS:
		switch (otmp->otyp) {
		    case STATUE:
			/* possibly overridden by mkcorpstat() */
			otmp->corpsenm = rndmonnum();
			if (!verysmall(&mons[otmp->corpsenm]) &&
				rn2(level_difficulty()/2 + 10) > 10)
			    (void) add_to_container(otmp,
						    mkobj(SPBOOK_CLASS,FALSE));
		}
		break;
	case COIN_CLASS:
		break;	/* do nothing */
	default:
		impossible("impossible mkobj %d, sym '%c'.", otmp->otyp,
						objects[otmp->otyp].oc_class);
		return (struct obj *)0;
	}

	/* Some things must get done (timers) even if init = 0 */
	switch (otmp->otyp) {
	    case CORPSE:
	    case EGG:
	    case FIGURINE:
	        set_corpsenm(otmp, otmp->corpsenm);
		break;
	}

	/* unique objects may have an associated artifact entry */
	if (objects[otyp].oc_unique && !otmp->oartifact)
	    otmp = mk_artifact(otmp, (aligntyp)A_NONE);
	otmp->owt = weight(otmp);
	return(otmp);
}

/*
 * Several areas of the code made direct reassignments
 * to obj->corpsenm. Because some special handling is
 * required in certain cases, place that handling here
 * and call this routine in place of the direct assignment.
 * 
 * If the object was a lizard or lichen corpse:
 *     - ensure that you don't end up with some
 *       other corpse type which has no rot-away timer.
 *
 * If the object was a troll corpse:
 *     - ensure that you don't end up with some other
 *       corpse type which resurrects from the dead.
 *
 * Re-calculates the weight of figurines and corpses to suit the
 * new species.
 *
 * Existing timeout value for egg hatch is preserved.
 *
 */
void
set_corpsenm(obj, id)
struct obj *obj;
int id;
{
	long when = 0L;

	if (obj->timed) {
	    if (obj->otyp == EGG)
		when = stop_timer(HATCH_EGG, obj_to_any(obj));
	    else {
	    	when = 0L;
		obj_stop_timers(obj);	/* corpse or figurine */
	    }
	}
	obj->corpsenm = id;
	switch(obj->otyp) {
	    case CORPSE:
		start_corpse_timeout(obj);
		obj->owt = weight(obj);
		break;
	    case FIGURINE:
		if (obj->corpsenm != NON_PM
		    && !dead_species(obj->corpsenm,TRUE)
		    && (carried(obj) || mcarried(obj)))
			attach_fig_transform_timeout(obj);
		obj->owt = weight(obj);
		break;
	    case EGG:
		if (obj->corpsenm != NON_PM
		    && !dead_species(obj->corpsenm,TRUE))
			attach_egg_hatch_timeout(obj, when);
		break;
	    default:	/* tin, etc. */
		obj->owt = weight(obj);
		break;
	}
}

/*
 * Start a corpse decay or revive timer.
 * This takes the age of the corpse into consideration as of 3.4.0.
 */
void
start_corpse_timeout(body)
	struct obj *body;
{
	long when; 		/* rot away when this old */
	long corpse_age;	/* age of corpse          */
	int rot_adjust;
	short action;

#define TAINT_AGE (50L)		/* age when corpses go bad */
#define TROLL_REVIVE_CHANCE 37	/* 1/37 chance for 50 turns ~ 75% chance */
#define ROT_AGE (250L)		/* age when corpses rot away */

	/* lizards and lichen don't rot or revive */
	if (body->corpsenm == PM_LIZARD || body->corpsenm == PM_LICHEN) return;

	action = ROT_CORPSE;		/* default action: rot away */
	rot_adjust = in_mklev ? 25 : 10;	/* give some variation */
	corpse_age = monstermoves - body->age;
	if (corpse_age > ROT_AGE)
		when = rot_adjust;
	else
		when = ROT_AGE - corpse_age;
	when += (long)(rnz(rot_adjust) - rot_adjust);

	if (is_rider(&mons[body->corpsenm])) {
		/*
		 * Riders always revive.  They have a 1/3 chance per turn
		 * of reviving after 12 turns.  Always revive by 500.
		 */
		action = REVIVE_MON;
		for (when = 12L; when < 500L; when++)
		    if (!rn2(3)) break;

	} else if (mons[body->corpsenm].mlet == S_TROLL && !body->norevive) {
		long age;
		for (age = 2; age <= TAINT_AGE; age++)
		    if (!rn2(TROLL_REVIVE_CHANCE)) {	/* troll revives */
			action = REVIVE_MON;
			when = age;
			break;
		    }
	}
	
	if (body->norevive) body->norevive = 0;
	(void) start_timer(when, TIMER_OBJECT, action, obj_to_any(body));
}

void
bless(otmp)
register struct obj *otmp;
{
#ifdef GOLDOBJ
	if (otmp->oclass == COIN_CLASS) return;
#endif
	otmp->cursed = 0;
	otmp->blessed = 1;
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
	else if (otmp->otyp == FIGURINE && otmp->timed)
		(void) stop_timer(FIG_TRANSFORM, obj_to_any(otmp));
	return;
}

void
unbless(otmp)
register struct obj *otmp;
{
	otmp->blessed = 0;
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
}

void
curse(otmp)
register struct obj *otmp;
{
#ifdef GOLDOBJ
	if (otmp->oclass == COIN_CLASS) return;
#endif
	otmp->blessed = 0;
	otmp->cursed = 1;
	/* welded two-handed weapon interferes with some armor removal */
	if (otmp == uwep && bimanual(uwep)) reset_remarm();
	/* rules at top of wield.c state that twoweapon cannot be done
	   with cursed alternate weapon */
	if (otmp == uswapwep && u.twoweap)
	    drop_uswapwep();
	/* some cursed items need immediate updating */
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
	else if (otmp->otyp == FIGURINE) {
		if (otmp->corpsenm != NON_PM
		    && !dead_species(otmp->corpsenm,TRUE)
		    && (carried(otmp) || mcarried(otmp)))
			attach_fig_transform_timeout(otmp);
	}
	return;
}

void
uncurse(otmp)
register struct obj *otmp;
{
	otmp->cursed = 0;
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
	else if (otmp->otyp == FIGURINE && otmp->timed)
	    (void) stop_timer(FIG_TRANSFORM, obj_to_any(otmp));
	return;
}

void
blessorcurse(otmp, chance)
register struct obj *otmp;
register int chance;
{
	if(otmp->blessed || otmp->cursed) return;

	if(!rn2(chance)) {
	    if(!rn2(2)) {
		curse(otmp);
	    } else {
		bless(otmp);
	    }
	}
	return;
}

int
bcsign(otmp)
register struct obj *otmp;
{
	return(!!otmp->blessed - !!otmp->cursed);
}

/*
 *  Calculate the weight of the given object.  This will recursively follow
 *  and calculate the weight of any containers.
 *
 *  Note:  It is possible to end up with an incorrect weight if some part
 *	   of the code messes with a contained object and doesn't update the
 *	   container's weight.
 */
int
weight(obj)
register struct obj *obj;
{
	int wt = objects[obj->otyp].oc_weight;

	if (obj->otyp == LARGE_BOX && obj->spe == 1) /* Schroedinger's Cat */
		wt += mons[PM_HOUSECAT].cwt;
	if (Is_container(obj) || obj->otyp == STATUE) {
		struct obj *contents;
		register int cwt = 0;

		if (obj->otyp == STATUE && obj->corpsenm >= LOW_PM)
		    wt = (int)obj->quan *
			 ((int)mons[obj->corpsenm].cwt * 3 / 2);

		for(contents=obj->cobj; contents; contents=contents->nobj)
			cwt += weight(contents);
		/*
		 *  The weight of bags of holding is calculated as the weight
		 *  of the bag plus the weight of the bag's contents modified
		 *  as follows:
		 *
		 *	Bag status	Weight of contents
		 *	----------	------------------
		 *	cursed			2x
		 *	blessed			x/4 + 1
		 *	otherwise		x/2 + 1
		 *
		 *  The macro DELTA_CWT in pickup.c also implements these
		 *  weight equations.
		 *
		 *  Note:  The above checks are performed in the given order.
		 *	   this means that if an object is both blessed and
		 *	   cursed (not supposed to happen), it will be treated
		 *	   as cursed.
		 */
		if (obj->otyp == BAG_OF_HOLDING)
		    cwt = obj->cursed ? (cwt * 2) :
					(1 + (cwt / (obj->blessed ? 4 : 2)));

		return wt + cwt;
	}
	if (obj->otyp == CORPSE && obj->corpsenm >= LOW_PM) {
		long long_wt = obj->quan * (long) mons[obj->corpsenm].cwt;

		wt = (long_wt > LARGEST_INT) ? LARGEST_INT : (int)long_wt;
		if (obj->oeaten) wt = eaten_stat(wt, obj);
		return wt;
	} else if (obj->oclass == FOOD_CLASS && obj->oeaten) {
		return eaten_stat((int)obj->quan * wt, obj);
	} else if (obj->oclass == COIN_CLASS)
		return (int)((obj->quan + 50L) / 100L);
	else if (obj->otyp == HEAVY_IRON_BALL && obj->owt != 0)
		return((int)(obj->owt));	/* kludge for "very" heavy iron ball */
	return(wt ? wt*(int)obj->quan : ((int)obj->quan + 1)>>1);
}

static int treefruits[] = {APPLE,ORANGE,PEAR,BANANA,EUCALYPTUS_LEAF};

struct obj *
rnd_treefruit_at(x,y)
int x, y;
{
	return mksobj_at(treefruits[rn2(SIZE(treefruits))], x, y, TRUE, FALSE);
}

struct obj *
mkgold(amount, x, y)
long amount;
int x, y;
{
    register struct obj *gold = g_at(x,y);

    if (amount <= 0L)
	amount = (long)(1 + rnd(level_difficulty()+2) * rnd(30));
    if (gold) {
	gold->quan += amount;
    } else {
	gold = mksobj_at(GOLD_PIECE, x, y, TRUE, FALSE);
	gold->quan = amount;
    }
    gold->owt = weight(gold);
    return (gold);
}

/* return TRUE if the corpse has special timing */
#define special_corpse(num)  (((num) == PM_LIZARD)		\
				|| ((num) == PM_LICHEN)		\
				|| (is_rider(&mons[num]))	\
				|| (mons[num].mlet == S_TROLL))

/*
 * OEXTRA note: Passing mtmp causes mtraits to be saved
 * even if ptr passed as well, but ptr is always used for
 * the corpse type (corpsenm). That allows the corpse type
 * to be different from the original monster,
 *	i.e.  vampire -> human corpse
 * yet still allow restoration of the original monster upon
 * resurrection.
 */
struct obj *
mkcorpstat(objtype, mtmp, ptr, x, y, corpstatflags)
int objtype;	/* CORPSE or STATUE */
struct monst *mtmp;
struct permonst *ptr;
int x, y;
unsigned corpstatflags;
{
	register struct obj *otmp;
	boolean init = ((corpstatflags & CORPSTAT_INIT) != 0);

	if (objtype != CORPSE && objtype != STATUE)
	    impossible("making corpstat type %d", objtype);
	if (x == 0 && y == 0) {		/* special case - random placement */
		otmp = mksobj(objtype, init, FALSE);
		if (otmp) (void)rloco(otmp);
	} else
		otmp = mksobj_at(objtype, x, y, init, FALSE);
	if (otmp) {
	    if (mtmp) {
		struct obj *otmp2;

		if (!ptr) ptr = mtmp->data;
		/* save_mtraits frees original data pointed to by otmp */
		otmp2 = save_mtraits(otmp, mtmp);
		if (otmp2) otmp = otmp2;
	    }
	    /* use the corpse or statue produced by mksobj() as-is
	       unless `ptr' is non-null */
	    if (ptr) {
		int old_corpsenm = otmp->corpsenm;

		otmp->corpsenm = monsndx(ptr);
		otmp->owt = weight(otmp);
		if (otmp->otyp == CORPSE &&
			(special_corpse(old_corpsenm) ||
				special_corpse(otmp->corpsenm))) {
		    obj_stop_timers(otmp);
		    start_corpse_timeout(otmp);
		}
	    }
	}
	return(otmp);
}

/*
 * Return the type of monster that this corpse will
 * revive as, even if it has a monster structure
 * attached to it. In that case, you can't just
 * use obj->corpsenm, because the stored monster
 * type can, and often is, different.
 * The return value is an index into mons[].
 */
int
corpse_revive_type(obj)
struct obj *obj;
{
	int revivetype;
	struct monst *mtmp;
	if (has_omonst(obj) &&
		((mtmp = get_mtraits(obj, FALSE)) != (struct monst *)0)) {
	    /* mtmp is a temporary pointer to a monster's stored
		attributes, not a real monster */
	    revivetype = mtmp->mnum;
	} else
	    revivetype = obj->corpsenm;
	return revivetype;
}

/*
 * Attach a monster id to an object, to provide
 * a lasting association between the two.
 */
struct obj *
obj_attach_mid(obj, mid)
struct obj *obj;
unsigned mid;
{
    if (!mid || !obj) return (struct obj *)0;
    newomid(obj);
    *OMID(obj) = mid;
    return obj;
}

static struct obj *
save_mtraits(obj, mtmp)
struct obj *obj;
struct monst *mtmp;
{
	if (mtmp->ispriest) forget_temple_entry(mtmp);	/* EPRI() */
	if (!has_omonst(obj)) newomonst(obj);
	if (has_omonst(obj)) {
		struct monst *mtmp2 = OMONST(obj);
		*mtmp2 = *mtmp;
		mtmp2->mextra = (struct mextra *)0;
		if (mtmp->data) mtmp2->mnum = monsndx(mtmp->data);
		/* invalidate pointers */
		/* m_id is needed to know if this is a revived quest leader */
		/* but m_id must be cleared when loading bones */
		mtmp2->nmon     = (struct monst *)0;
		mtmp2->data     = (struct permonst *)0;
		mtmp2->minvent  = (struct obj *)0;
		if (mtmp->mextra) copy_mextra(mtmp2, mtmp);
#ifndef GOLDOBJ
		/* not a pointer but is discarded along with minvent */
		mtmp2->mgold	= 0L;
#endif
	}
	return obj;
}

/* returns a pointer to a new monst structure based on
 * the one contained within the obj.
 */
struct monst *
get_mtraits(obj, copyof)
struct obj *obj;
boolean copyof;
{
	struct monst *mtmp = (struct monst *)0;
	struct monst *mnew = (struct monst *)0;

	if (has_omonst(obj)) mtmp = OMONST(obj);
	if (mtmp) {
	    if (copyof) {
		mnew = newmonst();
		*mnew = *mtmp;
		mnew->mextra = (struct mextra *)0;
		if (mtmp->mextra) copy_mextra(mnew, mtmp);
	    } else {
	      /* Never insert this returned pointer into mon chains! */
	    	mnew = mtmp;
	    }
	}
	return mnew;
}

/* make an object named after someone listed in the scoreboard file */
struct obj *
mk_tt_object(objtype, x, y)
int objtype; /* CORPSE or STATUE */
register int x, y;
{
	register struct obj *otmp, *otmp2;
	boolean initialize_it;

	/* player statues never contain books */
	initialize_it = (objtype != STATUE);
	if ((otmp = mksobj_at(objtype, x, y, initialize_it, FALSE)) != 0) {
	    /* tt_oname will return null if the scoreboard is empty */
	    if ((otmp2 = tt_oname(otmp)) != 0) otmp = otmp2;
	}
	return(otmp);
}

/* make a new corpse or statue, uninitialized if a statue (i.e. no books) */
struct obj *
mk_named_object(objtype, ptr, x, y, nm)
int objtype;	/* CORPSE or STATUE */
struct permonst *ptr;
int x, y;
const char *nm;
{
	struct obj *otmp;
	unsigned corpstatflags = (objtype != STATUE) ?
				 CORPSTAT_INIT : CORPSTAT_NONE;

	otmp = mkcorpstat(objtype, (struct monst *)0, ptr,
				x, y, corpstatflags);
	if (nm)
		otmp = oname(otmp, nm);
	return(otmp);
}

boolean
is_flammable(otmp)
register struct obj *otmp;
{
	int otyp = otmp->otyp;
	int omat = objects[otyp].oc_material;

	/* Candles can be burned, but they're not flammable in the sense that
	 * they can't get fire damage and it makes no sense for them to be
	 * fireproofed.
	 */
	if (Is_candle(otmp))
		return FALSE;

	if (objects[otyp].oc_oprop == FIRE_RES || otyp == WAN_FIRE)
		return FALSE;

	return((boolean)((omat <= WOOD && omat != LIQUID) || omat == PLASTIC));
}

boolean
is_rottable(otmp)
register struct obj *otmp;
{
	int otyp = otmp->otyp;

	return((boolean)(objects[otyp].oc_material <= WOOD &&
			objects[otyp].oc_material != LIQUID));
}

/*
 * These routines maintain the single-linked lists headed in level.objects[][]
 * and threaded through the nexthere fields in the object-instance structure.
 */

/* put the object at the given location */
void
place_object(otmp, x, y)
register struct obj *otmp;
int x, y;
{
    register struct obj *otmp2 = level.objects[x][y];

    if (otmp->where != OBJ_FREE)
	panic("place_object: obj not free");

    obj_no_longer_held(otmp);
    /* (could bypass this vision update if there is already a boulder here) */
    if (otmp->otyp == BOULDER) block_point(x,y);	/* vision */

    /* obj goes under boulders */
    if (otmp2 && (otmp2->otyp == BOULDER)) {
	otmp->nexthere = otmp2->nexthere;
	otmp2->nexthere = otmp;
    } else {
	otmp->nexthere = otmp2;
	level.objects[x][y] = otmp;
    }

    /* set the new object's location */
    otmp->ox = x;
    otmp->oy = y;

    otmp->where = OBJ_FLOOR;

    /* add to floor chain */
    otmp->nobj = fobj;
    fobj = otmp;
    if (otmp->timed) obj_timer_checks(otmp, x, y, 0);
}

#define ROT_ICE_ADJUSTMENT 2	/* rotting on ice takes 2 times as long */

/* If ice was affecting any objects correct that now
 * Also used for starting ice effects too. [zap.c]
 */
void
obj_ice_effects(x, y, do_buried)
int x, y;
boolean do_buried;
{
	struct obj *otmp;

	for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere) {
		if (otmp->timed) obj_timer_checks(otmp, x, y, 0);
	}
	if (do_buried) {
	    for (otmp = level.buriedobjlist; otmp; otmp = otmp->nobj) {
 		if (otmp->ox == x && otmp->oy == y) {
			if (otmp->timed) obj_timer_checks(otmp, x, y, 0);
		}
	    }
	}
}

/*
 * Returns an obj->age for a corpse object on ice, that would be the
 * actual obj->age if the corpse had just been lifted from the ice.
 * This is useful when just using obj->age in a check or calculation because
 * rot timers pertaining to the object don't have to be stopped and
 * restarted etc.
 */
long
peek_at_iced_corpse_age(otmp)
struct obj *otmp;
{
    long age, retval = otmp->age;
    
    if (otmp->otyp == CORPSE && otmp->on_ice) {
	/* Adjust the age; must be same as obj_timer_checks() for off ice*/
	age = monstermoves - otmp->age;
	retval = otmp->age + (age / ROT_ICE_ADJUSTMENT);
#ifdef DEBUG_EFFECTS
	pline_The("%s age has ice modifications:otmp->age = %ld, returning %ld.",
		s_suffix(doname(otmp)),otmp->age, retval);
	pline("Effective age of corpse: %ld.",
		monstermoves - retval);
#endif
    }
    return retval;
}

STATIC_OVL void
obj_timer_checks(otmp, x, y, force)
struct obj *otmp;
xchar x, y;
int force;	/* 0 = no force so do checks, <0 = force off, >0 force on */
{
    long tleft = 0L;
    short action = ROT_CORPSE;
    boolean restart_timer = FALSE;
    boolean on_floor = (otmp->where == OBJ_FLOOR);
    boolean buried = (otmp->where == OBJ_BURIED);

    /* Check for corpses just placed on or in ice */
    if (otmp->otyp == CORPSE && (on_floor || buried) && is_ice(x,y)) {
	tleft = stop_timer(action, obj_to_any(otmp));
	if (tleft == 0L) {
		action = REVIVE_MON;
		tleft = stop_timer(action, obj_to_any(otmp));
	} 
	if (tleft != 0L) {
	    long age;
	    
	    /* mark the corpse as being on ice */
	    otmp->on_ice = 1;
#ifdef DEBUG_EFFECTS
	    pline("%s is now on ice at %d,%d.", The(xname(otmp)),x,y);
#endif
	    /* Adjust the time remaining */
	    tleft *= ROT_ICE_ADJUSTMENT;
	    restart_timer = TRUE;
	    /* Adjust the age; must be same as in obj_ice_age() */
	    age = monstermoves - otmp->age;
	    otmp->age = monstermoves - (age * ROT_ICE_ADJUSTMENT);
	}
    }
    /* Check for corpses coming off ice */
    else if ((force < 0) ||
	     (otmp->otyp == CORPSE && otmp->on_ice &&
	     ((on_floor && !is_ice(x,y)) || !on_floor))) {
	tleft = stop_timer(action, obj_to_any(otmp));
	if (tleft == 0L) {
		action = REVIVE_MON;
		tleft = stop_timer(action, obj_to_any(otmp));
	}
	if (tleft != 0L) {
		long age;

		otmp->on_ice = 0;
#ifdef DEBUG_EFFECTS
	    	pline("%s is no longer on ice at %d,%d.", The(xname(otmp)),x,y);
#endif
		/* Adjust the remaining time */
		tleft /= ROT_ICE_ADJUSTMENT;
		restart_timer = TRUE;
		/* Adjust the age */
		age = monstermoves - otmp->age;
		otmp->age = otmp->age + (age / ROT_ICE_ADJUSTMENT);
	}
    }
    /* now re-start the timer with the appropriate modifications */ 
    if (restart_timer)
	(void) start_timer(tleft, TIMER_OBJECT, action, obj_to_any(otmp));
}

#undef ROT_ICE_ADJUSTMENT

void
remove_object(otmp)
register struct obj *otmp;
{
    xchar x = otmp->ox;
    xchar y = otmp->oy;

    if (otmp->where != OBJ_FLOOR)
	panic("remove_object: obj not on floor");
    extract_nexthere(otmp, &level.objects[x][y]);
    extract_nobj(otmp, &fobj);
    /* update vision iff this was the only boulder at its spot */
    if (otmp->otyp == BOULDER && !sobj_at(BOULDER, x, y))
	unblock_point(x,y); /* vision */
    if (otmp->timed) obj_timer_checks(otmp,x,y,0);
}

/* throw away all of a monster's inventory */
void
discard_minvent(mtmp)
struct monst *mtmp;
{
    struct obj *otmp;

    while ((otmp = mtmp->minvent) != 0) {
	obj_extract_self(otmp);
	obfree(otmp, (struct obj *)0);	/* dealloc_obj() isn't sufficient */
    }
}

/*
 * Free obj from whatever list it is on in preperation of deleting it or
 * moving it elsewhere.  This will perform all high-level consequences
 * involved with removing the item.  E.g. if the object is in the hero's
 * inventory and confers heat resistance, the hero will lose it.
 *
 * Object positions:
 *	OBJ_FREE	not on any list
 *	OBJ_FLOOR	fobj, level.locations[][] chains (use remove_object)
 *	OBJ_CONTAINED	cobj chain of container object
 *	OBJ_INVENT	hero's invent chain (use freeinv)
 *	OBJ_MINVENT	monster's invent chain
 *	OBJ_MIGRATING	migrating chain
 *	OBJ_BURIED	level.buriedobjs chain
 *	OBJ_ONBILL	on billobjs chain
 */
void
obj_extract_self(obj)
    struct obj *obj;
{
    switch (obj->where) {
	case OBJ_FREE:
	    break;
	case OBJ_FLOOR:
	    remove_object(obj);
	    break;
	case OBJ_CONTAINED:
	    extract_nobj(obj, &obj->ocontainer->cobj);
	    container_weight(obj->ocontainer);
	    break;
	case OBJ_INVENT:
	    freeinv(obj);
	    break;
	case OBJ_MINVENT:
	    extract_nobj(obj, &obj->ocarry->minvent);
	    break;
	case OBJ_MIGRATING:
	    extract_nobj(obj, &migrating_objs);
	    break;
	case OBJ_BURIED:
	    extract_nobj(obj, &level.buriedobjlist);
	    break;
	case OBJ_ONBILL:
	    extract_nobj(obj, &billobjs);
	    break;
	default:
	    panic("obj_extract_self");
	    break;
    }
}


/* Extract the given object from the chain, following nobj chain. */
void
extract_nobj(obj, head_ptr)
    struct obj *obj, **head_ptr;
{
    struct obj *curr, *prev;

    curr = *head_ptr;
    for (prev = (struct obj *) 0; curr; prev = curr, curr = curr->nobj) {
	if (curr == obj) {
	    if (prev)
		prev->nobj = curr->nobj;
	    else
		*head_ptr = curr->nobj;
	    break;
	}
    }
    if (!curr) panic("extract_nobj: object lost");
    obj->where = OBJ_FREE;
}


/*
 * Extract the given object from the chain, following nexthere chain.
 *
 * This does not set obj->where, this function is expected to be called
 * in tandem with extract_nobj, which does set it.
 */
void
extract_nexthere(obj, head_ptr)
    struct obj *obj, **head_ptr;
{
    struct obj *curr, *prev;

    curr = *head_ptr;
    for (prev = (struct obj *) 0; curr; prev = curr, curr = curr->nexthere) {
	if (curr == obj) {
	    if (prev)
		prev->nexthere = curr->nexthere;
	    else
		*head_ptr = curr->nexthere;
	    break;
	}
    }
    if (!curr) panic("extract_nexthere: object lost");
}


/*
 * Add obj to mon's inventory.  If obj is able to merge with something already
 * in the inventory, then the passed obj is deleted and 1 is returned.
 * Otherwise 0 is returned.
 */
int
add_to_minv(mon, obj)
    struct monst *mon;
    struct obj *obj;
{
    struct obj *otmp;

    if (obj->where != OBJ_FREE)
	panic("add_to_minv: obj not free");

    /* merge if possible */
    for (otmp = mon->minvent; otmp; otmp = otmp->nobj)
	if (merged(&otmp, &obj))
	    return 1;	/* obj merged and then free'd */
    /* else insert; don't bother forcing it to end of chain */
    obj->where = OBJ_MINVENT;
    obj->ocarry = mon;
    obj->nobj = mon->minvent;
    mon->minvent = obj;
    return 0;	/* obj on mon's inventory chain */
}

/*
 * Add obj to container, make sure obj is "free".  Returns (merged) obj.
 * The input obj may be deleted in the process.
 */
struct obj *
add_to_container(container, obj)
    struct obj *container, *obj;
{
    struct obj *otmp;

    if (obj->where != OBJ_FREE)
	panic("add_to_container: obj not free");
    if (container->where != OBJ_INVENT && container->where != OBJ_MINVENT)
	obj_no_longer_held(obj);

    /* merge if possible */
    for (otmp = container->cobj; otmp; otmp = otmp->nobj)
	if (merged(&otmp, &obj)) return (otmp);

    obj->where = OBJ_CONTAINED;
    obj->ocontainer = container;
    obj->nobj = container->cobj;
    container->cobj = obj;
    return (obj);
}

void
add_to_migration(obj)
    struct obj *obj;
{
    if (obj->where != OBJ_FREE)
	panic("add_to_migration: obj not free");

    obj->where = OBJ_MIGRATING;
    obj->nobj = migrating_objs;
    migrating_objs = obj;
}

void
add_to_buried(obj)
    struct obj *obj;
{
    if (obj->where != OBJ_FREE)
	panic("add_to_buried: obj not free");

    obj->where = OBJ_BURIED;
    obj->nobj = level.buriedobjlist;
    level.buriedobjlist = obj;
}

/* Recalculate the weight of this container and all of _its_ containers. */
STATIC_OVL void
container_weight(container)
    struct obj *container;
{
    container->owt = weight(container);
    if (container->where == OBJ_CONTAINED)
	container_weight(container->ocontainer);
/*
    else if (container->where == OBJ_INVENT)
	recalculate load delay here ???
*/
}

/*
 * Deallocate the object.  _All_ objects should be run through here for
 * them to be deallocated.
 */
void
dealloc_obj(obj)
    struct obj *obj;
{
    if (obj->where != OBJ_FREE)
	panic("dealloc_obj: obj not free");

    /* free up any timers attached to the object */
    if (obj->timed)
	obj_stop_timers(obj);

    /*
     * Free up any light sources attached to the object.
     *
     * We may want to just call del_light_source() without any
     * checks (requires a code change there).  Otherwise this
     * list must track all objects that can have a light source
     * attached to it (and also requires lamplit to be set).
     */
    if (obj_sheds_light(obj))
	del_light_source(LS_OBJECT, obj_to_any(obj));

    if (obj == thrownobj) thrownobj = (struct obj*)0;

    if (obj->oextra) dealloc_oextra(obj->oextra);
    free((genericptr_t) obj);
}

/* create an object from a horn of plenty; mirrors bagotricks(makemon.c) */
int
hornoplenty(horn, tipping)
struct obj *horn;
boolean tipping;  /* caller emptying entire contents; affects shop handling */
{
    int objcount = 0;

    if (!horn || horn->otyp != HORN_OF_PLENTY) {
	impossible("bad horn o' plenty");
    } else if (horn->spe < 1) {
	pline(nothing_happens);
    } else {
	struct obj *obj;
	const char *what;

	consume_obj_charge(horn, !tipping);
	if (!rn2(13)) {
	    obj = mkobj(POTION_CLASS, FALSE);
	    if (objects[obj->otyp].oc_magic) do {
		obj->otyp = rnd_class(POT_BOOZE, POT_WATER);
	    } while (obj->otyp == POT_SICKNESS);
	    what = (obj->quan > 1L) ? "Some potions" : "A potion";
	} else {
	    obj = mkobj(FOOD_CLASS, FALSE);
	    if (obj->otyp == FOOD_RATION && !rn2(7))
		obj->otyp = LUMP_OF_ROYAL_JELLY;
	    what = "Some food";
	}
	++objcount;
	pline("%s %s out.", what, vtense(what, "spill"));
	obj->blessed = horn->blessed;
	obj->cursed = horn->cursed;
	obj->owt = weight(obj);
	/* using a shop's horn of plenty entails a usage fee and also
	   confers ownership of the created item to the shopkeeper */
	if (horn->unpaid)
	    addtobill(obj, FALSE, FALSE, tipping);
	/* if it ended up on bill, we don't want "(unpaid, N zorkids)"
	   being included in its formatted name during next message */
	iflags.suppress_price++;
	if (!tipping) {
	    obj = hold_another_object(obj, u.uswallow ?
				      "Oops!  %s out of your reach!" :
					(Is_airlevel(&u.uz) ||
					 Is_waterlevel(&u.uz) ||
					 levl[u.ux][u.uy].typ < IRONBARS ||
					 levl[u.ux][u.uy].typ >= ICE) ?
					  "Oops!  %s away from you!" :
					    "Oops!  %s to the floor!",
				      The(aobjnam(obj, "slip")),
				      (const char *)0);
	} else {
	    /* assumes this is taking place at hero's location */
	    if (!can_reach_floor(TRUE)) {
		hitfloor(obj);		/* does altar check, message, drop */
	    } else {
		if (IS_ALTAR(levl[u.ux][u.uy].typ))
		    doaltarobj(obj);	/* does its own drop message */
		else
		    pline("%s %s to the %s.", Doname2(obj),
			  otense(obj, "drop"), surface(u.ux, u.uy));
		dropy(obj);
	    }
	}
	iflags.suppress_price--;
	if (horn->dknown) makeknown(HORN_OF_PLENTY);
    }
    return objcount;
}

#ifdef WIZARD
/* Check all object lists for consistency. */
void
obj_sanity_check()
{
    int x, y;
    struct obj *obj;
    struct monst *mon;
    const char *mesg;

    mesg = "fobj sanity";
    for (obj = fobj; obj; obj = obj->nobj) {
	if (obj->where != OBJ_FLOOR) {
	    pline("%s obj %s %s@(%d,%d): %s\n", mesg,
		  fmt_ptr((genericptr_t)obj),
		  where_name(obj->where),
		  obj->ox, obj->oy, doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "location sanity";
    for (x = 0; x < COLNO; x++)
	for (y = 0; y < ROWNO; y++)
	    for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
		if (obj->where != OBJ_FLOOR) {
		    pline("%s obj %s %s@(%d,%d): %s\n", mesg,
			  fmt_ptr((genericptr_t)obj),
			  where_name(obj->where),
			  obj->ox, obj->oy, doname(obj));
		}

    mesg = "invent sanity";
    for (obj = invent; obj; obj = obj->nobj) {
	if (obj->where != OBJ_INVENT) {
	    pline("%s obj %s %s: %s\n", mesg,
		  fmt_ptr((genericptr_t)obj),
		  where_name(obj->where), doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "migrating sanity";
    for (obj = migrating_objs; obj; obj = obj->nobj) {
	if (obj->where != OBJ_MIGRATING) {
	    pline("%s obj %s %s: %s\n", mesg,
		  fmt_ptr((genericptr_t)obj),
		  where_name(obj->where), doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "buried sanity";
    for (obj = level.buriedobjlist; obj; obj = obj->nobj) {
	if (obj->where != OBJ_BURIED) {
	    pline("%s obj %s %s: %s\n", mesg,
		  fmt_ptr((genericptr_t)obj),
		  where_name(obj->where), doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "bill sanity";
    for (obj = billobjs; obj; obj = obj->nobj) {
	if (obj->where != OBJ_ONBILL) {
	    pline("%s obj %s %s: %s\n", mesg,
		  fmt_ptr((genericptr_t)obj),
		  where_name(obj->where), doname(obj));
	}
	/* shouldn't be a full container on the bill */
	if (obj->cobj) {
	    pline("%s obj %s contains %s! %s\n", mesg,
		  fmt_ptr((genericptr_t)obj),
		  something, doname(obj));
	}
    }

    mesg = "minvent sanity";
    for (mon = fmon; mon; mon = mon->nmon)
	for (obj = mon->minvent; obj; obj = obj->nobj) {
	    if (obj->where != OBJ_MINVENT) {
		pline("%s obj %s %s: %s\n", mesg,
		      fmt_ptr((genericptr_t)obj),
		      where_name(obj->where), doname(obj));
	    }
	    if (obj->ocarry != mon) {
		pline("%s obj %s (%s) not held by mon %s (%s)\n", mesg,
		      fmt_ptr((genericptr_t)obj), doname(obj),
		      fmt_ptr((genericptr_t)mon), mon_nam(mon));
	    }
	    check_contained(obj, mesg);
	}
}

/* This must stay consistent with the defines in obj.h. */
static const char *obj_state_names[NOBJ_STATES] = {
	"free",		"floor",	"contained",	"invent",
	"minvent",	"migrating",	"buried",	"onbill"
};

STATIC_OVL const char *
where_name(where)
    int where;
{
    return (where<0 || where>=NOBJ_STATES) ? "unknown" : obj_state_names[where];
}

/* obj sanity check: check objs contained by container */
STATIC_OVL void
check_contained(container, mesg)
    struct obj *container;
    const char *mesg;
{
    struct obj *obj;

    for (obj = container->cobj; obj; obj = obj->nobj) {
	if (obj->where != OBJ_CONTAINED)
	    pline("contained %s obj %s: %s\n", mesg,
		  fmt_ptr((genericptr_t)obj),
		  where_name(obj->where));
	else if (obj->ocontainer != container)
	    pline("%s obj %s not in container %s\n", mesg,
		  fmt_ptr((genericptr_t)obj),
		  fmt_ptr((genericptr_t)container));
    }
}
#endif /* WIZARD */

/*mkobj.c*/

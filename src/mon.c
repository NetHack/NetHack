/*	SCCS Id: @(#)mon.c	3.4	2003/08/24	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* If you're using precompiled headers, you don't want this either */
#ifdef MICROPORT_BUG
#define MKROOM_H
#endif

#include "hack.h"
#include "mfndpos.h"
#include "edog.h"
#include <ctype.h>

STATIC_DCL boolean FDECL(restrap,(struct monst *));
STATIC_DCL long FDECL(mm_aggression, (struct monst *,struct monst *));
#ifdef OVL2
STATIC_DCL int NDECL(pick_animal);
STATIC_DCL int FDECL(select_newcham_form, (struct monst *));
STATIC_DCL void FDECL(kill_eggs, (struct obj *));
#endif

#ifdef REINCARNATION
#define LEVEL_SPECIFIC_NOCORPSE(mdat) \
	 (Is_rogue_level(&u.uz) || \
	   (level.flags.graveyard && is_undead(mdat) && rn2(3)))
#else
#define LEVEL_SPECIFIC_NOCORPSE(mdat) \
	   (level.flags.graveyard && is_undead(mdat) && rn2(3))
#endif


#if 0
/* part of the original warning code which was replaced in 3.3.1 */
#ifdef OVL1
#define warnDelay 10
long lastwarntime;
int lastwarnlev;

const char *warnings[] = {
	"white", "pink", "red", "ruby", "purple", "black"
};

STATIC_DCL void NDECL(warn_effects);
#endif /* OVL1 */
#endif /* 0 */

#ifndef OVLB
STATIC_VAR short cham_to_pm[];
#else
STATIC_DCL struct obj *FDECL(make_corpse,(struct monst *));
STATIC_DCL void FDECL(m_detach, (struct monst *, struct permonst *));
STATIC_DCL void FDECL(lifesaved_monster, (struct monst *));

/* convert the monster index of an undead to its living counterpart */
int
undead_to_corpse(mndx)
int mndx;
{
	switch (mndx) {
	case PM_KOBOLD_ZOMBIE:
	case PM_KOBOLD_MUMMY:	mndx = PM_KOBOLD;  break;
	case PM_DWARF_ZOMBIE:
	case PM_DWARF_MUMMY:	mndx = PM_DWARF;  break;
	case PM_GNOME_ZOMBIE:
	case PM_GNOME_MUMMY:	mndx = PM_GNOME;  break;
	case PM_ORC_ZOMBIE:
	case PM_ORC_MUMMY:	mndx = PM_ORC;  break;
	case PM_ELF_ZOMBIE:
	case PM_ELF_MUMMY:	mndx = PM_ELF;  break;
	case PM_VAMPIRE:
	case PM_VAMPIRE_LORD:
#if 0	/* DEFERRED */
	case PM_VAMPIRE_MAGE:
#endif
	case PM_HUMAN_ZOMBIE:
	case PM_HUMAN_MUMMY:	mndx = PM_HUMAN;  break;
	case PM_GIANT_ZOMBIE:
	case PM_GIANT_MUMMY:	mndx = PM_GIANT;  break;
	case PM_ETTIN_ZOMBIE:
	case PM_ETTIN_MUMMY:	mndx = PM_ETTIN;  break;
	default:  break;
	}
	return mndx;
}

/* Convert the monster index of some monsters (such as quest guardians)
 * to their generic species type.
 *
 * Return associated character class monster, rather than species
 * if mode is 1.
 */
int
genus(mndx, mode)
int mndx, mode;
{
	switch (mndx) {
/* Quest guardians */
	case PM_STUDENT:     mndx = mode ? PM_VALKYRIE  : PM_HUMAN; break;
	case PM_CHIEFTAIN:   mndx = mode ? PM_BARBARIAN : PM_HUMAN; break;
	case PM_NEANDERTHAL: mndx = mode ? PM_CAVEMAN   : PM_HUMAN; break;
	case PM_ATTENDANT:   mndx = mode ? PM_HEALER    : PM_HUMAN; break;
	case PM_PAGE:        mndx = mode ? PM_KNIGHT    : PM_HUMAN; break;
	case PM_ABBOT:       mndx = mode ? PM_MONK      : PM_HUMAN; break;
	case PM_ACOLYTE:     mndx = mode ? PM_PRIEST    : PM_HUMAN; break;
	case PM_HUNTER:      mndx = mode ? PM_RANGER    : PM_HUMAN; break;
	case PM_THUG:        mndx = mode ? PM_ROGUE     : PM_HUMAN; break;
	case PM_ROSHI:       mndx = mode ? PM_SAMURAI   : PM_HUMAN; break;
#ifdef TOURIST
	case PM_GUIDE:       mndx = mode ? PM_TOURIST   : PM_HUMAN; break;
#endif
	case PM_APPRENTICE:  mndx = mode ? PM_WIZARD    : PM_HUMAN; break;
	case PM_WARRIOR:     mndx = mode ? PM_VALKYRIE  : PM_HUMAN; break;
	default:
		if (mndx >= LOW_PM && mndx < NUMMONS) {
			struct permonst *ptr = &mons[mndx];
			if (is_human(ptr))      mndx = PM_HUMAN;
			else if (is_elf(ptr))   mndx = PM_ELF;
			else if (is_dwarf(ptr)) mndx = PM_DWARF;
			else if (is_gnome(ptr)) mndx = PM_GNOME;
			else if (is_orc(ptr))   mndx = PM_ORC;
		}
		break;
	}
	return mndx;
}

/* convert monster index to chameleon index */
int
pm_to_cham(mndx)
int mndx;
{
	int mcham;

	switch (mndx) {
	case PM_CHAMELEON:	mcham = CHAM_CHAMELEON; break;
	case PM_DOPPELGANGER:	mcham = CHAM_DOPPELGANGER; break;
	case PM_SANDESTIN:	mcham = CHAM_SANDESTIN; break;
	default: mcham = CHAM_ORDINARY; break;
	}
	return mcham;
}

/* convert chameleon index to monster index */
STATIC_VAR short cham_to_pm[] = {
		NON_PM,		/* placeholder for CHAM_ORDINARY */
		PM_CHAMELEON,
		PM_DOPPELGANGER,
		PM_SANDESTIN,
};

#define KEEPTRAITS(mon)	(mon->isshk || mon->mtame || \
			 (mon->data->geno & G_UNIQ) || is_reviver(mon->data) || \
			 (mon->m_id == quest_status.leader_m_id))

/* Creates a monster corpse, a "special" corpse, or nothing if it doesn't
 * leave corpses.  Monsters which leave "special" corpses should have
 * G_NOCORPSE set in order to prevent wishing for one, finding tins of one,
 * etc....
 */
STATIC_OVL struct obj *
make_corpse(mtmp)
register struct monst *mtmp;
{
	register struct permonst *mdat = mtmp->data;
	int num;
	struct obj *obj = (struct obj *)0;
	int x = mtmp->mx, y = mtmp->my;
	int mndx = monsndx(mdat);

	switch(mndx) {
	    case PM_GRAY_DRAGON:
	    case PM_SILVER_DRAGON:
#if 0	/* DEFERRED */
	    case PM_SHIMMERING_DRAGON:
#endif
	    case PM_RED_DRAGON:
	    case PM_ORANGE_DRAGON:
	    case PM_WHITE_DRAGON:
	    case PM_BLACK_DRAGON:
	    case PM_BLUE_DRAGON:
	    case PM_GREEN_DRAGON:
	    case PM_YELLOW_DRAGON:
		/* Make dragon scales.  This assumes that the order of the */
		/* dragons is the same as the order of the scales.	   */
		if (!rn2(mtmp->mrevived ? 20 : 3)) {
		    num = GRAY_DRAGON_SCALES + monsndx(mdat) - PM_GRAY_DRAGON;
		    obj = mksobj_at(num, x, y, FALSE, FALSE);
		    obj->spe = 0;
		    obj->cursed = obj->blessed = FALSE;
		}
		goto default_1;

	    case PM_WHITE_UNICORN:
	    case PM_GRAY_UNICORN:
	    case PM_BLACK_UNICORN:
		if (mtmp->mrevived && rn2(20)) {
			if (canseemon(mtmp))
			   pline("%s recently regrown horn crumbles to dust.",
				s_suffix(Monnam(mtmp)));
		} else
			(void) mksobj_at(UNICORN_HORN, x, y, TRUE, FALSE);
		goto default_1;
	    case PM_LONG_WORM:
		(void) mksobj_at(WORM_TOOTH, x, y, TRUE, FALSE);
		goto default_1;
	    case PM_VAMPIRE:
	    case PM_VAMPIRE_LORD:
		/* include mtmp in the mkcorpstat() call */
		num = undead_to_corpse(mndx);
		obj = mkcorpstat(CORPSE, mtmp, &mons[num], x, y, TRUE);
		obj->age -= 100;		/* this is an *OLD* corpse */
		break;
	    case PM_KOBOLD_MUMMY:
	    case PM_DWARF_MUMMY:
	    case PM_GNOME_MUMMY:
	    case PM_ORC_MUMMY:
	    case PM_ELF_MUMMY:
	    case PM_HUMAN_MUMMY:
	    case PM_GIANT_MUMMY:
	    case PM_ETTIN_MUMMY:
	    case PM_KOBOLD_ZOMBIE:
	    case PM_DWARF_ZOMBIE:
	    case PM_GNOME_ZOMBIE:
	    case PM_ORC_ZOMBIE:
	    case PM_ELF_ZOMBIE:
	    case PM_HUMAN_ZOMBIE:
	    case PM_GIANT_ZOMBIE:
	    case PM_ETTIN_ZOMBIE:
		num = undead_to_corpse(mndx);
		obj = mkcorpstat(CORPSE, mtmp, &mons[num], x, y, TRUE);
		obj->age -= 100;		/* this is an *OLD* corpse */
		break;
	    case PM_IRON_GOLEM:
		num = d(2,6);
		while (num--)
			obj = mksobj_at(IRON_CHAIN, x, y, TRUE, FALSE);
		mtmp->mnamelth = 0;
		break;
	    case PM_GLASS_GOLEM:
		num = d(2,4);   /* very low chance of creating all glass gems */
		while (num--)
			obj = mksobj_at((LAST_GEM + rnd(9)), x, y, TRUE, FALSE);
		mtmp->mnamelth = 0;
		break;
	    case PM_CLAY_GOLEM:
		obj = mksobj_at(ROCK, x, y, FALSE, FALSE);
		obj->quan = (long)(rn2(20) + 50);
		obj->owt = weight(obj);
		mtmp->mnamelth = 0;
		break;
	    case PM_STONE_GOLEM:
		obj = mkcorpstat(STATUE, (struct monst *)0,
			mdat, x, y, FALSE);
		break;
	    case PM_WOOD_GOLEM:
		num = d(2,4);
		while(num--) {
			obj = mksobj_at(QUARTERSTAFF, x, y, TRUE, FALSE);
		}
		mtmp->mnamelth = 0;
		break;
	    case PM_LEATHER_GOLEM:
		num = d(2,4);
		while(num--)
			obj = mksobj_at(LEATHER_ARMOR, x, y, TRUE, FALSE);
		mtmp->mnamelth = 0;
		break;
	    case PM_GOLD_GOLEM:
		/* Good luck gives more coins */
		obj = mkgold((long)(200 - rnl(101)), x, y);
		mtmp->mnamelth = 0;
		break;
	    case PM_PAPER_GOLEM:
		num = rnd(4);
		while (num--)
			obj = mksobj_at(SCR_BLANK_PAPER, x, y, TRUE, FALSE);
		mtmp->mnamelth = 0;
		break;
	    default_1:
	    default:
		if (mvitals[mndx].mvflags & G_NOCORPSE)
		    return (struct obj *)0;
		else	/* preserve the unique traits of some creatures */
		    obj = mkcorpstat(CORPSE, KEEPTRAITS(mtmp) ? mtmp : 0,
				     mdat, x, y, TRUE);
		break;
	}
	/* All special cases should precede the G_NOCORPSE check */

	/* if polymorph or undead turning has killed this monster,
	   prevent the same attack beam from hitting its corpse */
	if (flags.bypasses) bypass_obj(obj);

	if (mtmp->mnamelth)
	    obj = oname(obj, NAME(mtmp));

	/* Avoid "It was hidden under a green mold corpse!" 
	 *  during Blind combat. An unseen monster referred to as "it"
	 *  could be killed and leave a corpse.  If a hider then hid
	 *  underneath it, you could be told the corpse type of a
	 *  monster that you never knew was there without this.
	 *  The code in hitmu() substitutes the word "something"
	 *  if the corpses obj->dknown is 0.
	 */
	if (Blind && !sensemon(mtmp)) obj->dknown = 0;

#ifdef INVISIBLE_OBJECTS
	/* Invisible monster ==> invisible corpse */
	obj->oinvis = mtmp->minvis;
#endif

	stackobj(obj);
	newsym(x, y);
	return obj;
}

#endif /* OVLB */
#ifdef OVL1

#if 0
/* part of the original warning code which was replaced in 3.3.1 */
STATIC_OVL void
warn_effects()
{
    if (warnlevel == 100) {
	if(!Blind && uwep &&
	    (warnlevel > lastwarnlev || moves > lastwarntime + warnDelay)) {
	    Your("%s %s!", aobjnam(uwep, "glow"),
		hcolor(NH_LIGHT_BLUE));
	    lastwarnlev = warnlevel;
	    lastwarntime = moves;
	}
	warnlevel = 0;
	return;
    }

    if (warnlevel >= SIZE(warnings))
	warnlevel = SIZE(warnings)-1;
    if (!Blind &&
	    (warnlevel > lastwarnlev || moves > lastwarntime + warnDelay)) {
	const char *which, *what, *how;
	long rings = (EWarning & (LEFT_RING|RIGHT_RING));

	if (rings) {
	    what = Hallucination ? "mood ring" : "ring";
	    how = "glows";	/* singular verb */
	    if (rings == LEFT_RING) {
		which = "left ";
	    } else if (rings == RIGHT_RING) {
		which = "right ";
	    } else {		/* both */
		which = "";
		what = (const char *) makeplural(what);
		how = "glow";	/* plural verb */
	    }
	    Your("%s%s %s %s!", which, what, how, hcolor(warnings[warnlevel]));
	} else {
	    if (Hallucination)
		Your("spider-sense is tingling...");
	    else
		You_feel("apprehensive as you sense a %s flash.",
		    warnings[warnlevel]);
	}

	lastwarntime = moves;
	lastwarnlev = warnlevel;
    }
}
#endif /* 0 */

/* check mtmp and water/lava for compatibility, 0 (survived), 1 (died) */
int
minliquid(mtmp)
register struct monst *mtmp;
{
    boolean inpool, inlava, infountain;

    inpool = is_pool(mtmp->mx,mtmp->my) &&
	     !is_flyer(mtmp->data) && !is_floater(mtmp->data);
    inlava = is_lava(mtmp->mx,mtmp->my) &&
	     !is_flyer(mtmp->data) && !is_floater(mtmp->data);
    infountain = IS_FOUNTAIN(levl[mtmp->mx][mtmp->my].typ);

#ifdef STEED
	/* Flying and levitation keeps our steed out of the liquid */
	/* (but not water-walking or swimming) */
	if (mtmp == u.usteed && (Flying || Levitation))
		return (0);
#endif

    /* Gremlin multiplying won't go on forever since the hit points
     * keep going down, and when it gets to 1 hit point the clone
     * function will fail.
     */
    if (mtmp->data == &mons[PM_GREMLIN] && (inpool || infountain) && rn2(3)) {
	if (split_mon(mtmp, (struct monst *)0))
	    dryup(mtmp->mx, mtmp->my, FALSE);
	if (inpool) water_damage(mtmp->minvent, FALSE, FALSE);
	return (0);
    } else if (mtmp->data == &mons[PM_IRON_GOLEM] && inpool && !rn2(5)) {
	int dam = d(2,6);
	if (cansee(mtmp->mx,mtmp->my))
	    pline("%s rusts.", Monnam(mtmp));
	mtmp->mhp -= dam;
	if (mtmp->mhpmax > dam) mtmp->mhpmax -= dam;
	if (mtmp->mhp < 1) {
	    mondead(mtmp);
	    if (mtmp->mhp < 1) return (1);
	}
	water_damage(mtmp->minvent, FALSE, FALSE);
	return (0);
    }

    if (inlava) {
	/*
	 * Lava effects much as water effects. Lava likers are able to
	 * protect their stuff. Fire resistant monsters can only protect
	 * themselves  --ALI
	 */
	if (!is_clinger(mtmp->data) && !likes_lava(mtmp->data)) {
	    if (!resists_fire(mtmp)) {
		if (cansee(mtmp->mx,mtmp->my))
		    pline("%s %s.", Monnam(mtmp),
			  mtmp->data == &mons[PM_WATER_ELEMENTAL] ?
			  "boils away" : "burns to a crisp");
		mondead(mtmp);
	    }
	    else {
		if (--mtmp->mhp < 1) {
		    if (cansee(mtmp->mx,mtmp->my))
			pline("%s surrenders to the fire.", Monnam(mtmp));
		    mondead(mtmp);
		}
		else if (cansee(mtmp->mx,mtmp->my))
		    pline("%s burns slightly.", Monnam(mtmp));
	    }
	    if (mtmp->mhp > 0) {
		(void) fire_damage(mtmp->minvent, FALSE, FALSE,
						mtmp->mx, mtmp->my);
		rloc(mtmp);
		return 0;
	    }
	    return (1);
	}
    } else if (inpool) {
	/* Most monsters drown in pools.  flooreffects() will take care of
	 * water damage to dead monsters' inventory, but survivors need to
	 * be handled here.  Swimmers are able to protect their stuff...
	 */
	if (!is_clinger(mtmp->data)
	    && !is_swimmer(mtmp->data) && !amphibious(mtmp->data)) {
	    if (cansee(mtmp->mx,mtmp->my)) {
		    pline("%s drowns.", Monnam(mtmp));
	    }
	    if (u.ustuck && u.uswallow && u.ustuck == mtmp) {
	    /* This can happen after a purple worm plucks you off a
		flying steed while you are over water. */
		pline("%s sinks as water rushes in and flushes you out.",
			Monnam(mtmp));
	    }
	    mondead(mtmp);
	    if (mtmp->mhp > 0) {
		rloc(mtmp);
		water_damage(mtmp->minvent, FALSE, FALSE);
		return 0;
	    }
	    return (1);
	}
    } else {
	/* but eels have a difficult time outside */
	if (mtmp->data->mlet == S_EEL && !Is_waterlevel(&u.uz)) {
	    if(mtmp->mhp > 1) mtmp->mhp--;
	    monflee(mtmp, 2, FALSE, FALSE);
	}
    }
    return (0);
}


int
mcalcmove(mon)
struct monst *mon;
{
    int mmove = mon->data->mmove;

    /* Note: MSLOW's `+ 1' prevents slowed speed 1 getting reduced to 0;
     *	     MFAST's `+ 2' prevents hasted speed 1 from becoming a no-op;
     *	     both adjustments have negligible effect on higher speeds.
     */
    if (mon->mspeed == MSLOW)
	mmove = (2 * mmove + 1) / 3;
    else if (mon->mspeed == MFAST)
	mmove = (4 * mmove + 2) / 3;

#ifdef STEED
    if (mon == u.usteed) {
	if (u.ugallop && flags.mv) {
	    /* average movement is 1.50 times normal */
	    mmove = ((rn2(2) ? 4 : 5) * mmove) / 3;
	}
    }
#endif

    return mmove;
}

/* actions that happen once per ``turn'', regardless of each
   individual monster's metabolism; some of these might need to
   be reclassified to occur more in proportion with movement rate */
void
mcalcdistress()
{
    struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	if (DEADMONSTER(mtmp)) continue;

	/* must check non-moving monsters once/turn in case
	 * they managed to end up in liquid */
	if (mtmp->data->mmove == 0) {
	    if (vision_full_recalc) vision_recalc(0);
	    if (minliquid(mtmp)) continue;
	}

	/* regenerate hit points */
	mon_regen(mtmp, FALSE);

	/* possibly polymorph shapechangers and lycanthropes */
	if (mtmp->cham && !rn2(6))
	    (void) newcham(mtmp, (struct permonst *)0, FALSE, FALSE);
	were_change(mtmp);

	/* gradually time out temporary problems */
	if (mtmp->mblinded && !--mtmp->mblinded)
	    mtmp->mcansee = 1;
	if (mtmp->mfrozen && !--mtmp->mfrozen)
	    mtmp->mcanmove = 1;
	if (mtmp->mfleetim && !--mtmp->mfleetim)
	    mtmp->mflee = 0;

	/* FIXME: mtmp->mlstmv ought to be updated here */
    }
}

int
movemon()
{
    register struct monst *mtmp, *nmtmp;
    register boolean somebody_can_move = FALSE;
#if 0
    /* part of the original warning code which was replaced in 3.3.1 */
    warnlevel = 0;
#endif

    /*
    Some of you may remember the former assertion here that
    because of deaths and other actions, a simple one-pass
    algorithm wasn't possible for movemon.  Deaths are no longer
    removed to the separate list fdmon; they are simply left in
    the chain with hit points <= 0, to be cleaned up at the end
    of the pass.

    The only other actions which cause monsters to be removed from
    the chain are level migrations and losedogs().  I believe losedogs()
    is a cleanup routine not associated with monster movements, and
    monsters can only affect level migrations on themselves, not others
    (hence the fetching of nmon before moving the monster).  Currently,
    monsters can jump into traps, read cursed scrolls of teleportation,
    and drink cursed potions of raise level to change levels.  These are
    all reflexive at this point.  Should one monster be able to level
    teleport another, this scheme would have problems.
    */

    for(mtmp = fmon; mtmp; mtmp = nmtmp) {
	nmtmp = mtmp->nmon;

	/* Find a monster that we have not treated yet.	 */
	if(DEADMONSTER(mtmp))
	    continue;
	if(mtmp->movement < NORMAL_SPEED)
	    continue;

	mtmp->movement -= NORMAL_SPEED;
	if (mtmp->movement >= NORMAL_SPEED)
	    somebody_can_move = TRUE;

	if (vision_full_recalc) vision_recalc(0);	/* vision! */

	if (minliquid(mtmp)) continue;

	if (is_hider(mtmp->data)) {
	    /* unwatched mimics and piercers may hide again  [MRS] */
	    if(restrap(mtmp))   continue;
	    if(mtmp->m_ap_type == M_AP_FURNITURE ||
				mtmp->m_ap_type == M_AP_OBJECT)
		    continue;
	    if(mtmp->mundetected) continue;
	}

	/* continue if the monster died fighting */
	if (Conflict && !mtmp->iswiz && mtmp->mcansee) {
	    /* Note:
	     *  Conflict does not take effect in the first round.
	     *  Therefore, A monster when stepping into the area will
	     *  get to swing at you.
	     *
	     *  The call to fightm() must be _last_.  The monster might
	     *  have died if it returns 1.
	     */
	    if (couldsee(mtmp->mx,mtmp->my) &&
		(distu(mtmp->mx,mtmp->my) <= BOLT_LIM*BOLT_LIM) &&
							fightm(mtmp))
		continue;	/* mon might have died */
	}
	if(dochugw(mtmp))		/* otherwise just move the monster */
	    continue;
    }
#if 0
    /* part of the original warning code which was replaced in 3.3.1 */
    if(warnlevel > 0)
	warn_effects();
#endif

    if (any_light_source())
	vision_full_recalc = 1;	/* in case a mon moved with a light source */
    dmonsfree();	/* remove all dead monsters */

    /* a monster may have levteleported player -dlc */
    if (u.utotype) {
	deferred_goto();
	/* changed levels, so these monsters are dormant */
	somebody_can_move = FALSE;
    }

    return somebody_can_move;
}

#endif /* OVL1 */
#ifdef OVLB

#define mstoning(obj)	(ofood(obj) && \
					(touch_petrifies(&mons[(obj)->corpsenm]) || \
					(obj)->corpsenm == PM_MEDUSA))

/*
 * Maybe eat a metallic object (not just gold).
 * Return value: 0 => nothing happened, 1 => monster ate something,
 * 2 => monster died (it must have grown into a genocided form, but
 * that can't happen at present because nothing which eats objects
 * has young and old forms).
 */
int
meatmetal(mtmp)
	register struct monst *mtmp;
{
	register struct obj *otmp;
	struct permonst *ptr;
	int poly, grow, heal, mstone;

	/* If a pet, eating is handled separately, in dog.c */
	if (mtmp->mtame) return 0;

	/* Eats topmost metal object if it is there */
	for (otmp = level.objects[mtmp->mx][mtmp->my];
						otmp; otmp = otmp->nexthere) {
	    if (mtmp->data == &mons[PM_RUST_MONSTER] && !is_rustprone(otmp))
		continue;
	    if (is_metallic(otmp) && !obj_resists(otmp, 5, 95) &&
		touch_artifact(otmp,mtmp)) {
		if (mtmp->data == &mons[PM_RUST_MONSTER] && otmp->oerodeproof) {
		    if (canseemon(mtmp) && flags.verbose) {
			pline("%s eats %s!",
				Monnam(mtmp),
				distant_name(otmp,doname));
		    }
		    /* The object's rustproofing is gone now */
		    otmp->oerodeproof = 0;
		    mtmp->mstun = 1;
		    if (canseemon(mtmp) && flags.verbose) {
			pline("%s spits %s out in disgust!",
			      Monnam(mtmp), distant_name(otmp,doname));
		    }
		/* KMH -- Don't eat indigestible/choking objects */
		} else if (otmp->otyp != AMULET_OF_STRANGULATION &&
				otmp->otyp != RIN_SLOW_DIGESTION) {
		    if (cansee(mtmp->mx,mtmp->my) && flags.verbose)
			pline("%s eats %s!", Monnam(mtmp),
				distant_name(otmp,doname));
		    else if (flags.soundok && flags.verbose)
			You_hear("a crunching sound.");
		    mtmp->meating = otmp->owt/2 + 1;
		    /* Heal up to the object's weight in hp */
		    if (mtmp->mhp < mtmp->mhpmax) {
			mtmp->mhp += objects[otmp->otyp].oc_weight;
			if (mtmp->mhp > mtmp->mhpmax) mtmp->mhp = mtmp->mhpmax;
		    }
		    if(otmp == uball) {
			unpunish();
			delobj(otmp);
		    } else if (otmp == uchain) {
			unpunish();	/* frees uchain */
		    } else {
			poly = polyfodder(otmp);
			grow = mlevelgain(otmp);
			heal = mhealup(otmp);
			mstone = mstoning(otmp);
			delobj(otmp);
			ptr = mtmp->data;
			if (poly) {
			    if (newcham(mtmp, (struct permonst *)0,
					FALSE, FALSE))
				ptr = mtmp->data;
			} else if (grow) {
			    ptr = grow_up(mtmp, (struct monst *)0);
			} else if (mstone) {
			    if (poly_when_stoned(ptr)) {
				mon_to_stone(mtmp);
				ptr = mtmp->data;
			    } else if (!resists_ston(mtmp)) {
				if (canseemon(mtmp))
				    pline("%s turns to stone!", Monnam(mtmp));
				monstone(mtmp);
				ptr = (struct permonst *)0;
			    }
			} else if (heal) {
			    mtmp->mhp = mtmp->mhpmax;
			}
			if (!ptr) return 2;		 /* it died */
		    }
		    /* Left behind a pile? */
		    if (rnd(25) < 3)
			(void)mksobj_at(ROCK, mtmp->mx, mtmp->my, TRUE, FALSE);
		    newsym(mtmp->mx, mtmp->my);
		    return 1;
		}
	    }
	}
	return 0;
}

int
meatobj(mtmp)		/* for gelatinous cubes */
	register struct monst *mtmp;
{
	register struct obj *otmp, *otmp2;
	struct permonst *ptr;
	int poly, grow, heal, count = 0, ecount = 0;
	char buf[BUFSZ];

	buf[0] = '\0';
	/* If a pet, eating is handled separately, in dog.c */
	if (mtmp->mtame) return 0;

	/* Eats organic objects, including cloth and wood, if there */
	/* Engulfs others, except huge rocks and metal attached to player */
	for (otmp = level.objects[mtmp->mx][mtmp->my]; otmp; otmp = otmp2) {
	    otmp2 = otmp->nexthere;
	    if (is_organic(otmp) && !obj_resists(otmp, 5, 95) &&
		    touch_artifact(otmp,mtmp)) {
		if (otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) &&
			!resists_ston(mtmp))
		    continue;
		if (otmp->otyp == AMULET_OF_STRANGULATION ||
				otmp->otyp == RIN_SLOW_DIGESTION)
		    continue;
		++count;
		if (cansee(mtmp->mx,mtmp->my) && flags.verbose)
		    pline("%s eats %s!", Monnam(mtmp),
			    distant_name(otmp, doname));
		else if (flags.soundok && flags.verbose)
		    You_hear("a slurping sound.");
		/* Heal up to the object's weight in hp */
		if (mtmp->mhp < mtmp->mhpmax) {
		    mtmp->mhp += objects[otmp->otyp].oc_weight;
		    if (mtmp->mhp > mtmp->mhpmax) mtmp->mhp = mtmp->mhpmax;
		}
		if (Has_contents(otmp)) {
		    register struct obj *otmp3;
		    /* contents of eaten containers become engulfed; this
		       is arbitrary, but otherwise g.cubes are too powerful */
		    while ((otmp3 = otmp->cobj) != 0) {
			obj_extract_self(otmp3);
			if (otmp->otyp == ICE_BOX && otmp3->otyp == CORPSE) {
			    otmp3->age = monstermoves - otmp3->age;
			    start_corpse_timeout(otmp3);
			}
			(void) mpickobj(mtmp, otmp3);
		    }
		}
		poly = polyfodder(otmp);
		grow = mlevelgain(otmp);
		heal = mhealup(otmp);
		delobj(otmp);		/* munch */
		ptr = mtmp->data;
		if (poly) {
		    if (newcham(mtmp, (struct permonst *)0, FALSE, FALSE))
			ptr = mtmp->data;
		} else if (grow) {
		    ptr = grow_up(mtmp, (struct monst *)0);
		} else if (heal) {
		    mtmp->mhp = mtmp->mhpmax;
		}
		/* in case it polymorphed or died */
		if (ptr != &mons[PM_GELATINOUS_CUBE])
		    return !ptr ? 2 : 1;
	    } else if (otmp->oclass != ROCK_CLASS &&
				    otmp != uball && otmp != uchain) {
		++ecount;
		if (ecount == 1) {
			Sprintf(buf, "%s engulfs %s.", Monnam(mtmp),
			    distant_name(otmp,doname));
		} else if (ecount == 2)
			Sprintf(buf, "%s engulfs several objects.", Monnam(mtmp));
		obj_extract_self(otmp);
		(void) mpickobj(mtmp, otmp);	/* slurp */
	    }
	    /* Engulf & devour is instant, so don't set meating */
	    if (mtmp->minvis) newsym(mtmp->mx, mtmp->my);
	}
	if (ecount > 0) {
	    if (cansee(mtmp->mx, mtmp->my) && flags.verbose && buf[0])
		pline("%s", buf);
	    else if (flags.soundok && flags.verbose)
	    	You_hear("%s slurping sound%s.",
			ecount == 1 ? "a" : "several",
			ecount == 1 ? "" : "s");
	}
	return ((count > 0) || (ecount > 0)) ? 1 : 0;
}

void
mpickgold(mtmp)
	register struct monst *mtmp;
{
    register struct obj *gold;
    int mat_idx;

    if ((gold = g_at(mtmp->mx, mtmp->my)) != 0) {
	mat_idx = objects[gold->otyp].oc_material;
#ifndef GOLDOBJ
	mtmp->mgold += gold->quan;
	delobj(gold);
#else
        obj_extract_self(gold);
        add_to_minv(mtmp, gold);
#endif
	if (cansee(mtmp->mx, mtmp->my) ) {
	    if (flags.verbose && !mtmp->isgd)
		pline("%s picks up some %s.", Monnam(mtmp),
			mat_idx == GOLD ? "gold" : "money");
	    newsym(mtmp->mx, mtmp->my);
	}
    }
}
#endif /* OVLB */
#ifdef OVL2

boolean
mpickstuff(mtmp, str)
	register struct monst *mtmp;
	register const char *str;
{
	register struct obj *otmp, *otmp2;

/*	prevent shopkeepers from leaving the door of their shop */
	if(mtmp->isshk && inhishop(mtmp)) return FALSE;

	for(otmp = level.objects[mtmp->mx][mtmp->my]; otmp; otmp = otmp2) {
	    otmp2 = otmp->nexthere;
/*	Nymphs take everything.  Most monsters don't pick up corpses. */
	    if (!str ? searches_for_item(mtmp,otmp) :
		  !!(index(str, otmp->oclass))) {
		if (otmp->otyp == CORPSE && mtmp->data->mlet != S_NYMPH &&
			/* let a handful of corpse types thru to can_carry() */
			!touch_petrifies(&mons[otmp->corpsenm]) &&
			otmp->corpsenm != PM_LIZARD &&
			!acidic(&mons[otmp->corpsenm])) continue;
		if (!touch_artifact(otmp,mtmp)) continue;
		if (!can_carry(mtmp,otmp)) continue;
		if (is_pool(mtmp->mx,mtmp->my)) continue;
#ifdef INVISIBLE_OBJECTS
		if (otmp->oinvis && !perceives(mtmp->data)) continue;
#endif
		if (cansee(mtmp->mx,mtmp->my) && flags.verbose)
			pline("%s picks up %s.", Monnam(mtmp),
			      (distu(mtmp->my, mtmp->my) <= 5) ?
				doname(otmp) : distant_name(otmp, doname));
		obj_extract_self(otmp);
		/* unblock point after extract, before pickup */
		if (otmp->otyp == BOULDER)
		    unblock_point(otmp->ox,otmp->oy);	/* vision */
		(void) mpickobj(mtmp, otmp);	/* may merge and free otmp */
		m_dowear(mtmp, FALSE);
		newsym(mtmp->mx, mtmp->my);
		return TRUE;			/* pick only one object */
	    }
	}
	return FALSE;
}

#endif /* OVL2 */
#ifdef OVL0

int
curr_mon_load(mtmp)
register struct monst *mtmp;
{
	register int curload = 0;
	register struct obj *obj;

	for(obj = mtmp->minvent; obj; obj = obj->nobj) {
		if(obj->otyp != BOULDER || !throws_rocks(mtmp->data))
			curload += obj->owt;
	}

	return curload;
}

int
max_mon_load(mtmp)
register struct monst *mtmp;
{
	register long maxload;

	/* Base monster carrying capacity is equal to human maximum
	 * carrying capacity, or half human maximum if not strong.
	 * (for a polymorphed player, the value used would be the
	 * non-polymorphed carrying capacity instead of max/half max).
	 * This is then modified by the ratio between the monster weights
	 * and human weights.  Corpseless monsters are given a capacity
	 * proportional to their size instead of weight.
	 */
	if (!mtmp->data->cwt)
		maxload = (MAX_CARR_CAP * (long)mtmp->data->msize) / MZ_HUMAN;
	else if (!strongmonst(mtmp->data)
		|| (strongmonst(mtmp->data) && (mtmp->data->cwt > WT_HUMAN)))
		maxload = (MAX_CARR_CAP * (long)mtmp->data->cwt) / WT_HUMAN;
	else	maxload = MAX_CARR_CAP; /*strong monsters w/cwt <= WT_HUMAN*/

	if (!strongmonst(mtmp->data)) maxload /= 2;

	if (maxload < 1) maxload = 1;

	return (int) maxload;
}

/* for restricting monsters' object-pickup */
boolean
can_carry(mtmp,otmp)
struct monst *mtmp;
struct obj *otmp;
{
	int otyp = otmp->otyp, newload = otmp->owt;
	struct permonst *mdat = mtmp->data;

	if (notake(mdat)) return FALSE;		/* can't carry anything */

	if (otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) &&
		!(mtmp->misc_worn_check & W_ARMG) && !resists_ston(mtmp))
	    return FALSE;
	if (otyp == CORPSE && is_rider(&mons[otmp->corpsenm]))
	    return FALSE;
	if (objects[otyp].oc_material == SILVER && hates_silver(mdat) &&
		(otyp != BELL_OF_OPENING || !is_covetous(mdat)))
	    return FALSE;

#ifdef STEED
	/* Steeds don't pick up stuff (to avoid shop abuse) */
	if (mtmp == u.usteed) return (FALSE);
#endif
	if (mtmp->isshk) return(TRUE); /* no limit */
	if (mtmp->mpeaceful && !mtmp->mtame) return(FALSE);
	/* otherwise players might find themselves obligated to violate
	 * their alignment if the monster takes something they need
	 */

	/* special--boulder throwers carry unlimited amounts of boulders */
	if (throws_rocks(mdat) && otyp == BOULDER)
		return(TRUE);

	/* nymphs deal in stolen merchandise, but not boulders or statues */
	if (mdat->mlet == S_NYMPH)
		return (boolean)(otmp->oclass != ROCK_CLASS);

	if (curr_mon_load(mtmp) + newload > max_mon_load(mtmp)) return FALSE;

	return(TRUE);
}

/* return number of acceptable neighbour positions */
int
mfndpos(mon, poss, info, flag)
	register struct monst *mon;
	coord *poss;	/* coord poss[9] */
	long *info;	/* long info[9] */
	long flag;
{
	struct permonst *mdat = mon->data;
	register xchar x,y,nx,ny;
	register int cnt = 0;
	register uchar ntyp;
	uchar nowtyp;
	boolean wantpool,poolok,lavaok,nodiag;
	boolean rockok = FALSE, treeok = FALSE, thrudoor;
	int maxx, maxy;

	x = mon->mx;
	y = mon->my;
	nowtyp = levl[x][y].typ;

	nodiag = (mdat == &mons[PM_GRID_BUG]);
	wantpool = mdat->mlet == S_EEL;
	poolok = is_flyer(mdat) || is_clinger(mdat) ||
		 (is_swimmer(mdat) && !wantpool);
	lavaok = is_flyer(mdat) || is_clinger(mdat) || likes_lava(mdat);
	thrudoor = ((flag & (ALLOW_WALL|BUSTDOOR)) != 0L);
	if (flag & ALLOW_DIG) {
	    struct obj *mw_tmp;

	    /* need to be specific about what can currently be dug */
	    if (!needspick(mdat)) {
		rockok = treeok = TRUE;
	    } else if ((mw_tmp = MON_WEP(mon)) && mw_tmp->cursed &&
		       mon->weapon_check == NO_WEAPON_WANTED) {
		rockok = is_pick(mw_tmp);
		treeok = is_axe(mw_tmp);
	    } else {
		rockok = (m_carrying(mon, PICK_AXE) ||
			  (m_carrying(mon, DWARVISH_MATTOCK) &&
			   !which_armor(mon, W_ARMS)));
		treeok = (m_carrying(mon, AXE) ||
			  (m_carrying(mon, BATTLE_AXE) &&
			   !which_armor(mon, W_ARMS)));
	    }
	    thrudoor |= rockok || treeok;
	}

nexttry:	/* eels prefer the water, but if there is no water nearby,
		   they will crawl over land */
	if(mon->mconf) {
		flag |= ALLOW_ALL;
		flag &= ~NOTONL;
	}
	if(!mon->mcansee)
		flag |= ALLOW_SSM;
	maxx = min(x+1,COLNO-1);
	maxy = min(y+1,ROWNO-1);
	for(nx = max(1,x-1); nx <= maxx; nx++)
	  for(ny = max(0,y-1); ny <= maxy; ny++) {
	    if(nx == x && ny == y) continue;
	    if(IS_ROCK(ntyp = levl[nx][ny].typ) &&
	       !((flag & ALLOW_WALL) && may_passwall(nx,ny)) &&
	       !((IS_TREE(ntyp) ? treeok : rockok) && may_dig(nx,ny))) continue;
	    /* KMH -- Added iron bars */
	    if (ntyp == IRONBARS && !(flag & ALLOW_BARS)) continue;
	    if(IS_DOOR(ntyp) && !amorphous(mdat) &&
	       ((levl[nx][ny].doormask & D_CLOSED && !(flag & OPENDOOR)) ||
		(levl[nx][ny].doormask & D_LOCKED && !(flag & UNLOCKDOOR))) &&
	       !thrudoor) continue;
	    if(nx != x && ny != y && (nodiag ||
#ifdef REINCARNATION
	       ((IS_DOOR(nowtyp) &&
		 ((levl[x][y].doormask & ~D_BROKEN) || Is_rogue_level(&u.uz))) ||
		(IS_DOOR(ntyp) &&
		 ((levl[nx][ny].doormask & ~D_BROKEN) || Is_rogue_level(&u.uz))))
#else
	       ((IS_DOOR(nowtyp) && (levl[x][y].doormask & ~D_BROKEN)) ||
		(IS_DOOR(ntyp) && (levl[nx][ny].doormask & ~D_BROKEN)))
#endif
	       ))
		continue;
	    if((is_pool(nx,ny) == wantpool || poolok) &&
	       (lavaok || !is_lava(nx,ny))) {
		int dispx, dispy;
		boolean monseeu = (mon->mcansee && (!Invis || perceives(mdat)));
		boolean checkobj = OBJ_AT(nx,ny);

		/* Displacement also displaces the Elbereth/scare monster,
		 * as long as you are visible.
		 */
		if(Displaced && monseeu && (mon->mux==nx) && (mon->muy==ny)) {
		    dispx = u.ux;
		    dispy = u.uy;
		} else {
		    dispx = nx;
		    dispy = ny;
		}

		info[cnt] = 0;
		if ((checkobj || Displaced) && onscary(dispx, dispy, mon)) {
		    if(!(flag & ALLOW_SSM)) continue;
		    info[cnt] |= ALLOW_SSM;
		}
		if((nx == u.ux && ny == u.uy) ||
		   (nx == mon->mux && ny == mon->muy)) {
			if (nx == u.ux && ny == u.uy) {
				/* If it's right next to you, it found you,
				 * displaced or no.  We must set mux and muy
				 * right now, so when we return we can tell
				 * that the ALLOW_U means to attack _you_ and
				 * not the image.
				 */
				mon->mux = u.ux;
				mon->muy = u.uy;
			}
			if(!(flag & ALLOW_U)) continue;
			info[cnt] |= ALLOW_U;
		} else {
			if(MON_AT(nx, ny)) {
				struct monst *mtmp2 = m_at(nx, ny);
				long mmflag = flag | mm_aggression(mon, mtmp2);

				if (!(mmflag & ALLOW_M)) continue;
				info[cnt] |= ALLOW_M;
				if (mtmp2->mtame) {
					if (!(mmflag & ALLOW_TM)) continue;
					info[cnt] |= ALLOW_TM;
				}
			}
			/* Note: ALLOW_SANCT only prevents movement, not */
			/* attack, into a temple. */
			if(level.flags.has_temple &&
			   *in_rooms(nx, ny, TEMPLE) &&
			   !*in_rooms(x, y, TEMPLE) &&
			   in_your_sanctuary((struct monst *)0, nx, ny)) {
				if(!(flag & ALLOW_SANCT)) continue;
				info[cnt] |= ALLOW_SANCT;
			}
		}
		if(checkobj && sobj_at(CLOVE_OF_GARLIC, nx, ny)) {
			if(flag & NOGARLIC) continue;
			info[cnt] |= NOGARLIC;
		}
		if(checkobj && sobj_at(BOULDER, nx, ny)) {
			if(!(flag & ALLOW_ROCK)) continue;
			info[cnt] |= ALLOW_ROCK;
		}
		if (monseeu && onlineu(nx,ny)) {
			if(flag & NOTONL) continue;
			info[cnt] |= NOTONL;
		}
		if (nx != x && ny != y && bad_rock(mdat, x, ny)
			    && bad_rock(mdat, nx, y)
			    && (bigmonst(mdat) || (curr_mon_load(mon) > 600)))
			continue;
		/* The monster avoids a particular type of trap if it's familiar
		 * with the trap type.  Pets get ALLOW_TRAPS and checking is
		 * done in dogmove.c.  In either case, "harmless" traps are
		 * neither avoided nor marked in info[].
		 */
		{ register struct trap *ttmp = t_at(nx, ny);
		    if(ttmp) {
			if(ttmp->ttyp >= TRAPNUM || ttmp->ttyp == 0)  {
impossible("A monster looked at a very strange trap of type %d.", ttmp->ttyp);
			    continue;
			}
			if ((ttmp->ttyp != RUST_TRAP
					|| mdat == &mons[PM_IRON_GOLEM])
				&& ttmp->ttyp != STATUE_TRAP
				&& ((ttmp->ttyp != PIT
				    && ttmp->ttyp != SPIKED_PIT
				    && ttmp->ttyp != TRAPDOOR
				    && ttmp->ttyp != HOLE)
				      || (!is_flyer(mdat)
				    && !is_floater(mdat)
				    && !is_clinger(mdat))
				      || In_sokoban(&u.uz))
				&& (ttmp->ttyp != SLP_GAS_TRAP ||
				    !resists_sleep(mon))
				&& (ttmp->ttyp != BEAR_TRAP ||
				    (mdat->msize > MZ_SMALL &&
				     !amorphous(mdat) && !is_flyer(mdat)))
				&& (ttmp->ttyp != FIRE_TRAP ||
				    !resists_fire(mon))
				&& (ttmp->ttyp != SQKY_BOARD || !is_flyer(mdat))
				&& (ttmp->ttyp != WEB || (!amorphous(mdat) &&
				    !webmaker(mdat)))
			) {
			    if (!(flag & ALLOW_TRAPS)) {
				if (mon->mtrapseen & (1L << (ttmp->ttyp - 1)))
				    continue;
			    }
			    info[cnt] |= ALLOW_TRAPS;
			}
		    }
		}
		poss[cnt].x = nx;
		poss[cnt].y = ny;
		cnt++;
	    }
	}
	if(!cnt && wantpool && !is_pool(x,y)) {
		wantpool = FALSE;
		goto nexttry;
	}
	return(cnt);
}

#endif /* OVL0 */
#ifdef OVL1

/* Monster against monster special attacks; for the specified monster
   combinations, this allows one monster to attack another adjacent one
   in the absence of Conflict.  There is no provision for targetting
   other monsters; just hand to hand fighting when they happen to be
   next to each other. */
STATIC_OVL long
mm_aggression(magr, mdef)
struct monst *magr,	/* monster that is currently deciding where to move */
	     *mdef;	/* another monster which is next to it */
{
	/* supposedly purple worms are attracted to shrieking because they
	   like to eat shriekers, so attack the latter when feasible */
	if (magr->data == &mons[PM_PURPLE_WORM] &&
		mdef->data == &mons[PM_SHRIEKER])
	    return ALLOW_M|ALLOW_TM;
	/* Various other combinations such as dog vs cat, cat vs rat, and
	   elf vs orc have been suggested.  For the time being we don't
	   support those. */
	return 0L;
}

boolean
monnear(mon, x, y)
register struct monst *mon;
register int x,y;
/* Is the square close enough for the monster to move or attack into? */
{
	register int distance = dist2(mon->mx, mon->my, x, y);
	if (distance==2 && mon->data==&mons[PM_GRID_BUG]) return 0;
	return((boolean)(distance < 3));
}

/* really free dead monsters */
void
dmonsfree()
{
    struct monst **mtmp;
    int count = 0;

    for (mtmp = &fmon; *mtmp;) {
	if ((*mtmp)->mhp <= 0) {
	    struct monst *freetmp = *mtmp;
	    *mtmp = (*mtmp)->nmon;
	    dealloc_monst(freetmp);
	    count++;
	} else
	    mtmp = &(*mtmp)->nmon;
    }

    if (count != iflags.purge_monsters)
	impossible("dmonsfree: %d removed doesn't match %d pending",
		   count, iflags.purge_monsters);
    iflags.purge_monsters = 0;
}

#endif /* OVL1 */
#ifdef OVLB

/* called when monster is moved to larger structure */
void
replmon(mtmp, mtmp2)
register struct monst *mtmp, *mtmp2;
{
    struct obj *otmp;

    /* transfer the monster's inventory */
    for (otmp = mtmp2->minvent; otmp; otmp = otmp->nobj) {
#ifdef DEBUG
	if (otmp->where != OBJ_MINVENT || otmp->ocarry != mtmp)
	    panic("replmon: minvent inconsistency");
#endif
	otmp->ocarry = mtmp2;
    }
    mtmp->minvent = 0;

    /* remove the old monster from the map and from `fmon' list */
    relmon(mtmp);

    /* finish adding its replacement */
#ifdef STEED
    if (mtmp == u.usteed) ; else	/* don't place steed onto the map */
#endif
    place_monster(mtmp2, mtmp2->mx, mtmp2->my);
    if (mtmp2->wormno)	    /* update level.monsters[wseg->wx][wseg->wy] */
	place_wsegs(mtmp2); /* locations to mtmp2 not mtmp. */
    if (emits_light(mtmp2->data)) {
	/* since this is so rare, we don't have any `mon_move_light_source' */
	new_light_source(mtmp2->mx, mtmp2->my,
			 emits_light(mtmp2->data),
			 LS_MONSTER, (genericptr_t)mtmp2);
	/* here we rely on the fact that `mtmp' hasn't actually been deleted */
	del_light_source(LS_MONSTER, (genericptr_t)mtmp);
    }
    mtmp2->nmon = fmon;
    fmon = mtmp2;
    if (u.ustuck == mtmp) u.ustuck = mtmp2;
#ifdef STEED
    if (u.usteed == mtmp) u.usteed = mtmp2;
#endif
    if (mtmp2->isshk) replshk(mtmp,mtmp2);

    /* discard the old monster */
    dealloc_monst(mtmp);
}

/* release mon from display and monster list */
void
relmon(mon)
register struct monst *mon;
{
	register struct monst *mtmp;

	if (fmon == (struct monst *)0)  panic ("relmon: no fmon available.");

	remove_monster(mon->mx, mon->my);

	if(mon == fmon) fmon = fmon->nmon;
	else {
		for(mtmp = fmon; mtmp && mtmp->nmon != mon; mtmp = mtmp->nmon) ;
		if(mtmp)    mtmp->nmon = mon->nmon;
		else	    panic("relmon: mon not in list.");
	}
}

/* remove effects of mtmp from other data structures */
STATIC_OVL void
m_detach(mtmp, mptr)
struct monst *mtmp;
struct permonst *mptr;	/* reflects mtmp->data _prior_ to mtmp's death */
{
	if (mtmp->mleashed) m_unleash(mtmp, FALSE);
	    /* to prevent an infinite relobj-flooreffects-hmon-killed loop */
	mtmp->mtrapped = 0;
	mtmp->mhp = 0; /* simplify some tests: force mhp to 0 */
	relobj(mtmp, 0, FALSE);
	remove_monster(mtmp->mx, mtmp->my);
	if (emits_light(mptr))
	    del_light_source(LS_MONSTER, (genericptr_t)mtmp);
	newsym(mtmp->mx,mtmp->my);
	unstuck(mtmp);
	fill_pit(mtmp->mx, mtmp->my);

	if(mtmp->isshk) shkgone(mtmp);
	if(mtmp->wormno) wormgone(mtmp);
	iflags.purge_monsters++;
}

/* find the worn amulet of life saving which will save a monster */
struct obj *
mlifesaver(mon)
struct monst *mon;
{
	if (!nonliving(mon->data)) {
	    struct obj *otmp = which_armor(mon, W_AMUL);

	    if (otmp && otmp->otyp == AMULET_OF_LIFE_SAVING)
		return otmp;
	}
	return (struct obj *)0;
}

STATIC_OVL void
lifesaved_monster(mtmp)
struct monst *mtmp;
{
	struct obj *lifesave = mlifesaver(mtmp);

	if (lifesave) {
		/* not canseemon; amulets are on the head, so you don't want */
		/* to show this for a long worm with only a tail visible. */
		/* Nor do you check invisibility, because glowing and disinte- */
		/* grating amulets are always visible. */
		if (cansee(mtmp->mx, mtmp->my)) {
			pline("But wait...");
			pline("%s medallion begins to glow!",
				s_suffix(Monnam(mtmp)));
			makeknown(AMULET_OF_LIFE_SAVING);
			if (attacktype(mtmp->data, AT_EXPL)
			    || attacktype(mtmp->data, AT_BOOM))
				pline("%s reconstitutes!", Monnam(mtmp));
			else
				pline("%s looks much better!", Monnam(mtmp));
			pline_The("medallion crumbles to dust!");
		}
		m_useup(mtmp, lifesave);
		mtmp->mcanmove = 1;
		mtmp->mfrozen = 0;
		if (mtmp->mtame && !mtmp->isminion) {
			wary_dog(mtmp, FALSE);
		}
		if (mtmp->mhpmax <= 0) mtmp->mhpmax = 10;
		mtmp->mhp = mtmp->mhpmax;
		if (mvitals[monsndx(mtmp->data)].mvflags & G_GENOD) {
			if (cansee(mtmp->mx, mtmp->my))
			    pline("Unfortunately %s is still genocided...",
				mon_nam(mtmp));
		} else
			return;
	}
	mtmp->mhp = 0;
}

void
mondead(mtmp)
register struct monst *mtmp;
{
	struct permonst *mptr;
	int tmp;

	if(mtmp->isgd) {
		/* if we're going to abort the death, it *must* be before
		 * the m_detach or there will be relmon problems later */
		if(!grddead(mtmp)) return;
	}
	lifesaved_monster(mtmp);
	if (mtmp->mhp > 0) return;

#ifdef STEED
	/* Player is thrown from his steed when it dies */
	if (mtmp == u.usteed)
		dismount_steed(DISMOUNT_GENERIC);
#endif

	mptr = mtmp->data;		/* save this for m_detach() */
	/* restore chameleon, lycanthropes to true form at death */
	if (mtmp->cham)
	    set_mon_data(mtmp, &mons[cham_to_pm[mtmp->cham]], -1);
	else if (mtmp->data == &mons[PM_WEREJACKAL])
	    set_mon_data(mtmp, &mons[PM_HUMAN_WEREJACKAL], -1);
	else if (mtmp->data == &mons[PM_WEREWOLF])
	    set_mon_data(mtmp, &mons[PM_HUMAN_WEREWOLF], -1);
	else if (mtmp->data == &mons[PM_WERERAT])
	    set_mon_data(mtmp, &mons[PM_HUMAN_WERERAT], -1);

	/* if MAXMONNO monsters of a given type have died, and it
	 * can be done, extinguish that monster.
	 *
	 * mvitals[].died does double duty as total number of dead monsters
	 * and as experience factor for the player killing more monsters.
	 * this means that a dragon dying by other means reduces the
	 * experience the player gets for killing a dragon directly; this
	 * is probably not too bad, since the player likely finagled the
	 * first dead dragon via ring of conflict or pets, and extinguishing
	 * based on only player kills probably opens more avenues of abuse
	 * for rings of conflict and such.
	 */
	tmp = monsndx(mtmp->data);
	if (mvitals[tmp].died < 255) mvitals[tmp].died++;

	/* if it's a (possibly polymorphed) quest leader, mark him as dead */
	if (mtmp->m_id == quest_status.leader_m_id)
	    quest_status.leader_is_dead = TRUE;
#ifdef MAIL
	/* if the mail daemon dies, no more mail delivery.  -3. */
	if (tmp == PM_MAIL_DAEMON) mvitals[tmp].mvflags |= G_GENOD;
#endif

#ifdef KOPS
	if (mtmp->data->mlet == S_KOP) {
	    /* Dead Kops may come back. */
	    switch(rnd(5)) {
		case 1:	     /* returns near the stairs */
			(void) makemon(mtmp->data,xdnstair,ydnstair,NO_MM_FLAGS);
			break;
		case 2:	     /* randomly */
			(void) makemon(mtmp->data,0,0,NO_MM_FLAGS);
			break;
		default:
			break;
	    }
	}
#endif
	if(mtmp->iswiz) wizdead();
	if(mtmp->data->msound == MS_NEMESIS) nemdead();
	if(glyph_is_invisible(levl[mtmp->mx][mtmp->my].glyph))
	    unmap_object(mtmp->mx, mtmp->my);
	m_detach(mtmp, mptr);
}

/* TRUE if corpse might be dropped, magr may die if mon was swallowed */
boolean
corpse_chance(mon, magr, was_swallowed)
struct monst *mon;
struct monst *magr;			/* killer, if swallowed */
boolean was_swallowed;			/* digestion */
{
	struct permonst *mdat = mon->data;
	int i, tmp;

	if (mdat == &mons[PM_VLAD_THE_IMPALER] || mdat->mlet == S_LICH) {
	    if (cansee(mon->mx, mon->my) && !was_swallowed)
		pline("%s body crumbles into dust.", s_suffix(Monnam(mon)));
	    return FALSE;
	}

	/* Gas spores always explode upon death */
	for(i = 0; i < NATTK; i++) {
	    if (mdat->mattk[i].aatyp == AT_BOOM) {
	    	if (mdat->mattk[i].damn)
	    	    tmp = d((int)mdat->mattk[i].damn,
	    	    		(int)mdat->mattk[i].damd);
	    	else if(mdat->mattk[i].damd)
	    	    tmp = d((int)mdat->mlevel+1, (int)mdat->mattk[i].damd);
	    	else tmp = 0;
		if (was_swallowed && magr) {
		    if (magr == &youmonst) {
			There("is an explosion in your %s!",
			      body_part(STOMACH));
			Sprintf(killer_buf, "%s explosion",
				s_suffix(mdat->mname));
			if (Half_physical_damage) tmp = (tmp+1) / 2;
			losehp(tmp, killer_buf, KILLED_BY_AN);
		    } else {
			if (flags.soundok) You_hear("an explosion.");
			magr->mhp -= tmp;
			if (magr->mhp < 1) mondied(magr);
			if (magr->mhp < 1) { /* maybe lifesaved */
			    if (canspotmon(magr))
				pline("%s rips open!", Monnam(magr));
			} else if (canseemon(magr))
			    pline("%s seems to have indigestion.",
				  Monnam(magr));
		    }

		    return FALSE;
		}

	    	Sprintf(killer_buf, "%s explosion", s_suffix(mdat->mname));
	    	killer = killer_buf;
	    	killer_format = KILLED_BY_AN;
	    	explode(mon->mx, mon->my, -1, tmp, MON_EXPLODE, EXPL_NOXIOUS); 
	    	return (FALSE);
	    }
  	}

	/* must duplicate this below check in xkilled() since it results in
	 * creating no objects as well as no corpse
	 */
	if (LEVEL_SPECIFIC_NOCORPSE(mdat))
		return FALSE;

	if (bigmonst(mdat) || mdat == &mons[PM_LIZARD]
		   || is_golem(mdat)
		   || is_mplayer(mdat)
		   || is_rider(mdat))
		return TRUE;
	return (boolean) (!rn2((int)
		(2 + ((int)(mdat->geno & G_FREQ)<2) + verysmall(mdat))));
}

/* drop (perhaps) a cadaver and remove monster */
void
mondied(mdef)
register struct monst *mdef;
{
	mondead(mdef);
	if (mdef->mhp > 0) return;	/* lifesaved */

	if (corpse_chance(mdef, (struct monst *)0, FALSE) &&
	    (accessible(mdef->mx, mdef->my) || is_pool(mdef->mx, mdef->my)))
		(void) make_corpse(mdef);
}

/* monster disappears, not dies */
void
mongone(mdef)
register struct monst *mdef;
{
#ifdef STEED
	/* Player is thrown from his steed when it disappears */
	if (mdef == u.usteed)
		dismount_steed(DISMOUNT_GENERIC);
#endif

	discard_minvent(mdef);	/* release monster's inventory */
#ifndef GOLDOBJ
	mdef->mgold = 0L;
#endif
	m_detach(mdef, mdef->data);
}

/* drop a statue or rock and remove monster */
void
monstone(mdef)
register struct monst *mdef;
{
	struct obj *otmp, *obj, *oldminvent;
	xchar x = mdef->mx, y = mdef->my;
	boolean wasinside = FALSE;

	/* we have to make the statue before calling mondead, to be able to
	 * put inventory in it, and we have to check for lifesaving before
	 * making the statue....
	 */
	lifesaved_monster(mdef);
	if (mdef->mhp > 0) return;

	mdef->mtrapped = 0;	/* (see m_detach) */

	if ((int)mdef->data->msize > MZ_TINY ||
		    !rn2(2 + ((int) (mdef->data->geno & G_FREQ) > 2))) {
		oldminvent = 0;
		/* some objects may end up outside the statue */
		while ((obj = mdef->minvent) != 0) {
		    obj_extract_self(obj);
		    if (obj->owornmask)
			update_mon_intrinsics(mdef, obj, FALSE, TRUE);
		    obj_no_longer_held(obj);
		    if (obj->owornmask & W_WEP)
			setmnotwielded(mdef,obj);
		    obj->owornmask = 0L;
		    if (obj->otyp == BOULDER ||
#if 0				/* monsters don't carry statues */
     (obj->otyp == STATUE && mons[obj->corpsenm].msize >= mdef->data->msize) ||
#endif
				obj_resists(obj, 0, 0)) {
			if (flooreffects(obj, x, y, "fall")) continue;
			place_object(obj, x, y);
		    } else {
			if (obj->lamplit) end_burn(obj, TRUE);
			obj->nobj = oldminvent;
			oldminvent = obj;
		    }
		}
		/* defer statue creation until after inventory removal
		   so that saved monster traits won't retain any stale
		   item-conferred attributes */
		otmp = mkcorpstat(STATUE, mdef, mdef->data, x, y, FALSE);
		if (mdef->mnamelth) otmp = oname(otmp, NAME(mdef));
		while ((obj = oldminvent) != 0) {
		    oldminvent = obj->nobj;
		    (void) add_to_container(otmp, obj);
		}
#ifndef GOLDOBJ
		if (mdef->mgold) {
			struct obj *au;
			au = mksobj(GOLD_PIECE, FALSE, FALSE);
			au->quan = mdef->mgold;
			au->owt = weight(au);
			(void) add_to_container(otmp, au);
			mdef->mgold = 0;
		}
#endif
		/* Archeologists should not break unique statues */
		if (mdef->data->geno & G_UNIQ)
			otmp->spe = 1;
		otmp->owt = weight(otmp);
	} else
		otmp = mksobj_at(ROCK, x, y, TRUE, FALSE);

	stackobj(otmp);
	/* mondead() already does this, but we must do it before the newsym */
	if(glyph_is_invisible(levl[x][y].glyph))
	    unmap_object(x, y);
	if (cansee(x, y)) newsym(x,y);
	/* We don't currently trap the hero in the statue in this case but we could */
	if (u.uswallow && u.ustuck == mdef) wasinside = TRUE;
	mondead(mdef);
	if (wasinside) {
		if (is_animal(mdef->data))
			You("%s through an opening in the new %s.",
				locomotion(youmonst.data, "jump"),
				xname(otmp));
	}
}

/* another monster has killed the monster mdef */
void
monkilled(mdef, fltxt, how)
register struct monst *mdef;
const char *fltxt;
int how;
{
	boolean be_sad = FALSE;		/* true if unseen pet is killed */

	if ((mdef->wormno ? worm_known(mdef) : cansee(mdef->mx, mdef->my))
		&& fltxt)
	    pline("%s is %s%s%s!", Monnam(mdef),
			nonliving(mdef->data) ? "destroyed" : "killed",
		    *fltxt ? " by the " : "",
		    fltxt
		 );
	else
	    be_sad = (mdef->mtame != 0);

	/* no corpses if digested or disintegrated */
	if(how == AD_DGST || how == -AD_RBRE)
	    mondead(mdef);
	else
	    mondied(mdef);

	if (be_sad && mdef->mhp <= 0)
	    You("have a sad feeling for a moment, then it passes.");
}

void
unstuck(mtmp)
register struct monst *mtmp;
{
	if(u.ustuck == mtmp) {
		if(u.uswallow){
			u.ux = mtmp->mx;
			u.uy = mtmp->my;
			u.uswallow = 0;
			u.uswldtim = 0;
			if (Punished) placebc();
			vision_full_recalc = 1;
			docrt();
		}
		u.ustuck = 0;
	}
}

void
killed(mtmp)
register struct monst *mtmp;
{
	xkilled(mtmp, 1);
}

/* the player has killed the monster mtmp */
void
xkilled(mtmp, dest)
	register struct monst *mtmp;
/*
 * Dest=1, normal; dest=0, don't print message; dest=2, don't drop corpse
 * either; dest=3, message but no corpse
 */
	int	dest;
{
	register int tmp, x = mtmp->mx, y = mtmp->my;
	register struct permonst *mdat;
	int mndx;
	register struct obj *otmp;
	register struct trap *t;
	boolean redisp = FALSE;
	boolean wasinside = u.uswallow && (u.ustuck == mtmp);


	/* KMH, conduct */
	u.uconduct.killer++;

	if (dest & 1) {
	    const char *verb = nonliving(mtmp->data) ? "destroy" : "kill";

	    if (!wasinside && !canspotmon(mtmp))
		You("%s it!", verb);
	    else {
		You("%s %s!", verb,
		    !mtmp->mtame ? mon_nam(mtmp) :
			x_monnam(mtmp,
				 mtmp->mnamelth ? ARTICLE_NONE : ARTICLE_THE,
				 "poor",
				 mtmp->mnamelth ? SUPPRESS_SADDLE : 0,
				 FALSE));
	    }
	}

	if (mtmp->mtrapped && (t = t_at(x, y)) != 0 &&
		(t->ttyp == PIT || t->ttyp == SPIKED_PIT) &&
		sobj_at(BOULDER, x, y))
	    dest |= 2;     /*
			    * Prevent corpses/treasure being created "on top"
			    * of the boulder that is about to fall in. This is
			    * out of order, but cannot be helped unless this
			    * whole routine is rearranged.
			    */

	/* your pet knows who just killed it...watch out */
	if (mtmp->mtame && !mtmp->isminion) EDOG(mtmp)->killed_by_u = 1;

	/* dispose of monster and make cadaver */
	if(stoned) monstone(mtmp);
	else mondead(mtmp);

	if (mtmp->mhp > 0) { /* monster lifesaved */
		/* Cannot put the non-visible lifesaving message in
		 * lifesaved_monster() since the message appears only when you
		 * kill it (as opposed to visible lifesaving which always
		 * appears).
		 */
		stoned = FALSE;
		if (!cansee(x,y)) pline("Maybe not...");
		return;
	}

	mdat = mtmp->data; /* note: mondead can change mtmp->data */
	mndx = monsndx(mdat);

	if (stoned) {
		stoned = FALSE;
		goto cleanup;
	}

	if((dest & 2) || LEVEL_SPECIFIC_NOCORPSE(mdat))
		goto cleanup;

#ifdef MAIL
	if(mdat == &mons[PM_MAIL_DAEMON]) {
		stackobj(mksobj_at(SCR_MAIL, x, y, FALSE, FALSE));
		redisp = TRUE;
	}
#endif
	if((!accessible(x, y) && !is_pool(x, y)) ||
	   (x == u.ux && y == u.uy)) {
	    /* might be mimic in wall or corpse in lava or on player's spot */
	    redisp = TRUE;
	    if(wasinside) spoteffects(TRUE);
	} else if(x != u.ux || y != u.uy) {
		/* might be here after swallowed */
		if (!rn2(6) && !(mvitals[mndx].mvflags & G_NOCORPSE)
#ifdef KOPS
					&& mdat->mlet != S_KOP
#endif
							) {
			int typ;

			otmp = mkobj_at(RANDOM_CLASS, x, y, TRUE);
			/* Don't create large objects from small monsters */
			typ = otmp->otyp;
			if (mdat->msize < MZ_HUMAN && typ != FOOD_RATION
			    && typ != LEASH
			    && typ != FIGURINE
			    && (otmp->owt > 3 ||
				objects[typ].oc_big /*oc_bimanual/oc_bulky*/ ||
				is_spear(otmp) || is_pole(otmp) ||
				typ == MORNING_STAR)) {
			    delobj(otmp);
			} else redisp = TRUE;
		}
		/* Whether or not it always makes a corpse is, in theory,
		 * different from whether or not the corpse is "special";
		 * if we want both, we have to specify it explicitly.
		 */
		if (corpse_chance(mtmp, (struct monst *)0, FALSE))
			(void) make_corpse(mtmp);
	}
	if(redisp) newsym(x,y);
cleanup:
	/* punish bad behaviour */
	if(is_human(mdat) && (!always_hostile(mdat) && mtmp->malign <= 0) &&
	   (mndx < PM_ARCHEOLOGIST || mndx > PM_WIZARD) &&
	   u.ualign.type != A_CHAOTIC) {
		HTelepat &= ~INTRINSIC;
		change_luck(-2);
		You("murderer!");
		if (Blind && !Blind_telepat)
		    see_monsters(); /* Can't sense monsters any more. */
	}
	if((mtmp->mpeaceful && !rn2(2)) || mtmp->mtame)	change_luck(-1);
	if (is_unicorn(mdat) &&
				sgn(u.ualign.type) == sgn(mdat->maligntyp)) {
		change_luck(-5);
		You_feel("guilty...");
	}

	/* give experience points */
	tmp = experience(mtmp, (int)mvitals[mndx].died + 1);
	more_experienced(tmp, 0);
	newexplevel();		/* will decide if you go up */

	/* adjust alignment points */
	if (mtmp->m_id == quest_status.leader_m_id) {		/* REAL BAD! */
	    adjalign(-(u.ualign.record+(int)ALIGNLIM/2));
	    pline("That was %sa bad idea...",
	    		u.uevent.qcompleted ? "probably " : "");
	} else if (mdat->msound == MS_NEMESIS)	/* Real good! */
	    adjalign((int)(ALIGNLIM/4));
	else if (mdat->msound == MS_GUARDIAN) {	/* Bad */
	    adjalign(-(int)(ALIGNLIM/8));
	    if (!Hallucination) pline("That was probably a bad idea...");
	    else pline("Whoopsie-daisy!");
	}else if (mtmp->ispriest) {
		adjalign((p_coaligned(mtmp)) ? -2 : 2);
		/* cancel divine protection for killing your priest */
		if (p_coaligned(mtmp)) u.ublessed = 0;
		if (mdat->maligntyp == A_NONE)
			adjalign((int)(ALIGNLIM / 4));		/* BIG bonus */
	} else if (mtmp->mtame) {
		adjalign(-15);	/* bad!! */
		/* your god is mighty displeased... */
		if (!Hallucination) You_hear("the rumble of distant thunder...");
		else You_hear("the studio audience applaud!");
	} else if (mtmp->mpeaceful)
		adjalign(-5);

	/* malign was already adjusted for u.ualign.type and randomization */
	adjalign(mtmp->malign);
}

/* changes the monster into a stone monster of the same type */
/* this should only be called when poly_when_stoned() is true */
void
mon_to_stone(mtmp)
    register struct monst *mtmp;
{
    if(mtmp->data->mlet == S_GOLEM) {
	/* it's a golem, and not a stone golem */
	if(canseemon(mtmp))
	    pline("%s solidifies...", Monnam(mtmp));
	if (newcham(mtmp, &mons[PM_STONE_GOLEM], FALSE, FALSE)) {
	    if(canseemon(mtmp))
		pline("Now it's %s.", an(mtmp->data->mname));
	} else {
	    if(canseemon(mtmp))
		pline("... and returns to normal.");
	}
    } else
	impossible("Can't polystone %s!", a_monnam(mtmp));
}

void
mnexto(mtmp)	/* Make monster mtmp next to you (if possible) */
	struct monst *mtmp;
{
	coord mm;

#ifdef STEED
	if (mtmp == u.usteed) {
		/* Keep your steed in sync with you instead */
		mtmp->mx = u.ux;
		mtmp->my = u.uy;
		return;
	}
#endif

	if(!enexto(&mm, u.ux, u.uy, mtmp->data)) return;
	rloc_to(mtmp, mm.x, mm.y);
	return;
}

/* mnearto()
 * Put monster near (or at) location if possible.
 * Returns:
 *	1 - if a monster was moved from x, y to put mtmp at x, y.
 *	0 - in most cases.
 */
boolean
mnearto(mtmp,x,y,move_other)
register struct monst *mtmp;
xchar x, y;
boolean move_other;	/* make sure mtmp gets to x, y! so move m_at(x, y) */
{
	struct monst *othermon = (struct monst *)0;
	xchar newx, newy;
	coord mm;

	if ((mtmp->mx == x) && (mtmp->my == y)) return(FALSE);

	if (move_other && (othermon = m_at(x, y))) {
		if (othermon->wormno)
			remove_worm(othermon);
		else
			remove_monster(x, y);
	}

	newx = x;
	newy = y;

	if (!goodpos(newx, newy, mtmp, 0)) {
		/* actually we have real problems if enexto ever fails.
		 * migrating_mons that need to be placed will cause
		 * no end of trouble.
		 */
		if (!enexto(&mm, newx, newy, mtmp->data)) return(FALSE);
		newx = mm.x; newy = mm.y;
	}

	rloc_to(mtmp, newx, newy);

	if (move_other && othermon) {
	    othermon->mx = othermon->my = 0;
	    (void) mnearto(othermon, x, y, FALSE);
	    if ((othermon->mx != x) || (othermon->my != y))
		return(TRUE);
	}

	return(FALSE);
}


static const char *poiseff[] = {

	" feel weaker", "r brain is on fire",
	"r judgement is impaired", "r muscles won't obey you",
	" feel very sick", " break out in hives"
};

void
poisontell(typ)

	int	typ;
{
	pline("You%s.", poiseff[typ]);
}

void
poisoned(string, typ, pname, fatal)
const char *string, *pname;
int  typ, fatal;
{
	int i, plural, kprefix = KILLED_BY_AN;
	boolean thrown_weapon = (fatal < 0);

	if (thrown_weapon) fatal = -fatal;
	if(strcmp(string, "blast") && !thrown_weapon) {
	    /* 'blast' has already given a 'poison gas' message */
	    /* so have "poison arrow", "poison dart", etc... */
	    plural = (string[strlen(string) - 1] == 's')? 1 : 0;
	    /* avoid "The" Orcus's sting was poisoned... */
	    pline("%s%s %s poisoned!", isupper(*string) ? "" : "The ",
			string, plural ? "were" : "was");
	}

	if(Poison_resistance) {
		if(!strcmp(string, "blast")) shieldeff(u.ux, u.uy);
		pline_The("poison doesn't seem to affect you.");
		return;
	}
	/* suppress killer prefix if it already has one */
	if (!strncmpi(pname, "the ", 4) ||
		!strncmpi(pname, "an ", 3) ||
		!strncmpi(pname, "a ", 2) ||
	    /* ... or if it seems to be a proper name */
		isupper(*pname)) {
	    /*[ does this need a plural check too? ]*/
	    kprefix = KILLED_BY;
	}
	i = rn2(fatal + 20*thrown_weapon);
	if(i == 0 && typ != A_CHA) {
		u.uhp = -1;
		pline_The("poison was deadly...");
	} else if(i <= 5) {
		/* Check that a stat change was made */
		if (adjattrib(typ, thrown_weapon ? -1 : -rn1(3,3), 1))
		    pline("You%s!", poiseff[typ]);
	} else {
		i = thrown_weapon ? rnd(6) : rn1(10,6);
		if(Half_physical_damage) i = (i+1) / 2;
		losehp(i, pname, kprefix);
	}
	if(u.uhp < 1) {
		killer_format = kprefix;
		killer = pname;
		/* "Poisoned by a poisoned ___" is redundant */
		done(strstri(pname, "poison") ? DIED : POISONING);
	}
	(void) encumber_msg();
}

/* monster responds to player action; not the same as a passive attack */
/* assumes reason for response has been tested, and response _must_ be made */
void
m_respond(mtmp)
register struct monst *mtmp;
{
    if(mtmp->data->msound == MS_SHRIEK) {
	if(flags.soundok) {
	    pline("%s shrieks.", Monnam(mtmp));
	    stop_occupation();
	}
	if (!rn2(10)) {
	    if (!rn2(13))
		(void) makemon(&mons[PM_PURPLE_WORM], 0, 0, NO_MM_FLAGS);
	    else
		(void) makemon((struct permonst *)0, 0, 0, NO_MM_FLAGS);

	}
	aggravate();
    }
    if(mtmp->data == &mons[PM_MEDUSA]) {
	register int i;
	for(i = 0; i < NATTK; i++)
	     if(mtmp->data->mattk[i].aatyp == AT_GAZE) {
		 (void) gazemu(mtmp, &mtmp->data->mattk[i]);
		 break;
	     }
    }
}

#endif /* OVLB */
#ifdef OVL2

void
setmangry(mtmp)
register struct monst *mtmp;
{
	mtmp->mstrategy &= ~STRAT_WAITMASK;
	if(!mtmp->mpeaceful) return;
	if(mtmp->mtame) return;
	mtmp->mpeaceful = 0;
	if(mtmp->ispriest) {
		if(p_coaligned(mtmp)) adjalign(-5); /* very bad */
		else adjalign(2);
	} else
		adjalign(-1);		/* attacking peaceful monsters is bad */
	if (couldsee(mtmp->mx, mtmp->my)) {
		if (humanoid(mtmp->data) || mtmp->isshk || mtmp->isgd)
		    pline("%s gets angry!", Monnam(mtmp));
		else if (flags.verbose && flags.soundok) growl(mtmp);
	}

	/* attacking your own quest leader will anger his or her guardians */
	if (!flags.mon_moving &&	/* should always be the case here */
		mtmp->data == &mons[quest_info(MS_LEADER)]) {
	    struct monst *mon;
	    struct permonst *q_guardian = &mons[quest_info(MS_GUARDIAN)];
	    int got_mad = 0;

	    /* guardians will sense this attack even if they can't see it */
	    for (mon = fmon; mon; mon = mon->nmon)
		if (!DEADMONSTER(mon) && mon->data == q_guardian && mon->mpeaceful) {
		    mon->mpeaceful = 0;
		    if (canseemon(mon)) ++got_mad;
		}
	    if (got_mad && !Hallucination)
		pline_The("%s appear%s to be angry too...",
		      got_mad == 1 ? q_guardian->mname :
				    makeplural(q_guardian->mname),
		      got_mad == 1 ? "s" : "");
	}
}

void
wakeup(mtmp)
register struct monst *mtmp;
{
	mtmp->msleeping = 0;
	mtmp->meating = 0;	/* assume there's no salvagable food left */
	setmangry(mtmp);
	if(mtmp->m_ap_type) seemimic(mtmp);
	else if (flags.forcefight && !flags.mon_moving && mtmp->mundetected) {
	    mtmp->mundetected = 0;
	    newsym(mtmp->mx, mtmp->my);
	}
}

/* Wake up nearby monsters. */
void
wake_nearby()
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (!DEADMONSTER(mtmp) && distu(mtmp->mx,mtmp->my) < u.ulevel*20) {
		mtmp->msleeping = 0;
		if (mtmp->mtame && !mtmp->isminion)
		    EDOG(mtmp)->whistletime = moves;
	    }
	}
}

/* Wake up monsters near some particular location. */
void
wake_nearto(x, y, distance)
register int x, y, distance;
{
	register struct monst *mtmp;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (!DEADMONSTER(mtmp) && mtmp->msleeping && (distance == 0 ||
				 dist2(mtmp->mx, mtmp->my, x, y) < distance))
		mtmp->msleeping = 0;
	}
}

/* NOTE: we must check for mimicry before calling this routine */
void
seemimic(mtmp)
register struct monst *mtmp;
{
	unsigned old_app = mtmp->mappearance;
	uchar old_ap_type = mtmp->m_ap_type;

	mtmp->m_ap_type = M_AP_NOTHING;
	mtmp->mappearance = 0;

	/*
	 *  Discovered mimics don't block light.
	 */
	if (((old_ap_type == M_AP_FURNITURE &&
	      (old_app == S_hcdoor || old_app == S_vcdoor)) ||
	     (old_ap_type == M_AP_OBJECT && old_app == BOULDER)) &&
	    !does_block(mtmp->mx, mtmp->my, &levl[mtmp->mx][mtmp->my]))
	    unblock_point(mtmp->mx, mtmp->my);

	newsym(mtmp->mx,mtmp->my);
}

/* force all chameleons to become normal */
void
rescham()
{
	register struct monst *mtmp;
	int mcham;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		mcham = (int) mtmp->cham;
		if (mcham) {
			mtmp->cham = CHAM_ORDINARY;
			(void) newcham(mtmp, &mons[cham_to_pm[mcham]],
				       FALSE, FALSE);
		}
		if(is_were(mtmp->data) && mtmp->data->mlet != S_HUMAN)
			new_were(mtmp);
		if(mtmp->m_ap_type && cansee(mtmp->mx, mtmp->my)) {
			seemimic(mtmp);
			/* we pretend that the mimic doesn't */
			/* know that it has been unmasked.   */
			mtmp->msleeping = 1;
		}
	}
}

/* Let the chameleons change again -dgk */
void
restartcham()
{
	register struct monst *mtmp;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		mtmp->cham = pm_to_cham(monsndx(mtmp->data));
		if (mtmp->data->mlet == S_MIMIC && mtmp->msleeping &&
				cansee(mtmp->mx, mtmp->my)) {
			set_mimic_sym(mtmp);
			newsym(mtmp->mx,mtmp->my);
		}
	}
}

/* called when restoring a monster from a saved level; protection
   against shape-changing might be different now than it was at the
   time the level was saved. */
void
restore_cham(mon)
struct monst *mon;
{
	int mcham;

	if (Protection_from_shape_changers) {
	    mcham = (int) mon->cham;
	    if (mcham) {
		mon->cham = CHAM_ORDINARY;
		(void) newcham(mon, &mons[cham_to_pm[mcham]], FALSE, FALSE);
	    } else if (is_were(mon->data) && !is_human(mon->data)) {
		new_were(mon);
	    }
	} else if (mon->cham == CHAM_ORDINARY) {
	    mon->cham = pm_to_cham(monsndx(mon->data));
	}
}

/* unwatched hiders may hide again; if so, a 1 is returned.  */
STATIC_OVL boolean
restrap(mtmp)
register struct monst *mtmp;
{
	if(mtmp->cham || mtmp->mcan || mtmp->m_ap_type ||
	   cansee(mtmp->mx, mtmp->my) || rn2(3) || (mtmp == u.ustuck) ||
	   (sensemon(mtmp) && distu(mtmp->mx, mtmp->my) <= 2))
		return(FALSE);

	if(mtmp->data->mlet == S_MIMIC) {
		set_mimic_sym(mtmp);
		return(TRUE);
	} else
	    if(levl[mtmp->mx][mtmp->my].typ == ROOM)  {
		mtmp->mundetected = 1;
		return(TRUE);
	    }

	return(FALSE);
}

short *animal_list = 0;		/* list of PM values for animal monsters */
int animal_list_count;

void
mon_animal_list(construct)
boolean construct;
{
	if (construct) {
	    short animal_temp[SPECIAL_PM];
	    int i, n;

	 /* if (animal_list) impossible("animal_list already exists"); */

	    for (n = 0, i = LOW_PM; i < SPECIAL_PM; i++)
		if (is_animal(&mons[i])) animal_temp[n++] = i;
	 /* if (n == 0) animal_temp[n++] = NON_PM; */

	    animal_list = (short *)alloc(n * sizeof *animal_list);
	    (void) memcpy((genericptr_t)animal_list,
			  (genericptr_t)animal_temp,
			  n * sizeof *animal_list);
	    animal_list_count = n;
	} else {	/* release */
	    if (animal_list) free((genericptr_t)animal_list), animal_list = 0;
	    animal_list_count = 0;
	}
}

STATIC_OVL int
pick_animal()
{
	if (!animal_list) mon_animal_list(TRUE);

	return animal_list[rn2(animal_list_count)];
}

STATIC_OVL int
select_newcham_form(mon)
struct monst *mon;
{
	int mndx = NON_PM;

	switch (mon->cham) {
	    case CHAM_SANDESTIN:
		if (rn2(7)) mndx = pick_nasty();
		break;
	    case CHAM_DOPPELGANGER:
		if (!rn2(7)) mndx = pick_nasty();
		else if (rn2(3)) mndx = rn1(PM_WIZARD - PM_ARCHEOLOGIST + 1,
					    PM_ARCHEOLOGIST);
		break;
	    case CHAM_CHAMELEON:
		if (!rn2(3)) mndx = pick_animal();
		break;
	    case CHAM_ORDINARY:
	      {
		struct obj *m_armr = which_armor(mon, W_ARM);

		if (m_armr && Is_dragon_scales(m_armr))
		    mndx = Dragon_scales_to_pm(m_armr) - mons;
		else if (m_armr && Is_dragon_mail(m_armr))
		    mndx = Dragon_mail_to_pm(m_armr) - mons;
	      }
		break;
	}
#ifdef WIZARD
	/* For debugging only: allow control of polymorphed monster; not saved */
	if (wizard && iflags.mon_polycontrol) {
		char pprompt[BUFSZ], buf[BUFSZ];
		int tries = 0;
		do {
			Sprintf(pprompt,
				"Change %s into what kind of monster? [type the name]",
				mon_nam(mon));
			getlin(pprompt,buf);
			mndx = name_to_mon(buf);
			if (mndx < LOW_PM)
				You("cannot polymorph %s into that.", mon_nam(mon));
			else break;
		} while(++tries < 5);
		if (tries==5) pline(thats_enough_tries);
	}
#endif /*WIZARD*/
	if (mndx == NON_PM) mndx = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
	return mndx;
}

/* make a chameleon look like a new monster; returns 1 if it actually changed */
int
newcham(mtmp, mdat, polyspot, msg)
struct monst *mtmp;
struct permonst *mdat;
boolean polyspot;	/* change is the result of wand or spell of polymorph */
boolean msg;		/* "The oldmon turns into a newmon!" */
{
	int mhp, hpn, hpd;
	int mndx, tryct;
	struct permonst *olddata = mtmp->data;
	char oldname[BUFSZ];

	if (msg) {
	    /* like Monnam() but never mention saddle */
	    Strcpy(oldname, x_monnam(mtmp, ARTICLE_THE, (char *)0,
				     SUPPRESS_SADDLE, FALSE));
	    oldname[0] = highc(oldname[0]);
	}

	/* mdat = 0 -> caller wants a random monster shape */
	tryct = 0;
	if (mdat == 0) {
	    while (++tryct <= 100) {
		mndx = select_newcham_form(mtmp);
		mdat = &mons[mndx];
		if ((mvitals[mndx].mvflags & G_GENOD) != 0 ||
			is_placeholder(mdat)) continue;
		/* polyok rules out all M2_PNAME and M2_WERE's;
		   select_newcham_form might deliberately pick a player
		   character type, so we can't arbitrarily rule out all
		   human forms any more */
		if (is_mplayer(mdat) || (!is_human(mdat) && polyok(mdat)))
		    break;
	    }
	    if (tryct > 100) return 0;	/* Should never happen */
	} else if (mvitals[monsndx(mdat)].mvflags & G_GENOD)
	    return(0);	/* passed in mdat is genocided */

	if(is_male(mdat)) {
		if(mtmp->female) mtmp->female = FALSE;
	} else if (is_female(mdat)) {
		if(!mtmp->female) mtmp->female = TRUE;
	} else if (!is_neuter(mdat)) {
		if(!rn2(10)) mtmp->female = !mtmp->female;
	}

	if (In_endgame(&u.uz) && is_mplayer(olddata)) {
		/* mplayers start out as "Foo the Bar", but some of the
		 * titles are inappropriate when polymorphed, particularly
		 * into the opposite sex.  players don't use ranks when
		 * polymorphed, so dropping the rank for mplayers seems
		 * reasonable.
		 */
		char *p = index(NAME(mtmp), ' ');
		if (p) {
			*p = '\0';
			mtmp->mnamelth = p - NAME(mtmp) + 1;
		}
	}

	if(mdat == mtmp->data) return(0);	/* still the same monster */

	if(mtmp->wormno) {			/* throw tail away */
		wormgone(mtmp);
		place_monster(mtmp, mtmp->mx, mtmp->my);
	}

	hpn = mtmp->mhp;
	hpd = (mtmp->m_lev < 50) ? ((int)mtmp->m_lev)*8 : mdat->mlevel;
	if(!hpd) hpd = 4;

	mtmp->m_lev = adj_lev(mdat);		/* new monster level */

	mhp = (mtmp->m_lev < 50) ? ((int)mtmp->m_lev)*8 : mdat->mlevel;
	if(!mhp) mhp = 4;

	/* new hp: same fraction of max as before */
#ifndef LINT
	mtmp->mhp = (int)(((long)hpn*(long)mhp)/(long)hpd);
#endif
	if(mtmp->mhp < 0) mtmp->mhp = hpn;	/* overflow */
/* Unlikely but not impossible; a 1HD creature with 1HP that changes into a
   0HD creature will require this statement */
	if (!mtmp->mhp) mtmp->mhp = 1;

/* and the same for maximum hit points */
	hpn = mtmp->mhpmax;
#ifndef LINT
	mtmp->mhpmax = (int)(((long)hpn*(long)mhp)/(long)hpd);
#endif
	if(mtmp->mhpmax < 0) mtmp->mhpmax = hpn;	/* overflow */
	if (!mtmp->mhpmax) mtmp->mhpmax = 1;

	/* take on the new form... */
	set_mon_data(mtmp, mdat, 0);

	if (emits_light(olddata) != emits_light(mtmp->data)) {
	    /* used to give light, now doesn't, or vice versa,
	       or light's range has changed */
	    if (emits_light(olddata))
		del_light_source(LS_MONSTER, (genericptr_t)mtmp);
	    if (emits_light(mtmp->data))
		new_light_source(mtmp->mx, mtmp->my, emits_light(mtmp->data),
				 LS_MONSTER, (genericptr_t)mtmp);
	}
	if (!mtmp->perminvis || pm_invisible(olddata))
	    mtmp->perminvis = pm_invisible(mdat);
	mtmp->minvis = mtmp->invis_blkd ? 0 : mtmp->perminvis;
	if (!(hides_under(mdat) && OBJ_AT(mtmp->mx, mtmp->my)) &&
			!(mdat->mlet == S_EEL && is_pool(mtmp->mx, mtmp->my)))
		mtmp->mundetected = 0;
	if (u.ustuck == mtmp) {
		if(u.uswallow) {
			if(!attacktype(mdat,AT_ENGL)) {
				/* Does mdat care? */
				if (!noncorporeal(mdat) && !amorphous(mdat) &&
				    !is_whirly(mdat) &&
				    (mdat != &mons[PM_YELLOW_LIGHT])) {
					You("break out of %s%s!", mon_nam(mtmp),
					    (is_animal(mdat)?
					    "'s stomach" : ""));
					mtmp->mhp = 1;  /* almost dead */
				}
				expels(mtmp, olddata, FALSE);
			} else {
				/* update swallow glyphs for new monster */
				swallowed(0);
			}
		} else if (!sticks(mdat) && !sticks(youmonst.data))
			unstuck(mtmp);
	}

#ifndef DCC30_BUG
	if (mdat == &mons[PM_LONG_WORM] && (mtmp->wormno = get_wormno()) != 0) {
#else
	/* DICE 3.0 doesn't like assigning and comparing mtmp->wormno in the
	 * same expression.
	 */
	if (mdat == &mons[PM_LONG_WORM] &&
		(mtmp->wormno = get_wormno(), mtmp->wormno != 0)) {
#endif
	    /* we can now create worms with tails - 11/91 */
	    initworm(mtmp, rn2(5));
	    if (count_wsegs(mtmp))
		place_worm_tail_randomly(mtmp, mtmp->mx, mtmp->my);
	}

	newsym(mtmp->mx,mtmp->my);

	if (msg) {
	    uchar save_mnamelth = mtmp->mnamelth;
	    mtmp->mnamelth = 0;
	    pline("%s turns into %s!", oldname,
		  mdat == &mons[PM_GREEN_SLIME] ? "slime" :
		  x_monnam(mtmp, ARTICLE_A, (char*)0, SUPPRESS_SADDLE, FALSE));
	    mtmp->mnamelth = save_mnamelth;
	}

	possibly_unwield(mtmp, polyspot);	/* might lose use of weapon */
	mon_break_armor(mtmp, polyspot);
	if (!(mtmp->misc_worn_check & W_ARMG))
	    mselftouch(mtmp, "No longer petrify-resistant, ",
			!flags.mon_moving);
	m_dowear(mtmp, FALSE);

	/* This ought to re-test can_carry() on each item in the inventory
	 * rather than just checking ex-giants & boulders, but that'd be
	 * pretty expensive to perform.  If implemented, then perhaps
	 * minvent should be sorted in order to drop heaviest items first.
	 */
	/* former giants can't continue carrying boulders */
	if (mtmp->minvent && !throws_rocks(mdat)) {
	    register struct obj *otmp, *otmp2;

	    for (otmp = mtmp->minvent; otmp; otmp = otmp2) {
		otmp2 = otmp->nobj;
		if (otmp->otyp == BOULDER) {
		    /* this keeps otmp from being polymorphed in the
		       same zap that the monster that held it is polymorphed */
		    if (polyspot) bypass_obj(otmp);
		    obj_extract_self(otmp);
		    /* probably ought to give some "drop" message here */
		    if (flooreffects(otmp, mtmp->mx, mtmp->my, "")) continue;
		    place_object(otmp, mtmp->mx, mtmp->my);
		}
	    }
	}

	return(1);
}

/* sometimes an egg will be special */
#define BREEDER_EGG (!rn2(77))

/*
 * Determine if the given monster number can be hatched from an egg.
 * Return the monster number to use as the egg's corpsenm.  Return
 * NON_PM if the given monster can't be hatched.
 */
int
can_be_hatched(mnum)
int mnum;
{
    /* ranger quest nemesis has the oviparous bit set, making it
       be possible to wish for eggs of that unique monster; turn
       such into ordinary eggs rather than forbidding them outright */
    if (mnum == PM_SCORPIUS) mnum = PM_SCORPION;

    mnum = little_to_big(mnum);
    /*
     * Queen bees lay killer bee eggs (usually), but killer bees don't
     * grow into queen bees.  Ditto for [winged-]gargoyles.
     */
    if (mnum == PM_KILLER_BEE || mnum == PM_GARGOYLE ||
	    (lays_eggs(&mons[mnum]) && (BREEDER_EGG ||
		(mnum != PM_QUEEN_BEE && mnum != PM_WINGED_GARGOYLE))))
	return mnum;
    return NON_PM;
}

/* type of egg laid by #sit; usually matches parent */
int
egg_type_from_parent(mnum, force_ordinary)
int mnum;	/* parent monster; caller must handle lays_eggs() check */
boolean force_ordinary;
{
    if (force_ordinary || !BREEDER_EGG) {
	if (mnum == PM_QUEEN_BEE) mnum = PM_KILLER_BEE;
	else if (mnum == PM_WINGED_GARGOYLE) mnum = PM_GARGOYLE;
    }
    return mnum;
}

/* decide whether an egg of the indicated monster type is viable; */
/* also used to determine whether an egg or tin can be created... */
boolean
dead_species(m_idx, egg)
int m_idx;
boolean egg;
{
	/*
	 * For monsters with both baby and adult forms, genociding either
	 * form kills all eggs of that monster.  Monsters with more than
	 * two forms (small->large->giant mimics) are more or less ignored;
	 * fortunately, none of them have eggs.  Species extinction due to
	 * overpopulation does not kill eggs.
	 */
	return (boolean)
		(m_idx >= LOW_PM &&
		 ((mvitals[m_idx].mvflags & G_GENOD) != 0 ||
		  (egg &&
		   (mvitals[big_to_little(m_idx)].mvflags & G_GENOD) != 0)));
}

/* kill off any eggs of genocided monsters */
STATIC_OVL void
kill_eggs(obj_list)
struct obj *obj_list;
{
	struct obj *otmp;

	for (otmp = obj_list; otmp; otmp = otmp->nobj)
	    if (otmp->otyp == EGG) {
		if (dead_species(otmp->corpsenm, TRUE)) {
		    /*
		     * It seems we could also just catch this when
		     * it attempted to hatch, so we wouldn't have to
		     * search all of the objlists.. or stop all
		     * hatch timers based on a corpsenm.
		     */
		    kill_egg(otmp);
		}
#if 0	/* not used */
	    } else if (otmp->otyp == TIN) {
		if (dead_species(otmp->corpsenm, FALSE))
		    otmp->corpsenm = NON_PM;	/* empty tin */
	    } else if (otmp->otyp == CORPSE) {
		if (dead_species(otmp->corpsenm, FALSE))
		    ;		/* not yet implemented... */
#endif
	    } else if (Has_contents(otmp)) {
		kill_eggs(otmp->cobj);
	    }
}

/* kill all members of genocided species */
void
kill_genocided_monsters()
{
	struct monst *mtmp, *mtmp2;
	boolean kill_cham[CHAM_MAX_INDX+1];
	int mndx;

	kill_cham[CHAM_ORDINARY] = FALSE;	/* (this is mndx==0) */
	for (mndx = 1; mndx <= CHAM_MAX_INDX; mndx++)
	  kill_cham[mndx] = (mvitals[cham_to_pm[mndx]].mvflags & G_GENOD) != 0;
	/*
	 * Called during genocide, and again upon level change.  The latter
	 * catches up with any migrating monsters as they finally arrive at
	 * their intended destinations, so possessions get deposited there.
	 *
	 * Chameleon handling:
	 *	1) if chameleons have been genocided, destroy them
	 *	   regardless of current form;
	 *	2) otherwise, force every chameleon which is imitating
	 *	   any genocided species to take on a new form.
	 */
	for (mtmp = fmon; mtmp; mtmp = mtmp2) {
	    mtmp2 = mtmp->nmon;
	    if (DEADMONSTER(mtmp)) continue;
	    mndx = monsndx(mtmp->data);
	    if ((mvitals[mndx].mvflags & G_GENOD) || kill_cham[mtmp->cham]) {
		if (mtmp->cham && !kill_cham[mtmp->cham])
		    (void) newcham(mtmp, (struct permonst *)0, FALSE, FALSE);
		else
		    mondead(mtmp);
	    }
	    if (mtmp->minvent) kill_eggs(mtmp->minvent);
	}

	kill_eggs(invent);
	kill_eggs(fobj);
	kill_eggs(level.buriedobjlist);
}

#endif /* OVL2 */
#ifdef OVLB

void
golemeffects(mon, damtype, dam)
register struct monst *mon;
int damtype, dam;
{
    int heal = 0, slow = 0;

    if (mon->data == &mons[PM_FLESH_GOLEM]) {
	if (damtype == AD_ELEC) heal = dam / 6;
	else if (damtype == AD_FIRE || damtype == AD_COLD) slow = 1;
    } else if (mon->data == &mons[PM_IRON_GOLEM]) {
	if (damtype == AD_ELEC) slow = 1;
	else if (damtype == AD_FIRE) heal = dam;
    } else {
	return;
    }
    if (slow) {
	if (mon->mspeed != MSLOW)
	    mon_adjust_speed(mon, -1, (struct obj *)0);
    }
    if (heal) {
	if (mon->mhp < mon->mhpmax) {
	    mon->mhp += dam;
	    if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
	    if (cansee(mon->mx, mon->my))
		pline("%s seems healthier.", Monnam(mon));
	}
    }
}

boolean
angry_guards(silent)
register boolean silent;
{
	register struct monst *mtmp;
	register int ct = 0, nct = 0, sct = 0, slct = 0;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		if((mtmp->data == &mons[PM_WATCHMAN] ||
			       mtmp->data == &mons[PM_WATCH_CAPTAIN])
					&& mtmp->mpeaceful) {
			ct++;
			if(cansee(mtmp->mx, mtmp->my) && mtmp->mcanmove) {
				if (distu(mtmp->mx, mtmp->my) == 2) nct++;
				else sct++;
			}
			if (mtmp->msleeping || mtmp->mfrozen) {
				slct++;
				mtmp->msleeping = mtmp->mfrozen = 0;
			}
			mtmp->mpeaceful = 0;
		}
	}
	if(ct) {
	    if(!silent) { /* do we want pline msgs? */
		if(slct) pline_The("guard%s wake%s up!",
				 slct > 1 ? "s" : "", slct == 1 ? "s" : "");
		if(nct || sct) {
			if(nct) pline_The("guard%s get%s angry!",
				nct == 1 ? "" : "s", nct == 1 ? "s" : "");
			else if(!Blind)
				You("see %sangry guard%s approaching!",
				  sct == 1 ? "an " : "", sct > 1 ? "s" : "");
		} else if(flags.soundok)
			You_hear("the shrill sound of a guard's whistle.");
	    }
	    return(TRUE);
	}
	return(FALSE);
}

void
pacify_guards()
{
	register struct monst *mtmp;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if (mtmp->data == &mons[PM_WATCHMAN] ||
		mtmp->data == &mons[PM_WATCH_CAPTAIN])
	    mtmp->mpeaceful = 1;
	}
}

void
mimic_hit_msg(mtmp, otyp)
struct monst *mtmp;
short otyp;
{
	short ap = mtmp->mappearance;

	switch(mtmp->m_ap_type) {
	    case M_AP_NOTHING:			
	    case M_AP_FURNITURE:
	    case M_AP_MONSTER:
		break;
	    case M_AP_OBJECT:
		if (otyp == SPE_HEALING || otyp == SPE_EXTRA_HEALING) {
		    pline("%s seems a more vivid %s than before.",
				The(simple_typename(ap)),
				c_obj_colors[objects[ap].oc_color]);
		}
		break;
	}
}
#endif /* OVLB */

/*mon.c*/

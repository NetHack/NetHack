/*	SCCS Id: @(#)makemon.c	3.4	2002/02/07	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "epri.h"
#include "emin.h"
#include "edog.h"
#ifdef REINCARNATION
#include <ctype.h>
#endif

STATIC_VAR NEARDATA struct monst zeromonst;

/* this assumes that a human quest leader or nemesis is an archetype
   of the corresponding role; that isn't so for some roles (tourist
   for instance) but is for the priests and monks we use it for... */
#define quest_mon_represents_role(mptr,role_pm) \
		(mptr->mlet == S_HUMAN && Role_if(role_pm) && \
		  (mptr->msound == MS_LEADER || mptr->msound == MS_NEMESIS))

#ifdef OVL0
STATIC_DCL boolean FDECL(uncommon, (int));
STATIC_DCL int FDECL(align_shift, (struct permonst *));
#endif /* OVL0 */
STATIC_DCL boolean FDECL(wrong_elem_type, (struct permonst *));
STATIC_DCL void FDECL(m_initgrp,(struct monst *,int,int,int));
STATIC_DCL void FDECL(m_initthrow,(struct monst *,int,int));
STATIC_DCL void FDECL(m_initweap,(struct monst *));
#ifdef OVL1
STATIC_DCL void FDECL(m_initinv,(struct monst *));
#endif /* OVL1 */

extern const int monstr[];

#define m_initsgrp(mtmp, x, y)	m_initgrp(mtmp, x, y, 3)
#define m_initlgrp(mtmp, x, y)	m_initgrp(mtmp, x, y, 10)
#define toostrong(monindx, lev) (monstr[monindx] > lev)
#define tooweak(monindx, lev)	(monstr[monindx] < lev)

#ifdef OVLB
boolean
is_home_elemental(ptr)
register struct permonst *ptr;
{
	if (ptr->mlet == S_ELEMENTAL)
	    switch (monsndx(ptr)) {
		case PM_AIR_ELEMENTAL: return Is_airlevel(&u.uz);
		case PM_FIRE_ELEMENTAL: return Is_firelevel(&u.uz);
		case PM_EARTH_ELEMENTAL: return Is_earthlevel(&u.uz);
		case PM_WATER_ELEMENTAL: return Is_waterlevel(&u.uz);
	    }
	return FALSE;
}

/*
 * Return true if the given monster cannot exist on this elemental level.
 */
STATIC_OVL boolean
wrong_elem_type(ptr)
    register struct permonst *ptr;
{
    if (ptr->mlet == S_ELEMENTAL) {
	return((boolean)(!is_home_elemental(ptr)));
    } else if (Is_earthlevel(&u.uz)) {
	/* no restrictions? */
    } else if (Is_waterlevel(&u.uz)) {
	/* just monsters that can swim */
	if(!is_swimmer(ptr)) return TRUE;
    } else if (Is_firelevel(&u.uz)) {
	if (!pm_resistance(ptr,MR_FIRE)) return TRUE;
    } else if (Is_airlevel(&u.uz)) {
	if(!(is_flyer(ptr) && ptr->mlet != S_TRAPPER) && !is_floater(ptr)
	   && !amorphous(ptr) && !noncorporeal(ptr) && !is_whirly(ptr))
	    return TRUE;
    }
    return FALSE;
}

STATIC_OVL void
m_initgrp(mtmp, x, y, n)	/* make a group just like mtmp */
register struct monst *mtmp;
register int x, y, n;
{
	coord mm;
	register int cnt = rnd(n);
	struct monst *mon;
#if defined(__GNUC__) && (defined(HPUX) || defined(DGUX))
	/* There is an unresolved problem with several people finding that
	 * the game hangs eating CPU; if interrupted and restored, the level
	 * will be filled with monsters.  Of those reports giving system type,
	 * there were two DG/UX and two HP-UX, all using gcc as the compiler.
	 * hcroft@hpopb1.cern.ch, using gcc 2.6.3 on HP-UX, says that the
	 * problem went away for him and another reporter-to-newsgroup
	 * after adding this debugging code.  This has almost got to be a
	 * compiler bug, but until somebody tracks it down and gets it fixed,
	 * might as well go with the "but it went away when I tried to find
	 * it" code.
	 */
	int cnttmp,cntdiv;

	cnttmp = cnt;
# ifdef DEBUG
	pline("init group call x=%d,y=%d,n=%d,cnt=%d.", x, y, n, cnt);
# endif
	cntdiv = ((u.ulevel < 3) ? 4 : (u.ulevel < 5) ? 2 : 1);
#endif
	/* Tuning: cut down on swarming at low character levels [mrs] */
	cnt /= (u.ulevel < 3) ? 4 : (u.ulevel < 5) ? 2 : 1;
#if defined(__GNUC__) && (defined(HPUX) || defined(DGUX))
	if (cnt != (cnttmp/cntdiv)) {
		pline("cnt=%d using %d, cnttmp=%d, cntdiv=%d", cnt,
			(u.ulevel < 3) ? 4 : (u.ulevel < 5) ? 2 : 1,
			cnttmp, cntdiv);
	}
#endif
	if(!cnt) cnt++;
#if defined(__GNUC__) && (defined(HPUX) || defined(DGUX))
	if (cnt < 0) cnt = 1;
	if (cnt > 10) cnt = 10;
#endif

	mm.x = x;
	mm.y = y;
	while(cnt--) {
		if (peace_minded(mtmp->data)) continue;
		/* Don't create groups of peaceful monsters since they'll get
		 * in our way.  If the monster has a percentage chance so some
		 * are peaceful and some are not, the result will just be a
		 * smaller group.
		 */
		if (enexto(&mm, mm.x, mm.y, mtmp->data)) {
		    mon = makemon(mtmp->data, mm.x, mm.y, NO_MM_FLAGS);
		    mon->mpeaceful = FALSE;
		    mon->mavenge = 0;
		    set_malign(mon);
		    /* Undo the second peace_minded() check in makemon(); if the
		     * monster turned out to be peaceful the first time we
		     * didn't create it at all; we don't want a second check.
		     */
		}
	}
}

STATIC_OVL
void
m_initthrow(mtmp,otyp,oquan)
struct monst *mtmp;
int otyp,oquan;
{
	register struct obj *otmp;

	otmp = mksobj(otyp, TRUE, FALSE);
	otmp->quan = (long) rn1(oquan, 3);
	otmp->owt = weight(otmp);
	if (otyp == ORCISH_ARROW) otmp->opoisoned = TRUE;
	(void) mpickobj(mtmp, otmp);
}

#endif /* OVLB */
#ifdef OVL2

STATIC_OVL void
m_initweap(mtmp)
register struct monst *mtmp;
{
	register struct permonst *ptr = mtmp->data;
	register int mm = monsndx(ptr);
	struct obj *otmp;

#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz)) return;
#endif
/*
 *	first a few special cases:
 *
 *		giants get a boulder to throw sometimes.
 *		ettins get clubs
 *		kobolds get darts to throw
 *		centaurs get some sort of bow & arrows or bolts
 *		soldiers get all sorts of things.
 *		kops get clubs & cream pies.
 */
	switch (ptr->mlet) {
	    case S_GIANT:
		if (rn2(2)) (void)mongets(mtmp, (mm != PM_ETTIN) ?
				    BOULDER : CLUB);
		break;
	    case S_HUMAN:
		if(is_mercenary(ptr)) {
		    int w1 = 0, w2 = 0;
		    switch (mm) {

			case PM_WATCHMAN:
			case PM_SOLDIER:
			  if (!rn2(3)) {
			      w1 = rn1(BEC_DE_CORBIN - PARTISAN + 1, PARTISAN);
			      w2 = rn2(2) ? DAGGER : KNIFE;
			  } else w1 = rn2(2) ? SPEAR : SHORT_SWORD;
			  break;
			case PM_SERGEANT:
			  w1 = rn2(2) ? FLAIL : MACE;
			  break;
			case PM_LIEUTENANT:
			  w1 = rn2(2) ? BROADSWORD : LONG_SWORD;
			  break;
			case PM_CAPTAIN:
			case PM_WATCH_CAPTAIN:
			  w1 = rn2(2) ? LONG_SWORD : SILVER_SABER;
			  break;
			default:
			  if (!rn2(4)) w1 = DAGGER;
			  if (!rn2(7)) w2 = SPEAR;
			  break;
		    }
		    if (w1) (void)mongets(mtmp, w1);
		    if (!w2 && w1 != DAGGER && !rn2(4)) w2 = KNIFE;
		    if (w2) (void)mongets(mtmp, w2);
		} else if (is_elf(ptr)) {
		    if (rn2(2))
			(void) mongets(mtmp,
				   rn2(2) ? ELVEN_MITHRIL_COAT : ELVEN_CLOAK);
		    if (rn2(2)) (void)mongets(mtmp, ELVEN_LEATHER_HELM);
		    else if (!rn2(4)) (void)mongets(mtmp, ELVEN_BOOTS);
		    if (rn2(2)) (void)mongets(mtmp, ELVEN_DAGGER);
		    switch (rn2(3)) {
			case 0:
			    if (!rn2(4)) (void)mongets(mtmp, ELVEN_SHIELD);
			    if (rn2(3)) (void)mongets(mtmp, ELVEN_SHORT_SWORD);
			    (void)mongets(mtmp, ELVEN_BOW);
			    m_initthrow(mtmp, ELVEN_ARROW, 12);
			    break;
			case 1:
			    (void)mongets(mtmp, ELVEN_BROADSWORD);
			    if (rn2(2)) (void)mongets(mtmp, ELVEN_SHIELD);
			    break;
			case 2:
			    if (rn2(2)) {
				(void)mongets(mtmp, ELVEN_SPEAR);
				(void)mongets(mtmp, ELVEN_SHIELD);
			    }
			    break;
		    }
		    if (mm == PM_ELVENKING) {
			if (rn2(3) || (in_mklev && Is_earthlevel(&u.uz)))
			    (void)mongets(mtmp, PICK_AXE);
			if (!rn2(50)) (void)mongets(mtmp, CRYSTAL_BALL);
		    }
		} else if (ptr->msound == MS_PRIEST ||
			quest_mon_represents_role(ptr,PM_PRIEST)) {
		    otmp = mksobj(MACE, FALSE, FALSE);
		    if(otmp) {
			otmp->spe = rnd(3);
			if(!rn2(2)) curse(otmp);
			(void) mpickobj(mtmp, otmp);
		    }
		}
		break;

	    case S_ANGEL:
		{
		    int spe2;

		    /* create minion stuff; can't use mongets */
		    otmp = mksobj(LONG_SWORD, FALSE, FALSE);

		    /* maybe make it special */
		    if (!rn2(20) || is_lord(ptr))
			otmp = oname(otmp, artiname(
				rn2(2) ? ART_DEMONBANE : ART_SUNSWORD));
		    bless(otmp);
		    otmp->oerodeproof = TRUE;
		    spe2 = rn2(4);
		    otmp->spe = max(otmp->spe, spe2);
		    (void) mpickobj(mtmp, otmp);

		    otmp = mksobj(!rn2(4) || is_lord(ptr) ?
				  SHIELD_OF_REFLECTION : LARGE_SHIELD,
				  FALSE, FALSE);
		    otmp->cursed = FALSE;
		    otmp->oerodeproof = TRUE;
		    otmp->spe = 0;
		    (void) mpickobj(mtmp, otmp);
		}
		break;

	    case S_HUMANOID:
		if (mm == PM_HOBBIT) {
		    switch (rn2(3)) {
			case 0:
			    (void)mongets(mtmp, DAGGER);
			    break;
			case 1:
			    (void)mongets(mtmp, ELVEN_DAGGER);
			    break;
			case 2:
			    (void)mongets(mtmp, SLING);
			    break;
		      }
		    if (!rn2(10)) (void)mongets(mtmp, ELVEN_MITHRIL_COAT);
		    if (!rn2(10)) (void)mongets(mtmp, DWARVISH_CLOAK);
		} else if (is_dwarf(ptr)) {
		    if (rn2(7)) (void)mongets(mtmp, DWARVISH_CLOAK);
		    if (rn2(7)) (void)mongets(mtmp, IRON_SHOES);
		    if (!rn2(4)) {
			(void)mongets(mtmp, DWARVISH_SHORT_SWORD);
			/* note: you can't use a mattock with a shield */
			if (rn2(2)) (void)mongets(mtmp, DWARVISH_MATTOCK);
			else {
				(void)mongets(mtmp, AXE);
				(void)mongets(mtmp, DWARVISH_ROUNDSHIELD);
			}
			(void)mongets(mtmp, DWARVISH_IRON_HELM);
			if (!rn2(3))
			    (void)mongets(mtmp, DWARVISH_MITHRIL_COAT);
		    } else {
			(void)mongets(mtmp, !rn2(3) ? PICK_AXE : DAGGER);
		    }
		}
		break;
# ifdef KOPS
	    case S_KOP:		/* create Keystone Kops with cream pies to
				 * throw. As suggested by KAA.	   [MRS]
				 */
		if (!rn2(4)) m_initthrow(mtmp, CREAM_PIE, 2);
		if (!rn2(3)) (void)mongets(mtmp,(rn2(2)) ? CLUB : RUBBER_HOSE);
		break;
# endif
	    case S_ORC:
		if(rn2(2)) (void)mongets(mtmp, ORCISH_HELM);
		switch (mm != PM_ORC_CAPTAIN ? mm :
			rn2(2) ? PM_MORDOR_ORC : PM_URUK_HAI) {
		    case PM_MORDOR_ORC:
			if(!rn2(3)) (void)mongets(mtmp, SCIMITAR);
			if(!rn2(3)) (void)mongets(mtmp, ORCISH_SHIELD);
			if(!rn2(3)) (void)mongets(mtmp, KNIFE);
			if(!rn2(3)) (void)mongets(mtmp, ORCISH_CHAIN_MAIL);
			break;
		    case PM_URUK_HAI:
			if(!rn2(3)) (void)mongets(mtmp, ORCISH_CLOAK);
			if(!rn2(3)) (void)mongets(mtmp, ORCISH_SHORT_SWORD);
			if(!rn2(3)) (void)mongets(mtmp, IRON_SHOES);
			if(!rn2(3)) {
			    (void)mongets(mtmp, ORCISH_BOW);
			    m_initthrow(mtmp, ORCISH_ARROW, 12);
			}
			if(!rn2(3)) (void)mongets(mtmp, URUK_HAI_SHIELD);
			break;
		    default:
			if (mm != PM_ORC_SHAMAN && rn2(2))
			  (void)mongets(mtmp, (mm == PM_GOBLIN || rn2(2) == 0)
						   ? ORCISH_DAGGER : SCIMITAR);
		}
		break;
	    case S_OGRE:
		if (!rn2(mm == PM_OGRE_KING ? 3 : mm == PM_OGRE_LORD ? 6 : 12))
		    (void) mongets(mtmp, BATTLE_AXE);
		else
		    (void) mongets(mtmp, CLUB);
		break;
	    case S_TROLL:
		if (!rn2(2)) switch (rn2(4)) {
		    case 0: (void)mongets(mtmp, RANSEUR); break;
		    case 1: (void)mongets(mtmp, PARTISAN); break;
		    case 2: (void)mongets(mtmp, GLAIVE); break;
		    case 3: (void)mongets(mtmp, SPETUM); break;
		}
		break;
	    case S_KOBOLD:
		if (!rn2(4)) m_initthrow(mtmp, DART, 12);
		break;

	    case S_CENTAUR:
		if (rn2(2)) {
		    if(ptr == &mons[PM_FOREST_CENTAUR]) {
			(void)mongets(mtmp, BOW);
			m_initthrow(mtmp, ARROW, 12);
		    } else {
			(void)mongets(mtmp, CROSSBOW);
			m_initthrow(mtmp, CROSSBOW_BOLT, 12);
		    }
		}
		break;
	    case S_WRAITH:
		(void)mongets(mtmp, KNIFE);
		(void)mongets(mtmp, LONG_SWORD);
		break;
	    case S_ZOMBIE:
		if (!rn2(4)) (void)mongets(mtmp, LEATHER_ARMOR);
		if (!rn2(4))
			(void)mongets(mtmp, (rn2(3) ? KNIFE : SHORT_SWORD));
		break;
	    case S_LIZARD:
		if (mm == PM_SALAMANDER)
			(void)mongets(mtmp, (rn2(7) ? SPEAR : rn2(3) ?
					     TRIDENT : STILETTO));
		break;
	    case S_DEMON:
		switch (mm) {
		    case PM_BALROG:
			(void)mongets(mtmp, BULLWHIP);
			(void)mongets(mtmp, BROADSWORD);
			break;
		    case PM_ORCUS:
			(void)mongets(mtmp, WAN_DEATH); /* the Wand of Orcus */
			break;
		    case PM_HORNED_DEVIL:
			(void)mongets(mtmp, rn2(4) ? TRIDENT : BULLWHIP);
			break;
		    case PM_DISPATER:
			(void)mongets(mtmp, WAN_STRIKING);
			break;
		    case PM_YEENOGHU:
			(void)mongets(mtmp, FLAIL);
			break;
		}
		/* prevent djinnis and mail daemons from leaving objects when
		 * they vanish
		 */
		if (!is_demon(ptr)) break;
		/* fall thru */
/*
 *	Now the general case, Some chance of getting some type
 *	of weapon for "normal" monsters.  Certain special types
 *	of monsters will get a bonus chance or different selections.
 */
	    default:
	      {
		int bias;

		bias = is_lord(ptr) + is_prince(ptr) * 2 + extra_nasty(ptr);
		switch(rnd(14 - (2 * bias))) {
		    case 1:
			if(strongmonst(ptr)) (void) mongets(mtmp, BATTLE_AXE);
			else m_initthrow(mtmp, DART, 12);
			break;
		    case 2:
			if(strongmonst(ptr))
			    (void) mongets(mtmp, TWO_HANDED_SWORD);
			else {
			    (void) mongets(mtmp, CROSSBOW);
			    m_initthrow(mtmp, CROSSBOW_BOLT, 12);
			}
			break;
		    case 3:
			(void) mongets(mtmp, BOW);
			m_initthrow(mtmp, ARROW, 12);
			break;
		    case 4:
			if(strongmonst(ptr)) (void) mongets(mtmp, LONG_SWORD);
			else m_initthrow(mtmp, DAGGER, 3);
			break;
		    case 5:
			if(strongmonst(ptr))
			    (void) mongets(mtmp, LUCERN_HAMMER);
			else (void) mongets(mtmp, AKLYS);
			break;
		    default:
			break;
		}
	      }
	      break;
	}
	if ((int) mtmp->m_lev > rn2(75))
		(void) mongets(mtmp, rnd_offensive_item(mtmp));
}

#endif /* OVL2 */
#ifdef OVL1

#ifdef GOLDOBJ
/*
 *   Makes up money for monster's inventory.
 *   This will change with silver & copper coins
 */
void 
mkmonmoney(mtmp, amount)
struct monst *mtmp;
long amount;
{
    struct obj *gold = mksobj(GOLD_PIECE, FALSE, FALSE);
    gold->quan = amount;
    add_to_minv(mtmp, gold);
}
#endif

STATIC_OVL void
m_initinv(mtmp)
register struct	monst	*mtmp;
{
	register int cnt;
	register struct obj *otmp;
	register struct permonst *ptr = mtmp->data;
#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz)) return;
#endif
/*
 *	Soldiers get armour & rations - armour approximates their ac.
 *	Nymphs may get mirror or potion of object detection.
 */
	switch(ptr->mlet) {

	    case S_HUMAN:
		if(is_mercenary(ptr)) {
		    register int mac;

		    switch(monsndx(ptr)) {
			case PM_GUARD: mac = -1; break;
			case PM_SOLDIER: mac = 3; break;
			case PM_SERGEANT: mac = 0; break;
			case PM_LIEUTENANT: mac = -2; break;
			case PM_CAPTAIN: mac = -3; break;
			case PM_WATCHMAN: mac = 3; break;
			case PM_WATCH_CAPTAIN: mac = -2; break;
			default: impossible("odd mercenary %d?", monsndx(ptr));
				mac = 0;
				break;
		    }

		    if (mac < -1 && rn2(5))
			mac += 7 + mongets(mtmp, (rn2(5)) ?
					   PLATE_MAIL : CRYSTAL_PLATE_MAIL);
		    else if (mac < 3 && rn2(5))
			mac += 6 + mongets(mtmp, (rn2(3)) ?
					   SPLINT_MAIL : BANDED_MAIL);
		    else if (rn2(5))
			mac += 3 + mongets(mtmp, (rn2(3)) ?
					   RING_MAIL : STUDDED_LEATHER_ARMOR);
		    else
			mac += 2 + mongets(mtmp, LEATHER_ARMOR);

		    if (mac < 10 && rn2(3))
			mac += 1 + mongets(mtmp, HELMET);
		    else if (mac < 10 && rn2(2))
			mac += 1 + mongets(mtmp, DENTED_POT);
		    if (mac < 10 && rn2(3))
			mac += 1 + mongets(mtmp, SMALL_SHIELD);
		    else if (mac < 10 && rn2(2))
			mac += 2 + mongets(mtmp, LARGE_SHIELD);
		    if (mac < 10 && rn2(3))
			mac += 1 + mongets(mtmp, LOW_BOOTS);
		    else if (mac < 10 && rn2(2))
			mac += 2 + mongets(mtmp, HIGH_BOOTS);
		    if (mac < 10 && rn2(3))
			mac += 1 + mongets(mtmp, LEATHER_GLOVES);
		    else if (mac < 10 && rn2(2))
			mac += 1 + mongets(mtmp, LEATHER_CLOAK);

		    if(ptr != &mons[PM_GUARD] &&
			ptr != &mons[PM_WATCHMAN] &&
			ptr != &mons[PM_WATCH_CAPTAIN]) {
			if (!rn2(3)) (void) mongets(mtmp, K_RATION);
			if (!rn2(2)) (void) mongets(mtmp, C_RATION);
			if (ptr != &mons[PM_SOLDIER] && !rn2(3))
				(void) mongets(mtmp, BUGLE);
		    } else
			   if (ptr == &mons[PM_WATCHMAN] && rn2(3))
				(void) mongets(mtmp, TIN_WHISTLE);
		} else if (ptr == &mons[PM_SHOPKEEPER]) {
		    (void) mongets(mtmp,SKELETON_KEY);
		    switch (rn2(4)) {
		    /* MAJOR fall through ... */
		    case 0: (void) mongets(mtmp, WAN_MAGIC_MISSILE);
		    case 1: (void) mongets(mtmp, POT_EXTRA_HEALING);
		    case 2: (void) mongets(mtmp, POT_HEALING);
		    case 3: (void) mongets(mtmp, WAN_STRIKING);
		    }
		} else if (ptr->msound == MS_PRIEST ||
			quest_mon_represents_role(ptr,PM_PRIEST)) {
		    (void) mongets(mtmp, rn2(7) ? ROBE :
					     rn2(3) ? CLOAK_OF_PROTECTION :
						 CLOAK_OF_MAGIC_RESISTANCE);
		    (void) mongets(mtmp, SMALL_SHIELD);
#ifndef GOLDOBJ
		    mtmp->mgold = (long)rn1(10,20);
#else
		    mkmonmoney(mtmp,(long)rn1(10,20));
#endif
		} else if (quest_mon_represents_role(ptr,PM_MONK)) {
		    (void) mongets(mtmp, rn2(11) ? ROBE :
					     CLOAK_OF_MAGIC_RESISTANCE);
		}
		break;
	    case S_NYMPH:
		if(!rn2(2)) (void) mongets(mtmp, MIRROR);
		if(!rn2(2)) (void) mongets(mtmp, POT_OBJECT_DETECTION);
		break;
	    case S_GIANT:
		if (ptr == &mons[PM_MINOTAUR]) {
		    if (!rn2(3) || (in_mklev && Is_earthlevel(&u.uz)))
			(void) mongets(mtmp, WAN_DIGGING);
		} else if (is_giant(ptr)) {
		    for (cnt = rn2((int)(mtmp->m_lev / 2)); cnt; cnt--) {
			otmp = mksobj(rnd_class(DILITHIUM_CRYSTAL,LUCKSTONE-1),
				      FALSE, FALSE);
			otmp->quan = (long) rn1(2, 3);
			otmp->owt = weight(otmp);
			(void) mpickobj(mtmp, otmp);
		    }
		}
		break;
	    case S_WRAITH:
		if (ptr == &mons[PM_NAZGUL]) {
			otmp = mksobj(RIN_INVISIBILITY, FALSE, FALSE);
			curse(otmp);
			(void) mpickobj(mtmp, otmp);
		}
		break;
	    case S_LICH:
		if (ptr == &mons[PM_MASTER_LICH] && !rn2(13))
			(void)mongets(mtmp, (rn2(7) ? ATHAME : WAN_NOTHING));
		else if (ptr == &mons[PM_ARCH_LICH] && !rn2(3)) {
			otmp = mksobj(rn2(3) ? ATHAME : QUARTERSTAFF,
				      TRUE, rn2(13) ? FALSE : TRUE);
			if (otmp->spe < 2) otmp->spe = rnd(3);
			if (!rn2(4)) otmp->oerodeproof = 1;
			(void) mpickobj(mtmp, otmp);
		}
		break;
	    case S_MUMMY:
		if (rn2(7)) (void)mongets(mtmp, MUMMY_WRAPPING);
		break;
	    case S_QUANTMECH:
		if (!rn2(20)) {
			otmp = mksobj(LARGE_BOX, FALSE, FALSE);
			otmp->spe = 1; /* flag for special box */
			otmp->owt = weight(otmp);
			(void) mpickobj(mtmp, otmp);
		}
		break;
	    case S_LEPRECHAUN:
#ifndef GOLDOBJ
		mtmp->mgold = (long) d(level_difficulty(), 30);
#else
		mkmonmoney(mtmp, (long) d(level_difficulty(), 30));
#endif
		break;
	    case S_DEMON:
	    	/* moved here from m_initweap() because these don't
		   have AT_WEAP so m_initweap() is not called for them */
		if (ptr == &mons[PM_ICE_DEVIL] && !rn2(4)) {
			(void)mongets(mtmp, SPEAR);
		} else if (ptr == &mons[PM_ASMODEUS]) {
			(void)mongets(mtmp, WAN_COLD);
			(void)mongets(mtmp, WAN_FIRE);
		}
		break;
	    default:
		break;
	}

	/* ordinary soldiers rarely have access to magic (or gold :-) */
	if (ptr == &mons[PM_SOLDIER] && rn2(13)) return;

	if ((int) mtmp->m_lev > rn2(50))
		(void) mongets(mtmp, rnd_defensive_item(mtmp));
	if ((int) mtmp->m_lev > rn2(100))
		(void) mongets(mtmp, rnd_misc_item(mtmp));
#ifndef GOLDOBJ
	if (likes_gold(ptr) && !mtmp->mgold && !rn2(5))
		mtmp->mgold =
		      (long) d(level_difficulty(), mtmp->minvent ? 5 : 10);
#else
	if (likes_gold(ptr) && !findgold(mtmp->minvent) && !rn2(5))
		mkmonmoney(mtmp, (long) d(level_difficulty(), mtmp->minvent ? 5 : 10));
#endif
}

struct monst *
clone_mon(mon)
struct monst *mon;
{
	coord mm;
	struct monst *m2;

	/* may be too weak or have been extinguished for population control */
	if (mon->mhp <= 1 || (mvitals[monsndx(mon->data)].mvflags & G_EXTINCT))
	    return (struct monst *)0;

	mm.x = mon->mx;
	mm.y = mon->my;
	if (!enexto(&mm, mm.x, mm.y, mon->data) || MON_AT(mm.x, mm.y))
	    return (struct monst *)0;
	m2 = newmonst(0);
	*m2 = *mon;			/* copy condition of old monster */
	m2->nmon = fmon;
	fmon = m2;
	m2->m_id = flags.ident++;
	if (!m2->m_id) m2->m_id = flags.ident++;	/* ident overflowed */
	m2->mx = mm.x;
	m2->my = mm.y;

	m2->minvent = (struct obj *) 0; /* objects don't clone */
	m2->mleashed = FALSE;
#ifndef GOLDOBJ
	m2->mgold = 0L;
#endif
	/* Max HP the same, but current HP halved for both.  The caller
	 * might want to override this by halving the max HP also.
	 * When current HP is odd, the original keeps the extra point.
	 */
	m2->mhpmax = mon->mhpmax;
	m2->mhp = mon->mhp / 2;
	mon->mhp -= m2->mhp;

	/* since shopkeepers and guards will only be cloned if they've been
	 * polymorphed away from their original forms, the clone doesn't have
	 * room for the extra information.  we also don't want two shopkeepers
	 * around for the same shop.
	 */
	if (mon->isshk) m2->isshk = FALSE;
	if (mon->isgd) m2->isgd = FALSE;
	if (mon->ispriest) m2->ispriest = FALSE;
	m2->mxlth = 0;
	place_monster(m2, m2->mx, m2->my);
	if (emits_light(m2->data))
	    new_light_source(m2->mx, m2->my, emits_light(m2->data),
			     LS_MONSTER, (genericptr_t)m2);
	if (m2->mnamelth) {
	    m2->mnamelth = 0; /* or it won't get allocated */
	    m2 = christen_monst(m2, NAME(mon));
	}
	newsym(m2->mx,m2->my);	/* display the new monster */
	if (mon->mtame) {
	    struct monst *m3;

	    if (mon->isminion) {
		m3 = newmonst(sizeof(struct epri) + mon->mnamelth);
		*m3 = *m2;
		m3->mxlth = sizeof(struct epri);
		if (m2->mnamelth) Strcpy(NAME(m3), NAME(m2));
		*(EPRI(m3)) = *(EPRI(mon));
		replmon(m2, m3);
		m2 = m3;
	    } else {
		/* because m2 is a copy of mon it is tame but not init'ed.
		 * however, tamedog will not re-tame a tame dog, so m2
		 * must be made non-tame to get initialized properly.
		 */
		m2->mtame = 0;
		if ((m3 = tamedog(m2, (struct obj *)0)) != 0) {
		    m2 = m3;
		    *(EDOG(m2)) = *(EDOG(mon));
		}
	    }
	}
	return m2;
}

/*
 * called with [x,y] = coordinates;
 *	[0,0] means anyplace
 *	[u.ux,u.uy] means: near player (if !in_mklev)
 *
 *	In case we make a monster group, only return the one at [x,y].
 */
struct monst *
makemon(ptr, x, y, mmflags)
register struct permonst *ptr;
register int	x, y;
register int	mmflags;
{
	register struct monst *mtmp;
	int mndx, mcham, ct, mitem, xlth;
	boolean anymon = (!ptr);
	boolean byyou = (x == u.ux && y == u.uy);
	boolean allow_minvent = ((mmflags & NO_MINVENT) == 0);
	boolean countbirth = ((mmflags & MM_NOCOUNTBIRTH) == 0);
	unsigned gpflags = (mmflags & MM_IGNOREWATER) ? MM_IGNOREWATER : 0;
	uchar lim;

	/* if caller wants random location, do it here */
	if(x == 0 && y == 0) {
		int tryct = 0;	/* careful with bigrooms */
		struct monst fakemon;

		fakemon.data = ptr;	/* set up for goodpos */
		do {
			x = rn1(COLNO-3,2);
			y = rn2(ROWNO);
		} while(!goodpos(x, y, ptr ? &fakemon : (struct monst *)0, gpflags) ||
			(!in_mklev && tryct++ < 50 && cansee(x, y)));
	} else if (byyou && !in_mklev) {
		coord bypos;

		if(enexto_core(&bypos, u.ux, u.uy, ptr, gpflags)) {
			x = bypos.x;
			y = bypos.y;
		} else
			return((struct monst *)0);
	}

	/* if a monster already exists at the position, return */
	if(MON_AT(x, y))
		return((struct monst *) 0);

	if(ptr){
		mndx = monsndx(ptr);
		/* if you are to make a specific monster and it has
		   already been genocided, return */
		if (mvitals[mndx].mvflags & G_GENOD) return((struct monst *) 0);
#ifdef DEBUG
		if (wizard && (mvitals[mndx].mvflags & G_EXTINCT))
		    pline("Explicitly creating extinct monster %s.",
			mons[mndx].mname);
#endif
	} else {
		/* make a random (common) monster that can survive here.
		 * (the special levels ask for random monsters at specific
		 * positions, causing mass drowning on the medusa level,
		 * for instance.)
		 */
		int tryct = 0;	/* maybe there are no good choices */
		struct monst fakemon;
		do {
			if(!(ptr = rndmonst())) {
#ifdef DEBUG
			    pline("Warning: no monster.");
#endif
			    return((struct monst *) 0);	/* no more monsters! */
			}
			fakemon.data = ptr;	/* set up for goodpos */
		} while(!goodpos(x, y, &fakemon, gpflags) && tryct++ < 50);
		mndx = monsndx(ptr);
	}
	/* if it's unique, don't ever make it again */
	if (ptr->geno & G_UNIQ) mvitals[mndx].mvflags |= G_EXTINCT;

	/* Once a certain number of monsters are created, don't create any more
	 * at random (i.e. make them extinct).  The previous (3.2) behavior was
	 * to do this when a certain number had _died_, which didn't make
	 * much sense.
	 * This version makes a little more sense but still requires that
	 * the caller manually decrement mvitals if the monster is created
	 * under circumstances where one would not logically expect the
	 * creation to reduce the supply of wild monsters.  Monster cloning
 	 * might be one case that requires that in order to reduce the
	 * possibility of abuse, but currently doesn't.
	 */
	if (mvitals[mndx].born < 255 && countbirth) mvitals[mndx].born++;
	lim = mbirth_limit(mndx);
	if ((int) mvitals[mndx].born >= lim && !(mons[mndx].geno & G_NOGEN) &&
		!(mvitals[mndx].mvflags & G_EXTINCT)) {
#ifdef DEBUG
		pline("Automatically extinguished %s.",
					makeplural(mons[mndx].mname));
#endif
		mvitals[mndx].mvflags |= G_EXTINCT;
		reset_rndmonst(mndx);
	}

	xlth = ptr->pxlth;
	if (mmflags & MM_EDOG) xlth += sizeof(struct edog);
	else if (mmflags & MM_EMIN) xlth += sizeof(struct emin);
	mtmp = newmonst(xlth);
	*mtmp = zeromonst;		/* clear all entries in structure */
	(void)memset((genericptr_t)mtmp->mextra, 0, xlth);
	mtmp->nmon = fmon;
	fmon = mtmp;
	mtmp->m_id = flags.ident++;
	if (!mtmp->m_id) mtmp->m_id = flags.ident++;	/* ident overflowed */
	set_mon_data(mtmp, ptr, 0);
	if (mtmp->data->msound == MS_LEADER)
	    quest_status.leader_m_id = mtmp->m_id;
	mtmp->mxlth = xlth;
	mtmp->mnum = mndx;

	mtmp->m_lev = adj_lev(ptr);
	if (is_golem(ptr)) {
	    mtmp->mhpmax = mtmp->mhp = golemhp(mndx);
	} else if (is_rider(ptr)) {
	    /* We want low HP, but a high mlevel so they can attack well */
	    mtmp->mhpmax = mtmp->mhp = d(10,8);
	} else if (ptr->mlevel > 49) {
	    /* "special" fixed hp monster
	     * the hit points are encoded in the mlevel in a somewhat strange
	     * way to fit in the 50..127 positive range of a signed character
	     * above the 1..49 that indicate "normal" monster levels */
	    mtmp->mhpmax = mtmp->mhp = 2*(ptr->mlevel - 6);
	    mtmp->m_lev = mtmp->mhp / 4;	/* approximation */
	} else if (ptr->mlet == S_DRAGON && mndx >= PM_GRAY_DRAGON) {
	    /* adult dragons */
	    mtmp->mhpmax = mtmp->mhp = (int) (In_endgame(&u.uz) ?
		(8 * mtmp->m_lev) : (4 * mtmp->m_lev + d((int)mtmp->m_lev, 4)));
	} else if (!mtmp->m_lev) {
	    mtmp->mhpmax = mtmp->mhp = rnd(4);
	} else {
	    mtmp->mhpmax = mtmp->mhp = d((int)mtmp->m_lev, 8);
	    if (is_home_elemental(ptr))
		mtmp->mhpmax = (mtmp->mhp *= 3);
	}

	if (is_female(ptr)) mtmp->female = TRUE;
	else if (is_male(ptr)) mtmp->female = FALSE;
	else mtmp->female = rn2(2);	/* ignored for neuters */

	if (In_sokoban(&u.uz) && !mindless(ptr))  /* know about traps here */
	    mtmp->mtrapseen = (1L << (PIT - 1)) | (1L << (HOLE - 1));
	if (ptr->msound == MS_LEADER)		/* leader knows about portal */
	    mtmp->mtrapseen |= (1L << (MAGIC_PORTAL-1));

	place_monster(mtmp, x, y);
	mtmp->mcansee = mtmp->mcanmove = TRUE;
	mtmp->mpeaceful = (mmflags & MM_ANGRY) ? FALSE : peace_minded(ptr);

	switch(ptr->mlet) {
		case S_MIMIC:
			set_mimic_sym(mtmp);
			break;
		case S_SPIDER:
		case S_SNAKE:
			if(in_mklev)
			    if(x && y)
				(void) mkobj_at(0, x, y, TRUE);
			if(hides_under(ptr) && OBJ_AT(x, y))
			    mtmp->mundetected = TRUE;
			break;
		case S_LIGHT:
		case S_ELEMENTAL:
			if (mndx == PM_STALKER || mndx == PM_BLACK_LIGHT) {
			    mtmp->perminvis = TRUE;
			    mtmp->minvis = TRUE;
			}
			break;
		case S_EEL:
			if (is_pool(x, y))
			    mtmp->mundetected = TRUE;
			break;
		case S_LEPRECHAUN:
			mtmp->msleeping = 1;
			break;
		case S_JABBERWOCK:
		case S_NYMPH:
			if (rn2(5) && !u.uhave.amulet) mtmp->msleeping = 1;
			break;
		case S_ORC:
			if (Race_if(PM_ELF)) mtmp->mpeaceful = FALSE;
			break;
		case S_UNICORN:
			if (is_unicorn(ptr) &&
					sgn(u.ualign.type) == sgn(ptr->maligntyp))
				mtmp->mpeaceful = TRUE;
			break;
		case S_BAT:
			if (Inhell && is_bat(ptr))
			    mon_adjust_speed(mtmp, 2, (struct obj *)0);
			break;
	}
	if ((ct = emits_light(mtmp->data)) > 0)
		new_light_source(mtmp->mx, mtmp->my, ct,
				 LS_MONSTER, (genericptr_t)mtmp);
	mitem = 0;	/* extra inventory item for this monster */

	if ((mcham = pm_to_cham(mndx)) != CHAM_ORDINARY) {
		/* If you're protected with a ring, don't create
		 * any shape-changing chameleons -dgk
		 */
		if (Protection_from_shape_changers)
			mtmp->cham = CHAM_ORDINARY;
		else {
			mtmp->cham = mcham;
			(void) newcham(mtmp, rndmonst(), FALSE, FALSE);
		}
	} else if (mndx == PM_WIZARD_OF_YENDOR) {
		mtmp->iswiz = TRUE;
		flags.no_of_wizards++;
		if (flags.no_of_wizards == 1 && Is_earthlevel(&u.uz))
			mitem = SPE_DIG;
	} else if (mndx == PM_DJINNI) {
		flags.djinni_count++;
	} else if (mndx == PM_GHOST) {
		flags.ghost_count++;
		if (!(mmflags & MM_NONAME))
			mtmp = christen_monst(mtmp, rndghostname());
	} else if (mndx == PM_VLAD_THE_IMPALER) {
		mitem = CANDELABRUM_OF_INVOCATION;
	} else if (mndx == PM_CROESUS) {
		mitem = TWO_HANDED_SWORD;
	} else if (ptr->msound == MS_NEMESIS) {
		mitem = BELL_OF_OPENING;
	} else if (mndx == PM_PESTILENCE) {
		mitem = POT_SICKNESS;
	}
	if (mitem && allow_minvent) (void) mongets(mtmp, mitem);

	if(in_mklev) {
		if(((is_ndemon(ptr)) ||
		    (mndx == PM_WUMPUS) ||
		    (mndx == PM_LONG_WORM) ||
		    (mndx == PM_GIANT_EEL)) && !u.uhave.amulet && rn2(5))
			mtmp->msleeping = TRUE;
	} else {
		if(byyou) {
			newsym(mtmp->mx,mtmp->my);
			set_apparxy(mtmp);
		}
	}
	if(is_dprince(ptr) && ptr->msound == MS_BRIBE) {
	    mtmp->mpeaceful = mtmp->minvis = mtmp->perminvis = 1;
	    mtmp->mavenge = 0;
	    if (uwep && uwep->oartifact == ART_EXCALIBUR)
		mtmp->mpeaceful = mtmp->mtame = FALSE;
	}
#ifndef DCC30_BUG
	if (mndx == PM_LONG_WORM && (mtmp->wormno = get_wormno()) != 0)
#else
	/* DICE 3.0 doesn't like assigning and comparing mtmp->wormno in the
	 * same expression.
	 */
	if (mndx == PM_LONG_WORM &&
		(mtmp->wormno = get_wormno(), mtmp->wormno != 0))
#endif
	{
	    /* we can now create worms with tails - 11/91 */
	    initworm(mtmp, rn2(5));
	    if (count_wsegs(mtmp)) place_worm_tail_randomly(mtmp, x, y);
	}
	set_malign(mtmp);		/* having finished peaceful changes */
	if(anymon) {
	    if ((ptr->geno & G_SGROUP) && rn2(2)) {
		m_initsgrp(mtmp, mtmp->mx, mtmp->my);
	    } else if (ptr->geno & G_LGROUP) {
		if(rn2(3))  m_initlgrp(mtmp, mtmp->mx, mtmp->my);
		else	    m_initsgrp(mtmp, mtmp->mx, mtmp->my);
	    }
	}

	if (allow_minvent) {
	    if(is_armed(ptr))
		m_initweap(mtmp);	/* equip with weapons / armor */
	    m_initinv(mtmp);  /* add on a few special items incl. more armor */
	    m_dowear(mtmp, TRUE);
	} else {
	    /* no initial inventory is allowed */
	    if (mtmp->minvent) discard_minvent(mtmp);
	    mtmp->minvent = (struct obj *)0;    /* caller expects this */
	}
	if ((ptr->mflags3 & M3_WAITMASK) && !(mmflags & MM_NOWAIT)) {
		if (ptr->mflags3 & M3_WAITFORU)
			mtmp->mstrategy |= STRAT_WAITFORU;
		if (ptr->mflags3 & M3_CLOSE)
			mtmp->mstrategy |= STRAT_CLOSE;
	}

	if (!in_mklev)
	    newsym(mtmp->mx,mtmp->my);	/* make sure the mon shows up */

	return(mtmp);
}

int
mbirth_limit(mndx)
int mndx;
{
	return (mndx == PM_NAZGUL ? 9 : mndx == PM_ERINYS ? 3 : MAXMONNO); 
}

/* used for wand/scroll/spell of create monster */
/* returns TRUE iff you know monsters have been created */
boolean
create_critters(cnt, mptr)
int cnt;
struct permonst *mptr;		/* usually null; used for confused reading */
{
	coord c;
	int x, y;
	struct monst *mon;
	boolean known = FALSE;
#ifdef WIZARD
	boolean ask = wizard;
#endif

	while (cnt--) {
#ifdef WIZARD
	    if (ask) {
		if (create_particular()) {
		    known = TRUE;
		    continue;
		}
		else ask = FALSE;	/* ESC will shut off prompting */
	    }
#endif
	    x = u.ux,  y = u.uy;
	    /* if in water, try to encourage an aquatic monster
	       by finding and then specifying another wet location */
	    if (!mptr && u.uinwater && enexto(&c, x, y, &mons[PM_GIANT_EEL]))
		x = c.x,  y = c.y;

	    mon = makemon(mptr, x, y, NO_MM_FLAGS);
	    if (mon && canspotmon(mon)) known = TRUE;
	}
	return known;
}

#endif /* OVL1 */
#ifdef OVL0

STATIC_OVL boolean
uncommon(mndx)
int mndx;
{
	if (mons[mndx].geno & (G_NOGEN | G_UNIQ)) return TRUE;
	if (mvitals[mndx].mvflags & G_GONE) return TRUE;
	if (Inhell)
		return(mons[mndx].maligntyp > A_NEUTRAL);
	else
		return((mons[mndx].geno & G_HELL) != 0);
}

/*
 *	shift the probability of a monster's generation by
 *	comparing the dungeon alignment and monster alignment.
 *	return an integer in the range of 0-5.
 */
STATIC_OVL int
align_shift(ptr)
register struct permonst *ptr;
{
    static NEARDATA long oldmoves = 0L;	/* != 1, starting value of moves */
    static NEARDATA s_level *lev;
    register int alshift;

    if(oldmoves != moves) {
	lev = Is_special(&u.uz);
	oldmoves = moves;
    }
    switch((lev) ? lev->flags.align : dungeons[u.uz.dnum].flags.align) {
    default:	/* just in case */
    case AM_NONE:	alshift = 0;
			break;
    case AM_LAWFUL:	alshift = (ptr->maligntyp+20)/(2*ALIGNWEIGHT);
			break;
    case AM_NEUTRAL:	alshift = (20 - abs(ptr->maligntyp))/ALIGNWEIGHT;
			break;
    case AM_CHAOTIC:	alshift = (-(ptr->maligntyp-20))/(2*ALIGNWEIGHT);
			break;
    }
    return alshift;
}

static NEARDATA struct {
	int choice_count;
	char mchoices[SPECIAL_PM];	/* value range is 0..127 */
} rndmonst_state = { -1, {0} };

/* select a random monster type */
struct permonst *
rndmonst()
{
	register struct permonst *ptr;
	register int mndx, ct;

	if (u.uz.dnum == quest_dnum && rn2(7) && (ptr = qt_montype()) != 0)
	    return ptr;

	if (rndmonst_state.choice_count < 0) {	/* need to recalculate */
	    int zlevel, minmlev, maxmlev;
	    boolean elemlevel;
#ifdef REINCARNATION
	    boolean upper;
#endif

	    rndmonst_state.choice_count = 0;
	    /* look for first common monster */
	    for (mndx = LOW_PM; mndx < SPECIAL_PM; mndx++)
		if (!uncommon(mndx)) break;
	    if (mndx == SPECIAL_PM) {
		/* evidently they've all been exterminated */
#ifdef DEBUG
		pline("rndmonst: no common mons!");
#endif
		return (struct permonst *)0;
	    } /* else `mndx' now ready for use below */
	    zlevel = level_difficulty();
	    /* determine the level of the weakest monster to make. */
	    minmlev = zlevel / 6;
	    /* determine the level of the strongest monster to make. */
	    maxmlev = (zlevel + u.ulevel) / 2;
#ifdef REINCARNATION
	    upper = Is_rogue_level(&u.uz);
#endif
	    elemlevel = In_endgame(&u.uz) && !Is_astralevel(&u.uz);

/*
 *	Find out how many monsters exist in the range we have selected.
 */
	    /* (`mndx' initialized above) */
	    for ( ; mndx < SPECIAL_PM; mndx++) {
		ptr = &mons[mndx];
		rndmonst_state.mchoices[mndx] = 0;
		if (tooweak(mndx, minmlev) || toostrong(mndx, maxmlev))
		    continue;
#ifdef REINCARNATION
		if (upper && !isupper(def_monsyms[(int)(ptr->mlet)])) continue;
#endif
		if (elemlevel && wrong_elem_type(ptr)) continue;
		if (uncommon(mndx)) continue;
		if (Inhell && (ptr->geno & G_NOHELL)) continue;
		ct = (int)(ptr->geno & G_FREQ) + align_shift(ptr);
		if (ct < 0 || ct > 127)
		    panic("rndmonst: bad count [#%d: %d]", mndx, ct);
		rndmonst_state.choice_count += ct;
		rndmonst_state.mchoices[mndx] = (char)ct;
	    }
/*
 *	    Possible modification:  if choice_count is "too low",
 *	    expand minmlev..maxmlev range and try again.
 */
	} /* choice_count+mchoices[] recalc */

	if (rndmonst_state.choice_count <= 0) {
	    /* maybe no common mons left, or all are too weak or too strong */
#ifdef DEBUG
	    Norep("rndmonst: choice_count=%d", rndmonst_state.choice_count);
#endif
	    return (struct permonst *)0;
	}

/*
 *	Now, select a monster at random.
 */
	ct = rnd(rndmonst_state.choice_count);
	for (mndx = LOW_PM; mndx < SPECIAL_PM; mndx++)
	    if ((ct -= (int)rndmonst_state.mchoices[mndx]) <= 0) break;

	if (mndx == SPECIAL_PM || uncommon(mndx)) {	/* shouldn't happen */
	    impossible("rndmonst: bad `mndx' [#%d]", mndx);
	    return (struct permonst *)0;
	}
	return &mons[mndx];
}

/* called when you change level (experience or dungeon depth) or when
   monster species can no longer be created (genocide or extinction) */
void
reset_rndmonst(mndx)
int mndx;	/* particular species that can no longer be created */
{
	/* cached selection info is out of date */
	if (mndx == NON_PM) {
	    rndmonst_state.choice_count = -1;	/* full recalc needed */
	} else if (mndx < SPECIAL_PM) {
	    rndmonst_state.choice_count -= rndmonst_state.mchoices[mndx];
	    rndmonst_state.mchoices[mndx] = 0;
	} /* note: safe to ignore extinction of unique monsters */
}

#endif /* OVL0 */
#ifdef OVL1

/*	The routine below is used to make one of the multiple types
 *	of a given monster class.  The second parameter specifies a
 *	special casing bit mask to allow the normal genesis
 *	masks to be deactivated.  Returns 0 if no monsters
 *	in that class can be made.
 */

struct permonst *
mkclass(class,spc)
char	class;
int	spc;
{
	register int	first, last, num = 0;
	int maxmlev, mask = (G_NOGEN | G_UNIQ) & ~spc;

	maxmlev = level_difficulty() >> 1;
	if(class < 1 || class >= MAXMCLASSES) {
	    impossible("mkclass called with bad class!");
	    return((struct permonst *) 0);
	}
/*	Assumption #1:	monsters of a given class are contiguous in the
 *			mons[] array.
 */
	for (first = LOW_PM; first < SPECIAL_PM; first++)
	    if (mons[first].mlet == class) break;
	if (first == SPECIAL_PM) return (struct permonst *) 0;

	for (last = first;
		last < SPECIAL_PM && mons[last].mlet == class; last++)
	    if (!(mvitals[last].mvflags & G_GONE) && !(mons[last].geno & mask)
					&& !is_placeholder(&mons[last])) {
		/* consider it */
		if(num && toostrong(last, maxmlev) &&
		   monstr[last] != monstr[last-1] && rn2(2)) break;
		num += mons[last].geno & G_FREQ;
	    }

	if(!num) return((struct permonst *) 0);

/*	Assumption #2:	monsters of a given class are presented in ascending
 *			order of strength.
 */
	for(num = rnd(num); num > 0; first++)
	    if (!(mvitals[first].mvflags & G_GONE) && !(mons[first].geno & mask)
					&& !is_placeholder(&mons[first])) {
		/* skew towards lower value monsters at lower exp. levels */
		num -= mons[first].geno & G_FREQ;
		if (num && adj_lev(&mons[first]) > (u.ulevel*2)) {
		    /* but not when multiple monsters are same level */
		    if (mons[first].mlevel != mons[first+1].mlevel)
			num--;
		}
	    }
	first--; /* correct an off-by-one error */

	return(&mons[first]);
}

int
adj_lev(ptr)	/* adjust strength of monsters based on u.uz and u.ulevel */
register struct permonst *ptr;
{
	int	tmp, tmp2;

	if (ptr == &mons[PM_WIZARD_OF_YENDOR]) {
		/* does not depend on other strengths, but does get stronger
		 * every time he is killed
		 */
		tmp = ptr->mlevel + mvitals[PM_WIZARD_OF_YENDOR].died;
		if (tmp > 49) tmp = 49;
		return tmp;
	}

	if((tmp = ptr->mlevel) > 49) return(50); /* "special" demons/devils */
	tmp2 = (level_difficulty() - tmp);
	if(tmp2 < 0) tmp--;		/* if mlevel > u.uz decrement tmp */
	else tmp += (tmp2 / 5);		/* else increment 1 per five diff */

	tmp2 = (u.ulevel - ptr->mlevel);	/* adjust vs. the player */
	if(tmp2 > 0) tmp += (tmp2 / 4);		/* level as well */

	tmp2 = (3 * ((int) ptr->mlevel))/ 2;	/* crude upper limit */
	if (tmp2 > 49) tmp2 = 49;		/* hard upper limit */
	return((tmp > tmp2) ? tmp2 : (tmp > 0 ? tmp : 0)); /* 0 lower limit */
}

#endif /* OVL1 */
#ifdef OVLB

struct permonst *
grow_up(mtmp, victim)	/* `mtmp' might "grow up" into a bigger version */
struct monst *mtmp, *victim;
{
	int oldtype, newtype, max_increase, cur_increase,
	    lev_limit, hp_threshold;
	struct permonst *ptr = mtmp->data;

	/* monster died after killing enemy but before calling this function */
	/* currently possible if killing a gas spore */
	if (mtmp->mhp <= 0)
	    return ((struct permonst *)0);

	/* note:  none of the monsters with special hit point calculations
	   have both little and big forms */
	oldtype = monsndx(ptr);
	newtype = little_to_big(oldtype);
	if (newtype == PM_PRIEST && mtmp->female) newtype = PM_PRIESTESS;

	/* growth limits differ depending on method of advancement */
	if (victim) {		/* killed a monster */
	    /*
	     * The HP threshold is the maximum number of hit points for the
	     * current level; once exceeded, a level will be gained.
	     * Possible bug: if somehow the hit points are already higher
	     * than that, monster will gain a level without any increase in HP.
	     */
	    hp_threshold = mtmp->m_lev * 8;		/* normal limit */
	    if (!mtmp->m_lev)
		hp_threshold = 4;
	    else if (is_golem(ptr))	/* strange creatures */
		hp_threshold = ((mtmp->mhpmax / 10) + 1) * 10 - 1;
	    else if (is_home_elemental(ptr))
		hp_threshold *= 3;
	    lev_limit = 3 * (int)ptr->mlevel / 2;	/* same as adj_lev() */
	    /* If they can grow up, be sure the level is high enough for that */
	    if (oldtype != newtype && mons[newtype].mlevel > lev_limit)
		lev_limit = (int)mons[newtype].mlevel;
	    /* number of hit points to gain; unlike for the player, we put
	       the limit at the bottom of the next level rather than the top */
	    max_increase = rnd((int)victim->m_lev + 1);
	    if (mtmp->mhpmax + max_increase > hp_threshold + 1)
		max_increase = max((hp_threshold + 1) - mtmp->mhpmax, 0);
	    cur_increase = (max_increase > 1) ? rn2(max_increase) : 0;
	} else {
	    /* a gain level potion or wraith corpse; always go up a level
	       unless already at maximum (49 is hard upper limit except
	       for demon lords, who start at 50 and can't go any higher) */
	    max_increase = cur_increase = rnd(8);
	    hp_threshold = 0;	/* smaller than `mhpmax + max_increase' */
	    lev_limit = 50;		/* recalc below */
	}

	mtmp->mhpmax += max_increase;
	mtmp->mhp += cur_increase;
	if (mtmp->mhpmax <= hp_threshold)
	    return ptr;		/* doesn't gain a level */

	if (is_mplayer(ptr)) lev_limit = 30;	/* same as player */
	else if (lev_limit < 5) lev_limit = 5;	/* arbitrary */
	else if (lev_limit > 49) lev_limit = (ptr->mlevel > 49 ? 50 : 49);

	if ((int)++mtmp->m_lev >= mons[newtype].mlevel && newtype != oldtype) {
	    ptr = &mons[newtype];
	    if (mvitals[newtype].mvflags & G_GENOD) {	/* allow G_EXTINCT */
		if (sensemon(mtmp))
		    pline("As %s grows up into %s, %s %s!", mon_nam(mtmp),
			an(ptr->mname), mhe(mtmp),
			nonliving(ptr) ? "expires" : "dies");
		set_mon_data(mtmp, ptr, -1);	/* keep mvitals[] accurate */
		mondied(mtmp);
		return (struct permonst *)0;
	    }
	    set_mon_data(mtmp, ptr, 1);		/* preserve intrinsics */
	    newsym(mtmp->mx, mtmp->my);		/* color may change */
	    lev_limit = (int)mtmp->m_lev;	/* never undo increment */
	}
	/* sanity checks */
	if ((int)mtmp->m_lev > lev_limit) {
	    mtmp->m_lev--;	/* undo increment */
	    /* HP might have been allowed to grow when it shouldn't */
	    if (mtmp->mhpmax == hp_threshold + 1) mtmp->mhpmax--;
	}
	if (mtmp->mhpmax > 50*8) mtmp->mhpmax = 50*8;	  /* absolute limit */
	if (mtmp->mhp > mtmp->mhpmax) mtmp->mhp = mtmp->mhpmax;

	return ptr;
}

#endif /* OVLB */
#ifdef OVL1

int
mongets(mtmp, otyp)
register struct monst *mtmp;
register int otyp;
{
	register struct obj *otmp;
	int spe;

	if (!otyp) return 0;
	otmp = mksobj(otyp, TRUE, FALSE);
	if (otmp) {
	    if (mtmp->data->mlet == S_DEMON) {
		/* demons never get blessed objects */
		if (otmp->blessed) curse(otmp);
	    } else if(is_lminion(mtmp)) {
		/* lawful minions don't get cursed, bad, or rusting objects */
		otmp->cursed = FALSE;
		if(otmp->spe < 0) otmp->spe = 0;
		otmp->oerodeproof = TRUE;
	    } else if(is_mplayer(mtmp->data) && is_sword(otmp)) {
		otmp->spe = (3 + rn2(4));
	    }

	    if(otmp->otyp == CANDELABRUM_OF_INVOCATION) {
		otmp->spe = 0;
		otmp->age = 0L;
		otmp->lamplit = FALSE;
		otmp->blessed = otmp->cursed = FALSE;
	    } else if (otmp->otyp == BELL_OF_OPENING) {
		otmp->blessed = otmp->cursed = FALSE;
	    } else if (otmp->otyp == SPE_BOOK_OF_THE_DEAD) {
		otmp->blessed = FALSE;
		otmp->cursed = TRUE;
	    }

	    /* leaders don't tolerate inferior quality battle gear */
	    if (is_prince(mtmp->data)) {
		if (otmp->oclass == WEAPON_CLASS && otmp->spe < 1)
		    otmp->spe = 1;
		else if (otmp->oclass == ARMOR_CLASS && otmp->spe < 0)
		    otmp->spe = 0;
	    }

	    spe = otmp->spe;
	    (void) mpickobj(mtmp, otmp);	/* might free otmp */
	    return(spe);
	} else return(0);
}

#endif /* OVL1 */
#ifdef OVLB

int
golemhp(type)
int type;
{
	switch(type) {
		case PM_STRAW_GOLEM: return 20;
		case PM_PAPER_GOLEM: return 20;
		case PM_ROPE_GOLEM: return 30;
		case PM_LEATHER_GOLEM: return 40;
		case PM_GOLD_GOLEM: return 40;
		case PM_WOOD_GOLEM: return 50;
		case PM_FLESH_GOLEM: return 40;
		case PM_CLAY_GOLEM: return 50;
		case PM_STONE_GOLEM: return 60;
		case PM_GLASS_GOLEM: return 60;
		case PM_IRON_GOLEM: return 80;
		default: return 0;
	}
}

#endif /* OVLB */
#ifdef OVL1

/*
 *	Alignment vs. yours determines monster's attitude to you.
 *	( some "animal" types are co-aligned, but also hungry )
 */
boolean
peace_minded(ptr)
register struct permonst *ptr;
{
	aligntyp mal = ptr->maligntyp, ual = u.ualign.type;

	if (always_peaceful(ptr)) return TRUE;
	if (always_hostile(ptr)) return FALSE;
	if (ptr->msound == MS_LEADER || ptr->msound == MS_GUARDIAN)
		return TRUE;
	if (ptr->msound == MS_NEMESIS)	return FALSE;

	if (race_peaceful(ptr)) return TRUE;
	if (race_hostile(ptr)) return FALSE;

	/* the monster is hostile if its alignment is different from the
	 * player's */
	if (sgn(mal) != sgn(ual)) return FALSE;

	/* Negative monster hostile to player with Amulet. */
	if (mal < A_NEUTRAL && u.uhave.amulet) return FALSE;

	/* minions are hostile to players that have strayed at all */
	if (is_minion(ptr)) return((boolean)(u.ualign.record >= 0));

	/* Last case:  a chance of a co-aligned monster being
	 * hostile.  This chance is greater if the player has strayed
	 * (u.ualign.record negative) or the monster is not strongly aligned.
	 */
	return((boolean)(!!rn2(16 + (u.ualign.record < -15 ? -15 : u.ualign.record)) &&
		!!rn2(2 + abs(mal))));
}

/* Set malign to have the proper effect on player alignment if monster is
 * killed.  Negative numbers mean it's bad to kill this monster; positive
 * numbers mean it's good.  Since there are more hostile monsters than
 * peaceful monsters, the penalty for killing a peaceful monster should be
 * greater than the bonus for killing a hostile monster to maintain balance.
 * Rules:
 *   it's bad to kill peaceful monsters, potentially worse to kill always-
 *	peaceful monsters
 *   it's never bad to kill a hostile monster, although it may not be good
 */
void
set_malign(mtmp)
struct monst *mtmp;
{
	schar mal = mtmp->data->maligntyp;
	boolean coaligned;

	if (mtmp->ispriest || mtmp->isminion) {
		/* some monsters have individual alignments; check them */
		if (mtmp->ispriest)
			mal = EPRI(mtmp)->shralign;
		else if (mtmp->isminion)
			mal = EMIN(mtmp)->min_align;
		/* unless alignment is none, set mal to -5,0,5 */
		/* (see align.h for valid aligntyp values)     */
		if(mal != A_NONE)
			mal *= 5;
	}

	coaligned = (sgn(mal) == sgn(u.ualign.type));
	if (mtmp->data->msound == MS_LEADER) {
		mtmp->malign = -20;
	} else if (mal == A_NONE) {
		if (mtmp->mpeaceful)
			mtmp->malign = 0;
		else
			mtmp->malign = 20;	/* really hostile */
	} else if (always_peaceful(mtmp->data)) {
		int absmal = abs(mal);
		if (mtmp->mpeaceful)
			mtmp->malign = -3*max(5,absmal);
		else
			mtmp->malign = 3*max(5,absmal); /* renegade */
	} else if (always_hostile(mtmp->data)) {
		int absmal = abs(mal);
		if (coaligned)
			mtmp->malign = 0;
		else
			mtmp->malign = max(5,absmal);
	} else if (coaligned) {
		int absmal = abs(mal);
		if (mtmp->mpeaceful)
			mtmp->malign = -3*max(3,absmal);
		else	/* renegade */
			mtmp->malign = max(3,absmal);
	} else	/* not coaligned and therefore hostile */
		mtmp->malign = abs(mal);
}

#endif /* OVL1 */
#ifdef OVLB

static NEARDATA char syms[] = {
	MAXOCLASSES, MAXOCLASSES+1, RING_CLASS, WAND_CLASS, WEAPON_CLASS,
	FOOD_CLASS, COIN_CLASS, SCROLL_CLASS, POTION_CLASS, ARMOR_CLASS,
	AMULET_CLASS, TOOL_CLASS, ROCK_CLASS, GEM_CLASS, SPBOOK_CLASS,
	S_MIMIC_DEF, S_MIMIC_DEF, S_MIMIC_DEF,
};

void
set_mimic_sym(mtmp)		/* KAA, modified by ERS */
register struct monst *mtmp;
{
	int typ, roomno, rt;
	unsigned appear, ap_type;
	int s_sym;
	struct obj *otmp;
	int mx, my;

	if (!mtmp) return;
	mx = mtmp->mx; my = mtmp->my;
	typ = levl[mx][my].typ;
					/* only valid for INSIDE of room */
	roomno = levl[mx][my].roomno - ROOMOFFSET;
	if (roomno >= 0)
		rt = rooms[roomno].rtype;
#ifdef SPECIALIZATION
	else if (IS_ROOM(typ))
		rt = OROOM,  roomno = 0;
#endif
	else	rt = 0;	/* roomno < 0 case for GCC_WARN */

	if (OBJ_AT(mx, my)) {
		ap_type = M_AP_OBJECT;
		appear = level.objects[mx][my]->otyp;
	} else if (IS_DOOR(typ) || IS_WALL(typ) ||
		   typ == SDOOR || typ == SCORR) {
		ap_type = M_AP_FURNITURE;
		/*
		 *  If there is a wall to the left that connects to this
		 *  location, then the mimic mimics a horizontal closed door.
		 *  This does not allow doors to be in corners of rooms.
		 */
		if (mx != 0 &&
			(levl[mx-1][my].typ == HWALL    ||
			 levl[mx-1][my].typ == TLCORNER ||
			 levl[mx-1][my].typ == TRWALL   ||
			 levl[mx-1][my].typ == BLCORNER ||
			 levl[mx-1][my].typ == TDWALL   ||
			 levl[mx-1][my].typ == CROSSWALL||
			 levl[mx-1][my].typ == TUWALL    ))
		    appear = S_hcdoor;
		else
		    appear = S_vcdoor;

		if(!mtmp->minvis || See_invisible)
		    block_point(mx,my);	/* vision */
	} else if (level.flags.is_maze_lev && rn2(2)) {
		ap_type = M_AP_OBJECT;
		appear = STATUE;
	} else if (roomno < 0) {
		ap_type = M_AP_OBJECT;
		appear = BOULDER;
		if(!mtmp->minvis || See_invisible)
		    block_point(mx,my);	/* vision */
	} else if (rt == ZOO || rt == VAULT) {
		ap_type = M_AP_OBJECT;
		appear = GOLD_PIECE;
	} else if (rt == DELPHI) {
		if (rn2(2)) {
			ap_type = M_AP_OBJECT;
			appear = STATUE;
		} else {
			ap_type = M_AP_FURNITURE;
			appear = S_fountain;
		}
	} else if (rt == TEMPLE) {
		ap_type = M_AP_FURNITURE;
		appear = S_altar;
	/*
	 * We won't bother with beehives, morgues, barracks, throne rooms
	 * since they shouldn't contain too many mimics anyway...
	 */
	} else if (rt >= SHOPBASE) {
		s_sym = get_shop_item(rt - SHOPBASE);
		if (s_sym < 0) {
			ap_type = M_AP_OBJECT;
			appear = -s_sym;
		} else {
			if (s_sym == RANDOM_CLASS)
				s_sym = syms[rn2((int)sizeof(syms)-2) + 2];
			goto assign_sym;
		}
	} else {
		s_sym = syms[rn2((int)sizeof(syms))];
assign_sym:
		if (s_sym >= MAXOCLASSES) {
			ap_type = M_AP_FURNITURE;
			appear = s_sym == MAXOCLASSES ? S_upstair : S_dnstair;
		} else if (s_sym == COIN_CLASS) {
			ap_type = M_AP_OBJECT;
			appear = GOLD_PIECE;
		} else {
			ap_type = M_AP_OBJECT;
			if (s_sym == S_MIMIC_DEF) {
				appear = STRANGE_OBJECT;
			} else {
				otmp = mkobj( (char) s_sym, FALSE );
				appear = otmp->otyp;
				/* make sure container contents are free'ed */
				obfree(otmp, (struct obj *) 0);
			}
		}
	}
	mtmp->m_ap_type = ap_type;
	mtmp->mappearance = appear;
}

#endif /* OVLB */

/*makemon.c*/

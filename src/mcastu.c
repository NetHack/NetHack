/*	SCCS Id: @(#)mcastu.c	3.3	2002/01/10	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL void FDECL(cursetxt,(struct monst *,BOOLEAN_P));
STATIC_DCL void FDECL(cast_wizard_spell,(struct monst *, int,int));
STATIC_DCL void FDECL(cast_cleric_spell,(struct monst *, int,int));
STATIC_DCL boolean FDECL(is_undirected_spell,(int,int));
STATIC_DCL boolean FDECL(spell_would_be_useless,(struct monst *,int,int));

#ifdef OVL0

extern const char *flash_types[];	/* from zap.c */

/* feedback when frustrated monster couldn't cast a spell */
STATIC_OVL
void
cursetxt(mtmp, undirected)
struct monst *mtmp;
boolean undirected;
{
	if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
	    const char *point_msg;  /* spellcasting monsters are impolite */

	    if (undirected)
		point_msg = "all around, then curses";
	    else if ((Invis && !perceives(mtmp->data) &&
			(mtmp->mux != u.ux || mtmp->muy != u.uy)) ||
		    (youmonst.m_ap_type == M_AP_OBJECT &&
			youmonst.mappearance == STRANGE_OBJECT) ||
		    u.uundetected)
		point_msg = "and curses in your general direction";
	    else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
		point_msg = "and curses at your displaced image";
	    else
		point_msg = "at you, then curses";

	    pline("%s points %s.", Monnam(mtmp), point_msg);
	} else if ((!(moves % 4) || !rn2(4))) {
	    if (flags.soundok) Norep("You hear a mumbled curse.");
	}
}

#endif /* OVL0 */
#ifdef OVLB

/* return values:
 * 1: successful spell
 * 0: unsuccessful spell
 */
int
castmu(mtmp, mattk, thinks_it_foundyou, foundyou)
	register struct monst *mtmp;
	register struct attack *mattk;
	boolean thinks_it_foundyou;
	boolean foundyou;
{
	int	dmg, ml = mtmp->m_lev;
	int ret;

	int spellnum = 0;

	/* Three cases:
	 * -- monster is attacking you.  Cast spell normally.
         * -- monster thinks it's attacking you.  Cast spell, but spells that
	 *    are not undirected fail with cursetxt() and loss of mspec_used.
         * -- monster isn't trying to attack.  Cast spell; spells that are
	 *    are not undirected don't go off, but they don't fail--they're
	 *    just not cast.
	 * Since most spells are directed, this means that a monster that isn't
	 * attacking casts spells only a small portion of the time that an
	 * attacking monster does.
	 */
	if (mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) {
	    spellnum = rn2(ml);
	    /* not trying to attack?  don't allow directed spells */
	    if (!thinks_it_foundyou &&
		    (!is_undirected_spell(mattk->adtyp, spellnum) ||
		    spell_would_be_useless(mtmp, mattk->adtyp, spellnum))) {
		if (foundyou)
		    impossible("spellcasting monster found you and doesn't know it?");
		return 0;
	    }
	}

	/* monster unable to cast spells? */
	if(mtmp->mcan || mtmp->mspec_used || !ml) {
	    cursetxt(mtmp, is_undirected_spell(mattk->adtyp, spellnum));
	    return(0);
	}

	if (mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) {
	    mtmp->mspec_used = 10 - mtmp->m_lev;
	    if (mtmp->mspec_used < 2) mtmp->mspec_used = 2;
	}

	/* monster can cast spells, but is casting a directed spell at the
	   wrong place?  If so, give a message, and return.  Do this *after*
	   penalizing mspec_used. */
	if (!foundyou && thinks_it_foundyou &&
		!is_undirected_spell(mattk->adtyp, spellnum)) {
	    pline("%s casts a spell at %s!",
		canseemon(mtmp) ? Monnam(mtmp) : "It",
		levl[mtmp->mux][mtmp->muy].typ == WATER
		    ? "empty water" : "thin air");
	    return(0);
	}

	nomul(0);
	if(rn2(ml*10) < (mtmp->mconf ? 100 : 20)) {	/* fumbled attack */
	    if (canseemon(mtmp) && flags.soundok)
		pline_The("air crackles around %s.", mon_nam(mtmp));
	    return(0);
	}
	pline("%s casts a spell%s!", Monnam(mtmp),
	    is_undirected_spell(mattk->adtyp, spellnum) ? "" : " at you");

/*
 *	As these are spells, the damage is related to the level
 *	of the monster casting the spell.
 */
	if (!foundyou) {
	    dmg = 0;
	    if (mattk->adtyp != AD_SPEL && mattk->adtyp != AD_CLRC) {
		impossible(
	      "%s casting non-hand-to-hand version of hand-to-hand spell %d?",
			   Monnam(mtmp), mattk->adtyp);
		return(0);
	    }
	} else if (mattk->damd)
	    dmg = d((int)((ml/2) + mattk->damn), (int)mattk->damd);
	else dmg = d((int)((ml/2) + 1), 6);
	if (Half_spell_damage) dmg = (dmg+1) / 2;

	ret = 1;

	switch (mattk->adtyp) {

	    case AD_FIRE:
		pline("You're enveloped in flames.");
		if(Fire_resistance) {
			shieldeff(u.ux, u.uy);
			pline("But you resist the effects.");
			dmg = 0;
		}
		burn_away_slime();
		break;
	    case AD_COLD:
		pline("You're covered in frost.");
		if(Cold_resistance) {
			shieldeff(u.ux, u.uy);
			pline("But you resist the effects.");
			dmg = 0;
		}
		break;
	    case AD_MAGM:
		You("are hit by a shower of missiles!");
		if(Antimagic) {
			shieldeff(u.ux, u.uy);
			pline_The("missiles bounce off!");
			dmg = 0;
		} else dmg = d((int)mtmp->m_lev/2 + 1,6);
		break;
	    case AD_SPEL:	/* wizard spell */
	    case AD_CLRC:       /* clerical spell */
	    {
		if (mattk->adtyp == AD_SPEL)
		    cast_wizard_spell(mtmp, dmg, spellnum);
		else
		    cast_cleric_spell(mtmp, dmg, spellnum);
		dmg = 0; /* done by the spell casting functions */
		break;
	    }
	}
	if(dmg) mdamageu(mtmp, dmg);
	return(ret);
}

/* monster wizard and cleric spellcasting functions */
/*
   If dmg is zero, then the monster is not casting at you.
   If the monster is intentionally not casting at you, we have previously
   called spell_would_be_useless() and the fallthroughs shouldn't happen.
   If you modify either of these, be sure to change is_undirected_spell()
   and spell_would_be_useless().
 */
STATIC_OVL
void
cast_wizard_spell(mtmp, dmg, spellnum)
struct monst *mtmp;
int dmg;
int spellnum;
{
    if (dmg == 0 && !is_undirected_spell(AD_SPEL, spellnum)) {
	impossible("cast directed wizard spell with dmg=0?");
	return;
    }

    switch(spellnum) {
	case 22:
	case 21:
	case 20:
	    pline("Oh no, %s's using the touch of death!", mhe(mtmp));
	    if (nonliving(youmonst.data) || is_demon(youmonst.data))
		You("seem no deader than before.");
	    else if (!Antimagic && rn2(mtmp->m_lev) > 12) {

		if(Hallucination)
		    You("have an out of body experience.");
		else  {
		    killer_format = KILLED_BY_AN;
		    killer = "touch of death";
		    done(DIED);
		}
	    } else {
		    if(Antimagic) shieldeff(u.ux, u.uy);
		    pline("Lucky for you, it didn't work!");
	    }
	    dmg = 0;
	    break;
	case 19:
	case 18:
	    if(mtmp->iswiz && flags.no_of_wizards == 1) {
		    pline("Double Trouble...");
		    clonewiz();
		    dmg = 0;
		    break;
	    } /* else fall into the next case */
	case 17:
	case 16:
	case 15:
	    if(mtmp->iswiz)
		verbalize("Destroy the thief, my pets!");
	    else
		pline("A monster appears!");
	    nasty(mtmp);	/* summon something nasty */
	    /* fall into the next case */
	case 14:		/* aggravate all monsters */
	case 13:
	    You_feel("that monsters are aware of your presence.");
	    aggravate();
	    dmg = 0;
	    break;
	case 12:		/* curse random items */
	case 11:
	case 10:
	    You_feel("as if you need some help.");
	    rndcurse();
	    dmg = 0;
	    break;
	case 9:
	case 8:		/* destroy armor */
	    if (Antimagic) {
		    shieldeff(u.ux, u.uy);
		    pline("A field of force surrounds you!");
	    } else if(!destroy_arm(some_armor(&youmonst)))
		    Your("skin itches.");
	    dmg = 0;
	    break;
	case 7:
	case 6:		/* drain strength */
	    if(Antimagic) {
		shieldeff(u.ux, u.uy);
		You_feel("momentarily weakened.");
	    } else {
		You("suddenly feel weaker!");
		dmg = mtmp->m_lev - 6;
		if(Half_spell_damage) dmg = (dmg+1) / 2;
		losestr(rnd(dmg));
		if(u.uhp < 1)
		    done_in_by(mtmp);
	    }
	    dmg = 0;
	    break;
	case 5:		/* make invisible if not */
	case 4:
	    if (!mtmp->minvis && !mtmp->invis_blkd) {
		if(canseemon(mtmp)) {
		    if (!See_invisible)
			pline("%s suddenly disappears!", Monnam(mtmp));
		    else
			pline("%s suddenly becomes transparent!", Monnam(mtmp));
		}
		mon_set_minvis(mtmp);
		dmg = 0;
		break;
	    }
	    /* else fall through to next case */
	case 3:		/* stun */
	    if (Antimagic || Free_action) {
		shieldeff(u.ux, u.uy);
		if(!Stunned)
		    You_feel("momentarily disoriented.");
		make_stunned(1L, FALSE);
	    } else {
		if (Stunned)
		    You("struggle to keep your balance.");
		else
		    You("reel...");
		dmg = d(ACURR(A_DEX) < 12 ? 6 : 4, 4);
		if(Half_spell_damage) dmg = (dmg+1) / 2;
		make_stunned(HStun + dmg, FALSE);
	    }
	    dmg = 0;
	    break;
	case 2:		/* haste self */
	    mon_adjust_speed(mtmp, 1);
	    dmg = 0;
	    break;
	case 1:		/* cure self */
	    if(mtmp->mhp < mtmp->mhpmax) {
		if (canseemon(mtmp))
		    pline("%s looks better.", Monnam(mtmp));
		if((mtmp->mhp += rnd(8)) > mtmp->mhpmax)
		    mtmp->mhp = mtmp->mhpmax;
		dmg = 0;
		break;
	    }
	    /* else fall through to default */
	case 0:
	default:		/* psi bolt */
	    /* prior to 3.3.2 Antimagic was setting the damage to 1--this
	       made the spell virtually harmless to players with magic res. */
	    if(Antimagic) {
		shieldeff(u.ux, u.uy);
		dmg = (dmg + 1)/2;
	    }
	    if (dmg <= 5)
		You("get a slight %sache.",body_part(HEAD));
	    else if (dmg <= 10)
		Your("brain is on fire!");
	    else if (dmg <= 20)
		Your("%s suddenly aches painfully!", body_part(HEAD));
	    else
		Your("%s suddenly aches very painfully!", body_part(HEAD));
	    break;
    }
    if(dmg) mdamageu(mtmp, dmg);
}

STATIC_OVL
void
cast_cleric_spell(mtmp, dmg, spellnum)
struct monst *mtmp;
int dmg;
int spellnum;
{
    if (dmg == 0 && !is_undirected_spell(AD_CLRC, spellnum)) {
	impossible("cast directed cleric spell with dmg=0?");
	return;
    }

    switch(spellnum) {
	case 14:
	    /* this is physical damage, not magical damage */
	    pline("A sudden geyser slams into you from nowhere!");
	    dmg = d(8, 6);
	    if(Half_physical_damage) dmg = (dmg+1) / 2;
	    break;
	case 13:
	    pline("A pillar of fire strikes all around you!");
	    if (Fire_resistance) {
		shieldeff(u.ux, u.uy);
		dmg = 0;
	    } else
		dmg = d(8, 6);
	    if(Half_spell_damage) dmg = (dmg+1) / 2;
	    burn_away_slime();
	    (void) burnarmor(&youmonst);
	    destroy_item(SCROLL_CLASS, AD_FIRE);
	    destroy_item(POTION_CLASS, AD_FIRE);
	    destroy_item(SPBOOK_CLASS, AD_FIRE);
	    burn_floor_paper(u.ux, u.uy, TRUE, FALSE);
	    break;
	case 12:
	    pline("A bolt of lightning strikes down at you from above!");
	    if (ureflects("It bounces off your %s.", "") || Shock_resistance) {
		shieldeff(u.ux, u.uy);
		dmg = 0;
	    } else
		dmg = d(8, 6);
	    if(Half_spell_damage) dmg = (dmg+1) / 2;
	    destroy_item(WAND_CLASS, AD_ELEC);
	    destroy_item(RING_CLASS, AD_ELEC);
	    break;
	case 11:		/* curse random items */
	case 10:
	    You_feel("as if you need some help.");
	    rndcurse();
	    dmg = 0;
	    break;
	case 9:
	case 8:		/* insects */
	{
	    /* Try for insects, and if there are none
	       left, go for (sticks to) snakes.  -3. */
	    int i;
	    struct permonst *pm = mkclass(S_ANT,0);
	    struct monst *mtmp2;
	    char let = (pm ? S_ANT : S_SNAKE);
	    boolean success;

	    success = pm ? TRUE : FALSE;
	    for (i = 0; i <= (int) mtmp->m_lev; i++) {
	       if ((pm = mkclass(let,0)) &&
			(mtmp2 = makemon(pm, u.ux, u.uy, NO_MM_FLAGS))) {
		    success = TRUE;
		    mtmp2->msleeping = mtmp2->mpeaceful = mtmp2->mtame = 0;
		    set_malign(mtmp2);
		}
	    }
	    if (canseemon(mtmp)) {
		/* wrong--should check if you can see the insects/snakes */
		if (!success)
		    pline("%s casts at a clump of sticks, but nothing happens.",
			Monnam(mtmp));
		else if (let == S_SNAKE)
		    pline("%s transforms clump of sticks into snakes!",
			Monnam(mtmp));
		else
		    pline("%s summons insects!", Monnam(mtmp));
	    }
	    dmg = 0;
	    break;
	}
	case 6:
	case 7:		/* blindness */
	    /* note: resists_blnd() doesn't apply here */
	    if (!Blinded) {
		int num_eyes = eyecount(youmonst.data);
		pline("Scales cover your %s!",
		      (num_eyes == 1) ?
		      body_part(EYE) : makeplural(body_part(EYE)));
		make_blinded(Half_spell_damage ? 100L:200L, FALSE);
		if (!Blind) Your(vision_clears);
		dmg = 0;
		break;
	    }
	    /* otherwise fall through */
	case 4:
	case 5:		/* hold */
	    if (Antimagic || Free_action) {
		shieldeff(u.ux, u.uy);
		if(multi >= 0)
		    You("stiffen briefly.");
		nomul(-1);
	    } else {
		if (multi >= 0)
		    You("are frozen in place!");
		dmg = 4 + (int)mtmp->m_lev;
		if (Half_spell_damage) dmg = (dmg+1) / 2;
		nomul(-dmg);
	    }
	    dmg = 0;
	    break;
	case 3:
	    if(Antimagic) {
		shieldeff(u.ux, u.uy);
		You_feel("momentarily dizzy.");
	    } else {
		dmg = (int)mtmp->m_lev;
		if(Half_spell_damage) dmg = (dmg+1) / 2;
		make_confused(HConfusion + dmg, TRUE);
	    }
	    dmg = 0;
	    break;
	case 2:
	case 1:		/* cure self */
	    if(mtmp->mhp < mtmp->mhpmax) {
		if (canseemon(mtmp))
		    pline("%s looks better.", Monnam(mtmp));
		if((mtmp->mhp += rnd(8)) > mtmp->mhpmax)
		    mtmp->mhp = mtmp->mhpmax;
		dmg = 0;
		break;
	    }
	    /* else fall through to default */
	default:		/* wounds */
	case 0:
	    if(Antimagic) {
		shieldeff(u.ux, u.uy);
		dmg = (dmg + 1)/2;
	    }
	    if (dmg <= 5)
		Your("skin itches badly for a moment.");
	    else if (dmg <= 10)
		pline("Wounds appear on your body!");
	    else if (dmg <= 20)
		pline("Severe wounds appear on your body!");
	    else
		pline("Your body is covered with painful wounds!");
	    break;
    }
    if(dmg) mdamageu(mtmp, dmg);
}

STATIC_DCL
boolean
is_undirected_spell(adtyp, spellnum)
int adtyp;
int spellnum;
{
    if (adtyp == AD_SPEL) {
	if ((spellnum >= 13 && spellnum <= 19) || spellnum == 5 ||
		spellnum == 4 || spellnum == 2 || spellnum == 1)
	    return TRUE;
    } else if (adtyp == AD_CLRC) {
	if (spellnum == 9 || spellnum == 8 || spellnum == 2 || spellnum == 1)
	    return TRUE;
    }
    return FALSE;
}

/* Some undirected spells are useless under some circumstances */
STATIC_DCL
boolean
spell_would_be_useless(mtmp, adtyp, spellnum)
struct monst *mtmp;
int spellnum;
int adtyp;
{
    if (adtyp == AD_SPEL) {
	/* aggravate monsters, etc. won't be cast by peaceful monsters */
	if (mtmp->mpeaceful && spellnum >= 13 && spellnum <= 19)
	    return TRUE;
	/* haste self when already fast */
	if (mtmp->permspeed == MFAST && spellnum == 2)
	    return TRUE;
	/* invisibility when already invisible */
	if ((mtmp->minvis || mtmp->invis_blkd) && (spellnum == 4 || spellnum == 5))
	    return TRUE;
	/* peaceful monster won't cast invisibility if you can't see invisible,
	   same as when monster drink potions of invisibility.  This doesn't
	   really make a lot of sense, but lets the player avoid hitting
	   peaceful monsters by mistake */
	if (mtmp->mpeaceful && !See_invisible && (spellnum == 4 || spellnum == 5))
	    return TRUE;
	/* healing when already healed */
	if (mtmp->mhp == mtmp->mhpmax && spellnum == 1)
	    return TRUE;
    } else if (adtyp == AD_CLRC) {
	/* summon insects/sticks to snakes won't be cast by peaceful monsters */
	if (mtmp->mpeaceful && (spellnum == 9 || spellnum == 8))
	    return TRUE;
	/* healing when already healed */
	if (mtmp->mhp == mtmp->mhpmax && (spellnum == 1 || spellnum == 2))
	    return TRUE;
    }
    return FALSE;
}

#endif /* OVLB */
#ifdef OVL0

/* convert 1..10 to 0..9; add 10 for second group (spell casting) */
#define ad_to_typ(k) (10 + (int)k - 1)

int
buzzmu(mtmp, mattk)		/* monster uses spell (ranged) */
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	/* don't print constant stream of curse messages for 'normal'
	   spellcasting monsters at range */
	if (mattk->adtyp > AD_SPC2)
	    return(0);

	if (mtmp->mcan) {
	    cursetxt(mtmp, FALSE);
	    return(0);
	}
	if(lined_up(mtmp) && rn2(3)) {
	    nomul(0);
	    if(mattk->adtyp && (mattk->adtyp < 11)) { /* no cf unsigned >0 */
		if(canseemon(mtmp))
		    pline("%s zaps you with a %s!", Monnam(mtmp),
			  flash_types[ad_to_typ(mattk->adtyp)]);
		buzz(-ad_to_typ(mattk->adtyp), (int)mattk->damn,
		     mtmp->mx, mtmp->my, sgn(tbx), sgn(tby));
	    } else impossible("Monster spell %d cast", mattk->adtyp-1);
	}
	return(1);
}

#endif /* OVL0 */

/*mcastu.c*/

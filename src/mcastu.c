/*	SCCS Id: @(#)mcastu.c	3.3	97/11/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL void FDECL(cursetxt,(struct monst *));

#ifdef OVL0

extern const char *flash_types[];	/* from zap.c */

/* feedback when frustrated monster couldn't cast a spell */
STATIC_OVL
void
cursetxt(mtmp)
struct monst *mtmp;
{
	if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
	    const char *point_msg;  /* spellcasting monsters are impolite */

	    if ((Invis && !perceives(mtmp->data) &&
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

int
castmu(mtmp, mattk)	/* monster casts spell at you */
	register struct monst *mtmp;
	register struct attack *mattk;
{
	int	dmg, ml = mtmp->m_lev;

	if(mtmp->mcan || mtmp->mspec_used || !ml) {  /* could not attack */
	    cursetxt(mtmp);
	    return(0);
	} else {
	    nomul(0);
	    if(rn2(ml*10) < (mtmp->mconf ? 100 : 20)) {	/* fumbled attack */
		if (canseemon(mtmp) && flags.soundok)
		    pline_The("air crackles around %s.", mon_nam(mtmp));
		return(0);
	    }
	}
/*
 *	As these are spells, the damage is related to the level
 *	of the monster casting the spell.
 */
	if (mattk->damd)
		dmg = d((int)((ml/3) + mattk->damn), (int)mattk->damd);
	else dmg = d((int)((ml/3) + 1), 6);
	if (Half_spell_damage) dmg = (dmg+1) / 2;

	switch(mattk->adtyp)   {

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
	    case AD_SPEL:	/* random spell */

		mtmp->mspec_used = 10 - mtmp->m_lev;
		if (mtmp->mspec_used < 2) mtmp->mspec_used = 2;
		switch(rn2((int)mtmp->m_lev)) {
		    case 22:
		    case 21:
		    case 20:
			pline("Oh no, %s's using the touch of death!",
			      humanoid(mtmp->data)
				  ? (mtmp->female ? "she" : "he")
				  : "it"
			     );
			if (nonliving(youmonst.data) || is_demon(youmonst.data))
			    You("seem no deader than before.");
			else if (!Antimagic && rn2(ml) > 12) {

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
			nasty(mtmp);	/* summon something nasty */
			/* fall into the next case */
		    case 14:		/* aggravate all monsters */
		    case 13:
			aggravate();
			dmg = 0;
			break;
		    case 12:		/* curse random items */
		    case 11:
		    case 10:
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
			    dmg = ml - 6;
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
			    if(canseemon(mtmp) && !See_invisible)
				pline("%s suddenly disappears!", Monnam(mtmp));
			    mon_set_minvis(mtmp);
			    dmg = 0;
			    break;
			} /* else fall into the next case */
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
			    if((mtmp->mhp += rnd(8)) > mtmp->mhpmax)
				mtmp->mhp = mtmp->mhpmax;
			    dmg = 0;
			    break;
			} /* else fall through to default case */
		    default:		/* psi bolt */
			if(Antimagic) {
			    shieldeff(u.ux, u.uy);
			    You("get a slight %sache.",body_part(HEAD));
			    dmg = 1;
			} else {
			    if (dmg <= 10)
				Your("brain is on fire!");
			    else Your("%s suddenly aches!", body_part(HEAD));
			}
			break;
		}
		break;

	    case AD_CLRC:	/* clerical spell */
		mtmp->mspec_used = 10 - mtmp->m_lev;
		if (mtmp->mspec_used < 2) mtmp->mspec_used = 2;
		switch(rn2((int)mtmp->m_lev)) {
		    /* Other ideas: lightning bolts, towers of flame,
				    gush of water  -3. */

		    default:		/* confuse */
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
		    case 12:		/* curse random items */
		    case 11:
		    case 10:
			rndcurse();
			dmg = 0;
			break;
		    case 9:
		    case 8:		/* insects */
			/* Try for insects, and if there are none
			   left, go for (sticks to) snakes.  -3. */
			{
			int i;
			struct permonst *pm = mkclass(S_ANT,0);
			struct monst *mtmp2;
			char let = (pm ? S_ANT : S_SNAKE);

			for (i = 0; i <= (int) mtmp->m_lev; i++)
			   if ((pm = mkclass(let,0)) &&
			(mtmp2 = makemon(pm, u.ux, u.uy, NO_MM_FLAGS))) {
				mtmp2->msleeping = mtmp2->mpeaceful =
					mtmp2->mtame = 0;
				set_malign(mtmp2);
			    }
			}
			dmg = 0;
			break;
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
		    case 4:
		    case 5:		/* wound */
			if(Antimagic) {
			    shieldeff(u.ux, u.uy);
			    Your("skin itches badly for a moment.");
			    dmg = 0;
			} else {
			    pline("Wounds appear on your body!");
			    dmg = d(2,8) + 1;
			    if (Half_spell_damage) dmg = (dmg+1) / 2;
			}
			break;
		    case 3:		/* hold */
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
		    case 2:
		    case 1:		/* cure self */
			if(mtmp->mhp < mtmp->mhpmax) {
			    if((mtmp->mhp += rnd(8)) > mtmp->mhpmax)
				mtmp->mhp = mtmp->mhpmax;
			    dmg = 0;
			    break;
			} /* else fall through to default case */
		}
	}
	if(dmg) mdamageu(mtmp, dmg);
	return(1);
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
	if(mtmp->mcan || mattk->adtyp > AD_SPC2) {
	    cursetxt(mtmp);
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

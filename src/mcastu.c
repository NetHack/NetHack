/* NetHack 3.7	mcastu.c	$NHDT-Date: 1770949988 2026/02/12 18:33:08 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.111 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

enum mcast_spells {
    MCAST_PSI_BOLT = 0,
    MCAST_OPEN_WOUNDS,
    MCAST_LIGHTNING,
    MCAST_FIRE_PILLAR,
    MCAST_GEYSER,
    MCAST_DEATH_TOUCH,

    MCAST_CURE_SELF,
    MCAST_HASTE_SELF,
    MCAST_DISAPPEAR,

    MCAST_AGGRAVATION,
    MCAST_STUN_YOU,
    MCAST_WEAKEN_YOU,
    MCAST_CONFUSE_YOU,
    MCAST_PARALYZE,
    MCAST_BLIND_YOU,

    MCAST_DESTRY_ARMR,
    MCAST_CURSE_ITEMS,

    MCAST_INSECTS,
    MCAST_SUMMON_MONS,
    MCAST_CLONE_WIZ
};

staticfn void cursetxt(struct monst *, boolean);
staticfn int choose_magic_spell(struct monst *);
staticfn int choose_clerical_spell(struct monst *);
staticfn int m_cure_self(struct monst *, int);
staticfn void mcast_death_touch(struct monst *);
staticfn void mcast_clone_wiz(struct monst *);
staticfn void mcast_summon_mons(struct monst *);
staticfn void mcast_destroy_armor(void);
staticfn void mcast_weaken_you(struct monst *, int);
staticfn void mcast_disappear(struct monst *);
staticfn void mcast_stun_you(int);
staticfn int mcast_geyser(int);
staticfn int mcast_fire_pillar(struct monst *, int);
staticfn int mcast_lightning(struct monst *, int);
staticfn int mcast_psi_bolt(int);
staticfn int mcast_open_wounds(int);
staticfn void mcast_insects(struct monst *);
staticfn void mcast_blind_you(void);
staticfn int mcast_paralyze(struct monst *);
staticfn void mcast_confuse_you(struct monst *);
staticfn void mcast_spell(struct monst *, int, int);
staticfn boolean is_undirected_spell(int);
staticfn boolean spell_would_be_useless(struct monst *, int);

/* feedback when frustrated monster couldn't cast a spell */
staticfn void
cursetxt(struct monst *mtmp, boolean undirected)
{
    if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
        const char *point_msg; /* spellcasting monsters are impolite */

        if (undirected)
            point_msg = "all around, then curses";
        else if ((Invis && !perceives(mtmp->data)
                  && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                 || is_obj_mappear(&gy.youmonst, STRANGE_OBJECT)
                 || u.uundetected)
            point_msg = "and curses in your general direction";
        else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
            point_msg = "and curses at your displaced image";
        else
            point_msg = "at you, then curses";

        pline_mon(mtmp, "%s points %s.", Monnam(mtmp), point_msg);
    } else if ((!(svm.moves % 4) || !rn2(4))) {
        if (!Deaf)
            Norep("You hear a mumbled curse.");   /* Deaf-aware */
    }
}

/* convert a level-based random selection into a specific mage spell;
   inappropriate choices will be screened out by spell_would_be_useless() */
staticfn int
choose_magic_spell(struct monst *mtmp)
{
    int spellval = rn2(mtmp->m_lev);

    /* for 3.4.3 and earlier, val greater than 22 selected default spell */
    while (spellval > 24 && rn2(25))
        spellval = rn2(spellval);

    switch (spellval) {
    case 24:
    case 23:
        if (Antimagic || Hallucination)
            return MCAST_PSI_BOLT;
        FALLTHROUGH;
        /*FALLTHRU*/
    case 22:
    case 21:
    case 20:
        return MCAST_DEATH_TOUCH;
    case 19:
    case 18:
        return MCAST_CLONE_WIZ;
    case 17:
    case 16:
    case 15:
        return MCAST_SUMMON_MONS;
    case 14:
    case 13:
        return MCAST_AGGRAVATION;
    case 12:
    case 11:
    case 10:
        return MCAST_CURSE_ITEMS;
    case 9:
    case 8:
        return MCAST_DESTRY_ARMR;
    case 7:
    case 6:
        return MCAST_WEAKEN_YOU;
    case 5:
    case 4:
        return MCAST_DISAPPEAR;
    case 3:
        return MCAST_STUN_YOU;
    case 2:
        return MCAST_HASTE_SELF;
    case 1:
        return MCAST_CURE_SELF;
    case 0:
    default:
        return MCAST_PSI_BOLT;
    }
}

/* convert a level-based random selection into a specific cleric spell */
staticfn int
choose_clerical_spell(struct monst *mtmp)
{
    int spellnum = rn2(mtmp->m_lev);

    /* for 3.4.3 and earlier, num greater than 13 selected the default spell
     */
    while (spellnum > 15 && rn2(16))
        spellnum = rn2(spellnum);

    switch (spellnum) {
    case 15:
    case 14:
        if (rn2(3))
            return MCAST_OPEN_WOUNDS;
        FALLTHROUGH;
        /*FALLTHRU*/
    case 13:
        return MCAST_GEYSER;
    case 12:
        return MCAST_FIRE_PILLAR;
    case 11:
        return MCAST_LIGHTNING;
    case 10:
    case 9:
        return MCAST_CURSE_ITEMS;
    case 8:
        return MCAST_INSECTS;
    case 7:
    case 6:
        return MCAST_BLIND_YOU;
    case 5:
    case 4:
        return MCAST_PARALYZE;
    case 3:
    case 2:
        return MCAST_CONFUSE_YOU;
    case 1:
        return MCAST_CURE_SELF;
    case 0:
    default:
        return MCAST_OPEN_WOUNDS;
    }
}

/* return values:
 * 1: successful spell
 * 0: unsuccessful spell
 */
int
castmu(
    struct monst *mtmp,   /* caster */
    struct attack *mattk, /* caster's current attack */
    boolean thinks_it_foundyou,    /* might be mistaken if displaced */
    boolean foundyou)              /* knows hero's precise location */
{
    int dmg, ml = mtmp->m_lev;
    int ret;
    int spellnum = 0;

    /* Three cases:
     * -- monster is attacking you.  Search for a useful spell.
     * -- monster thinks it's attacking you.  Search for a useful spell,
     *    without checking for undirected.  If the spell found is directed,
     *    it fails with cursetxt() and loss of mspec_used.
     * -- monster isn't trying to attack.  Select a spell once.  Don't keep
     *    searching; if that spell is not useful (or if it's directed),
     *    return and do something else.
     * Since most spells are directed, this means that a monster that isn't
     * attacking casts spells only a small portion of the time that an
     * attacking monster does.
     */
    if ((mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) && ml) {
        int cnt = 40;

        do {
            if (mattk->adtyp == AD_SPEL)
                spellnum = choose_magic_spell(mtmp);
            else
                spellnum = choose_clerical_spell(mtmp);
            /* not trying to attack?  don't allow directed spells */
            if (!thinks_it_foundyou) {
                if (!is_undirected_spell(spellnum)
                    || spell_would_be_useless(mtmp, spellnum)) {
                    if (foundyou)
                        impossible(
                       "spellcasting monster found you and doesn't know it?");
                    return M_ATTK_MISS;
                }
                break;
            }
        } while (--cnt > 0
                 && spell_would_be_useless(mtmp, spellnum));
        if (cnt == 0)
            return M_ATTK_MISS;
    }

    /* monster unable to cast spells? */
    if (mtmp->mcan || mtmp->mspec_used || !ml
        || m_seenres(mtmp, cvt_adtyp_to_mseenres(mattk->adtyp))) {
        cursetxt(mtmp, is_undirected_spell(spellnum));
        return M_ATTK_MISS;
    }

    debugpline3("castmu:%s,lvl:%i,spell:%i", noit_Monnam(mtmp), ml, spellnum);

    if (mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) {
        /* monst->m_lev is unsigned (uchar), monst->mspec_used is int */
        mtmp->mspec_used = (int) ((mtmp->m_lev < 8) ? (10 - mtmp->m_lev) : 2);
    }

    /* Monster can cast spells, but is casting a directed spell at the
     * wrong place?  If so, give a message, and return.
     * Do this *after* penalizing mspec_used.
     *
     * FIXME?
     *  Shouldn't wall of lava have a case similar to wall of water?
     *  And should cold damage hit water or lava instead of missing
     *  even when the caster has targeted the wrong spot?  Likewise
     *  for fire mis-aimed at ice.
     */
    if (!foundyou && thinks_it_foundyou
        && !is_undirected_spell(spellnum)) {
        pline_mon(mtmp, "%s casts a spell at %s!",
                 canseemon(mtmp) ? Monnam(mtmp) : "Something",
                 is_waterwall(mtmp->mux, mtmp->muy) ? "empty water"
                                                    : "thin air");
        return M_ATTK_MISS;
    }

    nomul(0);
    if (rn2(ml * 10) < (mtmp->mconf ? 100 : 20)) { /* fumbled attack */
        Soundeffect(se_air_crackles, 60);
        if (canseemon(mtmp) && !Deaf) {
            set_msg_xy(mtmp->mx, mtmp->my);
            pline_The("air crackles around %s.", mon_nam(mtmp));
        }
        return M_ATTK_MISS;
    }
    if (canspotmon(mtmp) || !is_undirected_spell(spellnum)) {
        pline_mon(mtmp, "%s casts a spell%s!",
                 canspotmon(mtmp) ? Monnam(mtmp) : "Something",
                 is_undirected_spell(spellnum) ? ""
                 : (Invis && !perceives(mtmp->data)
                    && !u_at(mtmp->mux, mtmp->muy))
                   ? " at a spot near you"
                   : (Displaced && !u_at(mtmp->mux, mtmp->muy))
                     ? " at your displaced image"
                     : " at you");
    }

    /*
     * As these are spells, the damage is related to the level
     * of the monster casting the spell.
     */
    if (!foundyou) {
        dmg = 0;
        if (mattk->adtyp != AD_SPEL && mattk->adtyp != AD_CLRC) {
            impossible(
              "%s casting non-hand-to-hand version of hand-to-hand spell %d?",
                       Monnam(mtmp), mattk->adtyp);
            return M_ATTK_MISS;
        }
    } else if (mattk->damd)
        dmg = d((int) ((ml / 2) + mattk->damn), (int) mattk->damd);
    else
        dmg = d((int) ((ml / 2) + 1), 6);
    if (Half_spell_damage)
        dmg = (dmg + 1) / 2;

    ret = M_ATTK_HIT;
    /*
     * FIXME: none of these hit the steed when hero is riding, nor do
     *  they inflict damage on carried items.
     */
    switch (mattk->adtyp) {
    case AD_FIRE:
        pline("You're enveloped in flames.");
        if (Fire_resistance) {
            shieldeff(u.ux, u.uy);
            pline("But you resist the effects.");
            monstseesu(M_SEEN_FIRE);
            dmg = 0;
        } else {
            monstunseesu(M_SEEN_FIRE);
        }
        burn_away_slime();
        /* burn up flammable items on the floor, melt ice terrain */
        mon_spell_hits_spot(mtmp, AD_FIRE, u.ux, u.uy);
        break;
    case AD_COLD:
        pline("You're covered in frost.");
        if (Cold_resistance) {
            shieldeff(u.ux, u.uy);
            pline("But you resist the effects.");
            monstseesu(M_SEEN_COLD);
            dmg = 0;
        } else {
            monstunseesu(M_SEEN_COLD);
        }
        /* freeze water or lava terrain */
        /* FIXME: mon_spell_hits_spot() uses zap_over_floor(); unlike with
         * fire, it does not target susceptible floor items with cold */
        mon_spell_hits_spot(mtmp, AD_COLD, u.ux, u.uy);
        break;
    case AD_MAGM:
        You("are hit by a shower of missiles!");
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            pline_The("missiles bounce off!");
            monstseesu(M_SEEN_MAGR);
            dmg = 0;
        } else {
            dmg = d((int) mtmp->m_lev / 2 + 1, 6);
            monstunseesu(M_SEEN_MAGR);
        }
        /* shower of magic missiles scuffs an engraving */
        mon_spell_hits_spot(mtmp, AD_MAGM, u.ux, u.uy);
        break;
    case AD_SPEL: /* wizard spell */
    case AD_CLRC: /* clerical spell */
        mcast_spell(mtmp, dmg, spellnum);
        dmg = 0; /* done by the spell casting functions */
        break;
    } /* switch */
    if (dmg)
        mdamageu(mtmp, dmg);
    return ret;
}

staticfn int
m_cure_self(struct monst *mtmp, int dmg)
{
    if (mtmp->mhp < mtmp->mhpmax) {
        if (canseemon(mtmp))
            pline_mon(mtmp, "%s looks better.", Monnam(mtmp));
        /* note: player healing does 6d4; this used to do 1d8 */
        healmon(mtmp, d(3, 6), 0);
        dmg = 0;
    }
    return dmg;
}

/* unlike the finger of death spell which behaves like a wand of death,
   this monster spell only attacks the hero */
void
touch_of_death(struct monst *mtmp)
{
    char kbuf[BUFSZ];
    int dmg = 50 + d(8, 6);
    int drain = dmg / 2;

    /* if we get here, we know that hero isn't magic resistant and isn't
       poly'd into an undead or demon */
    You_feel("drained...");
    (void) death_inflicted_by(kbuf, "the touch of death", mtmp);

    if (Upolyd) {
        u.mh = 0;
        rehumanize(); /* fatal iff Unchanging */
    } else if (drain >= u.uhpmax) {
        svk.killer.format = KILLED_BY;
        Strcpy(svk.killer.name, kbuf);
        done(DIED);
    } else {
        /* HP manipulation similar to poisoned(attrib.c) */
        int olduhp = u.uhp,
            uhpmin = minuhpmax(3),
            newuhpmax = u.uhpmax - drain;

        setuhpmax(max(newuhpmax, uhpmin), FALSE);
        dmg = adjuhploss(dmg, olduhp); /* reduce pending damage if uhp has
                                        * already been reduced due to drop
                                        * in uhpmax */
        losehp(dmg, kbuf, KILLED_BY);
    }
    svk.killer.name[0] = '\0'; /* not killed if we get here... */
}

/* give a reason for death by some monster spells */
char *
death_inflicted_by(
    char *outbuf,            /* assumed big enough; pm_names are short */
    const char *deathreason, /* cause of death */
    struct monst *mtmp)      /* monster who caused it */
{
    Strcpy(outbuf, deathreason);
    if (mtmp) {
        struct permonst *mptr = mtmp->data,
            *champtr = (ismnum(mtmp->cham)) ? &mons[mtmp->cham] : mptr;
        const char *realnm = pmname(champtr, Mgender(mtmp)),
            *fakenm = pmname(mptr, Mgender(mtmp));

        /* greatly simplified extract from done_in_by(), primarily for
           reason for death due to 'touch of death' spell; if mtmp is
           shape changed, it won't be a vampshifter or mimic since they
           can't cast spells */
        if (!type_is_pname(champtr) && !the_unique_pm(mptr))
            realnm = an(realnm);
        Sprintf(eos(outbuf), " inflicted by %s%s",
                the_unique_pm(mptr) ? "the " : "", realnm);
        if (champtr != mptr)
            Sprintf(eos(outbuf), " imitating %s", an(fakenm));
    }
    return outbuf;
}

/*
 * Monster wizard and cleric spellcasting functions.
 */

staticfn void
mcast_death_touch(struct monst *mtmp)
{
    pline("Oh no, %s's using the touch of death!", mhe(mtmp));
    if (nonliving(gy.youmonst.data) || is_demon(gy.youmonst.data)) {
        You("seem no deader than before.");
    } else if (!Antimagic && rn2(mtmp->m_lev) > 12) {
        if (Hallucination) {
            You("have an out of body experience.");
        } else {
            touch_of_death(mtmp);
        }
        monstunseesu(M_SEEN_MAGR);
    } else {
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            monstseesu(M_SEEN_MAGR);
        }
        pline("Lucky for you, it didn't work!");
    }
}

staticfn void
mcast_clone_wiz(struct monst *mtmp)
{
    if (mtmp->iswiz && svc.context.no_of_wizards == 1) {
        pline("Double Trouble...");
        clonewiz();
    } else
        impossible("bad wizard cloning?");
}

staticfn void
mcast_summon_mons(struct monst *mtmp)
{
    int count = nasty(mtmp);

    if (!count) {
        ; /* nothing was created? */
    } else if (mtmp->iswiz) {
        SetVoice(mtmp, 0, 80, 0);
        verbalize("Destroy the thief, my pet%s!", plur(count));
    } else {
        boolean one = (count == 1);
        const char *mappear = one ? "A monster appears"
                                  : "Monsters appear";

        /* messages not quite right if plural monsters created but
           only a single monster is seen */
        if (Invis && !perceives(mtmp->data)
            && (mtmp->mux != u.ux || mtmp->muy != u.uy))
            pline("%s %s a spot near you!", mappear,
                  one ? "at" : "around");
        else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
            pline("%s %s your displaced image!", mappear,
                  one ? "by" : "around");
        else
            pline("%s from nowhere!", mappear);
    }
}

staticfn void
mcast_destroy_armor(void)
{
    if (Antimagic) {
        shieldeff(u.ux, u.uy);
        monstseesu(M_SEEN_MAGR);
        pline("A field of force surrounds you!");
    } else if (!destroy_arm()) {
        Your("skin itches.");
    } else {
        /* monsters only realize you aren't magic-protected if armor is
           actually destroyed */
        monstunseesu(M_SEEN_MAGR);
    }
}

staticfn void
mcast_weaken_you(struct monst *mtmp, int dmg)
{
    if (Antimagic) {
        shieldeff(u.ux, u.uy);
        monstseesu(M_SEEN_MAGR);
        You_feel("momentarily weakened.");
    } else {
        char kbuf[BUFSZ];

        You("suddenly feel weaker!");
        dmg = mtmp->m_lev - 6;
        if (dmg < 1) /* paranoia since only chosen when m_lev is high */
            dmg = 1;
        if (Half_spell_damage)
            dmg = (dmg + 1) / 2;
        losestr(rnd(dmg),
                death_inflicted_by(kbuf, "strength loss", mtmp),
                KILLED_BY);
        svk.killer.name[0] = '\0'; /* not killed if we get here... */
        monstunseesu(M_SEEN_MAGR);
    }
}

staticfn void
mcast_disappear(struct monst *mtmp)
{
    if (!mtmp->minvis && !mtmp->invis_blkd) {
        if (canseemon(mtmp))
            pline_mon(mtmp, "%s suddenly %s!", Monnam(mtmp),
                      !See_invisible ? "disappears" : "becomes transparent");
        mon_set_minvis(mtmp, FALSE);
        if (cansee(mtmp->mx, mtmp->my) && !canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
    } else
        impossible("no reason for monster to cast disappear spell?");
}

staticfn void
mcast_stun_you(int dmg)
{
    if (Antimagic || Free_action) {
        shieldeff(u.ux, u.uy);
        monstseesu(M_SEEN_MAGR);
        if (!Stunned)
            You_feel("momentarily disoriented.");
        make_stunned(1L, FALSE);
    } else {
        You(Stunned ? "struggle to keep your balance." : "reel...");
        dmg = d(ACURR(A_DEX) < 12 ? 6 : 4, 4);
        if (Half_spell_damage)
            dmg = (dmg + 1) / 2;
        make_stunned((HStun & TIMEOUT) + (long) dmg, FALSE);
        monstunseesu(M_SEEN_MAGR);
    }
}

staticfn int
mcast_geyser(int dmg)
{
    /* this is physical damage (force not heat),
     * not magical damage or fire damage
     */
    pline("A sudden geyser slams into you from nowhere!");
    dmg = d(8, 6);
    if (Half_physical_damage)
        dmg = (dmg + 1) / 2;
#if 0   /* since inventory items aren't affected, don't include this */
        /* make floor items wet */
    water_damage_chain(level.objects[u.ux][u.uy], TRUE);
#endif
    return dmg;
}

staticfn int
mcast_fire_pillar(struct monst *mtmp, int dmg)
{
    int orig_dmg;

    pline("A pillar of fire strikes all around you!");
    orig_dmg = dmg = d(8, 6);
    if (Fire_resistance) {
        shieldeff(u.ux, u.uy);
        monstseesu(M_SEEN_FIRE);
        dmg = 0;
    } else {
        monstunseesu(M_SEEN_FIRE);
    }
    if (Half_spell_damage)
        dmg = (dmg + 1) / 2;
    burn_away_slime();
    (void) burnarmor(&gy.youmonst);
    /* item destruction dmg */
    (void) destroy_items(&gy.youmonst, AD_FIRE, orig_dmg);
    ignite_items(gi.invent);
    /* burn up flammable items on the floor, melt ice terrain */
    mon_spell_hits_spot(mtmp, AD_FIRE, u.ux, u.uy);
    return dmg;
}

staticfn int
mcast_lightning(struct monst *mtmp, int dmg)
{
    int orig_dmg;
    boolean reflects;

    Soundeffect(se_bolt_of_lightning, 80);
    pline("A bolt of lightning strikes down at you from above!");
    reflects = ureflects("It bounces off your %s%s.", "");
    orig_dmg = dmg = d(8, 6);
    if (reflects || Shock_resistance) {
        shieldeff(u.ux, u.uy);
        dmg = 0;
        if (reflects) {
            monstseesu(M_SEEN_REFL);
            return dmg;
        }
        monstunseesu(M_SEEN_REFL);
        monstseesu(M_SEEN_ELEC);
    } else {
        monstunseesu(M_SEEN_ELEC | M_SEEN_REFL);
    }
    if (Half_spell_damage)
        dmg = (dmg + 1) / 2;
    (void) destroy_items(&gy.youmonst, AD_ELEC, orig_dmg);
    /* lightning might destroy iron bars if hero is on such a spot;
       reflection protects terrain here [execution won't get here due
       to 'if (reflects) break' above] but hero resistance doesn't;
       do this before maybe blinding the hero via flashburn() */
    mon_spell_hits_spot(mtmp, AD_ELEC, u.ux, u.uy);
    /* blind hero; no effect if already blind */
    (void) flashburn((long) rnd(100), TRUE);
    return dmg;
}

staticfn int
mcast_psi_bolt(int dmg)
{
    /* prior to 3.4.0 Antimagic was setting the damage to 1--this
       made the spell virtually harmless to players with magic res. */
    if (Antimagic) {
        shieldeff(u.ux, u.uy);
        monstseesu(M_SEEN_MAGR);
        dmg = (dmg + 1) / 2;
    } else {
        monstunseesu(M_SEEN_MAGR);
    }
    if (dmg <= 5)
        You("get a slight %sache.", body_part(HEAD));
    else if (dmg <= 10)
        Your("brain is on fire!");
    else if (dmg <= 20)
        Your("%s suddenly aches painfully!", body_part(HEAD));
    else
        Your("%s suddenly aches very painfully!", body_part(HEAD));
    return dmg;
}

staticfn int
mcast_open_wounds(int dmg)
{
    if (Antimagic) {
        shieldeff(u.ux, u.uy);
        monstseesu(M_SEEN_MAGR);
        dmg = (dmg + 1) / 2;
    } else {
        monstunseesu(M_SEEN_MAGR);
    }
    if (dmg <= 5)
        Your("skin itches badly for a moment.");
    else if (dmg <= 10)
        pline("Wounds appear on your body!");
    else if (dmg <= 20)
        pline("Severe wounds appear on your body!");
    else
        Your("body is covered with painful wounds!");
    return dmg;
}

staticfn void
mcast_insects(struct monst *mtmp)
{
    /* Try for insects, and if there are none
       left, go for (sticks to) snakes.  -3. */
    struct permonst *pm = mkclass(S_ANT, 0);
    struct monst *mtmp2 = (struct monst *) 0;
    char whatbuf[QBUFSZ], let = (pm ? S_ANT : S_SNAKE);
    boolean success = FALSE, seecaster;
    int i, quan, oldseen, newseen;
    coord bypos;
    const char *fmt, *what;

    oldseen = monster_census(TRUE);
    quan = (mtmp->m_lev < 2) ? 1 : rnd((int) mtmp->m_lev / 2);
    if (quan < 3)
        quan = 3;
    for (i = 0; i <= quan; i++) {
        if (!enexto(&bypos, mtmp->mux, mtmp->muy, mtmp->data))
            return;
        if ((pm = mkclass(let, 0)) != 0
            && (mtmp2 = makemon(pm, bypos.x, bypos.y, MM_ANGRY | MM_NOMSG))
            != 0) {
            success = TRUE;
            mtmp2->msleeping = mtmp2->mpeaceful = mtmp2->mtame = 0;
            set_malign(mtmp2);
        }
    }
    newseen = monster_census(TRUE);

    /* not canspotmon() which includes unseen things sensed via warning */
    seecaster = canseemon(mtmp) || tp_sensemon(mtmp) || Detect_monsters;
    what = (let == S_SNAKE) ? "snakes" : "insects";
    if (Hallucination)
        what = makeplural(bogusmon(whatbuf, (char *) 0));

    fmt = 0;
    if (!seecaster) {
        if (newseen <= oldseen || Unaware) {
            /* unseen caster fails or summons unseen critters,
               or unconscious hero ("You dream that you hear...") */
            You_hear("someone summoning %s.", what);
        } else {
            char *arg;

            if (what != whatbuf)
                what = strcpy(whatbuf, what);
            /* unseen caster summoned seen critter(s) */
            arg = (newseen == oldseen + 1) ? an(makesingular(what))
                                           : whatbuf;
            if (!Deaf) {
                Soundeffect(se_someone_summoning, 100);
                You_hear("someone summoning something, and %s %s.", arg,
                         vtense(arg, "appear"));
            } else {
                pline("%s %s.", upstart(arg), vtense(arg, "appear"));
            }
        }

        /* seen caster, possibly producing unseen--or just one--critters;
           hero is told what the caster is doing and doesn't necessarily
           observe complete accuracy of that caster's results (in other
           words, no need to fuss with visibility or singularization;
           player is told what's happening even if hero is unconscious) */
    } else if (!success) {
        fmt = "%s casts at a clump of sticks, but nothing happens.%s";
        what = "";
    } else if (let == S_SNAKE) {
        fmt = "%s transforms a clump of sticks into %s!";
    } else if (Invis && !perceives(mtmp->data)
               && (mtmp->mux != u.ux || mtmp->muy != u.uy)) {
        fmt = "%s summons %s around a spot near you!";
    } else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy)) {
        fmt = "%s summons %s around your displaced image!";
    } else {
        fmt = "%s summons %s!";
    }
    if (fmt) {
        DISABLE_WARNING_FORMAT_NONLITERAL;
        pline_mon(mtmp, fmt, Monnam(mtmp), what);
        RESTORE_WARNING_FORMAT_NONLITERAL;
    }
}

staticfn void
mcast_blind_you(void)
{
    /* note: resists_blnd() doesn't apply here */
    if (!Blinded) {
        int num_eyes = eyecount(gy.youmonst.data);

        pline("Scales cover your %s!", (num_eyes == 1)
                                       ? body_part(EYE)
                                       : makeplural(body_part(EYE)));
        make_blinded(Half_spell_damage ? 100L : 200L, FALSE);
        if (!Blind)
            Your1(vision_clears);
    } else
        impossible("no reason for monster to cast blindness spell?");
}

staticfn int
mcast_paralyze(struct monst *mtmp)
{
    int dmg = 0;

    if (Antimagic || Free_action) {
        shieldeff(u.ux, u.uy);
        monstseesu(M_SEEN_MAGR);
        if (gm.multi >= 0)
            You("stiffen briefly.");
        dmg = 1; /* to produce nomul(-1), not actual damage */
    } else {
        if (gm.multi >= 0)
            You("are frozen in place!");
        dmg = 4 + (int) mtmp->m_lev;
        if (Half_spell_damage)
            dmg = (dmg + 1) / 2;
        monstunseesu(M_SEEN_MAGR);
    }
    nomul(-dmg);
    gm.multi_reason = "paralyzed by a monster";
    gn.nomovemsg = 0;
    return dmg;
}

staticfn void
mcast_confuse_you(struct monst *mtmp)
{
    if (Antimagic) {
        shieldeff(u.ux, u.uy);
        monstseesu(M_SEEN_MAGR);
        You_feel("momentarily dizzy.");
    } else {
        boolean oldprop = !!Confusion;
        int dmg = (int) mtmp->m_lev;

        if (Half_spell_damage)
            dmg = (dmg + 1) / 2;
        make_confused(HConfusion + dmg, TRUE);
        if (Hallucination)
            You_feel("%s!", oldprop ? "trippier" : "trippy");
        else
            You_feel("%sconfused!", oldprop ? "more " : "");
        monstunseesu(M_SEEN_MAGR);
    }
}

/*
   If dmg is zero, then the monster is not casting at you.
   If the monster is intentionally not casting at you, we have previously
   called spell_would_be_useless() and spellnum should always be a valid
   undirected spell.
   If you modify either of these, be sure to change is_undirected_spell()
   and spell_would_be_useless().
 */
staticfn void
mcast_spell(struct monst *mtmp, int dmg, int spellnum)
{
    if (dmg < 0) {
        impossible("monster cast spell (%d) with negative dmg (%d)?",
                   spellnum, dmg);
        return;
    }
    if (dmg == 0 && !is_undirected_spell(spellnum)) {
        impossible("cast directed wizard spell (%d) with dmg=0?", spellnum);
        return;
    }

    switch (spellnum) {
    case MCAST_DEATH_TOUCH:
        mcast_death_touch(mtmp);
        dmg = 0;
        break;
    case MCAST_CLONE_WIZ:
        mcast_clone_wiz(mtmp);
        dmg = 0;
        break;
    case MCAST_SUMMON_MONS:
        mcast_summon_mons(mtmp);
        dmg = 0;
        break;
    case MCAST_AGGRAVATION:
        You_feel("that monsters are aware of your presence.");
        aggravate();
        dmg = 0;
        break;
    case MCAST_CURSE_ITEMS:
        You_feel("as if you need some help.");
        rndcurse();
        dmg = 0;
        break;
    case MCAST_DESTRY_ARMR:
        mcast_destroy_armor();
        dmg = 0;
        break;
    case MCAST_WEAKEN_YOU: /* drain strength */
        mcast_weaken_you(mtmp, dmg);
        dmg = 0;
        break;
    case MCAST_DISAPPEAR: /* makes self invisible */
        mcast_disappear(mtmp);
        dmg = 0;
        break;
    case MCAST_STUN_YOU:
        mcast_stun_you(dmg);
        dmg = 0;
        break;
    case MCAST_HASTE_SELF:
        mon_adjust_speed(mtmp, 1, (struct obj *) 0);
        dmg = 0;
        break;
    case MCAST_CURE_SELF:
        dmg = m_cure_self(mtmp, dmg);
        break;
    case MCAST_PSI_BOLT:
        dmg = mcast_psi_bolt(dmg);
        break;
    case MCAST_GEYSER:
        dmg = mcast_geyser(dmg);
        break;
    case MCAST_FIRE_PILLAR:
        dmg = mcast_fire_pillar(mtmp, dmg);
        break;
    case MCAST_LIGHTNING:
        dmg = mcast_lightning(mtmp, dmg);
        break;
    case MCAST_INSECTS:
        mcast_insects(mtmp);
        dmg = 0;
        break;
    case MCAST_BLIND_YOU:
        mcast_blind_you();
        dmg = 0;
        break;
    case MCAST_PARALYZE:
        dmg = mcast_paralyze(mtmp);
        break;
    case MCAST_CONFUSE_YOU:
        mcast_confuse_you(mtmp);
        dmg = 0;
        break;
    case MCAST_OPEN_WOUNDS:
        dmg = mcast_open_wounds(dmg);
        break;
    default:
        impossible("mcastu: invalid magic spell (%d)", spellnum);
        dmg = 0;
        break;
    }

    if (dmg)
        mdamageu(mtmp, dmg);
}

staticfn boolean
is_undirected_spell(int spellnum)
{
    switch (spellnum) {
    case MCAST_CLONE_WIZ:
    case MCAST_SUMMON_MONS:
    case MCAST_AGGRAVATION:
    case MCAST_DISAPPEAR:
    case MCAST_HASTE_SELF:
    case MCAST_CURE_SELF:
    case MCAST_INSECTS:
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

/* Some spells are useless under some circumstances. */
staticfn boolean
spell_would_be_useless(struct monst *mtmp, int spellnum)
{
    /* Some spells don't require the player to really be there and can be cast
     * by the monster when you're invisible, yet still shouldn't be cast when
     * the monster doesn't even think you're there.
     * This check isn't quite right because it always uses your real position.
     * We really want something like "if the monster could see mux, muy".
     */
    boolean mcouldseeu = couldsee(mtmp->mx, mtmp->my);

    switch (spellnum) {
    case MCAST_CLONE_WIZ:
        /* only the Wizard is allowed to clone himself */
        if (!mtmp->iswiz || svc.context.no_of_wizards > 1)
            return TRUE;
        if (!mcouldseeu)
            return TRUE;
        break;
    case MCAST_SUMMON_MONS:
        /* don't summon monsters if it doesn't think you're around */
        if (!mcouldseeu || mtmp->mpeaceful)
            return TRUE;
        break;
    case MCAST_AGGRAVATION:
        /* aggravate monsters, etc. won't be cast by peaceful monsters */
        if (!mcouldseeu || mtmp->mpeaceful)
            return TRUE;
        /* aggravation (global wakeup) when everyone is already active */
        /* if nothing needs to be awakened then this spell is useless
           but caster might not realize that [chance to pick it then
           must be very small otherwise caller's many retry attempts
           will eventually end up picking it too often] */
        if (!has_aggravatables(mtmp))
            return rn2(100) ? TRUE : FALSE;
        break;
    case MCAST_HASTE_SELF:
        /* haste self when already fast */
        if (mtmp->permspeed == MFAST)
            return TRUE;
        break;
    case MCAST_DISAPPEAR:
        /* invisibility when already invisible */
        if (mtmp->minvis || mtmp->invis_blkd)
            return TRUE;
        /* peaceful monster won't cast invisibility if you can't see
           invisible,
           same as when monsters drink potions of invisibility.  This doesn't
           really make a lot of sense, but lets the player avoid hitting
           peaceful monsters by mistake */
        if (mtmp->mpeaceful && !See_invisible)
            return TRUE;
        break;
    case MCAST_CURE_SELF:
        /* healing when already healed */
        if (mtmp->mhp == mtmp->mhpmax)
            return TRUE;
        break;
    case MCAST_INSECTS:
        /* summon insects/sticks to snakes won't be cast by peaceful monsters */
        if (!mcouldseeu || mtmp->mpeaceful)
            return TRUE;
        break;
    case MCAST_BLIND_YOU:
        if (Blinded)
            return TRUE;
        break;
    default:
        break;
    }
    return FALSE;
}

/* monster uses spell (ranged) */
int
buzzmu(struct monst *mtmp, struct attack *mattk)
{
    /* don't print constant stream of curse messages for 'normal'
       spellcasting monsters at range */
    if (!BZ_VALID_ADTYP(mattk->adtyp))
        return M_ATTK_MISS;

    if (mtmp->mcan || m_seenres(mtmp, cvt_adtyp_to_mseenres(mattk->adtyp))) {
        cursetxt(mtmp, FALSE);
        return M_ATTK_MISS;
    }
    if (lined_up(mtmp) && rn2(3)) {
        nomul(0);
        if (canseemon(mtmp))
            pline_mon(mtmp, "%s zaps you with a %s!", Monnam(mtmp),
                  flash_str(BZ_OFS_AD(mattk->adtyp), FALSE));
        gb.buzzer = mtmp;
        buzz(BZ_M_SPELL(BZ_OFS_AD(mattk->adtyp)), (int) mattk->damn,
             mtmp->mx, mtmp->my, sgn(gt.tbx), sgn(gt.tby));
        gb.buzzer = 0;
        return M_ATTK_HIT;
    }
    return M_ATTK_MISS;
}

/*mcastu.c*/

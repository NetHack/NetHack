/* NetHack 3.7	mhitu.c	$NHDT-Date: 1625838648 2021/07/09 13:50:48 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.246 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

static NEARDATA struct obj *mon_currwep = (struct obj *) 0;

static void missmu(struct monst *, boolean, struct attack *);
static void mswings(struct monst *, struct obj *, boolean);
static void wildmiss(struct monst *, struct attack *);
static void summonmu(struct monst *, boolean);
static int hitmu(struct monst *, struct attack *);
static int gulpmu(struct monst *, struct attack *);
static int explmu(struct monst *, struct attack *, boolean);
static void mayberem(struct monst *, const char *, struct obj *,
                     const char *);
static int passiveum(struct permonst *, struct monst *, struct attack *);

#define ld() ((yyyymmdd((time_t) 0) - (getyear() * 10000L)) == 0xe5)

DISABLE_WARNING_FORMAT_NONLITERAL

void
hitmsg(struct monst *mtmp, struct attack *mattk)
{
    int compat;
    const char *pfmt = 0;
    char *Monst_name = Monnam(mtmp);

    /* Note: if opposite gender, "seductively" */
    /* If same gender, "engagingly" for nymph, normal msg for others */
    if ((compat = could_seduce(mtmp, &g.youmonst, mattk)) != 0
        && !mtmp->mcan && !mtmp->mspec_used) {
        pline("%s %s you %s.", Monst_name,
              !Blind ? "smiles at" : !Deaf ? "talks to" : "touches",
              (compat == 2) ? "engagingly" : "seductively");
    } else {
        switch (mattk->aatyp) {
        case AT_BITE:
            pfmt = "%s bites!";
            break;
        case AT_KICK:
            pline("%s kicks%c", Monst_name,
                  thick_skinned(g.youmonst.data) ? '.' : '!');
            break;
        case AT_STNG:
            pfmt = "%s stings!";
            break;
        case AT_BUTT:
            pfmt = "%s butts!";
            break;
        case AT_TUCH:
            pfmt = "%s touches you!";
            break;
        case AT_TENT:
            pfmt = "%s tentacles suck you!";
            Monst_name = s_suffix(Monst_name);
            break;
        case AT_EXPL:
        case AT_BOOM:
            pfmt = "%s explodes!";
            break;
        default:
            pfmt = "%s hits!";
        }
        if (pfmt)
            pline(pfmt, Monst_name);
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* monster missed you */
static void
missmu(struct monst *mtmp, boolean nearmiss, struct attack *mattk)
{
    if (!canspotmon(mtmp))
        map_invisible(mtmp->mx, mtmp->my);

    if (could_seduce(mtmp, &g.youmonst, mattk) && !mtmp->mcan)
        pline("%s pretends to be friendly.", Monnam(mtmp));
    else
        pline("%s %smisses!", Monnam(mtmp),
              (nearmiss && Verbose(1, missmu)) ? "just " : "");

    stop_occupation();
}

/* strike types P|S|B: Pierce (pointed: stab) => "thrusts",
   Slash (edged: slice) or whack (blunt: Bash) => "swings" */
const char *
mswings_verb(
    struct obj *mwep, /* attacker's weapon */
    boolean bash)     /* True: using polearm while too close */
{
    const char *verb;
    int otyp = mwep->otyp,
        /* (monsters don't actually wield towels, wet or otherwise) */
        lash = (objects[otyp].oc_skill == P_WHIP || is_wet_towel(mwep)),
        /* some weapons can have more than one strike type; for those,
           give a mix of thrust and swing (caller doesn't care either way) */
        thrust = ((objects[otyp].oc_dir & PIERCE) != 0
                  && ((objects[otyp].oc_dir & ~PIERCE) == 0 || !rn2(2)));

    verb = bash ? "bashes with" /*sigh*/
           : lash ? "lashes"
             : thrust ? "thrusts"
               : "swings";
    /* (might have caller also pass attacker's formatted name so that
       if hallucination makes that be plural, we could use vtense() to
       adjust the result to match) */
    return verb;
}

/* monster swings obj */
static void
mswings(
    struct monst *mtmp, /* attacker */
    struct obj *otemp,  /* attacker's weapon */
    boolean bash)       /* True: polearm used at too close range */
{
    if (Verbose(1, mswings) && !Blind && mon_visible(mtmp)) {
        pline("%s %s %s%s %s.", Monnam(mtmp), mswings_verb(otemp, bash),
              (otemp->quan > 1L) ? "one of " : "", mhis(mtmp), xname(otemp));
    }
}

/* return how a poison attack was delivered */
const char *
mpoisons_subj(struct monst *mtmp, struct attack *mattk)
{
    if (mattk->aatyp == AT_WEAP) {
        struct obj *mwep = (mtmp == &g.youmonst) ? uwep : MON_WEP(mtmp);
        /* "Foo's attack was poisoned." is pretty lame, but at least
           it's better than "sting" when not a stinging attack... */
        return (!mwep || !mwep->opoisoned) ? "attack" : "weapon";
    } else {
        return (mattk->aatyp == AT_TUCH) ? "contact"
                  : (mattk->aatyp == AT_GAZE) ? "gaze"
                       : (mattk->aatyp == AT_BITE) ? "bite" : "sting";
    }
}

/* called when your intrinsic speed is taken away */
void
u_slow_down(void)
{
    HFast = 0L;
    if (!Fast)
        You("slow down.");
    else /* speed boots */
        Your("quickness feels less natural.");
    exercise(A_DEX, FALSE);
}

/* monster attacked your displaced image */
static void
wildmiss(struct monst *mtmp, struct attack *mattk)
{
    int compat;
    const char *Monst_name; /* Monnam(mtmp) */

    /* no map_invisible() -- no way to tell where _this_ is coming from */

    if (!Verbose(1, wildmiss))
        return;
    if (!cansee(mtmp->mx, mtmp->my))
        return;
    /* maybe it's attacking an image around the corner? */

    compat = ((mattk->adtyp == AD_SEDU || mattk->adtyp == AD_SSEX)
              ? could_seduce(mtmp, &g.youmonst, mattk) : 0);
    Monst_name = Monnam(mtmp);

    if (!mtmp->mcansee || (Invis && !perceives(mtmp->data))) {
        const char *swings = (mattk->aatyp == AT_BITE) ? "snaps"
                             : (mattk->aatyp == AT_KICK) ? "kicks"
                               : (mattk->aatyp == AT_STNG
                                  || mattk->aatyp == AT_BUTT
                                  || nolimbs(mtmp->data)) ? "lunges"
                                 : "swings";

        if (compat)
            pline("%s tries to touch you and misses!", Monst_name);
        else
            switch (rn2(3)) {
            case 0:
                pline("%s %s wildly and misses!", Monst_name, swings);
                break;
            case 1:
                pline("%s attacks a spot beside you.", Monst_name);
                break;
            case 2:
                pline("%s strikes at %s!", Monst_name,
                      is_waterwall(mtmp->mux,mtmp->muy)
                        ? "empty water"
                        : "thin air");
                break;
            default:
                pline("%s %s wildly!", Monst_name, swings);
                break;
            }

    } else if (Displaced) {
        /* give 'displaced' message even if hero is Blind */
        if (compat)
            pline("%s smiles %s at your %sdisplaced image...", Monst_name,
                  (compat == 2) ? "engagingly" : "seductively",
                  Invis ? "invisible " : "");
        else
            pline("%s strikes at your %sdisplaced image and misses you!",
                  /* Note:  if you're both invisible and displaced, only
                   * monsters which see invisible will attack your displaced
                   * image, since the displaced image is also invisible. */
                  Monst_name, Invis ? "invisible " : "");

    } else if (Underwater) {
        /* monsters may miss especially on water level where
           bubbles shake the player here and there */
        if (compat)
            pline("%s reaches towards your distorted image.", Monst_name);
        else
            pline("%s is fooled by water reflections and misses!",
                  Monst_name);

    } else
        impossible("%s attacks you without knowing your location?",
                   Monst_name);
}

void
expels(
    struct monst *mtmp,
    struct permonst *mdat, /* if mtmp is polymorphed, mdat != mtmp->data */
    boolean message)
{
    g.context.botl = 1;
    if (message) {
        if (digests(mdat)) {
            You("get regurgitated!");
        } else if (enfolds(mdat)) {
            pline("%s unfolds and you are released!", Monnam(mtmp));
        } else {
            char blast[40];
            struct attack *attk = attacktype_fordmg(mdat, AT_ENGL, AD_ANY);

            blast[0] = '\0';
            if (!attk) {
                impossible("Swallower has no engulfing attack?");
            } else {
                if (is_whirly(mdat)) {
                    switch (attk->adtyp) {
                    case AD_ELEC:
                        Strcpy(blast, " in a shower of sparks");
                        break;
                    case AD_COLD:
                        Strcpy(blast, " in a blast of frost");
                        break;
                    }
                } else {
                    Strcpy(blast, " with a squelch");
                }
                You("get expelled from %s%s!", mon_nam(mtmp), blast);
            }
        }
    }
    unstuck(mtmp); /* ball&chain returned in unstuck() */
    mnexto(mtmp, RLOC_NOMSG);
    newsym(u.ux, u.uy);
    spoteffects(TRUE);
    /* to cover for a case where mtmp is not in a next square */
    if (um_dist(mtmp->mx, mtmp->my, 1))
        pline("Brrooaa...  You land hard at some distance.");
}

/* select a monster's next attack, possibly substituting for its usual one */
struct attack *
getmattk(struct monst *magr, struct monst *mdef,
         int indx, int prev_result[], struct attack *alt_attk_buf)
{
    struct permonst *mptr = magr->data;
    struct attack *attk = &mptr->mattk[indx];
    struct obj *weap = (magr == &g.youmonst) ? uwep : MON_WEP(magr);

    /* honor SEDUCE=0 */
    if (!SYSOPT_SEDUCE) {
        extern const struct attack c_sa_no[NATTK];

        /* if the first attack is for SSEX damage, all six attacks will be
           substituted (expected succubus/incubus handling); if it isn't
           but another one is, only that other one will be substituted */
        if (mptr->mattk[0].adtyp == AD_SSEX) {
            *alt_attk_buf = c_sa_no[indx];
            attk = alt_attk_buf;
        } else if (attk->adtyp == AD_SSEX) {
            *alt_attk_buf = *attk;
            attk = alt_attk_buf;
            attk->adtyp = AD_DRLI;
        }
    }

    /* prevent a monster with two consecutive disease or hunger attacks
       from hitting with both of them on the same turn; if the first has
       already hit, switch to a stun attack for the second */
    if (indx > 0 && prev_result[indx - 1] > MM_MISS
        && (attk->adtyp == AD_DISE || attk->adtyp == AD_PEST
            || attk->adtyp == AD_FAMN)
        && attk->adtyp == mptr->mattk[indx - 1].adtyp) {
        *alt_attk_buf = *attk;
        attk = alt_attk_buf;
        attk->adtyp = AD_STUN;

    /* make drain-energy damage be somewhat in proportion to energy */
    } else if (attk->adtyp == AD_DREN && mdef == &g.youmonst) {
        int ulev = max(u.ulevel, 6);

        *alt_attk_buf = *attk;
        attk = alt_attk_buf;
        /* 3.6.0 used 4d6 but since energy drain came out of max energy
           once current energy was gone, that tended to have a severe
           effect on low energy characters; it's now 2d6 with ajustments */
        if (u.uen <= 5 * ulev && attk->damn > 1) {
            attk->damn -= 1; /* low energy: 2d6 -> 1d6 */
            if (u.uenmax <= 2 * ulev && attk->damd > 3)
                attk->damd -= 3; /* very low energy: 1d6 -> 1d3 */
        } else if (u.uen > 12 * ulev) {
            attk->damn += 1; /* high energy: 2d6 -> 3d6 */
            if (u.uenmax > 20 * ulev)
                attk->damd += 3; /* very high energy: 3d6 -> 3d9 */
            /* note: 3d9 is slightly higher than previous 4d6 */
        }

    /* holders/engulfers who release the hero have mspec_used set to rnd(2)
       and can't re-hold/re-engulf until it has been decremented to zero */
    } else if (magr->mspec_used && (attk->aatyp == AT_ENGL
                                    || attk->aatyp == AT_HUGS
                                    || attk->adtyp == AD_STCK)) {
        boolean wimpy = (attk->damd == 0); /* lichen, violet fungus */

        /* can't re-engulf or re-grab yet; switch to simpler attack */
        *alt_attk_buf = *attk;
        attk = alt_attk_buf;
        if (attk->adtyp == AD_ACID || attk->adtyp == AD_ELEC
            || attk->adtyp == AD_COLD || attk->adtyp == AD_FIRE) {
            attk->aatyp = AT_TUCH;
        } else {
            attk->aatyp = AT_CLAW; /* attack message will be "<foo> hits" */
            attk->adtyp = AD_PHYS;
        }
        attk->damn = 1; /* relatively weak: 1d6 */
        attk->damd = 6;
        if (wimpy && attk->aatyp == AT_CLAW) {
            attk->aatyp = AT_TUCH;
            attk->damn = attk->damd = 0;
        }

    /* barrow wight, Nazgul, erinys have weapon attack for non-physical
       damage; force physical damage if attacker has been cancelled or
       if weapon is sufficiently interesting; a few unique creatures
       have two weapon attacks where one does physical damage and other
       doesn't--avoid forcing physical damage for those */
    } else if (indx == 0 && magr != &g.youmonst
               && attk->aatyp == AT_WEAP && attk->adtyp != AD_PHYS
               && !(mptr->mattk[1].aatyp == AT_WEAP
                    && mptr->mattk[1].adtyp == AD_PHYS)
               && (magr->mcan
                   || (weap && ((weap->otyp == CORPSE
                                 && touch_petrifies(&mons[weap->corpsenm]))
                                || is_art(weap, ART_STORMBRINGER)
                                || is_art(weap, ART_VORPAL_BLADE))))) {
        *alt_attk_buf = *attk;
        attk = alt_attk_buf;
        attk->adtyp = AD_PHYS;
    }
    return attk;
}

/*
 * mattacku: monster attacks you
 *      returns 1 if monster dies (e.g. "yellow light"), 0 otherwise
 *      Note: if you're displaced or invisible the monster might attack the
 *              wrong position...
 *      Assumption: it's attacking you or an empty square; if there's another
 *              monster which it attacks by mistake, the caller had better
 *              take care of it...
 */
int
mattacku(register struct monst *mtmp)
{
    struct attack *mattk, alt_attk;
    int i, j = 0, tmp, sum[NATTK];
    struct permonst *mdat = mtmp->data;
    /*
     * ranged: Is it near you?  Affects your actions.
     * ranged2: Does it think it's near you?  Affects its actions.
     * foundyou: Is it attacking you or your image?
     * youseeit: Can you observe the attack?  It might be attacking your
     *     image around the corner, or invisible, or you might be blind.
     * skipnonmagc: Are further physical attack attempts useless?  (After
     *     a wild miss--usually due to attacking displaced image.  Avoids
     *     excessively verbose miss feedback when monster can do multiple
     *     attacks and would miss the same wrong spot each time.)
     */
    boolean ranged = (distu(mtmp->mx, mtmp->my) > 3),
            range2 = !monnear(mtmp, mtmp->mux, mtmp->muy),
            foundyou = u_at(mtmp->mux, mtmp->muy),
            youseeit = canseemon(mtmp),
            skipnonmagc = FALSE;

    if (!ranged)
        nomul(0);
    if (DEADMONSTER(mtmp) || (Underwater && !is_swimmer(mtmp->data)))
        return 0;

    /* If swallowed, can only be affected by u.ustuck */
    if (u.uswallow) {
        if (mtmp != u.ustuck)
            return 0;
        u.ustuck->mux = u.ux;
        u.ustuck->muy = u.uy;
        range2 = 0;
        foundyou = 1;
        if (u.uinvulnerable)
            return 0; /* stomachs can't hurt you! */

    } else if (u.usteed) {
        if (mtmp == u.usteed)
            /* Your steed won't attack you */
            return 0;
        /* Orcs like to steal and eat horses and the like */
        if (!rn2(is_orc(mtmp->data) ? 2 : 4)
            && next2u(mtmp->mx, mtmp->my)) {
            /* Attack your steed instead */
            i = mattackm(mtmp, u.usteed);
            if ((i & MM_AGR_DIED))
                return 1;
            /* make sure steed is still alive and within range */
            if ((i & MM_DEF_DIED) || !u.usteed
                || !next2u(mtmp->mx, mtmp->my))
                return 0;
            /* Let your steed retaliate */
            return !!(mattackm(u.usteed, mtmp) & MM_DEF_DIED);
        }
    }

    if (u.uundetected && !range2 && foundyou && !u.uswallow) {
        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        u.uundetected = 0;
        if (is_hider(g.youmonst.data) && u.umonnum != PM_TRAPPER) {
            /* ceiling hider */
            coord cc; /* maybe we need a unexto() function? */
            struct obj *obj;

            You("fall from the %s!", ceiling(u.ux, u.uy));
            /* take monster off map now so that its location
               is eligible for placing hero; we assume that a
               removed monster remembers its old spot <mx,my> */
            remove_monster(mtmp->mx, mtmp->my);
            if (!enexto(&cc, u.ux, u.uy, g.youmonst.data)
                /* a fish won't voluntarily swap positions
                   when it's in water and hero is over land */
                || (mtmp->data->mlet == S_EEL
                    && is_pool(mtmp->mx, mtmp->my)
                    && !is_pool(u.ux, u.uy))) {
                /* couldn't find any spot for hero; this used to
                   kill off attacker, but now we just give a "miss"
                   message and keep both mtmp and hero at their
                   original positions; hero has become unconcealed
                   so mtmp's next move will be a regular attack */
                place_monster(mtmp, mtmp->mx, mtmp->my); /* put back */
                newsym(u.ux, u.uy); /* u.uundetected was toggled */
                pline("%s draws back as you drop!", Monnam(mtmp));
                return 0;
            }

            /* put mtmp at hero's spot and move hero to <cc.x,.y> */
            newsym(mtmp->mx, mtmp->my); /* finish removal */
            place_monster(mtmp, u.ux, u.uy);
            if (mtmp->wormno) {
                worm_move(mtmp);
                /* tail hasn't grown, so if it now occupies <cc.x,.y>
                   then one of its original spots must be free */
                if (m_at(cc.x, cc.y))
                    (void) enexto(&cc, u.ux, u.uy, g.youmonst.data);
            }
            teleds(cc.x, cc.y, TELEDS_ALLOW_DRAG); /* move hero */
            set_apparxy(mtmp);
            newsym(u.ux, u.uy);

            if (g.youmonst.data->mlet != S_PIERCER)
                return 0; /* lurkers don't attack */

            obj = which_armor(mtmp, WORN_HELMET);
            if (obj && is_metallic(obj)) {
                Your("blow glances off %s %s.", s_suffix(mon_nam(mtmp)),
                     helm_simple_name(obj));
            } else {
                if (3 + find_mac(mtmp) <= rnd(20)) {
                    pline("%s is hit by a falling piercer (you)!",
                          Monnam(mtmp));
                    if ((mtmp->mhp -= d(3, 6)) < 1)
                        killed(mtmp);
                } else
                    pline("%s is almost hit by a falling piercer (you)!",
                          Monnam(mtmp));
            }

        } else {
            /* surface hider */
            if (!youseeit) {
                pline("It tries to move where you are hiding.");
            } else {
                /* Ugly kludge for eggs.  The message is phrased so as
                 * to be directed at the monster, not the player,
                 * which makes "laid by you" wrong.  For the
                 * parallelism to work, we can't rephrase it, so we
                 * zap the "laid by you" momentarily instead.
                 */
                struct obj *obj = g.level.objects[u.ux][u.uy];

                if (obj || u.umonnum == PM_TRAPPER
                    || (g.youmonst.data->mlet == S_EEL
                        && is_pool(u.ux, u.uy))) {
                    int save_spe = 0; /* suppress warning */

                    if (obj) {
                        save_spe = obj->spe;
                        if (obj->otyp == EGG)
                            obj->spe = 0;
                    }
                    /* note that m_monnam() overrides hallucination, which is
                       what we want when message is from mtmp's perspective */
                    if (g.youmonst.data->mlet == S_EEL
                        || u.umonnum == PM_TRAPPER)
                        pline(
                             "Wait, %s!  There's a hidden %s named %s there!",
                              m_monnam(mtmp),
                              pmname(g.youmonst.data, Ugender), g.plname);
                    else
                        pline(
                          "Wait, %s!  There's a %s named %s hiding under %s!",
                              m_monnam(mtmp), pmname(g.youmonst.data, Ugender),
                              g.plname, doname(g.level.objects[u.ux][u.uy]));
                    if (obj)
                        obj->spe = save_spe;
                } else
                    impossible("hiding under nothing?");
            }
            newsym(u.ux, u.uy);
        }
        return 0;
    }

    /* hero might be a mimic, concealed via #monster */
    if (g.youmonst.data->mlet == S_MIMIC && U_AP_TYPE && !range2
        && foundyou && !u.uswallow) {
        boolean sticky = sticks(g.youmonst.data);

        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        if (sticky && !youseeit)
            pline("It gets stuck on you.");
        else /* see note about m_monnam() above */
            pline("Wait, %s!  That's a %s named %s!", m_monnam(mtmp),
                  pmname(g.youmonst.data, Ugender), g.plname);
        if (sticky)
            set_ustuck(mtmp);
        g.youmonst.m_ap_type = M_AP_NOTHING;
        g.youmonst.mappearance = 0;
        newsym(u.ux, u.uy);
        return 0;
    }

    /* non-mimic hero might be mimicking an object after eating m corpse */
    if (U_AP_TYPE == M_AP_OBJECT && !range2 && foundyou && !u.uswallow) {
        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        if (!youseeit)
            pline("%s %s!", Something, (likes_gold(mtmp->data)
                                      && g.youmonst.mappearance == GOLD_PIECE)
                                           ? "tries to pick you up"
                                           : "disturbs you");
        else /* see note about m_monnam() above */
            pline("Wait, %s!  That %s is really %s named %s!", m_monnam(mtmp),
                  mimic_obj_name(&g.youmonst),
                  an(pmname(&mons[u.umonnum], Ugender)), g.plname);
        if (g.multi < 0) { /* this should always be the case */
            char buf[BUFSZ];

            Sprintf(buf, "You appear to be %s again.",
                    Upolyd ? (const char *) an(pmname(g.youmonst.data,
                                                      flags.female))
                           : (const char *) "yourself");
            unmul(buf); /* immediately stop mimicking */
        }
        return 0;
    }

    /*  Work out the armor class differential   */
    tmp = AC_VALUE(u.uac) + 10; /* tmp ~= 0 - 20 */
    tmp += mtmp->m_lev;
    if (g.multi < 0)
        tmp += 4;
    if ((Invis && !perceives(mdat)) || !mtmp->mcansee)
        tmp -= 2;
    if (mtmp->mtrapped)
        tmp -= 2;
    if (tmp <= 0)
        tmp = 1;

    /* make eels visible the moment they hit/miss us */
    if (mdat->mlet == S_EEL && mtmp->minvis && cansee(mtmp->mx, mtmp->my)) {
        mtmp->minvis = 0;
        newsym(mtmp->mx, mtmp->my);
    }

    /* when not cancelled and not in current form due to shapechange, many
       demons can summon more demons and were creatures can summon critters;
       also, were creature might change from human to animal or vice versa */
    if (mtmp->cham == NON_PM && !mtmp->mcan && !range2
        && (is_demon(mdat) || is_were(mdat))) {
        summonmu(mtmp, youseeit);
        mdat = mtmp->data; /* update cached value in case of were change */
    }

    if (u.uinvulnerable) { /* in the midst of successful prayer */
        /* monsters won't attack you */
        if (mtmp == u.ustuck) {
            pline("%s loosens its grip slightly.", Monnam(mtmp));
        } else if (!range2) {
            if (youseeit || sensemon(mtmp))
                pline("%s starts to attack you, but pulls back.",
                      Monnam(mtmp));
            else
                You_feel("%s move nearby.", something);
        }
        return 0;
    }

    /* Unlike defensive stuff, don't let them use item _and_ attack. */
    if (find_offensive(mtmp)) {
        int foo = use_offensive(mtmp);

        if (foo != 0)
            return (foo == 1);
    }

    g.skipdrin = FALSE; /* [see mattackm(mhitm.c)] */

    for (i = 0; i < NATTK; i++) {
        sum[i] = MM_MISS;
        if (i > 0 && foundyou /* previous attack might have moved hero */
            && (mtmp->mux != u.ux || mtmp->muy != u.uy))
            continue; /* fill in sum[] with 'miss' but skip other actions */
        mon_currwep = (struct obj *) 0;
        mattk = getmattk(mtmp, &g.youmonst, i, sum, &alt_attk);
        if ((u.uswallow && mattk->aatyp != AT_ENGL)
            || (skipnonmagc && mattk->aatyp != AT_MAGC)
            || (g.skipdrin && mattk->aatyp == AT_TENT
                && mattk->adtyp == AD_DRIN))
            continue;

        switch (mattk->aatyp) {
        case AT_CLAW: /* "hand to hand" attacks */
        case AT_KICK:
        case AT_BITE:
        case AT_STNG:
        case AT_TUCH:
        case AT_BUTT:
        case AT_TENT:
            if (!range2 && (!MON_WEP(mtmp) || mtmp->mconf || Conflict
                            || !touch_petrifies(g.youmonst.data))) {
                if (foundyou) {
                    if (tmp > (j = rnd(20 + i))) {
                        if (mattk->aatyp != AT_KICK
                            || !thick_skinned(g.youmonst.data))
                            sum[i] = hitmu(mtmp, mattk);
                    } else
                        missmu(mtmp, (tmp == j), mattk);
                } else {
                    wildmiss(mtmp, mattk);
                    /* skip any remaining non-spell attacks */
                    skipnonmagc = TRUE;
                }
            }
            break;

        case AT_HUGS: /* automatic if prev two attacks succeed */
            /* Note: if displaced, prev attacks never succeeded */
            if ((!range2 && i >= 2 && sum[i - 1] && sum[i - 2])
                || mtmp == u.ustuck)
                sum[i] = hitmu(mtmp, mattk);
            break;

        case AT_GAZE: /* can affect you either ranged or not */
            /* Medusa gaze already operated through m_respond in
               dochug(); don't gaze more than once per round. */
            if (mdat != &mons[PM_MEDUSA])
                sum[i] = gazemu(mtmp, mattk);
            break;

        case AT_EXPL: /* automatic hit if next to, and aimed at you */
            if (!range2)
                sum[i] = explmu(mtmp, mattk, foundyou);
            break;

        case AT_ENGL:
            if (!range2) {
                if (foundyou) {
                    if (u.uswallow
                        || (!mtmp->mspec_used && tmp > (j = rnd(20 + i)))) {
                        /* force swallowing monster to be displayed
                           even when hero is moving away */
                        flush_screen(1);
                        sum[i] = gulpmu(mtmp, mattk);
                    } else {
                        missmu(mtmp, (tmp == j), mattk);
                    }
                } else if (digests(mtmp->data)) {
                    pline("%s gulps some air!", Monnam(mtmp));
                } else {
                    if (youseeit)
                        pline("%s lunges forward and recoils!", Monnam(mtmp));
                    else
                        You_hear("a %s nearby.",
                                 is_whirly(mtmp->data) ? "rushing noise"
                                                       : "splat");
                }
            }
            break;
        case AT_BREA:
            if (range2)
                sum[i] = breamu(mtmp, mattk);
            /* Note: breamu takes care of displacement */
            break;
        case AT_SPIT:
            if (range2)
                sum[i] = spitmu(mtmp, mattk);
            /* Note: spitmu takes care of displacement */
            break;
        case AT_WEAP:
            if (range2) {
                if (!Is_rogue_level(&u.uz))
                    thrwmu(mtmp);
            } else {
                int hittmp = 0;

                /* Rare but not impossible.  Normally the monster
                 * wields when 2 spaces away, but it can be
                 * teleported or whatever....
                 */
                if (mtmp->weapon_check == NEED_WEAPON || !MON_WEP(mtmp)) {
                    mtmp->weapon_check = NEED_HTH_WEAPON;
                    /* mon_wield_item resets weapon_check as appropriate */
                    if (mon_wield_item(mtmp) != 0)
                        break;
                }
                if (foundyou) {
                    mon_currwep = MON_WEP(mtmp);
                    if (mon_currwep) {
                        boolean bash = (is_pole(mon_currwep)
                                        && next2u(mtmp->mx, mtmp->my));

                        hittmp = hitval(mon_currwep, &g.youmonst);
                        tmp += hittmp;
                        mswings(mtmp, mon_currwep, bash);
                    }
                    if (tmp > (j = g.mhitu_dieroll = rnd(20 + i)))
                        sum[i] = hitmu(mtmp, mattk);
                    else
                        missmu(mtmp, (tmp == j), mattk);
                    /* KMH -- Don't accumulate to-hit bonuses */
                    if (mon_currwep)
                        tmp -= hittmp;
                } else {
                    wildmiss(mtmp, mattk);
                    /* skip any remaining non-spell attacks */
                    skipnonmagc = TRUE;
                }
            }
            break;
        case AT_MAGC:
            if (range2)
                sum[i] = buzzmu(mtmp, mattk);
            else
                sum[i] = castmu(mtmp, mattk, TRUE, foundyou);
            break;

        default: /* no attack */
            break;
        }
        if (g.context.botl)
            bot();
        /* give player a chance of waking up before dying -kaa */
        if (sum[i] == MM_HIT) { /* successful attack */
            if (u.usleep && u.usleep < g.moves && !rn2(10)) {
                g.multi = -1;
                g.nomovemsg = "The combat suddenly awakens you.";
            }
        }
        if ((sum[i] & MM_AGR_DIED))
            return 1; /* attacker dead */
        if ((sum[i] & MM_AGR_DONE))
            break; /* attacker teleported, no more attacks */
        /* sum[i] == 0: unsuccessful attack */
    }
    return 0;
}

/* monster summons help for its fight against hero */
static void
summonmu(struct monst *mtmp, boolean youseeit)
{
    struct permonst *mdat = mtmp->data;

    /*
     * Extracted from mattacku() to reduce clutter there.
     * Caller has verified that 'mtmp' hasn't been cancelled
     * and isn't a shapechanger.
     */

    if (is_demon(mdat)) {
        if (mdat != &mons[PM_BALROG] && mdat != &mons[PM_AMOROUS_DEMON]) {
            if (!rn2(13))
                (void) msummon(mtmp);
        }
        return; /* no such thing as a demon were creature, so we're done */
    }

    if (is_were(mdat)) {
        if (is_human(mdat)) { /* maybe switch to animal form */
            if (!rn2(5 - (night() * 2)))
                new_were(mtmp);
        } else { /* maybe switch to back human form */
            if (!rn2(30))
                new_were(mtmp);
        }
        mdat = mtmp->data; /* form change invalidates cached value */

        if (!rn2(10)) { /* maybe summon compatible critters */
            int numseen, numhelp;
            char buf[BUFSZ], genericwere[BUFSZ];

            Strcpy(genericwere, "creature");
            if (youseeit)
                pline("%s summons help!", Monnam(mtmp));
            numhelp = were_summon(mdat, FALSE, &numseen, genericwere);
            if (youseeit) {
                if (numhelp > 0) {
                    if (numseen == 0)
                        You_feel("hemmed in.");
                } else {
                    pline("But none comes.");
                }
            } else {
                const char *from_nowhere;

                if (!Deaf) {
                    pline("%s %s!", Something, makeplural(growl_sound(mtmp)));
                    from_nowhere = "";
                } else {
                    from_nowhere = " from nowhere";
                }
                if (numhelp > 0) {
                    if (numseen < 1) {
                        You_feel("hemmed in.");
                    } else {
                        if (numseen == 1)
                            Sprintf(buf, "%s appears", an(genericwere));
                        else
                            Sprintf(buf, "%s appear",
                                    makeplural(genericwere));
                        pline("%s%s!", upstart(buf), from_nowhere);
                    }
                } /* else no help came; but you didn't know it tried */
            }
        } /* summon critters */
        return;
    } /* were creature */
}

boolean
diseasemu(struct permonst *mdat)
{
    if (Sick_resistance) {
        You_feel("a slight illness.");
        return FALSE;
    } else {
        make_sick(Sick ? Sick / 3L + 1L : (long) rn1(ACURR(A_CON), 20),
                  mdat->pmnames[NEUTRAL], TRUE, SICK_NONVOMITABLE);
        return TRUE;
    }
}

/* check whether slippery clothing protects from hug or wrap attack */
boolean
u_slip_free(struct monst *mtmp, struct attack *mattk)
{
    struct obj *obj;

    /* greased armor does not protect against AT_ENGL+AD_WRAP */
    if (mattk->aatyp == AT_ENGL)
        return FALSE;

    obj = (uarmc ? uarmc : uarm);
    if (!obj)
        obj = uarmu;
    if (mattk->adtyp == AD_DRIN)
        obj = uarmh;

    /* if your cloak/armor is greased, monster slips off; this
       protection might fail (33% chance) when the armor is cursed */
    if (obj && (obj->greased || obj->otyp == OILSKIN_CLOAK)
        && (!obj->cursed || rn2(3))) {
        pline("%s %s your %s %s!", Monnam(mtmp),
              (mattk->adtyp == AD_WRAP) ? "slips off of"
                                        : "grabs you, but cannot hold onto",
              obj->greased ? "greased" : "slippery",
              /* avoid "slippery slippery cloak"
                 for undiscovered oilskin cloak */
              (obj->greased || objects[obj->otyp].oc_name_known)
                  ? xname(obj)
                  : cloak_simple_name(obj));

        if (obj->greased && !rn2(2)) {
            pline_The("grease wears off.");
            obj->greased = 0;
            update_inventory();
        }
        return TRUE;
    }
    return FALSE;
}

/* armor that sufficiently covers the body might be able to block magic */
int
magic_negation(struct monst *mon)
{
    struct obj *o;
    long wearmask;
    int armpro, mc = 0;
    boolean is_you = (mon == &g.youmonst),
            via_amul = FALSE,
            gotprot = is_you ? (EProtection != 0L)
                             /* high priests have innate protection */
                             : (mon->data == &mons[PM_HIGH_CLERIC]);

    for (o = is_you ? g.invent : mon->minvent; o; o = o->nobj) {
        /* a_can field is only applicable for armor (which must be worn) */
        if ((o->owornmask & W_ARMOR) != 0L) {
            armpro = objects[o->otyp].a_can;
            if (armpro > mc)
                mc = armpro;
        } else if ((o->owornmask & W_AMUL) != 0L) {
            via_amul = (o->otyp == AMULET_OF_GUARDING);
        }
        /* if we've already confirmed Protection, skip additional checks */
        if (is_you || gotprot)
            continue;

        /* omit W_SWAPWEP+W_QUIVER; W_ART+W_ARTI handled by protects() */
        wearmask = W_ARMOR | W_ACCESSORY;
        if (o->oclass == WEAPON_CLASS || is_weptool(o))
            wearmask |= W_WEP;
        if (protects(o, ((o->owornmask & wearmask) != 0L) ? TRUE : FALSE))
            gotprot = TRUE;
    }

    if (gotprot) {
        /* extrinsic Protection increases mc by 1 (2 for amulet of guarding);
           multiple sources don't provide multiple increments */
        mc += via_amul ? 2 : 1;
        if (mc > 3)
            mc = 3;
    } else if (mc < 1) {
        /* intrinsic Protection is weaker (play balance; obtaining divine
           protection is too easy); it confers minimum mc 1 instead of 0 */
        if ((is_you && ((HProtection && u.ublessed > 0) || u.uspellprot))
            /* aligned priests and angels have innate intrinsic Protection */
            || (mon->data == &mons[PM_ALIGNED_CLERIC] || is_minion(mon->data)))
            mc = 1;
    }
    return mc;
}

/*
 * hitmu: monster hits you
 * returns MM_ flags
*/
static int
hitmu(register struct monst *mtmp, register struct attack *mattk)
{
    struct permonst *mdat = mtmp->data;
    struct permonst *olduasmon = g.youmonst.data;
    int res;
    struct mhitm_data mhm;
    mhm.hitflags = MM_MISS;
    mhm.permdmg = 0;
    mhm.specialdmg = 0;
    mhm.done = FALSE;

    if (!canspotmon(mtmp))
        map_invisible(mtmp->mx, mtmp->my);

    /*  If the monster is undetected & hits you, you should know where
     *  the attack came from.
     */
    if (mtmp->mundetected && (hides_under(mdat) || mdat->mlet == S_EEL)) {
        mtmp->mundetected = 0;
        if (!tp_sensemon(mtmp) && !Detect_monsters) {
            struct obj *obj;
            const char *what;
            char Amonbuf[BUFSZ];

            if ((obj = g.level.objects[mtmp->mx][mtmp->my]) != 0) {
                if (Blind && !obj->dknown)
                    what = something;
                else if (is_pool(mtmp->mx, mtmp->my) && !Underwater)
                    what = "the water";
                else
                    what = doname(obj);

                Strcpy(Amonbuf, Amonnam(mtmp));
                /* mtmp might be invisible with hero unable to see same */
                if (!strcmp(Amonbuf, "It")) /* note: not strcmpi() */
                    Strcpy(Amonbuf, Something);
                pline("%s was hidden under %s!", Amonbuf, what);
            }
            newsym(mtmp->mx, mtmp->my);
        }
    }

    /*  First determine the base damage done */
    mhm.damage = d((int) mattk->damn, (int) mattk->damd);
    if ((is_undead(mdat) || is_vampshifter(mtmp)) && midnight())
        mhm.damage += d((int) mattk->damn, (int) mattk->damd); /* extra dmg */

    mhitm_adtyping(mtmp, mattk, &g.youmonst, &mhm);

    (void) mhitm_knockback(mtmp, &g.youmonst, mattk, &mhm.hitflags, (MON_WEP(mtmp) != 0));

    if (mhm.done)
        return mhm.hitflags;

    if ((Upolyd ? u.mh : u.uhp) < 1) {
        /* already dead? call rehumanize() or done_in_by() as appropriate */
        mdamageu(mtmp, 1);
        mhm.damage = 0;
    }

    /*  Negative armor class reduces damage done instead of fully protecting
     *  against hits.
     */
    if (mhm.damage && u.uac < 0) {
        mhm.damage -= rnd(-u.uac);
        if (mhm.damage < 1)
            mhm.damage = 1;
    }

    if (mhm.damage) {
        if (Half_physical_damage
            /* Mitre of Holiness, even if not currently blessed */
            || (Role_if(PM_CLERIC) && uarmh && is_quest_artifact(uarmh)
                && mon_hates_blessings(mtmp)))
            mhm.damage = (mhm.damage + 1) / 2;

        if (mhm.permdmg) { /* Death's life force drain */
            int lowerlimit, *hpmax_p;
            /*
             * Apply some of the damage to permanent hit points:
             *  polymorphed         100% against poly'd hpmax
             *  hpmax > 25*lvl      100% against normal hpmax
             *  hpmax > 10*lvl  50..100%
             *  hpmax >  5*lvl  25..75%
             *  otherwise        0..50%
             * Never reduces hpmax below 1 hit point per level.
             */
            mhm.permdmg = rn2(mhm.damage / 2 + 1);
            if (Upolyd || u.uhpmax > 25 * u.ulevel)
                mhm.permdmg = mhm.damage;
            else if (u.uhpmax > 10 * u.ulevel)
                mhm.permdmg += mhm.damage / 2;
            else if (u.uhpmax > 5 * u.ulevel)
                mhm.permdmg += mhm.damage / 4;

            if (Upolyd) {
                hpmax_p = &u.mhmax;
                /* [can't use g.youmonst.m_lev] */
                lowerlimit = min((int) g.youmonst.data->mlevel, u.ulevel);
            } else {
                hpmax_p = &u.uhpmax;
                lowerlimit = minuhpmax(1);
            }
            if (*hpmax_p - mhm.permdmg > lowerlimit)
                *hpmax_p -= mhm.permdmg;
            else if (*hpmax_p > lowerlimit)
                *hpmax_p = lowerlimit;
            /* else unlikely...
             * already at or below minimum threshold; do nothing */
            g.context.botl = 1;
        }

        mdamageu(mtmp, mhm.damage);
    }

    if (mhm.damage)
        res = passiveum(olduasmon, mtmp, mattk);
    else
        res = MM_HIT;
    stop_occupation();
    return res;
}

/* An interface for use when taking a blindfold off, for example,
 * to see if an engulfing attack should immediately take affect, like
 * a passive attack. TRUE if engulfing blindness occurred */
boolean
gulp_blnd_check(void)
{
    struct attack *mattk;

    if (!Blinded && u.uswallow
        && (mattk = attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND))
        && can_blnd(u.ustuck, &g.youmonst, mattk->aatyp, (struct obj *) 0)) {
        ++u.uswldtim; /* compensate for gulpmu change */
        (void) gulpmu(u.ustuck, mattk);
        return TRUE;
    }
    return FALSE;
}

/* monster swallows you, or damage if u.uswallow */
static int
gulpmu(struct monst *mtmp, struct attack *mattk)
{
    struct trap *t = t_at(u.ux, u.uy);
    int tmp = d((int) mattk->damn, (int) mattk->damd);
    int tim_tmp;
    struct obj *otmp2;
    int i;
    boolean physical_damage = FALSE;

    if (!u.uswallow) { /* swallows you */
        int omx = mtmp->mx, omy = mtmp->my;

        if (!engulf_target(mtmp, &g.youmonst))
            return MM_MISS;
        if ((t && is_pit(t->ttyp)) && sobj_at(BOULDER, u.ux, u.uy))
            return MM_MISS;

        if (Punished)
            unplacebc(); /* ball&chain go away */
        remove_monster(omx, omy);
        mtmp->mtrapped = 0; /* no longer on old trap */
        place_monster(mtmp, u.ux, u.uy);
        set_ustuck(mtmp);
        newsym(mtmp->mx, mtmp->my);
        /* 3.7: dismount for all engulfers, not just for purple worms */
        if (u.usteed) {
            char buf[BUFSZ];

            Strcpy(buf, mon_nam(u.usteed));
            urgent_pline("%s %s forward and plucks you off %s!",
                         Some_Monnam(mtmp),
                         /* 't', purple 'w' */
                         is_animal(mtmp->data) ? "lunges"
                           /* 'v', air 'E' */
                           : is_whirly(mtmp->data) ? "whirls"
                             /* none (some 'v', already whirling) */
                             : unsolid(mtmp->data) ? "flows"
                               /* ochre 'j', Juiblex */
                               : amorphous(mtmp->data) ? "oozes"
                                 /* none (all AT_ENGL are already covered) */
                                 : "surges",
                         buf);
            dismount_steed(DISMOUNT_ENGULFED);
        } else {
            urgent_pline("%s %s!", Monnam(mtmp),
                         digests(mtmp->data) ? "swallows you whole"
                           : "engulfs you");
        }
        stop_occupation();
        reset_occupations(); /* behave as if you had moved */

        if (u.utrap) {
            You("are released from the %s!",
                u.utraptype == TT_WEB ? "web" : "trap");
            reset_utrap(FALSE);
        }

        i = number_leashed();
        if (i > 0) {
            const char *s = (i > 1) ? "leashes" : "leash";

            pline_The("%s %s loose.", s, vtense(s, "snap"));
            unleash_all();
        }

        if (touch_petrifies(g.youmonst.data) && !resists_ston(mtmp)) {
            /* put the attacker back where it started;
               the resulting statue will end up there
               [note: if poly'd hero could ride or non-poly'd hero could
               acquire touch_petrifies() capability somehow, this code
               would need to deal with possibility of steed having taken
               engulfer's previous spot when hero was forcibly dismounted] */
            remove_monster(mtmp->mx, mtmp->my); /* u.ux,u.uy */
            place_monster(mtmp, omx, omy);
            minstapetrify(mtmp, TRUE);
            /* normally unstuck() would do this, but we're not
               fully swallowed yet so that won't work here */
            if (Punished)
                placebc();
            set_ustuck((struct monst *) 0);
            return (!DEADMONSTER(mtmp)) ? MM_MISS : MM_AGR_DIED;
        }

        display_nhwindow(WIN_MESSAGE, FALSE);
        vision_recalc(2); /* hero can't see anything */
        u.uswallow = 1;
        /* for digestion, shorter time is more dangerous;
           for other swallowings, longer time means more
           chances for the swallower to attack */
        if (mattk->adtyp == AD_DGST) {
            tim_tmp = 25 - (int) mtmp->m_lev;
            if (tim_tmp > 0)
                tim_tmp = rnd(tim_tmp) / 2;
            else if (tim_tmp < 0)
                tim_tmp = -(rnd(-tim_tmp) / 2);
            /* having good armor & high constitution makes
               it take longer for you to be digested, but
               you'll end up trapped inside for longer too */
            tim_tmp += -u.uac + 10 + (ACURR(A_CON) / 3 - 1);
        } else {
            /* higher level attacker takes longer to eject hero */
            tim_tmp = rnd((int) mtmp->m_lev + 10 / 2);
        }
        /* u.uswldtim always set > 1 */
        u.uswldtim = (unsigned) ((tim_tmp < 2) ? 2 : tim_tmp);
        swallowed(1); /* update the map display, shows hero swallowed */
        if (!flaming(mtmp->data)) {
            for (otmp2 = g.invent; otmp2; otmp2 = otmp2->nobj)
                (void) snuff_lit(otmp2);
        }
    }

    if (mtmp != u.ustuck)
        return MM_MISS;
    if (Punished) {
        /* ball&chain are in limbo while swallowed; update their internal
           location to be at swallower's spot */
        if (uchain->where == OBJ_FREE)
            uchain->ox = mtmp->mx, uchain->oy = mtmp->my;
        if (uball->where == OBJ_FREE)
            uball->ox = mtmp->mx, uball->oy = mtmp->my;
    }
    if (u.uswldtim > 0)
        u.uswldtim -= 1;

    switch (mattk->adtyp) {
    case AD_DGST:
        physical_damage = TRUE;
        if (Slow_digestion) {
            /* Messages are handled below */
            u.uswldtim = 0;
            tmp = 0;
        } else if (u.uswldtim == 0) {
            pline("%s totally digests you!", Monnam(mtmp));
            tmp = u.uhp;
            if (Half_physical_damage)
                tmp *= 2; /* sorry */
        } else {
            pline("%s%s digests you!", Monnam(mtmp),
                  (u.uswldtim == 2) ? " thoroughly"
                                    : (u.uswldtim == 1) ? " utterly" : "");
            exercise(A_STR, FALSE);
        }
        break;
    case AD_PHYS:
        physical_damage = TRUE;
        if (mtmp->data == &mons[PM_FOG_CLOUD]) {
            You("are laden with moisture and %s",
                flaming(g.youmonst.data)
                    ? "are smoldering out!"
                    : Breathless ? "find it mildly uncomfortable."
                                 : amphibious(g.youmonst.data)
                                       ? "feel comforted."
                                       : "can barely breathe!");
            /* NB: Amphibious includes Breathless */
            if (Amphibious && !flaming(g.youmonst.data))
                tmp = 0;
        } else {
            You("are %s!", enfolds(mtmp->data) ? "being squashed"
                                               : "pummeled with debris");
            exercise(A_STR, FALSE);
        }
        break;
    case AD_ACID:
        if (Acid_resistance) {
            You("are covered with a seemingly harmless goo.");
            monstseesu(M_SEEN_ACID);
            tmp = 0;
        } else {
            if (Hallucination)
                pline("Ouch!  You've been slimed!");
            else
                You("are covered in slime!  It burns!");
            exercise(A_STR, FALSE);
        }
        break;
    case AD_BLND:
        if (can_blnd(mtmp, &g.youmonst, mattk->aatyp, (struct obj *) 0)) {
            if (!Blind) {
                long was_blinded = Blinded;

                if (!Blinded)
                    You_cant("see in here!");
                make_blinded((long) tmp, FALSE);
                if (!was_blinded && !Blind)
                    Your1(vision_clears);
            } else
                /* keep him blind until disgorged */
                make_blinded(Blinded + 1, FALSE);
        }
        tmp = 0;
        break;
    case AD_ELEC:
        if (!mtmp->mcan && rn2(2)) {
            pline_The("air around you crackles with electricity.");
            if (Shock_resistance) {
                shieldeff(u.ux, u.uy);
                You("seem unhurt.");
                monstseesu(M_SEEN_ELEC);
                ugolemeffects(AD_ELEC, tmp);
                tmp = 0;
            }
        } else
            tmp = 0;
        break;
    case AD_COLD:
        if (!mtmp->mcan && rn2(2)) {
            if (Cold_resistance) {
                shieldeff(u.ux, u.uy);
                You_feel("mildly chilly.");
                monstseesu(M_SEEN_COLD);
                ugolemeffects(AD_COLD, tmp);
                tmp = 0;
            } else
                You("are freezing to death!");
        } else
            tmp = 0;
        break;
    case AD_FIRE:
        if (!mtmp->mcan && rn2(2)) {
            if (Fire_resistance) {
                shieldeff(u.ux, u.uy);
                You_feel("mildly hot.");
                monstseesu(M_SEEN_FIRE);
                ugolemeffects(AD_FIRE, tmp);
                tmp = 0;
            } else
                You("are burning to a crisp!");
            burn_away_slime();
        } else
            tmp = 0;
        break;
    case AD_DISE:
        if (!diseasemu(mtmp->data))
            tmp = 0;
        break;
    case AD_DREN:
        /* AC magic cancellation doesn't help when engulfed */
        if (!mtmp->mcan && rn2(4)) /* 75% chance */
            drain_en(tmp);
        tmp = 0;
        break;
    default:
        physical_damage = TRUE;
        tmp = 0;
        break;
    }

    if (physical_damage)
        tmp = Maybe_Half_Phys(tmp);

    mdamageu(mtmp, tmp);
    if (tmp)
        stop_occupation();

    if (!u.uswallow) {
        ; /* life-saving has already expelled swallowed hero */
    } else if (touch_petrifies(g.youmonst.data) && !resists_ston(mtmp)) {
        pline("%s very hurriedly %s you!", Monnam(mtmp),
              digests(mtmp->data) ? "regurgitates"
              : enfolds(mtmp->data) ? "releases"
                : "expels");
        expels(mtmp, mtmp->data, FALSE);
    } else if (!u.uswldtim || g.youmonst.data->msize >= MZ_HUGE) {
        /* As of 3.6.2: u.uswldtim used to be set to 0 by life-saving but it
           expels now so the !u.uswldtim case is no longer possible;
           however, polymorphing into a huge form while already
           swallowed is still possible */
        You("get %s!", digests(mtmp->data) ? "regurgitated"
                       : enfolds(mtmp->data) ? "released"
                         : "expelled");
        if (Verbose(1, gulpmu)
            && (digests(mtmp->data) && Slow_digestion))
            pline("Obviously %s doesn't like your taste.", mon_nam(mtmp));
        expels(mtmp, mtmp->data, FALSE);
    }
    return MM_HIT;
}

/* monster explodes in your face */
static int
explmu(struct monst *mtmp, struct attack *mattk, boolean ufound)
{
    boolean kill_agr = TRUE;
    boolean not_affected;
    int tmp;

    if (mtmp->mcan)
        return MM_MISS;

    tmp = d((int) mattk->damn, (int) mattk->damd);
    not_affected = defended(mtmp, (int) mattk->adtyp);

    if (!ufound) {
        pline("%s explodes at a spot in %s!",
              canseemon(mtmp) ? Monnam(mtmp) : "It",
              is_waterwall(mtmp->mux,mtmp->muy) ? "empty water"
                                                : "thin air");
    } else {
        hitmsg(mtmp, mattk);
    }

    switch (mattk->adtyp) {
    case AD_COLD:
    case AD_FIRE:
    case AD_ELEC:
        mon_explodes(mtmp, mattk);
        if (!DEADMONSTER(mtmp))
            kill_agr = FALSE; /* lifesaving? */
        break;
    case AD_BLND:
        not_affected = resists_blnd(&g.youmonst);
        if (ufound && !not_affected) {
            /* sometimes you're affected even if it's invisible */
            if (mon_visible(mtmp) || (rnd(tmp /= 2) > u.ulevel)) {
                You("are blinded by a blast of light!");
                make_blinded((long) tmp, FALSE);
                if (!Blind)
                    Your1(vision_clears);
            } else if (Verbose(1, explmu))
                You("get the impression it was not terribly bright.");
        }
        break;
    case AD_HALU:
        not_affected |= Blind || (u.umonnum == PM_BLACK_LIGHT
                                  || u.umonnum == PM_VIOLET_FUNGUS
                                  || dmgtype(g.youmonst.data, AD_STUN));
        if (ufound && !not_affected) {
            boolean chg;
            if (!Hallucination)
                You("are caught in a blast of kaleidoscopic light!");
            /* avoid hallucinating the black light as it dies */
            mondead(mtmp);    /* remove it from map now */
            kill_agr = FALSE; /* already killed (maybe lifesaved) */
            chg =
                make_hallucinated(HHallucination + (long) tmp, FALSE, 0L);
            You("%s.", chg ? "are freaked out" : "seem unaffected");
        }
        break;
    default:
        impossible("unknown exploder damage type %d", mattk->adtyp);
        break;
    }
    if (not_affected) {
        You("seem unaffected by it.");
        ugolemeffects((int) mattk->adtyp, tmp);
    }
    if (kill_agr && !DEADMONSTER(mtmp))
        mondead(mtmp);
    wake_nearto(mtmp->mx, mtmp->my, 7 * 7);
    return (!DEADMONSTER(mtmp)) ? MM_MISS : MM_AGR_DIED;
}

/* monster gazes at you */
int
gazemu(struct monst *mtmp, struct attack *mattk)
{
    static const char *const reactions[] = {
        "confused",              /* [0] */
        "stunned",               /* [1] */
        "puzzled",   "dazzled",  /* [2,3] */
        "irritated", "inflamed", /* [4,5] */
        "tired",                 /* [6] */
        "dulled",                /* [7] */
    };
    int react = -1;
    boolean cancelled = (mtmp->mcan != 0), already = FALSE;
    boolean mcanseeu = canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)
        && mtmp->mcansee;

    if (m_seenres(mtmp, cvt_adtyp_to_mseenres(mattk->adtyp)))
        return MM_MISS;

    /* assumes that hero has to see monster's gaze in order to be
       affected, rather than monster just having to look at hero;
       when hallucinating, hero's brain doesn't register what
       it's seeing correctly so the gaze is usually ineffective
       [this could be taken a lot farther and select a gaze effect
       appropriate to what's currently being displayed, giving
       ordinary monsters a gaze attack when hero thinks he or she
       is facing a gazing creature, but let's not go that far...] */
    if (Hallucination && rn2(4))
        cancelled = TRUE;

    switch (mattk->adtyp) {
    case AD_STON:
        if (cancelled || !mtmp->mcansee) {
            if (!canseemon(mtmp))
                break; /* silently */
            pline("%s %s.", Monnam(mtmp),
                  (mtmp->data == &mons[PM_MEDUSA] && mtmp->mcan)
                      ? "doesn't look all that ugly"
                      : "gazes ineffectually");
            break;
        }
        if (Reflecting && couldsee(mtmp->mx, mtmp->my)
            && mtmp->data == &mons[PM_MEDUSA]) {
            /* hero has line of sight to Medusa and she's not blind */
            boolean useeit = canseemon(mtmp);

            if (useeit)
                (void) ureflects("%s gaze is reflected by your %s.",
                                 s_suffix(Monnam(mtmp)));
            if (mon_reflects(
                    mtmp, !useeit ? (char *) 0
                                  : "The gaze is reflected away by %s %s!"))
                break;
            if (!m_canseeu(mtmp)) { /* probably you're invisible */
                if (useeit)
                    pline(
                      "%s doesn't seem to notice that %s gaze was reflected.",
                          Monnam(mtmp), mhis(mtmp));
                break;
            }
            if (useeit)
                pline("%s is turned to stone!", Monnam(mtmp));
            g.stoned = TRUE;
            killed(mtmp);

            if (!DEADMONSTER(mtmp))
                break;
            return MM_AGR_DIED;
        }
        if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)
            && !Stone_resistance) {
            You("meet %s gaze.", s_suffix(mon_nam(mtmp)));
            stop_occupation();
            if (poly_when_stoned(g.youmonst.data) && polymon(PM_STONE_GOLEM))
                break;
            urgent_pline("You turn to stone...");
            g.killer.format = KILLED_BY;
            Strcpy(g.killer.name, pmname(mtmp->data, Mgender(mtmp)));
            done(STONING);
        }
        break;
    case AD_CONF:
        if (mcanseeu && !mtmp->mspec_used && rn2(5)) {
            if (cancelled) {
                react = 0; /* "confused" */
                already = (mtmp->mconf != 0);
            } else {
                int conf = d(3, 4);

                mtmp->mspec_used = mtmp->mspec_used + (conf + rn2(6));
                if (!Confusion)
                    pline("%s gaze confuses you!", s_suffix(Monnam(mtmp)));
                else
                    You("are getting more and more confused.");
                make_confused(HConfusion + conf, FALSE);
                stop_occupation();
            }
        }
        break;
    case AD_STUN:
        if (mcanseeu && !mtmp->mspec_used && rn2(5)) {
            if (cancelled) {
                react = 1; /* "stunned" */
                already = (mtmp->mstun != 0);
            } else {
                int stun = d(2, 6);

                mtmp->mspec_used = mtmp->mspec_used + (stun + rn2(6));
                pline("%s stares piercingly at you!", Monnam(mtmp));
                make_stunned((HStun & TIMEOUT) + (long) stun, TRUE);
                stop_occupation();
            }
        }
        break;
    case AD_BLND:
        if (canseemon(mtmp) && !resists_blnd(&g.youmonst)
            && distu(mtmp->mx, mtmp->my) <= BOLT_LIM * BOLT_LIM) {
            if (cancelled) {
                react = rn1(2, 2); /* "puzzled" || "dazzled" */
                already = (mtmp->mcansee == 0);
                /* Archons gaze every round; we don't want cancelled ones
                   giving the "seems puzzled/dazzled" message that often */
                if (mtmp->mcan && mtmp->data == &mons[PM_ARCHON] && rn2(5))
                    react = -1;
            } else {
                int blnd = d((int) mattk->damn, (int) mattk->damd);

                You("are blinded by %s radiance!", s_suffix(mon_nam(mtmp)));
                make_blinded((long) blnd, FALSE);
                stop_occupation();
                /* not blind at this point implies you're wearing
                   the Eyes of the Overworld; make them block this
                   particular stun attack too */
                if (!Blind) {
                    Your1(vision_clears);
                } else {
                    long oldstun = (HStun & TIMEOUT), newstun = (long) rnd(3);

                    /* we don't want to increment stun duration every time
                       or sighted hero will become incapacitated */
                    make_stunned(max(oldstun, newstun), TRUE);
                }
            }
        }
        break;
    case AD_FIRE:
        if (mcanseeu && !mtmp->mspec_used && rn2(5)) {
            if (cancelled) {
                react = rn1(2, 4); /* "irritated" || "inflamed" */
            } else {
                int dmg = d(2, 6), lev = (int) mtmp->m_lev;

                pline("%s attacks you with a fiery gaze!", Monnam(mtmp));
                stop_occupation();
                if (Fire_resistance) {
                    shieldeff(u.ux, u.uy);
                    pline_The("fire doesn't feel hot!");
                    monstseesu(M_SEEN_FIRE);
                    ugolemeffects(AD_FIRE, d(12, 6));
                    dmg = 0;
                }
                burn_away_slime();
                if (lev > rn2(20))
                    (void) burnarmor(&g.youmonst);
                if (lev > rn2(20))
                    destroy_item(SCROLL_CLASS, AD_FIRE);
                if (lev > rn2(20))
                    destroy_item(POTION_CLASS, AD_FIRE);
                if (lev > rn2(25))
                    destroy_item(SPBOOK_CLASS, AD_FIRE);
                if (lev > rn2(20))
                    ignite_items(g.invent);
                if (dmg)
                    mdamageu(mtmp, dmg);
            }
        }
        break;
#ifdef PM_BEHOLDER /* work in progress */
    case AD_SLEE:
        if (mcanseeu && g.multi >= 0 && !rn2(5) && !Sleep_resistance) {
            if (cancelled) {
                react = 6;                      /* "tired" */
                already = (mtmp->mfrozen != 0); /* can't happen... */
            } else {
                fall_asleep(-rnd(10), TRUE);
                pline("%s gaze makes you very sleepy...",
                      s_suffix(Monnam(mtmp)));
            }
        }
        break;
    case AD_SLOW:
        if (mcanseeu
            && (HFast & (INTRINSIC | TIMEOUT)) && !defended(mtmp, AD_SLOW)
            && !rn2(4)) {
            if (cancelled) {
                react = 7; /* "dulled" */
                already = (mtmp->mspeed == MSLOW);
            } else {
                u_slow_down();
                stop_occupation();
            }
        }
        break;
#endif /* BEHOLDER */
    default:
        impossible("Gaze attack %d?", mattk->adtyp);
        break;
    }
    if (react >= 0) {
        if (Hallucination && rn2(3))
            react = rn2(SIZE(reactions));
        /* cancelled/hallucinatory feedback; monster might look "confused",
           "stunned",&c but we don't actually set corresponding attribute */
        pline("%s looks %s%s.", Monnam(mtmp),
              !rn2(3) ? "" : already ? "quite "
                                     : (!rn2(2) ? "a bit " : "somewhat "),
              reactions[react]);
    }
    return MM_MISS;
}

/* mtmp hits you for n points damage */
void
mdamageu(struct monst *mtmp, int n)
{
    g.context.botl = 1;
    if (Upolyd) {
        u.mh -= n;
        if (u.mh < 1)
            rehumanize();
    } else {
        u.uhp -= n;
        if (u.uhp < 1)
            done_in_by(mtmp, DIED);
    }
}

/* returns 0 if seduction impossible,
 *         1 if fine,
 *         2 if wrong gender for nymph
 */
int
could_seduce(
    struct monst *magr, struct monst *mdef,
    struct attack *mattk) /* non-Null: current atk; Null: general capability */
{
    struct permonst *pagr;
    boolean agrinvis, defperc;
    xint16 genagr, gendef;
    int adtyp;

    if (is_animal(magr->data))
        return 0;
    if (magr == &g.youmonst) {
        pagr = g.youmonst.data;
        agrinvis = (Invis != 0);
        genagr = poly_gender();
    } else {
        pagr = magr->data;
        agrinvis = magr->minvis;
        genagr = gender(magr);
    }
    if (mdef == &g.youmonst) {
        defperc = (See_invisible != 0);
        gendef = poly_gender();
    } else {
        defperc = perceives(mdef->data);
        gendef = gender(mdef);
    }

    adtyp = mattk ? mattk->adtyp
            : dmgtype(pagr, AD_SSEX) ? AD_SSEX
              : dmgtype(pagr, AD_SEDU) ? AD_SEDU
                : AD_PHYS;
    if (adtyp == AD_SSEX && !SYSOPT_SEDUCE)
        adtyp = AD_SEDU;

    if (agrinvis && !defperc && adtyp == AD_SEDU)
        return 0;

    /* nymphs have two attacks, one for steal-item damage and the other
       for seduction, both pass the could_seduce() test;
       incubi/succubi have three attacks, their claw attacks for damage
       don't pass the test */
    if ((pagr->mlet != S_NYMPH && pagr != &mons[PM_AMOROUS_DEMON])
        || (adtyp != AD_SEDU && adtyp != AD_SSEX && adtyp != AD_SITM))
        return 0;

    return (genagr == 1 - gendef) ? 1 : (pagr->mlet == S_NYMPH) ? 2 : 0;
}

/* returns 1 if monster teleported (or hero leaves monster's vicinity) */
int
doseduce(struct monst *mon)
{
    struct obj *ring, *nring;
    boolean fem = (mon->data == &mons[PM_AMOROUS_DEMON]
                   && Mgender(mon) == FEMALE); /* otherwise incubus */
    boolean seewho, naked; /* True iff no armor */
    int attr_tot, tried_gloves = 0;
    char qbuf[QBUFSZ], Who[QBUFSZ];

    if (mon->mcan || mon->mspec_used) {
        pline("%s acts as though %s has got a %sheadache.", Monnam(mon),
              mhe(mon), mon->mcan ? "severe " : "");
        return 0;
    }
    if (unconscious()) {
        pline("%s seems dismayed at your lack of response.", Monnam(mon));
        return 0;
    }
    seewho = canseemon(mon);
    if (!seewho)
        pline("Someone caresses you...");
    else
        You_feel("very attracted to %s.", mon_nam(mon));
    /* cache the seducer's name in a local buffer */
    Strcpy(Who, (!seewho ? (fem ? "She" : "He") : Monnam(mon)));

    /* if in the process of putting armor on or taking armor off,
       interrupt that activity now */
    (void) stop_donning((struct obj *) 0);
    /* don't try to take off gloves if cursed weapon blocks them */
    if (welded(uwep))
        tried_gloves = 1;

    for (ring = g.invent; ring; ring = nring) {
        nring = ring->nobj;
        if (ring->otyp != RIN_ADORNMENT)
            continue;
        if (fem) {
            if (ring->owornmask && uarmg) {
                /* don't take off worn ring if gloves are in the way */
                if (!tried_gloves++)
                    mayberem(mon, Who, uarmg, "gloves");
                if (uarmg)
                    continue; /* next ring might not be worn */
            }
            /* confirmation prompt when charisma is high bypassed if deaf */
            if (!Deaf && rn2(20) < ACURR(A_CHA)) {
                (void) safe_qbuf(qbuf, "\"That ",
                                 " looks pretty.  May I have it?\"", ring,
                                 xname, simpleonames, "ring");
                makeknown(RIN_ADORNMENT);
                if (yn(qbuf) == 'n')
                    continue;
            } else
                pline("%s decides she'd like %s, and takes it.",
                      Who, yname(ring));
            makeknown(RIN_ADORNMENT);
            /* might be in left or right ring slot or weapon/alt-wep/quiver */
            if (ring->owornmask)
                remove_worn_item(ring, FALSE);
            freeinv(ring);
            (void) mpickobj(mon, ring);
        } else {
            if (uleft && uright && uleft->otyp == RIN_ADORNMENT
                && uright->otyp == RIN_ADORNMENT)
                break;
            if (ring == uleft || ring == uright)
                continue;
            if (uarmg) {
                /* don't put on ring if gloves are in the way */
                if (!tried_gloves++)
                    mayberem(mon, Who, uarmg, "gloves");
                if (uarmg)
                    break; /* no point trying further rings */
            }
            /* confirmation prompt when charisma is high bypassed if deaf */
            if (!Deaf && rn2(20) < ACURR(A_CHA)) {
                (void) safe_qbuf(qbuf, "\"That ",
                                 " looks pretty.  Would you wear it for me?\"",
                                 ring, xname, simpleonames, "ring");
                makeknown(RIN_ADORNMENT);
                if (yn(qbuf) == 'n')
                    continue;
            } else {
                pline("%s decides you'd look prettier wearing %s,",
                      Who, yname(ring));
                pline("and puts it on your finger.");
            }
            makeknown(RIN_ADORNMENT);
            if (!uright) {
                pline("%s puts %s on your right %s.",
                      Who, the(xname(ring)), body_part(HAND));
                setworn(ring, RIGHT_RING);
            } else if (!uleft) {
                pline("%s puts %s on your left %s.",
                      Who, the(xname(ring)), body_part(HAND));
                setworn(ring, LEFT_RING);
            } else if (uright && uright->otyp != RIN_ADORNMENT) {
                /* note: the "replaces" message might be inaccurate if
                   hero's location changes and the process gets interrupted,
                   but trying to figure that out in advance in order to use
                   alternate wording is not worth the effort */
                pline("%s replaces %s with %s.",
                      Who, yname(uright), yname(ring));
                Ring_gone(uright);
                /* ring removal might cause loss of levitation which could
                   drop hero onto trap that transports hero somewhere else */
                if (u.utotype || !next2u(mon->mx, mon->my))
                    return 1;
                setworn(ring, RIGHT_RING);
            } else if (uleft && uleft->otyp != RIN_ADORNMENT) {
                /* see "replaces" note above */
                pline("%s replaces %s with %s.",
                      Who, yname(uleft), yname(ring));
                Ring_gone(uleft);
                if (u.utotype || !next2u(mon->mx, mon->my))
                    return 1;
                setworn(ring, LEFT_RING);
            } else
                impossible("ring replacement");
            Ring_on(ring);
            prinv((char *) 0, ring, 0L);
        }
    }

    naked = (!uarmc && !uarmf && !uarmg && !uarms && !uarmh && !uarmu);
    urgent_pline("%s %s%s.", Who,
                 Deaf ? "seems to murmur into your ear"
                 : naked ? "murmurs sweet nothings into your ear"
                   : "murmurs in your ear",
                 naked ? "" : ", while helping you undress");
    mayberem(mon, Who, uarmc, cloak_simple_name(uarmc));
    if (!uarmc)
        mayberem(mon, Who, uarm, suit_simple_name(uarm));
    mayberem(mon, Who, uarmf, "boots");
    if (!tried_gloves)
        mayberem(mon, Who, uarmg, "gloves");
    mayberem(mon, Who, uarms, "shield");
    mayberem(mon, Who, uarmh, helm_simple_name(uarmh));
    if (!uarmc && !uarm)
        mayberem(mon, Who, uarmu, "shirt");

    /* removing armor (levitation boots, or levitation ring to make
       room for adornment ring with incubus case) might result in the
       hero falling through a trap door or landing on a teleport trap
       and changing location, so hero might not be adjacent to seducer
       any more (mayberem() has its own adjacency test so we don't need
       to check after each potential removal) */
    if (u.utotype || !next2u(mon->mx, mon->my))
        return 1;

    if (uarm || uarmc) {
        if (!Deaf) {
            if (!(ld() && mon->female)) {
                verbalize("You're such a %s; I wish...",
                          flags.female ? "sweet lady" : "nice guy");
            } else {
                struct obj *yourgloves = u_carried_gloves();

                /* have her call your gloves by their correct
                   name, possibly revealing them to you */
                if (yourgloves)
                    yourgloves->dknown = 1;
                verbalize("Well, then you owe me %s%s!",
                          yourgloves ? yname(yourgloves)
                                     : "twelve pairs of gloves",
                          yourgloves ? " and eleven more pairs of gloves"
                                     : "");
            }
        } else if (seewho)
            pline("%s appears to sigh.", Monnam(mon));
        /* else no regret message if can't see or hear seducer */

        if (!tele_restrict(mon))
            (void) rloc(mon, RLOC_MSG);
        return 1;
    }
    if (u.ualign.type == A_CHAOTIC)
        adjalign(1);

    /* by this point you have discovered mon's identity, blind or not... */
    urgent_pline(
             "Time stands still while you and %s lie in each other's arms...",
                 noit_mon_nam(mon));
    /* 3.6.1: a combined total for charisma plus intelligence of 35-1
       used to guarantee successful outcome; now total maxes out at 32
       as far as deciding what will happen; chance for bad outcome when
       Cha+Int is 32 or more is 2/35, a bit over 5.7% */
    attr_tot = ACURR(A_CHA) + ACURR(A_INT);
    if (rn2(35) > min(attr_tot, 32)) {
        /* Don't bother with mspec_used here... it didn't get tired! */
        pline("%s seems to have enjoyed it more than you...",
              noit_Monnam(mon));
        switch (rn2(5)) {
        case 0:
            You_feel("drained of energy.");
            u.uen = 0;
            u.uenmax -= rnd(Half_physical_damage ? 5 : 10);
            exercise(A_CON, FALSE);
            if (u.uenmax < 0)
                u.uenmax = 0;
            break;
        case 1:
            You("are down in the dumps.");
            (void) adjattrib(A_CON, -1, TRUE);
            exercise(A_CON, FALSE);
            g.context.botl = 1;
            break;
        case 2:
            Your("senses are dulled.");
            (void) adjattrib(A_WIS, -1, TRUE);
            exercise(A_WIS, FALSE);
            g.context.botl = 1;
            break;
        case 3:
            if (!resists_drli(&g.youmonst)) {
                You_feel("out of shape.");
                losexp("overexertion");
            } else {
                You("have a curious feeling...");
            }
            exercise(A_CON, FALSE);
            exercise(A_DEX, FALSE);
            exercise(A_WIS, FALSE);
            break;
        case 4: {
            int tmp;

            You_feel("exhausted.");
            exercise(A_STR, FALSE);
            tmp = rn1(10, 6);
            losehp(Maybe_Half_Phys(tmp), "exhaustion", KILLED_BY);
            break;
        } /* case 4 */
        } /* switch */
    } else {
        mon->mspec_used = rnd(100); /* monster is worn out */
        You("seem to have enjoyed it more than %s...", noit_mon_nam(mon));
        switch (rn2(5)) {
        case 0:
            You_feel("raised to your full potential.");
            exercise(A_CON, TRUE);
            u.uen = (u.uenmax += rnd(5));
            if (u.uenmax > u.uenpeak)
                u.uenpeak = u.uenmax;
            break;
        case 1:
            You_feel("good enough to do it again.");
            (void) adjattrib(A_CON, 1, TRUE);
            exercise(A_CON, TRUE);
            g.context.botl = 1;
            break;
        case 2:
            You("will always remember %s...", noit_mon_nam(mon));
            (void) adjattrib(A_WIS, 1, TRUE);
            exercise(A_WIS, TRUE);
            g.context.botl = 1;
            break;
        case 3:
            pline("That was a very educational experience.");
            pluslvl(FALSE);
            exercise(A_WIS, TRUE);
            break;
        case 4:
            You_feel("restored to health!");
            u.uhp = u.uhpmax;
            if (Upolyd)
                u.mh = u.mhmax;
            exercise(A_STR, TRUE);
            g.context.botl = 1;
            break;
        }
    }

    if (mon->mtame) { /* don't charge */
        ;
    } else if (rn2(20) < ACURR(A_CHA)) {
        pline("%s demands that you pay %s, but you refuse...",
              noit_Monnam(mon), noit_mhim(mon));
    } else if (u.umonnum == PM_LEPRECHAUN) {
        pline("%s tries to take your gold, but fails...", noit_Monnam(mon));
    } else {
        long cost;
        long umoney = money_cnt(g.invent);

        if (umoney > (long) LARGEST_INT - 10L)
            cost = (long) rnd(LARGEST_INT) + 500L;
        else
            cost = (long) rnd((int) umoney + 10) + 500L;
        if (mon->mpeaceful) {
            cost /= 5L;
            if (!cost)
                cost = 1L;
        }
        if (cost > umoney)
            cost = umoney;
        if (!cost) {
            verbalize("It's on the house!");
        } else {
            pline("%s takes %ld %s for services rendered!", noit_Monnam(mon),
                  cost, currency(cost));
            money2mon(mon, cost);
            g.context.botl = 1;
        }
    }
    if (!rn2(25))
        mon->mcan = 1; /* monster is worn out */
    if (!tele_restrict(mon))
        (void) rloc(mon, RLOC_MSG);
    return 1;
}

/* 'mon' tries to remove a piece of hero's armor */
static void
mayberem(struct monst *mon,
         const char *seducer, /* only used for alternate message */
         struct obj *obj, const char *str)
{
    char qbuf[QBUFSZ];

    if (!obj || !obj->owornmask)
        return;
    /* removal of a previous item might have sent the hero elsewhere
       (loss of levitation that leads to landing on a transport trap) */
    if (u.utotype || !next2u(mon->mx, mon->my))
        return;

    /* being deaf overrides confirmation prompt for high charisma */
    if (Deaf) {
        pline("%s takes off your %s.", seducer, str);
    } else if (rn2(20) < ACURR(A_CHA)) {
        Sprintf(qbuf, "\"Shall I remove your %s, %s?\"", str,
                (!rn2(2) ? "lover" : !rn2(2) ? "dear" : "sweetheart"));
        if (yn(qbuf) == 'n')
            return;
    } else {
        char hairbuf[BUFSZ];

        Sprintf(hairbuf, "let me run my fingers through your %s",
                body_part(HAIR));
        verbalize("Take off your %s; %s.", str,
                  (obj == uarm)
                     ? "let's get a little closer"
                     : (obj == uarmc || obj == uarms)
                        ? "it's in the way"
                        : (obj == uarmf)
                           ? "let me rub your feet"
                           : (obj == uarmg)
                              ? "they're too clumsy"
                              : (obj == uarmu)
                                 ? "let me massage you"
                                 /* obj == uarmh */
                                 : hairbuf);
    }
    remove_worn_item(obj, TRUE);
}

/* FIXME:
 *  sequencing issue:  a monster's attack might cause poly'd hero
 *  to revert to normal form.  The messages for passive counterattack
 *  would look better if they came before reverting form, but we need
 *  to know whether hero reverted in order to decide whether passive
 *  damage applies.
 */
static int
passiveum(struct permonst *olduasmon, struct monst *mtmp, struct attack *mattk)
{
    int i, tmp;
    struct attack *oldu_mattk = 0;

    /*
     * mattk      == mtmp's attack that hit you;
     * oldu_mattk == your passive counterattack (even if mtmp's attack
     *               has already caused you to revert to normal form).
     */
    for (i = 0; !oldu_mattk; i++) {
        if (i >= NATTK)
            return MM_HIT;
        if (olduasmon->mattk[i].aatyp == AT_NONE
            || olduasmon->mattk[i].aatyp == AT_BOOM)
            oldu_mattk = &olduasmon->mattk[i];
    }
    if (oldu_mattk->damn)
        tmp = d((int) oldu_mattk->damn, (int) oldu_mattk->damd);
    else if (oldu_mattk->damd)
        tmp = d((int) olduasmon->mlevel + 1, (int) oldu_mattk->damd);
    else
        tmp = 0;

    /* These affect the enemy even if you were "killed" (rehumanized) */
    switch (oldu_mattk->adtyp) {
    case AD_ACID:
        if (!rn2(2)) {
            pline("%s is splashed by %s%s!", Monnam(mtmp),
                  /* temporary? hack for sequencing issue:  "your acid"
                     looks strange coming immediately after player has
                     been told that hero has reverted to normal form */
                  !Upolyd ? "" : "your ", hliquid("acid"));
            if (resists_acid(mtmp)) {
                pline("%s is not affected.", Monnam(mtmp));
                tmp = 0;
            }
        } else
            tmp = 0;
        if (!rn2(30))
            erode_armor(mtmp, ERODE_CORRODE);
        if (!rn2(6))
            acid_damage(MON_WEP(mtmp));
        goto assess_dmg;
    case AD_STON: /* cockatrice */
    {
        long protector = attk_protection((int) mattk->aatyp),
             wornitems = mtmp->misc_worn_check;

        /* wielded weapon gives same protection as gloves here */
        if (MON_WEP(mtmp) != 0)
            wornitems |= W_ARMG;

        if (!resists_ston(mtmp)
            && (protector == 0L
                || (protector != ~0L
                    && (wornitems & protector) != protector))) {
            if (poly_when_stoned(mtmp->data)) {
                mon_to_stone(mtmp);
                return 1;
            }
            pline("%s turns to stone!", Monnam(mtmp));
            g.stoned = 1;
            xkilled(mtmp, XKILL_NOMSG);
            if (!DEADMONSTER(mtmp))
                return MM_HIT;
            return MM_AGR_DIED;
        }
        return MM_HIT;
    }
    case AD_ENCH: /* KMH -- remove enchantment (disenchanter) */
        if (mon_currwep) {
            /* by_you==True: passive counterattack to hero's action
               is hero's fault */
            (void) drain_item(mon_currwep, TRUE);
            /* No message */
        }
        return MM_HIT;
    default:
        break;
    }
    if (!Upolyd)
        return MM_HIT;

    /* These affect the enemy only if you are still a monster */
    if (rn2(3))
        switch (oldu_mattk->adtyp) {
        case AD_PHYS:
            if (oldu_mattk->aatyp == AT_BOOM) {
                You("explode!");
                /* KMH, balance patch -- this is okay with unchanging */
                rehumanize();
                goto assess_dmg;
            }
            break;
        case AD_PLYS: /* Floating eye */
            if (tmp > 127)
                tmp = 127;
            if (u.umonnum == PM_FLOATING_EYE) {
                if (!rn2(4))
                    tmp = 127;
                if (mtmp->mcansee && haseyes(mtmp->data) && rn2(3)
                    && (perceives(mtmp->data) || !Invis)) {
                    if (Blind)
                        pline("As a blind %s, you cannot defend yourself.",
                              pmname(g.youmonst.data,
                                     flags.female ? FEMALE : MALE));
                    else {
                        if (mon_reflects(mtmp,
                                         "Your gaze is reflected by %s %s."))
                            return 1;
                        pline("%s is frozen by your gaze!", Monnam(mtmp));
                        paralyze_monst(mtmp, tmp);
                        return MM_AGR_DONE;
                    }
                }
            } else { /* gelatinous cube */
                pline("%s is frozen by you.", Monnam(mtmp));
                paralyze_monst(mtmp, tmp);
                return MM_AGR_DONE;
            }
            return MM_HIT;
        case AD_COLD: /* Brown mold or blue jelly */
            if (resists_cold(mtmp)) {
                shieldeff(mtmp->mx, mtmp->my);
                pline("%s is mildly chilly.", Monnam(mtmp));
                golemeffects(mtmp, AD_COLD, tmp);
                tmp = 0;
                break;
            }
            pline("%s is suddenly very cold!", Monnam(mtmp));
            u.mh += tmp / 2;
            if (u.mhmax < u.mh)
                u.mhmax = u.mh;
            if (u.mhmax > ((g.youmonst.data->mlevel + 1) * 8))
                (void) split_mon(&g.youmonst, mtmp);
            break;
        case AD_STUN: /* Yellow mold */
            if (!mtmp->mstun) {
                mtmp->mstun = 1;
                pline("%s %s.", Monnam(mtmp),
                      makeplural(stagger(mtmp->data, "stagger")));
            }
            tmp = 0;
            break;
        case AD_FIRE: /* Red mold */
            if (resists_fire(mtmp)) {
                shieldeff(mtmp->mx, mtmp->my);
                pline("%s is mildly warm.", Monnam(mtmp));
                golemeffects(mtmp, AD_FIRE, tmp);
                tmp = 0;
                break;
            }
            pline("%s is suddenly very hot!", Monnam(mtmp));
            break;
        case AD_ELEC:
            if (resists_elec(mtmp)) {
                shieldeff(mtmp->mx, mtmp->my);
                pline("%s is slightly tingled.", Monnam(mtmp));
                golemeffects(mtmp, AD_ELEC, tmp);
                tmp = 0;
                break;
            }
            pline("%s is jolted with your electricity!", Monnam(mtmp));
            break;
        default:
            tmp = 0;
            break;
        }
    else
        tmp = 0;

 assess_dmg:
    if ((mtmp->mhp -= tmp) <= 0) {
        pline("%s dies!", Monnam(mtmp));
        xkilled(mtmp, XKILL_NOMSG);
        if (!DEADMONSTER(mtmp))
            return MM_HIT;
        return MM_AGR_DIED;
    }
    return MM_HIT;
}

struct monst *
cloneu(void)
{
    struct monst *mon;
    int mndx = monsndx(g.youmonst.data);

    if (u.mh <= 1)
        return (struct monst *) 0;
    if (g.mvitals[mndx].mvflags & G_EXTINCT)
        return (struct monst *) 0;
    mon = makemon(g.youmonst.data, u.ux, u.uy,
                  NO_MINVENT | MM_EDOG | MM_NOMSG);
    if (!mon)
        return NULL;
    mon->mcloned = 1;
    mon = christen_monst(mon, g.plname);
    initedog(mon);
    mon->m_lev = g.youmonst.data->mlevel;
    mon->mhpmax = u.mhmax;
    mon->mhp = u.mh / 2;
    u.mh -= mon->mhp;
    g.context.botl = 1;
    return mon;
}

/*mhitu.c*/

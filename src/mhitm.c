/* NetHack 3.6	mhitm.c	$NHDT-Date: 1513297346 2017/12/15 00:22:26 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.99 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

extern boolean notonhead;

static NEARDATA boolean vis, far_noise;
static NEARDATA long noisetime;
static NEARDATA struct obj *otmp;

static const char brief_feeling[] =
    "have a %s feeling for a moment, then it passes.";

STATIC_DCL char *FDECL(mon_nam_too, (char *, struct monst *, struct monst *));
STATIC_DCL int FDECL(hitmm, (struct monst *, struct monst *,
                             struct attack *));
STATIC_DCL int FDECL(gazemm, (struct monst *, struct monst *,
                              struct attack *));
STATIC_DCL int FDECL(gulpmm, (struct monst *, struct monst *,
                              struct attack *));
STATIC_DCL int FDECL(explmm, (struct monst *, struct monst *,
                              struct attack *));
STATIC_DCL int FDECL(mdamagem, (struct monst *, struct monst *,
                                struct attack *));
STATIC_DCL void FDECL(mswingsm, (struct monst *, struct monst *,
                                 struct obj *));
STATIC_DCL void FDECL(noises, (struct monst *, struct attack *));
STATIC_DCL void FDECL(missmm, (struct monst *, struct monst *,
                               struct attack *));
STATIC_DCL int FDECL(passivemm, (struct monst *, struct monst *,
                                 BOOLEAN_P, int));

/* Needed for the special case of monsters wielding vorpal blades (rare).
 * If we use this a lot it should probably be a parameter to mdamagem()
 * instead of a global variable.
 */
static int dieroll;

/* returns mon_nam(mon) relative to other_mon; normal name unless they're
   the same, in which case the reference is to {him|her|it} self */
STATIC_OVL char *
mon_nam_too(outbuf, mon, other_mon)
char *outbuf;
struct monst *mon, *other_mon;
{
    if (mon != other_mon)
        Strcpy(outbuf, mon_nam(mon));
    else
        switch (pronoun_gender(mon, FALSE)) {
        case 0:
            Strcpy(outbuf, "himself");
            break;
        case 1:
            Strcpy(outbuf, "herself");
            break;
        default:
            Strcpy(outbuf, "itself");
            break;
        }
    return outbuf;
}

STATIC_OVL void
noises(magr, mattk)
register struct monst *magr;
register struct attack *mattk;
{
    boolean farq = (distu(magr->mx, magr->my) > 15);

    if (!Deaf && (farq != far_noise || moves - noisetime > 10)) {
        far_noise = farq;
        noisetime = moves;
        You_hear("%s%s.",
                 (mattk->aatyp == AT_EXPL) ? "an explosion" : "some noises",
                 farq ? " in the distance" : "");
    }
}

STATIC_OVL
void
missmm(magr, mdef, mattk)
register struct monst *magr, *mdef;
struct attack *mattk;
{
    const char *fmt;
    char buf[BUFSZ], mdef_name[BUFSZ];

    if (vis) {
        if (!canspotmon(magr))
            map_invisible(magr->mx, magr->my);
        if (!canspotmon(mdef))
            map_invisible(mdef->mx, mdef->my);
        if (mdef->m_ap_type)
            seemimic(mdef);
        if (magr->m_ap_type)
            seemimic(magr);
        fmt = (could_seduce(magr, mdef, mattk) && !magr->mcan)
                  ? "%s pretends to be friendly to"
                  : "%s misses";
        Sprintf(buf, fmt, Monnam(magr));
        pline("%s %s.", buf, mon_nam_too(mdef_name, mdef, magr));
    } else
        noises(magr, mattk);
}

/*
 *  fightm()  -- fight some other monster
 *
 *  Returns:
 *      0 - Monster did nothing.
 *      1 - If the monster made an attack.  The monster might have died.
 *
 *  There is an exception to the above.  If mtmp has the hero swallowed,
 *  then we report that the monster did nothing so it will continue to
 *  digest the hero.
 */
 /* have monsters fight each other */
int
fightm(mtmp)
register struct monst *mtmp;
{
    register struct monst *mon, *nmon;
    int result, has_u_swallowed;
#ifdef LINT
    nmon = 0;
#endif
    /* perhaps the monster will resist Conflict */
    if (resist(mtmp, RING_CLASS, 0, 0))
        return 0;

    if (u.ustuck == mtmp) {
        /* perhaps we're holding it... */
        if (itsstuck(mtmp))
            return 0;
    }
    has_u_swallowed = (u.uswallow && (mtmp == u.ustuck));

    for (mon = fmon; mon; mon = nmon) {
        nmon = mon->nmon;
        if (nmon == mtmp)
            nmon = mtmp->nmon;
        /* Be careful to ignore monsters that are already dead, since we
         * might be calling this before we've cleaned them up.  This can
         * happen if the monster attacked a cockatrice bare-handedly, for
         * instance.
         */
        if (mon != mtmp && !DEADMONSTER(mon)) {
            if (monnear(mtmp, mon->mx, mon->my)) {
                if (!u.uswallow && (mtmp == u.ustuck)) {
                    if (!rn2(4)) {
                        pline("%s releases you!", Monnam(mtmp));
                        u.ustuck = 0;
                    } else
                        break;
                }

                /* mtmp can be killed */
                bhitpos.x = mon->mx;
                bhitpos.y = mon->my;
                notonhead = 0;
                result = mattackm(mtmp, mon);

                if (result & MM_AGR_DIED)
                    return 1; /* mtmp died */
                /*
                 * If mtmp has the hero swallowed, lie and say there
                 * was no attack (this allows mtmp to digest the hero).
                 */
                if (has_u_swallowed)
                    return 0;

                /* Allow attacked monsters a chance to hit back. Primarily
                 * to allow monsters that resist conflict to respond.
                 */
                if ((result & MM_HIT) && !(result & MM_DEF_DIED) && rn2(4)
                    && mon->movement >= NORMAL_SPEED) {
                    mon->movement -= NORMAL_SPEED;
                    notonhead = 0;
                    (void) mattackm(mon, mtmp); /* return attack */
                }

                return (result & MM_HIT) ? 1 : 0;
            }
        }
    }
    return 0;
}

/*
 * mdisplacem() -- attacker moves defender out of the way;
 *                 returns same results as mattackm().
 */
int
mdisplacem(magr, mdef, quietly)
register struct monst *magr, *mdef;
boolean quietly;
{
    struct permonst *pa, *pd;
    int tx, ty, fx, fy;

    /* sanity checks; could matter if we unexpectedly get a long worm */
    if (!magr || !mdef || magr == mdef)
        return MM_MISS;
    pa = magr->data, pd = mdef->data;
    tx = mdef->mx, ty = mdef->my; /* destination */
    fx = magr->mx, fy = magr->my; /* current location */
    if (m_at(fx, fy) != magr || m_at(tx, ty) != mdef)
        return MM_MISS;

    /* The 1 in 7 failure below matches the chance in attack()
     * for pet displacement.
     */
    if (!rn2(7))
        return MM_MISS;

    /* Grid bugs cannot displace at an angle. */
    if (pa == &mons[PM_GRID_BUG] && magr->mx != mdef->mx
        && magr->my != mdef->my)
        return MM_MISS;

    /* undetected monster becomes un-hidden if it is displaced */
    if (mdef->mundetected)
        mdef->mundetected = 0;
    if (mdef->m_ap_type && mdef->m_ap_type != M_AP_MONSTER)
        seemimic(mdef);
    /* wake up the displaced defender */
    mdef->msleeping = 0;
    mdef->mstrategy &= ~STRAT_WAITMASK;
    finish_meating(mdef);

    /*
     * Set up the visibility of action.
     * You can observe monster displacement if you can see both of
     * the monsters involved.
     */
    vis = (canspotmon(magr) && canspotmon(mdef));

    if (touch_petrifies(pd) && !resists_ston(magr)) {
        if (which_armor(magr, W_ARMG) != 0) {
            if (poly_when_stoned(pa)) {
                mon_to_stone(magr);
                return MM_HIT; /* no damage during the polymorph */
            }
            if (!quietly && canspotmon(magr))
                pline("%s turns to stone!", Monnam(magr));
            monstone(magr);
            if (!DEADMONSTER(magr))
                return MM_HIT; /* lifesaved */
            else if (magr->mtame && !vis)
                You(brief_feeling, "peculiarly sad");
            return MM_AGR_DIED;
        }
    }

    remove_monster(fx, fy); /* pick up from orig position */
    remove_monster(tx, ty);
    place_monster(magr, tx, ty); /* put down at target spot */
    place_monster(mdef, fx, fy);
    if (vis && !quietly)
        pline("%s moves %s out of %s way!", Monnam(magr), mon_nam(mdef),
              is_rider(pa) ? "the" : mhis(magr));
    newsym(fx, fy);  /* see it */
    newsym(tx, ty);  /*   all happen */
    flush_screen(0); /* make sure it shows up */

    return MM_HIT;
}

/*
 * mattackm() -- a monster attacks another monster.
 *
 *          --------- aggressor died
 *         /  ------- defender died
 *        /  /  ----- defender was hit
 *       /  /  /
 *      x  x  x
 *
 *      0x4     MM_AGR_DIED
 *      0x2     MM_DEF_DIED
 *      0x1     MM_HIT
 *      0x0     MM_MISS
 *
 * Each successive attack has a lower probability of hitting.  Some rely on
 * success of previous attacks.  ** this doen't seem to be implemented -dl **
 *
 * In the case of exploding monsters, the monster dies as well.
 */
int
mattackm(magr, mdef)
register struct monst *magr, *mdef;
{
    int i,          /* loop counter */
        tmp,        /* amour class difference */
        strike = 0, /* hit this attack */
        attk,       /* attack attempted this time */
        struck = 0, /* hit at least once */
        res[NATTK]; /* results of all attacks */
    struct attack *mattk, alt_attk;
    struct permonst *pa, *pd;

    if (!magr || !mdef)
        return MM_MISS; /* mike@genat */
    if (!magr->mcanmove || magr->msleeping)
        return MM_MISS;
    pa = magr->data;
    pd = mdef->data;

    /* Grid bugs cannot attack at an angle. */
    if (pa == &mons[PM_GRID_BUG] && magr->mx != mdef->mx
        && magr->my != mdef->my)
        return MM_MISS;

    /* Calculate the armour class differential. */
    tmp = find_mac(mdef) + magr->m_lev;
    if (mdef->mconf || !mdef->mcanmove || mdef->msleeping) {
        tmp += 4;
        mdef->msleeping = 0;
    }

    /* undetect monsters become un-hidden if they are attacked */
    if (mdef->mundetected) {
        mdef->mundetected = 0;
        newsym(mdef->mx, mdef->my);
        if (canseemon(mdef) && !sensemon(mdef)) {
            if (Unaware) {
                boolean justone = (mdef->data->geno & G_UNIQ) != 0L;
                const char *montype;

                montype = noname_monnam(mdef, justone ? ARTICLE_THE
                                                      : ARTICLE_NONE);
                if (!justone)
                    montype = makeplural(montype);
                You("dream of %s.", montype);
            } else
                pline("Suddenly, you notice %s.", a_monnam(mdef));
        }
    }

    /* Elves hate orcs. */
    if (is_elf(pa) && is_orc(pd))
        tmp++;

    /* Set up the visibility of action */
    vis = (cansee(magr->mx, magr->my) && cansee(mdef->mx, mdef->my)
           && (canspotmon(magr) || canspotmon(mdef)));

    /* Set flag indicating monster has moved this turn.  Necessary since a
     * monster might get an attack out of sequence (i.e. before its move) in
     * some cases, in which case this still counts as its move for the round
     * and it shouldn't move again.
     */
    magr->mlstmv = monstermoves;

    /* Now perform all attacks for the monster. */
    for (i = 0; i < NATTK; i++) {
        res[i] = MM_MISS;
        mattk = getmattk(magr, mdef, i, res, &alt_attk);
        otmp = (struct obj *) 0;
        attk = 1;
        switch (mattk->aatyp) {
        case AT_WEAP: /* "hand to hand" attacks */
            if (distmin(magr->mx, magr->my, mdef->mx, mdef->my) > 1) {
                /* D: Do a ranged attack here! */
                strike = thrwmm(magr, mdef);
                if (DEADMONSTER(mdef))
                    res[i] = MM_DEF_DIED;
                if (DEADMONSTER(magr))
                    res[i] |= MM_AGR_DIED;
                break;
            }
            if (magr->weapon_check == NEED_WEAPON || !MON_WEP(magr)) {
                magr->weapon_check = NEED_HTH_WEAPON;
                if (mon_wield_item(magr) != 0)
                    return 0;
            }
            possibly_unwield(magr, FALSE);
            otmp = MON_WEP(magr);

            if (otmp) {
                if (vis)
                    mswingsm(magr, mdef, otmp);
                tmp += hitval(otmp, mdef);
            }
            /*FALLTHRU*/
        case AT_CLAW:
        case AT_KICK:
        case AT_BITE:
        case AT_STNG:
        case AT_TUCH:
        case AT_BUTT:
        case AT_TENT:
            /* Nymph that teleported away on first attack? */
            if (distmin(magr->mx, magr->my, mdef->mx, mdef->my) > 1)
                /* Continue because the monster may have a ranged attack. */
                continue;
            /* Monsters won't attack cockatrices physically if they
             * have a weapon instead.  This instinct doesn't work for
             * players, or under conflict or confusion.
             */
            if (!magr->mconf && !Conflict && otmp && mattk->aatyp != AT_WEAP
                && touch_petrifies(mdef->data)) {
                strike = 0;
                break;
            }
            dieroll = rnd(20 + i);
            strike = (tmp > dieroll);
            /* KMH -- don't accumulate to-hit bonuses */
            if (otmp)
                tmp -= hitval(otmp, mdef);
            if (strike) {
                res[i] = hitmm(magr, mdef, mattk);
                if ((mdef->data == &mons[PM_BLACK_PUDDING]
                     || mdef->data == &mons[PM_BROWN_PUDDING])
                    && (otmp && (objects[otmp->otyp].oc_material == IRON
                                 || objects[otmp->otyp].oc_material == METAL))
                    && mdef->mhp > 1
                    && !mdef->mcan) {
                    if (clone_mon(mdef, 0, 0)) {
                        if (vis && canspotmon(mdef)) {
                            char buf[BUFSZ];

                            Strcpy(buf, Monnam(mdef));
                            pline("%s divides as %s hits it!", buf,
                                  mon_nam(magr));
                        }
                    }
                }
            } else
                missmm(magr, mdef, mattk);
            break;

        case AT_HUGS: /* automatic if prev two attacks succeed */
            strike = (i >= 2 && res[i - 1] == MM_HIT && res[i - 2] == MM_HIT);
            if (strike)
                res[i] = hitmm(magr, mdef, mattk);

            break;

        case AT_GAZE:
            strike = 0;
            res[i] = gazemm(magr, mdef, mattk);
            break;

        case AT_EXPL:
            /* D: Prevent explosions from a distance */
            if (distmin(magr->mx,magr->my,mdef->mx,mdef->my) > 1)
                continue;

            res[i] = explmm(magr, mdef, mattk);
            if (res[i] == MM_MISS) { /* cancelled--no attack */
                strike = 0;
                attk = 0;
            } else
                strike = 1; /* automatic hit */
            break;

        case AT_ENGL:
            if (u.usteed && mdef == u.usteed) {
                strike = 0;
                break;
            }
            /* D: Prevent engulf from a distance */
            if (distmin(magr->mx, magr->my, mdef->mx, mdef->my) > 1)
                continue;
            /* Engulfing attacks are directed at the hero if possible. -dlc */
            if (u.uswallow && magr == u.ustuck)
                strike = 0;
            else if ((strike = (tmp > rnd(20 + i))) != 0)
                res[i] = gulpmm(magr, mdef, mattk);
            else
                missmm(magr, mdef, mattk);
            break;

        case AT_BREA:
            if (!monnear(magr, mdef->mx, mdef->my)) {
                strike = breamm(magr, mattk, mdef);

                /* We don't really know if we hit or not; pretend we did. */
                if (strike)
                    res[i] |= MM_HIT;
                if (DEADMONSTER(mdef))
                    res[i] = MM_DEF_DIED;
                if (DEADMONSTER(magr))
                    res[i] |= MM_AGR_DIED;
            }
            else
                strike = 0;
            break;

        case AT_SPIT:
            if (!monnear(magr, mdef->mx, mdef->my)) {
                strike = spitmm(magr, mattk, mdef);

                /* We don't really know if we hit or not; pretend we did. */
                if (strike)
                    res[i] |= MM_HIT;
                if (DEADMONSTER(mdef))
                    res[i] = MM_DEF_DIED;
                if (DEADMONSTER(magr))
                    res[i] |= MM_AGR_DIED;
            }
            break;

        default: /* no attack */
            strike = 0;
            attk = 0;
            break;
        }

        if (attk && !(res[i] & MM_AGR_DIED)
            && distmin(magr->mx, magr->my, mdef->mx, mdef->my) <= 1)
            res[i] = passivemm(magr, mdef, strike, res[i] & MM_DEF_DIED);

        if (res[i] & MM_DEF_DIED)
            return res[i];
        if (res[i] & MM_AGR_DIED)
            return res[i];
        /* return if aggressor can no longer attack */
        if (!magr->mcanmove || magr->msleeping)
            return res[i];
        if (res[i] & MM_HIT)
            struck = 1; /* at least one hit */
    }

    return (struck ? MM_HIT : MM_MISS);
}

/* Returns the result of mdamagem(). */
STATIC_OVL int
hitmm(magr, mdef, mattk)
register struct monst *magr, *mdef;
struct attack *mattk;
{
    if (vis) {
        int compat;
        char buf[BUFSZ], mdef_name[BUFSZ];

        if (!canspotmon(magr))
            map_invisible(magr->mx, magr->my);
        if (!canspotmon(mdef))
            map_invisible(mdef->mx, mdef->my);
        if (mdef->m_ap_type)
            seemimic(mdef);
        if (magr->m_ap_type)
            seemimic(magr);
        if ((compat = could_seduce(magr, mdef, mattk)) && !magr->mcan) {
            Sprintf(buf, "%s %s", Monnam(magr),
                    mdef->mcansee ? "smiles at" : "talks to");
            pline("%s %s %s.", buf, mon_nam(mdef),
                  compat == 2 ? "engagingly" : "seductively");
        } else {
            char magr_name[BUFSZ];

            Strcpy(magr_name, Monnam(magr));
            switch (mattk->aatyp) {
            case AT_BITE:
                Sprintf(buf, "%s bites", magr_name);
                break;
            case AT_STNG:
                Sprintf(buf, "%s stings", magr_name);
                break;
            case AT_BUTT:
                Sprintf(buf, "%s butts", magr_name);
                break;
            case AT_TUCH:
                Sprintf(buf, "%s touches", magr_name);
                break;
            case AT_TENT:
                Sprintf(buf, "%s tentacles suck", s_suffix(magr_name));
                break;
            case AT_HUGS:
                if (magr != u.ustuck) {
                    Sprintf(buf, "%s squeezes", magr_name);
                    break;
                }
                /*FALLTHRU*/
            default:
                Sprintf(buf, "%s hits", magr_name);
            }
            pline("%s %s.", buf, mon_nam_too(mdef_name, mdef, magr));
        }
    } else
        noises(magr, mattk);

    return mdamagem(magr, mdef, mattk);
}

/* Returns the same values as mdamagem(). */
STATIC_OVL int
gazemm(magr, mdef, mattk)
register struct monst *magr, *mdef;
struct attack *mattk;
{
    char buf[BUFSZ];

    if (vis) {
        if (mdef->data->mlet == S_MIMIC
            && mdef->m_ap_type != M_AP_NOTHING)
            seemimic(mdef);
        Sprintf(buf, "%s gazes at", Monnam(magr));
        pline("%s %s...", buf,
              canspotmon(mdef) ? mon_nam(mdef) : "something");
    }

    if (magr->mcan || !magr->mcansee || !mdef->mcansee
        || (magr->minvis && !perceives(mdef->data)) || mdef->msleeping) {
        if (vis && canspotmon(mdef))
            pline("but nothing happens.");
        return MM_MISS;
    }
    /* call mon_reflects 2x, first test, then, if visible, print message */
    if (magr->data == &mons[PM_MEDUSA] && mon_reflects(mdef, (char *) 0)) {
        if (canseemon(mdef))
            (void) mon_reflects(mdef, "The gaze is reflected away by %s %s.");
        if (mdef->mcansee) {
            if (mon_reflects(magr, (char *) 0)) {
                if (canseemon(magr))
                    (void) mon_reflects(magr,
                                      "The gaze is reflected away by %s %s.");
                return MM_MISS;
            }
            if (mdef->minvis && !perceives(magr->data)) {
                if (canseemon(magr)) {
                    pline(
                      "%s doesn't seem to notice that %s gaze was reflected.",
                          Monnam(magr), mhis(magr));
                }
                return MM_MISS;
            }
            if (canseemon(magr))
                pline("%s is turned to stone!", Monnam(magr));
            monstone(magr);
            if (!DEADMONSTER(magr))
                return MM_MISS;
            return MM_AGR_DIED;
        }
    }

    return mdamagem(magr, mdef, mattk);
}

/* return True if magr is allowed to swallow mdef, False otherwise */
boolean
engulf_target(magr, mdef)
struct monst *magr, *mdef;
{
    struct rm *lev;
    int dx, dy;

    /* can't swallow something that's too big */
    if (mdef->data->msize >= MZ_HUGE)
        return FALSE;

    /* (hypothetical) engulfers who can pass through walls aren't
     limited by rock|trees|bars */
    if ((magr == &youmonst) ? Passes_walls : passes_walls(magr->data))
        return TRUE;

    /* don't swallow something in a spot where attacker wouldn't
       otherwise be able to move onto; we don't want to engulf
       a wall-phaser and end up with a non-phaser inside a wall */
    dx = mdef->mx, dy = mdef->my;
    if (mdef == &youmonst)
        dx = u.ux, dy = u.uy;
    lev = &levl[dx][dy];
    if (IS_ROCK(lev->typ) || closed_door(dx, dy) || IS_TREE(lev->typ)
        /* not passes_bars(); engulfer isn't squeezing through */
        || (lev->typ == IRONBARS && !is_whirly(magr->data)))
        return FALSE;

    return TRUE;
}

/* Returns the same values as mattackm(). */
STATIC_OVL int
gulpmm(magr, mdef, mattk)
register struct monst *magr, *mdef;
register struct attack *mattk;
{
    xchar ax, ay, dx, dy;
    int status;
    char buf[BUFSZ];
    struct obj *obj;

    if (!engulf_target(magr, mdef))
        return MM_MISS;

    if (vis) {
        /* [this two-part formatting dates back to when only one x_monnam
           result could be included in an expression because the next one
           would overwrite first's result -- that's no longer the case] */
        Sprintf(buf, "%s swallows", Monnam(magr));
        pline("%s %s.", buf, mon_nam(mdef));
    }
    for (obj = mdef->minvent; obj; obj = obj->nobj)
        (void) snuff_lit(obj);

    if (is_vampshifter(mdef)
        && newcham(mdef, &mons[mdef->cham], FALSE, FALSE)) {
        if (vis) {
            /* 'it' -- previous form is no longer available and
               using that would be excessively verbose */
            pline("%s expels %s.", Monnam(magr),
                  canspotmon(mdef) ? "it" : something);
            if (canspotmon(mdef))
                pline("It turns into %s.", a_monnam(mdef));
        }
        return MM_HIT; /* bypass mdamagem() */
    }

    /*
     *  All of this manipulation is needed to keep the display correct.
     *  There is a flush at the next pline().
     */
    ax = magr->mx;
    ay = magr->my;
    dx = mdef->mx;
    dy = mdef->my;
    /*
     *  Leave the defender in the monster chain at it's current position,
     *  but don't leave it on the screen.  Move the aggressor to the
     *  defender's position.
     */
    remove_monster(ax, ay);
    place_monster(magr, dx, dy);
    newsym(ax, ay); /* erase old position */
    newsym(dx, dy); /* update new position */

    status = mdamagem(magr, mdef, mattk);

    if ((status & (MM_AGR_DIED | MM_DEF_DIED))
        == (MM_AGR_DIED | MM_DEF_DIED)) {
        ;                              /* both died -- do nothing  */
    } else if (status & MM_DEF_DIED) { /* defender died */
        /*
         *  Note:  remove_monster() was called in relmon(), wiping out
         *  magr from level.monsters[mdef->mx][mdef->my].  We need to
         *  put it back and display it.  -kd
         */
        place_monster(magr, dx, dy);
        newsym(dx, dy);
        /* aggressor moves to <dx,dy> and might encounter trouble there */
        if (minliquid(magr) || (t_at(dx, dy) && mintrap(magr) == 2))
            status |= MM_AGR_DIED;
    } else if (status & MM_AGR_DIED) { /* aggressor died */
        place_monster(mdef, dx, dy);
        newsym(dx, dy);
    } else {                           /* both alive, put them back */
        if (cansee(dx, dy))
            pline("%s is regurgitated!", Monnam(mdef));

        remove_monster(dx,dy);
        place_monster(magr, ax, ay);
        place_monster(mdef, dx, dy);
        newsym(ax, ay);
        newsym(dx, dy);
    }

    return status;
}

STATIC_OVL int
explmm(magr, mdef, mattk)
struct monst *magr, *mdef;
struct attack *mattk;
{
    int result;

    if (magr->mcan)
        return MM_MISS;

    if (cansee(magr->mx, magr->my))
        pline("%s explodes!", Monnam(magr));
    else
        noises(magr, mattk);

    result = mdamagem(magr, mdef, mattk);

    /* Kill off aggressor if it didn't die. */
    if (!(result & MM_AGR_DIED)) {
        boolean was_leashed = (magr->mleashed != 0);

        mondead(magr);
        if (!DEADMONSTER(magr))
            return result; /* life saved */
        result |= MM_AGR_DIED;

        /* mondead() -> m_detach() -> m_unleash() always suppresses
           the m_unleash() slack message, so deliver it here instead */
        if (was_leashed)
            Your("leash falls slack.");
    }
    if (magr->mtame) /* give this one even if it was visible */
        You(brief_feeling, "melancholy");

    return result;
}

/*
 *  See comment at top of mattackm(), for return values.
 */
STATIC_OVL int
mdamagem(magr, mdef, mattk)
register struct monst *magr, *mdef;
register struct attack *mattk;
{
    struct obj *obj;
    char buf[BUFSZ];
    struct permonst *pa = magr->data, *pd = mdef->data;
    int armpro, num, tmp = d((int) mattk->damn, (int) mattk->damd),
                     res = MM_MISS;
    boolean cancelled;

    if ((touch_petrifies(pd) /* or flesh_petrifies() */
         || (mattk->adtyp == AD_DGST && pd == &mons[PM_MEDUSA]))
        && !resists_ston(magr)) {
        long protector = attk_protection((int) mattk->aatyp),
             wornitems = magr->misc_worn_check;

        /* wielded weapon gives same protection as gloves here */
        if (otmp != 0)
            wornitems |= W_ARMG;

        if (protector == 0L
            || (protector != ~0L && (wornitems & protector) != protector)) {
            if (poly_when_stoned(pa)) {
                mon_to_stone(magr);
                return MM_HIT; /* no damage during the polymorph */
            }
            if (vis && canspotmon(magr))
                pline("%s turns to stone!", Monnam(magr));
            monstone(magr);
            if (!DEADMONSTER(magr))
                return MM_HIT; /* lifesaved */
            else if (magr->mtame && !vis)
                You(brief_feeling, "peculiarly sad");
            return MM_AGR_DIED;
        }
    }

    /* cancellation factor is the same as when attacking the hero */
    armpro = magic_negation(mdef);
    cancelled = magr->mcan || !(rn2(10) >= 3 * armpro);

    switch (mattk->adtyp) {
    case AD_DGST:
        /* eating a Rider or its corpse is fatal */
        if (is_rider(pd)) {
            if (vis && canseemon(magr))
                pline("%s %s!", Monnam(magr),
                      (pd == &mons[PM_FAMINE])
                          ? "belches feebly, shrivels up and dies"
                          : (pd == &mons[PM_PESTILENCE])
                                ? "coughs spasmodically and collapses"
                                : "vomits violently and drops dead");
            mondied(magr);
            if (!DEADMONSTER(magr))
                return 0; /* lifesaved */
            else if (magr->mtame && !vis)
                You(brief_feeling, "queasy");
            return MM_AGR_DIED;
        }
        if (flags.verbose && !Deaf)
            verbalize("Burrrrp!");
        tmp = mdef->mhp;
        /* Use up amulet of life saving */
        if (!!(obj = mlifesaver(mdef)))
            m_useup(mdef, obj);

        /* Is a corpse for nutrition possible?  It may kill magr */
        if (!corpse_chance(mdef, magr, TRUE) || DEADMONSTER(magr))
            break;

        /* Pets get nutrition from swallowing monster whole.
         * No nutrition from G_NOCORPSE monster, eg, undead.
         * DGST monsters don't die from undead corpses
         */
        num = monsndx(pd);
        if (magr->mtame && !magr->isminion
            && !(mvitals[num].mvflags & G_NOCORPSE)) {
            struct obj *virtualcorpse = mksobj(CORPSE, FALSE, FALSE);
            int nutrit;

            set_corpsenm(virtualcorpse, num);
            nutrit = dog_nutrition(magr, virtualcorpse);
            dealloc_obj(virtualcorpse);

            /* only 50% nutrition, 25% of normal eating time */
            if (magr->meating > 1)
                magr->meating = (magr->meating + 3) / 4;
            if (nutrit > 1)
                nutrit /= 2;
            EDOG(magr)->hungrytime += nutrit;
        }
        break;
    case AD_STUN:
        if (magr->mcan)
            break;
        if (canseemon(mdef))
            pline("%s %s for a moment.", Monnam(mdef),
                  makeplural(stagger(pd, "stagger")));
        mdef->mstun = 1;
        goto physical;
    case AD_LEGS:
        if (magr->mcan) {
            tmp = 0;
            break;
        }
        goto physical;
    case AD_WERE:
    case AD_HEAL:
    case AD_PHYS:
    physical:
        if (mattk->aatyp == AT_KICK && thick_skinned(pd)) {
            tmp = 0;
        } else if (mattk->aatyp == AT_WEAP) {
            if (otmp) {
                struct obj *marmg;

                if (otmp->otyp == CORPSE
                    && touch_petrifies(&mons[otmp->corpsenm]))
                    goto do_stone;
                tmp += dmgval(otmp, mdef);
                if ((marmg = which_armor(magr, W_ARMG)) != 0
                    && marmg->otyp == GAUNTLETS_OF_POWER)
                    tmp += rn1(4, 3); /* 3..6 */
                if (tmp < 1) /* is this necessary?  mhitu.c has it... */
                    tmp = 1;
                if (otmp->oartifact) {
                    (void) artifact_hit(magr, mdef, otmp, &tmp, dieroll);
                    if (DEADMONSTER(mdef))
                        return (MM_DEF_DIED
                                | (grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
                }
                if (tmp)
                    rustm(mdef, otmp);
            }
        } else if (pa == &mons[PM_PURPLE_WORM] && pd == &mons[PM_SHRIEKER]) {
            /* hack to enhance mm_aggression(); we don't want purple
               worm's bite attack to kill a shrieker because then it
               won't swallow the corpse; but if the target survives,
               the subsequent engulf attack should accomplish that */
            if (tmp >= mdef->mhp && mdef->mhp > 1)
                tmp = mdef->mhp - 1;
        }
        break;
    case AD_FIRE:
        if (cancelled) {
            tmp = 0;
            break;
        }
        if (vis && canseemon(mdef))
            pline("%s is %s!", Monnam(mdef), on_fire(pd, mattk));
        if (completelyburns(pd)) { /* paper golem or straw golem */
            if (vis && canseemon(mdef))
                pline("%s burns completely!", Monnam(mdef));
            mondead(mdef); /* was mondied() but that dropped paper scrolls */
            if (!DEADMONSTER(mdef))
                return 0;
            else if (mdef->mtame && !vis)
                pline("May %s roast in peace.", mon_nam(mdef));
            return (MM_DEF_DIED | (grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
        }
        tmp += destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
        tmp += destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
        if (resists_fire(mdef)) {
            if (vis && canseemon(mdef))
                pline_The("fire doesn't seem to burn %s!", mon_nam(mdef));
            shieldeff(mdef->mx, mdef->my);
            golemeffects(mdef, AD_FIRE, tmp);
            tmp = 0;
        }
        /* only potions damage resistant players in destroy_item */
        tmp += destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
        break;
    case AD_COLD:
        if (cancelled) {
            tmp = 0;
            break;
        }
        if (vis && canseemon(mdef))
            pline("%s is covered in frost!", Monnam(mdef));
        if (resists_cold(mdef)) {
            if (vis && canseemon(mdef))
                pline_The("frost doesn't seem to chill %s!", mon_nam(mdef));
            shieldeff(mdef->mx, mdef->my);
            golemeffects(mdef, AD_COLD, tmp);
            tmp = 0;
        }
        tmp += destroy_mitem(mdef, POTION_CLASS, AD_COLD);
        break;
    case AD_ELEC:
        if (cancelled) {
            tmp = 0;
            break;
        }
        if (vis && canseemon(mdef))
            pline("%s gets zapped!", Monnam(mdef));
        tmp += destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
        if (resists_elec(mdef)) {
            if (vis && canseemon(mdef))
                pline_The("zap doesn't shock %s!", mon_nam(mdef));
            shieldeff(mdef->mx, mdef->my);
            golemeffects(mdef, AD_ELEC, tmp);
            tmp = 0;
        }
        /* only rings damage resistant players in destroy_item */
        tmp += destroy_mitem(mdef, RING_CLASS, AD_ELEC);
        break;
    case AD_ACID:
        if (magr->mcan) {
            tmp = 0;
            break;
        }
        if (resists_acid(mdef)) {
            if (vis && canseemon(mdef))
                pline("%s is covered in %s, but it seems harmless.",
                      Monnam(mdef), hliquid("acid"));
            tmp = 0;
        } else if (vis && canseemon(mdef)) {
            pline("%s is covered in %s!", Monnam(mdef), hliquid("acid"));
            pline("It burns %s!", mon_nam(mdef));
        }
        if (!rn2(30))
            erode_armor(mdef, ERODE_CORRODE);
        if (!rn2(6))
            acid_damage(MON_WEP(mdef));
        break;
    case AD_RUST:
        if (magr->mcan)
            break;
        if (pd == &mons[PM_IRON_GOLEM]) {
            if (vis && canseemon(mdef))
                pline("%s falls to pieces!", Monnam(mdef));
            mondied(mdef);
            if (!DEADMONSTER(mdef))
                return 0;
            else if (mdef->mtame && !vis)
                pline("May %s rust in peace.", mon_nam(mdef));
            return (MM_DEF_DIED | (grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
        }
        erode_armor(mdef, ERODE_RUST);
        mdef->mstrategy &= ~STRAT_WAITFORU;
        tmp = 0;
        break;
    case AD_CORR:
        if (magr->mcan)
            break;
        erode_armor(mdef, ERODE_CORRODE);
        mdef->mstrategy &= ~STRAT_WAITFORU;
        tmp = 0;
        break;
    case AD_DCAY:
        if (magr->mcan)
            break;
        if (pd == &mons[PM_WOOD_GOLEM] || pd == &mons[PM_LEATHER_GOLEM]) {
            if (vis && canseemon(mdef))
                pline("%s falls to pieces!", Monnam(mdef));
            mondied(mdef);
            if (!DEADMONSTER(mdef))
                return 0;
            else if (mdef->mtame && !vis)
                pline("May %s rot in peace.", mon_nam(mdef));
            return (MM_DEF_DIED | (grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
        }
        erode_armor(mdef, ERODE_CORRODE);
        mdef->mstrategy &= ~STRAT_WAITFORU;
        tmp = 0;
        break;
    case AD_STON:
        if (magr->mcan)
            break;
    do_stone:
        /* may die from the acid if it eats a stone-curing corpse */
        if (munstone(mdef, FALSE))
            goto post_stone;
        if (poly_when_stoned(pd)) {
            mon_to_stone(mdef);
            tmp = 0;
            break;
        }
        if (!resists_ston(mdef)) {
            if (vis && canseemon(mdef))
                pline("%s turns to stone!", Monnam(mdef));
            monstone(mdef);
        post_stone:
            if (!DEADMONSTER(mdef))
                return 0;
            else if (mdef->mtame && !vis)
                You(brief_feeling, "peculiarly sad");
            return (MM_DEF_DIED | (grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
        }
        tmp = (mattk->adtyp == AD_STON ? 0 : 1);
        break;
    case AD_TLPT:
        if (!cancelled && tmp < mdef->mhp && !tele_restrict(mdef)) {
            char mdef_Monnam[BUFSZ];
            boolean wasseen = canspotmon(mdef);
            /* save the name before monster teleports, otherwise
               we'll get "it" in the suddenly disappears message */
            if (vis && wasseen)
                Strcpy(mdef_Monnam, Monnam(mdef));
            mdef->mstrategy &= ~STRAT_WAITFORU;
            (void) rloc(mdef, TRUE);
            if (vis && wasseen && !canspotmon(mdef) && mdef != u.usteed)
                pline("%s suddenly disappears!", mdef_Monnam);
        }
        break;
    case AD_SLEE:
        if (!cancelled && !mdef->msleeping
            && sleep_monst(mdef, rnd(10), -1)) {
            if (vis && canspotmon(mdef)) {
                Strcpy(buf, Monnam(mdef));
                pline("%s is put to sleep by %s.", buf, mon_nam(magr));
            }
            mdef->mstrategy &= ~STRAT_WAITFORU;
            slept_monst(mdef);
        }
        break;
    case AD_PLYS:
        if (!cancelled && mdef->mcanmove) {
            if (vis && canspotmon(mdef)) {
                Strcpy(buf, Monnam(mdef));
                pline("%s is frozen by %s.", buf, mon_nam(magr));
            }
            paralyze_monst(mdef, rnd(10));
        }
        break;
    case AD_SLOW:
        if (!cancelled && mdef->mspeed != MSLOW) {
            unsigned int oldspeed = mdef->mspeed;

            mon_adjust_speed(mdef, -1, (struct obj *) 0);
            mdef->mstrategy &= ~STRAT_WAITFORU;
            if (mdef->mspeed != oldspeed && vis && canspotmon(mdef))
                pline("%s slows down.", Monnam(mdef));
        }
        break;
    case AD_CONF:
        /* Since confusing another monster doesn't have a real time
         * limit, setting spec_used would not really be right (though
         * we still should check for it).
         */
        if (!magr->mcan && !mdef->mconf && !magr->mspec_used) {
            if (vis && canseemon(mdef))
                pline("%s looks confused.", Monnam(mdef));
            mdef->mconf = 1;
            mdef->mstrategy &= ~STRAT_WAITFORU;
        }
        break;
    case AD_BLND:
        if (can_blnd(magr, mdef, mattk->aatyp, (struct obj *) 0)) {
            register unsigned rnd_tmp;

            if (vis && mdef->mcansee && canspotmon(mdef))
                pline("%s is blinded.", Monnam(mdef));
            rnd_tmp = d((int) mattk->damn, (int) mattk->damd);
            if ((rnd_tmp += mdef->mblinded) > 127)
                rnd_tmp = 127;
            mdef->mblinded = rnd_tmp;
            mdef->mcansee = 0;
            mdef->mstrategy &= ~STRAT_WAITFORU;
        }
        tmp = 0;
        break;
    case AD_HALU:
        if (!magr->mcan && haseyes(pd) && mdef->mcansee) {
            if (vis && canseemon(mdef))
                pline("%s looks %sconfused.", Monnam(mdef),
                      mdef->mconf ? "more " : "");
            mdef->mconf = 1;
            mdef->mstrategy &= ~STRAT_WAITFORU;
        }
        tmp = 0;
        break;
    case AD_CURS:
        if (!night() && (pa == &mons[PM_GREMLIN]))
            break;
        if (!magr->mcan && !rn2(10)) {
            mdef->mcan = 1; /* cancelled regardless of lifesave */
            mdef->mstrategy &= ~STRAT_WAITFORU;
            if (is_were(pd) && pd->mlet != S_HUMAN)
                were_change(mdef);
            if (pd == &mons[PM_CLAY_GOLEM]) {
                if (vis && canseemon(mdef)) {
                    pline("Some writing vanishes from %s head!",
                          s_suffix(mon_nam(mdef)));
                    pline("%s is destroyed!", Monnam(mdef));
                }
                mondied(mdef);
                if (!DEADMONSTER(mdef))
                    return 0;
                else if (mdef->mtame && !vis)
                    You(brief_feeling, "strangely sad");
                return (MM_DEF_DIED
                        | (grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
            }
            if (!Deaf) {
                if (!vis)
                    You_hear("laughter.");
                else if (canseemon(magr))
                    pline("%s chuckles.", Monnam(magr));
            }
        }
        break;
    case AD_SGLD:
        tmp = 0;
        if (magr->mcan)
            break;
        /* technically incorrect; no check for stealing gold from
         * between mdef's feet...
         */
        {
            struct obj *gold = findgold(mdef->minvent);

            if (!gold)
                break;
            obj_extract_self(gold);
            add_to_minv(magr, gold);
        }
        mdef->mstrategy &= ~STRAT_WAITFORU;
        if (vis && canseemon(mdef)) {
            Strcpy(buf, Monnam(magr));
            pline("%s steals some gold from %s.", buf, mon_nam(mdef));
        }
        if (!tele_restrict(magr)) {
            boolean couldspot = canspotmon(magr);
            (void) rloc(magr, TRUE);
            if (vis && couldspot && !canspotmon(magr))
                pline("%s suddenly disappears!", buf);
        }
        break;
    case AD_DRLI:
        if (!cancelled && !rn2(3) && !resists_drli(mdef)) {
            tmp = d(2, 6);
            if (vis && canspotmon(mdef))
                pline("%s suddenly seems weaker!", Monnam(mdef));
            mdef->mhpmax -= tmp;
            if (mdef->m_lev == 0)
                tmp = mdef->mhp;
            else
                mdef->m_lev--;
            /* Automatic kill if drained past level 0 */
        }
        break;
    case AD_SSEX:
    case AD_SITM: /* for now these are the same */
    case AD_SEDU:
        if (magr->mcan)
            break;
        /* find an object to steal, non-cursed if magr is tame */
        for (obj = mdef->minvent; obj; obj = obj->nobj)
            if (!magr->mtame || !obj->cursed)
                break;

        if (obj) {
            char onambuf[BUFSZ], mdefnambuf[BUFSZ];

            /* make a special x_monnam() call that never omits
               the saddle, and save it for later messages */
            Strcpy(mdefnambuf,
                   x_monnam(mdef, ARTICLE_THE, (char *) 0, 0, FALSE));

            otmp = obj;
            if (u.usteed == mdef && otmp == which_armor(mdef, W_SADDLE))
                /* "You can no longer ride <steed>." */
                dismount_steed(DISMOUNT_POLY);
            obj_extract_self(otmp);
            if (otmp->owornmask) {
                mdef->misc_worn_check &= ~otmp->owornmask;
                if (otmp->owornmask & W_WEP)
                    mwepgone(mdef);
                otmp->owornmask = 0L;
                update_mon_intrinsics(mdef, otmp, FALSE, FALSE);
                /* give monster a chance to wear other equipment on its next
                   move instead of waiting until it picks something up */
                mdef->misc_worn_check |= I_SPECIAL;
            }
            /* add_to_minv() might free otmp [if it merges] */
            if (vis)
                Strcpy(onambuf, doname(otmp));
            (void) add_to_minv(magr, otmp);
            if (vis && canseemon(mdef)) {
                Strcpy(buf, Monnam(magr));
                pline("%s steals %s from %s!", buf, onambuf, mdefnambuf);
            }
            possibly_unwield(mdef, FALSE);
            mdef->mstrategy &= ~STRAT_WAITFORU;
            mselftouch(mdef, (const char *) 0, FALSE);
            if (DEADMONSTER(mdef))
                return (MM_DEF_DIED
                        | (grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
            if (pa->mlet == S_NYMPH && !tele_restrict(magr)) {
                boolean couldspot = canspotmon(magr);

                (void) rloc(magr, TRUE);
                if (vis && couldspot && !canspotmon(magr))
                    pline("%s suddenly disappears!", buf);
            }
        }
        tmp = 0;
        break;
    case AD_DREN:
        if (!cancelled && !rn2(4))
            xdrainenergym(mdef, (boolean) (vis && canspotmon(mdef)
                                           && mattk->aatyp != AT_ENGL));
        tmp = 0;
        break;
    case AD_DRST:
    case AD_DRDX:
    case AD_DRCO:
        if (!cancelled && !rn2(8)) {
            if (vis && canspotmon(magr))
                pline("%s %s was poisoned!", s_suffix(Monnam(magr)),
                      mpoisons_subj(magr, mattk));
            if (resists_poison(mdef)) {
                if (vis && canspotmon(mdef) && canspotmon(magr))
                    pline_The("poison doesn't seem to affect %s.",
                              mon_nam(mdef));
            } else {
                if (rn2(10))
                    tmp += rn1(10, 6);
                else {
                    if (vis && canspotmon(mdef))
                        pline_The("poison was deadly...");
                    tmp = mdef->mhp;
                }
            }
        }
        break;
    case AD_DRIN:
        if (notonhead || !has_head(pd)) {
            if (vis && canspotmon(mdef))
                pline("%s doesn't seem harmed.", Monnam(mdef));
            /* Not clear what to do for green slimes */
            tmp = 0;
            break;
        }
        if ((mdef->misc_worn_check & W_ARMH) && rn2(8)) {
            if (vis && canspotmon(magr) && canseemon(mdef)) {
                Strcpy(buf, s_suffix(Monnam(mdef)));
                pline("%s helmet blocks %s attack to %s head.", buf,
                      s_suffix(mon_nam(magr)), mhis(mdef));
            }
            break;
        }
        res = eat_brains(magr, mdef, vis, &tmp);
        break;
    case AD_SLIM:
        if (cancelled)
            break; /* physical damage only */
        if (!rn2(4) && !slimeproof(pd)) {
            if (!munslime(mdef, FALSE) && !DEADMONSTER(mdef)) {
                if (newcham(mdef, &mons[PM_GREEN_SLIME], FALSE, vis && canseemon(mdef)))
                    pd = mdef->data;
                mdef->mstrategy &= ~STRAT_WAITFORU;
                res = MM_HIT;
            }
            /* munslime attempt could have been fatal,
               potentially to multiple monsters (SCR_FIRE) */
            if (DEADMONSTER(magr))
                res |= MM_AGR_DIED;
            if (DEADMONSTER(mdef))
                res |= MM_DEF_DIED;
            tmp = 0;
        }
        break;
    case AD_STCK:
        if (cancelled)
            tmp = 0;
        break;
    case AD_WRAP: /* monsters cannot grab one another, it's too hard */
        if (magr->mcan)
            tmp = 0;
        break;
    case AD_ENCH:
        /* there's no msomearmor() function, so just do damage */
        /* if (cancelled) break; */
        break;
    default:
        tmp = 0;
        break;
    }
    if (!tmp)
        return res;

    if ((mdef->mhp -= tmp) < 1) {
        if (m_at(mdef->mx, mdef->my) == magr) { /* see gulpmm() */
            remove_monster(mdef->mx, mdef->my);
            mdef->mhp = 1; /* otherwise place_monster will complain */
            place_monster(mdef, mdef->mx, mdef->my);
            mdef->mhp = 0;
        }
        monkilled(mdef, "", (int) mattk->adtyp);
        if (!DEADMONSTER(mdef))
            return res; /* mdef lifesaved */
        else if (res == MM_AGR_DIED)
            return (MM_DEF_DIED | MM_AGR_DIED);

        if (mattk->adtyp == AD_DGST) {
            /* various checks similar to dog_eat and meatobj.
             * after monkilled() to provide better message ordering */
            if (mdef->cham >= LOW_PM) {
                (void) newcham(magr, (struct permonst *) 0, FALSE, TRUE);
            } else if (pd == &mons[PM_GREEN_SLIME] && !slimeproof(pa)) {
                (void) newcham(magr, &mons[PM_GREEN_SLIME], FALSE, TRUE);
            } else if (pd == &mons[PM_WRAITH]) {
                (void) grow_up(magr, (struct monst *) 0);
                /* don't grow up twice */
                return (MM_DEF_DIED | (!DEADMONSTER(magr) ? 0 : MM_AGR_DIED));
            } else if (pd == &mons[PM_NURSE]) {
                magr->mhp = magr->mhpmax;
            }
        }
        /* caveat: above digestion handling doesn't keep `pa' up to date */

        return (MM_DEF_DIED | (grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
    }
    return (res == MM_AGR_DIED) ? MM_AGR_DIED : MM_HIT;
}

void
paralyze_monst(mon, amt)
struct monst *mon;
int amt;
{
    if (amt > 127)
        amt = 127;

    mon->mcanmove = 0;
    mon->mfrozen = amt;
    mon->meating = 0; /* terminate any meal-in-progress */
    mon->mstrategy &= ~STRAT_WAITFORU;
}

/* `mon' is hit by a sleep attack; return 1 if it's affected, 0 otherwise */
int
sleep_monst(mon, amt, how)
struct monst *mon;
int amt, how;
{
    if (resists_sleep(mon)
        || (how >= 0 && resist(mon, (char) how, 0, NOTELL))) {
        shieldeff(mon->mx, mon->my);
    } else if (mon->mcanmove) {
        finish_meating(mon); /* terminate any meal-in-progress */
        amt += (int) mon->mfrozen;
        if (amt > 0) { /* sleep for N turns */
            mon->mcanmove = 0;
            mon->mfrozen = min(amt, 127);
        } else { /* sleep until awakened */
            mon->msleeping = 1;
        }
        return 1;
    }
    return 0;
}

/* sleeping grabber releases, engulfer doesn't; don't use for paralysis! */
void
slept_monst(mon)
struct monst *mon;
{
    if ((mon->msleeping || !mon->mcanmove) && mon == u.ustuck
        && !sticks(youmonst.data) && !u.uswallow) {
        pline("%s grip relaxes.", s_suffix(Monnam(mon)));
        unstuck(mon);
    }
}

void
rustm(mdef, obj)
struct monst *mdef;
struct obj *obj;
{
    int dmgtyp = -1, chance = 1;

    if (!mdef || !obj)
        return; /* just in case */
    /* AD_ACID and AD_ENCH are handled in passivemm() and passiveum() */
    if (dmgtype(mdef->data, AD_CORR)) {
        dmgtyp = ERODE_CORRODE;
    } else if (dmgtype(mdef->data, AD_RUST)) {
        dmgtyp = ERODE_RUST;
    } else if (dmgtype(mdef->data, AD_FIRE)
               /* steam vortex: fire resist applies, fire damage doesn't */
               && mdef->data != &mons[PM_STEAM_VORTEX]) {
        dmgtyp = ERODE_BURN;
        chance = 6;
    }

    if (dmgtyp >= 0 && !rn2(chance))
        (void) erode_obj(obj, (char *) 0, dmgtyp, EF_GREASE | EF_VERBOSE);
}

STATIC_OVL void
mswingsm(magr, mdef, otemp)
struct monst *magr, *mdef;
struct obj *otemp;
{
    if (flags.verbose && !Blind && mon_visible(magr)) {
        pline("%s %s %s%s %s at %s.", Monnam(magr),
              (objects[otemp->otyp].oc_dir & PIERCE) ? "thrusts" : "swings",
              (otemp->quan > 1L) ? "one of " : "", mhis(magr), xname(otemp),
              mon_nam(mdef));
    }
}

/*
 * Passive responses by defenders.  Does not replicate responses already
 * handled above.  Returns same values as mattackm.
 */
STATIC_OVL int
passivemm(magr, mdef, mhit, mdead)
register struct monst *magr, *mdef;
boolean mhit;
int mdead;
{
    register struct permonst *mddat = mdef->data;
    register struct permonst *madat = magr->data;
    char buf[BUFSZ];
    int i, tmp;

    for (i = 0;; i++) {
        if (i >= NATTK)
            return (mdead | mhit); /* no passive attacks */
        if (mddat->mattk[i].aatyp == AT_NONE)
            break;
    }
    if (mddat->mattk[i].damn)
        tmp = d((int) mddat->mattk[i].damn, (int) mddat->mattk[i].damd);
    else if (mddat->mattk[i].damd)
        tmp = d((int) mddat->mlevel + 1, (int) mddat->mattk[i].damd);
    else
        tmp = 0;

    /* These affect the enemy even if defender killed */
    switch (mddat->mattk[i].adtyp) {
    case AD_ACID:
        if (mhit && !rn2(2)) {
            Strcpy(buf, Monnam(magr));
            if (canseemon(magr))
                pline("%s is splashed by %s %s!", buf,
                      s_suffix(mon_nam(mdef)), hliquid("acid"));
            if (resists_acid(magr)) {
                if (canseemon(magr))
                    pline("%s is not affected.", Monnam(magr));
                tmp = 0;
            }
        } else
            tmp = 0;
        if (!rn2(30))
            erode_armor(magr, ERODE_CORRODE);
        if (!rn2(6))
            acid_damage(MON_WEP(magr));
        goto assess_dmg;
    case AD_ENCH: /* KMH -- remove enchantment (disenchanter) */
        if (mhit && !mdef->mcan && otmp) {
            (void) drain_item(otmp, FALSE);
            /* No message */
        }
        break;
    default:
        break;
    }
    if (mdead || mdef->mcan)
        return (mdead | mhit);

    /* These affect the enemy only if defender is still alive */
    if (rn2(3))
        switch (mddat->mattk[i].adtyp) {
        case AD_PLYS: /* Floating eye */
            if (tmp > 127)
                tmp = 127;
            if (mddat == &mons[PM_FLOATING_EYE]) {
                if (!rn2(4))
                    tmp = 127;
                if (magr->mcansee && haseyes(madat) && mdef->mcansee
                    && (perceives(madat) || !mdef->minvis)) {
                    /* construct format string; guard against '%' in Monnam */
                    Strcpy(buf, s_suffix(Monnam(mdef)));
                    (void) strNsubst(buf, "%", "%%", 0);
                    Strcat(buf, " gaze is reflected by %s %s.");
                    if (mon_reflects(magr,
                                     canseemon(magr) ? buf : (char *) 0))
                        return (mdead | mhit);
                    Strcpy(buf, Monnam(magr));
                    if (canseemon(magr))
                        pline("%s is frozen by %s gaze!", buf,
                              s_suffix(mon_nam(mdef)));
                    paralyze_monst(magr, tmp);
                    return (mdead | mhit);
                }
            } else { /* gelatinous cube */
                Strcpy(buf, Monnam(magr));
                if (canseemon(magr))
                    pline("%s is frozen by %s.", buf, mon_nam(mdef));
                paralyze_monst(magr, tmp);
                return (mdead | mhit);
            }
            return 1;
        case AD_COLD:
            if (resists_cold(magr)) {
                if (canseemon(magr)) {
                    pline("%s is mildly chilly.", Monnam(magr));
                    golemeffects(magr, AD_COLD, tmp);
                }
                tmp = 0;
                break;
            }
            if (canseemon(magr))
                pline("%s is suddenly very cold!", Monnam(magr));
            mdef->mhp += tmp / 2;
            if (mdef->mhpmax < mdef->mhp)
                mdef->mhpmax = mdef->mhp;
            if (mdef->mhpmax > ((int) (mdef->m_lev + 1) * 8))
                (void) split_mon(mdef, magr);
            break;
        case AD_STUN:
            if (!magr->mstun) {
                magr->mstun = 1;
                if (canseemon(magr))
                    pline("%s %s...", Monnam(magr),
                          makeplural(stagger(magr->data, "stagger")));
            }
            tmp = 0;
            break;
        case AD_FIRE:
            if (resists_fire(magr)) {
                if (canseemon(magr)) {
                    pline("%s is mildly warmed.", Monnam(magr));
                    golemeffects(magr, AD_FIRE, tmp);
                }
                tmp = 0;
                break;
            }
            if (canseemon(magr))
                pline("%s is suddenly very hot!", Monnam(magr));
            break;
        case AD_ELEC:
            if (resists_elec(magr)) {
                if (canseemon(magr)) {
                    pline("%s is mildly tingled.", Monnam(magr));
                    golemeffects(magr, AD_ELEC, tmp);
                }
                tmp = 0;
                break;
            }
            if (canseemon(magr))
                pline("%s is jolted with electricity!", Monnam(magr));
            break;
        default:
            tmp = 0;
            break;
        }
    else
        tmp = 0;

assess_dmg:
    if ((magr->mhp -= tmp) <= 0) {
        monkilled(magr, "", (int) mddat->mattk[i].adtyp);
        return (mdead | mhit | MM_AGR_DIED);
    }
    return (mdead | mhit);
}

/* hero or monster has successfully hit target mon with drain energy attack */
void
xdrainenergym(mon, givemsg)
struct monst *mon;
boolean givemsg;
{
    if (mon->mspec_used < 20 /* limit draining */
        && (attacktype(mon->data, AT_MAGC)
            || attacktype(mon->data, AT_BREA))) {
        mon->mspec_used += d(2, 2);
        if (givemsg)
            pline("%s seems lethargic.", Monnam(mon));
    }
}

/* "aggressive defense"; what type of armor prevents specified attack
   from touching its target? */
long
attk_protection(aatyp)
int aatyp;
{
    long w_mask = 0L;

    switch (aatyp) {
    case AT_NONE:
    case AT_SPIT:
    case AT_EXPL:
    case AT_BOOM:
    case AT_GAZE:
    case AT_BREA:
    case AT_MAGC:
        w_mask = ~0L; /* special case; no defense needed */
        break;
    case AT_CLAW:
    case AT_TUCH:
    case AT_WEAP:
        w_mask = W_ARMG; /* caller needs to check for weapon */
        break;
    case AT_KICK:
        w_mask = W_ARMF;
        break;
    case AT_BUTT:
        w_mask = W_ARMH;
        break;
    case AT_HUGS:
        w_mask = (W_ARMC | W_ARMG); /* attacker needs both to be protected */
        break;
    case AT_BITE:
    case AT_STNG:
    case AT_ENGL:
    case AT_TENT:
    default:
        w_mask = 0L; /* no defense available */
        break;
    }
    return w_mask;
}

/*mhitm.c*/

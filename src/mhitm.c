/* NetHack 3.7	mhitm.c	$NHDT-Date: 1606558747 2020/11/28 10:19:07 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.145 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

static const char brief_feeling[] =
    "have a %s feeling for a moment, then it passes.";

static int FDECL(hitmm, (struct monst *, struct monst *,
                         struct attack *, struct obj *, int));
static int FDECL(gazemm, (struct monst *, struct monst *, struct attack *));
static int FDECL(gulpmm, (struct monst *, struct monst *, struct attack *));
static int FDECL(explmm, (struct monst *, struct monst *, struct attack *));
static int FDECL(mdamagem, (struct monst *, struct monst *,
                            struct attack *, struct obj *, int));
static void FDECL(mswingsm, (struct monst *, struct monst *, struct obj *));
static void FDECL(noises, (struct monst *, struct attack *));
static void FDECL(pre_mm_attack, (struct monst *, struct monst *));
static void FDECL(missmm, (struct monst *, struct monst *, struct attack *));
static int FDECL(passivemm, (struct monst *, struct monst *,
                             BOOLEAN_P, int, struct obj *));

static void
noises(magr, mattk)
register struct monst *magr;
register struct attack *mattk;
{
    boolean farq = (distu(magr->mx, magr->my) > 15);

    if (!Deaf && (farq != g.far_noise || g.moves - g.noisetime > 10)) {
        g.far_noise = farq;
        g.noisetime = g.moves;
        You_hear("%s%s.",
                 (mattk->aatyp == AT_EXPL) ? "an explosion" : "some noises",
                 farq ? " in the distance" : "");
    }
}

static void
pre_mm_attack(magr, mdef)
struct monst *magr, *mdef;
{
    boolean showit = FALSE;

    /* unhiding or unmimicking happens even if hero can't see it
       because the formerly concealed monster is now in action */
    if (M_AP_TYPE(mdef)) {
        seemimic(mdef);
        showit |= g.vis;
    } else if (mdef->mundetected) {
        mdef->mundetected = 0;
        showit |= g.vis;
    }
    if (M_AP_TYPE(magr)) {
        seemimic(magr);
        showit |= g.vis;
    } else if (magr->mundetected) {
        magr->mundetected = 0;
        showit |= g.vis;
    }

    if (g.vis) {
        if (!canspotmon(magr))
            map_invisible(magr->mx, magr->my);
        else if (showit)
            newsym(magr->mx, magr->my);
        if (!canspotmon(mdef))
            map_invisible(mdef->mx, mdef->my);
        else if (showit)
            newsym(mdef->mx, mdef->my);
    }
}

static
void
missmm(magr, mdef, mattk)
register struct monst *magr, *mdef;
struct attack *mattk;
{
    const char *fmt;
    char buf[BUFSZ];

    pre_mm_attack(magr, mdef);

    if (g.vis) {
        fmt = (could_seduce(magr, mdef, mattk) && !magr->mcan)
                  ? "%s pretends to be friendly to"
                  : "%s misses";
        Sprintf(buf, fmt, Monnam(magr));
        pline("%s %s.", buf, mon_nam_too(mdef, magr));
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
                        set_ustuck((struct monst *) 0);
                        pline("%s releases you!", Monnam(mtmp));
                    } else
                        break;
                }

                /* mtmp can be killed */
                g.bhitpos.x = mon->mx;
                g.bhitpos.y = mon->my;
                g.notonhead = 0;
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
                    g.notonhead = 0;
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
    if (M_AP_TYPE(mdef) && M_AP_TYPE(mdef) != M_AP_MONSTER)
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
    g.vis = (canspotmon(magr) && canspotmon(mdef));

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
            else if (magr->mtame && !g.vis)
                You(brief_feeling, "peculiarly sad");
            return MM_AGR_DIED;
        }
    }

    remove_monster(fx, fy); /* pick up from orig position */
    remove_monster(tx, ty);
    place_monster(magr, tx, ty); /* put down at target spot */
    place_monster(mdef, fx, fy);
    if (g.vis && !quietly)
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
 *      0x8     MM_AGR_DONE
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
        res[NATTK], /* results of all attacks */
        dieroll = 0;
    struct attack *mattk, alt_attk;
    struct obj *mwep;
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
    g.vis = ((cansee(magr->mx, magr->my) && canspotmon(magr))
             || (cansee(mdef->mx, mdef->my) && canspotmon(mdef)));

    /* Set flag indicating monster has moved this turn.  Necessary since a
     * monster might get an attack out of sequence (i.e. before its move) in
     * some cases, in which case this still counts as its move for the round
     * and it shouldn't move again.
     */
    magr->mlstmv = g.monstermoves;

    /* controls whether a mind flayer uses all of its tentacle-for-DRIN
       attacks; when fighting a headless monster, stop after the first
       one because repeating the same failing hit (or even an ordinary
       tentacle miss) is very verbose and makes the flayer look stupid */
    g.skipdrin = FALSE;

    /* Now perform all attacks for the monster. */
    for (i = 0; i < NATTK; i++) {
        res[i] = MM_MISS;
        mattk = getmattk(magr, mdef, i, res, &alt_attk);
        mwep = (struct obj *) 0;
        attk = 1;
        /* reduce verbosity for mind flayer attacking creature without a
           head (or worm's tail); this is similar to monster with multiple
           attacks after a wildmiss against displaced or invisible hero */
        if (g.skipdrin && mattk->aatyp == AT_TENT && mattk->adtyp == AD_DRIN)
            continue;

        switch (mattk->aatyp) {
        case AT_WEAP: /* "hand to hand" attacks */
            if (distmin(magr->mx, magr->my, mdef->mx, mdef->my) > 1) {
                /* D: Do a ranged attack here! */
                strike = (thrwmm(magr, mdef) == MM_MISS) ? 0 : 1;
                if (strike)
                    /* don't really know if we hit or not; pretend we did */
                    res[i] |= MM_HIT;
                if (DEADMONSTER(mdef))
                    res[i] = MM_DEF_DIED;
                if (DEADMONSTER(magr))
                    res[i] |= MM_AGR_DIED;
                break;
            }
            if (magr->weapon_check == NEED_WEAPON || !MON_WEP(magr)) {
                magr->weapon_check = NEED_HTH_WEAPON;
                if (mon_wield_item(magr) != 0)
                    return MM_MISS;
            }
            possibly_unwield(magr, FALSE);
            if ((mwep = MON_WEP(magr)) != 0) {
                if (g.vis)
                    mswingsm(magr, mdef, mwep);
                tmp += hitval(mwep, mdef);
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
            if (!magr->mconf && !Conflict && mwep && mattk->aatyp != AT_WEAP
                && touch_petrifies(mdef->data)) {
                strike = 0;
                break;
            }
            dieroll = rnd(20 + i);
            strike = (tmp > dieroll);
            /* KMH -- don't accumulate to-hit bonuses */
            if (mwep)
                tmp -= hitval(mwep, mdef);
            if (strike) {
                res[i] = hitmm(magr, mdef, mattk, mwep, dieroll);
                if ((mdef->data == &mons[PM_BLACK_PUDDING]
                     || mdef->data == &mons[PM_BROWN_PUDDING])
                    && (mwep && (objects[mwep->otyp].oc_material == IRON
                                 || objects[mwep->otyp].oc_material == METAL))
                    && mdef->mhp > 1 && !mdef->mcan) {
                    struct monst *mclone;

                    if ((mclone = clone_mon(mdef, 0, 0)) != 0) {
                        if (g.vis && canspotmon(mdef)) {
                            char buf[BUFSZ];

                            Strcpy(buf, Monnam(mdef));
                            pline("%s divides as %s hits it!", buf,
                                  mon_nam(magr));
                        }
                        mintrap(mclone);
                    }
                }
            } else
                missmm(magr, mdef, mattk);
            break;

        case AT_HUGS: /* automatic if prev two attacks succeed */
            strike = (i >= 2 && res[i - 1] == MM_HIT && res[i - 2] == MM_HIT);
            if (strike)
                res[i] = hitmm(magr, mdef, mattk, (struct obj *) 0, 0);

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
            if (mdef->data == &mons[PM_SHADE]) { /* no silver teeth... */
                if (g.vis)
                    pline("%s attempt to engulf %s is futile.",
                          s_suffix(Monnam(magr)), mon_nam(mdef));
                strike = 0;
                break;
            }
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
                strike = (breamm(magr, mattk, mdef) == MM_MISS) ? 0 : 1;

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
                strike = (spitmm(magr, mattk, mdef) == MM_MISS) ? 0 : 1;

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
            res[i] = passivemm(magr, mdef, strike,
                               (res[i] & MM_DEF_DIED), mwep);

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
static int
hitmm(magr, mdef, mattk, mwep, dieroll)
register struct monst *magr, *mdef;
struct attack *mattk;
struct obj *mwep;
int dieroll;
{
    boolean weaponhit = (mattk->aatyp == AT_WEAP
                         || (mattk->aatyp == AT_CLAW && mwep)),
            silverhit = (weaponhit && mwep
                         && objects[mwep->otyp].oc_material == SILVER);

    pre_mm_attack(magr, mdef);

    if (g.vis) {
        int compat;
        char buf[BUFSZ];

        if ((compat = could_seduce(magr, mdef, mattk)) && !magr->mcan) {
            Sprintf(buf, "%s %s", Monnam(magr),
                    mdef->mcansee ? "smiles at" : "talks to");
            pline("%s %s %s.", buf, mon_nam(mdef),
                  compat == 2 ? "engagingly" : "seductively");
        } else if (shade_miss(magr, mdef, mwep, FALSE, TRUE)) {
            return MM_MISS; /* bypass mdamagem() */
        } else {
            char magr_name[BUFSZ];

            Strcpy(magr_name, Monnam(magr));
            buf[0] = '\0';
            switch (mattk->aatyp) {
            case AT_BITE:
                Snprintf(buf, sizeof(buf), "%s bites", magr_name);
                break;
            case AT_STNG:
                Snprintf(buf, sizeof(buf), "%s stings", magr_name);
                break;
            case AT_BUTT:
                Snprintf(buf, sizeof(buf), "%s butts", magr_name);
                break;
            case AT_TUCH:
                Snprintf(buf, sizeof(buf), "%s touches", magr_name);
                break;
            case AT_TENT:
                Snprintf(buf, sizeof(buf), "%s tentacles suck", s_suffix(magr_name));
                break;
            case AT_HUGS:
                if (magr != u.ustuck) {
                    Snprintf(buf, sizeof(buf), "%s squeezes", magr_name);
                    break;
                }
                /*FALLTHRU*/
            default:
                if (!weaponhit || !mwep || !mwep->oartifact)
                    Snprintf(buf, sizeof(buf), "%s hits", magr_name);
                break;
            }
            if (*buf)
                pline("%s %s.", buf, mon_nam_too(mdef, magr));

            if (mon_hates_silver(mdef) && silverhit) {
                char *mdef_name = mon_nam_too(mdef, magr);

                /* note: mon_nam_too returns a modifiable buffer; so
                   does s_suffix, but it returns a single static buffer
                   and we might be calling it twice for this message */
                Strcpy(magr_name, s_suffix(magr_name));
                if (!noncorporeal(mdef->data) && !amorphous(mdef->data)) {
                    if (mdef != magr) {
                        mdef_name = s_suffix(mdef_name);
                    } else {
                        (void) strsubst(mdef_name, "himself", "his own");
                        (void) strsubst(mdef_name, "herself", "her own");
                        (void) strsubst(mdef_name, "itself", "its own");
                    }
                    Strcat(mdef_name, " flesh");
                }

                pline("%s %s sears %s!", magr_name, /*s_suffix(magr_name), */
                      simpleonames(mwep), mdef_name);
            }
        }
    } else
        noises(magr, mattk);

    return mdamagem(magr, mdef, mattk, mwep, dieroll);
}

/* Returns the same values as mdamagem(). */
static int
gazemm(magr, mdef, mattk)
register struct monst *magr, *mdef;
struct attack *mattk;
{
    char buf[BUFSZ];

    if (g.vis) {
        if (mdef->data->mlet == S_MIMIC
            && M_AP_TYPE(mdef) != M_AP_NOTHING)
            seemimic(mdef);
        Sprintf(buf, "%s gazes at", Monnam(magr));
        pline("%s %s...", buf,
              canspotmon(mdef) ? mon_nam(mdef) : "something");
    }

    if (magr->mcan || !magr->mcansee || !mdef->mcansee
        || (magr->minvis && !perceives(mdef->data)) || mdef->msleeping) {
        if (g.vis && canspotmon(mdef))
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

    return mdamagem(magr, mdef, mattk, (struct obj *) 0, 0);
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
    if ((magr == &g.youmonst) ? Passes_walls : passes_walls(magr->data))
        return TRUE;

    /* don't swallow something in a spot where attacker wouldn't
       otherwise be able to move onto; we don't want to engulf
       a wall-phaser and end up with a non-phaser inside a wall */
    dx = mdef->mx, dy = mdef->my;
    if (mdef == &g.youmonst)
        dx = u.ux, dy = u.uy;
    lev = &levl[dx][dy];
    if (IS_ROCK(lev->typ) || closed_door(dx, dy) || IS_TREE(lev->typ)
        /* not passes_bars(); engulfer isn't squeezing through */
        || (lev->typ == IRONBARS && !is_whirly(magr->data)))
        return FALSE;

    return TRUE;
}

/* Returns the same values as mattackm(). */
static int
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

    if (g.vis) {
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
        if (g.vis) {
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
    remove_monster(dx, dy);
    remove_monster(ax, ay);
    place_monster(magr, dx, dy);
    newsym(ax, ay); /* erase old position */
    newsym(dx, dy); /* update new position */

    status = mdamagem(magr, mdef, mattk, (struct obj *) 0, 0);

    if ((status & (MM_AGR_DIED | MM_DEF_DIED))
        == (MM_AGR_DIED | MM_DEF_DIED)) {
        ;                              /* both died -- do nothing  */
    } else if (status & MM_DEF_DIED) { /* defender died */
        /*
         *  Note:  remove_monster() was called in relmon(), wiping out
         *  magr from level.monsters[mdef->mx][mdef->my].  We need to
         *  put it back and display it.  -kd
         */
        if (!goodpos(dx, dy, magr, MM_IGNOREWATER))
            dx = ax, dy = ay;
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

static int
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

    result = mdamagem(magr, mdef, mattk, (struct obj *) 0, 0);

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
static int
mdamagem(magr, mdef, mattk, mwep, dieroll)
struct monst *magr, *mdef;
struct attack *mattk;
struct obj *mwep;
int dieroll;
{
    struct permonst *pa = magr->data, *pd = mdef->data;
    int armpro;
    boolean cancelled;
    struct mhitm_data mhm;
    mhm.damage = d((int) mattk->damn, (int) mattk->damd);
    mhm.hitflags = MM_MISS;
    mhm.permdmg = 0;
    mhm.specialdmg = 0;
    mhm.dieroll = dieroll;
    mhm.done = FALSE;

    if ((touch_petrifies(pd) /* or flesh_petrifies() */
         || (mattk->adtyp == AD_DGST && pd == &mons[PM_MEDUSA]))
        && !resists_ston(magr)) {
        long protector = attk_protection((int) mattk->aatyp),
             wornitems = magr->misc_worn_check;

        /* wielded weapon gives same protection as gloves here */
        if (mwep)
            wornitems |= W_ARMG;

        if (protector == 0L
            || (protector != ~0L && (wornitems & protector) != protector)) {
            if (poly_when_stoned(pa)) {
                mon_to_stone(magr);
                return MM_HIT; /* no damage during the polymorph */
            }
            if (g.vis && canspotmon(magr))
                pline("%s turns to stone!", Monnam(magr));
            monstone(magr);
            if (!DEADMONSTER(magr))
                return MM_HIT; /* lifesaved */
            else if (magr->mtame && !g.vis)
                You(brief_feeling, "peculiarly sad");
            return MM_AGR_DIED;
        }
    }

    /* cancellation factor is the same as when attacking the hero */
    armpro = magic_negation(mdef);
    cancelled = magr->mcan || !(rn2(10) >= 3 * armpro);

    mhitm_adtyping(magr, mattk, mdef, &mhm);
    if (mhm.done)
        return mhm.hitflags;

    if (!mhm.damage)
        return mhm.hitflags;

    if ((mdef->mhp -= mhm.damage) < 1) {
        if (m_at(mdef->mx, mdef->my) == magr) { /* see gulpmm() */
            remove_monster(mdef->mx, mdef->my);
            mdef->mhp = 1; /* otherwise place_monster will complain */
            place_monster(mdef, mdef->mx, mdef->my);
            mdef->mhp = 0;
        }
        g.zombify = !mwep && zombie_maker(magr->data)
            && ((mattk->aatyp == AT_TUCH
                 || mattk->aatyp == AT_CLAW
                 || mattk->aatyp == AT_BITE)
                && zombie_form(mdef->data) != NON_PM);
        monkilled(mdef, "", (int) mattk->adtyp);
        g.zombify = FALSE; /* reset */
        if (!DEADMONSTER(mdef))
            return mhm.hitflags; /* mdef lifesaved */
        else if (mhm.hitflags == MM_AGR_DIED)
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
    return (mhm.hitflags == MM_AGR_DIED) ? MM_AGR_DIED : MM_HIT;
}

int
mon_poly(magr, mdef, dmg)
struct monst *magr, *mdef;
int dmg;
{
    if (mdef == &g.youmonst) {
        if (Antimagic) {
            shieldeff(mdef->mx, mdef->my);
        } else if (Unchanging) {
            ; /* just take a little damage */
        } else {
            /* system shock might take place in polyself() */
            if (u.ulycn == NON_PM) {
                You("are subjected to a freakish metamorphosis.");
                polyself(0);
            } else if (u.umonnum != u.ulycn) {
                You_feel("an unnatural urge coming on.");
                you_were();
            } else {
                You_feel("a natural urge coming on.");
                you_unwere(FALSE);
            }
            dmg = 0;
        }
    } else {
        char Before[BUFSZ];

        Strcpy(Before, Monnam(mdef));
        if (resists_magm(mdef)) {
            /* Magic resistance */
            if (g.vis)
                shieldeff(mdef->mx, mdef->my);
        } else if (resist(mdef, WAND_CLASS, 0, TELL)) {
            /* general resistance to magic... */
            ;
        } else if (!rn2(25) && mdef->cham == NON_PM
                   && (mdef->mcan
                       || pm_to_cham(monsndx(mdef->data)) != NON_PM)) {
            /* system shock; this variation takes away half of mon's HP
               rather than kill outright */
            if (g.vis)
                pline("%s shudders!", Before);

            dmg += (mdef->mhpmax + 1) / 2;
            mdef->mhp -= dmg;
            dmg = 0;
            if (DEADMONSTER(mdef)) {
                if (magr == &g.youmonst)
                    xkilled(mdef, XKILL_GIVEMSG | XKILL_NOCORPSE);
                else
                    monkilled(mdef, "", AD_RBRE);
            }
        } else if (newcham(mdef, (struct permonst *) 0, FALSE, FALSE)) {
            if (g.vis && canspotmon(mdef))
                pline("%s%s turns into %s.", Before,
                      !flags.verbose ? ""
                       : " undergoes a freakish metamorphosis and",
                      x_monnam(mdef, ARTICLE_A, (char *) 0,
                               (SUPPRESS_NAME | SUPPRESS_IT
                                | SUPPRESS_INVISIBLE), FALSE));
            dmg = 0;
        } else {
            if (g.vis && flags.verbose)
                pline1(nothing_happens);
        }
    }
    return dmg;
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
        && !sticks(g.youmonst.data) && !u.uswallow) {
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

static void
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
static int
passivemm(magr, mdef, mhitb, mdead, mwep)
register struct monst *magr, *mdef;
boolean mhitb;
int mdead;
struct obj *mwep;
{
    register struct permonst *mddat = mdef->data;
    register struct permonst *madat = magr->data;
    char buf[BUFSZ];
    int i, tmp;
    int mhit = mhitb ? MM_HIT : MM_MISS;

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
        if (mhitb && !rn2(2)) {
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
        if (mhitb && !mdef->mcan && mwep) {
            (void) drain_item(mwep, FALSE);
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

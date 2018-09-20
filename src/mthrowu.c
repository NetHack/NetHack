/* NetHack 3.6	mthrowu.c	$NHDT-Date: 1514152830 2017/12/24 22:00:30 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.73 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL int FDECL(monmulti, (struct monst *, struct obj *, struct obj *));
STATIC_DCL void FDECL(monshoot, (struct monst *, struct obj *, struct obj *));
STATIC_DCL int FDECL(drop_throw, (struct obj *, BOOLEAN_P, int, int));
STATIC_DCL boolean FDECL(m_lined_up, (struct monst *, struct monst *));

#define URETREATING(x, y) \
    (distmin(u.ux, u.uy, x, y) > distmin(u.ux0, u.uy0, x, y))

#define POLE_LIM 5 /* How far monsters can use pole-weapons */

#define PET_MISSILE_RANGE2 36 /* Square of distance within which pets shoot */

/*
 * Keep consistent with breath weapons in zap.c, and AD_* in monattk.h.
 */
STATIC_OVL NEARDATA const char *breathwep[] = {
    "fragments", "fire", "frost", "sleep gas", "a disintegration blast",
    "lightning", "poison gas", "acid", "strange breath #8",
    "strange breath #9"
};

extern boolean notonhead; /* for long worms */
STATIC_VAR int mesg_given; /* for m_throw()/thitu() 'miss' message */

/* hero is hit by something other than a monster */
int
thitu(tlev, dam, objp, name)
int tlev, dam;
struct obj **objp;
const char *name; /* if null, then format `*objp' */
{
    struct obj *obj = objp ? *objp : 0;
    const char *onm, *knm;
    boolean is_acid;
    int kprefix = KILLED_BY_AN, dieroll;
    char onmbuf[BUFSZ], knmbuf[BUFSZ];

    if (!name) {
        if (!obj)
            panic("thitu: name & obj both null?");
        name = strcpy(onmbuf,
                      (obj->quan > 1L) ? doname(obj) : mshot_xname(obj));
        knm = strcpy(knmbuf, killer_xname(obj));
        kprefix = KILLED_BY; /* killer_name supplies "an" if warranted */
    } else {
        knm = name;
        /* [perhaps ought to check for plural here to] */
        if (!strncmpi(name, "the ", 4) || !strncmpi(name, "an ", 3)
            || !strncmpi(name, "a ", 2))
            kprefix = KILLED_BY;
    }
    onm = (obj && obj_is_pname(obj)) ? the(name)
          : (obj && obj->quan > 1L) ? name
            : an(name);
    is_acid = (obj && obj->otyp == ACID_VENOM);

    if (u.uac + tlev <= (dieroll = rnd(20))) {
        ++mesg_given;
        if (Blind || !flags.verbose) {
            pline("It misses.");
        } else if (u.uac + tlev <= dieroll - 2) {
            if (onm != onmbuf)
                Strcpy(onmbuf, onm); /* [modifiable buffer for upstart()] */
            pline("%s %s you.", upstart(onmbuf), vtense(onmbuf, "miss"));
        } else
            You("are almost hit by %s.", onm);
        return 0;
    } else {
        if (Blind || !flags.verbose)
            You("are hit%s", exclam(dam));
        else
            You("are hit by %s%s", onm, exclam(dam));

        if (is_acid && Acid_resistance) {
            pline("It doesn't seem to hurt you.");
        } else if (obj && obj->oclass == POTION_CLASS) {
            /* an explosion which scatters objects might hit hero with one
               (potions deliberately thrown at hero are handled by m_throw) */
            potionhit(&youmonst, obj, POTHIT_OTHER_THROW);
            *objp = obj = 0; /* potionhit() uses up the potion */
        } else {
            if (obj && objects[obj->otyp].oc_material == SILVER
                && Hate_silver) {
                /* extra damage already applied by dmgval() */
                pline_The("silver sears your flesh!");
                exercise(A_CON, FALSE);
            }
            if (is_acid)
                pline("It burns!");
            losehp(dam, knm, kprefix); /* acid damage */
            exercise(A_STR, FALSE);
        }
        return 1;
    }
}

/* Be sure this corresponds with what happens to player-thrown objects in
 * dothrow.c (for consistency). --KAA
 * Returns 0 if object still exists (not destroyed).
 */
STATIC_OVL int
drop_throw(obj, ohit, x, y)
register struct obj *obj;
boolean ohit;
int x, y;
{
    int retvalu = 1;
    int create;
    struct monst *mtmp;
    struct trap *t;

    if (obj->otyp == CREAM_PIE || obj->oclass == VENOM_CLASS
        || (ohit && obj->otyp == EGG))
        create = 0;
    else if (ohit && (is_multigen(obj) || obj->otyp == ROCK))
        create = !rn2(3);
    else
        create = 1;

    if (create && !((mtmp = m_at(x, y)) != 0 && mtmp->mtrapped
                    && (t = t_at(x, y)) != 0
                    && is_pit(t->ttyp))) {
        int objgone = 0;

        if (down_gate(x, y) != -1)
            objgone = ship_object(obj, x, y, FALSE);
        if (!objgone) {
            if (!flooreffects(obj, x, y, "fall")) {
                place_object(obj, x, y);
                if (!mtmp && x == u.ux && y == u.uy)
                    mtmp = &youmonst;
                if (mtmp && ohit)
                    passive_obj(mtmp, obj, (struct attack *) 0);
                stackobj(obj);
                retvalu = 0;
            }
        }
    } else
        obfree(obj, (struct obj *) 0);
    return retvalu;
}

/* The monster that's being shot at when one monster shoots at another */
STATIC_OVL struct monst *target = 0;
/* The monster that's doing the shooting/throwing */
STATIC_OVL struct monst *archer = 0;

/* calculate multishot volley count for mtmp throwing otmp (if not ammo) or
   shooting otmp with mwep (if otmp is ammo and mwep appropriate launcher) */
STATIC_OVL int
monmulti(mtmp, otmp, mwep)
struct monst *mtmp;
struct obj *otmp, *mwep;
{
    int skill = (int) objects[otmp->otyp].oc_skill;
    int multishot = 1;

    if (otmp->quan > 1L /* no point checking if there's only 1 */
        /* ammo requires corresponding launcher be wielded */
        && (is_ammo(otmp)
               ? matching_launcher(otmp, mwep)
               /* otherwise any stackable (non-ammo) weapon */
               : otmp->oclass == WEAPON_CLASS)
        && !mtmp->mconf) {
        /* Assumes lords are skilled, princes are expert */
        if (is_prince(mtmp->data))
            multishot += 2;
        else if (is_lord(mtmp->data))
            multishot++;
        /* fake players treated as skilled (regardless of role limits) */
        else if (is_mplayer(mtmp->data))
            multishot++;

        /* this portion is different from hero multishot; from slash'em?
         */
        /* Elven Craftsmanship makes for light, quick bows */
        if (otmp->otyp == ELVEN_ARROW && !otmp->cursed)
            multishot++;
        if (ammo_and_launcher(otmp, uwep) && mwep->otyp == ELVEN_BOW
            && !mwep->cursed)
            multishot++;
        /* 1/3 of launcher enchantment */
        if (ammo_and_launcher(otmp, mwep) && mwep->spe > 1)
            multishot += (long) rounddiv(mwep->spe, 3);
        /* Some randomness */
        multishot = (long) rnd((int) multishot);

        /* class bonus */
        switch (monsndx(mtmp->data)) {
        case PM_CAVEMAN: /* give bonus for low-tech gear */
            if (skill == -P_SLING || skill == P_SPEAR)
                multishot++;
            break;
        case PM_MONK: /* allow higher volley count */
            if (skill == -P_SHURIKEN)
                multishot++;
            break;
        case PM_RANGER:
            if (skill != P_DAGGER)
                multishot++;
            break;
        case PM_ROGUE:
            if (skill == P_DAGGER)
                multishot++;
            break;
        case PM_NINJA:
            if (skill == -P_SHURIKEN || skill == -P_DART)
                multishot++;
            /*FALLTHRU*/
        case PM_SAMURAI:
            if (otmp->otyp == YA && mwep->otyp == YUMI)
                multishot++;
            break;
        default:
            break;
        }
        /* racial bonus */
        if ((is_elf(mtmp->data) && otmp->otyp == ELVEN_ARROW
            && mwep->otyp == ELVEN_BOW)
            || (is_orc(mtmp->data) && otmp->otyp == ORCISH_ARROW
                && mwep->otyp == ORCISH_BOW)
            || (is_gnome(mtmp->data) && otmp->otyp == CROSSBOW_BOLT
                && mwep->otyp == CROSSBOW))
            multishot++;
    }

    if (otmp->quan < multishot)
        multishot = (int) otmp->quan;
    if (multishot < 1)
        multishot = 1;
    return multishot;
}

/* mtmp throws otmp, or shoots otmp with mwep, at hero or at monster mtarg */
STATIC_OVL void
monshoot(mtmp, otmp, mwep)
struct monst *mtmp;
struct obj *otmp, *mwep;
{
    struct monst *mtarg = target;
    int dm = distmin(mtmp->mx, mtmp->my,
                     mtarg ? mtarg->mx : mtmp->mux,
                     mtarg ? mtarg->my : mtmp->muy),
        multishot = monmulti(mtmp, otmp, mwep);
        /*
         * Caller must have called linedup() to set up tbx, tby.
         */

    if (canseemon(mtmp)) {
        const char *onm;
        char onmbuf[BUFSZ], trgbuf[BUFSZ];

        if (multishot > 1) {
            /* "N arrows"; multishot > 1 implies otmp->quan > 1, so
               xname()'s result will already be pluralized */
            Sprintf(onmbuf, "%d %s", multishot, xname(otmp));
            onm = onmbuf;
        } else {
            /* "an arrow" */
            onm = singular(otmp, xname);
            onm = obj_is_pname(otmp) ? the(onm) : an(onm);
        }
        m_shot.s = ammo_and_launcher(otmp, mwep) ? TRUE : FALSE;
        Strcpy(trgbuf, mtarg ? mon_nam(mtarg) : "");
        if (!strcmp(trgbuf, "it"))
            Strcpy(trgbuf, humanoid(mtmp->data) ? "someone" : something);
        pline("%s %s %s%s%s!", Monnam(mtmp),
              m_shot.s ? "shoots" : "throws", onm,
              mtarg ? " at " : "", trgbuf);
        m_shot.o = otmp->otyp;
    } else {
        m_shot.o = STRANGE_OBJECT; /* don't give multishot feedback */
    }
    m_shot.n = multishot;
    for (m_shot.i = 1; m_shot.i <= m_shot.n; m_shot.i++) {
        m_throw(mtmp, mtmp->mx, mtmp->my, sgn(tbx), sgn(tby), dm, otmp);
        /* conceptually all N missiles are in flight at once, but
           if mtmp gets killed (shot kills adjacent gas spore and
           triggers explosion, perhaps), inventory will be dropped
           and otmp might go away via merging into another stack */
        if (DEADMONSTER(mtmp) && m_shot.i < m_shot.n)
            /* cancel pending shots (perhaps ought to give a message here
               since we gave one above about throwing/shooting N missiles) */
            break; /* endmultishot(FALSE); */
    }
    /* reset 'm_shot' */
    m_shot.n = m_shot.i = 0;
    m_shot.o = STRANGE_OBJECT;
    m_shot.s = FALSE;
}

/* an object launched by someone/thing other than player attacks a monster;
   return 1 if the object has stopped moving (hit or its range used up) */
int
ohitmon(mtmp, otmp, range, verbose)
struct monst *mtmp; /* accidental target, located at <bhitpos.x,.y> */
struct obj *otmp;   /* missile; might be destroyed by drop_throw */
int range;          /* how much farther will object travel if it misses;
                       use -1 to signify to keep going even after hit,
                       unless it's gone (used for rolling_boulder_traps) */
boolean verbose;    /* give message(s) even when you can't see what happened */
{
    int damage, tmp;
    boolean vis, ismimic;
    int objgone = 1;
    struct obj *mon_launcher = archer ? MON_WEP(archer) : NULL;

    notonhead = (bhitpos.x != mtmp->mx || bhitpos.y != mtmp->my);
    ismimic = mtmp->m_ap_type && mtmp->m_ap_type != M_AP_MONSTER;
    vis = cansee(bhitpos.x, bhitpos.y);

    tmp = 5 + find_mac(mtmp) + omon_adj(mtmp, otmp, FALSE);
    /* High level monsters will be more likely to hit */
    /* This check applies only if this monster is the target
     * the archer was aiming at. */
    if (archer && target == mtmp) {
        if (archer->m_lev > 5)
            tmp += archer->m_lev - 5;
        if (mon_launcher && mon_launcher->oartifact)
            tmp += spec_abon(mon_launcher, mtmp);
    }
    if (tmp < rnd(20)) {
        if (!ismimic) {
            if (vis)
                miss(distant_name(otmp, mshot_xname), mtmp);
            else if (verbose && !target)
                pline("It is missed.");
        }
        if (!range) { /* Last position; object drops */
            (void) drop_throw(otmp, 0, mtmp->mx, mtmp->my);
            return 1;
        }
    } else if (otmp->oclass == POTION_CLASS) {
        if (ismimic)
            seemimic(mtmp);
        mtmp->msleeping = 0;
        if (vis)
            otmp->dknown = 1;
        /* probably thrown by a monster rather than 'other', but the
           distinction only matters when hitting the hero */
        potionhit(mtmp, otmp, POTHIT_OTHER_THROW);
        return 1;
    } else {
        damage = dmgval(otmp, mtmp);
        if (otmp->otyp == ACID_VENOM && resists_acid(mtmp))
            damage = 0;
        if (ismimic)
            seemimic(mtmp);
        mtmp->msleeping = 0;
        if (vis) {
            if (otmp->otyp == EGG)
                pline("Splat! %s is hit with %s egg!", Monnam(mtmp),
                      otmp->known ? an(mons[otmp->corpsenm].mname) : "an");
            else
                hit(distant_name(otmp, mshot_xname), mtmp, exclam(damage));
        } else if (verbose && !target)
            pline("%s%s is hit%s", (otmp->otyp == EGG) ? "Splat! " : "",
                  Monnam(mtmp), exclam(damage));

        if (otmp->opoisoned && is_poisonable(otmp)) {
            if (resists_poison(mtmp)) {
                if (vis)
                    pline_The("poison doesn't seem to affect %s.",
                              mon_nam(mtmp));
            } else {
                if (rn2(30)) {
                    damage += rnd(6);
                } else {
                    if (vis)
                        pline_The("poison was deadly...");
                    damage = mtmp->mhp;
                }
            }
        }
        if (objects[otmp->otyp].oc_material == SILVER
            && mon_hates_silver(mtmp)) {
            if (vis)
                pline_The("silver sears %s flesh!", s_suffix(mon_nam(mtmp)));
            else if (verbose && !target)
                pline("Its flesh is seared!");
        }
        if (otmp->otyp == ACID_VENOM && cansee(mtmp->mx, mtmp->my)) {
            if (resists_acid(mtmp)) {
                if (vis || (verbose && !target))
                    pline("%s is unaffected.", Monnam(mtmp));
            } else {
                if (vis)
                    pline_The("%s burns %s!", hliquid("acid"), mon_nam(mtmp));
                else if (verbose && !target)
                    pline("It is burned!");
            }
        }
        if (otmp->otyp == EGG && touch_petrifies(&mons[otmp->corpsenm])) {
            if (!munstone(mtmp, TRUE))
                minstapetrify(mtmp, TRUE);
            if (resists_ston(mtmp))
                damage = 0;
        }

        if (!DEADMONSTER(mtmp)) { /* might already be dead (if petrified) */
            mtmp->mhp -= damage;
            if (DEADMONSTER(mtmp)) {
                if (vis || (verbose && !target))
                    pline("%s is %s!", Monnam(mtmp),
                          (nonliving(mtmp->data) || is_vampshifter(mtmp)
                           || !canspotmon(mtmp)) ? "destroyed" : "killed");
                /* don't blame hero for unknown rolling boulder trap */
                if (!context.mon_moving && (otmp->otyp != BOULDER
                                            || range >= 0 || otmp->otrapped))
                    xkilled(mtmp, XKILL_NOMSG);
                else
                    mondied(mtmp);
            }
        }

        /* blinding venom and cream pie do 0 damage, but verify
           that the target is still alive anyway */
        if (!DEADMONSTER(mtmp)
            && can_blnd((struct monst *) 0, mtmp,
                        (uchar) ((otmp->otyp == BLINDING_VENOM) ? AT_SPIT
                                                                : AT_WEAP),
                        otmp)) {
            if (vis && mtmp->mcansee)
                pline("%s is blinded by %s.", Monnam(mtmp), the(xname(otmp)));
            mtmp->mcansee = 0;
            tmp = (int) mtmp->mblinded + rnd(25) + 20;
            if (tmp > 127)
                tmp = 127;
            mtmp->mblinded = tmp;
        }

        objgone = drop_throw(otmp, 1, bhitpos.x, bhitpos.y);
        if (!objgone && range == -1) { /* special case */
            obj_extract_self(otmp);    /* free it for motion again */
            return 0;
        }
        return 1;
    }
    return 0;
}

#define MT_FLIGHTCHECK(pre)                                             \
    (/* missile hits edge of screen */                                  \
     !isok(bhitpos.x + dx, bhitpos.y + dy)                              \
     /* missile hits the wall */                                        \
     || IS_ROCK(levl[bhitpos.x + dx][bhitpos.y + dy].typ)               \
     /* missile hit closed door */                                      \
     || closed_door(bhitpos.x + dx, bhitpos.y + dy)                     \
     /* missile might hit iron bars */                                  \
     /* the random chance for small objects hitting bars is */          \
     /* skipped when reaching them at point blank range */              \
     || (levl[bhitpos.x + dx][bhitpos.y + dy].typ == IRONBARS           \
         && hits_bars(&singleobj,                                       \
                      bhitpos.x, bhitpos.y,                             \
                      bhitpos.x + dx, bhitpos.y + dy,                   \
                      ((pre) ? 0 : !rn2(5)), 0))                        \
     /* Thrown objects "sink" */                                        \
     || (!(pre) && IS_SINK(levl[bhitpos.x][bhitpos.y].typ)))

void
m_throw(mon, x, y, dx, dy, range, obj)
struct monst *mon;       /* launching monster */
int x, y, dx, dy, range; /* launch point, direction, and range */
struct obj *obj;         /* missile (or stack providing it) */
{
    struct monst *mtmp;
    struct obj *singleobj;
    char sym = obj->oclass;
    int hitu = 0, oldumort, blindinc = 0;

    bhitpos.x = x;
    bhitpos.y = y;
    notonhead = FALSE; /* reset potentially stale value */

    if (obj->quan == 1L) {
        /*
         * Remove object from minvent.  This cannot be done later on;
         * what if the player dies before then, leaving the monster
         * with 0 daggers?  (This caused the infamous 2^32-1 orcish
         * dagger bug).
         *
         * VENOM is not in minvent - it should already be OBJ_FREE.
         * The extract below does nothing.
         */

        /* not possibly_unwield, which checks the object's */
        /* location, not its existence */
        if (MON_WEP(mon) == obj)
            setmnotwielded(mon, obj);
        obj_extract_self(obj);
        singleobj = obj;
        obj = (struct obj *) 0;
    } else {
        singleobj = splitobj(obj, 1L);
        obj_extract_self(singleobj);
    }

    singleobj->owornmask = 0; /* threw one of multiple weapons in hand? */

    if ((singleobj->cursed || singleobj->greased) && (dx || dy) && !rn2(7)) {
        if (canseemon(mon) && flags.verbose) {
            if (is_ammo(singleobj))
                pline("%s misfires!", Monnam(mon));
            else
                pline("%s as %s throws it!", Tobjnam(singleobj, "slip"),
                      mon_nam(mon));
        }
        dx = rn2(3) - 1;
        dy = rn2(3) - 1;
        /* check validity of new direction */
        if (!dx && !dy) {
            (void) drop_throw(singleobj, 0, bhitpos.x, bhitpos.y);
            return;
        }
    }

    if (MT_FLIGHTCHECK(TRUE)) {
        (void) drop_throw(singleobj, 0, bhitpos.x, bhitpos.y);
        return;
    }
    mesg_given = 0; /* a 'missile misses' message has not yet been shown */

    /* Note: drop_throw may destroy singleobj.  Since obj must be destroyed
     * early to avoid the dagger bug, anyone who modifies this code should
     * be careful not to use either one after it's been freed.
     */
    if (sym)
        tmp_at(DISP_FLASH, obj_to_glyph(singleobj));
    while (range-- > 0) { /* Actually the loop is always exited by break */
        bhitpos.x += dx;
        bhitpos.y += dy;
        if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
            if (ohitmon(mtmp, singleobj, range, TRUE))
                break;
        } else if (bhitpos.x == u.ux && bhitpos.y == u.uy) {
            if (multi)
                nomul(0);

            if (singleobj->oclass == GEM_CLASS
                && singleobj->otyp <= LAST_GEM + 9 /* 9 glass colors */
                && is_unicorn(youmonst.data)) {
                if (singleobj->otyp > LAST_GEM) {
                    You("catch the %s.", xname(singleobj));
                    You("are not interested in %s junk.",
                        s_suffix(mon_nam(mon)));
                    makeknown(singleobj->otyp);
                    dropy(singleobj);
                } else {
                    You(
                     "accept %s gift in the spirit in which it was intended.",
                        s_suffix(mon_nam(mon)));
                    (void) hold_another_object(singleobj,
                                               "You catch, but drop, %s.",
                                               xname(singleobj),
                                               "You catch:");
                }
                break;
            }
            if (singleobj->oclass == POTION_CLASS) {
                if (!Blind)
                    singleobj->dknown = 1;
                potionhit(&youmonst, singleobj, POTHIT_MONST_THROW);
                break;
            }
            oldumort = u.umortality;
            switch (singleobj->otyp) {
                int dam, hitv;
            case EGG:
                if (!touch_petrifies(&mons[singleobj->corpsenm])) {
                    impossible("monster throwing egg type %d",
                               singleobj->corpsenm);
                    hitu = 0;
                    break;
                }
            /* fall through */
            case CREAM_PIE:
            case BLINDING_VENOM:
                hitu = thitu(8, 0, &singleobj, (char *) 0);
                break;
            default:
                dam = dmgval(singleobj, &youmonst);
                hitv = 3 - distmin(u.ux, u.uy, mon->mx, mon->my);
                if (hitv < -4)
                    hitv = -4;
                if (is_elf(mon->data)
                    && objects[singleobj->otyp].oc_skill == P_BOW) {
                    hitv++;
                    if (MON_WEP(mon) && MON_WEP(mon)->otyp == ELVEN_BOW)
                        hitv++;
                    if (singleobj->otyp == ELVEN_ARROW)
                        dam++;
                }
                if (bigmonst(youmonst.data))
                    hitv++;
                hitv += 8 + singleobj->spe;
                if (dam < 1)
                    dam = 1;
                hitu = thitu(hitv, dam, &singleobj, (char *) 0);
            }
            if (hitu && singleobj->opoisoned && is_poisonable(singleobj)) {
                char onmbuf[BUFSZ], knmbuf[BUFSZ];

                Strcpy(onmbuf, xname(singleobj));
                Strcpy(knmbuf, killer_xname(singleobj));
                poisoned(onmbuf, A_STR, knmbuf,
                         /* if damage triggered life-saving,
                            poison is limited to attrib loss */
                         (u.umortality > oldumort) ? 0 : 10, TRUE);
            }
            if (hitu && can_blnd((struct monst *) 0, &youmonst,
                                 (uchar) ((singleobj->otyp == BLINDING_VENOM)
                                             ? AT_SPIT
                                             : AT_WEAP),
                                 singleobj)) {
                blindinc = rnd(25);
                if (singleobj->otyp == CREAM_PIE) {
                    if (!Blind)
                        pline("Yecch!  You've been creamed.");
                    else
                        pline("There's %s sticky all over your %s.",
                              something, body_part(FACE));
                } else if (singleobj->otyp == BLINDING_VENOM) {
                    const char *eyes = body_part(EYE);

                    if (eyecount(youmonst.data) != 1)
                        eyes = makeplural(eyes);
                    /* venom in the eyes */
                    if (!Blind)
                        pline_The("venom blinds you.");
                    else
                        Your("%s %s.", eyes, vtense(eyes, "sting"));
                }
            }
            if (hitu && singleobj->otyp == EGG) {
                if (!Stoned && !Stone_resistance
                    && !(poly_when_stoned(youmonst.data)
                         && polymon(PM_STONE_GOLEM))) {
                    make_stoned(5L, (char *) 0, KILLED_BY, "");
                }
            }
            stop_occupation();
            if (hitu) {
                (void) drop_throw(singleobj, hitu, u.ux, u.uy);
                break;
            }
        }
        if (!range /* reached end of path */
            || MT_FLIGHTCHECK(FALSE)) {
            if (singleobj) { /* hits_bars might have destroyed it */
                if (m_shot.n > 1
                    && (!mesg_given || bhitpos.x != u.ux || bhitpos.y != u.uy)
                    && (cansee(bhitpos.x, bhitpos.y)
                        || (archer && canseemon(archer))))
                    pline("%s misses.", The(mshot_xname(singleobj)));
                (void) drop_throw(singleobj, 0, bhitpos.x, bhitpos.y);
            }
            break;
        }
        tmp_at(bhitpos.x, bhitpos.y);
        delay_output();
    }
    tmp_at(bhitpos.x, bhitpos.y);
    delay_output();
    tmp_at(DISP_END, 0);
    mesg_given = 0; /* reset */

    if (blindinc) {
        u.ucreamed += blindinc;
        make_blinded(Blinded + (long) blindinc, FALSE);
        if (!Blind)
            Your1(vision_clears);
    }
}

#undef MT_FLIGHTCHECK

/* Monster throws item at another monster */
int
thrwmm(mtmp, mtarg)
struct monst *mtmp, *mtarg;
{
    struct obj *otmp, *mwep;
    register xchar x, y;
    boolean ispole;

    /* Polearms won't be applied by monsters against other monsters */
    if (mtmp->weapon_check == NEED_WEAPON || !MON_WEP(mtmp)) {
        mtmp->weapon_check = NEED_RANGED_WEAPON;
        /* mon_wield_item resets weapon_check as appropriate */
        if (mon_wield_item(mtmp) != 0)
            return 0;
    }

    /* Pick a weapon */
    otmp = select_rwep(mtmp);
    if (!otmp)
        return 0;
    ispole = is_pole(otmp);

    x = mtmp->mx;
    y = mtmp->my;

    mwep = MON_WEP(mtmp); /* wielded weapon */

    if (!ispole && m_lined_up(mtarg, mtmp)) {
        int chance = max(BOLT_LIM - distmin(x, y, mtarg->mx, mtarg->my), 1);

        if (!mtarg->mflee || !rn2(chance)) {
            if (ammo_and_launcher(otmp, mwep)
                && dist2(mtmp->mx, mtmp->my, mtarg->mx, mtarg->my)
                   > PET_MISSILE_RANGE2)
                return 0; /* Out of range */
            /* Set target monster */
            target = mtarg;
            archer = mtmp;
            monshoot(mtmp, otmp, mwep); /* multishot shooting or throwing */
            archer = target = (struct monst *) 0;
            nomul(0);
            return 1;
        }
    }
    return 0;
}

/* monster spits substance at monster */
int
spitmm(mtmp, mattk, mtarg)
struct monst *mtmp, *mtarg;
struct attack *mattk;
{
    struct obj *otmp;

    if (mtmp->mcan) {
        if (!Deaf)
            pline("A dry rattle comes from %s throat.",
                  s_suffix(mon_nam(mtmp)));
        return 0;
    }
    if (m_lined_up(mtarg, mtmp)) {
        switch (mattk->adtyp) {
        case AD_BLND:
        case AD_DRST:
            otmp = mksobj(BLINDING_VENOM, TRUE, FALSE);
            break;
        default:
            impossible("bad attack type in spitmu");
            /* fall through */
        case AD_ACID:
            otmp = mksobj(ACID_VENOM, TRUE, FALSE);
            break;
        }
        if (!rn2(BOLT_LIM-distmin(mtmp->mx,mtmp->my,mtarg->mx,mtarg->my))) {
            if (canseemon(mtmp))
                pline("%s spits venom!", Monnam(mtmp));
            target = mtarg;
            m_throw(mtmp, mtmp->mx, mtmp->my, sgn(tbx), sgn(tby),
                    distmin(mtmp->mx,mtmp->my,mtarg->mx,mtarg->my), otmp);
            target = (struct monst *)0;
            nomul(0);

            /* If this is a pet, it'll get hungry. Minions and
             * spell beings won't hunger */
            if (mtmp->mtame && !mtmp->isminion) {
                struct edog *dog = EDOG(mtmp);

                /* Hunger effects will catch up next move */
                if (dog->hungrytime > 1)
                    dog->hungrytime -= 5;
            }

            return 1;
        }
    }
    return 0;
}

/* monster breathes at monster (ranged) */
int
breamm(mtmp, mattk, mtarg)
struct monst *mtmp, *mtarg;
struct attack  *mattk;
{
    /* if new breath types are added, change AD_ACID to max type */
    int typ = (mattk->adtyp == AD_RBRE) ? rnd(AD_ACID) : mattk->adtyp ;

    if (m_lined_up(mtarg, mtmp)) {
        if (mtmp->mcan) {
            if (!Deaf) {
                if (canseemon(mtmp))
                    pline("%s coughs.", Monnam(mtmp));
                else
                    You_hear("a cough.");
            }
            return 0;
        }
        if (!mtmp->mspec_used && rn2(3)) {
            if ((typ >= AD_MAGM) && (typ <= AD_ACID)) {
                if (canseemon(mtmp))
                    pline("%s breathes %s!", Monnam(mtmp), breathwep[typ - 1]);
                dobuzz((int) (-20 - (typ - 1)), (int)mattk->damn,
                       mtmp->mx, mtmp->my, sgn(tbx), sgn(tby), FALSE);
                nomul(0);
                /* breath runs out sometimes. Also, give monster some
                 * cunning; don't breath if the target fell asleep.
                 */
                mtmp->mspec_used = 6 + rn2(18);

                /* If this is a pet, it'll get hungry. Minions and
                 * spell beings won't hunger */
                if (mtmp->mtame && !mtmp->isminion) {
                    struct edog *dog = EDOG(mtmp);

                    /* Hunger effects will catch up next move */
                    if (dog->hungrytime >= 10)
                        dog->hungrytime -= 10;
                }
            } else impossible("Breath weapon %d used", typ-1);
        } else
            return 0;
    }
    return 1;
}



/* remove an entire item from a monster's inventory; destroy that item */
void
m_useupall(mon, obj)
struct monst *mon;
struct obj *obj;
{
    obj_extract_self(obj);
    if (obj->owornmask) {
        if (obj == MON_WEP(mon))
            mwepgone(mon);
        mon->misc_worn_check &= ~obj->owornmask;
        update_mon_intrinsics(mon, obj, FALSE, FALSE);
        obj->owornmask = 0L;
    }
    obfree(obj, (struct obj *) 0);
}

/* remove one instance of an item from a monster's inventory */
void
m_useup(mon, obj)
struct monst *mon;
struct obj *obj;
{
    if (obj->quan > 1L) {
        obj->quan--;
        obj->owt = weight(obj);
    } else {
        m_useupall(mon, obj);
    }
}

/* monster attempts ranged weapon attack against player */
void
thrwmu(mtmp)
struct monst *mtmp;
{
    struct obj *otmp, *mwep;
    xchar x, y;
    const char *onm;

    /* Rearranged beginning so monsters can use polearms not in a line */
    if (mtmp->weapon_check == NEED_WEAPON || !MON_WEP(mtmp)) {
        mtmp->weapon_check = NEED_RANGED_WEAPON;
        /* mon_wield_item resets weapon_check as appropriate */
        if (mon_wield_item(mtmp) != 0)
            return;
    }

    /* Pick a weapon */
    otmp = select_rwep(mtmp);
    if (!otmp)
        return;

    if (is_pole(otmp)) {
        int dam, hitv;

        if (otmp != MON_WEP(mtmp))
            return; /* polearm must be wielded */
        if (dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) > POLE_LIM
            || !couldsee(mtmp->mx, mtmp->my))
            return; /* Out of range, or intervening wall */

        if (canseemon(mtmp)) {
            onm = xname(otmp);
            pline("%s thrusts %s.", Monnam(mtmp),
                  obj_is_pname(otmp) ? the(onm) : an(onm));
        }

        dam = dmgval(otmp, &youmonst);
        hitv = 3 - distmin(u.ux, u.uy, mtmp->mx, mtmp->my);
        if (hitv < -4)
            hitv = -4;
        if (bigmonst(youmonst.data))
            hitv++;
        hitv += 8 + otmp->spe;
        if (dam < 1)
            dam = 1;

        (void) thitu(hitv, dam, &otmp, (char *) 0);
        stop_occupation();
        return;
    }

    x = mtmp->mx;
    y = mtmp->my;
    /* If you are coming toward the monster, the monster
     * should try to soften you up with missiles.  If you are
     * going away, you are probably hurt or running.  Give
     * chase, but if you are getting too far away, throw.
     */
    if (!lined_up(mtmp)
        || (URETREATING(x, y)
            && rn2(BOLT_LIM - distmin(x, y, mtmp->mux, mtmp->muy))))
        return;

    mwep = MON_WEP(mtmp); /* wielded weapon */
    monshoot(mtmp, otmp, mwep); /* multishot shooting or throwing */
    nomul(0);
}

/* monster spits substance at you */
int
spitmu(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
    struct obj *otmp;

    if (mtmp->mcan) {
        if (!Deaf)
            pline("A dry rattle comes from %s throat.",
                  s_suffix(mon_nam(mtmp)));
        return 0;
    }
    if (lined_up(mtmp)) {
        switch (mattk->adtyp) {
        case AD_BLND:
        case AD_DRST:
            otmp = mksobj(BLINDING_VENOM, TRUE, FALSE);
            break;
        default:
            impossible("bad attack type in spitmu");
        /* fall through */
        case AD_ACID:
            otmp = mksobj(ACID_VENOM, TRUE, FALSE);
            break;
        }
        if (!rn2(BOLT_LIM
                 - distmin(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy))) {
            if (canseemon(mtmp))
                pline("%s spits venom!", Monnam(mtmp));
            m_throw(mtmp, mtmp->mx, mtmp->my, sgn(tbx), sgn(tby),
                    distmin(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy), otmp);
            nomul(0);
            return 0;
        } else {
            obj_extract_self(otmp);
            obfree(otmp, (struct obj *) 0);
        }
    }
    return 0;
}

/* monster breathes at you (ranged) */
int
breamu(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
    /* if new breath types are added, change AD_ACID to max type */
    int typ = (mattk->adtyp == AD_RBRE) ? rnd(AD_ACID) : mattk->adtyp;

    if (lined_up(mtmp)) {
        if (mtmp->mcan) {
            if (!Deaf) {
                if (canseemon(mtmp))
                    pline("%s coughs.", Monnam(mtmp));
                else
                    You_hear("a cough.");
            }
            return 0;
        }
        if (!mtmp->mspec_used && rn2(3)) {
            if ((typ >= AD_MAGM) && (typ <= AD_ACID)) {
                if (canseemon(mtmp))
                    pline("%s breathes %s!", Monnam(mtmp),
                          breathwep[typ - 1]);
                buzz((int) (-20 - (typ - 1)), (int) mattk->damn, mtmp->mx,
                     mtmp->my, sgn(tbx), sgn(tby));
                nomul(0);
                /* breath runs out sometimes. Also, give monster some
                 * cunning; don't breath if the player fell asleep.
                 */
                if (!rn2(3))
                    mtmp->mspec_used = 10 + rn2(20);
                if (typ == AD_SLEE && !Sleep_resistance)
                    mtmp->mspec_used += rnd(20);
            } else
                impossible("Breath weapon %d used", typ - 1);
        }
    }
    return 1;
}

boolean
linedup(ax, ay, bx, by, boulderhandling)
register xchar ax, ay, bx, by;
int boulderhandling; /* 0=block, 1=ignore, 2=conditionally block */
{
    int dx, dy, boulderspots;

    /* These two values are set for use after successful return. */
    tbx = ax - bx;
    tby = ay - by;

    /* sometimes displacement makes a monster think that you're at its
       own location; prevent it from throwing and zapping in that case */
    if (!tbx && !tby)
        return FALSE;

    if ((!tbx || !tby || abs(tbx) == abs(tby)) /* straight line or diagonal */
        && distmin(tbx, tby, 0, 0) < BOLT_LIM) {
        if ((ax == u.ux && ay == u.uy) ? (boolean) couldsee(bx, by)
                                       : clear_path(ax, ay, bx, by))
            return TRUE;
        /* don't have line of sight, but might still be lined up
           if that lack of sight is due solely to boulders */
        if (boulderhandling == 0)
            return FALSE;
        dx = sgn(ax - bx), dy = sgn(ay - by);
        boulderspots = 0;
        do {
            /* <bx,by> is guaranteed to eventually converge with <ax,ay> */
            bx += dx, by += dy;
            if (IS_ROCK(levl[bx][by].typ) || closed_door(bx, by))
                return FALSE;
            if (sobj_at(BOULDER, bx, by))
                ++boulderspots;
        } while (bx != ax || by != ay);
        /* reached target position without encountering obstacle */
        if (boulderhandling == 1 || rn2(2 + boulderspots) < 2)
            return TRUE;
    }
    return FALSE;
}

STATIC_OVL boolean
m_lined_up(mtarg, mtmp)
struct monst *mtarg, *mtmp;
{
    return (linedup(mtarg->mx, mtarg->my, mtmp->mx, mtmp->my, 0));
}


/* is mtmp in position to use ranged attack? */
boolean
lined_up(mtmp)
register struct monst *mtmp;
{
    boolean ignore_boulders;

    /* hero concealment usually trumps monst awareness of being lined up */
    if (Upolyd && rn2(25)
        && (u.uundetected || (youmonst.m_ap_type != M_AP_NOTHING
                              && youmonst.m_ap_type != M_AP_MONSTER)))
        return FALSE;

    ignore_boulders = (throws_rocks(mtmp->data)
                       || m_carrying(mtmp, WAN_STRIKING));
    return linedup(mtmp->mux, mtmp->muy, mtmp->mx, mtmp->my,
                   ignore_boulders ? 1 : 2);
}

/* check if a monster is carrying a particular item */
struct obj *
m_carrying(mtmp, type)
struct monst *mtmp;
int type;
{
    register struct obj *otmp;

    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == type)
            return otmp;
    return (struct obj *) 0;
}

void
hit_bars(objp, objx, objy, barsx, barsy, your_fault, from_invent)
struct obj **objp;      /* *objp will be set to NULL if object breaks */
int objx, objy, barsx, barsy;
boolean your_fault, from_invent;
{
    struct obj *otmp = *objp;
    int obj_type = otmp->otyp;
    boolean unbreakable = (levl[barsx][barsy].wall_info & W_NONDIGGABLE) != 0;

    if (your_fault
        ? hero_breaks(otmp, objx, objy, from_invent)
        : breaks(otmp, objx, objy)) {
        *objp = 0; /* object is now gone */
        /* breakage makes its own noises */
        if (obj_type == POT_ACID) {
            if (cansee(barsx, barsy) && !unbreakable)
                pline_The("iron bars are dissolved!");
            else
                You_hear(Hallucination ? "angry snakes!" : "a hissing noise.");
            if (!unbreakable)
                dissolve_bars(barsx, barsy);
        }
    }
    else if (obj_type == BOULDER || obj_type == HEAVY_IRON_BALL)
        pline("Whang!");
    else if (otmp->oclass == COIN_CLASS
             || objects[obj_type].oc_material == GOLD
             || objects[obj_type].oc_material == SILVER)
        pline("Clink!");
    else
        pline("Clonk!");
}

/* TRUE iff thrown/kicked/rolled object doesn't pass through iron bars */
boolean
hits_bars(obj_p, x, y, barsx, barsy, always_hit, whodidit)
struct obj **obj_p; /* *obj_p will be set to NULL if object breaks */
int x, y, barsx, barsy;
int always_hit; /* caller can force a hit for items which would fit through */
int whodidit;   /* 1==hero, 0=other, -1==just check whether it'll pass thru */
{
    struct obj *otmp = *obj_p;
    int obj_type = otmp->otyp;
    boolean hits = always_hit;

    if (!hits)
        switch (otmp->oclass) {
        case WEAPON_CLASS: {
            int oskill = objects[obj_type].oc_skill;

            hits = (oskill != -P_BOW && oskill != -P_CROSSBOW
                    && oskill != -P_DART && oskill != -P_SHURIKEN
                    && oskill != P_SPEAR
                    && oskill != P_KNIFE); /* but not dagger */
            break;
        }
        case ARMOR_CLASS:
            hits = (objects[obj_type].oc_armcat != ARM_GLOVES);
            break;
        case TOOL_CLASS:
            hits = (obj_type != SKELETON_KEY && obj_type != LOCK_PICK
                    && obj_type != CREDIT_CARD && obj_type != TALLOW_CANDLE
                    && obj_type != WAX_CANDLE && obj_type != LENSES
                    && obj_type != TIN_WHISTLE && obj_type != MAGIC_WHISTLE);
            break;
        case ROCK_CLASS: /* includes boulder */
            if (obj_type != STATUE || mons[otmp->corpsenm].msize > MZ_TINY)
                hits = TRUE;
            break;
        case FOOD_CLASS:
            if (obj_type == CORPSE && mons[otmp->corpsenm].msize > MZ_TINY)
                hits = TRUE;
            else
                hits = (obj_type == MEAT_STICK
                        || obj_type == HUGE_CHUNK_OF_MEAT);
            break;
        case SPBOOK_CLASS:
        case WAND_CLASS:
        case BALL_CLASS:
        case CHAIN_CLASS:
            hits = TRUE;
            break;
        default:
            break;
        }

    if (hits && whodidit != -1) {
        hit_bars(obj_p, x,y, barsx,barsy, whodidit, FALSE);
    }

    return hits;
}

/*mthrowu.c*/

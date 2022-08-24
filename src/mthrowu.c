/* NetHack 3.7	mthrowu.c	$NHDT-Date: 1629497158 2021/08/20 22:05:58 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.117 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static int monmulti(struct monst *, struct obj *, struct obj *);
static void monshoot(struct monst *, struct obj *, struct obj *);
static boolean ucatchgem(struct obj *, struct monst *);
static const char* breathwep_name(int);
static int drop_throw(struct obj *, boolean, coordxy, coordxy);
static int m_lined_up(struct monst *, struct monst *);

#define URETREATING(x, y) \
    (distmin(u.ux, u.uy, x, y) > distmin(u.ux0, u.uy0, x, y))

/*
 * Keep consistent with breath weapons in zap.c, and AD_* in monattk.h.
 */
static NEARDATA const char *breathwep[] = {
    "fragments", "fire", "frost", "sleep gas", "a disintegration blast",
    "lightning", "poison gas", "acid", "strange breath #8",
    "strange breath #9"
};

/* hallucinatory ray types */
const char *const hallublasts[] = {
    "asteroids", "beads", "bubbles", "butterflies", "champagne", "chaos",
    "coins", "cotton candy", "crumbs", "dark matter", "darkness", "dust specks",
    "emoticons", "emotions", "entropy", "flowers", "foam", "fog", "gamma rays",
    "gelatin", "gemstones", "ghosts", "glass shards", "glitter", "good vibes",
    "gravel", "gravity", "gravy", "grawlixes", "holy light", "hornets",
    "hot air", "hyphens", "hypnosis", "infrared", "insects", "laser beams",
    "leaves", "lightening", "logic gates", "magma", "marbles", "mathematics",
    "megabytes", "metal shavings", "metapatterns", "meteors", "mist", "mud",
    "music", "nanites", "needles", "noise", "nostalgia", "oil", "paint",
    "photons", "pixels", "plasma", "polarity", "powder", "powerups",
    "prismatic light", "pure logic", "purple", "radio waves", "rainbows",
    "rock music", "rocket fuel", "rope", "sadness", "salt", "sand", "scrolls",
    "sludge", "smileys", "snowflakes", "sparkles", "specularity", "spores",
    "stars", "steam", "tetrahedrons", "text", "the past", "tornadoes",
    "toxic waste", "ultraviolet light", "viruses", "water", "waveforms", "wind",
    "X-rays", "zorkmids"
};

/* Return a random hallucinatory blast. */
const char *
rnd_hallublast(void)
{
    return hallublasts[rn2(SIZE(hallublasts))];
}

boolean
m_has_launcher_and_ammo(struct monst* mtmp)
{
    struct obj *mwep = MON_WEP(mtmp);

    if (mwep && is_launcher(mwep)) {
        struct obj *otmp;

        for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
            if (ammo_and_launcher(otmp, mwep))
                return TRUE;
    }
    return FALSE;
}

/* hero is hit by something other than a monster (though it could be a
   missile thrown or shot by a monster) */
int
thitu(
    int tlev, /* pseudo-level used when deciding whether to hit hero's AC */
    int dam,
    struct obj **objp,
    const char *name) /* if null, then format `*objp' */
{
    struct obj *obj = objp ? *objp : 0;
    const char *onm, *knm;
    boolean is_acid, named = (name != 0);
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
        ++g.mesg_given;
        if (Blind || !Verbose(2, thitu1)) {
            pline("It misses.");
        } else if (u.uac + tlev <= dieroll - 2) {
            if (onm != onmbuf)
                Strcpy(onmbuf, onm); /* [modifiable buffer for upstart()] */
            pline("%s %s you.", upstart(onmbuf), vtense(onmbuf, "miss"));
        } else
            You("are almost hit by %s.", onm);
        return 0;
    } else {
        if (Blind || !Verbose(2, thitu2))
            You("are hit%s", exclam(dam));
        else
            You("are hit by %s%s", onm, exclam(dam));

        if (is_acid && Acid_resistance) {
            pline("It doesn't seem to hurt you.");
            monstseesu(M_SEEN_ACID);
        } else if (stone_missile(obj) && passes_rocks(g.youmonst.data)) {
            /* use 'named' as an approximation for "hitting from above";
               we avoid "passes through you" for horizontal flight path
               because missile stops and that wording would suggest that
               it should keep going */
            pline("It %s you.",
                  named ? "passes harmlessly through" : "doesn't harm");
        } else if (obj && obj->oclass == POTION_CLASS) {
            /* an explosion which scatters objects might hit hero with one
               (potions deliberately thrown at hero are handled by m_throw) */
            potionhit(&g.youmonst, obj, POTHIT_OTHER_THROW);
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
static int
drop_throw(
    register struct obj *obj,
    boolean ohit,
    coordxy x,
    coordxy y)
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
                if (!mtmp && u_at(x, y))
                    mtmp = &g.youmonst;
                if (mtmp && ohit)
                    passive_obj(mtmp, obj, (struct attack *) 0);
                stackobj(obj);
                retvalu = 0;
            }
        }
    } else {
        delobj(obj);
    }
    g.thrownobj = 0;
    return retvalu;
}

/* calculate multishot volley count for mtmp throwing otmp (if not ammo) or
   shooting otmp with mwep (if otmp is ammo and mwep appropriate launcher) */
static int
monmulti(struct monst* mtmp, struct obj* otmp, struct obj* mwep)
{
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
        /* for arrow, we checked bow&arrow when entering block, but for
           bow, so far we've only validated that otmp is a weapon stack;
           need to verify that it's a stack of arrows rather than darts */
        if (mwep && mwep->otyp == ELVEN_BOW && ammo_and_launcher(otmp, mwep)
            && !mwep->cursed)
            multishot++;
        /* 1/3 of launcher enchantment */
        if (ammo_and_launcher(otmp, mwep) && mwep->spe > 1)
            multishot += (long) rounddiv(mwep->spe, 3);
        /* Some randomness */
        multishot = rnd((int) multishot);

        /* class bonus */
        multishot += multishot_class_bonus(monsndx(mtmp->data), otmp, mwep);

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
static void
monshoot(struct monst* mtmp, struct obj* otmp, struct obj* mwep)
{
    struct monst *mtarg = g.mtarget;
    int dm = distmin(mtmp->mx, mtmp->my,
                     mtarg ? mtarg->mx : mtmp->mux,
                     mtarg ? mtarg->my : mtmp->muy),
        multishot = monmulti(mtmp, otmp, mwep);

    /*
     * Caller must have called linedup() to set up <g.tbx, g.tby>.
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
        g.m_shot.s = ammo_and_launcher(otmp, mwep) ? TRUE : FALSE;
        Strcpy(trgbuf, mtarg ? some_mon_nam(mtarg) : "");
        pline("%s %s %s%s%s!", Monnam(mtmp),
              g.m_shot.s ? "shoots" : "throws", onm,
              mtarg ? " at " : "", trgbuf);
        g.m_shot.o = otmp->otyp;
    } else {
        g.m_shot.o = STRANGE_OBJECT; /* don't give multishot feedback */
    }
    g.m_shot.n = multishot;
    for (g.m_shot.i = 1; g.m_shot.i <= g.m_shot.n; g.m_shot.i++) {
        m_throw(mtmp, mtmp->mx, mtmp->my, sgn(g.tbx), sgn(g.tby), dm, otmp);
        /* conceptually all N missiles are in flight at once, but
           if mtmp gets killed (shot kills adjacent gas spore and
           triggers explosion, perhaps), inventory will be dropped
           and otmp might go away via merging into another stack */
        if (DEADMONSTER(mtmp) && g.m_shot.i < g.m_shot.n)
            /* cancel pending shots (perhaps ought to give a message here
               since we gave one above about throwing/shooting N missiles) */
            break; /* endmultishot(FALSE); */
    }
    /* reset 'g.m_shot' */
    g.m_shot.n = g.m_shot.i = 0;
    g.m_shot.o = STRANGE_OBJECT;
    g.m_shot.s = FALSE;
}

/* an object launched by someone/thing other than player attacks a monster;
   return 1 if the object has stopped moving (hit or its range used up) */
int
ohitmon(
    struct monst *mtmp, /* accidental target, located at <g.bhitpos.x,.y> */
    struct obj *otmp,   /* missile; might be destroyed by drop_throw */
    int range,          /* how much farther will object travel if it misses;
                         * use -1 to signify to keep going even after hit,
                         * unless it's gone (used for rolling_boulder_traps) */
    boolean verbose)/* give message(s) even when you can't see what happened */
{
    int damage, tmp;
    boolean vis, ismimic;
    int objgone = 1;
    struct obj *mon_launcher = g.marcher ? MON_WEP(g.marcher) : NULL;

    g.notonhead = (g.bhitpos.x != mtmp->mx || g.bhitpos.y != mtmp->my);
    ismimic = M_AP_TYPE(mtmp) && M_AP_TYPE(mtmp) != M_AP_MONSTER;
    vis = cansee(g.bhitpos.x, g.bhitpos.y);
    if (vis)
        otmp->dknown = 1;

    tmp = 5 + find_mac(mtmp) + omon_adj(mtmp, otmp, FALSE);
    /* High level monsters will be more likely to hit */
    /* This check applies only if this monster is the target
     * the archer was aiming at. */
    if (g.marcher && g.mtarget == mtmp) {
        if (g.marcher->m_lev > 5)
            tmp += g.marcher->m_lev - 5;
        if (mon_launcher && mon_launcher->oartifact)
            tmp += spec_abon(mon_launcher, mtmp);
    }
    if (tmp < rnd(20)) {
        if (!ismimic) {
            if (vis)
                miss(distant_name(otmp, mshot_xname), mtmp);
            else if (verbose && !g.mtarget)
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
        /* probably thrown by a monster rather than 'other', but the
           distinction only matters when hitting the hero */
        potionhit(mtmp, otmp, POTHIT_OTHER_THROW);
        return 1;
    } else {
        int material = objects[otmp->otyp].oc_material;
        boolean harmless = (stone_missile(otmp) && passes_rocks(mtmp->data));

        damage = dmgval(otmp, mtmp);
        if (otmp->otyp == ACID_VENOM && resists_acid(mtmp))
            damage = 0;
#if 0 /* can't use this because we don't have the attacker */
        if (is_orc(mtmp->data) && is_elf(?magr?))
            damage++;
#endif
        if (ismimic)
            seemimic(mtmp);
        mtmp->msleeping = 0;
        if (vis) {
            if (otmp->otyp == EGG) {
                pline("Splat!  %s is hit with %s egg!", Monnam(mtmp),
                      otmp->known ? an(mons[otmp->corpsenm].pmnames[NEUTRAL])
                                  : "an");
            } else {
                char how[BUFSZ];

                if (!harmless)
                    Strcpy(how, exclam(damage)); /* "!" or "." */
                else
                    Sprintf(how, " but passes harmlessly through %.9s.",
                            mhim(mtmp));
                hit(distant_name(otmp, mshot_xname), mtmp, how);
            }
        } else if (verbose && !g.mtarget)
            pline("%s%s is hit%s", (otmp->otyp == EGG) ? "Splat!  " : "",
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
        if (material == SILVER && mon_hates_silver(mtmp)) {
            boolean flesh = (!noncorporeal(mtmp->data)
                             && !amorphous(mtmp->data));

            /* note: extra silver damage is handled by dmgval() */
            if (vis) {
                char *m_name = mon_nam(mtmp);

                if (flesh) /* s_suffix returns a modifiable buffer */
                    m_name = strcat(s_suffix(m_name), " flesh");
                pline_The("silver sears %s!", m_name);
            } else if (verbose && !g.mtarget) {
                pline("%s is seared!", flesh ? "Its flesh" : "It");
            }
        }
        if (otmp->otyp == ACID_VENOM && cansee(mtmp->mx, mtmp->my)) {
            if (resists_acid(mtmp)) {
                if (vis || (verbose && !g.mtarget))
                    pline("%s is unaffected.", Monnam(mtmp));
            } else {
                if (vis)
                    pline_The("%s burns %s!", hliquid("acid"), mon_nam(mtmp));
                else if (verbose && !g.mtarget)
                    pline("It is burned!");
            }
        }
        if (otmp->otyp == EGG && touch_petrifies(&mons[otmp->corpsenm])) {
            if (!munstone(mtmp, FALSE))
                minstapetrify(mtmp, FALSE);
            if (resists_ston(mtmp))
                damage = 0;
        }

        /* might already be dead (if petrified) */
        if (!harmless && !DEADMONSTER(mtmp)) {
            mtmp->mhp -= damage;
            if (DEADMONSTER(mtmp)) {
                if (vis || (verbose && !g.mtarget))
                    pline("%s is %s!", Monnam(mtmp),
                          (nonliving(mtmp->data) || is_vampshifter(mtmp)
                           || !canspotmon(mtmp)) ? "destroyed" : "killed");
                /* don't blame hero for unknown rolling boulder trap */
                if (!g.context.mon_moving && (otmp->otyp != BOULDER
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
                /* shorten object name to reduce redundancy in the
                   two message [first via hit() above] sequence:
                   "The {splash of venom,cream pie} hits <mon>."
                   "<Mon> is blinded by the {venom,pie}." */
                pline("%s is blinded by %s.", Monnam(mtmp),
                      the((otmp->oclass == VENOM_CLASS) ? "venom"
                          : (otmp->otyp == CREAM_PIE) ? "pie"
                            : xname(otmp))); /* catchall; not used */
            mtmp->mcansee = 0;
            tmp = (int) mtmp->mblinded + rnd(25) + 20;
            if (tmp > 127)
                tmp = 127;
            mtmp->mblinded = tmp;
        }

        objgone = drop_throw(otmp, 1, g.bhitpos.x, g.bhitpos.y);
        if (!objgone && range == -1) { /* special case */
            obj_extract_self(otmp);    /* free it for motion again */
            return 0;
        }
        return 1;
    }
    return 0;
}

/* hero catches gem thrown by mon iff poly'd into unicorn; might drop it */
static boolean
ucatchgem(
    struct obj *gem,   /* caller has verified gem->oclass */
    struct monst *mon)
{
    /* won't catch rock or gray stone; catch (then drop) worthless glass */
    if (gem->otyp <= LAST_GEM + NUM_GLASS_GEMS
        && is_unicorn(g.youmonst.data)) {
        char *gem_xname = xname(gem),
             *mon_s_name = s_suffix(mon_nam(mon));

        if (gem->otyp > LAST_GEM) {
            You("catch the %s.", gem_xname);
            You("are not interested in %s junk.", mon_s_name);
            makeknown(gem->otyp);
            dropy(gem);
        } else {
            You("accept %s gift in the spirit in which it was intended.",
                mon_s_name);
            (void) hold_another_object(gem, "You catch, but drop, %s.",
                                       gem_xname, "You catch:");
        }
        return TRUE;
    }
    return FALSE;
}

#define MT_FLIGHTCHECK(pre,forcehit) \
    (/* missile hits edge of screen */                                  \
     !isok(g.bhitpos.x + dx, g.bhitpos.y + dy)                          \
     /* missile hits the wall */                                        \
     || IS_ROCK(levl[g.bhitpos.x + dx][g.bhitpos.y + dy].typ)           \
     /* missile hit closed door */                                      \
     || closed_door(g.bhitpos.x + dx, g.bhitpos.y + dy)                 \
     /* missile might hit iron bars */                                  \
     /* the random chance for small objects hitting bars is */          \
     /* skipped when reaching them at point blank range */              \
     || (levl[g.bhitpos.x + dx][g.bhitpos.y + dy].typ == IRONBARS       \
         && hits_bars(&singleobj,                                       \
                      g.bhitpos.x, g.bhitpos.y,                         \
                      g.bhitpos.x + dx, g.bhitpos.y + dy,               \
                      ((pre) ? 0 : forcehit), 0))                       \
     /* Thrown objects "sink" */                                        \
     || (!(pre) && IS_SINK(levl[g.bhitpos.x][g.bhitpos.y].typ))         \
     )

void
m_throw(
    struct monst *mon,      /* launching monster */
    coordxy x, coordxy y,           /* launch point */
    coordxy dx, coordxy dy,         /* direction */
    int range,              /* maximum distance */
    struct obj *obj)        /* missile (or stack providing it) */
{
    struct monst *mtmp;
    struct obj *singleobj;
    boolean forcehit;
    char sym = obj->oclass;
    int hitu = 0, oldumort, blindinc = 0;

    g.bhitpos.x = x;
    g.bhitpos.y = y;
    g.notonhead = FALSE; /* reset potentially stale value */

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

        /* not possibly_unwield(), which checks the object's location,
           not its existence */
        if (MON_WEP(mon) == obj)
            setmnotwielded(mon, obj);
        obj_extract_self(obj);
        singleobj = obj;
        obj = (struct obj *) 0;
    } else {
        singleobj = splitobj(obj, 1L);
        obj_extract_self(singleobj);
    }
    /* global pointer for missile object in OBJ_FREE state */
    g.thrownobj = singleobj;

    singleobj->owornmask = 0; /* threw one of multiple weapons in hand? */
    if (!canseemon(mon))
        clear_dknown(singleobj); /* singleobj->dknown = 0; */

    if ((singleobj->cursed || singleobj->greased) && (dx || dy) && !rn2(7)) {
        if (canseemon(mon) && Verbose(2, m_throw)) {
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
            (void) drop_throw(singleobj, 0, g.bhitpos.x, g.bhitpos.y);
            return;
        }
    }

    if (MT_FLIGHTCHECK(TRUE, 0)) {
        (void) drop_throw(singleobj, 0, g.bhitpos.x, g.bhitpos.y);
        return;
    }
    g.mesg_given = 0; /* a 'missile misses' message has not yet been shown */

    /* Note: drop_throw may destroy singleobj.  Since obj must be destroyed
     * early to avoid the dagger bug, anyone who modifies this code should
     * be careful not to use either one after it's been freed.
     */
    if (sym)
        tmp_at(DISP_FLASH, obj_to_glyph(singleobj, rn2_on_display_rng));
    while (range-- > 0) { /* Actually the loop is always exited by break */
        singleobj->ox = g.bhitpos.x += dx;
        singleobj->oy = g.bhitpos.y += dy;
        if (cansee(g.bhitpos.x, g.bhitpos.y))
            singleobj->dknown = 1;

        mtmp = m_at(g.bhitpos.x, g.bhitpos.y);
        if (mtmp && shade_miss(mon, mtmp, singleobj, TRUE, TRUE)) {
            /* if mtmp is a shade and missile passes harmlessly through it,
               give message and skip it in order to keep going */
            mtmp = (struct monst *) 0;
        } else if (mtmp) {
            if (ohitmon(mtmp, singleobj, range, TRUE))
                break;
        } else if (u_at(g.bhitpos.x, g.bhitpos.y)) {
            if (g.multi)
                nomul(0);

            if (singleobj->oclass == POTION_CLASS) {
                potionhit(&g.youmonst, singleobj, POTHIT_MONST_THROW);
                break;
            } else if (singleobj->oclass == GEM_CLASS) {
                /* hero might be poly'd into a unicorn */
                if (ucatchgem(singleobj, mon))
                    break;
            }
            oldumort = u.umortality;

            switch (singleobj->otyp) {
            case EGG:
                if (!touch_petrifies(&mons[singleobj->corpsenm])) {
                    impossible("monster throwing egg type %d",
                               singleobj->corpsenm);
                    hitu = 0;
                    break;
                }
                /*FALLTHRU*/
            case CREAM_PIE:
            case BLINDING_VENOM:
                hitu = thitu(8, 0, &singleobj, (char *) 0);
                break;
            default:
                {
                    int dam, hitv;

                    dam = dmgval(singleobj, &g.youmonst);
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
                    if (bigmonst(g.youmonst.data))
                        hitv++;
                    hitv += 8 + singleobj->spe;
                    if (dam < 1)
                        dam = 1;
                    hitu = thitu(hitv, dam, &singleobj, (char *) 0);
                }
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
            if (hitu && can_blnd((struct monst *) 0, &g.youmonst,
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

                    if (eyecount(g.youmonst.data) != 1)
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
                    && !(poly_when_stoned(g.youmonst.data)
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

        forcehit = !rn2(5);
        if (!range || MT_FLIGHTCHECK(FALSE, forcehit)) {
            /* end of path or blocked */
            if (singleobj) { /* hits_bars might have destroyed it */
                /* note: pline(The(missile)) rather than pline_The(missile)
                   in order to get "Grimtooth" rather than "The Grimtooth" */
                if (range && cansee(g.bhitpos.x, g.bhitpos.y)
                    && IS_SINK(levl[g.bhitpos.x][g.bhitpos.y].typ))
                    pline("%s %s onto the sink.", The(mshot_xname(singleobj)),
                          otense(singleobj, Hallucination ? "plop" : "drop"));
                else if (g.m_shot.n > 1
                         && (!g.mesg_given
                             || g.bhitpos.x != u.ux || g.bhitpos.y != u.uy)
                         && (cansee(g.bhitpos.x, g.bhitpos.y)
                             || (g.marcher && canseemon(g.marcher))))
                    pline("%s misses.", The(mshot_xname(singleobj)));

                (void) drop_throw(singleobj, 0, g.bhitpos.x, g.bhitpos.y);
            }
            break;
        }
        tmp_at(g.bhitpos.x, g.bhitpos.y);
        delay_output();
    }
    tmp_at(g.bhitpos.x, g.bhitpos.y);
    delay_output();
    tmp_at(DISP_END, 0);
    g.mesg_given = 0; /* reset */

    if (blindinc) {
        u.ucreamed += blindinc;
        make_blinded(Blinded + (long) blindinc, FALSE);
        if (!Blind)
            Your1(vision_clears);
    }
    /* note: all early returns follow drop_throw() which clears thrownobj */
    g.thrownobj = 0;
    return;
}

#undef MT_FLIGHTCHECK

/* Monster throws item at another monster */
int
thrwmm(struct monst* mtmp, struct monst* mtarg)
{
    struct obj *otmp, *mwep;
    register coordxy x, y;
    boolean ispole;

    /* Polearms won't be applied by monsters against other monsters */
    if (mtmp->weapon_check == NEED_WEAPON || !MON_WEP(mtmp)) {
        mtmp->weapon_check = NEED_RANGED_WEAPON;
        /* mon_wield_item resets weapon_check as appropriate */
        if (mon_wield_item(mtmp) != 0)
            return MM_MISS;
    }

    /* Pick a weapon */
    otmp = select_rwep(mtmp);
    if (!otmp)
        return MM_MISS;
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
                return MM_MISS; /* Out of range */
            /* Set target monster */
            g.mtarget = mtarg;
            g.marcher = mtmp;
            monshoot(mtmp, otmp, mwep); /* multishot shooting or throwing */
            g.marcher = g.mtarget = (struct monst *) 0;
            nomul(0);
            return MM_HIT;
        }
    }
    return MM_MISS;
}

/* monster spits substance at monster */
int
spitmm(struct monst* mtmp, struct attack* mattk, struct monst* mtarg)
{
    struct obj *otmp;

    if (mtmp->mcan) {
        if (!Deaf)
            pline("A dry rattle comes from %s throat.",
                  s_suffix(mon_nam(mtmp)));
        return MM_MISS;
    }
    if (m_lined_up(mtarg, mtmp)) {
        boolean utarg = (mtarg == &g.youmonst);
        coordxy tx = utarg ? mtmp->mux : mtarg->mx;
        coordxy ty = utarg ? mtmp->muy : mtarg->my;

        switch (mattk->adtyp) {
        case AD_BLND:
        case AD_DRST:
            otmp = mksobj(BLINDING_VENOM, TRUE, FALSE);
            break;
        default:
            impossible("bad attack type in spitmm");
            /*FALLTHRU*/
        case AD_ACID:
            otmp = mksobj(ACID_VENOM, TRUE, FALSE);
            break;
        }
        if (!rn2(BOLT_LIM-distmin(mtmp->mx,mtmp->my,tx,ty))) {
            if (canseemon(mtmp))
                pline("%s spits venom!", Monnam(mtmp));
            if (!utarg)
                g.mtarget = mtarg;
            m_throw(mtmp, mtmp->mx, mtmp->my, sgn(g.tbx), sgn(g.tby),
                    distmin(mtmp->mx,mtmp->my,tx,ty), otmp);
            g.mtarget = (struct monst *)0;
            nomul(0);

            /* If this is a pet, it'll get hungry. Minions and
             * spell beings won't hunger */
            if (mtmp->mtame && !mtmp->isminion) {
                struct edog *dog = EDOG(mtmp);

                /* Hunger effects will catch up next move */
                if (dog->hungrytime > 1)
                    dog->hungrytime -= 5;
            }

            return MM_HIT;
        } else {
            obj_extract_self(otmp);
            obfree(otmp, (struct obj *) 0);
        }
    }
    return MM_MISS;
}

/* Return the name of a breath weapon. If the player is hallucinating, return
 * a silly name instead.
 * typ is AD_MAGM, AD_FIRE, etc */
static const char *
breathwep_name(int typ)
{
    if (Hallucination)
        return rnd_hallublast();

    return breathwep[BZ_OFS_AD(typ)];
}

/* monster breathes at monster (ranged) */
int
breamm(struct monst* mtmp, struct attack* mattk, struct monst* mtarg)
{
    int typ = get_atkdam_type(mattk->adtyp);

    if (m_lined_up(mtarg, mtmp)) {
        if (mtmp->mcan) {
            if (!Deaf) {
                if (canseemon(mtmp))
                    pline("%s coughs.", Monnam(mtmp));
                else
                    You_hear("a cough.");
            }
            return MM_MISS;
        }

        /* if we've seen the actual resistance, don't bother, or
           if we're close by and they reflect, just jump the player */
        if (m_seenres(mtmp, cvt_adtyp_to_mseenres(typ))
            || (m_seenres(mtmp, M_SEEN_REFL)
                && monnear(mtmp, mtmp->mux, mtmp->muy)))
            return MM_HIT;

        if (!mtmp->mspec_used && rn2(3)) {
            if (BZ_VALID_ADTYP(typ)) {
                boolean utarget = (mtarg == &g.youmonst);
                if (canseemon(mtmp))
                    pline("%s breathes %s!",
                          Monnam(mtmp), breathwep_name(typ));
                dobuzz(BZ_M_BREATH(BZ_OFS_AD(typ)), (int) mattk->damn,
                       mtmp->mx, mtmp->my, sgn(g.tbx), sgn(g.tby), utarget);
                nomul(0);
                /* breath runs out sometimes. Also, give monster some
                 * cunning; don't breath if the target fell asleep.
                 */
                if (!utarget || !rn2(3))
                    mtmp->mspec_used = 8 + rn2(18);
                if (utarget && typ == AD_SLEE && !Sleep_resistance)
                    mtmp->mspec_used += rnd(20);

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
            return MM_MISS;
    }
    return MM_HIT;
}

/* remove an entire item from a monster's inventory; destroy that item */
void
m_useupall(struct monst* mon, struct obj* obj)
{
    extract_from_minvent(mon, obj, TRUE, FALSE);
    obfree(obj, (struct obj *) 0);
}

/* remove one instance of an item from a monster's inventory */
void
m_useup(struct monst* mon, struct obj* obj)
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
thrwmu(struct monst* mtmp)
{
    struct obj *otmp, *mwep;
    coordxy x, y;
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
        int dam, hitv, rang;

        if (otmp != MON_WEP(mtmp))
            return; /* polearm must be wielded */

        /*
         * MON_POLE_DIST encompasses knight's move range (5): two spots
         * away provided it's not on a straight diagonal, same as skilled
         * hero.  Using polearm while adjacent is allowed but the verb
         * is adjusted from "thrusts" to "bashes", where the hero would
         * have to switch from applying a polearm to ordinary melee attack
         * to accomplish that.
         *
         *  .545.
         *  52125
         *  41014
         *  52125
         *  .545.
         */
        rang = dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy);
        if (rang > MON_POLE_DIST || !couldsee(mtmp->mx, mtmp->my))
            return; /* Out of range, or intervening wall */

        if (canseemon(mtmp)) {
            onm = xname(otmp);
            pline("%s %s %s.", Monnam(mtmp),
                  /* "thrusts" or "swings", or "bashes with" if adjacent */
                  mswings_verb(otmp, (rang <= 2) ? TRUE : FALSE),
                  obj_is_pname(otmp) ? the(onm) : an(onm));
        }

        dam = dmgval(otmp, &g.youmonst);
        hitv = 3 - distmin(u.ux, u.uy, mtmp->mx, mtmp->my);
        if (hitv < -4)
            hitv = -4;
        if (bigmonst(g.youmonst.data))
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
spitmu(struct monst* mtmp, struct attack* mattk)
{
    return spitmm(mtmp, mattk, &g.youmonst);
}

/* monster breathes at you (ranged) */
int
breamu(struct monst* mtmp, struct attack* mattk)
{
    return breamm(mtmp, mattk, &g.youmonst);
}

/* Move from (ax,ay) to (bx,by), but only if distance is up to BOLT_LIM
   and only in straight line or diagonal, calling fnc for each step.
   Stops if fnc return TRUE, or if step was blocked by wall or closed door.
   Returns TRUE if fnc returned TRUE. */
boolean
linedup_callback(
    coordxy ax,
    coordxy ay,
    coordxy bx,
    coordxy by,
    boolean (*fnc)(coordxy, coordxy))
{
    int dx, dy;

    /* These two values are set for use after successful return. */
    g.tbx = ax - bx;
    g.tby = ay - by;

    /* sometimes displacement makes a monster think that you're at its
       own location; prevent it from throwing and zapping in that case */
    if (!g.tbx && !g.tby)
        return FALSE;

    /* straight line, orthogonal to the map or diagonal */
    if ((!g.tbx || !g.tby || abs(g.tbx) == abs(g.tby))
        && distmin(g.tbx, g.tby, 0, 0) < BOLT_LIM) {
        dx = sgn(ax - bx), dy = sgn(ay - by);
        do {
            /* <bx,by> is guaranteed to eventually converge with <ax,ay> */
            bx += dx, by += dy;
            if (!isok(bx, by))
                return FALSE;
            if (IS_ROCK(levl[bx][by].typ) || closed_door(bx, by)
                || is_waterwall(bx, by))
                return FALSE;
            if ((*fnc)(bx, by))
                return TRUE;
        } while (bx != ax || by != ay);
    }
    return FALSE;
}

boolean
linedup(
    register coordxy ax,
    register coordxy ay,
    register coordxy bx,
    register coordxy by,
    int boulderhandling) /* 0=block, 1=ignore, 2=conditionally block */
{
    int dx, dy, boulderspots;

    /* These two values are set for use after successful return. */
    g.tbx = ax - bx;
    g.tby = ay - by;

    /* sometimes displacement makes a monster think that you're at its
       own location; prevent it from throwing and zapping in that case */
    if (!g.tbx && !g.tby)
        return FALSE;

    /* straight line, orthogonal to the map or diagonal */
    if ((!g.tbx || !g.tby || abs(g.tbx) == abs(g.tby))
        && distmin(g.tbx, g.tby, 0, 0) < BOLT_LIM) {
        if (u_at(ax, ay) ? (boolean) couldsee(bx, by)
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
            if (IS_ROCK(levl[bx][by].typ) || closed_door(bx, by)
                || is_waterwall(bx, by))
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

static int
m_lined_up(struct monst* mtarg, struct monst* mtmp)
{
    boolean utarget = (mtarg == &g.youmonst);
    coordxy tx = utarget ? mtmp->mux : mtarg->mx;
    coordxy ty = utarget ? mtmp->muy : mtarg->my;
    boolean ignore_boulders = utarget && (throws_rocks(mtmp->data)
                                          || m_carrying(mtmp, WAN_STRIKING));

    /* hero concealment usually trumps monst awareness of being lined up */
    if (utarget && Upolyd && rn2(25)
        && (u.uundetected || (U_AP_TYPE != M_AP_NOTHING
                              && U_AP_TYPE != M_AP_MONSTER)))
        return FALSE;

    /* [no callers care about the 1 vs 2 situation any more] */
    return linedup(tx, ty, mtmp->mx, mtmp->my,
                   utarget ? (ignore_boulders ? 1 : 2) : 0);
}


/* is mtmp in position to use ranged attack on hero? */
boolean
lined_up(register struct monst* mtmp)
{
    return m_lined_up(&g.youmonst, mtmp) ? TRUE : FALSE;
}

/* check if a monster is carrying a particular item */
struct obj *
m_carrying(struct monst* mtmp, int type)
{
    register struct obj *otmp;

    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == type)
            return otmp;
    return (struct obj *) 0;
}

void
hit_bars(
    struct obj **objp,    /* *objp will be set to NULL if object breaks */
    coordxy objx, coordxy objy,   /* hero's spot (when wielded) or missile's spot */
    coordxy barsx, coordxy barsy, /* adjacent spot where bars are located */
    unsigned breakflags)  /* breakage control */
{
    struct obj *otmp = *objp;
    int obj_type = otmp->otyp;
    boolean nodissolve = (levl[barsx][barsy].wall_info & W_NONDIGGABLE) != 0,
            your_fault = (breakflags & BRK_BY_HERO) != 0;

    if (your_fault
        ? hero_breaks(otmp, objx, objy, breakflags)
        : breaks(otmp, objx, objy)) {
        *objp = 0; /* object is now gone */
        /* breakage makes its own noises */
        if (obj_type == POT_ACID) {
            if (cansee(barsx, barsy) && !nodissolve)
                pline_The("iron bars are dissolved!");
            else
                You_hear(Hallucination ? "angry snakes!"
                                       : "a hissing noise.");
            if (!nodissolve)
                dissolve_bars(barsx, barsy);
        }
    } else {
        if (!Deaf)
            pline("%s!", (obj_type == BOULDER || obj_type == HEAVY_IRON_BALL)
                         ? "Whang"
                         : (otmp->oclass == COIN_CLASS
                            || objects[obj_type].oc_material == GOLD
                            || objects[obj_type].oc_material == SILVER)
                           ? "Clink"
                           : "Clonk");
    }
}

/* TRUE iff thrown/kicked/rolled object doesn't pass through iron bars */
boolean
hits_bars(
    struct obj **obj_p,   /* *obj_p will be set to NULL if object breaks */
    coordxy x, coordxy y,
    coordxy barsx, coordxy barsy,
    int always_hit,       /* caller can force a hit for items which would
                           * fit through */
    int whodidit)         /* 1==hero, 0=other, -1==just check whether it
                           * will pass through */
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
        hit_bars(obj_p, x, y, barsx, barsy,
                 (whodidit == 1) ? BRK_BY_HERO : 0);
    }

    return hits;
}

/*mthrowu.c*/

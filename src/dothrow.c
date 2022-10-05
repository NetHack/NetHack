/* NetHack 3.7	dothrow.c	$NHDT-Date: 1664979333 2022/10/05 14:15:33 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.250 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2013. */
/* NetHack may be freely redistributed.  See license for details. */

/* Contains code for 't' (throw) */

#include "hack.h"

static int throw_obj(struct obj *, int);
static boolean ok_to_throw(int *);
static int throw_ok(struct obj *);
static void autoquiver(void);
static struct obj *find_launcher(struct obj *);
static int gem_accept(struct monst *, struct obj *);
static void tmiss(struct obj *, struct monst *, boolean);
static int throw_gold(struct obj *);
static void check_shop_obj(struct obj *, coordxy, coordxy, boolean);
static boolean harmless_missile(struct obj *);
static void breakmsg(struct obj *, boolean);
static boolean toss_up(struct obj *, boolean);
static void sho_obj_return_to_u(struct obj * obj);
static boolean mhurtle_step(genericptr_t, coordxy, coordxy);

/* uwep might already be removed from inventory so test for W_WEP instead;
   for Valk+Mjollnir, caller needs to validate the strength requirement */
#define AutoReturn(o,wmsk) \
    ((((wmsk) & W_WEP) != 0                                             \
      && ((o)->otyp == AKLYS                                            \
          || ((o)->oartifact == ART_MJOLLNIR && Role_if(PM_VALKYRIE)))) \
     || (o)->otyp == BOOMERANG)

/* g.thrownobj (decl.c) tracks an object until it lands */

int
multishot_class_bonus(int pm, struct obj *ammo,
                      struct obj *launcher) /* can be NULL */
{
    int multishot = 0;
    schar skill = objects[ammo->otyp].oc_skill;

    switch (pm) {
    case PM_CAVE_DWELLER:
        /* give bonus for low-tech gear */
        if (skill == -P_SLING || skill == P_SPEAR)
            multishot++;
        break;
    case PM_MONK:
        /* allow higher volley count despite skill limitation */
        if (skill == -P_SHURIKEN)
            multishot++;
        break;
    case PM_RANGER:
        /* arbitrary; encourage use of other missiles beside daggers */
        if (skill != P_DAGGER)
            multishot++;
        break;
    case PM_ROGUE:
        /* possibly should add knives... */
        if (skill == P_DAGGER)
            multishot++;
        break;
    case PM_NINJA:
        if (skill == -P_SHURIKEN || skill == -P_DART)
            multishot++;
        /*FALLTHRU*/
    case PM_SAMURAI:
        /* role-specific launcher and its ammo */
        if (ammo->otyp == YA && launcher && launcher->otyp == YUMI)
            multishot++;
        break;
    default:
        break; /* No bonus */
    }

    return multishot;
}

/* Throw the selected object, asking for direction */
static int
throw_obj(struct obj *obj, int shotlimit)
{
    struct obj *otmp, *oldslot;
    int multishot;
    schar skill;
    long wep_mask;
    boolean twoweap, weakmultishot;

    /* ask "in what direction?" */
    if (!getdir((char *) 0)) {
        /* No direction specified, so cancel the throw;
         * might need to undo an object split.
         * We used to use freeinv(obj),addinv(obj) here, but that can
         * merge obj into another stack--usually quiver--even if it hadn't
         * been split from there (possibly triggering a panic in addinv),
         * and freeinv+addinv potentially has other side-effects.
         */
        if (obj->o_id == g.context.objsplit.parent_oid
            || obj->o_id == g.context.objsplit.child_oid)
            (void) unsplitobj(obj);
        return ECMD_CANCEL; /* no time passes */
    }

    /*
     * Throwing gold is usually for getting rid of it when
     * a leprechaun approaches, or for bribing an oncoming
     * angry monster.  So throw the whole object.
     *
     * If the gold is in quiver, throw one coin at a time,
     * possibly using a sling.
     */
    if (obj->oclass == COIN_CLASS && obj != uquiver)
        return throw_gold(obj); /* check */

    if (!canletgo(obj, "throw")) {
        return ECMD_OK;
    }
    if (is_art(obj, ART_MJOLLNIR) && obj != uwep) {
        pline("%s must be wielded before it can be thrown.", The(xname(obj)));
        return ECMD_OK;
    }
    if ((is_art(obj, ART_MJOLLNIR) && ACURR(A_STR) < STR19(25))
        || (obj->otyp == BOULDER && !throws_rocks(g.youmonst.data))) {
        pline("It's too heavy.");
        return ECMD_TIME;
    }
    if (!u.dx && !u.dy && !u.dz) {
        You("cannot throw an object at yourself.");
        return ECMD_OK;
    }
    u_wipe_engr(2);
    if (!uarmg && obj->otyp == CORPSE && touch_petrifies(&mons[obj->corpsenm])
        && !Stone_resistance) {
        You("throw %s with your bare %s.",
            corpse_xname(obj, (const char *) 0, CXN_PFX_THE),
            /* throwing with one hand, but pluralize since the
               expression "with your bare hands" sounds better */
            makeplural(body_part(HAND)));
        Sprintf(g.killer.name, "throwing %s bare-handed", killer_xname(obj));
        instapetrify(g.killer.name);
    }
    if (welded(obj)) {
        weldmsg(obj);
        return ECMD_TIME;
    }
    if (is_wet_towel(obj))
        dry_a_towel(obj, -1, FALSE);

    /* Multishot calculations
     * (potential volley of up to N missiles; default for N is 1)
     */
    multishot = 1;
    skill = objects[obj->otyp].oc_skill;
    if (obj->quan > 1L /* no point checking if there's only 1 */
        /* ammo requires corresponding launcher be wielded */
        && (is_ammo(obj) ? matching_launcher(obj, uwep)
                         /* otherwise any stackable (non-ammo) weapon */
                         : obj->oclass == WEAPON_CLASS)
        && !(Confusion || Stunned)) {
        /* some roles don't get a volley bonus until becoming expert */
        weakmultishot = (Role_if(PM_WIZARD) || Role_if(PM_CLERIC)
                         || (Role_if(PM_HEALER) && skill != P_KNIFE)
                         || (Role_if(PM_TOURIST) && skill != -P_DART)
                         /* poor dexterity also inhibits multishot */
                         || Fumbling || ACURR(A_DEX) <= 6);

        /* Bonus if the player is proficient in this weapon... */
        switch (P_SKILL(weapon_type(obj))) {
        case P_EXPERT:
            multishot++;
        /*FALLTHRU*/
        case P_SKILLED:
            if (!weakmultishot)
                multishot++;
            break;
        default: /* basic or unskilled: no bonus */
            break;
        }
        /* ...or is using a special weapon for their role... */
        multishot += multishot_class_bonus(Role_switch, obj, uwep);

        /* ...or using their race's special bow; no bonus for spears */
        if (!weakmultishot)
            switch (Race_switch) {
            case PM_ELF:
                if (obj->otyp == ELVEN_ARROW && uwep
                    && uwep->otyp == ELVEN_BOW)
                    multishot++;
                break;
            case PM_ORC:
                if (obj->otyp == ORCISH_ARROW && uwep
                    && uwep->otyp == ORCISH_BOW)
                    multishot++;
                break;
            case PM_GNOME:
                /* arbitrary; there isn't any gnome-specific gear */
                if (skill == -P_CROSSBOW)
                    multishot++;
                break;
            case PM_HUMAN:
            case PM_DWARF:
            default:
                break; /* No bonus */
            }

        /* crossbows are slow to load and probably shouldn't allow multiple
           shots at all, but that would result in players never using them;
           instead, high strength is necessary to load and shoot quickly */
        if (multishot > 1 && skill == -P_CROSSBOW
            && ammo_and_launcher(obj, uwep)
            && (int) ACURRSTR < (Race_if(PM_GNOME) ? 16 : 18))
            multishot = rnd(multishot);

        multishot = rnd(multishot);
        if ((long) multishot > obj->quan)
            multishot = (int) obj->quan;
        if (shotlimit > 0 && multishot > shotlimit)
            multishot = shotlimit;
    }

    g.m_shot.s = ammo_and_launcher(obj, uwep) ? TRUE : FALSE;
    /* give a message if shooting more than one, or if player
       attempted to specify a count */
    if (multishot > 1 || shotlimit > 0) {
        /* "You shoot N arrows." or "You throw N daggers." */
        You("%s %d %s.", g.m_shot.s ? "shoot" : "throw",
            multishot, /* (might be 1 if player gave shotlimit) */
            (multishot == 1) ? singular(obj, xname) : xname(obj));
    }

    wep_mask = obj->owornmask;
    oldslot = 0;
    g.m_shot.o = obj->otyp;
    g.m_shot.n = multishot;
    for (g.m_shot.i = 1; g.m_shot.i <= g.m_shot.n; g.m_shot.i++) {
        twoweap = u.twoweap;
        /* split this object off from its slot if necessary */
        if (obj->quan > 1L) {
            otmp = splitobj(obj, 1L);
        } else {
            otmp = obj;
            if (otmp->owornmask)
                remove_worn_item(otmp, FALSE);
            oldslot = obj->nobj;
        }
        freeinv(otmp);
        throwit(otmp, wep_mask, twoweap, oldslot);
        (void) encumber_msg();
    }
    g.m_shot.n = g.m_shot.i = 0;
    g.m_shot.o = STRANGE_OBJECT;
    g.m_shot.s = FALSE;

    return ECMD_TIME;
}

/* common to dothrow() and dofire() */
static boolean
ok_to_throw(int *shotlimit_p) /* (see dothrow()) */
{
    *shotlimit_p = LIMIT_TO_RANGE_INT(0, LARGEST_INT, g.command_count);
    g.multi = 0; /* reset; it's been used up */

    if (notake(g.youmonst.data)) {
        You("are physically incapable of throwing or shooting anything.");
        return FALSE;
    } else if (nohands(g.youmonst.data)) {
        You_cant("throw or shoot without hands."); /* not body_part(HAND) */
        return FALSE;
        /*[what about !freehand(), aside from cursed missile launcher?]*/
    }
    if (check_capacity((char *) 0))
        return FALSE;
    return TRUE;
}

/* getobj callback for object to be thrown */
static int
throw_ok(struct obj *obj)
{
    if (!obj)
        return GETOBJ_EXCLUDE;

    if (obj->bknown && welded(obj)) /* not a candidate if known to be stuck */
        return GETOBJ_DOWNPLAY;

    if (AutoReturn(obj, obj->owornmask)
        /* to get here, obj is boomerang or is uwep and (alkys or Mjollnir) */
        && (obj->oartifact != ART_MJOLLNIR || ACURR(A_STR) >= STR19(25)))
        return GETOBJ_SUGGEST;

    if (obj->quan == 1 && (obj == uwep || (obj == uswapwep && u.twoweap)))
        return GETOBJ_DOWNPLAY;

    if (obj->oclass == COIN_CLASS)
        return GETOBJ_SUGGEST;

    if (!uslinging() && obj->oclass == WEAPON_CLASS)
        return GETOBJ_SUGGEST;
    /* Possible extension: exclude weapons that make no sense to throw,
       such as whips, bows, slings, rubber hoses. */

    if (uslinging() && obj->oclass == GEM_CLASS)
        return GETOBJ_SUGGEST;

    if (throws_rocks(g.youmonst.data) && obj->otyp == BOULDER)
        return GETOBJ_SUGGEST;

    return GETOBJ_DOWNPLAY;
}

/* the #throw command */
int
dothrow(void)
{
    register struct obj *obj;
    int shotlimit;

    /*
     * Since some characters shoot multiple missiles at one time,
     * allow user to specify a count prefix for 'f' or 't' to limit
     * number of items thrown (to avoid possibly hitting something
     * behind target after killing it, or perhaps to conserve ammo).
     *
     * Prior to 3.3.0, command ``3t'' meant ``t(shoot) t(shoot) t(shoot)''
     * and took 3 turns.  Now it means ``t(shoot at most 3 missiles)''.
     *
     * [3.6.0:  shot count setup has been moved into ok_to_throw().]
     */
    if (!ok_to_throw(&shotlimit))
        return ECMD_OK;

    obj = getobj("throw", throw_ok, GETOBJ_PROMPT | GETOBJ_ALLOWCNT);
    /* it is also possible to throw food */
    /* (or jewels, or iron balls... ) */

    return obj ? throw_obj(obj, shotlimit) : ECMD_CANCEL;
}

/* KMH -- Automatically fill quiver */
/* Suggested by Jeffrey Bay <jbay@convex.hp.com> */
static void
autoquiver(void)
{
    struct obj *otmp, *oammo = 0, *omissile = 0, *omisc = 0, *altammo = 0;

    if (uquiver)
        return;

    /* Scan through the inventory */
    for (otmp = g.invent; otmp; otmp = otmp->nobj) {
        if (otmp->owornmask || otmp->oartifact || !otmp->dknown) {
            ; /* Skip it */
        } else if (otmp->otyp == ROCK
                   /* seen rocks or known flint or known glass */
                   || (otmp->otyp == FLINT
                       && objects[otmp->otyp].oc_name_known)
                   || (otmp->oclass == GEM_CLASS
                       && objects[otmp->otyp].oc_material == GLASS
                       && objects[otmp->otyp].oc_name_known)) {
            if (uslinging())
                oammo = otmp;
            else if (ammo_and_launcher(otmp, uswapwep))
                altammo = otmp;
            else if (!omisc)
                omisc = otmp;
        } else if (otmp->oclass == GEM_CLASS) {
            ; /* skip non-rock gems--they're ammo but
                 player has to select them explicitly */
        } else if (is_ammo(otmp)) {
            if (ammo_and_launcher(otmp, uwep))
                /* Ammo matched with launcher (bow+arrow, crossbow+bolt) */
                oammo = otmp;
            else if (ammo_and_launcher(otmp, uswapwep))
                altammo = otmp;
            else
                /* Mismatched ammo (no better than an ordinary weapon) */
                omisc = otmp;
        } else if (is_missile(otmp)) {
            /* Missile (dart, shuriken, etc.) */
            omissile = otmp;
        } else if (otmp->oclass == WEAPON_CLASS && throwing_weapon(otmp)) {
            /* Ordinary weapon */
            if (objects[otmp->otyp].oc_skill == P_DAGGER && !omissile)
                omissile = otmp;
            else if (otmp->otyp == AKLYS)
                continue;
            else
                omisc = otmp;
        }
    }

    /* Pick the best choice */
    if (oammo)
        setuqwep(oammo);
    else if (omissile)
        setuqwep(omissile);
    else if (altammo)
        setuqwep(altammo);
    else if (omisc)
        setuqwep(omisc);

    return;
}

/* look through hero inventory for launcher matching ammo,
   avoiding known cursed items. Returns NULL if no match. */
static struct obj *
find_launcher(struct obj *ammo)
{
    struct obj *otmp, *oX;

    if (!ammo)
        return (struct obj *) 0;

    for (oX = 0, otmp = g.invent; otmp; otmp = otmp->nobj) {
        if (otmp->cursed && otmp->bknown)
            continue; /* known to be cursed, so skip */
        if (ammo_and_launcher(ammo, otmp)) {
            if (otmp->bknown)
                return otmp; /* known-B or known-U (known-C won't get here) */
            if (!oX)
                oX = otmp; /* unknown-BUC; used if no known-BU item found */
        }
    }
    return oX;
}

/* the #fire command -- throw from the quiver or use wielded polearm */
int
dofire(void)
{
    int shotlimit;
    struct obj *obj;
    /* AutoReturn() verifies Valkyrie if weapon is Mjollnir, but it relies
       on its caller to make sure hero is strong enough to throw that */
    boolean uwep_Throw_and_Return = (uwep && AutoReturn(uwep, uwep->owornmask)
                                     && (uwep->oartifact != ART_MJOLLNIR
                                         || ACURR(A_STR) >= STR19(25)));
    int altres, res = ECMD_OK;

    /*
     * Same as dothrow(), except we use quivered missile instead
     * of asking what to throw/shoot.  [Note: with the advent of
     * fireassist that is no longer accurate...]
     *
     * If hero is wielding a thrown-and-return weapon and quiver
     * is empty or contains ammo, use the wielded weapon (won't
     * have any ammo's launcher wielded due to the weapon).
     * If quiver is empty, use autoquiver to fill it when the
     * corresponding option is on.
     * If option is off or autoquiver doesn't select anything,
     * we ask what to throw.
     * Then we put the chosen item into the quiver slot unless
     * it is already in another slot.  [Matters most if it is a
     * stack but also matters for single item if this throw gets
     * aborted (ESC at the direction prompt).]
     */
    if (!ok_to_throw(&shotlimit))
        return ECMD_OK;

    obj = uquiver;
    /* if wielding a throw-and-return weapon, throw it if quiver is empty
       or has ammo rather than missiles [since the throw/return weapon is
       wielded, the ammo's launcher isn't; the ammo-only policy avoids
       throwing Mjollnir if quiver contains daggers] */
    if (uwep_Throw_and_Return && (!obj || is_ammo(obj))) {
        obj = uwep;

    } else if (!obj) {
        if (!flags.autoquiver) {
            /* if we're wielding a polearm, apply it */
            if (uwep && is_pole(uwep)) {
                return use_pole(uwep, TRUE);
            /* if we're wielding a bullwhip, apply it */
            } else if (uwep && uwep->otyp == BULLWHIP) {
                return use_whip(uwep);
            } else if (iflags.fireassist
                       && uswapwep && is_pole(uswapwep)
                       && !(uswapwep->cursed && uswapwep->bknown)) {
                /* we have a known not-cursed polearm as swap weapon.
                   swap to it and retry */
                cmdq_add_ec(CQ_CANNED, doswapweapon);
                cmdq_add_ec(CQ_CANNED, dofire);
                return ECMD_OK; /* haven't taken any time yet */
            } else {
                You("have no ammunition readied.");
            }
        } else {
            autoquiver();
            obj = uquiver;
            if (obj) {
                /* give feedback if quiver has now been filled */
                uquiver->owornmask &= ~W_QUIVER; /* less verbose */
                prinv("You ready:", obj, 0L);
                uquiver->owornmask |= W_QUIVER;
            } else {
                You("have nothing appropriate for your quiver.");
            }
        }
    }

    /* if autoquiver is disabled or has failed, prompt for missile */
    if (!obj) {
        /* in case we're using ^A to repeat prior 'f' command, don't
           use direction of previous throw as getobj()'s choice here */
        g.in_doagain = 0;

        /* this gives its own feedback about populating the quiver slot */
        res = doquiver_core("fire");
        if (res != ECMD_OK && res != ECMD_TIME)
            return res;

        obj = uquiver;
    }

    if (uquiver && is_ammo(uquiver) && iflags.fireassist) {
        struct obj *olauncher;

        /* Try to find a launcher */
        if (ammo_and_launcher(uquiver, uwep)) {
            obj = uquiver;
        } else if (ammo_and_launcher(uquiver, uswapwep)) {
            /* swap weapons and retry fire */
            cmdq_add_ec(CQ_CANNED, doswapweapon);
            cmdq_add_ec(CQ_CANNED, dofire);
            return res;
        } else if ((olauncher = find_launcher(uquiver)) != 0) {
            /* wield launcher, retry fire */
            if (uwep && !flags.pushweapon)
                cmdq_add_ec(CQ_CANNED, doswapweapon);
            cmdq_add_ec(CQ_CANNED, dowield);
            cmdq_add_key(CQ_CANNED, olauncher->invlet);
            cmdq_add_ec(CQ_CANNED, dofire);
            return res;
        }
    }

    altres = obj ? throw_obj(obj, shotlimit) : ECMD_CANCEL;
    /* fire can take time by filling quiver (if that causes something which
       was wielded to be unwielded) even if the throw itself gets cancelled */
    return (res == ECMD_TIME) ? res : altres;
}

/* if in midst of multishot shooting/throwing, stop early */
void
endmultishot(boolean verbose)
{
    if (g.m_shot.i < g.m_shot.n) {
        if (verbose && !g.context.mon_moving) {
            You("stop %s after the %d%s %s.",
                g.m_shot.s ? "firing" : "throwing",
                g.m_shot.i, ordin(g.m_shot.i),
                g.m_shot.s ? "shot" : "toss");
        }
        g.m_shot.n = g.m_shot.i; /* make current shot be the last */
    }
}

/* Object hits floor at hero's feet.
   Called from drop(), throwit(), hold_another_object(), litter(). */
void
hitfloor(
    struct obj *obj,
    boolean verbosely) /* usually True; False if caller has given drop mesg */
{
    if (IS_SOFT(levl[u.ux][u.uy].typ) || u.uinwater || u.uswallow) {
        dropy(obj);
        return;
    }
    if (IS_ALTAR(levl[u.ux][u.uy].typ)) {
        doaltarobj(obj);
    } else if (verbosely) {
        const char *surf = surface(u.ux, u.uy);
        struct trap *t = t_at(u.ux, u.uy);

        /* describe something that might keep the object where it is
           or precede next message stating that it falls */
        if (t && t->tseen) {
            switch (t->ttyp) {
            case TRAPDOOR:
                surf = "trap door";
                break;
            case HOLE:
                surf = "edge of the hole";
                break;
            case PIT:
            case SPIKED_PIT:
                surf = "edge of the pit";
                break;
            default:
                break;
            }
        }
        pline("%s %s the %s.", Doname2(obj), otense(obj, "hit"), surf);
    }

    if (hero_breaks(obj, u.ux, u.uy, BRK_FROM_INV))
        return;
    if (ship_object(obj, u.ux, u.uy, FALSE))
        return;
    dropz(obj, TRUE);
}

/*
 * Walk a path from src_cc to dest_cc, calling a proc for each location
 * except the starting one.  If the proc returns FALSE, stop walking
 * and return FALSE.  If stopped early, dest_cc will be the location
 * before the failed callback.
 */
boolean
walk_path(coord *src_cc, coord *dest_cc,
          boolean (*check_proc)(genericptr_t, coordxy, coordxy),
          genericptr_t arg)
{
    int err;
    coordxy x, y, dx, dy, x_change, y_change, i, prev_x, prev_y;
    boolean keep_going = TRUE;

    /* Use Bresenham's Line Algorithm to walk from src to dest.
     *
     * This should be replaced with a more versatile algorithm
     * since it handles slanted moves in a suboptimal way.
     * Going from 'x' to 'y' needs to pass through 'z', and will
     * fail if there's an obstable there, but it could choose to
     * pass through 'Z' instead if that way imposes no obstacle.
     *     ..y          .Zy
     *     xz.    vs    x..
     * Perhaps we should check both paths and accept whichever
     * one isn't blocked.  But then multiple zigs and zags could
     * potentially produce a meandering path rather than the best
     * attempt at a straight line.  And (*check_proc)() would
     * need to work more like 'travel', distinguishing between
     * testing a possible move and actually attempting that move.
     */
    dx = dest_cc->x - src_cc->x;
    dy = dest_cc->y - src_cc->y;
    prev_x = x = src_cc->x;
    prev_y = y = src_cc->y;

    if (dx < 0) {
        x_change = -1;
        dx = -dx;
    } else
        x_change = 1;
    if (dy < 0) {
        y_change = -1;
        dy = -dy;
    } else
        y_change = 1;

    i = err = 0;
    if (dx < dy) {
        while (i++ < dy) {
            prev_x = x;
            prev_y = y;
            y += y_change;
            err += dx << 1;
            if (err > dy) {
                x += x_change;
                err -= dy << 1;
            }
            /* check for early exit condition */
            if (!(keep_going = (*check_proc)(arg, x, y)))
                break;
        }
    } else {
        while (i++ < dx) {
            prev_x = x;
            prev_y = y;
            x += x_change;
            err += dy << 1;
            if (err > dx) {
                y += y_change;
                err -= dx << 1;
            }
            /* check for early exit condition */
            if (!(keep_going = (*check_proc)(arg, x, y)))
                break;
        }
    }

    if (keep_going)
        return TRUE; /* successful */

    dest_cc->x = prev_x;
    dest_cc->y = prev_y;
    return FALSE;
}

/* hack for hurtle_step() -- it ought to be changed to take an argument
   indicating lev/fly-to-dest vs lev/fly-to-dest-minus-one-land-on-dest
   vs drag-to-dest; original callers use first mode, jumping wants second,
   grappling hook backfire and thrown chained ball need third */
boolean
hurtle_jump(genericptr_t arg, coordxy x, coordxy y)
{
    boolean res;
    long save_EWwalking = EWwalking;

    /* prevent jumping over water from being placed in that water */
    EWwalking |= I_SPECIAL;
    res = hurtle_step(arg, x, y);
    EWwalking = save_EWwalking;
    return res;
}

/*
 * Single step for the hero flying through the air from jumping, flying,
 * etc.  Called from hurtle() and jump() via walk_path().  We expect the
 * argument to be a pointer to an integer -- the range -- which is
 * used in the calculation of points off if we hit something.
 *
 * Bumping into monsters won't cause damage but will wake them and make
 * them angry.  Auto-pickup isn't done, since you don't have control over
 * your movements at the time.
 *
 * Possible additions/changes:
 *      o really attack monster if we hit one
 *      o set stunned if we hit a wall or door
 *      o reset nomul when we stop
 *      o creepy feeling if pass through monster (if ever implemented...)
 *      o bounce off walls
 *      o let jumps go over boulders
 */
boolean
hurtle_step(genericptr_t arg, coordxy x, coordxy y)
{
    coordxy ox, oy;
    int *range = (int *) arg;
    struct obj *obj;
    struct monst *mon;
    boolean may_pass = TRUE, via_jumping, stopping_short;
    struct trap *ttmp;
    int dmg = 0;

    if (!isok(x, y)) {
        You_feel("the spirits holding you back.");
        return FALSE;
    } else if (!in_out_region(x, y)) {
        return FALSE;
    } else if (*range == 0) {
        return FALSE; /* previous step wants to stop now */
    }
    via_jumping = (EWwalking & I_SPECIAL) != 0L;
    stopping_short = (via_jumping && *range < 2);

    if (!Passes_walls || !(may_pass = may_passwall(x, y))) {
        boolean odoor_diag = (IS_DOOR(levl[x][y].typ)
                              && (levl[x][y].doormask & D_ISOPEN)
                              && (u.ux - x) && (u.uy - y));

        if (IS_ROCK(levl[x][y].typ) || closed_door(x, y) || odoor_diag) {
            const char *s;

            if (odoor_diag)
                You("hit the door edge!");
            pline("Ouch!");
            if (IS_TREE(levl[x][y].typ))
                s = "bumping into a tree";
            else if (IS_ROCK(levl[x][y].typ))
                s = "bumping into a wall";
            else
                s = "bumping into a door";
            dmg = rnd(2 + *range);
            losehp(Maybe_Half_Phys(dmg), s, KILLED_BY);
            wake_nearto(x,y, 10);
            return FALSE;
        }
        if (levl[x][y].typ == IRONBARS) {
            You("crash into some iron bars.  Ouch!");
            dmg = rnd(2 + *range);
            losehp(Maybe_Half_Phys(dmg), "crashing into iron bars",
                   KILLED_BY);
            wake_nearto(x,y, 20);
            return FALSE;
        }
        if ((obj = sobj_at(BOULDER, x, y)) != 0) {
            You("bump into a %s.  Ouch!", xname(obj));
            dmg = rnd(2 + *range);
            losehp(Maybe_Half_Phys(dmg), "bumping into a boulder", KILLED_BY);
            wake_nearto(x,y, 10);
            return FALSE;
        }
        if (!may_pass) {
            /* did we hit a no-dig non-wall position? */
            You("smack into something!");
            dmg = rnd(2 + *range);
            losehp(Maybe_Half_Phys(dmg), "touching the edge of the universe",
                   KILLED_BY);
            wake_nearto(x,y, 10);
            return FALSE;
        }
        if ((u.ux - x) && (u.uy - y) && bad_rock(g.youmonst.data, u.ux, y)
            && bad_rock(g.youmonst.data, x, u.uy)) {
            boolean too_much = (g.invent
                                && (inv_weight() + weight_cap() > 600));

            /* Move at a diagonal. */
            if (bigmonst(g.youmonst.data) || too_much) {
                You("%sget forcefully wedged into a crevice.",
                    too_much ? "and all your belongings " : "");
                dmg = rnd(2 + *range);
                losehp(Maybe_Half_Phys(dmg), "wedging into a narrow crevice",
                       KILLED_BY);
                wake_nearto(x,y, 10);
                return FALSE;
            }
        }
    }

    if ((mon = m_at(x, y)) != 0
#if 0   /* we can't include these two exceptions unless we know we're
         * going to end up past the current spot rather than on it;
         * for that, we need to know that the range is not exhausted
         * and also that the next spot doesn't contain an obstacle */
        && !(mon->mundetected && hides_under(mon) && (Flying || Levitation))
        && !(mon->mundetected && mon->data->mlet == S_EEL
             && (Flying || Levitation || Wwalking))
#endif
        ) {
        const char *mnam;
        int glyph = glyph_at(x, y);

        mon->mundetected = 0; /* wakeup() will handle mimic */
        /* after unhiding; combination of a_monnam() and some_mon_nam();
           yields "someone" or "something" instead of "it" for unseen mon */
        mnam = x_monnam(mon, ARTICLE_A, (char *) 0,
                        ((has_mgivenname(mon) ? SUPPRESS_SADDLE : 0)
                         | AUGMENT_IT),
                        FALSE);
        if (!glyph_is_monster(glyph) && !glyph_is_invisible(glyph))
            You("find %s by bumping into %s.", mnam, noit_mhim(mon));
        else
            You("bump into %s.", mnam);
        wakeup(mon, FALSE);
        if (!canspotmon(mon))
            map_invisible(mon->mx, mon->my);
        setmangry(mon, FALSE);
        if (touch_petrifies(mon->data)
            /* this is a bodily collision, so check for body armor */
            && !uarmu && !uarm && !uarmc) {
            Sprintf(g.killer.name, "bumping into %s",
                    an(pmname(mon->data, NEUTRAL)));
            instapetrify(g.killer.name);
        }
        if (touch_petrifies(g.youmonst.data)
            && !which_armor(mon, W_ARMU | W_ARM | W_ARMC)) {
            minstapetrify(mon, TRUE);
        }
        wake_nearto(x, y, 10);
        return FALSE;
    }

    if ((u.ux - x) && (u.uy - y)
        && bad_rock(g.youmonst.data, u.ux, y)
        && bad_rock(g.youmonst.data, x, u.uy)) {
        /* Move at a diagonal. */
        if (Sokoban) {
            You("come to an abrupt halt!");
            return FALSE;
        }
    }

    /* caller has already determined that dragging the ball is allowed;
       if ball is carried we might still need to drag the chain */
    if (Punished) {
        int bc_control;
        coordxy ballx, bally, chainx, chainy;
        boolean cause_delay;

        if (drag_ball(x, y, &bc_control, &ballx, &bally, &chainx,
                      &chainy, &cause_delay, TRUE))
            move_bc(0, bc_control, ballx, bally, chainx, chainy);
    }

    ox = u.ux;
    oy = u.uy;
    u_on_newpos(x, y); /* set u.<ux,uy>, u.usteed-><mx,my>; cliparound(); */
    newsym(ox, oy);    /* update old position */
    vision_recalc(1);  /* update for new position */
    flush_screen(1);
    /* if terrain type changes, levitation or flying might become blocked
       or unblocked; might issue message, so do this after map+vision has
       been updated for new location instead of right after u_on_newpos() */
    if (levl[u.ux][u.uy].typ != levl[ox][oy].typ)
        switch_terrain();

    if (is_pool(x, y) && !u.uinwater) {
        if ((Is_waterlevel(&u.uz) && is_waterwall(x,y))
            || !(Levitation || Flying || Wwalking)) {
            /* couldn't move while hurtling; allow movement now so that
               drown() will give a chance to crawl out of pool and survive */
            g.multi = 0;
            (void) drown();
            return FALSE;
        } else if (!Is_waterlevel(&u.uz) && !stopping_short) {
            Norep("You move over %s.", an(is_moat(x, y) ? "moat" : "pool"));
        }
    } else if (is_lava(x, y) && !stopping_short) {
        Norep("You move over some lava.");
    }

    /* FIXME:
     * Each trap should really trigger on the recoil if it would
     * trigger during normal movement. However, not all the possible
     * side-effects of this are tested [as of 3.4.0] so we trigger
     * those that we have tested, and offer a message for the ones
     * that we have not yet tested.
     */
    if ((ttmp = t_at(x, y)) != 0) {
        if (stopping_short) {
            ; /* see the comment above hurtle_jump() */
        } else if (ttmp->ttyp == MAGIC_PORTAL) {
            dotrap(ttmp, NO_TRAP_FLAGS);
            return FALSE;
        } else if (ttmp->ttyp == VIBRATING_SQUARE) {
            pline("The ground vibrates as you pass it.");
            dotrap(ttmp, NO_TRAP_FLAGS); /* doesn't print messages */
        } else if (ttmp->ttyp == FIRE_TRAP) {
            dotrap(ttmp, NO_TRAP_FLAGS);
        } else if ((is_pit(ttmp->ttyp) || is_hole(ttmp->ttyp))
                   && Sokoban) {
            /* air currents overcome the recoil in Sokoban;
               when jumping, caller performs last step and enters trap */
            if (!via_jumping)
                dotrap(ttmp, NO_TRAP_FLAGS);
            *range = 0;
            return TRUE;
        } else {
            if (ttmp->tseen)
                You("pass right over %s.", an(trapname(ttmp->ttyp, FALSE)));
        }
    }
    if (--*range < 0) /* make sure our range never goes negative */
        *range = 0;
    if (*range != 0)
        delay_output();
    return TRUE;
}

static boolean
mhurtle_step(genericptr_t arg, coordxy x, coordxy y)
{
    struct monst *mon = (struct monst *) arg;
    struct monst *mtmp;

    /* TODO: Treat walls, doors, iron bars, etc. specially
     * rather than just stopping before.
     */
    if (!isok(x, y))
        return FALSE;

    if (goodpos(x, y, mon, MM_IGNOREWATER | MM_IGNORELAVA)
        && m_in_out_region(mon, x, y)) {
        int res;

        if (mon != u.usteed) {
            remove_monster(mon->mx, mon->my);
            newsym(mon->mx, mon->my);
            place_monster(mon, x, y);
            newsym(mon->mx, mon->my);
        } else {
            /* steed is hurtling, move hero which will also move steed */
            u.ux0 = u.ux, u.uy0 = u.uy;
            u_on_newpos(x, y);
            newsym(u.ux0, u.uy0); /* update old position */
            vision_recalc(0); /* new location => different lines of sight */
        }
        flush_screen(1);
        delay_output();
        set_apparxy(mon);
        if (is_waterwall(x, y))
            return FALSE;
        res = mintrap(mon, HURTLING);
        if (res == Trap_Killed_Mon
            || res == Trap_Caught_Mon
            || res == Trap_Moved_Mon)
            return FALSE;
        return TRUE;
    }
    if ((mtmp = m_at(x, y)) != 0) {
        if (canseemon(mon) || canseemon(mtmp))
            pline("%s bumps into %s.", Monnam(mon), a_monnam(mtmp));
        wakeup(mon, !g.context.mon_moving);
        wakeup(mtmp, !g.context.mon_moving);
        if (touch_petrifies(mtmp->data)
            && !which_armor(mon, W_ARMU | W_ARM | W_ARMC)) {
            minstapetrify(mon, !g.context.mon_moving);
            newsym(mon->mx, mon->my);
        }
        if (touch_petrifies(mon->data)
            && !which_armor(mtmp, W_ARMU | W_ARM | W_ARMC)) {
            minstapetrify(mtmp, !g.context.mon_moving);
            newsym(mtmp->mx, mtmp->my);
        }
    }

    return FALSE;
}

/*
 * The player moves through the air for a few squares as a result of
 * throwing or kicking something.
 *
 * dx and dy should be the direction of the hurtle, not of the original
 * kick or throw and be only.
 */
void
hurtle(int dx, int dy, int range, boolean verbose)
{
    coord uc, cc;

    /* The chain is stretched vertically, so you shouldn't be able to move
     * very far diagonally.  The premise that you should be able to move one
     * spot leads to calculations that allow you to only move one spot away
     * from the ball, if you are levitating over the ball, or one spot
     * towards the ball, if you are at the end of the chain.  Rather than
     * bother with all of that, assume that there is no slack in the chain
     * for diagonal movement, give the player a message and return.
     */
    if (Punished && !carried(uball)) {
        You_feel("a tug from the iron ball.");
        nomul(0);
        return;
    } else if (u.utrap) {
        You("are anchored by the %s.",
            u.utraptype == TT_WEB
                ? "web"
                : u.utraptype == TT_LAVA
                      ? hliquid("lava")
                      : u.utraptype == TT_INFLOOR
                            ? surface(u.ux, u.uy)
                            : u.utraptype == TT_BURIEDBALL ? "buried ball"
                                                           : "trap");
        nomul(0);
        return;
    }

    /* make sure dx and dy are [-1,0,1] */
    dx = sgn(dx);
    dy = sgn(dy);

    if (!range || (!dx && !dy) || u.ustuck)
        return; /* paranoia */

    nomul(-range);
    g.multi_reason = "moving through the air";
    g.nomovemsg = ""; /* it just happens */
    if (verbose)
        You("%s in the opposite direction.", range > 1 ? "hurtle" : "float");
    /* if we're in the midst of shooting multiple projectiles, stop */
    endmultishot(TRUE);
    uc.x = u.ux;
    uc.y = u.uy;
    /* this setting of cc is only correct if dx and dy are [-1,0,1] only */
    cc.x = u.ux + (dx * range);
    cc.y = u.uy + (dy * range);
    (void) walk_path(&uc, &cc, hurtle_step, (genericptr_t) &range);
}

/* Move a monster through the air for a few squares. */
void
mhurtle(struct monst *mon, int dx, int dy, int range)
{
    coord mc, cc;

    /* At the very least, debilitate the monster */
    mon->movement = 0;
    mon->mstun = 1;

    /* Is the monster stuck or too heavy to push?
     * (very large monsters have too much inertia, even floaters and flyers)
     */
    if (mon->data->msize >= MZ_HUGE || mon == u.ustuck || mon->mtrapped) {
        if (canseemon(mon))
            pline("%s doesn't budge!", Monnam(mon));
        return;
    }

    /* Make sure dx and dy are [-1,0,1] */
    dx = sgn(dx);
    dy = sgn(dy);
    if (!range || (!dx && !dy))
        return; /* paranoia */
    /* don't let grid bugs be hurtled diagonally */
    if (dx && dy && NODIAG(monsndx(mon->data)))
        return;

    /* undetected monster can be moved by your strike */
    if (mon->mundetected) {
        mon->mundetected = 0;
        newsym(mon->mx, mon->my);
    }
    if (M_AP_TYPE(mon))
        seemimic(mon);

    /* Send the monster along the path */
    mc.x = mon->mx;
    mc.y = mon->my;
    cc.x = mon->mx + (dx * range);
    cc.y = mon->my + (dy * range);
    (void) walk_path(&mc, &cc, mhurtle_step, (genericptr_t) mon);
    if (!DEADMONSTER(mon) && t_at(mon->mx, mon->my))
        (void) mintrap(mon, FORCEBUNGLE);
    else
        (void) minliquid(mon);
    return;
}

static void
check_shop_obj(struct obj *obj, coordxy x, coordxy y, boolean broken)
{
    boolean costly_xy;
    struct monst *shkp = shop_keeper(*u.ushops);

    if (!shkp)
        return;

    costly_xy = costly_spot(x, y);
    if (broken || !costly_xy || *in_rooms(x, y, SHOPBASE) != *u.ushops) {
        /* thrown out of a shop or into a different shop */
        if (is_unpaid(obj))
            (void) stolen_value(obj, u.ux, u.uy, (boolean) shkp->mpeaceful,
                                FALSE);
        if (broken)
            obj->no_charge = 1;
    } else if (costly_xy) {
        char *oshops = in_rooms(x, y, SHOPBASE);

        /* ushops0: in case we threw while levitating and recoiled
           out of shop (most likely to the shk's spot in front of door) */
        if (*oshops == *u.ushops || *oshops == *u.ushops0) {
            if (is_unpaid(obj)) {
                long gt = Has_contents(obj) ? contained_gold(obj, TRUE) : 0L;

                subfrombill(obj, shkp);
                if (gt > 0L)
                    donate_gold(gt, shkp, TRUE);
            } else if (x != shkp->mx || y != shkp->my) {
                sellobj(obj, x, y);
            }
        }
    }
}

/* Will 'obj' cause damage if it falls on hero's head when thrown upward?
   Not used to handle things which break when they hit.
   Stone missile hitting hero w/ Passes_walls attribute handled separately. */
static boolean
harmless_missile(struct obj *obj)
{
    int otyp = obj->otyp;

    /* this list is fairly arbitrary */
    switch (otyp) {
    case SLING:
    case EUCALYPTUS_LEAF:
    case KELP_FROND:
    case SPRIG_OF_WOLFSBANE:
    case FORTUNE_COOKIE:
    case PANCAKE:
        return TRUE;
    case RUBBER_HOSE:
    case BAG_OF_TRICKS:
        return (obj->spe < 1);
    case SACK:
    case OILSKIN_SACK:
    case BAG_OF_HOLDING:
        return !Has_contents(obj);
    default:
        if (obj->oclass == SCROLL_CLASS) /* scrolls but not all paper objs */
            return TRUE;
        if (objects[otyp].oc_material == CLOTH)
            return TRUE;
        break;
    }
    return FALSE;
}

/*
 * Hero tosses an object upwards with appropriate consequences.
 *
 * Returns FALSE if the object is gone.
 */
static boolean
toss_up(struct obj *obj, boolean hitsroof)
{
    const char *action;
    int otyp = obj->otyp;
    boolean petrifier = ((otyp == EGG || otyp == CORPSE)
                         && touch_petrifies(&mons[obj->corpsenm]));
    /* note: obj->quan == 1 */

    if (!has_ceiling(&u.uz)) {
        action = "flies up into"; /* into "the sky" or "the water above" */
    } else if (hitsroof) {
        if (breaktest(obj)) {
            pline("%s hits the %s.", Doname2(obj), ceiling(u.ux, u.uy));
            breakmsg(obj, !Blind);
            breakobj(obj, u.ux, u.uy, TRUE, TRUE);
            return FALSE;
        }
        action = "hits";
    } else {
        action = "almost hits";
    }
    pline("%s %s the %s, then falls back on top of your %s.", Doname2(obj),
          action, ceiling(u.ux, u.uy), body_part(HEAD));

    /* object now hits you */

    if (obj->oclass == POTION_CLASS) {
        potionhit(&g.youmonst, obj, POTHIT_HERO_THROW);
    } else if (breaktest(obj)) {
        int blindinc;

        /* need to check for blindness result prior to destroying obj */
        blindinc = ((otyp == CREAM_PIE || otyp == BLINDING_VENOM)
                    /* AT_WEAP is ok here even if attack type was AT_SPIT */
                    && can_blnd(&g.youmonst, &g.youmonst, AT_WEAP, obj))
                       ? rnd(25)
                       : 0;
        breakmsg(obj, !Blind);
        breakobj(obj, u.ux, u.uy, TRUE, TRUE);
        obj = 0; /* it's now gone */
        switch (otyp) {
        case EGG:
            if (petrifier && !Stone_resistance
                && !(poly_when_stoned(g.youmonst.data)
                     && polymon(PM_STONE_GOLEM))) {
                /* egg ends up "all over your face"; perhaps
                   visored helmet should still save you here */
                if (uarmh)
                    Your("%s fails to protect you.", helm_simple_name(uarmh));
                goto petrify;
            }
            /*FALLTHRU*/
        case CREAM_PIE:
        case BLINDING_VENOM:
            pline("You've got it all over your %s!", body_part(FACE));
            if (blindinc) {
                if (otyp == BLINDING_VENOM && !Blind)
                    pline("It blinds you!");
                u.ucreamed += blindinc;
                make_blinded(Blinded + (long) blindinc, FALSE);
                if (!Blind)
                    Your1(vision_clears);
            }
            break;
        default:
            break;
        }
        return FALSE;
    } else if (harmless_missile(obj)) {
        pline("It doesn't hurt.");
        hitfloor(obj, FALSE);
        g.thrownobj = 0;
    } else { /* neither potion nor other breaking object */
        int material = objects[otyp].oc_material;
        boolean is_silver = (material == SILVER),
                less_damage = (uarmh && is_metallic(uarmh)
                               && (!is_silver || !Hate_silver)),
                harmless = (stone_missile(obj)
                            && passes_rocks(g.youmonst.data)),
                artimsg = FALSE;
        int dmg = dmgval(obj, &g.youmonst);

        if (obj->oartifact && !harmless)
            /* need a fake die roll here; rn1(18,2) avoids 1 and 20 */
            artimsg = artifact_hit((struct monst *) 0, &g.youmonst, obj, &dmg,
                                   rn1(18, 2));

        if (!dmg) { /* probably wasn't a weapon; base damage on weight */
            dmg = ((int) obj->owt + 99) / 100;
            dmg = (dmg <= 1) ? 1 : rnd(dmg);
            if (dmg > 6)
                dmg = 6;
            /* since obj is a non-weapon, bonuses for silver and blessed
               haven't been applied (otherwise '!dmg' test will fail when
               they're applicable here); we don't have to worry about
               dmgval()'s artifact light against gremlin or axe against
               woody creature since both involve weapons; hero-as-shade is
               hypothetical because hero can't polymorph into that form */
            if (g.youmonst.data == &mons[PM_SHADE] && !is_silver)
                dmg = 0;
            if (obj->blessed && mon_hates_blessings(&g.youmonst))
                dmg += rnd(4);
            if (is_silver && Hate_silver)
                dmg += rnd(20);
        }
        if (dmg > 1 && less_damage)
            dmg = 1;
        if (dmg > 0)
            dmg += u.udaminc;
        if (dmg < 0)
            dmg = 0; /* beware negative rings of increase damage */
        dmg = Maybe_Half_Phys(dmg);

        if (uarmh) {
            /* note: 'harmless' and 'petrifier' are mutually exclusive */
            if ((less_damage && dmg < (Upolyd ? u.mh : u.uhp)) || harmless) {
                if (!artimsg) {
                    if (!harmless) /* !harmless => less_damage here */
                        pline("Fortunately, you are wearing a hard helmet.");
                    else
                        pline("Unfortunately, you are wearing %s.",
                              an(helm_simple_name(uarmh))); /* helm or hat */
                }

            /* helmet definitely protects you when it blocks petrification */
            } else if (!petrifier) {
                if (Verbose(0, toss_up))
                    Your("%s does not protect you.", helm_simple_name(uarmh));
            }
            /* stone missile against hero in xorn form would have been
               harmless, but hitting a worn helmet negates that */
            harmless = FALSE;
        } else if (petrifier && !Stone_resistance
                   && !(poly_when_stoned(g.youmonst.data)
                        && polymon(PM_STONE_GOLEM))) {
 petrify:
            g.killer.format = KILLED_BY;
            Strcpy(g.killer.name, "elementary physics"); /* what goes up... */
            You("turn to stone.");
            if (obj)
                dropy(obj); /* bypass most of hitfloor() */
            g.thrownobj = 0;  /* now either gone or on floor */
            done(STONING);
            return obj ? TRUE : FALSE;
        }
        if (is_silver && Hate_silver)
            pline_The("silver sears you!");
        if (harmless)
            hit(thesimpleoname(obj), &g.youmonst, " but doesn't hurt.");

        hitfloor(obj, TRUE);
        g.thrownobj = 0;
        if (!harmless)
            losehp(dmg, "falling object", KILLED_BY_AN);
    }
    return TRUE;
}

/* return true for weapon meant to be thrown; excludes ammo */
boolean
throwing_weapon(struct obj *obj)
{
    return (boolean) (is_missile(obj) || is_spear(obj)
                      /* daggers and knife (excludes scalpel) */
                      || (is_blade(obj) && !is_sword(obj)
                          && (objects[obj->otyp].oc_dir & PIERCE))
                      /* special cases [might want to add AXE] */
                      || obj->otyp == WAR_HAMMER || obj->otyp == AKLYS);
}

/* the currently thrown object is returning to you (not for boomerangs) */
static void
sho_obj_return_to_u(struct obj *obj)
{
    /* might already be our location (bounced off a wall) */
    if ((u.dx || u.dy) && (g.bhitpos.x != u.ux || g.bhitpos.y != u.uy)) {
        int x = g.bhitpos.x - u.dx, y = g.bhitpos.y - u.dy;

        tmp_at(DISP_FLASH, obj_to_glyph(obj, rn2_on_display_rng));
        while (isok(x,y) && (x != u.ux || y != u.uy)) {
            tmp_at(x, y);
            delay_output();
            x -= u.dx;
            y -= u.dy;
        }
        tmp_at(DISP_END, 0);
    }
}

/* throw an object, NB: obj may be consumed in the process */
void
throwit(struct obj *obj,
    long wep_mask,       /* used to re-equip returning boomerang */
    boolean twoweap,     /* used to restore twoweapon mode if
                            wielded weapon returns */
    struct obj *oldslot) /* for thrown-and-return used with !fixinv */
{
    register struct monst *mon;
    int range, urange;
    boolean crossbowing, clear_thrownobj = FALSE,
            impaired = (Confusion || Stunned || Blind
                        || Hallucination || Fumbling),
            tethered_weapon = (obj->otyp == AKLYS && (wep_mask & W_WEP) != 0);

    g.notonhead = FALSE; /* reset potentially stale value */
    if ((obj->cursed || obj->greased) && (u.dx || u.dy) && !rn2(7)) {
        boolean slipok = TRUE;

        if (ammo_and_launcher(obj, uwep)) {
            pline("%s!", Tobjnam(obj, "misfire"));
        } else {
            /* only slip if it's greased or meant to be thrown */
            if (obj->greased || throwing_weapon(obj))
                /* BUG: this message is grammatically incorrect if obj has
                   a plural name; greased gloves or boots for instance. */
                pline("%s as you throw it!", Tobjnam(obj, "slip"));
            else
                slipok = FALSE;
        }
        if (slipok) {
            u.dx = rn2(3) - 1;
            u.dy = rn2(3) - 1;
            if (!u.dx && !u.dy)
                u.dz = 1;
            impaired = TRUE;
        }
    }

    if ((u.dx || u.dy || (u.dz < 1))
        && calc_capacity((int) obj->owt) > SLT_ENCUMBER
        && (Upolyd ? (u.mh < 5 && u.mh != u.mhmax)
                   : (u.uhp < 10 && u.uhp != u.uhpmax))
        && obj->owt > (unsigned) ((Upolyd ? u.mh : u.uhp) * 2)
        && !Is_airlevel(&u.uz)) {
        You("have so little stamina, %s drops from your grasp.",
            the(xname(obj)));
        exercise(A_CON, FALSE);
        u.dx = u.dy = 0;
        u.dz = 1;
    }

    g.thrownobj = obj;
    g.thrownobj->was_thrown = 1;
    iflags.returning_missile = AutoReturn(obj, wep_mask) ? (genericptr_t) obj
                                                         : (genericptr_t) 0;
    /* NOTE:  No early returns after this point or returning_missile
       will be left with a stale pointer. */

    if (u.uswallow) {
        if (obj == uball) {
            uball->ox = uchain->ox = u.ux;
            uball->oy = uchain->oy = u.uy;
        }
        mon = u.ustuck;
        g.bhitpos.x = mon->mx;
        g.bhitpos.y = mon->my;
        if (tethered_weapon)
            tmp_at(DISP_TETHER, obj_to_glyph(obj, rn2_on_display_rng));
    } else if (u.dz) {
        if (u.dz < 0
            /* Mjollnir must we wielded to be thrown--caller verifies this;
               aklys must we wielded as primary to return when thrown */
            && iflags.returning_missile
            && !impaired) {
            pline("%s the %s and returns to your hand!", Tobjnam(obj, "hit"),
                  ceiling(u.ux, u.uy));
            obj = addinv_before(obj, oldslot);
            (void) encumber_msg();
            if (obj->owornmask & W_QUIVER) /* in case addinv() autoquivered */
                setuqwep((struct obj *) 0);
            setuwep(obj);
            set_twoweap(twoweap); /* u.twoweap = twoweap */
        } else if (u.dz < 0) {
            (void) toss_up(obj, rn2(5) && !Underwater);
        } else if (u.dz > 0 && u.usteed && obj->oclass == POTION_CLASS
                   && rn2(6)) {
            /* alternative to prayer or wand of opening/spell of knock
               for dealing with cursed saddle:  throw holy water > */
            potionhit(u.usteed, obj, POTHIT_HERO_THROW);
        } else {
            hitfloor(obj, TRUE);
        }
        clear_thrownobj = TRUE;
        goto throwit_return;

    } else if (obj->otyp == BOOMERANG && !Underwater) {
        if (Is_airlevel(&u.uz) || Levitation)
            hurtle(-u.dx, -u.dy, 1, TRUE);
        iflags.returning_missile = 0; /* doesn't return if it hits monster */
        mon = boomhit(obj, u.dx, u.dy);
        if (mon == &g.youmonst) { /* the thing was caught */
            exercise(A_DEX, TRUE);
            obj = addinv_before(obj, oldslot);
            (void) encumber_msg();
            if (wep_mask && !(obj->owornmask & wep_mask)) {
                setworn(obj, wep_mask);
                /* moot; can no longer two-weapon with missile(s) */
                set_twoweap(twoweap); /* u.twoweap = twoweap */
            }
            clear_thrownobj = TRUE;
            goto throwit_return;
        }
    } else {
        /* crossbow range is independent of strength */
        crossbowing = (ammo_and_launcher(obj, uwep)
                       && weapon_type(uwep) == P_CROSSBOW);
        urange = (crossbowing ? 18 : (int) ACURRSTR) / 2;
        /* balls are easy to throw or at least roll;
         * also, this insures the maximum range of a ball is greater
         * than 1, so the effects from throwing attached balls are
         * actually possible
         */
        if (obj->otyp == HEAVY_IRON_BALL)
            range = urange - (int) (obj->owt / 100);
        else
            range = urange - (int) (obj->owt / 40);
        if (obj == uball) {
            if (u.ustuck)
                range = 1;
            else if (range >= 5)
                range = 5;
        }
        if (range < 1)
            range = 1;

        if (is_ammo(obj)) {
            if (ammo_and_launcher(obj, uwep)) {
                if (crossbowing)
                    range = BOLT_LIM;
                else
                    range++;
            } else if (obj->oclass != GEM_CLASS)
                range /= 2;
        }

        if (Is_airlevel(&u.uz) || Levitation) {
            /* action, reaction... */
            urange -= range;
            if (urange < 1)
                urange = 1;
            range -= urange;
            if (range < 1)
                range = 1;
        }

        if (obj->otyp == BOULDER)
            range = 20; /* you must be giant */
        else if (is_art(obj, ART_MJOLLNIR))
            range = (range + 1) / 2; /* it's heavy */
        else if (tethered_weapon) /* primary weapon is aklys */
            /* if an aklys is going to return, range is limited by the
               length of the attached cord [implicit aspect of item] */
            range = min(range, BOLT_LIM / 2);
        else if (obj == uball && u.utrap && u.utraptype == TT_INFLOOR)
            range = 1;

        if (Underwater)
            range = 1;

        mon = bhit(u.dx, u.dy, range,
                   tethered_weapon ? THROWN_TETHERED_WEAPON : THROWN_WEAPON,
                   (int (*)(MONST_P, OBJ_P)) 0,
                   (int (*)(OBJ_P, OBJ_P)) 0, &obj);
        g.thrownobj = obj; /* obj may be null now */

        /* have to do this after bhit() so u.ux & u.uy are correct */
        if (Is_airlevel(&u.uz) || Levitation)
            hurtle(-u.dx, -u.dy, urange, TRUE);

        if (!obj) {
            /* bhit display cleanup was left with this caller
               for tethered_weapon, but clean it up now since
               we're about to return */
            if (tethered_weapon)
                tmp_at(DISP_END, 0);
            goto throwit_return;
        }
    }

    if (mon) {
        boolean obj_gone;

        if (mon->isshk && obj->where == OBJ_MINVENT && obj->ocarry == mon) {
            clear_thrownobj = TRUE;
            goto throwit_return; /* alert shk caught it */
        }
        (void) snuff_candle(obj);
        g.notonhead = (g.bhitpos.x != mon->mx || g.bhitpos.y != mon->my);
        obj_gone = thitmonst(mon, obj);
        /* Monster may have been tamed; this frees old mon [obsolete] */
        mon = m_at(g.bhitpos.x, g.bhitpos.y);

        /* [perhaps this should be moved into thitmonst or hmon] */
        if (mon && mon->isshk
            && (!inside_shop(u.ux, u.uy)
                || !index(in_rooms(mon->mx, mon->my, SHOPBASE), *u.ushops)))
            hot_pursuit(mon);

        if (obj_gone)
            g.thrownobj = (struct obj *) 0;
    }

    if (!g.thrownobj) {
        /* missile has already been handled */
        if (tethered_weapon)
            tmp_at(DISP_END, 0);
    } else if (u.uswallow && !iflags.returning_missile) {
 swallowit:
        if (obj != uball)
            (void) mpickobj(u.ustuck, obj); /* clears 'g.thrownobj' */
        else
            clear_thrownobj = TRUE;
        goto throwit_return;
    } else {
        /* Mjollnir must be wielded to be thrown--caller verifies this;
           aklys must be wielded as primary to return when thrown */
        if (iflags.returning_missile) { /* Mjollnir or aklys */
            if (rn2(100)) {
                if (tethered_weapon)
                    tmp_at(DISP_END, BACKTRACK);
                else
                    sho_obj_return_to_u(obj); /* display its flight */

                if (!impaired && rn2(100)) {
                    pline("%s to your hand!", Tobjnam(obj, "return"));
                    obj = addinv_before(obj, oldslot);
                    (void) encumber_msg();
                    /* addinv autoquivers an aklys if quiver is empty;
                       if obj is quivered, remove it before wielding */
                    if (obj->owornmask & W_QUIVER)
                        setuqwep((struct obj *) 0);
                    setuwep(obj);
                    set_twoweap(twoweap); /* u.twoweap = twoweap */
                    if (cansee(g.bhitpos.x, g.bhitpos.y))
                        newsym(g.bhitpos.x, g.bhitpos.y);
                } else {
                    int dmg = rn2(2);

                    if (!dmg) {
                        pline(Blind ? "%s lands %s your %s."
                                    : "%s back to you, landing %s your %s.",
                              Blind ? Something : Tobjnam(obj, "return"),
                              Levitation ? "beneath" : "at",
                              makeplural(body_part(FOOT)));
                    } else {
                        dmg += rnd(3);
                        pline(Blind ? "%s your %s!"
                                    : "%s back toward you, hitting your %s!",
                              Tobjnam(obj, Blind ? "hit" : "fly"),
                              body_part(ARM));
                        if (obj->oartifact)
                            (void) artifact_hit((struct monst *) 0,
                                                &g.youmonst, obj, &dmg, 0);
                        losehp(Maybe_Half_Phys(dmg), killer_xname(obj),
                               KILLED_BY);
                    }

                    if (u.uswallow)
                        goto swallowit;
                    if (!ship_object(obj, u.ux, u.uy, FALSE))
                        dropy(obj);
                }
                clear_thrownobj = TRUE;
                goto throwit_return;
            } else {
                if (tethered_weapon)
                    tmp_at(DISP_END, 0);
                /* when this location is stepped on, the weapon will be
                   auto-picked up due to 'obj->was_thrown' of 1;
                   addinv() prevents thrown Mjollnir from being placed
                   into the quiver slot, but an aklys will end up there if
                   that slot is empty at the time; since hero will need to
                   explicitly rewield the weapon to get throw-and-return
                   capability back anyway, quivered or not shouldn't matter */
                pline("%s to return!", Tobjnam(obj, "fail"));

                if (u.uswallow)
                    goto swallowit;
                /* continue below with placing 'obj' at target location */
            }
        }

        if ((!IS_SOFT(levl[g.bhitpos.x][g.bhitpos.y].typ) && breaktest(obj))
            /* venom [via #monster to spit while poly'd] fails breaktest()
               but we want to force breakage even when location IS_SOFT() */
            || obj->oclass == VENOM_CLASS) {
            tmp_at(DISP_FLASH, obj_to_glyph(obj, rn2_on_display_rng));
            tmp_at(g.bhitpos.x, g.bhitpos.y);
            delay_output();
            tmp_at(DISP_END, 0);
            breakmsg(obj, cansee(g.bhitpos.x, g.bhitpos.y));
            breakobj(obj, g.bhitpos.x, g.bhitpos.y, TRUE, TRUE);
            clear_thrownobj = TRUE;
            goto throwit_return;
        }
        if (!Deaf && !Underwater) {
            /* Some sound effects when item lands in water or lava */
            if (is_pool(g.bhitpos.x, g.bhitpos.y)
                || (is_lava(g.bhitpos.x, g.bhitpos.y) && !is_flammable(obj)))
                pline((weight(obj) > 9) ? "Splash!" : "Plop!");
        }
        if (flooreffects(obj, g.bhitpos.x, g.bhitpos.y, "fall")) {
            clear_thrownobj = TRUE;
            goto throwit_return;
        }
        obj_no_longer_held(obj);
        if (mon && mon->isshk && is_pick(obj)) {
            if (cansee(g.bhitpos.x, g.bhitpos.y))
                pline("%s snatches up %s.", Monnam(mon), the(xname(obj)));
            if (*u.ushops || obj->unpaid)
                check_shop_obj(obj, g.bhitpos.x, g.bhitpos.y, FALSE);
            (void) mpickobj(mon, obj); /* may merge and free obj */
            clear_thrownobj = TRUE;
            goto throwit_return;
        }
        (void) snuff_candle(obj);
        if (!mon && ship_object(obj, g.bhitpos.x, g.bhitpos.y, FALSE)) {
            clear_thrownobj = TRUE;
            goto throwit_return;
        }
        g.thrownobj = (struct obj *) 0;
        place_object(obj, g.bhitpos.x, g.bhitpos.y);
        /* container contents might break;
           do so before turning ownership of g.thrownobj over to shk
           (container_impact_dmg handles item already owned by shop) */
        if (!IS_SOFT(levl[g.bhitpos.x][g.bhitpos.y].typ))
            /* <x,y> is spot where you initiated throw, not g.bhitpos */
            container_impact_dmg(obj, u.ux, u.uy);
        /* charge for items thrown out of shop;
           shk takes possession for items thrown into one */
        if ((*u.ushops || obj->unpaid) && obj != uball)
            check_shop_obj(obj, g.bhitpos.x, g.bhitpos.y, FALSE);

        stackobj(obj);
        if (obj == uball)
            drop_ball(g.bhitpos.x, g.bhitpos.y);
        if (cansee(g.bhitpos.x, g.bhitpos.y))
            newsym(g.bhitpos.x, g.bhitpos.y);
        if (obj_sheds_light(obj))
            g.vision_full_recalc = 1;
    }

 throwit_return:
    iflags.returning_missile = (genericptr_t) 0;
    if (clear_thrownobj)
        g.thrownobj = (struct obj *) 0;
    return;
}

/* an object may hit a monster; various factors adjust chance of hitting */
int
omon_adj(struct monst *mon, struct obj *obj, boolean mon_notices)
{
    int tmp = 0;

    /* size of target affects the chance of hitting */
    tmp += (mon->data->msize - MZ_MEDIUM); /* -2..+5 */
    /* sleeping target is more likely to be hit */
    if (mon->msleeping) {
        tmp += 2;
        if (mon_notices)
            mon->msleeping = 0;
    }
    /* ditto for immobilized target */
    if (!mon->mcanmove || !mon->data->mmove) {
        tmp += 4;
        if (mon_notices && mon->data->mmove && !rn2(10)) {
            mon->mcanmove = 1;
            mon->mfrozen = 0;
        }
    }
    /* some objects are more likely to hit than others */
    switch (obj->otyp) {
    case HEAVY_IRON_BALL:
        if (obj != uball)
            tmp += 2;
        break;
    case BOULDER:
        tmp += 6;
        break;
    default:
        if (obj->oclass == WEAPON_CLASS || is_weptool(obj)
            || obj->oclass == GEM_CLASS)
            tmp += hitval(obj, mon);
        break;
    }
    return tmp;
}

/* thrown object misses target monster */
static void
tmiss(struct obj *obj, struct monst *mon, boolean maybe_wakeup)
{
    const char *missile = mshot_xname(obj);

    /* If the target can't be seen or doesn't look like a valid target,
       avoid "the arrow misses it," or worse, "the arrows misses the mimic."
       An attentive player will still notice that this is different from
       an arrow just landing short of any target (no message in that case),
       so will realize that there is a valid target here anyway. */
    if (!canseemon(mon) || (M_AP_TYPE(mon) && M_AP_TYPE(mon) != M_AP_MONSTER))
        pline("%s %s.", The(missile), otense(obj, "miss"));
    else
        miss(missile, mon);
    if (maybe_wakeup && !rn2(3))
        wakeup(mon, TRUE);
    return;
}

#define quest_arti_hits_leader(obj, mon)      \
    (obj->oartifact && is_quest_artifact(obj) \
     && mon->m_id == g.quest_status.leader_m_id)

/*
 * Object thrown by player arrives at monster's location.
 * Return 1 if obj has disappeared or otherwise been taken care of,
 * 0 if caller must take care of it.
 * Also used for kicked objects and for polearms/grapnel applied at range.
 */
int
thitmonst(
    struct monst *mon,
    struct obj *obj) /* g.thrownobj or g.kickedobj or uwep */
{
    register int tmp;     /* Base chance to hit */
    register int disttmp; /* distance modifier */
    int otyp = obj->otyp, hmode;
    boolean guaranteed_hit = engulfing_u(mon);
    int dieroll;

    hmode = (obj == uwep) ? HMON_APPLIED
              : (obj == g.kickedobj) ? HMON_KICKED
                : HMON_THROWN;

    /* Differences from melee weapons:
     *
     * Dex still gives a bonus, but strength does not.
     * Polymorphed players lacking attacks may still throw.
     * There's a base -1 to hit.
     * No bonuses for fleeing or stunned targets (they don't dodge
     *    melee blows as readily, but dodging arrows is hard anyway).
     * Not affected by traps, etc.
     * Certain items which don't in themselves do damage ignore 'tmp'.
     * Distance and monster size affect chance to hit.
     */
    tmp = -1 + Luck + find_mac(mon) + u.uhitinc
          + maybe_polyd(g.youmonst.data->mlevel, u.ulevel);
    if (ACURR(A_DEX) < 4)
        tmp -= 3;
    else if (ACURR(A_DEX) < 6)
        tmp -= 2;
    else if (ACURR(A_DEX) < 8)
        tmp -= 1;
    else if (ACURR(A_DEX) >= 14)
        tmp += (ACURR(A_DEX) - 14);

    /* Modify to-hit depending on distance; but keep it sane.
     * Polearms get a distance penalty even when wielded; it's
     * hard to hit at a distance.
     */
    disttmp = 3 - distmin(u.ux, u.uy, mon->mx, mon->my);
    if (disttmp < -4)
        disttmp = -4;
    tmp += disttmp;

    /* gloves are a hindrance to proper use of bows */
    if (uarmg && uwep && objects[uwep->otyp].oc_skill == P_BOW) {
        switch (uarmg->otyp) {
        case GAUNTLETS_OF_POWER: /* metal */
            tmp -= 2;
            break;
        case GAUNTLETS_OF_FUMBLING:
            tmp -= 3;
            break;
        case LEATHER_GLOVES:
        case GAUNTLETS_OF_DEXTERITY:
            break;
        default:
            impossible("Unknown type of gloves (%d)", uarmg->otyp);
            break;
        }
    }

    tmp += omon_adj(mon, obj, TRUE);
    if (is_orc(mon->data)
        && maybe_polyd(is_elf(g.youmonst.data), Race_if(PM_ELF)))
        tmp++;
    if (guaranteed_hit) {
        tmp += 1000; /* Guaranteed hit */
    }

    /* throwing real gems to co-aligned unicorns boosts Luck,
       to cross-aligned unicorns changes Luck by random amount;
       throwing worthless glass doesn't affect Luck but doesn't anger them;
       3.7: treat rocks and gray stones as attacks rather than like glass
       and also treat gems or glass shot via sling as attacks */
    if (obj->oclass == GEM_CLASS && is_unicorn(mon->data)
        && objects[obj->otyp].oc_material != MINERAL && !uslinging()) {
        if (helpless(mon)) {
            tmiss(obj, mon, FALSE);
            return 0;
        } else if (mon->mtame) {
            pline("%s catches and drops %s.", Monnam(mon), the(xname(obj)));
            return 0;
        } else {
            pline("%s catches %s.", Monnam(mon), the(xname(obj)));
            return gem_accept(mon, obj);
        }
    }

    /* don't make game unwinnable if naive player throws artifact
       at leader... (kicked artifact is ok too; HMON_APPLIED could
       occur if quest artifact polearm or grapnel ever gets added) */
    if (hmode != HMON_APPLIED && quest_arti_hits_leader(obj, mon)) {
        /* AIS: changes to wakeup() means that it's now less inappropriate
           here than it used to be, but manual version works just as well */
        mon->msleeping = 0;
        mon->mstrategy &= ~STRAT_WAITMASK;

        if (mon->mcanmove) {
            pline("%s catches %s.", Monnam(mon), the(xname(obj)));
            if (mon->mpeaceful) {
                boolean next2u = monnear(mon, u.ux, u.uy);

                finish_quest(obj); /* acknowledge quest completion */
                pline("%s %s %s back to you.", Monnam(mon),
                      (next2u ? "hands" : "tosses"), the(xname(obj)));
                if (!next2u)
                    sho_obj_return_to_u(obj);
                obj = addinv(obj); /* back into your inventory */
                (void) encumber_msg();
            } else {
                /* angry leader caught it and isn't returning it */
                if (*u.ushops || obj->unpaid) /* not very likely... */
                    check_shop_obj(obj, mon->mx, mon->my, FALSE);
                (void) mpickobj(mon, obj);
            }
            return 1; /* caller doesn't need to place it */
        }
        return 0;
    }

    dieroll = rnd(20);

    if (obj->oclass == WEAPON_CLASS || is_weptool(obj)
        || obj->oclass == GEM_CLASS) {
        if (hmode == HMON_KICKED) {
            /* throwing adjustments and weapon skill bonus don't apply */
            tmp -= (is_ammo(obj) ? 5 : 3);
        } else if (is_ammo(obj)) {
            if (!ammo_and_launcher(obj, uwep)) {
                tmp -= 4;
            } else {
                tmp += uwep->spe - greatest_erosion(uwep);
                tmp += weapon_hit_bonus(uwep);
                if (uwep->oartifact)
                    tmp += spec_abon(uwep, mon);
                /*
                 * Elves and Samurais are highly trained w/bows,
                 * especially their own special types of bow.
                 * Polymorphing won't make you a bow expert.
                 */
                if ((Race_if(PM_ELF) || Role_if(PM_SAMURAI))
                    && (!Upolyd || your_race(g.youmonst.data))
                    && objects[uwep->otyp].oc_skill == P_BOW) {
                    tmp++;
                    if (Race_if(PM_ELF) && uwep->otyp == ELVEN_BOW)
                        tmp++;
                    else if (Role_if(PM_SAMURAI) && uwep->otyp == YUMI)
                        tmp++;
                }
            }
        } else { /* thrown non-ammo or applied polearm/grapnel */
            if (otyp == BOOMERANG) /* arbitrary */
                tmp += 4;
            else if (throwing_weapon(obj)) /* meant to be thrown */
                tmp += 2;
            else if (obj == g.thrownobj) /* not meant to be thrown */
                tmp -= 2;
            /* we know we're dealing with a weapon or weptool handled
               by WEAPON_SKILLS once ammo objects have been excluded */
            tmp += weapon_hit_bonus(obj);
        }

        if (tmp >= dieroll) {
            boolean wasthrown = (g.thrownobj != 0),
                    /* remember weapon attribute; hmon() might destroy obj */
                    chopper = is_axe(obj);

            /* attack hits mon */
            if (hmode == HMON_APPLIED) { /* ranged hit with wielded polearm */
                /* hmon()'s caller is expected to do this; however, hmon()
                   delivers the "hit with wielded weapon for first time"
                   gamelog message when applicable */
                u.uconduct.weaphit++;
            }
            if (hmon(mon, obj, hmode, dieroll)) { /* mon still alive */
                if (mon->wormno)
                    cutworm(mon, g.bhitpos.x, g.bhitpos.y, chopper);
            }
            exercise(A_DEX, TRUE);
            /* if hero was swallowed and projectile killed the engulfer,
               'obj' got added to engulfer's inventory and then dropped,
               so we can't safely use that pointer anymore; it escapes
               the chance to be used up here... */
            if (wasthrown && !g.thrownobj)
                return 1;

            /* projectiles other than magic stones sometimes disappear
               when thrown; projectiles aren't among the types of weapon
               that hmon() might have destroyed so obj is intact */
            if (objects[otyp].oc_skill < P_NONE
                && objects[otyp].oc_skill > -P_BOOMERANG
                && !objects[otyp].oc_magic) {
                /* we were breaking 2/3 of everything unconditionally.
                 * we still don't want anything to survive unconditionally,
                 * but we need ammo to stay around longer on average.
                 */
                int broken, chance;

                chance = 3 + greatest_erosion(obj) - obj->spe;
                if (chance > 1)
                    broken = rn2(chance);
                else
                    broken = !rn2(4);
                if (obj->blessed && !rnl(4))
                    broken = 0;

                /* Flint and hard gems don't break easily */
                if (((obj->oclass == GEM_CLASS && objects[otyp].oc_tough)
                     || obj->otyp == FLINT) && !rn2(2))
                    broken = 0;

                if (broken) {
                    if (*u.ushops || obj->unpaid)
                        check_shop_obj(obj, g.bhitpos.x, g.bhitpos.y, TRUE);
                    obfree(obj, (struct obj *) 0);
                    return 1;
                }
            }
            passive_obj(mon, obj, (struct attack *) 0);
        } else {
            tmiss(obj, mon, TRUE);
            if (hmode == HMON_APPLIED)
                wakeup(mon, TRUE);
        }

    } else if (otyp == HEAVY_IRON_BALL) {
        exercise(A_STR, TRUE);
        if (tmp >= dieroll) {
            int was_swallowed = guaranteed_hit;

            exercise(A_DEX, TRUE);
            if (!hmon(mon, obj, hmode, dieroll)) { /* mon killed */
                if (was_swallowed && !u.uswallow && obj == uball)
                    return 1; /* already did placebc() */
            }
        } else {
            tmiss(obj, mon, TRUE);
        }

    } else if (otyp == BOULDER) {
        exercise(A_STR, TRUE);
        if (tmp >= dieroll) {
            exercise(A_DEX, TRUE);
            (void) hmon(mon, obj, hmode, dieroll);
        } else {
            tmiss(obj, mon, TRUE);
        }

    } else if ((otyp == EGG || otyp == CREAM_PIE || otyp == BLINDING_VENOM
                || otyp == ACID_VENOM)
               && (guaranteed_hit || ACURR(A_DEX) > rnd(25))) {
        (void) hmon(mon, obj, hmode, dieroll);
        return 1; /* hmon used it up */

    } else if (obj->oclass == POTION_CLASS
               && (guaranteed_hit || ACURR(A_DEX) > rnd(25))) {
        potionhit(mon, obj, POTHIT_HERO_THROW);
        return 1;

    } else if (befriend_with_obj(mon->data, obj)
               || (mon->mtame && dogfood(mon, obj) <= ACCFOOD)) {
        if (tamedog(mon, obj)) {
            return 1; /* obj is gone */
        } else {
            tmiss(obj, mon, FALSE);
            mon->msleeping = 0;
            mon->mstrategy &= ~STRAT_WAITMASK;
        }
    } else if (guaranteed_hit) {
        char trail[BUFSZ];
        char *monname;
        struct permonst *md = u.ustuck->data;

        /* this assumes that guaranteed_hit is due to swallowing */
        wakeup(mon, TRUE);
        if (obj->otyp == CORPSE && touch_petrifies(&mons[obj->corpsenm])) {
            if (is_animal(md)) {
                minstapetrify(u.ustuck, TRUE);
                /* Don't leave a cockatrice corpse available in a statue */
                if (!u.uswallow) {
                    delobj(obj);
                    return 1;
                }
            }
        }
        Strcpy(trail,
               digests(md) ? " entrails" : is_whirly(md) ? " currents" : "");
        monname = mon_nam(mon);
        if (*trail)
            monname = s_suffix(monname);
        pline("%s into %s%s.", Tobjnam(obj, "vanish"), monname, trail);
    } else {
        tmiss(obj, mon, TRUE);
    }

    return 0;
}

static int
gem_accept(register struct monst *mon, register struct obj *obj)
{
    static NEARDATA const char
        nogood[]     = " is not interested in your junk.",
        acceptgift[] = " accepts your gift.",
        maybeluck[]  = " hesitatingly",
        noluck[]     = " graciously",
        addluck[]    = " gratefully";
    char buf[BUFSZ];
    boolean is_buddy = sgn(mon->data->maligntyp) == sgn(u.ualign.type);
    boolean is_gem = objects[obj->otyp].oc_material == GEMSTONE;
    int ret = 0;

    Strcpy(buf, Monnam(mon));
    mon->mpeaceful = 1;
    mon->mavenge = 0;

    /* object properly identified */
    if (obj->dknown && objects[obj->otyp].oc_name_known) {
        if (is_gem) {
            if (is_buddy) {
                Strcat(buf, addluck);
                change_luck(5);
            } else {
                Strcat(buf, maybeluck);
                change_luck(rn2(7) - 3);
            }
        } else {
            Strcat(buf, nogood);
            goto nopick;
        }

    /* making guesses */
    } else if (has_oname(obj) || objects[obj->otyp].oc_uname) {
        if (is_gem) {
            if (is_buddy) {
                Strcat(buf, addluck);
                change_luck(2);
            } else {
                Strcat(buf, maybeluck);
                change_luck(rn2(3) - 1);
            }
        } else {
            Strcat(buf, nogood);
            goto nopick;
        }

    /* value completely unknown to @ */
    } else {
        if (is_gem) {
            if (is_buddy) {
                Strcat(buf, addluck);
                change_luck(1);
            } else {
                Strcat(buf, maybeluck);
                change_luck(rn2(3) - 1);
            }
        } else {
            Strcat(buf, noluck);
        }
    }
    Strcat(buf, acceptgift);
    if (*u.ushops || obj->unpaid)
        check_shop_obj(obj, mon->mx, mon->my, TRUE);
    (void) mpickobj(mon, obj); /* may merge and free obj */
    ret = 1;

 nopick:
    if (!Blind)
        pline1(buf);
    if (!tele_restrict(mon))
        (void) rloc(mon, RLOC_MSG);
    return ret;
}

/*
 * Comments about the restructuring of the old breaks() routine.
 *
 * There are now three distinct phases to object breaking:
 *     breaktest() - which makes the check/decision about whether the
 *                   object is going to break.
 *     breakmsg()  - which outputs a message about the breakage,
 *                   appropriate for that particular object. Should
 *                   only be called after a positive breaktest().
 *                   on the object and, if it going to be called,
 *                   it must be called before calling breakobj().
 *                   Calling breakmsg() is optional.
 *     breakobj()  - which actually does the breakage and the side-effects
 *                   of breaking that particular object. This should
 *                   only be called after a positive breaktest() on the
 *                   object.
 *
 * Each of the above routines is currently static to this source module.
 * There are two routines callable from outside this source module which
 * perform the routines above in the correct sequence.
 *
 *   hero_breaks() - called when an object is to be broken as a result
 *                   of something that the hero has done. (throwing it,
 *                   kicking it, etc.)
 *   breaks()      - called when an object is to be broken for some
 *                   reason other than the hero doing something to it.
 */

/*
 * The hero causes breakage of an object (throwing, dropping it, etc.)
 * Return 0 if the object didn't break, 1 if the object broke.
 */
int
hero_breaks(struct obj *obj,
            coordxy x, coordxy y, /* object location (ox, oy may not be right) */
            unsigned breakflags)
{
    /* from_invent: thrown or dropped by player; maybe on shop bill;
       by-hero is implicit so callers don't need to specify BRK_BY_HERO */
    boolean from_invent = (breakflags & BRK_FROM_INV) != 0,
            in_view = Blind ? FALSE : (from_invent || cansee(x, y));
    unsigned brk = (breakflags & BRK_KNOWN_OUTCOME);

    /* only call breaktest if caller hasn't already specified the outcome */
    if (!brk)
        brk = breaktest(obj) ? BRK_KNOWN2BREAK : BRK_KNOWN2NOTBREAK;
    if (brk == BRK_KNOWN2NOTBREAK)
        return 0;

    breakmsg(obj, in_view);
    breakobj(obj, x, y, TRUE, from_invent);
    return 1;
}

/*
 * The object is going to break for a reason other than the hero doing
 * something to it.
 * Return 0 if the object doesn't break, 1 if the object broke.
 */
int
breaks(struct obj *obj,
       coordxy x, coordxy y) /* object location (ox, oy may not be right) */
{
    boolean in_view = Blind ? FALSE : cansee(x, y);

    if (!breaktest(obj))
        return 0;
    breakmsg(obj, in_view);
    breakobj(obj, x, y, FALSE, FALSE);
    return 1;
}

void
release_camera_demon(struct obj *obj, coordxy x, coordxy y)
{
    struct monst *mtmp;
    if (!rn2(3)
        && (mtmp = makemon(&mons[rn2(3) ? PM_HOMUNCULUS : PM_IMP], x, y,
                           MM_NOMSG)) != 0) {
        if (canspotmon(mtmp))
            pline("%s is released!", Hallucination
                                         ? An(rndmonnam(NULL))
                                         : "The picture-painting demon");
        mtmp->mpeaceful = !obj->cursed;
        set_malign(mtmp);
    }
}

/*
 * Unconditionally break an object. Assumes all resistance checks
 * and break messages have been delivered prior to getting here.
 */
void
breakobj(
    struct obj *obj,
    coordxy x, coordxy y,    /* object location (ox, oy may not be right) */
    boolean hero_caused, /* is this the hero's fault? */
    boolean from_invent)
{
    boolean fracture = FALSE;

    switch (obj->oclass == POTION_CLASS ? POT_WATER : obj->otyp) {
    case MIRROR:
        if (hero_caused)
            change_luck(-2);
        break;
    case POT_WATER:      /* really, all potions */
        obj->in_use = 1; /* in case it's fatal */
        if (obj->otyp == POT_OIL && obj->lamplit) {
            explode_oil(obj, x, y);
        } else if (next2u(x, y)) {
            if (!breathless(g.youmonst.data) || haseyes(g.youmonst.data)) {
                /* wet towel protects both eyes and breathing */
                if (obj->otyp != POT_WATER && !Half_gas_damage) {
                    if (!breathless(g.youmonst.data)) {
                        /* [what about "familiar odor" when known?] */
                        You("smell a peculiar odor...");
                    } else {
                        const char *eyes = body_part(EYE);

                        if (eyecount(g.youmonst.data) != 1)
                            eyes = makeplural(eyes);
                        Your("%s %s.", eyes, vtense(eyes, "water"));
                    }
                }
                potionbreathe(obj);
            }
        }
        /* monster breathing isn't handled... [yet?] */
        break;
    case EXPENSIVE_CAMERA:
        release_camera_demon(obj, x, y);
        break;
    case EGG:
        /* breaking your own eggs is bad luck */
        if (hero_caused && obj->spe && obj->corpsenm >= LOW_PM)
            change_luck((schar) -min(obj->quan, 5L));
        break;
    case BOULDER:
    case STATUE:
        /* caller will handle object disposition;
           we're just doing the shop theft handling */
        fracture = TRUE;
        break;
    default:
        break;
    }

    if (hero_caused) {
        if (from_invent || obj->unpaid) {
            if (*u.ushops || obj->unpaid)
                check_shop_obj(obj, x, y, TRUE);
        } else if (!obj->no_charge && costly_spot(x, y)) {
            /* it is assumed that the obj is a floor-object */
            char *o_shop = in_rooms(x, y, SHOPBASE);
            struct monst *shkp = shop_keeper(*o_shop);

            if (shkp) { /* (implies *o_shop != '\0') */
                struct eshk *eshkp = ESHK(shkp);

                /* base shk actions on her peacefulness at start of
                   this turn, so that "simultaneous" multiple breakage
                   isn't drastically worse than single breakage */
                if (g.hero_seq != eshkp->break_seq)
                    eshkp->seq_peaceful = shkp->mpeaceful;
                if ((stolen_value(obj, x, y, eshkp->seq_peaceful, FALSE) > 0L)
                    && (*o_shop != u.ushops[0] || !inside_shop(u.ux, u.uy))
                    && g.hero_seq != eshkp->break_seq)
                    make_angry_shk(shkp, x, y);
                /* make_angry_shk() is only called on the first instance
                   of breakage during any particular hero move */
                eshkp->break_seq = g.hero_seq;
            }
        }
    }
    if (!fracture)
        delobj(obj);
}

/*
 * Check to see if obj is going to break, but don't actually break it.
 * Return 0 if the object isn't going to break, 1 if it is.
 */
boolean
breaktest(struct obj *obj)
{
    if (obj_resists(obj, 1, 99))
        return 0;
    if (objects[obj->otyp].oc_material == GLASS && !obj->oartifact
        && obj->oclass != GEM_CLASS)
        return 1;
    switch (obj->oclass == POTION_CLASS ? POT_WATER : obj->otyp) {
    case EXPENSIVE_CAMERA:
    case POT_WATER: /* really, all potions */
    case EGG:
    case CREAM_PIE:
    case MELON:
    case ACID_VENOM:
    case BLINDING_VENOM:
        return 1;
    default:
        return 0;
    }
}

static void
breakmsg(struct obj *obj, boolean in_view)
{
    const char *to_pieces;

    to_pieces = "";
    switch (obj->oclass == POTION_CLASS ? POT_WATER : obj->otyp) {
    default: /* glass or crystal wand */
        if (obj->oclass != WAND_CLASS)
            impossible("breaking odd object?");
        /*FALLTHRU*/
    case CRYSTAL_PLATE_MAIL:
    case LENSES:
    case MIRROR:
    case CRYSTAL_BALL:
    case EXPENSIVE_CAMERA:
        to_pieces = " into a thousand pieces";
    /*FALLTHRU*/
    case POT_WATER: /* really, all potions */
        if (!in_view)
            You_hear("%s shatter!", something);
        else
            pline("%s shatter%s%s!", Doname2(obj),
                  (obj->quan == 1L) ? "s" : "", to_pieces);
        break;
    case EGG:
    case MELON:
        pline("Splat!");
        break;
    case CREAM_PIE:
        if (in_view)
            pline("What a mess!");
        break;
    case ACID_VENOM:
    case BLINDING_VENOM:
        pline("Splash!");
        break;
    }
}

static int
throw_gold(struct obj *obj)
{
    int range, odx, ody;
    register struct monst *mon;

    if (!u.dx && !u.dy && !u.dz) {
        You("cannot throw gold at yourself.");
        /* If we tried to throw part of a stack, force it to merge back
           together (same as in throw_obj).  Essential for gold. */
        if (obj->o_id == g.context.objsplit.parent_oid
            || obj->o_id == g.context.objsplit.child_oid)
            (void) unsplitobj(obj);
        return ECMD_CANCEL;
    }
    freeinv(obj);
    if (u.uswallow) {
        const char *swallower = mon_nam(u.ustuck);

        if (digests(u.ustuck->data))
            /* note: s_suffix() returns a modifiable buffer */
            swallower = strcat(s_suffix(swallower), " entrails");
        pline_The("gold disappears into %s.", swallower);
        add_to_minv(u.ustuck, obj);
        return ECMD_TIME;
    }

    if (u.dz) {
        if (u.dz < 0 && !Is_airlevel(&u.uz) && !Underwater
            && !Is_waterlevel(&u.uz)) {
            pline_The("gold hits the %s, then falls back on top of your %s.",
                      ceiling(u.ux, u.uy), body_part(HEAD));
            /* some self damage? */
            if (uarmh)
                pline("Fortunately, you are wearing %s!",
                      an(helm_simple_name(uarmh)));
        }
        g.bhitpos.x = u.ux;
        g.bhitpos.y = u.uy;
    } else {
        /* consistent with range for normal objects */
        range = (int) ((ACURRSTR) / 2 - obj->owt / 40);

        /* see if the gold has a place to move into */
        odx = u.ux + u.dx;
        ody = u.uy + u.dy;
        if (!ZAP_POS(levl[odx][ody].typ) || closed_door(odx, ody)) {
            g.bhitpos.x = u.ux;
            g.bhitpos.y = u.uy;
        } else {
            mon = bhit(u.dx, u.dy, range, THROWN_WEAPON,
                       (int (*)(MONST_P, OBJ_P)) 0,
                       (int (*)(OBJ_P, OBJ_P)) 0, &obj);
            if (!obj)
                return ECMD_TIME; /* object is gone */
            if (mon) {
                if (ghitm(mon, obj)) /* was it caught? */
                    return ECMD_TIME;
            } else {
                if (ship_object(obj, g.bhitpos.x, g.bhitpos.y, FALSE))
                    return ECMD_TIME;
            }
        }
    }

    if (flooreffects(obj, g.bhitpos.x, g.bhitpos.y, "fall"))
        return ECMD_TIME;
    if (u.dz > 0)
        pline_The("gold hits the %s.", surface(g.bhitpos.x, g.bhitpos.y));
    place_object(obj, g.bhitpos.x, g.bhitpos.y);
    if (*u.ushops)
        sellobj(obj, g.bhitpos.x, g.bhitpos.y);
    stackobj(obj);
    newsym(g.bhitpos.x, g.bhitpos.y);
    return ECMD_TIME;
}

/*dothrow.c*/

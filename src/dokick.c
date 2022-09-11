/* NetHack 3.7	dokick.c	$NHDT-Date: 1625963851 2021/07/11 00:37:31 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.167 $ */
/* Copyright (c) Izchak Miller, Mike Stephenson, Steve Linhart, 1989. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define is_bigfoot(x) ((x) == &mons[PM_SASQUATCH])
#define martial()                                 \
    (martial_bonus() || is_bigfoot(g.youmonst.data) \
     || (uarmf && uarmf->otyp == KICKING_BOOTS))

/* g.kickedobj (decl.c) tracks a kicked object until placed or destroyed */

static void kickdmg(struct monst *, boolean);
static boolean maybe_kick_monster(struct monst *, coordxy, coordxy);
static void kick_monster(struct monst *, coordxy, coordxy);
static int kick_object(coordxy, coordxy, char *);
static int really_kick_object(coordxy, coordxy);
static char *kickstr(char *, const char *);
static boolean watchman_thief_arrest(struct monst *);
static boolean watchman_door_damage(struct monst *, coordxy, coordxy);
static void kick_dumb(coordxy, coordxy);
static void kick_ouch(coordxy, coordxy, const char *);
static void otransit_msg(struct obj *, boolean, boolean, long);
static void drop_to(coord *, schar, coordxy, coordxy);

static const char kick_passes_thru[] = "kick passes harmlessly through";

/* kicking damage when not poly'd into a form with a kick attack */
static void
kickdmg(struct monst *mon, boolean clumsy)
{
    int mdx, mdy;
    int dmg = (ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 15;
    int specialdmg, kick_skill = P_NONE;
    boolean trapkilled = FALSE;

    if (uarmf && uarmf->otyp == KICKING_BOOTS)
        dmg += 5;

    /* excessive wt affects dex, so it affects dmg */
    if (clumsy)
        dmg /= 2;

    /* kicking a dragon or an elephant will not harm it */
    if (thick_skinned(mon->data))
        dmg = 0;

    /* attacking a shade is normally useless */
    if (mon->data == &mons[PM_SHADE])
        dmg = 0;

    specialdmg = special_dmgval(&g.youmonst, mon, W_ARMF, (long *) 0);

    if (mon->data == &mons[PM_SHADE] && !specialdmg) {
        pline_The("%s.", kick_passes_thru);
        /* doesn't exercise skill or abuse alignment or frighten pet,
           and shades have no passive counterattack */
        return;
    }

    if (M_AP_TYPE(mon))
        seemimic(mon);

    check_caitiff(mon);

    /* squeeze some guilt feelings... */
    if (mon->mtame) {
        abuse_dog(mon);
        if (mon->mtame)
            monflee(mon, (dmg ? rnd(dmg) : 1), FALSE, FALSE);
        else
            mon->mflee = 0;
    }

    if (dmg > 0) {
        /* convert potential damage to actual damage */
        dmg = rnd(dmg);
        if (martial()) {
            if (dmg > 1)
                kick_skill = P_MARTIAL_ARTS;
            dmg += rn2(ACURR(A_DEX) / 2 + 1);
        }
        /* a good kick exercises your dex */
        exercise(A_DEX, TRUE);
    }
    dmg += specialdmg; /* for blessed (or hypothetically, silver) boots */
    if (uarmf)
        dmg += uarmf->spe;
    dmg += u.udaminc; /* add ring(s) of increase damage */
    if (dmg > 0)
        mon->mhp -= dmg;
    if (!DEADMONSTER(mon) && martial() && !bigmonst(mon->data) && !rn2(3)
        && mon->mcanmove && mon != u.ustuck && !mon->mtrapped) {
        /* see if the monster has a place to move into */
        mdx = mon->mx + u.dx;
        mdy = mon->my + u.dy;
        /* TODO: replace with mhurtle? */
        if (goodpos(mdx, mdy, mon, 0)) {
            pline("%s reels from the blow.", Monnam(mon));
            if (m_in_out_region(mon, mdx, mdy)) {
                remove_monster(mon->mx, mon->my);
                newsym(mon->mx, mon->my);
                place_monster(mon, mdx, mdy);
                newsym(mon->mx, mon->my);
                set_apparxy(mon);
                if (mintrap(mon, NO_TRAP_FLAGS) == Trap_Killed_Mon)
                    trapkilled = TRUE;
            }
        }
    }

    (void) passive(mon, uarmf, TRUE, !DEADMONSTER(mon), AT_KICK, FALSE);
    if (DEADMONSTER(mon) && !trapkilled)
        killed(mon);

    /* may bring up a dialog, so put this after all messages */
    if (kick_skill != P_NONE) /* exercise proficiency */
        use_skill(kick_skill, 1);
}

static boolean
maybe_kick_monster(struct monst *mon, coordxy x, coordxy y)
{
    if (mon) {
        boolean save_forcefight = g.context.forcefight;

        g.bhitpos.x = x;
        g.bhitpos.y = y;
        if (!mon->mpeaceful || !canspotmon(mon))
            g.context.forcefight = TRUE; /* attack even if invisible */
        /* kicking might be halted by discovery of hidden monster,
           by player declining to attack peaceful monster,
           or by passing out due to encumbrance */
        if (attack_checks(mon, (struct obj *) 0) || overexertion())
            mon = 0; /* don't kick after all */
        g.context.forcefight = save_forcefight;
    }
    return (boolean) (mon != 0);
}

static void
kick_monster(struct monst *mon, coordxy x, coordxy y)
{
    boolean clumsy = FALSE;
    int i, j;

    /* anger target even if wild miss will occur */
    setmangry(mon, TRUE);

    if (Levitation && !rn2(3) && verysmall(mon->data)
        && !is_flyer(mon->data)) {
        pline("Floating in the air, you miss wildly!");
        exercise(A_DEX, FALSE);
        (void) passive(mon, uarmf, FALSE, 1, AT_KICK, FALSE);
        return;
    }

    /* reveal hidden target even if kick ends up missing (note: being
       hidden doesn't affect chance to hit so neither does this reveal) */
    if (mon->mundetected
        || (M_AP_TYPE(mon) && M_AP_TYPE(mon) != M_AP_MONSTER)) {
        if (M_AP_TYPE(mon))
            seemimic(mon);
        mon->mundetected = 0;
        if (!canspotmon(mon))
            map_invisible(x, y);
        else
            newsym(x, y);
        There("is %s here.",
              canspotmon(mon) ? a_monnam(mon) : "something hidden");
    }

    /* Kick attacks by kicking monsters are normal attacks, not special.
     * This is almost always worthless, since you can either take one turn
     * and do all your kicks, or else take one turn and attack the monster
     * normally, getting all your attacks _including_ all your kicks.
     * If you have >1 kick attack, you get all of them.
     */
    if (Upolyd && attacktype(g.youmonst.data, AT_KICK)) {
        struct attack *uattk;
        int sum, kickdieroll, armorpenalty, specialdmg,
            attknum = 0,
            tmp = find_roll_to_hit(mon, AT_KICK, (struct obj *) 0, &attknum,
                                   &armorpenalty);
        mon_maybe_wakeup_on_hit(mon);

        for (i = 0; i < NATTK; i++) {
            /* first of two kicks might have provoked counterattack
               that has incapacitated the hero (ie, floating eye) */
            if (g.multi < 0)
                break;

            uattk = &g.youmonst.data->mattk[i];
            /* we only care about kicking attacks here */
            if (uattk->aatyp != AT_KICK)
                continue;

            kickdieroll = rnd(20);
            specialdmg = special_dmgval(&g.youmonst, mon, W_ARMF, (long *) 0);
            if (mon->data == &mons[PM_SHADE] && !specialdmg) {
                /* doesn't matter whether it would have hit or missed,
                   and shades have no passive counterattack */
                Your("%s %s.", kick_passes_thru, mon_nam(mon));
                break; /* skip any additional kicks */
            } else if (tmp > kickdieroll) {
                You("kick %s.", mon_nam(mon));
                sum = damageum(mon, uattk, specialdmg);
                (void) passive(mon, uarmf, (sum != MM_MISS),
                               !(sum & MM_DEF_DIED), AT_KICK, FALSE);
                if ((sum & MM_DEF_DIED))
                    break; /* Defender died */
            } else {
                missum(mon, uattk, (tmp + armorpenalty > kickdieroll));
                (void) passive(mon, uarmf, FALSE, 1, AT_KICK, FALSE);
            }
        }
        return;
    }

    i = -inv_weight();
    j = weight_cap();

    /* What the following confusing if statements mean:
     * If you are over 70% of carrying capacity, you go through a "deal no
     * damage" check, and if that fails, a "clumsy kick" check.
     * At this % of carrycap | Chance of no damage | Chance of clumsiness
     *             [70%-80%) |                 1/4 |                  1/3
     *             [80%-90%) |                 1/3 |                  1/2
     *            [90%-100%) |                 1/2 |                   1
     */
    if (i < (j * 3) / 10) {
        if (!rn2((i < j / 10) ? 2 : (i < j / 5) ? 3 : 4)) {
            if (martial())
                goto doit;
            Your("clumsy kick does no damage.");
            (void) passive(mon, uarmf, FALSE, 1, AT_KICK, FALSE);
            return;
        }
        if (i < j / 10)
            clumsy = TRUE;
        else if (!rn2((i < j / 5) ? 2 : 3))
            clumsy = TRUE;
    }

    if (Fumbling)
        clumsy = TRUE;

    else if (uarm && objects[uarm->otyp].oc_bulky && ACURR(A_DEX) < rnd(25))
        clumsy = TRUE;
 doit:
    You("kick %s.", mon_nam(mon));
    if (!rn2(clumsy ? 3 : 4) && (clumsy || !bigmonst(mon->data))
        && mon->mcansee && !mon->mtrapped && !thick_skinned(mon->data)
        && mon->data->mlet != S_EEL && haseyes(mon->data) && mon->mcanmove
        && !mon->mstun && !mon->mconf && !mon->msleeping
        && mon->data->mmove >= 12) {
        if (!nohands(mon->data) && !rn2(martial() ? 5 : 3)) {
            pline("%s blocks your %skick.", Monnam(mon),
                  clumsy ? "clumsy " : "");
            (void) passive(mon, uarmf, FALSE, 1, AT_KICK, FALSE);
            return;
        } else {
            maybe_mnexto(mon);
            if (mon->mx != x || mon->my != y) {
                (void) unmap_invisible(x, y);
                pline("%s %s, %s evading your %skick.", Monnam(mon),
                      (can_teleport(mon->data) && !noteleport_level(mon))
                          ? "teleports"
                          : is_floater(mon->data)
                                ? "floats"
                                : is_flyer(mon->data) ? "swoops"
                                                      : (nolimbs(mon->data)
                                                         || slithy(mon->data))
                                                            ? "slides"
                                                            : "jumps",
                      clumsy ? "easily" : "nimbly", clumsy ? "clumsy " : "");
                (void) passive(mon, uarmf, FALSE, 1, AT_KICK, FALSE);
                return;
            }
        }
    }
    kickdmg(mon, clumsy);
}

/*
 *  Return TRUE if caught (the gold taken care of), FALSE otherwise.
 *  The gold object is *not* attached to the fobj chain!
 */
boolean
ghitm(register struct monst *mtmp, register struct obj *gold)
{
    boolean msg_given = FALSE;

    if (!likes_gold(mtmp->data) && !mtmp->isshk && !mtmp->ispriest
        && !mtmp->isgd && !is_mercenary(mtmp->data)) {
        wakeup(mtmp, TRUE);
    } else if (!mtmp->mcanmove) {
        /* too light to do real damage */
        if (canseemon(mtmp)) {
            pline_The("%s harmlessly %s %s.", xname(gold),
                      otense(gold, "hit"), mon_nam(mtmp));
            msg_given = TRUE;
        }
    } else {
        long umoney, value = gold->quan * objects[gold->otyp].oc_cost;

        mtmp->msleeping = 0;
        finish_meating(mtmp);
        if (!mtmp->isgd && !rn2(4)) /* not always pleasing */
            setmangry(mtmp, TRUE);
        /* greedy monsters catch gold */
        if (cansee(mtmp->mx, mtmp->my))
            pline("%s catches the gold.", Monnam(mtmp));
        (void) mpickobj(mtmp, gold);
        gold = (struct obj *) 0; /* obj has been freed */
        if (mtmp->isshk) {
            long robbed = ESHK(mtmp)->robbed;

            if (robbed) {
                robbed -= value;
                if (robbed < 0L)
                    robbed = 0L;
                pline_The("amount %scovers %s recent losses.",
                          !robbed ? "" : "partially ", mhis(mtmp));
                ESHK(mtmp)->robbed = robbed;
                if (!robbed)
                    make_happy_shk(mtmp, FALSE);
            } else {
                if (mtmp->mpeaceful) {
                    ESHK(mtmp)->credit += value;
                    You("have %ld %s in credit.", ESHK(mtmp)->credit,
                        currency(ESHK(mtmp)->credit));
                } else
                    verbalize("Thanks, scum!");
            }
        } else if (mtmp->ispriest) {
            if (mtmp->mpeaceful)
                verbalize("Thank you for your contribution.");
            else
                verbalize("Thanks, scum!");
        } else if (mtmp->isgd) {
            umoney = money_cnt(g.invent);
            /* Some of these are iffy, because a hostile guard
               won't become peaceful and resume leading hero
               out of the vault.  If he did do that, player
               could try fighting, then weasle out of being
               killed by throwing his/her gold when losing. */
            verbalize(umoney ? "Drop the rest and follow me."
                      : hidden_gold(TRUE)
                        ? "You still have hidden gold.  Drop it now."
                        : mtmp->mpeaceful
                          ? "I'll take care of that; please move along."
                          : "I'll take that; now get moving.");
        } else if (is_mercenary(mtmp->data)) {
            boolean was_angry = !mtmp->mpeaceful;
            long goldreqd = 0L;

            if (mtmp->data == &mons[PM_SOLDIER])
                goldreqd = 100L;
            else if (mtmp->data == &mons[PM_SERGEANT])
                goldreqd = 250L;
            else if (mtmp->data == &mons[PM_LIEUTENANT])
                goldreqd = 500L;
            else if (mtmp->data == &mons[PM_CAPTAIN])
                goldreqd = 750L;

            if (goldreqd && rn2(3)) {
                umoney = money_cnt(g.invent);
                goldreqd += (umoney + u.ulevel * rn2(5)) / ACURR(A_CHA);
                if (value > goldreqd)
                    mtmp->mpeaceful = TRUE;
            }

            if (!mtmp->mpeaceful) {
                if (goldreqd)
                    verbalize("That's not enough, coward!");
                else /* unbribeable (watchman) */
                    verbalize("I don't take bribes from scum like you!");
            } else if (was_angry) {
                verbalize("That should do.  Now beat it!");
            } else {
                verbalize("Thanks for the tip, %s.",
                          flags.female ? "lady" : "buddy");
            }
        }
        return TRUE;
    }

    if (!msg_given)
        miss(xname(gold), mtmp);
    return FALSE;
}

/* container is kicked, dropped, thrown or otherwise impacted by player.
 * Assumes container is on floor.  Checks contents for possible damage. */
void
container_impact_dmg(struct obj *obj, coordxy x,
                     coordxy y) /* coordinates where object was before the impact, not after */
{
    struct monst *shkp;
    struct obj *otmp, *otmp2;
    long loss = 0L;
    boolean costly, insider, frominv;

    /* only consider normal containers */
    if (!Is_container(obj) || !Has_contents(obj) || Is_mbag(obj))
        return;

    costly = ((shkp = shop_keeper(*in_rooms(x, y, SHOPBASE)))
              && costly_spot(x, y));
    insider = (*u.ushops && inside_shop(u.ux, u.uy)
               && *in_rooms(x, y, SHOPBASE) == *u.ushops);
    /* if dropped or thrown, shop ownership flags are set on this obj */
    frominv = (obj != g.kickedobj);

    for (otmp = obj->cobj; otmp; otmp = otmp2) {
        const char *result = (char *) 0;

        otmp2 = otmp->nobj;
        if (objects[otmp->otyp].oc_material == GLASS
            && otmp->oclass != GEM_CLASS && !obj_resists(otmp, 33, 100)) {
            result = "shatter";
        } else if (otmp->otyp == EGG && !rn2(3)) {
            result = "cracking";
        }
        if (result) {
            if (otmp->otyp == MIRROR)
                change_luck(-2);

            /* eggs laid by you.  penalty is -1 per egg, max 5,
             * but it's always exactly 1 that breaks */
            if (otmp->otyp == EGG && otmp->spe && otmp->corpsenm >= LOW_PM)
                change_luck(-1);
            You_hear("a muffled %s.", result);
            if (costly) {
                if (frominv && !otmp->unpaid)
                    otmp->no_charge = 1;
                loss +=
                    stolen_value(otmp, x, y, (boolean) shkp->mpeaceful, TRUE);
            }
            if (otmp->quan > 1L) {
                useup(otmp);
            } else {
                obj_extract_self(otmp);
                obfree(otmp, (struct obj *) 0);
            }
            /* contents of this container are no longer known */
            obj->cknown = 0;
        }
    }
    if (costly && loss) {
        if (!insider) {
            You("caused %ld %s worth of damage!", loss, currency(loss));
            make_angry_shk(shkp, x, y);
        } else {
            You("owe %s %ld %s for objects destroyed.", shkname(shkp), loss,
                currency(loss));
        }
    }
}

/* jacket around really_kick_object */
static int
kick_object(coordxy x, coordxy y, char *kickobjnam)
{
    int res = 0;

    *kickobjnam = '\0';
    /* if a pile, the "top" object gets kicked */
    g.kickedobj = g.level.objects[x][y];
    if (g.kickedobj) {
        /* kick object; if doing is fatal, done() will clean up g.kickedobj */
        Strcpy(kickobjnam, killer_xname(g.kickedobj)); /* matters iff res==0 */
        res = really_kick_object(x, y);
        g.kickedobj = (struct obj *) 0;
    }
    return res;
}

/* guts of kick_object */
static int
really_kick_object(coordxy x, coordxy y)
{
    int range;
    struct monst *mon, *shkp = 0;
    struct trap *trap;
    char bhitroom;
    boolean costly, isgold, slide = FALSE;

    /* g.kickedobj should always be set due to conditions of call */
    if (!g.kickedobj || g.kickedobj->otyp == BOULDER || g.kickedobj == uball
        || g.kickedobj == uchain)
        return 0;

    if ((trap = t_at(x, y)) != 0) {
        if ((is_pit(trap->ttyp) && !Passes_walls) || trap->ttyp == WEB) {
            if (!trap->tseen)
                find_trap(trap);
            You_cant("kick %s that's in a %s!", something,
                     Hallucination ? "tizzy"
                         : (trap->ttyp == WEB) ? "web"
                             : "pit");
            return 1;
        }
        if (trap->ttyp == STATUE_TRAP) {
            activate_statue_trap(trap, x,y, FALSE);
            return 1;
        }
    }

    if (Fumbling && !rn2(3)) {
        Your("clumsy kick missed.");
        return 1;
    }

    if (!uarmf && g.kickedobj->otyp == CORPSE
        && touch_petrifies(&mons[g.kickedobj->corpsenm])
        && !Stone_resistance) {
        You("kick %s with your bare %s.",
            corpse_xname(g.kickedobj, (const char *) 0, CXN_PFX_THE),
            makeplural(body_part(FOOT)));
        if (poly_when_stoned(g.youmonst.data) && polymon(PM_STONE_GOLEM)) {
            ; /* hero has been transformed but kick continues */
        } else {
            /* normalize body shape here; foot, not body_part(FOOT) */
            Sprintf(g.killer.name, "kicking %s barefoot",
                    killer_xname(g.kickedobj));
            instapetrify(g.killer.name);
        }
    }

    isgold = (g.kickedobj->oclass == COIN_CLASS);
    {
        int k_owt = (int) g.kickedobj->owt;

        /* for non-gold stack, 1 item will be split off below (unless an
           early return occurs, so we aren't moving the split to here);
           calculate the range for that 1 rather than for the whole stack */
        if (g.kickedobj->quan > 1L && !isgold) {
            long save_quan = g.kickedobj->quan;

            g.kickedobj->quan = 1L;
            k_owt = weight(g.kickedobj);
            g.kickedobj->quan = save_quan;
        }

        /* range < 2 means the object will not move
           (maybe dexterity should also figure here) */
        range = ((int) ACURRSTR) / 2 - k_owt / 40;
    }

    if (martial())
        range += rnd(3);

    if (is_pool(x, y)) {
        /* you're in the water too; significantly reduce range */
        range = range / 3 + 1; /* {1,2}=>1, {3,4,5}=>2, {6,7,8}=>3 */
    } else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
        /* you're in air, since is_pool did not match */
        range += rnd(3);
    } else {
        if (is_ice(x, y))
            range += rnd(3), slide = TRUE;
        if (g.kickedobj->greased)
            range += rnd(3), slide = TRUE;
    }

    /* Mjollnir is magically too heavy to kick */
    if (is_art(g.kickedobj, ART_MJOLLNIR))
        range = 1;

    /* see if the object has a place to move into */
    if (!ZAP_POS(levl[x + u.dx][y + u.dy].typ)
        || closed_door(x + u.dx, y + u.dy))
        range = 1;

    costly = (!(g.kickedobj->no_charge && !Has_contents(g.kickedobj))
              && (shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 0
              && costly_spot(x, y));

    if (IS_ROCK(levl[x][y].typ) || closed_door(x, y)) {
        if ((!martial() && rn2(20) > ACURR(A_DEX))
            || IS_ROCK(levl[u.ux][u.uy].typ) || closed_door(u.ux, u.uy)) {
            if (Blind)
                pline("It doesn't come loose.");
            else
                pline("%s %sn't come loose.",
                      The(distant_name(g.kickedobj, xname)),
                      otense(g.kickedobj, "do"));
            return (!rn2(3) || martial());
        }
        if (Blind)
            pline("It comes loose.");
        else
            pline("%s %s loose.", The(distant_name(g.kickedobj, xname)),
                  otense(g.kickedobj, "come"));
        obj_extract_self(g.kickedobj);
        newsym(x, y);
        if (costly && (!costly_spot(u.ux, u.uy)
                       || !index(u.urooms, *in_rooms(x, y, SHOPBASE))))
            addtobill(g.kickedobj, FALSE, FALSE, FALSE);
        if (!flooreffects(g.kickedobj, u.ux, u.uy, "fall")) {
            place_object(g.kickedobj, u.ux, u.uy);
            stackobj(g.kickedobj);
            newsym(u.ux, u.uy);
        }
        return 1;
    }

    /* a box gets a chance of breaking open here */
    if (Is_box(g.kickedobj)) {
        boolean otrp = g.kickedobj->otrapped;

        if (range < 2)
            pline("THUD!");
        container_impact_dmg(g.kickedobj, x, y);
        if (g.kickedobj->olocked) {
            if (!rn2(5) || (martial() && !rn2(2))) {
                You("break open the lock!");
                breakchestlock(g.kickedobj, FALSE);
                if (otrp)
                    (void) chest_trap(g.kickedobj, LEG, FALSE);
                return 1;
            }
        } else {
            if (!rn2(3) || (martial() && !rn2(2))) {
                pline_The("lid slams open, then falls shut.");
                g.kickedobj->lknown = 1;
                if (otrp)
                    (void) chest_trap(g.kickedobj, LEG, FALSE);
                return 1;
            }
        }
        if (range < 2)
            return 1;
        /* else let it fall through to the next cases... */
    }

    /* fragile objects should not be kicked */
    if (hero_breaks(g.kickedobj, g.kickedobj->ox, g.kickedobj->oy, 0))
        return 1;

    /* too heavy to move.  range is calculated as potential distance from
     * player, so range == 2 means the object may move up to one square
     * from its current position
     */
    if (range < 2) {
        if (!Is_box(g.kickedobj))
            pline("Thump!");
        return (!rn2(3) || martial());
    }

    if (g.kickedobj->quan > 1L) {
        if (!isgold) {
            g.kickedobj = splitobj(g.kickedobj, 1L);
        } else {
            if (rn2(20)) {
                static NEARDATA const char *const flyingcoinmsg[] = {
                    "scatter the coins", "knock coins all over the place",
                    "send coins flying in all directions",
                };

                if (!Deaf)
                    pline1("Thwwpingg!");
                You("%s!", flyingcoinmsg[rn2(SIZE(flyingcoinmsg))]);
                (void) scatter(x, y, rnd(3), VIS_EFFECTS | MAY_HIT,
                               g.kickedobj);
                newsym(x, y);
                return 1;
            }
            if (g.kickedobj->quan > 300L) {
                pline("Thump!");
                return (!rn2(3) || martial());
            }
        }
    }

    if (slide && !Blind)
        pline("Whee!  %s %s across the %s.", Doname2(g.kickedobj),
              otense(g.kickedobj, "slide"), surface(x, y));

    if (costly && !isgold)
        addtobill(g.kickedobj, FALSE, FALSE, TRUE);
    obj_extract_self(g.kickedobj);
    (void) snuff_candle(g.kickedobj);
    newsym(x, y);
    mon = bhit(u.dx, u.dy, range, KICKED_WEAPON,
               (int (*) (struct monst *, struct obj *)) 0,
               (int (*) (struct obj *, struct obj *)) 0, &g.kickedobj);
    if (!g.kickedobj)
        return 1; /* object broken */

    if (mon) {
        if (mon->isshk && g.kickedobj->where == OBJ_MINVENT
            && g.kickedobj->ocarry == mon)
            return 1; /* alert shk caught it */
        g.notonhead = (mon->mx != g.bhitpos.x || mon->my != g.bhitpos.y);
        if (isgold ? ghitm(mon, g.kickedobj)      /* caught? */
                   : thitmonst(mon, g.kickedobj)) /* hit && used up? */
            return 1;
    }

    /* the object might have fallen down a hole;
       ship_object() will have taken care of shop billing */
    if (g.kickedobj->where == OBJ_MIGRATING)
        return 1;

    bhitroom = *in_rooms(g.bhitpos.x, g.bhitpos.y, SHOPBASE);
    if (costly && (!costly_spot(g.bhitpos.x, g.bhitpos.y)
                   || *in_rooms(x, y, SHOPBASE) != bhitroom)) {
        if (isgold)
            costly_gold(x, y, g.kickedobj->quan, FALSE);
        else
            (void) stolen_value(g.kickedobj, x, y, (boolean) shkp->mpeaceful,
                                FALSE);
        costly = FALSE; /* already billed */
    }

    if (flooreffects(g.kickedobj, g.bhitpos.x, g.bhitpos.y, "fall"))
        return 1;
    if (costly) {
        long gt = 0L;

        /* costly + landed outside shop handled above; must be inside shop */
        if (g.kickedobj->unpaid)
            subfrombill(g.kickedobj, shkp);

        /* if billed for contained gold during kick, get a refund now */
        if (Has_contents(g.kickedobj)
            && (gt = contained_gold(g.kickedobj, TRUE)) > 0L)
            donate_gold(gt, shkp, FALSE);
    }
    place_object(g.kickedobj, g.bhitpos.x, g.bhitpos.y);
    stackobj(g.kickedobj);
    newsym(g.kickedobj->ox, g.kickedobj->oy);
    return 1;
}

/* cause of death if kicking kills kicker */
static char *
kickstr(char *buf, const char *kickobjnam)
{
    const char *what;

    if (*kickobjnam)
        what = kickobjnam;
    else if (g.maploc == &g.nowhere)
        what = "nothing";
    else if (IS_DOOR(g.maploc->typ))
        what = "a door";
    else if (IS_TREE(g.maploc->typ))
        what = "a tree";
    else if (IS_STWALL(g.maploc->typ))
        what = "a wall";
    else if (IS_ROCK(g.maploc->typ))
        what = "a rock";
    else if (IS_THRONE(g.maploc->typ))
        what = "a throne";
    else if (IS_FOUNTAIN(g.maploc->typ))
        what = "a fountain";
    else if (IS_GRAVE(g.maploc->typ))
        what = "a headstone";
    else if (IS_SINK(g.maploc->typ))
        what = "a sink";
    else if (IS_ALTAR(g.maploc->typ))
        what = "an altar";
    else if (IS_DRAWBRIDGE(g.maploc->typ))
        what = "a drawbridge";
    else if (g.maploc->typ == STAIRS)
        what = "the stairs";
    else if (g.maploc->typ == LADDER)
        what = "a ladder";
    else if (g.maploc->typ == IRONBARS)
        what = "an iron bar";
    else
        what = "something weird";
    return strcat(strcpy(buf, "kicking "), what);
}

static boolean
watchman_thief_arrest(struct monst *mtmp)
{
    if (is_watch(mtmp->data) && couldsee(mtmp->mx, mtmp->my)
        && mtmp->mpeaceful) {
        mon_yells(mtmp, "Halt, thief!  You're under arrest!");
        (void) angry_guards(FALSE);
        return TRUE;
    }
    return FALSE;
}

static boolean
watchman_door_damage(struct monst *mtmp, coordxy x, coordxy y)
{
    if (is_watch(mtmp->data) && mtmp->mpeaceful
        && couldsee(mtmp->mx, mtmp->my)) {
        if (levl[x][y].looted & D_WARNED) {
            mon_yells(mtmp,
                      "Halt, vandal!  You're under arrest!");
            (void) angry_guards(FALSE);
        } else {
            mon_yells(mtmp, "Hey, stop damaging that door!");
            levl[x][y].looted |= D_WARNED;
        }
        return TRUE;
    }
    return FALSE;
}

static void
kick_dumb(coordxy x, coordxy y)
{
    exercise(A_DEX, FALSE);
    if (martial() || ACURR(A_DEX) >= 16 || rn2(3)) {
        You("kick at empty space.");
        if (Blind)
            feel_location(x, y);
    } else {
        pline("Dumb move!  You strain a muscle.");
        exercise(A_STR, FALSE);
        set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
    }
    if ((Is_airlevel(&u.uz) || Levitation) && rn2(2))
        hurtle(-u.dx, -u.dy, 1, TRUE);
}

static void
kick_ouch(coordxy x, coordxy y, const char *kickobjnam)
{
    int dmg;
    char buf[BUFSZ];

    pline("Ouch!  That hurts!");
    exercise(A_DEX, FALSE);
    exercise(A_STR, FALSE);
    if (isok(x, y)) {
        if (Blind)
            feel_location(x, y); /* we know we hit it */
        if (is_drawbridge_wall(x, y) >= 0) {
            pline_The("drawbridge is unaffected.");
            /* update maploc to refer to the drawbridge */
            (void) find_drawbridge(&x, &y);
            g.maploc = &levl[x][y];
        }
    }
    if (!rn2(3))
        set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
    dmg = rnd(ACURR(A_CON) > 15 ? 3 : 5);
    losehp(Maybe_Half_Phys(dmg), kickstr(buf, kickobjnam), KILLED_BY);
    if (Is_airlevel(&u.uz) || Levitation)
        hurtle(-u.dx, -u.dy, rn1(2, 4), TRUE); /* assume it's heavy */
}

/* the #kick command */
int
dokick(void)
{
    coordxy x, y;
    int avrg_attrib;
    int glyph, oldglyph = -1;
    register struct monst *mtmp;
    boolean no_kick = FALSE;
    char buf[BUFSZ];

    if (nolimbs(g.youmonst.data) || slithy(g.youmonst.data)) {
        You("have no legs to kick with.");
        no_kick = TRUE;
    } else if (verysmall(g.youmonst.data)) {
        You("are too small to do any kicking.");
        no_kick = TRUE;
    } else if (u.usteed) {
        if (yn_function("Kick your steed?", ynchars, 'y', TRUE) == 'y') {
            You("kick %s.", mon_nam(u.usteed));
            kick_steed();
            return ECMD_TIME;
        } else {
            return ECMD_OK;
        }
    } else if (Wounded_legs) {
        legs_in_no_shape("kicking", FALSE);
        no_kick = TRUE;
    } else if (near_capacity() > SLT_ENCUMBER) {
        Your("load is too heavy to balance yourself for a kick.");
        no_kick = TRUE;
    } else if (g.youmonst.data->mlet == S_LIZARD) {
        Your("legs cannot kick effectively.");
        no_kick = TRUE;
    } else if (u.uinwater && !rn2(2)) {
        Your("slow motion kick doesn't hit anything.");
        no_kick = TRUE;
    } else if (u.utrap) {
        no_kick = TRUE;
        switch (u.utraptype) {
        case TT_PIT:
            if (!Passes_walls)
                pline("There's not enough room to kick down here.");
            else
                no_kick = FALSE;
            break;
        case TT_WEB:
        case TT_BEARTRAP:
            You_cant("move your %s!", body_part(LEG));
            break;
        default:
            break;
        }
    } else if (sobj_at(BOULDER, u.ux, u.uy) && !Passes_walls) {
        pline("There's not enough room to kick in here.");
        no_kick = TRUE;
    }

    if (no_kick) {
        /* ignore direction typed before player notices kick failed */
        display_nhwindow(WIN_MESSAGE, TRUE); /* --More-- */
        return ECMD_FAIL;
    }

    if (!getdir((char *) 0))
        return ECMD_CANCEL;
    if (!u.dx && !u.dy)
        return ECMD_CANCEL;

    x = u.ux + u.dx;
    y = u.uy + u.dy;

    /* KMH -- Kicking boots always succeed */
    if (uarmf && uarmf->otyp == KICKING_BOOTS)
        avrg_attrib = 99;
    else
        avrg_attrib = (ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 3;

    if (u.uswallow) {
        switch (rn2(3)) {
        case 0:
            You_cant("move your %s!", body_part(LEG));
            break;
        case 1:
            if (digests(u.ustuck->data)) {
                pline("%s burps loudly.", Monnam(u.ustuck));
                break;
            }
            /*FALLTHRU*/
        default:
            Your("feeble kick has no effect.");
            break;
        }
        return ECMD_TIME;
    } else if (u.utrap && u.utraptype == TT_PIT) {
        /* must be Passes_walls */
        You("kick at the side of the pit.");
        return ECMD_TIME;
    }
    if (Levitation) {
        coordxy xx, yy;

        xx = u.ux - u.dx;
        yy = u.uy - u.dy;
        /* doors can be opened while levitating, so they must be
         * reachable for bracing purposes
         * Possible extension: allow bracing against stuff on the side?
         */
        if (isok(xx, yy) && !IS_ROCK(levl[xx][yy].typ)
            && !IS_DOOR(levl[xx][yy].typ)
            && (!Is_airlevel(&u.uz) || !OBJ_AT(xx, yy))) {
            You("have nothing to brace yourself against.");
            return ECMD_OK;
        }
    }

    mtmp = isok(x, y) ? m_at(x, y) : 0;
    /* might not kick monster if it is hidden and becomes revealed,
       if it is peaceful and player declines to attack, or if the
       hero passes out due to encumbrance with low hp; g.context.move
       will be 1 unless player declines to kick peaceful monster */
    if (mtmp) {
        oldglyph = glyph_at(x, y);
        if (!maybe_kick_monster(mtmp, x, y))
            return (g.context.move ? ECMD_TIME : ECMD_OK);
    }

    wake_nearby();
    u_wipe_engr(2);

    if (!isok(x, y)) {
        g.maploc = &g.nowhere;
        kick_ouch(x, y, "");
        return ECMD_TIME;
    }
    g.maploc = &levl[x][y];

    /*
     * The next five tests should stay in their present order:
     * monsters, pools, objects, non-doors, doors.
     *
     * [FIXME:  Monsters who are hidden underneath objects or
     * in pools should lead to hero kicking the concealment
     * rather than the monster, probably exposing the hidden
     * monster in the process.  And monsters who are hidden on
     * ceiling shouldn't be kickable (unless hero is flying?);
     * kicking toward them should just target whatever is on
     * the floor at that spot.]
     */

    if (mtmp) {
        /* save mtmp->data (for recoil) in case mtmp gets killed */
        struct permonst *mdat = mtmp->data;

        kick_monster(mtmp, x, y);
        glyph = glyph_at(x, y);
        /* see comment in attack_checks() */
        if (DEADMONSTER(mtmp)) { /* DEADMONSTER() */
            /* if we mapped an invisible monster and immediately
               killed it, we don't want to forget what we thought
               was there before the kick */
            if (glyph != oldglyph && glyph_is_invisible(glyph))
                show_glyph(x, y, oldglyph);
        } else if (!canspotmon(mtmp)
                   /* check <x,y>; monster that evades kick by jumping
                      to an unseen square doesn't leave an I behind */
                   && mtmp->mx == x && mtmp->my == y
                   && !glyph_is_invisible(glyph)
                   && !engulfing_u(mtmp)) {
            map_invisible(x, y);
        }
        /* recoil if floating */
        if ((Is_airlevel(&u.uz) || Levitation) && g.context.move) {
            int range;

            range =
                ((int) g.youmonst.data->cwt + (weight_cap() + inv_weight()));
            if (range < 1)
                range = 1; /* divide by zero avoidance */
            range = (3 * (int) mdat->cwt) / range;

            if (range < 1)
                range = 1;
            hurtle(-u.dx, -u.dy, range, TRUE);
        }
        return ECMD_TIME;
    }
    (void) unmap_invisible(x, y);
    if (is_pool(x, y) ^ !!u.uinwater) {
        /* objects normally can't be removed from water by kicking */
        You("splash some %s around.", hliquid("water"));
        return ECMD_TIME;
    }

    if (OBJ_AT(x, y) && (!Levitation || Is_airlevel(&u.uz)
                         || Is_waterlevel(&u.uz) || sobj_at(BOULDER, x, y))) {
        char kickobjnam[BUFSZ];

        if (kick_object(x, y, kickobjnam)) {
            if (Is_airlevel(&u.uz))
                hurtle(-u.dx, -u.dy, 1, TRUE); /* assume it's light */
            return ECMD_TIME;
        }
        kick_ouch(x, y, kickobjnam);
        return ECMD_TIME;
    }

    if (!IS_DOOR(g.maploc->typ)) {
        if (g.maploc->typ == SDOOR) {
            if (!Levitation && rn2(30) < avrg_attrib) {
                cvt_sdoor_to_door(g.maploc); /* ->typ = DOOR */
                pline("Crash!  %s a secret door!",
                      /* don't "kick open" when it's locked
                         unless it also happens to be trapped */
                      (g.maploc->doormask & (D_LOCKED | D_TRAPPED)) == D_LOCKED
                          ? "Your kick uncovers"
                          : "You kick open");
                exercise(A_DEX, TRUE);
                if (g.maploc->doormask & D_TRAPPED) {
                    g.maploc->doormask = D_NODOOR;
                    b_trapped("door", FOOT);
                } else if (g.maploc->doormask != D_NODOOR
                           && !(g.maploc->doormask & D_LOCKED))
                    g.maploc->doormask = D_ISOPEN;
                feel_newsym(x, y); /* we know it's gone */
                if (g.maploc->doormask == D_ISOPEN
                    || g.maploc->doormask == D_NODOOR)
                    unblock_point(x, y); /* vision */
                return ECMD_TIME;
            } else {
                kick_ouch(x, y, "");
                return ECMD_TIME;
            }
        }
        if (g.maploc->typ == SCORR) {
            if (!Levitation && rn2(30) < avrg_attrib) {
                pline("Crash!  You kick open a secret passage!");
                exercise(A_DEX, TRUE);
                g.maploc->typ = CORR;
                feel_newsym(x, y); /* we know it's gone */
                unblock_point(x, y); /* vision */
                return ECMD_TIME;
            } else {
                kick_ouch(x, y, "");
                return ECMD_TIME;
            }
        }
        if (IS_THRONE(g.maploc->typ)) {
            register int i;
            if (Levitation) {
                kick_dumb(x, y);
                return ECMD_TIME;
            }
            if ((Luck < 0 || g.maploc->looted) && !rn2(3)) {
                g.maploc->looted = 0; /* don't leave loose ends.. */
                g.maploc->typ = ROOM;
                (void) mkgold((long) rnd(200), x, y);
                if (Blind)
                    pline("CRASH!  You destroy it.");
                else {
                    pline("CRASH!  You destroy the throne.");
                    newsym(x, y);
                }
                exercise(A_DEX, TRUE);
                return ECMD_TIME;
            } else if (Luck > 0 && !rn2(3) && !g.maploc->looted) {
                (void) mkgold((long) rn1(201, 300), x, y);
                i = Luck + 1;
                if (i > 6)
                    i = 6;
                while (i--)
                    (void) mksobj_at(
                        rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE - 1), x, y,
                        FALSE, TRUE);
                if (Blind)
                    You("kick %s loose!", something);
                else {
                    You("kick loose some ornamental coins and gems!");
                    newsym(x, y);
                }
                /* prevent endless milking */
                g.maploc->looted = T_LOOTED;
                return ECMD_TIME;
            } else if (!rn2(4)) {
                if (dunlev(&u.uz) < dunlevs_in_dungeon(&u.uz)) {
                    fall_through(FALSE, 0);
                    return ECMD_TIME;
                } else {
                    kick_ouch(x, y, "");
                    return ECMD_TIME;
                }
            }
            kick_ouch(x, y, "");
            return ECMD_TIME;
        }
        if (IS_ALTAR(g.maploc->typ)) {
            if (Levitation) {
                kick_dumb(x, y);
                return ECMD_TIME;
            }
            You("kick %s.", (Blind ? something : "the altar"));
            altar_wrath(x, y);
            if (!rn2(3)) {
                kick_ouch(x, y, "");
                return ECMD_TIME;
            }
            exercise(A_DEX, TRUE);
            return ECMD_TIME;
        }
        if (IS_FOUNTAIN(g.maploc->typ)) {
            if (Levitation) {
                kick_dumb(x, y);
                return ECMD_TIME;
            }
            You("kick %s.", (Blind ? something : "the fountain"));
            if (!rn2(3)) {
                kick_ouch(x, y, "");
                return ECMD_TIME;
            }
            /* make metal boots rust */
            if (uarmf && rn2(3))
                if (water_damage(uarmf, "metal boots", TRUE) == ER_NOTHING) {
                    Your("boots get wet.");
                    /* could cause short-lived fumbling here */
                }
            exercise(A_DEX, TRUE);
            return ECMD_TIME;
        }
        if (IS_GRAVE(g.maploc->typ)) {
            if (Levitation) {
                kick_dumb(x, y);
                return ECMD_TIME;
            }
            if (rn2(4)) {
                kick_ouch(x, y, "");
                return ECMD_TIME;
            }
            exercise(A_WIS, FALSE);
            if (Role_if(PM_ARCHEOLOGIST) || Role_if(PM_SAMURAI)
                || ((u.ualign.type == A_LAWFUL) && (u.ualign.record > -10))) {
                adjalign(-sgn(u.ualign.type));
            }
            g.maploc->typ = ROOM;
            g.maploc->doormask = 0;
            (void) mksobj_at(ROCK, x, y, TRUE, FALSE);
            del_engr_at(x, y);
            if (Blind)
                pline("Crack!  %s broke!", Something);
            else {
                pline_The("headstone topples over and breaks!");
                newsym(x, y);
            }
            return ECMD_TIME;
        }
        if (g.maploc->typ == IRONBARS) {
            kick_ouch(x, y, "");
            return ECMD_TIME;
        }
        if (IS_TREE(g.maploc->typ)) {
            struct obj *treefruit;

            /* nothing, fruit or trouble? 75:23.5:1.5% */
            if (rn2(3)) {
                if (!rn2(6) && !(g.mvitals[PM_KILLER_BEE].mvflags & G_GONE))
                    You_hear("a low buzzing."); /* a warning */
                kick_ouch(x, y, "");
                return ECMD_TIME;
            }
            if (rn2(15) && !(g.maploc->looted & TREE_LOOTED)
                && (treefruit = rnd_treefruit_at(x, y))) {
                long nfruit = 8L - rnl(7), nfall;
                short frtype = treefruit->otyp;

                treefruit->quan = nfruit;
                treefruit->owt = weight(treefruit);
                if (is_plural(treefruit))
                    pline("Some %s fall from the tree!", xname(treefruit));
                else
                    pline("%s falls from the tree!", An(xname(treefruit)));
                nfall = scatter(x, y, 2, MAY_HIT, treefruit);
                if (nfall != nfruit) {
                    /* scatter left some in the tree, but treefruit
                     * may not refer to the correct object */
                    treefruit = mksobj(frtype, TRUE, FALSE);
                    treefruit->quan = nfruit - nfall;
                    pline("%ld %s got caught in the branches.",
                          nfruit - nfall, xname(treefruit));
                    dealloc_obj(treefruit);
                }
                exercise(A_DEX, TRUE);
                exercise(A_WIS, TRUE); /* discovered a new food source! */
                newsym(x, y);
                g.maploc->looted |= TREE_LOOTED;
                return ECMD_TIME;
            } else if (!(g.maploc->looted & TREE_SWARM)) {
                int cnt = rnl(4) + 2;
                int made = 0;
                coord mm;

                mm.x = x;
                mm.y = y;
                while (cnt--) {
                    if (enexto(&mm, mm.x, mm.y, &mons[PM_KILLER_BEE])
                        && makemon(&mons[PM_KILLER_BEE], mm.x, mm.y,
                                   MM_ANGRY|MM_NOMSG))
                        made++;
                }
                if (made)
                    pline("You've attracted the tree's former occupants!");
                else
                    You("smell stale honey.");
                g.maploc->looted |= TREE_SWARM;
                return ECMD_TIME;
            }
            kick_ouch(x, y, "");
            return ECMD_TIME;
        }
        if (IS_SINK(g.maploc->typ)) {
            int gend = poly_gender();

            if (Levitation) {
                kick_dumb(x, y);
                return ECMD_TIME;
            }
            if (rn2(5)) {
                if (!Deaf)
                    pline("Klunk!  The pipes vibrate noisily.");
                else
                    pline("Klunk!");
                exercise(A_DEX, TRUE);
                return ECMD_TIME;
            } else if (!(g.maploc->looted & S_LPUDDING) && !rn2(3)
                       && !(g.mvitals[PM_BLACK_PUDDING].mvflags & G_GONE)) {
                if (Blind)
                    You_hear("a gushing sound.");
                else
                    pline("A %s ooze gushes up from the drain!",
                          hcolor(NH_BLACK));
                (void) makemon(&mons[PM_BLACK_PUDDING], x, y, MM_NOMSG);
                exercise(A_DEX, TRUE);
                newsym(x, y);
                g.maploc->looted |= S_LPUDDING;
                return ECMD_TIME;
            } else if (!(g.maploc->looted & S_LDWASHER) && !rn2(3)
                       && !(g.mvitals[PM_AMOROUS_DEMON].mvflags & G_GONE)) {
                /* can't resist... */
                pline("%s returns!", (Blind ? Something : "The dish washer"));
                if (makemon(&mons[PM_AMOROUS_DEMON], x, y,
                            MM_NOMSG | ((gend == 1 || (gend == 2 && rn2(2)))
                                        ? MM_MALE : MM_FEMALE)))
                    newsym(x, y);
                g.maploc->looted |= S_LDWASHER;
                exercise(A_DEX, TRUE);
                return ECMD_TIME;
            } else if (!rn2(3)) {
                if (Blind && Deaf)
                    Sprintf(buf, " %s", body_part(FACE));
                else
                    buf[0] = '\0';
                pline("%s%s%s.", !Deaf ? "Flupp! " : "",
                      !Blind
                          ? "Muddy waste pops up from the drain"
                          : !Deaf
                              ? "You hear a sloshing sound"  /* Deaf-aware */
                              : "Something splashes you in the", buf);
                if (!(g.maploc->looted & S_LRING)) { /* once per sink */
                    if (!Blind)
                        You_see("a ring shining in its midst.");
                    (void) mkobj_at(RING_CLASS, x, y, TRUE);
                    newsym(x, y);
                    exercise(A_DEX, TRUE);
                    exercise(A_WIS, TRUE); /* a discovery! */
                    g.maploc->looted |= S_LRING;
                }
                return ECMD_TIME;
            }
            kick_ouch(x, y, "");
            return ECMD_TIME;
        }
        if (g.maploc->typ == STAIRS || g.maploc->typ == LADDER
            || IS_STWALL(g.maploc->typ)) {
            if (!IS_STWALL(g.maploc->typ) && g.maploc->ladder == LA_DOWN) {
                kick_dumb(x, y);
                return ECMD_TIME;
            }
            kick_ouch(x, y, "");
            return ECMD_TIME;
        }
        kick_dumb(x, y);
        return ECMD_TIME;
    }

    if (g.maploc->doormask == D_ISOPEN || g.maploc->doormask == D_BROKEN
        || g.maploc->doormask == D_NODOOR) {
        kick_dumb(x, y);
        return ECMD_TIME; /* uses a turn */
    }

    /* not enough leverage to kick open doors while levitating */
    if (Levitation) {
        kick_ouch(x, y, "");
        return ECMD_TIME;
    }

    exercise(A_DEX, TRUE);
    /* door is known to be CLOSED or LOCKED */
    if (rnl(35) < avrg_attrib + (!martial() ? 0 : ACURR(A_DEX))) {
        boolean shopdoor = *in_rooms(x, y, SHOPBASE) ? TRUE : FALSE;
        /* break the door */
        if (g.maploc->doormask & D_TRAPPED) {
            if (Verbose(0, dokick))
                You("kick the door.");
            exercise(A_STR, FALSE);
            g.maploc->doormask = D_NODOOR;
            b_trapped("door", FOOT);
        } else if (ACURR(A_STR) > 18 && !rn2(5) && !shopdoor) {
            pline("As you kick the door, it shatters to pieces!");
            exercise(A_STR, TRUE);
            g.maploc->doormask = D_NODOOR;
        } else {
            pline("As you kick the door, it crashes open!");
            exercise(A_STR, TRUE);
            g.maploc->doormask = D_BROKEN;
        }
        feel_newsym(x, y); /* we know we broke it */
        unblock_point(x, y); /* vision */
        if (shopdoor) {
            add_damage(x, y, SHOP_DOOR_COST);
            pay_for_damage("break", FALSE);
        }
        if (in_town(x, y))
            (void) get_iter_mons(watchman_thief_arrest);
    } else {
        if (Blind)
            feel_location(x, y); /* we know we hit it */
        exercise(A_STR, TRUE);
        /* note: this used to be unconditional "WHAMMM!!!" but that has a
           fairly strong connotation of noise that a deaf hero shouldn't
           hear; we've kept the extra 'm's and one of the extra '!'s */
        pline("%s!!", (Deaf || !rn2(3)) ? "Thwack" : "Whammm");
        if (in_town(x, y))
            (void) get_iter_mons_xy(watchman_door_damage, x, y);
    }
    return ECMD_TIME;
}

static void
drop_to(coord *cc, schar loc, coordxy x, coordxy y)
{
    stairway *stway = stairway_at(x, y);

    /* cover all the MIGR_xxx choices generated by down_gate() */
    switch (loc) {
    case MIGR_RANDOM: /* trap door or hole */
        if (Is_stronghold(&u.uz)) {
            cc->x = valley_level.dnum;
            cc->y = valley_level.dlevel;
            break;
        } else if (In_endgame(&u.uz) || Is_botlevel(&u.uz)) {
            cc->y = cc->x = 0;
            break;
        }
        /*FALLTHRU*/
    case MIGR_STAIRS_UP:
    case MIGR_LADDER_UP:
    case MIGR_SSTAIRS:
        if (stway) {
            cc->x = stway->tolev.dnum;
            cc->y = stway->tolev.dlevel;
        } else {
            cc->x = u.uz.dnum;
            cc->y = u.uz.dlevel + 1;
        }
        break;
    default:
    case MIGR_NOWHERE:
        /* y==0 means "nowhere", in which case x doesn't matter */
        cc->y = cc->x = 0;
        break;
    }
}

/* player or missile impacts location, causing objects to fall down */
void
impact_drop(
    struct obj *missile,  /* caused impact, won't drop itself */
    coordxy x, coordxy y, /* location affected */
    xint16 dlev)          /* if !0 send to dlev near player */
{
    schar toloc;
    register struct obj *obj, *obj2;
    register struct monst *shkp;
    long oct, dct, price, debit, robbed;
    boolean angry, costly, isrock;
    coord cc;

    if (!OBJ_AT(x, y))
        return;

    toloc = down_gate(x, y);
    drop_to(&cc, toloc, x, y);
    if (!cc.y)
        return;

    if (dlev) {
        /* send objects next to player falling through trap door.
         * checked in obj_delivery().
         */
        toloc = MIGR_WITH_HERO;
        cc.y = dlev;
    }

    costly = costly_spot(x, y);
    price = debit = robbed = 0L;
    angry = FALSE;
    shkp = (struct monst *) 0;
    /* if 'costly', we must keep a record of ESHK(shkp) before
     * it undergoes changes through the calls to stolen_value.
     * the angry bit must be reset, if needed, in this fn, since
     * stolen_value is called under the 'silent' flag to avoid
     * unsavory pline repetitions.
     */
    if (costly) {
        if ((shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 0) {
            debit = ESHK(shkp)->debit;
            robbed = ESHK(shkp)->robbed;
            angry = !shkp->mpeaceful;
        }
    }

    isrock = (missile && missile->otyp == ROCK);
    oct = dct = 0L;
    for (obj = g.level.objects[x][y]; obj; obj = obj2) {
        obj2 = obj->nexthere;
        if (obj == missile)
            continue;
        /* number of objects in the pile */
        oct += obj->quan;
        if (obj == uball || obj == uchain)
            continue;
        /* boulders can fall too, but rarely & never due to rocks */
        if ((isrock && obj->otyp == BOULDER)
            || rn2(obj->otyp == BOULDER ? 30 : 3))
            continue;
        obj_extract_self(obj);

        if (costly) {
            price += stolen_value(obj, x, y,
                                  (costly_spot(u.ux, u.uy)
                                   && index(u.urooms,
                                            *in_rooms(x, y, SHOPBASE))),
                                  TRUE);
            /* set obj->no_charge to 0 */
            if (Has_contents(obj))
                picked_container(obj); /* does the right thing */
            if (obj->oclass != COIN_CLASS)
                obj->no_charge = 0;
        }

        add_to_migration(obj);
        obj->ox = cc.x;
        obj->oy = cc.y;
        obj->owornmask = (long) toloc;

        /* number of fallen objects */
        dct += obj->quan;
    }

    if (dct && cansee(x, y)) { /* at least one object fell */
        const char *what = (dct == 1L ? "object falls" : "objects fall");

        if (missile)
            pline("From the impact, %sother %s.",
                  dct == oct ? "the " : dct == 1L ? "an" : "", what);
        else if (oct == dct)
            pline("%s adjacent %s %s.", dct == 1L ? "The" : "All the", what,
                  g.gate_str);
        else
            pline("%s adjacent %s %s.",
                  dct == 1L ? "One of the" : "Some of the",
                  dct == 1L ? "objects falls" : what, g.gate_str);
    }

    if (costly && shkp && price) {
        if (ESHK(shkp)->robbed > robbed) {
            You("removed %ld %s worth of goods!", price, currency(price));
            if (cansee(shkp->mx, shkp->my)) {
                if (ESHK(shkp)->customer[0] == 0)
                    (void) strncpy(ESHK(shkp)->customer, g.plname, PL_NSIZ);
                if (angry)
                    pline("%s is infuriated!", Shknam(shkp));
                else
                    pline("\"%s, you are a thief!\"", g.plname);
            } else
                You_hear("a scream, \"Thief!\"");
            hot_pursuit(shkp);
            (void) angry_guards(FALSE);
            return;
        }
        if (ESHK(shkp)->debit > debit) {
            long amt = (ESHK(shkp)->debit - debit);
            You("owe %s %ld %s for goods lost.", shkname(shkp), amt,
                currency(amt));
        }
    }
}

/* NOTE: ship_object assumes otmp was FREED from fobj or invent.
 * <x,y> is the point of drop.  otmp is _not_ an <x,y> resident:
 * otmp is either a kicked, dropped, or thrown object.
 */
boolean
ship_object(struct obj *otmp, coordxy x, coordxy y, boolean shop_floor_obj)
{
    schar toloc;
    coordxy ox, oy;
    coord cc;
    struct obj *obj;
    struct trap *t;
    boolean nodrop, unpaid, container, impact = FALSE, chainthere = FALSE;
    long n = 0L;

    if (!otmp)
        return FALSE;
    if ((toloc = down_gate(x, y)) == MIGR_NOWHERE)
        return FALSE;
    drop_to(&cc, toloc, x, y);
    if (!cc.y)
        return FALSE;

    /* objects other than attached iron ball always fall down ladder,
       but have a chance of staying otherwise */
    nodrop = (otmp == uball) || (otmp == uchain)
             || (toloc != MIGR_LADDER_UP && rn2(3));

    container = Has_contents(otmp);
    unpaid = is_unpaid(otmp);

    if (OBJ_AT(x, y)) {
        for (obj = g.level.objects[x][y]; obj; obj = obj->nexthere) {
            if (obj == uchain)
                chainthere = TRUE;
            else if (obj != otmp)
                n += obj->quan;
        }
        if (n)
            impact = TRUE;
    }
    /* boulders never fall through trap doors, but they might knock
       other things down before plugging the hole */
    if (otmp->otyp == BOULDER && ((t = t_at(x, y)) != 0)
        && is_hole(t->ttyp)) {
        if (impact)
            impact_drop(otmp, x, y, 0);
        return FALSE; /* let caller finish the drop */
    }

    if (cansee(x, y))
        otransit_msg(otmp, nodrop, chainthere, n);

    if (nodrop) {
        if (impact) {
            impact_drop(otmp, x, y, 0);
            maybe_unhide_at(x, y);
        }
        return FALSE;
    }

    if (unpaid || shop_floor_obj) {
        if (unpaid) {
            (void) stolen_value(otmp, u.ux, u.uy, TRUE, FALSE);
        } else {
            ox = otmp->ox;
            oy = otmp->oy;
            (void) stolen_value(
                otmp, ox, oy,
                (costly_spot(u.ux, u.uy)
                 && index(u.urooms, *in_rooms(ox, oy, SHOPBASE))),
                FALSE);
        }
        /* set otmp->no_charge to 0 */
        if (container)
            picked_container(otmp); /* happens to do the right thing */
        if (otmp->oclass != COIN_CLASS)
            otmp->no_charge = 0;
    }

    if (otmp->owornmask)
        remove_worn_item(otmp, TRUE);

    /* some things break rather than ship */
    if (breaktest(otmp)) {
        const char *result;

        if (objects[otmp->otyp].oc_material == GLASS
            || otmp->otyp == EXPENSIVE_CAMERA) {
            if (otmp->otyp == MIRROR)
                change_luck(-2);
            result = "crash";
        } else {
            /* penalty for breaking eggs laid by you */
            if (otmp->otyp == EGG && otmp->spe && otmp->corpsenm >= LOW_PM)
                change_luck((schar) -min(otmp->quan, 5L));
            result = "splat";
        }
        You_hear("a muffled %s.", result);
        obj_extract_self(otmp);
        obfree(otmp, (struct obj *) 0);
        return TRUE;
    }

    add_to_migration(otmp);
    otmp->ox = cc.x;
    otmp->oy = cc.y;
    otmp->owornmask = (long) toloc;

    /* boulder from rolling boulder trap, no longer part of the trap */
    if (otmp->otyp == BOULDER)
        otmp->otrapped = 0;

    if (impact) {
        /* the objs impacted may be in a shop other than
         * the one in which the hero is located.  another
         * check for a shk is made in impact_drop.  it is, e.g.,
         * possible to kick/throw an object belonging to one
         * shop into another shop through a gap in the wall,
         * and cause objects belonging to the other shop to
         * fall down a trap door--thereby getting two shopkeepers
         * angry at the hero in one shot.
         */
        impact_drop(otmp, x, y, 0);
        newsym(x, y);
    }
    return TRUE;
}

void
obj_delivery(boolean near_hero)
{
    register struct obj *otmp, *otmp2;
    int nx = 0, ny = 0;
    int where;
    boolean nobreak, noscatter;
    stairway *stway;
    d_level fromdlev;
    boolean isladder;

    for (otmp = g.migrating_objs; otmp; otmp = otmp2) {
        otmp2 = otmp->nobj;
        if (otmp->ox != u.uz.dnum || otmp->oy != u.uz.dlevel)
            continue;

        where = (int) (otmp->owornmask & 0x7fffL); /* destination code */
        if ((where & MIGR_TO_SPECIES) != 0)
            continue;

        nobreak = (where & MIGR_NOBREAK) != 0;
        noscatter = (where & MIGR_WITH_HERO) != 0;
        where &= ~(MIGR_NOBREAK | MIGR_NOSCATTER);

        if (!near_hero ^ (where == MIGR_WITH_HERO))
            continue;

        obj_extract_self(otmp);
        otmp->owornmask = 0L;
        fromdlev.dnum = otmp->omigr_from_dnum;
        fromdlev.dlevel = otmp->omigr_from_dlevel;

        isladder = FALSE;

        switch (where) {
        case MIGR_LADDER_UP:
            isladder = TRUE;
            /*FALLTHRU*/
        case MIGR_STAIRS_UP:
        case MIGR_SSTAIRS:
            if ((stway = stairway_find_from(&fromdlev, isladder)) != 0) {
                nx = stway->sx;
                ny = stway->sy;
            }
            break;
        case MIGR_WITH_HERO:
            nx = u.ux, ny = u.uy;
            break;
        default:
        case MIGR_RANDOM:
            nx = ny = 0;
            break;
        }
        otmp->omigr_from_dnum = 0;
        otmp->omigr_from_dlevel = 0;
        if (nx > 0) {
            place_object(otmp, nx, ny);
            if (!nobreak && !IS_SOFT(levl[nx][ny].typ)) {
                if (where == MIGR_WITH_HERO) {
                    if (breaks(otmp, nx, ny))
                        continue;
                } else if (breaktest(otmp)) {
                    /* assume it broke before player arrived, no messages */
                    delobj(otmp);
                    continue;
                }
            }
            stackobj(otmp);
            if (!noscatter)
                (void) scatter(nx, ny, rnd(2), 0, otmp);
            else
                newsym(nx, ny);
        } else { /* random location */
            /* set dummy coordinates because there's no
               current position for rloco() to update */
            otmp->ox = otmp->oy = 0;
            if (rloco(otmp) && !nobreak && breaktest(otmp)) {
                /* assume it broke before player arrived, no messages */
                delobj(otmp);
            }
        }
    }
}

void
deliver_obj_to_mon(struct monst *mtmp, int cnt, unsigned long deliverflags)
{
    struct obj *otmp, *otmp2;
    int where, maxobj = 1;
    boolean at_crime_scene = In_mines(&u.uz);

    if ((deliverflags & DF_RANDOM) && cnt > 1)
        maxobj = rnd(cnt);
    else if (deliverflags & DF_ALL)
        maxobj = 0;
    else
        maxobj = 1;

#define DELIVER_PM (M2_UNDEAD | M2_WERE | M2_HUMAN | M2_ELF | M2_DWARF \
                    | M2_GNOME | M2_ORC | M2_DEMON | M2_GIANT)

    cnt = 0;
    for (otmp = g.migrating_objs; otmp; otmp = otmp2) {
        otmp2 = otmp->nobj;
        where = (int) (otmp->owornmask & 0x7fffL); /* destination code */
        if ((where & MIGR_TO_SPECIES) == 0)
            continue;

        if (otmp->migr_species != NON_PM
            && ((mtmp->data->mflags2 & DELIVER_PM)
                == (unsigned) otmp->migr_species)) {
            obj_extract_self(otmp);
            otmp->owornmask = 0L;
            otmp->ox = otmp->oy = 0;

            /* special treatment for orcs and their kind */
            if ((otmp->corpsenm & M2_ORC) != 0 && has_oname(otmp)) {
                if (!has_mgivenname(mtmp)) {
                    if (at_crime_scene || !rn2(2))
                        mtmp = christen_orc(mtmp,
                                            at_crime_scene ? ONAME(otmp)
                                                           : (char *) 0,
                                            /* bought the stolen goods */
                                            " the Fence");
                }
                free_oname(otmp);
            }
            otmp->migr_species = NON_PM;
            otmp->omigr_from_dnum = 0;
            otmp->omigr_from_dlevel = 0;
            (void) add_to_minv(mtmp, otmp);
            cnt++;
            if (maxobj && cnt >= maxobj)
                break;
            /* getting here implies DF_ALL */
        }
    }
}

static void
otransit_msg(register struct obj *otmp, boolean nodrop, boolean chainthere, long num)
{
    char *optr = 0, obuf[BUFSZ], xbuf[BUFSZ];

    if (otmp->otyp == CORPSE) {
        /* Tobjnam() calls xname() and would yield "The corpse";
           we want more specific "The newt corpse" or "Medusa's corpse" */
        optr = upstart(corpse_xname(otmp, (char *) 0, CXN_PFX_THE));
    } else {
        optr = Tobjnam(otmp, (char *) 0);
    }
    Strcpy(obuf, optr);

    if (num || chainthere) {
        /* As of 3.6.2: use a separate buffer for the suffix to avoid risk of
           overrunning obuf[] (let pline() handle truncation if necessary) */
        if (num) { /* means: other objects are impacted */
            Sprintf(xbuf, " %s %s object%s", otense(otmp, "hit"),
                    (num == 1L) ? "another" : "other", (num > 1L) ? "s" : "");
        } else { /* chain-only msg */
            Sprintf(xbuf, " %s your chain", otense(otmp, "rattle"));
        }
        if (nodrop)
            Sprintf(eos(xbuf), ".");
        else
            Sprintf(eos(xbuf), " and %s %s.",
                    otense(otmp, "fall"), g.gate_str);
        pline("%s%s", obuf, xbuf);
    } else if (!nodrop)
        pline("%s %s %s.", obuf, otense(otmp, "fall"), g.gate_str);
}

/* migration destination for objects which fall down to next level */
schar
down_gate(coordxy x, coordxy y)
{
    struct trap *ttmp;
    stairway *stway = stairway_at(x, y);

    g.gate_str = 0;
    /* this matches the player restriction in goto_level() */
    if (on_level(&u.uz, &qstart_level) && !ok_to_quest()) {
        return MIGR_NOWHERE;
    }
    if (stway && !stway->up && !stway->isladder) {
        g.gate_str = "down the stairs";
        return (stway->tolev.dnum == u.uz.dnum) ? MIGR_STAIRS_UP
                                                : MIGR_SSTAIRS;
    }
    if (stway && !stway->up && stway->isladder) {
        g.gate_str = "down the ladder";
        return MIGR_LADDER_UP;
    }
    /* hole will always be flagged as seen; trap drop might or might not */
    if ((ttmp = t_at(x, y)) != 0 && ttmp->tseen && is_hole(ttmp->ttyp)) {
        g.gate_str = (ttmp->ttyp == TRAPDOOR) ? "through the trap door"
                                              : "through the hole";
        return MIGR_RANDOM;
    }
    return MIGR_NOWHERE;
}

/*dokick.c*/

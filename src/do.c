/* NetHack 3.6	do.c	$NHDT-Date: 1582155879 2020/02/19 23:44:39 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.228 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

/* Contains code for 'd', 'D' (drop), '>', '<' (up, down) */

#include "hack.h"

static void FDECL(trycall, (struct obj *));
static void NDECL(polymorph_sink);
static boolean NDECL(teleport_sink);
static void FDECL(dosinkring, (struct obj *));
static int FDECL(drop, (struct obj *));
static int NDECL(wipeoff);
static int FDECL(menu_drop, (int));
static NHFILE *NDECL(currentlevel_rewrite);
static void NDECL(final_level);
/* static boolean FDECL(badspot, (XCHAR_P,XCHAR_P)); */

static NEARDATA const char drop_types[] = { ALLOW_COUNT, COIN_CLASS,
                                            ALL_CLASSES, 0 };

/* 'd' command: drop one inventory item */
int
dodrop()
{
    int result, i = (g.invent) ? 0 : (SIZE(drop_types) - 1);

    if (*u.ushops)
        sellobj_state(SELL_DELIBERATE);
    result = drop(getobj(&drop_types[i], "drop"));
    if (*u.ushops)
        sellobj_state(SELL_NORMAL);
    if (result)
        reset_occupations();

    return result;
}

/* Called when a boulder is dropped, thrown, or pushed.  If it ends up
 * in a pool, it either fills the pool up or sinks away.  In either case,
 * it's gone for good...  If the destination is not a pool, returns FALSE.
 */
boolean
boulder_hits_pool(otmp, rx, ry, pushing)
struct obj *otmp;
register int rx, ry;
boolean pushing;
{
    if (!otmp || otmp->otyp != BOULDER) {
        impossible("Not a boulder?");
    } else if (!Is_waterlevel(&u.uz) && is_pool_or_lava(rx, ry)) {
        boolean lava = is_lava(rx, ry), fills_up;
        const char *what = waterbody_name(rx, ry);
        schar ltyp = levl[rx][ry].typ;
        int chance = rn2(10); /* water: 90%; lava: 10% */
        fills_up = lava ? chance == 0 : chance != 0;

        if (fills_up) {
            struct trap *ttmp = t_at(rx, ry);

            if (ltyp == DRAWBRIDGE_UP) {
                levl[rx][ry].drawbridgemask &= ~DB_UNDER; /* clear lava */
                levl[rx][ry].drawbridgemask |= DB_FLOOR;
            } else
                levl[rx][ry].typ = ROOM, levl[rx][ry].flags = 0;

            if (ttmp)
                (void) delfloortrap(ttmp);
            bury_objs(rx, ry);

            newsym(rx, ry);
            if (pushing) {
                char whobuf[BUFSZ];

                Strcpy(whobuf, "you");
                if (u.usteed)
                    Strcpy(whobuf, y_monnam(u.usteed));
                pline("%s %s %s into the %s.", upstart(whobuf),
                      vtense(whobuf, "push"), the(xname(otmp)), what);
                if (flags.verbose && !Blind)
                    pline("Now you can cross it!");
                /* no splashing in this case */
            }
        }
        if (!fills_up || !pushing) { /* splashing occurs */
            if (!u.uinwater) {
                if (pushing ? !Blind : cansee(rx, ry)) {
                    There("is a large splash as %s %s the %s.",
                          the(xname(otmp)), fills_up ? "fills" : "falls into",
                          what);
                } else if (!Deaf)
                    You_hear("a%s splash.", lava ? " sizzling" : "");
                wake_nearto(rx, ry, 40);
            }

            if (fills_up && u.uinwater && distu(rx, ry) == 0) {
                u.uinwater = 0;
                docrt();
                g.vision_full_recalc = 1;
                You("find yourself on dry land again!");
            } else if (lava && distu(rx, ry) <= 2) {
                int dmg;
                You("are hit by molten %s%c",
                    hliquid("lava"), Fire_resistance ? '.' : '!');
                burn_away_slime();
                dmg = d((Fire_resistance ? 1 : 3), 6);
                losehp(Maybe_Half_Phys(dmg), /* lava damage */
                       "molten lava", KILLED_BY);
            } else if (!fills_up && flags.verbose
                       && (pushing ? !Blind : cansee(rx, ry)))
                pline("It sinks without a trace!");
        }

        /* boulder is now gone */
        if (pushing)
            delobj(otmp);
        else
            obfree(otmp, (struct obj *) 0);
        return TRUE;
    }
    return FALSE;
}

/* Used for objects which sometimes do special things when dropped; must be
 * called with the object not in any chain.  Returns TRUE if the object goes
 * away.
 */
boolean
flooreffects(obj, x, y, verb)
struct obj *obj;
int x, y;
const char *verb;
{
    struct trap *t;
    struct monst *mtmp;
    struct obj *otmp;
    boolean tseen;
    int ttyp = NO_TRAP;

    if (obj->where != OBJ_FREE)
        panic("flooreffects: obj not free");

    /* make sure things like water_damage() have no pointers to follow */
    obj->nobj = obj->nexthere = (struct obj *) 0;

    if (obj->otyp == BOULDER && boulder_hits_pool(obj, x, y, FALSE)) {
        return TRUE;
    } else if (obj->otyp == BOULDER && (t = t_at(x, y)) != 0
               && (is_pit(t->ttyp) || is_hole(t->ttyp))) {
        ttyp = t->ttyp;
        tseen = t->tseen ? TRUE : FALSE;
        if (((mtmp = m_at(x, y)) && mtmp->mtrapped)
            || (u.utrap && u.ux == x && u.uy == y)) {
            if (*verb && (cansee(x, y) || distu(x, y) == 0))
                pline("%s boulder %s into the pit%s.",
                      Blind ? "A" : "The",
                      vtense((const char *) 0, verb),
                      mtmp ? "" : " with you");
            if (mtmp) {
                if (!passes_walls(mtmp->data) && !throws_rocks(mtmp->data)) {
                    /* dieroll was rnd(20); 1: maximum chance to hit
                       since trapped target is a sitting duck */
                    int damage, dieroll = 1;

                    /* As of 3.6.2: this was calling hmon() unconditionally
                       so always credited/blamed the hero but the boulder
                       might have been thrown by a giant or launched by
                       a rolling boulder trap triggered by a monster or
                       dropped by a scroll of earth read by a monster */
                    if (g.context.mon_moving) {
                        /* normally we'd use ohitmon() but it can call
                           drop_throw() which calls flooreffects() */
                        damage = dmgval(obj, mtmp);
                        mtmp->mhp -= damage;
                        if (DEADMONSTER(mtmp)) {
                            if (canspotmon(mtmp))
                                pline("%s is %s!", Monnam(mtmp),
                                      (nonliving(mtmp->data)
                                       || is_vampshifter(mtmp))
                                      ? "destroyed" : "killed");
                            mondied(mtmp);
                        }
                    } else {
                        (void) hmon(mtmp, obj, HMON_THROWN, dieroll);
                    }
                    if (!DEADMONSTER(mtmp) && !is_whirly(mtmp->data))
                        return FALSE; /* still alive */
                }
                mtmp->mtrapped = 0;
            } else {
                if (!Passes_walls && !throws_rocks(g.youmonst.data)) {
                    losehp(Maybe_Half_Phys(rnd(15)),
                           "squished under a boulder", NO_KILLER_PREFIX);
                    return FALSE; /* player remains trapped */
                } else
                    reset_utrap(TRUE);
            }
        }
        if (*verb) {
            if (Blind && (x == u.ux) && (y == u.uy)) {
                You_hear("a CRASH! beneath you.");
            } else if (!Blind && cansee(x, y)) {
                pline_The("boulder %s%s.",
                          (ttyp == TRAPDOOR && !tseen)
                              ? "triggers and " : "",
                          (ttyp == TRAPDOOR)
                              ? "plugs a trap door"
                              : (ttyp == HOLE) ? "plugs a hole"
                                               : "fills a pit");
            } else {
                You_hear("a boulder %s.", verb);
            }
        }
        /*
         * Note:  trap might have gone away via ((hmon -> killed -> xkilled)
         *  || mondied) -> mondead -> m_detach -> fill_pit.
         */
        if ((t = t_at(x, y)) != 0)
            deltrap(t);
        useupf(obj, 1L);
        bury_objs(x, y);
        newsym(x, y);
        return TRUE;
    } else if (is_lava(x, y)) {
        return lava_damage(obj, x, y);
    } else if (is_pool(x, y)) {
        /* Reasonably bulky objects (arbitrary) splash when dropped.
         * If you're floating above the water even small things make
         * noise.  Stuff dropped near fountains always misses */
        if ((Blind || (Levitation || Flying)) && !Deaf
            && ((x == u.ux) && (y == u.uy))) {
            if (!Underwater) {
                if (weight(obj) > 9) {
                    pline("Splash!");
                } else if (Levitation || Flying) {
                    pline("Plop!");
                }
            }
            map_background(x, y, 0);
            newsym(x, y);
        }
        return water_damage(obj, NULL, FALSE) == ER_DESTROYED;
    } else if (u.ux == x && u.uy == y && (t = t_at(x, y)) != 0
               && (uteetering_at_seen_pit(t) || uescaped_shaft(t))) {
        if (Blind && !Deaf)
            You_hear("%s tumble downwards.", the(xname(obj)));
        else
            pline("%s %s into %s %s.", The(xname(obj)),
                  otense(obj, "tumble"), the_your[t->madeby_u],
                  is_pit(t->ttyp) ? "pit" : "hole");
    } else if (obj->globby) {
        /* Globby things like puddings might stick together */
        while (obj && (otmp = obj_nexto_xy(obj, x, y, TRUE)) != 0) {
            pudding_merge_message(obj, otmp);
            /* intentionally not getting the melded object; obj_meld may set
             * obj to null. */
            (void) obj_meld(&obj, &otmp);
        }
        return (boolean) !obj;
    }
    return FALSE;
}

/* obj is an object dropped on an altar */
void
doaltarobj(obj)
register struct obj *obj;
{
    if (Blind)
        return;

    if (obj->oclass != COIN_CLASS) {
        /* KMH, conduct */
        u.uconduct.gnostic++;
    } else {
        /* coins don't have bless/curse status */
        obj->blessed = obj->cursed = 0;
    }

    if (obj->blessed || obj->cursed) {
        There("is %s flash as %s %s the altar.",
              an(hcolor(obj->blessed ? NH_AMBER : NH_BLACK)), doname(obj),
              otense(obj, "hit"));
        if (!Hallucination)
            obj->bknown = 1; /* ok to bypass set_bknown() */
    } else {
        pline("%s %s on the altar.", Doname2(obj), otense(obj, "land"));
        if (obj->oclass != COIN_CLASS)
            obj->bknown = 1; /* ok to bypass set_bknown() */
    }
}

static void
trycall(obj)
register struct obj *obj;
{
    if (!objects[obj->otyp].oc_name_known && !objects[obj->otyp].oc_uname)
        docall(obj);
}

/* Transforms the sink at the player's position into
   a fountain, throne, altar or grave. */
static void
polymorph_sink()
{
    uchar sym = S_sink;
    boolean sinklooted;
    int algn;

    if (levl[u.ux][u.uy].typ != SINK)
        return;

    sinklooted = levl[u.ux][u.uy].looted != 0;
    g.level.flags.nsinks--;
    levl[u.ux][u.uy].doormask = 0; /* levl[][].flags */
    switch (rn2(4)) {
    default:
    case 0:
        sym = S_fountain;
        levl[u.ux][u.uy].typ = FOUNTAIN;
        levl[u.ux][u.uy].blessedftn = 0;
        if (sinklooted)
            SET_FOUNTAIN_LOOTED(u.ux, u.uy);
        g.level.flags.nfountains++;
        break;
    case 1:
        sym = S_throne;
        levl[u.ux][u.uy].typ = THRONE;
        if (sinklooted)
            levl[u.ux][u.uy].looted = T_LOOTED;
        break;
    case 2:
        sym = S_altar;
        levl[u.ux][u.uy].typ = ALTAR;
        /* 3.6.3: this used to pass 'rn2(A_LAWFUL + 2) - 1' to
           Align2amask() but that evaluates its argument more than once */
        algn = rn2(3) - 1; /* -1 (A_Cha) or 0 (A_Neu) or +1 (A_Law) */
        levl[u.ux][u.uy].altarmask = ((Inhell && rn2(3)) ? AM_NONE
                                      : Align2amask(algn));
        break;
    case 3:
        sym = S_room;
        levl[u.ux][u.uy].typ = ROOM;
        make_grave(u.ux, u.uy, (char *) 0);
        if (levl[u.ux][u.uy].typ == GRAVE)
            sym = S_grave;
        break;
    }
    /* give message even if blind; we know we're not levitating,
       so can feel the outcome even if we can't directly see it */
    if (levl[u.ux][u.uy].typ != ROOM)
        pline_The("sink transforms into %s!", an(defsyms[sym].explanation));
    else
        pline_The("sink vanishes.");
    newsym(u.ux, u.uy);
}

/* Teleports the sink at the player's position;
   return True if sink teleported. */
static boolean
teleport_sink()
{
    int cx, cy;
    int cnt = 0;
    struct trap *trp;
    struct engr *eng;

    do {
        cx = rnd(COLNO - 1);
        cy = rn2(ROWNO);
        trp = t_at(cx, cy);
        eng = engr_at(cx, cy);
    } while ((levl[cx][cy].typ != ROOM || trp || eng || cansee(cx, cy))
             && cnt++ < 200);

    if (levl[cx][cy].typ == ROOM && !trp && !eng) {
        /* create sink at new position */
        levl[cx][cy].typ = SINK;
        levl[cx][cy].looted = levl[u.ux][u.uy].looted;
        newsym(cx, cy);
        /* remove old sink */
        levl[u.ux][u.uy].typ = ROOM;
        levl[u.ux][u.uy].looted = 0;
        newsym(u.ux, u.uy);
        return TRUE;
    }
    return FALSE;
}

/* obj is a ring being dropped over a kitchen sink */
static void
dosinkring(obj)
register struct obj *obj;
{
    struct obj *otmp, *otmp2;
    boolean ideed = TRUE;
    boolean nosink = FALSE;

    You("drop %s down the drain.", doname(obj));
    obj->in_use = TRUE;  /* block free identification via interrupt */
    switch (obj->otyp) { /* effects that can be noticed without eyes */
    case RIN_SEARCHING:
        You("thought %s got lost in the sink, but there it is!", yname(obj));
        goto giveback;
    case RIN_SLOW_DIGESTION:
        pline_The("ring is regurgitated!");
 giveback:
        obj->in_use = FALSE;
        dropx(obj);
        trycall(obj);
        return;
    case RIN_LEVITATION:
        pline_The("sink quivers upward for a moment.");
        break;
    case RIN_POISON_RESISTANCE:
        You("smell rotten %s.", makeplural(fruitname(FALSE)));
        break;
    case RIN_AGGRAVATE_MONSTER:
        pline("Several %s buzz angrily around the sink.",
              Hallucination ? makeplural(rndmonnam(NULL)) : "flies");
        break;
    case RIN_SHOCK_RESISTANCE:
        pline("Static electricity surrounds the sink.");
        break;
    case RIN_CONFLICT:
        You_hear("loud noises coming from the drain.");
        break;
    case RIN_SUSTAIN_ABILITY: /* KMH */
        pline_The("%s flow seems fixed.", hliquid("water"));
        break;
    case RIN_GAIN_STRENGTH:
        pline_The("%s flow seems %ser now.",
                  hliquid("water"),
                  (obj->spe < 0) ? "weak" : "strong");
        break;
    case RIN_GAIN_CONSTITUTION:
        pline_The("%s flow seems %ser now.",
                  hliquid("water"),
                  (obj->spe < 0) ? "less" : "great");
        break;
    case RIN_INCREASE_ACCURACY: /* KMH */
        pline_The("%s flow %s the drain.",
                  hliquid("water"),
                  (obj->spe < 0) ? "misses" : "hits");
        break;
    case RIN_INCREASE_DAMAGE:
        pline_The("water's force seems %ser now.",
                  (obj->spe < 0) ? "small" : "great");
        break;
    case RIN_HUNGER:
        ideed = FALSE;
        for (otmp = g.level.objects[u.ux][u.uy]; otmp; otmp = otmp2) {
            otmp2 = otmp->nexthere;
            if (otmp != uball && otmp != uchain
                && !obj_resists(otmp, 1, 99)) {
                if (!Blind) {
                    pline("Suddenly, %s %s from the sink!", doname(otmp),
                          otense(otmp, "vanish"));
                    ideed = TRUE;
                }
                delobj(otmp);
            }
        }
        break;
    case MEAT_RING:
        /* Not the same as aggravate monster; besides, it's obvious. */
        pline("Several flies buzz around the sink.");
        break;
    case RIN_TELEPORTATION:
        nosink = teleport_sink();
        /* give message even if blind; we know we're not levitating,
           so can feel the outcome even if we can't directly see it */
        pline_The("sink %svanishes.", nosink ? "" : "momentarily ");
        ideed = FALSE;
        break;
    case RIN_POLYMORPH:
        polymorph_sink();
        nosink = TRUE;
        /* for S_room case, same message as for teleportation is given */
        ideed = (levl[u.ux][u.uy].typ != ROOM);
        break;
    default:
        ideed = FALSE;
        break;
    }
    if (!Blind && !ideed) {
        ideed = TRUE;
        switch (obj->otyp) { /* effects that need eyes */
        case RIN_ADORNMENT:
            pline_The("faucets flash brightly for a moment.");
            break;
        case RIN_REGENERATION:
            pline_The("sink looks as good as new.");
            break;
        case RIN_INVISIBILITY:
            You("don't see anything happen to the sink.");
            break;
        case RIN_FREE_ACTION:
            You_see("the ring slide right down the drain!");
            break;
        case RIN_SEE_INVISIBLE:
            You_see("some %s in the sink.",
                    Hallucination ? "oxygen molecules" : "air");
            break;
        case RIN_STEALTH:
            pline_The("sink seems to blend into the floor for a moment.");
            break;
        case RIN_FIRE_RESISTANCE:
            pline_The("hot %s faucet flashes brightly for a moment.",
                      hliquid("water"));
            break;
        case RIN_COLD_RESISTANCE:
            pline_The("cold %s faucet flashes brightly for a moment.",
                      hliquid("water"));
            break;
        case RIN_PROTECTION_FROM_SHAPE_CHAN:
            pline_The("sink looks nothing like a fountain.");
            break;
        case RIN_PROTECTION:
            pline_The("sink glows %s for a moment.",
                      hcolor((obj->spe < 0) ? NH_BLACK : NH_SILVER));
            break;
        case RIN_WARNING:
            pline_The("sink glows %s for a moment.", hcolor(NH_WHITE));
            break;
        case RIN_TELEPORT_CONTROL:
            pline_The("sink looks like it is being beamed aboard somewhere.");
            break;
        case RIN_POLYMORPH_CONTROL:
            pline_The(
                  "sink momentarily looks like a regularly erupting geyser.");
            break;
        default:
            break;
        }
    }
    if (ideed)
        trycall(obj);
    else if (!nosink)
        You_hear("the ring bouncing down the drainpipe.");

    if (!rn2(20) && !nosink) {
        pline_The("sink backs up, leaving %s.", doname(obj));
        obj->in_use = FALSE;
        dropx(obj);
    } else if (!rn2(5)) {
        freeinv(obj);
        obj->in_use = FALSE;
        obj->ox = u.ux;
        obj->oy = u.uy;
        add_to_buried(obj);
    } else
        useup(obj);
}

/* some common tests when trying to drop or throw items */
boolean
canletgo(obj, word)
struct obj *obj;
const char *word;
{
    if (obj->owornmask & (W_ARMOR | W_ACCESSORY)) {
        if (*word)
            Norep("You cannot %s %s you are wearing.", word, something);
        return FALSE;
    }
    if (obj->otyp == LOADSTONE && obj->cursed) {
        /* getobj() kludge sets corpsenm to user's specified count
           when refusing to split a stack of cursed loadstones */
        if (*word) {
            /* getobj() ignores a count for throwing since that is
               implicitly forced to be 1; replicate its kludge... */
            if (!strcmp(word, "throw") && obj->quan > 1L)
                obj->corpsenm = 1;
            pline("For some reason, you cannot %s%s the stone%s!", word,
                  obj->corpsenm ? " any of" : "", plur(obj->quan));
        }
        obj->corpsenm = 0; /* reset */
        set_bknown(obj, 1);
        return FALSE;
    }
    if (obj->otyp == LEASH && obj->leashmon != 0) {
        if (*word)
            pline_The("leash is tied around your %s.", body_part(HAND));
        return FALSE;
    }
    if (obj->owornmask & W_SADDLE) {
        if (*word)
            You("cannot %s %s you are sitting on.", word, something);
        return FALSE;
    }
    return TRUE;
}

static int
drop(obj)
register struct obj *obj;
{
    if (!obj)
        return 0;
    if (!canletgo(obj, "drop"))
        return 0;
    if (obj == uwep) {
        if (welded(uwep)) {
            weldmsg(obj);
            return 0;
        }
        setuwep((struct obj *) 0);
    }
    if (obj == uquiver) {
        setuqwep((struct obj *) 0);
    }
    if (obj == uswapwep) {
        setuswapwep((struct obj *) 0);
    }

    if (u.uswallow) {
        /* barrier between you and the floor */
        if (flags.verbose) {
            char *onam_p, monbuf[BUFSZ];

            /* doname can call s_suffix, reusing its buffer */
            Strcpy(monbuf, s_suffix(mon_nam(u.ustuck)));
            onam_p = is_unpaid(obj) ? yobjnam(obj, (char *) 0) : doname(obj);
            You("drop %s into %s %s.", onam_p, monbuf,
                mbodypart(u.ustuck, STOMACH));
        }
    } else {
        if ((obj->oclass == RING_CLASS || obj->otyp == MEAT_RING)
            && IS_SINK(levl[u.ux][u.uy].typ)) {
            dosinkring(obj);
            return 1;
        }
        if (!can_reach_floor(TRUE)) {
            /* we might be levitating due to #invoke Heart of Ahriman;
               if so, levitation would end during call to freeinv()
               and we want hitfloor() to happen before float_down() */
            boolean levhack = finesse_ahriman(obj);

            if (levhack)
                ELevitation = W_ART; /* other than W_ARTI */
            if (flags.verbose)
                You("drop %s.", doname(obj));
            /* Ensure update when we drop gold objects */
            if (obj->oclass == COIN_CLASS)
                g.context.botl = 1;
            freeinv(obj);
            hitfloor(obj, TRUE);
            if (levhack)
                float_down(I_SPECIAL | TIMEOUT, W_ARTI | W_ART);
            return 1;
        }
        if (!IS_ALTAR(levl[u.ux][u.uy].typ) && flags.verbose)
            You("drop %s.", doname(obj));
    }
    dropx(obj);
    return 1;
}

/* dropx - take dropped item out of inventory;
   called in several places - may produce output
   (eg ship_object() and dropy() -> sellobj() both produce output) */
void
dropx(obj)
register struct obj *obj;
{
    /* Ensure update when we drop gold objects */
    if (obj->oclass == COIN_CLASS)
        g.context.botl = 1;
    freeinv(obj);
    if (!u.uswallow) {
        if (ship_object(obj, u.ux, u.uy, FALSE))
            return;
        if (IS_ALTAR(levl[u.ux][u.uy].typ))
            doaltarobj(obj); /* set bknown */
    }
    dropy(obj);
}

/* dropy - put dropped object at destination; called from lots of places */
void
dropy(obj)
struct obj *obj;
{
    dropz(obj, FALSE);
}

/* dropz - really put dropped object at its destination... */
void
dropz(obj, with_impact)
struct obj *obj;
boolean with_impact;
{
    if (obj == uwep)
        setuwep((struct obj *) 0);
    if (obj == uquiver)
        setuqwep((struct obj *) 0);
    if (obj == uswapwep)
        setuswapwep((struct obj *) 0);

    if (!u.uswallow && flooreffects(obj, u.ux, u.uy, "drop"))
        return;
    /* uswallow check done by GAN 01/29/87 */
    if (u.uswallow) {
        boolean could_petrify = FALSE;
        boolean could_poly = FALSE;
        boolean could_slime = FALSE;
        boolean could_grow = FALSE;
        boolean could_heal = FALSE;

        if (obj != uball) { /* mon doesn't pick up ball */
            if (obj->otyp == CORPSE) {
                could_petrify = touch_petrifies(&mons[obj->corpsenm]);
                could_poly = polyfodder(obj);
                could_slime = (obj->corpsenm == PM_GREEN_SLIME);
                could_grow = (obj->corpsenm == PM_WRAITH);
                could_heal = (obj->corpsenm == PM_NURSE);
            }
            if (is_unpaid(obj))
                (void) stolen_value(obj, u.ux, u.uy, TRUE, FALSE);
            (void) mpickobj(u.ustuck, obj);
            if (is_animal(u.ustuck->data)) {
                if (could_poly || could_slime) {
                    (void) newcham(u.ustuck,
                                   could_poly ? (struct permonst *) 0
                                              : &mons[PM_GREEN_SLIME],
                                   FALSE, could_slime);
                    delobj(obj); /* corpse is digested */
                } else if (could_petrify) {
                    minstapetrify(u.ustuck, TRUE);
                    /* Don't leave a cockatrice corpse in a statue */
                    if (!u.uswallow)
                        delobj(obj);
                } else if (could_grow) {
                    (void) grow_up(u.ustuck, (struct monst *) 0);
                    delobj(obj); /* corpse is digested */
                } else if (could_heal) {
                    u.ustuck->mhp = u.ustuck->mhpmax;
                    delobj(obj); /* corpse is digested */
                }
            }
        }
    } else {
        place_object(obj, u.ux, u.uy);
        if (with_impact)
            container_impact_dmg(obj, u.ux, u.uy);
        if (obj == uball)
            drop_ball(u.ux, u.uy);
        else if (g.level.flags.has_shop)
            sellobj(obj, u.ux, u.uy);
        stackobj(obj);
        if (Blind && Levitation)
            map_object(obj, 0);
        newsym(u.ux, u.uy); /* remap location under self */
    }
}

/* things that must change when not held; recurse into containers.
   Called for both player and monsters */
void
obj_no_longer_held(obj)
struct obj *obj;
{
    if (!obj) {
        return;
    } else if (Has_contents(obj)) {
        struct obj *contents;

        for (contents = obj->cobj; contents; contents = contents->nobj)
            obj_no_longer_held(contents);
    }
    switch (obj->otyp) {
    case CRYSKNIFE:
        /* Normal crysknife reverts to worm tooth when not held by hero
         * or monster; fixed crysknife has only 10% chance of reverting.
         * When a stack of the latter is involved, it could be worthwhile
         * to give each individual crysknife its own separate 10% chance,
         * but we aren't in any position to handle stack splitting here.
         */
        if (!obj->oerodeproof || !rn2(10)) {
            /* if monsters aren't moving, assume player is responsible */
            if (!g.context.mon_moving && !g.program_state.gameover)
                costly_alteration(obj, COST_DEGRD);
            obj->otyp = WORM_TOOTH;
            obj->oerodeproof = 0;
        }
        break;
    }
}

/* 'D' command: drop several things */
int
doddrop()
{
    int result = 0;

    if (!g.invent) {
        You("have nothing to drop.");
        return 0;
    }
    add_valid_menu_class(0); /* clear any classes already there */
    if (*u.ushops)
        sellobj_state(SELL_DELIBERATE);
    if (flags.menu_style != MENU_TRADITIONAL
        || (result = ggetobj("drop", drop, 0, FALSE, (unsigned *) 0)) < -1)
        result = menu_drop(result);
    if (*u.ushops)
        sellobj_state(SELL_NORMAL);
    if (result)
        reset_occupations();

    return result;
}

/* Drop things from the hero's inventory, using a menu. */
static int
menu_drop(retry)
int retry;
{
    int n, i, n_dropped = 0;
    long cnt;
    struct obj *otmp, *otmp2;
    menu_item *pick_list;
    boolean all_categories = TRUE;
    boolean drop_everything = FALSE;

    if (retry) {
        all_categories = (retry == -2);
    } else if (flags.menu_style == MENU_FULL) {
        all_categories = FALSE;
        n = query_category("Drop what type of items?", g.invent,
                           UNPAID_TYPES | ALL_TYPES | CHOOSE_ALL | BUC_BLESSED
                               | BUC_CURSED | BUC_UNCURSED | BUC_UNKNOWN,
                           &pick_list, PICK_ANY);
        if (!n)
            goto drop_done;
        for (i = 0; i < n; i++) {
            if (pick_list[i].item.a_int == ALL_TYPES_SELECTED)
                all_categories = TRUE;
            else if (pick_list[i].item.a_int == 'A')
                drop_everything = TRUE;
            else
                add_valid_menu_class(pick_list[i].item.a_int);
        }
        free((genericptr_t) pick_list);
    } else if (flags.menu_style == MENU_COMBINATION) {
        unsigned ggoresults = 0;

        all_categories = FALSE;
        /* Gather valid classes via traditional NetHack method */
        i = ggetobj("drop", drop, 0, TRUE, &ggoresults);
        if (i == -2)
            all_categories = TRUE;
        if (ggoresults & ALL_FINISHED) {
            n_dropped = i;
            goto drop_done;
        }
    }

    if (drop_everything) {
        /*
         * Dropping a burning potion of oil while levitating can cause
         * an explosion which might destroy some of hero's inventory,
         * so the old code
         *      for (otmp = g.invent; otmp; otmp = otmp2) {
         *          otmp2 = otmp->nobj;
         *          n_dropped += drop(otmp);
         *      }
         * was unreliable and could lead to an "object lost" panic.
         *
         * Use the bypass bit to mark items already processed (hence
         * not droppable) and rescan inventory until no unbypassed
         * items remain.
         */
        bypass_objlist(g.invent, FALSE); /* clear bypass bit for invent */
        while ((otmp = nxt_unbypassed_obj(g.invent)) != 0)
            n_dropped += drop(otmp);
        /* we might not have dropped everything (worn armor, welded weapon,
           cursed loadstones), so reset any remaining inventory to normal */
        bypass_objlist(g.invent, FALSE);
    } else {
        /* should coordinate with perm invent, maybe not show worn items */
        n = query_objlist("What would you like to drop?", &g.invent,
                          (USE_INVLET | INVORDER_SORT), &pick_list, PICK_ANY,
                          all_categories ? allow_all : allow_category);
        if (n > 0) {
            /*
             * picklist[] contains a set of pointers into inventory, but
             * as soon as something gets dropped, they might become stale
             * (see the drop_everything code above for an explanation).
             * Just checking to see whether one is still in the g.invent
             * chain is not sufficient validation since destroyed items
             * will be freed and items we've split here might have already
             * reused that memory and put the same pointer value back into
             * g.invent.  Ditto for using invlet to validate.  So we start
             * by setting bypass on all of g.invent, then check each pointer
             * to verify that it is in g.invent and has that bit set.
             */
            bypass_objlist(g.invent, TRUE);
            for (i = 0; i < n; i++) {
                otmp = pick_list[i].item.a_obj;
                for (otmp2 = g.invent; otmp2; otmp2 = otmp2->nobj)
                    if (otmp2 == otmp)
                        break;
                if (!otmp2 || !otmp2->bypass)
                    continue;
                /* found next selected invent item */
                cnt = pick_list[i].count;
                if (cnt < otmp->quan) {
                    if (welded(otmp)) {
                        ; /* don't split */
                    } else if (otmp->otyp == LOADSTONE && otmp->cursed) {
                        /* same kludge as getobj(), for canletgo()'s use */
                        otmp->corpsenm = (int) cnt; /* don't split */
                    } else {
                        otmp = splitobj(otmp, cnt);
                    }
                }
                n_dropped += drop(otmp);
            }
            bypass_objlist(g.invent, FALSE); /* reset g.invent to normal */
            free((genericptr_t) pick_list);
        }
    }

 drop_done:
    return n_dropped;
}

/* the '>' command */
int
dodown()
{
    struct trap *trap = 0;
    boolean stairs_down = ((u.ux == xdnstair && u.uy == ydnstair)
                           || (u.ux == g.sstairs.sx && u.uy == g.sstairs.sy
                               && !g.sstairs.up)),
            ladder_down = (u.ux == xdnladder && u.uy == ydnladder);

    if (u_rooted())
        return 1;

    if (stucksteed(TRUE)) {
        return 0;
    }
    /* Levitation might be blocked, but player can still use '>' to
       turn off controlled levitation */
    if (HLevitation || ELevitation) {
        if ((HLevitation & I_SPECIAL) || (ELevitation & W_ARTI)) {
            /* end controlled levitation */
            if (ELevitation & W_ARTI) {
                struct obj *obj;

                for (obj = g.invent; obj; obj = obj->nobj) {
                    if (obj->oartifact
                        && artifact_has_invprop(obj, LEVITATION)) {
                        if (obj->age < g.monstermoves)
                            obj->age = g.monstermoves;
                        obj->age += rnz(100);
                    }
                }
            }
            if (float_down(I_SPECIAL | TIMEOUT, W_ARTI)) {
                return 1; /* came down, so moved */
            } else if (!HLevitation && !ELevitation) {
                Your("latent levitation ceases.");
                return 1; /* did something, effectively moved */
            }
        }
        if (BLevitation) {
            ; /* weren't actually floating after all */
        } else if (Blind) {
            /* Avoid alerting player to an unknown stair or ladder.
             * Changes the message for a covered, known staircase
             * too; staircase knowledge is not stored anywhere.
             */
            if (stairs_down)
                stairs_down =
                    (glyph_to_cmap(levl[u.ux][u.uy].glyph) == S_dnstair);
            else if (ladder_down)
                ladder_down =
                    (glyph_to_cmap(levl[u.ux][u.uy].glyph) == S_dnladder);
        }
        if (Is_airlevel(&u.uz))
            You("are floating in the %s.", surface(u.ux, u.uy));
        else if (Is_waterlevel(&u.uz))
            You("are floating in %s.",
                is_pool(u.ux, u.uy) ? "the water" : "a bubble of air");
        else
            floating_above(stairs_down ? "stairs" : ladder_down
                                                    ? "ladder"
                                                    : surface(u.ux, u.uy));
        return 0; /* didn't move */
    }

    if (Upolyd && ceiling_hider(&mons[u.umonnum]) && u.uundetected) {
        u.uundetected = 0;
        if (Flying) { /* lurker above */
            You("fly out of hiding.");
        } else { /* piercer */
            You("drop to the %s.", surface(u.ux, u.uy));
            if (is_pool_or_lava(u.ux, u.uy)) {
                pooleffects(FALSE);
            } else {
                (void) pickup(1);
                if ((trap = t_at(u.ux, u.uy)) != 0)
                    dotrap(trap, TOOKPLUNGE);
            }
        }
        return 1; /* came out of hiding; might need '>' again to go down */
    }

    if (!stairs_down && !ladder_down) {
        trap = t_at(u.ux, u.uy);
        if (trap && (uteetering_at_seen_pit(trap) || uescaped_shaft(trap))) {
            dotrap(trap, TOOKPLUNGE);
            return 1;
        } else if (!trap || !is_hole(trap->ttyp)
                   || !Can_fall_thru(&u.uz) || !trap->tseen) {
            if (flags.autodig && !g.context.nopick && uwep && is_pick(uwep)) {
                return use_pick_axe2(uwep);
            } else {
                You_cant("go down here.");
                return 0;
            }
        }
    }
    if (u.ustuck) {
        You("are %s, and cannot go down.",
            !u.uswallow ? "being held" : is_animal(u.ustuck->data)
                                             ? "swallowed"
                                             : "engulfed");
        return 1;
    }
    if (on_level(&valley_level, &u.uz) && !u.uevent.gehennom_entered) {
        You("are standing at the gate to Gehennom.");
        pline("Unspeakable cruelty and harm lurk down there.");
        if (yn("Are you sure you want to enter?") != 'y')
            return 0;
        pline("So be it.");
        u.uevent.gehennom_entered = 1; /* don't ask again */
    }

    if (!next_to_u()) {
        You("are held back by your pet!");
        return 0;
    }

    if (trap) {
        const char *down_or_thru = trap->ttyp == HOLE ? "down" : "through";
        const char *actn = Flying ? "fly"
                                  : locomotion(g.youmonst.data, "jump");

        if (g.youmonst.data->msize >= MZ_HUGE) {
            char qbuf[QBUFSZ];

            You("don't fit %s easily.", down_or_thru);
            Sprintf(qbuf, "Try to squeeze %s?", down_or_thru);
            if (yn(qbuf) == 'y') {
                if (!rn2(3)) {
                    actn = "manage to squeeze";
                    losehp(Maybe_Half_Phys(rnd(4)),
                           "contusion from a small passage", KILLED_BY);
                } else {
                    You("were unable to fit %s.", down_or_thru);
                    return 0;
                }
            } else {
                return 0;
            }
        }
        You("%s %s the %s.", actn, down_or_thru,
            trap->ttyp == HOLE ? "hole" : "trap door");
    }
    if (trap && Is_stronghold(&u.uz)) {
        goto_hell(FALSE, TRUE);
    } else {
        g.at_ladder = (boolean) (levl[u.ux][u.uy].typ == LADDER);
        next_level(!trap);
        g.at_ladder = FALSE;
    }
    return 1;
}

/* the '<' command */
int
doup()
{
    if (u_rooted())
        return 1;

    /* "up" to get out of a pit... */
    if (u.utrap && u.utraptype == TT_PIT) {
        climb_pit();
        return 1;
    }

    if ((u.ux != xupstair || u.uy != yupstair)
        && (!xupladder || u.ux != xupladder || u.uy != yupladder)
        && (!g.sstairs.sx || u.ux != g.sstairs.sx || u.uy != g.sstairs.sy
            || !g.sstairs.up)) {
        You_cant("go up here.");
        return 0;
    }
    if (stucksteed(TRUE)) {
        return 0;
    }
    if (u.ustuck) {
        You("are %s, and cannot go up.",
            !u.uswallow ? "being held" : is_animal(u.ustuck->data)
                                             ? "swallowed"
                                             : "engulfed");
        return 1;
    }
    if (near_capacity() > SLT_ENCUMBER) {
        /* No levitation check; inv_weight() already allows for it */
        Your("load is too heavy to climb the %s.",
             levl[u.ux][u.uy].typ == STAIRS ? "stairs" : "ladder");
        return 1;
    }
    if (ledger_no(&u.uz) == 1) {
        if (iflags.debug_fuzzer)
            return 0;
        if (yn("Beware, there will be no return!  Still climb?") != 'y')
            return 0;
    }
    if (!next_to_u()) {
        You("are held back by your pet!");
        return 0;
    }
    g.at_ladder = (boolean) (levl[u.ux][u.uy].typ == LADDER);
    prev_level(TRUE);
    g.at_ladder = FALSE;
    return 1;
}

/* check that we can write out the current level */
static NHFILE *
currentlevel_rewrite()
{
    NHFILE *nhfp;
    char whynot[BUFSZ];
#ifdef MFLOPPY
    int savemode;
#endif

    /* since level change might be a bit slow, flush any buffered screen
     *  output (like "you fall through a trap door") */
    mark_synch();

    nhfp = create_levelfile(ledger_no(&u.uz), whynot);
    if (!nhfp) {
        /*
         * This is not quite impossible: e.g., we may have
         * exceeded our quota. If that is the case then we
         * cannot leave this level, and cannot save either.
         * Another possibility is that the directory was not
         * writable.
         */
        pline1(whynot);
        return (NHFILE *) 0;
    }

#ifdef MFLOPPY
    savemode = nhfp->mode;
    nhfp->mode = COUNTING;
    if (!savelev(nhfp, ledger_no(&u.uz))) {
        close_nhfile(nhfp);
        delete_levelfile(ledger_no(&u.uz));
        pline("NetHack is out of disk space for making levels!");
        You("can save, quit, or continue playing.");
        return -1;
    }
    nhfp->mode = savemode;
#endif
    return nhfp;
}

#ifdef INSURANCE
void
save_currentstate()
{
    NHFILE *nhfp;

    if (flags.ins_chkpt) {
        /* write out just-attained level, with pets and everything */
        nhfp = currentlevel_rewrite();
        if (!nhfp)
            return;
        if (nhfp->structlevel)
            bufon(nhfp->fd);
        nhfp->mode = WRITING;
        savelev(nhfp,ledger_no(&u.uz));
        close_nhfile(nhfp);
    }

    /* write out non-level state */
    savestateinlock();
}
#endif

/*
static boolean
badspot(x, y)
register xchar x, y;
{
    return (boolean) ((levl[x][y].typ != ROOM
                       && levl[x][y].typ != AIR
                       && levl[x][y].typ != CORR)
                      || MON_AT(x, y));
}
*/

/* when arriving on a level, if hero and a monster are trying to share same
   spot, move one; extracted from goto_level(); also used by wiz_makemap() */
void
u_collide_m(mtmp)
struct monst *mtmp;
{
    coord cc;

    if (!mtmp || mtmp == u.usteed || mtmp != m_at(u.ux, u.uy)) {
        impossible("level arrival collision: %s?",
                   !mtmp ? "no monster"
                     : (mtmp == u.usteed) ? "steed is on map"
                       : "monster not co-located");
        return;
    }

    /* There's a monster at your target destination; it might be one
       which accompanied you--see mon_arrive(dogmove.c)--or perhaps
       it was already here.  Randomly move you to an adjacent spot
       or else the monster to any nearby location.  Prior to 3.3.0
       the latter was done unconditionally. */
    if (!rn2(2) && enexto(&cc, u.ux, u.uy, g.youmonst.data)
        && distu(cc.x, cc.y) <= 2)
        u_on_newpos(cc.x, cc.y); /*[maybe give message here?]*/
    else
        mnexto(mtmp);

    if ((mtmp = m_at(u.ux, u.uy)) != 0) {
        /* there was an unconditional impossible("mnexto failed")
           here, but it's not impossible and we're prepared to cope
           with the situation, so only say something when debugging */
        if (wizard)
            pline("(monster in hero's way)");
        if (!rloc(mtmp, TRUE) || (mtmp = m_at(u.ux, u.uy)) != 0)
            /* no room to move it; send it away, to return later */
            m_into_limbo(mtmp);
    }
}

void
goto_level(newlevel, at_stairs, falling, portal)
d_level *newlevel;
boolean at_stairs, falling, portal;
{
    int l_idx, save_mode;
    NHFILE *nhfp;
    xchar new_ledger;
    boolean cant_go_back, great_effort,
            up = (depth(newlevel) < depth(&u.uz)),
            newdungeon = (u.uz.dnum != newlevel->dnum),
            was_in_W_tower = In_W_tower(u.ux, u.uy, &u.uz),
            familiar = FALSE,
            new = FALSE; /* made a new level? */
    struct monst *mtmp;
    char whynot[BUFSZ];
    char *annotation;

    if (dunlev(newlevel) > dunlevs_in_dungeon(newlevel))
        newlevel->dlevel = dunlevs_in_dungeon(newlevel);
    if (newdungeon && In_endgame(newlevel)) { /* 1st Endgame Level !!! */
        if (!u.uhave.amulet)
            return;  /* must have the Amulet */
        if (!wizard) /* wizard ^V can bypass Earth level */
            assign_level(newlevel, &earth_level); /* (redundant) */
    }
    new_ledger = ledger_no(newlevel);
    if (new_ledger <= 0)
        done(ESCAPED); /* in fact < 0 is impossible */

    /* If you have the amulet and are trying to get out of Gehennom,
     * going up a set of stairs sometimes does some very strange things!
     * Biased against law and towards chaos.  (The chance to be sent
     * down multiple levels when attempting to go up are significantly
     * less than the corresponding comment in older versions indicated
     * due to overlooking the effect of the call to assign_rnd_lvl().)
     *
     * Odds for making it to the next level up, or of being sent down:
     *  "up"    L      N      C
     *   +1   75.0   75.0   75.0
     *    0    6.25   8.33  12.5
     *   -1   11.46  12.50  12.5
     *   -2    5.21   4.17   0.0
     *   -3    2.08   0.0    0.0
     *
     * 3.7.0: the chance for the "mysterious force" to kick in goes down
     * as it kicks in, starting at 25% per climb attempt and dropping off
     * gradually but substantially.  The drop off is greater when hero is
     * sent down farther so benefits lawfuls more than chaotics this time.
     */
    if (Inhell && up && u.uhave.amulet && !newdungeon && !portal
        && (dunlev(&u.uz) < dunlevs_in_dungeon(&u.uz) - 3)) {
        if (!rn2(4 + g.context.mysteryforce)) {
            int odds = 3 + (int) u.ualign.type,   /* 2..4 */
                diff = (odds <= 1) ? 0 : rn2(odds); /* paranoia */

            if (diff != 0) {
                assign_rnd_level(newlevel, &u.uz, diff);
                /* assign_rnd_level() may have used a value less than diff */
                diff = newlevel->dlevel - u.uz.dlevel; /* actual descent */
                /* if inside the tower, stay inside */
                if (was_in_W_tower && !On_W_tower_level(newlevel))
                    diff = 0;
            }
            if (diff == 0)
                assign_level(newlevel, &u.uz);

            pline("A mysterious force momentarily surrounds you...");
            /* each time it kicks in, the chance of doing so again may drop;
               that drops faster, on average, when being sent down farther so
               while the impact is reduced for everybody compared to earlier
               versions, it is reduced least for chaotics, most for lawfuls */
            g.context.mysteryforce += rn2(diff + 2); /* L:0-4, N:0-3, C:0-2 */

            if (on_level(newlevel, &u.uz)) {
                (void) safe_teleds(FALSE);
                (void) next_to_u();
                return;
            }
            new_ledger = ledger_no(newlevel);
            at_stairs = g.at_ladder = FALSE;
        }
    }

    /* Prevent the player from going past the first quest level unless
     * (s)he has been given the go-ahead by the leader.
     */
    if (on_level(&u.uz, &qstart_level) && !newdungeon && !ok_to_quest()) {
        pline("A mysterious force prevents you from descending.");
        return;
    }

    if (on_level(newlevel, &u.uz))
        return; /* this can happen */

    /* tethered movement makes level change while trapped feasible */
    if (u.utrap && u.utraptype == TT_BURIEDBALL)
        buried_ball_to_punishment(); /* (before we save/leave old level) */

    nhfp = currentlevel_rewrite();
    if (!nhfp)
        return;

    /* discard context which applies to the level we're leaving;
       for lock-picking, container may be carried, in which case we
       keep context; if on the floor, it's about to be saved+freed and
       maybe_reset_pick() needs to do its carried() check before that */
    maybe_reset_pick((struct obj *) 0);
    reset_trapset(); /* even if to-be-armed trap obj is accompanying hero */
    iflags.travelcc.x = iflags.travelcc.y = 0; /* travel destination cache */
    g.context.polearm.hitmon = (struct monst *) 0; /* polearm target */
    /* digging context is level-aware and can actually be resumed if
       hero returns to the previous level without any intervening dig */

    if (falling) /* assuming this is only trap door or hole */
        impact_drop((struct obj *) 0, u.ux, u.uy, newlevel->dlevel);

    check_special_room(TRUE); /* probably was a trap door */
    if (Punished)
        unplacebc();
    reset_utrap(FALSE); /* needed in level_tele */
    fill_pit(u.ux, u.uy);
    set_ustuck((struct monst *) 0); /* idem */
    u.uswallow = u.uswldtim = 0;
    u.uinwater = 0;
    u.uundetected = 0; /* not hidden, even if means are available */
    keepdogs(FALSE);
    recalc_mapseen(); /* recalculate map overview before we leave the level */
    /*
     *  We no longer see anything on the level.  Make sure that this
     *  follows u.uswallow set to null since uswallow overrides all
     *  normal vision.
     */
    vision_recalc(2);

    /*
     * Save the level we're leaving.  If we're entering the endgame,
     * we can get rid of all existing levels because they cannot be
     * reached any more.  We still need to use savelev()'s cleanup
     * for the level being left, to recover dynamic memory in use and
     * to avoid dangling timers and light sources.
     */
    cant_go_back = (newdungeon && In_endgame(newlevel));
    if (!cant_go_back) {
        update_mlstmv(); /* current monsters are becoming inactive */
        if (nhfp->structlevel)
            bufon(nhfp->fd);       /* use buffered output */
    }
    save_mode = nhfp->mode;
    nhfp->mode = cant_go_back ? FREEING : (WRITING | FREEING);
    savelev(nhfp, ledger_no(&u.uz));
    nhfp->mode = save_mode;
    /* air bubbles and clouds are saved in game-state rather than with the
       level they're used on; in normal play, you can't leave and return
       to any endgame level--bubbles aren't needed once you move to the
       next level so used to be discarded when the next special level was
       loaded; but in wizard mode you can leave and return, and since they
       aren't saved with the level and restored upon return (new ones are
       created instead), we need to discard them to avoid a memory leak;
       so bubbles are now discarded as we leave the level they're used on */
    if (Is_waterlevel(&u.uz) || Is_airlevel(&u.uz)) {
        NHFILE tmpnhfp;

        zero_nhfile(&tmpnhfp);
        tmpnhfp.fd = -1;
        tmpnhfp.mode = FREEING;
        save_waterlevel(&tmpnhfp);
    }
    close_nhfile(nhfp);
    if (cant_go_back) {
        /* discard unreachable levels; keep #0 */
        for (l_idx = maxledgerno(); l_idx > 0; --l_idx)
            delete_levelfile(l_idx);
        /* mark #overview data for all dungeon branches as uninteresting */
        for (l_idx = 0; l_idx < g.n_dgns; ++l_idx)
            remdun_mapseen(l_idx);
    }

    if (Is_rogue_level(newlevel) || Is_rogue_level(&u.uz))
        assign_graphics(Is_rogue_level(newlevel) ? ROGUESET : PRIMARY);
#ifdef USE_TILES
    substitute_tiles(newlevel);
#endif
    check_gold_symbol();
    /* record this level transition as a potential seen branch unless using
     * some non-standard means of transportation (level teleport).
     */
    if ((at_stairs || falling || portal) && (u.uz.dnum != newlevel->dnum))
        recbranch_mapseen(&u.uz, newlevel);
    assign_level(&u.uz0, &u.uz);
    assign_level(&u.uz, newlevel);
    assign_level(&u.utolev, newlevel);
    u.utotype = 0;
    if (!builds_up(&u.uz)) { /* usual case */
        if (dunlev(&u.uz) > dunlev_reached(&u.uz))
            dunlev_reached(&u.uz) = dunlev(&u.uz);
    } else {
        if (dunlev_reached(&u.uz) == 0
            || dunlev(&u.uz) < dunlev_reached(&u.uz))
            dunlev_reached(&u.uz) = dunlev(&u.uz);
    }

    /* set default level change destination areas */
    /* the special level code may override these */
    (void) memset((genericptr_t) &g.updest, 0, sizeof g.updest);
    (void) memset((genericptr_t) &g.dndest, 0, sizeof g.dndest);

    if (!(g.level_info[new_ledger].flags & LFILE_EXISTS)) {
        /* entering this level for first time; make it now */
        if (g.level_info[new_ledger].flags & (FORGOTTEN | VISITED)) {
            impossible("goto_level: returning to discarded level?");
            g.level_info[new_ledger].flags &= ~(FORGOTTEN | VISITED);
        }
        mklev();
        new = TRUE; /* made the level */
    } else {
        /* returning to previously visited level; reload it */
        nhfp = open_levelfile(new_ledger, whynot);
        if (tricked_fileremoved(nhfp, whynot)) {
            /* we'll reach here if running in wizard mode */
            error("Cannot continue this game.");
        }
        reseed_random(rn2);
        reseed_random(rn2_on_display_rng);
        minit(); /* ZEROCOMP */
        getlev(nhfp, g.hackpid, new_ledger);
        /* when in wizard mode, it is possible to leave from and return to
           any level in the endgame; above, we discarded bubble/cloud info
           when leaving Plane of Water or Air so recreate some now */
        if (Is_waterlevel(&u.uz) || Is_airlevel(&u.uz)) {
            NHFILE tmpnhfp;

            zero_nhfile(&tmpnhfp);
            tmpnhfp.fd = -1;
            restore_waterlevel(&tmpnhfp);
        }
        close_nhfile(nhfp);
        oinit(); /* reassign level dependent obj probabilities */
    }
    reglyph_darkroom();
    u.uinwater = 0;
    /* do this prior to level-change pline messages */
    vision_reset();         /* clear old level's line-of-sight */
    g.vision_full_recalc = 0; /* don't let that reenable vision yet */
    flush_screen(-1);       /* ensure all map flushes are postponed */

    if (portal && !In_endgame(&u.uz)) {
        /* find the portal on the new level */
        register struct trap *ttrap;

        for (ttrap = g.ftrap; ttrap; ttrap = ttrap->ntrap)
            if (ttrap->ttyp == MAGIC_PORTAL)
                break;

        if (!ttrap)
            panic("goto_level: no corresponding portal!");
        seetrap(ttrap);
        u_on_newpos(ttrap->tx, ttrap->ty);
    } else if (at_stairs && !In_endgame(&u.uz)) {
        if (up) {
            if (g.at_ladder)
                u_on_newpos(xdnladder, ydnladder);
            else if (newdungeon)
                u_on_sstairs(1);
            else
                u_on_dnstairs();
            /* you climb up the {stairs|ladder};
               fly up the stairs; fly up along the ladder */
            great_effort = (Punished && !Levitation);
            if (flags.verbose || great_effort)
                pline("%s %s up%s the %s.",
                      great_effort ? "With great effort, you" : "You",
                      Levitation ? "float" : Flying ? "fly" : "climb",
                      (Flying && g.at_ladder) ? " along" : "",
                      g.at_ladder ? "ladder" : "stairs");
        } else { /* down */
            if (g.at_ladder)
                u_on_newpos(xupladder, yupladder);
            else if (newdungeon)
                u_on_sstairs(0);
            else
                u_on_upstairs();
            if (!u.dz) {
                ; /* stayed on same level? (no transit effects) */
            } else if (Flying) {
                if (flags.verbose)
                    You("fly down %s.",
                        g.at_ladder ? "along the ladder" : "the stairs");
            } else if (near_capacity() > UNENCUMBERED
                       || Punished || Fumbling) {
                You("fall down the %s.", g.at_ladder ? "ladder" : "stairs");
                if (Punished) {
                    drag_down();
                    ballrelease(FALSE);
                }
                /* falling off steed has its own losehp() call */
                if (u.usteed)
                    dismount_steed(DISMOUNT_FELL);
                else
                    losehp(Maybe_Half_Phys(rnd(3)),
                           g.at_ladder ? "falling off a ladder"
                                     : "tumbling down a flight of stairs",
                           KILLED_BY);
                selftouch("Falling, you");
            } else { /* ordinary descent */
                if (flags.verbose)
                    You("%s.", g.at_ladder ? "climb down the ladder"
                                         : "descend the stairs");
            }
        }
    } else { /* trap door or level_tele or In_endgame */
        u_on_rndspot((up ? 1 : 0) | (was_in_W_tower ? 2 : 0));
        if (falling) {
            if (Punished)
                ballfall();
            selftouch("Falling, you");
        }
    }

    if (Punished)
        placebc();
    obj_delivery(FALSE);
    losedogs();
    kill_genocided_monsters(); /* for those wiped out while in limbo */
    /*
     * Expire all timers that have gone off while away.  Must be
     * after migrating monsters and objects are delivered
     * (losedogs and obj_delivery).
     */
    run_timers();

    /* hero might be arriving at a spot containing a monster;
       if so, move one or the other to another location */
    if ((mtmp = m_at(u.ux, u.uy)) != 0)
        u_collide_m(mtmp);

    initrack();

    /* initial movement of bubbles just before vision_recalc */
    if (Is_waterlevel(&u.uz) || Is_airlevel(&u.uz))
        movebubbles();
    else if (Is_firelevel(&u.uz))
        fumaroles();

    if (g.level_info[new_ledger].flags & FORGOTTEN) {
        forget_map(ALL_MAP); /* forget the map */
        forget_traps();      /* forget all traps too */
        familiar = TRUE;
        g.level_info[new_ledger].flags &= ~FORGOTTEN;
    }

    /* Reset the screen. */
    vision_reset(); /* reset the blockages */
    g.glyphmap_perlevel_flags = 0L; /* force per-level mapglyph() changes */
    docrt();        /* does a full vision recalc */
    flush_screen(-1);

    /*
     *  Move all plines beyond the screen reset.
     */

    /* deferred arrival message for level teleport looks odd if given
       after the various messages below so give it before them */
    if (g.dfr_post_msg && !strncmpi(g.dfr_post_msg, "You materialize", 15)) {
        pline("%s", g.dfr_post_msg);
        free((genericptr_t) g.dfr_post_msg), g.dfr_post_msg = 0;
    }

    /* special levels can have a custom arrival message */
    deliver_splev_message();

    /* Check whether we just entered Gehennom. */
    if (!In_hell(&u.uz0) && Inhell) {
        if (Is_valley(&u.uz)) {
            You("arrive at the Valley of the Dead...");
            pline_The("odor of burnt flesh and decay pervades the air.");
#ifdef MICRO
            display_nhwindow(WIN_MESSAGE, FALSE);
#endif
            You_hear("groans and moans everywhere.");
        } else
            pline("It is hot here.  You smell smoke...");

        record_achievement(ACH_HELL); /* reached Gehennom */
    }
    /* in case we've managed to bypass the Valley's stairway down */
    if (Inhell && !Is_valley(&u.uz))
        u.uevent.gehennom_entered = 1;

    if (familiar) {
        static const char *const fam_msgs[4] = {
            "You have a sense of deja vu.",
            "You feel like you've been here before.",
            "This place %s familiar...", 0 /* no message */
        };
        static const char *const halu_fam_msgs[4] = {
            "Whoa!  Everything %s different.",
            "You are surrounded by twisty little passages, all alike.",
            "Gee, this %s like uncle Conan's place...", 0 /* no message */
        };
        const char *mesg;
        char buf[BUFSZ];
        int which = rn2(4);

        if (Hallucination)
            mesg = halu_fam_msgs[which];
        else
            mesg = fam_msgs[which];
        if (mesg && index(mesg, '%')) {
            Sprintf(buf, mesg, !Blind ? "looks" : "seems");
            mesg = buf;
        }
        if (mesg)
            pline1(mesg);
    }

    /* special location arrival messages/events */
    if (In_endgame(&u.uz)) {
        if (newdungeon)
            record_achievement(ACH_ENDG); /* reached endgame */
        if (new && on_level(&u.uz, &astral_level)) {
            final_level(); /* guardian angel,&c */
            record_achievement(ACH_ASTR); /* reached Astral level */
        } else if (newdungeon && u.uhave.amulet) {
            resurrect(); /* force confrontation with Wizard */
        }
    } else if (In_quest(&u.uz)) {
        onquest(); /* might be reaching locate|goal level */
    } else if (In_V_tower(&u.uz)) {
        if (newdungeon && In_hell(&u.uz0))
            pline_The("heat and smoke are gone.");
    } else if (Is_knox(&u.uz)) {
        /* alarm stops working once Croesus has died */
        if (new || !g.mvitals[PM_CROESUS].died) {
            You("have penetrated a high security area!");
            pline("An alarm sounds!");
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                mtmp->msleeping = 0;
            }
        }
    } else if (In_mines(&u.uz)) {
        if (newdungeon)
            record_achievement(ACH_MINE);
    } else if (In_sokoban(&u.uz)) {
        if (newdungeon)
            record_achievement(ACH_SOKO);
    } else {
        if (new && Is_rogue_level(&u.uz)) {
            You("enter what seems to be an older, more primitive world.");
        } else if (new && Is_bigroom(&u.uz)) {
            record_achievement(ACH_BGRM);
        }
        /* main dungeon message from your quest leader */
        if (!In_quest(&u.uz0) && at_dgn_entrance("The Quest")
            && !(u.uevent.qcompleted || u.uevent.qexpelled
                 || g.quest_status.leader_is_dead)) {
            if (!u.uevent.qcalled) {
                u.uevent.qcalled = 1;
                /* main "leader needs help" message */
                com_pager("quest_portal");
            } else { /* reminder message */
                com_pager(Role_if(PM_ROGUE) ? "quest_portal_demand"
                                            : "quest_portal_again");
            }
        }
    }

    assign_level(&u.uz0, &u.uz); /* reset u.uz0 */
#ifdef INSURANCE
    save_currentstate();
#endif

    if ((annotation = get_annotation(&u.uz)) != 0)
        You("remember this level as %s.", annotation);

    /* give room entrance message, if any */
    check_special_room(FALSE);
    /* deliver objects traveling with player */
    obj_delivery(TRUE);

    /* assume this will always return TRUE when changing level */
    (void) in_out_region(u.ux, u.uy);
    (void) pickup(1);
}

static void
final_level()
{
    struct monst *mtmp;

    /* reset monster hostility relative to player */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        reset_hostility(mtmp);
    }

    /* create some player-monsters */
    create_mplayers(rn1(4, 3), TRUE);

    /* create a guardian angel next to player, if worthy */
    gain_guardian_angel();
}

/* change levels at the end of this turn, after monsters finish moving */
void
schedule_goto(tolev, at_stairs, falling, portal_flag, pre_msg, post_msg)
d_level *tolev;
boolean at_stairs, falling;
int portal_flag;
const char *pre_msg, *post_msg;
{
    int typmask = 0100; /* non-zero triggers `deferred_goto' */

    /* destination flags (`goto_level' args) */
    if (at_stairs)
        typmask |= 1;
    if (falling)
        typmask |= 2;
    if (portal_flag)
        typmask |= 4;
    if (portal_flag < 0)
        typmask |= 0200; /* flag for portal removal */
    u.utotype = typmask;
    /* destination level */
    assign_level(&u.utolev, tolev);

    if (pre_msg)
        g.dfr_pre_msg = dupstr(pre_msg);
    if (post_msg)
        g.dfr_post_msg = dupstr(post_msg);
}

/* handle something like portal ejection */
void
deferred_goto()
{
    if (!on_level(&u.uz, &u.utolev)) {
        d_level dest, oldlev;
        int typmask = u.utotype; /* save it; goto_level zeroes u.utotype */

        assign_level(&dest, &u.utolev);
        assign_level(&oldlev, &u.uz);
        if (g.dfr_pre_msg)
            pline1(g.dfr_pre_msg);
        goto_level(&dest, !!(typmask & 1), !!(typmask & 2), !!(typmask & 4));
        if (typmask & 0200) { /* remove portal */
            struct trap *t = t_at(u.ux, u.uy);

            if (t) {
                deltrap(t);
                newsym(u.ux, u.uy);
            }
        }
        if (g.dfr_post_msg && !on_level(&u.uz, &oldlev))
            pline1(g.dfr_post_msg);
    }
    u.utotype = 0; /* our caller keys off of this */
    if (g.dfr_pre_msg)
        free((genericptr_t) g.dfr_pre_msg), g.dfr_pre_msg = 0;
    if (g.dfr_post_msg)
        free((genericptr_t) g.dfr_post_msg), g.dfr_post_msg = 0;
}

/*
 * Return TRUE if we created a monster for the corpse.  If successful, the
 * corpse is gone.
 */
boolean
revive_corpse(corpse)
struct obj *corpse;
{
    struct monst *mtmp, *mcarry;
    boolean is_uwep, chewed;
    xchar where;
    char cname[BUFSZ];
    struct obj *container = (struct obj *) 0;
    int container_where = 0;

    where = corpse->where;
    is_uwep = (corpse == uwep);
    chewed = (corpse->oeaten != 0);
    Strcpy(cname, corpse_xname(corpse,
                               chewed ? "bite-covered" : (const char *) 0,
                               CXN_SINGULAR));
    mcarry = (where == OBJ_MINVENT) ? corpse->ocarry : 0;

    if (where == OBJ_CONTAINED) {
        struct monst *mtmp2;

        container = corpse->ocontainer;
        mtmp2 = get_container_location(container, &container_where, (int *) 0);
        /* container_where is the outermost container's location even if
         * nested */
        if (container_where == OBJ_MINVENT && mtmp2)
            mcarry = mtmp2;
    }
    mtmp = revive(corpse, FALSE); /* corpse is gone if successful */

    if (mtmp) {
        switch (where) {
        case OBJ_INVENT:
            if (is_uwep)
                pline_The("%s writhes out of your grasp!", cname);
            else
                You_feel("squirming in your backpack!");
            break;

        case OBJ_FLOOR:
            if (cansee(mtmp->mx, mtmp->my))
                pline("%s rises from the dead!",
                      chewed ? Adjmonnam(mtmp, "bite-covered")
                             : Monnam(mtmp));
            break;

        case OBJ_MINVENT: /* probably a nymph's */
            if (cansee(mtmp->mx, mtmp->my)) {
                if (canseemon(mcarry))
                    pline("Startled, %s drops %s as it revives!",
                          mon_nam(mcarry), an(cname));
                else
                    pline("%s suddenly appears!",
                          chewed ? Adjmonnam(mtmp, "bite-covered")
                                 : Monnam(mtmp));
            }
            break;
        case OBJ_CONTAINED: {
            char sackname[BUFSZ];

            if (container_where == OBJ_MINVENT && cansee(mtmp->mx, mtmp->my)
                && mcarry && canseemon(mcarry) && container) {
                pline("%s writhes out of %s!", Amonnam(mtmp),
                      yname(container));
            } else if (container_where == OBJ_INVENT && container) {
                Strcpy(sackname, an(xname(container)));
                pline("%s %s out of %s in your pack!",
                      Blind ? Something : Amonnam(mtmp),
                      locomotion(mtmp->data, "writhes"), sackname);
            } else if (container_where == OBJ_FLOOR && container
                       && cansee(mtmp->mx, mtmp->my)) {
                Strcpy(sackname, an(xname(container)));
                pline("%s escapes from %s!", Amonnam(mtmp), sackname);
            }
            break;
        }
        default:
            /* we should be able to handle the other cases... */
            impossible("revive_corpse: lost corpse @ %d", where);
            break;
        }
        return TRUE;
    }
    return FALSE;
}

/* Revive the corpse via a timeout. */
/*ARGSUSED*/
void
revive_mon(arg, timeout)
anything *arg;
long timeout UNUSED;
{
    struct obj *body = arg->a_obj;
    struct permonst *mptr = &mons[body->corpsenm];
    struct monst *mtmp;
    xchar x, y;

    /* corpse will revive somewhere else if there is a monster in the way;
       Riders get a chance to try to bump the obstacle out of their way */
    if ((mptr->mflags3 & M3_DISPLACES) != 0 && body->where == OBJ_FLOOR
        && get_obj_location(body, &x, &y, 0) && (mtmp = m_at(x, y)) != 0) {
        boolean notice_it = canseemon(mtmp); /* before rloc() */
        char *monname = Monnam(mtmp);

        if (rloc(mtmp, TRUE)) {
            if (notice_it && !canseemon(mtmp))
                pline("%s vanishes.", monname);
            else if (!notice_it && canseemon(mtmp))
                pline("%s appears.", Monnam(mtmp)); /* not pre-rloc monname */
            else if (notice_it && dist2(mtmp->mx, mtmp->my, x, y) > 2)
                pline("%s teleports.", monname); /* saw it and still see it */
        }
    }

    /* if we succeed, the corpse is gone */
    if (!revive_corpse(body)) {
        long when;
        int action;

        if (is_rider(mptr) && rn2(99)) { /* Rider usually tries again */
            action = REVIVE_MON;
            for (when = 3L; when < 67L; when++)
                if (!rn2(3))
                    break;
        } else { /* rot this corpse away */
            You_feel("%sless hassled.", is_rider(mptr) ? "much " : "");
            action = ROT_CORPSE;
            when = 250L - (g.monstermoves - body->age);
            if (when < 1L)
                when = 1L;
        }
        (void) start_timer(when, TIMER_OBJECT, action, arg);
    }
}

int
donull()
{
    return 1; /* Do nothing, but let other things happen */
}

static int
wipeoff(VOID_ARGS)
{
    if (u.ucreamed < 4)
        u.ucreamed = 0;
    else
        u.ucreamed -= 4;
    if (Blinded < 4)
        Blinded = 0;
    else
        Blinded -= 4;
    if (!Blinded) {
        pline("You've got the glop off.");
        u.ucreamed = 0;
        if (!gulp_blnd_check()) {
            Blinded = 1;
            make_blinded(0L, TRUE);
        }
        return 0;
    } else if (!u.ucreamed) {
        Your("%s feels clean now.", body_part(FACE));
        return 0;
    }
    return 1; /* still busy */
}

int
dowipe()
{
    if (u.ucreamed) {
        static NEARDATA char buf[39];

        Sprintf(buf, "wiping off your %s", body_part(FACE));
        set_occupation(wipeoff, buf, 0);
        /* Not totally correct; what if they change back after now
         * but before they're finished wiping?
         */
        return 1;
    }
    Your("%s is already clean.", body_part(FACE));
    return 1;
}

/* common wounded legs feedback */
void
legs_in_no_shape(for_what, by_steed)
const char *for_what; /* jumping, kicking, riding */
boolean by_steed;
{
    if (by_steed && u.usteed) {
        pline("%s is in no shape for %s.", Monnam(u.usteed), for_what);
    } else {
        long wl = (EWounded_legs & BOTH_SIDES);
        const char *bp = body_part(LEG);

        if (wl == BOTH_SIDES)
            bp = makeplural(bp);
        Your("%s%s %s in no shape for %s.",
             (wl == LEFT_SIDE) ? "left " : (wl == RIGHT_SIDE) ? "right " : "",
             bp, (wl == BOTH_SIDES) ? "are" : "is", for_what);
    }
}

void
set_wounded_legs(side, timex)
long side;
int timex;
{
    /* KMH -- STEED
     * If you are riding, your steed gets the wounded legs instead.
     * You still call this function, but don't lose hp.
     * Caller is also responsible for adjusting messages.
     */

    g.context.botl = 1;
    if (!Wounded_legs)
        ATEMP(A_DEX)--;

    if (!Wounded_legs || (HWounded_legs & TIMEOUT) < timex)
        set_itimeout(&HWounded_legs, (long) timex);
    EWounded_legs = side;
    (void) encumber_msg();
}

void
heal_legs(how)
int how; /* 0: ordinary, 1: dismounting steed, 2: limbs turn to stone */
{
    if (Wounded_legs) {
        g.context.botl = 1;
        if (ATEMP(A_DEX) < 0)
            ATEMP(A_DEX)++;

        /* when mounted, wounded legs applies to the steed;
           during petrification countdown, "your limbs turn to stone"
           before the final stages and that calls us (how==2) to cure
           wounded legs, but we want to suppress the feel better message */
        if (!u.usteed && how != 2) {
            const char *legs = body_part(LEG);

            if ((EWounded_legs & BOTH_SIDES) == BOTH_SIDES)
                legs = makeplural(legs);
            /* this used to say "somewhat better" but that was
               misleading since legs are being fully healed */
            Your("%s %s better.", legs, vtense(legs, "feel"));
        }

        HWounded_legs = EWounded_legs = 0L;

        /* Wounded_legs reduces carrying capacity, so we want
           an encumbrance check when they're healed.  However,
           while dismounting, first steed's legs get healed,
           then hero is dropped to floor and a new encumbrance
           check is made [in dismount_steed()].  So don't give
           encumbrance feedback during the dismount stage
           because it could seem to be shown out of order and
           it might be immediately contradicted [able to carry
           more when steed becomes healthy, then possible floor
           feedback, then able to carry less when back on foot]. */
        if (how == 0)
            (void) encumber_msg();
    }
}

/*do.c*/

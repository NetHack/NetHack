/* NetHack 3.7	do.c	$NHDT-Date: 1652831519 2022/05/17 23:51:59 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.304 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

/* Contains code for 'd', 'D' (drop), '>', '<' (up, down) */

#include "hack.h"

static void polymorph_sink(void);
static boolean teleport_sink(void);
static void dosinkring(struct obj *);
static int drop(struct obj *);
static int menudrop_split(struct obj *, long);
static boolean engulfer_digests_food(struct obj *);
static int wipeoff(void);
static int menu_drop(int);
static NHFILE *currentlevel_rewrite(void);
static void final_level(void);

/* static boolean badspot(coordxy,coordxy); */

/* the #drop command: drop one inventory item */
int
dodrop(void)
{
    int result;

    if (*u.ushops)
        sellobj_state(SELL_DELIBERATE);
    result = drop(getobj("drop", any_obj_ok,
                         GETOBJ_PROMPT | GETOBJ_ALLOWCNT));
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
boulder_hits_pool(
    struct obj *otmp,
    coordxy rx, coordxy ry,
    boolean pushing)
{
    if (!otmp || otmp->otyp != BOULDER) {
        impossible("Not a boulder?");
    } else if (is_pool_or_lava(rx, ry)) {
        boolean lava = is_lava(rx, ry), fills_up;
        const char *what = waterbody_name(rx, ry);
        schar ltyp = levl[rx][ry].typ;
        int chance = rn2(10); /* water: 90%; lava: 10% */
        struct monst *mtmp;

        /* chance for boulder to fill pool:  Plane of Water==0%,
           lava 10%, wall of water==50%, other water==90% */
        fills_up = Is_waterlevel(&u.uz) ? FALSE
                   : IS_WATERWALL(ltyp) ? (chance < 5)
                     : lava ? (chance == 0) : (chance != 0);

        if (fills_up) {
            struct trap *ttmp = t_at(rx, ry);

            if (ltyp == DRAWBRIDGE_UP) {
                levl[rx][ry].drawbridgemask &= ~DB_UNDER; /* clear lava */
                levl[rx][ry].drawbridgemask |= DB_FLOOR;
            } else {
                levl[rx][ry].typ = ROOM, levl[rx][ry].flags = 0;
            }
            /* 3.7: normally DEADMONSTER() is used when traversing the fmon
               list--dead monsters usually aren't still at specific map
               locations; however, if ice melts causing a giant to drown,
               that giant would still be on the map when it drops inventory;
               if it was carrying a boulder which now fills the pool, 'mtmp'
               will be dead here; killing it again would yield impossible
               "dmonsfree: N removed doesn't match N+1 pending" when other
               monsters have finished their current turn */
            if ((mtmp = m_at(rx, ry)) != 0 && !DEADMONSTER(mtmp))
                mondied(mtmp);

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
                if (Verbose(0, boulder_hits_pool1) && !Blind)
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
                } else if (!Deaf) {
                    You_hear("a%s splash.", lava ? " sizzling" : "");
                }
                wake_nearto(rx, ry, 40);
            }

            if (fills_up && u.uinwater && distu(rx, ry) == 0) {
                set_uinwater(0); /* u.uinwater = 0 */
                docrt();
                g.vision_full_recalc = 1;
                You("find yourself on dry land again!");
            } else if (lava && next2u(rx, ry)) {
                int dmg;

                You("are hit by molten %s%c",
                    hliquid("lava"), Fire_resistance ? '.' : '!');
                burn_away_slime();
                dmg = d((Fire_resistance ? 1 : 3), 6);
                losehp(Maybe_Half_Phys(dmg), /* lava damage */
                       "molten lava", KILLED_BY);
            } else if (!fills_up && Verbose(0, boulder_hits_pool2)
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
flooreffects(struct obj *obj, coordxy x, coordxy y, const char *verb)
{
    struct trap *t;
    struct monst *mtmp;
    struct obj *otmp;
    coord save_bhitpos;
    boolean tseen;
    int ttyp = NO_TRAP, res = FALSE;

    if (obj->where != OBJ_FREE)
        panic("flooreffects: obj not free");

    /* make sure things like water_damage() have no pointers to follow */
    obj->nobj = obj->nexthere = (struct obj *) 0;
    /* erode_obj() (called from water_damage() or lava_damage()) needs
       bhitpos, but that was screwing up wand zapping that called us from
       rloco(), so we now restore bhitpos before we return */
    save_bhitpos = g.bhitpos;
    g.bhitpos.x = x, g.bhitpos.y = y;

    if (obj->otyp == BOULDER && boulder_hits_pool(obj, x, y, FALSE)) {
        res = TRUE;
    } else if (obj->otyp == BOULDER && (t = t_at(x, y)) != 0
               && (is_pit(t->ttyp) || is_hole(t->ttyp))) {
        ttyp = t->ttyp;
        tseen = t->tseen ? TRUE : FALSE;
        if (((mtmp = m_at(x, y)) && mtmp->mtrapped)
            || (u.utrap && u_at(x,y))) {
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
                        res = FALSE; /* still alive, boulder still intact */
                }
                mtmp->mtrapped = 0;
            } else {
                if (!Passes_walls && !throws_rocks(g.youmonst.data)) {
                    losehp(Maybe_Half_Phys(rnd(15)),
                           "squished under a boulder", NO_KILLER_PREFIX);
                    goto deletedwithboulder;
                } else
                    reset_utrap(TRUE);
            }
        }
        if (*verb) {
            if (Blind && u_at(x, y)) {
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
 deletedwithboulder:
        if ((t = t_at(x, y)) != 0)
            deltrap(t);
        useupf(obj, 1L);
        bury_objs(x, y);
        newsym(x, y);
        res = TRUE;
    } else if (is_lava(x, y)) {
        res = lava_damage(obj, x, y);
    } else if (is_pool(x, y)) {
        /* Reasonably bulky objects (arbitrary) splash when dropped.
         * If you're floating above the water even small things make
         * noise.  Stuff dropped near fountains always misses */
        if ((Blind || (Levitation || Flying)) && !Deaf && u_at(x, y)) {
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
        res = water_damage(obj, NULL, FALSE) == ER_DESTROYED;
    } else if (u_at(x, y) && (t = t_at(x, y)) != 0
               && (uteetering_at_seen_pit(t) || uescaped_shaft(t))) {
        if (is_pit(t->ttyp)) {
            if (Blind && !Deaf)
                You_hear("%s tumble downwards.", the(xname(obj)));
            else
                pline("%s into %s pit.", Tobjnam(obj, "tumble"),
                      the_your[t->madeby_u]);
        } else if (ship_object(obj, x, y, FALSE)) {
            /* ship_object will print an appropriate "the item falls
             * through the hole" message, so no need to do it here. */
            res = TRUE;
        }
    } else if (obj->globby) {
        /* Globby things like puddings might stick together */
        while (obj && (otmp = obj_nexto_xy(obj, x, y, TRUE)) != 0) {
            pudding_merge_message(obj, otmp);
            /* intentionally not getting the melded object; obj_meld may set
             * obj to null. */
            (void) obj_meld(&obj, &otmp);
        }
        res = (boolean) !obj;
    }

    g.bhitpos = save_bhitpos;
    return res;
}

/* obj is an object dropped on an altar */
void
doaltarobj(struct obj *obj)
{
    if (Blind)
        return;

    if (obj->oclass != COIN_CLASS) {
        /* KMH, conduct */
        if (!u.uconduct.gnostic++)
            livelog_printf(LL_CONDUCT,
                           "eschewed atheism, by dropping %s on an altar",
                           doname(obj));
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

/* If obj is neither formally identified nor informally called something
 * already, prompt the player to call its object type. */
void
trycall(struct obj *obj)
{
    if (!objects[obj->otyp].oc_name_known && !objects[obj->otyp].oc_uname)
        docall(obj);
}

/* Transforms the sink at the player's position into
   a fountain, throne, altar or grave. */
static void
polymorph_sink(void)
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
teleport_sink(void)
{
    coordxy cx, cy;
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
dosinkring(struct obj *obj)
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
canletgo(struct obj *obj, const char *word)
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
drop(struct obj *obj)
{
    if (!obj)
        return ECMD_OK;
    if (!canletgo(obj, "drop"))
        return ECMD_OK;
    if (obj == uwep) {
        if (welded(uwep)) {
            weldmsg(obj);
            return ECMD_OK;
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
        if (Verbose(0, drop1)) {
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
            return ECMD_TIME;
        }
        if (!can_reach_floor(TRUE)) {
            /* we might be levitating due to #invoke Heart of Ahriman;
               if so, levitation would end during call to freeinv()
               and we want hitfloor() to happen before float_down() */
            boolean levhack = finesse_ahriman(obj);

            if (levhack)
                ELevitation = W_ART; /* other than W_ARTI */
            if (Verbose(0, drop2))
                You("drop %s.", doname(obj));
            freeinv(obj);
            hitfloor(obj, TRUE);
            if (levhack)
                float_down(I_SPECIAL | TIMEOUT, W_ARTI | W_ART);
            return ECMD_TIME;
        }
        if (!IS_ALTAR(levl[u.ux][u.uy].typ) && Verbose(0, drop3))
            You("drop %s.", doname(obj));
    }
    dropx(obj);
    return ECMD_TIME;
}

/* dropx - take dropped item out of inventory;
   called in several places - may produce output
   (eg ship_object() and dropy() -> sellobj() both produce output) */
void
dropx(struct obj *obj)
{
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
dropy(struct obj *obj)
{
    dropz(obj, FALSE);
}

/* dropz - really put dropped object at its destination... */
void
dropz(struct obj *obj, boolean with_impact)
{
    if (obj == uwep)
        setuwep((struct obj *) 0);
    if (obj == uquiver)
        setuqwep((struct obj *) 0);
    if (obj == uswapwep)
        setuswapwep((struct obj *) 0);

    if (u.uswallow) {
        /* hero has dropped an item while inside an engulfer */
        if (obj != uball) { /* mon doesn't pick up ball */
            /* moving shop item into engulfer's inventory treated as theft */
            if (is_unpaid(obj))
                (void) stolen_value(obj, u.ux, u.uy, TRUE, FALSE);
            /* add to engulfer's inventory if not immediately eaten */
            if (!engulfer_digests_food(obj))
                (void) mpickobj(u.ustuck, obj);
        }
    } else {
        if (flooreffects(obj, u.ux, u.uy, "drop"))
            return;
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
    (void) encumber_msg();
}

/* when swallowed, move dropped object from OBJ_FREE to u.ustuck's inventory;
   for purple worm, immediately eat any corpse, glob, or special meat item
   from object polymorph; return True if object is used up, False otherwise */
static boolean
engulfer_digests_food(struct obj *obj)
{
    /* animal swallower (purple worn, trapper, lurker above) eats any
       corpse, glob, or meat <item> but not other types of food */
    if (is_animal(u.ustuck->data)
        && (obj->otyp == CORPSE || obj->globby
            || obj->otyp == MEATBALL || obj->otyp == HUGE_CHUNK_OF_MEAT
            || obj->otyp == MEAT_RING || obj->otyp == MEAT_STICK)) {
        boolean could_petrify = FALSE,
                could_poly = FALSE, could_slime = FALSE,
                could_grow = FALSE, could_heal = FALSE;

        if (obj->otyp == CORPSE) {
            could_petrify = touch_petrifies(&mons[obj->corpsenm]);
            could_poly = polyfodder(obj);
            could_grow = (obj->corpsenm == PM_WRAITH);
            could_heal = (obj->corpsenm == PM_NURSE);
        } else if (obj->otyp == GLOB_OF_GREEN_SLIME) {
            could_slime = TRUE;
        }
        /* see or feel the effect */
        pline("%s instantly digested!", Tobjnam(obj, "are"));

        if (could_poly || could_slime) {
            (void) newcham(u.ustuck, could_slime ? &mons[PM_GREEN_SLIME] : 0,
                           could_slime ? NC_SHOW_MSG : NO_NC_FLAGS);
        } else if (could_petrify) {
            minstapetrify(u.ustuck, TRUE);
        } else if (could_grow) {
            (void) grow_up(u.ustuck, (struct monst *) 0);
        } else if (could_heal) {
            u.ustuck->mhp = u.ustuck->mhpmax;
            /* False: don't realize that sight is cured from inside */
            mcureblindness(u.ustuck, FALSE);
        }
        delobj(obj); /* always used up */
        return TRUE;
    }
    return FALSE;
}

/* things that must change when not held; recurse into containers.
   Called for both player and monsters */
void
obj_no_longer_held(struct obj *obj)
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

/* the #droptype command: drop several things */
int
doddrop(void)
{
    int result = ECMD_OK;

    if (!g.invent) {
        You("have nothing to drop.");
        return ECMD_OK;
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

static int /* check callers */
menudrop_split(struct obj *otmp, long cnt)
{
    if (cnt && cnt < otmp->quan) {
        if (welded(otmp)) {
            ; /* don't split */
        } else if (otmp->otyp == LOADSTONE && otmp->cursed) {
            /* same kludge as getobj(), for canletgo()'s use */
            otmp->corpsenm = (int) cnt; /* don't split */
        } else {
            otmp = splitobj(otmp, cnt);
        }
    }
    return drop(otmp);
}

/* Drop things from the hero's inventory, using a menu. */
static int
menu_drop(int retry)
{
    int n, i, n_dropped = 0;
    struct obj *otmp, *otmp2;
    menu_item *pick_list;
    boolean all_categories = TRUE, drop_everything = FALSE, autopick = FALSE;
    boolean drop_justpicked = FALSE;
    long justpicked_quan = 0;

    if (retry) {
        all_categories = (retry == -2);
    } else if (flags.menu_style == MENU_FULL) {
        all_categories = FALSE;
        n = query_category("Drop what type of items?", g.invent,
                           (UNPAID_TYPES | ALL_TYPES | CHOOSE_ALL
                            | BUC_BLESSED | BUC_CURSED | BUC_UNCURSED
                            | BUC_UNKNOWN | JUSTPICKED | INCLUDE_VENOM),
                           &pick_list, PICK_ANY);
        if (!n)
            goto drop_done;
        for (i = 0; i < n; i++) {
            if (pick_list[i].item.a_int == ALL_TYPES_SELECTED) {
                all_categories = TRUE;
            } else if (pick_list[i].item.a_int == 'A') {
                drop_everything = autopick = TRUE;
            } else if (pick_list[i].item.a_int == 'P') {
                justpicked_quan = max(0, pick_list[i].count);
                drop_justpicked = TRUE;
                add_valid_menu_class(pick_list[i].item.a_int);
            } else {
                add_valid_menu_class(pick_list[i].item.a_int);
                drop_everything = FALSE;
            }
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

    if (autopick) {
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
         *
         * FIXME?  if something explodes, or even breaks, we probably
         * ought to halt the traversal or perhaps ask player whether
         * to halt it.
         */
        bypass_objlist(g.invent, FALSE); /* clear bypass bit for invent */
        while ((otmp = nxt_unbypassed_obj(g.invent)) != 0) {
            if (drop_everything || all_categories || allow_category(otmp))
                n_dropped += ((drop(otmp) == ECMD_TIME) ? 1 : 0);
        }
        /* we might not have dropped everything (worn armor, welded weapon,
           cursed loadstones), so reset any remaining inventory to normal */
        bypass_objlist(g.invent, FALSE);
    } else if (drop_justpicked && count_justpicked(g.invent) == 1) {
        /* drop the just picked item automatically, if only one stack */
        otmp = find_justpicked(g.invent);
        if (otmp)
            n_dropped += ((menudrop_split(otmp, justpicked_quan) == ECMD_TIME) ? 1 : 0);
    } else {
        /* should coordinate with perm invent, maybe not show worn items */
        n = query_objlist("What would you like to drop?", &g.invent,
                          (USE_INVLET | INVORDER_SORT | INCLUDE_VENOM),
                          &pick_list, PICK_ANY,
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
                n_dropped += ((menudrop_split(otmp, pick_list[i].count) == ECMD_TIME) ? 1 : 0);
            }
            bypass_objlist(g.invent, FALSE); /* reset g.invent to normal */
            free((genericptr_t) pick_list);
        }
    }

 drop_done:
    return (n_dropped ? ECMD_TIME : ECMD_OK);
}

/* the #down command */
int
dodown(void)
{
    struct trap *trap = 0;
    stairway *stway = stairway_at(u.ux, u.uy);
    boolean stairs_down = (stway && !stway->up && !stway->isladder),
            ladder_down = (stway && !stway->up &&  stway->isladder);

    set_move_cmd(DIR_DOWN, 0);

    if (u_rooted())
        return ECMD_TIME;

    if (stucksteed(TRUE)) {
        return ECMD_OK;
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
                        if (obj->age < g.moves)
                            obj->age = g.moves;
                        obj->age += rnz(100);
                    }
                }
            }
            if (float_down(I_SPECIAL | TIMEOUT, W_ARTI)) {
                return ECMD_TIME; /* came down, so moved */
            } else if (!HLevitation && !ELevitation) {
                Your("latent levitation ceases.");
                return ECMD_TIME; /* did something, effectively moved */
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
        return ECMD_OK; /* didn't move */
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
        return ECMD_TIME; /* came out of hiding; need '>' again to go down */
    }

    if (u.ustuck) {
        You("are %s, and cannot go down.",
            !u.uswallow ? "being held" : is_animal(u.ustuck->data)
                                             ? "swallowed"
                                             : "engulfed");
        return ECMD_TIME;
    }

    if (!stairs_down && !ladder_down) {
        trap = t_at(u.ux, u.uy);
        if (trap && (uteetering_at_seen_pit(trap) || uescaped_shaft(trap))) {
            dotrap(trap, TOOKPLUNGE);
            return ECMD_TIME;
        } else if (!trap || !is_hole(trap->ttyp)
                   || !Can_fall_thru(&u.uz) || !trap->tseen) {
            if (flags.autodig && !g.context.nopick && uwep && is_pick(uwep)) {
                return use_pick_axe2(uwep);
            } else {
                You_cant("go down here.");
                return ECMD_OK;
            }
        }
    }
    if (on_level(&valley_level, &u.uz) && !u.uevent.gehennom_entered) {
        You("are standing at the gate to Gehennom.");
        pline("Unspeakable cruelty and harm lurk down there.");
        if (yn("Are you sure you want to enter?") != 'y')
            return ECMD_OK;
        pline("So be it.");
        u.uevent.gehennom_entered = 1; /* don't ask again */
    }

    if (!next_to_u()) {
        You("are held back by your pet!");
        return ECMD_OK;
    }

    if (trap) {
        const char *down_or_thru = trap->ttyp == HOLE ? "down" : "through";
        const char *actn = u_locomotion("jump");

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
                    return ECMD_OK;
                }
            } else {
                return ECMD_OK;
            }
        }
        You("%s %s the %s.", actn, down_or_thru,
            trap->ttyp == HOLE ? "hole" : "trap door");
    }
    if (trap && Is_stronghold(&u.uz)) {
        goto_hell(FALSE, TRUE);
    } else if (trap) {
        goto_level(&(trap->dst), FALSE, FALSE, FALSE);
    } else {
        g.at_ladder = (boolean) (levl[u.ux][u.uy].typ == LADDER);
        next_level(!trap);
        g.at_ladder = FALSE;
    }
    return ECMD_TIME;
}

/* the #up command - move up a staircase */
int
doup(void)
{
    stairway *stway = stairway_at(u.ux,u.uy);

    set_move_cmd(DIR_UP, 0);

    if (u_rooted())
        return ECMD_TIME;

    /* "up" to get out of a pit... */
    if (u.utrap && u.utraptype == TT_PIT) {
        climb_pit();
        return ECMD_TIME;
    }

    if (!stway || (stway && !stway->up)) {
        You_cant("go up here.");
        return ECMD_OK;
    }
    if (stucksteed(TRUE)) {
        return ECMD_OK;
    }
    if (u.ustuck) {
        You("are %s, and cannot go up.",
            !u.uswallow ? "being held" : is_animal(u.ustuck->data)
                                             ? "swallowed"
                                             : "engulfed");
        return ECMD_TIME;
    }
    if (near_capacity() > SLT_ENCUMBER) {
        /* No levitation check; inv_weight() already allows for it */
        Your("load is too heavy to climb the %s.",
             levl[u.ux][u.uy].typ == STAIRS ? "stairs" : "ladder");
        return ECMD_TIME;
    }
    if (ledger_no(&u.uz) == 1) {
        if (iflags.debug_fuzzer)
            return ECMD_OK;
        if (yn("Beware, there will be no return!  Still climb?") != 'y')
            return ECMD_OK;
    }
    if (!next_to_u()) {
        You("are held back by your pet!");
        return ECMD_OK;
    }
    g.at_ladder = (boolean) (levl[u.ux][u.uy].typ == LADDER);
    prev_level(TRUE);
    g.at_ladder = FALSE;
    return ECMD_TIME;
}

/* check that we can write out the current level */
static NHFILE *
currentlevel_rewrite(void)
{
    NHFILE *nhfp;
    char whynot[BUFSZ];

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

    return nhfp;
}

#ifdef INSURANCE
void
save_currentstate(void)
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
badspot(register coordxy x, register coordxy y)
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
u_collide_m(struct monst *mtmp)
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
        && next2u(cc.x, cc.y))
        u_on_newpos(cc.x, cc.y); /*[maybe give message here?]*/
    else
        mnexto(mtmp, RLOC_NOMSG);

    if ((mtmp = m_at(u.ux, u.uy)) != 0) {
        /* there was an unconditional impossible("mnexto failed")
           here, but it's not impossible and we're prepared to cope
           with the situation, so only say something when debugging */
        if (wizard)
            pline("(monster in hero's way)");
        if (!rloc(mtmp, RLOC_NOMSG) || (mtmp = m_at(u.ux, u.uy)) != 0)
            /* no room to move it; send it away, to return later */
            m_into_limbo(mtmp);
    }
}

DISABLE_WARNING_FORMAT_NONLITERAL

void
goto_level(
    d_level *newlevel, /* destination */
    boolean at_stairs, /* True if arriving via stairs/ladder */
    boolean falling,   /* when falling to level, objects might tag along */
    boolean portal)    /* True if arriving via magic portal */
{
    int l_idx, save_mode;
    NHFILE *nhfp;
    xint16 new_ledger;
    boolean cant_go_back, great_effort,
            up = (depth(newlevel) < depth(&u.uz)),
            newdungeon = (u.uz.dnum != newlevel->dnum),
            was_in_W_tower = In_W_tower(u.ux, u.uy, &u.uz),
            familiar = FALSE,
            new = FALSE; /* made a new level? */
    struct monst *mtmp;
    char whynot[BUFSZ];
    char *annotation;
    int dist = depth(newlevel) - depth(&u.uz);
    boolean do_fall_dmg = FALSE;

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
    set_uinwater(0); /* u.uinwater = 0 */
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
    } else {
        free_luathemes(TRUE);
    }
    save_mode = nhfp->mode;
    nhfp->mode = cant_go_back ? FREEING : (WRITING | FREEING);
    savelev(nhfp, ledger_no(&u.uz));
    nhfp->mode = save_mode;
    close_nhfile(nhfp);
    if (cant_go_back) {
        /* discard unreachable levels; keep #0 */
        for (l_idx = maxledgerno(); l_idx > 0; --l_idx)
            delete_levelfile(l_idx);
        /* mark #overview data for all dungeon branches as uninteresting */
        for (l_idx = 0; l_idx < g.n_dgns; ++l_idx)
            remdun_mapseen(l_idx);
        /* get rid of mons & objs scheduled to migrate to discarded levels */
        discard_migrations();
    }

    if (Is_rogue_level(newlevel) || Is_rogue_level(&u.uz))
        assign_graphics(Is_rogue_level(newlevel) ? ROGUESET : PRIMARYSET);
    check_gold_symbol();
    /* record this level transition as a potential seen branch unless using
     * some non-standard means of transportation (level teleport).
     */
    if ((at_stairs || falling || portal) && (u.uz.dnum != newlevel->dnum))
        recbranch_mapseen(&u.uz, newlevel);
    assign_level(&u.uz0, &u.uz);
    assign_level(&u.uz, newlevel);
    assign_level(&u.utolev, newlevel);
    u.utotype = UTOTYPE_NONE;
    if (!builds_up(&u.uz)) { /* usual case */
        if (dunlev(&u.uz) > dunlev_reached(&u.uz))
            dunlev_reached(&u.uz) = dunlev(&u.uz);
    } else {
        if (dunlev_reached(&u.uz) == 0
            || dunlev(&u.uz) < dunlev_reached(&u.uz))
            dunlev_reached(&u.uz) = dunlev(&u.uz);
    }

    stairway_free_all();
    /* set default level change destination areas */
    /* the special level code may override these */
    (void) memset((genericptr_t) &g.updest, 0, sizeof g.updest);
    (void) memset((genericptr_t) &g.dndest, 0, sizeof g.dndest);

    if (!(g.level_info[new_ledger].flags & LFILE_EXISTS)) {
        /* entering this level for first time; make it now */
        if (g.level_info[new_ledger].flags & (VISITED)) {
            impossible("goto_level: returning to discarded level?");
            g.level_info[new_ledger].flags &= ~(VISITED);
        }
        mklev();
        new = TRUE; /* made the level */
        familiar = bones_include_name(g.plname);
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
        close_nhfile(nhfp);
        oinit(); /* reassign level dependent obj probabilities */
    }
    reglyph_darkroom();
    set_uinwater(0); /* u.uinwater = 0 */
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

        if (!ttrap) {
            if (u.uevent.qexpelled
                && (Is_qstart(&u.uz0) || Is_qstart(&u.uz))) {
                /* we're coming back from or going into the quest home level,
                   after already getting expelled once. The portal back
                   doesn't exist anymore - see expulsion(). */
                u_on_rndspot(0);
            } else {
                panic("goto_level: no corresponding portal!");
            }
        } else {
            seetrap(ttrap);
            u_on_newpos(ttrap->tx, ttrap->ty);
        }
    } else if (at_stairs && !In_endgame(&u.uz)) {
        if (up) {
            stairway *stway = stairway_find_from(&u.uz0, g.at_ladder);
            if (stway) {
                u_on_newpos(stway->sx, stway->sy);
                stway->u_traversed = TRUE;
            } else if (newdungeon)
                u_on_sstairs(1);
            else
                u_on_dnstairs();
            /* you climb up the {stairs|ladder};
               fly up the stairs; fly up along the ladder */
            great_effort = (Punished && !Levitation);
            if (Verbose(0, go_to_level1) || great_effort)
                pline("%s %s up%s the %s.",
                      great_effort ? "With great effort, you" : "You",
                      u_locomotion("climb"),
                      (Flying && g.at_ladder) ? " along" : "",
                      g.at_ladder ? "ladder" : "stairs");
        } else { /* down */
            stairway *stway = stairway_find_from(&u.uz0, g.at_ladder);
            if (stway) {
                u_on_newpos(stway->sx, stway->sy);
                stway->u_traversed = TRUE;
            } else if (newdungeon)
                u_on_sstairs(0);
            else
                u_on_upstairs();
            if (!u.dz) {
                ; /* stayed on same level? (no transit effects) */
            } else if (Flying) {
                if (Verbose(0, go_to_level2))
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
                if (Verbose(0, go_to_level3))
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
            do_fall_dmg = TRUE;
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

    /* Reset the screen. */
    vision_reset(); /* reset the blockages */
    reset_glyphmap(gm_levelchange);
    docrt(); /* does a full vision recalc */
    flush_screen(-1);

    /*
     *  Move all plines beyond the screen reset.
     */

    /* deferred arrival message for level teleport looks odd if given
       after the various messages below, so give it before them;
       [it might have already been delivered via docrt() -> see_monsters()
       -> Sting_effects() -> maybe_lvltport_feedback(), in which case
       'dfr_post_msg' has already been reset to Null];
       if 'dfr_post_msg' is "you materialize on a different level" then
       maybe_lvltport_feedback() will deliver it now and then free it */
    if (g.dfr_post_msg)
        maybe_lvltport_feedback(); /* potentially called by Sting_effects() */

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
            hellish_smoke_mesg(); /* "It is hot here.  You smell smoke..." */

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
            /* [TODO: copy of same TODO below; if an achievement for
               receiving quest call from leader gets added, that should
               come after logging new level entry] */
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

    /* this was originally done earlier; moved here to be logged after
       any achievement related to entering a dungeon branch
       [TODO: if an achievement for receiving quest call from leader
       gets added, that should come after this rather than take place
       where the message is delivered above] */
    if (new) {
        char dloc[QBUFSZ];
        /* Astral is excluded as a major event here because entry to it
           is already one due to that being an achievement */
        boolean major = In_endgame(&u.uz) && !Is_astralevel(&u.uz);

        (void) describe_level(dloc, 2);
        livelog_printf(major ? LL_ACHIEVE : LL_DEBUG, "entered %s", dloc);
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
    /* shop repair is normally done when shopkeepers move, but we may
       need to catch up for lost time here; do this before maybe dying
       so bones map will include it */
    if (!new)
        fix_shop_damage();

    /* fall damage? */
    if (do_fall_dmg) {
        int dmg = d(dist, 6);

        dmg = Maybe_Half_Phys(dmg);
        losehp(dmg, "falling down a mine shaft", KILLED_BY);
    }

    (void) pickup(1);
    return;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* give a message when entering a Gehennom level other than the Valley;
   also given if restoring a game in that situation */
void
hellish_smoke_mesg(void)
{
    if (Inhell && !Is_valley(&u.uz))
        pline("It is hot here.  You %s smoke...",
              olfaction(g.youmonst.data) ? "smell" : "sense");
}

/* usually called from goto_level(); might be called from Sting_effects() */
void
maybe_lvltport_feedback(void)
{
    if (g.dfr_post_msg && !strncmpi(g.dfr_post_msg, "You materialize", 15)) {
        /* "You materialize on a different level." */
        pline("%s", g.dfr_post_msg);
        free((genericptr_t) g.dfr_post_msg), g.dfr_post_msg = 0;
    }
}

static void
final_level(void)
{
    /* reset monster hostility relative to player */
    iter_mons(reset_hostility);

    /* create some player-monsters */
    create_mplayers(rn1(4, 3), TRUE);

    /* create a guardian angel next to player, if worthy */
    gain_guardian_angel();
}

/* change levels at the end of this turn, after monsters finish moving */
void
schedule_goto(d_level *tolev, int utotype_flags,
              const char *pre_msg, const char *post_msg)
{
    /* UTOTYPE_DEFERRED is used, so UTOTYPE_NONE can trigger deferred_goto() */
    u.utotype = utotype_flags | UTOTYPE_DEFERRED;
    /* destination level */
    assign_level(&u.utolev, tolev);

    if (pre_msg)
        g.dfr_pre_msg = dupstr(pre_msg);
    if (post_msg)
        g.dfr_post_msg = dupstr(post_msg);
}

/* handle something like portal ejection */
void
deferred_goto(void)
{
    if (!on_level(&u.uz, &u.utolev)) {
        d_level dest, oldlev;
        int typmask = u.utotype; /* save it; goto_level zeroes u.utotype */

        assign_level(&dest, &u.utolev);
        assign_level(&oldlev, &u.uz);
        if (g.dfr_pre_msg)
            pline1(g.dfr_pre_msg);
        goto_level(&dest, !!(typmask & UTOTYPE_ATSTAIRS),
                   !!(typmask & UTOTYPE_FALLING),
                   !!(typmask & UTOTYPE_PORTAL));
        if (typmask & UTOTYPE_RMPORTAL) { /* remove portal */
            struct trap *t = t_at(u.ux, u.uy);

            if (t) {
                deltrap(t);
                newsym(u.ux, u.uy);
            }
        }
        if (g.dfr_post_msg && !on_level(&u.uz, &oldlev))
            pline1(g.dfr_post_msg);
    }
    u.utotype = UTOTYPE_NONE; /* our caller keys off of this */
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
revive_corpse(struct obj *corpse)
{
    struct monst *mtmp, *mcarry;
    boolean is_uwep, chewed;
    xint16 where;
    char cname[BUFSZ];
    struct obj *container = (struct obj *) 0;
    int container_where = 0;
    boolean is_zomb = (mons[corpse->corpsenm].mlet == S_ZOMBIE);

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
            if (cansee(mtmp->mx, mtmp->my)) {
                const char *effect = "";

                if (mtmp->data == &mons[PM_DEATH])
                    effect = " in a whirl of spectral skulls";
                else if (mtmp->data == &mons[PM_PESTILENCE])
                    effect = " in a churning pillar of flies";
                else if (mtmp->data == &mons[PM_FAMINE])
                    effect = " in a ring of withered crops";

                pline("%s rises from the dead%s!",
                      chewed ? Adjmonnam(mtmp, "bite-covered")
                             : Monnam(mtmp), effect);
            }
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
        case OBJ_BURIED:
            if (is_zomb) {
                maketrap(mtmp->mx, mtmp->my, PIT);
                if (cansee(mtmp->mx, mtmp->my)) {
                    struct trap *ttmp;

                    ttmp = t_at(mtmp->mx, mtmp->my);
                    ttmp->tseen = TRUE;
                    pline("%s claws itself out of the ground!", Amonnam(mtmp));
                    newsym(mtmp->mx, mtmp->my);
                } else if (distu(mtmp->mx, mtmp->my) < 5*5)
                    You_hear("scratching noises.");
                break;
            }
            /*FALLTHRU*/
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
revive_mon(anything *arg, long timeout UNUSED)
{
    struct obj *body = arg->a_obj;
    struct permonst *mptr = &mons[body->corpsenm];
    struct monst *mtmp;
    coordxy x, y;

    /* corpse will revive somewhere else if there is a monster in the way;
       Riders get a chance to try to bump the obstacle out of their way */
    if (is_displacer(mptr) && body->where == OBJ_FLOOR
        && get_obj_location(body, &x, &y, 0) && (mtmp = m_at(x, y)) != 0) {
        boolean notice_it = canseemon(mtmp); /* before rloc() */
        char *monname = Monnam(mtmp);

        if (rloc(mtmp, RLOC_NOMSG)) {
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
            when = rider_revival_time(body, TRUE);
        } else { /* rot this corpse away */
            if (!obj_has_timer(body, ROT_CORPSE))
                You_feel("%sless hassled.", is_rider(mptr) ? "much " : "");
            action = ROT_CORPSE;
            when = (long) d(5, 50) - (g.moves - body->age);
            if (when < 1L)
                when = 1L;
        }
        if (!obj_has_timer(body, action))
            (void) start_timer(when, TIMER_OBJECT, action, arg);
    }
}

/* Timeout callback. Revive the corpse as a zombie. */
void
zombify_mon(anything *arg, long timeout)
{
    struct obj *body = arg->a_obj;
    int zmon = zombie_form(&mons[body->corpsenm]);

    if (zmon != NON_PM && !(g.mvitals[zmon].mvflags & G_GENOD)) {
        if (has_omid(body))
            free_omid(body);
        if (has_omonst(body))
            free_omonst(body);

        set_corpsenm(body, zmon);
        revive_mon(arg, timeout);
    } else {
        rot_corpse(arg, timeout);
    }
}

boolean
cmd_safety_prevention(const char *cmddesc, const char *act, int *flagcounter)
{
    if (flags.safe_wait && !iflags.menu_requested
        && !g.multi && monster_nearby()) {
        char buf[QBUFSZ];

        buf[0] = '\0';
        if (iflags.cmdassist || !(*flagcounter)++)
            Sprintf(buf, "  Use '%s' prefix to force %s.",
                    visctrl(cmd_from_func(do_reqmenu)), cmddesc);
        Norep("%s%s", act, buf);
        return TRUE;
    }
    *flagcounter = 0;
    return FALSE;
}

/* '.' command: do nothing == rest; also the
   ' ' command iff 'rest_on_space' option is On */
int
donull(void)
{
    if (cmd_safety_prevention("a no-op (to rest)",
                          "Are you waiting to get hit?",
                          &g.did_nothing_flag))
        return ECMD_OK;
    return ECMD_TIME; /* Do nothing, but let other things happen */
}

static int
wipeoff(void)
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

/* the #wipe command - wipe off your face */
int
dowipe(void)
{
    if (u.ucreamed) {
        static NEARDATA char buf[39];

        Sprintf(buf, "wiping off your %s", body_part(FACE));
        set_occupation(wipeoff, buf, 0);
        /* Not totally correct; what if they change back after now
         * but before they're finished wiping?
         */
        return ECMD_TIME;
    }
    Your("%s is already clean.", body_part(FACE));
    return ECMD_TIME;
}

/* common wounded legs feedback */
void
legs_in_no_shape(const char *for_what, /* jumping, kicking, riding */
                 boolean by_steed)
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
set_wounded_legs(long side, int timex)
{
    /* KMH -- STEED
     * If you are riding, your steed gets the wounded legs instead.
     * You still call this function, but don't lose hp.
     * Caller is also responsible for adjusting messages.
     */
    g.context.botl = 1;
    if (!Wounded_legs)
        ATEMP(A_DEX)--;

    if (!Wounded_legs || (HWounded_legs & TIMEOUT) < (long) timex)
        set_itimeout(&HWounded_legs, (long) timex);
    /* the leg being wounded and its timeout might differ from one
       attack to the next, but we don't track the legs separately;
       3.7: both legs will ultimately heal together; this used to use
       direct assignment instead of bitwise-OR so getting wounded in
       one leg mysteriously healed the other */
    EWounded_legs |= side;
    (void) encumber_msg();
}

void
heal_legs(
    int how) /* 0: ordinary, 1: dismounting steed, 2: limbs turn to stone */
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

/* NetHack 3.6	hack.c	$NHDT-Date: 1540591769 2018/10/26 22:09:29 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.194 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* #define DEBUG */ /* uncomment for debugging */

STATIC_DCL void NDECL(maybe_wail);
STATIC_DCL int NDECL(moverock);
STATIC_DCL int FDECL(still_chewing, (XCHAR_P, XCHAR_P));
STATIC_DCL void NDECL(dosinkfall);
STATIC_DCL boolean FDECL(findtravelpath, (int));
STATIC_DCL boolean FDECL(trapmove, (int, int, struct trap *));
STATIC_DCL void NDECL(switch_terrain);
STATIC_DCL struct monst *FDECL(monstinroom, (struct permonst *, int));
STATIC_DCL boolean FDECL(doorless_door, (int, int));
STATIC_DCL void FDECL(move_update, (BOOLEAN_P));

#define IS_SHOP(x) (rooms[x].rtype >= SHOPBASE)

/* mode values for findtravelpath() */
#define TRAVP_TRAVEL 0
#define TRAVP_GUESS  1
#define TRAVP_VALID  2

static anything tmp_anything;

anything *
uint_to_any(ui)
unsigned ui;
{
    tmp_anything = zeroany;
    tmp_anything.a_uint = ui;
    return &tmp_anything;
}

anything *
long_to_any(lng)
long lng;
{
    tmp_anything = zeroany;
    tmp_anything.a_long = lng;
    return &tmp_anything;
}

anything *
monst_to_any(mtmp)
struct monst *mtmp;
{
    tmp_anything = zeroany;
    tmp_anything.a_monst = mtmp;
    return &tmp_anything;
}

anything *
obj_to_any(obj)
struct obj *obj;
{
    tmp_anything = zeroany;
    tmp_anything.a_obj = obj;
    return &tmp_anything;
}

boolean
revive_nasty(x, y, msg)
int x, y;
const char *msg;
{
    register struct obj *otmp, *otmp2;
    struct monst *mtmp;
    coord cc;
    boolean revived = FALSE;

    for (otmp = level.objects[x][y]; otmp; otmp = otmp2) {
        otmp2 = otmp->nexthere;
        if (otmp->otyp == CORPSE
            && (is_rider(&mons[otmp->corpsenm])
                || otmp->corpsenm == PM_WIZARD_OF_YENDOR)) {
            /* move any living monster already at that location */
            if ((mtmp = m_at(x, y)) && enexto(&cc, x, y, mtmp->data))
                rloc_to(mtmp, cc.x, cc.y);
            if (msg)
                Norep("%s", msg);
            revived = revive_corpse(otmp);
        }
    }

    /* this location might not be safe, if not, move revived monster */
    if (revived) {
        mtmp = m_at(x, y);
        if (mtmp && !goodpos(x, y, mtmp, 0)
            && enexto(&cc, x, y, mtmp->data)) {
            rloc_to(mtmp, cc.x, cc.y);
        }
        /* else impossible? */
    }

    return revived;
}

STATIC_OVL int
moverock()
{
    register xchar rx, ry, sx, sy;
    register struct obj *otmp;
    register struct trap *ttmp;
    register struct monst *mtmp;

    sx = u.ux + u.dx, sy = u.uy + u.dy; /* boulder starting position */
    while ((otmp = sobj_at(BOULDER, sx, sy)) != 0) {
        /* make sure that this boulder is visible as the top object */
        if (otmp != level.objects[sx][sy])
            movobj(otmp, sx, sy);

        rx = u.ux + 2 * u.dx; /* boulder destination position */
        ry = u.uy + 2 * u.dy;
        nomul(0);
        if (Levitation || Is_airlevel(&u.uz)) {
            if (Blind)
                feel_location(sx, sy);
            You("don't have enough leverage to push %s.", the(xname(otmp)));
            /* Give them a chance to climb over it? */
            return -1;
        }
        if (verysmall(youmonst.data) && !u.usteed) {
            if (Blind)
                feel_location(sx, sy);
            pline("You're too small to push that %s.", xname(otmp));
            goto cannot_push;
        }
        if (isok(rx, ry) && !IS_ROCK(levl[rx][ry].typ)
            && levl[rx][ry].typ != IRONBARS
            && (!IS_DOOR(levl[rx][ry].typ) || !(u.dx && u.dy)
                || doorless_door(rx, ry)) && !sobj_at(BOULDER, rx, ry)) {
            ttmp = t_at(rx, ry);
            mtmp = m_at(rx, ry);

            /* KMH -- Sokoban doesn't let you push boulders diagonally */
            if (Sokoban && u.dx && u.dy) {
                if (Blind)
                    feel_location(sx, sy);
                pline("%s won't roll diagonally on this %s.",
                      The(xname(otmp)), surface(sx, sy));
                goto cannot_push;
            }

            if (revive_nasty(rx, ry, "You sense movement on the other side."))
                return -1;

            if (mtmp && !noncorporeal(mtmp->data)
                && (!mtmp->mtrapped
                    || !(ttmp && is_pit(ttmp->ttyp)))) {
                if (Blind)
                    feel_location(sx, sy);
                if (canspotmon(mtmp)) {
                    pline("There's %s on the other side.", a_monnam(mtmp));
                } else {
                    You_hear("a monster behind %s.", the(xname(otmp)));
                    map_invisible(rx, ry);
                }
                if (flags.verbose)
                    pline("Perhaps that's why %s cannot move it.",
                          u.usteed ? y_monnam(u.usteed) : "you");
                goto cannot_push;
            }

            if (ttmp) {
                /* if a trap operates on the boulder, don't attempt
                   to move any others at this location; return -1
                   if another boulder is in hero's way, or 0 if he
                   should advance to the vacated boulder position */
                switch (ttmp->ttyp) {
                case LANDMINE:
                    if (rn2(10)) {
                        obj_extract_self(otmp);
                        place_object(otmp, rx, ry);
                        newsym(sx, sy);
                        pline("KAABLAMM!!!  %s %s land mine.",
                              Tobjnam(otmp, "trigger"),
                              ttmp->madeby_u ? "your" : "a");
                        blow_up_landmine(ttmp);
                        /* if the boulder remains, it should fill the pit */
                        fill_pit(u.ux, u.uy);
                        if (cansee(rx, ry))
                            newsym(rx, ry);
                        return sobj_at(BOULDER, sx, sy) ? -1 : 0;
                    }
                    break;
                case SPIKED_PIT:
                case PIT:
                    obj_extract_self(otmp);
                    /* vision kludge to get messages right;
                       the pit will temporarily be seen even
                       if this is one among multiple boulders */
                    if (!Blind)
                        viz_array[ry][rx] |= IN_SIGHT;
                    if (!flooreffects(otmp, rx, ry, "fall")) {
                        place_object(otmp, rx, ry);
                    }
                    if (mtmp && !Blind)
                        newsym(rx, ry);
                    return sobj_at(BOULDER, sx, sy) ? -1 : 0;
                case HOLE:
                case TRAPDOOR:
                    if (Blind)
                        pline("Kerplunk!  You no longer feel %s.",
                              the(xname(otmp)));
                    else
                        pline("%s%s and %s a %s in the %s!",
                              Tobjnam(otmp, (ttmp->ttyp == TRAPDOOR)
                                                ? "trigger"
                                                : "fall"),
                              (ttmp->ttyp == TRAPDOOR) ? "" : " into",
                              otense(otmp, "plug"),
                              (ttmp->ttyp == TRAPDOOR) ? "trap door" : "hole",
                              surface(rx, ry));
                    deltrap(ttmp);
                    delobj(otmp);
                    bury_objs(rx, ry);
                    levl[rx][ry].wall_info &= ~W_NONDIGGABLE;
                    levl[rx][ry].candig = 1;
                    if (cansee(rx, ry))
                        newsym(rx, ry);
                    return sobj_at(BOULDER, sx, sy) ? -1 : 0;
                case LEVEL_TELEP:
                case TELEP_TRAP: {
                    int newlev = 0; /* lint suppression */
                    d_level dest;

                    if (ttmp->ttyp == LEVEL_TELEP) {
                        newlev = random_teleport_level();
                        if (newlev == depth(&u.uz) || In_endgame(&u.uz))
                            /* trap didn't work; skip "disappears" message */
                            goto dopush;
                    }
                    if (u.usteed)
                        pline("%s pushes %s and suddenly it disappears!",
                              upstart(y_monnam(u.usteed)), the(xname(otmp)));
                    else
                        You("push %s and suddenly it disappears!",
                            the(xname(otmp)));
                    if (ttmp->ttyp == TELEP_TRAP) {
                        (void) rloco(otmp);
                    } else {
                        obj_extract_self(otmp);
                        add_to_migration(otmp);
                        get_level(&dest, newlev);
                        otmp->ox = dest.dnum;
                        otmp->oy = dest.dlevel;
                        otmp->owornmask = (long) MIGR_RANDOM;
                    }
                    seetrap(ttmp);
                    return sobj_at(BOULDER, sx, sy) ? -1 : 0;
                }
                default:
                    break; /* boulder not affected by this trap */
                }
            }

            if (closed_door(rx, ry))
                goto nopushmsg;
            if (boulder_hits_pool(otmp, rx, ry, TRUE))
                continue;
            /*
             * Re-link at top of fobj chain so that pile order is preserved
             * when level is restored.
             */
            if (otmp != fobj) {
                remove_object(otmp);
                place_object(otmp, otmp->ox, otmp->oy);
            }

            {
#ifdef LINT /* static long lastmovetime; */
                long lastmovetime;
                lastmovetime = 0;
#else
                /* note: reset to zero after save/restore cycle */
                static NEARDATA long lastmovetime;
#endif
            dopush:
                if (!u.usteed) {
                    if (moves > lastmovetime + 2 || moves < lastmovetime)
                        pline("With %s effort you move %s.",
                              throws_rocks(youmonst.data) ? "little"
                                                          : "great",
                              the(xname(otmp)));
                    exercise(A_STR, TRUE);
                } else
                    pline("%s moves %s.", upstart(y_monnam(u.usteed)),
                          the(xname(otmp)));
                lastmovetime = moves;
            }

            /* Move the boulder *after* the message. */
            if (glyph_is_invisible(levl[rx][ry].glyph))
                unmap_object(rx, ry);
            movobj(otmp, rx, ry); /* does newsym(rx,ry) */
            if (Blind) {
                feel_location(rx, ry);
                feel_location(sx, sy);
            } else {
                newsym(sx, sy);
            }
        } else {
        nopushmsg:
            if (u.usteed)
                pline("%s tries to move %s, but cannot.",
                      upstart(y_monnam(u.usteed)), the(xname(otmp)));
            else
                You("try to move %s, but in vain.", the(xname(otmp)));
            if (Blind)
                feel_location(sx, sy);
        cannot_push:
            if (throws_rocks(youmonst.data)) {
                boolean
                    canpickup = (!Sokoban
                                 /* similar exception as in can_lift():
                                    when poly'd into a giant, you can
                                    pick up a boulder if you have a free
                                    slot or into the overflow ('#') slot
                                    unless already carrying at least one */
                              && (inv_cnt(FALSE) < 52 || !carrying(BOULDER))),
                    willpickup = (canpickup && autopick_testobj(otmp, TRUE));

                if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
                    You("aren't skilled enough to %s %s from %s.",
                        willpickup ? "pick up" : "push aside",
                        the(xname(otmp)), y_monnam(u.usteed));
                } else {
                    /*
                     * willpickup:  you easily pick it up
                     * canpickup:   you could easily pick it up
                     * otherwise:   you easily push it aside
                     */
                    pline("However, you %seasily %s.",
                          (willpickup || !canpickup) ? "" : "could ",
                          (willpickup || canpickup) ? "pick it up"
                                                    : "push it aside");
                    sokoban_guilt();
                    break;
                }
                break;
            }

            if (!u.usteed
                && (((!invent || inv_weight() <= -850)
                     && (!u.dx || !u.dy || (IS_ROCK(levl[u.ux][sy].typ)
                                            && IS_ROCK(levl[sx][u.uy].typ))))
                    || verysmall(youmonst.data))) {
                pline(
                   "However, you can squeeze yourself into a small opening.");
                sokoban_guilt();
                break;
            } else
                return -1;
        }
    }
    return 0;
}

/*
 *  still_chewing()
 *
 *  Chew on a wall, door, or boulder.  [What about statues?]
 *  Returns TRUE if still eating, FALSE when done.
 */
STATIC_OVL int
still_chewing(x, y)
xchar x, y;
{
    struct rm *lev = &levl[x][y];
    struct obj *boulder = sobj_at(BOULDER, x, y);
    const char *digtxt = (char *) 0, *dmgtxt = (char *) 0;

    if (context.digging.down) /* not continuing previous dig (w/ pick-axe) */
        (void) memset((genericptr_t) &context.digging, 0,
                      sizeof (struct dig_info));

    if (!boulder
        && ((IS_ROCK(lev->typ) && !may_dig(x, y))
            /* may_dig() checks W_NONDIGGABLE but doesn't handle iron bars */
            || (lev->typ == IRONBARS && (lev->wall_info & W_NONDIGGABLE)))) {
        You("hurt your teeth on the %s.",
            (lev->typ == IRONBARS)
                ? "bars"
                : IS_TREE(lev->typ)
                    ? "tree"
                    : "hard stone");
        nomul(0);
        return 1;
    } else if (context.digging.pos.x != x || context.digging.pos.y != y
               || !on_level(&context.digging.level, &u.uz)) {
        context.digging.down = FALSE;
        context.digging.chew = TRUE;
        context.digging.warned = FALSE;
        context.digging.pos.x = x;
        context.digging.pos.y = y;
        assign_level(&context.digging.level, &u.uz);
        /* solid rock takes more work & time to dig through */
        context.digging.effort =
            (IS_ROCK(lev->typ) && !IS_TREE(lev->typ) ? 30 : 60) + u.udaminc;
        You("start chewing %s %s.",
            (boulder || IS_TREE(lev->typ) || lev->typ == IRONBARS)
                ? "on a"
                : "a hole in the",
            boulder
                ? "boulder"
                : IS_TREE(lev->typ)
                    ? "tree"
                    : IS_ROCK(lev->typ)
                        ? "rock"
                        : (lev->typ == IRONBARS)
                            ? "bar"
                            : "door");
        watch_dig((struct monst *) 0, x, y, FALSE);
        return 1;
    } else if ((context.digging.effort += (30 + u.udaminc)) <= 100) {
        if (flags.verbose)
            You("%s chewing on the %s.",
                context.digging.chew ? "continue" : "begin",
                boulder
                    ? "boulder"
                    : IS_TREE(lev->typ)
                        ? "tree"
                        : IS_ROCK(lev->typ)
                            ? "rock"
                            : (lev->typ == IRONBARS)
                                ? "bars"
                                : "door");
        context.digging.chew = TRUE;
        watch_dig((struct monst *) 0, x, y, FALSE);
        return 1;
    }

    /* Okay, you've chewed through something */
    if(!u.uconduct.food++)
        livelog_printf(LL_CONDUCT, "ate for the first time, by chewing through %s",
                boulder
                    ? "a boulder"
                    : IS_TREE(lev->typ)
                        ? "a tree"
                        : IS_ROCK(lev->typ)
                            ? "rock"
                            : (lev->typ == IRONBARS)
                                ? "iron bars"
                                : "a door");

    u.uhunger += rnd(20);

    if (boulder) {
        delobj(boulder);         /* boulder goes bye-bye */
        You("eat the boulder."); /* yum */

        /*
         *  The location could still block because of
         *      1. More than one boulder
         *      2. Boulder stuck in a wall/stone/door.
         *
         *  [perhaps use does_block() below (from vision.c)]
         */
        if (IS_ROCK(lev->typ) || closed_door(x, y)
            || sobj_at(BOULDER, x, y)) {
            block_point(x, y); /* delobj will unblock the point */
            /* reset dig state */
            (void) memset((genericptr_t) &context.digging, 0,
                          sizeof (struct dig_info));
            return 1;
        }

    } else if (IS_WALL(lev->typ)) {
        if (*in_rooms(x, y, SHOPBASE)) {
            add_damage(x, y, SHOP_WALL_DMG);
            dmgtxt = "damage";
        }
        digtxt = "chew a hole in the wall.";
        if (level.flags.is_maze_lev) {
            lev->typ = ROOM;
        } else if (level.flags.is_cavernous_lev && !in_town(x, y)) {
            lev->typ = CORR;
        } else {
            lev->typ = DOOR;
            lev->doormask = D_NODOOR;
        }
    } else if (IS_TREE(lev->typ)) {
        digtxt = "chew through the tree.";
        lev->typ = ROOM;
    } else if (lev->typ == IRONBARS) {
        digtxt = "eat through the bars.";
        dissolve_bars(x, y);
    } else if (lev->typ == SDOOR) {
        if (lev->doormask & D_TRAPPED) {
            lev->doormask = D_NODOOR;
            b_trapped("secret door", 0);
        } else {
            digtxt = "chew through the secret door.";
            lev->doormask = D_BROKEN;
        }
        lev->typ = DOOR;

    } else if (IS_DOOR(lev->typ)) {
        if (*in_rooms(x, y, SHOPBASE)) {
            add_damage(x, y, SHOP_DOOR_COST);
            dmgtxt = "break";
        }
        if (lev->doormask & D_TRAPPED) {
            lev->doormask = D_NODOOR;
            b_trapped("door", 0);
        } else {
            digtxt = "chew through the door.";
            lev->doormask = D_BROKEN;
        }

    } else { /* STONE or SCORR */
        digtxt = "chew a passage through the rock.";
        lev->typ = CORR;
    }

    unblock_point(x, y); /* vision */
    newsym(x, y);
    if (digtxt)
        You1(digtxt); /* after newsym */
    if (dmgtxt)
        pay_for_damage(dmgtxt, FALSE);
    (void) memset((genericptr_t) &context.digging, 0,
                  sizeof (struct dig_info));
    return 0;
}

void
movobj(obj, ox, oy)
register struct obj *obj;
register xchar ox, oy;
{
    /* optimize by leaving on the fobj chain? */
    remove_object(obj);
    newsym(obj->ox, obj->oy);
    place_object(obj, ox, oy);
    newsym(ox, oy);
}

static NEARDATA const char fell_on_sink[] = "fell onto a sink";

STATIC_OVL void
dosinkfall()
{
    register struct obj *obj;
    int dmg;
    boolean lev_boots = (uarmf && uarmf->otyp == LEVITATION_BOOTS),
            innate_lev = ((HLevitation & (FROMOUTSIDE | FROMFORM)) != 0L),
            /* to handle being chained to buried iron ball, trying to
               levitate but being blocked, then moving onto adjacent sink;
               no need to worry about being blocked by terrain because we
               couldn't be over a sink at the same time */
            blockd_lev = (BLevitation == I_SPECIAL),
            ufall = (!innate_lev && !blockd_lev
                     && !(HFlying || EFlying)); /* BFlying */

    if (!ufall) {
        You((innate_lev || blockd_lev) ? "wobble unsteadily for a moment."
                                       : "gain control of your flight.");
    } else {
        long save_ELev = ELevitation, save_HLev = HLevitation;

        /* fake removal of levitation in advance so that final
           disclosure will be right in case this turns out to
           be fatal; fortunately the fact that rings and boots
           are really still worn has no effect on bones data */
        ELevitation = HLevitation = 0L;
        You("crash to the floor!");
        dmg = rn1(8, 25 - (int) ACURR(A_CON));
        losehp(Maybe_Half_Phys(dmg), fell_on_sink, NO_KILLER_PREFIX);
        exercise(A_DEX, FALSE);
        selftouch("Falling, you");
        for (obj = level.objects[u.ux][u.uy]; obj; obj = obj->nexthere)
            if (obj->oclass == WEAPON_CLASS || is_weptool(obj)) {
                You("fell on %s.", doname(obj));
                losehp(Maybe_Half_Phys(rnd(3)), fell_on_sink,
                       NO_KILLER_PREFIX);
                exercise(A_CON, FALSE);
            }
        ELevitation = save_ELev;
        HLevitation = save_HLev;
    }

    /*
     * Interrupt multi-turn putting on/taking off of armor (in which
     * case we reached the sink due to being teleported while busy;
     * in 3.4.3, Boots_on()/Boots_off() [called via (*afternmv)() when
     * 'multi' reaches 0] triggered a crash if we were donning/doffing
     * levitation boots [because the Boots_off() below causes 'uarmf'
     * to be null by the time 'afternmv' gets called]).
     *
     * Interrupt donning/doffing if we fall onto the sink, or if the
     * code below is going to remove levitation boots even when we
     * haven't fallen (innate floating or flying becoming unblocked).
     */
    if (ufall || lev_boots) {
        (void) stop_donning(lev_boots ? uarmf : (struct obj *) 0);
        /* recalculate in case uarmf just got set to null */
        lev_boots = (uarmf && uarmf->otyp == LEVITATION_BOOTS);
    }

    /* remove worn levitation items */
    ELevitation &= ~W_ARTI;
    HLevitation &= ~(I_SPECIAL | TIMEOUT);
    HLevitation++;
    if (uleft && uleft->otyp == RIN_LEVITATION) {
        obj = uleft;
        Ring_off(obj);
        off_msg(obj);
    }
    if (uright && uright->otyp == RIN_LEVITATION) {
        obj = uright;
        Ring_off(obj);
        off_msg(obj);
    }
    if (lev_boots) {
        obj = uarmf;
        (void) Boots_off();
        off_msg(obj);
    }
    HLevitation--;
    /* probably moot; we're either still levitating or went
       through float_down(), but make sure BFlying is up to date */
    float_vs_flight();
}

/* intended to be called only on ROCKs or TREEs */
boolean
may_dig(x, y)
register xchar x, y;
{
    struct rm *lev = &levl[x][y];

    return (boolean) !((IS_STWALL(lev->typ) || IS_TREE(lev->typ))
                       && (lev->wall_info & W_NONDIGGABLE));
}

boolean
may_passwall(x, y)
register xchar x, y;
{
    return (boolean) !(IS_STWALL(levl[x][y].typ)
                       && (levl[x][y].wall_info & W_NONPASSWALL));
}

boolean
bad_rock(mdat, x, y)
struct permonst *mdat;
register xchar x, y;
{
    return (boolean) ((Sokoban && sobj_at(BOULDER, x, y))
                      || (IS_ROCK(levl[x][y].typ)
                          && (!tunnels(mdat) || needspick(mdat)
                              || !may_dig(x, y))
                          && !(passes_walls(mdat) && may_passwall(x, y))));
}

/* caller has already decided that it's a tight diagonal; check whether a
   monster--who might be the hero--can fit through, and if not then return
   the reason why:  1: can't fit, 2: possessions won't fit, 3: sokoban
   returns 0 if we can squeeze through */
int
cant_squeeze_thru(mon)
struct monst *mon;
{
    int amt;
    struct permonst *ptr = mon->data;

    /* too big? */
    if (bigmonst(ptr)
        && !(amorphous(ptr) || is_whirly(ptr) || noncorporeal(ptr)
             || slithy(ptr) || can_fog(mon)))
        return 1;

    /* lugging too much junk? */
    amt = (mon == &youmonst) ? inv_weight() + weight_cap()
                             : curr_mon_load(mon);
    if (amt > 600)
        return 2;

    /* Sokoban restriction applies to hero only */
    if (mon == &youmonst && Sokoban)
        return 3;

    /* can squeeze through */
    return 0;
}

boolean
invocation_pos(x, y)
xchar x, y;
{
    return (boolean) (Invocation_lev(&u.uz)
                      && x == inv_pos.x && y == inv_pos.y);
}

/* return TRUE if (dx,dy) is an OK place to move
 * mode is one of DO_MOVE, TEST_MOVE, TEST_TRAV, or TEST_TRAP
 */
boolean
test_move(ux, uy, dx, dy, mode)
int ux, uy, dx, dy;
int mode;
{
    int x = ux + dx;
    int y = uy + dy;
    register struct rm *tmpr = &levl[x][y];
    register struct rm *ust;

    context.door_opened = FALSE;
    /*
     *  Check for physical obstacles.  First, the place we are going.
     */
    if (IS_ROCK(tmpr->typ) || tmpr->typ == IRONBARS) {
        if (Blind && mode == DO_MOVE)
            feel_location(x, y);
        if (Passes_walls && may_passwall(x, y)) {
            ; /* do nothing */
        } else if (Underwater) {
            /* note: if water_friction() changes direction due to
               turbulence, new target destination will always be water,
               so we won't get here, hence don't need to worry about
               "there" being somewhere the player isn't sure of */
            if (mode == DO_MOVE)
                pline("There is an obstacle there.");
            return FALSE;
        } else if (tmpr->typ == IRONBARS) {
            if ((dmgtype(youmonst.data, AD_RUST)
                 || dmgtype(youmonst.data, AD_CORR)) && mode == DO_MOVE
                && still_chewing(x, y)) {
                return FALSE;
            }
            if (!(Passes_walls || passes_bars(youmonst.data))) {
                if (mode == DO_MOVE && iflags.mention_walls)
                    You("cannot pass through the bars.");
                return FALSE;
            }
        } else if (tunnels(youmonst.data) && !needspick(youmonst.data)) {
            /* Eat the rock. */
            if (mode == DO_MOVE && still_chewing(x, y))
                return FALSE;
        } else if (flags.autodig && !context.run && !context.nopick && uwep
                   && is_pick(uwep)) {
            /* MRKR: Automatic digging when wielding the appropriate tool */
            if (mode == DO_MOVE)
                (void) use_pick_axe2(uwep);
            return FALSE;
        } else {
            if (mode == DO_MOVE) {
                if (is_db_wall(x, y))
                    pline("That drawbridge is up!");
                /* sokoban restriction stays even after puzzle is solved */
                else if (Passes_walls && !may_passwall(x, y)
                         && In_sokoban(&u.uz))
                    pline_The("Sokoban walls resist your ability.");
                else if (iflags.mention_walls)
                    pline("It's %s.",
                          (IS_WALL(tmpr->typ) || tmpr->typ == SDOOR) ? "a wall"
                          : IS_TREE(tmpr->typ) ? "a tree"
                          : "solid stone");
            }
            return FALSE;
        }
    } else if (IS_DOOR(tmpr->typ)) {
        if (closed_door(x, y)) {
            if (Blind && mode == DO_MOVE)
                feel_location(x, y);
            if (Passes_walls) {
                ; /* do nothing */
            } else if (can_ooze(&youmonst)) {
                if (mode == DO_MOVE)
                    You("ooze under the door.");
            } else if (Underwater) {
                if (mode == DO_MOVE)
                    pline("There is an obstacle there.");
                return FALSE;
            } else if (tunnels(youmonst.data) && !needspick(youmonst.data)) {
                /* Eat the door. */
                if (mode == DO_MOVE && still_chewing(x, y))
                    return FALSE;
            } else {
                if (mode == DO_MOVE) {
                    if (amorphous(youmonst.data))
                        You(
   "try to ooze under the door, but can't squeeze your possessions through.");
                    if (flags.autoopen && !context.run && !Confusion
                        && !Stunned && !Fumbling) {
                        context.door_opened = context.move =
                            doopen_indir(x, y);
                    } else if (x == ux || y == uy) {
                        if (Blind || Stunned || ACURR(A_DEX) < 10
                            || Fumbling) {
                            if (u.usteed) {
                                You_cant("lead %s through that closed door.",
                                         y_monnam(u.usteed));
                            } else {
                                pline("Ouch!  You bump into a door.");
                                exercise(A_DEX, FALSE);
                            }
                        } else
                            pline("That door is closed.");
                    }
                } else if (mode == TEST_TRAV || mode == TEST_TRAP)
                    goto testdiag;
                return FALSE;
            }
        } else {
        testdiag:
            if (dx && dy && !Passes_walls
                && (!doorless_door(x, y) || block_door(x, y))) {
                /* Diagonal moves into a door are not allowed. */
                if (mode == DO_MOVE) {
                    if (Blind)
                        feel_location(x, y);
                    if (Underwater || iflags.mention_walls)
                        You_cant("move diagonally into an intact doorway.");
                }
                return FALSE;
            }
        }
    }
    if (dx && dy && bad_rock(youmonst.data, ux, y)
        && bad_rock(youmonst.data, x, uy)) {
        /* Move at a diagonal. */
        switch (cant_squeeze_thru(&youmonst)) {
        case 3:
            if (mode == DO_MOVE)
                You("cannot pass that way.");
            return FALSE;
        case 2:
            if (mode == DO_MOVE)
                You("are carrying too much to get through.");
            return FALSE;
        case 1:
            if (mode == DO_MOVE)
                Your("body is too large to fit through.");
            return FALSE;
        default:
            break; /* can squeeze through */
        }
    } else if (dx && dy && worm_cross(ux, uy, x, y)) {
        /* consecutive long worm segments are at <ux,y> and <x,uy> */
        if (mode == DO_MOVE)
            pline("%s is in your way.", Monnam(m_at(ux, y)));
        return FALSE;
    }
    /* Pick travel path that does not require crossing a trap.
     * Avoid water and lava using the usual running rules.
     * (but not u.ux/u.uy because findtravelpath walks toward u.ux/u.uy) */
    if (context.run == 8 && (mode != DO_MOVE)
        && (x != u.ux || y != u.uy)) {
        struct trap *t = t_at(x, y);

        if ((t && t->tseen)
            || (!Levitation && !Flying && !is_clinger(youmonst.data)
                && is_pool_or_lava(x, y) && levl[x][y].seenv))
            return (mode == TEST_TRAP);
    }

    if (mode == TEST_TRAP)
        return FALSE; /* do not move through traps */

    ust = &levl[ux][uy];

    /* Now see if other things block our way . . */
    if (dx && dy && !Passes_walls && IS_DOOR(ust->typ)
        && (!doorless_door(ux, uy) || block_entry(x, y))) {
        /* Can't move at a diagonal out of a doorway with door. */
        if (mode == DO_MOVE && iflags.mention_walls)
            You_cant("move diagonally out of an intact doorway.");
        return FALSE;
    }

    if (sobj_at(BOULDER, x, y) && (Sokoban || !Passes_walls)) {
        if (!(Blind || Hallucination) && (context.run >= 2)
            && mode != TEST_TRAV) {
            if (mode == DO_MOVE && iflags.mention_walls)
                pline("A boulder blocks your path.");
            return FALSE;
        }
        if (mode == DO_MOVE) {
            /* tunneling monsters will chew before pushing */
            if (tunnels(youmonst.data) && !needspick(youmonst.data)
                && !Sokoban) {
                if (still_chewing(x, y))
                    return FALSE;
            } else if (moverock() < 0)
                return FALSE;
        } else if (mode == TEST_TRAV) {
            struct obj *obj;

            /* never travel through boulders in Sokoban */
            if (Sokoban)
                return FALSE;

            /* don't pick two boulders in a row, unless there's a way thru */
            if (sobj_at(BOULDER, ux, uy) && !Sokoban) {
                if (!Passes_walls
                    && !(tunnels(youmonst.data) && !needspick(youmonst.data))
                    && !carrying(PICK_AXE) && !carrying(DWARVISH_MATTOCK)
                    && !((obj = carrying(WAN_DIGGING))
                         && !objects[obj->otyp].oc_name_known))
                    return FALSE;
            }
        }
        /* assume you'll be able to push it when you get there... */
    }

    /* OK, it is a legal place to move. */
    return TRUE;
}

/*
 * Find a path from the destination (u.tx,u.ty) back to (u.ux,u.uy).
 * A shortest path is returned.  If guess is TRUE, consider various
 * inaccessible locations as valid intermediate path points.
 * Returns TRUE if a path was found.
 */
STATIC_OVL boolean
findtravelpath(mode)
int mode;
{
    /* if travel to adjacent, reachable location, use normal movement rules */
    if ((mode == TRAVP_TRAVEL || mode == TRAVP_VALID) && context.travel1
        && distmin(u.ux, u.uy, u.tx, u.ty) == 1
        && !(u.ux != u.tx && u.uy != u.ty && NODIAG(u.umonnum))) {
        context.run = 0;
        if (test_move(u.ux, u.uy, u.tx - u.ux, u.ty - u.uy, TEST_MOVE)) {
            if (mode == TRAVP_TRAVEL) {
                u.dx = u.tx - u.ux;
                u.dy = u.ty - u.uy;
                nomul(0);
                iflags.travelcc.x = iflags.travelcc.y = -1;
            }
            return TRUE;
        }
        if (mode == TRAVP_TRAVEL)
            context.run = 8;
    }
    if (u.tx != u.ux || u.ty != u.uy) {
        xchar travel[COLNO][ROWNO];
        xchar travelstepx[2][COLNO * ROWNO];
        xchar travelstepy[2][COLNO * ROWNO];
        xchar tx, ty, ux, uy;
        int n = 1;      /* max offset in travelsteps */
        int set = 0;    /* two sets current and previous */
        int radius = 1; /* search radius */
        int i;

        /* If guessing, first find an "obvious" goal location.  The obvious
         * goal is the position the player knows of, or might figure out
         * (couldsee) that is closest to the target on a straight path.
         */
        if (mode == TRAVP_GUESS || mode == TRAVP_VALID) {
            tx = u.ux;
            ty = u.uy;
            ux = u.tx;
            uy = u.ty;
        } else {
            tx = u.tx;
            ty = u.ty;
            ux = u.ux;
            uy = u.uy;
        }

    noguess:
        (void) memset((genericptr_t) travel, 0, sizeof(travel));
        travelstepx[0][0] = tx;
        travelstepy[0][0] = ty;

        while (n != 0) {
            int nn = 0;

            for (i = 0; i < n; i++) {
                int dir;
                int x = travelstepx[set][i];
                int y = travelstepy[set][i];
                static int ordered[] = { 0, 2, 4, 6, 1, 3, 5, 7 };
                /* no diagonal movement for grid bugs */
                int dirmax = NODIAG(u.umonnum) ? 4 : 8;
                boolean alreadyrepeated = FALSE;

                for (dir = 0; dir < dirmax; ++dir) {
                    int nx = x + xdir[ordered[dir]];
                    int ny = y + ydir[ordered[dir]];

                    /*
                     * When guessing and trying to travel as close as possible
                     * to an unreachable target space, don't include spaces
                     * that would never be picked as a guessed target in the
                     * travel matrix describing hero-reachable spaces.
                     * This stops travel from getting confused and moving
                     * the hero back and forth in certain degenerate
                     * configurations of sight-blocking obstacles, e.g.
                     *
                     *  T         1. Dig this out and carry enough to not be
                     *   ####       able to squeeze through diagonal gaps.
                     *   #--.---    Stand at @ and target travel at space T.
                     *    @.....
                     *    |.....
                     *
                     *  T         2. couldsee() marks spaces marked a and x
                     *   ####       as eligible guess spaces to move the hero
                     *   a--.---    towards.  Space a is closest to T, so it
                     *    @xxxxx    gets chosen.  Travel system moves @ right
                     *    |xxxxx    to travel to space a.
                     *
                     *  T         3. couldsee() marks spaces marked b, c and x
                     *   ####       as eligible guess spaces to move the hero
                     *   a--c---    towards.  Since findtravelpath() is called
                     *    b@xxxx    repeatedly during travel, it doesn't
                     *    |xxxxx    remember that it wanted to go to space a,
                     *              so in comparing spaces b and c, b is
                     *              chosen, since it seems like the closest
                     *              eligible space to T. Travel system moves @
                     *              left to go to space b.
                     *
                     *            4. Go to 2.
                     *
                     * By limiting the travel matrix here, space a in the
                     * example above is never included in it, preventing
                     * the cycle.
                     */
                    if (!isok(nx, ny)
                        || ((mode == TRAVP_GUESS) && !couldsee(nx, ny)))
                        continue;
                    if ((!Passes_walls && !can_ooze(&youmonst)
                         && closed_door(x, y)) || sobj_at(BOULDER, x, y)
                        || test_move(x, y, nx-x, ny-y, TEST_TRAP)) {
                        /* closed doors and boulders usually
                         * cause a delay, so prefer another path */
                        if (travel[x][y] > radius - 3) {
                            if (!alreadyrepeated) {
                                travelstepx[1 - set][nn] = x;
                                travelstepy[1 - set][nn] = y;
                                /* don't change travel matrix! */
                                nn++;
                                alreadyrepeated = TRUE;
                            }
                            continue;
                        }
                    }
                    if (test_move(x, y, nx - x, ny - y, TEST_TRAV)
                        && (levl[nx][ny].seenv
                            || (!Blind && couldsee(nx, ny)))) {
                        if (nx == ux && ny == uy) {
                            if (mode == TRAVP_TRAVEL || mode == TRAVP_VALID) {
                                u.dx = x - ux;
                                u.dy = y - uy;
                                if (mode == TRAVP_TRAVEL
                                    && x == u.tx && y == u.ty) {
                                    nomul(0);
                                    /* reset run so domove run checks work */
                                    context.run = 8;
                                    iflags.travelcc.x = iflags.travelcc.y = -1;
                                }
                                return TRUE;
                            }
                        } else if (!travel[nx][ny]) {
                            travelstepx[1 - set][nn] = nx;
                            travelstepy[1 - set][nn] = ny;
                            travel[nx][ny] = radius;
                            nn++;
                        }
                    }
                }
            }

#ifdef DEBUG
            if (iflags.trav_debug) {
                /* Use of warning glyph is arbitrary. It stands out. */
                tmp_at(DISP_ALL, warning_to_glyph(1));
                for (i = 0; i < nn; ++i) {
                    tmp_at(travelstepx[1 - set][i], travelstepy[1 - set][i]);
                }
                delay_output();
                if (flags.runmode == RUN_CRAWL) {
                    delay_output();
                    delay_output();
                }
                tmp_at(DISP_END, 0);
            }
#endif /* DEBUG */

            n = nn;
            set = 1 - set;
            radius++;
        }

        /* if guessing, find best location in travel matrix and go there */
        if (mode == TRAVP_GUESS) {
            int px = tx, py = ty; /* pick location */
            int dist, nxtdist, d2, nd2;

            dist = distmin(ux, uy, tx, ty);
            d2 = dist2(ux, uy, tx, ty);
            for (tx = 1; tx < COLNO; ++tx)
                for (ty = 0; ty < ROWNO; ++ty)
                    if (travel[tx][ty]) {
                        nxtdist = distmin(ux, uy, tx, ty);
                        if (nxtdist == dist && couldsee(tx, ty)) {
                            nd2 = dist2(ux, uy, tx, ty);
                            if (nd2 < d2) {
                                /* prefer non-zigzag path */
                                px = tx;
                                py = ty;
                                d2 = nd2;
                            }
                        } else if (nxtdist < dist && couldsee(tx, ty)) {
                            px = tx;
                            py = ty;
                            dist = nxtdist;
                            d2 = dist2(ux, uy, tx, ty);
                        }
                    }

            if (px == u.ux && py == u.uy) {
                /* no guesses, just go in the general direction */
                u.dx = sgn(u.tx - u.ux);
                u.dy = sgn(u.ty - u.uy);
                if (test_move(u.ux, u.uy, u.dx, u.dy, TEST_MOVE))
                    return TRUE;
                goto found;
            }
#ifdef DEBUG
            if (iflags.trav_debug) {
                /* Use of warning glyph is arbitrary. It stands out. */
                tmp_at(DISP_ALL, warning_to_glyph(2));
                tmp_at(px, py);
                delay_output();
                if (flags.runmode == RUN_CRAWL) {
                    delay_output();
                    delay_output();
                    delay_output();
                    delay_output();
                }
                tmp_at(DISP_END, 0);
            }
#endif /* DEBUG */
            tx = px;
            ty = py;
            ux = u.ux;
            uy = u.uy;
            set = 0;
            n = radius = 1;
            mode = TRAVP_TRAVEL;
            goto noguess;
        }
        return FALSE;
    }

found:
    u.dx = 0;
    u.dy = 0;
    nomul(0);
    return FALSE;
}

boolean
is_valid_travelpt(x,y)
int x,y;
{
    int tx = u.tx;
    int ty = u.ty;
    boolean ret;
    int g = glyph_at(x,y);
    if (x == u.ux && y == u.uy)
        return TRUE;
    if (isok(x,y) && glyph_is_cmap(g) && S_stone == glyph_to_cmap(g)
        && !levl[x][y].seenv)
        return FALSE;
    u.tx = x;
    u.ty = y;
    ret = findtravelpath(TRAVP_VALID);
    u.tx = tx;
    u.ty = ty;
    return ret;
}

/* try to escape being stuck in a trapped state by walking out of it;
   return true iff moving should continue to intended destination
   (all failures and most successful escapes leave hero at original spot) */
STATIC_OVL boolean
trapmove(x, y, desttrap)
int x, y;              /* targetted destination, <u.ux+u.dx,u.uy+u.dy> */
struct trap *desttrap; /* nonnull if another trap at <x,y> */
{
    boolean anchored = FALSE;
    const char *predicament, *culprit;
    char *steedname = !u.usteed ? (char *) 0 : y_monnam(u.usteed);

    if (!u.utrap)
        return TRUE; /* sanity check */

    /*
     * Note: caller should call reset_utrap() when we set u.utrap to 0.
     */

    switch (u.utraptype) {
    case TT_BEARTRAP:
        if (flags.verbose) {
            predicament = "caught in a bear trap";
            if (u.usteed)
                Norep("%s is %s.", upstart(steedname), predicament);
            else
                Norep("You are %s.", predicament);
        }
        /* [why does diagonal movement give quickest escape?] */
        if ((u.dx && u.dy) || !rn2(5))
            u.utrap--;
        if (!u.utrap)
            goto wriggle_free;
        break;
    case TT_PIT:
        if (desttrap && desttrap->tseen
            && is_pit(desttrap->ttyp))
            return TRUE; /* move into adjacent pit */
        /* try to escape; position stays same regardless of success */
        climb_pit();
        break;
    case TT_WEB:
        if (uwep && uwep->oartifact == ART_STING) {
            /* escape trap but don't move and don't destroy it */
            u.utrap = 0; /* caller will call reset_utrap() */
            pline("Sting cuts through the web!");
            break;
        }
        if (--u.utrap) {
            if (flags.verbose) {
                predicament = "stuck to the web";
                if (u.usteed)
                    Norep("%s is %s.", upstart(steedname), predicament);
                else
                    Norep("You are %s.", predicament);
            }
        } else {
            if (u.usteed)
                pline("%s breaks out of the web.", upstart(steedname));
            else
                You("disentangle yourself.");
        }
        break;
    case TT_LAVA:
        if (flags.verbose) {
            predicament = "stuck in the lava";
            if (u.usteed)
                Norep("%s is %s.", upstart(steedname), predicament);
            else
                Norep("You are %s.", predicament);
        }
        if (!is_lava(x, y)) {
            u.utrap--;
            if ((u.utrap & 0xff) == 0) {
                u.utrap = 0;
                if (u.usteed)
                    You("lead %s to the edge of the %s.", steedname,
                        hliquid("lava"));
                else
                    You("pull yourself to the edge of the %s.",
                        hliquid("lava"));
            }
        }
        u.umoved = TRUE;
        break;
    case TT_INFLOOR:
    case TT_BURIEDBALL:
        anchored = (u.utraptype == TT_BURIEDBALL);
        if (anchored) {
            coord cc;

            cc.x = u.ux, cc.y = u.uy;
            /* can move normally within radius 1 of buried ball */
            if (buried_ball(&cc) && dist2(x, y, cc.x, cc.y) <= 2) {
                /* ugly hack: we need to issue some message here
                   in case "you are chained to the buried ball"
                   was the most recent message given, otherwise
                   our next attempt to move out of tether range
                   after this successful move would have its
                   can't-do-that message suppressed by Norep */
                if (flags.verbose)
                    Norep("You move within the chain's reach.");
                return TRUE;
            }
        }
        if (--u.utrap) {
            if (flags.verbose) {
                if (anchored) {
                    predicament = "chained to the";
                    culprit = "buried ball";
                } else {
                    predicament = "stuck in the";
                    culprit = surface(u.ux, u.uy);
                }
                if (u.usteed) {
                    if (anchored)
                        Norep("You and %s are %s %s.", steedname, predicament,
                              culprit);
                    else
                        Norep("%s is %s %s.", upstart(steedname), predicament,
                              culprit);
                } else
                    Norep("You are %s %s.", predicament, culprit);
            }
        } else {
wriggle_free:
            if (u.usteed)
                pline("%s finally %s free.", upstart(steedname),
                      !anchored ? "lurches" : "wrenches the ball");
            else
                You("finally %s free.",
                    !anchored ? "wriggle" : "wrench the ball");
            if (anchored)
                buried_ball_to_punishment();
        }
        break;
    default:
        impossible("trapmove: stuck in unknown trap? (%d)",
                   (int) u.utraptype);
        break;
    }
    return FALSE;
}

boolean
u_rooted()
{
    if (!youmonst.data->mmove) {
        You("are rooted %s.",
            Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)
                ? "in place"
                : "to the ground");
        nomul(0);
        return TRUE;
    }
    return FALSE;
}

void
domove()
{
    register struct monst *mtmp;
    register struct rm *tmpr;
    register xchar x, y;
    struct trap *trap = NULL;
    int wtcap;
    boolean on_ice;
    xchar chainx = 0, chainy = 0,
          ballx = 0, bally = 0;         /* ball&chain new positions */
    int bc_control = 0;                 /* control for ball&chain */
    boolean cause_delay = FALSE;        /* dragging ball will skip a move */

    u_wipe_engr(rnd(5));

    if (context.travel) {
        if (!findtravelpath(FALSE))
            (void) findtravelpath(TRUE);
        context.travel1 = 0;
    }

    if (((wtcap = near_capacity()) >= OVERLOADED
         || (wtcap > SLT_ENCUMBER
             && (Upolyd ? (u.mh < 5 && u.mh != u.mhmax)
                        : (u.uhp < 10 && u.uhp != u.uhpmax))))
        && !Is_airlevel(&u.uz)) {
        if (wtcap < OVERLOADED) {
            You("don't have enough stamina to move.");
            exercise(A_CON, FALSE);
        } else
            You("collapse under your load.");
        nomul(0);
        return;
    }
    if (u.uswallow) {
        u.dx = u.dy = 0;
        u.ux = x = u.ustuck->mx;
        u.uy = y = u.ustuck->my;
        mtmp = u.ustuck;
    } else {
        if (Is_airlevel(&u.uz) && rn2(4) && !Levitation && !Flying) {
            switch (rn2(3)) {
            case 0:
                You("tumble in place.");
                exercise(A_DEX, FALSE);
                break;
            case 1:
                You_cant("control your movements very well.");
                break;
            case 2:
                pline("It's hard to walk in thin air.");
                exercise(A_DEX, TRUE);
                break;
            }
            return;
        }

        /* check slippery ice */
        on_ice = !Levitation && is_ice(u.ux, u.uy);
        if (on_ice) {
            static int skates = 0;

            if (!skates)
                skates = find_skates();
            if ((uarmf && uarmf->otyp == skates) || resists_cold(&youmonst)
                || Flying || is_floater(youmonst.data)
                || is_clinger(youmonst.data) || is_whirly(youmonst.data)) {
                on_ice = FALSE;
            } else if (!rn2(Cold_resistance ? 3 : 2)) {
                HFumbling |= FROMOUTSIDE;
                HFumbling &= ~TIMEOUT;
                HFumbling += 1; /* slip on next move */
            }
        }
        if (!on_ice && (HFumbling & FROMOUTSIDE))
            HFumbling &= ~FROMOUTSIDE;

        x = u.ux + u.dx;
        y = u.uy + u.dy;
        if (Stunned || (Confusion && !rn2(5))) {
            register int tries = 0;

            do {
                if (tries++ > 50) {
                    nomul(0);
                    return;
                }
                confdir();
                x = u.ux + u.dx;
                y = u.uy + u.dy;
            } while (!isok(x, y) || bad_rock(youmonst.data, x, y));
        }
        /* turbulence might alter your actual destination */
        if (u.uinwater) {
            water_friction();
            if (!u.dx && !u.dy) {
                nomul(0);
                return;
            }
            x = u.ux + u.dx;
            y = u.uy + u.dy;
        }
        if (!isok(x, y)) {
            nomul(0);
            return;
        }
        if (((trap = t_at(x, y)) && trap->tseen)
            || (Blind && !Levitation && !Flying && !is_clinger(youmonst.data)
                && is_pool_or_lava(x, y) && levl[x][y].seenv)) {
            if (context.run >= 2) {
                if (iflags.mention_walls) {
                    if (trap && trap->tseen) {
                        int tt = what_trap(trap->ttyp);
                        You("stop in front of %s.",
                            an(defsyms[trap_to_defsym(tt)].explanation));
                    } else if (is_pool_or_lava(x,y) && levl[x][y].seenv) {
                        You("stop at the edge of the %s.",
                            hliquid(is_pool(x,y) ? "water" : "lava"));
                    }
                }
                nomul(0);
                context.move = 0;
                return;
            } else
                nomul(0);
        }

        if (u.ustuck && (x != u.ustuck->mx || y != u.ustuck->my)) {
            if (distu(u.ustuck->mx, u.ustuck->my) > 2) {
                /* perhaps it fled (or was teleported or ... ) */
                u.ustuck = 0;
            } else if (sticks(youmonst.data)) {
                /* When polymorphed into a sticking monster,
                 * u.ustuck means it's stuck to you, not you to it.
                 */
                You("release %s.", mon_nam(u.ustuck));
                u.ustuck = 0;
            } else {
                /* If holder is asleep or paralyzed:
                 *      37.5% chance of getting away,
                 *      12.5% chance of waking/releasing it;
                 * otherwise:
                 *       7.5% chance of getting away.
                 * [strength ought to be a factor]
                 * If holder is tame and there is no conflict,
                 * guaranteed escape.
                 */
                switch (rn2(!u.ustuck->mcanmove ? 8 : 40)) {
                case 0:
                case 1:
                case 2:
                pull_free:
                    You("pull free from %s.", mon_nam(u.ustuck));
                    u.ustuck = 0;
                    break;
                case 3:
                    if (!u.ustuck->mcanmove) {
                        /* it's free to move on next turn */
                        u.ustuck->mfrozen = 1;
                        u.ustuck->msleeping = 0;
                    }
                /*FALLTHRU*/
                default:
                    if (u.ustuck->mtame && !Conflict && !u.ustuck->mconf)
                        goto pull_free;
                    You("cannot escape from %s!", mon_nam(u.ustuck));
                    nomul(0);
                    return;
                }
            }
        }

        mtmp = m_at(x, y);
        if (mtmp && !is_safepet(mtmp)) {
            /* Don't attack if you're running, and can see it */
            /* It's fine to displace pets, though */
            /* We should never get here if forcefight */
            if (context.run && ((!Blind && mon_visible(mtmp)
                                 && ((mtmp->m_ap_type != M_AP_FURNITURE
                                      && mtmp->m_ap_type != M_AP_OBJECT)
                                     || Protection_from_shape_changers))
                                || sensemon(mtmp))) {
                nomul(0);
                context.move = 0;
                return;
            }
        }
    }

    u.ux0 = u.ux;
    u.uy0 = u.uy;
    bhitpos.x = x;
    bhitpos.y = y;
    tmpr = &levl[x][y];

    /* attack monster */
    if (mtmp) {
        /* don't stop travel when displacing pets; if the
           displace fails for some reason, attack() in uhitm.c
           will stop travel rather than domove */
        if (!is_safepet(mtmp) || context.forcefight)
            nomul(0);
        /* only attack if we know it's there */
        /* or if we used the 'F' command to fight blindly */
        /* or if it hides_under, in which case we call attack() to print
         * the Wait! message.
         * This is different from ceiling hiders, who aren't handled in
         * attack().
         */

        /* If they used a 'm' command, trying to move onto a monster
         * prints the below message and wastes a turn.  The exception is
         * if the monster is unseen and the player doesn't remember an
         * invisible monster--then, we fall through to attack() and
         * attack_check(), which still wastes a turn, but prints a
         * different message and makes the player remember the monster.
         */
        if (context.nopick && !context.travel
            && (canspotmon(mtmp) || glyph_is_invisible(levl[x][y].glyph))) {
            if (mtmp->m_ap_type && !Protection_from_shape_changers
                && !sensemon(mtmp))
                stumble_onto_mimic(mtmp);
            else if (mtmp->mpeaceful && !Hallucination)
                /* m_monnam(): "dog" or "Fido", no "invisible dog" or "it" */
                pline("Pardon me, %s.", m_monnam(mtmp));
            else
                You("move right into %s.", mon_nam(mtmp));
            return;
        }
        if (context.forcefight || !mtmp->mundetected || sensemon(mtmp)
            || ((hides_under(mtmp->data) || mtmp->data->mlet == S_EEL)
                && !is_safepet(mtmp))) {
            /* try to attack; note that it might evade */
            /* also, we don't attack tame when _safepet_ */
            if (attack(mtmp))
                return;
        }
    }

    if (context.forcefight && levl[x][y].typ == IRONBARS && uwep) {
        struct obj *obj = uwep;

        if (breaktest(obj)) {
            if (obj->quan > 1L)
                obj = splitobj(obj, 1L);
            else
                setuwep((struct obj *)0);
            freeinv(obj);
        }
        hit_bars(&obj, u.ux, u.uy, x, y, TRUE, TRUE);
        return;
    }

    /* specifying 'F' with no monster wastes a turn */
    if (context.forcefight
        /* remembered an 'I' && didn't use a move command */
        || (glyph_is_invisible(levl[x][y].glyph) && !context.nopick)) {
        struct obj *boulder = 0;
        boolean explo = (Upolyd && attacktype(youmonst.data, AT_EXPL)),
                solid = !accessible(x, y);
        int glyph = glyph_at(x, y); /* might be monster */
        char buf[BUFSZ];

        if (!Underwater) {
            boulder = sobj_at(BOULDER, x, y);
            /* if a statue is displayed at the target location,
               player is attempting to attack it [and boulder
               handling below is suitable for handling that] */
            if (glyph_is_statue(glyph)
                || (Hallucination && glyph_is_monster(glyph)))
                boulder = sobj_at(STATUE, x, y);

            /* force fight at boulder/statue or wall/door while wielding
               pick:  start digging to break the boulder or wall */
            if (context.forcefight
                /* can we dig? */
                && uwep && dig_typ(uwep, x, y)
                /* should we dig? */
                && !glyph_is_invisible(glyph) && !glyph_is_monster(glyph)) {
                (void) use_pick_axe2(uwep);
                return;
            }
        }

        /* about to become known empty -- remove 'I' if present */
        unmap_object(x, y);
        if (boulder)
            map_object(boulder, TRUE);
        newsym(x, y);
        glyph = glyph_at(x, y); /* might have just changed */

        if (boulder) {
            Strcpy(buf, ansimpleoname(boulder));
        } else if (Underwater && !is_pool(x, y)) {
            /* Underwater, targetting non-water; the map just shows blank
               because you don't see remembered terrain while underwater;
               although the hero can attack an adjacent monster this way,
               assume he can't reach out far enough to distinguish terrain */
            Sprintf(buf, (Is_waterlevel(&u.uz) && levl[x][y].typ == AIR)
                             ? "an air bubble"
                             : "nothing");
        } else if (solid) {
            /* glyph might indicate unseen terrain if hero is blind;
               unlike searching, this won't reveal what that terrain is
               (except for solid rock, where the glyph would otherwise
               yield ludicrous "dark part of a room") */
            Strcpy(buf, (levl[x][y].typ == STONE) ? "solid rock"
                         : glyph_is_cmap(glyph)
                            ? the(defsyms[glyph_to_cmap(glyph)].explanation)
                            : (const char *) "an unknown obstacle");
            /* note: 'solid' is misleadingly named and catches pools
               of water and lava as well as rock and walls */
        } else {
            Strcpy(buf, "thin air");
        }
        You("%s%s %s.",
            !(boulder || solid) ? "" : !explo ? "harmlessly " : "futilely ",
            explo ? "explode at" : "attack", buf);

        nomul(0);
        if (explo) {
            wake_nearby();
            u.mh = -1; /* dead in the current form */
            rehumanize();
        }
        return;
    }
    (void) unmap_invisible(x, y);
    /* not attacking an animal, so we try to move */
    if ((u.dx || u.dy) && u.usteed && stucksteed(FALSE)) {
        nomul(0);
        return;
    }

    if (u_rooted())
        return;

    if (u.utrap) {
        boolean moved = trapmove(x, y, trap);

        if (!u.utrap)
            reset_utrap(TRUE); /* might resume levitation or flight */
        /* might not have escaped, or did escape but remain in same spot */
        if (!moved)
            return;
    }

    if (!test_move(u.ux, u.uy, x - u.ux, y - u.uy, DO_MOVE)) {
        if (!context.door_opened) {
            context.move = 0;
            nomul(0);
        }
        return;
    }

    /* Move ball and chain.  */
    if (Punished)
        if (!drag_ball(x, y, &bc_control, &ballx, &bally, &chainx, &chainy,
                       &cause_delay, TRUE))
            return;

    /* Check regions entering/leaving */
    if (!in_out_region(x, y))
        return;

    /* now move the hero */
    mtmp = m_at(x, y);
    u.ux += u.dx;
    u.uy += u.dy;
    /* Move your steed, too */
    if (u.usteed) {
        u.usteed->mx = u.ux;
        u.usteed->my = u.uy;
        exercise_steed();
    }

    /*
     * If safepet at destination then move the pet to the hero's
     * previous location using the same conditions as in attack().
     * there are special extenuating circumstances:
     * (1) if the pet dies then your god angers,
     * (2) if the pet gets trapped then your god may disapprove,
     * (3) if the pet was already trapped and you attempt to free it
     * not only do you encounter the trap but you may frighten your
     * pet causing it to go wild!  moral: don't abuse this privilege.
     *
     * Ceiling-hiding pets are skipped by this section of code, to
     * be caught by the normal falling-monster code.
     */
    if (is_safepet(mtmp) && !(is_hider(mtmp->data) && mtmp->mundetected)) {
        /* if trapped, there's a chance the pet goes wild */
        if (mtmp->mtrapped) {
            if (!rn2(mtmp->mtame)) {
                mtmp->mtame = mtmp->mpeaceful = mtmp->msleeping = 0;
                if (mtmp->mleashed)
                    m_unleash(mtmp, TRUE);
                growl(mtmp);
            } else {
                yelp(mtmp);
            }
        }

        /* seemimic/newsym should be done before moving hero, otherwise
           the display code will draw the hero here before we possibly
           cancel the swap below (we can ignore steed mx,my here) */
        u.ux = u.ux0, u.uy = u.uy0;
        mtmp->mundetected = 0;
        if (mtmp->m_ap_type)
            seemimic(mtmp);
        else if (!mtmp->mtame)
            newsym(mtmp->mx, mtmp->my);
        u.ux = mtmp->mx, u.uy = mtmp->my; /* resume swapping positions */

        if (mtmp->mtrapped && (trap = t_at(mtmp->mx, mtmp->my)) != 0
            && is_pit(trap->ttyp)
            && sobj_at(BOULDER, trap->tx, trap->ty)) {
            /* can't swap places with pet pinned in a pit by a boulder */
            u.ux = u.ux0, u.uy = u.uy0; /* didn't move after all */
            if (u.usteed)
                u.usteed->mx = u.ux, u.usteed->my = u.uy;
        } else if (u.ux0 != x && u.uy0 != y && NODIAG(mtmp->data - mons)) {
            /* can't swap places when pet can't move to your spot */
            u.ux = u.ux0, u.uy = u.uy0;
            if (u.usteed)
                u.usteed->mx = u.ux, u.usteed->my = u.uy;
            You("stop.  %s can't move diagonally.", upstart(y_monnam(mtmp)));
        } else if (u.ux0 != x && u.uy0 != y && bad_rock(mtmp->data, x, u.uy0)
                   && bad_rock(mtmp->data, u.ux0, y)
                   && (bigmonst(mtmp->data) || (curr_mon_load(mtmp) > 600))) {
            /* can't swap places when pet won't fit thru the opening */
            u.ux = u.ux0, u.uy = u.uy0; /* didn't move after all */
            if (u.usteed)
                u.usteed->mx = u.ux, u.usteed->my = u.uy;
            You("stop.  %s won't fit through.", upstart(y_monnam(mtmp)));
        } else {
            char pnambuf[BUFSZ];

            /* save its current description in case of polymorph */
            Strcpy(pnambuf, y_monnam(mtmp));
            mtmp->mtrapped = 0;
            remove_monster(x, y);
            place_monster(mtmp, u.ux0, u.uy0);
            newsym(x, y);
            newsym(u.ux0, u.uy0);

            You("%s %s.", mtmp->mtame ? "swap places with" : "frighten",
                pnambuf);

            /* check for displacing it into pools and traps */
            switch (minliquid(mtmp) ? 2 : mintrap(mtmp)) {
            case 0:
                break;
            case 1: /* trapped */
            case 3: /* changed levels */
                /* there's already been a trap message, reinforce it */
                abuse_dog(mtmp);
                adjalign(-3);
                break;
            case 2:
                /* drowned or died...
                 * you killed your pet by direct action, so get experience
                 * and possibly penalties;
                 * we want the level gain message, if it happens, to occur
                 * before the guilt message below
                 */
                {
                    /* minliquid() and mintrap() call mondead() rather than
                       killed() so we duplicate some of the latter here */
                    int tmp, mndx;

                    if(!u.uconduct.killer++)
                        livelog_write_string (LL_CONDUCT, "killed for the first time");
                    mndx = monsndx(mtmp->data);
                    tmp = experience(mtmp, (int) mvitals[mndx].died);
                    more_experienced(tmp, 0);
                    newexplevel(); /* will decide if you go up */
                }
                /* That's no way to treat a pet!  Your god gets angry.
                 *
                 * [This has always been pretty iffy.  Why does your
                 * patron deity care at all, let alone enough to get mad?]
                 */
                if (rn2(4)) {
                    You_feel("guilty about losing your pet like this.");
                    u.ugangr++;
                    adjalign(-15);
                }
                break;
            default:
                pline("that's strange, unknown mintrap result!");
                break;
            }
        }
    }

    reset_occupations();
    if (context.run) {
        if (context.run < 8)
            if (IS_DOOR(tmpr->typ) || IS_ROCK(tmpr->typ)
                || IS_FURNITURE(tmpr->typ))
                nomul(0);
    }

    if (hides_under(youmonst.data) || youmonst.data->mlet == S_EEL
        || u.dx || u.dy)
        (void) hideunder(&youmonst);

    /*
     * Mimics (or whatever) become noticeable if they move and are
     * imitating something that doesn't move.  We could extend this
     * to non-moving monsters...
     */
    if ((u.dx || u.dy) && (youmonst.m_ap_type == M_AP_OBJECT
                           || youmonst.m_ap_type == M_AP_FURNITURE))
        youmonst.m_ap_type = M_AP_NOTHING;

    check_leash(u.ux0, u.uy0);

    if (u.ux0 != u.ux || u.uy0 != u.uy) {
        u.umoved = TRUE;
        /* Clean old position -- vision_recalc() will print our new one. */
        newsym(u.ux0, u.uy0);
        /* Since the hero has moved, adjust what can be seen/unseen. */
        vision_recalc(1); /* Do the work now in the recover time. */
        invocation_message();
    }

    if (Punished) /* put back ball and chain */
        move_bc(0, bc_control, ballx, bally, chainx, chainy);

    if (u.umoved)
        spoteffects(TRUE);

    /* delay next move because of ball dragging */
    /* must come after we finished picking up, in spoteffects() */
    if (cause_delay) {
        nomul(-2);
        multi_reason = "dragging an iron ball";
        nomovemsg = "";
    }

    if (context.run && flags.runmode != RUN_TPORT) {
        /* display every step or every 7th step depending upon mode */
        if (flags.runmode != RUN_LEAP || !(moves % 7L)) {
            if (flags.time)
                context.botl = 1;
            curs_on_u();
            delay_output();
            if (flags.runmode == RUN_CRAWL) {
                delay_output();
                delay_output();
                delay_output();
                delay_output();
            }
        }
    }
}

/* combat increases metabolism */
boolean
overexertion()
{
    /* this used to be part of domove() when moving to a monster's
       position, but is now called by attack() so that it doesn't
       execute if you decline to attack a peaceful monster */
    gethungry();
    if ((moves % 3L) != 0L && near_capacity() >= HVY_ENCUMBER) {
        int *hp = (!Upolyd ? &u.uhp : &u.mh);

        if (*hp > 1) {
            *hp -= 1;
        } else {
            You("pass out from exertion!");
            exercise(A_CON, FALSE);
            fall_asleep(-10, FALSE);
        }
    }
    return (boolean) (multi < 0); /* might have fainted (forced to sleep) */
}

void
invocation_message()
{
    /* a special clue-msg when on the Invocation position */
    if (invocation_pos(u.ux, u.uy) && !On_stairs(u.ux, u.uy)) {
        char buf[BUFSZ];
        struct obj *otmp = carrying(CANDELABRUM_OF_INVOCATION);

        nomul(0); /* stop running or travelling */
        if (u.usteed)
            Sprintf(buf, "beneath %s", y_monnam(u.usteed));
        else if (Levitation || Flying)
            Strcpy(buf, "beneath you");
        else
            Sprintf(buf, "under your %s", makeplural(body_part(FOOT)));

        You_feel("a strange vibration %s.", buf);
        u.uevent.uvibrated = 1;
        if (otmp && otmp->spe == 7 && otmp->lamplit)
            pline("%s %s!", The(xname(otmp)),
                  Blind ? "throbs palpably" : "glows with a strange light");
    }
}

/* moving onto different terrain;
   might be going into solid rock, inhibiting levitation or flight,
   or coming back out of such, reinstating levitation/flying */
STATIC_OVL void
switch_terrain()
{
    struct rm *lev = &levl[u.ux][u.uy];
    boolean blocklev = (IS_ROCK(lev->typ) || closed_door(u.ux, u.uy)
                        || (Is_waterlevel(&u.uz) && lev->typ == WATER));

    if (blocklev) {
        /* called from spoteffects(), stop levitating but skip float_down() */
        if (Levitation)
            You_cant("levitate in here.");
        BLevitation |= FROMOUTSIDE;
    } else if (BLevitation) {
        BLevitation &= ~FROMOUTSIDE;
        /* we're probably levitating now; if not, we must be chained
           to a buried iron ball so get float_up() feedback for that */
        if (Levitation || BLevitation)
            float_up();
    }
    /* the same terrain that blocks levitation also blocks flight */
    if (blocklev) {
        if (Flying)
            You_cant("fly in here.");
        BFlying |= FROMOUTSIDE;
    } else if (BFlying) {
        BFlying &= ~FROMOUTSIDE;
        float_vs_flight(); /* maybe toggle (BFlying & I_SPECIAL) */
        /* [minor bug: we don't know whether this is beginning flight or
           resuming it; that could be tracked so that this message could
           be adjusted to "resume flying", but isn't worth the effort...] */
        if (Flying)
            You("start flying.");
    }
}

/* extracted from spoteffects; called by spoteffects to check for entering or
   leaving a pool of water/lava, and by moveloop to check for staying on one;
   returns true to skip rest of spoteffects */
boolean
pooleffects(newspot)
boolean newspot;             /* true if called by spoteffects */
{
    /* check for leaving water */
    if (u.uinwater) {
        boolean still_inwater = FALSE; /* assume we're getting out */

        if (!is_pool(u.ux, u.uy)) {
            if (Is_waterlevel(&u.uz))
                You("pop into an air bubble.");
            else if (is_lava(u.ux, u.uy))
                You("leave the %s...", hliquid("water")); /* oops! */
            else
                You("are on solid %s again.",
                    is_ice(u.ux, u.uy) ? "ice" : "land");
        } else if (Is_waterlevel(&u.uz)) {
            still_inwater = TRUE;
        } else if (Levitation) {
            You("pop out of the %s like a cork!", hliquid("water"));
        } else if (Flying) {
            You("fly out of the %s.", hliquid("water"));
        } else if (Wwalking) {
            You("slowly rise above the surface.");
        } else {
            still_inwater = TRUE;
        }
        if (!still_inwater) {
            boolean was_underwater = (Underwater && !Is_waterlevel(&u.uz));

            u.uinwater = 0;       /* leave the water */
            if (was_underwater) { /* restore vision */
                docrt();
                vision_full_recalc = 1;
            }
        }
    }

    /* check for entering water or lava */
    if (!u.ustuck && !Levitation && !Flying && is_pool_or_lava(u.ux, u.uy)) {
        if (u.usteed
            && (is_flyer(u.usteed->data) || is_floater(u.usteed->data)
                || is_clinger(u.usteed->data))) {
            /* floating or clinging steed keeps hero safe (is_flyer() test
               is redundant; it can't be true since Flying yielded false) */
            return FALSE;
        } else if (u.usteed) {
            /* steed enters pool */
            dismount_steed(Underwater ? DISMOUNT_FELL : DISMOUNT_GENERIC);
            /* dismount_steed() -> float_down() -> pickup()
               (float_down doesn't do autopickup on Air or Water) */
            if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))
                return FALSE;
            /* even if we actually end up at same location, float_down()
               has already done spoteffect()'s trap and pickup actions */
            if (newspot)
                check_special_room(FALSE); /* spoteffects */
            return TRUE;
        }
        /* not mounted */

        /* drown(),lava_effects() return true if hero changes
           location while surviving the problem */
        if (is_lava(u.ux, u.uy)) {
            if (lava_effects())
                return TRUE;
        } else if (!Wwalking
                   && (newspot || !u.uinwater || !(Swimming || Amphibious))) {
            if (drown())
                return TRUE;
        }
    }
    return FALSE;
}

void
spoteffects(pick)
boolean pick;
{
    static int inspoteffects = 0;
    static coord spotloc;
    static int spotterrain;
    static struct trap *spottrap = (struct trap *) 0;
    static unsigned spottraptyp = NO_TRAP;

    struct monst *mtmp;
    struct trap *trap = t_at(u.ux, u.uy);
    int trapflag = iflags.failing_untrap ? FAILEDUNTRAP : 0;

    /* prevent recursion from affecting the hero all over again
       [hero poly'd to iron golem enters water here, drown() inflicts
       damage that triggers rehumanize() which calls spoteffects()...] */
    if (inspoteffects && u.ux == spotloc.x && u.uy == spotloc.y
        /* except when reason is transformed terrain (ice -> water) */
        && spotterrain == levl[u.ux][u.uy].typ
        /* or transformed trap (land mine -> pit) */
        && (!spottrap || !trap || trap->ttyp == spottraptyp))
        return;

    ++inspoteffects;
    spotterrain = levl[u.ux][u.uy].typ;
    spotloc.x = u.ux, spotloc.y = u.uy;

    /* moving onto different terrain might cause Lev or Fly to toggle */
    if (spotterrain != levl[u.ux0][u.uy0].typ || !on_level(&u.uz, &u.uz0))
        switch_terrain();

    if (pooleffects(TRUE))
        goto spotdone;

    check_special_room(FALSE);
    if (IS_SINK(levl[u.ux][u.uy].typ) && Levitation)
        dosinkfall();
    if (!in_steed_dismounting) { /* if dismounting, we'll check again later */
        boolean pit;

        /* if levitation is due to time out at the end of this
           turn, allowing it to do so could give the perception
           that a trap here is being triggered twice, so adjust
           the timeout to prevent that */
        if (trap && (HLevitation & TIMEOUT) == 1L
            && !(ELevitation || (HLevitation & ~(I_SPECIAL | TIMEOUT)))) {
            if (rn2(2)) { /* defer timeout */
                incr_itimeout(&HLevitation, 1L);
            } else { /* timeout early */
                if (float_down(I_SPECIAL | TIMEOUT, 0L)) {
                    /* levitation has ended; we've already triggered
                       any trap and [usually] performed autopickup */
                    trap = 0;
                    pick = FALSE;
                }
            }
        }
        /*
         * If not a pit, pickup before triggering trap.
         * If pit, trigger trap before pickup.
         */
        pit = (trap && is_pit(trap->ttyp));
        if (pick && !pit)
            (void) pickup(1);

        if (trap) {
            /*
             * dotrap on a fire trap calls melt_ice() which triggers
             * spoteffects() (again) which can trigger the same fire
             * trap (again). Use static spottrap to prevent that.
             * We track spottraptyp because some traps morph
             * (landmine to pit) and any new trap type
             * should get triggered.
             */
            if (!spottrap || spottraptyp != trap->ttyp) {
                spottrap = trap;
                spottraptyp = trap->ttyp;
                dotrap(trap, trapflag); /* fall into arrow trap, etc. */
                spottrap = (struct trap *) 0;
                spottraptyp = NO_TRAP;
            }
        }
        if (pick && pit)
            (void) pickup(1);
    }
    /* Warning alerts you to ice danger */
    if (Warning && is_ice(u.ux, u.uy)) {
        static const char *const icewarnings[] = {
            "The ice seems very soft and slushy.",
            "You feel the ice shift beneath you!",
            "The ice, is gonna BREAK!", /* The Dead Zone */
        };
        long time_left = spot_time_left(u.ux, u.uy, MELT_ICE_AWAY);

        if (time_left && time_left < 15L)
            pline("%s", icewarnings[(time_left < 5L) ? 2
                                    : (time_left < 10L) ? 1
                                      : 0]);
    }
    if ((mtmp = m_at(u.ux, u.uy)) && !u.uswallow) {
        mtmp->mundetected = mtmp->msleeping = 0;
        switch (mtmp->data->mlet) {
        case S_PIERCER:
            pline("%s suddenly drops from the %s!", Amonnam(mtmp),
                  ceiling(u.ux, u.uy));
            if (mtmp->mtame) { /* jumps to greet you, not attack */
                ;
            } else if (uarmh && is_metallic(uarmh)) {
                pline("Its blow glances off your %s.",
                      helm_simple_name(uarmh));
            } else if (u.uac + 3 <= rnd(20)) {
                You("are almost hit by %s!",
                    x_monnam(mtmp, ARTICLE_A, "falling", 0, TRUE));
            } else {
                int dmg;

                You("are hit by %s!",
                    x_monnam(mtmp, ARTICLE_A, "falling", 0, TRUE));
                dmg = d(4, 6);
                if (Half_physical_damage)
                    dmg = (dmg + 1) / 2;
                mdamageu(mtmp, dmg);
            }
            break;
        default: /* monster surprises you. */
            if (mtmp->mtame)
                pline("%s jumps near you from the %s.", Amonnam(mtmp),
                      ceiling(u.ux, u.uy));
            else if (mtmp->mpeaceful) {
                You("surprise %s!",
                    Blind && !sensemon(mtmp) ? something : a_monnam(mtmp));
                mtmp->mpeaceful = 0;
            } else
                pline("%s attacks you by surprise!", Amonnam(mtmp));
            break;
        }
        mnexto(mtmp); /* have to move the monster */
    }
spotdone:
    if (!--inspoteffects) {
        spotterrain = STONE; /* 0 */
        spotloc.x = spotloc.y = 0;
    }
    return;
}

/* returns first matching monster */
STATIC_OVL struct monst *
monstinroom(mdat, roomno)
struct permonst *mdat;
int roomno;
{
    register struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (mtmp->data == mdat
            && index(in_rooms(mtmp->mx, mtmp->my, 0), roomno + ROOMOFFSET))
            return mtmp;
    }
    return (struct monst *) 0;
}

char *
in_rooms(x, y, typewanted)
register xchar x, y;
register int typewanted;
{
    static char buf[5];
    char rno, *ptr = &buf[4];
    int typefound, min_x, min_y, max_x, max_y_offset, step;
    register struct rm *lev;

#define goodtype(rno)   \
    (!typewanted                                                    \
     || (typefound = rooms[rno - ROOMOFFSET].rtype) == typewanted   \
     || (typewanted == SHOPBASE && typefound > SHOPBASE))

    switch (rno = levl[x][y].roomno) {
    case NO_ROOM:
        return ptr;
    case SHARED:
        step = 2;
        break;
    case SHARED_PLUS:
        step = 1;
        break;
    default: /* i.e. a regular room # */
        if (goodtype(rno))
            *(--ptr) = rno;
        return ptr;
    }

    min_x = x - 1;
    max_x = x + 1;
    if (x < 1)
        min_x += step;
    else if (x >= COLNO)
        max_x -= step;

    min_y = y - 1;
    max_y_offset = 2;
    if (min_y < 0) {
        min_y += step;
        max_y_offset -= step;
    } else if ((min_y + max_y_offset) >= ROWNO)
        max_y_offset -= step;

    for (x = min_x; x <= max_x; x += step) {
        lev = &levl[x][min_y];
        y = 0;
        if ((rno = lev[y].roomno) >= ROOMOFFSET && !index(ptr, rno)
            && goodtype(rno))
            *(--ptr) = rno;
        y += step;
        if (y > max_y_offset)
            continue;
        if ((rno = lev[y].roomno) >= ROOMOFFSET && !index(ptr, rno)
            && goodtype(rno))
            *(--ptr) = rno;
        y += step;
        if (y > max_y_offset)
            continue;
        if ((rno = lev[y].roomno) >= ROOMOFFSET && !index(ptr, rno)
            && goodtype(rno))
            *(--ptr) = rno;
    }
    return ptr;
}

/* is (x,y) in a town? */
boolean
in_town(x, y)
register int x, y;
{
    s_level *slev = Is_special(&u.uz);
    register struct mkroom *sroom;
    boolean has_subrooms = FALSE;

    if (!slev || !slev->flags.town)
        return FALSE;

    /*
     * See if (x,y) is in a room with subrooms, if so, assume it's the
     * town.  If there are no subrooms, the whole level is in town.
     */
    for (sroom = &rooms[0]; sroom->hx > 0; sroom++) {
        if (sroom->nsubrooms > 0) {
            has_subrooms = TRUE;
            if (inside_room(sroom, x, y))
                return TRUE;
        }
    }

    return !has_subrooms;
}

STATIC_OVL void
move_update(newlev)
register boolean newlev;
{
    char *ptr1, *ptr2, *ptr3, *ptr4;

    Strcpy(u.urooms0, u.urooms);
    Strcpy(u.ushops0, u.ushops);
    if (newlev) {
        u.urooms[0] = '\0';
        u.uentered[0] = '\0';
        u.ushops[0] = '\0';
        u.ushops_entered[0] = '\0';
        Strcpy(u.ushops_left, u.ushops0);
        return;
    }
    Strcpy(u.urooms, in_rooms(u.ux, u.uy, 0));

    for (ptr1 = &u.urooms[0], ptr2 = &u.uentered[0], ptr3 = &u.ushops[0],
         ptr4 = &u.ushops_entered[0];
         *ptr1; ptr1++) {
        if (!index(u.urooms0, *ptr1))
            *(ptr2++) = *ptr1;
        if (IS_SHOP(*ptr1 - ROOMOFFSET)) {
            *(ptr3++) = *ptr1;
            if (!index(u.ushops0, *ptr1))
                *(ptr4++) = *ptr1;
        }
    }
    *ptr2 = '\0';
    *ptr3 = '\0';
    *ptr4 = '\0';

    /* filter u.ushops0 -> u.ushops_left */
    for (ptr1 = &u.ushops0[0], ptr2 = &u.ushops_left[0]; *ptr1; ptr1++)
        if (!index(u.ushops, *ptr1))
            *(ptr2++) = *ptr1;
    *ptr2 = '\0';
}

/* possibly deliver a one-time room entry message */
void
check_special_room(newlev)
register boolean newlev;
{
    register struct monst *mtmp;
    char *ptr;

    move_update(newlev);

    if (*u.ushops0)
        u_left_shop(u.ushops_left, newlev);

    if (!*u.uentered && !*u.ushops_entered) /* implied by newlev */
        return; /* no entrance messages necessary */

    /* Did we just enter a shop? */
    if (*u.ushops_entered)
        u_entered_shop(u.ushops_entered);

    for (ptr = &u.uentered[0]; *ptr; ptr++) {
        int roomno = *ptr - ROOMOFFSET, rt = rooms[roomno].rtype;
        boolean msg_given = TRUE;

        /* Did we just enter some other special room? */
        /* vault.c insists that a vault remain a VAULT,
         * and temples should remain TEMPLEs,
         * but everything else gives a message only the first time */
        switch (rt) {
        case ZOO:
            pline("Welcome to David's treasure zoo!");
            break;
        case SWAMP:
            pline("It %s rather %s down here.", Blind ? "feels" : "looks",
                  Blind ? "humid" : "muddy");
            break;
        case COURT:
            You("enter an opulent throne room!");
            break;
        case LEPREHALL:
            You("enter a leprechaun hall!");
            break;
        case MORGUE:
            if (midnight()) {
                const char *run = locomotion(youmonst.data, "Run");
                pline("%s away!  %s away!", run, run);
            } else
                You("have an uncanny feeling...");
            break;
        case BEEHIVE:
            You("enter a giant beehive!");
            break;
        case COCKNEST:
            You("enter a disgusting nest!");
            break;
        case ANTHOLE:
            You("enter an anthole!");
            break;
        case BARRACKS:
            if (monstinroom(&mons[PM_SOLDIER], roomno)
                || monstinroom(&mons[PM_SERGEANT], roomno)
                || monstinroom(&mons[PM_LIEUTENANT], roomno)
                || monstinroom(&mons[PM_CAPTAIN], roomno))
                You("enter a military barracks!");
            else
                You("enter an abandoned barracks.");
            break;
        case DELPHI: {
            struct monst *oracle = monstinroom(&mons[PM_ORACLE], roomno);
            if (oracle) {
                if (!oracle->mpeaceful)
                    verbalize("You're in Delphi, %s.", plname);
                else
                    verbalize("%s, %s, welcome to Delphi!",
                              Hello((struct monst *) 0), plname);
            } else
                msg_given = FALSE;
            break;
        }
        case TEMPLE:
            intemple(roomno + ROOMOFFSET);
        /*FALLTHRU*/
        default:
            msg_given = (rt == TEMPLE);
            rt = 0;
            break;
        }
        if (msg_given)
            room_discovered(roomno);

        if (rt != 0) {
            rooms[roomno].rtype = OROOM;
            if (!search_special(rt)) {
                /* No more room of that type */
                switch (rt) {
                case COURT:
                    level.flags.has_court = 0;
                    break;
                case SWAMP:
                    level.flags.has_swamp = 0;
                    break;
                case MORGUE:
                    level.flags.has_morgue = 0;
                    break;
                case ZOO:
                    level.flags.has_zoo = 0;
                    break;
                case BARRACKS:
                    level.flags.has_barracks = 0;
                    break;
                case TEMPLE:
                    level.flags.has_temple = 0;
                    break;
                case BEEHIVE:
                    level.flags.has_beehive = 0;
                    break;
                }
            }
            if (rt == COURT || rt == SWAMP || rt == MORGUE || rt == ZOO)
                for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                    if (DEADMONSTER(mtmp))
                        continue;
                    if (!Stealth && !rn2(3))
                        mtmp->msleeping = 0;
                }
        }
    }

    return;
}

/* returns
   1 = cannot pickup, time taken
   0 = cannot pickup, no time taken
  -1 = do normal pickup
  -2 = loot the monster */
int
pickup_checks()
{
    /* uswallow case added by GAN 01/29/87 */
    if (u.uswallow) {
        if (!u.ustuck->minvent) {
            if (is_animal(u.ustuck->data)) {
                You("pick up %s tongue.", s_suffix(mon_nam(u.ustuck)));
                pline("But it's kind of slimy, so you drop it.");
            } else
                You("don't %s anything in here to pick up.",
                    Blind ? "feel" : "see");
            return 1;
        } else {
            return -2; /* loot the monster inventory */
        }
    }
    if (is_pool(u.ux, u.uy)) {
        if (Wwalking || is_floater(youmonst.data) || is_clinger(youmonst.data)
            || (Flying && !Breathless)) {
            You("cannot dive into the %s to pick things up.",
                hliquid("water"));
            return 0;
        } else if (!Underwater) {
            You_cant("even see the bottom, let alone pick up %s.", something);
            return 0;
        }
    }
    if (is_lava(u.ux, u.uy)) {
        if (Wwalking || is_floater(youmonst.data) || is_clinger(youmonst.data)
            || (Flying && !Breathless)) {
            You_cant("reach the bottom to pick things up.");
            return 0;
        } else if (!likes_lava(youmonst.data)) {
            You("would burn to a crisp trying to pick things up.");
            return 0;
        }
    }
    if (!OBJ_AT(u.ux, u.uy)) {
        register struct rm *lev = &levl[u.ux][u.uy];

        if (IS_THRONE(lev->typ))
            pline("It must weigh%s a ton!", lev->looted ? " almost" : "");
        else if (IS_SINK(lev->typ))
            pline_The("plumbing connects it to the floor.");
        else if (IS_GRAVE(lev->typ))
            You("don't need a gravestone.  Yet.");
        else if (IS_FOUNTAIN(lev->typ))
            You("could drink the %s...", hliquid("water"));
        else if (IS_DOOR(lev->typ) && (lev->doormask & D_ISOPEN))
            pline("It won't come off the hinges.");
        else
            There("is nothing here to pick up.");
        return 0;
    }
    if (!can_reach_floor(TRUE)) {
        struct trap *traphere = t_at(u.ux, u.uy);
        if (traphere && uteetering_at_seen_pit(traphere))
            You("cannot reach the bottom of the pit.");
        else if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
            rider_cant_reach();
        else if (Blind && !can_reach_floor(TRUE))
            You("cannot reach anything here.");
        else
            You("cannot reach the %s.", surface(u.ux, u.uy));
        return 0;
    }
    return -1; /* can do normal pickup */
}

/* the ',' command */
int
dopickup(VOID_ARGS)
{
    int count, tmpcount, ret;

    /* awful kludge to work around parse()'s pre-decrement */
    count = (multi || (save_cm && *save_cm == cmd_from_func(dopickup))) ? multi + 1 : 0;
    multi = 0; /* always reset */

    if ((ret = pickup_checks() >= 0))
        return ret;
    else if (ret == -2) {
        tmpcount = -count;
        return loot_mon(u.ustuck, &tmpcount, (boolean *) 0);
    } /* else ret == -1 */

    return pickup(-count);
}

/* stop running if we see something interesting */
/* turn around a corner if that is the only way we can proceed */
/* do not turn left or right twice */
void
lookaround()
{
    register int x, y;
    int i, x0 = 0, y0 = 0, m0 = 1, i0 = 9;
    int corrct = 0, noturn = 0;
    struct monst *mtmp;
    struct trap *trap;

    /* Grid bugs stop if trying to move diagonal, even if blind.  Maybe */
    /* they polymorphed while in the middle of a long move. */
    if (NODIAG(u.umonnum) && u.dx && u.dy) {
        You("cannot move diagonally.");
        nomul(0);
        return;
    }

    if (Blind || context.run == 0)
        return;
    for (x = u.ux - 1; x <= u.ux + 1; x++)
        for (y = u.uy - 1; y <= u.uy + 1; y++) {
            if (!isok(x, y) || (x == u.ux && y == u.uy))
                continue;
            if (NODIAG(u.umonnum) && x != u.ux && y != u.uy)
                continue;

            if ((mtmp = m_at(x, y)) != 0
                && mtmp->m_ap_type != M_AP_FURNITURE
                && mtmp->m_ap_type != M_AP_OBJECT
                && (!mtmp->minvis || See_invisible) && !mtmp->mundetected) {
                if ((context.run != 1 && !mtmp->mtame)
                    || (x == u.ux + u.dx && y == u.uy + u.dy
                        && !context.travel)) {
                    if (iflags.mention_walls)
                        pline("%s blocks your path.", upstart(a_monnam(mtmp)));
                    goto stop;
                }
            }

            if (levl[x][y].typ == STONE)
                continue;
            if (x == u.ux - u.dx && y == u.uy - u.dy)
                continue;

            if (IS_ROCK(levl[x][y].typ) || levl[x][y].typ == ROOM
                || IS_AIR(levl[x][y].typ)) {
                continue;
            } else if (closed_door(x, y) || (mtmp && is_door_mappear(mtmp))) {
                if (x != u.ux && y != u.uy)
                    continue;
                if (context.run != 1) {
                    if (iflags.mention_walls)
                        You("stop in front of the door.");
                    goto stop;
                }
                goto bcorr;
            } else if (levl[x][y].typ == CORR) {
            bcorr:
                if (levl[u.ux][u.uy].typ != ROOM) {
                    if (context.run == 1 || context.run == 3
                        || context.run == 8) {
                        i = dist2(x, y, u.ux + u.dx, u.uy + u.dy);
                        if (i > 2)
                            continue;
                        if (corrct == 1 && dist2(x, y, x0, y0) != 1)
                            noturn = 1;
                        if (i < i0) {
                            i0 = i;
                            x0 = x;
                            y0 = y;
                            m0 = mtmp ? 1 : 0;
                        }
                    }
                    corrct++;
                }
                continue;
            } else if ((trap = t_at(x, y)) && trap->tseen) {
                if (context.run == 1)
                    goto bcorr; /* if you must */
                if (x == u.ux + u.dx && y == u.uy + u.dy) {
                    if (iflags.mention_walls) {
                        int tt = what_trap(trap->ttyp);
                        You("stop in front of %s.",
                            an(defsyms[trap_to_defsym(tt)].explanation));
                    }
                    goto stop;
                }
                continue;
            } else if (is_pool_or_lava(x, y)) {
                /* water and lava only stop you if directly in front, and stop
                 * you even if you are running
                 */
                if (!Levitation && !Flying && !is_clinger(youmonst.data)
                    && x == u.ux + u.dx && y == u.uy + u.dy) {
                    /* No Wwalking check; otherwise they'd be able
                     * to test boots by trying to SHIFT-direction
                     * into a pool and seeing if the game allowed it
                     */
                    if (iflags.mention_walls)
                        You("stop at the edge of the %s.",
                            hliquid(is_pool(x,y) ? "water" : "lava"));
                    goto stop;
                }
                continue;
            } else { /* e.g. objects or trap or stairs */
                if (context.run == 1)
                    goto bcorr;
                if (context.run == 8)
                    continue;
                if (mtmp)
                    continue; /* d */
                if (((x == u.ux - u.dx) && (y != u.uy + u.dy))
                    || ((y == u.uy - u.dy) && (x != u.ux + u.dx)))
                    continue;
            }
        stop:
            nomul(0);
            return;
        } /* end for loops */

    if (corrct > 1 && context.run == 2) {
        if (iflags.mention_walls)
            pline_The("corridor widens here.");
        goto stop;
    }
    if ((context.run == 1 || context.run == 3 || context.run == 8) && !noturn
        && !m0 && i0 && (corrct == 1 || (corrct == 2 && i0 == 1))) {
        /* make sure that we do not turn too far */
        if (i0 == 2) {
            if (u.dx == y0 - u.uy && u.dy == u.ux - x0)
                i = 2; /* straight turn right */
            else
                i = -2; /* straight turn left */
        } else if (u.dx && u.dy) {
            if ((u.dx == u.dy && y0 == u.uy) || (u.dx != u.dy && y0 != u.uy))
                i = -1; /* half turn left */
            else
                i = 1; /* half turn right */
        } else {
            if ((x0 - u.ux == y0 - u.uy && !u.dy)
                || (x0 - u.ux != y0 - u.uy && u.dy))
                i = 1; /* half turn right */
            else
                i = -1; /* half turn left */
        }

        i += u.last_str_turn;
        if (i <= 2 && i >= -2) {
            u.last_str_turn = i;
            u.dx = x0 - u.ux;
            u.dy = y0 - u.uy;
        }
    }
}

/* check for a doorway which lacks its door (NODOOR or BROKEN) */
STATIC_OVL boolean
doorless_door(x, y)
int x, y;
{
    struct rm *lev_p = &levl[x][y];

    if (!IS_DOOR(lev_p->typ))
        return FALSE;
    /* all rogue level doors are doorless but disallow diagonal access, so
       we treat them as if their non-existant doors were actually present */
    if (Is_rogue_level(&u.uz))
        return FALSE;
    return !(lev_p->doormask & ~(D_NODOOR | D_BROKEN));
}

/* used by drown() to check whether hero can crawl from water to <x,y> */
boolean
crawl_destination(x, y)
int x, y;
{
    /* is location ok in general? */
    if (!goodpos(x, y, &youmonst, 0))
        return FALSE;

    /* orthogonal movement is unrestricted when destination is ok */
    if (x == u.ux || y == u.uy)
        return TRUE;

    /* diagonal movement has some restrictions */
    if (NODIAG(u.umonnum))
        return FALSE; /* poly'd into a grid bug... */
    if (Passes_walls)
        return TRUE; /* or a xorn... */
    /* pool could be next to a door, conceivably even inside a shop */
    if (IS_DOOR(levl[x][y].typ) && (!doorless_door(x, y) || block_door(x, y)))
        return FALSE;
    /* finally, are we trying to squeeze through a too-narrow gap? */
    return !(bad_rock(youmonst.data, u.ux, y)
             && bad_rock(youmonst.data, x, u.uy));
}

/* something like lookaround, but we are not running */
/* react only to monsters that might hit us */
int
monster_nearby()
{
    register int x, y;
    register struct monst *mtmp;

    /* Also see the similar check in dochugw() in monmove.c */
    for (x = u.ux - 1; x <= u.ux + 1; x++)
        for (y = u.uy - 1; y <= u.uy + 1; y++) {
            if (!isok(x, y) || (x == u.ux && y == u.uy))
                continue;
            if ((mtmp = m_at(x, y)) && mtmp->m_ap_type != M_AP_FURNITURE
                && mtmp->m_ap_type != M_AP_OBJECT
                && (!mtmp->mpeaceful || Hallucination)
                && (!is_hider(mtmp->data) || !mtmp->mundetected)
                && !noattacks(mtmp->data) && mtmp->mcanmove
                && !mtmp->msleeping  /* aplvax!jcn */
                && !onscary(u.ux, u.uy, mtmp) && canspotmon(mtmp))
                return 1;
        }
    return 0;
}

void
nomul(nval)
register int nval;
{
    if (multi < nval)
        return;              /* This is a bug fix by ab@unido */
    u.uinvulnerable = FALSE; /* Kludge to avoid ctrl-C bug -dlc */
    u.usleep = 0;
    multi = nval;
    if (nval == 0)
        multi_reason = NULL;
    context.travel = context.travel1 = context.mv = context.run = 0;
}

/* called when a non-movement, multi-turn action has completed */
void
unmul(msg_override)
const char *msg_override;
{
    multi = 0; /* caller will usually have done this already */
    if (msg_override)
        nomovemsg = msg_override;
    else if (!nomovemsg)
        nomovemsg = You_can_move_again;
    if (*nomovemsg)
        pline1(nomovemsg);
    nomovemsg = 0;
    u.usleep = 0;
    multi_reason = NULL;
    if (afternmv) {
        int NDECL((*f)) = afternmv;
        /* clear afternmv before calling it (to override the
           encumbrance hack for levitation--see weight_cap()) */
        afternmv = (int NDECL((*))) 0;
        (void) (*f)();
    }
}

STATIC_OVL void
maybe_wail()
{
    static short powers[] = { TELEPORT, SEE_INVIS, POISON_RES, COLD_RES,
                              SHOCK_RES, FIRE_RES, SLEEP_RES, DISINT_RES,
                              TELEPORT_CONTROL, STEALTH, FAST, INVIS };

    if (moves <= wailmsg + 50)
        return;

    wailmsg = moves;
    if (Role_if(PM_WIZARD) || Race_if(PM_ELF) || Role_if(PM_VALKYRIE)) {
        const char *who;
        int i, powercnt;

        who = (Role_if(PM_WIZARD) || Role_if(PM_VALKYRIE)) ? urole.name.m
                                                           : "Elf";
        if (u.uhp == 1) {
            pline("%s is about to die.", who);
        } else {
            for (i = 0, powercnt = 0; i < SIZE(powers); ++i)
                if (u.uprops[powers[i]].intrinsic & INTRINSIC)
                    ++powercnt;

            pline((powercnt >= 4) ? "%s, all your powers will be lost..."
                                  : "%s, your life force is running out.",
                  who);
        }
    } else {
        You_hear(u.uhp == 1 ? "the wailing of the Banshee..."
                            : "the howling of the CwnAnnwn...");
    }
}

void
losehp(n, knam, k_format)
register int n;
register const char *knam;
boolean k_format;
{
    if (Upolyd) {
        u.mh -= n;
        if (u.mhmax < u.mh)
            u.mhmax = u.mh;
        context.botl = 1;
        if (u.mh < 1)
            rehumanize();
        else if (n > 0 && u.mh * 10 < u.mhmax && Unchanging)
            maybe_wail();
        return;
    }

    u.uhp -= n;
    if (u.uhp > u.uhpmax)
        u.uhpmax = u.uhp; /* perhaps n was negative */
    else
        context.travel = context.travel1 = context.mv = context.run = 0;
    context.botl = 1;
    if (u.uhp < 1) {
        killer.format = k_format;
        if (killer.name != knam) /* the thing that killed you */
            Strcpy(killer.name, knam ? knam : "");
        You("die...");
        done(DIED);
    } else if (n > 0 && u.uhp * 10 < u.uhpmax) {
        maybe_wail();
    }
}

int
weight_cap()
{
    long carrcap, save_ELev = ELevitation, save_BLev = BLevitation;

    /* boots take multiple turns to wear but any properties they
       confer are enabled at the start rather than the end; that
       causes message sequencing issues for boots of levitation
       so defer their encumbrance benefit until they're fully worn */
    if (afternmv == Boots_on && (ELevitation & W_ARMF) != 0L) {
        ELevitation &= ~W_ARMF;
        float_vs_flight(); /* in case Levitation is blocking Flying */
    }
    /* levitation is blocked by being trapped in the floor, but it still
       functions enough in that situation to enhance carrying capacity */
    BLevitation &= ~I_SPECIAL;

    carrcap = 25 * (ACURRSTR + ACURR(A_CON)) + 50;
    if (Upolyd) {
        /* consistent with can_carry() in mon.c */
        if (youmonst.data->mlet == S_NYMPH)
            carrcap = MAX_CARR_CAP;
        else if (!youmonst.data->cwt)
            carrcap = (carrcap * (long) youmonst.data->msize) / MZ_HUMAN;
        else if (!strongmonst(youmonst.data)
                 || (strongmonst(youmonst.data)
                     && (youmonst.data->cwt > WT_HUMAN)))
            carrcap = (carrcap * (long) youmonst.data->cwt / WT_HUMAN);
    }

    if (Levitation || Is_airlevel(&u.uz) /* pugh@cornell */
        || (u.usteed && strongmonst(u.usteed->data))) {
        carrcap = MAX_CARR_CAP;
    } else {
        if (carrcap > MAX_CARR_CAP)
            carrcap = MAX_CARR_CAP;
        if (!Flying) {
            if (EWounded_legs & LEFT_SIDE)
                carrcap -= 100;
            if (EWounded_legs & RIGHT_SIDE)
                carrcap -= 100;
        }
        if (carrcap < 0)
            carrcap = 0;
    }

    if (ELevitation != save_ELev || BLevitation != save_BLev) {
        ELevitation = save_ELev;
        BLevitation = save_BLev;
        float_vs_flight();
    }

    return (int) carrcap;
}

static int wc; /* current weight_cap(); valid after call to inv_weight() */

/* returns how far beyond the normal capacity the player is currently. */
/* inv_weight() is negative if the player is below normal capacity. */
int
inv_weight()
{
    register struct obj *otmp = invent;
    register int wt = 0;

    while (otmp) {
        if (otmp->oclass == COIN_CLASS)
            wt += (int) (((long) otmp->quan + 50L) / 100L);
        else if (otmp->otyp != BOULDER || !throws_rocks(youmonst.data))
            wt += otmp->owt;
        otmp = otmp->nobj;
    }
    wc = weight_cap();
    return (wt - wc);
}

/*
 * Returns 0 if below normal capacity, or the number of "capacity units"
 * over the normal capacity the player is loaded.  Max is 5.
 */
int
calc_capacity(xtra_wt)
int xtra_wt;
{
    int cap, wt = inv_weight() + xtra_wt;

    if (wt <= 0)
        return UNENCUMBERED;
    if (wc <= 1)
        return OVERLOADED;
    cap = (wt * 2 / wc) + 1;
    return min(cap, OVERLOADED);
}

int
near_capacity()
{
    return calc_capacity(0);
}

int
max_capacity()
{
    int wt = inv_weight();

    return (wt - (2 * wc));
}

boolean
check_capacity(str)
const char *str;
{
    if (near_capacity() >= EXT_ENCUMBER) {
        if (str)
            pline1(str);
        else
            You_cant("do that while carrying so much stuff.");
        return 1;
    }
    return 0;
}

int
inv_cnt(incl_gold)
boolean incl_gold;
{
    register struct obj *otmp = invent;
    register int ct = 0;

    while (otmp) {
        if (incl_gold || otmp->invlet != GOLD_SYM)
            ct++;
        otmp = otmp->nobj;
    }
    return ct;
}

/* Counts the money in an object chain. */
/* Intended use is for your or some monster's inventory, */
/* now that u.gold/m.gold is gone.*/
/* Counting money in a container might be possible too. */
long
money_cnt(otmp)
struct obj *otmp;
{
    while (otmp) {
        /* Must change when silver & copper is implemented: */
        if (otmp->oclass == COIN_CLASS)
            return otmp->quan;
        otmp = otmp->nobj;
    }
    return 0L;
}

/*hack.c*/

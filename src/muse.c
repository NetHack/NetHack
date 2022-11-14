/* NetHack 3.7	muse.c	$NHDT-Date: 1654972707 2022/06/11 18:38:27 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.164 $ */
/*      Copyright (C) 1990 by Ken Arromdee                         */
/* NetHack may be freely redistributed.  See license for details.  */

/*
 * Monster item usage routines.
 */

#include "hack.h"

/* Let monsters use magic items.  Arbitrary assumptions: Monsters only use
 * scrolls when they can see, monsters know when wands have 0 charges,
 * monsters cannot recognize if items are cursed are not, monsters which
 * are confused don't know not to read scrolls, etc....
 */

static int precheck(struct monst *, struct obj *);
static void mzapwand(struct monst *, struct obj *, boolean);
static void mplayhorn(struct monst *, struct obj *, boolean);
static void mreadmsg(struct monst *, struct obj *);
static void mquaffmsg(struct monst *, struct obj *);
static boolean m_use_healing(struct monst *);
static boolean m_sees_sleepy_soldier(struct monst *);
static boolean linedup_chk_corpse(coordxy, coordxy);
static void m_use_undead_turning(struct monst *, struct obj *);
static boolean hero_behind_chokepoint(struct monst *);
static boolean mon_has_friends(struct monst *);
static int mbhitm(struct monst *, struct obj *);
static void mbhit(struct monst *, int, int (*)(MONST_P, OBJ_P),
                  int (*)(OBJ_P, OBJ_P), struct obj *);
static struct permonst *muse_newcham_mon(struct monst *);
static int mloot_container(struct monst *mon, struct obj *, boolean);
static void you_aggravate(struct monst *);
#if 0
static boolean necrophiliac(struct obj *, boolean);
#endif
static void mon_consume_unstone(struct monst *, struct obj *, boolean,
                                boolean);
static boolean cures_stoning(struct monst *, struct obj *, boolean);
static boolean mcould_eat_tin(struct monst *);
static boolean muse_unslime(struct monst *, struct obj *, struct trap *,
                            boolean);
static int cures_sliming(struct monst *, struct obj *);
static boolean green_mon(struct monst *);

/* Any preliminary checks which may result in the monster being unable to use
 * the item.  Returns 0 if nothing happened, 2 if the monster can't do
 * anything (i.e. it teleported) and 1 if it's dead.
 */
static int
precheck(struct monst* mon, struct obj* obj)
{
    boolean vis;

    if (!obj)
        return 0;
    vis = cansee(mon->mx, mon->my);

    if (obj->oclass == POTION_CLASS) {
        coord cc;
        static const char *const empty = "The potion turns out to be empty.";
        struct monst *mtmp;

        if (objdescr_is(obj, "milky")) {
            if (!(g.mvitals[PM_GHOST].mvflags & G_GONE)
                && !rn2(POTION_OCCUPANT_CHANCE(g.mvitals[PM_GHOST].born))) {
                if (!enexto(&cc, mon->mx, mon->my, &mons[PM_GHOST]))
                    return 0;
                mquaffmsg(mon, obj);
                m_useup(mon, obj);
                mtmp = makemon(&mons[PM_GHOST], cc.x, cc.y, MM_NOMSG);
                if (!mtmp) {
                    if (vis)
                        pline1(empty);
                } else {
                    if (vis) {
                        pline(
                            "As %s opens the bottle, an enormous %s emerges!",
                              mon_nam(mon),
                              Hallucination ? rndmonnam(NULL)
                                            : (const char *) "ghost");
                        pline("%s is frightened to death, and unable to move.",
                              Monnam(mon));
                    }
                    paralyze_monst(mon, 3);
                }
                return 2;
            }
        }
        if (objdescr_is(obj, "smoky")
            && !(g.mvitals[PM_DJINNI].mvflags & G_GONE)
            && !rn2(POTION_OCCUPANT_CHANCE(g.mvitals[PM_DJINNI].born))) {
            if (!enexto(&cc, mon->mx, mon->my, &mons[PM_DJINNI]))
                return 0;
            mquaffmsg(mon, obj);
            m_useup(mon, obj);
            mtmp = makemon(&mons[PM_DJINNI], cc.x, cc.y, MM_NOMSG);
            if (!mtmp) {
                if (vis)
                    pline1(empty);
            } else {
                if (vis)
                    pline("In a cloud of smoke, %s emerges!", a_monnam(mtmp));
                pline("%s speaks.", vis ? Monnam(mtmp) : Something);
                /* I suspect few players will be upset that monsters */
                /* can't wish for wands of death here.... */
                if (rn2(2)) {
                    verbalize("You freed me!");
                    mtmp->mpeaceful = 1;
                    set_malign(mtmp);
                } else {
                    verbalize("It is about time.");
                    if (vis)
                        pline("%s vanishes.", Monnam(mtmp));
                    mongone(mtmp);
                }
            }
            return 2;
        }
    }
    if (obj->oclass == WAND_CLASS && obj->cursed
        && !rn2(WAND_BACKFIRE_CHANCE)) {
        int dam = d(obj->spe + 2, 6);

        /* 3.6.1: no Deaf filter; 'if' message doesn't warrant it, 'else'
           message doesn't need it since You_hear() has one of its own */
        if (vis) {
            pline("%s zaps %s, which suddenly explodes!", Monnam(mon),
                  an(xname(obj)));
        } else {
            /* same near/far threshold as mzapwand() */
            int range = couldsee(mon->mx, mon->my) /* 9 or 5 */
                           ? (BOLT_LIM + 1) : (BOLT_LIM - 3);

            You_hear("a zap and an explosion %s.",
                     (mdistu(mon) <= range * range)
                        ? "nearby" : "in the distance");
        }
        m_useup(mon, obj);
        mon->mhp -= dam;
        if (DEADMONSTER(mon)) {
            monkilled(mon, "", AD_RBRE);
            return 1;
        }
        g.m.has_defense = g.m.has_offense = g.m.has_misc = 0;
        /* Only one needed to be set to 0 but the others are harmless */
    }
    return 0;
}

/* when a monster zaps a wand give a message, deduct a charge, and if it
   isn't directly seen, remove hero's memory of the number of charges */
static void
mzapwand(
    struct monst *mtmp,
    struct obj *otmp,
    boolean self)
{
    if (otmp->spe < 1) {
        impossible("Mon zapping wand with %d charges?", otmp->spe);
        return;
    }
    if (!canseemon(mtmp)) {
        int range = couldsee(mtmp->mx, mtmp->my) /* 9 or 5 */
                       ? (BOLT_LIM + 1) : (BOLT_LIM - 3);

        You_hear("a %s zap.", (mdistu(mtmp) <= range * range)
                                 ? "nearby" : "distant");
        unknow_object(otmp); /* hero loses info when unseen obj is used */
    } else if (self) {
        pline("%s with %s!",
              monverbself(mtmp, Monnam(mtmp), "zap", (char *) 0),
              doname(otmp));
    } else {
        pline("%s zaps %s!", Monnam(mtmp), an(xname(otmp)));
        stop_occupation();
    }
    otmp->spe -= 1;
}

/* similar to mzapwand() but for magical horns (only instrument mons play) */
static void
mplayhorn(
    struct monst *mtmp,
    struct obj *otmp,
    boolean self)
{
    char *objnamp, objbuf[BUFSZ];

    if (!canseemon(mtmp)) {
        int range = couldsee(mtmp->mx, mtmp->my) /* 9 or 5 */
                       ? (BOLT_LIM + 1) : (BOLT_LIM - 3);

        You_hear("a horn being played %s.",
                 (mdistu(mtmp) <= range * range)
                    ? "nearby" : "in the distance");
        unknow_object(otmp); /* hero loses info when unseen obj is used */
    } else if (self) {
        otmp->dknown = 1;
        objnamp = xname(otmp);
        if (strlen(objnamp) >= QBUFSZ)
            objnamp = simpleonames(otmp);
        Sprintf(objbuf, "a %s directed at", objnamp);
        /* "<mon> plays a <horn> directed at himself!" */
        pline("%s!", monverbself(mtmp, Monnam(mtmp), "play", objbuf));
        makeknown(otmp->otyp); /* (wands handle this slightly differently) */
    } else {
        otmp->dknown = 1;
        objnamp = xname(otmp);
        if (strlen(objnamp) >= QBUFSZ)
            objnamp = simpleonames(otmp);
        pline("%s %s %s directed at you!",
              /* monverbself() would adjust the verb if hallucination made
                 subject plural; stick with singular here, at least for now */
              Monnam(mtmp), "plays", an(objnamp));
        makeknown(otmp->otyp);
        stop_occupation();
    }
    otmp->spe -= 1; /* use a charge */
}

static void
mreadmsg(struct monst* mtmp, struct obj* otmp)
{
    boolean vismon = canseemon(mtmp);
    char onambuf[BUFSZ];
    short saverole;
    unsigned savebknown;

    if (!vismon && Deaf)
        return; /* no feedback */

    otmp->dknown = 1; /* seeing or hearing it read reveals its label */
    /* shouldn't be able to hear curse/bless status of unseen scrolls;
       for priest characters, bknown will always be set during naming */
    savebknown = otmp->bknown;
    saverole = Role_switch;
    if (!vismon) {
        otmp->bknown = 0;
        if (Role_if(PM_CLERIC))
            Role_switch = 0;
    }
    Strcpy(onambuf, singular(otmp, doname));
    Role_switch = saverole;
    otmp->bknown = savebknown;

    if (vismon)
        pline("%s reads %s!", Monnam(mtmp), onambuf);
    else
        You_hear("%s reading %s.",
                 x_monnam(mtmp, ARTICLE_A, (char *) 0,
                          (SUPPRESS_IT | SUPPRESS_INVISIBLE | SUPPRESS_SADDLE),
                          FALSE),
                 onambuf);

    if (mtmp->mconf)
        pline("Being confused, %s mispronounces the magic words...",
              vismon ? mon_nam(mtmp) : mhe(mtmp));
}

static void
mquaffmsg(struct monst* mtmp, struct obj* otmp)
{
    if (canseemon(mtmp)) {
        otmp->dknown = 1;
        pline("%s drinks %s!", Monnam(mtmp), singular(otmp, doname));
    } else if (!Deaf)
        You_hear("a chugging sound.");
}

/* Defines for various types of stuff.  The order in which monsters prefer
 * to use them is determined by the order of the code logic, not the
 * numerical order in which they are defined.
 */
#define MUSE_SCR_TELEPORTATION 1
#define MUSE_WAN_TELEPORTATION_SELF 2
#define MUSE_POT_HEALING 3
#define MUSE_POT_EXTRA_HEALING 4
#define MUSE_WAN_DIGGING 5
#define MUSE_TRAPDOOR 6
#define MUSE_TELEPORT_TRAP 7
#define MUSE_UPSTAIRS 8
#define MUSE_DOWNSTAIRS 9
#define MUSE_WAN_CREATE_MONSTER 10
#define MUSE_SCR_CREATE_MONSTER 11
#define MUSE_UP_LADDER 12
#define MUSE_DN_LADDER 13
#define MUSE_SSTAIRS 14
#define MUSE_WAN_TELEPORTATION 15
#define MUSE_BUGLE 16
#define MUSE_UNICORN_HORN 17
#define MUSE_POT_FULL_HEALING 18
#define MUSE_LIZARD_CORPSE 19
#define MUSE_WAN_UNDEAD_TURNING 20 /* also an offensive item */
/*
#define MUSE_INNATE_TPT 9999
 * We cannot use this.  Since monsters get unlimited teleportation, if they
 * were allowed to teleport at will you could never catch them.  Instead,
 * assume they only teleport at random times, despite the inconsistency
 * that if you polymorph into one you teleport at will.
 */

static boolean
m_use_healing(struct monst* mtmp)
{
    struct obj *obj;

    if ((obj = m_carrying(mtmp, POT_FULL_HEALING)) != 0) {
        g.m.defensive = obj;
        g.m.has_defense = MUSE_POT_FULL_HEALING;
        return TRUE;
    }
    if ((obj = m_carrying(mtmp, POT_EXTRA_HEALING)) != 0) {
        g.m.defensive = obj;
        g.m.has_defense = MUSE_POT_EXTRA_HEALING;
        return TRUE;
    }
    if ((obj = m_carrying(mtmp, POT_HEALING)) != 0) {
        g.m.defensive = obj;
        g.m.has_defense = MUSE_POT_HEALING;
        return TRUE;
    }
    return FALSE;
}

/* return TRUE if monster mtmp can see at least one sleeping soldier */
static boolean
m_sees_sleepy_soldier(struct monst *mtmp)
{
    coordxy x = mtmp->mx, y = mtmp->my;
    coordxy xx, yy;
    struct monst *mon;

    /* Distance is arbitrary.  What we really want to do is
     * have the soldier play the bugle when it sees or
     * remembers soldiers nearby...
     */
    for (xx = x - 3; xx <= x + 3; xx++)
        for (yy = y - 3; yy <= y + 3; yy++) {
            if (!isok(xx, yy) || (xx == x && yy == y))
                continue;
            if ((mon = m_at(xx, yy)) != 0 && is_mercenary(mon->data)
                && mon->data != &mons[PM_GUARD]
                && helpless(mon))
                return TRUE;
        }
    return FALSE;
}

/* Select a defensive item/action for a monster.  Returns TRUE iff one is
   found. */
boolean
find_defensive(struct monst* mtmp, boolean tryescape)
{
    struct obj *obj;
    struct trap *t;
    int fraction;
    coordxy x = mtmp->mx, y = mtmp->my;
    boolean stuck = (mtmp == u.ustuck),
            immobile = (mtmp->data->mmove == 0);
    stairway *stway;

    g.m.defensive = (struct obj *) 0;
    g.m.has_defense = 0;

    if (is_animal(mtmp->data) || mindless(mtmp->data))
        return FALSE;
    if (!tryescape && dist2(x, y, mtmp->mux, mtmp->muy) > 25)
        return FALSE;
    if (u.uswallow && stuck)
        return FALSE;

    /*
     * Since unicorn horns don't get used up, the monster would look
     * silly trying to use the same cursed horn round after round,
     * so skip cursed unicorn horns.
     *
     * Unicorns use their own horns; they're excluded from inventory
     * scanning by nohands().  Ki-rin is depicted in the AD&D Monster
     * Manual with same horn as a unicorn, so let it use its horn too.
     * is_unicorn() doesn't include it; the class differs and it has
     * no interest in gems.
     */
    if (mtmp->mconf || mtmp->mstun || !mtmp->mcansee) {
        obj = 0;
        if (!nohands(mtmp->data)) {
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if (obj->otyp == UNICORN_HORN && !obj->cursed)
                    break;
        }
        if (obj || is_unicorn(mtmp->data) || mtmp->data == &mons[PM_KI_RIN]) {
            g.m.defensive = obj;
            g.m.has_defense = MUSE_UNICORN_HORN;
            return TRUE;
        }
    }

    if (mtmp->mconf || mtmp->mstun) {
        struct obj *liztin = 0;

        for (obj = mtmp->minvent; obj; obj = obj->nobj) {
            if (obj->otyp == CORPSE && obj->corpsenm == PM_LIZARD) {
                g.m.defensive = obj;
                g.m.has_defense = MUSE_LIZARD_CORPSE;
                return TRUE;
            } else if (obj->otyp == TIN && obj->corpsenm == PM_LIZARD) {
                liztin = obj;
            }
        }
        /* confused or stunned monster might not be able to open tin */
        if (liztin && mcould_eat_tin(mtmp) && rn2(3)) {
            g.m.defensive = liztin;
            /* tin and corpse ultimately end up being handled the same */
            g.m.has_defense = MUSE_LIZARD_CORPSE;
            return TRUE;
        }
    }

    /* It so happens there are two unrelated cases when we might want to
     * check specifically for healing alone.  The first is when the monster
     * is blind (healing cures blindness).  The second is when the monster
     * is peaceful; then we don't want to flee the player, and by
     * coincidence healing is all there is that doesn't involve fleeing.
     * These would be hard to combine because of the control flow.
     * Pestilence won't use healing even when blind.
     */
    if (!mtmp->mcansee && !nohands(mtmp->data)
        && mtmp->data != &mons[PM_PESTILENCE]) {
        if (m_use_healing(mtmp))
            return TRUE;
    }

    /* monsters aren't given wands of undead turning but if they
       happen to have picked one up, use it against corpse wielder;
       when applicable, use it now even if 'mtmp' isn't wounded */
    if (!mtmp->mpeaceful && !nohands(mtmp->data)
        && uwep && uwep->otyp == CORPSE
        && touch_petrifies(&mons[uwep->corpsenm])
        && !poly_when_stoned(mtmp->data) && !resists_ston(mtmp)
        && lined_up(mtmp)) { /* only lines up if distu range is within 5*5 */
        /* could use m_carrying(), then nxtobj() when matching wand
           is empty, but direct traversal is actually simpler here */
        for (obj = mtmp->minvent; obj; obj = obj->nobj)
            if (obj->otyp == WAN_UNDEAD_TURNING && obj->spe > 0) {
                g.m.defensive = obj;
                g.m.has_defense = MUSE_WAN_UNDEAD_TURNING;
                return TRUE;
            }
    }

    if (!tryescape) {
        /* do we try to heal? */
        fraction = u.ulevel < 10 ? 5 : u.ulevel < 14 ? 4 : 3;
        if (mtmp->mhp >= mtmp->mhpmax
            || (mtmp->mhp >= 10 && mtmp->mhp * fraction >= mtmp->mhpmax))
            return FALSE;

        if (mtmp->mpeaceful) {
            if (!nohands(mtmp->data)) {
                if (m_use_healing(mtmp))
                    return TRUE;
            }
            return FALSE;
        }
    }

    if (stuck || immobile) {
        ; /* fleeing by stairs or traps is not possible */
    } else if (levl[x][y].typ == STAIRS) {
        stway = stairway_at(x,y);
        if (stway && !stway->up && stway->tolev.dnum == u.uz.dnum) {
            if (!is_floater(mtmp->data))
                g.m.has_defense = MUSE_DOWNSTAIRS;
        } else if (stway && stway->up && stway->tolev.dnum == u.uz.dnum) {
            g.m.has_defense = MUSE_UPSTAIRS;
        } else if (stway &&  stway->tolev.dnum != u.uz.dnum) {
            if (stway->up || !is_floater(mtmp->data))
                g.m.has_defense = MUSE_SSTAIRS;
        }
    } else if (levl[x][y].typ == LADDER) {
        stway = stairway_at(x,y);
        if (stway && stway->up && stway->tolev.dnum == u.uz.dnum) {
            g.m.has_defense = MUSE_UP_LADDER;
        } else if (stway && !stway->up && stway->tolev.dnum == u.uz.dnum) {
            if (!is_floater(mtmp->data))
                g.m.has_defense = MUSE_DN_LADDER;
        } else if (stway && stway->tolev.dnum != u.uz.dnum) {
            if (stway->up || !is_floater(mtmp->data))
                g.m.has_defense = MUSE_SSTAIRS;
        }
    } else {
        /* Note: trap doors take precedence over teleport traps. */
        coordxy xx, yy, i, locs[10][2];
        boolean ignore_boulders = (verysmall(mtmp->data)
                                   || throws_rocks(mtmp->data)
                                   || passes_walls(mtmp->data)),
            diag_ok = !NODIAG(monsndx(mtmp->data));

        for (i = 0; i < 10; ++i) /* 10: 9 spots plus sentinel */
            locs[i][0] = locs[i][1] = 0;
        /* collect viable spots; monster's <mx,my> comes first */
        locs[0][0] = x, locs[0][1] = y;
        i = 1;
        for (xx = x - 1; xx <= x + 1; xx++)
            for (yy = y - 1; yy <= y + 1; yy++)
                if (isok(xx, yy) && (xx != x || yy != y)) {
                    locs[i][0] = xx, locs[i][1] = yy;
                    ++i;
                }
        /* look for a suitable trap among the viable spots */
        for (i = 0; i < 10; ++i) {
            xx = locs[i][0], yy = locs[i][1];
            if (!xx)
                break; /* we've run out of spots */
            /* skip if it's hero's location
               or a diagonal spot and monster can't move diagonally
               or some other monster is there */
            if (u_at(xx, yy)
                || (xx != x && yy != y && !diag_ok)
                || (g.level.monsters[xx][yy] && !(xx == x && yy == y)))
                continue;
            /* skip if there's no trap or can't/won't move onto trap */
            if ((t = t_at(xx, yy)) == 0
                || (!ignore_boulders && sobj_at(BOULDER, xx, yy))
                || onscary(xx, yy, mtmp))
                continue;
            /* use trap if it's the correct type */
            if (is_hole(t->ttyp)
                && !is_floater(mtmp->data)
                && !mtmp->isshk && !mtmp->isgd && !mtmp->ispriest
                && Can_fall_thru(&u.uz)) {
                g.trapx = xx;
                g.trapy = yy;
                g.m.has_defense = MUSE_TRAPDOOR;
                break; /* no need to look at any other spots */
            } else if (t->ttyp == TELEP_TRAP) {
                g.trapx = xx;
                g.trapy = yy;
                g.m.has_defense = MUSE_TELEPORT_TRAP;
            }
        }
    }

    if (nohands(mtmp->data)) /* can't use objects */
        goto botm;

    if (is_mercenary(mtmp->data) && (obj = m_carrying(mtmp, BUGLE)) != 0
        && m_sees_sleepy_soldier(mtmp)) {
        g.m.defensive = obj;
        g.m.has_defense = MUSE_BUGLE;
    }

    /* use immediate physical escape prior to attempting magic */
    if (g.m.has_defense) /* stairs, trap door or tele-trap, bugle alert */
        goto botm;

    /* kludge to cut down on trap destruction (particularly portals) */
    t = t_at(x, y);
    if (t && (is_pit(t->ttyp) || t->ttyp == WEB
              || t->ttyp == BEAR_TRAP))
        t = 0; /* ok for monster to dig here */

#define nomore(x)       if (g.m.has_defense == x) continue;
    /* selection could be improved by collecting all possibilities
       into an array and then picking one at random */
    for (obj = mtmp->minvent; obj; obj = obj->nobj) {
        /* don't always use the same selection pattern */
        if (g.m.has_defense && !rn2(3))
            break;

        /* nomore(MUSE_WAN_DIGGING); */
        if (g.m.has_defense == MUSE_WAN_DIGGING)
            break;
        if (obj->otyp == WAN_DIGGING && obj->spe > 0 && !stuck && !t
            && !mtmp->isshk && !mtmp->isgd && !mtmp->ispriest
            && !is_floater(mtmp->data)
            /* monsters digging in Sokoban can ruin things */
            && !Sokoban
            /* digging wouldn't be effective; assume they know that */
            && !(levl[x][y].wall_info & W_NONDIGGABLE)
            && !(Is_botlevel(&u.uz) || In_endgame(&u.uz))
            && !(is_ice(x, y) || is_pool(x, y) || is_lava(x, y))
            && !(mtmp->data == &mons[PM_VLAD_THE_IMPALER]
                 && In_V_tower(&u.uz))) {
            g.m.defensive = obj;
            g.m.has_defense = MUSE_WAN_DIGGING;
        }
        nomore(MUSE_WAN_TELEPORTATION_SELF);
        nomore(MUSE_WAN_TELEPORTATION);
        if (obj->otyp == WAN_TELEPORTATION && obj->spe > 0) {
            /* use the TELEP_TRAP bit to determine if they know
             * about noteleport on this level or not.  Avoids
             * ineffective re-use of teleportation.  This does
             * mean if the monster leaves the level, they'll know
             * about teleport traps.
             */
            if (!noteleport_level(mtmp)
                || !mon_knows_traps(mtmp, TELEP_TRAP)) {
                g.m.defensive = obj;
                g.m.has_defense = (mon_has_amulet(mtmp))
                                    ? MUSE_WAN_TELEPORTATION
                                    : MUSE_WAN_TELEPORTATION_SELF;
            }
        }
        nomore(MUSE_SCR_TELEPORTATION);
        if (obj->otyp == SCR_TELEPORTATION && mtmp->mcansee
            && haseyes(mtmp->data)
            && (!obj->cursed || (!(mtmp->isshk && inhishop(mtmp))
                                 && !mtmp->isgd && !mtmp->ispriest))) {
            /* see WAN_TELEPORTATION case above */
            if (!noteleport_level(mtmp)
                || !mon_knows_traps(mtmp, TELEP_TRAP)) {
                g.m.defensive = obj;
                g.m.has_defense = MUSE_SCR_TELEPORTATION;
            }
        }

        if (mtmp->data != &mons[PM_PESTILENCE]) {
            nomore(MUSE_POT_FULL_HEALING);
            if (obj->otyp == POT_FULL_HEALING) {
                g.m.defensive = obj;
                g.m.has_defense = MUSE_POT_FULL_HEALING;
            }
            nomore(MUSE_POT_EXTRA_HEALING);
            if (obj->otyp == POT_EXTRA_HEALING) {
                g.m.defensive = obj;
                g.m.has_defense = MUSE_POT_EXTRA_HEALING;
            }
            nomore(MUSE_WAN_CREATE_MONSTER);
            if (obj->otyp == WAN_CREATE_MONSTER && obj->spe > 0) {
                g.m.defensive = obj;
                g.m.has_defense = MUSE_WAN_CREATE_MONSTER;
            }
            nomore(MUSE_POT_HEALING);
            if (obj->otyp == POT_HEALING) {
                g.m.defensive = obj;
                g.m.has_defense = MUSE_POT_HEALING;
            }
        } else { /* Pestilence */
            nomore(MUSE_POT_FULL_HEALING);
            if (obj->otyp == POT_SICKNESS) {
                g.m.defensive = obj;
                g.m.has_defense = MUSE_POT_FULL_HEALING;
            }
            nomore(MUSE_WAN_CREATE_MONSTER);
            if (obj->otyp == WAN_CREATE_MONSTER && obj->spe > 0) {
                g.m.defensive = obj;
                g.m.has_defense = MUSE_WAN_CREATE_MONSTER;
            }
        }
        nomore(MUSE_SCR_CREATE_MONSTER);
        if (obj->otyp == SCR_CREATE_MONSTER) {
            g.m.defensive = obj;
            g.m.has_defense = MUSE_SCR_CREATE_MONSTER;
        }
    }
 botm:
    return (boolean) !!g.m.has_defense;
#undef nomore
}

/* Perform a defensive action for a monster.  Must be called immediately
 * after find_defensive().  Return values are 0: did something, 1: died,
 * 2: did something and can't attack again (i.e. teleported).
 */
int
use_defensive(struct monst* mtmp)
{
    int i, fleetim, how = 0;
    struct obj *otmp = g.m.defensive;
    boolean vis, vismon, oseen;
    const char *Mnam;
    stairway *stway;

    if ((i = precheck(mtmp, otmp)) != 0)
        return i;
    vis = cansee(mtmp->mx, mtmp->my);
    vismon = canseemon(mtmp);
    oseen = otmp && vismon;

    /* when using defensive choice to run away, we want monster to avoid
       rushing right straight back; don't override if already scared */
    fleetim = !mtmp->mflee ? (33 - (30 * mtmp->mhp / mtmp->mhpmax)) : 0;
#define m_flee(m)                          \
    if (fleetim && !m->iswiz) {            \
        monflee(m, fleetim, FALSE, FALSE); \
    }

    switch (g.m.has_defense) {
    case MUSE_UNICORN_HORN:
        if (vismon) {
            if (otmp)
                pline("%s uses a unicorn horn!", Monnam(mtmp));
            else
                pline_The("tip of %s's horn glows!", mon_nam(mtmp));
        }
        if (!mtmp->mcansee) {
            mcureblindness(mtmp, vismon);
        } else if (mtmp->mconf || mtmp->mstun) {
            mtmp->mconf = mtmp->mstun = 0;
            if (vismon)
                pline("%s seems steadier now.", Monnam(mtmp));
        } else
            impossible("No need for unicorn horn?");
        return 2;
    case MUSE_BUGLE:
        if (vismon)
            pline("%s plays %s!", Monnam(mtmp), doname(otmp));
        else if (!Deaf)
            You_hear("a bugle playing reveille!");
        awaken_soldiers(mtmp);
        return 2;
    case MUSE_WAN_TELEPORTATION_SELF:
        if ((mtmp->isshk && inhishop(mtmp)) || mtmp->isgd || mtmp->ispriest)
            return 2;
        m_flee(mtmp);
        mzapwand(mtmp, otmp, TRUE);
        how = WAN_TELEPORTATION;
 mon_tele:
        if (tele_restrict(mtmp)) { /* mysterious force... */
            if (vismon && how)     /* mentions 'teleport' */
                makeknown(how);
            /* monster learns that teleportation isn't useful here */
            if (noteleport_level(mtmp))
                mon_learns_traps(mtmp, TELEP_TRAP);
            return 2;
        }
        if ((mon_has_amulet(mtmp) || On_W_tower_level(&u.uz)) && !rn2(3)) {
            if (vismon)
                pline("%s seems disoriented for a moment.", Monnam(mtmp));
            return 2;
        }
        if (oseen && how)
            makeknown(how);
        (void) rloc(mtmp, RLOC_MSG);
        return 2;
    case MUSE_WAN_TELEPORTATION:
        g.zap_oseen = oseen;
        mzapwand(mtmp, otmp, FALSE);
        g.m_using = TRUE;
        mbhit(mtmp, rn1(8, 6), mbhitm, bhito, otmp);
        /* monster learns that teleportation isn't useful here */
        if (noteleport_level(mtmp))
            mon_learns_traps(mtmp, TELEP_TRAP);
        g.m_using = FALSE;
        return 2;
    case MUSE_SCR_TELEPORTATION: {
        int obj_is_cursed = otmp->cursed;

        if (mtmp->isshk || mtmp->isgd || mtmp->ispriest)
            return 2;
        m_flee(mtmp);
        mreadmsg(mtmp, otmp);
        m_useup(mtmp, otmp); /* otmp might be free'ed */
        how = SCR_TELEPORTATION;
        if (obj_is_cursed || mtmp->mconf) {
            int nlev;
            d_level flev;

            if (mon_has_amulet(mtmp) || In_endgame(&u.uz)) {
                if (vismon)
                    pline("%s seems very disoriented for a moment.",
                          Monnam(mtmp));
                return 2;
            }
            nlev = random_teleport_level();
            if (nlev == depth(&u.uz)) {
                if (vismon)
                    pline("%s shudders for a moment.", Monnam(mtmp));
                return 2;
            }
            get_level(&flev, nlev);
            migrate_to_level(mtmp, ledger_no(&flev), MIGR_RANDOM,
                             (coord *) 0);
            if (oseen)
                makeknown(SCR_TELEPORTATION);
        } else
            goto mon_tele;
        return 2;
    }
    case MUSE_WAN_DIGGING: {
        struct trap *ttmp;

        m_flee(mtmp);
        mzapwand(mtmp, otmp, FALSE);
        if (oseen)
            makeknown(WAN_DIGGING);
        if (IS_FURNITURE(levl[mtmp->mx][mtmp->my].typ)
            || IS_DRAWBRIDGE(levl[mtmp->mx][mtmp->my].typ)
            || (is_drawbridge_wall(mtmp->mx, mtmp->my) >= 0)
            || stairway_at(mtmp->mx, mtmp->my)) {
            pline_The("digging ray is ineffective.");
            return 2;
        }
        if (!Can_dig_down(&u.uz) && !levl[mtmp->mx][mtmp->my].candig) {
            /* can't dig further if there's already a pit (or other trap)
               here, or if pit creation fails for some reason */
            if (t_at(mtmp->mx, mtmp->my)
                || !(ttmp = maketrap(mtmp->mx, mtmp->my, PIT))) {
                if (vismon) {
                    pline_The("%s here is too hard to dig in.",
                              surface(mtmp->mx, mtmp->my));
                }
                return 2;
            }
            /* pit creation succeeded */
            if (vis) {
                seetrap(ttmp);
                pline("%s has made a pit in the %s.", Monnam(mtmp),
                      surface(mtmp->mx, mtmp->my));
            }
            return (mintrap(mtmp, FORCEBUNGLE) == Trap_Killed_Mon) ? 1 : 2;
        }
        ttmp = maketrap(mtmp->mx, mtmp->my, HOLE);
        if (!ttmp)
            return 2;
        seetrap(ttmp);
        if (vis) {
            pline("%s has made a hole in the %s.", Monnam(mtmp),
                  surface(mtmp->mx, mtmp->my));
            pline("%s %s through...", Monnam(mtmp),
                  is_flyer(mtmp->data) ? "dives" : "falls");
        } else if (!Deaf)
            You_hear("%s crash through the %s.", something,
                     surface(mtmp->mx, mtmp->my));
        /* we made sure that there is a level for mtmp to go to */
        migrate_to_level(mtmp, ledger_no(&u.uz) + 1, MIGR_RANDOM,
                         (coord *) 0);
        return 2;
    }
    case MUSE_WAN_UNDEAD_TURNING:
        g.zap_oseen = oseen;
        mzapwand(mtmp, otmp, FALSE);
        g.m_using = TRUE;
        mbhit(mtmp, rn1(8, 6), mbhitm, bhito, otmp);
        g.m_using = FALSE;
        return 2;
    case MUSE_WAN_CREATE_MONSTER: {
        coord cc;
        struct monst *mon;
        /* pm: 0 => random, eel => aquatic, croc => amphibious */
        struct permonst *pm = !is_pool(mtmp->mx, mtmp->my) ? 0
                            : &mons[u.uinwater ? PM_GIANT_EEL : PM_CROCODILE];

        if (!enexto(&cc, mtmp->mx, mtmp->my, pm))
            return 0;
        mzapwand(mtmp, otmp, FALSE);
        mon = makemon((struct permonst *) 0, cc.x, cc.y, NO_MM_FLAGS);
        if (mon && canspotmon(mon) && oseen)
            makeknown(WAN_CREATE_MONSTER);
        return 2;
    }
    case MUSE_SCR_CREATE_MONSTER: {
        coord cc;
        struct permonst *pm = 0, *fish = 0;
        int cnt = 1;
        struct monst *mon;
        boolean known = FALSE;

        if (!rn2(73))
            cnt += rnd(4);
        if (mtmp->mconf || otmp->cursed)
            cnt += 12;
        if (mtmp->mconf)
            pm = fish = &mons[PM_ACID_BLOB];
        else if (is_pool(mtmp->mx, mtmp->my))
            fish = &mons[u.uinwater ? PM_GIANT_EEL : PM_CROCODILE];
        mreadmsg(mtmp, otmp);
        while (cnt--) {
            /* `fish' potentially gives bias towards water locations;
               `pm' is what to actually create (0 => random) */
            if (!enexto(&cc, mtmp->mx, mtmp->my, fish))
                break;
            mon = makemon(pm, cc.x, cc.y, NO_MM_FLAGS);
            if (mon && canspotmon(mon))
                known = TRUE;
        }
        /* The only case where we don't use oseen.  For wands, you
         * have to be able to see the monster zap the wand to know
         * what type it is.  For teleport scrolls, you have to see
         * the monster to know it teleported.
         */
        if (known)
            makeknown(SCR_CREATE_MONSTER);
        else
            trycall(otmp);
        m_useup(mtmp, otmp);
        return 2;
    }
    case MUSE_TRAPDOOR:
        /* trap doors on "bottom" levels of dungeons are rock-drop
         * trap doors, not holes in the floor.  We check here for
         * safety.
         */
        if (Is_botlevel(&u.uz))
            return 0;
        m_flee(mtmp);
        if (vis) {
            struct trap *t = t_at(g.trapx, g.trapy);

            Mnam = Monnam(mtmp);
            pline("%s %s into a %s!", Mnam,
                  vtense(fakename[0], locomotion(mtmp->data, "jump")),
                  (t->ttyp == TRAPDOOR) ? "trap door" : "hole");
            if (levl[g.trapx][g.trapy].typ == SCORR) {
                levl[g.trapx][g.trapy].typ = CORR;
                unblock_point(g.trapx, g.trapy);
            }
            seetrap(t_at(g.trapx, g.trapy));
        }

        /*  don't use rloc_to() because worm tails must "move" */
        remove_monster(mtmp->mx, mtmp->my);
        newsym(mtmp->mx, mtmp->my); /* update old location */
        place_monster(mtmp, g.trapx, g.trapy);
        if (mtmp->wormno)
            worm_move(mtmp);
        newsym(g.trapx, g.trapy);

        migrate_to_level(mtmp, ledger_no(&u.uz) + 1, MIGR_RANDOM,
                         (coord *) 0);
        return 2;
    case MUSE_UPSTAIRS:
        m_flee(mtmp);
        stway = stairway_at(mtmp->mx, mtmp->my);
        if (!stway)
            return 0;
        if (ledger_no(&u.uz) == 1)
            goto escape; /* impossible; level 1 upstairs are SSTAIRS */
        if (Inhell && mon_has_amulet(mtmp) && !rn2(4)
            && (dunlev(&u.uz) < dunlevs_in_dungeon(&u.uz) - 3)) {
            if (vismon)
                pline(
    "As %s climbs the stairs, a mysterious force momentarily surrounds %s...",
                      mon_nam(mtmp), mhim(mtmp));
            /* simpler than for the player; this will usually be
               the Wizard and he'll immediately go right to the
               upstairs, so there's not much point in having any
               chance for a random position on the current level */
            migrate_to_level(mtmp, ledger_no(&u.uz) + 1, MIGR_RANDOM,
                             (coord *) 0);
        } else {
            if (vismon)
                pline("%s escapes upstairs!", Monnam(mtmp));
            migrate_to_level(mtmp, ledger_no(&(stway->tolev)), MIGR_STAIRS_DOWN,
                             (coord *) 0);
        }
        return 2;
    case MUSE_DOWNSTAIRS:
        m_flee(mtmp);
        stway = stairway_at(mtmp->mx, mtmp->my);
        if (!stway)
            return 0;
        if (vismon)
            pline("%s escapes downstairs!", Monnam(mtmp));
        migrate_to_level(mtmp, ledger_no(&(stway->tolev)), MIGR_STAIRS_UP,
                         (coord *) 0);
        return 2;
    case MUSE_UP_LADDER:
        m_flee(mtmp);
        stway = stairway_at(mtmp->mx, mtmp->my);
        if (!stway)
            return 0;
        if (vismon)
            pline("%s escapes up the ladder!", Monnam(mtmp));
        migrate_to_level(mtmp, ledger_no(&(stway->tolev)), MIGR_LADDER_DOWN,
                         (coord *) 0);
        return 2;
    case MUSE_DN_LADDER:
        m_flee(mtmp);
        stway = stairway_at(mtmp->mx, mtmp->my);
        if (!stway)
            return 0;
        if (vismon)
            pline("%s escapes down the ladder!", Monnam(mtmp));
        migrate_to_level(mtmp, ledger_no(&(stway->tolev)), MIGR_LADDER_UP,
                         (coord *) 0);
        return 2;
    case MUSE_SSTAIRS:
        m_flee(mtmp);
        stway = stairway_at(mtmp->mx, mtmp->my);
        if (!stway)
            return 0;
        if (ledger_no(&u.uz) == 1) {
 escape:
            /* Monsters without the Amulet escape the dungeon and
             * are gone for good when they leave up the up stairs.
             * A monster with the Amulet would leave it behind
             * (mongone -> mdrop_special_objs) but we force any
             * monster who manages to acquire it or the invocation
             * tools to stick around instead of letting it escape.
             * Don't let the Wizard escape even when not carrying
             * anything of interest unless there are more than 1
             * of him.
             */
            if (mon_has_special(mtmp)
                || (mtmp->iswiz && g.context.no_of_wizards < 2))
                return 0;
            if (vismon)
                pline("%s escapes the dungeon!", Monnam(mtmp));
            mongone(mtmp);
            return 2;
        }
        if (vismon)
            pline("%s escapes %sstairs!", Monnam(mtmp),
                  stway->up ? "up" : "down");
        /* going from the Valley to Castle (Stronghold) has no sstairs
           to target, but having g.sstairs.<sx,sy> == <0,0> will work the
           same as specifying MIGR_RANDOM when mon_arrive() eventually
           places the monster, so we can use MIGR_SSTAIRS unconditionally */
        migrate_to_level(mtmp, ledger_no(&(stway->tolev)), MIGR_SSTAIRS,
                         (coord *) 0);
        return 2;
    case MUSE_TELEPORT_TRAP:
        m_flee(mtmp);
        if (vis) {
            Mnam = Monnam(mtmp);
            pline("%s %s onto a teleport trap!", Mnam,
                  vtense(fakename[0], locomotion(mtmp->data, "jump")));
            seetrap(t_at(g.trapx, g.trapy));
        }
        /*  don't use rloc_to() because worm tails must "move" */
        remove_monster(mtmp->mx, mtmp->my);
        newsym(mtmp->mx, mtmp->my); /* update old location */
        place_monster(mtmp, g.trapx, g.trapy);
        if (mtmp->wormno)
            worm_move(mtmp);
        newsym(g.trapx, g.trapy);

        goto mon_tele;
    case MUSE_POT_HEALING:
        mquaffmsg(mtmp, otmp);
        i = d(6 + 2 * bcsign(otmp), 4);
        mtmp->mhp += i;
        if (mtmp->mhp > mtmp->mhpmax)
            mtmp->mhp = ++mtmp->mhpmax;
        if (!otmp->cursed && !mtmp->mcansee)
            mcureblindness(mtmp, vismon);
        if (vismon)
            pline("%s looks better.", Monnam(mtmp));
        if (oseen)
            makeknown(POT_HEALING);
        m_useup(mtmp, otmp);
        return 2;
    case MUSE_POT_EXTRA_HEALING:
        mquaffmsg(mtmp, otmp);
        i = d(6 + 2 * bcsign(otmp), 8);
        mtmp->mhp += i;
        if (mtmp->mhp > mtmp->mhpmax)
            mtmp->mhp = (mtmp->mhpmax += (otmp->blessed ? 5 : 2));
        if (!mtmp->mcansee)
            mcureblindness(mtmp, vismon);
        if (vismon)
            pline("%s looks much better.", Monnam(mtmp));
        if (oseen)
            makeknown(POT_EXTRA_HEALING);
        m_useup(mtmp, otmp);
        return 2;
    case MUSE_POT_FULL_HEALING:
        mquaffmsg(mtmp, otmp);
        if (otmp->otyp == POT_SICKNESS)
            unbless(otmp); /* Pestilence */
        mtmp->mhp = (mtmp->mhpmax += (otmp->blessed ? 8 : 4));
        if (!mtmp->mcansee && otmp->otyp != POT_SICKNESS)
            mcureblindness(mtmp, vismon);
        if (vismon)
            pline("%s looks completely healed.", Monnam(mtmp));
        if (oseen)
            makeknown(otmp->otyp);
        m_useup(mtmp, otmp);
        return 2;
    case MUSE_LIZARD_CORPSE:
        /* not actually called for its unstoning effect */
        mon_consume_unstone(mtmp, otmp, FALSE, FALSE);
        return 2;
    case 0:
        return 0; /* i.e. an exploded wand */
    default:
        impossible("%s wanted to perform action %d?", Monnam(mtmp),
                   g.m.has_defense);
        break;
    }
    return 0;
#undef m_flee
}

int
rnd_defensive_item(struct monst* mtmp)
{
    struct permonst *pm = mtmp->data;
    int difficulty = mons[(monsndx(pm))].difficulty;
    int trycnt = 0;

    if (is_animal(pm) || attacktype(pm, AT_EXPL) || mindless(mtmp->data)
        || pm->mlet == S_GHOST || pm->mlet == S_KOP)
        return 0;
 try_again:
    switch (rn2(8 + (difficulty > 3) + (difficulty > 6) + (difficulty > 8))) {
    case 6:
    case 9:
        if (noteleport_level(mtmp) && ++trycnt < 2)
            goto try_again;
        if (!rn2(3))
            return WAN_TELEPORTATION;
        /*FALLTHRU*/
    case 0:
    case 1:
        return SCR_TELEPORTATION;
    case 8:
    case 10:
        if (!rn2(3))
            return WAN_CREATE_MONSTER;
        /*FALLTHRU*/
    case 2:
        return SCR_CREATE_MONSTER;
    case 3:
        return POT_HEALING;
    case 4:
        return POT_EXTRA_HEALING;
    case 5:
        return (mtmp->data != &mons[PM_PESTILENCE]) ? POT_FULL_HEALING
                                                    : POT_SICKNESS;
    case 7: /* wand of digging */
        /* usually avoid digging in Sokoban */
        if (Sokoban && rn2(4))
            goto try_again;
        /* some creatures shouldn't dig down to another level when hurt */
        if (is_floater(pm) || mtmp->isshk || mtmp->isgd || mtmp->ispriest)
            return 0;
        return WAN_DIGGING;
    }
    /*NOTREACHED*/
    return 0;
}

#define MUSE_WAN_DEATH 1
#define MUSE_WAN_SLEEP 2
#define MUSE_WAN_FIRE 3
#define MUSE_WAN_COLD 4
#define MUSE_WAN_LIGHTNING 5
#define MUSE_WAN_MAGIC_MISSILE 6
#define MUSE_WAN_STRIKING 7
#define MUSE_SCR_FIRE 8
#define MUSE_POT_PARALYSIS 9
#define MUSE_POT_BLINDNESS 10
#define MUSE_POT_CONFUSION 11
#define MUSE_FROST_HORN 12
#define MUSE_FIRE_HORN 13
#define MUSE_POT_ACID 14
#define MUSE_WAN_TELEPORTATION 15
#define MUSE_POT_SLEEPING 16
#define MUSE_SCR_EARTH 17
#define MUSE_CAMERA 18
/*#define MUSE_WAN_UNDEAD_TURNING 20*/ /* also a defensive item so don't
                                     * redefine; nonconsecutive value is ok */

static boolean
linedup_chk_corpse(coordxy x, coordxy y)
{
    return (sobj_at(CORPSE, x, y) != 0);
}

static void
m_use_undead_turning(struct monst* mtmp, struct obj* obj)
{
    coordxy ax = u.ux + sgn(mtmp->mux - mtmp->mx) * 3,
            ay = u.uy + sgn(mtmp->muy - mtmp->my) * 3;
    coordxy bx = mtmp->mx, by = mtmp->my;

    if (!(obj->otyp == WAN_UNDEAD_TURNING && obj->spe > 0))
        return;

    /* not necrophiliac(); unlike deciding whether to pick this
       type of wand up, we aren't interested in corpses within
       carried containers until they're moved into open inventory;
       we don't check whether hero is poly'd into an undead--the
       wand's turning effect is too weak to be a useful direct
       attack--only whether hero is carrying at least one corpse */
    if (carrying(CORPSE)) {
        /*
         * Hero is carrying one or more corpses but isn't wielding
         * a cockatrice corpse (unless being hit by one won't do
         * the monster much harm); otherwise we'd be using this wand
         * as a defensive item with higher priority.
         *
         * Might be cockatrice intended as a weapon (or being denied
         * to glove-wearing monsters for use as a weapon) or lizard
         * intended as a cure or lichen intended as veggy food or
         * sacrifice fodder being lugged to an altar.  Zapping with
         * this will deprive hero of one from each stack although
         * they might subsequently be recovered after killing again.
         * In the sacrifice fodder case, it could even be to the
         * player's advantage (fresher corpse if a new one gets
         * dropped; player might not choose to spend a wand charge
         * on that when/if hero acquires this wand).
         */
        g.m.offensive = obj;
        g.m.has_offense = MUSE_WAN_UNDEAD_TURNING;
    } else if (linedup_callback(ax, ay, bx, by, linedup_chk_corpse)) {
        /* There's a corpse on the ground in a direct line from the
         * monster to the hero, and up to 3 steps beyond.
         */
        g.m.offensive = obj;
        g.m.has_offense = MUSE_WAN_UNDEAD_TURNING;
    }
}

/* from monster's point of view, is hero behind a chokepoint? */
static boolean
hero_behind_chokepoint(struct monst *mtmp)
{
    coordxy dx = sgn(mtmp->mx - mtmp->mux);
    coordxy dy = sgn(mtmp->my - mtmp->muy);

    coordxy x = mtmp->mux + dx;
    coordxy y = mtmp->muy + dy;

    int dir = xytod(dx, dy);
    int dir_l = DIR_CLAMP(DIR_LEFT2(dir));
    int dir_r = DIR_CLAMP(DIR_RIGHT2(dir));

    coord c1, c2;

    dtoxy(&c1, dir_l);
    dtoxy(&c2, dir_r);
    c1.x += x, c2.x += x;
    c1.y += y, c2.y += y;

    if ((!isok(c1.x, c1.y) || !accessible(c1.x, c1.y))
        && (!isok(c2.x, c2.y) || !accessible(c2.x, c2.y)))
        return TRUE;
    return FALSE;
}

/* hostile monster has another hostile next to it */
static boolean
mon_has_friends(struct monst *mtmp)
{
    coordxy dx, dy;
    struct monst *mon2;

    if (mtmp->mtame || mtmp->mpeaceful)
        return FALSE;

    for (dx = -1; dx <= 1; dx++)
        for (dy = -1; dy <= 1; dy++) {
            coordxy x = mtmp->mx + dx;
            coordxy y = mtmp->my + dy;

            if (isok(x, y) && (mon2 = m_at(x, y)) != 0
                && mon2 != mtmp
                && !mon2->mtame && !mon2->mpeaceful)
                return TRUE;
        }

    return FALSE;
}

/* Select an offensive item/action for a monster.  Returns TRUE iff one is
 * found.
 */
boolean
find_offensive(struct monst* mtmp)
{
    register struct obj *obj;
    boolean reflection_skip = m_seenres(mtmp, M_SEEN_REFL) != 0
        || monnear(mtmp, mtmp->mux, mtmp->muy);
    struct obj *helmet = which_armor(mtmp, W_ARMH);

    g.m.offensive = (struct obj *) 0;
    g.m.has_offense = 0;
    if (mtmp->mpeaceful || is_animal(mtmp->data) || mindless(mtmp->data)
        || nohands(mtmp->data))
        return FALSE;
    if (u.uswallow)
        return FALSE;
    if (in_your_sanctuary(mtmp, 0, 0))
        return FALSE;
    if (dmgtype(mtmp->data, AD_HEAL)
        && !uwep && !uarmu && !uarm && !uarmh
        && !uarms && !uarmg && !uarmc && !uarmf)
        return FALSE;
    /* all offensive items require orthogonal or diagonal targeting */
    if (!lined_up(mtmp))
        return FALSE;

#define nomore(x)       if (g.m.has_offense == x) continue;
    /* this picks the last viable item rather than prioritizing choices */
    for (obj = mtmp->minvent; obj; obj = obj->nobj) {
        if (!reflection_skip) {
            nomore(MUSE_WAN_DEATH);
            if (obj->otyp == WAN_DEATH && obj->spe > 0
                && !m_seenres(mtmp, M_SEEN_MAGR)) {
                g.m.offensive = obj;
                g.m.has_offense = MUSE_WAN_DEATH;
            }
            nomore(MUSE_WAN_SLEEP);
            if (obj->otyp == WAN_SLEEP && obj->spe > 0 && g.multi >= 0
                && !m_seenres(mtmp, M_SEEN_SLEEP)) {
                g.m.offensive = obj;
                g.m.has_offense = MUSE_WAN_SLEEP;
            }
            nomore(MUSE_WAN_FIRE);
            if (obj->otyp == WAN_FIRE && obj->spe > 0
                && !m_seenres(mtmp, M_SEEN_FIRE)) {
                g.m.offensive = obj;
                g.m.has_offense = MUSE_WAN_FIRE;
            }
            nomore(MUSE_FIRE_HORN);
            if (obj->otyp == FIRE_HORN && obj->spe > 0 && can_blow(mtmp)
                && !m_seenres(mtmp, M_SEEN_FIRE)) {
                g.m.offensive = obj;
                g.m.has_offense = MUSE_FIRE_HORN;
            }
            nomore(MUSE_WAN_COLD);
            if (obj->otyp == WAN_COLD && obj->spe > 0
                && !m_seenres(mtmp, M_SEEN_COLD)) {
                g.m.offensive = obj;
                g.m.has_offense = MUSE_WAN_COLD;
            }
            nomore(MUSE_FROST_HORN);
            if (obj->otyp == FROST_HORN && obj->spe > 0 && can_blow(mtmp)
                && !m_seenres(mtmp, M_SEEN_COLD)) {
                g.m.offensive = obj;
                g.m.has_offense = MUSE_FROST_HORN;
            }
            nomore(MUSE_WAN_LIGHTNING);
            if (obj->otyp == WAN_LIGHTNING && obj->spe > 0
                && !m_seenres(mtmp, M_SEEN_ELEC)) {
                g.m.offensive = obj;
                g.m.has_offense = MUSE_WAN_LIGHTNING;
            }
            nomore(MUSE_WAN_MAGIC_MISSILE);
            if (obj->otyp == WAN_MAGIC_MISSILE && obj->spe > 0
                && !m_seenres(mtmp, M_SEEN_MAGR)) {
                g.m.offensive = obj;
                g.m.has_offense = MUSE_WAN_MAGIC_MISSILE;
            }
        }
        nomore(MUSE_WAN_UNDEAD_TURNING);
        m_use_undead_turning(mtmp, obj);
        nomore(MUSE_WAN_STRIKING);
        if (obj->otyp == WAN_STRIKING && obj->spe > 0
            && !m_seenres(mtmp, M_SEEN_MAGR)) {
            g.m.offensive = obj;
            g.m.has_offense = MUSE_WAN_STRIKING;
        }
        nomore(MUSE_WAN_TELEPORTATION);
        if (obj->otyp == WAN_TELEPORTATION && obj->spe > 0
            /* don't give controlled hero a free teleport */
            && !Teleport_control
            /* same hack as MUSE_WAN_TELEPORTATION_SELF */
            && (!noteleport_level(mtmp)
                || !mon_knows_traps(mtmp, TELEP_TRAP))
            /* do try to move hero to a more vulnerable spot */
            && (onscary(u.ux, u.uy, mtmp)
                || (hero_behind_chokepoint(mtmp) && mon_has_friends(mtmp))
                || (stairway_at(u.ux, u.uy)))) {
            g.m.offensive = obj;
            g.m.has_offense = MUSE_WAN_TELEPORTATION;
        }
        nomore(MUSE_POT_PARALYSIS);
        if (obj->otyp == POT_PARALYSIS && g.multi >= 0) {
            g.m.offensive = obj;
            g.m.has_offense = MUSE_POT_PARALYSIS;
        }
        nomore(MUSE_POT_BLINDNESS);
        if (obj->otyp == POT_BLINDNESS && !attacktype(mtmp->data, AT_GAZE)) {
            g.m.offensive = obj;
            g.m.has_offense = MUSE_POT_BLINDNESS;
        }
        nomore(MUSE_POT_CONFUSION);
        if (obj->otyp == POT_CONFUSION) {
            g.m.offensive = obj;
            g.m.has_offense = MUSE_POT_CONFUSION;
        }
        nomore(MUSE_POT_SLEEPING);
        if (obj->otyp == POT_SLEEPING
            && !m_seenres(mtmp, M_SEEN_SLEEP)) {
            g.m.offensive = obj;
            g.m.has_offense = MUSE_POT_SLEEPING;
        }
        nomore(MUSE_POT_ACID);
        if (obj->otyp == POT_ACID
            && !m_seenres(mtmp, M_SEEN_ACID)) {
            g.m.offensive = obj;
            g.m.has_offense = MUSE_POT_ACID;
        }
        /* we can safely put this scroll here since the locations that
         * are in a 1 square radius are a subset of the locations that
         * are in wand or throwing range (in other words, always lined_up())
         */
        nomore(MUSE_SCR_EARTH);
        if (obj->otyp == SCR_EARTH
            && ((helmet && is_metallic(helmet)) || mtmp->mconf
                || amorphous(mtmp->data) || passes_walls(mtmp->data)
                || noncorporeal(mtmp->data) || unsolid(mtmp->data)
                || !rn2(10))
            && dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= 2
            && mtmp->mcansee && haseyes(mtmp->data)
            && !Is_rogue_level(&u.uz)
            && (!In_endgame(&u.uz) || Is_earthlevel(&u.uz))) {
            g.m.offensive = obj;
            g.m.has_offense = MUSE_SCR_EARTH;
        }
        nomore(MUSE_CAMERA);
        if (obj->otyp == EXPENSIVE_CAMERA
            && (!Blind || hates_light(g.youmonst.data))
            && dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= 2
            && obj->spe > 0 && !rn2(6)) {
            g.m.offensive = obj;
            g.m.has_offense = MUSE_CAMERA;
        }
#if 0
        nomore(MUSE_SCR_FIRE);
        if (obj->otyp == SCR_FIRE && resists_fire(mtmp)
            && dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= 2
            && mtmp->mcansee && haseyes(mtmp->data)
            && !m_seenres(mtmp, M_SEEN_FIRE)) {
            g.m.offensive = obj;
            g.m.has_offense = MUSE_SCR_FIRE;
        }
#endif /* 0 */
    }
    return (boolean) !!g.m.has_offense;
#undef nomore
}

static
int
mbhitm(register struct monst* mtmp, register struct obj* otmp)
{
    int tmp;
    boolean reveal_invis = FALSE, hits_you = (mtmp == &g.youmonst);

    if (!hits_you && otmp->otyp != WAN_UNDEAD_TURNING) {
        mtmp->msleeping = 0;
        if (mtmp->m_ap_type)
            seemimic(mtmp);
    }
    switch (otmp->otyp) {
    case WAN_STRIKING:
        reveal_invis = TRUE;
        if (hits_you) {
            if (Antimagic) {
                shieldeff(u.ux, u.uy);
                pline("Boing!");
            } else if (rnd(20) < 10 + u.uac) {
                pline_The("wand hits you!");
                tmp = d(2, 12);
                if (Half_spell_damage)
                    tmp = (tmp + 1) / 2;
                losehp(tmp, "wand", KILLED_BY_AN);
            } else
                pline_The("wand misses you.");
            stop_occupation();
            nomul(0);
        } else if (resists_magm(mtmp)) {
            shieldeff(mtmp->mx, mtmp->my);
            pline("Boing!");
        } else if (rnd(20) < 10 + find_mac(mtmp)) {
            tmp = d(2, 12);
            hit("wand", mtmp, exclam(tmp));
            (void) resist(mtmp, otmp->oclass, tmp, TELL);
            if (cansee(mtmp->mx, mtmp->my) && g.zap_oseen)
                makeknown(WAN_STRIKING);
        } else {
            miss("wand", mtmp);
            if (cansee(mtmp->mx, mtmp->my) && g.zap_oseen)
                makeknown(WAN_STRIKING);
        }
        break;
    case WAN_TELEPORTATION:
        if (hits_you) {
            tele();
            if (g.zap_oseen)
                makeknown(WAN_TELEPORTATION);
        } else {
            /* for consistency with zap.c, don't identify */
            if (mtmp->ispriest && *in_rooms(mtmp->mx, mtmp->my, TEMPLE)) {
                if (cansee(mtmp->mx, mtmp->my))
                    pline("%s resists the magic!", Monnam(mtmp));
            } else if (!tele_restrict(mtmp))
                (void) rloc(mtmp, RLOC_MSG);
        }
        break;
    case WAN_CANCELLATION:
    case SPE_CANCELLATION:
        (void) cancel_monst(mtmp, otmp, FALSE, TRUE, FALSE);
        break;
    case WAN_UNDEAD_TURNING: {
        boolean learnit = FALSE;

        if (hits_you) {
            unturn_you();
            learnit = g.zap_oseen;
        } else {
            boolean wake = FALSE;

            if (unturn_dead(mtmp)) /* affects mtmp's invent, not mtmp */
                wake = TRUE;
            if (is_undead(mtmp->data) || is_vampshifter(mtmp)) {
                wake = reveal_invis = TRUE;
                /* context.bypasses=True: if resist() happens to be fatal,
                   make_corpse() will set obj->bypass on the new corpse
                   so that mbhito() will skip it instead of reviving it */
                g.context.bypasses = TRUE; /* for make_corpse() */
                (void) resist(mtmp, WAND_CLASS, rnd(8), NOTELL);
            }
            if (wake) {
                if (!DEADMONSTER(mtmp))
                    wakeup(mtmp, FALSE);
                learnit = g.zap_oseen;
            }
        }
        if (learnit)
            makeknown(WAN_UNDEAD_TURNING);
        break;
    }
    default:
        break;
    }
    if (reveal_invis && !DEADMONSTER(mtmp)
        && cansee(g.bhitpos.x, g.bhitpos.y) && !canspotmon(mtmp))
        map_invisible(g.bhitpos.x, g.bhitpos.y);

    return 0;
}

/* A modified bhit() for monsters.  Based on bhit() in zap.c.  Unlike
 * buzz(), bhit() doesn't take into account the possibility of a monster
 * zapping you, so we need a special function for it.  (Unless someone wants
 * to merge the two functions...)
 */
static void
mbhit(
    struct monst *mon,  /* monster shooting the wand */
    register int range, /* direction and range */
    int (*fhitm)(MONST_P, OBJ_P),
    int (*fhito)(OBJ_P, OBJ_P), /* fns called when mon/obj hit */
    struct obj *obj)                     /* 2nd arg to fhitm/fhito */
{
    register struct monst *mtmp;
    register struct obj *otmp;
    register uchar typ;
    int ddx, ddy;

    g.bhitpos.x = mon->mx;
    g.bhitpos.y = mon->my;
    ddx = sgn(mon->mux - mon->mx);
    ddy = sgn(mon->muy - mon->my);

    while (range-- > 0) {
        coordxy x, y;

        g.bhitpos.x += ddx;
        g.bhitpos.y += ddy;
        x = g.bhitpos.x;
        y = g.bhitpos.y;

        if (!isok(x, y)) {
            g.bhitpos.x -= ddx;
            g.bhitpos.y -= ddy;
            break;
        }
        if (find_drawbridge(&x, &y))
            switch (obj->otyp) {
            case WAN_STRIKING:
                destroy_drawbridge(x, y);
            }
        if (u_at(g.bhitpos.x, g.bhitpos.y)) {
            (*fhitm)(&g.youmonst, obj);
            range -= 3;
        } else if ((mtmp = m_at(g.bhitpos.x, g.bhitpos.y)) != 0) {
            if (cansee(g.bhitpos.x, g.bhitpos.y) && !canspotmon(mtmp))
                map_invisible(g.bhitpos.x, g.bhitpos.y);
            (*fhitm)(mtmp, obj);
            range -= 3;
        }
        /* modified by GAN to hit all objects */
        if (fhito) {
            int hitanything = 0;
            register struct obj *next_obj;

            for (otmp = g.level.objects[g.bhitpos.x][g.bhitpos.y]; otmp;
                 otmp = next_obj) {
                /* Fix for polymorph bug, Tim Wright */
                next_obj = otmp->nexthere;
                hitanything += (*fhito)(otmp, obj);
            }
            if (hitanything)
                range--;
        }
        typ = levl[g.bhitpos.x][g.bhitpos.y].typ;
        if (IS_DOOR(typ) || typ == SDOOR) {
            switch (obj->otyp) {
            /* note: monsters don't use opening or locking magic
               at present, but keep these as placeholders */
            case WAN_OPENING:
            case WAN_LOCKING:
            case WAN_STRIKING:
                if (doorlock(obj, g.bhitpos.x, g.bhitpos.y)) {
                    if (g.zap_oseen)
                        makeknown(obj->otyp);
                    /* if a shop door gets broken, add it to
                       the shk's fix list (no cost to player) */
                    if (levl[g.bhitpos.x][g.bhitpos.y].doormask == D_BROKEN
                        && *in_rooms(g.bhitpos.x, g.bhitpos.y, SHOPBASE))
                        add_damage(g.bhitpos.x, g.bhitpos.y, 0L);
                }
                break;
            }
        }
        if (!ZAP_POS(typ)
            || (IS_DOOR(typ) && (levl[g.bhitpos.x][g.bhitpos.y].doormask
                                 & (D_LOCKED | D_CLOSED)))) {
            g.bhitpos.x -= ddx;
            g.bhitpos.y -= ddy;
            break;
        }
    }
}

/* Perform an offensive action for a monster.  Must be called immediately
 * after find_offensive().  Return values are same as use_defensive().
 */
int
use_offensive(struct monst* mtmp)
{
    int i;
    struct obj *otmp = g.m.offensive;
    boolean oseen;

    /* offensive potions are not drunk, they're thrown */
    if (otmp->oclass != POTION_CLASS && (i = precheck(mtmp, otmp)) != 0)
        return i;
    oseen = otmp && canseemon(mtmp);

    switch (g.m.has_offense) {
    case MUSE_WAN_DEATH:
    case MUSE_WAN_SLEEP:
    case MUSE_WAN_FIRE:
    case MUSE_WAN_COLD:
    case MUSE_WAN_LIGHTNING:
    case MUSE_WAN_MAGIC_MISSILE:
        mzapwand(mtmp, otmp, FALSE);
        if (oseen)
            makeknown(otmp->otyp);
        g.m_using = TRUE;
        buzz(BZ_M_WAND(BZ_OFS_WAN(otmp->otyp)),
             (otmp->otyp == WAN_MAGIC_MISSILE) ? 2 : 6, mtmp->mx, mtmp->my,
             sgn(mtmp->mux - mtmp->mx), sgn(mtmp->muy - mtmp->my));
        g.m_using = FALSE;
        return (DEADMONSTER(mtmp)) ? 1 : 2;
    case MUSE_FIRE_HORN:
    case MUSE_FROST_HORN:
        mplayhorn(mtmp, otmp, FALSE);
        g.m_using = TRUE;
        buzz(BZ_M_WAND(BZ_OFS_AD((otmp->otyp == FROST_HORN) ? AD_COLD : AD_FIRE)),
             rn1(6, 6), mtmp->mx, mtmp->my, sgn(mtmp->mux - mtmp->mx),
             sgn(mtmp->muy - mtmp->my));
        g.m_using = FALSE;
        return (DEADMONSTER(mtmp)) ? 1 : 2;
    case MUSE_WAN_TELEPORTATION:
    case MUSE_WAN_UNDEAD_TURNING:
    case MUSE_WAN_STRIKING:
        g.zap_oseen = oseen;
        mzapwand(mtmp, otmp, FALSE);
        g.m_using = TRUE;
        mbhit(mtmp, rn1(8, 6), mbhitm, bhito, otmp);
        g.m_using = FALSE;
        return 2;
    case MUSE_SCR_EARTH: {
        /* TODO: handle steeds */
        coordxy x, y;
        /* don't use monster fields after killing it */
        boolean confused = (mtmp->mconf ? TRUE : FALSE);
        int mmx = mtmp->mx, mmy = mtmp->my;
        boolean is_cursed = otmp->cursed, is_blessed = otmp->blessed;

        mreadmsg(mtmp, otmp);
        /* Identify the scroll */
        if (canspotmon(mtmp)) {
            pline_The("%s rumbles %s %s!", ceiling(mtmp->mx, mtmp->my),
                      otmp->blessed ? "around" : "above", mon_nam(mtmp));
            if (oseen)
                makeknown(otmp->otyp);
        } else if (cansee(mtmp->mx, mtmp->my)) {
            pline_The("%s rumbles in the middle of nowhere!",
                      ceiling(mtmp->mx, mtmp->my));
            if (mtmp->minvis)
                map_invisible(mtmp->mx, mtmp->my);
            if (oseen)
                makeknown(otmp->otyp);
        }

        /* could be fatal to monster, so use up the scroll before
           there's a chance that monster's inventory will be dropped */
        m_useup(mtmp, otmp);

        /* Loop through the surrounding squares */
        for (x = mmx - 1; x <= mmx + 1; x++) {
            for (y = mmy - 1; y <= mmy + 1; y++) {
                /* Is this a suitable spot? */
                if (isok(x, y) && !closed_door(x, y)
                    && !IS_ROCK(levl[x][y].typ) && !IS_AIR(levl[x][y].typ)
                    && (((x == mmx) && (y == mmy)) ? !is_blessed : !is_cursed)
                    && (x != u.ux || y != u.uy)) {
                    (void) drop_boulder_on_monster(x, y, confused, FALSE);
                }
            }
        }
        /* Attack the player */
        if (distmin(mmx, mmy, u.ux, u.uy) == 1 && !is_cursed) {
            drop_boulder_on_player(confused, !is_cursed, FALSE, TRUE);
        }

        return (DEADMONSTER(mtmp)) ? 1 : 2;
    } /* case MUSE_SCR_EARTH */
    case MUSE_CAMERA: {
        if (Hallucination)
            verbalize("Say cheese!");
        else
            pline("%s takes a picture of you with %s!",
                  Monnam(mtmp), an(xname(otmp)));
        g.m_using = TRUE;
        if (!Blind) {
            You("are blinded by the flash of light!");
            make_blinded(Blinded + (long) rnd(1 + 50), FALSE);
        }
        lightdamage(otmp, TRUE, 5);
        g.m_using = FALSE;
        otmp->spe--;
        return 1;
    } /* case MUSE_CAMERA */
#if 0
    case MUSE_SCR_FIRE: {
        boolean vis = cansee(mtmp->mx, mtmp->my);

        mreadmsg(mtmp, otmp);
        if (mtmp->mconf) {
            if (vis)
                pline("Oh, what a pretty fire!");
        } else {
            struct monst *mtmp2;
            int num;

            if (vis)
                pline_The("scroll erupts in a tower of flame!");
            shieldeff(mtmp->mx, mtmp->my);
            pline("%s is uninjured.", Monnam(mtmp));
            (void) destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
            (void) destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
            (void) destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
            ignite_items(mtmp->minvent);
            num = (2 * (rn1(3, 3) + 2 * bcsign(otmp)) + 1) / 3;
            if (Fire_resistance)
                You("are not harmed.");
            burn_away_slime();
            if (Half_spell_damage)
                num = (num + 1) / 2;
            else
                losehp(num, "scroll of fire", KILLED_BY_AN);
            for (mtmp2 = fmon; mtmp2; mtmp2 = mtmp2->nmon) {
                if (DEADMONSTER(mtmp2))
                    continue;
                if (mtmp == mtmp2)
                    continue;
                if (dist2(mtmp2->mx, mtmp2->my, mtmp->mx, mtmp->my) < 3) {
                    if (resists_fire(mtmp2))
                        continue;
                    mtmp2->mhp -= num;
                    if (resists_cold(mtmp2))
                        mtmp2->mhp -= 3 * num;
                    if (DEADMONSTER(mtmp2)) {
                        mondied(mtmp2);
                        break;
                    }
                }
            }
        }
        return 2;
    }
#endif /* 0 */
    case MUSE_POT_PARALYSIS:
    case MUSE_POT_BLINDNESS:
    case MUSE_POT_CONFUSION:
    case MUSE_POT_SLEEPING:
    case MUSE_POT_ACID:
        /* Note: this setting of dknown doesn't suffice.  A monster
         * which is out of sight might throw and it hits something _in_
         * sight, a problem not existing with wands because wand rays
         * are not objects.  Also set dknown in mthrowu.c.
         */
        if (cansee(mtmp->mx, mtmp->my)) {
            otmp->dknown = 1;
            pline("%s hurls %s!", Monnam(mtmp), singular(otmp, doname));
        }
        m_throw(mtmp, mtmp->mx, mtmp->my, sgn(mtmp->mux - mtmp->mx),
                sgn(mtmp->muy - mtmp->my),
                distmin(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy), otmp);
        return 2;
    case 0:
        return 0; /* i.e. an exploded wand */
    default:
        impossible("%s wanted to perform action %d?", Monnam(mtmp),
                   g.m.has_offense);
        break;
    }
    return 0;
}

int
rnd_offensive_item(struct monst* mtmp)
{
    struct permonst *pm = mtmp->data;
    int difficulty = mons[(monsndx(pm))].difficulty;

    if (is_animal(pm) || attacktype(pm, AT_EXPL) || mindless(mtmp->data)
        || pm->mlet == S_GHOST || pm->mlet == S_KOP)
        return 0;
    if (difficulty > 7 && !rn2(35))
        return WAN_DEATH;
    switch (rn2(9 - (difficulty < 4) + 4 * (difficulty > 6))) {
    case 0: {
        struct obj *helmet = which_armor(mtmp, W_ARMH);

        if ((helmet && is_metallic(helmet)) || amorphous(pm)
            || passes_walls(pm) || noncorporeal(pm) || unsolid(pm))
            return SCR_EARTH;
    } /* fall through */
    case 1:
        return WAN_STRIKING;
    case 2:
        return POT_ACID;
    case 3:
        return POT_CONFUSION;
    case 4:
        return POT_BLINDNESS;
    case 5:
        return POT_SLEEPING;
    case 6:
        return POT_PARALYSIS;
    case 7:
    case 8:
        return WAN_MAGIC_MISSILE;
    case 9:
        return WAN_SLEEP;
    case 10:
        return WAN_FIRE;
    case 11:
        return WAN_COLD;
    case 12:
        return WAN_LIGHTNING;
    }
    /*NOTREACHED*/
    return 0;
}

#define MUSE_POT_GAIN_LEVEL 1
#define MUSE_WAN_MAKE_INVISIBLE 2
#define MUSE_POT_INVISIBILITY 3
#define MUSE_POLY_TRAP 4
#define MUSE_WAN_POLYMORPH 5
#define MUSE_POT_SPEED 6
#define MUSE_WAN_SPEED_MONSTER 7
#define MUSE_BULLWHIP 8
#define MUSE_POT_POLYMORPH 9
#define MUSE_BAG 10

boolean
find_misc(struct monst* mtmp)
{
    register struct obj *obj;
    struct permonst *mdat = mtmp->data;
    coordxy x = mtmp->mx, y = mtmp->my;
    struct trap *t;
    coordxy xx, yy;
    int pmidx = NON_PM;
    boolean immobile = (mdat->mmove == 0);
    boolean stuck = (mtmp == u.ustuck);

    g.m.misc = (struct obj *) 0;
    g.m.has_misc = 0;
    if (is_animal(mdat) || mindless(mdat))
        return 0;
    if (u.uswallow && stuck)
        return FALSE;

    /* We arbitrarily limit to times when a player is nearby for the
     * same reason as Junior Pac-Man doesn't have energizers eaten until
     * you can see them...
     */
    if (dist2(x, y, mtmp->mux, mtmp->muy) > 36)
        return FALSE;

    if (!stuck && !immobile && (mtmp->cham == NON_PM)
        && mons[(pmidx = monsndx(mdat))].difficulty < 6) {
        boolean ignore_boulders = (verysmall(mdat) || throws_rocks(mdat)
                                   || passes_walls(mdat)),
            diag_ok = !NODIAG(pmidx);

        for (xx = x - 1; xx <= x + 1; xx++)
            for (yy = y - 1; yy <= y + 1; yy++)
                if (isok(xx, yy) && (xx != u.ux || yy != u.uy)
                    && (diag_ok || xx == x || yy == y)
                    && ((xx == x && yy == y) || !g.level.monsters[xx][yy]))
                    if ((t = t_at(xx, yy)) != 0
                        && (ignore_boulders || !sobj_at(BOULDER, xx, yy))
                        && !onscary(xx, yy, mtmp)) {
                        /* use trap if it's the correct type */
                        if (t->ttyp == POLY_TRAP) {
                            g.trapx = xx;
                            g.trapy = yy;
                            g.m.has_misc = MUSE_POLY_TRAP;
                            return TRUE;
                        }
                    }
    }
    if (nohands(mdat))
        return 0;

    /* normally we would want to bracket a macro expansion containing
       'if' without matching 'else' with 'do { ... } while (0)' but we
       can't do that here because it would intercept 'continue' */
#define nomore(x)       if (g.m.has_misc == (x)) continue
    /*
     * [bug?]  Choice of item is not prioritized; the last viable one
     * in the monster's inventory will be chosen.
     * 'nomore()' is nearly worthless because it only screens checking
     * of duplicates when there is no alternate type in between them.
     *
     * MUSE_BAG issues:
     * should allow looting floor container instead of needing the
     * monster to have picked it up and now be carrying it which takes
     * extra time and renders heavily filled containers immune;
     * hero should have a chance to see the monster fail to open a
     * locked container instead of monster always knowing lock state
     * (may not be feasible to implement--requires too much per-object
     * info for each monster);
     * monster with key should be able to unlock a locked floor
     * container and not know whether it is trapped.
     */
    for (obj = mtmp->minvent; obj; obj = obj->nobj) {
        /* Monsters shouldn't recognize cursed items; this kludge is
           necessary to prevent serious problems though... */
        if (obj->otyp == POT_GAIN_LEVEL
            && (!obj->cursed
                || (!mtmp->isgd && !mtmp->isshk && !mtmp->ispriest))) {
            g.m.misc = obj;
            g.m.has_misc = MUSE_POT_GAIN_LEVEL;
        }
        nomore(MUSE_BULLWHIP);
        if (obj->otyp == BULLWHIP && !mtmp->mpeaceful
            /* the random test prevents whip-wielding
               monster from attempting disarm every turn */
            && uwep && !rn2(5) && obj == MON_WEP(mtmp)
            /* hero's location must be known and adjacent */
            && u_at(mtmp->mux, mtmp->muy)
            && next2u(mtmp->mx, mtmp->my)
            /* don't bother if it can't work (this doesn't
               prevent cursed weapons from being targeted) */
            && (canletgo(uwep, "")
                || (u.twoweap && canletgo(uswapwep, "")))) {
            g.m.misc = obj;
            g.m.has_misc = MUSE_BULLWHIP;
        }
        /* Note: peaceful/tame monsters won't make themselves
         * invisible unless you can see them.  Not really right, but...
         */
        nomore(MUSE_WAN_MAKE_INVISIBLE);
        if (obj->otyp == WAN_MAKE_INVISIBLE && obj->spe > 0 && !mtmp->minvis
            && !mtmp->invis_blkd && (!mtmp->mpeaceful || See_invisible)
            && (!attacktype(mtmp->data, AT_GAZE) || mtmp->mcan)) {
            g.m.misc = obj;
            g.m.has_misc = MUSE_WAN_MAKE_INVISIBLE;
        }
        nomore(MUSE_POT_INVISIBILITY);
        if (obj->otyp == POT_INVISIBILITY && !mtmp->minvis
            && !mtmp->invis_blkd && (!mtmp->mpeaceful || See_invisible)
            && (!attacktype(mtmp->data, AT_GAZE) || mtmp->mcan)) {
            g.m.misc = obj;
            g.m.has_misc = MUSE_POT_INVISIBILITY;
        }
        nomore(MUSE_WAN_SPEED_MONSTER);
        if (obj->otyp == WAN_SPEED_MONSTER && obj->spe > 0
            && mtmp->mspeed != MFAST && !mtmp->isgd) {
            g.m.misc = obj;
            g.m.has_misc = MUSE_WAN_SPEED_MONSTER;
        }
        nomore(MUSE_POT_SPEED);
        if (obj->otyp == POT_SPEED && mtmp->mspeed != MFAST && !mtmp->isgd) {
            g.m.misc = obj;
            g.m.has_misc = MUSE_POT_SPEED;
        }
        nomore(MUSE_WAN_POLYMORPH);
        if (obj->otyp == WAN_POLYMORPH && obj->spe > 0
            && (mtmp->cham == NON_PM) && mons[monsndx(mdat)].difficulty < 6) {
            g.m.misc = obj;
            g.m.has_misc = MUSE_WAN_POLYMORPH;
        }
        nomore(MUSE_POT_POLYMORPH);
        if (obj->otyp == POT_POLYMORPH && (mtmp->cham == NON_PM)
            && mons[monsndx(mdat)].difficulty < 6) {
            g.m.misc = obj;
            g.m.has_misc = MUSE_POT_POLYMORPH;
        }
        nomore(MUSE_BAG);
        if (Is_container(obj) && obj->otyp != BAG_OF_TRICKS && !rn2(5)
            && !g.m.has_misc && Has_contents(obj)
            && !obj->olocked && !obj->otrapped) {
            g.m.misc = obj;
            g.m.has_misc = MUSE_BAG;
        }
    }
    return (boolean) !!g.m.has_misc;
#undef nomore
}

/* type of monster to polymorph into; defaults to one suitable for the
   current level rather than the totally arbitrary choice of newcham() */
static struct permonst *
muse_newcham_mon(struct monst* mon)
{
    struct obj *m_armr;

    if ((m_armr = which_armor(mon, W_ARM)) != 0) {
        if (Is_dragon_scales(m_armr))
            return Dragon_scales_to_pm(m_armr);
        else if (Is_dragon_mail(m_armr))
            return Dragon_mail_to_pm(m_armr);
    }
    return rndmonst();
}

static int
mloot_container(
    struct monst *mon,
    struct obj *container,
    boolean vismon)
{
    char contnr_nam[BUFSZ], mpronounbuf[20];
    boolean nearby;
    int takeout_indx, takeout_count, howfar, res = 0;

    if (!container || !Has_contents(container) || container->olocked)
        return res; /* 0 */
    /* FIXME: handle cursed bag of holding */
    if (Is_mbag(container) && container->cursed)
        return res; /* 0 */

    switch (rn2(10)) {
    default: /* case 0, 1, 2, 3: */
        takeout_count = 1;
        break;
    case 4: case 5: case 6:
        takeout_count = 2;
        break;
    case 7: case 8:
        takeout_count = 3;
        break;
    case 9:
        takeout_count = 4;
        break;
    }
    howfar = mdistu(mon);
    nearby = (howfar <= 7 * 7);
    contnr_nam[0] = mpronounbuf[0] = '\0';
    if (vismon) {
        /* do this once so that when hallucinating it won't change
           from one item to the next */
        Strcpy(mpronounbuf, mhe(mon));
    }

    for (takeout_indx = 0; takeout_indx < takeout_count; ++takeout_indx) {
        struct obj *xobj;
        int nitems;

        if (!Has_contents(container)) /* might have removed all items */
            break;
        /* TODO?
         *  Monster ought to prioritize on something it wants to use.
         */
        nitems = 0;
        for (xobj = container->cobj; xobj != 0; xobj = xobj->nobj)
            ++nitems;
        /* nitems is always greater than 0 due to Has_contents() check;
           throttle item removal as the container becomes less filled */
        if (!rn2(nitems + 1))
            break;
        nitems = rn2(nitems);
        for (xobj = container->cobj; nitems > 0; xobj = xobj->nobj)
            --nitems;

        container->cknown = 0; /* hero no longer knows container's contents
                                * even if [attempted] removal is observed */
        if (!*contnr_nam) {
            /* xname sets dknown, distant_name might depending on its own
               idea about nearness */
            Strcpy(contnr_nam, an(nearby ? xname(container)
                                         : distant_name(container, xname)));
        }
        /* this was originally just 'can_carry(mon, xobj)' which
           covers objects a monster shouldn't pick up but also
           checks carrying capacity; for that, it ended up counting
           xobj's weight twice when container is carried; so take
           xobj out, check whether it can be carried, and then put
           it back (below) if it can't be */
        obj_extract_self(xobj); /* this reduces container's weight */
        /* check whether mon can handle xobj and whether weight of xobj plus
           minvent (including container, now without xobj) can be carried */
        if (can_carry(mon, xobj)) {
            if (vismon) {
                if (howfar > 2) /* not adjacent */
                    Norep("%s rummages through %s.", Monnam(mon), contnr_nam);
                else if (takeout_indx == 0) /* adjacent, first item */
                    pline("%s removes %s from %s.", Monnam(mon),
                          doname(xobj), contnr_nam);
                else /* adjacent, additional items */
                    pline("%s removes %s.", upstart(mpronounbuf),
                          doname(xobj));
            }
            if (container->otyp == ICE_BOX)
                removed_from_icebox(xobj); /* resume rotting for corpse */
            /* obj_extract_self(xobj); -- already done above */
            (void) mpickobj(mon, xobj);
            res = 2;
        } else { /* couldn't carry xobj separately so put back inside */
            /* an achievement prize (castle's wand?) might already be
               marked nomerge (when it hasn't been in invent yet) */
            boolean already_nomerge = xobj->nomerge != 0,
                    just_xobj = !Has_contents(container);

            /* this doesn't restore the original contents ordering
               [shouldn't be a problem; even though this item didn't
               give the rummage message, that's what mon was doing] */
            xobj->nomerge = 1;
            xobj = add_to_container(container, xobj);
            if (!already_nomerge)
                xobj->nomerge = 0;
            container->owt = weight(container);
            if (just_xobj)
                break; /* out of takeout_count loop */
        } /* can_carry */
    } /* takeout_count */
    return res;
}

DISABLE_WARNING_UNREACHABLE_CODE

int
use_misc(struct monst* mtmp)
{
    char nambuf[BUFSZ];
    boolean vis, vismon, vistrapspot, oseen;
    int i;
    struct trap *t;
    struct obj *otmp = g.m.misc;

    if ((i = precheck(mtmp, otmp)) != 0)
        return i;
    vis = cansee(mtmp->mx, mtmp->my);
    vismon = canseemon(mtmp);
    oseen = otmp && vismon;

    switch (g.m.has_misc) {
    case MUSE_POT_GAIN_LEVEL:
        mquaffmsg(mtmp, otmp);
        if (otmp->cursed) {
            if (Can_rise_up(mtmp->mx, mtmp->my, &u.uz)) {
                register int tolev = depth(&u.uz) - 1;
                d_level tolevel;

                get_level(&tolevel, tolev);
                /* insurance against future changes... */
                if (on_level(&tolevel, &u.uz))
                    goto skipmsg;
                if (vismon) {
                    pline("%s rises up, through the %s!", Monnam(mtmp),
                          ceiling(mtmp->mx, mtmp->my));
                    trycall(otmp);
                }
                m_useup(mtmp, otmp);
                migrate_to_level(mtmp, ledger_no(&tolevel), MIGR_RANDOM,
                                 (coord *) 0);
                return 2;
            } else {
 skipmsg:
                if (vismon) {
                    pline("%s looks uneasy.", Monnam(mtmp));
                    trycall(otmp);
                }
                m_useup(mtmp, otmp);
                return 2;
            }
        }
        if (vismon)
            pline("%s seems more experienced.", Monnam(mtmp));
        if (oseen)
            makeknown(POT_GAIN_LEVEL);
        m_useup(mtmp, otmp);
        if (!grow_up(mtmp, (struct monst *) 0))
            return 1;
        /* grew into genocided monster */
        return 2;
    case MUSE_WAN_MAKE_INVISIBLE:
    case MUSE_POT_INVISIBILITY:
        if (otmp->otyp == WAN_MAKE_INVISIBLE) {
            mzapwand(mtmp, otmp, TRUE);
        } else
            mquaffmsg(mtmp, otmp);
        /* format monster's name before altering its visibility */
        Strcpy(nambuf, mon_nam(mtmp));
        mon_set_minvis(mtmp);
        if (vismon && mtmp->minvis) { /* was seen, now invisible */
            if (canspotmon(mtmp)) {
                pline("%s body takes on a %s transparency.",
                      upstart(s_suffix(nambuf)),
                      Hallucination ? "normal" : "strange");
            } else {
                pline("Suddenly you cannot see %s.", nambuf);
                if (vis)
                    map_invisible(mtmp->mx, mtmp->my);
            }
            if (oseen)
                makeknown(otmp->otyp);
        }
        if (otmp->otyp == POT_INVISIBILITY) {
            if (otmp->cursed)
                you_aggravate(mtmp);
            m_useup(mtmp, otmp);
        }
        return 2;
    case MUSE_WAN_SPEED_MONSTER:
        mzapwand(mtmp, otmp, TRUE);
        mon_adjust_speed(mtmp, 1, otmp);
        return 2;
    case MUSE_POT_SPEED:
        mquaffmsg(mtmp, otmp);
        /* note difference in potion effect due to substantially
           different methods of maintaining speed ratings:
           player's character becomes "very fast" temporarily;
           monster becomes "one stage faster" permanently */
        mon_adjust_speed(mtmp, 1, otmp);
        m_useup(mtmp, otmp);
        return 2;
    case MUSE_WAN_POLYMORPH:
        mzapwand(mtmp, otmp, TRUE);
        (void) newcham(mtmp, muse_newcham_mon(mtmp),
                       NC_VIA_WAND_OR_SPELL | NC_SHOW_MSG);
        if (oseen)
            makeknown(WAN_POLYMORPH);
        return 2;
    case MUSE_POT_POLYMORPH:
        mquaffmsg(mtmp, otmp);
        m_useup(mtmp, otmp);
        if (vismon)
            pline("%s suddenly mutates!", Monnam(mtmp));
        (void) newcham(mtmp, muse_newcham_mon(mtmp), NC_SHOW_MSG);
        if (oseen)
            makeknown(POT_POLYMORPH);
        return 2;
    case MUSE_POLY_TRAP:
        t = t_at(g.trapx, g.trapy);
        vistrapspot = cansee(t->tx, t->ty);
        if (vis || vistrapspot)
            seetrap(t);
        if (vismon || vistrapspot) {
            pline("%s deliberately %s onto a %s trap!", Some_Monnam(mtmp),
                  vtense(fakename[0], locomotion(mtmp->data, "jump")),
                  t->tseen ? "polymorph" : "hidden");
            /* note: if mtmp is unseen because it is invisible, its new
               shape will also be invisible and could produce "Its armor
               falls off" messages during the transformation; those make
               more sense after we've given "Someone jumps onto a trap." */
        }

        /*  don't use rloc() due to worms */
        remove_monster(mtmp->mx, mtmp->my);
        newsym(mtmp->mx, mtmp->my);
        place_monster(mtmp, g.trapx, g.trapy);
        if (mtmp->wormno)
            worm_move(mtmp);
        newsym(g.trapx, g.trapy);

        (void) newcham(mtmp, (struct permonst *) 0, NC_SHOW_MSG);
        return 2;
    case MUSE_BAG:
        return mloot_container(mtmp, otmp, vismon);
    case MUSE_BULLWHIP:
        /* attempt to disarm hero */
        {
            const char *The_whip = vismon ? "The bullwhip" : "A whip";
            int where_to = rn2(4);
            struct obj *obj = uwep;
            const char *hand;
            char the_weapon[BUFSZ];

            if (!obj || !canletgo(obj, "")
                || (u.twoweap && canletgo(uswapwep, "") && rn2(2)))
                obj = uswapwep;
            if (!obj)
                break; /* shouldn't happen after find_misc() */

            Strcpy(the_weapon, the(xname(obj)));
            hand = body_part(HAND);
            if (bimanual(obj))
                hand = makeplural(hand);

            if (vismon)
                pline("%s flicks a bullwhip towards your %s!", Monnam(mtmp),
                      hand);
            if (obj->otyp == HEAVY_IRON_BALL) {
                pline("%s fails to wrap around %s.", The_whip, the_weapon);
                return 1;
            }
            urgent_pline("%s wraps around %s you're wielding!", The_whip,
                         the_weapon);
            if (welded(obj)) {
                pline("%s welded to your %s%c",
                      !is_plural(obj) ? "It is" : "They are", hand,
                      !obj->bknown ? '!' : '.');
                /* obj->bknown = 1; */ /* welded() takes care of this */
                where_to = 0;
            }
            if (!where_to) {
                pline_The("whip slips free."); /* not `The_whip' */
                return 1;
            } else if (where_to == 3 && mon_hates_silver(mtmp)
                       && objects[obj->otyp].oc_material == SILVER) {
                /* this monster won't want to catch a silver
                   weapon; drop it at hero's feet instead */
                where_to = 2;
            }
            remove_worn_item(obj, FALSE);
            freeinv(obj);
            switch (where_to) {
            case 1: /* onto floor beneath mon */
                pline("%s yanks %s from your %s!", Monnam(mtmp), the_weapon,
                      hand);
                place_object(obj, mtmp->mx, mtmp->my);
                break;
            case 2: /* onto floor beneath you */
                pline("%s yanks %s to the %s!", Monnam(mtmp), the_weapon,
                      surface(u.ux, u.uy));
                dropy(obj);
                break;
            case 3: /* into mon's inventory */
                pline("%s snatches %s!", Monnam(mtmp), the_weapon);
                (void) mpickobj(mtmp, obj);
                break;
            }
            return 1;
        }
        /*NOTREACHED*/
        return 0;
    case 0:
        return 0; /* i.e. an exploded wand */
    default:
        impossible("%s wanted to perform action %d?", Monnam(mtmp),
                   g.m.has_misc);
        break;
    }
    return 0;
}

RESTORE_WARNINGS

static void
you_aggravate(struct monst* mtmp)
{
    pline("For some reason, %s presence is known to you.",
          s_suffix(noit_mon_nam(mtmp)));
    cls();
#ifdef CLIPPING
    cliparound(mtmp->mx, mtmp->my);
#endif
    show_glyph(mtmp->mx, mtmp->my, mon_to_glyph(mtmp, rn2_on_display_rng));
    display_self();
    You_feel("aggravated at %s.", noit_mon_nam(mtmp));
    display_nhwindow(WIN_MAP, TRUE);
    docrt();
    if (unconscious()) {
        g.multi = -1;
        g.nomovemsg = "Aggravated, you are jolted into full consciousness.";
    }
    newsym(mtmp->mx, mtmp->my);
    if (!canspotmon(mtmp))
        map_invisible(mtmp->mx, mtmp->my);
}

int
rnd_misc_item(struct monst* mtmp)
{
    struct permonst *pm = mtmp->data;
    int difficulty = mons[(monsndx(pm))].difficulty;

    if (is_animal(pm) || attacktype(pm, AT_EXPL) || mindless(mtmp->data)
        || pm->mlet == S_GHOST || pm->mlet == S_KOP)
        return 0;
    /* Unlike other rnd_item functions, we only allow _weak_ monsters
     * to have this item; after all, the item will be used to strengthen
     * the monster and strong monsters won't use it at all...
     */
    if (difficulty < 6 && !rn2(30))
        return rn2(6) ? POT_POLYMORPH : WAN_POLYMORPH;

    if (!rn2(40) && !nonliving(pm) && !is_vampshifter(mtmp))
        return AMULET_OF_LIFE_SAVING;

    switch (rn2(3)) {
    case 0:
        if (mtmp->isgd)
            return 0;
        return rn2(6) ? POT_SPEED : WAN_SPEED_MONSTER;
    case 1:
        if (mtmp->mpeaceful && !See_invisible)
            return 0;
        return rn2(6) ? POT_INVISIBILITY : WAN_MAKE_INVISIBLE;
    case 2:
        return POT_GAIN_LEVEL;
    }
    /*NOTREACHED*/
    return 0;
}

#if 0
/* check whether hero is carrying a corpse or contained petrifier corpse */
static boolean
necrophiliac(struct obj* objlist, boolean any_corpse)
{
    while (objlist) {
        if (objlist->otyp == CORPSE
            && (any_corpse || touch_petrifies(&mons[objlist->corpsenm])))
            return TRUE;
        if (Has_contents(objlist) && necrophiliac(objlist->cobj, FALSE))
            return TRUE;
        objlist = objlist->nobj;
    }
    return FALSE;
}
#endif

boolean
searches_for_item(struct monst* mon, struct obj* obj)
{
    int typ = obj->otyp;

    /* don't let monsters interact with protected items on the floor */
    if (obj->where == OBJ_FLOOR
        && (obj->ox == mon->mx && obj->oy == mon->my)
        && onscary(obj->ox, obj->oy, mon)) {
        return FALSE;
    }

    if (is_animal(mon->data) || mindless(mon->data)
        || mon->data == &mons[PM_GHOST]) /* don't loot bones piles */
        return FALSE;

    if (typ == WAN_MAKE_INVISIBLE || typ == POT_INVISIBILITY)
        return (boolean) (!mon->minvis && !mon->invis_blkd
                          && !attacktype(mon->data, AT_GAZE));
    if (typ == WAN_SPEED_MONSTER || typ == POT_SPEED)
        return (boolean) (mon->mspeed != MFAST);

    switch (obj->oclass) {
    case WAND_CLASS:
        if (obj->spe <= 0)
            return FALSE;
        if (typ == WAN_DIGGING)
            return (boolean) !is_floater(mon->data);
        if (typ == WAN_POLYMORPH)
            return (boolean) (mons[monsndx(mon->data)].difficulty < 6);
        if (objects[typ].oc_dir == RAY || typ == WAN_STRIKING
            || typ == WAN_UNDEAD_TURNING
            || typ == WAN_TELEPORTATION || typ == WAN_CREATE_MONSTER)
            return TRUE;
        break;
    case POTION_CLASS:
        if (typ == POT_HEALING || typ == POT_EXTRA_HEALING
            || typ == POT_FULL_HEALING || typ == POT_POLYMORPH
            || typ == POT_GAIN_LEVEL || typ == POT_PARALYSIS
            || typ == POT_SLEEPING || typ == POT_ACID || typ == POT_CONFUSION)
            return TRUE;
        if (typ == POT_BLINDNESS && !attacktype(mon->data, AT_GAZE))
            return TRUE;
        break;
    case SCROLL_CLASS:
        if (typ == SCR_TELEPORTATION || typ == SCR_CREATE_MONSTER
            || typ == SCR_EARTH || typ == SCR_FIRE)
            return TRUE;
        break;
    case AMULET_CLASS:
        if (typ == AMULET_OF_LIFE_SAVING)
            return (boolean) !(nonliving(mon->data) || is_vampshifter(mon));
        if (typ == AMULET_OF_REFLECTION || typ == AMULET_OF_GUARDING)
            return TRUE;
        break;
    case TOOL_CLASS:
        if (typ == PICK_AXE)
            return (boolean) needspick(mon->data);
        if (typ == UNICORN_HORN)
            return (boolean) (!obj->cursed && !is_unicorn(mon->data)
                              && mon->data != &mons[PM_KI_RIN]);
        if (typ == FROST_HORN || typ == FIRE_HORN)
            return (obj->spe > 0 && can_blow(mon));
        if (Is_container(obj) && !(Is_mbag(obj) && obj->cursed)
            && !obj->olocked)
            return TRUE;
        if (typ == EXPENSIVE_CAMERA)
            return (obj->spe > 0);
        break;
    case FOOD_CLASS:
        if (typ == CORPSE)
            return (boolean) (((mon->misc_worn_check & W_ARMG) != 0L
                               && touch_petrifies(&mons[obj->corpsenm]))
                              || (!resists_ston(mon)
                                  && cures_stoning(mon, obj, FALSE)));
        if (typ == TIN)
            return (boolean) (mcould_eat_tin(mon)
                              && (!resists_ston(mon)
                                  && cures_stoning(mon, obj, TRUE)));
        if (typ == EGG)
            return (boolean) touch_petrifies(&mons[obj->corpsenm]);
        break;
    default:
        break;
    }

    return FALSE;
}

DISABLE_WARNING_FORMAT_NONLITERAL

boolean
mon_reflects(struct monst* mon, const char* str)
{
    struct obj *orefl = which_armor(mon, W_ARMS);

    if (orefl && orefl->otyp == SHIELD_OF_REFLECTION) {
        if (str) {
            pline(str, s_suffix(mon_nam(mon)), "shield");
            makeknown(SHIELD_OF_REFLECTION);
        }
        return TRUE;
    } else if (arti_reflects(MON_WEP(mon))) {
        /* due to wielded artifact weapon */
        if (str)
            pline(str, s_suffix(mon_nam(mon)), "weapon");
        return TRUE;
    } else if ((orefl = which_armor(mon, W_AMUL))
               && orefl->otyp == AMULET_OF_REFLECTION) {
        if (str) {
            pline(str, s_suffix(mon_nam(mon)), "amulet");
            makeknown(AMULET_OF_REFLECTION);
        }
        return TRUE;
    } else if ((orefl = which_armor(mon, W_ARM))
               && (orefl->otyp == SILVER_DRAGON_SCALES
                   || orefl->otyp == SILVER_DRAGON_SCALE_MAIL)) {
        if (str)
            pline(str, s_suffix(mon_nam(mon)), "armor");
        return TRUE;
    } else if (mon->data == &mons[PM_SILVER_DRAGON]
               || mon->data == &mons[PM_CHROMATIC_DRAGON]) {
        /* Silver dragons only reflect when mature; babies do not */
        if (str)
            pline(str, s_suffix(mon_nam(mon)), "scales");
        return TRUE;
    }
    return FALSE;
}

boolean
ureflects(const char* fmt, const char* str)
{
    /* Check from outermost to innermost objects */
    if (EReflecting & W_ARMS) {
        if (fmt && str) {
            pline(fmt, str, "shield");
            makeknown(SHIELD_OF_REFLECTION);
        }
        return TRUE;
    } else if (EReflecting & W_WEP) {
        /* Due to wielded artifact weapon */
        if (fmt && str)
            pline(fmt, str, "weapon");
        return TRUE;
    } else if (EReflecting & W_AMUL) {
        if (fmt && str) {
            pline(fmt, str, "medallion");
            makeknown(AMULET_OF_REFLECTION);
        }
        return TRUE;
    } else if (EReflecting & W_ARM) {
        if (fmt && str)
            pline(fmt, str, uskin ? "luster" : "armor");
        return TRUE;
    } else if (g.youmonst.data == &mons[PM_SILVER_DRAGON]) {
        if (fmt && str)
            pline(fmt, str, "scales");
        return TRUE;
    }
    return FALSE;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* cure mon's blindness (use_defensive, dog_eat, meatobj) */
void
mcureblindness(struct monst* mon, boolean verbos)
{
    if (!mon->mcansee) {
        mon->mcansee = 1;
        mon->mblinded = 0;
        if (verbos && haseyes(mon->data))
            pline("%s can see again.", Monnam(mon));
    }
}

/* TRUE if the monster ate something */
boolean
munstone(struct monst* mon, boolean by_you)
{
    struct obj *obj;
    boolean tinok;

    if (resists_ston(mon))
        return FALSE;
    if (mon->meating || helpless(mon))
        return FALSE;
    mon->mstrategy &= ~STRAT_WAITFORU;

    tinok = mcould_eat_tin(mon);
    for (obj = mon->minvent; obj; obj = obj->nobj) {
        if (cures_stoning(mon, obj, tinok)) {
            mon_consume_unstone(mon, obj, by_you, TRUE);
            return TRUE;
        }
    }
    return FALSE;
}

static void
mon_consume_unstone(
    struct monst *mon,
    struct obj *obj,
    boolean by_you,
    boolean stoning) /* True: stop petrification, False: cure stun && confusion */
{
    boolean vis = canseemon(mon), tinned = obj->otyp == TIN,
            food = obj->otyp == CORPSE || tinned,
            acid = obj->otyp == POT_ACID
                   || (food && acidic(&mons[obj->corpsenm])),
            lizard = food && obj->corpsenm == PM_LIZARD;
    int nutrit = food ? dog_nutrition(mon, obj) : 0; /* also sets meating */

    /* give a "<mon> is slowing down" message and also remove
       intrinsic speed (comparable to similar effect on the hero) */
    if (stoning)
        mon_adjust_speed(mon, -3, (struct obj *) 0);

    if (vis) {
        long save_quan = obj->quan;

        obj->quan = 1L;
        pline("%s %s %s.", Monnam(mon),
              ((obj->oclass == POTION_CLASS) ? "quaffs"
               : (obj->otyp == TIN) ? "opens and eats the contents of"
                 : "eats"),
              distant_name(obj, doname));
        obj->quan = save_quan;
    } else if (!Deaf)
        You_hear("%s.",
                 (obj->oclass == POTION_CLASS) ? "drinking" : "chewing");

    m_useup(mon, obj);
    /* obj is now gone */

    if (acid && !tinned && !resists_acid(mon)) {
        mon->mhp -= rnd(15);
        if (vis)
            pline("%s has a very bad case of stomach acid.", Monnam(mon));
        if (DEADMONSTER(mon)) {
            pline("%s dies!", Monnam(mon));
            if (by_you)
                /* hero gets credit (experience) and blame (possible loss
                   of alignment and/or luck and/or telepathy depending on
                   mon) for the kill but does not break pacifism conduct */
                xkilled(mon, XKILL_NOMSG | XKILL_NOCONDUCT);
            else
                mondead(mon);
            return;
        }
    }
    if (stoning && vis) {
        if (Hallucination)
            pline("What a pity - %s just ruined a future piece of art!",
                  mon_nam(mon));
        else
            pline("%s seems limber!", Monnam(mon));
    }
    if (lizard && (mon->mconf || mon->mstun)) {
        mon->mconf = 0;
        mon->mstun = 0;
        if (vis && !is_bat(mon->data) && mon->data != &mons[PM_STALKER])
            pline("%s seems steadier now.", Monnam(mon));
    }
    if (mon->mtame && !mon->isminion && nutrit > 0) {
        struct edog *edog = EDOG(mon);

        if (edog->hungrytime < g.moves)
            edog->hungrytime = g.moves;
        edog->hungrytime += nutrit;
        mon->mconf = 0;
    }
    /* use up monster's next move */
    mon->movement -= NORMAL_SPEED;
    mon->mlstmv = g.moves;
}

/* decide whether obj can cure petrification; also used when picking up */
static boolean
cures_stoning(struct monst* mon, struct obj* obj, boolean tinok)
{
    if (obj->otyp == POT_ACID)
        return TRUE;
    if (obj->otyp == GLOB_OF_GREEN_SLIME)
        return (boolean) slimeproof(mon->data);
    if (obj->otyp != CORPSE && (obj->otyp != TIN || !tinok))
        return FALSE;
    /* corpse, or tin that mon can open */
    if (obj->corpsenm == NON_PM) /* empty/special tin */
        return FALSE;
    return (boolean) (obj->corpsenm == PM_LIZARD
                      || acidic(&mons[obj->corpsenm]));
}

static boolean
mcould_eat_tin(struct monst* mon)
{
    struct obj *obj, *mwep;
    boolean welded_wep;

    /* monkeys who manage to steal tins can't open and eat them
       even if they happen to also have the appropriate tool */
    if (is_animal(mon->data))
        return FALSE;

    mwep = MON_WEP(mon);
    welded_wep = mwep && mwelded(mwep);
    /* this is different from the player; tin opener or dagger doesn't
       have to be wielded, and knife can be used instead of dagger */
    for (obj = mon->minvent; obj; obj = obj->nobj) {
        /* if stuck with a cursed weapon, don't check rest of inventory */
        if (welded_wep && obj != mwep)
            continue;

        if (obj->otyp == TIN_OPENER
            || (obj->oclass == WEAPON_CLASS
                && (objects[obj->otyp].oc_skill == P_DAGGER
                    || objects[obj->otyp].oc_skill == P_KNIFE)))
            return TRUE;
    }
    return FALSE;
}

/* TRUE if monster does something to avoid turning into green slime */
boolean
munslime(struct monst* mon, boolean by_you)
{
    struct obj *obj, odummy;
    struct permonst *mptr = mon->data;

    /*
     * muse_unslime() gives "mon starts turning green", "mon zaps
     * itself with a wand of fire", and "mon's slime burns away"
     * messages.  Monsters who don't get any chance at that just have
     * (via our caller) newcham()'s "mon turns into slime" feedback.
     */

    if (slimeproof(mptr))
        return FALSE;
    if (mon->meating || helpless(mon))
        return FALSE;
    mon->mstrategy &= ~STRAT_WAITFORU;

    /* if monster can breathe fire, do so upon self; a monster who deals
       fire damage by biting, clawing, gazing, and especially exploding
       isn't able to cure itself of green slime with its own attack
       [possible extension: monst capable of casting high level clerical
       spells could toss pillar of fire at self--probably too suicidal] */
    if (!mon->mcan && !mon->mspec_used
        && attacktype_fordmg(mptr, AT_BREA, AD_FIRE)) {
        odummy = cg.zeroobj; /* otyp == STRANGE_OBJECT */
        return muse_unslime(mon, &odummy, (struct trap *) 0, by_you);
    }

    /* same MUSE criteria as use_defensive() */
    if (!is_animal(mptr) && !mindless(mptr)) {
        struct trap *t;

        for (obj = mon->minvent; obj; obj = obj->nobj)
            if (cures_sliming(mon, obj))
                return muse_unslime(mon, obj, (struct trap *) 0, by_you);

        if (((t = t_at(mon->mx, mon->my)) == 0 || t->ttyp != FIRE_TRAP)
            && mptr->mmove && !mon->mtrapped) {
            coordxy xy[2][8], x, y, idx, ridx, nxy = 0;

            for (x = mon->mx - 1; x <= mon->mx + 1; ++x)
                for (y = mon->my - 1; y <= mon->my + 1; ++y)
                    if (isok(x, y) && accessible(x, y)
                        && !m_at(x, y) && (x != u.ux || y != u.uy)) {
                        xy[0][nxy] = x, xy[1][nxy] = y;
                        ++nxy;
                    }
            for (idx = 0; idx < nxy; ++idx) {
                ridx = rn1(nxy - idx, idx);
                if (ridx != idx) {
                    x = xy[0][idx];
                    xy[0][idx] = xy[0][ridx];
                    xy[0][ridx] = x;
                    y = xy[1][idx];
                    xy[1][idx] = xy[1][ridx];
                    xy[1][ridx] = y;
                }
                if ((t = t_at(xy[0][idx], xy[1][idx])) != 0
                    && t->ttyp == FIRE_TRAP)
                    break;
            }
        }
        if (t && t->ttyp == FIRE_TRAP)
            return muse_unslime(mon, (struct obj *) &cg.zeroobj, t, by_you);

    } /* MUSE */

    return FALSE;
}

/* mon uses an item--selected by caller--to burn away incipient slime */
static boolean
muse_unslime(
    struct monst *mon,
    struct obj *obj,
    struct trap *trap,
    boolean by_you) /* true: if mon kills itself, hero gets credit/blame */
{                   /* [by_you not honored if 'mon' triggers fire trap]. */
    struct obj *odummyp;
    int otyp = obj->otyp, dmg = 0;
    boolean vis = canseemon(mon), res = TRUE;

    if (vis)
        pline("%s starts turning %s.", Monnam(mon),
              green_mon(mon) ? "into ooze" : hcolor(NH_GREEN));
    /* -4 => sliming, causes quiet loss of enhanced speed */
    mon_adjust_speed(mon, -4, (struct obj *) 0);

    if (trap) {
        const char *Mnam = vis ? Monnam(mon) : 0;

        if (mon->mx == trap->tx && mon->my == trap->ty) {
            if (vis)
                pline("%s triggers %s fire trap!", Mnam,
                      trap->tseen ? "the" : "a");
        } else {
            remove_monster(mon->mx, mon->my);
            newsym(mon->mx, mon->my);
            place_monster(mon, trap->tx, trap->ty);
            if (mon->wormno) /* won't happen; worms don't MUSE to unslime */
                worm_move(mon);
            newsym(mon->mx, mon->my);
            if (vis)
                pline("%s %s %s %s fire trap!", Mnam,
                      vtense(fakename[0], locomotion(mon->data, "move")),
                      is_floater(mon->data) ? "over" : "onto",
                      trap->tseen ? "the" : "a");
        }
        (void) mintrap(mon, FORCETRAP);
    } else if (otyp == STRANGE_OBJECT) {
        /* monster is using fire breath on self */
        if (vis)
            pline("%s.", monverbself(mon, Monnam(mon), "breath", "fire on"));
        if (!rn2(3))
            mon->mspec_used = rn1(10, 5);
        /* -21 => monster's fire breath; 1 => # of damage dice */
        dmg = zhitm(mon, by_you ? 21 : -21, 1, &odummyp);
    } else if (otyp == SCR_FIRE) {
        mreadmsg(mon, obj);
        if (mon->mconf) {
            if (cansee(mon->mx, mon->my))
                pline("Oh, what a pretty fire!");
            if (vis)
                trycall(obj);
            m_useup(mon, obj); /* after trycall() */
            vis = FALSE;       /* skip makeknown() below */
            res = FALSE;       /* failed to cure sliming */
        } else {
            dmg = (2 * (rn1(3, 3) + 2 * bcsign(obj)) + 1) / 3;
            m_useup(mon, obj); /* before explode() */
            /* -11 => monster's fireball */
            explode(mon->mx, mon->my, -11, dmg, SCROLL_CLASS,
                    /* by_you: override -11 for mon but not others */
                    by_you ? -EXPL_FIERY : EXPL_FIERY);
            dmg = 0; /* damage has been applied by explode() */
        }
    } else if (otyp == POT_OIL) {
        char Pronoun[40];
        boolean was_lit = obj->lamplit ? TRUE : FALSE, saw_lit = FALSE;
        /*
         * If not already lit, requires two actions.  We cheat and let
         * monster do both rather than render the potion unuseable.
         *
         * Monsters don't start with oil and don't actively pick up oil
         * so this may never occur in a real game.  (Possible though;
         * nymph can steal potions of oil; shapechanger could take on
         * nymph form or vacuum up stuff as a g.cube and then eventually
         * engage with a green slime.)
         */

        if (obj->quan > 1L)
            obj = splitobj(obj, 1L);
        if (vis && !was_lit) {
            pline("%s ignites %s.", Monnam(mon), ansimpleoname(obj));
            saw_lit = TRUE;
        }
        begin_burn(obj, was_lit);
        vis |= canseemon(mon); /* burning potion may improve visibility */
        if (vis) {
            if (!Unaware)
                obj->dknown = 1; /* hero is watching mon drink obj */
            pline("%s quaffs a burning %s",
                  saw_lit ? upstart(strcpy(Pronoun, mhe(mon))) : Monnam(mon),
                  simpleonames(obj));
            makeknown(POT_OIL);
        }
        dmg = d(3, 4); /* [**TEMP** (different from hero)] */
        m_useup(mon, obj);
    } else { /* wand/horn of fire w/ positive charge count */
        if (obj->otyp == FIRE_HORN)
            mplayhorn(mon, obj, TRUE);
        else
            mzapwand(mon, obj, TRUE);
        /* -1 => monster's wand of fire; 2 => # of damage dice */
        dmg = zhitm(mon, by_you ? 1 : -1, 2, &odummyp);
    }

    if (dmg) {
        /* zhitm() applies damage but doesn't kill creature off;
           for fire breath, dmg is going to be 0 (fire breathers are
           immune to fire damage) but for wand of fire or fire horn,
           'mon' could have taken damage so might die */
        if (DEADMONSTER(mon)) {
            if (by_you) {
                /* mon killed self but hero gets credit and blame (except
                   for pacifist conduct); xkilled()'s message would say
                   "You killed/destroyed <mon>" so give our own message */
                if (vis)
                    pline("%s is %s by the fire!", Monnam(mon),
                          nonliving(mon->data) ? "destroyed" : "killed");
                xkilled(mon, XKILL_NOMSG | XKILL_NOCONDUCT);
            } else
                monkilled(mon, "fire", AD_FIRE);
        } else {
            /* non-fatal damage occurred */
            if (vis)
                pline("%s is burned%s", Monnam(mon), exclam(dmg));
        }
    }
    if (vis) {
        if (res && !DEADMONSTER(mon))
            pline("%s slime is burned away!", s_suffix(Monnam(mon)));
        if (otyp != STRANGE_OBJECT)
            makeknown(otyp);
    }
    /* use up monster's next move */
    mon->movement -= NORMAL_SPEED;
    mon->mlstmv = g.moves;
    return res;
}

/* decide whether obj can be used to cure green slime */
static int
cures_sliming(struct monst *mon, struct obj *obj)
{
    /* scroll of fire */
    if (obj->otyp == SCR_FIRE)
        return (haseyes(mon->data) && mon->mcansee && !nohands(mon->data));

    /* potion of oil; will be set burning if not already */
    if (obj->otyp == POT_OIL)
        return !nohands(mon->data);

    /* non-empty wand or horn of fire;
       hero doesn't need hands or even limbs to zap, so mon doesn't either */
    return ((obj->otyp == WAN_FIRE
             || (obj->otyp == FIRE_HORN && can_blow(mon)))
            && obj->spe > 0);
}

/* TRUE if monster appears to be green; for active TEXTCOLOR, we go by
   the display color, otherwise we just pick things that seem plausibly
   green (which doesn't necessarily match the TEXTCOLOR categorization) */
static boolean
green_mon(struct monst* mon)
{
    struct permonst *ptr = mon->data;

    if (Hallucination)
        return FALSE;
#ifdef TEXTCOLOR
    if (iflags.use_color)
        return (ptr->mcolor == CLR_GREEN || ptr->mcolor == CLR_BRIGHT_GREEN);
#endif
    /* approximation */
    if (strstri(ptr->pmnames[NEUTRAL], "green")
        || (ptr->pmnames[MALE] && strstri(ptr->pmnames[MALE], "green"))
        || (ptr->pmnames[FEMALE] && strstri(ptr->pmnames[FEMALE], "green")))
        return TRUE;
    switch (monsndx(ptr)) {
    case PM_FOREST_CENTAUR:
    case PM_GARTER_SNAKE:
    case PM_GECKO:
    case PM_GREMLIN:
    case PM_HOMUNCULUS:
    case PM_JUIBLEX:
    case PM_LEPRECHAUN:
    case PM_LICHEN:
    case PM_LIZARD:
    case PM_WOOD_NYMPH:
        return TRUE;
    default:
        if (is_elf(ptr) && !is_prince(ptr) && !is_lord(ptr)
            && ptr != &mons[PM_GREY_ELF])
            return TRUE;
        break;
    }
    return FALSE;
}

/*muse.c*/

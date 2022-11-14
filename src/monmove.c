/* NetHack 3.7	monmove.c	$NHDT-Date: 1651886999 2022/05/07 01:29:59 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.179 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mfndpos.h"
#include "artifact.h"

static void watch_on_duty(struct monst *);
static int disturb(struct monst *);
static void release_hero(struct monst *);
static void distfleeck(struct monst *, int *, int *, int *);
static int m_arrival(struct monst *);
static void mind_blast(struct monst *);
static boolean holds_up_web(coordxy, coordxy);
static int count_webbing_walls(coordxy, coordxy);
static boolean soko_allow_web(struct monst *);
static boolean leppie_avoidance(struct monst *);
static void leppie_stash(struct monst *);
static boolean m_balks_at_approaching(struct monst *);
static boolean stuff_prevents_passage(struct monst *);
static int vamp_shift(struct monst *, struct permonst *, boolean);

/* monster has triggered trapped door lock or was present when it got
   triggered remotely (at door spot, door hit by zap);
   returns True if mtmp dies */
boolean
mb_trapped(struct monst *mtmp, boolean canseeit)
{
    if (Verbose(2, mb_trapped)) {
        if (canseeit && !Unaware)
            pline("KABOOM!!  You see a door explode.");
        else if (!Deaf)
            You_hear("a %s explosion.",
                     (mdistu(mtmp) > 7 * 7) ? "distant" : "nearby");
    }
    wake_nearto(mtmp->mx, mtmp->my, 7 * 7);
    mtmp->mstun = 1;
    mtmp->mhp -= rnd(15);
    if (DEADMONSTER(mtmp)) {
        mondied(mtmp);
        if (DEADMONSTER(mtmp))
            return TRUE;
        /* will get here if lifesaved */
    }
    mon_learns_traps(mtmp, TRAPPED_DOOR);
    return FALSE;
}

/* push coordinate x,y to mtrack, making monster remember where it was */
void
mon_track_add(struct monst *mtmp, coordxy x, coordxy y)
{
    int j;

    for (j = MTSZ - 1; j > 0; j--)
        mtmp->mtrack[j] = mtmp->mtrack[j - 1];
    mtmp->mtrack[0].x = x;
    mtmp->mtrack[0].y = y;
}

void
mon_track_clear(struct monst *mtmp)
{
    memset(mtmp->mtrack, 0, sizeof(mtmp->mtrack));
}

/* check whether a monster is carrying a locking/unlocking tool */
boolean
monhaskey(
    struct monst *mon,
    boolean for_unlocking) /* true => credit card ok, false => not ok */
{
    if (for_unlocking && m_carrying(mon, CREDIT_CARD))
        return TRUE;
    return m_carrying(mon, SKELETON_KEY) || m_carrying(mon, LOCK_PICK);
}

void
mon_yells(struct monst* mon, const char* shout)
{
    if (Deaf) {
        if (canspotmon(mon))
            /* Sidenote on "A watchman angrily waves her arms!"
             * Female being called watchman is correct (career name).
             */
            pline("%s angrily %s %s %s!",
                Amonnam(mon),
                nolimbs(mon->data) ? "shakes" : "waves",
                mhis(mon),
                nolimbs(mon->data) ? mbodypart(mon, HEAD)
                                   : makeplural(mbodypart(mon, ARM)));
    } else {
        if (canspotmon(mon))
            pline("%s yells:", Amonnam(mon));
        else
            You_hear("someone yell:");
        verbalize1(shout);
    }
}

static void
watch_on_duty(register struct monst* mtmp)
{
    coordxy x, y;

    if (mtmp->mpeaceful && in_town(u.ux + u.dx, u.uy + u.dy)
        && mtmp->mcansee && m_canseeu(mtmp) && !rn2(3)) {
        if (picking_lock(&x, &y) && IS_DOOR(levl[x][y].typ)
            && (levl[x][y].doormask & D_LOCKED)) {
            if (couldsee(mtmp->mx, mtmp->my)) {
                if (levl[x][y].looted & D_WARNED) {
                    mon_yells(mtmp, "Halt, thief!  You're under arrest!");
                    (void) angry_guards(!!Deaf);
                } else {
                    mon_yells(mtmp, "Hey, stop picking that lock!");
                    levl[x][y].looted |= D_WARNED;
                }
                stop_occupation();
            }
        } else if (is_digging()) {
            /* chewing, wand/spell of digging are checked elsewhere */
            watch_dig(mtmp, g.context.digging.pos.x, g.context.digging.pos.y,
                      FALSE);
        }
    }
}

/* move a monster; if a threat to busy hero, stop doing whatever it is */
int
dochugw(
    register struct monst *mtmp,
    boolean chug) /* True: monster is moving;
                   * False: monster was just created or has teleported
                   * so perform stop-what-you're-doing-if-close-enough-
                   * to-be-a-threat check but don't move mtmp */
{
    coordxy x = mtmp->mx, y = mtmp->my; /* 'mtmp's location before dochug() */
    /* skip canspotmon() if occupation is Null */
    boolean already_saw_mon = (chug && g.occupation) ? canspotmon(mtmp) : 0;
    int rd = chug ? dochug(mtmp) : 0;

    /*
     * A similar check is in monster_nearby() in hack.c.
     * [The two checks have a lot of differences and chances are high
     * that some of those are unintentional.]
     */

    /* check whether hero notices monster and stops current activity */
    if (g.occupation && !rd
        /* monster is hostile and can attack (or hallu distorts knowledge) */
        && (Hallucination || (!mtmp->mpeaceful && !noattacks(mtmp->data)))
        /* it's close enough to be a threat */
        && mdistu(mtmp) <= (BOLT_LIM + 1) * (BOLT_LIM + 1)
        /* and either couldn't see it before, or it was too far away */
        && (!already_saw_mon || !couldsee(x, y)
            || distu(x, y) > (BOLT_LIM + 1) * (BOLT_LIM + 1))
        /* can see it now, or sense it and would normally see it */
        && canspotmon(mtmp) && couldsee(mtmp->mx, mtmp->my)
        /* monster isn't paralyzed or afraid (scare monster/Elbereth) */
        && mtmp->mcanmove && !onscary(u.ux, u.uy, mtmp))
        stop_occupation();

    return rd;
}

boolean
onscary(coordxy x, coordxy y, struct monst *mtmp)
{
    struct engr *ep;

    /* creatures who are directly resistant to magical scaring:
     * humans aren't monsters
     * uniques have ascended their base monster instincts
     * Rodney, lawful minions, Angels, the Riders, shopkeepers
     * inside their own shop, priests inside their own temple */
    if (mtmp->iswiz || is_lminion(mtmp) || mtmp->data == &mons[PM_ANGEL]
        || is_rider(mtmp->data)
        || mtmp->data->mlet == S_HUMAN || unique_corpstat(mtmp->data)
        || (mtmp->isshk && inhishop(mtmp))
        || (mtmp->ispriest && inhistemple(mtmp)))
        return FALSE;

    /* <0,0> is used by musical scaring to check for the above;
     * it doesn't care about scrolls or engravings or dungeon branch */
    if (x == 0 && y == 0)
        return TRUE;

    /* should this still be true for defiled/molochian altars? */
    if (IS_ALTAR(levl[x][y].typ)
        && (mtmp->data->mlet == S_VAMPIRE || is_vampshifter(mtmp)))
        return TRUE;

    /* the scare monster scroll doesn't have any of the below
     * restrictions, being its own source of power */
    if (sobj_at(SCR_SCARE_MONSTER, x, y))
        return TRUE;

    /*
     * Creatures who don't (or can't) fear a written Elbereth:
     * all the above plus shopkeepers (even if poly'd into non-human),
     * vault guards (also even if poly'd), blind or peaceful monsters,
     * humans and elves, and minotaurs.
     *
     * If the player isn't actually on the square OR the player's image
     * isn't displaced to the square, no protection is being granted.
     *
     * Elbereth doesn't work in Gehennom, the Elemental Planes, or the
     * Astral Plane; the influence of the Valar only reaches so far.
     */
    return ((ep = sengr_at("Elbereth", x, y, TRUE)) != 0
            && (u_at(x, y)
                || (Displaced && mtmp->mux == x && mtmp->muy == y)
                || (ep->guardobjects && vobj_at(x, y)))
            && !(mtmp->isshk || mtmp->isgd || !mtmp->mcansee
                 || mtmp->mpeaceful || mtmp->data->mlet == S_HUMAN
                 || mtmp->data == &mons[PM_MINOTAUR]
                 || Inhell || In_endgame(&u.uz)));
}

/* regenerate lost hit points */
void
mon_regen(struct monst *mon, boolean digest_meal)
{
    if (mon->mhp < mon->mhpmax
        && (g.moves % 20 == 0 || regenerates(mon->data)))
        mon->mhp++;
    if (mon->mspec_used)
        mon->mspec_used--;
    if (digest_meal) {
        if (mon->meating) {
            mon->meating--;
            if (mon->meating <= 0)
                finish_meating(mon);
        }
    }
}

/*
 * Possibly awaken the given monster.  Return a 1 if the monster has been
 * jolted awake.
 */
static int
disturb(register struct monst *mtmp)
{
    /*
     * + Ettins are hard to surprise.
     * + Nymphs, jabberwocks, and leprechauns do not easily wake up.
     *
     * Wake up if:
     *  in direct LOS                                           AND
     *  within 10 squares                                       AND
     *  not stealthy or (mon is an ettin and 9/10)              AND
     *  (mon is not a nymph, jabberwock, or leprechaun) or 1/50 AND
     *  Aggravate or mon is (dog or human) or
     *      (1/7 and mon is not mimicing furniture or object)
     */
    if (couldsee(mtmp->mx, mtmp->my) && mdistu(mtmp) <= 100
        && (!Stealth || (mtmp->data == &mons[PM_ETTIN] && rn2(10)))
        && (!(mtmp->data->mlet == S_NYMPH
              || mtmp->data == &mons[PM_JABBERWOCK]
#if 0 /* DEFERRED */
              || mtmp->data == &mons[PM_VORPAL_JABBERWOCK]
#endif
              || mtmp->data->mlet == S_LEPRECHAUN) || !rn2(50))
        && (Aggravate_monster
            || (mtmp->data->mlet == S_DOG || mtmp->data->mlet == S_HUMAN)
            || (!rn2(7) && M_AP_TYPE(mtmp) != M_AP_FURNITURE
                && M_AP_TYPE(mtmp) != M_AP_OBJECT))) {
        wake_msg(mtmp, !mtmp->mpeaceful);
        mtmp->msleeping = 0;
        return 1;
    }
    return 0;
}

/* ungrab/expel held/swallowed hero */
static void
release_hero(struct monst *mon)
{
    if (mon == u.ustuck) {
        if (u.uswallow) {
            expels(mon, mon->data, TRUE);
        } else if (!sticks(g.youmonst.data)) {
            unstuck(mon); /* let go */
            You("get released!");
        }
    }
}

struct monst *
find_pmmonst(int pm)
{
    struct monst *mtmp = 0;

    if ((g.mvitals[pm].mvflags & G_GENOD) == 0)
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (mtmp->data == &mons[pm])
                break;
        }

    return mtmp;
}

/* killer bee 'mon' is on a spot containing lump of royal jelly 'obj' and
   will eat it if there is no queen bee on the level; return 1: mon died,
   0: mon ate jelly and lived; -1: mon didn't eat jelly to use its move */
int
bee_eat_jelly(struct monst* mon, struct obj* obj)
{
    int m_delay;
    struct monst *mtmp = find_pmmonst(PM_QUEEN_BEE);

    /* if there's no queen on the level, eat the royal jelly and become one */
    if (!mtmp) {
        m_delay = obj->blessed ? 3 : !obj->cursed ? 5 : 7;
        if (obj->quan > 1L)
            obj = splitobj(obj, 1L);
        if (canseemon(mon))
            pline("%s eats %s.", Monnam(mon), an(xname(obj)));
        delobj(obj);

        if ((int) mon->m_lev < mons[PM_QUEEN_BEE].mlevel - 1)
            mon->m_lev = (uchar) (mons[PM_QUEEN_BEE].mlevel - 1);
        /* there should be delay after eating, but that's too much
           hassle; transform immediately, then have a short delay */
        (void) grow_up(mon, (struct monst *) 0);

        if (DEADMONSTER(mon))
            return 1; /* dead; apparently queen bees have been genocided */
        mon->mfrozen = m_delay, mon->mcanmove = 0;
        return 0; /* bee used its move */
    }
    return -1; /* a queen is already present; ordinary bee hasn't moved yet */
}

/* FIXME: gremlins don't flee from monsters wielding Sunsword or wearing
   gold dragon scales/mail, nor from gold dragons, only from the hero */
#define flees_light(mon) \
    ((mon)->data == &mons[PM_GREMLIN]                                     \
     && ((uwep && uwep->lamplit && artifact_light(uwep))                  \
         || (uarm && uarm->lamplit && artifact_light(uarm)))              \
     /* not applicable if mon can't see or hero isn't in line of sight */ \
     && mon->mcansee && couldsee(mon->mx, mon->my))                       \
     /* doesn't matter if hero is invisible--light being emitted isn't */

/* monster begins fleeing for the specified time, 0 means untimed flee
 * if first, only adds fleetime if monster isn't already fleeing
 * if fleemsg, prints a message about new flight, otherwise, caller should */
void
monflee(
    struct monst *mtmp,
    int fleetime,
    boolean first,
    boolean fleemsg)
{
    /* shouldn't happen; maybe warrants impossible()? */
    if (DEADMONSTER(mtmp))
        return;

    if (mtmp == u.ustuck)
        release_hero(mtmp); /* expels/unstuck */

    if (!first || !mtmp->mflee) {
        /* don't lose untimed scare */
        if (!fleetime)
            mtmp->mfleetim = 0;
        else if (!mtmp->mflee || mtmp->mfleetim) {
            fleetime += (int) mtmp->mfleetim;
            /* ensure monster flees long enough to visibly stop fighting */
            if (fleetime == 1)
                fleetime++;
            mtmp->mfleetim = (unsigned) min(fleetime, 127);
        }
        if (!mtmp->mflee && fleemsg && canseemon(mtmp)
            && M_AP_TYPE(mtmp) != M_AP_FURNITURE
            && M_AP_TYPE(mtmp) != M_AP_OBJECT) {
            /* unfortunately we can't distinguish between temporary
               sleep and temporary paralysis, so both conditions
               receive the same alternate message */
            if (!mtmp->mcanmove || !mtmp->data->mmove) {
                pline("%s seems to flinch.", Adjmonnam(mtmp, "immobile"));
            } else if (flees_light(mtmp)) {
                if (Unaware) {
                    /* tell the player even if the hero is unconscious */
                    pline("%s is frightened.", Monnam(mtmp));
                } else if (rn2(10) || Deaf) {
                    /* via flees_light(), will always be either via uwep
                       (Sunsword) or uarm (gold dragon scales/mail) or both;
                       TODO? check for both and describe the one which is
                       emitting light with a bigger radius */
                    const char *lsrc = (uwep && artifact_light(uwep))
                                       ? bare_artifactname(uwep)
                                       : (uarm && artifact_light(uarm))
                                         ? yname(uarm)
                                         : "[its imagination?]";

                    pline("%s flees from the painful light of %s.",
                          Monnam(mtmp), lsrc);
                } else {
                    verbalize("Bright light!");
                }
            } else {
                pline("%s turns to flee.", Monnam(mtmp));
            }
        }

        if (mtmp->data == &mons[PM_VROCK] && !mtmp->mspec_used) {
            mtmp->mspec_used = 75 + rn2(25);
            (void) create_gas_cloud(mtmp->mx, mtmp->my, 5, 8);
        }

        mtmp->mflee = 1;
    }
    /* ignore recently-stepped spaces when made to flee */
    mon_track_clear(mtmp);
}

static void
distfleeck(
    struct monst *mtmp,
    int *inrange, int *nearby, int *scared) /* output */
{
    int seescaryx, seescaryy;
    boolean sawscary = FALSE, bravegremlin = (rn2(5) == 0);

    *inrange = (dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy)
                <= (BOLT_LIM * BOLT_LIM));
    *nearby = *inrange && monnear(mtmp, mtmp->mux, mtmp->muy);

    /* Note: if your image is displaced, the monster sees the Elbereth
     * at your displaced position, thus never attacking your displaced
     * position, but possibly attacking you by accident.  If you are
     * invisible, it sees the Elbereth at your real position, thus never
     * running into you by accident but possibly attacking the spot
     * where it guesses you are.
     */
    if (!mtmp->mcansee || (Invis && !perceives(mtmp->data))) {
        seescaryx = mtmp->mux;
        seescaryy = mtmp->muy;
    } else {
        seescaryx = u.ux;
        seescaryy = u.uy;
    }

    sawscary = onscary(seescaryx, seescaryy, mtmp);
    if (*nearby && (sawscary
                    || (flees_light(mtmp) && !bravegremlin)
                    || (!mtmp->mpeaceful && in_your_sanctuary(mtmp, 0, 0)))) {
        *scared = 1;
        monflee(mtmp, rnd(rn2(7) ? 10 : 100), TRUE, TRUE);
    } else
        *scared = 0;
}

#undef flees_light

/* perform a special one-time action for a monster; returns -1 if nothing
   special happened, 0 if monster uses up its turn, 1 if monster is killed */
static int
m_arrival(struct monst* mon)
{
    mon->mstrategy &= ~STRAT_ARRIVE; /* always reset */

    return -1;
}

/* a mind flayer unleashes a mind blast  */
static void
mind_blast(register struct monst* mtmp)
{
    struct monst *m2, *nmon = (struct monst *) 0;

    if (canseemon(mtmp))
        pline("%s concentrates.", Monnam(mtmp));
    if (mdistu(mtmp) > BOLT_LIM * BOLT_LIM) {
        You("sense a faint wave of psychic energy.");
        return;
    }
    pline("A wave of psychic energy pours over you!");
    if (mtmp->mpeaceful
        && (!Conflict || resist_conflict(mtmp))) {
        pline("It feels quite soothing.");
    } else if (!u.uinvulnerable) {
        int dmg;
        boolean m_sen = sensemon(mtmp);

        if (m_sen || (Blind_telepat && rn2(2)) || !rn2(10)) {
            /* hiding monsters are brought out of hiding when hit by
                a psychic blast, so do the same for hiding poly'd hero */
            if (u.uundetected) {
                u.uundetected = 0;
                newsym(u.ux, u.uy);
            } else if (U_AP_TYPE != M_AP_NOTHING
                        /* hero has no way to hide as monster but
                            check for that theoretical case anyway */
                        && U_AP_TYPE != M_AP_MONSTER) {
                g.youmonst.m_ap_type = M_AP_NOTHING;
                g.youmonst.mappearance = 0;
                newsym(u.ux, u.uy);
            }
            pline("It locks on to your %s!",
                    m_sen ? "telepathy"
                    : Blind_telepat ? "latent telepathy"
                    : "mind"); /* note: hero is never mindless */
            dmg = rnd(15);
            if (Half_spell_damage)
                dmg = (dmg + 1) / 2;
            losehp(dmg, "psychic blast", KILLED_BY_AN);
        }
    }
    for (m2 = fmon; m2; m2 = nmon) {
        nmon = m2->nmon;
        if (DEADMONSTER(m2))
            continue;
        if (m2->mpeaceful == mtmp->mpeaceful)
            continue;
        if (mindless(m2->data))
            continue;
        if (m2 == mtmp)
            continue;
        if ((telepathic(m2->data) && (rn2(2) || m2->mblinded)) || !rn2(10)) {
            /* wake it up first, to bring hidden monster out of hiding */
            wakeup(m2, FALSE);
            if (cansee(m2->mx, m2->my))
                pline("It locks on to %s.", mon_nam(m2));
            m2->mhp -= rnd(15);
            if (DEADMONSTER(m2))
                monkilled(m2, "", AD_DRIN);
        }
    }
}

/* returns 1 if monster died moving, 0 otherwise */
/* The whole dochugw/m_move/distfleeck/mfndpos section is serious spaghetti
 * code. --KAA
 */
int
dochug(register struct monst* mtmp)
{
    register struct permonst *mdat;
    register int status = MMOVE_NOTHING;
    int inrange, nearby, scared, res;
    struct obj *otmp;
    boolean panicattk = FALSE;

    /*
     * PHASE ONE: Pre-movement adjustments
     */

    mdat = mtmp->data;

    if (mtmp->mstrategy & STRAT_ARRIVE) {
        res = m_arrival(mtmp);
        if (res >= 0)
            return res;
    }
    /* check for waitmask status change */
    if ((mtmp->mstrategy & STRAT_WAITFORU)
        && (m_canseeu(mtmp) || mtmp->mhp < mtmp->mhpmax))
        mtmp->mstrategy &= ~STRAT_WAITFORU;

    /* update quest status flags */
    quest_stat_check(mtmp);

    if (!mtmp->mcanmove || (mtmp->mstrategy & STRAT_WAITMASK)) {
        if (Hallucination)
            newsym(mtmp->mx, mtmp->my);
        if (mtmp->mcanmove && (mtmp->mstrategy & STRAT_CLOSE)
            && !mtmp->msleeping && monnear(mtmp, u.ux, u.uy))
            quest_talk(mtmp); /* give the leaders a chance to speak */
        return 0;             /* other frozen monsters can't do anything */
    }

    /* there is a chance we will wake it */
    if (mtmp->msleeping && !disturb(mtmp)) {
        if (Hallucination)
            newsym(mtmp->mx, mtmp->my);
        return 0;
    }

    /* not frozen or sleeping: wipe out texts written in the dust */
    wipe_engr_at(mtmp->mx, mtmp->my, 1, FALSE);

    /* confused monsters get unconfused with small probability */
    if (mtmp->mconf && !rn2(50))
        mtmp->mconf = 0;

    /* stunned monsters get un-stunned with larger probability */
    if (mtmp->mstun && !rn2(10))
        mtmp->mstun = 0;

    /* Some monsters teleport. Teleportation costs a turn. */
    if (mtmp->mflee && !rn2(40) && can_teleport(mdat) && !mtmp->iswiz
        && !noteleport_level(mtmp)) {
        if (rloc(mtmp, RLOC_MSG))
            leppie_stash(mtmp);
        return 0;
    }

    /* Shriekers and Medusa have irregular abilities which must be
       checked every turn. These abilities do not cost a turn when
       used. */
    if (mdat->msound == MS_SHRIEK && !um_dist(mtmp->mx, mtmp->my, 1))
        m_respond(mtmp);
    if (mdat == &mons[PM_MEDUSA] && couldsee(mtmp->mx, mtmp->my))
        m_respond(mtmp);
    if (DEADMONSTER(mtmp))
        return 1; /* m_respond gaze can kill medusa */

    /* fleeing monsters might regain courage */
    if (mtmp->mflee && !mtmp->mfleetim && mtmp->mhp == mtmp->mhpmax
        && !rn2(25))
        mtmp->mflee = 0;

    /* Cease conflict-induced swallow/grab if conflict has ended. Releasing
       the hero in this way uses up the monster's turn. */
    if (mtmp == u.ustuck && mtmp->mpeaceful && !mtmp->mconf && !Conflict) {
        release_hero(mtmp);
        return 0;
    }

    /*
     * PHASE TWO: Special Movements and Actions
     */

    /* The monster decides where it thinks you are. This call to set_apparxy()
       must be done after you move and before the monster does. The
       set_apparxy() call in m_move() doesn't suffice since the variables
       inrange, etc. all depend on stuff set by set_apparxy().
     */
    set_apparxy(mtmp);

    /* Monsters that want to acquire things may teleport, so do it before
       inrange is set. This costs a turn only if mstate is set.  */
    if (is_covetous(mdat)) {
        (void) tactics(mtmp);
        /* tactics -> mnexto -> deal_with_overcrowding */
        if (mtmp->mstate)
            return 0;
    }

    /* check distance and scariness of attacks */
    distfleeck(mtmp, &inrange, &nearby, &scared);

    /* search for and potentially use defensive or miscellaneous items. */
    if (find_defensive(mtmp, FALSE)) {
        if (use_defensive(mtmp) != 0)
            return 1;
    } else if (find_misc(mtmp)) {
        if (use_misc(mtmp) != 0)
            return 1;
    }

    /* Demonic Blackmail! */
    if (nearby && mdat->msound == MS_BRIBE && mtmp->mpeaceful && !mtmp->mtame
        && !u.uswallow) {
        if (mtmp->mux != u.ux || mtmp->muy != u.uy) {
            pline("%s whispers at thin air.",
                  cansee(mtmp->mux, mtmp->muy) ? Monnam(mtmp) : "It");

            if (is_demon(g.youmonst.data)) {
                /* "Good hunting, brother" */
                if (!tele_restrict(mtmp))
                    (void) rloc(mtmp, RLOC_MSG);
            } else {
                mtmp->minvis = mtmp->perminvis = 0;
                /* Why?  For the same reason in real demon talk */
                pline("%s gets angry!", Amonnam(mtmp));
                mtmp->mpeaceful = 0;
                set_malign(mtmp);
                /* since no way is an image going to pay it off */
            }
        } else if (demon_talk(mtmp))
            return 1; /* you paid it off */
    }

    /* the watch will look around and see if you are up to no good :-) */
    if (is_watch(mdat)) {
        watch_on_duty(mtmp);
    /* mind flayers can make psychic attacks! */
    } else if (is_mind_flayer(mdat) && !rn2(20)) {
        mind_blast(mtmp);
    }

    /* If monster is nearby you, and has to wield a weapon, do so.  This
     * costs the monster a move, of course.
     */
    if ((!mtmp->mpeaceful || Conflict) && inrange
        && dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= 8
        && attacktype(mdat, AT_WEAP)) {
        struct obj *mw_tmp;

        /* The scared check is necessary.  Otherwise a monster that is
         * one square near the player but fleeing into a wall would keep
         * switching between pick-axe and weapon.  If monster is stuck
         * in a trap, prefer ranged weapon (wielding is done in thrwmu).
         * This may cost the monster an attack, but keeps the monster
         * from switching back and forth if carrying both.
         */
        mw_tmp = MON_WEP(mtmp);
        if (!(scared && mw_tmp && is_pick(mw_tmp))
            && mtmp->weapon_check == NEED_WEAPON
            && !(mtmp->mtrapped && !nearby && select_rwep(mtmp))) {
            mtmp->weapon_check = NEED_HTH_WEAPON;
            if (mon_wield_item(mtmp) != 0)
                return 0;
        }
    }

    /*
     * PHASE THREE: Now the actual movement phase
     */

    /* Hezrous create clouds of stench. This does not cost a move. */
    if (mtmp->data == &mons[PM_HEZROU]) /* stench */
        create_gas_cloud(mtmp->mx, mtmp->my, 1, 8);

    /* A killer bee may eat honey in order to turn into a queen bee,
       costing it a move. */
    if (mdat == &mons[PM_KILLER_BEE]
        /* could be smarter and deliberately move to royal jelly, but
           then we'd need to scan the level for queen bee in advance;
           avoid that overhead and rely on serendipity... */
        && (otmp = sobj_at(LUMP_OF_ROYAL_JELLY, mtmp->mx, mtmp->my)) != 0
        && (res = bee_eat_jelly(mtmp, otmp)) >= 0)
        return res;

    /* A monster that passes the following checks has the opportunity
       to move. Movement itself is handled by the m_move() function. */
    if (!nearby || mtmp->mflee || scared || mtmp->mconf || mtmp->mstun
        || (mtmp->minvis && !rn2(3))
        || (mdat->mlet == S_LEPRECHAUN && !findgold(g.invent)
            && (findgold(mtmp->minvent) || rn2(2)))
        || (is_wanderer(mdat) && !rn2(4)) || (Conflict && !mtmp->iswiz)
        || (!mtmp->mcansee && !rn2(4)) || mtmp->mpeaceful) {

        /* Possibly cast an undirected spell if not attacking you */
        /* note that most of the time castmu() will pick a directed
           spell and do nothing, so the monster moves normally */
        /* arbitrary distance restriction to keep monster far away
           from you from having cast dozens of sticks-to-snakes
           or similar spells by the time you reach it */
        if (!mtmp->mspec_used
            && dist2(mtmp->mx, mtmp->my, u.ux, u.uy) <= 49) {
            struct attack *a;

            for (a = &mdat->mattk[0]; a < &mdat->mattk[NATTK]; a++) {
                if (a->aatyp == AT_MAGC
                    && (a->adtyp == AD_SPEL || a->adtyp == AD_CLRC)) {
                    if ((castmu(mtmp, a, FALSE, FALSE) & MM_HIT)) {
                        status = MMOVE_DONE; /* bypass m_move() */
                        break;
                    }
                }
            }
        }

        if (!status)
            status = m_move(mtmp, 0);
        if (status != MMOVE_DIED)
            distfleeck(mtmp, &inrange, &nearby, &scared); /* recalc */

        switch (status) { /* for pets, cases 0 and 3 are equivalent */
        case MMOVE_NOMOVES:
            if (scared)
                panicattk = TRUE;
            /*FALLTHRU*/
        case MMOVE_NOTHING: /* no movement, but it can still attack you */
        case MMOVE_DONE: /* absolutely no movement */
            /* vault guard might have vanished */
            if (mtmp->isgd && (DEADMONSTER(mtmp) || mtmp->mx == 0))
                return 1; /* behave as if it died */
            /* During hallucination, monster appearance should
             * still change - even if it doesn't move.
             */
            if (Hallucination)
                newsym(mtmp->mx, mtmp->my);
            break;
        case MMOVE_MOVED: /* monster moved */
            /* if confused grabber has wandered off, let go */
            if (mtmp == u.ustuck && !next2u(mtmp->mx, mtmp->my))
                unstuck(mtmp);
            /* Maybe it stepped on a trap and fell asleep... */
            if (helpless(mtmp))
                return 0;
            /* Monsters can move and then shoot on same turn;
               our hero can't.  Is that fair? */
            if (!nearby && (ranged_attk(mdat)
                            || attacktype(mdat, AT_WEAP)
                            || find_offensive(mtmp)))
                break;
            /* a monster that's digesting you can move at the
             * same time -dlc
             */
            if (engulfing_u(mtmp))
                return mattacku(mtmp);
            return 0;
        case MMOVE_DIED: /* monster died */
            return 1;
        }
    }

    /*
     * PHASE FOUR: Standard Attacks
     */

    /* Now, attack the player if possible - one attack set per monst */
    if (status != MMOVE_DONE && (!mtmp->mpeaceful
                                 || (Conflict && !resist_conflict(mtmp)))) {
        if (((inrange && !scared) || panicattk) && !noattacks(mdat)
            /* [is this hp check really needed?] */
            && (Upolyd ? u.mh : u.uhp) > 0) {
            if (mattacku(mtmp))
                return 1; /* monster died (e.g. exploded) */
        }
        if (mtmp->wormno) {
            if (wormhitu(mtmp))
                return 1; /* worm died (poly'd hero passive counter-attack) */
        }
    }
    /* special speeches for quest monsters */
    if (!helpless(mtmp) && nearby)
        quest_talk(mtmp);
    /* extra emotional attack for vile monsters */
    if (inrange && mtmp->data->msound == MS_CUSS && !mtmp->mpeaceful
        && couldsee(mtmp->mx, mtmp->my) && !mtmp->minvis && !rn2(5))
        cuss(mtmp);

    /* note: can't get here when monster is dead, so this always returns 0 */
    return (status == MMOVE_DIED);
}

static NEARDATA const char practical[] = { WEAPON_CLASS, ARMOR_CLASS,
                                           GEM_CLASS, FOOD_CLASS, 0 };
static NEARDATA const char magical[] = { AMULET_CLASS, POTION_CLASS,
                                         SCROLL_CLASS, WAND_CLASS,
                                         RING_CLASS,   SPBOOK_CLASS, 0 };
static NEARDATA const char indigestion[] = { BALL_CLASS, ROCK_CLASS, 0 };
static NEARDATA const char boulder_class[] = { ROCK_CLASS, 0 };
static NEARDATA const char gem_class[] = { GEM_CLASS, 0 };

boolean
itsstuck(register struct monst* mtmp)
{
    if (sticks(g.youmonst.data) && mtmp == u.ustuck && !u.uswallow) {
        pline("%s cannot escape from you!", Monnam(mtmp));
        return TRUE;
    }
    return FALSE;
}

/*
 * should_displace()
 *
 * Displacement of another monster is a last resort and only
 * used on approach. If there are better ways to get to target,
 * those should be used instead. This function does that evaluation.
 */
boolean
should_displace(
    struct monst *mtmp,
    coord *poss, /* coord poss[9] */
    long *info,  /* long info[9] */
    int cnt,
    coordxy gx,
    coordxy gy)
{
    int shortest_with_displacing = -1;
    int shortest_without_displacing = -1;
    int count_without_displacing = 0;
    register int i, nx, ny;
    int ndist;

    for (i = 0; i < cnt; i++) {
        nx = poss[i].x;
        ny = poss[i].y;
        ndist = dist2(nx, ny, gx, gy);
        if (MON_AT(nx, ny) && (info[i] & ALLOW_MDISP) && !(info[i] & ALLOW_M)
            && !undesirable_disp(mtmp, nx, ny)) {
            if (shortest_with_displacing == -1
                || (ndist < shortest_with_displacing))
                shortest_with_displacing = ndist;
        } else {
            if ((shortest_without_displacing == -1)
                || (ndist < shortest_without_displacing))
                shortest_without_displacing = ndist;
            count_without_displacing++;
        }
    }
    if (shortest_with_displacing > -1
        && (shortest_with_displacing < shortest_without_displacing
            || !count_without_displacing))
        return TRUE;
    return FALSE;
}

/* have monster wield a pick-axe if it wants to dig and it has one;
   return True if it spends this move wielding one, False otherwise */
boolean
m_digweapon_check(
    struct monst *mtmp,
    coordxy nix, coordxy niy)
{
    boolean can_tunnel = FALSE;
    struct obj *mw_tmp = MON_WEP(mtmp);

    if (!Is_rogue_level(&u.uz))
        can_tunnel = tunnels(mtmp->data);

    if (can_tunnel && needspick(mtmp->data) && !mwelded(mw_tmp)
        && (may_dig(nix, niy) || closed_door(nix, niy))) {
        /* may_dig() is either IS_STWALL or IS_TREE */
        if (closed_door(nix, niy)) {
            if (!mw_tmp || !is_pick(mw_tmp) || !is_axe(mw_tmp))
                mtmp->weapon_check = NEED_PICK_OR_AXE;
        } else if (IS_TREE(levl[nix][niy].typ)) {
            if (!mw_tmp || !is_axe(mw_tmp))
                mtmp->weapon_check = NEED_AXE;
        } else if (IS_STWALL(levl[nix][niy].typ)) {
            if (!mw_tmp || !is_pick(mw_tmp))
                mtmp->weapon_check = NEED_PICK_AXE;
        }
        if (mtmp->weapon_check >= NEED_PICK_AXE && mon_wield_item(mtmp))
            return TRUE;
    }
    return FALSE;
}

/* does leprechaun want to avoid the hero? */
static boolean
leppie_avoidance(struct monst *mtmp)
{
    struct obj *lepgold, *ygold;

    if (mtmp->data == &mons[PM_LEPRECHAUN]
        && ((lepgold = findgold(mtmp->minvent))
            && (lepgold->quan
                > ((ygold = findgold(g.invent)) ? ygold->quan : 0L))))
        return TRUE;

    return FALSE;
}

/* unseen leprechaun with gold might stash it */
static void
leppie_stash(struct monst *mtmp)
{
    struct obj *gold;

    if (mtmp->data == &mons[PM_LEPRECHAUN]
        && !DEADMONSTER(mtmp)
        && !m_canseeu(mtmp)
        && !*in_rooms(mtmp->mx, mtmp->my, SHOPBASE)
        && levl[mtmp->mx][mtmp->my].typ == ROOM
        && !t_at(mtmp->mx, mtmp->my)
        && rn2(4)
        && (gold = findgold(mtmp->minvent)) != 0) {
        mdrop_obj(mtmp, gold, FALSE);
        gold = g_at(mtmp->mx, mtmp->my);
        if (gold)
            (void) bury_an_obj(gold, (boolean *) 0);
    }
}

/* does monster want to avoid you? */
static boolean
m_balks_at_approaching(struct monst* mtmp)
{
    /* peaceful, far away, or can't see you */
    if (mtmp->mpeaceful
        || (dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) >= 5*5)
        || !m_canseeu(mtmp))
        return FALSE;

    /* has ammo+launcher */
    if (m_has_launcher_and_ammo(mtmp))
        return TRUE;

    /* is using a polearm and in range */
    if (MON_WEP(mtmp) && is_pole(MON_WEP(mtmp))
        && dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= MON_POLE_DIST)
        return TRUE;

    /* can attack from distance, and hp loss or attack not used */
    if (ranged_attk(mtmp->data)
        && ((mtmp->mhp < (mtmp->mhpmax+1) / 3)
            || !mtmp->mspec_used))
        return TRUE;

    return FALSE;
}

static boolean
holds_up_web(coordxy x, coordxy y)
{
    stairway *sway;

    if (!isok(x, y)
        || IS_ROCK(levl[x][y].typ)
        || ((levl[x][y].typ == STAIRS || levl[x][y].typ == LADDER)
            && (sway = stairway_at(x, y)) != 0 && sway->up)
        || levl[x][y].typ == IRONBARS)
        return TRUE;

    return FALSE;
}

/* returns the number of walls in the four cardinal directions that could
   hold up a web */
static int
count_webbing_walls(coordxy x, coordxy y)
{
    return (holds_up_web(x, y - 1) + holds_up_web(x + 1, y)
            + holds_up_web(x, y + 1) + holds_up_web(x - 1, y));
}

/* reject webs which interfere with solving Sokoban */
static boolean
soko_allow_web(struct monst *mon)
{
    stairway *stway;

    /* for a non-Sokoban level or a solved Sokoban level, no restriction */
    if (!Sokoban)
        return TRUE;
    /* not-yet-solved Sokoban level:  allow web only when spinner can see
       the stairs up [we really want 'is in same chamber as stairs up'] */
    stway = stairway_find_dir(TRUE); /* stairs up */
    if (stway && m_cansee(mon, stway->sx, stway->sy))
        return TRUE;
    return FALSE;
}

/* monster might spin a web */
static void
maybe_spin_web(struct monst *mtmp)
{
    if (webmaker(mtmp->data)
        && !helpless(mtmp) && !mtmp->mspec_used
        && !t_at(mtmp->mx, mtmp->my) && soko_allow_web(mtmp)) {
        struct trap *trap;
        int prob = ((((mtmp->data == &mons[PM_GIANT_SPIDER]) ? 15 : 5)
                     * (count_webbing_walls(mtmp->mx, mtmp->my) + 1))
                    - (3 * count_traps(WEB)));

        if (rn2(1000) < prob
            && (trap = maketrap(mtmp->mx, mtmp->my, WEB)) != 0) {
            mtmp->mspec_used = d(4, 4); /* 4..16 */
            if (cansee(mtmp->mx, mtmp->my)) {
                char mbuf[BUFSZ];

                Strcpy(mbuf, canspotmon(mtmp) ? y_monnam(mtmp) : something);
                pline("%s spins a web.", upstart(mbuf));
                trap->tseen = 1;
            }
            if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE))
                add_damage(mtmp->mx, mtmp->my, 0L);
        }
    }
}

/* Handles the movement of a standard monster. */
/* Return values:
 * 0: did not move, but can still attack and do other stuff.
 * 1: moved, possibly can attack.
 * 2: monster died.
 * 3: did not move, and can't do anything else either.
 */
int
m_move(register struct monst* mtmp, register int after)
{
    int appr, etmp;
    coordxy gx, gy, nix, niy;
    xint16 chcnt;
    int chi; /* could be schar except for stupid Sun-2 compiler */
    boolean likegold = 0, likegems = 0, likeobjs = 0, likemagic = 0,
            conceals = 0;
    boolean likerock = 0, can_tunnel = 0;
    boolean can_open = 0, can_unlock = 0 /*, doorbuster = 0 */;
    boolean uses_items = 0, setlikes = 0;
    boolean avoid = FALSE;
    boolean better_with_displacing = FALSE;
    boolean sawmon = canspotmon(mtmp); /* before it moved */
    struct permonst *ptr;
    struct monst *mtoo;
    schar mmoved = MMOVE_NOTHING; /* not strictly nec.: chi >= 0 will do */
    long info[9];
    long flag;
    int omx = mtmp->mx, omy = mtmp->my;

    if (mtmp->mtrapped) {
        int i = mintrap(mtmp, NO_TRAP_FLAGS);

        if (i == Trap_Killed_Mon) {
            newsym(mtmp->mx, mtmp->my);
            return MMOVE_DIED;
        } /* it died */
        if (i == Trap_Caught_Mon)
            return MMOVE_NOTHING; /* still in trap, so didn't move */
    }
    ptr = mtmp->data; /* mintrap() can change mtmp->data -dlc */

    if (mtmp->meating) {
        mtmp->meating--;
        if (mtmp->meating <= 0)
            finish_meating(mtmp);
        return MMOVE_DONE; /* still eating */
    }
    if (hides_under(ptr) && OBJ_AT(mtmp->mx, mtmp->my) && rn2(10))
        return MMOVE_NOTHING; /* do not leave hiding place */

    /* Where does 'mtmp' think you are?  Not necessary if m_move() called
       from this file, but needed for other calls of m_move(). */
    set_apparxy(mtmp); /* set mtmp->mux, mtmp->muy */

    if (!Is_rogue_level(&u.uz))
        can_tunnel = tunnels(ptr);
    can_open = !(nohands(ptr) || verysmall(ptr));
    can_unlock = ((can_open && monhaskey(mtmp, TRUE))
                  || mtmp->iswiz || is_rider(ptr));
    /* doorbuster = is_giant(ptr); */
    if (mtmp->wormno)
        goto not_special;
    /* my dog gets special treatment */
    if (mtmp->mtame) {
        mmoved = dog_move(mtmp, after);
        goto postmov;
    }

    /* and the acquisitive monsters get special treatment */
    if (is_covetous(ptr)) {
        coordxy tx = STRAT_GOALX(mtmp->mstrategy),
              ty = STRAT_GOALY(mtmp->mstrategy);
        struct monst *intruder = m_at(tx, ty);
        /*
         * if there's a monster on the object or in possession of it,
         * attack it.
         */
        if ((dist2(mtmp->mx, mtmp->my, tx, ty) < 2) && intruder
            && (intruder != mtmp)) {
            g.notonhead = (intruder->mx != tx || intruder->my != ty);
            if (mattackm(mtmp, intruder) == 2)
                return MMOVE_DIED;
            mmoved = MMOVE_MOVED;
        } else
            mmoved = MMOVE_NOTHING;
        goto postmov;
    }

    /* likewise for shopkeeper, guard, or priest */
    if (mtmp->isshk || mtmp->isgd || mtmp->ispriest) {
        int xm = mtmp->isshk ? shk_move(mtmp)
                 : mtmp->isgd ? gd_move(mtmp)
                   : pri_move(mtmp);

        switch (xm) {
        case -2:
            return MMOVE_DIED;
        case -1:
            mmoved = MMOVE_NOTHING; /* shk follow hero outside shop */
            break;
        case 0:
            mmoved = MMOVE_NOTHING;
            goto postmov;
        case 1:
            mmoved = MMOVE_MOVED;
            goto postmov;
        default: impossible("unknown shk/gd/pri_move return value (%i)", xm);
            mmoved = MMOVE_NOTHING;
            goto postmov;
        }
    }

#ifdef MAIL_STRUCTURES
    if (ptr == &mons[PM_MAIL_DAEMON]) {
        if (!Deaf && canseemon(mtmp))
            verbalize("I'm late!");
        mongone(mtmp);
        return MMOVE_DIED;
    }
#endif

    /* teleport if that lies in our nature */
    if (ptr == &mons[PM_TENGU] && !rn2(5) && !mtmp->mcan
        && !tele_restrict(mtmp)) {
        if (mtmp->mhp < 7 || mtmp->mpeaceful || rn2(2))
            (void) rloc(mtmp, RLOC_MSG);
        else
            mnexto(mtmp, RLOC_MSG);
        mmoved = MMOVE_MOVED;
        goto postmov;
    }
 not_special:
    if (u.uswallow && !mtmp->mflee && u.ustuck != mtmp)
        return MMOVE_MOVED;
    omx = mtmp->mx;
    omy = mtmp->my;
    gx = mtmp->mux;
    gy = mtmp->muy;
    appr = mtmp->mflee ? -1 : 1;
    if (mtmp->mconf || engulfing_u(mtmp)) {
        appr = 0;
    } else {
        boolean should_see = (couldsee(omx, omy)
                              && (levl[gx][gy].lit || !levl[omx][omy].lit)
                              && (dist2(omx, omy, gx, gy) <= 36));

        if (!mtmp->mcansee
            || (should_see && Invis && !perceives(ptr) && rn2(11))
            || is_obj_mappear(&g.youmonst, STRANGE_OBJECT) || u.uundetected
            || (is_obj_mappear(&g.youmonst, GOLD_PIECE) && !likes_gold(ptr))
            || (mtmp->mpeaceful && !mtmp->isshk) /* allow shks to follow */
            || ((monsndx(ptr) == PM_STALKER || ptr->mlet == S_BAT
                 || ptr->mlet == S_LIGHT) && !rn2(3)))
            appr = 0;

        if (appr == 1 && leppie_avoidance(mtmp))
            appr = -1;

        /* hostiles with ranged weapon or attack try to stay away */
        if (m_balks_at_approaching(mtmp))
            appr = -1;

        if (!should_see && can_track(ptr)) {
            register coord *cp;

            cp = gettrack(omx, omy);
            if (cp) {
                gx = cp->x;
                gy = cp->y;
            }
        }
    }

    if ((!mtmp->mpeaceful || !rn2(10)) && (!Is_rogue_level(&u.uz))) {
        boolean in_line = (lined_up(mtmp)
             && (distmin(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy)
                 <= (throws_rocks(g.youmonst.data) ? 20 : ACURRSTR / 2 + 1)));

        if (appr != 1 || !in_line) {
            /* Monsters in combat won't pick stuff up, avoiding the
             * situation where you toss arrows at it and it has nothing
             * better to do than pick the arrows up.
             */
            register int pctload =
                (curr_mon_load(mtmp) * 100) / max_mon_load(mtmp);

            /* look for gold or jewels nearby */
            likegold = (likes_gold(ptr) && pctload < 95);
            likegems = (likes_gems(ptr) && pctload < 85);
            uses_items = (!mindless(ptr) && !is_animal(ptr) && pctload < 75);
            likeobjs = (likes_objs(ptr) && pctload < 75);
            likemagic = (likes_magic(ptr) && pctload < 85);
            likerock = (throws_rocks(ptr) && pctload < 50 && !Sokoban);
            conceals = hides_under(ptr);
            setlikes = TRUE;
        }
    }

#define SQSRCHRADIUS 5

    {
        register int minr = SQSRCHRADIUS; /* not too far away */
        register struct obj *otmp;
        register coordxy xx, yy;
        coordxy oomx, oomy, lmx, lmy;
        struct trap *ttmp;

        /* cut down the search radius if it thinks character is closer. */
        if (distmin(mtmp->mux, mtmp->muy, omx, omy) < SQSRCHRADIUS
            && !mtmp->mpeaceful)
            minr--;
        /* guards shouldn't get too distracted */
        if (!mtmp->mpeaceful && is_mercenary(ptr))
            minr = 1;

        if ((likegold || likegems || likeobjs || likemagic || likerock
             || conceals) && (!*in_rooms(omx, omy, SHOPBASE)
                              || (!rn2(25) && !mtmp->isshk))) {
 look_for_obj:
            oomx = min(COLNO - 1, omx + minr);
            oomy = min(ROWNO - 1, omy + minr);
            lmx = max(1, omx - minr);
            lmy = max(0, omy - minr);
            for (otmp = fobj; otmp; otmp = otmp->nobj) {
                /* monsters may pick rocks up, but won't go out of their way
                   to grab them; this might hamper sling wielders, but it cuts
                   down on move overhead by filtering out most common item */
                if (otmp->otyp == ROCK)
                    continue;
                /* avoid special items; once hero picks them up, they'll
                   cease being special */
                if (is_mines_prize(otmp) || is_soko_prize(otmp))
                    continue;

                xx = otmp->ox;
                yy = otmp->oy;
                /* Nymphs take everything.  Most other creatures should not
                 * pick up corpses except as a special case like in
                 * searches_for_item().  We need to do this check in
                 * mpickstuff() as well.
                 */
                if (xx >= lmx && xx <= oomx && yy >= lmy && yy <= oomy) {
                    /* don't get stuck circling around object that's
                       underneath an immobile or hidden monster;
                       paralysis victims excluded */
                    if ((mtoo = m_at(xx, yy)) != 0
                        && (helpless(mtoo) || mtoo->mundetected
                            || (mtoo->mappearance && !mtoo->iswiz)
                            || !mtoo->data->mmove))
                        continue;
                    /* the mfndpos() test for whether to allow a move to a
                       water location accepts flyers, but they can't reach
                       underwater objects, so being able to move to a spot
                       is insufficient for deciding whether to do so */
                    if ((is_pool(xx, yy) && !is_swimmer(ptr))
                        || (is_lava(xx, yy) && !likes_lava(ptr)))
                        continue;

                    /* ignore obj if there's a trap and monster knows it */
                    if ((ttmp = t_at(xx, yy)) != 0
                        && mon_knows_traps(mtmp, ttmp->ttyp)) {
                        if (gx == xx && gy == yy) {
                            gx = mtmp->mux;
                            gy = mtmp->muy;
                        }
                        continue;
                    }

                    if (((likegold && otmp->oclass == COIN_CLASS)
                         || (likeobjs && strchr(practical, otmp->oclass)
                             && (otmp->otyp != CORPSE
                                 || (ptr->mlet == S_NYMPH
                                     && !is_rider(&mons[otmp->corpsenm]))))
                         || (likemagic && strchr(magical, otmp->oclass))
                         || (uses_items && searches_for_item(mtmp, otmp))
                         || (likerock && otmp->otyp == BOULDER)
                         || (likegems && otmp->oclass == GEM_CLASS
                             && objects[otmp->otyp].oc_material != MINERAL)
                         || (conceals && !cansee(otmp->ox, otmp->oy))
                         || (ptr == &mons[PM_GELATINOUS_CUBE]
                             && !strchr(indigestion, otmp->oclass)
                             && !(otmp->otyp == CORPSE
                                  && touch_petrifies(&mons[otmp->corpsenm]))))
                        && touch_artifact(otmp, mtmp)) {
                        if (can_carry(mtmp, otmp) > 0
                            && (throws_rocks(ptr) || !sobj_at(BOULDER, xx, yy))
                            && (!is_unicorn(ptr)
                                || objects[otmp->otyp].oc_material == GEMSTONE)
                            /* Don't get stuck circling an Elbereth */
                            && !onscary(xx, yy, mtmp)) {
                            minr = distmin(omx, omy, xx, yy);
                            oomx = min(COLNO - 1, omx + minr);
                            oomy = min(ROWNO - 1, omy + minr);
                            lmx = max(1, omx - minr);
                            lmy = max(0, omy - minr);
                            gx = otmp->ox;
                            gy = otmp->oy;
                            if (gx == omx && gy == omy) {
                                mmoved = MMOVE_DONE; /* actually unnecessary */
                                goto postmov;
                            }
                        }
                    }
                }
            }
        } else if (likegold) {
            /* don't try to pick up anything else, but use the same loop */
            uses_items = 0;
            likegems = likeobjs = likemagic = likerock = conceals = 0;
            goto look_for_obj;
        }

        if (minr < SQSRCHRADIUS && appr == -1) {
            if (distmin(omx, omy, mtmp->mux, mtmp->muy) <= 3) {
                gx = mtmp->mux;
                gy = mtmp->muy;
            } else
                appr = 1;
        }
    }

    /* don't tunnel if hostile and close enough to prefer a weapon */
    if (can_tunnel && needspick(ptr)
        && ((!mtmp->mpeaceful || Conflict)
            && dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= 8))
        can_tunnel = FALSE;

    nix = omx;
    niy = omy;
    flag = mon_allowflags(mtmp);
    {
        int i, j, nx, ny, nearer;
        int jcnt, cnt;
        int ndist, nidist;
        coord *mtrk;
        coord poss[9];

        cnt = mfndpos(mtmp, poss, info, flag);
        if (cnt == 0) {
            if (find_defensive(mtmp, TRUE) && use_defensive(mtmp))
                return MMOVE_DONE;
            return MMOVE_NOMOVES;
        }
        chcnt = 0;
        jcnt = min(MTSZ, cnt - 1);
        chi = -1;
        nidist = dist2(nix, niy, gx, gy);
        /* allow monsters be shortsighted on some levels for balance */
        if (!mtmp->mpeaceful && g.level.flags.shortsighted
            && nidist > (couldsee(nix, niy) ? 144 : 36) && appr == 1)
            appr = 0;
        if (is_unicorn(ptr) && noteleport_level(mtmp)) {
            /* on noteleport levels, perhaps we cannot avoid hero */
            for (i = 0; i < cnt; i++)
                if (!(info[i] & NOTONL))
                    avoid = TRUE;
        }
        better_with_displacing =
            should_displace(mtmp, poss, info, cnt, gx, gy);
        for (i = 0; i < cnt; i++) {
            if (avoid && (info[i] & NOTONL))
                continue;
            nx = poss[i].x;
            ny = poss[i].y;

            if (MON_AT(nx, ny) && (info[i] & ALLOW_MDISP)
                && !(info[i] & ALLOW_M) && !better_with_displacing)
                continue;
            if (appr != 0) {
                mtrk = &mtmp->mtrack[0];
                for (j = 0; j < jcnt; mtrk++, j++)
                    if (nx == mtrk->x && ny == mtrk->y)
                        if (rn2(4 * (cnt - j)))
                            goto nxti;
            }

            nearer = ((ndist = dist2(nx, ny, gx, gy)) < nidist);

            if ((appr == 1 && nearer) || (appr == -1 && !nearer)
                || (!appr && !rn2(++chcnt)) || (mmoved == MMOVE_NOTHING)) {
                nix = nx;
                niy = ny;
                nidist = ndist;
                chi = i;
                mmoved = MMOVE_MOVED;
            }
 nxti:
            ;
        }
    }

    if (mmoved != MMOVE_NOTHING) {
        if (mmoved == MMOVE_MOVED && !u_at(nix, niy) && itsstuck(mtmp))
            return MMOVE_DONE;

        if (mmoved == MMOVE_MOVED && m_digweapon_check(mtmp, nix, niy))
            return MMOVE_DONE;

        /* If ALLOW_U is set, either it's trying to attack you, or it
         * thinks it is.  In either case, attack this spot in preference to
         * all others.
         */
        /* Actually, this whole section of code doesn't work as you'd expect.
         * Most attacks are handled in dochug().  It calls distfleeck(), which
         * among other things sets nearby if the monster is near you--and if
         * nearby is set, we never call m_move unless it is a special case
         * (confused, stun, etc.)  The effect is that this ALLOW_U (and
         * mfndpos) has no effect for normal attacks, though it lets a
         * confused monster attack you by accident.
         */
        if (info[chi] & ALLOW_U) {
            nix = mtmp->mux;
            niy = mtmp->muy;
        }
        if (u_at(nix, niy)) {
            mtmp->mux = u.ux;
            mtmp->muy = u.uy;
            return MMOVE_NOTHING;
        }
        /* The monster may attack another based on 1 of 2 conditions:
         * 1 - It may be confused.
         * 2 - It may mistake the monster for your (displaced) image.
         * Pets get taken care of above and shouldn't reach this code.
         * Conflict gets handled even farther away (movemon()).
         */
        if ((info[chi] & ALLOW_M) || (nix == mtmp->mux && niy == mtmp->muy))
            return m_move_aggress(mtmp, nix, niy);

        if ((info[chi] & ALLOW_MDISP)) {
            struct monst *mtmp2;
            int mstatus;

            mtmp2 = m_at(nix, niy);
            mstatus = mdisplacem(mtmp, mtmp2, FALSE);
            if ((mstatus & MM_AGR_DIED) || (mstatus & MM_DEF_DIED))
                return MMOVE_DIED;
            if (mstatus & MM_HIT)
                return MMOVE_MOVED;
            return MMOVE_DONE;
        }

        if (!m_in_out_region(mtmp, nix, niy))
            return MMOVE_DONE;

        /* move a normal monster; for a long worm, remove_monster() and
           place_monster() only manipulate the head; they leave tail as-is */
        remove_monster(omx, omy);
        place_monster(mtmp, nix, niy);
        /* for a long worm, insert a new segment to reconnect the head
           with the tail; worm_move() keeps the end of the tail if worm
           is scheduled to grow, removes that for move-without-growing */
        if (mtmp->wormno)
            worm_move(mtmp);

        maybe_unhide_at(mtmp->mx, mtmp->my);

        mon_track_add(mtmp, omx, omy);
    } else {
        if (is_unicorn(ptr) && rn2(2) && !tele_restrict(mtmp)) {
            (void) rloc(mtmp, RLOC_MSG);
            return MMOVE_MOVED;
        }
        /* for a long worm, shrink it (by discarding end of tail) when
           it has failed to move */
        if (mtmp->wormno)
            worm_nomove(mtmp);
    }
 postmov:
    if (mmoved == MMOVE_MOVED || mmoved == MMOVE_DONE) {
        boolean canseeit = cansee(mtmp->mx, mtmp->my),
                didseeit = canseeit;

        if (mmoved == MMOVE_MOVED) {
            int trapret;

            /* normal monster move will already have <nix,niy>,
               but pet dog_move() with 'goto postmov' won't */
            nix = mtmp->mx, niy = mtmp->my;
            /* sequencing issue:  when monster movement decides that a
               monster can move to a door location, it moves the monster
               there before dealing with the door rather than after;
               so a vampire/bat that is going to shift to fog cloud and
               pass under the door is already there but transformation
               into fog form--and its message, when in sight--has not
               happened yet; we have to move monster back to previous
               location before performing the vamp_shift() to make the
               message happen at right time, then back to the door again
               [if we did the shift above, before moving the monster,
               we would need to duplicate it in dog_move()...] */
            if (is_vampshifter(mtmp) && !amorphous(mtmp->data)
                && IS_DOOR(levl[nix][niy].typ)
                && ((levl[nix][niy].doormask & (D_LOCKED | D_CLOSED)) != 0)
                && can_fog(mtmp)) {
                if (sawmon) {
                    remove_monster(nix, niy);
                    place_monster(mtmp, omx, omy);
                    newsym(nix, niy), newsym(omx, omy);
                }
                if (vamp_shift(mtmp, &mons[PM_FOG_CLOUD], sawmon)) {
                    ptr = mtmp->data; /* update cached value */
                }
                if (sawmon) {
                    remove_monster(omx, omy);
                    place_monster(mtmp, nix, niy);
                    newsym(omx, omy), newsym(nix, niy);
                }
            }

            newsym(omx, omy); /* update the old position */
            trapret = mintrap(mtmp, NO_TRAP_FLAGS);
            if (trapret == Trap_Killed_Mon || trapret == Trap_Moved_Mon) {
                if (mtmp->mx)
                    newsym(mtmp->mx, mtmp->my);
                return MMOVE_DIED; /* it died */
            }
            ptr = mtmp->data; /* in case mintrap() caused polymorph */

            /* open a door, or crash through it, if 'mtmp' can */
            if (IS_DOOR(levl[mtmp->mx][mtmp->my].typ)
                && !passes_walls(ptr) /* doesn't need to open doors */
                && !can_tunnel) {     /* taken care of below */
                struct rm *here = &levl[mtmp->mx][mtmp->my];
                boolean btrapped = (here->doormask & D_TRAPPED) != 0;
    /* used after monster 'who' has been moved to closed door spot 'where'
       which will now be changed to door state 'what' with map update */
#define UnblockDoor(where,who,what) \
    do {                                                        \
        (where)->doormask = (what);                             \
        newsym((who)->mx, (who)->my);                           \
        unblock_point((who)->mx, (who)->my);                    \
        vision_recalc(0);                                       \
        /* update cached value since it might change */         \
        canseeit = didseeit || cansee((who)->mx, (who)->my);    \
    } while (0)

                /* if mon has MKoT, disarm door trap; no message given */
                if (btrapped && has_magic_key(mtmp)) {
                    /* BUG: this lets a vampire or blob or a doorbuster
                       holding the Key disarm the trap even though it isn't
                       using that Key when squeezing under or smashing the
                       door.  Not significant enough to worry about; perhaps
                       the Key's magic is more powerful for monsters? */
                    here->doormask &= ~D_TRAPPED;
                    btrapped = FALSE;
                }
                if ((here->doormask & (D_LOCKED | D_CLOSED)) != 0
                    && amorphous(ptr)) {
                    if (Verbose(2, m_move1) && canseemon(mtmp))
                        pline("%s %s under the door.", Monnam(mtmp),
                              (ptr == &mons[PM_FOG_CLOUD]
                               || ptr->mlet == S_LIGHT) ? "flows" : "oozes");
                } else if (here->doormask & D_LOCKED && can_unlock) {
                    /* like the vampshift hack above, there are sequencing
                       issues when the monster is moved to the door's spot
                       first then door handling plus feedback comes after */

                    UnblockDoor(here, mtmp, !btrapped ? D_ISOPEN : D_NODOOR);
                    if (btrapped) {
                        if (mb_trapped(mtmp, canseeit))
                            return MMOVE_DIED;
                    } else {
                        if (Verbose(2, m_move2)) {
                            if (canseeit && canspotmon(mtmp))
                                pline("%s unlocks and opens a door.",
                                      Monnam(mtmp));
                            else if (canseeit)
                                You_see("a door unlock and open.");
                            else if (!Deaf)
                                You_hear("a door unlock and open.");
                        }
                    }
                } else if (here->doormask == D_CLOSED && can_open) {
                    UnblockDoor(here, mtmp, !btrapped ? D_ISOPEN : D_NODOOR);
                    if (btrapped) {
                        if (mb_trapped(mtmp, canseeit))
                            return MMOVE_DIED;
                    } else {
                        if (Verbose(2, m_move3)) {
                            if (canseeit && canspotmon(mtmp))
                                pline("%s opens a door.", Monnam(mtmp));
                            else if (canseeit)
                                You_see("a door open.");
                            else if (!Deaf)
                                You_hear("a door open.");
                        }
                    }
                } else if (here->doormask & (D_LOCKED | D_CLOSED)) {
                    /* mfndpos guarantees this must be a doorbuster */
                    unsigned mask;

                    mask = ((btrapped || ((here->doormask & D_LOCKED) != 0
                                          && !rn2(2))) ? D_NODOOR
                            : D_BROKEN);
                    UnblockDoor(here, mtmp, mask);
                    if (btrapped) {
                        if (mb_trapped(mtmp, canseeit))
                            return MMOVE_DIED;
                    } else {
                        if (Verbose(2, m_move4)) {
                            if (canseeit && canspotmon(mtmp))
                                pline("%s smashes down a door.",
                                      Monnam(mtmp));
                            else if (canseeit)
                                You_see("a door crash open.");
                            else if (!Deaf)
                                You_hear("a door crash open.");
                        }
                    }
                    /* if it's a shop door, schedule repair */
                    if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE))
                        add_damage(mtmp->mx, mtmp->my, 0L);
                }
            } else if (levl[mtmp->mx][mtmp->my].typ == IRONBARS) {
                /* 3.6.2: was using may_dig() but that doesn't handle bars;
                   AD_RUST catches rust monsters but metallivorous() is
                   needed for xorns and rock moles */
                if (!(levl[mtmp->mx][mtmp->my].wall_info & W_NONDIGGABLE)
                    && (dmgtype(ptr, AD_RUST) || dmgtype(ptr, AD_CORR)
                        || metallivorous(ptr))) {
                    if (canseemon(mtmp))
                        pline("%s eats through the iron bars.", Monnam(mtmp));
                    dissolve_bars(mtmp->mx, mtmp->my);
                    return MMOVE_DONE;
                } else if (Verbose(2, m_move5) && canseemon(mtmp))
                    Norep("%s %s %s the iron bars.", Monnam(mtmp),
                          /* pluralization fakes verb conjugation */
                          makeplural(locomotion(ptr, "pass")),
                          passes_walls(ptr) ? "through" : "between");
            }

            /* possibly dig */
            if (can_tunnel && may_dig(mtmp->mx, mtmp->my)
                && mdig_tunnel(mtmp))
                return MMOVE_DIED; /* mon died (position already updated) */

            /* set also in domove(), hack.c */
            if (engulfing_u(mtmp)
                && (mtmp->mx != omx || mtmp->my != omy)) {
                /* If the monster moved, then update */
                u.ux0 = u.ux;
                u.uy0 = u.uy;
                u.ux = mtmp->mx;
                u.uy = mtmp->my;
                swallowed(0);
            } else
                newsym(mtmp->mx, mtmp->my);
        }
        if (OBJ_AT(mtmp->mx, mtmp->my) && mtmp->mcanmove) {
            /* recompute the likes tests, in case we polymorphed
             * or if the "likegold" case got taken above */
            if (setlikes) {
                int pctload = (curr_mon_load(mtmp) * 100) / max_mon_load(mtmp);

                /* look for gold or jewels nearby */
                likegold = (likes_gold(ptr) && pctload < 95);
                likegems = (likes_gems(ptr) && pctload < 85);
                uses_items =
                    (!mindless(ptr) && !is_animal(ptr) && pctload < 75);
                likeobjs = (likes_objs(ptr) && pctload < 75);
                likemagic = (likes_magic(ptr) && pctload < 85);
                likerock = (throws_rocks(ptr) && pctload < 50 && !Sokoban);
                conceals = hides_under(ptr);
            }

            /* Maybe a rock mole just ate some metal object */
            if (metallivorous(ptr)) {
                if (meatmetal(mtmp) == 2)
                    return MMOVE_DIED; /* it died */
            }

            if (g_at(mtmp->mx, mtmp->my) && likegold)
                mpickgold(mtmp);

            /* Maybe a cube ate just about anything */
            if (ptr == &mons[PM_GELATINOUS_CUBE]) {
                if ((etmp = meatobj(mtmp)) >= 2)
                    return etmp; /* it died or got forced off the level */
            }
            /* Maybe a purple worm ate a corpse */
            if (ptr == &mons[PM_PURPLE_WORM]
                || ptr == &mons[PM_BABY_PURPLE_WORM]
                || ptr == &mons[PM_PIRANHA]) {
                if ((etmp = meatcorpse(mtmp)) >= 2)
                    return etmp; /* it died or got forced off the level */
            }

            if (!*in_rooms(mtmp->mx, mtmp->my, SHOPBASE) || !rn2(25)) {
                boolean picked = FALSE;

                if (likeobjs)
                    picked |= mpickstuff(mtmp, practical);
                if (likemagic)
                    picked |= mpickstuff(mtmp, magical);
                if (likerock)
                    picked |= mpickstuff(mtmp, boulder_class);
                if (likegems)
                    picked |= mpickstuff(mtmp, gem_class);
                if (uses_items)
                    picked |= mpickstuff(mtmp, (char *) 0);
                if (picked)
                    mmoved = MMOVE_DONE;
            }

            if (mtmp->minvis) {
                newsym(mtmp->mx, mtmp->my);
                if (mtmp->wormno)
                    see_wsegs(mtmp);
            }
        }

        maybe_spin_web(mtmp);

        if (hides_under(ptr) || ptr->mlet == S_EEL) {
            /* Always set--or reset--mundetected if it's already hidden
               (just in case the object it was hiding under went away);
               usually set mundetected unless monster can't move.  */
            if (mtmp->mundetected
                || (!helpless(mtmp) && rn2(5)))
                (void) hideunder(mtmp);
            newsym(mtmp->mx, mtmp->my);
        }
        if (mtmp->isshk) {
            after_shk_move(mtmp);
        }
    }
    return mmoved;
}

/* The part of m_move that deals with a monster attacking another monster (and
 * that monster possibly retaliating).
 * Extracted into its own function so that it can be called with monsters that
 * have special move patterns (shopkeepers, priests, etc) that want to attack
 * other monsters but aren't just roaming freely around the level (so allowing
 * m_move to run fully for them could select an invalid move).
 * x and y are the coordinates mtmp wants to attack.
 * Return values are the same as for m_move, but this function only return 2
 * (mtmp died) or 3 (mtmp made its move).
 */
int
m_move_aggress(struct monst* mtmp, coordxy x, coordxy y)
{
    struct monst *mtmp2;
    int mstatus;

    mtmp2 = m_at(x, y);

    g.notonhead = mtmp2 && (x != mtmp2->mx || y != mtmp2->my);
    /* note: mstatus returns 0 if mtmp2 is nonexistent */
    mstatus = mattackm(mtmp, mtmp2);

    if (mstatus & MM_AGR_DIED) /* aggressor died */
        return MMOVE_DIED;

    if ((mstatus & MM_HIT) && !(mstatus & MM_DEF_DIED) && rn2(4)
        && mtmp2->movement >= NORMAL_SPEED) {
        mtmp2->movement -= NORMAL_SPEED;
        g.notonhead = 0;
        mstatus = mattackm(mtmp2, mtmp); /* return attack */
        if (mstatus & MM_DEF_DIED)
            return MMOVE_DIED;
    }
    return MMOVE_DONE;
}

void
dissolve_bars(coordxy x, coordxy y)
{
    levl[x][y].typ = (Is_special(&u.uz) || *in_rooms(x, y, 0)) ? ROOM : CORR;
    levl[x][y].flags = 0;
    newsym(x, y);
    if (u_at(x, y))
        switch_terrain();
}

boolean
closed_door(coordxy x, coordxy y)
{
    return (boolean) (IS_DOOR(levl[x][y].typ)
                      && (levl[x][y].doormask & (D_LOCKED | D_CLOSED)));
}

boolean
accessible(coordxy x, coordxy y)
{
    int levtyp = levl[x][y].typ;

    /* use underlying terrain in front of closed drawbridge */
    if (levtyp == DRAWBRIDGE_UP)
        levtyp = db_under_typ(levl[x][y].drawbridgemask);

    return (boolean) (ACCESSIBLE(levtyp) && !closed_door(x, y));
}

/* decide where the monster thinks you are standing */
void
set_apparxy(register struct monst* mtmp)
{
    boolean notseen, notthere, gotu;
    int disp;
    coordxy mx = mtmp->mux, my = mtmp->muy;
    long umoney = money_cnt(g.invent);

    /*
     * do cheapest and/or most likely tests first
     */

    /* pet knows your smell; grabber still has hold of you; monsters which
       know where you are don't suddenly forget, if you haven't moved away */
    if (mtmp->mtame || mtmp == u.ustuck || u_at(mx, my)) {
            mtmp->mux = u.ux;
            mtmp->muy = u.uy;
            return;
    }

    notseen = (!mtmp->mcansee || (Invis && !perceives(mtmp->data)));
    notthere = (Displaced && mtmp->data != &mons[PM_DISPLACER_BEAST]);
    /* add cases as required.  eg. Displacement ... */
    if (Underwater) {
        disp = 1;
    } else if (notseen) {
        /* Xorns can smell quantities of valuable metal
           like that in solid gold coins, treat as seen */
        disp = (mtmp->data == &mons[PM_XORN] && umoney) ? 0 : 1;
    } else if (notthere) {
        disp = couldsee(mx, my) ? 2 : 1;
    } else {
        disp = 0;
    }
    if (!disp) {
        mtmp->mux = u.ux;
        mtmp->muy = u.uy;
        return;
    }

    /* without something like the following, invisibility and displacement
       are too powerful */
    gotu = notseen ? !rn2(3) : notthere ? !rn2(4) : FALSE;

    if (!gotu) {
        register int try_cnt = 0;

        do {
            if (++try_cnt > 200) {
                mx = u.ux;
                my = u.uy;
                break; /* punt */
            }
            mx = u.ux - disp + rn2(2 * disp + 1);
            my = u.uy - disp + rn2(2 * disp + 1);
        } while (!isok(mx, my)
                 || (disp != 2 && mx == mtmp->mx && my == mtmp->my)
                 || ((mx != u.ux || my != u.uy) && !passes_walls(mtmp->data)
                     && !(accessible(mx, my)
                          || (closed_door(mx, my)
                              && (can_ooze(mtmp) || can_fog(mtmp)))))
                 || !couldsee(mx, my));
    } else {
        mx = u.ux;
        my = u.uy;
    }

    mtmp->mux = mx;
    mtmp->muy = my;
}

/*
 * mon-to-mon displacement is a deliberate "get out of my way" act,
 * not an accidental bump, so we don't consider mstun or mconf in
 * undesired_disp().
 *
 * We do consider many other things about the target and its
 * location however.
 */
boolean
undesirable_disp(
    struct monst *mtmp, /* barging creature */
    coordxy x,
    coordxy y) /* spot 'mtmp' is considering moving to */
{
    boolean is_pet = (mtmp && mtmp->mtame && !mtmp->isminion);
    struct trap *trap = t_at(x, y);

    if (is_pet) {
        /* Pets avoid a trap if you've seen it usually. */
        if (trap && trap->tseen && rn2(40))
            return TRUE;
        /* Pets avoid cursed locations */
        if (cursed_object_at(x, y))
            return TRUE;

    /* Monsters avoid a trap if they've seen that type before */
    } else if (trap && rn2(40)
               && mon_knows_traps(mtmp, trap->ttyp)) {
        return TRUE;
    }

    /* oversimplification:  creatures that bargethrough can't swap places
       when target monster is in rock or closed door or water (in particular,
       avoid moving to spots where mondied() won't leave a corpse; doesn't
       matter whether barger is capable of moving to such a target spot if
       it were unoccupied) */
    if (!accessible(x, y)
        /* mondied() allows is_pool() as an exception to !accessible(),
           but we'll only do that if 'mtmp' is already at a water location
           so that we don't swap a water critter onto land */
        && !(is_pool(x, y) && is_pool(mtmp->mx, mtmp->my)))
        return TRUE;

    return FALSE;
}

/*
 * Inventory prevents passage under door.
 * Used by can_ooze() and can_fog().
 */
static boolean
stuff_prevents_passage(struct monst* mtmp)
{
    struct obj *chain, *obj;

    if (mtmp == &g.youmonst) {
        chain = g.invent;
    } else {
        chain = mtmp->minvent;
    }
    for (obj = chain; obj; obj = obj->nobj) {
        int typ = obj->otyp;

        if (typ == COIN_CLASS && obj->quan > 100L)
            return TRUE;
        if (obj->oclass != GEM_CLASS && !(typ >= ARROW && typ <= BOOMERANG)
            && !(typ >= DAGGER && typ <= CRYSKNIFE) && typ != SLING
            && !is_cloak(obj) && typ != FEDORA && !is_gloves(obj)
            && typ != LEATHER_JACKET && typ != CREDIT_CARD && !is_shirt(obj)
            && !(typ == CORPSE && verysmall(&mons[obj->corpsenm]))
            && typ != FORTUNE_COOKIE && typ != CANDY_BAR && typ != PANCAKE
            && typ != LEMBAS_WAFER && typ != LUMP_OF_ROYAL_JELLY
            && obj->oclass != AMULET_CLASS && obj->oclass != RING_CLASS
            && obj->oclass != VENOM_CLASS && typ != SACK
            && typ != BAG_OF_HOLDING && typ != BAG_OF_TRICKS
            && !Is_candle(obj) && typ != OILSKIN_SACK && typ != LEASH
            && typ != STETHOSCOPE && typ != BLINDFOLD && typ != TOWEL
            && typ != TIN_WHISTLE && typ != MAGIC_WHISTLE
            && typ != MAGIC_MARKER && typ != TIN_OPENER && typ != SKELETON_KEY
            && typ != LOCK_PICK)
            return TRUE;
        if (Is_container(obj) && obj->cobj)
            return TRUE;
    }
    return FALSE;
}

boolean
can_ooze(struct monst* mtmp)
{
    if (!amorphous(mtmp->data) || stuff_prevents_passage(mtmp))
        return FALSE;
    return TRUE;
}

/* monster can change form into a fog if necessary */
boolean
can_fog(struct monst* mtmp)
{
    if (!(g.mvitals[PM_FOG_CLOUD].mvflags & G_GENOD) && is_vampshifter(mtmp)
        && !Protection_from_shape_changers && !stuff_prevents_passage(mtmp))
        return TRUE;
    return FALSE;
}

static int
vamp_shift(
    struct monst *mon,
    struct permonst *ptr,
    boolean domsg)
{
    int reslt = 0;
    char oldmtype[BUFSZ];
    boolean sawmon = canseemon(mon); /* before shape change */

    /* remember current monster type before shapechange */
    Strcpy(oldmtype, domsg ? noname_monnam(mon, ARTICLE_THE) : "");

    if (mon->data == ptr) {
        /* already right shape */
        reslt = 1;
        domsg = FALSE;
    } else if (is_vampshifter(mon)) {
        reslt = newcham(mon, ptr, NO_NC_FLAGS);
    }

    if (reslt && domsg) {
        /* might have seen vampire/bat/wolf with infravision then be
           unable to see the same creature when it turns into a fog cloud */
        if (canspotmon(mon))
            You("%s %s where %s was.",
                !canseemon(mon) ? "now detect" : "observe",
                noname_monnam(mon, ARTICLE_A), oldmtype);
        else
            You("can no longer %s %s.", sawmon ? "see" : "sense", oldmtype);
        /* this message is given when it turns into a fog cloud
           in order to move under a closed door */
        display_nhwindow(WIN_MESSAGE, FALSE);
    }

    return reslt;
}

/*monmove.c*/

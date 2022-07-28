/* NetHack 3.7	wizard.c	$NHDT-Date: 1646688073 2022/03/07 21:21:13 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.85 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

/* wizard code - inspired by rogue code from Merlyn Leroy (digi-g!brian) */
/*             - heavily modified to give the wiz balls.  (genat!mike)   */
/*             - dewimped and given some maledictions. -3. */
/*             - generalized for 3.1 (mike@bullns.on01.bull.ca) */

#include "hack.h"

static short which_arti(int);
static boolean mon_has_arti(struct monst *, short);
static struct monst *other_mon_has_arti(struct monst *, short);
static struct obj *on_ground(short);
static boolean you_have(int);
static unsigned long target_on(int, struct monst *);
static unsigned long strategy(struct monst *);

/* adding more neutral creatures will tend to reduce the number of monsters
   summoned by nasty(); adding more lawful creatures will reduce the number
   of monsters summoned by lawfuls; adding more chaotic creatures will reduce
   the number of monsters summoned by chaotics; prior to 3.6.1, there were
   only four lawful candidates, so lawful summoners tended to summon more
   (trying to get lawful or neutral but obtaining chaotic instead) than
   their chaotic counterparts */
static NEARDATA const int nasties[] = {
    /* neutral */
    PM_COCKATRICE, PM_ETTIN, PM_STALKER, PM_MINOTAUR,
    PM_OWLBEAR, PM_PURPLE_WORM, PM_XAN, PM_UMBER_HULK,
    PM_XORN, PM_ZRUTY, PM_LEOCROTTA, PM_BALUCHITHERIUM,
    PM_CARNIVOROUS_APE, PM_FIRE_ELEMENTAL, PM_JABBERWOCK,
    PM_IRON_GOLEM, PM_OCHRE_JELLY, PM_GREEN_SLIME,
    PM_DISPLACER_BEAST, PM_GENETIC_ENGINEER,
    /* chaotic */
    PM_BLACK_DRAGON, PM_RED_DRAGON, PM_ARCH_LICH, PM_VAMPIRE_LEADER,
    PM_MASTER_MIND_FLAYER, PM_DISENCHANTER, PM_WINGED_GARGOYLE,
    PM_STORM_GIANT, PM_OLOG_HAI, PM_ELF_NOBLE, PM_ELVEN_MONARCH,
    PM_OGRE_TYRANT, PM_CAPTAIN, PM_GREMLIN,
    /* lawful */
    PM_SILVER_DRAGON, PM_ORANGE_DRAGON, PM_GREEN_DRAGON,
    PM_YELLOW_DRAGON, PM_GUARDIAN_NAGA, PM_FIRE_GIANT,
    PM_ALEAX, PM_COUATL, PM_HORNED_DEVIL, PM_BARBED_DEVIL,
    /* (Archons, titans, ki-rin, and golden nagas are suitably nasty, but
       they're summoners so would aggravate excessive summoning) */
};

static NEARDATA const unsigned wizapp[] = {
    PM_HUMAN,      PM_WATER_DEMON,  PM_VAMPIRE,       PM_RED_DRAGON,
    PM_TROLL,      PM_UMBER_HULK,   PM_XORN,          PM_XAN,
    PM_COCKATRICE, PM_FLOATING_EYE, PM_GUARDIAN_NAGA, PM_TRAPPER,
};

/* If you've found the Amulet, make the Wizard appear after some time */
/* Also, give hints about portal locations, if amulet is worn/wielded -dlc */
void
amulet(void)
{
    struct monst *mtmp;
    struct trap *ttmp;
    struct obj *amu;

#if 0 /* caller takes care of this check */
    if (!u.uhave.amulet)
        return;
#endif
    if ((((amu = uamul) != 0 && amu->otyp == AMULET_OF_YENDOR)
         || ((amu = uwep) != 0 && amu->otyp == AMULET_OF_YENDOR))
        && !rn2(15)) {
        for (ttmp = g.ftrap; ttmp; ttmp = ttmp->ntrap) {
            if (ttmp->ttyp == MAGIC_PORTAL) {
                int du = distu(ttmp->tx, ttmp->ty);
                if (du <= 9)
                    pline("%s hot!", Tobjnam(amu, "feel"));
                else if (du <= 64)
                    pline("%s very warm.", Tobjnam(amu, "feel"));
                else if (du <= 144)
                    pline("%s warm.", Tobjnam(amu, "feel"));
                /* else, the amulet feels normal */
                break;
            }
        }
    }

    if (!g.context.no_of_wizards)
        return;
    /* find Wizard, and wake him if necessary */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (mtmp->iswiz && mtmp->msleeping && !rn2(40)) {
            mtmp->msleeping = 0;
            if (!next2u(mtmp->mx, mtmp->my))
                You(
      "get the creepy feeling that somebody noticed your taking the Amulet.");
            return;
        }
    }
}

int
mon_has_amulet(struct monst *mtmp)
{
    register struct obj *otmp;

    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == AMULET_OF_YENDOR)
            return 1;
    return 0;
}

int
mon_has_special(struct monst *mtmp)
{
    register struct obj *otmp;

    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == AMULET_OF_YENDOR
            || any_quest_artifact(otmp)
            || otmp->otyp == BELL_OF_OPENING
            || otmp->otyp == CANDELABRUM_OF_INVOCATION
            || otmp->otyp == SPE_BOOK_OF_THE_DEAD)
            return 1;
    return 0;
}

/*
 *      New for 3.1  Strategy / Tactics for the wiz, as well as other
 *      monsters that are "after" something (defined via mflag3).
 *
 *      The strategy section decides *what* the monster is going
 *      to attempt, the tactics section implements the decision.
 */
#define STRAT(w, x, y, typ)                            \
    ((unsigned long) (w) | ((unsigned long) (x) << 16) \
     | ((unsigned long) (y) << 8) | (unsigned long) (typ))

#define M_Wants(mask) (mtmp->data->mflags3 & (mask))

static short
which_arti(int mask)
{
    switch (mask) {
    case M3_WANTSAMUL:
        return AMULET_OF_YENDOR;
    case M3_WANTSBELL:
        return BELL_OF_OPENING;
    case M3_WANTSCAND:
        return CANDELABRUM_OF_INVOCATION;
    case M3_WANTSBOOK:
        return SPE_BOOK_OF_THE_DEAD;
    default:
        break; /* 0 signifies quest artifact */
    }
    return 0;
}

/*
 *      If "otyp" is zero, it triggers a check for the quest_artifact,
 *      since bell, book, candle, and amulet are all objects, not really
 *      artifacts right now.  [MRS]
 */
static boolean
mon_has_arti(struct monst *mtmp, short otyp)
{
    register struct obj *otmp;

    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
        if (otyp) {
            if (otmp->otyp == otyp)
                return 1;
        } else if (any_quest_artifact(otmp))
            return 1;
    }
    return 0;
}

static struct monst *
other_mon_has_arti(struct monst *mtmp, short otyp)
{
    register struct monst *mtmp2;

    for (mtmp2 = fmon; mtmp2; mtmp2 = mtmp2->nmon)
        /* no need for !DEADMONSTER check here since they have no inventory */
        if (mtmp2 != mtmp)
            if (mon_has_arti(mtmp2, otyp))
                return mtmp2;

    return (struct monst *) 0;
}

static struct obj *
on_ground(short otyp)
{
    register struct obj *otmp;

    for (otmp = fobj; otmp; otmp = otmp->nobj)
        if (otyp) {
            if (otmp->otyp == otyp)
                return otmp;
        } else if (any_quest_artifact(otmp))
            return otmp;
    return (struct obj *) 0;
}

static boolean
you_have(int mask)
{
    switch (mask) {
    case M3_WANTSAMUL:
        return (boolean) u.uhave.amulet;
    case M3_WANTSBELL:
        return (boolean) u.uhave.bell;
    case M3_WANTSCAND:
        return (boolean) u.uhave.menorah;
    case M3_WANTSBOOK:
        return (boolean) u.uhave.book;
    case M3_WANTSARTI:
        return (boolean) u.uhave.questart;
    default:
        break;
    }
    return 0;
}

static unsigned long
target_on(int mask, struct monst *mtmp)
{
    register short otyp;
    register struct obj *otmp;
    register struct monst *mtmp2;

    if (!M_Wants(mask))
        return (unsigned long) STRAT_NONE;

    otyp = which_arti(mask);
    if (!mon_has_arti(mtmp, otyp)) {
        if (you_have(mask))
            return STRAT(STRAT_PLAYER, u.ux, u.uy, mask);
        else if ((otmp = on_ground(otyp)))
            return STRAT(STRAT_GROUND, otmp->ox, otmp->oy, mask);
        else if ((mtmp2 = other_mon_has_arti(mtmp, otyp)) != 0
                 /* when seeking the Amulet, avoid targetting the Wizard
                    or temple priests (to protect Moloch's high priest) */
                 && (otyp != AMULET_OF_YENDOR
                     || (!mtmp2->iswiz && !inhistemple(mtmp2))))
            return STRAT(STRAT_MONSTR, mtmp2->mx, mtmp2->my, mask);
    }
    return (unsigned long) STRAT_NONE;
}

static unsigned long
strategy(struct monst *mtmp)
{
    unsigned long strat, dstrat;

    if (!is_covetous(mtmp->data)
        /* perhaps a shopkeeper has been polymorphed into a master
           lich; we don't want it teleporting to the stairs to heal
           because that will leave its shop untended */
        || (mtmp->isshk && inhishop(mtmp))
        /* likewise for temple priests */
        || (mtmp->ispriest && inhistemple(mtmp)))
        return (unsigned long) STRAT_NONE;

    switch ((mtmp->mhp * 3) / mtmp->mhpmax) { /* 0-3 */

    default:
    case 0: /* panic time - mtmp is almost snuffed */
        return (unsigned long) STRAT_HEAL;

    case 1: /* the wiz is less cautious */
        if (mtmp->data != &mons[PM_WIZARD_OF_YENDOR])
            return (unsigned long) STRAT_HEAL;
    /* else fall through */

    case 2:
        dstrat = STRAT_HEAL;
        break;

    case 3:
        dstrat = STRAT_NONE;
        break;
    }

    if (g.context.made_amulet)
        if ((strat = target_on(M3_WANTSAMUL, mtmp)) != STRAT_NONE)
            return strat;

    if (u.uevent.invoked) { /* priorities change once gate opened */
        if ((strat = target_on(M3_WANTSARTI, mtmp)) != STRAT_NONE)
            return strat;
        if ((strat = target_on(M3_WANTSBOOK, mtmp)) != STRAT_NONE)
            return strat;
        if ((strat = target_on(M3_WANTSBELL, mtmp)) != STRAT_NONE)
            return strat;
        if ((strat = target_on(M3_WANTSCAND, mtmp)) != STRAT_NONE)
            return strat;
    } else {
        if ((strat = target_on(M3_WANTSBOOK, mtmp)) != STRAT_NONE)
            return strat;
        if ((strat = target_on(M3_WANTSBELL, mtmp)) != STRAT_NONE)
            return strat;
        if ((strat = target_on(M3_WANTSCAND, mtmp)) != STRAT_NONE)
            return strat;
        if ((strat = target_on(M3_WANTSARTI, mtmp)) != STRAT_NONE)
            return strat;
    }
    return dstrat;
}

/* pick a destination for a covetous monster to flee to so that it can
   heal or for guardians (Kops) to congregate at to block hero's progress */
void
choose_stairs(
    coordxy *sx, coordxy *sy, /* output; left as-is if no spot found */
    boolean dir) /* True: forward, False: backtrack (usually up) */
{
    stairway *stway;
    boolean stdir = builds_up(&u.uz) ? dir : !dir;

    /* look for stairs in direction 'stdir' (True: up, False: down) */
    stway = stairway_find_type_dir(FALSE, stdir);
    if (!stway) {
        /* no stairs; look for ladder it that direction */
        stway = stairway_find_type_dir(TRUE, stdir);
        if (!stway) {
            /* no ladder either; look for branch stairs or ladder in any
               direction */
            for (stway = g.stairs; stway; stway = stway->next)
                if (stway->tolev.dnum != u.uz.dnum)
                    break;
            /* if no branch stairs/ladder, check for regular stairs in
               opposite direction, then for regular ladder if necessary */
            if (!stway) {
                stway = stairway_find_type_dir(FALSE, !stdir);
                if (!stway)
                    stway = stairway_find_type_dir(TRUE, !stdir);
            }
        }
        /* [note: 'stway' could still be Null if the only access to this
           level is via magic portal] */
    }

    if (stway)
        *sx = stway->sx, *sy = stway->sy;
}

DISABLE_WARNING_UNREACHABLE_CODE

int
tactics(struct monst *mtmp)
{
    unsigned long strat = strategy(mtmp);
    coordxy sx = 0, sy = 0, mx, my;

    mtmp->mstrategy =
        (mtmp->mstrategy & (STRAT_WAITMASK | STRAT_APPEARMSG)) | strat;

    switch (strat) {
    case STRAT_HEAL: /* hide and recover */
        mx = mtmp->mx, my = mtmp->my;
        /* if wounded, hole up on or near the stairs (to block them) */
        choose_stairs(&sx, &sy, (mtmp->m_id % 2));
        mtmp->mavenge = 1; /* covetous monsters attack while fleeing */
        if (In_W_tower(mx, my, &u.uz)
            || (mtmp->iswiz && !sx && !mon_has_amulet(mtmp))) {
            if (!rn2(3 + mtmp->mhp / 10))
                (void) rloc(mtmp, RLOC_MSG);
        } else if (sx && (mx != sx || my != sy)) {
            if (!mnearto(mtmp, sx, sy, TRUE, RLOC_MSG)) {
                /* couldn't move to the target spot for some reason,
                   so stay where we are (don't actually need rloc_to()
                   because mtmp is still on the map at <mx,my>... */
                rloc_to(mtmp, mx, my);
                return 0;
            }
            mx = mtmp->mx, my = mtmp->my; /* update cached location */
        }
        /* if you're not around, cast healing spells */
        if (distu(mx, my) > (BOLT_LIM * BOLT_LIM))
            if (mtmp->mhp <= mtmp->mhpmax - 8) {
                mtmp->mhp += rnd(8);
                return 1;
            }
        /*FALLTHRU*/

    case STRAT_NONE: /* harass */
        if (!rn2(!mtmp->mflee ? 5 : 33))
            mnexto(mtmp, RLOC_MSG);
        return 0;

    default: /* kill, maim, pillage! */
    {
        long where = (strat & STRAT_STRATMASK);
        coordxy tx = STRAT_GOALX(strat), ty = STRAT_GOALY(strat);
        int targ = (int) (strat & STRAT_GOAL);
        struct obj *otmp;

        if (!targ) { /* simply wants you to close */
            return 0;
        }
        if (u_at(tx, ty) || where == STRAT_PLAYER) {
            /* player is standing on it (or has it) */
            mnexto(mtmp, RLOC_MSG);
            return 0;
        }
        if (where == STRAT_GROUND) {
            if (!MON_AT(tx, ty) || (mtmp->mx == tx && mtmp->my == ty)) {
                /* teleport to it and pick it up */
                rloc_to(mtmp, tx, ty); /* clean old pos */

                if ((otmp = on_ground(which_arti(targ))) != 0) {
                    if (cansee(mtmp->mx, mtmp->my))
                        pline("%s picks up %s.", Monnam(mtmp),
                              distant_name(otmp, doname));
                    obj_extract_self(otmp);
                    (void) mpickobj(mtmp, otmp);
                    return 1;
                } else
                    return 0;
            } else {
                /* a monster is standing on it - cause some trouble */
                if (!rn2(5))
                    mnexto(mtmp, RLOC_MSG);
                return 0;
            }
        } else { /* a monster has it - 'port beside it. */
            mx = mtmp->mx, my = mtmp->my;
            if (!mnearto(mtmp, tx, ty, FALSE, RLOC_MSG))
                rloc_to(mtmp, mx, my); /* no room? stay put */
            return 0;
        }
    } /* default case */
    } /* switch */
    /*NOTREACHED*/
    return 0;
}

RESTORE_WARNINGS

/* are there any monsters mon could aggravate? */
boolean
has_aggravatables(struct monst *mon)
{
    struct monst *mtmp;
    boolean in_w_tower = In_W_tower(mon->mx, mon->my, &u.uz);

    if (in_w_tower != In_W_tower(u.ux, u.uy, &u.uz))
        return FALSE;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (in_w_tower != In_W_tower(mtmp->mx, mtmp->my, &u.uz))
            continue;
        if ((mtmp->mstrategy & STRAT_WAITFORU) != 0
            || helpless(mtmp))
            return TRUE;
    }
    return FALSE;
}

void
aggravate(void)
{
    register struct monst *mtmp;
    boolean in_w_tower = In_W_tower(u.ux, u.uy, &u.uz);

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (in_w_tower != In_W_tower(mtmp->mx, mtmp->my, &u.uz))
            continue;
        mtmp->mstrategy &= ~(STRAT_WAITFORU | STRAT_APPEARMSG);
        mtmp->msleeping = 0;
        if (!mtmp->mcanmove && !rn2(5)) {
            mtmp->mfrozen = 0;
            mtmp->mcanmove = 1;
        }
    }
}

/* "Double Trouble" spell cast by the Wizard; caller is responsible for
   only casting this when there is currently one wizard in existence;
   the clone can't use it unless/until its creator has been killed off */
void
clonewiz(void)
{
    register struct monst *mtmp2;

    if ((mtmp2 = makemon(&mons[PM_WIZARD_OF_YENDOR], u.ux, u.uy, MM_NOWAIT))
        != 0) {
        mtmp2->msleeping = mtmp2->mtame = mtmp2->mpeaceful = 0;
        if (!u.uhave.amulet && rn2(2)) { /* give clone a fake */
            (void) add_to_minv(mtmp2,
                               mksobj(FAKE_AMULET_OF_YENDOR, TRUE, FALSE));
        }
        mtmp2->m_ap_type = M_AP_MONSTER;
        mtmp2->mappearance = wizapp[rn2(SIZE(wizapp))];
        newsym(mtmp2->mx, mtmp2->my);
    }
}

/* also used by newcham() */
int
pick_nasty(int difcap) /* if non-zero, try to make difficulty be lower
                          than this */
{
    int alt, res = nasties[rn2(SIZE(nasties))];

    /* To do?  Possibly should filter for appropriate forms when
     * in the elemental planes or surrounded by water or lava.
     *
     * We want monsters represented by uppercase on rogue level,
     * but we don't try very hard.
     */
    if (Is_rogue_level(&u.uz)
        && !('A' <= mons[res].mlet && mons[res].mlet <= 'Z'))
        res = nasties[rn2(SIZE(nasties))];

    /* if genocided or too difficult or out of place, try a substitute
       when a suitable one exists
           arch-lich -> master lich,
           master mind flayer -> mind flayer,
       but the substitutes are likely to be genocided too */
    alt = res;
    if ((g.mvitals[res].mvflags & G_GENOD) != 0
        || (difcap > 0 && mons[res].difficulty >= difcap)
         /* note: nasty() -> makemon() ignores G_HELL|G_NOHELL;
            arch-lich and master lich are both flagged as hell-only;
            this filtering demotes arch-lich to master lich when
            outside of Gehennom (unless the latter has been genocided) */
        || (mons[res].geno & (Inhell ? G_NOHELL : G_HELL)) != 0)
        alt = big_to_little(res);
    if (alt != res && (g.mvitals[alt].mvflags & G_GENOD) == 0) {
        const char *mnam = mons[alt].pmnames[NEUTRAL],
                   *lastspace = rindex(mnam, ' ');

        /* only non-juveniles can become alternate choice */
        if (strncmp(mnam, "baby ", 5)
            && (!lastspace
                || (strcmp(lastspace, " hatchling")
                    && strcmp(lastspace, " pup")
                    && strcmp(lastspace, " cub"))))
            res = alt;
    }

    return res;
}

/* create some nasty monsters, aligned with the caster or neutral; chaotic
   and unaligned are treated as equivalent; if summoner is Null, this is
   for late-game harassment (after the Wizard has been killed at least once
   or the invocation ritual has been performed), in which case we treat
   'summoner' as neutral, since that will produce the greatest number of
   creatures on average (in 3.6.0 and earlier, Null was treated as chaotic);
   returns the number of monsters created */
int
nasty(struct monst *summoner)
{
    struct monst *mtmp;
    coord bypos;
    int i, j, count, census, tmp, makeindex,
        s_cls, m_cls, difcap, trylimit, castalign;

#define MAXNASTIES 10 /* more than this can be created */

    /* some candidates may be created in groups, so simple count
       of non-null makemon() return is inadequate */
    census = monster_census(FALSE);

    if (!rn2(10) && Inhell) {
        /* this might summon a demon prince or lord */
        count = msummon((struct monst *) 0); /* summons like WoY */
    } else {
        count = 0;
        s_cls = summoner ? summoner->data->mlet : 0;
        difcap = summoner ? summoner->data->difficulty : 0; /* spellcasters */
        castalign = summoner ? sgn(summoner->data->maligntyp) : 0;
        tmp = (u.ulevel > 3) ? u.ulevel / 3 : 1;
        /* if we don't have a casting monster, nasties appear around hero,
           otherwise they'll appear around spot summoner thinks she's at */
        bypos.x = u.ux;
        bypos.y = u.uy;
        for (i = rnd(tmp); i > 0 && count < MAXNASTIES; --i) {
            /* Of the 44 nasties[], 10 are lawful, 14 are chaotic,
             * and 20 are neutral.  [These numbers are up date for
             * 3.7.0; the ones in the next paragraph are not....]
             *
             * Neutral caster, used for late-game harrassment,
             * has 18/42 chance to stop the inner loop on each
             * critter, 24/42 chance for another iteration.
             * Lawful caster has 28/42 chance to stop unless the
             * summoner is an angel or demon, in which case the
             * chance is 26/42.
             * Chaotic or unaligned caster has 32/42 chance to
             * stop, so will summon fewer creatures on average.
             *
             * The outer loop potentially gives chaotic/unaligned
             * a chance to even things up since others will hit
             * MAXNASTIES sooner, but its number of iterations is
             * randomized so it won't always do so.
             */
            for (j = 0; j < 20; j++) {
                /* Don't create more spellcasters of the monsters' level or
                 * higher--avoids chain summoners filling up the level.
                 */
                trylimit = 10 + 1; /* 10 tries */
                do {
                    if (!--trylimit)
                        goto nextj; /* break this loop, continue outer one */
                    makeindex = pick_nasty(difcap);
                    m_cls = mons[makeindex].mlet;
                } while ((difcap > 0 && mons[makeindex].difficulty >= difcap
                          && attacktype(&mons[makeindex], AT_MAGC))
                         || (s_cls == S_DEMON && m_cls == S_ANGEL)
                         || (s_cls == S_ANGEL && m_cls == S_DEMON));
                /* do this after picking the monster to place */
                if (summoner && !enexto(&bypos, summoner->mux, summoner->muy,
                                        &mons[makeindex]))
                    continue;
                /* this honors genocide but overrides extinction; it ignores
                   inside-hell-only (G_HELL) & outside-hell-only (G_NOHELL) */
                if ((mtmp = makemon(&mons[makeindex], bypos.x, bypos.y,
                                    NO_MM_FLAGS)) != 0) {
                    mtmp->msleeping = mtmp->mpeaceful = mtmp->mtame = 0;
                    set_malign(mtmp);
                } else {
                    /* random monster to substitute for geno'd selection;
                       unlike direct choice, not forced to be hostile [why?];
                       limit spellcasters to inhibit chain summoning */
                    if ((mtmp = makemon((struct permonst *) 0,
                                        bypos.x, bypos.y,
                                        MM_NOMSG)) != 0) {
                        m_cls = mtmp->data->mlet;
                        if ((difcap > 0 && mtmp->data->difficulty >= difcap
                             && attacktype(mtmp->data, AT_MAGC))
                            || (s_cls == S_DEMON && m_cls == S_ANGEL)
                            || (s_cls == S_ANGEL && m_cls == S_DEMON))
                            mtmp = unmakemon(mtmp, NO_MM_FLAGS); /* Null */
                    }
                }

                if (mtmp) {
                    /* create at most one arch-lich or Archon regardless
                       of who is doing the summoning (note: Archon is
                       not in nasties[] but could be chosen as random
                       replacement for a genocided selection) */
                    if (mtmp->data == &mons[PM_ARCH_LICH]
                        || mtmp->data == &mons[PM_ARCHON]) {
                        tmp = min(mons[PM_ARCHON].difficulty, /* A:26 */
                                  mons[PM_ARCH_LICH].difficulty); /* L:31 */
                        if (!difcap || difcap > tmp)
                            difcap = tmp; /* rest must be lower difficulty */
                    }
                    /* delay first use of spell or breath attack */
                    mtmp->mspec_used = rnd(4);

                    if (++count >= MAXNASTIES
                        || mtmp->data->maligntyp == 0
                        || sgn(mtmp->data->maligntyp) == castalign)
                        break;
                }
 nextj:
                ; /* empty; label must be followed by a statement */
            } /* for j */
        } /* for i */
    }

    if (count)
        count = monster_census(FALSE) - census;
    return count;
}

/* Let's resurrect the wizard, for some unexpected fun. */
void
resurrect(void)
{
    struct monst *mtmp, **mmtmp;
    long elapsed;
    const char *verb;

    if (!g.context.no_of_wizards) {
        /* make a new Wizard */
        verb = "kill";
        mtmp = makemon(&mons[PM_WIZARD_OF_YENDOR], u.ux, u.uy, MM_NOWAIT);
        /* affects experience; he's not coming back from a corpse
           but is subject to repeated killing like a revived corpse */
        if (mtmp) mtmp->mrevived = 1;
    } else {
        /* look for a migrating Wizard */
        verb = "elude";
        mmtmp = &g.migrating_mons;
        while ((mtmp = *mmtmp) != 0) {
            if (mtmp->iswiz
                /* if he has the Amulet, he won't bring it to you */
                && !mon_has_amulet(mtmp)
                && (elapsed = g.moves - mtmp->mlstmv) > 0L) {
                mon_catchup_elapsed_time(mtmp, elapsed);
                if (elapsed >= LARGEST_INT)
                    elapsed = LARGEST_INT - 1;
                elapsed /= 50L;
                if (mtmp->msleeping && rn2((int) elapsed + 1))
                    mtmp->msleeping = 0;
                if (mtmp->mfrozen == 1) /* would unfreeze on next move */
                    mtmp->mfrozen = 0, mtmp->mcanmove = 1;
                if (!helpless(mtmp)) {
                    *mmtmp = mtmp->nmon;
                    mon_arrive(mtmp, -1); /* -1: Wiz_arrive (dog.c) */
                    /* note: there might be a second Wizard; if so,
                       he'll have to wait til the next resurrection */
                    break;
                }
            }
            mmtmp = &mtmp->nmon;
        }
    }

    if (mtmp) {
        mtmp->mtame = mtmp->mpeaceful = 0; /* paranoia */
        set_malign(mtmp);
        if (!Deaf) {
            pline("A voice booms out...");
            verbalize("So thou thought thou couldst %s me, fool.", verb);
        }
    }
}

/* Here, we make trouble for the poor shmuck who actually
   managed to do in the Wizard. */
void
intervene(void)
{
    int which = Is_astralevel(&u.uz) ? rnd(4) : rn2(6);
    /* cases 0 and 5 don't apply on the Astral level */
    switch (which) {
    case 0:
    case 1:
        You_feel("vaguely nervous.");
        break;
    case 2:
        if (!Blind)
            You("notice a %s glow surrounding you.", hcolor(NH_BLACK));
        rndcurse();
        break;
    case 3:
        aggravate();
        break;
    case 4:
        (void) nasty((struct monst *) 0);
        break;
    case 5:
        resurrect();
        break;
    }
}

void
wizdead(void)
{
    g.context.no_of_wizards--;
    if (!u.uevent.udemigod) {
        u.uevent.udemigod = TRUE;
        u.udg_cnt = rn1(250, 50);
    }
}

const char *const random_insult[] = {
    "antic",      "blackguard",   "caitiff",    "chucklehead",
    "coistrel",   "craven",       "cretin",     "cur",
    "dastard",    "demon fodder", "dimwit",     "dolt",
    "fool",       "footpad",      "imbecile",   "knave",
    "maledict",   "miscreant",    "niddering",  "poltroon",
    "rattlepate", "reprobate",    "scapegrace", "varlet",
    "villein", /* (sic.) */
    "wittol",     "worm",         "wretch",
};

const char *const random_malediction[] = {
    "Hell shall soon claim thy remains,", "I chortle at thee, thou pathetic",
    "Prepare to die, thou", "Resistance is useless,",
    "Surrender or die, thou", "There shall be no mercy, thou",
    "Thou shalt repent of thy cunning,", "Thou art as a flea to me,",
    "Thou art doomed,", "Thy fate is sealed,",
    "Verily, thou shalt be one dead"
};

/* Insult or intimidate the player */
void
cuss(struct monst *mtmp)
{
    if (Deaf)
        return;
    if (mtmp->iswiz) {
        if (!rn2(5)) /* typical bad guy action */
            pline("%s laughs fiendishly.", Monnam(mtmp));
        else if (u.uhave.amulet && !rn2(SIZE(random_insult)))
            verbalize("Relinquish the amulet, %s!",
                      random_insult[rn2(SIZE(random_insult))]);
        else if (u.uhp < 5 && !rn2(2)) /* Panic */
            verbalize(rn2(2) ? "Even now thy life force ebbs, %s!"
                             : "Savor thy breath, %s, it be thy last!",
                      random_insult[rn2(SIZE(random_insult))]);
        else if (mtmp->mhp < 5 && !rn2(2)) /* Parthian shot */
            verbalize(rn2(2) ? "I shall return." : "I'll be back.");
        else
            verbalize("%s %s!",
                      random_malediction[rn2(SIZE(random_malediction))],
                      random_insult[rn2(SIZE(random_insult))]);
    } else if (is_lminion(mtmp)
               && !(mtmp->isminion && EMIN(mtmp)->renegade)) {
        com_pager("angel_cuss"); /* TODO: the Hallucination msg */
        /*com_pager(rn2(QTN_ANGELIC - 1 + (Hallucination ? 1 : 0))
          + QT_ANGELIC);*/
    } else {
        if (!rn2(is_minion(mtmp->data) ? 100 : 5))
            pline("%s casts aspersions on your ancestry.", Monnam(mtmp));
        else
            com_pager("demon_cuss");
    }
    wake_nearto(mtmp->mx, mtmp->my, 5 * 5);
}

/*wizard.c*/

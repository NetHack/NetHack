/* NetHack 3.7	fountain.c	$NHDT-Date: 1699582923 2023/11/10 02:22:03 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.100 $ */
/*      Copyright Scott R. Turner, srt@ucla, 10/27/86 */
/* NetHack may be freely redistributed.  See license for details. */

/* Code for drinking from fountains. */

#include "hack.h"

staticfn void dowatersnakes(void);
staticfn void dowaterdemon(void);
staticfn void dowaternymph(void);
staticfn void gush(coordxy, coordxy, genericptr_t) NONNULLARG3;
staticfn void dofindgem(void);
staticfn boolean watchman_warn_fountain(struct monst *) NONNULLARG1;

DISABLE_WARNING_FORMAT_NONLITERAL

/* used when trying to dip in or drink from fountain or sink or pool while
   levitating above it, or when trying to move downwards in that state */
void
floating_above(const char *what)
{
    const char *umsg = "are floating high above the %s.";

    if (u.utrap && (u.utraptype == TT_INFLOOR || u.utraptype == TT_LAVA)) {
        /* when stuck in floor (not possible at fountain or sink location,
           so must be attempting to move down), override the usual message */
        umsg = "are trapped in the %s.";
        what = surface(u.ux, u.uy); /* probably redundant */
    }
    You(umsg, what);
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* Fountain of snakes! */
staticfn void
dowatersnakes(void)
{
    int num = rn1(5, 2);
    struct monst *mtmp;

    if (!(svm.mvitals[PM_WATER_MOCCASIN].mvflags & G_GONE)) {
        if (!Blind) {
            pline("An endless stream of %s pours forth!",
                  Hallucination ? makeplural(rndmonnam(NULL)) : "snakes");
        } else {
            Soundeffect(se_snakes_hissing, 75);
            You_hear("%s hissing!", something);
        }
        while (num-- > 0)
            if ((mtmp = makemon(&mons[PM_WATER_MOCCASIN], u.ux, u.uy,
                                MM_NOMSG)) != 0
                && t_at(mtmp->mx, mtmp->my))
                (void) mintrap(mtmp, NO_TRAP_FLAGS);
    } else {
        Soundeffect(se_furious_bubbling, 20);
        pline_The("fountain bubbles furiously for a moment, then calms.");
    }
}

/* Water demon */
staticfn void
dowaterdemon(void)
{
    struct monst *mtmp;

    if (!(svm.mvitals[PM_WATER_DEMON].mvflags & G_GONE)) {
        if ((mtmp = makemon(&mons[PM_WATER_DEMON], u.ux, u.uy,
                            MM_NOMSG)) != 0) {
            if (!Blind)
                You("unleash %s!", a_monnam(mtmp));
            else
                You_feel("the presence of evil.");

            /* Give those on low levels a (slightly) better chance of survival
             */
            if (rnd(100) > (80 + level_difficulty())) {
                pline("Grateful for %s release, %s grants you a wish!",
                      mhis(mtmp), mhe(mtmp));
                /* give a wish and discard the monster (mtmp set to null) */
                mongrantswish(&mtmp);
            } else if (t_at(mtmp->mx, mtmp->my))
                (void) mintrap(mtmp, NO_TRAP_FLAGS);
        }
    } else {
        Soundeffect(se_furious_bubbling, 20);
        pline_The("fountain bubbles furiously for a moment, then calms.");
    }
}

/* Water Nymph */
staticfn void
dowaternymph(void)
{
    struct monst *mtmp;

    if (!(svm.mvitals[PM_WATER_NYMPH].mvflags & G_GONE)
        && (mtmp = makemon(&mons[PM_WATER_NYMPH], u.ux, u.uy,
                           MM_NOMSG)) != 0) {
        if (!Blind)
            You("attract %s!", a_monnam(mtmp));
        else
            You_hear("a seductive voice.");
        mtmp->msleeping = 0;
        if (t_at(mtmp->mx, mtmp->my))
            (void) mintrap(mtmp, NO_TRAP_FLAGS);
    } else if (!Blind) {
        Soundeffect(se_bubble_rising, 50);
        Soundeffect(se_loud_pop, 50);
        pline("A large bubble rises to the surface and pops.");
    } else {
        Soundeffect(se_loud_pop, 50);
        You_hear("a loud pop.");
    }
}

/* Gushing forth along LOS from (u.ux, u.uy) */
void
dogushforth(int drinking)
{
    int madepool = 0;

    do_clear_area(u.ux, u.uy, 7, gush, (genericptr_t) &madepool);
    if (!madepool) {
        if (drinking)
            Your("thirst is quenched.");
        else
            pline("Water sprays all over you.");
    }
}

staticfn void
gush(coordxy x, coordxy y, genericptr_t poolcnt)
{
    struct monst *mtmp;
    struct trap *ttmp;

    if (((x + y) % 2) || u_at(x, y)
        || (rn2(1 + distmin(u.ux, u.uy, x, y))) || (levl[x][y].typ != ROOM)
        || (sobj_at(BOULDER, x, y)) || nexttodoor(x, y))
        return;

    if ((ttmp = t_at(x, y)) != 0 && !delfloortrap(ttmp))
        return;

    if (!((*(int *) poolcnt)++))
        pline("Water gushes forth from the overflowing fountain!");

    /* Put a pool at x, y */
    set_levltyp(x, y, POOL);
    levl[x][y].flags = 0;
    /* No kelp! */
    del_engr_at(x, y);
    water_damage_chain(svl.level.objects[x][y], TRUE);

    if ((mtmp = m_at(x, y)) != 0)
        (void) minliquid(mtmp);
    else
        newsym(x, y);
}

/* Find a gem in the sparkling waters. */
staticfn void
dofindgem(void)
{
    if (!Blind)
        You("spot a gem in the sparkling waters!");
    else
        You_feel("a gem here!");
    (void) mksobj_at(rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE - 1), u.ux, u.uy,
                     FALSE, FALSE);
    SET_FOUNTAIN_LOOTED(u.ux, u.uy);
    newsym(u.ux, u.uy);
    exercise(A_WIS, TRUE); /* a discovery! */
}

staticfn boolean
watchman_warn_fountain(struct monst *mtmp)
{
    if (is_watch(mtmp->data) && couldsee(mtmp->mx, mtmp->my)
        && mtmp->mpeaceful) {
        if (!Deaf) {
            pline("%s yells:", Amonnam(mtmp));
            verbalize("Hey, stop using that fountain!");
        } else {
            pline("%s earnestly %s %s %s!",
                  Amonnam(mtmp),
                  nolimbs(mtmp->data) ? "shakes" : "waves",
                  mhis(mtmp),
                  nolimbs(mtmp->data)
                  ? mbodypart(mtmp, HEAD)
                  : makeplural(mbodypart(mtmp, ARM)));
        }
        return TRUE;
    }
    return FALSE;
}

void
dryup(coordxy x, coordxy y, boolean isyou)
{
    if (IS_FOUNTAIN(levl[x][y].typ)
        && (!rn2(3) || FOUNTAIN_IS_WARNED(x, y))) {
        if (isyou && in_town(x, y) && !FOUNTAIN_IS_WARNED(x, y)) {
            struct monst *mtmp;

            SET_FOUNTAIN_WARNED(x, y);
            /* Warn about future fountain use. */
            mtmp = get_iter_mons(watchman_warn_fountain);
            /* You can see or hear this effect */
            if (!mtmp)
                pline_The("flow reduces to a trickle.");
            return;
        }
        if (isyou && wizard) {
            if (y_n("Dry up fountain?") == 'n')
                return;
        }
        /* FIXME: sight-blocking clouds should use block_point() when
           being created and unblock_point() when going away, then this
           glyph hackery wouldn't be necessary */
        if (cansee(x, y)) {
            int glyph = glyph_at(x, y);

            if (!glyph_is_cmap(glyph) || glyph_to_cmap(glyph) != S_cloud)
                pline_The("fountain dries up!");
        }
        /* replace the fountain with ordinary floor */
        set_levltyp(x, y, ROOM); /* updates level.flags.nfountains */
        levl[x][y].flags = 0;
        levl[x][y].blessedftn = 0;
        /* The location is seen if the hero/monster is invisible
           or felt if the hero is blind. */
        newsym(x, y);
        if (isyou && in_town(x, y))
            (void) angry_guards(FALSE);
    }
}

/* quaff from a fountain when standing on its location */
void
drinkfountain(void)
{
    /* What happens when you drink from a fountain? */
    boolean mgkftn = (levl[u.ux][u.uy].blessedftn == 1);
    int fate = rnd(30);

    if (Levitation) {
        floating_above("fountain");
        return;
    }

    if (mgkftn && u.uluck >= 0 && fate >= 10) {
        int i, ii, littleluck = (u.uluck < 4);

        pline("Wow!  This makes you feel great!");
        /* blessed restore ability */
        for (ii = 0; ii < A_MAX; ii++)
            if (ABASE(ii) < AMAX(ii)) {
                ABASE(ii) = AMAX(ii);
                disp.botl = TRUE;
            }
        /* gain ability, blessed if "natural" luck is high */
        i = rn2(A_MAX); /* start at a random attribute */
        for (ii = 0; ii < A_MAX; ii++) {
            if (adjattrib(i, 1, littleluck ? -1 : 0) && littleluck)
                break;
            if (++i >= A_MAX)
                i = 0;
        }
        display_nhwindow(WIN_MESSAGE, FALSE);
        pline("A wisp of vapor escapes the fountain...");
        exercise(A_WIS, TRUE);
        levl[u.ux][u.uy].blessedftn = 0;
        return;
    }

    if (fate < 10) {
        pline_The("cool draught refreshes you.");
        u.uhunger += rnd(10); /* don't choke on water */
        newuhs(FALSE);
        if (mgkftn)
            return;
    } else {
        switch (fate) {
        case 19: /* Self-knowledge */
            You_feel("self-knowledgeable...");
            display_nhwindow(WIN_MESSAGE, FALSE);
            enlightenment(MAGICENLIGHTENMENT, ENL_GAMEINPROGRESS);
            exercise(A_WIS, TRUE);
            pline_The("feeling subsides.");
            break;
        case 20: /* Foul water */
            pline_The("water is foul!  You gag and vomit.");
            morehungry(rn1(20, 11));
            vomit();
            break;
        case 21: /* Poisonous */
            pline_The("water is contaminated!");
            if (Poison_resistance) {
                pline("Perhaps it is runoff from the nearby %s farm.",
                      fruitname(FALSE));
                losehp(rnd(4), "unrefrigerated sip of juice", KILLED_BY_AN);
                break;
            }
            poison_strdmg(rn1(4, 3), rnd(10), "contaminated water",
                          KILLED_BY);
            exercise(A_CON, FALSE);
            break;
        case 22: /* Fountain of snakes! */
            dowatersnakes();
            break;
        case 23: /* Water demon */
            dowaterdemon();
            break;
        case 24: { /* Maybe curse some items */
            struct obj *obj;
            int buc_changed = 0;

            pline("This water's no good!");
            morehungry(rn1(20, 11));
            exercise(A_CON, FALSE);
            /* this is more severe than rndcurse() */
            for (obj = gi.invent; obj; obj = obj->nobj)
                if (obj->oclass != COIN_CLASS && !obj->cursed && !rn2(5)) {
                    curse(obj);
                    ++buc_changed;
                }
            if (buc_changed)
                update_inventory();
            break;
        }
        case 25: /* See invisible */
            if (Blind) {
                if (Invisible) {
                    You("feel transparent.");
                } else {
                    You("feel very self-conscious.");
                    pline("Then it passes.");
                }
            } else {
                You_see("an image of someone stalking you.");
                pline("But it disappears.");
            }
            HSee_invisible |= FROMOUTSIDE;
            newsym(u.ux, u.uy);
            exercise(A_WIS, TRUE);
            break;
        case 26: /* See Monsters */
            if (monster_detect((struct obj *) 0, 0))
                pline_The("%s tastes like nothing.", hliquid("water"));
            exercise(A_WIS, TRUE);
            break;
        case 27: /* Find a gem in the sparkling waters. */
            if (!FOUNTAIN_IS_LOOTED(u.ux, u.uy)) {
                dofindgem();
                break;
            }
            /*FALLTHRU*/
        case 28: /* Water Nymph */
            dowaternymph();
            break;
        case 29: /* Scare */
        {
            struct monst *mtmp;

            pline("This %s gives you bad breath!",
                  hliquid("water"));
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                monflee(mtmp, 0, FALSE, FALSE);
            }
            break;
        }
        case 30: /* Gushing forth in this room */
            dogushforth(TRUE);
            break;
        default:
            pline("This tepid %s is tasteless.",
                  hliquid("water"));
            break;
        }
    }
    dryup(u.ux, u.uy, TRUE);
}

/* dip an object into a fountain when standing on its location */
void
dipfountain(struct obj *obj)
{
    int er = ER_NOTHING;
    boolean is_hands = (obj == &hands_obj);

    if (Levitation) {
        floating_above("fountain");
        return;
    }

    if (obj->otyp == LONG_SWORD && u.ulevel >= 5
        && !rn2(Role_if(PM_KNIGHT) ? 6 : 30)
        /* once upon a time it was possible to poly N daggers into N swords */
        && obj->quan == 1L && !obj->oartifact
        && !exist_artifact(LONG_SWORD, artiname(ART_EXCALIBUR))) {
        static const char lady[] = "Lady of the Lake";

        if (u.ualign.type != A_LAWFUL) {
            /* Ha!  Trying to cheat her. */
            pline("A freezing mist rises from the %s"
                  " and envelopes the sword.",
                  hliquid("water"));
            pline_The("fountain disappears!");
            curse(obj);
            if (obj->spe > -6 && !rn2(3))
                obj->spe--;
            obj->oerodeproof = FALSE;
            exercise(A_WIS, FALSE);
            livelog_printf(LL_ARTIFACT,
                           "was denied %s!  The %s has deemed %s unworthy",
                           artiname(ART_EXCALIBUR), lady, uhim());
        } else {
            /* The lady of the lake acts! - Eric Backus */
            /* Be *REAL* nice */
            pline(
              "From the murky depths, a hand reaches up to bless the sword.");
            pline("As the hand retreats, the fountain disappears!");
            obj = oname(obj, artiname(ART_EXCALIBUR),
                        ONAME_VIA_DIP | ONAME_KNOW_ARTI);
            discover_artifact(ART_EXCALIBUR);
            bless(obj);
            obj->oeroded = obj->oeroded2 = 0;
            obj->oerodeproof = TRUE;
            exercise(A_WIS, TRUE);
            livelog_printf(LL_ARTIFACT, "was given %s by the %s",
                           artiname(ART_EXCALIBUR), lady);
        }
        update_inventory();
        set_levltyp(u.ux, u.uy, ROOM); /* updates level.flags.nfountains */
        levl[u.ux][u.uy].flags = 0;
        newsym(u.ux, u.uy);
        if (in_town(u.ux, u.uy))
            (void) angry_guards(FALSE);
        return;
    } else if (is_hands || obj == uarmg) {
        er = wash_hands();
    } else {
        er = water_damage(obj, NULL, TRUE);
    }

    if (er == ER_DESTROYED || (er != ER_NOTHING && !rn2(2))) {
        return; /* no further effect */
    }

    switch (rnd(30)) {
    case 16: /* Curse the item */
        if (!is_hands && obj->oclass != COIN_CLASS && !obj->cursed) {
            curse(obj);
        }
        break;
    case 17:
    case 18:
    case 19:
    case 20: /* Uncurse the item */
        if (!is_hands && obj->cursed) {
            if (!Blind)
                pline_The("%s glows for a moment.", hliquid("water"));
            uncurse(obj);
        } else {
            pline("A feeling of loss comes over you.");
        }
        break;
    case 21: /* Water Demon */
        dowaterdemon();
        break;
    case 22: /* Water Nymph */
        dowaternymph();
        break;
    case 23: /* an Endless Stream of Snakes */
        dowatersnakes();
        break;
    case 24: /* Find a gem */
        if (!FOUNTAIN_IS_LOOTED(u.ux, u.uy)) {
            dofindgem();
            break;
        }
        /*FALLTHRU*/
    case 25: /* Water gushes forth */
        dogushforth(FALSE);
        break;
    case 26: /* Strange feeling */
        pline("A strange tingling runs up your %s.", body_part(ARM));
        break;
    case 27: /* Strange feeling */
        You_feel("a sudden chill.");
        break;
    case 28: /* Strange feeling */
        pline("An urge to take a bath overwhelms you.");
        {
            long money = money_cnt(gi.invent);
            struct obj *otmp;

            if (money > 10) {
                /* Amount to lose.  Might get rounded up as fountains don't
                 * pay change... */
                money = somegold(money) / 10;
                for (otmp = gi.invent; otmp && money > 0; otmp = otmp->nobj)
                    if (otmp->oclass == COIN_CLASS) {
                        int denomination = objects[otmp->otyp].oc_cost;
                        long coin_loss =
                            (money + denomination - 1) / denomination;
                        coin_loss = min(coin_loss, otmp->quan);
                        otmp->quan -= coin_loss;
                        money -= coin_loss * denomination;
                        if (!otmp->quan)
                            delobj(otmp);
                    }
                You("lost some of your gold in the fountain!");
                CLEAR_FOUNTAIN_LOOTED(u.ux, u.uy);
                exercise(A_WIS, FALSE);
            }
        }
        break;
    case 29: /* You see coins */
        /* We make fountains have more coins the closer you are to the
         * surface.  After all, there will have been more people going
         * by.  Just like a shopping mall!  Chris Woodbury  */

        if (FOUNTAIN_IS_LOOTED(u.ux, u.uy))
            break;
        SET_FOUNTAIN_LOOTED(u.ux, u.uy);
        (void) mkgold((long) (rnd((dunlevs_in_dungeon(&u.uz) - dunlev(&u.uz)
                                   + 1) * 2) + 5),
                      u.ux, u.uy);
        if (!Blind)
            pline("Far below you, you see coins glistening in the %s.",
                  hliquid("water"));
        exercise(A_WIS, TRUE);
        newsym(u.ux, u.uy);
        break;
    default:
        if (er == ER_NOTHING)
            pline1(nothing_seems_to_happen);
        break;
    }
    update_inventory();
    dryup(u.ux, u.uy, TRUE);
}

/* dipping '-' in fountain, pool, or sink */
int
wash_hands(void)
{
    const char *hands = makeplural(body_part(HAND));
    int res = ER_NOTHING;
    boolean was_glib = !!Glib;

    You("wash your %s%s in the %s.", uarmg ? "gloved " : "", hands,
        hliquid("water"));
    if (Glib) {
        make_glib(0);
        Your("%s are no longer slippery.", fingers_or_gloves(TRUE));
    }
    if (uarmg)
        res = water_damage(uarmg, (const char *) 0, TRUE);
    /* not what ER_GREASED is for, but the checks in dipfountain just
       compare the result to ER_DESTROYED and ER_NOTHING, so it works */
    if (was_glib && res == ER_NOTHING)
        res = ER_GREASED;
    return res;
}

/* convert a sink into a fountain */
void
breaksink(coordxy x, coordxy y)
{
    if (cansee(x, y) || u_at(x, y))
        pline_The("pipes break!  Water spurts out!");
    /* updates level.flags.nsinks and level.flags.nfountains */
    set_levltyp(x, y, FOUNTAIN);
    levl[x][y].looted = 0;
    levl[x][y].blessedftn = 0;
    SET_FOUNTAIN_LOOTED(x, y);
    newsym(x, y);
}

/* quaff from a sink while standing on its location */
void
drinksink(void)
{
    struct obj *otmp;
    struct monst *mtmp;

    if (Levitation) {
        floating_above("sink");
        return;
    }
    switch (rn2(20)) {
    case 0:
        You("take a sip of very cold %s.", hliquid("water"));
        break;
    case 1:
        You("take a sip of very warm %s.", hliquid("water"));
        break;
    case 2:
        You("take a sip of scalding hot %s.", hliquid("water"));
        if (Fire_resistance) {
            pline("It seems quite tasty.");
            monstseesu(M_SEEN_FIRE);
        } else {
            losehp(rnd(6), "sipping boiling water", KILLED_BY);
            monstunseesu(M_SEEN_FIRE);
        }
        /* boiling water burns considered fire damage */
        break;
    case 3:
        if (svm.mvitals[PM_SEWER_RAT].mvflags & G_GONE)
            pline_The("sink seems quite dirty.");
        else {
            mtmp = makemon(&mons[PM_SEWER_RAT], u.ux, u.uy, MM_NOMSG);
            if (mtmp)
                pline("Eek!  There's %s in the sink!",
                      (Blind || !canspotmon(mtmp)) ? "something squirmy"
                                                   : a_monnam(mtmp));
        }
        break;
    case 4:
        for (;;) {
            otmp = mkobj(POTION_CLASS, FALSE);
            if (otmp->otyp != POT_WATER)
                break;
            /* reject water and try again */
            obfree(otmp, (struct obj *) 0);
        }
        otmp->cursed = otmp->blessed = 0;
        pline("Some %s liquid flows from the faucet.",
              Blind ? "odd" : hcolor(OBJ_DESCR(objects[otmp->otyp])));
        otmp->dknown = !(Blind || Hallucination);
        otmp->quan++;       /* Avoid panic upon useup() */
        otmp->fromsink = 1; /* kludge for docall() */
        (void) dopotion(otmp);
        obfree(otmp, (struct obj *) 0);
        break;
    case 5:
        if (!(levl[u.ux][u.uy].looted & S_LRING)) {
            You("find a ring in the sink!");
            (void) mkobj_at(RING_CLASS, u.ux, u.uy, TRUE);
            levl[u.ux][u.uy].looted |= S_LRING;
            exercise(A_WIS, TRUE);
            newsym(u.ux, u.uy);
        } else
            pline("Some dirty %s backs up in the drain.", hliquid("water"));
        break;
    case 6:
        breaksink(u.ux, u.uy);
        break;
    case 7:
        pline_The("%s moves as though of its own will!", hliquid("water"));
        if ((svm.mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
            || !makemon(&mons[PM_WATER_ELEMENTAL], u.ux, u.uy, MM_NOMSG))
            pline("But it quiets down.");
        break;
    case 8:
        pline("Yuk, this %s tastes awful.", hliquid("water"));
        more_experienced(1, 0);
        newexplevel();
        break;
    case 9:
        pline("Gaggg... this tastes like sewage!  You vomit.");
        morehungry(rn1(30 - ACURR(A_CON), 11));
        vomit();
        break;
    case 10:
        pline("This %s contains toxic wastes!", hliquid("water"));
        if (!Unchanging) {
            You("undergo a freakish metamorphosis!");
            polyself(POLY_NOFLAGS);
        }
        break;
    /* more odd messages --JJB */
    case 11:
        Soundeffect(se_clanking_pipe, 50);
        You_hear("clanking from the pipes...");
        break;
    case 12:
        Soundeffect(se_sewer_song, 100);
        You_hear("snatches of song from among the sewers...");
        break;
    case 13:
        pline("Ew, what a stench!");
        create_gas_cloud(u.ux, u.uy, 1, 4);
        break;
    case 19:
        if (Hallucination) {
            pline("From the murky drain, a hand reaches up... --oops--");
            break;
        }
        /*FALLTHRU*/
    default:
        You("take a sip of %s %s.",
            rn2(3) ? (rn2(2) ? "cold" : "warm") : "hot",
            hliquid("water"));
    }
}

/* for #dip(potion.c) when standing on a sink */
void
dipsink(struct obj *obj)
{
    boolean try_call = FALSE,
            not_looted_yet = (levl[u.ux][u.uy].looted & S_LRING) == 0,
            is_hands = (obj == &hands_obj || (uarmg && obj == uarmg));

    if (!rn2(not_looted_yet ? 25 : 15)) {
        /* can't rely on using sink for unlimited scroll blanking; however,
           since sink will be converted into a fountain, hero can dip again */
        breaksink(u.ux, u.uy); /* "The pipes break!  Water spurts out!" */
        if (Glib && is_hands)
            Your("%s are still slippery.", fingers_or_gloves(TRUE));
        return;
    } else if (is_hands) {
        (void) wash_hands();
        return;
    } else if (obj->oclass != POTION_CLASS) {
        You("hold %s under the tap.", the(xname(obj)));
        if (water_damage(obj, (const char *) 0, TRUE) == ER_NOTHING)
            pline1(nothing_seems_to_happen);
        return;
    }

    /* at this point the object must be a potion */
    You("pour %s%s down the drain.", (obj->quan > 1L ? "one of " : ""),
        the(xname(obj)));
    switch (obj->otyp) {
    case POT_POLYMORPH:
        polymorph_sink();
        try_call = TRUE;
        break;
    case POT_OIL:
        if (!Blind) {
            pline("It leaves an oily film on the basin.");
            try_call = TRUE;
        } else {
            pline1(nothing_seems_to_happen);
        }
        break;
    case POT_ACID:
        /* acts like a drain cleaner product */
        try_call = TRUE;
        if (!Blind) {
            pline_The("drain seems less clogged.");
        } else if (!Deaf) {
            You_hear("a sucking sound.");
        } else {
            pline1(nothing_seems_to_happen);
            try_call = FALSE;
        }
        break;
    case POT_LEVITATION:
        sink_backs_up(u.ux, u.uy);
        try_call = TRUE;
        break;
    case POT_OBJECT_DETECTION:
        if (!(levl[u.ux][u.uy].looted & S_LRING)) {
            You("sense a ring lost down the drain.");
            try_call = TRUE;
            break;
        }
        /* FALLTHRU */
    case POT_GAIN_LEVEL:
    case POT_GAIN_ENERGY:
    case POT_MONSTER_DETECTION:
    case POT_FRUIT_JUICE:
    case POT_WATER:
        /* potions with no potionbreathe() effects, plus water.  if effects
           are added to potionbreathe these should go to that instead (except
           for water). */
        pline1(nothing_seems_to_happen);
        break;
    default:
        /* hero can feel the vapor on her skin, so no need to check Blind or
           breathless for this message */
        pline("A wisp of vapor rises up...");
        /* NB: potionbreathe calls trycall or makeknown as appropriate */
        if (!breathless(gy.youmonst.data) || haseyes(gy.youmonst.data))
            potionbreathe(obj);
        break;
    }
    if (try_call && obj->dknown)
        trycall(obj);
    useup(obj);
}

/* find a ring in a sink */
void
sink_backs_up(coordxy x, coordxy y)
{
    char buf[BUFSZ];

    if (!Blind)
        Strcpy(buf, "Muddy waste pops up from the drain");
    else if (!Deaf)
        Strcpy(buf, "You hear a sloshing sound"); /* Deaf-aware */
    else
        Sprintf(buf, "Something splashes you in the %s", body_part(FACE));
    pline("%s%s.", !Deaf ? "Flupp!  " : "", buf);

    if (!(levl[x][y].looted & S_LRING)) { /* once per sink */
        if (!Blind)
            You_see("a ring shining in its midst.");
        (void) mkobj_at(RING_CLASS, x, y, TRUE);
        newsym(x, y);
        exercise(A_DEX, TRUE);
        exercise(A_WIS, TRUE); /* a discovery! */
        levl[x][y].looted |= S_LRING;
    }
}

/*fountain.c*/

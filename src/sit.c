/* NetHack 3.7	sit.c	$NHDT-Date: 1627414178 2021/07/27 19:29:38 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.73 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

static void throne_sit_effect(void);
static int lay_an_egg(void);

/* take away the hero's money */
void
take_gold(void)
{
    struct obj *otmp, *nobj;
    int lost_money = 0;

    for (otmp = gi.invent; otmp; otmp = nobj) {
        nobj = otmp->nobj;
        if (otmp->oclass == COIN_CLASS) {
            lost_money = 1;
            remove_worn_item(otmp, FALSE);
            delobj(otmp);
        }
    }
    if (!lost_money) {
        You_feel("a strange sensation.");
    } else {
        You("notice you have no gold!");
        gc.context.botl = 1;
    }
}

/* maybe do something when hero sits on a throne */
static void
throne_sit_effect(void)
{
    if (rnd(6) > 4) {
        switch (rnd(13)) {
        case 1:
            (void) adjattrib(rn2(A_MAX), -rn1(4, 3), FALSE);
            losehp(rnd(10), "cursed throne", KILLED_BY_AN);
            break;
        case 2:
            (void) adjattrib(rn2(A_MAX), 1, FALSE);
            break;
        case 3:
            pline("A%s electric shock shoots through your body!",
                  (Shock_resistance) ? "n" : " massive");
            losehp(Shock_resistance ? rnd(6) : rnd(30), "electric chair",
                   KILLED_BY_AN);
            exercise(A_CON, FALSE);
            break;
        case 4:
            You_feel("much, much better!");
            if (Upolyd) {
                if (u.mh >= (u.mhmax - 5))
                    u.mhmax += 4;
                u.mh = u.mhmax;
            }
            if (u.uhp >= (u.uhpmax - 5)) {
                u.uhpmax += 4;
                if (u.uhpmax > u.uhppeak)
                    u.uhppeak = u.uhpmax;
            }
            u.uhp = u.uhpmax;
            u.ucreamed = 0;
            make_blinded(0L, TRUE);
            make_sick(0L, (char *) 0, FALSE, SICK_ALL);
            heal_legs(0);
            gc.context.botl = 1;
            break;
        case 5:
            take_gold();
            break;
        case 6:
            if (u.uluck + rn2(5) < 0) {
                You_feel("your luck is changing.");
                change_luck(1);
            } else
                makewish();
            break;
        case 7:
            {
                int cnt = rnd(10);

                /* Magical voice not affected by deafness */
                pline("A voice echoes:");
                SetVoice((struct monst *) 0, 0, 80, voice_throne);
                verbalize("Thine audience hath been summoned, %s!",
                          flags.female ? "Dame" : "Sire");
                while (cnt--)
                    (void) makemon(courtmon(), u.ux, u.uy, NO_MM_FLAGS);
                break;
            }
        case 8:
            /* Magical voice not affected by deafness */
            pline("A voice echoes:");
            SetVoice((struct monst *) 0, 0, 80, voice_throne);
            verbalize("By thine Imperious order, %s...",
                      flags.female ? "Dame" : "Sire");
            do_genocide(5); /* REALLY|ONTHRONE, see do_genocide() */
            break;
        case 9:
            /* Magical voice not affected by deafness */
            pline("A voice echoes:");
            SetVoice((struct monst *) 0, 0, 80, voice_throne);
            verbalize(
                      "A curse upon thee for sitting upon this most holy throne!");
            if (Luck > 0) {
                make_blinded(Blinded + rn1(100, 250), TRUE);
                change_luck((Luck > 1) ? -rnd(2) : -1);
            } else
                rndcurse();
            break;
        case 10:
            if (Luck < 0 || (HSee_invisible & INTRINSIC)) {
                if (gl.level.flags.nommap) {
                    pline("A terrible drone fills your head!");
                    make_confused((HConfusion & TIMEOUT) + (long) rnd(30),
                                  FALSE);
                } else {
                    pline("An image forms in your mind.");
                    do_mapping();
                }
            } else {
                /* avoid "vision clears" if hero can't see */
                if (!Blind) {
                    Your("vision becomes clear.");
                } else {
                    int num_of_eyes = eyecount(gy.youmonst.data);
                    const char *eye = body_part(EYE);

                    /* note: 1 eye case won't actually happen--can't
                       sit on throne when poly'd into always-levitating
                       floating eye and can't polymorph into Cyclops */
                    switch (num_of_eyes) { /* 2, 1, or 0 */
                    default:
                    case 2: /* more than 1 eye */
                        eye = makeplural(eye);
                        /*FALLTHRU*/
                    case 1: /* one eye (Cyclops, floating eye) */
                        Your("%s %s...", eye, vtense(eye, "tingle"));
                        break;
                    case 0: /* no eyes */
                        You("have a very strange feeling in your %s.",
                            body_part(HEAD));
                        break;
                    }
                }
                HSee_invisible |= FROMOUTSIDE;
                newsym(u.ux, u.uy);
            }
            break;
        case 11:
            if (Luck < 0) {
                You_feel("threatened.");
                aggravate();
            } else {
                You_feel("a wrenching sensation.");
                tele(); /* teleport him */
            }
            break;
        case 12:
            You("are granted an insight!");
            if (gi.invent) {
                /* rn2(5) agrees w/seffects() */
                identify_pack(rn2(5), FALSE);
            }
            break;
        case 13:
            Your("mind turns into a pretzel!");
            make_confused((HConfusion & TIMEOUT) + (long) rn1(7, 16),
                          FALSE);
            break;
        default:
            impossible("throne effect");
            break;
        }
    } else {
        if (is_prince(gy.youmonst.data) || u.uevent.uhand_of_elbereth)
            You_feel("very comfortable here.");
        else
            You_feel("somehow out of place...");
    }

    if (!rn2(3) && IS_THRONE(levl[u.ux][u.uy].typ)) {
        /* may have teleported */
        levl[u.ux][u.uy].typ = ROOM, levl[u.ux][u.uy].flags = 0;
        pline_The("throne vanishes in a puff of logic.");
        newsym(u.ux, u.uy);
    }
}

/* hero lays an egg */
static int
lay_an_egg(void)
{
    struct obj *uegg;

    if (!flags.female) {
        pline("%s can't lay eggs!",
              Hallucination
              ? "You may think you are a platypus, but a male still"
              : "Males");
        return ECMD_OK;
    } else if (u.uhunger < (int) objects[EGG].oc_nutrition) {
        You("don't have enough energy to lay an egg.");
        return ECMD_OK;
    } else if (eggs_in_water(gy.youmonst.data)) {
        if (!(Underwater || Is_waterlevel(&u.uz))) {
            pline("A splash tetra you are not.");
            return ECMD_OK;
        }
        if (Upolyd
            && (gy.youmonst.data == &mons[PM_GIANT_EEL]
                || gy.youmonst.data == &mons[PM_ELECTRIC_EEL])) {
            You("yearn for the Sargasso Sea.");
            return ECMD_OK;
        }
    }
    uegg = mksobj(EGG, FALSE, FALSE);
    uegg->spe = 1;
    uegg->quan = 1L;
    uegg->owt = weight(uegg);
    /* this sets hatch timers if appropriate */
    set_corpsenm(uegg, egg_type_from_parent(u.umonnum, FALSE));
    uegg->known = uegg->dknown = 1;
    You("%s an egg.", eggs_in_water(gy.youmonst.data) ? "spawn" : "lay");
    dropy(uegg);
    stackobj(uegg);
    morehungry((int) objects[EGG].oc_nutrition);
    return ECMD_TIME;
}

/* #sit command */
int
dosit(void)
{
    static const char sit_message[] = "sit on the %s.";
    register struct trap *trap = t_at(u.ux, u.uy);
    register int typ = levl[u.ux][u.uy].typ;

    if (u.usteed) {
        You("are already sitting on %s.", mon_nam(u.usteed));
        return ECMD_OK;
    }
    if (u.uundetected && is_hider(gy.youmonst.data) && u.umonnum != PM_TRAPPER)
        u.uundetected = 0; /* no longer on the ceiling */

    if (!can_reach_floor(FALSE)) {
        if (u.uswallow)
            There("are no seats in here!");
        else if (Levitation)
            You("tumble in place.");
        else
            You("are sitting on air.");
        return ECMD_OK;
    } else if (u.ustuck && !sticks(gy.youmonst.data)) {
        /* holding monster is next to hero rather than beneath, but
           hero is in no condition to actually sit at has/her own spot */
        if (humanoid(u.ustuck->data))
            pline("%s won't offer %s lap.", Monnam(u.ustuck), mhis(u.ustuck));
        else
            pline("%s has no lap.", Monnam(u.ustuck));
        return ECMD_OK;
    } else if (is_pool(u.ux, u.uy) && !Underwater) { /* water walking */
        goto in_water;
    }

    if (OBJ_AT(u.ux, u.uy)
        /* ensure we're not standing on the precipice */
        && !(uteetering_at_seen_pit(trap) || uescaped_shaft(trap))) {
        register struct obj *obj;

        obj = gl.level.objects[u.ux][u.uy];
        if (gy.youmonst.data->mlet == S_DRAGON && obj->oclass == COIN_CLASS) {
            You("coil up around your %shoard.",
                (obj->quan + money_cnt(gi.invent) < u.ulevel * 1000)
                ? "meager " : "");
        } else if (obj->otyp == TOWEL) {
            pline("It's probably not a good time for a picnic...");
        } else {
            You("sit on %s.", the(xname(obj)));
            if (obj->otyp == CORPSE && amorphous(&mons[obj->corpsenm]))
                pline("It's squishy...");
            else if (!(Is_box(obj) || objects[obj->otyp].oc_material == CLOTH))
                pline("It's not very comfortable...");
        }
    } else if (trap != 0 || (u.utrap && (u.utraptype >= TT_LAVA))) {
        if (u.utrap) {
            exercise(A_WIS, FALSE); /* you're getting stuck longer */
            if (u.utraptype == TT_BEARTRAP) {
                You_cant("sit down with your %s in the bear trap.",
                         body_part(FOOT));
                u.utrap++;
            } else if (u.utraptype == TT_PIT) {
                if (trap && trap->ttyp == SPIKED_PIT) {
                    You("sit down on a spike.  Ouch!");
                    losehp(Half_physical_damage ? rn2(2) : 1,
                           "sitting on an iron spike", KILLED_BY);
                    exercise(A_STR, FALSE);
                } else
                    You("sit down in the pit.");
                u.utrap += rn2(5);
            } else if (u.utraptype == TT_WEB) {
                You("sit in the spider web and get entangled further!");
                u.utrap += rn1(10, 5);
            } else if (u.utraptype == TT_LAVA) {
                /* Must have fire resistance or they'd be dead already */
                You("sit in the %s!", hliquid("lava"));
                if (Slimed)
                    burn_away_slime();
                u.utrap += rnd(4);
                losehp(d(2, 10), "sitting in lava",
                       KILLED_BY); /* lava damage */
            } else if (u.utraptype == TT_INFLOOR
                       || u.utraptype == TT_BURIEDBALL) {
                You_cant("maneuver to sit!");
                u.utrap++;
            }
        } else {
            You("sit down.");
            dotrap(trap, VIASITTING);
        }
    } else if ((Underwater || Is_waterlevel(&u.uz))
                && !eggs_in_water(gy.youmonst.data)) {
        if (Is_waterlevel(&u.uz))
            There("are no cushions floating nearby.");
        else
            You("sit down on the muddy bottom.");
    } else if (is_pool(u.ux, u.uy) && !eggs_in_water(gy.youmonst.data)) {
 in_water:
        You("sit in the %s.", hliquid("water"));
        if (!rn2(10) && uarm)
            (void) water_damage(uarm, "armor", TRUE);
        if (!rn2(10) && uarmf && uarmf->otyp != WATER_WALKING_BOOTS)
            (void) water_damage(uarm, "armor", TRUE);
    } else if (IS_SINK(typ)) {
        You(sit_message, defsyms[S_sink].explanation);
        Your("%s gets wet.",
             humanoid(gy.youmonst.data) ? "rump" : "underside");
    } else if (IS_ALTAR(typ)) {
        You(sit_message, defsyms[S_altar].explanation);
        altar_wrath(u.ux, u.uy);
    } else if (IS_GRAVE(typ)) {
        You(sit_message, defsyms[S_grave].explanation);
    } else if (typ == STAIRS) {
        You(sit_message, "stairs");
    } else if (typ == LADDER) {
        You(sit_message, "ladder");
    } else if (is_lava(u.ux, u.uy)) {
        /* must be WWalking */
        You(sit_message, hliquid("lava"));
        burn_away_slime();
        if (likes_lava(gy.youmonst.data)) {
            pline_The("%s feels warm.", hliquid("lava"));
            return ECMD_TIME;
        }
        pline_The("%s burns you!", hliquid("lava"));
        losehp(d((Fire_resistance ? 2 : 10), 10), /* lava damage */
               "sitting on lava", KILLED_BY);
    } else if (is_ice(u.ux, u.uy)) {
        You(sit_message, defsyms[S_ice].explanation);
        if (!Cold_resistance)
            pline_The("ice feels cold.");
    } else if (typ == DRAWBRIDGE_DOWN) {
        You(sit_message, "drawbridge");
    } else if (IS_THRONE(typ)) {
        You(sit_message, defsyms[S_throne].explanation);
        throne_sit_effect();
    } else if (lays_eggs(gy.youmonst.data)) {
        return lay_an_egg();
    } else {
        pline("Having fun sitting on the %s?", surface(u.ux, u.uy));
    }
    return ECMD_TIME;
}

/* curse a few inventory items at random! */
void
rndcurse(void)
{
    int nobj = 0;
    int cnt, onum;
    struct obj *otmp;
    static const char mal_aura[] = "feel a malignant aura surround %s.";

    if (u_wield_art(ART_MAGICBANE) && rn2(20)) {
        You(mal_aura, "the magic-absorbing blade");
        return;
    }

    if (Antimagic) {
        shieldeff(u.ux, u.uy);
        You(mal_aura, "you");
    }

    for (otmp = gi.invent; otmp; otmp = otmp->nobj) {
        /* gold isn't subject to being cursed or blessed */
        if (otmp->oclass == COIN_CLASS)
            continue;
        nobj++;
    }
    if (nobj) {
        for (cnt = rnd(6 / ((!!Antimagic) + (!!Half_spell_damage) + 1));
             cnt > 0; cnt--) {
            onum = rnd(nobj);
            for (otmp = gi.invent; otmp; otmp = otmp->nobj) {
                /* as above */
                if (otmp->oclass == COIN_CLASS)
                    continue;
                if (--onum == 0)
                    break; /* found the target */
            }
            /* the !otmp case should never happen; picking an already
               cursed item happens--avoid "resists" message in that case */
            if (!otmp || otmp->cursed)
                continue; /* next target */

            if (otmp->oartifact && spec_ability(otmp, SPFX_INTEL)
                && rn2(10) < 8) {
                pline("%s!", Tobjnam(otmp, "resist"));
                continue;
            }

            if (otmp->blessed)
                unbless(otmp);
            else
                curse(otmp);
        }
        update_inventory();
    }

    /* treat steed's saddle as extended part of hero's inventory */
    if (u.usteed && !rn2(4) && (otmp = which_armor(u.usteed, W_SADDLE)) != 0
        && !otmp->cursed) { /* skip if already cursed */
        if (otmp->blessed)
            unbless(otmp);
        else
            curse(otmp);
        if (!Blind) {
            pline("%s %s.", Yobjnam2(otmp, "glow"),
                  hcolor(otmp->cursed ? NH_BLACK : (const char *) "brown"));
            otmp->bknown = Hallucination ? 0 : 1; /* bypass set_bknown() */
        } else {
            otmp->bknown = 0; /* bypass set_bknown() */
        }
    }
}

/* remove a random INTRINSIC ability from hero.
   returns the intrinsic property which was removed,
   or 0 if nothing was removed. */
int
attrcurse(void)
{
    int ret = 0;

    switch (rnd(11)) {
    case 1:
        if (HFire_resistance & INTRINSIC) {
            HFire_resistance &= ~INTRINSIC;
            You_feel("warmer.");
            ret = FIRE_RES;
            break;
        }
        /*FALLTHRU*/
    case 2:
        if (HTeleportation & INTRINSIC) {
            HTeleportation &= ~INTRINSIC;
            You_feel("less jumpy.");
            ret = TELEPORT;
            break;
        }
        /*FALLTHRU*/
    case 3:
        if (HPoison_resistance & INTRINSIC) {
            HPoison_resistance &= ~INTRINSIC;
            You_feel("a little sick!");
            ret = POISON_RES;
            break;
        }
        /*FALLTHRU*/
    case 4:
        if (HTelepat & INTRINSIC) {
            HTelepat &= ~INTRINSIC;
            if (Blind && !Blind_telepat)
                see_monsters(); /* Can't sense mons anymore! */
            Your("senses fail!");
            ret = TELEPAT;
            break;
        }
        /*FALLTHRU*/
    case 5:
        if (HCold_resistance & INTRINSIC) {
            HCold_resistance &= ~INTRINSIC;
            You_feel("cooler.");
            ret = COLD_RES;
            break;
        }
        /*FALLTHRU*/
    case 6:
        if (HInvis & INTRINSIC) {
            HInvis &= ~INTRINSIC;
            You_feel("paranoid.");
            ret = INVIS;
            break;
        }
        /*FALLTHRU*/
    case 7:
        if (HSee_invisible & INTRINSIC) {
            HSee_invisible &= ~INTRINSIC;
            if (!See_invisible) {
                set_mimic_blocking();
                see_monsters();
                /* might not be able to see self anymore */
                newsym(u.ux, u.uy);
            }
            You("%s!", Hallucination ? "tawt you taw a puttie tat"
                                     : "thought you saw something");
            ret = SEE_INVIS;
            break;
        }
        /*FALLTHRU*/
    case 8:
        if (HFast & INTRINSIC) {
            HFast &= ~INTRINSIC;
            You_feel("slower.");
            ret = FAST;
            break;
        }
        /*FALLTHRU*/
    case 9:
        if (HStealth & INTRINSIC) {
            HStealth &= ~INTRINSIC;
            You_feel("clumsy.");
            ret = STEALTH;
            break;
        }
        /*FALLTHRU*/
    case 10:
        /* intrinsic protection is just disabled, not set back to 0 */
        if (HProtection & INTRINSIC) {
            HProtection &= ~INTRINSIC;
            You_feel("vulnerable.");
            ret = PROTECTION;
            break;
        }
        /*FALLTHRU*/
    case 11:
        if (HAggravate_monster & INTRINSIC) {
            HAggravate_monster &= ~INTRINSIC;
            You_feel("less attractive.");
            ret = AGGRAVATE_MONSTER;
            break;
        }
        /*FALLTHRU*/
    default:
        break;
    }
    return ret;
}

/*sit.c*/

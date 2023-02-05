/* NetHack 3.7	explode.c	$NHDT-Date: 1619553210 2021/04/27 19:53:30 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.77 $ */
/*      Copyright (C) 1990 by Ken Arromdee */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static int explosionmask(struct monst *, uchar, char);
static void engulfer_explosion_msg(uchar, char);

/* Note: Arrays are column first, while the screen is row first */
static const int explosion[3][3] = {
        { S_expl_tl, S_expl_ml, S_expl_bl },
        { S_expl_tc, S_expl_mc, S_expl_bc },
        { S_expl_tr, S_expl_mr, S_expl_br } };

/* what to do at [x+i][y+j] for i=-1,0,1 and j=-1,0,1 */
enum explode_action {
    EXPL_NONE = 0, /* not specified yet or no shield effect needed */
    EXPL_MON  = 1, /* monster is affected */
    EXPL_HERO = 2, /* hero is affected */
    EXPL_SKIP = 4  /* don't apply shield effect (out of bounds) */
};

/* check if shield effects are needed for location affected by explosion */
static int
explosionmask(
    struct monst *m, /* target monster (might be youmonst) */
    uchar adtyp,     /* damage type */
    char olet)       /* object class (only matters for AD_DISN) */
{
    int res = EXPL_NONE;

    if (m == &gy.youmonst) {
        switch (adtyp) {
        case AD_PHYS:
            /* leave 'res' with EXPL_NONE */
            break;
        case AD_MAGM:
            if (Antimagic)
                res = EXPL_HERO;
            break;
        case AD_FIRE:
            if (Fire_resistance)
                res = EXPL_HERO;
            break;
        case AD_COLD:
            if (Cold_resistance)
                res = EXPL_HERO;
            break;
        case AD_DISN:
            if ((olet == WAND_CLASS)
                ? (nonliving(m->data) || is_demon(m->data))
                : Disint_resistance)
                res = EXPL_HERO;
            break;
        case AD_ELEC:
            if (Shock_resistance)
                res = EXPL_HERO;
            break;
        case AD_DRST:
            if (Poison_resistance)
                res = EXPL_HERO;
            break;
        case AD_ACID:
            if (Acid_resistance)
                res = EXPL_HERO;
            break;
        default:
            impossible("explosion type %d?", adtyp);
            break;
        }

    } else {
        /* 'm' is a monster */
        switch (adtyp) {
        case AD_PHYS:
            break;
        case AD_MAGM:
            if (resists_magm(m))
                res = EXPL_MON;
            break;
        case AD_FIRE:
            if (resists_fire(m))
                res = EXPL_MON;
            break;
        case AD_COLD:
            if (resists_cold(m))
                res = EXPL_MON;
            break;
        case AD_DISN:
            if ((olet == WAND_CLASS)
                ? (nonliving(m->data) || is_demon(m->data)
                   || is_vampshifter(m))
                : !!resists_disint(m))
                res = EXPL_MON;
            break;
        case AD_ELEC:
            if (resists_elec(m))
                res = EXPL_MON;
            break;
        case AD_DRST:
            if (resists_poison(m))
                res = EXPL_MON;
            break;
        case AD_ACID:
            if (resists_acid(m))
                res = EXPL_MON;
            break;
        default:
            impossible("explosion type %d?", adtyp);
            break;
        }
    }
    return res;
}

static void
engulfer_explosion_msg(uchar adtyp, char olet)
{
    const char *adj = (char *) 0;

    if (digests(u.ustuck->data)) {
        switch (adtyp) {
        case AD_FIRE:
            adj = "heartburn";
            break;
        case AD_COLD:
            adj = "chilly";
            break;
        case AD_DISN:
            if (olet == WAND_CLASS)
                adj = "irradiated by pure energy";
            else
                adj = "perforated";
            break;
        case AD_ELEC:
            adj = "shocked";
            break;
        case AD_DRST:
            adj = "poisoned";
            break;
        case AD_ACID:
            adj = "an upset stomach";
            break;
        default:
            adj = "fried";
            break;
        }
        pline("%s gets %s!", Monnam(u.ustuck), adj);
    } else {
        switch (adtyp) {
        case AD_FIRE:
            adj = "toasted";
            break;
        case AD_COLD:
            adj = "chilly";
            break;
        case AD_DISN:
            if (olet == WAND_CLASS)
                adj = "overwhelmed by pure energy";
            else
                adj = "perforated";
            break;
        case AD_ELEC:
            adj = "shocked";
            break;
        case AD_DRST:
            adj = "intoxicated";
            break;
        case AD_ACID:
            adj = "burned";
            break;
        default:
            adj = "fried";
            break;
        }
        pline("%s gets slightly %s!", Monnam(u.ustuck), adj);
    }
}

/* Note: I had to choose one of three possible kinds of "type" when writing
 * this function: a wand type (like in zap.c), an adtyp, or an object type.
 * Wand types get complex because they must be converted to adtyps for
 * determining such things as fire resistance.  Adtyps get complex in that
 * they don't supply enough information--was it a player or a monster that
 * did it, and with a wand, spell, or breath weapon?  Object types share both
 * these disadvantages....
 *
 * Note: anything with a AT_BOOM AD_PHYS attack uses PHYS_EXPL_TYPE for type.
 *
 * Important note about Half_physical_damage:
 *      Unlike losehp(), explode() makes the Half_physical_damage adjustments
 *      itself, so the caller should never have done that ahead of time.
 *      It has to be done this way because the damage value is applied to
 *      things beside the player. Care is taken within explode() to ensure
 *      that Half_physical_damage only affects the damage applied to the hero.
 */
void
explode(
    coordxy x, coordxy y, /* explosion's location;
                           * adjacent spots are also affected */
    int type,     /* same as in zap.c; -(wand typ) for some WAND_CLASS */
    int dam,      /* damage amount */
    char olet,    /* object class or BURNING_OIL or MON_EXPLODE */
    int expltype) /* explosion type: controls color of explosion glyphs */
{
    int i, j, k, damu = dam;
    boolean starting = 1;
    boolean visible, any_shield;
    int uhurt = 0; /* 0=unhurt, 1=items damaged, 2=you and items damaged */
    const char *str = (const char *) 0;
    int idamres, idamnonres;
    struct monst *mtmp, *mdef = 0;
    uchar adtyp;
    int explmask[3][3]; /* 0=normal explosion, 1=do shieldeff, 2=do nothing */
    coordxy xx, yy;
    boolean shopdamage = FALSE, generic = FALSE,
            do_hallu = FALSE, inside_engulfer, grabbed, grabbing;
    coord grabxy;
    char hallu_buf[BUFSZ], killr_buf[BUFSZ];
    short exploding_wand_typ = 0;
    boolean you_exploding = (olet == MON_EXPLODE && type >= 0);
    boolean didmsg = FALSE;

    if (olet == WAND_CLASS) { /* retributive strike */
        /* 'type' is passed as (wand's object type * -1); save
           object type and convert 'type' itself to zap-type */
        if (type < 0) {
            type = -type;
            exploding_wand_typ = (short) type;
            /* most attack wands produce specific explosions;
               other types produce a generic magical explosion */
            if (objects[type].oc_dir == RAY
                && type != WAN_DIGGING && type != WAN_SLEEP) {
                type -= WAN_MAGIC_MISSILE;
                if (type < 0 || type > 9) {
                    impossible("explode: wand has bad zap type (%d).", type);
                    type = 0;
                }
            } else
                type = 0;
        }
        switch (Role_switch) {
        case PM_CLERIC:
        case PM_MONK:
        case PM_WIZARD:
            damu /= 5;
            break;
        case PM_HEALER:
        case PM_KNIGHT:
            damu /= 2;
            break;
        default:
            break;
        }
    } else if (olet == BURNING_OIL) {
        /* used to provide extra information to zap_over_floor() */
        exploding_wand_typ = POT_OIL;
    } else if (olet == SCROLL_CLASS) {
        /* ditto */
        exploding_wand_typ = SCR_FIRE;
    }
    /* muse_unslime: SCR_FIRE */
    if (expltype < 0) {
        /* hero gets credit/blame for killing this monster, not others */
        mdef = m_at(x, y);
        expltype = -expltype;
    }
    /* if hero is engulfed and caused the explosion, only hero and
       engulfer will be affected */
    inside_engulfer = (u.uswallow && type >= 0);
    /* held but not engulfed implies holder is reaching into second spot
       so might get hit by double damage */
    grabbed = grabbing = FALSE;
    if (u.ustuck && !u.uswallow) {
        if (Upolyd && sticks(gy.youmonst.data))
            grabbing = TRUE;
        else
            grabbed = TRUE;
        grabxy.x = u.ustuck->mx;
        grabxy.y = u.ustuck->my;
    } else
        grabxy.x = grabxy.y = 0; /* lint suppression */
    /* FIXME:
     *  It is possible for a grabber to be outside the explosion
     *  radius and reaching inside to hold the hero.  If so, it ought
     *  to take damage (the extra half of double damage).  It is also
     *  possible for poly'd hero to be outside the radius and reaching
     *  in to hold a monster.  Hero should take damage in that situation.
     *
     *  Probably the simplest way to handle this would be to expand
     *  the radius used when collecting targets but exclude everything
     *  beyond the regular radius which isn't reaching inside.  Then
     *  skip harm to gear of any extended targets when inflicting damage.
     */

    if (olet == MON_EXPLODE && !you_exploding) {
        /* when explode() is called recursively, gk.killer.name might change so
           we need to retain a copy of the current value for this explosion */
        str = strcpy(killr_buf, gk.killer.name);
        do_hallu = (Hallucination
                    && (strstri(str, "'s explosion")
                        || strstri(str, "s' explosion")));
    }
    if (type == PHYS_EXPL_TYPE) {
        /* currently only gas spores */
        adtyp = AD_PHYS;
    } else {
        /* If str is e.g. "flaming sphere's explosion" from above, we want to
         * still assign adtyp appropriately, but not replace str. */
        const char *adstr = NULL;

        switch (abs(type) % 10) {
        case 0:
            adstr = "magical blast";
            adtyp = AD_MAGM;
            break;
        case 1:
            adstr = (olet == BURNING_OIL) ? "burning oil"
                     : (olet == SCROLL_CLASS) ? "tower of flame" : "fireball";
            /* fire damage, not physical damage */
            adtyp = AD_FIRE;
            break;
        case 2:
            adstr = "ball of cold";
            adtyp = AD_COLD;
            break;
        case 4:
            adstr = (olet == WAND_CLASS) ? "death field"
                                         : "disintegration field";
            adtyp = AD_DISN;
            break;
        case 5:
            adstr = "ball of lightning";
            adtyp = AD_ELEC;
            break;
        case 6:
            adstr = "poison gas cloud";
            adtyp = AD_DRST;
            break;
        case 7:
            adstr = "splash of acid";
            adtyp = AD_ACID;
            break;
        default:
            impossible("explosion base type %d?", type);
            return;
        }
        if (!str)
            str = adstr;
    }

    any_shield = visible = FALSE;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) {
            xx = x + i - 1;
            yy = y + j - 1;
            if (!isok(xx, yy)) {
                explmask[i][j] = EXPL_SKIP;
                continue;
            }
            explmask[i][j] = EXPL_NONE;

            if (u_at(xx, yy)) {
                explmask[i][j] = explosionmask(&gy.youmonst, adtyp, olet);
            }
            /* can be both you and mtmp if you're swallowed or riding */
            mtmp = m_at(xx, yy);
            if (!mtmp && u_at(xx, yy))
                mtmp = u.usteed;
            if (mtmp && DEADMONSTER(mtmp))
                mtmp = 0;
            if (mtmp) {
                explmask[i][j] |= explosionmask(mtmp, adtyp, olet);
            }

            if (mtmp && cansee(xx, yy) && !canspotmon(mtmp))
                map_invisible(xx, yy);
            else if (!mtmp)
                (void) unmap_invisible(xx, yy);
            if (cansee(xx, yy))
                visible = TRUE;
            if ((explmask[i][j] & (EXPL_MON | EXPL_HERO)) != 0)
                any_shield = TRUE;
        }

    if (visible) {
        /* Start the explosion */
        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++) {
                if (explmask[i][j] == EXPL_SKIP)
                    continue;
                xx = x + i - 1;
                yy = y + j - 1;
                tmp_at(starting ? DISP_BEAM : DISP_CHANGE,
                       explosion_to_glyph(expltype, explosion[i][j]));
                tmp_at(xx, yy);
                starting = 0;
            }
        curs_on_u(); /* will flush screen and output */

        if (any_shield && flags.sparkle) { /* simulate shield effect */
            for (k = 0; k < SHIELD_COUNT; k++) {
                for (i = 0; i < 3; i++)
                    for (j = 0; j < 3; j++) {
                        xx = x + i - 1;
                        yy = y + j - 1;
                        if ((explmask[i][j] & (EXPL_MON | EXPL_HERO)) != 0)
                            /*
                             * Bypass tmp_at() and send the shield glyphs
                             * directly to the buffered screen.  tmp_at()
                             * will clean up the location for us later.
                             */
                            show_glyph(xx, yy,
                                       cmap_to_glyph(shield_static[k]));
                    }
                curs_on_u(); /* will flush screen and output */
                delay_output();
            }

            /* Cover last shield glyph with blast symbol. */
            for (i = 0; i < 3; i++)
                for (j = 0; j < 3; j++) {
                    xx = x + i - 1;
                    yy = y + j - 1;
                    if ((explmask[i][j] & (EXPL_MON | EXPL_HERO)) != 0)
                        show_glyph(xx, yy,
                                   explosion_to_glyph(expltype,
                                                      explosion[i][j]));
                }

        } else { /* delay a little bit. */
            delay_output();
            delay_output();
        }

        tmp_at(DISP_END, 0); /* clear the explosion */
    } else {
        if (olet == MON_EXPLODE || olet == TRAP_EXPLODE) {
            str = "explosion";
            generic = TRUE;
        }
        if (!Deaf && olet != SCROLL_CLASS) {
            Soundeffect(se_blast, 75);
            You_hear("a blast.");
            didmsg = TRUE;
        }
    }

    if (!Deaf && !didmsg)
        pline("Boom!");

    /* apply effects to monsters and floor objects first, in case the
       damage to the hero is fatal and leaves bones */
    if (dam) {
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++) {
                if (explmask[i][j] == EXPL_SKIP)
                    continue;
                xx = x + i - 1;
                yy = y + j - 1;
                if (u_at(xx, yy)) {
                    uhurt = ((explmask[i][j] & EXPL_HERO) != 0) ? 1 : 2;
                    /* If the player is attacking via polyself into something
                     * with an explosion attack, leave them (and their gear)
                     * unharmed, to avoid punishing them from using such
                     * polyforms creatively */
                    if (!gc.context.mon_moving && you_exploding)
                        uhurt = 0;
                } else if (inside_engulfer) {
                    /* for inside_engulfer, only <u.ux,u.uy> is affected */
                    continue;
                }
                idamres = idamnonres = 0;
                /* Affect the floor unless the player caused the explosion
                 * from inside their engulfer. */
                if (!(u.uswallow && !gc.context.mon_moving))
                    (void) zap_over_floor(xx, yy, type,
                                          &shopdamage, FALSE,
                                          exploding_wand_typ);

                mtmp = m_at(xx, yy);
                if (!mtmp && u_at(xx, yy))
                    mtmp = u.usteed;
                if (!mtmp)
                    continue;
                if (do_hallu) {
                    int tryct = 0;

                    /* replace "gas spore" with a different description
                       for each target (we can't distinguish personal names
                       like "Barney" here in order to suppress "the" below,
                       so avoid any which begins with a capital letter) */
                    do {
                        Sprintf(hallu_buf, "%s explosion",
                                s_suffix(rndmonnam((char *) 0)));
                    } while (*hallu_buf != lowc(*hallu_buf) && ++tryct < 20);
                    str = hallu_buf;
                }
                if (engulfing_u(mtmp)) {
                    engulfer_explosion_msg(adtyp, olet);
                } else if (cansee(xx, yy)) {
                    if (mtmp->m_ap_type)
                        seemimic(mtmp);
                    pline("%s is caught in the %s!", Monnam(mtmp), str);
                }

                if (adtyp == AD_FIRE) {
                    (void) burnarmor(mtmp);
                    ignite_items(mtmp->minvent);
                }
                idamres += destroy_mitem(mtmp, SCROLL_CLASS, (int) adtyp);
                idamres += destroy_mitem(mtmp, SPBOOK_CLASS, (int) adtyp);
                idamnonres += destroy_mitem(mtmp, POTION_CLASS, (int) adtyp);
                idamnonres += destroy_mitem(mtmp, RING_CLASS, (int) adtyp);
                idamnonres += destroy_mitem(mtmp, WAND_CLASS, (int) adtyp);

                if ((explmask[i][j] & EXPL_MON) != 0) {
                    golemeffects(mtmp, (int) adtyp, dam + idamres);
                    mtmp->mhp -= idamnonres;
                } else {
                    /* call resist with 0 and do damage manually so 1) we can
                     * get out the message before doing the damage, and 2) we
                     * can call mondied, not killed, if it's not your blast
                     */
                    int mdam = dam;

                    if (resist(mtmp, olet, 0, FALSE)) {
                        /* inside_engulfer: <xx,yy> == <u.ux,u.uy> */
                        if (cansee(xx, yy) || inside_engulfer)
                            pline("%s resists the %s!", Monnam(mtmp), str);
                        mdam = (dam + 1) / 2;
                    }
                    /* if grabber is reaching into hero's spot and
                       hero's spot is within explosion radius, grabber
                       gets hit by double damage */
                    if (grabbed && mtmp == u.ustuck && next2u(x, y))
                        mdam *= 2;
                    /* being resistant to opposite type of damage makes
                       target more vulnerable to current type of damage
                       (when target is also resistant to current type,
                       we won't get here) */
                    if (resists_cold(mtmp) && adtyp == AD_FIRE)
                        mdam *= 2;
                    else if (resists_fire(mtmp) && adtyp == AD_COLD)
                        mdam *= 2;
                    mtmp->mhp -= mdam;
                    mtmp->mhp -= (idamres + idamnonres);
                }
                if (DEADMONSTER(mtmp)) {
                    int xkflg = ((adtyp == AD_FIRE
                                  && completelyburns(mtmp->data))
                                 ? XKILL_NOCORPSE : 0);

                    if (!gc.context.mon_moving) {
                        xkilled(mtmp, XKILL_GIVEMSG | xkflg);
                    } else if (mdef && mtmp == mdef) {
                        /* 'mdef' killed self trying to cure being turned
                         * into slime due to some action by the player.
                         * Hero gets the credit (experience) and most of
                         * the blame (possible loss of alignment and/or
                         * luck and/or telepathy depending on mtmp) but
                         * doesn't break pacifism.  xkilled()'s message
                         * would be "you killed <mdef>" so give our own.
                         */
                        if (cansee(mtmp->mx, mtmp->my) || canspotmon(mtmp))
                            pline("%s is %s!", Monnam(mtmp),
                                  xkflg ? "burned completely"
                                        : nonliving(mtmp->data) ? "destroyed"
                                                                : "killed");
                        xkilled(mtmp, XKILL_NOMSG | XKILL_NOCONDUCT | xkflg);
                    } else {
                        if (xkflg)
                            adtyp = AD_RBRE; /* no corpse */
                        monkilled(mtmp, "", (int) adtyp);
                    }
                } else if (!gc.context.mon_moving) {
                    /* all affected monsters, even if mdef is set */
                    setmangry(mtmp, TRUE);
                }
            }
        }
    }

    /* Do your injury last */
    if (uhurt) {
        /* give message for any monster-induced explosion
           or player-induced one other than scroll of fire */
        if (Verbose(1, explode) && (type < 0 || olet != SCROLL_CLASS)) {
            if (do_hallu) { /* (see explanation above) */
                do {
                    Sprintf(hallu_buf, "%s explosion",
                            s_suffix(rndmonnam((char *) 0)));
                } while (*hallu_buf != lowc(*hallu_buf));
                str = hallu_buf;
            }
            You("are caught in the %s!", str);
            iflags.last_msg = PLNMSG_CAUGHT_IN_EXPLOSION;
        }
        /* do property damage first, in case we end up leaving bones */
        if (adtyp == AD_FIRE)
            burn_away_slime();
        if (Invulnerable) {
            damu = 0;
            You("are unharmed!");
        } else if (adtyp == AD_PHYS || adtyp == AD_ACID)
            damu = Maybe_Half_Phys(damu);
        if (adtyp == AD_FIRE) {
            (void) burnarmor(&gy.youmonst);
            ignite_items(gi.invent);
        }
        destroy_item(SCROLL_CLASS, (int) adtyp);
        destroy_item(SPBOOK_CLASS, (int) adtyp);
        destroy_item(POTION_CLASS, (int) adtyp);
        destroy_item(RING_CLASS, (int) adtyp);
        destroy_item(WAND_CLASS, (int) adtyp);

        ugolemeffects((int) adtyp, damu);
        if (uhurt == 2) {
            /* if poly'd hero is grabbing another victim, hero takes
               double damage (note: don't rely on u.ustuck here because
               that victim might have been killed when hit by the blast) */
            if (grabbing && dist2((int) grabxy.x, (int) grabxy.y, x, y) <= 2)
                damu *= 2;
            /* hero does not get same fire-resistant vs cold and
               cold-resistant vs fire double damage as monsters [why not?] */
            if (Upolyd)
                u.mh -= damu;
            else
                u.uhp -= damu;
            gc.context.botl = 1;
        }

        /* You resisted the damage, lets not keep that to ourselves */
        if (uhurt == 1)
            monstseesu_ad(adtyp);

        if (u.uhp <= 0 || (Upolyd && u.mh <= 0)) {
            if (Upolyd) {
                rehumanize();
            } else {
                if (olet == MON_EXPLODE) {
                    if (generic) /* explosion was unseen; str=="explosion", */
                        ;        /* gk.killer.name=="gas spore's explosion". */
                    else if (str != gk.killer.name && str != hallu_buf)
                        Strcpy(gk.killer.name, str);
                    gk.killer.format = KILLED_BY_AN;
                } else if (type >= 0 && olet != SCROLL_CLASS) {
                    gk.killer.format = NO_KILLER_PREFIX;
                    Snprintf(gk.killer.name, sizeof gk.killer.name,
                             "caught %sself in %s own %s", uhim(),
                             uhis(), str);
                } else {
                    gk.killer.format = (!strcmpi(str, "tower of flame")
                                     || !strcmpi(str, "fireball"))
                                        ? KILLED_BY_AN
                                        : KILLED_BY;
                    Strcpy(gk.killer.name, str);
                }
                if (iflags.last_msg == PLNMSG_CAUGHT_IN_EXPLOSION
                    || iflags.last_msg == PLNMSG_TOWER_OF_FLAME) /*seffects()*/
                    pline("It is fatal.");
                else
                    pline_The("%s is fatal.", str);
                /* Known BUG: BURNING suppresses corpse in bones data,
                   but done does not handle killer reason correctly */
                done((adtyp == AD_FIRE) ? BURNING : DIED);
            }
        }
        exercise(A_STR, FALSE);
    }

    if (shopdamage) {
        pay_for_damage((adtyp == AD_FIRE) ? "burn away"
                          : (adtyp == AD_COLD) ? "shatter"
                             : (adtyp == AD_DISN) ? "disintegrate"
                                : "destroy",
                       FALSE);
    }

    /* explosions are noisy */
    i = dam * dam;
    if (i < 50)
        i = 50; /* in case random damage is very small */
    if (inside_engulfer)
        i = (i + 3) / 4;
    wake_nearto(x, y, i);
}

struct scatter_chain {
    struct scatter_chain *next; /* pointer to next scatter item */
    struct obj *obj;            /* pointer to the object        */
    coordxy ox;                 /* location of                  */
    coordxy oy;                 /*      item                    */
    schar dx;                   /* direction of                 */
    schar dy;                   /*      travel                  */
    int range;                  /* range of object              */
    boolean stopped;            /* flag for in-motion/stopped   */
};

/*
 * scflags:
 *      VIS_EFFECTS     Add visual effects to display
 *      MAY_HITMON      Objects may hit monsters
 *      MAY_HITYOU      Objects may hit hero
 *      MAY_HIT         Objects may hit you or monsters
 *      MAY_DESTROY     Objects may be destroyed at random
 *      MAY_FRACTURE    Stone objects can be fractured (statues, boulders)
 */

/* returns number of scattered objects */
long
scatter(coordxy sx, coordxy sy,  /* location of objects to scatter */
        int blastforce,  /* force behind the scattering */
        unsigned int scflags,
        struct obj *obj) /* only scatter this obj        */
{
    register struct obj *otmp;
    register int tmp;
    int farthest = 0;
    uchar typ;
    long qtmp;
    boolean used_up;
    boolean individual_object = obj ? TRUE : FALSE;
    boolean shop_origin, lostgoods = FALSE;
    struct monst *mtmp, *shkp = 0;
    struct scatter_chain *stmp, *stmp2 = 0;
    struct scatter_chain *schain = (struct scatter_chain *) 0;
    long total = 0L;

    if (individual_object && (obj->ox != sx || obj->oy != sy))
        impossible("scattered object <%d,%d> not at scatter site <%d,%d>",
                   obj->ox, obj->oy, sx, sy);

    shop_origin = ((shkp = shop_keeper(*in_rooms(sx, sy, SHOPBASE))) != 0
                && costly_spot(sx, sy));
    if (shop_origin)
        credit_report(shkp, 0, TRUE);   /* establish baseline, without msgs */

    while ((otmp = (individual_object ? obj : gl.level.objects[sx][sy])) != 0) {
        if (otmp == uball || otmp == uchain) {
            boolean waschain = (otmp == uchain);

            Soundeffect(se_chain_shatters, 25);
            pline_The("chain shatters!");
            unpunish();
            if (waschain)
                continue;
        }
        if (otmp->quan > 1L) {
            qtmp = otmp->quan - 1L;
            if (qtmp > LARGEST_INT)
                qtmp = LARGEST_INT;
            qtmp = (long) rnd((int) qtmp);
            otmp = splitobj(otmp, qtmp);
        } else {
            obj = (struct obj *) 0; /* all used */
        }
        obj_extract_self(otmp);
        used_up = FALSE;

        /* 9 in 10 chance of fracturing boulders or statues */
        if ((scflags & MAY_FRACTURE) != 0
            && (otmp->otyp == BOULDER || otmp->otyp == STATUE)
            && rn2(10)) {
            if (otmp->otyp == BOULDER) {
                if (cansee(sx, sy)) {
                    pline("%s apart.", Tobjnam(otmp, "break"));
                } else {
                    Soundeffect(se_stone_breaking, 100);
                    You_hear("stone breaking.");
                }
                fracture_rock(otmp);
                place_object(otmp, sx, sy);
                if ((otmp = sobj_at(BOULDER, sx, sy)) != 0) {
                    /* another boulder here, restack it to the top */
                    obj_extract_self(otmp);
                    place_object(otmp, sx, sy);
                }
            } else {
                struct trap *trap;

                if ((trap = t_at(sx, sy)) && trap->ttyp == STATUE_TRAP)
                    deltrap(trap);
                if (cansee(sx, sy)) {
                    pline("%s.", Tobjnam(otmp, "crumble"));
                } else {
                    Soundeffect(se_stone_crumbling, 100); 
                    You_hear("stone crumbling.");
                }
                (void) break_statue(otmp);
                place_object(otmp, sx, sy); /* put fragments on floor */
            }
            newsym(sx, sy); /* in case it's beyond radius of 'farthest' */
            used_up = TRUE;

            /* 1 in 10 chance of destruction of obj; glass, egg destruction */
        } else if ((scflags & MAY_DESTROY) != 0
                   && (!rn2(10) || (objects[otmp->otyp].oc_material == GLASS
                                    || otmp->otyp == EGG))) {
            if (breaks(otmp, (coordxy) sx, (coordxy) sy))
                used_up = TRUE;
        }

        if (!used_up) {
            stmp = (struct scatter_chain *) alloc(sizeof *stmp);
            stmp->next = (struct scatter_chain *) 0;
            stmp->obj = otmp;
            stmp->ox = sx;
            stmp->oy = sy;
            tmp = rn2(N_DIRS); /* get the direction */
            stmp->dx = xdir[tmp];
            stmp->dy = ydir[tmp];
            tmp = blastforce - (otmp->owt / 40);
            if (tmp < 1)
                tmp = 1;
            stmp->range = rnd(tmp); /* anywhere up to that determ. by wt */
            if (farthest < stmp->range)
                farthest = stmp->range;
            stmp->stopped = FALSE;
            if (!schain)
                schain = stmp;
            else
                stmp2->next = stmp;
            stmp2 = stmp;
        }
    }

    while (farthest-- > 0) {
        for (stmp = schain; stmp; stmp = stmp->next) {
            if ((stmp->range-- > 0) && (!stmp->stopped)) {
                gt.thrownobj = stmp->obj; /* mainly in case it kills hero */
                gb.bhitpos.x = stmp->ox + stmp->dx;
                gb.bhitpos.y = stmp->oy + stmp->dy;
                typ = levl[gb.bhitpos.x][gb.bhitpos.y].typ;
                if (!isok(gb.bhitpos.x, gb.bhitpos.y)) {
                    gb.bhitpos.x -= stmp->dx;
                    gb.bhitpos.y -= stmp->dy;
                    stmp->stopped = TRUE;
                } else if (!ZAP_POS(typ)
                           || closed_door(gb.bhitpos.x, gb.bhitpos.y)) {
                    gb.bhitpos.x -= stmp->dx;
                    gb.bhitpos.y -= stmp->dy;
                    stmp->stopped = TRUE;
                } else if ((mtmp = m_at(gb.bhitpos.x, gb.bhitpos.y)) != 0) {
                    if (scflags & MAY_HITMON) {
                        stmp->range--;
                        if (ohitmon(mtmp, stmp->obj, 1, FALSE)) {
                            stmp->obj = (struct obj *) 0;
                            stmp->stopped = TRUE;
                        }
                    }
                } else if (u_at(gb.bhitpos.x, gb.bhitpos.y)) {
                    if (scflags & MAY_HITYOU) {
                        int hitvalu, hitu;

                        if (gm.multi)
                            nomul(0);
                        hitvalu = 8 + stmp->obj->spe;
                        if (bigmonst(gy.youmonst.data))
                            hitvalu++;
                        hitu = thitu(hitvalu, dmgval(stmp->obj, &gy.youmonst),
                                     &stmp->obj, (char *) 0);
                        if (!stmp->obj)
                            stmp->stopped = TRUE;
                        if (hitu) {
                            stmp->range -= 3;
                            stop_occupation();
                        }
                    }
                } else {
                    if (scflags & VIS_EFFECTS) {
                        /* tmp_at(gb.bhitpos.x, gb.bhitpos.y); */
                        /* delay_output(); */
                    }
                }
                stmp->ox = gb.bhitpos.x;
                stmp->oy = gb.bhitpos.y;
                if (IS_SINK(levl[stmp->ox][stmp->oy].typ))
                    stmp->stopped = TRUE;
                gt.thrownobj = (struct obj *) 0;
            }
        }
    }
    for (stmp = schain; stmp; stmp = stmp2) {
        coordxy x, y;
        boolean obj_left_shop = FALSE;

        stmp2 = stmp->next;
        x = stmp->ox;
        y = stmp->oy;
        if (stmp->obj) {
            if (x != sx || y != sy) {
                total += stmp->obj->quan;
                obj_left_shop = (shop_origin && !costly_spot(x, y));
            }
            if (!flooreffects(stmp->obj, x, y, "land")) {
                if (obj_left_shop
                    && strchr(u.urooms, *in_rooms(u.ux, u.uy, SHOPBASE))) {
                    /* At the moment this only takes on gold. While it is
                       simple enough to call addtobill for other items that
                       leave the shop due to scatter(), by default the hero
                       will get billed for the full shopkeeper asking-price
                       on the object's way out of shop. That can leave the
                       hero in a pickle. Even if the hero then manages to
                       retrieve the item and drop it back inside the shop,
                       the owed charges will only be reduced at that point
                       by the lesser shopkeeper buying-price.
                       The non-gold situation will likely get adjusted further.
                     */
                    if (stmp->obj->otyp == GOLD_PIECE) {
                        addtobill(stmp->obj, FALSE, FALSE, TRUE);
                        lostgoods = TRUE;
                    }
                }
                place_object(stmp->obj, x, y);
                stackobj(stmp->obj);
            }
        }
        free((genericptr_t) stmp);
        newsym(x, y);
    }
    newsym(sx, sy);
    if (u_at(sx, sy) && u.uundetected && hides_under(gy.youmonst.data))
        (void) hideunder(&gy.youmonst);
    if (((mtmp = m_at(sx, sy)) != 0) && mtmp->mtrapped)
        mtmp->mtrapped = 0;
    maybe_unhide_at(sx, sy);
    if (lostgoods) /* implies shop_origin and therefore shkp valid */
        credit_report(shkp, 1, FALSE);
    return total;
}

/*
 * Splatter burning oil from x,y to the surrounding area.
 *
 * This routine should really take a how and direction parameters.
 * The how is how it was caused, e.g. kicked verses thrown.  The
 * direction is which way to spread the flaming oil.  Different
 * "how"s would give different dispersal patterns.  For example,
 * kicking a burning flask will splatter differently from a thrown
 * flask hitting the ground.
 *
 * For now, just perform a "regular" explosion.
 */
void
splatter_burning_oil(coordxy x, coordxy y, boolean diluted_oil)
{
    int dmg = d(diluted_oil ? 3 : 4, 4);

/* ZT_SPELL(ZT_FIRE) = ZT_SPELL(AD_FIRE-1) = 10+(2-1) = 11 */
#define ZT_SPELL_O_FIRE 11 /* value kludge, see zap.c */
    explode(x, y, ZT_SPELL_O_FIRE, dmg, BURNING_OIL, EXPL_FIERY);
}

/* lit potion of oil is exploding; extinguish it as a light source before
   possibly killing the hero and attempting to save bones */
void
explode_oil(struct obj *obj, coordxy x, coordxy y)
{
    boolean diluted_oil = obj->odiluted;

    if (!obj->lamplit)
        impossible("exploding unlit oil");
    end_burn(obj, TRUE);
    splatter_burning_oil(x, y, diluted_oil);
}

/* Convert a damage type into an explosion display type. */
int
adtyp_to_expltype(const int adtyp)
{
    switch(adtyp) {
    case AD_ELEC:
        /* Electricity isn't magical, but there currently isn't an electric
         * explosion type. Magical is the next best thing. */
    case AD_SPEL:
    case AD_DREN:
    case AD_ENCH:
        return EXPL_MAGICAL;
    case AD_FIRE:
        return EXPL_FIERY;
    case AD_COLD:
        return EXPL_FROSTY;
    case AD_DRST:
    case AD_DRDX:
    case AD_DRCO:
    case AD_DISE:
    case AD_PEST:
    case AD_PHYS: /* gas spore */
        return EXPL_NOXIOUS;
    default:
        impossible("adtyp_to_expltype: bad explosion type %d", adtyp);
        return EXPL_FIERY;
    }
}

/* A monster explodes in a way that produces a real explosion (e.g. a sphere or
 * gas spore, not a yellow light or similar).
 * This is some common code between explmu() and explmm().
 */
void
mon_explodes(struct monst *mon, struct attack *mattk)
{
    int dmg;
    int type;
    if (mattk->damn) {
        dmg = d((int) mattk->damn, (int) mattk->damd);
    }
    else if (mattk->damd) {
        dmg = d((int) mon->data->mlevel + 1, (int) mattk->damd);
    }
    else {
        dmg = 0;
    }

    if (mattk->adtyp == AD_PHYS) {
        type = PHYS_EXPL_TYPE;
    }
    else if (mattk->adtyp >= AD_MAGM && mattk->adtyp <= AD_SPC2) {
        /* The -1, +20, *-1 math is to set it up as a 'monster breath' type for
         * the explosions (it isn't, but this is the closest analogue). */
        type = -((mattk->adtyp - 1) + 20);
    }
    else {
        impossible("unknown type for mon_explode %d", mattk->adtyp);
        return;
    }

    /* Kill it now so it won't appear to be caught in its own explosion.
     * Must check to see if already dead - which happens if this is called from
     * an AT_BOOM attack upon death. */
    if (!DEADMONSTER(mon)) {
        mondead(mon);
    }

    /* This might end up killing you, too; you never know...
     * also, it is used in explode() messages */
    Sprintf(gk.killer.name, "%s explosion",
            s_suffix(pmname(mon->data, Mgender(mon))));
    gk.killer.format = KILLED_BY_AN;

    explode(mon->mx, mon->my, type, dmg, MON_EXPLODE,
            adtyp_to_expltype(mattk->adtyp));

    /* reset killer */
    gk.killer.name[0] = '\0';
}

/*explode.c*/

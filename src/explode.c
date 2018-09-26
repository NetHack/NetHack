/* NetHack 3.6	explode.c	$NHDT-Date: 1522454717 2018/03/31 00:05:17 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.56 $ */
/*      Copyright (C) 1990 by Ken Arromdee */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* Note: Arrays are column first, while the screen is row first */
static int explosion[3][3] = { { S_explode1, S_explode4, S_explode7 },
                               { S_explode2, S_explode5, S_explode8 },
                               { S_explode3, S_explode6, S_explode9 } };

/* Note: I had to choose one of three possible kinds of "type" when writing
 * this function: a wand type (like in zap.c), an adtyp, or an object type.
 * Wand types get complex because they must be converted to adtyps for
 * determining such things as fire resistance.  Adtyps get complex in that
 * they don't supply enough information--was it a player or a monster that
 * did it, and with a wand, spell, or breath weapon?  Object types share both
 * these disadvantages....
 *
 * Important note about Half_physical_damage:
 *      Unlike losehp(), explode() makes the Half_physical_damage adjustments
 *      itself, so the caller should never have done that ahead of time.
 *      It has to be done this way because the damage value is applied to
 *      things beside the player. Care is taken within explode() to ensure
 *      that Half_physical_damage only affects the damage applied to the hero.
 */
void
explode(x, y, type, dam, olet, expltype)
int x, y;
int type; /* the same as in zap.c; passes -(wand typ) for some WAND_CLASS */
int dam;
char olet;
int expltype;
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
    boolean shopdamage = FALSE, generic = FALSE, physical_dmg = FALSE,
            do_hallu = FALSE, inside_engulfer, grabbed, grabbing;
    coord grabxy;
    char hallu_buf[BUFSZ], killr_buf[BUFSZ];
    short exploding_wand_typ = 0;

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
        case PM_PRIEST:
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
        if (Upolyd && sticks(youmonst.data))
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

    if (olet == MON_EXPLODE) {
        /* when explode() is called recursively, killer.name might change so
           we need to retain a copy of the current value for this explosion */
        str = strcpy(killr_buf, killer.name);
        do_hallu = (Hallucination
                    && (strstri(str, "'s explosion")
                        || strstri(str, "s' explosion")));
        adtyp = AD_PHYS;
    } else
        switch (abs(type) % 10) {
        case 0:
            str = "magical blast";
            adtyp = AD_MAGM;
            break;
        case 1:
            str = (olet == BURNING_OIL) ? "burning oil"
                     : (olet == SCROLL_CLASS) ? "tower of flame" : "fireball";
            /* fire damage, not physical damage */
            adtyp = AD_FIRE;
            break;
        case 2:
            str = "ball of cold";
            adtyp = AD_COLD;
            break;
        case 4:
            str = (olet == WAND_CLASS) ? "death field"
                                       : "disintegration field";
            adtyp = AD_DISN;
            break;
        case 5:
            str = "ball of lightning";
            adtyp = AD_ELEC;
            break;
        case 6:
            str = "poison gas cloud";
            adtyp = AD_DRST;
            break;
        case 7:
            str = "splash of acid";
            adtyp = AD_ACID;
            break;
        default:
            impossible("explosion base type %d?", type);
            return;
        }

    any_shield = visible = FALSE;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) {
            if (!isok(i + x - 1, j + y - 1)) {
                explmask[i][j] = 2;
                continue;
            } else
                explmask[i][j] = 0;

            if (i + x - 1 == u.ux && j + y - 1 == u.uy) {
                switch (adtyp) {
                case AD_PHYS:
                    explmask[i][j] = 0;
                    break;
                case AD_MAGM:
                    explmask[i][j] = !!Antimagic;
                    break;
                case AD_FIRE:
                    explmask[i][j] = !!Fire_resistance;
                    break;
                case AD_COLD:
                    explmask[i][j] = !!Cold_resistance;
                    break;
                case AD_DISN:
                    explmask[i][j] = (olet == WAND_CLASS)
                                         ? !!(nonliving(youmonst.data)
                                              || is_demon(youmonst.data))
                                         : !!Disint_resistance;
                    break;
                case AD_ELEC:
                    explmask[i][j] = !!Shock_resistance;
                    break;
                case AD_DRST:
                    explmask[i][j] = !!Poison_resistance;
                    break;
                case AD_ACID:
                    explmask[i][j] = !!Acid_resistance;
                    physical_dmg = TRUE;
                    break;
                default:
                    impossible("explosion type %d?", adtyp);
                    break;
                }
            }
            /* can be both you and mtmp if you're swallowed or riding */
            mtmp = m_at(i + x - 1, j + y - 1);
            if (!mtmp && i + x - 1 == u.ux && j + y - 1 == u.uy)
                mtmp = u.usteed;
            if (mtmp) {
                if (DEADMONSTER(mtmp))
                    explmask[i][j] = 2;
                else
                    switch (adtyp) {
                    case AD_PHYS:
                        break;
                    case AD_MAGM:
                        explmask[i][j] |= resists_magm(mtmp);
                        break;
                    case AD_FIRE:
                        explmask[i][j] |= resists_fire(mtmp);
                        break;
                    case AD_COLD:
                        explmask[i][j] |= resists_cold(mtmp);
                        break;
                    case AD_DISN:
                        explmask[i][j] |= (olet == WAND_CLASS)
                                              ? (nonliving(mtmp->data)
                                                 || is_demon(mtmp->data)
                                                 || is_vampshifter(mtmp))
                                              : resists_disint(mtmp);
                        break;
                    case AD_ELEC:
                        explmask[i][j] |= resists_elec(mtmp);
                        break;
                    case AD_DRST:
                        explmask[i][j] |= resists_poison(mtmp);
                        break;
                    case AD_ACID:
                        explmask[i][j] |= resists_acid(mtmp);
                        break;
                    default:
                        impossible("explosion type %d?", adtyp);
                        break;
                    }
            }
            if (mtmp && cansee(i + x - 1, j + y - 1) && !canspotmon(mtmp))
                map_invisible(i + x - 1, j + y - 1);
            else if (!mtmp)
                (void) unmap_invisible(i + x - 1, j + y - 1);
            if (cansee(i + x - 1, j + y - 1))
                visible = TRUE;
            if (explmask[i][j] == 1)
                any_shield = TRUE;
        }

    if (visible) {
        /* Start the explosion */
        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++) {
                if (explmask[i][j] == 2)
                    continue;
                tmp_at(starting ? DISP_BEAM : DISP_CHANGE,
                       explosion_to_glyph(expltype, explosion[i][j]));
                tmp_at(i + x - 1, j + y - 1);
                starting = 0;
            }
        curs_on_u(); /* will flush screen and output */

        if (any_shield && flags.sparkle) { /* simulate shield effect */
            for (k = 0; k < SHIELD_COUNT; k++) {
                for (i = 0; i < 3; i++)
                    for (j = 0; j < 3; j++) {
                        if (explmask[i][j] == 1)
                            /*
                             * Bypass tmp_at() and send the shield glyphs
                             * directly to the buffered screen.  tmp_at()
                             * will clean up the location for us later.
                             */
                            show_glyph(i + x - 1, j + y - 1,
                                       cmap_to_glyph(shield_static[k]));
                    }
                curs_on_u(); /* will flush screen and output */
                delay_output();
            }

            /* Cover last shield glyph with blast symbol. */
            for (i = 0; i < 3; i++)
                for (j = 0; j < 3; j++) {
                    if (explmask[i][j] == 1)
                        show_glyph(
                            i + x - 1, j + y - 1,
                            explosion_to_glyph(expltype, explosion[i][j]));
                }

        } else { /* delay a little bit. */
            delay_output();
            delay_output();
        }

        tmp_at(DISP_END, 0); /* clear the explosion */
    } else {
        if (olet == MON_EXPLODE) {
            str = "explosion";
            generic = TRUE;
        }
        if (!Deaf && olet != SCROLL_CLASS)
            You_hear("a blast.");
    }

    if (dam)
        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++) {
                if (explmask[i][j] == 2)
                    continue;
                if (i + x - 1 == u.ux && j + y - 1 == u.uy)
                    uhurt = (explmask[i][j] == 1) ? 1 : 2;
                /* for inside_engulfer, only <u.ux,u.uy> is affected */
                else if (inside_engulfer)
                    continue;
                idamres = idamnonres = 0;
                if (type >= 0 && !u.uswallow)
                    (void) zap_over_floor((xchar) (i + x - 1),
                                          (xchar) (j + y - 1), type,
                                          &shopdamage, exploding_wand_typ);

                mtmp = m_at(i + x - 1, j + y - 1);
                if (!mtmp && i + x - 1 == u.ux && j + y - 1 == u.uy)
                    mtmp = u.usteed;
                if (!mtmp)
                    continue;
                if (do_hallu) {
                    /* replace "gas spore" with a different description
                       for each target (we can't distinguish personal names
                       like "Barney" here in order to suppress "the" below,
                       so avoid any which begins with a capital letter) */
                    do {
                        Sprintf(hallu_buf, "%s explosion",
                                s_suffix(rndmonnam((char *) 0)));
                    } while (*hallu_buf != lowc(*hallu_buf));
                    str = hallu_buf;
                }
                if (u.uswallow && mtmp == u.ustuck) {
                    const char *adj = (char *) 0;

                    if (is_animal(u.ustuck->data)) {
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
                } else if (cansee(i + x - 1, j + y - 1)) {
                    if (mtmp->m_ap_type)
                        seemimic(mtmp);
                    pline("%s is caught in the %s!", Monnam(mtmp), str);
                }

                idamres += destroy_mitem(mtmp, SCROLL_CLASS, (int) adtyp);
                idamres += destroy_mitem(mtmp, SPBOOK_CLASS, (int) adtyp);
                idamnonres += destroy_mitem(mtmp, POTION_CLASS, (int) adtyp);
                idamnonres += destroy_mitem(mtmp, WAND_CLASS, (int) adtyp);
                idamnonres += destroy_mitem(mtmp, RING_CLASS, (int) adtyp);

                if (explmask[i][j] == 1) {
                    golemeffects(mtmp, (int) adtyp, dam + idamres);
                    mtmp->mhp -= idamnonres;
                } else {
                    /* call resist with 0 and do damage manually so 1) we can
                     * get out the message before doing the damage, and 2) we
                     * can call mondied, not killed, if it's not your blast
                     */
                    int mdam = dam;

                    if (resist(mtmp, olet, 0, FALSE)) {
                        /* inside_engulfer: <i+x-1,j+y-1> == <u.ux,u.uy> */
                        if (cansee(i + x - 1, j + y - 1) || inside_engulfer)
                            pline("%s resists the %s!", Monnam(mtmp), str);
                        mdam = (dam + 1) / 2;
                    }
                    /* if grabber is reaching into hero's spot and
                       hero's spot is within explosion radius, grabber
                       gets hit by double damage */
                    if (grabbed && mtmp == u.ustuck && distu(x, y) <= 2)
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

                    if (!context.mon_moving) {
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
                } else if (!context.mon_moving) {
                    /* all affected monsters, even if mdef is set */
                    setmangry(mtmp, TRUE);
                }
            }

    /* Do your injury last */
    if (uhurt) {
        /* give message for any monster-induced explosion
           or player-induced one other than scroll of fire */
        if (flags.verbose && (type < 0 || olet != SCROLL_CLASS)) {
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
        } else if (adtyp == AD_PHYS || physical_dmg)
            damu = Maybe_Half_Phys(damu);
        if (adtyp == AD_FIRE)
            (void) burnarmor(&youmonst);
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
            context.botl = 1;
        }

        if (u.uhp <= 0 || (Upolyd && u.mh <= 0)) {
            if (Upolyd) {
                rehumanize();
            } else {
                if (olet == MON_EXPLODE) {
                    if (generic) /* explosion was unseen; str=="explosion", */
                        ;        /* killer.name=="gas spore's explosion"    */
                    else if (str != killer.name && str != hallu_buf)
                        Strcpy(killer.name, str);
                    killer.format = KILLED_BY_AN;
                } else if (type >= 0 && olet != SCROLL_CLASS) {
                    killer.format = NO_KILLER_PREFIX;
                    Sprintf(killer.name, "caught %sself in %s own %s", uhim(),
                            uhis(), str);
                } else {
                    killer.format = (!strcmpi(str, "tower of flame")
                                     || !strcmpi(str, "fireball"))
                                        ? KILLED_BY_AN
                                        : KILLED_BY;
                    Strcpy(killer.name, str);
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
    xchar ox;                   /* location of                  */
    xchar oy;                   /*      item                    */
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
scatter(sx, sy, blastforce, scflags, obj)
int sx, sy;     /* location of objects to scatter */
int blastforce; /* force behind the scattering */
unsigned int scflags;
struct obj *obj; /* only scatter this obj        */
{
    register struct obj *otmp;
    register int tmp;
    int farthest = 0;
    uchar typ;
    long qtmp;
    boolean used_up;
    boolean individual_object = obj ? TRUE : FALSE;
    struct monst *mtmp;
    struct scatter_chain *stmp, *stmp2 = 0;
    struct scatter_chain *schain = (struct scatter_chain *) 0;
    long total = 0L;

    while ((otmp = (individual_object ? obj : level.objects[sx][sy])) != 0) {
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
                if (cansee(sx, sy))
                    pline("%s apart.", Tobjnam(otmp, "break"));
                else
                    You_hear("stone breaking.");
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
                if (cansee(sx, sy))
                    pline("%s.", Tobjnam(otmp, "crumble"));
                else
                    You_hear("stone crumbling.");
                (void) break_statue(otmp);
                place_object(otmp, sx, sy); /* put fragments on floor */
            }
            newsym(sx, sy); /* in case it's beyond radius of 'farthest' */
            used_up = TRUE;

            /* 1 in 10 chance of destruction of obj; glass, egg destruction */
        } else if ((scflags & MAY_DESTROY) != 0
                   && (!rn2(10) || (objects[otmp->otyp].oc_material == GLASS
                                    || otmp->otyp == EGG))) {
            if (breaks(otmp, (xchar) sx, (xchar) sy))
                used_up = TRUE;
        }

        if (!used_up) {
            stmp = (struct scatter_chain *) alloc(sizeof *stmp);
            stmp->next = (struct scatter_chain *) 0;
            stmp->obj = otmp;
            stmp->ox = sx;
            stmp->oy = sy;
            tmp = rn2(8); /* get the direction */
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
                bhitpos.x = stmp->ox + stmp->dx;
                bhitpos.y = stmp->oy + stmp->dy;
                typ = levl[bhitpos.x][bhitpos.y].typ;
                if (!isok(bhitpos.x, bhitpos.y)) {
                    bhitpos.x -= stmp->dx;
                    bhitpos.y -= stmp->dy;
                    stmp->stopped = TRUE;
                } else if (!ZAP_POS(typ)
                           || closed_door(bhitpos.x, bhitpos.y)) {
                    bhitpos.x -= stmp->dx;
                    bhitpos.y -= stmp->dy;
                    stmp->stopped = TRUE;
                } else if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
                    if (scflags & MAY_HITMON) {
                        stmp->range--;
                        if (ohitmon(mtmp, stmp->obj, 1, FALSE)) {
                            stmp->obj = (struct obj *) 0;
                            stmp->stopped = TRUE;
                        }
                    }
                } else if (bhitpos.x == u.ux && bhitpos.y == u.uy) {
                    if (scflags & MAY_HITYOU) {
                        int hitvalu, hitu;

                        if (multi)
                            nomul(0);
                        hitvalu = 8 + stmp->obj->spe;
                        if (bigmonst(youmonst.data))
                            hitvalu++;
                        hitu = thitu(hitvalu, dmgval(stmp->obj, &youmonst),
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
                        /* tmp_at(bhitpos.x, bhitpos.y); */
                        /* delay_output(); */
                    }
                }
                stmp->ox = bhitpos.x;
                stmp->oy = bhitpos.y;
            }
        }
    }
    for (stmp = schain; stmp; stmp = stmp2) {
        int x, y;

        stmp2 = stmp->next;
        x = stmp->ox;
        y = stmp->oy;
        if (stmp->obj) {
            if (x != sx || y != sy)
                total += stmp->obj->quan;
            place_object(stmp->obj, x, y);
            stackobj(stmp->obj);
        }
        free((genericptr_t) stmp);
        newsym(x, y);
    }

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
splatter_burning_oil(x, y)
int x, y;
{
/* ZT_SPELL(ZT_FIRE) = ZT_SPELL(AD_FIRE-1) = 10+(2-1) = 11 */
#define ZT_SPELL_O_FIRE 11 /* value kludge, see zap.c */
    explode(x, y, ZT_SPELL_O_FIRE, d(4, 4), BURNING_OIL, EXPL_FIERY);
}

/* lit potion of oil is exploding; extinguish it as a light source before
   possibly killing the hero and attempting to save bones */
void
explode_oil(obj, x, y)
struct obj *obj;
int x, y;
{
    if (!obj->lamplit)
        impossible("exploding unlit oil");
    end_burn(obj, TRUE);
    splatter_burning_oil(x, y);
}

/*explode.c*/
